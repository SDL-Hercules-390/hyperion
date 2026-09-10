/* HMCWDT.C    (C) Copyright Jan Jaeger, 1999-2012                   */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*                                                                   */
/*              Hardware Management Console                          */
/*                     Watchdog Timer                                */
/*            (Control Virtual Machine Time Bomb)                    */
/*                 ( Diagnose 0x288 )                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"
#define _HMCWDT_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#include "hmcwdt.h"

/* forward references */
int hmcwdt_do_cmds( char*, char );

/*-----------------------------------------------------------*/
/* Diagnose 288: Watchdog Timer: ASYNC Action Commands Thread*/
/*-----------------------------------------------------------*/
/* Note: the  hmcwdt_doing_cmds flag indicates if this       */
/* thread is executing commands                              */
/*-----------------------------------------------------------*/
static void* hmcwdt_action_cmds_thread( void* arg )
{
    int   rc;
    char* cmdline;
    char  cmdsep;

    UNREFERENCED( arg );
    UNREFERENCED( rc );

    if (sysblk.hmcwdt_debug)
        logmsg( ">>>> hmcwdt_action_cmds_thread - started\n" );

    /* Set action command thread priority to Server thread          */
    /* priority in order to action commands if guest OS has not     */
    /* reset the timer and the timeout expires.                     */
    SET_THREAD_PRIORITY( sysblk.srvprio, sysblk.qos_user_initiated );

    /* Mark the thread as active */
    OBTAIN_HMCWDT_LOCK();
    {
        if ( sysblk.hmcwdt_doing_cmds )
        {
            /* already doing cmds - ignore this request */
            WRMSG( HHC01959, "I", "hmcwdt_action_cmds_thread: already doing cmds, ignoring request"  );
            RELEASE_HMCWDT_LOCK();
            return NULL;
        }

        sysblk.hmcwdt_doing_cmds = TRUE;
        cmdline = strdup( sysblk.hmcwdt_cmds );
        cmdsep = sysblk.hmcwdt_cmdsep;

    }
    RELEASE_HMCWDT_LOCK();

    WRMSG( HHC01958, "I", "Timer Expired. Executing", cmdline );
    hmcwdt_do_cmds( cmdline, cmdsep );

    /* Mark the thread as done */
    OBTAIN_HMCWDT_LOCK();
    {
        sysblk.hmcwdt_doing_cmds = FALSE;
        free (cmdline);
    }
    RELEASE_HMCWDT_LOCK();

    if (sysblk.hmcwdt_debug)
        logmsg( ">>>> hmcwdt_action_cmds_thread - ended\n" );
    return NULL;
}

/*-----------------------------------------------------------*/
/*  Diagnose 288: Watchdog Timer Thread (hmcwdt_thread)      */
/*-----------------------------------------------------------*/
/* Note: the sysblk.hmcwdt_active flag indicates if the      */
/* timer is active and is only set when the thread is        */
/* running                                                   */
/*-----------------------------------------------------------*/
static void* hmcwdt_thread( void* arg )
{
    int   rc;
    TID   tid;

    UNREFERENCED( arg );
    UNREFERENCED( rc );

    LOG_THREAD_BEGIN( HMCWDT_THREAD_NAME );

    /* Set watchdog priority to Server thread priority              */
    /* in order to action commands if guest OS has not reset        */
    /* the timer and the timeout expires. No impact as it           */
    /* should just be waiting anyway until a timeout.               */
    SET_THREAD_PRIORITY( sysblk.srvprio, sysblk.qos_user_initiated );

    /* Mark the timer as active */
    OBTAIN_HMCWDT_LOCK();
    {
        sysblk.hmcwdt_active = TRUE;
    }
    RELEASE_HMCWDT_LOCK();

    /* Watchdog thread main loop - wait for timer to be canceled */
    while ( true )
    {
        OBTAIN_HMCWDT_LOCK();
        {
            /* timer was closed/cancelled or we are in shutdown mode?*/
            if ( sysblk.hmcwdt_canceled || sysblk.shutdown)
            {
                break;
            }

            /* timer expired? */
            if ( sysblk.hmcwdt_expire_time < hmcwdt_get_expire_time( 0 ) )
            {
                /* are we already doing cmds? */
                if( !sysblk.hmcwdt_doing_cmds )
                {
                    rc = create_thread( &tid, DETACHED, hmcwdt_action_cmds_thread,
                                        NULL, "HMCWDT do cmds");
                    if(rc)
                        WRMSG(HHC00102, "E", strerror(rc));
                }

                /* reset the timer to default timeout */
                sysblk.hmcwdt_expire_time = hmcwdt_get_expire_time( HMCWDT_DEFAULT_TIMEOUT );
            }
        }
        RELEASE_HMCWDT_LOCK();
        USLEEP( HMCWDT_US_SLEEP );
    }

    /* timer expired or closed/cancelled; reset state to inactive*/
    /* already have lock */
    sysblk.hmcwdt_canceled = FALSE;
    sysblk.hmcwdt_active = FALSE;
    sysblk.hmcwdt_tid = 0;
    sysblk.hmcwdt_expire_time = 0;

    RELEASE_HMCWDT_LOCK();

    LOG_THREAD_END( HMCWDT_THREAD_NAME );
    return NULL;
}

