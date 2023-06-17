/* HTHREADS.C   (C) Copyright "Fish" (David B. Trout), 2013-2021     */
/*              Hercules locking and threading                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Based on original pttrace.c (C) Copyright Greg Smith, 2003-2013   */
/* Collaboratively enhanced and extended  by Mark L. Gaubatz         */
/* and "Fish" (David B. Trout), May-June 2013                        */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HTHREAD_C_
#define _HUTIL_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Hercules Internal thread structure                                */
/*-------------------------------------------------------------------*/
struct HTHREAD                  /* Hercules internal thread structure*/
{
    LIST_ENTRY   ht_link;       /* links threads together in a chain */
    const char*  ht_cr_locat;   /* Location where thread was created */
    TIMEVAL      ht_cr_time;    /* Time of day when it was created   */
    TID          ht_tid;        /* Thread-Id of the thread           */
    LOCK*        ht_ob_lock;    /* Lock attempting to obtain or NULL */
    TIMEVAL      ht_ob_time;    /* Time of day when obtain attempted */
    const char*  ht_ob_where;   /* Location where obtain attempted   */
    const char*  ht_name;       /* strdup of Thread name             */
    bool         ht_footprint;  /* Footprint for deadlock detection  */
};
typedef struct HTHREAD HTHREAD; /* Shorter name for the same thing   */

/*-------------------------------------------------------------------*/
/* Hercules Internal ILOCK structure                                 */
/*-------------------------------------------------------------------*/
struct ILOCK                    /* Hercules internal ILOCK structure */
{
    LIST_ENTRY   il_link;       /* links locks together in a chain   */
    void*        il_addr;       /* Lock address                      */
    const char*  il_name;       /* strdup of Lock name               */
    const char*  il_ob_locat;   /* Location where lock was obtained  */
    TIMEVAL      il_ob_time;    /* Time of day when it was obtained  */
    TID          il_ob_tid;     /* Thread-Id of who obtained it      */
    HLOCK        il_locklock;   /* Internal ILOCK structure lock     */
    union      {
    HLOCK        il_lock;       /* The actual locking model mutex    */
    HRWLOCK      il_rwlock;     /* The actual locking model rwlock   */
               };
    const char*  il_cr_locat;   /* Location where lock was created   */
    TIMEVAL      il_cr_time;    /* Time of day when it was created   */
    TID          il_cr_tid;     /* Thread-Id of who created it       */
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
        case EEXIST:          err_desc = "already init'ed";  break;
        case EAGAIN:          err_desc = "max recursion";    break;
        case EPERM:           err_desc = "not owned";        break;
        case EINVAL:          err_desc = "invalid argument"; break;
        case EDEADLK:         err_desc = "deadlock";         break;
        case ENOTRECOVERABLE: err_desc = "not recoverable";  break;
        case EOWNERDEAD:      err_desc = "owner dead";       break;
        case EBUSY:           err_desc = "busy";             break;
        case ETIMEDOUT:       err_desc = "timeout";          break;
        default:              err_desc = "(unknown)";        break;
    }

    // "'%s(%s)' failed: rc=%d: %s; tid="TIDPAT", loc=%s"
    WRMSG( HHC90013, "E", calltype, ilk->il_name, rc, err_desc,
        TID_CAST( hthread_self() ), TRIMLOC( err_loc ));

    if (EEXIST == rc)
    {
        // "lock %s was already initialized at %s"
        WRMSG( HHC90028, "I", ilk->il_name, TRIMLOC( ilk->il_cr_locat ));
    }
    if (ilk->il_ob_tid)
    {
        // "lock %s was %s by thread "TIDPAT" at %s"
        WRMSG( HHC90014, "I", ilk->il_name,
            EEXIST == rc ? "still held" : "obtained",
            TID_CAST( ilk->il_ob_tid ),
            TRIMLOC( ilk->il_ob_locat ));
    }
}

/*-------------------------------------------------------------------*/
/* Hthreads internal variables                                       */
/*-------------------------------------------------------------------*/
static LIST_ENTRY  locklist;        /* Internal locks list anchor    */
static HLOCK       listlock;        /* Lock for accessing locklist   */
static int         lockcount;       /* Number of locks in list       */
static LIST_ENTRY  threadlist;      /* Internal threads list anchor  */
static HLOCK       threadlock;      /* Lock for accessing threadlist */
static int         threadcount;     /* Number of threads in list     */
static bool        inited = false;  /* true = internally initialized */

/*-------------------------------------------------------------------*/
/* Internal macros to control access to our internal lists           */
/*-------------------------------------------------------------------*/
#define LockLocksList()         hthread_mutex_lock   ( &listlock   )
#define UnlockLocksList()       hthread_mutex_unlock ( &listlock   )
#define LockThreadsList()       hthread_mutex_lock   ( &threadlock )
#define UnlockThreadsList()     hthread_mutex_unlock ( &threadlock )

