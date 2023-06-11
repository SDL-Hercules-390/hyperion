/* BOOTSTRAP.C  (C) Copyright Ivan Warren, 2003-2012                 */
/*              (C) Copyright "Fish" (David B. Trout), 2005-2012     */
/*              Hercules executable main module                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module is the initial entry point of the Hercules emulator.  */
/* The main() function performs platform-specific functions before   */
/* calling the impl function which launches the emulator.            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "cckd.h"
#include "cckddasd.h"

#if !defined( _MSVC_ )

struct termios stdin_attrs    = {0};    // (saved stdin  term settings)
struct termios stdout_attrs   = {0};    // (saved stdout term settings)
struct termios stderr_attrs   = {0};    // (saved stderr term settings)

static void save_term_attrs()
{
    tcgetattr( STDIN_FILENO,  &stdin_attrs  );
    tcgetattr( STDOUT_FILENO, &stdout_attrs );
    tcgetattr( STDERR_FILENO, &stderr_attrs );
}

static void restore_term_attrs()
{
    tcsetattr( STDIN_FILENO,  TCSANOW, &stdin_attrs  );
    tcsetattr( STDOUT_FILENO, TCSANOW, &stdout_attrs );
    tcsetattr( STDERR_FILENO, TCSANOW, &stderr_attrs );
}

#if defined( HAVE_SIGNAL_HANDLING )

int crash_signo = -1;                   // Saved signal number of crash

struct sigaction  sa_CRASH    = {0};    // (for catching exceptions)
struct sigaction  sa_SIGFPE   = {0};    // (original SIGFPE  action)
struct sigaction  sa_SIGILL   = {0};    // (original SIGILL  action)
struct sigaction  sa_SIGSEGV  = {0};    // (original SIGSEGV action)
#if defined( HAVE_DECL_SIGBUS ) && HAVE_DECL_SIGBUS
struct sigaction  sa_SIGBUS   = {0};    // (original SIGBUS  action)
#endif

static void crash_signal_handler( int signo );

static void install_crash_handler()
{
    /* Install our crash signal handler */
    sa_CRASH.sa_handler = &crash_signal_handler;
    sigaction( SIGFPE,  &sa_CRASH, &sa_SIGFPE  );
    sigaction( SIGILL,  &sa_CRASH, &sa_SIGILL  );
    sigaction( SIGSEGV, &sa_CRASH, &sa_SIGSEGV );
#if defined( HAVE_DECL_SIGBUS ) && HAVE_DECL_SIGBUS
    sigaction( SIGBUS,  &sa_CRASH, &sa_SIGBUS  );
#endif
}

static void uninstall_crash_handler()
{
    /* Restore original signal handlers */
    sigaction( SIGFPE,  &sa_SIGFPE,  0 );
    sigaction( SIGILL,  &sa_SIGILL,  0 );
    sigaction( SIGSEGV, &sa_SIGSEGV, 0 );
#if defined( HAVE_DECL_SIGBUS ) && HAVE_DECL_SIGBUS
    sigaction( SIGBUS,  &sa_SIGBUS,  0 );
#endif
}

static void log_crashed_msg( FILE* stream )
{
    fprintf
    (
        stream,

        "\n"
        "\n"
        "\n"
        "    +++ OOPS! +++\n"
        "\n"
        "Hercules has crashed! (%s)\n"
        "\n"

        , strsignal( crash_signo )
    );
    if (sysblk.ulimit_unlimited)
        fprintf
        (
            stream,
            "Creating crash dump... This will take a while...\n"
            "\n"
        );
    else
        fprintf
        (
            stream,
            "Crash dump NOT created due to ulimit setting...\n"
            "\n"
        );
    fflush( stream );
}

