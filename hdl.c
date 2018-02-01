/* HDL.C        (C) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HDL_C_
#define _HUTIL_DLL_

#include "hercules.h"
#include "opcode.h"

/*-------------------------------------------------------------------*/
/*    List of dynamic modules to pre-load at hdl_main startup        */
/*-------------------------------------------------------------------*/

HDLPRE hdl_preload[] =
{
    { "hdteq",              HDL_LOAD_NOMSG                     },
    { "dyncrypt",           HDL_LOAD_NOMSG                     },

    //                      (examples...)
#if 0
    { "dyn_test1",          HDL_LOAD_DEFAULT                   },
    { "/foo/dyn_test2",     HDL_LOAD_NOMSG                     },
    { "../bar/dyn_test3",   HDL_LOAD_NOMSG | HDL_LOAD_NOUNLOAD },
#endif
    { NULL,                 0                                  },
};

/*-------------------------------------------------------------------*/
/*                     Global variables                              */
/*-------------------------------------------------------------------*/
static LOCK         hdl_lock;               /* loader lock           */
static HDLMOD*      hdl_mods     = NULL;    /* modules chain         */
static HDLMOD*      hdl_curmod   = NULL;    /* module being loaded   */
static HDLDEP*      hdl_depend   = NULL;    /* struct version/size   */
static const char*  hdl_modpath  = NULL;    /* modules load path     */
static bool         hdl_arg_p    = false;   /* -p cmdline opt given  */
static HDLSHUT*     hdl_shutlist = NULL;    /* Shutdown call list    */
static bool         hdl_shutting = false;   /* shutdown in progesss  */

/*-------------------------------------------------------------------*/
/*             Pointers pass to hdl_main by impl.c                   */
/*-------------------------------------------------------------------*/
static PANDISP*    hdl_real_pandisp;        /* real panel_display    */
static PANCMD*     hdl_real_pancmd;         /* real panel_command    */
static REPOPCODE*  hdl_real_repopcode;      /* real replace_opcode   */
static DEVHND*     hdl_real_ckd_DEVHND;     /* CKD devices DEVHND    */
static DEVHND*     hdl_real_fba_DEVHND;     /* FBA devices DEVHND    */

/*-------------------------------------------------------------------*/
/*         Internal entry-point functions forward references         */
/*-------------------------------------------------------------------*/
static void  hdl_register_symbols_ep ( REGSYM* regsym );
static void  hdl_resolve_symbols_ep  ( GETSYM* getsym );
static void  hdl_define_devtypes_ep  ( DEFDEV* defdev );

/*-------------------------------------------------------------------*/
/*           HDL user callback functions forward references          */
/*-------------------------------------------------------------------*/
static int   hdl_check_depends_cb    ( const char* name, const char* version, int size );
static void  hdl_register_symbols_cb ( const char* name, void* symbol );
static void* hdl_resolve_symbols_cb  ( const char* name );
static void  hdl_define_devtypes_cb  ( const char* typname, DEVHND* devhnd );
static void  hdl_define_instructs_cb ( U32 hdl_arch, int opcode, const char* name, void* func );
static void  hdl_replace_opcode      ( bool insert, HDLINS* );
static void  hdl_term                ( void* arg );

/*-------------------------------------------------------------------*/
/*          hdl_main  --  initialize Hercules Dynamic Loader         */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_main
(
    PANDISP*    real_pandisp,               /* real panel_display    */
    PANCMD*     real_pancmd,                /* real panel_command    */
    REPOPCODE*  real_repopcode,             /* real replace_opcode   */
    DEVHND*     real_ckd_DEVHND,            /* CKD devices DEVHND    */
    DEVHND*     real_fba_DEVHND             /* FBA devices DEVHND    */
)
{
    /* Called ONCE by impl.c during Hercules startup */

    HDLPRE*  preload;
    initialize_lock( &hdl_lock );
    hdl_shutting = false;
    dlinit();

    /*
    **  Save the pointers that were passed to us by impl.c.
    **  Our internal entry-point functions need these pointers.
    */
    hdl_real_pandisp    = real_pandisp;     /*  real panel_display   */
    hdl_real_pancmd     = real_pancmd;      /*  real panel_command   */
    hdl_real_repopcode  = real_repopcode;   /*  real replace_opcode  */
    hdl_real_ckd_DEVHND = real_ckd_DEVHND;  /*  CKD devices DEVHND   */
    hdl_real_fba_DEVHND = real_fba_DEVHND ; /*  FBA devices DEVHND   */

    /*
    **  Manually create the main Hercules executable HDLMOD entry
    **  and insert it as the first entry in our HDL modules chain.
    **  This eliminates the issue of needing to do dlopen( self ).
    */
    if (!(hdl_curmod = malloc( sizeof( HDLMOD ))))
    {
        char buf[64];
        MSGBUF( buf,  "malloc(%d)", (int) sizeof( HDLMOD ));
        // "HDL: error in function %s: %s"
        WRMSG( HHC01511, "S", buf, strerror( errno ));
        return -1;
    }

    hdl_curmod->name       =  strdup( "*Hercules" );
    hdl_curmod->flags      =  (HDL_LOAD_MAIN | HDL_LOAD_NOUNLOAD);

    hdl_curmod->depsec_ep  =  NULL;
    hdl_curmod->regsec_ep  =  hdl_register_symbols_ep;
    hdl_curmod->ressec_ep  =  hdl_resolve_symbols_ep;
    hdl_curmod->devsec_ep  =  hdl_define_devtypes_ep;
    hdl_curmod->inssec_ep  =  NULL;
    hdl_curmod->finsec_ep  =  NULL;

    hdl_curmod->symbols    =  NULL;
    hdl_curmod->devices    =  NULL;
    hdl_curmod->instructs  =  NULL;
    hdl_curmod->next       =  hdl_mods;
    hdl_mods               =  hdl_curmod;

    /*
    **  Create an initial symbols list by manualy registering
    **  and resolving all symbols needed for proper HDL support.
    */
    obtain_lock( &hdl_lock );
    {
        hdl_register_symbols_ep ( &hdl_register_symbols_cb );
        hdl_resolve_symbols_ep  ( &hdl_resolve_symbols_cb  );
        hdl_define_devtypes_ep  ( &hdl_define_devtypes_cb  );
    }
    release_lock( &hdl_lock );

    /* Register our termination routine */
    hdl_addshut( "hdl_term", hdl_term, NULL );

    /* Pre-load all HDL modules that must be loaded at startup */
    for (preload = hdl_preload; preload->name; preload++)
        hdl_loadmod( preload->name, preload->flag );

    return 0;
}

