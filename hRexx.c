/* HREXX.C      (c)Copyright Enrico Sorichetti, 2012                 */
/*              Rexx Interpreter Support                             */
/*                                                                   */
/*  Released under "The Q Public License Version 1"                  */
/*  (http://www.hercules-390.org/herclic.html) as modifications to   */
/*  Hercules.                                                        */

/*  inspired by the previous Rexx implementation by Jan Jaeger       */

#include "hstdinc.h"

#ifndef _HREXX_C_
#define _HREXX_C_

#define _HENGINE_DLL_
#include "hercules.h"

#if defined(ENABLE_OBJECT_REXX) || defined(ENABLE_REGINA_REXX)

#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)
#define REXX_TRY_OOREXX( _FOUND_ ) do \
{ \
    RexxPackage = OOREXX_PACKAGE; \
    RexxDynamicLoader = &ObjectRexxDynamicLoader; \
    RexxRegisterFunctions = &ObjectRexxRegisterFunctions; \
    RexxRegisterHandlers = &ObjectRexxRegisterHandlers; \
    RexxExecCmd = &ObjectRexxExecCmd; \
    RexxExecInstoreCmd = &ObjectRexxExecInstoreCmd; \
    RexxExecSub = &ObjectRexxExecSub; \
    rc = (*RexxDynamicLoader)(); \
    if ( rc == 0 ) \
    { \
        RexxStatus = _ENABLED_ ;\
        WRMSG( HHC17525, "I", RexxPackage); \
        goto _FOUND_ ; \
    } \
} while(0) ;

#define REXX_TRY_REGINA( _FOUND_ ) do \
{ \
    RexxPackage = REGINA_PACKAGE; \
    RexxDynamicLoader = &ReginaRexxDynamicLoader; \
    RexxRegisterFunctions = &ReginaRexxRegisterFunctions; \
    RexxRegisterHandlers = &ReginaRexxRegisterHandlers; \
    RexxExecCmd = &ReginaRexxExecCmd; \
    RexxExecInstoreCmd = &ReginaRexxExecInstoreCmd; \
    RexxExecSub = &ReginaRexxExecSub; \
    rc = (*RexxDynamicLoader)(); \
    if ( rc == 0 ) \
    { \
        RexxStatus = _ENABLED_ ;\
        WRMSG( HHC17525, "I", RexxPackage); \
        goto _FOUND_ ; \
    } \
} while(0) ;

#define SETREXX_RESET() do \
{ \
    RexxPackage = ""; \
    RexxDynamicLoader = NULL; \
    RexxRegisterFunctions = NULL; \
    RexxRegisterHandlers = NULL; \
    RexxExecCmd = NULL; \
    RexxExecInstoreCmd = NULL; \
    RexxExecSub = NULL; \
    RexxStatus = _DISABLED_ ; \
} while (0) ;
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */

#include "hRexx.h"

// forward references
int  exec_cmd(int, char  **, char* );
int  exec_instore_cmd(char *, char *);
void InitializePaths(char *);
void InitializeExtensions(char *);
void InitializeOptions(void);

#if defined(ENABLE_OBJECT_REXX)
int ObjectRexxDynamicLoader();
int ObjectRexxRegisterFunctions();
int ObjectRexxRegisterHandlers();
int ObjectRexxExecCmd();
int ObjectRexxExecInstoreCmd();
int ObjectRexxExecSub();
#endif
#if defined(ENABLE_REGINA_REXX)
int ReginaRexxDynamicLoader();
int ReginaRexxRegisterFunctions();
int ReginaRexxRegisterHandlers();
int ReginaRexxExecCmd();
int ReginaRexxExecInstoreCmd();
int ReginaRexxExecSub();
#endif

void *hRexxLibHandle = NULL;       /* Library handle */
void *hRexxApiLibHandle = NULL;    /* Api Library handle ooRexx*/

#if defined(ENABLE_OBJECT_REXX)  && !defined(ENABLE_REGINA_REXX)
char *RexxPackage = OOREXX_PACKAGE;
char *RexxLibrary = OOREXX_LIBRARY;
char *RexxApiLibrary = OOREXX_API_LIBRARY;
int (*RexxDynamicLoader)() = &ObjectRexxDynamicLoader;
int (*RexxRegisterFunctions)() = &ObjectRexxRegisterFunctions;
int (*RexxRegisterHandlers)() = &ObjectRexxRegisterHandlers;
int (*RexxExecCmd)() = &ObjectRexxExecCmd;
int (*RexxExecInstoreCmd)() = &ObjectRexxExecInstoreCmd;
int (*RexxExecSub)() = &ObjectRexxExecSub;

