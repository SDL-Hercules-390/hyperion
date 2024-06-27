/* DYNGUI.C     (C) Copyright "Fish" (David B. Trout), 2003-2012     */
/*              Hercules External GUI Interface DLL                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _DYNGUI_C_
#define _DYNGUI_DLL_

#include "hercules.h"   // (#includes "config." w/#define for VERSION)
#include "devtype.h"
#include "opcode.h"

///////////////////////////////////////////////////////////////////////////////
// Some handy macros...       (feel free to add these to hercules.h)

#ifndef BOOL
#define BOOL  BYTE
#endif
#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif

///////////////////////////////////////////////////////////////////////////////
// Our global variables...    (initialized by our "Initialize" function)

#define  INPUT_STREAM_FILE_PTR    ( stdin  )
#define  OUTPUT_STREAM_FILE_PTR   ( stdout )
#define  STATUS_STREAM_FILE_PTR   ( stderr )
#define  MAX_COMMAND_LEN          (  1024  )
#define  DEF_MAXRATES_RPT_INTVL   (  1440  )

#if defined( WIN32 ) && !defined( HDL_USE_LIBTOOL )
#if !defined( _MSVC_ )
  SYSBLK            *psysblk;                  // (ptr to Herc's SYSBLK structure)
  #define  sysblk  (*psysblk)
#endif
#endif

static FILE*    fOutputStream        = NULL;   // (stdout stream)
static FILE*    fStatusStream        = NULL;   // (stderr stream)
static int      nInputStreamFileNum  =  -1;    // (file descriptor for stdin stream)
static int      gui_nounload         =   1;    // (nounload indicator)

// The device query buffer SHOULD be the maximum device filename length
// plus the maximum descriptive length of any/all options for the device,
// but there is no #define for either so we have no choice but to impose
// our own maximum.

#define  MAX_DEVICEQUERY_LEN      ( 1024 + 256 )

///////////////////////////////////////////////////////////////////////////////
// Some forward references...   (our own functions that we call)

void  Initialize         ();
void  ProcessingLoop     ();
void  Cleanup            ();
void  UpdateTargetCPU    ();
void  ReadInputData      (int nTimeoutMillsecs);
void  ProcessInputData   ();
void* gui_panel_command  (char* pszCommand);
void  UpdateStatus       ();
void  HandleForcedRefresh();
void  UpdateCPUStatus    ();
void  UpdateRegisters    ();
void  UpdateDeviceStatus ();
void  NewUpdateDevStats  ();

void  gui_fprintf( FILE* stream, const char* pszFormat, ... );

///////////////////////////////////////////////////////////////////////////////
// Our main processing loop...

BOOL bDoneProcessing = FALSE;       // (set to TRUE to exit)

void ProcessingLoop()
{
    // Notify logger_thread we're in control

    sysblk.panel_init = 1;

    // Our main purpose in life: read input stream and process
    // any commands that may be entered, and send periodic status
    // information back to the external gui via its status stream.

    // Note we only exit whenever our bDoneProcessing flag is set
    // which is normally not done until just before Herc unloads
    // us which is normally not done until immediately before it
    // terminates.

    // Also note we re-retrieve sysblk.panrate each iteration
    // since it could change from one iteration to the next as a result
    // of the Hercules "panrate" command being entered and processed.

    while (!bDoneProcessing)
    {
        UpdateTargetCPU();      // ("cpu" command could have changed it)
        UpdateStatus();         // (keep sending status back to gui...)

        ReadInputData( sysblk.panrate );

        ProcessInputData();     // (if there even is any of course...)
    }
}

///////////////////////////////////////////////////////////////////////////////

int     pcpu                = INT_MAX;  // target cpu# for commands and displays
REGS*   pTargetCPU_REGS     = NULL;     // pointer to target cpu REGS

int     prev_pcpu           = INT_MAX;  // (previous value)
REGS*   pPrevTargetCPU_REGS = NULL;     // (previous value)

REGS    copyregs;                       // (copy of active cpu's REGS)
REGS    copysieregs;                    // (same but when in SIE mode)
REGS*   CopyREGS( int cpu );            // (fwd ref)

///////////////////////////////////////////////////////////////////////////////

void  UpdateTargetCPU ()
{
    if (!sysblk.shutdown)
        pTargetCPU_REGS = CopyREGS( pcpu = sysblk.pcpu );
}

///////////////////////////////////////////////////////////////////////////////
// (get non-moving/non-dynamic working copy of active cpu's register context)

REGS* CopyREGS( int cpu )               // (same logic as in panel.c)
{
    REGS* regs;

    if (cpu < 0 || cpu >= sysblk.maxcpu)
        cpu = 0;

    obtain_lock( &sysblk.cpulock[cpu] );

    if (!(regs = sysblk.regs[cpu]))
    {
        release_lock( &sysblk.cpulock[cpu] );
        return &sysblk.dummyregs;
    }

    memcpy( &copyregs, regs, sysblk.regs_copy_len );

    if (!copyregs.hostregs)
    {
        release_lock(&sysblk.cpulock[cpu]);
        return &sysblk.dummyregs;
    }

#if defined(_FEATURE_SIE)
    if (regs->sie_active)
    {
        memcpy( &copysieregs, GUESTREGS, sysblk.regs_copy_len );
        copyregs.guestregs = &copysieregs;
        copysieregs.hostregs = &copyregs;
        regs = &copysieregs;
    }
    else
#endif
        regs = &copyregs;

    MAYBE_SET_PSW_IA_FROM_IP( regs );

    release_lock( &sysblk.cpulock[cpu] );
    return regs;
}

///////////////////////////////////////////////////////////////////////////////

char*  pszInputBuff    = NULL;                  // ptr to buffer
int    nInputBuffSize  = (MAX_COMMAND_LEN+1);   // how big the buffer is
int    nInputLen       = 0;                     // amount of data it's holding

///////////////////////////////////////////////////////////////////////////////

void ReadInputData ( int nTimeoutMillsecs )
{
    size_t  nMaxBytesToRead;
    int     nBytesRead;
    char*   pReadBuffer;

    // Wait for keyboard input data to arrive...

#if !defined( _MSVC_ )

    fd_set          input_fd_set;
    struct timeval  wait_interval_timeval;
    int             rc;

    FD_ZERO ( &input_fd_set );
    FD_SET  ( nInputStreamFileNum, &input_fd_set );

    wait_interval_timeval.tv_sec  =  nTimeoutMillsecs / 1000;
    wait_interval_timeval.tv_usec = (nTimeoutMillsecs % 1000) * 1000;

    if ((rc = select( nInputStreamFileNum+1, &input_fd_set, NULL, NULL, &wait_interval_timeval )) < 0)
    {
        if (HSO_EINTR == HSO_errno)
            return;             // (we were interrupted by a signal)

        // A bonafide error occurred; abort...

        WRMSG
        (
            HHC01511, "S"
            ,"select()"
            ,strerror(HSO_errno)
        );

        bDoneProcessing = TRUE;     // (force main loop to exit)
        return;
    }

    // Has keyboard input data indeed arrived yet?

    if (!FD_ISSET( nInputStreamFileNum, &input_fd_set ))
        return;     // (nothing for us to do...)

#endif // !defined( _MSVC_ )

    // Ensure our buffer never overflows... (-2 because
    // we need room for at least 1 byte + NULL terminator)

    MINMAX(nInputLen,0,(nInputBuffSize-2));

    // Read input data into next available buffer location...
    // (nMaxBytesToRead-1 == room for NULL terminator)

    pReadBuffer     = (pszInputBuff   + nInputLen);
    nMaxBytesToRead = (nInputBuffSize - nInputLen) - 1;

#if !defined( _MSVC_ )

    if ((nBytesRead = read( nInputStreamFileNum, pReadBuffer, nMaxBytesToRead )) < 0)
    {
        if (EINTR == errno)
            return;             // (we were interrupted by a signal)

        // A bonafide error occurred; abort...

        WRMSG
        (
            HHC01511, "S"
            ,"read()"
            ,strerror(errno)
        );

        bDoneProcessing = TRUE;     // (force main loop to exit)
        return;
    }

#else // defined( _MSVC_ )

    if ( ( nBytesRead = w32_get_stdin_char( pReadBuffer, nTimeoutMillsecs ) ) <= 0 )
        return;

#endif // !defined( _MSVC_ )

    // Update amount of input data we have and
    // ensure that it's always NULL terminated...

    MINMAX(nBytesRead,0,nInputBuffSize);
    nInputLen += nBytesRead;
    MINMAX(nInputLen,0,(nInputBuffSize-1));
    *(pszInputBuff + nInputLen) = 0;
}

///////////////////////////////////////////////////////////////////////////////

char*  pszCommandBuff    = NULL;                // ptr to buffer
int    nCommandBuffSize  = (MAX_COMMAND_LEN+1); // how big the buffer is
int    nCommandLen       = 0;                   // amount of data it's holding

///////////////////////////////////////////////////////////////////////////////
// Process the data we just read from the input stream...

void  ProcessInputData ()
{
    char*  pNewLineChar;

    // Ensure our buffer is NULL terminated...

    MINMAX(nInputLen,0,(nInputBuffSize-1));
    *(pszInputBuff + nInputLen) = 0;

    // Input commands are delimited by newline characters...

    while (nInputLen && (pNewLineChar = strchr(pszInputBuff,'\n')) != NULL)
    {
        // Extract command from input buffer
        // into our command processing buffer...

        nCommandLen = (pNewLineChar - pszInputBuff);
        MINMAX(nCommandLen,0,(nCommandBuffSize-1));
        memcpy(pszCommandBuff, pszInputBuff, nCommandLen);
        *(pszCommandBuff + nCommandLen) = 0;

        // Process the extracted command...

        // Note that we always call the registered "panel_command" function
        // rather than call our "gui_panel_command" function directly. This
        // is in case some other DLL has overridden OUR command handler...

        panel_command ( pszCommandBuff );   // (call registered handler)

        // Shift remaining data back to beginning of input buffer...

        nInputLen = ((pszInputBuff + nInputLen) - (pNewLineChar+1));
        MINMAX(nInputLen,0,(nInputBuffSize-1));
        memmove(pszInputBuff,pNewLineChar+1,nInputLen);
        *(pszInputBuff + nInputLen) = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// (These are actually boolean flags..)

double gui_version           = 0.0;     // (version of HercGUI we're talking to)

BYTE   gui_forced_refresh    = 1;       // (force initial update refresh)

BYTE   gui_wants_gregs       = 0;
BYTE   gui_wants_gregs64     = 0;
BYTE   gui_wants_cregs       = 0;
BYTE   gui_wants_cregs64     = 0;
BYTE   gui_wants_aregs       = 0;
BYTE   gui_wants_fregs       = 0;
BYTE   gui_wants_fregs64     = 0;
BYTE   gui_wants_devlist     = 0;
BYTE   gui_wants_new_devlist = 1;       // (should always be initially on)

BYTE   gui_wants_aggregates  = 1;
BYTE   gui_wants_cpupct      = 0;
BYTE   gui_wants_cpupct_all  = 0;
int    prev_cpupct    [ MAX_CPU_ENGS ];
U32    prev_mips_rate  = 0;
U32    prev_sios_rate  = 0;

///////////////////////////////////////////////////////////////////////////////
// Our Hercules "panel_command" override...

void*  gui_panel_command (char* pszCommand)
{
    void* (*next_panel_command_handler)(char* pszCommand);

    // Special GUI commands start with ']'. At the moment, all these special
    // gui commands tell us is what status information it's interested in...

    if ( ']' != *pszCommand )
        goto NotSpecialGUICommand;

    gui_forced_refresh = 1;                         // (forced update refresh)

    pszCommand++;                                   // (bump past ']')

    if (strncasecmp(pszCommand,"VERS=",5) == 0)
    {
        gui_version = atof(pszCommand+5);
        return NULL;
    }

    if (strncasecmp(pszCommand,"SCD=",4) == 0)
    {
        // (set current directory)
        if (chdir(pszCommand+4) != 0)
        {
            // (inform gui of error)
            char *cwd = getcwd( NULL, 0 );
            if (cwd)
            {
                debug_cd_cmd( cwd );
                free( cwd );
            }
        }
        return NULL;
    }

    if (strncasecmp(pszCommand,"GREGS=",6) == 0)
    {
        gui_wants_gregs = atoi(pszCommand+6);
        return NULL;
    }

    if (strncasecmp(pszCommand,"GREGS64=",8) == 0)
    {
        gui_wants_gregs64 = atoi(pszCommand+8);
        return NULL;
    }

    if (strncasecmp(pszCommand,"CREGS=",6) == 0)
    {
        gui_wants_cregs = atoi(pszCommand+6);
        return NULL;
    }

    if (strncasecmp(pszCommand,"CREGS64=",8) == 0)
    {
        gui_wants_cregs64 = atoi(pszCommand+8);
        return NULL;
    }

    if (strncasecmp(pszCommand,"AREGS=",6) == 0)
    {
        gui_wants_aregs = atoi(pszCommand+6);
        return NULL;
    }

    if (strncasecmp(pszCommand,"FREGS=",6) == 0)
    {
        gui_wants_fregs = atoi(pszCommand+6);
        return NULL;
    }

    if (strncasecmp(pszCommand,"FREGS64=",8) == 0)
    {
        gui_wants_fregs64 = atoi(pszCommand+8);
        return NULL;
    }

    if (strncasecmp(pszCommand,"DEVLIST=",8) == 0)
    {
        gui_wants_devlist = atoi(pszCommand+8);
        if ( gui_wants_devlist )
            gui_wants_new_devlist = 0;
        return NULL;
    }

    if (strncasecmp(pszCommand,"NEWDEVLIST=",11) == 0)
    {
        gui_wants_new_devlist = atoi(pszCommand+11);
        if ( gui_wants_new_devlist )
            gui_wants_devlist = 0;
        return NULL;
    }

    if (strncasecmp( pszCommand, "MAINSTOR=", 9 ) == 0)
    {
        char buffer[64] = {0};

        MSGBUF( buffer, "%"PRIu64, (U64) pTargetCPU_REGS->mainstor );
        TRACE("**** sending: \"MAINSTOR=%s\"\n", buffer );
        gui_fprintf( fStatusStream, "MAINSTOR=%s\n", buffer );

        // Here's a trick! Hercules reports its version number to the GUI
        // by means of the MAINSIZE value! Later releases of HercGUI know
        // to interpret mainsizes less than 1000 as Hercule's version number.
        // Earlier versions of HercGUI will simply try to interpret it as
        // the actual mainsize, but no real harm is done since we immediately
        // send it the CORRECT mainsize immediately afterwards. This allows
        // future versions of HercGUI to know which version of Hercules they
        // are talking to and thus whether it supports a given feature or not.

#if defined( _MSVC_ )

        // PROGRAMMING NOTE: we use 'VERS_MAJ' and 'VERS_INT' here and NOT
        // the VERSION string since the 'VERSION' string can be any value
        // the user wants and thus might not be numeric nor even correspond
        // at all to Hercules's actual build version.
        //
        // The VERS_MAJ and VERS_INT variables however are ALWAYS be numbers
        // and thus should ALWAYS equal Hercules's actual build version. The
        // makefile.bat file on Windows guarantees this and ensures neither
        // has any leading zeroes (which would cause them to be intrepreted
        // by the C compiler as invalid octal numbers for e.g. 3.08, etc).

        MSGBUF( buffer, "%u.%u", VERS_MAJ, VERS_INT );
#else
        //  VERSION is set in configure.ac as x.xx. (why we insist on using
        //  two digits for the second part is unclear) However, if 'xx' is
        //  greater than '07' (as it would be for Hercules versions 3.08 and
        //  3.09 for example) then printing VERS_INT with a %d format fails
        //  with a compiler error about it being an invalid octal constant.
        //
        //  On Windows the makefile.bat build script used to build Hercules
        //  with ensures this will never happen. On non-Windows platforms
        //  however it could conceivably still happen, so for the time being
        //  we have no choice but to use the full VERSION string instead and
        //  hope all non-Windows GUIs will be able to properly parse it.

        MSGBUF( buffer, "%s", VERSION );
#endif
        TRACE("**** sending: \"MAINSIZE=%s\"\n", buffer );
        gui_fprintf( fStatusStream, "MAINSIZE=%s\n", buffer );

        if (gui_version < 1.12)
        {
            // Older versions of HercGUI only support
            // maximum 32-bit (4GB) mainsize values

            MSGBUF( buffer, "%"PRIu32, (U32) sysblk.mainsize );
            TRACE("**** (gui_version < 1.12): sending: \"MAINSIZE=%s\"\n", buffer );
        }
        else
        {
            // Newer versions of HercGUI now support
            // up to 64-bit (8M TB) mainsize values

            MSGBUF( buffer, "%"PRIu64, (U64) sysblk.mainsize );
            TRACE("**** (gui_version >= 1.12): sending: \"MAINSIZE=%s\"\n", buffer );
        }

        gui_fprintf( fStatusStream, "MAINSIZE=%s\n", buffer );
        return NULL;
    }

    if (strncasecmp(pszCommand,"CPUPCT=",7) == 0)
    {
        gui_wants_cpupct = atoi(pszCommand+7);
        return NULL;
    }

    if (strncasecmp(pszCommand,"CPUPCTALL=",10) == 0)
    {
        if (!(gui_wants_cpupct_all = atoi(pszCommand+10)))
            memset( &prev_cpupct[0], 0xFF, sizeof(prev_cpupct) );
        return NULL;
    }

    if (strncasecmp(pszCommand,"AGGREGATE=",10) == 0)
    {
        gui_wants_aggregates = atoi(pszCommand+10);
        gui_forced_refresh = 1;
        return NULL;
    }

    // Silently ignore any unrecognized special GUI commands...

    return NULL;        // (silently ignore it)

NotSpecialGUICommand:

    // Otherwise it's not a command that we handle. Call the next higher
    // level command handler which, under normal circumstances SHOULD be
    // Hercules's "panel_command" function, but which MAY have been over-
    // ridden by yet some OTHER dynamically loaded command handler...

    next_panel_command_handler = hdl_next( &gui_panel_command );

    if (!next_panel_command_handler)    // (extremely unlikely!)
        return (char *)-1;              // (extremely unlikely!)

    return  next_panel_command_handler( pszCommand );
}

///////////////////////////////////////////////////////////////////////////////
// Status updating control fields...

QWORD  psw, prev_psw;
BYTE   wait_bit;
BYTE   prev_cpustate   = 0xFF;
U64    prev_instcount  = 0;
U64    curr_instcount  = 0;

U32    prev_gr   [16];
U64    prev_gr64 [16];

U32    prev_cr   [16];
U64    prev_cr64 [16];

U32    prev_ar   [16];

U32    prev_fpr  [8*2];
U32    prev_fpr64[16*2];

///////////////////////////////////////////////////////////////////////////////
// Send status information messages back to the gui...

void  UpdateStatus ()
{
    BOOL  bStatusChanged = FALSE;   // (whether or not anything has changed)

    if (sysblk.shutdown) return;

    copy_psw(pTargetCPU_REGS, psw);
    wait_bit = (psw[1] & 0x02);

    // The SYS light and %CPU-Utilization
    // information we send *ALL* the time...

    if (!(0
        || CPUSTATE_STOPPING == pTargetCPU_REGS->cpustate
        || CPUSTATE_STOPPED  == pTargetCPU_REGS->cpustate
    ))
    {
        gui_fprintf(fStatusStream,

            "SYS=%c\n"

            ,wait_bit ? '0' : '1'
        );
    }

    if (gui_wants_cpupct)
    {
        if (gui_wants_aggregates)
        {
            int cpu, cpupct = 0, started = 0;
            for (cpupct=0, cpu=0; cpu < sysblk.maxcpu; cpu++)
            {
                if (1
                    && IS_CPU_ONLINE( cpu )
                    && CPUSTATE_STARTED == sysblk.regs[ cpu ]->cpustate
                )
                {
                    started++;
                    cpupct += sysblk.regs[ cpu ]->cpupct;
                }
            }
            gui_fprintf(fStatusStream,

                "CPUPCT=%d\n"

                ,started ? (cpupct / started) : 0
            );
        }
        else
        {
            gui_fprintf(fStatusStream,

                "CPUPCT=%d\n"

                ,pTargetCPU_REGS->cpupct
            );
        }
    }
    if (gui_wants_cpupct_all)
    {
        int  i, cpupct;

        for (i = 0; i < sysblk.hicpu; i++)
        {
            if (0
                || !IS_CPU_ONLINE(i)
                || CPUSTATE_STARTED != sysblk.regs[i]->cpustate
            )
                cpupct = 0;
            else
                cpupct = sysblk.regs[i]->cpupct;

            if (cpupct != prev_cpupct[i])
            {
                prev_cpupct[i] = cpupct;
                gui_fprintf( fStatusStream, "CPUPCT%02d=%d\n", i, cpupct );
            }
        }
    }

    // Determine if we need to inform the GUI of anything...

    bStatusChanged = FALSE;   // (whether or not anything has changed)

    curr_instcount = gui_wants_aggregates ?
        sysblk.instcount : INSTCOUNT( pTargetCPU_REGS );

    if (0
        || gui_forced_refresh
        || pTargetCPU_REGS           != pPrevTargetCPU_REGS
        || pcpu                      != prev_pcpu
        || curr_instcount            != prev_instcount
        || pTargetCPU_REGS->cpustate != prev_cpustate
        || memcmp( prev_psw, psw, sizeof( prev_psw )) != 0
    )
    {
        bStatusChanged = TRUE;          // (something has indeed changed...)

        if (gui_forced_refresh)         // (forced refresh?)
            HandleForcedRefresh();      // (reset all prev values)

        // Save new values for next time...

        pPrevTargetCPU_REGS = pTargetCPU_REGS;
        prev_cpustate       = pTargetCPU_REGS->cpustate;
        prev_instcount      = curr_instcount;
        prev_pcpu           = pcpu;
        memcpy( prev_psw, psw, sizeof( prev_psw ));
    }

    // If anything has changed, inform the GUI...

    if (bStatusChanged)
    {
        UpdateCPUStatus();      // (update the status line info...)
        UpdateRegisters();      // (update the registers display...)
    }

    // PROGRAMMING NOTE: my original [rather poorly designed I admit] logic
    // sent device status messages to the GUI *continuously* (i.e. all the
    // time), even when both Herc and the channel subsystem was idle. This
    // proved to be terribly inefficient, causing the GUI to consume *FAR*
    // too much valuable CPU cycles parsing all of those messages.

    // Thus, starting with this version of dyngui, we now only send device
    // status messages to the GUI only whenever the device's status actually
    // changes, but only if it (the GUI) specifically requests such notifi-
    // cations of course (via the new "]NEWDEVLIST=" special message).

    // The new(er) version of HercGUI understands (and thus requests) these
    // newer format device status messages, but older versions of HercGUI
    // of course do not. Thus in order to remain compatible with the current
    // (older) version of the GUI, we still need to support the inefficient
    // technique of constantly sending a constant stream of device status
    // messages.

    // Eventually at some point this existing original inefficient technique
    // logic will be removed (once everyone has had time to upgrade to the
    // newer version of HercGUI), but for now, at least for the next couple
    // of HercGUI release cycles, we need to keep it.

    if (gui_wants_devlist)      // (if the device list is visible)
        UpdateDeviceStatus();   // (update the list of devices...)

    else // (the two options are mutually exclusive from one another)

    if (gui_wants_new_devlist)  // (if the device list is visible)
        NewUpdateDevStats();    // (update the list of devices...)

    gui_forced_refresh  = 0;    // (reset switch; must follow devlist update)
}

///////////////////////////////////////////////////////////////////////////////
// Forced GUI Refresh: reset all "previous" values to force update...

void HandleForcedRefresh()
{
    prev_mips_rate          = INT_MAX;
    prev_sios_rate          = INT_MAX;
    prev_instcount          = ULLONG_MAX;
    prev_pcpu               = INT_MAX;
    pPrevTargetCPU_REGS     = NULL;
    prev_cpustate           = 0xFF;
    memset( prev_psw,         0xFF,  sizeof(prev_psw) );

    memset(   &prev_gr   [0], 0xFF,
        sizeof(prev_gr) );

    memset(   &prev_cr   [0], 0xFF,
        sizeof(prev_cr) );

    memset(   &prev_ar   [0], 0xFF,
        sizeof(prev_ar) );

    memset(   &prev_fpr  [0], 0xFF,
        sizeof(prev_fpr) );

    memset(   &prev_gr64 [0], 0xFF,
        sizeof(prev_gr64) );

    memset(   &prev_cr64 [0], 0xFF,
        sizeof(prev_cr64) );

    memset(   &prev_fpr64[0], 0xFF,
        sizeof(prev_fpr64) );

    memset(   &prev_cpupct   [0], 0xFF,
        sizeof(prev_cpupct   ) );
}

///////////////////////////////////////////////////////////////////////////////
// Send status information messages back to the gui...

void  UpdateCPUStatus ()
{
    if (sysblk.shutdown) return;

    if (pTargetCPU_REGS == &sysblk.dummyregs)
    {
        // pTargetCPU_REGS == &sysblk.dummyregs; cpu is offline

        gui_fprintf(fStatusStream, "STATUS="

            "%s%02X (((((((((((((((((((((((( OFFLINE ))))))))))))))))))))))))\n",

            PTYPSTR(pcpu) ,pcpu);
    }
    else // pTargetCPU_REGS != &sysblk.dummyregs; cpu is online
    {
        // CPU status line...  (PSW, status indicators, and instruction count)

        gui_fprintf(fStatusStream, "STATUS="

            "%s%02X "

            "PSW=%2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X "

            "%c%c%c%c%c%c%c%c "

            "instcount=%"PRIu64"\n"

            ,PTYPSTR(pTargetCPU_REGS->cpuad), pTargetCPU_REGS->cpuad

            ,psw[0], psw[1], psw[2],  psw[3]
            ,psw[4], psw[5], psw[6],  psw[7]
            ,psw[8], psw[9], psw[10], psw[11], psw[12], psw[13], psw[14], psw[15]

            ,CPUSTATE_STOPPED == pTargetCPU_REGS->cpustate ? 'M' : '.'
            ,sysblk.instbreak                              ? 'T' : '.'
            ,wait_bit                                      ? 'W' : '.'
            ,pTargetCPU_REGS->loadstate                    ? 'L' : '.'
            ,pTargetCPU_REGS->checkstop                    ? 'C' : '.'
            ,PROBSTATE(&pTargetCPU_REGS->psw)              ? 'P' : '.'
            ,
#if        defined(_FEATURE_SIE)
            SIE_MODE(pTargetCPU_REGS)                      ? 'S' : '.'
#else  // !defined(_FEATURE_SIE)
                                                                   '.'
#endif //  defined(_FEATURE_SIE)
            ,
#if        defined(_900)
            ARCH_900_IDX == pTargetCPU_REGS->arch_mode     ? 'Z' : '.'
#else  // !defined(_900)
                                                                   '.'
#endif //  defined(_900)
            ,curr_instcount
        );

    } // endif cpu is online/offline

    // MIPS rate and SIOS rate...
    {
        U32* mipsrate;
        U32* siosrate;

        if (gui_wants_aggregates)
        {
            mipsrate = &sysblk.mipsrate;
            siosrate = &sysblk.siosrate;
        }
        else
        {
            mipsrate = &pTargetCPU_REGS->mipsrate;
            siosrate = &pTargetCPU_REGS->siosrate;
        }

        if (*mipsrate != prev_mips_rate)
        {
            gui_fprintf( fStatusStream,

                "MIPS=%4d.%2.2d\n"

                , *mipsrate / 1000000
                ,(*mipsrate % 1000000) / 10000
            );

            prev_mips_rate = *mipsrate;
        }

        if (*siosrate != prev_sios_rate)
        {
            gui_fprintf( fStatusStream,

                "SIOS=%4d\n"

                ,*siosrate
            );

            prev_sios_rate = *siosrate;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Send status information messages back to the gui...

#define REG32FMT  "%8.8"PRIX32
#define REG64FMT  "%16.16"PRIX64

void  UpdateRegisters ()
{
    if (sysblk.shutdown) return;

    if (gui_wants_gregs)
    {
        if (0
            || prev_gr[0] != pTargetCPU_REGS->GR_L(0)
            || prev_gr[1] != pTargetCPU_REGS->GR_L(1)
            || prev_gr[2] != pTargetCPU_REGS->GR_L(2)
            || prev_gr[3] != pTargetCPU_REGS->GR_L(3)
        )
        {
            prev_gr[0] = pTargetCPU_REGS->GR_L(0);
            prev_gr[1] = pTargetCPU_REGS->GR_L(1);
            prev_gr[2] = pTargetCPU_REGS->GR_L(2);
            prev_gr[3] = pTargetCPU_REGS->GR_L(3);

            gui_fprintf(fStatusStream,

                "GR0-3="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->GR_L(0)
                ,pTargetCPU_REGS->GR_L(1)
                ,pTargetCPU_REGS->GR_L(2)
                ,pTargetCPU_REGS->GR_L(3)
            );
        }

        if (0
            || prev_gr[4] != pTargetCPU_REGS->GR_L(4)
            || prev_gr[5] != pTargetCPU_REGS->GR_L(5)
            || prev_gr[6] != pTargetCPU_REGS->GR_L(6)
            || prev_gr[7] != pTargetCPU_REGS->GR_L(7)
        )
        {
            prev_gr[4] = pTargetCPU_REGS->GR_L(4);
            prev_gr[5] = pTargetCPU_REGS->GR_L(5);
            prev_gr[6] = pTargetCPU_REGS->GR_L(6);
            prev_gr[7] = pTargetCPU_REGS->GR_L(7);

            gui_fprintf(fStatusStream,

                "GR4-7="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->GR_L(4)
                ,pTargetCPU_REGS->GR_L(5)
                ,pTargetCPU_REGS->GR_L(6)
                ,pTargetCPU_REGS->GR_L(7)
            );
        }

        if (0
            || prev_gr[8]  != pTargetCPU_REGS->GR_L(8)
            || prev_gr[9]  != pTargetCPU_REGS->GR_L(9)
            || prev_gr[10] != pTargetCPU_REGS->GR_L(10)
            || prev_gr[11] != pTargetCPU_REGS->GR_L(11)
        )
        {
            prev_gr[8]  = pTargetCPU_REGS->GR_L(8);
            prev_gr[9]  = pTargetCPU_REGS->GR_L(9);
            prev_gr[10] = pTargetCPU_REGS->GR_L(10);
            prev_gr[11] = pTargetCPU_REGS->GR_L(11);

            gui_fprintf(fStatusStream,

                "GR8-B="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->GR_L(8)
                ,pTargetCPU_REGS->GR_L(9)
                ,pTargetCPU_REGS->GR_L(10)
                ,pTargetCPU_REGS->GR_L(11)
            );
        }

        if (0
            || prev_gr[12] != pTargetCPU_REGS->GR_L(12)
            || prev_gr[13] != pTargetCPU_REGS->GR_L(13)
            || prev_gr[14] != pTargetCPU_REGS->GR_L(14)
            || prev_gr[15] != pTargetCPU_REGS->GR_L(15)
        )
        {
            prev_gr[12] = pTargetCPU_REGS->GR_L(12);
            prev_gr[13] = pTargetCPU_REGS->GR_L(13);
            prev_gr[14] = pTargetCPU_REGS->GR_L(14);
            prev_gr[15] = pTargetCPU_REGS->GR_L(15);

            gui_fprintf(fStatusStream,

                "GRC-F="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->GR_L(12)
                ,pTargetCPU_REGS->GR_L(13)
                ,pTargetCPU_REGS->GR_L(14)
                ,pTargetCPU_REGS->GR_L(15)
            );
        }
    }

    if (gui_wants_gregs64)
    {
        if (0
            || prev_gr64[0] != pTargetCPU_REGS->GR_G(0)
            || prev_gr64[1] != pTargetCPU_REGS->GR_G(1)
        )
        {
            prev_gr64[0] = pTargetCPU_REGS->GR_G(0);
            prev_gr64[1] = pTargetCPU_REGS->GR_G(1);

            gui_fprintf(fStatusStream,

                "64_GR0-1="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(0)
                ,pTargetCPU_REGS->GR_G(1)
            );
        }

        if (0
            || prev_gr64[2] != pTargetCPU_REGS->GR_G(2)
            || prev_gr64[3] != pTargetCPU_REGS->GR_G(3)
        )
        {
            prev_gr64[2] = pTargetCPU_REGS->GR_G(2);
            prev_gr64[3] = pTargetCPU_REGS->GR_G(3);

            gui_fprintf(fStatusStream,

                "64_GR2-3="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(2)
                ,pTargetCPU_REGS->GR_G(3)
            );
        }

        if (0
            || prev_gr64[4] != pTargetCPU_REGS->GR_G(4)
            || prev_gr64[5] != pTargetCPU_REGS->GR_G(5)
        )
        {
            prev_gr64[4] = pTargetCPU_REGS->GR_G(4);
            prev_gr64[5] = pTargetCPU_REGS->GR_G(5);

            gui_fprintf(fStatusStream,

                "64_GR4-5="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(4)
                ,pTargetCPU_REGS->GR_G(5)
            );
        }

        if (0
            || prev_gr64[6] != pTargetCPU_REGS->GR_G(6)
            || prev_gr64[7] != pTargetCPU_REGS->GR_G(7)
        )
        {
            prev_gr64[6] = pTargetCPU_REGS->GR_G(6);
            prev_gr64[7] = pTargetCPU_REGS->GR_G(7);

            gui_fprintf(fStatusStream,

                "64_GR6-7="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(6)
                ,pTargetCPU_REGS->GR_G(7)
            );
        }

        if (0
            || prev_gr64[8]  != pTargetCPU_REGS->GR_G(8)
            || prev_gr64[9]  != pTargetCPU_REGS->GR_G(9)
        )
        {
            prev_gr64[8] = pTargetCPU_REGS->GR_G(8);
            prev_gr64[9] = pTargetCPU_REGS->GR_G(9);

            gui_fprintf(fStatusStream,

                "64_GR8-9="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(8)
                ,pTargetCPU_REGS->GR_G(9)
            );
        }

        if (0
            || prev_gr64[10] != pTargetCPU_REGS->GR_G(10)
            || prev_gr64[11] != pTargetCPU_REGS->GR_G(11)
        )
        {
            prev_gr64[10] = pTargetCPU_REGS->GR_G(10);
            prev_gr64[11] = pTargetCPU_REGS->GR_G(11);

            gui_fprintf(fStatusStream,

                "64_GRA-B="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(10)
                ,pTargetCPU_REGS->GR_G(11)
            );
        }

        if (0
            || prev_gr64[12] != pTargetCPU_REGS->GR_G(12)
            || prev_gr64[13] != pTargetCPU_REGS->GR_G(13)
        )
        {
            prev_gr64[12] = pTargetCPU_REGS->GR_G(12);
            prev_gr64[13] = pTargetCPU_REGS->GR_G(13);

            gui_fprintf(fStatusStream,

                "64_GRC-D="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(12)
                ,pTargetCPU_REGS->GR_G(13)
            );
        }

        if (0
            || prev_gr64[14] != pTargetCPU_REGS->GR_G(14)
            || prev_gr64[15] != pTargetCPU_REGS->GR_G(15)
        )
        {
            prev_gr64[14] = pTargetCPU_REGS->GR_G(14);
            prev_gr64[15] = pTargetCPU_REGS->GR_G(15);

            gui_fprintf(fStatusStream,

                "64_GRE-F="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->GR_G(14)
                ,pTargetCPU_REGS->GR_G(15)
            );
        }
    }

    if (gui_wants_cregs)
    {
        if (0
            || prev_cr[0] != pTargetCPU_REGS->CR_L(0)
            || prev_cr[1] != pTargetCPU_REGS->CR_L(1)
            || prev_cr[2] != pTargetCPU_REGS->CR_L(2)
            || prev_cr[3] != pTargetCPU_REGS->CR_L(3)
        )
        {
            prev_cr[0] = pTargetCPU_REGS->CR_L(0);
            prev_cr[1] = pTargetCPU_REGS->CR_L(1);
            prev_cr[2] = pTargetCPU_REGS->CR_L(2);
            prev_cr[3] = pTargetCPU_REGS->CR_L(3);

            gui_fprintf(fStatusStream,

                "CR0-3="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->CR_L(0)
                ,pTargetCPU_REGS->CR_L(1)
                ,pTargetCPU_REGS->CR_L(2)
                ,pTargetCPU_REGS->CR_L(3)
            );
        }

        if (0
            || prev_cr[4] != pTargetCPU_REGS->CR_L(4)
            || prev_cr[5] != pTargetCPU_REGS->CR_L(5)
            || prev_cr[6] != pTargetCPU_REGS->CR_L(6)
            || prev_cr[7] != pTargetCPU_REGS->CR_L(7)
        )
        {
            prev_cr[4] = pTargetCPU_REGS->CR_L(4);
            prev_cr[5] = pTargetCPU_REGS->CR_L(5);
            prev_cr[6] = pTargetCPU_REGS->CR_L(6);
            prev_cr[7] = pTargetCPU_REGS->CR_L(7);

            gui_fprintf(fStatusStream,

                "CR4-7="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->CR_L(4)
                ,pTargetCPU_REGS->CR_L(5)
                ,pTargetCPU_REGS->CR_L(6)
                ,pTargetCPU_REGS->CR_L(7)
            );
        }

        if (0
            || prev_cr[8]  != pTargetCPU_REGS->CR_L(8)
            || prev_cr[9]  != pTargetCPU_REGS->CR_L(9)
            || prev_cr[10] != pTargetCPU_REGS->CR_L(10)
            || prev_cr[11] != pTargetCPU_REGS->CR_L(11)
        )
        {
            prev_cr[8]  = pTargetCPU_REGS->CR_L(8);
            prev_cr[9]  = pTargetCPU_REGS->CR_L(9);
            prev_cr[10] = pTargetCPU_REGS->CR_L(10);
            prev_cr[10] = pTargetCPU_REGS->CR_L(10);

            gui_fprintf(fStatusStream,

                "CR8-B="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->CR_L(8)
                ,pTargetCPU_REGS->CR_L(9)
                ,pTargetCPU_REGS->CR_L(10)
                ,pTargetCPU_REGS->CR_L(11)
            );
        }

        if (0
            || prev_cr[12] != pTargetCPU_REGS->CR_L(12)
            || prev_cr[13] != pTargetCPU_REGS->CR_L(13)
            || prev_cr[14] != pTargetCPU_REGS->CR_L(14)
            || prev_cr[15] != pTargetCPU_REGS->CR_L(15)
        )
        {
            prev_cr[12] = pTargetCPU_REGS->CR_L(12);
            prev_cr[13] = pTargetCPU_REGS->CR_L(13);
            prev_cr[14] = pTargetCPU_REGS->CR_L(14);
            prev_cr[15] = pTargetCPU_REGS->CR_L(15);

            gui_fprintf(fStatusStream,

                "CRC-F="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->CR_L(12)
                ,pTargetCPU_REGS->CR_L(13)
                ,pTargetCPU_REGS->CR_L(14)
                ,pTargetCPU_REGS->CR_L(15)
            );
        }
    }

    if (gui_wants_cregs64)
    {
        if (0
            || prev_cr64[0] != pTargetCPU_REGS->CR_G(0)
            || prev_cr64[1] != pTargetCPU_REGS->CR_G(1)
        )
        {
            prev_cr64[0] = pTargetCPU_REGS->CR_G(0);
            prev_cr64[1] = pTargetCPU_REGS->CR_G(1);

            gui_fprintf(fStatusStream,

                "64_CR0-1="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(0)
                ,pTargetCPU_REGS->CR_G(1)
            );
        }

        if (0
            || prev_cr64[2] != pTargetCPU_REGS->CR_G(2)
            || prev_cr64[3] != pTargetCPU_REGS->CR_G(3)
        )
        {
            prev_cr64[2] = pTargetCPU_REGS->CR_G(2);
            prev_cr64[3] = pTargetCPU_REGS->CR_G(3);

            gui_fprintf(fStatusStream,

                "64_CR2-3="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(2)
                ,pTargetCPU_REGS->CR_G(3)
            );
        }

        if (0
            || prev_cr64[4] != pTargetCPU_REGS->CR_G(4)
            || prev_cr64[5] != pTargetCPU_REGS->CR_G(5)
        )
        {
            prev_cr64[4] = pTargetCPU_REGS->CR_G(4);
            prev_cr64[5] = pTargetCPU_REGS->CR_G(5);

            gui_fprintf(fStatusStream,

                "64_CR4-5="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(4)
                ,pTargetCPU_REGS->CR_G(5)
            );
        }

        if (0
            || prev_cr64[6] != pTargetCPU_REGS->CR_G(6)
            || prev_cr64[7] != pTargetCPU_REGS->CR_G(7)
        )
        {
            prev_cr64[6] = pTargetCPU_REGS->CR_G(6);
            prev_cr64[7] = pTargetCPU_REGS->CR_G(7);

            gui_fprintf(fStatusStream,

                "64_CR6-7="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(6)
                ,pTargetCPU_REGS->CR_G(7)
            );
        }

        if (0
            || prev_cr64[8]  != pTargetCPU_REGS->CR_G(8)
            || prev_cr64[9]  != pTargetCPU_REGS->CR_G(9)
        )
        {
            prev_cr64[8] = pTargetCPU_REGS->CR_G(8);
            prev_cr64[9] = pTargetCPU_REGS->CR_G(9);

            gui_fprintf(fStatusStream,

                "64_CR8-9="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(8)
                ,pTargetCPU_REGS->CR_G(9)
            );
        }

        if (0
            || prev_cr64[10] != pTargetCPU_REGS->CR_G(10)
            || prev_cr64[11] != pTargetCPU_REGS->CR_G(11)
        )
        {
            prev_cr64[10] = pTargetCPU_REGS->CR_G(10);
            prev_cr64[11] = pTargetCPU_REGS->CR_G(11);

            gui_fprintf(fStatusStream,

                "64_CRA-B="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(10)
                ,pTargetCPU_REGS->CR_G(11)
            );
        }

        if (0
            || prev_cr64[12] != pTargetCPU_REGS->CR_G(12)
            || prev_cr64[13] != pTargetCPU_REGS->CR_G(13)
        )
        {
            prev_cr64[12] = pTargetCPU_REGS->CR_G(12);
            prev_cr64[13] = pTargetCPU_REGS->CR_G(13);

            gui_fprintf(fStatusStream,

                "64_CRC-D="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(12)
                ,pTargetCPU_REGS->CR_G(13)
            );
        }

        if (0
            || prev_cr64[14] != pTargetCPU_REGS->CR_G(14)
            || prev_cr64[15] != pTargetCPU_REGS->CR_G(15)
        )
        {
            prev_cr64[14] = pTargetCPU_REGS->CR_G(14);
            prev_cr64[15] = pTargetCPU_REGS->CR_G(15);

            gui_fprintf(fStatusStream,

                "64_CRE-F="REG64FMT" "REG64FMT"\n"

                ,pTargetCPU_REGS->CR_G(14)
                ,pTargetCPU_REGS->CR_G(15)
            );
        }
    }

    if (gui_wants_aregs)
    {
        if (0
            || prev_ar[0] != pTargetCPU_REGS->AR(0)
            || prev_ar[1] != pTargetCPU_REGS->AR(1)
            || prev_ar[2] != pTargetCPU_REGS->AR(2)
            || prev_ar[3] != pTargetCPU_REGS->AR(3)
        )
        {
            prev_ar[0] = pTargetCPU_REGS->AR(0);
            prev_ar[1] = pTargetCPU_REGS->AR(1);
            prev_ar[2] = pTargetCPU_REGS->AR(2);
            prev_ar[3] = pTargetCPU_REGS->AR(3);

            gui_fprintf(fStatusStream,

                "AR0-3="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->AR(0)
                ,pTargetCPU_REGS->AR(1)
                ,pTargetCPU_REGS->AR(2)
                ,pTargetCPU_REGS->AR(3)
            );
        }

        if (0
            || prev_ar[4] != pTargetCPU_REGS->AR(4)
            || prev_ar[5] != pTargetCPU_REGS->AR(5)
            || prev_ar[6] != pTargetCPU_REGS->AR(6)
            || prev_ar[7] != pTargetCPU_REGS->AR(7)
        )
        {
            prev_ar[4] = pTargetCPU_REGS->AR(4);
            prev_ar[5] = pTargetCPU_REGS->AR(5);
            prev_ar[6] = pTargetCPU_REGS->AR(6);
            prev_ar[7] = pTargetCPU_REGS->AR(7);

            gui_fprintf(fStatusStream,

                "AR4-7="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->AR(4)
                ,pTargetCPU_REGS->AR(5)
                ,pTargetCPU_REGS->AR(6)
                ,pTargetCPU_REGS->AR(7)
            );
        }

        if (0
            || prev_ar[8]  != pTargetCPU_REGS->AR(8)
            || prev_ar[9]  != pTargetCPU_REGS->AR(9)
            || prev_ar[10] != pTargetCPU_REGS->AR(10)
            || prev_ar[11] != pTargetCPU_REGS->AR(11)
        )
        {
            prev_ar[8]  = pTargetCPU_REGS->AR(8);
            prev_ar[9]  = pTargetCPU_REGS->AR(9);
            prev_ar[10] = pTargetCPU_REGS->AR(10);
            prev_ar[11] = pTargetCPU_REGS->AR(11);

            gui_fprintf(fStatusStream,

                "AR8-B="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->AR(8)
                ,pTargetCPU_REGS->AR(9)
                ,pTargetCPU_REGS->AR(10)
                ,pTargetCPU_REGS->AR(11)
            );
        }

        if (0
            || prev_ar[12] != pTargetCPU_REGS->AR(12)
            || prev_ar[13] != pTargetCPU_REGS->AR(13)
            || prev_ar[14] != pTargetCPU_REGS->AR(14)
            || prev_ar[15] != pTargetCPU_REGS->AR(15)
        )
        {
            prev_ar[12] = pTargetCPU_REGS->AR(12);
            prev_ar[13] = pTargetCPU_REGS->AR(13);
            prev_ar[14] = pTargetCPU_REGS->AR(14);
            prev_ar[15] = pTargetCPU_REGS->AR(15);

            gui_fprintf(fStatusStream,

                "ARC-F="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->AR(12)
                ,pTargetCPU_REGS->AR(13)
                ,pTargetCPU_REGS->AR(14)
                ,pTargetCPU_REGS->AR(15)
            );
        }
    }

    if (gui_wants_fregs)
    {
        if (0
            || prev_fpr[0] != pTargetCPU_REGS->fpr[0]
            || prev_fpr[1] != pTargetCPU_REGS->fpr[1]
            || prev_fpr[2] != pTargetCPU_REGS->fpr[2]
            || prev_fpr[3] != pTargetCPU_REGS->fpr[3]
        )
        {
            prev_fpr[0] = pTargetCPU_REGS->fpr[0];
            prev_fpr[1] = pTargetCPU_REGS->fpr[1];
            prev_fpr[2] = pTargetCPU_REGS->fpr[2];
            prev_fpr[3] = pTargetCPU_REGS->fpr[3];

            gui_fprintf(fStatusStream,

                "FR0-2="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[0]
                ,pTargetCPU_REGS->fpr[1]
                ,pTargetCPU_REGS->fpr[2]
                ,pTargetCPU_REGS->fpr[3]
            );
        }

        if (0
            || prev_fpr[4] != pTargetCPU_REGS->fpr[4]
            || prev_fpr[5] != pTargetCPU_REGS->fpr[5]
            || prev_fpr[6] != pTargetCPU_REGS->fpr[6]
            || prev_fpr[7] != pTargetCPU_REGS->fpr[7]
        )
        {
            prev_fpr[4] = pTargetCPU_REGS->fpr[4];
            prev_fpr[5] = pTargetCPU_REGS->fpr[5];
            prev_fpr[6] = pTargetCPU_REGS->fpr[6];
            prev_fpr[7] = pTargetCPU_REGS->fpr[7];

            gui_fprintf(fStatusStream,

                "FR4-6="REG32FMT" "REG32FMT" "REG32FMT" "REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[4]
                ,pTargetCPU_REGS->fpr[5]
                ,pTargetCPU_REGS->fpr[6]
                ,pTargetCPU_REGS->fpr[7]
            );
        }
    }

    if (gui_wants_fregs64)
    {
        if (0
            || prev_fpr64[0] != pTargetCPU_REGS->fpr[0]
            || prev_fpr64[1] != pTargetCPU_REGS->fpr[1]
            || prev_fpr64[2] != pTargetCPU_REGS->fpr[2]
            || prev_fpr64[3] != pTargetCPU_REGS->fpr[3]
        )
        {
            prev_fpr64[0] = pTargetCPU_REGS->fpr[0];
            prev_fpr64[1] = pTargetCPU_REGS->fpr[1];
            prev_fpr64[2] = pTargetCPU_REGS->fpr[2];
            prev_fpr64[3] = pTargetCPU_REGS->fpr[3];

            gui_fprintf(fStatusStream,

                "64_FR0-1="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[0]  ,pTargetCPU_REGS->fpr[1]
                ,pTargetCPU_REGS->fpr[2]  ,pTargetCPU_REGS->fpr[3]
            );
        }

        if (0
            || prev_fpr64[4] != pTargetCPU_REGS->fpr[4]
            || prev_fpr64[5] != pTargetCPU_REGS->fpr[5]
            || prev_fpr64[6] != pTargetCPU_REGS->fpr[6]
            || prev_fpr64[7] != pTargetCPU_REGS->fpr[7]
        )
        {
            prev_fpr64[4] = pTargetCPU_REGS->fpr[4];
            prev_fpr64[5] = pTargetCPU_REGS->fpr[5];
            prev_fpr64[6] = pTargetCPU_REGS->fpr[6];
            prev_fpr64[7] = pTargetCPU_REGS->fpr[7];

            gui_fprintf(fStatusStream,

                "64_FR2-3="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[4]  ,pTargetCPU_REGS->fpr[5]
                ,pTargetCPU_REGS->fpr[6]  ,pTargetCPU_REGS->fpr[7]
            );
        }

        if (0
            || prev_fpr64[8]  != pTargetCPU_REGS->fpr[8]
            || prev_fpr64[9]  != pTargetCPU_REGS->fpr[9]
            || prev_fpr64[10] != pTargetCPU_REGS->fpr[10]
            || prev_fpr64[11] != pTargetCPU_REGS->fpr[11]
        )
        {
            prev_fpr64[8]  = pTargetCPU_REGS->fpr[8];
            prev_fpr64[9]  = pTargetCPU_REGS->fpr[9];
            prev_fpr64[10] = pTargetCPU_REGS->fpr[10];
            prev_fpr64[11] = pTargetCPU_REGS->fpr[11];

            gui_fprintf(fStatusStream,

                "64_FR4-5="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[8]  ,pTargetCPU_REGS->fpr[9]
                ,pTargetCPU_REGS->fpr[10] ,pTargetCPU_REGS->fpr[11]

            );
        }

        if (0
            || prev_fpr64[12] != pTargetCPU_REGS->fpr[12]
            || prev_fpr64[13] != pTargetCPU_REGS->fpr[13]
            || prev_fpr64[14] != pTargetCPU_REGS->fpr[14]
            || prev_fpr64[15] != pTargetCPU_REGS->fpr[15]
        )
        {
            prev_fpr64[12] = pTargetCPU_REGS->fpr[12];
            prev_fpr64[13] = pTargetCPU_REGS->fpr[13];
            prev_fpr64[14] = pTargetCPU_REGS->fpr[14];
            prev_fpr64[15] = pTargetCPU_REGS->fpr[15];

            gui_fprintf(fStatusStream,

                "64_FR6-7="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[12] ,pTargetCPU_REGS->fpr[13]
                ,pTargetCPU_REGS->fpr[14] ,pTargetCPU_REGS->fpr[15]

            );
        }

        if (0
            || prev_fpr64[16] != pTargetCPU_REGS->fpr[16]
            || prev_fpr64[17] != pTargetCPU_REGS->fpr[17]
            || prev_fpr64[18] != pTargetCPU_REGS->fpr[18]
            || prev_fpr64[19] != pTargetCPU_REGS->fpr[19]
        )
        {
            prev_fpr64[16] = pTargetCPU_REGS->fpr[16];
            prev_fpr64[17] = pTargetCPU_REGS->fpr[17];
            prev_fpr64[18] = pTargetCPU_REGS->fpr[18];
            prev_fpr64[19] = pTargetCPU_REGS->fpr[19];

            gui_fprintf(fStatusStream,

                "64_FR8-9="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[16] ,pTargetCPU_REGS->fpr[17]
                ,pTargetCPU_REGS->fpr[18] ,pTargetCPU_REGS->fpr[19]
            );
        }

        if (0
            || prev_fpr64[20] != pTargetCPU_REGS->fpr[20]
            || prev_fpr64[21] != pTargetCPU_REGS->fpr[21]
            || prev_fpr64[22] != pTargetCPU_REGS->fpr[22]
            || prev_fpr64[23] != pTargetCPU_REGS->fpr[23]
        )
        {
            prev_fpr64[20] = pTargetCPU_REGS->fpr[20];
            prev_fpr64[21] = pTargetCPU_REGS->fpr[21];
            prev_fpr64[22] = pTargetCPU_REGS->fpr[22];
            prev_fpr64[23] = pTargetCPU_REGS->fpr[23];

            gui_fprintf(fStatusStream,

                "64_FRA-B="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[20] ,pTargetCPU_REGS->fpr[21]
                ,pTargetCPU_REGS->fpr[22] ,pTargetCPU_REGS->fpr[23]
            );
        }

        if (0
            || prev_fpr64[24] != pTargetCPU_REGS->fpr[24]
            || prev_fpr64[25] != pTargetCPU_REGS->fpr[25]
            || prev_fpr64[26] != pTargetCPU_REGS->fpr[26]
            || prev_fpr64[27] != pTargetCPU_REGS->fpr[27]
        )
        {
            prev_fpr64[24] = pTargetCPU_REGS->fpr[24];
            prev_fpr64[25] = pTargetCPU_REGS->fpr[25];
            prev_fpr64[26] = pTargetCPU_REGS->fpr[26];
            prev_fpr64[27] = pTargetCPU_REGS->fpr[27];

            gui_fprintf(fStatusStream,

                "64_FRC-D="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[24] ,pTargetCPU_REGS->fpr[25]
                ,pTargetCPU_REGS->fpr[26] ,pTargetCPU_REGS->fpr[27]
            );
        }

        if (0
            || prev_fpr64[28] != pTargetCPU_REGS->fpr[28]
            || prev_fpr64[29] != pTargetCPU_REGS->fpr[29]
            || prev_fpr64[30] != pTargetCPU_REGS->fpr[30]
            || prev_fpr64[31] != pTargetCPU_REGS->fpr[31]
        )
        {
            prev_fpr64[28] = pTargetCPU_REGS->fpr[28];
            prev_fpr64[29] = pTargetCPU_REGS->fpr[29];
            prev_fpr64[30] = pTargetCPU_REGS->fpr[30];
            prev_fpr64[31] = pTargetCPU_REGS->fpr[31];

            gui_fprintf(fStatusStream,

                "64_FRE-F="REG32FMT""REG32FMT" "REG32FMT""REG32FMT"\n"

                ,pTargetCPU_REGS->fpr[28] ,pTargetCPU_REGS->fpr[29]
                ,pTargetCPU_REGS->fpr[30] ,pTargetCPU_REGS->fpr[31]
            );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

char  szQueryDeviceBuff[ MAX_DEVICEQUERY_LEN + 1 ]; // (always +1 for safety!)

///////////////////////////////////////////////////////////////////////////////
// Send status information messages back to the gui...  (VERY inefficient!)

void  UpdateDeviceStatus ()
{
    DEVBLK* pDEVBLK;
    char*   pDEVClass;
    BYTE    chOnlineStat, chBusyStat, chPendingStat, chOpenStat;
    BYTE    lcss;
    char    lcss_num[4];

    if (sysblk.shutdown) return;

    // Process ALL the devices in the entire configuration each time...

    for (pDEVBLK = sysblk.firstdev; pDEVBLK != NULL; pDEVBLK = pDEVBLK->nextdev)
    {
        // Does this device actually exist in the configuration?

        if (!IS_DEV( pDEVBLK )) continue;   // (no, skip)

        // Retrieve this device's filename and optional settings parameter values...

        szQueryDeviceBuff[MAX_DEVICEQUERY_LEN] = 0; // (buffer allows room for 1 extra)

        (pDEVBLK->hnd->query)(pDEVBLK, &pDEVClass, MAX_DEVICEQUERY_LEN, szQueryDeviceBuff);

        if (0 != szQueryDeviceBuff[MAX_DEVICEQUERY_LEN])    // (buffer overflow?)
        {
            WRMSG
            (
                HHC01540, "E"
                ,SSID_TO_LCSS(pDEVBLK->ssid)
                ,pDEVBLK->devnum

            );
        }

        szQueryDeviceBuff[MAX_DEVICEQUERY_LEN] = 0;   // (enforce NULL termination)

        // Device status flags...
                                                                              chOnlineStat  =
                                                                              chBusyStat    =
                                                                              chPendingStat =
                                                                              chOpenStat    = '0';

        if ((!pDEVBLK->console && pDEVBLK->fd >= 0) ||
            ( pDEVBLK->console && pDEVBLK->connected))                        chOnlineStat  = '1';
        if (pDEVBLK->busy)                                                    chBusyStat    = '1';
        if (IOPENDING(pDEVBLK))                                               chPendingStat = '1';
        if (pDEVBLK->fd > MAX(STDIN_FILENO,MAX(STDOUT_FILENO,STDERR_FILENO))) chOpenStat    = '1';

        if ((lcss = SSID_TO_LCSS( pDEVBLK->ssid )) >= 1)
            MSGBUF( lcss_num, "%1d:", lcss );
        else
            lcss_num[0] = 0;

        // Send status message back to gui...

#if defined(_FEATURE_INTEGRATED_3270_CONSOLE)
        if (pDEVBLK == sysblk.sysgdev)
            gui_fprintf( fStatusStream,

                "DEV=%s0000 SYSG %-4.4s %c%c%c%c %s\n"

                ,lcss_num
                ,pDEVClass

                ,chOnlineStat
                ,chBusyStat
                ,chPendingStat
                ,chOpenStat

                ,szQueryDeviceBuff
            );
        else
#endif // defined(_FEATURE_INTEGRATED_3270_CONSOLE)
            gui_fprintf(fStatusStream,

                "DEV=%s%4.4X %4.4X %-4.4s %c%c%c%c %s\n"

                ,lcss_num
                ,pDEVBLK->devnum
                ,pDEVBLK->devtype
                ,pDEVClass

                ,chOnlineStat
                ,chBusyStat
                ,chPendingStat
                ,chOpenStat

                ,szQueryDeviceBuff
            );
    }

    // Since the device list can be in any order and devices can be added
    // and/or removed at any time, the GUI needs to know "That's all the
    // devices there are" so that it can detect when devices are removed...

    gui_fprintf(fStatusStream, "DEV=X\n");    // (indicates end of list)
}

///////////////////////////////////////////////////////////////////////////////
// Send device status msgs to the gui IF NEEDED...  (slightly more efficient)

void  NewUpdateDevStats ()
{
    DEVBLK*     pDEVBLK;
    GUISTAT*    pGUIStat;
    char*       pDEVClass;
    BYTE        chOnlineStat, chBusyStat, chPendingStat, chOpenStat;
    BOOL        bUpdatesSent = FALSE;
    static BOOL bFirstBatch  = TRUE;
    BYTE        lcss;
    char        lcss_num[4];

    if (sysblk.shutdown) return;

    // Process ALL the devices in the entire configuration each time...

    // (But only send device status messages to the GUI only when the
    // device's status actually changes and not continuously like before)

    for (pDEVBLK = sysblk.firstdev; pDEVBLK != NULL; pDEVBLK = pDEVBLK->nextdev)
    {
        pGUIStat = pDEVBLK->pGUIStat;

        if ((lcss = SSID_TO_LCSS( pDEVBLK->ssid )) >= 1)
            MSGBUF( lcss_num, "%1d:", lcss );
        else
            lcss_num[0] = 0;

        // Does this device exist in the configuration?

        if (!IS_DEV( pDEVBLK ))
        {
            // This device no longer exists in the configuration...
            // If we haven't yet notified the GUI about this device
            // being deleted from the configuration, then do so at
            // this time...

            if (*pGUIStat->pszNewStatStr)
            {
                // Send "device deleted" message...

                gui_fprintf ( fStatusStream, "DEVD=%s%4.4X\n", lcss_num, pDEVBLK->devnum );
                bUpdatesSent = TRUE;

                *pGUIStat->pszNewStatStr = 0;   // (prevent re-reporting it)
                *pGUIStat->pszOldStatStr = 0;   // (prevent re-reporting it)
            }

            continue;   // (go on to next device)
        }

        // Retrieve this device's filename and optional settings parameter values...

        szQueryDeviceBuff[MAX_DEVICEQUERY_LEN] = 0; // (buffer allows room for 1 extra)

        (pDEVBLK->hnd->query)(pDEVBLK, &pDEVClass, MAX_DEVICEQUERY_LEN, szQueryDeviceBuff);

        if (0 != szQueryDeviceBuff[MAX_DEVICEQUERY_LEN])    // (buffer overflow?)
        {
            WRMSG
            (
                HHC01540, "E"
                ,SSID_TO_LCSS(pDEVBLK->ssid)
                ,pDEVBLK->devnum
            );
        }

        szQueryDeviceBuff[MAX_DEVICEQUERY_LEN] = 0;   // (enforce NULL termination)

        // Device status flags...
                                                                              chOnlineStat  =
                                                                              chBusyStat    =
                                                                              chPendingStat =
                                                                              chOpenStat    = '0';

        if ((!pDEVBLK->console && pDEVBLK->fd >= 0) ||
            ( pDEVBLK->console && pDEVBLK->connected))                        chOnlineStat  = '1';
        if (pDEVBLK->busy)                                                    chBusyStat    = '1';
        if (IOPENDING(pDEVBLK))                                               chPendingStat = '1';
        if (pDEVBLK->fd > MAX(STDIN_FILENO,MAX(STDOUT_FILENO,STDERR_FILENO))) chOpenStat    = '1';

        // Build a new "device added" or "device changed"
        // status string for this device...

#if defined(_FEATURE_INTEGRATED_3270_CONSOLE)
        if (pDEVBLK == sysblk.sysgdev)
            snprintf( pGUIStat->pszNewStatStr, GUI_STATSTR_BUFSIZ,

                "DEV%c=%s0000 SYSG %-4.4s %c%c%c%c %s"

                ,*pGUIStat->pszOldStatStr ? 'C' : 'A'

                ,lcss_num
                ,pDEVClass

                ,chOnlineStat
                ,chBusyStat
                ,chPendingStat
                ,chOpenStat

                ,szQueryDeviceBuff
            );
        else
#endif // defined(_FEATURE_INTEGRATED_3270_CONSOLE)
            snprintf( pGUIStat->pszNewStatStr, GUI_STATSTR_BUFSIZ,

                "DEV%c=%s%4.4X %4.4X %-4.4s %c%c%c%c %s"

                ,*pGUIStat->pszOldStatStr ? 'C' : 'A'

                ,lcss_num
                ,pDEVBLK->devnum
                ,pDEVBLK->devtype
                ,pDEVClass

                ,chOnlineStat
                ,chBusyStat
                ,chPendingStat
                ,chOpenStat

                ,szQueryDeviceBuff
            );

        *(pGUIStat->pszNewStatStr + GUI_STATSTR_BUFSIZ - 1) = 0;

        // If the new status string is different from the old one,
        // then send the new one to the GUI and swap buffer ptrs
        // for next time. In this way we only send device status
        // msgs to the GUI only when the status actually changes...

        if (strcmp( pGUIStat->pszNewStatStr, pGUIStat->pszOldStatStr ))
        {
            gui_fprintf ( fStatusStream, "%s\n", pGUIStat->pszNewStatStr );
            bUpdatesSent = TRUE;
            {
                register char*
                          pszSavStatStr = pGUIStat->pszNewStatStr;
                pGUIStat->pszNewStatStr = pGUIStat->pszOldStatStr;
                pGUIStat->pszOldStatStr =           pszSavStatStr;
            }
        }
    }

    // Only send End-of-Batch indicator if we sent any updates or
    // if this is the first device-list update since powering on.

    if ( bUpdatesSent || bFirstBatch )
    {
        bFirstBatch = FALSE;
        gui_fprintf(fStatusStream, "DEVX=\n");  // (send end-of-batch indicator)
    }
}

///////////////////////////////////////////////////////////////////////////////
// Our Hercules "debug_cpu_state" override...
//
// Hercules calls the following function from several different places to fix
// an unintentional problem caused by the new logger mechanism (wherein stdout
// and stderr now point to the same stream) due to a oversight (bug) on my part
// wherein the 'LOAD' and 'MAN' messages are being [mistakenly] written to stdout
// instead of stderr (where they normally should be). The current version of
// the gui expects both messages to come in on the stdout stream, but due to the
// recent logger changes, they now come in on the stderr stream instead (because
// stdout was duped to stderr by the new logger logic) thus causing the gui to
// miss seeing them without the below fix. The below fix simply corrects for the
// problem by simply writing the two messages to the stdout stream where older
// versions of the gui expect to see them.

void*  gui_debug_cpu_state ( REGS* pREGS )
{
void *(*next_debug_call)(REGS *);

    static BOOL bLoading = FALSE;
    static BOOL bStopped = FALSE;

    if (sysblk.shutdown) return NULL;

    if (pTargetCPU_REGS && pREGS != pTargetCPU_REGS)
        return NULL;

    if (bLoading != (pREGS->loadstate ? TRUE : FALSE))
    {
        bLoading  = (pREGS->loadstate ? TRUE : FALSE);
        gui_fprintf(stdout,"LOAD=%c\n", bLoading ? '1' : '0');
    }

    if (bStopped != ((CPUSTATE_STOPPED == pREGS->cpustate) ? TRUE : FALSE))
    {
        bStopped  = ((CPUSTATE_STOPPED == pREGS->cpustate) ? TRUE : FALSE);
        gui_fprintf(stdout,"MAN=%c\n", bStopped ? '1' : '0');
    }

    if((next_debug_call = hdl_next( &gui_debug_cpu_state )))
        return next_debug_call( pREGS );

    return NULL;    // (I have no idea why this is a void* func)
}

///////////////////////////////////////////////////////////////////////////////
// Our Hercules "debug_cd_cmd" hook...
//
// The following function is called by the 'cd_cmd' panel command to notify
// the GUI of what the new current directory was just changed to...

void* gui_debug_cd_cmd( char* pszCWD )
{
    ASSERT( pszCWD );
    if (gui_version >= 1.12)
        gui_fprintf( fStatusStream, "]CWD=%s\n", pszCWD );
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Streams 'fprintf' function to prevent interleaving collision problem...

LOCK gui_fprintf_lock;

void gui_fprintf( FILE* stream, const char* pszFormat, ... )
{
    va_list vl;
    va_start( vl, pszFormat );
    obtain_lock ( &gui_fprintf_lock );
    vfprintf( stream, pszFormat, vl );
    fflush( stream );
    release_lock( &gui_fprintf_lock );
}

///////////////////////////////////////////////////////////////////////////////
// Acquire any resources we need in order to operate...
// (called by 'gui_panel_display' before main loop initiates...)

void  Initialize ()
{
    // Disable the "quiet" command...
    {
        char cmdbuf[128] = {0};
        MSGBUF( cmdbuf, "%s %s %s", "$zapcmd", "quiet", "NoCmd" );
        VERIFY( 0 == InternalHercCmd( cmdbuf ));
    }

    // Initialize streams...

    fOutputStream = OUTPUT_STREAM_FILE_PTR;
    fStatusStream = STATUS_STREAM_FILE_PTR;

    nInputStreamFileNum = fileno( INPUT_STREAM_FILE_PTR );

    // Allocate input stream buffer...

    if (!(pszInputBuff = (char *) malloc( nInputBuffSize )))
    {
        fprintf(stderr,
            "HHC90000D DBG: malloc() pszInputBuff failed: %s\n",
            strerror( errno ));
        exit(0);
    }

    memset(pszInputBuff, 0, nInputBuffSize);
    nInputLen = 0;

    // Allocate command processing buffer...

    if (!(pszCommandBuff = (char *) malloc( nCommandBuffSize )))
    {
        fprintf(stderr,
            "HHC90000D DBG: malloc() pszCommandBuff failed: %s\n",
            strerror( errno ));
        exit(0);
    }

    memset(pszCommandBuff, 0, nCommandBuffSize);
    nCommandLen = 0;

    // Initialize some variables...

    HandleForcedRefresh();
}

///////////////////////////////////////////////////////////////////////////////
// Release any resources we acquired in order to operate...
// (called by 'gui_panel_display' when main loop terminates...)

void Cleanup()
{
    if  (pszInputBuff)
    free(pszInputBuff);

    if  (pszCommandBuff)
    free(pszCommandBuff);
}

///////////////////////////////////////////////////////////////////////////////
// Hercules  "daemon_task"  -or-  "panel_display"  override...

void gui_panel_display ()
{
    SET_THREAD_NAME( "dyngui" );    // (for debugging)
    WRMSG( HHC01541,"I" );          // "Dyngui.dll initiated"
    Initialize();                   // (acquire resources)

    ProcessingLoop();               // PRIMARY PROCESSING LOOP... (does not
                                    // return until Hercules is terminated)

    Cleanup();                      // (relinquish resources)
    WRMSG( HHC01542,"I" );          // "Dyngui.dll terminated"
}

/*****************************************************************************\
                 Hercules Dynamic Loader control sections...
\*****************************************************************************/
/*
   Note that ALL of the below "sections" are actually just simple functions
   that Hercules's dynamic loader logic calls, thus allowing you to insert
   whatever 'C' code you may need directly into any of the below sections.
*/
///////////////////////////////////////////////////////////////////////////////
//                        HDL_DEPENDENCY_SECTION
// The following are the various Hercules structures whose layout this module
// depends on. The layout of the following structures (size and version) MUST
// match the layout that was used to build Hercules with. If the size/version
// of any of the following structures changes (and a new version of Hercules
// is built using the new layout), then THIS module must also be built with
// the new layout as well. The layout (size/version) of the structure as it
// was when Hercules was built MUST MATCH the layout as it was when THIS DLL
// was built)

