/* HSTRUCTS.H   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright TurboHercules, SAS 2011                */
/*              (C) and others 2013-2023                             */
/*              Hercules Structure Definitions                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

//      This header auto-#included by 'hercules.h'...
//
//      The <config.h> header and other required headers are
//      presumed to have already been #included ahead of it...

//---------------------------------------------------------------------
//             *** IMPORTANT PROGRAMMING NOTE ***
//---------------------------------------------------------------------
//
//  Normally, using any type of architecturally dependent constant
//  in any of the below REGS, SYSBLK or DEVBLK structures (such as
//  RADR) would be considered a serious error, since the width of
//  such constants varies depending on which architecture is being
//  built at the time. RADR for example is 32-bits wide (U32) for
//  the S370 and S390 architectures (__GEN_ARCH == 370 or 390) but
//  is 64-bits wide (U64) for z/Arch (__GEN_ARCH == 900), which
//  would result in a different size/layout for each struct (REGS,
//  SYSBLK and DEVBLK), which needless to say would be VERY BAD!
//
//  Our "RADR" constant however (which is used in several different
//  places in the below structures) is an exception to the rule. It
//  is completely safe to use RADR in the below structs because it
//  is always #defined as a 64-bit wide U64 whenever build support
//  for z/Architecture SIE is defined (i.e. _FEATURE_ZSIE), which
//  forces RADR to always be #defined as U64 regardless of which
//  build architecture is being built. When the 370 or 390 build
//  architectures are being built for example, RADR -- which would
//  normally be #defined as U32 for 370 and 390 -- is nevertheless
//  defined as U64 instead via the following code in "feature.h":
//
//          #if !defined( _FEATURE_ZSIE )
//            #define RADR    U32
//            #define F_RADR  "%8.8"PRIX32
//          #else
//            #define RADR    U64
//            #define F_RADR  "%16.16"PRIX64
//          #endif
//
//  Thus it is safe to use 'RADR' in any of the below structures
//  since it should thus *always* end up being defined as a U64.
//
//  Using any OTHER type of build architecture dependent constant
//  in any of the below strutures would be a SERIOUS ARCHITECTURE
//  DEPENDENCY VIOLATION!
//
//---------------------------------------------------------------------

#ifndef _HSTRUCTS_H
#define _HSTRUCTS_H

#include "hercules.h"
#include "opcode.h"

#include "telnet.h"         // Need telnet_t
#include "stfl.h"           // Need STFL_HERC_BY_SIZE
#include "cckd.h"           // Need CCKD structs
#include "transact.h"       // Need Transactional Execution Facility

#if !defined( _FEATURE_SIE )
  /*------------------------------------------------------------*/
  /* FIXME: all RADR fields should be changed to U64 instead!   */
  /* Refer to *** IMPORTANT PROGRAMMING NOTE *** further above! */
  /*------------------------------------------------------------*/
  #error RADR struct use causes arch dep violation for non-SIE builds!
#endif


/*-------------------------------------------------------------------*/
/*              Typedefs for CPU bitmap fields                       */
/*-------------------------------------------------------------------*/
/* A CPU bitmap contains one bit for each processing engine.  The    */
/* width of the bitmap depends on the maximum number of processing   */
/* engines that was selected at build time.                          */
/*                                                                   */
/* Due to GCC and MSVC limitations, a 'MAX_CPU_ENGS' value greater   */
/* than 64 (e.g. 128) is only supported on platforms whose long long */
/* integer size is actually 128 bits:                                */
/*                                                                   */
/* "As an extension the integer scalar type __int128 is supported    */
/*  for targets which have an integer mode wide enough to hold 128   */
/*  bits.  ... There is no support in GCC for expressing an integer  */
/*  constant of type __int128 for targets with long long integer     */
/*  less than 128 bits wide."                                        */
/*-------------------------------------------------------------------*/

#if MAX_CPU_ENGS <= 0
  #error MAX_CPU_ENGS must be greater than zero!
#elif MAX_CPU_ENGS <= 32
    typedef U32                 CPU_BITMAP;
    #define F_CPU_BITMAP        "%8.8"PRIX32
#elif MAX_CPU_ENGS <= 64
    typedef U64                 CPU_BITMAP;
    #define F_CPU_BITMAP        "%16.16"PRIX64
#elif MAX_CPU_ENGS <= 128
  typedef __uint128_t         CPU_BITMAP;
  // ZZ FIXME: No printf format support for __uint128_t yet, so we will incorrectly display...
  #define SUPPRESS_128BIT_PRINTF_FORMAT_WARNING
  #define F_CPU_BITMAP        "%16.16"PRIX64
#else
  #error MAX_CPU_ENGS cannot exceed 128
#endif

/*-------------------------------------------------------------------*/
/*       Structure definition for CPU register context               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Note: REGS is very susceptable to performance problems due to     */
/*       key fields either crossing or split across cache line       */
/*       boundaries. In addition, if allocated in the stack, the     */
/*       allocation unit is frequently on an 8-byte boundary rather  */
/*       than on a cache-line boundary. Consequently, extreme        */
/*       attention is paid to the cache line boundaries in the REGS  */
/*       structure to ensure that fields don't cross 64-byte         */
/*       boundaries and as much attention as possible to ensure      */
/*       32-byte boundaries are not crossed.                         */
/*                                                                   */
/*-------------------------------------------------------------------*/
struct REGS {                           /* Processor registers       */

#define HDL_NAME_REGS   "REGS"          /* Eye-Catch NAME            */
#define HDL_VERS_REGS   "SDL 4.00"      /* Internal Version Number   */
#define HDL_SIZE_REGS   sizeof(REGS)

        BLOCK_HEADER;                   /* Name of block   REGS_CP00 */

        U64     cpuid;                  /* Formatted CPU ID          */
        U32     cpuserial;              /* CPU serial number         */
        U16     cpumodel;               /* CPU model number          */
        U8      cpuversion;             /* CPU version code          */
        U8      cpu_reserved;           /* (reserved for future use) */

                                        /* --- 64-byte cache line -- */
        SYSBLK *sysblk;                 /* Pointer to sysblk         */

        ALIGN_8
        U32     ints_state;             /* CPU Interrupts Status     */
        U32     ints_mask;              /* Respective Interrupts Mask*/
        CPU_BITMAP cpubit;              /* Only this CPU's bit is 1  */

        ALIGN_16
        BYTE    cpustate;               /* CPU stopped/started state */
        BYTE    _cpu_reserved2;         /* Available...              */
        U16     extccpu;                /* CPU causing external call */
        int     arch_mode;              /* Architectural mode        */
        BYTE   *ip;                     /* Mainstor inst address     */

        DW      px;                     /* Prefix register           */
#define PX_G    px.D
#define PX_L    px.F.L.F

        PSW     psw;                    /* Program status word       */

        ALIGN_128
        BYTE    malfcpu                 /* Malfuction alert flags    */
                    [ MAX_CPU_ENGS ];   /* for each CPU (1=pending)  */

        ALIGN_128
        BYTE    emercpu                 /* Emergency signal flags    */
                    [ MAX_CPU_ENGS ];   /* for each CPU (1=pending)  */

        /* AIA - Instruction fetch accelerator                       */

        ALIGN_128
        BYTE   *aip;                    /* Mainstor page address     */

        ALIGN_8
        BYTE   *aie;                    /* Mainstor page end address */

        DW      aiv;                    /* Virtual page address      */
#define AIV_G   aiv.D
#define AIV_L   aiv.F.L.F

        U64     bear;                   /* Breaking event address reg*/
        U64     bear_ex;                /* (same, but for EX/EXRL)   */

        ALIGN_128
        DW      gr[16];                 /* General registers         */
        U32     ar[16];                 /* Access registers          */
        U32     fpr[32];                /* FP registers              */
        U32     fpc;                    /* FP Control register       */

#define GR_G(_r)     gr[(_r)].D
#define GR_H(_r)     gr[(_r)].F.H.F       /* Fullword bits 0-31      */
#define GR_HHH(_r)   gr[(_r)].F.H.H.H.H   /* Halfword bits 0-15      */
#define GR_HHL(_r)   gr[(_r)].F.H.H.L.H   /* Halfword low, bits 16-31*/
#define GR_HHLCL(_r) gr[(_r)].F.H.H.L.B.L /* Character, bits 24-31   */
#define GR_L(_r)     gr[(_r)].F.L.F       /* Fullword low, bits 32-63*/
#define GR_LHH(_r)   gr[(_r)].F.L.H.H.H   /* Halfword bits 32-47     */
#define GR_LHL(_r)   gr[(_r)].F.L.H.L.H   /* Halfword low, bits 48-63*/
#define GR_LHHCH(_r) gr[(_r)].F.L.H.H.B.H /* Character, bits 32-39   */
#define GR_LA24(_r)  gr[(_r)].F.L.A.A     /* 24 bit addr, bits 40-63 */
#define GR_LA8(_r)   gr[(_r)].F.L.A.B     /* 24 bit addr, unused bits*/
#define GR_LHLCL(_r) gr[(_r)].F.L.H.L.B.L /* Character, bits 56-63   */
#define GR_LHLCH(_r) gr[(_r)].F.L.H.L.B.H /* Character, bits 48-55   */

#define AR(_r)       ar[(_r)]

        ALIGN_128
        DW           cr_struct[1+16+16];  /* Control registers       */
#define XR(_crn)     cr_struct[1+(_crn)]
#define CR_G(_r)     XR((_r)).D           /* Bits 0-63               */
#define CR_H(_r)     XR((_r)).F.H.F       /* Fullword bits 0-31      */
#define CR_HHH(_r)   XR((_r)).F.H.H.H.H   /* Halfword bits 0-15      */
#define CR_HHL(_r)   XR((_r)).F.H.H.L.H   /* Halfword low, bits 16-31*/
#define CR_L(_r)     XR((_r)).F.L.F       /* Fullword low, bits 32-63*/
#define CR_LHH(_r)   XR((_r)).F.L.H.H.H   /* Halfword bits 32-47     */
#define CR_LHHCH(_r) XR((_r)).F.L.H.H.B.H /* Character, bits 32-39   */
#define CR_LHL(_r)   XR((_r)).F.L.H.L.H   /* Halfword low, bits 48-63*/

#define CR_ASD_REAL    -1
#define CR_ALB_OFFSET  16
#define ARN(_arn)      ((_arn) >= USE_ARMODE ? ((_arn) & 0xF) : (_arn))

        U32     dxc;                    /* Data exception code       */

        DW      mc;                     /* Monitor Code              */
#define MC_G    mc.D
#define MC_L    mc.F.L.F

        DW      ea;                     /* Exception address         */
#define EA_G    ea.D
#define EA_L    ea.F.L.F

        DW      et;                     /* Execute Target address    */
#define ET_G    et.D
#define ET_L    et.F.L.F

        unsigned int                    /* Flags (cpu thread only)   */
                execflag:1,             /* 1=EXecuted instruction    */
                exrl:1,                 /* 1=EXRL, 0=EX instruction  */
                permode:1,              /* 1=PER active              */
                instinvalid:1,          /* 1=Inst field is invalid   */
                opinterv:1,             /* 1=Operator intervening    */
                checkstop:1,            /* 1=CPU is checkstop-ed     */
                hostint:1,              /* 1=Host generated interrupt*/
                host:1,                 /* REGS are hostregs         */
                guest:1,                /* REGS are guestregs        */
                diagnose:1;             /* Diagnose instr executing  */
        unsigned int                    /* Flags (intlock serialized)*/
                dummy:1,                /* 1=Dummy regs structure    */
                configured:1,           /* 1=CPU is online           */
                loadstate:1,            /* 1=CPU is in load state    */
                ghostregs:1,            /* 1=Ghost registers (panel) */
                invalidate:1,           /* 1=Do AIA/AEA invalidation */
                insttrace:1,            /* 1=Inst trace enabled      */
                breakortrace:1,         /* 1=Inst break/trace active */
                stepping:1,             /* 1=Inst stepping is active */
                stepwait:1,             /* 1=Wait in inst stepping   */
                sigp_reset:1,           /* 1=SIGP cpu reset received */
                sigp_ini_reset:1;       /* 1=SIGP initial cpu reset  */

