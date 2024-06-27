/* HCONSTS.H  (C) Copyright "Fish" (David B. Trout), 2005-2012       */
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
/*                     Maximum CPU Engines                           */
/*-------------------------------------------------------------------*/

#if !defined( MAX_CPU_ENGS )
  #if defined( HAVE___INT128_T )
    #define MAX_CPU_ENGS      128
  #else
    #define MAX_CPU_ENGS       64
  #endif
#endif

// (PREFERRED default MAX_CPU_ENGS)
#define PREF_DEF_MAXCPU         8       /* Default sysblk.maxcpu
                                           to 8 according to old
                                           MAX_CPU_ENGS default      */

#define MAX_CPU_LOOPS         256       /* UNROLLED_EXECUTE loops    */

/*-------------------------------------------------------------------*/
/*               Some handy quantity definitions                     */
/*-------------------------------------------------------------------*/
#define  ONE_KILOBYTE   ((U32)                     (1024))  /* 2^10 (16^2)  * 4  */
#define  TWO_KILOBYTE   ((U32)(2           *        1024))  /* 2^11 (16^2)  * 8  */
#define  FOUR_KILOBYTE  ((U32)(4           *        1024))  /* 2^12 (16^3)       */
#define  _64_KILOBYTE   ((U32)(64          *        1024))  /* 2^16 (16^4)       */
#define  HALF_MEGABYTE  ((U32)(512         *        1024))  /* 2^19 (16^4)  * 8  */
#define  ONE_MEGABYTE   ((U32)(1024        *        1024))  /* 2^20 (16^5)       */
#define  ONE_GIGABYTE   ((U64)ONE_MEGABYTE * (U64) (1024))  /* 2^30 (16^7)  * 4  */
#define  ONE_TERABYTE   (ONE_GIGABYTE      * (U64) (1024))  /* 2^40 (16^10)      */
#define  ONE_PETABYTE   (ONE_TERABYTE      * (U64) (1024))  /* 2^50 (16^12) * 4  */
#define  ONE_EXABYTE    (ONE_PETABYTE      * (U64) (1024))  /* 2^60 (16^15)      */
#if defined(U128)
#define  ONE_ZETTABYTE  (ONE_EXABYTE       * (U128)(1024))  /* 2^70 (16^17) * 4  */
#define  ONE_YOTTABYTE  (ONE_ZETTABYTE     * (U128)(1024))  /* 2^80 (16^20)      */
#endif

#define  SHIFT_KILOBYTE     10
#define  SHIFT_1K           10      // (slightly shorter name)
#define  SHIFT_2K           11
#define  SHIFT_4K           12
#define  SHIFT_64KBYTE      16
#define  SHIFT_MEGABYTE     20
#define  SHIFT_GIGABYTE     30
#define  SHIFT_TERABYTE     40
#define  SHIFT_PETABYTE     50
#define  SHIFT_EXABYTE      60
#define  SHIFT_ZETTABYTE    70
#define  SHIFT_YOTTABYTE    80

/* IEC Binary Prefixes, etc */
#define  ONE_KIBIBYTE  ((U32)                     (1024))  /* 2^10 (16^2)  * 4  */
#define  TWO_KIBIBYTE  ((U32)(2           *        1024))  /* 2^11 (16^2)  * 8  */
#define  FOUR_KIBIBYTE ((U32)(4           *        1024))  /* 2^12 (16^3)       */
#define  _64_KIBIBYTE  ((U32)(64          *        1024))  /* 2^16 (16^4)       */
#define  HALF_MEBIBYTE ((U32)(512         *        1024))  /* 2^19 (16^4)  * 8  */
#define  ONE_MEBIBYTE  ((U32)(1024        *        1024))  /* 2^20 (16^5)       */
#define  ONE_GIBIBYTE  ((U64)ONE_MEBIBYTE * (U64) (1024))  /* 2^30 (16^7)  * 4  */
#define  ONE_TEBIBYTE  (ONE_GIBIBYTE      * (U64) (1024))  /* 2^40 (16^10)      */
#define  ONE_PEBIBYTE  (ONE_TEBIBYTE      * (U64) (1024))  /* 2^50 (16^12) * 4  */
#define  ONE_EXBIBYTE  (ONE_PEBIBYTE      * (U64) (1024))  /* 2^60 (16^15)      */
#if defined(U128)
#define  ONE_ZEBIBYTE  (ONE_EXBIBYTE      * (U128)(1024))  /* 2^70 (16^17) * 4  */
#define  ONE_YOBIBYTE  (ONE_ZEBIBYTE      * (U128)(1024))  /* 2^80 (16^20)      */
#endif