static void crash_signal_handler( int signo )
{
    crash_signo = signo;
    uninstall_crash_handler();
    restore_term_attrs();
    log_crashed_msg( stderr );
    log_crashed_msg( stdout );
}
#endif /* defined( HAVE_SIGNAL_HANDLING ) */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*  For Unix-like platforms, the main() function:                    */
/*                                                                   */
/*   - Installs a crash handler if signal handling is available      */
/*   - Sets the privilege level                                      */
/*   - Initializes the LIBTOOL environment                           */
/*   - Passes control to the impl() function in impl.c               */
/*                                                                   */
/*-------------------------------------------------------------------*/
int main( int ac, char* av[] )
{
    int rc = 0;
    save_term_attrs();
    DROP_PRIVILEGES( CAP_SYS_NICE );
    SET_THREAD_NAME( BOOTSTRAP_NAME );
#if defined( HDL_USE_LIBTOOL )
    LTDL_SET_PRELOADED_SYMBOLS();
#endif
#if defined( HAVE_SIGNAL_HANDLING )
    install_crash_handler();
#endif
    rc = impl( ac, av );
    restore_term_attrs();
    return rc;
}

#else // defined( _MSVC_ )
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  For Windows platforms, the main() function:                      */
/*                                                                   */
/*   - Disables the standard CRT invalid parameter handler           */
/*   - Requests a minimum resolution for periodic timers             */
/*   - Sets up an exception trap                                     */
/*   - Passes control to the impl() function in impl.c               */
/*                                                                   */
/*  The purpose of the exception trap is to call a function which    */
/*  will create a minidump in the event of a Hercules crash (which   */
/*  can then be analyzed offline to determine where Herc crashed).   */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*                                                                   */
/*          **--------------------------------------**               */
/*          **       IMPORTANT!  PLEASE NOTE!       **               */
/*          **--------------------------------------**               */
/*                                                                   */
/*                                                                   */
/*  Without a compatible version of DbgHelp.dll to use, the below    */
/*  exception handling logic accomplishes absolutely *NOTHING*!      */
/*                                                                   */
/*  The below exception handling requires DbgHelp.dll version 6.1    */
/*  or greater. Windows XP is known to ship a version of DbgHelp     */
/*  that is too old (version 5.1). Hercules *SHOULD* be shipping     */
/*  DbgHelp.DLL as part of its distribution. The latest version      */
/*  of the DbgHelp redistributable DLL can be downloaded as part     */
/*  of Microsoft's "Debugging Tools for Windows" package at the      */
/*  following URLs:                                                  */
/*                                                                   */
/*  http://msdn.microsoft.com/en-us/library/ms679294(VS.85).aspx     */
/*  http://go.microsoft.com/FWLink/?LinkId=84137                     */
/*  http://www.microsoft.com/whdc/devtools/debugging/default.mspx    */
/*                                                                   */
/*                                                                   */
/*                        ** NOTE **                                 */
/*                                                                   */
/*  The DbgHelp.dll that ships in Windows is *NOT* redistributable!  */
/*  The "Debugging Tools for Windows" DbgHelp *IS* redistributable!  */
/*                                                                   */
/*-------------------------------------------------------------------*/

#pragma optimize( "", off )   // (this code should NOT be optimized!)

typedef BOOL (MINIDUMPWRITEDUMPFUNC)
(
    HANDLE                             hProcess,
    DWORD                              ProcessId,
    HANDLE                             hDumpFile,
    MINIDUMP_TYPE                      DumpType,
    PMINIDUMP_EXCEPTION_INFORMATION    ExceptionParam,
    PMINIDUMP_USER_STREAM_INFORMATION  UserStreamParam,
    PMINIDUMP_CALLBACK_INFORMATION     CallbackParam
);

static MINIDUMPWRITEDUMPFUNC*  g_pfnMiniDumpWriteDumpFunc  = NULL;
static HMODULE                 g_hDbgHelpDll               = NULL;

///////////////////////////////////////////////////////////////////////////////
// Global string buffers to prevent C4748 warning: "/GS can not protect
// parameters and local variables from local buffer overrun because
// optimizations are disabled in function"

