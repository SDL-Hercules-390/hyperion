/* NETSUPP.C   (C) Copyright "Fish" (David B. Trout), 2017           */
/*                 Networking Support Functions                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"
#include "netsupp.h"
#include "tuntap.h"         // (need TUNTAP_Read function)

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

int  MACTabAdd ( MACTAB* tab, const BYTE* mac, const BYTE flags )
{
    int  i, n = -1;

    for (i=0; i < MACTABMAX; i++)
    {
        if (tab[i].inuse)
        {
            if (memcmp( tab[i].mac, mac, sizeof( tab[i].mac )) == 0)
#if 0 // technically this should be an error
                return -EEXIST; // (already exists)
#else // but we'll treat it as success
                return 0;       // (already exists)
#endif
        }
        else // (unused entry)
        {
            if (n < 0)
                n = i;
        }
    }

    if (n < 0)
        return -ENOSPC; // (table full)

    tab[n].inuse = TRUE;
    tab[n].flags = flags;

    memcpy( tab[n].mac, mac, sizeof( tab[n].mac ));

    return 0;
}

/*-------------------------------------------------------------------*/

int  MACTabRem ( MACTAB* tab, const BYTE* mac )
{
    int  i;

    for (i=0; i < MACTABMAX; i++)
    {
        if (memcmp( tab[i].mac, mac, sizeof( tab[i].mac )) == 0)
        {
            memset( &tab[i], 0, sizeof( MACTAB ));
            return 0;
        }
    }

#if 0 // technically this should be an error
    return -ENOENT; // (not found)
#else // but we'll treat it as success
    return 0;       // (not found)
#endif
}

/*-------------------------------------------------------------------*/

int  IsMACTab ( const MACTAB* tab, const BYTE* mac )
{
    int  i;
    for (i=0; i < MACTABMAX; i++)
        if (tab[i].inuse)
            if (memcmp( tab[i].mac, mac, sizeof( tab[i].mac )) == 0)
                return i;
    return -ENOENT; // (not found)
}

/*-------------------------------------------------------------------*/
/*         Fold a 32-bit checksum into a 16-bit checksum             */
/*-------------------------------------------------------------------*/

static inline void  FoldSum32( U32* pSum )
{
    *pSum  = (*pSum >> 16) + (*pSum & 0xFFFF);  // (add hi 16 to lo 16)
    *pSum  = (*pSum >> 16) + (*pSum & 0xFFFF);  // (add carry (if any))
}

/*-------------------------------------------------------------------*/
/*        Calculate UN-complemented Pseudo Header checksum           */
/*-------------------------------------------------------------------*/

U16  PseudoHdrCheckSum ( ip_hdr* pIP )
{
    pseudo_hdr  ph;

    ph.ph_ip_src  =  pIP->ip_src;
    ph.ph_ip_dst  =  pIP->ip_dst;
    ph.ph_zero    =  0;
    ph.ph_ip_p    =  pIP->ip_p;
    ph.ph_ulen    =  htons( ntohs( pIP->ip_len ) - (pIP->ip_hl * 4) );

    return CheckSum( (BYTE*) &ph, pseudo_hdr_size );
}

/*-------------------------------------------------------------------*/
/*           Calculate UN-complemented 16-bit checksum               */
/*-------------------------------------------------------------------*/

