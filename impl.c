/* IMPL.C       (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
/*              Hercules Initialization Module                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module initializes the Hercules S/370 or ESA/390 emulator.   */
/* It builds the system configuration blocks, creates threads for    */
/* central processors, HTTP server, logger task and activates the    */
/* control panel which runs under the main thread when in foreground */
/* mode.                                                             */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _IMPL_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "devtype.h"
#include "herc_getopt.h"
#include "hostinfo.h"
#include "history.h"
#include "hRexx.h"
#if defined( OPTION_W32_CTCI )      // (need tt32_get_default_iface)
#include "w32ctca.h"
#endif
#include "cckddasd.h"               // (need cckd_gc_rpt_states)

static char shortopts[] =

    "eh::f:o:r:db:vt::p:l:s:";

#if defined(HAVE_GETOPT_LONG)
static struct option longopts[] =
{
    { "externalgui",    no_argument, NULL, 'e' },
    { "help",     optional_argument, NULL, 'h' },
    { "version",        no_argument, NULL, 'V' },
    { "config",   required_argument, NULL, 'f' },
    { "output",   required_argument, NULL, 'o' },
    { "logfile",  required_argument, NULL, 'o' },
    { "rcfile",   required_argument, NULL, 'r' },
    { "daemon",         no_argument, NULL, 'd' },
    { "herclogo", required_argument, NULL, 'b' },
    { "verbose",        no_argument, NULL, 'v' },
    { "test",     optional_argument, NULL, 't' },
    { "modpath",  required_argument, NULL, 'p' },
    { "ldmod",    required_argument, NULL, 'l' },
    { "defsym",   required_argument, NULL, 's' },
    { NULL,       0,                 NULL,  0  }
};
#endif

static LOGCALLBACK  log_callback = NULL;

struct cfgandrcfile
{
   const char * filename;             /* Or NULL                     */
   const char * const envname;        /* Name of environ var to test */
   const char * const defaultfile;    /* Default file                */
   const char * const whatfile;       /* config/restart, for message */
};

enum cfgorrc
{
   want_cfg,
   want_rc,
   cfgorrccount
};

static struct cfgandrcfile cfgorrc[ cfgorrccount ] =
{
   { NULL, "HERCULES_CNF", "hercules.cnf", "Configuration", },
   { NULL, "HERCULES_RC",  "hercules.rc",  "Run Commands",  },
};

#define                 MAX_MODS     50         /* Max mods to load  */
static char*  modnames[ MAX_MODS ] = {0};       /* ptrs to modnames  */
static int    modcount = 0;                     /* count of modnames */

/* forward define process_script_file (ISW20030220-3) */
extern int process_script_file( const char*, bool );
/* extern int quit_cmd(int argc, char *argv[],char *cmdline);        */

/* Forward declarations:                                             */
static void init_progname( int argc, char* argv[] );

static int process_args( int argc, char* argv[] );
#define PROCESS_ARGS_OK     0
#define PROCESS_ARGS_ERROR  1
#define PROCESS_ARGS_EXIT   2
/* End of forward declarations.                                      */

/*-------------------------------------------------------------------*/
/* Register a LOG callback                                           */
/*-------------------------------------------------------------------*/
DLL_EXPORT void registerLogCallback( LOGCALLBACK cb )
{
    log_callback = cb;
}

/*-------------------------------------------------------------------*/
/* Subroutine to exit process after flushing stderr and stdout       */
/*-------------------------------------------------------------------*/
static void delayed_exit (int exit_code)
{
    UNREFERENCED(exit_code);

    /* Delay exiting is to give the system
     * time to display the error message. */
#if defined( _MSVC_ )
    SetConsoleCtrlHandler( NULL, FALSE); // disable Ctrl-C intercept
#endif
    sysblk.shutimmed = TRUE;

    fflush(stderr);
    fflush(stdout);
    USLEEP(100000);
    do_shutdown();
    fflush(stderr);
    fflush(stdout);
    USLEEP(100000);
    return;
}

/*-------------------------------------------------------------------*/
/*                   SIGINT signal handler                           */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The SIGINT signal is sent to a process by its controlling        */
/*  terminal when a user wishes to interrupt the process. This       */
/*  is typically initiated by pressing Ctrl+C, but on some           */
/*  systems the "delete" character or "break" key can be used.       */
/*                                                                   */
/*-------------------------------------------------------------------*/
static void sigint_handler( int signo )
{
    UNREFERENCED( signo );

    signal( SIGINT, sigint_handler );

    /* Ignore signal unless presented on console thread */
    if (!equal_threads( thread_id(), sysblk.cnsltid ))
        return;

    /* Exit if previous SIGINT request was not actioned */
    if (sysblk.sigintreq)
    {
        /* Release the configuration */
        release_config( NULL );
        delayed_exit(1);
    }

    /* Set SIGINT request pending flag */
    sysblk.sigintreq = 1;

    /* Activate instruction stepping */
    sysblk.instbreak = 1;
    SET_IC_TRACE;
}

/*-------------------------------------------------------------------*/
/*                    SIGTERM signal handler                         */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The SIGTERM signal is sent to a process to request its           */
/*  termination. Unlike the SIGKILL signal, it can be caught         */
/*  and interpreted or ignored by the process. This allows the       */
/*  process to perform nice termination releasing resources          */
/*  and saving state if appropriate. SIGINT is nearly identical      */
/*  to SIGTERM.                                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
static void sigterm_handler( int signo )
{
    UNREFERENCED( signo );

    signal( SIGTERM, sigterm_handler );

    /* Ignore signal unless presented on main program (impl) thread */
    if (!equal_threads( thread_id(), sysblk.impltid ))
        return;

    /* Initiate system shutdown */
    do_shutdown();
}

#if defined( _MSVC_ )
/*-------------------------------------------------------------------*/
/* Perform immediate/emergency shutdown                              */
/*-------------------------------------------------------------------*/
static void do_emergency_shutdown()
{
    sysblk.shutdown = TRUE;

    if (!sysblk.shutimmed)
    {
        sysblk.shutimmed = TRUE;
        do_shutdown();
    }
    else // (already in progress)
    {
        while (!sysblk.shutfini)
            USLEEP(100000);
    }
}

/*-------------------------------------------------------------------*/
/* Windows console control signal handler                            */
/*-------------------------------------------------------------------*/
static BOOL WINAPI console_ctrl_handler( DWORD signo )
{
    switch ( signo )
    {
    ///////////////////////////////////////////////////////////////
    //
    //                    PROGRAMMING NOTE
    //
    ///////////////////////////////////////////////////////////////
    //
    // "SetConsoleCtrlHandler function HandlerRoutine Callback
    //  Function:"
    //
    //   "Return Value:"
    //
    //     "If the function handles the control signal, it
    //      should return TRUE."
    //
    //   "CTRL_LOGOFF_EVENT:"
    //
    //     "Note that this signal is received only by services.
    //      Interactive applications are terminated at logoff,
    //      so they are not present when the system sends this
    //      signal."
    //
    //   "CTRL_SHUTDOWN_EVENT:"
    //
    //     "Interactive applications are not present by the time
    //      the system sends this signal, therefore it can be
    //      received only be services in this situation."
    //
    //   "CTRL_CLOSE_EVENT:"
    //
    //      "... if the process does not respond within a certain
    //       time-out period (5 seconds for CTRL_CLOSE_EVENT..."
    //
    ///////////////////////////////////////////////////////////////
    //
    //  What this all boils down to is we'll never receive the
    //  logoff and shutdown signals (via this callback), and
    //  we only have a maximum of 5 seconds to return TRUE from
    //  the CTRL_CLOSE_EVENT signal. Thus, as normal shutdowns
    //  may likely take longer than 5 seconds and our goal is
    //  to try hard to shutdown Hercules as gracfully as we can,
    //  we are left with little choice but to always perform
    //  an immediate/emergency shutdown for CTRL_CLOSE_EVENT.
    //
    ///////////////////////////////////////////////////////////////

        case CTRL_BREAK_EVENT:

            // "CTRL_BREAK_EVENT received: %s"
            WRMSG( HHC01400, "I", "pressing interrupt key" );

            OBTAIN_INTLOCK( NULL );
            ON_IC_INTKEY;
            WAKEUP_CPUS_MASK( sysblk.waiting_mask );
            RELEASE_INTLOCK( NULL );

            return TRUE;

        case CTRL_C_EVENT:

            if (!sysblk.shutimmed)
                // "CTRL_C_EVENT received: %s"
                WRMSG( HHC01401, "I", "initiating emergency shutdown" );
            do_emergency_shutdown();
            return TRUE;


        case CTRL_CLOSE_EVENT:

            if (!sysblk.shutimmed)
                // "CTRL_CLOSE_EVENT received: %s"
                WRMSG( HHC01402, "I", "initiating emergency shutdown" );
            do_emergency_shutdown();
            return TRUE;

        default:

            return FALSE;  // (not handled; call next signal handler)
    }

    UNREACHABLE_CODE( return FALSE );
}

