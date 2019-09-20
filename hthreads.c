/* HTHREADS.C   (C) Copyright "Fish" (David B. Trout), 2013          */
/*              Hercules locking and threading                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Based on original pttrace.c (C) Copyright Greg Smith, 2003-2013   */
/* Collaboratively enhanced and extended in May-June 2013            */
/* by Mark L. Gaubatz and "Fish" (David B. Trout)                    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HTHREAD_C_
#define _HUTIL_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Hercules Internal ILOCK structure                                 */
/*-------------------------------------------------------------------*/
struct ILOCK                    /* Hercules internal ILOCK structure */
{
    LIST_ENTRY   locklink;      /* links locks together in a chain   */
    void*        addr;          /* Lock address                      */
    const char*  name;          /* Lock name                         */
    const char*  location;      /* Location where it was obtained    */
    TIMEVAL      time;          /* Time of day when it was obtained  */
    TID          tid;           /* Thread-Id of who obtained it      */
    HLOCK        locklock;      /* Internal ILOCK structure lock     */
    union      {
    HLOCK        lock;          /* The actual locking model mutex    */
    HRWLOCK      rwlock;        /* The actual locking model rwlock   */
               };
};
typedef struct ILOCK ILOCK;     /* Shorter name for the same thing   */

/*-------------------------------------------------------------------*/
/* Internal PTT trace helper macros                                  */
/*-------------------------------------------------------------------*/
#define PTTRACE(_msg,_data1,_data2,_loc,_result)                      \
  do {                                                                \
    if (pttclass & PTT_CL_THR)                                        \
      ptt_pthread_trace(PTT_CL_THR,                                   \
        _msg,_data1,_data2,_loc,_result,NULL);                        \
  } while(0)
#define PTTRACE2(_msg,_data1,_data2,_loc,_result,_tv)                 \
  do {                                                                \
    if (pttclass & PTT_CL_THR)                                        \
      ptt_pthread_trace(PTT_CL_THR,                                   \
        _msg,_data1,_data2,_loc,_result,_tv);                         \
  } while(0)

/*-------------------------------------------------------------------*/
/* Default stack size for create_thread                              */
/*-------------------------------------------------------------------*/
#define HTHREAD_STACK_SIZE      ONE_MEGABYTE

/*-------------------------------------------------------------------*/
/* Issue locking call error message                                  */
/*-------------------------------------------------------------------*/
static void loglock( ILOCK* ilk, const int rc, const char* calltype,
                                               const char* err_loc )
{
    const char* err_desc;

    switch (rc)
    {
        case EAGAIN:          err_desc = "max recursion";    break;
        case EPERM:           err_desc = "not owned";        break;
        case EINVAL:          err_desc = "invalid argument"; break;
        case EDEADLK:         err_desc = "deadlock";         break;
        case ENOTRECOVERABLE: err_desc = "not recoverable";  break;
        case EOWNERDEAD:      err_desc = "owner dead";       break;
        case EBUSY:           err_desc = "busy";             break; /* (should not occur) */
        case ETIMEDOUT:       err_desc = "timeout";          break; /* (should not occur) */
        default:              err_desc = "(unknown)";        break;
    }

    // "'%s(%s)' failed: rc=%d: %s; tid="TIDPAT", loc=%s"
    WRMSG( HHC90013, "E", calltype, ilk->name, rc, err_desc,
        TID_CAST( hthread_self() ), TRIMLOC( err_loc ));

    if (ilk->tid)
    {
        // "lock %s was obtained by thread "TIDPAT" at %s"
        WRMSG( HHC90014, "I", ilk->name, TID_CAST( ilk->tid ),
            TRIMLOC( ilk->location ));
    }
}

/*-------------------------------------------------------------------*/
/* Hthreads internal variables                                       */
/*-------------------------------------------------------------------*/
static LIST_ENTRY  locklist;        /* Internal Locks list anchor    */
static HLOCK       listlock;        /* Lock for accessing Locks list */
static int         lockcount;       /* Number of locks in our list   */