        CACHE_ALIGN
        S64     tod_epoch;              /* TOD epoch for this CPU    */
        TOD     clkc;                   /* 0-7=Clock comparator epoch,
                                           8-63=Comparator bits 0-55 */
        S64     cpu_timer;              /* CPU timer                 */
        U32     todpr;                  /* TOD programmable register */

        S64     int_timer;              /* S/370 Interval timer      */
        S64     ecps_vtimer;            /* ECPS Virtual Int. timer   */
        S32     old_timer;              /* S/370 Interval timer int  */
        S32     ecps_oldtmr;            /* ECPS Virtual Int. tmr int */

        BYTE   *ecps_vtmrpt;            /* Pointer to VTMR or zero   */

        U64     rcputime;               /* Real CPU time used (us)   */
        U64     bcputime;               /* Base (reset) CPU time (us)*/
        U64     prevcount;              /* Previous instruction count*/
        U32     instcount;              /* Instruction counter       */
        U32     mipsrate;               /* Instructions per second   */
        U32     siocount;               /* SIO/SSCH counter          */
        U32     siosrate;               /* IOs per second            */
        U64     siototal;               /* Total SIO/SSCH count      */

        int     cpupct;                 /* Percent CPU busy          */
        U64     waittod;                /* Time of day last wait     */
        U64     waittime;               /* Wait time in interval     */
        U64     waittime_accumulated;   /* Wait time accumulated     */

        CACHE_ALIGN
        DAT     dat;                    /* Fields for DAT use        */

        U16     chanset;                /* Connected channel set     */
        U16     monclass;               /* Monitor event class       */
        U16     cpuad;                  /* CPU address for STAP      */
        BYTE    excarid;                /* Exception access register */
        BYTE    opndrid;                /* Operand access register   */
        BYTE    exinst[8];              /* Target of Execute (EX)    */
        BYTE   *mainstor;               /* -> Main storage           */
        BYTE   *storkeys;               /* -> Main storage key array */
        RADR    mainlim;                /* Central Storage limit or  */
                                        /* guest storage limit (SIE) */
        union
        {
            PSA_3XX *psa;               /* -> PSA for 370 and ESA    */
            PSA_900 *zpsa;              /* -> PSA for z/Arch         */
        };

    /*---------------------------------------------------------------*/
    /*                     PROGRAMMING NOTE                          */
    /*---------------------------------------------------------------*/
    /* The fields 'hostregs' and 'guestregs' have been moved outside */
    /* the scope of _FEATURE_SIE to reduce conditional code.         */
    /*                                                               */
    /*   'sysblk.regs[i]'   ALWAYS points to the host regs           */
    /*                                                               */
    /*   'hostregs'         ALWAYS = sysblk.regs[i]                  */
    /*                      in BOTH host regs and guest regs         */
    /*                                                               */
    /*   'guestregs'        ALWAYS = sysblk.regs[i]->guestregs       */
    /*                      in BOTH host regs and guest regs,        */
    /*                      but will be NULL until the first SIE     */
    /*                      instruction is executed on that CPU.     */
    /*                                                               */
    /*   'host'             ALWAYS 1 in host regs ONLY               */
    /*   'sie_active'       ALWAYS 1 in host regs ONLY whenever      */
    /*                      the SIE instruction is executing.        */
    /*                                                               */
    /*   'guest'            ALWAYS 1 in guest regs ONLY              */
    /*   'sie_mode'         ALWAYS 1 in guest regs ONLY              */
    /*                                                               */
    /*   'sie_state'        host real address of the SIEBK           */
    /*   'siebk'            mainstor  address of the SIEBK           */
    /*                                                               */
    /*---------------------------------------------------------------*/

        REGS   *hostregs;               /* Pointer to the hypervisor
                                           register context          */
        REGS   *guestregs;              /* Pointer to the guest
                                           register context          */

#if defined( _FEATURE_SIE )

        CACHE_ALIGN
        RADR    sie_state;              /* Address of the SIE state
                                           descriptor block or 0 when
                                           not running under SIE     */
        SIEBK  *siebk;                  /* SIE State Desc structure  */
        RADR    sie_px;                 /* Host address of guest px  */
        RADR    sie_mso;                /* Main Storage Origin       */
        RADR    sie_xso;                /* eXpanded Storage Origin   */
        RADR    sie_xsl;                /* eXpanded Storage Limit    */
        RADR    sie_rcpo;               /* Ref and Change Preserv.   */
        RADR    sie_scao;               /* System Contol Area        */
        S64     sie_epoch;              /* TOD offset in state desc. */
#endif
        unsigned int
                sie_active:1,           /* SIE active   (host  only) */
                sie_mode:1,             /* In SIE mode  (guest only) */
                sie_pref:1;             /* Preferred-storage mode    */
                                        /* (e.g. V=R guest)          */

        bool    sie_fld;                /* SIE2BK fld field provided */

        ALIGN_16
        U16     perc;                   /* PER code                  */
        RADR    peradr;                 /* PER address               */
        BYTE    peraid;                 /* PER access id             */
        RADR    periaddr;               /* Fetched instruct. address */

     /*
      * Making the following flags 'stand-alone' (instead of bit-
      * flags like they were) addresses a compiler bit-flag serial-
      * ization issue that occurs with the 'SYNCHRONIZE_CPUS' macro
      * used during synchronize broadcast (cpu<->cpu communication)
      */
        ALIGN_8
        bool    intwait;                /* true = Waiting on intlock */
        BYTE    inst[8];                /* Fetched instruction when
                                           instruction crosses a page
                                           boundary                  */
        BYTE    *invalidate_main;       /* Mainstor addr to invalidat*/

        CACHE_ALIGN
        PSW     captured_zpsw;          /* Captured-z/Arch PSW reg   */
#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
        CACHE_ALIGN
        VFREGS *vf;                     /* Vector Facility           */
#endif
        CACHE_ALIGN
        jmp_buf progjmp;                /* longjmp destination for
                                           program check return      */
        jmp_buf archjmp;                /* longjmp destination to
                                           switch architecture mode  */
        jmp_buf exitjmp;                /* longjmp destination for
                                           CPU thread exit           */
        COND    intcond;                /* CPU interrupt condition   */
        LOCK    *cpulock;               /* CPU lock for this CPU     */

        /* Mainstor address lookup accelerator                       */

        BYTE    aea_mode;               /* aea addressing mode       */

        int     aea_ar_struct[5+16];
#define AEA_AR(_arn) aea_ar_struct[5+(_arn)]

        BYTE    aea_common_struct[1+16+16];
#define AEA_COMMON(_asd) aea_common_struct[1+(_asd)]

        BYTE    aea_aleprot[16];        /* ale protected             */

     /* Function pointers */
        pi_func  program_interrupt;

     /* Active Facility List */
        BYTE    facility_list[ STFL_HERC_BY_SIZE ];

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
    /*---------------------------------------------------------------*/
    /*      Transactional-Execution Facility control fields          */
    /*---------------------------------------------------------------*/

        TDB     txf_tdb;                /* Internal TDB              */

        bool    txf_NTSTG;              /* true == NTSTG instruction */
        bool    txf_contran;            /* true == CONSTRAINED mode  */
        bool    txf_UPGM_abort;         /* true == transaction was
                                           aborted due to TAC_UPGM   */
        int     txf_aborts;             /* Abort count               */
        S32     txf_PPA;                /* PPA assistance level      */
        BYTE    txf_tnd;                /* Transaction nesting depth.
                                           Use txf_lock to access!   */

        BYTE    txf_ctlflag;            /* Flags for access mode
                                           change, float allowed     */

#define TXF_CTL_AR      0x08            /* AR reg changes allowed
                                           in transaction mode       */
#define TXF_CTL_FLOAT   0x04            /* Float and vector allowed
                                           in transaction mode       */
#define TXF_CTL_PIFC    0x03            /* PROG 'rupt filtering ctl. */

        U16     txf_higharchange;       /* Highest level that
                                           AR change is active       */

        U16     txf_highfloat;          /* Highest level that
                                           float is active           */

        U16     txf_instctr;            /* Instruction counter for
                                           contran and auto abort*/

        U16     txf_abortctr;           /* If non-zero, abort when
                                           txf_instctr >= this value */

        U16     txf_pifc;               /* Program-Interruption
                                           Filtering Control (PIFC)  */

#define TXF_PIFC_NONE       0           /* Exception conditions having
                                           classes 1, 2 or 3 always
                                           result in an interruption */

#define TXF_PIFC_LIMITED    1           /* Exception conditions having
                                           classes 1 or 2 result in an
                                           interruption; conditions
                                           having class 3 do not result
                                           in an interruption.       */

#define TXF_PIFC_MODERATE   2           /* Only exception conditions
                                           having class 1 result in an
                                           interruption; conditions
                                           having classes 2 or 3 do not
                                           result in an interruption */

#define TXF_PIFC_RESERVED   3           /* Reserved (invalid)        */

        U64     txf_tdba;               /* TBEGIN TDB address        */
        int     txf_tdba_b1;            /* TBEGIN op1 base address   */
        U64     txf_conflict;           /* Logical address where
                                           conflict was detected     */

        TPAGEMAP  txf_pagesmap[ MAX_TXF_PAGES ]; /* Page addresses   */
        int       txf_pgcnt;            /* Entries in TPAGEMAP table */

        BYTE    txf_gprmask;            /* GPR register restore mask */
        DW      txf_savedgr[16];        /* Saved gpr register values */

        int     txf_tac;                /* Transaction abort code.
                                           Use txf_lock to access!   */

        int     txf_random_tac;         /* Random abort code         */

        /* --------------- TXF debugging --------------------------- */

        int          txf_who;           /* CPU doing delayed abort   */
        const char*  txf_loc;           /* Where the abort occurred  */

        /*-----------------------------------------------------------*/
        /* Transaction Abort PSW fields                              */

        PSW        txf_tapsw;           /* Transaction abort PSW     */
        BYTE*      txf_ip;              /* AIA Mainstor inst address */
        BYTE*      txf_aip;             /* AIA Mainstor page address */
        DW         txf_aiv;             /* AIA Virtual page address  */

        /*-----------------------------------------------------------*/
        /* CONSTRAINED transaction instruction fetching constraint   */

        BYTE*   txf_aie;                /* Maximum trans ip address  */
        U64     txf_aie_aiv;            /* Virtual page address      */
        U64     txf_aie_aiv2;           /* 2nd page if trans crosses */
        int     txf_aie_off2;           /* Offset into 2nd page      */
        /*-----------------------------------------------------------*/

        U32     txf_piid;               /* Transaction Program
                                           Interrupt Identifier      */
        BYTE    txf_dxc_vxc;            /* Data/Vector Exception Code*/

        U32     txf_why;                /* why transaction aborted   */
                                        /* see transact.h for codes  */
        int     txf_lastarn;            /* Last access arn           */

        U16     txf_pifctab[ MAX_TXF_TND ];   /* PIFC control table  */

        TXFTRACE  txf_trace;            /* Saved values for tracing  */

#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */

     /* ------------------------------------------------------------ */
        U64     regs_copy_end;          /* Copy regs to here         */
     /* ------------------------------------------------------------ */

     /* Runtime opcode tables. Use 'replace_opcode' to modify */

        const INSTR_FUNC    *s370_runtime_opcode_xxxx,
                            *s370_runtime_opcode_e3________xx,
                            *s370_runtime_opcode_eb________xx,
                            *s370_runtime_opcode_ec________xx,
                            *s370_runtime_opcode_ed________xx;

