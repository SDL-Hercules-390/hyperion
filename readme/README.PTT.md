![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules PTT Tracing

## Contents

1. [History](#History)
2. [Using PTT](#Using-PTT)
3. [Future Direction](#Future-Direction)

## History

**`PTT`** is a misnomer.

The Hercules "Threading and locking trace debugger" is not just for thread and lock tracing.  It is actually a generic Hercules internal event tracing debugging facility.

Early in Hercules development when threading and locks were introduced, we had a lot of trouble with incorrect lock management, resulting in deadlocks and other thread related problems.  We're better at it now, but we still have problems with it from time to time.  (Programmers are not perfect creatures and sometimes make mistakes, Hercules developers included.)

Greg Smith was the one that took up the challenge and created our basic PTT tracing facility to try and help identify such threading/locking problems. (The `PTT` orignally stood for "POSIX Thread Tracing".)

It was primarily written to intercept lock and condition variable calls and add a table entry to an internal wraparound event table which could then be dumped (displayed) on demand whenever it appeared a deadlock had occurred.

He designed it however as pretty much a generic "event tracing" tool, such that _ANY_ "event" could be traced, and not just locking or threading events.

The `ptt` command is used to create (allocate) the table and to define what type (class) of events should be traced. Enter `help ptt` for more information:

```
    Command               Description
    ----------------      -------------------------------------------------------
    ptt                  *Activate or display internal trace table

    Format: "ptt [?] [events] [options] [nnnnnn]"

    When specified with no operands, the ptt command displays the defined trace
    parameters and the contents of the internal trace table.

    When specified with operands, the ptt command defines the trace parameters
    identifying which events are to be traced. When the last option is numeric,
    it defines the size of the trace table itself and activates tracing.

    Events:    (should be specified first, before any options are specified)

         (no)log          trace internal logger events
         (no)tmr          trace internal timer events
         (no)thr          trace internal threading events
         (no)inf          trace instruction information events
         (no)err          trace instruction error events
         (no)pgm          trace program interrupt events
         (no)csf          trace interlocked instruction type events
         (no)sie          trace SIE instruction events
         (no)sig          trace SIGP instruction events
         (no)io           trace I/O instruction events
         (no)txf          trace Transactional-Execution Facility events
         (no)lcs1         trace LCS timing events
         (no)lcs2         trace LCS general debugging events
         (no)qeth         trace QETH general debugging events
         (no)xxx          trace undefined/generic/custom events

    options:   (should be specified last, after any events are specified)

         ?                show currently defined trace parameters
         (no)lock         lock table before updating
         (no)tod          timestamp table entries
         (no)wrap         wraparound trace table
         (no)dtax         dump table at exit
         to=nnn           automatic display timeout  (number of seconds)
         nnnnnn           table size                 (number of entries)
```

The `PTT` macro statement is then used to add events to the table.

The `ptt` command without any operands is used to dump (display) the table.

As originally written by Greg, the locking call intercept functions were part of the general PTT facility, and macro redefinitions were created to automatically route our locking calls (`obtain_lock`, `release_lock`, etc) to the appropriate "ptt_pthread_xxxx" function that would then add the entry to the trace table.

In June of 2013 however, Fish needed to make some enhancements to the lock functions to automatically detect and report errors (non-zero return codes from any of the locking calls) under the guidance of Mark L. Gaubatz.

It was at this time it was recognized that the generalized "PTT intercept" functions that were originally a part of the pttrace.c module as originally written/designed by Greg Smith should actually be moved into a separate "threading/locking" module ([hthreads.c](../hthreads.c)), thereby leaving the [pttrace.c](../pttrace.c)/[.h](../pttrace.h) modules responsible for only what they were meant for: generalized event table tracing.

This allowed us to clean up much of the macro redefinition monkey business originally in the [hthreads.h](../hthreads.h) header that was originally coded solely for the purpose of interfacing with the "PTT" internal trace table facility.

Now, the `htreads.c` module is responsible for calling the proper threading implementation function (pthreads on non-Windows or fthreads on Windows) as well as checking the return code and, if necessary, calling the "PTT" facility to add an entry to the trace table.

## Using PTT

The PTT tracing facility can be used to add generic "events" to the internal trace table, and is thus ideal for helping to debug _ANY_ Hercules module/driver and not just for debugging lock/thread handling.

To use it, simply add a `PTT` macro statement to your code:
```C
    PTT( trclass, msg, data1, data2, rc );
or e.g:
    PTT_XXX( msg, data1, data2, rc );
```
    
The specified macro arguments are then added to the trace table entry when the event occurs:
```C
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
```

The message must be a string value that remains valid for the duration of Hercules execution.  Usually it is coded as a string constant ("message") and not as a pointer variable name (since the variable might go out of scope or change in value, which would lead to erroneous/misleading trace messages). It is suggested to try and keep them short (no longer than 11-12 characters or so).

The two data pointer values can be anything.  They do not need to actually point to anything.  They could be a 64-bit integer value for example.  Same idea with the return code: it can be any integer value meaningful to you and your code/driver.  It does not have to be an actual return code.

The trace class must be one of the predefined classes. To make PTT easier to use, helper macros are defined that include the predefined class already in the macro definition itself, so all you have to do is use the helper macro instead, specifying only your "message" and data values without having to be bothered with also coding the trace class too:
```C
    #define PTT_LOG(   m, d1, d2, rc )  PTT( PTT_CL_LOG,   m, d1, d2, rc )
    #define PTT_TMR(   m, d1, d2, rc )  PTT( PTT_CL_TMR,   m, d1, d2, rc )
    #define PTT_THR(   m, d1, d2, rc )  PTT( PTT_CL_THR,   m, d1, d2, rc )
    #define PTT_INF(   m, d1, d2, rc )  PTT( PTT_CL_INF,   m, d1, d2, rc )
    #define PTT_ERR(   m, d1, d2, rc )  PTT( PTT_CL_ERR,   m, d1, d2, rc )
    #define PTT_PGM(   m, d1, d2, rc )  PTT( PTT_CL_PGM,   m, d1, d2, rc )
    #define PTT_CSF(   m, d1, d2, rc )  PTT( PTT_CL_CSF,   m, d1, d2, rc )
    #define PTT_SIE(   m, d1, d2, rc )  PTT( PTT_CL_SIE,   m, d1, d2, rc )
    #define PTT_SIG(   m, d1, d2, rc )  PTT( PTT_CL_SIG,   m, d1, d2, rc )
    #define PTT_IO(    m, d1, d2, rc )  PTT( PTT_CL_IO,    m, d1, d2, rc )
    #define PTT_TXF(   m, d1, d2, rc )  PTT( PTT_CL_TXF,   m, d1, d2, rc )
    #define PTT_LCS1(  m, d1, d2, rc )  PTT( PTT_CL_LCS1,  m, d1, d2, rc )
    #define PTT_LCS2(  m, d1, d2, rc )  PTT( PTT_CL_LCS2,  m, d1, d2, rc )
    #define PTT_QETH(  m, d1, d2, rc )  PTT( PTT_CL_QETH,  m, d1, d2, rc )
    #define PTT_XXX(   m, d1, d2, rc )  PTT( PTT_CL_XXX,   m, d1, d2, rc )
```

For generic event type trace entries you should use the `PTT_XXX` helper macro. The other helper macros are predefined classes meant for specific areas of Hercules logic where you want the PTT trace statement to remain a permanent part of Hercules source, allowing it to be activated at any time via the `ptt` command.  Other classes are being developed/defined for future use to make it easier to define your own trace classes separate from the Hercules predefined internal classes. The `PTT_XXX` macro is meant for temporary one-shot debugging.

When PTT tracing is activated via the `ptt` command to allocate a non-zero number of trace table entries (enter `help ptt` for more information), the PTT facility will then insert your trace events into the table as they occur. (Note that the table is a wrap-around table unless the `nowrap` option is specified.)

When the entry is added, the thread id of the thread making the call, the current time of day and the source file name and line number where the call was made are added to the trace table entry along with whatever other debug information you may need (up to 3 different 64-bit values, being the `d1`, `d2` and `rc` arguments of the macros you see above). They can be anything you want. An address, a numeric value. Anything! It's just a 64-bit value as far as PTT is concerned.

When you later enter the `ptt` command with no arguments to dump (display) the table, the resulting display might then look something like this:
```
    HHC90021D 08:41:34.817511 00001414 Processor CP00  io.c:789           stsch b4ob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817512 00001414 Processor CP00  io.c:791           stsch afob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817512 00001414 Processor CP00  io.c:798           stsch b4rl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817512 00001414 Processor CP00  io.c:800           stsch afrl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817517 00001414 Processor CP00  io.c:789           stsch b4ob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817518 00001414 Processor CP00  io.c:791           stsch afob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817518 00001414 Processor CP00  io.c:798           stsch b4rl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817519 00001414 Processor CP00  io.c:800           stsch afrl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817520 00001414 Processor CP00  io.c:789           stsch b4ob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817520 00001414 Processor CP00  io.c:791           stsch afob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817520 00001414 Processor CP00  io.c:798           stsch b4rl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817521 00001414 Processor CP00  io.c:800           stsch afrl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817522 00001414 Processor CP00  io.c:789           stsch b4ob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817522 00001414 Processor CP00  io.c:791           stsch afob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817522 00001414 Processor CP00  io.c:798           stsch b4rl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.817523 00001414 Processor CP00  io.c:800           stsch afrl        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.818615 000025a8 dev 0580 thrd   tapedev.c:2841     locat b4ob        000000000002421c 0000000000000000 0000000000000000
    HHC90021D 08:41:34.818617 000025a8 dev 0580 thrd   tapedev.c:2843     locat afob        000000000002421c 0000000000000000 0000000000000000
    HHC90021D 08:41:34.818621 000025a8 dev 0580 thrd   hettape.c:381      fsb_het b4fsb     0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:34.819892 000025a8 dev 0580 thrd   hettape.c:385      fsb_het affsb     0000000000000000 0000000000000000 0000000000000001
    HHC90021D 08:41:35.819169 000025a8 dev 0580 thrd   hettape.c:381      fsb_het b4fsb     0000000000000000 00000000000003e8 0000000000000000
    HHC90021D 08:41:35.820135 000025a8 dev 0580 thrd   hettape.c:385      fsb_het affsb     0000000000000000 00000000000003e8 00000000000003e9
    HHC90021D 08:41:36.167940 00001414 Processor CP00  io.c:789           stsch b4ob        0000000000000000 0000000000000000 0000000000000000
    HHC90021D 08:41:36.819231 000025a8 dev 0580 thrd   hettape.c:381      fsb_het b4fsb     0000000000000000 00000000000007d0 0000000000000000
    HHC90021D 08:41:36.820209 000025a8 dev 0580 thrd   hettape.c:385      fsb_het affsb     0000000000000000 00000000000007d0 00000000000007d1
    HHC90021D 08:41:37.819128 000025a8 dev 0580 thrd   hettape.c:381      fsb_het b4fsb     0000000000000000 0000000000000bb8 0000000000000000
    HHC90021D 08:41:37.820136 000025a8 dev 0580 thrd   hettape.c:385      fsb_het affsb     0000000000000000 0000000000000bb8 0000000000000bb9
```
The **first column** is the gettimeofday of when the trace event was logged, down to the microsecond.

The **second column** is the id of the thread that logged it.

The **third column** is the thread name.

The **fourth** is the of course the filename and line number of the source file where the event was logged.

The **fifth** is a description of the event (i.e. the `m` message argument of your PTT helper macro statement, e.g:
```C
    PTT_XXX( "foobar!", val1, val2, val3 );
```

The **last 3 columns** are of course the formatted display of the `d1`, `d2` and `rc` helper macro values, being whatever debug information you wish to place into your trace record.


## Future Direction

The future direction of the PTT tracing facility is to eventually allow each module/driver define their own private trace event classes allowing the user (or especially _you_, the Hercules developer!) to tell PTT which of your private classes it should trace.  The hope is to provide faster and easier debugging without impacting the overall speed of execution that "LOGMSG" and "TRACE" macros normally incur (which simply log messages to the Hercules panel).

Each module will ideally be able to define their own set of classes and class names separate from other modules.  How exactly this is going to work has not been thought out yet.  It is still in the design / "TODO" stage.

Hopefully someone will find the time to take up the challenge and develop it for us.

"Fish" (David B. Trout)<br>
updated January 30, 2023
