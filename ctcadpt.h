/* CTCADPT.H    (C) Copyright James A. Pierson, 2002-2012            */
/*              (C) Copyright Roger Bowler, 2000-2012                */
/*              (C) Copyright Willem Konynenberg, 2000-2009          */
/*              (C) Copyright Vic Cross, 2001-2009                   */
/*              (C) Copyright David B. Trout, 2002-2009              */
/*              Hercules Channel-to-Channel Emulation Support        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __CTCADPT_H_
#define __CTCADPT_H_

#include "netsupp.h"            // (Networking Support Functions)

// --------------------------------------------------------------------
// Pack all structures to byte boundary...
// --------------------------------------------------------------------

#undef ATTRIBUTE_PACKED
#if defined(_MSVC_)
 #pragma pack(push)
 #pragma pack(1)
 #define ATTRIBUTE_PACKED
#else
 #define ATTRIBUTE_PACKED __attribute__((packed))
#endif

// --------------------------------------------------------------------
// Definitions for 3088 model numbers
// --------------------------------------------------------------------

#define CTC_3088_01     0x308801        // 3172 XCA
#define CTC_3088_04     0x308804        // 3088 model 1 CTCA
#define CTC_3088_08     0x308808        // 3088 model 2 CTCA
#define CTC_3088_1E     0x30881E        // FICON CTC
#define CTC_3088_1F     0x30881F        // 3172 LCS
#define CTC_3088_60     0x308860        // OSA or 8232 LCS
#define CTC_3088_61     0x308861        // CLAW device

// --------------------------------------------------------------------
// Media Access Control address (MAC address)
// --------------------------------------------------------------------

#ifndef IFHWADDRLEN                     // (only predefined on Linux)
#define IFHWADDRLEN  6                  // Ethernet MAC address length
#endif

typedef uint8_t  MAC[ IFHWADDRLEN ];    // Data Type for MAC Addresses

// --------------------------------------------------------------------
// LCS structure typedefs   (actual structures defined further below)
// --------------------------------------------------------------------

#define LCS_MAX_PORTS   4   // Maximum support ports per LCS device

struct  _LCSBLK;            // Common Storage for LCS Emulation
struct  _LCSDEV;            // LCS Device
struct  _LCSPORT;           // LCS Port (or Relative Adapter)
struct  _LCSRTE;            // LCS Routing Entries
struct  _LCSHDR;            // LCS Frame Header
struct  _LCSCMDHDR;         // LCS Command Frame Header
struct  _LCSSTDFRM;         // LCS Standard Command Frame
struct  _LCSSTRTFRM;        // LCS Startup & Start LAN Command Frames
struct  _LCSQIPFRM;         // LCS Query IP Assists Command Frame
struct  _LCSLSTFRM;         // LCS LAN Statistics Command Frame
struct  _LCSLSSFRM;         // LCS LAN Statistics SNA Command Frame
struct  _LCSIPMPAIR;        // LCS IP Multicast Pair structure
struct  _LCSIPMFRM;         // LCS Set IP Multicast Command Frame
struct  _LCSETHFRM;         // LCS Ethernet Passthru Frame
struct  _LCSATTN;           // LCS Attention Required
struct  _LCSOCTL;           // LCS SNA Outbound Control
struct  _LCSICTL;           // LCS SNA Inbound Control
struct  _LCSBAF1;           // LCS SNA baffle 1
struct  _LCSBAF2;           // LCS SNA baffle 2
struct  _LCSIBH;            // LCS SNA Inbound Buffer Header
struct  _LCSCONN;           // LCS SNA Connection

typedef struct  _LCSBLK     LCSBLK,     *PLCSBLK;
typedef struct  _LCSDEV     LCSDEV,     *PLCSDEV;
typedef struct  _LCSPORT    LCSPORT,    *PLCSPORT;
typedef struct  _LCSRTE     LCSRTE,     *PLCSRTE;
typedef struct  _LCSHDR     LCSHDR,     *PLCSHDR;
typedef struct  _LCSCMDHDR  LCSCMDHDR,  *PLCSCMDHDR;
typedef struct  _LCSSTDFRM  LCSSTDFRM,  *PLCSSTDFRM;
typedef struct  _LCSSTRTFRM LCSSTRTFRM, *PLCSSTRTFRM;
typedef struct  _LCSQIPFRM  LCSQIPFRM,  *PLCSQIPFRM;
typedef struct  _LCSLSTFRM  LCSLSTFRM,  *PLCSLSTFRM;
typedef struct  _LCSLSSFRM  LCSLSSFRM,  *PLCSLSSFRM;
typedef struct  _LCSIPMPAIR LCSIPMPAIR, *PLCSIPMPAIR;
typedef struct  _LCSIPMFRM  LCSIPMFRM,  *PLCSIPMFRM;
typedef struct  _LCSETHFRM  LCSETHFRM,  *PLCSETHFRM;
typedef struct  _LCSATTN    LCSATTN,    *PLCSATTN;
typedef struct  _LCSOCTL    LCSOCTL,    *PLCSOCTL;
typedef struct  _LCSICTL    LCSICTL,    *PLCSICTL;
typedef struct  _LCSBAF1    LCSBAF1,    *PLCSBAF1;
typedef struct  _LCSBAF2    LCSBAF2,    *PLCSBAF2;
typedef struct  _LCSIBH     LCSIBH,     *PLCSIBH;
typedef struct  _LCSCONN    LCSCONN,    *PLCSCONN;


// --------------------------------------------------------------------
// External Declarations
// --------------------------------------------------------------------

extern int      CTCX_Init( DEVBLK* pDEVBLK, int argc, char *argv[] );
extern int      CTCX_Close( DEVBLK* pDEVBLK );
extern void     CTCX_Query( DEVBLK* pDEVBLK, char** ppszClass,
                            int     iBufLen, char*  pBuffer );
extern void     CTCX_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                                 BYTE    bFlags,  BYTE  bChained,
                                 U32     sCount,  BYTE  bPrevCode,
                                 int     iCCWSeq, BYTE* pIOBuf,
                                 BYTE*   pMore,   BYTE* pUnitStat,
                                 U32*    pResidual );

extern int      CTCE_Close( DEVBLK* pDEVBLK );
extern void     CTCE_Query( DEVBLK* pDEVBLK, char** ppszClass,
                            int     iBufLen, char*  pBuffer );

extern int      CTCI_Init( DEVBLK* pDEVBLK, int argc, char *argv[] );
extern int      CTCI_Close( DEVBLK* pDEVBLK );
extern void     CTCI_Query( DEVBLK* pDEVBLK, char** ppszClass,
                            int     iBufLen, char*  pBuffer );
extern void     CTCI_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                                 BYTE    bFlags,  BYTE  bChained,
                                 U32     sCount,  BYTE  bPrevCode,
                                 int     iCCWSeq, BYTE* pIOBuf,
                                 BYTE*   pMore,   BYTE* pUnitStat,
                                 U32*    pResidual );

extern void     CTCI_Read( DEVBLK* pDEVBLK,   U32   sCount,
                           BYTE*   pIOBuf,    BYTE* UnitStat,
                           U32*    pResidual, BYTE* pMore );
extern void     CTCI_Write( DEVBLK* pDEVBLK,   U32   sCount,
                            BYTE*   pIOBuf,    BYTE* UnitStat,
                            U32*    pResidual );

extern int      LCS_Init( DEVBLK* pDEVBLK, int argc, char *argv[] );
extern int      LCS_Close( DEVBLK* pDEVBLK );
extern void     LCS_Query( DEVBLK* pDEVBLK, char** ppszClass,
                           int     iBufLen, char*  pBuffer );
extern void     LCS_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                                BYTE    bFlags,  BYTE  bChained,
                                U32     sCount,  BYTE  bPrevCode,
                                int     iCCWSeq, BYTE* pIOBuf,
                                BYTE*   pMore,   BYTE* pUnitStat,
                                U32*    pResidual );

extern void     packet_trace( BYTE *addr, int len, BYTE dir );



/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **              STANDARD ETHERNET FRAMES LAYOUT                     **
 **                                                                  **
 **********************************************************************
\**********************************************************************/


