/* CTC_PTP.H    (C) Copyright Ian Shorter, 2011-2012                 */
/*              MPC Point-To-Point Support (PTP)                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __CTC_PTP_H_
#define __CTC_PTP_H_

/* ----------------------------------------------------------------- */
/* Pack all structures to byte boundary...                           */
/* ----------------------------------------------------------------- */

#undef ATTRIBUTE_PACKED
#if defined(_MSVC_)
 #pragma pack(push)
 #pragma pack(1)
 #define ATTRIBUTE_PACKED
#else
 #define ATTRIBUTE_PACKED __attribute__((packed))
#endif


/* ----------------------------------------------------------------- */
/* PTP group size and relative device numbers                        */
/* ----------------------------------------------------------------- */
#define PTP_GROUP_SIZE         2

// The following defines aren't used, yet.

#define PTP_READ_DEVICE        0
#define PTP_WRITE_DEVICE       1

#define _IS_PTP_TYPE_DEVICE(_dev, _type) \
       ((_dev)->member == (_type))

#define IS_PTP_READ_DEVICE(_dev) \
       _IS_PTP_TYPE_DEVICE((_dev),PTP_READ_DEVICE)

#define IS_PTP_WRITE_DEVICE(_dev) \
        _IS_PTP_TYPE_DEVICE((_dev),PTP_WRITE_DEVICE)


/*-------------------------------------------------------------------*/
/* PTP timer values                                                  */
/*-------------------------------------------------------------------*/
#define PTP_READ_TIMEOUT_SECS  (5)      // five seconds

#define PTP_DELAY_USECS        (100)    // 100 microseconds delay; used
                                        // mostly by enqueue frame buffer
                                        // full delay loop...


/* ***************************************************************** */
/*                                                                   */
/* PTP Device Control Blocks                                         */
/*                                                                   */
/* ***************************************************************** */

struct _PTPBLK;                     // PTPBLK
struct _PTPATH;                     // PTPATH
struct _PTPINT;                     // PTPINT
struct _PTPHDR;                     // PTPHDR

typedef struct _PTPBLK PTPBLK, *PPTPBLK;
typedef struct _PTPATH PTPATH, *PPTPATH;
typedef struct _PTPINT PTPINT, *PPTPINT;
typedef struct _PTPHDR PTPHDR, *PPTPHDR;


/* ----------------------------------------------------------------- */
/* PTPBLK: There is one PTPBLK block for each PTP device pair. The   */
/* block contains information common to the PTP device pair.         */
/* ----------------------------------------------------------------- */

struct  _PTPBLK
{
    PPTPATH     pPTPATHRead;               // PTPATH Read path
    PPTPATH     pPTPATHWrite;              // PTPATH Write path

    DEVBLK*     pDEVBLKRead;               // DEVBLK Read path
    DEVBLK*     pDEVBLKWrite;              // DEVBLK Write path

    int         fd;                        // TUN/TAP fd
    TID         tid;                       // Read Thread ID
    pid_t       pid;                       // Read Thread pid

    LOCK        ReadBufferLock;            // Read buffer LOCK
    PPTPHDR     pReadBuffer;               // Read buffer
    int         iReadBufferGen;            // Read buffer generation

    LOCK        ReadEventLock;             // Condition LOCK
    COND        ReadEvent;                 // Condition signal

    LOCK        UnsolListLock;             // Unsolicited interrupt list LOCK
    PPTPINT     pFirstPTPINT;              // First PTPINT in list

    LOCK        UpdateLock;                // Lock

    u_int       uDebugMask;                // Debug mask
    u_int       fIPv4Spec:1;               // IPv4 specified
    u_int       fIPv6Spec:1;               // IPv6 specified
    u_int       fCloseInProgress:1;        // Close in progress
    u_int       fActive4:1;                // IPv4 connection active
    u_int       fActive6:1;                // IPv6 connection active
    u_int       fActiveLL6:1;              // IPv6 link local connection active
    u_int       fPreconfigured:1;          // TUN interface pre-configured
    u_int       fPreGuestIPAddr4:1;        // TUN interface pre-configured with guest IPv4 address
    u_int       fReadWaiting:1;            // ptp_read waiting
    u_int       fHaltOrClear:1;            // HSCH or CSCH issued

