/* HIFR.H       (C) Copyright Roger Bowler, 2013                     */
/*              (C) Copyright Ian Shorter, 2013                      */
/*              Hercules IPV6 struct hifr/ifreq/in6_ifreq support    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __HIFR_H_
#define __HIFR_H_

//---------------------------------------------------------------------
// The 'ifreq' interface request structure
//---------------------------------------------------------------------

#if !defined(OPTION_W32_CTCI) && !defined(HAVE_LINUX_IF_TUN_H) && !defined(HAVE_NET_IF_H)

  struct ifreq
  {
    union
    {
      char ifrn_name[IFNAMSIZ];         // (interface name)
    }
    ifr_ifrn;

    union
    {
      struct sockaddr ifru_addr;        // (IP address)
      struct sockaddr ifru_netmask;     // (network mask)
      struct sockaddr ifru_hwaddr;      // (MAC address)
      short int       ifru_flags;       // (flags)
      int             ifru_mtu;         // (maximum transmission unit)
    }
    ifr_ifru;
  };
  typedef struct ifreq ifreq;

  #define  ifr_name      ifr_ifrn.ifrn_name
  #define  ifr_hwaddr    ifr_ifru.ifru_hwaddr
  #define  ifr_addr      ifr_ifru.ifru_addr
  #define  ifr_netmask   ifr_ifru.ifru_netmask
  #define  ifr_flags     ifr_ifru.ifru_flags
  #define  ifr_mtu       ifr_ifru.ifru_mtu

#endif // !defined(OPTION_W32_CTCI) && !defined(HAVE_LINUX_IF_TUN_H) && !defined(HAVE_NET_IF_H)

//---------------------------------------------------------------------
// The 'in6_ifreq' IPv6 interface request structure
//---------------------------------------------------------------------

#if defined(ENABLE_IPV6)
  #if defined(HAVE_IN6_IFREQ_IFR6_ADDR)
    #if defined(HAVE_LINUX_IPV6_H)
      #include <linux/ipv6.h>
    #elif defined(HAVE_NETINET6_IN6_VAR_H)
      #include <netinet6/in6_var.h>
    #else
      #error HAVE_IN6_IFREQ_IFR6_ADDR is defined but which header to include is undefined!
    #endif
  #else /*!defined(HAVE_IN6_IFREQ_IFR6_ADDR)*/
    struct in6_ifreq {
        struct in6_addr  ifr6_addr;
        U32              ifr6_prefixlen;
        int              ifr6_ifindex;
    };
  #endif /*defined(HAVE_IN6_IFREQ_IFR6_ADDR)*/
#endif /* defined(ENABLE_IPV6) */

//---------------------------------------------------------------------
//          The INTERNAL Hercules 'hifr' structure
//---------------------------------------------------------------------
/*
  The Hercules ifr (hifr) structure. Why? Because an ifreq stucture is
  not large enough to hold inet6 addresses, an in6_ifreq structure is
  needed for them. The hifr structure contains both an ifreq stucture
  and an in6_ifreq structure.

  The ifreq structure is the parameter to most of the ioctl requests
  issued by hercifc, with ifrn_name specifying the device to which the
  ioctl request applies, and the ifr_ifru union containing the
  appropriate value for the ioctl.

  When an ioctl request is made to set an inet6 address, an ifreq
  structure is not used; instead an in6_ifreq structure is the
  parameter. The field ifr6_ifindex specifies the device to which the
  ioctl request applies, and the ifr6_addr and ifr6_prefixlen fields
  contain the appropriate values for the ioctl. The ifr6_ifindex value
  is obtained using the ifrn_name value.

  Although the ifrn_name is not used when an in6_ifreq structure is the
  parameter to the ioctl request, it is needed for hercifc to use in
  error messages, etc.

  To distinguish when the in6_ifreq structure is the parameter to the
  ioctl request the field hifr_afamily must contain the value AF_INET6,
  otherwise it should contain the value AF_INET or zero.

  ioctl requests with an ifreq structure as the parameter are made
  using an inet socket, whereas ioctl requests with an in6_ifreq
  structure as the parameter are made using an inet6 socket.

*/
struct hifr
{
    struct ifreq ifreq;
#if defined(ENABLE_IPV6)
    struct in6_ifreq  in6_ifreq;
#endif /*defined(ENABLE_IPV6)*/
    int               hifr_afamily;
};
typedef struct hifr hifr;

#if defined( __APPLE__ )          \
 || defined( __sun__ )            \
 || defined( FREEBSD_OR_NETBSD )

  #define  hifr_name       ifreq.ifr_name
#else
  #define  hifr_name       ifreq.ifr_ifrn.ifrn_name
#endif
  #define  hifr_addr       ifreq.ifr_ifru.ifru_addr
  #define  hifr_netmask    ifreq.ifr_ifru.ifru_netmask
  #define  hifr_broadaddr  ifreq.ifr_ifru.ifru_broadaddr
  #define  hifr_hwaddr     ifreq.ifr_ifru.ifru_hwaddr
#if defined( __FreeBSD__ )
  /* short   ifru_flags[2];                                          */
  #define  hifr_flags      ifreq.ifr_ifru.ifru_flags[0]
#else
  #define  hifr_flags      ifreq.ifr_ifru.ifru_flags
#endif
#if defined( __OpenBSD__ )
  #define  hifr_mtu        ifreq.ifr_ifru.ifru_metric
#else
  #define  hifr_mtu        ifreq.ifr_ifru.ifru_mtu
#endif

#if defined(ENABLE_IPV6)
  #define  hifr6_addr      in6_ifreq.ifr6_addr
  #define  hifr6_prefixlen in6_ifreq.ifr6_prefixlen
  #define  hifr6_ifindex   in6_ifreq.ifr6_ifindex
#endif /* defined(ENABLE_IPV6) */

//---------------------------------------------------------------------
//                  Some handy default values
//---------------------------------------------------------------------

#define DEF_MTU         1500
#define DEF_MTU_STR    "1500"

typedef struct ifreq ifreq;

#endif // __HIFR_H_