#define  SHIFT_KIBIBYTE     10
#define  SHIFT_MEBIBYTE     20
#define  SHIFT_GIBIBYTE     30
#define  SHIFT_TEBIBYTE     40
#define  SHIFT_PEBIBYTE     50
#define  SHIFT_EXBIBYTE     60
#define  SHIFT_ZEBIBYTE     70
#define  SHIFT_YOBIBYTE     80

/* US VERSIONS */
#define ONE_HUNDRED     ((U32)                        (100))    /* zeros = 2  */
#define ONE_THOUSAND    ((U32)                       (1000))    /* zeros = 3  */
#define ONE_MILLION     ((U32)(1000           *       1000))    /* zeros = 6  */
#define ONE_BILLION     ((U64)ONE_MILLION     * (U64)(1000))    /* zeros = 9  */
#define ONE_TRILLION    ((U64)ONE_BILLION     * (U64)(1000))    /* zeros = 12 */
#define ONE_QUADRILLION ((U64)ONE_TRILLION    * (U64)(1000))    /* zeros = 15 */
#define ONE_QUINTILLION ((U64)ONE_QUADRILLION * (U64)(1000))    /* zeros = 18 */
#define ONE_SEXTILLION  ((U64)ONE_QUINTILLION * (U64)(1000))    /* zeros = 21 */
#define ONE_SEPTILLION  ((U64)ONE_SEXTILLION  * (U64)(1000))    /* zeros = 24 */

#define  _1K       1024         // (just a much shorter name)
#define  _2K       2048         // (just a much shorter name)
#define  _4K       4096         // (just a much shorter name)
#define  _1M    1048576         // (just a much shorter name)

/*-------------------------------------------------------------------*/
/*               Hercules "MAINSIZE" constants                       */
/*-------------------------------------------------------------------*/

#define MIN_370_MAINSIZE_BYTES      (1ULL << SHIFT_64KBYTE)
#define MIN_390_MAINSIZE_BYTES      (1ULL << SHIFT_MEGABYTE)
#define MIN_900_MAINSIZE_BYTES      (1ULL << SHIFT_MEGABYTE)
#define DEF_MAINSIZE_BYTES          (2ULL << SHIFT_MEGABYTE)
#define MAX_370_MAINSIZE_BYTES      (2ULL << SHIFT_GIGABYTE)  // (YES! 2GB!)
#define MAX_390_MAINSIZE_BYTES      (2ULL << SHIFT_GIGABYTE)
#define MAX_900_MAINSIZE_BYTES      (ULLONG_MAX)

#define MIN_370_MAINSIZE_PAGES      (MIN_370_MAINSIZE_BYTES  >> SHIFT_4K)
#define MIN_390_MAINSIZE_PAGES      (MIN_390_MAINSIZE_BYTES  >> SHIFT_4K)
#define MIN_900_MAINSIZE_PAGES      (MIN_900_MAINSIZE_BYTES  >> SHIFT_4K)
#define DEF_MAINSIZE_PAGES          (DEF_MAINSIZE_BYTES      >> SHIFT_4K)
#define MAX_370_MAINSIZE_PAGES      (MAX_370_MAINSIZE_BYTES  >> SHIFT_4K)
#define MAX_390_MAINSIZE_PAGES      (MAX_390_MAINSIZE_BYTES  >> SHIFT_4K)
#define MAX_900_MAINSIZE_PAGES      (MAX_900_MAINSIZE_BYTES  >> SHIFT_4K)

#define MIN_ARCH_MAINSIZE_BYTES     0   // (slot 0 = minimum for arch)
#define MAX_ARCH_MAINSIZE_BYTES     1   // (slot 1 = maximum for arch)

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

#define PATHSEPC '/'          // (character)
#define PATHSEPS "/"          // (string)
#define PATH_SEP "/"          // (same thing)

#define SPACE    ' '          //  <---<<<  Look close!  There's a space there!

#define MAX_ENVVAR_LEN        32768     // (just a reasonable limit)
#define MAX_CFG_LINELEN       32768     // (to support long defsyms)

#if    MAX_CFG_LINELEN         <  MAX_ENVVAR_LEN
#error MAX_CFG_LINELEN must be >= MAX_ENVVAR_LEN
#endif

