/* IPL.C        (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2024                             */
/*              ESA/390 Initial Program Loader                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */
/*                                                                   */
/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements the Initial Program Load (IPL) function of */
/* the S/370, ESA/390 and z/Architectures, described in the manuals: */
/*                                                                   */
/*     GA22-7000    System/370 Principles of Operation               */
/*     SA22-7201    ESA/390 Principles of Operation                  */
/*     SA22-7832    z/Architecture Principles of Operation.          */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _IPL_C
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#ifdef FEATURE_076_MSA_EXTENSION_FACILITY_3
#include "hexterns.h"
#endif

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

// Note: non-arch-dep static functions should ideally be at the end
// but you can put them here by using an ifdef similar to the below

/*-------------------------------------------------------------------*/
/* Function to reset instruction count and CPU time                  */
/*-------------------------------------------------------------------*/
#ifndef _reset_instcount_
#define _reset_instcount_

static INLINE void cpu_reset_instcount_and_cputime( REGS* regs )
{
    /* Reset instruction counts, I/O counts and real CPU time */
    regs->prevcount = 0;
    regs->instcount = 0;
    regs->mipsrate  = 0;
    regs->siocount  = 0;
    regs->siosrate  = 0;
    regs->siototal  = 0;
    regs->cpupct    = 0;
    regs->rcputime  = 0;
    regs->bcputime  = thread_cputime_us( regs );
}
#endif // _reset_instcount_

/*-------------------------------------------------------------------*/
/* Function to perform Subystem Reset                                */
/*-------------------------------------------------------------------*/
/* Locks held on entry/exit:                                         */
/*  sysblk.intlock                                                   */
/*-------------------------------------------------------------------*/
#ifndef _subsystem_reset_
#define _subsystem_reset_

