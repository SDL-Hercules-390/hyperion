/* logmsg.C     (C) Copyright Ivan Warren, 2003-2012                 */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*               logmsg frontend routing                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _LOGMSG_C_
#define _HUTIL_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*                Helper macro "BFR_VSNPRINTF"                       */
/*-------------------------------------------------------------------*/
/* Original design by Fish                                           */
/* Modified by Jay Maynard                                           */
/* Further modification by Ivan Warren                               */
/*                                                                   */
/* Purpose:                                                          */
/*                                                                   */
/*  set 'bfr' to contain a C string based on a message format        */
/*  and va_list of args. NOTE: 'bfr' must be free()d when done.      */
/*                                                                   */
/* Local variables referenced:                                       */
/*                                                                   */
/*  char*    bfr;     must be originally defined                     */
/*  int      siz;     must be defined and contain a start size       */
/*  va_list  vl;      must be defined and initialised with va_start  */
/*  char*    fmt;     is the message format                          */
/*  int       rc;     will contain final size                        */
/*                                                                   */
/*-------------------------------------------------------------------*/

#define  BFR_CHUNKSIZE    (256)

#define  BFR_VSNPRINTF()                                            \
                                                                    \
    do                                                              \
    {                                                               \
        va_list  original_vl;                                       \
        va_copy( original_vl, vl );                                 \
                                                                    \
        bfr = (char*) calloc( 1, siz );                             \
        rc = -1;                                                    \
                                                                    \
        while (bfr && rc < 0)                                       \
        {                                                           \
            rc = vsnprintf( bfr, siz, fmt, vl );                    \
                                                                    \
            if (rc >= 0 && rc < siz)                                \
                break;                                              \
                                                                    \
            rc = -1;                                                \
            siz += BFR_CHUNKSIZE;                                   \
                                                                    \
            if (siz > 65536)                                        \
                break;                                              \
                                                                    \
            bfr = realloc( bfr, siz );                              \
            va_copy( vl, original_vl );                             \
        }                                                           \
                                                                    \
        if (bfr && strlen( bfr ) == 0 && strlen( fmt) != 0)         \
        {                                                           \
            free( bfr );                                            \
            bfr = strdup( fmt );                                    \
        }                                                           \
        else                                                        \
        {                                                           \
            if (bfr)                                                \
            {                                                       \
                char* p = strdup( bfr );                            \
                free( bfr );                                        \
                bfr = p;                                            \
            }                                                       \
        }                                                           \
    }                                                               \
    while (0)

/*-------------------------------------------------------------------*/
/*       panel_command message capturing structs and vars            */
/*-------------------------------------------------------------------*/

typedef struct CAPTMSGS         // captured messages structure
{
    char*       msgs;           // pointer to captured messages
    size_t      szmsgs;         // size of captured messages buffer
}
CAPTMSGS;

typedef struct CAPTCTL          // message capturing control entry
{
    TID         tid;            // id of thread actively capturing
    CAPTMSGS*   captmsgs;       // ptr to captured messages struct
}
CAPTCTL;

static CAPTCTL  captctl_tab     [ MAX_CPU_ENGS + 4 ]   = {0};
static LOCK     captctl_lock;
static bool     wrmsg_quiet = false; // suppress panel output if true

#define  lock_capture()         obtain_lock(  &captctl_lock )
#define  unlock_capture()       release_lock( &captctl_lock );

/*-------------------------------------------------------------------*/
/*             panel_command message capturing functions             */
/*-------------------------------------------------------------------*/

static void InitCAPTCTL()
{
    static BYTE didthis = FALSE;
    if (didthis) return;
    didthis = TRUE;
    initialize_lock( &captctl_lock );
    memset( captctl_tab, 0, sizeof( captctl_tab ));
}

static CAPTCTL* FindCAPTCTL( TID tid )
{
    CAPTCTL*  pCAPTCTL;
    int       i;

    /* Search our capture control table for the desired entry */
    for (i=0, pCAPTCTL = captctl_tab; i < (int) _countof( captctl_tab ); i++, pCAPTCTL++)
    {
        if (equal_threads( pCAPTCTL->tid, tid ))
            return pCAPTCTL;
    }
    return NULL;
}

