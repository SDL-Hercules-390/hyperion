/* LOGGER.C     (C) Copyright Jan Jaeger, 2003-2012                  */
/*              System logger functions                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* If standard output or standard error is redirected then the log   */
/* is written to the redirection.                                    */
/* If standard output and standard error are both redirected then    */
/* the system log is written to the redirection of standard error    */
/* the redirection of standard output is ignored in this case,       */
/* and background mode is assumed.                                   */

/* Any thread can determine background mode by inspecting stderr     */
/* for isatty()                                                      */

#include "hstdinc.h"

#define _LOGGER_C_
#define _HUTIL_DLL_

#include "hercules.h"
#include "opcode.h"                     /* Required for SETMODE      */

/*********************************************************************/
/* logger work variables                                             */
/*********************************************************************/

static COND  logger_cond;
static LOCK  logger_lock;

static int   logger_init_flg = FALSE;   /* reset by logger_init()    */

static char *logger_buffer;
static int   logger_bufsize;

static int   logger_currmsg;            /* index of most recent msg  */
static int   logger_wrapped;            /* msg buffer has wrapped    */

static int   logger_active = 0;         /* set by logger_thread,     */
                                        /* cleared by logger_term    */

static FILE *logger_syslog[2];          /* Syslog read/write pipe    */
       int   logger_syslogfd[2];        /*   pairs                   */
static FILE *logger_hrdcpy;             /* Hardcopy log or zero      */
static int   logger_hrdcpyfd;           /* Hardcopt fd or -1         */
static char  logger_filename[MAX_PATH];

/*********************************************************************/
/*              log_read  -  read system log                         */
/*********************************************************************/
/*                                                                   */
/* parameters:                                                       */
/*                                                                   */
/*   msg        pointer to char pointer. The char pointer will       */
/*              point to one or more messages upon return.           */
/*                                                                   */
/*   msgidx     index value used on multiple read requests. Pass     */
/*              a value of -1 on the first call to begin reading     */
/*              from the oldest message in the log. On the next      */
/*              call pass the updated value to retrieve the next     */
/*              group of messages.                                   */
/*                                                                   */
/*   block      LOG_NOBLOCK - non blocking request                   */
/*              LOG_BLOCK   - blocking request                       */
/* returns:                                                          */
/*                                                                   */
/*   length of msg data returned or zero.                            */
/*                                                                   */
/* PROGRAMMING NOTE:                                                 */
/*                                                                   */
/*   The string of message data that *msg points to when the return  */
/*   value is non-zero is NOT null terminated! (since it is in fact  */
/*   a pointer directly into the system's live message buffer). You  */
/*   should take care to NEVER attempt to call strlen on this value! */
/*                                                                   */
/*   A zero is returned whenever: a) no new messages have arrived    */
/*   since the last time the function was called, or b) the logger   */
/*   facility is no longer active (i.e. Hercules itself has ended).  */
/*                                                                   */
/*********************************************************************/
DLL_EXPORT int log_read( char** msg, int* msgidx, int block )
{
int bytes_returned;

    obtain_lock( &logger_lock );

    while (*msgidx == logger_currmsg && block)
    {
        if (logger_active)
        {
            wait_condition( &logger_cond, &logger_lock );
        }
        else
        {
            *msgidx = logger_currmsg;
            *msg = logger_buffer + logger_currmsg;
            release_lock( &logger_lock );
            return 0;
        }
    }

    if (*msgidx != logger_currmsg)
    {
        if (*msgidx < 0)
            *msgidx = logger_wrapped ? logger_currmsg : 0;

        if (*msgidx < 0 || *msgidx >= logger_bufsize)
            *msgidx = 0;

        *msg = logger_buffer + *msgidx;

        if (*msgidx >= logger_currmsg)
        {
            bytes_returned = logger_bufsize - *msgidx;
            *msgidx = 0;
        }
        else
        {
            bytes_returned = logger_currmsg - *msgidx;
            *msgidx = logger_currmsg;
        }
    }
    else
        bytes_returned = 0;

    release_lock( &logger_lock );

    return bytes_returned;
}

