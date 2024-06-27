/* CMDTAB.C     (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2002-2012     */
/*              (C) Copyright Jan Jaeger, 2003-2012                  */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              (C) and others 2013-2023                             */
/*              Route all Hercules configuration statements          */
/*              and panel commands to the appropriate functions      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _CMDTAB_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "history.h"

/*-------------------------------------------------------------------*/
/* Prototype of a command processing function                        */
/*-------------------------------------------------------------------*/
#define     CMDFUNC_ARGS_PROTO  int argc, char *argv[], char *cmdline
#define     CMDFUNC_ARGS            argc,       argv,         cmdline
typedef int CMDFUNC( CMDFUNC_ARGS_PROTO );

/*-------------------------------------------------------------------*/
/* Handle externally defined commands. If a command's primary        */
/* processing function is defined in some other module other than    */
/* this one, the below EXTCMD macro will route our module to it.     */
/* This macro must be #defined before "cmdtab.h" is #included.       */
/*-------------------------------------------------------------------*/

#define EXTCMD(      _cmd )    call_ ## _cmd
#define CALL_EXTCMD( _cmd )                        \
                                                   \
  int call_ ## _cmd ( CMDFUNC_ARGS_PROTO )  {      \
      return   _cmd ( CMDFUNC_ARGS       ); }

/*-------------------------------------------------------------------*/
/* Layout of CMDTAB Command Routing Table                            */
/*-------------------------------------------------------------------*/
struct CMDTAB
{
    const char    *statement;           /* Command or statement      */
    const char    *shortdesc;           /* Short description         */
    const char    *longdesc;            /* Long description          */
    CMDFUNC       *function;            /* Handler function          */
    const size_t   mincmdlen;           /* Minimum abbreviation      */
    BYTE           type;                /* Command type (see below)  */
};
typedef struct CMDTAB CMDTAB;

/*-------------------------------------------------------------------*/
/* Command types                                                     */
/*-------------------------------------------------------------------*/
#define DISABLED       0x00             /* Disabled command          */
#define SYSOPER        0x01             /* System Operator           */
#define SYSMAINT       0x02             /* System Maintainer         */
#define SYSPROG        0x04             /* Systems Programmer        */
#define SYSNONE        0x08             /* Valid with no group       */
#define SYSCONFIG      0x10             /* System Configuration      */
#define SYSDEVEL       0x20             /* System Developer          */
#define SYSDEBUG       0x40             /* Enable Debugging          */
#define SYSNDIAG8      0x80             /* Invalid for DIAG008       */
#define SYSALL         0x7F             /* Enable for any lvls       */

/* Combinations as a one-word mnemonic */

#define SYSCMD              (SYSOPER | SYSMAINT | SYSPROG | SYSCONFIG | SYSDEVEL | SYSDEBUG)
#define SYSCMDNDIAG8        (SYSCMD    | SYSNDIAG8)
#define SYSCFGNDIAG8        (SYSCONFIG | SYSNDIAG8)
#define SYSALLNDIAG8        (SYSALL    | SYSNDIAG8)
#define SYSCMDNOPER         (SYSCMD       - SYSOPER)
#define SYSCMDNOPERNDIAG8   (SYSCMDNDIAG8 - SYSOPER)
#define SYSCMDNOPERNPROG    (SYSCMDNOPER  - SYSPROG)
#define SYSPROGDEVEL        (SYSPROG | SYSDEVEL)
#define SYSPROGDEVELDEBUG   (SYSPROG | SYSDEVEL | SYSDEBUG)

/*-------------------------------------------------------------------*/
/* Macros used to define entries in the command table                */
/*-------------------------------------------------------------------*/
#define _COMMAND(_s,   _f,_t,_d,_l)  {(_s),(_d),(_l),(_f),(0), (_t)},
#define _CMDABBR(_s,_a,_f,_t,_d,_l)  {(_s),(_d),(_l),(_f),(_a),(_t)},

/*-------------------------------------------------------------------*/
/* Macros used to declare the functions before they're referenced.   */
/*-------------------------------------------------------------------*/
#define _FW_REF_CMD(_s,   _f,_t,_d,_l)  extern int (_f)( CMDFUNC_ARGS_PROTO );
#define _FW_REF_ABR(_s,_a,_f,_t,_d,_l)  extern int (_f)( CMDFUNC_ARGS_PROTO );

