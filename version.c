/* VERSION.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2021                             */
/*              Hercules Version Display Module                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module displays the Hercules program name and version,       */
/* copyright notice, build date and time, and build information.     */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _VERSION_C_
#define _HUTIL_DLL_

#include "hercules.h"
#include "machdep.h"

/*-------------------------------------------------------------------*/
/*           "Unusual" (i.e. noteworthy) build options...            */
/*-------------------------------------------------------------------*/

static const char *build_info[] = {

/*-------------------------------------------------------------------*/
/*                        Built with:                                */
/*-------------------------------------------------------------------*/

    "Built with: "

/*  Report compiler environment
 *
 *  The build process, when able, can set
 *  CONFIGURE_COMPILER_VERSION_STRING to the desired value for reporting
 *  purposes, and not rely on the partial information that the compilers
 *  offer via predefined macro values.
 *
 *  Predefined macro values for individual compilers is documented at
 *  http://sourceforge.net/p/predef/wiki/Compilers/.
 *
 *  +------------------------------------------------------------------+
 *  |   CAUTION                                                        |
 *  |   -------                                                        |
 *  |   The specification of any compiler here is not to be            |
 *  |   construed as being supported by Hercules or that the           |
 *  |   compiler can be successfully used to compile Hercules.         |
 *  +------------------------------------------------------------------+
 *
 */

#define __string(s)     #s
#define __defer(m, ...) m(__VA_ARGS__)
#define value(s)        __defer(__string, s)

#if defined(CONFIGURE_COMPILER_VERSION_STRING)
    CONFIGURE_COMPILER_VERSION_STRING,
#elif defined(_ACC_)
    "ACC",
#elif defined(_CMB_)
    "Altium MicroBlaze C " value(__VERSION__)
    #if defined(__REVISION__)
        " Patch " value(__REVISION__)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(__CHC__)
    "Altium C-to-Hardware " value(__VERSION)
    #if defined(__REVISION__)
        " Patch " value(__REVISION__)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(__ACK__)
    "Amsterdam Compiler Kit",
#elif defined(__CC_ARM)
    "ARM Compiler " __ARMCC_VERSION,
#elif defined(AZTEC_C) || defined(__AZTEC_C__)
    "Aztec C " value(__VERSION),
#elif defined(__CC65__)
    "CC65 " value(__CC65__),
#elif defined(__clang__)
    #if defined(__apple_build_version__)
        "Apple "
    #endif
    "Clang"
    #if defined(_MSC_VER)
        #if defined(__c2__)
            "/C2"
        #elif defined(__llvm__)
            "/LLVM"
        #endif
    #endif
    " " __clang_version__,
#elif defined(__DECC)
    "Compaq C " value(__DECC_VER),
#elif defined(__convexc__)
    "Convex C",
#elif defined(__COMPCERT__)
    "CompCert",
#elif defined(__COVERITY__)
    "Coverity C Static Analyzer",
#elif defined(__DCC__)
    "Diab C " value(__VERSION_NUMBER__),
#elif defined(_DICE)
    "DICE C",
#elif defined(__DMC__)
    "Digital Mars " value(__DMC__),
#elif defined(__SYSC__)
    "Dignus Systems/C " value(__SYSC_VER__),
#elif defined(__DJGPP__)
    "DJGPP " __DJGPP__ "." __DJGPP_MINOR__,
#elif defined(__PATHCC__)
    "EKOPATH " value(__PATHCC__) "." value(__PATHCC_MINOR__)
    #if defined(__PATHCC_PATCHLEVEL__)
         "." value(__PATHCC_PATCHLEVEL__)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(__ghs__)
    "Green Hill C " value(__GHS_VERSION_NUMBER__)
                " " value(__GHS_REVISION_DATE__),
#elif defined(__IAR_SYSTEMS_ICC__)
    "IAR C " __VER__,
#elif defined(__xlc__) || defined(__xlC__) || defined(__IBMC__)
    "IBM "
    #if defined(__MVS__) || defined(__COMPILER_VER__)
        "z/OS C "
    #else
        "XL C "
    #endif
    value(__IBMC__),
#elif defined(__INTEL_COMPILER) || defined(__ICC) ||                    \
      defined(_ECC) || defined(_ICL)
    "Intel C " value(__INTEL_COMPILER)
           " " value(__INTEL_COMPILER_BUILD_DATE),
#elif defined(__IMAGECRAFT__)
    "ImageCraft C",
#elif defined(__KEIL__)
    "KEIL CARM " value(__CA__),
#elif defined(__C166__)
    "KEIL C166",
#elif defined(__C51__)
    "KEIL C51",
#elif defined(__LCC__)
    "LCC",
#elif defined(__llvm__)
    "LLVM",
#elif defined(__HIGHC__)
    "MetaWare High C",
#elif defined(__MWERKS__)
    "Metrowerks CodeWarrior",
#elif defined( _MSC_VER )

   /* && !defined(__clang__) by definition due to prior test     */
   /*     __clang__ with _MSC_VER indicates MSC build with Clang */
   /*     and the intent here is to only identify the compiler.  */

    "Microsoft "

#if   _MSC_VER == VS2008
     "Visual Studio 2008"
#elif _MSC_VER == VS2010
     "Visual Studio 2010"
#elif _MSC_VER == VS2012
     "Visual Studio 2012"
#elif _MSC_VER == VS2013
     "Visual Studio 2013"
#elif _MSC_VER == VS2015
     "Visual Studio 2015"
#elif _MSC_VER >= VS2017  && _MSC_VER < VS2019
     "Visual Studio 2017"
#elif _MSC_VER >= VS2019  && _MSC_VER < VS2022
     "Visual Studio 2019"
#elif _MSC_VER >= VS2022
     "Visual Studio 2022"
#else
     "Visual C"
#endif
        " (MSVC " value( _MSC_FULL_VER )
    #if defined(   _MSC_BUILD )
        " " value( _MSC_BUILD )
    #endif

        ")"

    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(_MRI)
    "Microtec C",
#elif defined(__NDPC__) || defined(__NDPX__)
    "Microway NDP C",
#elif defined(__sgi)
    "MIPSpro "
    #if defined(_COMPILER_VERSION)
        value(_COMPILER_VERSION)
    #else
        value(_SGI_COMPILER_VERSION)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(MIRACLE)
    "Miracle C",
#elif defined(__CC_NORCROFT)
    "Norcroft C",   /* __ARMCC_VERSION is a floating-point number */
#elif defined(__NWCC__)
    "NWCC",
#elif defined(__OPEN64__) || defined(__OPENCC__)
    "Open64 "
    #if defined(__OPEN64__)
        __OPEN64__
    #else
        value(__OPENCC__)
        "." value(__OPENCC_MINOR__)
        /* __OPENCC_PATCHLEVEL__ is a floating-point number */
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(__SUNPRO_C)
    "Oracle Solaris Studio",
#elif defined(__PACIFIC__)
    "Pacific C",
#elif defined(_PACC_VER)
    "Palm C",
#elif defined(__POCC__)
    "Pelles C",
#elif defined(__PGI)
    "Portland Group C " value(__PGIC__)
                    "." value(__PGIC_MINOR__)
    #if defined(__PGIC_PATCHLEVEL__) && __PGIC_PATCHLEVEL__ > 0
                    "." value(__PGIC_PATCHLEVEL__)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(__RENESAS__) || defined(__HITACHI__)
    #if defined(__RENESAS__)
        "Renesas C " value(__RENESAS_VERSION__)
    #else
        "Hitachi C " value(__HITACHI_VERSION__)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(SASC) || defined(__SASC) || defined(__SASC__)
    "SAS/C "
    #if defined(__VERSION__) && defined(__REVISION__)
        value(__VERSION__) "." value(__REVISION__)
    #else
        value(__SASC__)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#elif defined(_SCO_DS)
    "SCO OpenServer",
#elif defined(SDCC)
    "Small Device C Compiler " value(SDCC),
#elif defined(__SNC__)
    "SN Compiler",
#elif defined(__VOSC__)
    "Stratus VOS "
    #if __VOSC__
        "K&R"
    #else
        "ANSI"
    #endif
    " C Compiler",
#elif defined(__TenDRA__)
    "TenDRA C",
#elif defined(__TI_COMPILER_VERSION__)
    "Texas Instruments C Compiler" value(__TI_COMPILER_VERSION),
#elif defined(THINKC3) || defined(THINKC4)
    "THINK C Version "
    #if defined(THINKC3)
        "3"
    #else
        "4"
    #endif
    ".x",
#elif defined(__TINYC__)
    "Tiny C",
#elif defined(__TURBOC__)
    "Turbo C " value(__TURBOC__),
#elif defined(_UCC)
    "Ultimate C " value(_MAJOR_REV) "." value(_MINOR_REV),
#elif defined(__USLC__)
    "USL C " value(__SCO_VERSION__),
#elif defined(__VBCC__)
    "VBCC",
#elif defined(__GNUC__)
    /*
     * This must be the last test due to "GCC compliant" compilers from
     * other vendors.
     */
    "GCC " __VERSION__,
#else
    /* Unknown compiler in use... */
#endif


/*  Report toolchain
 *
 *  The build process, when able, can set
 *  CONFIGURE_TOOLCHAIN_VERSION_STRING to the desired value for
 *  purposes, and not rely on the partial information that the compilers
 *  toolchain reporting offer via predefined macro values.
 *
 */

#if defined(CONFIGURE_TOOLCHAIN_VERSION_STRING)
    CONFIGURE_TOOLCHAIN_VERSION_STRING,
#elif defined(__MINGW32__) || defined(__MINGW64__)
    "MinGW "
    #if defined(__MINGW32__)
        "32-bit " value(__MINGW32_MAJOR_VERSION__)
              "." value(__MINGW32_MINOR_VERSION__)
    #else
        "64-bit " value(__MINGW64_VERSION_MAJOR__)
              "." value(__MINGW64_VERSION_MINOR__)
    #endif
    ,   /* Don't forget the comma to keep the compiler happy! */
#else
    /* Unknown toolchain in use... */
#endif


/* Report Host Environment */

#if !defined(HOST_ARCH)
  #error HOST_ARCH is undefined
#endif

#if defined(_MSVC_)
  #define QSTR_HOST_ARCH    QSTR(HOST_ARCH)
#else
  #define QSTR_HOST_ARCH         HOST_ARCH
#endif

/*-------------------------------------------------------------------*/
/*                        Build type:                                */
/*-------------------------------------------------------------------*/

    "Build type: "

#if   defined(_AIX)
    "AIX"
#elif defined(__ANDROID__)
    "Android"
    #if defined(__ANDROID_API__)
        value(__ANDROID_API__)
    #endif
#elif defined(UTS)
    "Amdahl UTS"
#elif defined(AMIGA) || defined(__amigaos__)
    "AmigaOS"
#elif defined(aegis)
    "Apollo AEGIS"
#elif defined(apollo)
    "Apollo Domain/OS"
#elif defined(__OS__)
    "BeOS"
#elif defined(__bg__) || defined(__THW_BLUEGENE__)
    "Blue Gene"
    #if defined(__bgq__) || defined(__TOS_BGQ__)
        "/Q"
    #endif
#elif defined(__FREEBSD__)                                          ||  \
      defined(__NetBSD__)                                           ||  \
      defined(__OpenBSD__)                                          ||  \
      defined(__bsdi__)                                             ||  \
      defined(__DragonFly__)
    #if defined(__FREEBSD__)
        "Free"
    #elif defined(__NetBSD__)
        "Net"
    #elif defined(__OpenBSD__)
        "Open"
    #elif defined(__DragonFly__)
        "DragonFly "
    #endif
    "BSD"
    #if defined(__bsdi__)
        "/OS"
    #endif
    #if   defined(BSD)
        " " value(BSD)
    #elif defined(__FreeBSD_version)
        " " value(__FreeBSD_version)
    #elif defined(__FreeBSD__)
        " " value(__FreeBSD__)
    #elif defined(__NetBSD_Version__)
        " " value(__NetBSD_Version__)
    #endif
#elif defined(__convex__)
    "ConvexOS"
#elif defined(__CYGWIN__)
    "Cygwin Environment"
#elif defined(__dgux__) || defined(__DGUX__)
    "DG/UX"
#elif defined(_SEQUENT_) || defined(sequent)
    "DYNIX/ptx"
#elif defined(_ECOS)
    "eCos"
#elif defined(__EMX__)
    "EMX Environment"
#elif defined(__gnu_hurd__)
    "GNU/Hurd"
#elif defined(__FreeBSD_kernel__) && defined(__GLIBC__)
    "GNU/kFreeBSD"
#elif defined(__gnu_linux__)
    "GNU/Linux"
#elif defined(_hpux) || defined(hpux) || defined(__hpux)
    "HP-UX"
#elif defined(__OS400__)
    "IBM OS/400"
#elif defined(__INTEGRITY)
    "INTEGRITY"
#elif defined(__INTERIX)
    "Interix Environment"
#elif defined(sgi) || defined(__sgi)
    "IRIX"
#elif defined(__linux__)    // non-GNU-based Linuxes do exist
    "Linux"
#elif defined(__Lynx__)
    "LynxOS"
#elif defined(macintosh) || defined(Macintosh) ||                       \
      (defined(__APPLE__) && defined(__MACH__))
    "Mac OS "
    #if defined(macintosh) || defined(Macintosh)
        "9"
    #else
        "X"
    #endif
#elif defined(__OS9000) || defined(_OSK)
    "Microware OS-9"
#elif defined(__minix)
    "MINIX"
#elif defined(__MORPHOS__)
    "MorphOS"
#elif defined(mpeix) || defined(__mpexl)
    "MPE/iX"
#elif defined(MSDOS) || defined(__MSDOS__) || defined(_MSDOS) ||        \
      defined(__DOS__)
    "MSDOS"
#elif defined(__TANDEM)
    "Tandem NonStop"
#elif defined(__nucleus__)
    "Nucleus RTOS"
#elif defined(OS2) || defined(_OS2) || defined(__OS2__) ||              \
      defined(__TOS_OS2__)
    "OS/2"
#elif defined(__palmos__)
    "Palm OS"
#elif defined(EPLAN9)
    "Plan 9"
#elif defined(pyr)
    "Pyramid DC/OSx"
#elif defined(__QNX__) || defined(__QNXNTO__)
    "QNX"
    #if   defined(_NTO_VERSION)
        " " value(_NTO_VERSION)
    #elif defined(BBNDK_VERSION_CURRENT)
        " " value(BBNDK_VERSION_CURRENT)
    #endif
#elif defined(sinux)
    "Reliant UNIX"
#elif defined(_SCO_DS)
    "SCO OpenServer"
#elif defined(sun) || defined(__sun)
    #if defined(__SVR4) || defined(__svr4__)
        "Solaris"
    #else
        "SunOS"
    #endif
#elif defined(__VOS__)
    "Stratus VOS " value(__VOS__)
#elif defined(__SYLLABLE__)
    "Syllable"
#elif defined(__SYMBIAN32__)
    "Symbian OS"
#elif defined(__osf__) || defined(__osf)
    "Tru64 (OSF/1)"
#elif defined(ultrix) || defined(__ultrix) || defined(__ultrix__)
    "Ultrix"
#elif defined(_UNICOS)
    "UNICOS " value(_UNICOS)
#elif defined(_CRAY) || defined(__crayx1)
    "UNICOS/mp"
#elif defined(sco) || defined(_UNIXWARE7)
    "UnixWare"
#elif defined(_UWIN)
    "U/Win"
#elif defined(VMS) || defined(__VMS)
    "VMS " value(__VMS_PER)
#elif defined(__VXWORKS__) || defined(__vxworks)
    "VxWorks"
    #if   defined(_WRS_VXWORKS_MAJOR)
        " " value(_WRS_VXWORKS_MAJOR)
        "." value(_WRS_VXWORKS_MINOR)
        "." value(_WRS_VSWORKS_MAINT)
    #endif
    #if defined(__RTP__)
        " Real-Time"
    #endif
    #if defined(_WRS_KERNEL)
        " Kernel"
    #endif
#elif defined(_MSVC_) ||                                                \
      defined(_WIN16) || defined(_WIN32) || defined(_WIN64) ||          \
      defined(__WIN32__) || defined(__WINDOWS__) ||                     \
      defined(_WIN32_WCE)
    "Windows"
    #if   defined(_WIN32_WCE)
        " CE " value(_WIN32_WCE)
    #elif defined(_MSVC_) && !defined(__llvm__)
        " MSVC"
    #endif
#elif defined(_WINDU_SOURCE)
    "Wind/U " value(_WINDU_SOURCE) " Environment"
#elif defined(__MVS__) || defined(__HOS_MVS__)
    "z/OS"
#else
    "*nix"
#endif
    " "
    QSTR_HOST_ARCH
    " "
#if defined(DEBUG)
    "** DEBUG ** "
#endif
    "host architecture build",

/*  Report HQA configuration information
 *
 *  FIXME: Add this section
 *
 */

/*-------------------------------------------------------------------*/
/*                   Mode and Max CPU Engines:                       */
/*-------------------------------------------------------------------*/

/* Report emulation modes */

#if !defined(_ARCH_NUM_1)
    "Mode:"
#else
    "Modes:"
#endif
#if defined(_370)
    " " _ARCH_370_NAME
#endif
#if defined(_390)
    " " _ARCH_390_NAME
#endif
#if defined(_900)
    " " _ARCH_900_NAME
#endif
    ,

    "Max CPU Engines: " QSTR( MAX_CPU_ENGS ),

/*-------------------------------------------------------------------*/
/*                            Using:                                 */
/*-------------------------------------------------------------------*/

    "Using   shared libraries",

#if !defined(_MSVC_)
  #if !defined(NO_SETUID)
    "Using   "
    #if defined(HAVE_SETRESUID)
     "setresuid()"
    #elif defined(HAVE_SETREUID)
     "setreuid()"
    #else
     "(UNKNOWN)"
    #endif
     " for setting privileges",
  #endif
#endif

#if defined( OPTION_FTHREADS )
    "Using   Fish threads Threading Model",
#else
    "Using   POSIX threads Threading Model",
#endif

#if        OPTION_MUTEX_DEFAULT == OPTION_MUTEX_NORMAL
    "Using   Normal Mutex Locking Model",
#elif      OPTION_MUTEX_DEFAULT == OPTION_MUTEX_ERRORCHECK
    "Using   Error-Checking Mutex Locking Model",
#elif      OPTION_MUTEX_DEFAULT == OPTION_MUTEX_RECURSIVE
    "Using   Recursive Mutex Locking Model",
#else
    "Using   (undefined) Mutex Locking Model",
#endif

/*-------------------------------------------------------------------*/
/*                        With / Without:                            */
/*-------------------------------------------------------------------*/

#if !defined(_MSVC_)
  #if defined(NO_SETUID)
    "Without setuid support",
  #endif
#endif

#if defined(OPTION_SHARED_DEVICES)
    "With    Shared Devices support",
#else
    "Without Shared Devices support",
#endif

    "With    Dynamic loading support",
    "With    External GUI support",

// (only report when full/complete keepalive is not available)
#if !defined( HAVE_FULL_KEEPALIVE )
  #if defined( HAVE_PARTIAL_KEEPALIVE )
      "With    Partial TCP keepalive support",
  #elif defined( HAVE_BASIC_KEEPALIVE )
      "With    Basic TCP keepalive support",
  #else
      "Without TCP keepalive support",
  #endif
#endif

#if defined(ENABLE_IPV6)
    "With    IPV6 support",
#else
    "Without IPV6 support",
#endif

    "With    HTTP Server support",

#if defined(NO_IEEE_SUPPORT)
    "Without IEEE support",
#else
    #if defined(HAVE_SQRTL)
        "With    sqrtl support",
    #else
        "Without sqrtl support",
    #endif
#endif

#if defined( HAVE_SIGNAL_HANDLING )
    "With    Signal handling",
#else
    "Without Signal handling",
#endif

#if defined( OPTION_WATCHDOG )
    "With    Watchdog monitoring",
#else
    "Without Watchdog monitoring",
#endif

#if defined( CCKD_BZIP2 )
    "With    CCKD BZIP2 support",
#else
    "Without CCKD BZIP2 support",
#endif

#if defined(HET_BZIP2)
    "With    HET BZIP2 support",
#else
    "Without HET BZIP2 support",
#endif

#if defined( HAVE_ZLIB )
    "With    ZLIB support",
#else
    "Without ZLIB support",
#endif

#if defined(HAVE_REGEX_H) || defined(HAVE_PCRE)
    "With    Regular Expressions support",
#else
    "Without Regular Expressions support",
#endif

#if defined(HAVE_OBJECT_REXX)
    "With    Object REXX support",
#else
    "Without Object REXX support",
#endif
#if defined(HAVE_REGINA_REXX)
    "With    Regina REXX support",
#else
    "Without Regina REXX support",
#endif

#if defined(OPTION_HAO)
    "With    Automatic Operator support",
#else
    "Without Automatic Operator support",
#endif

    "Without National Language Support",
    "With    CCKD64 Support",

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
    "With    Transactional-Execution Facility support",
#else
    "Without Transactional-Execution Facility support",
#endif

#if defined( OPTION_OPTINST )
    "With    \"Optimized\" instructions",
#else
    "Without \"Optimized\" instructions",
#endif




//---------------------------------------------------------------------
// Fishtest:  log 'featall.h' Research/Workaround build options

#if defined( OPTION_USE_SKAIP_AS_LOCK )         // Use SKAIP as lock, not RCP
    "With    OPTION_USE_SKAIP_AS_LOCK",
#endif
#if defined( OPTION_SIE2BK_FLD_COPY )           // SIE2BK 'fld' is NOT a mask
    "With    OPTION_SIE2BK_FLD_COPY",
#endif
#if defined( OPTION_IODELAY_KLUDGE )            // IODELAY kludge for Linux
    "With    OPTION_IODELAY_KLUDGE",
#endif
#if defined( OPTION_MVS_TELNET_WORKAROUND )     // Handle non-std MVS telnet
    "With    OPTION_MVS_TELNET_WORKAROUND",
#endif
#if defined( OPTION_SIE_PURGE_DAT_ALWAYS )      // Ivan 2016-07-30: purge DAT
    "With    OPTION_SIE_PURGE_DAT_ALWAYS",
#endif
#if defined( OPTION_NOASYNC_SF_CMDS )           // Bypass bug in cache logic
    "With    OPTION_NOASYNC_SF_CMDS",           // (see GitHub Issue #618!)
#endif

//---------------------------------------------------------------------






/*-------------------------------------------------------------------*/
/*                 Machine dependent assists:                        */
/*-------------------------------------------------------------------*/

    "Machine dependent assists:"

#if !defined( ASSIST_CMPXCHG1  ) \
 && !defined( ASSIST_CMPXCHG4  ) \
 && !defined( ASSIST_CMPXCHG8  ) \
 && !defined( ASSIST_CMPXCHG16 ) \
 && !defined( ASSIST_FETCH_DW  ) \
 && !defined( ASSIST_STORE_DW  ) \
 && CAN_IAF2 == IAF2_ATOMICS_UNAVAILABLE
    " (none)",
#else
  #if defined( ASSIST_CMPXCHG1 )
                    " cmpxchg1"
  #endif
  #if defined( ASSIST_CMPXCHG4 )
                    " cmpxchg4"
  #endif
  #if defined( ASSIST_CMPXCHG8 )
                    " cmpxchg8"
  #endif
  #if defined( ASSIST_CMPXCHG16 )
                    " cmpxchg16"
  #endif
  #if defined( ASSIST_FETCH_DW )
                    " fetch_dw"
  #endif
  #if defined( ASSIST_STORE_DW )
                    " store_dw"
  #endif
  #if     CAN_IAF2 != IAF2_ATOMICS_UNAVAILABLE
                    " hatomics"
    #if   CAN_IAF2 == IAF2_C11_STANDARD_ATOMICS
      "=C11"
    #elif CAN_IAF2 == IAF2_MICROSOFT_INTRINSICS
      "=msvcIntrinsics"
    #elif CAN_IAF2 == IAF2_ATOMIC_INTRINSICS
      "=atomicIntrinsics"
    #elif CAN_IAF2 == IAF2_SYNC_BUILTINS
      "=syncBuiltins"
    #else
      "=UNKNOWN"
    #endif
  #endif
    ,
#endif
};

