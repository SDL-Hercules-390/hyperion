/* HDL.C        (C) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */

#include "hstdinc.h"

#define _HDL_C_
#define _HUTIL_DLL_

#include "hercules.h"
#include "opcode.h"

/*-------------------------------------------------------------------*/
/*    List of dynamic modules to pre-load at hdl_main startup        */
/*-------------------------------------------------------------------*/
#if defined( OPTION_DYNAMIC_LOAD )

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
static LOCK     hdl_lock;                /* loader lock              */
static DLLENT*  hdl_dll      = NULL;     /* dll chain                */
static DLLENT*  hdl_cdll     = NULL;     /* current dll (hdl_lock)   */
static HDLDEP*  hdl_depend   = NULL;     /* Version codes in hdlmain */
static char*    hdl_modpath  = NULL;     /* modules load path        */
static bool     hdl_arg_p    = false;    /* -p cmdline opt specified */

/*-------------------------------------------------------------------*/
/*                 Function forward references                       */
/*-------------------------------------------------------------------*/
static void  hdl_didf          ( int, int, char*, void* );
static void  hdl_modify_opcode ( bool insert, HDLINS* );

#endif // defined( OPTION_DYNAMIC_LOAD )

/*-------------------------------------------------------------------*/
/*     More global variables and function forward references         */
/*-------------------------------------------------------------------*/

static HDLSHUT* hdl_shutlist = NULL;     /* Shutdown call list       */
static bool     hdl_shutting = false;    /* hdl shutdown in progesss */

DLL_EXPORT char* (*hdl_device_type_equates)( const char* );


/*-------------------------------------------------------------------*/
/*              hdl_addshut - add shutdown call                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_addshut( char* shutname, SHUTFUNC* shutfunc, void* shutarg )
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
/*             hdl_delshut - remove shutdown call                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_delshut( SHUTFUNC* shutfunc, void* shutarg )
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
/*          hdl_atexit - call shutdown entries in LIFO order         */
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


#if defined( OPTION_DYNAMIC_LOAD )

/*-------------------------------------------------------------------*/
/*             hdl_checkpath - check module path                     */
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
/*              hdl_initpath - initialize module path                */
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
/*                 hdl_getpath - return module path                  */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* hdl_getpath()
{
    if (!hdl_modpath)           // (if default not set yet)
        hdl_initpath( NULL );   // (then initilize default)
    return hdl_modpath;         // (return current value)
}


/*-------------------------------------------------------------------*/
/*             hdl_setpath - Set module load path                    */
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
/*           hdl_dlopen - load a dynamic module                      */
/*-------------------------------------------------------------------*/
static void* hdl_dlopen( char* filename, int flag )
{
    char*   fullname;
    void*   ret;
    size_t  fulllen = 0;

    /*
     *  Check in this order:
     *
     *    1. filename as passed
     *    2. filename with extension if needed
     *    3. modpath added if basename( filename )
     *    4. extension added to #3
     */
    if ((ret = dlopen( filename, flag ))) /* try filename as-is first */
        return ret;

     //  2. filename with extension if needed

    fulllen = strlen( filename ) + strlen( hdl_modpath ) + 2 + HDL_SUFFIX_LENGTH;
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
     //  3. modpath added if basename( filename )

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

    //  4. extension added to #3

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
/*              hdl_dvad - register device type                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_dvad( char* devname, DEVHND* devhnd )
{
    HDLDEV*  newhnd  = malloc( sizeof( HDLDEV ));

    newhnd->name  =  strdup( devname );
    newhnd->hnd   =  devhnd;
    newhnd->next  =  hdl_cdll->hndent;

    hdl_cdll->hndent = newhnd;
}


/*-------------------------------------------------------------------*/
/*             hdl_fhnd - find registered device handler             */
/*-------------------------------------------------------------------*/
static DEVHND* hdl_fhnd( const char* devname )
{
    DLLENT*  dllent;
    HDLDEV*  hndent;

    for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        for (hndent = dllent->hndent; hndent; hndent = hndent->next)
        {
            if (!strcasecmp( devname, hndent->name ))
                return hndent->hnd;
        }
    }

    return NULL;
}


