/* TT32IF.H (C) Copyright SoftDevLabs, 2013-2017                     */
/*              TunTap32 networking interface structure and defines  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

// Copyright (C) 2013-2017, Software Development Laboratories, "Fish" (David B. Trout)
/////////////////////////////////////////////////////////////////////////////////////////
//
//      tt32if.h    --  declarations for inquiring about network interfaces
//
/////////////////////////////////////////////////////////////////////////////////////////
//
//  http://www.musl-libc.org/
//  http://git.musl-libc.org/cgit/musl/plain/COPYRIGHT
//
//  musl as a whole is licensed under the following standard MIT license:
//
//  Copyright © 2005-2012 Rich Felker
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//  [...]
//
//  All other files which have no copyright comments are original works
//  Copyright © 2005-2012 Rich Felker, the main author of this library.
//  The decision to exclude such comments is intentional, as it should be
//  possible to carry around the complete source code on tiny storage
//  media. All public header files (include/*) should be treated as Public
//  Domain as they intentionally contain no content which can be covered
//  by copyright. Some source modules may fall in this category as well.
//  If you believe that a file is so trivial that it should be in the
//  Public Domain, please contact me and, if I agree, I will explicitly
//  release it from copyright.
//
//  The following files are trivial, in my opinion not copyrightable in
//  the first place, and hereby explicitly released to the Public Domain:
//
//  All public headers: include/*
//  Startup files: crt/*
//
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _TT32IF_H_
#define _TT32IF_H_

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------

struct if_nameindex
{
    unsigned int if_index;
    char *if_name;
};

//---------------------------------------------------------------------------------------

struct ifaddr {
    struct sockaddr ifa_addr;
    union {
        struct sockaddr ifu_broadaddr;
        struct sockaddr ifu_dstaddr;
    } ifa_ifu;
    struct iface *ifa_ifp;
    struct ifaddr *ifa_next;
};

#define ifa_broadaddr   ifa_ifu.ifu_broadaddr
#define ifa_dstaddr     ifa_ifu.ifu_dstaddr

//---------------------------------------------------------------------------------------

struct ifmap {
    unsigned long int mem_start;
    unsigned long int mem_end;
    unsigned short int base_addr;
    unsigned char irq;
    unsigned char dma;
    unsigned char port;
};

//---------------------------------------------------------------------------------------
// Open flags

#define IFF_TUN             0x0001
#define IFF_TAP             0x0002
#define IFF_NO_PI           0x1000  // (allowed but ignored; no pi is default)
#define IFF_ONE_QUEUE       0x2000  // (allowed but ignored; no queue support)

//---------------------------------------------------------------------------------------
// Fish: 2006-06-16: Must #undef first since PSDK header
//                   'ws2tcpip.h' #defines them differently
#undef  IFF_UP
#undef  IFF_BROADCAST
#undef  IFF_DEBUG
#undef  IFF_LOOPBACK
#undef  IFF_POINTOPOINT
#undef  IFF_NOTRAILERS
#undef  IFF_RUNNING
#undef  IFF_NOARP
#undef  IFF_PROMISC
#undef  IFF_ALLMULTI
#undef  IFF_MASTER
#undef  IFF_SLAVE
#undef  IFF_MULTICAST
#undef  IFF_PORTSEL
#undef  IFF_AUTOMEDIA

//---------------------------------------------------------------------------------------

#define IFF_UP              0x0001      //  @  Interface is up.
#define IFF_BROADCAST       0x0002      //  #  Broadcast address valid.
#define IFF_DEBUG           0x0004      //  @  Turn on debugging.
#define IFF_LOOPBACK        0x0008      //     Is a loopback net.
#define IFF_POINTOPOINT     0x0010      //     Interface is point-to-point link.
#define IFF_NOTRAILERS      0x0020      //     Avoid use of trailers (obsolete).
#define IFF_RUNNING         0x0040      //  +  Interface is operational.
#define IFF_NOARP           0x0080      //     No address resolution protocol.
#define IFF_PROMISC         0x0100      //  @  Receive all packets.
#define IFF_ALLMULTI        0x0200      //  @  Receive all multicast packets.
#define IFF_MASTER          0x0400      //     Master of a load balancer.
#define IFF_SLAVE           0x0800      //     Slave of a load balancer.
#define IFF_MULTICAST       0x1000      //  +  Supports multicast.
#define IFF_PORTSEL         0x2000      //     Can set media type.
#define IFF_AUTOMEDIA       0x4000      //     Auto media select active.
#define IFF_DYNAMIC         0x8000      //     Dialup device with changing addresses.

                                        // (@)  Supported.
                                        // (#)  Allowed but ignored.
                                        // (+)  Read-only (unmodifiable).

                                        // All others are unsupported by CTCI-WIN.

//---------------------------------------------------------------------------------------

#ifndef IFHWADDRLEN
#define IFHWADDRLEN     6
#endif
#ifndef IFNAMSIZ
#define IFNAMSIZ        16
#endif

struct ifreq {
    union {
        char ifrn_name[IFNAMSIZ];
    } ifr_ifrn;
    union {
        struct sockaddr ifru_addr;
        struct sockaddr ifru_dstaddr;
        struct sockaddr ifru_broadaddr;
        struct sockaddr ifru_netmask;
        struct sockaddr ifru_hwaddr;
        short int ifru_flags;
        int ifru_ivalue;
        int ifru_mtu;
        struct ifmap ifru_map;
        char ifru_slave[IFNAMSIZ];
        char ifru_newname[IFNAMSIZ];
        void *ifru_data;
    } ifr_ifru;
};
typedef struct ifreq ifreq;

#define ifr_name        ifr_ifrn.ifrn_name
#define ifr_hwaddr      ifr_ifru.ifru_hwaddr
#define ifr_addr        ifr_ifru.ifru_addr
#define ifr_dstaddr     ifr_ifru.ifru_dstaddr
#define ifr_broadaddr   ifr_ifru.ifru_broadaddr
#define ifr_netmask     ifr_ifru.ifru_netmask
#define ifr_flags       ifr_ifru.ifru_flags
#define ifr_metric      ifr_ifru.ifru_ivalue
#define ifr_mtu         ifr_ifru.ifru_mtu
#define ifr_map         ifr_ifru.ifru_map
#define ifr_slave       ifr_ifru.ifru_slave
#define ifr_data        ifr_ifru.ifru_data
#define ifr_ifindex     ifr_ifru.ifru_ivalue
#define ifr_bandwidth   ifr_ifru.ifru_ivalue
#define ifr_qlen        ifr_ifru.ifru_ivalue
#define ifr_newname     ifr_ifru.ifru_newname
#define _IOT_ifreq      _IOT(_IOTS(char),IFNAMSIZ,_IOTS(char),16,0,0)
#define _IOT_ifreq_short _IOT(_IOTS(char),IFNAMSIZ,_IOTS(short),1,0,0)
#define _IOT_ifreq_int  _IOT(_IOTS(char),IFNAMSIZ,_IOTS(int),1,0,0)

//---------------------------------------------------------------------------------------
// Win32 (MinGW/Cygwin/MSVC) does not have these
// so we need to define them ourselves.

/* Use the definitions from the kernel header files.  */
//#include <asm/ioctls.h>