        const INSTR_FUNC    *s390_runtime_opcode_xxxx,
                            *s390_runtime_opcode_e3________xx,
                            *s390_runtime_opcode_eb________xx,
                            *s390_runtime_opcode_ec________xx,
                            *s390_runtime_opcode_ed________xx;

        const INSTR_FUNC    *z900_runtime_opcode_xxxx,
                            *z900_runtime_opcode_e3________xx,
                            *z900_runtime_opcode_eb________xx,
                            *z900_runtime_opcode_ec________xx,
                            *z900_runtime_opcode_ed________xx;

#if !defined( OPTION_NO_E3_OPTINST )
        const INSTR_FUNC    *s370_runtime_opcode_e3_0______xx,
                            *s390_runtime_opcode_e3_0______xx,
                            *z900_runtime_opcode_e3_0______xx;
#endif

     /* TLB - Translation lookaside buffer                           */
        unsigned int tlbID;             /* Validation identifier     */
        TLB     tlb;                    /* Translation lookaside buf */

        BLOCK_TRAILER;                  /* Name of block  END        */
};
// end REGS

/*-------------------------------------------------------------------*/
/* Structure definition for the Vector Facility                      */
/*-------------------------------------------------------------------*/
#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
struct VFREGS {                          /* Vector Facility Registers*/
        unsigned int
                online:1;               /* 1=VF is online            */
        U64     vsr;                    /* Vector Status Register    */
        U64     vac;                    /* Vector Activity Count     */
        BYTE    vmr[VECTOR_SECTION_SIZE/8];  /* Vector Mask Register */
        U32     vr[16][VECTOR_SECTION_SIZE]; /* Vector Registers     */
};
#endif /* defined( _FEATURE_S370_S390_VECTOR_FACILITY ) */

// #if defined(FEATURE_REGION_RELOCATE)
/*-------------------------------------------------------------------*/
/* Zone Parameter Block                                              */
/*-------------------------------------------------------------------*/
struct ZPBLK {
        RADR mso;                      /* Main Storage Origin        */
        RADR msl;                      /* Main Storage Length        */
        RADR eso;                      /* Expanded Storage Origin    */
        RADR esl;                      /* Expanded Storage Length    */
        RADR mbo;                      /* Measurement block origin   */
        BYTE mbk;                      /* Measurement block key      */
        int  mbm;                      /* Measurement block mode     */
        int  mbd;                      /* Device connect time mode   */
};
// #endif /*defined(FEATURE_REGION_RELOCATE)*/

/*-------------------------------------------------------------------*/
/* Guest System Information block   EBCDIC DATA                      */
/*-------------------------------------------------------------------*/
struct GSYSINFO {
        BYTE    loadparm[8];
        BYTE    lparname[8];
        BYTE    manufact[16];
        BYTE    plant[4];
        BYTE    model[16];
        BYTE    modelcapa[16];
        BYTE    modelperm[16];
        BYTE    modeltemp[16];
        BYTE    systype[8];
        BYTE    sysname[8];
        BYTE    sysplex[8];
        BYTE    cpid[16];
        BYTE    vmid[8];
};


/*-------------------------------------------------------------------*/
/* Operation Modes                                                   */
/*-------------------------------------------------------------------*/
enum OPERATION_MODE
{
    om_basic = 0,   /*  lparmode = 0                                 */
    om_mif   = 1,   /*  lparmode = 1; cpuidfmt = 0; partitions 1-16  */
    om_emif  = 2    /*  lparmode = 1; cpuidfmt = 1; partitions 0-255 */
};

/* We use the below in sysblk, but it's not defined except on Apple. */
#if !defined( BUILD_APPLE_M1 )
    typedef unsigned qos_class_t;
#endif

/*-------------------------------------------------------------------*/
/* System configuration block                                        */
/*-------------------------------------------------------------------*/
struct SYSBLK {
#define HDL_NAME_SYSBLK   "SYSBLK"
#define HDL_VERS_SYSBLK   "SDL 4.2"     /* Internal Version Number   */
#define HDL_SIZE_SYSBLK   sizeof(SYSBLK)
        BLOCK_HEADER;                   /* Name of block - SYSBLK    */
        char   *hercules_pgmname;       /* Starting program name     */
        char   *hercules_pgmpath;       /* Starting pgm path name    */
        char   *hercules_cmdline;       /* Hercules Command line     */
        char   *netdev;                 /* Network device name       */
#define DEF_NETDEV init_sysblk_netdev() /* Retrieve sysblk.netdev    */

  const char  **vers_info;              /* Version information       */
  const char  **bld_opts;               /* Build options             */
  const char  **extpkg_vers;            /* External Package versions */

        bool    ulimit_unlimited;       /* ulimit -c unlimited       */
        pid_t   hercules_pid;           /* Process Id of Hercules    */
        time_t  impltime;               /* TOD system was IMPL'ed    */
        LOCK    bindlock;               /* Sockdev bind lock         */
        LOCK    config;                 /* (Re)Configuration Lock    */
        int     arch_mode;              /* Architecturual mode       */
                                        /* 0 == S/370   (ARCH_370_IDX)   */
                                        /* 1 == ESA/390 (ARCH_390_IDX)   */
                                        /* 2 == ESAME   (ARCH_900_IDX)   */
        RADR    mainsize;               /* Main storage size (bytes) */
        BYTE   *mainstor;               /* -> Main storage           */
        BYTE   *storkeys;               /* -> Main storage key array */
        u_int   lock_mainstor:1;        /* Request mainstor to lock  */
        u_int   mainstor_locked:1;      /* Main storage locked       */
        U32     xpndsize;               /* Expanded size in 4K pages */
        BYTE   *xpndstor;               /* -> Expanded storage       */
        u_int   lock_xpndstor:1;        /* Request xpndstor to lock  */
        u_int   xpndstor_locked:1;      /* Expanded storage locked   */
        U64     todstart;               /* Time of initialisation    */
        U64     cpuid;                  /* CPU identifier for STIDP  */
        U32     cpuserial;              /* CPU serial number         */
        U16     cpumodel;               /* CPU model number          */
        BYTE    cpuversion;             /* CPU version code          */
        BYTE    cpuidfmt;               /* STIDP format 0|1          */
        TID     impltid;                /* Thread-id for main progr. */
        TID     loggertid;              /* logger_thread Thread-id   */
#if defined( OPTION_WATCHDOG )
        TID     wdtid;                  /* Thread-id for watchdog    */
#if defined( _MSVC_ )
        /* Normal operation: both false. During suspend but before
           resume, suspended = true, resumed = false. After waking
           up from being suspended, suspended = false, resumed = true.
           Watchdog thread will reset resumed to false again once it
           notices resumed = true.
        */
        bool    sys_suspended;          /* System has been suspended */
        bool    sys_resumed;            /* System has been resumed   */
#endif
#endif
        enum OPERATION_MODE operation_mode; /* CPU operation mode    */
        u_int   lparmode:1;             /* LPAR mode active          */
        U16     lparnum;                /* LPAR identification number*/
        U16     ipldev;                 /* IPL device                */
        int     iplcpu;                 /* IPL cpu                   */
        int     ipllcss;                /* IPL lcss                  */
        int     numvec;                 /* Number vector processors  */
        int     maxcpu;                 /* Max number of CPUs        */
        int     cpus;                   /* Number CPUs configured    */
        int     hicpu;                  /* Highest online cpunum + 1 */
        int     topology;               /* Configuration topology... */
#define TOPOLOGY_HORIZ  0               /* ...horizontal polarization*/
#define TOPOLOGY_VERT   1               /* ...vertical polarization  */
        int     topchnge;               /* 1 = Topology Change Report
                                           pending (CPU cfg on/off)  */
        U32     cpmcr;                  /* Dynamic CP model rating   */
        U32     cpmpcr;                 /* ... Permanent             */
        U32     cpmtcr;                 /* ... Temporary             */
        U32     cpncr;                  /* Dynamic CP nominal rating */
        U32     cpnpcr;                 /* ... Permanent             */
        U32     cpntcr;                 /* ... Temporary             */
        U32     cpmcap;                 /* Dynamic CP model capacity */
        U32     cpncap;                 /* Dynamic CP nominal cap.   */
        U32     cpscap;                 /* Dynamic CP secondary cap. */
        U32     cpacap;                 /* Dynamic CP alternate cap. */
        U8      cpccr;                  /* Dynamic CP change reason  */
        U8      cpcai;                  /* Dynamic CP capacity adj.  */
        U8      hhc_111_112;            /* HHC00111/HHC00112 issued  */
        U8      unused1;                /* (pad/align/unused/avail)  */

        COND    cpucond;                /* CPU config/deconfig cond  */
        LOCK    cpulock[ MAX_CPU_ENGS ];/* CPU lock               */

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )

        /* Transactional-Execution Facility locks                    */

        LOCK    txf_lock[ MAX_CPU_ENGS ]; /* CPU transaction lock for
                                             txf_tnd/txf_tac access  */

#define OBTAIN_TXFLOCK( regs )    obtain_lock ( &sysblk.txf_lock[ (regs)->cpuad ])
#define RELEASE_TXFLOCK( regs )   release_lock( &sysblk.txf_lock[ (regs)->cpuad ])

        // PROGRAMMING NOTE: we purposely define the below count
        // as a signed value (rather than unsigned) so that we can
        // detect if, due to a bug, it ever goes negative (which
        // would indicate a serious logic error!). This is checked
        // by the UPDATE_SYSBLK_TRANSCPUS macro, which should be
        // the only way this field is ever updated.

        S32     txf_transcpus;          /* Counts transacting CPUs   */

        /* Transactional-Execution Facility debugging flags          */

        U32     txf_tracing;            /* TXF tracing control;      */
                                        /* see #defines below.       */
        U32     txf_why_mask;           /* (only when TXF_TR_WHY)    */
        int     txf_tac;                /* (only when TXF_TR_TAC)    */
        int     txf_tnd;                /* (only when TXF_TR_TND)    */
        int     txf_fails;              /* (only when TXF_TR_FAILS)  */
        int     txf_cpuad;              /* (only when TXF_TR_CPU)    */

        TID     rubtid;                 /* Threadid for rubato timer */
        LOCK    rublock;                /* Rubato thread lock        */
        U32     txf_counter;            /* counts TBEGIN/TBEGINC     */
        int     txf_timerint;           /* modulation of timerint    */

#define TXF_TR_INSTR    0x80000000      // instructions
#define TXF_TR_C        0x08000000      // constrained
#define TXF_TR_U        0x04000000      // unconstrained
#define TXF_TR_SUCCESS  0x00800000      // success
#define TXF_TR_FAILURE  0x00400000      // failure
#define TXF_TR_WHY      0x00200000      // why mask
#define TXF_TR_TAC      0x00100000      // TAC
#define TXF_TR_TND      0x00080000      // TND
#define TXF_TR_CPU      0x00040000      // specific CPU
#define TXF_TR_FAILS    0x00020000      // aborted count
#define TXF_TR_TDB      0x00000800      // tdb
#define TXF_TR_PAGES    0x00000080      // page information
#define TXF_TR_LINES    0x00000040      // cache lines too

        TXFSTATS  txf_stats[2];         /* Transactional statistics
                                           (slot 0 = unconstrained)  */
#define TXF_STATS( ctr, contran )       \
                                        \
