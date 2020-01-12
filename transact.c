/* TRANSACT.C   (C) Copyright "Fish" (David B. Trout), 2017          */
/*      Defines Transactional Execution Facility instructions        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html)                      */
/*   as modifications to Hercules.                                   */

#include "hstdinc.h"

#define _TRANSACT_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

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

    CONTRAN_INSTR_CHECK( regs );

    /* Retrieve abort count */
    abort_count = regs->GR_L( r1 );

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

    if (_GEN_ARCH != 900)
      ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if ((regs->sysblk->facility_list[2][9] & 0x40) == 0x00)   /* not installed */
      ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if (!(regs->CR(0) & CR0_TXC))
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIAL_OPERATION_EXCEPTION);

    if (regs->contran)
      ARCH_DEP(abort_transaction)(regs, ABORT_RETRY_PGMCHK, ABORT_CODE_INSTR);

    regs->gr[r1].F.L.F = (U32)regs->tranlvl;
    regs->gr[r1].F.H.F = 0;

} /* end DEF_INST( extract_transaction_nesting_depth ) */


/*-------------------------------------------------------------------*/
/* B2F8 TEND - Transaction End  (CONSTRAINED or unconstrained)   [S] */
/*-------------------------------------------------------------------*/
DEF_INST( transaction_end )
{
int     i;
int     j;
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE     *altaddr;
BYTE     *saveaddr;
BYTE     *mainaddr = NULL;
TPAGEMAP *pmap;
REGS     *hregs = regs->hostregs;

    S( inst, regs, b2, effective_addr2 );

    if (_GEN_ARCH != 900)
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if ((regs->sysblk->facility_list[2][9] & 0x40) == 0x00)   /* not installed */
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);
    if (!(hregs->CR(0) & CR0_TXC))
        ARCH_DEP(program_interrupt)(hregs, PGM_SPECIAL_OPERATION_EXCEPTION);

    if (hregs->tranabortnum > 0)
        ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_PGMCHK, regs->rabortcode);

    if (hregs->tranlvl == 0)
        return;

    /*-----------------------------------------------------*/
    /*  Serialize the process by getting the interrupt     */
    /*  lock and synchronizing the CPUS.                   */
    /*-----------------------------------------------------*/

    OBTAIN_INTLOCK(hregs);
    SYNCHRONIZE_CPUS(hregs);

    hregs->tranlvl--;

    if (hregs->tranlvl > 0)
    {
      if (hregs->tranhigharchange > hregs->tranlvl)
        hregs->tranhigharchange--;

      if (hregs->tranhigharchange == hregs->tranlvl)
        hregs->tranctlflag |= TXF_CTL_AR;

      if (hregs->tranhighfloat > hregs->tranlvl)
        hregs->tranhighfloat--;

      if (hregs->tranhighfloat == hregs->tranlvl)
        hregs->tranctlflag |= TXF_CTL_FLOAT;

      hregs->tranprogfiltlvl = hregs->tranprogfilttab[hregs->tranlvl - 1];
      return;
    }

    if (sysblk.txf_transcpus > 0)
      sysblk.txf_transcpus--;

    /*--------------------------------------------------------*/
    /* If an abort code is already set, abort the transaction */
    /* and exit                                               */
    /*--------------------------------------------------------*/
    if (hregs->abortcode)
      ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_CC, hregs->abortcode);

    hregs->abortcode = 0;    /* clear the abort code */
    hregs->contran = 0;         /* clear the tran type code */
    hregs->tranabortnum = 0;

    /*--------------------------------------------------------*/
    /*  Scan the page map table.  There is one entry in the   */
    /*  page map table for each page referenced while in      */
    /*  transaction mode.  There is also a cache line map     */
    /*  that keeps track of what cache lines within the       */
    /*  page have been referenced and whether the reference   */
    /*  was a store or a fetch.  The current data in the      */
    /*  cache line is saved when the alternate entry is       */
    /*  created.   This saved data must match what is in main */
    /*  storage now, or the transation will be aborted with   */
    /*  a conflict, since that means that some other CPU or   */
    /*  the channel subsystem has stored into the cache line, */
    /*--------------------------------------------------------*/
    pmap = hregs->tpagemap;

    for (i = 0; i < hregs->tranpagenum; i++, pmap++)
    {
      for (j = 0; j < ZCACHE_LINE_PAGE; j++)
      {
        if (pmap->cachemap[j] > CM_CLEAN)
        {
          saveaddr = pmap->altpageaddr + (j << ZCACHE_LINE_SHIFT) + ZCACHE_PAGE_SIZE;
          mainaddr = pmap->mainpageaddr + (j << ZCACHE_LINE_SHIFT);

          if (memcmp(saveaddr, mainaddr, ZCACHE_LINE_SIZE) != 0)
          {
            if (pmap->cachemap[j] == CM_STORED)
              ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_CC, ABORT_CODE_STORE_CNF);
            else
              ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_CC, ABORT_CODE_FETCH_CNF);  
          }
        }
      }
    }

    /*---------------------------------------------------------*/
    /*  We have now validated all of the cache lines that  we  */
    /*  touched, and all other CPUs are dormant.   Now update  */
    /*  the real cache lines from the shadow cache lines.      */
    /*---------------------------------------------------------*/
    pmap = hregs->tpagemap;

    for (i = 0; i < hregs->tranpagenum; i++, pmap++)
    {
      for (j = 0; j < ZCACHE_LINE_PAGE; j++)
      {
        if (pmap->cachemap[j] > CM_CLEAN)
        {
          altaddr = pmap->altpageaddr + (j << ZCACHE_LINE_SHIFT);
          mainaddr = pmap->mainpageaddr + (j << ZCACHE_LINE_SHIFT);
          memcpy(mainaddr, altaddr, ZCACHE_LINE_SIZE);
        }
      }
    }

    /*----------------------------------------------------------*/
    /*  We are done.  Set the condition code, release the lock  */
    /*  and exit.                                               */
    /*----------------------------------------------------------*/
    hregs->psw.cc = 0;
    hregs->tranpagenum = 0;

    RELEASE_INTLOCK(hregs);

} /* end DEF_INST( transaction_end ) */


