/* QDIO.C       (C) Copyright Jan Jaeger, 2003-2012                  */
/*              (C) Copyright Harold Grovesteen, 2011                */
/*              Queued Direct Input Output                           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */
/*                                                                   */
/*      This module contains the Signal Adapter instruction          */
/*      and QEBSM EQBS and SQBS instructions                         */

#include "hstdinc.h"

#define _QDIO_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "qdio.h"
#include "inline.h"

DISABLE_GCC_UNUSED_SET_WARNING;

#undef PTIO
 #define PTIO(_class, _name) \
 PTT(PTT_CL_ ## _class,_name,regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff),regs->psw.IA_L)

#if defined(FEATURE_QUEUED_DIRECT_IO)

/*-------------------------------------------------------------------*/
/* B274 SIGA  - Signal Adapter                                   [S] */
/*-------------------------------------------------------------------*/
DEF_INST(signal_adapter)
{
int     b2;
RADR    effective_addr2;
DEVBLK *dev;                            /* -> device block           */

    S(inst, regs, b2, effective_addr2);

//  ARCH_DEP(display_inst) (regs, inst);
    PRIV_CHECK(regs);

#if defined(_FEATURE_SIE)
    if(SIE_STATE_BIT_OFF(regs, EC3, SIGAA))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    PTIO(IO,"SIGA");

    /* Specification exception if invalid function code */
    if((regs->GR_L(0)
#if defined(FEATURE_QEBSM)
                      & ~(FACILITY_ENABLED( HERC_QEBSM, regs ) ? SIGA_TOKEN : 0)
#endif /*defined(FEATURE_QEBSM)*/
                      ) > SIGA_FC_MAX)
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Locate the device block for this subchannel */
#if defined(FEATURE_QEBSM)
    if (FACILITY_ENABLED( HERC_QEBSM, regs )
      && (regs->GR_L(0) & SIGA_TOKEN))
        dev = find_device_by_subchan (TKN2IOID(regs->GR_G(1)));
    else
#endif /*defined(FEATURE_QEBSM)*/
    {
        /* Program check if the ssid including lcss is invalid */
        SSID_CHECK(regs);
        dev = find_device_by_subchan (regs->GR_L(1));
    }

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled or is not a QDIO subchannel */
    if (dev == NULL
        || (dev->pmcw.flag5 & PMCW5_V) == 0
        || (dev->pmcw.flag5 & PMCW5_E) == 0
        || (dev->pmcw.flag4 & PMCW4_Q) == 0)
    {
        PTIO(ERR,"*SIGA");
#if defined(_FEATURE_QUEUED_DIRECT_IO_ASSIST)
        SIE_INTERCEPT(regs);
#endif
        regs->psw.cc = 3;
        return;
    }

    /* Obtain the device lock */
    obtain_lock (&dev->lock);

    /* Check that device is QDIO active */
    if ((dev->scsw.flag2 & SCSW2_Q) == 0)
    {
        PTIO(ERR,"*SIGA");
        release_lock (&dev->lock);
        regs->psw.cc = 1;
        return;
    }

    switch(
           regs->GR_L(0)
#if defined(FEATURE_QEBSM)
                         & ~(FACILITY_ENABLED( HERC_QEBSM, regs ) ? SIGA_TOKEN : 0)
#endif /*defined(FEATURE_QEBSM)*/
                         ) {
    case SIGA_FC_R:
        if(dev->hnd->siga_r)
            regs->psw.cc = (dev->hnd->siga_r) (dev, regs->GR_L(2) );
        else
        {
            PTIO(ERR,"*SIGA");
            regs->psw.cc = 3;
        }
        break;

    case SIGA_FC_W:
        if(dev->hnd->siga_w)
            regs->psw.cc = (dev->hnd->siga_w) (dev, regs->GR_L(2) );
        else
        {
            PTIO(ERR,"*SIGA");
            regs->psw.cc = 3;
        }
        break;

    case SIGA_FC_S:
#if SIGA_FC_MAX >= SIGA_FC_S
        if(dev->hnd->siga_s)
            regs->psw.cc = (dev->hnd->siga_s) (dev, regs->GR_L(2), regs->GR_L(3) );
        else
        {
            PTIO(ERR,"*SIGA");
            regs->psw.cc = 3;
        }
#else
        /* No signalling required for sync requests as we emulate
           a real machine */
        regs->psw.cc = 0;
#endif
        break;

#if SIGA_FC_MAX >= SIGA_FC_M
    case SIGA_FC_M:
        if(dev->hnd->siga_m)
            regs->psw.cc = (dev->hnd->siga_m) (dev, regs->GR_L(2) );
        else
        {
            PTIO(ERR,"*SIGA");
            regs->psw.cc = 3;
        }
        break;
#endif

    default:
        PTIO(ERR,"*SIGA");

    }

    release_lock (&dev->lock);
}


#if defined(FEATURE_QEBSM)
/*-------------------------------------------------------------------*/
/* EB8A SQBS  - Set Queue Buffer State                       [RSY-a] */
/*-------------------------------------------------------------------*/
DEF_INST(set_queue_buffer_state)
{
int     r1, r3;                /* Register numbers                   */
int     b2;                    /* effective address base             */
VADR    effective_addr2;       /* effective address                  */

DEVBLK  *dev;                  /* Data device DEVBLK                 */

BYTE    state;                 /* Target buffer state                */
BYTE    oldstate;              /* Current buffer state               */
BYTE    nextstate;             /* Next buffer's state                */
U32     queues;                /* Total number of queues             */
U32     count;                 /* Number of buffers to set           */
U32     qidx;                  /* Queue index                        */
U32     bidx;                  /* SLSB buffer state index            */
U64     slsba;                 /* Storage list state block address   */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

//  ARCH_DEP(display_inst) (regs, inst);
    FACILITY_CHECK( HERC_QEBSM, regs );
    PRIV_CHECK(regs);

#if defined(_FEATURE_SIE)
    if(SIE_STATE_BIT_OFF(regs, EC3, SIGAA))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    PTIO(INF,"SQBS");

    dev = find_device_by_subchan(TKN2IOID(regs->GR_G(1)));

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled or is not a QDIO subchannel */
    if (dev == NULL
        || (dev->pmcw.flag5 & PMCW5_V) == 0
        || (dev->pmcw.flag5 & PMCW5_E) == 0
        || (dev->pmcw.flag4 & PMCW4_Q) == 0)
    {
        PTIO(ERR,"*SQBS");
#if defined(_FEATURE_QUEUED_DIRECT_IO_ASSIST)
        SIE_INTERCEPT(regs);
#endif
        regs->psw.cc = 2;
        return;
    }

#if 0
    /* Check that device is QDIO active */
    if ((dev->scsw.flag2 & SCSW2_Q) == 0)
    {
        PTIO(ERR,"*SQBS");
        regs->psw.cc = 2;
        return;
    }
#endif

    qidx  = regs->GR_H(r1);       /* Queue index */
    bidx  = regs->GR_L(r1);       /* Buffer index */
    count = regs->GR_L(r3) < 128 ? regs->GR_L(r3) : 128; /* Number of buffers */
    state = effective_addr2 & 0xFF;

    queues = (U32)(dev->qdio.i_qcnt + dev->qdio.o_qcnt);

    if ( (qidx >= queues) || (bidx > 127) )
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    if (qidx < (U32)dev->qdio.i_qcnt)
        slsba = dev->qdio.i_slsbla[qidx];
    else
        slsba = dev->qdio.o_slsbla[qidx - dev->qdio.i_qcnt];

    oldstate = nextstate = ARCH_DEP(wfetchb)((VADR)(slsba+bidx), USE_REAL_ADDR, regs);

    while (count && oldstate == nextstate)
    {
        ARCH_DEP(wstoreb)(state, (VADR)(slsba+bidx), USE_REAL_ADDR, regs);

        bidx++; bidx &= 0x7F;              /* Advance and wrap index */
        count--;

        if(count)
            nextstate = ARCH_DEP(wfetchb)((VADR)(slsba+bidx), USE_REAL_ADDR, regs);
    }

    regs->GR_L(r1) = bidx;              /* Return buffer state index */
    regs->GR_L(r3) = count;    /* Return number of unchanged buffers */
    regs->psw.cc = count ? 1 : 0;
}


/*-------------------------------------------------------------------*/
/* B99C EQBS  - Extract Queue Buffer State                   [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_queue_buffer_state)
{
int     r1, r2, r3, m4;       /* Register numbers                    */
VADR    effective_addr2 = 0;  /* effective address                   */

DEVBLK  *dev;                 /* Data device DEVBLK                  */

int     autoack;              /* Flag for auto-acknowkledgement      */
BYTE    state;                /* State extracted from first buffer   */
BYTE    nextstate;            /*    next buffer                      */
U32     queues;               /* Total number of queues              */
U32     count;                /* Number of buffers to set            */
U32     qidx;                 /* Queue index                         */
U32     bidx;                 /* SLSB buffer state index             */
U64     slsba;                /* Storage list state block address    */

    RRF_RM(inst, regs, r1, r2, r3, m4);

//  ARCH_DEP(display_inst) (regs, inst);
    FACILITY_CHECK( HERC_QEBSM, regs );
    PRIV_CHECK(regs);

#if defined(_FEATURE_SIE)
    if(SIE_STATE_BIT_OFF(regs, EC3, SIGAA))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    PTIO(INF,"EQBS");

    dev = find_device_by_subchan(TKN2IOID(regs->GR_G(1)));

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled or is not a QDIO subchannel */
    if (dev == NULL
        || (dev->pmcw.flag5 & PMCW5_V) == 0
        || (dev->pmcw.flag5 & PMCW5_E) == 0
        || (dev->pmcw.flag4 & PMCW4_Q) == 0)
    {
        PTIO(ERR,"*EQBS");
#if defined(_FEATURE_QUEUED_DIRECT_IO_ASSIST)
        SIE_INTERCEPT(regs);
#endif
        regs->psw.cc = 2;
        return;
    }

#if 0
    /* Check that device is QDIO active */
    if ((dev->scsw.flag2 & SCSW2_Q) == 0)
    {
        PTIO(ERR,"*EQBS");
        regs->psw.cc = 2;
        return;
    }
#endif

    qidx  = regs->GR_H(r1);                           /* Queue index */
    bidx  = regs->GR_L(r1);                          /* Buffer index */
    autoack = (regs->GR_H(r2) & 0x80000000);
    count = regs->GR_L(r3) < 128 ? regs->GR_L(r3) : 128; /* Number of buffers */

    queues = (U32)(dev->qdio.i_qcnt + dev->qdio.o_qcnt);

    if ( (qidx >= queues) || (bidx > 127) )
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    if (qidx < (U32)dev->qdio.i_qcnt)
        slsba = dev->qdio.i_slsbla[qidx];
    else
        slsba = dev->qdio.o_slsbla[qidx - dev->qdio.i_qcnt];

    state = nextstate = ARCH_DEP(wfetchb)((VADR)(slsba+bidx), USE_REAL_ADDR, regs);

    while(count && state == nextstate)
    {
        if (autoack)
            switch(nextstate) {

                case SLSBE_INPUT_COMPLETED:
                    ARCH_DEP(wstoreb)
                        (SLSBE_INPUT_ACKED, (VADR)(slsba+bidx), USE_REAL_ADDR, regs);
                    break;
#if 0
                case SLSBE_OUTPUT_COMPLETED:
                    ARCH_DEP(wstoreb)
                        (SLSBE_OUTPUT_PRIMED, (VADR)(slsba+bidx), USE_REAL_ADDR, regs);
                    break;
#endif
            }

        bidx++; bidx &= 0x7F;              /* Advance and wrap index */
        count--;

        if(count)
            nextstate = ARCH_DEP(wfetchb)((VADR)(slsba+bidx), USE_REAL_ADDR, regs);
    }

    regs->GR_L(r1) = bidx;
    regs->GR_LHLCL(r2) = state;
    regs->GR_L(r3) = count;
    regs->psw.cc = count ? 1 : 0;
}
#endif /*defined(FEATURE_QEBSM)*/