static INLINE void subsystem_reset()
{
    /* Perform subsystem reset
     *
     * GA22-7000-10 IBM System/370 Principles of Operation, Chapter 4.
     *              Control, Subsystem Reset, p. 4-34
     * SA22-7085-00 IBM System/370 Extended Architecture Principles of
     *              Operation, Chapter 4. Control, Subsystem Reset,
     *              p. 4-28
     * SA22-7832-09 z/Architecture Principles of Operation, Chapter 4.
     *              Control, Subsystem Reset, p. 4-57
     */

    /* Clear pending external interrupts */
    OFF_IC_SERVSIG;
    OFF_IC_INTKEY;

    /* Reset the I/O subsystem */
    RELEASE_INTLOCK( NULL );
    {
        io_reset ();
    }
    OBTAIN_INTLOCK( NULL );
}
#endif // _subsystem_reset_

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/* Function to perform System Reset   (either 'normal' or 'clear')   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( system_reset )( const int target_mode, const bool clear,
                              const bool ipl, const int cpu )
{
    int         rc;
    int         n;
    int         regs_mode;
    int         architecture_switch;
    REGS*       regs;
    CPU_BITMAP  mask;

    /* Configure the CPU if it's not online yet.
     * Note: Configure implies initial reset.
     */
    if (!IS_CPU_ONLINE( cpu ))
    {
        sysblk.arch_mode = target_mode;

        if ((rc = configure_cpu( cpu )))
            return rc;
    }

    /* HercGUI hook so it can update its LEDs */
    HDC1( debug_cpu_state, sysblk.regs[ cpu ] );

    /* Determine the target architecture mode for reset.
     *
     * A system reset normal never changes the architecture mode,
     * nor does a system reset clear or IPL for architectures
     * other than z/Architecture.  For z/Architecture however,
     * a system reset clear or IPL changes the architecture mode
     * to ESA/390.
     */
    if (target_mode > ARCH_390_IDX && (clear || ipl))
        regs_mode = ARCH_390_IDX;
    else
        regs_mode = target_mode;

    /* Remember for later whether this is an architecture switch */
    architecture_switch = (regs_mode != sysblk.arch_mode);

    /* Signal all CPUs in configuration to stop and reset */
    {
        /* Switch lock context to hold both sigplock and intlock */
        RELEASE_INTLOCK( NULL );
        {
            obtain_lock( &sysblk.sigplock );
        }
        OBTAIN_INTLOCK( NULL );

        /* Ensure no external updates pending */
        OFF_IC_SERVSIG;
        OFF_IC_INTKEY;

        /* Loop through CPUs and issue appropriate CPU reset function */

        mask = sysblk.config_mask;

        for (n = 0; mask; mask >>= 1, ++n)
        {
            if (mask & 1)
            {
                regs = sysblk.regs[n];

                /* Signal CPU reset function: if requesting CPU
                 * with CLEAR or architecture change, then signal
                 * initial CPU reset.  Otherwise, signal a normal
                 * CPU reset.
                 */
                if (0
                    || architecture_switch
                    || (n == cpu && (clear || ipl))
                )
                    regs->sigp_ini_reset = TRUE;
                else
                    regs->sigp_reset = TRUE;

                regs->opinterv = TRUE;
                regs->cpustate = CPUSTATE_STOPPING;
                ON_IC_INTERRUPT( regs );
                WAKEUP_CPU( regs );
            }
        }

        /* Return to hold of just intlock */
        RELEASE_INTLOCK( NULL );
        {
            release_lock( &sysblk.sigplock );
        }
        OBTAIN_INTLOCK( NULL );
    }

    /* Wait for CPUs to complete their resets */
    {
        int i;
        bool wait = true;

        for (n = 0; wait && n < 300; ++n)
        {
            mask = sysblk.config_mask;
            wait = false;

            for (i=0; mask; mask >>= 1, ++i)
            {
                if (!(mask & 1))
                    continue;

                regs = sysblk.regs[ i ];

                if (regs->cpustate != CPUSTATE_STOPPED)
                {
                    wait = true;

                    /* Release intlock, take a nap, and re-acquire */
                    RELEASE_INTLOCK( NULL );
                    {
                        USLEEP( 10000 );  // (wait 10 milliseconds)
                    }
                    OBTAIN_INTLOCK( NULL );
                }
            }
        }
    }

    /* FIXME: Recovery code is needed to handle the case where
     * CPUs are misbehaving. Outstanding locks should be reported,
     * then take-over CPUs and perform initial reset of each CPU.
     */
    if (n >= 300)   // (more than 300 * 10 milliseconds == 3 seconds?)
    {
        // "DBG: %s"
        WRMSG( HHC90000, "E", "Could not perform reset within three seconds" );
    }

    /* Clear Crypto Wrapping Keys. We do this regardless of whether
       the facility is enabled for the given architecture or not
       since there is no real harm in always doing so. Note too that
       we only do this when the architecture is NOT being switched
       so that it only gets done once and not twice since it doesn't
       matter what the current architecture mode is since the crypto
       wrapping keys aren't associated with any given architecture.
    */
#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 ) // (underscore!)
    if (clear && !architecture_switch)
        renew_wrapping_keys();
