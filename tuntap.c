/* TUNTAP.C    (C) Copyright James A. Pierson, 2002-2012             */
/*             (C) Copyright "Fish" (David B. Trout), 2002-2009      */
/*              Hercules - TUN/TAP Abstraction Layer                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

// TUN/TAP implementations differ among platforms. Linux and FreeBSD
// offer much the same functionality but with differing semantics.
// Windows does not have TUN/TAP but thanks to "Fish" (David B. Trout)
// we have a way of emulating the TUN/TAP interface through a set of
// custom DLLs he has provided us.
//
// This abstraction layer is an attempt to create a common API set
// that works on all platforms with (hopefully) equal results.
//

/* On  Linux  you  open  the character special file /dev/net/tun and */
/* then select the particular tunnel by ioctl().                     */
/*                                                                   */
/* On  FreeBSD  and  OS/X you can open /dev/tun which will clone the */
/* interface  and  give  you a file descriptor for /dev/tun<n> where */
/* <n>   is  the  lowest  unused  interface.   For  a  preconfigured */
/* interface  you  open  /dev/tun0  (or  which ever one you desire). */
/* Thus  for  preconfigured FreeBSD interfaces we need to modify the */
/* name of the character file being opened.                          */

/* On NetBSD you open /dev/tun<n> which must be given by the user,   */
/* who has pre-configured that tunnel for use.                       */
/* If you want a L2 (ethernet) interface, open /dev/tap or           */
/* /dev/tap<n>.  You cannot switch from one mode to the other.       */


#include "hstdinc.h"

/* jbs 1/19/2008 added ifdef on __SOLARIS__ */
#if !defined(__SOLARIS__)

#include "hercules.h"
#include "tuntap.h"
#include "devtype.h"
#include "ctcadpt.h"
#include "hercifc.h"

#if defined( OPTION_W32_CTCI )
#include "w32ctca.h"
#endif

#if defined(__FreeBSD__)
#include <net/if_tun.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <libprocstat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/sysctl.h>

#define FBSD_TUN_EXT 0
#define FBSD_TUN_INT 1
#endif

// ====================================================================
// Declarations
// ====================================================================

#if !defined( OPTION_W32_CTCI )

static int IFC_IOCtl( int fd, unsigned long int iRequest, char* argp );
static int ifc_fd[2] = { -1, -1 };
static pid_t ifc_pid = 0;

static void tuntap_term( void* arg )
{
    UNREFERENCED( arg );
    close(ifc_fd[0]);
    close(ifc_fd[1]);
    ifc_fd[0] = ifc_fd[1] = -1;
    kill(ifc_pid, SIGINT);
}

#endif /* if !defined( OPTION_W32_CTCI ) */


// ====================================================================
// Primary Module Entry Points
// ====================================================================

//
// TUNTAP_SetMode           (TUNTAP_CreateInterface helper)
//
#if defined(__linux__) || defined(OPTION_W32_CTCI)
static int TUNTAP_SetMode (int fd, struct hifr *hifr, int iFlags)
{
    int rc;

    /* Try TUNTAP_ioctl first */
    rc = TUNTAP_IOCtl (fd, TUNSETIFF, (char *) hifr);

#if !defined( OPTION_W32_CTCI )
    /* If invalid value, try with the pre-2.4.5 value */
    if (0 > rc && errno == EINVAL)
        rc = TUNTAP_IOCtl (fd, ('T' << 8) | 202, (char *) hifr);

    /* kludge for EPERM and linux 2.6.18 */
    if (0 > rc && errno == EPERM && !(IFF_NO_HERCIFC & iFlags))
    {
        int             ifd[2];
        const char     *hercifc;
        pid_t           pid;
        CTLREQ          ctlreq;
        fd_set          selset;
        struct timeval  tv;
        int             sv_err;
        int             status;

        if (socketpair (AF_UNIX, SOCK_STREAM, 0, ifd) < 0)
            return -1;

        if (!(hercifc = get_symbol ("HERCULES_IFC")) || !*hercifc)
            hercifc = HERCIFC_CMD;

        pid = fork();

        if (pid < 0)
            return -1;
        else if (pid == 0)
        {
            /* child */
            char msglvl[16] = {0};
            dup2 (ifd[0], STDIN_FILENO);
            dup2 (STDOUT_FILENO, STDERR_FILENO);
            dup2 (ifd[0], STDOUT_FILENO);
            close (ifd[1]);
            MSGBUF( msglvl, "%d", sysblk.msglvl );
            execlp( hercifc, hercifc, msglvl, query_codepage(), NULL );
            WRMSG( HHC00136, "E", "execlp()", strerror( errno ) );
            exit( 127 );
        }

        /* parent */
        close(ifd[0]);

        /* Request hercifc to issue the TUNSETIFF ioctl */
        memset (&ctlreq, 0, CTLREQ_SIZE);
        ctlreq.iCtlOp = TUNSETIFF;
        ctlreq.iProcID = fd;
        memcpy (&ctlreq.iru.hifr, hifr, sizeof (struct hifr));
        VERIFY(CTLREQ_SIZE == write (ifd[1], &ctlreq, CTLREQ_SIZE));

        /* Get response, if any, from hercifc */
        FD_ZERO (&selset);
        FD_SET (ifd[1], &selset);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        rc = select (ifd[1]+1, &selset, NULL, NULL, &tv);
        if (rc > 0)
        {
            rc = read (ifd[1], &ctlreq, CTLREQ_SIZE);
            if (rc > 0)
                memcpy (hifr, &ctlreq.iru.hifr, sizeof (struct hifr));
        }
        else if (rc == 0)
        {
            // "Timeout for module %s, possible older version"
            WRMSG( HHC00135, "E", hercifc );
            errno = EPERM;
            rc = -1;
        }

        /* clean-up */
        sv_err = errno;
        close (ifd[1]);
        kill (pid, SIGKILL);
        waitpid (pid, &status, 0);
        errno = sv_err;
    }
#endif /* if !defined( OPTION_W32_CTCI ) */

    return rc;
}   // End of function  TUNTAP_SetMode()
#endif /* __linux__ || OPTION_W32_CTCI */


//
// TUNTAP_CreateInterface
//
//
// Creates a new network interface using TUN/TAP. Reading from or
// writing to the file descriptor returned from this call will pass
// network packets to/from the virtual network interface.
//
// A TUN interface is a Point-To-Point connection from the driving
// system's IP stack to the guest OS running within Hercules.
//
// A TAP interface in a virtual network adapter that "tap's" off the
// driving system's network stack.
//
// On *nix boxen, this is accomplished by opening the special TUN/TAP
// character device (usually /dev/net/tun). Once the character device
// is opened, an ioctl call is done to set they type of interface to be
// created, IFF_TUN or IFF_TAP. Once the interface is created, the
// interface name is returned in pszNetDevName.
//
// Input:
//      pszTUNDevice  Pointer to the name of the TUN/TAP char device
//      iFlags        Flags for the new interface:
//                       IFF_TAP   - Create a TAP interface or
//                       IFF_TUN   - Create a TUN interface
//                       IFF_NO_PI - Do not include packet information
//
// On Win32, calls are made to Fish's TT32 DLL's to accomplish the same
// functionality. There are, however, a few differences in regards to the
// arguments:
//
// Input:
//      pszTUNDevice   Pointer to a string that describes the physical
//                     adapter to attach the TUN/TAP interface to.
//                     This string can contain any of the following:
//                       1) IP address (in a.b.c.d notation)
//                       2) MAC address (in xx-xx-xx-xx-xx-xx or
//                                          xx:xx:xx:xx:xx:xx notation).
//                       3) Name of the adapter as displayed on your
//                          Network and Dial-ip Connections window
//                          (Windows 2000 only future implementation)
//      iFlags         Flags for the new interface:
//                       IFF_TAP   - Create a TAP interface or
//                       IFF_TUN   - Create a TUN interface
//                       IFF_NO_PI - Do not include packet information
//                       IFF_OSOCK - Create as socket (CTCI-WIN v3.3)
//      pszNetDevName  Pointer to receive the name if the interface,
//                     If this field is prefilled then that interface
//                     will be allocated.
//
// Output:
//      pfd            Pointer to receive the file descriptor of the
//                       TUN/TAP interface.
//      pszNetDevName  Pointer to receive the name of the interface.
//      pinternal      Pointer to receive the status of the interface;
//                     1 for devices created by Hercules, 0 for pre-existing
//                     devices. To be used on systems that impose manual 
//                     removal of devices (e.g. FreeBSD).