// PROGRAMMING NOTE: Cygwin's headers define some (but not all)
// of the below values, but we unfortunately MUST use the below
// defined values since they're what TunTap32 expects...

#undef SIOCGIFCONF      // (discard Cygwin's value to use below instead)
#undef SIOCGIFFLAGS     // (discard Cygwin's value to use below instead)
#undef SIOCGIFADDR      // (discard Cygwin's value to use below instead)
#undef SIOCGIFBRDADDR   // (discard Cygwin's value to use below instead)
#undef SIOCGIFNETMASK   // (discard Cygwin's value to use below instead)
#undef SIOCGIFMETRIC    // (discard Cygwin's value to use below instead)
#undef SIOCGIFMTU       // (discard Cygwin's value to use below instead)
#undef SIOCGIFHWADDR    // (discard Cygwin's value to use below instead)

/* Routing table calls.  */
#define SIOCADDRT       0x890B          /* add routing table entry      */
#define SIOCDELRT       0x890C          /* delete routing table entry   */
#define SIOCRTMSG       0x890D          /* call to routing system       */

/* Socket configuration controls. */
#define SIOCGIFNAME     0x8910          /* get iface name               */
#define SIOCSIFLINK     0x8911          /* set iface channel            */
#define SIOCGIFCONF     0x8912          /* get iface list               */
#define SIOCGIFFLAGS    0x8913          /* get flags                    */
#define SIOCSIFFLAGS    0x8914          /* set flags                    */
#define SIOCGIFADDR     0x8915          /* get PA address               */
#define SIOCSIFADDR     0x8916          /* set PA address (*)           */
#define SIOCGIFDSTADDR  0x8917          /* get remote PA address        */
#define SIOCSIFDSTADDR  0x8918          /* set remote PA address (*)    */
#define SIOCGIFBRDADDR  0x8919          /* get broadcast PA address     */
#define SIOCSIFBRDADDR  0x891a          /* set broadcast PA address (*) */
#define SIOCGIFNETMASK  0x891b          /* get network PA mask          */
#define SIOCSIFNETMASK  0x891c          /* set network PA mask (*)      */
#define SIOCGIFMETRIC   0x891d          /* get metric                   */
#define SIOCSIFMETRIC   0x891e          /* set metric                   */
#define SIOCGIFMEM      0x891f          /* get memory address (BSD)     */
#define SIOCSIFMEM      0x8920          /* set memory address (BSD)     */
#define SIOCGIFMTU      0x8921          /* get MTU size                 */
#define SIOCSIFMTU      0x8922          /* set MTU size (*)             */
#define SIOCSIFHWADDR   0x8924          /* set hardware address (*)     */
#define SIOCGIFENCAP    0x8925          /* get/set encapsulations       */
#define SIOCSIFENCAP    0x8926
#define SIOCGIFHWADDR   0x8927          /* Get hardware address         */
#define SIOCGIFSLAVE    0x8929          /* Driver slaving support       */
#define SIOCSIFSLAVE    0x8930
#define SIOCADDMULTI    0x8931          /* Multicast address lists      */
#define SIOCDELMULTI    0x8932
#define SIOCGIFINDEX    0x8933          /* name -> if_index mapping     */
#define SIOGIFINDEX     SIOCGIFINDEX    /* misprint compatibility :-)   */
#define SIOCSIFPFLAGS   0x8934          /* set/get extended flags set   */
#define SIOCGIFPFLAGS   0x8935
#define SIOCDIFADDR     0x8936          /* delete PA address            */
#define SIOCSIFHWBROADCAST     0x8937   /* set HW broadcast addr (*)    */
#define SIOCGIFCOUNT    0x8938          /* get number of devices        */

    /*------------------------------------------------------------------*/
    /*    (*)  CTCI-WIN:  Rejected if interface is already IFF_UP.      */
    /*------------------------------------------------------------------*/