#if defined( _MSVC_ )

// The following are missing from MINGW32/MSVC...

#ifndef     S_IRGRP
  #define   S_IRGRP           0
#endif

#ifndef     S_IWGRP
  #define   S_IWGRP           0
#endif

#ifndef     SIGUSR1
  #define   SIGUSR1           30
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
/*       Definitions for program product OS restriction flag.        */
/*-------------------------------------------------------------------*/
/* This flag is ORed with the SCLP READ CPU INFO response code. A    */
/* '4' here makes the CPU look like an IFL (Integrated Facility for  */
/* Linux) engine which can't run licensed ESA/390 or z/Arch OSes.    */
/*-------------------------------------------------------------------*/
#define PGM_PRD_OS_RESTRICTED 4                 /* Restricted        */
#define PGM_PRD_OS_LICENSED   0                 /* Licensed          */

/*-------------------------------------------------------------------*/
/*           Storage access bits used by logical_to_main_l           */
/*-------------------------------------------------------------------*/
#define ACC_CHECK          0x0001          /* Possible storage update*/
#define ACC_WRITE          0x0002          /* Storage update         */
#define ACC_READ           0x0004          /* Storage read           */

/*-------------------------------------------------------------------*/
/*        Storage access bits used by other dat.h routines           */
/*-------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------*/
/* Special value for arn parameter for translate functions in dat.c  */
/*-------------------------------------------------------------------*/
#define USE_INST_SPACE          (-1)    /* Instruction space virtual */
#define USE_REAL_ADDR           (-2)    /* Real address (DAT access)
                                           (prevents TLB hit/use)    */
#define USE_PRIMARY_SPACE       (-3)    /* Primary space virtual     */
#define USE_SECONDARY_SPACE     (-4)    /* Secondary space virtual   */
#define USE_HOME_SPACE          (-5)    /* Home space virtual        */
#define USE_ARMODE              (16)    /* OR with access register
                                           number to force AR mode   */
/*-------------------------------------------------------------------*/
/*              Interception codes used by longjmp/SIE               */
/*-------------------------------------------------------------------*/
#define SIE_NO_INTERCEPT        (-1)    /* Continue (after pgmint)   */
#define SIE_HOST_INT_PEND       (-2)    /* Host interrupt pending    */
#define SIE_HOST_PGM_INT        (-3)    /* Host program interrupt    */
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
#define SIE_MAX_NEG            (-17)    /* (maximum negative value)  */

#if defined( SIE_DEBUG_PERFMON )
#define SIE_PERF_ENTER         ( 0 )    /* SIE performance monitor   */
#define SIE_MIN_NEG_DEBUG      (-24)    /* (minimum negative value)  */
#define SIE_PERF_EXEC_U        (-24)    /* run_sie unrolled exec     */
#define SIE_PERF_EXEC          (-25)    /* run_sie execute inst      */
#define SIE_PERF_INTCHECK      (-26)    /* run_sie intcheck          */
#define SIE_PERF_RUNLOOP_2     (-27)    /* run_sue runloop 2         */
#define SIE_PERF_RUNLOOP_1     (-28)    /* run_sie runloop 1         */
#define SIE_PERF_RUNSIE        (-29)    /* run_sie entered           */
#define SIE_PERF_EXIT          (-30)    /* SIE exit                  */
#define SIE_PERF_ENTER_F       (-31)    /* Enter Fast (retain state) */
#define SIE_MAX_NEG_DEBUG      (-31)    /* (maximum negative value)  */
#endif

/*-------------------------------------------------------------------*/
/*             Definitions for CTC protocol types                    */
/*-------------------------------------------------------------------*/
#define CTC_LCS                 1       /* LCS device                */
#define CTC_CTCI                2       /* CTC link to TCP/IP stack  */
#define CTC_PTP                 3       /* PTP link to TCP/IP stack  */
#define CTC_CTCE                4       /* Enhanced CTC link via TCP */
#define CTC_CTCT                6       /* CTC link via TCP          */

#define CTCE_TRACE_ON          -1       /* CTCE permanent tracing on */
#define CTCE_TRACE_OFF         -2       /* CTCE tracing turned off   */
#define CTCE_TRACE_STARTUP     20       /* CTCE startup tracing max  */

/*-------------------------------------------------------------------*/
/*                Script processing constants                        */
/*-------------------------------------------------------------------*/
#define MAX_SCRIPT_STMT       1024      /* Max script stmt length    */
#define MAX_SCRIPT_DEPTH        10      /* Max script nesting depth  */

