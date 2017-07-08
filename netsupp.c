/* NETSUPP.C   (C) Copyright "Fish" (David B. Trout), 2017           */
/*                 Networking Support Functions                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"
#include "netsupp.h"

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
                return -EEXIST; // (already exists)
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

    return -ENOENT; // (not found)
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
        nSum += ntohs( *(puShort+ 0) );
        nSum += ntohs( *(puShort+ 1) );
        nSum += ntohs( *(puShort+ 2) );
        nSum += ntohs( *(puShort+ 3) );
        nSum += ntohs( *(puShort+ 4) );
        nSum += ntohs( *(puShort+ 5) );
        nSum += ntohs( *(puShort+ 6) );
        nSum += ntohs( *(puShort+ 7) );
        nSum += ntohs( *(puShort+ 8) );
        nSum += ntohs( *(puShort+ 9) );
        nSum += ntohs( *(puShort+10) );
        nSum += ntohs( *(puShort+11) );
        nSum += ntohs( *(puShort+12) );
        nSum += ntohs( *(puShort+13) );
        nSum += ntohs( *(puShort+14) );
        nSum += ntohs( *(puShort+15) );
        nSum += ntohs( *(puShort+16) );
        nSum += ntohs( *(puShort+17) );
        nSum += ntohs( *(puShort+18) );
        nSum += ntohs( *(puShort+19) );
        nSum += ntohs( *(puShort+20) );
        nSum += ntohs( *(puShort+21) );
        nSum += ntohs( *(puShort+22) );
        nSum += ntohs( *(puShort+23) );
        nSum += ntohs( *(puShort+24) );
        nSum += ntohs( *(puShort+25) );
        nSum += ntohs( *(puShort+26) );
        nSum += ntohs( *(puShort+27) );
        nSum += ntohs( *(puShort+28) );
        nSum += ntohs( *(puShort+29) );
        nSum += ntohs( *(puShort+30) );
        nSum += ntohs( *(puShort+31) );
        nSum += ntohs( *(puShort+32) );
        nSum += ntohs( *(puShort+33) );
        nSum += ntohs( *(puShort+34) );
        nSum += ntohs( *(puShort+35) );
        nSum += ntohs( *(puShort+36) );
        nSum += ntohs( *(puShort+37) );
        nSum += ntohs( *(puShort+38) );
        nSum += ntohs( *(puShort+39) );
        nSum += ntohs( *(puShort+40) );
        nSum += ntohs( *(puShort+41) );
        nSum += ntohs( *(puShort+42) );
        nSum += ntohs( *(puShort+43) );
        nSum += ntohs( *(puShort+44) );
        nSum += ntohs( *(puShort+45) );
        nSum += ntohs( *(puShort+46) );
        nSum += ntohs( *(puShort+47) );
        nSum += ntohs( *(puShort+48) );
        nSum += ntohs( *(puShort+49) );
        nSum += ntohs( *(puShort+50) );
        nSum += ntohs( *(puShort+51) );
        nSum += ntohs( *(puShort+52) );
        nSum += ntohs( *(puShort+53) );
        nSum += ntohs( *(puShort+54) );
        nSum += ntohs( *(puShort+55) );
        nSum += ntohs( *(puShort+56) );
        nSum += ntohs( *(puShort+57) );
        nSum += ntohs( *(puShort+58) );
        nSum += ntohs( *(puShort+59) );
        nSum += ntohs( *(puShort+60) );
        nSum += ntohs( *(puShort+61) );
        nSum += ntohs( *(puShort+62) );
        nSum += ntohs( *(puShort+63) );
        puShort +=               64;
        FoldSum32( &nSum );
    }
    nBytes += 128;

    while ((nBytes -= 32) >= 0)
    {
        nSum += ntohs( *(puShort+ 0) );
        nSum += ntohs( *(puShort+ 1) );
        nSum += ntohs( *(puShort+ 2) );
        nSum += ntohs( *(puShort+ 3) );
        nSum += ntohs( *(puShort+ 4) );
        nSum += ntohs( *(puShort+ 5) );
        nSum += ntohs( *(puShort+ 6) );
        nSum += ntohs( *(puShort+ 7) );
        nSum += ntohs( *(puShort+ 8) );
        nSum += ntohs( *(puShort+ 9) );
        nSum += ntohs( *(puShort+10) );
        nSum += ntohs( *(puShort+11) );
        nSum += ntohs( *(puShort+12) );
        nSum += ntohs( *(puShort+13) );
        nSum += ntohs( *(puShort+14) );
        nSum += ntohs( *(puShort+15) );
        puShort +=               16;
    }
    nBytes += 32;

    FoldSum32( &nSum );

    while ((nBytes -= 8) >= 0)
    {
        nSum += ntohs( *(puShort+0) );
        nSum += ntohs( *(puShort+1) );
        nSum += ntohs( *(puShort+2) );
        nSum += ntohs( *(puShort+3) );
        puShort +=               4;
    }
    nBytes += 8;

    while ((nBytes -= 2) >= 0)
    {
        nSum += ntohs( *(puShort+0) );
        puShort +=               1;
    }
    nBytes += 2;

    if (nBytes)
        nSum += ntohs( *((BYTE*)puShort) );

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

        switch (pIP->ip_p)
        {
            case IPPROTO_TCP:
            {
                tcp_hdr* pTCP = (tcp_hdr*) pPacket;

                pIP->ip_sum  = 0;   // (start clean)
                pTCP->th_sum = 0;   // (start clean)

                // Handle upper TCP layer first

                pTCP->th_sum = htons( PseudoHdrCheckSum( pIP ));
                pTCP->th_sum = htons( InetCheckSum( (BYTE*) pTCP, (S32)( nPacketLen - nIPHdrLen )));

                // Handle lower IP layer last

                pIP->ip_sum = htons( InetCheckSum( (BYTE*) pIP, (S32) nIPHdrLen ));
            }
            break;

            case IPPROTO_UDP:
            {
                udp_hdr* pUDP = (udp_hdr*) pPacket;

                pIP->ip_sum  = 0;   // (start clean)
                pUDP->uh_sum = 0;   // (start clean)

                // Handle upper UDP layer first

                pUDP->uh_sum = htons( PseudoHdrCheckSum( pIP ));
                pUDP->uh_sum = htons( InetCheckSum( (BYTE*) pUDP, (S32) ntohs( pUDP->uh_ulen )));

                // Handle lower IP layer last

                pIP->ip_sum = htons( InetCheckSum( (BYTE*) pIP, (S32) nIPHdrLen ));
            }
            break;

            default: // (some other protocol)
            {
                // But since it IS an IP packet we need to
                // still calculate the IP header checksum!

                pIP->ip_sum = 0;   // (start clean)
                pIP->ip_sum = htons( InetCheckSum( (BYTE*) pIP, (S32) nIPHdrLen ));
            }
            break;
        }

        // Go on to the next IP packet...

        pIP = (ip_hdr*) (((BYTE*)pIP) + nPacketLen );
        nBytes -= MIN( nBytes, (size_t) nPacketLen );

    } // while (nBytes > ip_hdr_size)
}