U16  CheckSum ( const BYTE* pBuffer, S32 nBytes )
{
    U32   nSum     =  0;
    U16*  puShort  =  (U16*) pBuffer;

    while ((nBytes -= 128) >= 0)
    {
        nSum  +=  *( puShort +  0 );
        nSum  +=  *( puShort +  1 );
        nSum  +=  *( puShort +  2 );
        nSum  +=  *( puShort +  3 );
        nSum  +=  *( puShort +  4 );
        nSum  +=  *( puShort +  5 );
        nSum  +=  *( puShort +  6 );
        nSum  +=  *( puShort +  7 );
        nSum  +=  *( puShort +  8 );
        nSum  +=  *( puShort +  9 );
        nSum  +=  *( puShort + 10 );
        nSum  +=  *( puShort + 11 );
        nSum  +=  *( puShort + 12 );
        nSum  +=  *( puShort + 13 );
        nSum  +=  *( puShort + 14 );
        nSum  +=  *( puShort + 15 );
        nSum  +=  *( puShort + 16 );
        nSum  +=  *( puShort + 17 );
        nSum  +=  *( puShort + 18 );
        nSum  +=  *( puShort + 19 );
        nSum  +=  *( puShort + 20 );
        nSum  +=  *( puShort + 21 );
        nSum  +=  *( puShort + 22 );
        nSum  +=  *( puShort + 23 );
        nSum  +=  *( puShort + 24 );
        nSum  +=  *( puShort + 25 );
        nSum  +=  *( puShort + 26 );
        nSum  +=  *( puShort + 27 );
        nSum  +=  *( puShort + 28 );
        nSum  +=  *( puShort + 29 );
        nSum  +=  *( puShort + 30 );
        nSum  +=  *( puShort + 31 );
        nSum  +=  *( puShort + 32 );
        nSum  +=  *( puShort + 33 );
        nSum  +=  *( puShort + 34 );
        nSum  +=  *( puShort + 35 );
        nSum  +=  *( puShort + 36 );
        nSum  +=  *( puShort + 37 );
        nSum  +=  *( puShort + 38 );
        nSum  +=  *( puShort + 39 );
        nSum  +=  *( puShort + 40 );
        nSum  +=  *( puShort + 41 );
        nSum  +=  *( puShort + 42 );
        nSum  +=  *( puShort + 43 );
        nSum  +=  *( puShort + 44 );
        nSum  +=  *( puShort + 45 );
        nSum  +=  *( puShort + 46 );
        nSum  +=  *( puShort + 47 );
        nSum  +=  *( puShort + 48 );
        nSum  +=  *( puShort + 49 );
        nSum  +=  *( puShort + 50 );
        nSum  +=  *( puShort + 51 );
        nSum  +=  *( puShort + 52 );
        nSum  +=  *( puShort + 53 );
        nSum  +=  *( puShort + 54 );
        nSum  +=  *( puShort + 55 );
        nSum  +=  *( puShort + 56 );
        nSum  +=  *( puShort + 57 );
        nSum  +=  *( puShort + 58 );
        nSum  +=  *( puShort + 59 );
        nSum  +=  *( puShort + 60 );
        nSum  +=  *( puShort + 61 );
        nSum  +=  *( puShort + 62 );
        nSum  +=  *( puShort + 63 );
        puShort += 64;
        FoldSum32( &nSum );
    }
    nBytes += 128;

    while ((nBytes -= 32) >= 0)
    {
        nSum  +=  *( puShort +  0 );
        nSum  +=  *( puShort +  1 );
        nSum  +=  *( puShort +  2 );
        nSum  +=  *( puShort +  3 );
        nSum  +=  *( puShort +  4 );
        nSum  +=  *( puShort +  5 );
        nSum  +=  *( puShort +  6 );
        nSum  +=  *( puShort +  7 );
        nSum  +=  *( puShort +  8 );
        nSum  +=  *( puShort +  9 );
        nSum  +=  *( puShort + 10 );
        nSum  +=  *( puShort + 11 );
        nSum  +=  *( puShort + 12 );
        nSum  +=  *( puShort + 13 );
        nSum  +=  *( puShort + 14 );
        nSum  +=  *( puShort + 15 );
        puShort += 16;
    }
    nBytes += 32;

    FoldSum32( &nSum );

    while ((nBytes -= 8) >= 0)
    {
        nSum  +=  *( puShort + 0 );
        nSum  +=  *( puShort + 1 );
        nSum  +=  *( puShort + 2 );
        nSum  +=  *( puShort + 3 );
        puShort += 4;
    }
    nBytes += 8;

    while ((nBytes -= 2) >= 0)
    {
        nSum  +=  *( puShort + 0 );
        puShort += 1;
    }
    nBytes += 2;

    if (nBytes)
        nSum  +=  *( (BYTE*) puShort );

    FoldSum32( &nSum );

    return (U16) nSum;      // (UN-complemented!)
}

/*-------------------------------------------------------------------*/
/*       Calculate Internet 16-bit ONES-COMPLEMENT checksum          */
/*-------------------------------------------------------------------*/

U16  InetCheckSum ( const BYTE* pBuffer, S32 nBytes )
{
    return ~CheckSum( pBuffer, nBytes );    // COMPLEMENTED!
}

/*-------------------------------------------------------------------*/
/*       Perform Ip Checksum Offloading for Ethernet Frame           */
/*-------------------------------------------------------------------*/

