/* DIAGNOSE.C   (C) Copyright Roger Bowler, 2000-2012                */
/*              ESA/390 Diagnose Functions                           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements miscellaneous diagnose functions           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Hercules-specific diagnose calls by Jay Maynard.             */
/*      Set/reset bad frame indicator call by Jan Jaeger.            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DIAGNOSE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#ifndef COMPILE_THIS_ONLY_ONCE
#define COMPILE_THIS_ONLY_ONCE

#undef ATTRIBUTE_PACKED
#if defined(_MSVC_)
 #pragma pack(push)
 #pragma pack(1)
 #define ATTRIBUTE_PACKED
#else
 #define ATTRIBUTE_PACKED __attribute__((packed))
#endif

/* Start of headers and defines copied from S390-tools-2.14.0 include/boot/ipl.h */
/* The copied headers have been modified */

/* IPL Parameter List header */
struct ipl_pl_hdr {
  uint32_t len;
  uint8_t  flags;
  uint8_t  reserved1[2];
  uint8_t  version;
} ATTRIBUTE_PACKED;
#define IPL_FLAG_SECURE    0x40
#define IPL_MAX_SUPPORTED_VERSION 0

//  /* IPL Parameter Block header */
//  struct ipl_pb_hdr {
//    uint32_t len;
//    uint8_t  pbt;
//  } ATTRIBUTE_PACKED;

/* IPL Parameter Block 0 with common fields */
struct ipl_pb0_common {
  uint32_t len;
  uint8_t  pbt;
  uint8_t  flags;
  uint8_t  reserved1[2];
  uint8_t  loadparm[8];
  uint8_t  reserved2[84];
} ATTRIBUTE_PACKED;
#define IPL_RB_COMPONENT_FLAG_SIGNED  0x80
#define IPL_RB_COMPONENT_FLAG_VERIFIED  0x40
/* Following define values copied from linux-5.8.17 arch/s390/include/uapi/asmi/ipl.h */
#define IPL_TYPE_UNKNOWN    1
#define IPL_TYPE_CCW        2
#define IPL_TYPE_FCP        4
#define IPL_TYPE_FCP_DUMP   8
#define IPL_TYPE_NSS       16
#define IPL_TYPE_NVME      32
/* Following define value copied from S390-tools-2.14.0 include/boot/ipl.h */
#define IPL_TYPE_PV       0x5

/* IPL Parameter Block 0 for CCW */
struct ipl_pb0_ccw {
  uint32_t len;
  uint8_t  pbt;
  uint8_t  flags;
  uint8_t  reserved1[2];
  uint8_t  loadparm[8];
  uint8_t  reserved2[84];
  uint8_t  reserved3[1];
  uint8_t  reserved6 : 5,
           ssid : 3;
  uint16_t devno;
  uint8_t  vm_flags;
  uint8_t  reserved4[3];
  uint32_t vm_parm_len;
  uint8_t  nss_name[8];
  uint8_t  vm_parm[64];
  uint8_t  reserved5[8];
} ATTRIBUTE_PACKED;

//  /* IPL Parameter Block 0 for FCP */
//  struct ipl_pb0_fcp {
//    uint32_t len;
//    uint8_t  pbt;
//    uint8_t  reserved1[3];
//    uint8_t  loadparm[8];
//    uint8_t  reserved2[304];
//    uint8_t  opt;
//    uint8_t  reserved3[3];
//    uint8_t  cssid;
//    uint8_t  reserved4[1];
//    uint8_t  devno;
//    uint8_t  reserved5[4];
//    uint64_t wwpn;
//    uint64_t lun;
//    uint32_t bootprog;
//    uint8_t  reserved6[12];
//    uint64_t br_lba;
//    uint32_t scp_data_len;
//    uint8_t  reserved7[260];
//    uint8_t  scp_data[];
//  } ATTRIBUTE_PACKED;

//  /* Structure must not have any padding */
//  struct ipl_pb0_pv_comp {
//    uint64_t tweak_pref;
//    uint64_t addr;
//    uint64_t len;
//  };
//  STATIC_ASSERT(sizeof(struct ipl_pb0_pv_comp) == 3 * 8)