/*-------------------------------------------------------------------*/
/*           hdl_dlopen  --  ask host to open dynamic library        */
/*-------------------------------------------------------------------*/
static void* hdl_dlopen( const char* filename, int flag )
{
    char*   fullname;
    void*   ret;
    size_t  fulllen = 0;

#if defined( HDL_USE_LIBTOOL )
    UNREFERENCED( flag );   /* stupid libtool doesn't support flags! */
#endif

    /*
     *  Check in this order:
     *
     *   1.  filename as passed
     *   2.  filename with extension if needed
     *   3.  modpath added if basename( filename )
     *   4.  extension added to #3
     */
    if ((ret = dlopen( filename, flag ))) /* try filename as-is first */
        return ret;

     //  2.  filename with extension if needed

    fulllen = strlen( hdl_modpath ) + 1 + strlen( filename ) + HDL_SUFFIX_LENGTH + 1;
    fullname = calloc( 1, fulllen );

    if (!fullname)
        return NULL;

#if defined( HDL_MODULE_SUFFIX )

    strlcpy( fullname, filename,          fulllen );
    strlcat( fullname, HDL_MODULE_SUFFIX, fulllen );

    if ((ret = dlopen( fullname, flag )))   /* try filename with suffix next */
    {
        free( fullname );
        return ret;
    }
#endif

     //  3.  modpath added if basename( filename )

    if (hdl_modpath && *hdl_modpath)
    {
        char* filenamecopy = strdup( filename );

        strlcpy( fullname, hdl_modpath,              fulllen );
        strlcat( fullname, PATHSEPS,                 fulllen );
        strlcat( fullname, basename( filenamecopy ), fulllen );

        free( filenamecopy );
    }
    else
        strlcpy( fullname, filename, fulllen );

    if ((ret = dlopen( fullname, flag )))
    {
        free( fullname );
        return ret;
    }

    //  4.  extension added to #3

#if defined( HDL_MODULE_SUFFIX )

    strlcat( fullname, HDL_MODULE_SUFFIX, fulllen );

    if ((ret = dlopen( fullname, flag )))
    {
        free( fullname );
        return ret;
    }
#endif

    return NULL;
}

/*-------------------------------------------------------------------*/
/*           hdl_loadmod  --  load a hercules module                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_loadmod( const char* name, int flags )
{
    /* Called by hsccmd.c's "ldmod_cmd" function */

    const char*  modname;
    HDLMOD*      mod;
    HDLMOD*      wrkmod;
    HDLSYM*      sym;

    /* Search module chain to see if module is already loaded */
    modname = (modname = strrchr( name, '/' )) ? modname+1 : name;

    for (mod = hdl_mods; mod; mod = mod->next)
    {
        if (strfilenamecmp( modname, mod->name ) == 0)
        {
            // "HDL: module %s already loaded"
            WRMSG( HHC01519, "E", mod->name );
            return -1;
        }
    }

    if (!(mod = malloc( sizeof( HDLMOD ))))
    {
        // "HDL: error in function %s: %s"
        WRMSG( HHC01511, "E", "malloc()", strerror( errno ));
        return -1;
    }

    /* Load the module */
    mod->name = strdup( modname );

    if (!(mod->handle = hdl_dlopen( name, RTLD_NOW )))
    {
        if (!(flags & HDL_LOAD_NOMSG))
            // "HDL: unable to open module %s: %s"
            WRMSG( HHC01516, "E", name, dlerror());

        free( mod );
        return -1;
    }

    mod->flags = (flags & (~HDL_LOAD_WAS_FORCED));

    /* Retrieve the module's HDL_DEPENDENCY_SECTION entry-point */
    if (!(mod->depsec_ep = dlsym( mod->handle, "hdl_check_depends_ep" )))
    {
        // "HDL: no HDL_DEPENDENCY_SECTION in %s: %s"
        WRMSG( HHC01517, "E", mod->name, dlerror());
        dlclose( mod->handle );
        free( mod );
        return -1;
    }

    /* Reject loading the same module again twice */
    for (wrkmod = hdl_mods; wrkmod; wrkmod = wrkmod->next)
    {
        if (wrkmod->depsec_ep == mod->depsec_ep)
        {
            // "HDL: module %s is duplicate of %s"
            WRMSG( HHC01520, "E", mod->name, wrkmod->name );
            dlclose( mod->handle );
            free( mod );
            return -1;
        }
    }

    /* Retrieve pointers to the module's entry-point functions */
    mod->regsec_ep = dlsym( mod->handle, "hdl_register_symbols_ep" );
    mod->ressec_ep = dlsym( mod->handle, "hdl_resolve_symbols_ep"  );
    mod->devsec_ep = dlsym( mod->handle, "hdl_define_devtypes_ep"  );
    mod->inssec_ep = dlsym( mod->handle, "hdl_define_instructs_ep" );
    mod->finsec_ep = dlsym( mod->handle, "hdl_on_module_unload_ep" );

    /* No modules or device types registered yet */
    mod->symbols   = NULL;
    mod->devices   = NULL;
    mod->instructs = NULL;

    obtain_lock( &hdl_lock );
    {
        /* Call module's HDL_DEPENDENCY_SECTION */
        if (mod->depsec_ep( &hdl_check_depends_cb ) != 0)
        {
            const char* sev = (flags & HDL_LOAD_FORCE) ? "W" : "E";

            // "HDL: dependency check failed for module %s"
            WRMSG( HHC01518, sev, mod->name );

            /* Allow module to be loaded anyway if forced */
            if (flags & HDL_LOAD_FORCE)
            {
                /* Remember module was forcibly loaded */
                mod->flags |= HDL_LOAD_WAS_FORCED;
            }
            else /* Abort the module load */
            {
                dlclose( mod->handle );
                free( mod );
                release_lock( &hdl_lock );
                return -1;
            }
        }

        /* Update the current module pointer for the benefit of the
           various module callback functions so they can access and
           update the current module's HDLMOD structure.
        */
        hdl_curmod = mod;

        /* Call the module's HDL_REGISTER_SECTION, if it has one */
        if (mod->regsec_ep)
            mod->regsec_ep( &hdl_register_symbols_cb );

        /* Insert current module at the head of the modules chain */
        mod->next = hdl_mods;
        hdl_mods = mod;

        /* Reset symbol loadcounts before re-resolving symbols */
        for (wrkmod = hdl_mods; wrkmod; wrkmod = wrkmod->next)
            for (sym = wrkmod->symbols; sym; sym = sym->next)
                sym->refcnt = 0;

        /* Call HDL_RESOLVER_SECTION again for every loaded module */
        for (wrkmod = hdl_mods; wrkmod; wrkmod = wrkmod->next)
        {
            if (wrkmod->ressec_ep)
                wrkmod->ressec_ep( &hdl_resolve_symbols_cb );
        }

        /* Call the module's HDL_DEVICE_SECTION, if it has one */
        if (mod->devsec_ep)
            mod->devsec_ep( &hdl_define_devtypes_cb );

        /* Call the module's HDL_INSTRUCTION_SECTION, if it has one */
        if (mod->inssec_ep)
            mod->inssec_ep( &hdl_define_instructs_cb );
    }
    release_lock( &hdl_lock );

    return 0;
}