#define MIN_PAUSE_TIMEOUT    0.001      /* Minimum pause seconds     */
#define DEF_PAUSE_TIMEOUT      1.0      /* Default pause seconds     */
#define MAX_PAUSE_TIMEOUT    999.0      /* Maximum pause seconds     */

#define MIN_RUNTEST_DUR      0.001      /* Minimum runtest seconds   */
#define DEF_RUNTEST_DUR       30.0      /* Default runtest seconds   */
#define MAX_RUNTEST_DUR      300.0      /* Maximum runtest seconds   */

#define MAX_RUNTEST_FACTOR  (((4.0 * 1024.0 * 1024.0 * 1024.0) - 1.0) \
                            / 1000000.0 /* (usecs) */                 \
                            / MAX_RUNTEST_DUR)

/*-------------------------------------------------------------------*/
/*                  Watchdog thread interval                         */
/*-------------------------------------------------------------------*/

#define WATCHDOG_SECS           20      /* watchdog thread interval  */
#define WAIT_FOR_DEBUGGER_SECS  45      /* wait for debugger attach  */

/*-------------------------------------------------------------------*/
/*                   Panel thread heartbeat                          */
/*-------------------------------------------------------------------*/

#define WAIT_FOR_KEYBOARD_INPUT_SLEEP_MILLISECS  (20)

/*-------------------------------------------------------------------*/
/*       Definitions for "OSTAILOR" command/statement                */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  MOST  significant bit  =  Monitor event         (code 0x0040)    */
/*  LEAST significant bit  =  Operation exception   (code 0x0001)    */
/*                                                                   */
/*  An 'ON'  bit means the Program Interrupt *WILL* be traced.       */
/*  An 'OFF' bit means the Program Interrupt will *NOT* be traced.   */
/*                                                                   */
/*  The 'pgmtrace' command can be used to display the bit setting.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
#define PGMBIT( pgm_code )   (1ULL << (pgm_code-1)) /* helper macro  */

#define OSTAILOR_QUIET          0x80000000
#define       OS_QUIET         (0x0000000000000000ULL) /* Trace none */
#define OSTAILOR_NULL           0x40000000
#define       OS_NULL          (0xFFFFFFFFFFFFFFFFULL) /* Trace all  */
#define OSTAILOR_DEFAULT        0x20000000
#define       OS_DEFAULT       (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_SPACE_SWITCH_EVENT                    )        \
        | PGMBIT(  PGM_MONITOR_EVENT                         )        \
     ))
#define OSTAILOR_OS390          0x10000000
#define       OS_OS390         (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_PRIVILEGED_OPERATION_EXCEPTION        )        \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_SPACE_SWITCH_EVENT                    )        \
        | PGMBIT(  PGM_ASTE_VALIDITY_EXCEPTION               )        \
        | PGMBIT(  PGM_ASTE_SEQUENCE_EXCEPTION               )        \
        | PGMBIT(  PGM_STACK_FULL_EXCEPTION                  )        \
        | PGMBIT(  PGM_STACK_EMPTY_EXCEPTION                 )        \
        | PGMBIT(  PGM_STACK_OPERATION_EXCEPTION             )        \
        | PGMBIT(  PGM_MONITOR_EVENT                         )        \
     ))
#define OSTAILOR_ZOS            0x08000000
#define       OS_ZOS           (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_PROTECTION_EXCEPTION                  )        \
        | PGMBIT(  PGM_DATA_EXCEPTION                        )        \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_SPACE_SWITCH_EVENT                    )        \
        | PGMBIT(  PGM_ASTE_VALIDITY_EXCEPTION               )        \
        | PGMBIT(  PGM_ASTE_SEQUENCE_EXCEPTION               )        \
        | PGMBIT(  PGM_STACK_FULL_EXCEPTION                  )        \
        | PGMBIT(  PGM_STACK_EMPTY_EXCEPTION                 )        \
        | PGMBIT(  PGM_STACK_OPERATION_EXCEPTION             )        \
        | PGMBIT(  PGM_ASCE_TYPE_EXCEPTION                   )        \
        | PGMBIT(  PGM_REGION_THIRD_TRANSLATION_EXCEPTION    )        \
        | PGMBIT(  PGM_MONITOR_EVENT                         )        \
     ))