atomic_update64( &sysblk.txf_stats[ contran ? 1 : 0 ].txf_ ## ctr, +1 )

#define TXF_CONSTRAINED( contran ) (contran ? "CONSTRAINED" : "UNconstrained" )

#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */

        TOD     cpucreateTOD[ MAX_CPU_ENGS ];   /* CPU creation time */
        TID     cputid[ MAX_CPU_ENGS ];         /* CPU thread ids    */
        clockid_t                               /* CPU clock         */
                cpuclockid[ MAX_CPU_ENGS ];     /* identifiers       */

        BYTE    ptyp[ MAX_CPU_ENGS ];   /* SCCB ptyp for each engine */
        LOCK    todlock;                /* TOD clock update lock     */
        TID     todtid;                 /* Thread-id for TOD update  */
        REGS   *regs[ MAX_CPU_ENGS + 1];/* Registers for each CPU    */

        /* Active Facility List */
        BYTE    facility_list[ NUM_GEN_ARCHS ][ STFL_HERC_DW_SIZE * sizeof( DW ) ];

     /* CPU Measurement Counter facility
        CPU Measurement Sampling facility
        Load Program Parameter facility */
        U64     program_parameter;      /* Program Parameter Register*/

#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
        HRANDHAND  wkrandhand;          /* secure random api handle  */
        BYTE    wkaes_reg[32];          /* Wrapping-key registers    */
        BYTE    wkdea_reg[24];
        BYTE    wkvpaes_reg[32];        /* Wrapping-key Verification */
        BYTE    wkvpdea_reg[24];        /* Pattern registers         */
        bool    use_def_crypt;          /* true = use srand()/rand() */
#endif /* defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( _FEATURE_047_CMPSC_ENH_FACILITY )
        BYTE    zpbits;                 /* Zeropad alignment bits    */
#define MIN_CMPSC_ZP_BITS   1           /* align to half word bndry  */
#define DEF_CMPSC_ZP_BITS   8           /* align to  256 byte bndry  */
#define MAX_CMPSC_ZP_BITS   12          /* align to 4096 page bndry  */
#define CMPSC_ZP_BITS       ((U8)sysblk.zpbits)
#define CMPSC_ZP_BYTES      ((U16)1 << CMPSC_ZP_BITS)
#define CMPSC_ZP_MASK       (((U64)-1) >> (64 - CMPSC_ZP_BITS))
#endif

#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
        VFREGS  vf[ MAX_CPU_ENGS ];     /* Vector Facility           */
#endif
#if defined(_FEATURE_SIE)
        ZPBLK   zpb[FEATURE_SIE_MAXZONES];  /* SIE Zone Parameter Blk*/
#endif /*defined(_FEATURE_SIE)*/
#if defined(OPTION_FOOTPRINT_BUFFER)
        REGS    footprregs[ MAX_CPU_ENGS ][OPTION_FOOTPRINT_BUFFER];
        U32     footprptr[ MAX_CPU_ENGS ];
#endif
#define LOCK_OWNER_NONE  0xFFFF
#define LOCK_OWNER_OTHER 0xFFFE
        U16     mainowner;              /* Mainlock owner            */
        U16     intowner;               /* Intlock owner             */

        LOCK    mainlock;               /* Main storage lock         */
        LOCK    intlock;                /* Interrupt lock            */
        LOCK    iointqlk;               /* I/O Interrupt Queue lock  */
        LOCK    sigplock;               /* Signal processor lock     */
        ATTR    detattr;                /* Detached thread attribute */
        ATTR    joinattr;               /* Joinable thread attribute */
#define  DETACHED  &sysblk.detattr      /* (helper macro)            */
#define  JOINABLE  &sysblk.joinattr     /* (helper macro)            */
        TID     cnsltid;                /* Thread-id for console     */
        TID     socktid;                /* Thread-id for sockdev     */
                                        /* 3270 Console keepalive:   */
        int     kaidle;                 /* keepalive idle seconds    */
        int     kaintv;                 /* keepalive probe interval  */
        int     kacnt;                  /* keepalive probe count     */
        LOCK    cnslpipe_lock;          /* signaled flag access lock */
        int     cnslpipe_flag;          /* 1 == already signaled     */
        int     cnslwpipe;              /* fd for sending signal     */
        int     cnslrpipe;              /* fd for receiving signal   */
        LOCK    sockpipe_lock;          /* signaled flag access lock */
        int     sockpipe_flag;          /* 1 == already signaled     */
        int     sockwpipe;              /* fd for sending signal     */
        int     sockrpipe;              /* fd for receiving signal   */
        RADR    mbo;                    /* Measurement block origin  */
        BYTE    mbk;                    /* Measurement block key     */
        int     mbm;                    /* Measurement block mode    */
        int     mbd;                    /* Device connect time mode  */
        LOCK    scrlock;                /* Script processing lock    */
        COND    scrcond;                /* Test script condition     */
        int     scrtest;                /* 1 == test mode active     */
        double  scrfactor;              /* Testing timeout factor    */
        TID     cmdtid;                 /* Active command thread     */
        char    cmdsep;                 /* Command Separator char    */
        BYTE    sysgroup;               /* Panel Command grouping    */
#define SYSGROUP_SYSOPER     0x01       /* Computer operator group   */
#define SYSGROUP_SYSMAINT    0x02       /* System Maintainer group   */
#define SYSGROUP_SYSPROG     0x04       /* System Programmer group   */
#define SYSGROUP_SYSNONE     0x08       /* Command valid w/no group  */
#define SYSGROUP_SYSCONFIG   0x10       /* System Configuration group*/
#define SYSGROUP_SYSDEVEL    0x20       /* Hercules Developer group  */
#define SYSGROUP_SYSDEBUG    0x40       /* Internal Debug group      */
#define SYSGROUP_SYSNDIAG    0x80       /* Not supported by DIAG008  */
#define SYSGROUP_SYSALL      0x7F       /* All groups but no DIAG008 */
#define SYSGROUP_PROGDEVELDEBUG ( SYSGROUP_SYSPROG  | \
                                  SYSGROUP_SYSDEVEL | \
                                  SYSGROUP_SYSDEBUG )
#if defined(_DEBUG) || defined(DEBUG)
  #define  DEFAULT_SYSGROUP     SYSGROUP_SYSALL
#else
  #define  DEFAULT_SYSGROUP     (0                    \
                                 | SYSGROUP_SYSOPER   \
                                 | SYSGROUP_SYSMAINT  \
                                 | SYSGROUP_SYSPROG   \
                                 | SYSGROUP_SYSNONE   \
                                 | SYSGROUP_SYSCONFIG \
                                )
#endif
        BYTE    diag8opt;               /* Diagnose 8 option         */
#define DIAG8CMD_ENABLE   0x01          /* Enable DIAG8 interface    */
#define DIAG8CMD_ECHO     0x80          /* Echo command to console   */
        BYTE    shcmdopt;               /* Host shell access option  */
#define SHCMDOPT_ENABLE   0x01          /* Allow host shell access   */
#define SHCMDOPT_DIAG8    0x02          /* Allow for DIAG8 as well   */
        int     panrate;                /* Panel refresh rate        */
        int     pan_colors;             /* Panel colors option:      */

#define PANC_NONE   0                   /* No colors (default)       */
#define PANC_DARK   1                   /* Dark background scheme    */
#define PANC_LIGHT  2                   /* Light/white background    */

        int pan_color[7][2];            /* Panel message colors:     */

#define PANC_X_IDX      0               /*  (default)                */
#define PANC_I_IDX      1               /*  'I'nformational          */
#define PANC_E_IDX      2               /*  'E'rror                  */
#define PANC_W_IDX      3               /*  'W'arning                */
#define PANC_D_IDX      4               /*  'D'ebug                  */
#define PANC_S_IDX      5               /*  'S'evere                 */
#define PANC_A_IDX      6               /*  'A'ction                 */

#define PANC_FG_IDX     0               /*  Foreground               */
#define PANC_BG_IDX     1               /*  Background               */

        int     timerint;               /* microsecs timer interval  */
        int     cfg_timerint;           /* (value defined in config) */
        char   *pantitle;               /* Alt console panel title   */
#if defined( OPTION_SCSI_TAPE )
        /* Access to all SCSI fields controlled by sysblk.stape_lock */
        LOCK    stape_lock;             /* LOCK for all SCSI fields  */
        int     auto_scsi_mount_secs;   /* scsimount frequency: #of  */
                                        /*   seconds; 0 == disabled  */
#define DEF_SCSIMOUNT_SECS  (5)         /* Default scsimount value   */
        TID     stape_getstat_tid;      /* Tape-status worker thread */
        TID     stape_mountmon_tid;     /* Tape-mount  worker thread */
        COND    stape_getstat_cond;     /* Tape-status thread COND   */
        u_int   stape_getstat_busy:1;   /* 1=Status thread is busy   */
        LIST_ENTRY  stape_status_link;  /* get status request chain  */
        LIST_ENTRY  stape_mount_link;   /* scsimount  request chain  */
        struct  timeval
                stape_query_status_tod; /* TOD of last status query  */
#endif // defined( OPTION_SCSI_TAPE )

        /*-----------------------------------------------------------*/
        /*      Devices                                              */
        /*-----------------------------------------------------------*/

        DEVBLK *firstdev;               /* -> First device block     */
        DEVBLK *sysgdev;                /* -> devblk for SYSG console*/
        DEVBLK ***devnum_fl;            /* 1st level table for fast  */
                                        /* devnum lookup             */
        DEVBLK ***subchan_fl;           /* Subchannel table fast     */
                                        /* lookup table              */
        int     highsubchan[FEATURE_LCSS_MAX];  /* Highest subchan+1 */
        CHPBLK *firstchp;               /* -> First channel path     */
        LOCK    dasdcache_lock;         /* Global DASD caching lock  */

        int     num_pfxs;               /* Number  command prefixes  */
        char*   cmd_pfxs;               /* Default command prefixes  */
        char*   used_pfxs;              /* Used    command prefixes  */

        /*-----------------------------------------------------------*/
        /*      I/O Management                                       */
        /*-----------------------------------------------------------*/

        int     mss;                    /* Multiple CSS enabled      */
        int     lcssmax;                /* Maximum Subchannel-Set Id */
        LOCK    crwlock;                /* CRW queue lock            */
        U32    *crwarray;               /* CRW queue                 */
        U32     crwalloc;               /* #of entries allocated     */
        U32     crwcount;               /* #of entries queued        */
        U32     crwindex;               /* CRW queue index           */
        IOINT  *iointq;                 /* I/O interrupt queue       */
        DEVBLK *ioq;                    /* I/O queue                 */
        LOCK    ioqlock;                /* I/O queue lock            */
        COND    ioqcond;                /* I/O queue condition       */
        int     devtwait;               /* Device threads waiting    */
        int     devtnbr;                /* Number of device threads  */
        int     devtmax;                /* Max device threads        */
        int     devthwm;                /* High water mark           */
        int     devtunavail;            /* Count thread unavailable  */
        RADR    addrlimval;             /* Address limit value (SAL) */
#if defined(_FEATURE_VM_BLOCKIO)
        U16     servcode;               /* External interrupt code   */
        BYTE    biosubcd;               /* Block I/O sub int. code   */
        BYTE    biostat;                /* Block I/O status          */
        U64     bioparm;                /* Block I/O interrupt parm  */
        DEVBLK  *biodev;                /* Block I/O device          */
        /* Note: biodev is only used to detect BIO interrupt tracing */
#endif /* defined(FEATURE_VM_BLOCKIO) */
        TAMDIR *tamdir;                 /* Acc/Rej AUTOMOUNT dir ctl */
        char   *defdir;                 /* Default AUTOMOUNT dir     */
        U32     servparm;               /* Service signal parameter  */
        unsigned int                    /* Flags                     */
                sys_reset:1,            /* 1 = system in reset state */
                ipled:1,                /* 1 = guest has been IPL'ed */
                sfcmd:1,                /* 1 = 'sf' command issued   */
                daemon_mode:1,          /* Daemon mode active        */
                panel_init:1,           /* Panel display initialized */
                npquiet:1,              /* New Panel quiet indicator */
#if defined(_FEATURE_SYSTEM_CONSOLE)
                scpecho:1,              /* scp echo mode indicator   */
                scpimply:1,             /* scp imply mode indicator  */