/*-------------------------------------------------------------------*/
/* Create forward references for all commands in the command table   */
/*-------------------------------------------------------------------*/
#define _FW_REF
#define COMMAND             _FW_REF_CMD
#define CMDABBR             _FW_REF_ABR

#include "cmdtab.h"         /* (declare all command functions) */

#undef  COMMAND             /* (will be re-#defined below) */
#undef  CMDABBR             /* (will be re-#defined below) */
#undef _FW_REF

/*-------------------------------------------------------------------*/
/*              THE ACTUAL COMMAND TABLE ITSELF                      */
/*-------------------------------------------------------------------*/
#define COMMAND             _COMMAND
#define CMDABBR             _CMDABBR
static  CMDTAB   cmdtab[]  = {              /* (COMMAND table)       */
  #include      "cmdtab.h"                  /* (COMMAND entries)     */
  COMMAND ( NULL, NULL, 0, NULL, NULL )     /* (end of table)        */
};

/*-------------------------------------------------------------------*/
/* Externally defined commands are defined/routed here               */
/*-------------------------------------------------------------------*/

  CALL_EXTCMD ( cachestats_cmd )            /* (lives in cache.c)    */
  CALL_EXTCMD ( shrd_cmd       )            /* (lives in shared.c)   */
  CALL_EXTCMD ( ptt_cmd        )            /* (lives in pttrace.c)  */
  CALL_EXTCMD ( locks_cmd      )            /* (lives in hthreads.c) */
  CALL_EXTCMD ( threads_cmd    )            /* (lives in hthreads.c) */

/*-------------------------------------------------------------------*/
/* $zapcmd - internal debug - may cause havoc - use with caution     */
/*-------------------------------------------------------------------*/
int zapcmd_cmd( CMDFUNC_ARGS_PROTO )
{
CMDTAB* cmdent;
int i;

//  Format:
//
//      $zapcmd  cmdname  CFG|NOCFG|CMD|NOCMD
//
//  For normal non-DEBUG production release builds, use the sequence:
//
//      msglvl   VERBOSE      (optional)
//      msglvl   DEBUG        (optional)
//      cmdlvl   DEBUG        (*required!*) (because not DEBUG build,
//      $zapcmd  cmdname  CMD                and $zapcmd is SYSDEBUG)
//
//  In other words, the $zapcmd is itself a "debug" level command, and
//  thus in order to use it, the debug cmdlvl must be set first (which
//  is the default for DEBUG builds but not for Release/Retail builds).

    UNREFERENCED(cmdline);

    if (argc > 1)
    {
        for (cmdent = cmdtab; cmdent->statement; cmdent++)
        {
            if (!strcasecmp(argv[1], cmdent->statement))
            {
                if (argc > 2)
                    for (i = 2; i < argc; i++)
                    {
                        if (!strcasecmp(argv[i],"Cfg"))
                            cmdent->type |= SYSCONFIG;
                        else
                        if (!strcasecmp(argv[i],"NoCfg"))
                            cmdent->type &= ~SYSCONFIG;
                        else
                        if (!strcasecmp(argv[i],"Cmd"))
                            cmdent->type |= SYSCMD;
                        else
                        if (!strcasecmp(argv[i],"NoCmd"))
                            cmdent->type &= ~SYSCMD;
                        else
                        {
                            LOGMSG( "Invalid arg: %s: %s %s [(No)Cfg|(No)Cmd]\n",
                                argv[i], argv[0], argv[1] );
                            return -1;
                        }
                    }
                else
                    LOGMSG( "%s: %s(%sCfg,%sCmd)\n", argv[0], cmdent->statement,
                      (cmdent->type & SYSCONFIG) ? "" : "No",
                      (cmdent->type & SYSCMD)    ? "" : "No" );
                return 0;
            }
        }
        LOGMSG( "%s: %s not in command table\n", argv[0], argv[1] );
        return -1;
    }
    else
        LOGMSG( "Usage: %s <command> [(No)Cfg|(No)Cmd]\n", argv[0] );
    return -1;
}

/*-------------------------------------------------------------------*/
/* Command processing helper functions                               */
/*-------------------------------------------------------------------*/
int  OnOffCommand   ( CMDFUNC_ARGS_PROTO );     /* (see hsccmd.c)    */
int  sf_cmd         ( CMDFUNC_ARGS_PROTO );     /* (see hsccmd.c)    */

