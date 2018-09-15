/* MACHCHK.C    (C) Copyright Jan Jaeger, 2000-2012                  */
/*              ESA/390 Machine Check Functions                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */
/*                                                                   */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* The machine check function supports dynamic I/O configuration.    */
/* When a subchannel is added/changed/deleted an ancillary           */
/* channel report is made pending.  This ancillary channel           */
/* report can be read by the store channel report word I/O           */
/* instruction.  Changes to the availability will result in          */
/* Messages IOS150I and IOS151I (device xxxx now/not available)      */
/* Added Instruction processing damage machine check function such   */
/* that abends/waits/loops in instructions will be reflected to the  */
/* system running under hercules as a machine malfunction.  This     */
/* includes the machine check, checkstop, and malfunction alert      */
/* external interrupt as defined in the architecture. - 6/8/01 *JJ   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _MACHCHK_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"

#if !defined(_MACHCHK_C)

#define _MACHCHK_C

/*-------------------------------------------------------------------*/
/* Build a Channel Path Reset Channel Report                         */
/*-------------------------------------------------------------------*/
void build_chp_reset_chrpt( BYTE chpid, int solicited, int found )
{
U32 crw_erc, crwarray[8], crwcount=0;

    /* Just return if shutting down */
    if (sysblk.shutdown)
        return;

    chpid = ((U32)chpid) & CRW_RSID_MASK;

    /* If a subchannel was found on this path and was reset. Ref:
       SA22-7832 "Channel-Path-Reset-Function-Completion Signaling   */
    if (found)
        crw_erc = CRW_ERC_INIT;         /* Init'ed, parms unchanged  */
    else
        crw_erc = CRW_ERC_RESET;        /* Error, parms initialized  */

    /* Build the Channel Path Reset Channel Report */
    crwarray[crwcount++] = (solicited ? CRW_SOL : 0) |
        CRW_RSC_CHPID | CRW_AR | crw_erc | chpid;

    /* Queue the Channel Report */
    VERIFY( queue_channel_report( crwarray, crwcount ) == 0 );
}


/*-------------------------------------------------------------------*/
/* Build a device attach Channel Report                              */
/*-------------------------------------------------------------------*/
void build_attach_chrpt( DEVBLK *dev )
{
U32 ssid, subchan, crwarray[8], crwcount=0;
int devlock_obtained;

    /* Just return if shutting down */
    if (sysblk.shutdown)
        return;

    /* Retrieve Source IDs */
    devlock_obtained = (try_obtain_lock( &dev->lock ) == 0);
    {
        ssid    = ((U32)SSID_TO_LCSS( dev->ssid )) & CRW_RSID_MASK;
        subchan = ((U32)dev->subchan)              & CRW_RSID_MASK;
    }
    if (devlock_obtained)
        release_lock( &dev->lock );

    /* Build Subchannel Alert Channel Report */
    crwarray[crwcount++] =
        0
        | (sysblk.mss ? CRW_CHAIN : 0)
        | CRW_RSC_SUBCH
        | CRW_AR
        | CRW_ERC_ALERT
        | subchan
        ;
    if (sysblk.mss)
        crwarray[crwcount++] =
            0
            | CRW_RSC_SUBCH
            | CRW_AR
            | CRW_ERC_ALERT
            | (ssid << 8)
            ;

    /* Queue the Channel Report(s) */
    VERIFY( queue_channel_report( crwarray, crwcount ) == 0 );
}


/*-------------------------------------------------------------------*/
/* Build a device detach Channel Report                              */
/*-------------------------------------------------------------------*/
void build_detach_chrpt( DEVBLK *dev )
{
U32 ssid, subchan, crwarray[8], crwcount=0;
int devlock_obtained;

    /* Just return if shutting down */
    if (sysblk.shutdown)
        return;

    /* Retrieve Source IDs */
    devlock_obtained = (try_obtain_lock( &dev->lock ) == 0);
    {
        ssid    = ((U32)SSID_TO_LCSS( dev->ssid )) & CRW_RSID_MASK;
        subchan = ((U32)dev->subchan)              & CRW_RSID_MASK;
    }
    if (devlock_obtained)
        release_lock( &dev->lock );

    /* Build Subchannel Alert Channel Report */
    crwarray[crwcount++] =
        0
        | (sysblk.mss ? CRW_CHAIN : 0)
        | CRW_RSC_SUBCH
        | CRW_AR
        | CRW_ERC_ALERT
        | subchan
        ;
    if (sysblk.mss)
        crwarray[crwcount++] =
            0
            | CRW_RSC_SUBCH
            | CRW_AR
            | CRW_ERC_ALERT
            | (ssid << 8)
            ;

    /* Queue the Channel Report */
    VERIFY( queue_channel_report( crwarray, crwcount ) == 0 );
}