/*-------------------------------------------------------------------*/
/* Internal macros to control access to our internal locks list      */
/*-------------------------------------------------------------------*/
#define LockLocksList()         hthread_mutex_lock( &listlock )
#define UnlockLocksList()       hthread_mutex_unlock( &listlock )

/*-------------------------------------------------------------------*/
/* Initialize HTHTREADS package                                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hthreads_internal_init()
{
    static BYTE bDidInit = FALSE;

    if (!bDidInit)
    {
        MATTR attr;
        int rc, minprio, maxprio;

#if defined( OPTION_FTHREADS )
        fthreads_internal_init();
#endif
        /* Initialize our internal lock */

        rc = hthread_mutexattr_init( &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_settype( &attr, HTHREAD_MUTEX_DEFAULT );
        if (rc)
            goto fatal;

        rc = hthread_mutex_init( &listlock, &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_destroy( &attr );
        if (rc)
            goto fatal;

        /* Initialize our locks list anchor */

        InitializeListHead( &locklist );
        lockcount = 0;

        /* Retrieve the minimum/maximum scheduling priority */

        minprio = hthread_get_priority_min( HTHREAD_POLICY );
        maxprio = hthread_get_priority_max( HTHREAD_POLICY );

        if (minprio >= 0 && maxprio >= 0 && maxprio >= minprio)
        {
            /* "The Open Group Base Specifications Issue 7"
               "Scheduling Policies", "SCHED_RR": "[...] Conforming
               implementations shall provide a priority range of at
               least 32 priorities for this policy."
            */
            if (((maxprio - minprio) + 1) < 32)
                goto fatal;

            sysblk.minprio = minprio;
            sysblk.maxprio = maxprio;
        }

        /* One-time initialization completed */

        bDidInit = TRUE;
        return;

fatal:
        perror( "Fatal error in hthreads_internal_init function" );
        exit(1);
    }
}

/*-------------------------------------------------------------------*/
/* Find or allocate and initialize an internal ILOCK structure.      */
/*-------------------------------------------------------------------*/
static ILOCK* hthreads_get_ILOCK( void* addr, const char* name )
{
    ILOCK*       ilk;               /* Pointer to ILOCK structure    */
    LIST_ENTRY*  ple;               /* Ptr to LIST_ENTRY structure   */

    /* Search list to see if this lock has already been allocated */

    LockLocksList();

    for (ple = locklist.Flink; ple != &locklist; ple = ple->Flink)
    {
        ilk = CONTAINING_RECORD( ple, ILOCK, locklink );
        if (ilk->addr == addr)
            break;
    }

    /* If needed, allocate a new ILOCK structure for this lock */

    if (&locklist == ple)
    {
        if (!(ilk = calloc_aligned( sizeof( ILOCK ), 64 )))
        {
            perror( "Fatal error in hthreads_get_ILOCK function" );
            exit(1);
        }
        ilk->addr = addr;
        InsertListTail( &locklist, &ilk->locklink );
        lockcount++;
    }

    ilk->name = name;
    ilk->location = "null:0";
    ilk->tid = 0;
    ilk->time.tv_sec = 0;
    ilk->time.tv_usec = 0;

    UnlockLocksList();

    return ilk;
}

/*-------------------------------------------------------------------*/
/* Initialize a lock                                                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_lock( LOCK* plk, const char* name,
                                         const char* location )
{
    int     rc;
    MATTR   attr;
    ILOCK*  ilk = hthreads_get_ILOCK( plk, name );

    /* Initialize the requested lock */

    rc = hthread_mutexattr_init( &attr );
    if (rc)
        goto fatal;

    rc = hthread_mutexattr_settype( &attr, HTHREAD_MUTEX_DEFAULT );
    if (rc)
        goto fatal;

    rc = hthread_mutex_init( &ilk->locklock, &attr );
    if (rc)
        goto fatal;

    rc = hthread_mutex_init( &ilk->lock, &attr );
    if (rc)
        goto fatal;

    rc = hthread_mutexattr_destroy( &attr );
    if (rc)
        goto fatal;

    plk->ilk = ilk; /* (LOCK is now initialized) */
    PTTRACE( "lock init", plk, 0, location, PTT_MAGIC );
    return 0;