/*-------------------------------------------------------------------*/
/* Windows hidden window message handler                             */
/*-------------------------------------------------------------------*/
static LRESULT CALLBACK MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch (msg)
    {
    ///////////////////////////////////////////////////////////////
    //
    //                    PROGRAMMING NOTE
    //
    ///////////////////////////////////////////////////////////////
    //
    //  "If an application returns FALSE in response to
    //   WM_QUERYENDSESSION, it still appears in the shutdown
    //   UI. Note that the system does not allow console
    //   applications or applications without a visible window
    //   to cancel shutdown. These applications are automatically
    //   terminated if they do not respond to WM_QUERYENDSESSION
    //   or WM_ENDSESSION within 5 seconds or if they return FALSE
    //   in response to WM_QUERYENDSESSION."
    //
    ///////////////////////////////////////////////////////////////
    //
    //  What this all boils down to is we can NEVER prevent the
    //  user from logging off or shutting down since not only are
    //  we a console application but our window is created invisible
    //  as well. Thus we only have a maximum of 5 seconds to return
    //  TRUE from WM_QUERYENDSESSION or return 0 from WM_ENDSESSION,
    //  and since a normal shutdown may likely take longer than 5
    //  seconds and our goal is to try hard to shutdown Hercules as
    //  gracfully as possible, we are left with little choice but to
    //  perform an immediate emergency shutdown once we receive the
    //  WM_ENDSESSION message with a WPARAM value of TRUE.
    //
    ///////////////////////////////////////////////////////////////

        case WM_QUERYENDSESSION:

            // "%s received: %s"
            WRMSG( HHC01403, "I", "WM_QUERYENDSESSION", "allow" );
            return TRUE;    // Vote "YES"... (we have no choice!)

        case WM_ENDSESSION:

            if (!wParam)    // FALSE? (session not really ending?)
            {
                // Some other application (or the user themselves)
                // has aborted the logoff or system shutdown...

                // "%s received: %s"
                WRMSG( HHC01403, "I", "WM_ENDSESSION", "aborted" );
                return 0; // (message processed)
            }

            // User is logging off or the system is being shutdown.
            // We have a maximum of 5 seconds to shutdown Hercules.

            // "%s received: %s"
            WRMSG( HHC01403, "I", "WM_ENDSESSION", "initiating emergency shutdown" );
            do_emergency_shutdown();
            return 0; // (message handled)

        case WM_POWERBROADCAST:

            // Notifies applications that a power-management event
            // has occurred.

            switch (wParam)
            {
                case PBT_APMSUSPEND:

                    // Notifies applications that the computer
                    // is about to enter a suspended state.

                    sysblk.sys_suspended = true;
                    sysblk.sys_resumed   = false;
                    break;

                case PBT_APMRESUMESUSPEND:

                    // Notifies applications that the system has resumed
                    // operation after being suspended.

                    /*
                       The below special "FALLTHRU" comment lets GCC know that we are
                       purposely falling through to the next switch case and is needed
                       in order to suppress the warning that GCC would otherwise issue.
                    */
                    /* FALLTHRU */

                case PBT_APMRESUMEAUTOMATIC:

                    // Notifies applications that the computer has woken up
                    // automatically to handle an event.

                    sysblk.sys_suspended = false;
                    sysblk.sys_resumed   = true;
                    break;

                default:

                    break;  /* (do nothing) */
            }

            /*
               The below special "FALLTHRU" comment lets GCC know that we are
               purposely falling through to the next switch case and is needed
               in order to suppress the warning that GCC would otherwise issue.
            */
            /* FALLTHRU */

        default:

            return DefWindowProc( hWnd, msg, wParam, lParam );
    }

    UNREACHABLE_CODE( return 0 );
}

// Create invisible message handling window...

HANDLE  g_hWndEvt  = NULL;  // (temporary window creation event)
HWND    g_hMsgWnd  = NULL;  // (window handle of message window)

static void* WinMsgThread( void* arg )
{
    WNDCLASS  wc    =  {0};

    UNREFERENCED( arg );

    wc.lpfnWndProc    =  MainWndProc;
    wc.hInstance      =  GetModuleHandle(0);
    wc.lpszClassName  =  "Hercules";

    RegisterClass( &wc );

    g_hMsgWnd = CreateWindowEx( 0,
        "Hercules", "Hercules",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL,
        GetModuleHandle(0), NULL );

    SetEvent( g_hWndEvt );      // (indicate create window completed)

    if (g_hMsgWnd)  // (pump messages if window successfully created)
    {
        MSG msg;
        while (GetMessage( &msg, NULL , 0 , 0 ))
        {
            TranslateMessage ( &msg );
            DispatchMessage  ( &msg );
        }
    }
    return NULL;
}
#endif /* defined( _MSVC_ ) */