// --------------------------------------------------------------------
// Ethernet Frame Header                (network byte order)
// --------------------------------------------------------------------

struct _ETHFRM
{
    MAC         bDestMAC;                //  +0
    MAC         bSrcMAC;                 //  +6
    HWORD       hwEthernetType;          //  +C  (see below #defines)
    BYTE        bData[FLEXIBLE_ARRAY];   //  +E
} ATTRIBUTE_PACKED;


typedef struct _ETHFRM ETHFRM, *PETHFRM;


// To quote Wikipedia "The EtherType field is two octets long
// and it can be used for two different purposes. Values of 1500
// and below mean that it is used to indicate the size of the
// payload in octets, while values of 1536 and above indicate
// that it is used as an EtherType, to indicate which protocol
// is encapsulated in the payload of the frame."
#define  ETH_TYPE           0x0600     // 0x0600 = 1536

#define  ETH_TYPE_IP        0x0800
#define  ETH_TYPE_ARP       0x0806
#define  ETH_TYPE_RARP      0x8035
#define  ETH_TYPE_SNA       0x80D5     // IBM SNA Service over Ethernet
#define  ETH_TYPE_IPV6      0x86dd
#define  ETH_TYPE_VLANTAG   0x8100

#define  ETH_LLC_SNAP_SIZE  8


// --------------------------------------------------------------------
// IP Version 4 Frame Header (Type 0x0800)  (network byte order)
// --------------------------------------------------------------------

struct  _IP4FRM
{
    BYTE        bVersIHL;                //  +0  Vers:4, IHL:4
    BYTE        bTOS;                    //  +1
    HWORD       hwTotalLength;           //  +2
    HWORD       hwIdentification;        //  +4
    U16         bFlagsFragOffset;        //  +6  Flags:3, FragOffset:13
    BYTE        bTTL;                    //  +8
    BYTE        bProtocol;               //  +9
    HWORD       hwChecksum;              //  +A
    U32         lSrcIP;                  //  +C
    U32         lDstIP;                  // +10
    BYTE        bData[FLEXIBLE_ARRAY];   // +14
} ATTRIBUTE_PACKED;


typedef struct _IP4FRM IP4FRM, *PIP4FRM;


// --------------------------------------------------------------------
// Address Resolution Protocol Frame (Type 0x0806) (network byte order)
// Reverse Address Resolution Protocol Frame (Type 0x8035) (network bo)
// --------------------------------------------------------------------

struct  _ARPFRM
{
    HWORD       hwHardwareType;          //  +0
    HWORD       hwProtocolType;          //  +2
    BYTE        bHardwareSize;           //  +4
    BYTE        bProtocolSize;           //  +5
    HWORD       hwOperation;             //  +6
    MAC         bSendEthAddr;            //  +8
    U32         lSendIPAddr;             // +12
    MAC         bTargEthAddr;            // +16
    U32         lTargIPAddr;             // +1C
} ATTRIBUTE_PACKED;


typedef struct _ARPFRM ARPFRM, *PARPFRM;


#define  ARP_REQUEST        0x01
#define  ARP_REPLY          0x02
#define  RARP_REQUEST       0x03
#define  RARP_REPLY         0x04


// --------------------------------------------------------------------
// IP Version 6 Frame Header (Type 0x86DD)  (network byte order)
// --------------------------------------------------------------------

struct  _IP6FRM
{
    BYTE      bVersTCFlow[4];            //  +0  Vers:4, TC:8 FlowID:20
    BYTE      bPayloadLength[2];         //  +4
    BYTE      bNextHeader;               //  +6  (same as IPv4 Protocol)
    BYTE      bHopLimit;                 //  +7
    BYTE      bSrcAddr[16];              //  +8
    BYTE      bDstAddr[16];              // +18
    BYTE      bPayload[FLEXIBLE_ARRAY];  // +28  The payload
} ATTRIBUTE_PACKED;

typedef struct _IP6FRM IP6FRM, *PIP6FRM;


/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **                  CTCI DEVICE CONTROL BLOCKS                      **
 **                                                                  **
 **********************************************************************
\**********************************************************************/


#define MAX_CTCI_FRAME_SIZE( pCTCBLK )      \
    (                                       \
        pCTCBLK->iMaxFrameBufferSize  /* (whatever CTCI_Init defined) */  \
        - sizeof( CTCIHDR )                 \
        - sizeof( CTCISEG )                 \
        - sizeof_member(CTCIHDR,hwOffset)   \
    )


#define MAX_LCS_ETH_FRAME_SIZE( pLCSDEV )   \
    (                                       \
        pLCSDEV->iMaxFrameBufferSize  /* (whatever LCS_Startup defined) */  \
        - sizeof( LCSETHFRM )               \
        - sizeof_member(LCSHDR,hwOffset)    \
    )

// PROGRAMMING NOTE: the following frame buffer size should always be
// no smaller than the maximum frame buffer size possible for an LCS
// device (currently hard-coded in S390 Linux to be 0x5000 via the
// #define LCS_IOBUFFERSIZE). Also note that the minimum and maximum
// frame buffer size, according to IBM documentation, is 16K to 64K.

#define CTC_MIN_FRAME_BUFFER_SIZE (0x4000)  // Min. frame buffer size
#define CTC_DEF_FRAME_BUFFER_SIZE (0x5000)  // Def. frame buffer size
#define CTC_MAX_FRAME_BUFFER_SIZE (0xFFFF)  // Max. frame buffer size

#define CTC_DELAY_USECS           (100)     // 100 microseconds delay;
                                            // used mostly by enqueue
                                            // frame buffer delay loop.

struct  _CTCBLK;
struct  _CTCIHDR;
struct  _CTCISEG;

typedef struct _CTCBLK  CTCBLK, *PCTCBLK;
typedef struct _CTCIHDR CTCIHDR,*PCTCIHDR;
typedef struct _CTCISEG CTCISEG,*PCTCISEG;


// --------------------------------------------------------------------
// CTCBLK -                                (host byte order)
// --------------------------------------------------------------------

struct  _CTCBLK
{
    int         fd;                       // TUN/TAP fd
    TID         tid;                      // Read Thread ID
    pid_t       pid;                      // Read Thread pid

    DEVBLK*     pDEVBLK[2];               // 0 - Read subchannel
                                          // 1 - Write subchannel

#define CTC_READ_SUBCHANN    0            // 0 - Read subchannel
#define CTC_WRITE_SUBCHANN   1            // 1 - Write cubchannel