/*********************************************************************/
/*             log_line  -  find index for given line                */
/*********************************************************************/
/* Find the buffer index for a specific line, with the the value 1   */
/* indicating the last (most current) line, 2 meaning the last two   */
/* lines, 3 meaning the three most current lines, ... etc.           */
/*                                                                   */
/* Example:                                                          */
/*                                                                   */
/*   char*  msgbuf;                                                  */
/*   int    msglen;                                                  */
/*   int    msgidx;                                                  */
/*                                                                   */
/*   // read the last 5 lines of syslog...                           */
/*   msgidx = log_line(5);                                           */
/*   while ((msglen = log_read( &msgbuf, &msgidx, LOG_NOBLOCK )))    */
/*       function_to_process_log( msgbuf, msglen );                  */
/*                                                                   */
/*********************************************************************/
DLL_EXPORT int log_line(int linenumber)
{
char *msgbuf[2] = {NULL, NULL}, *tmpbuf = NULL;
int  msgidx[2] = { -1, -1 };
int  msglen[2] = {0, 0};
int  i;

    if(!linenumber++)
        return logger_currmsg;

    /* Find the last two blocks in the log */
    do {
        msgidx[0] = msgidx[1];
        msgbuf[0] = msgbuf[1];
        msglen[0] = msglen[1];
    } while((msglen[1] = log_read(&msgbuf[1], &msgidx[1], LOG_NOBLOCK)));

    for(i = 0; i < 2 && linenumber; i++)
        if(msgidx[i] != -1)
        {
            for(; linenumber > 0; linenumber--)
            {
                if(!(tmpbuf = (void *)memrchr(msgbuf[i],'\n',msglen[i])))
                    break;
                msglen[i] = (int)(tmpbuf - msgbuf[i]);
            }
            if(!linenumber)
                break;
        }

    while(i < 2 && tmpbuf && (*tmpbuf == '\r' || *tmpbuf == '\n'))
    {
        tmpbuf++;
        msglen[i]++;
    }

    return i ? msglen[i] + msgidx[0] : msglen[i];
}

#if !defined( _MSVC_ )

static int  saved_stdout;

static void logger_redirect()
{
    /* Save current stdout for later */
    saved_stdout = dup( STDOUT_FILENO );

    /* Redirect stdout to the logger */
    if (dup2( logger_syslogfd[ LOG_WRITE ], STDOUT_FILENO ) == -1)
    {
        if (logger_hrdcpy)
        {
            // "Logger: error in function %s: %s"
            fprintf( logger_hrdcpy, MSG( HHC02102, "E", "dup2()", strerror( errno )));
        }
        exit(1);
    }
}

DLL_EXPORT void logger_unredirect()
{
    /* Restore stdout */
    dup2(  saved_stdout, STDOUT_FILENO );
    close( saved_stdout );
}

#endif // !defined( _MSVC_ )

static void logger_term(void *arg)
{
    UNREFERENCED(arg);

    log_wakeup(NULL);
    usleep(1000);

    /* Flush all pending logger o/p before redirecting?? */
    fflush(stdout);

    if (logger_active)
    {
        /* Redirect all output to stderr */
        dup2(STDERR_FILENO, STDOUT_FILENO);

        /* Tell logger thread we want it to exit */
        logger_active = 0;
        log_wakeup(NULL);
        usleep(1000);

        if (sysblk.loggertid != 0 && !sysblk.shutdown)
        {
            sleep(2);
            /* Logger is now terminating */
            obtain_lock(&logger_lock);

            /* Wait for the logger to terminate */
            join_thread( sysblk.loggertid, NULL );
            detach_thread( sysblk.loggertid );

            release_lock(&logger_lock);

        }

        /* In external GUI mode, the external GUI will receive this
           message when the logger_thread writes it to the hardcopy
           file just before it exits. If we also issued it here too,
           it would end up receiving the message twice, so we skip
           issuing it here if we're running in external GUI mode.

           In all OTHER cases however (i.e. if we're NOT running in
           external GUI mode), it's unimportant to us whether or not
           it also gets written to the hardcopy file. We ALWAYS need
           to issue the message here so that it appears on the screen.
        */
        if (!daemon_task)
        {
            // "Thread id "TIDPAT", prio %2d, name %s ended"
            FWRMSG( stderr, HHC00101, "I", TID_CAST( thread_id()),
                get_thread_priority(), LOGGER_THREAD_NAME );
            fflush( stderr );
        }
    }
}