int             TUNTAP_CreateInterface( char* pszTUNDevice,
                                        int   iFlags,
                                        int*  pfd,
                                        char* pszNetDevName,
                                        int*  pinternal )
{
    int fd;                        // File descriptor

#if defined (__FreeBSD__)
    // Were we passed a device name?
    if(*pszNetDevName) {
        // Yes, we got a name. 
        char device[PATH_MAX] = "/dev/"; 
        strlcat(device, pszNetDevName, sizeof(device));

        fd = TUNTAP_Open( device, O_RDWR );

        // Does it exist?
        if( fd < 0 ) {
            if (ENOENT == errno) {
                // It doesn't exist. 
                WRMSG( HHC99998, "I", "TUNTAP_CreateInterface: Device doesn't exist yet:", device);
                *pinternal = FBSD_TUN_INT;

                // Open /dev/tun itself to trigger creation of a new device
                fd = TUNTAP_Open( pszTUNDevice, O_RDWR );

                if( fd < 0 ) { 
                    // Something is not right. Bailing.
                    WRMSG( HHC00137, "E", pszTUNDevice, strerror( errno ));
                    return -1;
                }    

                // Setting explicit IFF_POINTOPOINT. Not sure if necessary but...
                int mode = IFF_POINTOPOINT;
                if (ioctl(fd, TUNSIFMODE, &mode) < 0) {
                    WRMSG( HHC00137, "E", pszTUNDevice, strerror( errno ));
                    return -1;
                }

                struct ifreq ifr;
                memset(&ifr, 0, sizeof(ifr));

                if (ioctl(fd, TUNGIFNAME, &ifr) < 0) {
                    WRMSG( HHC99998, "E", "TUNTAP_CreateInterface: ioctl(fd, TUNGIFNAME, &ifr) failed:", strerror( errno ));
                    return -1;
                }

                // What is our current device name and what do we want it to be
                ifr.ifr_data = pszNetDevName;

                int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
                if (sock < 0) {
                    WRMSG( HHC99998, "E", "TUNTAP_CreateInterface: socket(AF_LOCAL, SOCK_DGRAM, 0) failed:", strerror( errno ));
                    return -1;
                }

                if (TUNTAP_IOCtl(sock, SIOCSIFNAME, &ifr) < 0) {
                    WRMSG( HHC99998, "E", "TUNTAP_CreateInterface: ioctl(sock, SIOCSIFNAME, &ifr) failed:", strerror( errno ));
                    return -1;
                }

                close(sock);

            } else {
                // It exists but we can't work with it. Bailing.
                WRMSG( HHC00137, "E", pszTUNDevice, strerror( errno ));
                return -1;
            }
        } else {
            // It exists, and we can open it. Cool.
            *pinternal = FBSD_TUN_EXT;
            WRMSG( HHC99998, "I", "TUNTAP_CreateInterface: Device exists and is usable:", device);
        } 
    } else {
        // No, we didn't get a name. Create the first available tun device and go with it.
        *pinternal = FBSD_TUN_INT;

        fd = TUNTAP_Open( pszTUNDevice, O_RDWR );
        if( fd < 0 ) { 
            // Something is not right. Bailing.
            WRMSG( HHC00137, "E", pszTUNDevice, strerror( errno ));
            return -1;
        }    
        
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));

        // Fetching the name of the created device
        if (ioctl(fd, TUNGIFNAME, &ifr) < 0) {
            WRMSG( HHC99998, "E", "TUNTAP_CreateInterface: ioctl(fd, TUNGIFNAME, &ifr) failed:", strerror( errno ));
            return -1;
        }
    
        strcpy( pszNetDevName, ifr.ifr_name );
    
        WRMSG( HHC99998, "I", "TUNTAP_CreateInterface: Device was created:", ifr.ifr_name);
    }

    *pfd = fd;
    return 0;
#else

#if !defined( OPTION_W32_CTCI )
    struct utsname utsbuf;

    if( uname( &utsbuf ) != 0 )
    {
        // "Error in function %s: %s"
        WRMSG( HHC00136, "E", "uname()", strerror( errno ));

        return -1;
    }
#endif

    // Open TUN device
    fd = TUNTAP_Open( pszTUNDevice, O_RDWR | (iFlags & IFF_OSOCK) );

    if( fd < 0 )
    {
        // "Error opening TUN/TAP device %s: %s"
        WRMSG( HHC00137, "E", pszTUNDevice, strerror( errno ));
        return -1;
    }

    *pfd = fd;

#if !defined( OPTION_W32_CTCI )
    if ( strncasecmp( utsbuf.sysname, "linux",  5 ) == 0 )
#endif
    {
#if defined (__linux__) || defined(OPTION_W32_CTCI)
        // Linux kernel (builtin tun device) or Windows
        struct hifr hifr;

        memset( &hifr, 0, sizeof( hifr ) );
        hifr.hifr_flags = iFlags & ~(iFlags & IFF_OSOCK) & 0xffff;
        if(*pszNetDevName)
            STRLCPY( hifr.hifr_name, pszNetDevName );

        if( TUNTAP_SetMode (fd, &hifr, iFlags) < 0 )
        {
#if !defined( OPTION_W32_CTCI )
            logmsg("nohif %x\n", IFF_NO_HERCIFC & iFlags);
            if (EPERM == errno && (IFF_NO_HERCIFC & iFlags))
                // "Preconfigured interface %s does not exist or is not accessible by Hercules (EPERM)"
                WRMSG( HHC00154, "E", hifr.hifr_name );
            else
#endif // !defined( OPTION_W32_CTCI )
                // "Error setting TUN/TAP mode %s: %s"
                WRMSG( HHC00138, "E", hifr.hifr_name, strerror( errno ));
            return -1;
        }

        strcpy( pszNetDevName, hifr.hifr_name );
#endif /* __linux__ || OPTION_W32_CTCI */
    }