/*-------------------------------------------------------------------*/
/*           hdl_bdnm - build device module name                     */
/*-------------------------------------------------------------------*/
static char* hdl_bdnm( const char* ltype )
{
    char*   dtname;
    size_t  len, size;

    /* Don't forget the extra +1 for the \0 ending.             @PJJ */
    size = strlen( ltype ) + sizeof( HDL_HDTP_Q ) + 1;  // "hdt"

    dtname = malloc( size );

    strlcpy( dtname, HDL_HDTP_Q, size );    // "hdt"
    strlcat( dtname, ltype,      size );

    for (len = 0; len < strlen( dtname ); len++)
        if (isupper( dtname[ len ]))
            dtname[ len ] = tolower( dtname[ len ]);

    return dtname;
}


/*-------------------------------------------------------------------*/
/*                 hdl_getdev - obtain device handler                */
/*-------------------------------------------------------------------*/
DLL_EXPORT DEVHND* hdl_getdev( const char* devtype )
{
    DEVHND*  hnd;
    char*    hdtname;
    char*    ltype;

    if ((hnd = hdl_fhnd( devtype )))
        return hnd;

    hdtname = hdl_bdnm( devtype );

    if (0
        || hdl_load( hdtname, HDL_LOAD_NOMSG )
        || !hdl_fhnd( devtype )
    )
    {
        if (hdl_device_type_equates)
        {
            if ((ltype = hdl_device_type_equates( devtype )))
            {
                free( hdtname );
                hdtname = hdl_bdnm( ltype );
                hdl_load( hdtname, HDL_LOAD_NOMSG );
            }
        }
    }

    free( hdtname );

    return hdl_fhnd( devtype );
}


/*-------------------------------------------------------------------*/
/*              hdl_listmods - list all entry points                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_listmods( int flags )
{
    DLLENT*  dllent;
    MODENT*  modent;
    char     buf[ 256 ];
    int      len;

    for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        // "HDL: module type = %s, name = %s, flags = (%sunloadable, %sforced)"
        WRMSG( HHC01531, "I"
            , (dllent->flags & HDL_LOAD_MAIN)       ? "main" : "load"
            ,  dllent->name
            , (dllent->flags & HDL_LOAD_NOUNLOAD)   ? "not " : ""
            , (dllent->flags & HDL_LOAD_WAS_FORCED) ? ""     : "not "
        );

        for (modent = dllent->modent; modent; modent = modent->modnext)
        {
            if (0
                || (flags & HDL_LIST_ALL)
                || !(1
                     && (dllent->flags & HDL_LOAD_MAIN)
                     && !modent->fep
                    )
            )
            {
                // "HDL:  symbol = %s, loadcount = %d%s, owner = %s"
                WRMSG( HHC01532, "I"
                    , modent->name
                    , modent->count
                    , modent->fep ? "" : ", unresolved"
                    , dllent->name
                );
            }
        }

        if (dllent->hndent)
        {
            HDLDEV*  hndent;
            len = 0;

            for (hndent = dllent->hndent; hndent; hndent = hndent->next)
                len += snprintf( buf + len, sizeof( buf ) - len, " %s", hndent->name );

            // "HDL:  devtype(s) =%s"
            WRMSG( HHC01533, "I", buf );
        }

        if (dllent->insent)
        {
            HDLINS*  insent;

            for (insent = dllent->insent; insent; insent = insent->next)
            {
                len = 0;

#if defined( _370 )
                if (insent->archflags & HDL_INSTARCH_370)
                    len += snprintf( buf + len, sizeof( buf ) - len, ", archlvl = " _ARCH_370_NAME );
#endif

#if defined( _390 )
                if (insent->archflags & HDL_INSTARCH_390)
                    len += snprintf( buf + len, sizeof( buf ) - len, ", archlvl = " _ARCH_390_NAME );
#endif

#if defined( _900 )
                if (insent->archflags & HDL_INSTARCH_900)
                    len += snprintf( buf + len, sizeof( buf ) - len, ", archlvl = " _ARCH_900_NAME );
#endif
                // "HDL:  instruction = %s, opcode = %4.4X%s"
                WRMSG( HHC01534, "I"
                    , insent->instname
                    , insent->opcode
                    , buf
                );
            }
        }
    }
}


/*-------------------------------------------------------------------*/
/*              hdl_listdeps - list all dependencies                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hdl_listdeps()
{
    HDLDEP*  depent;

    for (depent = hdl_depend; depent; depent = depent->next)
        // "HDL: dependency %s version %s size %d"
        WRMSG( HHC01535, "I", depent->name, depent->version, depent->size );
}


/*-------------------------------------------------------------------*/
/*         hdl_dadd - add dependency                                 */
/*-------------------------------------------------------------------*/
static void hdl_dadd( char* name, char* version, int size )
{
    HDLDEP**  newdep;

    /* Locate last link in chain */
    for (newdep = &(hdl_depend); *newdep; newdep = &((*newdep)->next));

    (*newdep) = malloc( sizeof( HDLDEP ));

    (*newdep)->next     =  NULL;
    (*newdep)->name     =  strdup(name);
    (*newdep)->version  =  strdup(version);
    (*newdep)->size     =  size;
}


