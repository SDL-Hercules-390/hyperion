/* W32CTCA.C    (C) Copyright "Fish" (David B. Trout), 2002-2014     */
/*    CTCI-WIN (Channel to Channel link to Win32 TCP/IP stack)       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"
#include "hercules.h"

#if !defined(OPTION_W32_CTCI)
int w32ctca_dummy = 0;
#else // defined(OPTION_W32_CTCI)

#include "w32ctca.h"
#include "tt32api.h"        // (exported TunTap32.dll functions)
#ifdef __CYGWIN__
  #include <sys/cygwin.h>   // (for cygwin_conv_to_full_win32_path)
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// We prefer the '_ex' variety as they resolve the "no error" errno issue...
// But if they're not available we'll settle for the older version...

#define TT32_PROCADDRS( name )                                  \
                                                                \
ptuntap32_ ## name ## _ex   g_tt32_pfn_ ## name ## _ex = NULL;  \
ptuntap32_ ## name          g_tt32_pfn_ ## name        = NULL

#define _GET_TT32_PROCADDRS( name )                               \
                                                                  \
    g_tt32_pfn_ ## name ## _ex  =                                 \
    (ptuntap32_ ## name ## _ex  ) GetProcAddress( g_tt32_hmoddll, \
     "tuntap32_" # name   "_ex" );                                \
    g_tt32_pfn_ ## name         =                                 \
    (ptuntap32_ ## name         ) GetProcAddress( g_tt32_hmoddll, \
     "tuntap32_" # name         )

#define GET_OPTIONAL_TT32_PROCADDR( name )                        \
                                                                  \
    _GET_TT32_PROCADDRS( name )

#define GET_REQUIRED_TT32_PROCADDR( name )                        \
                                                                  \
    _GET_TT32_PROCADDRS( name );                                  \
    if ( !g_tt32_pfn_ ## name ) goto error

///////////////////////////////////////////////////////////////////////////////////////////
// Global variables...

#define  TT32_DEFAULT_IFACE     "00-00-5E-80-00-00"

CRITICAL_SECTION  g_tt32_lock;   // (lock for accessing ALL below variables)

char              g_tt32_dllname [ MAX_TT32_DLLNAMELEN ]  = {0};
HMODULE           g_tt32_hmoddll                          = NULL;

TT32_PROCADDRS ( open                  );
TT32_PROCADDRS ( close                 );
TT32_PROCADDRS ( read                  );
TT32_PROCADDRS ( write                 );
TT32_PROCADDRS ( ioctl                 );
TT32_PROCADDRS ( get_stats             );
TT32_PROCADDRS ( get_default_iface     );
TT32_PROCADDRS ( set_debug_output_func );
TT32_PROCADDRS ( version_string        );
TT32_PROCADDRS ( version_numbers       );
TT32_PROCADDRS ( copyright_string      );
TT32_PROCADDRS ( build_herc_iface_mac  );
TT32_PROCADDRS ( beg_write_multi       );
TT32_PROCADDRS ( end_write_multi       );

///////////////////////////////////////////////////////////////////////////////////////////

BOOL GetTT32ProcAddrs()
{
    // (required entry-points...)

    GET_REQUIRED_TT32_PROCADDR ( open                  );
    GET_REQUIRED_TT32_PROCADDR ( close                 );
    GET_REQUIRED_TT32_PROCADDR ( read                  );
    GET_REQUIRED_TT32_PROCADDR ( write                 );
    GET_REQUIRED_TT32_PROCADDR ( ioctl                 );
    GET_REQUIRED_TT32_PROCADDR ( get_stats             );
    GET_REQUIRED_TT32_PROCADDR ( get_default_iface     );
    GET_REQUIRED_TT32_PROCADDR ( set_debug_output_func );
    GET_REQUIRED_TT32_PROCADDR ( version_string        );
    GET_REQUIRED_TT32_PROCADDR ( version_numbers       );
    GET_REQUIRED_TT32_PROCADDR ( copyright_string      );

    // (optional entry-points...)

    GET_OPTIONAL_TT32_PROCADDR ( build_herc_iface_mac );
    GET_OPTIONAL_TT32_PROCADDR ( beg_write_multi      );
    GET_OPTIONAL_TT32_PROCADDR ( end_write_multi      );

    LeaveCriticalSection(&g_tt32_lock);
    return TRUE;

error:

    FreeLibrary( g_tt32_hmoddll );
    g_tt32_hmoddll = NULL;
    WRMSG ( HHC04102, "E" );
    LeaveCriticalSection(&g_tt32_lock);
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Debug string output function for use by the TUNTAP32.DLL...

void __cdecl tt32_output_debug_string( const char* debug_string )
{
    char *p,  *p2;
    char *nl, *nl2;

    // Write each line separately so it looks nice
    for (p2 = p = strdup( debug_string );
        *p2 && (nl2 = nl = strchr( p2, '\n' )); p2 = nl+1)
    {
        // Remove trailing whitespace to conform to Hercules logmsg format.
        for (; nl2 >= p2 && isspace((unsigned char)*nl2); --nl2)
            *nl2 = 0;
        if (*p2)
            // "DBG: %s"
            WRMSG ( HHC90000, "D", p2 );
    }

    free(p);
}

BOOL tt32_loaddll(); // (fwd ref)

bool  enable_tt32_debug_tracing( int enable )
{
    // Pass to TunTap32 DLL a pointer to the function it can use to
    // display debug messages with. This function of ours (that we
    // are passing it a pointer to) will then display its debugging
    // message (string) on the Hercules console so we can see it.

    // Returns true/false = whether debug tracing is enabled or not.

    if (!g_tt32_pfn_set_debug_output_func)
        if (!tt32_loaddll() || !g_tt32_pfn_set_debug_output_func)
            return false; // (debug tracing NOT enabled)

    g_tt32_pfn_set_debug_output_func( enable ? &tt32_output_debug_string : NULL );

    return enable;  // (enabled or disabled as requested)
}

///////////////////////////////////////////////////////////////////////////////////////////
// Load the TUNTAP32.DLL...

BOOL tt32_loaddll()
{
    const char*  pszDLLName;
    char         tt32_dllname_in_buff  [ MAX_PATH ];
    char         tt32_dllname_out_buff [ MAX_PATH ] = {0};
    static int   tt32_init_done = 0;

    if (!tt32_init_done)
    {
        InitializeCriticalSection( &g_tt32_lock );
        tt32_init_done = 1;
    }

    EnterCriticalSection(&g_tt32_lock);

    if (g_tt32_hmoddll)
    {
        LeaveCriticalSection(&g_tt32_lock);
        return TRUE;
    }

    // First, determine the name of the DLL we should try loading...

    if (!(pszDLLName = get_symbol( "HERCULES_IFC" )) || !*pszDLLName)
        pszDLLName = DEF_TT32_DLLNAME;

    ASSERT( pszDLLName && *pszDLLName );

    // Then check to see if the "name" contains path information or not...

    if ( strchr( pszDLLName, '/' ) || strchr( pszDLLName, '\\' ) )
    {
        // It's already a path...
        STRLCPY( tt32_dllname_in_buff, pszDLLName );
    }
    else
    {
        // It's not a path, so make it one, using loadable module path

        STRLCPY( tt32_dllname_in_buff, hdl_getpath());
        STRLCAT( tt32_dllname_in_buff, PATHSEPS );
        STRLCAT( tt32_dllname_in_buff, pszDLLName );
    }

    // Now convert it to a full path...

    // PROGRAMMING NOTE: It's important here to ensure that our end result is a path
    // with BACKWARD slashes in it and NOT forward slashes! LoadLibrary is one of the
    // few Win32 functions that cannot handle paths with forward slashes in it. For
    // 'open', etc, yeah, forward slashes are fine, but for LoadLibrary they're not!

#ifdef _MSVC_
    if ( !_fullpath( tt32_dllname_out_buff, tt32_dllname_in_buff, sizeof(tt32_dllname_out_buff) ) )
        STRLCPY(     tt32_dllname_out_buff, tt32_dllname_in_buff );
#else // (presumed cygwin)
    cygwin_conv_to_full_win32_path( tt32_dllname_in_buff, tt32_dllname_out_buff );
#endif // _MSVC_

    tt32_dllname_out_buff[ sizeof(tt32_dllname_out_buff) - 1 ] = 0;

    // Finally, copy it to our global home for it...

    STRLCPY( g_tt32_dllname, tt32_dllname_out_buff );

    ASSERT(g_tt32_dllname[0]);

    g_tt32_hmoddll = LoadLibraryEx( g_tt32_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );

    if (!g_tt32_hmoddll)
    {
        // Try again WITHOUT the path this time...

        STRLCPY( g_tt32_dllname, pszDLLName );

        g_tt32_hmoddll = LoadLibraryEx( g_tt32_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );

        if (!g_tt32_hmoddll)
        {
            char str[MAX_TT32_DLLNAMELEN + 32];
            DWORD dwLastError = GetLastError();
            LeaveCriticalSection(&g_tt32_lock);
            sprintf(str, "LoadLibraryEx(%s)", g_tt32_dllname);
            WRMSG ( HHC00161, "E", str, (int)dwLastError, strerror(dwLastError) );
            return FALSE;
        }
    }

    // Resolve our required DLL entry-point variables...

    if (!GetTT32ProcAddrs())
        return FALSE;

    WRMSG ( HHC04100, "I", g_tt32_dllname, g_tt32_pfn_version_string() );

#if defined(DEBUG) || defined(_DEBUG)
    enable_tt32_debug_tracing(1);   // (enable debug tracing by default for DEBUG builds)
#endif

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////

void tt32_version_numbers( int* major, int* inter, int* minor, int* build )
{
    *major = *inter = *minor = *build = 0;
    if (tt32_loaddll())
        g_tt32_pfn_version_numbers( major, inter, minor, build );
}

///////////////////////////////////////////////////////////////////////////////////////////

int  tt32_open( char* pszGatewayDevice, int iFlags )
{
    int rc, errnum;
    if (!tt32_loaddll()) return -1;
    if (!      g_tt32_pfn_open_ex )
        return g_tt32_pfn_open    ( pszGatewayDevice, iFlags );
    rc    =    g_tt32_pfn_open_ex ( pszGatewayDevice, iFlags, &errnum );
    errno =                                                    errnum;
    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////

int  tt32_read( int fd, u_char* buffer, u_long size )
{
    int rc, errnum;
    if (!tt32_loaddll()) return -1;
    if (!      g_tt32_pfn_read_ex )
        return g_tt32_pfn_read    ( fd, buffer, size );
    rc    =    g_tt32_pfn_read_ex ( fd, buffer, size, &errnum );
    errno =                                            errnum;
    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////

int  tt32_write( int fd, u_char* buffer, u_long size )
{
    int rc, errnum;
    if (!tt32_loaddll()) return -1;
    if (!      g_tt32_pfn_write_ex )
        return g_tt32_pfn_write    ( fd, buffer, size );
    rc    =    g_tt32_pfn_write_ex ( fd, buffer, size, &errnum );
    errno =                                             errnum;
    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////

int  tt32_beg_write_multi( int fd, u_long bufsiz )
{
    int rc; if (!tt32_loaddll()) return -1;

    if (     g_tt32_pfn_beg_write_multi_ex ) { int        errnum;
        rc = g_tt32_pfn_beg_write_multi_ex ( fd, bufsiz, &errnum );
                                                  errno = errnum;} else
    if (     g_tt32_pfn_beg_write_multi )
        rc = g_tt32_pfn_beg_write_multi    ( fd, bufsiz );
    else
        rc = 0;   // (don't fail, treat as nop instead)
    return rc;
}

int  tt32_end_write_multi( int fd )
{
    int rc; if (!tt32_loaddll()) return -1;

    if (     g_tt32_pfn_end_write_multi_ex ) { int        errnum;
        rc = g_tt32_pfn_end_write_multi_ex ( fd,         &errnum );
                                                  errno = errnum;} else
    if (     g_tt32_pfn_end_write_multi )
        rc = g_tt32_pfn_end_write_multi    ( fd );
    else
        rc = 0;   // (don't fail, treat as nop instead)
    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////

int  tt32_close( int fd )
{
    int rc, errnum;
    if (!tt32_loaddll()) return -1;
#if defined(DEBUG) || defined(_DEBUG)
    display_tt32_stats(fd);
#endif
    if (!      g_tt32_pfn_close_ex )
        return g_tt32_pfn_close    ( fd );
    rc    =    g_tt32_pfn_close_ex ( fd, &errnum );
    errno =                               errnum;
    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////

int  tt32_ioctl( int fd, int iRequest, char* argp )
{
    int rc, errnum;
    if (!tt32_loaddll()) return -1;
    if (!      g_tt32_pfn_ioctl_ex )
        return g_tt32_pfn_ioctl    ( fd, iRequest, argp );
    rc    =    g_tt32_pfn_ioctl_ex ( fd, iRequest, argp, &errnum );
    errno =                                               errnum;
    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////

const char*  tt32_get_default_iface()
{
    int errnum;
    const char* pszDefaultIFace = NULL;
    if (tt32_loaddll())
    {
        if (!                    g_tt32_pfn_get_default_iface_ex )
               pszDefaultIFace = g_tt32_pfn_get_default_iface    ();
        else { pszDefaultIFace = g_tt32_pfn_get_default_iface_ex ( &errnum );
            errno =                                                 errnum;
        }
    }
    return ( pszDefaultIFace ? pszDefaultIFace : TT32_DEFAULT_IFACE );
}

///////////////////////////////////////////////////////////////////////////////////////////

int  display_tt32_stats( int fd )
{
    int errnum;
    TT32STATS stats;

    if (!tt32_loaddll()) return -1;

    memset(&stats, 0, sizeof(stats));
    stats.dwStructSize = sizeof(stats);

    if (!  g_tt32_pfn_get_stats_ex )
           g_tt32_pfn_get_stats    ( fd, &stats );
    else { g_tt32_pfn_get_stats_ex ( fd, &stats, &errnum );
        errno =                                   errnum;
    }

    if (stats.dwStructSize >= sizeof(stats))
    {
        // New version 3.3 stats

        // "%s Statistics:"
        WRMSG( HHC04101, "I", g_tt32_dllname );

        // "  %s%5luK"
        WRMSG( HHC04103, "I", "Size of Kernel Hold Buffer:      ", (stats.dwKernelBuffSize   + 1023) / 1024 );
        WRMSG( HHC04103, "I", "Size of DLL I/O Buffer:          ", (stats.dwReadBuffSize     + 1023) / 1024 );
        WRMSG( HHC04103, "I", "Maximum DLL I/O Bytes Received:  ", (stats.dwMaxBytesReceived + 1023) / 1024 );

        // "  %12"PRId64"  %s"
        WRMSG( HHC04104, "I", stats.n64WriteCalls,             "Total Write Calls" );
        WRMSG( HHC04104, "I", stats.n64WriteIOs,               "Total Write I/Os" );
        WRMSG( HHC04104, "I", stats.n64ZeroMACPacketsWritten,  "Packets To All Zeroes MAC Written" );
        WRMSG( HHC04104, "I", stats.n64PacketsWritten,         "Total Packets Written" );
        WRMSG( HHC04104, "I", stats.n64BytesWritten,           "Total Bytes Written" );
        WRMSG( HHC04104, "I", stats.n64ReadCalls,              "Total Read Calls" );
        WRMSG( HHC04104, "I", stats.n64ReadIOs,                "Total Read I/Os" );
        WRMSG( HHC04104, "I", stats.n64InternalPackets,        "Internally Handled ARP Packets" );
        WRMSG( HHC04104, "I", stats.n64OwnPacketsIgnored,      "Packets From Ourself" );
        WRMSG( HHC04104, "I", stats.n64IgnoredPackets,         "Total Ignored Packets" );
        WRMSG( HHC04104, "I", stats.n64ZeroMACPacketsRead,     "Packets To All Zeroes MAC Read" );
        WRMSG( HHC04104, "I", stats.n64PacketsRead,            "Total Packets Read" );
        WRMSG( HHC04104, "I", stats.n64BytesRead,              "Total Bytes Read" );
    }
    else
    {
        // Old pre version 3.3 stats

        // "%s Statistics:"
        WRMSG( HHC04101, "I", g_tt32_dllname );

        // "  %s%5luK"
        WRMSG( HHC04103, "I", "Size of Kernel Hold Buffer:      ", (stats.dwKernelBuffSize   + 1023) / 1024 );
        WRMSG( HHC04103, "I", "Size of DLL I/O Buffer:          ", (stats.dwReadBuffSize     + 1023) / 1024 );
        WRMSG( HHC04103, "I", "Maximum DLL I/O Bytes Received:  ", (stats.dwMaxBytesReceived + 1023) / 1024 );

        // "  %12"PRId64"  %s"
        WRMSG( HHC04104, "I", stats.n64WriteCalls,             "Total Write Calls" );
        WRMSG( HHC04104, "I", stats.n64WriteIOs,               "Total Write I/Os" );
//      WRMSG( HHC04104, "I", stats.n64ZeroMACPacketsWritten,  "Packets To All Zeroes MAC Written" );
        WRMSG( HHC04104, "I", stats.n64PacketsWritten,         "Total Packets Written" );
        WRMSG( HHC04104, "I", stats.n64BytesWritten,           "Total Bytes Written" );
        WRMSG( HHC04104, "I", stats.n64ReadCalls,              "Total Read Calls" );
        WRMSG( HHC04104, "I", stats.n64ReadIOs,                "Total Read I/Os" );
        WRMSG( HHC04104, "I", stats.n64InternalPackets,        "Internally Handled ARP Packets" );
//      WRMSG( HHC04104, "I", stats.n64OwnPacketsIgnored,      "Packets From Ourself" );
        WRMSG( HHC04104, "I", stats.n64IgnoredPackets,         "Total Ignored Packets" );
//      WRMSG( HHC04104, "I", stats.n64ZeroMACPacketsRead,     "Packets To All Zeroes MAC Read" );
        WRMSG( HHC04104, "I", stats.n64PacketsRead,            "Total Packets Read" );
        WRMSG( HHC04104, "I", stats.n64BytesRead,              "Total Bytes Read" );
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// BOOLEAN FUNCTION

int tt32_build_herc_iface_mac ( BYTE* out_mac, const BYTE* in_ip )
{
    // We prefer to let TunTap32 do it for us (since IT'S the one
    // that decides what it should really be) but if they're using
    // an older version of TunTap32 that doesn't have the function
    // then we'll do it ourselves just like before...

    if (!g_tt32_pfn_build_herc_iface_mac)
        return 0;   // (FALSE: must do it yourself)

    g_tt32_pfn_build_herc_iface_mac( out_mac, in_ip );
    return 1;       // (TRUE: ok we did it for you)
}

///////////////////////////////////////////////////////////////////////////////////////////

#endif // !defined(OPTION_W32_CTCI)

///////////////////////////////////////////////////////////////////////////////////////////