/*-------------------------------------------------------------------*/
/* Add a Channel Report to the queue                                 */
/*-------------------------------------------------------------------*/
int queue_channel_report( U32* crwarray, U32 crwcount )
{
    OBTAIN_CRWLOCK();

    if ((sysblk.crwcount + crwcount) > sysblk.crwalloc)
    {
        /* Allocate larger queue */
        U32   newalloc  = sysblk.crwalloc + crwcount;
        U32*  newarray  = malloc( newalloc * sizeof(U32));

        if (!newarray)
        {
            /* Set overflow in last CRW */
            if (sysblk.crwarray)
                *(sysblk.crwarray + sysblk.crwcount - 1) |= CRW_OFLOW;
            RELEASE_CRWLOCK();
            return -1;
        }

        /* Copy existing queue to new queue */
        if (sysblk.crwarray)
        {
            memcpy( newarray, sysblk.crwarray, sysblk.crwcount * sizeof(U32));
            free( sysblk.crwarray );
        }

        /* Start using new queue */
        sysblk.crwarray = newarray;
        sysblk.crwalloc = newalloc;
    }

    /* Add the new CRWs to the queue */
    memcpy( sysblk.crwarray + sysblk.crwcount, crwarray, crwcount * sizeof(U32));
    sysblk.crwcount += crwcount;
    RELEASE_CRWLOCK();

    /* Signal waiting CPUs that a Channel Report is pending */
    machine_check_crwpend();
    return 0;

} /* end function queue_channel_report */


/*-------------------------------------------------------------------*/
/* Indicate CRW pending                                              */
/*-------------------------------------------------------------------*/
void machine_check_crwpend()
{
    /* Signal waiting CPUs that a Channel Report is pending */
    int  have_lock  = have_lock( &sysblk.intlock );
    if (!have_lock)
        OBTAIN_INTLOCK( NULL );
    ON_IC_CHANRPT;
    WAKEUP_CPUS_MASK (sysblk.waiting_mask);
    if (!have_lock)
        RELEASE_INTLOCK(NULL);

} /* end function machine_check_crwpend */


/*-------------------------------------------------------------------*/
/* Return next Channel Report Word (CRW)                             */
/*-------------------------------------------------------------------*/
U32 get_next_channel_report_word( REGS* regs )
{
U32 crw = 0;

    UNREFERENCED(regs);
    OBTAIN_CRWLOCK();
    ASSERT( sysblk.crwindex <= sysblk.crwcount );
    ASSERT( sysblk.crwcount <= sysblk.crwalloc );
    if (sysblk.crwcount)
    {
        if (sysblk.crwindex < sysblk.crwcount)
        {
            VERIFY((crw = *(sysblk.crwarray + sysblk.crwindex)) != 0);
            sysblk.crwindex++; // (for next time)
        }
        else // (sysblk.crwindex >= sysblk.crwcount)
        {
            sysblk.crwindex = 0;
            sysblk.crwcount = 0;
        }
    }
    RELEASE_CRWLOCK();
    return crw;

} /* end function get_next_channel_report_word */

#endif /*!defined(_MACHCHK_C)*/


/*-------------------------------------------------------------------*/
/* Present Machine Check Interrupt                                   */
/* Input:                                                            */
/*      regs    Pointer to the CPU register context                  */
/* Output:                                                           */
/*      mcic    Machine check interrupt code                         */
/*      xdmg    External damage code                                 */
/*      fsta    Failing storage address                              */
/* Return code:                                                      */
/*      0=No machine check, 1=Machine check presented                */
/*                                                                   */
/* Generic machine check function.  At the momement the only         */
/* supported machine check is the channel report.                    */
/*                                                                   */
/* This routine must be called with the sysblk.intlock held          */
/*-------------------------------------------------------------------*/
int ARCH_DEP(present_mck_interrupt)(REGS *regs, U64 *mcic, U32 *xdmg, RADR *fsta)
{
int rc = 0;

    UNREFERENCED_370(regs);
    UNREFERENCED_370(mcic);
    UNREFERENCED_370(xdmg);
    UNREFERENCED_370(fsta);

#ifdef FEATURE_CHANNEL_SUBSYSTEM
    /* If there is a crw pending and we are enabled for the channel
       report interrupt subclass then process the interrupt */
    if( OPEN_IC_CHANRPT(regs) )
    {
        *mcic =  MCIC_CP |
               MCIC_WP |
               MCIC_MS |
               MCIC_PM |
               MCIC_IA |
#ifdef FEATURE_HEXADECIMAL_FLOATING_POINT
               MCIC_FP |
#endif /*FEATURE_HEXADECIMAL_FLOATING_POINT*/
               MCIC_GR |
               MCIC_CR |
               MCIC_ST |
#ifdef FEATURE_ACCESS_REGISTERS
               MCIC_AR |
#endif /*FEATURE_ACCESS_REGISTERS*/
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) && defined(FEATURE_EXTENDED_TOD_CLOCK)
               MCIC_PR |
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) && defined(FEATURE_EXTENDED_TOD_CLOCK)*/
#if defined(FEATURE_BINARY_FLOATING_POINT)
               MCIC_XF |
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
               MCIC_AP |
               MCIC_CT |
               MCIC_CC ;
        *xdmg = 0;
        *fsta = 0;
        OFF_IC_CHANRPT;
        rc = 1;
    }

    if(!IS_IC_CHANRPT)