static void logger_logfile_write( const void* pBuff, size_t nBytes )
{
    char* pLeft = (char*) pBuff;
    int   nLeft = (int)   nBytes;

    /* daemon_mode only (wherein both stdout/stderr have both
       been redirected): don't write to hardcopy during shutdown
       to prevent duplicate messages from occurring when stderr
       is redirected to stdout (or vice versa) via the command
       line (e.g. 2>&1).

       The duplicate messages occur because during shutdown
       all messages are issued to stderr and we redirect stderr
       to stdout during shutdown.  This causes log messages
       (written to stdout) to be written to the logfile as well
       as to stderr too, which, due to our redirection, ends up
       being written again to the very same (hardcopy) file.
       The below test prevents this from happening.
    */
    if (nLeft && (!sysblk.daemon_mode || !sysblk.shutdown))
    {
        /* (ignore any errors; we did the best we could) */
        fwrite( pLeft, nLeft, 1, logger_hrdcpy );
    }

    if (sysblk.shutdown)
        fflush( logger_hrdcpy );
}

/* ZZ FIXME:
 * This should really be part of logmsg, as the timestamps have currently
 * the time when the logger reads the message from the log pipe.  There can be
 * quite a delay at times when there is a high system activity. Moving the timestamp
 * to logmsg() will fix this.
 * The timestamp option should also NOT depend on anything like daemon mode.
 * log entries should always be timestamped, in a fixed format, such that
 * log readers may decide to skip the timestamp when displaying (ie panel.c).
 */
static void logger_logfile_timestamp()
{
    if (!sysblk.daemon_mode)
    {
        struct timeval  now;
        time_t          tt;
        char            hhmmss[10];

        gettimeofday( &now, NULL ); tt = now.tv_sec;
        STRLCPY( hhmmss, ctime(&tt)+11 );
        logger_logfile_write( hhmmss, strlen(hhmmss) );
    }
}

DLL_EXPORT void logger_timestamped_logfile_write( const void* pBuff, size_t nBytes )
{
    if (logger_hrdcpy)
    {
        if (!sysblk.logoptnotime)
            logger_logfile_timestamp();
        logger_logfile_write( pBuff, nBytes );
    }
}

