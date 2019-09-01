![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules PTT Tracing

## Contents

1. [History](#History)
2. [Using PTT](#Using-PTT)
3. [Future Direction](#Future-Direction)

## History

'PTT' is a misnomer.

The Hercules "Threading and locking trace debugger" is not just for thread and lock tracing.  It is actually a generic Hercules internal event tracing debugging facility.

Early in Hercules development when threading and locks were introduced, we had a lot of trouble with incorrect lock management, resulting in deadlocks and other thread related problems.  We've gotten better at it but we still have problems with it from time to time.  (Programmers are not perfect and make mistakes, Hercules developers included.)

Greg Smith was the one that took up the challenge and created our basic PTT tracing facility to try and help identify such threading/locking problems.

It was primarily written to intercept lock and condition variable calls and add a table entry to an internal wraparound event table which could then be dumped (displayed) on demand whenever it appeared a deadlock had occurred.

He designed it however as pretty much a generic "event tracing" tool, such that ANY "event" could be traced, and not just locking events.

The 'ptt' command is used to create (allocate) the table and to define what type (class) of events should be traced.

The "PTT" macro is then used to add events to the table.

The 'ptt' command without any operands is used to dump (display) the table.

As originally written by Greg, the locking call intercept functions were part of the general PTT facility, and macro redefinitions were created to automatically route our locking calls (obtain_lock, release_lock, etc) to the appropriate "ptt_pthread_xxxx" function that would then add the entry to the trace table.

In June of 2013 however, Fish needed to make some enhancements to the lock functions to automatically detect and report errors (non-zero return codes from any of the locking calls) under the guidance of Mark L. Gaubatz.

It was at this time it was recognized that the generalized "PTT intercept" functions that were originally a part of the pttrace.c module as originally written/designed by Greg Smith should actually be moved into a separate "threading/locking" module ([hthreads.c](../hthreads.c)), thereby leaving the [pttrace.c](../pttrace.c)/[.h](../pttrace.h) modules responsible for only what they were meant for: generalized event table tracing.

This allowed us to clean up much of the macro redefinition monkey business originally in the [hthreads.h](../hthreads.h) header that was originally coded solely for the purpose of interfacing with the "PTT" internal trace table facility.

Now, the htreads.c module is responsible for calling the proper threading implementation function (pthreads on non-Windows or fthreads on Windows) as well as checking the return code and, if necessary, calling the "PTT" facility to add an entry to the trace table.

## Using PTT

The PTT tracing facility can be used to add generic "events" to the internal trace table, and is thus ideal for helping to debug *ANY* module/driver and not just for debugging lock handling.

To use it, simply add a 'PTT' macro call to your code.  The parameters to the macro are as follows:  
              `PTT( class, msg, p1, p2, rc );`  

Where:  
```
    U32             class;      /* Trace class (see header)  */
    const char*     msg;        /* Trace message             */
    const void*     p1;         /* Data 1                    */
    const void*     p2;         /* Data 2                    */
    int             rc;         /* Return code               */
```

The message must be a string value that remains valid for the duration of Hercules execution.  Usually it is coded as a string constant ("message") and not a pointer variable name (since the variable might go out of scope or change in value, which would lead to erroneous/misleading trace messages).

The two data pointer values can be anything.  They do not need to actually point to anything.  They could be a 64-bit integer value for example.  Same idea with the return code: it can be any integer value meaningful to you and your code/driver.  It does not have to be an actual return code.

The trace class must be one of the predefined classes listed below:
```
    PTT_CL_LOG      0x00000001      /* Logger records            */
    PTT_CL_TMR      0x00000002      /* Timer/Clock records       */
    PTT_CL_THR      0x00000004      /* Thread records            */
    PTT_CL_INF      0x00000008      /* Instruction info          */
    PTT_CL_ERR      0x00000010      /* Instruction error/unsup   */
    PTT_CL_PGM      0x00000020      /* Program interrupt         */
    PTT_CL_CSF      0x00000040      /* Compare&Swap failure      */
    PTT_CL_SIE      0x00000080      /* Interpretive Execution    */
    PTT_CL_SIG      0x00000100      /* SIGP signalling           */
    PTT_CL_IO       0x00000200      /* IO                        */
```

For generic event type trace entries (as opposed to, say, instruction type trace entries), you should use the 'PTT_CL_INF' class.  Other classes are being developed/defined for future use to make it easier to define your own trace classes separate from the Hercules predefined internal classes.

When a PTT tracing is activated via the `ptt` command (to define/allocate a non-zero number of entries trace table), the PTT facility will then add your trace event to the table.

When the entry is added, the thread id of the thread making the call, the current time of day and the source file name and line number where the call was made are added to the trace table entry.

When you later enter the `ptt` command with no arguments to dump/display the table, the resulting display might then look something like this:
```
    qeth.c:3180  20:37:50.942314 000015D0 af select    0000000000000000 0000000000000000 00000001
    qeth.c:3144  20:37:50.942322 000015D0 b4 procinpq  0000000000000000 0000000000000000 00000000
    qeth.c:1297  20:37:50.942325 000015D0 b4 tt read   0000000000000000 000000000000ffff 00000000
    qeth.c:1300  20:37:50.942585 000015D0 af tt read   0000000000000000 000000000000ffff 0000003c
    qeth.c:3146  20:37:50.942588 000015D0 af procinpq  0000000000000000 0000000000000000 00000000
    qeth.c:3178  20:37:50.942598 000015D0 b4 select    0000000000000000 0000000000000000 00000000
    qeth.c:3180  20:37:50.942615 000015D0 af select    0000000000000000 0000000000000000 00000001
    cpu.c:1022   20:37:50.942617 000011C0 *IOINT       000000000001003d 0000000000f4e2a0 28000000
    qeth.c:3144  20:37:50.942617 000015D0 b4 procinpq  0000000000000000 0000000000000000 00000000
    qeth.c:1297  20:37:50.942618 000015D0 b4 tt read   0000000000000000 000000000000ffff 00000000
    qeth.c:1300  20:37:50.942632 000015D0 af tt read   0000000000000000 000000000000ffff 00000000
    qeth.c:3146  20:37:50.942632 000015D0 af procinpq  0000000000000000 0000000000000000 00000000
    qeth.c:3178  20:37:50.942634 000015D0 b4 select    0000000000000000 0000000000000000 00000000
```


## Future Direction

The future direction of the PTT tracing facility is to eventually allow each module/driver define their own private trace event classes allowing the user (or especially you the Hercules developer!) to tell PTT which of your private classes it should trace.  The hope is to provide faster and easier debugging without impacting the overall speed of execution that "logmsg" and "TRACE" macros (which simply call logmsg) normally incur.

Each module will ideally be able to define their own set of classes and class names separate from other modules.  How exactly this is going to work has not been thought out yet.  It is still in the design / "TODO" stage.

Hopefully someone will find the time to take up the challenge and develop it for us.

"Fish" (David B. Trout)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;June 2, 2013
