/* BLDCFG.C     (c) Copyright Roger Bowler, 1999-2012                */
/*              (c) Copyright Jan Jaeger, 1999-2012                  */
/*              (c) Copyright TurboHercules, SAS 2010-2011           */
/*              ESA/390 Configuration Builder                        */
/* Interpretive Execution - (c) Copyright Jan Jaeger, 1999-2012      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support - (c) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module builds the configuration tables for the Hercules      */
/* ESA/390 emulator.  It reads information about the processors      */
/* and I/O devices from a configuration file.  It allocates          */
/* main storage and expanded storage, initializes control blocks,    */
/* and creates detached threads to handle console attention          */
/* requests and to maintain the TOD clock and CPU timers.            */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      TOD clock offset contributed by Jay Maynard                  */
/*      Dynamic device attach/detach by Jan Jaeger                   */
/*      OSTAILOR parameter by Jay Maynard                            */
/*      PANRATE parameter by Reed H. Petty                           */
/*      CPUPRIO parameter by Jan Jaeger                              */
/*      HERCPRIO, TODPRIO, DEVPRIO parameters by Mark L. Gaubatz     */
/*      $(DEFSYM) symbol substitution support by Ivan Warren         */
/*      Patch for ${var=def} symbol substitution (hax #26),          */
/*          and INCLUDE <filename> support (modified hax #27),       */
/*          contributed by Enrico Sorichetti based on                */
/*          original patches by "Hackules"                           */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#ifndef _BLDCFG_C_
#define _BLDCFG_C_
#endif

#ifndef _HENGINE_DLL_
#define _HENGINE_DLL_
#endif

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "hostinfo.h"
#include "hdl.h"
#include "tapedev.h"

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_2)
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "bldcfg.c"
 #undef   _GEN_ARCH
#endif

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "bldcfg.c"
 #undef   _GEN_ARCH
#endif

/*-------------------------------------------------------------------*/
/* Function to build system configuration                            */
/*-------------------------------------------------------------------*/
int build_config (const char *hercules_cnf)
{
int     i;                              /* Array subscript           */
int     devtmax;                        /* Max number device threads */

    /*      From impl.c, using system defaults of:
     *
     * LPARNUM  1                       # LPAR 1 with LPAR ID 01
     * CPUIDFMT 0                       # CPU ID format 0
     * XPNDSIZE 0                       # Expanded storage size
     */

    sysblk.xpndsize = 0;

    /* Set sysblk.maxcpu to our preferred default value, if possible */
#if (PREF_DEF_MAXCPU <= MAX_CPU_ENGINES)
    sysblk.maxcpu = PREF_DEF_MAXCPU;
#else
    WARNING( "sysblk.maxcpu reduced from " QSTR( PREF_DEF_MAXCPU ) " to " QSTR( MAX_CPU_ENGINES ))
    sysblk.maxcpu = MAX_CPU_ENGINES;
#endif

#ifdef    _FEATURE_S370_S390_VECTOR_FACILITY
    sysblk.numvec = sysblk.maxcpu;
#else
    sysblk.numvec = 0;
#endif

    /* Default architecture is highest supported.  Thus
       z/Arch first (900), then S/390 second (390), etc.
    */
#if defined(                  _900 )
    VERIFY( set_archlvl( _ARCH_900_NAME ));
#elif defined(                _390 )
    VERIFY( set_archlvl( _ARCH_390_NAME ));
#else //                      _370
    VERIFY( set_archlvl( _ARCH_370_NAME ));
#endif

    devtmax  = MAX_DEVICE_THREADS;

    ptt_trace_init (0, 1);

    /* Set max number device threads */
    sysblk.devtmax = devtmax;
    sysblk.devtwait = sysblk.devtnbr =
    sysblk.devthwm  = sysblk.devtunavail = 0;

#if defined(OPTION_LPP_RESTRICT)
    /* Default the licence setting */
    losc_set(PGM_PRD_OS_RESTRICTED);
#endif

    /* Reset the clock steering registers */
    csr_reset();

    /* Default CPU type CP */
    for (i = 0; i < sysblk.maxcpu; i++)
        sysblk.ptyp[i] = SCCB_PTYP_CP;

    /* Default main storage to 2M with one CPU */
    configure_storage(2 << (SHIFT_MEBIBYTE - 12));
    configure_numcpu(1);

    if (hercules_cnf && (process_config(hercules_cnf)))
        return -1;

    /* Connect each channel set to its home cpu */
    for (i = 0; i < sysblk.maxcpu; i++)
        if (IS_CPU_ONLINE(i))
            sysblk.regs[i]->chanset = i < FEATURE_LCSS_MAX ? i : 0xFFFF;

    return 0;
} /* end function build_config */

/*-------------------------------------------------------------------*/
/* Function to initialize the default network device                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* init_sysblk_netdev()
{
    if (!sysblk.netdev)
    {
        /* Initialize default NETDEV */

#if defined( __APPLE__ ) || defined( __FreeBSD__ )
        sysblk.netdev = strdup( "/dev/tun" );
#elif !defined( OPTION_W32_CTCI )
        sysblk.netdev = strdup( "/dev/net/tun" );
#else
        sysblk.netdev = strdup( tt32_get_default_iface() );
#endif
    }
    return sysblk.netdev;
}

#endif /*!defined(_GEN_ARCH)*/
