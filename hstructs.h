/* HSTRUCTS.H   (c) Copyright Roger Bowler, 1999-2012                */
/*              (c) Copyright TurboHercules, SAS 2011                */
/*              Hercules Structure Definitions                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

//      This header auto-#included by 'hercules.h'...
//
//      The <config.h> header and other required headers are
//      presumed to have already been #included ahead of it...

#ifndef _HSTRUCTS_H
#define _HSTRUCTS_H

#include "hercules.h"
#include "opcode.h"
#include "telnet/include/telnet.h"      // Need telnet_t

/*-------------------------------------------------------------------*/
/* Typedefs for CPU bitmap fields                                    */
/*                                                                   */
/* A CPU bitmap contains one bit for each processing engine.         */
/* The width of the bitmap depends on the maximum number of          */
/* processing engines which was selected at build time.              */
/*-------------------------------------------------------------------*/

#if MAX_CPU_ENGINES <= 32
    typedef U32                 CPU_BITMAP;
    #define F_CPU_BITMAP        "%8.8"PRIX32
#elif MAX_CPU_ENGINES <= 64
    typedef U64                 CPU_BITMAP;
    #define F_CPU_BITMAP        "%16.16"PRIX64
#elif MAX_CPU_ENGINES <= 128
 #if SIZEOF_SIZE_T == 4
   #error MAX_CPU_ENGINES > 64 only supported on 64 bit platforms
 #endif

 #if defined(_MSVC_)
    WARNING( "MAX_CPU_ENGINES in Windows is 64" )
    typedef U64                 CPU_BITMAP;
    #define F_CPU_BITMAP        "%16.16"PRIX64
    #undef  MAX_CPU_ENGINES
    #define MAX_CPU_ENGINES     64
 #else
    typedef __uint128_t         CPU_BITMAP;
 // ZZ FIXME: No printf format support for __uint128_t yet, so we will incorrectly display...
    #define F_CPU_BITMAP        "%16.16"PRIX64
 #endif

#else
 #error MAX_CPU_ENGINES cannot exceed 128
#endif


/*-------------------------------------------------------------------*/
/* Structure definition for CPU register context                     */
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
#define HDL_VERS_REGS   "4.00"          /* Internal Version Number   */
#define HDL_SIZE_REGS   sizeof(REGS)
/*000*/ BLOCK_HEADER;                   /* Name of block   REGS_CP00 */
/*030*/ U64     cpuid;                  /* Formatted CPU ID          */
/*038*/ U32     cpuserial;              /* CPU serial number         */
/*03C*/ U16     cpumodel;               /* CPU model number          */
/*03E*/ U8      cpuversion;             /* CPU version code          */
/*03F*/ U8      _cpu_reserved;          /* Reserved for future use   */
                                        /* --- 64-byte cache line -- */
/*040*/ SYSBLK *sysblk;                 /* Pointer to sysblk         */
        ALIGN_8
/*048*/ U32     ints_state;             /* CPU Interrupts Status     */
/*04C*/ U32     ints_mask;              /* Respective Interrupts Mask*/
/*050*/ CPU_BITMAP cpubit;              /* Only this CPU's bit is 1  */
        ALIGN_16
/*060*/ BYTE    cpustate;               /* CPU stopped/started state */
/*061*/ BYTE    _cpu_reserved2;         /* Available...              */
/*062*/ U16     extccpu;                /* CPU causing external call */
/*064*/ int     arch_mode;              /* Architectural mode        */
/*068*/ BYTE   *ip;                     /* Mainstor inst address     */
/*070*/ DW      px;                     /* Prefix register           */
                                        /* --- 64-byte cache line -- */
/*080*/ PSW     psw;                    /* Program status word       */
/*0E8-0FF*/                             /* Available...              */

        ALIGN_128                       /* --- 64-byte cache line -- */
/*100*/ BYTE    malfcpu                 /* Malfuction alert flags    */
                    [MAX_CPU_ENGINES];  /* for each CPU (1=pending)  */
        ALIGN_128
/*180*/ BYTE    emercpu                 /* Emergency signal flags    */
                    [MAX_CPU_ENGINES];  /* for each CPU (1=pending)  */

     /* AIA - Instruction fetch accelerator                          */
/*200*/ ALIGN_128
        BYTE   *aip;                    /* Mainstor page address     */
/*208*/ ALIGN_8
        uintptr_t aim;                  /* Mainstor xor address      */
/*210*/ ALIGN_8
        BYTE   *aie;                    /* Mainstor page end address */
/*218*/ DW      aiv;                    /* Virtual page address      */

/*220*/ U64     bear;                   /* Breaking event address reg*/
/*228*/ BYTE   *bear_ip;                /* Breaking event inst ptr   */

/*230-27F*/                             /* Available...              */


/*280*/ ALIGN_128
        DW      gr[16];                 /* General registers         */
/*300*/ U32     ar[16];                 /* Access registers          */
/*340*/ U32     fpr[32];                /* Floating point registers  */
/*380*/ U32     fpc;                    /* IEEE Floating Point
                                                    Control Register */
/*384*/ BYTE    __reserved_space[52];   /* Available...              */

#define CR_ASD_REAL     -1
#define CR_ALB_OFFSET   16

#ifndef NOCHECK_AEA_ARRAY_BOUNDS
/*3B8*/ DW      cr_struct[1+16+16];
#define XR(_crn) cr_struct[1+(_crn)]
#else
#define XR(_crn) cr[(_crn)]
/*3B8*/ DW      cr_special[1];          /* Negative Index into cr    */
/*3C0*/ DW      cr[16];                 /* Control registers         */
/*440*/ DW      alb[16];                /* Accesslist Lookaside cr   */
#endif

/*4C0*/ U32     dxc;                    /* Data exception code       */
/*4C4*/                                 /* Available...              */
/*4C8*/ DW      mc;                     /* Monitor Code              */
/*4D0*/ DW      ea;                     /* Exception address         */
/*4D8*/ DW      et;                     /* Execute Target address    */

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
                tracing:1,              /* 1=Trace is active         */
                stepwait:1,             /* 1=Wait in inst stepping   */
                sigpreset:1,            /* 1=SIGP cpu reset received */
                sigpireset:1;           /* 1=SIGP initial cpu reset  */

        CACHE_ALIGN                     /* --- 64-byte cache line -- */
        S64     tod_epoch;              /* TOD epoch for this CPU    */
        TOD     clkc;                   /* 0-7=Clock comparator epoch,
                                           8-63=Comparator bits 0-55 */
        TOD     cpu_timer;              /* CPU timer                 */
        TOD     cpu_timer_epoch;        /* CPU timer epoch           */
        U32     cpu_timer_mode;         /* CPU timer source mode     */
        U32     todpr;                  /* TOD programmable register */

        S64     int_timer;              /* S/370 Interval timer      */
        S64     ecps_vtimer;            /* ECPS Virtual Int. timer   */
        S32     old_timer;              /* S/370 Interval timer int  */
        S32     ecps_oldtmr;            /* ECPS Virtual Int. tmr int */

                                        /* --- 64-byte cache line -- */
        BYTE   *ecps_vtmrpt;            /* Pointer to VTMR or zero   */

        U64     rcputime;               /* Real CPU time used (us)   */
        U64     bcputime;               /* Base (reset) CPU time (us)*/
        U64     prevcount;              /* Previous instruction count*/
        U32     instcount;              /* Instruction counter       */
        U32     mipsrate;               /* Instructions per second   */
        U32     siocount;               /* SIO/SSCH counter          */
        U32     siosrate;               /* IOs per second            */
        U64     siototal;               /* Total SIO/SSCH count      */
                                        /* --- 64-byte cache line -- */
        int     cpupct;                 /* Percent CPU busy          */
        U64     waittod;                /* Time of day last wait (us)*/
        U64     waittime;               /* Wait time (us) in interval*/

        CACHE_ALIGN                     /* --- 64-byte cache line -- */
        DAT     dat;                    /* Fields for DAT use        */