#if defined( OPTION_WATCHDOG )
/*-------------------------------------------------------------------*/
/*  watchdog_thread - monitor system for deadlocks                   */
/*-------------------------------------------------------------------*/
static void* watchdog_thread( void* arg )
{
    REGS* regs;
    S64   savecount[ MAX_CPU_ENGS ];
    int   cpu;
    int   sleep_seconds  = WATCHDOG_SECS;
    int   sleep_secs2nd  = 3;
    int   slept_secs;
    int   rc;

    bool  deadlock_reported = false;
    bool  hung_cpu_reported = false;

    CPU_BITMAP  hung_cpus_mask  = 0;

    UNREFERENCED( arg );

    for (cpu=0; cpu < sysblk.maxcpu; cpu++)
        savecount[ cpu ] = -1;

    /* Set watchdog priority LOWER than the CPU thread priority
       such that it will not invalidly detect an inoperable CPU
    */
    SET_THREAD_PRIORITY( MAX( sysblk.minprio, sysblk.cpuprio - 1 ), sysblk.qos_user_initiated);
    UNREFERENCED(rc);

    do
    {
        /* PROGRAMMING NOTE: sleeping in many small increments (rather
           than one large sleep) prevents problems that can occur when
           the system resumes (awakens) after having been suspended.
           (GH Issue #458 "Hercules crash after resume from suspend")
        */
        for (slept_secs=0; slept_secs < sleep_seconds; ++slept_secs)
        {
            SLEEP( 1 );     /* (sleep one second at a time) */

#if defined( _MSVC_ )
            /* Start over again upon resume from suspend. This should
               hopefully resolve GitHub Issue #489 "Hercules 4.4.1
               crashes after OSA failure" by preventing malfunctioning
               CPU false positives.
            */
            if (sysblk.sys_suspended || sysblk.sys_resumed)
            {
                /* We're either being suspended or resumed */
                sleep_seconds  = WATCHDOG_SECS;
                sleep_secs2nd  = 3;
                slept_secs     = 0;
                hung_cpus_mask = 0;

                /* If being resumed, reset our flags to normal */
                if (!sysblk.sys_suspended && sysblk.sys_resumed)
                {
                    sysblk.sys_suspended = false;
                    sysblk.sys_resumed   = false;
                }

                continue;   /* (start over) */
            }
#endif
        } /* (end for (slept_secs ...) */

#if defined( _MSVC_ )
        // Disable all watchdog logic while debugger is attached
        if (IsDebuggerPresent())
            continue;
#endif
        /* Check for and report any deadlocks */
        if (hthread_report_deadlocks( deadlock_reported ? NULL : "S" ))
        {
            /*****************************************************/
            /*               DEADLOCK DETECTED!                  */
            /*****************************************************/

            if (!deadlock_reported)
            {
                // "DEADLOCK!"
                WRMSG( HHC90024, "S" );
                HDC1( debug_watchdog_signal, NULL );
            }
            deadlock_reported = true;
        }

        for (cpu=0; cpu < sysblk.maxcpu; cpu++)
        {
            /* We're only interested in ONLINE and STARTED CPUs */
            if (0
                || !IS_CPU_ONLINE( cpu )
                || (regs = sysblk.regs[ cpu ])->cpustate != CPUSTATE_STARTED
            )
            {
                /* CPU not ONLINE or not STARTED */
                savecount[ cpu ] = -1;
                continue;
            }

            /* CPU is ONLINE and STARTED. Now check to see if it's
               maybe in a WAITSTATE. If so, we're not interested.
            */
            if (0
                || WAITSTATE( &regs->psw )
#if defined( _FEATURE_WAITSTATE_ASSIST )
                || (1
                    && regs->sie_active
                    && WAITSTATE( &GUESTREGS->psw )
                   )
#endif
            )
            {
                /* CPU is in a WAITSTATE */
                savecount[ cpu ] = -1;
                continue;
            }

            /* We have found a running CPU that should be executing
               instructions. Compare its current instruction count
               with our previously saved value. If they're different
               then it has obviously executed SOME instructions and
               all is well. Save its current instruction counter and
               move on the next CPU. This one appears to be healthy.
            */
            if (INSTCOUNT( regs ) != (U64) savecount[ cpu ])
            {
                /* Save updated instruction count for next time */
                savecount[ cpu ] = INSTCOUNT( regs );
                continue;
            }

            /*****************************************************/
            /*           MALFUNCTIONING CPU DETECTED!            */
            /*****************************************************/

            hung_cpus_mask |= CPU_BIT( cpu );
        }

        /* If any hung CPUs were detected, do a second pass in
           case there is another CPU that also stopped executing
           instructions a few seconds after the first one did.
        */
        if (hung_cpus_mask && sleep_seconds != sleep_secs2nd)
        {
            sleep_seconds = sleep_secs2nd;
            continue;
        }

        /* Report all hung CPUs all at the same time */
        if (hung_cpus_mask && !hung_cpu_reported)
        {
            for (cpu=0; cpu < sysblk.maxcpu; cpu++)
            {
                if (hung_cpus_mask & CPU_BIT( cpu ))
                {
                    // "PROCESSOR %s%02X APPEARS TO BE HUNG!"
                    WRMSG( HHC00822, "S", PTYPSTR( cpu ), cpu );
                    HDC1( debug_watchdog_signal, sysblk.regs[ cpu ]);
                }
            }

            hung_cpu_reported = true;
        }

        /* Create a crash dump if any problems were detected */
        if (deadlock_reported || hung_cpu_reported)
        {
#if defined( _MSVC_ )
            // Give developer time to attach a debugger before crashing
            // If they do so, then prevent the crash from occurring as
            // long as their debugger is still attached, but once they
            // detach their debugger, then go ahead and allow the crash

            // "You have %d seconds to attach a debugger before crash dump will be taken!"
            WRMSG( HHC00823, "S", WAIT_FOR_DEBUGGER_SECS );
            {
                int i;
                for (i=0; !IsDebuggerPresent() && i < WAIT_FOR_DEBUGGER_SECS; ++i)
                    SLEEP( 1 );

                // Don't crash if there is now a debugger attached
                if (IsDebuggerPresent())
                {
                    // "Debugger attached! NOT crashing!"
                    WRMSG( HHC00824, "S" );
                    continue;
                }

                // "TIME'S UP! (or debugger has been detached!) - Forcing crash dump!"
                WRMSG( HHC00825, "S" );
            }
#endif
            /* Display additional debugging information */
            panel_command( "ptt" );
            panel_command( "ipending" );
            panel_command( "locks held sort tid" );
            panel_command( "threads waiting sort tid" );

            /* Display the instruction each hung CPU was executing */
            if (hung_cpus_mask)
            {
                BYTE* ip;
                REGS* regs;

                for (cpu=0; cpu < sysblk.maxcpu; cpu++)
                {
                    if (hung_cpus_mask & CPU_BIT( cpu ))
                    {
                        /* Backup to actual instruction being executed */
                        regs = sysblk.regs[ cpu ];
                        SET_PSW_IA_AND_MAYBE_IP( regs, PSW_IA_FROM_IP( regs, -REAL_ILC( regs )));

                        /* Display instruction that appears to be hung */
                        ip = regs->ip < regs->aip ? regs->inst : regs->ip;
                        ARCH_DEP( display_inst )( regs, ip );
                    }
                }
            }

            /* Give logger thread time to log messages */
            SLEEP(1);

            /* Create the crash dump for offline analysis */
            CRASH();
        }
    }
    while (!sysblk.shutdown);

    return NULL;
}
#endif /* defined( OPTION_WATCHDOG ) */

/*-------------------------------------------------------------------*/
/* Herclin (plain line mode Hercules) message callback function      */
/*-------------------------------------------------------------------*/
void* log_do_callback( void* dummy )
{
    char* msgbuf;
    int   msglen;
    int   msgidx = -1;

    UNREFERENCED( dummy );

    while (!sysblk.shutfini && logger_isactive())
    {
        msglen = log_read( &msgbuf, &msgidx, LOG_NOBLOCK );

        if (msglen)
        {
            log_callback( msgbuf, msglen );
            continue;
        }

        /* wait a bit for new message(s) to arrive before retrying */
        USLEEP( PANEL_REFRESH_RATE_FAST * 1000 );
    }

    /* Let them know logger thread has ended */
    log_callback( NULL, 0 );

    return (NULL);
}

/*-------------------------------------------------------------------*/
/* Return panel command handler to herclin line mode hercules        */
/*-------------------------------------------------------------------*/
DLL_EXPORT  COMMANDHANDLER  getCommandHandler()
{
    return (panel_command);
}

/*-------------------------------------------------------------------*/
/* Process .RC file thread.                                          */
/* Called synchronously when in daemon mode. Else asynchronously.    */
/*-------------------------------------------------------------------*/
static void* process_rc_file( void* dummy )
{
    char  pathname[ MAX_PATH ];

    UNREFERENCED( dummy );

    /* Obtain the name of the hercules.rc file or default */
    if (0
        /* Neither '-r' nor "HERCULES_RC" environment variable given */
        || !cfgorrc[want_rc].filename
        || !cfgorrc[want_rc].filename[0]
    )
    {
        /* Check for "hercules.rc" file in current directory */
        struct stat st;
        if (stat( "hercules.rc", &st ) == 0)
            cfgorrc[want_rc].filename = "hercules.rc";
    }

    /* If we have a hercules.rc file, process it */
    if (1
        && cfgorrc[want_rc].filename
        && cfgorrc[want_rc].filename[0]
        && strcasecmp( cfgorrc[want_rc].filename, "None" ) != 0
    )
    {
        hostpath( pathname, cfgorrc[want_rc].filename, sizeof( pathname ));

#if 1 // ZZ FIXME: THIS NEEDS TO GO

        /* Wait for panel thread to engage */
        if (!sysblk.daemon_mode)
            while (!sysblk.panel_init)
                USLEEP( 10 * 1000 );

#endif // ZZ FIXME: THIS NEEDS TO GO

        /* Run the script processor for this file */
        process_script_file( pathname, true );
    }

    return NULL;
}