#if !defined( OPTION_W32_CTCI )
    else
    {
        if ((iFlags & IFF_TUN) && !strstr(pszTUNDevice, "tun")) {
             WRMSG( HHC00156, "W", pszTUNDevice );
        }
        if ((iFlags & IFF_TAP) && !strstr(pszTUNDevice, "tap")) {
             WRMSG( HHC00157, "W", pszTUNDevice );
        }

        // Other OS: Simply use basename of the device
        // Notes: (JAP) This is problematic at best. Until we have a
        //        clean FreeBSD compile from the base tree I can't
        //        spend a lot of time on this... so it will remain.
        //        My best guess is that this will cause other functions
        //        to fail miserably but I have no way to test it.
        // This should work on OS X with Christoph Pfisterer's TUN driver,
        //        since it does set the device name to the basename of the
        //        file. -- JRM
        char *p = strrchr( pszTUNDevice, '/' );

        if( p )
            strncpy( pszNetDevName, ++p, IFNAMSIZ );
        else
        {
            // "Invalid TUN/TAP device name %s"
            WRMSG( HHC00139, "E", pszTUNDevice );
            return -1;
        }
    }
#endif // !defined( OPTION_W32_CTCI )

    return 0;
#endif // defined (__FreeBSD__)
}   // End of function  TUNTAP_CreateInterface()


//
// Redefine 'TUNTAP_IOCtl' for the remainder of the functions.
// This forces all 'ioctl' calls to go to 'hercifc'.
//
#if !defined( OPTION_W32_CTCI )
  #undef  TUNTAP_IOCtl
  #define TUNTAP_IOCtl    IFC_IOCtl
#endif

//
// TUNTAP_Close
//
//
// Closes a TUN/TAP network interface. 
//
// On Linux, TUNTAP_Close is defined as close() but contrary to Linux, 
// FreeBSD does not remove the interface after close()ing it. 
// This leaves the device visible in e.g. ifconfig in a disconnected state. 
// Removing it requires an explicit ioctl.
//
// Input:
//      fd	File descriptor of the TUN/TAP device
//

#if defined (__FreeBSD__)
int             TUNTAP_Close( int fd, int internal )
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    if (ioctl(fd, TUNGIFNAME, &ifr) < 0) {
        WRMSG( HHC99998, "E", "TUNTAP_Close: ioctl(fd, TUNGIFNAME, &ifr) failed:", strerror( errno ));
        return -1;
    }

    WRMSG( HHC99998, "I", "TUNTAP_Close: Closing", ifr.ifr_name);

    if (close(fd) < 0) {
        WRMSG( HHC99999, "E", "TUNTAP_Close: Closing the file descriptor failed!");
        return -1;
    } 

    // Only remove the device from the OS if we created it outselves
    if (internal) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            return -1;
        }

        if (ioctl(sock, SIOCIFDESTROY, &ifr) < 0) {
            WRMSG( HHC99998, "E", "TUNTAP_Close: ioctl(sock, SIOCIFDESTROY, &ifr) failed:", strerror( errno ));
            close(sock);
            return -1;
        }
    }

    return 0;
}
#endif

//
// TUNTAP_ClrIPAddr
//
#ifdef   OPTION_TUNTAP_CLRIPADDR
int             TUNTAP_ClrIPAddr( char* pszNetDevName )
{
    struct hifr hifr;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );

    return TUNTAP_IOCtl( 0, SIOCDIFADDR, (char*)&hifr );
}   // End of function  TUNTAP_ClrIPAddr()
#endif /* OPTION_TUNTAP_CLRIPADDR */


//
// TUNTAP_SetIPAddr
//

int             TUNTAP_SetIPAddr( char*  pszNetDevName,
                                  char*  pszIPAddr )
{
#if defined (__FreeBSD__)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, pszNetDevName, IFNAMSIZ);
    
    // Get current flags - directly because hercifc doesn't know GET ioctls
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        WRMSG( HHC99998, "E", "TUNTAP_SetIPAddr: ioctl(sock, SIOCGIFFLAGS, &ifr) failed:", strerror( errno ));
        close(sock);
        return -1;
    }
    
    // Set IFF_UP and IFF_POINTOPOINT flags
    ifr.ifr_flags |= ( IFF_UP | IFF_POINTOPOINT);
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        WRMSG( HHC99998, "E", "TUNTAP_SetIPAddr: ioctl(sock, SIOCSIFFLAGS, &ifr) failed:", strerror( errno ));
        close(sock);
        return -1;
    }
    
    // Prepwork 
    struct in_aliasreq ifra;
    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, pszNetDevName, IFNAMSIZ);

    // Local address
    memset(&ifra.ifra_addr, 0, sizeof(ifra.ifra_addr));
    ifra.ifra_addr.sin_len = sizeof(struct sockaddr_in);
    ifra.ifra_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, pszIPAddr, &ifra.ifra_addr.sin_addr) != 1) {
        WRMSG( HHC99998, "E", "TUNTAP_SetIPAddr: inet_pton(AF_INET, pszIPAddr, &ifra.ifra_addr.sin_addr) failed:", strerror( errno ));
        return -1;
    }
    
    // Dest address 
    // We don't know the dest address in this function but the ioctl needs it to set up the device.
    // We have to set a dummy address which will be overwritten later in TUNTAP_SetDestAddr.
    // Using an address in the 192.0.2.0/24 range which according to RFC 5737 cannot be used for anything but documentation purposes.
    char* dummy_ip = "192.0.2.1"; // RFC 5737
    memset(&ifra.ifra_dstaddr, 0, sizeof(ifra.ifra_dstaddr));
    ifra.ifra_dstaddr.sin_len = sizeof(struct sockaddr_in);
    ifra.ifra_dstaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, dummy_ip, &ifra.ifra_dstaddr.sin_addr) != 1) {
        WRMSG( HHC99998, "E", "TUNTAP_SetIPAddr: inet_pton(AF_INET, dummy_ip, &ifra.ifra_dstaddr.sin_addr) failed:", strerror( errno ));
        return -1;
    }

    // Netmask
    // We don't know the netmask either.
    // Set it to 255.255.255.255 until it gets overwritten later in TUNTAP_SetNetmask.
    memset(&ifra.ifra_mask, 0, sizeof(ifra.ifra_mask));
    ifra.ifra_mask.sin_len = sizeof(struct sockaddr_in);
    ifra.ifra_mask.sin_family = AF_INET;
    if (inet_pton(AF_INET, "255.255.255.255", &ifra.ifra_mask.sin_addr) != 1) {
        WRMSG( HHC99998, "E", "TUNTAP_SetIPAddr: inet_pton(AF_INET, \"255.255.255.255\", &ifra.ifra_mask.sin_addr) failed:", strerror( errno ));
        return -1;
    }

    if (ioctl(sock, SIOCAIFADDR, &ifra) < 0) { 
        WRMSG( HHC99998, "E", "TUNTAP_SetIPAddr: ioctl(sock, SIOCAIFADDR, &ifra) failed:", strerror( errno ));
        close(sock);
        return -1;
    }

    close(sock);
    return 0;
#else 

    struct hifr         hifr;
    struct sockaddr_in* sin;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    sin = (struct sockaddr_in*)&hifr.hifr_addr;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );
    hifr.hifr_afamily = AF_INET;

    if( !pszIPAddr  ||
        !inet_aton( pszIPAddr, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid IP %s"
        WRMSG( HHC00141, "E", pszNetDevName, !pszIPAddr ? "NULL" : pszIPAddr );
        return -1;
    }

    return TUNTAP_IOCtl( 0, SIOCSIFADDR, (char*)&hifr );
#endif // defined (__FreeBSD__)
}   // End of function  TUNTAP_SetIPAddr()


//
// TUNTAP_SetDestAddr
//