static CAPTCTL* NewCAPTCTL( TID tid, CAPTMSGS* pCAPTMSGS )
{
    CAPTCTL* pCAPTCTL;

    /* Find an available CAPTCTL entry */
    if (!(pCAPTCTL = FindCAPTCTL( (TID) 0 )))
        return NULL;

    /* Fill in the new entry */
    pCAPTCTL->tid      = tid;
    pCAPTCTL->captmsgs = pCAPTMSGS;

    return pCAPTCTL;
}

/*-------------------------------------------------------------------*/

static CAPTCTL* start_capturing( CAPTMSGS* pCAPTMSGS )
{
    CAPTCTL* pCAPTCTL;

    InitCAPTCTL();  // (in case it hasn't been done yet)

    lock_capture();
    {
        pCAPTCTL = NewCAPTCTL( thread_id(), pCAPTMSGS );
    }
    unlock_capture();

    return pCAPTCTL;
}

static void stop_capturing( CAPTCTL* pCAPTCTL )
{
    if (!pCAPTCTL)      // (if start capturing failed)
        return;         // (then no capturing to stop)

    /* Zero this capturing control table entry so it can be reused */
    lock_capture();
    {
        memset( pCAPTCTL, 0, sizeof( CAPTCTL ));
    }
    unlock_capture();
}

static void capture_message( const char* msg, CAPTMSGS* pCAPTMSGS )
{
    if (!msg || !*msg || !pCAPTMSGS)
        return;

    /* Initialize the captured messages buffer if needed */
    if (!pCAPTMSGS->msgs || !pCAPTMSGS->szmsgs)
    {
        pCAPTMSGS->msgs = malloc( pCAPTMSGS->szmsgs = 1 );
        *pCAPTMSGS->msgs = 0; // (null terminate)
    }

    /* Reallocate capture buffer to accommodate the new message */
    pCAPTMSGS->msgs = realloc( pCAPTMSGS->msgs, pCAPTMSGS->szmsgs += strlen( msg ));

    /* Add this new message to our buffer of captured messages */
    strlcat( pCAPTMSGS->msgs, msg, pCAPTMSGS->szmsgs );
}

/*-------------------------------------------------------------------*/
/*            Issue panel command and capture results                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int panel_command_capture( char* cmd, char** resp, bool quiet )
{
    int rc;
    CAPTCTL*  pCAPTCTL;             // ptr to capturing control entry
    CAPTMSGS  captmsgs  = {0};      // captured messages structure

    /* Start capturing */
    wrmsg_quiet = quiet;            // caller can suppress panel output
                                    // by setting quiet to true
    pCAPTCTL = start_capturing( &captmsgs );

    /* Execute the Hercules panel command and save the return code */
    rc = (int) (uintptr_t) panel_command( cmd );

    /* Pass any captured log messages back to the caller */
    *resp = captmsgs.msgs;

    /* Stop capturing */
    stop_capturing( pCAPTCTL );
    wrmsg_quiet = false;            // reinstate panel output

    /* Return to caller */
    return rc;
}

/*-------------------------------------------------------------------*/
/*               _flog_write_pipe  helper function                   */
/*-------------------------------------------------------------------*/
/* logger is non-blocking to prevent stalls so we try for a short    */
/* time to get the data through. Strictly log writes are atomic as   */
/* they should be < PIPE_BUF bytes (see pipe(7) but we do try and    */
/* handle the case where we are passed more data                     */
/*-------------------------------------------------------------------*/
static int do_write_pipe( int fd, const char *msg, int len)
{
    // Note: logger write fd is non-blocking
    int retry = 5;
    int written = 0;
    int rc;
    while ((rc = write_pipe( fd, msg, len )) != len && retry--)
    {
        if (rc == -1)
        {
            if (errno == EAGAIN)
            {
                // logger is backlogged; wait a bit before retrying
                USLEEP(10000);
                continue;
            }
            break;
        }
        if (rc < len)
        {
            // Short write (may occur if write >= PIPE_BUF)
            len -= rc;
            msg += rc;
            written += rc;
        }
    }
    return rc == -1 ? -1 : written + len;
}