fatal:

    perror( "Fatal error in hthread_initialize_lock function" );
    exit(1);
}

/*-------------------------------------------------------------------*/
/* Initialize a R/W lock                                             */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_rwlock( RWLOCK* plk, const char* name,
                                           const char* location )
{
    int     rc;
    RWATTR  attr1;    /* for primary lock */
    MATTR   attr2;    /* for internal locklock */
    ILOCK*  ilk = hthreads_get_ILOCK( plk, name );

    /* Initialize the requested lock */

    rc = hthread_rwlockattr_init( &attr1 );
    if (rc)
        goto fatal;

    rc = hthread_mutexattr_init( &attr2 );
    if (rc)
        goto fatal;

    rc = hthread_rwlockattr_setpshared( &attr1, HTHREAD_RWLOCK_DEFAULT );
    if (rc)
        goto fatal;

    rc = hthread_mutexattr_settype( &attr2, HTHREAD_MUTEX_DEFAULT );
    if (rc)
        goto fatal;

    rc = hthread_rwlock_init( &ilk->rwlock, &attr1 );
    if (rc)
        goto fatal;

    rc = hthread_mutex_init( &ilk->locklock, &attr2 );
    if (rc)
        goto fatal;

    plk->ilk = ilk;     /* (RWLOCK is now initialized) */
    PTTRACE( "rwlock init", plk, &attr1, location, PTT_MAGIC );

    rc = hthread_mutexattr_destroy( &attr2 );
    if (rc)
        goto fatal;

    rc = hthread_rwlockattr_destroy( &attr1 );
    if (rc)
        goto fatal;

    return 0;

fatal:

    perror( "Fatal error in hthread_initialize_rwlock function" );
    exit(1);
}