//  /* IPL Parameter Block 0 for PV */
//  struct ipl_pb0_pv {
//    uint32_t len;
//    uint8_t  pbt;
//    uint8_t  reserved1[3];
//    uint8_t  loadparm[8];
//    uint8_t  reserved2[84];
//    uint8_t  reserved3[3];
//    uint8_t  version;
//    uint8_t  reserved4[4];
//    uint32_t num_comp;
//    uint64_t pv_hdr_addr;
//    uint64_t pv_hdr_size;
//    struct ipl_pb0_pv_comp components[];
//  } ATTRIBUTE_PACKED;
//  #define IPL_PARM_BLOCK_VERSION    0x1

struct ipl_parameter_block {
  struct ipl_pl_hdr hdr;
  union {
//      struct ipl_pb_hdr pb0_hdr;
    struct ipl_pb0_common common;
    struct ipl_pb0_ccw ccw;
//      struct ipl_pb0_fcp fcp;
//      struct ipl_pb0_pv pv;
//      char raw[PAGE_SIZE - sizeof(struct ipl_pl_hdr)];
  };
} ATTRIBUTE_PACKED;

/* End of headers and defines copied from S390-tools-2.14.0 include/boot/ipl.h */

#if defined(_MSVC_)
 #pragma pack(pop)
#endif

/*-------------------------------------------------------------------*/
/*                                                                   */
/*-------------------------------------------------------------------*/

/* Diagnose 308 function subcodes */
#define DIAG308_START_KERNEL       1
#define DIAG308_REL_HSA            2
#define DIAG308_LOAD_CLEAR         3
#define DIAG308_LOAD_NORMAL_DUMP   4
#define DIAG308_SET                5
#define DIAG308_STORE              6
#define DIAG308_LOAD_NORMAL        7
#define DIAG308_SET_PV             8
#define DIAG308_UNPACK_PV         10

/* Diagnose 308 return codes */
#define DIAG308_RC_OK        0x0001
#define DIAG308_RC_NOCONFIG  0x0102

#endif // COMPILE_THIS_ONLY_ONCE

#ifndef STOP_CPUS_AND_IPL
#define STOP_CPUS_AND_IPL
#if defined( FEATURE_PROGRAM_DIRECTED_REIPL )
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
#endif /* defined( FEATURE_PROGRAM_DIRECTED_REIPL ) */
#endif // STOP_CPUS_AND_IPL

/*-------------------------------------------------------------------*/
/* Diagnose instruction                                              */
/*-------------------------------------------------------------------*/
void ARCH_DEP( diagnose_call )( REGS* regs, int r1, int r3, int b2, VADR effective_addr2 )
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

        ARCH_DEP(diagnose_002) (regs, r1, r3);

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
          /* and r1/r3 must contain C'POWEROFF' in EBCDIC */
          || regs->GR_L(r1) != 0xD7D6E6C5
          || regs->GR_L(r3) != 0xD9D6C6C6)
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
        regs->psw.cc = ARCH_DEP(mssf_call) (r1, r3, regs);
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
        ARCH_DEP(diag204_call) (r1, r3, regs);
        regs->psw.cc = 0;
        break;

    case 0x224:
    /*---------------------------------------------------------------*/
    /* Diagnose 224: CPU Names                                       */
    /*---------------------------------------------------------------*/
        ARCH_DEP(diag224_call) (r1, r3, regs);
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
        regs->psw.cc = ARCH_DEP(vm_timebomb) (r1, r3, regs);
        break;
#endif

#ifdef FEATURE_EMULATE_VM
    case 0x000:
    /*---------------------------------------------------------------*/
    /* Diagnose 000: Store Extended Identification Code              */
    /*---------------------------------------------------------------*/
        ARCH_DEP(extid_call) (r1, r3, regs);
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
        regs->psw.cc = ARCH_DEP(cpcmd_call) (r1, r3, regs);
        break;

    case 0x00C:
    /*---------------------------------------------------------------*/
    /* Diagnose 00C: Pseudo Timer                                    */
    /*---------------------------------------------------------------*/
        ARCH_DEP(pseudo_timer) (code, r1, r3, regs);
        break;

    case 0x024:
    /*---------------------------------------------------------------*/
    /* Diagnose 024: Device Type and Features                        */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(diag_devtype) (r1, r3, regs);
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
        regs->GR_L(r3) = 44;
        regs->psw.cc = 2;
        break;

    case 0x0A4:
    /*---------------------------------------------------------------*/
    /* Diagnose 0A4: Synchronous I/O (Standard CMS Blocksize)        */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(syncblk_io) (r1, r3, regs);