/*-------------------------------------------------------------------*/
/* internal helper function:  write message to logger facility pipe  */
/*-------------------------------------------------------------------*/
static void _flog_write_pipe( FILE* f, const char* msg )
{
    /* Send message through logger facility pipe to panel.c,
       or display it directly to the terminal via fprintf
       if this is a utility message or we're shutting down
       or we're otherwise unable to send it through the pipe.
    */
    int rc = 0;
    int len = (int) strlen( msg );
    if (0
        || sysblk.shutdown
        || stdout != f
        || !logger_syslogfd[ LOG_WRITE ]
        || (rc = do_write_pipe( logger_syslogfd[ LOG_WRITE ], msg, len )) < 0
    )
    {
        // Something went wrong or we're shutting down.
        // Write the message to the screen instead...

        fprintf( f, "%s", msg );   /* (write msg to screen) */

        // PROGRAMMING NOTE: the external GUI receives messages
        // not only via its logfile stream but also via its stderr
        // stream too, so if there's an external gui, we skip the
        // logfile write in order to prevent duplicate messages.
        //
        // If no external GUI exists however, then we need to
        // write the message to the logfile WITH a timstamp.

        if (1
            && sysblk.shutdown      // (shutting down?)
            && !extgui              // (no external gui?)
        )
        {
            // (then we need to timestamp the logmsg...)
            // (note: call does nothing if no logfile exists!)

            logger_timestamped_logfile_write( msg, len );
        }
    }
}


/*-------------------------------------------------------------------*/
/*                           flog_write                              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The "flog_write" function is the function responsible for either  */
/* sending a formatted message through the Hercules logger facilty   */
/* pipe to panel.c for display to the user (handled by logger.c),    */
/* for optionally capturing messages issued by the DIAG8 interface,  */
/* or for printing the message directly to the terminal in the case  */
/* of utilities.                                                     */
/*                                                                   */
/*    'msg'        is the formatted message.                         */
/*    'panel'      indicates where the message should be sent:       */
/*                                                                   */
/*                                                                   */
/* WRMSG_CAPTURE   allows the message to be captured (if capturing   */
/*                 is active) but does not write it to the logger    */
/*                 facility pipe. Such messages are never shown to   */
/*                 the user.                                         */
/*                                                                   */
/* WRMSG_PANEL     only writes the message to the logger facility    */
/*                 pipe (to be shown to the user). Such messages     */
/*                 are never captured.                               */
/*                                                                   */
/* WRMSG_NORMAL    writes the message to the logger facility pipe    */
/*                 and captures the message as well (if capturing    */
/*                 is active).                                       */
/*                                                                   */
/*-------------------------------------------------------------------*/
static void flog_write( int panel, FILE* f, const char* msg )
{
    CAPTCTL* pCAPTCTL = NULL;       // (MUST be initialized to NULL)

    /* Retrieve capture control entry if capturing is allowed/active */
    if (panel & WRMSG_CAPTURE)
    {
        InitCAPTCTL();  // (in case it hasn't been done yet)

        /* Locate the capturing control entry for this thread */
        lock_capture();
        {
            pCAPTCTL = FindCAPTCTL( thread_id() );
        }
        unlock_capture();
    }

    /* If write to panel wanted, send message through logmsg pipe */
    if ((panel & WRMSG_PANEL) && !wrmsg_quiet)
        _flog_write_pipe( f, msg );

    /* Capture this message if capturing is active for this thread */
    if (pCAPTCTL)
        capture_message( msg, pCAPTCTL->captmsgs );
}