/*-------------------------------------------------------------------*/
/* Obtain a lock                                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_obtain_lock( LOCK* plk, const char* location )
{
    int rc;
    U64 waitdur;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "lock before", plk, NULL, location, PTT_MAGIC );
    rc = hthread_mutex_trylock( &ilk->lock );
    if (EBUSY == rc)
    {
        waitdur = host_tod();
        rc = hthread_mutex_lock( &ilk->lock );
        gettimeofday( &tv, NULL );
        waitdur = host_tod() - waitdur;
    }
    else
    {
        gettimeofday( &tv, NULL );
        waitdur = 0;
    }
    PTTRACE2( "lock after", plk, (void*) waitdur, location, rc, &tv );
    if (rc)
        loglock( ilk, rc, "obtain_lock", location );
    if (!rc || EOWNERDEAD == rc)
    {
        hthread_mutex_lock( &ilk->locklock );
        ilk->location = location;
        ilk->tid = hthread_self();
        memcpy( &ilk->time, &tv, sizeof( TIMEVAL ));
        hthread_mutex_unlock( &ilk->locklock );
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Release a lock                                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_release_lock( LOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_mutex_unlock( &ilk->lock );
    PTTRACE( "unlock", plk, NULL, location, rc );
    if (rc)
        loglock( ilk, rc, "release_lock", location );
    hthread_mutex_lock( &ilk->locklock );
    ilk->location = "null:0";
    ilk->tid = 0;
    hthread_mutex_unlock( &ilk->locklock );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Release a R/W lock                                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_release_rwlock( RWLOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_rwlock_unlock( &ilk->rwlock );
    PTTRACE( "rwunlock", plk, NULL, location, rc );
    if (rc)
        loglock( ilk, rc, "release_rwlock", location );
    hthread_mutex_lock( &ilk->locklock );
    ilk->location = "null:0";
    ilk->tid = 0;
    hthread_mutex_unlock( &ilk->locklock );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Destroy a lock                                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_destroy_lock( LOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    UNREFERENCED( location );
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_mutex_destroy( &ilk->lock );
    LockLocksList();
    RemoveListEntry( &ilk->locklink );
    lockcount--;
    UnlockLocksList();
    free_aligned( ilk );
    plk->ilk = NULL;
    return rc;
}

/*-------------------------------------------------------------------*/
/* Destroy a R/W lock                                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_destroy_rwlock( RWLOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    UNREFERENCED( location );
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_rwlock_destroy( &ilk->rwlock );
    LockLocksList();
    RemoveListEntry( &ilk->locklink );
    lockcount--;
    UnlockLocksList();
    free_aligned( ilk );
    plk->ilk = NULL;
    return rc;
}

/*-------------------------------------------------------------------*/
/* Try to obtain a lock                                              */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_try_obtain_lock( LOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "try before", plk, NULL, location, PTT_MAGIC );
    rc = hthread_mutex_trylock( &ilk->lock );
    gettimeofday( &tv, NULL );
    PTTRACE2( "try after", plk, NULL, location, rc, &tv );
    if (rc && EBUSY != rc)
        loglock( ilk, rc, "try_obtain_lock", location );
    if (!rc || EOWNERDEAD == rc)
    {
        hthread_mutex_lock( &ilk->locklock );
        ilk->location = location;
        ilk->tid = hthread_self();
        memcpy( &ilk->time, &tv, sizeof( TIMEVAL ));
        hthread_mutex_unlock( &ilk->locklock );
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Test if *ANYONE* is holding given lock  (pseudo-boolean function) */
/*-------------------------------------------------------------------*/
/* PROGRAMMING NOTE: this function ISN'T a boolean function but can  */
/* be treated as such, with the following caveat: it only determines */
/* if the given lock is held by anyone, but not necessarily by YOU!  */
/* That is to say, if the function returns 0 (false), then it means  */
/* the lock is NOT being held -- by ANYONE. If the function returns  */
/* non-zero (true) however, then it only means the lock *is* being   */
/* held -- by SOMEONE -- but that someone may NOT be you!  It might  */
/* be held by someone OTHER than you!                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_test_lock( LOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    UNREFERENCED( location );
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_mutex_trylock( &ilk->lock );
    if (rc)
        return rc;
    hthread_mutex_unlock( &ilk->lock );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Check if *YOU* are holding a given lock        (boolean function) */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_have_lock( LOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_equal_threads( hthread_self(), ilk->tid, location );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Obtain read-only access to R/W lock                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_obtain_rdlock( RWLOCK* plk, const char* location )
{
    int rc;
    U64 waitdur;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "rdlock before", plk, NULL, location, PTT_MAGIC );
    rc = hthread_rwlock_tryrdlock( &ilk->rwlock );
    if (EBUSY == rc)
    {
        waitdur = host_tod();
        rc = hthread_rwlock_rdlock( &ilk->rwlock );
        waitdur = host_tod() - waitdur;
    }
    else
        waitdur = 0;
    PTTRACE( "rdlock after", plk, (void*) waitdur, location, rc );
    if (rc)
        loglock( ilk, rc, "obtain_rdloc", location );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Obtain exclusive write access to a R/W lock                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_obtain_wrlock( RWLOCK* plk, const char* location )
{
    int rc;
    U64 waitdur;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "wrlock before", plk, NULL, location, PTT_MAGIC );
    rc = hthread_rwlock_trywrlock( &ilk->rwlock );
    if (EBUSY == rc)
    {
        waitdur = host_tod();
        rc = hthread_rwlock_wrlock( &ilk->rwlock );
        gettimeofday( &tv, NULL );
        waitdur = host_tod() - waitdur;
    }
    else
    {
        gettimeofday( &tv, NULL );
        waitdur = 0;
    }
    PTTRACE2( "wrlock after", plk, (void*) waitdur, location, rc, &tv );
    if (rc)
        loglock( ilk, rc, "obtain_wrlock", location );
    if (!rc || EOWNERDEAD == rc)
    {
        hthread_mutex_lock( &ilk->locklock );
        ilk->location = location;
        ilk->tid = hthread_self();
        memcpy( &ilk->time, &tv, sizeof( TIMEVAL ));
        hthread_mutex_unlock( &ilk->locklock );
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Try to obtain read-only access to a R/W lock                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_try_obtain_rdlock( RWLOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "tryrd before", plk, NULL, location, PTT_MAGIC );
    rc = hthread_rwlock_tryrdlock( &ilk->rwlock );
    PTTRACE( "tryrd after", plk, NULL, location, rc );
    if (rc && EBUSY != rc)
        loglock( ilk, rc, "try_obtain_rdlock", location );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Try to obtain exclusive write access to a R/W lock                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_try_obtain_wrlock( RWLOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "trywr before", plk, NULL, location, PTT_MAGIC );
    rc = hthread_rwlock_trywrlock( &ilk->rwlock );
    gettimeofday( &tv, NULL );
    PTTRACE2( "trywr after", plk, NULL, location, rc, &tv );
    if (rc && EBUSY != rc)
        loglock( ilk, rc, "try_obtain_wrlock", location );
    if (!rc)
    {
        hthread_mutex_lock( &ilk->locklock );
        ilk->location = location;
        ilk->tid = hthread_self();
        memcpy( &ilk->time, &tv, sizeof( TIMEVAL ));
        hthread_mutex_unlock( &ilk->locklock );
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Test if read-only access to a R/W lock is held                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_test_rdlock( RWLOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    UNREFERENCED( location );
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_rwlock_tryrdlock( &ilk->rwlock );
    if (rc)
        return rc;
    hthread_rwlock_unlock( &ilk->rwlock );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Test if exclusive write access to a R/W lock is held              */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_test_wrlock( RWLOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    UNREFERENCED( location );
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_rwlock_trywrlock( &ilk->rwlock );
    if (rc)
        return rc;
    hthread_rwlock_unlock( &ilk->rwlock );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Initialize a condition variable                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_condition( COND* plc, const char* location )
{
    int rc;
    PTTRACE( "cond init", NULL, plc, location, PTT_MAGIC );
    rc = hthread_cond_init( plc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Destroy a condition variable                                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_destroy_condition( COND* plc, const char* location )
{
    int rc;
    UNREFERENCED( location );
    rc = hthread_cond_destroy( plc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Signal a condition variable (releases only one thread)            */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_signal_condition( COND* plc, const char* location )
{
    int rc;
    rc = hthread_cond_signal( plc );
    PTTRACE( "signal", NULL, plc, location, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Broadcast a condition variable (releases all waiting threads)     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_broadcast_condition( COND* plc, const char* location )
{
    int rc;
    rc = hthread_cond_broadcast( plc );
    PTTRACE( "broadcast", NULL, plc, location, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Wait for condition variable to be signaled                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_wait_condition( COND* plc, LOCK* plk, const char* location )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "wait before", plk, plc, location, PTT_MAGIC );
    rc = hthread_cond_wait( plc, &ilk->lock );
    PTTRACE( "wait after", plk, plc, location, rc );
    ilk->tid = hthread_thread_id( location );
    if (rc)
        loglock( ilk, rc, "wait_condition", location );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Timed wait for a condition variable to be signaled                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_timed_wait_condition( COND* plc, LOCK* plk,
                                              const struct timespec* tm,
                                              const char* location )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "tw before", plk, plc, location, PTT_MAGIC );
    rc = hthread_cond_timedwait( plc, &ilk->lock, tm );
    PTTRACE( "tw after", plk, plc, location, rc );
    ilk->tid = hthread_thread_id( location );
    if (rc && ETIMEDOUT != rc)
        loglock( ilk, rc, "timed_wait_condition", location );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Internal helper function to initialize a thread attribute         */
/*-------------------------------------------------------------------*/
static INLINE int hthread_init_thread_attr( ATTR* pat, int state,
                                            const char* location )
{
    int rc;
    UNREFERENCED( location );
    rc = hthread_attr_init( pat );
    if (!rc)
        rc = hthread_attr_setstacksize( pat, HTHREAD_STACK_SIZE );
    if (!rc)
        rc = hthread_attr_setdetachstate( pat, state );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Initialize a thread attribute to "joinable"                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_join_attr( ATTR* pat, const char* location )
{
    int rc;
    rc = hthread_init_thread_attr( pat, HTHREAD_CREATE_JOINABLE, location );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Initialize a thread attribute to "detached" (non-joinable)        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_detach_attr( ATTR* pat, const char* location )
{
    int rc;
    rc = hthread_init_thread_attr( pat, HTHREAD_CREATE_DETACHED, location );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Internal function to list abandoned locks when thread exits       */
/*-------------------------------------------------------------------*/
static void hthread_list_abandoned_locks( TID tid, const char* exit_loc )
{
    ILOCK*       ilk;
    LIST_ENTRY*  ple;

    if (sysblk.shutdown)
        return;

    LockLocksList();
    for (ple = locklist.Flink; ple != &locklist; ple = ple->Flink)
    {
        ilk = CONTAINING_RECORD( ple, ILOCK, locklink );
        if (hthread_equal(ilk->tid,tid))
        {
            char tod[27];           /* "YYYY-MM-DD HH:MM:SS.uuuuuu"  */

            FormatTIMEVAL( &ilk->time, tod, sizeof( tod ));

            if (exit_loc)
            {
                // "Thread "TIDPAT" has abandoned at %s lock %s obtained on %s at %s"
                WRMSG( HHC90016, "E", TID_CAST( tid ), TRIMLOC( exit_loc ),
                    ilk->name, &tod[11], TRIMLOC( ilk->location ));
            }
            else
            {
                // "Thread "TIDPAT" has abandoned lock %s obtained on %s at %s"
                WRMSG( HHC90015, "E", TID_CAST( tid ),
                    ilk->name, &tod[11], TRIMLOC( ilk->location ));
            }
        }
    }
    UnlockLocksList();
}

/*-------------------------------------------------------------------*/
/* Internal thread function to intercept thread exit via return      */
/*-------------------------------------------------------------------*/
static void* hthread_func( void* arg2 )
{
    THREAD_FUNC*  pfn  = (THREAD_FUNC*) *((void**)arg2+0);
    void*         arg  = (void*)        *((void**)arg2+1);
    const char*   name = (const char*)  *((void**)arg2+2);
    TID           tid  = hthread_self();
    void*         rc;
    free( arg2 );
    if (name)
        SET_THREAD_NAME_ID( tid, name );
    rc = pfn( arg );
    hthread_list_abandoned_locks( tid, NULL );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Create a new thread                                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_create_thread( TID* ptid, ATTR* pat,
                                       THREAD_FUNC* pfn, void* arg,
                                       const char* name, const char* location )
{
    int rc;
    void** arg2;
    arg2 = malloc( 3 * sizeof( void* ));
    *(arg2+0) = (void*) pfn;
    *(arg2+1) = (void*) arg;
    *(arg2+2) = (void*) strdup(name);
    rc = hthread_create( ptid, pat, hthread_func, arg2 );
    PTTRACE( "create", (void*)*ptid, NULL, location, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Wait for a thread to terminate                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_join_thread( TID tid, void** prc, const char* location )
{
    int rc;
    PTTRACE( "join before", (void*) tid, prc ? *prc : NULL, location, PTT_MAGIC );
    rc = hthread_join( tid, prc );
    PTTRACE( "join after",  (void*) tid, prc ? *prc : NULL, location, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Detach from a thread (release resources or change to "detached")  */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_detach_thread( TID tid, const char* location )
{
    int rc;
    PTTRACE( "dtch before", (void*) tid, NULL, location, PTT_MAGIC );
    rc = hthread_detach( tid );
    PTTRACE( "dtch after", (void*) tid, NULL, location, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Return calling thread's ID                                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT TID  hthread_thread_id( const char* location )
{
    TID tid;
    UNREFERENCED( location );
    tid = hthread_self();
    return tid;
}

/*-------------------------------------------------------------------*/
/* Exit immediately from a thread                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hthread_exit_thread( void* rc, const char* location )
{
    TID tid;
    tid = hthread_self();
    hthread_list_abandoned_locks( tid, location );
    hthread_exit( rc );
}

/*-------------------------------------------------------------------*/
/* Compare two thread IDs                         (boolean function) */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_equal_threads( TID tid1, TID tid2, const char* location )
{
    int rc;
    UNREFERENCED( location );
    rc = hthread_equal( tid1, tid2 );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Change a thread's dispatching priority    (HTHREADS function)     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hthread_set_thread_prio( TID tid, int prio, const char* location )
{
    int rc;
    struct sched_param param = {0};

    /* Convert hthread priority relative to 0 to pthread priority */
    param.sched_priority = sysblk.minprio + prio;

    if (equal_threads( tid, 0 ))
        tid = thread_id();

    SETMODE( ROOT );
    {
        rc = hthread_setschedparam( tid, HTHREAD_POLICY, &param );
    }
    SETMODE( USER );

    if (rc != 0)
    {
        if (EPERM != rc)
        {
            // "'%s' failed at loc=%s: rc=%d: %s"
            WRMSG( HHC90020, "W", "hthread_setschedparam()",
                TRIMLOC( location ), rc, strerror( rc ));
        }
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Retrieve a thread's dispatching priority   (HTHREADS function)    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hthread_get_thread_prio( TID tid, const char* location )
{
    int rc, policy, prio;
    struct sched_param  param = {0};

    if (equal_threads( tid, 0 ))
        tid = thread_id();

    SETMODE( ROOT );
    {
        rc = hthread_getschedparam( tid, &policy, &param );
    }
    SETMODE( USER );

    /* Convert pthread priority to hthread priority relative to 0 */
    prio = param.sched_priority - sysblk.minprio;

    if (rc != 0)
    {
        prio = -1;  /* (return negative value to indicate error) */

        if (EPERM != rc)
        {
            // "'%s' failed at loc=%s: rc=%d: %s"
            WRMSG( HHC90020, "W", "hthread_getschedparam()",
                TRIMLOC( location ), rc, strerror( rc ));
        }
    }
    return prio;
}

/*-------------------------------------------------------------------*/
/* locks_cmd helper function: save offline copy of all locks in list */
/*-------------------------------------------------------------------*/
static int hthreads_copy_locks_list( ILOCK** ppILOCK )
{
    ILOCK*       ilk;               /* Pointer to ILOCK structure    */
    ILOCK*       ilka;              /* Pointer to ILOCK array        */
    LIST_ENTRY*  ple;               /* Ptr to LIST_ENTRY structure   */
    int i;

    LockLocksList();

    if (!(*ppILOCK = ilka = (ILOCK*) malloc( lockcount * sizeof( ILOCK ))))
    {
        UnlockLocksList();
        return 0;
    }

    for (i=0, ple = locklist.Flink; ple != &locklist; ple = ple->Flink, i++)
    {
        ilk = CONTAINING_RECORD( ple, ILOCK, locklink );
        memcpy( &ilka[i], ilk, sizeof( ILOCK ));
    }

    i = lockcount;
    UnlockLocksList();
    return i;
}

/*-------------------------------------------------------------------*/
/* locks_cmd sort functions                                          */
/*-------------------------------------------------------------------*/
static int sortby_tim( const ILOCK* p1, const ILOCK* p2 )
{
    return     (p1->time.tv_sec    !=     p2->time.tv_sec)  ?
        ((long) p1->time.tv_sec  - (long) p2->time.tv_sec)  :
        ((long) p1->time.tv_usec - (long) p2->time.tv_usec) ;
}
static int sortby_tid( const ILOCK* p1, const ILOCK* p2 )
{
    return equal_threads( p1->tid,             p2->tid ) ? 0 :
              ((intptr_t) p1->tid - (intptr_t) p2->tid);
}
static int sortby_nam( const ILOCK* p1, const ILOCK* p2 )
{
    return strcasecmp( p1->name, p2->name );
}
static int sortby_loc( const ILOCK* p1, const ILOCK* p2 )
{
    return strcasecmp( p1->location, p2->location );
}

/*-------------------------------------------------------------------*/
/* locks_cmd - list internal locks                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int locks_cmd( int argc, char* argv[], char* cmdline )
{
    ILOCK*       ilk;               /* Pointer to ILOCK array        */
    char         tod[27];           /* "YYYY-MM-DD HH:MM:SS.uuuuuu"  */
    TID          tid = 0;           /* Requested thread id           */
    int          seq = 1;           /* Requested sort order          */
    int count, i, rc = 0;
    char c;

    UNREFERENCED( cmdline );

    /*  Format: "locks [ALL|tid|HELD] [SORT NAME|OWNER|TIME|LOC]"  */
    /*  Note:    TID is alias for OWNER,  TOD is alias for TIME.   */

    if (argc <= 1)
        tid = 0;
    else if (strcasecmp( argv[1], "ALL"  ) == 0)
        tid = 0;
    else if (strcasecmp( argv[1], "HELD" ) == 0)
        tid = (TID) -1;
    else if (sscanf( argv[1], SCN_TIDPAT "%c", &tid, &c ) != 1)
        rc = -1;

    if (!rc)
    {
        if (argc == 4)
        {
            if (strcasecmp( argv[2], "SORT" ) != 0)
                rc = -1;
            else
            {
                     if (strcasecmp( argv[3], "TIME"  ) == 0) seq = 1;
                else if (strcasecmp( argv[3], "TOD"   ) == 0) seq = 1;
                else if (strcasecmp( argv[3], "OWNER" ) == 0) seq = 2;
                else if (strcasecmp( argv[3], "TID"   ) == 0) seq = 2;
                else if (strcasecmp( argv[3], "NAME"  ) == 0) seq = 3;
                else if (strcasecmp( argv[3], "LOC"   ) == 0) seq = 4;
                else
                    rc = -1;
            }
        }
        else if (argc != 1 && argc != 2)
            rc = -1;

        /* If no errors, perform the requested function */

        if (!rc)
        {
            /* Retrieve a copy of the locks list */

            count = hthreads_copy_locks_list( &ilk );

            /* Sort them into the requested sequence */

            if (count)
            {
                switch (seq)
                {
                case 1: qsort( ilk, count, sizeof( ILOCK ), (CMPFUNC*) sortby_tim ); break;
                case 2: qsort( ilk, count, sizeof( ILOCK ), (CMPFUNC*) sortby_tid ); break;
                case 3: qsort( ilk, count, sizeof( ILOCK ), (CMPFUNC*) sortby_nam ); break;
                case 4: qsort( ilk, count, sizeof( ILOCK ), (CMPFUNC*) sortby_loc ); break;
                default:
                    BREAK_INTO_DEBUGGER();
                }

                /* Display the requested locks */

                for (c=0, i=0; i < count; i++ )
                {
                    if (0
                        || !tid
                        || (equal_threads( tid, (TID) -1 ) && !equal_threads( ilk[i].tid, 0 ))
                        || equal_threads( tid, ilk[i].tid )
                    )
                    {
                        c=1;
                        FormatTIMEVAL( &ilk[i].time, tod, sizeof( tod ));
                        // "Lock=%s, tid="TIDPAT", tod=%s, loc=%s"
                        WRMSG( HHC90017, "I", ilk[i].name, TID_CAST( ilk[i].tid ),
                            &tod[11], TRIMLOC( ilk[i].location ));
                    }
                }

                free( ilk );
            }
            else
                c = 1;

            /* Print results */

            if (!c)
            {
                // "No locks found for thread "TIDPAT"."
                WRMSG( HHC90019, "W", TID_CAST( tid ));
            }
            else if (!tid)
            {
                // "Total locks defined: %d"
                WRMSG( HHC90018, "I", count );
            }
        }
    }

    if (rc)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
    }

    return rc;
}
