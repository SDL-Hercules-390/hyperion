/* HOSTINFO.C   (C) Copyright "Fish" (David B. Trout), 2002-2012     */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*                 Hercules functions to set/query host information  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HOSTINFO_C_
#define _HUTIL_DLL_

#include "hercules.h"

#if defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )
#  if defined(HAVE_SYS_SYSCTL_H)
#    include <sys/sysctl.h>
#  endif
#endif /* #if defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD ) */

DLL_EXPORT HOST_INFO  hostinfo;     /* Host system information       */

#if !defined( _MSVC_ )
static void linux_init_hostinfo          ( HOST_INFO* pHostInfo );
static bool linux_preferred_host_cpu_info( HOST_INFO* pHostInfo );
static void linux_fallback_host_cpu_info ( HOST_INFO* pHostInfo );
#endif

/*-------------------------------------------------------------------*/
/*              Initialize host system information                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT void init_hostinfo ( HOST_INFO* pHostInfo )
{
    if (!pHostInfo)
        pHostInfo = &hostinfo;

    INIT_BLOCK_HEADER_TRAILER( pHostInfo, HOST_INFO );

    pHostInfo->valid_cache_nums = TRUE;     /* assume the cache size numbers are good */

#if defined( _MSVC_ )

    w32_init_hostinfo( pHostInfo );     // (Windows only...)

#else // Linux, etc.

    linux_init_hostinfo( pHostInfo );   // (Linux only...)

#endif

    // (common to both Windows *and* Linux...)

    pHostInfo->hostpagesz = (U64) HPAGESIZE();

    if ( pHostInfo->cachelinesz == 0 )
    {
        pHostInfo->cachelinesz = 64;
        pHostInfo->valid_cache_nums = FALSE;
    }

    if ( pHostInfo->L1Dcachesz == 0 && pHostInfo->L1Icachesz == 0 && pHostInfo->L1Ucachesz == 0 )
    {
        pHostInfo->L1Dcachesz = pHostInfo->L1Icachesz = ((U64)8 << SHIFT_KILOBYTE );
        pHostInfo->valid_cache_nums = FALSE;
    }

    if ( pHostInfo->L2cachesz == 0 )
    {
        pHostInfo->L2cachesz = ((U64)256 << SHIFT_KILOBYTE );
        pHostInfo->valid_cache_nums = FALSE;
    }
}

#if !defined( _MSVC_ )

    //-----------------------------------------------------------------
    //                 LINUX-ONLY...
    //-----------------------------------------------------------------

