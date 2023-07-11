/* IO.C         (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              I/O Instructions                                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements all I/O instructions of the                */
/* S/370 and ESA/390 architectures, as described in the manuals      */
/* GA22-7000-03 System/370 Principles of Operation                   */
/* SA22-7201-06 ESA/390 Principles of Operation                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      STCPS and SCHM instructions by Jan Jaeger                    */
/*      STCRW instruction by Jan Jaeger                              */
/*      Instruction decode by macros - Jan Jaeger                    */
/*      Instruction decode rework - Jan Jaeger                       */
/*      Correct nullification of TIO and TSCH - Jan Jaeger           */
/*      Lock device during MSCH update - Jan Jaeger                  */
/*      SIE support - Jan Jaeger                                     */
/*      SAL instruction added - Jan Jaeger                           */
/*      RCHP instruction added - Jan Jaeger                          */
/*      CONCS instruction added - Jan Jaeger                         */
/*      DISCS instruction added - Jan Jaeger                         */
/*      TPI fix - Jay Maynard, found by Greg Smith                   */
/*      STCRW instruction nullification correction - Jan Jaeger      */
/*      I/O rate counter - Valery Pogonchenko                        */
/*      64-bit IDAW support - Roger Bowler v209                      */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _IO_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "chsc.h"

#undef PTIO
#undef PTIO_CH

