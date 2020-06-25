/* TRANSACT.C   (C) Copyright "Fish" (David B. Trout), 2017-2020     */
/*              (C) Copyright Bob Wood, 2019-2020                    */
/*      Defines Transactional Execution Facility instructions        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html)                      */
/*   as modifications to Hercules.                                   */

/*-------------------------------------------------------------------*/
/* This module implements the z/Architecture Transactional-Execution */
/* Facility as documented in IBM reference manual SA22-7832-12 "The  */
/* z/Architecture Principles of Operation". Specifically chapter 5   */
/* "Program Execution" pages 5-89 to page 5-109 contain a detailed   */
/* description of the "Transactional-Execution Facility".            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _TRANSACT_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "transact.h"
#include "hexdumpe.h"

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) \
&& !defined( DID_TXF_DEBUGGING )
   #define   DID_TXF_DEBUGGING
/*-------------------------------------------------------------------*/
/*                      TXF Debugging                                */
/*-------------------------------------------------------------------*/

#define ENABLE_TXF_DEBUG   0    // 0: never, 1: always, #undef: maybe
//#define ENABLE_TXF_PTT          // #define: enable, #undef: disable
//#define NO_MADDR_L_OPTIMIZE     // CAREFUL! See PROGRAMMING NOTE!

// If no debugging preference specified, but this is a DEBUG build ...
#if (!defined( ENABLE_TXF_DEBUG ) && defined( DEBUG )) \
 || ( defined( ENABLE_TXF_DEBUG ) && ENABLE_TXF_DEBUG )
  #define TXF_DEBUG                   // ... then enable debugging
#endif

#if defined( TXF_DEBUG )
  #define  ENABLE_TRACING_STMTS   1   // (Fish: DEBUGGING)
  #include "dbgtrace.h"               // (Fish: DEBUGGING)
  #define  NO_TXF_OPTIMIZE            // (Fish: DEBUGGING)
#endif

#if defined( NO_TXF_OPTIMIZE )
  #if defined( _MSVC_ )
    #pragma optimize( "", off )       // (for reliable breakpoints)
  #else // e.g. Linux/gcc
    #pragma GCC optimize ("O0")       // (for reliable breakpoints)
  #endif
#endif

#if !defined( ENABLE_TXF_PTT )
  #undef  PTT_TXF
  #define PTT_TXF( ... )              // (nothing)
#endif

#endif // DID_TXF_DEBUGGING

#if defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY )
/*-------------------------------------------------------------------*/
/* B2E8 PPA   - Perform Processor Assist                     [RRF-c] */
/*-------------------------------------------------------------------*/

#define PPA_MAX_HELP_THRESHOLD  16
#define PPA_MED_HELP_THRESHOLD   8
#define PPA_MIN_HELP_THRESHOLD   1

DEF_INST( perform_processor_assist )
{
int     r1, r2;                         /* Operand register numbers  */
int     m3;                             /* M3 Mask value             */
U32     abort_count;                    /* Transaction Abort count   */

    RRF_M( inst, regs, r1, r2, m3 );

    TRAN_INSTR_CHECK( regs );

    /* Retrieve abort count */
    abort_count = regs->GR_L( r1 );

    PTT_TXF( "TXF PPA", m3, abort_count, 0 );

    switch (m3)
    {
    case 1: // Transaction Abort Assist
    {
        /* Provide least amount of assistance required */
        if (abort_count >= PPA_MAX_HELP_THRESHOLD)
        {
            /* Provide maximal assistance */
            // TODO... do something useful
        }
        else if (abort_count >= PPA_MED_HELP_THRESHOLD)
        {
            /* Provide medium assistance */
            // TODO... do something useful
        }
        else if (abort_count >= PPA_MIN_HELP_THRESHOLD)
        {
            /* Provide minimal assistance */
            // TODO... do something useful
        }
        else // zero!
        {
            /* Provide NO assistance at all */
            // (why are you wasting my time?!)
        }
        return;
    }

    default:     /* (unknown/unsupported assist) */
        return;  /* (ignore unsupported assists) */
    }

    UNREACHABLE_CODE( return );

} /* end DEF_INST( perform_processor_assist ) */

#endif /* defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY ) */


#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
/*-------------------------------------------------------------------*/
/* B2EC ETND  - Extract Transaction Nesting Depth              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( extract_transaction_nesting_depth )
{
int     r1, r2;                         /* Operand register numbers  */

    RRE( inst, regs, r1, r2 );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF ETND", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    CONTRAN_INSTR_CHECK( regs );

    regs->GR_L(r1) = (U32) regs->txf_tnd;

    PTT_TXF( "TXF ETND", 0, 0, regs->txf_tnd );

} /* end DEF_INST( extract_transaction_nesting_depth ) */