/*-------------------------------------------------------------------*/
/*         hdl_dchk - dependency check                               */
/*-------------------------------------------------------------------*/
static int hdl_dchk( char* name, char* version, int size )
{
    HDLDEP*  depent;

    /* Locate last link in chain */
    for (depent = hdl_depend; depent && strcmp( name, depent->name ); depent = depent->next);

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
    else
        hdl_dadd( name, version, size );

    return 0;
}


/*-------------------------------------------------------------------*/
/*               hdl_findsym - find entry point                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* hdl_findsym( char* name )
{
    DLLENT*  dllent;
    MODENT*  modent;
    void*    sym;

    /* Find entry point and increase loadcount */
    for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        for (modent = dllent->modent; modent; modent = modent->modnext)
        {
            if (strcmp( name, modent->name ) == 0)
            {
                modent->count++;
                return modent->fep;
            }
        }
    }

    /* If not found then lookup as regular symbol */
    for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if ((sym = dlsym( dllent->dll, name )))
        {
            if (!(modent = malloc( sizeof( MODENT ))))
            {
                // "HDL: error in function %s: %s"
                WRMSG( HHC01511, "E", "malloc()", strerror( errno ));
                return NULL;
            }

            modent->fep    =  sym;
            modent->name   =  strdup( name );
            modent->count  =  1;

            /* Insert current entry as first in chain */
            modent->modnext = dllent->modent;
            dllent->modent  = modent;

            return sym;
        }
    }

    /* No entry point found */
    return NULL;
}


/*-------------------------------------------------------------------*/
/*               hdl_next - find next entry point in chain           */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* hdl_next( void* sym )
{
    DLLENT*  dllent;
    MODENT*  modent  = NULL;
    char*    name;

    for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        for (modent = dllent->modent; modent; modent = modent->modnext)
        {
            if (modent->fep == sym)
                break;
        }

        if (modent && modent->fep == sym)
            break;
    }

    if (!modent)
        return NULL;

    name = modent->name;

    if (!(modent = modent->modnext))
    {
        if ((dllent = dllent->dllnext))
            modent = dllent->modent;
        else
            return NULL;
    }

    /* Find entry point */
    for (; dllent; dllent = dllent->dllnext, modent = dllent->modent)
    {
        for (; modent; modent = modent->modnext)
        {
            if (!strcmp( name, modent->name ))
                return modent->fep;
        }
    }

    return NULL;
}