#endif

    /* Finish the reset in the requested mode if switching architectures */
    if (architecture_switch)
    {
        sysblk.arch_mode = regs_mode;
        return ARCH_DEP( system_reset )( target_mode, clear, ipl, cpu );
    }

    /* Perform subsystem reset
     *
     * GA22-7000-10 IBM System/370 Principles of Operation, Chapter 4.
     *              Control, Subsystem Reset, p. 4-34
     * SA22-7085-00 IBM System/370 Extended Architecture Principles of
     *              Operation, Chapter 4. Control, Subsystem Reset,
     *              p. 4-28
     * SA22-7832-09 z/Architecture Principles of Operation, Chapter 4.
     *              Control, Subsystem Reset, p. 4-57
     */
    subsystem_reset();

    /* Perform system-reset-clear additional functions */
    if (clear)
    {
        /* Finish reset-clear of all CPUs in the configuration */
        for (n = 0; n < sysblk.maxcpu; ++n)
        {
            if (IS_CPU_ONLINE( n ))
            {
                regs = sysblk.regs[ n ];

                /* Clear all the registers (AR, GPR, FPR, VR)
                 * as part of the CPU CLEAR RESET operation
                 */
                memset( regs->ar,  0, sizeof( regs->ar  ));
                memset( regs->gr,  0, sizeof( regs->gr  ));
                memset( regs->vfp, 0, sizeof( regs->vfp ));

#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
                memset( regs->vf->vr, 0, sizeof( regs->vf->vr ));
#endif

                /* Clear the instruction counter and CPU time used */
                cpu_reset_instcount_and_cputime( regs );
            }
        }

        /* Clear storage */
        sysblk.main_clear = sysblk.xpnd_clear = 0;
        storage_clear();
        xstorage_clear();

        /* Clear IPL program parameter */
        sysblk.program_parameter = 0;
    }

    /* If IPL call, reset CPU instruction counts and times */
    else if (ipl)
    {
        CPU_BITMAP  mask  = sysblk.config_mask;

        for (n=0; mask; mask >>= 1, ++n)
        {
            if (mask & 0x01)
                cpu_reset_instcount_and_cputime( sysblk.regs[ n ]);
        }
    }

    /* If IPL or system-reset-clear, clear the system
     * instruction counter, rates, and IPLed indicator.
     */
    if (clear || ipl)
    {
        /* Clear system instruction counter and CPU rates */
        sysblk.instcount = 0;
        sysblk.mipsrate  = 0;
        sysblk.siosrate  = 0;
        sysblk.ipled     = FALSE;
    }

    /* Set horizontal polarization and clear the
       topology-change-report-pending condition.
    */
#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
    sysblk.topology = TOPOLOGY_HORIZ;
    sysblk.topchnge = 0;
#endif

    /* Set the system state to "reset" */
    sysblk.sys_reset = TRUE;

    return 0;
} /* end function system_reset */

/*-------------------------------------------------------------------*/
/*                  LOAD (aka IPL) functions...                      */
/*-------------------------------------------------------------------*/
/*  Performing an Initial Program Load (aka IPL) involves three      */
/*  distinct phases: in phase 1 the system is reset (registers       */
/*  and, for load-clear, storage), and in phase two the actual       */
/*  Initial Program Loading from the IPL device takes place. Finally,*/
/*  in phase three, the IPL PSW is loaded and the CPU is started.    */
/*-------------------------------------------------------------------*/

PSW     captured_zpsw;                  /* Captured z/Arch PSW       */

/*-------------------------------------------------------------------*/
/* Common LOAD (IPL) begin: system-reset (register/storage clearing) */
/*-------------------------------------------------------------------*/
int ARCH_DEP( common_load_begin )( int cpu, int clear )
{
    const bool ipl = true;
    int target_mode;
    int capture;
    int rc;

    /* Ensure dummyregs archmode matches SYSBLK archmode */
    sysblk.dummyregs.arch_mode = sysblk.arch_mode;

    capture = TRUE
        && !clear
        && IS_CPU_ONLINE( cpu )
        && sysblk.arch_mode == ARCH_900_IDX
        ;

    /* Capture the z/Arch PSW if this is a Load-normal IPL */
    if (capture)
        captured_zpsw = sysblk.regs[ cpu ]->psw;

    /* Perform system-reset-normal or system-reset-clear function.
     *
     * SA22-7085-0 IBM System/370 Extended Architecture Principles
     *             of Operation, Chapter 12, Operator Facilities,
     *             LOAD-CLEAR KEY and LOAD-NORMAL KEY, p. 12-3.
     */

    target_mode = sysblk.arch_mode > ARCH_390_IDX ?
                                     ARCH_390_IDX : sysblk.arch_mode;

    if ((rc = ARCH_DEP( system_reset )( target_mode, clear, ipl, cpu )) != 0)
        return rc;

    /* Save our captured-z/Arch-PSW if this is a Load-normal IPL
       since the initial_cpu_reset call cleared it to zero. */
    if (capture)
        sysblk.regs[ cpu ]->captured_zpsw = captured_zpsw;

    /* The actual IPL (load) now begins... */
    sysblk.regs[ cpu ]->loadstate = TRUE;

    return 0;
} /* end function common_load_begin */