/*-------------------------------------------------------------------*/
/* Display cmdline arguments help                                    */
/*-------------------------------------------------------------------*/
static void arghelp()
{
    char   pgm[ MAX_PATH ];
    char*  strtok_str = NULL;

    STRLCPY( pgm, sysblk.hercules_pgmname );

    // "Usage: %s [--help[=SHORT|LONG|VERSION|BUILD]] -f config-filename|\"none\" [-o logfile-name] [-r rcfile-name] [-d] [-b logo-filename] [-s sym=val] [-t [factor]] [-p dyn-load-dir] [[-l dynmod-to-load]...] [> logfile]"
    WRMSG( HHC01407, "S", strtok_r( pgm, ".", &strtok_str ) );

    fflush( stderr );
    fflush( stdout );
    USLEEP( 100000 );
}

/* Functions in module skey.h/.c, needed by impl.c */

extern BYTE s370_get_storage_key( U64 abs );
extern BYTE s390_get_storage_key( U64 abs );
extern BYTE z900_get_storage_key( U64 abs );

/*-------------------------------------------------------------------*/
/* IMPL main entry point                                             */
/*-------------------------------------------------------------------*/
DLL_EXPORT int impl( int argc, char* argv[] )
{
TID     rctid;                          /* RC file thread identifier */
TID     logcbtid;                       /* RC file thread identifier */
int     rc, maxprio, minprio;

    SET_THREAD_NAME( IMPL_THREAD_NAME );

    /* Seed the pseudo-random number generator */
    init_random();

    /* Save minprio/maxprio, which were set in bootstrap.c when it
       called SET_THREAD_NAME at or near the beginning of main().
    */
    minprio = sysblk.minprio;
    maxprio = sysblk.maxprio;

    /* Clear the system configuration block */
    memset( &sysblk, 0, sizeof( SYSBLK ) );

    /* Restore saved minprio/maxprio into SYSBLK */
    sysblk.minprio = minprio;
    sysblk.maxprio = maxprio;

    /* Lock SYSBLK into memory since it's referenced so frequently.
       Note that the call could fail when the working set is small
       but that's okay. We did our best. Locking it isn't critical.
    */
    MLOCK( &sysblk, sizeof( SYSBLK ));

#if defined (_MSVC_)
    _setmaxstdio(2048);
#endif

    INIT_BLOCK_HEADER_TRAILER( (&sysblk), SYSBLK );

    // Set some defaults
    sysblk.msglvl = DEFAULT_MLVL;
    sysblk.logoptnotime = 0;
    sysblk.logoptnodate = 1;

    sysblk.num_pfxs  = (int) strlen( DEF_CMDPREFIXES );
    sysblk.cmd_pfxs  = malloc( sysblk.num_pfxs );
    sysblk.used_pfxs = malloc( sysblk.num_pfxs );

    memcpy( sysblk.cmd_pfxs, DEF_CMDPREFIXES, sysblk.num_pfxs );
    memset( sysblk.used_pfxs,       0,        sysblk.num_pfxs );

    /* Initialize program name and version strings arrays */
    init_progname( argc, argv );
    init_sysblk_version_str_arrays( NULL );

    /* Process two common Unix options right here to avoid displaying
       a bunch of unrelated threading messages on exit which just clutter
       up the output and hide the real information.

       Yes, this is a kludge, but it's not the only one in this thing.
    */
    if (argc == 2 && strcmp(argv[1], "--version") == 0)
    {
        display_version(stdout, 0, NULL);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--usage") == 0)
    {
        arghelp();
        return 0;
    }

    /* Initialize SETMODE and set user authority */
    SETMODE( INIT );

    /* Remain compatible with older external gui versions */
    if (argc >= 1 && strncmp( argv[argc-1], "EXTERNALGUI", 11 ) == 0)
    {
        extgui = TRUE;
        argc--;
    }

    /* Initialize Hercules Threading package */
    hthreads_internal_init();

    /* Initialize 'hostinfo' BEFORE display_version is called */
    init_hostinfo( &hostinfo );

#ifdef _MSVC_
    /* Initialize sockets package */
    VERIFY( socket_init() == 0 );
#endif

    /* Ensure hdl_atexit is called in case of shutdown.
       hdl_atexit ensures entries are called only once.
    */
    atexit( hdl_atexit );

    set_symbol( "VERSION", VERSION);
    set_symbol( "BDATE", __DATE__ );
    set_symbol( "BTIME", __TIME__ );

    {
        char num_procs[64];

        if ( hostinfo.num_packages     != 0 &&
             hostinfo.num_physical_cpu != 0 &&
             hostinfo.num_logical_cpu  != 0 )
        {
            MSGBUF( num_procs, "LP=%d, Cores=%d, CPUs=%d", hostinfo.num_logical_cpu,
                                hostinfo.num_physical_cpu, hostinfo.num_packages );
        }
        else
        {
            if ( hostinfo.num_procs > 1 )
                MSGBUF( num_procs, "MP=%d", hostinfo.num_procs );
            else if ( hostinfo.num_procs == 1 )
                STRLCPY( num_procs, "UP" );
            else
                STRLCPY( num_procs, "" );
        }

        set_symbol( "HOSTNAME", hostinfo.nodename );
        set_symbol( "HOSTOS", hostinfo.sysname );
        set_symbol( "HOSTOSREL", hostinfo.release );
        set_symbol( "HOSTOSVER", hostinfo.version );
        set_symbol( "HOSTARCH", hostinfo.machine );
        set_symbol( "HOSTNUMCPUS", num_procs );
    }

    set_symbol( "MODNAME", sysblk.hercules_pgmname );
    set_symbol( "MODPATH", sysblk.hercules_pgmpath );

    sysblk.sysgroup = DEFAULT_SYSGROUP;

    /* set default console port addresses */
    sysblk.cnslport = strdup("3270");

    /* Initialize automatic creation of missing tape file to default */
    sysblk.auto_tape_create = DEF_AUTO_TAPE_CREATE;

    /* Default command separator is OFF (disabled) */
    sysblk.cmdsep = 0;

#if defined(_FEATURE_SYSTEM_CONSOLE)
    /* set default for scpecho to TRUE */
    sysblk.scpecho = TRUE;

    /* set fault for scpimply to FALSE */
    sysblk.scpimply = FALSE;
#endif

    /* set default system state to reset */
    sysblk.sys_reset = TRUE;

    /* Set default SHCMDOPT to DISABLE NODIAG8 */
    sysblk.shcmdopt = 0;

    /* Save process ID */
    sysblk.hercules_pid = getpid();

    /* Save thread ID of main program */
    sysblk.impltid = thread_id();

    /* Save TOD of when we were first IMPL'ed */
    time( &sysblk.impltime );

    /* Set to LPAR mode with LPAR 1, LPAR ID of 01, and CPUIDFMT 0   */
    sysblk.lparmode = 1;                /* LPARNUM 1    # LPAR ID 01 */
    sysblk.lparnum  = 1;                /* ...                       */
    sysblk.cpuidfmt = 0;                /* CPUIDFMT 0                */
    sysblk.operation_mode = om_mif;     /* Default to MIF operaitons */

    /* set default CPU identifier */
    sysblk.cpumodel = 0x0586;
    sysblk.cpuversion = 0xFD;
    sysblk.cpuserial = 0x000001;
    sysblk.cpuid = createCpuId( sysblk.cpumodel, sysblk.cpuversion,
                                sysblk.cpuserial, 0 );

    sysblk.panrate = PANEL_REFRESH_RATE_SLOW;

    /* set default Program Interrupt Trace */
    sysblk.pgminttr = OS_DEFAULT;
    sysblk.ostailor = OSTAILOR_DEFAULT;

    sysblk.timerint = DEF_TOD_UPDATE_USECS;

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
    sysblk.txf_timerint = sysblk.timerint;
#endif

#if defined( _FEATURE_ECPSVM )
    sysblk.ecpsvm.available = 0;
    sysblk.ecpsvm.level = 20;
#endif

#if defined( OPTION_SHARED_DEVICES )
    sysblk.shrdport = 0;
#endif

    /* setup defaults for CONFIG symbols  */
    {
        char buf[8];

        set_symbol( "LPARNAME", str_lparname());
        set_symbol( "LPARNUM",  "1" );
        set_symbol( "CPUIDFMT", "0" );

        MSGBUF( buf, "%06X", sysblk.cpuserial );
        set_symbol( "CPUSERIAL", buf );

        MSGBUF( buf, "%04X", sysblk.cpumodel );
        set_symbol( "CPUMODEL", buf );

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
        defsym_TXF_models();
#endif
    }

#if defined( _FEATURE_047_CMPSC_ENH_FACILITY )
    sysblk.zpbits  = DEF_CMPSC_ZP_BITS;
#endif

    /* Initialize Trace File helper function pointers */
    sysblk.s370_gsk = &s370_get_storage_key;
    sysblk.s390_gsk = &s390_get_storage_key;
    sysblk.z900_gsk = &z900_get_storage_key;

    sysblk.s370_vtr = &s370_virt_to_real;
    sysblk.s390_vtr = &s390_virt_to_real;
    sysblk.z900_vtr = &z900_virt_to_real;

    sysblk.s370_sit = &s370_store_int_timer;
    sysblk.gct      = &get_cpu_timer;

    /* Initialize locks, conditions, and attributes */
    initialize_lock( &sysblk.tracefileLock );
    initialize_lock( &sysblk.bindlock );
    initialize_lock( &sysblk.config   );
    initialize_lock( &sysblk.todlock  );
    initialize_lock( &sysblk.mainlock );
    initialize_lock( &sysblk.intlock  );
    initialize_lock( &sysblk.iointqlk );
    initialize_lock( &sysblk.sigplock );
    initialize_lock( &sysblk.scrlock  );
    initialize_lock( &sysblk.crwlock  );
    initialize_lock( &sysblk.ioqlock  );
    initialize_lock( &sysblk.dasdcache_lock );
#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
    initialize_lock( &sysblk.rublock );
#endif

    initialize_condition( &sysblk.scrcond );
    initialize_condition( &sysblk.ioqcond );

#if defined( OPTION_SHARED_DEVICES )
    initialize_lock( &sysblk.shrdlock );
    initialize_condition( &sysblk.shrdcond );
    initialize_lock( &sysblk.shrdtracelock );
#endif

    sysblk.mainowner = LOCK_OWNER_NONE;
    sysblk.intowner  = LOCK_OWNER_NONE;

    /* Initialize thread creation attributes so all of hercules
       can use them at any time when they need to create_thread
    */
    initialize_detach_attr( DETACHED );
    initialize_join_attr( JOINABLE );

    /* Initialize CPU ENGINES locks and conditions */
    initialize_condition( &sysblk.cpucond );
    {
        int i; char buf[32];
        for (i=0; i < MAX_CPU_ENGS; i++)
        {
            MSGBUF( buf,    "&sysblk.cpulock[%*d]", MAX_CPU_ENGS > 99 ? 3 : 2, i );
            initialize_lock( &sysblk.cpulock[i] );
            set_lock_name(   &sysblk.cpulock[i], buf );

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
            MSGBUF( buf,    "&sysblk.txf_lock[%*d]", MAX_CPU_ENGS > 99 ? 3 : 2, i );
            initialize_lock( &sysblk.txf_lock[i] );
            set_lock_name(   &sysblk.txf_lock[i], buf );
#endif
        }
    }
    initialize_condition( &sysblk.all_synced_cond );
    initialize_condition( &sysblk.sync_done_cond );

    /* Copy length for regs */
    sysblk.regs_copy_len = (int)((uintptr_t)&sysblk.dummyregs.regs_copy_end
                               - (uintptr_t)&sysblk.dummyregs);

    /* Set the daemon_mode flag indicating whether we running in
       background/daemon mode or not (meaning both stdout/stderr
       are redirected to a non-tty device). Note that this flag
       needs to be set before logger_init gets called since the
       logger_logfile_write function relies on its setting.
    */
    if (!isatty(STDERR_FILENO) && !isatty(STDOUT_FILENO))
        sysblk.daemon_mode = 1;       /* Leave -d intact */

    /* Initialize panel colors */
    set_panel_colors();

    /* Initialize the logmsg pipe and associated logger thread.
       This causes all subsequent logmsg's to be redirected to
       the logger facility for handling by virtue of stdout/stderr
       being redirected to the logger facility.

       NOTE that the logger facility must ALWAYS be initialized
       since other threads depend on it to be able to retrieve
       log messages. This is true regardless of whether or not
       the log messages are being redirected to a hardcopy file.

       The HAO thread (Hercules Automatic Operator) for example
       becomes essentially useless if it's unable to retrieve
       log messages to see whether they match any of its rules.
    */
    logger_init();

    /* Setup the default codepage */
    set_codepage( NULL );

    /* Initialize default HDL modules load directory */
    hdl_initpath( NULL );

    /* Cap the default nice value to zero if setuid is not available */
#if !defined( _MSVC_ )
  #if !defined( NO_SETUID )
    if (sysblk.suid)
  #endif
    {
        if (sysblk.hercnice < 0)
            sysblk.hercnice = 0;
    }
#endif

    /* Initialize default thread priorities */
    sysblk.cpuprio  = DEFAULT_CPU_PRIO;  /* (lowest)  */
    sysblk.devprio  = DEFAULT_DEV_PRIO;  /*     |     */
    sysblk.srvprio  = DEFAULT_SRV_PRIO;  /*     |     */
    sysblk.hercprio = DEFAULT_HERC_PRIO  /*     V     */;
    sysblk.todprio  = DEFAULT_TOD_PRIO;  /* (highest) */

#if defined( BUILD_APPLE_M1 )
    /* Initialize default qos classes high to low */
    sysblk.qos_user_interactive = QOS_CLASS_USER_INTERACTIVE;
    sysblk.qos_user_initiated   = QOS_CLASS_USER_INITIATED;
    sysblk.qos_default          = QOS_CLASS_DEFAULT;
    sysblk.qos_utility          = QOS_CLASS_UTILITY;
    sysblk.qos_background       = QOS_CLASS_BACKGROUND;
#endif

    /* Set the priority of the main Hercules thread */
    SET_THREAD_PRIORITY( sysblk.hercprio, sysblk.qos_user_initiated );
    if (rc != 0)
    {
#if defined( BUILD_APPLE_M1 )
        // "Setting main thread QoS to USER_INITIATED failed: %s"
        WRMSG( HHC00113, "E", strerror( rc ));
#else
        // "set_thread_priority( %d ) failed: %s"
        WRMSG( HHC00109, "E", sysblk.hercprio, strerror( rc ));
#endif

        // "Defaulting all threads to priority %d"
        WRMSG( HHC00110, "W", sysblk.minprio );

        sysblk.cpuprio  = sysblk.minprio;  /* (lowest)  */
        sysblk.devprio  = sysblk.minprio;  /*     |     */
        sysblk.srvprio  = sysblk.minprio;  /*     |     */
        sysblk.hercprio = sysblk.minprio;  /*     V     */
        sysblk.todprio  = sysblk.minprio;  /* (highest) */
    }

    /* PROGRAMMING NOTE: we defer setting thread priorities until AFTER
       the sysblk's default thread priority fields have been properly
       initialized, just as we also defer logging any thread started
       messages until after the logger has been initialized, since it
       is the logger that processes all WRMSG() calls.
    */

    /* Set the priority of the logger thread */
    set_thread_priority_id(sysblk.loggertid, sysblk.srvprio);

    /* Process command-line arguments. Exit if any serious errors. */
    if ((rc = process_args( argc, argv )) != PROCESS_ARGS_OK)
    {
        switch (rc)
        {
          case PROCESS_ARGS_ERROR:
            display_version(stdout, 0, NULL);

            /* Show them our command line arguments */
            arghelp();

            // "Terminating due to %d argument errors"
            WRMSG( HHC02343, "S", rc );
            /* FALLTHROUGH */ // silence GCC warning

          case PROCESS_ARGS_EXIT:
            delayed_exit( rc );
            return rc;
        }
    }

    /* The following comment is incorrect and misleading because
       logger_init() had already been called above, before the
       display_version() call.  So we won't do it again here.
    */

    /* Now display the version information again after logger_init
       has been called so that either the panel display thread or the
       external gui can see the version which was previously possibly
       only displayed to the actual physical screen the first time we
       did it further above (depending on whether we're running in
       daemon_mode (external gui mode) or not). This is the call that
       the panel thread or external gui actually "sees".

       The first call further above wasn't seen by either since it
       was issued before logger_init was called and thus got written
       directly to the physical screen whereas this one will be inter-
       cepted and handled by the logger facility thereby allowing the
       panel thread or external gui to "see" it and thus display it.
    */

    /* Always show version right away */
    display_version(stdout, 0, NULL);
    display_build_options ( stdout, 0 );
    display_extpkg_vers   ( stdout, 0 );

    /* We log these *after* the version number display. */

    /* Log our own thread started message (better late than never) */
    LOG_THREAD_BEGIN(IMPL_THREAD_NAME);

    /* Log the logger thread */
    LOG_TID_BEGIN(sysblk.loggertid, LOGGER_THREAD_NAME);

    /* Warn if crash dumps aren't enabled */
#if !defined( _MSVC_ )
    {
        struct rlimit  core_limit;
        if (getrlimit( RLIMIT_CORE, &core_limit ) == 0)
            sysblk.ulimit_unlimited = (RLIM_INFINITY == core_limit.rlim_cur);
        if (!sysblk.ulimit_unlimited)
            // "Crash dumps NOT enabled"
            WRMSG( HHC00017, "W" );
    }
#endif

    /* Report whether Hercules is running in "elevated" mode or not */
    // HHC00018 "Hercules is %srunning in elevated mode"
    if (are_elevated())
        WRMSG( HHC00018, "I", "" );
    else
        WRMSG( HHC00018, "W", "NOT " );

#if !defined(WIN32) && !defined(HAVE_STRERROR_R)
    strerror_r_init();
#endif

#if defined(OPTION_SCSI_TAPE)
    initialize_lock      ( &sysblk.stape_lock         );
    initialize_condition ( &sysblk.stape_getstat_cond );
    InitializeListHead   ( &sysblk.stape_mount_link   );
    InitializeListHead   ( &sysblk.stape_status_link  );
#endif /* defined(OPTION_SCSI_TAPE) */

    if (sysblk.scrtest)
    {
        // "Hercules is running in test mode."
        WRMSG (HHC00019, "W" );
        if (sysblk.scrfactor != 1.0)
            // "Test timeout factor = %3.1f"
            WRMSG( HHC00021, "I", sysblk.scrfactor );
    }

    /* Set default TCP keepalive values */

#if !defined( HAVE_BASIC_KEEPALIVE )

    WARNING("TCP keepalive headers not found; check configure.ac")
    WARNING("TCP keepalive support will NOT be generated")
    // "This build of Hercules does not support TCP keepalive"
    WRMSG( HHC02321, "E" );

#else // basic, partial or full: must attempt setting keepalive

  #if !defined( HAVE_FULL_KEEPALIVE ) && !defined( HAVE_PARTIAL_KEEPALIVE )

    WARNING("This build of Hercules will only have basic TCP keepalive support")
    // "This build of Hercules has only basic TCP keepalive support"
    WRMSG( HHC02322, "W" );

  #elif !defined( HAVE_FULL_KEEPALIVE )

    #if !defined( SUPPRESS_PARTIAL_KEEPALIVE_WARNING )
    WARNING("This build of Hercules will only have partial TCP keepalive support")
    #endif

    // "This build of Hercules has only partial TCP keepalive support"
    WRMSG( HHC02323, "W" );

  #endif // (basic or partial)

    /*
    **  Note: we need to try setting them to our desired values first
    **  and then retrieve the set values afterwards to detect systems
    **  which do not allow some values to be changed to ensure SYSBLK
    **  gets initialized with proper working default values.
    */
    {
        int rc, sfd, idle, intv, cnt;

        /* Need temporary socket for setting/getting */
        sfd = socket( AF_INET, SOCK_STREAM, 0 );
        if (sfd < 0)
        {
            WRMSG( HHC02219, "E", "socket()", strerror( HSO_errno ));
            idle = 0;
            intv = 0;
            cnt  = 0;
        }
        else
        {
            idle = KEEPALIVE_IDLE_TIME;
            intv = KEEPALIVE_PROBE_INTERVAL;
            cnt  = KEEPALIVE_PROBE_COUNT;

            /* First, try setting the desired values */
            rc = set_socket_keepalive( sfd, idle, intv, cnt );

            if (rc < 0)
            {
                WRMSG( HHC02219, "E", "set_socket_keepalive()", strerror( HSO_errno ));
                idle = 0;
                intv = 0;
                cnt  = 0;
            }
            else
            {
                /* Report partial success */
                if (rc > 0)
                {
                    // "Not all TCP keepalive settings honored"
                    WRMSG( HHC02320, "W" );
                }

                sysblk.kaidle = idle;
                sysblk.kaintv = intv;
                sysblk.kacnt  = cnt;

                /* Retrieve current values from system */
                if (get_socket_keepalive( sfd, &idle, &intv, &cnt ) < 0)
                    WRMSG( HHC02219, "E", "get_socket_keepalive()", strerror( HSO_errno ));
            }
            close_socket( sfd );
        }

        /* Initialize SYSBLK with default values */
        sysblk.kaidle = idle;
        sysblk.kaintv = intv;
        sysblk.kacnt  = cnt;
    }

#endif // (KEEPALIVE)

    /* Initialize runtime opcode tables */
    init_runtime_opcode_tables();

    /* Initialize the Hercules Dynamic Loader (HDL) */
    rc = hdl_main
    (
        &the_real_panel_display,
        &the_real_panel_command,
        &the_real_replace_opcode,
        &ckd_dasd_device_hndinfo,
        &fba_dasd_device_hndinfo
    );

    if (rc != 0)
    {
        delayed_exit( rc );
        return rc;
    }

    /* Load DYNGUI module if needed */
    if (extgui)
    {
        if (hdl_loadmod( "dyngui", HDL_LOAD_NOUNLOAD ) != 0)
        {
            USLEEP( 10000 ); // (give logger time to show them the error message)
            // "Load of dyngui.dll failed, hercules terminated"
            WRMSG( HHC01409, "S" );
            delayed_exit( -1 );
            return 1;
        }
    }

    /* Load modules specified via command-line -l/--ldmod option */
    if (modcount)
    {
        int modnum;
        bool err = false;

        for (modnum = 0; modnum < modcount; modnum++)
        {
            if (hdl_loadmod( modnames[ modnum ], HDL_LOAD_DEFAULT) != 0)
                err = true;

            free( modnames[ modnum ]);
            modnames[ modnum ] = NULL;
        }

        modcount = 0;  // (reset to zero now that they're all loaded)

        if (err)
        {
            USLEEP( 10000 ); // (give logger time to display message)
            // "Hercules terminating, see previous messages for reason"
            WRMSG( HHC01408, "S");
            delayed_exit( -1 );
            return 1;
        }
    }

    /* The SIGINT signal is sent to a process by its controlling
       terminal when a user wishes to interrupt the process. This
       is typically initiated by pressing Ctrl+C, but on some
       systems, the "delete" character or "break" key can be used.

       The SIGTERM signal is sent to a process to request its
       termination. Unlike the SIGKILL signal, it can be caught
       and interpreted or ignored by the process. This allows
       the process to perform nice termination releasing resources
       and saving state if appropriate. SIGINT is nearly identical
       to SIGTERM.
    */

    /* Register the SIGINT handler */
    if (signal( SIGINT, sigint_handler ) == SIG_ERR)
    {
        // "Cannot register %s handler: %s"
        WRMSG( HHC01410, "S", "SIGINT", strerror( errno ));
        delayed_exit( -1 );
        return 1;
    }

    /* Register the SIGTERM handler */
    if (signal( SIGTERM, sigterm_handler ) == SIG_ERR)
    {
        // "Cannot register %s handler: %s"
        WRMSG( HHC01410, "S", "SIGTERM", strerror( errno ));
        delayed_exit( -1 );
        return 1;
    }

#if defined( _MSVC_ )
    /* Register the Window console ctrl handlers */
    if (!IsDebuggerPresent())
    {
        TID dummy;
        BOOL bSuccess = FALSE;

        /* The Windows the console_ctrl_handler function
           accomplishes the same thing as what a SIGINT or
           SIGTERM signal handler does on Unix systems: it
           handles Ctrl+C and Close events.
        */
        if (!SetConsoleCtrlHandler( console_ctrl_handler, TRUE ))
        {
            // "Cannot register %s handler: %s"
            WRMSG( HHC01410, "S", "Console-ctrl", strerror( errno ));
            delayed_exit(-1);
            return 1;
        }

        g_hWndEvt = CreateEvent( NULL, TRUE, FALSE, NULL );

        if ((rc = create_thread( &dummy, DETACHED,
            WinMsgThread, (void*) &bSuccess, "WinMsgThread" )) != 0)
        {
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ));
            CloseHandle( g_hWndEvt );
            delayed_exit(-1);
            return 1;
        }

        // (wait for thread to create window)
        WaitForSingleObject( g_hWndEvt, INFINITE );
        CloseHandle( g_hWndEvt );

        // Was message window successfully created?

        if (!g_hMsgWnd)
        {
            // "Error in function %s: %s"
            WRMSG( HHC00136, "E", "WinMsgThread", "CreateWindowEx() failed");
            delayed_exit(-1);
            return 1;
        }
    }
