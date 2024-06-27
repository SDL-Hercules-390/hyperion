/* CHSC.C       (C) Copyright Jan Jaeger, 2002-2012                  */
/*              Channel Subsystem Call                               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/* This module implements channel subsystem interface functions      */
/* for the Hercules ESA/390 emulator.                                */
/*                                                                   */
/* This implementation is based on the S/390 Linux implementation    */

#include "hstdinc.h"

#define _CHSC_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "chsc.h"

#if defined(FEATURE_CHSC)

/*-------------------------------------------------------------------*/
/* CHSC Debugging                                                    */
/*-------------------------------------------------------------------*/

#define ENABLE_CHSC_DEBUG   0    // 1:always, 0:never, #undef:maybe

#if (!defined(ENABLE_CHSC_DEBUG) && defined(DEBUG)) || \
    (defined(ENABLE_CHSC_DEBUG) && ENABLE_CHSC_DEBUG)
 #define CHSC_DEBUG
#endif

#if defined(CHSC_DEBUG)
 #define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
 #include "dbgtrace.h"                   // (Fish: DEBUGGING)
 #define  NO_CHSC_OPTIMIZE               // (Fish: DEBUGGING) (MSVC only)
#endif

#if defined( _MSVC_ ) && defined( NO_CHSC_OPTIMIZE )
  #pragma optimize( "", off )           // disable optimizations for reliable breakpoints
#endif

#if defined(CHSC_DEBUG)
  static inline void DUMP_CHSC_REQRSP(U16 req, CHSC_REQ *chsc_req);
#endif


/*-------------------------------------------------------------------*/
/* CHSC_REQ12: Store Configuration Information                       */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_get_conf_info) (CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
U16 req_len, rsp_len;

CHSC_REQ12 *chsc_req12 = (CHSC_REQ12 *)(chsc_req);
CHSC_RSP12 *chsc_rsp12 = (CHSC_RSP12 *)(chsc_rsp+1);

    /* Set response length based on MIF mode */
    switch (sysblk.operation_mode)
    {
        case om_emif:
            rsp_len = sizeof(CHSC_RSP) + sizeof(CHSC_RSP12);
            break;

        default:
            rsp_len = 0x0138;
            break;
    }

    FETCH_HW(req_len, chsc_req12->length);

    /* If the response won't fit in the requested length, return an
     * error back to the caller.
     */
    if (!chsc_max_rsp(req_len, sizeof(CHSC_RSP12)))
        return chsc_req_errreq(chsc_rsp, 0);

    /* Clear the response area */
    memset(chsc_rsp12, 0, rsp_len);

    /* Where did this bit come from? Is it a z-machine indicator? */
    chsc_rsp12->unknow00A = 0x01;

    /* FIXME: BASIC mode operations are not returning the proper
     *        information; instead, return of an error may be required.
     */
    if (sysblk.operation_mode != om_basic)
    {
        BYTE*   z = (BYTE*)chsc_rsp;
        BYTE    valid_bit_domain;
        size_t  valid_bit_offset;
        size_t  lparname_offset;

        /* Save Hercules LPAR number as the partition number */
        *(z + 11) = valid_bit_domain = sysblk.lparnum;

        /* Set current LPAR name valid bit and LPAR name
         *
         * Note: The value fields are arrays of *ALL* defined and active
         *       LPAR names and validity bits for the host system. It
         *       should also be noted that the offsets to the array of
         *       LPAR name valid bits and to the array of LPAR names is
         *       different between non-EMIF and EMIF modes.
         *
         * FIXME: Future. Supply and maintain the information from *ALL*
         *        active Hercules instances on the host system.
         */
        switch (sysblk.operation_mode)
        {
            case om_mif:
                --valid_bit_domain;
                valid_bit_offset = 180;
                lparname_offset  = 184;
                break;

         /* case om_emif: */
            default:
                valid_bit_offset = 184;
                lparname_offset  = 216;
                break;
        }

        /* Set LPAR name */
        get_lparname(z + lparname_offset + (sysblk.lparnum << 3));

        /* Set valid bit */
        *(z + valid_bit_offset + (valid_bit_domain >> 3)) |=
            (0x80 >> (valid_bit_domain & 0x07));
    }

    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}