/*-------------------------------------------------------------------*/
/* Function to run initial CCW chain from IPL device and load IPLPSW */
/* Returns 0 if successful, -1 if error                              */
/* intlock MUST be held on entry                                     */
/*-------------------------------------------------------------------*/
int ARCH_DEP( load_ipl )( U16 lcss, U16 devnum, int cpu, int clear )
{
REGS   *regs;                           /* -> Regs                   */
DEVBLK *dev;                            /* -> Device control block   */
int     i;                              /* Array subscript           */
BYTE    unitstat;                       /* IPL device unit status    */
BYTE    chanstat;                       /* IPL device channel status */
int rc;

    /* Get started */
    if ((rc = ARCH_DEP( common_load_begin )( cpu, clear )))
        return rc;

    /* Ensure CPU is online */
    if (!IS_CPU_ONLINE(cpu))
    {
        char buf[80];
        MSGBUF(buf, "CP%2.2X Offline", devnum);
        // "Processor %s%02X: ipl failed: %s"
        WRMSG (HHC00810, "E", PTYPSTR(sysblk.pcpu), sysblk.pcpu, buf);
        return -1;
    }

    /* The actual IPL proper starts here... */

    regs = sysblk.regs[cpu];    /* Point to IPL CPU's registers */
    /* Point to the device block for the IPL device */
    dev = find_device_by_devnum (lcss,devnum);
    if (dev == NULL)
    {
        char buf[80];
        MSGBUF( buf, "device %1d:%04X not found", lcss, devnum );
        // "Processor %s%02X: ipl failed: %s"
        WRMSG( HHC00810, "E", PTYPSTR( sysblk.pcpu ), sysblk.pcpu, buf );

        /* HercGUI hook so it can update its LEDs */
        HDC1( debug_cpu_state, regs );

        return -1;
    }

    if(sysblk.haveiplparm)
    {
        for(i=0;i<16;i++)
        {
            regs->GR_L(i)=fetch_fw(&sysblk.iplparmstring[i*4]);
        }
        sysblk.haveiplparm=0;
    }

    /* Set Main Storage Reference and Update bits */
    ARCH_DEP( or_storage_key )( regs->PX, (STORKEY_REF | STORKEY_CHANGE) );
    sysblk.main_clear = sysblk.xpnd_clear = 0;

    /* Build the IPL CCW at location 0 */
    regs->psa->iplpsw[0] = 0x02;              /* CCW command = Read */
    regs->psa->iplpsw[1] = 0;                 /* Data address = zero */
    regs->psa->iplpsw[2] = 0;
    regs->psa->iplpsw[3] = 0;
    regs->psa->iplpsw[4] = CCW_FLAGS_CC | CCW_FLAGS_SLI;
                                        /* CCW flags */
    regs->psa->iplpsw[5] = 0;                 /* Reserved byte */
    regs->psa->iplpsw[6] = 0;                 /* Byte count = 24 */
    regs->psa->iplpsw[7] = 24;

    /* Enable the subchannel for the IPL device */
    dev->pmcw.flag5 |= PMCW5_E;

    /* Build the operation request block */
    memset (&dev->orb, 0, sizeof(ORB));
    dev->busy = 1;

    RELEASE_INTLOCK(NULL);

    /* Execute the IPL channel program */
    ARCH_DEP(execute_ccw_chain) (dev);

    OBTAIN_INTLOCK(NULL);

    /* Clear the interrupt pending and device busy conditions */
    OBTAIN_IOINTQLK();
    {
        DEQUEUE_IO_INTERRUPT_QLOCKED(&dev->ioint);
        DEQUEUE_IO_INTERRUPT_QLOCKED(&dev->pciioint);
        DEQUEUE_IO_INTERRUPT_QLOCKED(&dev->attnioint);
    }
    RELEASE_IOINTQLK();

    dev->busy = 0;
    dev->scsw.flag2 = 0;
    dev->scsw.flag3 = 0;

    /* Check that load completed normally */
    unitstat = dev->scsw.unitstat;
    chanstat = dev->scsw.chanstat;

    if (unitstat != (CSW_CE | CSW_DE) || chanstat != 0)
    {
        char buf[80];
        char buf2[16];

        memset(buf,0,sizeof(buf));
        for (i=0; i < (int)dev->numsense; i++)
        {
            MSGBUF(buf2, "%2.2X", dev->sense[i]);
            STRLCAT( buf, buf2 );
            if ((i & 3) == 3) STRLCAT( buf, " " );
        }
        {
            char buffer[256];
            MSGBUF(buffer, "architecture mode %s, csw status %2.2X%2.2X, sense %s",
                get_arch_name( NULL ),
                unitstat, chanstat, buf);
            WRMSG (HHC00828, "E", PTYPSTR(sysblk.pcpu), sysblk.pcpu, buffer);
        }

        /* HercGUI hook so it can update its LEDs */
        HDC1( debug_cpu_state, regs );

        return -1;
    }

#ifdef FEATURE_S370_CHANNEL
    /* Test the EC mode bit in the IPL PSW */
    if (regs->psa->iplpsw[1] & 0x08) {
        /* In EC mode, store device address at locations 184-187 */
        STORE_FW(regs->psa->ioid, dev->devnum);
    } else {
        /* In BC mode, store device address at locations 2-3 */
        STORE_HW(regs->psa->iplpsw + 2, dev->devnum);
    }
#endif /*FEATURE_S370_CHANNEL*/

#ifdef FEATURE_CHANNEL_SUBSYSTEM
    /* Set LPUM */
    dev->pmcw.lpum = 0x80;
    STORE_FW(regs->psa->ioid, (dev->ssid<<16)|dev->subchan);

    /* Store zeroes at locations 188-191 */
    memset (regs->psa->ioparm, 0, 4);
#endif /*FEATURE_CHANNEL_SUBSYSTEM*/

    /* Save IPL device number, cpu number and lcss */
    sysblk.ipldev = devnum;
    sysblk.iplcpu = regs->cpuad;
    sysblk.ipllcss = lcss;
    sysblk.ipled = TRUE;

    /* Finish up... */
    return ARCH_DEP(common_load_finish) (regs);
} /* end function load_ipl */