/*-------------------------------------------------------------------*/
/*             hdl_freemod  --  unload a hercules module             */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_freemod( const char* name )
{
    /* Called by hsccmd.c's "rmmod_cmd" function */

    const char*  modname;
    DEVBLK*      dev;

    HDLMOD*      mod;
    HDLMOD**     ppmod;

    HDLSYM*      sym;
    HDLSYM*      cursym;

    HDLDEV*      device;
    HDLDEV*      savedev;

    HDLINS*      ins;
    HDLINS*      saveins;

    modname = (modname = strrchr( name, '/' )) ? modname+1 : name;

    obtain_lock( &hdl_lock );
    {
        /* Locate this module's entry in our chain */
        for (ppmod = &(hdl_mods);
            *ppmod && strfilenamecmp( modname, (*ppmod)->name ) != 0;
             ppmod = &((*ppmod)->next))
        {
            ; // (nop)
        }

        /* Was module found? */
        if (!*ppmod)
        {
            release_lock( &hdl_lock );
            // "HDL: module %s not found"
            WRMSG( HHC01524, "E", modname );
            return -1;
        }

        /* Error if it 's not allowed to be unloaded */
        if ((*ppmod)->flags & (HDL_LOAD_MAIN | HDL_LOAD_NOUNLOAD))
        {
            release_lock( &hdl_lock );
            // "HDL: unloading of module %s not allowed"
            WRMSG( HHC01521, "E", (*ppmod)->name );
            return -1;
        }

        /* Disallow device modules to be unloaded if a device
           is still using it by searching the device chain to
           see if any of their DEVHND device handler pointers
           matches the one for this module.
        */
        for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
        {
            /* Is device in use? */
            if (IS_DEV( dev ))
            {
                /* Search DEVHND chain for this module to see
                   if any of them match this device's. If so,
                   then the current device is still using the
                   module being unloaded (the device is still
                   "bound" to the module being unloaded).
                */
                for (device = (*ppmod)->devices; device; device = device->next)
                {
                    /* Is device still using this module? */
                    if (dev->hnd == device->hnd)
                    {
                        release_lock( &hdl_lock );
                        // "HDL: module %s bound to device %1d:%04X"
                        WRMSG( HHC01522, "E", (*ppmod)->name,
                            SSID_TO_LCSS( dev->ssid ), dev->devnum );
                        return -1;
                    }
                }
            }
        }

        /* Call module's HDL_FINAL_SECTION, if it has one */
        if ((*ppmod)->finsec_ep)
        {
            int rc;

            if ((rc = ((*ppmod)->finsec_ep)()))
            {
                release_lock( &hdl_lock );
                // "HDL: unload of module %s rejected by final section"
                WRMSG( HHC01523, "E", (*ppmod)->name );
                return rc;
            }
        }

        sym = (*ppmod)->symbols;

        /* Free all symbols that this module registered */
        while (sym)
        {
            cursym = sym;

            /* Go on to the next entry (if there is one) */
            sym = sym->next;

            /* free the symbol */
            free( cursym->name );
            free( cursym );
        }

        /* Remove the module from our chain */
        mod  = *ppmod;
        *ppmod = (*ppmod)->next;

        /* Free all device registrations */
        for (device = mod->devices; device;)
        {
            free( device->name );
            savedev = device->next;
            free( device );
            device = savedev;
        }

        /* Revert and free all instruction overrides for this module */
        for (ins = mod->instructs; ins;)
        {
            /* Revert this instruction override */
            hdl_replace_opcode( false, ins );

            /* Free resources for this instruction override */
            free( ins->instname );  // (free name string)
            saveins = ins->next;    // (save before freeing struct)
            free( ins );            // (free HDLINS struct)

            /* Do for all overrides in this module's chain */
            ins = saveins;
        }

        /* Free the module's HDLMOD struct */
        free( mod->name );
        dlclose( mod->handle );
        free( mod );

        /* Reset symbol loadcounts for all remaining modules */
        for (mod = hdl_mods; mod; mod = mod->next)
            for (cursym = mod->symbols; cursym; cursym = cursym->next)
                cursym->refcnt = 0;

        /* Call all remaining modules' HDL_RESOLVER_SECTION again */
        for (mod = hdl_mods; mod; mod = mod->next)
        {
            if (mod->ressec_ep)
                mod->ressec_ep( &hdl_resolve_symbols_cb );
        }
    }
    release_lock( &hdl_lock );

    return 0;
}