/*-------------------------------------------------------------------*/
/* Retrieve ptr to build information strings array...                */
/*         (returns #of entries in array)                            */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  get_buildinfo_strings(const char*** pppszBldInfoStr)
{
    if (!pppszBldInfoStr) return 0;
    *pppszBldInfoStr = build_info;
    return ( sizeof(build_info) / sizeof(build_info[0]) );
}

/*-------------------------------------------------------------------*/
/*                     (helper function)                             */
/*-------------------------------------------------------------------*/
static void display_str( FILE* f, int httpfd, const char* str )
{
    if (f != stdout)
        if (httpfd)
            hprintf( httpfd, "%s\n", str );
        else
            fprintf( f, "%s\n", str );
    else
        LOGMSG( "%s\n", str );
}

/*-------------------------------------------------------------------*/
/*              Display "prog" version information                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT void display_version( FILE* f, int httpfd, const char* prog )
{
    const char** p = sysblk.vers_info;

    /* If external gui being used, set stdout & stderr streams
       to unbuffered so we don't have to flush them all the time
       in order to ensure consistent sequence of log messages.
    */
    if (extgui)
    {
        setvbuf( stderr, NULL, _IONBF, 0 );
        setvbuf( stdout, NULL, _IONBF, 0 );
    }

    if (prog)  // called from e.g. "cgibin_debug_version_info()"?
    {
        char buf[256];
        MSGBUF( buf, MSG( HHC01413, "I", prog, VERSION ));
        display_str( f, httpfd, RTRIM( buf ));
        ++p; // (skip past first str)
    }

    for (; *p; ++p)
        display_str( f, httpfd, *p );
}

