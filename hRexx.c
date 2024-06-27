/* HREXX.C      (C) Copyright Enrico Sorichetti, 2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2017          */
/*              Hercules Rexx Support                                */
/*                                                                   */
/*  Released under "The Q Public License Version 1"                  */
/*  (http://www.hercules-390.org/herclic.html) as modifications to   */
/*  Hercules.                                                        */

/*  inspired by the previous Rexx implementation by Jan Jaeger       */

/*-------------------------------------------------------------------*/
/* This module contains Rexx Interpreter support code. It supports   */
/* both Regina Rexx as well as Open Object Rexx (OORexx).            */
/*                                                                   */
/* PLEASE NOTE that some of the below code is only activated when    */
/* the HAVE_OBJECT_REXX *and* HAVE_REGINA_REXX build macros are  */
/* BOTH #defined at build time, which provides the ability to choose */
/* at runtime which interpreter should be used. If only one of the   */
/* build constants is #defined then support for that intrepreter     */
/* and ONLY that intrepreter is generated and the ability to choose  */
/* at runtime which one to use is disabled.                          */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HREXX_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "hRexx.h"

#ifdef HAVE_REXX

/*-------------------------------------------------------------------*/
/* Hercules Rexx implementation default values                       */
/*-------------------------------------------------------------------*/
#define DEFAULT_PACKAGE             ""
#define DEFAULT_REXXPATH            ""
#define DEFAULT_EXTENSIONS          EXTENSIONS

#define MODE_COMMAND                0
#define MODE_SUBROUTINE             1

#define DEFAULT_MODE                MODE_COMMAND
#define DEFAULT_MSGLEVEL            FALSE
#define DEFAULT_MSGPREFIX           FALSE
#define DEFAULT_ERRPREFIX           FALSE
#define DEFAULT_USERESOLVER         TRUE
#define DEFAULT_USESYSPATH          TRUE

static char*  DefaultPackage;     // see InitializeDefaults() function
static char*  DefaultRexxPath;    // see InitializeDefaults() function
static char*  DefaultExtensions;  // see InitializeDefaults() function
static BYTE   DefaultMode;        // see InitializeDefaults() function

/*-------------------------------------------------------------------*/
/* Hercules Rexx implementation publicly visible global variables    */
/*-------------------------------------------------------------------*/
char*           PackageName       = NULL;
char*           PackageVersion    = NULL;
char*           PackageSource     = NULL;
char            PackageMajorVers  = '0';
BYTE            MsgLevel          = DEFAULT_MSGLEVEL;
BYTE            MsgPrefix         = DEFAULT_MSGPREFIX;
BYTE            ErrPrefix         = DEFAULT_ERRPREFIX;
PFNEXECCMDFUNC  ExecCmd           = NULL;
PFNEXECSUBFUNC  ExecSub           = NULL;
PFNHALTEXECFUNC HaltExec          = NULL;

/*-------------------------------------------------------------------*/
/* Hercules Rexx implementation private global variables             */
/*-------------------------------------------------------------------*/
static char*    PkgNames[ PKGS ]  = { OOREXX_PKGNAME, REGINA_PKGNAME };

static BYTE     OORexxAvailable   = FALSE;
static BYTE     ReginaAvailable   = FALSE;

static BYTE     RexxMode          = DEFAULT_MODE;
static BYTE     useResolver       = DEFAULT_USERESOLVER;
static BYTE     useSysPath        = DEFAULT_USESYSPATH;

static char*    RexxPath          = NULL;
static char**   RexxPathArray     = NULL;
static int      RexxPathCount     = 0;

static char*    SysPath           = NULL;
static char**   SysPathArray      = NULL;
static int      SysPathCount      = 0;

static char*    Extensions        = NULL;
static char**   ExtensionsArray   = NULL;
static int      ExtensionsCount   = 0;

/*-------------------------------------------------------------------*/
/*                Script processing control                          */
/*-------------------------------------------------------------------*/
struct SCRCTL {                     /* Script control structure      */
    LIST_ENTRY      link;           /* Just a link in the chain      */
    struct timeval  scr_tv;         /* When script was started       */
    char*           scr_name;       /* Name of script being run      */
    TID             scr_tid;        /* Script thread id              */
    BYTE            scr_mode;       /* Execution mode (cmd/sub)      */
    BYTE            scr_ended;      /* Script ended flag             */
    BYTE            scr_pkg;        /* Rexx package that started it  */
};
typedef struct SCRCTL SCRCTL;       /* typedef is easier to use      */

static LIST_ENTRY   scr_list = {0,0}; /* Script list anchor entry    */
static LOCK         scr_lock;       /* Lock for accessing list       */
static COND         scr_cond;       /* Thread signalling condition   */

struct THRARG {                     /* Create async thread arg       */
    SCRCTL*         scr_ctl;        /* Ptr to SCRCTL                 */
    void*           scr_args;       /* Script argument(s)            */
    int             scr_numargs;    /* Number of arguments           */
};
typedef struct THRARG THRARG;       /* typedef is easier to use      */

/*-------------------------------------------------------------------*/
/* Macro to build package function name                              */
/*-------------------------------------------------------------------*/
#define PASTEM( _p, _n )            _p ## _n
#define PKG_FUNC2( _p, _n )         PASTEM( _p, _n )
#define PKG_FUNC( _pkg, _name )     PKG_FUNC2( _pkg, _name )

/*-------------------------------------------------------------------*/
/* Package functions to Load or Activate the Rexx package            */
/*-------------------------------------------------------------------*/
#ifdef OBJECT_REXX
  extern BYTE PKG_FUNC( OOREXX_PKG, Load   )( BYTE verbose );
  extern BYTE PKG_FUNC( OOREXX_PKG, Enable )( BYTE verbose );
#else
  static BYTE PKG_FUNC( OOREXX_PKG, Load   )( BYTE verbose ) {UNREFERENCED(verbose);return FALSE;}
  static BYTE PKG_FUNC( OOREXX_PKG, Enable )( BYTE verbose ) {UNREFERENCED(verbose);return FALSE;}
#endif
#ifdef REGINA_REXX
  extern BYTE PKG_FUNC( REGINA_PKG, Load   )( BYTE verbose );
  extern BYTE PKG_FUNC( REGINA_PKG, Enable )( BYTE verbose );
#else
  static BYTE PKG_FUNC( REGINA_PKG, Load   )( BYTE verbose ) {UNREFERENCED(verbose);return FALSE;}
  static BYTE PKG_FUNC( REGINA_PKG, Enable )( BYTE verbose ) {UNREFERENCED(verbose);return FALSE;}
#endif

/*-------------------------------------------------------------------*/
/* Function forward references                                       */
/*-------------------------------------------------------------------*/
static void  InitializeDefaults    ();
static void  InitializePaths       ( char* wPath );
static void  InitializeExtensions  ( char* wExtensions );
static int   Enable                ( char* wPackage );
static void  Disable               ();
static void  DisplayOptions        ();
static BYTE  CancelRexxExec        ( TID tid );
static BYTE  CancelAllRexxExecs    ();

