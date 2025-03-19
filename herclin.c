/* HERCLIN.C    (C) Copyright Roger Bowler, 2005-2012                */
/*              (C) and others 2013-2021                             */
/*              Hercules Interface Control Program                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/************************************************/
/* (C) Copyright 2005-2009 Roger Bowler & Others*/
/* Initial author : Ivan Warren                 */
/*                                              */
/* HERCLIN.C                                    */
/*                                              */
/* THIS IS SAMPLE CODE DEMONSTRATING            */
/* THE USE OF THE INITIAL HARDWARE PANEL        */
/* INTERFACE FEATURE.                           */
/************************************************/

#include "hstdinc.h"
#include "hercules.h"

/**********************************************/
/* The following function is the LOG callback */
/* function. It gets called by the engine     */
/* whenever a log message needs to be         */
/* displayed. This function therefore         */
/* might be invoked from a separate thread    */
/**********************************************/

bool hercules_has_exited = false;

void log_callback( const char* msg, size_t msglen )
{
    UNREFERENCED( msg );

    if (!msglen)
    {
        hercules_has_exited = true;
        return;
    }

    fflush( stdout );
    fwrite( msg, 1, msglen, stdout );
    fflush( stdout );
    fflush( stderr );
}

/**********************************************/
/* Keyboard thread responsible for reading    */
/* user input from the keyboard. It's a sep-  */
/* arate thread because 'fgets' blocks until  */
/* the user enters a command and presses      */
/* the enter key.                             */
/**********************************************/

LOCK  kb_lock;      // keyboard lock
COND  kb_read_cond; // keyboard read requested
COND  kb_data_cond; // keyboard data available
TID   kb_tid;       // keyboard thread id
char *cmdbuf, *cmd; // keyboard variables

#define CMDBUFSIZ  1024

void* kb_thread( void* argp )
{
    UNREFERENCED( argp );

    while (1)
    {
        if ((cmd = fgets( cmdbuf, CMDBUFSIZ, stdin )))
        {
            obtain_lock( &kb_lock );
            {
                /* (remove newline) */
                cmd[ strlen( cmd ) - 1 ] = 0;

                broadcast_condition( &kb_data_cond );
                wait_condition( &kb_read_cond, &kb_lock );
            }
            release_lock( &kb_lock );
        }
    }

    UNREACHABLE_CODE( return NULL );
}

int main( int argc, char* argv[] )
{
    int rc;

    /*****************************************/
    /* COMMANDHANDLER is the function type   */
    /* of the engine's panel command handler */
    /* this MUST be resolved at run time     */
    /* since some HDL module might have      */
    /* redirected the initial engine function*/
    /*****************************************/

    COMMANDHANDLER  process_command;

#if defined( HDL_USE_LIBTOOL )
    /* LTDL Preloaded symbols for HDL using libtool */
    LTDL_SET_PRELOADED_SYMBOLS();
#endif

    /******************************************/
    /* Register the 'log_callback' function   */
    /* as the log message callback routine    */
    /******************************************/
    registerLogCallback( log_callback );

    /******************************************/
    /* Initialize the HERCULE Engine          */
    /******************************************/
    if ((rc = impl( argc, argv )) == 0)
    {
        sysblk.panel_init = 1;
        sysblk.herclin = 1;

        /******************************************/
        /* Get the command handler function       */
        /* This MUST be done after IML            */
        /******************************************/
        process_command = getCommandHandler();

        /******************************************/
        /* Create locks, conditions and buffers   */
        /* needed by the keyboard thread...       */
        /******************************************/

        initialize_lock( &kb_lock );
        initialize_condition( &kb_read_cond );
        initialize_condition( &kb_data_cond );

        cmdbuf = (char*) malloc( CMDBUFSIZ );

        /******************************************/
        /* Create a worker thread to read from    */
        /* the keyboard so we don't block forever */
        /* waiting for keyboard during shutdown.  */
        /******************************************/

        rc = create_thread( &kb_tid, DETACHED,
             kb_thread, NULL, HERCLIN_KB_THREAD );
        if (rc)
        {
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "S", strerror( rc ));
            return -1;
        }

        /******************************************/
        /* Read keyboard, pass to Command Handler */
        /******************************************/

        obtain_lock( &kb_lock );
        {
            while (!hercules_has_exited)
            {
                /* Request more keyboard data */
                broadcast_condition( &kb_read_cond );

                /* Wait for data to arrive */
                while
                (1
                    && !hercules_has_exited
                    && timed_wait_condition_relative_usecs
                       (
                           &kb_data_cond,
                           &kb_lock,
                           PANEL_REFRESH_RATE_FAST * 1000,
                           NULL
                       )
                       != 0
                )
                {
                    ;  // (do nothing! keep waiting!)
                }

                /* Process keyboard data */
                if (!hercules_has_exited)
                    process_command( cmd );
            }
        }
        release_lock( &kb_lock );

        sysblk.panel_init = 0;
    }

    return rc;
}
