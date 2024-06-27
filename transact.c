/* TRANSACT.C   (C) Copyright "Fish" (David B. Trout), 2017-2021     */
/*              (C) Copyright Bob Wood, 2019-2020                    */
/*              (C) and others 2021-2023                             */
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

/*-------------------------------------------------------------------*/
/*                  Credit where credit is due                       */
/*-------------------------------------------------------------------*/
/* The overall design of Hercules's Transactional-Execution Facility */
/* was originally designed by Bob Wood so he gets most of the credit */
/* for what you see. Fish only gets credit for minor corrections and */
/* enhancements to Bob's design. I just wanted to make that clear.   */
/* It was only Fish which then -- along with LOTS of help from MANY  */
/* other fellow Hercules developers -- that made various minor fixes */
/* and enhancements to Bob's original design to reach the point of   */
/* what you see here. But it is Bob Wood that is the TRUE HERCULEAN  */
/* that should get the bulk of the credit for Hercules's overall TXF */
/* (Transactional Execution Facility) implementation. THANKS BOB!    */
/* And a special THANK YOU to Peter J. and Jürgen W. too for all of  */
/* the MANY long hours each of you put in testing and bug hunting!   */
/* Thank you all! You're the greatest! The Hercules project is truly */
/* indebted to each and every one of you! Thank you! :`)             */
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
#define ENABLE_TXF_PTT          // #define: enable, #undef: disable
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
DEF_INST( perform_processor_assist )
{
int     r1, r2;                         /* Operand register numbers  */
int     m3;                             /* M3 Mask value             */
U32     abort_count;                    /* Transaction Abort count   */

    RRF_M( inst, regs, r1, r2, m3 );

    UNREFERENCED( r2 );

    TXF_INSTR_CHECK( regs );

    /* Retrieve abort count */
    abort_count = regs->GR_L( r1 );

    PTT_TXF( "TXF PPA", m3, abort_count, 0 );

    switch (m3)
    {
    case 1: // Transaction Abort Assist
    {
        regs->txf_PPA = abort_count;
        return;
    }
#if defined( FEATURE_081_PPA_IN_ORDER_FACILITY )
    case 15: // In-order Execution Assist
    {
        if (FACILITY_ENABLED( 081_PPA_IN_ORDER, regs ))
        {
            /*
               "When the function code in the M3 field is 15 and the
                PPA-in-order facility is installed, the processor is
                requested to complete processing all instructions prior
                to this PPA instruction, as observed by this CPU, before
                attempting storage-operand references for any instruction
                after this PPA instruction."

               "The R1 and R2 fields are ignored and the instruction is
                executed as a no-operation."

               "The in-order-execution assist does not necessarily perform
                any of the steps for architectural serialization described
                in the section "CPU Serialization" on page 5-130."
            */

            /* NOTE: Hercules does not currently support this assist,
               but if we ever do add code to support it this is where
               such code should go...
            */

            // TODO: add PPA-in-order Facility support code here...

            return;
        }

        return;  /* (ignore unsupported assists) */
    }
#endif // defined(  FEATURE_081_PPA_IN_ORDER_FACILITY )

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

    UNREFERENCED( r2 );

    TXF_SIE_INTERCEPT( regs, ETND );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF ETND", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    TXFC_INSTR_CHECK( regs );

    regs->GR_L(r1) = (U32) regs->txf_tnd;

    PTT_TXF( "TXF ETND", 0, 0, regs->txf_tnd );

} /* end DEF_INST( extract_transaction_nesting_depth ) */

/*-------------------------------------------------------------------*/
/*       Reset CONSTRAINED trans instruction fetch constraint        */
/*-------------------------------------------------------------------*/
void ARCH_DEP( reset_txf_aie )( REGS* regs )
{
    regs->txf_contran  = false;
    regs->txf_aie      = NULL;
    regs->txf_aie_aiv  = 0;
    regs->txf_aie_aiv2 = 0;
    regs->txf_aie_off2 = 0;
}

/*-------------------------------------------------------------------*/
/*        Set CONSTRAINED trans instruction fetch constraint         */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_txf_aie )( REGS* regs )
{
    regs->txf_contran  = true;
    regs->txf_aie      = regs->ip - 6 + 256; // (minus-6 for TBEGINC)
    regs->txf_aie_aiv  = regs->AIV;

    /* Is aie in next page? */
    if (regs->txf_aie >= (regs->aip + ZPAGEFRAME_PAGESIZE))
    {
        /* Define aie offset into next page for vstore.h */
        regs->txf_aie_aiv2 = regs->txf_aie_aiv + ZPAGEFRAME_PAGESIZE;
        regs->txf_aie_off2 = regs->txf_aie - (regs->aip + ZPAGEFRAME_PAGESIZE);
    }
    else
    {
        /* Prevent vstore.h next page offset match */
        regs->txf_aie_aiv2 = ~regs->txf_aie_aiv;
    }
}

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
int         txf_tnd, txf_tac, slot;
bool        per_tend = false;           /* true = check for PER TEND */

    S( inst, regs, b2, effective_addr2 );

    TXF_SIE_INTERCEPT( regs, TEND );

    TXF_EXECUTE_INSTR_CHECK( regs );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF end", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    if (regs->txf_abortctr)
    {
        PTT_TXF( "*TXF end", regs->txf_abortctr, regs->txf_contran, regs->txf_tnd );
        regs->txf_why |= TXF_WHY_RAND_ABORT;
        ABORT_TRANS( regs, ABORT_RETRY_PGMCHK, regs->txf_random_tac );
        UNREACHABLE_CODE( return );
    }

    /* Set condition code based on CURRENT transaction level */
    if (!regs->txf_tnd)
    {
        /* Not currently in transactional-execution mode.
           Set CC 2 and treat as no-op (i.e. just return
           since there is no active transaction to end)
        */
        PTT_TXF( "*TXF end", 0, 0, 0 );
        regs->psw.cc = 2;   /* CPU wasn't in transaction mode */
        return;             /* Nothing to do. Just return. */
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
        BYTE*  txf_aie;          /* (saved original value) */
        U64    txf_aie_aiv;      /* (saved original value) */
        U64    txf_aie_aiv2;     /* (saved original value) */
        int    txf_aie_off2;     /* (saved original value) */
        BYTE   refchg;           /* (storagekey work flag) */

        SYNCHRONIZE_CPUS( regs );

        OBTAIN_TXFLOCK( regs );
        {
            regs->txf_tnd--;

            txf_tnd = regs->txf_tnd;
            txf_tac = regs->txf_tac;
        }
        RELEASE_TXFLOCK( regs );

        TXF_TRACE_INIT( regs );

        /* Still in transaction-execution mode? */
        if (txf_tnd)
        {
            /*---------------------------------------------------------*/
            /* Abort transaction if abort code already set by someone. */
            /*---------------------------------------------------------*/
            if (txf_tac)
            {
                PTT_TXF( "*TXF end", txf_tac, regs->txf_contran, txf_tnd );
                regs->txf_why |= TXF_WHY_DELAYED_ABORT;
                ABORT_TRANS( regs, ABORT_RETRY_CC, txf_tac );
                UNREACHABLE_CODE( return );
            }

            /*--------------------------------------------*/
            /*           NESTED TRANSACTION END           */
            /*--------------------------------------------*/

            if (TXF_TRACE( regs, SUCCESS, regs->txf_contran ))
            {
                // "TXF: %s%02X: %sSuccessful %s Nested TEND for TND %d => %d"
                WRMSG( HHC17700, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                    TXF_CONSTRAINED( regs->txf_contran ), txf_tnd + 1, txf_tnd );
            }

            /* If we're now at or below the highest nesting level
               that allowed AR changes or float instructions, then
               enable (set) the corresponding control flag.
               Otherwise leave it alone (should already be off).
            */
            if (txf_tnd <= regs->txf_higharchange)
            {
                regs->txf_higharchange = txf_tnd;
                regs->txf_ctlflag |= TXF_CTL_AR;
            }

            if (txf_tnd <= regs->txf_highfloat)
            {
                regs->txf_highfloat = txf_tnd;
                regs->txf_ctlflag |= TXF_CTL_FLOAT;
            }

            /* Set PIFC for this nesting level */
            regs->txf_pifc = regs->txf_pifctab[ txf_tnd - 1 ];

            /* Remain in transactional-execution mode */
            PTT_TXF( "TXF end", 0, regs->txf_contran, txf_tnd );
            RELEASE_INTLOCK( regs );
            return;
        }

        /*---------------------------------------------------------*/
        /*         THE OUTERMOST TRANSACTION HAS ENDED             */
        /*---------------------------------------------------------*/

        /*---------------------------------------------------------*/
        /* Abort transaction if abort code already set by someone. */
        /*---------------------------------------------------------*/
        if (txf_tac)
        {
            PTT_TXF( "*TXF end", txf_tac, regs->txf_contran, txf_tnd );
            regs->txf_why |= TXF_WHY_DELAYED_ABORT;
            regs->txf_tnd++; // (prevent 'abort_transaction' crash)
            ABORT_TRANS( regs, ABORT_RETRY_CC, txf_tac );
            UNREACHABLE_CODE( return );
        }

        /*---------------------------------------------------------*/
        /*  End the transaction normally if no conflicts detected  */
        /*---------------------------------------------------------*/

        txf_contran  = regs->txf_contran;        /* save */
        txf_abortctr = regs->txf_abortctr;       /* save */
        txf_aie      = regs->txf_aie;            /* save */
        txf_aie_aiv  = regs->txf_aie_aiv;        /* save */
        txf_aie_aiv2 = regs->txf_aie_aiv2;       /* save */
        txf_aie_off2 = regs->txf_aie_off2;       /* save */

        regs->txf_contran  = false;              /* reset */
        regs->txf_abortctr = 0;                  /* reset */
        regs->txf_aie      = NULL;               /* reset */
        regs->txf_aie_aiv  = 0;                  /* reset */
        regs->txf_aie_aiv2 = 0;                  /* reset */
        regs->txf_aie_off2 = 0;                  /* reset */

        /*---------------------------------------------------------*/
        /*                 Scan for conflicts                      */
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
        /*  the channel subsystem has stored into the cache line.  */
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
                /*          TRANSACTION FAILURE         */
                /*--------------------------------------*/

                if (pmap->virtpageaddr)
                    regs->txf_conflict = pmap->virtpageaddr + (j << ZCACHE_LINE_SHIFT);
                else
                    regs->txf_conflict = 0;

                txf_tac = (pmap->cachemap[j] == CM_STORED) ?
                    TAC_STORE_CNF : TAC_FETCH_CNF;

                PTT_TXF( "*TXF end", txf_tac, txf_contran, txf_tnd );

                regs->txf_contran  = txf_contran;      /* restore */
                regs->txf_abortctr = txf_abortctr;     /* restore */
                regs->txf_aie      = txf_aie;          /* restore */
                regs->txf_aie_aiv  = txf_aie_aiv;      /* restore */
                regs->txf_aie_aiv2 = txf_aie_aiv2;     /* restore */
                regs->txf_aie_off2 = txf_aie_off2;     /* restore */

                regs->txf_why |= TXF_WHY_CONFLICT;
                regs->txf_tnd++; // (prevent 'abort_transaction' crash)
                ABORT_TRANS( regs, ABORT_RETRY_CC, txf_tac );
                UNREACHABLE_CODE( return );
            }
        }

        /*---------------------------------------------------------*/
        /*                 TRANSACTION SUCCESS                     */
        /*---------------------------------------------------------*/
        /*  We have now validated all of the cache lines that we   */
        /*  touched, and all other CPUs are dormant.  Now update   */
        /*  the real cache lines from the shadow cache lines.      */
        /*---------------------------------------------------------*/

        if (TXF_TRACE( regs, SUCCESS, txf_contran ))
        {
            // "TXF: %s%02X: %sSuccessful Outermost %s TEND"
            WRMSG( HHC17701, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                TXF_CONSTRAINED( txf_contran ));
        }

        /* Commit all of our transactional changes */
        pmap = regs->txf_pagesmap;

        for (i=0; i < regs->txf_pgcnt; i++, pmap++)
        {
            refchg = STORKEY_REF; // (wouldn't be in map if it wasn't!)

            if (TXF_TRACE_PAGES( regs, txf_contran ))
            {
                // "TXF: %s%02X: %svirt 0x%16.16"PRIX64", abs 0x%16.16"PRIX64", alt 0x%16.16"PRIX64
                WRMSG( HHC17704, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                    (U64) pmap->virtpageaddr,
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
                refchg |= STORKEY_CHANGE;

                if (TXF_TRACE_LINES( regs, txf_contran ))
                {
                    // "TXF: %s%02X: %sWe stored:  +"
                    dump_cache( regs, TXF_DUMP_PFX( HHC17707 ), j, altaddr );
                }
            }

            ARCH_DEP( or_storage_key )( MAIN_TO_ABS( pmap->mainpageaddr ), refchg );
        }

        /* Mark the page map as now being empty */
        regs->txf_pgcnt = 0;

        /*------------------------------------------*/
        /*  We are done. Release INTLOCK and exit.  */
        /*------------------------------------------*/

        PTT_TXF( "TXF end", 0, 0, 0 );

        /*---------------------------------------------*/
        /*          Trace failure retry success        */
        /*---------------------------------------------*/
        if (1
            && MLVL( VERBOSE )
            && regs->txf_aborts
            && TXF_TRACE( regs, FAILURE, txf_contran )
        )
        {
            // "TXF: %s%02X: %s%s transaction succeeded after %d retries"
            WRMSG( HHC17718, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                TXF_CONSTRAINED( txf_contran ), regs->txf_aborts );
        }

        /* Track TXF statistics */
        slot = regs->txf_aborts < TXF_STATS_RETRY_SLOTS ?
               regs->txf_aborts : TXF_STATS_RETRY_SLOTS - 1;
        TXF_STATS( retries[ slot ], txf_contran );

        /* Track retries high watermark */
        if ((U64)regs->txf_aborts > sysblk.txf_stats[ txf_contran ].txf_retries_hwm)
            sysblk.txf_stats[ txf_contran ].txf_retries_hwm = (U64)regs->txf_aborts;

        /* Reset abort count */
        regs->txf_aborts = 0;

        /* Reset PPA assistance */
        regs->txf_PPA = 0;

        /* Reset CONSTRAINED trans instruction fetch constraint */
        ARCH_DEP( reset_txf_aie )( regs );

        PERFORM_SERIALIZATION( regs );

        /* Check if a transaction-end PER event is wanted */
        if (EN_IC_PER_TEND( regs ))
        {
            regs->peradr = regs->periaddr;
            ON_IC_PER_TEND( regs );
            per_tend = true;
        }
    }
    RELEASE_INTLOCK( regs );

    /* If a PER TEND event was generated, check to
       see if we can take the interrupt right away.
    */
    if (per_tend)
    {
        if (OPEN_IC_PER_TEND( regs ))
            RETURN_INTCHECK( regs );
    }

} /* end DEF_INST( transaction_end ) */


