/* EXTERNAL.C   (c) Copyright Roger Bowler, 1999-2012                */
/*              ESA/390 External Interrupt and Timer                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (c) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (c) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements external interrupt, timer, and signalling  */
/* functions for the Hercules ESA/390 emulator.                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      TOD clock offset contributed by Jay Maynard                  */
/*      Correction to timer interrupt by Valery Pogonchenko          */
/*      TOD clock drag factor contributed by Jan Jaeger              */
/*      CPU timer and clock comparator interrupt improvements by     */
/*          Jan Jaeger, after a suggestion by Willem Koynenberg      */
/*      Prevent TOD clock and CPU timer from going back - Jan Jaeger */
/*      Use longjmp on all interrupt types - Jan Jaeger              */
/*      Fix todclock - Jay Maynard                                   */
/*      Modifications for Interpretive Execution (SIE) by Jan Jaeger */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#if !defined(_EXTERNAL_C_)
#define _EXTERNAL_C_
#endif

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/*-------------------------------------------------------------------*/
/* Load external interrupt new PSW                                   */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(external_interrupt) (int code, REGS *regs)
{
RADR    pfx;
PSA     *psa;
int     rc;

    PTT_SIG("*EXTINT",code,regs->cpuad,regs->psw.IA_L);

#if defined(_FEATURE_SIE)
    /* Set the main storage reference and change bits */
    if(SIE_MODE(regs)
#if defined(_FEATURE_EXPEDITED_SIE_SUBSET)
                       && !SIE_FEATB(regs, S, EXP_TIMER)
#endif /*defined(_FEATURE_EXPEDITED_SIE_SUBSET)*/
#if defined(_FEATURE_EXTERNAL_INTERRUPT_ASSIST)
                       && !SIE_FEATB(regs, EC0, EXTA)
#endif
                                                            )
    {
        /* Point to SIE copy of PSA in state descriptor */
        psa = (void*)(regs->hostregs->mainstor + SIE_STATE(regs) + SIE_IP_PSA_OFFSET);
        STORAGE_KEY(SIE_STATE(regs), regs->hostregs) |= (STORKEY_REF | STORKEY_CHANGE);
    }
    else
#endif /*defined(_FEATURE_SIE)*/
    {
        /* Point to PSA in main storage */
        pfx = regs->PX;
#if defined(_FEATURE_EXPEDITED_SIE_SUBSET)
        SIE_TRANSLATE(&pfx, ACCTYPE_SIE, regs);
#endif /*defined(_FEATURE_EXPEDITED_SIE_SUBSET)*/
        psa = (void*)(regs->mainstor + pfx);
        STORAGE_KEY(pfx, regs) |= (STORKEY_REF | STORKEY_CHANGE);
    }

    /* Store the interrupt code in the PSW */
    regs->psw.intcode = code;


    /* Zero extcpuad field unless extcall or ems signal or blockio */
    if(code != EXT_EXTERNAL_CALL_INTERRUPT
#if defined(FEATURE_VM_BLOCKIO)
    && code != EXT_BLOCKIO_INTERRUPT
#endif /* defined(FEATURE_VM_BLOCKIO) */
    && code != EXT_EMERGENCY_SIGNAL_INTERRUPT)
        STORE_HW(psa->extcpad,0);

#if defined(FEATURE_BCMODE)
    /* For ECMODE, store external interrupt code at PSA+X'86' */
    if ( ECMODE(&regs->psw) )
#endif /*defined(FEATURE_BCMODE)*/
        STORE_HW(psa->extint,code);

    if ( !SIE_MODE(regs)
#if defined(_FEATURE_EXPEDITED_SIE_SUBSET)
                       || SIE_FEATB(regs, S, EXP_TIMER)
#endif /*defined(_FEATURE_EXPEDITED_SIE_SUBSET)*/
#if defined(_FEATURE_EXTERNAL_INTERRUPT_ASSIST)
                       || SIE_FEATB(regs, EC0, EXTA)
#endif
       )
    {
        /* Store current PSW at PSA+X'18' */
        ARCH_DEP(store_psw) (regs, psa->extold);

        /* Load new PSW from PSA+X'58' */
        rc = ARCH_DEP(load_psw) (regs, psa->extnew);

        if ( rc )
        {
            RELEASE_INTLOCK(regs);
            ARCH_DEP(program_interrupt)(regs, rc);
        }
    }

#if defined(FEATURE_INTERVAL_TIMER)
    /* Ensure the interval timer is uptodate */
    ARCH_DEP(store_int_timer_nolock) (regs);
#endif
    RELEASE_INTLOCK(regs);


    if ( SIE_MODE(regs)
#if defined(_FEATURE_EXPEDITED_SIE_SUBSET)
                       && !SIE_FEATB(regs, S, EXP_TIMER)
#endif /*defined(_FEATURE_EXPEDITED_SIE_SUBSET)*/
#if defined(_FEATURE_EXTERNAL_INTERRUPT_ASSIST)
                       && !SIE_FEATB(regs, EC0, EXTA)
#endif
       )
        longjmp (regs->progjmp, SIE_INTERCEPT_EXT);
    else
        longjmp (regs->progjmp, SIE_NO_INTERCEPT);

} /* end function external_interrupt */