#if defined(FEATURE_CHANNEL_SUBSYSTEM)
 #define PTIO(    _class, _name      ) \
  PTT( PTT_CL_ ## _class, _name, regs->GR_L(1), (U32)(effective_addr2 & 0xffffffff), regs->psw.IA_L )
 #define PTIO_CH( _class, _name, _op ) \
  PTT( PTT_CL_ ## _class, _name, _op, (U32)(effective_addr2 & 0xffffffff), regs->psw.IA_L )
#else
 #define PTIO(    _class, _name      ) \
  PTT( PTT_CL_ ## _class, _name, (U32)(effective_addr2 & 0xffffffff), 0, regs->psw.IA_L )
 #define PTIO_CH( _class, _name, _op ) \
  PTT( PTT_CL_ ## _class, _name, _op, 0, regs->psw.IA_L )
#endif

#if defined( FEATURE_CHANNEL_SUBSYSTEM )

/*-------------------------------------------------------------------*/
/* B230 CSCH  - Clear Subchannel                                 [S] */
/*-------------------------------------------------------------------*/
DEF_INST( clear_subchannel )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */

    S( inst, regs, b2, effective_addr2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    PTIO( IO, "CSCH" );

#if defined( _FEATURE_IO_ASSIST )
    if (SIE_STATE_BIT_OFF( regs, EC0, IOA ) && !regs->sie_pref)
#endif
    {
        PTIO( IO, "CSCH (sie)" );
        SIE_INTERCEPT( regs );
    }

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "CSCH (dev)", dev->devnum, 0, 0 );

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled */
    if (0
        || !dev
        || !(dev->pmcw.flag5 & PMCW5_V)
        || !(dev->pmcw.flag5 & PMCW5_E)
    )
    {
        PTIO( ERR, "*CSCH" );
#if defined( _FEATURE_IO_ASSIST )
        PTIO( ERR, "*CSCH (sie)" );
        SIE_INTERCEPT( regs );
#endif
        regs->psw.cc = 3;
        return;
    }

    /* Perform clear subchannel and set condition code zero */
    clear_subchan( regs, dev );

    regs->psw.cc = 0;
}


/*-------------------------------------------------------------------*/
/* B231 HSCH  - Halt Subchannel                                  [S] */
/*-------------------------------------------------------------------*/
DEF_INST( halt_subchannel )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */

    S( inst, regs, b2, effective_addr2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    PTIO( IO, "HSCH" );

#if defined( _FEATURE_IO_ASSIST )
    if (SIE_STATE_BIT_OFF( regs, EC0, IOA ) && !regs->sie_pref)
#endif
    {
        PTIO( IO, "HSCH (sie)" );
        SIE_INTERCEPT( regs );
    }

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "HSCH (dev)", dev->devnum, 0, 0 );

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled */
    if (0
        || !dev
        || !(dev->pmcw.flag5 & PMCW5_V)
        || !(dev->pmcw.flag5 & PMCW5_E)
    )
    {
        PTIO( ERR, "*HSCH" );
#if defined( _FEATURE_IO_ASSIST )
        PTIO( ERR, "*HSCH (sie)" );
        SIE_INTERCEPT( regs );
#endif
        regs->psw.cc = 3;
        return;
    }

    /* Perform halt subchannel and set condition code */
    if ((regs->psw.cc = halt_subchan( regs, dev )) != 0)
        PTIO( ERR, "*HSCH" );
}


/*-------------------------------------------------------------------*/
/* B232 MSCH  - Modify Subchannel                                [S] */
/*-------------------------------------------------------------------*/
DEF_INST( modify_subchannel )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */
PMCW    pmcw;                           /* Path management ctl word  */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    PTIO( IO, "MSCH" );

    PTIO( IO, "MSCH (sie)" );
    SIE_INTERCEPT( regs );

    FW_CHECK( effective_addr2, regs );

    /* Fetch the updated path management control word */
    ARCH_DEP( vfetchc )( &pmcw, sizeof(PMCW)-1, effective_addr2, b2, regs );

    /* Program check if reserved bits are not zero */
    if (0
        || (pmcw.flag4 & PMCW4_RESV)
        || (pmcw.flag5 & PMCW5_LM) == PMCW5_LM_RESV
#if !defined( _FEATURE_IO_ASSIST )
        || (pmcw.flag4 & PMCW4_A)
        || (pmcw.zone != 0)
        || (pmcw.flag25 & PMCW25_VISC)
        || (pmcw.flag27 & PMCW27_I)
#endif
        || (pmcw.flag26 != 0)
        || (pmcw.flag27 & PMCW27_RESV)
    )
        ARCH_DEP( program_interrupt )( regs, PGM_OPERAND_EXCEPTION );

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "MSCH (dev)", dev->devnum, 0, 0 );

    /* Condition code 3 if subchannel does not exist */
    if (!dev)
    {
        PTIO( ERR, "*MSCH" );
        regs->psw.cc = 3;
        return;
    }

    /* If the subchannel is invalid then return cc0 */
    if (!(dev->pmcw.flag5 & PMCW5_V))
    {
        PTIO( ERR, "*MSCH" );
        regs->psw.cc = 0;
        return;
    }

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Obtain the device lock */
    OBTAIN_DEVLOCK( dev );
    {
        /* Condition code 1 if subchannel is status pending
           with other than intermediate status */
        if (1
            &&  (dev->scsw.flag3 & SCSW3_SC_PEND)
            && !(dev->scsw.flag3 & SCSW3_SC_INTER)
        )
        {
            PTIO( ERR, "*MSCH" );
            regs->psw.cc = 1;
            RELEASE_DEVLOCK( dev );
            return;
        }

        /* Condition code 2 if subchannel is busy */
        if (dev->busy || IOPENDING( dev ))
        {
            PTIO( ERR, "*MSCH" );
            regs->psw.cc = 2;
            RELEASE_DEVLOCK( dev );
            return;
        }

        /* Update the enabled (E), limit mode (LM), and
           measurement mode (MM), and multipath (D) bits */
        dev->pmcw.flag5 &=              ~(PMCW5_E | PMCW5_LM | PMCW5_MM | PMCW5_D);
        dev->pmcw.flag5 |= (pmcw.flag5 & (PMCW5_E | PMCW5_LM | PMCW5_MM | PMCW5_D));

        /* Update the measurement block index */
        memcpy( dev->pmcw.mbi, pmcw.mbi, sizeof( HWORD ));

        /* Update the interruption parameter */
        memcpy( dev->pmcw.intparm, pmcw.intparm, sizeof( FWORD ));

        /* Update the ISC and A fields */
        dev->pmcw.flag4 &=              ~(PMCW4_ISC | PMCW4_A);
        dev->pmcw.flag4 |= (pmcw.flag4 & (PMCW4_ISC | PMCW4_A));

        /* Update the path management (LPM and POM) fields */
        dev->pmcw.lpm = pmcw.lpm;
        dev->pmcw.pom = pmcw.pom;

        /* Update zone, VISC, I and S bit */
        dev->pmcw.zone    =  pmcw.zone;
        dev->pmcw.flag25 &=              ~(PMCW25_VISC);
        dev->pmcw.flag25 |= (pmcw.flag25 & PMCW25_VISC);
        dev->pmcw.flag26  =  pmcw.flag26;
        dev->pmcw.flag27  =  pmcw.flag27;

#if defined( _FEATURE_IO_ASSIST )
        /* Relate the device storage view to the requested zone */
        {
            RADR  mso, msl;

            mso =  sysblk.zpb[ dev->pmcw.zone ].mso << 20;
            msl = (sysblk.zpb[ dev->pmcw.zone ].msl << 20) | 0xFFFFF;

            /* Ensure channel program checks on incorrect zone defs */
            if (0
                || mso > (sysblk.mainsize - 1)
                || msl > (sysblk.mainsize - 1)
                || mso > msl
            )
                mso = msl = 0;

            dev->mainstor = &(sysblk.mainstor[ mso ]);
            dev->mainlim  = msl - mso;
            dev->storkeys = ARCH_DEP( get_ptr_to_storekey )( mso );
        }
#endif

        /* Set device priority from the interruption subclass bits.
         * Note: PMCW priorities range from 0 to 7 with 0 having a higher
         *       priority than 7. Consequently, we'll set the device
         *       priority bits accordingly.
         */
        dev->priority &= 0xFF00FFFFULL;
        dev->priority |= 0x00800000ULL >> ((dev->pmcw.flag4 & PMCW4_ISC) >> 3);

    }
    RELEASE_DEVLOCK( dev );

    /* Set condition code 0 */
    regs->psw.cc = 0;
}