/*-------------------------------------------------------------------*/
/*              hdl_addshut - add shutdown call                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_addshut( const char* shutname, SHUTDN* shutfunc, void* shutarg )
{
    HDLSHUT*   shut;
    HDLSHUT**  pshut;

    if (hdl_shutting)
        return;

    /* avoid duplicates - keep the first one */
    for (pshut = &(hdl_shutlist); *pshut; pshut = &((*pshut)->next) )
    {
        if (1
            && (*pshut)->shutfunc == shutfunc
            && (*pshut)->shutarg  == shutarg
        )
            return;
    }

    /* Add new entry to head of list (LIFO) */

    shut = malloc( sizeof( HDLSHUT ));

    shut->next      =  hdl_shutlist;
    shut->shutname  =  shutname;
    shut->shutfunc  =  shutfunc;
    shut->shutarg   =  shutarg;

    hdl_shutlist = shut;
}

/*-------------------------------------------------------------------*/
/*             hdl_delshut  --  remove shutdown call                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_delshut( SHUTDN* shutfunc, void* shutarg )
{
    HDLSHUT**  pshut;
    HDLSHUT*   shut;
    int        rc = -1;

    if (hdl_shutting)
        return rc;

    for (pshut = &(hdl_shutlist); *pshut; pshut = &((*pshut)->next) )
    {
        if (1
            && (*pshut)->shutfunc == shutfunc
            && (*pshut)->shutarg  == shutarg
        )
        {
            shut = *pshut;
            {
                *pshut = (*pshut)->next;
            }
            free( shut );

            rc = 0;
        }
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/*       hdl_atexit  --  call shutdown entries in LIFO order         */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_atexit( void )
{
    HDLSHUT*  shut;

    if (MLVL( DEBUG ))
        // "HDL: begin shutdown sequence"
        WRMSG( HHC01500, "I" );

    hdl_shutting = true;

    for (shut = hdl_shutlist; shut; shut = hdl_shutlist)
    {
        /* Remove shutdown call entry to ensure it is called once */
        hdl_shutlist = shut->next;
        {
            if (MLVL( DEBUG ))
                // "HDL: calling %s"
                WRMSG( HHC01501, "I", shut->shutname );

            shut->shutfunc( shut->shutarg );

            if (MLVL( DEBUG ))
                // "HDL: %s complete"
                WRMSG( HHC01502, "I", shut->shutname );
        }
        free( shut );
    }

    if (MLVL( DEBUG ))
        // "HDL: shutdown sequence complete"
        WRMSG( HHC01504, "I" );
}

/*-------------------------------------------------------------------*/
/*            hdl_checkpath  --  check module path                   */
/*-------------------------------------------------------------------*/
/* Returns: TRUE == path is valid, FALSE == path is invalid          */
/*-------------------------------------------------------------------*/
static BYTE hdl_checkpath( const char* path )
{
    // MAYBE issue HHC01536 warning message if directory is invalid

    char workpath[ MAX_PATH ];
    struct stat statbuf;
    int invalid;

#ifdef _MSVC_
    int len; char c;
#endif

    STRLCPY( workpath, path );

#ifdef _MSVC_

    // stat: If path contains the location of a directory,
    // it cannot contain a trailing backslash. If it does,
    // -1 will be returned and errno will be set to ENOENT.
    // Therefore we need to remove any trailing backslash.

    c = 0;
    len = strlen( workpath );

    // Trailing path separator?

    if (0
        || workpath[ len - 1 ] == '/'
        || workpath[ len - 1 ] == '\\'
    )
    {
        // Yes, remove it and remember that we did so.

        c = workpath[ len - 1 ];     // (remember)
        workpath[ len - 1 ] = 0;     // (remove it)
    }
#endif // _MSVC_

    // Is the directory valid? (does it exist?)

    invalid =
    (0
        || stat( workpath, &statbuf ) != 0
        || !S_ISDIR( statbuf.st_mode )
    );

#ifdef _MSVC_

    // (restore trailing path separator if removed)

    if (c)
        workpath[ len - 1 ] = c;
#endif

    if (invalid && MLVL( VERBOSE ))
    {
        // "HDL: WARNING: '%s' is not a valid directory"
        WRMSG( HHC01536, "W", path );
    }

    return !invalid ? TRUE : FALSE;     // (valid or invalid)
}

/*-------------------------------------------------------------------*/
/*            hdl_initpath  --  initialize module path               */
/*-------------------------------------------------------------------*/
/*
    1) -p from startup 
    2) HERCULES_LIB environment variable 
    3) MODULESDIR compile time define 
    4) Hercules executable directory
    5) "hercules"
*/
DLL_EXPORT void hdl_initpath( const char* path )
{
    free( hdl_modpath );    // (discard old value, if any)

    // 1) -p from startup

    if (path)
    {
        hdl_arg_p = true;  // (remember -p cmdline option specified)
        hdl_modpath = strdup( path );
    }
    else
    {
        // 2) HERCULES_LIB environment variable

        char* def;
        char pathname[ MAX_PATH ];

        if ((def = getenv("HERCULES_LIB")))
        {
            hostpath( pathname, def, sizeof( pathname ));
            hdl_modpath = strdup( pathname );
        }
        else
        {
            // 3) MODULESDIR compile time define

#if defined( MODULESDIR )
            hostpath( pathname, MODULESDIR, sizeof( pathname ));
            hdl_modpath = strdup( pathname );
#else
            // 4) Hercules executable directory

            if (sysblk.hercules_pgmpath && strlen( sysblk.hercules_pgmpath ))
            {
                hdl_modpath = strdup( sysblk.hercules_pgmpath );
            }
            else
            {
                // 5) "hercules"

                hostpath( pathname, "hercules", sizeof( pathname ));
                hdl_modpath = strdup( pathname );
            }
#endif // defined( MODULESDIR )
        }
    }

    // Check path and MAYBE issue HHC01536 warning if it's invalid

    hdl_checkpath( hdl_getpath() );

#if defined( ENABLE_BUILTIN_SYMBOLS )
    set_symbol( "MODPATH", hdl_getpath() );
#endif
}

/*-------------------------------------------------------------------*/
/*              hdl_getpath  --  return module path                  */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* hdl_getpath()
{
    if (!hdl_modpath)           // (if default not set yet)
        hdl_initpath( NULL );   // (then initilize default)
    return hdl_modpath;         // (return current value)
}