/*-------------------------------------------------------------------*/
/* Common LOAD (IPL) finish: load IPL PSW and start CPU              */
/*-------------------------------------------------------------------*/
int ARCH_DEP(common_load_finish) (REGS *regs)
{
int rc;
    /* Zeroize the interrupt code in the PSW */
    regs->psw.intcode = 0;

    /* Load IPL PSW from PSA+X'0' */
    if ((rc = ARCH_DEP(load_psw) (regs, regs->psa->iplpsw)) )
    {
        char buf[80];
        MSGBUF(buf, "architecture mode %s, invalid ipl psw %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                get_arch_name( NULL ),
                regs->psa->iplpsw[0], regs->psa->iplpsw[1],
                regs->psa->iplpsw[2], regs->psa->iplpsw[3],
                regs->psa->iplpsw[4], regs->psa->iplpsw[5],
                regs->psa->iplpsw[6], regs->psa->iplpsw[7]);
        WRMSG (HHC00839, "E", PTYPSTR(sysblk.pcpu), sysblk.pcpu, buf);

        /* HercGUI hook so it can update its LEDs */
        HDC1( debug_cpu_state, regs );

        return rc;
    }

    /* Set the CPU into the started state */
    regs->opinterv = 0;
    regs->cpustate = CPUSTATE_STARTED;

    /* The actual IPL (load) is now completed... */
    regs->loadstate = 0;

    /* reset sys_reset flag to indicate a active machine */
    sysblk.sys_reset = FALSE;

    /* Signal the CPU to retest stopped indicator */
    WAKEUP_CPU (regs);

    /* HercGUI hook so it can update its LEDs */
    HDC1( debug_cpu_state, regs );

    return 0;
} /* end function common_load_finish */