/*-------------------------------------------------------------------*/
/* B2FC TABORT - Transaction Abort                               [S] */
/*-------------------------------------------------------------------*/
DEF_INST( transaction_abort )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );

    TXF_SIE_INTERCEPT( regs, TABORT );

    TXFC_INSTR_CHECK( regs );
    TXF_EXECUTE_INSTR_CHECK( regs );

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

    TXFC_INSTR_CHECK( regs );

    /* CC in transaction abort PSW = 2 or 3 based on operand2 bit 63 */
    regs->txf_tapsw.cc = (effective_addr2 & 0x1) == 0 ? 2 : 3;

    /* Abort the transaction */

    PTT_TXF( "TXF TABORT", effective_addr2, regs->txf_contran, regs->txf_tnd );
    regs->txf_why |= TXF_WHY_TABORT_INSTR;
    ABORT_TRANS( regs, ABORT_RETRY_CC, (int) effective_addr2 );
    UNREACHABLE_CODE( return );

} /* end DEF_INST( transaction_abort ) */


/*-------------------------------------------------------------------*/
/* E325 NTSTG - Nontransactional Store                       [RXY-a] */
/*-------------------------------------------------------------------*/
DEF_INST( nontransactional_store )
{
int     r1;                             /* Value of r1 field         */
int     b2;                             /* Base of effective address */
int     x2;                             /* Index register            */
VADR    effective_addr2;                /* Effective address         */

    RXY( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

    PTT_TXF( "TXF NTSTG", regs->GR_G( r1 ), effective_addr2, 0 );

    /* Nontransactionally store register contents at operand address */
    regs->txf_NTSTG = true;
    ARCH_DEP( vstore8 )( regs->GR_G( r1 ), effective_addr2, b2, regs );

} /* end DEF_INST( nontransactional_store ) */


/* (forward reference) */
void ARCH_DEP( process_tbegin )( bool txf_contran, REGS* regs, S16 i2,
                                 U64 tdba, int b1 );

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

    SIL( inst, regs, i2, b1, effective_addr1 );
    PER_ZEROADDR_XCHECK( regs, b1 );

    TXF_SIE_INTERCEPT( regs, TBEGIN );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF TBEGIN", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    TXF_EXECUTE_INSTR_CHECK( regs );

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
        DW_CHECK( effective_addr1, regs );

        /* Check that all bytes of the TDB are accessible */
        ARCH_DEP( validate_operand )( effective_addr1, b1,
                                      sizeof( TDB ) - 1,
                                      ACCTYPE_WRITE, regs );
    }

    OBTAIN_INTLOCK( regs );
    {
        /* Let our helper function do all the grunt work */
        PTT_TXF( "TXF TBEGIN", 0, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( process_tbegin )( false, regs, i2, effective_addr1, b1 );
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

    TXF_SIE_INTERCEPT( regs, TBEGINC );

    if (!(regs->CR(0) & CR0_TXC))
    {
        PTT_TXF( "*TXF TBEGINC", regs->CR(0), regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    TXF_EXECUTE_INSTR_CHECK( regs );

    /* CONSTRAINED: Specification Exception if b1 is non-zero */
    if (b1)
    {
        PTT_TXF( "*TXF TBEGINC", b1, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        UNREACHABLE_CODE( return );
    }

    /* CONSTRAINED: ignore some i2 bits */
    i2 &= ~(TXF_CTL_FLOAT | TXF_CTL_PIFC);

    /* Obtain interrupt lock so "process_tbegin" can do SYNCHRONIZE_CPUS */
    OBTAIN_INTLOCK( regs );
    {
        /* Let our helper function do all the grunt work */
        PTT_TXF( "TXF TBEGINC", 0, regs->txf_contran, regs->txf_tnd );
        ARCH_DEP( process_tbegin )( true, regs, i2, 0, 0 );
    }
    RELEASE_INTLOCK( regs );

} /* end DEF_INST( transaction_begin_constrained ) */

#endif /* defined( FEATURE_050_CONSTR_TRANSACT_FACILITY ) */

/*-------------------------------------------------------------------*/
/*       process_tbegin  --  common TBEGIN/TBEGINC logic             */
/*-------------------------------------------------------------------*/
/*    The interrupt lock (INTLOCK) *MUST* be held upon entry         */
/*-------------------------------------------------------------------*/
void ARCH_DEP( process_tbegin )( bool txf_contran, REGS* regs, S16 i2,
                                 U64 tdba, int b1 )
{
int         n, tdc;
TPAGEMAP   *pmap;

    /* Temporarily pause other CPUs while TBEGIN/TBEGINC is processed.
       NOTE: this *must* be done *BEFORE* checking nesting depth. */
    SYNCHRONIZE_CPUS( regs );

    PERFORM_SERIALIZATION( regs );

    /* Check for maximum nesting depth exceeded */
    if (regs->txf_tnd >= MAX_TXF_TND)
    {
        PTT_TXF( "*TXF beg", MAX_TXF_TND, regs->txf_contran, regs->txf_tnd );
        regs->txf_why |= TXF_WHY_NESTING;
        ABORT_TRANS( regs, ABORT_RETRY_PGMCHK, TAC_NESTING );
        UNREACHABLE_CODE( return );
    }

    /* Count transaction */
    atomic_update32( &sysblk.txf_counter, +1 );

    TXFC_INSTR_CHECK( regs );    /* Unallowed in CONSTRAINED mode */

    /*---------------------------------------------*/
    /*  Increase nesting depth                     */
    /*---------------------------------------------*/
    OBTAIN_TXFLOCK( regs );
    {
        regs->txf_tnd++;
    }
    RELEASE_TXFLOCK( regs );

    /* set cc=0 at transaction start */
    regs->psw.cc = TXF_CC_SUCCESS;

    TXF_TRACE_INIT( regs );

    /* first/outermost transaction? */
    if (regs->txf_tnd == 1)
    {
        /*-----------------------------------------------------------*/
        /*              BEGIN OUTERMOST TRANSACTION                  */
        /*-----------------------------------------------------------*/

        /* Count total transactions */
        if (!regs->txf_aborts)
            TXF_STATS( trans, txf_contran );

        /* Set internal TDB to invalid until it's actually populated */
        memset( &regs->txf_tdb, 0, sizeof( TDB ));

        /* Initialize the page map */

        pmap = regs->txf_pagesmap;
        regs->txf_pgcnt = 0;

        for (n=0; n < MAX_TXF_PAGES; n++, pmap++)
        {
            pmap->mainpageaddr = NULL;
            memset( pmap->cachemap, CM_CLEAN, sizeof( pmap->cachemap ));
        }

        /* Initialize other fields */

        regs->txf_UPGM_abort = false;      /* reset flag             */
        regs->txf_contran    = txf_contran;/* set transaction type   */
        regs->txf_instctr    = 0;          /* instruction counter    */
        regs->txf_abortctr   = 0;          /* abort instr count      */
        regs->txf_tac        = 0;          /* clear the abort code   */
        regs->txf_conflict   = 0;          /* clear conflict address */
        regs->txf_piid       = 0;          /* program interrupt id   */
        regs->txf_lastarn    = 0;          /* last access arn        */
        regs->txf_why        = 0;          /* no abort cause (yet)   */

        memcpy( regs->txf_savedgr, regs->gr, sizeof( regs->txf_savedgr ));
        memset( regs->txf_pifctab, 0, sizeof( regs->txf_pifctab ));

        regs->txf_ctlflag      = (i2 >> 0) & (TXF_CTL_AR | TXF_CTL_FLOAT);
        regs->txf_pifc         = (i2 >> 0) & (TXF_CTL_PIFC);
        regs->txf_gprmask      = (i2 >> 8) & (0xFF);
        regs->txf_higharchange = (regs->txf_ctlflag & TXF_CTL_AR)    ? regs->txf_tnd : 0;
        regs->txf_highfloat    = (regs->txf_ctlflag & TXF_CTL_FLOAT) ? regs->txf_tnd : 0;
        regs->txf_tdba         = tdba;
        regs->txf_tdba_b1      = b1;

        /* Set CONSTRAINED trans instruction fetch constraint */
        if (txf_contran)
            ARCH_DEP( set_txf_aie )( regs );

        /*----------------------------------*/
        /*  Save the Transaction Abort PSW  */
        /*----------------------------------*/
        {
            PSW origpsw;
            memcpy( &origpsw, &regs->psw, sizeof( PSW ));
            {
                n = txf_contran ? -6 : 0;
                regs->psw.IA = PSW_IA_FROM_IP( regs, n );
                memcpy( &regs->txf_tapsw, &regs->psw, sizeof( PSW ));
                regs->txf_ip  = regs->ip;
                regs->txf_aip = regs->aip;
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
                /* FALLTHRU */

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

        if (txf_contran)
        {
            /* Nested CONSTRAINED transactions are not allowed */
            if (regs->txf_contran)
            {
                PTT_TXF( "*TXF begc", MAX_TXF_TND, regs->txf_contran, regs->txf_tnd );
                regs->txf_why |= TXF_WHY_NESTING;
                ABORT_TRANS( regs, ABORT_RETRY_PGMCHK, TAC_NESTING );
                UNREACHABLE_CODE( return );
            }

            /* CONSTRAINED nested within unconstrained... */

            /* For CONSTRAINED transactions, if the transaction
               nesting depth is already greater than zero (i.e.
               the CPU is already in the unconstrained TX mode),
               then execution simply proceeds as if this were a
               unconstrained transaction. (The TX mode does NOT
               switch to constrained mode)

               In this case, the effective F control is zeroed,
               and the effective PIFC remains unchanged. This
               allows an outer, nonconstrained transaction to
               call a service function that may use constrained
               TX mode.
            */
            i2 &= ~(TXF_CTL_FLOAT | TXF_CTL_PIFC);
            i2 |= regs->txf_pifc;
        }

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

    if (TXF_TRACE( regs, SUCCESS, regs->txf_contran ))
    {
        // "TXF: %s%02X: %sSuccessful %s TBEGIN%s; TND now %d"
        WRMSG( HHC17702, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
            regs->txf_tnd > 1 ? "nested" : "outermost",
            regs->txf_contran ? "C" : "", regs->txf_tnd );
    }

    /*--------------------------------------------------------------*/
    /*           Report failed transaction retries                  */
    /*--------------------------------------------------------------*/
    if (1
        && MLVL( VERBOSE )
        && regs->txf_aborts
        && TXF_TRACE( regs, FAILURE, regs->txf_contran )
    )
    {
        // "TXF: %s%02X: %s%s transaction retry #%d..."
        WRMSG( HHC17717, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
            TXF_CONSTRAINED( regs->txf_contran ), regs->txf_aborts );
    }

} /* end ARCH_DEP( process_tbegin ) */

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) \
&& !defined( COMPILE_THIS_ONLY_ONCE )
   #define   COMPILE_THIS_ONLY_ONCE
/*-------------------------------------------------------------------*/
/*         Transaction Abort Code to Condition Code table            */
/*-------------------------------------------------------------------*/
/*     (see Fig. 5-14 on page 5-102 of SA22-7832-12 z/Arch POPs)     */
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
/*  and then does one of the following based on the 'retry' flag:    */
/*                                                                   */
/*    retry = ABORT_RETRY_RETURN: return to the caller. Used when    */
/*            the abort is called from e.g. an interrupt handler.    */
/*                                                                   */
/*    retry = ABORT_RETRY_CC: set the condition code and then do     */
/*            a longjmp to progjmp.                                  */
/*                                                                   */
/*    retry = ABORT_RETRY_PGMCHK: generate a program check. Used     */
/*            only for constraint violations.                        */
/*                                                                   */
/*  PROGRAMMING NOTE: a negative ABORT_RETRY_PGMCHK code is passed   */
/*  when called directly from run_cpu/run_sie logic in cpu.c/sie.c   */
/*  to properly deal with the fact that the instruction, at that     */
/*  that point in time, has not been dispatched nor decoded yet,     */
/*  and thus the ip instruction pointer is still pointing AT the     */
/*  instruction being aborted instead of past it (as what normally   */
/*  occurs when we're called from an instruction function) as well   */
/*  as the fact that the ilc has not been calculated yet either      */
/*  (since the instruction hasn't been decoded yet). This is done    */
/*  SOLELY for proper reporting of where the actual program check    */
/*  interruption actually occurred.                                  */
/*                                                                   */
/*-------------------------------------------------------------------*/
void ARCH_DEP( abort_transaction )( REGS* regs, int raw_retry, int txf_tac, const char* loc )
{
#if !defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

    /* S370 and S390 don't support Transactional-Execution Facility */

    UNREFERENCED( regs      );
    UNREFERENCED( raw_retry );
    UNREFERENCED( txf_tac   );
    UNREFERENCED( loc       );

    CRASH();   /* Should NEVER be called for S/370 or S/390 */

#else /* only Z900 supports Transactional-Execution Facility */

BYTE       txf_gprmask;
int        txf_tnd, i;
bool       txf_contran, had_INTLOCK;
U32        txf_piid;
U64        txf_conflict;
U64        txf_bea;
TDB*       pi_tdb   = NULL; /* Program Interrupt TDB @ fixed 0x1800  */
TDB*       tb_tdb   = NULL; /* TBEGIN-specified TDB @ operand-1 addr */
VADR       txf_atia;        /* Aborted Transaction Instruction Addr. */
int        retry;           /* Actual retry code                     */

    UNREFERENCED( loc );

    PTT_TXF( "*TXF abort", raw_retry, txf_tac, regs->txf_contran );

    /* Set the initial Transaction Abort Code */
    if (!regs->txf_tac)
        regs->txf_tac = txf_tac;

    TXF_TRACE_INIT( regs );

    // LOGIC ERROR if CPU not in transactional-execution mode
    if (!regs->txf_tnd)
        CRASH();

    /* Get the ACTUAL 'retry' code */
    retry = ((raw_retry < 0) ? -raw_retry : raw_retry);

    /* Normal abort? Or special instruction dispatch abort? */
    if (raw_retry >= 0)
    {
        /* Normal instruction abort: the PREVIOUS instruction
           is the one where the abort actually occurred at. */
        txf_atia = PSW_IA_FROM_IP( regs, -REAL_ILC( regs ));
        PTT_TXF( "TXF ATIA", txf_atia, 0, -REAL_ILC( regs ) );
    }
    else // (raw_retry < 0)
    {
        /* Instruction dispatch abort: the CURRENT instruction
           address is where the abort actually occurred at. */
        txf_atia = PSW_IA_FROM_IP( regs, 0 );
        PTT_TXF( "TXF ATIA", txf_atia, 0, 0 );
    }

    /* Obtain the interrupt lock if we don't already have it */
    if (IS_INTLOCK_HELD( regs ))
        had_INTLOCK = true;
    else
    {
        OBTAIN_INTLOCK( regs );
        had_INTLOCK = false;
    }

    PERFORM_SERIALIZATION( regs );

    /*---------------------------------------------*/
    /*       Failure of transaction retry?         */
    /*---------------------------------------------*/
    if (1
        && MLVL( VERBOSE )
        && regs->txf_aborts
        && TXF_TRACE( regs, FAILURE, regs->txf_contran )
    )
    {
        // "TXF: %s%02X: %s%s transaction retry #%d FAILED!"
        WRMSG( HHC17719, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
            TXF_CONSTRAINED( regs->txf_contran ), regs->txf_aborts );
    }

    /*---------------------------------------------*/
    /*          Count the abort/retry              */
    /*---------------------------------------------*/

    /* Count total retries for this transaction */
    regs->txf_aborts++;

    /* Provide PPA assist for constrained transactions too */
    if (regs->txf_contran)
        regs->txf_PPA = regs->txf_aborts;

    /* Track total aborts by cause (TAC) */
    if (regs->txf_tac == TAC_MISC)
        TXF_STATS( aborts_by_tac_misc, regs->txf_contran );
    else
    {
        int slot = regs->txf_tac < TXF_STATS_TAC_SLOTS ?
                   regs->txf_tac : 0; // (0 == "other")
        TXF_STATS( aborts_by_tac[ slot ], regs->txf_contran );
    }

    /*---------------------------------------------*/
    /*  Clean up the transaction flags             */
    /*---------------------------------------------*/

    txf_tnd      = regs->txf_tnd;        /* save orig value */
    txf_contran  = regs->txf_contran;    /* save orig value */
    txf_gprmask  = regs->txf_gprmask;    /* save orig value */
    txf_conflict = regs->txf_conflict;   /* save orig value */
    txf_piid     = regs->txf_piid;       /* save orig value */
    txf_bea      = regs->bear;           /* save orig value */

    /*---------------------------------------------*/
    /*  Log the failure if debugging enabled       */
    /*---------------------------------------------*/

    if (TXF_TRACE( regs, FAILURE, txf_contran ))
    {
        /* Report the reason WHY the transaction was aborted */
        char why[ 256 ] = {0};

        if (regs->txf_why)
            txf_why_str( why, sizeof( why ), regs->txf_why );
        else
            STRLCPY( why, " ?" );

        // "TXF: %s%02X: %sFailed %s %s Transaction for TND %d: %s = %s, why =%s"
        WRMSG( HHC17703, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
            txf_tnd > 1 ? "Nested" : "Outermost",
            TXF_CONSTRAINED( txf_contran ), txf_tnd,
            tac2short( txf_tac ), tac2long( txf_tac ), why );

        /* If this is a delayed abort, log who detected/requested it */
        /* Delayed aborts are those detected by another CPU, not us. */
        if (regs->txf_why & TXF_WHY_DELAYED_ABORT)
        {
            char who[16] = {0};

            if (regs->txf_who < 0)
                STRLCPY( who, "channel" );
            else if (regs->txf_who == regs->cpuad) // (shouldn't occur)
                STRLCPY( who, "ourself" );
            else
                MSGBUF( who, "%s%02X", PTYPSTR( regs->txf_who ), regs->txf_who );

            if (MLVL( VERBOSE ))
            {
                // "TXF: %s%02X: %sAbort set by %s at %s"
                WRMSG( HHC17720, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                    who, TRIMLOC( regs->txf_loc ));
            }
        }

        /* Print page map if requested */
        if (TXF_TRACE_MAP( regs, txf_contran ))
        {
            TPAGEMAP*  pmap;
            BYTE*      mainaddr;
            BYTE*      altaddr;
            int        i, j;

            pmap = regs->txf_pagesmap;

            for (i=0; i < regs->txf_pgcnt; i++, pmap++)
            {
                if (TXF_TRACE_PAGES( regs, txf_contran ))
                {
                    // "TXF: %s%02X: %svirt 0x%16.16"PRIX64", abs 0x%16.16"PRIX64", alt 0x%16.16"PRIX64
                    WRMSG( HHC17704, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                        (U64) pmap->virtpageaddr,
                        (U64)(pmap->mainpageaddr - regs->mainstor),
                        (U64) pmap->altpageaddr );
                }

                if (TXF_TRACE_LINES( regs, txf_contran ))
                {
                    for (j=0; j < ZCACHE_LINE_PAGE; j++)
                    {
                        if (pmap->cachemap[j] == CM_CLEAN)
                            continue;

                        mainaddr = pmap->mainpageaddr + (j << ZCACHE_LINE_SHIFT);
                        altaddr  = pmap->altpageaddr  + (j << ZCACHE_LINE_SHIFT);

                        // "TXF: %s%02X: %sThere now:  +"
                        dump_cache( regs, TXF_DUMP_PFX( HHC17705 ), j, mainaddr );

                        if (pmap->cachemap[j] == CM_FETCHED)
                            // "TXF: %s%02X: %sWe fetched: +"
                            dump_cache( regs, TXF_DUMP_PFX( HHC17706 ), j, altaddr );
                        else
                            // "TXF: %s%02X: %sWe stored:  +"
                            dump_cache( regs, TXF_DUMP_PFX( HHC17707 ), j, altaddr );
                    }
                }
            }
        }
    }

    /*---------------------------------------------*/
    /*  Clean up the transaction flags             */
    /*---------------------------------------------*/

    regs->txf_NTSTG     = false;
    regs->txf_contran   = false;
    regs->txf_tac       = 0;
    regs->txf_abortctr  = 0;
    regs->txf_instctr   = 0;
    regs->txf_pgcnt     = 0;
    regs->txf_conflict  = 0;
    regs->txf_piid      = 0;

    /*---------------------------------------------*/
    /*  Reset transaction nesting depth            */
    /*---------------------------------------------*/
    OBTAIN_TXFLOCK( regs );
    {
        regs->txf_tnd = 0;
    }
    RELEASE_TXFLOCK( regs );

    /* Reset CONSTRAINED trans instruction fetch constraint */
    ARCH_DEP( reset_txf_aie )( regs );

    /*-----------------------------------------------------*/
    /*    Trace program interrupt BEFORE updating PSW      */
    /*-----------------------------------------------------*/
    /*  If the retry code is ABORT_RETRY_PGMCHK, then we   */
    /*  will eventually be calling the program_interrupt   */
    /*  function for PGM_TRANSACTION_CONSTRAINT_EXCEPTION  */
    /*  so we MUST trace the program interrupt here. We    */
    /*  CANNOT let the program_interrupt function do that  */
    /*  for us like it normally does since it would report */
    /*  the wrong PSW. Thus we MUST do it ourselves here.  */
    /*-----------------------------------------------------*/

    if (retry == ABORT_RETRY_PGMCHK)
    {
        int pcode, ilc;

        pcode = PGM_TRANSACTION_CONSTRAINT_EXCEPTION | PGM_TXF_EVENT;

        if (raw_retry == -ABORT_RETRY_PGMCHK) // (negative?)
        {
            /* Instruction dispatch program interrupt: since the
               instruction has not been decoded yet (and the PSW
               and 'ip' pointer bumped appropriately (instead,
               it is pointing directly AT the failing instruction
               and not past it like normal)), we must bump the ip
               past the instruction that program checked so that
               the 'trace_program_interrupt' function can then
               back it up to point to the correct instruction.
            */
            ilc = ILC( *regs->ip );
            regs->ip += ilc;
            {
                PTT_TXF( "TXF trpi+ilc", regs->ip, ilc, 0 );
                ARCH_DEP( trace_program_interrupt )( regs, pcode, ilc );
            }
            regs->ip -= ilc;
        }
        else /* Normal program interrupt after instruction decode */
        {
            /* Fix PSW and get instruction length (ilc) */
            ilc = ARCH_DEP( fix_program_interrupt_PSW )( regs );

            /* Trace program checks other than PER event */
            PTT_TXF( "TXF trpi", regs->ip, ilc, 0 );
            ARCH_DEP( trace_program_interrupt )( regs, pcode, ilc );
        }

        /* Save the program interrupt id */
        txf_piid   = pcode;
        txf_piid  |= (ilc << 16);
    }

    /*----------------------------------------------------*/
    /*  Set the current PSW to the Transaction Abort PSW  */
    /*----------------------------------------------------*/

    /* PROGRAMMING NOTE: it's CRITICAL to invalidate the aia BEFORE
       setting the current PSW to the Transaction Abort PSW, since
       INVALIDATE_AIA *might* update the PSW's instruction address
       to a value different from what txf_tapsw says it should be!
    */
    INVALIDATE_AIA( regs ); // (do *before* PSW memcpy!)
    memcpy( &regs->psw, &regs->txf_tapsw, sizeof( PSW ));
    regs->ip  = regs->txf_ip;
    regs->aip = regs->txf_aip;
    regs->aiv = regs->txf_aiv;

    /*---------------------------------------------*/
    /*     Set the condition code in the PSW       */
    /*---------------------------------------------*/

    if (txf_tac && txf_tac < (int) _countof( tac2cc ))
                                    regs->psw.cc = tac2cc[ txf_tac ];
    else if (txf_tac == TAC_MISC)   regs->psw.cc = TXF_CC_TRANSIENT;
    else if (txf_tac >= TAC_TABORT) regs->psw.cc = TXF_CC_INDETERMINATE;

    /*---------------------------------------------*/
    /*      Put data/vector exception code         */
    /*    into byte 2 of FPCR if appropriate.      */
    /*---------------------------------------------*/

    if (1
        && (txf_piid & 0xFF) == PGM_DATA_EXCEPTION
        && regs->txf_ctlflag & TXF_CTL_FLOAT
        && regs->CR(0) & CR0_AFP
    )
    {
        regs->fpc &= ~((U32) 0xFF              << 8);
        regs->fpc |=  ((U32) regs->txf_dxc_vxc << 8);
    }

    /*------------------------------------------------*/
    /*               Populate TDBs                    */
    /*------------------------------------------------*/
    /*  Populate the TDBs. For program-interrupts,    */
    /*  the TDB is at fixed storage location 0x1800   */
    /*  in low core.  Additionally, the address of    */
    /*  a second TDB may be specified by the TBEGIN   */
    /*  instruction itself. (The operand-1 address    */
    /*  for TBEGINC is ignored.)                      */
    /*------------------------------------------------*/

    /* CONSTRAINED transaction or Program Interrupt?  */
    if (0
        || txf_contran          /* CONSTRAINED trans? */
        || txf_tac == TAC_UPGM  /* Unfiltered PGM?    */
        || txf_tac == TAC_FPGM  /* Filtered   PGM?    */
    )
    {
        RADR pi_tdba = 0x1800;
        pi_tdba = APPLY_PREFIXING( pi_tdba, regs->PX );
        pi_tdb = (TDB*)(regs->mainstor + pi_tdba);
    }

    /* TBEGIN-specified TDB? */
    if (!txf_contran && regs->txf_tdba_b1)
    {
        RADR  real_tdb;
        int   stid;
        int   xcode;

        /* Convert TDB address to absolute mainstor address */
        if ((xcode = ARCH_DEP( virt_to_real )( &real_tdb, &stid,
            regs->txf_tdba, regs->txf_tdba_b1, regs, ACCTYPE_WRITE )) == 0)
        {
            tb_tdb = (TDB*)(regs->mainstor + APPLY_PREFIXING( real_tdb, regs->PX ));
        }
        else
        {
            PTT_TXF( "*TXF vrfail", xcode, regs->txf_tdba, regs->txf_tdba_b1 );

            if (TXF_TRACING())
            {
                // "TXF: %s%02X: %sTranslation exception %4.4hX (%s) on TBEGIN tdba 0x%16.16"PRIx64
                WRMSG( HHC17713, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                    (U16)xcode, PIC2Name( xcode ), regs->txf_tdba );
            }

            /* Transaction Abort processing, Note 4: "If the TDBA is
               valid, but the block has become inaccessible subsequent
               to the execution of the outermost TBEGIN instruction,
               the TDB is not accessed and condition code 1 applies."
            */
            regs->psw.cc = TXF_CC_INDETERMINATE;
        }
    }

    /*--------------------------------------------------*/
    /*             Populate our internal TDB            */
    /*--------------------------------------------------*/
    {
        regs->txf_tdb.tdb_format = 1;
        regs->txf_tdb.tdb_eaid   = regs->excarid;
        regs->txf_tdb.tdb_flags  = (txf_contran ? TDB_CTI : 0x00);

        if (0
            || txf_tac == TAC_FETCH_CNF
            || txf_tac == TAC_STORE_CNF
        )
            regs->txf_tdb.tdb_flags |= (txf_conflict ? TDB_CTV : 0x00);

        if (0
            || (txf_piid & 0xFF) == PGM_DATA_EXCEPTION
            || (txf_piid & 0xFF) == PGM_VECTOR_PROCESSING_EXCEPTION
        )
            regs->txf_tdb.tdb_dxc = regs->txf_dxc_vxc;

        STORE_HW( regs->txf_tdb.tdb_tnd,      (U16) txf_tnd      );
        STORE_DW( regs->txf_tdb.tdb_tac,      (U64) txf_tac      );
        STORE_DW( regs->txf_tdb.tdb_teid,     (U64) regs->TEA    );
        STORE_FW( regs->txf_tdb.tdb_piid,     (U32) txf_piid     );
        STORE_DW( regs->txf_tdb.tdb_atia,     (U64) txf_atia     );
        STORE_DW( regs->txf_tdb.tdb_conflict, (U64) txf_conflict );
        STORE_DW( regs->txf_tdb.tdb_bea,      (U64) txf_bea      );

        for (i=0; i < 16; i++)
            STORE_DW( regs->txf_tdb.tdb_gpr[i], regs->GR_G( i ));
    }

    /*----------------------------------------------------*/
    /*  Copy internal TDB to its proper storage location  */
    /*----------------------------------------------------*/

    if (pi_tdb)
    {
        memcpy( pi_tdb, &regs->txf_tdb, sizeof( TDB ));
        ARCH_DEP( or_storage_key )( MAIN_TO_ABS( pi_tdb ), (STORKEY_REF | STORKEY_CHANGE) );
    }

    if (tb_tdb)
    {
        memcpy( tb_tdb, &regs->txf_tdb, sizeof( TDB ));
        ARCH_DEP( or_storage_key )( MAIN_TO_ABS( tb_tdb ), (STORKEY_REF | STORKEY_CHANGE) );
    }

    /* Trace TDB if requested */
    if (TXF_TRACE_TDB( regs, txf_contran ))
        dump_tdb( regs, &regs->txf_tdb );

    /*----------------------------------------------------*/
    /*         Restore the requested registers            */
    /*----------------------------------------------------*/
    for (i=0; i < 16; i += 2, txf_gprmask <<= 1)
    {
        if (txf_gprmask & 0x80)
        {
            regs->gr[ i+0 ] = regs->txf_savedgr[ i+0 ];
            regs->gr[ i+1 ] = regs->txf_savedgr[ i+1 ];
        }
    }

    /*----------------------------------------------------*/
    /*                 Release INTLOCK                    */
    /*----------------------------------------------------*/
    /*  Release the interrupt lock if we obtained it, or  */
    /*  if we won't be directly returning. That is if we  */
    /*  will be jumping to progjmp via program_interrupt  */
    /*  or via SIE_NO_INTERCEPT, then we need to release  */
    /*  the interrupt lock beforehand. Otherwise if we    */
    /*  already held it when we were called, then we need */
    /*  to return to the caller with it still held.       */
    /*----------------------------------------------------*/

    if (!had_INTLOCK || retry != ABORT_RETRY_RETURN)
    {
        PERFORM_SERIALIZATION( regs );
        RELEASE_INTLOCK( regs );
    }

    /*----------------------------------------------------*/
    /*       RETURN TO CALLER OR JUMP AS REQUESTED        */
    /*----------------------------------------------------*/

    if (retry == ABORT_RETRY_RETURN)
    {
        /* Caller requested that we return back to
           them so they can decide what to do next.
           This 'retry' code is typically used by
           the various interrupt handlers (external,
           machine check, restart, program and I/O).
        */
        PTT_TXF( "*TXF abrtret", 0, txf_contran, txf_tnd );
        return; // (caller decides what to do next)
    }

    if (retry == ABORT_RETRY_CC)
    {
        /* Transaction failures have their PSW set to the
           Transaction Abort PSW, which for unconstrained
           transactions points to the instruction immediately
           following the TBEGIN instruction (so that the
           condition code can then be used to decide whether
           to bother retrying the transaction or not), and
           for constrained transactions points to the TBEGINC
           instruction itself (so that the transaction can
           then be unconditionally retried).

           Either way, we simply jump directly back to the
           'run_cpu' loop to redispatch this CPU, thereby
           causing it to continue executing instructions at
           where the Transaction Abort PSW said it should.
        */
        PTT_TXF( "*TXF abrtjmp", 0, txf_contran, txf_tnd );
        longjmp( regs->progjmp, SIE_NO_INTERCEPT );
        UNREACHABLE_CODE( return );
    }

    /*   (a transaction constraint has been violated...)   */

    ASSERT( retry == ABORT_RETRY_PGMCHK );  // (sanity check)

    /* The caller has requested via retry == ABORT_RETRY_PGMCHK
       that a program interrupt should be thrown for this aborted
       transaction. This of course implies an unfilterable program
       interrupt since transaction constraints cannot be ignored
       nor filtered. A program interrupt WILL occur. If it was a
       CONSTRAINED transaction, it will be retried at the TBEGINC
       instruction. For unconstrained transactions it will restart
       at the instruction immediately following TBEGIN instruction
       with a condition code of 3.
    */
    PTT_TXF( "*TXF abrtpgm", 0, txf_contran, txf_tnd );
    ARCH_DEP( program_interrupt )( regs,
        PGM_TXF_EVENT | PGM_TRANSACTION_CONSTRAINT_EXCEPTION );
    UNREACHABLE_CODE( return );

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

} /* end ARCH_DEP( abort_transaction ) */

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
/*-------------------------------------------------------------------*/
/*                  Program Interrupt Filtering                      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Determine the proper CC (Condition Code) based on the Exception   */
/* Condition (pgm interrupt code) and Transactional-Execution class, */
/* which depends on whether interrupt can/should be filtered or not. */
/* Refer to Figure 5-16 on page 5-104 (and 5-14 on page 5-102) of    */
/* manual: SA22-7832-12 "z/Architecture Principles of Operation".    */
/*                                                                   */
/* If the program interrupt can be filtered, the abort_transaction   */
/* function is called with retry = ABORT_RETRY_CC, which causes it   */
/* to jump back to the CPU instruction processing loop to continue   */
/* on with the next instruction (which abort_transaction should've   */
/* set, via Transaction Abort PSW, to the instruction immediately    */
/* following the TBEGIN instruction, which should normally be a      */
/* jump on condition instruction).                                   */
/*                                                                   */
/* Otherwise if the program interrupt CANNOT be filtered, we abort   */
/* the transaction with an unfiltered program interrupt abort code   */
/* (TAC_UPGM) and then return back to the caller so that they can    */
/* continue with normal program interrupt processing.                */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT void ARCH_DEP( txf_do_pi_filtering )( REGS* regs, int pcode )
{
bool    filt;                   /* true == filter the interrupt      */
int     txclass;                /* Transactional Execution Class     */
int     fcc, ucc;               /* Filtered/Unfiltered conditon code */

    PTT_TXF( "TXF filt?", pcode, regs->txf_contran, regs->txf_tnd );

    /* We shouldn't even be called if no transaction is active */
    if (!regs->txf_tnd)
        CRASH();

    switch (pcode & 0xFF)  // (interrupt code)
    {
    case PGM_OPERATION_EXCEPTION:
    case PGM_PRIVILEGED_OPERATION_EXCEPTION:
    case PGM_EXECUTE_EXCEPTION:

        txclass = 1;        /* Class 1 can't be filtered */
        filt = false;
        ucc = TXF_CC_PERSISTENT;
        break;

    case PGM_SPECIFICATION_EXCEPTION:

        txclass = 3;
        filt = true;
        ucc = TXF_CC_TRANSIENT, fcc = TXF_CC_PERSISTENT;
        break;

    case PGM_PROTECTION_EXCEPTION:
    case PGM_ADDRESSING_EXCEPTION:
    case PGM_SEGMENT_TRANSLATION_EXCEPTION:
    case PGM_PAGE_TRANSLATION_EXCEPTION:
    case PGM_ASCE_TYPE_EXCEPTION:
    case PGM_REGION_FIRST_TRANSLATION_EXCEPTION:
    case PGM_REGION_SECOND_TRANSLATION_EXCEPTION:
    case PGM_REGION_THIRD_TRANSLATION_EXCEPTION:

        /* Did interrupt occur during instruction fetch? */
        if (regs->txf_lastarn == USE_INST_SPACE)
        {
            txclass = 1;        /* Class 1 can't be filtered */
            filt = false;
        }
        else
        {
            txclass = 2;
            filt = true;
        }
        ucc = TXF_CC_TRANSIENT, fcc = TXF_CC_PERSISTENT;
        break;

    case PGM_DATA_EXCEPTION:

        /* Check Data Exception Code (DXC) */
        switch (regs->dxc)
        {
        case DXC_AFP_REGISTER:
        case DXC_BFP_INSTRUCTION:
        case DXC_DFP_INSTRUCTION:
        case DXC_VECTOR_INSTRUCTION:

            txclass = 1;        /* Class 1 can't be filtered */
            filt = false;
            ucc = TXF_CC_TRANSIENT;
            break;

        default:

            txclass = 3;
            filt = true;
            ucc = TXF_CC_TRANSIENT, fcc = TXF_CC_PERSISTENT;
            break;

        } /* end switch (regs->dxc) */
        break;

    case PGM_FIXED_POINT_OVERFLOW_EXCEPTION:
    case PGM_FIXED_POINT_DIVIDE_EXCEPTION:
    case PGM_DECIMAL_OVERFLOW_EXCEPTION:
    case PGM_DECIMAL_DIVIDE_EXCEPTION:
    case PGM_EXPONENT_OVERFLOW_EXCEPTION:
    case PGM_EXPONENT_UNDERFLOW_EXCEPTION:
    case PGM_SIGNIFICANCE_EXCEPTION:
    case PGM_FLOATING_POINT_DIVIDE_EXCEPTION:
    case PGM_VECTOR_PROCESSING_EXCEPTION:
    case PGM_SQUARE_ROOT_EXCEPTION:

        txclass = 3;
        filt = true;
        ucc = TXF_CC_TRANSIENT, fcc = TXF_CC_PERSISTENT;
        break;

    case PGM_TRANSLATION_SPECIFICATION_EXCEPTION:
    case PGM_SPECIAL_OPERATION_EXCEPTION:
    case PGM_TRANSACTION_CONSTRAINT_EXCEPTION:

        txclass = 1;        /* Class 1 can't be filtered */
        filt = false;
        ucc = TXF_CC_PERSISTENT;
        break;

    case PGM_ALET_SPECIFICATION_EXCEPTION:
    case PGM_ALEN_TRANSLATION_EXCEPTION:
    case PGM_ALE_SEQUENCE_EXCEPTION:
    case PGM_ASTE_VALIDITY_EXCEPTION:
    case PGM_ASTE_SEQUENCE_EXCEPTION:
    case PGM_EXTENDED_AUTHORITY_EXCEPTION:

        txclass = 2;
        filt = true;
        ucc = TXF_CC_TRANSIENT, fcc = TXF_CC_PERSISTENT;
        break;

    default:

        txclass = 0;        /* Class 0 can't be filtered */
        filt = false;
        ucc = TXF_CC_SUCCESS;
        break;

    } /* end switch (pcode & 0xFF) */

    /* CONSTRAINED transactions cannot be filtered */
    if (regs->txf_contran)
        filt = false;

    /*---------------------------------------*/
    /*  Can interrupt POSSIBLY be filtered?  */
    /*---------------------------------------*/

    if (filt)
    {
        /* Is Program-Interruption-Filtering Overide enabled? */
        if (regs->CR(0) & CR0_PIFO)
            filt = false; /* Then interrupt cannot be filtered */
        else
        {
            /* Check PIFC (see Fig. 5-15 on page 5-104) */
            switch (regs->txf_pifc)
            {
            case TXF_PIFC_NONE:

                filt = false;
                break;

            case TXF_PIFC_LIMITED:

                filt = (txclass >= 3) ? true : false;
                break;

            case TXF_PIFC_MODERATE:
            case TXF_PIFC_RESERVED:
            default:

                filt = (txclass >= 2) ? true : false;
                break;
            }
        }
    }

    /*-------------------------------------------*/
    /*   Can interrupt ABSOLUTELY be filtered?   */
    /*-------------------------------------------*/

    if (filt)  /* NOW we can rely this flag */
    {
        /*--------------------------------------*/
        /* TAC_FPGM: filtered Program Interrupt */
        /*--------------------------------------*/

        regs->psw.cc = fcc;
        PTT_TXF( "TXF filt!", pcode, regs->txf_contran, regs->txf_tnd );
        regs->txf_why |= TXF_WHY_FILT_INT;
        ABORT_TRANS( regs, ABORT_RETRY_CC, TAC_FPGM );
        UNREACHABLE_CODE( return );
    }

    /*---------------------------------------------*/
    /*  TAC_UPGM: unfilterable Program Interrupt   */
    /*---------------------------------------------*/

    /* Abort the transaction and return to the caller */
    regs->psw.cc = ucc;
    PTT_TXF( "TXF unfilt!", pcode, regs->txf_contran, regs->txf_tnd );
    regs->txf_why |= TXF_WHY_UNFILT_INT;
    ABORT_TRANS( regs, ABORT_RETRY_RETURN, TAC_UPGM );

} /* end txf_do_pi_filtering */

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

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

    PTT_TXF( "TXF alloc", 0, 0, 0 );

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
/*         Free Transactional-Execution Facility Pages Map           */
/*-------------------------------------------------------------------*/
void free_txfmap( REGS* regs )
{
int        i;
TPAGEMAP*  pmap = regs->txf_pagesmap;

    PTT_TXF( "TXF free", 0, 0, 0 );

    /* LOGIC ERROR if CPU still executing a transaction */
    if (!sysblk.shutdown && regs->txf_tnd)
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
/*             Delay-Abort all active transactions                   */
/*-------------------------------------------------------------------*/
void txf_abort_all( U16 cpuad, int why, const char* location )
{
    int    cpu;
    REGS*  regs;

    why |= TXF_WHY_DELAYED_ABORT;

    for (cpu=0, regs = sysblk.regs[ 0 ];
        cpu < sysblk.maxcpu; regs = sysblk.regs[ ++cpu ])
    {
        /* Skip ourselves or any CPU that isn't online */
        if (0
            || !IS_CPU_ONLINE( cpu )
            || regs->cpuad == cpuad
        )
            continue;

        /* If this CPU is executing a transaction, then force it
           to eventually fail by setting a transation abort code.
        */
        OBTAIN_TXFLOCK( regs );
        {
            if (1
                &&  regs->txf_tnd
                && !regs->txf_tac
            )
            {
                regs->txf_tac   =  TAC_MISC;
                regs->txf_why  |=  why;
                regs->txf_who   =  cpuad;
                regs->txf_loc   =  TRIMLOC( location );

                PTT_TXF( "*TXF h delay", regs->cpuad, regs->txf_contran, regs->txf_tnd );
            }

            /* (check guestregs too just to be sure) */

            if (1
                &&  GUESTREGS
                &&  GUESTREGS->txf_tnd
                && !GUESTREGS->txf_tac
            )
            {
                GUESTREGS->txf_tac   =  TAC_MISC;
                GUESTREGS->txf_why  |=  why;
                GUESTREGS->txf_who   =  cpuad;
                GUESTREGS->txf_loc   =  TRIMLOC( location );

                PTT_TXF( "*TXF g delay", GUESTREGS->cpuad, GUESTREGS->txf_contran, GUESTREGS->txf_tnd );
            }
        }
        RELEASE_TXFLOCK( regs );
    }
}

//---------------------------------------------------------------------
//                   Keep Otimization Enabled
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
/*              Transactional Address Translation                    */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  This function is called after every 'maddr_l' dynamic address    */
/*  translation call for any CPU in transactional-execution mode     */
/*  to "translate" the passed address to an alternate address in     */
/*  a different page. Both vaddr and regs must be valid.             */
/*                                                                   */
/*  The real storage address is mapped to an alternate address and   */
/*  the contents of the real page are saved.  The cache line being   */
/*  accessed within the page is marked as having been accessed.      */
/*                                                                   */
/*  The first time that a cache line is accessed, it is captured     */
/*  from the real page.  When a cache line or page is captured two   */
/*  copies are made:  one copy is presented to the caller and one    */
/*  is a saved copy to be used at TEND commit time to see if that    */
/*  cache line was changed by anyone.                                */
/*                                                                   */
/*  Input:                                                           */
/*       vaddr    Logical address as passed to maddr_l               */
/*       len      Length of data access for cache line purposes      */
/*       arn      Access register number as passed to maddr_l        */
/*       regs     Pointer to the CPU register context                */
/*       acctype  Type of access: READ, WRITE, INSTFETCH, etc.       */
/*       maddr    Guest absolute storage MAINADDR address output     */
/*                from 'maddr_l' address translation call            */
/*                                                                   */
/* Returns:                                                          */
/*      An alternate address from the transacting CPU's transaction  */
/*      page map (TPAGEMAP).                                         */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT BYTE* txf_maddr_l( const U64  vaddr,   const size_t  len,
                              const int  arn,     REGS*         regs,
                              const int  acctype, BYTE*         maddr )
{
    BYTE *pageaddr, *pageaddrc;
    BYTE *savepage, *savepagec;
    BYTE *altpage,  *altpagec;

    U64  addrwork;              /* maddr converted to U64            */
    U64  addrpage;              /* Corresponding page address        */

    int  pageoffs;              /* addrwork offset into addrpage     */
    int  cacheidx;              /* Corresponding cache line          */
    int  cacheidxe;             /* Corresponding ending cache line   */
    int  i;                     /* Work variable                     */
    int  txf_acctype;           /* ACC_READ or ACC_WRITE             */

    BYTE cmtype;                /* Cache Map access type             */

    TPAGEMAP*  pmap;            /* Pointer to Transaction Page Map   */

    ASSERT( regs && regs->txf_tnd );      /* (sanity check) */

    /* Check if our transaction has already been aborted */
    if (regs->txf_tac)
    {
        PTT_TXF( "*TXF mad TAC", regs->txf_tac, regs->txf_contran, regs->txf_tnd );
        if (!(regs->txf_why & TXF_WHY_DELAYED_ABORT))
        {
            regs->txf_why  |=  TXF_WHY_DELAYED_ABORT;
            regs->txf_who   =  regs->cpuad;
            regs->txf_loc   =  TRIMLOC( PTT_LOC );
        }
        ABORT_TRANS( regs, ABORT_RETRY_CC, regs->txf_tac );
        UNREACHABLE_CODE( return maddr );
    }

    /* Normalize access type for TXF usage */
    txf_acctype = TXF_ACCTYPE( acctype );

    /*  Constrained transactions constraint #4: "The transaction's
        storage operands access no more than four octowords. Note:
        LOAD ON CONDITION and STORE ON CONDITION are considered to
        reference storage regardless of the condition code."
        (SA22-7832-12, page 5-109)
    */
    if (regs->txf_contran && len > (4 * ZOCTOWORD_SIZE))
    {
        int txf_tac = TXF_IS_FETCH_ACCTYPE() ? TAC_FETCH_OVF
                                             : TAC_STORE_OVF;
        regs->txf_why |= TXF_WHY_CONSTRAINT_4;
        PTT_TXF( "*TXF mad len", txf_tac, regs->txf_contran, regs->txf_tnd );
        ABORT_TRANS( regs, ABORT_RETRY_CC, txf_tac );
        UNREACHABLE_CODE( return maddr );
    }

    /* Save last translation arn */
    regs->txf_lastarn = arn;

    /* Calculate range of cache lines for this storage access */

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

        if (cacheidxe > (ZCACHE_LINE_PAGE - 1))
            cacheidxe = (ZCACHE_LINE_PAGE - 1);
    }

    /* Check if we have already captured this page and if not,
       capture it and save a copy.  The copy is used at commit
       time to determine if any unexpected changes were made.
    */
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
            int txf_tac = TXF_IS_FETCH_ACCTYPE() ?
                TAC_FETCH_OVF : TAC_STORE_OVF;
            regs->txf_why |= TXF_WHY_MAX_PAGES;

            PTT_TXF( "*TXF mad max", txf_tac, regs->txf_contran, regs->txf_tnd );
            ABORT_TRANS( regs, ABORT_RETRY_CC, txf_tac );
            UNREACHABLE_CODE( return maddr );
        }

        pageaddr = (BYTE*) addrpage;
        pmap     = &regs->txf_pagesmap[ regs->txf_pgcnt ];
        altpage  = pmap->altpageaddr;
        savepage = altpage + ZPAGEFRAME_PAGESIZE;

        /* Capture a copy of this page */
        memcpy( altpage,  pageaddr, ZPAGEFRAME_PAGESIZE );
        memcpy( savepage, altpage,  ZPAGEFRAME_PAGESIZE );

        /* Finish mapping this page */
        pmap->mainpageaddr = (BYTE*) addrpage;
        pmap->virtpageaddr = vaddr & ZPAGEFRAME_PAGEMASK;
        regs->txf_pgcnt++;
    }

    /* Calculate alternate address and cache map access type */
    maddr = altpage + pageoffs;
    cmtype = TXF_IS_FETCH_ACCTYPE() ? CM_FETCHED : CM_STORED;

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

            memcpy( altpagec,  pageaddrc, ZCACHE_LINE_SIZE );
            memcpy( savepagec, altpagec,  ZCACHE_LINE_SIZE );

            /* Remember how we accessed this cache line */
            pmap->cachemap[ cacheidx ] = cmtype;
            break;

        case CM_FETCHED:

            /* Remember how we accessed this cache line */
            pmap->cachemap[ cacheidx ] = cmtype;
            break;

        case CM_STORED:

            /* Cache lines marked CM_STORED must stay that way */
            break;

        } /* switch (pmap->cachemap[cacheidx]) */

    } /* for (; cacheidx <= cacheidxe; cacheidx++) */

    /* Done. Return alternate address */
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
    //          Short name          Long name

    /*   0 */   "TAC 0",            "(undefined Abort Code)",
    /*   1 */   "TAC 1",            "(undefined Abort Code)",
    /*   2 */   "TAC_EXT",          "External interruption",
    /*   3 */   "TAC 3",            "(undefined Abort Code)",
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
    /*  17 */   "TAC 17",           "(undefined Abort Code)",
    /*  18 */   "TAC 18",           "(undefined Abort Code)",
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
DLL_EXPORT const char* txf_why_str( char* buffer, int buffsize, int why )
{
    #define TXF_WHY_FORMAT( _why ) ((why & _why) ? " " #_why : "")

    //                           1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
    snprintf( buffer, buffsize, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"

        , TXF_WHY_FORMAT( TXF_WHY_INSTRADDR                ) // 1
        , TXF_WHY_FORMAT( TXF_WHY_INSTRCOUNT               ) // 2
        , TXF_WHY_FORMAT( TXF_WHY_RAND_ABORT               ) // 3
        , TXF_WHY_FORMAT( TXF_WHY_CSP_INSTR                ) // 4
        , TXF_WHY_FORMAT( TXF_WHY_CSPG_INSTR               ) // 5
        , TXF_WHY_FORMAT( TXF_WHY_SIE_EXIT                 ) // 6
        , TXF_WHY_FORMAT( TXF_WHY_CONFLICT                 ) // 7
        , TXF_WHY_FORMAT( TXF_WHY_MAX_PAGES                ) // 8
        , TXF_WHY_FORMAT( TXF_WHY_EXT_INT                  ) // 9
        , TXF_WHY_FORMAT( TXF_WHY_UNFILT_INT               ) // 10
        , TXF_WHY_FORMAT( TXF_WHY_FILT_INT                 ) // 11
        , TXF_WHY_FORMAT( TXF_WHY_RESTART_INT              ) // 12
        , TXF_WHY_FORMAT( TXF_WHY_IO_INT                   ) // 13
        , TXF_WHY_FORMAT( TXF_WHY_MCK_INT                  ) // 14
        , TXF_WHY_FORMAT( TXF_WHY_DELAYED_ABORT            ) // 15
        , TXF_WHY_FORMAT( TXF_WHY_TABORT_INSTR             ) // 16
        , TXF_WHY_FORMAT( TXF_WHY_CONTRAN_INSTR            ) // 17
        , TXF_WHY_FORMAT( TXF_WHY_CONTRAN_BRANCH           ) // 18
        , TXF_WHY_FORMAT( TXF_WHY_CONTRAN_RELATIVE_BRANCH  ) // 19
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_INSTR               ) // 20
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_FLOAT_INSTR         ) // 21
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_ACCESS_INSTR        ) // 22
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_NONRELATIVE_BRANCH  ) // 23
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_BRANCH_SET_MODE     ) // 24
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_SET_ADDRESSING_MODE ) // 25
        , TXF_WHY_FORMAT( TXF_WHY_TRAN_MISC_INSTR          ) // 26
        , TXF_WHY_FORMAT( TXF_WHY_NESTING                  ) // 27
        , TXF_WHY_FORMAT( TXF_WHY_STORKEY                  ) // 28
        , TXF_WHY_FORMAT( TXF_WHY_IPTE_INSTR               ) // 29
        , TXF_WHY_FORMAT( TXF_WHY_IDTE_INSTR               ) // 30
        , TXF_WHY_FORMAT( TXF_WHY_CONSTRAINT_4             ) // 31
        , ""                                                 // 32
    );
    return buffer;
}