#define GR_G(_r) gr[(_r)].D
#define GR_H(_r) gr[(_r)].F.H.F          /* Fullword bits 0-31       */
#define GR_HHH(_r) gr[(_r)].F.H.H.H.H    /* Halfword bits 0-15       */
#define GR_HHL(_r) gr[(_r)].F.H.H.L.H    /* Halfword low, bits 16-31 */
#define GR_HHLCL(_r) gr[(_r)].F.H.H.L.B.L   /* Character, bits 24-31 */
#define GR_L(_r) gr[(_r)].F.L.F          /* Fullword low, bits 32-63 */
#define GR_LHH(_r) gr[(_r)].F.L.H.H.H    /* Halfword bits 32-47      */
#define GR_LHL(_r) gr[(_r)].F.L.H.L.H    /* Halfword low, bits 48-63 */
#define GR_LHHCH(_r) gr[(_r)].F.L.H.H.B.H   /* Character, bits 32-39 */
#define GR_LA24(_r) gr[(_r)].F.L.A.A     /* 24 bit addr, bits 40-63  */
#define GR_LA8(_r) gr[(_r)].F.L.A.B      /* 24 bit addr, unused bits */
#define GR_LHLCL(_r) gr[(_r)].F.L.H.L.B.L   /* Character, bits 56-63 */
#define GR_LHLCH(_r) gr[(_r)].F.L.H.L.B.H   /* Character, bits 48-55 */
#define CR_G(_r)   XR((_r)).D            /* Bits 0-63                */
#define CR_H(_r)   XR((_r)).F.H.F        /* Fullword bits 0-31       */
#define CR_HHH(_r) XR((_r)).F.H.H.H.H    /* Halfword bits 0-15       */
#define CR_HHL(_r) XR((_r)).F.H.H.L.H    /* Halfword low, bits 16-31 */
#define CR_L(_r)   XR((_r)).F.L.F        /* Fullword low, bits 32-63 */
#define CR_LHH(_r) XR((_r)).F.L.H.H.H    /* Halfword bits 32-47      */
#define CR_LHHCH(_r) XR((_r)).F.L.H.H.B.H   /* Character, bits 32-39 */
#define CR_LHL(_r) XR((_r)).F.L.H.L.H    /* Halfword low, bits 48-63 */
#define MC_G      mc.D
#define MC_L      mc.F.L.F
#define EA_G      ea.D
#define EA_L      ea.F.L.F
#define ET_G      et.D
#define ET_L      et.F.L.F
#define PX_G      px.D
#define PX_L      px.F.L.F
#define AIV_G     aiv.D
#define AIV_L     aiv.F.L.F
#define AR(_r)    ar[(_r)]

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
            PSA_3XX *psa;         /* -> PSA for this CPU 370 and ESA */
            PSA_900 *zpsa;     /* -> PSA for this CPU when in z arch */
        };

     /*
      * The fields hostregs and guestregs have been move outside the
      * scope of _FEATURE_SIE to reduce conditional code.
      *
      *   sysblk.regs[i] always points to the host regs
      *   flag `host' is always 1 for the host regs
      *   flag `guest' is always 1 for the guest regs
      *   `hostregs' is always equal to sysblk.regs[i] (in both
      *       hostregs and guestregs)
      *   `guestregs' is always equal to sysblk.regs[i]->guestregs
      *       (in both hostregs and guestregs).
      *       sysblk.regs[i]->guestregs is NULL until the first SIE
      *       instruction is executed on that CPU.
      *   `sie_active' is 1 in hostregs if SIE is executing
      *       and the current register context is `guestregs'
      *   `sie_mode' is 1 in guestregs always
      *   `sie_state' has the real address of the SIEBK
      *   `siebk' has the mainstor address of the SIEBK
      */
        REGS   *hostregs;               /* Pointer to the hypervisor
                                           register context          */
        REGS   *guestregs;              /* Pointer to the guest
                                           register context          */

#if defined(_FEATURE_SIE)
        CACHE_ALIGN                     /* --- 64-byte cache line -- */
        RADR    sie_state;              /* Address of the SIE state
                                           descriptor block or 0 when
                                           not running under SIE     */
        SIEBK  *siebk;                  /* Sie State Desc structure  */
        RADR    sie_px;                 /* Host address of guest px  */
        RADR    sie_mso;                /* Main Storage Origin       */
        RADR    sie_xso;                /* eXpanded Storage Origin   */
        RADR    sie_xsl;                /* eXpanded Storage Limit    */
        RADR    sie_rcpo;               /* Ref and Change Preserv.   */
        RADR    sie_scao;               /* System Contol Area        */
        S64     sie_epoch;              /* TOD offset in state desc. */
#endif /*defined(_FEATURE_SIE)*/
        unsigned int
                sie_active:1,           /* SIE active (host only)    */
                sie_mode:1,             /* Running under SIE (guest) */
                sie_pref:1;             /* Preferred-storage mode    */

// #if defined(FEATURE_PER)
        ALIGN_16
        U16     perc;                   /* PER code                  */
        RADR    peradr;                 /* PER address               */
        BYTE    peraid;                 /* PER access id             */
// #endif /*defined(FEATURE_PER)*/

     /*
      * Making the following flags 'stand-alone' (instead of bit-
      * flags like they were) addresses a compiler bit-flag serial-
      * ization issue that occurs with the 'SYNCHRONIZE_CPUS' macro
      * used during synchronize broadcast (cpu<->cpu communication)
      */
        ALIGN_8
        int     intwait;                /* 1=Waiting on intlock      */
        BYTE    inst[8];                /* Fetched instruction when
                                           instruction crosses a page
                                           boundary                  */
        BYTE    *invalidate_main;       /* Mainstor addr to invalidat*/
        CACHE_ALIGN                     /* --- 64-byte cache line -- */
        PSW     captured_zpsw;          /* Captured-z/Arch PSW reg   */
#if defined(_FEATURE_VECTOR_FACILITY)
        CACHE_ALIGN
        VFREGS *vf;                     /* Vector Facility           */
