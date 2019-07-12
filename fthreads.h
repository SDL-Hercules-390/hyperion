/*  FTHREADS.H  (C) Copyright "Fish" (David B. Trout), 2001-2012     */
/*              Fish's WIN32 version of pthreads                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _FTHREADS_H_
#define _FTHREADS_H_

#include "hercules.h"

////////////////////////////////////////////////////////////////////////////////////
// fthread mutex attribute types...
//
//  FTHREAD_MUTEX_NORMAL      This type of mutex does not detect deadlock. A thread
//                            attempting to relock this mutex without first unlocking
//                            it shall deadlock. Attempting to unlock a mutex locked
//                            by a different thread results in undefined behavior.
//                            Attempting to unlock an unlocked mutex results in
//                            undefined behavior. The FTHREAD_MUTEX_NORMAL mutex type
//                            is not currently supported by fthreads.
//
//  FTHREAD_MUTEX_ERRORCHECK  This type of mutex provides error checking. A thread
//                            attempting to relock this mutex without first unlocking
//                            it shall return with an error. A thread attempting to
//                            unlock a mutex which another thread has locked shall
//                            return with an error. A thread attempting to unlock an
//                            unlocked mutex shall return with an error.
//
//  FTHREAD_MUTEX_RECURSIVE   A thread attempting to relock this mutex without first
//                            unlocking it shall succeed in locking the mutex. The
//                            relocking deadlock which can occur with mutexes of type
//                            FTHREAD_MUTEX_NORMAL cannot occur with this type of mutex.
//                            Multiple locks of this mutex shall require the same number
//                            of unlocks to release the mutex before another thread
//                            can acquire the mutex. A thread attempting to unlock a
//                            mutex which another thread has locked shall return with
//                            an error. A thread attempting to unlock an unlocked mutex
//                            shall return with an error.

#define  FTHREAD_MUTEX_NORMAL       0x44656164                  // "Dead" in ASCII  (*UNSUPPORTED*)
#define  FTHREAD_MUTEX_ERRORCHECK   0x4F6E6365                  // "Once" in ASCII
#define  FTHREAD_MUTEX_RECURSIVE    0x4D616E79                  // "Many" in ASCII
#define  FTHREAD_MUTEX_DEFAULT      FTHREAD_MUTEX_ERRORCHECK

////////////////////////////////////////////////////////////////////////////////////
// fthread thread attribute types...

#define  FTHREAD_CREATE_JOINABLE    0x4A6F696E                  // "Join" in ASCII
#define  FTHREAD_CREATE_DETACHED    0x44697363                  // "Disc" in ASCII
#define  FTHREAD_CREATE_DEFAULT     FTHREAD_CREATE_JOINABLE

////////////////////////////////////////////////////////////////////////////////////
// Just a handy macro to have around...

#define RC(rc)  (rc)

////////////////////////////////////////////////////////////////////////////////////
// (need struct timespec for fthread_cond_timedwait)

#if !defined( TIMESPEC_IN_SYS_TYPES_H ) && !defined( TIMESPEC_IN_TIME_H ) && (_MSC_VER < VS2015)
  // Avoid double definition for VS2015 as it defines timespec in ...\ucrt\time.h(39)
  // (need to define it ourselves)
  struct timespec
  {
      time_t  tv_sec;     // (seconds)
      long    tv_nsec;    // (nanoseconds)
  };
#endif

////////////////////////////////////////////////////////////////////////////////////
// fthread typedefs...

typedef void*                    FT_W32_HANDLE;       // HANDLE
typedef unsigned long            FT_W32_DWORD;        // DWORD

typedef FT_W32_DWORD             fthread_t;           // thread id
typedef FT_W32_DWORD             fthread_mutexattr_t; // mutex attribute

typedef void* (FT_THREAD_FUNC)(void*);                // thread function
typedef FT_THREAD_FUNC* PFT_THREAD_FUNC;              // thread function ptr

typedef struct _tagFTU_MUTEX        // fthread "mutex" structure
{
    FT_W32_HANDLE  hMutex;          // (ptr to actual mutex structure)
}
fthread_mutex_t;

typedef struct _tagFTU_COND         // fthread "condition variable" structure
{
    FT_W32_HANDLE  hCondVar;        // (ptr to actual condition variable structure)
}
fthread_cond_t;

typedef struct _tagFTU_ATTR         // fthread "thread attribute" structure
{
    size_t        nStackSize;       // (initial stack size in bytes)
    int           nDetachState;     // (requested detach state: detached/joinable)
}
fthread_attr_t;

////////////////////////////////////////////////////////////////////////////////////
// Thread Scheduling...

#define SCHED_RR                (1)     // Same as Hercules
#define FTHREAD_POLICY        SCHED_RR  // Our only valid scheduling policy

// The following relative priority scheme should match hthread's scheme. That
// is to say, we should have the same number of priority "slots" as hthreads.
// This allows easy translation of any hthread (pthread) priority value to a
// Window host priority "slot".

#define FTHREAD_IDLE            (1)     // DEFAULT_XXX_PRIO
#define FTHREAD_LOWEST          (2)     // DEFAULT_CPU_PRIO
#define FTHREAD_BELOW_NORMAL    (3)     // DEFAULT_DEV_PRIO
#define FTHREAD_NORMAL          (4)     // DEFAULT_SRV_PRIO
#define FTHREAD_ABOVE_NORMAL    (5)     // DEFAULT_HERC_PRIO
#define FTHREAD_HIGHEST         (6)     // DEFAULT_XXX_PRIO
#define FTHREAD_TIME_CRITICAL   (7)     // DEFAULT_TOD_PRIO

#define FTHREAD_MIN_PRIO        (0)     // (POSIX Requires at least 32 slots)
#define FTHREAD_MAX_PRIO       (31)     // (POSIX Requires at least 32 slots)

struct sched_param                      // Scheduling parameters structure...
{
    int  sched_priority;                // fthread priority
};
typedef struct sched_param sched_param;

FT_DLL_IMPORT  void  fthreads_internal_init   ();
FT_DLL_IMPORT  int   fthread_getschedparam    ( fthread_t dwThreadID, int* pnPolicy,       struct sched_param* pSCHPARM );
FT_DLL_IMPORT  int   fthread_setschedparam    ( fthread_t dwThreadID, int   nPolicy, const struct sched_param* pSCHPARM );
FT_DLL_IMPORT  int   fthread_get_priority_min ( int nPolicy );
FT_DLL_IMPORT  int   fthread_get_priority_max ( int nPolicy );

////////////////////////////////////////////////////////////////////////////////////
// Initialize a "thread attribute"...

FT_DLL_IMPORT
int  fthread_attr_init
(
    fthread_attr_t*  pThreadAttr
);

////////////////////////////////////////////////////////////////////////////////////
// Destroy a "thread attribute"...

FT_DLL_IMPORT
int  fthread_attr_destroy
(
    fthread_attr_t*  pThreadAttr
);

////////////////////////////////////////////////////////////////////////////////////
// Set a thread's "detachstate" attribute...

FT_DLL_IMPORT
int  fthread_attr_setdetachstate
(
    fthread_attr_t*  pThreadAttr,
    int              nDetachState
);

////////////////////////////////////////////////////////////////////////////////////
// Retrieve a thread's "detachstate" attribute...

FT_DLL_IMPORT
int  fthread_attr_getdetachstate
(
    const fthread_attr_t*  pThreadAttr,
    int*                   pnDetachState
);

////////////////////////////////////////////////////////////////////////////////////
// Set a thread's initial stack size...

FT_DLL_IMPORT
int  fthread_attr_setstacksize
(
    fthread_attr_t*  pThreadAttr,
    size_t           nStackSize
);

////////////////////////////////////////////////////////////////////////////////////
// Retrieve a thread's initial stack size...

FT_DLL_IMPORT
int  fthread_attr_getstacksize
(
    const fthread_attr_t*  pThreadAttr,
    size_t*                pnStackSize
);

////////////////////////////////////////////////////////////////////////////////////
// Join a thread (i.e. wait for a thread's termination)...

FT_DLL_IMPORT
int  fthread_join
(
    fthread_t       dwThreadID,
    void**          pExitVal
);

////////////////////////////////////////////////////////////////////////////////////
// Detach a thread (i.e. ignore a thread's termination)...

FT_DLL_IMPORT
int  fthread_detach
(
    fthread_t  dwThreadID
);

/////////////////////////////////////////////////////////////////////////////////////
// Get the handle for a specific Thread ID

DLL_EXPORT
HANDLE fthread_get_handle
(
    fthread_t       dwThreadID             // Thread ID
);

////////////////////////////////////////////////////////////////////////////////////
// Create a new thread...

FT_DLL_IMPORT
int  fthread_create
(
    fthread_t*       pdwThreadID,
    fthread_attr_t*  pThreadAttr,
    PFT_THREAD_FUNC  pfnThreadFunc,
    void*            pvThreadArgs
);

////////////////////////////////////////////////////////////////////////////////////
// Exit from a thread...

FT_DLL_IMPORT
void  fthread_exit
(
    void*  ExitVal
);

////////////////////////////////////////////////////////////////////////////////////
// Return thread-id...

FT_DLL_IMPORT
fthread_t  fthread_self
(
);

////////////////////////////////////////////////////////////////////////////////////
// Compare thread-ids...

FT_DLL_IMPORT
int  fthread_equal
(
    fthread_t  dwThreadID_1,
    fthread_t  dwThreadID_2
);

////////////////////////////////////////////////////////////////////////////////////
// Initialize a "mutex"...

FT_DLL_IMPORT
int  fthread_mutex_init
(
          fthread_mutex_t*      pFT_MUTEX,
    const fthread_mutexattr_t*  pFT_MUTEX_ATTR
);

////////////////////////////////////////////////////////////////////////////////////
// Destroy a "mutex"...

FT_DLL_IMPORT
int  fthread_mutex_destroy
(
    fthread_mutex_t*  pFT_MUTEX
);

////////////////////////////////////////////////////////////////////////////////////
// Lock a "mutex"...

FT_DLL_IMPORT
int  fthread_mutex_lock
(
    fthread_mutex_t*  pFT_MUTEX
);

////////////////////////////////////////////////////////////////////////////////////
// Try to lock a "mutex"...

FT_DLL_IMPORT
int  fthread_mutex_trylock
(
    fthread_mutex_t*  pFT_MUTEX
);

////////////////////////////////////////////////////////////////////////////////////
// Unlock a "mutex"...

FT_DLL_IMPORT
int  fthread_mutex_unlock
(
    fthread_mutex_t*  pFT_MUTEX
);

////////////////////////////////////////////////////////////////////////////////////
// Initialize a "condition"...

FT_DLL_IMPORT
int  fthread_cond_init
(
    fthread_cond_t*  pFT_COND_VAR
);

////////////////////////////////////////////////////////////////////////////////////
// Destroy a "condition"...

FT_DLL_IMPORT
int  fthread_cond_destroy
(
    fthread_cond_t*  pFT_COND_VAR
);

////////////////////////////////////////////////////////////////////////////////////
// 'Signal' a "condition"...   (causes ONE waiting thread to be released)

FT_DLL_IMPORT
int  fthread_cond_signal
(
    fthread_cond_t*  pFT_COND_VAR
);

////////////////////////////////////////////////////////////////////////////////////
// 'Broadcast' a "condition"...  (causes ALL waiting threads to be released)

FT_DLL_IMPORT
int  fthread_cond_broadcast
(
    fthread_cond_t*  pFT_COND_VAR
);

////////////////////////////////////////////////////////////////////////////////////
// Wait for a "condition" to occur...

FT_DLL_IMPORT
int  fthread_cond_wait
(
    fthread_cond_t*   pFT_COND_VAR,
    fthread_mutex_t*  pFT_MUTEX
);

////////////////////////////////////////////////////////////////////////////////////
// Wait (but not forever) for a "condition" to occur...

FT_DLL_IMPORT
int  fthread_cond_timedwait
(
    fthread_cond_t*         pFT_COND_VAR,
    fthread_mutex_t*        pFT_MUTEX,
    const struct timespec*  pTimeTimeout
);

////////////////////////////////////////////////////////////////////////////////////
// Initialize a "mutex" attribute...

FT_DLL_IMPORT
int  fthread_mutexattr_init
(
    fthread_mutexattr_t*  pFT_MUTEX_ATTR
);

////////////////////////////////////////////////////////////////////////////////////
// Destroy a "mutex" attribute...

FT_DLL_IMPORT
int  fthread_mutexattr_destroy
(
    fthread_mutexattr_t*  pFT_MUTEX_ATTR
);

////////////////////////////////////////////////////////////////////////////////////
// Retrieve "mutex" attribute type...

FT_DLL_IMPORT
int fthread_mutexattr_gettype
(
    const fthread_mutexattr_t*  pFT_MUTEX_ATTR,
    int*                        pnMutexType
);

////////////////////////////////////////////////////////////////////////////////////
// Set "mutex" attribute type...

FT_DLL_IMPORT
int fthread_mutexattr_settype
(
    fthread_mutexattr_t*  pFT_MUTEX_ATTR,
    int                   nMutexType
);

////////////////////////////////////////////////////////////////////////////////////

#endif // _FTHREADS_H_