/*-------------------------------------------------------------------*/
/*                   Display build options                           */
/*-------------------------------------------------------------------*/
DLL_EXPORT void display_build_options( FILE* f, int httpfd )
{
    const char** p;
    for (p = sysblk.bld_opts; *p; ++p)
        display_str( f, httpfd, *p );
}

/*-------------------------------------------------------------------*/
/*              Display External Package versions                    */
/*-------------------------------------------------------------------*/

#include "crypto_version.h"
#include "decnumber_version.h"
#include "softfloat_version.h"
#include "telnet_version.h"

DLL_EXPORT void display_extpkg_vers( FILE* f, int httpfd )
{
    const char** p;
    for (p = sysblk.extpkg_vers; *p; ++p)
        display_str( f, httpfd, *p );
}

/*-------------------------------------------------------------------*/
/*             Initialize SYSBLK info strings arrays                 */
/*-------------------------------------------------------------------*/

static void init_hercver_strings( const char* prog );   // (fwd ref)
static void init_bldopts_strings();                     // (fwd ref)
static void init_extpkgs_strings();                     // (fwd ref)

/*-------------------------------------------------------------------*/

DLL_EXPORT void init_sysblk_version_str_arrays( const char* prog )
{
    init_hercver_strings( prog );
    init_bldopts_strings();
    init_extpkgs_strings();
}