/*-------------------------------------------------------------------*/
/* B2FC TABORT - Transaction Abort                               [S] */
/*-------------------------------------------------------------------*/
DEF_INST( transaction_abort )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );

    if (_GEN_ARCH != 900)
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if ((regs->sysblk->facility_list[2][9] & 0x40) == 0x00)   /* not installed */
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    CONTRAN_INSTR_CHECK( regs );

    ARCH_DEP(abort_transaction)(regs, ABORT_RETRY_CC, (int)effective_addr2);

} /* end DEF_INST( transaction_abort ) */


/*-------------------------------------------------------------------*/
/* E325 NTSTG - Nontransactional Store                       [RXY-a] */
/*-------------------------------------------------------------------*/
DEF_INST( nontransactional_store )
{
int     r1;                             /* Value of R1 field         */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
QWORD   qwork;
REGS    *hregs = regs->hostregs;
NTRANTBL *nt;

    RXY( inst, regs, r1, b2, effective_addr2 );

    if (_GEN_ARCH != 900)
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if ((regs->sysblk->facility_list[2][9] & 0x40) == 0x00)   /* not installed */
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);


    // FIXME: BUG? checking 'hregs', but calling abort with 'regs'! Is that right?
    if (hregs->contran)
        ARCH_DEP(abort_transaction)(regs, ABORT_RETRY_PGMCHK, ABORT_CODE_INSTR);
    // CONTRAN_INSTR_CHECK( hregs );


    if (hregs->ntranstorectr >= MAX_TXF_NTSTG)
      ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_PGMCHK, ABORT_CODE_FETCH_OVF);

    /* Store regs in workarea */
    STORE_DW(qwork, regs->GR_G(r1));

    /* Store R1 to second operand
       Provide storage consistancy by means of obtaining
       the main storage access lock */
    ARCH_DEP(vstorec) (qwork, 8 - 1, effective_addr2, b2, regs);

    hregs->ntranstorectr++;

    nt = &hregs->ntrantbl[hregs->ntranstorectr - 1];
    nt->effective_addr = effective_addr2;
    nt->arn = b2;
    nt->skey = regs->psw.pkey;

    if (hregs->psw.amode64)
        nt->amodebits = AMODE_64;
    else
        if (hregs->psw.amode)
            nt->amodebits = AMODE_31;
        else
            nt->amodebits = AMODE_24;

    memcpy(&nt->ntran_data, &qwork, 8);

} /* end DEF_INST( nontransactional_store ) */