/*-------------------------------------------------------------------*/
/*          hdl_regi - register entry point                          */
/*-------------------------------------------------------------------*/
static void hdl_regi( char* name, void* sym )
{
    MODENT*  modent  = malloc( sizeof( MODENT ));

    modent->fep      =  sym;
    modent->name     =  strdup( name );
    modent->count    =  0;
    modent->modnext  =  hdl_cdll->modent;

    hdl_cdll->modent = modent;
}


/*-------------------------------------------------------------------*/
/*          hdl_term - process all "HDL_FINAL_SECTION"s              */
/*-------------------------------------------------------------------*/
static void hdl_term( void* unused )
{
    DLLENT*  dllent;

    UNREFERENCED( unused );

    if (MLVL( DEBUG ))
        // "HDL: begin termination sequence"
        WRMSG( HHC01512, "I" );

    /* Call all final routines, in reverse load order */
    for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if (dllent->hdlfini)
        {
            if (MLVL( DEBUG ))
                // "HDL: calling module cleanup routine %s"
                WRMSG( HHC01513, "I", dllent->name );

            dllent->hdlfini();

            if (MLVL( DEBUG ))
                // "HDL: module cleanup routine %s complete"
                WRMSG( HHC01514, "I", dllent->name );
        }
    }

    if (MLVL( DEBUG ))
        // "HDL: termination sequence complete"
        WRMSG( HHC01515, "I" );
}


/*-------------------------------------------------------------------*/
/*          hdl_main - initialize hercules dynamic loader            */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_main()
{
    HDLPRE*  preload;

    initialize_lock( &hdl_lock );

    hdl_shutting = false;

    dlinit();

    if (!(hdl_cdll = hdl_dll = malloc( sizeof( DLLENT ))))
    {
        char buf[64];
        MSGBUF( buf,  "malloc(%d)", (int) sizeof( DLLENT ));
        // "HDL: error in function %s: %s"
        WRMSG( HHC01511, "S", buf, strerror( errno ));
        return -1;
    }

    hdl_cdll->name = strdup( "*Hercules" );

#if 1 // (ZZ FIXME: might not work on all platforms)

    /* This was a nice trick. Unfortunately, on some platforms  */
    /* it becomes impossible. Some platforms need fully defined */
    /* DLLs, some other platforms do not allow dlopen(self)     */

    if (!(hdl_cdll->dll = hdl_dlopen( NULL, RTLD_NOW )))
    {
        // "HDL: unable to open module %s: %s"
        WRMSG( HHC01516, "S", "hercules", dlerror());
        return -1;
    }

    hdl_cdll->flags = (HDL_LOAD_MAIN | HDL_LOAD_NOUNLOAD);

    /* Locate the "hdl_depc" entry point */
    if (!(hdl_cdll->hdldepc = dlsym( hdl_cdll->dll, HDL_DEPC_Q )))
    {
        // "HDL: no dependency section in %s: %s"
        WRMSG( HHC01517, "S",  hdl_cdll->name, dlerror());
        return -1;
    }

    hdl_cdll->hdlinit = dlsym( hdl_cdll->dll, HDL_INIT_Q ); // "hdl_init"
    hdl_cdll->hdlreso = dlsym( hdl_cdll->dll, HDL_RESO_Q ); // "hdl_reso"
    hdl_cdll->hdlddev = dlsym( hdl_cdll->dll, HDL_DDEV_Q ); // "hdl_ddev"
    hdl_cdll->hdldins = dlsym( hdl_cdll->dll, HDL_DINS_Q ); // "hdl_dins"
    hdl_cdll->hdlfini = dlsym( hdl_cdll->dll, HDL_FINI_Q ); // "hdl_fini"

#endif // (ZZ FIXME: might not work on all platforms)

    /* No modules or device types registered yet */
    hdl_cdll->modent = NULL;
    hdl_cdll->hndent = NULL;
    hdl_cdll->insent = NULL;

    /* No dll's loaded yet */
    hdl_cdll->dllnext = NULL;

    obtain_lock( &hdl_lock );
    {
        if (hdl_cdll->hdldepc) hdl_cdll->hdldepc( &hdl_dadd );
        if (hdl_cdll->hdlinit) hdl_cdll->hdlinit( &hdl_regi );
        if (hdl_cdll->hdlreso) hdl_cdll->hdlreso( &hdl_findsym );
        if (hdl_cdll->hdlddev) hdl_cdll->hdlddev( &hdl_dvad );
        if (hdl_cdll->hdldins) hdl_cdll->hdldins( &hdl_didf );
    }
    release_lock( &hdl_lock );

    /* Register termination exit */
    hdl_addshut( "hdl_term", hdl_term, NULL );

    for (preload = hdl_preload; preload->name; preload++)
        hdl_load( preload->name, preload->flag );

    return 0;
}