/*-------------------------------------------------------------------*/
/* ReportVersionSource: display package's VERSION and SOURCE info.   */
/*-------------------------------------------------------------------*/
static void ReportVersionSource()
{
    // "REXX(%s) VERSION: %s"
    // "REXX(%s) SOURCE: %s"
    WRMSG( HHC17528, "I", PackageName, PackageVersion );
    WRMSG( HHC17529, "I", PackageName, PackageSource  );
}

/*-------------------------------------------------------------------*/
/* Convert Package Name to Package Number                            */
/*-------------------------------------------------------------------*/
static BYTE PkgName2Num( const char* pkgname )
{
    BYTE pkgnum;
    for (pkgnum = 0; pkgnum < _countof( PkgNames ); pkgnum++)
        if (strcasecmp( PkgNames[ pkgnum ], pkgname ) == 0)
            return pkgnum;
    return -1;
}

/*-------------------------------------------------------------------*/
/* Convert Package Number to Package Name                            */
/*-------------------------------------------------------------------*/
static const char* PkgNum2Name( BYTE pkgnum )
{
    if (pkgnum < _countof( PkgNames ))
        return PkgNames[ pkgnum ];
    return "??????";
}

/*-------------------------------------------------------------------*/
/* Check if valid package name                    (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE IsValidPackage( const char* pkgname )
{
    if (pkgname)
    {
        BYTE pkgnum;
        for (pkgnum=0; pkgnum < PKGS; pkgnum++)
            if (strcasecmp( pkgname, PkgNum2Name( pkgnum )) == 0)
                return TRUE;
    }
    return FALSE;
}

/*-------------------------------------------------------------------*/
/* Check if package is enabled or not             (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE IsEnabled( const char* pkgname )
{
    return
    (1
        && PackageName
        && PackageVersion
        && PackageSource
        && ExecCmd
        && ExecSub
        && HaltExec
        && (0
            || (1
                && pkgname
                && IsValidPackage( pkgname )
                && strcasecmp( PackageName, pkgname ) == 0
               )
            || (1
                && !pkgname
                && IsValidPackage( PackageName )
               )
           )
    );
}

/*-------------------------------------------------------------------*/
/* Check if package is disabled or not            (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE IsDisabled()
{
    return !IsEnabled( NULL );
}

/*-------------------------------------------------------------------*/
/* Load all supported Rexx packages                                  */
/*-------------------------------------------------------------------*/
void InitializeRexx()      // (called by impl.c at system startup)
{
    static const BYTE verbose = TRUE;   // (always during startup)

    /* Initialize script control */
    initialize_lock      ( &scr_lock );
    initialize_condition ( &scr_cond );
    InitializeListHead   ( &scr_list );

    /* Initialize default options */
    InitializeDefaults();

    /* Load all available Rexx packages */
    OORexxAvailable = PKG_FUNC( OOREXX_PKG, Load )( verbose );
    ReginaAvailable = PKG_FUNC( REGINA_PKG, Load )( verbose );

    /* Automatically enable default Rexx package if possible */
    if (Enable( DefaultPackage ) != 0)
    {
        // "REXX(%s) Could not enable %sRexx package"
        WRMSG( HHC17511, "E", "", "default " );
    }

    /* Always show options at start regardless of Rexx start success */
    DisplayOptions();
}

/*-------------------------------------------------------------------*/
/* Initialize Rexx options to their starting default values          */
/*-------------------------------------------------------------------*/
static void InitializeDefaults()
{
    const char* sym;

    /* Retrieve and save the user's preferred default values */

    if (!(sym = get_symbol( "HREXX_PACKAGE" )) || !*sym)
        DefaultPackage = strdup( DEFAULT_PACKAGE );
    else
        DefaultPackage = strdup( sym );

    if (!(sym = get_symbol( "HREXX_MODE" )) || !*sym)
        DefaultMode = DEFAULT_MODE;
    else
    {
        if (strcasecmp( sym, "Command" ) == 0)
            DefaultMode = MODE_COMMAND;
        else if (strcasecmp( sym, "Subroutine" ) == 0)
            DefaultMode = MODE_SUBROUTINE;
        else
            DefaultMode = DEFAULT_MODE;
    }

    if (!(sym = get_symbol( "HREXX_PATH" )) || !*sym)
        DefaultRexxPath = strdup( DEFAULT_REXXPATH );
    else
        DefaultRexxPath = strdup( sym );

    if (!(sym = get_symbol( "HREXX_EXTENSIONS" )) || !*sym)
        DefaultExtensions = strdup( DEFAULT_EXTENSIONS );
    else
        DefaultExtensions = strdup( sym );

    /* Initialize the defaults */

    PackageName = DefaultPackage;
    RexxMode    = DefaultMode;

    InitializePaths( NULL );
    InitializeExtensions( NULL );
}

/*-------------------------------------------------------------------*/
/* InitializePaths and InitializeExtensions helper function          */
/*-------------------------------------------------------------------*/
static void InitArray( const char* str, const char* delims, const char* def,
                       BYTE empty0, /* array[0] == empty string? */
                       char** arraystr, char*** array, int* count )
{
    int i;

    /* Free the old information */
    if (*count)
    {
        char** arrayents = *array;

        for (i=0; i < *count; i++)
            free( arrayents[i] );

        free( *array );
        *array = NULL;

        free( *arraystr );
        *arraystr = NULL;

        *count = 0;
    }

    /* Initialize string to its new value */
    if (str)
        *arraystr = strdup( str );
    else if (def)
        *arraystr = strdup( def );

    if (!*arraystr && empty0)
        *arraystr = strdup( "" );

    /* Rebuild the array using its new string value */
    if (*arraystr)
    {
        char*  work;
        char*  token;
        char** arrayents;

        work = strdup( *arraystr );

        /* The 'empty0' option requests an extra empty string
           entry at index 0 followed by the other entries.
        */
        *count = (empty0 ? 1 : 0) + tkcount( work, delims );
        arrayents = *array = malloc( *count * sizeof( char* ));

        if (empty0)
            arrayents[0] = strdup("");

        for (i = empty0 ? 1 : 0, token = strtok( work, delims );
            token; token = strtok( NULL, delims ), i++)
        {
            arrayents[i] = strdup( token );
        }

        free( work );
    }
}

/*-------------------------------------------------------------------*/
/* Initialize Rexx search paths                                      */
/*-------------------------------------------------------------------*/
static void InitializePaths( char* wPath )
{
    const char* path = get_symbol( "PATH" );
    BYTE empty0 = FALSE;

    if (path && !*path)
        path = NULL;

    InitArray( wPath, PATHDELIM, DefaultRexxPath, empty0,
        &RexxPath, &RexxPathArray, &RexxPathCount );

    InitArray( path, PATHDELIM, NULL, empty0,
        &SysPath, &SysPathArray, &SysPathCount );
}

/*-------------------------------------------------------------------*/
/* Initialize Rexx extensions                                        */
/*-------------------------------------------------------------------*/
static void InitializeExtensions( char* wExtensions )
{
    /* The 'empty0' option requests an extra empty string
       entry at index 0 followed by the other entries. Thus
       our ExtensionsArray will always have at least one
       entry and ExtensionsCount will always be at least 1.
    */
    BYTE empty0 = TRUE;

    InitArray( wExtensions, EXTDELIM, DefaultExtensions, empty0,
        &Extensions, &ExtensionsArray, &ExtensionsCount );

    ASSERT( ExtensionsCount >= 1 );
}