#endif /*defined(_FEATURE_VECTOR_FACILITY)*/

        CACHE_ALIGN
        jmp_buf progjmp;                /* longjmp destination for
                                           program check return      */
        jmp_buf archjmp;                /* longjmp destination to
                                           switch architecture mode  */
        jmp_buf exitjmp;                /* longjmp destination for
                                           CPU thread exit           */
        COND    intcond;                /* CPU interrupt condition   */
        LOCK    *cpulock;               /* CPU lock for this CPU     */

     /* Mainstor address lookup accelerator                          */

        BYTE    aea_mode;               /* aea addressing mode       */

#ifndef NOCHECK_AEA_ARRAY_BOUNDS
        int     aea_ar_struct[5+16];
#define AEA_AR(_arn) aea_ar_struct[5+(_arn)]
#else
#define AEA_AR(_arn) aea_ar[(_arn)]
        int     aea_ar_special[5];      /* Negative index into ar    */
        int     aea_ar[16];             /* arn to cr number          */
#endif

#ifndef NOCHECK_AEA_ARRAY_BOUNDS
        BYTE    aea_common_struct[1+16+16];
#define AEA_COMMON(_asd) aea_common_struct[1+(_asd)]
#else
#define AEA_COMMON(_asd) aea_common[(_asd)]
        BYTE    aea_common_special[1];  /* real asd                  */
        BYTE    aea_common[16];         /* 1=asd is not private      */
        BYTE    aea_common_alb[16];     /* alb pseudo registers      */
#endif

        BYTE    aea_aleprot[16];        /* ale protected             */

     /* Function pointers */
        pi_func program_interrupt;
        func    trace_br;

     /* Active Facility List */
        BYTE    facility_list[STFL_HBYTESIZE];

     /* ------------------------------------------------------------ */
        U64     regs_copy_end;          /* Copy regs to here         */
     /* ------------------------------------------------------------ */

     /* Runtime opcode tables, use replace_opcode to modify */
        const zz_func *s370_runtime_opcode_xxxx,
               *s370_runtime_opcode_e3________xx,
               *s370_runtime_opcode_eb________xx,
               *s370_runtime_opcode_ec________xx,
               *s370_runtime_opcode_ed________xx;
        const zz_func *s390_runtime_opcode_xxxx,
               *s390_runtime_opcode_e3________xx,
               *s390_runtime_opcode_eb________xx,
               *s390_runtime_opcode_ec________xx,
               *s390_runtime_opcode_ed________xx;
        const zz_func *z900_runtime_opcode_xxxx,
               *z900_runtime_opcode_e3________xx,
               *z900_runtime_opcode_eb________xx,
               *z900_runtime_opcode_ec________xx,
               *z900_runtime_opcode_ed________xx;
#ifdef OPTION_OPTINST
        const zz_func
               *s370_runtime_opcode_e3_0______xx,
               *s390_runtime_opcode_e3_0______xx,
               *z900_runtime_opcode_e3_0______xx;
#endif /* #ifdef OPTION_OPTINST */

     /* TLB - Translation lookaside buffer                           */
        unsigned int tlbID;             /* Validation identifier     */
        TLB     tlb;                    /* Translation lookaside buf */

        BLOCK_TRAILER;                  /* Name of block  END        */
};

/*-------------------------------------------------------------------*/
/* Structure definition for the Vector Facility                      */
/*-------------------------------------------------------------------*/
#if defined(_FEATURE_VECTOR_FACILITY)
struct VFREGS {                          /* Vector Facility Registers*/
        unsigned int
                online:1;               /* 1=VF is online            */
        U64     vsr;                    /* Vector Status Register    */
        U64     vac;                    /* Vector Activity Count     */
        BYTE    vmr[VECTOR_SECTION_SIZE/8];  /* Vector Mask Register */
        U32     vr[16][VECTOR_SECTION_SIZE]; /* Vector Registers     */
};
#endif /*defined(_FEATURE_VECTOR_FACILITY)*/

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


/*-------------------------------------------------------------------*/
/* System configuration block                                        */
/*-------------------------------------------------------------------*/
struct SYSBLK {
#define HDL_NAME_SYSBLK   "SYSBLK"
#define HDL_VERS_SYSBLK   "4.00"        /* Internal Version Number   */
#define HDL_SIZE_SYSBLK   sizeof(SYSBLK)
        BLOCK_HEADER;                   /* Name of block - SYSBLK    */
        char   *hercules_pgmname;       /* Starting program name     */
        char   *hercules_pgmpath;       /* Starting pgm path name    */
        char   *hercules_cmdline;       /* Hercules Command line     */
        pid_t   hercules_pid;           /* Process Id of Hercules    */
        time_t  impltime;               /* TOD system was IMPL'ed    */
        LOCK    config;                 /* (Re)Configuration Lock    */
        int     arch_mode;              /* Architecturual mode       */
                                        /* 0 == S/370   (ARCH_370)   */
                                        /* 1 == ESA/390 (ARCH_390)   */
                                        /* 2 == ESAME   (ARCH_900)   */
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
        TID     wdtid;                  /* Thread-id for watchdog    */
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
        COND    cpucond;                /* CPU config/deconfig cond  */
        LOCK    cpulock[MAX_CPU_ENGINES];  /* CPU lock               */
        TOD     cpucreateTOD[MAX_CPU_ENGINES];  /* CPU creation time */
        TID     cputid[MAX_CPU_ENGINES];   /* CPU thread identifiers */
        BYTE    ptyp[MAX_CPU_ENGINES];  /* SCCB ptyp for each engine */
        LOCK    todlock;                /* TOD clock update lock     */
        TID     todtid;                 /* Thread-id for TOD update  */
        REGS   *regs[MAX_CPU_ENGINES+1];   /* Registers for each CPU */
        LOCK    caplock[MAX_CPU_ENGINES]; /* CP capping locks        */
        int     caplocked[MAX_CPU_ENGINES]; /* Indication locked     */
        TID     captid;                 /* TID capping manager       */
        U32     capvalue;               /* Capping value in mips     */

        /* Active Facility List */
        BYTE    facility_list[GEN_MAXARCH][STFL_HBYTESIZE];

