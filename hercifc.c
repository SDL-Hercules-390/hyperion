/* HERCIFC.H     (C) Copyright Roger Bowler, 2000-2012               */
/*               (C) Copyright James A. Pierson, 2002-2009           */
/*              Hercules Interface Configuration Program             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

// Based on code originally written by Roger Bowler
// Modified to communicate via unix sockets.
//
// This module configures the TUN/TAP interface for Hercules.
// It is invoked as a setuid root program by tuntap.c
//
// The are no command line arguments anymore.
//
// Error messages are written to stderr, which is redirected to
// the Hercules message log by ctcadpt.c
//
// The exit status is zero if successful, non-zero if error.
//

#include "hstdinc.h"

#if defined(BUILD_HERCIFC)
#include "hercules.h"
#include "hercifc.h"

#define UTILITY_NAME    "hercifc"
#define UTILITY_DESC    "Hercules Network Interface Configuration Program"

// --------------------------------------------------------------------
// HERCIFC program entry point
// --------------------------------------------------------------------

int main( int argc, char **argv )
{
    char*       pszProgName  = NULL;    // Name of this program
    char*       pOp          = NULL;    // Operation text
    char*       pIF          = NULL;    // -> interface name
    void*       pArg         = NULL;    // -> hifr or rtentry
    CTLREQ      ctlreq;                 // Request Buffer
    int         fd_inet;                // Socket descriptor
#if defined(ENABLE_IPV6)
    int         fd_inet6;               // Socket descriptor
#endif /* defined(ENABLE_IPV6) */
    int         fd;                     // FD for ioctl
    int         rc;                     // Return code
    pid_t       ppid;                   // Parent's PID
    int         answer;                 // 1 = write answer to stdout

    UNREFERENCED( argc );

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pszProgName );

    DROP_PRIVILEGES(CAP_NET_ADMIN);

    pszProgName = strdup( argv[0] );

    if (argv[1])
    {
        sysblk.msglvl = atoi( argv[1] );

        if (argv[2])
            set_codepage_no_msgs( argv[2] );
    }

    // Must not be run from the commandline
    if( isatty( STDIN_FILENO ) )
    {
        // "%s: Must be called from within Hercules; ... exiting"
        FWRMSG( stderr, HHC00162, "E", pszProgName );
        exit( 1 );
    }

    // Obtain a socket for ioctl operations
    fd_inet = socket( AF_INET, SOCK_DGRAM, 0 );

    if( fd_inet < 0 )
    {
        // "%s: Cannot obtain %s socket: %s; ... exiting"
        FWRMSG( stderr, HHC00163, "E", pszProgName, "inet", strerror( errno ));
        exit( 2 );
    }

#if defined(ENABLE_IPV6)
    fd_inet6 = socket( AF_INET6, SOCK_DGRAM, 0 );

    if( fd_inet6 < 0 )
    {
        // "%s: Cannot obtain %s socket: %s; ... exiting"
        FWRMSG( stderr, HHC00163, "E", pszProgName, "inet6", strerror( errno ));
        fd_inet6 = fd_inet;
    }