/*-------------------------------------------------------------------*/
/* CHSC_REQ4: Store Subchannel Description Data                      */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_get_sch_desc) (CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
U16 req_len, rsp_len, lcss, max_rsp, work;
int sch, f_sch, l_sch, num_sch, max_sch;

CHSC_REQ4 *chsc_req4 = (CHSC_REQ4 *)(chsc_req);
CHSC_RSP4 *chsc_rsp4 = (CHSC_RSP4 *)(chsc_rsp+1);

    FETCH_HW(work,chsc_req4->f_sch); f_sch = work;
    FETCH_HW(work,chsc_req4->l_sch); l_sch = work;
    FETCH_HW(lcss,chsc_req4->ssidfmt);
    lcss &= CHSC_REQ4_SSID;
    lcss >>= 4;

    FETCH_HW(req_len, chsc_req4->length);

    if (!(max_rsp = chsc_max_rsp(req_len, sizeof(CHSC_RSP4))) || l_sch < f_sch)
        return chsc_req_errreq(chsc_rsp, 0);

    num_sch = (l_sch - f_sch) + 1;
    max_sch = sysblk.highsubchan[lcss]-1;
    max_rsp = (U16) min((int)max_rsp, num_sch);
    rsp_len = sizeof(CHSC_RSP) + (max_rsp * sizeof(CHSC_RSP4));

    if (f_sch <= max_sch)
    {
    DEVBLK *dev;

        for(sch = f_sch; sch <= l_sch && max_rsp; sch++, max_rsp--, chsc_rsp4++)
        {
            memset(chsc_rsp4, 0, sizeof(CHSC_RSP4) );
            if (sch <= max_sch)
            {
                if((dev = find_device_by_subchan((LCSS_TO_SSID(lcss) << 16)|sch)))
                {
                    int n;
                    chsc_rsp4->flags1 |= CHSC_RSP4_F1_SCH_VALID;
                    if(dev->pmcw.flag5 & PMCW5_V)
                        chsc_rsp4->flags1 |= CHSC_RSP4_F1_DEV_VALID;
                    chsc_rsp4->flags1 |= ((dev->pmcw.flag25 & PMCW25_TYPE) >> 2);
                    chsc_rsp4->path_mask = dev->pmcw.pim;
                    chsc_rsp4->unit_addr = dev->devnum & 0xff;
                    STORE_HW(chsc_rsp4->devno,dev->devnum);
                    STORE_HW(chsc_rsp4->sch, sch);
                    memcpy(chsc_rsp4->chpid, dev->pmcw.chpid, 8);
                    if(dev->fla[0])
                        chsc_rsp4->fla_valid_mask = dev->pmcw.pim;
                    for(n = 0; n < 8; n++)
                        if(dev->pmcw.pim & (0x80 >> n))
                        {
                            STORE_HW(chsc_rsp4->fla[n], dev->fla[n]);
                        }
                }
            }
        }
    }
    else /* f_sch > max_sch */
    {
        for(sch = f_sch; sch <= l_sch && max_rsp; sch++, max_rsp--, chsc_rsp4++)
            memset(chsc_rsp4, 0, sizeof(CHSC_RSP4) );
    }

    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}