/*-----------------------------------------------------------*/
/*  Diagnose 288: Helpers                                    */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/*  Diagnose 288: Open (initialize) Timer                    */
/*-----------------------------------------------------------*/
int hmcwdt_diag288_open( U32 timeout )
{
    int rc = 0;
    int n;

     /* Initialize the watchdog timer state */
    sysblk.hmcwdt_canceled = FALSE;
    sysblk.hmcwdt_active = FALSE;
    sysblk.hmcwdt_expire_time = hmcwdt_get_expire_time( timeout );

    /* Start the watchdog thread */
    rc = create_thread( &sysblk.hmcwdt_tid, DETACHED,
                        hmcwdt_thread, NULL, HMCWDT_THREAD_NAME );
    if (rc)
    {
        // "Error in function create_thread(): %s"
        WRMSG( HHC00102, "E", strerror( rc ));
        return rc;
    }

    /* Note: in a Diag 288 instruction, so only a short wait for timer startup */
    n = 10;
    while ( n-- >  0 )
    {
        usleep( HMCWDT_US_SLEEP );

        OBTAIN_HMCWDT_LOCK();
        {
            if ( sysblk.hmcwdt_active == TRUE)
            {
                RELEASE_HMCWDT_LOCK();
                break;
            }
        }
        RELEASE_HMCWDT_LOCK();
    }
    if (n <= 0)
    {
        // something happened; force cancel (without lock)
        sysblk.hmcwdt_canceled = TRUE;
        rc = -1;
    }
    return rc;
}

/*-----------------------------------------------------------*/
/*  Diagnose 288: Reset/Change Timer Timeout                 */
/*-----------------------------------------------------------*/
int hmcwdt_diag288_reset( U32 timeout )
{
    int rc = 0;

    OBTAIN_HMCWDT_LOCK();
    {
        sysblk.hmcwdt_expire_time = hmcwdt_get_expire_time( timeout );
    }
    RELEASE_HMCWDT_LOCK();
    return rc;
}

/*-----------------------------------------------------------*/
/*  Diagnose 288: Close/Cancel Timer                         */
/*-----------------------------------------------------------*/
int hmcwdt_diag288_close( U32 timeout )
{
    int rc = 0;
    int n;

    UNREFERENCED( timeout );

    /* initiate timer thread shutdown*/
    OBTAIN_HMCWDT_LOCK();
    {
        if ( sysblk.hmcwdt_active == FALSE )  // timer thread is not active
        {
            RELEASE_HMCWDT_LOCK();
            return -2;
        }
        sysblk.hmcwdt_canceled = TRUE;
    }
    RELEASE_HMCWDT_LOCK();

    /* Note: in a Diag 288 instruction so only a short wait for timer to stop */
    n = 10;
    while ( n-- > 0 )
    {
        usleep( HMCWDT_US_SLEEP );

        OBTAIN_HMCWDT_LOCK();
        {
            if ( sysblk.hmcwdt_active == FALSE)
            {
                RELEASE_HMCWDT_LOCK();
                break;
            }
        }
        RELEASE_HMCWDT_LOCK();
    }
    if (n <= 0)  rc = -1;
    return rc;
}