/*-------------------------------------------------------------------*/
/*               DEBUG:  Hex dump a cache line                       */
/*-------------------------------------------------------------------*/
void dump_cache( REGS* regs, const char* pfxfmt, int linenum , const BYTE* line)
{
    char*  /* (work) */  dmp  = NULL;
    const char*          dat  = (const char*) line;
    static const size_t  skp  = 0;
    static const size_t  amt  = ZCACHE_LINE_SIZE;
    U64 /* (cosmetic) */ adr  = linenum << ZCACHE_LINE_SHIFT;
    static const size_t  bpg  = 4; // bytes per formatted group
    static const size_t  gpl  = 4; // formatted groups per line
    char dump_pfx[64]         = {0};

    MSGBUF(     dump_pfx, pfxfmt, TXF_CPUAD( regs ), TXF_QSIE( regs ));
    RTRIM(      dump_pfx );
    hexdumpe16( dump_pfx, &dmp, dat, skp, amt, adr, bpg, gpl );

    if (dmp)
    {
        LOGMSG( "%s", dmp );
        free( dmp );
    }
    else
    {
        // "TXF: %s%02X: %serror in function %s: %s"
        WRMSG( HHC17708, "E", TXF_CPUAD( regs ), TXF_QSIE( regs ),
            "dump_cache()", strerror( errno ));
    }
}