/*-------------------------------------------------------------------*/
/* CHSC_REQ6: Store Subchannel Control-Unit Data                     */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_get_cu_desc) (CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
U16 req_len, rsp_len, lcss, cun, max_rsp, work;
int sch, f_sch, l_sch, num_sch, max_sch;

CHSC_REQ6 *chsc_req6 = (CHSC_REQ6 *)(chsc_req);
CHSC_RSP6 *chsc_rsp6 = (CHSC_RSP6 *)(chsc_rsp+1);

    FETCH_HW(work,chsc_req6->f_sch); f_sch = work;
    FETCH_HW(work,chsc_req6->l_sch); l_sch = work;
    FETCH_HW(lcss,chsc_req6->ssidfmt);
    lcss &= CHSC_REQ6_SSID;
//  lcss >>= 0;

    FETCH_HW(req_len, chsc_req6->length);

    if (!(max_rsp = chsc_max_rsp(req_len, sizeof(CHSC_RSP6))) || l_sch < f_sch)
        return chsc_req_errreq(chsc_rsp, 0);

    num_sch = (l_sch - f_sch) + 1;
    max_sch = sysblk.highsubchan[lcss]-1;
    max_rsp = (U16) min((int)max_rsp, num_sch);
    rsp_len = sizeof(CHSC_RSP) + (max_rsp * sizeof(CHSC_RSP6));

    if (f_sch <= max_sch)
    {
    DEVBLK *dev;

        for(sch = f_sch; sch <= l_sch && max_rsp; sch++, max_rsp--, chsc_rsp6++)
        {
            memset(chsc_rsp6, 0, sizeof(CHSC_RSP6) );
            if (sch <= max_sch)
            {
                if((dev = find_device_by_subchan((LCSS_TO_SSID(lcss) << 16)|sch)))
                {
                    int n;
                    chsc_rsp6->flags1 |= CHSC_RSP6_F1_SCH_VALID;
                    if(dev->pmcw.flag5 & PMCW5_V)
                        chsc_rsp6->flags1 |= CHSC_RSP6_F1_DEV_VALID;
                    chsc_rsp6->flags1 |= ((dev->pmcw.flag25 & PMCW25_TYPE) >> 2);
                    chsc_rsp6->path_mask = dev->pmcw.pim;
                    STORE_HW(chsc_rsp6->devnum,dev->devnum);
                    STORE_HW(chsc_rsp6->sch, sch);
                    memcpy(chsc_rsp6->chpid, dev->pmcw.chpid, 8);
                    for(n = 0; n < 8; n++)
                    {
                        if(dev->pmcw.pim & (0x80 >> n))
                        {
                            cun = ((dev->devnum & 0x00F0) << 4) | dev->pmcw.chpid[n];
                            STORE_HW(chsc_rsp6->cun[n], cun);
                        }
                    }
                }
            }
        }
    }
    else /* f_sch > max_sch */
    {
        for(sch = f_sch; sch <= l_sch && max_rsp; sch++, max_rsp--, chsc_rsp6++)
            memset(chsc_rsp6, 0, sizeof(CHSC_RSP6) );
    }

    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}


/*-------------------------------------------------------------------*/
/* CHSC_REQ10: Store Channel-Subsystem Characteristics               */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_get_css_info) (REGS *regs, CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
CHSC_RSP10 *chsc_rsp10;
U16 req_len, rsp_len;

    chsc_rsp10 = (CHSC_RSP10 *)(chsc_rsp+1);

    FETCH_HW(req_len, chsc_req->length);

    rsp_len = sizeof(CHSC_RSP) + sizeof(CHSC_RSP10);

    if (!chsc_max_rsp(req_len, sizeof(CHSC_RSP10)))
        return chsc_req_errreq(chsc_rsp, 0);

    memset(chsc_rsp10->general_char, 0, sizeof(chsc_rsp10->general_char));
    memset(chsc_rsp10->chsc_char,    0, sizeof(chsc_rsp10->chsc_char));

#if defined(FEATURE_REGION_RELOCATE)
    CHSC_SB(chsc_rsp10->general_char,2);
    CHSC_SB(chsc_rsp10->general_char,5);
#endif
#if defined(FEATURE_CANCEL_IO_FACILITY)
    CHSC_SB(chsc_rsp10->general_char,6);
#endif

    CHSC_SB(chsc_rsp10->general_char,7);         /* Concurrent Sense */
    CHSC_SB(chsc_rsp10->general_char,12);              /* Dynamic IO */

    if (sysblk.lparmode)
    {
        CHSC_SB(chsc_rsp10->general_char,10);                 /* MIF */
        CHSC_SB(chsc_rsp10->general_char,13);                /* LPAR */
    }

#if defined(FEATURE_QUEUED_DIRECT_IO)
    CHSC_SB(chsc_rsp10->general_char,41);         /* Adapter Int Fac */

    CHSC_SB(chsc_rsp10->chsc_char,1);            /* 0x0002 Supported */
    CHSC_SB(chsc_rsp10->chsc_char,2);            /* 0x0006 Supported */
    CHSC_SB(chsc_rsp10->chsc_char,3);            /* 0x0004 Supported */
    CHSC_SB(chsc_rsp10->chsc_char,8);            /* 0x0024 Supported */

    if (FACILITY_ENABLED( HERC_QDIO_ASSIST, regs ))
        CHSC_SB(chsc_rsp10->general_char,61);         /* QDIO Assist */
#endif /*defined(FEATURE_QUEUED_DIRECT_IO)*/

#if defined(_FEATURE_QDIO_TDD)
    if (FACILITY_ENABLED( HERC_QDIO_TDD, regs ))
        CHSC_SB(chsc_rsp10->general_char,56);  /* AIF Time Delay Dis */
#endif /*defined(_FEATURE_QDIO_TDD)*/

#if defined(_FEATURE_QEBSM)
    if (FACILITY_ENABLED( HERC_QEBSM, regs ))
    {
        CHSC_SB(chsc_rsp10->general_char,58); /* SQBS/EQBS Available */
        CHSC_SB(chsc_rsp10->general_char,66); /* SQBS/EQBS Interpret */
    }
#endif /*defined(_FEATURE_QEBSM)*/

#if defined(_FEATURE_QDIO_THININT)
    if (FACILITY_ENABLED( HERC_QDIO_THININT, regs ))
    {
        CHSC_SB(chsc_rsp10->general_char,67);   /* OSA/FCP Thin Ints */
        CHSC_SB(chsc_rsp10->chsc_char,107);      /* 0x0021 Supported */
    }
#endif /*defined(_FEATURE_QDIO_THININT)*/

//  CHSC_SB(chsc_rsp10->general_char,45);            /* Multiple CSS */
//  CHSC_SB(chsc_rsp10->general_char,64);        /* QDIO Multiple CU */
//  CHSC_SB(chsc_rsp10->general_char,65);      /* OSA System Console */
//  CHSC_SB(chsc_rsp10->general_char,82);                     /* CIB */
//  CHSC_SB(chsc_rsp10->general_char,88);                     /* FCX */

//  CHSC_SB(chsc_rsp10->chsc_char,84);                       /* SECM */
//  CHSC_SB(chsc_rsp10->chsc_char,86);                       /* SCMC */
//  CHSC_SB(chsc_rsp10->chsc_char,107);   /* Set Channel Subsys Char */
//  CHSC_SB(chsc_rsp10->chsc_char,108);                /* Fast CHSCs */

    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}