    U16         iMaxFrameBufferSize;      // Device Buffer Size
    BYTE        bFrameBuffer[CTC_DEF_FRAME_BUFFER_SIZE]; // (this really SHOULD be dynamically allocated!)
    U16         iFrameOffset;             // Curr Offset into Buffer
    U16         sMTU;                     // Max MTU

    LOCK        Lock;                     // Data LOCK
    LOCK        EventLock;                // Condition LOCK
    COND        Event;                    // Condition signal

    u_int       fDebug:1;                 // Debugging
    u_int       fOldFormat:1;             // Old Config Format
    u_int       fCreated:1;               // Interface Created
    u_int       fStarted:1;               // Startup Received
    u_int       fDataPending:1;           // Data is pending for read device
    u_int       fCloseInProgress:1;       // Close in progress
    u_int       fPreconfigured:1;         // TUN device pre-configured
    u_int       fReadWaiting:1;           // CTCI_Read waiting
    u_int       fHaltOrClear:1;           // HSCH or CSCH issued

    int         iKernBuff;                // Kernel buffer in K bytes.
    int         iIOBuff;                  // I/O buffer in K bytes.
    char        szGuestIPAddr[32];        // IP Address (Guest OS)
    char        szDriveIPAddr[32];        // IP Address (Driver)
    char        szNetMask[32];            // Netmask for P2P link
    char        szMTU[32];
    char        szTUNCharDevName[256];    // TUN/TAP special char device filename (/dev/net/tun)
    char        szTUNIfName[IFNAMSIZ];    // Network Interface Name (e.g. tun0)
    char        szMACAddress[32];         // MAC Address
};



/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **                   CTCI DEVICE FRAMES                             **
 **                                                                  **
 **********************************************************************
\**********************************************************************/


// --------------------------------------------------------------------
// CTCI Block Header                    (host byte order)
// --------------------------------------------------------------------