/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */

#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev dyngui_LTX_hdl_ddev
#define hdl_depc dyngui_LTX_hdl_depc
#define hdl_reso dyngui_LTX_hdl_reso
#define hdl_init dyngui_LTX_hdl_init
#define hdl_fini dyngui_LTX_hdl_fini
#endif

HDL_DEPENDENCY_SECTION          // (define module dependencies)

    HDL_DEPENDENCY ( HERCULES );        // Hercules itself
    HDL_DEPENDENCY ( SYSBLK   );        // Master control block
    HDL_DEPENDENCY ( REGS     );        // CPU regs and such
    HDL_DEPENDENCY ( DEVBLK   );        // Device info block

END_DEPENDENCY_SECTION

///////////////////////////////////////////////////////////////////////////////
//                        HDL_REGISTER_SECTION
// The following section defines the entry points within Hercules that THIS
// module is overriding (replacing). That is to say, THIS module's functions
// will be called by Hercules instead of the normal Hercules function (if any).
// The functions defined below thus provide additional/different functionality
// above/beyond the functionality normally provided by Hercules. (Of course,
// yet OTHER dlls may have further overridden whatever overrides we register
// here, such as would likely be the case for panel command overrides).

HDL_REGISTER_SECTION            // ("Register" our entry-points)
{
    // Perform static module initialization...

    gui_nounload = 1;                       // (reject any unload attempt)
    initialize_lock( &gui_fprintf_lock );   // (initialize GUI fprintf LOCK)

    // Register all of our override entry-points...

    //             Hercules's       Our
    //             registered       overriding
    //             entry-point      entry-point
    //             name             value
    HDL_REGISTER ( panel_display,   gui_panel_display   );// (Yep! We override EITHER!)
    HDL_REGISTER ( daemon_task,     gui_panel_display   );// (Yep! We override EITHER!)
    HDL_REGISTER ( debug_cpu_state, gui_debug_cpu_state );
    HDL_REGISTER ( debug_cd_cmd,    gui_debug_cd_cmd    );
    HDL_REGISTER ( panel_command,   gui_panel_command   );
}
END_REGISTER_SECTION