/*-----------------------------------------------------------*/
/*  Timer Expired: Process Commands  (recursive)             */
/*-----------------------------------------------------------*/
/* reference: cmdtab.c: the_real_panel_command(char* cmdline)*/
/*-----------------------------------------------------------*/
int hmcwdt_do_cmds( char* cmdline, char cmdsep )
{
    int rc = 0;                 /* Return Code from command  */

    if (sysblk.hmcwdt_debug)
        logmsg( ">>>>> hmcwdt_do_cmds- executing commands: %s\n", cmdline );

    /* Handle command separation */
    if (cmdsep)
    {
        /* Does cmdline contain any separator characters? */
        char*  firstsep  = strchr( cmdline, cmdsep );
        if (firstsep)
        {
            /* Yes! Must process commands separately */
            size_t  sepcharindex  = (firstsep - cmdline);
            char*   first_cmd     = strdup( cmdline );

            /* Mark end of first command */
            first_cmd[ sepcharindex ] = 0;

            TRIM( first_cmd );
            if (strncasecmp( first_cmd, "PAUSE", 5 ) == 0)
            {
                /* PAUSE command - pause for specified number of seconds */
                char*  arg = first_cmd + 5;
                int     pause_seconds = atoi( arg );

                if (pause_seconds > 0)
                {
                    // "HMC Watchdog Timer: %s: %d"
                    WRMSG( HHC01955, "I", "pausing for seconds", pause_seconds);
                    SLEEP( pause_seconds );
                }
                else
                {
                    // "HMC Watchdog Timer: %s: %s"
                    WRMSG( HHC01958, "E", "invalid pause value", arg);
                }
            }

            else if (strncasecmp( first_cmd, "HMCWDT", 6 ) == 0)
            {
                // "HMC Watchdog Timer: %s: %s"
                WRMSG( HHC01958, "I", "ignoring hmcwdt command", first_cmd);
            }

            else
            {
                /* Process command */
                if (strlen( first_cmd ) > 0)
                    panel_command( first_cmd );
            }

            /* Process remaining command(s) */
            rc = hmcwdt_do_cmds( first_cmd + sepcharindex + 1, cmdsep );

            /* Prevent memory leak */
            free( first_cmd );

            /* Return with retcode of second command */
            return rc;
        }
    }

    /* Only one command  */
    rc =  (int) (uintptr_t) panel_command( cmdline );
    return rc;
}

// Convert seconds to microseconds
#define SEC_TO_US(sec) ((sec)*1000000)
// Convert nanoseconds to microseconds
#define NS_TO_US(ns)    ((ns)/1000)

/*-----------------------------------------------------------*/
/*  Diagnose 288: Get Expire Time (microseconds)             */
/*-----------------------------------------------------------*/
U64 hmcwdt_get_expire_time( U32 timeout )
{
    U64 us          = 0;
    int rc;

    {
    #if defined( __linux__ )
        struct timespec ts;
        rc = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        if( rc == 0 )
            us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec + 500);
        else
        {
            // "HMC Watchdog Timer: %s: %s"
            WRMSG( HHC01958, "E", "clock_gettime failed", strerror(errno));
        }

    #else
        // microsecond resolution getimeofday
        struct timeval  tv;
        rc = gettimeofday( &tv, NULL );
        if( rc == 0 )
            us = SEC_TO_US((uint64_t)tv.tv_sec) + tv.tv_usec;
        else
        {
            // "HMC Watchdog Timer: %s: %s"
            WRMSG( HHC01958, "E", "gettimeofday failed", strerror(errno));
        }

    #endif
    }

    return us + SEC_TO_US(timeout);
}

/*-----------------------------------------------------------*/
/*  HMCWDT Command Helpers                                   */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/*  HMCWDT: Get Timeout (remaining seconds)                  */
/*-----------------------------------------------------------*/
U32 hmcwdt_get_timeout(  )
{
    U64 us          = 0;
    if (sysblk.hmcwdt_expire_time > 0)
    {
        us = sysblk.hmcwdt_expire_time - hmcwdt_get_expire_time( 0 );

        return (U32)(us / SEC_TO_US(1));
    }

    /* timer is not active */
    return 0;

}

/*-----------------------------------------------------------*/
/* HMCWDT Command: $expire                                   */
/*-----------------------------------------------------------*/
/* return: 0: OK, 1: not active                              */
int hmcwdt_test_expire( U32 timeout )
{
    int rc;

    /* expire the active watchdog timer */
    OBTAIN_HMCWDT_LOCK();
    {
        if ( HMCWDT_IS_ENABLED_ACTIVE )
        {
            // Note: only get here for hmcwdt_cmd. Issue message
            // before the actual expiry happens
            WRMSG( HHC02204 , "I", "HMCWDT", "expired" );

            sysblk.hmcwdt_expire_time = 0;
            RELEASE_HMCWDT_LOCK();

            usleep( HMCWDT_US_SLEEP + HMCWDT_US_SLEEP/10 );     // 110% of timer thread

            OBTAIN_HMCWDT_LOCK();
            sysblk.hmcwdt_expire_time = hmcwdt_get_expire_time( timeout );

            rc = 0;
        }

        else
        {
            /* timer is not active */
            rc = 1;
        }
    }
    RELEASE_HMCWDT_LOCK();
    return rc;
}

