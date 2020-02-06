/* TUNTAP.H    (C) Copyright James A. Pierson, 2002-2012             */
/*             (C) Copyright "Fish" (David B. Trout), 2002-2012      */
/*              Hercules - TUN/TAP Abstraction Layer                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __TUNTAP_H_
#define __TUNTAP_H_

#include "hercules.h"

#if !defined( OPTION_W32_CTCI )
  #include "hifr.h"             // struct in6_ifreq, struct hifr
#else
  #include "tt32if.h"           // struct if_req
  #include "hifr.h"             // struct in6_ifreq, struct hifr
  #include "tt32api.h"          // struct tt32ctl
#endif

#if !defined( OPTION_W32_CTCI )

// ====================================================================
//  TunTap ioctl codes and Standard and TUNSETIFF ifr interface flags
// ====================================================================

#if defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )
  #define  HERCTUN_DEV  "/dev/tun"     // Default TUN/TAP char dev
#else
  #define  HERCTUN_DEV  "/dev/net/tun"  // Default TUN/TAP char dev
#endif

#if !defined(HAVE_LINUX_IF_TUN_H)

  /* tuntap ioctl defines */
  #define TUNSETNOCSUM    _IOW('T', 200, int)
  #define TUNSETDEBUG     _IOW('T', 201, int)
  #define TUNSETIFF       _IOW('T', 202, int)
  #define TUNSETPERSIST   _IOW('T', 203, int)
  #define TUNSETOWNER     _IOW('T', 204, int)

  /* tuntap TUNSETIFF ifr flags */
  #define IFF_TUN         0x0001
  #define IFF_TAP         0x0002
  #define IFF_NO_PI       0x1000    /* Don't provide packet info     */
  #define IFF_ONE_QUEUE   0x2000    /* Use only one packet queue     */
#endif // !defined(HAVE_LINUX_IF_TUN_H)

  /* Passed  from ctc_ctci to tuntap to indicate that the interface  */
  /* is configured and that only the interface name is to be set.    */
  #define IFF_NO_HERCIFC  0x10000

#if !defined(HAVE_NET_IF_H)
  /* Standard interface flags. */
  #define IFF_UP          0x1       /* interface is up               */
  #define IFF_BROADCAST   0x2       /* broadcast address valid       */
  #define IFF_DEBUG       0x4       /* Turn on debugging.            */
  #define IFF_LOOPBACK    0x8       /* is a loopback net             */
  #define IFF_NOTRAILERS  0x20      /* avoid use of trailers         */
  #define IFF_RUNNING     0x40      /* resources allocated           */
  #define IFF_PROMISC     0x100     /* receive all packets           */
  #define IFF_MULTICAST   0x1000    /* Supports multicast            */
#elif !defined(IFF_PROMISC)
  #define IFF_PROMISC     0x100     /* required by qeth.h            */
#endif // !defined(HAVE_NET_IF_H)

#endif // !defined( OPTION_W32_CTCI )

// ====================================================================
//                      Declarations
// ====================================================================

//
// Create TUN/TAP Interface
//
extern int      TUNTAP_CreateInterface  ( char*   pszTUNDevice,
                                          int     iFlags,
                                          int*    pfd,
                                          char*   pszNetDevName );
//
// TUNTAP_CreateInterface flag: open as SOCKET (CTCI-WIN v3.3+ only)
//
#if defined( OPTION_W32_CTCI )
  #define IFF_OSOCK             _O_TT32NOTIFY
#else
  #define IFF_OSOCK             0
#endif

//
// Configure TUN/TAP Interface
//

#ifdef   OPTION_TUNTAP_CLRIPADDR
extern int      TUNTAP_ClrIPAddr        ( char*   pszNetDevName );
#endif
extern int      TUNTAP_SetIPAddr        ( char*   pszNetDevName,
                                          char*   pszIPAddr );
extern int      TUNTAP_SetDestAddr      ( char*   pszNetDevName,
                                          char*   pszDestAddr );
#ifdef OPTION_TUNTAP_SETNETMASK
extern int      TUNTAP_SetNetMask       ( char*   pszNetDevName,
                                          char*   pszNetMask );
#endif

#ifdef OPTION_TUNTAP_SETBRDADDR
extern int      TUNTAP_SetBCastAddr     ( char*   pszNetDevName,
                                          char*   pszBCastAddr );
#endif