/*-------------------------------------------------------------------*/
/* Perform external interrupt if pending                             */
/*                                                                   */
/* This function is called by the CPU to check whether any           */
/* external interrupt conditions are pending, and to perform         */
/* an external interrupt if so.  If multiple external interrupts     */
/* are pending, then only the highest priority interrupt is taken,   */
/* and any other interrupts remain pending.  Remaining interrupts    */
/* will be processed one-by-one during subsequent calls.             */
/*                                                                   */
/* Important notes:                                                  */
/* (i)  This function must NOT be called if the CPU is disabled      */
/*      for external interrupts (PSW bit 7 is zero).                 */
/* (ii) The caller MUST hold the interrupt lock (sysblk.intlock)     */
/*      to ensure correct serialization of interrupt pending bits.   */
/*-------------------------------------------------------------------*/
void ARCH_DEP(perform_external_interrupt) (REGS *regs)
{
PSA    *psa;                            /* -> Prefixed storage area  */
U16     cpuad;                          /* Originating CPU address   */
#if defined(FEATURE_VM_BLOCKIO)
#if defined(FEATURE_ESAME)
RADR    servpadr;      /* Address of 64-bit block I/O interrupt */
#endif
U16     servcode;      /* Service Signal or Block I/O Interrupt code */
#endif /* defined(FEATURE_VM_BLOCKIO) */

    /* External interrupt if console interrupt key was depressed */
    if ( OPEN_IC_INTKEY(regs) && !SIE_MODE(regs) )
    {
        WRMSG (HHC00840, "I");

        /* Reset interrupt key pending */
        OFF_IC_INTKEY;

        /* Generate interrupt key interrupt */
        ARCH_DEP(external_interrupt) (EXT_INTERRUPT_KEY_INTERRUPT, regs);
    }

    /* External interrupt if malfunction alert is pending */
    if (OPEN_IC_MALFALT(regs))
    {
        /* Find first CPU which generated a malfunction alert */
        for (cpuad = 0; regs->malfcpu[cpuad] == 0; cpuad++)
        {
            if (cpuad >= sysblk.maxcpu)
            {
                OFF_IC_MALFALT(regs);
                return;
            }
        } /* end for(cpuad) */

// /*debug*/ logmsg (_("External interrupt: Malfuction Alert from CPU %d\n"),
// /*debug*/    cpuad);

        /* Reset the indicator for the CPU which was found */
        regs->malfcpu[cpuad] = 0;

        /* Store originating CPU address at PSA+X'84' */
        psa = (void*)(regs->mainstor + regs->PX);
        STORE_HW(psa->extcpad,cpuad);

        /* Reset emergency signal pending flag if there are
           no other CPUs which generated emergency signal */
        OFF_IC_MALFALT(regs);
        while (++cpuad < sysblk.maxcpu)
        {
            if (regs->malfcpu[cpuad])
            {
                ON_IC_MALFALT(regs);
                break;
            }
        } /* end while */

        /* Generate emergency signal interrupt */
        ARCH_DEP(external_interrupt) (EXT_MALFUNCTION_ALERT_INTERRUPT, regs);
    }


    /* External interrupt if emergency signal is pending */
    if (OPEN_IC_EMERSIG(regs))
    {
        /* Find first CPU which generated an emergency signal */
        for (cpuad = 0; regs->emercpu[cpuad] == 0; cpuad++)
        {
            if (cpuad >= sysblk.maxcpu)
            {
                OFF_IC_EMERSIG(regs);
                return;
            }
        } /* end for(cpuad) */

// /*debug*/ logmsg (_("External interrupt: Emergency Signal from CPU %d\n"),
// /*debug*/    cpuad);

        /* Reset the indicator for the CPU which was found */
        regs->emercpu[cpuad] = 0;

        /* Store originating CPU address at PSA+X'84' */
        psa = (void*)(regs->mainstor + regs->PX);
        STORE_HW(psa->extcpad,cpuad);

        /* Reset emergency signal pending flag if there are
           no other CPUs which generated emergency signal */
        OFF_IC_EMERSIG(regs);
        while (++cpuad < sysblk.maxcpu)
        {
            if (regs->emercpu[cpuad])
            {
                ON_IC_EMERSIG(regs);
                break;
            }
        } /* end while */

        /* Generate emergency signal interrupt */
        ARCH_DEP(external_interrupt) (EXT_EMERGENCY_SIGNAL_INTERRUPT, regs);
    }

    /* External interrupt if external call is pending */
    if (OPEN_IC_EXTCALL(regs))
    {
//  /*debug*/logmsg (_("External interrupt: External Call from CPU %d\n"),
//  /*debug*/       regs->extccpu);

        /* Reset external call pending */
        OFF_IC_EXTCALL(regs);

        /* Store originating CPU address at PSA+X'84' */
        psa = (void*)(regs->mainstor + regs->PX);
        STORE_HW(psa->extcpad,regs->extccpu);

        /* Generate external call interrupt */
        ARCH_DEP(external_interrupt) (EXT_EXTERNAL_CALL_INTERRUPT, regs);
    }

    /* External interrupt if TOD clock exceeds clock comparator */
    if ( tod_clock(regs) > regs->clkc
        && OPEN_IC_CLKC(regs) )
    {
        if (CPU_STEPPING_OR_TRACING_ALL)
        {
            WRMSG (HHC00841, "I");
        }
        ARCH_DEP(external_interrupt) (EXT_CLOCK_COMPARATOR_INTERRUPT, regs);
    }

    /* External interrupt if CPU timer is negative */
    if ( CPU_TIMER(regs) < 0
        && OPEN_IC_PTIMER(regs) )
    {
        if (CPU_STEPPING_OR_TRACING_ALL)
        {
            WRMSG (HHC00842, "I", CPU_TIMER(regs) );
        }
        ARCH_DEP(external_interrupt) (EXT_CPU_TIMER_INTERRUPT, regs);
    }

    /* External interrupt if interval timer interrupt is pending */
#if defined(FEATURE_INTERVAL_TIMER)
    if (OPEN_IC_ITIMER(regs)
#if defined(_FEATURE_SIE)
        && !(SIE_STATB(regs, M, ITMOF))
#endif /*defined(_FEATURE_SIE)*/
        )
    {
        if (CPU_STEPPING_OR_TRACING_ALL)
        {
            WRMSG (HHC00843, "I");
        }
        OFF_IC_ITIMER(regs);
        ARCH_DEP(external_interrupt) (EXT_INTERVAL_TIMER_INTERRUPT, regs);
    }

#if defined(FEATURE_ECPSVM)
    if ( OPEN_IC_ECPSVTIMER(regs) )
    {
        OFF_IC_ECPSVTIMER(regs);
        ARCH_DEP(external_interrupt) (EXT_VINTERVAL_TIMER_INTERRUPT,regs);
    }
#endif /*FEATURE_ECPSVM*/
#endif /*FEATURE_INTERVAL_TIMER*/

    /* External interrupt if service signal is pending */
    if ( OPEN_IC_SERVSIG(regs) && !SIE_MODE(regs) )
    {

#if defined(FEATURE_VM_BLOCKIO)

        /* Note: Both Block I/O and Service Signal are enabled by the */
        /* the same CR0 bit.  Hence they are handled in the same code */
        switch(sysblk.servcode)
        {
        case EXT_BLOCKIO_INTERRUPT:  /* VM Block I/O Interrupt */

           if (sysblk.biodev->ccwtrace)
           {
           WRMSG (HHC00844, "I",
                SSID_TO_LCSS(sysblk.biodev->ssid),
                sysblk.biodev->devnum,
                sysblk.servcode,
                sysblk.bioparm,
                sysblk.biostat,
                sysblk.biosubcd
                );
           }

           servcode = EXT_BLOCKIO_INTERRUPT;

#if defined(FEATURE_ESAME)
/* Real address used to store the 64-bit interrupt parameter */
#define VM_BLOCKIO_INT_PARM 0x11B8
           if (sysblk.biosubcd == 0x07)
           {
           /* 8-byte interrupt parm */

               if (CPU_STEPPING_OR_TRACING_ALL)
               {
                  char buf[40];
                  MSGBUF(buf, "%16.16X", (unsigned) sysblk.bioparm);
                  WRMSG (HHC00845,"I", buf);
               }

               /* Set the main storage reference and change bits   */
               /* for 64-bit interruption parameter.               */
               /* Note: This is handled for the first 4K page in   */
               /* ARCH_DEP(external_interrupt), but not for the    */
               /* the second 4K page used for the 64-bit interrupt */
               /* parameter.                                       */

               /* Point to 2nd page of PSA in main storage */
               servpadr=APPLY_PREFIXING(VM_BLOCKIO_INT_PARM,regs->PX);

               STORAGE_KEY(servpadr, regs)
                     |= (STORKEY_REF | STORKEY_CHANGE);

#if 0
               /* Store the 64-bit interrupt parameter */
               logmsg (_("Saving 64-bit Block I/O interrupt parm at "
                         "%16.16X: %16.16X\n"),
                         servpadr,
                         sysblk.bioparm
                      );
#endif

               STORE_DW(regs->mainstor + servpadr,sysblk.bioparm);
               psa = (void*)(regs->mainstor + regs->PX);
           }
           else
           {
#endif  /* defined(FEATURE_ESAME) */

           /* 4-byte interrupt parm */

              if (CPU_STEPPING_OR_TRACING_ALL)
              {
                 char buf[40];
                 MSGBUF(buf, "%8.8X", (U32) sysblk.bioparm);
                 WRMSG (HHC00845,"I", buf);
              }

              /* Store Block I/O parameter at PSA+X'80' */
              psa = (void*)(regs->mainstor + regs->PX);
              STORE_FW(psa->extparm,(U32)sysblk.bioparm);

#if defined(FEATURE_ESAME)
           }
#endif

           /* Store sub-interruption code and status at PSA+X'84' */
           STORE_HW(psa->extcpad,(sysblk.biosubcd<<8)|sysblk.biostat);

           /* Reset interruption data */
           sysblk.bioparm  = 0;
           sysblk.biosubcd = 0;
           sysblk.biostat  = 0;

           break;

        case EXT_SERVICE_SIGNAL_INTERRUPT: /* Service Signal */
        default:
             servcode = EXT_SERVICE_SIGNAL_INTERRUPT;

            /* Apply prefixing if the parameter is a storage address */
            if ( (sysblk.servparm & SERVSIG_ADDR) )
                sysblk.servparm =
                     APPLY_PREFIXING (sysblk.servparm, regs->PX);

             if (CPU_STEPPING_OR_TRACING_ALL)
             {
                 WRMSG (HHC00846,"I", sysblk.servparm);
             }

             /* Store service signal parameter at PSA+X'80' */
             psa = (void*)(regs->mainstor + regs->PX);
             STORE_FW(psa->extparm,sysblk.servparm);

        }  /* end switch(sysblk.servcode) */
        /* Reset service parameter */
        sysblk.servparm = 0;

        /* Reset service code */
        sysblk.servcode = 0;

        /* Reset service signal pending */
        OFF_IC_SERVSIG;

        /* Generate service signal interrupt */
        ARCH_DEP(external_interrupt) (servcode, regs);

#else /* defined(FEATURE_VM_BLOCKIO) */

        /* Apply prefixing if the parameter is a storage address */
        if ( (sysblk.servparm & SERVSIG_ADDR) )
            sysblk.servparm =
                APPLY_PREFIXING (sysblk.servparm, regs->PX);

        if (CPU_STEPPING_OR_TRACING_ALL)
        {
            WRMSG (HHC00846,"I", sysblk.servparm);
        }

        /* Store service signal parameter at PSA+X'80' */
        psa = (void*)(regs->mainstor + regs->PX);
        STORE_FW(psa->extparm,sysblk.servparm);

        /* Reset service parameter */
        sysblk.servparm = 0;

        /* Reset service signal pending */
        OFF_IC_SERVSIG;

        /* Generate service signal interrupt */
        ARCH_DEP(external_interrupt) (EXT_SERVICE_SIGNAL_INTERRUPT, regs);

#endif /* defined(FEATURE_VM_BLOCKIO) */

    }  /* end OPEN_IC_SERVSIG(regs) */

} /* end function perform_external_interrupt */