static WCHAR  g_wszHercDrive [ 4 * _MAX_DRIVE ]  = {0};
static WCHAR  g_wszHercDir   [ 4 * _MAX_DIR   ]  = {0};
static WCHAR  g_wszFileDir   [ 4 * _MAX_DIR   ]  = {0};
static WCHAR  g_wszHercPath  [ 4 * _MAX_PATH  ]  = {0};
static WCHAR  g_wszDumpPath  [ 4 * _MAX_PATH  ]  = {0};
static WCHAR  g_wszFileName  [ 4 * _MAX_FNAME ]  = {0};

static TCHAR    g_szSaveTitle[ 512 ] = {0};
static LPCTSTR  g_pszTempTitle = _T("{98C1C303-2A9E-11d4-9FF5-0060677l8D04}");

static int g_itracen = 0;   // (saved cckdblk.itracen value)

///////////////////////////////////////////////////////////////////////////////
// (forward references)

static LONG WINAPI HerculesUnhandledExceptionFilter( EXCEPTION_POINTERS* pExceptionPtrs );
static void ProcessException( EXCEPTION_POINTERS* pExceptionPtrs );
static BOOL CreateMiniDump( EXCEPTION_POINTERS* pExceptionPtrs );
static void BuildUserStreams( MINIDUMP_USER_STREAM_INFORMATION* pMDUSI );
static BOOL IsDataSectionNeeded( const WCHAR* pwszModuleName );
static BOOL CALLBACK MyMiniDumpCallback
(
    PVOID                            pParam,
    const PMINIDUMP_CALLBACK_INPUT   pInput,
    PMINIDUMP_CALLBACK_OUTPUT        pOutput
);
static HWND FindConsoleHandle();

///////////////////////////////////////////////////////////////////////////////
// MessageBoxTimeout API support...

#include <winuser.h>                // (need WINUSERAPI)
#pragma comment(lib,"user32.lib")   // (need user32.dll)

#ifndef MB_TIMEDOUT
#define MB_TIMEDOUT   32000         // (timed out return code)
#endif

#if defined( _UNICODE ) || defined( UNICODE )
  int  WINAPI  MessageBoxTimeoutW ( IN HWND hWnd,  IN LPCWSTR lpText,      IN LPCWSTR lpCaption,
                                    IN UINT uType, IN WORD    wLanguageId, IN DWORD   dwMilliseconds );
  #define MessageBoxTimeout MessageBoxTimeoutW
#else // not unicode
  int  WINAPI  MessageBoxTimeoutA ( IN HWND hWnd,  IN LPCSTR  lpText,      IN LPCSTR  lpCaption,
                                    IN UINT uType, IN WORD    wLanguageId, IN DWORD   dwMilliseconds );
  #define MessageBoxTimeout MessageBoxTimeoutA
#endif // unicode or not

///////////////////////////////////////////////////////////////////////////////

#include <Mmsystem.h>               // (timeBeginPeriod, timeEndPeriod)
#pragma comment( lib, "Winmm" )     // (timeBeginPeriod, timeEndPeriod)

