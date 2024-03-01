/* HSCCMD.C     (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright "Fish" (David B. Trout), 2002-2009     */
/*              (C) Copyright TurboHercules SAS, 2011                */
/*              (C) and others 2013-2023                             */
/*              Execute Hercules System Commands                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module implements the various Hercules System Console        */
/* (i.e. hardware console) commands that the emulator supports.      */
/* To define a new commmand, add an entry to the "Commands" CMDTAB   */
/* table pointing to the command processing function, and optionally */
/* add additional help text to the HelpTab HELPTAB. Both tables are  */
/* near the end of this module.                                      */
/*-------------------------------------------------------------------*/

/* HSCCMD.C has been split into:
 *
 *        hscemode.c        - cpu trace and display commands
 *        hscpufun.c        - cpu functions like ipl, reset
 *        hscloc.c          - locate command
 *        loadmem.c         - load core and load text
 */
/*
   Standard conventions are:

     argc             contains the number of elements in argv[]
     argv[0]          contains the actual command
     argv[1..argc-1]  contains the optional arguments
     cmdline          contains the original command line

     return code:

        = 0     Success
        < 0     Error: Command not executed
        > 1     Failure:  one or more functions could not complete

   int $test_cmd( int argc, char* argv[], char* cmdline )
   {
       .
       .
       .
       return rc
   }
*/

#include "hstdinc.h"

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

#define _HSCCMD_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "history.h"
#include "httpmisc.h"
#include "tapedev.h"
#include "dasdtab.h"
#include "ctcadpt.h"
#include "ctc_ptp.h"
#include "qeth.h"
#include "cckddasd.h"
#include "inline.h"

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/* f? command - display currently defined unusable storage range(s)  */
/*-------------------------------------------------------------------*/
int ARCH_DEP( fquest_cmd )()
{
    U64   aaddr, begbad = 0;
    bool  looking4bad = true, bad, foundbad = false;

    /* Scan all of defined storage to locate all bad frames */
    OBTAIN_INTLOCK( NULL );
    {
        for (aaddr=0; aaddr < sysblk.mainsize; aaddr += STORAGE_KEY_PAGESIZE)
        {
            /* NOTE: we use the internal "_get_storage_key" function
               here so we're returned the internal STORKEY_BADFRM bit.
            */
            bad = ARCH_DEP( _get_storage_key )( aaddr, SKEY_K ) & STORKEY_BADFRM;

            if (looking4bad && bad)
            {
                foundbad = true;
                begbad = aaddr;         /* Beginning of bad range */
                looking4bad = false;
            }
            else if (!looking4bad && !bad)
            {
                // "Storage "F_RADR"-"F_RADR" set to unusable"
                WRMSG( HHC02390, "I", begbad, aaddr - 1);
                looking4bad = true;
            }
        }
    }
    RELEASE_INTLOCK( NULL );

    if (!looking4bad)
        // "Storage "F_RADR"-"F_RADR" set to unusable"
        WRMSG( HHC02390, "I", begbad, aaddr - 1);
    else if (!foundbad)
        // "No unusable storage found"
        WRMSG( HHC02391, "I" );

    return 0;
}

/*-------------------------------------------------------------------*/
/* f- and f+ commands - mark page frame as -unusable or +usable      */
/*-------------------------------------------------------------------*/
int ARCH_DEP( fonoff_cmd )( REGS* regs, char* cmdline )
{
    char*   cmd = cmdline;              /* Copy of panel command     */
    U64     aaddr;                      /* Absolute storage address  */
    U64     saddr, eaddr;               /* Range start/end addresses */
    int     len;                        /* Number of bytes to alter  */
    bool    plus_enable_on;             /* true == x+, false == x-   */
    char    buf[64];                    /* Message buffer            */

    plus_enable_on = (cmd[1] == '+');

    /* Parse the range operand(s) */
    if ((len = parse_range( cmd+2, sysblk.mainsize-1, &saddr, &eaddr, NULL )) < 0)
        return 0; /* (error message already issued) */

    /* Round start/end address to page boundary */
    saddr &= STORAGE_KEY_PAGEMASK;
    eaddr &= STORAGE_KEY_PAGEMASK;

    /* Mark all frames in range as usable or unusable */
    for (aaddr = saddr; aaddr <= eaddr; aaddr += STORAGE_KEY_PAGESIZE)
    {
        if (aaddr > regs->mainlim)
        {
            MSGBUF( buf, F_RADR, aaddr);
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", buf, "" );
            return -1;
        }

        /* Note: we must use the internal "_xxx_storage_key"
           functions to be able to directly set/clear the
           internal STORKEY_BADFRM bit.
        */
        if (plus_enable_on)
            ARCH_DEP( _and_storage_key )( aaddr, STORKEY_BADFRM, SKEY_K );
        else
            ARCH_DEP( _or_storage_key )(  aaddr, STORKEY_BADFRM, SKEY_K );
    }

    MSGBUF( buf, "Storage "F_RADR"-"F_RADR, saddr, aaddr - 1 );
    // "%-14s set to %s"
    WRMSG( HHC02204, "I", buf, plus_enable_on ? "usable" : "unusable" );
    return 0;
}

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "hsccmd.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "hsccmd.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

// (forward references, etc)

#if defined( SIZEOF_INT_P ) && SIZEOF_INT_P >= 8
  #define MAX_DEVLIST_DEVICES   (64*1024)   // 64-bit builds
#else
  #define MAX_DEVLIST_DEVICES   (1024)      // 32-bit builds
#endif

#if defined( _FEATURE_ECPSVM )
extern int ecpsvm_command( int argc, char **argv );
#endif

int HercCmdLine ( char *cmdline );
int exec_cmd(int argc, char *argv[],char *cmdline);

/*-------------------------------------------------------------------*/
/* $test command - do something or other                             */
/*-------------------------------------------------------------------*/

/* $test command helper thread */
static void* test_thread( void* parg)
{
    TID tid = thread_id();                  /* thread identity */
    int rc, secs = (int)(uintptr_t)parg;    /* how long to wait */
    struct timespec ts;                     /* nanosleep argument */

    ts.tv_sec  = secs;
    ts.tv_nsec = 0;

    /* Introduce Heisenberg */
    sched_yield();

    /* Do nanosleep for the specified number of seconds */
    LOGMSG("*** $test thread "TIDPAT": sleeping for %d seconds...\n", TID_CAST( tid ), secs );
    rc = nanosleep( &ts, NULL );
    LOGMSG("*** $test thread "TIDPAT": %d second sleep done; rc=%d\n", TID_CAST( tid ), secs, rc );

    return NULL;
}

/* $test command helper thread */
static void* test_locks_thread( void* parg)
{
    // test thread exit with lock still held
    static LOCK testlock;
    UNREFERENCED(parg);
    initialize_lock( &testlock );
    obtain_lock( &testlock );
    return NULL;
}

static LOCK deadlocks_a;
static LOCK deadlocks_b;
static LOCK deadlocks_c;

/* $test command helper thread */
static void* deadlocks_1( void* parg)
{
    UNREFERENCED( parg );

    // 1 acq a, then b

    obtain_lock( &deadlocks_a );
    {
        SLEEP(1);

        obtain_lock( &deadlocks_b );
        {
            SLEEP(1);
        }
        release_lock( &deadlocks_b );
    }
    release_lock( &deadlocks_a );

    return NULL;
}
static void* deadlocks_2( void* parg)
{
    UNREFERENCED( parg );

    // 2 acq b, then c

    obtain_lock( &deadlocks_b );
    {
        SLEEP(1);

        obtain_lock( &deadlocks_c );
        {
            SLEEP(1);
        }
        release_lock( &deadlocks_c );
    }
    release_lock( &deadlocks_b );

    return NULL;
}
static void* deadlocks_3( void* parg)
{
    UNREFERENCED( parg );

    // 3 acq c, then a

    obtain_lock( &deadlocks_c );
    {
        SLEEP(1);

        obtain_lock( &deadlocks_a );
        {
            SLEEP(1);
        }
        release_lock( &deadlocks_a );
    }
    release_lock( &deadlocks_c );

    return NULL;
}

#define  NUM_THREADS    10
#define  MAX_WAIT_SECS  6

int $test_cmd(int argc, char *argv[],char *cmdline)
{
    int i, secs, rc;
    TID tids[ NUM_THREADS ];

    //UNREFERENCED(argc);
    //UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    if (sysblk.scrtest)
    {
        // "%s%s"
        WRMSG( HHC00001, "E", "", "WRONG! Perhaps you meant 'runtest' instead?");
        return -1;
    }

    if (argc > 1)
    {
        if (CMD( argv[1], CRASH, 5 ))
            CRASH();
        else if (CMD( argv[1], DEADLOCK, 8 ))
        {
            static TID tid;

            initialize_lock( &deadlocks_a );
            initialize_lock( &deadlocks_b );
            initialize_lock( &deadlocks_c );

            set_lock_name( &deadlocks_a, "a" );
            set_lock_name( &deadlocks_b, "b" );
            set_lock_name( &deadlocks_c, "c" );

            VERIFY( create_thread( &tid, DETACHED, deadlocks_1, 0, "#1" ) == 0);
            VERIFY( create_thread( &tid, DETACHED, deadlocks_2, 0, "#2" ) == 0);
            VERIFY( create_thread( &tid, DETACHED, deadlocks_3, 0, "#3" ) == 0);
        }
        else if (CMD( argv[1], LOCKS, 5 ))
        {
            // test thread exit with lock still held
            static TID tid;
            VERIFY( create_thread( &tid, DETACHED,
                test_locks_thread, 0, "test_locks_thread" ) == 0);
        }
        else if (CMD( argv[1], LOCKS2, 6 ))
        {
            // test lock init ALREADY INIT
            static LOCK testlock;
            initialize_lock( &testlock );
            initialize_lock( &testlock );   // (error here)
            destroy_lock( &testlock );
        }
        else if (CMD( argv[1], LOCKS3, 6 ))
        {
            // test lock init ALREADY INIT STILL HELD
            static LOCK testlock;
            initialize_lock( &testlock );
            obtain_lock( &testlock );
            initialize_lock( &testlock );   // (error here)
            release_lock( &testlock );
            destroy_lock( &testlock );
        }
        else if (CMD( argv[1], LOCKS4, 6 ))
        {
            // test destroy lock STILL HELD
            static LOCK testlock;
            initialize_lock( &testlock );
            obtain_lock( &testlock );
            destroy_lock( &testlock );      // (error here)
        }
        else if (CMD( argv[1], NANO, 4 ))
        {
            /*-------------------------------------------*/
            /*             test 'nanosleep'              */
            /*  Use "$test &" to run test in background  */
            /*-------------------------------------------*/

            /* Create the test threads */
            LOGMSG("*** $test command: creating threads...\n");

            for (i=0; i < NUM_THREADS; i++)
            {
                secs = 1 + rand() % MAX_WAIT_SECS;

                if ((rc = create_thread( &tids[i], JOINABLE, test_thread,
                    (void*)(uintptr_t) secs, "test_thread" )) != 0)
                {
                    // "Error in function create_thread(): %s"
                    WRMSG( HHC00102, "E", strerror( rc ));
                    tids[i] = 0;
                }

                secs = rand() % 3;
                if (secs)
                    SLEEP(1);
            }

            /* Wait for all threads to exit */

            LOGMSG("*** $test command: waiting for threads to exit...\n");

            for (i=0; i < NUM_THREADS; i++)
                if (tids[i])
                    join_thread( tids[i], NULL );

            LOGMSG("*** $test command: test complete.\n");
        }
#if defined( HAVE_SIGNAL_HANDLING )
#if defined( HAVE_DECL_SIGBUS ) && HAVE_DECL_SIGBUS
        else if (CMD( argv[1], SIGBUS,  6 )) raise( SIGBUS  );
#endif
        else if (CMD( argv[1], SIGFPE,  6 )) raise( SIGFPE  );
        else if (CMD( argv[1], SIGILL,  6 )) raise( SIGILL  );
        else if (CMD( argv[1], SIGSEGV, 7 )) raise( SIGSEGV );
        else if (CMD( argv[1], SIGUSR1, 7 )) raise( SIGUSR1 );
        else if (CMD( argv[1], SIGUSR2, 7 )) raise( SIGUSR2 );
#endif
        else
        {
            // "%s%s"
            WRMSG( HHC00001, "E", argv[1], ": unknown test");
        }
        return 0;
    }

    // "%s%s"
    WRMSG( HHC00001, "E", argv[0], ": missing argument");

    return 0;
}

/* ---------------------- (end $test command) ---------------------- */

/* Issue generic Device not found error message */
static inline int devnotfound_msg( U16 lcss, U16 devnum )
{
    // HHC02200 "%1d:%04X device not found"
    WRMSG( HHC02200, "E", lcss, devnum );
    return -1;
}

/* Issue generic Missing device number message */
static inline void missing_devnum()
{
    // "Device number missing"
    WRMSG( HHC02201, "E" );
}

/*-------------------------------------------------------------------*/
/* maxrates command  -  report maximum seen mips/sios rates          */
/*-------------------------------------------------------------------*/
int maxrates_cmd(int argc, char *argv[],char *cmdline)
{
    char buf[128];

    UPPER_ARGV_0( argv );

    UNREFERENCED(cmdline);

    if (argc > 1)
    {
        if (argc > 2)
        {
            WRMSG(HHC02205, "E", argv[2], "");
        }
        else if ( CMD( argv[1],midnight,3 ) )
        {
            time_t      current_time;
            struct tm  *current_tm;
            time_t      since_midnight = 0;

            current_time = time( NULL );
            current_tm   = localtime( &current_time );
            since_midnight = (time_t)( ( ( current_tm->tm_hour  * 60 ) +
                                           current_tm->tm_min ) * 60   +
                                           current_tm->tm_sec );
            curr_int_start_time = current_time - since_midnight;

            maxrates_rpt_intvl = 1440;
            WRMSG( HHC02204, "I", argv[0], "midnight" );
        }
        else if (CMD( argv[1], RESET, 5 ))
        {
            curr_high_mips_rate = 0;
            curr_high_sios_rate = 0;
            WRMSG( HHC02268, "I", "Done!" );
            return 0;
        }
        else
        {
            int   interval = 0;
            BYTE  c;
            if ( sscanf( argv[1], "%d%c", &interval, &c ) != 1 || interval < 1 )
            {
                WRMSG(HHC02205, "E", argv[1], ": invalid maxrates interval" );
            }
            else
            {
                MSGBUF( buf, "%d minutes", interval);
                maxrates_rpt_intvl = interval;
                WRMSG(HHC02204, "I", argv[0], buf );
            }
        }
    }
    else
    {
        char*   pszPrevIntervalStartDateTime = NULL;
        char*   pszCurrIntervalStartDateTime = NULL;
        char*   pszCurrentDateTime           = NULL;
        time_t  current_time    = 0;
        int     rc              = TRUE;
        size_t  len             = 0;

        current_time = time( NULL );

        do {
                pszPrevIntervalStartDateTime = strdup( ctime( &prev_int_start_time ) );
                len = strlen(pszPrevIntervalStartDateTime);
                if ( pszPrevIntervalStartDateTime != NULL && len > 0 )
                {
                    pszPrevIntervalStartDateTime[len - 1] = 0;
                }
                else
                {
                    rc = FALSE;
                    break;
                }

                pszCurrIntervalStartDateTime = strdup( ctime( &curr_int_start_time ) );
                len = strlen(pszCurrIntervalStartDateTime);
                if ( pszCurrIntervalStartDateTime != NULL && len > 0 )
                {
                    pszCurrIntervalStartDateTime[len - 1] = 0;
                }
                else
                {
                    rc = FALSE;
                    break;
                }

                pszCurrentDateTime           = strdup( ctime(    &current_time     ) );
                len = strlen(pszCurrentDateTime);
                if ( pszCurrentDateTime != NULL && len > 0 )
                {
                    pszCurrentDateTime[len - 1] = 0;
                }
                else
                {
                    rc = FALSE;
                    break;
                }

                break;

            } while(0);

        if ( rc )
        {
            WRMSG(HHC02268, "I", "Highest observed MIPS and IO/s rates:");
            if ( prev_int_start_time != curr_int_start_time )
            {
                MSGBUF( buf, "From %s to %s", pszPrevIntervalStartDateTime,
                         pszCurrIntervalStartDateTime);
                WRMSG(HHC02268, "I", buf);
                MSGBUF( buf, "MIPS: %d.%02d", prev_high_mips_rate / 1000000,
                         prev_high_mips_rate % 1000000);
                WRMSG(HHC02268, "I", buf);
                MSGBUF( buf, "IO/s: %d", prev_high_sios_rate);
                WRMSG(HHC02268, "I", buf);
            }
            MSGBUF( buf, "From %s to %s", pszCurrIntervalStartDateTime,
                     pszCurrentDateTime);
            WRMSG(HHC02268, "I", buf);
            MSGBUF( buf, "MIPS: %d.%02d", curr_high_mips_rate / 1000000,
                     curr_high_mips_rate % 1000000);
            WRMSG(HHC02268, "I", buf);
            MSGBUF( buf, "IO/s: %d", curr_high_sios_rate);
            WRMSG(HHC02268, "I", buf);
            MSGBUF( buf, "Current interval is %d minutes", maxrates_rpt_intvl);
            WRMSG(HHC02268, "I", buf);
        }
        else
        {
            WRMSG( HHC02219, "E", "strdup()", "zero length");
        }

        free( pszPrevIntervalStartDateTime );
        free( pszCurrIntervalStartDateTime );
        free( pszCurrentDateTime           );
    }

    return 0;   // (make compiler happy)
}

/*-------------------------------------------------------------------*/
/* message command - Display a line of text at the console           */
/*-------------------------------------------------------------------*/
static int message_cmd(int argc,char *argv[], char *cmdline,bool withhdr)
{
    char    *msgtxt;
    time_t  mytime;
    struct  tm *mytm;
    int     toskip,state,i;
    msgtxt=NULL;
    toskip=3;
    if (argc>2)
    {
        if ( CMD(argv[2],AT,2) )
        {
            toskip=5;
        }
    }

    for (state=0,i=0;cmdline[i];i++)
    {
        if (!state)
        {
            if (cmdline[i]!=' ')
            {
                state=1;
                toskip--;
                if (!toskip) break;
            }
        }
        else
        {
            if (cmdline[i]==' ')
            {
                state=0;
                if (toskip==1)
                {
                    i++;
                    toskip=0;
                    break;
                }
            }
        }
    }
    if (!toskip || !cmdline[i])
    {
        msgtxt=&cmdline[i];
    }
    if (msgtxt)
    {
        char* msg = msgtxt;
        char msgbuf[256];

        if (withhdr)
        {
            char *lparname = str_lparname();
            time(&mytime);
            mytm=localtime(&mytime);
            MSGBUF(msgbuf, " %2.2d:%2.2d:%2.2d  * MSG FROM %s: %s",
                     mytm->tm_hour,
                     mytm->tm_min,
                     mytm->tm_sec,
                     (strlen(lparname)!=0)? lparname: "HERCULES",
                     msgtxt );
            msg = msgbuf;
        }

        LOGMSG( "%s\n", msg );
        return 0;
    }
    return -1;
}

/*-------------------------------------------------------------------*/
/* msg/msgnoh command - Display a line of text at the console        */
/*-------------------------------------------------------------------*/
int msg_cmd( int argc, char* argv[], char* cmdline )
{
    int rc;

    UPPER_ARGV_0( argv );

    if (argc < 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    else
    {
        bool withhdr = CMD( argv[0], MSGNOH, 6 ) ? false : true;
        rc = message_cmd( argc, argv, cmdline, withhdr );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* comment command - do absolutely nothing                           */
/*-------------------------------------------------------------------*/
int comment_cmd(int argc, char *argv[],char *cmdline)
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);
    // Do nothing; command has already been echo'ed to console...
    return 0;   // (make compiler happy)
}


/*-------------------------------------------------------------------*/
/* quit or exit command helper thread                                */
/*-------------------------------------------------------------------*/
static void* quit_thread( void* arg )
{
    static const useconds_t  quitdelay_usecs  =
        (2 * WAIT_FOR_KEYBOARD_INPUT_SLEEP_MILLISECS) * 1000;

    UNREFERENCED( arg );

    // Wait twice as long as the panel thread waits for
    // keyboard input so that it has time to process its
    // messages at least twice to ensure that the "exit"
    // command has time to be echoed to the screen.

    USLEEP( quitdelay_usecs );

    // Now proceed with a normal shutdown, which waits for
    // the guest to quiesce itself beforehand (if appropriate)
    // or else simply proceeds with a normal Hercules shutdown
    // via our "do_shutdown_now" helper function.

    do_shutdown();  // ALWAYS go through this function!
    return NULL;
}

/*-------------------------------------------------------------------*/
/* quit or exit command - terminate the emulator                     */
/*-------------------------------------------------------------------*/
int quit_cmd( int argc, char* argv[], char* cmdline )
{
    static TID tid;

    UNREFERENCED( argc );
    UNREFERENCED( argv );
    UNREFERENCED( cmdline );

    if (0
        || (argc > 2 )
        || (argc > 1 && !CMD( argv[1], FORCE, 5 ))
    )
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[argc-1], "" );
        return -1;
    }

    if (argc > 1)
        sysblk.shutimmed = TRUE;  // ('FORCE' option was given)

    // Launch the shutdown from a separate thread only
    // so the "exit" command can be echoed to the panel.

    VERIFY( create_thread( &tid, DETACHED,
        quit_thread, 0, "quit_thread" ) == 0);

    return 0;
}

/*-------------------------------------------------------------------*/
/* quitmout - define maximum wait-for-guest-to-quiesce timeout value */
/*-------------------------------------------------------------------*/
int quitmout_cmd( int argc, char* argv[], char* cmdline )
{
    long quitmout = 0;
    char* endptr  = NULL;
    char buf[16]  = {0};

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    if (argc < 2)
    {
        MSGBUF( buf, "%d", sysblk.quitmout );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );
        return 0;
    }

    if (0
        || argc > 2
        || (quitmout = strtol( argv[1], &endptr, 10 )) < 0
        || (1
            && (0 == quitmout || LONG_MAX == quitmout)
            && (ERANGE == errno || EINVAL == errno)
           )
        || (1
            && *endptr != ' '
            && *endptr != 0
           )
    )
    {
        // "Invalid argument(s). Type 'help %s' for assistance."
        WRMSG( HHC02211, "E", argv[0] );
        return -1;
    }

    MSGBUF( buf, "%d", sysblk.quitmout = (int) quitmout );

    // "%-14s set to %s"
    WRMSG( HHC02204, "I", argv[0], buf );
    return 0;
}