     /* CPU Measurement Counter facility
        CPU Measurement Sampling facility
        Load Program Parameter facility */
        U64     program_parameter;      /* Program Parameter Register*/

#ifdef _FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3
        RWLOCK  wklock;                 /* Update lock               */
        BYTE    wkaes_reg[32];          /* Wrapping-key registers    */
        BYTE    wkdea_reg[24];
        BYTE    wkvpaes_reg[32];        /* Wrapping-key Verification */
        BYTE    wkvpdea_reg[24];        /* Pattern registers         */
#endif /* FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3 */
#if defined(_FEATURE_CMPSC_ENHANCEMENT_FACILITY)
        BYTE    zpbits;                 /* Zeropad alignment bits    */
#define MIN_CMPSC_ZP_BITS   1           /* align to half word bndry  */
#define DEF_CMPSC_ZP_BITS   8           /* align to  256 byte bndry  */
#define MAX_CMPSC_ZP_BITS   12          /* align to 4096 page bndry  */
#define CMPSC_ZP_BITS       ((U8)sysblk.zpbits)
#define CMPSC_ZP_BYTES      ((U16)1 << CMPSC_ZP_BITS)
#define CMPSC_ZP_MASK       (((U64)-1) >> (64 - CMPSC_ZP_BITS))
#endif /* defined(_FEATURE_CMPSC_ENHANCEMENT_FACILITY) */

#if defined(_FEATURE_VECTOR_FACILITY)
        VFREGS  vf[MAX_CPU_ENGINES];    /* Vector Facility           */
#endif /*defined(_FEATURE_VECTOR_FACILITY)*/
#if defined(_FEATURE_SIE)
        ZPBLK   zpb[FEATURE_SIE_MAXZONES];  /* SIE Zone Parameter Blk*/
#endif /*defined(_FEATURE_SIE)*/
#if defined(OPTION_FOOTPRINT_BUFFER)
        REGS    footprregs[MAX_CPU_ENGINES][OPTION_FOOTPRINT_BUFFER];
        U32     footprptr[MAX_CPU_ENGINES];
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
        int     sockwpipe;              /* Sockdev signaling pipe Wr */
        int     sockrpipe;              /* Sockdev signaling pipe Rd */
        RADR    mbo;                    /* Measurement block origin  */
        BYTE    mbk;                    /* Measurement block key     */
        int     mbm;                    /* Measurement block mode    */
        int     mbd;                    /* Device connect time mode  */
        LOCK    scrlock;                /* Script processing lock    */
        COND    scrcond;                /* Test script condition     */
        int     scrtest;                /* 1 == test mode active     */
        double  scrfactor;              /* Testing timeout factor    */
        TID     cmdtid;                 /* Active command thread     */
        char   *cmdsep;                 /* Single Char cmd Sep       */
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
        int     timerint;               /* microsecs timer interval  */
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
        /*      Control Units                                        */
        /*-----------------------------------------------------------*/

        U16     cuhigh;                 /* Highest used CU number    */
       CHAINBLK cuchain;                /* -> CU chain               */


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
                daemon_mode:1,          /* Daemon mode active        */
                panel_init:1,           /* Panel display initialized */
                npquiet:1,              /* New Panel quiet indicator */
#if defined(_FEATURE_SYSTEM_CONSOLE)
                scpecho:1,              /* scp echo mode indicator   */
                scpimply:1,             /* scp imply mode indicator  */
#endif
                sigintreq:1,            /* 1 = SIGINT request pending*/
                insttrace:1,            /* 1 = Instruction trace     */
                inststep:1,             /* 1 = Instruction step      */
                shutdown:1,             /* 1 = shutdown requested    */
                shutfini:1,             /* 1 = shutdown complete     */
                shutimmed:1,            /* 1 = shutdown req immed    */
                main_clear:1,           /* 1 = mainstor is cleared   */
                xpnd_clear:1,           /* 1 = xpndstor is cleared   */
                showregsfirst:1,        /* 1 = show regs before inst */
                showregsnone:1,         /* 1 = show no registers     */
                nomountedtapereinit:1,  /* 1 = disallow tape devinit
                                             if tape already mounted */
                noautoinit:1,           /* 1=AUTOINIT disabled       */
                legacysenseid:1,        /* ena/disa senseid on       */
                                        /*   legacy devices          */
                haveiplparm:1,          /* IPL PARM a la VM          */
                logoptnotime:1,         /* 1 = don't timestamp log   */
                nolrasoe:1,             /* 1 = No trace LRA Special  */
                                        /*     Operation Exceptions  */
                noch9oflow:1;           /* Suppress CH9 O'Flow trace */
        U32     ints_state;             /* Common Interrupts Status  */
        CPU_BITMAP config_mask;         /* Configured CPUs           */
        CPU_BITMAP started_mask;        /* Started CPUs              */
        CPU_BITMAP waiting_mask;        /* Waiting CPUs              */
        U64     traceaddr[2];           /* Tracing address range     */
        U64     stepaddr[2];            /* Stepping address range    */
        BYTE    iplparmstring[64];      /* 64 bytes loadable at IPL  */
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
        int     pcpu;                   /* Tgt CPU panel cmd & displ */
        int     hercprio;               /* Hercules process priority */
        int     todprio;                /* TOD Clock thread priority */
        int     cpuprio;                /* CPU thread priority       */
        int     devprio;                /* Device thread priority    */
        int     srvprio;                /* Listeners thread priority */
        TID     httptid;                /* HTTP listener thread id   */

     /* Fields used by SYNCHRONIZE_CPUS */
        int     syncing;                /* 1=Sync in progress        */
        CPU_BITMAP sync_mask;           /* CPU mask for syncing CPUs */
        COND    sync_cond;              /* COND for syncing CPU      */
        COND    sync_bc_cond;           /* COND for other CPUs       */
#if defined(OPTION_SHARED_DEVICES)
        TID     shrdtid;                /* Shared device listener    */
        U16     shrdport;               /* Shared device server port */
        U32     shrdcount;              /* IO count                  */
        SHRD_TRACE  *shrdtrace;         /* Internal trace table      */
        SHRD_TRACE  *shrdtracep;        /* Current pointer           */
        SHRD_TRACE  *shrdtracex;        /* End of trace table        */
        int          shrdtracen;        /* Number of entries         */
#endif
#ifdef OPTION_IODELAY_KLUDGE
        int     iodelay;                /* I/O delay kludge for linux*/
#endif /*OPTION_IODELAY_KLUDGE*/

#if !defined(NO_SETUID)
        uid_t   ruid, euid, suid;
        gid_t   rgid, egid, sgid;
#endif /*!defined(NO_SETUID)*/

#if defined(OPTION_COUNTING)
        S64     count[OPTION_COUNTING];
#define COUNT(n) sysblk.count[(n)]++
#else
#define COUNT(n)
#endif

#if defined(OPTION_INSTRUCTION_COUNTING)
#define IMAP_FIRST sysblk.imap01
        U64 imap01[256];
        U64 imapa4[256];
        U64 imapa5[16];
        U64 imapa6[256];
        U64 imapa7[16];
        U64 imapb2[256];
        U64 imapb3[256];
        U64 imapb9[256];
        U64 imapc0[16];
        U64 imapc2[16];                                         /*@Z9*/
        U64 imapc4[16];                                         /*208*/
        U64 imapc6[16];                                         /*208*/
        U64 imapc8[16];
        U64 imape3[256];
        U64 imape4[256];
        U64 imape5[256];
        U64 imapeb[256];
        U64 imapec[256];
        U64 imaped[256];
        U64 imapxx[256];
#define IMAP_SIZE \
            ( sizeof(sysblk.imap01) \
            + sizeof(sysblk.imapa4) \
            + sizeof(sysblk.imapa5) \
            + sizeof(sysblk.imapa6) \
            + sizeof(sysblk.imapa7) \
            + sizeof(sysblk.imapb2) \
            + sizeof(sysblk.imapb3) \
            + sizeof(sysblk.imapb9) \
            + sizeof(sysblk.imapc0) \
            + sizeof(sysblk.imapc2) /*@Z9*/ \
            + sizeof(sysblk.imapc4) /*208*/ \
            + sizeof(sysblk.imapc6) /*208*/ \
            + sizeof(sysblk.imapc8) \
            + sizeof(sysblk.imape3) \
            + sizeof(sysblk.imape4) \
            + sizeof(sysblk.imape5) \
            + sizeof(sysblk.imapeb) \
            + sizeof(sysblk.imapec) \
            + sizeof(sysblk.imaped) \
            + sizeof(sysblk.imapxx) )
#endif