/*-------------------------------------------------------------------*/
/* CHSC_REQ24: Store Subchannel QDIO Data                            */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_get_ssqd) (CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
U16 req_len, rsp_len, lcss, max_rsp, work;
int sch, f_sch, l_sch, num_sch, max_sch;

CHSC_REQ24 *chsc_req24 = (CHSC_REQ24 *)(chsc_req);
CHSC_RSP24 *chsc_rsp24 = (CHSC_RSP24 *)(chsc_rsp+1);

    FETCH_HW(work,chsc_req24->f_sch); f_sch = work;
    FETCH_HW(work,chsc_req24->l_sch); l_sch = work;
    FETCH_HW(lcss,chsc_req24->ssidfmt);
    lcss &= CHSC_REQ24_SSID;
    lcss >>= 4;

    FETCH_HW(req_len, chsc_req24->length);

    if (!(max_rsp = chsc_max_rsp(req_len, sizeof(CHSC_RSP24))) || l_sch < f_sch)
        return chsc_req_errreq(chsc_rsp, 0);

    num_sch = (l_sch - f_sch) + 1;
    max_sch = sysblk.highsubchan[lcss]-1;
    max_rsp = (U16) min((int)max_rsp, num_sch);
    rsp_len = sizeof(CHSC_RSP) + (max_rsp * sizeof(CHSC_RSP24));

    if (f_sch <= max_sch)
    {
    DEVBLK *dev;

        for(sch = f_sch; sch <= l_sch && max_rsp; sch++, max_rsp--, chsc_rsp24++)
        {
            memset(chsc_rsp24, 0, sizeof(CHSC_RSP24) );
            if (sch <= max_sch)
            {
                if((dev = find_device_by_subchan((LCSS_TO_SSID(lcss) << 16)|sch)))
                    if(dev->hnd->ssqd)
                        (dev->hnd->ssqd)(dev, chsc_rsp24);
            }
        }
    }
    else /* f_sch > max_sch */
    {
        for(sch = f_sch; sch <= l_sch && max_rsp; sch++, max_rsp--, chsc_rsp24++)
            memset(chsc_rsp24, 0, sizeof(CHSC_RSP24) );
    }

    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}