/*-------------------------------------------------------------------*/
/*                   DEBUG:  Hex dump TDB                            */
/*-------------------------------------------------------------------*/
void dump_tdb( REGS* regs, TDB* tdb )
{
    /* NOT verbose tracing -or- invalid TDB : do storage dump of TDB */
    if (!MLVL( VERBOSE ) || tdb->tdb_format != 1)
    {
        char*  /* (work) */  dmp  = NULL;
        const char*          dat  = (const char*) tdb;
        static const size_t  skp  = 0;
        static const size_t  amt  = sizeof( TDB );
        static const U64     adr  = 0; // (cosmetic)
        static const size_t  bpg  = 8; // bytes per formatted group
        static const size_t  gpl  = 2; // formatted groups per line
        char dump_pfx[64]         = {0};

        // "TXF: %s%02X: %s%s dump of TDB:"
        WRMSG( HHC17709, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), "Storage" );

        MSGBUF(     dump_pfx, MSG( HHC17710, "D", TXF_CPUAD( regs ), TXF_QSIE( regs )));
        RTRIM(      dump_pfx );
        hexdumpe16( dump_pfx, &dmp, dat, skp, amt, adr, bpg, gpl );

        if (dmp)
        {
            LOGMSG( "%s", dmp );
            free( dmp );
        }
        else
        {
            // "TXF: %s%02X: %serror in function %s: %s"
            WRMSG( HHC17708, "E", TXF_CPUAD( regs ), TXF_QSIE( regs ),
                "dump_tdb()", strerror( errno ));
        }
        return;
    }

    /* Otherwise if verbose messages wanted and TDB is valid,
       then do a formatted print of each individual TDB field.
    */
    if (tdb->tdb_format == 1 && MLVL( VERBOSE ))
    {
        char buf[128] = {0};
        char flgs[32] = {0};

        U16  tnd;
        U32  piid;
        U64  tac, tkn, atia, teid, bea;

        U64  gr[16];

        // "TXF: %s%02X: %s%s dump of TDB:"
        WRMSG( HHC17709, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), "Formatted" );

        // Fmt: 1, TND: 1, EAID: 00, DXC/VXC: 00, PIID: 00000000, Flags: (none)

        FETCH_HW( tnd,  tdb->tdb_tnd  );
        FETCH_FW( piid, tdb->tdb_piid );

        if (!tdb->tdb_flags) STRLCPY( flgs, "none" );
        else MSGBUF( flgs, "%s%s"
            , tdb->tdb_flags & TDB_CTV ? "TDB_CTV, " : ""
            , tdb->tdb_flags & TDB_CTI ? "TDB_CTI, " : ""
        );
        RTRIM(  flgs ); rtrim ( flgs, "," );

        MSGBUF( buf, "Fmt: %u, TND: %"PRIu16", EAID: %02X, DXC/VXC: %02X, PIID: %08"PRIX32", Flags: (%s)",
            tdb->tdb_format, tnd, tdb->tdb_eaid, tdb->tdb_dxc, piid, flgs );
        WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), buf );

        // TAC:   0x0000000000000008: TAC_STORE_OVF = Store overflow

        FETCH_DW( tac, tdb->tdb_tac );

        MSGBUF( buf, "TAC:   0x%16.16"PRIX64": %s = %s",
            tac, tac2short( tac ), tac2long( tac ) );
        WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), buf );

        // Token: 0x0000000000000000, ATIA:  0x00000000000024EA

        FETCH_DW( tkn,  tdb->tdb_conflict );
        FETCH_DW( atia, tdb->tdb_atia     );

        MSGBUF( buf, "Token: 0x%16.16"PRIX64", ATIA:  0x%16.16"PRIX64, tkn, atia );
        WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), buf );

        /* Display the instruction that the transaction aborted on */
        {
            REGS*  tregs = copy_regs( regs );
            int    xcode, stid;
            RADR   real_atia;

            /* Point the PSW to the aborted instruction */
            SET_PSW_IA_AND_MAYBE_IP( tregs, atia );

            /* Get absolute mainstor addr of that instruction */
            if ((xcode = ARCH_DEP( virt_to_real )( &real_atia,
                &stid, atia, 0, tregs, ACCTYPE_INSTFETCH )) == 0)
            {
                BYTE*  ip        = (tregs->mainstor + APPLY_PREFIXING( real_atia, tregs->PX ));
                bool   showregs  = sysblk.showregsnone;
                U16    code      = CODE_FROM_PIID( piid );
                int    ilc       = ILC_FROM_PIID(  piid );

                // (blank line)
                WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), "" );
                {
                    if (code)
                    {
                        // e.g. "Operation exception code 0201 ilc 2"
                        MSGBUF( buf, "%s code %4.4X ilc %d", PIC2Name( code ), code, ilc );
                        WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), buf );
                    }

                    sysblk.showregsnone = true;
                    {
                        int n = 0;
                        BYTE inst[6];

                        memcpy( inst, ip, sizeof( inst ));
                        ilc = ILC( inst[0] );

                                     n += idx_snprintf( n, buf, sizeof( buf ), "%16.16"PRIX64" ", atia );
                                     n += idx_snprintf( n, buf, sizeof( buf ), "INST=%2.2X%2.2X", inst[0], inst[1] );
                        if (ilc > 2){n += idx_snprintf( n, buf, sizeof( buf ), "%2.2X%2.2X",      inst[2], inst[3] );}
                        if (ilc > 4){n += idx_snprintf( n, buf, sizeof( buf ), "%2.2X%2.2X",      inst[4], inst[5] );}
                                     n += idx_snprintf( n, buf, sizeof( buf ), " %s", (ilc < 4) ? "        "
                                                                                    : (ilc < 6) ? "    "
                                                                                    :             "" );
                        n += PRINT_INST( regs->arch_mode, inst, buf + n );

                        // "AAAAAAAAAAAAAAAA INST=112233445566 XXXXX op1,op2                name"
                        WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), buf );
                    }
                    sysblk.showregsnone = showregs;
                }
                // (blank line)
                WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), "" );
            }
            free_aligned( tregs );
        }

        // TEID:  0x0000000000000000, BEA:   0x000000000000255C

        FETCH_DW( teid, tdb->tdb_teid );
        FETCH_DW( bea,  tdb->tdb_bea  );

        MSGBUF( buf, "TEID:  0x%16.16"PRIX64", BEA:   0x%16.16"PRIX64, teid, bea );
        WRMSG( HHC17721, "D", TXF_CPUAD( regs ), TXF_QSIE( regs ), buf );

        // GR (General Registers)