void EtherIpv4CkSumOffload( BYTE* pFrame, size_t nBytes )
{
    ip_hdr*   pIP;
    BYTE      nIPHdrLen;
    BYTE*     pPacket;
    U16       nPacketLen;

    // We only handle IPv4 checksum offloading

    if (ETHERTYPE_IP != ntohs( ((eth_hdr*) pFrame)->ether_type ))
        return;

    pIP      =  (ip_hdr*) (((BYTE*)pFrame) + eth_hdr_size);
    nBytes  -=  eth_hdr_size;

    while (nBytes > ip_hdr_size)
    {
        nPacketLen =  ntohs( pIP->ip_len );
        nIPHdrLen  =  pIP->ip_hl * sizeof( U32 );
        pPacket    =  (((BYTE*)pIP) + nIPHdrLen);

        /* Only compute checksum if not already present */

        if (!pIP->ip_sum)
        {
            switch (pIP->ip_p)
            {
                case IPPROTO_TCP:
                {
                    tcp_hdr* pTCP = (tcp_hdr*) pPacket;

                    pIP->ip_sum  = 0;   // (start clean)
                    pTCP->th_sum = 0;   // (start clean)

                    // Handle upper TCP layer first

                    pTCP->th_sum = PseudoHdrCheckSum( pIP );
                    pTCP->th_sum = InetCheckSum( (BYTE*) pTCP, (S32)( nPacketLen - nIPHdrLen ));

                    // Handle lower IP layer last

                    pIP->ip_sum = InetCheckSum( (BYTE*) pIP, (S32) nIPHdrLen );
                }
                break;

                case IPPROTO_UDP:
                {
                    udp_hdr* pUDP = (udp_hdr*) pPacket;

                    pIP->ip_sum  = 0;   // (start clean)
                    pUDP->uh_sum = 0;   // (start clean)

                    // Handle upper UDP layer first

                    pUDP->uh_sum = PseudoHdrCheckSum( pIP );
                    pUDP->uh_sum = InetCheckSum( (BYTE*) pUDP, (S32) ntohs( pUDP->uh_ulen ));

                    // Handle lower IP layer last

                    pIP->ip_sum = InetCheckSum( (BYTE*) pIP, (S32) nIPHdrLen );
                }
                break;

                case IPPROTO_ICMP:
                {
                    icmp_hdr* pICMP = (icmp_hdr*) pPacket;

                    pIP->ip_sum     = 0;   // (start clean)
                    pICMP->icmp_sum = 0;   // (start clean)

                    // Handle upper ICMP layer first

                    // pICMP->icmp_sum = PseudoHdrCheckSum( pIP );
                    pICMP->icmp_sum = InetCheckSum( (BYTE*) pICMP, (S32)( nPacketLen - nIPHdrLen ));

                    // Handle lower IP layer last

                    pIP->ip_sum = InetCheckSum( (BYTE*) pIP, (S32) nIPHdrLen );
                }
                break;

                default: // (some other protocol)
                {
                    // But since it IS an IP packet we need to
                    // still calculate the IP header checksum!

                    pIP->ip_sum = 0;   // (start clean)
                    pIP->ip_sum = InetCheckSum( (BYTE*) pIP, (S32) nIPHdrLen );
                }
                break;
            }
        }

        // Go on to the next IP packet...

        pIP = (ip_hdr*) (((BYTE*)pIP) + nPacketLen );
        nBytes -= MIN( nBytes, (size_t) nPacketLen );

    } // while (nBytes > ip_hdr_size)
}

/*-------------------------------------------------------------------*/
/*                 Timed read from tuntap device                     */
/*-------------------------------------------------------------------*/

int read_tuntap( int fd, BYTE* buffer, size_t nBuffLen, int secs )
{
    int nBytesRead;

#if !defined( OPTION_W32_CTCI ) // (i.e. Linux only)

    int rc;
    fd_set readset;
    struct timeval tv = {secs,0};
    FD_ZERO( &readset );
    FD_SET( fd, &readset );
    while ((rc = select( fd+1, &readset, NULL, NULL, &tv )) < 0 && errno == EINTR);
    if (rc < 0)
        return -1;
    if (rc == 0)
        return 0;

#endif // !defined( OPTION_W32_CTCI ) // (i.e. Linux only)

    nBytesRead = TUNTAP_Read( fd, buffer, nBuffLen );
    return nBytesRead;
}