#endif /*FEATURE_CHANNEL_SUBSYSTEM*/
        OFF_IC_CHANRPT;

    return rc;
} /* end function present_mck_interrupt */


void ARCH_DEP(sync_mck_interrupt) (REGS *regs)
{
int     rc;                             /* Return code               */
PSA    *psa;                            /* -> Prefixed storage area  */

U64     mcic = MCIC_P  |  /* Instruction processing damage */
               MCIC_WP |
               MCIC_MS |
               MCIC_PM |
               MCIC_IA |
#ifdef FEATURE_HEXADECIMAL_FLOATING_POINT
               MCIC_FP |
#endif /*FEATURE_HEXADECIMAL_FLOATING_POINT*/
               MCIC_GR |
               MCIC_CR |
               MCIC_ST |
#ifdef FEATURE_ACCESS_REGISTERS
               MCIC_AR |
#endif /*FEATURE_ACCESS_REGISTERS*/
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) && defined(FEATURE_EXTENDED_TOD_CLOCK)
               MCIC_PR |
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) && defined(FEATURE_EXTENDED_TOD_CLOCK)*/
#if defined(FEATURE_BINARY_FLOATING_POINT)
               MCIC_XF |
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
               MCIC_CT |
               MCIC_CC ;
U32     xdmg = 0;
RADR    fsta = 0;


    /* Release intlock if held */
    if (regs->cpuad == sysblk.intowner)
        RELEASE_INTLOCK(regs);

    /* Release mainlock if held */
    if (regs->cpuad == sysblk.mainowner)
        RELEASE_MAINLOCK(regs);

    /* Exit SIE when active */
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
    if(regs->sie_active)
        ARCH_DEP(sie_exit) (regs, SIE_HOST_INTERRUPT);
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/


    /* Set the main storage reference and change bits */
    STORAGE_KEY(regs->PX, regs) |= (STORKEY_REF | STORKEY_CHANGE);

    /* Point to the PSA in main storage */
    psa = (void*)(regs->mainstor + regs->PX);

    /* Store registers in machine check save area */
    ARCH_DEP(store_status) (regs, regs->PX);

#if !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
// ZZ
    /* Set the extended logout area to zeros */
    memset(psa->storepsw, 0, 16);
#endif

    /* Store the machine check interrupt code at PSA+232 */
    STORE_DW(psa->mckint, mcic);

    /* Trace the machine check interrupt */
    if (CPU_STEPPING_OR_TRACING(regs, 0))
#if defined(_FEATURE_SIE)
        WRMSG (HHC00824, "I", regs->sie_active ? "IE" : PTYPSTR(regs->cpuad),
            regs->sie_active ? regs->guestregs->cpuad : regs->cpuad, mcic);
#else
        WRMSG (HHC00824, "I", PTYPSTR(regs->cpuad), regs->cpuad, mcic);
#endif

    /* Store the external damage code at PSA+244 */
    STORE_FW(psa->xdmgcode, xdmg);

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* Store the failing storage address at PSA+248 */
    STORE_DW(psa->mcstorad, fsta);
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    /* Store the failing storage address at PSA+248 */
    STORE_FW(psa->mcstorad, fsta);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Store current PSW at PSA+X'30' */
    ARCH_DEP(store_psw) ( regs, psa->mckold );

    /* Load new PSW from PSA+X'70' */
    rc = ARCH_DEP(load_psw) ( regs, psa->mcknew );

    if ( rc )
        ARCH_DEP(program_interrupt) (regs, rc);
} /* end function sync_mck_interrupt */


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "machchk.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "machchk.c"
#endif