#endif

#if defined( HAVE_DECL_SIGPIPE ) && HAVE_DECL_SIGPIPE
    /* Ignore the SIGPIPE signal, otherwise Hercules may terminate with
       Broken Pipe error if the printer driver writes to a closed pipe */
    if (signal( SIGPIPE, SIG_IGN ) == SIG_ERR)
    {
        // "Cannot suppress SIGPIPE signal: %s"
        WRMSG( HHC01411, "E", strerror( errno ));
    }
#endif

    {
        int fds[2];
        initialize_lock(&sysblk.cnslpipe_lock);
        initialize_lock(&sysblk.sockpipe_lock);
        sysblk.cnslpipe_flag=0;
        sysblk.sockpipe_flag=0;
        VERIFY( create_pipe(fds) >= 0 );
        sysblk.cnslwpipe=fds[1];
        sysblk.cnslrpipe=fds[0];
        VERIFY( create_pipe(fds) >= 0 );
        sysblk.sockwpipe=fds[1];
        sysblk.sockrpipe=fds[0];
    }

#ifdef HAVE_REXX
    /* Initialize Rexx */
    InitializeRexx();
#endif

    /* System initialisation time */
    sysblk.todstart = hw_clock() << 8;

    hdl_addshut( "release_config", release_config, NULL );