/*-------------------------------------------------------------------*/
/* history command                                                   */
/*-------------------------------------------------------------------*/
int History(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);
    /* last stored command is for sure command 'hst' so remove it
       this is the only place where history_remove is called */
    history_remove();
    history_requested = 1;
    /* only 'hst' called */
    if (argc == 1) {
      if (history_relative_line(-1) == -1)
        history_requested = 0;
      return 0;
    }
    /* hst with argument called */
    if (argc == 2) {
      int x;
      switch (argv[1][0]) {
      case 'l':
        history_show();
        history_requested = 0;
        break;
      default:
        x = atoi(argv[1]);
        if (x>0) {
          if (history_absolute_line(x) == -1)
            history_requested = 0;
        }
        else {
          if (x<0) {
            if (history_relative_line(x) == -1)
              history_requested = 0;
          }
          else {
            /* x == 0 */
            history_show();
            history_requested = 0;
          }
        }
      }
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* log command - direct log output                                   */
/*-------------------------------------------------------------------*/
int log_cmd(int argc, char *argv[], char *cmdline)
{
    int rc = 0;
    UNREFERENCED(cmdline);
    if ( argc > 2 )
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    else if ( argc == 2 )
    {
        if ( CMD(argv[1],off,3) )
            log_sethrdcpy(NULL);
        else
            log_sethrdcpy(argv[1]);
    }
    else
    {
        if ( strlen( log_dsphrdcpy() ) == 0 )
            // "Logger: log switched off"
            WRMSG( HHC02106, "I" );
        else
            // "Logger: log to %s"
            WRMSG( HHC02105, "I", log_dsphrdcpy() );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* logopt command - change log options                               */
/*-------------------------------------------------------------------*/
int logopt_cmd( int argc, char* argv[], char* cmdline)
{
    int i, rc = 0;
    char buf[64];
    bool bDateStamp = !sysblk.logoptnodate;
    bool bTimeStamp = !sysblk.logoptnotime;

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    if (argc <= 1)
    {
        MSGBUF( buf, "%s %s"
            , bDateStamp ? "DATESTAMP" : "NODATESTAMP"
            , bTimeStamp ? "TIMESTAMP" : "NOTIMESTAMP"
        );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );
        return 0;
    }

    // ISO 8601: YYYY-MM-DD

    for (i=1; i < argc; i++)
    {
        if (CMD( argv[i], DATESTAMP, 4 ))
        {
            bDateStamp = true;
            continue;
        }
        if (CMD( argv[i], NODATESTAMP, 6 ))
        {
            bDateStamp = false;
            continue;
        }
        if (CMD( argv[i], TIMESTAMP, 4 ))
        {
            bTimeStamp = true;
            continue;
        }
        if (CMD( argv[i], NOTIMESTAMP, 6 ))
        {
            bTimeStamp = false;
            continue;
        }

        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[i], "" );
        return -1;
    }

    sysblk.logoptnodate = !bDateStamp;
    sysblk.logoptnotime = !bTimeStamp;

    MSGBUF( buf, "%s %s"
        , bDateStamp ? "DATESTAMP" : "NODATESTAMP"
        , bTimeStamp ? "TIMESTAMP" : "NOTIMESTAMP"
    );

    // "%-14s set to %s"
    WRMSG( HHC02204, "I", argv[0], buf );

    return rc;
}

/*-------------------------------------------------------------------*/
/* uptime command - display how long Hercules has been running       */
/*-------------------------------------------------------------------*/

int uptime_cmd(int argc, char *argv[],char *cmdline)
{
int     rc = 0;
time_t  now;
unsigned uptime, weeks, days, hours, mins, secs;

    UNREFERENCED( cmdline );

    if ( argc > 1 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    else
    {
        time( &now );

        uptime = (unsigned) difftime( now, sysblk.impltime );

#define  SECS_PER_MIN     ( 60                 )
#define  SECS_PER_HOUR    ( 60 * SECS_PER_MIN  )
#define  SECS_PER_DAY     ( 24 * SECS_PER_HOUR )
#define  SECS_PER_WEEK    (  7 * SECS_PER_DAY  )

        weeks = uptime /  SECS_PER_WEEK;
                uptime %= SECS_PER_WEEK;
        days  = uptime /  SECS_PER_DAY;
                uptime %= SECS_PER_DAY;
        hours = uptime /  SECS_PER_HOUR;
                uptime %= SECS_PER_HOUR;
        mins  = uptime /  SECS_PER_MIN;
                uptime %= SECS_PER_MIN;
        secs  = uptime;

        if (weeks)
        {
            WRMSG( HHC02206, "I",
                    weeks, weeks >  1 ? "s" : "",
                    days,  days  != 1 ? "s" : "",
                    hours, mins, secs );
        }
        else if (days)
        {
            WRMSG( HHC02207, "I",
                    days, days != 1 ? "s" : "",
                    hours, mins, secs );
        }
        else
        {
            WRMSG( HHC02208, "I",
                    hours, mins, secs );
        }
    }
    return rc;
#undef SECS_PER_MIN
#undef SECS_PER_HOUR
#undef SECS_PER_DAY
#undef SECS_PER_WEEK
}

/*-------------------------------------------------------------------*/
/* version command - display version information                     */
/*-------------------------------------------------------------------*/
int version_cmd(int argc, char *argv[],char *cmdline)
{
int rc = 0;
    UNREFERENCED(cmdline);

    if ( argc > 1 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    else
    {
        display_version      ( stdout, 0, NULL );
        display_build_options( stdout, 0 );
        display_extpkg_vers  ( stdout, 0 );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Display cctape for specified printer                              */
/*-------------------------------------------------------------------*/
int cctape_cmd( int argc, char* argv[], char* cmdline )
{
    int      rc;
    U16      lcss;
    U16      devnum;
    DEVBLK*  dev;
    char*    devclass;
    char     buffer[256];

    UNREFERENCED( cmdline );

    /* Our only argument is the required device number */
    if (argc < 2)
    {
        // "Device number missing"
        WRMSG( HHC02201, "E" );
        return -1 ;
    }

    if ((rc = parse_single_devnum( argv[1], &lcss, &devnum )) < 0)
        return -1;    // (message already displayed)

    if (!(dev = find_device_by_devnum( lcss, devnum )))
    {
        // HHC02200 "%1d:%04X device not found"
        devnotfound_msg( lcss, devnum );
        return -1;
    }

    (dev->hnd->query)( dev, &devclass, 0, NULL );

    if (strcasecmp( devclass, "PRT" ) != 0)
    {
        // "%1d:%04X device is not a %s"
        WRMSG( HHC02209, "E", lcss, devnum, "printer" );
        return -1;
    }

    if (0x1403 != dev->devtype)
    {
        // "command '%s' invalid for device type %04X"
        WRMSG( HHC02239, "E", "cctape", dev->devtype );
        return -1;
    }

    FormatCCTAPE( buffer, sizeof( buffer ),
        dev->lpi, dev->lpp, dev->cctape );

    // "%1d:%04X %s"
    WRMSG( HHC02210, "I", lcss, devnum, buffer );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Display fcb for specified printer                                 */
/*-------------------------------------------------------------------*/
int fcb_cmd( int argc, char* argv[], char* cmdline )
{
    int      rc;
    U16      lcss;
    U16      devnum;
    DEVBLK*  dev;
    char*    devclass;
    char     buffer[256];

    UNREFERENCED( cmdline );

    /* Our only argument is the required device number */
    if (argc < 2)
    {
        // "Device number missing"
        WRMSG( HHC02201, "E" );
        return -1 ;
    }

    if ((rc = parse_single_devnum( argv[1], &lcss, &devnum )) < 0)
        return -1;    // (message already displayed)

    if (!(dev = find_device_by_devnum( lcss, devnum )))
    {
        // HHC02200 "%1d:%04X device not found"
        devnotfound_msg( lcss, devnum );
        return -1;
    }

    (dev->hnd->query)( dev, &devclass, 0, NULL );

    if (strcasecmp( devclass, "PRT" ) != 0)
    {
        // "%1d:%04X device is not a %s"
        WRMSG( HHC02209, "E", lcss, devnum, "printer" );
        return -1;
    }

    if (0x1403 == dev->devtype)
    {
        // "command '%s' invalid for device type %04X"
        WRMSG( HHC02239, "E", "fcb", dev->devtype );
        return -1;
    }

    FormatFCB( buffer, sizeof( buffer ),
        dev->index, dev->lpi, dev->lpp, dev->fcb );

    // "%1d:%04X %s"
    WRMSG( HHC02210, "I", lcss, devnum, buffer );
    return 0;
}

/*-------------------------------------------------------------------*/
/* start command - start CPU (or printer/punch  if argument given)   */
/*-------------------------------------------------------------------*/
int start_cmd(int argc, char *argv[], char *cmdline)
{
    int rc = 0;

    UNREFERENCED(cmdline);

    if (argc < 2 && !is_diag_instr())
    {
        rc = start_cmd_cpu( argc, argv, cmdline );
    }
    else if ( argc == 2 )
    {
        /* start specified printer/punch device */

        U16      devnum;
        U16      lcss;
        int      stopdev;
        DEVBLK*  dev;
        char*    devclass;

        if ( parse_single_devnum(argv[1],&lcss,&devnum) < 0 )
        {
            rc = -1;
        }
        else if (!(dev = find_device_by_devnum (lcss,devnum)))
        {
            // HHC02200 "%1d:%04X device not found"
            devnotfound_msg(lcss,devnum);
            rc = -1;
        }
        else
        {
            (dev->hnd->query)(dev, &devclass, 0, NULL);

            if ( CMD(devclass,PRT,3) || CMD(devclass,PCH,3) )
            {
                if(dev->stopdev == TRUE)
                {
                    /* un-stop the unit record device and raise attention interrupt */
                    /* PRINTER or PUNCH */

                    stopdev = dev->stopdev;

                    dev->stopdev = FALSE;

                    rc = device_attention (dev, CSW_DE);

                    if (rc) dev->stopdev = stopdev;

                    switch (rc) {
                    case 0: WRMSG(HHC02212, "I", lcss,devnum);
                        break;
                    case 1: WRMSG(HHC02213, "E", lcss, devnum, ": busy or interrupt pending");
                        break;
                    case 2: WRMSG(HHC02213, "E", lcss, devnum, ": attention request rejected");
                        break;
                    case 3: WRMSG(HHC02213, "E", lcss, devnum, ": subchannel not enabled");
                        break;
                    }

                    if ( rc != 0 )
                        rc = -1;
                }
                else
                {
                    WRMSG(HHC02213, "W", lcss, devnum, ": already started");
                }
            }
            else
            {
                WRMSG(HHC02209, "E", lcss, devnum, "printer or punch" );
                rc = -1;
            }
        }
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* g command - turn off single stepping and start CPU                */
/*-------------------------------------------------------------------*/
int g_cmd(int argc, char *argv[], char *cmdline)
{
    int i;
    int rc = 0;

    UNREFERENCED(cmdline);

    if ( argc == 1 )
    {
        OBTAIN_INTLOCK(NULL);
        sysblk.instbreak = 0;
        SET_IC_TRACE;
        for (i = 0; i < sysblk.hicpu; i++)
        {
            if (IS_CPU_ONLINE(i) && sysblk.regs[i]->stepwait)
            {
                sysblk.regs[i]->cpustate = CPUSTATE_STARTED;
                WAKEUP_CPU(sysblk.regs[i]);
            }
        }
        RELEASE_INTLOCK(NULL);
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* stop command - stop CPU (or printer device if argument given)     */
/*-------------------------------------------------------------------*/
int stop_cmd(int argc, char *argv[], char *cmdline)
{
    int rc = 0;

    UNREFERENCED(cmdline);

    if (argc < 2 && !is_diag_instr())
    {
        rc = stop_cmd_cpu( argc, argv, cmdline );
    }
    else if ( argc == 2 )
    {
        /* stop specified printer/punch device */

        U16      devnum;
        U16      lcss;
        DEVBLK*  dev;
        char*    devclass;

        if ( parse_single_devnum(argv[1],&lcss,&devnum) < 0 )
        {
            rc = -1;
        }
        else if (!(dev = find_device_by_devnum (lcss, devnum)))
        {
            // HHC02200 "%1d:%04X device not found"
            devnotfound_msg(lcss,devnum);
            rc = -1;
        }
        else
        {
            (dev->hnd->query)(dev, &devclass, 0, NULL);

            if (CMD(devclass,PRT,3) || CMD(devclass,PCH,3) )
            {
                dev->stopdev = TRUE;

                WRMSG(HHC02214, "I", lcss, devnum );
            }
            else
            {
                WRMSG(HHC02209, "E", lcss, devnum, "printer or punch" );
                rc = -1;
            }
        }
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* quiet command - quiet PANEL                                       */
/*-------------------------------------------------------------------*/
int quiet_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);
    UNREFERENCED(argc);
    UNREFERENCED(argv);

    UPPER_ARGV_0( argv );

    if (extgui)
    {
        WRMSG(HHC02215, "W");
        return 0;
    }

    sysblk.npquiet = !sysblk.npquiet;
    // "%-14s: %s"
    WRMSG( HHC02203, "I", argv[0], sysblk.npquiet ? "DISABLED" : "ENABLED" );
    return 0;
}

#if defined( OPTION_IODELAY_KLUDGE )
/*-------------------------------------------------------------------*/
/* iodelay command - display or set I/O delay value                  */
/*-------------------------------------------------------------------*/
int iodelay_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    if ( argc > 2 )
        WRMSG( HHC01455, "E", argv[0] );
    else if ( argc == 2 )
    {
        int     iodelay = 0;
        BYTE    c;                      /* Character work area       */

        if (sscanf(argv[1], "%d%c", &iodelay, &c) != 1 || iodelay < 0 )
            WRMSG(HHC02205, "E", argv[1], "" );
        else
        {
            sysblk.iodelay = iodelay;
            if ( MLVL(VERBOSE) )
                WRMSG(HHC02204, "I", argv[0], argv[1] );
        }
    }
    else
    {
        char msgbuf[8];
        MSGBUF( msgbuf, "%d", sysblk.iodelay );
        WRMSG(HHC02203, "I", argv[0], msgbuf );
    }

    return 0;
}
#endif /* defined( OPTION_IODELAY_KLUDGE ) */

/*-------------------------------------------------------------------*/
/* autoinit_cmd - show or set AUTOINIT switch                        */
/*-------------------------------------------------------------------*/
int autoinit_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc == 2)
    {
        /* Set the option to the requested value */
        if      (CMD( argv[1], ON,  2 )) sysblk.auto_tape_create = TRUE;
        else if (CMD( argv[1], OFF, 3 )) sysblk.auto_tape_create = FALSE;
        else
        {
            // "Missing or invalid argument(s)"
            WRMSG( HHC17000, "E" );
            return -1;
        }
    }
    else if (argc < 1 || argc > 2)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return -1;
    }

    if (argc == 1)
    {
        /* Display current setting */

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], sysblk.auto_tape_create ? "ON" : "OFF" );
    }
    else
    {
        /* Display the setting that was just set */

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], sysblk.auto_tape_create ? "ON" : "OFF" );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*           check_define_default_automount_dir                      */
/*-------------------------------------------------------------------*/
static char* check_define_default_automount_dir()
{
    /* Define default AUTOMOUNT directory if needed */
    if (sysblk.tamdir && sysblk.defdir == NULL)
    {
        char cwd[ MAX_PATH ];
        TAMDIR *pNewTAMDIR = malloc( sizeof(TAMDIR) );
        if (!pNewTAMDIR)
        {
            char buf[64];
            MSGBUF( buf, "malloc(%d)", (int)sizeof(TAMDIR));
            WRMSG(HHC01430, "E", buf, strerror(errno));
            return NULL;
        }
        VERIFY( getcwd( cwd, sizeof(cwd) ) != NULL );
        if (cwd[strlen(cwd)-1] != *PATH_SEP)
            STRLCAT( cwd, PATH_SEP );
        pNewTAMDIR->dir = strdup (cwd);
        pNewTAMDIR->len = (int)strlen (cwd);
        pNewTAMDIR->rej = 0;
        pNewTAMDIR->next = sysblk.tamdir;
        sysblk.tamdir = pNewTAMDIR;
        sysblk.defdir = pNewTAMDIR->dir;
        WRMSG(HHC01447, "I", sysblk.defdir);
    }

    return sysblk.defdir;
}

/*-------------------------------------------------------------------*/
/* Add directory to AUTOMOUNT allowed/disallowed directories list    */
/*                                                                   */
/* Input:  tamdir     pointer to work character array of at least    */
/*                    MAX_PATH size containing an allowed/disallowed */
/*                    directory specification, optionally prefixed   */
/*                    with the '+' or '-' indicator.                 */
/*                                                                   */
/*         ppTAMDIR   address of TAMDIR ptr that upon successful     */
/*                    completion is updated to point to the TAMDIR   */
/*                    entry that was just successfully added.        */
/*                                                                   */
/* Output: upon success, ppTAMDIR is updated to point to the TAMDIR  */
/*         entry just added. Upon error, ppTAMDIR is set to NULL and */
/*         the original input character array is set to the inter-   */
/*         mediate value being processed when the error occurred.    */
/*                                                                   */
/* Returns:  0 == success                                            */
/*           1 == unresolvable path                                  */
/*           2 == path inaccessible                                  */
/*           3 == conflict w/previous                                */
/*           4 == duplicates previous                                */
/*           5 == out of memory                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int add_tamdir( char *tamdir, TAMDIR **ppTAMDIR )
{
    char pathname[MAX_PATH];
    int  rc, rej = 0;
    char dirwrk[ MAX_PATH ] = {0};

    *ppTAMDIR = NULL;

    if (*tamdir == '-')
    {
        rej = 1;
        memmove (tamdir, tamdir+1, MAX_PATH);
    }
    else if (*tamdir == '+')
    {
        rej = 0;
        memmove (tamdir, tamdir+1, MAX_PATH);
    }

    /* Convert tamdir to absolute path ending with a slash */

#if defined( _MSVC_ )
    /* (expand any embedded %var% environment variables) */
    rc = expand_environ_vars( tamdir, dirwrk, MAX_PATH );
    if (rc == 0)
        strlcpy (tamdir, dirwrk, MAX_PATH);
#endif

    if (!realpath( tamdir, dirwrk ))
        return (1); /* ("unresolvable path") */
    strlcpy (tamdir, dirwrk, MAX_PATH);

    hostpath(pathname, tamdir, MAX_PATH);
    strlcpy (tamdir, pathname, MAX_PATH);

    /* Verify that the path is valid */
    if (access( tamdir, R_OK | W_OK ) != 0)
        return (2); /* ("path inaccessible") */

    /* Append trailing path separator if needed */
    rc = (int)strlen( tamdir );
    if (tamdir[rc-1] != *PATH_SEP)
        strlcat (tamdir, PATH_SEP, MAX_PATH);

    /* Check for duplicate/conflicting specification */
    for (*ppTAMDIR = sysblk.tamdir;
         *ppTAMDIR;
         *ppTAMDIR = (*ppTAMDIR)->next)
    {
        if (strfilenamecmp( tamdir, (*ppTAMDIR)->dir ) == 0)
        {
            if ((*ppTAMDIR)->rej != rej)
                return (3); /* ("conflict w/previous") */
            else
                return (4); /* ("duplicates previous") */
        }
    }

    /* Allocate new AUTOMOUNT directory entry */
    *ppTAMDIR = malloc( sizeof(TAMDIR) );
    if (!*ppTAMDIR)
        return (5); /* ("out of memory") */

    /* Fill in the new entry... */
    (*ppTAMDIR)->dir = strdup (tamdir);
    (*ppTAMDIR)->len = (int)strlen (tamdir);
    (*ppTAMDIR)->rej = rej;
    (*ppTAMDIR)->next = NULL;

    /* Add new entry to end of existing list... */
    if (sysblk.tamdir == NULL)
        sysblk.tamdir = *ppTAMDIR;
    else
    {
        TAMDIR *pTAMDIR = sysblk.tamdir;
        while (pTAMDIR->next)
            pTAMDIR = pTAMDIR->next;
        pTAMDIR->next = *ppTAMDIR;
    }

    /* Use first allowable dir as default */
    if (rej == 0 && sysblk.defdir == NULL)
        sysblk.defdir = (*ppTAMDIR)->dir;

    return (0); /* ("success") */
}

/*-------------------------------------------------------------------*/
/* automount_cmd - show or update AUTOMOUNT directories list         */
/*-------------------------------------------------------------------*/
int automount_cmd(int argc, char *argv[], char *cmdline)
{
char pathname[MAX_PATH];
int rc;

    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    if (argc < 2)
    {
        WRMSG(HHC02202, "E", argv[0] );
        return -1;
    }

    check_define_default_automount_dir();

    if ( CMD(argv[1],list,4) )
    {
        TAMDIR* pTAMDIR = sysblk.tamdir;

        if (argc != 2)
        {
            WRMSG(HHC02202, "E", argv[0] );
            return -1;
        }

        if (!pTAMDIR)
        {
            WRMSG(HHC02216, "E");
            return -1;
        }

        // List all entries...

        for (; pTAMDIR; pTAMDIR = pTAMDIR->next)
            WRMSG(HHC02217, "I"
                ,pTAMDIR->rej ? '-' : '+'
                ,pTAMDIR->dir
                );
        return 0;
    }

    if ( CMD(argv[1],add,3) || *argv[1] == '+' )
    {
        char *argv2;
        char tamdir[MAX_PATH+1]; /* +1 for optional '+' or '-' prefix */
        TAMDIR* pTAMDIR = NULL;
//      int was_empty = (sysblk.tamdir == NULL);

        if ( *argv[1] == '+' )
        {
            argv2 = argv[1] + 1;

            if (argc != 2 )
            {
                WRMSG(HHC02202, "E", argv[0] );
                return -1;
            }
        }
        else
        {
            argv2 = argv[2];

            if (argc != 3 )
            {
                WRMSG(HHC02202, "E", argv[0] );
                return -1;
            }
        }


        // Add the requested entry...

        hostpath(pathname, argv2, MAX_PATH);
        strlcpy (tamdir, pathname, MAX_PATH);

        rc = add_tamdir( tamdir, &pTAMDIR );

        // Did that work?

        switch (rc)
        {
            default:     /* (oops!) */
            {
                WRMSG(HHC02218, "E");
                return -1;
            }

            case 5:     /* ("out of memory") */
            {
                WRMSG(HHC02219, "E", "malloc()", strerror(ENOMEM));
                return -1;
            }

            case 1:     /* ("unresolvable path") */
            case 2:     /* ("path inaccessible") */
            {
                WRMSG(HHC02205, "E", tamdir, "");
                return -1;
            }

            case 3:     /* ("conflict w/previous") */
            {
                WRMSG(HHC02205, "E", tamdir, ": 'conflicts with previous specification'");
                return -1;
            }

            case 4:     /* ("duplicates previous") */
            {
                WRMSG(HHC02205, "E", tamdir, ": 'duplicates previous specification'");
                return -1;
            }

            case 0:     /* ("success") */
            {
                char buf[80];
                MSGBUF( buf, "%s%s automount directory", pTAMDIR->dir == sysblk.defdir ? "default " : "",
                    pTAMDIR->rej ? "disallowed" : "allowed");
                WRMSG(HHC02203, "I", buf, pTAMDIR->dir);

                /* Define default AUTOMOUNT directory if needed */

                if (sysblk.defdir == NULL)
                {
                    static char cwd[ MAX_PATH ];

                    VERIFY( getcwd( cwd, sizeof(cwd) ) != NULL );
                    rc = (int)strlen( cwd );
                    if (cwd[rc-1] != *PATH_SEP)
                        STRLCAT( cwd, PATH_SEP );

                    if (!(pTAMDIR = malloc( sizeof(TAMDIR) )))
                    {
                        char buf[40];
                        MSGBUF( buf, "malloc(%d)", (int)sizeof(TAMDIR));
                        WRMSG(HHC02219, "E", buf, strerror(ENOMEM));
                        sysblk.defdir = cwd; /* EMERGENCY! */
                    }
                    else
                    {
                        pTAMDIR->dir = strdup (cwd);
                        pTAMDIR->len = (int)strlen (cwd);
                        pTAMDIR->rej = 0;
                        pTAMDIR->next = sysblk.tamdir;
                        sysblk.tamdir = pTAMDIR;
                        sysblk.defdir = pTAMDIR->dir;
                    }

                    WRMSG(HHC02203, "I", "default automount directory", sysblk.defdir);
                }

                return 0;
            }
        }
    }

    if ( CMD(argv[1],del,3) || *argv[1] == '-')
    {
        char *argv2;
        char tamdir1[MAX_PATH+1] = {0};     // (resolved path)
        char tamdir2[MAX_PATH+1] = {0};     // (expanded but unresolved path)
        char workdir[MAX_PATH+1] = {0};     // (work)
        char *tamdir = tamdir1;             // (-> tamdir2 on retry)

        TAMDIR* pPrevTAMDIR = NULL;
        TAMDIR* pCurrTAMDIR = sysblk.tamdir;

//      int was_empty = (sysblk.tamdir == NULL);

        if ( *argv[1] == '-' )
        {
            argv2 = argv[1] + 1;

            if (argc != 2 )
            {
                WRMSG(HHC02202, "E", argv[0] );
                return -1;
            }
        }
        else
        {
            argv2 = argv[2];

            if (argc != 3 )
            {
                WRMSG(HHC02202, "E", argv[0] );
                return -1;
            }
        }

        // Convert argument to absolute path ending with a slash

        STRLCPY( tamdir2, argv2 );
        if      (tamdir2[0] == '-') memmove (&tamdir2[0], &tamdir2[1], MAX_PATH);
        else if (tamdir2[0] == '+') memmove (&tamdir2[0], &tamdir2[1], MAX_PATH);

#if defined( _MSVC_ )
        // (expand any embedded %var% environment variables)
        rc = expand_environ_vars( tamdir2, workdir, MAX_PATH );
        if (rc == 0)
            strlcpy (tamdir2, workdir, MAX_PATH);
#endif

        if (sysblk.defdir == NULL
#if defined( _MSVC_ )
            || tamdir2[1] == ':'    // (fullpath given?)
#else
            || tamdir2[0] == '/'    // (fullpath given?)
#endif
            || tamdir2[0] == '.'    // (relative path given?)
        )
            tamdir1[0] = 0;         // (then use just given spec)
        else                        // (else prepend with default)
            STRLCPY( tamdir1, sysblk.defdir );

        // (finish building path to be resolved)
        STRLCAT( tamdir1, tamdir2 );

        // (try resolving it to an absolute path and
        //  append trailing path separator if needed)

        if (realpath(tamdir1, workdir) != NULL)
        {
            strlcpy (tamdir1, workdir, MAX_PATH);
            rc = (int)strlen( tamdir1 );
            if (tamdir1[rc-1] != *PATH_SEP)
                strlcat (tamdir1, PATH_SEP, MAX_PATH);
            tamdir = tamdir1;   // (try tamdir1 first)
        }
        else
            tamdir = tamdir2;   // (try only tamdir2)

        rc = (int)strlen( tamdir2 );
        if (tamdir2[rc-1] != *PATH_SEP)
            strlcat (tamdir2, PATH_SEP, MAX_PATH);

        hostpath(pathname, tamdir2, MAX_PATH);
        strlcpy (tamdir2, pathname, MAX_PATH);

        // Find entry to be deleted...

        for (;;)
        {
            for (pCurrTAMDIR = sysblk.tamdir, pPrevTAMDIR = NULL;
                pCurrTAMDIR;
                pPrevTAMDIR = pCurrTAMDIR, pCurrTAMDIR = pCurrTAMDIR->next)
            {
                if (strfilenamecmp( pCurrTAMDIR->dir, tamdir ) == 0)
                {
                    int def = (sysblk.defdir == pCurrTAMDIR->dir);

                    // Delete the found entry...

                    if (pPrevTAMDIR)
                        pPrevTAMDIR->next = pCurrTAMDIR->next;
                    else
                        sysblk.tamdir = pCurrTAMDIR->next;

                    free( pCurrTAMDIR->dir );
                    free( pCurrTAMDIR );

                    // (point back to list begin)
                    pCurrTAMDIR = sysblk.tamdir;

                    WRMSG(HHC02220, "I", pCurrTAMDIR ? "" : ", list now empty");

                    // Default entry just deleted?

                    if (def)
                    {
                        if (!pCurrTAMDIR)
                            sysblk.defdir = NULL;  // (no default)
                        else
                        {
                            // Set new default entry...

                            for (; pCurrTAMDIR; pCurrTAMDIR = pCurrTAMDIR->next)
                            {
                                if (pCurrTAMDIR->rej == 0)
                                {
                                    sysblk.defdir = pCurrTAMDIR->dir;
                                    break;
                                }
                            }

                            // If we couldn't find an existing allowable
                            // directory entry to use as the new default,
                            // then add the current directory and use it.

                            if (!pCurrTAMDIR)
                            {
                                static char cwd[ MAX_PATH ] = {0};

                                VERIFY( getcwd( cwd, sizeof(cwd) ) != NULL );
                                rc = (int)strlen( cwd );
                                if (cwd[rc-1] != *PATH_SEP)
                                    STRLCAT( cwd, PATH_SEP );

                                if (!(pCurrTAMDIR = malloc( sizeof(TAMDIR) )))
                                {
                                    char buf[40];
                                    MSGBUF( buf, "malloc(%d)", (int)sizeof(TAMDIR));
                                    WRMSG(HHC02219, "E", buf, strerror(ENOMEM));
                                    sysblk.defdir = cwd; /* EMERGENCY! */
                                }
                                else
                                {
                                    pCurrTAMDIR->dir = strdup (cwd);
                                    pCurrTAMDIR->len = (int)strlen (cwd);
                                    pCurrTAMDIR->rej = 0;
                                    pCurrTAMDIR->next = sysblk.tamdir;
                                    sysblk.tamdir = pCurrTAMDIR;
                                    sysblk.defdir = pCurrTAMDIR->dir;
                                }
                            }

                            WRMSG(HHC02203, "I", "default automount directory", sysblk.defdir);
                        }
                    }

                    return 0;   // (success)
                }
            }

            // (not found; try tamdir2 if we haven't yet)

            if (tamdir == tamdir2) break;
            tamdir = tamdir2;
        }

        if (sysblk.tamdir == NULL)
            WRMSG(HHC02216, "E");
        else
            WRMSG(HHC02221, "E");
        return -1;
    }

    WRMSG(HHC02222, "E");
    return 0;
}

#if defined( OPTION_SCSI_TAPE )

// (helper function for 'scsimount' and 'devlist' commands)
static void try_scsi_refresh( DEVBLK* dev )
{
    // PROGRAMMING NOTE: we can only ever cause the auto-scsi-mount
    // thread to startup or shutdown [according to the current user
    // setting] if the current drive status is "not mounted".

    // What we unfortunately CANNOT do (indeed MUST NOT do!) however
    // is actually "force" a refresh of a current [presumably bogus]
    // "mounted" status (to presumably detect that a tape that was
    // once mounted has now been manually unmounted for example).

    // The reasons for why this is not possible is clearly explained
    // in the 'update_status_scsitape' function in 'scsitape.c'.

    // If the user manually unloaded a mounted tape (such that there
    // is now no longer a tape mounted even though the drive status
    // says there is), then they unfortunately have no choice but to
    // manually issue the 'devinit' command themselves, because, as
    // explained, we unfortunately cannot refresh a mounted status
    // for them (due to the inherent danger of doing so as explained
    // by comments in 'update_status_scsitape' in member scsitape.c).

    GENTMH_PARMS  gen_parms;

    gen_parms.action  = GENTMH_SCSI_ACTION_UPDATE_STATUS;
    gen_parms.dev     = dev;

    VERIFY( dev->tmh->generic( &gen_parms ) == 0 ); // (maybe update status)
    USLEEP( 10 * 1000 );                            // (let thread start/end)
}

/*-------------------------------------------------------------------*/
/* scsimount command - display or adjust the SCSI auto-mount option  */
/*-------------------------------------------------------------------*/
int scsimount_cmd(int argc, char *argv[], char *cmdline)
{
    DEVBLK*  dev;
    int      tapeloaded;
    char*    tapemsg="";
    char     volname[7];
    BYTE     mountreq, unmountreq;
    char*    label_type;
    char     buf[512];

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if (argc == 2)
    {
        if ( CMD(argv[1],no,2) )
        {
            sysblk.auto_scsi_mount_secs = 0;
        }
        else if ( CMD(argv[1],yes,3) )
        {
            sysblk.auto_scsi_mount_secs = DEF_SCSIMOUNT_SECS;
        }
        else
        {
            int auto_scsi_mount_secs; BYTE c;
            if ( sscanf( argv[1], "%d%c", &auto_scsi_mount_secs, &c ) != 1
                || auto_scsi_mount_secs < 0 || auto_scsi_mount_secs > 99 )
            {
                WRMSG (HHC02205, "E", argv[1], "");
                return -1;
            }
            sysblk.auto_scsi_mount_secs = auto_scsi_mount_secs;
        }
        if ( MLVL(VERBOSE) )
        {
            WRMSG( HHC02204, "I", argv[0], argv[1] );
            return 0;
        }
    }

    if ( MLVL(VERBOSE) )
    {
        if ( sysblk.auto_scsi_mount_secs )
        {
            MSGBUF( buf, "%d", sysblk.auto_scsi_mount_secs );
            WRMSG(HHC02203, "I", argv[0], buf);
        }
        else
            WRMSG(HHC02203, "I", argv[0], "NO");
    }

    // Scan the device list looking for all SCSI tape devices
    // with either an active scsi mount thread and/or an out-
    // standing tape mount request...

    for ( dev = sysblk.firstdev; dev; dev = dev->nextdev )
    {
        if ( !dev->allocated || TAPEDEVT_SCSITAPE != dev->tapedevt )
            continue;  // (not an active SCSI tape device; skip)

        try_scsi_refresh( dev );    // (see comments in function)

        MSGBUF( buf,
            "thread %s active for drive %u:%4.4X = %s"
            ,dev->stape_mntdrq.link.Flink ? "IS" : "is NOT"
            ,SSID_TO_LCSS(dev->ssid)
            ,dev->devnum
            ,dev->filename
        );
        WRMSG(HHC02275, "I", buf);

        if (!dev->tdparms.displayfeat)
            continue;

        mountreq   = FALSE;     // (default)
        unmountreq = FALSE;     // (default)

        if (0
            || TAPEDISPTYP_MOUNT       == dev->tapedisptype
            || TAPEDISPTYP_UNMOUNT     == dev->tapedisptype
            || TAPEDISPTYP_UMOUNTMOUNT == dev->tapedisptype
        )
        {
            tapeloaded = dev->tmh->tapeloaded( dev, NULL, 0 );

            if ( TAPEDISPTYP_MOUNT == dev->tapedisptype && !tapeloaded )
            {
                mountreq   = TRUE;
                unmountreq = FALSE;
                tapemsg = dev->tapemsg1;
            }
            else if ( TAPEDISPTYP_UNMOUNT == dev->tapedisptype && tapeloaded )
            {
                unmountreq = TRUE;
                mountreq   = FALSE;
                tapemsg = dev->tapemsg1;
            }
            else // ( TAPEDISPTYP_UMOUNTMOUNT == dev->tapedisptype )
            {
                if (tapeloaded)
                {
                    if ( !(dev->tapedispflags & TAPEDISPFLG_MESSAGE2) )
                    {
                        unmountreq = TRUE;
                        mountreq   = FALSE;
                        tapemsg = dev->tapemsg1;
                    }
                }
                else // (!tapeloaded)
                {
                    mountreq   = TRUE;
                    unmountreq = FALSE;
                    tapemsg = dev->tapemsg2;
                }
            }
        }

        if ((mountreq || unmountreq) && ' ' != *tapemsg)
        {
            switch (*(tapemsg+7))
            {
                case 'A': label_type = "ascii-standard"; break;
                case 'S': label_type = "standard"; break;
                case 'N': label_type = "non"; break;
                case 'U': label_type = "un"; break;
                default : label_type = "??"; break;
            }

            volname[0]=0;

            if (*(tapemsg+1))
            {
                strncpy( volname, tapemsg+1, 6 );
                volname[6]=0;
            }

            WRMSG(HHC02223, "I"
                    ,mountreq ? "Mount" : "Dismount"
                    ,label_type
                    ,volname
                    ,SSID_TO_LCSS(dev->ssid)
                    ,dev->devnum
                    ,dev->filename);
        }
        else
        {
            MSGBUF( buf, "no requests pending for drive %u:%4.4X = %s.",
                LCSS_DEVNUM, dev->filename );
            WRMSG(HHC02275, "I", buf);
        }
    }

    return 0;
}
#endif /* defined( OPTION_SCSI_TAPE ) */

/*-------------------------------------------------------------------*/
/* cckd command                                                      */
/*-------------------------------------------------------------------*/
int cckd_cmd(int argc, char *argv[], char *cmdline)
{
    char*   p;
    int     rc = -1;
    char*   strtok_str = NULL;
    bool    bVerbose = MLVL( VERBOSE ) ? true : false;

    if (0
        || argc != 2
        || !cmdline
        || strlen( cmdline ) < 5
        || !(p = strtok_r( cmdline + 4, " \t", &strtok_str ))
    )
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
    else
        rc = cckd_command( p, bVerbose );

    return rc;
}

/*-------------------------------------------------------------------*/
/* ctc command - enable/disable CTC debugging                        */
/*-------------------------------------------------------------------*/
int ctc_cmd( int argc, char *argv[], char *cmdline )
{
    DEVBLK*  dev;
    CTCBLK*  pCTCBLK;
    PTPATH*  pPTPATH;
    PTPBLK*  pPTPBLK;
    LCSDEV*  pLCSDEV;
    LCSBLK*  pLCSBLK;
    U16      lcss;
    U16      devnum;
    u_int    mask = 0;
    BYTE     onoff = FALSE;
    BYTE     startup = FALSE;
    BYTE     all = FALSE;
    BYTE     invalid = FALSE;
    int      iTraceLen = LCS_TRACE_LEN_DEFAULT;
    int      iDiscTrace = LCS_DISC_TRACE_ZERO;
    int      i;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    // Format:  "ctc  debug  { on | off }  [ <devnum> | ALL ]"

    /* Check that there are at least two tokens */
    if (argc < 2)
    {
        invalid = TRUE;
    }

    /* Check the second token is debug. */
    if (argc >= 2)
    {
        if (!CMD(argv[1],debug,5))
        {
            invalid = TRUE;
        }
    }

    /* Check the third token is startup, on or off. */
    if (argc >= 3)
    {
        if (CMD(argv[2],startup,7))
        {
            startup = TRUE;
        }
        else if (CMD(argv[2],on,2))
        {
            onoff = TRUE;
        }
        else if (!CMD(argv[2],off,3))
        {
            invalid = TRUE;
        }
    }
    else // (argc < 3)
    {
        // "ctc debug" by itself lists the CTC debugging state for all CTC devices

        static const char* yes_startup  = "STARTUP";
        static const char* yes_on       = "ON";
        static const char* no_off       = "OFF";
        const char* on_or_off;

        for ( dev = sysblk.firstdev; dev; dev = dev->nextdev )
        {
            if (0
                || !dev->allocated
                || 0x3088 != dev->devtype
                || (1
                    && CTC_CTCI != dev->ctctype
                    && CTC_LCS  != dev->ctctype
                    && CTC_PTP  != dev->ctctype
                    && CTC_CTCE != dev->ctctype
                   )
            )
                continue;

            if (CTC_CTCI == dev->ctctype)
            {
                pCTCBLK = dev->dev_data;
                on_or_off = (pCTCBLK->fDebug) ? yes_on : no_off;
            }
            else if (CTC_LCS == dev->ctctype)
            {
                pLCSDEV = dev->dev_data;
                pLCSBLK = pLCSDEV->pLCSBLK;
                on_or_off = (pLCSBLK->fDebug) ? yes_on : no_off;
            }
            else if (CTC_CTCE == dev->ctctype)  /* CTCE is not grouped. */
            {
                if (dev->ctce_trace_cntr >= 0)
                    on_or_off = yes_startup;
                else
                    on_or_off = (CTCE_TRACE_ON == dev->ctce_trace_cntr) ? yes_on : no_off;
            }
            else // (CTC_PTP == dev->ctctype)
            {
                pPTPATH = dev->dev_data;
                pPTPBLK = pPTPATH->pPTPBLK;
                on_or_off = (pPTPBLK->uDebugMask) ? yes_on : no_off;
            }

            // "%1d:%04X: CTC DEBUG is %s"
            WRMSG( HHC00903, "I", LCSS_DEVNUM, on_or_off );
        }

        return 0;
    }

    /* Check whether there is a fourth token. If there isn't, assume the fourth token is all.  */
    if (argc < 4)
    {
        all = TRUE;
    }

    /* Check the fourth token is all or a device address. */
    if (argc >= 4)
    {
        if (CMD(argv[3],ALL,3))
        {
            all = TRUE;
        }
        else if (parse_single_devnum( argv[3], &lcss, &devnum) != 0)
        {
            invalid = TRUE;
        }
    }

    /* Check the fifth and later keyword and value tokens that are optional with on. */
    if (argc >= 5)
    {
        if( onoff )
        {
            for ( i = 4; i < argc; i++ )
            {
                if (CMD(argv[i],trace,2))
                {
                    if ( (i + 1) < argc )
                    {
                        i++;
                        iTraceLen = atoi( argv[i] );
                        if ((iTraceLen < LCS_TRACE_LEN_MINIMUM || iTraceLen > LCS_TRACE_LEN_MAXIMUM) && iTraceLen != LCS_TRACE_LEN_ZERO)
                        {
                            invalid = TRUE;
                        }
                    }
                    else
                    {
                        invalid = TRUE;
                    }
                }
                else if (CMD(argv[i],discard,3))
                {
                    if ( (i + 1) < argc )
                    {
                        i++;
                        iDiscTrace = atoi( argv[i] );
                        if ((iDiscTrace < LCS_DISC_TRACE_MINIMUM || iDiscTrace > LCS_DISC_TRACE_MAXIMUM) && iDiscTrace != LCS_DISC_TRACE_ZERO)
                        {
                            invalid = TRUE;
                        }
                    }
                    else
                    {
                        invalid = TRUE;
                    }
                }
                else
                {
                    invalid = TRUE;
                    break;
                }
            }
        }
        else
        {
            invalid = TRUE;
        }
    }

    /* Check whether the entered command is invalid. */
    if (invalid)
    {
        /* HHC02299 "Invalid command usage. Type 'help %s' for assistance." */
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /* Prepare the default debug mask for CTC_PTP devices with on. */
    if( onoff )
    {
        mask = DBGPTPPACKET;
    }

    /* */
    if (all)
    {
        for ( dev = sysblk.firstdev; dev; dev = dev->nextdev )
        {
            if (0
                || !dev->allocated
                || 0x3088 != dev->devtype
                || (1
                    && CTC_CTCI != dev->ctctype
                    && CTC_LCS  != dev->ctctype
                    && CTC_PTP  != dev->ctctype
                    && CTC_CTCE != dev->ctctype
                   )
            )
                continue;

            if (CTC_CTCI == dev->ctctype)
            {
                pCTCBLK = dev->dev_data;
                pCTCBLK->fDebug = onoff;
            }
            else if (CTC_LCS == dev->ctctype)
            {
                pLCSDEV = dev->dev_data;
                pLCSBLK = pLCSDEV->pLCSBLK;
                pLCSBLK->fDebug = onoff;
                pLCSBLK->iTraceLen = iTraceLen;
                pLCSBLK->iDiscTrace = iDiscTrace;
            }
            else if (CTC_CTCE == dev->ctctype)  /* CTCE is not grouped. */
            {
                if (onoff)
                {
                    dev->ctce_trace_cntr = CTCE_TRACE_ON;
                }
                else if (startup)
                {
                    dev->ctce_trace_cntr = CTCE_TRACE_STARTUP;
                }
                else
                {
                    dev->ctce_trace_cntr = CTCE_TRACE_OFF;
                }
            }
            else // (CTC_PTP == dev->ctctype)
            {
                pPTPATH = dev->dev_data;
                pPTPBLK = pPTPATH->pPTPBLK;
                pPTPBLK->uDebugMask = mask;
            }
        }

        /* HHC02204 "%-14s set to %s" */
        WRMSG(HHC02204, "I", "CTC DEBUG", startup ? "startup ALL" : onoff ? "on ALL" : "off ALL");
    }
    else
    {
        int acount;
        DEVGRP* pDEVGRP;
        DEVBLK* pDEVBLK;

        if (!(dev = find_device_by_devnum ( lcss, devnum )))
        {
            // HHC02200 "%1d:%04X device not found"
            devnotfound_msg( lcss, devnum );
            return -1;
        }

        pDEVGRP = dev->group;
        acount = 0;

        if (CTC_CTCI == dev->ctctype)
        {
            if (pDEVGRP && pDEVGRP->acount)  /* CTCI should be a group of two devices. */
            {
                pDEVBLK = pDEVGRP->memdev[0];
                pCTCBLK = pDEVBLK->dev_data;
                pCTCBLK->fDebug = onoff;
                acount = pDEVGRP->acount;
            }
        }
        else if (CTC_LCS == dev->ctctype)  /* LCS should be a group of one or more devices. */
        {
            if (pDEVGRP && pDEVGRP->acount)
            {
                pDEVBLK = pDEVGRP->memdev[0];
                pLCSDEV = pDEVBLK->dev_data;
                pLCSBLK = pLCSDEV->pLCSBLK;
                pLCSBLK->fDebug = onoff;
                pLCSBLK->iTraceLen = iTraceLen;
                pLCSBLK->iDiscTrace = iDiscTrace;
                acount = pDEVGRP->acount;
            }
        }
        else if (CTC_PTP == dev->ctctype)  /* PTP should be a group of two devices. */
        {
            if (pDEVGRP && pDEVGRP->acount)
            {
                pDEVBLK = pDEVGRP->memdev[0];
                pPTPATH = pDEVBLK->dev_data;
                pPTPBLK = pPTPATH->pPTPBLK;
                pPTPBLK->uDebugMask = mask;
                acount = pDEVGRP->acount;
            }
        }
        else if (CTC_CTCE == dev->ctctype)  /* CTCE is not grouped. */
        {
            if (onoff)
            {
                dev->ctce_trace_cntr = CTCE_TRACE_ON;
            }
            else if (startup)
            {
                dev->ctce_trace_cntr = CTCE_TRACE_STARTUP;
            }
            else
            {
                dev->ctce_trace_cntr = CTCE_TRACE_OFF;
            }
        }
        else
        {
            /* HHC02209 "%1d:%04X device is not a %s" */
            WRMSG(HHC02209, "E", lcss, devnum, "supported CTCI, LSC, PTP or CTCE" );
            return -1;
        }

        {
          char buf1[32] = {0};
          char buf2[128];
          if (acount) {
            MSGBUF( buf1, " group (%d devices)", acount);
          }
          MSGBUF( buf2, "%s for %s device %1d:%04X%s",
                  startup ? "STARTUP" : onoff ? "ON" : "OFF",
                  CTC_CTCE == dev->ctctype ? "CTCE" : CTC_LCS == dev->ctctype ? "LCS" : CTC_PTP == dev->ctctype ? "PTP" : "CTCI",
                  lcss, devnum,
                  buf1 );
          /* HHC02204 "%-14s set to %s" */
          WRMSG(HHC02204, "I", "CTC DEBUG", buf2);
        }
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* ptp command - enable/disable PTP debugging                        */
/*-------------------------------------------------------------------*/
int ptp_cmd( int argc, char *argv[], char *cmdline )
{
    DEVBLK*  dev;
    PTPBLK*  pPTPBLK;
    U16      lcss;
    U16      devnum;
    u_int    all;
    u_int    onoff;
    u_int    mask;
    int      i;
    u_int    j;
    DEVGRP*  pDEVGRP;
    DEVBLK*  pDEVBLK;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    // Format:  "ptp  debug  on  [ [ <devnum>|ALL ] [ mask ] ]"
    // Format:  "ptp  debug  off [ [ <devnum>|ALL ] ]"

    if ( argc >= 2 && CMD(argv[1],debug,5) )
    {

        if ( argc >= 3 )
        {
            if ( CMD(argv[2],on,2) )
            {
                onoff = TRUE;
                mask = DBGPTPPACKET;
            }
            else if ( CMD(argv[2],off,3) )
            {
                onoff = FALSE;
                mask = 0;
            }
            else
            {
                // HHC02299 "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }
        else
        {
            // HHC02299 "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }

        all = TRUE;
        if ( argc >= 4 )
        {
            if ( CMD(argv[3],all,3) )
            {
                all = TRUE;
            }
            else if ( parse_single_devnum( argv[3], &lcss, &devnum) == 0 )
            {
                all = FALSE;
            }
            else
            {
                // HHC02299 "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }

        if ( argc >= 5 )
        {
            if ( onoff == TRUE)
            {
                mask = 0;
                j = 0;
                for ( i = 4 ; i < argc ; i++ )
                {
                    if ( CMD(argv[i],packet,6) )
                    {
                        j = DBGPTPPACKET;
                    }
                    else if ( CMD(argv[i],data,4) )
                    {
                        j = DBGPTPDATA;
                    }
                    else if ( CMD(argv[i],expand,6) )
                    {
                        j = DBGPTPEXPAND;
                    }
                    else if ( CMD(argv[i],updown,6) )
                    {
                        j = DBGPTPUPDOWN;
                    }
                    else if ( CMD(argv[i],ccw,3) )
                    {
                        j = DBGPTPCCW;
                    }
                    else
                    {
                        j = atoi( argv[i] );
                        if ( j < 1 || j > 255 )
                        {
                            // HHC02299 "Invalid command usage. Type 'help %s' for assistance."
                            WRMSG( HHC02299, "E", argv[0] );
                            return -1;
                        }
                    }
                    mask |= j;
                }
            }
            else
            {
                // HHC02299 "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }

        if ( all )
        {
            for ( dev = sysblk.firstdev; dev; dev = dev->nextdev )
            {
                if ( dev->allocated &&
                     dev->devtype == 0x3088 &&
                     dev->ctctype == CTC_PTP )
                {
                    pPTPBLK = dev->group->grp_data;
                    pPTPBLK->uDebugMask = mask;
                }
            }

            // HHC02204 "%-14s set to %s"
            WRMSG(HHC02204, "I", "PTP DEBUG", onoff ? "on ALL" : "off ALL");
        }
        else
        {
            if ( !(dev = find_device_by_devnum( lcss, devnum )) )
            {
                // HHC02200 "%1d:%04X device not found"
                devnotfound_msg( lcss, devnum );
                return -1;
            }

            if ( !dev->allocated ||
                 dev->devtype != 0x3088 ||
                 dev->ctctype != CTC_PTP )
            {
                // HHC02209 "%1d:%04X device is not a '%s'"
                WRMSG(HHC02209, "E", lcss, devnum, "PTP" );
                return -1;
            }

            pDEVGRP = dev->group;

            for ( i=0; i < pDEVGRP->acount; i++ )
            {
                pDEVBLK = pDEVGRP->memdev[i];
                pPTPBLK = pDEVBLK->group->grp_data;
                pPTPBLK->uDebugMask = mask;
            }

            {
            char buf[128];
            MSGBUF( buf, "%s for %s device %1d:%04X pair",
                    onoff ? "ON" : "OFF",
                    "PTP",
                    lcss, devnum );
            // HHC02204 "%-14s set to %s"
            WRMSG(HHC02204, "I", "PTP DEBUG", buf);
            }
        }

        return 0;
    }

    // HHC02299 "Invalid command usage. Type 'help %s' for assistance."
    WRMSG( HHC02299, "E", argv[0] );
    return -1;

}

/*-------------------------------------------------------------------*/
/* qeth command - enable/disable QETH debugging                      */
/*-------------------------------------------------------------------*/
int qeth_cmd( int argc, char *argv[], char *cmdline )
{
    DEVBLK*  dev;
    OSA_GRP* grp;
    U16      lcss;
    U16      devnum;
    u_int    all;
    u_int    onoff;
    u_int    mask;
    int      i;
    u_int    j;
    DEVGRP*  pDEVGRP;
    char     charaddr[48];
    int      numaddr;
    BYTE     found = FALSE;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    // Format:  "QETH  DEBUG  {ON|OFF}  [ [<devnum>|ALL] [mask ...] ]"
    // Format:  "QETH  ADDR             [ [<devnum>|ALL]            ]"

    if ( argc >= 2 && CMD(argv[1],debug,5) )
    {

        if ( argc >= 3 )
        {
            if ( CMD(argv[2],on,2) )
            {
                onoff = TRUE;
                mask = DBGQETHPACKET;
            }
            else if ( CMD(argv[2],off,3) )
            {
                onoff = FALSE;
                mask = 0;
            }
            else
            {
                // "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }
        else
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }

        all = TRUE;
        if ( argc >= 4 )
        {
            if ( CMD(argv[3],all,3) )
            {
                all = TRUE;
            }
            else if ( parse_single_devnum( argv[3], &lcss, &devnum) == 0 )
            {
                all = FALSE;
            }
            else
            {
                // "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }

        if ( argc >= 5 )
        {
            if ( onoff == TRUE)
            {
                // mask:

                // PLEASE KEEP THESE IN ALPHABETICAL ORDER!
                // PLEASE NOTE THE MINIMUM ABBREVIATIONS!

                // 'Ccw', 'DAta', 'DRopped', 'Expand', 'Interupts',
                // 'Packet', 'Queues', 'SBale', 'SIga', 'Updown',

                // 0xhhhhhhhh hexadecimal value

                mask = 0;
                for ( i = 4 ; i < argc ; i++ )
                {

                    if (0) ;
                    else if ( CMD(argv[i],ccw,1) )
                    {
                        j = DBGQETHCCW;
                    }
                    else if ( CMD(argv[i],data,2) )
                    {
                        j = DBGQETHDATA;
                    }
                    else if ( CMD(argv[i],dropped,2) )
                    {
                        j = DBGQETHDROP;
                    }
                    else if ( CMD(argv[i],expand,1) )
                    {
                        j = DBGQETHEXPAND;
                    }
                    else if ( CMD(argv[i],interupts,1) )
                    {
                        j = DBGQETHINTRUPT;
                    }
                    else if ( CMD(argv[i],packet,1) )
                    {
                        j = DBGQETHPACKET;
                    }
                    else if ( CMD(argv[i],queues,1) )
                    {
                        j = DBGQETHQUEUES;
                    }
                    else if ( CMD(argv[i],sbale,2) )
                    {
                        j = DBGQETHSBALE;
                    }
                    else if ( CMD(argv[i],siga,2) )
                    {
                        j = DBGQETHSIGA;
                    }
                    else if ( CMD(argv[i],updown,1) )
                    {
                        j = DBGQETHUPDOWN;
                    }
                    else
                    {
                        char* endptr;
                        j = strtoul( argv[i], &endptr, 16 );
                        if (0
                            || (1
                                && (0 == j || UINT_MAX == j)
                                && ERANGE == errno
                               )
                            || (1
                                && *endptr != ' '
                                && *endptr != 0
                               )
                        )
                        {
                            // "Invalid command usage. Type 'help %s' for assistance."
                            WRMSG( HHC02299, "E", argv[0] );
                            return -1;
                        }
                    }
                    mask |= j;
                }
            }
            else
            {
                // "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }

        if ( all )
        {
            found = FALSE;

            for ( dev = sysblk.firstdev; dev; dev = dev->nextdev )
            {
                if ( dev->allocated &&
                     dev->devtype == 0x1731 )
                {
                    grp = dev->group->grp_data;
                    grp->debugmask = mask;
                    found = TRUE;
                }
            }

            if (!found)
            {
                // "No %s devices found"
                WRMSG( HHC02347, "E", "QETH" );
                return -1;
            }

            // "%-14s set to %s"
            WRMSG(HHC02204, "I", "QETH DEBUG", onoff ? "ON ALL" : "OFF ALL");
        }
        else
        {
            if ( !(dev = find_device_by_devnum( lcss, devnum )) )
            {
                // "%1d:%04X device not found"
                devnotfound_msg( lcss, devnum );
                return -1;
            }

            if ( !dev->allocated ||
                 dev->devtype != 0x1731 )
            {
                // "%1d:%04X device is not a '%s'"
                WRMSG(HHC02209, "E", lcss, devnum, "QETH" );
                return -1;
            }

            pDEVGRP = dev->group;

            for ( i=0; i < pDEVGRP->acount; i++ )
            {
                grp = dev->group->grp_data;
                grp->debugmask = mask;
            }

            {
            char buf[128];
            MSGBUF( buf, "%s for %s device %1d:%04X group",
                    onoff ? "ON" : "OFF",
                    "QETH",
                    lcss, devnum );
            // "%-14s set to %s"
            WRMSG(HHC02204, "I", "QETH DEBUG", buf);
            }
        }

        return 0;
    }

    if ( CMD(argv[1],addr,4) )
    {

        if ( argc < 3 )
        {
            all = TRUE;
            pDEVGRP = NULL;
        }
        else
        {
            if ( CMD(argv[2],all,3) )
            {
                all = TRUE;
                pDEVGRP = NULL;
            }
            else if ( parse_single_devnum( argv[2], &lcss, &devnum) == 0 )
            {
                if ( !(dev = find_device_by_devnum( lcss, devnum )) )
                {
                    // "%1d:%04X device not found"
                    devnotfound_msg( lcss, devnum );
                    return -1;
                }
                if ( !dev->allocated ||
                     dev->devtype != 0x1731 )
                {
                    // "%1d:%04X device is not a '%s'"
                    WRMSG(HHC02209, "E", lcss, devnum, "QETH" );
                    return -1;
                }
                all = FALSE;
                pDEVGRP = dev->group;
            }
            else
            {
                // "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                return -1;
            }
        }

        if (argc > 3)
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }

        grp = NULL;
        found = FALSE;

        for ( dev = sysblk.firstdev; dev; dev = dev->nextdev )
        {
          /* Check the device is a QETH device */
          if ( dev->allocated &&
               dev->devtype == 0x1731 )
          {
            /* Check whether we are displaying all QETH groups or just a specific QETH group */
            if (all == TRUE || pDEVGRP == dev->group)
            {
              /* Check whether we have already displayed this QETH group */
              if (grp != dev->group->grp_data)
              {
                /* Check whether this QETH group is complete */
                grp = dev->group->grp_data;
                if (dev->group->members == dev->group->acount)
                {
                  /* The first device of a complete QETH group, display the addresses */
                  found = TRUE;
                  numaddr = 0;

                  /* Display registered MAC addresses. */
                  for (i = 0; i < OSA_MAXMAC; i++)
                  {
                    if (grp->mac[i].type)
                    {
                      MSGBUF( charaddr,
                                "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
                                grp->mac[i].addr[0],
                                grp->mac[i].addr[1],
                                grp->mac[i].addr[2],
                                grp->mac[i].addr[3],
                                grp->mac[i].addr[4],
                                grp->mac[i].addr[5] );
                      // "%s device %1d:%04X group has registered MAC address %s"
                      WRMSG(HHC02344, "I", dev->typname, LCSS_DEVNUM,
                                charaddr );
                      numaddr++;
                    }
                  }

                  /* Display registered IPv4 address. */
                  if (grp->ipaddr4[0].type == IPV4_TYPE_INUSE)
                  {
                    hinet_ntop( AF_INET, grp->ipaddr4[0].addr, charaddr, sizeof(charaddr) );
                    // "%s device %1d:%04X group has registered IP address %s"
                    WRMSG(HHC02345, "I", dev->typname, LCSS_DEVNUM,
                            charaddr );
                    numaddr++;
                  }

                  /* Display registered IPv6 addresses. */
                  for (i = 0; i < OSA_MAXIPV6; i++)
                  {
                    if (grp->ipaddr6[i].type == IPV6_TYPE_INUSE)
                    {
                      hinet_ntop( AF_INET6, grp->ipaddr6[i].addr, charaddr, sizeof(charaddr) );
                      // "%s device %1d:%04X group has registered IP address %s"
                      WRMSG(HHC02345, "I", dev->typname, LCSS_DEVNUM,
                              charaddr );
                      numaddr++;
                    }
                  }

                  /* Display whether there were any registered addresses. */
                  if (numaddr == 0)
                  {
                  // "%s device %1d:%04X group has no registered MAC or IP addresses"
                  WRMSG(HHC02346, "I", dev->typname, LCSS_DEVNUM);
                  }
                }
              }
            }
          }
        } /* end for (dev = ... */

        if (!found)
        {
            // "No %s devices found"
            WRMSG( HHC02347, "E", "QETH" );
            return -1;
        }

        return 0;
    }

    // "Invalid command usage. Type 'help %s' for assistance."
    WRMSG( HHC02299, "E", argv[0] );
    return -1;

}  /* End of qeth_cmd */

#if defined( OPTION_W32_CTCI )
/*-------------------------------------------------------------------*/
/* tt32 command - control/query CTCI-WIN functionality               */
/*-------------------------------------------------------------------*/
int tt32_cmd( int argc, char *argv[], char *cmdline )
{
    char buf[64];

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );
    MSGBUF( buf, ". Type 'help %s' for assistance", argv[0] );

    if (argc < 2)
    {
        // "Missing argument(s). Type 'help %s' for assistance."
        WRMSG( HHC02202, "E", argv[0] );
        return -1;
    }

    if (CMD( argv[1], STATS, 5 ))
    {
        DEVBLK*  dev;
        char*    devclass;
        int      rc;
        U16      devnum;
        U16      lcss;

        if (argc < 3)
        {
            // HHC02201 "Device number missing"
            missing_devnum();
            return -1;
        }

        if (argc > 3)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[3], buf );
            return -1;
        }

        if ((rc = parse_single_devnum( argv[2], &lcss, &devnum )) < 0)
            return -1;

        if (!(dev = find_device_by_devnum( lcss, devnum )) &&
            !(dev = find_device_by_devnum( lcss, devnum ^ 0x01 )))
        {
            // HHC02200 "%1d:%04X device not found"
            devnotfound_msg( lcss, devnum );
            return -1;
        }

        // Device must be a non-8232 "CTCA" LCS/CTCI device or an "OSA" device

        (dev->hnd->query)( dev, &devclass, 0, NULL );

        if (1
            && strcasecmp( devclass, "OSA" ) != 0
            && (0
                || strcasecmp( devclass, "CTCA" ) != 0
                || strcmp( dev->typname, "8232" ) == 0
                || (1
                    && CTC_CTCI != dev->ctctype
                    && CTC_LCS  != dev->ctctype
                   )
               )
        )
        {
            // "%1d:%04X device is not a '%s'"
            WRMSG( HHC02209, "E", lcss, devnum, "supported CTCI, LCS or OSA device" );
            return -1;
        }

        if (debug_tt32_stats)
        {
            debug_tt32_stats( dev->fd );
            return 0;
        }

        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "debug_tt32_stats()", "function itself is NULL" );
        return -1;
    }

    if (CMD( argv[1], DEBUG, 5 ))
    {
        if (argc > 2)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[2], buf );
            return -1;
        }

        if (debug_tt32_tracing)
        {
            bool enabled = debug_tt32_tracing(1); // 1=ON
            WRMSG( HHC02204, "I", "TT32 DEBUG", enabled ? "enabled" : "disabled" );
            return enabled ? 0 : -1;
        }

        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "debug_tt32_tracing()", "function itself is NULL" );
        return -1;
    }

    if (CMD( argv[1], NODEBUG, 7 ))
    {
        if (argc > 2)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[2], buf );
            return -1;
        }

        if (debug_tt32_tracing)
        {
            bool enabled = debug_tt32_tracing(0); // 0=OFF
            WRMSG( HHC02204, "I", "TT32 DEBUG", enabled ? "enabled" : "disabled" );
            return !enabled ? 0 : -1;
        }

        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "debug_tt32_tracing()", "function itself is NULL" );
        return -1;
    }

    // "Invalid argument %s%s"
    WRMSG( HHC02205, "E", argv[1], buf );
    return -1;
}
#endif /* defined( OPTION_W32_CTCI ) */

/*-------------------------------------------------------------------*/
/* sclproot command - set SCLP base directory                        */
/*-------------------------------------------------------------------*/
int sclproot_cmd( int argc, char* argv[], char* cmdline )
{
    char* basedir;

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    if (argc > 1)
    {
        char* p = "NONE";

        if (CMD( argv[1], NONE, 4 ))
            set_sce_dir( NULL );
        else

            set_sce_dir( p = argv[1] );

        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], p );
    }
    else
    {
        if ((basedir = get_sce_dir()))
        {
            char buf[ MAX_PATH + 64 ];
            char* p = strchr( basedir, ' ' );

            if (!p)
                p = basedir;
            else
            {
                MSGBUF( buf, "\"%s\"", basedir );
                p = buf;
            }

            // "%-14s: %s"
            WRMSG( HHC02203, "I", argv[0], p );
        }
        else
        {
            // "%-14s: %s"
            WRMSG( HHC02203, "I", "SCLP disk I/O", "disabled" );
        }
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* engines command                                                   */
/*-------------------------------------------------------------------*/
int engines_cmd( int argc, char* argv[], char* cmdline )
{
    int cpu, count;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc < 1 || argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    if (argc == 2)
    {
        BYTE   c;
        BYTE   type;                    /* Processor engine type     */
        BYTE   ptyp[ MAX_CPU_ENGS ];    /* SCCB ptyp for each engine */
        char*  styp;                    /* -> Engine type string     */
        char*  strtok_str = NULL;       /* strtok_r work variable    */
        char*  arg1 = strdup( argv[1] );/* (save before modifying)   */

        /* Default all engines to type "CP" */
        memset( ptyp, short2ptyp( "CP" ), sizeof( ptyp ));

        /* Parse processor engine types operand, and save the results
           for eventual sysblk update if no errors are detected.
           Example: "ENGINES  4*CP,AP,2*IP"
        */
        styp = strtok_r( argv[1], ",", &strtok_str );

        for (cpu=0; styp;)
        {
            count = 1;

            if (isdigit( (unsigned char)styp[0] ))
            {
                if (0
                    || sscanf( styp, "%d%c", &count, &c ) != 2
                    || c != '*'
                    || count < 1
                )
                {
                    // "Invalid syntax %s for %s"
                    WRMSG( HHC01456, "E", styp, argv[0] );
                    free( arg1 );
                    return -1;
                }

                styp = strchr( styp, '*' ) + 1;
            }

                 if (CMD( styp, CP, 2)) type = short2ptyp( "CP" );
            else if (CMD( styp, CF, 2)) type = short2ptyp( "CF" );
            else if (CMD( styp, IL, 2)) type = short2ptyp( "IL" );
            else if (CMD( styp, AP, 2)) type = short2ptyp( "AP" );
            else if (CMD( styp, IP, 2)) type = short2ptyp( "IP" );
            else
            {
                // "Invalid value %s specified for %s"
                WRMSG( HHC01451, "E", styp, argv[0] );
                free( arg1 );
                return -1;
            }

            /* (update ptyp work array) */
            while (count-- > 0 && cpu < sysblk.maxcpu)
                ptyp[ cpu++ ] = type;

            styp = strtok_r( NULL, ",", &strtok_str );
        }

        /* If we make it this far, then update sysblk */
        for (cpu=0; cpu < sysblk.maxcpu; ++cpu)
        {
            sysblk.ptyp[ cpu ] = ptyp[ cpu ];

            // "Processor %s%02X: engine %02X type %1d set: %s"
            WRMSG( HHC00827, "I"
                , PTYPSTR( cpu ), cpu
                , cpu
                , ptyp[ cpu ]
                , ptyp2short( ptyp[ cpu ])
            );
        }

        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], arg1 );
        free( arg1 );
    }
    else // (display current setting)
    {
        char typ[ 7 + 1 ];  // "nnn*XX,\0"
        char arg1[ (MAX_CPU_ENGS * 7) + 1 ] = {0};

        count = 0;

        for (cpu=0; cpu < sysblk.maxcpu; ++cpu)
        {
            if (cpu == 0)
                count = 1;
            else
            {
                if (sysblk.ptyp[ cpu ] == sysblk.ptyp[ cpu-1 ])
                    ++count;
                else
                {
                    if (count == 1)
                        MSGBUF( typ, "%s,", PTYPSTR( cpu-1 ));
                    else
                        MSGBUF( typ, "%1d*%s,", count, PTYPSTR( cpu-1 ));

                    STRLCAT( arg1, typ );
                    count = 1;
                }
            }
        }

        if (count <= 1)
        {
            if (count)
                MSGBUF( typ, "%s,", PTYPSTR( cpu-1 ));
            else
                STRLCPY( typ, "(none)" );
        }
        else
            MSGBUF( typ, "%1d*%s,", count, PTYPSTR( cpu-1 ));

        STRLCAT( arg1, typ );

        RTRIMS( arg1, "," );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], arg1 );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* sysepoch command  1900|1960 [+|-142]                              */
/*-------------------------------------------------------------------*/
int sysepoch_cmd(int argc, char *argv[], char *cmdline)
{
int     rc          = 0;
char   *ssysepoch   = NULL;
char   *syroffset   = NULL;
int     sysepoch    = 1900;
S32     yroffset    = 0;
BYTE    c;

    UNREFERENCED(cmdline);

    if ( argc < 2 || argc > 3 )
    {
        WRMSG( HHC01455, "E", argv[0] );
        rc = -1;
    }
    else for ( ;; )
    {
        ssysepoch = argv[1];
        if ( argc == 3 )
            syroffset = argv[2];
        else
            syroffset = NULL;

        /* Parse system epoch operand */
        if (ssysepoch != NULL)
        {
            if (strlen(ssysepoch) != 4
                || sscanf(ssysepoch, "%d%c", &sysepoch, &c) != 1
                || ( sysepoch != 1900 && sysepoch != 1960 ) )
            {
                if ( sysepoch == 1900 || sysepoch == 1960 )
                    WRMSG( HHC01451, "E", ssysepoch, argv[0] );
                else
                    WRMSG( HHC01457, "E", argv[0], "1900|1960" );
                rc = -1;
                break;
            }
        }

        /* Parse year offset operand */
        if (syroffset != NULL)
        {
            if (sscanf(syroffset, "%d%c", &yroffset, &c) != 1
                || (yroffset < -142) || (yroffset > 142))
            {
                WRMSG( HHC01451, "E", syroffset, argv[0] );
                rc = -1;
                break;
            }
        }

        configure_epoch(sysepoch);
        configure_yroffset(yroffset);

        break;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* yroffset command                                                  */
/*-------------------------------------------------------------------*/
int yroffset_cmd(int argc, char *argv[], char *cmdline)
{
int     rc  = 0;
S32     yroffset;
BYTE    c;

    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    /* Parse year offset operand */
    if ( argc == 2 )
    {
        for ( ;; )
        {
            if (sscanf(argv[1], "%d%c", &yroffset, &c) != 1
                || (yroffset < -142) || (yroffset > 142))
            {
                WRMSG( HHC01451, "E", argv[1], argv[0] );
                rc = -1;
                break;
            }
            else
            {
                configure_yroffset(yroffset);
                if ( MLVL(VERBOSE) )
                    WRMSG( HHC02204, "I", argv[0], argv[1] );
            }
            break;
        }
    }
    else
    {
        WRMSG( HHC01455, "E", argv[0] );
        rc = -1;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* tzoffset command                                                  */
/*-------------------------------------------------------------------*/
int tzoffset_cmd(int argc, char *argv[], char *cmdline)
{
int rc = 0;
S32 tzoffset;
BYTE c;

    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    /* Parse timezone offset operand */
    if ( argc == 2 )
    {
        for ( ;; )
        {
            if (strlen(argv[1]) != 5
                || sscanf(argv[1], "%d%c", &tzoffset, &c) != 1
                || (tzoffset < -2359) || (tzoffset > 2359))
            {
                WRMSG( HHC01451, "E", argv[1], argv[0] );
                rc = -1;
                break;
            }
            else
            {
                configure_tzoffset(tzoffset);
                if ( MLVL( VERBOSE ) )
                    WRMSG( HHC02204, "I", argv[0], argv[1] );
            }
            break;
        }
    }
    else
    {
        WRMSG( HHC01455, "E", argv[0] );
        rc = -1;
    }

    return rc;
}

#if defined( HAVE_MLOCKALL )
/*-------------------------------------------------------------------*/
/* memlock - lock all hercules memory                                */
/*-------------------------------------------------------------------*/
int memlock_cmd(int argc, char *argv[], char *cmdline)
{
int rc = 0;

    UNREFERENCED(cmdline);

    if ( argc > 1 )
    {
        if ( CMD(argv[1],on,2)  )
            rc = configure_memlock(TRUE);
        else if ( CMD(argv[1],off,3) )
            rc = configure_memlock(FALSE);
        else
        {
            WRMSG( HHC02205, "E", argv[1], "" );
            return 0;
        }

        if(rc)
            logmsg("%s error: %s\n",argv[0],strerror(rc));
    }

    return rc;
}

int memfree_cmd(int argc, char *argv[], char *cmdline)
{
    int  mfree;
    char c;

    UNREFERENCED(cmdline);

    if ( argc > 1 && sscanf(argv[1], "%d%c", &mfree, &c) == 1 && mfree >= 0)
        configure_memfree(mfree);
    else
        logmsg("memfree %d\n",configure_memfree(-1));

    return 0;
}
#endif /* defined( HAVE_MLOCKALL ) */

int qstor_cmd(int argc, char *argv[], char *cmdline);
int mainsize_cmd(int argc, char *argv[], char *cmdline);
int xpndsize_cmd(int argc, char *argv[], char *cmdline);

/*-------------------------------------------------------------------*/
/* mainsize command                                                  */
/*-------------------------------------------------------------------*/
int mainsize_cmd( int argc, char* argv[], char* cmdline )
{
    //  "MAINSIZE nnn[x]" --> 'nnn' = number, 'x' optional size suffix
    //
    //  If size suffix not given then 'nnn' is number of megabytes.
    //
    //  The value we calculate and pass to the configure_storage()
    //  function however, is always specified in number of 4K pages.

    char*  qstor_cmdline  =    "qstor    main"  ;
    char*  qstor_args[2]  =  { "qstor", "main" };

    U64    mainsize;                // (the 'nnn' value they gave us)
    U64    mainsize_numpages;       // (calculated from nnn + suffix)

    BYTE   f = ' ';                 // (optional size suffix)
    BYTE   c = '\0';                // (work for sscanf call)

    char   lockopt[16];             // (LOCKED/UNLOCKED work)
    bool   lock_mainstor = false;   // (true == "LOCKED" given)
    int    i, rc;                   // (work)

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    if (argc < 2)
        return qstor_cmd( 2, qstor_args, qstor_cmdline );

    /* Parse main storage size operand */
    rc = sscanf( argv[1], "%"SCNu64"%c%c", &mainsize, &f, &c );

    if (rc < 1 || rc > 2)
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    /* Handle size suffix and suffix overflow */
    {
        U64  suffix_oflow_mask  = 0xFFFFFFFFFFFFFFFFULL;

        mainsize_numpages = mainsize;  /* (converted to pages below) */

        if (rc == 2)
        {
            switch (toupper( (unsigned char)f ))
            {
            case 'B':
                suffix_oflow_mask = 0;
                mainsize_numpages >>= 12;
                if (mainsize & 0x0FFF)
                    ++mainsize_numpages;
                break;

            case 'K':
                suffix_oflow_mask <<= 55;
                mainsize_numpages >>= 2;
                if (mainsize & 0x03)
                    ++mainsize_numpages;
                break;

            case 'M':
                suffix_oflow_mask <<= 45;
                mainsize_numpages <<= SHIFT_MEBIBYTE - SHIFT_4K;
                break;

            case 'G':
                suffix_oflow_mask <<= 35;
                mainsize_numpages <<= SHIFT_GIBIBYTE - SHIFT_4K;
                break;

            case 'T':
                suffix_oflow_mask <<= 25;
                mainsize_numpages <<= SHIFT_TEBIBYTE - SHIFT_4K;
                break;

            case 'P':
                suffix_oflow_mask <<= 15;
                mainsize_numpages <<= SHIFT_PEBIBYTE - SHIFT_4K;
                break;

            case 'E':
                suffix_oflow_mask <<=  5;
                mainsize_numpages <<= SHIFT_EXBIBYTE - SHIFT_4K;
                break;
            /*----------------------------------------------------------
            default:        // Force error
                break;      // ...
            ----------------------------------------------------------*/
            }
        }
        else /* default is megabytes if no suffix  */
        {
            suffix_oflow_mask <<= 45;
            mainsize_numpages <<= SHIFT_MEBIBYTE - SHIFT_4K;
        }

        /* Error if more than 16E-1 or suffix use causes U64 overflow */
        if (0
            || mainsize_numpages >= 0x0010000000000000ULL // (over 16E-1)
            || (mainsize & suffix_oflow_mask)             // (too much)
        )
        {
            // "Invalid value %s specified for %s"
            WRMSG( HHC01451, "E", argv[1], argv[0]);
            return (-1);
        }

        mainsize = (mainsize_numpages << SHIFT_4K);

        // "mainsize_numpages" now holds mainsize value in #of 4K pages
        // and "mainsize" holds the equivalent value in number of bytes.
    }

    /* Validate requested storage size */
    if (adjust_mainsize( sysblk.arch_mode, mainsize ) != mainsize)
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    /* Process options */
    for (i=2; i < argc; ++i)
    {
        strnupper( lockopt, argv[i], (u_int) sizeof( lockopt ));

#if 0   // TEMPORARILY DISABLED; config.c doesn't support locking storage yet)
        if (strabbrev( "LOCKED", lockopt, 1 ) && mainsize_numpages)
            lock_mainstor = true;
        else
#endif
        if (strabbrev( "UNLOCKED", lockopt, 3 ))
            lock_mainstor = false;
        else
        {
            // "Invalid value %s specified for %s"
            WRMSG( HHC01451, "E", argv[i], argv[0] );
            return -1;
        }
    }

    /* Set lock request ("UNLOCKED" forced when mainsize = 0) */
    if (!mainsize_numpages) lock_mainstor = false;
    sysblk.lock_mainstor =  lock_mainstor;

    /* Update main storage size */
    rc = configure_storage( mainsize_numpages );

    if (rc >= 0)
    {
        if (MLVL( VERBOSE ))
            // Show them the results
            qstor_cmd( 2, qstor_args, qstor_cmdline );
    }
    else if (HERRCPUONL == rc)
    {
        // "CPUs must be offline or stopped"
        WRMSG( HHC02389, "E" );
    }
    else
    {
        // "Configure storage error %d"
        WRMSG( HHC02388, "E", rc );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* xpndsize command                                                  */
/*-------------------------------------------------------------------*/
int xpndsize_cmd(int argc, char *argv[], char *cmdline)
{
U64     xpndsize;
BYTE    f = ' ', c = '\0';
int     rc;
u_int   i;
char    check[16];
char   *q_argv[2] = { "qstor", "xpnd" };
u_int   lockreq = 0;
u_int   locktype = 0;

    UNREFERENCED(cmdline);

    if ( argc == 1 )
    {
        return qstor_cmd( 2, q_argv, "qstor xpnd" );
    }

    /* Parse expanded storage size operand */
    rc = sscanf(argv[1], "%"SCNu64"%c%c", &xpndsize, &f, &c);

    if (rc > 2 )
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    /* Handle size suffix and suffix overflow */
    {
        U64 shiftsize = xpndsize;
        size_t sizeof_RADR = (ARCH_900_IDX == sysblk.arch_mode) ? sizeof(U64)
                                                            : sizeof(U32);
        if ( rc == 2 )
        {
            switch (toupper((unsigned char)f))
            {
            case 'B':
                shiftsize >>= SHIFT_MEBIBYTE;
                if (xpndsize & 0x00000000000FFFFFULL)
                    ++shiftsize;
                break;
            case 'K':
                shiftsize >>= SHIFT_MEBIBYTE - SHIFT_KIBIBYTE;
                if (xpndsize & 0x00000000000003FFULL)
                    ++shiftsize;
                break;
            case 'M':
                break;
            case 'G':
                shiftsize <<= SHIFT_GIBIBYTE - SHIFT_MEBIBYTE;
                break;
            case 'T':
                shiftsize <<= SHIFT_TEBIBYTE - SHIFT_MEBIBYTE;
                break;
            case 'P':
                shiftsize <<= SHIFT_PEBIBYTE - SHIFT_MEBIBYTE;
                break;
            case 'E':
                shiftsize <<= SHIFT_EXBIBYTE - SHIFT_MEBIBYTE;
                break;
            default:
                /* Force error */
                shiftsize = 0;
                xpndsize = 1;
                break;
            }
        }

        if (0
            || (xpndsize && !shiftsize)               /* Overflow occurred */
            || (1
                && sizeof_RADR <= sizeof(U32)         /* 32-bit addressing */
                && shiftsize > 0x0000000000001000ULL  /* 4G maximum        */
                )
            || (1
                && sizeof_RADR >= sizeof(U64)         /* 32-bit addressing */
                && shiftsize >= 0x0000100000000000ULL /* 16E-1 maximum     */
                )
        )
        {
            // "Invalid value %s specified for %s"
            WRMSG( HHC01451, "E", argv[1], argv[0]);
            return (-1);
        }

        xpndsize = shiftsize;
    }

    /* Process options */
    for (i = 2; (int)i < argc; ++i)
    {
        strnupper(check, argv[i], (u_int)sizeof(check));

#if 0   // Interim - Storage is not locked yet in config.c
        if (strabbrev("LOCKED", check, 1) && xpndsize)
        {
            lockreq = 1;
            locktype = 1;
        }
        else
#endif
        if (strabbrev("UNLOCKED", check, 3))
        {
            lockreq = 1;
            locktype = 0;
        }
        else
        {
            // "Invalid value %s specified for %s"
            WRMSG( HHC01451, "E", argv[i], argv[0] );
            return -1;
        }
    }
    if (!xpndsize)
        sysblk.lock_xpndstor = 0;
    else if (lockreq)
        sysblk.lock_xpndstor = locktype;

    rc = configure_xstorage( xpndsize );
    if (rc >= 0)
    {
        if (MLVL( VERBOSE ))
            qstor_cmd( 2, q_argv, "qstor xpnd" );
    }
    else
    {
        if (HERRCPUONL == rc)
        {
            // "CPUs must be offline or stopped"
            WRMSG( HHC02389, "E" );
        }
        else
        {
            // "Configure expanded storage error %d"
            WRMSG( HHC02387, "E", rc );
        }
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Deprecated 'xxxPRIO' and HERCNICE commands                        */
/*-------------------------------------------------------------------*/
static int deprecated_cmd( char* argv0 )
{
    // "Command '%s' is deprecated%s"
    WRMSG( HHC02256, "W", STR2UPPER( argv0 ), " and ignored." );
    return 0; // (return success to prevent error message)
}

#define DEPRECATED_PRIONICE_CMD( _cmd )                 \
                                                        \
    int _cmd( int argc, char* argv[], char* cmdline )   \
    {                                                   \
        UNREFERENCED( argc );                           \
        UNREFERENCED( cmdline );                        \
        return deprecated_cmd( argv[0] );               \
    }

DEPRECATED_PRIONICE_CMD( hercnice_cmd );
DEPRECATED_PRIONICE_CMD( hercprio_cmd );
DEPRECATED_PRIONICE_CMD( cpuprio_cmd  );
DEPRECATED_PRIONICE_CMD( devprio_cmd  );
DEPRECATED_PRIONICE_CMD( todprio_cmd  );
DEPRECATED_PRIONICE_CMD( srvprio_cmd  );

#if 0 /* INCOMPLETE */
/*-------------------------------------------------------------------*/
/* numvec command                                                    */
/*-------------------------------------------------------------------*/
int numvec_cmd(int argc, char *argv[], char *cmdline)
{
U16 numvec;
BYTE c;

    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    /* Parse maximum number of Vector processors operand */
    if ( argc == 2 )
    {
        if (sscanf(argv[1], "%hu%c", &numvec, &c) != 1
            || numvec > sysblk.maxcpu)
        {
            WRMSG( HHC01451, "E", argv[1], argv[0] );
            return -1;
        }
        else
        {
            sysblk.numvec = numvec;
            if ( MLVL(VERBOSE) )
                WRMSG( HHC02204, "I", argv[0], argv[1] );
        }
    }
    else if ( argc == 1)
    {
        char msgbuf[16];
        MSGBUF(msgbuf, "%d", sysblk.numvec );
        WRMSG( HHC02203, "I", argv[0], msgbuf );
        if ( sysblk.numvec == 0 )
            return 1;
    }
    else
    {
        WRMSG( HHC01455, "E", argv[0] );
        return  -1;
    }

    return 0;
}
#endif

/*-------------------------------------------------------------------*/
/* netdev command                                                    */
/*-------------------------------------------------------------------*/
int netdev_cmd( int argc, char* argv[], char* cmdline )
{
    // Specifies the name (or for Windows, the IP or MAC address)
    // of the underlying default host network device to be used for
    // all Hercules communications devices unless overridden on
    // the device statement itself.
    //
    // The default for Linux is '/dev/net/tun'. The default for Apple
    // and FreeBSD is '/dev/tun'.
    //
    // The default for Windows is whatever SoftDevLabs's CTCI-WIN
    // product returns as its default CTCI-WIN host network adapter,
    // which for older versions of CTCI-WIN (3.5.0) is the first
    // network adapter returned by Windows in its adapter binding
    // order or for newer versions of CTCI-WIN (3.6.0) whatever is
    // defined as your default CTCI-WIN host network adapter.

    const char* netdev = DEF_NETDEV;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc < 2)
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], netdev[0] ?  netdev : "(empty)" );
        return 0;
    }

    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    netdev = TRIM( argv[1] );
    free( sysblk.netdev );
    sysblk.netdev = strdup( netdev );

    // "%-14s set to %s"
    WRMSG( HHC02204, "I", argv[0], netdev[0] ?  netdev : "(empty)" );
    return 0;
}

/*-------------------------------------------------------------------*/
/* numcpu command                                                    */
/*-------------------------------------------------------------------*/
int numcpu_cmd( int argc, char* argv[], char* cmdline )
{
    int   rc;
    U16   numcpu;
    char  msgbuf[32];
    BYTE  c;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Ensure only two arguments passed */
    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Display current value */
    if (argc == 1)
    {
        MSGBUF( msgbuf, "%d", sysblk.cpus );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], msgbuf );

        if (sysblk.cpus == 0)
            return 1;   // warning?
        else
            return 0;   // success
    }

    /* Parse maximum number of CPUs operand */
    if (sscanf( argv[1], "%hu%c", &numcpu, &c ) != 1)
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    if (numcpu > sysblk.maxcpu)
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], "; NUMCPU must be <= MAXCPU" );
        return -1;
    }

    /* Configure number of online CPUs */
    rc = configure_numcpu( numcpu );

    switch (rc)
    {
        case 0:
        {
            if (MLVL( VERBOSE ))
            {
                MSGBUF( msgbuf, "%d", sysblk.cpus );

                // "%-14s set to %s"
                WRMSG( HHC02204, "I", argv[0], msgbuf );
            }
        }
        break;

        case HERRCPUONL:
        {
            // "CPUs must be offline or stopped"
            WRMSG( HHC02389, "E" );
        }
        break;

        default:
        {
            // "Configure CPU error %d"
            WRMSG( HHC02386, "E", rc );
        }
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* maxcpu command                                                    */
/*-------------------------------------------------------------------*/
int maxcpu_cmd( int argc, char* argv[], char* cmdline )
{
    int   rc;
    U16   maxcpu;
    char  msgbuf[32];
    BYTE  c;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Ensure only two arguments passed */
    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Display current value */
    if (argc == 1)
    {
        MSGBUF( msgbuf, "%d", sysblk.maxcpu );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], msgbuf );

        if (sysblk.maxcpu == 0)
            return 1;   // warning?
        else
            return 0;   // success
    }

    /* Parse maximum number of CPUs operand */
    if (sscanf( argv[1], "%hu%c", &maxcpu, &c ) != 1
        || maxcpu > MAX_CPU_ENGS)
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    /* Configure maximum number of online CPUs */
    rc = configure_maxcpu( maxcpu );

    switch (rc)
    {
        case 0:
        {
            if (MLVL( VERBOSE ))
            {
                MSGBUF( msgbuf, "%d", sysblk.maxcpu );

                // "%-14s set to %s"
                WRMSG( HHC02204, "I", argv[0], msgbuf );
            }
        }
        break;

        case HERRCPUONL:
        {
            // "CPUs must be offline or stopped"
            WRMSG( HHC02389, "E" );
        }
        break;

        default:
        {
            // "Configure CPU error %d"
            WRMSG( HHC02386, "E", rc );
        }
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* cnslport command - set console port                               */
/*-------------------------------------------------------------------*/
int cnslport_cmd( int argc, char* argv[], char* cmdline )
{
    static char const* def_port = "3270";
    int rc = 0;
    int i;

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        rc = -1;
    }
    else if (argc == 1)
    {
        // Display current value

        char buf[128];

        if (strchr( sysblk.cnslport, ':' ) == NULL)
        {
            MSGBUF( buf, "on port %s", sysblk.cnslport);
        }
        else
        {
            char* serv;
            char* host = NULL;
            char* port = strdup( sysblk.cnslport );

            if ((serv = strchr( port, ':' )))
            {
                *serv++ = '\0';

                if (*port)
                    host = port;
            }

            MSGBUF( buf, "for host %s on port %s", host, serv);
            free( port );
        }

        // "%s server %slistening %s"
        WRMSG( HHC17001, "I", "Console", "", buf);
        rc = 0;
    }
    else
    {
        // Set new value

        char* port;
        char* host = strdup( argv[1] );

        if ((port = strchr( host, ':' )) == NULL)
            port = host;
        else
            *port++ = '\0';

        for (i=0; i < (int) strlen( port ); i++)
        {
            if (!isdigit( (unsigned char)port[i] ))
            {
                // "Invalid value %s specified for %s"
                WRMSG( HHC01451, "E", port, argv[0] );
                rc = -1;
                break;
            }
        }

        if (rc != -1)
        {
            i = atoi( port );

            if (i < 0 || i > 65535)
            {
                // "Invalid value %s specified for %s"
                WRMSG( HHC01451, "E", port, argv[0] );
                rc = -1;
            }
            else
                rc = 1;
        }

        free( host );
    }

    if (rc != 0)
    {
        const char* port = (rc == -1) ? def_port : argv[1];

        if (sysblk.sysgport && str_eq( port, sysblk.sysgport ))
        {
            // "%s cannot be the same as %s"
            WRMSG( HHC01453, "E", argv[0], "SYSGPORT" );
            rc = -1;
        }
        else
        {
            free( sysblk.cnslport );
            sysblk.cnslport = NULL;

            if (rc == -1)
            {
                // "Default port %s being used for %s"
                WRMSG( HHC01452, "W", def_port, argv[0] );
                sysblk.cnslport = strdup( def_port );
                rc = 1;
            }
            else
            {
                sysblk.cnslport = strdup( argv[1] );
                rc = 0;
                // "%-14s set to %s"
                WRMSG( HHC02204, "I", argv[0], sysblk.cnslport );
            }
        }
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* sysgport command - define SYSG console port                       */
/*-------------------------------------------------------------------*/
int sysgport_cmd( int argc, char* argv[], char* cmdline )
{
    static char const* def_port = "3278";
    bool disabled = false;
    int rc = 0;
    int i;

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        rc = -1;
    }
    else if (argc == 1) // Display current value
    {
        char buf[128];

        if (sysblk.sysgport && strchr( sysblk.sysgport, ':' ) == NULL)
        {
            MSGBUF( buf, "on port %s", sysblk.sysgport );
        }
        else // (!sysblk.sysgport || host:port specified)
        {
            if (sysblk.sysgport)
            {
                char* serv = NULL;
                char* host = NULL;
                char* port = NULL;

                port = strdup( sysblk.sysgport );

                if ((serv = strchr( port, ':' )))
                {
                    *serv++ = '\0';

                    if (*port)
                        host = port;
                }

                MSGBUF( buf, "for host %s on port %s", host, serv );
                free( port );
            }
        }

        // If SYSGPORT specified -AND- no SYSG device connected yet...
        if (sysblk.sysgport && (!sysblk.sysgdev || !sysblk.sysgdev->connected))
        {
            // "%s server %slistening %s"
            WRMSG( HHC17001, "I", "SYSG console", "", buf );
        }
        else // SYSGPORT NOT specified -OR- SYSG device already connected
        {
            // "%s server %slistening %s"
            WRMSG( HHC17001, "I", "SYSG console", "NOT ", "on any port" );
        }
        rc = 0;
    }
    else // Set new value
    {
        if (str_caseless_eq( argv[1], "NO" ))
        {
            disabled = true;
            rc = 1;
        }
        else
        {
            char* port;
            char* host = strdup( argv[1] );

            if ((port = strchr( host, ':' )) == NULL)
                port = host;
            else
                *port++ = '\0';

            for (i=0; i < (int) strlen( port ); i++)
            {
                if (!isdigit( (unsigned char)port[i] ))
                {
                    // "Invalid value %s specified for %s"
                    WRMSG( HHC01451, "E", port, argv[0] );
                    rc = -1;
                    break;
                }
            }

            if (rc != -1)  // (if no parsing error)
            {
                i = atoi( port );

                if (i < 0 || i > 65535)
                {
                    // "Invalid value %s specified for %s"
                    WRMSG( HHC01451, "E", port, argv[0] );
                    rc = -1;
                }
                else
                    rc = 1;
            }

            free( host );
        }
    }

    if (rc != 0) // (new value specified or error)
    {
        const char* port = (rc == -1) ? def_port : argv[1];

        if (!disabled && str_eq( port, sysblk.cnslport ))
        {
            // "%s cannot be the same as %s"
            WRMSG( HHC01453, "E", argv[0], "CNSLPORT" );
            rc = -1;
        }
        else // (disabled || port okay)
        {
            free( sysblk.sysgport );
            sysblk.sysgport = NULL;

            if (!disabled && rc == -1)
            {
                // "Default port %s being used for %s"
                WRMSG( HHC01452, "W", def_port, argv[0] );
                sysblk.sysgport = strdup( def_port );
                rc = 1;
            }
            else // (disabled || rc != -1)
            {
                if (!disabled)
                    sysblk.sysgport = strdup( argv[1] );

                // "%-14s set to %s"
                WRMSG( HHC02204, "I", argv[0],
                    disabled ? "NO" : sysblk.sysgport );
                rc = 0;
            }
        }
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* http command - manage HTTP server                                 */
/*-------------------------------------------------------------------*/
int http_cmd(int argc, char *argv[], char *cmdline)
{
    int     rc = 0;

    UNREFERENCED(cmdline);

    argc--;
    argv++;

    rc = http_command(argc, argv);

    return rc;
}

/*-------------------------------------------------------------------*/
/* toddrag command - display or set TOD clock drag factor            */
/*-------------------------------------------------------------------*/
int toddrag_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if ( argc == 2 )
    {
        double toddrag = -1.0;

        sscanf(argv[1], "%lf", &toddrag);

        if (toddrag >= 0.0001 && toddrag <= 10000.0)
        {
            /* Set clock steering based on drag factor */
            set_tod_steering(-(1.0-(1.0/toddrag)));
            if ( MLVL(VERBOSE) )
                WRMSG(HHC02204, "I", argv[0], argv[1] );
        }
        else
        {
            WRMSG( HHC01451, "E", argv[1], argv[0] );
            return -1;
        }
    }
    else
    {
        char buf[32];
        MSGBUF( buf, "%lf",(1.0/(1.0+get_tod_steering())));
        WRMSG(HHC02203, "I", argv[0], buf);
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* panopt command - display or set panel option(s)                   */
/*-------------------------------------------------------------------*/
int panopt_cmd( int argc, char* argv[], char* cmdline)
{
    char*  pantitle     = NULL;
    int    panrate      = sysblk.panrate;
    int    pan_colors   = sysblk.pan_colors;
    bool   devnameonly  = sysblk.devnameonly ? true : false;
    char   buf[256];
    int    i;

    UNREFERENCED( cmdline );
    UPPER_ARGV_0(  argv   );

    // panopt [FULLPATH|NAMEONLY] [RATE=nnn] [MSGCOLOR=NO|DARK|LIGHT] [TITLE=xxx]

    if (argc <= 1)
    {
        /* Display current settings */

        char* quote = sysblk.pantitle ?
            strpbrk( sysblk.pantitle, WHITESPACE ) ? "\"" : "" : "";

        MSGBUF( buf, "%s RATE=%d MSGCOLOR=%s %sTITLE=%s%s"
            , sysblk.devnameonly ? "NAMEONLY" : "FULLPATH"
            , sysblk.panrate
            , sysblk.pan_colors == PANC_NONE  ? "NO"    :
              sysblk.pan_colors == PANC_DARK  ? "DARK"  :
              sysblk.pan_colors == PANC_LIGHT ? "LIGHT" : "(err!)"
            , quote , sysblk.pantitle ? sysblk.pantitle : "(default)" , quote
        );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );
        return 0;
    }

    /* Too many arguments? */
    if (argc > 5)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /* Examine each argument in turn */
    for (i=1; i < argc; i++)
    {
        if      (CMD( argv[i], NAMEONLY,        4 )) devnameonly = true;
        else if (CMD( argv[i], FULLPATH,        4 )) devnameonly = false;
        else if (CMD( argv[i], MSGCOLOR=NO,    11 )) pan_colors  = PANC_NONE;
        else if (CMD( argv[i], MSGCOLOR=DARK,  13 )) pan_colors  = PANC_DARK;
        else if (CMD( argv[i], MSGCOLOR=LIGHT, 14 )) pan_colors  = PANC_LIGHT;
        else if (CMD( argv[i], RATE=FAST,       9 )) panrate = PANEL_REFRESH_RATE_FAST;
        else if (CMD( argv[i], RATE=SLOW,       9 )) panrate = PANEL_REFRESH_RATE_SLOW;
        else if (strncasecmp( argv[i], "RATE=", 5 ) == 0)
        {
            int rate = 0;
            int rc = sscanf( argv[i]+5, "%d", &rate );

            if (1
                && rc > 0
                && rate >= PANEL_REFRESH_RATE_MIN
                && rate <= PANEL_REFRESH_RATE_MAX
            )
            {
                panrate = rate;
            }
            else
            {
                char buf[20];
                char buf2[64];

                if (rc == 0)
                {
                    MSGBUF( buf, "%s", argv[i]+5 );
                    MSGBUF( buf2, "; not numeric value" );
                }
                else
                {
                    MSGBUF( buf, "%d", rate );
                    MSGBUF( buf2, "; not within range %d to %d inclusive",
                        (int)PANEL_REFRESH_RATE_MIN,
                        PANEL_REFRESH_RATE_MAX );
                }

                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", buf, buf2 );
                return -1;
            }
        }
        else if (strncasecmp( argv[i], "TITLE=", 6 ) == 0)
        {
            pantitle = argv[i]+6;
        }
        else
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[i], "" );
            return -1;
        }
    }

    /* Update setting(s) */

    sysblk.panrate     = panrate;
    sysblk.pan_colors  = pan_colors;
    sysblk.devnameonly = devnameonly;

    if (pantitle)
    {
        free( sysblk.pantitle );
        sysblk.pantitle = (strlen( pantitle ) > 0) ? strdup( pantitle ) : NULL;
        set_console_title( NULL );
    }
    set_panel_colors();

    if (MLVL( VERBOSE ))
    {
        char* quote = sysblk.pantitle ?
            strpbrk( sysblk.pantitle, WHITESPACE ) ? "\"" : "" : "";

        MSGBUF( buf, "%s RATE=%d MSGCOLOR=%s %sTITLE=%s%s"
            , sysblk.devnameonly ? "NAMEONLY" : "FULLPATH"
            , sysblk.panrate
            , sysblk.pan_colors == PANC_NONE  ? "NO"    :
              sysblk.pan_colors == PANC_DARK  ? "DARK"  :
              sysblk.pan_colors == PANC_LIGHT ? "LIGHT" : "(err!)"
            , quote , sysblk.pantitle ? sysblk.pantitle : "(default)" , quote
        );

        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], buf );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* panrate command - display or set rate at which console refreshes  */
/*-------------------------------------------------------------------*/
int panrate_cmd( int argc, char* argv[], char* cmdline )
{
    char msgbuf[16];

    UNREFERENCED( cmdline );
    UPPER_ARGV_0(  argv   );

    // "Command '%s' is deprecated%s"
    WRMSG( HHC02256, "W", argv[0], "; use PANOPT RATE=xxx instead" );

    if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if (argc == 2)
    {
        if      (CMD( argv[1], FAST, 4 )) sysblk.panrate = PANEL_REFRESH_RATE_FAST;
        else if (CMD( argv[1], SLOW, 4 )) sysblk.panrate = PANEL_REFRESH_RATE_SLOW;
        else
        {
            int trate = 0;
            int rc;

            rc = sscanf( argv[1], "%d", &trate );

            if (rc > 0 && trate >= (1000 / CLK_TCK) && trate <= 5000)
                sysblk.panrate = trate;
            else // error
            {
                char buf[20];
                char buf2[64];

                if (rc == 0)
                {
                    MSGBUF( buf, "%s", argv[1] );
                    MSGBUF( buf2, "; not numeric value" );
                }
                else
                {
                    MSGBUF( buf, "%d", trate );
                    MSGBUF( buf2, "; not within range %d to 5000 inclusive",
                        (1000 / (int) CLK_TCK ));
                }

                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", buf, buf2 );
                return -1;
            }
        }

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], argv[1] );
    }
    else
    {
        MSGBUF( msgbuf, "%d", sysblk.panrate );
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], msgbuf );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* pantitle xxxxxxxx command - set console title                     */
/*-------------------------------------------------------------------*/
int pantitle_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );
    UPPER_ARGV_0(  argv   );

    // "Command '%s' is deprecated%s"
    WRMSG( HHC02256, "W", argv[0], "; use PANOPT TITLE=xxx instead" );

    if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /* Update pantitle if operand is specified */
    if (argc == 2)
    {
        free( sysblk.pantitle );

        sysblk.pantitle = (strlen( argv[1] ) == 0) ? NULL : strdup( argv[1] );

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0],
                sysblk.pantitle ? sysblk.pantitle : "(none)" );

        set_console_title( NULL );
    }
    else
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0],
            sysblk.pantitle ? sysblk.pantitle : "(none)" );

    return 0;
}