struct _CTCIHDR                         // CTCI Block Header
{
    HWORD   hwOffset;                   // Offset of next block
    BYTE    bData[FLEXIBLE_ARRAY];      // start of data (CTCISEG)
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// CTCI Segment Header                  (host byte order)
// --------------------------------------------------------------------

struct _CTCISEG                         // CTCI Segment Header
{
    HWORD   hwLength;                   // Segment length including
                                        //   this header
    HWORD   hwType;                     // Ethernet packet type
    HWORD   _reserved;                  // Unused, set to zeroes
    BYTE    bData[FLEXIBLE_ARRAY];      // Start of data (IP pakcet)
} ATTRIBUTE_PACKED;



/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **                  LCS DEVICE CONTROL BLOCKS                       **
 **                                                                  **
 **********************************************************************
\**********************************************************************/

// --------------------------------------------------------------------
// LCS SNA Outbound Control                     (network byte order)
// Goes from the guest (i.e. VTAM) to the XCA (i.e. LCS)
// --------------------------------------------------------------------

struct _LCSOCTL                           // LCS SNA Outbound Control
{
    U16         XCNOBUFC;
    BYTE        XCNOSTAT;
#define  XCNOXIDF  0x80
#define  XCNOERRF  0x40
#define  XCNORSVD  0x20
#define  XCNOSEGF  0x08
#define  XCNOSGST  0x0C
#define  XCNOSEGL  0x04
#define  XCNOSLOW  0x02
#define  XCNOMAXW  0x01
    BYTE        XCNOFMT;
    union _onp
    {
        U16     XCNONUMS;
        U16     XCNOPGCT;
    } np;
    union _onh
    {
        U16     XCNONUMR;
        U16     XCNOHDSZ;
    } nh;
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// LCS SNA Inbound Control                      (network byte order)
// Goes from the XCA (i.e. LCS) to the guest (i.e. VTAM)
// --------------------------------------------------------------------

struct _LCSICTL                           // LCS SNA Inbound Control
{
    U16         XCNIBUFC;
    BYTE        XCNISTAT;
#define  XCNIXIDF  0x80
#define  XCNIERRF  0x40
#define  XCNIRSVD  0x08
#define  XCNISLOW  0x02
#define  XCNIMAXW  0x01
    BYTE        XCNIFMT;
    union _inp
    {
        U16     XCNINUMS;
        U16     XCNIPGCT;
    } np;
    U16         XCNINUMR;
} ATTRIBUTE_PACKED;


// The first 8-bytes of the reply are copied to XCNCB + 2F8
//  SNA Data Areas Volume 1, Cross-Channel Node Control Block (XCNCB)
//
//  The 8-byte control area that follows is an external
//  interface to other devices connected to VTAM via a channel.
//  Bits should not randomly be added to this structure. Also,
//  bits added to the input structure (XCNICTL) should be
//  duplicated in the output structure (XCNOCTL).
//  ====================================================================================
//  760 (2F8) CHARACTER  8 XCNICTL   INPUT CONTROL AREA
//  760 (2F8) UNSIGNED   2 XCNIBUFC  Number of buffers received in
//                                   last WRITE operation (unpacked
//                                   format) or number of data
//                                   bytes received in last WRITE
//                                   operation (packed format)
//  762 (2FA) BITSTRING  2 XCNISTAT  INPUT STATUS AREA - IF 0,
//                                   NORMAL DATA TRANSFER HAS
//                                   OCCURRED
//            1... ....    XCNIXIDF  + XID TRANSFER (IF XCNIERRF IS      <<< ISTTSCXE tests
//                                   ALSO ON, DISCONTACTING)
//            .1.. ....    XCNIERRF  + ERROR RETRY OF LAST               <<<      "
//                                   TRANSMISSION
//            ..11 ....    *         NOT USED- AVAILABLE
//            .... 1...    *         RESERVED                            <<<      "
//            .... .1..    *         NOT USED - AVAILABLE
//            .... ..1.    XCNISLOW  + SLOWDOWN - OUTPUT DATA HAS        <<<      "
//                                   NOT BEEN RECEIVED
//            .... ...1    XCNIMAXW  + THE NEXT TRANSMISSION             <<<      "
//                                   REQUIRES THE MAXIMUM NUMBER OF
//                                   BUFFERS AVAILABLE IN THIS SIDE
//                                   OF THE ADAPTER
//  763 (2FB) UNSIGNED   1 XCNIFMT   + CONTROL FORMAT (VALID ONLY
//                                   DURING XID EXCHANGE)
//  764 (2FC) SIGNED     2 XCNINUMS  INCREMENTAL COUNT OF NUMBER OF
//                                   BUFFERS SENT (USED FOR DATA
//                                   LINK CONTROL INTEGRITY)
//  764 (2FC) SIGNED     2 XCNIPGCT  DURING XID EXCHANGE, THIS
//                                   FIELD WILL REPRESENT THE
//                                   NUMBER OF PAGES TO BE
//                                   ALLOCATED FOR THE CTC BUFFER.
//                                   AFTER XID IS COMPLETE, IT WILL
//                                   BE CLEARED FOR BEGINNING DATA
//                                   TRANSFER
//  766 (2FE) SIGNED     2 XCNINUMR  INCREMENTAL COUNT OF NUMBER OF
//                                   BUFFERS RECEIVED (USED FOR
//                                   DATA LINK CONTROL INTEGRITY)
//
//
//  The 8-byte control area that follows is an external
//  interface to other devices connected to VTAM via a channel.
//  See XCNICTL for additional information.
//  ====================================================================================
//  768 (300) CHARACTER  8 XCNOCTL   OUTPUT CONTROL AREA
//  768 (300) UNSIGNED   2 XCNOBUFC  Number of buffers transmitted
//                                   in last WRITE operation when
//                                   unpacked format is used (the
//                                   number of READ buffers which
//                                   will be used by the other side
//                                   of the adapter), or number of
//                                   data bytes transmitted in last
//                                   WRITE operation when packed
//                                   format is used
//  770 (302) BITSTRING  2 XCNOSTAT  OUTPUT STATUS AREA - IF 0,
//                                   NORMAL DATA TRANSFER WILL
//                                   OCCUR
//            1... ....    XCNOXIDF  + XID TRANSFER (IF XCNOERRF IS
//                                   ALSO ON, DISCONTACTING)
//            .1.. ....    XCNOERRF  + ERROR RETRY OF LAST
//                                   TRANSMISSION
//            ..1. ....    *         RESERVED
//            ...1 ....    *         NOT USED - AVAILABLE
//            .... 11..    XCNOSGST  OUTPUT SEGMENTATION STATE
//            .... 1...    XCNOSEGF  FIRST SEGMENT
//            .... .1..    XCNOSEGL  LAST SEGMENT
//            .... ..1.    XCNOSLOW  + SLOWDOWN - INPUT DATA HAS
//                                   NOT BEEN RECEIVED
//            .... ...1    XCNOMAXW  + THE NEXT TRANSMISSION
//                                   REQUIRES THE MAXIMUM NUMBER OF
//                                   BUFFERS AVAILABLE IN THE OTHER
//                                   SIDE OF THE ADAPTER
//  771 (303) UNSIGNED   1 XCNOFMT   + CONTROL FORMAT (VALID ONLY
//                                   DURING XID EXCHANGE)
//  772 (304) SIGNED     2 XCNONUMS  INCREMENTAL COUNT OF NUMBER OF
//                                   BUFFERS SENT (USED FOR DATA
//                                   LINK CONTROL INTEGRITY)
//  772 (304) SIGNED     2 XCNOPGCT  DURING XID EXCHANGE, THIS
//                                   FIELD WILL REPRESENT THE
//                                   NUMBER OF PAGES TO BE
//                                   ALLOCATED FOR THE CTC BUFFER.
//                                   AFTER XID IS COMPLETE, IT WILL
//                                   BE CLEARED FOR BEGINNING DATA
//                                   TRANSFER
//  774 (306) SIGNED     2 XCNONUMR  INCREMENTAL COUNT OF NUMBER OF
//                                   BUFFERS RECEIVED (USED FOR
//                                   DATA LINK CONTROL INTEGRITY)
//  774 (306) SIGNED     2 XCNOHDSZ  DURING XID EXCHANGE, THIS
//                                   FIELD WILL REPRESENT THE SIZE
//                                   OF THE SMS HEADER IN USE BY
//                                   THIS HOST. AFTER XID IS
//                                   COMPLETE, IT WILL BE CLEARED
//                                   FOR DATA TRANSFER
//


// --------------------------------------------------------------------
// - baffle 1 and baffle 2 are always seen together.
// - baffle 1 seems to have a minimum length of 12 bytes, but is often
//   longer, with a maximum seen length of 34 bytes. Seen length has
//   always been a even number.
// - baffle 2 seems to have a minimum length of 3 bytes, but is often
//   longer, with a maximum seen length of 44 bytes.
// - The seen total length of baffle 1 and baffle 2 has always been an
//   even number, a multiple of 2.
// --------------------------------------------------------------------


// --------------------------------------------------------------------
// LCS SNA baffle 1                             (network byte order)
// --------------------------------------------------------------------

struct _LCSBAF1                        // LCS SNA baffle 1
{
    HWORD       hwLenBaf1;             //  0  Length of baffle 1
    HWORD       hwTypeBaf;             //  2  Type of baffle
    HWORD       hwLenBaf2;             //  4  Length of baffle 2
    BYTE        bByte06;               //  6    bit value
    BYTE        bByte07;               //  7    byte value
    BYTE        bTokenA[4];            //  8
    BYTE        bTokenB[4];            //  C
    BYTE        bByte16;               // 10
    BYTE        bByte17;               // 11
    BYTE        bByte18[2];            // 12
    BYTE        bByte20[4];            // 14
    BYTE        bTokenC[4];            // 18
    BYTE        bByte28[4];            // 1C
    BYTE        bByte32[2];            // 20
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// LCS SNA baffle 2                             (network byte order)
// --------------------------------------------------------------------

struct _LCSBAF2                        // LCS SNA baffle 2
{
    BYTE        bByte00;               //  0  Always seems to contain 0x01.
    HWORD       hwSeqNum;              //  1  Sequence number
    BYTE        bByte03[2];            //  3
    BYTE        bByte05;               //  5
    BYTE        bByte06;               //  6
    BYTE        bByte07;               //  7
    BYTE        bByte08;               //  8
    BYTE        bByte09;               //  9
    BYTE        bByte10;               //  A
    BYTE        bByte11;               //  B
    BYTE        bByte12[5];            //  C
    BYTE        bByte17;               // 11
    BYTE        bByte18[5];            // 12
    BYTE        bByte23;               // 17
    BYTE        bByte24;               // 18
    BYTE        bByte25;               // 19
    BYTE        bByte26;               // 1A
    BYTE        bByte27;               // 1B
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// LCS SNA Inbound Buffer Header
// --------------------------------------------------------------------

struct _LCSIBH                         // LCS SNA Inbound Buffer Header
{
    PLCSIBH   pNextLCSIBH;             // Pointer to next LCSIBH
    int       iAreaLen;                // Data area length
    int       iDataLen;                // Data length
    BYTE      bData[FLEXIBLE_ARRAY];   //
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// LCS SNA Connection
// --------------------------------------------------------------------

struct _LCSCONN                        // LCS SNA Connection
{
    PLCSCONN  pNextLCSCONN;            // Pointer to next LCSCONN
    BYTE      bInToken[4];             // Inbound Token. VTAM tells LCS that
                                       // it will use this token for inbound.
    BYTE      bOutToken[4];            // Outbound Token. LCS tells VTAM that
                                       // it will use this token for outbound.
    MAC       bLocalMAC;               // Local MAC address
    MAC       bRemoteMAC;              // Remote MAC address
    U16       hwCreated;               //
#define LCSCONN_CREATED_INBOUND 1
#define LCSCONN_CREATED_OUTBOUND 2
    U16       hwNotUsed;               //
    U16       hwXIDSeqNum;             // XID Exchange Sequence number
    U16       hwDataSeqNum;            // Data Sequence number
    U16       hwLocalNS;               // Local NS
    U16       hwLocalNR;               // Local NR
    U16       hwRemoteNS;              // Remote NS
    U16       hwRemoteNR;              // Remote NR
    u_int     fIframe:1;               // I-frame received

};


// --------------------------------------------------------------------
// LCS Device                                   (host byte order)
// --------------------------------------------------------------------

struct  _LCSDEV
{
    PLCSDEV     pNext;                  // -> Next device LSCDEV

    PLCSBLK     pLCSBLK;                // -> LCSBLK
    DEVBLK*     pDEVBLK[2];             // 0 - Read subchannel
                                        // 1 - Write cubchannel

#define LCS_READ_SUBCHANN    0          // 0 - Read subchannel
#define LCS_WRITE_SUBCHANN   1          // 1 - Write cubchannel

