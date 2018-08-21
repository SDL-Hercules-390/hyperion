/* NETSUPP.H   (C) Copyright "Fish" (David B. Trout), 2017           */
/*                 Networking Support Functions                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _NETSUPP_H_
#define _NETSUPP_H_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*                     MACTAB structure                              */
/*-------------------------------------------------------------------*/
struct MACTAB
{
#ifndef    IFHWADDRLEN
  #define  IFHWADDRLEN      6       /* Mac OSX is missing this       */
#endif
    BYTE   inuse;                   /* In use flag: 0=free, 1=in use */
    BYTE   flags;                   /* Optional user-provided flags  */
    BYTE   mac[ IFHWADDRLEN ];      /* HW MAC address                */
};

typedef struct MACTAB  MACTAB, *PMACTAB;

#define MACTABMAX           32      /* Maximum MACTAB entries        */

/*-------------------------------------------------------------------*/
/*                     MACTAB functions                              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*    MACTabAdd:    add a new MAC to the table.                      */
/*    MACTabRem:    remove an old MAC from the table.                */
/*    IsMACTab:     check if MAC is already in table.                */
/*                                                                   */
/* All functions return >= 0 if successful, < 0  upon failure.       */
/* IsMACTab returns the table entry number if found else -ENOENT.    */
/*                                                                   */
/*-------------------------------------------------------------------*/
extern int  MACTabAdd (       MACTAB* tab, const BYTE* mac, const BYTE flags );
extern int  MACTabRem (       MACTAB* tab, const BYTE* mac );
extern int  IsMACTab  ( const MACTAB* tab, const BYTE* mac );

/*-------------------------------------------------------------------*/
/*                     Ethernet header                               */
/*-------------------------------------------------------------------*/
struct eth_hdr
{
    U8      ether_dhost[6];     // dst MAC
    U8      ether_shost[6];     // src MAC
    U16     ether_type;         // frame type
};

typedef struct eth_hdr          eth_hdr;
#define                         eth_hdr_size    sizeof( eth_hdr )
#define ETHERTYPE_IP            0x0800          // IP ether_type

/*-------------------------------------------------------------------*/
/*                        IP header                                  */
/*-------------------------------------------------------------------*/
struct ip_hdr
{
#ifdef  WORDS_BIGENDIAN
#ifdef _AIX
    U8      ip_v_hl;
#else
    U8      ip_v:4,         // IP version (4)
            ip_hl:4;        // hdr len
#endif
#else
    U8      ip_hl:4,        // hdr len
            ip_v:4;         // IP version (4)
#endif
    BYTE    ip_tos;         // Type of service
    S16     ip_len;         // Packet length
    U16     ip_id;          // Identification
    S16     ip_off;         // Fragment offset
    BYTE    ip_ttl;         // Time to Live
    BYTE    ip_p;           // Protocol
    U16     ip_sum;         // Checksum
    U32     ip_src;         // src addr
    U32     ip_dst;         // dst addr
};

typedef struct ip_hdr       ip_hdr;
#define                     ip_hdr_size         sizeof( ip_hdr )

/*-------------------------------------------------------------------*/
/*                          TCP header                               */
/*-------------------------------------------------------------------*/
struct tcp_hdr
{
    U16     th_sport;       // src port
    U16     th_dport;       // dst port
    U32     th_seq;         // seq num
    U32     th_ack;         // ack num

#ifdef  WORDS_BIGENDIAN
#ifdef  _AIX
    BYTE    th_off_x2;
#else
    BYTE    th_off:4;       // payload offset
    BYTE    th_x2:4;        // (unused)
#endif
#else
    BYTE    th_x2:4;        // (unused)
    BYTE    th_off:4;       // payload offset
#endif
    BYTE    th_flags;
    U16     th_win;         // window size
    U16     th_sum;         // checksum
    U16     th_urp;         // urgent ptr
};

typedef struct tcp_hdr      tcp_hdr;
#define                     tcp_hdr_size        sizeof( tcp_hdr )


/*-------------------------------------------------------------------*/
/*                           UDP header                              */
/*-------------------------------------------------------------------*/
struct udp_hdr
{
    U16     uh_sport;       // src port
    U16     uh_dport;       // dst port
    S16     uh_ulen;        // packet len
    U16     uh_sum;         // checksum
};

typedef struct udp_hdr      udp_hdr;
#define                     udp_hdr_size        sizeof( udp_hdr )

/*-------------------------------------------------------------------*/
/*                     TCP/UDP "Pseudo Header"                       */
/*-------------------------------------------------------------------*/

struct pseudo_hdr
{
    U32   ph_ip_src;        // src IP addr      from IP hdr
    U32   ph_ip_dst;        // dst IP addr      from IP hdr
    U8    ph_zero;          //                  resvered/not used;
                            //                  must be binary zero
    U8    ph_ip_p;          // protocol         from IP hdr
    U16   ph_ulen;          // data len         UDP: from hdr
                            //                  TCP: calculated
};

typedef struct pseudo_hdr   pseudo_hdr;
#define                     pseudo_hdr_size     sizeof( pseudo_hdr )

/*-------------------------------------------------------------------*/
/*                 Checksum Offloading functions                     */
/*-------------------------------------------------------------------*/

extern void EtherIpv4CkSumOffload( BYTE* pFrame, size_t nBytes );

extern U16  CheckSum          ( const BYTE* pBuffer, S32 nBytes );
extern U16  InetCheckSum      ( const BYTE* pBuffer, S32 nBytes );
extern U16  PseudoHdrCheckSum ( ip_hdr* pIP );

#endif // _NETSUPP_H_