#if 0
/*-------------------------------------------------------------------*/
/* CHSC_REQ31: Enable Facility                                       */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_enable_facility) (CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
U16 req_len, rsp_len, facility;

CHSC_REQ31* chsc_req31 = (CHSC_REQ31*) (chsc_req);

    FETCH_HW( req_len, chsc_req31->length );

    rsp_len = sizeof(CHSC_RSP) + 0;

    if (!chsc_max_rsp(req_len, 0))
        return chsc_req_errreq(chsc_rsp, 0);

    /* Fetch requested facility and enable it */
    FETCH_HW( facility, chsc_req31->facility );

    switch (facility)
    {
    case CHSC_REQ31_MSS:
//      if(FACILITY_ENABLED_DEV(MCSS))
        {
            /* Enable Multiple Subchannel-Sets Facility */
            STORE_HW( chsc_rsp->rsp, CHSC_REQ_OK );
        }

    default: /* Unknown Facility */
        STORE_HW( chsc_rsp->rsp, CHSC_REQ_FACILITY );
        break;
    }

    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}
#endif


#if defined(_FEATURE_QDIO_THININT)
/*-------------------------------------------------------------------*/
/* CHSC_REQ21: Set Subchannel Indicator                              */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_set_sci) (CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
U16 req_len, rsp_len;
DEVBLK *dev;
U32 ssid;
int rc;

CHSC_REQ21 *chsc_req21 = (CHSC_REQ21 *)(chsc_req);

    FETCH_HW( req_len, chsc_req21->length );

    rsp_len = sizeof(CHSC_RSP) + 0;

    if (!chsc_max_rsp(req_len, 0))
        return chsc_req_errreq(chsc_rsp, 0);

    /* Fetch requested Subchannel Id */
    FETCH_FW(ssid, chsc_req21->ssid);

    if((dev = find_device_by_subchan(ssid)))
        if(dev->hnd->ssci)
            if(!(rc = (dev->hnd->ssci)(dev, chsc_req21)))
                return chsc_req_ok(chsc_rsp, sizeof(CHSC_RSP), 0);

//  return chsc_req_errreq(chsc_rsp, 0);
    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}
#endif /*defined(_FEATURE_QDIO_THININT)*/


/*-------------------------------------------------------------------*/
/* CHSC_REQ2: Store Channel Path Description                         */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(chsc_get_chp_desc) (CHSC_REQ *chsc_req, CHSC_RSP *chsc_rsp)
{
U16 req_len, rsp_len, max_rsp;
int fmt1, chp, f_chp, l_chp, num_chps;
size_t rsp_size;
DEVBLK *dev;

CHSC_REQ2   *chsc_req2   = (CHSC_REQ2 *)  (chsc_req);
CHSC_RSP2   *chsc_rsp2   = (CHSC_RSP2 *)  (chsc_rsp+1);
CHSC_RSP2F1 *chsc_rsp2f1 = (CHSC_RSP2F1 *)(chsc_rsp+1);

    f_chp = chsc_req2->first_chpid;
    l_chp = chsc_req2->last_chpid;

    FETCH_HW(req_len, chsc_req2->length);

    fmt1 = (chsc_req2->flags1 & CHSC_REQ2_F1_C) ? 1 : 0;
    rsp_size = fmt1 ? sizeof(CHSC_RSP2F1) : sizeof(CHSC_RSP2);

    if(!(max_rsp = chsc_max_rsp(req_len, rsp_size))
// ZZ || (chsc_req2->rfmt != 1 && chsc_req2->rfmt != 2)
      || f_chp > l_chp)
        return chsc_req_errreq(chsc_rsp, 0);

    num_chps = (l_chp - f_chp) + 1;
    max_rsp  = (U16) min((int)max_rsp, num_chps);
    rsp_len  = sizeof(CHSC_RSP) + (max_rsp * rsp_size);

    if (!fmt1)
    {
        for(chp = f_chp; chp <= l_chp && max_rsp; chp++, max_rsp--, chsc_rsp2++)
        {
            memset(chsc_rsp2, 0, sizeof(CHSC_RSP2));
            chsc_rsp2->chpid = chp;

            for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
                if (dev->allocated
                  && (dev->pmcw.chpid[0] == chp)
                  && dev->chptype[0])
                {
                    chsc_rsp2->flags1   |= CHSC_RSP2_F1_CHPID_VALID;
                    chsc_rsp2->chp_type  = dev->chptype[0];
//                  chsc_rsp2->lsn       = 0;
//                  chsc_rsp2->swla      = 0;
//                  chsc_rsp2->chla      = 0;
                    break;
                }
        }
    }
    else
    {
        for(chp = f_chp; chp <= l_chp && max_rsp; chp++, max_rsp--, chsc_rsp2f1++)
        {
            memset(chsc_rsp2f1, 0, sizeof(CHSC_RSP2F1));
            chsc_rsp2f1->chpid = chp;

            for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
                if (dev->allocated
                  && (dev->pmcw.chpid[0] == chp)
                  && dev->chptype[0])
                {
                    chsc_rsp2f1->flags1   |= CHSC_RSP2F1_F1_CHPID_VALID;
                    chsc_rsp2f1->chp_type  = dev->chptype[0];
//                  chsc_rsp2f1->lsn       = 0;
//                  chsc_rsp2f1->chpp      = 0;
//                  STORE_HW(chsc_rsp2f1->mdc,0x0000);
//                  STORE_HW(chsc_rsp2f1->flags2,0x0000);
                    break;
                }
        }
    }

    return chsc_req_ok(chsc_rsp, rsp_len, 0);
}