int             TUNTAP_SetDestAddr( char*  pszNetDevName,
                                    char*  pszDestAddr )
{
#if defined (__FreeBSD__)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct ifreq ifr;

    // Fetch existing IP
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, pszNetDevName, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        WRMSG( HHC99998, "E", "TUNTAP_SetDestAddr: ioctl(sock, SIOCGIFADDR, &ifr) failed:", strerror( errno ));
        close(sock);
        return -1;
    }
    struct in_addr existing_ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;

    // Fetch existing netmask
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, pszNetDevName, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0) {
        WRMSG( HHC99998, "E", "TUNTAP_SetDestAddr: ioctl(sock, SIOCGIFNETMASK, &ifr) failed:", strerror( errno ));
        close(sock);
        return -1;
        }
    struct in_addr existing_netmask = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;

    // Create request
    struct in_aliasreq ifra;
    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, pszNetDevName, IFNAMSIZ);

    struct sockaddr_in* tmp_sin;

    // Source address
    tmp_sin = (struct sockaddr_in*)&ifra.ifra_addr;
    tmp_sin->sin_len = sizeof(struct sockaddr_in);
    tmp_sin->sin_family = AF_INET;
    tmp_sin->sin_addr = existing_ip;    

    // Destination address
    // We have the 'real' destination IP now, so we can overwrite the RFC 5737 IP.
    tmp_sin = (struct sockaddr_in*)&ifra.ifra_dstaddr;
    tmp_sin->sin_len = sizeof(struct sockaddr_in);
    tmp_sin->sin_family = AF_INET;
    if (inet_pton(AF_INET, pszDestAddr, &tmp_sin->sin_addr) != 1) {
        WRMSG( HHC99998, "E", "TUNTAP_SetDestAddr: inet_pton(AF_INET, pszDestAddr, &tmp_sin->sin_addr) failed:", strerror( errno ));    
        close(sock);
        return 1;
    }

    // Netmask
    tmp_sin = (struct sockaddr_in*)&ifra.ifra_mask;
    tmp_sin->sin_len = sizeof(struct sockaddr_in);
    tmp_sin->sin_family = AF_INET;
    tmp_sin->sin_addr = existing_netmask;
    
    if (ioctl(sock, SIOCAIFADDR, &ifra) < 0) { 
        WRMSG( HHC99998, "E", "TUNTAP_SetDestAddr: ioctl(sock, SIOCAIFADDR, &ifra) failed:", strerror( errno ));
        close(sock);
        return -1;
    }

    close(sock);
    return 0;
#else 

    struct hifr         hifr;
    struct sockaddr_in* sin;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    sin = (struct sockaddr_in*)&hifr.hifr_addr;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( !pszDestAddr  ||
        !inet_aton( pszDestAddr, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid destination address %s"
        WRMSG( HHC00142, "E", pszNetDevName, !pszDestAddr ? "NULL" : pszDestAddr );
            return -1;
    }

    return TUNTAP_IOCtl( 0, SIOCSIFDSTADDR, (char*)&hifr );
#endif // defined (__FreeBSD__)
}   // End of function  TUNTAP_SetDestAddr()


//
// TUNTAP_SetNetMask
//
#ifdef OPTION_TUNTAP_SETNETMASK
int           TUNTAP_SetNetMask( char*  pszNetDevName,
                                 char*  pszNetMask )
{
#if defined (__FreeBSD__)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct ifreq ifr;

    // Fetch existing IP, set in TUNTAP_SetIPAddr()
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, pszNetDevName, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        WRMSG( HHC99998, "E", "TUNTAP_SetNetMask: ioctl(sock, SIOCGIFADDR, &ifr) failed:", strerror( errno ));
        close(sock);
        return -1;
    }
    struct in_addr existing_src_ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;

    // Fetch existing destination IP - proper one or dummy 
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, pszNetDevName, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFDSTADDR, &ifr) < 0) {
        WRMSG( HHC99998, "E", "TUNTAP_SetNetMask: ioctl(sock, SIOCGIFDSTADDR, &ifr) failed:", strerror( errno ));
        close(sock);
        return -1;
        }
    struct in_addr existing_dst_ip = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;

    // Create request
    struct in_aliasreq ifra;
    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, pszNetDevName, IFNAMSIZ);

    struct sockaddr_in* tmp_sin;

    // Source address
    tmp_sin = (struct sockaddr_in*)&ifra.ifra_addr;
    tmp_sin->sin_len = sizeof(struct sockaddr_in);
    tmp_sin->sin_family = AF_INET;
    tmp_sin->sin_addr = existing_src_ip;    

    // Destination address
    tmp_sin = (struct sockaddr_in*)&ifra.ifra_dstaddr;
    tmp_sin->sin_len = sizeof(struct sockaddr_in);
    tmp_sin->sin_family = AF_INET;
    tmp_sin->sin_addr = existing_dst_ip;    

    // Netmask
    // We have the 'real' netmask now, so we can overwrite the default 255.255.255.255 netmask.
    tmp_sin = (struct sockaddr_in*)&ifra.ifra_mask;
    tmp_sin->sin_len = sizeof(struct sockaddr_in);
    tmp_sin->sin_family = AF_INET;
    if (inet_pton(AF_INET, pszNetMask, &tmp_sin->sin_addr) != 1) {
        WRMSG( HHC99998, "E", "TUNTAP_SetNetMask: inet_pton(AF_INET, pszNetMask, &tmp_sin->sin_addr) failed:", strerror( errno ));    
        close(sock);
        return 1;
    }
    
    if (ioctl(sock, SIOCAIFADDR, &ifra) < 0) { 
        WRMSG( HHC99998, "E", "TUNTAP_SetNetMask: ioctl(sock, SIOCAIFADDR, &ifra) failed:", strerror( errno ));
        close(sock);
        return -1;
    }

    close(sock);
    return 0;
#else

    struct hifr         hifr;
    struct sockaddr_in* sin;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    sin = (struct sockaddr_in*)&hifr.hifr_netmask;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( !pszNetMask  ||
        !inet_aton( pszNetMask, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid net mask %s"
        WRMSG( HHC00143, "E", pszNetDevName, !pszNetMask ? "NULL" : pszNetMask );
            return -1;
    }

    return TUNTAP_IOCtl( 0, SIOCSIFNETMASK, (char*)&hifr );
#endif // defined (__FreeBSD__)
}   // End of function  TUNTAP_SetNetMask()
#endif // OPTION_TUNTAP_SETNETMASK


