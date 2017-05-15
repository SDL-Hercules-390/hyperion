/* DIAGNOSE.C   (c) Copyright Roger Bowler, 2000-2012                */
/*              ESA/390 Diagnose Functions                           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support - (c) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements miscellaneous diagnose functions           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Hercules-specific diagnose calls by Jay Maynard.             */
/*      Set/reset bad frame indicator call by Jan Jaeger.            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#if !defined(_DIAGNOSE_C_)
#define _DIAGNOSE_C_
#endif

#include "hercules.h"

#include "opcode.h"

#include "inline.h"

#if !defined(_DIAGNOSE_H)

#define _DIAGNOSE_H

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/

/* Diagnose 308 function subcodes */
#define DIAG308_IPL_CLEAR       3       /* IPL clear                 */
#define DIAG308_IPL_NORMAL      4       /* IPL normal/dump           */
#define DIAG308_SET_PARAM       5       /* Set IPL parameters        */
#define DIAG308_STORE_PARAM     6       /* Store IPL parameters      */

/* Diagnose 308 return codes */
#define DIAG308_RC_OK           1

#endif /*!defined(_DIAGNOSE_H)*/

#if defined(OPTION_DYNAMIC_LOAD) && defined(FEATURE_HERCULES_DIAGCALLS)

static void ARCH_DEP(diagf14_call)(int r1, int r3, REGS *regs)
{
char name[32+1];
char entry[64];
unsigned int  i;
void (*dllcall)(int, int, REGS *);

static char *prefix[] = {
#if defined(_370)
    "s370_diagf14_",
#endif
#if defined(_390)
    "s390_diagf14_",
#endif
#if defined(_900)
    "z900_diagf14_"
#endif
    };

    ARCH_DEP(vfetchc) (name,sizeof(name)-2, regs->GR(r1), USE_REAL_ADDR, regs);

    for(i = 0; i < sizeof(name)-1; i++)
    {
        name[i] = guest_to_host(name[i]);
        if(!isprint(name[i]) || isspace(name[i]))
        {
            name[i] = '\0';
            break;
        }
    }
    /* Ensure string terminator */
    name[i] = '\0';
    STRLCPY( entry, prefix[regs->arch_mode] );
    STRLCAT( entry, name );

    if( (dllcall = HDL_FINDSYM(entry)) )
        dllcall(r1, r3, regs);
    else
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

}
#endif /*defined(OPTION_DYNAMIC_LOAD)*/

#if defined(FEATURE_PROGRAM_DIRECTED_REIPL) && !defined(STOP_CPUS_AND_IPL)
#define STOP_CPUS_AND_IPL
/*---------------------------------------------------------------------------*/
/* Within diagnose 0x308 (re-ipl) a thread is started with the next code.    */
/*---------------------------------------------------------------------------*/
static void *stop_cpus_and_ipl(int *ipltype)
{
  int i;
  char iplcmd[256];
  int cpustates;
  CPU_BITMAP mask;

  panel_command("stopall");

  WRMSG(HHC01900, "I");
  sprintf(iplcmd, "%s %03X", ipltype, sysblk.ipldev);
  do
  {
    OBTAIN_INTLOCK(NULL);
    cpustates = CPUSTATE_STOPPED;
    mask = sysblk.started_mask;
    for(i = 0; mask; i++)
    {
      if(mask & 1)
      {
       WRMSG(HHC01901, "I", PTYPSTR(i), i);
        if(IS_CPU_ONLINE(i) && sysblk.regs[i]->cpustate != CPUSTATE_STOPPED)
          cpustates = sysblk.regs[i]->cpustate;
      }
      mask >>= 1;
    }
    RELEASE_INTLOCK(NULL);
    if(cpustates != CPUSTATE_STOPPED)
    {
      WRMSG(HHC01902, "I");
      SLEEP(1);
    }
  }
  while(cpustates != CPUSTATE_STOPPED);

  panel_command(iplcmd);

  return NULL;
}
#endif /*defined(FEATURE_PROGRAM_DIRECTED_REIPL) && !defined(STOP_CPUS_AND_IPL)*/