/*-------------------------------------------------------------------*/
/* B25F CHSC  - Channel Subsystem Call                         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(channel_subsystem_call)
{
int     r1, r2;                                 /* register values   */
VADR    n;                                      /* Unsigned work     */
BYTE   *mn;                                     /* Unsigned work     */
U16     req_len;                                /* Length of request */
U16     req;                                    /* Request code      */
CHSC_REQ *chsc_req;                             /* Request structure */
CHSC_RSP *chsc_rsp;                             /* Response structure*/

    RRE(inst, regs, r1, r2);

    /* Display instruction if debugging */
#if defined(CHSC_DEBUG)
    ARCH_DEP(display_inst) (regs, inst);
#endif

    PER_ZEROADDR_CHECK( regs, r1 );
    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);

    SIE_INTERCEPT(regs);

    PTT_INF("CHSC",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);

    /* Check operand-1 for page alignment */
    n = regs->GR(r1) & ADDRESS_MAXWRAP(regs);

    if(n & 0xFFF)
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Get pointer to request/response */
    mn = MADDR(n, r1, regs, ACCTYPE_READ, regs->psw.pkey);
    chsc_req = (CHSC_REQ*)(mn);

    /* Fetch length of request field */
    FETCH_HW(req_len, chsc_req->length);

    /* Point to beginning of response field */
    chsc_rsp = (CHSC_RSP*)((BYTE*)chsc_req + req_len);

    /* Check for invalid request length */
    if((req_len < sizeof(CHSC_REQ))
      || (req_len > (CHSC_REQRSP_SIZE - sizeof(CHSC_RSP))))
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);

    /* Fetch the CHSC request code */
    FETCH_HW(req,chsc_req->req);

    /* Verify we have write access to the page */
    ARCH_DEP(validate_operand) (n, r1, 0, ACCTYPE_WRITE, regs);

    switch(req) {

        case CHSC_REQ_CHPDESC: /* 0x0002  Store Channel-Path Description */
            regs->psw.cc = ARCH_DEP(chsc_get_chp_desc) (chsc_req, chsc_rsp);
            break;

        case CHSC_REQ_SCHDESC: /* 0x0004  Store Subchannel Description Data */
            regs->psw.cc = ARCH_DEP(chsc_get_sch_desc) (chsc_req, chsc_rsp);
            break;

        case CHSC_REQ_CUDESC:  /* 0x0006  Store Subchannel Control-Unit Data */
            regs->psw.cc = ARCH_DEP(chsc_get_cu_desc) (chsc_req, chsc_rsp);
            break;

        case CHSC_REQ_CSSINFO: /* 0x0010  Store Channel-Subsystem Characteristics */
            regs->psw.cc = ARCH_DEP(chsc_get_css_info) (regs, chsc_req, chsc_rsp);
            break;

        case CHSC_REQ_CNFINFO: /* 0x0012  Store Configuration Information */
            regs->psw.cc = ARCH_DEP(chsc_get_conf_info) (chsc_req, chsc_rsp);
            break;

#if defined(_FEATURE_QDIO_THININT)

        case CHSC_REQ_SETSSSI: /* 0x0021  Set Subchannel Indicator */
            if (FACILITY_ENABLED( HERC_QDIO_THININT, regs ))
            {
                regs->psw.cc = ARCH_DEP(chsc_set_sci) (chsc_req, chsc_rsp);
                break;
            }
            else
                goto chsc_error;

#endif /*defined(_FEATURE_QDIO_THININT)*/

        case CHSC_REQ_GETSSQD: /* 0x0024  Store Subchannel QDIO Data */
            regs->psw.cc = ARCH_DEP(chsc_get_ssqd) (chsc_req, chsc_rsp);
            break;

#if 0
        case CHSC_REQ_ENFACIL: /* 0x0031  Enable Facility */
            regs->psw.cc = ARCH_DEP(chsc_enable_facility) (chsc_req, chsc_rsp);
            break;
#endif

        default:
#if defined(_FEATURE_QDIO_THININT)
        chsc_error:
#endif /*defined(_FEATURE_QDIO_THININT)*/
            PTT_ERR("*CHSC",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
            if( HDC3(debug_chsc_unknown_request, chsc_rsp, chsc_req, regs) )
                break;
            STORE_HW(chsc_rsp->length,sizeof(CHSC_RSP));
            STORE_HW(chsc_rsp->rsp,CHSC_REQ_INVALID);
            /* No reason code */
            STORE_FW(chsc_rsp->info,0);
            /* Return cc0 even for unsupported requests?? */
            regs->psw.cc = 0;
            break;
    }

    /* Show results if debugging */
    #if defined(CHSC_DEBUG)
    DUMP_CHSC_REQRSP( req, chsc_req );
    #endif
}
#endif /*defined(FEATURE_CHSC)*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "chsc.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "chsc.c"
#endif