#define GRPAIR_FMT( r )                                 \
        do                                              \
        {                                               \
            FETCH_DW( gr[r+0], tdb->tdb_gpr[r+0] );     \
            FETCH_DW( gr[r+1], tdb->tdb_gpr[r+1] );     \
            MSGBUF( buf, "GR %02u: 0x%16.16"PRIX64      \
                       ", GR %02u: 0x%16.16"PRIX64,     \
                       r+0, gr[r+0], r+1, gr[r+1] );    \
            WRMSG( HHC17721, "D", TXF_CPUAD( regs ),    \
                TXF_QSIE( regs ), buf );                \
        }                                               \
        while (0)

        GRPAIR_FMT(  0 );
        GRPAIR_FMT(  2 );
        GRPAIR_FMT(  4 );
        GRPAIR_FMT(  6 );
        GRPAIR_FMT(  8 );
        GRPAIR_FMT( 10 );
        GRPAIR_FMT( 12 );
        GRPAIR_FMT( 14 );
    }
}

/*-------------------------------------------------------------------*/
/*                    TXF capable model numbers                      */
/*-------------------------------------------------------------------*/

struct TXFMODELS
{
    const U16    cpumodel;      // hex model number
    const char*  pszModel;      // (same things as char string)
    const char*  pszSymbol;     // DEFSYM symbol name
};
typedef struct TXFMODELS  TXFMODELS;