/*-------------------------------------------------------------------*/
/* Diagnose instruction                                              */
/*-------------------------------------------------------------------*/
void ARCH_DEP(diagnose_call) (VADR effective_addr2, int b2,
                              int r1, int r2, REGS *regs)
{
#ifdef FEATURE_HERCULES_DIAGCALLS
U32   n;                                /* 32-bit operand value      */
#endif /*FEATURE_HERCULES_DIAGCALLS*/
U32   code;

    code = effective_addr2;

    switch(code) {


#if defined(FEATURE_IO_ASSIST)
    case 0x002:
    /*---------------------------------------------------------------*/
    /* Diagnose 002: Update Interrupt Interlock Control Bit in PMCW  */
    /*---------------------------------------------------------------*/

        ARCH_DEP(diagnose_002) (regs, r1, r2);

        break;
#endif


    case 0x01F:
    /*---------------------------------------------------------------*/
    /* Diagnose 01F: Power Off                                       */
    /*---------------------------------------------------------------*/

        /* If diag8opt is not enabled then we are not allowed
         * to manipulate the real machine i.e. hercules itself
         */
        if(!(sysblk.diag8opt & DIAG8CMD_ENABLE))
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

        /* The poweroff diagnose is only valid on the 9221 */
        if (sysblk.cpumodel != 0x9221
          /* and r1/r2 must contain C'POWEROFF' in EBCDIC */
          || regs->GR_L(r1) != 0xD7D6E6C5
          || regs->GR_L(r2) != 0xD9D6C6C6)
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

        regs->cpustate = CPUSTATE_STOPPING;
        ON_IC_INTERRUPT(regs);

        /* Release the configuration */
        do_shutdown();

        /* Power Off: exit hercules */
        exit(0);

#if defined(FEATURE_HYPERVISOR) || defined(FEATURE_EMULATE_VM)
    case 0x044:
    /*---------------------------------------------------------------*/
    /* Diagnose 044: Voluntary Time Slice End                        */
    /*---------------------------------------------------------------*/
        ARCH_DEP(scpend_call) ();
        break;
#endif


#ifdef FEATURE_MSSF_CALL
    case 0x080:
    /*---------------------------------------------------------------*/
    /* Diagnose 080: MSSF Call                                       */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(mssf_call) (r1, r2, regs);
        break;
#endif /*FEATURE_MSSF_CALL*/


#if defined(FEATURE_HYPERVISOR) || defined(FEATURE_EMULATE_VM)
    case 0x09C:
    /*---------------------------------------------------------------*/
    /* Diagnose 09C: Voluntary Time Slice End With Target CPU        */
    /*---------------------------------------------------------------*/
        ARCH_DEP(scpend_call) ();   // (treat same as DIAG X'44')
        break;
#endif


#if defined(FEATURE_HYPERVISOR)
    case 0x204:
    /*---------------------------------------------------------------*/
    /* Diagnose 204: LPAR RMF Interface                              */
    /*---------------------------------------------------------------*/
        ARCH_DEP(diag204_call) (r1, r2, regs);
        regs->psw.cc = 0;
        break;

    case 0x224:
    /*---------------------------------------------------------------*/
    /* Diagnose 224: CPU Names                                       */
    /*---------------------------------------------------------------*/
        ARCH_DEP(diag224_call) (r1, r2, regs);
        regs->psw.cc = 0;
        break;
#endif /*defined(FEATURE_HYPERVISOR)*/

#if 0
    case 0x21C:
    /*---------------------------------------------------------------*/
    /* Diagnose 21C: ????                                            */
    /*---------------------------------------------------------------*/
        /*INCOMPLETE*/
        regs->psw.cc = 0;
        break;
#endif

#if 0
    case 0x288;:
    /*---------------------------------------------------------------*/
    /* Diagnose 288: Control Virtual Machine Time Bomb               */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(vm_timebomb) (r1, r2, regs);
        break;
#endif

#ifdef FEATURE_EMULATE_VM
    case 0x000:
    /*---------------------------------------------------------------*/
    /* Diagnose 000: Store Extended Identification Code              */
    /*---------------------------------------------------------------*/
        ARCH_DEP(extid_call) (r1, r2, regs);
        break;

    case 0x008:
    /*---------------------------------------------------------------*/
    /* Diagnose 008: Virtual Console Function                        */
    /*---------------------------------------------------------------*/
        /* If diag8opt is not enabled then we are not allowed
         * to manipulate the real machine i.e. hercules itself
         */
        if(!(sysblk.diag8opt & DIAG8CMD_ENABLE))
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

        /* Process CP command and set condition code */
        regs->psw.cc = ARCH_DEP(cpcmd_call) (r1, r2, regs);
        break;

    case 0x00C:
    /*---------------------------------------------------------------*/
    /* Diagnose 00C: Pseudo Timer                                    */
    /*---------------------------------------------------------------*/
        ARCH_DEP(pseudo_timer) (code, r1, r2, regs);
        break;

    case 0x024:
    /*---------------------------------------------------------------*/
    /* Diagnose 024: Device Type and Features                        */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(diag_devtype) (r1, r2, regs);
        break;

    case 0x05C:
    /*---------------------------------------------------------------*/
    /* Diagnose 05C: Error Message Editing                           */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        regs->psw.cc = 0;
        break;

    case 0x060:
    /*---------------------------------------------------------------*/
    /* Diagnose 060: Virtual Machine Storage Size                    */
    /*---------------------------------------------------------------*/
        /* Load main storage size in bytes into R1 register */
        regs->GR_L(r1) = regs->mainlim + 1;
        break;

    case 0x064:
    /*---------------------------------------------------------------*/
    /* Diagnose 064: Named Saved Segment Manipulation                */
    /*---------------------------------------------------------------*/
        /* Return code 44 cond code 2 means segment does not exist */
        regs->GR_L(r2) = 44;
        regs->psw.cc = 2;
        break;

    case 0x0A4:
    /*---------------------------------------------------------------*/
    /* Diagnose 0A4: Synchronous I/O (Standard CMS Blocksize)        */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(syncblk_io) (r1, r2, regs);
//      logmsg ("Diagnose X\'0A4\': CC=%d, R15=%8.8X\n",      /*debug*/
//              regs->psw.cc, regs->GR_L(15));                 /*debug*/
        break;

    case 0x0A8:
    /*---------------------------------------------------------------*/
    /* Diagnose 0A8: Synchronous General I/O                         */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(syncgen_io) (r1, r2, regs);
//      logmsg ("Diagnose X\'0A8\': CC=%d, R15=%8.8X\n",      /*debug*/
//              regs->psw.cc, regs->GR_L(15));                 /*debug*/
        break;

    case 0x0B0:
    /*---------------------------------------------------------------*/
    /* Diagnose 0B0: Access Re-IPL Data                              */
    /*---------------------------------------------------------------*/
        ARCH_DEP(access_reipl_data) (r1, r2, regs);
        break;

    case 0x0DC:
    /*---------------------------------------------------------------*/
    /* Diagnose 0DC: Control Application Monitor Record Collection   */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        regs->GR_L(r2) = 0;
        regs->psw.cc = 0;
        break;

    case 0x210:
    /*---------------------------------------------------------------*/
    /* Diagnose 210: Retrieve Device Information                     */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(device_info) (r1, r2, regs);
        break;

    case 0x214:
    /*---------------------------------------------------------------*/
    /* Diagnose 214: Pending Page Release                            */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(diag_ppagerel) (r1, r2, regs);
        break;


    case 0x220:
    /*---------------------------------------------------------------*/
    /* Diagnose 220: TOD Epoch                                       */
    /*---------------------------------------------------------------*/
        ODD_CHECK(r2, regs);

        switch(regs->GR_L(r1))
        {
            case 0:
                /* Obtain TOD features */
                regs->GR_L(r2)  =0xc0000000;
                regs->GR_L(r2+1)=0x00000000;
                break;
            case 1:
                /* Obtain TOD offset to real TOD in R2, R2+1 */
                regs->GR_L(r2)  = (regs->tod_epoch >> 24) & 0xFFFFFFFF;
                regs->GR_L(r2+1)= (regs->tod_epoch << 8) & 0xFFFFFFFF;
                break;
            default:
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        }
        break;


    case 0x23C:
    /*---------------------------------------------------------------*/
    /* Diagnose 23C: Address Space Services                          */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        regs->GR_L(r2) = 0;
        break;


#if defined(FEATURE_VM_BLOCKIO)
    case 0x250:
    /*---------------------------------------------------------------*/
    /* Diagnose 250: Standardized Block I/O                          */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(vm_blockio) (r1, r2, regs);
        break;
#endif /*defined(FEATURE_VM_BLOCKIO)*/


    case 0x260:
    /*---------------------------------------------------------------*/
    /* Diagnose 260: Access Certain Virtual Machine Information      */
    /*---------------------------------------------------------------*/
        ARCH_DEP(vm_info) (r1, r2, regs);
        break;

    case 0x264:
    /*---------------------------------------------------------------*/
    /* Diagnose 264: CP Communication                                */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        PTT_ERR("*DIAG264",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
        regs->psw.cc = 0;
        break;

    case 0x270:
    /*---------------------------------------------------------------*/
    /* Diagnose 270: Pseudo Timer Extended                           */
    /*---------------------------------------------------------------*/
        ARCH_DEP(pseudo_timer) (code, r1, r2, regs);
        break;

    case 0x274:
    /*---------------------------------------------------------------*/
    /* Diagnose 274: Set Timezone Interrupt Flag                     */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        PTT_ERR("*DIAG274",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
        regs->psw.cc = 0;
        break;
#endif /*FEATURE_EMULATE_VM*/


#ifdef FEATURE_PROGRAM_DIRECTED_REIPL
    case 0x308:
    /*---------------------------------------------------------------*/
    /* Diagnose 308: IPL functions                                   */
    /*---------------------------------------------------------------*/
        switch(r2)
        {
            TID   tid;                              /* Thread identifier         */
            char *ipltype;                          /* "ipl" or "iplc"           */
            int   rc;

        case DIAG308_IPL_CLEAR:
            ipltype = "iplc";
            goto diag308_cthread;
        case DIAG308_IPL_NORMAL:
            ipltype = "ipl";
        diag308_cthread:
            rc = create_thread(&tid, DETACHED, stop_cpus_and_ipl, ipltype, "Stop cpus and ipl");
            if(rc)
                WRMSG(HHC00102, "E", strerror(rc));
            regs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT(regs);
            break;
        case DIAG308_SET_PARAM:
            /* INCOMPLETE */
            regs->GR(1) = DIAG308_RC_OK;
            break;
        case DIAG308_STORE_PARAM:
            /* INCOMPLETE */
            regs->GR(1) = DIAG308_RC_OK;
            break;
        default:
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        } /* end switch(r2) */
        break;
#endif /*FEATURE_PROGRAM_DIRECTED_REIPL*/


#ifdef FEATURE_HERCULES_DIAGCALLS
    case 0xF00:
    /*---------------------------------------------------------------*/
    /* Diagnose F00: Hercules normal mode                            */
    /*---------------------------------------------------------------*/
        /* If diag8opt is not enabled then we are not allowed
         * to manipulate the real machine i.e. hercules itself
         */
        if(!(sysblk.diag8opt & DIAG8CMD_ENABLE))
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

        sysblk.inststep = 0;
        SET_IC_TRACE;
        break;

    case 0xF04:
    /*---------------------------------------------------------------*/
    /* Diagnose F04: Hercules single step mode                       */
    /*---------------------------------------------------------------*/
        /* If diag8opt is not enabled then we are not allowed
         * to manipulate the real machine i.e. hercules itself
         */
        if(!(sysblk.diag8opt & DIAG8CMD_ENABLE))
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

        sysblk.inststep = 1;
        SET_IC_TRACE;
        break;

    case 0xF08:
    /*---------------------------------------------------------------*/
    /* Diagnose F08: Hercules get instruction counter                */
    /*---------------------------------------------------------------*/
        regs->GR_L(r1) = (U32)INSTCOUNT(regs);
        break;

    case 0xF0C:
    /*---------------------------------------------------------------*/
    /* Diagnose F0C: Set/reset bad frame indicator                   */
    /*---------------------------------------------------------------*/
        /* If diag8opt is not enabled then we are not allowed
         * to manipulate the real machine i.e. hercules itself
         */
        if(!(sysblk.diag8opt & DIAG8CMD_ENABLE))
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

        /* Load 4K block address from R2 register */
        n = regs->GR_L(r2) & ADDRESS_MAXWRAP(regs);

        /* Convert real address to absolute address */
        n = APPLY_PREFIXING (n, regs->PX);

        /* Addressing exception if block is outside main storage */
        if ( n > regs->mainlim )
        {
            ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);
            break;
        }

        /* Update the storage key from R1 register bit 31 */
        STORAGE_KEY(n, regs) &= ~(STORKEY_BADFRM);
        STORAGE_KEY(n, regs) |= regs->GR_L(r1) & STORKEY_BADFRM;

        break;

    case 0xF10:
    /*---------------------------------------------------------------*/
    /* Diagnose F10: Hercules CPU stop                               */
    /*---------------------------------------------------------------*/
        /* If diag8opt is not enabled then we are not allowed
         * to manipulate the real machine i.e. hercules itself
         */
        if(!(sysblk.diag8opt & DIAG8CMD_ENABLE))
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

        regs->cpustate = CPUSTATE_STOPPING;
        ON_IC_INTERRUPT(regs);
        break;

#if defined(OPTION_DYNAMIC_LOAD)
    case 0xF14:
    /*---------------------------------------------------------------*/
    /* Diagnose F14: Hercules DLL interface                          */
    /*---------------------------------------------------------------*/
        ARCH_DEP(diagf14_call) (r1, r2, regs);
        break;
#endif /*defined(OPTION_DYNAMIC_LOAD)*/

#if defined(_FEATURE_HOST_RESOURCE_ACCESS_FACILITY)
    case 0xF18:
    /*---------------------------------------------------------------*/
    /* Diagnose F18: Hercules Access Host Resource                   */
    /*---------------------------------------------------------------*/
        ARCH_DEP(diagf18_call) (r1, r2, regs);
        break;
#endif /* defined(_FEATURE_HOST_RESOURCE_ACCESS_FACILITY) */

#if !defined(NO_SIGABEND_HANDLER)
    /* The following diagnose calls cause a exigent (non-repressible)
       machine check, and are used for test purposes only *JJ */
    case 0xFE8:
    /*---------------------------------------------------------------*/
    /* Diagnose FE8: Simulate Illegal Instruction                    */
    /*---------------------------------------------------------------*/
        raise(SIGILL);
        break;

    case 0xFEC:
    /*---------------------------------------------------------------*/
    /* Diagnose FEC: Simulate Floating Point Exception               */
    /*---------------------------------------------------------------*/
        raise(SIGFPE);
        break;

    case 0xFF0:
    /*---------------------------------------------------------------*/
    /* Diagnose FF0: Simulate Segment Violation                      */
    /*---------------------------------------------------------------*/
        CRASH();
        break;

    case 0xFF4:
    /*---------------------------------------------------------------*/
    /* Diagnose FF4: Simulate BUS Error                              */
    /*---------------------------------------------------------------*/
        raise(SIGBUS);
        break;

    case 0xFF8:
    /*---------------------------------------------------------------*/
    /* Diagnose FF8: Simulate Loop                                   */
    /*---------------------------------------------------------------*/
        while(1);
        /* (never reached) */

    case 0xFFC:
    /*---------------------------------------------------------------*/
    /* Diagnose FFC: Simulate Wait                                   */
    /*---------------------------------------------------------------*/
        SLEEP(300);
        break;
#endif /*!defined(NO_SIGABEND_HANDLER)*/

#endif /*FEATURE_HERCULES_DIAGCALLS*/


    default:
    /*---------------------------------------------------------------*/
    /* Diagnose xxx: Invalid function code                           */
    /*---------------------------------------------------------------*/

        if( HDC4(debug_diagnose, code, r1, r2, regs) )
            return;

        /* Power Off diagnose on 4361, 9371, 9373, 9375, 9377, 9221: */
        /*                                                           */
        /*          DS 0H                                            */
        /*          DC X'8302',S(SHUTDATA)     MUST BE R0 AND R2     */
        /*          ...                                              */
        /*          DS 0H                                            */
        /* SHUTDATA DC X'0000FFFF'             MUST BE X'0000FFFF'   */

        if (0 == r1 && 2 == r2
             && sysblk.cpuversion != 0xFF
             && (sysblk.cpumodel == 0x4361
              || (sysblk.cpumodel & 0xFFF9) == 0x9371   /* (937X) */
              || sysblk.cpumodel == 0x9221)
           )
        {
            if (0x0000FFFF == ARCH_DEP(vfetch4)(effective_addr2, b2, regs))
            {
                /* If diag8opt is not enabled then we are not allowed
                 * to manipulate the real machine i.e. hercules itself
                 */
                if(!(sysblk.diag8opt & DIAG8CMD_ENABLE))
                    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

                regs->cpustate = CPUSTATE_STOPPING;
                ON_IC_INTERRUPT(regs);

                /* Release the configuration */
                do_shutdown();

                /* Power Off: exit hercules */
                exit(0);
            }
        }

#if defined(FEATURE_S370_CHANNEL) && defined(OPTION_NOP_MODEL158_DIAGNOSE)
        if (regs->cpumodel != 0x0158)
#endif
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        return;

    } /* end switch(code) */

    return;

} /* end function diagnose_call */


#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "diagnose.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "diagnose.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