/*-------------------------------------------------------------------*/
/*           hdl_setpath  --  Set module load path                   */
/*-------------------------------------------------------------------*/
/* Return: -1 error (not set), +1 warning (not set), 0 okay (set)    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_setpath( const char* path )
{
    char    pathname[ MAX_PATH ];       /* pathname conversion       */
    char    abspath [ MAX_PATH ];       /* pathname conversion       */

    ASSERT( path );                     /* Sanity check              */

    // Reject paths that are too long

    if (strlen( path ) >= MAX_PATH)
    {
        // "HDL: directory '%s' rejected; exceeds maximum length of %d"
        WRMSG( HHC01505, "E", path, MAX_PATH );
        return -1;      // (error; not set)
    }

    // Ignore 'modpath_cmd()' when -p cmdline option is specified

    if (hdl_arg_p)
    {
        // "HDL: directory '%s' rejected; '-p' cmdline option rules"
        WRMSG( HHC01506, "W", path );

        // "HDL: directory remains '%s' from '-p' cmdline option"
        WRMSG( HHC01507, "W", hdl_modpath );
        return +1;      // (warning; not set)
    }

    // Convert path to host format

    hostpath( pathname, path, sizeof( pathname ));

    // Convert path to absolute path if it's relative

    abspath[0] = 0;

    if (1
        && '.' == pathname[0]               // (relative path?)
        && !realpath( pathname, abspath )   // (doesn't exist?)
    )
    {
        char buf[ MAX_PATH + 128 ];

        MSGBUF( buf,  "\"%s\": %s", pathname, strerror( errno ));

        // "HDL: error in function %s: %s"
        WRMSG( HHC01511, "W", "realpath()", buf );
        abspath[0] = 0;
    }

    // Use unresolved path if unable to resolve to an absolute path

    if (!abspath[0])
        STRLCPY( abspath, pathname );

    // Set the path as requested

    free( hdl_modpath );
    hdl_modpath = strdup( abspath );

    // Update the MODPATH symbol

#if defined( ENABLE_BUILTIN_SYMBOLS )
    set_symbol( "MODPATH", hdl_getpath() );
#endif

    // "HDL: loadable module directory is '%s'"
    WRMSG( HHC01508, "I", hdl_getpath() );

    // Check path and MAYBE issue HHC01536 warning if it's invalid

    hdl_checkpath( hdl_getpath() );

    return 0;   // (success; set)
}