    U16         sAddr;                  // Device Base Address
    BYTE        bMode;                  // (see below #defines)
    BYTE        bPort;                  // Relative Adapter No.
    BYTE        bType;                  // (see below #defines)
    char*       pszIPAddress;           // IP Address (string)

    U32         lIPAddress;             // IP Address (binary)
                                        // (network byte order)

    LOCK        DevEventLock;           // Condition LOCK
    COND        DevEvent;               // Condition signal

    u_int       fDevCreated:1;          // DEVBLK(s) Created
    u_int       fDevStarted:1;          // Device Started
    u_int       fRouteAdded:1;          // Routing Added
    u_int       fReplyPending:1;        // Cmd Reply is Pending
    u_int       fDataPending:1;         // Data is Pending
    u_int       fReadWaiting:1;         // LCS_Read waiting
    u_int       fHaltOrClear:1;         // HSCH or CSCH issued

    U16         hwOctlSize;             // SNA
    LCSOCTL     Octl;                   // SNA Outbound Control
                                        // (network byte order)
    U16         hwIctlSize;             // SNA
    LCSICTL     Ictl;                   // SNA Inbound Control
                                        // (network byte order)

    U16         hwTypeBaf;              // SNA LCSBAF1 type
    U16         hwSeqNumBaf;            // SNA LCSBAF2 sequence number
    u_int       fChanProgActive:1;      // SNA Channel Program Active
    u_int       fAttnRequired:1;        // SNA Attention Required
    u_int       fPendingIctl:1;         // SNA Pending has LCSICTL structure
    u_int       fReceiveFrames:1;       // SNA Receive Frames from Network
    u_int       fTuntapError:1;         // SNA TUNTAP_Write error
    int         iTuntapErrno;           // SNA TUNTAP_Write error number
    BYTE        bFlipFlop;              // SNA

    U16         hwInXIDSeqNum;          // SNA XID Exchange Sequence number
    U16         hwInDataSeqNum;         // SNA Data Sequence number

    LOCK        LCSIBHChainLock;        // SNA LCSIBH Chain LOCK
    PLCSIBH     pFirstLCSIBH;           // SNA First LCSIBH in chain
    PLCSIBH     pLastLCSIBH;            // SNA Last LCSIBH in chain

    LOCK        LCSCONNChainLock;       // SNA LCSCONN Chain LOCK
    PLCSCONN    pFirstLCSCONN;          // SNA First LCSCONN in chain

    LOCK        InOutLock;              // SNA Inbound Outbound LOCK

    LOCK        DevDataLock;            // Data LOCK. This lock is used to
                                        // serialize data being added to or
                                        // removed from bFrameBuffer.

    U16         iFrameOffset;           // Curr Offset into Buffer
    U16         iMaxFrameBufferSize;    // Device Buffer Size
    BYTE        bFrameBuffer[CTC_DEF_FRAME_BUFFER_SIZE]; // (this really SHOULD be dynamically allocated!)
};


#define LCSDEV_MODE_IP          0x01
#define LCSDEV_MODE_SNA         0x02


#define LCSDEV_TYPE_NONE        0x00
#define LCSDEV_TYPE_PRIMARY     0x01
#define LCSDEV_TYPE_SECONDARY   0x02


#define  WCTL  0x17          // Write Control
#define  SCB   0x14          // Sense Command Byte

// --------------------------------------------------------------------
// LCS Port (or Relative Adapter)               (host byte order)
// --------------------------------------------------------------------

struct  _LCSPORT
{
    BYTE        bPort;                    // Relative Adapter No
    BYTE        nMCastCount;              // Active MACTAB entries
    MAC         MAC_Address;              // MAC Address of Adapter
    PLCSRTE     pRoutes;                  // -> Routes chain
    PLCSBLK     pLCSBLK;                  // -> LCSBLK
    MACTAB      MCastTab[ MACTABMAX ];    // Multicast table

    U16         sIPAssistsSupported;      // (See #defines below)
    U16         sIPAssistsEnabled;        // (See #defines below)

    LOCK        PortDataLock;             // Data LOCK
    LOCK        PortEventLock;            // Condition LOCK
    COND        PortEvent;                // Condition signal

    u_int       fUsed:1;                  // Port is used
    u_int       fLocalMAC:1;              // MAC is specified in OAT
    u_int       fPortCreated:1;           // Interface Created
    u_int       fPortStarted:1;           // Startup Received
    u_int       fRouteAdded:1;            // Routing Added
    u_int       fCloseInProgress:1;       // Close in progress
    u_int       fPreconfigured:1;         // TAP device pre-configured
    u_int       fDoCkSumOffload:1;        // Do manual CSUM Offload
    u_int       fDoMCastAssist:1;         // Do manual MCAST Assist

    int         fd;                       // TUN/TAP fd
    TID         tid;                      // Read Thread ID
    pid_t       pid;                      // Read Thread pid
    int         icDevices;                // Device count
    char        szNetIfName[IFNAMSIZ];    // Network Interface Name (e.g. tap0)
    char        szMACAddress[32];         // MAC Address
    char        szGWAddress[32];          // Gateway for W32
};

// --------------------------------------------------------------------
// LCS Assists flags
// --------------------------------------------------------------------

#define LCS_ARP_PROCESSING            0x0001
#define LCS_INBOUND_CHECKSUM_SUPPORT  0x0002
#define LCS_OUTBOUND_CHECKSUM_SUPPORT 0x0004
#define LCS_IP_FRAG_REASSEMBLY        0x0008
#define LCS_IP_FILTERING              0x0010
#define LCS_IP_V6_SUPPORT             0x0020
#define LCS_MULTICAST_SUPPORT         0x0040


// --------------------------------------------------------------------
// LCSRTE - Routing Entries                     (host byte order)
// --------------------------------------------------------------------

struct  _LCSRTE
{
    char*       pszNetAddr;
    char*       pszNetMask;
    PLCSRTE     pNext;
};


// --------------------------------------------------------------------
// LCS Attention Required                       (host byte order)
// --------------------------------------------------------------------

struct _LCSATTN                           // LCS Attention Required
{
    PLCSATTN    pNext;                    // -> Next in chain
    PLCSDEV     pDevice;                  // -> Device
};


// --------------------------------------------------------------------
// LCSBLK - Common Storage for LCS Emulation    (host byte order)
// --------------------------------------------------------------------

struct  _LCSBLK
{
    char*       pszTUNDevice;             // TUN/TAP char device
    char*       pszOATFilename;           // OAT Filename
    char*       pszIPAddress;             // IP Address

    u_int       fDebug:1;
    u_int       fCloseInProgress:1;       // Close in progress
#if defined( OPTION_W32_CTCI )
    u_int       fNoMultiWrite:1;          // CTCI-WIN v3.3+ WinPCap v4.1+
#endif
    int         icDevices;                // Number of devices
    int         iKernBuff;                // Kernel buffer in K bytes.
    int         iIOBuff;                  // I/O buffer in K bytes.
    int         iTraceLen;                // Maximum data to be traced in bytes.
#define LCS_TRACE_LEN_ZERO    0
#define LCS_TRACE_LEN_MINIMUM 64
#define LCS_TRACE_LEN_DEFAULT 128
#define LCS_TRACE_LEN_MAXIMUM 65535
    int         iDiscTrace;               // Maximum discard to be traced in bytes.
#define LCS_DISC_TRACE_ZERO    0
#define LCS_DISC_TRACE_MINIMUM 16
#define LCS_DISC_TRACE_MAXIMUM 65535