#if defined( WIN32 ) && !defined( HDL_USE_LIBTOOL )
#if !defined( _MSVC_ )
  #undef sysblk
#endif
///////////////////////////////////////////////////////////////////////////////
//                        HDL_RESOLVER_SECTION
// The following section "resolves" entry-points that this module needs. The
// below HDL_RESOLVE entries define the names of Hercules's registered entry-
// points that we need "imported" to us (so that we may call those functions
// directly ourselves). The HDL_RESOLVE_SYMPTR entries set the named pointer
// variable value (i.e. the name of OUR pointer variable) to the registered
// entry-point value that was registered by Hercules or some other DLL.

HDL_RESOLVER_SECTION            // ("Resolve" needed entry-points)
{
    //            Registered
    //            entry-points
    //            that we call
    HDL_RESOLVE ( panel_command );

#if !defined( _MSVC_ )
    //                    Our pointer-     Registered entry-
    //                    variable name    point value name
    HDL_RESOLVE_SYMPTR (  psysblk,         sysblk            );
#endif
}
END_RESOLVER_SECTION
#endif

///////////////////////////////////////////////////////////////////////////////
//                        HDL_FINAL_SECTION
// The following section defines what should be done immediately before this
// module is unloaded. It is nothing more than a function that is called by
// Hercules just before your module is unloaded. You can do anything you want
// but the normal thing to do is release any resources that were acquired when
// your module was loaded (e.g. release memory that was malloc'ed, etc).

HDL_FINAL_SECTION
{
    rc = gui_nounload;              // (reject unloads once activated)

    if (sysblk.shutdown)
    {
        bDoneProcessing = TRUE;     // (tell main loop to stop processing)
        USLEEP(100000);             // (brief delay to give GUI time
                                    //  to display ALL shutdown msgs)
    }
}
END_FINAL_SECTION

///////////////////////////////////////////////////////////////////////////////