/*-------------------------------------------------------------------*/
/* shell command                                                     */
/*-------------------------------------------------------------------*/
int sh_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( argc );
    UNREFERENCED( argv );

    if (!(sysblk.shcmdopt & SHCMDOPT_ENABLE))
    {
        // "Shell/Exec commands are disabled"
        WRMSG( HHC02227, "E" );
        return -1;
    }

    for (cmdline += 2; isspace( (unsigned char)*cmdline ); ++cmdline) /* (nop) */;
    return (*cmdline) ? herc_system( cmdline ) : -1;
}

/*-------------------------------------------------------------------*/
/* pgmprdos config command                                           */
/*-------------------------------------------------------------------*/
int pgmprdos_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Parse program product OS allowed */
    if (argc == 2)
    {
        if (0
            || CMD( argv[1], LICENSED, 3 )
            || CMD( argv[1], LICENCED, 3 )  // (alternate spelling)
        )
        {
            losc_set( PGM_PRD_OS_LICENSED );
        }
        else if (CMD( argv[1], RESTRICTED, 3 ))
        {
            losc_set( PGM_PRD_OS_RESTRICTED );
        }
        else
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "" );
            return -1;
        }
    }
    else
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* diag8cmd command                                                  */
/*-------------------------------------------------------------------*/
int diag8_cmd( int argc, char* argv[], char* cmdline )
{
    int i;
    char buf[40];

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc > 3)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Did they enter any arguments to the command? */
    if (argc > 1)
    {
        BYTE diag8opt = sysblk.diag8opt;

        /* Parse diag8cmd argument(s) */
        for (i=1; i < argc; i++)
        {
            if (CMD( argv[i], echo, 4 ))
                diag8opt |= DIAG8CMD_ECHO;
            else
            if (CMD( argv[i], noecho, 6 ))
                diag8opt &= ~DIAG8CMD_ECHO;
            else
            if (CMD( argv[i], enable, 3 ))
                diag8opt |= DIAG8CMD_ENABLE;
            else
            if (CMD( argv[i], disable, 4 ))
                diag8opt &= ~DIAG8CMD_ENABLE;
            else
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[i], "" );
                return -1;
            }
        }

        /* Update sysblk */
        sysblk.diag8opt = diag8opt;

        if (MLVL( VERBOSE ))
        {
            MSGBUF( buf, "%sABLE  %sECHO",
                (sysblk.diag8opt & DIAG8CMD_ENABLE) ? "EN" : "DIS",
                (sysblk.diag8opt & DIAG8CMD_ECHO)   ? ""   : "NO" );

            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], buf );
        }
    }
    else /* Display current diag8cmd settings */
    {
        MSGBUF( buf, "%sABLE  %sECHO",
            (sysblk.diag8opt & DIAG8CMD_ENABLE) ? "EN" : "DIS",
            (sysblk.diag8opt & DIAG8CMD_ECHO)   ? ""   : "NO" );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", "DIAG8CMD", buf );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* shcmdopt command                                                  */
/*-------------------------------------------------------------------*/
int shcmdopt_cmd( int argc, char* argv[], char* cmdline )
{
    int i;
    char buf[40];

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc > 1)
    {
        BYTE shcmdopt = sysblk.shcmdopt;

        for (i=1; i < argc; i++)
        {
                 if (CMD( argv[i], enable,  3 )) shcmdopt |= SHCMDOPT_ENABLE;
            else if (CMD( argv[i], diag8,   4 )) shcmdopt |= SHCMDOPT_DIAG8;
            else if (CMD( argv[i], disable, 4 )) shcmdopt &= ~SHCMDOPT_ENABLE;
            else if (CMD( argv[i], nodiag8, 6 )) shcmdopt &= ~SHCMDOPT_DIAG8;
            else
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[i], "" );
                return -1;
            }
        }

        /* Update sysblk */
        sysblk.shcmdopt = shcmdopt;

        if (MLVL( VERBOSE ))
        {
            MSGBUF( buf, "%sABLE  %sDIAG8"
                , (sysblk.shcmdopt & SHCMDOPT_ENABLE) ? "EN" : "DIS"
                , (sysblk.shcmdopt & SHCMDOPT_DIAG8)  ? ""   : "NO"
            );

            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], buf );
        }
    }
    else
    {
        MSGBUF( buf, "%sABLE  %sDIAG8"
            , (sysblk.shcmdopt & SHCMDOPT_ENABLE) ? "EN" : "DIS"
            , (sysblk.shcmdopt & SHCMDOPT_DIAG8)  ? ""   : "NO"
        );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* legacysenseid command                                             */
/*-------------------------------------------------------------------*/
int legacysenseid_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    /* Parse Legacy SenseID option */
    if ( argc > 2 )
    {
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    if ( argc == 2 )
    {
        if ( CMD(argv[1],enable,3) || CMD(argv[1],on,2) )
            sysblk.legacysenseid = TRUE;
        else if ( CMD(argv[1],disable,4) || CMD(argv[1],off,3) )
            sysblk.legacysenseid = FALSE;
        else
        {
            WRMSG( HHC02205, "E", argv[1] , "" );
            return -1;
        }
        if ( MLVL(VERBOSE) )
            WRMSG( HHC02204, "I", argv[0],
                   sysblk.legacysenseid ? "enabled" : "disabled" );
    }
    else
        WRMSG( HHC02203, "I", argv[0], sysblk.legacysenseid ? "enabled" : "disabled" );

    return 0;
}

/*-------------------------------------------------------------------
 * cp_updt command       User code page management
 *
 * cp_updt altER ebcdic|ascii (p,v)       modify user pages
 * cp_updt refERENCE codepage             copy existing page to user area
 * cp_updt reset                          reset user area contents to '\0'
 * cp_updt dsp|disPLAY ebcdic|ascii       display user pages
 * cp_updt impORT ebcdic|ascii filename   import filename to user table
 * cp_updt expORT ebcdic|ascii filename   export filename to user table
 *
 * ebcdic = g2h (Guest to Host) Guest = Mainframe OS
 * ascii  = h2g (Host to Guest) Host  = PC OS (Unix/Linux/Windows)
 *-------------------------------------------------------------------*/
int cp_updt_cmd(int argc, char *argv[], char *cmdline)
{
    int     rc = 0;

    UNREFERENCED(cmdline);

    /*  This is coded in this manner in case additional checks are
     *  made later.
     */

    if ( argc == 2 && CMD(argv[1],reset,5) )
    {
        argc--;
        argv++;

        rc = update_codepage( argc, argv, argv[0] );
    }
    else if ( ( argc == 2 || argc == 3 ) && CMD(argv[1],reference,3) )
    {
        argc--;
        argv++;

        rc = update_codepage( argc, argv, argv[0] );
    }
    else if ( ( argc == 3 ) && ( CMD(argv[1],dsp,3) || CMD(argv[1],display,3) ) )
    {
        argc--;
        argv++;

        rc = update_codepage( argc, argv, argv[0] );
    }
    else if ( ( argc == 2 ) && CMD(argv[1],test,4) )
    {
        argc--;
        argv++;

        rc = update_codepage( argc, argv, argv[0] );
    }
    else if ( ( argc == 4 || argc == 6 ) && CMD(argv[1],alter,3) )
    {
        argc--;
        argv++;

        rc = update_codepage( argc, argv, argv[0] );
    }
    else if ( ( argc == 4 || argc == 6 ) && CMD(argv[1],export,3) )
    {
        argc--;
        argv++;

        rc = update_codepage( argc, argv, argv[0] );
    }
    else if ( ( argc == 4 || argc == 6 ) && CMD(argv[1],import,3) )
    {
        argc--;
        argv++;

        rc = update_codepage( argc, argv, argv[0] );
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* codepage xxxxxxxx command      *** KEEP AFTER CP_UPDT_CMD ***     */
/* Note: maint can never be a code page name                         */
/*-------------------------------------------------------------------*/
int codepage_cmd( int argc, char* argv[], char* cmdline )
{
    int rc = 0;

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    /* passthru to cp_updt_cmd */
    if (argc >= 2 && CMD( argv[1], MAINT, 1 ))
    {
        argc--;
        argv++;
        rc = cp_updt_cmd( argc, argv, NULL );
    }
    else if (argc == 2 && valid_codepage_name( argv[1] ))
    {
        /* Update codepage if valid operand is specified */
        set_codepage( argv[1] );
    }
    else if (argc == 1)
    {
        const char* cp = query_codepage();
        // "Codepage is %s"
        WRMSG( HHC01476, "I", cp ? cp : "(NULL)" );
    }
    else
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* model config statement                                            */
/* operands: hardware_model [capacity_model [perm_model temp_model]] */
/*-------------------------------------------------------------------*/
int stsi_model_cmd( int argc, char* argv[], char* cmdline )
{
    const char* model_name[4] = { "hardware", "capacity", "perm", "temp" };
    char*            model[4] = { "",         "",         "",     "" };

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    /* Update model name if operand is specified */

    if (argc > 1)
    {
        int rc, m, n;

        /* Validate argument count */

        if (argc > 5)
        {
            // "Invalid number of arguments for %s"
            WRMSG( HHC01455, "E", argv[0] );
            return -1;
        }

        /* Validate and set new model and capacity
           numbers according to arguments */
        for (m=0, n=1; n < argc; m++, n++)
        {
            size_t len, i;

            if (!argv[n])
                break;

            model[m] = argv[n];
            len = strlen( model[m] );

            if (len > 16)
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", model[n], "; argument > 16 characters" );
                return -1;
            }

            /* Normal handling?  (i.e. is special handling character
               "*" or "=" NOT specified for this field?)
            */
            if (!(len == 1 && (model[m][0] == '*' || model[m][0] == '=')))
            {
                for (i=0; i < len; i++)
                {
                    if (!isalnum( (unsigned char)model[m][i] ) ||
                       (!isupper( (unsigned char)model[m][i] ) && !isdigit( (unsigned char)model[m][i] )))
                    {
                        char msgbuf[64];
                        MSGBUF( msgbuf, "%s-model = <%s>", model_name[m], model[m] );

                        // "Invalid argument %s%s"
                        WRMSG( HHC02205, "E", msgbuf, "; argument contains an invalid character (0-9 and uppercase A-Z only)"  );
                        return -1;
                    }
                }
            }
        }

        /* TRY setting their requested values... */

        if ((rc = set_model( model[0], model[1], model[2], model[3] )) != 0)
        {
            /* A non-zero return code indicates which field was bad */

            if (rc > 0 && rc <= 4)
            {
                char msgbuf[64];
                MSGBUF( msgbuf, "%s-model = <%s>", model_name[rc-1], model[rc-1] );

                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", msgbuf, "; argument contains an invalid character (0-9 and uppercase A-Z only)" );
            }
            else
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[0], "" );

            return -1;
        }

        /* Success: show them the results */

        if (MLVL( VERBOSE ))
        {
            char msgbuf[128];
            MSGBUF( msgbuf, "hardware(%s) capacity(%s) perm(%s) temp(%s)",
                            str_modelhard(), str_modelcapa(), str_modelperm(), str_modeltemp() );
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", "MODEL", msgbuf );
        }
    }
    else /* (no arguments == query current values) */
    {
        char msgbuf[128];
        MSGBUF( msgbuf, "hardware(%s) capacity(%s) perm(%s) temp(%s)",
                        str_modelhard(), str_modelcapa(), str_modelperm(), str_modeltemp() );
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], msgbuf );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* plant config statement                                            */
/*-------------------------------------------------------------------*/
int stsi_plant_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    /* Update plant name if operand is specified */
    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    if (argc == 1)  /* (no argument == query current value?) */
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], str_plant() );
    }
    else
    {
        size_t i;

        if (strlen( argv[1] ) > 4)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "; argument > 4 characters" );
            return -1;
        }

        for (i=0; i < strlen( argv[1] ); i++)
        {
            if (isalnum( (unsigned char)argv[1][i] ) &&
               (isupper( (unsigned char)argv[1][i] ) || isdigit( (unsigned char)argv[1][i] )))
                continue;

            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "; argument contains an invalid character (0-9 and uppercase A-Z only)" );
            return -1;
        }

        if (set_plant( argv[1] ) < 0)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "; argument contains an invalid character (0-9 and uppercase A-Z only)" );
            return -1;
        }

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], str_plant() );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* manufacturer config statement                                     */
/*-------------------------------------------------------------------*/
int stsi_manufacturer_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    /* Update manufacturer name if operand is specified */
    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    if (argc == 1)  /* (no argument == query current value?) */
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], str_manufacturer() );
    }
    else
    {
        size_t i;

        if (strlen (argv[1] ) > 16)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "; argument > 16 characters" );
            return -1;
        }

        for (i=0; i < strlen( argv[1] ); i++)
        {
            if (isalnum( (unsigned char)argv[1][i] ) &&
               (isupper( (unsigned char)argv[1][i] ) || isdigit( (unsigned char)argv[1][i] )))
                continue;

            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "; argument contains an invalid character (0-9 and uppercase A-Z only)" );
            return -1;
        }

        if (set_manufacturer( argv[1] ) < 0)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "; argument contains an invalid character (0-9 and uppercase A-Z only)");
            return -1;
        }

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], str_manufacturer() );
    }

    return 0;
}