    LOCK        AttnLock;                 // Attention LOCK
    PLCSATTN    pAttns;                   // -> Attention chain

    LOCK        AttnEventLock;            // Attention event LOCK
    COND        AttnEvent;                // Attention event signal

    TID         AttnTid;                  // Attention Thread ID
    pid_t       AttnPid;                  // Attention Thread pid

    PLCSDEV     pDevices;                 // -> Device chain
    LCSPORT     Port[LCS_MAX_PORTS];      // Port Blocks
};


/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **                   LCS DEVICE FRAMES                              **
 **                                                                  **
 **********************************************************************
\**********************************************************************/


// --------------------------------------------------------------------
// LCS Frame Header                             (network byte order)
// --------------------------------------------------------------------

struct _LCSHDR      // *ALL* LCS Frames start with the following header
{
    HWORD       hwOffset;               //  +0  Offset to next frame or 0
    BYTE        bType;                  //  +2  (see below #defines)
    BYTE        bSlot;                  //  +3  (i.e. port)
} ATTRIBUTE_PACKED;


#define  LCS_FRMTYP_CMD     0x00        // LCS command mode
#define  LCS_FRMTYP_ENET    0x01        // Ethernet Passthru
#define  LCS_FRMTYP_TR      0x02        // Token Ring
#define  LCS_FRMTYP_FDDI    0x07        // FDDI
#define  LCS_FRMTYP_AUTO    0xFF        // auto-detect

#define  LCS_FRMTYP_SNA     0x04        // SNA ?

// --------------------------------------------------------------------
// LCS Command Frame Header                     (network byte order)
// --------------------------------------------------------------------

struct _LCSCMDHDR    // All LCS *COMMAND* Frames start with this header
{
    LCSHDR      bLCSHdr;                //  +0  LCS Frame header

    BYTE        bCmdCode;               //  +4  (see below #defines)
    BYTE        bInitiator;             //  +5  (see below #defines)
    HWORD       hwSequenceNo;           //  +6
    HWORD       hwReturnCode;           //  +8

    BYTE        bLanType;               //  +A  usually LCS_FRMTYP_ENET
    BYTE        bRelAdapterNo;          //  +B  (i.e. port)
} ATTRIBUTE_PACKED;


#define  LCS_CMD_TIMING         0x00        // Timing request
#define  LCS_CMD_STRTLAN        0x01        // Start LAN
#define  LCS_CMD_STOPLAN        0x02        // Stop LAN
#define  LCS_CMD_GENSTAT        0x03        // Generate Stats
#define  LCS_CMD_LANSTAT        0x04        // LAN Stats
#define  LCS_CMD_LISTLAN        0x06        // List LAN
#define  LCS_CMD_STARTUP        0x07        // Start Host
#define  LCS_CMD_SHUTDOWN       0x08        // Shutdown Host
#define  LCS_CMD_LISTLAN2       0x0B        // List LAN (another version)
#define  LCS_CMD_QIPASSIST      0xB2        // Query IP Assists
#define  LCS_CMD_SETIPM         0xB4        // Set IP Multicast
#define  LCS_CMD_DELIPM         0xB5        // Delete IP Multicast

#define  LCS_CMD_STRTLAN_SNA    0x41        // Start LAN SNA
#define  LCS_CMD_STOPLAN_SNA    0x42        // Stop LAN SNA
#define  LCS_CMD_LANSTAT_SNA    0x44        // LAN Stats SNA

#define  LCS_INITIATOR_TCPIP    0x00        // TCP/IP
#define  LCS_INITIATOR_LGW      0x01        // LAN Gateway

#define  LCS_INITIATOR_SNA      0x80        // SNA

// --------------------------------------------------------------------
// LCS Standard Command Frame                   (network byte order)
// --------------------------------------------------------------------

struct _LCSSTDFRM
{
    LCSCMDHDR   bLCSCmdHdr;             // LCS Command Frame header

    HWORD       hwParameterCount;       //  +C
    BYTE        bOperatorFlags[3];      //  +E
    BYTE        _reserved[3];           // +11
    BYTE        bData[FLEXIBLE_ARRAY];  // +14
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// LCS Startup & Start LAN Command Frames       (network byte order)
// --------------------------------------------------------------------

struct _LCSSTRTFRM
{
    LCSCMDHDR   bLCSCmdHdr;             //  +0  LCS Command Frame header

    HWORD       hwBufferSize;           //  +C  For IP this is a buffer
                                        //      size that comes from the
                                        //      stack. For SNA it is
                                        //      something unknown that
                                        //      goes to VTAM (which checks
                                        //      for >= 0x0200, or == 0x02FF).
    BYTE        _unused[6];             //  +E
    FWORD       fwReadLength;           // +14  Length for Read CCW (0x0800 to 0xFFFF).
} ATTRIBUTE_PACKED;                     // +18


// --------------------------------------------------------------------
// LCS Query IP Assists Command Frame           (network byte order)
// --------------------------------------------------------------------

struct  _LCSQIPFRM
{
    LCSCMDHDR   bLCSCmdHdr;             // LCS Command Frame header

    HWORD       hwNumIPPairs;
    HWORD       hwIPAssistsSupported;   // (See "LCS Assists" above)
    HWORD       hwIPAssistsEnabled;     // (See "LCS Assists" above)
    HWORD       hwIPVersion;
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// LCS LAN Statistics Command Frames            (network byte order)
// --------------------------------------------------------------------

struct  _LCSLSTFRM                      // LAN Statistics for IP
{
    LCSCMDHDR   bLCSCmdHdr;             //  +0  LCS Command Frame header

    BYTE        _unused1[10];           //  +C
    MAC         MAC_Address;            // +16  MAC Address of Adapter
    FWORD       fwPacketsDeblocked;     // +1C
    FWORD       fwPacketsBlocked;       // +20
    FWORD       fwTX_Packets;           // +24
    FWORD       fwTX_Errors;            // +28
    FWORD       fwTX_PacketsDiscarded;  // +2C
    FWORD       fwRX_Packets;           // +30
    FWORD       fwRX_Errors;            // +34
    FWORD       fwRX_DiscardedNoBuffs;  // +38
    U32         fwRX_DiscardedTooLarge; // +3C
} ATTRIBUTE_PACKED;                     // +40

struct  _LCSLSSFRM                      // LAN Statistics for SNA
{
    LCSCMDHDR   bLCSCmdHdr;             //  +0  LCS Command Frame header

    BYTE        bUnknown1;              //  +C  The bytes from +D to +19 are
                                        //      probably a structure, and
                                        //      this byte contains a count
                                        //      of the structures.
    BYTE        bUnknown2;              //  +D   Adaptor type?  0x04 = Ethernet  0x02 = Token Ring
    BYTE        bUnknown3;              //  +E   Adaptor number?  i.e. port
    BYTE        bUnknown4;              //  +F
    BYTE        _unused1[2];            // +10
    BYTE        bMACsize;               // +12  Length of MAC Address.
    MAC         MAC_Address;            // +13  MAC Address.
    BYTE        _unused2[1];            // +19
} ATTRIBUTE_PACKED;                     // +1A


// --------------------------------------------------------------------
// LCS Set IP Multicast Command Frame           (network byte order)
// --------------------------------------------------------------------

struct  _LCSIPMPAIR
{
    U32         IP_Addr;
    MAC         MAC_Address;            // MAC Address of Adapter
    BYTE        _reserved[2];
} ATTRIBUTE_PACKED;

#define MAX_IP_MAC_PAIRS      32

struct  _LCSIPMFRM
{
    LCSCMDHDR   bLCSCmdHdr;             // LCS Command Frame header

