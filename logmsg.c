/* logmsg.C     (c) Copyright Ivan Warren, 2003-2012                 */
/*              (c) Copyright TurboHercules, SAS 2010-2011           */
/*               logmsg frontend routing                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HUTIL_DLL_
#define _LOGMSG_C_

#include "hercules.h"

#define  BFR_CHUNKSIZE    (256)

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
#if defined(_MSVC_)

//  PROGRAMMING NOTE: the only difference between the MSVC version
//  and *nix version of the below "BFR_VSNPRINTF" macro is the MSVC
//  version uses "vsnprintf_s" and the *nix version uses "vsnprintf".

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
            rc = _vsnprintf_s( bfr, siz, siz-1, fmt, vl );          \
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
        if (bfr != NULL && strlen(bfr) == 0 && strlen(fmt) != 0)    \
        {                                                           \
            free( bfr );                                            \
            bfr = strdup( fmt );                                    \
        }                                                           \
        else                                                        \
        {                                                           \
            if (bfr != NULL)                                        \
            {                                                       \
                char* p = strdup( bfr );                            \
                free( bfr );                                        \
                bfr = p;                                            \
            }                                                       \
        }                                                           \
    }                                                               \
    while (0)

#else

//  PROGRAMMING NOTE: the only difference between the *nix version
//  and MSVC version of the below "BFR_VSNPRINTF" macro is the *nix
//  version uses "vsnprintf" and the MSVC version uses "vsnprintf_s".

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
        if (bfr != NULL && strlen(bfr) == 0 && strlen(fmt) != 0)    \
        {                                                           \
            free( bfr );                                            \
            bfr = strdup( fmt );                                    \
        }                                                           \
        else                                                        \
        {                                                           \
            if (bfr != NULL)                                        \
            {                                                       \
                char* p = strdup( bfr );                            \
                free( bfr );                                        \
                bfr = p;                                            \
            }                                                       \
        }                                                           \
    }                                                               \
    while (0)

#endif

/*-------------------------------------------------------------------*/
/*            log message capturing routing functions                */
/*-------------------------------------------------------------------*/

struct LOG_ROUTES
{
    TID          t;     // thread id
    LOG_WRITER  *w;     // ptr to log routing writer func
    LOG_CLOSER  *c;     // ptr to log routing closer func
    void        *u;     // user context passed to each func
};
typedef struct LOG_ROUTES   LOG_ROUTES;

#define MAX_LOG_ROUTES (MAX_CPU_ENGINES + 4)
LOG_ROUTES  log_routes[ MAX_LOG_ROUTES ];

static LOCK  log_route_lock;
static int   log_route_inited = 0;

static void log_route_init()
{
    int i;

    if (log_route_inited)
        return;

    initialize_lock( &log_route_lock );

    for (i=0; i < MAX_LOG_ROUTES; i++)
    {
        log_routes[i].t  =  0;
        log_routes[i].w  =  NULL;
        log_routes[i].c  =  NULL;
        log_routes[i].u  =  NULL;
    }

    log_route_inited = 1;
}

static int log_route_search( TID t )
{
    int  i;
    for (i=0; i < MAX_LOG_ROUTES; i++)
    {
        if (equal_threads( log_routes[i].t, t ))
        {
            if (t == 0)
                log_routes[i].t = (TID) 1;

            return (i);
        }
    }
    return (-1);
}

/* Open a log redirection Driver route on a per-thread basis         */
/* Up to 16 concurent threads may have an alternate logging route    */
/* opened                                                            */
static int log_open( LOG_WRITER* lw, LOG_CLOSER* lc, void* uw )
{
    int slot;

    log_route_init();

    obtain_lock( &log_route_lock );
    {
        slot = log_route_search( (TID) 0 );

        if (slot < 0)
        {
            release_lock( &log_route_lock );
            return (-1);
        }

        log_routes[slot].t  =  thread_id();
        log_routes[slot].w  =  lw;
        log_routes[slot].c  =  lc;
        log_routes[slot].u  =  uw;
    }
    release_lock( &log_route_lock );
    return (0);
}

struct log_capture_data
{
    char*   obfr;       // ptr to captured message buffer
    size_t  sz;         // size of captured message buffer
};

static void log_capture_writer(void *vcd,char *msg)
{
    struct log_capture_data *cd;
    if(!vcd||!msg)return;
    cd=(struct log_capture_data *)vcd;
    if(cd->sz==0)
    {
        cd->sz=strlen(msg)+1;
        cd->obfr=malloc(cd->sz);
        cd->obfr[0]=0;
    }
    else
    {
        cd->sz+=strlen(msg);
        cd->obfr=realloc(cd->obfr,cd->sz);
    }
    strlcat(cd->obfr,msg,cd->sz);
    return;
}

static void log_close()
{
    int slot;

    log_route_init();

    obtain_lock( &log_route_lock );
    {
        slot = log_route_search( thread_id() );

        if (slot < 0)
        {
            release_lock( &log_route_lock );
            return;
        }

        // Call the Log Routing closer function
        log_routes[slot].c( log_routes[slot].u );

        log_routes[slot].t  =  0;
        log_routes[slot].w  =  NULL;
        log_routes[slot].c  =  NULL;
        log_routes[slot].u  =  NULL;
    }
    release_lock( &log_route_lock );
}

static void log_capture_closer(void *vcd)
{
    UNREFERENCED(vcd);
    return;
}

/*-------------------------------------------------------------------*/
/*                log message capturing functions                    */
/*-------------------------------------------------------------------*/
/* Capture log output routines. log_capture is a sample of how to    */
/* use log rerouting. log_capture takes 2 arguments: a ptr to the    */
/* capture function that takes 1 parameter and the parameter itself  */
/*-------------------------------------------------------------------*/