#endif
                sigintreq:1,            /* 1 = SIGINT request pending*/
                insttrace:1,            /* 1 = Inst trace enabled    */
                tfnostop:1,             /* 1 = tf continue tracing   */
                instbreak:1,            /* 1 = Inst break enabled    */
                shutbegin:1,            /* 1 = shutdown begin req    */
                shutdown:1,             /* 1 = shutdown requested    */
                shutfini:1,             /* 1 = shutdown complete     */
                shutimmed:1,            /* 1 = shutdown req immed    */
                main_clear:1,           /* 1 = mainstor is cleared   */
                xpnd_clear:1,           /* 1 = xpndstor is cleared   */
                showregsfirst:1,        /* 1 = show regs before inst */
                showregsnone:1,         /* 1 = show no registers     */
                nomountedtapereinit:1,  /* 1 = disallow tape devinit
                                             if tape already mounted */
                auto_tape_create:1,     /* 1=Automatically do minimal
                                           'hetinit' at tape open to
                                           create the tape file if it
                                           does not yet exist.       */
#define DEF_AUTO_TAPE_CREATE    TRUE    /* Default auto_tape_create  */
                legacysenseid:1,        /* ena/disa senseid on       */
                                        /*   legacy devices          */
                haveiplparm:1,          /* IPL PARM a la VM          */
                logoptnodate:1,         /* 1 = don't datestamp log   */
                logoptnotime:1,         /* 1 = don't timestamp log   */
                nolrasoe:1,             /* 1 = No trace LRA Special  */
                                        /*     Operation Exceptions  */
                noch9oflow:1,           /* Suppress CH9 O'Flow trace */
                devnameonly:1,          /* Display only dev filename */
                config_processed;       /* config file processed     */
        int     quitmout;               /* quit timeout value        */
        U32     ints_state;             /* Common Interrupts Status  */
        CPU_BITMAP config_mask;         /* Configured CPUs           */
        CPU_BITMAP started_mask;        /* Started CPUs              */
        CPU_BITMAP waiting_mask;        /* Waiting CPUs              */
        U16     breakasid;              /* Break ASID                */
        U64     breakaddr[2];           /* Break address range       */
        U64     traceaddr[2];           /* Tracing address range     */
        U64     auto_trace_beg;         /* Automatic t+ instcount    */
        U64     auto_trace_amt;         /* Automatic tracing amount  */
        BYTE    iplparmstring[64];      /* 64 bytes loadable at IPL  */
        char    loadparm[8+1];          /* Default LOADPARM          */
#ifdef _FEATURE_ECPSVM
//
        /* ECPS:VM */
        struct {
            u_int level:16;
            u_int debug:1;
            u_int available:1;
            u_int enabletrap:1;
            u_int freetrap:1;
        } ecpsvm;                       /* ECPS:VM structure         */
//
#endif
        U64     pgminttr;               /* Program int trace mask    */
        U32     ostailor;               /* Current OSTAILOR setting  */
        int     pcpu;                   /* Tgt CPU panel cmd & displ */
        int     hercnice;               /* Herc. process NICE value  */
        int     minprio;                /* pthread minimum priority  */
        int     maxprio;                /* pthread maximum priority  */
        int     hercprio;               /* Hercules THREAD priority  */
        int     todprio;                /* TOD Clock thread priority */
        int     cpuprio;                /* CPU thread priority       */
        int     devprio;                /* Device thread priority    */
        int     srvprio;                /* Listeners thread priority */
        TID     httptid;                /* HTTP listener thread id   */

     /* Classes of service for macOS's scheduler on Apple Silicon.   */
        qos_class_t qos_user_interactive;
        qos_class_t qos_user_initiated;
        qos_class_t qos_default;
        qos_class_t qos_utility;
        qos_class_t qos_background;

     /* Fields used by SYNCHRONIZE_CPUS */
        bool    syncing;                /* 1=Sync in progress        */
        CPU_BITMAP sync_mask;           /* CPU mask for syncing CPUs */
        COND    all_synced_cond;        /* Sync in progress COND     */
        COND    sync_done_cond;         /* Synchronization done COND */

#if defined( OPTION_SHARED_DEVICES )
        LOCK    shrdlock;               /* shrdport LOCK             */
        COND    shrdcond;               /* shrdport COND             */
        TID     shrdtid;                /* Shared device listener    */
        U16     shrdport;               /* Shared device server port */
        U32     shrdcount;              /* IO count                  */
        LOCK    shrdtracelock;          /* Trace table LOCK          */
        SHRD_TRACE  *shrdtrace;         /* Internal trace table      */
        SHRD_TRACE  *shrdtracep;        /* Current pointer           */
        SHRD_TRACE  *shrdtracex;        /* End of trace table        */
        int          shrdtracen;        /* Number of entries         */
        bool         shrddtax;          /* true=dump table at exit   */
#endif
#ifdef OPTION_IODELAY_KLUDGE
        int     iodelay;                /* I/O delay kludge for linux*/
#endif /*OPTION_IODELAY_KLUDGE*/

#if !defined(NO_SETUID)
        uid_t   ruid, euid, suid;
        gid_t   rgid, egid, sgid;
#endif /*!defined(NO_SETUID)*/

#if defined( OPTION_INSTR_COUNT_AND_TIME )

        bool    icount;                 /* true = enabled, else not. */
        struct timeval start_time;      /* OPCODE start time         */

        struct IMAPS {
            U64 imap01[256];
            U64 imapa4[256];
            U64 imapa5[16];
            U64 imapa6[256];
            U64 imapa7[16];
            U64 imapb2[256];
            U64 imapb3[256];
            U64 imapb9[256];
            U64 imapc0[16];
            U64 imapc2[16];
            U64 imapc4[16];
            U64 imapc6[16];
            U64 imapc8[16];
            U64 imape3[256];
            U64 imape4[256];
            U64 imape5[256];
            U64 imape7[256];
            U64 imapeb[256];
            U64 imapec[256];
            U64 imaped[256];
            U64 imapxx[256];

            U64 imap01T[256];
            U64 imapa4T[256];
            U64 imapa5T[16];
            U64 imapa6T[256];
            U64 imapa7T[16];
            U64 imapb2T[256];
            U64 imapb3T[256];
            U64 imapb9T[256];
            U64 imapc0T[16];
            U64 imapc2T[16];
            U64 imapc4T[16];
            U64 imapc6T[16];
            U64 imapc8T[16];
            U64 imape3T[256];
            U64 imape4T[256];
            U64 imape5T[256];
            U64 imape7T[256];
            U64 imapebT[256];
            U64 imapecT[256];
            U64 imapedT[256];
            U64 imapxxT[256];
        } imaps;

#endif // defined( OPTION_INSTR_COUNT_AND_TIME )

        char    *cnslport;              /* console port string       */
        char    *sysgport;              /* SYSG console port string  */
        char    **herclogo;             /* Constructed logo screen   */
        char    *logofile;              /* File name of logo file    */
        size_t  logolines;              /* Logo file number of lines */

        /*-----------------------------------------------------------*/
        /*      Trace File support                                   */
        /*-----------------------------------------------------------*/

        LOCK        tracefileLock;      /* LOCK for below fields     */
#define OBTAIN_TRACEFILE_LOCK()     obtain_lock(  &sysblk.tracefileLock )
#define RELEASE_TRACEFILE_LOCK()    release_lock( &sysblk.tracefileLock )
        const char* tracefilename;      /* File name of trace file   */
        FILE*       traceFILE;          /* Ptr to trace file FILE    */
        U64         curtracesize;       /* Current trace file size   */
        U64         maxtracesize;       /* Maximum trace file size   */
        void*       tracefilebuff;      /* Ptr to trace file buffer  */

        TFGSK*      s370_gsk;           /* s370_get_storage_key      */
        TFGSK*      s390_gsk;           /* s390_get_storage_key      */
        TFGSK*      z900_gsk;           /* z900_get_storage_key      */

        TFVTR*      s370_vtr;           /* virt_to_real              */
        TFVTR*      s390_vtr;           /* virt_to_real              */
        TFVTR*      z900_vtr;           /* virt_to_real              */

        TFSIT*      s370_sit;           /* store_int_timer           */
        TFGCT*      gct;                /* get_cpu_timer             */

        /* Merged Counters for all CPUs                              */
        U64     instcount;              /* Instruction counter       */
        U32     mipsrate;               /* Instructions per second   */
        U32     siosrate;               /* IOs per second            */

        int     regs_copy_len;          /* Length to copy for REGS   */

        REGS    dummyregs;              /* Regs for unconfigured CPU */

        unsigned int msglvl;            /* Message level             */

#define MLVL_VERBOSE 0x80000000         /* Show cfg file messages    */
#define MLVL_DEBUG   0x40000000         /* Prefix w/filename(line#)  */
#define MLVL_EMSGLOC 0x20000000         /* Show location of err msgs */

#if defined(_DEBUG) || defined(DEBUG)
  #define  DEFAULT_MLVL     (MLVL_VERBOSE | MLVL_DEBUG)
#else
  #define  DEFAULT_MLVL     (MLVL_VERBOSE | MLVL_EMSGLOC)
#endif

        BLOCK_TRAILER;                  /* eye-end                   */
};
// end SYSBLK

/*-------------------------------------------------------------------*/
/* I/O interrupt queue entry                                         */
/*-------------------------------------------------------------------*/

struct IOINT {                          /* I/O interrupt queue entry */
        IOINT  *next;                   /* -> next interrupt entry   */
        DEVBLK *dev;                    /* -> Device block           */
        int     priority;               /* Device priority           */
        unsigned int
                pending:1,              /* 1=Normal interrupt        */
                pcipending:1,           /* 1=PCI interrupt           */
                attnpending:1;          /* 1=ATTN interrupt          */
};

/*-------------------------------------------------------------------*/
/* SCSI support threads request structures...   (i.e. work items)    */
/*-------------------------------------------------------------------*/

#if defined(OPTION_SCSI_TAPE)

  struct STSTATRQ                       /* Status Update Request     */
  {
      LIST_ENTRY   link;                /* just a link in the chain  */
      DEVBLK*      dev;                 /* ptr to device block       */
  };
  typedef struct STSTATRQ  STSTATRQ;

  struct STMNTDRQ                       /* Automatic Mount Request   */
  {
      LIST_ENTRY   link;                /* just a link in the chain  */
      DEVBLK*      dev;                 /* ptr to device block       */
  };
  typedef struct STMNTDRQ  STMNTDRQ;

#endif // defined( OPTION_SCSI_TAPE )

/*-------------------------------------------------------------------*/
/* tdparms loader types (ACL mode)                                   */
/*-------------------------------------------------------------------*/
enum type_loader {  LDR_MANUAL, /* No automation             */
                    LDR_AUTO,   /* Automatic Mode            */
                    LDR_SILO,   /* Use LOAD DISPLAY          */
                    LDR_QUEUE   /* Like AUTO                 */
                 };

/*-------------------------------------------------------------------*/
/* Channel Path config block                                         */
/*-------------------------------------------------------------------*/
struct CHPBLK {
#define HDL_NAME_CHPBLK   "CHPBLK"
#define HDL_VERS_CHPBLK   "SDL 4.00"
#define HDL_SIZE_CHPBLK   sizeof(CHPBLK)
        CHPBLK *nextchp;
        BYTE    css;
        BYTE    chpid;
        BYTE    chptype;
};

/*-------------------------------------------------------------------*/
/* Channel prefetch logic IDAW Types                                 */
/*-------------------------------------------------------------------*/
typedef enum
{
   PF_NO_IDAW = 0,
   PF_IDAW1   = 1,      // Format-1 IDAW
   PF_IDAW2   = 2,      // Format-2 IDAW
   PF_MIDAW   = 3       // Modified-IDAW
}
PF_IDATYPE;