        char    *cnslport;              /* console port string       */
        char    **herclogo;             /* Constructed logo screen   */
        char    *logofile;              /* File name of logo file    */
        size_t  logolines;              /* Logo file number of lines */
#if defined(OPTION_MIPS_COUNTING)
        /* Merged Counters for all CPUs                              */
        U64     instcount;              /* Instruction counter       */
        U32     mipsrate;               /* Instructions per second   */
        U32     siosrate;               /* IOs per second            */
#endif /*defined(OPTION_MIPS_COUNTING)*/

        int     regs_copy_len;          /* Length to copy for REGS   */

        REGS    dummyregs;              /* Regs for unconfigured CPU */

        unsigned int msglvl;            /* Message level             */

#define MLVL_VERBOSE 0x80000000         /* Show cfg file messages    */
#define MLVL_DEBUG   0x40000000         /* Prefix w/filename(line#)  */
#define MLVL_EMSGLOC 0x20000000         /* Show location of err msgs */

#if defined(_DEBUG) || defined(DEBUG)
  #define  DEFAULT_MLVL     (MLVL_VERBOSE | MLVL_DEBUG)
#else
  #define  DEFAULT_MLVL     (MLVL_VERBOSE)
#endif

#if defined( OPTION_SHUTDOWN_CONFIRMATION )
#define QUITTIME_PERIOD     10
        int     quitmout;               /* Current timeout value     */
        time_t  shutquittime;           /* Quit requested time       */
        time_t  SSD_time;               /* SSD requested time        */
#endif // defined( OPTION_SHUTDOWN_CONFIRMATION )

        BLOCK_TRAILER;                  /* eye-end                   */
};

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
/* Control Unit configuration block                                  */
/*-------------------------------------------------------------------*/

typedef struct _CUBLK {                 /* CU configuration block    */
        CHAIN   chain;                  /* Block chain               */
        U16     cunum;                  /* CU number                 */
        void   *cudesc;                 /* CU descriptor             */
        U8      maxssid;                /* Maximum number of SSIDs   */
        U8      ssidcount;              /* Number of associated SSID */
       CHAINBLK sschain;                /* Associated SSIDs          */
        U16     maxdev;                 /* Maximum associated dev    */
        U16     devcount;               /* Number of associated dev  */
} CUBLK;


/*-------------------------------------------------------------------*/
/* tdparms loader types (ACL mode)                                   */
/*-------------------------------------------------------------------*/
enum type_loader {  LDR_MANUAL, /* No automation             */
                    LDR_AUTO,   /* Automatic Mode            */
                    LDR_SILO,   /* Use LOAD DISPLAY          */
                    LDR_QUEUE   /* Like AUTO                 */
                 };

/*-------------------------------------------------------------------*/
/* Subsystem ID configuration block                                  */
/*-------------------------------------------------------------------*/

typedef struct _SSBLK {                 /* Subsystem ID block        */
        CHAIN   chain;                  /* Block chain               */
        CUBLK  *cu;                     /* Owning CU                 */
        U16     ssid;                   /* Subsystem ID (if present) */
        LOCK    lock;                   /* Update lock               */
        void   *cudesc;                 /* CU descriptor             */
        U16     maxdev;                 /* Maximum associated dev    */
        U8      devcount;               /* Number of associated dev  */
        DEVBLK *dev[256];               /* Associated devblocks      */
        void   *stats;                  /* Associated statistics     */
} SSBLK;


/*-------------------------------------------------------------------*/
/* Channel Path config block                                         */
/*-------------------------------------------------------------------*/
struct CHPBLK {
#define HDL_NAME_CHPBLK   "DEVBLK"
#define HDL_VERS_CHPBLK   "3.08"
#define HDL_SIZE_CHPBLK   sizeof(CHPBLK)
        CHPBLK *nextchp;
        BYTE    css;
        BYTE    chpid;
        BYTE    chptype;
};


/*-------------------------------------------------------------------*/
/* Telnet Control Block                                              */
/*-------------------------------------------------------------------*/
#define TTYPE_LEN TELNET_MAX_TTYPE_LEN  /* (just a shorter name)     */
struct TELNET {

        char    ttype[TTYPE_LEN+1];     /* Client terminal type      */
        char    tgroup[16];             /* Terminal group name       */
        char    clientid[32];           /* Client Id string          */
        int     csock;                  /* Client socket             */

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
#define HDL_VERS_DEVBLK   "4.00"        /* Internal Version Number   */
#define HDL_SIZE_DEVBLK   sizeof(DEVBLK)
        BLOCK_HEADER;                   /* Name of block - DEVBLK    */

        DEVBLK *nextdev;                /* -> next device block      */
        LOCK    lock;                   /* Device block lock         */
        int     allocated;              /* Device block free/in use  */

        /*  device identification                                    */
        CUBLK  *cu;                     /* -> Control Unit block     */
        SSBLK  *ssidblk;                /* -> Subsystem ID block     */
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
        ORB     orb;                    /* Operation request blk @IWZ*/
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
#ifdef OPTION_CKD_KEY_TRACING
                ckdkeytrace:1,          /* 1=Log CKD_KEY_TRACE       */
#endif /*OPTION_CKD_KEY_TRACING*/
#if defined( OPTION_SHARED_DEVICES )
                shareable:1,            /* 1=Device is shareable     */
#endif // defined( OPTION_SHARED_DEVICES )
                console:1,              /* 1=Console device          */
                connected:1,            /* 1=Console client connected*/
                readpending:2,          /* 1=Console read pending    */
                connecting:1,           /* 1=Connecting to remote    */
                localhost:1,            /* 1=Remote is local         */
                batch:1,                /* 1=Called by dasdutil      */
                dasdcopy:1,             /* 1=Called by dasdcopy      */
                quiet:1,                /* 1=suppress open messages  */
                oslinux:1,              /* 1=Linux                   */
                orbtrace:1,             /* 1=ORB trace               */
                ccwtrace:1,             /* 1=CCW trace               */
                ccwstep:1,              /* 1=CCW single step         */
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
    ( !sysblk.ipled && ARCH_370 == sysblk.arch_mode )

        /*  Execute Channel Pgm Counts */
        U64     excps;                  /* Number of channel pgms Ex */

        /*  Device dependent data (generic)                          */
        void    *dev_data;

#ifdef EXTERNALGUI
        /*  External GUI fields                                      */
        GUISTAT* pGUIStat;              /* EXTERNALGUI Dev Stat Ctl  */
#endif

#if defined(_FEATURE_VM_BLOCKIO)
        /* VM DIAGNOSE X'250' Emulation Environment                  */
        struct VMBIOENV *vmd250env;     /* Established environment   */
#endif /* defined(FEATURE_VM_BLOCKIO) */