DLL_EXPORT char *log_capture(CAPTUREFUNC *func,void *p)
{
    struct log_capture_data cd;
    cd.obfr=NULL;
    cd.sz=0;
    log_open(log_capture_writer,log_capture_closer,&cd);
    func(p);
    log_close();
    return(cd.obfr);
}

DLL_EXPORT int log_capture_rc(CAPTUREFUNC *func,char *p,char **resp)
{
    int rc;
    struct log_capture_data cd = {0,0};
    log_open(log_capture_writer,log_capture_closer,&cd);
    rc = (int)(uintptr_t)func(p);
    log_close();
    *resp = cd.obfr;
    return rc;
}

/*-------------------------------------------------------------------*/
/*                     writemsg functions                            */
/*-------------------------------------------------------------------*/
/* The writemsg function is the primary 'WRMSG' macro function that  */
/* is responsible for formatting messages. Messages, once formatted  */
/* are then handed off to the log_write function to be eventually    */
/* displayed to the user or captured or both. (Refer to log_write)   */
/*                                                                   */
/* If the 'debug' MSGLVL option is enabled, formatted messages will  */
/* be prefixed with the source filename and line number where they   */
/* originated from.                                                  */
/*-------------------------------------------------------------------*/

static void vfwritemsg( FILE* f, const char* filename, int line, const char* func, const char* fmt, va_list vl )
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
        snprintf( msgbuf, bufsiz-1, "%s%s\n", prefix, bfr );
        flog_write( f, 0, msgbuf );
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

DLL_EXPORT void writemsg( const char* filename, int line, const char* func, const char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );
    vfwritemsg( stdout, filename, line, func, fmt, vl );
}

DLL_EXPORT void fwritemsg( FILE* f, const char* filename, int line, const char* func, const char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );
    vfwritemsg( f, filename, line, func, fmt, vl );
}

/*-------------------------------------------------------------------*/
/*            internal flog_write helper function                    */
/*-------------------------------------------------------------------*/
static void _flog_write_pipe( FILE* f, char* msg )
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
        || (rc = write_pipe( logger_syslogfd[ LOG_WRITE ], msg, len )) < 0
    )
    {
        fprintf( f, "%s", msg );   /* (write msg to screen) */

        if (1
            && sysblk.shutdown
#ifdef EXTERNALGUI
            // PROGRAMMING NOTE: the external GUI receives messages
            // not only via its logfile stream but also via its
            // stderr stream as well, so we skip the logfile write
            // in order to prevent duplicate messages.
            && !extgui
#endif
        )
        {
            // Note: call does nothing if no logfile exists.
            logger_timestamped_logfile_write( msg, len );
        }
    }
}

/*-------------------------------------------------------------------*/
/*                     log_write functions                           */
/*-------------------------------------------------------------------*/
/* The "log_write" function is the function responsible for either   */
/* sending a formatted message through the Hercules logger facilty   */
/* pipe (handled by logger.c) to panel.c for display to the user,    */
/* or for printing the message directly to the terminal screen (in   */
/* the case of utilities).                                           */
/*                                                                   */
/* 'msg' is the formatted message to be displayed. 'panel' tells     */
/* where the message should be sent: the value '1' (normal) sends    */
/* the message through the logger facility pipe only (for display    */
/* to the user via panel.c). Messages written using panel=1 can      */
/* never be captured. (See log message capturing functions further   */
/* above). using panel=2 allows a message to be displayed to the     */
/* user AND be captured as well. (It's sent through the logger pipe  */
/* to panel.c AND is captured too.) The value '0' is used when you   */
/* ONLY want to silently capture a message WITHOUT displaying it to  */
/* the user. Such messages are NEVER sent through the logger pipe    */
/* and thus never reach panel.c                                      */
/*                                                                   */
/* SUMMARY: panel=0: capture only, 1=panel only, 2=panel and capture */
/*-------------------------------------------------------------------*/
DLL_EXPORT void flog_write( FILE* f, int panel, char* msg )
{
    int slot;

    log_route_init();

    if (panel == 1)     /* Display message only; NEVER capture */
    {
        _flog_write_pipe( f, msg );
        return;   /* panel=1: NEVER capture; return immediately */
    }

    /* Retrieve message capture routing slot */
    obtain_lock( &log_route_lock );
    {
        slot = log_route_search( thread_id() );
    }
    release_lock( &log_route_lock );

    if (slot < 0 || panel > 0) /* Capture only or display & capture */
    {
        _flog_write_pipe( f, msg );
        if (slot < 0)
            return;
    }

    /* Allow message to be captured */
    log_routes[ slot ].w( log_routes[slot].u, msg );
}

DLL_EXPORT void log_write( int panel, char *msg )
{
    flog_write( stdout, panel, msg );
}

/*-------------------------------------------------------------------*/
/* logmsg functions: normal panel or buffer routing as appropriate.  */
/*-------------------------------------------------------------------*/
/* PROGRAMMING NOTE: these functions should NOT really ever be used  */
/* in a production environment. They only still exist because they   */
/* are easier to use than our 'WRMSG' macro. "logmsg" is a drop-in   */
/* replacement for "printf" and should only be used for debugging    */
/* while developing new code. Production code should use "WRMSG".    */
/*-------------------------------------------------------------------*/
static void vflogmsg( FILE* f, char* fmt, va_list vl )
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

    flog_write( f, 0, bfr );

#ifdef NEED_LOGMSG_FFLUSH
    fflush(f);
#endif

    free( bfr );
}

DLL_EXPORT void flogmsg( FILE* f, char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );
    vflogmsg( f, fmt, vl );
}

DLL_EXPORT void logmsg( char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );
    vflogmsg( stdout, fmt, vl );
}
