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
#if defined(HDL_USE_LIBTOOL)
#include "ltdl.h"
#endif

#if !defined( _MSVC_ ) && !defined (__MINGW32__)
/*-------------------------------------------------------------------*/
/* For Unix-like platforms, the main() function:                     */
/* - sets the privilege level                                        */
/* - initializes the LIBTOOL environment                             */
/* - passes control to the impl() function in impl.c                 */
/*-------------------------------------------------------------------*/
int main( int ac, char* av[] )
{
    DROP_PRIVILEGES( CAP_SYS_NICE );
    SET_THREAD_NAME( BOOTSTRAP_NAME );

#if defined( HDL_USE_LIBTOOL )
    LTDL_SET_PRELOADED_SYMBOLS();
#endif
    exit( impl( ac, av ));
}

#else // defined( _MSVC_ )
/*-------------------------------------------------------------------*/
/* For Windows platforms, the main() function:                       */
/* - disables the standard CRT invalid parameter handler             */
/* - requests a minimum resolution for periodic timers               */
/* - sets up an exception trap                                       */
/* - passes control to the impl() function in impl.c                 */
/*                                                                   */
/* The purpose of the exception trap is to call a function which     */
/* will write a minidump file in the event of a Hercules crash.      */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*          ******************************************               */
/*          **       IMPORTANT!  PLEASE NOTE!       **               */
/*          ******************************************               */
/*                                                                   */
/*  Without a compatible version of DBGHELP.dll to use, the below    */
/*  exception handling logic accomplishes absolutely *NOTHING*.      */
/*                                                                   */
/*  The below exception handling requires DBGHELP.DLL version 6.1    */
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
/*  The DbgHelp.dll that ships in Windows is *NOT* redistributable.  */
/*  The "Debugging Tools for Windows" DbgHelp *IS* redistributable.  */
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
        if (!sysblk.daemon_mode && isatty( STDERR_FILENO )) // (normal panel mode?)
        {
            EnableMenuItem( GetSystemMenu( FindConsoleHandle(), FALSE ),
                            SC_CLOSE, MF_BYCOMMAND | MF_GRAYED );
        }

        if (1
            && (g_hDbgHelpDll = LoadLibrary(_T("DbgHelp.dll")))
            && (g_pfnMiniDumpWriteDumpFunc = (MINIDUMPWRITEDUMPFUNC*)
                GetProcAddress( g_hDbgHelpDll, _T("MiniDumpWriteDump")))
        )
        {
            GetModuleFileNameW( NULL, g_wszHercPath, _countof( g_wszHercPath ) );
            _wsplitpath( g_wszHercPath, g_wszHercDrive, g_wszHercDir, NULL, NULL );
        }

        SetUnhandledExceptionFilter( HerculesUnhandledExceptionFilter );
        SetErrorMode( SEM_NOGPFAULTERRORBOX );
    }

    rc = impl(ac,av);       // (Hercules, do your thing!)

    // Each call to "timeBeginPeriod" must be matched with a call to "timeEndPeriod"

    timeEndPeriod( 1 );     // (no longer care about accurate time intervals)

    if (!IsDebuggerPresent())
    {
        if (!sysblk.daemon_mode && isatty( STDERR_FILENO )) // (normal panel mode?)
        {
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

    if (sysblk.daemon_mode)                     // (is an external GUI in control?)
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

        mdt = (MINIDUMP_TYPE)
        (0
            | MiniDumpWithPrivateReadWriteMemory
            | MiniDumpWithDataSegs
            | MiniDumpWithHandleData
//          | MiniDumpWithFullMemoryInfo
//          | MiniDumpWithThreadInfo
            | MiniDumpWithUnloadedModules
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

#define MAX_MINIDUMP_USER_STREAMS  (64)

static  char                  g_host_info_str [ 1024 ];
static  MINIDUMP_USER_STREAM  UserStreamArray [ MAX_MINIDUMP_USER_STREAMS ];

static void BuildUserStreams( MINIDUMP_USER_STREAM_INFORMATION* pMDUSI )
{
    const char** ppszBldInfoStr;
    int nNumBldInfoStrs;
    ULONG UserStreamCount;

    _ASSERTE( pMDUSI );

    get_hostinfo_str( NULL, g_host_info_str, sizeof(g_host_info_str) );
    nNumBldInfoStrs = get_buildinfo_strings( &ppszBldInfoStr );

    UserStreamCount = min( (3+nNumBldInfoStrs), MAX_MINIDUMP_USER_STREAMS );

    pMDUSI->UserStreamCount = UserStreamCount;
    pMDUSI->UserStreamArray = UserStreamArray;

    UserStreamCount = 0;

    if ( UserStreamCount < pMDUSI->UserStreamCount )
    {
        UserStreamArray[UserStreamCount].Type       = CommentStreamA;
        UserStreamArray[UserStreamCount].Buffer     =        VERSION;
        UserStreamArray[UserStreamCount].BufferSize = sizeof(VERSION);
        UserStreamCount++;
    }

    if ( UserStreamCount < pMDUSI->UserStreamCount )
    {
        UserStreamArray[UserStreamCount].Type       = CommentStreamA;
        UserStreamArray[UserStreamCount].Buffer     =        HERCULES_COPYRIGHT;
        UserStreamArray[UserStreamCount].BufferSize = sizeof(HERCULES_COPYRIGHT);
        UserStreamCount++;
    }

    if ( UserStreamCount < pMDUSI->UserStreamCount )
    {
        UserStreamArray[UserStreamCount].Type       = CommentStreamA;
        UserStreamArray[UserStreamCount].Buffer     =        g_host_info_str;
        UserStreamArray[UserStreamCount].BufferSize = (ULONG)strlen(g_host_info_str)+1;
        UserStreamCount++;
    }

    for (; nNumBldInfoStrs && UserStreamCount < pMDUSI->UserStreamCount;
        nNumBldInfoStrs--, UserStreamCount++, ppszBldInfoStr++ )
    {
        UserStreamArray[UserStreamCount].Type       = CommentStreamA;
        UserStreamArray[UserStreamCount].Buffer     = (PVOID)*ppszBldInfoStr;
        UserStreamArray[UserStreamCount].BufferSize = (ULONG)strlen(*ppszBldInfoStr)+1;
    }
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
    BOOL bRet = FALSE;

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

/* NOTE About MemoryCallback :
 * This is defined for DbgHelp > 6.1..
 * Since "false" is returned, it has been commented out.
 *
 * Additionally, false is now returned by default. This
 * ensures that the callback function will operate correctly
 * even with future versions of the DbhHelp DLL.
 * -- Ivan
 */
//        case MemoryCallback:
//        {
//            // We do not include any information here -> return FALSE
//            bRet = FALSE;
//        }
//        break;

// Following default block added by ISW 2005/05/06
          default:
            {
                // Do not return any information for unrecognized
                // callback types.
                bRet=FALSE;
            }
            break;

//      case CancelCallback:
//          break;
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