#define TXF_MODEL( model, name )    { 0x ## model, #model, #name }

static const TXFMODELS txf_models[] =
{
    // REF: https://www-01.ibm.com/servers/resourcelink/lib03060.nsf/pages/lsprITRzOSv2r3?OpenDocument#ibm-top

    TXF_MODEL( 1090, zPDT   ),
    TXF_MODEL( 2827, EC12   ),
    TXF_MODEL( 2828, BC12   ),
    TXF_MODEL( 2964, z13    ),
    TXF_MODEL( 2965, z13s   ),
    TXF_MODEL( 3906, z14    ),
    TXF_MODEL( 3907, z14ZR1 ),
    TXF_MODEL( 8561, z15    ),
    TXF_MODEL( 8562, z15T02 ),
    TXF_MODEL( 3931, z16    ),
};

/*-------------------------------------------------------------------*/
/* Boolean helper to return whether cpu model is TXF capable or not  */
/*-------------------------------------------------------------------*/
bool is_TXF_model( U16 cpumodel )
{
    size_t i;
    for (i=0; i < _countof( txf_models ); i++)
        if (cpumodel == txf_models[i].cpumodel)
            return true;
    return false;
}

/*-------------------------------------------------------------------*/
/* Helper function to define DEFSYM symbols for TXF models by name   */
/*-------------------------------------------------------------------*/
void defsym_TXF_models()
{
    size_t i;
    for (i=0; i < _countof( txf_models ); i++)
        // e.g. "CPUMODEL $(z13s)"  ==>  "CPUMODEL 2965"
        set_symbol( txf_models[i].pszSymbol, txf_models[i].pszModel );
}