#if defined( OPTION_SHARED_DEVICES )
/*-------------------------------------------------------------------*/
/* shrdport - shared dasd port number                                */
/*-------------------------------------------------------------------*/
int shrdport_cmd( int argc, char* argv[], char* cmdline )
{
    static int default_shrdport = SHARED_DEFAULT_PORT;
    U16  shrdport;
    BYTE c;
    char buf[16] = {0};

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Check for correct number of arguments */
    if (argc < 1 || argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Report current shared device port number */
    if (argc < 2)
    {
        // "%-14s: %s"
        MSGBUF( buf, "%hu", sysblk.shrdport );
        WRMSG( HHC02203, "I", argv[0], buf );
        return 0;
    }

    /* Update shared device port number */
    if (CMD( argv[1], START, 5 ))
        configure_shrdport( default_shrdport );
    else
    if (CMD( argv[1], STOP, 4 ))
        configure_shrdport( 0 );
    else
    if (1
        && strlen(  argv[1] ) >= 1
        && sscanf( argv[1], "%hu%c", &shrdport, &c ) == 1
        && (shrdport >= 1024 || shrdport == 0)
    )
    {
        if (!configure_shrdport( shrdport ))
            default_shrdport = shrdport;
    }
    else
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return 1;
    }

    if (MLVL( VERBOSE ))
    {
        MSGBUF( buf, "%hu", sysblk.shrdport );
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], buf );
    }

    return 0;
}
#endif /* defined( OPTION_SHARED_DEVICES ) */