        /*  Fields for remote devices                                */

        struct in_addr rmtaddr;         /* Remote address            */
        U16     rmtport;                /* Remote port number        */
        U16     rmtnum;                 /* Remote device number      */
        int     rmtid;                  /* Remote Id                 */
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

        /*  Device dependent fields for ctcadpt : Enhanced CTC  @PJJ */

        U16     ctcePktSeq;             /* CTCE Packet Sequence @PJJ */
                                        /*      # in debug msgs @PJJ */
        int     ctceSndSml;             /* CTCE Send Small size @PJJ */
        BYTE    ctcexState;             /* CTCE State   x-side  @PJJ */
        BYTE    ctcexCmd;               /* CTCE Command x-side  @PJJ */
        BYTE    ctceyState;             /* CTCE State   y-side  @PJJ */
        BYTE    ctceyCmd;               /* CTCE Command y-side  @PJJ */
        BYTE    ctceyCmdSCB;            /* CTCE Cmd SCB source  @PJJ */
        BYTE    ctce_UnitStat;          /* CTCE final UnitStat  @PJJ */
        int     ctcefd;                 /* CTCE RecvThread File @PJJ */
                                        /*      Desc / socket # @PJJ */
        LOCK    ctceEventLock;          /* CTCE Condition LOCK  @PJJ */
        COND    ctceEvent;              /* CTCE Recvd Condition @PJJ */
        int     ctce_lport;             /* CTCE Local  port #   @PJJ */
        int     ctce_rport;             /* CTCE Remote port #   @PJJ */
        struct in_addr ctce_ipaddr;     /* CTCE Dest IP addr    @PJJ */
        u_int   ctce_contention_loser:1;/* CTCE cmd collision   @PJJ */
        u_int   ctce_ccw_flags_cc:1;    /* CTCE ccw in progres  @PJJ */

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
        int     fcb    [ MAX_FCBSIZE ]; /* FCB image itself          */
        char*   fcbname;                /* FCB image or cctape name  */
        U16     cctape [ MAX_FCBSIZE ]; /* Carriage Control Tape     */
        int     chan9line;              /* Line number of channel 9  */
        int     chan12line;             /* Line number of channel 12 */
        int     currline;               /* curr line number          */
#define UCBSIZE_OTHER       304         /* UCS Buffer size 1403/3203 */
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
        off_t   fbaorigin;              /* Device origin block number*/
        off_t   fbarba;                 /* Relative byte offset      */
        off_t   fbaend;                 /* Last RBA in file          */
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
        off_t   ckdtrkoff;              /* Track image file offset   */
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
        BYTE    ckdreserved1;           /* Alignment                 */
        void   *cckd_ext;               /* -> Compressed ckddasd
                                           extension otherwise NULL  */
        BYTE    devcache:1;             /* 0 = device cache off
                                           1 = device cache on       */
        u_int   ckd3990:1;              /* 1=Control unit is 3990    */
        u_int   ckdxtdef:1;             /* 1=Define Extent processed */
        u_int   ckdsetfm:1;             /* 1=Set File Mask processed */
        u_int   ckdlocat:1;             /* 1=Locate Record processed */
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

        /*  Device dependent fields for QDIO devices                 */
        QDIO_DEV qdio;
        BYTE     qtype;                 /* QDIO device type          */

#define QTYPE_READ   1
#define QTYPE_WRITE  2
#define QTYPE_DATA   3

        BLOCK_TRAILER;                  /* eye-end                   */
};


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
/* Structure definitions for CKD headers                             */
/*-------------------------------------------------------------------*/
struct CKDDASD_DEVHDR {                 /* Device header             */
        BYTE    devid[8];               /* Device identifier         */
        FWORD   heads;                  /* #of heads per cylinder
                                           (bytes in reverse order)  */
        FWORD   trksize;                /* Track size (reverse order)*/
        BYTE    devtype;                /* Last 2 digits of device type
                                           (0x80=3380, 0x90=3390)    */
        BYTE    fileseq;                /* CKD image file sequence no.
                                           (0x00=only file, 0x01=first
                                           file of multiple files)   */
        HWORD   highcyl;                /* Highest cylinder number on
                                           this file, or zero if this
                                           is the last or only file
                                           (bytes in reverse order)  */
        BYTE    resv[492];              /* Reserved                  */
};

struct CKDDASD_TRKHDR {                 /* Track header              */
        BYTE    bin;                    /* Bin number                */
        HWORD   cyl;                    /* Cylinder number           */
        HWORD   head;                   /* Head number               */
};

struct CKDDASD_RECHDR {                 /* Record header             */
        HWORD   cyl;                    /* Cylinder number           */
        HWORD   head;                   /* Head number               */
        BYTE    rec;                    /* Record number             */
        BYTE    klen;                   /* Key length                */
        HWORD   dlen;                   /* Data length               */
};

#define CKDDASD_DEVHDR_SIZE     ((ssize_t)sizeof(CKDDASD_DEVHDR))
#define CKDDASD_TRKHDR_SIZE     ((ssize_t)sizeof(CKDDASD_TRKHDR))
#define CKDDASD_RECHDR_SIZE     ((ssize_t)sizeof(CKDDASD_RECHDR))

/* Null track formats */
#define CKDDASD_NULLTRK_FMT0       0    /* ha r0 r1 eot              */
#define CKDDASD_NULLTRK_FMT1       1    /* ha r0 eot                 */
#define CKDDASD_NULLTRK_FMT2       2    /* linux (3390 only)         */
#define CKDDASD_NULLTRK_FMTMAX     CKDDASD_NULLTRK_FMT2

#define CKDDASD_NULLTRK_SIZE0      (5 + 8 + 8 + 8 + 8)
#define CKDDASD_NULLTRK_SIZE1      (5 + 8 + 8 + 8)
#define CKDDASD_NULLTRK_SIZE2      (5 + 8 + 8 + (12 * (8 + 4096)) + 8)

/*-------------------------------------------------------------------*/
/* Structure definitions for Compressed CKD devices                  */
/*-------------------------------------------------------------------*/
struct CCKDDASD_DEVHDR {                /* Compress device header    */
/*  0 */BYTE             vrm[3];        /* Version Release Modifier  */
/*  3 */BYTE             options;       /* Options byte              */
/*  4 */S32              numl1tab;      /* Size of lvl 1 table       */
/*  8 */S32              numl2tab;      /* Size of lvl 2 tables      */
/* 12 */U32              size;          /* File size                 */
/* 16 */U32              used;          /* File used                 */
/* 20 */U32              free;          /* Position to free space    */
/* 24 */U32              free_total;    /* Total free space          */
/* 28 */U32              free_largest;  /* Largest free space        */
/* 32 */S32              free_number;   /* Number free spaces        */
/* 36 */U32              free_imbed;    /* Imbedded free space       */
/* 40 */FWORD            cyls;          /* Cylinders on device       */
/* 44 */BYTE             nullfmt;       /* Null track format         */
/* 45 */BYTE             compress;      /* Compression algorithm     */
/* 46 */S16              compress_parm; /* Compression parameter     */
/* 48 */BYTE             resv2[464];    /* Reserved                  */
};
#define CCKD_DEVHDR      CCKDDASD_DEVHDR

