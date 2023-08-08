/* PTTRACE.C    (C) Copyright Greg Smith, 2003-2013                  */
/*              Threading and locking trace debugger                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Pthread tracing functions                                         */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _PTTRACE_C_
#define _HUTIL_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Trace Table Entry                                                 */
/*-------------------------------------------------------------------*/
struct PTT_TRACE
{
    TID             tid;                /* Thread id                 */
    U64             trclass;            /* Trace class (see header)  */
    const char*     msg;                /* Trace message             */
    const void*     data1;              /* Data 1                    */
    const void*     data2;              /* Data 2                    */
    const char*     loc;                /* File name:line number     */
    struct timeval  tv;                 /* Time of day               */
    S64             rc;                 /* Return code               */
};
typedef struct PTT_TRACE PTT_TRACE;

/*-------------------------------------------------------------------*/
/* Trace classes table                                               */
/*-------------------------------------------------------------------*/
struct PTTCL
{
    const char*     name;               /* Trace class name. Should  */
                                        /* not start with chars "no" */
    U64             trcl;               /* Trace class               */
    int             alias;              /* 0: favored name, 1: alias */
};
typedef struct PTTCL PTTCL;

/*-------------------------------------------------------------------*/
/* Global variables                                                  */
/*-------------------------------------------------------------------*/
HLOCK      pttlock;                     /* Pthreads trace lock       */
DLL_EXPORT U64 pttclass  = 0;           /* Pthreads trace class      */
int        pttracen      = 0;           /* Number of table entries   */
int        pttracex      = 0;           /* Index of current entry    */
PTT_TRACE *pttrace       = NULL;        /* Pointer to current entry  */
int        pttnolock     = 0;           /* 1=no table locking        */
int        pttnotod      = 0;           /* 1=don't call gettimeofday */
int        pttnowrap     = 0;           /* 1=don't wrap              */
bool       pttdtax       = false;       /* true=dump table at exit   */
int        pttto         = 0;           /* timeout in seconds        */
COND       ptttocond;                   /* timeout thread condition  */
HLOCK      ptttolock;                   /* timeout thread lock       */
TID        ptttotid;                    /* timeout thread id         */
PTTCL      pttcltab[] =                 /* trace class names table   */
{
    /*  NOTE!  keywords "lock", "tod" and "wrap" are reserved        */
    /*         and must not be used for PTT trace class names!       */

    { "log"     , PTT_CL_LOG  , 0 },    /* Logger records            */
    { "tmr"     , PTT_CL_TMR  , 0 },    /* Timer/Clock records       */
    { "thr"     , PTT_CL_THR  , 0 },    /* Thread records            */
    { "inf"     , PTT_CL_INF  , 0 },    /* Instruction info          */
    { "err"     , PTT_CL_ERR  , 0 },    /* Instruction error/unsupp  */
    { "pgm"     , PTT_CL_PGM  , 0 },    /* Program interrupt         */
    { "csf"     , PTT_CL_CSF  , 0 },    /* Compare & Swap Failure    */
    { "sie"     , PTT_CL_SIE  , 0 },    /* Interpretive Execution    */
    { "sig"     , PTT_CL_SIG  , 0 },    /* SIGP signalling           */
    { "io"      , PTT_CL_IO   , 0 },    /* I/O                       */
#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
    { "txf"     , PTT_CL_TXF  , 0 },    /* Transact. Exec. Facility  */
#endif
    { "lcs1"    , PTT_CL_LCS1 , 0 },    /* LCS Timing Debug          */
    { "lcs2"    , PTT_CL_LCS2 , 0 },    /* LCS General Debugging     */
    { "qeth"    , PTT_CL_QETH , 0 },    /* QETH General Debugging    */
    { "xxx"     , PTT_CL_XXX  , 0 },    /* Undefined/generic/custom  */

    /* The following aliases are defined for backward compatibility  */

    { "logger"  , PTT_CL_LOG  , 1 },    /* alias for 'log'           */
    { "timer"   , PTT_CL_TMR  , 1 },    /* alias for 'tmr'           */
    { "threads" , PTT_CL_THR  , 1 },    /* alias for 'thr'           */
    { "control" , PTT_CL_INF  , 1 },    /* alias for 'inf'           */
    { "error"   , PTT_CL_ERR  , 1 },    /* alias for 'err'           */
    { "prog"    , PTT_CL_PGM  , 1 },    /* alias for 'pgm'           */
    { "inter"   , PTT_CL_CSF  , 1 },    /* alias for 'csf'           */
    { "signal"  , PTT_CL_SIG  , 1 },    /* alias for 'sig'           */
};