/*-------------------------------------------------------------------*/
/* Function to perform CPU Reset                                     */
/*-------------------------------------------------------------------*/
int ARCH_DEP(cpu_reset) (REGS *regs)
{
int i, rc = 0;                          /* Array subscript           */

    regs->ip = regs->inst;

    /* Clear indicators */
    regs->loadstate = 0;
    regs->checkstop = 0;
    regs->sigp_reset = 0;
    regs->extccpu = 0;
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    PTT_TXF( "TXF CPURES", 0, 0, regs->txf_tnd );
    /* EXIT SILENTLY from transactional execution mode */
    regs->txf_tnd = 0;
    regs->txf_aborts = 0;
    regs->txf_contran = false;
    regs->txf_UPGM_abort = false;
#endif
    for (i = 0; i < sysblk.maxcpu; i++)
        regs->emercpu[i] = 0;
    regs->instinvalid = 1;

    /* Clear interrupts */
    SET_IC_INITIAL_MASK(regs);
    SET_IC_INITIAL_STATE(regs);

    /* Clear the translation exception identification */
    regs->EA_G = 0;
    regs->excarid = 0;

    /* Clear monitor code */
    regs->MC_G = 0;

    /* Purge the lookaside buffers */
    ARCH_DEP(purge_tlb) (regs);

#if defined(FEATURE_ACCESS_REGISTERS)
    ARCH_DEP(purge_alb) (regs);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/

    if(regs->host)
    {
        /* Put the CPU into the stopped state */
        regs->opinterv = 0;
        regs->cpustate = CPUSTATE_STOPPED;
        ON_IC_INTERRUPT(regs);
    }

#ifdef FEATURE_INTERVAL_TIMER
    ARCH_DEP( store_int_timer_locked )( regs );
#endif

    if (regs->host && GUESTREGS)
    {
        rc = cpu_reset( GUESTREGS );

        /* CPU state of SIE copy cannot be controlled */
        GUESTREGS->opinterv = 0;
        GUESTREGS->cpustate = CPUSTATE_STARTED;
    }

    /* Re-initialize the facilities list for this CPU */
    init_cpu_facilities( regs );

    /* Ensure CPU ID is accurate in case archmode changed */
    setCpuIdregs( regs, -1, -1, -1, -1, true );

   return rc;
} /* end function cpu_reset */