static void* logger_thread( void* arg )
{
    int bytes_read;

    UNREFERENCED( arg );

#if !defined( _MSVC_ )
    logger_redirect();
#endif

    setvbuf( stdout, NULL, _IONBF, 0 );

    obtain_lock( &logger_lock );
    {
        logger_active = 1;

        /* Signal initialization complete */
        signal_condition( &logger_cond );
    }
    release_lock( &logger_lock );

    /* ZZ FIXME:  We must empty the read pipe before we terminate */
    /* (Couldn't we just loop waiting for a 'select(,&readset,,,timeout)'
        to return zero?? Or use the 'poll' function similarly?? - Fish) */

    while (logger_active)
    {
        bytes_read = read_pipe( logger_syslogfd[ LOG_READ ], logger_buffer + logger_currmsg,
          ((logger_bufsize - logger_currmsg) > LOG_DEFSIZE ?
            LOG_DEFSIZE : logger_bufsize - logger_currmsg ));

        if (!bytes_read)        /* Has pipe been closed? */
            break;              /* Yes, then we are done */

        if (bytes_read < 0)
        {
            int read_pipe_errno = HSO_errno;

            /* Ignore any/all errors during shutdown */
            if (sysblk.shutdown)
                continue;

            if (HSO_EINTR == read_pipe_errno)
                continue;

            obtain_lock( &logger_lock );
            {
                if (logger_hrdcpy)
                {
                    // "Logger: error in function %s: %s"
                    fprintf( logger_hrdcpy, MSG( HHC02102, "E",
                        "read_pipe()", strerror( read_pipe_errno )));
                }
            }
            release_lock( &logger_lock );

            bytes_read = 0;
        }

        /* If Hercules is not running in daemon mode and panel
           initialization is not yet complete, write message
           to stderr so the user can see it on the terminal */
        if (!sysblk.daemon_mode)
        {
            if (!sysblk.panel_init)
            {
                char* pLeft2 = logger_buffer + logger_currmsg;
                int   nLeft2 = bytes_read;

                /* (ignore any errors; we did the best we could) */
                if (nLeft2)
                    fwrite( pLeft2, nLeft2, 1, stderr );
            }
        }

        obtain_lock( &logger_lock );
        {
            /* Write log data to hardcopy file */
            if (logger_hrdcpy)
            {
                /* Need to prefix each line with a timestamp. */

                static int needstamp = 1;
                char*  pLeft  = logger_buffer + logger_currmsg;
                int    nLeft  = bytes_read;
                char*  pRight = NULL;
                int    nRight = 0;
                char*  pNL    = NULL;   /* (pointer to NEWLINE character) */

                if (needstamp)
                {
                    if (!sysblk.logoptnotime)
                        logger_logfile_timestamp();
                    needstamp = 0;
                }

                while ((pNL = memchr( pLeft, '\n', nLeft )) != NULL)
                {
                    pRight  = pNL + 1;
                    nRight  = nLeft - ((int)(pRight - pLeft));
                    nLeft  -= nRight;

                    if (nLeft)
                        logger_logfile_write( pLeft, nLeft );

                    pLeft = pRight;
                    nLeft = nRight;

                    if (!nLeft)
                    {
                        needstamp = 1;
                        break;
                    }

                    if (!sysblk.logoptnotime)
                        logger_logfile_timestamp();
                }

                if (nLeft)
                    logger_logfile_write( pLeft, nLeft );
            }
        }
        release_lock( &logger_lock );

        /* Increment buffer index to next available position */
        logger_currmsg += bytes_read;

        if (logger_currmsg >= logger_bufsize)
        {
            logger_currmsg = 0;
            logger_wrapped = 1;
        }

        /* Notify all interested parties new log data is available */
        obtain_lock( &logger_lock );
        {
            broadcast_condition( &logger_cond );
        }
        release_lock( &logger_lock );

    } /* end while(logger_active) */

    logger_active = 0;
    sysblk.loggertid = 0;

    /* Logger is now terminating */
    obtain_lock( &logger_lock );
    {
        /* Write final message to hardcopy file */
        if (logger_hrdcpy)
        {
            char buf[128];
            // "Thread id "TIDPAT", prio %2d, name %s ended"
            MSGBUF( buf, MSG( HHC00101, "I", TID_CAST( thread_id()),
                get_thread_priority(), LOGGER_THREAD_NAME ));
            logger_timestamped_logfile_write( buf, strlen( buf ));
        }

        /* Redirect all msgs to stderr */
        logger_syslog   [ LOG_WRITE ] = stderr;
        logger_syslogfd [ LOG_WRITE ] = STDERR_FILENO;
        fflush( stderr );

        /* Signal any waiting tasks */
        broadcast_condition( &logger_cond );
    }
    release_lock( &logger_lock );

    return NULL;
}