/*-------------------------------------------------------------------*/
/* B23B RCHP  - Reset Channel Path                               [S] */
/*-------------------------------------------------------------------*/
DEF_INST(reset_channel_path)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE    chpid;

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);

    PTIO(IO,"RCHP");

    /* Program check if reg 1 bits 0-23 not zero */
    if(regs->GR_L(1) & 0xFFFFFF00)
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);

    chpid = regs->GR_L(1) & 0xFF;

    if((regs->psw.cc = chp_reset(chpid, 1)) != 0)
    {
        PTIO(ERR,"*RCHP");
        RETURN_INTCHECK(regs);
    }
}


/*-------------------------------------------------------------------*/
/* B238 RSCH  - Resume Subchannel                                [S] */
/*-------------------------------------------------------------------*/
DEF_INST( resume_subchannel )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */

    S( inst, regs, b2, effective_addr2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    PTIO( IO, "RSCH" );

#if defined( _FEATURE_IO_ASSIST )
    if (SIE_STATE_BIT_OFF( regs, EC0, IOA ) && !regs->sie_pref)
#endif
    {
        PTIO( IO, "RSCH (sie)" );
        SIE_INTERCEPT( regs );
    }

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "RSCH (dev)", dev->devnum, 0, 0 );

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled */
    if (0
        || !dev
        || !(dev->pmcw.flag5 & PMCW5_V)
        || !(dev->pmcw.flag5 & PMCW5_E)
    )
    {
        PTIO( ERR, "*RSCH" );
#if defined( _FEATURE_IO_ASSIST )
        PTIO( ERR, "*RSCH (sie)" );
        SIE_INTERCEPT( regs );
#endif
        regs->psw.cc = 3;
        return;
    }

    /* Perform resume subchannel and set condition code */
    if ((regs->psw.cc = resume_subchan( regs, dev )) != 0)
        PTIO( ERR, "*RSCH" );

    regs->siocount++;
}


/*-------------------------------------------------------------------*/
/* B237 SAL   - Set Address Limit                                [S] */
/*-------------------------------------------------------------------*/
DEF_INST(set_address_limit)
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);

    PTIO(IO,"SAL");

    if(regs->GR_L(1) & 0x8000FFFF)
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    else
        sysblk.addrlimval = regs->GR_L(1);
}


/*-------------------------------------------------------------------*/
/* B23C SCHM  - Set Channel Monitor                              [S] */
/*-------------------------------------------------------------------*/
DEF_INST(set_channel_monitor)
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);

#if defined(_FEATURE_IO_ASSIST)
    if(SIE_STATE_BIT_OFF(regs, EC0, IOA) && !regs->sie_pref)
#endif
        SIE_INTERCEPT(regs);

    PTIO(IO,"SCHM");

    /* Reserved bits in gpr1 must be zero */
    if (regs->GR_L(1) & CHM_GPR1_RESV)
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);

    /* Program check if M bit one and gpr2 address not on
       a 32 byte boundary or highorder bit set in ESA/390 mode */
    if ((regs->GR_L(1) & CHM_GPR1_M)
     && (regs->GR_L(2) & CHM_GPR2_RESV))
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);

#if defined(_FEATURE_IO_ASSIST)
    /* Virtual use of I/O Assist features must be intercepted */
    if(SIE_MODE(regs)
      && ( (regs->GR_L(1) & CHM_GPR1_ZONE)
        || (regs->GR_L(1) & CHM_GPR1_A) ))
        SIE_INTERCEPT(regs);

    /* Zone must be a valid zone number */
    if (((regs->GR_L(1) & CHM_GPR1_ZONE) >> 16) >= FEATURE_SIE_MAXZONES)
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);

    if(regs->GR_L(1) & CHM_GPR1_A)
#endif /*defined(_FEATURE_IO_ASSIST)*/
    {
        /* Set the measurement block origin address */
        if (regs->GR_L(1) & CHM_GPR1_M)
        {
            sysblk.mbo = regs->GR(2);
            sysblk.mbk = (regs->GR_L(1) & CHM_GPR1_MBK) >> 24;
            sysblk.mbm = 1;
        }
        else
            sysblk.mbm = 0;

        sysblk.mbd = regs->GR_L(1) & CHM_GPR1_D;

    }