/*-------------------------------------------------------------------*/
/* lparname - set or display LPAR name                               */
/*-------------------------------------------------------------------*/
int lparname_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc < 1 || argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if (argc == 1)
    {
        // "%-14s: %s"
        WRMSG(HHC02203, "I", argv[0], str_lparname());
        return 0;
    }

    set_lparname( argv[1] );

    set_symbol( "LPARNAME", str_lparname() );

    if (MLVL( VERBOSE ))
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], str_lparname() );

    return 0;
}

/*-------------------------------------------------------------------*/
/* lparnum command - set or display LPAR identification number       */
/*-------------------------------------------------------------------*/
int lparnum_cmd( int argc, char* argv[], char* cmdline )
{
    size_t  arglen;
    U16     lparnum;
    char    chlparnum[20];
    char    chcpuidfmt[20];
    BYTE    c;
    BYTE    resetSuccessful;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc < 1 || argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /* Display LPAR identification number */
    if (argc == 1)
    {
        if (sysblk.lparmode)
            MSGBUF( chcpuidfmt, sysblk.cpuidfmt ? "%02X" : "%01X",
                    sysblk.lparnum );
        else
            STRLCPY( chcpuidfmt, "BASIC" );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], chcpuidfmt );
        return 0;
    }

    arglen = strlen( argv[1] );

    /* Update LPAR identification number */
    if (1
        && (arglen == 1 || arglen == 2)
        && sscanf( argv[1], "%hx%c", &lparnum, &c ) == 1
    )
    {
        if (1
            && ARCH_370_IDX == sysblk.arch_mode
            && (lparnum == 0 || lparnum > 16)
        )
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], ": must be 1 to 10 (hex) for ARCHMODE S370" );
            return -1;
        }

        /* Set new LPAR number, CPU ID format and operation mode */
        OBTAIN_INTLOCK( NULL );
        enable_lparmode( true );
        sysblk.lparnum = lparnum;

        if (lparnum == 0)
            sysblk.cpuidfmt = 1;
        else if (sysblk.cpuidfmt)
        {
            if (arglen == 1)
                sysblk.cpuidfmt = 0;
        }
        else if (arglen == 2 && lparnum != 16)
            sysblk.cpuidfmt = 1;

        setOperationMode();
    }
    else if (1
        && arglen == 5
        && strcasecmp( argv[1], "BASIC" ) == 0
    )
    {
        /* Set new LPAR number, CPU ID format and operation mode */
        OBTAIN_INTLOCK( NULL );
        enable_lparmode( false );
        sysblk.lparnum  = 0;
        sysblk.cpuidfmt = 0;
        sysblk.operation_mode = om_basic;
    }
    else
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1],
            ": must be BASIC, 1 to F (hex) or 00 to FF (hex)" );
        return -1;
    }

    /* Update all CPU IDs to reflect new LPARNUM */
    resetSuccessful = resetAllCpuIds();
    RELEASE_INTLOCK( NULL );

    /* Abort if unable to successfully reset all CPU Ids */
    if (!resetSuccessful)
        return -1;

    /* Report successful results */
    if (om_basic == sysblk.operation_mode)
    {
        STRLCPY( chcpuidfmt, "BASIC" );
        STRLCPY( chlparnum,  "BASIC" );
    }
    else // LPAR mode
    {
        STRLCPY( chcpuidfmt, sysblk.cpuidfmt ? "1"    : "0" );
        MSGBUF(  chlparnum,  sysblk.cpuidfmt ? "%02X" : "%01X", sysblk.lparnum );
    }

    set_symbol( "CPUIDFMT", chcpuidfmt );
    set_symbol( "LPARNUM",  chlparnum  );

    if (MLVL( VERBOSE ))
    {
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], chlparnum );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* cpuverid command - set or display cpu version                     */
/*-------------------------------------------------------------------*/
int cpuverid_cmd( int argc, char* argv[], char* cmdline )
{
    U32   version;
    char  chversion[16];
    BYTE  c;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc < 1 || argc > 3)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Display CPU version */
    if (argc == 1)
    {
        MSGBUF( chversion, "%02X", sysblk.cpuversion );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], chversion );
        return 0;
    }

    /* Update CPU version */
    if (1
        && (strlen( argv[1] ) == 2)
        && (sscanf( argv[1], "%x%c", &version, &c ) == 1)
    )
    {
        bool force = false;
        char* sev = "I";

        /* Check for 'FORCE' option */
        if (argc == 3)
        {
            if (CMD( argv[2], FORCE, 5 ))
                force = true;
            else
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[2], "" );
                return -1;
            }
        }

        /* Update all CPU identifiers */
        if (!setAllCpuIds_lock( -1, version, -1, -1, force ))
            return -1;

        MSGBUF( chversion,"%02X", sysblk.cpuversion );
        set_symbol( "CPUVERID", chversion );

        if (force)
        {
            sev = "W";
            MSGBUF( chversion,"%02X (FORCED)", sysblk.cpuversion );
        }

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, sev, argv[0], chversion );
    }
    else
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* cpumodel command - set or display cpu model                       */
/*-------------------------------------------------------------------*/
int cpumodel_cmd( int argc, char* argv[], char* cmdline )
{
    U32   cpumodel;
    char  chmodel[8];
    BYTE  c;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc < 1 || argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Display CPU model */
    if (argc == 1)
    {
        MSGBUF( chmodel, "%04X", sysblk.cpumodel );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], chmodel );
        return 0;
    }

    /* Update CPU model */
    if (1
        && (strlen( argv[1] ) >= 2)
        && (strlen( argv[1] ) <= 4)
        && (sscanf( argv[1], "%x%c", &cpumodel, &c ) == 1)
    )
    {
        /* Update all CPU IDs */
        if (!setAllCpuIds_lock( cpumodel, -1, -1, -1, false ))
            return -1;

        MSGBUF( chmodel, "%04X", sysblk.cpumodel );

        set_symbol( "CPUMODEL", chmodel );

        if (MLVL( VERBOSE ))
        {
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], chmodel );

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
            txf_model_warning( FACILITY_ENABLED_ARCH( 073_TRANSACT_EXEC, ARCH_900_IDX ));
#endif
        }
    }
    else
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* cpuserial command - set or display cpu serial                     */
/*-------------------------------------------------------------------*/
int cpuserial_cmd( int argc, char* argv[], char* cmdline )
{
    U32   cpuserial;
    char  chserial[8];
    BYTE  c;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc < 1 || argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Display CPU serial */
    if (argc == 1)
    {
        cpuserial = (U32) ((sysblk.cpuid & 0x00FFFFFF00000000ULL) >> 32);
        MSGBUF( chserial, "%06X", cpuserial );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], chserial );
        return 0;
    }

    /* Update CPU serial */
    if (1
        && (strlen( argv[1] ) >= 1)
        && (strlen( argv[1] ) <= 6)
        && (sscanf( argv[1], "%x%c", &cpuserial, &c ) == 1)
    )
    {
        /* Update all CPU IDs */
        if (!setAllCpuIds_lock( -1, -1, cpuserial, -1, false ))
            return -1;

        /* Show them the now newly-updated SYSBLK value */
        cpuserial = (U32) ((sysblk.cpuid & 0x00FFFFFF00000000ULL) >> 32);
        MSGBUF( chserial, "%06X", cpuserial );

        set_symbol( "CPUSERIAL", chserial );

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], chserial );
    }
    else
    {
        // "Invalid value %s specified for %s"
        WRMSG( HHC01451, "E", argv[1], argv[0] );
        return -1;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* cpuidfmt command - set or display STIDP format {0|1|BASIC}        */
/*-------------------------------------------------------------------*/
int cpuidfmt_cmd( int argc, char* argv[], char* cmdline )
{
    size_t  arglen;
    u_int   fmt;
    char    chfmt[40];
    BYTE    resetSuccessful;

    UNREFERENCED( cmdline);

    UPPER_ARGV_0( argv );

    if (argc < 1 || argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    /* Display CPUIDFMT*/
    if (argc == 1)
    {
        if (sysblk.lparmode)
            MSGBUF( chfmt, "%d", sysblk.cpuidfmt );
        else
            STRLCPY( chfmt, "BASIC" );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], chfmt );
        return 0;
    }

    arglen = strlen( argv[1] );

    /* Update the CPUIDFMT */
    if (1
        && arglen == 1
        && sscanf( argv[1], "%u", &fmt ) == 1
    )
    {
        if (fmt > 1)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], ": must be either 0 or 1" );
            return -1;
        }

        if (!sysblk.lparmode)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1],": not in LPAR mode" );
            return -1;
        }

        /* Setting format-0 cpuid format? */
        if (!fmt)
        {
            if (!sysblk.lparnum || sysblk.lparnum > 16)
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[1],": LPAR number not in range of 1 to 10 (hex)" );
                return -1;
            }
        }
        else /* Setting format-1 cpuid format */
        {
            if (ARCH_370_IDX == sysblk.arch_mode)
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[1], ": must be 0 for System/370" );
                return -1;
            }
        }

        /* Update the CPUIDFMT and reset all CPU Ids */

        OBTAIN_INTLOCK( NULL );

        if (sysblk.cpuidfmt != fmt)
        {
            sysblk.cpuidfmt = fmt;
            setOperationMode();
        }
    }
    else if (1
        && arglen == 5
        && strcasecmp( argv[1], "BASIC" ) == 0
    )
    {
        if (sysblk.lparmode)
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1],": In LPAR mode" );
            return -1;
        }

        /* Make sure all CPUs are deconfigured or stopped */
        if (are_any_cpus_started())
        {
            // "All CPU's must be stopped %s"
            WRMSG( HHC02253, "E", "to change CPUIDFMT" );
            return HERRCPUONL;
        }

        /* Update the CPUIDFMT and reset all CPU Ids */

        OBTAIN_INTLOCK( NULL );

        if (sysblk.cpuidfmt)
        {
            sysblk.cpuidfmt = 0;
            sysblk.lparnum  = 0;
            sysblk.operation_mode = om_basic;
        }
    }
    else
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], "" );
        return -1;
    }

    resetSuccessful = resetAllCpuIds();
    RELEASE_INTLOCK( NULL );

    /* Abort if unable to successfully reset all CPU Ids */
    if (!resetSuccessful)
        return -1;

    /* Report successful results */

    if (sysblk.lparmode)
        MSGBUF( chfmt, "%d", sysblk.cpuidfmt );
    else
        STRLCPY( chfmt, "BASIC" );

    set_symbol( "CPUIDFMT", chfmt );

    if (MLVL( VERBOSE ))
    {
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], chfmt );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* loadparm - set or display IPL parameter                           */
/*-------------------------------------------------------------------*/
int loadparm_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    /* Update the default loadparm value if operand is specified */
    if (argc > 2)
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return -1;
    }

    if (argc == 2)
    {
        STRLCPY( sysblk.loadparm, argv[1] );
        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], sysblk.loadparm );
    }
    else
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], sysblk.loadparm );

    return 0;
}

/*-------------------------------------------------------------------*/
/* devlist and qd command helper functions                           */
/*-------------------------------------------------------------------*/
static int SortDevBlkPtrsAscendingByDevnum(const void* pDevBlkPtr1, const void* pDevBlkPtr2)
{
    int rc;

    rc = (int)((*(DEVBLK**)pDevBlkPtr1)->devnum) -
         (int)((*(DEVBLK**)pDevBlkPtr2)->devnum);
    return rc;
}

static BYTE is_devclass_name( const char* name )
{
    return (0
        || strcasecmp( name, "CHAN" ) == 0
        || strcasecmp( name, "CON"  ) == 0
        || strcasecmp( name, "CTCA" ) == 0
        || strcasecmp( name, "DASD" ) == 0
        || strcasecmp( name, "DSP"  ) == 0
        || strcasecmp( name, "FCP"  ) == 0
        || strcasecmp( name, "LINE" ) == 0
        || strcasecmp( name, "OSA"  ) == 0
        || strcasecmp( name, "PCH"  ) == 0
        || strcasecmp( name, "PRT"  ) == 0
        || strcasecmp( name, "RDR"  ) == 0
        || strcasecmp( name, "TAPE" ) == 0
    );
}

/*-------------------------------------------------------------------*/
/* devlist command - list devices                                    */
/*-------------------------------------------------------------------*/
int devlist_cmd( int argc, char* argv[], char* cmdline )
{
    DEVBLK*   dev;
    char*     devclass;
    char      devstat[1024];
    char      devtype[8] = "";
    DEVBLK**  pDevBlkPtr;
    DEVBLK**  orig_pDevBlkPtrs;
    size_t    nDevCount, i, cnt;
    int       bTooMany = FALSE;
    U16       lcss;
    U16       ssid=0;
    U16       devnum;
    int       single_devnum = FALSE;
    char      buf[1024];
    char      cdevnum[8];

    DEVNUMSDESC  dnd;
    size_t       devncount = 0;
    int          dev_found = FALSE;

    UNREFERENCED( cmdline );

    if (1
        && argc == 2
        && strlen( argv[1] ) >= 3
        && strlen( argv[1] ) <= 4
    )
    {
        if (is_devclass_name(  argv[1] ))
            strupper( devtype, argv[1] );
    }

    if (argc >= 2 && !strlen( devtype ))
    {
        // We now also support multiple CCUU addresses.
        if (1
            && str_caseless_ne( argv[1], "sysg" )
            && (devncount = parse_devnums( argv[1], &dnd )) > 0
        )
        {
            ssid = LCSS_TO_SSID( dnd.lcss );
        }
        else
        {
            single_devnum = TRUE;

            if ( parse_single_devnum( argv[1], &lcss, &devnum ) < 0 )
            {
                // (error message already issued)
                return -1;
            }

            if (!(dev = find_device_by_devnum( lcss, devnum )))
            {
                // HHC02200 "%1d:%04X device not found"
                devnotfound_msg( lcss, devnum );
                return -1;
            }

            ssid = LCSS_TO_SSID( lcss );
        }
    }

    // Since we wish to display the list of devices in ascending device
    // number order, we build our own private sorted array of DEVBLK
    // pointers and use that instead to make the devlist command wholly
    // immune from the actual order/sequence of the actual DEVBLK chain.

    // Note too that there is no lock to lock access to ALL device blocks
    // (even though there really SHOULD be). The only lock there is is one
    // to lock an individual DEVBLK (which doesn't do us much good here).

    if (!(orig_pDevBlkPtrs = malloc( sizeof( DEVBLK* ) * MAX_DEVLIST_DEVICES )))
    {
        MSGBUF( buf, "malloc(%d)", (int)( sizeof( DEVBLK* ) * MAX_DEVLIST_DEVICES ));
        WRMSG( HHC02219, "E", buf, strerror( errno ));
        return -1;
    }

    nDevCount = 0;
    pDevBlkPtr = orig_pDevBlkPtrs;

    for (dev = sysblk.firstdev; dev && nDevCount <= MAX_DEVLIST_DEVICES; dev = dev->nextdev)
    {
        if (dev->allocated)  // (valid device?)
        {
            if (single_devnum && (dev->ssid != ssid || dev->devnum != devnum))
                continue;

            // Multiple devnum support is active when devncount > 0,
            // otherwise we are in single devnum or ALL devnum mode.
            for (i=0, dev_found=FALSE; dev_found==FALSE && i < devncount && !bTooMany; i++)
            {
                if (1
                    && dev->ssid == ssid
                    && dev->devnum >= dnd.da[i].cuu1
                    && dev->devnum <= dnd.da[i].cuu2
                )
                    dev_found = TRUE;
            }
            if (devncount > 0 && dev_found == FALSE)
                continue;

            if (nDevCount < MAX_DEVLIST_DEVICES)
            {
                *pDevBlkPtr = dev;      // (save ptr to DEVBLK)
                nDevCount++;            // (count array entries)
                pDevBlkPtr++;           // (bump to next entry)

                if (single_devnum)
                    break;
            }
            else
            {
                bTooMany = TRUE;        // (no more room)
                break;                  // (no more room)
            }
        }
    }

    ASSERT( nDevCount <= MAX_DEVLIST_DEVICES );   // (sanity check)

    // Sort the DEVBLK pointers into ascending sequence by device number.

    qsort( orig_pDevBlkPtrs, nDevCount, sizeof( DEVBLK* ), SortDevBlkPtrsAscendingByDevnum );

    // Now use our sorted array of DEVBLK pointers
    // to display our sorted list of devices...

    cnt = 0;

    for (i = nDevCount, pDevBlkPtr = orig_pDevBlkPtrs; i; --i, pDevBlkPtr++)
    {
        dev = *pDevBlkPtr;                  // --> DEVBLK

        /* Call device handler's query definition function */

#if defined( OPTION_SCSI_TAPE )
        if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            try_scsi_refresh( dev );  // (see comments in function)
#endif
        dev->hnd->query( dev, &devclass, sizeof( devstat ), devstat );

        if (0
            || strlen(     devtype ) == 0
            || strcasecmp( devtype, devclass ) == 0
        )
        {
            cnt++;

            /* Display the device definition and status */
            if (dev == sysblk.sysgdev)
                MSGBUF( cdevnum, "%s", "SYSG  " );
            else
                MSGBUF( cdevnum, "%1d:%04X", SSID_TO_LCSS( dev->ssid ),
                    dev->devnum );

            MSGBUF( buf, "%s %s %s %s%s%s"
                , cdevnum
                , dev->typname
                , devstat
                , dev->fd   >  2   ? "open "    : ""
                , dev->busy        ? "busy "    : ""
                , IOPENDING( dev ) ? "pending " : ""
            );

            // "%s" // devlist command
            WRMSG( HHC02279, "I", buf );

            if (dev->bs)
            {
                char *clientip, *clientname;

                get_connected_client( dev, &clientip, &clientname );

                if (clientip)
                {
                    MSGBUF( buf, "     (client %s (%s) connected)",
                        clientip, clientname );

                    // "%s" // devlist command
                    WRMSG( HHC02279, "I", buf );
                }
                else
                {
                    // "%s" // devlist command
                    WRMSG( HHC02279, "I", "     (no one currently connected)" );
                }

                free( clientip );
                free( clientname );
            }
        }
    }

    free( orig_pDevBlkPtrs );
    if (devncount > 0)
        free( dnd.da );

    if (bTooMany)
    {
        // "Not all devices shown (max %d)"
        WRMSG( HHC02237, "W", MAX_DEVLIST_DEVICES );
        return 2;      // (treat as warning)
    }

    if (cnt == 0)
    {
        // "Empty list"
        WRMSG( HHC02312, "W" );
        return 1;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* qd command - query device information                             */
/*-------------------------------------------------------------------*/
int qd_cmd( int argc, char* argv[], char* cmdline )
{
    DEVBLK*   dev;
    DEVBLK**  pDevBlkPtr;
    DEVBLK**  orig_pDevBlkPtrs;
    size_t    devncount = 0;
    size_t    nDevCount, i, j, num;
    int       bTooMany = 0;
    int       len = 0;
    U16       ssid = 0;
    BYTE      iobuf[256];
    BYTE      cbuf[17];
    char      buf[128];

    DEVNUMSDESC  dnd;
    char*        qdclass  = NULL;
    char*        devclass = NULL;

    UNREFERENCED( cmdline );

    if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    // Allocate work area

    if (!(orig_pDevBlkPtrs = malloc( sizeof( DEVBLK* ) * MAX_DEVLIST_DEVICES )))
    {
        // "Error in function %s: %s"
        MSGBUF( buf, "malloc(%d)", (int)( sizeof( DEVBLK* ) * MAX_DEVLIST_DEVICES ));
        WRMSG( HHC02219, "E", buf, strerror( errno ));
        return -1;
    }

    nDevCount = 0;
    pDevBlkPtr = orig_pDevBlkPtrs;

    if (argc == 2)
    {
        if (is_devclass_name( argv[1] ))
        {
            qdclass = argv[1];

            // Gather requested devices by class

            for (dev = sysblk.firstdev; dev && !bTooMany; dev = dev->nextdev)
            {
                if (!dev->allocated)
                    continue;

                dev->hnd->query( dev, &devclass, 0, NULL );

                if (strcasecmp( devclass, qdclass ) != 0)
                    continue;

                if (nDevCount < MAX_DEVLIST_DEVICES)
                {
                    *pDevBlkPtr = dev;      // (save ptr to DEVBLK)
                    nDevCount++;            // (count array entries)
                    pDevBlkPtr++;           // (bump to next entry)
                }
                else
                {
                    bTooMany = 1;           // (no more room)
                    break;                  // (no more room)
                }
            }
        }
        else if ((devncount = parse_devnums( argv[1], &dnd )) > 0)
        {
            ssid = LCSS_TO_SSID( dnd.lcss );

            // Gather requested devices by devnum(s)

            for (dev = sysblk.firstdev; dev && !bTooMany; dev = dev->nextdev)
            {
                if (!dev->allocated)
                    continue;

                for (i=0; i < devncount && !bTooMany; i++)
                {
                    if (1
                        && dev->ssid == ssid
                        && dev->devnum >= dnd.da[i].cuu1
                        && dev->devnum <= dnd.da[i].cuu2
                    )
                    {
                        if (nDevCount < MAX_DEVLIST_DEVICES)
                        {
                            *pDevBlkPtr = dev;      // (save ptr to DEVBLK)
                            nDevCount++;            // (count array entries)
                            pDevBlkPtr++;           // (bump to next entry)
                        }
                        else
                            bTooMany = 1;           // (no more room)
                        break;                      // (we found our device)
                    }
                }
            }

            free( dnd.da );
        }
        else
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            free( orig_pDevBlkPtrs );
            return -1;
        }
    }
    else
    {
        // Gather *ALL* devices

        for (dev = sysblk.firstdev; dev && !bTooMany; dev = dev->nextdev)
        {
            if (!dev->allocated)
                continue;

            if (nDevCount < MAX_DEVLIST_DEVICES)
            {
                *pDevBlkPtr = dev;      // (save ptr to DEVBLK)
                nDevCount++;            // (count array entries)
                pDevBlkPtr++;           // (bump to next entry)
            }
            else
            {
                bTooMany = 1;           // (no more room)
                break;                  // (no more room)
            }
        }
    }

    if (!nDevCount)
    {
        // "Empty list"
        WRMSG( HHC02312, "W" );
        free( orig_pDevBlkPtrs );
        return 0;
    }

    // Sort the DEVBLK pointers into ascending sequence by device number.

    qsort( orig_pDevBlkPtrs, nDevCount, sizeof(DEVBLK*), SortDevBlkPtrsAscendingByDevnum );

    // Now use our sorted array of DEVBLK pointers
    // to display our sorted list of devices...

    for (i = nDevCount, pDevBlkPtr = orig_pDevBlkPtrs; i; --i, pDevBlkPtr++)
    {
        dev = *pDevBlkPtr;                  // --> DEVBLK

        /* Display sense-id */
        if (dev->numdevid)
        {
            for (j=0; j < dev->numdevid; j++)
            {
                if (j == 0)
                    len = MSGBUF( buf, "%1d:%04X SNSID 00 ", LCSS_DEVNUM );
                else if (j % 16 == 0)
                {
                    // "%s" // qd command
                    WRMSG( HHC02280, "I", buf );
                    len = sprintf( buf, "             %2.2X ", (int) j );
                }
                if (j % 4 == 0)
                    len += sprintf( buf + len, " " );
                len += sprintf( buf + len, "%2.2X", dev->devid[j] );
            }
            // "%s" // qd command
            WRMSG( HHC02280, "I", buf );
        }

        /* Display device characteristics */
        if (dev->numdevchar)
        {
            for (j=0; j < dev->numdevchar; j++)
            {
                if (j == 0)
                    len = MSGBUF( buf, "%1d:%04X RDC   00 ", LCSS_DEVNUM );
                else if (j % 16 == 0)
                {
                    // "%s" // qd command
                    WRMSG( HHC02280, "I", buf );
                    len = sprintf( buf, "             %2.2X ", (int) j );
                }
                if (j % 4 == 0)
                    len += sprintf( buf + len, " " );
                len += sprintf( buf + len, "%2.2X", dev->devchar[j] );
            }
            // "%s" // qd command
            WRMSG( HHC02280, "I", buf );
        }

        /* Display configuration data */
        if (dev->rcd && (num = dev->rcd( dev, iobuf, 256 )) > 0)
        {
            cbuf[ sizeof( cbuf ) - 1 ] = 0;

            for (j=0; j < num; j++)
            {
                if (j == 0)
                    len = sprintf( buf, "%1d:%04X RCD   00 ", LCSS_DEVNUM );
                else if (j % 16 == 0)
                {
                    len += sprintf( buf + len, " |%s|", cbuf );
                    // "%s" // qd command
                    WRMSG( HHC02280, "I", buf );
                    len = sprintf( buf, "             %2.2X ", (int) j );
                }
                if (j % 4 == 0)
                    len += sprintf( buf + len, " " );
                len += sprintf( buf + len, "%2.2X", iobuf[j] );
                cbuf[ j % 16 ] = isprint( (unsigned char)guest_to_host( iobuf[j] )) ? guest_to_host( iobuf[j] ) : '.';
            }
            len += sprintf( buf + len, " |%s|", cbuf );
            // "%s" // qd command
            WRMSG( HHC02280, "I", buf );
        }

        /* If dasd, display subsystem status */
        if (dev->ckdcyls && (num = dasd_build_ckd_subsys_status( dev, iobuf, 44 )) > 0)
        {
            for (j=0; j < num; j++)
            {
                if (j == 0)
                    len = sprintf( buf, "%1d:%04X SNSS  00 ", LCSS_DEVNUM );
                else if (j % 16 == 0)
                {
                    // "%s" // qd command
                    WRMSG( HHC02280, "I", buf );
                    len = sprintf( buf, "             %2.2X ", (int) j );
                }
                if (j % 4 == 0)
                    len += sprintf( buf + len, " " );
                len += sprintf( buf + len, "%2.2X", iobuf[j] );
            }
            // "%s" // qd command
            WRMSG( HHC02280, "I", buf );
        }
    }

    free( orig_pDevBlkPtrs );

    if (bTooMany)
    {
        // "Not all devices shown (max %d)"
        WRMSG( HHC02237, "W", MAX_DEVLIST_DEVICES );
        return -1;      // (treat as error)
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* attach command - configure a device                               */
/*-------------------------------------------------------------------*/
int attach_cmd(int argc, char *argv[], char *cmdline)
{
    int rc;

    UNREFERENCED(cmdline);

    if (argc < 3)
    {
        WRMSG(HHC02202, "E", argv[0] );
        return -1;
    }
    rc = parse_and_attach_devices(argv[1],argv[2],argc-3,&argv[3]);

    return rc;
}

/*-------------------------------------------------------------------*/
/* detach command - remove device                                    */
/*-------------------------------------------------------------------*/
int detach_cmd( int argc, char* argv[], char* cmdline )
{
    DEVBLK  *dev;
    U16      lcss, devnum;
    int      rc;
    bool     force           = false;
    bool     intlock_needed  = false;

    UPPER_ARGV_0( argv );
    UNREFERENCED( cmdline );

    if (argc < 2)
    {
        missing_devnum();
        return -1;
    }

    /* The 'FORCE' option allows busy devices to be detached.
       PLEASE NOTE that doing so is inherently DANGEROUS and
       can easily cause Hercules to CRASH!
    */
    if (argc > 2)
    {
        if (argc > 3 || !(force = CMD( argv[2], FORCE, 5 )))
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }
    }

    if ((rc = parse_single_devnum( argv[1], &lcss, &devnum )) != 0)
        return -1;  // (error message already issued)

    /* Find the device block */
    if (!(dev = find_device_by_devnum( lcss, devnum )))
    {
        // "%1d:%04X %s does not exist"
        WRMSG( HHC01464, "E", lcss, devnum, "device" );
        return -1;
    }

    /* PROGRAMMING NOTE: intlock needs to be held in order to generate
       a channel report, but since dev->lock is also obtained as part
       of normal detach processing, we must obtain intlock FIRST to
       prevent a deadlock with channel.c 'execute_ccw_chain'. (The two
       locks MUST be obtained in the same sequence: intlock FIRST and
       then dev->lock afterwards.)
    */
    /* Determine if a channel report will need to be generated */
#if defined( _FEATURE_CHANNEL_SUBSYSTEM )
#if defined( _370 )
    if (sysblk.arch_mode != ARCH_370_IDX)
#endif
        intlock_needed = true;
#endif

    if (intlock_needed)
        OBTAIN_INTLOCK( NULL );

    /* Don't allow detaching devices which are still busy doing I/O.
       For grouped devices this includes *ANY* device in the group
       since all devices in the group will also be detached.
    */
    if (!force)
    {
        OBTAIN_DEVLOCK( dev );
        {
            /* Check if specified device is busy */
            if (dev->busy)
            {
                RELEASE_DEVLOCK( dev );
                if (intlock_needed)
                    RELEASE_INTLOCK( NULL );
                // "%1d:%04X busy or interrupt pending"
                WRMSG( HHC02231, "E", lcss, dev->devnum );
                return -1;
            }
            /* Check if any device in the group is busy */
            if (dev->group)
            {
                DEVGRP*  group    = dev->group;
                DEVBLK*  memdev;
                int  i;

                for (i=0; i < group->acount; i++)
                {
                    if ((memdev = group->memdev[i])->busy)
                    {
                        RELEASE_DEVLOCK( dev );
                        if (intlock_needed)
                            RELEASE_INTLOCK( NULL );
                        // "%1d:%04X busy or interrupt pending"
                        WRMSG( HHC02231, "E", lcss, memdev->devnum );
                        return -1;
                    }
                }
            }
        }
        RELEASE_DEVLOCK( dev );
    }

    /* Note: 'detach_device' will call 'detach_devblk' which obtains
       dev->lock and then issues the channel report if it's needed
       (which requires that intlock be held).
    */
    rc = detach_device( lcss, devnum );

    if (intlock_needed)
        RELEASE_INTLOCK( NULL );

    return rc;
}

/*-------------------------------------------------------------------*/
/* define command - rename a device                                  */
/*-------------------------------------------------------------------*/
int define_cmd(int argc, char *argv[], char *cmdline)
{
U16  devnum, newdevn;
U16 lcss,newlcss;
int rc;

    UNREFERENCED(cmdline);

    if (argc < 3)
    {
        WRMSG(HHC02202, "E", argv[0] );
        return -1;
    }

    rc=parse_single_devnum(argv[1],&lcss,&devnum);
    if ( rc < 0 )
    {
        return -1;
    }
    rc=parse_single_devnum(argv[2],&newlcss,&newdevn);
    if (rc<0)
    {
        return -1;
    }
    if ( lcss != newlcss )
    {
        WRMSG(HHC02238, "E");
        return -1;
    }

    rc = define_device (lcss, devnum, newdevn);

    return rc;
}

/*-------------------------------------------------------------------*/
/* pgmtrace command - trace program interrupts                       */
/*-------------------------------------------------------------------*/
int pgmtrace_cmd( int argc, char* argv[], char* cmdline )
{
    U64   rupt_mask;
    U64   prev_sysblk_pgminttr;
    int   abs_rupt_num;
    int       rupt_num;
    BYTE  c;

    UNREFERENCED( cmdline );

    /* No arguments = query current settings */
    if (argc < 2)
    {
             if (OS_NULL  == sysblk.pgminttr) WRMSG( HHC02281, "I", "pgmtrace == all"  );
        else if (OS_QUIET == sysblk.pgminttr) WRMSG( HHC02281, "I", "pgmtrace == none" );
        else
        {
            char flags[64+1]; int i;

            for (i=0; i < 64; i++)
                flags[i] = (sysblk.pgminttr & (1ULL << i)) ? ' ' : '*';

            flags[64] = 0;

            WRMSG( HHC02281, "I", "* = Tracing suppressed; otherwise tracing enabled" );
            WRMSG( HHC02281, "I", "0000000000000001111111111111111222222222222222233333333333333334" );
            WRMSG( HHC02281, "I", "123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0" );
            WRMSG( HHC02281, "I", flags );
        }
        return 0;
    }
    /* More than one argument = ERROR */
    else if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /* Otherwise argument is interrupt number to be added or removed */
    if (sscanf( argv[1], "%x%c", &rupt_num, &c ) != 1)
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], ": program interrupt number is invalid" );
        return -1;
    }

    if (0
        || (abs_rupt_num = abs( rupt_num )) < 1
        || (abs_rupt_num > 0x40)
    )
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], ": program interrupt number is out of range" );
        return -1;
    }

    /* Save current sysblk.pgminttr setting */
    prev_sysblk_pgminttr = sysblk.pgminttr;

    /* Build mask for interruption code to be turned on or off */
    rupt_mask = (1ULL << (abs_rupt_num - 1));

    /* Add or remove interruption code from program interrupt mask */
    if (rupt_num < 0)
        sysblk.pgminttr &= ~rupt_mask;
    else
        sysblk.pgminttr |=  rupt_mask;

    /* Clear the ostailor settings flag if it's now inaccurate */
    if (sysblk.pgminttr != prev_sysblk_pgminttr)
        sysblk.ostailor = 0;

    return 0;
}