    int         iKernBuff;                 // Kernel buffer in K bytes.
    int         iIOBuff;                   // I/O buffer in K bytes.
    int         iAFamily;                  // Address family
    char        szTUNCharDevName[256];     // TUN/TAP char device filename (e.g. /dev/net/tun)
    char        szTUNIfName[IFNAMSIZ];     // TUN interface name (e.g. tun0)
    char        szMaxBfru[8];              // Maximum buffers to use
    char        szMTU[8];                  // MTU size
    char        szMACAddress[32];          // MAC Address
    char        szDriveIPAddr4[20];        // IPv4 Address (Driver)
    char        szDrivePfxSiz4[8];         // IPv4 Prefix Size (Driver)
    char        szNetMask[20];             // IPv4 Netmask
    char        szGuestIPAddr4[20];        // IPv4 Address (Guest)
#if defined(ENABLE_IPV6)
    char        szDriveIPAddr6[48];        // IPv6 Address (Driver)
    char        szDrivePfxSiz6[8];         // IPv6 Prefix Size (Driver)
    char        szGuestIPAddr6[48];        // IPv6 Address (Guest)
    char        szDriveLLAddr6[48];        // IPv6 Link Local Address (Driver)
    char        szDriveLLxSiz6[8];         // IPv6 Link Local Prefix Size (Driver)
    char        szGuestLLAddr6[48];        // IPv6 Link Local Address (Guest)
#endif /* defined(ENABLE_IPV6) */

    int         iMaxBfru;                  // Maximum buffers to use
    int         iMTU;                      // MTU size
    struct in_addr  iaDriveIPAddr4;        // IPv4 Address (Driver)
    struct in_addr  iaGuestIPAddr4;        // IPv4 Address (Guest)
#if defined(ENABLE_IPV6)
    struct in6_addr iaDriveIPAddr6;        // IPv6 Address (Driver)
    struct in6_addr iaGuestIPAddr6;        // IPv6 Address (Guest)
    struct in6_addr iaDriveLLAddr6;        // IPv6 Link Local Address (Driver)
    struct in6_addr iaGuestLLAddr6;        // IPv6 Link Local Address (Guest)
#endif /* defined(ENABLE_IPV6) */
    BYTE        xSAaddress[4];             // x-sides Subarea address
    BYTE        xStartTime[8];             // x-sides start time (tod)
    BYTE        xFirstCsvSID2[8];          // x-sides first CSVcv SID2
    BYTE        xSecondCsvSID2[8];         // x-sides second CSVcv SID2
    BYTE        xTokensUpdated;            // x-sides Tokens updated
    BYTE        xTokenIssuerRm[4];         // x-sides Issuer Token
    BYTE        xTokenCmFilter[4];         // x-sides Cm Filter Token
    BYTE        xTokenCmConnection[4];     // x-sides Cm Communication Token
    BYTE        xTokenUlpFilter[4];        // x-sides ULP Filter Token
    BYTE        xTokenUlpConnection[4];    // x-sides ULP Communication Token

    U16         xDataLen1;                 // x-sides Data length one
    U16         xMaxReadLen;               // x-sides Maximum read length
    U16         xActMTU;                   // x-sides Actual MTU

    BYTE        yStartTime[8];             // y-sides start time (tod) from XID2 exchange
    BYTE        yTokensCopied;             // y-sides Tokens copied
    BYTE        yTokenIssuerRm[4];         // y-sides Issuer Token
    BYTE        yTokenCmFilter[4];         // y-sides Cm Filter Token
    BYTE        yTokenCmConnection[4];     // y-sides Cm Communication Token
    BYTE        yTokenUlpFilter[4];        // y-sides ULP Filter Token
    BYTE        yTokenUlpConnection[4];    // y-sides ULP Communication Token

    U16         yDataLen1;                 // y-sides Data length one from XID2 exchange
    U16         yMaxReadLen;               // y-sides Maximum read length from XID2 exchange
    U16         yActMTU;                   // y-sides Actual MTU

    U32         uSeqNumIssuer;             // Issuer Sequence number
    U32         uSeqNumCm;                 // Cm Sequence number
    U16         uIdNum;                    // ???

    BYTE        bActivate4;                // IPv4 activation progress
    BYTE        bActivate6;                // IPv6 activation progress
    BYTE        bActivateLL6;              // IPv6 link local activation progress
    BYTE        bTerminate4;               // IPv4 termination progress
    BYTE        bTerminate6;               // IPv6 termination progress
    BYTE        bTerminateLL6;             // IPv6 link local termination progress
};