/*-------------------------------------------------------------------*/
/* B2F8 TEND - Transaction End  (CONSTRAINED or unconstrained)   [S] */
/*-------------------------------------------------------------------*/
DEF_INST( transaction_end )
{
int         i, j;                       /* (work)                    */
int         b2;                         /* Base of effective addr    */
VADR        effective_addr2;            /* Effective address         */
BYTE       *altaddr;
BYTE       *saveaddr;
BYTE       *mainaddr;
TPAGEMAP   *pmap;
int         txf_tac;

    S( inst, regs, b2, effective_addr2 );

    TRAN_EXECUTE_INSTR_CHECK( regs );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF TEND", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    if (regs->txf_abortctr)
    {
        PTT_TXF( "*TXF TEND", regs->txf_abortctr, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_PGMCHK, regs->txf_random_tac );
        UNREACHABLE_CODE( return );
    }

    /* Set condition code based on CURRENT transaction level */
    if (!regs->txf_tnd)
    {
        /* Not currently in transactional-execution mode.
           Set CC 2 and treat as no-op (i.e. just return
           since there is no active transaction to end!)
        */
        PTT_TXF( "*TXF TEND", 0, 0, 0 );
        regs->psw.cc = 2;   /* CPU wasn't in transaction mode! */
        return;             /* Nothing to do! Just return! */
    }

    /* CPU was in transaction-execution mode at start of operation */
    regs->psw.cc = 0;

    /*-----------------------------------------------------*/
    /*  Serialize TEND processing by obtaining INTLOCK     */
    /*  and synchronizing the CPUS.                        */
    /*-----------------------------------------------------*/
    OBTAIN_INTLOCK( regs );
    {
        bool   txf_contran;      /* (saved original value) */
        U16    txf_abortctr;     /* (saved original value) */
        BYTE*  txf_tbeginc_aip;  /* (saved original value) */
        BYTE*  txf_aie;          /* (saved original value) */
        int    txf_aie_off2;     /* (saved original value) */

        SYNCHRONIZE_CPUS( regs );

        /* Decrease nesting depth */
        regs->txf_tnd--;

        /* Still in transaction-execution mode? */
        if (regs->txf_tnd)
        {
            /*--------------------------------------------*/
            /*           NESTED TRANSACTION END           */
            /*--------------------------------------------*/

            if (TXF_TRACE( SUCCESS, regs->txf_contran ))
            {
                // "TXF: Successful %s Nested TEND for TND %d => %d"
                WRMSG( HHC17700, "D", regs->txf_contran ? "Cons"
                    : "Uncons", regs->txf_tnd + 1, regs->txf_tnd );
            }

            /* If we're now at or below the highest nesting level
               that allowed AR changes or float instructions, then
               enable (set) the corresponding control flag.
               Otherwise leave it alone (should already be off).
            */
            if (regs->txf_tnd <= regs->txf_higharchange)
            {
                regs->txf_higharchange = regs->txf_tnd;
                regs->txf_ctlflag |= TXF_CTL_AR;
            }

            if (regs->txf_tnd <= regs->txf_highfloat)
            {
                regs->txf_highfloat = regs->txf_tnd;
                regs->txf_ctlflag |= TXF_CTL_FLOAT;
            }

            /* Set PIFC for this nesting level */
            regs->txf_pifc = regs->txf_pifctab[ regs->txf_tnd - 1 ];

            /* Remain in transactional-execution mode */
            PTT_TXF( "TXF TEND", 0, regs->txf_contran, regs->txf_tnd );
            RELEASE_INTLOCK( regs );
            return;
        }

        /*---------------------------------------------------------*/
        /*         THE OUTERMOST TRANSACTION HAS ENDED             */
        /*---------------------------------------------------------*/
        /* If an abort code is already set, abort the transaction  */
        /* and exit                                                */
        /*---------------------------------------------------------*/
        if (regs->txf_tac)
        {
            PTT_TXF( "*TXF TEND", regs->txf_tac, regs->txf_contran, regs->txf_tnd );
            regs->txf_tnd++; // (prevent 'abort_transaction' crash)
            ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_CC, regs->txf_tac );
            UNREACHABLE_CODE( return );
        }

        /*---------------------------------------------------------*/
        /* End transaction normally -- IF POSSIBLE (no conflicts)  */
        /*---------------------------------------------------------*/

        txf_contran     = regs->txf_contran;        /* save */
        txf_abortctr    = regs->txf_abortctr;       /* save */
        txf_tbeginc_aip = regs->txf_tbeginc_aip;    /* save */
        txf_aie         = regs->txf_aie;            /* save */
        txf_aie_off2    = regs->txf_aie_off2;       /* save */

        regs->txf_contran     = false;              /* reset */
        regs->txf_abortctr    = 0;                  /* reset */
        regs->txf_tbeginc_aip = NULL;               /* reset */
        regs->txf_aie         = NULL;               /* reset */
        regs->txf_aie_off2    = 0;                  /* reset */

        /*---------------------------------------------------------*/
        /*  Scan the page map table.  There is one entry in the    */
        /*  page map table for each page referenced while in       */
        /*  transaction mode.  There is also a cache line map      */
        /*  that keeps track of what cache lines within the        */
        /*  page have been referenced and whether the reference    */
        /*  was a store or a fetch.  The current data in the       */
        /*  cache line is saved when the alternate entry is        */
        /*  created. This saved data MUST match what is in main    */
        /*  storage now, or the transation will be aborted with    */
        /*  a conflict, since that means that some other CPU or    */
        /*  the channel subsystem has stored into the cache line,  */
        /*---------------------------------------------------------*/

        regs->txf_conflict = 0;
        pmap = regs->txf_pagesmap;

        for (i=0; i < regs->txf_pgcnt; i++, pmap++)
        {
            for (j=0; j < ZCACHE_LINE_PAGE; j++)
            {
                if (pmap->cachemap[j] == CM_CLEAN)
                    continue;

                mainaddr = pmap->mainpageaddr + (j << ZCACHE_LINE_SHIFT);
                saveaddr = pmap->altpageaddr  + (j << ZCACHE_LINE_SHIFT) + ZPAGEFRAME_PAGESIZE;

                if (memcmp( saveaddr, mainaddr, ZCACHE_LINE_SIZE ) == 0)
                    continue;

                /*--------------------------------------*/
                /*          TRANSACTION FAILURE!        */
                /*--------------------------------------*/

                if (pmap->virtpageaddr)
                    regs->txf_conflict = pmap->virtpageaddr + (j << ZCACHE_LINE_SHIFT);
                else
                    regs->txf_conflict = 0;

                txf_tac = (pmap->cachemap[j] == CM_STORED) ?
                    TAC_STORE_CNF : TAC_FETCH_CNF;

                PTT_TXF( "*TXF TEND", txf_tac, regs->txf_contran, regs->txf_tnd );

                regs->txf_contran     = txf_contran;      /* restore */
                regs->txf_abortctr    = txf_abortctr;     /* restore */
                regs->txf_tbeginc_aip = txf_tbeginc_aip;  /* restore */
                regs->txf_aie         = txf_aie;          /* restore */
                regs->txf_aie_off2    = txf_aie_off2;     /* restore */

                regs->txf_tnd++; // (prevent 'abort_transaction' crash)
                ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_CC, txf_tac );
                UNREACHABLE_CODE( return );
            }
        }

        /*---------------------------------------------------------*/
        /*                 TRANSACTION SUCCESS!                    */
        /*---------------------------------------------------------*/
        /*  We have now validated all of the cache lines that we   */
        /*  touched, and all other CPUs are dormant.   Now update  */
        /*  the real cache lines from the shadow cache lines.      */
        /*---------------------------------------------------------*/

        if (TXF_TRACE( SUCCESS, txf_contran ))
        {
            // "TXF: Successful Outermost %s TEND"
            WRMSG( HHC17701, "D", txf_contran ? "Cons" : "Uncons" );
        }

        pmap = regs->txf_pagesmap;

        for (i=0; i < regs->txf_pgcnt; i++, pmap++)
        {
            if (TXF_TRACE_PAGES( txf_contran ))
            {
                // "TXF: virt 0x%16.16"PRIX64", abs 0x%16.16"PRIX64", alt 0x%16.16"PRIX64
                WRMSG( HHC17704, "D", (U64) pmap->virtpageaddr,
                    (U64)(pmap->mainpageaddr - regs->mainstor),
                    (U64) pmap->altpageaddr );
            }

            for (j=0; j < ZCACHE_LINE_PAGE; j++)
            {
                if (pmap->cachemap[j] != CM_STORED)
                    continue;

                mainaddr = pmap->mainpageaddr + (j << ZCACHE_LINE_SHIFT);
                altaddr  = pmap->altpageaddr  + (j << ZCACHE_LINE_SHIFT);

                memcpy( mainaddr, altaddr, ZCACHE_LINE_SIZE );

                if (TXF_TRACE_LINES( txf_contran ))
                {
                    // "TXF: We stored:  +"
                    dump_cache( DUMP_PFX( HHC17707 ), j, altaddr );
                }
            }
        }

        /* Mark the page map as now being empty */
        regs->txf_pgcnt = 0;

        /*------------------------------------------*/
        /*  We are done! Release INTLOCK and exit.  */
        /*------------------------------------------*/

        PTT_TXF( "TXF TEND", 0, 0, 0 );

        /* Exiting from transactional-execution mode... */
        UPDATE_SYSBLK_TRANSCPUS( -1 );
    }
    RELEASE_INTLOCK( regs );

} /* end DEF_INST( transaction_end ) */