/*-------------------------------------------------------------------*/
/* Initialize HTHTREADS package                                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hthreads_internal_init()
{
    if (!inited)
    {
        MATTR attr;
        int rc, minprio, maxprio;

#if defined( OPTION_FTHREADS )
        fthreads_internal_init();
#endif
        /* Initialize our internal locks */

        rc = hthread_mutexattr_init( &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_settype( &attr, HTHREAD_MUTEX_DEFAULT );
        if (rc)
            goto fatal;

        rc = hthread_mutex_init( &listlock, &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutex_init( &threadlock, &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_destroy( &attr );
        if (rc)
            goto fatal;

        /* Initialize our list anchors */

        InitializeListHead( &locklist );
        lockcount = 0;

        InitializeListHead( &threadlist );
        threadcount = 0;

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

        /* Add an entry for the current thread to our threads list
           since it was created by the operating system and not us */
        {
            const char* ht_cr_locat = PTT_LOC; // (where created)
            HTHREAD* ht = calloc_aligned( sizeof( HTHREAD ), 64 );

            InitializeListLink( &ht->ht_link );
            gettimeofday( &ht->ht_cr_time, NULL );

            ht->ht_tid        =  hthread_self();
            ht->ht_name       =  strdup( "main" );
            ht->ht_cr_locat   =  ht_cr_locat;
            ht->ht_ob_lock    =  NULL;
            ht->ht_footprint  =  false;

            InsertListHead( &threadlist, &ht->ht_link );
            threadcount++;
        }

        /* One-time initialization completed */

        inited = true;
        return;

fatal:
        perror( "Fatal error in hthreads_internal_init function" );
        exit(1);
    }
}

/*-------------------------------------------------------------------*/
/*                     Find ILOCK structure.                         */
/* Note: the caller is responsible for locking the list beforehand!  */
/*-------------------------------------------------------------------*/
static ILOCK* hthreads_find_ILOCK_locked( const LOCK* plk, LIST_ENTRY* anchor  )
{
    ILOCK*       ilk;               /* Pointer to ILOCK structure    */
    LIST_ENTRY*  ple;               /* Ptr to LIST_ENTRY structure   */

    if (!anchor) anchor = &locklist;

    for (ple = anchor->Flink; ple != anchor; ple = ple->Flink)
    {
        ilk = CONTAINING_RECORD( ple, ILOCK, il_link );
        if (ilk->il_addr == plk)
        {
            RemoveListEntry( &ilk->il_link );
            InsertListHead( anchor, &ilk->il_link );
            return ilk;
        }
    }
    return NULL;
}

#if 0 // (currently unreferenced)
/*-------------------------------------------------------------------*/
/*                 Find internal ILOCK structure.                    */
/*-------------------------------------------------------------------*/
static ILOCK* hthreads_find_ILOCK( const LOCK* plk )
{
    ILOCK* ilk;
    LockLocksList();
    {
        ilk = hthreads_find_ILOCK_locked( plk, NULL );
    }
    UnlockLocksList();
    return ilk;
}
#endif

/*-------------------------------------------------------------------*/
/* Initialize a lock                                                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_lock( LOCK* plk, const char* name,
                                                    const char* create_loc )
{
    int     rc;
    MATTR   attr;     /* for internal lock structure lock */
    ILOCK*  ilk;

    LockLocksList();
    {
        ilk = hthreads_find_ILOCK_locked( plk, NULL );

        if (ilk)
        {
            loglock( ilk, EEXIST, "initialize lock", create_loc );
            UnlockLocksList();
            return EEXIST;
        }

        if (!(ilk = calloc_aligned( sizeof( ILOCK ), 64 )))
            goto fatal;

        gettimeofday( &ilk->il_cr_time, NULL );

        ilk->il_addr             =  plk;
        ilk->il_name             =  strdup( name );
        ilk->il_cr_locat         =  create_loc;
        ilk->il_cr_tid           =  hthread_self();
        ilk->il_ob_locat         =  "null:0";
        ilk->il_ob_time.tv_sec   =  0;
        ilk->il_ob_time.tv_usec  =  0;
        ilk->il_ob_tid           =  0;

        /* Initialize the requested lock */

        rc = hthread_mutexattr_init( &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_settype( &attr, HTHREAD_MUTEX_DEFAULT );
        if (rc)
            goto fatal;

        rc = hthread_mutex_init( &ilk->il_locklock, &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutex_init( &ilk->il_lock, &attr );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_destroy( &attr );
        if (rc)
            goto fatal;

        plk->ilk = ilk; /* (LOCK is now initialized) */

        InsertListHead( &locklist, &ilk->il_link );
        lockcount++;

        PTTRACE( "lock init", plk, 0, create_loc, PTT_MAGIC );
    }
    UnlockLocksList();

    return 0;

fatal:

    perror( "Fatal error in hthread_initialize_lock function" );
    exit(1);
}

/*-------------------------------------------------------------------*/
/* Initialize a R/W lock                                             */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_rwlock( RWLOCK* plk, const char* name,
                                                        const char* create_loc )
{
    int     rc;
    MATTR   attr1;    /* for internal lock structure lock */
    RWATTR  attr2;    /* for primary rwlock */
    ILOCK*  ilk;

    LockLocksList();
    {
        ilk = hthreads_find_ILOCK_locked( (LOCK*) plk, NULL );

        if (ilk)
        {
            loglock( ilk, EEXIST, "initialize rwlock", create_loc );
            UnlockLocksList();
            return EEXIST;
        }

        if (!(ilk = calloc_aligned( sizeof( ILOCK ), 64 )))
            goto fatal;

        gettimeofday( &ilk->il_cr_time, NULL );

        ilk->il_addr             =  plk;
        ilk->il_name             =  strdup( name );
        ilk->il_cr_locat         =  create_loc;
        ilk->il_cr_tid           =  hthread_self();
        ilk->il_ob_locat         =  "null:0";
        ilk->il_ob_time.tv_sec   =  0;
        ilk->il_ob_time.tv_usec  =  0;
        ilk->il_ob_tid           =  0;

        /* Initialize the requested lock */

        rc = hthread_mutexattr_init( &attr1 );
        if (rc)
            goto fatal;

        rc = hthread_rwlockattr_init( &attr2 );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_settype( &attr1, HTHREAD_MUTEX_DEFAULT );
        if (rc)
            goto fatal;

        rc = hthread_rwlockattr_setpshared( &attr2, HTHREAD_RWLOCK_DEFAULT );
        if (rc)
            goto fatal;

        rc = hthread_mutex_init( &ilk->il_locklock, &attr1 );
        if (rc)
            goto fatal;

        rc = hthread_rwlock_init( &ilk->il_rwlock, &attr2 );
        if (rc)
            goto fatal;

        rc = hthread_mutexattr_destroy( &attr1 );
        if (rc)
            goto fatal;

        rc = hthread_rwlockattr_destroy( &attr2 );
        if (rc)
            goto fatal;

        plk->ilk = ilk;     /* (RWLOCK is now initialized) */

        InsertListHead( &locklist, &ilk->il_link );
        lockcount++;

        PTTRACE( "rwlock init", plk, &attr2, create_loc, PTT_MAGIC );
    }
    UnlockLocksList();

    return 0;

fatal:

    perror( "Fatal error in hthread_initialize_rwlock function" );
    exit(1);
}

/*-------------------------------------------------------------------*/
/*                     Find HTHREAD structure.                       */
/* Note: the caller is responsible for locking the list beforehand!  */
/*-------------------------------------------------------------------*/
static HTHREAD* hthread_find_HTHREAD_locked( TID tid, LIST_ENTRY* anchor )
{
    HTHREAD*     ht;
    LIST_ENTRY*  ple;

    if (!anchor) anchor = &threadlist;

    for (ple = anchor->Flink; ple != anchor; ple = ple->Flink)
    {
        ht = CONTAINING_RECORD( ple, HTHREAD, ht_link );

        if (equal_threads( ht->ht_tid, tid ))
        {
            RemoveListEntry( &ht->ht_link );
            InsertListHead( anchor, &ht->ht_link );
            return ht;
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Internal thread function to find an HTHREAD entry in our list     */
/*-------------------------------------------------------------------*/
static HTHREAD* hthread_find_HTHREAD( TID tid )
{
    HTHREAD* ht;
    LockThreadsList();
    {
        ht = hthread_find_HTHREAD_locked( tid, NULL );
    }
    UnlockThreadsList();
    return ht;
}

/*-------------------------------------------------------------------*/
/* Remember that a thread is waiting to obtain a given lock          */
/*-------------------------------------------------------------------*/
static void hthread_obtaining_lock( LOCK* plk, const char* loc )
{
    HTHREAD* ht;
    if (!(ht = hthread_find_HTHREAD( hthread_self() )))
        return;
    ht->ht_ob_lock = plk;
    free( ht->ht_ob_where );
    ht->ht_ob_where = strdup( loc );
    gettimeofday( &ht->ht_ob_time, NULL );
}

/*-------------------------------------------------------------------*/
/* Forget that a thread was waiting for a lock                       */
/*-------------------------------------------------------------------*/
static void hthread_lock_obtained()
{
    HTHREAD* ht;
    if (!(ht = hthread_find_HTHREAD( hthread_self() )))
        return;
    ht->ht_ob_lock = NULL;
}

/*-------------------------------------------------------------------*/
/* Obtain a lock                                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_obtain_lock( LOCK* plk, const char* obtain_loc )
{
    int rc;
    U64 waitdur;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    hthread_obtaining_lock( plk, obtain_loc );
    PTTRACE( "lock before", plk, NULL, obtain_loc, PTT_MAGIC );
    rc = hthread_mutex_trylock( &ilk->il_lock );
    if (EBUSY == rc)
    {
        waitdur = host_tod();
        rc = hthread_mutex_lock( &ilk->il_lock );
        gettimeofday( &tv, NULL );
        waitdur = host_tod() - waitdur;
    }
    else
    {
        gettimeofday( &tv, NULL );
        waitdur = 0;
    }
    PTTRACE2( "lock after", plk, (void*) waitdur, obtain_loc, rc, &tv );
    hthread_lock_obtained();
    if (rc)
        loglock( ilk, rc, "obtain_lock", obtain_loc );
    if (!rc || EOWNERDEAD == rc)
    {
        hthread_mutex_lock( &ilk->il_locklock );
        {
            ilk->il_ob_locat = obtain_loc;
            ilk->il_ob_tid = hthread_self();
            memcpy( &ilk->il_ob_time, &tv, sizeof( TIMEVAL ));
        }
        hthread_mutex_unlock( &ilk->il_locklock );
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Release a lock                                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_release_lock( LOCK* plk, const char* release_loc )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_mutex_unlock( &ilk->il_lock );
    PTTRACE( "unlock", plk, NULL, release_loc, rc );
    if (rc)
        loglock( ilk, rc, "release_lock", release_loc );
    hthread_mutex_lock( &ilk->il_locklock );
    {
        ilk->il_ob_locat = "null:0";
        ilk->il_ob_tid = 0;
    }
    hthread_mutex_unlock( &ilk->il_locklock );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Release a R/W lock                                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_release_rwlock( RWLOCK* plk, const char* release_loc )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_rwlock_unlock( &ilk->il_rwlock );
    PTTRACE( "rwunlock", plk, NULL, release_loc, rc );
    if (rc)
        loglock( ilk, rc, "release_rwlock", release_loc );
    hthread_mutex_lock( &ilk->il_locklock );
    {
        ilk->il_ob_locat = "null:0";
        ilk->il_ob_tid = 0;
    }
    hthread_mutex_unlock( &ilk->il_locklock );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Destroy a lock                                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_destroy_lock( LOCK* plk, const char* destroy_loc )
{
    int rc1, rc2 = 0;
    ILOCK* ilk = (ILOCK*) plk->ilk;

    if (!ilk)
        return EINVAL;

    /* Destroy the Internal ILOCK structure lock */
    if ((rc1 = hthread_mutex_destroy( &ilk->il_locklock )) == 0)
    {
        /* Now destroy the actual locking model HLOCK lock */
        if ((rc2 = hthread_mutex_destroy( &ilk->il_lock )) != 0)
            loglock( ilk, rc2, "destroy_lock", destroy_loc );
    }
    else
        loglock( ilk, rc1, "destroy_lock", destroy_loc );

    if (rc1 == 0 && rc2 == 0)
    {
        LockLocksList();
        {
            RemoveListEntry( &ilk->il_link );
            lockcount--;
        }
        UnlockLocksList();

        free( ilk->il_name );
        free_aligned( ilk );
        plk->ilk = NULL;
    }

    return (rc1 ? rc1 : (rc2 ? rc2 : 0));
}

/*-------------------------------------------------------------------*/
/* Destroy a R/W lock                                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_destroy_rwlock( RWLOCK* plk, const char* destroy_loc )
{
    int rc1, rc2 = 0;
    ILOCK* ilk = (ILOCK*) plk->ilk;

    if (!ilk)
        return EINVAL;

    /* Destroy the Internal ILOCK structure HLOCK */
    if ((rc1 = hthread_mutex_destroy( &ilk->il_locklock )) == 0)
    {
        /* Now destroy the actual locking model HRWLOCK rwlock */
        if ((rc2 = hthread_rwlock_destroy( &ilk->il_rwlock )) != 0)
            loglock( ilk, rc2, "destroy_rwlock", destroy_loc );
    }
    else
        loglock( ilk, rc1, "destroy_rwlock", destroy_loc );

    if (rc1 == 0 && rc2 == 0)
    {
        LockLocksList();
        {
            RemoveListEntry( &ilk->il_link );
            lockcount--;
        }
        UnlockLocksList();

        free( ilk->il_name );
        free_aligned( ilk );
        plk->ilk = NULL;
    }

    return (rc1 ? rc1 : (rc2 ? rc2 : 0));
}

/*-------------------------------------------------------------------*/
/* Try to obtain a lock                                              */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_try_obtain_lock( LOCK* plk, const char* obtain_loc )
{
    int rc;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "try before", plk, NULL, obtain_loc, PTT_MAGIC );
    rc = hthread_mutex_trylock( &ilk->il_lock );
    gettimeofday( &tv, NULL );
    PTTRACE2( "try after", plk, NULL, obtain_loc, rc, &tv );
    if (rc && EBUSY != rc)
        loglock( ilk, rc, "try_obtain_lock", obtain_loc );
    if (!rc || EOWNERDEAD == rc)
    {
        hthread_mutex_lock( &ilk->il_locklock );
        {
            ilk->il_ob_locat = obtain_loc;
            ilk->il_ob_tid = hthread_self();
            memcpy( &ilk->il_ob_time, &tv, sizeof( TIMEVAL ));
        }
        hthread_mutex_unlock( &ilk->il_locklock );
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
DLL_EXPORT int  hthread_test_lock( LOCK* plk )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_mutex_trylock( &ilk->il_lock );
    if (rc)
        return rc;
    hthread_mutex_unlock( &ilk->il_lock );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Check if *YOU* are holding a given lock        (boolean function) */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_have_lock( LOCK* plk )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_equal_threads( hthread_self(), ilk->il_ob_tid );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Obtain read-only access to R/W lock                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_obtain_rdlock( RWLOCK* plk, const char* obtain_loc )
{
    int rc;
    U64 waitdur;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    hthread_obtaining_lock( (LOCK*) plk, obtain_loc );
    PTTRACE( "rdlock before", plk, NULL, obtain_loc, PTT_MAGIC );
    rc = hthread_rwlock_tryrdlock( &ilk->il_rwlock );
    if (EBUSY == rc)
    {
        waitdur = host_tod();
        rc = hthread_rwlock_rdlock( &ilk->il_rwlock );
        waitdur = host_tod() - waitdur;
    }
    else
        waitdur = 0;
    PTTRACE( "rdlock after", plk, (void*) waitdur, obtain_loc, rc );
    hthread_lock_obtained();
    if (rc)
        loglock( ilk, rc, "obtain_rdloc", obtain_loc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Obtain exclusive write access to a R/W lock                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_obtain_wrlock( RWLOCK* plk, const char* obtain_loc )
{
    int rc;
    U64 waitdur;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    hthread_obtaining_lock( (LOCK*) plk, obtain_loc );
    PTTRACE( "wrlock before", plk, NULL, obtain_loc, PTT_MAGIC );
    rc = hthread_rwlock_trywrlock( &ilk->il_rwlock );
    if (EBUSY == rc)
    {
        waitdur = host_tod();
        rc = hthread_rwlock_wrlock( &ilk->il_rwlock );
        gettimeofday( &tv, NULL );
        waitdur = host_tod() - waitdur;
    }
    else
    {
        gettimeofday( &tv, NULL );
        waitdur = 0;
    }
    PTTRACE2( "wrlock after", plk, (void*) waitdur, obtain_loc, rc, &tv );
    hthread_lock_obtained();
    if (rc)
        loglock( ilk, rc, "obtain_wrlock", obtain_loc );
    if (!rc || EOWNERDEAD == rc)
    {
        hthread_mutex_lock( &ilk->il_locklock );
        {
            ilk->il_ob_locat = obtain_loc;
            ilk->il_ob_tid = hthread_self();
            memcpy( &ilk->il_ob_time, &tv, sizeof( TIMEVAL ));
        }
        hthread_mutex_unlock( &ilk->il_locklock );
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Try to obtain read-only access to a R/W lock                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_try_obtain_rdlock( RWLOCK* plk, const char* obtain_loc )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "tryrd before", plk, NULL, obtain_loc, PTT_MAGIC );
    rc = hthread_rwlock_tryrdlock( &ilk->il_rwlock );
    PTTRACE( "tryrd after", plk, NULL, obtain_loc, rc );
    if (rc && EBUSY != rc)
        loglock( ilk, rc, "try_obtain_rdlock", obtain_loc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Try to obtain exclusive write access to a R/W lock                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_try_obtain_wrlock( RWLOCK* plk, const char* obtain_loc )
{
    int rc;
    ILOCK* ilk;
    TIMEVAL tv;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "trywr before", plk, NULL, obtain_loc, PTT_MAGIC );
    rc = hthread_rwlock_trywrlock( &ilk->il_rwlock );
    gettimeofday( &tv, NULL );
    PTTRACE2( "trywr after", plk, NULL, obtain_loc, rc, &tv );
    if (rc && EBUSY != rc)
        loglock( ilk, rc, "try_obtain_wrlock", obtain_loc );
    if (!rc)
    {
        hthread_mutex_lock( &ilk->il_locklock );
        {
            ilk->il_ob_locat = obtain_loc;
            ilk->il_ob_tid = hthread_self();
            memcpy( &ilk->il_ob_time, &tv, sizeof( TIMEVAL ));
        }
        hthread_mutex_unlock( &ilk->il_locklock );
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Test if read-only access to a R/W lock is held                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_test_rdlock( RWLOCK* plk )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_rwlock_tryrdlock( &ilk->il_rwlock );
    if (rc)
        return rc;
    hthread_rwlock_unlock( &ilk->il_rwlock );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Test if exclusive write access to a R/W lock is held              */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_test_wrlock( RWLOCK* plk )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    rc = hthread_rwlock_trywrlock( &ilk->il_rwlock );
    if (rc)
        return rc;
    hthread_rwlock_unlock( &ilk->il_rwlock );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Initialize a condition variable                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_condition( COND* plc, const char* init_loc )
{
    int rc;
    PTTRACE( "cond init", NULL, plc, init_loc, PTT_MAGIC );
    rc = hthread_cond_init( plc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Destroy a condition variable                                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_destroy_condition( COND* plc )
{
    int rc;
    rc = hthread_cond_destroy( plc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Signal a condition variable (releases only one thread)            */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_signal_condition( COND* plc, const char* sig_loc )
{
    int rc;
    rc = hthread_cond_signal( plc );
    PTTRACE( "signal", NULL, plc, sig_loc, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Broadcast a condition variable (releases all waiting threads)     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_broadcast_condition( COND* plc, const char* bcast_loc )
{
    int rc;
    rc = hthread_cond_broadcast( plc );
    PTTRACE( "broadcast", NULL, plc, bcast_loc, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Wait for condition variable to be signaled                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_wait_condition( COND* plc, LOCK* plk, const char* wait_loc )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "wait before", plk, plc, wait_loc, PTT_MAGIC );
    rc = hthread_cond_wait( plc, &ilk->il_lock );
    PTTRACE( "wait after", plk, plc, wait_loc, rc );
    ilk->il_ob_tid = hthread_self();
    if (rc)
        loglock( ilk, rc, "wait_condition", wait_loc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Timed wait for a condition variable to be signaled                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_timed_wait_condition( COND* plc, LOCK* plk,
                                              const struct timespec* tm,
                                              const char* wait_loc )
{
    int rc;
    ILOCK* ilk;
    ilk = (ILOCK*) plk->ilk;
    PTTRACE( "tw before", plk, plc, wait_loc, PTT_MAGIC );
    rc = hthread_cond_timedwait( plc, &ilk->il_lock, tm );
    PTTRACE( "tw after", plk, plc, wait_loc, rc );
    ilk->il_ob_tid = hthread_self();
    if (rc && ETIMEDOUT != rc)
        loglock( ilk, rc, "timed_wait_condition", wait_loc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Internal helper function to initialize a thread attribute         */
/*-------------------------------------------------------------------*/
static INLINE int hthread_init_thread_attr( ATTR* pat, int state )
{
    int rc;
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
DLL_EXPORT int  hthread_initialize_join_attr( ATTR* pat )
{
    int rc;
    rc = hthread_init_thread_attr( pat, HTHREAD_CREATE_JOINABLE );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Initialize a thread attribute to "detached" (non-joinable)        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_initialize_detach_attr( ATTR* pat )
{
    int rc;
    rc = hthread_init_thread_attr( pat, HTHREAD_CREATE_DETACHED );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Internal function to list abandoned locks when thread exits       */
/*-------------------------------------------------------------------*/
static void hthread_list_abandoned_locks( TID tid, const char* exit_loc )
{
    ILOCK*       ilk;
    LIST_ENTRY*  ple;
    char         threadname[16];

    if (sysblk.shutdown)
        return;

    get_thread_name( tid, threadname );

    LockLocksList();
    {
        for (ple = locklist.Flink; ple != &locklist; ple = ple->Flink)
        {
            ilk = CONTAINING_RECORD( ple, ILOCK, il_link );
            if (hthread_equal( ilk->il_ob_tid, tid ))
            {
                char tod[27];           /* "YYYY-MM-DD HH:MM:SS.uuuuuu"  */

                FormatTIMEVAL( &ilk->il_ob_time, tod, sizeof( tod ));

                if (exit_loc)
                {
                    // "Thread "TIDPAT" (%s) has abandoned at %s lock %s obtained on %s at %s"
                    WRMSG( HHC90016, "E", TID_CAST( tid ), threadname, TRIMLOC( exit_loc ),
                        ilk->il_name, &tod[11], TRIMLOC( ilk->il_ob_locat ));
                }
                else
                {
                    // "Thread "TIDPAT" (%s) has abandoned lock %s obtained on %s at %s"
                    WRMSG( HHC90015, "E", TID_CAST( tid ), threadname,
                        ilk->il_name, &tod[11], TRIMLOC( ilk->il_ob_locat ));
                }
            }
        }
    }
    UnlockLocksList();
}

/*-------------------------------------------------------------------*/
/* Internal thread function to remove a thread from our list         */
/*-------------------------------------------------------------------*/
static void hthread_has_exited( TID tid, const char* exit_loc )
{
    hthread_list_abandoned_locks( tid, exit_loc );

    LockThreadsList();
    {
        HTHREAD* ht = hthread_find_HTHREAD_locked( tid, NULL );
        if (ht)
        {
            RemoveListEntry( &ht->ht_link );
            threadcount--;
            free( ht->ht_name );
            free( ht->ht_ob_where );
            free_aligned( ht );
        }
    }
    UnlockThreadsList();
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
    {
        SET_THREAD_NAME_ID( tid, name );
        free( name );
    }
    rc = pfn( arg );
    hthread_has_exited( tid, NULL );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Create a new thread                                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_create_thread( TID* ptid, ATTR* pat,
                                       THREAD_FUNC* pfn, void* arg,
                                       const char* name,
                                       const char* create_loc )
{
    int rc;
    void** arg2;
    arg2 = malloc( 3 * sizeof( void* ));
    *(arg2+0) = (void*) pfn;
    *(arg2+1) = (void*) arg;
    *(arg2+2) = (void*) strdup( name );
    LockThreadsList();
    {
        if (0 == (rc = hthread_create( ptid, pat, hthread_func, arg2 )))
        {
            HTHREAD* ht = calloc_aligned( sizeof( HTHREAD ), 64 );

            InitializeListLink( &ht->ht_link );
            gettimeofday( &ht->ht_cr_time, NULL );

            ht->ht_cr_locat =  create_loc;
            ht->ht_name     =  strdup( name );
            ht->ht_tid      =  *ptid;
            ht->ht_ob_lock  =  NULL;

            InsertListHead( &threadlist, &ht->ht_link );
            threadcount++;
        }
    }
    UnlockThreadsList();
    PTTRACE( "create", (void*)*ptid, NULL, create_loc, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Wait for a thread to terminate                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_join_thread( TID tid, void** prc, const char* join_loc )
{
    int rc;
    PTTRACE( "join before", (void*) tid, prc ? *prc : NULL, join_loc, PTT_MAGIC );
    rc = hthread_join( tid, prc );
    PTTRACE( "join after",  (void*) tid, prc ? *prc : NULL, join_loc, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Detach from a thread (release resources or change to "detached")  */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_detach_thread( TID tid, const char* det_loc )
{
    int rc;
    PTTRACE( "dtch before", (void*) tid, NULL, det_loc, PTT_MAGIC );
    rc = hthread_detach( tid );
    PTTRACE( "dtch after", (void*) tid, NULL, det_loc, rc );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Exit immediately from a thread                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hthread_exit_thread( void* rc, const char* exit_loc )
{
    TID tid;
    tid = hthread_self();
    hthread_has_exited( tid, exit_loc );
    hthread_exit( rc );
}

/*-------------------------------------------------------------------*/
/* Compare two thread IDs                         (boolean function) */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  hthread_equal_threads( TID tid1, TID tid2 )
{
    int rc;
    rc = hthread_equal( tid1, tid2 );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Change a thread's dispatching priority    (HTHREADS function)     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hthread_set_thread_prio( TID tid, int prio, const char* prio_loc )
{
    int rc;
    struct sched_param param = {0};

    /* Convert hthread priority relative to 0 to pthread priority */
    param.sched_priority = sysblk.minprio + prio;

    if (equal_threads( tid, 0 ))
        tid = hthread_self();

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
                TRIMLOC( prio_loc ), rc, strerror( rc ));
        }
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* Retrieve a thread's dispatching priority   (HTHREADS function)    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hthread_get_thread_prio( TID tid, const char* prio_loc )
{
    int rc, policy, prio;
    struct sched_param  param = {0};

    if (equal_threads( tid, 0 ))
        tid = hthread_self();

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
                TRIMLOC( prio_loc ), rc, strerror( rc ));
        }
    }
    return prio;
}

/*-------------------------------------------------------------------*/
/* locks_cmd helper function: save private copy of all locks in list */
/*-------------------------------------------------------------------*/
static int hthreads_copy_locks_list( ILOCK** ppILOCK, LIST_ENTRY* anchor )
{
    ILOCK*       ilk;               /* Pointer to ILOCK structure    */
    ILOCK*       ilka;              /* Pointer to ILOCK array        */
    LIST_ENTRY*  ple;               /* Ptr to LIST_ENTRY structure   */
    int i, k;

    LockLocksList();
    {
        /* Allocate private list array */
        if (!(*ppILOCK = ilka = (ILOCK*) malloc( lockcount * sizeof( ILOCK ))))
        {
            UnlockLocksList();
            return 0;
        }

        /* Copy each live entry to the private array entry */
        for (i=0, ple = locklist.Flink; ple != &locklist; ple = ple->Flink, i++)
        {
            ilk = CONTAINING_RECORD( ple, ILOCK, il_link );
            memcpy( &ilka[i], ilk, sizeof( ILOCK ));
            ilka[i].il_name = strdup( ilk->il_name );
        }

        k = lockcount;  /* Save how entries there are */
    }
    UnlockLocksList();

    /* Chain their private array copy */
    InitializeListHead( anchor );
    for (i=0; i < k; i++)
        InsertListTail( anchor, &ilka[i].il_link );
    return k;
}

/*-------------------------------------------------------------------*/
/* locks_cmd sort functions                                          */
/*-------------------------------------------------------------------*/
static int lsortby_nam( const ILOCK* p1, const ILOCK* p2 )
{
    int rc = strcasecmp( p1->il_name, p2->il_name );
    return rc == 0 ? ((int)((S64)p1->il_addr - (S64)p2->il_addr)) : rc;
}
static int lsortby_ob_tim( const ILOCK* p1, const ILOCK* p2 )
{
    int rc  =  (p1->il_ob_time.tv_sec    !=     p2->il_ob_time.tv_sec)  ?
        ((long) p1->il_ob_time.tv_sec  - (long) p2->il_ob_time.tv_sec)  :
        ((long) p1->il_ob_time.tv_usec - (long) p2->il_ob_time.tv_usec) ;
    return rc == 0 ? lsortby_nam( p1, p2 ) : rc;
}
static int lsortby_ob_tid( const ILOCK* p1, const ILOCK* p2 )
{
    int rc = equal_threads( p1->il_ob_tid,             p2->il_ob_tid ) ? 0 :
                ((intptr_t) p1->il_ob_tid - (intptr_t) p2->il_ob_tid);
    return rc == 0 ? lsortby_nam( p1, p2 ) : rc;
}
static int lsortby_ob_loc( const ILOCK* p1, const ILOCK* p2 )
{
    int rc = strcasecmp( p1->il_ob_locat, p2->il_ob_locat );
    return rc == 0 ? lsortby_nam( p1, p2 ) : rc;
}

/*-------------------------------------------------------------------*/
/* locks_cmd - list internal locks                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int locks_cmd( int argc, char* argv[], char* cmdline )
{
    LIST_ENTRY  anchor;             /* Private locks list anchor     */
    ILOCK*      ilk;                /* Pointer to ILOCK array        */
    TID         tid = 0;            /* Requested thread id           */
    int         i, k, rc = 0;       /* Work vars and return code     */
    char        c;                  /* sscanf work; work flag        */
    TID_INT     tid_int;

    UNREFERENCED( cmdline );

    /*  Format: "locks [ALL|HELD|tid] [SORT NAME|{TID|OWNER}|{WHEN|TIME|TOD}|{WHERE|LOC}]"  */

         if (argc <= 1)               tid = (TID)  0;
    else if (CMD( argv[1], ALL,  3 )) tid = (TID)  0;
    else if (CMD( argv[1], HELD, 4 )) tid = (TID) -1;
    else if (sscanf( argv[1], SCN_TIDPAT "%c", &tid_int, &c ) != 1) {
        tid = (TID)tid_int;
        rc = -1;
    }

    if (!rc)
    {
        CMPFUNC* sortby = (CMPFUNC*) lsortby_nam;

        if (argc == 4)
        {
            if (!CMD( argv[2], SORT, 4 ))
                rc = -1;
            else
            {
                     if (CMD( argv[3], NAME,  4 )) sortby = (CMPFUNC*) lsortby_nam;
                else if (CMD( argv[3], TID,   3 )) sortby = (CMPFUNC*) lsortby_ob_tid;
                else if (CMD( argv[3], OWNER, 5 )) sortby = (CMPFUNC*) lsortby_ob_tid;
                else if (CMD( argv[3], WHEN,  4 )) sortby = (CMPFUNC*) lsortby_ob_tim;
                else if (CMD( argv[3], TIME,  4 )) sortby = (CMPFUNC*) lsortby_ob_tim;
                else if (CMD( argv[3], TOD,   3 )) sortby = (CMPFUNC*) lsortby_ob_tim;
                else if (CMD( argv[3], WHERE, 5 )) sortby = (CMPFUNC*) lsortby_ob_loc;
                else if (CMD( argv[3], LOC,   3 )) sortby = (CMPFUNC*) lsortby_ob_loc;
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
            k = hthreads_copy_locks_list( &ilk, &anchor );

            /* Sort them into the requested sequence */
            if (k)
            {
                char tod[32];           // "YYYY-MM-DD HH:MM:SS.uuuuuu"
                char threadname[16];

                qsort( ilk, k, sizeof( ILOCK ), sortby );

                /* Display the requested locks */
                for (c = 0 /* (zero locks found) */, i=0; i < k; i++ )
                {
                    /* ALL, HELD or specific TID? */
                    if (0
                        || !tid
                        || (equal_threads( tid, (TID) -1 ) && !equal_threads( ilk[i].il_ob_tid, 0 ))
                        || equal_threads( tid, ilk[i].il_ob_tid )
                    )
                    {
                        c = 1;  /* (at least one lock found) */

                        get_thread_name( ilk[i].il_cr_tid, threadname );
                        FormatTIMEVAL(  &ilk[i].il_cr_time, tod, sizeof( tod ));

                        // "Lock "PTR_FMTx" (%s) created by "TIDPAT" (%s) on %s at %s"
                        WRMSG( HHC90017, "I"

                            , PTR_CAST( ilk[i].il_addr )
                            , ilk[i].il_name

                            , TID_CAST( ilk[i].il_cr_tid )
                            , threadname
                            , &tod[11]
                            , TRIMLOC( ilk[i].il_cr_locat )
                        );

                        if (ilk[i].il_ob_tid)
                        {
                            get_thread_name( ilk[i].il_ob_tid, threadname );
                            FormatTIMEVAL(  &ilk[i].il_ob_time, tod, sizeof( tod ));

                            // "Lock "PTR_FMTx" (%s) obtained by "TIDPAT" (%s) on %s at %s"
                            WRMSG( HHC90029, "I"

                                , PTR_CAST( ilk[i].il_addr )
                                , ilk[i].il_name

                                , TID_CAST( ilk[i].il_ob_tid )
                                , threadname
                                , &tod[11]
                                , TRIMLOC( ilk[i].il_ob_locat )
                            );
                        }
                    }
                }

                /* Free our copy of the locks list */
                for (i=0; i < k; i++)
                    free( ilk[i].il_name );
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
                WRMSG( HHC90018, "I", k );
            }
        }
    }

    if (rc)
    {
        // "Invalid argument(s). Type 'help %s' for assistance."
        WRMSG( HHC02211, "E", argv[0] );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*               Update an internal lock's name                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hthread_set_lock_name( LOCK* plk, const char* name )
{
    LockLocksList();
    {
        ILOCK* ilk = hthreads_find_ILOCK_locked( plk, NULL );
        if (ilk)
        {
            free( ilk->il_name );
            ilk->il_name = strdup( name );
        }
    }
    UnlockLocksList();
}

/*-------------------------------------------------------------------*/
/*               Update an internal thread's name                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT void hthread_set_thread_name( TID tid, const char* name )
{
    if (!inited)
        hthreads_internal_init();

    LockThreadsList();
    {
        HTHREAD* ht = hthread_find_HTHREAD_locked( tid, NULL );
        if (ht)
        {
            free( ht->ht_name );
            ht->ht_name = strdup( name );
        }
    }
    UnlockThreadsList();
}

/*-------------------------------------------------------------------*/
/*           Retrieve a copy of a given lock's name                  */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* hthread_get_lock_name( const LOCK* plk )
{
    const char* name;
    LockLocksList();
    {
        ILOCK* ilk = hthreads_find_ILOCK_locked( plk, NULL );
        name = ilk ? ilk->il_name : "";
    }
    UnlockLocksList();
    return name;
}

/*-------------------------------------------------------------------*/
/*           Retrieve a copy of a given thread's name                */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* hthread_get_thread_name( TID tid, char* buffer16 )
{
    LockThreadsList();
    {
        HTHREAD* ht = hthread_find_HTHREAD_locked( tid, NULL );
        strlcpy( buffer16, ht ? ht->ht_name : "", 16 );
    }
    UnlockThreadsList();
    return buffer16;
}

/*-------------------------------------------------------------------*/
/* threads_cmd helper: save private copy of all threads in list      */
/*-------------------------------------------------------------------*/
static int hthreads_copy_threads_list( HTHREAD** ppHTHREAD, LIST_ENTRY* anchor )
{
    HTHREAD*     ht;                /* Pointer to HTHREAD structure  */
    HTHREAD*     hta;               /* Pointer to HTHREAD array      */
    LIST_ENTRY*  ple;               /* Ptr to LIST_ENTRY structure   */
    int i, k;

    LockThreadsList();
    {
        /* Allocate private list array */
        if (!(*ppHTHREAD = hta = (HTHREAD*) malloc( threadcount * sizeof( HTHREAD ))))
        {
            UnlockThreadsList();
            return 0;
        }

        /* Copy each live entry to the private array entry */
        for (i=0, ple = threadlist.Flink; ple != &threadlist; ple = ple->Flink, i++)
        {
            ht = CONTAINING_RECORD( ple, HTHREAD, ht_link );
            memcpy( &hta[i], ht, sizeof( HTHREAD ));
            hta[i].ht_name = strdup( ht->ht_name );
            hta[i].ht_footprint = false;
        }

        k = threadcount;  /* Save how entries there are */
    }
    UnlockThreadsList();

    /* Chain their private array copy */
    InitializeListHead( anchor );
    for (i=0; i < k; i++)
        InsertListTail( anchor, &hta[i].ht_link );
    return k;
}

/*-------------------------------------------------------------------*/
/* threads_cmd sort functions                                        */
/*-------------------------------------------------------------------*/
static int tsortby_nam( const HTHREAD* p1, const HTHREAD* p2 )
{
    return strcasecmp( p1->ht_name, p2->ht_name );
}
static int tsortby_tim( const HTHREAD* p1, const HTHREAD* p2 )
{
    return     (p1->ht_cr_time.tv_sec    !=     p2->ht_cr_time.tv_sec)  ?
        ((long) p1->ht_cr_time.tv_sec  - (long) p2->ht_cr_time.tv_sec)  :
        ((long) p1->ht_cr_time.tv_usec - (long) p2->ht_cr_time.tv_usec) ;
}
static int tsortby_tid( const HTHREAD* p1, const HTHREAD* p2 )
{
    return equal_threads( p1->ht_tid,             p2->ht_tid ) ? 0 :
              ((intptr_t) p1->ht_tid - (intptr_t) p2->ht_tid);
}
static int tsortby_loc( const HTHREAD* p1, const HTHREAD* p2 )
{
    return strcasecmp( p1->ht_cr_locat, p2->ht_cr_locat );
}

/*-------------------------------------------------------------------*/
/* threads_cmd - list threads                                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int threads_cmd( int argc, char* argv[], char* cmdline )
{
    LIST_ENTRY  anchor;             /* Private threads list anchor   */
    HTHREAD*     ht;                /* Pointer to HTHREAD array      */
    char         tod[27];           /* "YYYY-MM-DD HH:MM:SS.uuuuuu"  */
    char         ht_ob_time[27];    /* "YYYY-MM-DD HH:MM:SS.uuuuuu"  */
    TID          tid = 0;           /* Requested thread id           */
    TID_INT      tid_int;
    int          i, k, rc = 0;      /* Work vars and return code     */
    char         c;                 /* sscanf work; work flag        */

    UNREFERENCED( cmdline );

    // Format:  threads [ALL|WAITING|tid] [SORT NAME|TID|{WHEN|TIME|TOD}|{WHERE|LOC}]

         if (argc <= 1)                  tid = (TID)  0;
    else if (CMD( argv[1], ALL,     3 )) tid = (TID)  0;
    else if (CMD( argv[1], WAITING, 7 )) tid = (TID) -1;
    else if (sscanf( argv[1], SCN_TIDPAT "%c", &tid_int, &c ) != 1) {
        tid = (TID)tid_int;
        rc = -1;
    }

    if (!rc)
    {
        CMPFUNC* sortby = (CMPFUNC*) tsortby_tim;

        if (argc == 4)
        {
            if (!CMD( argv[2], SORT, 4 ))
                rc = -1;
            else
            {
                     if (CMD( argv[3], NAME,  4 )) sortby = (CMPFUNC*) tsortby_nam;
                else if (CMD( argv[3], TID,   3 )) sortby = (CMPFUNC*) tsortby_tid;
                else if (CMD( argv[3], WHEN,  4 )) sortby = (CMPFUNC*) tsortby_tim;
                else if (CMD( argv[3], TIME,  4 )) sortby = (CMPFUNC*) tsortby_tim;
                else if (CMD( argv[3], TOD,   3 )) sortby = (CMPFUNC*) tsortby_tim;
                else if (CMD( argv[3], WHERE, 5 )) sortby = (CMPFUNC*) tsortby_loc;
                else if (CMD( argv[3], LOC,   3 )) sortby = (CMPFUNC*) tsortby_loc;
                else
                    rc = -1;
            }
        }
        else if (argc != 1 && argc != 2)
            rc = -1;

        /* If no errors, perform the requested function */
        if (!rc)
        {
            /* Retrieve a copy of the threads list */
            k = hthreads_copy_threads_list( &ht, &anchor );

            /* Sort them into the requested sequence */
            if (k)
            {
                qsort( ht, k, sizeof( HTHREAD ), sortby );

                /* Display the requested threads */
                for (c = 0 /* (zero locks found) */, i=0; i < k; i++)
                {
                    /* ALL, WAITING or specific TID? */
                    if (0
                        || !tid
                        || (equal_threads( tid, (TID) -1 ) && ht[i].ht_ob_lock)
                        || equal_threads( tid, ht[i].ht_tid )
                    )
                    {
                        c = 1;  /* (at least one lock found) */

                        FormatTIMEVAL( &ht[i].ht_cr_time, tod, sizeof( tod ));

                        if (equal_threads( tid, (TID) -1 ) && ht[i].ht_ob_lock)
                        {
                            FormatTIMEVAL( &ht[i].ht_ob_time, ht_ob_time, sizeof( ht_ob_time ));

                            // "Thread %-15.15s tid="TIDPAT" waiting since %s at %s for lock %s = "PTR_FMTx

                            WRMSG( HHC90023, "W", ht[i].ht_name, TID_CAST( ht[i].ht_tid ),
                                &ht_ob_time[11], TRIMLOC( ht[i].ht_ob_where ),
                                get_lock_name( ht[i].ht_ob_lock ), PTR_CAST( ht[i].ht_ob_lock ));
                        }
                        else
                        {
                            // "Thread %-15.15s tid="TIDPAT" created on %s at %-18.18s"

                            WRMSG( HHC90022, "I", ht[i].ht_name, TID_CAST( ht[i].ht_tid ),
                                &tod[11], TRIMLOC( ht[i].ht_cr_locat ));
                        }
                    }
                }

                /* Free our copy of the threads list */
                for (i=0; i < k; i++)
                    free( ht[i].ht_name );
                free( ht );
            }
            else
                c = 1;

            /* Print results */
            if (!c)
            {
                // "No threads found with tid "TIDPAT"."
                WRMSG( HHC90026, "W", TID_CAST( tid ));
            }
            else if (!tid)
            {
                // "Total threads running: %d"
                WRMSG( HHC90027, "I", k );
            }
        }
    }

    if (rc)
    {
        // "Invalid argument(s). Type 'help %s' for assistance."
        WRMSG( HHC02211, "E", argv[0] );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Check if thread is deadlocked: true = deadlocked, false = not.    */
/* Note: caller is responsible for locking both lists beforehand!    */
/*-------------------------------------------------------------------*/
static bool hthread_is_deadlocked_locked( const char* sev, TID tid,
                                          LIST_ENTRY* ht_anchor,
                                          LIST_ENTRY* ilk_anchor )
{
    HTHREAD*  ht;
    HTHREAD*  ht2;
    ILOCK*    ilk;
    struct timeval  now;    // Current time of day.
    struct timeval  dur;    // How long we've been waiting for lock.

    // TECHNIQUE: starting with the requested thread, chase the lock
    // owner chain until we eventually reach a thread we have already
    // seen, at which point a deadlock has been detected.
    //
    // As each thread is chased, we set a footprint in order to know
    // whether or not we have seen this thread before.
    //
    // We stop our chase and immediately return false (no deadlock)
    // as soon as we reach a thread that is either not waiting for
    // any lock or is not waiting long enough for a lock.

    /* Start clean: remove all previous footprints */
    gettimeofday( &now, NULL );
    {
        LIST_ENTRY* le = ht_anchor->Flink;
        while (le != ht_anchor)
        {
            ht = CONTAINING_RECORD( le, HTHREAD, ht_link );
            ht->ht_footprint = false;
            le = le->Flink;
        }
    }

    /* Locate the thread entry for the thread we're interested in */
    if (!(ht = hthread_find_HTHREAD_locked( tid, ht_anchor )))
        return false;

    /* Chase thread's lock chain until deadlock detected */
    while (!ht->ht_footprint)
    {
        /* Remember that we've been here before */
        ht->ht_footprint = true;

        /* Is this thread waiting for a lock? */
        if (!ht->ht_ob_lock)
            return false;

        /* Has thread been waiting for this lock for a long time? */
        timeval_subtract( &ht->ht_ob_time, &now, &dur );
        if (dur.tv_sec < DEADLOCK_SECONDS)
            return false;

        /* Remember which thread is waiting to obtain a lock */
        ht2 = ht;

        /* Locate the lock entry that this thread is waiting for */
        if (!(ilk = hthreads_find_ILOCK_locked( ht->ht_ob_lock, ilk_anchor )))
            return false;

        /* Now switch over to chasing the lock chain for the thread
           currently owning the lock our thread is waiting to obtain.
        */
        if (!(ht = hthread_find_HTHREAD_locked( ilk->il_ob_tid, ht_anchor )))
            return false;

        // PROGRAMMING NOTE: due to the way hthreads tracks lock
        // attempts and lock acquisition (wherein it first marks
        // that a lock is ATTEMPTING to be obtained (by setting
        // ht_ob_lock), then AFTERWARDS marking that it has now
        // been successfully obtained (by CLEARING ht_ob_lock),
        // (which are two separate function calls), it becomes
        // possible for our COPIED locks list to indicate that
        // our thread is still waiting for a lock (as indicated
        // by "ht_ob_lock" being non-NULL in our COPIED threads
        // list) that in fact (in actuality) has already been
        // successfully obtained! (as inciated by our COPIED
        // locks list entry indicating that it is in fact OUR
        // thread that currently owns the lock in question!)
        //
        // That is to say, our working COPIES of the live locks
        // and threads lists, by coincidence, was made AFTER
        // the lock's "il_ob_tid" field was set to our thread's
        // id (indicating our thread successfully obtained the
        // lock) but BEFORE the thread entry's "ht_ob_lock" field
        // could be cleared to indicate the thread was no longer
        // waiting for the lock.
        //
        // Therefore we must always check whether the thread that
        // currently owns the lock that we are supposedly waiting
        // to obtain happens to be OURSELVES or not! If the thread
        // that currently owns the lock is not ourselves, then fine,
        // we continue chasing our lock chain for this new thread.
        // But if the lock owner is in fact OURSELVES, there is
        // obviously is no deadlock so we immediately return false.

        if (ht == ht2)          // (lock owned by OURSELVES?!)
            return false;       // (then obviously no deadlock!)
    }

    /* Report the deadlock if asked to do so */
    if (sev)
    {
        ht  = hthread_find_HTHREAD_locked( tid, ht_anchor );
        ilk = hthreads_find_ILOCK_locked( ht->ht_ob_lock, ilk_anchor );
        ht2 = hthread_find_HTHREAD_locked( ilk->il_ob_tid, ht_anchor );

        // "Thread %s is stuck waiting for lock %s held by thread %s"
        WRMSG( HHC90025, sev, ht->ht_name, ilk->il_name, ht2->ht_name );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/* Detect and report deadlocks - return number of deadlocked threads */
/*-------------------------------------------------------------------*/
DLL_EXPORT int hthread_report_deadlocks( const char* sev )
{
    HTHREAD*    ht;                 /* Private threads array         */
    LIST_ENTRY  ht_anchor;          /* Private threads array anchor  */
    int         ht_count;           /* Number of entries in array    */
    ILOCK*      ilk;                /* Private locks array           */
    LIST_ENTRY  ilk_anchor;         /* Private locks array anchor    */
    int         ilk_count;          /* Number of entries in array    */
    int         i, deadlocked = 0;  /* Work variables                */

    /* Retrieve a private array copy of both lists */
    ht_count  = hthreads_copy_threads_list ( &ht,  &ht_anchor  );
    ilk_count = hthreads_copy_locks_list   ( &ilk, &ilk_anchor );

    /* Check each thread in our array */
    for (i=0; i < ht_count; i++)
        if (hthread_is_deadlocked_locked( sev, ht[i].ht_tid, &ht_anchor, &ilk_anchor ))
            deadlocked++; /* count deadlocked threads */

    /* Free our private array copies of both lists */
    for (i=0; i < ht_count; i++)
        free( ht[i].ht_name );
    free( ht );
    for (i=0; i < ilk_count; i++)
        free( ilk[i].il_name );
    free( ilk );

    /* Return number of deadlocked threads detected */
    return deadlocked;
}