/*-------------------------------------------------------------------*/
/* Telnet Control Block                                              */
/*-------------------------------------------------------------------*/
#define TTYPE_LEN TELNET_MAX_TTYPE_LEN  /* (just a shorter name)     */
struct TELNET {

        char    tt1st[TTYPE_LEN+1];     /* Client's first term type  */
        char    ttype[TTYPE_LEN+1];     /* Negotiated terminal type  */
        char    tgroup[16];             /* Terminal group name       */
        char    clientid[32];           /* Client Id string          */
        int     csock;                  /* Client socket             */
        bool    sysg;                   /* SYSG port connection      */

        unsigned int  sendbuf_size;     /* One shot send buffer size */
        char         *sendbuf;          /* One shot send buffer      */

        telnet_t  *ctl;                 /* Ptr to libtelnet control  */
        DEVBLK    *dev;                 /* Device Block ptr or NULL  */
        U16        devnum;              /* Requested device number,  */
                                        /* or FFFF=any device number */
        BYTE       devclass;            /* 'D' = 3270 Display,       */
                                        /* 'K' = Keyboard console,   */
                                        /* 'P' = 3287 Printer        */
        BYTE       model;               /* 3270 model (2,3,4,5,X)    */

                                        /* ----- Boolean flags ----- */
        BYTE    do_tn3270;              /* TN3270 mode enabled       */
        BYTE    do_bin;                 /* Binary mode enabled       */
        BYTE    do_eor;                 /* End-of-record enabled     */
        BYTE    got_eor;                /* End-of-record received    */
        BYTE    got_break;              /* Break type code received  */
        BYTE    extatr;                 /* 3270 extended attributes  */
        BYTE    neg_done;               /* Negotiations complete     */
        BYTE    neg_fail;               /* Negotiations failure      */
        BYTE    send_err;               /* Socket send() failure     */
        BYTE    overflow;               /* Too much data accumulated */
        BYTE    overrun;                /* Unexpected extra data     */
};


/*-------------------------------------------------------------------*/
/* Device configuration block                                        */
/*-------------------------------------------------------------------*/
struct DEVBLK {                         /* Device configuration block*/
#define HDL_NAME_DEVBLK   "DEVBLK"      /* Internal Version Number   */
#define HDL_VERS_DEVBLK   "SDL 4.00"    /* Internal Version Number   */
#define HDL_SIZE_DEVBLK   sizeof(DEVBLK)
        BLOCK_HEADER;                   /* Name of block - DEVBLK    */

        DEVBLK *nextdev;                /* -> next device block      */
        LOCK    lock;                   /* Device block lock         */
        int     allocated;              /* Device block free/in use  */

        /*  device identification                                    */
        U16     ssid;                   /* Subsystem ID incl. lcssid */
        U16     subchan;                /* Subchannel number         */
        U16     devnum;                 /* Device number             */
        U16     devtype;                /* Device type               */
        U16     chanset;                /* Channel Set to which this
                                           device is connected S/370 */
        char   *typname;                /* Device type name          */

        int     member;                 /* Group member number       */
        DEVGRP *group;                  /* Device Group              */

        int     argc;                   /* Init number arguments     */
        char    **argv;                 /* Init arguments            */
        int     numconfdev;             /* The number of devices
                                           specified on config stmt
                                           or attach command         */

        /*  Storage accessible by device                             */

        BYTE   *mainstor;               /* -> Main storage           */
        BYTE   *storkeys;               /* -> Main storage key array */
        RADR    mainlim;                /* Central Storage limit or  */
                                        /* guest storage limit (SIE) */
        BYTE    serial[12];             /* Serial number (ASCII)     */
        char    filename[PATH_MAX+1];   /* filename (poss. prefixed
                                           with "|") or cmd prefix
                                           str if integrated console */

        /*  device i/o fields...                                     */

        int     fd;                     /* File desc / socket number */
        FILE   *fh;                     /* associated File handle    */
        bind_struct* bs;                /* -> bind_struct if socket-
                                           device, NULL otherwise    */

        /*  device buffer management fields                          */

        int     bufcur;                 /* Buffer data identifier    */
        BYTE   *buf;                    /* -> Device data buffer     */
        BYTE   *prev_buf;               /* (saved previous value)    */
        int     bufsize;                /* Device data buffer size   */
        int     buflen;                 /* Device buffer length used */
        int     bufoff;                 /* Offset into data buffer   */
        int     bufres;                 /* buffer residual length    */
        int     bufoffhi;               /* Highest offset allowed    */
        int     bufupdlo;               /* Lowest offset updated     */
        int     bufupdhi;               /* Highest offset updated    */
        U32     bufupd;                 /* 1=Buffer updated          */

        /*  device cache management fields                           */

        int     cache;                  /* Current cache index       */
        int     cachehits;              /* Cache hits                */
        int     cachemisses;            /* Cache misses              */
        int     cachewaits;             /* Cache waits               */

        /*  device compression support                               */

        int     comps;                  /* Acceptable compressions   */
        int     comp;                   /* Compression used          */
        int     compoff;                /* Offset to compressed data */

        /*  device i/o scheduling fields...                          */

        TID     tid;                    /* Thread-id executing CCW   */
        int     priority;               /* I/O q scehduling priority */
        DEVBLK *nextioq;                /* -> next device in I/O q   */
        IOINT   ioint;                  /* Normal i/o interrupt
                                               queue entry           */
        IOINT   pciioint;               /* PCI i/o interrupt
                                               queue entry           */
        IOINT   attnioint;              /* ATTN i/o interrupt
                                               queue entry           */
        int     cpuprio;                /* CPU thread priority       */
        int     devprio;                /* Device thread priority    */

        /*  fields used during ccw execution...                      */
        BYTE    chained;                /* Command chain and data chain
                                           bits from previous CCW    */
        BYTE    prev_chained;           /* Chaining flags from CCW
                                           preceding the data chain  */
        BYTE    code;                   /* Current CCW opcode        */
        BYTE    prevcode;               /* Previous CCW opcode       */
        int     ccwseq;                 /* CCW sequence number       */

        U32     ccwaddr;
        U16     idapmask;
        BYTE    idawfmt;
        BYTE    ccwfmt;
        BYTE    ccwkey;

        /*  device handler function pointers...                      */

        DEVHND *hnd;                    /* -> Device handlers        */

        DEVIM   *immed;                 /* Model Specific IM codes   */
                                        /* (overrides devhnd immed)  */
        int     is_immed;               /* Last command is Immediate */
        struct {                        /* iobuf validation          */
            int length;
            BYTE *data;
        }       iobuf;

        DEVRCD  *rcd;                   /* Read Configuration Data   */
        DEVSNS  *sns;                   /* Format sense bytes        */

        /*  emulated architecture fields...   (MUST be aligned!)     */

        int     reserved1;              /* ---(ensure alignment)---- */
        ORB     orb;                    /* Operation request blk     */
        PMCW    pmcw;                   /* Path management ctl word  */
        SCSW    scsw;                   /* Subchannel status word(XA)*/
        SCSW    pciscsw;                /* PCI subchannel status word*/
        SCSW    attnscsw;               /* ATTNsubchannel status word*/
        ESW     esw;                    /* Extended status word      */
        BYTE    ecw[32];                /* Extended control word     */
        U32     numsense;               /* Number of sense bytes     */
        BYTE    sense[256];             /* Sense bytes 3480+ 64 bytes*/
        U32     numdevid;               /* Number of device id bytes */
        BYTE    devid[256];             /* Device identifier bytes   */
        U32     numdevchar;             /* Number of devchar bytes   */
        BYTE    devchar[64];            /* Device characteristics    */
        BYTE    pgstat;                 /* Path Group Status         */
        BYTE    pgid[11];               /* Path Group ID             */
        BYTE    reserved2[4];           /* (pad/align/unused/avail)  */
        U16     fla[8];                 /* Full Link Address Array   */
        BYTE    chptype[8];             /* Channel Path Type         */
#if defined( OPTION_SHARED_DEVICES )
        COND    shiocond;               /* shared I/O active cond    */
        int     shiowaiters;            /* Num shared I/O waiters    */
        int     shioactive;             /* Sys Id active on shrd dev */
#define DEV_SYS_NONE    0               /* No active sys on shrd dev */
#define DEV_SYS_LOCAL   0xffff          /* Local sys act on shrd dev */
#endif // defined( OPTION_SHARED_DEVICES )
        BYTE    drvpwd[11];             /* Password for drive        */
        BYTE    sensemm[5];             /* Manuf. & model for sense  */

        /*  control flags...                                         */
        unsigned int                    /* Flags                     */
                append:1,               /* 1=append new data to end  */
                s370start:1,            /* 1=S/370 non-BMX behavior  */
                ckdkeytrace:1,          /* 1=Log CKD_KEY_TRACE       */
#if defined( OPTION_SHARED_DEVICES )
                shareable:1,            /* 1=Device is shareable     */
#endif // defined( OPTION_SHARED_DEVICES )
                console:1,              /* 1=Console device          */
                connected:1,            /* 1=Console client connected*/
                                        /* 1=Connected to remote dev */
                readpending:2,          /* 1=Console read pending    */
                connecting:1,           /* 1=Connecting to remote    */
                localhost:1,            /* 1=Remote is local         */
                batch:1,                /* 1=Called by dasdutil      */
                dasdcopy:1,             /* 1=Called by dasdcopy      */
                quiet:1,                /* 1=suppress open messages  */
                oslinux:1,              /* 1=Linux                   */
                orbtrace:1,             /* 1=ORB trace               */
                ccwtrace:1,             /* 1=CCW trace               */
                ccwopstrace:1,          /* 1=trace CCW opcodes       */
                cdwmerge:1,             /* 1=Channel will merge data
                                             chained write CCWs      */
                debug:1,                /* 1=generic debug flag      */
                reinit:1;               /* 1=devinit, not attach     */

        unsigned int                    /* Device state - serialized
                                            by dev->lock             */
                busy:1,                 /* 1=Device is busy          */
                reserved:1,             /* 1=Device is reserved      */
                suspended:1,            /* 1=Channel pgm suspended   */
                pending:1,              /* 1=I/O interrupt pending   */
                pcipending:1,           /* 1=PCI interrupt pending   */
                attnpending:1,          /* 1=ATTN interrupt pending  */
                startpending:1,         /* 1=startio pending         */
                resumesuspended:1,      /* 1=Hresuming suspended dev */
                tschpending:1;          /* 1=TSCH required to clear  */
                                        /*   pending interrupt       */
#define IOPENDING(_dev)         \
        ((_dev)->pending     || \
         (_dev)->pcipending  || \
         (_dev)->attnpending || \
         (_dev)->tschpending)

#define INITIAL_POWERON_370() \
    ( !sysblk.ipled && ARCH_370_IDX == sysblk.arch_mode )

        /*  Execute Channel Pgm Counts */
        U64     excps;                  /* Number of channel pgms Ex */

        /*  Device dependent data (generic)                          */
        void    *dev_data;

        /*  External GUI fields                                      */
        GUISTAT* pGUIStat;              /* EXTERNALGUI Dev Stat Ctl  */

#if defined(_FEATURE_VM_BLOCKIO)
        /* VM DIAGNOSE X'250' Emulation Environment                  */
        struct VMBIOENV *vmd250env;     /* Established environment   */
#endif /* defined(FEATURE_VM_BLOCKIO) */

        /*  Fields for remote devices                                */