#elif !defined(ENABLE_OBJECT_REXX) &&  defined(ENABLE_REGINA_REXX)
char *RexxPackage = REGINA_PACKAGE;
char *RexxLibrary = REGINA_LIBRARY;
char *RexxApiLibrary = "";
int (*RexxDynamicLoader)() = &ReginaRexxDynamicLoader;
int (*RexxRegisterFunctions)() = &ReginaRexxRegisterFunctions;
int (*RexxRegisterHandlers)() = &ReginaRexxRegisterHandlers;
int (*RexxExecCmd)() = &ReginaRexxExecCmd;
int (*RexxExecInstoreCmd)() = &ReginaRexxExecInstoreCmd;
int (*RexxExecSub)() = &ReginaRexxExecSub;

#else
char *RexxPackage = "";
char *RexxLibrary = "";
char *RexxApiLibrary = "";
int (*RexxDynamicLoader)() = NULL;
int (*RexxRegisterFunctions)() = NULL;
int (*RexxRegisterHandlers)() = NULL;
int (*RexxExecCmd)() = NULL;
int (*RexxExecInstoreCmd)() = NULL;
int (*RexxExecSub)() = NULL;

#endif

int    PathsInitialized = FALSE;

int    useRexxPath = TRUE;
char  *RexxPath = NULL;
char **RexxPathArray=NULL;
int    RexxPathMax = 0;
int    RexxPathCount = 0;

int    useSysPath = TRUE;
char  *SysPath = NULL;
char **SysPathArray=NULL;
int    SysPathMax = 0;
int    SysPathCount = 0;

int    ExtensionsInitialized = FALSE;
char  *Extensions = NULL;
char **ExtensionsArray=NULL;
int    ExtensionsMax = 0;
int    ExtensionsCount = 0;

int    OptionsInitialized = FALSE;
int    useResolver = TRUE;
int    MessageLevel = 0;
char  *MessagePrefix = NULL;
char  *ErrorPrefix = NULL;

#define _COMMAND_ 1
#define _SUBROUTINE_ 2
int    RexxMode = _COMMAND_ ;

#define _ENABLED_ 0
#define _DISABLED_ 1
int    RexxStatus = _ENABLED_ ;