/*-------------------------------------------------------------------*/
/* B2FC TABORT - Transaction Abort                               [S] */
/*-------------------------------------------------------------------*/
DEF_INST( transaction_abort )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );

    CONTRAN_INSTR_CHECK( regs );
    TRAN_EXECUTE_INSTR_CHECK( regs );

    if (effective_addr2 <= 255)
    {
        PTT_TXF( "*TXF TABORT", effective_addr2, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    if (!regs->txf_tnd)
    {
        PTT_TXF( "*TXF TABORT", 0, 0, 0 );
        regs->program_interrupt( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    CONTRAN_INSTR_CHECK( regs );

    /* CC in transaction abort PSW = 2 or 3 based on operand2 bit 63 */
    regs->txf_tapsw.cc = (effective_addr2 & 0x1) == 0 ? 2 : 3;

    /* Abort the transaction */

    PTT_TXF( "TXF TABORT", effective_addr2, regs->txf_contran, regs->txf_tnd );
    ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_CC, (int) effective_addr2 );
    UNREACHABLE_CODE( return );

} /* end DEF_INST( transaction_abort ) */


/*-------------------------------------------------------------------*/
/* E325 NTSTG - Nontransactional Store                       [RXY-a] */
/*-------------------------------------------------------------------*/
DEF_INST( nontransactional_store )
{
int     r1;                             /* Value of r1 field         */
int     b2;                             /* Base of effective address */
VADR    effective_addr2;                /* Effective address         */

    RXY( inst, regs, r1, b2, effective_addr2 );

    CONTRAN_INSTR_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

    PTT_TXF( "TXF NTSTG", regs->GR_G( r1 ), effective_addr2, 0 );

    /* Nontransactionally store register contents at operand address */
    regs->txf_NTSTG = true;
    ARCH_DEP( vstore8 )( regs->GR_G( r1 ), effective_addr2, b2, regs );

} /* end DEF_INST( nontransactional_store ) */


/* (forward reference) */
void ARCH_DEP( process_tbegin )( bool txf_contran, REGS* regs, S16 i2, TDB* tdb );

/*-------------------------------------------------------------------*/
/* E560 TBEGIN - Transaction Begin      (unconstrained)        [SIL] */
/*-------------------------------------------------------------------*/
/* Begin an unconstrained transaction.  If already in transaction    */
/* mode, just increment the nesting level, otherwise start the       */
/* transaction.                                                      */
/*-------------------------------------------------------------------*/
DEF_INST( transaction_begin )
{
S16     i2;                             /* 16-bit immediate value    */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
TDB*    tdb = NULL;                     /* Pointer to TDB            */

    SIL( inst, regs, i2, b1, effective_addr1 );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF TBEGIN", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    TRAN_EXECUTE_INSTR_CHECK( regs );

    /* Unconstrained: PIFC of 3 is invalid */
    if ((i2 & TXF_CTL_PIFC) == 3)
    {
        PTT_TXF( "*TXF TBEGIN", i2, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    /* Unconstrained: if b1 non-zero TDB must be aligned else ignore */
    if (b1)
    {
        RADR  real_tdb;
        int   stid;

        DW_CHECK( effective_addr1, regs );

        /* Check that all bytes of the TDB are accessible */
        ARCH_DEP( validate_operand )( effective_addr1, b1,
                                      sizeof( TDB ) - 1,
                                      ACCTYPE_WRITE, regs );

        /* Convert TDB address to absolute mainstor address */
        VERIFY( ARCH_DEP( virt_to_real )( &real_tdb, &stid, effective_addr1,
                                  b1, regs, ACCTYPE_WRITE ) == 0);
        real_tdb = APPLY_PREFIXING( real_tdb, regs->PX );
        tdb = (TDB*)(regs->mainstor + real_tdb);
    }

    OBTAIN_INTLOCK( regs );
    {
        /* Let our helper function do all the grunt work */
        regs->txf_tdb_addr = effective_addr1;
        PTT_TXF( "TXF TBEGIN", 0, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( process_tbegin )( false, regs, i2, tdb );
    }
    RELEASE_INTLOCK( regs );

} /* end DEF_INST( transaction_begin ) */


#if defined( FEATURE_050_CONSTR_TRANSACT_FACILITY )
/*-------------------------------------------------------------------*/
/* E561 TBEGINC - Transaction Begin      (CONSTRAINED)         [SIL] */
/*-------------------------------------------------------------------*/
/*  Begin a CONSTRAINED transaction.   The PSW address of this       */
/*  instruction is saved in the transaction data area which is part  */
/*  of the REGS area.  If already in CONSTRAINED transaction mode,   */
/*  the transaction is aborted and a program exception will be       */
/*  generated.  If in the unconstrained transaction mode, it will    */
/*  be treated as an unconstrained transaction.                      */
/*-------------------------------------------------------------------*/
DEF_INST( transaction_begin_constrained )
{
S16     i2;                             /* 16-bit immediate value    */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */

    SIL( inst, regs, i2, b1, effective_addr1 );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF TBEGINC", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    TRAN_EXECUTE_INSTR_CHECK( regs );

    /* CONSTRAINED: Specification Exception if b1 is non-zero */
    if (b1)
    {
        PTT_TXF( "*TXF TBEGINC", b1, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    /* CONSTRAINED: ignore some i2 bits */
    i2 &= ~(TXF_CTL_FLOAT | TXF_CTL_PIFC);

    OBTAIN_INTLOCK( regs );
    {
        /* Let our helper function do all the grunt work */
        regs->txf_tdb_addr = 0x1800;
        PTT_TXF( "TXF TBEGINC", 0, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( process_tbegin )( true, regs, i2, NULL );
    }
    RELEASE_INTLOCK( regs );

} /* end DEF_INST( transaction_begin_constrained ) */

#endif /* defined( FEATURE_050_CONSTR_TRANSACT_FACILITY ) */

/*-------------------------------------------------------------------*/
/*       process_tbegin  --  common TBEGIN/TBEGINC logic             */
/*-------------------------------------------------------------------*/
/*    The interrupt lock (INTLOCK) *MUST* be held upon entry!        */
/*-------------------------------------------------------------------*/
void ARCH_DEP( process_tbegin )( bool txf_contran, REGS* regs, S16 i2, TDB* tdb )
{
int         n, tdc;
TPAGEMAP   *pmap;

    /* Temporarily pause other CPUs while TBEGIN/TBEGINC is processed.
       NOTE: this *must* be done *BEFORE* checking nesting depth. */
    SYNCHRONIZE_CPUS( regs );

    /* Check for maximum nesting depth exceeded */
    if (regs->txf_tnd >= MAX_TXF_TND)
    {
        PTT_TXF( "*TXF beg", MAX_TXF_TND, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_PGMCHK, TAC_NESTING );
        UNREACHABLE_CODE( return );
    }

    CONTRAN_INSTR_CHECK( regs );    /* Unallowed in CONSTRAINED mode */

    regs->txf_tnd++;                /* increase the nesting level    */
    regs->psw.cc = TXF_CC_SUCCESS;  /* set cc=0 at transaction start */

    if (regs->txf_tnd == 1)         /* first/outermost transaction?  */
    {
        /*-----------------------------------------------------------*/
        /*              BEGIN OUTERMOST TRANSACTION                  */
        /*-----------------------------------------------------------*/

        UPDATE_SYSBLK_TRANSCPUS( +1 );  /* bump transacting CPUs ctr */

        /* Initialize the page map */

        pmap = regs->txf_pagesmap;
        regs->txf_pgcnt = 0;

        for (n=0; n < MAX_TXF_PAGES; n++, pmap++)
        {
            pmap->mainpageaddr = NULL;
            memset( pmap->cachemap, CM_CLEAN, sizeof( pmap->cachemap ));
        }

        /* Initialize other fields */

        regs->txf_contran    = txf_contran;/* set transaction type   */
        regs->txf_instctr    = 0;          /* instruction counter    */
        regs->txf_abortctr   = 0;          /* abort instr count      */
        regs->txf_tac        = 0;          /* clear the abort code   */
        regs->txf_conflict   = 0;          /* clear conflict address */
        regs->txf_piid       = 0;          /* program interrupt id   */
        regs->txf_lastacctyp = 0;          /* last access type       */
        regs->txf_lastarn    = 0;          /* last access arn        */
        regs->txf_why        = 0;          /* no abort cause (yet)   */

        memcpy( regs->txf_savedgr, regs->gr, sizeof( regs->txf_savedgr ));
        memset( regs->txf_pifctab, 0, sizeof( regs->txf_pifctab ));

        regs->txf_ctlflag      = (i2 >> 0) & (TXF_CTL_AR | TXF_CTL_FLOAT);
        regs->txf_pifc         = (i2 >> 0) & (TXF_CTL_PIFC);
        regs->txf_gprmask      = (i2 >> 8) & (0xFF);
        regs->txf_higharchange = (regs->txf_ctlflag & TXF_CTL_AR)    ? regs->txf_tnd : 0;
        regs->txf_highfloat    = (regs->txf_ctlflag & TXF_CTL_FLOAT) ? regs->txf_tnd : 0;
        regs->txf_tdb          = tdb;

        /* Set CONSTRAINED transaction instruction fetch constraint */
        if (txf_contran)
        {
            BYTE* aip2 = regs->aip + ZPAGEFRAME_PAGESIZE;

            regs->txf_tbeginc_aip = regs->aip;
            regs->txf_aie         = regs->ip + 256;
            regs->txf_aie_off2    = (regs->txf_aie < aip2) ? 0 :
                                    (regs->txf_aie - aip2);
        }
        else
        {
            regs->txf_tbeginc_aip = NULL;
            regs->txf_aie         = NULL;
            regs->txf_aie_off2    = 0;
        }

        /*----------------------------------*/
        /*  Save the Transaction Abort PSW  */
        /*----------------------------------*/
        {
            PSW origpsw;
            memcpy( &origpsw, &regs->psw, sizeof( PSW ));
            {
                n = txf_contran ? -6 : 0;
                regs->psw.IA = PSW_IA( regs, n );
                memcpy( &regs->txf_tapsw, &regs->psw, sizeof( PSW ));
                regs->txf_ip  = regs->ip;
                regs->txf_aip = regs->aip;
                regs->txf_aim = regs->aim;
                regs->txf_aiv = regs->aiv;
            }
            memcpy( &regs->psw, &origpsw, sizeof( PSW ));
        }

        /* Honor TDC (Transaction Diagnostic Control) setting */

        tdc = (int)
        (1
         && (regs->CR(2) & CR2_TDS)     /* TDC applies ONLY in problem state */
         && !PROBSTATE( &regs->psw )    /* but CPU is *NOT* in problem state */
        )
        ? TDC_NORMAL                    /* Treat as if TDC contained zero    */
        : (regs->CR(2) & CR2_TDC);      /* Otherwise, honor the TDC value    */

        switch (tdc)
        {
            default:
            case TDC_NORMAL:                /* NEVER randomly abort */
            case TDC_RESERVED:

                break;

            case TDC_MAYBE_RANDOM:          /* MAYBE randomly abort */

                if (rand() >= rand())
                    break;

                /* Fall through to choose when to randomly abort */

            case TDC_ALWAYS_RANDOM:         /* ALWAYS randomly abort */
            {
                /* Choose a random instruction to abort at */
                regs->txf_abortctr = (U16) (rand() % MAX_TXF_CONTRAN_INSTR);

                if (!txf_contran)
                    regs->txf_random_tac = (U16) 0;
                else
                {
                    /* Randomly chosen abort codes */
                    static const BYTE racode[10] =
                    {
                        TAC_FETCH_OVF,
                        TAC_STORE_OVF,
                        TAC_FETCH_CNF,
                        TAC_STORE_CNF,
                        TAC_INSTR,
                        TAC_NESTING,
                        TAC_FETCH_OTH,
                        TAC_STORE_OTH,
                        TAC_CACHE_OTH,
                        TAC_MISC,
                    };

                    /* Choose a random abort code */
                    regs->txf_random_tac = (U16) racode[ rand() % _countof( racode ) ];
                }
                break;
            }
        }
    }
    else /* (nested transaction) */
    {
        /*------------------------------------------------------*/
        /*              BEGIN NESTED TRANSACTION                */
        /*------------------------------------------------------*/
        /* Update highest nesting level that allowed AR changes */
        /*------------------------------------------------------*/

        if (!(i2 & TXF_CTL_AR))
            regs->txf_ctlflag &= ~TXF_CTL_AR;
        else if (regs->txf_higharchange == (regs->txf_tnd - 1))
            regs->txf_higharchange = regs->txf_tnd;

        /*--------------------------------------------------------------*/
        /* Update highest nesting level that allowed float instructions */
        /*--------------------------------------------------------------*/

        if (!(i2 & TXF_CTL_FLOAT))
            regs->txf_ctlflag &= ~TXF_CTL_FLOAT;
        else if (regs->txf_highfloat == (regs->txf_tnd - 1))
            regs->txf_highfloat = regs->txf_tnd;

        /*-----------------------------*/
        /* Update program filter level */
        /*-----------------------------*/

        regs->txf_pifctab[ regs->txf_tnd - 2 ] = regs->txf_pifc;
        regs->txf_pifc = MAX( regs->txf_pifc, (i2 & TXF_CTL_PIFC) );

    }

    PTT_TXF( "TXF beg", 0, regs->txf_contran, regs->txf_tnd );

    if (TXF_TRACE( SUCCESS, regs->txf_contran ))
    {
        // "TXF: Successful %s TBEGIN%s; TND now %d"
        WRMSG( HHC17702, "D", regs->txf_tnd > 1 ? "nested" : "outermost",
            regs->txf_contran ? "C" : "", regs->txf_tnd );
    }

} /* end ARCH_DEP( process_tbegin ) */

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */


#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) \
&& !defined( COMPILE_THIS_ONLY_ONCE )
   #define   COMPILE_THIS_ONLY_ONCE
/*-------------------------------------------------------------------*/
/*         Transaction Abort Code to Condition Code table            */
/*-------------------------------------------------------------------*/
/*     (see Fig. 514 on page 5-102 of SA22-7832-12 z/Arch POPs)      */
/*-------------------------------------------------------------------*/
static const int tac2cc[20] =
{
    TXF_CC_PERSISTENT,      //   0  (undefined)
    TXF_CC_PERSISTENT,      //   1  (undefined)
    TXF_CC_TRANSIENT,       //   2  TAC_EXT
    TXF_CC_PERSISTENT,      //   3  (undefined)
    TXF_CC_PERSISTENT,      //   4  TAC_UPGM
    TXF_CC_TRANSIENT,       //   5  TAC_MCK
    TXF_CC_TRANSIENT,       //   6  TAC_IO
    TXF_CC_TRANSIENT,       //   7  TAC_FETCH_OVF
    TXF_CC_TRANSIENT,       //   8  TAC_STORE_OVF
    TXF_CC_TRANSIENT,       //   9  TAC_FETCH_CNF
    TXF_CC_TRANSIENT,       //  10  TAC_STORE_CNF
    TXF_CC_PERSISTENT,      //  11  TAC_INSTR
    TXF_CC_PERSISTENT,      //  12  TAC_FPGM
    TXF_CC_PERSISTENT,      //  13  TAC_NESTING
    TXF_CC_TRANSIENT,       //  14  TAC_FETCH_OTH
    TXF_CC_TRANSIENT,       //  15  TAC_STORE_OTH
    TXF_CC_TRANSIENT,       //  16  TAC_CACHE_OTH
    TXF_CC_PERSISTENT,      //  17  (undefined)
    TXF_CC_PERSISTENT,      //  18  (undefined)
    TXF_CC_TRANSIENT        //  19  TAC_GUARDED
};
#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */


/*-------------------------------------------------------------------*/
/*                      abort_transaction                            */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Common routine to abort a transaction.  This routine restores    */
/*  the requested registers from transaction start, cleans up the    */
/*  transaction flags, updates the current PSW to the abort PSW      */
/*  and then does one of the following based on the retry flag:      */
/*                                                                   */
/*    retry = 0  return to the caller.  This is used when abort      */
/*               is called from an interrupt handler.                */
/*                                                                   */
/*    retry = 1  set the condition code and do longjmp to progjmp.   */
/*                                                                   */
/*    retry = 2  if in CONSTRAINED mode, generate a program check.   */
/*               Otherwise longjmp to progjmp.                       */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT void ARCH_DEP( abort_transaction )( REGS* regs, int retry, int txf_tac )
{
#if !defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

    /* S370 and S390 don't support Transactional-Execution Facility */

    UNREFERENCED( regs );
    UNREFERENCED( retry );
    UNREFERENCED( txf_tac );

    CRASH();   /* Should NEVER be called for S/370 or S/390! */

#else /* only Z900 supports Transactional-Execution Facility */

BYTE       txf_gprmask;
int        txf_tnd, i;
bool       txf_contran, had_INTLOCK;
U32        txf_piid;
U64        txf_conflict;
U64        txf_bea;
TDB*       pi_tdb   = NULL; /* Program Interrupt TDB @ fixed 0x1800  */
TDB*       tb_tdb   = NULL; /* TBEGIN-specified TDB @ operand-1 addr */
VADR       txf_atia = PSW_IA( regs, -REAL_ILC( regs ) );

    // LOGIC ERROR if CPU not in transactional-execution mode!
    if (!regs->txf_tnd)
        CRASH();

    /* Obtain the interrupt lock if we don't already have it */
    if (sysblk.intowner == regs->cpuad)
        had_INTLOCK = true;
    else
    {
        OBTAIN_INTLOCK( regs );
        had_INTLOCK = false;
    }

    PTT_TXF( "TXF ABORT", 0, regs->txf_contran, regs->txf_tnd );

    /*---------------------------------------------*/
    /*  Decrement count of transacting CPUs        */
    /*---------------------------------------------*/
    UPDATE_SYSBLK_TRANSCPUS( -1 );

    /*---------------------------------------------*/
    /*  Clean up the transaction flags             */
    /*---------------------------------------------*/

    txf_tnd      = regs->txf_tnd;        /* save orig value */
    txf_contran  = regs->txf_contran;    /* save orig value */
    txf_gprmask  = regs->txf_gprmask;    /* save orig value */
    txf_conflict = regs->txf_conflict;   /* save orig value */
    txf_piid     = regs->txf_piid;       /* save orig value */
    txf_bea      = regs->bear;           /* save orig value */

    if (TXF_TRACE( FAILURE, txf_contran ))
    {
        char why[ 256 ] = {0};

        if (regs->txf_why)
            txf_why_str( why, sizeof( why ), regs->txf_why );
        else
            STRLCPY( why, " ?" );

        // "TXF: Failed %s %s Transaction for TND %d: %s (%s), why =%s"
        WRMSG( HHC17703, "D", txf_tnd > 1 ? "Nested" : "Outermost",
            txf_contran ? "Cons" : "Uncons", txf_tnd,
            tac2short( txf_tac ), tac2long( txf_tac ), why );

        if (TXF_TRACE_MAP( txf_contran ))
        {
            TPAGEMAP*  pmap;
            BYTE*      mainaddr;
            BYTE*      altaddr;
            int        i, j;

            pmap = regs->txf_pagesmap;

            for (i=0; i < regs->txf_pgcnt; i++, pmap++)
            {
                if (TXF_TRACE_PAGES( txf_contran ))
                {
                    // "TXF: virt 0x%16.16"PRIX64", abs 0x%16.16"PRIX64", alt 0x%16.16"PRIX64
                    WRMSG( HHC17704, "D", (U64) pmap->virtpageaddr,
                        (U64)(pmap->mainpageaddr - regs->mainstor),
                        (U64) pmap->altpageaddr );
                }

                for (j=0; j < ZCACHE_LINE_PAGE; j++)
                {
                    if (pmap->cachemap[j] == CM_CLEAN)
                        continue;

                    mainaddr = pmap->mainpageaddr + (j << ZCACHE_LINE_SHIFT);
                    altaddr  = pmap->altpageaddr  + (j << ZCACHE_LINE_SHIFT);

                    if (TXF_TRACE_LINES( txf_contran ))
                    {
                        // "TXF: There now:  +"
                        dump_cache( DUMP_PFX( HHC17705 ), j, mainaddr );

                        if (pmap->cachemap[j] == CM_FETCHED)
                            // "TXF: We fetched: +"
                            dump_cache( DUMP_PFX( HHC17706 ), j, altaddr );
                        else
                            // "TXF: We stored:  +"
                            dump_cache( DUMP_PFX( HHC17707 ), j, altaddr );
                    }
                }
            }
        }
    }

    OBTAIN_TXFLOCK( regs );
    {
        regs->txf_tnd = 0;
    }
    RELEASE_TXFLOCK( regs );

    regs->txf_NTSTG     = false;
    regs->txf_contran   = false;
    regs->txf_tac       = 0;
    regs->txf_abortctr  = 0;
    regs->txf_instctr   = 0;
    regs->txf_pgcnt     = 0;
    regs->txf_conflict  = 0;
    regs->txf_piid      = 0;

    /*----------------------------------------------------*/
    /*  Set the current PSW to the Transaction Abort PSW  */
    /*----------------------------------------------------*/

    memcpy( &regs->psw, &regs->txf_tapsw, sizeof( PSW ));
    regs->ip  = regs->txf_ip;
    regs->aip = regs->txf_aip;
    regs->aim = regs->txf_aim;
    regs->aiv = regs->txf_aiv;
    INVALIDATE_AIA( regs );

    /*---------------------------------------------*/
    /*     Set the condition code in the PSW       */
    /*---------------------------------------------*/
    /*  For unfiltered program interrupts (aborts) */
    /*  the condition code is determined by the    */
    /* 'program_interrupt' function in cpu.c.  For */
    /* all other program interrupts (aborts), the  */
    /* condition code is documented in Fig. 5-14   */
    /* on page 5-102 of the z/Arch POPs.           */
    /*---------------------------------------------*/
    if (txf_tac != TAC_UPGM)
    {
             if (txf_tac && txf_tac < (int) _countof( tac2cc ))
                                        regs->psw.cc = tac2cc[ txf_tac ];
        else if (txf_tac == TAC_MISC)   regs->psw.cc = TXF_CC_TRANSIENT;
        else if (txf_tac >= TAC_TABORT) regs->psw.cc = TXF_CC_INDETERMINATE;
    }

    /*---------------------------------------------*/
    /* Release the interrupt lock if we obtained   */
    /* it, or if we won't be directly returning    */
    /* from this function i.e. if we'll be jumping */
    /* to progjmp via program_interrupt or via     */
    /* SIE_NO_INTERCEPT, then we need to release   */
    /* the interrupt lock beforehand. Otherwise if */
    /* it was already held upon entry then we need */
    /* to return to the caller with it still held. */
    /*---------------------------------------------*/

    if (!had_INTLOCK || retry != ABORT_RETRY_RETURN)
        RELEASE_INTLOCK( regs );

    /*---------------------------------------------*/
    /* Populate the TDBs. For program-interrupts,  */
    /* the TDB is at fixed storage location 0x1800 */
    /* in low core.  Additionally, the address of  */
    /* a second TDB may be specified by the TBEGIN */
    /* instruction itself. (The operand-1 address  */
    /* for TBEGINC is ignored.)                    */
    /*---------------------------------------------*/

    /* Program Interrupt TDB? */
    if (0
        || txf_tac == TAC_UPGM  /* Unfiltered PGM? */
        || txf_tac == TAC_FPGM  /* Filtered   PGM? */
    )
    {
        RADR tdb = 0x1800;
        tdb = APPLY_PREFIXING( tdb, regs->PX );
        pi_tdb = (TDB*)(regs->mainstor + tdb);
    }

    /* TBEGIN-specified TDB? */
    if (!txf_contran && regs->txf_tdb)
        tb_tdb = regs->txf_tdb;

    /* Populate the TDBs */

    if (pi_tdb)
    {
        /* PROGAMMING NOTE: see Figure 5-13 on page 5-94:
           certain TDB fields are stored only for TBEGIN-
           specified TDB. Otherwise the field is reserved.
        */
        pi_tdb->tdb_format = 1;
//      pi_tdb->tdb_eaid   = regs->excarid;
        pi_tdb->tdb_flags  = (txf_contran ? TDB_CTI : 0x00);

        STORE_HW( pi_tdb->tdb_tnd,      (U16) txf_tnd      );
        STORE_DW( pi_tdb->tdb_tac,      (U64) txf_tac      );
//      STORE_DW( pi_tdb->tdb_teid,     (U64) regs->TEA    );
//      STORE_FW( pi_tdb->tdb_piid,     (U32) txf_piid     );
        STORE_DW( pi_tdb->tdb_atia,     (U64) txf_atia     );
//      STORE_DW( pi_tdb->tdb_conflict, (U64) txf_conflict );
//      STORE_DW( pi_tdb->tdb_bea,      (U64) txf_bea      );

        for (i=0; i < 16; i++)
            STORE_DW( pi_tdb->tdb_gpr[i], regs->GR_G( i ));

        if (TXF_TRACE_TDB( txf_contran ))
            dump_tdb( pi_tdb, regs->txf_tdb_addr );
    }

    if (tb_tdb)
    {
        tb_tdb->tdb_format = 1;
        tb_tdb->tdb_eaid   = regs->excarid;
        pi_tdb->tdb_flags  = (txf_contran ? TDB_CTI : 0x00);

        if (0
            || txf_tac == TAC_FETCH_CNF
            || txf_tac == TAC_STORE_CNF
        )
            tb_tdb->tdb_flags |= (txf_conflict ? TDB_CTV : 0x00);

        if (0
            || txf_piid == PGM_DATA_EXCEPTION
            || txf_piid == PGM_VECTOR_PROCESSING_EXCEPTION
        )
            tb_tdb->tdb_dxc = regs->txf_dxcvxc;

        STORE_HW( tb_tdb->tdb_tnd,      (U16) txf_tnd      );
        STORE_DW( tb_tdb->tdb_tac,      (U64) txf_tac      );
        STORE_DW( tb_tdb->tdb_teid,     (U64) regs->TEA    );
        STORE_FW( tb_tdb->tdb_piid,     (U32) txf_piid     );
        STORE_DW( tb_tdb->tdb_atia,     (U64) txf_atia     );
        STORE_DW( tb_tdb->tdb_conflict, (U64) txf_conflict );
        STORE_DW( tb_tdb->tdb_bea,      (U64) txf_bea      );

        for (i=0; i < 16; i++)
            STORE_DW( tb_tdb->tdb_gpr[i], regs->GR_G( i ));

        if (TXF_TRACE_TDB( txf_contran ))
            dump_tdb( tb_tdb, regs->txf_tdb_addr );
    }

    /*---------------------------------------------*/
    /*      Restore the requested registers        */
    /*---------------------------------------------*/
    for (i=0; i < 16; i += 2, txf_gprmask <<= 1)
    {
        if (txf_gprmask & 0x80)
        {
            regs->gr[ i+0 ] = regs->txf_savedgr[ i+0 ];
            regs->gr[ i+1 ] = regs->txf_savedgr[ i+1 ];
        }
    }

    /*---------------------------------------------*/
    /*   Return to caller or jump as requested     */
    /*---------------------------------------------*/
    if (retry == ABORT_RETRY_RETURN)
    {
        /* Caller requested that we return back to
           them so they can decide what to do next.
           This 'retry' code is typically used by
           the various interrupt handlers (external,
           machine check, restart, program and I/O).
        */
        PTT_TXF( "TXF ABRTRET", regs, txf_tac, retry );
        return; // (caller decides what to do next)
    }

    /* Constrained transaction failures are routed
       to the program interrupt handler to be tried
       again since the architecture REQUIRES that
       all constrained transactions MUST eventually
       succeed.
    */
    if (txf_contran && retry == ABORT_RETRY_PGMCHK)
    {
        PTT_TXF( "*TXF ABRTPI", regs, txf_tac, retry );
        regs->txf_aborted = true;
        ARCH_DEP( program_interrupt )( regs, PGM_TRANSACTION_CONSTRAINT_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    /* Transaction failures have their PSW set to the
       Transaction Abort PSW, which for unconstrained
       transactions points to the instruction immediately
       following the TBEGIN instruction (so that the
       condition code can then be used to decide whether
       to bother retrying the transaction or not), and
       for constrained transactions points to the TBEGINC
       instruction itself (so that the transaction can
       then be unconditionally retried). Either way, we
       simply jump directly back to the 'run_cpu' loop
       to redispatch this CPU, thereby causing it to
       continue executing instructions at wherever the
       the Transaction Abort PSW said it should.
    */
    PTT_TXF( "*TXF ABRTJMP", regs, txf_tac, retry );
    longjmp( regs->progjmp, SIE_NO_INTERCEPT );
    UNREACHABLE_CODE( return );

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

} /* end ARCH_DEP( abort_transaction ) */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "transact.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "transact.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )

/*-------------------------------------------------------------------*/
/*       Allocate Transactional-Execution Facility Pages Map         */
/*-------------------------------------------------------------------*/
void alloc_txfmap( REGS* regs )
{
int        i;
size_t     msize;
BYTE*      altpage;
TPAGEMAP*  pmap = regs->txf_pagesmap;

    PTT_TXF( "TXF alloc", 0, regs, regs->cpuad );

    /* LOGIC ERROR if map still exists (memory leak; old map not freed)
       or if a transaction is still being executed on this CPU */
    if (pmap->altpageaddr || regs->txf_tnd)
        CRASH();

    msize = ZPAGEFRAME_PAGESIZE * MAX_TXF_PAGES * 2;
    altpage = (BYTE*) malloc_aligned( msize, ZPAGEFRAME_PAGESIZE );

    /* Initialize all page map entries */
    for (i=0; i < MAX_TXF_PAGES; i++, pmap++, altpage += (ZPAGEFRAME_PAGESIZE * 2))
    {
        pmap->virtpageaddr = 0;
        pmap->mainpageaddr = NULL;
        pmap->altpageaddr  = altpage;
        memset( pmap->cachemap, CM_CLEAN, sizeof( pmap->cachemap ));
    }

    regs->txf_tnd      = 0;
    regs->txf_abortctr = 0;
    regs->txf_contran  = false;
    regs->txf_instctr  = 0;
    regs->txf_pgcnt    = 0;
}

/*-------------------------------------------------------------------*/
/*       Free Transactional-Execution Facility Pages Map             */
/*-------------------------------------------------------------------*/
void free_txfmap( REGS* regs )
{
int        i;
TPAGEMAP*  pmap = regs->txf_pagesmap;

    PTT_TXF( "TXF free", 0, regs, regs->cpuad );

    /* LOGIC ERROR if CPU still executing a transaction */
    if (regs->txf_tnd)
        CRASH();

    /* Free TXF pages */
    free_aligned( pmap->altpageaddr );

    /* Clear/reset/re-initialize all pages map entries */
    for (i=0; i < MAX_TXF_PAGES; i++, pmap++)
    {
        pmap->virtpageaddr = 0;
        pmap->mainpageaddr = NULL;
        pmap->altpageaddr  = NULL;
        memset( pmap->cachemap, CM_CLEAN, sizeof( pmap->cachemap ));
    }
}

/*-------------------------------------------------------------------*/
/*                Check for TXF Storage Conflict                     */
/*-------------------------------------------------------------------*/
static inline bool txf_conflict_chk( REGS* regs, int acctype, U64 addrpage,
                                     int cacheidx, int cacheidxe )
{
int        i, k;
int        txf_tac = 0;
TPAGEMAP*  pmap;

    /* Skip if our CPU is not in transaction mode or if our transaction
       has already been aborted or if our CPU hasn't been started yet.
    */
    if (0
        || !regs
        || !regs->txf_tnd
        ||  regs->txf_tac
        ||  regs->cpustate != CPUSTATE_STARTED
    )
        return false;   // (no conflict detected)

    /* Scan our CPU's page map... */
    for (pmap = regs->txf_pagesmap, i=0; i < regs->txf_pgcnt; i++, pmap++)
    {
        /* (skip if not same page as the one we're to check) */
        if ((U64)pmap->mainpageaddr != addrpage)
            continue;

        /* Check for storage conflict with our CPU */
        for (k = cacheidx; k <= cacheidxe; k++)
        {
            /* If our transaction hasn't touched or accessed
               (stored into or fetched from) this cache line yet,
               then no conflict is possible. Skip this entry.
            */
            if (pmap->cachemap[k] == CM_CLEAN)
                continue;

            /* If we only fetched this cache line and they're only
               fetching from it too, then it's not a real conflict.
            */
            if (pmap->cachemap[k] == CM_FETCHED)
            {
                if (acctype == ACCTYPE_READ)
                    continue;

                /* Otherwise since they're storing into this cache
                   line, we must abort our CPU's transaction with
                   a fetch conflict.
                */
                txf_tac = TAC_FETCH_CNF;
            }
            else // (pmap->cachemap[k] == CM_STORED)
            {
                /* We stored into this cache line. Since they're
                   either fetching from or storing into the same
                   cache line, we must abort our CPU's transaction
                   with a store conflict.
                */
                txf_tac = TAC_STORE_CNF;
            }

            /* Abort this CPU's transaction */
            OBTAIN_TXFLOCK( regs );
            {
                regs->txf_tac = txf_tac;
                regs->txf_why |= TXF_WHY_CONFLICT;

                /* Save the logical address where the conflict
                   occurred if we were able to determine that.
                   Setting it to a non-zero value causes the
                   TDB_CTV (Conflict-Token Validity) flag to
                   be set in the Transaction Diagnostic Block
                   providing information to the user regarding
                   their failure.
                */
                if (pmap->virtpageaddr)
                    regs->txf_conflict = pmap->virtpageaddr + (cacheidx << ZCACHE_LINE_SHIFT);
                else
                    regs->txf_conflict = 0;
            }
            RELEASE_TXFLOCK( regs );

            PTT_TXF( "TXF chk!", addrpage, regs->txf_conflict, true );
            return true;    // (CONFLICT DETECTED!)

        } // end for cache line loop...

    } // end for page map loop...

    return false;   // (no conflict detected)
}

//---------------------------------------------------------------------
// PROGRAMMING NOTE: because the 'txf_maddr_l' function is an integral
// part of address translation its performance is absolutely critical.
// Disabling optimization even for debug builds reduces performance to
// the point of rendering Hercules pretty much unusable. Thus we keep
// optimization enabled for the 'txf_maddr_l' function even for DEBUG
// builds. (Disabling optimization for all of the other functions is
// okay since they're not called that often.)
//---------------------------------------------------------------------
#if defined( NO_TXF_OPTIMIZE )
  #if !defined( NO_MADDR_L_OPTIMIZE ) // CAREFUL! See PROGRAMMING NOTE!
    #if defined( _MSVC_ )
      #pragma optimize( "", on )      // KEEP OPTIMIZATION ENABLED!
    #else // e.g. Linux/gcc
      #pragma GCC optimize ("O3")     // KEEP OPTIMIZATION ENABLED!
    #endif
  #endif
#endif

/*-------------------------------------------------------------------*/
/*     Transactional Address Translation and Conflict Check          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  This function is called after every 'maddr_l' dynamic address    */
/*  translation and by channel.c to, either POSSIBLY "translate"     */
/*  the passed address to an alternate in a different page (only     */
/*  for 'maddr_l' calls) and/or to check if the passed address       */
/*  conflicts with the address of any currently transacting CPU      */
/*  (if either a 'maddr_l' call or a channel.c call).                */
/*                                                                   */
/*  If called by the 'maddr_l' address translation function, both    */
/*  vaddr and regs must be valid.  When called by channel.c however  */
/*  (conflict check only), they should both be passed as zero.       */
/*                                                                   */
/*  If the CPU is in transactional execution mode (CONSTRAINED or    */
/*  unconstrained), the maddr real storage address is saved in the   */
/*  transaction data area, and then it is mapped to an alternate     */
/*  address.  The contents of the real page are also saved.  The     */
/*  cache line within the page is marked as having been fetched or   */
/*  stored.   The first time that a cache line is accessed, it is    */
/*  captured from the real page.  When a cache line or page is       */
/*  captured, two copies are made.  One copy is presented to the     */
/*  caller, and one is a save copy which will be used to see if the  */
/*  cache line has changed.  In order to make sure that the capture  */
/*  is clean, the two copies must match.  If they do not match,      */
/*  the copy is retried up to MAX_CAPTURE_TRIES times.  If a copy    */
/*  cannot be made in that many tries, the transaction is aborted    */
/*  with a fetch conflict.  Note: it is OK to use real addresses     */
/*  here because the transaction will be aborted if the real page    */
/*  is invalidated.                                                  */
/*                                                                   */
/*  If the CPU is not executing any transaction, then only 'maddr'   */
/*  is checked to see if it conflicts with any transactions that     */
/*  might be executing on any other CPU (i.e. no translation is      */
/*  performed; the 'maddr' value that was passed is returned).       */
/*                                                                   */
/*  Otherwise if the CPU is executing a transaction, 'maddr' is      */
/*  mapped (translated) to an alternate page from the page map as    */
/*  explained further above and it is that alternate address that    */
/*  is then returned to the caller.                                  */
/*                                                                   */
/*  In both cases (maddr_l call or channel.c call) the address is    */
/*  checked for conflict against any currently transacting CPU and   */
/*  if so, the transacting CPU's transaction is aborted. Otherwise   */
/*  the translated/untranslated address is returned to the caller.   */
/*                                                                   */
/*  Input:                                                           */
/*       vaddr    Logical address as passed to maddr_l or NULL       */
/*       len      Length of data access for cache line purposes      */
/*       arn      Access register number as passed to maddr_l        */
/*       regs     Pointer to the CPU register context or NULL        */
/*       acctype  Type of access: READ, WRITE, INSTFETCH, etc.       */
/*       maddr    Guest absolute storage MAINADDR address output     */
/*                from 'maddr_l' address translation call or the     */
/*                absolute storage address being fetched or stored   */
/*                by channel.c for a check-conflict-only call.       */
/*                                                                   */
/* Returns:                                                          */
/*      Either the same value as passed in 'maddr' or POSSIBLY a     */
/*      corresponding alternate address from the CPU's transaction   */
/*      page map (TPAGEMAP) if the CPU is executing a transaction.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT BYTE* txf_maddr_l( U64 vaddr, size_t len, int arn, REGS* regs, int acctype, BYTE* maddr )
{
    BYTE *pageaddr, *pageaddrc;
    BYTE *savepage, *savepagec;
    BYTE *altpage,  *altpagec;

    REGS*  rchk;                /* Ptr to other CPU's hostregs       */

    U64  addrwork;              /* maddr converted to U64            */
    U64  addrpage;              /* Corresponding page address        */

    int  pageoffs;              /* addrwork offset into addrpage     */
    int  cacheidx;              /* Corresponding cache line          */
    int  cacheidxe;             /* Corresponding ending cache line   */
    int  i;                     /* Work variable                     */

    BYTE cmtype;                /* Cache Map access type             */

    TPAGEMAP*  pmap;            /* Pointer to Transaction Page Map   */

    /* Save last translation access type and arn */
    if (regs && regs->txf_tnd)
    {
        regs->txf_lastacctyp = acctype;
        regs->txf_lastarn    = arn;
    }

    addrwork = (U64) maddr;                     /* convert to U64    */
    addrpage = addrwork & ZPAGEFRAME_PAGEMASK;  /* address of page   */
    pageoffs = addrwork & ZPAGEFRAME_BYTEMASK;  /* offset into page  */

    /* Calculate starting cache line */
    cacheidx = pageoffs >> ZCACHE_LINE_SHIFT;   /* data cache line   */

    /* Calculate ending cache line */
    {
        /* Length from data to end of page */
        size_t  endlen  = ZPAGEFRAME_PAGESIZE - (vaddr & ZPAGEFRAME_BYTEMASK);

        /* Lesser of that length and actual data length */
        size_t  minlen  = MIN( len, endlen );

        /* Page offset to end of data */
        int  endingoff  = pageoffs + minlen;

        /* Data ends on this cache line */
        cacheidxe = endingoff >> ZCACHE_LINE_SHIFT;
    }

    /*------------------------------------------------------------------*/
    /*   If any CPUs are in transaction mode, we need to see if this    */
    /*   CPU is storing or fetching into a cache line that has been     */
    /*   touched by a CPU in transaction mode.  If that is the case,    */
    /*   we will flag the other CPU for abort.  The actual abort will   */
    /*   happen the next time the transacting CPU references storage    */
    /*   or gets to transaction end.                                    */
    /*                                                                  */
    /*   The process of moving CPUs in and out of transaction mode is   */
    /*   serialized, so that state will not change during the check.    */
    /*                                                                  */
    /*   The scan of the cache lines for the CPU is serialized by       */
    /*   obtaining the transaction lock for that CPU.  The lock is      */
    /*   also obtained to update the pagemap and/or cachemap.           */
    /*------------------------------------------------------------------*/
    for (i=0; i < sysblk.hicpu; i++)
    {
        /* Point to this CPU's hostregs */
        rchk = sysblk.regs[i];

        /* Check both hostregs *AND* guestregs for storage conflict
           with this CPU (as long as it's not ours!)

           PROGRAMMING NOTE: it's safe to stop checking if our
           store or fetch access to this cache line conflicts
           with any OTHER transacting CPU's cache line access
           (i.e. it's safe to stop our loop here) as we can be
           ABSOLUTELY 100% CERTAIN that there AREN'T any since
           no two transacting CPU's accesses can ever conflict
           with any other's (otherwise one of them would have
           had their transaction aborted due to the conflict).
        */
        if (0
            || (1
                && rchk != regs
                && txf_conflict_chk( rchk, acctype, addrpage, cacheidx, cacheidxe )
               )
            || (1
                && rchk->guestregs != regs
                && txf_conflict_chk( rchk->guestregs, acctype, addrpage, cacheidx, cacheidxe )
               )
        )
            break;  // CONFLICT DETECTED!

    } // end for all CPUs loop...

    /* Return now if channel conflict-check-only call
       or if our CPU is not executing any transaction */
    if (!regs || !regs->txf_tnd)
        return maddr;

    /* Otherwise check if our own CPU's transaction was aborted */
    if (regs->txf_tac)
    {
        PTT_TXF( "*TXF maddr_l", regs->txf_tac, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_CC, regs->txf_tac );
        UNREACHABLE_CODE( return maddr );
    }

    /*-----------------------------------------------------------*/
    /*                  TXF Translation Call                     */
    /*-----------------------------------------------------------*/
    /*  We will return an alternate real address to the caller,  */
    /*  which will be visible only to this transacting CPU.      */
    /*                                                           */
    /*  When/if the transaction is commited, the alternate page  */
    /*  will be copied to the real page, as long as there were   */
    /*  no changes to the cache lines we touched.  We mark all   */
    /*  cache lines that we touch (fetch from or store into).    */
    /*                                                           */
    /*  All cache lines we touched must not have changed in      */
    /*  the original page, or our transaction aborts.  We can    */
    /*  safely use real addresses here because if a page fault   */
    /*  occurs, the transaction aborts and we start over.        */
    /*                                                           */
    /*  In most cases, the length passed to MADDRL is correct,   */
    /*  but in some cases the source length is one byte instead  */
    /*  of the true length.  In those cases, we must check to    */
    /*  see if more than one cache line is crossed.              */
    /*-----------------------------------------------------------*/

    /*-----------------------------------------------------------*/
    /*  See if we have already captured this page and if not,    */
    /*  capture it and save a copy.  The copy is used at commit  */
    /*  time to determine if any unexpected changes were made.   */
    /*-----------------------------------------------------------*/

    altpage = NULL;
    pmap = regs->txf_pagesmap;

    /* Check if page already mapped. If so, use it */
    for (i=0; i < regs->txf_pgcnt; i++, pmap++)
    {
        if (addrpage == (U64) pmap->mainpageaddr)
        {
            altpage = pmap->altpageaddr;
            break;
        }
    }

    /* If not mapped yet, capture real page and save a copy */
    if (!altpage)
    {
        /* Abort transaction if too many pages were touched */
        if (regs->txf_pgcnt >= MAX_TXF_PAGES)
        {
            int txf_tac = (acctype == ACCTYPE_READ) ?
                TAC_FETCH_OVF : TAC_STORE_OVF;
            regs->txf_why |= TXF_WHY_MAX_PAGES;

            PTT_TXF( "*TXF maddr_l", txf_tac, regs->txf_contran, regs->txf_tnd );
            ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_PGMCHK, txf_tac );
            UNREACHABLE_CODE( return maddr );
        }

        pageaddr = (BYTE*) addrpage;
        pmap     = &regs->txf_pagesmap[ regs->txf_pgcnt ];
        altpage  = pmap->altpageaddr;
        savepage = altpage + ZPAGEFRAME_PAGESIZE;

        /* Try to capture a clean copy of this page */
        for (i=0; i < MAX_CAPTURE_TRIES; i++)
        {
            memcpy( altpage,  pageaddr, ZPAGEFRAME_PAGESIZE );
            memcpy( savepage, pageaddr, ZPAGEFRAME_PAGESIZE );

            if (memcmp( altpage, savepage, ZPAGEFRAME_PAGESIZE ) == 0)
                break;
        }

        /* Abort if unable to obtain clean capture of this page */
        if (i >= MAX_CAPTURE_TRIES)
        {
            // "TXF: Unable to obtain clean capture of page"
            WRMSG( HHC17711, "E" );
            PTT_TXF( "*TXF maddr_l", TAC_FETCH_CNF, regs->txf_contran, regs->txf_tnd );
            ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_CC, TAC_FETCH_CNF );
            UNREACHABLE_CODE( return maddr );
        }

        /* Finish mapping this page */
        pmap->mainpageaddr = (BYTE*) addrpage;
        pmap->virtpageaddr = vaddr & ZPAGEFRAME_PAGEMASK;
        regs->txf_pgcnt++;
    }

    /* Return alternate address from our page map instead */
    ASSERT( altpage ); // (sanity check)
    maddr  = altpage + pageoffs;
    cmtype = (acctype == ACCTYPE_READ) ? CM_FETCHED : CM_STORED;

    PTT_TXF( "TXF maddr_l:", addrpage, altpage, regs->txf_tnd );

    /* Update page map's cache line indicators for this reference */
    for (; cacheidx <= cacheidxe; cacheidx++)
    {
        switch (pmap->cachemap[ cacheidx ])
        {
        case CM_CLEAN:

            /* Cache line WAS marked as being clean so we should now
               refresh it in order to pick up any potential changes.
            */
            pageaddrc = pmap->mainpageaddr + (cacheidx << ZCACHE_LINE_SHIFT);
            altpagec  = pmap->altpageaddr  + (cacheidx << ZCACHE_LINE_SHIFT);
            savepagec = altpagec + ZPAGEFRAME_PAGESIZE;

            for (i=0; i < MAX_CAPTURE_TRIES; i++)
            {
                memcpy( altpagec,  pageaddrc, ZCACHE_LINE_SIZE );
                memcpy( savepagec, pageaddrc, ZCACHE_LINE_SIZE );

                if (memcmp( altpagec, savepagec, ZCACHE_LINE_SIZE ) == 0)
                    break;
            }

            /* Abort if unable to cleanly refresh this cache line */
            if (i >= MAX_CAPTURE_TRIES)
            {
                // "TXF: Unable to cleanly refresh cache line"
                WRMSG( HHC17712, "E" );
                PTT_TXF( "*TXF maddr_l", TAC_FETCH_CNF, regs->txf_contran, regs->txf_tnd );
                ARCH_DEP( abort_transaction )( regs, ABORT_RETRY_CC, TAC_FETCH_CNF );
                UNREACHABLE_CODE( return maddr );
            }

            pmap->cachemap[ cacheidx ] = cmtype;
            break;

        case CM_FETCHED:

            pmap->cachemap[ cacheidx ] = cmtype;
            break;

        case CM_STORED:

            /* Once cache line is marked CM_STORED it STAYS that way */
            break;

        } /* switch (pmap->cachemap[cacheidx]) */

    } /* for (; cacheidx <= cacheidxe; cacheidx++) */

    /* Return alternate address from our page map instead */
    PTT_TXF( "TXF maddr_l", maddr, len, regs->txf_tnd );
    return maddr;

} /* end function txf_maddr_l */

#if defined( NO_TXF_OPTIMIZE )
  #if defined( _MSVC_ )
    #pragma optimize( "", off )     // (for reliable breakpoints)
  #else // e.g. Linux/gcc
    #pragma GCC optimize ("O0")     // (for reliable breakpoints)
  #endif
#endif

/*-------------------------------------------------------------------*/
/*            DEBUG:  Transaction Abort Code names                   */
/*-------------------------------------------------------------------*/
static const char* tac_names[] =
{
    /*   0 */   "0",                "0",
    /*   1 */   "1",                "1",
    /*   2 */   "TAC_EXT",          "External interruption",
    /*   3 */   "3",                "3",
    /*   4 */   "TAC_UPGM",         "PGM Interruption (Unfiltered)",
    /*   5 */   "TAC_MCK",          "Machine-check Interruption",
    /*   6 */   "TAC_IO",           "I/O Interruption",
    /*   7 */   "TAC_FETCH_OVF",    "Fetch overflow",
    /*   8 */   "TAC_STORE_OVF",    "Store overflow",
    /*   9 */   "TAC_FETCH_CNF",    "Fetch conflict",
    /*  10 */   "TAC_STORE_CNF",    "Store conflict",
    /*  11 */   "TAC_INSTR",        "Restricted instruction",
    /*  12 */   "TAC_FPGM",         "PGM Interruption (Filtered)",
    /*  13 */   "TAC_NESTING",      "Nesting Depth exceeded",
    /*  14 */   "TAC_FETCH_OTH",    "Cache (fetch related)",
    /*  15 */   "TAC_STORE_OTH",    "Cache (store related)",
    /*  16 */   "TAC_CACHE_OTH",    "Cache (other)",
    /*  17 */   "17",               "17",
    /*  18 */   "18",               "18",
    /*  19 */   "TAC_GUARDED",      "Guarded-Storage Event related",

//  /*  20 */   "TAC_?????",        "Some future TAC code...",
//  /*  21 */   "TAC_?????",        "Some future TAC code...",
//  /* etc */   "TAC_?????",        "Some future TAC code...",

//  /* 255 */   "TAC_MISC",         "Miscellaneous condition",
//  /* 256 */   "TAC_TABORT",       "TABORT instruction",
};
static const char* tac2name( U64 tac, bool bLong )
{
    if (tac < (_countof( tac_names )/2))
        return tac_names[ (tac * 2) + (bLong ? 1 : 0) ];
    if (tac < 255) return (bLong ? "(undefined Abort Code)"  : "TAC_?????"  );
    return (tac == 255) ? (bLong ? "Miscellaneous condition" : "TAC_MISC"   )
                        : (bLong ? "TABORT instruction"      : "TAC_TABORT" );
}
const char* tac2long ( U64 tac ) { return tac2name( tac, true  ); }
const char* tac2short( U64 tac ) { return tac2name( tac, false ); }

/*-------------------------------------------------------------------*/
/*            DEBUG:  Constraint Violation Names                     */
/*-------------------------------------------------------------------*/
const char* txf_why_str( char* buffer, int buffsize, int why )
{
    #define TXF_WHY_FORMAT( _why ) ((why & _why) ? " " #_why : "")

    snprintf( buffer, buffsize, "%s%s%s%s%s%s%s%s%s%s%s%s%s"

        , TXF_WHY_FORMAT( TXF_WHY_INSTRADDR                )
        , TXF_WHY_FORMAT( TXF_WHY_INSTRCOUNT               )
        , TXF_WHY_FORMAT( TXF_WHY_RAND_ABORT               )
        , TXF_WHY_FORMAT( TXF_WHY_CSP_INSTR                )
        , TXF_WHY_FORMAT( TXF_WHY_CSPG_INSTR               )
        , TXF_WHY_FORMAT( TXF_WHY_SIE_EXIT                 )
        , TXF_WHY_FORMAT( TXF_WHY_CONFLICT                 )
        , TXF_WHY_FORMAT( TXF_WHY_MAX_PAGES                )
        , TXF_WHY_FORMAT( TXF_WHY_CONTRAN_INSTR            )
        , TXF_WHY_FORMAT( TXF_WHY_CONTRAN_BRANCH           )
        , TXF_WHY_FORMAT( TXF_WHY_CONTRAN_RELATIVE_BRANCH  )
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_INSTR               )
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_FLOAT_INSTR         )
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_ACCESS_INSTR        )
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_NONRELATIVE_BRANCH  )
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_BRANCH_SET_MODE     )
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_SET_ADDRESSING_MODE )
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_MISC_INSTR          )
    );
    return buffer;
}