#define SIOCGIFBR       0x8940          /* Bridging support             */
#define SIOCSIFBR       0x8941          /* Set bridging options         */

#define SIOCGIFTXQLEN   0x8942          /* Get the tx queue length      */
#define SIOCSIFTXQLEN   0x8943          /* Set the tx queue length      */


/* ARP cache control calls. */
                    /*  0x8950 - 0x8952  * obsolete calls, don't re-use */
#define SIOCDARP        0x8953          /* delete ARP table entry       */
#define SIOCGARP        0x8954          /* get ARP table entry          */
#define SIOCSARP        0x8955          /* set ARP table entry          */

/* RARP cache control calls. */
#define SIOCDRARP       0x8960          /* delete RARP table entry      */
#define SIOCGRARP       0x8961          /* get RARP table entry         */
#define SIOCSRARP       0x8962          /* set RARP table entry         */

/* Driver configuration calls */

#define SIOCGIFMAP      0x8970          /* Get device parameters        */
#define SIOCSIFMAP      0x8971          /* Set device parameters        */

/* DLCI configuration calls */

#define SIOCADDDLCI     0x8980          /* Create new DLCI device       */
#define SIOCDELDLCI     0x8981          /* Delete DLCI device           */

/* Device private ioctl calls.  */

/* These 16 ioctls are available to devices via the do_ioctl() device
   vector.  Each device should include this file and redefine these
   names as their own. Because these are device dependent it is a good
   idea _NOT_ to issue them to random objects and hope.  */

#define SIOCDEVPRIVATE          0x89F0  /* to 89FF */

/*
 *      These 16 ioctl calls are protocol private
 */

#define SIOCPROTOPRIVATE 0x89E0 /* to 89EF */

//---------------------------------------------------------------------------------------

struct ifconf {
    int ifc_len;
    union {
        void *ifcu_buf;
        struct ifreq *ifcu_req;
    } ifc_ifcu;
};

#define ifc_buf         ifc_ifcu.ifcu_buf
#define ifc_req         ifc_ifcu.ifcu_req
#define _IOT_ifconf _IOT(_IOTS(struct ifconf),1,0,0,0,0)

//---------------------------------------------------------------------------------------
// Fish: 2013-02-21: Added struct rtentry to tt32if.h for convenience.