//
// TUNTAP_SetBCastAddr
//
#ifdef OPTION_TUNTAP_SETBRDADDR
int           TUNTAP_SetBCastAddr( char*  pszNetDevName,
                                   char*  pszBCastAddr )
{
    struct hifr         hifr;
    struct sockaddr_in* sin;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    sin = (struct sockaddr_in*)&hifr.hifr_broadaddr;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( !pszBCastAddr  ||
        !inet_aton( pszBCastAddr, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid broadcast address %s"
        WRMSG( HHC00155, "E", pszNetDevName, !pszBCastAddr ? "NULL" : pszBCastAddr );
            return -1;
    }

    return TUNTAP_IOCtl( 0, SIOCSIFBRDADDR, (char*)&hifr );
}   // End of function  TUNTAP_SetBCastAddr()
#endif // OPTION_TUNTAP_SETBRDADDR


//
// TUNTAP_SetIPAddr6
//
#if defined(ENABLE_IPV6)
int             TUNTAP_SetIPAddr6( char*  pszNetDevName,
                                   char*  pszIPAddr6,
                                   char*  pszPrefixSize6 )
{
#if defined (__FreeBSD__)
    // Soon, soon 
    WRMSG( HHC99999, "I", "TUNTAP_SetIPAddr6: IPv6 has not been implemented yet." );
    return 0;
#else 

    struct hifr         hifr;
    int                 iPfxSiz;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    if( !pszIPAddr6 )
    {
        // "Net device %s: Invalid IP %s"
        WRMSG( HHC00141, "E", pszNetDevName, "NULL" );
        return -1;
    }

    if( !pszPrefixSize6 )
    {
        // "Net device %s: Invalid prefix length %s"
        WRMSG( HHC00153, "E", pszNetDevName, "NULL" );
        return -1;
    }

    iPfxSiz = atoi( pszPrefixSize6 );
    if( iPfxSiz < 0 || iPfxSiz > 128 )
    {
        // "Net device %s: Invalid prefix length %s"
        WRMSG( HHC00153, "E", pszNetDevName, pszPrefixSize6 );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );

    if( hinet_pton( AF_INET6, pszIPAddr6, &hifr.hifr6_addr ) != 1 )
    {
        // "Net device %s: Invalid IP %s"
        WRMSG( HHC00141, "E", pszNetDevName, pszIPAddr6 );
        return -1;
    }

    hifr.hifr6_prefixlen = iPfxSiz;
    hifr.hifr6_ifindex = hif_nametoindex( pszNetDevName );
    hifr.hifr_afamily = AF_INET6;

    return TUNTAP_IOCtl( 0, SIOCSIFADDR, (char*)&hifr );
#endif // defined (__FreeBSD__)
}   // End of function  TUNTAP_SetIPAddr6()
#endif /* defined(ENABLE_IPV6) */


//
// TUNTAP_GetMTU
//
int             TUNTAP_GetMTU( char*   pszNetDevName,
                               char**  ppszMTU )
{
    struct hifr         hifr;
    int                 rc;
    char                szMTU[8] = {0};

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    if( !ppszMTU )
    {
        // HHC00136 "Error in function %s: %s"
        WRMSG( HHC00136, "E", "TUNTAP_GetMTU", "Invalid parameters" );
        return -1;
    }

    *ppszMTU = NULL;

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );

#if defined( OPTION_W32_CTCI )
    rc = TUNTAP_IOCtl( 0, SIOCGIFMTU, (char*)&hifr );
#else // (non-Win32 platforms)
    {
        int sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
        rc = ioctl( sockfd, SIOCGIFMTU, &hifr );
        close( sockfd );
    }
#endif
    if( rc < 0 )
    {
        // "Error in function %s: %s"
        WRMSG( HHC00136, "E", "TUNTAP_GetMTU", strerror( errno ));
        return -1;
    }

    MSGBUF( szMTU, "%u", hifr.hifr_mtu );
    if (!(*ppszMTU = strdup( szMTU )))
    {
        errno = ENOMEM;
        return -1;
    }

    return 0;
}   // End of function  TUNTAP_GetMTU()