#if defined(FEATURE_SVS)
/*-------------------------------------------------------------------*/
/* B265 SVS   - Set Vector Summary                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(set_vector_summary)
{
int     r1, r2;                /* Register numbers                   */

    RRE(inst, regs, r1, r2);
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);
    ODD_CHECK(r1, regs);

    switch(regs->GR_L(1)) {

        case 3:   // Clear Global Summary
            regs->GR(r1+1) = 0;
            break;

        default:
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
     }
}
#endif /*defined(FEATURE_SVS)*/

#endif /*defined(FEATURE_QUEUED_DIRECT_IO)*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "qdio.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "qdio.c"
#endif

/*-------------------------------------------------------------------*/
/* Dummy debug trace callback function in case NULL is passed        */
/*-------------------------------------------------------------------*/
static inline void qdio_notrace( DEVBLK* dev, char* fmt, ... )
{
    UNREFERENCED( dev );
    UNREFERENCED( fmt );
}

/*-------------------------------------------------------------------*/
/*                   qdio_storage_access_check                       */
/*-------------------------------------------------------------------*/
/* Check storage access. Returns 0 if successful or CSW_PROGC or     */
/* CSW_PROTC if error. NOTE: only storage access is checked. The     */
/* caller should update the storage key as appropriate upon return.  */
/*-------------------------------------------------------------------*/
static inline int qdio_storage_access_check
(
    U64      addr,              /* Storage address being accessed    */
    size_t   len,               /* Length of storage being accessed  */
    int      key,               /* Storage access key                */
    int      acc,               /* Access type (STORKEY_REF/_CHANGE) */
    DEVBLK*  dev,               /* Pointer to device block           */
    QDIOTRC* dbg                /* Optional debug trace callback     */
)
{
    if (!dbg)
         dbg = qdio_notrace;

    if ((addr + len) > dev->mainlim)
    {
        dbg( dev, "Address %"PRIx64" + %"PRIx64
                  " beyond end of storage\n", addr, (U64) len );
        return CSW_PROGC;                           /* Beyond EOS    */
    }

    if (dev->orb.flag5 & ORB5_A)                    /* Check limits? */
    {
        if (dev->pmcw.flag5 & PMCW5_LM_LOW)         /* Lower limit?  */
        {
            if (addr < sysblk.addrlimval)           /* Below limit?  */
            {
                dbg( dev, "Address %"PRIx64
                          " below limit of %"PRIx64"\n",
                     addr, sysblk.addrlimval );
                return CSW_PROGC;
            }
        }

        if (dev->pmcw.flag5 & PMCW5_LM_HIGH)        /* Upper limit?  */
        {
            if ((addr + len) > sysblk.addrlimval)   /* Above limit?  */
            {
                dbg( dev, "Address %"PRIx64" + %"PRIx64
                          " above limit of %"PRIx64"\n",
                     addr, (U64) len, sysblk.addrlimval );
                return CSW_PROGC;
            }
        }
    }

    /* Key 0 is allowed to access all of storage in any manner. */
    if (key == 0)
        return 0;

    /* This may not be described anywhere and could be wrong, but
       apparently z/VM TC/IP expects Key 14 to allow access to all
       of storage, including storage frames with key 0.
    */
    if ((key & STORKEY_KEY) == 0xE0)    /* Key 14 special case? */
        return 0;

    /* If storage keys match then access to storage is allowed. */
    if (key == (STORAGE_KEY( addr,dev ) & STORKEY_KEY))
        return 0;

    /* Check for attempt to fetch protected storage. */
    if ((acc & STORKEY_REF) && (STORAGE_KEY( addr, dev ) & STORKEY_FETCH))
    {
        dbg( dev, "Fetch protected: key: %x, storkey: %x, acc: %x\n",
             key, STORAGE_KEY( addr, dev ), acc );
        return CSW_PROTC;
    }

    /* Check for attempt to modify protected storage. */
    if (acc & STORKEY_CHANGE)
    {
        dbg( dev, "Key mismatch: key: %x, storkey: %x, acc: %x\n",
             key, STORAGE_KEY( addr, dev ), acc );
        return CSW_PROTC;
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/*             qdio_storage_access_check_and_update                  */
/*-------------------------------------------------------------------*/
/* Check storage access and update reference and change bits.        */
/* Returns 0 if successful or CSW_PROGC or CSW_PROTC if error.       */
/* Storage key ref & change bits are only updated if successful.     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int qdio_storage_access_check_and_update
(
    U64      addr,              /* Storage address being accessed    */
    size_t   len,               /* Length of storage being accessed  */
    int      key,               /* Storage access key                */
    int      acc,               /* Access type (STORKEY_REF/_CHANGE) */
    DEVBLK*  dev,               /* Pointer to device block           */
    QDIOTRC* dbg                /* Optional debug trace callback     */
)
{
    int  rc;
    if ((rc = qdio_storage_access_check( addr, len, key, acc, dev, dbg )) == 0)
        STORAGE_KEY( addr, dev ) |= (acc & (STORKEY_REF | STORKEY_CHANGE));
    return rc;
}

#endif /*!defined(_GEN_ARCH)*/