/*-------------------------------------------------------------------*/
/*              hdl_listmods  --  list hercules modules              */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_listmods( int flags )
{
    /* Called by hsccmd.c "lsmod_cmd" */

    HDLMOD*  mod;
    HDLSYM*  sym;
    char     buf[ 256 ];
    int      len;

    for (mod = hdl_mods; mod; mod = mod->next)
    {
        // "HDL: name = %s, type = %s, flags = (%sunloadable, %sforced)"
        WRMSG( HHC01531, "I"
            ,  mod->name
            , (mod->flags & HDL_LOAD_MAIN)       ?  "EXE"    :
              (mod->flags & HDL_LOAD_PSEUDOMOD)  ?  "pseudo" :  "hdl"
            , (mod->flags & HDL_LOAD_NOUNLOAD)   ?  "NOT "   :   ""
            , (mod->flags & HDL_LOAD_WAS_FORCED) ?   ""      :  "not "
        );

        for (sym = mod->symbols; sym; sym = sym->next)
        {
            if (0
                || (flags & HDL_LIST_ALL)
                || !(1
                     && (mod->flags & HDL_LOAD_MAIN)
                     && !sym->symbol
                    )
            )
            {
                // "HDL:  symbol = %s, loadcount = %d%s, owner = %s"
                WRMSG( HHC01532, "I"
                    , sym->name
                    , sym->refcnt, sym->symbol ? "" : ", UNRESOLVED"
                    , mod->name
                );
            }
        }

        if (mod->devices)
        {
            HDLDEV*  device;
            len = 0;

            for (device = mod->devices; device; device = device->next)
                len += snprintf( buf + len, sizeof( buf ) - len, " %s", device->name );

            // (no blank before %s since string starts with it)
            // "HDL:  devtype(s) =%s"
            WRMSG( HHC01533, "I", buf );
        }

        if (mod->instructs)
        {
            HDLINS*  ins;

            for (ins = mod->instructs; ins; ins = ins->next)
            {
                len = 0;
#if defined(                                                                             _370 )
                if (ins->hdl_arch ==                                         HDL_INSTARCH_370)
                    len += snprintf( buf + len, sizeof( buf ) - len, ", archlvl = " _ARCH_370_NAME );
#endif

#if defined(                                                                             _390 )
                if (ins->hdl_arch ==                                         HDL_INSTARCH_390)
                    len += snprintf( buf + len, sizeof( buf ) - len, ", archlvl = " _ARCH_390_NAME );
#endif

#if defined(                                                                             _900 )
                if (ins->hdl_arch ==                                         HDL_INSTARCH_900)
                    len += snprintf( buf + len, sizeof( buf ) - len, ", archlvl = " _ARCH_900_NAME );
#endif
                // (no blank between %4.4X and %s since string starts with ", ")
                // "HDL:  instruction = %s, opcode = %4.4X%s"
                WRMSG( HHC01534, "I"
                    , ins->instname
                    , ins->opcode
                    , buf
                );
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*              hdl_listdeps  --  list module dependencies           */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_listdeps()
{
    /* Called by hsccmd.c "lsdep_cmd" */

    HDLDEP*  depent;

    for (depent = hdl_depend; depent; depent = depent->next)
        // "HDL: dependency %s version %s size %d"
        WRMSG( HHC01535, "I", depent->name, depent->version, depent->size );
}

/*-------------------------------------------------------------------*/
/*         hdl_build_devmod_name  --  build device module name       */
/*-------------------------------------------------------------------*/
static const char* hdl_build_devmod_name( const char* typname )
{
    char*   dtname;
    size_t  len, size;

    size = strlen( "hdt" ) + strlen( typname ) + 1;  // (+1 for NULL terminator)
    dtname = malloc( size );

    strlcpy( dtname, "hdt",   size );
    strlcat( dtname, typname, size );

    for (len = 0; len < strlen( dtname ); len++)
        dtname[ len ] = tolower( dtname[ len ]);

    return dtname;
}

/*-------------------------------------------------------------------*/
/*               ( hdl_DEVHND helper function )                       */
/*-------------------------------------------------------------------*/
static DEVHND* hdl_get_DEVHND( const char* typname )
{
    HDLMOD*  mod;
    HDLDEV*  device;

    /* Search device modules for the one handling this device-type */
    for (mod = hdl_mods; mod; mod = mod->next)
    {
        for (device = mod->devices; device; device = device->next)
        {
            if (!strcasecmp( typname, device->name ))
                return device->hnd;
        }
    }

    return NULL;
}

/*-------------------------------------------------------------------*/
/*     hdl_DEVHND  --  get device handler for given device-type      */
/*-------------------------------------------------------------------*/
DLL_EXPORT DEVHND* hdl_DEVHND( const char* typname )
{
    /* Called by config.c "attach_device" */

    DEVHND*      hnd;
    const char*  modname;

    /* Get the device handler for requested device-type */
    if ((hnd = hdl_get_DEVHND( typname )))
        return hnd;

    /* We couldn't find a handler for the requested device-type.
       It might be that the module that handles this device-type
       has not been loaded yet. Try loading it using the passed
       device-type and then try getting the device handler again.
    */
    modname = hdl_build_devmod_name( typname );

    if (0
        || hdl_loadmod( modname, HDL_LOAD_NOMSG ) != 0
        || !(hnd = hdl_get_DEVHND( typname ))
    )
    {
        /* The handler module for the requested device-type
           might have a different module name than the one
           for the requested device-type (i.e. the requested
           device-type might be equated to a another device-
           type). If there exists a "device equates" module,
           try calling its equate function to retrieve the
           equated device-type name.
        */
        if (hdl_devequ)
        {
            const char* equtyp = hdl_devequ( typname );

            if (equtyp)
            {
                /* Try loading the device handler module
                   for the equated device-type.
                */
                free( modname );
                modname = hdl_build_devmod_name( equtyp );
                hdl_loadmod( modname, HDL_LOAD_NOMSG );

                /* The module load of the device handler module
                   for the equated device-type either succeeded
                   or failed, but either way we don't care. If
                   it succeeded the "Last chance!" call further
                   below will succeed. Otherwise it will fail.
                */
            }
        }
    }

    free( modname );

    /* Last chance! */
    return hdl_get_DEVHND( typname );
}

/*-------------------------------------------------------------------*/
/*          hdl_term  --  process all "HDL_FINAL_SECTION"s           */
/*-------------------------------------------------------------------*/
static void hdl_term( void* arg )
{
    HDLMOD*  mod;

    UNREFERENCED( arg );

    if (MLVL( DEBUG ))
        // "HDL: begin termination sequence"
        WRMSG( HHC01512, "I" );

    /* Call all final routines, in LIFO order */
    for (mod = hdl_mods; mod; mod = mod->next)
    {
        if (mod->finsec_ep)
        {
            if (MLVL( DEBUG ))
                // "HDL: calling module cleanup routine %s"
                WRMSG( HHC01513, "I", mod->name );

            /* Call this module's HDL_FINAL_SECTION */
            mod->finsec_ep();

            if (MLVL( DEBUG ))
                // "HDL: module cleanup routine %s complete"
                WRMSG( HHC01514, "I", mod->name );
        }
    }

    if (MLVL( DEBUG ))
        // "HDL: termination sequence complete"
        WRMSG( HHC01515, "I" );
}

/*-------------------------------------------------------------------*/
/*            hdl_next  --  find next entry-point in chain           */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* hdl_next( const void* symbol )
{
    /* Called by various loadable modules to find the next handler */

    HDLMOD*      mod;
    HDLSYM*      sym  = NULL;
    const char*  name;

    /* Find the module that defined the specified symbol.
       (i.e. the first module in our LIFO chain (being the
       most recently loaded module) with a defined symbol
       whose address is identical to the one passed to us).
    */
    for (mod = hdl_mods; mod; mod = mod->next)
    {
        for (sym = mod->symbols; sym; sym = sym->next)
        {
            /* Compare symbol addresses: a match means this
               module was the one that defined this symbol.
            */
            if (sym->symbol == symbol)
                break;
        }

        /* Stop searching as soon as we find our symbol */
        if (sym)
            break;
    }

    /* Did we find our symbol? */
    if (sym)
    {
        /* Save the name of the symbol we're now looking for */
        name = sym->name;

        /* Does this module have a "next" link for this symbol?
           If so, then that's where we'll start our search for
           the next symbol registration. Otherwise we'll start
           our search with the NEXT module's symbols chain.
        */
        if (!(sym = sym->next))
        {
            /* No next symbol. Try the NEXT module's symbols */
            if (!(mod = mod->next))
                return NULL;        // (no "next" for symbol)

            sym = mod->symbols;     // (start searching here)
        }

        /* Find the "next entry" for our symbol by searching this
           module's (and all remaining modules') symbols for the
           first symbol that we find with the same name as ours.
        */
        for (; mod; mod = mod->next, sym = mod->symbols)
        {
            for (; sym; sym = sym->next)
            {
                if (strcmp( sym->name, name ) == 0)
                    return sym->symbol;
            }
        }
    }

    return NULL;
}

/*-------------------------------------------------------------------*/
/*               HDL INTERNAL ENTRY-POINT FUNCTIONS                  */
/*-------------------------------------------------------------------*/
/* The following functions are hard-coded equivalents of the hdl.h   */
/* HDL_REGISTER_SECTION, HDL_RESOLVER_SECTION and HDL_DEVICE_SECTION */
/* functions that allows hdl.c to initialize required and optional   */
/* symbol values (mostly in hsys.c = hsys.dll) that are needed for   */
/* HDL to function properly. The "hdl_main" initialization function  */
/* calls into these functions as part of its initialization logic.   */
/*                                                                   */
/* By doing things this way it eliminates the need for the previous  */
/* Hercules executable "hdlmain.c" source member and the subsequent  */
/* hdl_main "dlopen(self)" call which some systems do not support.   */
/*-------------------------------------------------------------------*/

static void**  UNRESOLVED  = NULL;

// HDL_REGISTER_SECTION
static void hdl_register_symbols_ep( REGSYM* regsym )
{
    HDL_REGISTER( panel_display,                 *hdl_real_pandisp   );
    HDL_REGISTER( panel_command,                 *hdl_real_pancmd    );
    HDL_REGISTER( replace_opcode,                *hdl_real_repopcode );

    HDL_REGISTER( daemon_task,                   *UNRESOLVED );
    HDL_REGISTER( system_command,                *UNRESOLVED );
    HDL_REGISTER( hdl_devequ,                    *UNRESOLVED );

    HDL_REGISTER( debug_cpu_state,               *UNRESOLVED );
    HDL_REGISTER( debug_cd_cmd,                  *UNRESOLVED );
    HDL_REGISTER( debug_program_interrupt,       *UNRESOLVED );
    HDL_REGISTER( debug_diagnose,                *UNRESOLVED );
    HDL_REGISTER( debug_sclp_unknown_command,    *UNRESOLVED );
    HDL_REGISTER( debug_sclp_unknown_event,      *UNRESOLVED );
    HDL_REGISTER( debug_sclp_unknown_event_mask, *UNRESOLVED );
    HDL_REGISTER( debug_sclp_event_data,         *UNRESOLVED );
    HDL_REGISTER( debug_chsc_unknown_request,    *UNRESOLVED );
    HDL_REGISTER( debug_watchdog_signal,         *UNRESOLVED );

#if defined( OPTION_W32_CTCI )
    HDL_REGISTER( debug_tt32_stats,              *UNRESOLVED );
    HDL_REGISTER( debug_tt32_tracing,            *UNRESOLVED );
#endif
}
// END_REGISTER_SECTION

/*-------------------------------------------------------------------*/

// HDL_RESOLVER_SECTION
static void hdl_resolve_symbols_ep( GETSYM* getsym )
{
    HDL_RESOLVE( panel_display                 );
    HDL_RESOLVE( panel_command                 );
    HDL_RESOLVE( replace_opcode                );

    HDL_RESOLVE( daemon_task                   );
    HDL_RESOLVE( system_command                );
    HDL_RESOLVE( hdl_devequ                    );

    HDL_RESOLVE( debug_cpu_state               );
    HDL_RESOLVE( debug_cd_cmd                  );
    HDL_RESOLVE( debug_program_interrupt       );
    HDL_RESOLVE( debug_diagnose                );
    HDL_RESOLVE( debug_sclp_unknown_command    );
    HDL_RESOLVE( debug_sclp_unknown_event      );
    HDL_RESOLVE( debug_sclp_unknown_event_mask );
    HDL_RESOLVE( debug_sclp_event_data         );
    HDL_RESOLVE( debug_chsc_unknown_request    );
    HDL_RESOLVE( debug_watchdog_signal         );

#if defined( OPTION_W32_CTCI )
    HDL_RESOLVE( debug_tt32_stats              );
    HDL_RESOLVE( debug_tt32_tracing            );
#endif
}
// END_RESOLVER_SECTION

/*-------------------------------------------------------------------*/

// HDL_DEVICE_SECTION
static void hdl_define_devtypes_ep( DEFDEV* defdev )
{
    HDL_DEVICE (  2305,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  2311,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  2314,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  3330,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  3340,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  3350,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  3375,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  3380,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  3390,  *hdl_real_ckd_DEVHND  );
    HDL_DEVICE (  9345,  *hdl_real_ckd_DEVHND  );

    HDL_DEVICE (  0671,  *hdl_real_fba_DEVHND  );
    HDL_DEVICE (  3310,  *hdl_real_fba_DEVHND  );
    HDL_DEVICE (  3370,  *hdl_real_fba_DEVHND  );
    HDL_DEVICE (  9313,  *hdl_real_fba_DEVHND  );
    HDL_DEVICE (  9332,  *hdl_real_fba_DEVHND  );
    HDL_DEVICE (  9335,  *hdl_real_fba_DEVHND  );
    HDL_DEVICE (  9336,  *hdl_real_fba_DEVHND  );
}
// END_DEVICE_SECTION

/*-------------------------------------------------------------------*/
/*             HDL 'SECTION' MACROS CALLBACK FUNCTIONS               */
/*-------------------------------------------------------------------*/
/* The below module callback functions (as identified by "_cb" in    */
/* their name) are called into by modules whenever they are first    */
/* loaded in order for them to pass information from themselves to   */
/* the main HDL logic (i.e. hdl.c, i.e. THIS source member).         */
/*                                                                   */
/* Each of the module "SECTION" macros (HDL_DEPENDENCY_SECTION,      */
/* HDL_REGISTER_SECTION, HDL_RESOLVER_SECTION, HDL_DEVICE_SECTION,   */
/* and HDL_INSTRUCTION_SECTION) defines the beginning of an "_ep"    */
/* module entry-point function that HDL calls into when the module   */
/* is first loaded. It passes to each of these module entry-point    */
/* functions the address of one of the below callback functions,     */
/* which the module's entry-point function then calls into (calls    */
/* the caller back again, hence the term "callback") in order to     */
/* pass its (the module's) information back to the caller (HDL).     */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*       hdl_check_depends_cb  --  dependency check callback         */
/*-------------------------------------------------------------------*/
static int hdl_check_depends_cb( const char* name, const char* version, int size )
{
    HDLDEP*  depent;

    /* Locate the dependency entry we're interested in */
    for (depent = hdl_depend; depent; depent = depent->next)
    {
        if (strcmp( name, depent->name ) == 0)
            break; // (we found it)
    }

    /* If we found it, then use it */
    if (depent)
    {
        if (strcmp( version, depent->version ))
        {
            // "HDL: dependency check failed for %s, version %s expected %s"
            WRMSG( HHC01509, "I", name, version, depent->version );
            return -1;
        }

        if (size != depent->size)
        {
            // "HDL: dependency check failed for %s, size %d expected %d"
            WRMSG( HHC01510, "I", name, size, depent->size );
            return -1;
        }
    }
    else /* Else not found; add it as a new entry in our chain */
    {
        depent = malloc( sizeof( HDLDEP ));

        depent->name     =  strdup( name );
        depent->version  =  strdup( version );
        depent->size     =  size;
        depent->next     =  hdl_depend;

        hdl_depend = depent;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*    hdl_register_symbols_cb  --  register entry-point callback     */
/*-------------------------------------------------------------------*/
static void hdl_register_symbols_cb( const char* name, void* symbol )
{
    HDLSYM*  newsym  = malloc( sizeof( HDLSYM ));

    newsym->name    =  strdup( name );
    newsym->symbol  =  symbol;
    newsym->refcnt  =  0;
    newsym->next    =  hdl_curmod->symbols;

    hdl_curmod->symbols = newsym;
}

/*-------------------------------------------------------------------*/
/*       hdl_resolve_symbols_cb  --  find entry-point callback       */
/*-------------------------------------------------------------------*/
static void* hdl_resolve_symbols_cb( const char* name )
{
    HDLMOD*  mod;
    HDLSYM*  sym;
    void*    symbol;

    /* Find entry-point and increase reference */
    for (mod = hdl_mods; mod; mod = mod->next)
    {
        for (sym = mod->symbols; sym; sym = sym->next)
        {
            if (strcmp( sym->name, name ) == 0)
            {
                sym->refcnt++;
                return sym->symbol;
            }
        }
    }

    /* If not found, then lookup as regular symbol (i.e. for each
       module in our chain, ask the host operating if the module
       exported a symbol by that name).
       */
    for (mod = hdl_mods; mod; mod = mod->next)
    {
        /* Ask host O/S if the symbol is exported from this module */
        if ((symbol = dlsym( mod->handle, name )))
        {
            /* Yep! It's a regular (unregistered) exported symbol.
               Add this symbol to our symbol chain for this module.
            */
            if (!(sym = malloc( sizeof( HDLSYM ))))
            {
                // "HDL: error in function %s: %s"
                WRMSG( HHC01511, "E", "malloc()", strerror( errno ));
                return NULL;
            }

            sym->name    =  strdup( name );
            sym->symbol  =  symbol;
            sym->refcnt  =  1;
            sym->next    =  mod->symbols;

            mod->symbols = sym;

            return symbol;
        }
    }

    /* Symbol not found */
    return NULL;
}

/*-------------------------------------------------------------------*/
/*               hdl_getsym  --  find symbol                (public) */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* hdl_getsym( const char* symname )
{
    /* This function is mostly for the benefit of the httpserv.c's
       'http_request' function to locate any cgibin functions that
       may have been overridden/added by a hercules dynamic module.
    */
    return hdl_resolve_symbols_cb( symname );
}

/*-------------------------------------------------------------------*/
/*     hdl_define_devtypes_cb  --  register device-type callback     */
/*-------------------------------------------------------------------*/
static void hdl_define_devtypes_cb( const char* typname, DEVHND* devhnd )
{
    HDLDEV*  newhnd  = malloc( sizeof( HDLDEV ));

    newhnd->name  =  strdup( typname );
    newhnd->hnd   =  devhnd;
    newhnd->next  =  hdl_curmod->devices;

    hdl_curmod->devices = newhnd;
}

/*-------------------------------------------------------------------*/
/*     hdl_define_instructs_cb  --  Define instruction callback      */
/*-------------------------------------------------------------------*/
static void hdl_define_instructs_cb( U32 hdl_arch, int opcode, const char* name, void* func )
{
    /*
    **  Verify caller sanity: caller only allowed to replace one
    **  instruction for one HDL architecture at a time since that
    **  is all an HDLINS struct has room for. (HDL_DEF_INST calls
    **  us separately for each HDL architecture.)
    */
    if (0
        || hdl_arch == HDL_INSTARCH_370         //  S/370
        || hdl_arch == HDL_INSTARCH_390         //  ESA/390
        || hdl_arch == HDL_INSTARCH_900         //  z/Arch
    )
    {
        /* Allocate a new HDLINS entry for this instruction */
        HDLINS*  newins  = malloc( sizeof( HDLINS ));

        /* Initialize the entry for this instruction */
        newins->instname   =  strdup( name );
        newins->hdl_arch   =  hdl_arch;
        newins->opcode     =  opcode > 0xff ? opcode : (opcode << 8) ;
        newins->instfunc   =  func;

        /* Add it to the current module's chain */
        newins->next =  hdl_curmod->instructs;
        hdl_curmod->instructs = newins;

        /* Call replace_opcode via helper to do the grunt work */
        hdl_replace_opcode( true, newins );
    }
    else
    {
        // "HDL: Invalid architecture passed to %s"
        WRMSG( HHC01503, "E", "hdl_define_instructs_cb" );
    }
}

/*-------------------------------------------------------------------*/
/*           ( hdl_define_instructs_cb helper function )             */
/*-------------------------------------------------------------------*/
static void hdl_replace_opcode( bool insert, HDLINS* instr )
{
  void* original = NULL;  // (for error check)

  if (insert)  // (insert == define replacement)
  {
    instr->original = NULL;

#ifdef                                      _370
    if (instr->hdl_arch   ==    HDL_INSTARCH_370)
      instr->original = replace_opcode( ARCH_370_IDX, instr->instfunc, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                      _390
    if (instr->hdl_arch   ==    HDL_INSTARCH_390)
      instr->original = replace_opcode( ARCH_390_IDX, instr->instfunc, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                      _900
    if (instr->hdl_arch   ==    HDL_INSTARCH_900)
      instr->original = replace_opcode( ARCH_900_IDX, instr->instfunc, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

    original = instr->original;
  }
  else // (!insert == restore previous)
  {

#ifdef                                 _370
    if (instr->hdl_arch == HDL_INSTARCH_370)
      original = replace_opcode(   ARCH_370_IDX, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                 _390
    if (instr->hdl_arch == HDL_INSTARCH_390)
      original = replace_opcode(   ARCH_390_IDX, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                 _900
    if (instr->hdl_arch == HDL_INSTARCH_900)
      original = replace_opcode(   ARCH_900_IDX, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

  }

  if (!original)
  {
    // "HDL: Invalid architecture passed to %s"
    WRMSG( HHC01503, "E", "hdl_replace_opcode" );
  }
}

/*-------------------------------------------------------------------*/