DLL_EXPORT void logger_init( void )
{
    int rc;

    initialize_condition( &logger_cond );
    initialize_lock( &logger_lock );
    logger_init_flg = TRUE;

    obtain_lock( &logger_lock );

    if (0
        || fileno( stdin  ) >= 0
        || fileno( stdout ) >= 0
        || fileno( stderr ) >= 0
    )
    {
        logger_syslog[ LOG_WRITE ] = stderr;

        /* If standard error is redirected, then use standard error
           as the log file. */
        if (sysblk.daemon_mode)
        {
            STRLCPY( logger_filename, "STDOUT redirected from command line" );
            /* Ignore standard output to the extent that it is
               treated as standard error */
            logger_hrdcpyfd = dup( STDOUT_FILENO );

            if (dup2( STDERR_FILENO, STDOUT_FILENO ) == -1)
            {
                // "Logger: error in function %s: %s"
                fprintf( stderr, MSG( HHC02102, "E", "dup2()", strerror( errno )));
                exit(1);
            }
        }
        else
        {
            if (!isatty( STDOUT_FILENO ))
            {
                STRLCPY( logger_filename, "STDOUT redirected from command line" );
                logger_hrdcpyfd = dup( STDOUT_FILENO );

                if (dup2( STDERR_FILENO, STDOUT_FILENO ) == -1)
                {
                    // "Logger: error in function %s: %s"
                    fprintf( stderr, MSG( HHC02102, "E", "dup2()", strerror( errno )));
                    exit(1);
                }
            }
            else if (!isatty( STDERR_FILENO ))
            {
                STRLCPY( logger_filename, "STDERR redirected from command line" );
                logger_hrdcpyfd = dup( STDERR_FILENO );

                if (dup2( STDOUT_FILENO, STDERR_FILENO ) == -1)
                {
                    // "Logger: error in function %s: %s"
                    fprintf( stderr, MSG( HHC02102, "E", "dup2()", strerror( errno )));
                    exit(1);
                }
            }
        }

        if (logger_hrdcpyfd == -1)
        {
            logger_hrdcpyfd = 0;
            // "Logger: error in function %s: %s"
            fprintf( stderr, MSG( HHC02102, "E", "dup()", strerror( errno )));
        }

        if (logger_hrdcpyfd)
        {
            if (!(logger_hrdcpy = fdopen( logger_hrdcpyfd, "w" )))
                // "Logger: error in function %s: %s"
                fprintf( stderr, MSG( HHC02102, "E", "fdopen()", strerror( errno )));
        }

        if (logger_hrdcpy)
            setvbuf( logger_hrdcpy, NULL, _IONBF, 0 );
    }
    else
    {
        logger_syslog[ LOG_WRITE ] = fopen( "LOG", "a" );
    }

    logger_bufsize = LOG_DEFSIZE;

    if (!(logger_buffer = malloc( logger_bufsize )))
    {
        char buf[40];
        MSGBUF( buf, "malloc(%d)", logger_bufsize );
        // "Logger: error in function %s: %s"
        fprintf( stderr, MSG( HHC02102, "E", buf, strerror( errno )));
        exit(1);
    }

    if (create_pipe( logger_syslogfd ))
    {
        // "Logger: error in function %s: %s"
        fprintf( stderr, MSG( HHC02102, "E", "create_pipe()", strerror( errno )));
        exit(1);  /* Hercules running without syslog */
    }

    setvbuf( logger_syslog[ LOG_WRITE ], NULL, _IONBF, 0 );

    rc = create_thread( &sysblk.loggertid, JOINABLE,
                        logger_thread, NULL, LOGGER_THREAD_NAME );
    if (rc)
    {
        fprintf( stderr, MSG( HHC00102, "E", strerror( rc )));
        exit(1);
    }

    /* Wait for logger_thread to start */
    wait_condition( &logger_cond, &logger_lock );

    release_lock( &logger_lock );

    /* call logger_term on system shutdown */
    hdl_addshut( "logger_term", logger_term, NULL );

}