#define CCKD_VERSION           0
#define CCKD_RELEASE           3
#define CCKD_MODLVL            1

#define CCKD_BIGENDIAN         2
#define CCKD_SPERRS            32        /* Space errors detected    */
#define CCKD_ORDWR             64        /* Opened read/write since
                                            last chkdsk              */
#define CCKD_OPENED            128

#define CCKD_COMPRESS_NONE     0x00
#define CCKD_COMPRESS_ZLIB     0x01
#define CCKD_COMPRESS_BZIP2    0x02
#define CCKD_COMPRESS_MASK     0x03

#define CCKD_STRESS_MINLEN     4096
#if defined(HAVE_LIBZ)
#define CCKD_STRESS_COMP       CCKD_COMPRESS_ZLIB
#else
#define CCKD_STRESS_COMP       CCKD_COMPRESS_NONE
#endif
#define CCKD_STRESS_PARM1      4
#define CCKD_STRESS_PARM2      2

struct CCKD_L2ENT {                     /* Level 2 table entry       */
        U32              pos;           /* Track offset              */
        U16              len;           /* Track length              */
        U16              size;          /* Track size  (size >= len) */
};

struct CCKD_FREEBLK {                   /* Free block (file)         */
        U32              pos;           /* Position next free blk    */
        U32              len;           /* Length this free blk      */
};

struct CCKD_IFREEBLK {                  /* Free block (internal)     */
        U32              pos;           /* Position next free blk    */
        U32              len;           /* Length this free blk      */
        int              prev;          /* Index to prev free blk    */
        int              next;          /* Index to next free blk    */
        int              pending;       /* 1=Free pending (don't use)*/
};

struct CCKD_RA {                        /* Readahead queue entry     */
        DEVBLK          *dev;           /* Readahead device          */
        int              trk;           /* Readahead track           */
        int              prev;          /* Index to prev entry       */
        int              next;          /* Index to next entry       */
};

typedef  U32          CCKD_L1ENT;       /* Level 1 table entry       */
typedef  CCKD_L1ENT   CCKD_L1TAB[];     /* Level 1 table             */
typedef  CCKD_L2ENT   CCKD_L2TAB[256];  /* Level 2 table             */
typedef  char         CCKD_TRACE[128];  /* Trace table entry         */

#define CCKDDASD_DEVHDR_SIZE   ((ssize_t)sizeof(CCKDDASD_DEVHDR))
#define CCKD_DEVHDR_SIZE       CCKDDASD_DEVHDR_SIZE
#define CCKD_DEVHDR_POS        CKDDASD_DEVHDR_SIZE
#define CCKD_L1ENT_SIZE        ((ssize_t)sizeof(CCKD_L1ENT))
#define CCKD_L1TAB_POS         ((CCKD_DEVHDR_POS)+(CCKD_DEVHDR_SIZE))
#define CCKD_L2ENT_SIZE        ((ssize_t)sizeof(CCKD_L2ENT))
#define CCKD_L2TAB_SIZE        ((ssize_t)sizeof(CCKD_L2TAB))
#define CCKD_FREEBLK_SIZE      ((ssize_t)sizeof(CCKD_FREEBLK))
#define CCKD_FREEBLK_ISIZE     ((ssize_t)sizeof(CCKD_IFREEBLK))
#define CCKD_IFREEBLK_SIZE     (CCKD_FREEBLK_ISIZE)

/* Flag bits */
#define CCKD_SIZE_EXACT         0x01    /* Space obtained is exact   */
#define CCKD_SIZE_ANY           0x02    /* Space can be any size     */
#define CCKD_L2SPACE            0x04    /* Space for a l2 table      */

/* adjustable values */

#define CCKD_FREE_MIN_SIZE     96       /* Minimum free space size   */
#define CCKD_FREE_MIN_INCR     32       /* Added for each 1024 spaces*/
#define CCKD_COMPRESS_MIN      512      /* Track images smaller than
                                           this won't be compressed  */
#define CCKD_MAX_SF            8        /* Maximum number of shadow
                                           files: 0 to 9 [0 disables
                                           shadow file support]      */
#define CCKD_MAX_READAHEADS    16       /* Max readahead trks        */
#define CCKD_MAX_RA_SIZE       16       /* Readahead queue size      */
#define CCKD_MAX_RA            9        /* Max readahead threads     */
#define CCKD_MAX_WRITER        9        /* Max writer threads        */
#define CCKD_MAX_GCOL          1        /* Max garbage collectors    */
#define CCKD_MAX_TRACE         200000   /* Max nbr trace entries     */
#define CCKD_MAX_FREEPEND      4        /* Max free pending cycles   */

#define CCKD_MIN_READAHEADS    0        /* Min readahead trks        */
#define CCKD_MIN_RA            0        /* Min readahead threads     */
#define CCKD_MIN_WRITER        1        /* Min writer threads        */
#define CCKD_MIN_GCOL          0        /* Min garbage collectors    */

#define CCKD_DEFAULT_RA_SIZE   4        /* Readahead queue size      */
#define CCKD_DEFAULT_RA        2        /* Default number readaheads */
#define CCKD_DEFAULT_WRITER    2        /* Default number writers    */
#define CCKD_DEFAULT_GCOL      1        /* Default number garbage
                                              collectors             */
#define CCKD_DEFAULT_GCOLWAIT  10       /* Default wait (seconds)    */
#define CCKD_DEFAULT_GCOLPARM  0        /* Default adjustment parm   */
#define CCKD_DEFAULT_READAHEADS 2       /* Default nbr to read ahead */
#define CCKD_DEFAULT_FREEPEND  -1       /* Default freepend cycles   */

#define CFBA_BLOCK_NUM         120      /* Number fba blocks / group */
#define CFBA_BLOCK_SIZE        61440    /* Size of a block group 60k */
                                        /* Number of bytes in an fba
                                           block group.  Probably
                                           should be a multiple of 512
                                           but has to be < 64K       */

struct CCKDBLK {                        /* Global cckd dasd block    */
        BYTE             id[8];         /* "CCKDBLK "                */
        DEVBLK          *dev1st;        /* 1st device in cckd queue  */
        unsigned int     batch:1,       /* 1=called in batch mode    */
                         sfmerge:1,     /* 1=sf-* merge              */
                         sfforce:1;     /* 1=sf-* force              */
        int              sflevel;       /* sfk xxxx level            */
        int              batchml;       /* message level for batch ops  */

        BYTE             comps;         /* Supported compressions    */
        BYTE             comp;          /* Override compression      */
        int              compparm;      /* Override compression parm */

        LOCK             gclock;        /* Garbage collector lock    */
        COND             gccond;        /* Garbage collector cond    */
        int              gcs;           /* Number garbage collector threads started */
        int              gca;           /* Number garbage collector threads active */
        int              gcmax;         /* Max garbage collectors    */
        int              gcwait;        /* Wait time in seconds      */
        int              gcparm;        /* Adjustment parm           */