/*-------------------------------------------------------------------*/
/*             hdl_load - load a dll                                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_load( char* name, int flags )
{
    DLLENT*  dllent;
    DLLENT*  tmpdll;
    MODENT*  modent;
    char*    modname;

    /* Search module chain to see if module is already loaded */
    modname = (modname = strrchr( name, '/' )) ? modname+1 : name;

    for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if (strfilenamecmp( modname, dllent->name ) == 0)
        {
            // "HDL: module %s already loaded"
            WRMSG( HHC01519, "E", dllent->name );
            return -1;
        }
    }

    if (!(dllent = malloc( sizeof( DLLENT ))))
    {
        // "HDL: error in function %s: %s"
        WRMSG( HHC01511, "E", "malloc()", strerror( errno ));
        return -1;
    }

    /* LOad the module */
    dllent->name = strdup( modname );

    if (!(dllent->dll = hdl_dlopen( name, RTLD_NOW )))
    {
        if (!(flags & HDL_LOAD_NOMSG))
            // "HDL: unable to open module %s: %s"
            WRMSG( HHC01516, "E", name, dlerror());

        free( dllent );
        return -1;
    }

    dllent->flags = (flags & (~HDL_LOAD_WAS_FORCED));

    /* Locate the "hdl_depc" entry point */
    if (!(dllent->hdldepc = dlsym( dllent->dll, HDL_DEPC_Q )))
    {
        // "HDL: no dependency section in %s: %s"
        WRMSG( HHC01517, "E", dllent->name, dlerror());
        dlclose( dllent->dll );
        free( dllent );
        return -1;
    }

    /* Reject loading the same module again twice */
    for (tmpdll = hdl_dll; tmpdll; tmpdll = tmpdll->dllnext)
    {
        if (tmpdll->hdldepc == dllent->hdldepc)
        {
            // "HDL: module %s is duplicate of %s"
            WRMSG( HHC01520, "E", dllent->name, tmpdll->name );
            dlclose(dllent->dll);
            free(dllent);
            return -1;
        }
    }

    dllent->hdlinit = dlsym( dllent->dll, HDL_INIT_Q ); // "hdl_init"
    dllent->hdlreso = dlsym( dllent->dll, HDL_RESO_Q ); // "hdl_reso"
    dllent->hdlddev = dlsym( dllent->dll, HDL_DDEV_Q ); // "hdl_ddev"
    dllent->hdldins = dlsym( dllent->dll, HDL_DINS_Q ); // "hdl_dins"
    dllent->hdlfini = dlsym( dllent->dll, HDL_FINI_Q ); // "hdl_fini"

    /* No modules or device types registered yet */
    dllent->modent = NULL;
    dllent->hndent = NULL;
    dllent->insent = NULL;

    obtain_lock( &hdl_lock );
    {
        /* Call module's HDL_DEPENDENCY_SECTION, if it has one */
        if (dllent->hdldepc)
        {
            if (dllent->hdldepc( &hdl_dchk ))
            {
                // "HDL: dependency check failed for module %s"
                WRMSG( HHC01518, "E", dllent->name );

                /* Allow module to be loaded anyway if forced */
                if (!(flags & HDL_LOAD_FORCE))
                {
                    dlclose( dllent->dll );
                    free( dllent );
                    release_lock( &hdl_lock );
                    return -1;
                }

                dllent->flags |= HDL_LOAD_WAS_FORCED;
            }
        }

        hdl_cdll = dllent;

        /* Call module's HDL_REGISTER_SECTION, if it has one */
        if (hdl_cdll->hdlinit)
            dllent->hdlinit( &hdl_regi );

        /* Insert current entry as first in chain */
        dllent->dllnext = hdl_dll;
        hdl_dll = dllent;

        /* Reset the loadcounts */
        for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
            for (modent = dllent->modent; modent; modent = modent->modnext)
                modent->count = 0;

        /* Call HDL_RESOLVER_SECTION again for every module in our chain */
        for (dllent = hdl_dll; dllent; dllent = dllent->dllnext)
        {
            if (dllent->hdlreso)
                dllent->hdlreso( &hdl_findsym );
        }

        /* Call module's HDL_DEVICE_SECTION, if it has one */
        if (hdl_cdll->hdlddev)
            hdl_cdll->hdlddev( &hdl_dvad );

        /* Call module's HDL_INSTRUCTION_SECTION, if it has one */
        if(hdl_cdll->hdldins)
            (hdl_cdll->hdldins)(&hdl_didf);

        hdl_cdll = NULL;
    }
    release_lock( &hdl_lock );

    return 0;
}