//      logmsg ("Diagnose X\'0A4\': CC=%d, R15=%8.8X\n",      /*debug*/
//              regs->psw.cc, regs->GR_L(15));                 /*debug*/
        break;

    case 0x0A8:
    /*---------------------------------------------------------------*/
    /* Diagnose 0A8: Synchronous General I/O                         */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(syncgen_io) (r1, r3, regs);
//      logmsg ("Diagnose X\'0A8\': CC=%d, R15=%8.8X\n",      /*debug*/
//              regs->psw.cc, regs->GR_L(15));                 /*debug*/
        break;

    case 0x0B0:
    /*---------------------------------------------------------------*/
    /* Diagnose 0B0: Access Re-IPL Data                              */
    /*---------------------------------------------------------------*/
        ARCH_DEP(access_reipl_data) (r1, r3, regs);
        break;

    case 0x0DC:
    /*---------------------------------------------------------------*/
    /* Diagnose 0DC: Control Application Monitor Record Collection   */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        regs->GR_L(r3) = 0;
        regs->psw.cc = 0;
        break;

    case 0x210:
    /*---------------------------------------------------------------*/
    /* Diagnose 210: Retrieve Device Information                     */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(device_info) (r1, r3, regs);
        break;

    case 0x214:
    /*---------------------------------------------------------------*/
    /* Diagnose 214: Pending Page Release                            */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(diag_ppagerel) (r1, r3, regs);
        break;


    case 0x220:
    /*---------------------------------------------------------------*/
    /* Diagnose 220: TOD Epoch                                       */
    /*---------------------------------------------------------------*/
        ODD_CHECK(r3, regs);

        switch(regs->GR_L(r1))
        {
            case 0:
                /* Obtain TOD features */
                regs->GR_L(r3)  =0xc0000000;
                regs->GR_L(r3+1)=0x00000000;
                break;
            case 1:
                /* Obtain TOD offset to real TOD in R2, R2+1 */
                regs->GR_L(r3)  = (regs->tod_epoch >> 24) & 0xFFFFFFFF;
                regs->GR_L(r3+1)= (regs->tod_epoch << 8) & 0xFFFFFFFF;
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
        regs->GR_L(r3) = 0;
        break;


#if defined(FEATURE_VM_BLOCKIO)
    case 0x250:
    /*---------------------------------------------------------------*/
    /* Diagnose 250: Standardized Block I/O                          */
    /*---------------------------------------------------------------*/
        regs->psw.cc = ARCH_DEP(vm_blockio) (r1, r3, regs);
        break;
#endif /*defined(FEATURE_VM_BLOCKIO)*/


    case 0x260:
    /*---------------------------------------------------------------*/
    /* Diagnose 260: Access Certain Virtual Machine Information      */
    /*---------------------------------------------------------------*/
        ARCH_DEP(vm_info) (r1, r3, regs);
        break;

    case 0x264:
    /*---------------------------------------------------------------*/
    /* Diagnose 264: CP Communication                                */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        PTT_ERR("*DIAG264",regs->GR_L(r1),regs->GR_L(r3),regs->psw.IA_L);
        regs->psw.cc = 0;
        break;

    case 0x270:
    /*---------------------------------------------------------------*/
    /* Diagnose 270: Pseudo Timer Extended                           */
    /*---------------------------------------------------------------*/
        ARCH_DEP(pseudo_timer) (code, r1, r3, regs);
        break;

    case 0x274:
    /*---------------------------------------------------------------*/
    /* Diagnose 274: Set Timezone Interrupt Flag                     */
    /*---------------------------------------------------------------*/
        /* This function is implemented as a no-operation */
        PTT_ERR("*DIAG274",regs->GR_L(r1),regs->GR_L(r3),regs->psw.IA_L);
        regs->psw.cc = 0;
        break;
#endif /*FEATURE_EMULATE_VM*/


    case 0x308:
    /*---------------------------------------------------------------*/
    /* Diagnose 308: IPL functions                                   */
    /*---------------------------------------------------------------*/
        PTT_ERR("*DIAG308",regs->GR_G(r1),regs->GR_G(r3),regs->psw.IA_L);

        switch(regs->GR_L(r3))
        {
#if defined(FEATURE_PROGRAM_DIRECTED_REIPL)
            TID   tid;                              /* Thread identifier         */
            char *ipltype;                          /* "ipl" or "iplc"           */
            int   rc;
#endif /*defined(FEATURE_PROGRAM_DIRECTED_REIPL)*/

        case DIAG308_START_KERNEL:
            /*-------------------------------------------------------*/
            /* Linux for z uses this function code, without a        */
            /* defined value, in a function named start_kernel (see  */
            /* ZIPL stage3.c). The Diagnose instruction is DIAG      */
            /* 1,1,0x308, with GPR 1 containing DIAG308_START_KERNEL */
            /* (i.e. equal to 1). This author has no idea what       */
            /* function start_kernel is expecting the Diagnose       */
            /* instruction to do (other than starting the kernel!),  */
            /* but the function does not check a return code, or     */
            /* anything else, and is prepared for the Diagnose       */
            /* instruction to program check. Fortunately, the        */
            /* function code DIAG308_START_KERNEL and the return     */
            /* code DIAG308_RC_OK both have the same value (i.e.     */
            /* equal to 1), so we'll simply return without doing     */
            /* anything, letting the function code become the        */
            /* return code.                                          */
            /*-------------------------------------------------------*/
            break;

#if defined(FEATURE_PROGRAM_DIRECTED_REIPL)
        case DIAG308_LOAD_CLEAR:
            ipltype = "iplc";
            goto diag308_cthread;
        case DIAG308_LOAD_NORMAL:
            ipltype = "ipl";
        diag308_cthread:
            rc = create_thread(&tid, DETACHED, stop_cpus_and_ipl, ipltype, "Stop cpus and ipl");
            if(rc)
                WRMSG(HHC00102, "E", strerror(rc));
            regs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT(regs);
            break;
        case DIAG308_SET:
            /* INCOMPLETE */
            regs->GR(1) = DIAG308_RC_OK;
            break;
#endif /*defined(FEATURE_PROGRAM_DIRECTED_REIPL)*/

        case DIAG308_STORE:
          {
            /*-------------------------------------------------------*/
            /* On entry                                              */
            /*   Rx    Rx must be an even numbered register          */
            /*         containing the real address of a 4K page      */
            /*         aligned storage area into which the IPL       */
            /*         parameter block will be copied.               */
            /* On return                                             */
            /*   Rx+1  Contains the return code.                     */
            /*-------------------------------------------------------*/
            RADR    stgarea;            /* Storage area real address */
            U32     headsize;
            U32     bodysize;
            struct ipl_parameter_block  ipb;
            int     lopasize;

            /* Program check if running problem state. */
            PRIV_CHECK(regs);

            /* Register Rx contains the real address of the storage area. */
            if(regs->psw.amode64) {
              stgarea = regs->GR_G(r1);
            } else {
              stgarea = regs->GR_L(r1);
            }

            /* Program check if Rx is not an even register number or     */
            /* the address contained in Rx is not on a 4K page boundary. */
            if (r1 & 1 || stgarea & 0xFFF)
            {
                ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
            }

            /* Ensure that the 4K storage area is addressable. */
            ARCH_DEP(validate_operand) (stgarea, USE_REAL_ADDR, 4095, ACCTYPE_WRITE, regs);

            /* Prepare the IPL parameter block. */
            memset(&ipb, 0, sizeof(ipb));
            /* Setup head and body values common to all. */
            ipb.hdr.version = IPL_MAX_SUPPORTED_VERSION;
            headsize = sizeof(ipb.hdr);
            memset(ipb.common.loadparm, 0x40, sizeof(ipb.common.loadparm));  /* EBCDIC spaces */
            lopasize = strlen(sysblk.loadparm);
            if (lopasize)
            {
                str_host_to_guest(ipb.common.loadparm, sysblk.loadparm, lopasize);
            }
            /* Setup head and body values specific to CCW. */
            bodysize = sizeof(ipb.ccw);
            STORE_FW(&ipb.hdr.len, (headsize + bodysize));
            STORE_FW(&ipb.ccw.len, bodysize);
            ipb.ccw.pbt = IPL_TYPE_CCW;
            STORE_HW(&ipb.ccw.devno, sysblk.ipldev);
            STRLCPY(ipb.ccw.vm_parm, "Hercules");  /* just to show its us! */

            /* Store the IPL parameter block in real storage */
            /* Note: vstorec copies a maximum of 256 bytes.  */
            ARCH_DEP(vstorec) (&ipb, (headsize+bodysize-1), stgarea, USE_REAL_ADDR, regs);

            /* Successful */
            regs->GR(r1+1) = DIAG308_RC_OK;
            break;
          }

        default:
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        } /* end switch(r3) */
        break;

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

        sysblk.instbreak = 0;
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

        sysblk.instbreak = 1;
        SET_IC_TRACE;
        break;

    case 0xF08:
    /*---------------------------------------------------------------*/
    /* Diagnose F08: Return Hercules instruction counter (32)        */
    /*---------------------------------------------------------------*/
        regs->GR_L( r1 ) = (U32) INSTCOUNT( regs );
        break;

    case 0xF09:
    /*---------------------------------------------------------------*/
    /* Diagnose F09: Return Hercules instruction counter (64)        */
    /*---------------------------------------------------------------*/
    {
        // Operand register r3 bits 32-47 specify the option code and
        // bits 48-63 specify the CPU Address for option code 1. For
        // option code 0, operand register r3 bits 48-63 are ignored.
        // Bits 0-31 of operand register r3 are also always ignored.
        // Any option code other than 0 or 1 causes a Specification
        // Exception Program Interrupt to occur.
        //
        // Option 0 = instruction count for entire system (all CPUs
        // together). Option 1 = instruction count for specific CPU
        // identified in bits 48-63 of r3.
        //
        // The register and bits that the 64-bit instruction count
        // is returned in depends on: 1) whether z/Architecture mode
        // is active or not, and 2) whether the specified operand-1
        // r1 register number is even or odd.
        //
        // If operand-1 register r1 is an even numbered register,
        // then the high-order bits 0-31 of the 64-bit instruction
        // count is returned in bits 32-63 of the even numbered
        // register, the low-order bits 32-63 the 64-bit instruction
        // count is returned in bits 32-63 of the r1+1 odd numbered
        // register and bits 0-31 of each register remain unmodified.
        //
        // If operand-1 register r1 specifies an odd numbered register
        // however, then the 64-bit instruction count is returned in
        // bits 0-63 of register r1 in z/Architecture mode, whereas a
        // Specification Exception Program Check Interrupt occurs in
        // both ESA/390 and System/370 architecture modes as 64-bit
        // registers don't exist in either of those architectures.
        //
        // Precluding a Specification Exception Program Interrupt,
        // Condition Code 0 is returned for option 0, whereas for
        // option 1, Condition Code 0 is only returned if the CPU
        // specified in bits 48-63 of the operand-3 register r3 is
        // currently valid and online. Otherwise if the specified
        // CPU is offline or does not exist in the configuration,
        // Condition Code 3 is returned and the r1 or r1 and r1+1
        // return value register(s) is/are not modified.

        U64   instcount=0;                  // Instruction count
        U16   opt = regs->GR_LHH( r3 );     // Option code

        if (regs->arch_mode != ARCH_900_IDX)// Not 64-bit architecture?
            ODD_CHECK( r1, regs );          // 64-bit regs don't exist!

        if (opt > 1)                        // Unsupported option?
        {
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
        }
        else if (opt == 1)                  // Count for specific CPU?
        {
            int cpu = regs->GR_LHL( r3 );   // Get desired CPU from r3

            if (cpu < 0 || cpu >= sysblk.maxcpu || !IS_CPU_ONLINE( cpu ))
            {
                regs->psw.cc = 3;           // CPU is invalid/offline
                break;                      // We are done
            }

            /* Retrieve the requested CPU's instruction count */
            instcount = sysblk.regs[ cpu ]->prevcount  // (U64)
                      + sysblk.regs[ cpu ]->instcount; // (U32)
            regs->psw.cc = 0;
        }
        else if (opt == 0)                  // Global system counter?
        {
            instcount = sysblk.instcount;   // Get total for ALL CPUs
            regs->psw.cc = 0;
        }

        /* Return the instruction count in the requested register(s) */
        if (r1 & 1)
            regs->GR_G( r1 ) = instcount;   // Contiguous 64-bit count
        else
        {
            /* Place the high-order 32 bits of the 64-bit count into
               bits 32-63 of the even numbered register and place the
               low-order 32 bits of the 64-bit count into bits 32-63
               of the r1+1 odd numbered register. For S/370 and S/390
               mode this corresponds to bits 0-31 of the register pair
               since registers are only 32 bits wide in 370 and 390.
            */
            regs->GR_L( r1    ) = (U32)((instcount >> 32) & 0xFFFFFFFF);
            regs->GR_L( r1 + 1) = (U32)((instcount      ) & 0xFFFFFFFF);
        }

        break;
    }

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
        n = regs->GR_L(r3) & ADDRESS_MAXWRAP(regs);

        /* Convert real address to absolute address */
        n = APPLY_PREFIXING (n, regs->PX);

        /* Addressing exception if block is outside main storage */
        if ( n > regs->mainlim )
        {
            ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);
            break;
        }

        /* Update bad-frame bit based on low-order bit of R1 register.
           Note: we must use the internal "_xxx_storage_key" functions
           to directly set/clear the internal STORKEY_BADFRM bit.
        */
        if (regs->GR_L(r1) & STORKEY_BADFRM)
            ARCH_DEP( _or_storage_key )( n, STORKEY_BADFRM, SKEY_K );
        else
            ARCH_DEP( _and_storage_key )( n, STORKEY_BADFRM, SKEY_K );

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