//
// TUNTAP_SetMTU
//
int             TUNTAP_SetMTU( char*  pszNetDevName,
                               char*  pszMTU )
{
    struct hifr         hifr;
    int                 iMTU;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    if( !pszMTU  || !*pszMTU )
    {
        // "Net device %s: Invalid MTU %s"
        WRMSG( HHC00144, "E", pszNetDevName, pszMTU ? pszMTU : "NULL" );
        return -1;
    }

    iMTU = atoi( pszMTU );
    if( iMTU < 46 || iMTU > 65536 )
    {
        // "Net device %s: Invalid MTU %s"
        WRMSG( HHC00144, "E", pszNetDevName, pszMTU );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    hifr.hifr_mtu = iMTU;

    return TUNTAP_IOCtl( 0, SIOCSIFMTU, (char*)&hifr );
}   // End of function  TUNTAP_SetMTU()


//
// TUNTAP_GetMACAddr
//
int           TUNTAP_GetMACAddr( char*   pszNetDevName,
                                 char**  ppszMACAddr )
{
#if defined(OPTION_TUNTAP_GETMACADDR)
    struct hifr         hifr;
    struct sockaddr*    addr;
    int                 rc;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    if( !ppszMACAddr )
    {
        // "Error in function %s: %s"
        WRMSG( HHC00136, "E", "TUNTAP_GetMACAddr", "Invalid parameters" );
        return -1;
    }

    *ppszMACAddr = NULL;

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    addr = (struct sockaddr*)&hifr.hifr_hwaddr;
    addr->sa_family = 1;    // ARPHRD_ETHER

#if defined( OPTION_W32_CTCI )
    rc = TUNTAP_IOCtl( 0, SIOCGIFHWADDR, (char*)&hifr );
#else // (non-Win32 platforms)
    {
        int sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
        rc = ioctl( sockfd, SIOCGIFHWADDR, &hifr );
        close( sockfd );
    }
#endif
    if( rc < 0 )
    {
        // "Error in function %s: %s"
        WRMSG( HHC00136, "E", "TUNTAP_GetMACAddr", strerror( errno ));
        return -1;
    }

    return FormatMAC( ppszMACAddr, (BYTE*) addr->sa_data );
#else // defined(OPTION_TUNTAP_GETMACADDR)
    UNREFERENCED(pszNetDevName);
    UNREFERENCED(ppszMACAddr);
    // "Error in function %s: %s"
    WRMSG( HHC00136, "E", "TUNTAP_GetMACAddr", "Unsupported" );
    return -1; // (unsupported)
#endif // defined(OPTION_TUNTAP_GETMACADDR)
}   // End of function  TUNTAP_GetMACAddr()


//
// TUNTAP_SetMACAddr
//
#ifdef OPTION_TUNTAP_SETMACADDR
int           TUNTAP_SetMACAddr( char*  pszNetDevName,
                                 char*  pszMACAddr )
{
    struct hifr         hifr;
    struct sockaddr*    addr;
    MAC                 mac;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    if( !pszMACAddr || ParseMAC( pszMACAddr, mac ) != 0 )
    {
        // "Net device %s: Invalid MAC address %s"
        WRMSG( HHC00145, "E", pszNetDevName, pszMACAddr ? pszMACAddr : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    addr = (struct sockaddr*)&hifr.hifr_hwaddr;
    memcpy( addr->sa_data, mac, IFHWADDRLEN );
    addr->sa_family = 1;    // ARPHRD_ETHER

    return TUNTAP_IOCtl( 0, SIOCSIFHWADDR, (char*)&hifr );

}   // End of function  TUNTAP_SetMACAddr()
#endif // OPTION_TUNTAP_SETMACADDR


//
// TUNTAP_SetFlags
//

int             TUNTAP_SetFlags ( char*  pszNetDevName,
                                  int    iFlags )
{
    struct hifr         hifr;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    hifr.hifr_flags = iFlags;

    return TUNTAP_IOCtl( 0, SIOCSIFFLAGS, (char*)&hifr );
}   // End of function  TUNTAP_SetFlags()


//
// TUNTAP_GetFlags
//

int      TUNTAP_GetFlags ( char*  pszNetDevName,
                           int*   piFlags )
{
    struct hifr         hifr;
    struct sockaddr_in* sin;
    int                 rc;

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    memset( &hifr, 0, sizeof( struct hifr ) );
    STRLCPY( hifr.hifr_name, pszNetDevName );
    sin = (struct sockaddr_in*)&hifr.hifr_addr;
    sin->sin_family = AF_INET;

    // PROGRAMMING NOTE: hercifc can't "get" information,
    // only "set" it. Thus because we normally use hercifc
    // to issue ioctl codes to the interface (on non-Win32)
    // we bypass hercifc altogether and issue the ioctl
    // ourselves directly to the device itself, bypassing
    // hercifc completely. Note that for Win32 however,
    // 'TUNTAP_IOCtl' routes to a TunTap32.DLL call and
    // thus works just fine. We need special handling
    // only for non-Win32 platforms. - Fish

#if defined( OPTION_W32_CTCI )

    rc = TUNTAP_IOCtl( 0, SIOCGIFFLAGS, (char*)&hifr );

#else // (non-Win32 platforms)
    {
        int sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
        rc = ioctl( sockfd, SIOCGIFFLAGS, &hifr );
    }
#endif

    *piFlags = hifr.hifr_flags;

    return rc;
}   // End of function  TUNTAP_GetFlags()


//
// TUNTAP_AddRoute
//
#ifdef OPTION_TUNTAP_DELADD_ROUTES
int           TUNTAP_AddRoute( char*  pszNetDevName,
                               char*  pszDestAddr,
                               char*  pszNetMask,
                               char*  pszGWAddr,
                               int    iFlags )
{
    struct rtentry      rtentry;
    struct sockaddr_in* sin;

    memset( &rtentry, 0, sizeof( struct rtentry ) );

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    rtentry.rt_dev = pszNetDevName;

    sin = (struct sockaddr_in*)&rtentry.rt_dst;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( !pszDestAddr  ||
        !inet_aton( pszDestAddr, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid destination address %s"
        WRMSG( HHC00142, "E", pszNetDevName, pszDestAddr ? pszDestAddr : "NULL" );
        return -1;
    }

    sin = (struct sockaddr_in*)&rtentry.rt_genmask;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( !pszNetMask  ||
        !inet_aton( pszNetMask, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid net mask %s"
        WRMSG( HHC00143, "E", pszNetDevName, pszNetMask ? pszNetMask : "NULL");
        return -1;
    }

    sin = (struct sockaddr_in*)&rtentry.rt_gateway;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( pszGWAddr )
    {
        if( !inet_aton( pszGWAddr, &sin->sin_addr ) )
        {
            // "Net device %s: Invalid gateway address %s"
            WRMSG( HHC00146, "E", pszNetDevName, pszGWAddr );
            return -1;
        }
    }

    rtentry.rt_flags = iFlags;

    return TUNTAP_IOCtl( 0, SIOCADDRT, (char*)&rtentry );
}   // End of function  TUNTAP_AddRoute()
#endif // OPTION_TUNTAP_DELADD_ROUTES


//
// TUNTAP_DelRoute
//
#ifdef OPTION_TUNTAP_DELADD_ROUTES
int           TUNTAP_DelRoute( char*  pszNetDevName,
                               char*  pszDestAddr,
                               char*  pszNetMask,
                               char*  pszGWAddr,
                               int    iFlags )
{
    struct rtentry      rtentry;
    struct sockaddr_in* sin;

    memset( &rtentry, 0, sizeof( struct rtentry ) );

    if( !pszNetDevName || !*pszNetDevName )
    {
        // "Invalid net device name %s"
        WRMSG( HHC00140, "E", pszNetDevName ? pszNetDevName : "NULL" );
        return -1;
    }

    rtentry.rt_dev = pszNetDevName;

    sin = (struct sockaddr_in*)&rtentry.rt_dst;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( !pszDestAddr  ||
        !inet_aton( pszDestAddr, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid destination address %s"
        WRMSG( HHC00142, "E", pszNetDevName, pszDestAddr ? pszDestAddr : "NULL" );
        return -1;
    }

    sin = (struct sockaddr_in*)&rtentry.rt_genmask;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( !pszNetMask  ||
        !inet_aton( pszNetMask, &sin->sin_addr ) )
    {
        // "Net device %s: Invalid net mask %s"
        WRMSG( HHC00143, "E", pszNetDevName, pszNetMask ? pszNetMask : "NULL" );
        return -1;
    }

    sin = (struct sockaddr_in*)&rtentry.rt_gateway;
    sin->sin_family = AF_INET;
    set_sockaddr_in_sin_len( sin );

    if( pszGWAddr )
    {
        if( !inet_aton( pszGWAddr, &sin->sin_addr ) )
        {
            // "Net device %s: Invalid gateway address %s"
            WRMSG( HHC00146, "E", pszNetDevName, pszGWAddr );
            return -1;
        }
    }

    rtentry.rt_flags = iFlags;

    return TUNTAP_IOCtl( 0, SIOCDELRT, (char*)&rtentry );
}   // End of function  TUNTAP_DelRoute()
#endif // OPTION_TUNTAP_DELADD_ROUTES



#if defined( OPTION_W32_CTCI )    // CTCI-WIN v3.7.0 or greater
#if defined( TT32QIFDSTADDRS )    // CTCI-WIN v3.7.0 or greater
/*--------------------------------------------------------------------*/
/*  TT32_AddIPAddr -- add IPv4 address to the virtual interface       */
/*--------------------------------------------------------------------*/
int TT32_AddIPAddr( int fd, BYTE* ipv4addr )
{
    return TUNTAP_IOCtl( fd, TT32AIFDSTADDR, (char*)ipv4addr );
}
/*--------------------------------------------------------------------*/
/*  TT32_DelIPAddr -- delete IPv4 address from the virtual interface  */
/*--------------------------------------------------------------------*/
int TT32_DelIPAddr( int fd, BYTE* ipv4addr )
{
    return TUNTAP_IOCtl( fd, TT32DIFDSTADDR, (char*)ipv4addr );
}
/*--------------------------------------------------------------------*/
/*  TT32_QryIPAddrs -- query virtual interface IPv4 addresses         */
/*--------------------------------------------------------------------*/
int TT32_QryIPAddrs( int fd, BYTE* ipv4addrs )
{
    // PROGRAMMING NOTE: buffer should be 256 bytes minimum
    return TUNTAP_IOCtl( fd, TT32QIFDSTADDRS, (char*)ipv4addrs );
}
#endif // defined( TT32QIFDSTADDRS )  // CTCI-WIN v3.7.0 or greater
#endif // defined( OPTION_W32_CTCI )  // CTCI-WIN v3.7.0 or greater



#if !defined( OPTION_W32_CTCI )
// ====================================================================
// HercIFC Helper Functions
// ====================================================================

//
// IFC_IOCtl
//

static int      IFC_IOCtl( int fd, unsigned long int iRequest, char* argp )
{
    const char* pszCfgCmd;     // Interface config command
    CTLREQ      ctlreq;

#if defined(DEBUG) || defined(_DEBUG)
    char*       request_name;  // debugging: name of ioctl request
    char        unknown_request[] = "Unknown (0x00000000)";
#endif

    UNREFERENCED( fd );

    memset( &ctlreq, 0, CTLREQ_SIZE );

    ctlreq.iCtlOp = iRequest;

#if defined(DEBUG) || defined(_DEBUG)

    // Select string to represent ioctl request for debugging.

    switch (iRequest) {
#ifdef OPTION_TUNTAP_CLRIPADDR
    case              SIOCDIFADDR:
        request_name="SIOCDIFADDR"; break;
#endif
    case              SIOCSIFADDR:
        request_name="SIOCSIFADDR"; break;

    case              SIOCSIFDSTADDR:
        request_name="SIOCSIFDSTADDR"; break;

    case              SIOCSIFMTU:
        request_name="SIOCSIFMTU"; break;

    case              SIOCSIFFLAGS:
        request_name="SIOCSIFFLAGS"; break;

    case              SIOCGIFFLAGS:
        request_name="SIOCGIFFLAGS"; break;

#ifdef OPTION_TUNTAP_SETNETMASK
    case              SIOCSIFNETMASK:
        request_name="SIOCSIFNETMASK"; break;
#endif
#ifdef OPTION_TUNTAP_SETMACADDR
    case              SIOCSIFHWADDR:
        request_name="SIOCSIFHWADDR"; break;
#endif
#ifdef OPTION_TUNTAP_DELADD_ROUTES
    case              SIOCADDRT:
        request_name="SIOCADDRT"; break;

    case              SIOCDELRT:
        request_name="SIOCDELRT"; break;
#endif
    default:
        sprintf(unknown_request,"Unknown (0x%lx)",iRequest);
        request_name=unknown_request;
    }

#endif // defined(DEBUG) || defined(_DEBUG)

#ifdef OPTION_TUNTAP_DELADD_ROUTES
    if( iRequest == SIOCADDRT ||
        iRequest == SIOCDELRT )
    {
      strcpy( ctlreq.szIFName, ((struct rtentry*)argp)->rt_dev );
      memcpy( &ctlreq.iru.rtentry, argp, sizeof( struct rtentry ) );
      ((struct rtentry*)argp)->rt_dev = NULL;
    }
    else
#endif
    {
      memcpy( &ctlreq.iru.hifr, argp, sizeof( struct hifr ) );
    }

    if( ifc_fd[0] == -1 && ifc_fd[1] == -1 )
    {
        if( socketpair( AF_UNIX, SOCK_STREAM, 0, ifc_fd ) < 0 )
        {
            // "Error in function %s: %s"
            WRMSG( HHC00136, "E", "socketpair()", strerror( errno ) );
            return -1;
        }

        // Obtain the name of the interface config program or default
        if (!(pszCfgCmd = get_symbol( "HERCULES_IFC" )) || !*pszCfgCmd)
            pszCfgCmd = HERCIFC_CMD;

        // Fork a process to execute the hercifc
        ifc_pid = fork();

        if( ifc_pid < 0 )
        {
            // "Error in function %s: %s"
            WRMSG( HHC00136, "E", "fork()", strerror( errno ) );
            return -1;
        }

        // The child process executes the configuration command
        if( ifc_pid == 0 )
        {
            /* Close all file descriptors
             * (except ifc_fd[1] and STDOUT FILENO)
             * (otherwise some devices are never closed)
             * (ex: SCSI tape devices can never be re-opened)
            */
            struct rlimit rlim;
            int i;
            rlim_t file_limit;
            char msglvl[16] = {0};
            getrlimit(RLIMIT_NOFILE,&rlim);
            /* While Linux and Cygwin have limits of 1024 files by default,
             * Mac OS X does not - its default is -1, or completely unlimited.
             * The following hack is to defend against trying to close 2
             * billion files. -- JRM */
            file_limit=rlim.rlim_max;
            file_limit=(file_limit>1024)?1024:file_limit;

            TRACE(MSG(HHC00148, "I", file_limit));

            for(i=0;(unsigned int)i<file_limit;i++)
            {
                if(i!=ifc_fd[1] && i!=STDOUT_FILENO)
                {
                    close(i);
                }
            }
            /* end close spurious FDs */
            dup2( ifc_fd[1], STDIN_FILENO  );
            dup2( STDOUT_FILENO, STDERR_FILENO );

            // Execute the interface configuration command
            MSGBUF( msglvl, "%d", sysblk.msglvl );
            (void)execlp( pszCfgCmd, pszCfgCmd, msglvl, query_codepage(), NULL );

            // The exec function returns only if unsuccessful
            // "Error in function %s: %s"
            WRMSG( HHC00136, "E", "execlp()", strerror( errno ) );
            exit( 127 );
        }

        /* Terminate TunTap on shutdown */
        hdl_addshut( "tuntap_term", tuntap_term, NULL );
    }

    // Populate some common fields
    ctlreq.iType = 1;

    TRACE(MSG(HHC00149,"I",request_name,ifc_fd[0],ifc_fd[1]));

    VERIFY(CTLREQ_SIZE == write( ifc_fd[0], &ctlreq, CTLREQ_SIZE ));

    return 0;
}   // End of function  IFC_IOCtl()

#endif // !defined( OPTION_W32_CTCI )


// The following functions used by Win32 *and* NON-Win32 platforms...

/*--------------------------------------------------------------------*/
/*                  build_herc_iface_mac                              */
/*--------------------------------------------------------------------*/
/* This function generates a default MAC address. The generated MAC   */
/* address is in the form 02:00:5E:xx:xx:xx where 'xx' are either     */
/* randomly generated values or else based on the passed IP address.  */
/* To build a MAC using random values pass NULL for the IP addr ptr.  */
/*--------------------------------------------------------------------*/
void build_herc_iface_mac ( BYTE* out_mac, const BYTE* in_ip )
{
    // Routine to build a default MAC address for
    // CTCI/LCS/QETH device virtual interfaces...

BYTE ip[4];

    if (!out_mac)
    {
        ASSERT( FALSE );
        return;             // (nothing for us to do!)
    }

    // We base our default MAC address on the last three bytes
    // of the IPv4 address (see further below). If it doesn't
    // have an IP address assigned to it yet however (in_ip is
    // NULL), then we temporarily generate a random IP address
    // only for the purpose of generating a default/random MAC.

    if (in_ip)
        memcpy( ip, in_ip, 4 );   // (use the passed value)
    else                          // (else create temporary)
    {
        int i; for(i=0; i < 4; i++)
            ip[i] = (BYTE)(rand() % 256);
    }

#if defined( OPTION_W32_CTCI )

    // We prefer to let TunTap32 do it for us (since IT'S the one
    // that decides what it should really be) but if they're using
    // an older version of TunTap32 that doesn't have the function
    // then we'll do it ourselves just like before...

    if (tt32_build_herc_iface_mac( out_mac, ip ))
    {
        out_mac[0] &= ~0x01;    // (ensure broadcast bit off)
        out_mac[0] |=  0x02;    // (set local assignment bit)
        return;
    }

#endif

    // Build a default MAC addr based on the guest (destination) ip
    // address so as to effectively *UNOFFICIALLY* assign ourselves
    // the following Ethernet address block:

    /* (from: http://www.iana.org/assignments/ethernet-numbers)
       (only the first 2 and last 2 paragraphs are of interest)

        IANA ETHERNET ADDRESS BLOCK - UNICAST USE

        The IANA owns an Ethernet address block which may be used for
        unicast address asignments or other special purposes.

        The IANA may assign unicast global IEEE 802 MAC address from it's
        assigned OUI (00-00-5E) for use in IETF standard track protocols.  The
        intended usage is for dynamic mapping between IP addresses and IEEE
        802 MAC addresses.  These IEEE 802 MAC addresses are not to be
        permanently assigned to any hardware interface, nor is this a
        substitute for a network equipment supplier getting its own OUI.

        ... (snipped)

        Using this representation, the range of Internet Unicast addresses is:

               00-00-5E-00-00-00  to  00-00-5E-FF-FF-FF  in hex, ...

        ... (snipped)

        The low order 24 bits of these unicast addresses are assigned as
        follows:

        Dotted Decimal          Description                     Reference
        ----------------------- ------------------------------- ---------
        000.000.000-000.000.255 Reserved                        [IANA]
        000.001.000-000.001.255 Virual Router Redundancy (VRRP) [Hinden]
        000.002.000-127.255.255 Reserved                        [IANA]
        128.000.000-255.255.255 Hercules TUNTAP (CTCI)          [Fish] (*UNOFFICIAL*)
    */

    // Here's what we're basically doing:

    //    00-00-5E-00-00-00  to  00-00-5E-00-00-FF  =  'Reserved' by IANA
    //    00-00-5E-00-01-00  to  00-00-5E-00-01-FF  =  'VRRP' by Hinden
    //    00-00-5E-00-02-00  to  00-00-5E-7F-FF-FF  =  (unassigned)
    //    00-00-5E-80-00-00  to  00-00-5E-FF-FF-FF  =  'Hercules' by Fish (*UNOFFICIAL*)

    //    00-00-5E-00-00-00   (starting value)
    //    00-00-5E-ip-ip-ip   (move in low-order 3 bytes of destination IP address)
    //    00-00-5E-8p-ip-ip   ('OR' on the x'80' high-order bit)

    out_mac[0] = 0x02;          // (set local assignment bit)
    out_mac[1] = 0x00;
    out_mac[2] = 0x5E;
    out_mac[3] = ip[1] | 0x80;  // (Hercules *UNOFFICIAL* range)
    out_mac[4] = ip[2];
    out_mac[5] = ip[3];
}


/* ------------------------------------------------------------------ */
/* ParseMAC                                                           */
/* ------------------------------------------------------------------ */
//
// Parse a string containing a MAC (hardware) address and return the
// binary equivalent.
//
// Input:
//      pszMACAddr   Pointer to string containing a MAC Address in the
//                   format "xx-xx-xx-xx-xx-xx" or "xx:xx:xx:xx:xx:xx".
//
// Output:
//      pbMACAddr    Pointer to a BYTE array to receive the MAC Address
//                   that MUST be at least sizeof(MAC) bytes long.
//
// Returns:
//      0 on success, -1 otherwise
//

int  ParseMAC( char* pszMACAddr, BYTE* pbMACAddr )
{
    char    work[((sizeof(MAC)*3)-0)];
    BYTE    sep;
    int       x;
    unsigned  i;

    if (strlen(pszMACAddr) != ((sizeof(MAC)*3)-1)
        || (sizeof(MAC) > 1 &&
            *(pszMACAddr+2) != '-' &&
            *(pszMACAddr+2) != ':')
    )
    {
        errno = EINVAL;
        return -1;
    }

    strncpy(work,pszMACAddr,((sizeof(MAC)*3)-1));
    work[((sizeof(MAC)*3)-1)] = sep = *(pszMACAddr+2);

    for (i=0; i < sizeof(MAC); i++)
    {
        if
        (0
            || !isxdigit((unsigned char)work[(i*3)+0])
            || !isxdigit((unsigned char)work[(i*3)+1])
            ||  sep  !=  work[(i*3)+2]
        )
        {
            errno = EINVAL;
            return -1;
        }

        work[(i*3)+2] = 0;
        sscanf(&work[(i*3)+0],"%x",&x);
        *(pbMACAddr+i) = x;
    }

    return 0;
}


/* ------------------------------------------------------------------ */
/* FormatMAC                                                          */
/* ------------------------------------------------------------------ */
//
// Format a binary hardware MAC address into a string value.
//
// Input:
//      mac          Pointer to BYTE array containing the MAC Address
//                   that MUST be at least IFHWADDRLEN bytes long.
//
// Output:
//      ppszMACAddr  Address of a char pointer that will be updated
//                   with the malloc'ed string address of the formatted
//                   MAC Address in the format "xx:xx:xx:xx:xx:xx".
//                   It is the caller's responsibility to free() it.
//
// Returns:
//      0 on success, -1 otherwise
//
int FormatMAC( char** ppszMACAddr, BYTE* mac )
{
    char szMAC[3 * IFHWADDRLEN] = {0};

    if (!ppszMACAddr || !mac)
    {
        errno = EINVAL;
        return -1;
    }

    MSGBUF( szMAC, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );

    if (!(*ppszMACAddr = strdup( szMAC )))
    {
        errno = ENOMEM;
        return -1;
    }

    return 0;
}


/* ------------------------------------------------------------------ */
/* packet_trace                                                       */
/* ------------------------------------------------------------------ */
void packet_trace( BYTE* pAddr, int iLen, BYTE bDir )
{
    net_data_trace( NULL, pAddr, iLen, bDir, 'I', "packet trace", 0 );
}


/* ------------------------------------------------------------------ */
/* net_data_trace                                                     */
/* ------------------------------------------------------------------ */
void net_data_trace( DEVBLK* pDEVBLK, BYTE* pAddr, int iLen, BYTE bDir, BYTE bSev, char* pWhat, U32 uOpt )
{
    int           offset;
    unsigned int  i;
    u_char        c = '\0';
    u_char        e = '\0';
    char          print_dev[16];
    char          print_ascii[17];
    char          print_ebcdic[17];
    char          print_line[64];
    char          tmp[32];

    UNREFERENCED( uOpt );


    if (pDEVBLK)
        MSGBUF(print_dev, "%1d:%04X %s", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,  pDEVBLK->typname );
    else
        MSGBUF(print_dev, "CTC" );

    for (offset = 0; offset < iLen; )
    {
        memset( print_ascii, ' ', sizeof(print_ascii)-1 );    /* set to spaces */
        print_ascii[sizeof(print_ascii)-1] = '\0';            /* with null termination */
        memset( print_ebcdic, ' ', sizeof(print_ebcdic)-1 );  /* set to spaces */
        print_ebcdic[sizeof(print_ebcdic)-1] = '\0';          /* with null termination */
        memset( print_line, 0, sizeof( print_line ) );

        MSGBUF(print_line, "+%4.4X%c ", offset, bDir );

        for( i = 0; i < 16; i++ )
        {
            c = *pAddr++;

            if( offset < iLen )
            {
                MSGBUF(tmp,"%2.2X", c );
                tmp[sizeof(tmp)-1] = '\0';
                STRLCAT( print_line, tmp );

                print_ebcdic[i] = print_ascii[i] = '.';
                e = guest_to_host( c );

                if( isprint( (unsigned char)e ) )
                    print_ebcdic[i] = e;
                if( isprint( (unsigned char)c ) )
                    print_ascii[i] = c;
            }
            else
            {
                STRLCAT( print_line, "  " );
            }

            offset++;
            if( ( offset & 3 ) == 0 )
            {
                STRLCAT( print_line, " " );
            }
        }

        // HHC00979 "%s: %s: %s %s %s"
        if( bSev == 'D' ) {
          WRMSG(HHC00979, "D", print_dev, pWhat, print_line, print_ascii, print_ebcdic );
        } else {
          WRMSG(HHC00979, "I", print_dev, pWhat, print_line, print_ascii, print_ebcdic );
        }
    }
}

#endif /*  !defined(__SOLARIS__)  jbs*/