/*-------------------------------------------------------------------*/
/*               DEBUG:  Hex dump a cache line                       */
/*-------------------------------------------------------------------*/
void dump_cache( const char* pfx, int linenum , const BYTE* line)
{
    char*  /* (work) */  buf  = NULL;
    const char*          dat  = (const char*) line;
    static const size_t  skp  = 0;
    static const size_t  amt  = ZCACHE_LINE_SIZE;
    U64 /* (cosmetic) */ adr  = linenum << ZCACHE_LINE_SHIFT;
    static const size_t  bpg  = 4; // bytes per formatted group
    static const size_t  gpl  = 4; // formatted groups per line

    hexdumpe16( pfx, &buf, dat, skp, amt, adr, bpg, gpl );

    if (buf)
    {
        LOGMSG( "%s", buf );
        free( buf );
    }
    else
    {
        // "TXF: error in function %s: %s"
        WRMSG( HHC17708, "E", "dump_cache()", strerror( errno ));
    }
}

/*-------------------------------------------------------------------*/
/*                   DEBUG:  Hex dump TDB                            */
/*-------------------------------------------------------------------*/
void dump_tdb( TDB* tdb, U64 logical_addr )
{
    char*  /* (work) */  buf  = NULL;
    const char*          dat  = (const char*) tdb;
    static const size_t  skp  = 0;
    static const size_t  amt  = sizeof( TDB );
    static const U64     adr  = 0; // (cosmetic)
    static const size_t  bpg  = 8; // bytes per formatted group
    static const size_t  gpl  = 2; // formatted groups per line

    // "TXF: TDB at 0x%16.16"PRIX64":"
    WRMSG( HHC17709, "D", logical_addr );

    // "TXF: TDB: +"
    hexdumpe16( DUMP_PFX( HHC17710 ), &buf, dat, skp, amt, adr, bpg, gpl );

    if (buf)
    {
        LOGMSG( "%s", buf );
        free( buf );
    }
    else
    {
        // "TXF: error in function %s: %s"
        WRMSG( HHC17708, "E", "dump_tdb()", strerror( errno ));
    }
}

#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */

#endif /*!defined(_GEN_ARCH)*/