/*-------------------------------------------------------------------*/
/* Internal helper macros                                            */
/*-------------------------------------------------------------------*/
#define PTT_TRACE_SIZE          sizeof(PTT_TRACE)

#define OBTAIN_PTTLOCK                                               \
  do if (!pttnolock) {                                               \
    int rc = hthread_mutex_lock( &pttlock );                         \
    if (rc)                                                          \
      BREAK_INTO_DEBUGGER();                                         \
  }                                                                  \
  while (0)

#define RELEASE_PTTLOCK                                              \
  do if (!pttnolock) {                                               \
    int rc = hthread_mutex_unlock( &pttlock );                       \
    if (rc)                                                          \
      BREAK_INTO_DEBUGGER();                                         \
  }                                                                  \
  while (0)

/*-------------------------------------------------------------------*/
/* Trace classes table lookup and helper functions                   */
/*-------------------------------------------------------------------*/
static PTTCL* pttcl_byname( const char* name, int* no )
{
    /* Return class table entry matching "[no]name" or NULL. */
    /* "no" flag set on return indicates match on "[no]name" */
    size_t i;
    *no = (strlen(name) > 2 && strncasecmp(name, "no", 2) == 0);
    if (*no) name += 2;
    for (i=0; i < _countof( pttcltab ); i++)
        if (strcasecmp( pttcltab[i].name, name ) == 0)
            return &pttcltab[i];
    return NULL;
}

#if 0 // (this function is not needed yet)
static PTTCL* pttcl_byclass( U64 trcl )
{
    /* Return first class table entry matching the expression */
    /* (trcl & pttcltab[i].trcl) == pttcltab[i].trcl i.e. the */
    /* trcl passed may have multiple bits on but only first   */
    /* sequentially searched table entry found is returned.   */
    int  i;
    for (i=0; i < _countof( pttcltab ); i++)
        if ((trcl & pttcltab[i].trcl) == pttcltab[i].trcl)
            return &pttcltab[i];
    return NULL;
}
#endif // (not needed yet)

static char* pttcl_all( U64 trcl, char** ppStr )
{
    /* Return string of class names corresponding to trcl. */
    /* Bits are examined right to left (low order to high) */
    /* Bits corresponding to unassigned classes ignored.   */
    /* CALLER responsible for freeing returned *ppStr buf. */
    size_t i, sep = 0, bufsz = 1;
    if (*ppStr) free(*ppStr);
    if ((*ppStr = malloc( bufsz )) != NULL)
    {
        **ppStr = 0; /* start with empty string */
        for (i=0; i < _countof( pttcltab ) && !pttcltab[i].alias; i++)
        {
            if (!(trcl & pttcltab[i].trcl))
                continue;
            bufsz += sep + strlen( pttcltab[i].name );
            if (!(*ppStr = realloc( *ppStr, bufsz )))
                break;
            if (sep)
                strlcat( *ppStr, " ", bufsz );
            strlcat( *ppStr, pttcltab[i].name, bufsz );
            sep = 1;
        }
    }
    return *ppStr;
}

/*-------------------------------------------------------------------*/
/* Show the currently defined trace parameters                       */
/*-------------------------------------------------------------------*/
static void ptt_showparms()
{
    char* str = NULL;   /* PTT trace classes that are active. */

    /* Build string identifying trace classes that are active */
    pttcl_all( pttclass, &str );

    if (str)
    {
        // "Pttrace: %s %s %s %s %s to=%d %d"
        WRMSG
        (
            HHC90012, "D",
            str,
            pttnolock ? "nolock" : "lock",
            pttnotod  ? "notod"  : "tod",
            pttnowrap ? "nowrap" : "wrap",
            !pttdtax  ? "nodtax" : "dtax",
            pttto,
            pttracen
        );
        free( str );
    }
}