struct rtentry      // (rtentry must be defined before 'hercifc.h')
{
    unsigned long int   rt_pad1;
    struct sockaddr     rt_dst;         // Target address.
    struct sockaddr     rt_gateway;     // Gateway addr (RTF_GATEWAY)
    struct sockaddr     rt_genmask;     // Target network mask (IP)
    unsigned short int  rt_flags;
    short int           rt_pad2;
    unsigned long int   rt_pad3;
    unsigned char       rt_tos;
    unsigned char       rt_class;
    short int           rt_pad4;
    short int           rt_metric;      // +1 for binary compatibility!
    char *              rt_dev;         // Forcing the device at add.
    unsigned long int   rt_mtu;         // Per route MTU/Window.
    unsigned long int   rt_window;      // Window clamping.
    unsigned short int  rt_irtt;        // Initial RTT.
};

//---------------------------------------------------------------------------------------
// Fish: 2013-02-21: Added routing #defines to tt32if.h for convenience.

#define RTF_UP                0x00000001    /* Route usable                  */
#define RTF_GATEWAY           0x00000002    /* Destination is a gateway      */
#define RTF_HOST              0x00000004    /* Host entry (net otherwise)    */
#define RTF_REINSTATE         0x00000008    /* Reinstate route after timeout */
#define RTF_DYNAMIC           0x00000010    /* Created dyn (by redirect)     */
#define RTF_MODIFIED          0x00000020    /* Modified dyn (by redirect)    */
#define RTF_MTU               0x00000040    /* Specific MTU for this route   */
#define RTF_MSS               RTF_MTU       /* Compatibility                 */
#define RTF_WINDOW            0x00000080    /* Per route window clamping     */
#define RTF_IRTT              0x00000100    /* Initial round trip time       */
#define RTF_REJECT            0x00000200    /* Reject route                  */
#define RTF_STATIC            0x00000400    /* Manually injected route       */
#define RTF_XRESOLVE          0x00000800    /* External resolver             */
#define RTF_NOFORWARD         0x00001000    /* Forwarding inhibited          */
#define RTF_THROW             0x00002000    /* Go to next class              */
#define RTF_NOPMTUDISC        0x00004000    /* Do not send packets with DF   */
#define RTF_DEFAULT           0x00010000    /* default - learned via ND      */
#define RTF_ALLONLINK         0x00020000    /* fallback, no routers on link  */
#define RTF_ADDRCONF          0x00040000    /* addrconf route - RA           */
#define RTF_LINKRT            0x00100000    /* link specific - device match  */
#define RTF_NONEXTHOP         0x00200000    /* route with no nexthop         */
#define RTF_CACHE             0x01000000    /* cache entry                   */
#define RTF_FLOW              0x02000000    /* flow significant route        */
#define RTF_POLICY            0x04000000    /* policy route                  */
#define RTCF_VALVE            0x00200000
#define RTCF_MASQ             0x00400000
#define RTCF_NAT              0x00800000
#define RTCF_DOREDIRECT       0x01000000
#define RTCF_LOG              0x02000000
#define RTCF_DIRECTSRC        0x04000000
#define RTF_NAT               0x08000000
#define RTF_BROADCAST         0x10000000
#define RTF_MULTICAST         0x20000000
#define RTF_INTERFACE         0x40000000
#define RTF_LOCAL             0x80000000
#define RTF_ADDRCLASSMASK     0xF8000000

#define RT_ADDRCLASS(flags)   ((__u_int32_t) flags >> 23)
#define RT_TOS(tos)           ((tos) & IPTOS_TOS_MASK)
#define RT_LOCALADDR(flags)   ((flags & RTF_ADDRCLASSMASK) \
                              == (RTF_LOCAL|RTF_INTERFACE))
#define RT_CLASS_UNSPEC       0
#define RT_CLASS_DEFAULT      253
#define RT_CLASS_MAIN         254
#define RT_CLASS_LOCAL        255
#define RT_CLASS_MAX          255

#define RTMSG_ACK             NLMSG_ACK
#define RTMSG_OVERRUN         NLMSG_OVERRUN
#define RTMSG_NEWDEVICE       0x11
#define RTMSG_DELDEVICE       0x12
#define RTMSG_NEWROUTE        0x21
#define RTMSG_DELROUTE        0x22
#define RTMSG_NEWRULE         0x31
#define RTMSG_DELRULE         0x32
#define RTMSG_CONTROL         0x40
#define RTMSG_AR_FAILED       0x51          /* Address Resolution failed     */

//---------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // _TT32IF_H_

/////////////////////////////////////////////////////////////////////////////////////////
