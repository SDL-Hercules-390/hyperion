/* HCONSTS.H  (c) Copyright "Fish" (David B. Trout), 2005-2012       */
/*           Hercules #define constants...                           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

//      This header auto-#included by 'hercules.h'...
//
//      The <config.h> header and other required headers are
//      presumed to have already been #included ahead of it...


#ifndef _HCONSTS_H
#define _HCONSTS_H

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Maximum CPU Engines                                               */
/*-------------------------------------------------------------------*/

#ifndef MAX_CPU_ENGINES
  #ifdef HAVE___UINT128_T
    #define MAX_CPU_ENGINES     128
  #else
    #define MAX_CPU_ENGINES      64
  #endif
#endif

#define PREF_DEF_MAXCPU           8     /*  Default sysblk.maxcpu
                                            to 8 according to old
                                            MAX_CPU_ENGINES default  */

/*-------------------------------------------------------------------*/
/* Miscellaneous system related constants we could be missing...     */
/*-------------------------------------------------------------------*/

#ifndef     MAX_PATH
  #ifdef    PATH_MAX
    #define MAX_PATH          PATH_MAX
  #else
    #define MAX_PATH          4096
  #endif
#endif

#define PATH_SEP            "/"

#if defined( _MSVC_ )

// The following are missing from MINGW32/MSVC...

#ifndef     S_IRGRP
  #define   S_IRGRP           0
#endif

#ifndef     S_IWGRP
  #define   S_IWGRP           0
#endif

#ifndef     SIGUSR2
  #define   SIGUSR2           31
#endif

#ifndef     IFNAMSIZ
  #define   IFNAMSIZ          16
#endif

#ifndef     IFHWADDRLEN
  #define   IFHWADDRLEN       6
#endif

#ifndef     EFAULT
  #if       defined          (WSAEFAULT)
    #define EFAULT            WSAEFAULT
  #else
    #define EFAULT            14
  #endif
#endif

#ifndef     ENOSYS
  #if       defined          (WSASYSCALLFAILURE)
    #define ENOSYS            WSASYSCALLFAILURE
  #else
    #define ENOSYS            88
  #endif
#endif

#ifndef     EOPNOTSUPP
  #if       defined          (WSAEOPNOTSUPP)
    #define EOPNOTSUPP        WSAEOPNOTSUPP
  #else
    #define EOPNOTSUPP        95
  #endif
#endif

#ifndef     ECONNRESET
  #if       defined          (WSAECONNRESET)
    #define ECONNRESET        WSAECONNRESET
  #else
    #define ECONNRESET        104
  #endif
#endif

#ifndef     ENOBUFS
  #if       defined          (ENOMEM)
    #define ENOBUFS           ENOMEM
  #else
    #define ENOBUFS           105
  #endif
#endif

#ifndef     EAFNOSUPPORT
  #if       defined          (WSAEAFNOSUPPORT)
    #define EAFNOSUPPORT      WSAEAFNOSUPPORT
  #else
    #define EAFNOSUPPORT      106
  #endif
#endif

#ifndef     EPROTOTYPE
  #if       defined          (WSAEPROTOTYPE)
    #define EPROTOTYPE        WSAEPROTOTYPE
  #else
    #define EPROTOTYPE        107
  #endif
#endif

#ifndef     ENOTSOCK
  #if       defined          (WSAENOTSOCK)
    #define ENOTSOCK          WSAENOTSOCK
  #else
    #define ENOTSOCK          108
  #endif
#endif

#ifndef     EADDRINUSE
  #if       defined          (WSAEADDRINUSE)
    #define EADDRINUSE        WSAEADDRINUSE
  #else
    #define EADDRINUSE        112
  #endif
#endif

#ifndef     ENETDOWN
  #if       defined          (WSAENETDOWN)
    #define ENETDOWN          WSAENETDOWN
  #else
    #define ENETDOWN          115
  #endif
#endif

#ifndef     ETIMEDOUT
  #if       defined          (WSAETIMEDOUT)
    #define ETIMEDOUT         WSAETIMEDOUT
  #else
    #define ETIMEDOUT         116
  #endif
#endif

#ifndef     EINPROGRESS
  #if       defined          (WSAEINPROGRESS)
    #define EINPROGRESS       WSAEINPROGRESS
  #else
    #define EINPROGRESS       119
  #endif
#endif

#ifndef     EMSGSIZE
  #if       defined          (E2BIG)
    #define EMSGSIZE          E2BIG
  #else
    #define EMSGSIZE          122
  #endif
#endif

#ifndef     EPROTONOSUPPORT
  #if       defined          (WSAEPROTONOSUPPORT)
    #define EPROTONOSUPPORT   WSAEPROTONOSUPPORT
  #else
    #define EPROTONOSUPPORT   123
  #endif
#endif

#ifndef     ENOTCONN
  #if       defined          (WSAENOTCONN)
    #define ENOTCONN          WSAENOTCONN
  #else
    #define ENOTCONN          128
  #endif
#endif

#ifndef     ENOTSUP
  #if       defined          (ENOSYS)
    #define ENOTSUP           ENOSYS
  #else
    #define ENOTSUP           134
  #endif