/*-------------------------------------------------------------------*/
/* Function to perform Initial CPU Reset                             */
/*-------------------------------------------------------------------*/
int ARCH_DEP( initial_cpu_reset )( REGS* regs )
{
    int rc = 0;

    /* Clear reset pending indicators */
    regs->sigp_ini_reset = regs->sigp_reset = 0;

    /* Clear the registers */
    memset ( &regs->psw,           0, sizeof( regs->psw           ));
    memset ( &regs->captured_zpsw, 0, sizeof( regs->captured_zpsw ));
    memset ( &regs->cr_struct,     0, sizeof( regs->cr_struct     ));
    regs->fpc    = 0;
    regs->PX     = 0;
    regs->psw.AMASK_G = AMASK24;

    /* Ensure memory sizes are properly indicated */
    regs->mainstor = sysblk.mainstor;
    regs->storkeys = sysblk.storkeys;
    regs->mainlim  = sysblk.mainsize ? (sysblk.mainsize - 1) : 0;
    regs->psa      = (PSA_3XX*)regs->mainstor;

    /* Perform a CPU reset (after setting PSA) */
    rc = ARCH_DEP( cpu_reset )( regs );

    regs->todpr  = 0;
    regs->clkc   = 0;
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    PTT_TXF( "TXF ICPURES", 0, 0, regs->txf_tnd );
    /* EXIT SILENTLY from transactional execution mode */
    regs->txf_tnd = 0;
    regs->txf_aborts = 0;
    regs->txf_contran = false;
    regs->txf_UPGM_abort = false;
#endif
    set_cpu_timer( regs, 0 );
#if defined( _FEATURE_INTERVAL_TIMER )
    set_int_timer( regs, 0 );
#endif

    /* The breaking event address register is initialised to 1 */
    regs->bear = 1;

    /* Initialize external interrupt masks in control register 0 */
    regs->CR(0) = CR0_XM_INTKEY | CR0_XM_EXTSIG |
      (FACILITY_ENABLED( HERC_INTERVAL_TIMER, regs ) ? CR0_XM_ITIMER : 0);

#if defined( FEATURE_S370_CHANNEL ) && !defined( FEATURE_ACCESS_REGISTERS )
    /* For S/370 initialize the channel masks in CR2 */
    regs->CR(2) = (U32)0xFFFFFFFFF;
#endif
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    regs->CR(2) &= ~(CR2_TDS | CR2_TDC);
#endif

    regs->chanset =
#if defined( FEATURE_CHANNEL_SWITCHING )
        regs->cpuad < FEATURE_LCSS_MAX ? regs->cpuad :
#endif
        0xFFFF;

    /* Initialize the machine check masks in control register 14 */
    regs->CR(14) = CR14_CHKSTOP | CR14_SYNCMCEL | CR14_XDMGRPT;

#if !defined( FEATURE_LINKAGE_STACK )
    /* For S/370 initialize the MCEL address in CR15 */
    regs->CR(15) = 512;
#endif

    if (regs->host && GUESTREGS)
    {
        int rc2 = initial_cpu_reset( GUESTREGS );
        if (rc2 != 0)
            rc = rc2;
    }

    return rc;
} /* end function initial_cpu_reset */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "ipl.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "ipl.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

//-------------------------------------------------------------------
//                      _FEATURE_XXX code
//-------------------------------------------------------------------
// Place any _FEATURE_XXX depdendent functions (WITH the underscore)
// here. You may need to define such functions whenever one or more
// build architectures has a given FEATURE_XXX (WITHOUT underscore)
// defined for it. The underscore means AT LEAST ONE of the build
// architectures #defined that feature. (See featchk.h) You must NOT
// use any #ifdef FEATURE_XXX here. Test for ONLY for _FEATURE_XXX.
// The functions in this area are compiled ONCE (only ONE time) and
// ONLY one time but are always compiled LAST after everything else.
//-------------------------------------------------------------------

/*********************************************************************/
/*                  IMPORTANT PROGRAMMING NOTE                       */
/*********************************************************************/
/*                                                                   */
/* It is CRITICALLY IMPORTANT to not use any architecture dependent  */
/* macros anywhere in any of your non-arch_dep functions. This means */
/* you CANNOT use GREG, RADR, VADR, etc. anywhere in your function,  */
/* nor can you call "ARCH_DEP(func)(args)" anywhere in your code!    */
/*                                                                   */
/* Basically you MUST NOT use any architecture dependent macro that  */
/* is #defined in the "feature.h" header.  If you you need to use    */
/* any of them, then your function MUST be an "ARCH_DEP" function    */
/* that is placed within the ARCH_DEP section at the beginning of    */
/* this module where it can be compiled multiple times, once for     */
/* each of the supported architectures so the macro gets #defined    */
/* to its proper value for the architecture. YOU HAVE BEEN WARNED.   */
/*                                                                   */
/*********************************************************************/

/*-------------------------------------------------------------------*/
/*  Load / IPL         (Load Normal  -or-  Load Clear)               */
/*-------------------------------------------------------------------*/