/*-----------------------------------------------------------*/
/*  HMCWDT Command: cmds                                     */
/*-----------------------------------------------------------*/
/* return: 0: OK                                             */
/*        -1: enabled; cmds can not be null                  */
/*        -2: malloc error                                   */
/*        -3: cmds > HMCWDT_MAX_CMDLEN                       */
/*-----------------------------------------------------------*/
int hmcwdt_set_cmds( char* cmds )
{
    int cmdslen = 0;

    if (cmds != NULL)
        cmdslen = strlen( cmds );

    if ( cmdslen > HMCWDT_MAX_CMDLEN )
        return -3;

    OBTAIN_HMCWDT_LOCK();
    {
        if ( HMCWDT_IS_ENABLED && cmdslen == 0 )
        {
            // "Watchdog timer is enabled, commands cannot be empty"
            RELEASE_HMCWDT_LOCK();
            return -1;
        }

        /* already have cmds? */
        if ( sysblk.hmcwdt_cmds != NULL )
        {
            free( sysblk.hmcwdt_cmds );
            sysblk.hmcwdt_cmds = NULL;
        }

        /* set new cmds */
        if ( cmdslen > 0 )
        {
            sysblk.hmcwdt_cmds = malloc( cmdslen + 1 );
            if (sysblk.hmcwdt_cmds == NULL)
            {
                RELEASE_HMCWDT_LOCK();
                return -2;
            }
            strncpy( sysblk.hmcwdt_cmds, cmds, cmdslen );
            sysblk.hmcwdt_cmds[cmdslen] = '\0';
        }
    }
    RELEASE_HMCWDT_LOCK();
    return 0;
}

/*-----------------------------------------------------------*/
/*  HMCWDT Command: cmdsep                                   */
/*-----------------------------------------------------------*/
/* return: 0: OK                                             */
/*-----------------------------------------------------------*/
int hmcwdt_set_cmdsep( char cmdsep )
{
    OBTAIN_HMCWDT_LOCK();
    {
        sysblk.hmcwdt_cmdsep = cmdsep;
    }
    RELEASE_HMCWDT_LOCK();
    return 0;
}

/*-----------------------------------------------------------*/
/*   HMCWDT Command:  $enable                                */
/*-----------------------------------------------------------*/
/* return: 0: OK                                             */
/*        -1: disabled; cmds can not be null                 */
/*-----------------------------------------------------------*/
int hmcwdt_set_enabled( )
{
    int rc;

    OBTAIN_HMCWDT_LOCK();
    {
        if ( HMCWDT_IS_DISABLED && sysblk.hmcwdt_cmds == NULL )
        {
            //no commands have been defined; can not be enabled
            rc = -1;
        }

        else
        {
            sysblk.hmcwdt_enabled = true;
            rc = 0;
        }
    }
    RELEASE_HMCWDT_LOCK();
    return rc;
}


/*-----------------------------------------------------------*/
/*  HMCWDT Command: $disable                                 */
/*-----------------------------------------------------------*/
/* return: 0: OK                                             */
/*        -1: enabled-active, can not be disabled            */
/*-----------------------------------------------------------*/
int hmcwdt_set_disabled( )
{
    int rc;

    OBTAIN_HMCWDT_LOCK();
    {
        if ( HMCWDT_IS_ENABLED_ACTIVE )
        {
            // "Watchdog timer is enabled-active, can not be disabled"
            rc = -1;
        }

        else
        {
            sysblk.hmcwdt_enabled = false;
            rc = 0;
        }
    }
    RELEASE_HMCWDT_LOCK();
    return rc;
}

/*-----------------------------------------------------------*/
/*   HMCWDT Command: Status                                  */
/*-----------------------------------------------------------*/
/* return: 0: OK                                             */
/*-----------------------------------------------------------*/
int hmcwdt_show_status( )
{
    char buffer[1024];

    char status_timeout[128];
    char status_cmds[512];
    char status_cmdsep[32];
    char* status_state;

    OBTAIN_HMCWDT_LOCK();
    {
        status_state = HMCWDT_STATE_STR;

        // TIMEOUT
        if ( HMCWDT_IS_ENABLED_ACTIVE )
            sprintf(status_timeout, "timeout: \t%d seconds", hmcwdt_get_timeout() );
        else
            sprintf(status_timeout, "timeout: \t(none)" );

        // CMDSEP
        if ( sysblk.hmcwdt_cmdsep != '\0' )
            sprintf(status_cmdsep, "cmdsep: \t'%c'", sysblk.hmcwdt_cmdsep );
        else
            sprintf(status_cmdsep, "cmdsep: \t(none)" );

        // CMDS
        if ( sysblk.hmcwdt_cmds != NULL )
            sprintf(status_cmds, "cmds: \t\t\"%s\"", sysblk.hmcwdt_cmds );
        else
            sprintf(status_cmds, "cmds: \t\t(none)" );

        sprintf(buffer, "status \n\tstate:\t\t%s\n \t%s\n \t%s\n \t%s",
            status_state,
            status_timeout,
            status_cmdsep,
            status_cmds
            );
        WRMSG( HHC01959, "I", buffer);
    }
    RELEASE_HMCWDT_LOCK();
    return 0;
}