static void linux_init_hostinfo( HOST_INFO* pHostInfo )
{
    {
#if defined( HAVE_SYS_UTSNAME_H )
        struct utsname uname_info;
        uname( &uname_info );

        STRLCPY( pHostInfo->sysname,  uname_info.sysname  );
        STRLCPY( pHostInfo->nodename, uname_info.nodename );
        STRLCPY( pHostInfo->release,  uname_info.release  );
        STRLCPY( pHostInfo->version,  uname_info.version  );
        STRLCPY( pHostInfo->machine,  uname_info.machine  );
#else
        STRLCPY( pHostInfo->sysname,  "(unknown)" );
        STRLCPY( pHostInfo->nodename, "(unknown)" );
        STRLCPY( pHostInfo->release,  "(unknown)" );
        STRLCPY( pHostInfo->version,  "(unknown)" );
        STRLCPY( pHostInfo->machine,  "(unknown)" );
#endif
    }

    // Retrieve host CPU information first...

    if (!linux_preferred_host_cpu_info( pHostInfo ))
        linux_fallback_host_cpu_info( pHostInfo );

    // then retrieve everything else...    

#if defined( HAVE_SYSCONF )
    #if defined(HAVE_DECL__SC_PHYS_PAGES) && \
                HAVE_DECL__SC_PHYS_PAGES

    pHostInfo->ullTotalPhys = (RADR)((RADR)sysconf(_SC_PAGESIZE) * (RADR)sysconf(_SC_PHYS_PAGES));

    #endif
#endif

#if defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )

    /* The mibs #ifdef-ed out below are not available on FreeBSD 9.1 */
    {
        size_t  length;
        int     mib[2];
        int     iRV;
        uint64_t ui64RV;

#if defined( VM_SWAPUSAGE )
        struct  xsw_usage   xsu;
#endif
        char    machine[64];

        memset( machine, 0, sizeof( machine ));
        mib[0] = CTL_HW;

        length = sizeof(machine);
        if ( sysctl( mib, 2, &machine, &length, NULL, 0 ) != -1 )
        {
            machine[length] = 0;
            STRLCPY( pHostInfo->machine, machine );
        }

#if !defined( __OpenBSD__ )

        length = sizeof(iRV);
        if ( sysctlbyname("kern.maxfilesperproc", &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->maxfilesopen = iRV;

        length = sizeof(iRV);
        if ( sysctlbyname("hw.vectorunit", &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->vector_unit = iRV;

        length = sizeof(iRV);
        if ( sysctlbyname("hw.optional.floatingpoint", &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->fp_unit = iRV;

        length = sizeof(ui64RV);
        if ( sysctlbyname("hw.busfrequency", &ui64RV, &length, NULL, 0 ) != -1 )
            pHostInfo->bus_speed = ui64RV;

        length = sizeof(ui64RV);
        if ( sysctlbyname("hw.cpufrequency", &ui64RV, &length, NULL, 0 ) != -1 )
            pHostInfo->cpu_speed = ui64RV;

        length = (size_t)sizeof(iRV);
        iRV = 0;
        if ( sysctlbyname("hw.optional.x86_64", &iRV, &length, NULL, 0 ) != -1 )
        {
            char mach[64];

            MSGBUF( mach, "%s %s", iRV != 0 ? "64-bit" : "32-bit", pHostInfo->machine );
            STRLCPY( pHostInfo->machine, mach );
            pHostInfo->cpu_64bits = 1;
        }

        iRV = 0;
        if ( sysctlbyname("hw.optional.aes", &iRV, &length, NULL, 0 ) != -1 )
        {
            char mach[64];

            MSGBUF( mach, "%s %s", iRV != 0 ? "64-bit" : "32-bit", pHostInfo->machine );
            STRLCPY( pHostInfo->machine, mach );
            pHostInfo->cpu_aes_extns = 1;
        }

#endif // !defined( __OpenBSD__ )

#if defined( HW_MEMSIZE )

        length = sizeof(ui64RV);
        mib[1] = HW_MEMSIZE;

        if ( sysctl( mib, 2, &ui64RV, &length, NULL, 0 ) != -1 )
            pHostInfo->TotalPhys = ui64RV;
#endif

        length = sizeof(iRV);
        mib[1] = HW_USERMEM;

        if ( sysctl( mib, 2, &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->AvailPhys = iRV;

        length = sizeof(iRV);
        mib[1] = HW_PAGESIZE;

        if ( sysctl( mib, 2, &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->hostpagesz = iRV;

#if defined( HW_CACHELINE )

        length = sizeof(iRV);
        mib[1] = HW_CACHELINE;

        if ( sysctl( mib, 2, &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->cachelinesz = iRV;
#endif

#if defined( HW_L1ICACHESIZE )

        length = sizeof(iRV);
        mib[1] = HW_L1ICACHESIZE;
        if ( sysctl( mib, 2, &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->L1Icachesz = iRV;
#endif

#if defined( HW_L1DCACHESIZE )

        length = sizeof(iRV);
        mib[1] = HW_L1DCACHESIZE;
        if ( sysctl( mib, 2, &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->L1Dcachesz = iRV;
#endif

#if defined( HW_L2CACHESIZE )

        length = sizeof(iRV);
        mib[1] = HW_L2CACHESIZE;

        if ( sysctl( mib, 2, &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->L2cachesz = iRV;
#endif

#if defined( HW_L3CACHESIZE )

        length = sizeof(iRV);
        mib[1] = HW_L3CACHESIZE;

        if ( sysctl( mib, 2, &iRV, &length, NULL, 0 ) != -1 )
            pHostInfo->L3cachesz = iRV;
#endif

#if defined( VM_SWAPUSAGE )

        mib[0] = CTL_VM;
        mib[1] = VM_SWAPUSAGE;
        length = sizeof(xsu);

        if ( sysctl( mib, 2, &xsu, &length, NULL, 0 ) != -1 )
        {
            pHostInfo->TotalPageFile = xsu.xsu_total;
            pHostInfo->AvailPageFile = xsu.xsu_total - xsu.xsu_used;
        }
#endif

    }

#endif // defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )

} // end linux_init_hostinfo

#if (defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )) && !defined( __OpenBSD__ )
#else
//-----------------------------------------------------------------
//                Parse a line of LSCPU output
//-----------------------------------------------------------------

static int parse_lscpu_value(  char* line, int txtlen, char* text )
{
    return (str_caseless_eq_n( line, text, txtlen )) ?
        atoi( RTRIM( LTRIM( line + txtlen ))) : 0;
}
#endif

//-----------------------------------------------------------------
//                Retrieve Host CPU Information...
//-----------------------------------------------------------------

static bool linux_preferred_host_cpu_info( HOST_INFO* pHostInfo )
{
    int num_sockets          = 0;
    int num_cores_per_socket = 0;
    int num_threads_per_core = 0;

    // Retrieving it directly from the host system is always best...

#if (defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )) && !defined( __OpenBSD__ )

    size_t  length;
    int     iRV;

    length = sizeof(iRV);
    if ( sysctlbyname("hw.packages", &iRV, &length, NULL, 0 ) != -1 )
        num_sockets = iRV;

    length = sizeof(iRV);
    if ( sysctlbyname("hw.physicalcpu", &iRV, &length, NULL, 0 ) != -1 )
    {
        pHostInfo->num_physical_cpu = iRV;

        if (num_sockets)
            num_cores_per_socket = pHostInfo->num_physical_cpu / num_sockets;
    }

    length = sizeof(iRV);
    if ( sysctlbyname("hw.logicalcpu", &iRV, &length, NULL, 0 ) != -1 )
        pHostInfo->num_logical_cpu = iRV;

#else // (OTHER LINUX)

    // Otherwise get it from the 'lscpu' utility...

    FILE* fp;       /* File pointer to LSCPU's stdout */
    int   rc;       /* pclose() status (lscpu's return code) */

    char* COMMAND   = "lscpu";      /* LSCPU command to be executed by popen() */
    char  buffer[ PATH_MAX ];       /* read buffer for each line of LSCPU's results */

    /* Issue LSCPU command (i.e. "open" a new process),
       capturing its stdout results stream... */

    if (!(fp = popen( COMMAND, "r" )))
    {
        // "Error in function %s: %s"
        WRMSG( HHC00136, "E", "popen()", strerror( errno ));
        return false;
    }

    /* Read and parse LSCPU's stdout, looking for desired strings... */

    while (fgets( buffer, PATH_MAX, fp ))
    {
        if (!num_sockets)          num_sockets          = parse_lscpu_value( LTRIM( buffer ), 10, "Socket(s):" );
        if (!num_cores_per_socket) num_cores_per_socket = parse_lscpu_value( LTRIM( buffer ), 19, "Core(s) per socket:" );
        if (!num_threads_per_core) num_threads_per_core = parse_lscpu_value( LTRIM( buffer ), 19, "Thread(s) per core:" );

        if (1
            && num_sockets
            && num_cores_per_socket
            && num_threads_per_core
        )
            break;
    }

    /* Close the lscpu process and check its return code... */

    if ((rc = pclose( fp )) < 0)
    {
        // "Error in function %s: %s"
        WRMSG( HHC00136, "E", "pclose()", strerror( errno ));
    }

#endif // (defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )) && !defined( __OpenBSD__ )

    // Update HOSTINFO appropriately...

    pHostInfo->num_packages      =  num_sockets;
    pHostInfo->num_physical_cpu  =  num_sockets * num_cores_per_socket;
    pHostInfo->num_logical_cpu   =  num_sockets * num_cores_per_socket * num_threads_per_core;;
    pHostInfo->num_procs         =  pHostInfo->num_logical_cpu;

    if (0
        || (1
            && pHostInfo->num_packages
            && pHostInfo->num_physical_cpu
            && pHostInfo->num_logical_cpu
           )
        || (pHostInfo->num_procs)
    )
        return true;
    else
        return false;
}

static void linux_fallback_host_cpu_info( HOST_INFO* pHostInfo )
{
    #if defined( HAVE_SYSCONF )
        #if defined( HAVE_DECL__SC_NPROCESSORS_ONLN ) && \
                     HAVE_DECL__SC_NPROCESSORS_ONLN

        pHostInfo->num_procs = sysconf( _SC_NPROCESSORS_ONLN );

        #else

        // (do nothing

        #endif

    #else

    // (do nothing

    #endif
}

#endif // !defined(_MSVC_)

/*-------------------------------------------------------------------*/
/* Build a host system information string for displaying purposes    */
/*      (the returned string does NOT end with a newline)            */
/*-------------------------------------------------------------------*/
DLL_EXPORT char* format_hostinfo ( HOST_INFO*  pHostInfo,
                                   char*       pszHostInfoStrBuff,
                                   size_t      nHostInfoStrBuffSiz )
{
    char num_procs[64];

    if (!pszHostInfoStrBuff || !nHostInfoStrBuffSiz)
        return pszHostInfoStrBuff;

    ASSERT( pszHostInfoStrBuff && nHostInfoStrBuffSiz );

    // Point to host information

    if (!pHostInfo)
        pHostInfo = &hostinfo;

    // If detailed CPU information is available...

    if ( pHostInfo->num_packages     != 0 &&
         pHostInfo->num_physical_cpu != 0 &&
         pHostInfo->num_logical_cpu  != 0 )
    {
        MSGBUF( num_procs, "LP=%d, Cores=%d, CPUs=%d", pHostInfo->num_logical_cpu,
                            pHostInfo->num_physical_cpu, pHostInfo->num_packages );
    }
    else // (only GENERIC number-of-CPUs information is available)
    {
        if (pHostInfo->num_procs > 1)         MSGBUF(  num_procs, "MP=%d", pHostInfo->num_procs );
        else if ( pHostInfo->num_procs == 1 ) STRLCPY( num_procs, "UP" );
        else                                  STRLCPY( num_procs, "" );
    }

#if defined( OPTION_LONG_HOSTINFO )

    // (long: show host version too)

    snprintf( pszHostInfoStrBuff, nHostInfoStrBuffSiz,

        "Running on: %s %s-%s. %s, %s%s",

        pHostInfo->nodename,
        pHostInfo->sysname,
        pHostInfo->release,
        pHostInfo->version,     // (host version)
        pHostInfo->machine,
        num_procs
    );

#else // (short: no host version info)

    // Dual Quad core (GENERIC):   HHC01417I Running on: ... MP=8
    // Dual Quad core (DETAILED):  HHC01417I Running on: ... LP=8, Cores=8, CPUs=2

    snprintf( pszHostInfoStrBuff, nHostInfoStrBuffSiz,

        "Running on: %s (%s-%s %s) %s",

        pHostInfo->nodename,
        pHostInfo->sysname,
        pHostInfo->release,
        pHostInfo->machine,
        num_procs
    );
#endif

    // (ensure string is NULL terminated)
    *(pszHostInfoStrBuff + nHostInfoStrBuffSiz - 1) = 0;

    return pszHostInfoStrBuff;
}
