/*  FTHREADS.C  (C) Copyright "Fish" (David B. Trout), 2001-2012     */
/*              Fish's WIN32 version of pthreads                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _FTHREADS_C_
#define _HUTIL_DLL_

#include "hercules.h"

#if defined( OPTION_FTHREADS )
#include "fthreads.h"

////////////////////////////////////////////////////////////////////////////////////
// Private implementation helper macros

#define MyInitializeCriticalSection(pCS)                (InitializeCriticalSectionAndSpinCount((CRITICAL_SECTION*)(pCS),3000))
#define MyEnterCriticalSection(pCS)                     (EnterCriticalSection((CRITICAL_SECTION*)(pCS)))
#define MyTryEnterCriticalSection(pCS)                  (TryEnterCriticalSection((CRITICAL_SECTION*)(pCS)))
#define MyLeaveCriticalSection(pCS)                     (LeaveCriticalSection((CRITICAL_SECTION*)(pCS)))
#define MyDeleteCriticalSection(pCS)                    (DeleteCriticalSection((CRITICAL_SECTION*)(pCS)))

#ifdef _MSVC_
#define MyCreateThread(sec,stack,start,parm,flags,tid)  ((HANDLE) _beginthreadex((sec),(unsigned)(stack),(start),(parm),(flags),(tid)))
#define MyExitThread(code)                              (_endthreadex((code)))
#else // (Cygwin)
#define MyCreateThread(sec,stack,start,parm,flags,tid)  (CreateThread((sec),(stack),(start),(parm),(flags),(tid)))
#define MyExitThread(code)                              (ExitThread((code)))
#endif // _MSVC_

#define MyCreateEvent(sec,man,set,name)                 (CreateEvent((sec),(man),(set),(name)))
#define MySetEvent(h)                                   (SetEvent((h)))
#define MyResetEvent(h)                                 (ResetEvent((h)))
#define MyDeleteEvent(h)                                (CloseHandle((h)))
#define MyCloseHandle(h)                                (CloseHandle((h)))

#define MyWaitForSingleObject(h,millisecs)              (WaitForSingleObject((h),(millisecs)))

////////////////////////////////////////////////////////////////////////////////////
// Private internal fthreads structures...

typedef struct _tagFT_MUTEX             // fthread "mutex" structure
{
    CRITICAL_SECTION  MutexLock;        // (lock for accessing this data)
    HANDLE            hUnlockedEvent;   // (signalled while NOT locked)
    DWORD             dwMutexType;      // (type of mutex (normal, etc))
    DWORD             dwLockOwner;      // (thread-id of who owns it)
    int               nLockedCount;     // (#of times lock acquired)
}
FT_MUTEX, *PFT_MUTEX;

typedef struct _tagFT_COND_VAR          // fthread "condition variable" structure
{
    CRITICAL_SECTION  CondVarLock;      // (lock for accessing this data)
    HANDLE            hSigXmitEvent;    // set during signal transmission
    HANDLE            hSigRecvdEvent;   // set once signal received by every-
                                        // one that's supposed to receive it.
    BOOL              bBroadcastSig;    // TRUE = "broadcast", FALSE = "signal"
    int               nNumWaiting;      // #of threads waiting to receive signal
}
FT_COND_VAR, *PFT_COND_VAR;

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Private internal fthreads functions...

static BOOL  IsValidMutexType ( DWORD dwMutexType )
{
    return (0
//      || FTHREAD_MUTEX_NORMAL     == dwMutexType  // (*UNSUPPORTED*)
        || FTHREAD_MUTEX_RECURSIVE  == dwMutexType
        || FTHREAD_MUTEX_ERRORCHECK == dwMutexType
//      || FTHREAD_MUTEX_DEFAULT    == dwMutexType  // (FTHREAD_MUTEX_RECURSIVE)
    );
}

////////////////////////////////////////////////////////////////////////////////////

static FT_MUTEX*  MallocFT_MUTEX ( )
{
    FT_MUTEX*  pFT_MUTEX = (FT_MUTEX*) malloc ( sizeof ( FT_MUTEX ) );
    if ( !pFT_MUTEX ) return NULL;
    memset ( pFT_MUTEX, 0xCD, sizeof ( FT_MUTEX ) );
    return pFT_MUTEX;
}

////////////////////////////////////////////////////////////////////////////////////

static BOOL  InitializeFT_MUTEX
(
    FT_MUTEX*     pFT_MUTEX,
    DWORD         dwMutexType
)
{
    // Note: UnlockedEvent created initially signalled

    if ( !(pFT_MUTEX->hUnlockedEvent = MyCreateEvent ( NULL, TRUE, TRUE, NULL )) )
    {
        memset ( pFT_MUTEX, 0xCD, sizeof ( FT_MUTEX ) );
        return FALSE;
    }

    MyInitializeCriticalSection ( &pFT_MUTEX->MutexLock );

    pFT_MUTEX->dwMutexType    = dwMutexType;
    pFT_MUTEX->dwLockOwner    = 0;
    pFT_MUTEX->nLockedCount   = 0;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////

static BOOL  UninitializeFT_MUTEX
(
    FT_MUTEX*    pFT_MUTEX
)
{
    if ( pFT_MUTEX->nLockedCount > 0 )
        return FALSE;   // (still in use)

    ASSERT( IsEventSet ( pFT_MUTEX->hUnlockedEvent ) );

    MyDeleteEvent ( pFT_MUTEX->hUnlockedEvent );
    MyDeleteCriticalSection ( &pFT_MUTEX->MutexLock );

    memset ( pFT_MUTEX, 0xCD, sizeof ( FT_MUTEX ) );

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////

static FT_COND_VAR*  MallocFT_COND_VAR ( )
{
    FT_COND_VAR*  pFT_COND_VAR = (FT_COND_VAR*) malloc ( sizeof ( FT_COND_VAR ) );
    if ( !pFT_COND_VAR ) return NULL;
    memset ( pFT_COND_VAR, 0xCD, sizeof ( FT_COND_VAR ) );
    return pFT_COND_VAR;
}

////////////////////////////////////////////////////////////////////////////////////

static BOOL  InitializeFT_COND_VAR
(
    FT_COND_VAR*  pFT_COND_VAR
)
{
    if ( ( pFT_COND_VAR->hSigXmitEvent = MyCreateEvent ( NULL, TRUE, FALSE, NULL ) ) )
    {
        // Note: hSigRecvdEvent created initially signaled

        if ( ( pFT_COND_VAR->hSigRecvdEvent = MyCreateEvent ( NULL, TRUE, TRUE, NULL ) ) )
        {
            MyInitializeCriticalSection ( &pFT_COND_VAR->CondVarLock );

            pFT_COND_VAR->bBroadcastSig = FALSE;
            pFT_COND_VAR->nNumWaiting   = 0;

            return TRUE;
        }

        MyDeleteEvent ( pFT_COND_VAR->hSigXmitEvent );
    }

    memset ( pFT_COND_VAR, 0xCD, sizeof ( FT_COND_VAR ) );

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////

static BOOL  UninitializeFT_COND_VAR
(
    FT_COND_VAR*  pFT_COND_VAR
)
{
    if (0
        ||  pFT_COND_VAR->nNumWaiting
        ||  IsEventSet ( pFT_COND_VAR->hSigXmitEvent  )
        || !IsEventSet ( pFT_COND_VAR->hSigRecvdEvent )
    )
        return FALSE;

    MyDeleteEvent ( pFT_COND_VAR->hSigXmitEvent  );
    MyDeleteEvent ( pFT_COND_VAR->hSigRecvdEvent );

    MyDeleteCriticalSection ( &pFT_COND_VAR->CondVarLock );

    memset ( pFT_COND_VAR, 0xCD, sizeof ( FT_COND_VAR ) );

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////

static BOOL  TryEnterFT_MUTEX
(
    FT_MUTEX*    pFT_MUTEX
)
{
    BOOL   bSuccess;
    DWORD  dwThreadId = GetCurrentThreadId();

    if ( hostinfo.trycritsec_avail )
    {
        bSuccess = MyTryEnterCriticalSection ( &pFT_MUTEX->MutexLock );

        if ( bSuccess )
        {
            pFT_MUTEX->nLockedCount++;
            ASSERT( pFT_MUTEX->nLockedCount > 0 );
            pFT_MUTEX->dwLockOwner = dwThreadId;
        }
    }
    else
    {
        MyEnterCriticalSection ( &pFT_MUTEX->MutexLock );

        ASSERT ( pFT_MUTEX->nLockedCount >= 0 );

        bSuccess = ( pFT_MUTEX->nLockedCount <= 0 || pFT_MUTEX->dwLockOwner == dwThreadId );

        if ( bSuccess )
        {
            pFT_MUTEX->nLockedCount++;
            ASSERT ( pFT_MUTEX->nLockedCount > 0 );
            pFT_MUTEX->dwLockOwner = dwThreadId;
            MyResetEvent ( pFT_MUTEX->hUnlockedEvent );
        }

        MyLeaveCriticalSection ( &pFT_MUTEX->MutexLock );
    }

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////

static void  EnterFT_MUTEX
(
    FT_MUTEX*    pFT_MUTEX
)
{
    DWORD  dwThreadId = GetCurrentThreadId();

    if ( hostinfo.trycritsec_avail )
    {
        MyEnterCriticalSection ( &pFT_MUTEX->MutexLock );
        pFT_MUTEX->dwLockOwner = dwThreadId;
        pFT_MUTEX->nLockedCount++;
        ASSERT ( pFT_MUTEX->nLockedCount > 0 );
    }
    else
    {
        for (;;)
        {
            MyEnterCriticalSection ( &pFT_MUTEX->MutexLock );
            ASSERT ( pFT_MUTEX->nLockedCount >= 0 );
            if ( pFT_MUTEX->nLockedCount <= 0 || pFT_MUTEX->dwLockOwner == dwThreadId ) break;
            MyLeaveCriticalSection ( &pFT_MUTEX->MutexLock );
            MyWaitForSingleObject ( pFT_MUTEX->hUnlockedEvent, INFINITE );
        }

        MyResetEvent ( pFT_MUTEX->hUnlockedEvent );
        pFT_MUTEX->dwLockOwner = dwThreadId;
        pFT_MUTEX->nLockedCount++;
        ASSERT ( pFT_MUTEX->nLockedCount > 0 );
        MyLeaveCriticalSection ( &pFT_MUTEX->MutexLock );
    }
}

////////////////////////////////////////////////////////////////////////////////////

static void  LeaveFT_MUTEX
(
    FT_MUTEX*    pFT_MUTEX
)
{
    if ( hostinfo.trycritsec_avail )
    {
        ASSERT ( pFT_MUTEX->nLockedCount > 0 );
        pFT_MUTEX->nLockedCount--;
        if ( pFT_MUTEX->nLockedCount <= 0 )
            pFT_MUTEX->dwLockOwner = 0;
        MyLeaveCriticalSection ( &pFT_MUTEX->MutexLock );
    }
    else
    {
        MyEnterCriticalSection ( &pFT_MUTEX->MutexLock );
        ASSERT ( pFT_MUTEX->nLockedCount > 0 );
        pFT_MUTEX->nLockedCount--;
        if ( pFT_MUTEX->nLockedCount <= 0 )
        {
            pFT_MUTEX->dwLockOwner = 0;
            MySetEvent ( pFT_MUTEX->hUnlockedEvent );
        }
        MyLeaveCriticalSection ( &pFT_MUTEX->MutexLock );
    }
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Now we get to the "meat" of fthreads...
//
// The below function atomically releases the caller's mutex and registers the fact
// that the caller wishes to wait on their condition variable (or rather the other
// way around: it first registers the fact that the caller wishes to wait on the
// condition by first acquiring the condition variable lock and then registering
// the wait and THEN afterwards (once the wait has been registered and condition
// variable lock acquired) releases the original mutex). This ensures that no one
// can "sneak a signal past us" from the time this function is called until we can
// manage to register the wait request since no signals can ever be sent while the
// condition variable is locked.

static int  BeginWait
(
    FT_COND_VAR*      pFT_COND_VAR,
    fthread_mutex_t*  pFTUSER_MUTEX
)
{
    int        rc;
    FT_MUTEX*  pFT_MUTEX;

    if (0
        || !pFT_COND_VAR                                          // (invalid ptr)
        || !pFTUSER_MUTEX                                         // (invalid ptr)
        || !(pFT_MUTEX = pFTUSER_MUTEX->hMutex)                   // (invalid ptr)
    )
        return RC(EINVAL);

    if (0
        ||  pFT_MUTEX    -> dwLockOwner  != GetCurrentThreadId()  // (mutex not owned)
        ||  pFT_MUTEX    -> nLockedCount <= 0                     // (mutex not locked)
    )
        return RC(EPERM);

    // First, acquire the fthreads condition variable lock...

    for (;;)
    {
        MyEnterCriticalSection ( &pFT_COND_VAR->CondVarLock );

        // It is always safe to proceed if the prior signal was completely
        // processed (received by everyone who was supposed to receive it)

        if ( IsEventSet ( pFT_COND_VAR->hSigRecvdEvent ) )
            break;

        // Prior signal not completely received yet... Verify that it is
        // still being transmitted...

        ASSERT ( IsEventSet ( pFT_COND_VAR->hSigXmitEvent ) );

        // If no one is currently waiting to receive [this signal not yet
        // completely received and still being transmitted], then we can
        // go ahead and receive it right now *regardless* of what type of
        // signal it is ("signal" or "broadcast") since we're *obviously*
        // the one who is supposed to receive it (since we ARE trying to
        // wait on it after all and it IS being transmitted. The 'xmit'
        // event is *always* turned off once everyone [who is *supposed*
        // to receive the signal] *has* received the signal. Thus, since
        // it's still being transmitted, that means *not* everyone who
        // *should* receive it *has* received it yet, and thus we can be
        // absolutely certain that we indeed *should* therefore receive it
        // since we *are* after all waiting for it).

        // Otherwise (prior signal not completely processed AND there are
        // still others waiting to receive it too (as well as us)), then if
        // it's a "broadcast" type signal, we can go ahead and receive that
        // type of signal too as well (along with the others); we just came
        // to the party a little bit late (but nevertheless in the nick of
        // time!), that's all...

        if ( !pFT_COND_VAR->nNumWaiting || pFT_COND_VAR->bBroadcastSig )
            break;

        // Otherwise it's a "signal" type signal (and not a broadcast type)
        // that hasn't been completely received yet, meaning only ONE thread
        // should be released. Thus, since there's already a thread (or more
        // than one thread) already waiting/trying to receive it, we need to
        // let [one of] THEM receive it and NOT US. Thus we go back to sleep
        // and wait for the signal processing currently in progress to finish
        // releasing the proper number of threads first. Only once that has
        // happened can we then be allowed to try and catch whatever signal
        // happens to come along next...

        MyLeaveCriticalSection ( &pFT_COND_VAR->CondVarLock );

        // (Programming Note: technically we should really be checking our
        // return code from the below wait call too)

        MyWaitForSingleObject ( pFT_COND_VAR->hSigRecvdEvent, INFINITE );
    }

    // Register the caller's wait request while we still have control
    // over this condition variable...

    pFT_COND_VAR->nNumWaiting++;        // (register wait request)

    // Now release the original mutex and thus any potential signalers...
    // (but note that no signal can actually ever be sent per se until
    // the condition variable which we currently have locked is first
    // released, which gets done in the WaitForTransmission function).

    if
    (
        (
            rc = fthread_mutex_unlock
            (
                pFTUSER_MUTEX
            )
        )
        != 0
    )
    {
        // Oops! Something went wrong. We couldn't release the original
        // caller's original mutex. Since we've already registered their
        // wait and already have the condition variable locked, we need
        // to first de-register their wait and release the condition var-
        // iable lock before returning our error (i.e. we essentially
        // need to back out what we previously did just above).

        logmsg("fthreads: BeginWait: fthread_mutex_unlock failed! rc=%d\n"
            ,rc );

        pFT_COND_VAR->nNumWaiting--;    // (de-register wait request)

        MyLeaveCriticalSection ( &pFT_COND_VAR->CondVarLock );

        return RC(rc);
    }

    // Our "begin-to-wait-on-condition-variable" task has been successfully
    // completed. We have essentially atomically released the originally mutex
    // and "begun our wait" on it (by registering the fact that there's someone
    // wanting to wait on it). Return to OUR caller with the condition variable
    // still locked (so no signals can be sent nor any threads released until
    // our caller calls the below WaitForTransmission function)...

    return RC(0);   // (success)
}

////////////////////////////////////////////////////////////////////////////////////
// Wait for the condition variable in question to receive a transmission...

static int  WaitForTransmission
(
    FT_COND_VAR*            pFT_COND_VAR,
    const struct timespec*  pTimeTimeout     // (NULL == INFINITE wait)
)
{
    DWORD  dwWaitRetCode, dwWaitMilliSecs;

    // If the signal has already arrived (i.e. is still being transmitted)
    // then there's no need to wait for it. Simply return success with the
    // condition variable still locked...

    if ( IsEventSet ( pFT_COND_VAR->hSigXmitEvent ) )
    {
        // There's no need to wait for the signal (transmission)
        // to arrive because it's already been sent! Just return.

        return 0;           // (transmission received!)
    }

    // Our loop to wait for our transmission to arrive...

    do
    {
        // Release condition var lock (so signal (transmission) can
        // be sent) and then wait for the signal (transmission)...

        MyLeaveCriticalSection ( &pFT_COND_VAR->CondVarLock );

        // Need to calculate a timeout value if this is a
        // timed condition wait as opposed to a normal wait...

        // Note that we unfortunately need to do this on each iteration
        // because Window's wait API requires a relative timeout value
        // rather than an absolute TOD timeout value like pthreads...

        if ( !pTimeTimeout )
        {
            dwWaitMilliSecs = INFINITE;
        }
        else
        {
            struct timeval  TimeNow;

            gettimeofday ( &TimeNow, NULL );

            if (TimeNow.tv_sec >  pTimeTimeout->tv_sec
                ||
                (
                    TimeNow.tv_sec == pTimeTimeout->tv_sec
                    &&
                    (TimeNow.tv_usec * 1000) > pTimeTimeout->tv_nsec
                )
            )
            {
                dwWaitMilliSecs = 0;
            }
            else
            {
                dwWaitMilliSecs =
                    ((pTimeTimeout->tv_sec - TimeNow.tv_sec) * 1000) +
                    ((pTimeTimeout->tv_nsec - (TimeNow.tv_usec * 1000)) / 1000000);
            }
        }

        // Finally we get to do the actual wait...

        dwWaitRetCode =
            MyWaitForSingleObject ( pFT_COND_VAR->hSigXmitEvent, dwWaitMilliSecs );

        // A signal (transmission) has been sent; reacquire our condition var lock
        // and receive the transmission (if it's still being transmitted that is)...

        MyEnterCriticalSection ( &pFT_COND_VAR->CondVarLock );

        // The "WAIT_OBJECT_0 == dwWaitRetCode && ..." clause in the below 'while'
        // statement ensures that we will always break out of our wait loop whenever
        // either a timeout occurs or our actual MyWaitForSingleObject call fails
        // for any reason. As long as dwWaitRetCode is WAIT_OBJECT_0 though *AND*
        // our event has still not been signaled yet, then we'll continue looping
        // to wait for a signal (transmission) that we're supposed to receive...

        // Also note that one might at first think/ask: "Gee, Fish, why do we need
        // to check to see if the condition variable's "hSigXmitEvent" event has
        // been set each time (via the 'IsEventSet' macro)? Shouldn't it *always*
        // be set if the above MyWaitForSingleObject call returns??" The answer is
        // of course no, it might NOT [still] be signaled. This is because whenever
        // someone *does* happen to signal it, we will of course be released from
        // our wait (the above MyWaitForSingleObject call) BUT... someone else who
        // was also waiting for it may have managed to grab our condition variable
        // lock before we could and they could have reset it. Thus we need to check
        // it again each time.
    }
    while ( WAIT_OBJECT_0 == dwWaitRetCode && !IsEventSet( pFT_COND_VAR->hSigXmitEvent ) );

    // Our signal (transmission) has either [finally] arrived or else we got
    // tired of waiting for it (i.e. we timed out) or else there was an error...

    if ( WAIT_OBJECT_0 == dwWaitRetCode ) return RC(0);
    if ( WAIT_TIMEOUT  == dwWaitRetCode ) return RC(ETIMEDOUT);

    // Our wait failed! Something is VERY wrong! Maybe the condition variable
    // was prematurely destroyed by someone? <shrug> In any case there's not
    // much we can do about it other than log the fact that it occurred and
    // return the error back to the caller. Their wait request has, believe
    // it or not, actually been completed (although not as they expected it
    // would in all likelihood!)...

    logmsg ( "fthreads: WaitForTransmission: MyWaitForSingleObject failed! dwWaitRetCode=%d (0x%8.8X)\n"
        ,dwWaitRetCode ,dwWaitRetCode );

    return RC(EFAULT);
}

////////////////////////////////////////////////////////////////////////////////////
// Send a "signal" or "broadcast" to a condition variable...

static int  QueueTransmission
(
    FT_COND_VAR*  pFT_COND_VAR,
    BOOL          bXmitType
)
{
    if (!pFT_COND_VAR)
        return RC(EINVAL);    // (invalid parameters were passed)

    // Wait for the condition variable to become free so we can begin transmitting
    // our signal... If the condition variable is still "busy" (in use), then that
    // means threads are still in the process of being released as a result of some
    // prior signal (transmission). Thus we must wait until that work is completed
    // first before we can send our new signal (transmission)...

    for (;;)
    {
        MyEnterCriticalSection ( &pFT_COND_VAR->CondVarLock );
        if ( IsEventSet ( pFT_COND_VAR->hSigRecvdEvent ) ) break;
        MyLeaveCriticalSection ( &pFT_COND_VAR->CondVarLock );
        MyWaitForSingleObject ( pFT_COND_VAR->hSigRecvdEvent, INFINITE );
    }

    // Turn on our transmitter...  (i.e. start transmitting our "signal" to
    // all threads who might be waiting to receive it (if there are any)...

    // If no one has registered their interest in receiving any transmissions
    // associated with this particular condition variable, then they are unfor-
    // tunately a little too late in doing so because we're ready to start our
    // transmission right now! If there's no one to receive our transmission,
    // then it simply gets lost (i.e. a "missed signal" situation has essenti-
    // ally occurred), but then that's not our concern here; our only concern
    // here is to transmit the signal (transmission) and nothing more. Tough
    // beans if there's no one listening to receive it... <shrug>

    if ( pFT_COND_VAR->nNumWaiting )                    // (anyone interested?)
    {
        pFT_COND_VAR->bBroadcastSig = bXmitType;        // (yep! set xmit type)
        MySetEvent   ( pFT_COND_VAR->hSigXmitEvent  );  // (turn on transmitter)
        MyResetEvent ( pFT_COND_VAR->hSigRecvdEvent );  // (serialize reception)
    }

    MyLeaveCriticalSection ( &pFT_COND_VAR->CondVarLock );

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// A thread has been released as a result of someone's signal or broadcast...

static void  ReceiveXmission
(
    FT_COND_VAR*  pFT_COND_VAR
)
{
    // If we were the only ones supposed to receive the transmission, (or
    // if no one remains to receive any transmissions), then turn off the
    // transmitter (i.e. stop sending the signal) and indicate that it has
    // been completely received all interested parties (i.e. by everyone
    // who was supposed to receive it)...

    pFT_COND_VAR->nNumWaiting--;    // (de-register wait since transmission
                                    //  has been successfully received now)

    // Determine whether any more waiters (threads) should also receive
    // this transmission (i.e. also be released) or whether we should
    // reset (turn off) our transmitter so as to not release any other
    // thread(s) besides ourselves...

    if (0
        || !pFT_COND_VAR->bBroadcastSig         // ("signal" == only us)
        ||  pFT_COND_VAR->nNumWaiting <= 0      // (no one else == only us)
    )
    {
        MyResetEvent ( pFT_COND_VAR->hSigXmitEvent  );   // (turn off transmitter)
        MySetEvent   ( pFT_COND_VAR->hSigRecvdEvent );   // (transmission complete)
    }
}

////////////////////////////////////////////////////////////////////////////////////
// The following function is called just before returning back to the caller. It
// first releases the condition variable lock (since we're now done with it) and
// then reacquires the caller's original mutex before returning [back to the caller].

// We MUST do things in that order! 1) release our condition variable lock, and THEN
// 2) try to reacquire the caller's original mutex. Otherwise a deadlock could occur!

// If we still had the condition variable locked before trying to acquire the caller's
// original mutex, we could easily become blocked if some other thread still already
// owned (had locked) the caller's mutex. We would then be unable to ever release our
// condition variable lock until whoever had the mutex locked first released it, but
// they would never be able to release it because we still had the condition variable
// still locked! Recall that upon entry to a wait call (see previous BeginWait function)
// we acquire the condition variable lock *first* (in order to register the caller's
// wait) before releasing their mutex. Thus, we MUST therefore release our condition
// variable lock FIRST and THEN try reacquiring their original mutex before returning.

static int  ReturnFromWait
(
    FT_COND_VAR*      pFT_COND_VAR,
    fthread_mutex_t*  pFTUSER_MUTEX,
    int               nRetCode
)
{
    int        rc;
    FT_MUTEX*  pFT_MUTEX;

    if (0
        || !pFT_COND_VAR                                          // (invalid ptr)
        || !pFTUSER_MUTEX                                         // (invalid ptr)
        || !(pFT_MUTEX = pFTUSER_MUTEX->hMutex)                   // (invalid ptr)
    )
        return RC(EINVAL);

    // (let other threads access this condition variable now...)

    MyLeaveCriticalSection ( &pFT_COND_VAR->CondVarLock );

    // (reacquire original mutex before returning back to the original caller...)

    if
    (
        (
            rc = fthread_mutex_lock
            (
                pFTUSER_MUTEX
            )
        )
        != 0
    )
    {
        // Oops! We were unable to reacquire the caller's original mutex! This
        // is actually a catastrophic type of error! The caller expects their
        // mutex to still be owned (locked) by themselves upon return, but we
        // were unable to reacquire it for them! Unfortunately there's nothing
        // we can do about this; the system is essentially hosed at this point.
        // Just log the fact that something went wrong and return. <shrug>

        logmsg("fthreads: ReturnFromWait: fthread_mutex_lock failed! rc=%d\n"
            ,rc );

        return RC(rc);        // (what went wrong)
    }

    // Return to caller with the requested return code. (The caller passes to us
    // the return code they wish to pass back to the original caller, so we just
    // return that same return code back to OUR caller (so they can then pass it
    // back to the original fthreads caller)).

    return RC(nRetCode);      // (as requested)
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Threading functions...

LIST_ENTRY        ThreadListHead;   // head list entry of list of joinable threads
CRITICAL_SECTION  ThreadListLock;   // lock for accessing list of joinable threads

#define LockThreadsList()    EnterCriticalSection ( &ThreadListLock )
#define UnlockThreadsList()  LeaveCriticalSection ( &ThreadListLock )

////////////////////////////////////////////////////////////////////////////////////
// internal joinable thread information

typedef struct _tagFTHREAD
{
    LIST_ENTRY  ThreadListLink;     // (links entries together in a chain)
    DWORD       dwThreadID;         // (thread-id)
    HANDLE      hThreadHandle;      // (Win32 thread handle)
    BOOL        bJoinable;          // (whether thread is joinable or detached)
    int         nJoinedCount;       // (#of threads that did join on this one)
    int         nPriority;          // (HTHREAD priority; -1 if not set yet)
    void*       ExitVal;            // (saved thread exit value)
    jmp_buf     JumpBuf;            // (jump buffer for fthread_exit)
}
FTHREAD, *PFTHREAD;

////////////////////////////////////////////////////////////////////////////////////
// (Note: returns with thread list lock still held if found; not held if not found)

static FTHREAD*  FindFTHREAD ( DWORD dwThreadID )
{
    FTHREAD*     pFTHREAD;
    LIST_ENTRY*  pListEntry;

    LockThreadsList();      // (acquire thread list lock)

    pListEntry = ThreadListHead.Flink;

    while ( pListEntry != &ThreadListHead )
    {
        pFTHREAD = CONTAINING_RECORD ( pListEntry, FTHREAD, ThreadListLink );

        pListEntry = pListEntry->Flink;

        if ( pFTHREAD->dwThreadID != dwThreadID )
            continue;

        return pFTHREAD;    // (return with thread list lock still held)
    }

    UnlockThreadsList();    // (release thread list lock)

    return NULL;            // (not found)
}

////////////////////////////////////////////////////////////////////////////////////

typedef struct _ftCallThreadParms
{
    PFT_THREAD_FUNC  pfnTheirThreadFunc;
    void*            pvTheirThreadArgs;
    FTHREAD*         pFTHREAD;
}
FT_CALL_THREAD_PARMS;

//----------------------------------------------------------------------------------

static unsigned __stdcall  FTWin32ThreadFunc
(
    void*  pMyArgs
)
{
    FT_CALL_THREAD_PARMS*  pCallTheirThreadParms;
    PFT_THREAD_FUNC        pfnTheirThreadFunc;
    void*                  pvTheirThreadArgs;
    FTHREAD*               pFTHREAD;

    pCallTheirThreadParms = (FT_CALL_THREAD_PARMS*) pMyArgs;

    pfnTheirThreadFunc = pCallTheirThreadParms->pfnTheirThreadFunc;
    pvTheirThreadArgs  = pCallTheirThreadParms->pvTheirThreadArgs;
    pFTHREAD           = pCallTheirThreadParms->pFTHREAD;

    free ( pCallTheirThreadParms );

    if ( setjmp ( pFTHREAD->JumpBuf ) == 0 )
        pFTHREAD->ExitVal = pfnTheirThreadFunc ( pvTheirThreadArgs );

    LockThreadsList();

    if ( !pFTHREAD->bJoinable )
    {
        // If we are not a joinable thread, we must free our
        // own resources ourselves, but ONLY IF the 'joined'
        // count is zero. If the 'joined' count is NOT zero,
        // then, however it occurred, there is still someone
        // waiting in the join function for us to exit, and
        // thus, we cannot free our resources at this time
        // (since the thread that did the join and which is
        // waiting for us to exit still needs access to our
        // resources). In such a situation the actual freeing
        // of resources is deferred and will be done by the
        // join function itself whenever it's done with them.

        if ( pFTHREAD->nJoinedCount <= 0 )
        {
            CloseHandle ( pFTHREAD->hThreadHandle );
            RemoveListEntry ( &pFTHREAD->ThreadListLink );
            free ( pFTHREAD );
        }
    }

    UnlockThreadsList();

    MyExitThread ( 0 );

    return 0;   // (make compiler happy)
}

/////////////////////////////////////////////////////////////////////////////////////
// Get the handle for a specific Thread ID

DLL_EXPORT
HANDLE fthread_get_handle
(
    fthread_t       dwThreadID              // Thread ID
)
{
    FTHREAD*      pFTHREAD = NULL;          // Local pointer storage

    if ( dwThreadID != 0 )
    {
        // If request is for current thread then it's easy

        if (GetCurrentThreadId() == dwThreadID)
            return GetCurrentThread();

        // Otherwise we need to look in our list

        pFTHREAD = FindFTHREAD( dwThreadID );
    }

    if ( pFTHREAD != NULL )
    {
        UnlockThreadsList();    // (release thread list lock
        return pFTHREAD->hThreadHandle;
    }

    return NULL;
}
////////////////////////////////////////////////////////////////////////////////////
// Create a new thread...

DLL_EXPORT
int  fthread_create
(
    fthread_t*       pdwThreadID,
    fthread_attr_t*  pThreadAttr,
    PFT_THREAD_FUNC  pfnThreadFunc,
    void*            pvThreadArgs
)
{
    FT_CALL_THREAD_PARMS*  pCallTheirThreadParms;
    size_t                 nStackSize;
    int                    nDetachState;
    FTHREAD*               pFTHREAD;
    HANDLE                 hThread;
    DWORD                  dwThreadID;

    if (0
        || !pdwThreadID
        || !pfnThreadFunc
    )
        return RC(EINVAL);

    if ( pThreadAttr )
    {
        if (pThreadAttr->nDetachState != FTHREAD_CREATE_DETACHED &&
            pThreadAttr->nDetachState != FTHREAD_CREATE_JOINABLE)
            return RC(EINVAL);

        nStackSize   = pThreadAttr->nStackSize;
        nDetachState = pThreadAttr->nDetachState;
    }
    else
    {
        nStackSize   = 0;
        nDetachState = FTHREAD_CREATE_DEFAULT;
    }

    pCallTheirThreadParms = (FT_CALL_THREAD_PARMS*)
        malloc ( sizeof ( FT_CALL_THREAD_PARMS ) );

    if ( !pCallTheirThreadParms )
    {
        logmsg("fthread_create: malloc(FT_CALL_THREAD_PARMS) failed\n");
        return RC(ENOMEM);      // (out of memory)
    }

    pFTHREAD = (FTHREAD*)
        malloc ( sizeof( FTHREAD ) );

    if ( !pFTHREAD )
    {
        logmsg("fthread_create: malloc(FTHREAD) failed\n");
        free ( pCallTheirThreadParms );
        return RC(ENOMEM);      // (out of memory)
    }

    pCallTheirThreadParms->pfnTheirThreadFunc = pfnThreadFunc;
    pCallTheirThreadParms->pvTheirThreadArgs  = pvThreadArgs;
    pCallTheirThreadParms->pFTHREAD           = pFTHREAD;

    InitializeListLink(&pFTHREAD->ThreadListLink);

    pFTHREAD->dwThreadID    = 0;
    pFTHREAD->hThreadHandle = NULL;
    pFTHREAD->bJoinable     = ((FTHREAD_CREATE_JOINABLE == nDetachState) ? (TRUE) : (FALSE));
    pFTHREAD->nJoinedCount  = 0;
    pFTHREAD->nPriority     = -1;
    pFTHREAD->ExitVal       = NULL;

    LockThreadsList();

    hThread =
        MyCreateThread ( NULL, nStackSize, FTWin32ThreadFunc, pCallTheirThreadParms, 0, &dwThreadID );

    if ( !hThread )
    {
        UnlockThreadsList();
        logmsg("fthread_create: MyCreateThread failed\n");
        free ( pCallTheirThreadParms );
        free ( pFTHREAD );
        return RC(EAGAIN);      // (unable to obtain required resources)
    }

    pFTHREAD->hThreadHandle = hThread;
    pFTHREAD->dwThreadID    = dwThreadID;
    *pdwThreadID            = dwThreadID;

    InsertListHead ( &ThreadListHead, &pFTHREAD->ThreadListLink );

    UnlockThreadsList();

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Exit from a thread...

DLL_EXPORT
void  fthread_exit
(
    void*  ExitVal
)
{
    FTHREAD* pFTHREAD;
    VERIFY ( pFTHREAD = FindFTHREAD ( GetCurrentThreadId() ) );
    pFTHREAD->ExitVal = ExitVal;
    UnlockThreadsList();
    longjmp ( pFTHREAD->JumpBuf, 1 );
}

////////////////////////////////////////////////////////////////////////////////////
// Join a thread (i.e. wait for a thread's termination)...

DLL_EXPORT
int  fthread_join
(
    fthread_t       dwThreadID,
    void**          pExitVal
)
{
    HANDLE    hThread;
    FTHREAD*  pFTHREAD;

    if ( GetCurrentThreadId() == dwThreadID )
        return RC(EDEADLK);             // (can't join self!)

    if ( !(pFTHREAD = FindFTHREAD ( dwThreadID ) ) )
        return RC(ESRCH);               // (thread not found)

    // (Note: threads list lock still held at this point
    //  since thread was found...)

    if ( !pFTHREAD->bJoinable )
    {
        UnlockThreadsList();
        return RC(EINVAL);              // (not a joinable thread)
    }

    ASSERT ( pFTHREAD->nJoinedCount >= 0 );
    pFTHREAD->nJoinedCount++;
    hThread = pFTHREAD->hThreadHandle;

    // Wait for thread to exit...

    UnlockThreadsList();
    {
        WaitForSingleObject ( hThread, INFINITE );
    }
    LockThreadsList();

    if ( pExitVal )
        *pExitVal = pFTHREAD->ExitVal;  // (pass back thread's exit value)

    ASSERT ( pFTHREAD->nJoinedCount > 0 );
    pFTHREAD->nJoinedCount--;

    // If this is the last thread to be resumed after having been suspended
    // (as a result of doing the join), then we need to do the detach (i.e.
    // to free resources), BUT ONLY IF the detach for the thread in question
    // has already been done by someone (which can be determined by virtue of
    // the "joinable" flag having already been changed back to non-joinable).

    // The idea here is that the 'detach' function purposely does not free
    // the resources unless the 'joined' count is zero. If the joined count
    // is NOT zero whenever the detach function is called, then the resources
    // cannot be freed since there's still a thread waiting to be resumed
    // from its join (perhaps us!), and it obviously still needs access to
    // the resources in question. Thus, in such a situation (i.e. there still
    // being a thread remaining to be woken up from its join when the detach
    // is done), the freeing of resources normally done by the detach function
    // is deferred so that WE can do the resource freeing ourselves once we
    // are done with them.

    if ( !pFTHREAD->bJoinable && pFTHREAD->nJoinedCount <= 0 )
    {
        CloseHandle ( pFTHREAD->hThreadHandle );
        RemoveListEntry ( &pFTHREAD->ThreadListLink );
        free ( pFTHREAD );
    }

    UnlockThreadsList();

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Detach a thread (i.e. ignore a thread's termination)...

DLL_EXPORT
int  fthread_detach
(
    fthread_t  dwThreadID
)
{
    FTHREAD*  pFTHREAD;

    if ( !( pFTHREAD = FindFTHREAD ( dwThreadID ) ) )
        return RC(ESRCH);           // (thread not found)

    // (Note: threads list lock still held at this point
    //  since thread was found...)

    if ( !pFTHREAD->bJoinable )
    {
        UnlockThreadsList();
        return RC(EINVAL);          // (not a joinable thread)
    }

    // If the thread has not yet exited, then IT will free its
    // own resources itself whenever it eventually does exit by
    // virtue of our changing it to a non-joinable thread type.

    pFTHREAD->bJoinable = FALSE;    // (indicate detach was done)

    // Otherwise we need to free its resources ourselves since
    // it obviously can't (since it has already exited).

    // Note that we cannot free the resources ourselves even if the
    // thread has already exited if there are still other threads
    // waiting to be woken up from their own join (since they will
    // still need to have access to the resources). In other words,
    // even if the thread has already exited (and thus it would seem
    // that our freeing the resources would be the proper thing to
    // do), we CANNOT do so if the 'join' count is non-zero.

    // In such a situation (the join count being non-zero indicating
    // there is still another thread waiting to be resumed from its
    // own join), we simply defer the actual freeing of resources to
    // the thread still waiting to be woken up from its join. Whenever
    // it does eventually wake up from its join, it will free the
    // resources for us, as long as we remember to reset the 'joinable'
    // flag back to non-joinable (which we've already done just above).

    if ( IsEventSet ( pFTHREAD->hThreadHandle ) && pFTHREAD->nJoinedCount <= 0 )
    {
        CloseHandle ( pFTHREAD->hThreadHandle );
        RemoveListEntry ( &pFTHREAD->ThreadListLink );
        free ( pFTHREAD );
    }

    UnlockThreadsList();

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Initialize a "thread attribute"...

DLL_EXPORT
int  fthread_attr_init
(
    fthread_attr_t*  pThreadAttr
)
{
    if ( !pThreadAttr )
        return RC(EINVAL);          // (invalid ptr)

    pThreadAttr->nDetachState = FTHREAD_CREATE_DEFAULT;
    pThreadAttr->nStackSize   = 0;

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Destroy a "thread attribute"...

DLL_EXPORT
int  fthread_attr_destroy
(
    fthread_attr_t*  pThreadAttr
)
{
    if ( !pThreadAttr )
        return RC(EINVAL);      // (invalid ptr)

    pThreadAttr->nDetachState = 0;
    pThreadAttr->nStackSize   = 0;

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Set a thread's "detachstate" attribute...

DLL_EXPORT
int  fthread_attr_setdetachstate
(
    fthread_attr_t*  pThreadAttr,
    int              nDetachState
)
{
    if ( !pThreadAttr )
        return RC(EINVAL);          // (invalid ptr)

    if ( FTHREAD_CREATE_DETACHED != nDetachState &&
         FTHREAD_CREATE_JOINABLE != nDetachState )
        return RC(EINVAL);          // (invalid detach state)

    pThreadAttr->nDetachState = nDetachState;

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Retrieve a thread's "detachstate" attribute...

DLL_EXPORT
int  fthread_attr_getdetachstate
(
    const fthread_attr_t*  pThreadAttr,
    int*                   pnDetachState
)
{
    if ( !pThreadAttr || !pnDetachState )
        return RC(EINVAL);          // (invalid ptr)

    if ( FTHREAD_CREATE_DETACHED != pThreadAttr->nDetachState &&
         FTHREAD_CREATE_JOINABLE != pThreadAttr->nDetachState )
        return RC(EINVAL);          // (invalid detach state)

    *pnDetachState = pThreadAttr->nDetachState;

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Set a thread's initial stack size...

DLL_EXPORT
int  fthread_attr_setstacksize
(
    fthread_attr_t*  pThreadAttr,
    size_t           nStackSize
)
{
    if ( !pThreadAttr )
        return RC(EINVAL);          // (invalid ptr)

    pThreadAttr->nStackSize = nStackSize;

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Retrieve a thread's initial stack size...

DLL_EXPORT
int  fthread_attr_getstacksize
(
    const fthread_attr_t*  pThreadAttr,
    size_t*                pnStackSize
)
{
    if ( !pThreadAttr || !pnStackSize )
        return RC(EINVAL);          // (invalid ptr)

    *pnStackSize = pThreadAttr->nStackSize;

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Return thread-id...

DLL_EXPORT
fthread_t  fthread_self ()
{
    return GetCurrentThreadId();
}

////////////////////////////////////////////////////////////////////////////////////
// Compare thread-ids...

DLL_EXPORT
int  fthread_equal
(
    fthread_t  dwThreadID_1,
    fthread_t  dwThreadID_2
)
{
    return ( dwThreadID_1 == dwThreadID_2 );
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Initialize a lock...

DLL_EXPORT
int  fthread_mutex_init
(
          fthread_mutex_t*      pFTUSER_MUTEX,
    const fthread_mutexattr_t*  pFT_MUTEX_ATTR
)
{
    DWORD  dwMutexType = 0;

    if ( !pFTUSER_MUTEX )
        return RC(EINVAL);      // (invalid mutex ptr)

    if ( pFT_MUTEX_ATTR && !IsValidMutexType ( dwMutexType = *pFT_MUTEX_ATTR ) )
        return RC(EINVAL);      // (invalid mutex attr ptr or mutex attr type)

    if ( !(pFTUSER_MUTEX->hMutex = MallocFT_MUTEX()) )
        return RC(ENOMEM);      // (out of memory)

    if ( !InitializeFT_MUTEX
    (
        pFTUSER_MUTEX->hMutex,
        pFT_MUTEX_ATTR ? dwMutexType : FTHREAD_MUTEX_DEFAULT
    ))
    {
        free ( pFTUSER_MUTEX->hMutex );
        pFTUSER_MUTEX->hMutex       = NULL;

        return RC(EAGAIN);      // (unable to obtain required resources)
    }

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Destroy a lock...

DLL_EXPORT
int  fthread_mutex_destroy
(
    fthread_mutex_t*  pFTUSER_MUTEX
)
{
    if ( !pFTUSER_MUTEX )
        return RC(EINVAL);      // (invalid ptr)

    if ( !UninitializeFT_MUTEX
    (
        pFTUSER_MUTEX->hMutex
    ))
        return RC(EBUSY);       // (still in use)

    free ( pFTUSER_MUTEX->hMutex );

    pFTUSER_MUTEX->hMutex = NULL;

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Try to lock a "mutex"...

DLL_EXPORT
int  fthread_mutex_trylock
(
    fthread_mutex_t*  pFTUSER_MUTEX
)
{
    if ( !pFTUSER_MUTEX )
        return RC(EINVAL);      // (invalid ptr)

    // Try to acquire the requested mutex...

    if
    (
        !TryEnterFT_MUTEX
        (
            pFTUSER_MUTEX->hMutex
        )
    )
        // We could not acquire the mutex; return 'busy'...

        return RC(EBUSY);

    // We successfully acquired the mutex... If the mutex type is recursive,
    // or, if not recursive (i.e. error-check), if this was the first/initial
    // lock on the mutex, then return success...

    if (0
        || FTHREAD_MUTEX_RECURSIVE == ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->dwMutexType
        || ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->nLockedCount <= 1
    )
        return RC(0);

    ASSERT ( FTHREAD_MUTEX_ERRORCHECK == ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->dwMutexType
        && ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->nLockedCount > 1 );

    // The mutex type is error-check and we already previously had the mutex locked
    // before (i.e. this was the *second* time we acquired this same mutex). Return
    // 'busy' after first releasing the mutex once (to decrement the locked count
    // back down to what it was (i.e. 1 (one))).

    LeaveFT_MUTEX
    (
        pFTUSER_MUTEX->hMutex
    );

    return RC(EBUSY);
}

////////////////////////////////////////////////////////////////////////////////////
// Lock a "mutex"...

DLL_EXPORT
int  fthread_mutex_lock
(
    fthread_mutex_t*  pFTUSER_MUTEX
)
{
    if ( !pFTUSER_MUTEX )
        return RC(EINVAL);      // (invalid ptr)

    // Try to acquire the requested mutex...

    if
    (
        !TryEnterFT_MUTEX
        (
            pFTUSER_MUTEX->hMutex
        )
    )
    {
        // We could not acquire the mutex. This means someone already owns the mutex,
        // so just do a normal acquire on the mutex. Both recursive and error-check
        // types will block until such time as the mutex is successfully acquired...

        EnterFT_MUTEX
        (
            pFTUSER_MUTEX->hMutex
        );

        return RC(0);
    }

    // We successfully acquired the mutex... If the mutex type is recursive,
    // or, if not recursive (i.e. error-check), if this was the first/initial
    // lock on the mutex, then return success...

    if (0
        || FTHREAD_MUTEX_RECURSIVE == ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->dwMutexType
        || ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->nLockedCount <= 1
    )
        return RC(0);

    ASSERT ( FTHREAD_MUTEX_ERRORCHECK == ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->dwMutexType
        && ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->nLockedCount > 1 );

    // The mutex type is error-check and we already previously had the mutex locked
    // before (i.e. this was the *second* time we acquired this same mutex). Return
    // 'deadlock' after first releasing the mutex once (to decrement the locked count
    // back down to what it was (i.e. 1 (one))).

    LeaveFT_MUTEX
    (
        pFTUSER_MUTEX->hMutex
    );

    return RC(EDEADLK);
}

////////////////////////////////////////////////////////////////////////////////////
// Unlock a "mutex"...

DLL_EXPORT
int  fthread_mutex_unlock
(
    fthread_mutex_t*  pFTUSER_MUTEX
)
{
    if (!pFTUSER_MUTEX)
        return RC(EINVAL); // (invalid ptr)

    if (0
        || GetCurrentThreadId() != ((PFT_MUTEX)pFTUSER_MUTEX->hMutex)->dwLockOwner  // (not owned)
        || ((PFT_MUTEX)pFTUSER_MUTEX->hMutex)->nLockedCount <= 0                    // (not locked)
    )
        return RC(EPERM);

    ASSERT ( ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->nLockedCount <= 1
        || FTHREAD_MUTEX_RECURSIVE == ((FT_MUTEX*)pFTUSER_MUTEX->hMutex)->dwMutexType );

    LeaveFT_MUTEX
    (
        pFTUSER_MUTEX->hMutex
    );

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Initialize a "condition"...

DLL_EXPORT
int  fthread_cond_init
(
    fthread_cond_t*  pFT_COND_VAR
)
{
    if ( !pFT_COND_VAR )
        return RC(EINVAL);      // (invalid ptr)

    if ( !(pFT_COND_VAR->hCondVar = MallocFT_COND_VAR()) )
        return RC(ENOMEM);      // (out of memory)

    if ( !InitializeFT_COND_VAR
    (
        pFT_COND_VAR->hCondVar
    ))
    {
        free ( pFT_COND_VAR->hCondVar );
        pFT_COND_VAR->hCondVar = NULL;
        return RC(EAGAIN);      // (unable to obtain required resources)
    }

    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Destroy a "condition"...

DLL_EXPORT
int  fthread_cond_destroy
(
    fthread_cond_t*  pFT_COND_VAR
)
{
    if ( !pFT_COND_VAR )
        return RC(EINVAL);      // (invalid ptr)

    if ( !UninitializeFT_COND_VAR
    (
        pFT_COND_VAR->hCondVar
    ))
        return RC(EBUSY);       // (still in use)

    free ( pFT_COND_VAR->hCondVar );
    pFT_COND_VAR->hCondVar = NULL;
    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// 'Signal' a "condition"...     (causes ONE waiting thread to be released)

DLL_EXPORT
int  fthread_cond_signal
(
    fthread_cond_t*  pFT_COND_VAR
)
{
    if ( !pFT_COND_VAR )
        return RC(EINVAL);      // (invalid ptr)

    return QueueTransmission
    (
        pFT_COND_VAR->hCondVar,
        FALSE               // (FALSE == not "broadcast")
    );
}

////////////////////////////////////////////////////////////////////////////////////
// 'Broadcast' a "condition"...  (causes ALL waiting threads to be released)

DLL_EXPORT
int  fthread_cond_broadcast
(
    fthread_cond_t*  pFT_COND_VAR
)
{
    if ( !pFT_COND_VAR )
        return RC(EINVAL);      // (invalid ptr)

    return QueueTransmission
    (
        pFT_COND_VAR->hCondVar,
        TRUE                // (TRUE == "broadcast" type)
    );
}

////////////////////////////////////////////////////////////////////////////////////
// Wait for a "condition" to occur...

DLL_EXPORT
int  fthread_cond_wait
(
    fthread_cond_t*   pFT_COND_VAR,
    fthread_mutex_t*  pFTUSER_MUTEX
)
{
    int rc;

    if (0
        || !pFT_COND_VAR        // (invalid ptr)
        || !pFTUSER_MUTEX       // (invalid ptr)
    )
        return RC(EINVAL);

    // The following call essentially atomically releases the caller's mutex
    // and does a wait. Of course, it doesn't really do the wait though; that's
    // actually done further below. BUT, it does atomically register the fact
    // that this thread *wishes* to do a wait by acquiring the condition variable
    // lock and then incrementing the #of waiters counter before it releases the
    // original mutex. Thus, whenever the below function call returns back to us,
    // we can be assured that: 1) our request to wait on this condition variable
    // has been registered AND 2) we have control of the condition variable in
    // question (i.e. we still hold the condition variable lock thus preventing
    // anyone from trying to send a signal just yet)...

    if
    (
        (
            rc = BeginWait
            (
                pFT_COND_VAR -> hCondVar,
                pFTUSER_MUTEX
            )
        )
        != 0
    )
    {
        // OOPS! Something went wrong. The original mutex has NOT been released
        // and we did NOT acquire our condition variable lock (and thus our wait
        // was not registered). Thus we can safely return back to the caller with
        // the original mutex still owned (held) by the caller.

        return RC(rc); // (return error code to caller; their wait failed)
    }

    // We only reach here if the condition var was successfully acquired AND our
    // wait was registered AND the original mutex was released so the signal can
    // be sent (but the signal (transmission) of course cannot ever be sent until
    // we first release our lock on our condition variable, which is of course is
    // what the below WaitForTransmission function call does within its wait loop)...

    rc = WaitForTransmission    // (wait for "signal" or "broadcast"...)
    (
        pFT_COND_VAR->hCondVar,
        NULL
    );

    // A signal (transmission) was sent and we're one of the ones (or the
    // only one) that's supposed to receive it...

    // If we're the only one that's supposed to receive this transmission,
    // then we need to turn off the transmitter (stop "sending" the signal)
    // so that no other threads get "woken up" (released) as a result of
    // this particular "signal" (transmission)...

    // (Note that the below call also de-registers our wait too)

    ReceiveXmission         // (reset transmitter)
    (
        pFT_COND_VAR->hCondVar
    );

    // Release the condition var lock (since we're done with it) and then
    // reacquire the caller's original mutex (if possible) and then return
    // back to the original caller with their original mutex held with what-
    // ever return code got set by the above wait call...

    return ReturnFromWait
    (
        pFT_COND_VAR -> hCondVar,
        pFTUSER_MUTEX,
        rc
    );
}

////////////////////////////////////////////////////////////////////////////////////
// Wait (but not forever) for a "condition" to occur...

// Refer to the comments in the above 'fthread_cond_wait' function (as well as all
// of our other internal functions too of course (BeginWait, WaitForTransmission,
// ReceiveXmission and ReturnFromWait)) for details regarding what's going on here
// (i.e. what we're doing below and why). The below function is essentially identical
// to the above 'fthread_cond_wait' function except that we don't wait forever; we
// only wait for a limited amount of time. Other than that they're exactly identical
// so there's no sense in repeating myself here...

DLL_EXPORT
int  fthread_cond_timedwait
(
    fthread_cond_t*         pFT_COND_VAR,
    fthread_mutex_t*        pFTUSER_MUTEX,
    const struct timespec*  pTimeTimeout
)
{
    int rc;

    if (0
        || !pFT_COND_VAR
        || !pFTUSER_MUTEX
        || !pTimeTimeout
    )
        return RC(EINVAL);

    if
    (
        (
            rc = BeginWait
            (
                pFT_COND_VAR -> hCondVar,
                pFTUSER_MUTEX
            )
        )
        != 0
    )
        return rc;

    rc = WaitForTransmission
    (
        pFT_COND_VAR->hCondVar,
        pTimeTimeout
    );

    ReceiveXmission
    (
        pFT_COND_VAR->hCondVar
    );

    return ReturnFromWait
    (
        pFT_COND_VAR -> hCondVar,
        pFTUSER_MUTEX,
        rc
    );
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Initialize a "mutex" attribute...

DLL_EXPORT
int  fthread_mutexattr_init ( fthread_mutexattr_t*  pFT_MUTEX_ATTR )
{
    if ( !pFT_MUTEX_ATTR )
        return RC(EINVAL);
    *pFT_MUTEX_ATTR = FTHREAD_MUTEX_DEFAULT;
    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Destroy a "mutex" attribute...

DLL_EXPORT
int  fthread_mutexattr_destroy ( fthread_mutexattr_t*  pFT_MUTEX_ATTR )
{
    if ( !pFT_MUTEX_ATTR )
        return RC(EINVAL);
    *pFT_MUTEX_ATTR = 0xCDCDCDCD;
    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Retrieve "mutex" attribute type...

DLL_EXPORT
int fthread_mutexattr_gettype
(
    const fthread_mutexattr_t*  pFT_MUTEX_ATTR,
    int*                        pnMutexType
)
{
    DWORD  dwMutexType;
    if ( !pFT_MUTEX_ATTR || !pnMutexType || !IsValidMutexType ( dwMutexType = *pFT_MUTEX_ATTR ) )
        return RC(EINVAL);
    *pnMutexType = (int) dwMutexType;
    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
// Set "mutex" attribute type...

DLL_EXPORT
int fthread_mutexattr_settype
(
    fthread_mutexattr_t*  pFT_MUTEX_ATTR,
    int                   nMutexType
)
{
    if ( !pFT_MUTEX_ATTR || !IsValidMutexType ( (DWORD) nMutexType ) )
        return RC(EINVAL);
    *pFT_MUTEX_ATTR = (DWORD) nMutexType;
    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Thread Execution Scheduling...

// (HELPER): Convert Windows priority to FthreadS priority, and vice-versa.

static int W2FPriority( int nWindowsPriority )
{
    switch (nWindowsPriority)
    {
        case  THREAD_PRIORITY_IDLE:           return  FTHREAD_IDLE;
        case  THREAD_PRIORITY_LOWEST:         return  FTHREAD_LOWEST;
        case  THREAD_PRIORITY_BELOW_NORMAL:   return  FTHREAD_BELOW_NORMAL;
        case  THREAD_PRIORITY_NORMAL:         return  FTHREAD_NORMAL;
        case  THREAD_PRIORITY_ABOVE_NORMAL:   return  FTHREAD_ABOVE_NORMAL;
        case  THREAD_PRIORITY_HIGHEST:        return  FTHREAD_HIGHEST;
        case  THREAD_PRIORITY_TIME_CRITICAL:  return  FTHREAD_TIME_CRITICAL;
        default:
        {
            CRASH();    // (WINDOWS OPERATING SYSTEM BUG!)
        }
    }
    UNREACHABLE_CODE( return DEFAULT_HERC_PRIO );
}

static int F2WPriority( int nFthreadPriority )
{
    if (nFthreadPriority <= FTHREAD_IDLE         ) return THREAD_PRIORITY_IDLE;
    if (nFthreadPriority <= FTHREAD_LOWEST       ) return THREAD_PRIORITY_LOWEST;
    if (nFthreadPriority <= FTHREAD_BELOW_NORMAL ) return THREAD_PRIORITY_BELOW_NORMAL;
    if (nFthreadPriority <= FTHREAD_NORMAL       ) return THREAD_PRIORITY_NORMAL;
    if (nFthreadPriority <= FTHREAD_ABOVE_NORMAL ) return THREAD_PRIORITY_ABOVE_NORMAL;
    if (nFthreadPriority <= FTHREAD_HIGHEST      ) return THREAD_PRIORITY_HIGHEST;
                                                   return THREAD_PRIORITY_TIME_CRITICAL;
}

////////////////////////////////////////////////////////////////////////////////////

DLL_EXPORT int fthread_getschedparam( fthread_t dwThreadID, int* pnPolicy, struct sched_param* pSCHPARM )
{
    FTHREAD*  pFTHREAD;
    HANDLE    hThread;
    int       nWindowsPriority;

    if (0
        || !pnPolicy
        || !pSCHPARM
        || !(hThread = fthread_get_handle( dwThreadID ))
        || THREAD_PRIORITY_ERROR_RETURN == (nWindowsPriority = GetThreadPriority( hThread ))
    )
        return RC(EINVAL);

    *pnPolicy = FTHREAD_POLICY;

    if (0
        || !(pFTHREAD = FindFTHREAD( dwThreadID ))
        ||   pFTHREAD->nPriority < 0
    )
        pSCHPARM->sched_priority = W2FPriority( nWindowsPriority );
    else
        pSCHPARM->sched_priority = pFTHREAD->nPriority;

    if (pFTHREAD)
        UnlockThreadsList();

    return RC(0);
}

DLL_EXPORT int fthread_setschedparam( fthread_t dwThreadID, int nPolicy, const struct sched_param* pSCHPARM )
{
    FTHREAD*  pFTHREAD;
    HANDLE    hThread;

    if (nPolicy != FTHREAD_POLICY)
        return RC(ENOTSUP);

    if (0
        || !pSCHPARM
        ||  pSCHPARM->sched_priority < FTHREAD_MIN_PRIO
        ||  pSCHPARM->sched_priority > FTHREAD_MAX_PRIO
        || !(hThread = fthread_get_handle( dwThreadID ))
        || !SetThreadPriority( hThread, F2WPriority( pSCHPARM->sched_priority ))
    )
        return RC(EINVAL);

    VERIFY((pFTHREAD = FindFTHREAD( dwThreadID )));
    pFTHREAD->nPriority = pSCHPARM->sched_priority;
    UnlockThreadsList();
    return RC(0);
}

////////////////////////////////////////////////////////////////////////////////////

DLL_EXPORT int fthread_get_priority_min( int nPolicy )
{
    if (nPolicy != FTHREAD_POLICY)
        return RC(ENOTSUP);
    return FTHREAD_MIN_PRIO;
}

DLL_EXPORT int fthread_get_priority_max( int nPolicy )
{
    if (nPolicy != FTHREAD_POLICY)
        return RC(ENOTSUP);
    return FTHREAD_MAX_PRIO;
}

////////////////////////////////////////////////////////////////////////////////////

DLL_EXPORT void fthreads_internal_init()
{
    static BOOL bDidThis  = FALSE;

    if (!bDidThis)
    {
        FTHREAD* pFTHREAD;
        bDidThis = TRUE;

        // Global initialization

        InitializeListHead ( &ThreadListHead );
        InitializeCriticalSection ( &ThreadListLock );

        // Add the current thread (impl = main startup thread) to our list
        // so fthread_getschedparam/fthread_setschedparam lookup succeeds.

        pFTHREAD = (FTHREAD*) malloc( sizeof( FTHREAD ));
        memset( pFTHREAD, 0, sizeof( FTHREAD ));

        InitializeListLink( &pFTHREAD->ThreadListLink );

        pFTHREAD->dwThreadID    = GetCurrentThreadId();
        pFTHREAD->hThreadHandle = GetCurrentThread();
        pFTHREAD->bJoinable     = FALSE; // (main thread cannot be joined)
        pFTHREAD->nJoinedCount  = 0;
        pFTHREAD->nPriority     = -1;
        pFTHREAD->ExitVal       = NULL;

        // (no need to lock list during initialization)
        InsertListHead( &ThreadListHead, &pFTHREAD->ThreadListLink );
    }
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

#endif // !defined(OPTION_FTHREADS)