/*-------------------------------------------------------------------*/
/* rexx Command - manage the Rexx environment                        */
/*-------------------------------------------------------------------*/
int rexx_cmd(int argc, char *argv[], char *CommandLine)
{
char *ARGSDESC[] = {"", "Start/Enable", "Path", "SysPath", "Extensions", "Resolver", "MsgLevel", "MsgPrefix", "ErrPrefix", "Mode"};
char *PACKAGES[] = {"", "ooRexx", "Regina" };

enum
{
    _NEEDPACKAGE = 1 ,
    _NEEDPATH ,
    _NEEDSYSPATH ,
    _NEEDEXTENSIONS ,
    _NEEDRESOLVER ,
    _NEEDMSGLEVL ,
    _NEEDMSGPREF ,
    _NEEDERRPREF ,
    _NEEDMODE ,
    _NEEDz
} ;

int   i;
int   rc = 0;
short RetRC = 0;
char  Result[1024];
char *ptr,*nxt;
char temp[33];
int  iarg,argl;

int  whatValue = 0;

int  haveEnable = FALSE;
int  haveDisable = FALSE;

int  havePath = FALSE;
int  haveSysPath = FALSE;
int  haveExtensions = FALSE;
int  haveResolver = FALSE;
int  haveMsgLevl = FALSE;
int  haveMsgPref = FALSE;
int  haveErrPref = FALSE;
int  haveMode = FALSE;

char *wPath = NULL;
int   wSysPath = TRUE;
char *wExtensions = NULL;
int   wResolver = TRUE;
int   wMsgLevl = 0;
char *wMsgPref = NULL;
char *wErrPref = NULL;
char *wPackage = NULL;
int   wRexxMode = 0;

    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(CommandLine);

    UNREFERENCED(haveDisable);
    UNREFERENCED(haveEnable);
    UNREFERENCED(*PACKAGES);

    UNREFERENCED(rc);
    UNREFERENCED(RetRC);

    if ( argc >= 2 )
    for (iarg=1; iarg<argc; iarg++)
    {
        argl = strlen(argv[iarg]);

        if ( whatValue > 0 )
        {
            switch ( whatValue )
            {
                case _NEEDPACKAGE :
                {
                    if ( HAVKEYW( REGINA_PACKAGE ) || HAVKEYW( OOREXX_PACKAGE ) )
                        wPackage = strdup(argv[iarg]);
                    else
                    {
                        WRMSG( HHC17509, "W", RexxPackage, argv[iarg], ARGSDESC[_NEEDPACKAGE]);
                        return -1;
                    }
                }
                break;

                case _NEEDPATH :
                {
                    wPath = strdup(argv[iarg]);
                }
                break;

                case _NEEDSYSPATH :
                {
                    if ( HAVKEYW( "reset" ) )
                    {
                        wSysPath = TRUE;
                    }
                    else if ( HAVKEYW( "enable" ) || HAVKEYW( "on" ) )
                    {
                        wSysPath = TRUE;
                    }
                    else if ( HAVKEYW( "disable" ) || HAVKEYW( "off" ) )
                    {
                        wSysPath = FALSE;
                    }
                    else
                    {
                        WRMSG( HHC17510, "W", RexxPackage, argv[iarg], (int)strlen(argv[iarg]), ARGSDESC[_NEEDSYSPATH]);
                        return -1;
                    }
                }
                break;

                case _NEEDEXTENSIONS :
                {
                    wExtensions = strdup(argv[iarg]);
                }
                break;

                case _NEEDRESOLVER :
                {
                    if ( HAVKEYW( "reset" ) )
                    {
                        wResolver = TRUE;
                    }
                    else if ( HAVKEYW( "enable" ) || HAVKEYW( "on" ) )
                    {
                        wResolver = TRUE;
                    }
                    else if ( HAVKEYW( "disable" ) || HAVKEYW( "off" ) )
                    {
                        wResolver = FALSE;
                    }
                    else
                    {
                        WRMSG( HHC17510, "W", RexxPackage, argv[iarg], (int)strlen(argv[iarg]), ARGSDESC[_NEEDRESOLVER]);
                        return -1;
                    }
                }
                break;

                case _NEEDMSGLEVL :
                {
                    if ( HAVKEYW( "reset" ) )
                    {
                        wMsgLevl = 0;
                    }
                    else
                    {
                        ptr = argv[iarg];
                        errno = 0;
                        wMsgLevl = (int) strtoul(argv[iarg],&nxt,10);
                        if (errno != 0 || nxt == ptr || *nxt != 0 || ( wMsgLevl < 0 || wMsgLevl > 9 ) )
                        {
                            WRMSG( HHC17510, "W", RexxPackage, argv[iarg], (int)strlen(argv[iarg]), ARGSDESC[_NEEDMSGLEVL]);
                            return -1;
                        }
                    }
                }
                break;

                case _NEEDMSGPREF :
                {
                    if ( strlen(argv[iarg]) > 9 )
                    {
                        WRMSG( HHC17510, "W", RexxPackage, argv[iarg], (int)strlen(argv[iarg]), ARGSDESC[_NEEDMSGPREF]);
                        return -1;
                    }
                    wMsgPref = strdup(argv[iarg]);
                }
                break;

                case _NEEDERRPREF :
                {
                    if ( strlen(argv[iarg]) > 9 )
                    {
                        WRMSG( HHC17510, "W", RexxPackage, argv[iarg], (int)strlen(argv[iarg]), ARGSDESC[_NEEDERRPREF]);
                        return -1;
                    }
                    wErrPref = strdup(argv[iarg]);
                }
                break;

                case _NEEDMODE :
                {
                    if ( argl >= 3 && argl <= 7 && HAVABBR( "command" ) )
                    {
                        wRexxMode = _COMMAND_;
                    }
                    else if ( argl >= 3 && argl <= 10 && HAVABBR( "subroutine" ) )
                    {
                        wRexxMode = _SUBROUTINE_;
                    }
                    else
                    {
                        WRMSG( HHC17510, "W", RexxPackage, argv[iarg], (int)strlen(argv[iarg]), ARGSDESC[_NEEDMODE]);
                        return -1;
                    }
                }
                break;

            }
            whatValue = 0;
            continue;
        }

        if ( !haveMode && HAVKEYW( "mode" ) )
        {
            haveMode = TRUE;
            whatValue = _NEEDMODE;
            continue;
        }

        if ( !haveDisable &&
             ( ( argl >= 3 && argl <= 7 && HAVABBR( "disable" ) ) ||
               ( argl >= 3 && argl <= 4 && HAVABBR( "stop"    ) ) ) )
        {
#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)
            haveDisable = TRUE;
            continue;
#else  /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */
            WRMSG( HHC17524, "I", RexxPackage);
            return 0;
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */
        }

        if ( !havePath &&
           ( ( argl >= 4 && argl <=  5 && HAVABBR( "paths"     ) ) ||
             ( argl >= 5 && argl <=  9 && HAVABBR( "rexxpaths" ) ) ) )
        {
            havePath = TRUE;
            whatValue = _NEEDPATH;
            continue;
        }

        if ( !haveSysPath &&
             ( argl >= 4 && argl <=  7 && HAVABBR( "syspath" ) ) )
        {
            haveSysPath = TRUE;
            whatValue = _NEEDSYSPATH;
            continue;
        }

        if ( !haveExtensions &&
           ( ( argl >= 3 && argl <= 10 && HAVABBR( "extensions" ) ) ||
             ( argl >= 3 && argl <=  8 && HAVABBR( "suffixes"   ) ) ) )
        {
            haveExtensions = TRUE;
            whatValue = _NEEDEXTENSIONS;
            continue;
        }

        if ( !haveResolver &&
             ( argl >= 5 && argl <=  8 && HAVABBR( "resolver" ) ) )
        {
            haveResolver = TRUE;
            whatValue = _NEEDRESOLVER;
            continue;
        }

        if ( !haveMsgLevl &&
             ( argl >= 4 && argl <=  8 && HAVABBR( "msglevel" ) ) )
        {
            haveMsgLevl = TRUE;
            whatValue = _NEEDMSGLEVL;
            continue;
        }

        if ( !haveMsgPref &&
             ( argl >= 4 && argl <=  9 && HAVABBR( "msgprefix" ) ) )
        {
            haveMsgPref = TRUE;
            whatValue = _NEEDMSGPREF;
            continue;
        }

        if ( !haveErrPref &&
             ( argl >= 4 && argl <=  9 && HAVABBR( "errprefix" ) ) )
        {
            haveErrPref = TRUE;
            whatValue = _NEEDERRPREF;
            continue;
        }

        if ( !haveEnable &&
           ( ( argl >= 3 && argl <=  6 && HAVABBR( "enable" ) ) ||
             ( argl >= 3 && argl <=  5 && HAVABBR( "start"  ) ) ) )
        {
#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)
            if ( hRexxLibHandle )
            {
                WRMSG( HHC17522, "I", RexxPackage);
                return -1;
            }
            haveEnable= TRUE;
            whatValue = _NEEDPACKAGE;
            continue;
#else /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */
            WRMSG( HHC17523, "I", RexxPackage);
            return 0;
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */
        }

        WRMSG( HHC17508, "W", RexxPackage, argv[iarg], iarg + 1 );
        return 0;
    }

    if ( whatValue == _NEEDPACKAGE )
    {
        wPackage = "auto";
    }
    else if ( whatValue > 0 )
    {
        WRMSG( HHC17507, "W", RexxPackage, ARGSDESC[whatValue] );
        return -1;
    }

    if ( haveMode )
    {
        RexxMode = wRexxMode;
    }

#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)
    if ( haveDisable )
    {
        if  ( hRexxLibHandle )
            HDLCLOSE( hRexxLibHandle);

        if  ( hRexxApiLibHandle )
            HDLCLOSE( hRexxApiLibHandle);

        RexxStatus = _DISABLED_ ;

        SETREXX_RESET()

        WRMSG( HHC17526, "I", RexxPackage);

    }
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */

    if ( havePath )
    {
        if ( strcasecmp( "reset", wPath) == 0 )
        {
            if (wPath)
            {
                free(wPath);
                wPath = NULL;
            }
            if (RexxPath)
            {
                free(RexxPath);
                RexxPath = NULL;
            }
            InitializePaths(NULL);
        }
        else
        {
            InitializePaths(wPath);
        }
    }

    if ( haveSysPath )
    {
        useSysPath = wSysPath;
    }

    if ( haveExtensions )
    {
        if ( strcasecmp( "reset", wExtensions) == 0 )
            InitializeExtensions(NULL);
        else
            InitializeExtensions(wExtensions);
    }

    if ( haveResolver )
    {
        useResolver = wResolver;
    }

    if ( haveMsgLevl )
    {
        MessageLevel = wMsgLevl;
    }

    if ( haveMsgPref )
    {
        if ( strcasecmp( "off", wMsgPref) == 0 || strcasecmp( "reset", wMsgPref) == 0 )
        {
            if ( MessagePrefix )
            {
                free(MessagePrefix);
                MessagePrefix = NULL;
            }
        }
        else
        {
            MessagePrefix = strdup(wMsgPref);
        }
        free(wMsgPref);
    }

    if ( haveErrPref )
    {
        if ( strcasecmp( "off", wErrPref) == 0 || strcasecmp( "reset", wErrPref) == 0 )
        {
            if ( ErrorPrefix )
            {
                free(ErrorPrefix);
                ErrorPrefix = NULL;
            }
        }
        else
        {
            ErrorPrefix = strdup(wErrPref);
        }
        free(wErrPref);
    }

    if ( haveMode )
    {
        RexxMode = wRexxMode;
    }

#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)
    if ( haveEnable )
    {
        if ( strcasecmp(wPackage, "auto" ) == 0  )
        {
            char *envvar;

            if ( !( envvar = getenv("HREXX_PACKAGE") ) )
            {
                REXX_TRY_OOREXX( Enable_Rexx_Loaded )
                REXX_TRY_REGINA( Enable_Rexx_Loaded )
                SETREXX_RESET()
                RexxStatus = _DISABLED_ ;
                WRMSG( HHC17526, "I", RexxPackage);
            }
            if ( strcasecmp(envvar, "auto" ) == 0 )
            {
                REXX_TRY_OOREXX( Enable_Rexx_Loaded )
                REXX_TRY_REGINA( Enable_Rexx_Loaded )
                SETREXX_RESET()
                RexxStatus = _DISABLED_ ;
                WRMSG( HHC17526, "I", RexxPackage);
            }

            if ( strcasecmp(envvar, "none" ) == 0 )
            {
                WRMSG( HHC17521, "I", "");
                return 0;
            }

            wPackage = envvar ;

        }

        if ( strcasecmp(wPackage, OOREXX_PACKAGE ) == 0  )
        {
            REXX_TRY_OOREXX( Enable_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
            return -1 ;
        }
        if ( strcasecmp(wPackage, REGINA_PACKAGE ) == 0  )
        {
            REXX_TRY_REGINA( Enable_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
            return -1 ;
        }

        WRMSG( HHC17501, "E", wPackage, "Oh shit, I should not be here");
        return -1;

    }
Enable_Rexx_Loaded:
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */

    if ( !PathsInitialized )
        InitializePaths(NULL);
    strlcpy(Result, "Rexx Path ", sizeof(Result));
    sprintf(temp,"(%2d) - ",RexxPathCount);
    strlcat(Result,temp,sizeof(Result));
    for (i=0; i<RexxPathCount; i++)
    {
        strlcat(Result,RexxPathArray[i],sizeof(Result));
        strlcat(Result,i == RexxPathCount-1 ? "" : PATHDELIM, sizeof(Result) );
    }
    WRMSG( HHC17500, "I", RexxPackage,Result);

    MSGBUF( Result, "Sys Path  (%2d) - %s ", SysPathCount, useSysPath ? "(ON)" : "(OFF)");
    WRMSG( HHC17500, "I", RexxPackage,Result);

    if ( !ExtensionsInitialized )
        InitializeExtensions(NULL);
    strcpy(Result, "Extensions");
    sprintf(temp,"(%2d) - ",ExtensionsCount);
    strlcat(Result,temp,sizeof(Result));
    for (i=0; i<ExtensionsCount; i++)
    {
        strlcat(Result, ExtensionsArray[i],sizeof(Result));
        strlcat(Result, i == ExtensionsCount-1 ? "" : EXTNDELIM, sizeof(Result) );
    }
    WRMSG( HHC17500, "I", RexxPackage,Result);

    if ( !OptionsInitialized )
        InitializeOptions();

    MSGBUF( Result, "Resolver       - %s",
            useResolver ? "(ON)" : "(OFF)" );
    WRMSG( HHC17500, "I", RexxPackage,Result);

    MSGBUF( Result, "Msg Level      - %d",
            MessageLevel);
    WRMSG( HHC17500, "I", RexxPackage,Result);

    MSGBUF( Result, "Msg Prefix     - %s",
            MessagePrefix ? MessagePrefix : "(OFF)" );
    WRMSG( HHC17500, "I", RexxPackage,Result);

    MSGBUF( Result, "Err Prefix     - %s",
            ErrorPrefix ? ErrorPrefix : "(OFF)" );
    WRMSG( HHC17500, "I", RexxPackage,Result);

    MSGBUF( Result, "Mode           - %s",
            ( RexxMode == _COMMAND_ ) ? "(Command)" : "(Subroutine)" );
    WRMSG( HHC17500, "I", RexxPackage,Result);

#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)
    if ( RexxStatus == _DISABLED_ )
    {
        WRMSG( HHC17521, "I","");
        return -1;
    }
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */

    rc = exec_instore_cmd("parse version _v;parse source  _s;return _v || '0a'x || _s;\n",Result);
    if  ( rc == 0 )
    {
        for (ptr = strtok(Result,"\n"); ptr; ptr = strtok(NULL, "\n"))
        {
            WRMSG( HHC17500, "I", RexxPackage, ptr);
        }
    }
    else
    {
        WRMSG( HHC17501, "I", RexxPackage, "Unexpected RC from instore script");
    }

    return 0;

}

/*-------------------------------------------------------------------*/
/* exec Command - execute a rexx script                              */
/*-------------------------------------------------------------------*/
int exec_cmd(int argc, char *argv[], char *CommandLine)
{
int   exec_cmd_rc;
int   wRexxMode, haveMode;
int   iarg,argl;
int   rc=0;
short RetRC = 0;
int   iPath,iExtn;
int   haveResolvedCommand = FALSE;

char  wCommand[MAX_FILENAME_LENGTH];
char *wArgs;

struct stat fstat;

    UNREFERENCED(rc);
    UNREFERENCED(RetRC);
    UNREFERENCED(CommandLine);

#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)

    if ( RexxStatus == _DISABLED_ )
    {
        WRMSG( HHC17521, "I", "");
        return -1 ;
    }
    if ( !hRexxLibHandle )
    {
        char *envvar;
        if ( !( envvar = getenv("HREXX_PACKAGE") ) )
        {
            REXX_TRY_OOREXX( exec_cmd_Rexx_Loaded )
            REXX_TRY_REGINA( exec_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        if ( strcasecmp(envvar, "auto" ) == 0 )
        {
            REXX_TRY_OOREXX( exec_cmd_Rexx_Loaded )
            REXX_TRY_REGINA( exec_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        if ( strcasecmp(envvar, "none" ) == 0 )
        {
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17521, "I", "");
            return 0;
        }
        if ( strcasecmp(envvar, OOREXX_PACKAGE ) == 0  )
        {
            REXX_TRY_OOREXX( exec_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        else if ( strcasecmp(envvar, REGINA_PACKAGE ) == 0  )
        {
            REXX_TRY_REGINA( exec_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        else
        {
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17501, "E", envvar,"Unknown Rexx Package");
            return -1;
        }

    }
exec_cmd_Rexx_Loaded:
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */

    if ( argc >= 2 )
    {
        iarg = 1 ;
        if ( HAVKEYW( "cmd" ) || HAVKEYW( "-cmd" ) || HAVKEYW( "/cmd" ) )
        {
            wRexxMode = _COMMAND_;
            haveMode = TRUE ;
        }
        else if ( HAVKEYW( "sub" ) || HAVKEYW( "-sub" ) || HAVKEYW( "/sub" ) )
        {
            wRexxMode = _SUBROUTINE_;
            haveMode = TRUE ;
        }
        else
        {
            wRexxMode = RexxMode;
            haveMode = FALSE ;
        }
        if ( haveMode == TRUE )
        {
            for ( iarg = 2; iarg < argc; iarg++)
                argv[iarg-1] = argv[iarg] ;
            argc = argc - 1 ;
        }
    }

    if (argc < 2)
    {
        WRMSG( HHC17505, "E", RexxPackage  );
        return -1;
    }

    strcpy(wCommand,argv[1]);

    if ( !PathsInitialized )
        InitializePaths(NULL);
    if ( !ExtensionsInitialized )
        InitializeExtensions(NULL);
    if ( !OptionsInitialized )
        InitializeOptions();

    if ( ! useResolver )
        goto skipResolver;

    haveResolvedCommand = FALSE;

    rc = stat( wCommand, &fstat );
    if ( ( rc == 0 ) && S_ISREG(fstat.st_mode) )
    {
        haveResolvedCommand = TRUE;
        goto endResolver;
    }

    if (strcmp(basename(wCommand),wCommand) != 0)
        goto endResolver;

    if ( RexxPathArray && useRexxPath )
    {
        for( iPath = 0; iPath<RexxPathCount; iPath++)
        {
            rc = stat( RexxPathArray[iPath] , &fstat );
            if ( ( rc != 0 ) || ( ! S_ISDIR(fstat.st_mode) ) )
                continue ;

            for( iExtn = 0; iExtn<ExtensionsCount; iExtn++)
            {
                sprintf(wCommand, PATHFORMAT, RexxPathArray[iPath], argv[1], ExtensionsArray[iExtn]);
                rc = stat( wCommand, &fstat );
                if ( ( rc == 0 ) && S_ISREG(fstat.st_mode) )
                {
                    haveResolvedCommand = TRUE;
                    goto endResolver;
                }
            }
        }
    }

    if ( SysPathArray && useSysPath )
    {
        for( iPath = 0; iPath<SysPathCount; iPath++)
        {
            rc = stat( SysPathArray[iPath] , &fstat );
            if ( ( rc != 0 ) || ( ! S_ISDIR(fstat.st_mode) ) )
                continue ;

            for( iExtn = 0;iExtn<ExtensionsCount;iExtn++)
            {
                sprintf(wCommand, PATHFORMAT, SysPathArray[iPath], argv[1], ExtensionsArray[iExtn]);
                rc = stat( wCommand, &fstat );
                if ( ( rc == 0 ) && S_ISREG(fstat.st_mode) )
                {
                    haveResolvedCommand = TRUE;
                    goto endResolver;
                }
            }
        }
    }

endResolver:
    if ( !haveResolvedCommand )
    {
        WRMSG( HHC17506, "I", RexxPackage, argv[1]);
        return -1;
    }

skipResolver:
    if ( wRexxMode == _COMMAND_ )
    {
        if (argc > 2)
        {
            for (argl = 0, iarg = 2; iarg < argc; iarg++)
                argl += (int)strlen(argv[iarg]) + 1;
            wArgs = malloc(argl);
            strlcpy(wArgs, argv[2],argl);
            for ( iarg = 3; iarg < argc; iarg++ )
            {
                strlcat( wArgs, " ", argl );
                strlcat( wArgs, argv[iarg], argl );
            }
        }
        else
            wArgs = NULL;

        exec_cmd_rc = (*RexxExecCmd)(wCommand, wArgs, argc, argv );

        if  ( wArgs )
        {
            free(wArgs);
        }

    }
    else if ( wRexxMode == _SUBROUTINE_ )
    {
        exec_cmd_rc = (*RexxExecSub)(wCommand, NULL, argc, argv);
    }
    else
        exec_cmd_rc = -1;

    return (exec_cmd_rc);
}

/*-------------------------------------------------------------------*/
/* exec Command - execute an Instore rexx script                     */
/*-------------------------------------------------------------------*/
int exec_instore_cmd( char *Command,  char *Result)
{
int rc=0;
short RetRC=0;
    UNREFERENCED(rc);
    UNREFERENCED(RetRC);

#if defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX)

    if ( RexxStatus == _DISABLED_ )
    {
        WRMSG( HHC17521, "I", "");
        return -1 ;
    }
    if ( !hRexxLibHandle )
    {
        char *envvar;
        if ( !( envvar = getenv("HREXX_PACKAGE") ) )
        {
            REXX_TRY_OOREXX( exec_instore_cmd_Rexx_Loaded )
            REXX_TRY_REGINA( exec_instore_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        if ( strcasecmp(envvar, "auto" ) == 0 )
        {
            REXX_TRY_OOREXX( exec_instore_cmd_Rexx_Loaded )
            REXX_TRY_REGINA( exec_instore_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        if ( strcasecmp(envvar, "none" ) == 0 )
        {
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17521, "I", "");
            return -1;
        }
        if ( strcasecmp(envvar, OOREXX_PACKAGE ) == 0  )
        {
            REXX_TRY_OOREXX( exec_instore_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        else if ( strcasecmp(envvar, REGINA_PACKAGE ) == 0  )
        {
            REXX_TRY_REGINA( exec_instore_cmd_Rexx_Loaded )
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17526, "I", RexxPackage);
        }
        else
        {
            SETREXX_RESET()
            RexxStatus = _DISABLED_ ;
            WRMSG( HHC17501, "E", envvar,"Unknown Rexx Package");
            return -1;
        }

    }
exec_instore_cmd_Rexx_Loaded:
#endif /* defined(ENABLE_OBJECT_REXX) && defined(ENABLE_REGINA_REXX) */

    return (*RexxExecInstoreCmd)( Command, NULL, RetRC, Result);

}

void InitializePaths(char * wPath)
{
char *ptr;
char *envvar;

    if ( RexxPathArray )
    {
        free(RexxPathArray) ;
        RexxPathArray = NULL;
        RexxPathMax = 0;
        RexxPathCount = 0;
    }

    if ( wPath )
    {
        RexxPath = strdup(wPath);
        free(wPath);
    }
    else
    {
        if ( ( envvar = getenv("HREXX_PATH") ) )
        {
            RexxPath = strdup(envvar);
        }
        else if ( ( envvar = getenv("HREXX_PATHS") ) )
        {
            RexxPath = strdup(envvar);
        }

    }

    if ( RexxPath )
    {
        RexxPathMax = tkcount(RexxPath,PATHDELIM) + 8 ;
        RexxPathArray = malloc(RexxPathMax * sizeof(char*));

        for (RexxPathCount = 0, ptr = strtok(RexxPath,PATHDELIM); ptr; ptr = strtok(NULL, PATHDELIM))
        {
            RexxPathArray[RexxPathCount] = ptr;
            RexxPathCount++;
        }
    }

    if ( SysPathArray )
    {
        free(SysPathArray) ;
        SysPathArray = NULL;
        SysPathMax = 0;
        SysPathCount = 0;
    }

    envvar = getenv("PATH");
    SysPath = strdup(envvar);

    SysPathMax = tkcount(SysPath,PATHDELIM) + 8 ;
    SysPathArray = malloc(SysPathMax * sizeof(char*));

    for (SysPathCount= 0,ptr = strtok(SysPath,PATHDELIM); ptr; ptr = strtok(NULL, PATHDELIM))
    {
        SysPathArray[SysPathCount] = ptr;
        SysPathCount++;
    }

    PathsInitialized = TRUE;
    return;
}

void InitializeExtensions(char * wExtensions)
{
char *ptr;
char *envvar;

    if ( ExtensionsArray )
    {
        free(ExtensionsArray) ;
        ExtensionsArray = NULL;
        ExtensionsMax = 0;
        ExtensionsCount = 0;
    }

    if ( wExtensions )
    {
        Extensions = strdup(wExtensions);
        free(wExtensions);
    }
    else
    {
        if ( ( envvar = getenv("HREXX_EXTENSIONS") ) )
        {
            Extensions = strdup(envvar);
        }
        else if ( ( envvar = getenv("HREXX_SUFFIXES") ) )
        {
            Extensions = strdup(envvar);
        }
        else
            Extensions = strdup(EXTENSIONS);
    }

    ExtensionsMax = tkcount(Extensions,EXTNDELIM) + 8;
    ExtensionsArray = malloc(ExtensionsMax * sizeof(char*));

    ExtensionsArray[0] = "";
    ExtensionsCount = 1 ;
    for (ptr = strtok(Extensions, EXTNDELIM ); ptr; ptr = strtok(NULL, EXTNDELIM))
    {
        ExtensionsArray[ExtensionsCount] = ptr;
        ExtensionsCount++;
    }
    ExtensionsInitialized = TRUE;
    return;
}

void InitializeOptions( void )
{
char *envvar;

    OptionsInitialized = TRUE ;
    useResolver = TRUE;
    MessageLevel = 0;
    MessagePrefix = NULL;
    ErrorPrefix = NULL;
    if ( ( envvar = getenv("HREXX_MODE") ) )
    {
        if ( strcasecmp(envvar, "command" ) == 0 )
        {
            RexxMode = _COMMAND_ ;
        }
        else if ( strcasecmp(envvar, "subroutine" ) == 0 )
        {
            RexxMode = _SUBROUTINE_ ;
        }
        else
            RexxMode = _COMMAND_ ;
    }
    else
        RexxMode = _COMMAND_ ;
}

#endif /* defined(ENABLE_OBJECT_REXX) || defined(ENABLE_REGINA_REXX)  */

#endif /* #ifndef _HREXX_C_  */