#if defined(_FEATURE_IO_ASSIST)
    else
    {
    int zone = SIE_MODE(regs) ? regs->siebk->zone :
                               ((regs->GR_L(1) & CHM_GPR1_ZONE) >> 16);

        /* Set the measurement block origin address */
        if (regs->GR_L(1) & CHM_GPR1_M)
        {
            sysblk.zpb[zone].mbo = regs->GR(2);
            sysblk.zpb[zone].mbk = (regs->GR_L(1) & CHM_GPR1_MBK) >> 24;
            sysblk.zpb[zone].mbm = 1;
        }
        else
            sysblk.zpb[zone].mbm = 0;

        sysblk.zpb[zone].mbd = regs->GR_L(1) & CHM_GPR1_D;

    }
#endif /*defined(_FEATURE_IO_ASSIST)*/

}


/*-------------------------------------------------------------------*/
/* B233 SSCH  - Start Subchannel                                 [S] */
/*-------------------------------------------------------------------*/
DEF_INST( start_subchannel )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */
ORB     orb;                            /* Operation request block   */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    PTIO( IO, "SSCH" );

#if defined( _FEATURE_IO_ASSIST )
    if (SIE_STATE_BIT_OFF( regs, EC0, IOA ) && !regs->sie_pref)
#endif
    {
        PTIO( IO, "SSCH (sie)" );
        SIE_INTERCEPT( regs );
    }

    FW_CHECK( effective_addr2, regs );

    /* Fetch the operation request block */
    ARCH_DEP( vfetchc )( &orb, sizeof( ORB )-1, effective_addr2, b2, regs );

    /* Program check if reserved bits are not zero */
    if (0
        || orb.flag5 & ORB5_B /* Fiber Channel Extension (FCX) unsupported */
        || orb.flag7 & ORB7_RESV
        || orb.ccwaddr[0] & 0x80
    )
        ARCH_DEP( program_interrupt )( regs, PGM_OPERAND_EXCEPTION );

#if !defined( FEATURE_INCORRECT_LENGTH_INDICATION_SUPPRESSION )
    /* Program check if incorrect length suppression */
    if (orb.flag7 & ORB7_L)
        ARCH_DEP( program_interrupt )( regs, PGM_OPERAND_EXCEPTION );
#endif

#if !defined( FEATURE_MIDAW_FACILITY )
    /* Program check if modified indirect data addressing requested */
    if (orb.flag7 & ORB7_D)
        ARCH_DEP( program_interrupt )( regs, PGM_OPERAND_EXCEPTION );
#endif

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "SSCH (dev)", dev->devnum, 0, 0 );

    /* Condition code 3 if subchannel does not exist,
       is not valid, is not enabled, or no path available */
    if (0
        || !dev
        || !(dev->pmcw.flag5 & PMCW5_V)
        || !(dev->pmcw.flag5 & PMCW5_E)
        || !(orb.lpm & dev->pmcw.pam)
    )
    {
        PTIO( ERR, "*SSCH" );
#if defined( _FEATURE_IO_ASSIST )
        PTIO( ERR, "*SSCH (sie)" );
        SIE_INTERCEPT( regs );
#endif
        regs->psw.cc = 3;
        return;
    }

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Clear the path not operational mask */
    dev->pmcw.pnom = 0;

    /* Copy the logical path mask */
    dev->pmcw.lpm = orb.lpm;

    /* Start the channel program and set the condition code */
    regs->psw.cc = ARCH_DEP( startio )( regs, dev, &orb );

    regs->siocount++;

    /* Set the last path used mask */
    if (regs->psw.cc == 0)
        dev->pmcw.lpum = 0x80;
    else
        PTIO( ERR, "*SSCH" );
}


/*-------------------------------------------------------------------*/
/* B23A STCPS - Store Channel Path Status                        [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_channel_path_status)
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK *dev;                            /* -> device block           */
BYTE    chpid;                          /* CHPID associated w/lpum   */
BYTE    work[32];                       /* Work area                 */
static const BYTE msbn[256] = {         /* Most signif. bit# (0 - 7) */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  */
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, /* 0x00 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* 0x10 */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 0x20 */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 0x30 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x50 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x70 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x80 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x90 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xA0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xB0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xC0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xD0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xE0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xF0 */
};

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);

    PTIO_CH( IO, "STCPS", 0 );

    /* Program check if operand not on 32 byte boundary */
    if ( effective_addr2 & 0x0000001F )
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    memset(work, 0, 32);

    /* Scan DEVBLK chain for busy devices */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        OBTAIN_DEVLOCK( dev );
        {
            if (1
                &&  dev->allocated                     /* Valid DEVBLK   */
                && (dev->pmcw.flag5 & PMCW5_V)         /* Valid device   */
                && (dev->pmcw.flag5 & PMCW5_E)         /* Device enabled */
                && (dev->scsw.flag3 & SCSW3_AC_SCHAC)  /* Subchan active */
                && (dev->scsw.flag3 & SCSW3_AC_DEVAC)  /* Device active  */
               )
            {
                /* Retrieve active CHPID */
                chpid = dev->pmcw.chpid[msbn[dev->pmcw.lpum]];

                /* Update channel path status work area */
                work[chpid/8] |= 0x80 >> (chpid % 8);
            }
        }
        RELEASE_DEVLOCK( dev );
    }

    /* Store channel path status word at operand address */
    ARCH_DEP(vstorec) ( work, 32-1, effective_addr2, b2, regs );
}