DLL_EXPORT char* log_dsphrdcpy( void )
{
    static char   buf[ MAX_PATH + 2 ];
    static char*  pzbuf = buf;

    if (!strchr( logger_filename, SPACE ))
        pzbuf = logger_filename;
    else
        MSGBUF( buf, "'%s'", logger_filename );

    return pzbuf;
}

DLL_EXPORT void log_sethrdcpy( const char* filename )
{
    FILE*  old_hrdcpy;

    /* If no filename specified then close the current logfile */
    if (!filename)
    {
        memset( logger_filename, 0, sizeof( logger_filename ));

        if (!logger_hrdcpy)
        {
            // "Logger: log not active"
            WRMSG( HHC02100, "E" );
        }
        else
        {
            /* Disable logging and close logfile */
            obtain_lock( &logger_lock );
            {
                old_hrdcpy      = logger_hrdcpy;
                logger_hrdcpy   = 0;
                logger_hrdcpyfd = 0;
            }
            release_lock( &logger_lock );

            // "Logger: log closed"
            fprintf( old_hrdcpy, MSG( HHC02101, "I" ));
            fclose(  old_hrdcpy );

            // "Logger: log closed"
            WRMSG( HHC02101, "I" );
        }
        return;
    }
    else
    {
        FILE*  new_hrdcpy;
        int    new_hrdcpyfd;
        char   pathname[ MAX_PATH ];

        /* Open the new logfile */

        hostpath( pathname, filename, sizeof( pathname ));

        memset( logger_filename, 0, sizeof( logger_filename ));

        new_hrdcpyfd = HOPEN( pathname,
                O_WRONLY | O_CREAT | O_TRUNC /* O_SYNC */,
                S_IRUSR  | S_IWUSR | S_IRGRP );

        if (new_hrdcpyfd < 0)
        {
            // "Logger: error in function %s: %s"
            WRMSG( HHC02102, "E","open()", strerror( errno ));
        }
        else
        {
            if (!(new_hrdcpy = fdopen( new_hrdcpyfd, "w" )))
            {
                // "Logger: error in function %s: %s"
                WRMSG( HHC02102,"E", "fdopen()", strerror( errno ));
                close( new_hrdcpyfd );
            }
            else
            {
                /* Set no buffering and switch to using new logfile */
                setvbuf( new_hrdcpy, NULL, _IONBF, 0 );

                obtain_lock( &logger_lock );
                {
                    old_hrdcpy      = logger_hrdcpy;
                    logger_hrdcpy   = new_hrdcpy;
                    logger_hrdcpyfd = new_hrdcpyfd;

                    STRLCPY( logger_filename, filename );
                }
                release_lock( &logger_lock );

                /* Write a message in the old logfile indicating
                   logging was switched to a different logfile,
                   and then close the old logfile afterwards.
                */
                if (old_hrdcpy)
                {
                    char buf[ MAX_PATH + 2 ];
                    const char* pzbuf = buf;

                    /* Quote the filename if it contains any blanks */
                    if (!strchr( filename, SPACE ))
                        pzbuf = filename;
                    else
                        MSGBUF( buf, "'%s'", filename );

                    // "Logger: log switched to %s"
                    fprintf( old_hrdcpy, MSG( HHC02104, "I", pzbuf ));
                    fclose(  old_hrdcpy );
                }
            }
        }
    }
}

/* log_wakeup - Wakeup any blocked threads.  Useful during shutdown. */
DLL_EXPORT void log_wakeup( void* arg )
{
    UNREFERENCED( arg );

    if (logger_init_flg)
    {
        obtain_lock( &logger_lock );
        {
            broadcast_condition( &logger_cond );
        }
        release_lock( &logger_lock );
    }
}

/* is logger active */
DLL_EXPORT int logger_isactive()
{
    return logger_active;
}