/*-------------------------------------------------------------------*/
/* Thread to print trace after timeout                               */
/*-------------------------------------------------------------------*/
static void* ptt_timeout( void* arg )
{
    static const char* thread_name = "ptt_timeout";
    struct timeval  now;
    struct timespec tm;

    UNREFERENCED( arg );

    SET_THREAD_NAME( thread_name );

    // "Thread id "TIDPAT", prio %2d, name %s started"
    LOG_THREAD_BEGIN( thread_name  );

    hthread_mutex_lock( &ptttolock );

    /* Wait for timeout period to expire */
    gettimeofday( &now, NULL );
    tm.tv_sec = now.tv_sec + pttto;
    tm.tv_nsec = now.tv_usec * 1000;
    hthread_cond_timedwait( &ptttocond, &ptttolock, &tm );

    /* Print the trace table automatically */
    if (hthread_equal( thread_id(), ptttotid ))
    {
        /* Show the parameters both before and after the table dump */
        ptt_showparms();
        if (ptt_pthread_print() > 0)
            ptt_showparms();
        pttto = 0;
        ptttotid = 0;
    }

    hthread_mutex_unlock( &ptttolock );

    // "Thread id "TIDPAT", prio %2d, name %s ended"
    LOG_THREAD_END( thread_name  );

    return NULL;
}