int main( int ac, char* av[] )
{
    int rc = 0;

    SET_THREAD_NAME( BOOTSTRAP_NAME );

    // Disable default invalid crt parameter handling

    DISABLE_CRT_INVALID_PARAMETER_HANDLER();

    // Request the highest possible time-interval accuracy...

    timeBeginPeriod( 1 );   // (one millisecond time interval accuracy)

    // If we're being debugged, then let the debugger
    // catch the exception. Otherwise, let our exception
    // handler catch it...

    if (!IsDebuggerPresent())
    {
        // Normal panel mode?

        if (/*!sysblk.daemon_mode &&*/ isatty( STDERR_FILENO ))
        {
            // Then disable the [x] Close button for safety

            EnableMenuItem( GetSystemMenu( FindConsoleHandle(), FALSE ),
                            SC_CLOSE, MF_BYCOMMAND | MF_GRAYED );
        }

        // Load the debugging helper library...

        if (1
            && (g_hDbgHelpDll = LoadLibrary(_T("DbgHelp.dll")))
            && (g_pfnMiniDumpWriteDumpFunc = (MINIDUMPWRITEDUMPFUNC*)
                GetProcAddress( g_hDbgHelpDll, _T("MiniDumpWriteDump")))
        )
        {
            GetModuleFileNameW( NULL, g_wszHercPath, _countof( g_wszHercPath ) );
            _wsplitpath( g_wszHercPath, g_wszHercDrive, g_wszHercDir, NULL, NULL );
        }

        // Enable exception handling...

        SetUnhandledExceptionFilter( HerculesUnhandledExceptionFilter );
        SetErrorMode( SEM_NOGPFAULTERRORBOX );
    }

    rc = impl(ac,av);       // (Hercules, do your thing!)

    // Each call to "timeBeginPeriod" must be matched with a call to "timeEndPeriod"

    timeEndPeriod( 1 );     // (no longer care about accurate time intervals)

    if (!IsDebuggerPresent())
    {
        // Normal panel mode?

        if (!sysblk.daemon_mode && isatty( STDERR_FILENO ))
        {
            // Re-enable the [x] Close button

            EnableMenuItem( GetSystemMenu( FindConsoleHandle(), FALSE ),
                        SC_CLOSE, MF_BYCOMMAND | MF_ENABLED );
        }
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
// Hercules unhandled exception filter...

#include <Wincon.h>                             // (need SetConsoleMode, etc)

#pragma optimize( "", off )

static LONG WINAPI HerculesUnhandledExceptionFilter( EXCEPTION_POINTERS* pExceptionPtrs )
{
    static BOOL bDidThis = FALSE;               // (if we did this once already)
    if (bDidThis)
        return EXCEPTION_EXECUTE_HANDLER;       // (quick exit to prevent loop)
    bDidThis = TRUE;
    SetErrorMode( 0 );                          // (reset back to default handling)

    // Stop CCKD tracing right away to preserve existing trace entries

    g_itracen = cckdblk.itracen;                // (save existing value)
    cckdblk.itracen = 0;                        // (set table size to 0)

    // Is an external GUI in control?

    if (sysblk.daemon_mode)
    {
        fflush( stdout );
        fflush( stderr );

        _ftprintf( stderr, _T("]!OOPS!\n") );   // (external GUI pre-processing...)

        fflush( stdout );
        fflush( stderr );
        Sleep( 10 );
    }
    else
    {
        // Normal panel mode: reset console mode and clear the screen...

        DWORD                       dwCellsWritten;
        CONSOLE_SCREEN_BUFFER_INFO  csbi;
        HANDLE                      hStdIn, hStdErr;
        COORD                       ptConsole = { 0, 0 };

        EnableMenuItem( GetSystemMenu( FindConsoleHandle(), FALSE ),
                    SC_CLOSE, MF_BYCOMMAND | MF_ENABLED );

        hStdIn  = GetStdHandle( STD_INPUT_HANDLE );
        hStdErr = GetStdHandle( STD_ERROR_HANDLE );

#define DEFAULT_CONSOLE_ATTRIBUTES     (0   \
        | FOREGROUND_RED                    \
        | FOREGROUND_GREEN                  \
        | FOREGROUND_BLUE                   \
        )

/* FIXME these are defined in SDK V6+ */
#ifndef ENABLE_INSERT_MODE
#define ENABLE_INSERT_MODE 0
#endif
#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0
#endif

#define DEFAULT_CONSOLE_INPUT_MODE     (0   \
        | ENABLE_ECHO_INPUT                 \
        | ENABLE_INSERT_MODE                \
        | ENABLE_LINE_INPUT                 \
        | ENABLE_MOUSE_INPUT                \
        | ENABLE_PROCESSED_INPUT            \
        | ENABLE_QUICK_EDIT_MODE            \
        )

#define DEFAULT_CONSOLE_OUTPUT_MODE    (0   \
        | ENABLE_PROCESSED_OUTPUT           \
        | ENABLE_WRAP_AT_EOL_OUTPUT         \
        )

        SetConsoleTextAttribute( hStdErr, DEFAULT_CONSOLE_ATTRIBUTES  );
        SetConsoleMode         ( hStdIn,  DEFAULT_CONSOLE_INPUT_MODE  );
        SetConsoleMode         ( hStdErr, DEFAULT_CONSOLE_OUTPUT_MODE );

        GetConsoleScreenBufferInfo( hStdErr, &csbi );
        FillConsoleOutputCharacter( hStdErr, ' ', csbi.dwSize.X * csbi.dwSize.Y, ptConsole, &dwCellsWritten );
        GetConsoleScreenBufferInfo( hStdErr, &csbi );
        FillConsoleOutputAttribute( hStdErr, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, ptConsole, &dwCellsWritten );
        SetConsoleCursorPosition  ( hStdErr, ptConsole );

        fflush( stdout );
        fflush( stderr );
        Sleep( 10 );
    }

    _tprintf( _T("\n\n") );
    _tprintf( _T("                      ***************\n") );
    _tprintf( _T("                      *    OOPS!    *\n") );
    _tprintf( _T("                      ***************\n") );
    _tprintf( _T("\n") );
    _tprintf( _T("                    Hercules has crashed!\n") );
    _tprintf( _T("\n") );
    _tprintf( _T("(you may or may not need to press ENTER if no 'oops!' dialog-box appears)\n") );
    _tprintf( _T("\n") );

    fflush( stdout );
    fflush( stderr );
    Sleep( 10 );

    ProcessException( pExceptionPtrs );     // (create a minidump, if possible)

    fflush( stdout );
    fflush( stderr );
    Sleep( 10 );

    timeEndPeriod( 1 );                     // (reset to default time interval)

    return EXCEPTION_EXECUTE_HANDLER;       // (quite likely exits the process)
}