/*-------------------------------------------------------------------*/
/* ostailor command - trace program interrupts                       */
/*-------------------------------------------------------------------*/
int ostailor_cmd( int argc, char* argv[], char* cmdline )
{
    char*  ostailor  = NULL;        /* (work variable)              */
    U64    mask      = 0;           /* OS_xxx... pgminttr bits      */
    U32    ost       = 0;           /* OSTAILOR_xxx... setting flag */
    bool   b_on      = false;       /* "+ostailor" specified        */
    bool   b_off     = false;       /* "-ostailor" specified        */
    bool   nolrasoe  = false;       /* true == No trace LRA Special */
                                    /*         Operation Exceptions */
    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    /* Error if more than one argument */
    if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /* If no arguments, display the current setting */
    if (argc < 2)
    {
        char   msgbuf [64] = {0};
        char   msgbuf2[64] = {0};

        if (sysblk.pgminttr == OS_VM          ) ostailor = "VM";

        if (sysblk.pgminttr == OS_DEFAULT     ) ostailor = "DEFAULT";
        if (sysblk.pgminttr == OS_QUIET       ) ostailor = "QUIET";
        if (sysblk.pgminttr == OS_NULL        ) ostailor = "NULL";

        if (sysblk.pgminttr == OS_OS390       ) ostailor = "OS/390";
        if (sysblk.pgminttr == OS_ZOS         ) ostailor = "z/OS";
        if (sysblk.pgminttr == OS_LINUX       ) ostailor = "LINUX";
        if (sysblk.pgminttr == OS_OPENSOLARIS ) ostailor = "OpenSolaris";

        if (sysblk.pgminttr == OS_VSE
                          && !sysblk.nolrasoe ) ostailor = "VSE";
        if (sysblk.pgminttr == OS_VSE
                          &&  sysblk.nolrasoe ) ostailor = "z/VSE";

        /* If no exact OSTAILOR match, check if maybe sysblk.ostailor
           identifies a verifiable previous "+OSTAILOR" setting. */
        if (!ostailor)
        {
            /* Is existing OSTAILOR value still valid? */
            if (sysblk.ostailor)
            {
                /* Yes, verify that it's still accurate */

                U64 test_pgminttr = OS_NULL; // (0xFFFFFFFFFFFFFFFFULL)

                if (sysblk.ostailor & OSTAILOR_VM         ) test_pgminttr &= OS_VM;

                if (sysblk.ostailor & OSTAILOR_DEFAULT    ) test_pgminttr &= OS_DEFAULT;
                if (sysblk.ostailor & OSTAILOR_QUIET      ) test_pgminttr &= OS_QUIET;
                if (sysblk.ostailor & OSTAILOR_NULL       ) test_pgminttr &= OS_NULL;

                if (sysblk.ostailor & OSTAILOR_OS390      ) test_pgminttr &= OS_OS390;
                if (sysblk.ostailor & OSTAILOR_ZOS        ) test_pgminttr &= OS_ZOS;
                if (sysblk.ostailor & OSTAILOR_LINUX      ) test_pgminttr &= OS_LINUX;
                if (sysblk.ostailor & OSTAILOR_OPENSOLARIS) test_pgminttr &= OS_OPENSOLARIS;

                if (sysblk.ostailor & OSTAILOR_VSE        ) test_pgminttr &= OS_VSE;

                /* Is sysblk.ostailor still accurate? */
                if (sysblk.pgminttr == test_pgminttr)
                {
                    /* Yep! Format current "xxx+yyy" OSTAILOR setting */

                    if (sysblk.ostailor & OSTAILOR_VM         ) STRLCAT( msgbuf2, "VM"          "+" );

                    if (sysblk.ostailor & OSTAILOR_DEFAULT    ) STRLCAT( msgbuf2, "DEFAULT"     "+" );
                    if (sysblk.ostailor & OSTAILOR_QUIET      ) STRLCAT( msgbuf2, "QUIET"       "+" );
                    if (sysblk.ostailor & OSTAILOR_NULL       ) STRLCAT( msgbuf2, "NULL"        "+" );

                    if (sysblk.ostailor & OSTAILOR_OS390      ) STRLCAT( msgbuf2, "OS390"       "+" );
                    if (sysblk.ostailor & OSTAILOR_ZOS        ) STRLCAT( msgbuf2, "ZOS"         "+" );
                    if (sysblk.ostailor & OSTAILOR_LINUX      ) STRLCAT( msgbuf2, "LINUX"       "+" );
                    if (sysblk.ostailor & OSTAILOR_OPENSOLARIS) STRLCAT( msgbuf2, "OPENSOLARIS" "+" );

                    if (sysblk.ostailor & OSTAILOR_VSE        )
                    {
                        char* vse = !sysblk.nolrasoe ? "VSE+" : "z/VSE+";
                        STRLCAT( msgbuf2, vse );
                    }

                    /* (remove trailing "+") */
                    ostailor = rtrim( msgbuf2, "+" );
                }
            }
        }

        if (!ostailor)
            MSGBUF( msgbuf, "Custom(0x%16.16"PRIX64")", sysblk.pgminttr );
        else
            MSGBUF( msgbuf, "%s", ostailor );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], msgbuf );
        return 0;
    }

    /* Otherwise the single argument specifies the ostailor setting */

    ostailor = argv[1];

    if (ostailor[0] == '+')
    {
        b_on  = true;
        b_off = false;
        ostailor++;
    }
    else if (ostailor[0] == '-')
    {
        b_on  = false;
        b_off = true;
        ostailor++;
    }
    else
    {
        b_on  = false;
        b_off = false;
    }

    nolrasoe = sysblk.nolrasoe;

         if (CMD( ostailor, VM,      2 ))     { ost = OSTAILOR_VM;          mask = OS_VM;          }
    else if (CMD( ostailor, Z/VM,    4 ))     { ost = OSTAILOR_VM;          mask = OS_VM;          }

    else if (CMD( ostailor, NONE,    4 ))     { ost = OSTAILOR_DEFAULT;     mask = OS_DEFAULT; b_on = false; b_off = false; }
    else if (CMD( ostailor, DEFAULT, 3 ))     { ost = OSTAILOR_DEFAULT;     mask = OS_DEFAULT; b_on = false; b_off = false; }
    else if (CMD( ostailor, QUIET,   5 ))     { ost = OSTAILOR_QUIET;       mask = OS_QUIET;   b_on = false; b_off = false; }
    else if (CMD( ostailor, NULL,    4 ))     { ost = OSTAILOR_NULL;        mask = OS_NULL;    b_on = false; b_off = false; }

    else if (CMD( ostailor, Z/OS,    4 ))     { ost = OSTAILOR_ZOS;         mask = OS_ZOS;         }
    else if (CMD( ostailor, OpenSolaris, 4 )) { ost = OSTAILOR_OPENSOLARIS; mask = OS_OPENSOLARIS; }
    else if (CMD( ostailor, LINUX,   2 ))     { ost = OSTAILOR_LINUX;       mask = OS_LINUX;       }
    else if (CMD( ostailor, OS/390,  2 ))     { ost = OSTAILOR_OS390;       mask = OS_OS390;       }

    else if (CMD( ostailor, VSE,     2 ))     { ost = OSTAILOR_VSE;         mask = OS_VSE;     nolrasoe = false; }
    else if (CMD( ostailor, Z/VSE,   4 ))     { ost = OSTAILOR_VSE;         mask = OS_VSE;     nolrasoe = true;  }
    else
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], ": unknown OS tailor specification" );
        return -1;
    }

    if (b_on)
    {
        sysblk.ostailor |= ost;
        sysblk.pgminttr &= mask;
    }
    else if (b_off)
    {
        sysblk.ostailor &= ~ost;
        sysblk.pgminttr |= ~mask;
    }
    else
    {
        sysblk.ostailor = ost;
        sysblk.pgminttr = mask;
    }

    sysblk.nolrasoe = nolrasoe;

    return 0;
}

/*-------------------------------------------------------------------*/
/* k command - print out cckd internal trace                         */
/*-------------------------------------------------------------------*/
int k_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    if (argc > 1)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    cckd_print_itrace();

    return 0;
}

/*-------------------------------------------------------------------*/
/* ds - display subchannel                                           */
/*-------------------------------------------------------------------*/
int ds_cmd(int argc, char *argv[], char *cmdline)
{
DEVBLK*  dev;
U16      devnum;
U16      lcss;
int rc;
char buf[4096];

    UNREFERENCED(cmdline);

    if (argc < 2)
    {
        missing_devnum();
        return -1;
    }
    else if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    rc=parse_single_devnum(argv[1],&lcss,&devnum);

    if (rc<0)
    {
        return -1;
    }

    if (!(dev = find_device_by_devnum (lcss,devnum)))
    {
        // HHC02200 "%1d:%04X device not found"
        devnotfound_msg(lcss,devnum);
        return -1;
    }

    display_subchannel (dev, buf, sizeof(buf), "HHC02268I ");
    LOGMSG( "%s", buf );

    return 0;
}

void *device_thread(void *arg);
/*-------------------------------------------------------------------*/
/* devtmax command - display or set max device threads               */
/*-------------------------------------------------------------------*/
int devtmax_cmd(int argc, char *argv[], char *cmdline)
{
    int devtmax = -2;

    TID tid;

    UNREFERENCED(cmdline);
    if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }
    else if ( argc == 2 )
    {
        sscanf(argv[1], "%d", &devtmax);

        if (devtmax >= -1)
            sysblk.devtmax = devtmax;
        else
        {
            WRMSG(HHC02205, "E", argv[1], ": must be -1 to n");
            return -1;
        }

        /* Create a new device thread if the I/O queue is not NULL
           and more threads can be created */

        /* the IOQ lock is obtained in order to write to sysblk.devtwait */
        OBTAIN_IOQLOCK();
        {
            if (sysblk.ioq && (!sysblk.devtmax || sysblk.devtnbr < sysblk.devtmax))
            {
                int rc;

                rc = create_thread( &tid, DETACHED, device_thread, NULL, "idle device thread" );
                if (rc)
                    WRMSG(HHC00102, "E", strerror(rc));
            }

            /* Wakeup threads in case they need to terminate */
            sysblk.devtwait = 0;
            broadcast_condition( &sysblk.ioqcond );
        }
        RELEASE_IOQLOCK();
    }
    else
        WRMSG(HHC02242, "I",
            sysblk.devtmax, sysblk.devtnbr, sysblk.devthwm,
            sysblk.devtwait, sysblk.devtunavail );

    return 0;
}

/*-------------------------------------------------------------------*/
/* sf commands - shadow file add/remove/set/compress/display         */
/*-------------------------------------------------------------------*/
int sf_cmd( int argc, char* argv[], char* cmdline )
{
char     action;                        /* Action character `+-cd'   */
char*    devascii;                      /* -> Device name            */
DEVBLK*  dev;                           /* -> Device block           */
U16      devnum;                        /* Device number             */
U16      lcss;                          /* Logical CSS               */
int      flag = 1;                      /* sf- flag (default merge)  */
int      level = 2;                     /* sfk level (default 2)     */
char     c;                             /* work for sscan            */
int      rc;

    UNREFERENCED( cmdline );

    if (strlen( argv[0] ) < 3 || !strchr( "+-cdk", argv[0][2] ))
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[0],
            ": must be 'sf+', 'sf-', 'sfc', 'sfk' or 'sfd'" );
        return -1;
    }

    action = argv[0][2];
    /*
     * device name either follows the action character or is the
     * next operand
     */
    if (strlen( argv[0] ) > 3)
        devascii = argv[0] + 3;
    else
    {
        argv++;  argc--;

        if (argc <= 0 || !(devascii = argv[0]))
        {
            // HHC02201 "Device number missing"
            missing_devnum();
            return -1;
        }
    }

    /* device name can be '*' meaning all cckd devices */
    if (strcmp( devascii, "*" ) == 0)
    {
        /* Verify that at least one cckd device exists in the configuration */
        for (dev = sysblk.firstdev; dev && !dev->cckd_ext; dev = dev->nextdev);
            /* nothing */

        if (!dev)       // (did we find any cckd devices?)
        {
            // "Empty list"
            WRMSG( HHC02216, "E" );
            return -1;
        }
        dev = NULL;
    }
    else
    {
        if (parse_single_devnum( devascii, &lcss, &devnum ) < 0)
            return -1;

        if (!(dev = find_device_by_devnum ( lcss, devnum )))
        {
            // HHC02200 "%1d:%04X device not found"
            rc = devnotfound_msg( lcss, devnum );
            return rc;
        }

        if (!dev->cckd_ext)
        {
            // "%1d:%04X device is not a %s"
            WRMSG( HHC02209, "E", lcss, devnum, "cckd device" );
            return -1;
        }
    }

    /* For 'sf-' the operand can be 'nomerge', 'merge' or 'force' */
    if (action == '-' && argc > 1)
    {
             if (CMD( argv[1], NOMERGE, 5 )) flag = 0;
        else if (CMD( argv[1], MERGE,   3 )) flag = 1;
        else if (CMD( argv[1], FORCE,   5 )) flag = 2;
        else
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1],
                ": operand must be 'merge', 'nomerge' or 'force'" );
            return -1;
        }

        argv++;  argc--;
    }

    /* For 'sfk' the operand is an integer -1 .. 4 */
    if (action == 'k' && argc > 1)
    {
        if (0
            || sscanf( argv[1], "%d%c", &level, &c ) != 1
            || level < -1
            || level >  4
        )
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1],
                ": operand must be a number -1 .. 4" );
            return -1;
        }

        argv++;  argc--;
    }

    /* No other operands allowed */
    if (argc > 1)
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], "" );
        return -1;
    }

    /* Set sf- flags in either cckdblk or the cckd extension */
    if (action == '-')
    {
        if (dev)
        {
            CCKD_EXT*  cckd   = dev->cckd_ext;

            cckd->sfmerge = (flag == 1);
            cckd->sfforce = (flag == 2);
        }
        else
        {
            cckdblk.sfmerge = (flag == 1);
            cckdblk.sfforce = (flag == 2);
        }
    }
    /* Set sfk level in either cckdblk or the cckd extension */
    else if (action == 'k')
    {
        if (dev)
        {
            CCKD_EXT*  cckd   = dev->cckd_ext;

            cckd->sflevel = level;
        }
        else
            cckdblk.sflevel = level;
    }

    /* Reject the command if the guest has been IPLed */
    if (action != 'd')
    {
        /* Unless test mode mode is active! Test scripts MUST be
           allowed to e.g. discard shadow files after their tests
           have completed to prevent them from failing the next
           time the test is run due to the state of the test dasd
           having been changed by the previous run! But if we're
           NOT running in test mode (i.e. if this is normal user
           execution), then don't allow them since doing to could
           cause damage to their guest's running state.
        */
        if (!sysblk.scrtest)    // (normal user non-test mode?)
        {
            if (sysblk.ipled)
            {
                // "Command cannot be issued once system has been IPLed"
                // "Hercules needs to be restarted before proceeding"
                WRMSG( HHC00829, "E" );
                WRMSG( HHC00831, "W" );
                return -1;
            }

            sysblk.sfcmd = TRUE;
        }
    }

    /* Process the command */
    switch (action)
    {
#if defined( OPTION_NOASYNC_SF_CMDS )
        case '+': cckd_sf_add   ( dev ); break;
        case '-': cckd_sf_remove( dev ); break;
        case 'c': cckd_sf_comp  ( dev ); break;
        case 'd': cckd_sf_stats ( dev ); break;
        case 'k': cckd_sf_chk   ( dev ); break;
#else
        TID tid;
        case '+': if (create_thread( &tid, DETACHED, cckd_sf_add,    dev, "sf+ command" )) cckd_sf_add   ( dev ); break;
        case '-': if (create_thread( &tid, DETACHED, cckd_sf_remove, dev, "sf- command" )) cckd_sf_remove( dev ); break;
        case 'c': if (create_thread( &tid, DETACHED, cckd_sf_comp,   dev, "sfc command" )) cckd_sf_comp  ( dev ); break;
        case 'd': if (create_thread( &tid, DETACHED, cckd_sf_stats,  dev, "sfd command" )) cckd_sf_stats ( dev ); break;
        case 'k': if (create_thread( &tid, DETACHED, cckd_sf_chk,    dev, "sfk command" )) cckd_sf_chk   ( dev ); break;
#endif
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* mounted_tape_reinit statement                                     */
/*-------------------------------------------------------------------*/
int mounted_tape_reinit_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if ( argc == 2 )
    {
        if ( CMD(argv[1],disallow,4) || CMD(argv[1],disable,4) )
            sysblk.nomountedtapereinit = TRUE;
        else if ( CMD(argv[1],allow,3) || CMD(argv[1],enable,3) )
            sysblk.nomountedtapereinit = FALSE;
        else
        {
            WRMSG(HHC02205, "E", argv[1], "");
            return -1;
        }
        if ( MLVL(VERBOSE) )
            WRMSG(HHC02204, "I", argv[0],
                  sysblk.nomountedtapereinit?"disabled":"enabled");
    }
    else
        WRMSG(HHC02203, "I", argv[0],
              sysblk.nomountedtapereinit?"disabled":"enabled");

    return 0;
}

/*-------------------------------------------------------------------*/
/* mt command - magnetic tape commands                              */
/*-------------------------------------------------------------------*/
int mt_cmd(int argc, char *argv[], char *cmdline)
{
DEVBLK*  dev;
U16      devnum;
U16      lcss;
int      rc, msg = TRUE;
int      count = 1;
char*    devclass;
BYTE     unitstat, code = 0;


    UNREFERENCED(cmdline);

    if (argc < 3)
    {
        WRMSG(HHC02202,"E", argv[0] );
        return -1;
    }

    if (argc > 4)
    {
        WRMSG(HHC02299,"E", argv[0]);
        return -1;
    }
    if ( !( ( CMD(argv[2],rew,3) ) ||
            ( CMD(argv[2],fsf,3) ) ||
            ( CMD(argv[2],bsf,3) ) ||
            ( CMD(argv[2],fsr,3) ) ||
            ( CMD(argv[2],bsr,3) ) ||
            ( CMD(argv[2],asf,3) ) ||
            ( CMD(argv[2],wtm,3) ) ||
            ( CMD(argv[2],dse,3) ) ||
            ( CMD(argv[2],dvol1,4) )
          )
       )
    {
        WRMSG( HHC02205, "E", argv[2], ". Type 'help mt' for assistance.");
        return -1;
    }

    if ( argc == 4  && !( CMD(argv[2],rew,3) ) )
    {
        for (rc = 0; rc < (int)strlen(argv[3]); rc++)
        {
            if ( !isdigit((unsigned char)argv[3][rc]) )
            {
                WRMSG( HHC02205, "E", argv[3], "; not in range of 1-9999");
                return -1;
            }
        }
        sscanf(argv[3],"%d", &count);
        if ( count < 1 || count > 9999 )
        {
            WRMSG( HHC02205, "E", argv[3], "; not in range of 1-9999");
            return -1;
        }
    }

    rc = parse_single_devnum( argv[1], &lcss, &devnum );

    if ( rc < 0)
    {
        return -1;
    }


    if (!(dev = find_device_by_devnum (lcss, devnum)))
    {
        // HHC02200 "%1d:%04X device not found"
        devnotfound_msg(lcss,devnum);
        return -1;
    }

    /* Obtain the device lock */
    OBTAIN_DEVLOCK( dev );

    /* Reject if device is busy or interrupt pending */
    if ( dev->busy || IOPENDING(dev) || (dev->scsw.flag3 & SCSW3_SC_PEND))
    {
        if (!sysblk.sys_reset)      // is the system in a reset status?
        {
            RELEASE_DEVLOCK( dev );
            WRMSG(HHC02231, "E", lcss, devnum );
            return -1;
        }
    }

    ASSERT( dev->hnd && dev->hnd->query );
    dev->hnd->query( dev, &devclass, 0, NULL );

    if ( strcmp(devclass,"TAPE") != 0 )
    {
        RELEASE_DEVLOCK( dev );
        WRMSG(HHC02209, "E", lcss, devnum, "TAPE" );
        return -1;

    }

    ASSERT( dev->tmh && dev->tmh->tapeloaded );
    if ( !dev->tmh->tapeloaded( dev, NULL, 0 ) )
    {
        RELEASE_DEVLOCK( dev );
        WRMSG(HHC02298, "E", LCSS_DEVNUM);
        return -1;
    }

    if ( CMD(argv[2],rew,3) )
    {
        if (argc > 3)
        {
            WRMSG(HHC02299,"E", argv[0]);
            msg = FALSE;
        }
        else
        {
            rc = dev->tmh->rewind( dev, &unitstat, code);

            if ( rc == 0 )
            {
                dev->eotwarning = 0;
            }
        }
    }
    else if ( CMD(argv[2],fsf,3) )
    {
        for ( ; count >= 1; count-- )
        {
            rc = dev->tmh->fsf( dev, &unitstat, code);

            if ( rc < 0 )
            {
                break;
            }
        }
    }
    else if ( CMD(argv[2],bsf,3) )
    {
        for ( ; count >= 1; count-- )
        {
            rc = dev->tmh->bsf( dev, &unitstat, code);

            if ( rc < 0 )
            {
                break;
            }
        }
    }
    else if ( CMD(argv[2],fsr,3) )
    {
        for ( ; count >= 1; count-- )
        {
            rc = dev->tmh->fsb( dev, &unitstat, code);

            if ( rc <= 0 )
            {
                break;
            }
        }
    }
    else if ( CMD(argv[2],bsr,3) )
    {
        for ( ; count >= 1; count-- )
        {
            rc = dev->tmh->bsb( dev, &unitstat, code);

            if ( rc < 0 )
            {
                break;
            }
        }
    }
    else if ( CMD(argv[2],asf,3) )
    {
        rc = dev->tmh->rewind( dev, &unitstat, code);
        if ( rc == 0 )
        {
            for ( ; count > 1; count-- )
            {
                rc = dev->tmh->fsf( dev, &unitstat, code);

                if ( rc < 0 )
                {
                    break;
                }
            }
        }
    }
    else if ( CMD(argv[2],dvol1,4) )
    {
        if (argc > 3)
        {
            WRMSG(HHC02299,"E", argv[0]);
            msg = FALSE;
        }
        else
        {
            if ( dev->blockid == 0 )
            {
                BYTE *sLABEL = malloc( MAX_TAPE_BLKSIZE );

                if (!sLABEL)
                {
                    WRMSG( HHC02801, "E", LCSS_DEVNUM, argv[2], "Out of memory" );
                    msg = FALSE;
                }
                else
                {
                    rc = dev->tmh->read( dev, sLABEL, &unitstat, code );

                    if ( rc == 80 )
                    {
                        int a = TRUE;
                        if ( strncmp( (char *)sLABEL, "VOL1", 4 ) != 0 )
                        {
                            str_guest_to_host( sLABEL, sLABEL, 51 );
                            a = FALSE;
                        }

                        if ( strncmp( (char *)sLABEL, "VOL1", 4 ) == 0 )
                        {
                            char msgbuf[64];
                            char volser[7];
                            char owner[15];

                            memset( msgbuf, 0, sizeof(msgbuf) );
                            memset( volser, 0, sizeof(volser) );
                            memset( owner,  0, sizeof(owner)  );

                            strncpy( volser, (char*)&sLABEL[04],  6 );
                            strncpy( owner,  (char*)&sLABEL[37], 14 );

                            MSGBUF( msgbuf, "%s%s%s%s%s",
                                            volser,
                                            strlen(owner) == 0? "":", Owner = \"",
                                            strlen(owner) == 0? "": owner,
                                            strlen(owner) == 0? "": "\"",
                                            a ? " (ASCII LABELED) ": "" );

                            WRMSG( HHC02805, "I", LCSS_DEVNUM, msgbuf );
                            msg = FALSE;
                        }
                        else
                            WRMSG( HHC02806, "I", LCSS_DEVNUM );
                    }
                    else
                    {
                        WRMSG( HHC02806, "I", LCSS_DEVNUM );
                    }

                    rc = dev->tmh->rewind( dev, &unitstat, code);

                    free( sLABEL );
                }
            }
            else
            {
                WRMSG( HHC02801, "E", LCSS_DEVNUM, argv[2], "Tape not at BOT" );
                msg = FALSE;
            }
        }
    }
    else if ( CMD(argv[2],wtm,3) )
    {
        if ( dev->readonly || dev->tdparms.logical_readonly )
        {
            WRMSG( HHC02804, "E", LCSS_DEVNUM );
            msg = FALSE;
        }
        else
        {
            for ( ; count >= 1; count-- )
            {
                rc = dev->tmh->wtm( dev, &unitstat, code);

                if ( rc >= 0 )
                {
                    dev->curfilen++;
                }
                else
                {
                    break;
                }
            }

            dev->tmh->sync( dev, &unitstat, code );
        }
    }
    else if ( CMD(argv[2],dse,3) )
    {
        if (argc > 3)
        {
            WRMSG(HHC02299,"E", argv[0]);
            msg = FALSE;
        }
        else
        {
            if ( dev->readonly || dev->tdparms.logical_readonly )
            {
                WRMSG( HHC02804, "E", LCSS_DEVNUM );
                msg = FALSE;
            }
            else
            {
                rc = dev->tmh->dse( dev, &unitstat, code);

                dev->tmh->sync( dev, &unitstat, code );
            }
        }
    }

    if ( msg )
    {
        if ( rc >= 0 )
        {
            WRMSG( HHC02800, "I", LCSS_DEVNUM, argv[2] );
        }
        else
        {
            char msgbuf[32];
            MSGBUF( msgbuf, "rc = %d", rc );
            WRMSG( HHC02801, "E", LCSS_DEVNUM, argv[2], msgbuf );
        }
    }

    RELEASE_DEVLOCK( dev );

    WRMSG( HHC02802, "I", LCSS_DEVNUM, dev->curfilen );
    WRMSG( HHC02803, "I", LCSS_DEVNUM, dev->blockid  );

    return 0;
}