extern int      TUNTAP_SetIPAddr6       ( char*   pszNetDevName,
                                          char*   pszIPAddr6,
                                          char*   pszPrefixLen6 );

extern int      TUNTAP_GetMTU           ( char*   pszNetDevName,
                                          char**  ppszMTU );

extern int      TUNTAP_SetMTU           ( char*   pszNetDevName,
                                          char*   pszMTU );

extern int      TUNTAP_GetMACAddr       ( char*   pszNetDevName,
                                          char**  ppszMACAddr );
#ifdef OPTION_TUNTAP_SETMACADDR
extern int      TUNTAP_SetMACAddr       ( char*   pszNetDevName,
                                          char*   pszMACAddr );
#endif
extern int      TUNTAP_SetFlags         ( char*   pszNetDevName,
                                          int     iFlags );

extern int      TUNTAP_GetFlags         ( char*   pszNetDevName,
                                          int*    piFlags );
#ifdef OPTION_TUNTAP_DELADD_ROUTES
extern int      TUNTAP_AddRoute         ( char*   pszNetDevName,
                                          char*   pszDestAddr,
                                          char*   pszNetMask,
                                          char*   pszGWAddr,
                                          int     iFlags );
extern int      TUNTAP_DelRoute         ( char*   pszNetDevName,
                                          char*   pszDestAddr,
                                          char*   pszNetMask,
                                          char*   pszGWAddr,
                                          int     iFlags );
#endif

#if defined( OPTION_W32_CTCI )   // CTCI-WIN v3.7.0 or greater
  #if defined( TT32QIFDSTADDRS ) // CTCI-WIN v3.7.0 or greater
    extern int TT32_AddIPAddr ( int fd, BYTE* ipv4addr  );
    extern int TT32_DelIPAddr ( int fd, BYTE* ipv4addr  );
    extern int TT32_QryIPAddrs( int fd, BYTE* ipv4addrs ); // >= 256 byte buffer
  #else
    static inline int TT32_AddIPAddr ( int fd, BYTE* ipv4addr )
    { UNREFERENCED( fd ); UNREFERENCED( ipv4addr ); return -1; }
    static inline int TT32_DelIPAddr ( int fd, BYTE* ipv4addr  )
    { UNREFERENCED( fd ); UNREFERENCED( ipv4addr ); return -1; }
    static inline int TT32_QryIPAddrs( int fd, BYTE* ipv4addrs )
    { UNREFERENCED( fd ); UNREFERENCED( ipv4addrs ); return -1; }
  #endif
#endif

// (functions used by *BOTH* Win32 *and* NON-Win32 platforms)

extern void build_herc_iface_mac ( BYTE* out_mac, const BYTE* in_ip );
extern int  ParseMAC( char* pszMACAddr, BYTE* pbMACAddr );
extern int  FormatMAC( char** ppszMACAddr, BYTE* mac );
extern void packet_trace( BYTE *addr, int len, BYTE dir );
extern void net_data_trace( DEVBLK* dev, BYTE* addr, int len, BYTE dir, BYTE sev, char* what, U32 opt );

// ====================================================================
//                      Helper Macros
// ====================================================================

#if defined( HAVE_STRUCT_SOCKADDR_IN_SIN_LEN )
  #define set_sockaddr_in_sin_len( sockaddr_in_ptr ) \
    (sockaddr_in_ptr)->sin_len = sizeof( struct sockaddr_in )
#else
  #define set_sockaddr_in_sin_len( sockaddr_in_ptr )
#endif

#if defined( OPTION_W32_CTCI )
  #define TUNTAP_Open           tt32_open
  #define TUNTAP_Close          tt32_close
  #define TUNTAP_Read           tt32_read
  #define TUNTAP_Write          tt32_write
  #define TUNTAP_BegMWrite(f,n) tt32_beg_write_multi(f,n)
  #define TUNTAP_EndMWrite(f)   tt32_end_write_multi(f)
  #define TUNTAP_IOCtl          tt32_ioctl
#else // !defined( OPTION_W32_CTCI )
  #define TUNTAP_Open           open
  #define TUNTAP_Close          close
  #define TUNTAP_Read           read
  #define TUNTAP_Write          write
  #define TUNTAP_BegMWrite(f,n)
  #define TUNTAP_EndMWrite(f)
  #define TUNTAP_IOCtl          ioctl
#endif // defined( OPTION_W32_CTCI )

#endif // __TUNTAP_H_