/*-------------------------------------------------------------------*/
/* Store Status                                                      */
/* Input:                                                            */
/*      sregs   Register context of CPU whose status is to be stored */
/*      aaddr   A valid absolute address of a 512-byte block into    */
/*              which status is to be stored                         */
/*              For an implicit store status, or an operator         */
/*              initiated store status the absolute address will be  */
/*              zero, for a store status at address order the        */
/*              supplied address will be nonzero                     */
/*-------------------------------------------------------------------*/
void ARCH_DEP(store_status) (REGS *ssreg, RADR aaddr)
{
int     i;                              /* Array subscript           */
PSA     *sspsa;                         /* -> Store status area      */

    /* Set reference and change bits */
    STORAGE_KEY(aaddr, ssreg) |= (STORKEY_REF | STORKEY_CHANGE);
#if defined(FEATURE_ESAME)
    /* The ESAME PSA is two pages in size */
    if(!aaddr)
        STORAGE_KEY(aaddr + 4096, ssreg) |= (STORKEY_REF | STORKEY_CHANGE);
#endif /*defined(FEATURE_ESAME)*/

#if defined(FEATURE_ESAME)
    /* For store status at address, we must adjust the PSA offset */
    /* ZZ THIS TEST IS NOT CONCLUSIVE */
    if(aaddr != 0 && aaddr != ssreg->PX)
        aaddr -= 512 + 4096 ;
#endif

    aaddr &= 0x7FFFFE00;

    /* Point to the PSA into which status is to be stored */
    sspsa = (void*)(ssreg->mainstor + aaddr);

    /* Store CPU timer in bytes 216-223 */
    STORE_DW(sspsa->storeptmr, cpu_timer(ssreg));

    /* Store clock comparator in bytes 224-231 */
#if defined(FEATURE_ESAME)
    STORE_DW(sspsa->storeclkc, ssreg->clkc);
#else /*defined(FEATURE_ESAME)*/
    STORE_DW(sspsa->storeclkc, ssreg->clkc << 8);
#endif /*defined(FEATURE_ESAME)*/

    /* Store PSW in bytes 256-263 */
    ARCH_DEP(store_psw) (ssreg, sspsa->storepsw);

    /* Store prefix register in bytes 264-267 */
    STORE_FW(sspsa->storepfx,ssreg->PX);

#if defined(FEATURE_ESAME)
    /* Store Floating Point Control Register */
    STORE_FW(sspsa->storefpc,ssreg->fpc);

    /* Store TOD Programable register */
    STORE_FW(sspsa->storetpr,ssreg->todpr);
#endif /*defined(FEATURE_ESAME)*/

#if defined(_900)
    /* Only store the arch mode indicator for a PSA type store status */
    if(!aaddr)
#if defined(FEATURE_ESAME)
        sspsa->arch = 1;
#else /*defined(FEATURE_ESAME)*/
        sspsa->arch = 0;
#endif /*defined(FEATURE_ESAME)*/
#endif /*defined(_900)*/

    /* Store access registers in bytes 288-351 */
    for (i = 0; i < 16; i++)
        STORE_FW(sspsa->storear[i],ssreg->AR(i));

    /* Store floating-point registers in bytes 352-383 */
#if defined(FEATURE_ESAME)
    for (i = 0; i < 32; i++)
#else /*!defined(FEATURE_ESAME)*/
    for (i = 0; i < 8; i++)
#endif /*!defined(FEATURE_ESAME)*/
        STORE_FW(sspsa->storefpr[i],ssreg->fpr[i]);

    /* Store general-purpose registers in bytes 384-447 */
    for (i = 0; i < 16; i++)
        STORE_W(sspsa->storegpr[i],ssreg->GR(i));

    /* Store control registers in bytes 448-511 */
    for (i = 0; i < 16; i++)
        STORE_W(sspsa->storecr[i],ssreg->CR(i));

} /* end function store_status */


#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "external.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "external.c"
#endif


void store_status (REGS *ssreg, U64 aaddr)
{
    switch(ssreg->arch_mode) {
#if defined(_370)
        case ARCH_370:
            aaddr &= 0x7FFFFFFF;
            s370_store_status (ssreg, aaddr);
            break;
#endif
#if defined(_390)
        case ARCH_390:
            aaddr &= 0x7FFFFFFF;
            s390_store_status (ssreg, aaddr);
            break;
#endif
#if defined(_900)
        case ARCH_900:
            z900_store_status (ssreg, aaddr);
            break;
#endif
    }
}
#endif /*!defined(_GEN_ARCH)*/