#if defined( OPTION_WATCHDOG )
    /* Start the watchdog thread */
    rc = create_thread( &sysblk.wdtid, DETACHED,
        watchdog_thread, NULL, WATCHDOG_THREAD_NAME );
    if (rc)
    {
        // "Error in function create_thread(): %s"
        WRMSG( HHC00102, "E", strerror( rc ));
        delayed_exit( -1 );
        return 1;
    }
#endif /* defined( OPTION_WATCHDOG ) */

    /* Build system configuration */
    if ( build_config (cfgorrc[want_cfg].filename) )
    {
        delayed_exit(-1);
        return 1;
    }

    sysblk.config_processed = true;
    sysblk.cfg_timerint = sysblk.timerint;

    /* Report CCKD dasd image garbage states at startup */
    cckd_gc_rpt_states();

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )

    if (FACILITY_ENABLED_ARCH( 073_TRANSACT_EXEC, ARCH_900_IDX ))
    {
        txf_model_warning( true );
        txf_set_timerint( true );
    }
    else
        txf_set_timerint( false );

#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */

    /* Process the .rc file synchronously when in daemon mode. */
    /* Otherwise Start up the RC file processing thread.       */
    if (sysblk.daemon_mode)
        process_rc_file( NULL );
    else
    {
        rc = create_thread( &rctid, DETACHED,
            process_rc_file, NULL, "process_rc_file" );
        if (rc)
            WRMSG( HHC00102, "E", strerror( rc ));
    }

    if (log_callback)
    {
        // 'herclin' called us. IT iS in charge. Create its requested
        // logmsg intercept callback function and return back to it.
        rc = create_thread( &logcbtid, DETACHED,
                      log_do_callback, NULL, "log_do_callback" );
        if (rc)
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ));

        return (rc);
    }

    /* Activate the control panel */
    if (!sysblk.daemon_mode)
        panel_display();  /* Returns only AFTER Hercules is shutdown */
    else
    {
        /*-----------------------------------------------*/
        /*            We're in daemon mode...            */
        /*-----------------------------------------------*/

        if (daemon_task)    /* External GUI in control? */
            daemon_task();  /* Returns only when GUI decides to */
        else
        {
            /* daemon mode without any external GUI... */

            process_script_file( "-", true );

            /* We come here only if the user did ctl-d on a tty,
               or we reached EOF on stdin.  No quit command has
               been issued (yet) since that (via do_shutdown())
               would not return.  So we issue the quit here once
               all CPUs have quiesced, since with no CPUs doing
               anything and stdin at EOF, there's no longer any
               reason for us to stick around!

               UNLESS, of course, there were never at any CPUs
               defined/configured to begin with! (NUMCPU 0 was
               specified). Such would be the case when Hercules
               was running solely as a shared dasd server with
               no guest operating IPLed for example.

               In such a case we must continue running in order
               to continue serving shared dasd I/O requests for
               our clients, so we continue running FOREVER. We
               NEVER exit. We can only go away (disappear) when
               the user wants us to, by either manually killing
               us or by issuing the 'quit' command via our HTTP
               Server interface.
            */
            while (0
                || sysblk.started_mask  /* CPU(s) still running?     */
                || !sysblk.config_mask  /* ZERO CPUs configured?     */
            )
                USLEEP( 10 * 1000 );    /* Wait on CPU(s) or forever */

            if (sysblk.config_mask)     /* If NUMCPU > 0, then do    */
                quit_cmd( 0, 0, 0);     /* normal/clean shutdown     */
        }
    }

    /*
    **  PROGRAMMING NOTE: the following code is only ever reached
    **  if Hercules is run in normal panel mode -OR- when it is run
    **  under the control of an external GUI (i.e. daemon_task).
    */
    ASSERT( sysblk.shutdown );  // (why else would we be here?!)