#define HEASKEDME_START       0x80         // He asked me Start
#define IANSWEREDHIM_START    0x40         // I answered him Start
#define IASKEDHIM_START       0x20         // I asked him Start
#define HEANSWEREDME_START    0x10         // He answered me Start
#define HETOLDMEHIS_ADDRESS   0x08         // He told me his Address
#define IANSWEREDHIS_ADDRESS  0x04         // I answered his Address
#define ITOLDHIMMY_ADDRESS    0x02         // I told him my Address
#define HEANSWEREDMY_ADDRESS  0x01         // He answered my Address
#define WEAREACTIVE           0xFF         // Are we active?
#define HEASKEDME_STOP        0x80         // He asked me Stop
#define IANSWEREDHIM_STOP     0x40         // I answered him Stop
#define IASKEDHIM_STOP        0x20         // I asked him Stop
#define HEANSWEREDME_STOP     0x10         // He answered me Stop
#define WEARETERMINATED       0xF0         // Are we terminated?

#define DBGPTPPACKET          0x00000001   // Packet
                                           // (i.e. the IP packets sent to
                                           // or received from the TUN device
                                           // in network byte order)
#define DBGPTPDATA            0x00000002   // Data
                                           // (i.e. the messages presented
                                           // to or accepted from the CTC
                                           // devices in network byte order
                                           // Note: a maximun of 256 bytes
                                           // is displayed)
#define DBGPTPEXPAND          0x00000004   // Data expanded
                                           // (i.e. the messages presented
                                           // to or accepted from the CTC
                                           // devices in network byte order
                                           // showing the MPC_TH etc.
                                           // Note: a maximun of 64 bytes
                                           // of data is displayed)
#define DBGPTPUPDOWN          0x00000010   // Connection up and down
#define DBGPTPCCW             0x00000020   // CCWs executed
#define DBGPTPCONFVALUE       0x00000080   // Configuration value
                                           // (i.e. values specified on, or
                                           // derived from, the config stmt.
                                           // Note: only available from
                                           // config stmt -d parameter)

/* ----------------------------------------------------------------- */
/* PTPATH: There are two PTPATH blocks for each PTP device pair,     */
/* one for the read device and one for the write device. The blocks  */
/* contain information specific to the read or write device.         */
/* ----------------------------------------------------------------- */

struct  _PTPATH
{
    PPTPBLK     pPTPBLK;                   // PTPBLK

    DEVBLK*     pDEVBLK;                   // DEVBLK

    LOCK        ChainLock;                 // Chain LOCK
    PPTPHDR     pFirstPTPHDR;              // First PTPHDR in chain
    PPTPHDR     pLastPTPHDR;               // Last PTPHDR in chain
    int         iNumPTPHDR;                // Number of PTPHDRs on chain

    LOCK        UnsolEventLock;            // Condition LOCK
    COND        UnsolEvent;                // Condition signal

    BYTE        fHandshaking:1;            // Handshaking in progress
    BYTE        fHandshakeCur;             // Handshake currently in progress
    BYTE        fHandshakeSta;             // Handshakes started
    BYTE        fHandshakeFin;             // Handshakes finished

    BYTE        bAttnCode;                 // The CCW opcode for which an
                                           // Attention interrupt was raised.

    BYTE        bDLCtype;                  // DLC type
    U32         uSeqNum;                   // Sequence number
};

#define HANDSHAKE_ONE    0x01              // Handshake one
#define HANDSHAKE_TWO    0x02              // Handshake two
#define HANDSHAKE_THREE  0x04              // Handshake three
#define HANDSHAKE_ALL    0x07              // Handshake all


/* ----------------------------------------------------------------- */
/* PTPINT: There is a PTPINT block for each unsolicited interrupt    */
/* that needs to be raised.                                          */
/* ----------------------------------------------------------------- */

struct _PTPINT                             // PTP Unsolicited Interrupt
{
    PPTPINT   pNextPTPINT;                 // Pointer to next PTPINT
    DEVBLK*   pDEVBLK;                     // DEVBLK
    BYTE      bStatus;                     // Interrupt device status
    int       iDelay;                      // Delay before presenting (millisec)
};


/* ----------------------------------------------------------------- */
/* PTP Message Buffer Header                                         */
/* ----------------------------------------------------------------- */

struct _PTPHDR                             // PTP Message Buffer Header
{
    PPTPHDR   pNextPTPHDR;                 // Pointer to next PTPHDR
    int       iAreaLen;                    // Data area length
    int       iDataLen;                    // Data length
                                           // End of the PTPHDR (or, if you
                                           // prefer, the start of the data).
} ATTRIBUTE_PACKED;
#define SIZE_HDR  sizeof(PTPHDR)           // Size of PTPHDR


#if defined(_MSVC_)
 #pragma pack(pop)
#endif

#endif // __CTC_PTP_H_