/*-------------------------------------------------------------------*/

#define APPEND_STR( ptr ) \
            append_ptr_to_array( (int*) &count, (void***) array, (void*) ptr );

static void append_ptr_to_array(  int*   count,  void***  array,  void*  ptr )
{
    *array = realloc( *array, ((*count) + 1) * sizeof( void* ));
    (*array)[ *count ] = ptr;
    ++*count;
}

/*-------------------------------------------------------------------*/

static void init_hercver_strings( const char* prog )
{
    int count = 0; const char*** array = &sysblk.vers_info;
    char buf[256]; if (*array) return; // (already built)

    // prog = Utility (HHC02499), NULL = Hercules (HHC01413).
    if (prog) MSGBUF( buf, MSG( HHC02499, "I",   prog,     VERSION ));
    else      MSGBUF( buf, MSG( HHC01413, "I", "Hercules", VERSION ));

    APPEND_STR( strdup( RTRIM( buf )));

    MSGBUF( buf, MSG( HHC01414, "I", HERCULES_COPYRIGHT ));
    APPEND_STR( strdup( RTRIM( buf )));

#if defined( CUSTOM_BUILD_STRING )
    MSGBUF( buf, MSG( HHC01417, "I", CUSTOM_BUILD_STRING ));
    APPEND_STR( strdup( RTRIM( buf )));
#endif

    MSGBUF( buf, MSG( HHC01415, "I", __DATE__, __TIME__ ));
    APPEND_STR( strdup( RTRIM( buf )));

    APPEND_STR( NULL );
}