/*-------------------------------------------------------------------*/
/* B239 STCRW - Store Channel Report Word                        [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_channel_report_word)
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
U32     crw;                            /* Channel Report Word       */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PTIO(IO,"STCRW");
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);
    FW_CHECK(effective_addr2, regs);

    /* Validate write access to operand before taking any
       pending channel report word off the queue */
    ARCH_DEP(validate_operand) (effective_addr2, b2, 0, ACCTYPE_WRITE, regs);

    /* Obtain any pending channel report */
    crw = get_next_channel_report_word(regs);

    PTIO_CH( IO, "STCRW crw", crw );

    /* Store channel report word at operand address */
    ARCH_DEP(vstore4) ( crw, effective_addr2, b2, regs );

    /* Indicate if channel report or zeros were stored */
    regs->psw.cc = (crw == 0) ? 1 : 0;

    if (regs->psw.cc != 0)
        PTIO(ERR,"*STCRW");
}


/*-------------------------------------------------------------------*/
/* B234 STSCH - Store Subchannel                                 [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_subchannel )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */
SCHIB   schib;                          /* Subchannel information blk*/

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    PTIO( IO, "STSCH" );
    PTIO( IO, "STSCH (sie)" );
    SIE_INTERCEPT( regs );

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "STSCH (dev)", dev->devnum, 0, 0 );

    /* Set condition code 3 if subchannel does not exist */
    if (!dev)
    {
        PTIO( ERR, "*STSCH" );
        regs->psw.cc = 3;
        return;
    }

    FW_CHECK( effective_addr2, regs );

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Build the subchannel information block */
    schib.pmcw = dev->pmcw;

    OBTAIN_DEVLOCK( dev );
    {
        if (dev->pciscsw.flag3 & SCSW3_SC_PEND)
            schib.scsw = dev->pciscsw;
        else
            schib.scsw = dev->scsw;
    }
    RELEASE_DEVLOCK( dev );

    memset( schib.moddep, 0, sizeof( schib.moddep ));

    /* Store the subchannel information block */
    ARCH_DEP( vstorec )( &schib, sizeof(SCHIB)-1, effective_addr2,
                b2, regs );

    /* Set condition code 0 */
    regs->psw.cc = 0;
}


/*-------------------------------------------------------------------*/
/* B236 TPI   - Test Pending Interruption                        [S] */
/*-------------------------------------------------------------------*/
DEF_INST( test_pending_interruption )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
PSA    *psa;                            /* -> Prefixed storage area  */
U64     dreg;                           /* Double register work area */
U32     ioid;                           /* I/O interruption address  */
U32     ioparm;                         /* I/O interruption parameter*/
U32     iointid;                        /* I/O interruption ident    */
int     icode;                          /* Intercept code            */
RADR    pfx;                            /* Prefix                    */
DEVBLK *dev;                            /* dev presenting interrupt  */

    S( inst, regs, b2, effective_addr2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

#if defined( _FEATURE_IO_ASSIST )
    if (SIE_STATE_BIT_OFF( regs, EC0, IOA ) && !regs->sie_pref)
#endif
       SIE_INTERCEPT( regs );

    PTIO( IO,"TPI" );

    FW_CHECK( effective_addr2, regs );

    /* validate operand before taking any action */
    if (effective_addr2 != 0)
        ARCH_DEP( validate_operand )( effective_addr2, b2, 8-1,
                                      ACCTYPE_WRITE, regs );

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    if (IS_IC_IOPENDING)
    {
        OBTAIN_INTLOCK( regs );
        {
            /* Test and clear pending interrupt
               and set the condition code
            */
            icode = ARCH_DEP( present_io_interrupt )( regs, &ioid, &ioparm,
                                                      &iointid, NULL, &dev );
        }
        RELEASE_INTLOCK( regs );

        /* Store the SSID word and I/O parameter
           if an interrupt was pending
        */
        if (icode)
        {
            if (0
                || effective_addr2 == 0
#if defined( _FEATURE_IO_ASSIST )
                || icode != SIE_NO_INTERCEPT
#endif
            )
            {
#if defined( _FEATURE_IO_ASSIST )
                if (icode != SIE_NO_INTERCEPT)
                {
                    /* Point to SIE copy of PSA in state descriptor */
                    psa = (void*)(HOSTREGS->mainstor + SIE_STATE(regs) + SIE_II_PSA_OFFSET);
                    ARCH_DEP( or_storage_key )( SIE_STATE( regs ), (STORKEY_REF | STORKEY_CHANGE) );
                }
                else
#endif
                {
                    /* Point to PSA in main storage */
                    pfx = regs->PX;
                    SIE_TRANSLATE( &pfx, ACCTYPE_SIE, regs );
                    psa = (void*)(regs->mainstor + pfx);
                    ARCH_DEP( or_storage_key )( pfx, (STORKEY_REF | STORKEY_CHANGE) );
                }

                /* If operand address is zero, store in PSA */
                STORE_FW( psa->ioid,ioid );
                STORE_FW( psa->ioparm,ioparm );
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( _FEATURE_IO_ASSIST )
                STORE_FW( psa->iointid,iointid );
#endif
                /* Do SIE intercept if needed */
#if defined( _FEATURE_IO_ASSIST )
                if (icode != SIE_NO_INTERCEPT)
                    longjmp( regs->progjmp, SIE_INTERCEPT_IOINST );
#endif
            }
            else
            {
                /* Otherwise store at operand location */
                dreg = ((U64)ioid << 32) | ioparm;
                ARCH_DEP( vstore8 )( dreg, effective_addr2, b2, regs );
            }
        }
    }
    else
    {
#if defined( _FEATURE_IO_ASSIST )
        /* If no I/O assisted devices have pending interrupts
           then we must intercept */
        SIE_INTERCEPT( regs );
#endif
        icode = 0;
    }

    regs->psw.cc = (icode == 0) ? 0 : 1;

    if (regs->psw.cc != 0)
        PTIO(ERR,"*TPI");
}


