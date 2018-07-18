/* HERCLIN.C     (C) Copyright Roger Bowler, 2005-2012               */
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

int hercules_has_exited = 0;

void log_callback( const char* msg, size_t msglen )
{
    UNREFERENCED( msg );

    if (!msglen)
    {
        hercules_has_exited = 1;
        return;
    }

    fflush( stdout );
    fwrite( msg, 1, msglen, stdout );
    fflush( stdout );
    fflush( stderr );
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

    char *cmdbuf, *cmd;

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

        /******************************************/
        /* Get the command handler function       */
        /* This MUST be done after IML            */
        /******************************************/
        process_command = getCommandHandler();

        /******************************************/
        /* Read STDIN and pass to Command Handler */
        /******************************************/
#define CMDBUFSIZ  1024
        cmdbuf = (char*) malloc( CMDBUFSIZ );

        do
        {
            if ((cmd = fgets( cmdbuf, CMDBUFSIZ, stdin )))
            {
                /* (remove newline) */
                cmd[ strlen( cmd ) - 1 ] = 0;

                process_command( cmd );
            }
        }
        while (!hercules_has_exited);

        sysblk.panel_init = 0;
    }

    return rc;
}