    HWORD       hwNumIPPairs;
    U16         hwIPAssistsSupported;
    U16         hwIPAssistsEnabled;
    U16         hwIPVersion;
    LCSIPMPAIR  IP_MAC_Pair[MAX_IP_MAC_PAIRS];
    U32         fwResponseData;
} ATTRIBUTE_PACKED;


// --------------------------------------------------------------------
// LCS Ethernet Passthru Frame                  (network byte order)
// --------------------------------------------------------------------

struct  _LCSETHFRM
{
    LCSHDR      bLCSHdr;                // LCS Frame header
    BYTE        bData[FLEXIBLE_ARRAY];  // Ethernet Frame
} ATTRIBUTE_PACKED;



/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **            Non-device specific structures                        **
 **                                                                  **
 **********************************************************************
\**********************************************************************/

//
// The LLC layer provides connectionless and conection-oriented data
// transfer.
//
// Connectionless data transfer is commonly referred to as LLC type 1,
// or LLC1. Connectionless service does not require you to establish
// data links or link stations. After a Service Access Point (SAP) has
// been enabled, the SAP can send and receive information to and from
// a remote SAP that also uses connectionless service. Connectionless
// service does not have any mode setting commands (such as SABME) and
// does not require that state information is maintained.
//
// Connection-oriented data transfer is referred to as LLC type 2, or
// LLC2. Connection-oriented service requires the establishment of
// link stations. When the link station is established, a mode setting
// command is necessary. Thereafter, each link station is responsible
// to maintain link state information.
//
// LLC2 is implemented whenever Systems Network Architecture (SNA)
// runs over a LAN or virtual LAN.
//
// Problems are rare in LLC1. In LLC2, two categories of problems can
// occur:
//   1. Sessions that do not establish
//   2. Established sessions that intermittently fail
//
// An LLC frame is called an LLC Protocol Data Unit (LPDU), and is
// formatted as follows:
//   DSAP (1 byte)
//   SSAP (1 byte)
//   Control Field (1 or 2 bytes)
//   Information Field (0 or more bytes)
//
// The LPDU includes two eight-bit address fields, called service
// access points (SAP), or collectively LSAP in the OSI terminology.
// Although the LSAP fields are 8 bits long, the low-order bit is
// reserved for special purposes, leaving only 128 values available
// for most purposes.
//
// DSAP (Destination SAP) is an 8-bit long field that represents the
// logical addresses of the network layer entity intended to receive
// the message. The low-order bit of the DSAP indicates whether it
// contains an individual or a group address:
// - if the low-order bit is 0, the remaining 7 bits of the DSAP
//   specify an individual address, which refers to a single local
//   service access point (LSAP) to which the packet should be
//   delivered.
// - if the low-order bit is 1, the remaining 7 bits of the DSAP
//   specify a group address, which refers to a group of LSAPs to
//   which the packet should be delivered.
//
// SSAP (Source SAP) is an 8-bit long field that represents the
// logical address of the network layer entity that has created the
// message. The low-order bit of the SSAP indicates whether the packet
// is a command or response packet:
// - if it's 0, the packet is a command packet, and
// - if it's 1, the packet is a response packet.
// The remaining 7 bits of the SSAP specify the LSAP (always an
// individual address) from which the packet was transmitted.
//
// LSAP numbers are globally assigned by the IEEE to uniquely identify
// well established international standards.
//

struct  _LPDU;
struct  _LLC;

typedef struct  _LPDU    LPDU,    *PLPDU;
typedef struct  _LLC     LLC,     *PLLC;

struct  _LPDU
{
    BYTE        bDSAP[1];
    BYTE        bSSAP[1];
    BYTE        bControl[2];
};

struct  _LLC
{
    LPDU        bLpdu;                 // LLC PDU DSAP, SSAP & Control
    BYTE        bInfo[6];              // LLC PDU Information
    U16         hwLpduSize;            // Size = 3 or 4
    U16         hwInfoSize;            // Size = 0 or 5.
    U16         hwDSAP;                // DSAP (Destination Service Access Point)
    U16         hwIG;                  // Individual or Group bit
    U16         hwSSAP;                // SSAP (Source Service Access Point)
    U16         hwCR;                  // Command or Response bit
    U16         hwNS;                  // NS count
    U16         hwNR;                  // NR count
    U16         hwPF;                  // Poll or Final bit
    U16         hwSS;                  // S bits, i.e. type of Supervisory
    U16         hwM;                   // M bits, i.e. type of Unnumbered
    U16         hwV;                   // FRMR V bit
    U16         hwZ;                   // FRMR Z bit
    U16         hwY;                   // FRMR Y bit
    U16         hwX;                   // FRMR X bit
    U16         hwW;                   // FRMR W bit
    U16         hwCF;                  // FRMR Control Field that caused reject
    U16         hwType;
};
#define SS_Receiver_Ready           0    // B'00'
#define SS_Receiver_Not_Ready       1    // B'01'
#define SS_Reject                   2    // B'10'
#define M_DM_Response               3    // B'00011' = DM Response (0x1F)
#define M_DISC_Command              8    // B'01000' = DISC Command (0x53)
#define M_UA_Response               12   // B'01100' = UA Response (0x73)
#define M_SABME_Command             15   // B'01111' = SABME Command (0x7F)
#define M_FRMR_Response             17   // B'10001' = FRMR Response (0x87)
#define M_XID_Command_or_Response   23   // B'10111'   XID Command or Response
#define M_TEST_Command_or_Response  28   // B'11100'   TEST Command or Response

#define LSAP_Null                 0x00   //
#define LSAP_SNA_Path_Control     0x04   //
#define LSAP_SNAP                 0xAA   // Sub-Network Access Protocol

#define Type_Information_Frame      1    //
#define Type_Supervisory_Frame      2    //
#define Type_Unnumbered_Frame       3    //



/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **                    SNA structures                                **
 **                                                                  **
 **********************************************************************
\**********************************************************************/

struct _XID3;
union  _CVhdr;
struct _NNcv;
struct _PSIDcv;
struct _TGDcv;
struct _CSVcv;
struct _HPRcv;

typedef struct _XID3   XID3,   *PXID3;
typedef union  _CVhdr  CVhdr,  *PCVhdr;
typedef struct _NNcv   NNcv,   *PNNcv;
typedef struct _PSIDcv PSIDcv, *PPSIDcv;
typedef struct _TGDcv  TGDcv,  *PTGDcv;
//  typedef struct _CSVcv  CSVcv,  *PCSVcv;
typedef struct _HPRcv  HPRcv,  *PHPRcv;

struct _XID3                       /* XID3                           */
{                                  /*                                */
                                   /* Note: The first 31-bytes       */
                                   /* of the XID3 are an XID3,       */
                                   /* defined in SNA Formats.        */
                                   /*                                */
/*000*/  BYTE   Format : 4;        /* Format of XID (4-bits),        */
         BYTE   Sending : 4;       /* Type XID-sending node (4-bits) */
/*001*/  BYTE   Length;            /* Length of the XID3             */
/*002*/  BYTE   NodeID[4];         /* Node identification:           */
                                   /* Block number (12 bits),        */
                                   /* ID number (20-bits)            */
/*006*/  BYTE   Reserved1[2];      /* Reserved                       */
/*008*/  BYTE   CharSend[2];       /* Characteristics of XID sender  */
/*00A*/  BYTE   Byte10;            /*                                */
/*00B*/  BYTE   Byte11;            /*                                */
/*00C*/  BYTE   Byte12;            /*                                */
/*00D*/  BYTE   Reserved2[2];      /* Reserved                       */
/*00F*/  BYTE   Byte15;            /*                                */
/*010*/  BYTE   TG;                /* Transmission Group Number      */
/*011*/  BYTE   DLC;               /* DLC type                       */
/*012*/  BYTE   DepSectionLength;  /* Dependant Section Length       */
                                   /* (including this length field)  */
/*013*/  BYTE   DepSection[FLEXIBLE_ARRAY];  /* Dependant Section    */
} ATTRIBUTE_PACKED;                /*                                */

/* Control Vector header                                             */
union _CVhdr                       /*                                */
{                                  /*                                */
    /* KL: The Key field precedes the Length field and the length    */
    /*     is the number of bytes, in binary, of the substructure's  */
    /*     Data field (e.g., Vector Data field). The Length field    */
    /*     value does not include the length of the substructure     */
    /*     Vector Header field (consisting of the Length and Key     */
    /*     fields).                                                  */
    struct {                       /*                                */
    /*000*/  BYTE   Key;           /* Vector key                     */
    /*001*/  BYTE   Length;        /* Vector length                  */
    } KL;                          /*                                */
    /* LT: The Length field precedes the Key field (also called the  */
    /*     "type" field hence "LT") and the length is the number of  */
    /*     bytes, in binary, of the substructure including both the  */
    /*     Vector Header field (consisting of the Length and Key     */
    /*     fields) and the Data field.                               */
    struct {                       /*                                */
    /*000*/  BYTE   Length;        /* Vector length                  */
    /*001*/  BYTE   Key;           /* Vector key                     */
    } LT;                          /*                                */
} ATTRIBUTE_PACKED;                /*                                */

/* Network Name Control Vector                                       */
struct _NNcv                       /* NNcv                           */
{                                  /*                                */
/*000*/  CVhdr  Header;            /* Vector header                  */
/*002*/  BYTE   Type;              /* Name type                      */
/*003*/  BYTE   Value[FLEXIBLE_ARRAY];  /*                           */
} ATTRIBUTE_PACKED;                /*                                */
#define KEY_NNcv 0x0E              /* NNcv key                       */
#define TYPE_PU_NAME 0xF1          /* PU Name                        */
#define TYPE_CP_NAME 0xF4          /* CP Name                        */
#define TYPE_LS_NAME 0xF7          /* Link Station Name              */

/* Product Set ID Control Vector                                     */
struct _PSIDcv                     /* PSIDcv                         */
{                                  /*                                */
/*000*/  CVhdr  Header;            /* Vector header                  */
/*002*/  BYTE   Data[FLEXIBLE_ARRAY];  /*                            */
} ATTRIBUTE_PACKED;                /*                                */
#define KEY_PSIDcv 0x10            /* PSIDcv key                     */

/* TG Description Control Vector                                     */
struct _TGDcv                      /* TGDcv                          */
{                                  /*                                */
/*000*/  CVhdr  Header;            /* Vector header                  */
/*002*/  BYTE   Data[FLEXIBLE_ARRAY];  /*                            */
} ATTRIBUTE_PACKED;                /*                                */
#define KEY_TGDcv 0x46             /* TGDcv key                      */

//  /* Call Security Verification Control Vector                         */
//  struct _CSVcv                      /* CSVcv                          */
//  {                                  /*                                */
//  /*000*/  CVhdr  Header;            /* Vector header                  */
//  /*002*/  BYTE   Reserved;          /* Reserved                       */
//  /*003*/  BYTE   LenSIDs;           /* Length of Security IDs         */
//                                     /* (including this length field)  */
//  /*004*/  BYTE   SID1[8];           /* First 8-byte Security ID       */
//                                     /* (random data or                */
//                                     /* enciphered random data)        */
//  /*00C*/  BYTE   SID2[8];           /* Second 8-byte Security ID      */
//                                     /* (random data or                */
//                                     /* enciphered random data or      */
//                                     /* space characters)              */
//  } ATTRIBUTE_PACKED;                /*                                */
//  #define KEY_CSVcv 0x56             /* CSVcv key                      */

/* HPR Capabilities Control Vector                                   */
struct _HPRcv                      /* HPRcv                          */
{                                  /*                                */
/*000*/  CVhdr  Header;            /* Vector header                  */
/*002*/  BYTE   Data[FLEXIBLE_ARRAY];  /*                            */
} ATTRIBUTE_PACKED;                /*                                */
#define KEY_HPRcv 0x61             /* HPRcv key                      */




/**********************************************************************\
 **********************************************************************
 **                                                                  **
 **                   INLINE FUNCTIONS                               **
 **                                                                  **
 **********************************************************************
\**********************************************************************/


// --------------------------------------------------------------------
// Set SenseID Information
// --------------------------------------------------------------------

static inline void SetSIDInfo( DEVBLK* pDEVBLK,
                               U16     wCUType,
                               BYTE    bCUMod,
                               U16     wDevType,
                               BYTE    bDevMod )
{
    BYTE* pSIDInfo = pDEVBLK->devid;

    memset( pSIDInfo, 0, sizeof(pDEVBLK->devid) );

    *pSIDInfo++ = 0x0FF;
    *pSIDInfo++ = (BYTE)(( wCUType >> 8 ) & 0x00FF );
    *pSIDInfo++ = (BYTE)( wCUType & 0x00FF );
    *pSIDInfo++ = bCUMod;
    *pSIDInfo++ = (BYTE)(( wDevType >> 8 ) & 0x00FF );
    *pSIDInfo++ = (BYTE)( wDevType & 0x00FF );
    *pSIDInfo++ = bDevMod;
    *pSIDInfo++ = 0x00;

    pDEVBLK->numdevid = 7;
}


// --------------------------------------------------------------------
// Set SenseID CIW Information
// --------------------------------------------------------------------

static inline void SetCIWInfo( DEVBLK* pDEVBLK,
                               U16     bOffset,
                               BYTE    bCIWType,
                               BYTE    bCIWOp,
                               U16     wCIWCount )
{
    BYTE* pSIDInfo = pDEVBLK->devid;

    pSIDInfo += 8;
    pSIDInfo += ( bOffset * 4 );

    *pSIDInfo++ = bCIWType | 0x40;
    *pSIDInfo++ = bCIWOp;
    *pSIDInfo++ = (BYTE)(( wCIWCount >> 8 ) & 0x00FF );
    *pSIDInfo++ = (BYTE)( wCIWCount & 0x00FF );

    pDEVBLK->numdevid += pDEVBLK->numdevid == 7 ? 5 : 4;
}


// --------------------------------------------------------------------

#if defined(_MSVC_)
 #pragma pack(pop)
#endif

#endif // __CTCADPT_H_