/*-------------------------------------------------------------------*/

static void init_bldopts_strings()
{
    int count = 0;
    const char*** array;
    array = &sysblk.bld_opts;
    if (*array) return; /* already built */

    {
        unsigned int num_strs;
        const char** ppszBldInfoStr = NULL;
        char wrkbuf[256];
        char buf[272];

        num_strs = get_buildinfo_strings( &ppszBldInfoStr );

        for (; num_strs; num_strs--, ppszBldInfoStr++ )
        {
            MSGBUF( buf, MSG( HHC01417, "I", *ppszBldInfoStr ));
            APPEND_STR( strdup( RTRIM( buf )));
        }

        init_hostinfo( &hostinfo );
        format_hostinfo( &hostinfo, wrkbuf, sizeof( wrkbuf ));

        MSGBUF( buf, MSG( HHC01417, "I", wrkbuf ));
        APPEND_STR( strdup( RTRIM( buf )));
    }

    APPEND_STR( NULL );
}

/*-------------------------------------------------------------------*/

static void init_extpkgs_strings()
{
    int count = 0;
    const char*** array;
    array = &sysblk.extpkg_vers;
    if (*array) return; /* already built */

    {
        char pkgbuf[256];
        char buf[272];  /* to contain pkgbuf + message header and being conservative */

        MSGBUF( pkgbuf, "Built with %s external package version %s", "crypto", crypto_version());
        MSGBUF( buf, MSG( HHC01417, "I", pkgbuf ));
        APPEND_STR( strdup( RTRIM( buf )));

        MSGBUF( pkgbuf, "Built with %s external package version %s", "decNumber", decnumber_version());
        MSGBUF( buf, MSG( HHC01417, "I", pkgbuf ));
        APPEND_STR( strdup( RTRIM( buf )));

        MSGBUF( pkgbuf, "Built with %s external package version %s", "SoftFloat", softfloat_version());
        MSGBUF( buf, MSG( HHC01417, "I", pkgbuf ));
        APPEND_STR( strdup( RTRIM( buf )));

        MSGBUF( pkgbuf, "Built with %s external package version %s", "telnet", telnet_version());
        MSGBUF( buf, MSG( HHC01417, "I", pkgbuf ));
        APPEND_STR( strdup( RTRIM( buf )));
    }

    APPEND_STR( NULL );
}

/*-------------------------------------------------------------------*/