/*-------------------------------------------------------------------*/
/* DUMP CHSC Request/Response buffer debugging function              */
/*-------------------------------------------------------------------*/
#if defined(CHSC_DEBUG)
static inline void DUMP_CHSC_REQRSP(U16 req, CHSC_REQ *chsc_req)
{
U16 req_len, rsp_len;
CHSC_RSP *chsc_rsp;
BYTE* p;
int disp, len;
#ifndef DUMPLINE
#define DUMPLINE MSGBUF(linebuf,"      +%04X:%s  |%s|\n",disp-16,hexbuf,charbuf)
#endif
BYTE linebuf[6+1+4+1+((1+8)*4)+2+1+16+1+1+1] = {0};
BYTE hexbuf[((1+8)*4)+1] = {0};
BYTE charbuf[16+1] = {0};
BYTE hex[2+1] = {0};
BYTE c;
BYTE dumpbuf[32*1024] = {0};

    p = (BYTE*) chsc_req;
    FETCH_HW(req_len, chsc_req->length);
    chsc_rsp = (CHSC_RSP*)(p + req_len);
    FETCH_HW(rsp_len, chsc_rsp->length);
    len = (req_len + rsp_len);
    memset(charbuf, '.', 16);

    for (disp=0; disp < len; disp++, p++)
    {
        if (disp && !(disp & 15))
        {
            DUMPLINE;
            STRLCAT( dumpbuf, linebuf );
            hexbuf[0] = 0;
        }
        if (!(disp & 3))
            STRLCAT( hexbuf, " " );
        MSGBUF( hex, "%2.2X", *p );
        STRLCAT( hexbuf, hex );
        c = guest_to_host(*p);
        if (!isprint((unsigned char)c) || iscntrl((unsigned char)c)) c = '.';
        charbuf[disp & 15] = c;
    }

    /* Finish partial last line */
    if (disp & 15)
    {
        for (; disp & 15; disp++)
        {
            if (!(disp & 3))
                STRLCAT( hexbuf, " " );
            STRLCAT( hexbuf, "  " );
            charbuf[disp & 15] = ' ';
        }
    }

    /* Last line */
    DUMPLINE;
    STRLCAT( dumpbuf, linebuf );

    logmsg("CHSC 0x%04X: req_len=0x%04X, rsp_len=0x%04X\n%s",
        req, req_len, rsp_len, dumpbuf);
}
#endif /*defined(CHSC_DEBUG)*/

#endif /*!defined(_GEN_ARCH)*/
