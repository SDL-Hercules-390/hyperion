/* ARCHLVL.C    (C) Copyright Jan Jaeger,   2010-2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2018-2019     */
/*                  Architecture Level functions                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _ARCHLVL_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"             // Need DEF_INST

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined(   _GEN_ARCH )

  // _ARCH_NUM_0 has been built.
  // Now build the other architectures...

  #if defined(                _ARCH_NUM_1 )
    #define     _GEN_ARCH     _ARCH_NUM_1
    #include    "archlvl.c"
  #endif

  #if defined(                _ARCH_NUM_2 )
    #undef      _GEN_ARCH
    #define     _GEN_ARCH     _ARCH_NUM_2
    #include    "archlvl.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                ACTUAL Architecture bit-masks                      */
/*-------------------------------------------------------------------*/

#undef  NONE
#undef  S370
#undef  E390
#undef  Z900
#undef  Z390
#undef  MALL

#define NONE     0x00               /* NO architectures or disabled  */
#define S370     0x80               /* S/370 architecture            */
#define E390     0x40               /* ESA/390 architecture          */
#define Z900     0x20               /* z/Arch architecture           */

#define Z390     (E390|Z900)        /* BOTH ESA/390 and z/Arch       */
#define MALL     (S370|Z390)        /* All architectures             */

/*-------------------------------------------------------------------*/
/*                  Architecture Level Table                         */
/*-------------------------------------------------------------------*/

static ARCHTAB archtab[] =
{
    //---------------------------------------------------------
    //
    //  The "_ARCH_nnn_NAME" entries MUST come first. The
    //  'get_archtab_by_arch' function expects it.
    //
    //  The FIRST entry is our DEFAULT initial startup archi-
    //  tecture set by the 'init_default_archmode' function.
    //
    //---------------------------------------------------------

#if defined( _900 )
    AT( _ARCH_900_NAME,  ARCH_900_IDX,  Z900 )  //  "z/Arch"  feat900.h
#endif
#if defined( _390 )
    AT( _ARCH_390_NAME,  ARCH_390_IDX,  E390 )  //  "ESA/390" feat390.h
#endif
#if defined( _370 )
    AT( _ARCH_370_NAME,  ARCH_370_IDX,  S370 )  //  "S/370"   feat370.h
#endif

    // Other supported names come afterwards, in any order.

#if defined( _370 )
    AT( "S370",          ARCH_370_IDX,  S370 )
    AT( "370",           ARCH_370_IDX,  S370 )
#endif
#if defined( _390 )
    AT( "ESA390",        ARCH_390_IDX,  E390 )
    AT( "E390",          ARCH_390_IDX,  E390 )
    AT( "E/390",         ARCH_390_IDX,  E390 )
    AT( "ESA",           ARCH_390_IDX,  E390 )
    AT( "S/390",         ARCH_390_IDX,  E390 )
    AT( "OS/390",        ARCH_390_IDX,  E390 )
    AT( "S390",          ARCH_390_IDX,  E390 )
    AT( "OS390",         ARCH_390_IDX,  E390 )
    AT( "390",           ARCH_390_IDX,  E390 )
#endif
#if defined( _900 )
    AT( "zArch",         ARCH_900_IDX,  Z900 )
    AT( "z",             ARCH_900_IDX,  Z900 )
    AT( "900",           ARCH_900_IDX,  Z900 )
    AT( "Z900",          ARCH_900_IDX,  Z900 )
    AT( "z/900",         ARCH_900_IDX,  Z900 )
    AT( "ESAME",         ARCH_900_IDX,  Z900 )
    AT( "ESA/ME",        ARCH_900_IDX,  Z900 )
#endif
};

/*-------------------------------------------------------------------*/
/*                   init_default_archmode                  (public) */
/*-------------------------------------------------------------------*/
void init_default_archmode()
{
    // Called ONCE by bldcfg.c 'build_config' during system startup.

    sysblk.arch_mode = archtab[0].num;  // (always the first entry)
}