#endif /* defined(ENABLE_IPV6) */

    ppid = getppid();

    // Process ioctl messages from Hercules
    while( 1 )
    {
        rc = read( STDIN_FILENO,
                   &ctlreq,
                   CTLREQ_SIZE );

        if( rc == -1 )
        {
            // "%s: I/O error on read: %s; ... exiting"
            FWRMSG( stderr, HHC00164, "E", pszProgName, strerror( errno ));
            close( STDIN_FILENO  );
            close( STDOUT_FILENO );
            close( STDERR_FILENO );
            exit( 3 );
        }

        if( ppid != getppid() )
        {
            sleep( 1 ); // Let other messages go first
            // "%s: Hercules disappeared!! ... exiting"
            FWRMSG( stderr, HHC00168, "E", pszProgName );
            close( STDIN_FILENO  );
            close( STDOUT_FILENO );
            close( STDERR_FILENO );
            exit( 4 );
        }

        fd = fd_inet;
        answer = 0;

        switch( ctlreq.iCtlOp )
        {
#if !defined( __APPLE__ ) && !defined( FREEBSD_OR_NETBSD )
        case TUNSETIFF:
            pOp  = "TUNSETIFF";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = "?";
            fd = ctlreq.iProcID;
            answer = 1;
            break;
#endif

        case SIOCSIFADDR:
            pOp  = "SIOCSIFADDR";

#if defined(ENABLE_IPV6)
            if( ctlreq.iru.hifr.hifr_afamily == AF_INET6 )
            {
                pArg = &ctlreq.iru.hifr.in6_ifreq;
                fd = fd_inet6;
            }
            else
#endif /* defined(ENABLE_IPV6) */

            {
                pArg = &ctlreq.iru.hifr.ifreq;
            }
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;

        case SIOCSIFDSTADDR:
            pOp  = "SIOCSIFDSTADDR";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;

        case SIOCSIFFLAGS:
            pOp  = "SIOCSIFFLAGS";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;

#if 0 /* (hercifc can't "get" information, only "set" it) */
        case SIOCGIFFLAGS:
            pOp  = "SIOCGIFFLAGS";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            answer = 1;
            break;
#endif /* (caller should do 'ioctl' directly themselves instead) */

        case SIOCSIFMTU:
            pOp  = "SIOCSIFMTU";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;

        case SIOCADDMULTI:
            pOp  = "SIOCADDMULTI";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;

        case SIOCDELMULTI:
            pOp  = "SIOCDELMULTI";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;

#ifdef OPTION_TUNTAP_SETNETMASK
        case SIOCSIFNETMASK:
            pOp  = "SIOCSIFNETMASK";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;
#endif
#ifdef OPTION_TUNTAP_SETMACADDR
        case SIOCSIFHWADDR:
            pOp  = "SIOCSIFHWADDR";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;
#endif
#ifdef OPTION_TUNTAP_DELADD_ROUTES
        case SIOCADDRT:
            pOp  = "SIOCADDRT";
            pArg = &ctlreq.iru.rtentry;
            pIF  = ctlreq.szIFName;
            ctlreq.iru.rtentry.rt_dev = ctlreq.szIFName;
            break;

        case SIOCDELRT:
            pOp  = "SIOCDELRT";
            pArg = &ctlreq.iru.rtentry;
            pIF  = ctlreq.szIFName;
            ctlreq.iru.rtentry.rt_dev = ctlreq.szIFName;
            break;
#endif
#ifdef OPTION_TUNTAP_CLRIPADDR
        case SIOCDIFADDR:
            pOp  = "SIOCDIFADDR";
            pArg = &ctlreq.iru.hifr.ifreq;
            pIF  = ctlreq.iru.hifr.hifr_name;
            break;
#endif
        case CTLREQ_OP_DONE:
            // HHC00169 "%s: DONE! ... exiting"
            FWRMSG( stderr, HHC00169, "I", pszProgName );
            close( STDIN_FILENO  );
            close( STDOUT_FILENO );
            close( STDERR_FILENO );
            exit( 0 );

        default:
            // "%s: Unknown request: %lX; ... ignoring and continuing"
            FWRMSG( stderr, HHC00165, "W", pszProgName, ctlreq.iCtlOp );
            continue;
        }

        if (MLVL( DEBUG ))
        {
            // "%s: Doing %s on %s ..."
            FWRMSG( stderr, HHC00167, "D", pszProgName, pOp, pIF );
        }

        rc = ioctl( fd, ctlreq.iCtlOp, pArg );
        if( rc < 0 )
        {
            if (1
        #if defined(SIOCSIFHWADDR) && defined(ENOTSUP)
                 /* Suppress spurious error message */
             && !(ctlreq.iCtlOp == SIOCSIFHWADDR && errno == ENOTSUP)
        #endif
        #if defined(SIOCDIFADDR) && defined(EINVAL)
                 /* Suppress spurious error message */
             && !(ctlreq.iCtlOp == SIOCDIFADDR   && errno == EINVAL)
        #endif
        #if defined(TUNSETIFF) && defined(EINVAL)
                 /* Suppress spurious error message */
             && !(ctlreq.iCtlOp == TUNSETIFF   && errno == EINVAL)
        #endif
               )
            {
                // "%s: ioctl error doing %s on %s: %d %s; ... ignoring and continuing"
                FWRMSG( stderr, HHC00166, "E", pszProgName, pOp, pIF, errno, strerror( errno ));
            }
        }
        else if (answer)
        {
            VERIFY(0 <= write( STDOUT_FILENO, &ctlreq, CTLREQ_SIZE ));
        }

    } // end while (1)

    UNREACHABLE_CODE( return -1 );
}

#endif // defined(BUILD_HERCIFC)