/*-------------------------------------------------------------------*/
/* Background command processing structure                           */
/*-------------------------------------------------------------------*/
struct BGCMD
{
    CMDFUNC  *func;                     /* Command function to call  */
    char     cmdline[FLEXIBLE_ARRAY];   /* Command's command-line    */
};
typedef struct BGCMD BGCMD;

/*-------------------------------------------------------------------*/
/* Thread to process a command in the "background"                   */
/*-------------------------------------------------------------------*/
void *bgcmd_thread( void *bgcmd )
{
BGCMD* pBGCMD;
char*  cmdline;
int    argc;
char*  argv[MAX_ARGS];

    pBGCMD = (BGCMD*) bgcmd;
    /* (must save copy of cmdline since parse_args modifies it) */
    cmdline = strdup( pBGCMD->cmdline );
    parse_args( pBGCMD->cmdline, MAX_ARGS, argv, &argc );
    pBGCMD->func( CMDFUNC_ARGS );
    free( pBGCMD );
    free( cmdline );
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Route Hercules command to proper command handling function        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int CallHercCmd ( CMDFUNC_ARGS_PROTO )
{
CMDTAB* pCmdTab;
size_t  cmdlen, matchlen;
int     rc = HERRINVCMD;             /* Default to invalid command   */
bool    isdiag;

    /* Let 'cscript' command run immediately in any context */
    if (argc >= 1 && strcasecmp( argv[0], "cscript" ) == 0)
        return cscript_cmd( CMDFUNC_ARGS );

    /* [ENTER] key by itself: either start the CPU or ignore
       it altogether when instruction stepping is not active. */
    if (0
        || !argc            /* (no args)    */
        || !argv[0]         /* (no cmd)     */
        || !cmdline         /* (no cmdline) */
        || !cmdline[0]      /* (empty cmd)  */
        || !argv[0][0]      /* (empty cmd)  */
    )
    {
        if (sysblk.instbreak)
            rc = start_cmd( 0, NULL, NULL );
        else
            rc = 0;         /* ignore [ENTER] */
        return rc;
    }

    /* (sanity check) */
    ASSERT( argc && cmdline && *cmdline && argv[0] && argv[0][0] );

    /* If a system command hook exists, let it try processing it first. */
    /* Only if it rejects it, will we then try processing it ourselves. */
    if (system_command)
        if ((rc = system_command( CMDFUNC_ARGS )) != HERRINVCMD)
            return rc;

    /* Check for comment command. We need to test for this separately
       rather than scanning our COMAMND table since the first token
       of the "command" might not be a single '#' or '*'. That is to
       say, the first token might be "#comment" or "*comment" (with
       no blanks following the '#' or '*'), which would thus not match
       either of our "comment_cmd" COMMAND table entries. Note that
       this also means the COMMAND table entries for '*' and '#' are
       redundant since they will never be used (since we're processing
       comments here and not via our table search), but that's okay. */
    if (0
        || argv[0][0] == '#'       /* (comment command?) */
        || argv[0][0] == '*'       /* (comment command?) */
    )
    {
        /* PROGRAMMING NOTE: we ALWAYS call the "comment_cmd" function
           for comments even though today it does nothing. We might decide
           to do additional processing for comments in the future (such
           as simply counting them for example), so we should ALWAYS call
           our comment_cmd function here instead of just returning. */
        rc = comment_cmd( CMDFUNC_ARGS );
        return rc;
    }

    /* Route standard formatted commands from our COMMAND routing table */

    /* PROGRAMMING NOTE: since our table is now sorted, the below could
       be converted to a more efficient binary search algorithm instead */

    isdiag = is_diag_instr();

    for (pCmdTab = cmdtab; pCmdTab->statement; pCmdTab++)
    {
        if (1
            && pCmdTab->function
            && (pCmdTab->type & sysblk.sysgroup)
            /********************************************************
            *  SECURITY CHECK: any command in the "SYSNDIAG8" group
            *  are not *ever* to be allowed to be issued via DIAG8!
            *  The following test causes us to consider only those
            *  commands that are either NOT in the SYSNDIAG8 group,
            *  or, for those that are, ONLY when DIAG8 isn't active.
            ********************************************************/
            && (!(pCmdTab->type & SYSNDIAG8) || !isdiag)
        )
        {
            cmdlen = pCmdTab->mincmdlen ? pCmdTab->mincmdlen : strlen( pCmdTab->statement );
            matchlen = MAX( strlen(argv[0]), cmdlen );

            if (!strncasecmp( argv[0], pCmdTab->statement, matchlen ))
            {
                char cmd[256]; /* (copy of command name) */

                /* Make full-length copy of the true command's name */
                STRLCPY( cmd, pCmdTab->statement );

                /* Replace abbreviated command with full command */
                if (pCmdTab->mincmdlen)
                    argv[0] = cmd; /* (since theirs may be abbreviated) */

                /* Run command in background? (last argument == '&') */
                if (strcmp(argv[argc-1],"&") == 0)
                {
                BGCMD *bgcmd;
                TID tid;
                int i,len;

                    /* Calculate length of the command-line (all arguments
                       except the last one), including intervening blanks. */
                    for (len=0, i=0; i < argc-1; i++ )
                    {
                        len += (int) strlen( (char*) argv[i] ) + 1;
                        if (strchr( argv[i], ' ' ))
                            len += 2; // (surrounding double quotes)
                    }

                    /* Build parameter to pass to background thread */
                    bgcmd = (BGCMD*) malloc( sizeof(BGCMD) + len );
                    bgcmd->func = pCmdTab->function;

                    /* Build private copy of cmdline for bgcmd_thread */
                    strlcpy( bgcmd->cmdline, argv[0], len );
                    for (i=1; i < argc-1; i++)
                    {
                        strlcat( bgcmd->cmdline,  " ",    len );

                        if (strchr( argv[i], ' ' ))
                            strlcat( bgcmd->cmdline,  "\"", len );

                        strlcat( bgcmd->cmdline, argv[i], len );

                        if (strchr( argv[i], ' ' ))
                            strlcat( bgcmd->cmdline,  "\"", len );
                    }

                    /* Process command asynchronously in the background */
                    rc = create_thread( &tid, DETACHED, bgcmd_thread, bgcmd, "bgcmd_thread" );
                }
                else /* (not a background command) */
                {
                    /* Does last argument start with two ampersands? */
                    if (strncmp( argv[argc-1], "&&", 2 ) == 0)
                        /* Yes, skip past the first one */
                        argv[argc-1]++; /* ("&&&" ==> "&&", "&&" ==> "&", etc) */

                    /* Process the Hercules command */
                    rc = pCmdTab->function( CMDFUNC_ARGS );
                }
                break;
            }
            /* end strncasecmp( table entry match ) */
        }
        /* end if ( valid table entry ) */
    }
    /* end for ( search table ) */

    /* If we didn't find an exact match in our table, check if
       this is maybe a special non-standard formatted command. */
    if (rc == HERRINVCMD && (sysblk.sysgroup & SYSCMDNOPER))
    {
        /* shadow file commands: add/remove/compress/display/check */
        if (0
            || !strncasecmp( cmdline, "sf+", 3 )
            || !strncasecmp( cmdline, "sf-", 3 )
            || !strncasecmp( cmdline, "sfc", 3 )
            || !strncasecmp( cmdline, "sfd", 3 )
            || !strncasecmp( cmdline, "sfk", 3 )
        )
            rc = sf_cmd( CMDFUNC_ARGS );
        else
        /* "x+" and "x-" commands: turn switch on or off */
        if (0
            || '+' == cmdline[1]
            || '-' == cmdline[1]
        )
            rc = OnOffCommand( CMDFUNC_ARGS );
    }

    return rc;
}
/* end CallHercCmd */

/*-------------------------------------------------------------------*/
/* Hercules command line processor                                   */
/*-------------------------------------------------------------------*/
static int DoCallHercCmdLine (char* pszCmdLine, BYTE internal)
{
    int      argc;
    char*    argv[ MAX_ARGS ];
    char*    cmdline  = NULL;
    int      rc       = -1;

    /* Save unmodified copy of the command line in case
       its format is unusual and needs customized parsing. */
    cmdline = strdup( pszCmdLine );

    /* Parse the command line into its individual arguments.
       Note: original command line now sprinkled with nulls */
    parse_args( pszCmdLine, MAX_ARGS, argv, &argc );

    /* Our primary Hercules command function gets first crack. */
    if ((rc = CallHercCmd( CMDFUNC_ARGS )) != HERRINVCMD)
        goto HercCmdExit;
    ASSERT( argv[0] ); /* (herc handles any/all empty commands) */

#if defined( _FEATURE_SYSTEM_CONSOLE )
    /* See if maybe it's a command that the guest understands. */
    if ( !internal && sysblk.scpimply && can_send_command() )
        rc = scp_command( cmdline, false,
                          sysblk.scpecho ? true : false,
                          cmdline[0] == '\\' ? true : false );
    else
#else
        UNREFERENCED( internal );
#endif
        /* Error: unknown/unsupported command */
        WRMSG( HHC01600, "E", argv[0] );

HercCmdExit:

    /* Free our saved copy */
    free( cmdline );

    if (MLVL( DEBUG ))
    {
        char msgbuf[32];
        MSGBUF( msgbuf, "RC = %d", rc );
        WRMSG( HHC90000, "D", msgbuf );
    }

    return rc;
}
DLL_EXPORT int InternalHercCmd (char* pszCmdLine)
{
    return DoCallHercCmdLine( pszCmdLine, 1 );
}
DLL_EXPORT int HercCmdLine (char* pszCmdLine)
{
    return DoCallHercCmdLine( pszCmdLine, 0 );
}
/* end DoCallHercCmdLine */

/*-------------------------------------------------------------------*/
/* Sort COMMAND table ascending by command/statement                 */
/*-------------------------------------------------------------------*/
static int SortCmdTab( const void* p1, const void* p2 )
{
    CMDTAB* pCmdTab1 = (CMDTAB*) p1;
    CMDTAB* pCmdTab2 = (CMDTAB*) p2;
    /* (special handling for NULL table terminator) */
    return (pCmdTab1->statement && pCmdTab2->statement) ?
        strcasecmp( pCmdTab1->statement, pCmdTab2->statement )
        : (!pCmdTab1->statement ? +1 : -1);
}

/*-------------------------------------------------------------------*/
/* HelpCommand - display more help for a given command    (internal) */
/*-------------------------------------------------------------------*/
int HelpCommand( CMDFUNC_ARGS_PROTO )
{
    static int didinit = 0;
    CMDTAB* pCmdTab;
    int     rc = 1;
    int     pfxlen = -1;
    char   *p = NULL;
    int     stmtlen, max_stmtlen = 9;

    UNREFERENCED( cmdline );

    if (!didinit)
    {
        qsort( cmdtab, _countof( cmdtab ), sizeof( CMDTAB ), SortCmdTab );
        didinit = 1;
    }

    /* Keep 'max_stmtlen' accurate */
    for (pCmdTab = cmdtab; pCmdTab->statement; pCmdTab++)
    {
        if (pCmdTab->statement)
        {
            if ((stmtlen = (int) strlen( pCmdTab->statement )) > max_stmtlen)
                max_stmtlen = stmtlen;
        }
    }

    /* Too many arguments? */
    if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", "help" );
        return 1;
    }

    /* If "help pfx*", calculate prefix length */
    if (argc == 2)
    {
        p = strchr(argv[1], '*');
        if (p != NULL)
        {
            pfxlen = (int)(p - argv[1]);
            if (strlen( argv[1] ) > (unsigned) (pfxlen + 1))
            {
                // "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }
    }

    /* List all commands or those matching given prefix?  */
    if (argc < 2 || pfxlen >= 0)    /* "help" or "help pfx*" ? */
    {
        int found = FALSE;
        int didlong = FALSE;
        char longflag;

        if (argc < 2)
            pfxlen = 0;

        /* List standard formatted commands from our routing table */
        for (pCmdTab = cmdtab; pCmdTab->statement; pCmdTab++)
        {
            if (pCmdTab->type == SYSPROGDEVELDEBUG)
            {
                /* Special System Programmer Developer Debug command */
                /* Command Level must also be Debug too, else hidden */
                if ((sysblk.sysgroup & SYSGROUP_PROGDEVELDEBUG) != SYSGROUP_PROGDEVELDEBUG)
                    continue;
            }
            if (  (pCmdTab->type & sysblk.sysgroup)
               && (pCmdTab->shortdesc)
               && (pfxlen == 0 || !strncasecmp( argv[1], pCmdTab->statement, pfxlen ))
            )
            {
                longflag = ' ';
                if (pCmdTab->longdesc)
                {
                    longflag = '*';
                    didlong = TRUE;
                }
                if (!found)
                {
                    found = TRUE;
                    WRMSG( HHC01603, "I", "" );
                    WRMSG( HHC01602, "I", max_stmtlen, max_stmtlen, "Command         ", ' ', "Description" );
                    WRMSG( HHC01602, "I", max_stmtlen, max_stmtlen, "----------------", ' ', "-----------------------------------------------" );
                }
                WRMSG( HHC01602, "I", max_stmtlen, max_stmtlen, pCmdTab->statement, longflag, pCmdTab->shortdesc );
            }
        }

        /* Prefix given but no matches found? */
        if (pfxlen > 0 && !found)
        {
            // "No help available for mask '%s'"
            WRMSG( HHC01609, "E", argv[1] );
            rc = -1;
        }
        else
        {
            WRMSG( HHC01603, "I", "" );
            if (didlong)
            {
                // " (*)  Enter \"help <command>\" for more info."
                WRMSG( HHC01610, "I" );
                WRMSG( HHC01603, "I", "" );
            }
            rc = 0;
        }
    }
    else /* (argc == 2 && pfxlen < 0) "help cmd": Show help for given command */
    {
        char longflag;
        size_t cmdlen, matchlen;

        for (pCmdTab = cmdtab; pCmdTab->statement; pCmdTab++)
        {
            cmdlen = pCmdTab->mincmdlen ? pCmdTab->mincmdlen : strlen( pCmdTab->statement );
            matchlen = MAX( strlen(argv[1]), cmdlen );

            if (pCmdTab->type == SYSPROGDEVELDEBUG)
            {
                /* Special System Programmer Developer Debug command */
                /* Command Level must also be Debug too, else hidden */
                if ((sysblk.sysgroup & SYSGROUP_PROGDEVELDEBUG) != SYSGROUP_PROGDEVELDEBUG)
                    continue;
            }
            if (1
                && (pCmdTab->shortdesc)
                && (pCmdTab->type & sysblk.sysgroup)
                && !strncasecmp( argv[1], pCmdTab->statement, matchlen )
            )
            {
                longflag = (pCmdTab->longdesc) ? '*' : ' ';

                WRMSG( HHC01603, "I", "" );
                WRMSG( HHC01602, "I", max_stmtlen, max_stmtlen, "Command         ", ' ', "Description" );
                WRMSG( HHC01602, "I", max_stmtlen, max_stmtlen, "----------------", ' ', "-------------------------------------------------------" );
                WRMSG( HHC01602, "I", max_stmtlen, max_stmtlen, pCmdTab->statement, longflag, pCmdTab->shortdesc);

                if (pCmdTab->longdesc)
                {
                    char buf[257];
                    int i = 0;
                    int j;

                    WRMSG( HHC01603, "I", "" );
                    while(pCmdTab->longdesc[i])
                    {
                        for(j = 0; j < (int)sizeof(buf)      &&
                                   pCmdTab->longdesc[i] &&
                                   pCmdTab->longdesc[i] != '\n'; i++, j++)
                        {
                            buf[j] = pCmdTab->longdesc[i];
                        }

                        buf[j] = '\0';
                        if(pCmdTab->longdesc[i] == '\n')
                            i++;
                        WRMSG( HHC01603, "I", buf );
                    }
                }
                WRMSG( HHC01603, "I", "" );
                rc = 0;
                break;
            }
        }

        if (rc == 1)  /* (not found?) */
        {
            // "Unknown command %s, no help available"
            WRMSG( HHC01604, "I", argv[1] );
            rc = -1;
        }
    }
    return rc;
}
/* end HelpCommand */

/*-------------------------------------------------------------------*/
/* Helper function to echo a command to the console       (internal) */
/*-------------------------------------------------------------------*/
static void EchoHercCmdLine( char* cmd )
{
    BYTE panel = WRMSG_NORMAL;  // (default)

    /* Don't echo the command if it's the DIAG8 interface issuing it
       unless the echo option is enabled, and if it is enabled, only
       echo it to the panel to prevent DIAG8 from capturing the echo.
    */
    if (is_diag_instr())
    {
        if (!(sysblk.diag8opt & DIAG8CMD_ECHO))
            return;             // (no logging whatsoever)
        panel = WRMSG_PANEL;    // (prevent DIAG8 capturing)
    }

    PWRMSG( panel, HHC01603, "I", RTRIM( cmd ));    // "%s"
}

void* FindSCRCTL( TID tid );// (external helper function; see script.c)

/*-------------------------------------------------------------------*/
/* panel_command entry-point:  determine if Hercules or SCP command  */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* the_real_panel_command( char* cmdline )
{
#define MAX_CMD_LEN (32768)

    char  cmd[ MAX_CMD_LEN ];           /* Copy of panel command     */
    char* pCmdLine;
    unsigned i;
    int hercecho = 1;                   /* Default echo to console   */
    int rc = 0;                         /* Return Code from command  */

    /* Handle command separation */
    if (sysblk.cmdsep)
    {
        /* Does cmdline contain any separator characters? */
        char*  firstsep  = strchr( cmdline, sysblk.cmdsep );
        if (firstsep)
        {
            /* Yes! Must process commands separately */
            void*   rc;
            size_t  sepcharindex  = (firstsep - cmdline);
            char*   first_cmd     = strdup( cmdline );

            /* Mark end of first command */
            first_cmd[ sepcharindex ] = 0;

            /* Process first command */
            panel_command( first_cmd );

            /* Process remaining command(s) */
            rc = panel_command( first_cmd + sepcharindex + 1 );

            /* Prevent memory leak */
            free( first_cmd );

            /* Return with retcode of second command */
            return rc;
        }
    }

    pCmdLine = cmdline; ASSERT( pCmdLine );

    /* Every command will be stored in history list,
       EXCEPT: null commands, script commands, scp input,
       and "silent" commands (prefixed with '-')
    */
    if (*pCmdLine != 0 && !FindSCRCTL( thread_id() ))
    {
        if (!(*pCmdLine == '-'))    /* (normal command?) */
        {

#if defined( _FEATURE_SYSTEM_CONSOLE )

            if (*pCmdLine == '.' || *pCmdLine == '!')
            {
                if (sysblk.scpecho)
                    history_add( cmdline );
            }
            else

#endif /* defined( _FEATURE_SYSTEM_CONSOLE ) */

                history_add( cmdline );
        }
    }

    /* Copy panel command to work area, skipping leading blanks.
       If the command starts with a -, then strip it and indicate
       that we do NOT want the command echoed to the console.
    */
    hercecho = 1;   // (default)

    while (*pCmdLine && isspace( (unsigned char)*pCmdLine ))
        pCmdLine++;

    i = 0;

    while (*pCmdLine && i < (MAX_CMD_LEN-1))
    {
        if (i == 0 && (0            /* (1st character?) */
            || *pCmdLine == '-'     /* (silent command) */
            || *pCmdLine == '#'     /* (silent comment) */
        ))
        {
            hercecho = 0;           /* (silence please) */

            if (*pCmdLine == '-')   /* (silent command?)*/
            {
                pCmdLine++;         /* (skip past the '-' ... */
                                    /* ... and remove blanks) */

                while (*pCmdLine && isspace( (unsigned char)*pCmdLine ))
                    pCmdLine++;     /* (get past blank) */
            }
        }

        cmd[i] = *pCmdLine;
        i++;
        pCmdLine++;
    }
    cmd[i] = 0;

    /* (just pressing the enter key shouldn't be echoed) */
    if (0 == cmd[0])
        hercecho = 0;               /* (silence please) */

#if defined( _FEATURE_SYSTEM_CONSOLE )

    /* Check if SCP command */
    if (0
        || cmd[0] == '.'
        || cmd[0] == '!'
        || cmd[0] == '\\'
    )
    {
        bool  priomsg  = cmd[0] == '!'  ? true : false;
        bool  scpecho  = sysblk.scpecho ? true : false;
        bool  mask     = cmd[0] == '\\' ? true : false;

        if (!cmd[1]) {      /* (empty command given?) */
            cmd[1] = ' ';   /* (must send something!) */
            cmd[2] = 0;
        }

        rc = scp_command( cmd+1, priomsg, scpecho, mask );
    }
    else

#endif /* defined( _FEATURE_SYSTEM_CONSOLE ) */

    /* Perform variable substitution */
    {
        char* cl;

        set_symbol( "CUU",  "$(CUU)"  );
        set_symbol( "CCUU", "$(CCUU)" );
        set_symbol( "DEVN", "$(DEVN)" );

        cl = resolve_symbol_string( cmd );

        if (cl)
        {
            STRLCPY( cmd, cl );
            free( cl );
        }

        if (hercecho && *cmd)
            EchoHercCmdLine( cmd );

        rc = HercCmdLine( cmd );
    }

    return (void*)((uintptr_t)rc);
}
/* end the_real_panel_command */