#if !defined(NO_SIGABEND_HANDLER)
void sigabend_handler (int signo)
{
REGS *regs = NULL;
TID tid;
int i;

    tid = thread_id();

    if( signo == SIGUSR2 )
    {
    DEVBLK *dev;
        if (0
            || equal_threads( tid, sysblk.cnsltid )
            || equal_threads( tid, sysblk.socktid )
#if defined(OPTION_SHARED_DEVICES)
            || equal_threads( tid, sysblk.shrdtid )
#endif // defined(OPTION_SHARED_DEVICES)
            || equal_threads( tid, sysblk.httptid )
        )
            return;
        for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
            if (0
                || equal_threads( dev->tid, tid )
#if defined(OPTION_SHARED_DEVICES)
                || equal_threads( dev->shrdtid, tid )
#endif // defined(OPTION_SHARED_DEVICES)
            )
                 break;
        if( dev == NULL)
        {
            if (!sysblk.shutdown)
                WRMSG(HHC00825, "E");
        }
        else
            if(dev->ccwtrace)
                WRMSG(HHC00826, "E", LCSS_DEVNUM);
        return;
    }

    for (i = 0; i < sysblk.maxcpu; i++)
    {
        if ( equal_threads( sysblk.cputid[i], tid ) )
        {
            regs = sysblk.regs[i];
            break;
        }
    }

    if (regs == NULL)
    {
        signal(signo, SIG_DFL);
        raise(signo);
        return;
    }

    if(MACHMASK(&regs->psw))
    {
#if defined(_FEATURE_SIE)
        WRMSG(HHC00822, "I", regs->sie_active ? "IE" : PTYPSTR(regs->cpuad),
            regs->sie_active ? regs->guestregs->cpuad : regs->cpuad,
            strsignal(signo) );
#else /*!defined(_FEATURE_SIE)*/
        WRMSG(HHC00822, "I", PTYPSTR(regs->cpuad), regs->cpuad, strsignal(signo));
#endif /*!defined(_FEATURE_SIE)*/

        display_inst(
#if defined(_FEATURE_SIE)
                     regs->sie_active ? regs->guestregs :
#endif /*defined(_FEATURE_SIE)*/
                                                          regs,
#if defined(_FEATURE_SIE)
          regs->sie_active ? regs->guestregs->ip :
#endif /*defined(_FEATURE_SIE)*/
                                                   regs->ip);

        switch(regs->arch_mode) {
#if defined(_370)
            case ARCH_370_IDX:
                s370_sync_mck_interrupt(regs);
                break;
#endif
#if defined(_390)
            case ARCH_390_IDX:
                s390_sync_mck_interrupt(regs);
                break;
#endif
#if defined(_900)
            case ARCH_900_IDX:
                z900_sync_mck_interrupt(regs);
                break;
#endif
        }
    }
    else
    {
#if defined(_FEATURE_SIE)
        WRMSG(HHC00823, "I", regs->sie_active ? "IE" : PTYPSTR(regs->cpuad),
            regs->sie_active ? regs->guestregs->cpuad : regs->cpuad,
            strsignal(signo));
#else /*!defined(_FEATURE_SIE)*/
        WRMSG(HHC00823, "I", PTYPSTR(regs->cpuad), regs->cpuad, strsignal(signo));
#endif /*!defined(_FEATURE_SIE)*/
        display_inst(
#if defined(_FEATURE_SIE)
                     regs->sie_active ? regs->guestregs :
#endif /*defined(_FEATURE_SIE)*/
                                                          regs,
#if defined(_FEATURE_SIE)
          regs->sie_active ? regs->guestregs->ip :
#endif /*defined(_FEATURE_SIE)*/
                                                   regs->ip);
        regs->cpustate = CPUSTATE_STOPPING;
        regs->checkstop = 1;
        ON_IC_INTERRUPT(regs);

        /* Notify other CPU's by means of a malfuction alert if possible */
        if (!try_obtain_lock(&sysblk.sigplock))
        {
            if(!try_obtain_lock(&sysblk.intlock))
            {
                for (i = 0; i < sysblk.maxcpu; i++)
                    if (i != regs->cpuad && IS_CPU_ONLINE(i))
                    {
                        ON_IC_MALFALT(sysblk.regs[i]);
                        sysblk.regs[i]->malfcpu[regs->cpuad] = 1;
                    }
                release_lock(&sysblk.intlock);
            }
            release_lock(&sysblk.sigplock);
        }

    }

    longjmp (regs->progjmp, SIE_INTERCEPT_MCK);
}
#endif /*!defined(NO_SIGABEND_HANDLER)*/

#endif /*!defined(_GEN_ARCH)*/