        LOCK             wrlock;        /* I/O lock                  */
        COND             wrcond;        /* I/O condition             */
        int              wrpending;     /* Number writes pending     */
        int              wrwaiting;     /* Number writers waiting    */
        int              wrs;           /* Number writer threads started  */
        int              wra;           /* Number writer threads active  */
        int              wrmax;         /* Max writer threads        */
        int              wrprio;        /* Writer thread priority    */

        LOCK             ralock;        /* Readahead lock            */
        COND             racond;        /* Readahead condition       */
        int              ras;           /* Number readahead threads started */
        int              raa;           /* Number readadead threads active */
        int              ramax;         /* Max readahead threads     */
        int              rawaiting;     /* Number threads waiting    */
        int              ranbr;         /* Readahead queue size      */
        int              readaheads;    /* Nbr tracks to read ahead  */
        CCKD_RA          ra[CCKD_MAX_RA_SIZE];    /* Readahead queue */
        int              ra1st;         /* First readahead entry     */
        int              ralast;        /* Last readahead entry      */
        int              rafree;        /* Free readahead entry      */

        LOCK             devlock;       /* Device chain lock         */
        COND             devcond;       /* Device chain condition    */
        int              devusers;      /* Number shared users       */
        int              devwaiters;    /* Number of waiters         */

        int              freepend;      /* Number freepend cycles    */
        int              nosfd;         /* 1=No stats rpt at close   */
        int              nostress;      /* 1=No stress writes        */
        int              linuxnull;     /* 1=Always check nulltrk    */
        int              fsync;         /* 1=Perform fsync()         */
        COND             termcond;      /* Termination condition     */

        U64              stats_switches;       /* Switches           */
        U64              stats_cachehits;      /* Cache hits         */
        U64              stats_cachemisses;    /* Cache misses       */
        U64              stats_readaheads;     /* Readaheads         */
        U64              stats_readaheadmisses;/* Readahead misses   */
        U64              stats_iowaits;        /* Waits for i/o      */
        U64              stats_cachewaits;     /* Waits for cache    */
        U64              stats_stresswrites;   /* Writes under stress*/
        U64              stats_l2cachehits;    /* L2 cache hits      */
        U64              stats_l2cachemisses;  /* L2 cache misses    */
        U64              stats_l2reads;        /* L2 reads           */
        U64              stats_reads;          /* Number reads       */
        U64              stats_readbytes;      /* Bytes read         */
        U64              stats_writes;         /* Number writes      */
        U64              stats_writebytes;     /* Bytes written      */
        U64              stats_gcolmoves;      /* Spaces moved       */
        U64              stats_gcolbytes;      /* Bytes moved        */

        CCKD_TRACE      *itrace;        /* Internal trace table      */
        CCKD_TRACE      *itracep;       /* Current pointer           */
        CCKD_TRACE      *itracex;       /* End of trace table        */
        int              itracen;       /* Number of entries         */

        int              bytemsgs;      /* Limit for `byte 0' msgs   */
};

struct CCKDDASD_EXT {                   /* Ext for compressed ckd    */
        DEVBLK          *devnext;       /* cckd device queue         */
        unsigned int     ckddasd:1,     /* 1=CKD dasd                */
                         fbadasd:1,     /* 1=FBA dasd                */
                         cckdioact:1,   /* 1=Channel program active  */
                         bufused:1,     /* 1=newbuf was used         */
                         updated:1,     /* 1=Update occurred         */
                         merging:1,     /* 1=File merge in progress  */
                         stopping:1,    /* 1=Device is closing       */
                         notnull:1,     /* 1=Device has track images */
                         l2ok:1,        /* 1=All l2s below bounds    */
                         sfmerge:1,     /* 1=sf-xxxx merge           */
                         sfforce:1;     /* 1=sf-xxxx force           */
        int              sflevel;       /* sfk xxxx level            */
        LOCK             filelock;      /* File lock                 */
        LOCK             cckdiolock;    /* I/O lock                  */
        COND             cckdiocond;    /* I/O condition             */
        S64              maxsize;       /* Maximum file size         */
        int              cckdwaiters;   /* Number I/O waiters        */
        int              wrpending;     /* Number writes pending     */
        int              ras;           /* Number readaheads active  */
        int              sfn;           /* Number active shadow files*/
        int              sfx;           /* Active level 2 file index */
        int              l1x;           /* Active level 2 table index*/
        CCKD_L2ENT      *l2;            /* Active level 2 table      */
        int              l2active;      /* Active level 2 cache entry*/
        off_t            l2bounds;      /* L2 tables boundary        */
        int              active;        /* Active cache entry        */
        BYTE            *newbuf;        /* Uncompressed buffer       */
        unsigned int     freemin;       /* Minimum free space size   */
        CCKD_IFREEBLK   *free;          /* Internal free space chain */
        int              freenbr;       /* Number free space entries */
        int              free1st;       /* Index of 1st entry        */
        int              freelast;      /* Index of last entry       */
        int              freeavail;     /* Index of available entry  */
        int              lastsync;      /* Time of last sync         */
        int              ralkup[CCKD_MAX_RA_SIZE];/* Lookup table    */
        int              ratrk;         /* Track to readahead        */
        unsigned int     totreads;      /* Total nbr trk reads       */
        unsigned int     totwrites;     /* Total nbr trk writes      */
        unsigned int     totl2reads;    /* Total nbr l2 reads        */
        unsigned int     cachehits;     /* Cache hits                */
        unsigned int     readaheads;    /* Number trks read ahead    */
        unsigned int     switches;      /* Number trk switches       */
        unsigned int     misses;        /* Number readahead misses   */
        int              fd[CCKD_MAX_SF+1];      /* File descriptors */
        BYTE             swapend[CCKD_MAX_SF+1]; /* Swap endian flag */
        BYTE             open[CCKD_MAX_SF+1];    /* Open flag        */
        int              reads[CCKD_MAX_SF+1];   /* Nbr track reads  */
        int              l2reads[CCKD_MAX_SF+1]; /* Nbr l2 reads     */
        int              writes[CCKD_MAX_SF+1];  /* Nbr track writes */
        CCKD_L1ENT      *l1[CCKD_MAX_SF+1];      /* Level 1 tables   */
        CCKDDASD_DEVHDR  cdevhdr[CCKD_MAX_SF+1]; /* cckd device hdr  */
};

#define CCKD_OPEN_NONE         0
#define CCKD_OPEN_RO           1
#define CCKD_OPEN_RD           2
#define CCKD_OPEN_RW           3

#ifdef EXTERNALGUI
struct GUISTAT
{
    char*   pszOldStatStr;
    char*   pszNewStatStr;
#define     GUI_STATSTR_BUFSIZ    256
    char    szStatStrBuff1[GUI_STATSTR_BUFSIZ];
    char    szStatStrBuff2[GUI_STATSTR_BUFSIZ];
};
#endif // EXTERNALGUI


#endif // _HSTRUCTS_H