/* (forward reference) */
void ARCH_DEP(process_tbegin)(REGS *hregs, S16 i2, VADR effective_addr1);


/*-------------------------------------------------------------------*/
/* E560 TBEGIN - Transaction Begin    (unconstrained)          [SIL] */
/*-------------------------------------------------------------------*/
/* Begin an unconstrained transaction.  If already in transaction    */
/* mode, just increment the nesting level, otherwise start the       */
/* transaction.                                                      */
/*-------------------------------------------------------------------*/
DEF_INST(transaction_begin)
{
S16     i2;                             /* 16-bit immediate value    */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
REGS    *hregs = regs->hostregs;

    SIL(inst, regs, i2, b1, effective_addr1);

    if (_GEN_ARCH != 900)
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if ((regs->sysblk->facility_list[2][9] & 0x40) == 0x00)   /* not installed */
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if (!(hregs->CR(0) & CR0_TXC))
        ARCH_DEP(program_interrupt)(hregs, PGM_SPECIAL_OPERATION_EXCEPTION);

    CONTRAN_INSTR_CHECK( hregs );

    OBTAIN_INTLOCK(hregs);
    SYNCHRONIZE_CPUS(hregs);
    ARCH_DEP(process_tbegin)(hregs, i2, effective_addr1);

} /* end DEF_INST( transaction_begin ) */