/*-------------------------------------------------------------------*/
/* ptt_dtax return Dump Table At Exit setting                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool ptt_dtax()
{
    return pttdtax; // Dump Table At Exit
}

/*-------------------------------------------------------------------*/
/* Process 'ptt' tracing command                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int ptt_cmd( int argc, char* argv[], char* cmdline )
{
    int  rc = 0;
    int  n, to = -1;
    char c;
    PTTCL* pPTTCL;
    int no;

    UNREFERENCED( cmdline );

    if (argc > 1)
    {
        int showparms = 0;

        /* process arguments; last arg can be trace table size */
        for (--argc, argv++; argc; --argc, ++argv)
        {
            if (strcasecmp("opts", argv[0]) == 0)
                continue;

            /* Try to find this trace class in our table */
            pPTTCL = pttcl_byname( argv[0], &no );

            /* Enable/disable tracing for class if found */
            if (pPTTCL)
            {
                U64 trcl = pPTTCL->trcl;
                if (no)
                    pttclass &= ~trcl;
                else
                    pttclass |=  trcl;
                continue;
            }
            /* Check if other valid PTT command argument */
            else if (strcasecmp("?", argv[0]) == 0)
            {
                showparms = 1;
            }
            else if (strcasecmp("dtax", argv[0]) == 0)
            {
                pttdtax = true;
                continue;
            }
            else if (strcasecmp("nodtax", argv[0]) == 0)
            {
                pttdtax = false;
                continue;
            }
            else if (strcasecmp("lock", argv[0]) == 0)
            {
                pttnolock = 0;
                continue;
            }
            else if (strcasecmp("nolock", argv[0]) == 0)
            {
                pttnolock = 1;
                continue;
            }
            else if (strcasecmp("tod", argv[0]) == 0)
            {
                pttnotod = 0;
                continue;
            }
            else if (strcasecmp("notod", argv[0]) == 0)
            {
                pttnotod = 1;
                continue;
            }
            else if (strcasecmp("wrap", argv[0]) == 0)
            {
                pttnowrap = 0;
                continue;
            }
            else if (strcasecmp("nowrap", argv[0]) == 0)
            {
                pttnowrap = 1;
                continue;
            }
            else if (strncasecmp("to=", argv[0], 3) == 0 && strlen(argv[0]) > 3
                  && (sscanf(&argv[0][3], "%d%c", &to, &c) == 1 && to >= 0))
            {
                pttto = to;
                continue;
            }
            else if (argc == 1 && sscanf(argv[0], "%d%c", &n, &c) == 1 && n >= 0)
            {
                OBTAIN_PTTLOCK;
                if (pttracen == 0)
                {
                    if (pttrace != NULL)
                    {
                        RELEASE_PTTLOCK;
                        // "Pttrace: trace is busy"
                        WRMSG(HHC90010, "E");
                        return -1;
                    }
                }
                else if (pttrace)
                {
                    pttracen = 0;
                    RELEASE_PTTLOCK;
                    USLEEP(1000);
                    OBTAIN_PTTLOCK;
                    free (pttrace);
                    pttrace = NULL;
                }
                ptt_trace_init( n, FALSE );
                RELEASE_PTTLOCK;
            }
            else
            {
                // "Pttrace: invalid argument %s"
                WRMSG(HHC90011, "E", argv[0]);
                rc = -1;
                break;
            }
        } /* for each ptt argument */

        /* wakeup timeout thread if to= specified */
        if (to >= 0 && ptttotid)
        {
            hthread_mutex_lock (&ptttolock);
            ptttotid = 0;
            hthread_cond_signal (&ptttocond);
            hthread_mutex_unlock (&ptttolock);
        }

        /* start timeout thread if positive to= specified */
        if (to > 0)
        {
            hthread_mutex_lock( &ptttolock );
            ptttotid = 0;
            rc = hthread_create( &ptttotid, NULL, ptt_timeout, NULL );
            if (rc)
                // "Error in function create_thread(): %s"
                WRMSG( HHC00102, "E", strerror( rc ));
            hthread_mutex_unlock( &ptttolock );
        }

        if (showparms)
            ptt_showparms();
    }
    else /* No arguments. Dump the table. */
    {
        /* Dump table when tracing is active (number of entries > 0) */
        if (pttracen)
        {
            /* Show parameters both before and after the table dump  */
            ptt_showparms();
            if (ptt_pthread_print() > 0)
                ptt_showparms();
        }
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Initialize PTT tracing                                            */
/*-------------------------------------------------------------------*/
DLL_EXPORT void ptt_trace_init( int nTableSize, BOOL init )
{
    if (nTableSize > 0)
        pttrace = calloc( nTableSize, PTT_TRACE_SIZE );
    else
        pttrace = NULL;

    pttracen = pttrace ? nTableSize : 0;
    pttracex = 0;

    if (init)       /* First time? */
    {
        int rc;
        MATTR attr;
        if ((rc = hthread_mutexattr_init( &attr )) != 0)
            BREAK_INTO_DEBUGGER();
        if ((rc = hthread_mutexattr_settype( &attr, HTHREAD_MUTEX_DEFAULT )) != 0)
            BREAK_INTO_DEBUGGER();
        if ((rc = hthread_mutex_init( &pttlock, &attr )) != 0)
            BREAK_INTO_DEBUGGER();
        if ((rc = hthread_mutex_init( &ptttolock, &attr )) != 0)
            BREAK_INTO_DEBUGGER();
        if ((rc = hthread_cond_init( &ptttocond )) != 0)
            BREAK_INTO_DEBUGGER();
        if ((rc = hthread_mutexattr_destroy( &attr )) != 0)
            BREAK_INTO_DEBUGGER();
        pttnolock = 0;
        pttnotod  = 0;
        pttnowrap = 0;
        pttto     = 0;
        ptttotid  = 0;
    }
}

/*-------------------------------------------------------------------*/
/* Primary PTT tracing function to fill in a PTT_TRACE table entry.  */
/*-------------------------------------------------------------------*/
DLL_EXPORT void ptt_pthread_trace (U64 trclass, const char *msg,
                                   const void *data1, const void *data2,
                                   const char *loc, S64 rc, TIMEVAL* pTV)
{
int i, n;

    if (pttrace == NULL || pttracen == 0 || !(pttclass & trclass)) return;
    /*
     * Messages from timer.c, clock.c and/or logger.c are not usually
     * that interesting and take up table space.  Check the flags to
     * see if we want to trace them.
     */
    loc = TRIMLOC( loc );
    if (!(pttclass & PTT_CL_TMR) && !strncasecmp( loc, "timer.c:",  8)) return;
    if (!(pttclass & PTT_CL_TMR) && !strncasecmp( loc, "clock.c:",  8)) return;
    if (!(pttclass & PTT_CL_LOG) && !strncasecmp( loc, "logger.c:", 9)) return;
    if (!(pttclass & PTT_CL_LOG) && !strncasecmp( loc, "logmsg.c:", 9)) return;

    /* Check for 'nowrap' */
    if (pttnowrap && pttracex + 1 >= pttracen) return;

    /* Consume another trace table entry */
    OBTAIN_PTTLOCK;
    if (pttrace == NULL || (n = pttracen) == 0)
    {
        RELEASE_PTTLOCK;
        return;
    }
    i = pttracex++;
    if (pttracex >= n)
        pttracex = 0;
    RELEASE_PTTLOCK;

    /* Fill in the trace table entry */
    if (pttnotod == 0)
    {
        if (pTV)
            memcpy( &pttrace[i].tv, pTV, sizeof( TIMEVAL ));
        else
            gettimeofday( &pttrace[i].tv, NULL );
    }
    pttrace[i].tid     = thread_id();
    pttrace[i].trclass = trclass;
    pttrace[i].msg     = msg;
    pttrace[i].data1   = data1;
    pttrace[i].data2   = data2;
    pttrace[i].loc     = loc;
    pttrace[i].rc      = rc;
}

/*-------------------------------------------------------------------*/
/* Function to print all PTT_TRACE table entries.                    */
/* Return code is the  #of table entries printed.                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int ptt_pthread_print ()
{
int   i, n, count = 0;
char  retcode[32]; // (retcode is 'int'; if x64, 19 digits or more!)
char  tod[27];     // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    if (pttrace && pttracen)
    {
        /* Temporarily disable tracing by indicating an empty table */
        OBTAIN_PTTLOCK;
        n = pttracen;       /* save number of trace table entries   */
        pttracen = 0;       /* indicate empty table to stop tracing */
        RELEASE_PTTLOCK;

        /* Print the trace table */
        i = pttracex;
        do
        {
            if (pttrace[i].tid)
            {
                char threadname[16];
                char lockname[32];
                const char* lname;

                FormatTIMEVAL( &pttrace[i].tv, tod, sizeof( tod ));
                get_thread_name( pttrace[i].tid, threadname );

                if (pttrace[i].trclass & PTT_CL_THR)
                {
                    /* For the thread class, an 'rc' of PTT_MAGIC
                       indicates its value is uninteresting to us,
                       so we don't bother showing it. Otherwise we
                       format it as a +/- decimal value.
                    */
                    if (pttrace[i].rc == PTT_MAGIC)
                        retcode[0] = 0;
                    else
                        MSGBUF( retcode, "%"PRId64, pttrace[i].rc );
                }
                else
                {
                    /* Not thread class: format return code
                       as just another 64-bit hex value.
                    */
                    MSGBUF( retcode, "%16.16"PRIx64, pttrace[i].rc );
                }

                /* If this is the thread class we know the data1 value
                   is USUALLY the address of the lock identifying which
                   lock was being obtained/released, so as a courtesy
                   we display its name after the message.  This might
                   not always work as the data1 value for SOME thread
                   trace entries might be NULL or be some other value.
                */
                lname = (pttrace[i].trclass & PTT_CL_THR) ?
                    get_lock_name( (LOCK*) pttrace[i].data1 ) : "";

                MSGBUF( lockname, "%s%s", lname[0] ? " " : "", lname );

                if (lockname[0] && !retcode[0])
                    retcode[0] = ' ', retcode[1] = 0;

                // "%s "TIDPAT" %-15.15s %-18.18s %-18.18s"PTR_FMTx" "PTR_FMTx" %s%s"
                WRMSG( HHC90021, "D"
                    , &tod[11]                          // Time of day (HH:MM:SS.usecs)
                    , TID_CAST( pttrace[i].tid )        // Thread id
                    , threadname                        // Thread name
                    , pttrace[i].loc                    // File name (string; 18 chars)
                    , pttrace[i].msg                    // Trace message (string; 18 chars)
                    , PTR_CAST( pttrace[i].data1 )      // Data value 1
                    , PTR_CAST( pttrace[i].data2 )      // Data value 2
                    , retcode                           // Return code (or empty string)
                    , lockname                          // Lock name   (or empty string)
                );

                count++;
            }
            if (++i >= n) i = 0;
        } while (i != pttracex);

        /* Clear all the table entries we just printed and
           enable tracing again starting at entry number 0.
           NOTE: there is no need to obtain the lock since:
           a) pttracex is never accessed unless pttracen is
           non-zero, b) because pttracen is an int, setting
           it to non-zero again should be atomic.
        */
        memset( pttrace, 0, PTT_TRACE_SIZE * n );
        pttracex = 0;
        pttracen = n;
    }

    return count;
}