/*-------------------------------------------------------------------*/
/* devinit command - assign/open a file for a configured device      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int devinit_cmd(int argc, char *argv[], char *cmdline)
{
DEVBLK*  dev;
U16      devnum;
U16      lcss;
int      i, rc;
int      init_argc;
char   **init_argv;
char   **save_argv = NULL;

    UNREFERENCED(cmdline);

    if (argc < 2)
    {
        WRMSG(HHC02202,"E", argv[0] );
        return -1;
    }

    rc=parse_single_devnum(argv[1],&lcss,&devnum);

    if (rc<0)
    {
        return -1;
    }

    if (!(dev = find_device_by_devnum (lcss, devnum)))
    {
        // HHC02200 "%1d:%04X device not found"
        devnotfound_msg(lcss,devnum);
        return -1;
    }

    /* Obtain the device lock */
    OBTAIN_DEVLOCK( dev );

    /* wait up to 0.1 seconds for the busy to go away */
    {
        int i = 20;
        while(i-- > 0 && (dev->busy
                         || IOPENDING(dev)
                         || (dev->scsw.flag3 & SCSW3_SC_PEND)))
        {
            RELEASE_DEVLOCK( dev );
            USLEEP(5000);
            OBTAIN_DEVLOCK( dev );
        }
    }

    /* Reject if device is busy or interrupt pending */
    if ((dev->busy || IOPENDING(dev)
     || (dev->scsw.flag3 & SCSW3_SC_PEND))
      && !sysblk.sys_reset)
    {
        RELEASE_DEVLOCK( dev );
        WRMSG(HHC02231, "E", lcss, devnum );
        return -1;
    }

    /* Build the device initialization arguments array */
    if (argc > 2)
    {
        /* Use the specified new arguments */
        init_argc = argc-2;
        init_argv = &argv[2];
    }
    else
    {
        /* Use the same arguments as originally used */
        init_argc = dev->argc;
        if (init_argc)
        {
            init_argv = malloc ( init_argc * sizeof(char*) );
            save_argv = malloc ( init_argc * sizeof(char*) );
            for (i = 0; i < init_argc; i++)
            {
                if (dev->argv[i])
                    init_argv[i] = strdup(dev->argv[i]);
                else
                    init_argv[i] = NULL;
                save_argv[i] = init_argv[i];
            }
        }
        else
            init_argv = NULL;
    }

    /* Save arguments for next time */
    for (i = 0; i < dev->argc; i++)
        if (dev->argv[i])
            free(dev->argv[i]);
    if (dev->argv)
        free(dev->argv);
    dev->argc = init_argc;
    if (init_argc > 0)
    {
        dev->argv = malloc ( init_argc * sizeof(char*) );
        for (i = 0; i < init_argc; i++)
            if (init_argv[i])
                dev->argv[i] = strdup(init_argv[i]);
            else
                dev->argv[i] = NULL;
    }
    else
        dev->argv = NULL;

    /* Call the device init routine to do the hard work */
    dev->reinit = 1;
    if ((rc = (dev->hnd->init)(dev, init_argc, init_argv)) < 0)
    {
        // "%1d:%04X device initialization failed"
        WRMSG(HHC02244,"E",lcss, devnum );
    } else {
        // "%1d:%04X device initialized"
        WRMSG(HHC02245, "I", lcss, devnum );
    }
    dev->reinit = 0;

    /* Release the device lock */
    RELEASE_DEVLOCK( dev );

    /* Free work memory */
    if (save_argv)
    {
        for (i=0; i < init_argc; i++)
            if (save_argv[i])
                free(save_argv[i]);
        free(save_argv);
        free(init_argv);
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* savecore filename command - save a core image to file             */
/*-------------------------------------------------------------------*/
int savecore_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

    char   *fname;                      /* -> File name (ASCIIZ)     */
    char   *loadaddr;                   /* loadcore memory address   */
    U64     work64;                     /* 64-bit work variable      */
    RADR    aaddr;                      /* Absolute storage address  */
    RADR    aaddr2;                     /* Absolute storage address  */
    int     fd;                         /* File descriptor           */
    U32     chunk;                      /* Bytes to write this time  */
    U32     written;                    /* Bytes written this time   */
    U64     total;                      /* Total bytes to be written */
    U64     saved;                      /* Total bytes saved so far  */
    BYTE    c;                          /* (dummy sscanf work area)  */
    char    pathname[MAX_PATH];         /* fname in host path format */
    time_t  begtime, curtime;           /* progress messages times   */
    char    fmt_mem[8];                 /* #of M/G/etc. saved so far */

    UNREFERENCED(cmdline);

    if (argc < 2)
    {
        // "Missing argument(s). Type 'help %s' for assistance."
        WRMSG(HHC02202,"E", argv[0] );
        return -1;
    }

    fname = argv[1];

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        // "Processor %s%02X: processor is not %s"
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }

    regs = sysblk.regs[sysblk.pcpu];

    if (argc < 3 || '*' == *(loadaddr = argv[2]))
    {
        /* Locate the first modified (changed) page */
        for
        (
            aaddr = 0;
            aaddr < sysblk.mainsize
                && !(ARCH_DEP( get_4K_storage_key )( aaddr ) & STORKEY_CHANGE);
            aaddr += STORAGE_KEY_4K_PAGESIZE
        )
        {
            ;   /* (nop) */
        }

        if (aaddr >= sysblk.mainsize)
            aaddr = 0;
        else
            aaddr &= ~0xFFF;
    }
    else if (sscanf(loadaddr, "%"SCNx64"%c", &work64, &c) !=1
        || work64 >= (U64) sysblk.mainsize )
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        // "Invalid argument %s%s"
        WRMSG(HHC02205, "E", loadaddr, ": invalid starting address" );
        return -1;
    }
    else
        aaddr = (RADR) work64;

    if (argc < 4 || '*' == *(loadaddr = argv[3]))
    {
        /* Locate the last modified (changed) page */
        for
        (
            aaddr2 = sysblk.mainsize - STORAGE_KEY_4K_PAGESIZE;
            aaddr2 > 0
              && !(ARCH_DEP( get_4K_storage_key )( aaddr2 ) & STORKEY_CHANGE);
            aaddr2 -= STORAGE_KEY_4K_PAGESIZE)
        {
            ;   /* (nop) */
        }

        if (ARCH_DEP( get_4K_storage_key )( aaddr2 ) & STORKEY_CHANGE)
            aaddr2 |= 0xFFF;
        else
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            // "Savecore: no modified storage found"
            WRMSG(HHC02246, "E");
            return -1;
        }
    }
    else if (sscanf(loadaddr, "%"SCNx64"%c", &work64, &c) !=1
        || work64 >= (U64) sysblk.mainsize )
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        // "Invalid argument %s%s"
        WRMSG(HHC02205, "E", loadaddr, ": invalid ending address" );
        return -1;
    }
    else
        aaddr2 = (RADR) work64;

    if (CPUSTATE_STOPPED != regs->cpustate)
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        // "Operation rejected: CPU not stopped"
        WRMSG(HHC02247, "E");
        return -1;
    }

    if (aaddr > aaddr2)
    {
        char buf[40];
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        MSGBUF( buf, "%16.16"PRIX64"-%16.16"PRIX64, (U64) aaddr, (U64) aaddr2);
        // "Invalid argument %s%s"
        WRMSG(HHC02205, "W", buf, ": invalid range" );
        return -1;
    }

    // "Saving locations %016X-%016X to file %s"
    {
        char buf1[32];
        char buf2[32];
        MSGBUF( buf1, "%"PRIX64, (U64) aaddr );
        MSGBUF( buf2, "%"PRIX64, (U64) aaddr2 );
        WRMSG(HHC02248, "I", buf1, buf2, fname );
    }

    hostpath(pathname, fname, sizeof(pathname));

    if ((fd = HOPEN(pathname, O_CREAT|O_WRONLY|O_EXCL|O_BINARY, S_IREAD|S_IWRITE|S_IRGRP)) < 0)
    {
        int saved_errno = errno;
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        // "Error in function %s: %s"
        WRMSG(HHC02219, "E", "open()", strerror(saved_errno) );
        return -1;
    }

    /* Calculate total number of bytes to be written */
    total = ((U64)aaddr2 - (U64)aaddr) + 1;
    saved = 0;

    /* Save start time */
    time( &begtime );

    /* Write smaller more manageable chunks until all is written */
    do
    {
        chunk = (64 * ONE_MEGABYTE);

        if (chunk > total)
            chunk = total;

        written = write( fd, regs->mainstor + aaddr, chunk );

        if ((S32)written < 0)
        {
            // "Error in function %s: %s"
            WRMSG(HHC02219, "E", "write()", strerror(errno) );
            return -1;
        }

        if (written < chunk)
        {
            // "Error in function %s: %s"
            WRMSG(HHC02219, "E", "write()", "incomplete" );
            return -1;
        }

        aaddr += chunk;
        saved += chunk;

        /* Time for progress message? */
        time( &curtime );
        if (difftime( curtime, begtime ) > 2.0)
        {
            begtime = curtime;
            // "%s bytes %s so far..."
            WRMSG( HHC02317, "I",
                fmt_memsize( saved, fmt_mem, sizeof( fmt_mem )),
                    "saved" );
        }
    }
    while ((total -= chunk) > 0);

    close(fd);

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    // "Operation complete"
    WRMSG(HHC02249, "I");

    return 0;
}

/*-------------------------------------------------------------------*/
/* Helper macro to compare symbol name                               */
/*-------------------------------------------------------------------*/

#if defined( CASELESS_SYMBOLS )
  #define  SYMCMP   strcasecmp
#else
  #define  SYMCMP   strcmp
#endif

/*-------------------------------------------------------------------*/
/*         is_reserved_symbol  -  defsym delsym helper               */
/*-------------------------------------------------------------------*/
static BYTE is_reserved_symbol( const char* sym )
{
    // PROGRAMMING NOTE: please TRY to keep the below
    // in at least *approximate* alphabetical order!

    return (0
        || SYMCMP( sym, "ARCHMODE"    ) == 0
        || SYMCMP( sym, "BDATE"       ) == 0
        || SYMCMP( sym, "BTIME"       ) == 0
        || SYMCMP( sym, "CUU"         ) == 0
        || SYMCMP( sym, "CCUU"        ) == 0
        || SYMCMP( sym, "CSS"         ) == 0
        || SYMCMP( sym, "CPUMODEL"    ) == 0
        || SYMCMP( sym, "CPUID"       ) == 0
        || SYMCMP( sym, "CPUSERIAL"   ) == 0
        || SYMCMP( sym, "CPUVERID"    ) == 0
        || SYMCMP( sym, "DATE"        ) == 0
        || SYMCMP( sym, "DEVN"        ) == 0
        || SYMCMP( sym, "HOSTNAME"    ) == 0
        || SYMCMP( sym, "HOSTOS"      ) == 0
        || SYMCMP( sym, "HOSTOSREL"   ) == 0
        || SYMCMP( sym, "HOSTOSVER"   ) == 0
        || SYMCMP( sym, "HOSTARCH"    ) == 0
        || SYMCMP( sym, "HOSTNUMCPUS" ) == 0
        || SYMCMP( sym, "LPARNUM"     ) == 0
        || SYMCMP( sym, "LPARNAME"    ) == 0
        || SYMCMP( sym, "MODPATH"     ) == 0
        || SYMCMP( sym, "MODNAME"     ) == 0
        || SYMCMP( sym, "SYSLEVEL"    ) == 0
        || SYMCMP( sym, "SYSTYPE"     ) == 0
        || SYMCMP( sym, "SYSNAME"     ) == 0
        || SYMCMP( sym, "SYSPLEX"     ) == 0
        || SYMCMP( sym, "TIME"        ) == 0
        || SYMCMP( sym, "VERSION"     ) == 0
    );
}

/*-------------------------------------------------------------------*/
/* defsym command - define substitution symbol                       */
/*-------------------------------------------------------------------*/
int defsym_cmd( int argc, char* argv[], char* cmdline )
{
    char*  sym;
    char*  value;

    UNREFERENCED( cmdline );

    if (argc < 2)
    {
        list_all_symbols();
        return 0;
    }

    /* point to symbol name */
    sym = strdup( argv[1] );

    if (!sym)
    {
        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "strdup()", strerror( errno ));
        return -1;
    }

#if defined( CASELESS_SYMBOLS )
    strupper( sym, sym );
#endif

    if (is_reserved_symbol( sym ))
    {
        // "Symbol name %s is reserved"
        WRMSG( HHC02197, "E", sym );
        free( sym );
        return -1;
    }

    if (argc > 3)
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[2], ": DEFSYM requires a single value (use quotes if necessary)" );
        free( sym );
        return -1;
    }

    /* Point to symbol value if specified, otherwise set to blank */
    value = (argc > 2) ? argv[2] : "";

    /* Define the symbol */
    set_symbol( sym, value );
    free( sym );
    return 0;
}

/*-------------------------------------------------------------------*/
/* delsym command - delete substitution symbol                       */
/*-------------------------------------------------------------------*/
int delsym_cmd( int argc, char* argv[], char* cmdline )
{
    char* sym;

    UNREFERENCED( cmdline );

    if (argc != 2)
    {
        // "Missing argument(s). Type 'help %s' for assistance."
        WRMSG( HHC02202, "E", argv[0] );
        return -1;
    }

    /* point to symbol name */
    sym = strdup( argv[1] );

    if (!sym)
    {
        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "strdup()", strerror( errno ));
        return -1;
    }

#if defined( CASELESS_SYMBOLS )
    strupper( sym, sym );
#endif

    if (is_reserved_symbol( sym ))
    {
        // "Symbol name %s is reserved"
        WRMSG( HHC02197, "E", sym );
        free( sym );
        return -1;
    }

    /* Delete the symbol */
    del_symbol( sym );
    free( sym );
    return 0;
}

/*-------------------------------------------------------------------*/
/* f? command - display currently defined unusable storage range(s)  */
/*-------------------------------------------------------------------*/
int fquest_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( argc );
    UNREFERENCED( argv );
    UNREFERENCED( cmdline );

    switch (sysblk.arch_mode)
    {
#if defined(     _370 )
        case ARCH_370_IDX:
          return s370_fquest_cmd();
#endif
#if defined(     _390 )
        case ARCH_390_IDX:
          return s390_fquest_cmd();
#endif
#if defined(     _900 )
        case ARCH_900_IDX:
          return z900_fquest_cmd();
#endif
        default: CRASH();
    }
    UNREACHABLE_CODE( return -1 );
}

/*-------------------------------------------------------------------*/
/* f- and f+ commands - mark page frame as -unusable or +usable      */
/*-------------------------------------------------------------------*/
static int fonoff_cmd( REGS* regs, char* cmdline )
{
    switch (sysblk.arch_mode)
    {
#if defined(     _370 )
        case ARCH_370_IDX:
          return s370_fonoff_cmd( regs, cmdline );
#endif
#if defined(     _390 )
        case ARCH_390_IDX:
          return s390_fonoff_cmd( regs, cmdline );
#endif
#if defined(     _900 )
        case ARCH_900_IDX:
          return z900_fonoff_cmd( regs, cmdline );
#endif
        default: CRASH();
    }
    UNREACHABLE_CODE( return -1 );
}

/*-------------------------------------------------------------------*/
/* x+ and x- commands - turn switches on or off                      */
/*-------------------------------------------------------------------*/
int OnOffCommand( int argc, char* argv[], char* cmdline )
{
    char*   cmd = cmdline;              /* (just a shorter name)     */
    bool    plus_enable_on;             /* true == x+, false == x-   */
    char*   onoroff;                    /* x+ == "on", x- == "off"   */
    DEVBLK* dev;
    BYTE    ccwops[256];
    U16     devnum;
    U16     lcss;
    REGS*   regs;
    size_t  cmdlen = strlen( cmdline );

    if (cmd[1] == '+')
    {
        plus_enable_on = true;
        onoroff = "ON";
    }
    else // (cmd[1] == '-')
    {
        plus_enable_on = false;
        onoroff = "OFF";
    }

    OBTAIN_INTLOCK( NULL );
    {
        if (!IS_CPU_ONLINE( sysblk.pcpu ))
        {
            RELEASE_INTLOCK( NULL );
            // "Processor %s%02X: processor is not %s"
            WRMSG( HHC00816, "W", PTYPSTR( sysblk.pcpu ), sysblk.pcpu, "online" );
            return 0;
        }

        regs = sysblk.regs[ sysblk.pcpu ];

        // f- and f+ commands - mark 4K page frame as -unusable or +usable

        if (cmd[0] == 'f')
        {
            int rc = fonoff_cmd( regs, cmdline );
            RELEASE_INTLOCK( NULL );
            return rc;
        }

        // t+ckd [devnum] and t-ckd [devnum] commands - turn CKD Search Key tracing on/off

        if (1
            && (cmd[0] == 't')
            && (cmd[2] == 'c' || cmd[2] == 'C')
            && (cmd[3] == 'k' || cmd[3] == 'K')
            && (cmd[4] == 'd' || cmd[4] == 'D')
            && (cmd[5] ==  0  || cmd[5] == ' ')
        )
        {
            char buf[64];   // (results message buffer)

            if (cmd[5] ==  0) // (just "t+ckd" without any device number)
            {
                // Enable/disable CKD Search Key tracing for all CKD devices...

                bool bFound = false;

                for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
                {
                    if (dev->devchar[10] == DEVCLASS_DASD)
                    {
                        bFound = true;
                        dev->ckdkeytrace = plus_enable_on;
                    }
                }

                if (!bFound)
                {
                    RELEASE_INTLOCK( NULL );
                    // "No dasd devices found"
                    WRMSG( HHC02226, "E" );
                    return -1;
                }

                // Build results message
                MSGBUF( buf, "%s for all dasd devices", onoroff );
            }
            else if (cmd[5] != ' ')
            {
                RELEASE_INTLOCK( NULL );
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", cmd, "" );
                return -1;
            }
            else // (optional device number presumably specified)
            {
                const char* p;
                U16 lcss, devnum;

                // Position to start of devnum operand
                for (p = &cmd[6]; *p && *p == ' '; ++p);

                // Parse the device number
                if (parse_single_devnum( p, &lcss, &devnum ) < 0)
                {
                    RELEASE_INTLOCK( NULL );
                    return -1;  // (error message already displayed)
                }

                // Validate device number
                if (!(dev = find_device_by_devnum( lcss, devnum )))
                {
                    RELEASE_INTLOCK( NULL );
                    // HHC02200 "%1d:%04X device not found"
                    devnotfound_msg( lcss, devnum );
                    return -1;
                }

                if (dev->devchar[10] != DEVCLASS_DASD)
                {
                    RELEASE_INTLOCK( NULL );
                    // "%1d:%04X is not a dasd device"
                    WRMSG( HHC02225, "E", lcss, devnum );
                    return -1;
                }

                // Enable/disable CKD Search Key tracing for this device
                dev->ckdkeytrace = plus_enable_on;

                // Build results message
                MSGBUF( buf, "%s for device %1d:%04X", onoroff, lcss, devnum );
            }

            RELEASE_INTLOCK( NULL );

            // "%-14s set to %s"
            WRMSG( HHC02204, "I", "CKD key trace", buf );
            return 0;
        }

        // "t+cpu [cpunum]" command - turn instruction tracing on/off for CPU(s)...

        if (1
            && (cmd[0] == 't')
            && (cmd[2] == 'c' || cmd[2] == 'C')
            && (cmd[3] == 'p' || cmd[3] == 'P')
            && (cmd[4] == 'u' || cmd[4] == 'U')
            && (cmd[5] ==  0  || cmd[5] == ' ')
        )
        {
            int cpu;
            char buf[64];   // (results message buffer)

            if (cmd[5] ==  0) // (just "t+cpu" without any CPU number)
            {
                // Enable/disable instruction tracing for all CPUs...

                sysblk.insttrace = plus_enable_on;

                for (cpu=0; cpu < sysblk.maxcpu; cpu++)
                {
                    if (IS_CPU_ONLINE( cpu ))
                        sysblk.regs[ cpu ]->insttrace = plus_enable_on;
                }

                // Build results message
                MSGBUF( buf, "%s for all CPUs", onoroff );
            }
            else if (cmd[5] != ' ')
            {
                RELEASE_INTLOCK( NULL );
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", cmd, "" );
                return -1;
            }
            else // (optional CPU number presumably specified)
            {
                const char* p;
                U16 cpu16;
                BYTE c;
                bool trace_cpu = false;

                // Position to start of cpunum operand
                for (p = &cmd[6]; *p && *p == ' '; ++p);

                // Parse the CPU number
                if (sscanf( p, "%hu%c", &cpu16, &c ) != 1)
                {
                    RELEASE_INTLOCK( NULL );
                    // "Invalid argument %s%s"
                    WRMSG( HHC02205, "E", cmd, "" );
                    return -1;
                }

                // Validate CPU number
                if (0
                    || cpu16 >= MAX_CPU_ENGS
                    || cpu16 >= sysblk.maxcpu
                    || !IS_CPU_ONLINE( cpu16 )
                )
                {
                    RELEASE_INTLOCK( NULL );
                    // "CPU %02X is not online"
                    WRMSG( HHC02254, "E", cpu16 );
                    return -1;
                }

                // Enable/disable instruction tracing for this CPU
                sysblk.regs[ cpu16 ]->insttrace = plus_enable_on;

                // Disable/enable overall instruction tracing depending
                // on whether it's now enabled/disabled for all CPUs or
                // not. (i.e. if it's enabled for some CPUs but not for
                // others or vice-versa, then overall tracing should be
                // enabled. Otherwise if it's not enabled for any CPU,
                // then it should be disabled.)

                for (cpu=0; cpu < sysblk.maxcpu; cpu++)
                {
                    if (IS_CPU_ONLINE( cpu ))
                    {
                        if (sysblk.regs[ cpu ]->insttrace)
                        {
                            trace_cpu = true;
                            sysblk.insttrace = 1;
                            break;
                        }
                    }
                }

                if (!trace_cpu)
                    sysblk.insttrace = 0;

                // Build results message
                MSGBUF( buf, "%s for %s%02X", onoroff, ptyp2short( sysblk.ptyp[ cpu16 ] ), cpu16 );
            }

            RELEASE_INTLOCK( NULL );

            // "%-14s set to %s"
            WRMSG( HHC02204, "I", "CPU tracing", buf );
            return 0;
        }

        // o+devn and o-devn commands - turn ORB tracing on/off
        // t+devn and t-devn commands - turn CCW tracing on/off

        if (1
            && (cmd[0] == 'o' || cmd[0] == 't')
            && parse_single_devnum_silent( &argv[0][2], &lcss, &devnum ) == 0
        )
        {
            size_t  buflen = cmdlen + 20 + 10 + 1;
            char*   typ;
            char*   for_ccws;
            char*   buf = malloc( buflen );

            if (!buf)
            {
                // "Error in function %s: %s"
                WRMSG( HHC02219, "E", "malloc()", strerror( ENOMEM ));
                RELEASE_INTLOCK( NULL );
                return -1;
            }

            if (!(dev = find_device_by_devnum( lcss, devnum )))
            {
                // HHC02200 "%1d:%04X device not found"
                devnotfound_msg( lcss, devnum );
                RELEASE_INTLOCK( NULL );
                free( buf );
                return -1;
            }

            // Check for and parse optional "(aa,bb,cc,...zz)" CCWs argument

            if (argc >= 2 && argv[1])
            {
                bool   err = false;
                char*  copyofarg1;
                char*  token;
                BYTE   ccw_opcode;

                if (strspn( argv[1], "(,)0123456789abcdefABCDEF" ) != strlen( argv[1] ))
                {
                    RELEASE_INTLOCK( NULL );
                    // "Invalid argument %s%s"
                    WRMSG( HHC02205, "E", argv[1], "" );
                    return -1;
                }

                copyofarg1 = strdup( argv[1] );
                memset( ccwops, 0, 256 );
                token = strtok( copyofarg1, "(,)" );

                while (token)
                {
                    // Validate token...
                    if (0
                        || strlen( token ) != 2
                        || !is_hex( token )
                    )
                    {
                        err = true;
                        break;
                    }

                    // Process token
                    ccw_opcode = (BYTE) strtol( token, NULL, 16 );
                    ccwops[ ccw_opcode ] = 0xFF;

                    // Get next token...
                    token = strtok( NULL, "(,)" );
                }

                free( copyofarg1 );

                if (err)
                {
                    RELEASE_INTLOCK( NULL );
                    // "Invalid argument %s%s"
                    WRMSG( HHC02205, "E", argv[1], "" );
                    return -1;
                }

                dev->ccwopstrace = true;
                memcpy( dev->ccwops, ccwops, 256 );
                for_ccws = " for CCWs ";
            }
            else
            {
                for_ccws = "";
                memset( dev->ccwops, 0xFF, 256 );
                dev->ccwopstrace = false;
            }

            if (cmd[0] == 'o')
            {
                typ = "ORB trace";
                dev->orbtrace = plus_enable_on;
            }
            else // (cmd[0] == 't')
            {
                typ = "CCW trace";
                dev->orbtrace = plus_enable_on;
                dev->ccwtrace = plus_enable_on;
            }

            snprintf( buf, buflen, "%s for %1d:%04X%s%s",
                typ, lcss, devnum,
                for_ccws[0] ? for_ccws : "",
                for_ccws[0] ? argv[1]  : "" );

            // "%-14s set to %s"
            WRMSG( HHC02204, "I", buf, onoroff );
            free( buf );
            RELEASE_INTLOCK( NULL );
            return 0;
        }
    }
    RELEASE_INTLOCK( NULL );

    // "Invalid argument %s%s"
    WRMSG( HHC02205, "E", cmd, "" );
    return -1;
}

/*-------------------------------------------------------------------*/
/* cmdsep                                                            */
/*-------------------------------------------------------------------*/
int cmdsep_cmd( int argc, char* argv[], char* cmdline )
{
    int rc = 0;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    // Display current setting if requested

    if (argc == 1)
    {
        if (!sysblk.cmdsep)
        {
            // "%-14s: %s"
            WRMSG( HHC02203, "I", argv[0], "Not set" );
        }
        else
        {
            char sepchar[2];

            sepchar[0] = sysblk.cmdsep;
            sepchar[1] = 0;

            // "%-14s: %s"
            WRMSG( HHC02203, "I", argv[0], sepchar );
        }
    }

    // Disable

    else if (argc == 2 && CMD( argv[1], off, 3 ))
    {
        sysblk.cmdsep = 0;

        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], "OFF" );
    }

    // Enable

    else if (argc == 2 && argv[1][1] == 0)
    {
        /* Reject invalid separator characters */
        if (0
            || argv[1][0] == '-'
            || argv[1][0] == '.'
            || argv[1][0] == '!'
        )
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "; '.', '-', and '!' are invalid characters" );
            rc = -1;
        }
        else
        {
            char sepchar[2];

            sepchar[0] = sysblk.cmdsep = argv[1][0];
            sepchar[1] = 0;

            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], sepchar );
        }
    }
    else if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    else
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], ", must be a single character" );
        rc = -1;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* iconpfxs -- define integrated console prefix characters           */
/*-------------------------------------------------------------------*/
int iconpfxs_cmd( int argc, char* argv[], char* cmdline )
{
    int    num_pfxs;                    /* Number  command prefixes  */
    char*  cmd_pfxs;                    /* Default command prefixes  */
    char*  used_pfxs;                   /* Used    command prefixes  */
    char*  p;                           /* (work)                    */

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Display current settings? */
    if (argc == 1)
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], sysblk.cmd_pfxs );
        return 0;
    }

    /* Too many arguments? */
    if (argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /* Reset list back to its original default? */
    if (str_eq( argv[1], "*" ))
    {
        num_pfxs  = (int) strlen( DEF_CMDPREFIXES );
        cmd_pfxs  = malloc( num_pfxs );
        used_pfxs = malloc( num_pfxs );

        if (!cmd_pfxs || !used_pfxs)
        {
            free( cmd_pfxs );
            free( used_pfxs );

            // "Out of memory"
            WRMSG( HHC00152, "E" );
            return -1;
        }

        memcpy( cmd_pfxs, DEF_CMDPREFIXES, num_pfxs );
    }
    else
    {
        int i;

        /* Define new set of command prefixes */

        num_pfxs = (int) strlen( argv[1] );
        ASSERT( num_pfxs > 0 );

        /* Verify each character is unique */

        for (i=0; i < num_pfxs-1; ++i)
        {
            p = memchr( &argv[1][i+1], argv[1][i], num_pfxs-i-1 );

            if (p)
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[1], "" );
                return -1;
            }
        }

        cmd_pfxs  = malloc( num_pfxs );
        used_pfxs = malloc( num_pfxs );

        if (!cmd_pfxs || !used_pfxs)
        {
            free( cmd_pfxs );
            free( used_pfxs );

            // "Out of memory"
            WRMSG( HHC00152, "E" );
            return -1;
        }

        memcpy( cmd_pfxs, argv[1], num_pfxs );
    }

    OBTAIN_INTLOCK( NULL );
    {
        DEVBLK*  dev;
        size_t   i;

        /* Ensure the new "used" list is set accurately */

        memset( used_pfxs, 0, num_pfxs );

        for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
        {
            /* Is this device's prefix char in the new list? */
            if (IS_INTEGRATED_CONS( dev ))
            {
                p = memchr( cmd_pfxs, dev->filename[0], num_pfxs );

                if (p)
                {
                    i = (p - cmd_pfxs);
                    used_pfxs[i] = TRUE;
                }
            }
        }

        /* Update sysblk with the new values */

        sysblk.num_pfxs = num_pfxs;

        free( sysblk.cmd_pfxs  );
        free( sysblk.used_pfxs );

        sysblk.cmd_pfxs  = cmd_pfxs;
        sysblk.used_pfxs = used_pfxs;
    }
    RELEASE_INTLOCK( NULL );

    // "%-14s set to %s"
    WRMSG( HHC02204, "I", argv[0], sysblk.cmd_pfxs );

    return 0;
}

#if defined( _FEATURE_SYSTEM_CONSOLE )
/*-------------------------------------------------------------------*/
/* ssd - signal shutdown command                                     */
/*-------------------------------------------------------------------*/
int ssd_cmd( int argc, char* argv[], char* cmdline )
{
    static const U16  count = 0;  // SCLP_READ_EVENT_DATA return value
    static const BYTE unit  = 0;  // SCLP_READ_EVENT_DATA return value

    UNREFERENCED( argc );
    UNREFERENCED( argv );
    UNREFERENCED( cmdline );

    signal_quiesce( count, unit );    // (notify guest of shutdown)

    return 0;
}

/*-------------------------------------------------------------------*/
/* scpecho - enable echo of '.' and '!' replys/responses to hardcopy */
/*           and console.                                            */
/*-------------------------------------------------------------------*/
int scpecho_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    if ( argc == 2 )
    {
        if ( CMD(argv[1],on,2)  )
            sysblk.scpecho = TRUE;
        else if ( CMD(argv[1],off,3) )
            sysblk.scpecho = FALSE;
        else
        {
            WRMSG( HHC02205, "E", argv[1], "" );
            return 0;
        }
    }
    else if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return 0;
    }
    if ( argc == 1 )
        WRMSG(HHC02203, "I", argv[0], (sysblk.scpecho ? "ON" : "OFF") );
    else
        WRMSG(HHC02204, "I", argv[0], (sysblk.scpecho ? "ON" : "OFF") );

    return 0;
}

/*-------------------------------------------------------------------*/
/* scpimply - enable/disable non-hercules commands to the scp        */
/*           if scp has enabled scp commands.                        */
/*-------------------------------------------------------------------*/
int scpimply_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);

    UPPER_ARGV_0( argv );

    if ( argc == 2 )
    {
        if ( CMD(argv[1],on,2) )
            sysblk.scpimply = TRUE;
        else if ( CMD(argv[1],off,3) )
            sysblk.scpimply = FALSE;
        else
        {
            WRMSG( HHC02205, "E", argv[1], "" );
            return 0;
        }
    }
    else if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return 0;
    }

    if ( argc == 1 )
        WRMSG(HHC02203, "I", argv[0], (sysblk.scpimply ? "ON" : "OFF") );
    else
        WRMSG(HHC02204, "I", argv[0], (sysblk.scpimply ? "ON" : "OFF") );
    return 0;
}
#endif /* defined( _FEATURE_SYSTEM_CONSOLE ) */