/*-------------------------------------------------------------------*/
/*             hdl_unload - unload a dll                             */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hdl_unload( char* name )
{
    char*     modname;
    DEVBLK*   dev;

    DLLENT**  dllent;
    DLLENT*   tmpdll;

    MODENT*   modent;
    MODENT*   tmpmod;

    HDLDEV*   hnd;
    HDLDEV*   nexthnd;

    HDLINS*   ins;
    HDLINS*   nextins;

    modname = (modname = strrchr( name, '/' )) ? modname+1 : name;

    obtain_lock( &hdl_lock );
    {
        /* Locate this module's entry in our chain */
        for (dllent = &(hdl_dll);
            *dllent && strfilenamecmp( modname, (*dllent)->name ) != 0;
            dllent = &((*dllent)->dllnext))
        {
            ; // (nop)
        }

        /* Was module found? */
        if (!*dllent)
        {
            release_lock( &hdl_lock );
            // "HDL: module %s not found"
            WRMSG( HHC01524, "E", modname );
            return -1;
        }

        /* Error if it 's not allowed to be unloaded */
        if ((*dllent)->flags & (HDL_LOAD_MAIN | HDL_LOAD_NOUNLOAD))
        {
            release_lock( &hdl_lock );
            // "HDL: unloading of module %s not allowed"
            WRMSG( HHC01521, "E", (*dllent)->name );
            return -1;
        }

        /* Disallow device modules to be unloaded if a device
           is still using it by searching the device chain to
           see if any of their DEVHND vector table matches the
           one for this module.
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
                for (hnd = (*dllent)->hndent; hnd; hnd = hnd->next)
                {
                    /* Is device still using this module? */
                    if (dev->hnd == hnd->hnd)
                    {
                        release_lock( &hdl_lock );
                        // "HDL: module %s bound to device %1d:%04X"
                        WRMSG( HHC01522, "E", (*dllent)->name,
                            SSID_TO_LCSS( dev->ssid ), dev->devnum );
                        return -1;
                    }
                }
            }
        }

        /* Call module's HDL_FINAL_SECTION, if it has one */
        if ((*dllent)->hdlfini)
        {
            int rc;

            if ((rc = ((*dllent)->hdlfini)()))
            {
                release_lock( &hdl_lock );
                // "HDL: unload of module %s rejected by final section"
                WRMSG( HHC01523, "E", (*dllent)->name );
                return rc;
            }
        }

        modent = (*dllent)->modent;

        /* Free all symbols */
        while (modent)
        {
            tmpmod = modent;

            /* remove current entry from chain */
            modent = modent->modnext;

            /* free module resources */
            free( tmpmod->name );
            free( tmpmod );
        }

        /* Remove the module from our chain */
        tmpdll  = *dllent;
        *dllent = (*dllent)->dllnext;

        /* Free all device registrations */
        for (hnd = tmpdll->hndent; hnd;)
        {
            free( hnd->name );
            nexthnd = hnd->next;
            free( hnd );
            hnd = nexthnd;
        }

        /* Free all instruction registrations */
        for (ins = tmpdll->insent; ins;)
        {
            /* Go back to using the previously defined
              instruction function
            */
            hdl_modify_opcode( false, ins );

            free( ins->instname );
            nextins = ins->next;
            free( ins );
            ins = nextins;
        }

        /* Free the module's chain entry itself */
        free( tmpdll->name );
        free( tmpdll );

        /* Reset loadcounts for existing modules */
        for (tmpdll = hdl_dll; tmpdll; tmpdll = tmpdll->dllnext)
            for (tmpmod = tmpdll->modent; tmpmod; tmpmod = tmpmod->modnext)
                tmpmod->count = 0;

        /* (Re-)Call all existing modules' HDL_RESOLVER_SECTION again */
        for (tmpdll = hdl_dll; tmpdll; tmpdll = tmpdll->dllnext)
        {
            if (tmpdll->hdlreso)
                tmpdll->hdlreso( &hdl_findsym );
        }
    }
    release_lock( &hdl_lock );

    return 0;
}