///////////////////////////////////////////////////////////////////////////////

static HWND FindConsoleHandle()
{
    HWND hWnd;
    if (!GetConsoleTitle( g_szSaveTitle, _countof( g_szSaveTitle )))
        return NULL;
    if (!SetConsoleTitle( g_pszTempTitle ))
        return NULL;
    Sleep(20);
    hWnd = FindWindow( NULL, g_pszTempTitle );
    SetConsoleTitle( g_szSaveTitle );
    return hWnd;
}

///////////////////////////////////////////////////////////////////////////////

#define  USE_MSGBOX_TIMEOUT     // (to force default = yes take a minidump)

#ifdef USE_MSGBOX_TIMEOUT
  #define  MESSAGEBOX   MessageBoxTimeout
#else
  #define  MESSAGEBOX   MessageBox
#endif

///////////////////////////////////////////////////////////////////////////////

static inline int MsgBox( HWND hWnd, LPCTSTR pszText, LPCTSTR pszCaption,
                          UINT uType, WORD wLanguageId, DWORD dwMilliseconds )
{
    return MESSAGEBOX( hWnd, pszText, pszCaption, uType
#ifdef USE_MSGBOX_TIMEOUT
        , wLanguageId
        , dwMilliseconds
#endif
    );
}

///////////////////////////////////////////////////////////////////////////////
// Create a minidump