#endif

#ifndef     ENOMEDIUM
  #if       defined          (ENOENT)
    #define ENOMEDIUM         ENOENT
  #else
    #define ENOMEDIUM         135
  #endif
#endif

#ifndef     EOVERFLOW
  #if       defined          (ERANGE)
    #define EOVERFLOW         ERANGE
  #else
    #define EOVERFLOW         139
  #endif
#endif

#endif // defined(_MSVC_)

//         CLK_TCK  not part of SUSE 7.2 specs;  added.  (VB)
#ifndef    CLK_TCK
  #define  CLK_TCK       CLOCKS_PER_SEC
#endif

/*-------------------------------------------------------------------*/
/* Default console tn3270/telnet session TCP keepalive values        */
/*-------------------------------------------------------------------*/
#if !defined( HAVE_BASIC_KEEPALIVE )
/* No TCP keepalive support at all */
#define  KEEPALIVE_IDLE_TIME        0   /* Idle time to first probe  */
#define  KEEPALIVE_PROBE_INTERVAL   0   /* Probe timeout value       */
#define  KEEPALIVE_PROBE_COUNT      0   /* Max probe timeouts        */
#elif !defined( HAVE_PARTIAL_KEEPALIVE )
/* Basic TCP keepalive support */
#define  KEEPALIVE_IDLE_TIME        7200/* Idle time to first probe  */
#define  KEEPALIVE_PROBE_INTERVAL   1   /* Probe timeout value       */
#define  KEEPALIVE_PROBE_COUNT      10  /* Max probe timeouts        */
#else
/* Partial or Full TCP keepalive support */
#define  KEEPALIVE_IDLE_TIME        3   /* Idle time to first probe  */
#define  KEEPALIVE_PROBE_INTERVAL   1   /* Probe timeout value       */
#define  KEEPALIVE_PROBE_COUNT      10  /* Max probe timeouts        */
#endif // (KEEPALIVE)

/*-------------------------------------------------------------------*/
/* Miscellaneous Hercules-related constants...                       */
/*-------------------------------------------------------------------*/
#if defined(OPTION_WINDOWS_HOST_FILENAMES)
#define PATHSEPC '\\'        /* Windows Versions */
#define PATHSEPS "\\"
#else
#define PATHSEPC '/'         /* Everyone else */
#define PATHSEPS "/"
#endif

#define SPACE   ' '    /* <---<<< Look close! There's a space there! */

/* Definitions for OS tailoring - msb eq mon event, lsb eq oper exc. */
#define OS_NONE         0x7FFFFFFFF7DE7FFFULL   /* No spec OS tail.  */
#define OS_OS390        0x7FF673FFF7DE7FFDULL   /* OS/390            */
#define OS_ZOS          0x7B7673FFF7DE7FB7ULL   /* z/OS              */
#define OS_VSE          0x7FF673FFF7DE7FFFULL   /* VSE               */
#define OS_VM           0x7FFFFFFFF7DE7FFCULL   /* VM                */
#define OS_OPENSOLARIS  0xF8FFFFFFFFDE7FF7ULL   /* OpenSolaris       */
#if !defined(NO_IEEE_SUPPORT)
#define OS_LINUX        0x78FFFFFFF7DE7FF7ULL   /* Linux             */
#else
#define OS_LINUX        0x78FFFFFFF7DE7FD6ULL   /* Linux             */
#endif

/* Definitions for program product OS restriction flag. This flag is ORed
   with the SCLP READ CPU INFO response code. A 4 here makes the CPU look
   like an IFL (Integrated Facility for Linux) engine, which cannot run
   licensed ESA/390 or z/Architecture OSes. */
#define PGM_PRD_OS_RESTRICTED 4                 /* Restricted        */
#define PGM_PRD_OS_LICENSED   0                 /* Licensed          */

/* Storage access bits used by logical_to_main */
#define ACC_CHECK          0x0001          /* Possible storage update*/
#define ACC_WRITE          0x0002          /* Storage update         */
#define ACC_READ           0x0004          /* Storage read           */

/* Storage access bits used by other dat.h routines */
#define ACC_NOTLB          0x0100          /* Don't do TLB lookup    */
#define ACC_PTE            0x0200          /* Return page table entry*/
#define ACC_LPTEA          0x0400          /* Esame page table entry */
#define ACC_SPECIAL_ART    0x0800          /* Used by BSG            */
#define ACC_ENH_MC         0x1000          /* Used by Enhanced MC    */