#if defined( FEATURE_050_CONSTR_TRANSACT_FACILITY )
/*-------------------------------------------------------------------*/
/* E561 TBEGINC - Transaction Begin    (CONSTRAINED)           [SIL] */
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
U64     ioffset;
U64     instaddr;
U64     ctlmask1;
U16     rand1;
U16     rand2;
BYTE    racode[10] = {7, 8, 9, 10, 11, 13, 14, 15, 16, 255};
int      i;
TPAGEMAP *pmap;
REGS     *hregs = regs->hostregs;
union {
  S16   i2num;
  BYTE  i2byte[2];
} i2union;
BYTE    mask = 0x00;

    SIL(inst, regs, i2, b1, effective_addr1);

    if (_GEN_ARCH != 900)
        ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

    if ((regs->sysblk->facility_list[2][6] & 0x20) == 0x00)   /* constrained not installed */
        ARCH_DEP(program_interrupt)(hregs, PGM_OPERATION_EXCEPTION);

    if (!(hregs->CR(0) & CR0_TXC))
        ARCH_DEP(program_interrupt)(hregs, PGM_SPECIAL_OPERATION_EXCEPTION);

    // FIXME: why -1?
    if (hregs->tranlvl > (MAX_TXF_LEVEL-1))
        ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_PGMCHK, ABORT_CODE_NESTING);

    OBTAIN_INTLOCK(hregs);
    SYNCHRONIZE_CPUS(hregs);

    if (hregs->tranlvl > 0)     /* if alreadyvin transaction mode */
    {
        if (hregs->contran)   /* already in constrained mode     */
            ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_PGMCHK, ABORT_CODE_INSTR);  /* abort       */
        ARCH_DEP(process_tbegin)(hregs, i2, effective_addr1);
        return;
    }

    pmap = hregs->tpagemap;

    for (i = 0; i < MAX_TXF_PAGES; i++, pmap++)
    {
        pmap->mainpageaddr = NULL;
        memset(pmap->cachemap, CM_CLEAN, sizeof(pmap->cachemap));
    }

    hregs->psw.cc = ABORT_CC_SUCCESS;         /* clear the condition code   */
    i2union.i2num = i2;       /* save the flag halfword   */
    hregs->tranpagenum = 0;    /* clear number of mapped pages  */

    memcpy(&hregs->tranabortpsw, &hregs->psw, sizeof(PSW));   /* save the abort psw */
    memcpy(hregs->tranregs, hregs->gr, sizeof(hregs->tranregs));  /* save the registers  */

    hregs->contran = 1;          /* set transaction type to constrained  */
    hregs->abortcode = 0;        /* clear the abort code  */
    hregs->traninstctr = 0;      /* instruction counter    */
    hregs->tranabortnum = 0;        /* abort number          */
    hregs->conflictaddr = 0;     /* clear conflict address */
    hregs->ntranstorectr = 0;

    sysblk.txf_transcpus++;     /* update transaction mode ctr */
    hregs->tranlvl = 1;        /* set now in transaction mode  */

    /* following bytes are reversed because i2 is stored as little endian */
    hregs->tranregmask = i2union.i2byte[1];   /* save mask of regsiters to restore on abort */
    hregs->tranctlflag = i2union.i2byte[0];     /* save the control flags */
    instaddr = (U64)hregs->aiv.D;    /*  get the logical beginning of page address */
    ioffset = (U64)(hregs->ip - 6) & PAGEFRAME_BYTEMASK;   /* get the page offset */
    instaddr += ioffset;    /* get the instruction address */
    hregs->tranabortpsw.ia.D = instaddr;  /* set it in the abort PSW */

    ctlmask1 = hregs->CR(2) & CR2_TDC;  /* isolate last two bits */

    switch (ctlmask1)               /* see what to do */
    {
    case TDC_NORMAL:
    case TDC_RESERVED:
        break;

    case TDC_ALWAYS_RANDOM:
        rand1 = (U16)(rand() % _countof( racode ));
        hregs->tranabortnum = (U16)(rand() % MAX_TXF_CONTRAN_INSTR);
        hregs->rabortcode = racode[rand1];
        break;

    case TDC_MAYBE_RANDOM:
        rand1 = (U16)rand();             /* get a random number    */
        rand2 = (U16)rand();

        if (rand1 < rand2)        /*  abort randomly */
        {
            rand1 = (U16)(rand() % _countof( racode ));
            hregs->tranabortnum = (U16)(rand() % MAX_TXF_CONTRAN_INSTR);
            hregs->rabortcode = racode[rand1];
        }
    }
    RELEASE_INTLOCK(regs);

} /* end DEF_INST( transaction_begin_constrained ) */

#endif /* defined( FEATURE_050_CONSTR_TRANSACT_FACILITY ) */