        struct in_addr rmtaddr;         /* Remote address            */
        U16     rmtport;                /* Remote port number        */
        U16     rmtnum;                 /* Remote device number      */
        int     rmtid;                  /* Remote Id                 */
        int     rmtver;                 /* Remote version level      */
        int     rmtrel;                 /* Remote release level      */
        DBLWRD  rmthdr;                 /* Remote header             */
        int     rmtcomp;                /* Remote compression parm   */
        int     rmtcomps;               /* Supported compressions    */
        int     rmtpurgen;              /* Remote purge count        */
        FWORD  *rmtpurge;               /* Remote purge list         */

#ifdef OPTION_SHARED_DEVICES
        /*  Fields for device sharing                                */
        TID     shrdtid;                /* Device thread id          */
        int     shrdid;                 /* Id for next client        */
        int     shrdconn;               /* Number connected clients  */
        int     shrdwait;               /* Signal indicator          */
        SHRD   *shrd[SHARED_MAX_SYS];   /* ->SHRD block              */
#endif

        /*  Device dependent fields for console                      */

        struct in_addr ipaddr;          /* Client IP address         */
        in_addr_t  acc_ipaddr;          /* Allowable clients IP addr */
        in_addr_t  acc_ipmask;          /* Allowable clients IP mask */

        TELNET  *tn;                    /* Telnet Control Block Ptr  */

        U32     rlen3270;               /* Length of data in buffer  */
        int     pos3270;                /* Current screen position   */
        int     keybdrem;               /* Number of bytes remaining
                                           in keyboard read buffer   */
        COND    kbcond;                 /* Wait for keyb condition   */
        int     kbwaiters;              /* Number of keyb waiters    */
        u_int   eab3270:1;              /* 1=Extended attributes     */
        u_int   ewa3270:1;              /* 1=Last erase was EWA      */
        u_int   prompt1052:1;           /* 1=Prompt for linemode i/p */
        BYTE    aid3270;                /* Current input AID value   */
        BYTE    mod3270;                /* 3270 model number         */

        /*  Device dependent fields for cardrdr                      */

        char    **more_files;           /* for more that one file in
                                           reader */
        char    **current_file;         /* counts how many additional
                                           reader files are avail    */
        int     cardpos;                /* Offset of next byte to be
                                           read from data buffer     */
        int     cardrem;                /* Number of bytes remaining
                                           in data buffer            */
        u_int   multifile:1;            /* 1=auto-open next i/p file */
        u_int   rdreof:1;               /* 1=Unit exception at EOF   */
        u_int   ebcdic:1;               /* 1=Card deck is EBCDIC     */
        u_int   ascii:1;                /* 1=Convert ASCII to EBCDIC */
        u_int   trunc:1;                /* Truncate overlength record*/
        u_int   autopad:1;              /* 1=Pad incomplete last rec
                                           to 80 bytes if EBCDIC     */

        /*  Device dependent fields for ctcadpt                      */

        DEVBLK *ctcpair;                /* -> Paired device block    */
        int     ctcpos;                 /* next byte offset          */
        int     ctcrem;                 /* bytes remaining in buffer */
        int     ctclastpos;             /* last packet read          */
        int     ctclastrem;             /* last packet read          */
        u_int   ctcxmode:1;             /* 1=Extended, 0=Basic mode  */
        BYTE    ctctype;                /* CTC_xxx device type       */
        BYTE    netdevname[IFNAMSIZ];   /* network device name       */

        /*  Device dependent fields for ctcadpt : Enhanced CTC       */

        U16     ctcePktSeq;             /* CTCE Packet Sequence      */
                                        /*      # in debug msgs      */
        int     ctceSndSml;             /* CTCE Send Small size      */
        BYTE    ctcexState;             /* CTCE State   x-side       */
        BYTE    ctcexCmd;               /* CTCE Command x-side       */
        BYTE    ctceyState;             /* CTCE State   y-side       */
        BYTE    ctceyCmd;               /* CTCE Command y-side       */
        BYTE    ctceyCmdSCB;            /* CTCE Cmd SCB source       */
        BYTE    ctce_UnitStat;          /* CTCE final UnitStat       */
        int     ctcefd;                 /* CTCE RecvThread File      */
                                        /*      Desc / socket #      */
        LOCK    ctceEventLock;          /* CTCE Condition LOCK       */
        COND    ctceEvent;              /* CTCE Recvd Condition      */
        int     ctce_lport;             /* CTCE Local  port #        */
        int     ctce_connect_lport;     /* CTCE Connect lport #      */
        int     ctce_rport;             /* CTCE Remote port #        */
        struct in_addr ctce_ipaddr;     /* CTCE Dest IP addr         */
        U16     ctce_WRT_sCount_rcvd[2];/* CTCE Last WRT sCount      */
        U16     ctce_rccuu;             /* CTCE Remote CTCA dev      */
        int     ctce_trace_cntr;        /* CTCE trace if > 0         */
        int     ctce_attn_delay;        /* CTCE pre-ATTN delay       */
        TID     ctce_listen_tid;        /* CTCE_ListenThread ID      */
        u_int   ctce_contention_loser:1;/* CTCE cmd collision        */
        u_int   ctce_ccw_flags_cc:1;    /* CTCE ccw in progres       */
        u_int   ctce_ficon:1;           /* CTCE type FICON           */
        u_int   ctce_remote_xmode:1;    /* CTCE y-side Ext mode      */
        u_int   ctce_system_reset:1;    /* CTCE initialized          */
        u_int   ctce_buf_next_read:1;   /* CTCE alt. buf use RD      */
        u_int   ctce_buf_next_write:1;  /* CTCE alt. buf use WR      */

        /*  Device dependent fields for printer                      */

        int     printpos;               /* Number of bytes already
                                           placed in print buffer    */
        int     printrem;               /* Number of bytes remaining
                                           in print buffer           */
        pid_t   ptpcpid;                /* print-to-pipe child pid   */
        u_int   crlf:1;                 /* 1=CRLF delimiters, 0=LF   */
        u_int   ispiped:1;              /* 1=Piped device            */
        u_int   stopdev:1;              /* T=stopped; F=started      */
        u_int   fcbcheck:1;             /* 1=signal FCB errors, else
                                           ignore FCB errors.        */
        u_int   diaggate:1;             /* 1=Diagnostic gate command */
        u_int   fold:1;                 /* 1=Fold to upper case      */
        u_int   ffpend:1;               /* Form Feed pending         */
        u_int   sp0after:1;             /* Did write without spacing */
        u_int   skpimmed:1;             /* Processing Skip Immediate */

        int     lpi;                    /* lines per inch 6/8        */
        int     index;                  /* 3211 indexing             */
        int     lpp;                    /* lines per page            */
#define FCBSIZE_3211        180         /* FCB image size for 3211   */
#define FCBSIZE_OTHER       256         /* FCB image size for 3203   */
#define MAX_FCBSIZE         256
        int     fcb    [ MAX_FCBSIZE ]; /* FCB image itself. The first
                                           entry is LPP, the remaining
                                           entries are the line number
                                           for each of the channels. */
        char*   fcbname;                /* FCB image or cctape name  */
        U16     cctape [ MAX_FCBSIZE ]; /* Carriage Control Tape. The
                                           1st entry (index 0) is for
                                           line 1, the 2nd (index 1)
                                           is for line 2, etc.       */
        int     chan9line;              /* Line number of channel 9  */
        int     chan12line;             /* Line number of channel 12 */
        int     currline;               /* curr line number          */
#define UCBSIZE_1403        240         /* UCS Buffer size 1403      */
#define UCBSIZE_3203        304         /* UCS Buffer size 3203      */
#define UCBSIZE_3211        432         /* UCS Buffer size 3211      */
#define MAX_UCBSIZE         432
        BYTE    ucb    [ MAX_UCBSIZE ]; /* UCS Buffer itself         */
#define PLBSIZE_OTHER       132         /* Print Line Buffer size    */
#define PLBSIZE_3211        132         /* Print Line Buffer size    */
#define MAX_PLBSIZE         150         /* Print Line Buffer size    */
        BYTE    plb    [ MAX_PLBSIZE ]; /* Print Line Buffer itself  */

        /*  Device dependent fields for tapedev                      */

        void   *omadesc;                /* -> OMA descriptor array   */
        U16     omafiles;               /* Number of OMA tape files  */
        U16     curfilen;               /* Current file number       */
        U32     blockid;                /* Current device block ID   */
        off_t   nxtblkpos;              /* Offset from start of file
                                           to next block             */
        off_t   prvblkpos;              /* Offset from start of file
                                           to previous block         */
        U32     curblkrem;              /* Number of bytes unread
                                           from current block        */
        U32     curbufoff;              /* Offset into buffer of data
                                           for next data chained CCW */
        U16     tapssdlen;              /* #of bytes of data prepared
                                           for Read Subsystem Data   */
        HETB   *hetb;                   /* HET control block         */

        struct                          /* TAPE device parms         */
        {
          u_int compress:1;             /* 1=Compression enabled     */
          u_int method:3;               /* Compression method        */
          u_int level:4;                /* Compression level         */
          u_int strictsize:1;           /* Strictly enforce MAXSIZE  */
          u_int displayfeat:1;          /* Device has a display      */
                                        /* feature installed         */
          u_int deonirq:1;              /* DE on IRQ on tape motion  */
                                        /* MVS 3.8j workaround       */
          u_int logical_readonly:1;     /* Tape is forced READ ONLY  */
/* These are not obsolete and are being used for new development     */
          u_int auto_create:1;          /* Create Tape if needed     */
          u_int mnt_req_type:2;         /* default to SL labels      */
          u_int SL_tape_mounted:1;      /* Tape is SL labeled        */
          u_int AL_tape_mounted:1;      /* Tape is AL labeled        */
          U16   chksize;                /* Chunk size                */
          off_t maxsize;                /* Maximum allowed TAPE file
                                           size                      */
/* These are not obsolete and are being used for new development     */
          enum  type_loader loader;     /* Loader Operation          */
          u_int ldr_req_remount:1;      /* Initiate Remount request  */
          u_int scr_tape_list:1;        /* Volume contains scrlist   */
          char *pszACLfilename;         /* Pointer to FQ filename for
                                           ACL AUTO mode options     */
          char *psACLvolsers;           /* pointer to ACL volser buf */
          char *psACLvolser;            /* pointer to cur ACL volser */
          u_int uiACLvolsers;           /* length of ACL volser buf  */
          char  pszIL_VOLSER[7];         /* Internal Label of Tape   */

        }       tdparms;                /* TAPE device parms         */

        off_t   eotmargin;              /* Amount of space left before
                                           reporting EOT (in bytes)  */
        u_int   fenced:1;               /* 1=Pos err; volume fenced  */
        u_int   readonly:1;             /* 1=Tape is write-protected */
        u_int   sns_pending:1;          /* Contingency Allegiance    */
                                        /* - means : don't build a   */
                                        /* sense on X'04' : it's     */
                                        /* aleady there              */
                                        /* NOTE : flag cleared by    */
                                        /*        sense command only */
                                        /*        or a device init   */
        u_int   SIC_active:1;           /* 1=SIC active              */
        u_int   forced_logging:1;       /* 1=Forced Error Logging    */
        u_int   eotwarning:1;           /* 1=EOT warning area reached*/
        u_int   noautomount:1;          /* 1=AUTOMOUNT disabled      */
        u_int   supvr_inhibit:1;        /* 1=Supvr-Inhibit mode      */
        u_int   write_immed:1;          /* 1=Write-Immediate mode    */
#if defined( OPTION_SCSI_TAPE )
        struct mtget mtget;             /* Current SCSI tape status  */
#define sstat  mtget.mt_gstat           /* Generic SCSI tape device-
                                           independent status field;
                                           (struct mtget->mt_gstat)  */
        u_int   stape_close_rewinds:1;  /* 1=Rewind at close         */
        u_int   stape_blkid_32:1;       /* 1=block-ids are 32 bits   */
        u_int   stape_no_erg:1;         /* 1=ignore Erase Gap CCWs   */
        u_int   stape_online:1;         /* 1=GMT_ONLINE is mounted   */
        /* Access to SCSI fields controlled via sysblk.stape_lock    */
        COND      stape_sstat_cond;     /* Tape-status updated COND  */
        STSTATRQ  stape_statrq;         /* Status request structure  */
        STMNTDRQ  stape_mntdrq;         /* Mounted request structure */
#endif // defined( OPTION_SCSI_TAPE )
        U32     msgid;                  /* Message Id of async. i/o  */
        BYTE    tapedevt;               /* Hercules tape device type */
        TAPEMEDIA_HANDLER *tmh;         /* Tape Media Handling       */
                                        /* dispatcher                */