static void ProcessException( EXCEPTION_POINTERS* pExceptionPtrs )
{
    int rc;
    UINT uiMBFlags =
        0
        | MB_SYSTEMMODAL
        | MB_TOPMOST
        | MB_SETFOREGROUND
        ;

    HWND hwndMBOwner = FindConsoleHandle();

    if (!hwndMBOwner || !IsWindowVisible(hwndMBOwner))
        hwndMBOwner = GetDesktopWindow();

    if ( !g_pfnMiniDumpWriteDumpFunc )
    {
        MsgBox
        (
            hwndMBOwner,
            _T("The creation of a crash dump for analysis by the Hercules ")
            _T("development team is NOT possible\nbecause the required 'DbgHelp.dll' ")
            _T("is missing or is not installed or was otherwise not located.")
            ,_T("OOPS!  Hercules has crashed!"),
            uiMBFlags
                | MB_ICONERROR
                | MB_OK
            ,0                  // (default/neutral language)
            ,(15 * 1000)        // (timeout == 15 seconds)
        );

        return;
    }

    if ( IDYES == ( rc = MsgBox
    (
        hwndMBOwner,
        _T("The creation of a crash dump for further analysis by ")
        _T("the Hercules development team is strongly suggested.\n\n")
        _T("Would you like to create a crash dump for ")
        _T("the Hercules development team to analyze?")
        ,_T("OOPS!  Hercules has crashed!"),
        uiMBFlags
            | MB_ICONERROR
            | MB_YESNO
        ,0                      // (default/neutral language)
        ,(10 * 1000)            // (timeout == 10 seconds)
    ))
        || MB_TIMEDOUT == rc
    )
    {
        if ( CreateMiniDump( pExceptionPtrs ) && MB_TIMEDOUT != rc )
        {
            MsgBox
            (
                hwndMBOwner,
                _T("Please send the dump to the Hercules development team for analysis.")
                ,_T("Dump Complete"),
                uiMBFlags
                    | MB_ICONEXCLAMATION
                    | MB_OK
                ,0              // (default/neutral language)
                ,(5 * 1000)     // (timeout == 5 seconds)
            );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// The following CreateMiniDump functions are based on
// Oleg Starodumov's sample at http://www.debuginfo.com

static BOOL CreateMiniDump( EXCEPTION_POINTERS* pExceptionPtrs )
{
    BOOL bSuccess = FALSE;
    HANDLE hDumpFile;

    _wmakepath( g_wszDumpPath, g_wszHercDrive, g_wszHercDir, L"Hercules", L".dmp" );

    _tprintf( _T("Creating crash dump \"%ls\"...\n\n"), g_wszDumpPath );
    _tprintf( _T("Please wait; this may take a few minutes...\n\n") );
    _tprintf( _T("(another message will appear when the dump is complete)\n\n") );

    hDumpFile = CreateFileW
    (
        g_wszDumpPath,
        GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    if ( hDumpFile && INVALID_HANDLE_VALUE != hDumpFile )
    {
        // Create the minidump

        MINIDUMP_EXCEPTION_INFORMATION    mdei;
        MINIDUMP_USER_STREAM_INFORMATION  mdusi;
        MINIDUMP_CALLBACK_INFORMATION     mci;
        MINIDUMP_TYPE                     mdt;

        BuildUserStreams( &mdusi );

        mdei.ThreadId           = GetCurrentThreadId();
        mdei.ExceptionPointers  = pExceptionPtrs;
        mdei.ClientPointers     = FALSE;

        mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE) MyMiniDumpCallback;
        mci.CallbackParam       = 0;

        // Ref: "EFFECTIVE MINIDUMPS"
        //      http://www.debuginfo.com/articles/effminidumps.html
        //      http://www.debuginfo.com/articles/effminidumps2.html

        mdt = (MINIDUMP_TYPE)
        (0
            | MiniDumpWithPrivateReadWriteMemory        // Needed to also dump heap
            | MiniDumpWithIndirectlyReferencedMemory    // Dump portions of the heap
                                                        // only if referenced in stack
            | MiniDumpWithDataSegs                      // Include global variables
            | MiniDumpWithHandleData                    // Dump all file handles too
            | MiniDumpWithUnloadedModules               // Tell us about unloaded DLLs
        );

        bSuccess = g_pfnMiniDumpWriteDumpFunc( GetCurrentProcess(), GetCurrentProcessId(),
            hDumpFile, mdt, (pExceptionPtrs != 0) ? &mdei : 0, &mdusi, &mci );

        CloseHandle( hDumpFile );

        if ( bSuccess )
        {
            _tprintf( _T("\nDump \"%ls\" created.\n\n"
                         "Please forward the dump to the Hercules team for analysis.\n\n"),
                         g_wszDumpPath );
        }
        else
            _tprintf( _T("\nMiniDumpWriteDump failed! Error: %u\n\n"), GetLastError() );
    }
    else
    {
        _tprintf( _T("\nCreateFile failed! Error: %u\n\n"), GetLastError() );
    }

    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// Build User Stream Arrays...

#define MAX_MINIDUMP_USER_STREAMS  (1024)   // (just an arbitrary value)
#define NUM_CCKD_TRACE_STRINGS       (64)   // (EACH STRING is one stream)
#define NUM_LOGFILE_MESSAGES       (1024)   // (counts as ONE big stream)
#define MAX_STREAM_BUFFERSIZE       (256)   // (max CommentStreamA length)

static  MINIDUMP_USER_STREAM  UserStreamArray [ MAX_MINIDUMP_USER_STREAMS ];

static ULONG StreamBufferSize( size_t len  )
{
    return (ULONG) (len < MAX_STREAM_BUFFERSIZE ? len : MAX_STREAM_BUFFERSIZE);
}

#define BUILD_SYSBLK_USER_STREAM( sysblk_xxx )                                 \
                                                                               \
    for (pstr = sysblk_xxx;                                                    \
        *pstr && StreamNum < MAX_MINIDUMP_USER_STREAMS; ++pstr, ++StreamNum)   \
    {                                                                          \
        UserStreamArray[ StreamNum ].Type       = CommentStreamA;              \
        UserStreamArray[ StreamNum ].Buffer     = (PVOID)         *pstr;       \
        UserStreamArray[ StreamNum ].BufferSize = StreamBufferSize( strlen( *pstr ) + 1 ); \
    }

static void BuildUserStreams( MINIDUMP_USER_STREAM_INFORMATION* pMDUSI )
{
    const char** pstr;
    ULONG StreamNum = 0;

    _ASSERTE( pMDUSI );

    pMDUSI->UserStreamArray = UserStreamArray;

    // Save version information

    BUILD_SYSBLK_USER_STREAM( sysblk.vers_info   );
    BUILD_SYSBLK_USER_STREAM( sysblk.bld_opts    );
    BUILD_SYSBLK_USER_STREAM( sysblk.extpkg_vers );

    // Save whether Hercules is running in "elevated" mode or not

    if (StreamNum < MAX_MINIDUMP_USER_STREAMS)
    {
        static char   buf[64]    = {0};
        const  char*  severity   = NULL;
        const  char*  is_or_not  = NULL;
        bool          evelated   = false;

        evelated  = are_elevated();
        severity  = evelated ? "I" : "W";
        is_or_not = evelated ? ""  : "NOT ";

        // "Hercules is %srunning in elevated mode"
        MSGBUF( buf, MSG_C( HHC00018, severity, is_or_not ));

        UserStreamArray[ StreamNum ].Type       = CommentStreamA;
        UserStreamArray[ StreamNum ].Buffer     = (PVOID)                      buf;
        UserStreamArray[ StreamNum ].BufferSize = StreamBufferSize( strlen( (const char*) buf ) + 1 );
        StreamNum++;
    }

    // Save last few entries of CCKD internal trace table

    if (1
        && StreamNum < MAX_MINIDUMP_USER_STREAMS
        && g_itracen && cckdblk.itrace    // (do we have a table?)
    )
    {
        // itrace       ptr to beginning of table (first entry)
        // itracep      ptr to "current" entry" (ptr to the entry that
        //              should be used next, i.e. the "oldest" entry)
        // itracex      ptr to end of table (i.e. points to the entry
        //              just PAST the very last entry in the table)
        // itracen      the number of entries there are in the table
        // itracec      how many of those entries are actively in use

        int i, k = min( NUM_CCKD_TRACE_STRINGS, cckdblk.itracec );
        CCKD_ITRACE* p = cckdblk.itracep;

        // Backup 'k' entries...   (note pointer arithmetic)

        for (i=0; i < k; ++i)
            if (--p < cckdblk.itrace)
                p = cckdblk.itracex - 1;

        // Dump 'k' entries starting from there...

        for (i=0; i < k && StreamNum < MAX_MINIDUMP_USER_STREAMS; ++i, ++p, ++StreamNum)
        {
            if (p >= cckdblk.itracex)
                p = cckdblk.itrace;

            UserStreamArray[ StreamNum ].Type       = CommentStreamA;
            UserStreamArray[ StreamNum ].Buffer     = (PVOID)                      p;
            UserStreamArray[ StreamNum ].BufferSize = StreamBufferSize( strlen( (const char*) p ) + 1 );
        }
    }

    // Save last few log messages

    if (StreamNum < MAX_MINIDUMP_USER_STREAMS)
    {
        char* logbuf_ptr;
        int   logbuf_idx;
        int   logbuf_bytes;

        logbuf_idx = log_line( NUM_LOGFILE_MESSAGES );

        if ((logbuf_bytes = log_read( &logbuf_ptr, &logbuf_idx, LOG_NOBLOCK )) > 0)
        {
            UserStreamArray[ StreamNum ].Type       = CommentStreamA;
            UserStreamArray[ StreamNum ].Buffer     = (PVOID) logbuf_ptr;
            UserStreamArray[ StreamNum ].BufferSize = StreamBufferSize( logbuf_bytes - 1 );
            StreamNum++;
        }
    }

    pMDUSI->UserStreamCount = StreamNum;
}

///////////////////////////////////////////////////////////////////////////////
// Custom minidump callback

static BOOL CALLBACK MyMiniDumpCallback
(
    PVOID                            pParam,
    const PMINIDUMP_CALLBACK_INPUT   pInput,
    PMINIDUMP_CALLBACK_OUTPUT        pOutput
)
{
    BOOL bRet = FALSE;      // (default unless we decide otherwise)

    if ( !pInput || !pOutput )
        return FALSE;

    switch ( pInput->CallbackType )
    {
        case IncludeModuleCallback:
        {
            // Include the module into the dump
            bRet = TRUE;
        }
        break;

        case IncludeThreadCallback:
        {
            // Include the thread into the dump
            bRet = TRUE;
        }
        break;

        case ModuleCallback:
        {
            // Are data sections available for this module ?

            if ( pOutput->ModuleWriteFlags & ModuleWriteDataSeg )
            {
                // Yes, but do we really need them?

                if ( !IsDataSectionNeeded( pInput->Module.FullPath ) )
                    pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
            }

            bRet = TRUE;
        }
        break;

        case ThreadCallback:
        {
            // Include all thread information into the minidump
            bRet = TRUE;
        }
        break;

        case ThreadExCallback:
        {
            // Include this information
            bRet = TRUE;
        }
        break;

        //-------------------------------------------------------------
        //
        //                  PROGRAMMING NOTE
        //
        // MemoryCallback is defined only for DbgHelp > 6.1.  Since we
        // were returning FALSE for this callback anyway (which is our
        // default), it was commented out altogether.
        //
        // This ensures our callback function will operate correctly
        // even with future versions of DbgHelp DLL.
        //
        // -- Ivan
        //
        //-------------------------------------------------------------

//      case MemoryCallback:
//      {
//          // We do not include any information here -> return FALSE
//          bRet = FALSE;
//      }
//      break;


        // Following default block added by ISW 2005/05/06

        default:
        {
            // Do not return any information for
            // any unrecognized callback types.

            bRet = FALSE;
        }
        break;
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// This function determines whether we need data sections of the given module

static BOOL IsDataSectionNeeded( const WCHAR* pwszModuleName )
{
    BOOL bNeeded = FALSE;

    _ASSERTE( pwszModuleName );

    _wsplitpath( pwszModuleName, NULL, g_wszFileDir, g_wszFileName, NULL );

    if ( _wcsicmp( g_wszFileName, L"ntdll" ) == 0 )
    {
        bNeeded = TRUE;
    }
    else if ( _wcsicmp( g_wszFileDir, g_wszHercDir ) == 0 )
    {
        bNeeded = TRUE;
    }

    return bNeeded;
}

///////////////////////////////////////////////////////////////////////////////

#pragma optimize( "", on )      // (we're done; re-enable optimizations)

#endif // !defined( _MSVC_ )