#if defined(_FEATURE_HOST_RESOURCE_ACCESS_FACILITY)
    case 0xF18:
    /*---------------------------------------------------------------*/
    /* Diagnose F18: Hercules Access Host Resource                   */
    /*---------------------------------------------------------------*/
        ARCH_DEP(diagf18_call) (r1, r3, regs);
        break;
#endif /* defined(_FEATURE_HOST_RESOURCE_ACCESS_FACILITY) */

    case 0xFF8:
    /*---------------------------------------------------------------*/
    /* Diagnose FF8: Hercules Infinite Loop (Malfunctioning CPU)     */
    /*---------------------------------------------------------------*/
        while(1);   /* (loop forever)  */
        break;      /* (never reached) */

    case 0xFFC:
    /*---------------------------------------------------------------*/
    /* Diagnose FFC: Hercules SLOW Instruction (Malfunctioning CPU)  */
    /*---------------------------------------------------------------*/
        SLEEP(300); /* (300 seconds = 5 minutes!) */
        break;

    case 0xFFD:
    /*---------------------------------------------------------------*/
    /* Diagnose FFD: Hercules Dummy "Slow(?)" Instruction            */
    /*---------------------------------------------------------------*/
    {
        /* r1 = microseconds */
        unsigned int secs  = regs->GR_L(r1) / ONE_MILLION;
        unsigned int usecs = regs->GR_L(r1) % ONE_MILLION;
        unsigned int i;
        for (i=0; i < secs; ++i)
            SLEEP(1);       /* (sleep one second at a time) */
        if (usecs)
            USLEEP(usecs);  /* (remaining microseconds, if any) */
        break;
    }

#endif /*FEATURE_HERCULES_DIAGCALLS*/

    default:
    /*---------------------------------------------------------------*/
    /* Diagnose xxx: Invalid function code or Power-Off diagnose     */
    /*---------------------------------------------------------------*/

        if( HDC4(debug_diagnose, code, r1, r3, regs) )
            return;

        /* Power Off diagnose on 4361, 9371, 9373, 9375, 9377, 9221: */
        /*                                                           */
        /*          DS 0H                                            */
        /*          DC X'8302',S(SHUTDATA)     MUST BE R0 AND R2     */
        /*          ...                                              */
        /*          DS 0H                                            */
        /* SHUTDATA DC X'0000FFFF'             MUST BE X'0000FFFF'   */

        if (0 == r1 && 2 == r3
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

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "diagnose.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "diagnose.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