/*-------------------------------------------------------------------*/
/* Disable Rexx                                                      */
/*-------------------------------------------------------------------*/
static void Disable()
{
    PackageName     = "";
    PackageVersion  = NULL;
    PackageSource   = NULL;
    ExecCmd         = NULL;
    ExecSub         = NULL;
    HaltExec        = NULL;
}

/*-------------------------------------------------------------------*/
/* Enable Rexx Package by Package Number          (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE EnablePkg( BYTE pkgnum, BYTE verbose )
{
    switch (pkgnum)
    {
        case                 OOREXX_PKGNUM:
            return PKG_FUNC( OOREXX_PKG, Enable )( verbose );

        case                 REGINA_PKGNUM:
            return PKG_FUNC( REGINA_PKG, Enable )( verbose );
    }

    if (verbose)
    {
        // "REXX(%s) Unknown/unsupported Rexx package '%s'"
        WRMSG( HHC17527, "E", PackageName, PkgNum2Name( pkgnum ));
    }

    return FALSE;
}

/*-------------------------------------------------------------------*/
/* Enable Rexx:   0 == success,  -1 error,  +1 "none" specified.     */
/*-------------------------------------------------------------------*/
static int Enable( char* wPackage )
{
    static const BYTE verbose = FALSE;

    if (!wPackage)                  // (if not specified)
        wPackage = DefaultPackage;  // (then use default)

    if (!wPackage[0])               // (unspecified same as "auto")
        wPackage = "auto";          // (unspecified same as "auto")

    /* If the desired package is already enabled then just return */
    if (IsEnabled( wPackage ))
        return 0;

    /* Enabling package "none" same as Disabling/Stopping Rexx */
    if (strcasecmp( wPackage, "none" ) == 0)
    {
        if (IsEnabled( NULL ))
        {
            char* oldpkg = strdup( PackageName );
            Disable();
            // "REXX(%s) Rexx has been stopped/disabled"
            WRMSG( HHC17526, "W", oldpkg );
            free( oldpkg );
        }
        else
        {
            // "REXX(%s) Rexx already stopped/disabled"
            WRMSG( HHC17523, "W", "" );
        }
        return +1; // ("none" requested)
    }

    /* Can't Enable a different Rexx package until the
       the currently enabled package is disabled first. */
    if (IsEnabled( NULL ))
    {
        // "REXX(%s) Rexx already started/enabled"
        WRMSG( HHC17522, "E", PackageName );
        return -1;
    }

    if (strcasecmp( wPackage, "auto" ) == 0)
    {
        if (1
            && OORexxAvailable
            && Enable( OOREXX_PKGNAME ) == 0
        )
        {
            // "REXX(%s) Rexx has been started/enabled"
            WRMSG( HHC17525, "I", OOREXX_PKGNAME );
        }
        else if (1
            && ReginaAvailable
            && Enable( REGINA_PKGNAME ) == 0
        )
        {
            // "REXX(%s) Rexx has been started/enabled"
            WRMSG( HHC17525, "I", REGINA_PKGNAME );
        }
        else
        {
            // "REXX(%s) Could not enable %sRexx package"
            WRMSG( HHC17511, "E", "", "either " );
            return -1;
        }
    }
    else if (strcasecmp( OOREXX_PKGNAME, wPackage ) == 0)
    {
        if (!EnablePkg( OOREXX_PKGNUM, verbose ))
        {
            // "REXX(%s) Could not enable %sRexx package"
            WRMSG( HHC17511, "E", wPackage, "requested " );
            return -1;
        }
        ReportVersionSource();
    }
    else if (strcasecmp( REGINA_PKGNAME, wPackage ) == 0)
    {
        if (!EnablePkg( REGINA_PKGNUM, verbose ))
        {
            // "REXX(%s) Could not enable %sRexx package"
            WRMSG( HHC17511, "E", wPackage, "requested " );
            return -1;
        }
        ReportVersionSource();
    }
    else
    {
        // "REXX(%s) Unknown/unsupported Rexx package '%s'"
        WRMSG( HHC17527, "E", "", wPackage );
        return -1;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* Discard SCRCTL entry                                              */
/*-------------------------------------------------------------------*/
static void FreeSCRCTL( SCRCTL* pCtl )
{
    /* Free list entry and return */
    free( pCtl->scr_name );
    free( pCtl );
}

/*-------------------------------------------------------------------*/
/* Prune SCRCTL entries                                              */
/*-------------------------------------------------------------------*/
static void PruneSCRCTL()
{
    LIST_ENTRY*  pLink  = NULL;
    SCRCTL*      pCtl   = NULL;

    for (pLink = scr_list.Flink; pLink != &scr_list; pLink = pLink->Flink)
    {
        pCtl = CONTAINING_RECORD( pLink, SCRCTL, link );

        if (pCtl->scr_ended)
        {
            pLink = pLink->Blink;   // (since entry is going away)
            RemoveListEntry( &pCtl->link );
            FreeSCRCTL( pCtl );     // (remove from list and free)
        }
    }
}

/*-------------------------------------------------------------------*/
/* Create SCRCTL entry                                               */
/*-------------------------------------------------------------------*/
static SCRCTL* NewSCRCTL
(
    char*   scr_name,               /* Name of script being run      */
    TID     scr_tid,                /* Script thread id              */
    BYTE    scr_mode                /* Execution mode (cmd/sub)      */
)
{
    SCRCTL*  pCtl;

    /* Prune list on regular basis */
    PruneSCRCTL();

    /* Create a new entry */
    if (0
        || !(pCtl = malloc( sizeof( SCRCTL )))
        || !(pCtl->scr_name = strdup( scr_name ))
    )
    {
        // "Out of memory"
        WRMSG( HHC00152, "E" );
        free( pCtl );
        return NULL;
    }

    /* Initialize the new entry */
    InitializeListLink( &pCtl->link );
    gettimeofday( &pCtl->scr_tv, NULL );

    pCtl->scr_tid   = scr_tid;
    pCtl->scr_mode  = scr_mode;
    pCtl->scr_ended = FALSE;
    pCtl->scr_pkg   = PkgName2Num( PackageName );

    return pCtl;
}

/*-------------------------------------------------------------------*/
/* Find SCRCTL entry                                                 */
/*-------------------------------------------------------------------*/
static SCRCTL* FindSCRCTL( TID tid )
{
    LIST_ENTRY*  pLink  = NULL;
    SCRCTL*      pCtl   = NULL;

    /* Prune list on regular basis */
    PruneSCRCTL();

    /* Search the list to locate the desired entry */
    for (pLink = scr_list.Flink; pLink != &scr_list; pLink = pLink->Flink)
    {
        pCtl = CONTAINING_RECORD( pLink, SCRCTL, link );
        if (equal_threads( pCtl->scr_tid, tid ))
            return pCtl; // found
    }

    return NULL;    // not found
}

/*-------------------------------------------------------------------*/
/* List SCRCTL entries                                               */
/*-------------------------------------------------------------------*/
static void ListSCRCTLs()
{
    LIST_ENTRY*  pLink  = NULL;
    SCRCTL*      pCtl   = NULL;
    char         tod[32];           // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    /* Prune list on regular basis */
    PruneSCRCTL();

    /* Check for empty list */
    if (IsListEmpty( &scr_list ))
    {
        // "No asynchronous Rexx scripts are currently running"
        WRMSG( HHC17516, "I" );
        return;
    }

    // "Started  Package ThreadId  Mode Name..."
    WRMSG( HHC17514, "I", "Started  Package ThreadId  Mode Name..." );
    WRMSG( HHC17514, "I", "-------  ------  --------  ---  -------------" );

    /* Display all entries in list */
    for (pLink = scr_list.Flink; pLink != &scr_list; pLink = pLink->Flink)
    {
        pCtl = CONTAINING_RECORD( pLink, SCRCTL, link );

        FormatTIMEVAL( &pCtl->scr_tv, tod, sizeof( tod ));

        tod[ 19 ] = 0; // "YYYY-MM-DD HH:MM:SS"

        // "%8s %6s  "TIDPAT"  %3s  %s"
        WRMSG( HHC17515, "I"
            , &tod[ 11 ] // HH:MM:SS
            , PkgNum2Name( pCtl->scr_pkg )
            , TID_CAST( pCtl->scr_tid )
            , pCtl->scr_mode == MODE_COMMAND    ? "cmd" :
              pCtl->scr_mode == MODE_SUBROUTINE ? "sub" : "???"
            , pCtl->scr_name
        );
    }
}

/*-------------------------------------------------------------------*/
/* rexx_cmd parsing control                                          */
/*-------------------------------------------------------------------*/
enum
{
    // the following enums must match the below ARGSDESC array

    _NEEDPACKAGE = 1,
    _NEEDPATH,
    _NEEDSYSPATH,
    _NEEDEXTENSIONS,
    _NEEDRESOLVER,
    _NEEDMSGLEVL,
    _NEEDMSGPREF,
    _NEEDERRPREF,
    _NEEDMODE,
    _NEEDCANCEL
};

static char* ARGSDESC[] =
{
    "",
    "Start/Enable",     // _NEEDPACKAGE
    "Path",             // _NEEDPATH
    "SysPath",          // _NEEDSYSPATH
    "Extensions",       // _NEEDEXTENSIONS
    "Resolver",         // _NEEDRESOLVER
    "MsgLevel",         // _NEEDMSGLEVL
    "MsgPrefix",        // _NEEDMSGPREF
    "ErrPrefix",        // _NEEDERRPREF
    "Mode",             // _NEEDMODE
    "Cancel"            // _NEEDCANCEL
};

/*-------------------------------------------------------------------*/
/* rexx Command - manage the Rexx environment                        */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Format:   'rexx [optname optvalue] ...'                           */
/*                                                                   */
/* Ena[ble]/Sta[rt]    Enable/Start a Rexx Package, where package    */
/*                     is either 'OORexx' (the default) or 'Regina'. */
/*                     Use the HREXX_PACKAGE environment variable    */
/*                     to define a preferred default value. "auto"   */
/*                     will automatically start the default package. */
/*                     Use "none" to prevent automatic enablement.   */
/* Disa[ble]/Sto[p]    Disable/Stop the Rexx package.                */
/*                                                                   */
/* RexxP[ath]/Path     List of directories to search for scripts.    */
/*                     No default. Use the HREXX_PATH environment    */
/*                     variable to define your preferred default.    */
/* SysP[ath]           Extend the search to the System Paths too.    */
/*                     'On' (default) or 'Off'.                      */
/* Ext[ensions]        List of extensions to use when searching for  */
/*                     scripts. A search with no extension is always */
/*                     done first. The HREXX_EXTENSIONS environment  */
/*                     can be used to set a different default list.  */
/* Suf[fixes]          Alias for 'Ext[ensions]'.                     */
/* Resolv[er]          'On' (default): Hercules will resolve the     */
/*                     script's full path. 'Off': the scriptname     */
/*                     is used as-is.                                */
/* MsgL[evel]          'Off' (default) or 'On' to disable or enable  */
/*                     Hercules messages HHC17503I and HHC17504I     */
/*                     that display a script's return code and value */
/*                     when it finishes executing.                   */
/* MsgP[refix]         'Off' (default) or 'On' to disable or enable  */
/*                     prefixing Rexx script 'say' messages with     */
/*                     Hercules message number HHC17540I.            */
/* ErrP[refix]         'Off' (default) or 'On' to disable or enable  */
/*                     prefixing Rexx script 'TRACE messages with    */
/*                     Hercules message number HHC17541D.            */
/* Mode                Define the preferred argument passing style.  */
/*                     'Com[mand]' (default) or 'Sub[routine]'. Use  */
/*                     the HREXX_MODE environment variable to define */
/*                     your preferred default mode.                  */
/* List                Lists currently running asynchronous scripts. */
/* Cancel              <tid> to halt a running asynchronous script.  */
/*                                                                   */
/* Setting any option to 'reset' resets the option to its default.   */
/* Enter the command without arguments to display current values.    */
/*                                                                   */
/*-------------------------------------------------------------------*/
int rexx_cmd( int argc, char* argv[], char* cmdline )
{
    int    iarg             = 0;
    int    argl             = 0;

    BYTE   haveEnable       = FALSE;
    BYTE   haveDisable      = FALSE;
    char*  wPackage         = NULL;
    BYTE   havePath         = FALSE;
    BYTE   haveSysPath      = FALSE;
    BYTE   haveExtensions   = FALSE;
    BYTE   haveResolver     = FALSE;
    BYTE   haveMsgLevel     = FALSE;
    BYTE   haveMsgPrefix    = FALSE;
    BYTE   haveErrPrefix    = FALSE;
    BYTE   haveMode         = FALSE;
    BYTE   haveCancel       = FALSE;

    BYTE   wSysPath         = useSysPath;
    BYTE   wResolver        = useResolver;
    BYTE   wMsgLevel        = MsgLevel;
    BYTE   wMsgPrefix       = MsgPrefix;
    BYTE   wErrPrefix       = ErrPrefix;
    BYTE   wMode            = RexxMode;

    char*  wPath            = NULL;
    char*  wExtensions      = NULL;

    int    whatValue        = 0;

    UNREFERENCED( cmdline );

    /* Just display the current options when no arguments are given */
    if (argc < 2)
    {
        DisplayOptions();
        return 0;
    }

    //----------------------------------------------------------------
    // First, parse and save their options. Once parsing is complete,
    // We will then process their options if no errors were detected.
    //----------------------------------------------------------------

    for (iarg=1; iarg < argc; iarg++)
    {
        // Get length of next token (option's name or its value).
        argl = strlen( argv[ iarg ]);

        if (!whatValue)
        {
            //-----------------------
            // Parse the option name
            //-----------------------

            if (HAVKEYW( "List" ))
            {
                obtain_lock( &scr_lock );
                {
                    ListSCRCTLs();
                }
                release_lock( &scr_lock );
                return 0;
            }

            if (!haveCancel && HAVKEYW( "Cancel" ))
            {
                haveCancel = TRUE;
                whatValue  = _NEEDCANCEL;
                continue;
            }

            if (!haveMode && HAVKEYW( "mode" ))
            {
                haveMode  = TRUE;
                whatValue = _NEEDMODE;
                continue;
            }

            if (!haveDisable &&
                ((argl >= 3 && argl <= 7 && HAVABBR( "Disable" )) ||
                 (argl >= 3 && argl <= 4 && HAVABBR( "Stop"    ))))
            {
                if (IsDisabled())
                {
                    // "REXX(%s) Rexx already stopped/disabled"
                    WRMSG( HHC17523, "E", "" );
                    return -1;
                }

                haveDisable = TRUE;
                continue;
            }

            if (!haveEnable &&
                ((argl >= 3 && argl <= 6 && HAVABBR( "Enable" )) ||
                 (argl >= 3 && argl <= 5 && HAVABBR( "Start"  ))))
            {
                if (IsEnabled( NULL ))
                {
                    // REXX(%s) Rexx already started/enabled"
                    WRMSG( HHC17522, "E", PackageName );
                    return -1;
                }

                haveEnable  = TRUE;
                whatValue   = _NEEDPACKAGE;
                continue;
            }

            if (!havePath &&
                ((argl >= 4 && argl <= 5 && HAVABBR( "Path"     )) ||
                 (argl >= 5 && argl <= 9 && HAVABBR( "RexxPath" ))))
            {
                havePath  = TRUE;
                whatValue = _NEEDPATH;
                continue;
            }

            if (!haveSysPath &&
                (argl >= 4 && argl <= 7 && HAVABBR( "SysPath" )))
            {
                haveSysPath = TRUE;
                whatValue   = _NEEDSYSPATH;
                continue;
            }

            if (!haveExtensions &&
                ((argl >= 3 && argl <= 10 && HAVABBR( "Extensions" )) ||
                 (argl >= 3 && argl <=  8 && HAVABBR( "Suffixes"   ))))
            {
                haveExtensions = TRUE;
                whatValue      = _NEEDEXTENSIONS;
                continue;
            }

            if (!haveResolver &&
                (argl >= 5 && argl <= 8 && HAVABBR( "Resolver" )))
            {
                haveResolver = TRUE;
                whatValue    = _NEEDRESOLVER;
                continue;
            }

            if (!haveMsgLevel &&
                (argl >= 4 && argl <= 8 && HAVABBR( "MsgLevel" )))
            {
                haveMsgLevel = TRUE;
                whatValue    = _NEEDMSGLEVL;
                continue;
            }

            if (!haveMsgPrefix &&
                (argl >= 4 && argl <= 9 && HAVABBR( "MsgPrefix" )))
            {
                haveMsgPrefix = TRUE;
                whatValue     = _NEEDMSGPREF;
                continue;
            }

            if (!haveErrPrefix &&
                 (argl >= 4 && argl <= 9 && HAVABBR( "ErrPrefix" )))
            {
                haveErrPrefix = TRUE;
                whatValue     = _NEEDERRPREF;
                continue;
            }

            // "REXX(%s) Argument %d: unknown option '%s'"
            WRMSG( HHC17508, "E", PackageName, iarg, argv[ iarg ]);
            return -1;
        }
        else // (whatValue > 0)
        {
            //------------------------
            // Parse the option value
            //------------------------

            switch (whatValue)
            {
                case _NEEDCANCEL:
                {
                    if (HAVKEYW( "All" ))
                        return CancelAllRexxExecs() ? 0 : -1;
                    else
                    {
                        TID_INT tid_int;
                        char c;

                        if (sscanf( argv[ iarg ], SCN_TIDPAT "%c", &tid_int, &c ) == 1)
                            return CancelRexxExec( (TID)tid_int ) ? 0 : -1;

                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;

                case _NEEDPACKAGE:
                {
                    if (0
                        || HAVKEYW( REGINA_PKGNAME )
                        || HAVKEYW( OOREXX_PKGNAME )
                        || HAVKEYW( "auto" )
                        || HAVKEYW( "none" )
                    )
                    {
                        wPackage = argv[ iarg ];
                    }
                    else if (HAVKEYW( "reset" ))
                    {
                        wPackage = DefaultPackage;
                    }
                    else
                    {
                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;

                case _NEEDEXTENSIONS:
                {
                    if (HAVKEYW( "reset" ))
                        wExtensions = DefaultExtensions;
                    else
                        wExtensions = argv[ iarg ];
                }
                break;

                case _NEEDPATH:
                {
                    if (HAVKEYW( "reset" ))
                        wPath = DefaultRexxPath;
                    else
                        wPath = argv[ iarg ];
                }
                break;

                case _NEEDSYSPATH:
                {
                    if (0
                        || HAVKEYW( "Enable" )
                        || HAVKEYW( "On" )
                    )
                    {
                        wSysPath = TRUE;
                    }
                    else if (0
                        || HAVKEYW( "Disable" )
                        || HAVKEYW( "Off" )
                    )
                    {
                        wSysPath = FALSE;
                    }
                    else if (HAVKEYW( "reset" ))
                    {
                        wSysPath = DEFAULT_USESYSPATH;
                    }
                    else
                    {
                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;

                case _NEEDRESOLVER:
                {
                    if (0
                        || HAVKEYW( "Enable" )
                        || HAVKEYW( "On" )
                    )
                    {
                        wResolver = TRUE;
                    }
                    else if (0
                        || HAVKEYW( "Disable" )
                        || HAVKEYW( "Off" )
                    )
                    {
                        wResolver = FALSE;
                    }
                    else if (HAVKEYW( "reset" ))
                    {
                        wResolver = DEFAULT_USERESOLVER;
                    }
                    else
                    {
                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;

                case _NEEDMSGLEVL:
                {
                    if (0
                        || HAVKEYW( "Disable" )
                        || HAVKEYW( "Off" )
                    )
                    {
                        wMsgLevel = FALSE;
                    }
                    else if (0
                        || HAVKEYW( "Enable" )
                        || HAVKEYW( "On" )
                    )
                    {
                        wMsgLevel = TRUE;
                    }
                    else if (HAVKEYW( "reset" ))
                    {
                        wMsgLevel = DEFAULT_MSGLEVEL;
                    }
                    else
                    {
                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;

                case _NEEDMSGPREF:
                {
                    if (0
                        || HAVKEYW( "Disable" )
                        || HAVKEYW( "Off" )
                    )
                    {
                        wMsgPrefix = FALSE;
                    }
                    else if (0
                        || HAVKEYW( "Enable" )
                        || HAVKEYW( "On" )
                    )
                    {
                        wMsgPrefix = TRUE;
                    }
                    else if (HAVKEYW( "reset" ))
                    {
                        wMsgPrefix = DEFAULT_MSGPREFIX;
                    }
                    else
                    {
                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;

                case _NEEDERRPREF:
                {
                    if (0
                        || HAVKEYW( "Disable" )
                        || HAVKEYW( "Off" )
                    )
                    {
                        wErrPrefix = FALSE;
                    }
                    else if (0
                        || HAVKEYW( "Enable" )
                        || HAVKEYW( "On" )
                    )
                    {
                        wErrPrefix = TRUE;
                    }
                    else if (HAVKEYW( "reset" ))
                    {
                        wErrPrefix = DEFAULT_ERRPREFIX;
                    }
                    else
                    {
                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;

                case _NEEDMODE:
                {
                    if (argl >= 3 && argl <= 7 && HAVABBR( "Command" ))
                    {
                        wMode = MODE_COMMAND;
                    }
                    else if (argl >= 3 && argl <= 10 && HAVABBR( "Subroutine" ))
                    {
                        wMode = MODE_SUBROUTINE;
                    }
                    else if (HAVKEYW( "reset" ))
                    {
                        wMode = DefaultMode;
                    }
                    else
                    {
                        // "REXX(%s) Option %s value '%s' is invalid"
                        WRMSG( HHC17509, "E", PackageName,
                            ARGSDESC[ whatValue ], argv[ iarg ]);
                        return -1;
                    }
                }
                break;
            }
            // end switch (whatValue)

            whatValue = 0;
            continue;
        }
    }
    // end for (iarg=1; iarg < argc; iarg++)

    if (whatValue == _NEEDPACKAGE)
    {
        // (they entered "rexx start" without specifying package)
        wPackage = DefaultPackage;
    }
    else if (whatValue > 0)
    {
        // "REXX(%s) Option '%s' needs a value"
        WRMSG( HHC17507, "E", PackageName, ARGSDESC[ whatValue ]);
        return -1;
    }

    //----------------------------------------------------------------
    // Option parsing complete. Process their requested options.
    //----------------------------------------------------------------

    if (haveMode)
        RexxMode = wMode;

    if (haveMsgLevel)
        MsgLevel = wMsgLevel;

    if (haveMsgPrefix)
        MsgPrefix = wMsgPrefix;

    if (haveErrPrefix)
        ErrPrefix = wErrPrefix;

    if (haveResolver)
        useResolver = wResolver;

    if (haveSysPath)
        useSysPath = wSysPath;

    if (havePath)
        InitializePaths( wPath );

    if (haveExtensions)
        InitializeExtensions( wExtensions );

    /* Disable/Enable the Rexx package, as needed */
    if (haveDisable)
    {
        if (IsEnabled( NULL ))
        {
            char* saved_pkgname = PackageName;
            Disable();
            // "REXX(%s) Rexx has been stopped/disabled"
            WRMSG( HHC17526, "I", saved_pkgname );
        }
        else
        {
            // "REXX(%s) Rexx already stopped/disabled"
            WRMSG( HHC17523, "W", PackageName );
        }
    }
    else if (haveEnable)
    {
        if (IsDisabled())
        {
            // Enable function will report version/source, etc.
            if (Enable( wPackage ) == 0)
                DisplayOptions();
        }
        else
        {
            // "REXX(%s) Rexx already started/enabled"
            WRMSG( HHC17522, "W", PackageName );
        }
    }

    return 0;

} // end rexx_cmd

/*-------------------------------------------------------------------*/
/* Display current options                                           */
/*-------------------------------------------------------------------*/
static void DisplayOptions()
{
    char buf[ 256 ] = {0};

    // "REXX(%s) %s"
    MSGBUF( buf, "Mode            : %s", RexxMode == MODE_COMMAND ? "Command" : "Subroutine" );
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));

    // "REXX(%s) %s"
    MSGBUF( buf, "MsgLevel        : %s", MsgLevel ? "On" : "Off" );
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));

    // "REXX(%s) %s"
    MSGBUF( buf, "MsgPrefix       : %s", MsgPrefix ? "On" : "Off" );
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));

    // "REXX(%s) %s"
    MSGBUF( buf, "ErrPrefix       : %s", ErrPrefix ? "On" : "Off" );
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));

    // "REXX(%s) %s"
    MSGBUF( buf, "Resolver        : %s", useResolver ? "On" : "Off" );
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));

    // "REXX(%s) %s"
    MSGBUF( buf, "SysPath    (%2d) : %s", SysPathCount, useSysPath ? "On" : "Off" );
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));

    MSGBUF( buf,"RexxPath   (%2d) : %s", RexxPathCount, RexxPathCount ? RexxPath : "" );
    // "REXX(%s) %s"
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));

    // NOTE: we use "ExtensionsCount-1" because the
    // first entry is our dummy empty-string entry.

    MSGBUF( buf, "Extensions (%2d) : %s", ExtensionsCount-1, ExtensionsCount > 1 ? Extensions : "" );
    // "REXX(%s) %s"
    WRMSG( HHC17500, "I", PackageName, RTRIM( buf ));
}

/*-------------------------------------------------------------------*/
/* Thread to run Rexx script in the background                       */
/*-------------------------------------------------------------------*/
static void* exec_async_thread( void* p )
{
    SCRCTL*  pCtl;                  // Pointer to our SCRCTL entry
    void*    args;                  // (depends on mode)
    int      argc;                  // (depends on mode)
    int      rc;                    // ExecCmd/ExecSub return code
    char*    cmd_cmdline = NULL;    // (MODE_COMMAND script cmdline)

    /* Retrieve needed arguments */
    obtain_lock( &scr_lock );
    {
        THRARG* arg = p;

        pCtl  = arg->scr_ctl;
        args  = arg->scr_args;
        argc  = arg->scr_numargs;

        pCtl->scr_ended = FALSE;

        //* Make copy of MODE_COMMAND cmdline before it disappears! */
        if (pCtl->scr_mode == MODE_COMMAND)
            cmd_cmdline = args ? strdup( args ) : NULL;

        /* Indicate arguments retrieved */
        signal_condition( &scr_cond );
    }
    release_lock( &scr_lock );

    /* Execute Rexx script asynchronously */
    if (pCtl->scr_mode == MODE_COMMAND)
    {
        rc = ExecCmd( pCtl->scr_name, cmd_cmdline );
        free( cmd_cmdline );
    }
    else // (pCtl->scr_mode == MODE_SUBROUTINE)
    {
        rc = ExecSub( pCtl->scr_name, argc, args );
    }

    /* Indicate script has ended */
    obtain_lock( &scr_lock );
    {
        pCtl->scr_ended = TRUE;
    }
    release_lock( &scr_lock );

    return INT2VOIDP( rc );    // (in case anyone is interested)
}

/*-------------------------------------------------------------------*/
/* Execute a Rexx script asyncronously                               */
/*-------------------------------------------------------------------*/
static int exec_async
(
    char*  scr_name,
    BYTE   scr_mode,
    void*  scr_args,
    int    scr_numargs
)
{
    int     rc;
    THRARG  thr_args;

    obtain_lock( &scr_lock );
    {
        TID dummy = 0;      // temp for NewSCRCTL()

        /* Initialize THRARG structure. Note that the SCRCTL struct's
           'tid' field (thr_args.scr_ctl->scr_tid) will be initially
           created with a null (0) dummy value but will get fixed
           automatically whenever the exec_async_thread is created.
        */
        thr_args.scr_args    = scr_args;
        thr_args.scr_numargs = scr_numargs;
        thr_args.scr_ctl     = NewSCRCTL( scr_name, dummy, scr_mode );

        if (!thr_args.scr_ctl)
            rc = -1; // (out of memory)
        else
        {
            /* Create async thread and update thread-id */
            rc = create_thread( &thr_args.scr_ctl->scr_tid, DETACHED,
                      exec_async_thread, &thr_args, scr_name );

            if (rc != 0)
            {
               // "Error in function create_thread(): %s"
               WRMSG( HHC00102, "E", strerror( rc ));
               FreeSCRCTL( thr_args.scr_ctl );
            }
            else
            {
                /* Wait for thread to initialize */
                wait_condition( &scr_cond, &scr_lock );

                /* Add this script to our list */
                InsertListTail( &scr_list, &thr_args.scr_ctl->link );
            }
        }
    }
    release_lock( &scr_lock );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Halt an executing Rexx script                  (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE CancelRexxExec( TID tid )
{
    BYTE rc = TRUE;

    obtain_lock( &scr_lock );
    {
        SCRCTL*  pCtl;
        BYTE     cur_pkg;

        if (!(pCtl = FindSCRCTL( tid )))
        {
            // "Asynchronous Rexx script "TIDPAT" not found"
            WRMSG( HHC17517, "E", TID_CAST( tid ));
            release_lock( &scr_lock );
            return FALSE;
        }

        /* Save current package */
        cur_pkg = PkgName2Num( PackageName );

        /* Temporarily enable same package as script's */
        if (!EnablePkg( pCtl->scr_pkg, FALSE ))
        {
            // "REXX(%s) Could not enable %sRexx package"
            WRMSG( HHC17511, "E", "", "temporary " );
            rc = FALSE;
        }
        else
        {
            /* Now ask Rexx to HALT that script */
            if (HaltExec( getpid(), pCtl->scr_tid ) != 0)
            {
                // "REXX(%s) Signal HALT "TIDPAT" %s"
                WRMSG( HHC17520, "E", PkgNum2Name( pCtl->scr_pkg ),
                    TID_CAST( pCtl->scr_tid ), "failed" );
                rc = FALSE;
            }
        }

        /* Re-enable the original package */
        if (!EnablePkg( cur_pkg, FALSE ))
        {
            // "REXX(%s) Could not enable %sRexx package"
            WRMSG( HHC17511, "E", PackageName, "original " );
            rc = FALSE;
        }
    }
    release_lock( &scr_lock );

    return rc;
}

/*-------------------------------------------------------------------*/
/* Halt an ALL asynchronous Rexx scripts          (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE CancelAllRexxExecs()
{
    BYTE rc = TRUE;

    obtain_lock( &scr_lock );
    {
        LIST_ENTRY*  pLink    = NULL;
        SCRCTL*      pCtl     = NULL;
        BYTE         cur_pkg  = PkgName2Num( PackageName );

        for (pLink = scr_list.Flink; pLink != &scr_list; pLink = pLink->Flink)
        {
            pCtl = CONTAINING_RECORD( pLink, SCRCTL, link );

            /* Temporarily enable same package as script's */
            if (!EnablePkg( pCtl->scr_pkg, FALSE ))
            {
                // "REXX(%s) Could not enable %sRexx package"
                WRMSG( HHC17511, "E", "", "temporary " );
                rc = FALSE;
            }
            else
            {
                /* Now ask Rexx to HALT that script */
                if (HaltExec( getpid(), pCtl->scr_tid ) != 0)
                {
                    // "REXX(%s) Signal HALT "TIDPAT" %s"
                    WRMSG( HHC17520, "E", PkgNum2Name( pCtl->scr_pkg ),
                        TID_CAST( pCtl->scr_tid ), "failed" );
                    rc = FALSE;
                }
            }
        }

        /* Re-enable the original package */
        if (!EnablePkg( cur_pkg, FALSE ))
        {
            // "REXX(%s) Could not enable %sRexx package"
            WRMSG( HHC17511, "E", PackageName, "original " );
            rc = FALSE;
        }

    }
    release_lock( &scr_lock );

    return rc;
}

/*-------------------------------------------------------------------*/
/* exec Command - execute a rexx script                              */
/*-------------------------------------------------------------------*/
int exec_cmd( int argc, char* argv[], char* cmdline )
{
    char* pscriptname;
    char  scriptname[ MAX_FULLPATH_LEN ];
    struct stat fstat;
    int   rc, mode, iarg, i, n;

    BYTE  haveMode  = FALSE;
    BYTE  found     = FALSE;
    BYTE  async     = FALSE;

    if (!(sysblk.shcmdopt & SHCMDOPT_ENABLE))
    {
        // "Shell/Exec commands are disabled"
        WRMSG( HHC02227, "E" );
        return -1;
    }

    /*-----------------------------------------------------------------

                          -------------------
                            PROGRAMMING NOTE
                          -------------------


      argv[0] is the Hercules command ("exec").

      argv[1] is the optional execution mode ("sub" or "cmd").

      argv[2] (or argv[1] if the optional mode wasn't specified)
              is the name of the Rexx script to be executed.

      argv[3] (or argv[2] if the optional mode wasn't specified)
              through "argv[argc-1]" are the script's arguments.

      When the script is called in subroutine mode, it is passed
      multiple arguments, so our job is easy: we simply call the
      "ExecSub" function passing it argv[3] (or argv[2]) through
      argv[argc-1].

                          *******************
                          **   IMPORTANT!  **
                          *******************

      When the script is called in command mode however, then we
      call "ExecCmd" and pass it just a single argument: the re-
      mainder of the unparsed command-line immediately following
      the script name, passed as a single long string.

      This means we need to pass it a pointer to the above "cmdline"
      argument that was passed to us, but indexed to where argv[2]
      (or argv[1]) begins, i.e. past the command name ("exec") and
      optional execution mode ("sub" or "cmd") and script name arg-
      uments, to where the argument immediately following the script
      name actually begins within the "cmdline" string.

      This means the "argv" array passed to us MUST correspond to
      a proper Hercules command parsing of the "cmdline" string.
      That is to say, you should NEVER call the "exec_cmd" function
      directly yourself.  Instead, you should call "panel_command"
      and let it call this function as expected.

      Refer to the "(mode == MODE_COMMAND)" statement block further
      below for details regarding this requirement.

    -----------------------------------------------------------------*/

    /* Make sure the Rexx Package is enabled for use */
    if (IsDisabled())
    {
        if (Enable( NULL ) != 0)
            return -1;
    }

    /* At the very minimum we need to have a script name */
    if (argc < 2)
    {
        // "REXX(%s) Exec: script name not specified"
        WRMSG( HHC17505, "E", PackageName  );
        return -1;
    }

    /* If they specified an execution mode on their 'exec' command,
       then after honoring it, remove it from the arguments array
       (but KEEP argv[0], the
    */
    mode = RexxMode;
    iarg = 1;

    if (0
        || HAVKEYW( "cmd"  ) || HAVKEYW( "com"  )
        || HAVKEYW( "-cmd" ) || HAVKEYW( "-com" )
        || HAVKEYW( "/cmd" ) || HAVKEYW( "/com" )
    )
    {
        mode = MODE_COMMAND;
        haveMode = TRUE;
    }
    else if (0
        || HAVKEYW( "sub"  )
        || HAVKEYW( "-sub" )
        || HAVKEYW( "/sub" )
    )
    {
        mode = MODE_SUBROUTINE;
        haveMode = TRUE;
    }

    /* Remove the script execution mode option from the command line */
    if (haveMode)
    {
        for (i=2; i < argc; i++)
            argv[ i-1 ] = argv[ i ];
        --argc;
    }

    /*                  AT THIS POINT...

       argv[0] is still the Hercules command ("exec").
       argv[1] is now the name of the Rexx script to be executed.
       argv[2] to argv[argc-1] are now the script's arguments (if any)
    */

    /* Check for asynchronous execution option ("&" as last arg) */
    if (argc > 2 && strcmp( argv[ argc-1 ], "&" ) == 0)
    {
        argc--; // (drop last argument)
        async = TRUE;
    }

    /* Save the name of the script they want to execute */
    pscriptname = argv[1];
    STRLCPY( scriptname, pscriptname );

    /* If the resolver option isn't enabled, use their name as-is */
    found = TRUE;
    if (!useResolver)
        goto resolved;
    found = FALSE;

    /* Otherwise we need to resolve the script name's full path.
       As the first attempt, use the scriptname exactly as they
       specified it.
    */
    rc = stat( pscriptname, &fstat );

    if (rc == 0 && S_ISREG( fstat.st_mode ))
    {
        found = TRUE;
        goto resolved;
    }

    /* If they specified a specific location then
       it obviously doesn't doesn't exist there.
    */
    if (strcmp( basename( scriptname ), pscriptname ) != 0)
        goto resolved;

    /* Otherwise it's only a scriptname without a path (and maybe
       without any extension either). Search the defined Rexx paths
       using no extension followed by each of the defined extensions.
       The first entry in our extensions array is an empty string
       to cause us to always try their name without any extension
       first, followed by all the remaining defined extensions.
    */
    if (RexxPathCount)
    {
        /* Check each directory in this list... */
        for (i = 0; i < RexxPathCount; i++)
        {
            /* Does this directory exist? */
            rc = stat( RexxPathArray[ i ], &fstat );
            if (rc != 0 || !S_ISDIR( fstat.st_mode ))
                continue;

            /* This directory does exist. Check to see if the script
               is in this directory by trying each defined extension.
            */
            for (n = 0; n < ExtensionsCount; n++)
            {
                /* Build full path scriptname with this extension */
                MSGBUF( scriptname, PATHFORMAT, RexxPathArray[ i ],
                    pscriptname, ExtensionsArray[ n ]);

                /* Is it in this directory with this extension? */
                rc = stat( scriptname, &fstat );
                if (rc == 0 && S_ISREG( fstat.st_mode ))
                {
                    found = TRUE;
                    goto resolved;
                }
            }
        }
    }

    if (useSysPath && SysPathCount)
    {
        /* Check each directory in this list... */
        for (i = 0; i < SysPathCount; i++)
        {
            /* Does this directory exist? */
            rc = stat( SysPathArray[ i ], &fstat );
            if (rc != 0 || !S_ISDIR( fstat.st_mode ))
                continue;

            /* This directory does exist. Check to see if the script
               is in this directory by trying each defined extension.
            */
            for (n = 0; n < ExtensionsCount; n++)
            {
                /* Build full path scriptname with this extension */
                MSGBUF( scriptname, PATHFORMAT, SysPathArray[ i ],
                    pscriptname, ExtensionsArray[ n ]);

                /* Is it in this directory with this extension? */
                rc = stat( scriptname, &fstat );
                if (rc == 0 && S_ISREG( fstat.st_mode ))
                {
                    found = TRUE;
                    goto resolved;
                }
            }
        }
    }

resolved:

    if (!found)
    {
        // "REXX(%s) Exec: script \"%s\" not found"
        WRMSG( HHC17506, "E", PackageName, pscriptname );
        return -1;
    }

    /* Call the proper Rexx package function to execute their script.

                         AT THIS POINT...

       argv[0] is still the Hercules command ("exec").
       argv[1] is still the name of the Rexx script to be executed.
       argv[2] to argv[argc-1] are still the script's arguments.
    */
    if (mode == MODE_SUBROUTINE)
    {
        /* The arguments passed to a Rexx script called in subroutine
           mode is the array of individual arguments (if any) following
           the script name.  This is easy since they've already been
           parsed for us by the Hercules's command-line parser, so we
           just pass them to the script as-is.
        */

        int     num_sub_args;       // (number of script arguments)
        char**  sub_args_array;     // (array of script arguments)

        if (argc > 2)
        {
            num_sub_args   = (argc - 2);
            sub_args_array = &argv[  2 ];
        }
        else
        {
            num_sub_args   = 0;
            sub_args_array = NULL;
        }

        if (!async)
        {
            /* Execute Rexx script synchronously */
            rc = ExecSub( scriptname, num_sub_args, sub_args_array );
        }
        else
        {
            /* Execute Rexx script asynchronously */
            rc = exec_async( scriptname, mode, sub_args_array, num_sub_args );
        }
    }
    else // (mode == MODE_COMMAND)
    {
        /*-------------------------------------------------------------

                             PROGRAMMING NOTE

           The only argument to a Rexx script called in command mode
           is the remainder of the command-line arguments immediately
           following the script name, passed as a single string, which
           is more difficult.  We can't simply build a string from the
           individual pre-parsed arguments passed to us as the result
           may not exactly match what was entered on the command line
           (e.g. we cannot presume multiple blanks between arguments
           are not important to the Rexx script being called).  Thus
           we instead simply point to the original unmodified Hercules
           "cmdline" string indexed to where the script's argument(s)
           string actully begins.

        -------------------------------------------------------------*/

        char* script_argstring;         // (script's only argument)

        if (argc < 3 || !cmdline)
            script_argstring = NULL;    // (no arguments)
        else
        {
            char*   execcmd;            // (herc cmdline "exec" token)
            char*   script_arg;         // (arg following script name)
            size_t  script_arg_idx;     // (index to script_argstring)

            execcmd        = argv[0];
            script_arg     = argv[2];
            script_arg_idx = (script_arg - execcmd);

            /* Check we were called properly: 4+1+len+1="exec name " */
            if (0
                || script_arg_idx < (4 + 1 + strlen( scriptname ) + 1)
                || script_arg_idx >= strlen( cmdline )
            )
            {
                // "REXX(%s) Malformed call to 'exec_cmd' function"
                WRMSG( HHC17513, "E", PackageName );
                return -1;
            }

            /* Left-trim the cmdline so its first char is argv[0][0] */
            cmdline = LTRIM( cmdline );

            /* If the first argument contains any blanks then it was
               originally surrounded by quotes which were removed by
               Hercules command-line parsing, so we need to backup by
               one position to point to the double quote where that
               argument actually began.
            */
            if (strchr( script_arg, ' ' ))
                --script_arg_idx;

            script_argstring = cmdline + script_arg_idx;
        }

        if (!async)
        {
            /* Execute Rexx script synchronously */
            rc = ExecCmd( scriptname, script_argstring );
        }
        else
        {
            /* Remove the " &&" async argument from script's cmdline.
               PLEASE NOTE that we *MUST* keep any resulting trailing
               blanks that might result since for command mode scripts
               we mustn't presume they aren't significant! (Refer to
               the PROGRAMMING NOTE further above for justification!)
            */
            if (script_argstring)
                script_argstring[ strlen( script_argstring ) - 3 ] = 0;

            /* Execute Rexx script asynchronously */
            rc = exec_async( scriptname, mode, script_argstring, 0 );
        }
    }

    return rc;

} // end exec_cmd

#endif // HAVE_REXX