/*-------------------------------------------------------------------*/
/*       process_tbegin  --  common TBEGIN/TBEGINC logic             */
/*-------------------------------------------------------------------*/
/*    The interrupt lock (INTLOCK) *MUST* be held upon entry!        */
/*-------------------------------------------------------------------*/
void ARCH_DEP(process_tbegin)(REGS *hregs, S16 i2, VADR effective_addr1)
{
U64     ioffset;
U64     instaddr;
U64     ctlmask1;
U16     rand1;
U16     rand2;
BYTE    arflag;
BYTE    flflag;
int     i;
U16     filtlvl;
TPAGEMAP *pmap;

    hregs->tranlvl++;         /* increase the nesting level */
    hregs->psw.cc = ABORT_CC_SUCCESS;        /* set cc=0 to indicate tranaction start */

    if (hregs->tranlvl == 1)  /* if starting the transaction  */
    {
        pmap = hregs->tpagemap;

        for (i = 0; i < MAX_TXF_PAGES; i++, pmap++)
        {
            pmap->mainpageaddr = NULL;
            memset(pmap->cachemap, CM_CLEAN, sizeof(pmap->cachemap));
        }

        hregs->tranpagenum = 0;   /* clear the mapped page counter     */
        hregs->contran = 0;       /* not a contrained transaction    */
        hregs->traninstctr = 0;   /* instruction counter    */
        hregs->tranabortnum = 0;  /* abort number          */
        hregs->abortcode = 0;     /* clear the abort code */
        hregs->conflictaddr = 0;    /* clear conflict address */
        hregs->ntranstorectr = 0;

        sysblk.txf_transcpus++;   /* increment transaction mode ctr */

        memcpy(&hregs->tranabortpsw, &hregs->psw, sizeof(PSW));   /* save the abort psw */
        memcpy(hregs->tranregs, hregs->gr, sizeof(hregs->tranregs)); /* save the registers */

        /* following bytes are reversed because i2 is stored as little endian */
        hregs->tranregmask = i2 >> 8;  /* save the register restore mask */
        hregs->tranctlflag = i2 & (TXF_CTL_AR | TXF_CTL_FLOAT);   /* save the control flags  */
        hregs->tranprogfiltlvl = i2 & TXF_CTL_PIFC;  /* program filter level */

        if (hregs->tranctlflag & TXF_CTL_AR)
            hregs->tranhigharchange = 1;
        else
            hregs->tranhigharchange = 0;

        if (hregs->tranctlflag & TXF_CTL_FLOAT)
            hregs->tranhighfloat = 1;
        else
            hregs->tranhighfloat = 0;

        if (PROBSTATE(&hregs->psw))    /* problem state */
            hregs->tdbaddr = (TDB *)effective_addr1;     /* save the tcb address */
        else
            if (hregs->CR(2) & CR2_TDS)       /* tdb only in problem state */
                hregs->tdbaddr = 0;
            else
                hregs->tdbaddr = (TDB *)effective_addr1;  /* set the tdb address */

        instaddr = (U64)hregs->aiv.D;       /* get logical start of page address  */
        ioffset = (U64)hregs->ip & PAGEFRAME_BYTEMASK;   /* get instruction offset */
        instaddr += ioffset;   /* compute logical instruction address */
        hregs->tranabortpsw.ia.D = instaddr;  /* save abort address */
        hregs->tranprogfiltlvl = i2 & TXF_CTL_PIFC;  /* set initial filter level */

        ctlmask1 = hregs->CR(2) & CR2_TDC;  /* isolate last two bits */

        switch (ctlmask1)               /* see what to do */
        {
        case TDC_NORMAL:
        case TDC_RESERVED:
            break;

        case TDC_ALWAYS_RANDOM:
            hregs->tranabortnum = (U16)(rand());
            hregs->rabortcode = 0;
            break;

        case TDC_MAYBE_RANDOM:
            rand1 = (U16)rand();             /* get a random number    */
            rand2 = (U16)rand();

            if (rand1 < rand2)
            {
                hregs->tranabortnum = (U16)rand();
                hregs->rabortcode = 0;
            }
        }
    }
    else
    {
        arflag = i2 & TXF_CTL_AR;   /* save the control flags  */

        if (hregs->tranhigharchange == hregs->tranlvl - 1 && arflag)
            hregs->tranhigharchange++;
        else
            hregs->tranctlflag &= ~TXF_CTL_AR;     /* turn off arflag */

        flflag = i2  & TXF_CTL_FLOAT;

        if (hregs->tranhigharchange == hregs->tranlvl - 1 && flflag)
            hregs->tranhighfloat++;
        else
            hregs->tranctlflag &= ~TXF_CTL_FLOAT;     /* turn off float flag */

        filtlvl = i2 & TXF_CTL_PIFC;  /* program filter level */

        hregs->tranprogfilttab[hregs->tranlvl - 2] = hregs->tranprogfiltlvl;
        hregs->tranprogfiltlvl = max(hregs->tranprogfiltlvl, filtlvl);
    }

    RELEASE_INTLOCK(hregs);
    return;
}

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

#endif /*!defined(_GEN_ARCH)*/