#define OSTAILOR_VSE            0x04000000
#define       OS_VSE           (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_SPACE_SWITCH_EVENT                    )        \
        | PGMBIT(  PGM_ASTE_VALIDITY_EXCEPTION               )        \
        | PGMBIT(  PGM_ASTE_SEQUENCE_EXCEPTION               )        \
        | PGMBIT(  PGM_STACK_FULL_EXCEPTION                  )        \
        | PGMBIT(  PGM_STACK_EMPTY_EXCEPTION                 )        \
        | PGMBIT(  PGM_STACK_OPERATION_EXCEPTION             )        \
        | PGMBIT(  PGM_MONITOR_EVENT                         )        \
     ))
#define OSTAILOR_VM             0x02000000
#define       OS_VM            (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_OPERATION_EXCEPTION                   )        \
        | PGMBIT(  PGM_PRIVILEGED_OPERATION_EXCEPTION        )        \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_SPACE_SWITCH_EVENT                    )        \
        | PGMBIT(  PGM_MONITOR_EVENT                         )        \
     ))
#if !defined( NO_IEEE_SUPPORT )

#define OSTAILOR_LINUX          0x01000000
#define       OS_LINUX         (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_PROTECTION_EXCEPTION                  )        \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_SPACE_SWITCH_EVENT                    )        \
        | PGMBIT(  PGM_REGION_FIRST_TRANSLATION_EXCEPTION    )        \
        | PGMBIT(  PGM_REGION_SECOND_TRANSLATION_EXCEPTION   )        \
        | PGMBIT(  PGM_REGION_THIRD_TRANSLATION_EXCEPTION    )        \
        | PGMBIT(  PGM_MONITOR_EVENT                         )        \
     ))

#else // defined( NO_IEEE_SUPPORT )

#define OSTAILOR_LINUX          0x01000000
#define       OS_LINUX         (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_OPERATION_EXCEPTION                   )        \
        | PGMBIT(  PGM_PROTECTION_EXCEPTION                  )        \
        | PGMBIT(  PGM_SPECIFICATION_EXCEPTION               )        \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_SPACE_SWITCH_EVENT                    )        \
        | PGMBIT(  PGM_REGION_FIRST_TRANSLATION_EXCEPTION    )        \
        | PGMBIT(  PGM_REGION_SECOND_TRANSLATION_EXCEPTION   )        \
        | PGMBIT(  PGM_REGION_THIRD_TRANSLATION_EXCEPTION    )        \
        | PGMBIT(  PGM_MONITOR_EVENT                         )        \
     ))

#endif
#define OSTAILOR_OPENSOLARIS    0x00800000
#define       OS_OPENSOLARIS   (0xFFFFFFFFFFFFFFFFULL &               \
    ~(0ULL                                                            \
        | PGMBIT(  PGM_PROTECTION_EXCEPTION                  )        \
        | PGMBIT(  PGM_SEGMENT_TRANSLATION_EXCEPTION         )        \
        | PGMBIT(  PGM_PAGE_TRANSLATION_EXCEPTION            )        \
        | PGMBIT(  PGM_TRACE_TABLE_EXCEPTION                 )        \
        | PGMBIT(  PGM_REGION_FIRST_TRANSLATION_EXCEPTION    )        \
        | PGMBIT(  PGM_REGION_SECOND_TRANSLATION_EXCEPTION   )        \
        | PGMBIT(  PGM_REGION_THIRD_TRANSLATION_EXCEPTION    )        \
     ))

/*-------------------------------------------------------------------*/
/*         Channel constants, also needed by ckddasd code            */
/*-------------------------------------------------------------------*/

/*  CAUTION: for performance reasons the size of the data portion
    of IOBUF (i.e. IOBUF_MINSIZE) should never be less than 64K-1,
    and sizes greater than 64K may cause stack overflows to occur.
*/
#define IOBUF_ALIGN         4096          /* Must be a power of 2    */
#define IOBUF_HDRSIZE       IOBUF_ALIGN   /* Multiple of IOBUF_ALIGN */
#define IOBUF_MINSIZE       65536         /* Multiple of IOBUF_ALIGN */
#define IOBUF_INCREASE      1048576       /* Multiple of IOBUF_ALIGN */

/*-------------------------------------------------------------------*/
/*         Default command prefixes for Integrated Consoles          */
/*-------------------------------------------------------------------*/

#define DEF_CMDPREFIXES     "/`=~@$%^&_:?0123456789"

#endif // _HCONSTS_H