#define ACCTYPE_HW         0               /* Hardware access        */
#define ACCTYPE_INSTFETCH  ACC_READ        /* Instruction fetch      */
#define ACCTYPE_READ       ACC_READ        /* Read storage           */
#define ACCTYPE_WRITE_SKP  ACC_CHECK       /* Write, skip change bit */
#define ACCTYPE_WRITE      ACC_WRITE       /* Write storage          */
#define ACCTYPE_TAR        0               /* TAR instruction        */
#define ACCTYPE_LRA        ACC_NOTLB       /* LRA instruction        */
#define ACCTYPE_TPROT      0               /* TPROT instruction      */
#define ACCTYPE_IVSK       0               /* ISVK instruction       */
#define ACCTYPE_BSG        ACC_SPECIAL_ART /* BSG instruction        */
#define ACCTYPE_PTE       (ACC_PTE|ACC_NOTLB) /* page table entry    */
#define ACCTYPE_SIE        0               /* SIE host access        */
#define ACCTYPE_STRAG      0               /* STRAG instruction      */
#define ACCTYPE_LPTEA     (ACC_LPTEA|ACC_NOTLB) /* LPTEA instruction */
#define ACCTYPE_EMC       (ACC_ENH_MC|ACCTYPE_WRITE) /* MC instr.    */

/* Special value for arn parameter for translate functions in dat.c */
#define USE_INST_SPACE          (-1)    /* Instruction space virtual */
#define USE_REAL_ADDR           (-2)    /* Real address              */
#define USE_PRIMARY_SPACE       (-3)    /* Primary space virtual     */
#define USE_SECONDARY_SPACE     (-4)    /* Secondary space virtual   */
#define USE_HOME_SPACE          (-5)    /* Home space virtual        */
#define USE_ARMODE              16      /* OR with access register
                                           number to force AR mode   */

/* Interception codes used by longjmp/SIE */
#define SIE_NO_INTERCEPT        (-1)    /* Continue (after pgmint)   */
#define SIE_HOST_INTERRUPT      (-2)    /* Host interrupt pending    */
#define SIE_HOST_PGMINT         (-3)    /* Host program interrupt    */
#define SIE_INTERCEPT_INST      (-4)    /* Instruction interception  */
#define SIE_INTERCEPT_INSTCOMP  (-5)    /* Instr. int TS/CS/CDS      */
#define SIE_INTERCEPT_EXTREQ    (-6)    /* External interrupt        */
#define SIE_INTERCEPT_IOREQ     (-7)    /* I/O interrupt             */
#define SIE_INTERCEPT_WAIT      (-8)    /* Wait state loaded         */
#define SIE_INTERCEPT_STOPREQ   (-9)    /* STOP reqeust              */
#define SIE_INTERCEPT_RESTART  (-10)    /* Restart interrupt         */
#define SIE_INTERCEPT_MCK      (-11)    /* Machine Check interrupt   */
#define SIE_INTERCEPT_EXT      (-12)    /* External interrupt pending*/
#define SIE_INTERCEPT_VALIDITY (-13)    /* SIE validity check        */
#define SIE_INTERCEPT_PER      (-14)    /* SIE guest per event       */
#define SIE_INTERCEPT_IOINT    (-15)    /* I/O Interruption          */
#define SIE_INTERCEPT_IOINTP   (-16)    /* I/O Interruption pending  */
#define SIE_INTERCEPT_IOINST   (-17)    /* I/O Instruction           */

#if defined(SIE_DEBUG_PERFMON)
#define SIE_PERF_ENTER          0       /* SIE performance monitor   */
#define SIE_PERF_ENTER_F       -31      /* Enter Fast (retain state) */
#define SIE_PERF_EXIT          -30      /* SIE exit                  */
#define SIE_PERF_RUNSIE        -29      /* run_sie entered           */
#define SIE_PERF_RUNLOOP_1     -28      /* run_sie runloop 1         */
#define SIE_PERF_RUNLOOP_2     -27      /* run_sue runloop 2         */
#define SIE_PERF_INTCHECK      -26      /* run_sie intcheck          */
#define SIE_PERF_EXEC          -25      /* run_sie execute inst      */
#define SIE_PERF_EXEC_U        -24      /* run_sie unrolled exec     */
#endif /*defined(SIE_DEBUG_PERFMON)*/

/*-------------------------------------------------------------------*/
/* Definitions for CTC protocol types                                */
/*-------------------------------------------------------------------*/

#define CTC_LCS                 1       /* LCS device                */
#define CTC_CTCI                2       /* CTC link to TCP/IP stack  */
#define CTC_PTP                 3       /* PTP link to TCP/IP stack  */
#define CTC_CTCE                4       /* Enhanced CTC link via TCP */

/*-------------------------------------------------------------------*/
/* Minimum, maximum and default scripting timeout values             */
/*-------------------------------------------------------------------*/

#define MIN_PAUSE_TIMEOUT       0.001   /* Minimum pause seconds     */
#define DEF_PAUSE_TIMEOUT         1.0   /* Default pause seconds     */
#define MAX_PAUSE_TIMEOUT       999.0   /* Maximum pause seconds     */

#define MIN_RUNTEST_DUR         0.001   /* Minimum runtest seconds   */
#define DEF_RUNTEST_DUR          30.0   /* Default runtest seconds   */
#define MAX_RUNTEST_DUR         300.0   /* Maximum runtest seconds   */

#define MAX_RUNTEST_FACTOR  (((4.0 * 1024.0 * 1024.0 * 1024.0) - 1.0) \
                            / 1000000.0 /* (usecs) */                 \
                            / MAX_RUNTEST_DUR)
#endif // _HCONSTS_H