/*-------------------------------------------------------------------*/
/* B235 TSCH  - Test Subchannel                                  [S] */
/*-------------------------------------------------------------------*/
DEF_INST( test_subchannel )
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */
IRB     irb;                            /* Interruption response blk */
int     cc;                             /* Condition Code            */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    PTIO( IO, "TSCH" );

#if defined( _FEATURE_IO_ASSIST )
    if (SIE_STATE_BIT_OFF( regs, EC0, IOA ) && !regs->sie_pref)
#endif
    {
        PTIO( IO, "TSCH (sie)" );
        SIE_INTERCEPT( regs );
    }

    FW_CHECK( effective_addr2, regs );

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "TSCH (dev)", dev->devnum, 0, 0 );

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled */
    if (0
        || !dev
        || !(dev->pmcw.flag5 & PMCW5_V)
        || !(dev->pmcw.flag5 & PMCW5_E)
    )
    {
        PTIO( ERR, "*TSCH" );
#if defined( _FEATURE_IO_ASSIST )
        PTIO( ERR, "*TSCH (sie)" );
        SIE_INTERCEPT( regs );
#endif
        regs->psw.cc = 3;
        return;
    }

    /* validate operand before taking any action */
    ARCH_DEP( validate_operand )( effective_addr2, b2, sizeof( IRB ) - 1,
                                  ACCTYPE_WRITE_SKP, regs );

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Test and clear pending status, set condition code */
    cc = test_subchan( regs, dev, &irb );

    /* Store the interruption response block */
    ARCH_DEP( vstorec )( &irb, sizeof(IRB)-1, effective_addr2, b2, regs );

    regs->psw.cc = cc;

    if (regs->psw.cc != 0)
        PTIO( ERR, "*TSCH" );
}


#if defined( FEATURE_CANCEL_IO_FACILITY )
/*-------------------------------------------------------------------*/
/* B276 XSCH  - Cancel Subchannel                                [S] */
/*-------------------------------------------------------------------*/
DEF_INST( cancel_subchannel )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
DEVBLK* dev;                            /* -> device block           */

    S( inst, regs, b2, effective_addr2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    PTIO( IO, "XSCH" );

#if defined( _FEATURE_IO_ASSIST )
    if (SIE_STATE_BIT_OFF( regs, EC0, IOA ) && !regs->sie_pref)
#endif
    {
        PTIO( IO, "XSCH (sie)" );
        SIE_INTERCEPT( regs );
    }

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK( regs );

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan( regs->GR_L( 1 ));

    if (dev)
        PTT( PTT_CL_IO, "XSCH (dev)", dev->devnum, 0, 0 );

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled */
    if (0
        || !dev
        || !(dev->pmcw.flag5 & PMCW5_V)
        || !(dev->pmcw.flag5 & PMCW5_E)
    )
    {
        PTIO( ERR, "*XSCH" );
#if defined( _FEATURE_IO_ASSIST )
        PTIO( ERR, "*XSCH (sie)" );
        SIE_INTERCEPT( regs );
#endif
        regs->psw.cc = 3;
        return;
    }

    /* Perform cancel subchannel and set condition code */
    regs->psw.cc = cancel_subchan( regs, dev );

    if (regs->psw.cc != 0)
        PTIO( ERR, "*XSCH" );
}
#endif /* defined( FEATURE_CANCEL_IO_FACILITY ) */
#endif /* defined( FEATURE_CHANNEL_SUBSYSTEM ) */