int load_ipl( U16 lcss, U16 devnum, int cpu, int clear )
{
    int rc = 0;

    switch ( sysblk.arch_mode )
    {
#if defined(_370)
        case ARCH_370_IDX: rc = s370_load_ipl( lcss, devnum, cpu, clear ); break;
#endif
#if defined(_390)
        case ARCH_390_IDX: rc = s390_load_ipl( lcss, devnum, cpu, clear ); break;
#endif
#if defined(_900)
        /* NOTE: z/Arch always starts out in ESA390 mode */
        case ARCH_900_IDX: rc = s390_load_ipl( lcss, devnum, cpu, clear ); break;
#endif
        default: CRASH();
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* initial_cpu_reset_all -- do initial_cpu_reset for every processor */
/*-------------------------------------------------------------------*/
void initial_cpu_reset_all()
{
    REGS* regs;
    int cpu;

    if (sysblk.cpus)
    {
        for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
        {
            if (IS_CPU_ONLINE( cpu ))
            {
                regs = sysblk.regs[ cpu ];
                initial_cpu_reset( regs ) ;
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*  Initial CPU Reset                                                */
/*-------------------------------------------------------------------*/
int initial_cpu_reset( REGS* regs )
{
    int rc = 0;

    switch ( regs->arch_mode )
    {
#if defined(_370)
        case ARCH_370_IDX: rc = s370_initial_cpu_reset( regs ); break;
#endif
#if defined(_390)
        case ARCH_390_IDX: rc = s390_initial_cpu_reset( regs ); break;
#endif
#if defined(_900)
        case ARCH_900_IDX: rc = z900_initial_cpu_reset( regs ); break;
#endif
        default: CRASH();
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*  System Reset            (Normal reset  or  Clear reset)          */
/*-------------------------------------------------------------------*/
int system_reset( const int target_mode, const bool clear, const bool ipl, const int cpu )
{
    int rc = 0;

    switch ( sysblk.arch_mode )
    {
#if defined( _370 )
        case ARCH_370_IDX: rc = s370_system_reset( target_mode, clear, ipl, cpu ); break;
#endif
#if defined( _390 )
        case ARCH_390_IDX: rc = s390_system_reset( target_mode, clear, ipl, cpu ); break;
#endif
#if defined( _900 )
        case ARCH_900_IDX: rc = z900_system_reset( target_mode, clear, ipl, cpu ); break;
#endif
        default: CRASH();
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* ordinary   CPU Reset    (no clearing takes place)                 */
/*-------------------------------------------------------------------*/
int cpu_reset (REGS *regs)
{
    int rc = 0;

    switch (regs->arch_mode)
    {
#if defined(_370)
        case ARCH_370_IDX:
            rc = s370_cpu_reset (regs);
            break;
#endif
#if defined(_390)
        case ARCH_390_IDX:
            rc = s390_cpu_reset (regs);
            break;
#endif
#if defined(_900)
        case ARCH_900_IDX:
            rc = z900_cpu_reset (regs);
            break;
#endif
        default: CRASH();
    }

    return (rc);
}

/*-------------------------------------------------------------------*/
/* Function to clear main storage                                    */
/*-------------------------------------------------------------------*/
void storage_clear()
{
    if (!sysblk.main_clear)
    {
        if (sysblk.mainstor) memset( sysblk.mainstor, 0x00, sysblk.mainsize );
        if (sysblk.storkeys) memset( sysblk.storkeys, 0x00, sysblk.mainsize / _STORKEY_ARRAY_UNITSIZE );
        sysblk.main_clear = 1;
    }
}

/*-------------------------------------------------------------------*/
/* Function to clear expanded storage                                */
/*-------------------------------------------------------------------*/
void xstorage_clear()
{
    if (!sysblk.xpnd_clear)
    {
        if (sysblk.xpndstor)
            memset( sysblk.xpndstor, 0x00, (size_t)sysblk.xpndsize * XSTORE_PAGESIZE );

        sysblk.xpnd_clear = 1;
    }
}

#endif /*!defined(_GEN_ARCH)*/