#ifdef _MSVC_
    SetConsoleCtrlHandler( console_ctrl_handler, FALSE );
    socket_deinit();
#endif
    fflush( stdout );
    USLEEP( 10000 );
    return 0; /* return back to bootstrap.c */
} /* end function impl */

/*-------------------------------------------------------------------*/
/* Initialize program name                                           */
/*-------------------------------------------------------------------*/
static void init_progname( int argc, char* argv[] )
{
    free( sysblk.hercules_pgmname );
    free( sysblk.hercules_pgmpath );

    if (argc < 1 || !strlen( argv[0] ))
    {
        sysblk.hercules_pgmname = strdup( "hercules" );
        sysblk.hercules_pgmpath = strdup( "" );
    }
    else
    {
        char path[ MAX_PATH ];

#if defined( _MSVC_ )
        GetModuleFileName( NULL, path, _countof( path ));
#else
        STRLCPY( path, argv[0] );
#endif
        sysblk.hercules_pgmname = strdup( basename( path ));
        sysblk.hercules_pgmpath = strdup( dirname( path ));
    }
}

/*-------------------------------------------------------------------*/
/*         process_args - Process command-line arguments             */
/*-------------------------------------------------------------------*/
static int process_args( int argc, char* argv[] )
{
    int  arg_error = PROCESS_ARGS_OK;
    int  c = 0;                         /* Next option flag          */

    // Save a copy of the command line before getopt mangles argv[]

    if (argc > 0)
    {
        int i;
        size_t len;

        for (len=0, i=0; i < argc; i++ )
            len += strlen( argv[i] ) + 1;

        sysblk.hercules_cmdline = malloc( len );
        strlcpy( sysblk.hercules_cmdline, argv[0], len );

        for (i=1; i < argc; i++)
        {
            strlcat( sysblk.hercules_cmdline,  " ",    len );
            strlcat( sysblk.hercules_cmdline, argv[i], len );
        }
    }

    opterr = 0; /* We'll print our own error messages thankyouverymuch */

    if (2 <= argc && !strcmp(argv[1], "-?"))
    {
        arg_error = PROCESS_ARGS_ERROR;
        goto error;
    }

    for (; EOF != c ;)
    {
        c =                       /* Work area for getopt */

#if defined( HAVE_GETOPT_LONG )
            getopt_long( argc, argv, shortopts, longopts, NULL );
#else
            getopt( argc, argv, shortopts );
#endif

        switch (c)
        {
            case EOF:

                break;      /* (we're done) */

            case 0:         /* getopt_long() set a variable; keep going */

                break;

            case 'V':
                display_version(stdout, 0, NULL);
                arg_error = PROCESS_ARGS_EXIT;
                break;

            case 'h':       /* -h[=type] or --help[=type] */

                if (optarg) /* help type specified? */
                {
                    if (0
                        || strcasecmp( optarg, "short"  ) == 0
                    )
                    {
                        arg_error = PROCESS_ARGS_ERROR;  // (forced by help option)
                        ;   // (do nothing)
                    }
                    else if (0
                        || strcasecmp( optarg, "version" ) == 0
                    )
                    {
                        display_version( stdout, 0, NULL );
                        arg_error = PROCESS_ARGS_EXIT;
                    }
                    else if (0
                        || strcasecmp( optarg, "build"   ) == 0
                    )
                    {
                        display_build_options( stdout, 0 );
                        arg_error = PROCESS_ARGS_EXIT;
                    }
                    else if (0
                        || strcasecmp( optarg, "all"  ) == 0
                        || strcasecmp( optarg, "long" ) == 0
                        || strcasecmp( optarg, "full" ) == 0
                    )
                    {
                        display_version      ( stdout, 0, NULL );
                        display_build_options( stdout, 0 );
                        display_extpkg_vers  ( stdout, 0 );
                        arg_error = PROCESS_ARGS_EXIT;
                    }
                    else
                    {
                        // "Invalid help option argument: %s"
                        WRMSG( HHC00025, "E", optarg );
                        arg_error = PROCESS_ARGS_ERROR;  // (forced by help option)
                    }
                }
                else
                {
                    /* Show them our command line arguments */
                    display_version(stdout, 0, NULL);
                    arghelp();
                    arg_error = PROCESS_ARGS_EXIT;
                }

                break;

            case 'f':

                cfgorrc[ want_cfg ].filename = optarg;
                break;

            case 'o':

                log_sethrdcpy( optarg );
                break;

            case 'r':

                cfgorrc[ want_rc ].filename = optarg;
                break;

            case 's':
            {
                char* sym        = NULL;
                char* value      = NULL;
                char* strtok_str = NULL;

                if (strlen( optarg ) >= 3)
                {
                    sym   = strtok_r( optarg, "=", &strtok_str);
                    value = strtok_r( NULL,   "=", &strtok_str);

                    if (sym && value)
                    {
                        int j;

                        for (j=0; j < (int) strlen( sym ); j++)
                        {
                            if (islower( (unsigned char)sym[j] ))
                                sym[j] = toupper( (unsigned char)sym[j] );
                        }

                        set_symbol( sym, value );
                    }
                    else
                        // "Symbol and/or Value is invalid; ignored"
                        WRMSG( HHC01419, "E" );
                }
                else
                    // "Symbol and/or Value is invalid; ignored"
                    WRMSG( HHC01419, "E" );
            }
            break;

            case 'p':

                hdl_initpath( optarg );
                break;

            case 'l':
            {
                char *dllname, *strtok_str = NULL;

                for(dllname = strtok_r(optarg,", ",&strtok_str);
                    dllname;
                    dllname = strtok_r(NULL,", ",&strtok_str))
                {
                    if (modcount < MAX_MODS)
                        modnames[ modcount++ ] = strdup( dllname ); // (caller will free)
                    else
                    {
                        // "Startup parm -l: maximum loadable modules %d exceeded; remainder not loaded"
                        WRMSG( HHC01406, "W", MAX_MODS );
                        break;
                    }
                }
            }
            break;

            case 'b':

                sysblk.logofile = optarg;
                break;

            case 'v':

                sysblk.msglvl |= MLVL_VERBOSE;
                break;

            case 'd':

                sysblk.daemon_mode = 1;
                break;

            case 'e':

                extgui = 1;
                break;

            case 't':

                sysblk.scrtest = 1;
                sysblk.scrfactor = 1.0;

                if (optarg)
                {
                    double scrfactor;
                    double max_factor = MAX_RUNTEST_FACTOR;

                    /* Round down to nearest 10th of a second */
                    max_factor = floor( max_factor * 10.0 ) / 10.0;
                    scrfactor = atof( optarg );

                    if (scrfactor >= 1.0 && scrfactor <= max_factor)
                        sysblk.scrfactor = scrfactor;
                    else
                    {
                        // "Test timeout factor %s outside of valid range 1.0 to %3.1f"
                        WRMSG( HHC00020, "S", optarg, max_factor );
                        arg_error = PROCESS_ARGS_ERROR;
                    }
                }
                break;

            default:
            {
                char buf[16];

                if (isprint( (unsigned char)optopt ))
                    MSGBUF( buf, "'-%c'", optopt );
                else
                    MSGBUF( buf, "(hex %2.2x)", optopt );

                // "Invalid/unsupported option: %s"
                WRMSG( HHC00023, "S", buf );
                arg_error = PROCESS_ARGS_ERROR;
            }
            break;

        } // end switch(c)
    } // end while

    while (optind < argc)
    {
        // "Unrecognized option: %s"
        WRMSG( HHC00024, "S", argv[ optind++ ]);
        arg_error = PROCESS_ARGS_ERROR;
    }

error:

    /* Terminate if invalid arguments were detected */
    if (arg_error != PROCESS_ARGS_OK)
    {
        /* Do nothing. Caller will call "arghelp" to
          show them our command-line arguments... */
    }
    else /* Check for config and rc file, but don't open */
    {
        struct stat st;
        int i, rv;
        bool using_default;

        for (i=0; cfgorrccount > i; i++)
        {
            using_default = false;

            /* If no value explicitly specified, try environment */
            if (0
                || !cfgorrc[i].filename
                || !cfgorrc[i].filename[0]
            )
            {
                const char* envname = get_symbol( cfgorrc[i].envname );

                if (envname && envname[0])
                    cfgorrc[i].filename = envname;
                else
                {
                    /* If no environment, use our hard coded default */
                    using_default = true;
                    cfgorrc[i].filename = cfgorrc[i].defaultfile;
                }
            }

            /* Explicit request for no file use at all? */
            if (strcasecmp( cfgorrc[i].filename, "none" ) == 0)
            {
               cfgorrc[i].filename = NULL;  /* Suppress file */
               continue;
            }

            /* File specified either explicitly, by environment,
               or by hard coded default: verify its existence.
            */
            if ((rv = stat( cfgorrc[i].filename, &st )) != 0)
            {
                /* If this is the .rc file, default to none */
                if (want_rc == i && using_default)
                {
                   cfgorrc[i].filename = NULL;  /* Suppress file */
                   continue;
                }

                // "%s file '%s' not found: %s"
                WRMSG( HHC02342, "S", cfgorrc[i].whatfile,
                    cfgorrc[i].filename, strerror( errno ));
                arg_error = PROCESS_ARGS_ERROR;
            }
        }
    }

    fflush( stderr );
    fflush( stdout );

    return arg_error;
}