/*-------------------------------------------------------------------*/
/* ldmod - load a module                                             */
/*-------------------------------------------------------------------*/
int ldmod_cmd( int argc, char* argv[], char* cmdline )
{
    int  i;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc <= 1)
    {
        // "HDL: usage: %s <module>"
        WRMSG( HHC01525, "E", argv[ 0 ]);
        return -1;
    }

    for (i=1; i < argc; i++)
    {
        // "HDL: loading module %s..."
        WRMSG( HHC01526, "I", argv[ i ]);

        if (hdl_loadmod( argv[i], 0 ) == 0)
            // "HDL: module %s loaded"
            WRMSG( HHC01527, "I", argv[ i ]);
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* rmmod - delete a module                                           */
/*-------------------------------------------------------------------*/
int rmmod_cmd( int argc, char* argv[], char* cmdline )
{
    int  i;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc <= 1)
    {
        // "HDL: usage: %s <module>"
        WRMSG( HHC01525, "E", argv[ 0 ]);
        return -1;
    }

    for (i=1; i < argc; i++)
    {
        // "HDL: unloading module %s..."
        WRMSG( HHC01528, "I", argv[ i ]);

        if (hdl_freemod( argv[ i ]) == 0)
            // "HDL: module %s unloaded"
            WRMSG( HHC01529, "I", argv[ i ]);
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* lsmod - list dynamic modules                                      */
/*-------------------------------------------------------------------*/
int lsmod_cmd( int argc, char* argv[], char* cmdline )
{
    int flags = HDL_LIST_DEFAULT;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc > 1)
    {
        if (0
            || argc > 2
            || !CMD( argv[1], ALL, 3 )
        )
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }

        flags = HDL_LIST_ALL;
    }

    hdl_listmods( flags );
    return 0;
}

/*-------------------------------------------------------------------*/
/* lsdep - list module dependencies                                  */
/*-------------------------------------------------------------------*/
int lsdep_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc != 1)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    hdl_listdeps();
    return 0;
}

/*-------------------------------------------------------------------*/
/* lsequ - list device equates                                       */
/*-------------------------------------------------------------------*/
int lsequ_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc != 1)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    return hdl_listequs();
}

/*-------------------------------------------------------------------*/
/* modpath - set module path                                         */
/*-------------------------------------------------------------------*/
int modpath_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc > 2)
    {
        // "HDL: incorrect syntax. Enter \"help %s\" for assistance"
        WRMSG( HHC01530, "E", argv[0] );
        return -1;
    }

    if (argc < 2)
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], hdl_getpath() );
    }
    else
    {
        int rc = hdl_setpath( argv[1] );

        if (rc != 0)
            return rc;  // (error or warning message already issued)

        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], hdl_getpath() );
    }

    return 0;
}

#if defined( _FEATURE_ECPSVM )
/*-------------------------------------------------------------------*/
/* ecpsvm - ECPS:VM command                                          */
/*-------------------------------------------------------------------*/
int ecpsvm_cmd( int argc, char *argv[], char *cmdline )
{
    int rc = 0;
    char msgbuf[64];

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    // EVM      ...     (deprecated)
    // ECPS:VM  ...     (deprecated)
    if (0
        || CMD( argv[0], evm,     3 )
        || CMD( argv[0], ecps:vm, 7 )
    )
    {
        // "Command '%s' is deprecated%s"
        WRMSG( HHC02256, "W", argv[0], "; use ECPSVM instead" );
        // (fall through to process their command anyway)
    }

    // ECPSVM NO
    if (argc == 2 && CMD( argv[1], no, 2 ))
    {
        sysblk.ecpsvm.available = FALSE;

        if (MLVL( VERBOSE ))
        {
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], "disabled" );
        }
    }
    // ECPSVM YES [TRAP|NOTRAP]
    else if (argc >= 2 && CMD( argv[1], yes, 3 ))
    {
        int err = 0;

        sysblk.ecpsvm.available = TRUE;

        if (argc == 2)
        {
            sysblk.ecpsvm.enabletrap = TRUE;
            MSGBUF( msgbuf, "%s", "enabled, trap support enabled" );
        }
        else // (argc == 3)
        {
            if (CMD( argv[2], trap, 4 ))
            {
                sysblk.ecpsvm.enabletrap = TRUE;
                MSGBUF( msgbuf, "%s", "enabled, trap support enabled" );
            }
            else if (CMD( argv[2], notrap, 6 ))
            {
                sysblk.ecpsvm.enabletrap = FALSE;
                MSGBUF( msgbuf, "%s", "enabled, trap support disabled" );
            }
            else
            {
                err = 1;
                // "Unknown ECPS:VM subcommand %s"
                WRMSG( HHC01721, "E", argv[2] );
                rc = -1;
            }
        }

        if (!err && MLVL( VERBOSE ))
        {
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], msgbuf );
        }
    }
    // ECPSVM LEVEL n
    else if (argc >= 2 && CMD( argv[1], level, 5 ))
    {
        int lvl = 20;

        if (argc == 3)
        {
            BYTE c;

            if (sscanf( argv[2], "%d%c", &lvl, &c ) != 1)
            {
                // "Invalid ECPS:VM level value : %s. Default of 20 used"
                WRMSG( HHC01723, "W", argv[2] );
                lvl = 20;
            }
        }

        sysblk.ecpsvm.level      = lvl;
        sysblk.ecpsvm.available  = TRUE;
        sysblk.ecpsvm.enabletrap = FALSE;

        MSGBUF( msgbuf, "enabled: level %d", lvl );
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], msgbuf );
    }
    else
    {
        // Some other subcommand (help, stats, enable, debug, etc)
        rc = ecpsvm_command( argc, argv );
    }

    return rc;
}
#endif /* defined( _FEATURE_ECPSVM ) */

/*-------------------------------------------------------------------*/
/* herclogo - Set the hercules logo file                             */
/*-------------------------------------------------------------------*/
int herclogo_cmd(int argc,char *argv[], char *cmdline)
{
    int rc = 0;
    char    fn[FILENAME_MAX];

    memset(fn,0,sizeof(fn));

    UNREFERENCED(cmdline);

    if ( argc < 2 )
    {
        sysblk.logofile=NULL;
        clearlogo();
        return 0;
    }
    if ( argc > 3 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    hostpath(fn,argv[1],sizeof(fn));

    rc = readlogo(fn);

    if ( rc == -1 && strcasecmp(fn,basename(fn)) == 0
                  && strlen(sysblk.hercules_pgmpath) > 0 )
    {
        char altfn[FILENAME_MAX];
        char pathname[MAX_PATH];

        memset(altfn,0,sizeof(altfn));

        MSGBUF(altfn,"%s%c%s", sysblk.hercules_pgmpath, PATHSEPC, fn);
        hostpath(pathname,altfn,sizeof(pathname));
        rc = readlogo(pathname);
    }

    if ( rc == -1 && MLVL(VERBOSE) )
        WRMSG( HHC01430, "E", "fopen()", strerror(errno) );

    return rc;
}

/*-------------------------------------------------------------------*/
/* sizeof - Display sizes of various structures/tables               */
/*-------------------------------------------------------------------*/
int sizeof_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );
    UNREFERENCED( argc );
    UNREFERENCED( argv );

    // HHC02257 "%s%7d"

    WRMSG( HHC02257, "I", "(unsigned short) ......", (int) sizeof( unsigned short ));
    WRMSG( HHC02257, "I", "(void*) ...............", (int) sizeof( void*          ));
    WRMSG( HHC02257, "I", "(unsigned int) ........", (int) sizeof( unsigned int   ));
    WRMSG( HHC02257, "I", "(long) ................", (int) sizeof( long           ));
    WRMSG( HHC02257, "I", "(long long) ...........", (int) sizeof( long long      ));
    WRMSG( HHC02257, "I", "(size_t) ..............", (int) sizeof( size_t         ));
    WRMSG( HHC02257, "I", "(off_t) ...............", (int) sizeof( off_t          ));
    WRMSG( HHC02257, "I", "FILENAME_MAX ..........", (int) FILENAME_MAX            );
    WRMSG( HHC02257, "I", "PATH_MAX ..............", (int) PATH_MAX                );
    WRMSG( HHC02257, "I", "SYSBLK ................", (int) sizeof( SYSBLK         ));
    WRMSG( HHC02257, "I", "REGS ..................", (int) sizeof( REGS           ));
    WRMSG( HHC02257, "I", "REGS (copy len) .......", (int) sysblk.regs_copy_len    );
    WRMSG( HHC02257, "I", "PSW ...................", (int) sizeof( PSW            ));
    WRMSG( HHC02257, "I", "DEVBLK ................", (int) sizeof( DEVBLK         ));
    WRMSG( HHC02257, "I", "TLB entry .............", (int) sizeof( TLB ) / TLBN    );
    WRMSG( HHC02257, "I", "TLB table .............", (int) sizeof( TLB            ));
    WRMSG( HHC02257, "I", "CPU_BITMAP ............", (int) sizeof( CPU_BITMAP     ));
    WRMSG( HHC02257, "I", "FD_SETSIZE ............", (int) FD_SETSIZE              );
    WRMSG( HHC02257, "I", "TID ...................", (int) sizeof( TID            ));
    WRMSG( HHC02257, "I", "STFL_IBM_LAST_BIT .....", (int) STFL_IBM_LAST_BIT );
    WRMSG( HHC02257, "I", "STFL_IBM_BY_SIZE ......", (int) STFL_IBM_BY_SIZE );
    WRMSG( HHC02257, "I", "STFL_IBM_DW_SIZE ......", (int) STFL_IBM_DW_SIZE );
    WRMSG( HHC02257, "I", "STFL_HERC_FIRST_BIT ...", (int) STFL_HERC_FIRST_BIT );
    WRMSG( HHC02257, "I", "STFL_HERC_LAST_BIT ....", (int) STFL_HERC_LAST_BIT );
    WRMSG( HHC02257, "I", "STFL_HERC_BY_SIZE .....", (int) STFL_HERC_BY_SIZE );
    WRMSG( HHC02257, "I", "STFL_HERC_DW_SIZE .....", (int) STFL_HERC_DW_SIZE );

    // HHC00001 "%s%s"

    WRMSG( HHC00001, "I", "", "TIDPAT ................ " TIDPAT );
    WRMSG( HHC00001, "I", "", "SCN_TIDPAT ............ " SCN_TIDPAT );

    return 0;
}

#if defined( OPTION_HAO )
/*-------------------------------------------------------------------*/
/* hao - Hercules Automatic Operator                                 */
/*-------------------------------------------------------------------*/
int hao_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);
    hao_command(cmdline);   /* (actual HAO code is in module hao.c) */
    return 0;
}
#endif /* defined( OPTION_HAO ) */

/*-------------------------------------------------------------------*/
/* conkpalv - set console session TCP keepalive values               */
/*-------------------------------------------------------------------*/
int conkpalv_cmd( int argc, char *argv[], char *cmdline )
{
#if !defined( HAVE_BASIC_KEEPALIVE )

    UNREFERENCED( argc );
    UNREFERENCED( argv );
    UNREFERENCED( cmdline );

    // "This build of Hercules does not support TCP keepalive"
    WRMSG( HHC02321, "E" );
    return -1;

#else // basic, partial or full: must attempt setting keepalive

    char buf[40];
    int rc, sfd, idle, intv, cnt;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

  #if !defined( HAVE_FULL_KEEPALIVE ) && !defined( HAVE_PARTIAL_KEEPALIVE )

    // "This build of Hercules has only basic TCP keepalive support"
    WRMSG( HHC02322, "W" );

  #elif !defined( HAVE_FULL_KEEPALIVE )

    // "This build of Hercules has only partial TCP keepalive support"
    WRMSG( HHC02323, "W" );

  #endif // (basic or partial)

    /* Validate and parse the arguments passed to us */
    if (0
        || !(argc == 2 || argc < 2)
        ||  (argc == 2 && parse_conkpalv( argv[1], &idle, &intv, &cnt ) != 0)
    )
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], "" );
        return -1;
    }

    /* Need a socket for setting/getting */
    sfd = socket( AF_INET, SOCK_STREAM, 0 );

    if (sfd < 0)
    {
        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "socket()", strerror( HSO_errno ));
        return -1;
    }

    /*
    **  Set the requested values. Note since all sockets start out
    **  with default values, we must also set the keepalive values
    **  to our desired default values first (unless we are setting
    **  new default values) before retrieving the values that will
    **  actually be used (or are already in use) by the system.
    */
    if (argc < 2)
    {
        /* Try using the existing default values */
        idle = sysblk.kaidle;
        intv = sysblk.kaintv;
        cnt  = sysblk.kacnt;
    }

    /* Set new or existing default keepalive values */
    if ((rc = set_socket_keepalive( sfd, idle, intv, cnt )) < 0)
    {
        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "set_socket_keepalive()", strerror( HSO_errno ));
        close_socket( sfd );
        return -1;
    }
    else if (rc > 0)
    {
        // "Not all TCP keepalive settings honored"
        WRMSG( HHC02320, "W" );
    }

    /* Retrieve the system's current keepalive values */
    if (get_socket_keepalive( sfd, &idle, &intv, &cnt ) < 0)
    {
        WRMSG( HHC02219, "E", "get_socket_keepalive()", strerror( HSO_errno ));
        close_socket( sfd );
        return -1;
    }
    close_socket( sfd );

    /* Update SYSBLK with the values the system is actually using */
    sysblk.kaidle = idle;
    sysblk.kaintv = intv;
    sysblk.kacnt  = cnt;

    /* Now report the values that are actually being used */
    MSGBUF( buf, "(%d,%d,%d)", sysblk.kaidle, sysblk.kaintv, sysblk.kacnt );

    if (argc == 2)
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", "conkpalv", buf );
    else
        // "%-14s: %s"
        WRMSG( HHC02203, "I", "conkpalv", buf );

    return rc;

#endif // (KEEPALIVE)
}

/*-------------------------------------------------------------------*/
/* msglevel command                                                  */
/*-------------------------------------------------------------------*/
int msglevel_cmd( int argc, char* argv[], char* cmdline )
{
    char msgbuf[64] = {0};

    UNREFERENCED( cmdline );

    if ( argc < 1 )
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return -1;
    }

    if ( argc > 1 )
    {
        char upperarg[16];
        int  msglvl = sysblk.msglvl;
        int  i;

        for (i=1; i < argc; i++)
        {
            strnupper( upperarg, argv[i], sizeof( upperarg ));

            if (0
                || strabbrev(  "VERBOSE", upperarg, 1 )
                || strabbrev( "+VERBOSE", upperarg, 2 )
                || strabbrev( "-TERSE",   upperarg, 2 )
            )
                msglvl |= MLVL_VERBOSE;
            else
            if (0
                || strabbrev(  "TERSE",   upperarg, 1 )
                || strabbrev( "+TERSE",   upperarg, 2 )
                || strabbrev( "-VERBOSE", upperarg, 2 )
            )
                msglvl &= ~MLVL_VERBOSE;
            else
            if (0
                || strabbrev(    "DEBUG", upperarg, 1 )
                || strabbrev(   "+DEBUG", upperarg, 2 )
                || strabbrev( "-NODEBUG", upperarg, 2 )
            )
                msglvl |= MLVL_DEBUG;
            else
            if (0
                || strabbrev(  "NODEBUG", upperarg, 1 )
                || strabbrev( "+NODEBUG", upperarg, 1 )
                || strabbrev(   "-DEBUG", upperarg, 2 )
            )
                msglvl &= ~MLVL_DEBUG;
            else
            if (0
                || strabbrev(    "EMSGLOC", upperarg, 1 )
                || strabbrev(   "+EMSGLOC", upperarg, 2 )
                || strabbrev( "-NOEMSGLOC", upperarg, 2 )
            )
                msglvl |= MLVL_EMSGLOC;
            else
            if (0
                || strabbrev(  "NOEMSGLOC", upperarg, 1 )
                || strabbrev( "+NOEMSGLOC", upperarg, 2 )
                || strabbrev(   "-EMSGLOC", upperarg, 2 )
            )
                msglvl &= ~MLVL_EMSGLOC;
            else
            {
                WRMSG( HHC02205, "E", argv[i], "" );
                return -1;
            }
            sysblk.msglvl = msglvl;
        }
    }

    if (MLVL( VERBOSE )) STRLCAT( msgbuf, " verbose"   );
    else                 STRLCAT( msgbuf, " terse"     );
    if (MLVL( DEBUG   )) STRLCAT( msgbuf, " debug"     );
    else                 STRLCAT( msgbuf, " nodebug"   );
    if (MLVL( EMSGLOC )) STRLCAT( msgbuf, " emsgloc"   );
    else                 STRLCAT( msgbuf, " noemsgloc" );

    WRMSG( HHC17012, "I", &msgbuf[1] );

    return 0;
}

/*-------------------------------------------------------------------*/
/* qcpuid helpers                                                    */
/*-------------------------------------------------------------------*/
static void qcpuid_cpuid( U64 cpuid, const char* source )
{
    // "%-6s: CPUID  = %16.16"PRIX64
    WRMSG( HHC17004, "I",  source, cpuid );
}

static void qcpuid_cpcsi( U64 cpuid, const char* source )
{
    U16   machinetype = (cpuid >> 16) & 0xFFFF;
    BYTE  seqc[16+1];

    bld_sysib_sequence( seqc );
    buf_guest_to_host( seqc, seqc, 16 );
    seqc[16] = 0;

    // "%-6s: CPC SI = %4.4X.%s.%s.%s.%s"
    WRMSG( HHC17005, "I",  source, machinetype,
        str_modelcapa(), str_manufacturer(), str_plant(), seqc );
}

/*-------------------------------------------------------------------*/
/* qcpuid command                                                    */
/*-------------------------------------------------------------------*/
int qcpuid_cmd( int argc, char* argv[], char* cmdline )
{
    REGS* regs;
    int   cpu, cpunum = sysblk.pcpu;
    char  buf[16];
    BYTE  c;

    UNREFERENCED( cmdline );
    UNREFERENCED( argv );

    if (argc < 1 || argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if (argc == 2)
    {
        if (strcasecmp( argv[1], "ALL" ) == 0)
            cpunum = -1;
        else if (0
            || sscanf( argv[1], "%x%c", &cpunum, &c) != 1
            || cpunum < 0
            || cpunum >= sysblk.hicpu
            || !IS_CPU_ONLINE( cpunum )
        )
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], ": target processor is invalid" );
            return -1;
        }
    }

    /* Show 'CPUID' from SYSBLK */
    qcpuid_cpuid( sysblk.cpuid, "SYSBLK" );

    /* Show 'CPUID' from REGS */
    for (cpu=0; cpu < sysblk.hicpu; cpu++)
    {
        if (1
            && IS_CPU_ONLINE( cpu )
            && (cpunum < 0 || cpu == cpunum)
        )
        {
            regs = sysblk.regs[ cpu ];
            MSGBUF( buf, "%s%02X",
                PTYPSTR( regs->cpuad ), regs->cpuad );
            qcpuid_cpuid( regs->cpuid, buf );
        }
    }

    if (ARCH_370_IDX != sysblk.arch_mode)
    {
        /* Show 'CPC SI' information from SYSBLK */
        qcpuid_cpcsi( sysblk.cpuid, "SYSBLK" );

        /* Show 'CPC SI' information from REGS */
        for (cpu=0; cpu < sysblk.hicpu; cpu++)
        {
            if (1
                && IS_CPU_ONLINE( cpu )
                && (cpunum < 0 || cpu == cpunum)
            )
            {
                regs = sysblk.regs[ cpu ];
                MSGBUF( buf, "%s%02X",
                    PTYPSTR( regs->cpuad ), regs->cpuad );
                qcpuid_cpcsi( regs->cpuid, buf );
            }
        }
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* qpfkeys command                                                   */
/*-------------------------------------------------------------------*/
int qpfkeys_cmd( int argc, char* argv[], char* cmdline )
{
    const char*  pszVAL;
    int          i;
    char         szPF[6];

    UNREFERENCED( cmdline );
    UNREFERENCED( argv );

    if (argc != 1)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return -1;
    }

#if defined( _MSVC_ )
    for (i=1; i <= 48; i++)
#else // *Nix
    for (i=1; i <= 20; i++)
#endif
    {
        MSGBUF( szPF, "PF%02d", i );                    // (name)
        if (!(pszVAL = get_symbol( szPF )) || !*pszVAL) // (value)
            pszVAL = "UNDEFINED";

        // "%.4s %s"
        WRMSG( HHC17199, "I", szPF, pszVAL );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* qpid command                                                      */
/*-------------------------------------------------------------------*/
int qpid_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );
    UNREFERENCED( argv );
    UNREFERENCED( argc );

    if (argc != 1)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return -1;
    }

    // "Process ID = %d"
    WRMSG( HHC17013, "I", sysblk.hercules_pid );

    return 0;
}

/*-------------------------------------------------------------------*/
/* qports command                                                    */
/*-------------------------------------------------------------------*/
int qports_cmd( int argc, char* argv[], char* cmdline )
{
    char buf[64];

    UNREFERENCED( cmdline );
    UNREFERENCED( argv );

    if (argc != 1)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return -1;
    }


    // HTTP SERVER...

    MSGBUF( buf, "on port %s with %s", http_get_port(), http_get_portauth() );

    // "%s server %slistening %s"
    WRMSG( HHC17001, "I", "HTTP", "", buf );


    // SHARED DASD SERVER...

#if defined( OPTION_SHARED_DEVICES )

    if (sysblk.shrdport > 0)
    {
        MSGBUF( buf, "on port %u", sysblk.shrdport );

        // "%s server %slistening %s"
        WRMSG( HHC17001, "I", "Shared DASD", "", buf );
    }
    else
    {
        // "%s server inactive"
        WRMSG( HHC17002, "I", "Shared DASD" );
    }
#else // !defined( OPTION_SHARED_DEVICES )

    // "%s support not included in this engine build"
    WRMSG( HHC17015, "I", "Shared DASD" );

#endif // defined( OPTION_SHARED_DEVICES )


    // CONSOLE...

    if (!strchr( sysblk.cnslport, ':' ))
    {
        MSGBUF( buf, "on port %s", sysblk.cnslport );
    }
    else
    {
        char* serv;
        char* host = NULL;
        char* port = strdup( sysblk.cnslport );

        if ((serv = strchr( port, ':' )))
        {
            *serv++ = '\0';
            if (*port)
                host = port;
        }

        MSGBUF( buf, "for host %s on port %s", host, serv );
        free( port );
    }

    // "%s server %slistening %s"
    WRMSG( HHC17001, "I", "Console", "", buf );


    // SYSG CONSOLE...

    if (sysblk.sysgport)
    {
        if (!strchr( sysblk.sysgport, ':' ))
        {
            MSGBUF( buf, "on port %s", sysblk.sysgport );
        }
        else
        {
            char* serv;
            char* host = NULL;
            char* port = strdup( sysblk.sysgport );

            if ((serv = strchr( port, ':' )))
            {
                *serv++ = '\0';
                if (*port)
                    host = port;
            }

            MSGBUF( buf, "for host %s on port %s", host, serv );
            free( port );
        }

        // "%s server %slistening %s"
        WRMSG( HHC17001, "I", "SYSG console",
            sysblk.sysgdev && sysblk.sysgdev->connected ? "NOT " : "", buf );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* qproc command                                                     */
/*-------------------------------------------------------------------*/
int qproc_cmd( int argc, char* argv[], char* cmdline )
{
    int   i, j;
    int   cpupct = 0;
    U32   mipsrate = 0;
    char  msgbuf[128];

    UNREFERENCED( cmdline );
    UNREFERENCED( argv );

    if (argc != 1)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return -1;
    }

    /* Visual Studio doesn't support macros split with #ifdefs */
    {
#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )

        u_int  nv  = sysblk.numvec;
#else
        u_int  nv  = 0;
#endif
        // "NumCPU = %2.2d, NumVEC = %2.2d, ReservedCPU = %2.2d, MaxCPU = %2.2d"
        WRMSG( HHC17007, "I", sysblk.cpus, nv,
            sysblk.maxcpu - sysblk.cpus, sysblk.maxcpu );
    }

    for (i=j=0; i < sysblk.maxcpu; i++)
    {
        if (1
            && IS_CPU_ONLINE( i )
            && sysblk.regs[i]->cpustate == CPUSTATE_STARTED
        )
        {
            j++;
            cpupct += sysblk.regs[i]->cpupct;
        }
    }

    mipsrate = sysblk.mipsrate;

    // "Avgproc  %2.2d %3.3d%%; MIPS[%4d.%2.2d]; SIOS[%6d]%s"
    WRMSG( HHC17008, "I",
        (j),
        (j == 0 ? 0 : (cpupct / j)),
        (mipsrate / 1000000),
        (mipsrate % 1000000) / 10000,
        sysblk.siosrate, "" );

    for (i=0; i < sysblk.maxcpu; i++)
    {
        if (IS_CPU_ONLINE( i ))
        {
            struct rusage  rusage;
            char*          pmsg     = "";

            if (getrusage( (int) sysblk.cputid[i], &rusage ) == 0)
            {
                char    kdays[18], udays[18];

                U64     kdd, khh, kmm, kss, kms,
                        udd, uhh, umm, uss, ums;

                if (unlikely( rusage.ru_stime.tv_usec >= 1000000 ))
                {
                    rusage.ru_stime.tv_sec += rusage.ru_stime.tv_usec / 1000000;
                    rusage.ru_stime.tv_usec %= 1000000;
                }

                kss = rusage.ru_stime.tv_sec;
                kdd = kss / 86400;

                if (kdd)
                {
                    kss %= 86400;
                    MSGBUF( kdays, "%"PRIu64"/", kdd );
                }
                else
                    kdays[0] = 0;

                khh = kss /  3600, kss %=  3600;
                kmm = kss /    60, kss %=    60;

                kms = (rusage.ru_stime.tv_usec + 500) / 1000;

                if (unlikely( rusage.ru_utime.tv_usec >= 1000000 ))
                {
                    rusage.ru_utime.tv_sec += rusage.ru_utime.tv_usec / 1000000;
                    rusage.ru_utime.tv_usec %= 1000000;
                }

                uss = rusage.ru_utime.tv_sec;
                udd = uss / 86400;

                if (udd)
                {
                    uss %= 86400;
                    MSGBUF( udays, "%"PRIu64"/", udd );
                }
                else
                    udays[0] = 0;

                uhh = uss /  3600, uss %=  3600;
                umm = uss /    60, uss %=    60;

                ums = (rusage.ru_utime.tv_usec + 500) / 1000;

                MSGBUF( msgbuf, " - Host Kernel(%s%02d:%02d:%02d.%03d) "
                                          "User(%s%02d:%02d:%02d.%03d)",
                        kdays, (int) khh, (int) kmm, (int) kss, (int) kms,
                        udays, (int) uhh, (int) umm, (int) uss, (int) ums);

                pmsg = msgbuf;
            }

            mipsrate = sysblk.regs[i]->mipsrate;

            // "PROC %s%2.2X %c %3.3d%%; MIPS[%4d.%2.2d]; SIOS[%6d]%s"
            WRMSG( HHC17009, "I", PTYPSTR(i), i,
                (sysblk.regs[i]->cpustate == CPUSTATE_STARTED ) ? '-' :
                (sysblk.regs[i]->cpustate == CPUSTATE_STOPPING) ? ':' : '*',
                (sysblk.regs[i]->cpupct),
                (mipsrate / 1000000),
                (mipsrate % 1000000) / 10000,
                (sysblk.regs[i]->siosrate),
                pmsg );
        }
    }

    // " - Started        : Stopping        * Stopped"
    WRMSG( HHC17010, "I" );

    return 0;
}

/*-------------------------------------------------------------------*/
/* qstor command                                                     */
/*-------------------------------------------------------------------*/
int qstor_cmd( int argc, char* argv[], char* cmdline )
{
    BYTE  display_main  = FALSE;
    BYTE  display_xpnd  = FALSE;

    char  memsize[128];

    UNREFERENCED( cmdline );

    if (argc < 2)
        display_main  = display_xpnd  = TRUE;
    else
    {
        char  check[16];
        int   i;

        for (i=1; i < argc; i++)
        {
            strupper( check, argv[1] ); // Uppercase for multiple checks

                 if (strabbrev( "MAINSIZE", check, 1 )) display_main = TRUE;
            else if (strabbrev( "XPNDSIZE", check, 1 )
                 ||  strabbrev( "EXPANDED", check, 1 )) display_xpnd = TRUE;
            else
            {
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[1], "; either 'mainsize' or 'xpndsize' is valid" );
                return -1;
            }
        }
    }

    if (display_main)
    {
        fmt_memsize_KB( sysblk.mainsize >> SHIFT_KIBIBYTE,
            memsize, sizeof( memsize ));

        // "%-8s storage is %s (%ssize); storage is %slocked"
        WRMSG( HHC17003, "I", "MAIN", memsize, "main",
            sysblk.mainstor_locked ? "" : "not " );
    }

    if (display_xpnd)
    {
        fmt_memsize_MB( sysblk.xpndsize >> (SHIFT_MEBIBYTE - XSTORE_PAGESHIFT),
            memsize, sizeof( memsize ));

        // "%-8s storage is %s (%ssize); storage is %slocked"
        WRMSG( HHC17003, "I", "EXPANDED", memsize, "xpnd",
            sysblk.xpndstor_locked ? "" : "not " );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* cmdlvl - display/set the current command level group(s)           */
/*-------------------------------------------------------------------*/
int cmdlvl_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    if (argc > 1)
    {
        char*  p;
        int    i;
        char   level[16];
        BYTE   on;
        BYTE   option;
        BYTE   merged_options  = sysblk.sysgroup;

        for (i=1; i < argc; i++)
        {
            p = argv[i];

           if (p[0] == '-')
           {
                on = FALSE;
                p++;
            }
            else if (p[0] == '+')
            {
                on = TRUE;
                p++;
            }
            else
                on = TRUE;

            strupper( level, p );

            if (strabbrev( "ALL", level, 1 ))
            {
                if (on)                                      option =  SYSGROUP_SYSALL;
                else                                         option = ~SYSGROUP_SYSNONE;
            }
            else if (strabbrev( "OPERATOR",      level, 4 )) option = SYSGROUP_SYSOPER;
            else if (strabbrev( "MAINTENANCE",   level, 5 )) option = SYSGROUP_SYSMAINT;
            else if (strabbrev( "PROGRAMMER",    level, 4 )) option = SYSGROUP_SYSPROG;
            else if (strabbrev( "CONFIGURATION", level, 6 )) option = SYSGROUP_SYSCONFIG;
            else if (strabbrev( "DEVELOPER",     level, 3 )) option = SYSGROUP_SYSDEVEL;
            else if (strabbrev( "DEBUG",         level, 3 )) option = SYSGROUP_SYSDEBUG;
            else
            {
                // "Invalid cmdlevel option: %s"
                WRMSG( HHC01605,"E", argv[i] );
                return -1;
            }

            if (on) merged_options |=  option;
            else    merged_options &= ~option;
        }

        sysblk.sysgroup = merged_options;
    }

    /* Display the current/new value */

    if (sysblk.sysgroup == SYSGROUP_SYSALL)
    {
        // "cmdlevel[%2.2X] is %s"
        WRMSG( HHC01606, "I", sysblk.sysgroup, "ALL" );
    }
    else if (sysblk.sysgroup == SYSGROUP_SYSNONE)
    {
        // "cmdlevel[%2.2X] is %s"
        WRMSG( HHC01606, "I", sysblk.sysgroup, "NONE" );
    }
    else
    {
        char buf[128];

        MSGBUF( buf, "%s%s%s%s%s%s",

            (sysblk.sysgroup & SYSGROUP_SYSOPER  ) ? "OPERATOR "      : "",
            (sysblk.sysgroup & SYSGROUP_SYSMAINT ) ? "MAINTENANCE "   : "",
            (sysblk.sysgroup & SYSGROUP_SYSPROG  ) ? "PROGRAMMER "    : "",
            (sysblk.sysgroup & SYSGROUP_SYSCONFIG) ? "CONFIGURATION " : "",
            (sysblk.sysgroup & SYSGROUP_SYSDEVEL ) ? "DEVELOPER "     : "",
            (sysblk.sysgroup & SYSGROUP_SYSDEBUG ) ? "DEBUG "         : ""
        );

        if (    strlen( buf ) > 1)
            buf[strlen( buf ) - 1] = 0;

        // "cmdlevel[%2.2X] is %s"
        WRMSG( HHC01606, "I", sysblk.sysgroup, buf );
    }

    return 0;
}

/* HSCCMD.C End-of-text */

#endif // !defined(_GEN_ARCH)