/*-------------------------------------------------------------------*/
/*                     hdl_modify_opcode                             */
/*-------------------------------------------------------------------*/
static void hdl_modify_opcode( bool insert, HDLINS* instr )
{
  if (insert)
  {

#ifdef                                      _370
    if (instr->archflags &      HDL_INSTARCH_370)
      instr->original = replace_opcode( ARCH_370_IDX, instr->instruction, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                      _390
    if (instr->archflags &      HDL_INSTARCH_390)
      instr->original = replace_opcode( ARCH_390_IDX, instr->instruction, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                      _900
    if (instr->archflags &      HDL_INSTARCH_900)
      instr->original = replace_opcode( ARCH_900_IDX, instr->instruction, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

  }
  else // (!insert)
  {

#ifdef                                 _370
    if (instr->archflags & HDL_INSTARCH_370)
      replace_opcode(              ARCH_370_IDX, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                 _390
    if (instr->archflags & HDL_INSTARCH_390)
      replace_opcode(              ARCH_390_IDX, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

#ifdef                                 _900
    if (instr->archflags & HDL_INSTARCH_900)
      replace_opcode(              ARCH_900_IDX, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff );
#endif

  }
  return;
}


/*-------------------------------------------------------------------*/
/*          hdl_didf - Define instruction call                       */
/*-------------------------------------------------------------------*/
static void hdl_didf( int archflags, int opcode, char* name, void* routine )
{
    HDLINS*  newins  = malloc( sizeof( HDLINS ));

    newins->opcode       =  opcode > 0xff ? opcode : (opcode << 8) ;
    newins->archflags    =  archflags;
    newins->instname     =  strdup( name );
    newins->instruction  =  routine;

    /* Insert into chain */
    newins->next =  hdl_cdll->insent;
    hdl_cdll->insent = newins;

    hdl_modify_opcode( true, newins );
}

#endif /*defined( OPTION_DYNAMIC_LOAD )*/