/*-------------------------------------------------------------------*/
/*                    get_archtab_by_arch                            */
/*-------------------------------------------------------------------*/
const ARCHTAB* get_archtab_by_arch( int archnum )
{
    // An "Architecture mode" (archmode, arch_mode) is the same
    // thing as an "Architecture number" (archnum, arch_num),
    // which is simply an index value determined at build time
    // depending on which architectures that Hercules was built
    // with support for (ARCH_nnn_IDX #defined by featchk.h) and
    // is used to index into architecture dependent tables such as
    // the sysblk.facility_list and the opcode tables in opcode.c

    size_t i;
    for (i=0; i < _countof( archtab ); i++)
        if (archtab[i].num == archnum)
            return &archtab[i];
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                    get_archtab_by_name                            */
/*-------------------------------------------------------------------*/
const ARCHTAB* get_archtab_by_name( const char* name )
{
    size_t i;
    for (i=0; i < _countof( archtab ); i++)
        if (strcasecmp( archtab[i].name, name ) == 0)
            return &archtab[i];
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                   get_arch_name_by_arch                           */
/*-------------------------------------------------------------------*/
const char* get_arch_name_by_arch( int archnum )
{
    const ARCHTAB* at = get_archtab_by_arch( archnum );
    return at ? at->name : NULL;
}

/*-------------------------------------------------------------------*/
/*                      get_arch_name                       (public) */
/*-------------------------------------------------------------------*/
const char* get_arch_name( REGS* regs )
{
    int archnum = regs ? regs->arch_mode : sysblk.arch_mode;
    return get_arch_name_by_arch( archnum );
}

/*-------------------------------------------------------------------*/
/*                     get_archmode_by_name                          */
/*-------------------------------------------------------------------*/
static int get_archmode_by_name( const char* archname )
{
    const ARCHTAB* at = get_archtab_by_name( archname );
    return at ? at->num : -1;
}

/*-------------------------------------------------------------------*/
/*                        archlvl_cmd                       (public) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* ARCHLVL S/370 | ESA/390 | z/ARCH                                  */
/*                                                                   */
/*-------------------------------------------------------------------*/
int archlvl_cmd( int argc, char* argv[], char* cmdline )
{
    U64   old_mainsize  = sysblk.mainsize;
    U64   new_mainsize  = sysblk.mainsize;
    int   old_arch_mode = sysblk.arch_mode;
    int   new_arch_mode = sysblk.arch_mode;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /*-----------------------------------------------------*/
    /*             Display Architecture Mode?              */
    /*-----------------------------------------------------*/

    if (argc < 2)
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", "ARCHLVL", get_arch_name( NULL ));
        return 0;
    }

    /* Too many arguments? */
    if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /*-----------------------------------------------------*/
    /*                Set Architecture                     */
    /*-----------------------------------------------------*/

    /* Make sure all CPUs are deconfigured or stopped */
    if (!are_all_cpus_stopped())
    {
        // "All CPU's must be stopped %s"
        WRMSG( HHC02253, "E", "to switch architectures" );
        return HERRCPUONL;
    }

    /* Determine new architecture mode */
    if ((new_arch_mode = get_archmode_by_name( argv[1] )) < 0)
    {
        // "ARCHLVL '%s' is invalid"
        WRMSG( HHC00895, "E", argv[1] );
        return -1;
    }

    /* Nothing for us to do unless the architecture changed */
    if (new_arch_mode != old_arch_mode )
    {
        /* Re-configure storage if MAINSIZE needs to be adjusted */
        if ((new_mainsize = adjust_mainsize( new_arch_mode, sysblk.mainsize )) != old_mainsize)
            configure_storage( new_mainsize >> SHIFT_4K );

        /* Switch to the new architecture mode as requested */
        OBTAIN_INTLOCK( NULL );
        {
            /* Perform system reset to switch architecture mode */
            static const bool clear = false, ipl = false;
            system_reset( new_arch_mode, clear, ipl, sysblk.pcpu );

            /* (ensure dummyregs matches new architecture) */
            sysblk.dummyregs.arch_mode = sysblk.arch_mode;
        }
        RELEASE_INTLOCK( NULL );
    }

    /* ALWAYS do an "initial_cpu_reset()" for all processors */
    initial_cpu_reset_all();

    /* Display results */
    if (argc > 1 && MLVL( VERBOSE ))
    {
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", "ARCHLVL", get_arch_name_by_arch( sysblk.arch_mode ));

        if (new_mainsize != old_mainsize)
        {
            char memsize[64];
            bool increased = (new_mainsize > old_mainsize);
            fmt_memsize_KB( sysblk.mainsize >> SHIFT_KIBIBYTE, memsize, sizeof( memsize ));

            // "MAINSIZE %screased to %s architectural %simum"
            WRMSG( HHC17006, "W", increased ? "in" : "de", memsize,
                increased ? "min" : "max" );
        }
    }

    /* If S/370 archmode was just set, force LPARNUM to BASIC.
       Else if LPARNUM is BASIC, change it back to LPARNUM 1
       if archmode is z/Arch. Else (ARCH_390_IDX) leave it alone.
       The user can override this automatic LPARNUM switching
       via their own subsequent LPARNUM stmt/cmd if such auto-
       matic behavior is not desired.
    */
    if (1
        && ARCH_370_IDX == sysblk.arch_mode
        && om_basic != sysblk.operation_mode
    )
        panel_command( "LPARNUM BASIC" );
    else if (1
        && ARCH_900_IDX == sysblk.arch_mode
        && om_basic == sysblk.operation_mode
    )
    {
        panel_command( "LPARNUM 1" );
        panel_command( "CPUIDFMT 0" );
    }

    return 0;
}

#endif /* !defined( _GEN_ARCH ) */