/*-------------------------------------------------------------------*/
/* Helper function to issue HHC02385W CPU Model warning if needed    */
/*-------------------------------------------------------------------*/
void txf_model_warning( bool txf_enabled_or_enabling_txf )
{
    if (1
        && sysblk.arch_mode == ARCH_900_IDX
        && txf_enabled_or_enabling_txf
        && sysblk.config_processed
        && !is_TXF_model( sysblk.cpumodel )
        && MLVL( VERBOSE )
    )
    {
        // "CPUMODEL %04X does not technically support TXF"
        WRMSG( HHC02385, "W", sysblk.cpumodel );
    }
}

/*-------------------------------------------------------------------*/
/* Helper function to set a proper TXF timerint value                */
/*-------------------------------------------------------------------*/
void txf_set_timerint( bool txf_enabled_or_enabling_txf )
{
    if (0
        || !sysblk.config_processed
        || sysblk.arch_mode != ARCH_900_IDX
    )
        return;

    if (txf_enabled_or_enabling_txf)
    {
        if (sysblk.timerint >= MIN_TXF_TIMERINT)
        {
            /* Use the user's defined timerint value for TXF */
            sysblk.txf_timerint = sysblk.timerint;
        }
        else
        {
            // "TXF: TIMERINT %d is too small; using default of %d instead"
            WRMSG( HHC17736, "W", sysblk.timerint, DEF_TXF_TIMERINT );

            sysblk.txf_timerint = sysblk.timerint = DEF_TXF_TIMERINT;
        }

        /* Start the rubato_thread if it hasn't been started yet */
        obtain_lock( &sysblk.rublock );
        {
            if (!sysblk.rubtid)
            {
                int rc = create_thread( &sysblk.rubtid, DETACHED,
                     rubato_thread, NULL, RUBATO_THREAD_NAME );
                if (rc)
                    // "Error in function create_thread(): %s"
                    WRMSG( HHC00102, "E", strerror( rc ));
            }
        }
        release_lock( &sysblk.rublock );
    }
    else
    {
        /* Stop the rubato_thread if it's still running */
        obtain_lock( &sysblk.rublock );
        {
            /* Tell rubato_thread to please exit */
            sysblk.rubtid = 0;
        }
        release_lock( &sysblk.rublock );

        /* Reset the timerint value back to its original value */
        sysblk.timerint = sysblk.cfg_timerint;
    }
}

#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */

#endif /*!defined(_GEN_ARCH)*/