#if defined(FEATURE_S370_CHANNEL)

/*-------------------------------------------------------------------*/
/* 9C00 SIO   - Start I/O                                        [S] */
/* 9C01 SIOF  - Start I/O Fast Release                           [S] */
/* 9C02 RIO   - Resume I/O                                       [S] */
/*-------------------------------------------------------------------*/
DEF_INST(start_io)
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */
DEVBLK *dev;                            /* -> device block for SIO   */
PSA    *psa;                            /* -> prefixed storage area  */
ORB     orb;                            /* Operation request blk     */
VADR    ccwaddr;                        /* CCW address for start I/O */
BYTE    ccwkey;                         /* Bits 0-3=key, 4=suspend   */
                                        /*      5-7=zero             */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );

#if defined(FEATURE_ECPSVM)
    if((inst[1])!=0x02)
    {
        if(ecpsvm_dosio(regs,b2,effective_addr2)==0)
        {
            return;
        }
    }
#endif

    PRIV_CHECK(regs);

    SIE_INTERCEPT(regs);

    PTIO(IO,"SIO");

    /* Locate the device block */
    if(regs->chanset == 0xFFFF ||
       !(dev = find_device_by_devnum (regs->chanset,effective_addr2)))
    {
        PTIO(ERR,"*SIO");
        regs->psw.cc = 3;
        return;
    }

    /* If CSW pending, drain interrupt and present the CSW */
    if (IOPENDING(dev) &&
        testio(regs, dev, inst[1]) == 1)
        regs->psw.cc = 1;

    /* Else, if RIO, resume the subchannel operation */
    else if (inst[1] == 0x02)
        regs->psw.cc = resume_subchan (regs, dev);

    /* Otherwise, start the channel program and set the condition code
     */
    else
    {
        /* Fetch key and CCW address from the CAW at PSA+X'48' */
        psa = (PSA*)(regs->mainstor + regs->PX);
        ccwkey = psa->caw[0] & 0xF0;
        ccwaddr = (psa->caw[1] << 16) |
                  (psa->caw[2] <<  8) |
                   psa->caw[3];

        /* Build the I/O operation request block */
        memset (&orb, 0, sizeof(ORB));
        orb.flag4 = ccwkey & (ORB4_KEY | ORB4_S);
        STORE_FW(orb.ccwaddr,ccwaddr);

        /* Indicate if CPU is to begin execution in S/360 or S/370 SIO
         * mode, not releasing the CPU until the first CCW has been
         * validated
         */
        dev->s370start = (inst[1] == 0x00 ||
                          (inst[1] == 0x01 &&
                           (dev->chptype[0] == CHP_TYPE_UNDEF ||
                            dev->chptype[0] == CHP_TYPE_BYTE)));

        /* Go start the I/O operation */
        regs->psw.cc = ARCH_DEP(startio) (regs, dev, &orb);
    }

    if (regs->psw.cc > 1)
        PTIO(ERR,"*SIO");

    regs->siocount++;
}


/*-------------------------------------------------------------------*/
/* 9D00 TIO   - Test I/O                                         [S] */
/* 9D01 CLRIO - Clear I/O                                        [S] */
/*-------------------------------------------------------------------*/
DEF_INST(test_io)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
DEVBLK *dev;                            /* -> device block for SIO   */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);
    PTIO(IO,"TIO");

    /* Locate the device block */
    if(regs->chanset == 0xFFFF
      || !(dev = find_device_by_devnum (regs->chanset,effective_addr2)) )
    {
        PTIO(ERR,"*TIO");
        regs->psw.cc = 3;
        return;
    }

    /* Test the device and set the condition code */
    regs->psw.cc = testio (regs, dev, inst[1]);

    if (regs->psw.cc != 0)
        PTIO(ERR,"*TIO");

    /* Yield time slice so that device handler may get some time */
    /* to possibly complete an I/O - to prevent a TIO Busy Loop  */
    if(regs->psw.cc == 2)
    {
        sched_yield();
    }
}


/*-------------------------------------------------------------------*/
/* 9E00 HIO   - Halt I/O                                         [S] */
/* 9E01 HDV   - Halt Device                                      [S] */
/*-------------------------------------------------------------------*/
DEF_INST(halt_io)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
DEVBLK *dev;                            /* -> device block for SIO   */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);
    PTIO(IO,"HIO");

    /* Locate the device block */
    if(regs->chanset == 0xFFFF
      || !(dev = find_device_by_devnum (regs->chanset,effective_addr2)) )
    {
        PTIO(ERR,"*HIO");
        regs->psw.cc = 3;
        return;
    }

    /* Test the device and set the condition code */
    regs->psw.cc = haltio (regs, dev, inst[1]);

    if (regs->psw.cc != 0)
        PTIO(ERR,"*HIO");
}