        /* ---------- Autoloader feature --------------------------- */
        TAPEAUTOLOADENTRY *als;          /* Autoloader stack         */
        int     alss;                    /* Autoloader stack size    */
        int     alsix;                   /* Current Autoloader index */
        char  **al_argv;                 /* ARGV in autoloader       */
        int     al_argc;                 /* ARGC in autoloader       */
#define AUTOLOADER_MAX      256          /* Maximum #of entries      */
        /* ---------- end Autoloader feature ----------------------- */

        /* 3480/3490/3590 tape vault support */

        TID     tape_mountmon_tid;      /* Thread ID for async mnts  */
        u_int   utapemountreq;          /* Count of tape mounts      */
        void   *ptvfb;                  /* reserve pointer to struct */

        /* 3480/3490/3590 Message display */
        char   *tapemsg;                /* tape volser               */
        char    tapemsg1[9];            /* 1st Host Message          */
        char    tapemsg2[9];            /* 2nd Host Message          */
        char    tapesysmsg[32];         /*     Unit Message     (SYS)*/
        char   *prev_tapemsg;           /* Previously displayed msg  */

        BYTE    tapedisptype;           /* Type of message display   */
        BYTE    tapedispflags;          /* How the msg is displayed  */

#define TAPEDISPTYP_IDLE           0    /* "READY" "NT RDY" etc (SYS)*/
#define TAPEDISPTYP_LOCATING       1    /* Locate in progress   (SYS)*/
#define TAPEDISPTYP_ERASING        2    /* DSE in progress      (SYS)*/
#define TAPEDISPTYP_REWINDING      3    /* Rewind in progress   (SYS)*/
#define TAPEDISPTYP_UNLOADING      4    /* Unload in progress   (SYS)*/
#define TAPEDISPTYP_CLEAN          5    /* Clean recommended    (SYS)*/
#define TAPEDISPTYP_MOUNT          6    /* Display Until Mounted     */
#define TAPEDISPTYP_UNMOUNT        7    /* Display Until Unmounted   */
#define TAPEDISPTYP_UMOUNTMOUNT    8    /* Display #1 Until Unmounted,
                                              then #2 Until Mounted  */
#define TAPEDISPTYP_WAITACT        9    /* Display until motion      */

#define IS_TAPEDISPTYP_SYSMSG( dev ) \
    (0 \
     || TAPEDISPTYP_IDLE      == (dev)->tapedisptype \
     || TAPEDISPTYP_LOCATING  == (dev)->tapedisptype \
     || TAPEDISPTYP_ERASING   == (dev)->tapedisptype \
     || TAPEDISPTYP_REWINDING == (dev)->tapedisptype \
     || TAPEDISPTYP_UNLOADING == (dev)->tapedisptype \
     || TAPEDISPTYP_CLEAN     == (dev)->tapedisptype \
    )

#define TAPEDISPFLG_ALTERNATE   0x80    /* Alternate msgs 1 & 2      */
#define TAPEDISPFLG_BLINKING    0x40    /* Selected msg blinks       */
#define TAPEDISPFLG_MESSAGE2    0x20    /* Display msg 2 instead of 1*/
#define TAPEDISPFLG_AUTOLOADER  0x10    /* Autoloader request        */
#define TAPEDISPFLG_REQAUTOMNT  0x08    /* ReqAutoMount has work     */

       /* Device dependent fields for Comm Line                      */
        COMMADPT *commadpt;             /* Single structure pointer  */

        /*  Device dependent fields for dasd (fba and ckd)           */

        char   *dasdsfn;                /* Shadow file name          */
        char   *dasdsfx;                /* Pointer to suffix char    */

        /*  Device dependent fields for fbadasd                      */

        FBADEV *fbatab;                 /* Device table entry        */
        int     fbanumblk;              /* Number of blocks in device*/
        int     fbablksiz;              /* Physical block size       */
        U64     fbaorigin;              /* Device origin block number*/
        U64     fbarba;                 /* Relative byte offset      */
        U64     fbaend;                 /* Last RBA in file          */
        /* Values from define extent */
        u_int   fbaxtdef:1;             /* 1=Extent defined          */
        BYTE    fbamask;                /* Define extent file mask   */
        U32     fbaxblkn;               /* Offset from start of device
                                           to first block of extent  */
        U32     fbaxfirst;              /* Block number within dataset
                                           of first block of extent  */
        U32     fbaxlast;               /* Block number within dataset
                                           of last block of extent   */
        /* Values from locate */
        BYTE    fbaoper;                /* Locate operation byte     */
        U16     fbalcnum;               /* Block count for locate    */
        U32     fbalcblk;               /* Block number within dataset
                                           of first block for locate */

        /*  Device dependent fields for ckddasd                      */

        int     ckdnumfd;               /* Number of CKD image files */
        int     ckdfd[CKD_MAXFILES];    /* CKD image file descriptors*/
        int     ckdhitrk[CKD_MAXFILES]; /* Highest track number
                                           in each CKD image file    */
        CKDDEV *ckdtab;                 /* Device table entry        */
        CKDCU  *ckdcu;                  /* Control unit entry        */
        U64     ckdtrkoff;              /* Track image file offset   */
        int     ckdcyls;                /* Number of cylinders       */
        int     ckdtrks;                /* Number of tracks          */
        int     ckdheads;               /* #of heads per cylinder    */
        int     ckdtrksz;               /* Track size                */
        int     ckdcurcyl;              /* Current cylinder          */
        int     ckdcurhead;             /* Current head              */
        int     ckdcurrec;              /* Current record id         */
        int     ckdcurkl;               /* Current record key length */
        int     ckdorient;              /* Current orientation       */
        int     ckdcuroper;             /* Curr op: read=6, write=5  */
        BYTE    ckdfcwrk[CKD_RECHDR_SIZE];/* data for 1st read count */
        BYTE    ckdxcode;               /* extended code             */
        U16     ckdcurdl;               /* Current record data length*/
        U16     ckdrem;                 /* #of bytes from current
                                           position to end of field  */
        U16     ckdpos;                 /* Offset into buffer of data
                                           for next data chained CCW */
        U16     ckdxblksz;              /* Define extent block size  */
        U16     ckdxbcyl;               /* Define extent begin cyl   */
        U16     ckdxbhead;              /* Define extent begin head  */
        U16     ckdxecyl;               /* Define extent end cyl     */
        U16     ckdxehead;              /* Define extent end head    */
        BYTE    ckdfmask;               /* Define extent file mask   */
        BYTE    ckdxgattr;              /* Define extent global attr */
        U16     ckdltranlf;             /* Locate record transfer
                                           length factor             */
        U16     ckdlmask;               /* Locate record mask        */
        BYTE    ckdloper;               /* Locate record operation   */
        BYTE    ckdlaux;                /* Locate record aux byte    */
        BYTE    ckdlcount;              /* Locate record count       */
        BYTE    ckdextcd;               /* extended code             */
        void   *cckd_ext;               /* -> CCKD_EXT, else NULL    */
        BYTE    cckd64:1;               /* 1=CCKD64/CFBA64           */
        BYTE    devcache:1;             /* 0 = device cache off
                                           1 = device cache on       */
        u_int   ckd3990:1;              /* 1=Control unit is 3990    */
        u_int   ckdxtdef:1;             /* 1=Define Extent processed */
        u_int   ckdsetfm:1;             /* 1=Set File Mask processed */
        u_int   ckdlocat:1;             /* 1=Locate Record processed */
        u_int   ckdfcoun:1;             /* 1=first read count done   */
        u_int   ckdspcnt:1;             /* 1=Space Count processed   */
        u_int   ckdseek:1;              /* 1=Seek command processed  */
        u_int   ckdskcyl:1;             /* 1=Seek cylinder processed */
        u_int   ckdrecal:1;             /* 1=Recalibrate processed   */
        u_int   ckdrdipl:1;             /* 1=Read IPL processed      */
        u_int   ckdxmark:1;             /* 1=End of track mark found */
        u_int   ckdhaeq:1;              /* 1=Search Home Addr Equal  */
        u_int   ckdideq:1;              /* 1=Search ID Equal         */
        u_int   ckdkyeq:1;              /* 1=Search Key Equal        */
        u_int   ckdwckd:1;              /* 1=Write R0 or Write CKD   */
        u_int   ckdtrkof:1;             /* 1=Track ovfl on this blk  */
        u_int   ckdssi:1;               /* 1=Set Special Intercept   */
        u_int   ckdnolazywr:1;          /* 1=Perform updates now     */
        u_int   ckdrdonly:1;            /* 1=Open read only          */
        u_int   ckdwrha:1;              /* 1=Write Home Address      */
                                        /* Line above ISW20030819-1  */
        u_int   ckdfakewr:1;            /* 1=Fake successful write
                                             for read only file      */
        BYTE    ckdnvs:1;               /* 1=NVS defined             */
        BYTE    ckdraid:1;              /* 1=RAID device             */
        U16     ckdssdlen;              /* #of bytes of data prepared
                                           for Read Subsystem Data   */

        /* Handler private data - all data that is private to a      */
        /* should now go there.                                      */
        void    *hpd;
        /*  Device dependent fields for QDIO devices                 */
        QDIO_DEV qdio;
        BYTE     qtype;                 /* QDIO device type          */

#define QTYPE_READ   1
#define QTYPE_WRITE  2
#define QTYPE_DATA   3

// E7 Prefix CCW support...

        BYTE    ckdformat;              /* Prefix CCW Format byte    */
        BYTE    ckdvalid;               /* Prefix CCW Validity byte  */
        BYTE    ckdauxiliary;           /* Prefix CCW auxiliary byte */

// Format byte
#define PFX_F_DX            0x00        /* Define Extent             */
#define PFX_F_DX_LRE        0x01        /* DX+Locate Record Extended */
#define PFX_F_DX_PSF        0x02        /* DX+Perform Subsys. Func.  */

// Validity byte
#define PFX_V_DX_VALID      0x80        /* Define Extent bytes valid */
#define PFX_V_TS_VALID      0x40        /* DX Time Stamp field valid */

// Auxiliary byte
#define PFX_A_SMR           0x80        /* Suspend Multipath Reconn. */
#define PFX_A_VALID         0x08        /* All DX+LRE parms valid    */

        BYTE    ccwops[256];            /* CCW opcodes to trace      */

        BLOCK_TRAILER;                  /* eye-end                   */
};
// end DEVBLK


/*-------------------------------------------------------------------*/
/* Device Group Structure     (group of related devices eg OSA)      */
/*-------------------------------------------------------------------*/
struct DEVGRP {                         /* Device Group Structure    */
        int     members;                /* #of member devices in grp */
        int     acount;                 /* #allocated members in grp */
        void   *grp_data;               /* Group dep data (generic)  */
        DEVBLK *memdev[FLEXIBLE_ARRAY]; /* Member devices            */
};


/*-------------------------------------------------------------------*/
/* Structure used by EXTERNAL GUI to track device status updates     */
/*-------------------------------------------------------------------*/
struct GUISTAT
{
    char*   pszOldStatStr;
    char*   pszNewStatStr;
#define     GUI_STATSTR_BUFSIZ    256
    char    szStatStrBuff1[GUI_STATSTR_BUFSIZ];
    char    szStatStrBuff2[GUI_STATSTR_BUFSIZ];
};

#endif // _HSTRUCTS_H