/*-------------------------------------------------------------------*/
/*                     writemsg functions                            */
/*-------------------------------------------------------------------*/
/* The writemsg function is the primary 'WRMSG' macro function that  */
/* is responsible for formatting messages which, once formatted, are */
/* then handed off to the flog_write function to be eventually shown */
/* to the user or captured or both, according to the 'panel' option. */
/*                                                                   */
/* If the 'debug' MSGLVL option is enabled, formatted messages will  */
/* be prefixed with the source filename and line number where they   */
/* originated from. Otherwise they are not.                          */
/*-------------------------------------------------------------------*/
static void vfwritemsg( BYTE panel, FILE* f,
                        const char* filename, int line, const char* func,
                        const char* fmt, va_list vl )
{
    char     prefix[ 32 ]  =  {0};
    char*    bfr           =  NULL;
    int      rc            =  1;
    int      siz           =  1024;
    char*    msgbuf;
    size_t   msglen, bufsiz;

  #ifdef NEED_LOGMSG_FFLUSH
    fflush( f );
  #endif

    // Format just the message part, without the filename and line number

    BFR_VSNPRINTF();  // Note: uses 'vl', 'bfr', 'siz', 'fmt' and 'rc'.
    if (!bfr)         // If BFR_VSNPRINTF runs out of memory,
        return;       // then there's nothing more we can do.

    bufsiz = msglen = strlen( bfr ) + 2;

    // Prefix message with filename and line number, if requested

    if (MLVL( DEBUG ))
    {
        char wrk[ 32 ];
        char *nl, *newbfr, *left, *right;
        size_t newsiz, pfxsiz, leftsize, rightsiz;

        MSGBUF( wrk, "%s(%d)", TRIMLOC( filename ), line );
        MSGBUF( prefix, MLVL_DEBUG_PFXFMT, wrk );

        pfxsiz = strlen( prefix );

        // Special handling for multi-line messages: insert the
        // debug prefix (prefix) before each line except the first
        // (which is is handled automatically further below)

        for (nl = strchr( bfr, '\n' ); nl && nl[1]; nl = strchr( nl, '\n' ))
        {
            left = bfr;
            right = nl+1;

            leftsize = ((nl+1) - bfr);
            rightsiz = bufsiz - leftsize;

            newsiz = bufsiz + pfxsiz;
            newbfr = malloc( newsiz );

            memcpy( newbfr, left, leftsize );
            memcpy( newbfr + leftsize, prefix, pfxsiz );
            memcpy( newbfr + leftsize + pfxsiz, right, rightsiz );

            free( bfr );
            bfr = newbfr;
            bufsiz = newsiz;
            nl = bfr + leftsize + pfxsiz;
        }

        bufsiz += pfxsiz;
    }

    msgbuf = calloc( 1, bufsiz );

    if (msgbuf)
    {
        snprintf( msgbuf, bufsiz, "%s%s", prefix, bfr );
        flog_write( panel, f, msgbuf );
        free( msgbuf );
    }

    // Show them where the error message came from, if requested

    if (1
        &&  MLVL( EMSGLOC )
        && !MLVL( DEBUG )
        && msglen > 10
        && strncasecmp( bfr, "HHC", 3 ) == 0
        && (0
            || 'S' == bfr[8]
            || 'E' == bfr[8]
            || 'W' == bfr[8]
           )
    )
    {
        // "Previous message from function '%s' at %s(%d)"
        FWRMSG( f, HHC00007, "I", func, TRIMLOC( filename ), line );
    }

    free( bfr );

  #ifdef NEED_LOGMSG_FFLUSH
    fflush( f );
  #endif

    log_wakeup( NULL );
}

/*-------------------------------------------------------------------*/
/* logmsg helper function: format message and pass to flog_write     */
/*-------------------------------------------------------------------*/
static void vflogmsg( BYTE panel, FILE* f, const char* fmt, va_list vl )
{
    char    *bfr =   NULL;
    int      rc;
    int      siz =   1024;

#ifdef NEED_LOGMSG_FFLUSH
    fflush(f);
#endif

    BFR_VSNPRINTF();  // Note: uses 'vl', 'bfr', 'siz', 'fmt' and 'rc'.
    if (!bfr)         // If BFR_VSNPRINTF runs out of memory,
        return;       // then there's nothing more we can do.

    flog_write( panel, f, bfr );

#ifdef NEED_LOGMSG_FFLUSH
    fflush(f);
#endif

    free( bfr );
}

/*-------------------------------------------------------------------*/
/*              primary public write message functions               */
/*-------------------------------------------------------------------*/
/* PROGRAMMING NOTE: these functions should NOT really ever be used  */
/* in a production environment. "logmsg" is a drop-in replacement    */
/* for "printf" and should only be used when debugging code still    */
/* under development. Production code should use the "WRMSG" macro   */
/* to appropriately call the below "fwritemsg" function.             */
/*-------------------------------------------------------------------*/

DLL_EXPORT void fwritemsg( const char* filename, int line, const char* func,
                           BYTE panel, FILE* f, const char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );
    vfwritemsg( panel, f, filename, line, func, fmt, vl );
}

DLL_EXPORT void logmsg( const char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );
    vflogmsg( WRMSG_NORMAL, stdout, fmt, vl );
}