/*-------------------------------------------------------------------*/
/* 9F00 TCH   - Test Channel                                     [S] */
/*-------------------------------------------------------------------*/
DEF_INST(test_channel)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
#if defined(_FEATURE_SIE)
BYTE    channelid;
U16     tch_ctl;
#endif /*defined(_FEATURE_SIE)*/

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    PTIO(IO,"TCH");

#if defined(_FEATURE_SIE)
    if(!SIE_MODE(regs))
    {
#endif /*defined(_FEATURE_SIE)*/
        /* Test for pending interrupt and set condition code */
        regs->psw.cc = testch (regs, effective_addr2 & 0xFF00);
#if defined(_FEATURE_SIE)
    }
    else
    {
        channelid = (effective_addr2 >> 8) & 0xFF;
        FETCH_HW(tch_ctl,((SIEBK*)(regs->siebk))->tch_ctl);
        if((channelid > 15)
         || ((0x8000 >> channelid) & tch_ctl))
            longjmp(regs->progjmp, SIE_INTERCEPT_INST);
        else
            regs->psw.cc = 0;
    }
#endif /*defined(_FEATURE_SIE)*/

    if (regs->psw.cc != 0)
        PTIO(ERR,"*TCH");
}


/*-------------------------------------------------------------------*/
/* B203 STIDC - Store Channel ID                                 [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_channel_id)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);
    PTIO(IO,"STIDC");

    /* Store Channel ID and set condition code */
    regs->psw.cc =
        stchan_id (regs, effective_addr2 & 0xFF00);

    if (regs->psw.cc != 0)
        PTIO(ERR,"*STIDC");
}


#if defined(FEATURE_CHANNEL_SWITCHING)
/*-------------------------------------------------------------------*/
/* B200 CONCS - Connect Channel Set                              [S] */
/*-------------------------------------------------------------------*/
DEF_INST(connect_channel_set)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i;

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);
    PTIO(IO,"CONCS");

    effective_addr2 &= 0xFFFF;

    /* Hercules has as many channelsets as CSS's */
    if(effective_addr2 >= FEATURE_LCSS_MAX)
    {
        PTIO(ERR,"*CONCS");
        regs->psw.cc = 3;
        return;
    }

    /* If the addressed channel set is currently connected
       then return with cc0 */
    if(regs->chanset == effective_addr2)
    {
        regs->psw.cc = 0;
        return;
    }

    /* Disconnect channel set */
    regs->chanset = 0xFFFF;

    OBTAIN_INTLOCK(regs);

    /* If the addressed channelset is connected to another
       CPU then return with cc1 */
    for (i = 0; i < sysblk.maxcpu; i++)
    {
        if (IS_CPU_ONLINE(i)
         && sysblk.regs[i]->chanset == effective_addr2)
        {
            RELEASE_INTLOCK(regs);
            regs->psw.cc = 1;
            return;
        }
    }

    /* Set channel set connected to current CPU */
    regs->chanset = effective_addr2;

    /* Interrupts may be pending on this channelset */
    ON_IC_IOPENDING;

    RELEASE_INTLOCK(regs);

    regs->psw.cc = 0;

}


/*-------------------------------------------------------------------*/
/* B201 DISCS - Disconnect Channel Set                           [S] */
/*-------------------------------------------------------------------*/
DEF_INST(disconnect_channel_set)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i;

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);
    SIE_INTERCEPT(regs);
    PTIO(IO,"DISCS");

    /* Hercules has as many channelsets as CSS's */
    if(effective_addr2 >= FEATURE_LCSS_MAX)
    {
        PTIO(ERR,"*DISCS");
        regs->psw.cc = 3;
        return;
    }

    /* If the addressed channel set is currently connected
       then disconect channel set and return with cc0 */
    if(regs->chanset == effective_addr2 && regs->chanset != 0xFFFF)
    {
        regs->chanset = 0xFFFF;
        regs->psw.cc = 0;
        return;
    }

    OBTAIN_INTLOCK(regs);

    /* If the addressed channelset is connected to another
       CPU then return with cc0 */
    for(i = 0; i < sysblk.maxcpu; i++)
    {
        if (IS_CPU_ONLINE(i)
         && sysblk.regs[i]->chanset == effective_addr2)
        {
            if(sysblk.regs[i]->cpustate != CPUSTATE_STARTED)
            {
                sysblk.regs[i]->chanset = 0xFFFF;
                regs->psw.cc = 0;
            }
            else
            {
                regs->psw.cc = 1;
                PTIO(ERR,"*DISCS");
            }
            RELEASE_INTLOCK(regs);
            return;
        }
    }

    RELEASE_INTLOCK(regs);

    /* The channel set is not connected, no operation
       is performed */
    regs->psw.cc = 0;
}
#endif /*defined(FEATURE_CHANNEL_SWITCHING)*/

#endif /*defined(FEATURE_S370_CHANNEL)*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "io.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "io.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
