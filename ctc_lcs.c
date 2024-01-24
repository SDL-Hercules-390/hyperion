/* CTC_LCS.C    (C) Copyright James A. Pierson, 2002-2012            */
/*              (C) Copyright "Fish" (David B. Trout), 2002-2011     */
/*              (C) and others 2013-2022                             */
/*              Hercules LAN Channel Station Support                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

/* jbs 10/27/2007 added _SOLARIS_ */
#if !defined(__SOLARIS__)

#include "hercules.h"
#include "ctcadpt.h"
#include "tuntap.h"
#include "opcode.h"
#include "herc_getopt.h"

#define MAX_TRACE_LEN 128

#define FROM_GUEST         '<'
#define TO_GUEST           '>'
#define NO_DIRECTION       ' '

//-----------------------------------------------------------------------------
//  DEBUGGING: use 'ENABLE_TRACING_STMTS' to activate the compile-time
//  "TRACE" and "VERIFY" debug macros. Use 'NO_LCS_OPTIMIZE' to disable
//  compiler optimization for this module (to make setting breakpoints
//  and stepping through code more reliable).
//
//  Use the "-d" device statement option (or the "ctc debug on" panel
//  command) to activate logmsg debugging (verbose debug log messages).
//
//  Use the 'ptt {lcs1|lcs2}' panel command to activate debug tracing
//  via the PTT facility.
//-----------------------------------------------------------------------------

//#define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
//#include "dbgtrace.h"                   // (Fish: DEBUGGING)
//#define  NO_LCS_OPTIMIZE                // (Fish: DEBUGGING) (MSVC only)
#define  LCS_NO_950_952                 // (Fish: DEBUGGING: HHC00950 and HHC00952 are rarely interesting)

#if defined( _MSVC_ ) && defined( NO_LCS_OPTIMIZE )
  #pragma optimize( "", off )           // disable optimizations for reliable breakpoints
#endif

// PROGRAMMING NOTE: in addition to normal logmsg debugging (i.e. the "-d"
// device statement option which activates the displaying of log messages)
// we also support debugging via the PTT facility to help with debugging
// otherwise hard to find race conditions. PTT tracing is always enabled,
// but is never activated except by demand via the 'ptt' panel command.

#undef  PTT_TIMING
#define PTT_TIMING      PTT_LCS1        // LCS Timing Debug
#undef  PTT_DEBUG
#define PTT_DEBUG       PTT_LCS2        // LCS General Debugging (including
                                        // LCS device lock and event tracing)

//-----------------------------------------------------------------------------
/* The following CCW codes are immediate commands. */
/*   0x03 - No-Operation                           */
/*   0x17 - Control                                */
/*   0x43 - Set Basic Mode                         */
/*   0xC3 - Set Extended Mode                      */

static BYTE  CTC_Immed_Commands [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* 00 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0, /* 10 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 20 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 30 */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* 40 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 80 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 90 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* A0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* B0 */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* C0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* D0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* E0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* F0 */
};

// First three octets of Multicast MAC address
static const BYTE mcast3[ 3 ] = { 0x01, 0x00, 0x5e };
static const MAC  zeromac     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* ------------------------------------------------------------------ */
/* SNA token                                                          */
/* ------------------------------------------------------------------ */

static  LOCK  TokenLock;
static  int   TokenLockInitialized = FALSE;
static  U32   uToken               = 0x40000240;

#define       INCREMENT_TOKEN        0x00000100

// ====================================================================
//                       Declarations
// ====================================================================

static void     LCS_Read( DEVBLK* pDEVBLK,   U32   sCount,
                          BYTE*   pIOBuf,    BYTE* UnitStat,
                          U32*    pResidual, BYTE* pMore );
static void     LCS_Write( DEVBLK* pDEVBLK,   U32   sCount,
                           BYTE*   pIOBuf,    BYTE* UnitStat,
                           U32*    pResidual );

static void     LCS_Assist( PLCSPORT pLCSPORT );

static void     LCS_Startup       ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_Shutdown      ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_StartLan      ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_StopLan       ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_QueryIPAssists( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_LanStats      ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_AddMulticast  ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_DelMulticast  ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_DefaultCmdProc( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );

static void*    LCS_PortThread( void* arg /* PLCSPORT pLCSPORT */ );
static void*    LCS_AttnThread( void* arg /* PLCSBLK pLCSBLK */ );

static void     LCS_EnqueueEthFrame     ( PLCSPORT pLCSPORT, PLCSDEV pLCSDEV, BYTE* pData, size_t iSize );
static int      LCS_DoEnqueueEthFrame   ( PLCSPORT pLCSPORT, PLCSDEV pLCSDEV, BYTE* pData, size_t iSize );

static void     LCS_EnqueueReplyFrame   ( PLCSDEV pLCSDEV, PLCSCMDHDR pReply, size_t iSize );
static int      LCS_DoEnqueueReplyFrame ( PLCSDEV pLCSDEV, PLCSCMDHDR pReply, size_t iSize );

static void     GetFrameInfo( PETHFRM pEthFrame, char* pPktType, U16* pEthType, BYTE* pHas8022, BYTE* pHas8022Snap );
static void     GetIfMACAddress( PLCSPORT pLCSPORT );
static int      BuildOAT( char* pszOATName, PLCSBLK pLCSBLK );
static char*    ReadOAT( char* pszOATName, FILE* fp, char* pszBuff );
static int      ParseArgs( DEVBLK* pDEVBLK, PLCSBLK pLCSBLK, int argc, char** argv );

static void     LCS_Read_SNA( DEVBLK* pDEVBLK,   U32   sCount,
                              BYTE*   pIOBuf,    BYTE* UnitStat,
                              U32*    pResidual, BYTE* pMore );
static void     LCS_Write_SNA( DEVBLK* pDEVBLK,   U32   sCount,
                               BYTE*   pIOBuf,    BYTE* UnitStat,
                               U32*    pResidual );

static void     LCS_StartLan_SNA  ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_StopLan_SNA   ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_LanStats_SNA  ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );
static void     LCS_UnsuppCmd_SNA ( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen );

static void     LCS_ProcessAccepted_SNA ( PLCSPORT pLCSPORT, PLCSDEV pLCSDEV, BYTE* pData, size_t iSize );

static int      ExtractLLC( PLLC pLLC, BYTE* pStart, int iSize );
static int      BuildLLC( PLLC pLLC, BYTE* pStart );

static void     Process_0D10 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0D00 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_8C0B (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C0A (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C25 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C22 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_8D00 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C0B (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C99 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C0D (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C0E (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);
static void     Process_0C98 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2);

static PLCSIBH  alloc_lcs_buffer( PLCSDEV pLCSDEV, int iSize );
static void     add_lcs_buffer_to_chain( PLCSDEV pLCSDEV, PLCSIBH pLCSIBH );
static PLCSIBH  remove_lcs_buffer_from_chain( PLCSDEV pLCSDEV );
static void     remove_and_free_any_lcs_buffers_on_chain( PLCSDEV pLCSDEV );
static void     free_lcs_buffer( PLCSDEV pLCSDEV, PLCSIBH pLCSIBH );

static PLCSCONN alloc_connection( PLCSDEV pLCSDEV );
static void     add_connection_to_chain( PLCSDEV pLCSDEV, PLCSCONN pLCSCONN );
static PLCSCONN find_connection_by_remote_mac( PLCSDEV pLCSDEV, MAC* pMAC );
static PLCSCONN find_connection_by_outbound_token( PLCSDEV pLCSDEV, BYTE* pToken );
static PLCSCONN find_connection_by_inbound_token( PLCSDEV pLCSDEV, BYTE* pToken );
//??  static PLCSCONN find_connection_by_token_and_mac( PLCSDEV pLCSDEV, BYTE* pToken, int iWhichToken, MAC* pMAC, int iWhichMAC );
static int      remove_connection_from_chain( PLCSDEV pLCSDEV, PLCSCONN pLCSCONN );
static void     remove_and_free_any_connections_on_chain( PLCSDEV pLCSDEV );
static void     free_connection( PLCSDEV pLCSDEV, PLCSCONN pLCSCONN );

// ====================================================================
//                       Helper macros
// ====================================================================

#define INIT_REPLY_FRAME( pReply, iReplyLen, pCmdFrame, iCmdLen )    \
    do                                                               \
    {                                                                \
        if ( (iCmdLen) >= (iReplyLen) )                              \
        {                                                            \
            memcpy( (pReply), (pCmdFrame), (iReplyLen) );            \
        }                                                            \
        else                                                         \
        {                                                            \
            memset( (pReply), 0, (iReplyLen) );                      \
            memcpy( (pReply), (pCmdFrame), (iCmdLen) );              \
            (iReplyLen) = (iCmdLen);                                 \
        }                                                            \
        STORE_HW( (pReply)->bLCSCmdHdr.bLCSHdr.hwOffset, 0x0000 );   \
        STORE_HW( (pReply)->bLCSCmdHdr.hwReturnCode, 0x0000 );       \
    }                                                                \
    while (0)

// ====================================================================
//                    find_group_device
// ====================================================================

static DEVBLK * find_group_device(DEVGRP *group, U16 devnum)
{
    int i;

    for (i = 0; i < group->acount; i++)
        if (group->memdev[i]->devnum == devnum)
            return group->memdev[i];

    return NULL;
}

// ====================================================================
//                          LCS_Init
// ====================================================================

int  LCS_Init( DEVBLK* pDEVBLK, int argc, char *argv[] )
{
    PLCSBLK     pLCSBLK;
    PLCSDEV     pLCSDev;
    PLCSPORT    pLCSPORT;
    int         i;
    int         rc;
    BYTE        bMode;   /* Either LCSDEV_MODE_IP or LCSDEV_MODE_SNA */
    struct in_addr  addr;               // Work area for addresses
    char        thread_name[32];        // Thread name


    pDEVBLK->devtype = 0x3088;

    pDEVBLK->excps   = 0;

    // Return when an existing group has been joined but is still incomplete
    if (!group_device( pDEVBLK, 0 ) && pDEVBLK->group)
        return 0;

    // We need to create a group, and as such determine the number of devices
    if (!pDEVBLK->group)
    {

        // Housekeeping
        pLCSBLK = malloc( sizeof( LCSBLK ));
        if (!pLCSBLK)
        {
            char buf[40];
            MSGBUF(buf, "malloc(%d)", (int) sizeof( LCSBLK ));
            // "%1d:%04X %s: error in function %s: %s"
            WRMSG( HHC00900, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                pDEVBLK->devnum, pDEVBLK->typname, buf, strerror(errno) );
            return -1;
        }
        memset( pLCSBLK, 0, sizeof( LCSBLK ));
        pLCSBLK->iTraceLen = LCS_TRACE_LEN_DEFAULT;

        // Initialize locking and event mechanisms
        initialize_lock( &pLCSBLK->AttnLock );
        initialize_lock( &pLCSBLK->AttnEventLock );
        initialize_condition( &pLCSBLK->AttnEvent );

        for (i=0; i < LCS_MAX_PORTS; i++)
        {
            pLCSPORT = &pLCSBLK->Port[i];
            memset( pLCSPORT, 0, sizeof ( LCSPORT ));

            pLCSPORT->bPort   = i;
            pLCSPORT->pLCSBLK = pLCSBLK;

            // Initialize locking and event mechanisms
            initialize_lock( &pLCSPORT->PortDataLock );
            initialize_lock( &pLCSPORT->PortEventLock );
            initialize_condition( &pLCSPORT->PortEvent );
        }

        // Parse configuration file statement
        rc = ParseArgs( pDEVBLK, pLCSBLK, argc, (char**) argv );
        if (rc < 0)
        {
            free( pLCSBLK );
            pLCSBLK = NULL;
            return -1;
        }
        bMode = rc;

        if (pLCSBLK->pszOATFilename)
        {
            // If an OAT file was specified, Parse it and build the
            // OAT table.
            if (BuildOAT( pLCSBLK->pszOATFilename, pLCSBLK ) != 0)
            {
                free( pLCSBLK );
                pLCSBLK = NULL;
                return -1;
            }
        }
        else
        {
            // Otherwise, build an OAT based on the address specified
            // in the config file with an assumption of IP mode.
            pLCSBLK->pDevices = malloc( sizeof( LCSDEV ));

            memset( pLCSBLK->pDevices, 0, sizeof( LCSDEV ));
            pLCSBLK->pDevices->sAddr        = pDEVBLK->devnum;
            pLCSBLK->pDevices->bPort        = 0;
            pLCSBLK->pDevices->pNext        = NULL;

            if (bMode == LCSDEV_MODE_IP)
            {
                if (pLCSBLK->pszIPAddress)
                {
                    pLCSBLK->pDevices->pszIPAddress = strdup( pLCSBLK->pszIPAddress );
                    inet_aton( pLCSBLK->pDevices->pszIPAddress, &addr );
                    pLCSBLK->pDevices->lIPAddress = addr.s_addr; // (network byte order)
                    pLCSBLK->pDevices->bType    = LCSDEV_TYPE_NONE;
                }
                else
                    pLCSBLK->pDevices->bType    = LCSDEV_TYPE_PRIMARY;

                pLCSBLK->pDevices->bMode        = LCSDEV_MODE_IP;

                pLCSBLK->icDevices = 2;
            }
            else
            {
                pLCSBLK->pDevices->bMode        = LCSDEV_MODE_SNA;

                pLCSBLK->icDevices = 1;
            }
        }

        // Now we must create the group
        if (!group_device( pDEVBLK, pLCSBLK->icDevices ))
        {
            pDEVBLK->group->grp_data = pLCSBLK;
            return 0;
        }
        else
            pDEVBLK->group->grp_data = pLCSBLK;

    }
    else
        pLCSBLK = pDEVBLK->group->grp_data;

    // When this code is reached the last devblk has been allocated.

    // Now build the LCSDEV's...

    // If an OAT is specified, the addresses that were specified in the
    // hercules.cnf file must match those that are specified in the OAT.

    for (pLCSDev = pLCSBLK->pDevices; pLCSDev; pLCSDev = pLCSDev->pNext)
    {
        pLCSDev->pDEVBLK[LCS_READ_SUBCHANN] = find_group_device( pDEVBLK->group, pLCSDev->sAddr );

        if (!pLCSDev->pDEVBLK[LCS_READ_SUBCHANN])
        {
            // "%1d:%04X CTC: lcs device %04X not in configuration"
            WRMSG( HHC00920, "E", SSID_TO_LCSS( pDEVBLK->group->memdev[0]->ssid ) ,
                  pDEVBLK->group->memdev[0]->devnum, pLCSDev->sAddr );
            return -1;
        }

        // Establish SENSE ID and Command Information Word data.
        // (In SNA mode VTAM checks the first four bytes of the
        // Sense ID for 0xFF308801, 0xFF308860 and 0xFF30881F.)
        SetSIDInfo( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN], 0x3088, 0x60, 0x3088, 0x01 );
//      SetCIWInfo( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN], 0, 0, 0x72, 0x0080 );
//      SetCIWInfo( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN], 1, 1, 0x83, 0x0004 );
//      SetCIWInfo( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN], 2, 2, 0x82, 0x0040 );

        pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->ctctype  = CTC_LCS;
        pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->ctcxmode = 1;
        pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->dev_data = pLCSDev;
        pLCSDev->pLCSBLK              = pLCSBLK;
        STRLCPY( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->filename, pLCSBLK->pszTUNDevice );

        // If this is an IP Passthru address, we need a write address
        if (pLCSDev->bMode == LCSDEV_MODE_IP)
        {
            // (the write device is the inverse of the read device)
            pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN] = find_group_device( pDEVBLK->group, pLCSDev->sAddr ^ 1 );

            if (!pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN])
            {
                // "%1d:%04X CTC: lcs device %04X not in configuration"
                WRMSG( HHC00920, "E", SSID_TO_LCSS( pDEVBLK->group->memdev[0]->ssid ),
                      pDEVBLK->group->memdev[0]->devnum, pLCSDev->sAddr ^ 1 );
                return -1;
            }

            // Establish SENSE ID and Command Information Word data.
            SetSIDInfo( pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN], 0x3088, 0x60, 0x3088, 0x01 );
//          SetCIWInfo( pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN], 0, 0, 0x72, 0x0080 );
//          SetCIWInfo( pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN], 1, 1, 0x83, 0x0004 );
//          SetCIWInfo( pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN], 2, 2, 0x82, 0x0040 );

            pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN]->ctctype  = CTC_LCS;
            pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN]->ctcxmode = 1;
            pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN]->dev_data = pLCSDev;

            STRLCPY( pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN]->filename, pLCSBLK->pszTUNDevice );
        }

        // Initialize the buffer size. See the programming note re
        // frame buffer size in ctcadpt.h, and note that the LCS_Startup
        // command might reduce the value in pLCSDEV->iMaxFrameBufferSize.
        // For SNA, the LCS_Startup command seems to be not used.
        pLCSDev->iMaxFrameBufferSize = sizeof(pLCSDev->bFrameBuffer);

        // Indicate that the DEVBLK(s) have been create sucessfully
        pLCSDev->fDevCreated = 1;

        // Initialize locking and event mechanisms
        initialize_lock( &pLCSDev->DevDataLock );
        initialize_lock( &pLCSDev->DevEventLock );
        initialize_condition( &pLCSDev->DevEvent );
        initialize_lock( &pLCSDev->LCSIBHChainLock );
        initialize_lock( &pLCSDev->LCSCONNChainLock );
        initialize_lock( &pLCSDev->InOutLock );

        // Create the TAP interface (if not already created by a
        // previous pass. More than one interface can exist on a port.

        pLCSPORT = &pLCSBLK->Port[ pLCSDev->bPort ];

        if (!pLCSPORT->fPortCreated)
        {
            int  rc;

            rc = TUNTAP_CreateInterface( pLCSBLK->pszTUNDevice,
                                         IFF_TAP | IFF_NO_PI,
                                         &pLCSPORT->fd,
                                         pLCSPORT->szNetIfName );

            if (rc < 0)
            {
                // "%1d:%04X %s: error in function %s: %s"
                WRMSG( HHC00900, "E", SSID_TO_LCSS( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->ssid),
                    pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->devnum, pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->typname,
                    "TUNTAP_CreateInterface", strerror( rc ));
                return -1;
            }

            // "%1d:%04X %s: interface %s, type %s opened"
            WRMSG( HHC00901, "I", SSID_TO_LCSS( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->ssid ),
                                  pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->devnum,
                                  pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->typname,
                                  pLCSPORT->szNetIfName, "TAP");

            //
            if (!pLCSPORT->fPreconfigured)
            {
#ifdef OPTION_TUNTAP_SETMACADDR
                if (pLCSPORT->fLocalMAC)
                {
                    VERIFY( TUNTAP_SetMACAddr( pLCSPORT->szNetIfName,
                                               pLCSPORT->szMACAddress ) == 0 );
                }
#endif // OPTION_TUNTAP_SETMACADDR
                VERIFY( TUNTAP_SetMTU( pLCSPORT->szNetIfName, "1500" ) == 0 );
                VERIFY( TUNTAP_SetIPAddr( pLCSPORT->szNetIfName, "0.0.0.0" ) == 0 );
            }

#if defined(OPTION_W32_CTCI)

            // Set the specified driver/dll i/o buffer sizes..
            {
                struct tt32ctl tt32ctl;

                memset( &tt32ctl, 0, sizeof( tt32ctl ));
                STRLCPY( tt32ctl.tt32ctl_name, pLCSPORT->szNetIfName );

                tt32ctl.tt32ctl_devbuffsize = pLCSBLK->iKernBuff;

                if (TUNTAP_IOCtl( pLCSPORT->fd, TT32SDEVBUFF, (char*) &tt32ctl ) != 0)
                {
                    // "%1d:%04X %s: ioctl %s failed for device %s: %s"
                    WRMSG( HHC00902, "W", SSID_TO_LCSS( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->ssid ),
                          pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->devnum,
                          pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->typname,
                          "TT32SDEVBUFF", pLCSPORT->szNetIfName, strerror( errno ));
                }

                tt32ctl.tt32ctl_iobuffsize = pLCSBLK->iIOBuff;

                if (TUNTAP_IOCtl( pLCSPORT->fd, TT32SIOBUFF, (char*) &tt32ctl ) != 0)
                {
                    // "%1d:%04X %s: ioctl %s failed for device %s: %s"
                    WRMSG( HHC00902, "W", SSID_TO_LCSS( pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->ssid ),
                          pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->devnum,
                          pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->typname,
                          "TT32SIOBUFF", pLCSPORT->szNetIfName, strerror( errno ) );
                }
            }
#endif

            // Get and display the tap interface MAC address.
            GetIfMACAddress( pLCSPORT );

            // Indicate that the port is used.
            pLCSPORT->fUsed        = 1;
            pLCSPORT->fPortCreated = 1;

            // Set assist flags
            LCS_Assist( pLCSPORT );

            // Now create the port thread to read packets from tuntap
            // The thread name uses the read device address of the first device pair.
            MSGBUF( thread_name, "%s %4.4X Port %d Thread",
                                 pLCSBLK->pDevices->pDEVBLK[LCS_READ_SUBCHANN]->typname,
                                 pLCSBLK->pDevices->pDEVBLK[LCS_READ_SUBCHANN]->devnum,
                                 pLCSPORT->bPort);
            rc = create_thread( &pLCSPORT->tid, JOINABLE,
                                LCS_PortThread, pLCSPORT, thread_name );
            if (rc)
            {
                // "Error in function create_thread(): %s"
                WRMSG( HHC00102, "E", strerror( rc ));
            }

            // Identify thread ID with devices on which they're active
            pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->tid = pLCSPORT->tid;
            if (pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN])
                pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN]->tid = pLCSPORT->tid;
        }

        // Add these devices to the ports device list.
        pLCSPORT->icDevices++;
        pLCSDev->pDEVBLK[LCS_READ_SUBCHANN]->fd = pLCSPORT->fd;

        if (pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN])
            pLCSDev->pDEVBLK[LCS_WRITE_SUBCHANN]->fd = pLCSPORT->fd;

    }   // End of  for (pLCSDev = pLCSBLK->pDevices; pLCSDev; pLCSDev = pLCSDev->pNext)


    // If this LCS has one or more SNA devices we need an attention required thread to present Attention interrupts to the guest.
    for (pLCSDev = pLCSBLK->pDevices; pLCSDev; pLCSDev = pLCSDev->pNext)
    {
        if (pLCSDev->bMode == LCSDEV_MODE_SNA)
        {
            // Now create the attention required thread to present Attention interrupts to the guest(s).
            // The thread name uses the read device address of the first device pair.
            MSGBUF( thread_name, "%s %4.4X AttnThread",
                                 pLCSBLK->pDevices->pDEVBLK[LCS_READ_SUBCHANN]->typname,
                                 pLCSBLK->pDevices->pDEVBLK[LCS_READ_SUBCHANN]->devnum);
            rc = create_thread( &pLCSBLK->AttnTid, JOINABLE,
                                LCS_AttnThread, pLCSBLK, thread_name );
            if (rc)
            {
                // "Error in function create_thread(): %s"
                WRMSG( HHC00102, "E", strerror( rc ));
            }

            // Initialize locking for the SNA token, if necessary.
            if (!TokenLockInitialized)
            {
                TokenLockInitialized = TRUE;
                initialize_lock( &TokenLock );
            }

            break;
        }
    }   // End of  for (pLCSDev = pLCSBLK->pDevices; pLCSDev; pLCSDev = pLCSDev->pNext)

    return 0;
}

// ====================================================================
//                          LCS_Assist
// ====================================================================
// Determine which IP assists we will be supporting, which depends on
// which assists the tuntap device itself supports, as well as which
// ones we can directly support ourselves if tuntap can't support it.
// The most important assist is perhaps the Checksum Offloading assist
// since we (or tuntap) can calculate checksums much more efficiently
// than our guest can using emulated instructions.
// --------------------------------------------------------------------

void LCS_Assist( PLCSPORT pLCSPORT )
{
#if defined( SIOCGIFHWADDR )
    MAC    mac  = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x01 };
    ifreq  ifr  = {0};
#endif

    // We shall always support the following assists for the guest.

    pLCSPORT->sIPAssistsSupported |= LCS_MULTICAST_SUPPORT;
    pLCSPORT->sIPAssistsEnabled   |= LCS_MULTICAST_SUPPORT;

    pLCSPORT->sIPAssistsSupported |= LCS_INBOUND_CHECKSUM_SUPPORT;
    pLCSPORT->sIPAssistsEnabled   |= LCS_INBOUND_CHECKSUM_SUPPORT;

    pLCSPORT->sIPAssistsSupported |= LCS_OUTBOUND_CHECKSUM_SUPPORT;
    pLCSPORT->sIPAssistsEnabled   |= LCS_OUTBOUND_CHECKSUM_SUPPORT;

    // Check if tuntap can handle the multicast assist for us.

#if defined( SIOCGIFHWADDR )
    STRLCPY( ifr.ifr_name, pLCSPORT->szNetIfName );
    memcpy( ifr.ifr_hwaddr.sa_data, mac, sizeof( MAC ));

    if (TUNTAP_IOCtl( pLCSPORT->fd, SIOCADDMULTI, (char*) &ifr ) == 0)
    {
        TUNTAP_IOCtl( pLCSPORT->fd, SIOCDELMULTI, (char*) &ifr );
        pLCSPORT->fDoMCastAssist = 0;   // (tuntap does it for us)
    }
    else
#endif // !defined( SIOCGIFHWADDR )
        pLCSPORT->fDoMCastAssist = 1;   // (we must do it ourself)

    // "CTC: lcs device port %2.2X: %s Multicast assist enabled"
    WRMSG( HHC00921, "I", pLCSPORT->bPort,
        pLCSPORT->fDoMCastAssist ? "manual" : "tuntap" );

    // Check if tuntap can do outbound checksum offloading for us.

#if 0 /* Disable for now. TAP Checksum offload seems broken */
#if defined( TUNSETOFFLOAD ) && defined( TUN_F_CSUM )
    if (TUNTAP_IOCtl( pLCSPORT->fd, TUNSETOFFLOAD, (char*) TUN_F_CSUM ) == 0)
    {
        pLCSPORT->fDoCkSumOffload = 0;    // (tuntap does it for us)
    }
    else
#endif
#endif
        pLCSPORT->fDoCkSumOffload = 1;    // (we must do it ourself)

    // "CTC: lcs device port %2.2X: %s Checksum Offload enabled"
    WRMSG( HHC00935, "I", pLCSPORT->bPort,
        pLCSPORT->fDoCkSumOffload ? "manual" : "tuntap" );

    // Check if tuntap can also do segmentation offloading for us.

#if 0 /* Disable for now. TCP Segment Offload needs
         to be enabled by stack using LCS */
#if defined( TUNSETOFFLOAD ) && defined( TUN_F_TSO4 ) && defined( TUN_F_UFO )
    /* Only do offload if doing TAP checksum offload */
    if (!pLCSPORT->fDoCkSumOffload)
    {
        if (TUNTAP_IOCtl( pLCSPORT->fd, TUNSETOFFLOAD, (char*)(TUN_F_CSUM | TUN_F_TSO4 | TUN_F_UFO)) == 0)
        {
            VERIFY( TUNTAP_IOCtl( pLCSPORT->fd, TUNSETOFFLOAD, (char*) TUN_F_CSUM ) == 0);
            pLCSPORT->sIPAssistsSupported |= LCS_IP_FRAG_REASSEMBLY;
            /* Do Not ENABLE!! */
            // pLCSPORT->sIPAssistsEnabled   |= LCS_IP_FRAG_REASSEMBLY;
            // "CTC: lcs device port %2.2X: %s Large Send Offload supported"
            WRMSG( HHC00938, "I", pLCSPORT->bPort, "tuntap" );
        }
    }
#endif
#endif
}

// ====================================================================
//                        LCS_ExecuteCCW
// ====================================================================

void  LCS_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                      BYTE    bFlags,  BYTE  bChained,
                      U32     sCount,  BYTE  bPrevCode,
                      int     iCCWSeq, BYTE* pIOBuf,
                      BYTE*   pMore,   BYTE* pUnitStat,
                      U32*    pResidual )
{
    PLCSDEV         pLCSDEV;
    int             iNum;               // Number of bytes to move
    BYTE            bOpCode;            // CCW opcode with modifier
                                        //   bits masked off

    UNREFERENCED( bFlags    );
    UNREFERENCED( bChained  );
    UNREFERENCED( bPrevCode );
    UNREFERENCED( iCCWSeq   );

    pLCSDEV = (PLCSDEV)pDEVBLK->dev_data;
    if (!pLCSDEV)
    {
        // Set command reject sense byte, and unit check status
        pDEVBLK->sense[0] = SENSE_CR;
        *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
        return;  // (incomplete group?)
    }

    // Intervention required if the device file is not open
    if (1
        && pDEVBLK->fd < 0
        && !IS_CCW_SENSE  ( bCode )
        && !IS_CCW_CONTROL( bCode )
    )
    {
        pDEVBLK->sense[0] = SENSE_IR;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    // Mask off the modifier bits in the CCW bOpCode
         if ((bCode & 0x07) == 0x07) bOpCode = 0x07;
    else if ((bCode & 0x03) == 0x02) bOpCode = 0x02;
    else if ((bCode & 0x0F) == 0x0C) bOpCode = 0x0C;
    else if ((bCode & 0x03) == 0x01) bOpCode = pDEVBLK->ctcxmode ? (bCode & 0x83) : 0x01;
    else if ((bCode & 0x1F) == 0x14) bOpCode = 0x14;
    else if ((bCode & 0x47) == 0x03) bOpCode = 0x03;
    else if ((bCode & 0xC7) == 0x43) bOpCode = 0x43;
    else
        bOpCode = bCode;


    // Process depending on CCW bOpCode
    switch (bOpCode)
    {
    case 0x01:  // 0MMMMM01  WRITE
        //------------------------------------------------------------
        // WRITE
        //------------------------------------------------------------

        // Return normal status if CCW count is zero
        if (sCount == 0)
        {
            *pUnitStat = CSW_CE | CSW_DE;
            break;
        }

        if (pLCSDEV->bMode == LCSDEV_MODE_IP)
        {
            LCS_Write( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual );
        }
        else  //  (pLCSDEV->bMode == LCSDEV_MODE_SNA)
        {
            LCS_Write_SNA( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual );
        }

        break;

    case 0x81:  // 1MMMMM01  WEOF
        //------------------------------------------------------------
        // WRITE EOF
        //------------------------------------------------------------

        // Return normal status
        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x02:  // MMMMMM10  READ
    case 0x0C:  // MMMM1100  RDBACK
        // -----------------------------------------------------------
        // READ & READ BACKWARDS
        // -----------------------------------------------------------

        // Read data and set unit status and residual byte count
        if (pLCSDEV->bMode == LCSDEV_MODE_IP)
        {
            LCS_Read( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual, pMore );
        }
        else  //  (pLCSDEV->bMode == LCSDEV_MODE_SNA)
        {
            LCS_Read_SNA( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual, pMore );
        }

        break;

    case 0x07:  // MMMMM111  CTL
        // -----------------------------------------------------------
        // CONTROL
        // -----------------------------------------------------------

        if (pLCSDEV->bMode == LCSDEV_MODE_SNA)
        {
            // For SNA this the first CCW in the chain
            // Write Control (0x17), Write (0x01), Read (0x02).
            pLCSDEV->bFlipFlop = bCode;
        }

        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x14:  // XXX10100  SCB
        // -----------------------------------------------------------
        // SENSE COMMAND BYTE
        // -----------------------------------------------------------

        if (pLCSDEV->bMode == LCSDEV_MODE_SNA)
        {
            // For SNA this the first CCW in the chain
            // Sense Command Byte (0x14), Write (0x01), Read (0x02).
            // The SCB does not expect any input.
            pLCSDEV->bFlipFlop = bCode;
        }

        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x03:  // M0MMM011  NOP
        // -----------------------------------------------------------
        // CONTROL NO-OPERATON
        // -----------------------------------------------------------

        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x43:  // 00XXX011  SBM
        // -----------------------------------------------------------
        // SET BASIC MODE
        // Also called Enable Compatability Mode (ECM) by SNA,
        // ECM is the last CCW issued after the XCA is inactivated.
        // -----------------------------------------------------------

        // Command reject if in basic mode
        if (pDEVBLK->ctcxmode == 0)
        {
            pDEVBLK->sense[0] = SENSE_CR;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;

            break;
        }

        // Reset extended mode and return normal status
        pDEVBLK->ctcxmode = 0;

        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0xC3:  // 11000011  SEM
        // -----------------------------------------------------------
        // SET EXTENDED MODE
        // Also called Disable Compatability Mode (DCM) by SNA.
        // DCM is the first CCW issued after the XCA is activated.
        // -----------------------------------------------------------

        if (pLCSDEV->bMode == LCSDEV_MODE_SNA)
        {
            // Reset the frame buffer to an empty state.
            PTT_DEBUG(       "GET  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
            obtain_lock( &pLCSDEV->DevDataLock );
            PTT_DEBUG(       "GOT  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
            pLCSDEV->iFrameOffset  = 0;
            pLCSDEV->fReplyPending = 0;
            pLCSDEV->fDataPending  = 0;
            pLCSDEV->fPendingIctl  = 0;
            PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
            release_lock( &pLCSDEV->DevDataLock );
            remove_and_free_any_lcs_buffers_on_chain( pLCSDEV );
            remove_and_free_any_connections_on_chain( pLCSDEV );
            pLCSDEV->bFlipFlop = 0;
        }

        pDEVBLK->ctcxmode = 1;

        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0xE3:  // 11100011
        // -----------------------------------------------------------
        // PREPARE (PREP)
        // -----------------------------------------------------------

        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0x04:  // 00000100  SENSE
      // -----------------------------------------------------------
      // SENSE
      // -----------------------------------------------------------

        // Command reject if in basic mode
        if (pDEVBLK->ctcxmode == 0)
        {
            pDEVBLK->sense[0] = SENSE_CR;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        // Calculate residual byte count
        iNum = ( sCount < pDEVBLK->numsense ) ?
            sCount : pDEVBLK->numsense;

        *pResidual = sCount - iNum;

        if (sCount < pDEVBLK->numsense)
            *pMore = 1;

        // Copy device sense bytes to channel I/O buffer
        memcpy( pIOBuf, pDEVBLK->sense, iNum );

        // Clear the device sense bytes
        memset( pDEVBLK->sense, 0, sizeof( pDEVBLK->sense ) );

        // Return unit status
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0xE4:  //  11100100  SID
        // -----------------------------------------------------------
        // SENSE ID
        // -----------------------------------------------------------

        // Calculate residual byte count
        iNum = ( sCount < pDEVBLK->numdevid ) ?
            sCount : pDEVBLK->numdevid;

        *pResidual = sCount - iNum;

        if (sCount < pDEVBLK->numdevid)
            *pMore = 1;

        // Copy device identifier bytes to channel I/O buffer
        memcpy( pIOBuf, pDEVBLK->devid, iNum );

        // Return unit status
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    default:
        // ------------------------------------------------------------
        // INVALID OPERATION
        // ------------------------------------------------------------

        // Set command reject sense byte, and unit check status
        pDEVBLK->sense[0] = SENSE_CR;
        *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
    }

    return;
}

// ====================================================================
//                    LCS_StartChannelProgram
// ====================================================================

void  LCS_StartChannelProgram( DEVBLK* pDEVBLK )
{
    PLCSDEV     pLCSDEV;


    pLCSDEV = (PLCSDEV)pDEVBLK->dev_data;
    if (!pLCSDEV) return;  // (incomplete group?)

    if (pLCSDEV->bMode == LCSDEV_MODE_SNA)
    {
        pLCSDEV->fChanProgActive = TRUE;
        pLCSDEV->bFlipFlop = 0;
    }

}

// ====================================================================
//                     LCS_EndChannelProgram
// ====================================================================

void  LCS_EndChannelProgram( DEVBLK* pDEVBLK )
{
    PLCSDEV     pLCSDEV;
    PLCSBLK     pLCSBLK;
    PLCSATTN    pLCSATTN;


    pLCSDEV = (PLCSDEV)pDEVBLK->dev_data;
    if (!pLCSDEV) return;  // (incomplete group?)

    if (pLCSDEV->bMode == LCSDEV_MODE_SNA)
    {

        if (pLCSDEV->fAttnRequired)
        {
            // It would have been nice to have simply called function
            // device_attention at this point, but the channel program
            // is still considered to be busy, and a return code of one
            // would be returned to us.

            pLCSBLK = pLCSDEV->pLCSBLK;

            /* Create an LCSATTN block */
            pLCSATTN = malloc( sizeof( LCSATTN ) );
            if (!pLCSATTN) return;  /* FixMe! Produce a message? */
            pLCSATTN->pNext = NULL;
            pLCSATTN->pDevice = pLCSDEV;

//          if (pLCSBLK->fDebug)                                                                         /* FixMe! Remove! */
//            net_data_trace( pDEVBLK, (BYTE*)pLCSATTN, sizeof( LCSATTN ), NO_DIRECTION, 'D', "LCSATTN in", 0 );  /* FixMe! Remove! */

            /* Add LCSATTN block to start of chain */
            PTT_DEBUG( "GET  AttnLock", 000, pDEVBLK->devnum, 000 );
            obtain_lock( &pLCSBLK->AttnLock );
            PTT_DEBUG( "GOT  AttnLock", 000, pDEVBLK->devnum, 000 );
            {
                PTT_DEBUG( "ADD  Attn", pLCSATTN, pDEVBLK->devnum, 000 );
                pLCSATTN->pNext = pLCSBLK->pAttns;
                pLCSBLK->pAttns = pLCSATTN;
            }
            PTT_DEBUG( "REL  AttnLock", 000, pDEVBLK->devnum, 000 );
            release_lock( &pLCSBLK->AttnLock );

            /* Signal the LCS_AttnThread to process the LCSATTN block(s) on the chain */
            PTT_DEBUG( "GET  AttnEventLock ", 000, pDEVBLK->devnum, 000 );
            obtain_lock( &pLCSBLK->AttnEventLock );
            PTT_DEBUG( "GOT  AttnEventLock ", 000, pDEVBLK->devnum, 000 );
            {
                PTT_DEBUG( "SIG  AttnEvent", 000, pDEVBLK->devnum, 000 );
                signal_condition( &pLCSBLK->AttnEvent );
            }
            PTT_DEBUG( "REL  AttnEventLock ", 000, pDEVBLK->devnum, 000 );
            release_lock( &pLCSBLK->AttnEventLock );

            pLCSDEV->fAttnRequired = FALSE;
        }

        pLCSDEV->bFlipFlop = 0;
        pLCSDEV->fChanProgActive = FALSE;
    }
}

// ====================================================================
//                           LCS_Close
// ====================================================================

int  LCS_Close( DEVBLK* pDEVBLK )
{
    PLCSDEV     pLCSDEV;
    PLCSBLK     pLCSBLK;
    PLCSPORT    pLCSPORT;

    pLCSDEV = (PLCSDEV)pDEVBLK->dev_data;
    if (!pLCSDEV) return 0;  // (was incomplete group)

    pLCSBLK  = pLCSDEV->pLCSBLK;
    pLCSPORT = &pLCSBLK->Port[pLCSDEV->bPort];

    pLCSPORT->icDevices--;

    PTT_DEBUG( "CLOSE: ENTRY      ", 000, pDEVBLK->devnum, pLCSPORT->bPort );

    // Is this the last device on the port?
    if (!pLCSPORT->icDevices)
    {
        PTT_DEBUG( "CLOSE: is last    ", 000, pDEVBLK->devnum, pLCSPORT->bPort );

        // PROGRAMMING NOTE: there's currently no way to interrupt
        // the "LCS_PortThread"s TUNTAP_Read of the adapter. Thus
        // we must simply wait for LCS_PortThread to eventually
        // notice that we're doing a close (via our setting of the
        // fCloseInProgress flag). Its TUNTAP_Read will eventually
        // timeout after a few seconds (currently 5, which is dif-
        // ferent than the DEF_NET_READ_TIMEOUT_SECS timeout value
        // CTCI_Read function uses) and will then do the close of
        // the adapter for us (TUNTAP_Close) so we don't have to.
        // All we need to do is ask it to exit (via our setting of
        // the fCloseInProgress flag) and then wait for it to exit
        // (which, as stated, could take up to a max of 5 seconds).

        // All of this is simply because it's poor form to close a
        // device from one thread while another thread is reading
        // from it. Attempting to do so could trip a race condition
        // wherein the internal i/o buffers used to process the
        // read request could have been freed (by the close call)
        // by the time the read request eventually gets serviced.

        if (pLCSPORT->fd >= 0)
        {
            TID tid = pLCSPORT->tid;
            PTT_DEBUG( "CLOSE: closing... ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            PTT_DEBUG(        "GET  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            obtain_lock( &pLCSPORT->PortEventLock );
            PTT_DEBUG(        "GOT  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            {
                if (pDEVBLK->ccwtrace || pLCSBLK->fDebug)
                    // "%1d:%04X CTC: lcs triggering port %2.2X event"
                    WRMSG( HHC00966, "I", SSID_TO_LCSS( pDEVBLK->ssid ), pDEVBLK->devnum, pLCSPORT->bPort );

                PTT_DEBUG( "CLOSING started=NO", 000, pDEVBLK->devnum, pLCSPORT->bPort );
                pLCSPORT->fPortStarted = 0;
                PTT_DEBUG( "SET  closeInProg  ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
                pLCSPORT->fCloseInProgress = 1;
                PTT_DEBUG(             "SIG  PortEvent    ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
                signal_condition( &pLCSPORT->PortEvent );
            }
            PTT_DEBUG(         "REL  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            release_lock( &pLCSPORT->PortEventLock );
            PTT_DEBUG( "join_thread       ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            join_thread( tid, NULL );
#if defined( OPTION_FTHREADS) 
            PTT_DEBUG( "detach_thread     ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            detach_thread( tid );   // only needed for Fish threads
#endif
        }

        if (pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN  ] && pLCSDEV->pDEVBLK[LCS_READ_SUBCHANN]->fd >= 0)
            pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN  ]->fd = -1;
        if (pLCSDEV->pDEVBLK[ LCS_WRITE_SUBCHANN ] && pLCSDEV->pDEVBLK[LCS_WRITE_SUBCHANN]->fd >= 0)
            pLCSDEV->pDEVBLK[ LCS_WRITE_SUBCHANN ]->fd = -1;

        PTT_DEBUG( "CLOSE: closed     ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    }
    else
        PTT_DEBUG( "CLOSE: not last   ", 000, pDEVBLK->devnum, pLCSPORT->bPort );

    PTT_DEBUG( "CLOSE: cleaning up", 000, pDEVBLK->devnum, pLCSPORT->bPort );

    // Housekeeping
    if (pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN  ] == pDEVBLK)
        pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN  ] = NULL;
    if (pLCSDEV->pDEVBLK[ LCS_WRITE_SUBCHANN ] == pDEVBLK)
        pLCSDEV->pDEVBLK[ LCS_WRITE_SUBCHANN ] = NULL;

    if (!pLCSDEV->pDEVBLK[LCS_READ_SUBCHANN] &&
        !pLCSDEV->pDEVBLK[LCS_WRITE_SUBCHANN])
    {
        // Remove this LCS Device from the chain...

        PLCSDEV  pCurrLCSDev  = NULL;
        PLCSDEV* ppPrevLCSDev = &pLCSBLK->pDevices;

        for (pCurrLCSDev = pLCSBLK->pDevices; pCurrLCSDev; pCurrLCSDev = pCurrLCSDev->pNext)
        {
            if (pCurrLCSDev == pLCSDEV)
            {
                *ppPrevLCSDev = pCurrLCSDev->pNext;

                if (pCurrLCSDev->pszIPAddress)
                {
                    free( pCurrLCSDev->pszIPAddress );
                    pCurrLCSDev->pszIPAddress = NULL;
                }

                free( pLCSDEV );
                pLCSDEV = NULL;
                break;
            }

            ppPrevLCSDev = &pCurrLCSDev->pNext;
        }
    }

    if (!pLCSBLK->pDevices)
    {
        if (pLCSBLK->pszTUNDevice  ) { free( pLCSBLK->pszTUNDevice   ); pLCSBLK->pszTUNDevice   = NULL; }
        if (pLCSBLK->pszOATFilename) { free( pLCSBLK->pszOATFilename ); pLCSBLK->pszOATFilename = NULL; }
        if (pLCSBLK->pszIPAddress  ) { free( pLCSBLK->pszIPAddress   ); pLCSBLK->pszIPAddress   = NULL; }


        if ( pLCSBLK->AttnTid )
        {
            TID tid = pLCSBLK->AttnTid;
            PTT_DEBUG( "CLOSE: closing... ", 000, 000,000 );
            PTT_DEBUG( "GET  AttnEventLock", 000, 000, 000 );
            obtain_lock( &pLCSBLK->AttnEventLock );
            PTT_DEBUG( "GOT  AttnEventLock", 000, 000, 000 );
            {
                PTT_DEBUG( "SET  closeInProg  ", 000, 000, 000 );
                pLCSBLK->fCloseInProgress = 1;
                PTT_DEBUG( "SIG  AttnEvent", 000, 000, 000 );
                signal_condition( &pLCSBLK->AttnEvent );
            }
            PTT_DEBUG( "REL  AttnEventLock", 000, 000, 000 );
            release_lock( &pLCSBLK->AttnEventLock );
            PTT_DEBUG( "join_thread       ", 000, 000, 000 );
            join_thread( tid, NULL );
            PTT_DEBUG( "detach_thread     ", 000, 000, 000 );
            detach_thread( tid );
        }

        free( pLCSBLK );
        pLCSBLK = NULL;
    }

    pDEVBLK->dev_data = NULL;

    PTT_DEBUG( "CLOSE: EXIT       ", 000, pDEVBLK->devnum, pLCSPORT->bPort );

    return 0;
}

// ====================================================================
//                         LCS_Query
// ====================================================================

void  LCS_Query( DEVBLK* pDEVBLK, char** ppszClass,
                 int     iBufLen, char*  pBuffer )
{
    char filename[ PATH_MAX + 1 ];      /* full path or just name    */

    char *sType[] = { "", " Pri", " Sec" };

    PLCSDEV  pLCSDEV;

    BEGIN_DEVICE_CLASS_QUERY( "CTCA", pDEVBLK, ppszClass, iBufLen, pBuffer );

    pLCSDEV = (PLCSDEV) pDEVBLK->dev_data;

    if (!pLCSDEV)
    {
        strlcpy(pBuffer,"*Uninitialized",iBufLen);
        return;
    }

    snprintf( pBuffer, iBufLen, "LCS Port %2.2X %s%s (%s)%s IO[%"PRIu64"]",
              pLCSDEV->bPort,
              pLCSDEV->bMode == LCSDEV_MODE_IP ? "IP" : "SNA",
              sType[pLCSDEV->bType],
              pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort].szNetIfName,
              pLCSDEV->pLCSBLK->fDebug ? " -d" : "",
              pDEVBLK->excps );
}

// ====================================================================
//                   LCS Multi-Write Support
// ====================================================================

#if defined( OPTION_W32_CTCI )

static void  LCS_BegMWrite( DEVBLK* pDEVBLK )
{
    if (((PLCSDEV)pDEVBLK->dev_data)->pLCSBLK->fNoMultiWrite) return;
    PTT_TIMING( "b4 begmw", 0, 0, 0 );
    TUNTAP_BegMWrite( pDEVBLK->fd, CTC_DEF_FRAME_BUFFER_SIZE );
    PTT_TIMING( "af begmw", 0, 0, 0);
}

static void  LCS_EndMWrite( DEVBLK* pDEVBLK, int nEthBytes, int nEthFrames )
{
    if (((PLCSDEV)pDEVBLK->dev_data)->pLCSBLK->fNoMultiWrite) return;
    PTT_TIMING( "b4 endmw", 0, nEthBytes, nEthFrames );
    TUNTAP_EndMWrite( pDEVBLK->fd );
    PTT_TIMING( "af endmw", 0, nEthBytes, nEthFrames );
}

#else // !defined( OPTION_W32_CTCI )

  #define  LCS_BegMWrite( pDEVBLK )
  #define  LCS_EndMWrite( pDEVBLK, nEthBytes, nEthFrames )

#endif // defined( OPTION_W32_CTCI )

// ====================================================================
//                         LCS_Write
// ====================================================================
// The guest o/s is issuing a Write CCW for our LCS device. All LCS
// Frames in its buffer which are NOT internal Command Frames will
// be immediately written to our host's adapter (via TunTap). Frames
// that are internal Command Frames however are processed internally
// and cause a "reply" frame to be enqueued to the LCS Device output
// buffer to be eventually returned back to the guest the next time
// it issues a Read CCW (satisfied by LCS_Read).
// --------------------------------------------------------------------
//
// IP mode handles Ethernet frames and LCS commands, both of which
// are prefixed with a 4-byte LCSHDR.
//
// The following illustrates an Ethernet frame containing an IPv4
// packet with an ICMP Ping reply:-
//   HHC00979D LCS: data: +0000< 00660100 02000042 22800200 00422281  .f......".....". ...../...../...a
//   HHC00979D LCS: data: +0010< 08004500 00540038 40004001 C423C0A8  ..E..T.8@.@..#.. ........ . .D.{y
//   HHC00979D LCS: data: +0020< FA7DC0A8 FA7E0000 CFF40002 00010A05  .}...~.......... .'{y.=...4......
//   HHC00979D LCS: data: +0030< 63600000 0000FDCF 06000000 00001011  c`.............. .-..............
//   HHC00979D LCS: data: +0040< 12131415 16171819 1A1B1C1D 1E1F2021  .............. ! ................
//   HHC00979D LCS: data: +0050< 22232425 26272829 2A2B2C2D 2E2F3031  "#$%&'()*+,-./01 ................
//   HHC00979D LCS: data: +0060< 32333435 36370000                    234567..         ........
// The first four bytes are the LCSHDR, the next fourteen bytes are
// the Ethernet header, and the remaining bytes are the IP packet
// with the ICMP Ping reply.
//
// The following illustrates an LCS Command frame containing a Start
// LAN command:-
//   HHC00979D LCS: data: +0000< 001D0000 01000000 00000100 00000000  ................ ................
//   HHC00979D LCS: data: +0010< 00000000 00000000 00000000 000000    ...............  ...............
// The first four bytes are the LCSHDR, the remaining bytes are the
// Start LAN command.
//

PUSH_GCC_WARNINGS()
DISABLE_GCC_UNUSED_SET_WARNING; // (because only Windows-only LCS_EndMWrite uses nEthBytes)

void  LCS_Write( DEVBLK* pDEVBLK,   U32   sCount,
                 BYTE*   pIOBuf,    BYTE* pUnitStat,
                 U32*    pResidual )
{

    PLCSDEV     pLCSDEV      = (PLCSDEV) pDEVBLK->dev_data;
    PLCSBLK     pLCSBLK      = pLCSDEV->pLCSBLK;
    PLCSPORT    pLCSPORT     = &pLCSBLK->Port[ pLCSDEV->bPort ];
    PLCSHDR     pLCSHDR      = NULL;
    PLCSCMDHDR  pCmdFrame    = NULL;
    PLCSETHFRM  pLCSEthFrame = NULL;
    PETHFRM     pEthFrame    = NULL;
    U16         iOffset      = 0;
    U16         iPrevOffset  = 0;
    U16         iLength      = 0;
    U16         iEthLen      = 0;
    U16         hwEthernetType;
    BYTE        bHas8022;
    BYTE        bHas8022Snap;
    int         nEthFrames   = 0;
    int         nEthBytes    = 0;
    int         iTraceLen;
    char        buf[32];
    char        cPktType[24];


    // Display up to pLCSBLK->iTraceLen bytes of the data coming from the guest, if debug is active
    if (pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: Accept data of size %d bytes from guest"
        WRMSG(HHC00981, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,  pDEVBLK->typname, (int)sCount );
        if (pLCSBLK->iTraceLen)
        {
            iTraceLen = sCount;
            if (iTraceLen > pLCSBLK->iTraceLen)
            {
                iTraceLen = pLCSBLK->iTraceLen;
                // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, (int)(sCount - iTraceLen) );
            }
            net_data_trace( pDEVBLK, pIOBuf, iTraceLen, FROM_GUEST, 'D', "data", 0 );
        }
    }

    // Process each frame in the buffer...

    PTT_DEBUG( "WRIT ENTRY        ", 000, pDEVBLK->devnum, -1 );
    PTT_TIMING( "beg write", 0, 0, 0 );

    LCS_BegMWrite( pDEVBLK ); // (performance)

    while (1)
    {
        // Fix-up the LCS header pointer to the current frame
        pLCSHDR = (PLCSHDR)( pIOBuf + iOffset );

        // Save current offset so we can tell how big next frame is
        iPrevOffset = iOffset;

        // Get the next frame offset, exit loop if 0
        FETCH_HW( iOffset, pLCSHDR->hwOffset );

        if (iOffset == 0)   // ("EOF")
            break;

        // Calculate size of this LCS Frame
        iLength = iOffset - iPrevOffset;

        switch (pLCSHDR->bType)
        {
        case LCS_FRMTYP_ENET:   // Ethernet Passthru

            PTT_DEBUG( "WRIT: Eth frame   ", 000, pDEVBLK->devnum, -1 );

            pLCSEthFrame = (PLCSETHFRM) pLCSHDR;
            pEthFrame    = (PETHFRM) pLCSEthFrame->bData;
            iEthLen      = iLength - sizeof(LCSETHFRM);

            GetFrameInfo( pEthFrame, &cPktType[0], &hwEthernetType, &bHas8022, &bHas8022Snap );

            // Fill in LCS source MAC address if not specified by guest program
            if (memcmp( pEthFrame->bSrcMAC, zeromac, sizeof( MAC )) == 0)
            {
                memcpy( pEthFrame->bSrcMAC, pLCSPORT->MAC_Address, IFHWADDRLEN);
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
                pEthFrame->bSrcMAC[5]++;    /* Get next MAC address */
#endif
            }

            // Perform outbound checksum offloading if necessary
            if (pLCSPORT->fDoCkSumOffload)
            {
                PTT_TIMING( "beg csumoff", 0, iEthLen, 0 );
                EtherIpv4CkSumOffload( (BYTE*) pEthFrame, iEthLen );
                PTT_TIMING( "end csumoff", 0, iEthLen, 0 );
            }

            // Trace Ethernet frame before sending to TAP device
            if (pLCSBLK->fDebug)
            {
                // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
                WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     pLCSHDR->bSlot, iEthLen, cPktType,
                                     pLCSPORT->szNetIfName );
                if (pLCSBLK->iTraceLen)
                {
                    iTraceLen = iEthLen;
                    if (iTraceLen > pLCSBLK->iTraceLen)
                    {
                        iTraceLen = pLCSBLK->iTraceLen;
                        // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                        WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                             iTraceLen, (iEthLen - iTraceLen) );
                    }
                    net_data_trace( pDEVBLK, (BYTE*)pEthFrame, iTraceLen, FROM_GUEST, 'D', "eth frame", 0 );
                }
            }

            // Write the Ethernet frame to the TAP device
            nEthBytes += iEthLen;
            nEthFrames++;
            PTT_DEBUG( "WRIT: writing...  ", 000, pDEVBLK->devnum, -1 );
            PTT_TIMING( "b4 write", 0, iEthLen, 1 );
            if (TUNTAP_Write( pDEVBLK->fd, (BYTE*) pEthFrame, iEthLen ) != iEthLen)
            {
                PTT_TIMING( "*WRITE ERR", 0, iEthLen, 1 );
                // "%1d:%04X CTC: error writing to file %s: %s"
                WRMSG( HHC00936, "E",
                        SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->filename,
                        strerror( errno ) );
                pDEVBLK->sense[0] = SENSE_EC;
                *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
                LCS_EndMWrite( pDEVBLK, nEthBytes, nEthFrames );
                PTT_DEBUG( "WRIT EXIT         ", 000, pDEVBLK->devnum, -1 );
                return;
            }
            PTT_TIMING( "af write", 0, iEthLen, 1 );
            break;

        case LCS_FRMTYP_CMD:    // LCS Command Frame

            pCmdFrame = (PLCSCMDHDR)pLCSHDR;

            PTT_DEBUG( "WRIT: Cmd frame   ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );

            // Trace received command frame...
            if (pLCSBLK->fDebug)
            {
                // "%1d:%04X CTC: lcs command packet received"
                WRMSG( HHC00922, "D", SSID_TO_LCSS( pDEVBLK->ssid ), pDEVBLK->devnum );
                net_data_trace( pDEVBLK, (BYTE*)pCmdFrame, iLength, FROM_GUEST, 'D', "command", 0 );
            }

            // Ignore packets that appear to be inbound and not outbound.
            //
            // The Linux kernel LCS driver has two values defined that
            // might be found in variable pCmdFrame->bInitiator: #define
            // LCS_INITIATOR_TCPIP (0x00), and #define LCS_INITIATOR_LGW
            // (0x01), where 'LGW' is an abbreviation of 'LAN Gateway'.
            //
            // Older kernel LCS drivers had lots of code related to LGW,
            // but most of it has been removed from modern kernels (4.3,
            // at the time of writing).
            //
            // I'm not sure, but I think that applications, for example
            // IBM's Operator Facility/2, could send commands to a 3172
            // from a host attached to the LAN, and those commands would
            // arrive with pCmdFrame->bInitiator == LCS_INITIATOR_LGW.
            //
            // The current Linux kernel LCS driver only checks for the
            // pCmdFrame->bInitiator == LCS_INITIATOR_LGW in inbound
            // packets arriving from the LCS device; outbound packets
            // sent to the LCS device always have pCmdFrame->bInitiator
            // set to LCS_INITIATOR_TCPIP.

            if (pCmdFrame->bInitiator == LCS_INITIATOR_LGW)
            {
                PTT_DEBUG( "CMD initiator LGW", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    // "%1d:%04X CTC: lcs command packet IGNORED (bInitiator == LGW)"
                    WRMSG( HHC00977, "D", SSID_TO_LCSS( pDEVBLK->ssid ), pDEVBLK->devnum );
                break;
            }

            switch (pCmdFrame->bCmdCode)
            {
                //  HHC00933  =  "%1d:%04X CTC: executing command %s"

            case LCS_CMD_STARTUP:       // Start Host
                PTT_DEBUG( "CMD=StartUp       ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "startup" );
                LCS_Startup( pLCSDEV, pCmdFrame, iLength );
                break;

            case LCS_CMD_SHUTDOWN:      // Shutdown Host
                PTT_DEBUG( "CMD=Shutdown      ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "shutdown" );
                LCS_Shutdown( pLCSDEV, pCmdFrame, iLength );
                break;

            case LCS_CMD_STRTLAN:       // Start LAN
                PTT_DEBUG( "CMD=Start LAN     ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "start lan" );
                LCS_StartLan( pLCSDEV, pCmdFrame, iLength );
                break;

            case LCS_CMD_STOPLAN:       // Stop LAN
                PTT_DEBUG( "CMD=Stop LAN      ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "stop lan" );
                LCS_StopLan( pLCSDEV, pCmdFrame, iLength );
                break;

            case LCS_CMD_QIPASSIST:     // Query IP Assists
                PTT_DEBUG( "CMD=Query IPAssist", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "query IP assist" );
                LCS_QueryIPAssists( pLCSDEV, pCmdFrame, iLength );
                break;

            case LCS_CMD_LANSTAT:       // LAN Stats
                PTT_DEBUG( "CMD=LAN Statistics", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "lan statistics" );
                LCS_LanStats( pLCSDEV, pCmdFrame, iLength );
                break;

            case LCS_CMD_SETIPM:        // Set IP Multicast
                PTT_DEBUG( "CMD=Set IP Multicast", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "set multicast" );
                LCS_AddMulticast( pLCSDEV, pCmdFrame, iLength );
                break;

            case LCS_CMD_DELIPM:        // Delete IP Multicast
                PTT_DEBUG( "CMD=Delete IP Multicast", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "delete multicast" );
                LCS_DelMulticast( pLCSDEV, pCmdFrame, iLength );
                break;

//          case LCS_CMD_GENSTAT:       // General Stats
//          case LCS_CMD_LISTLAN:       // List LAN
//          case LCS_CMD_LISTLAN2:      // List LAN (another version)
//          case LCS_CMD_TIMING:        // Timing request
            default:
                PTT_DEBUG( "*CMD=Unsupported! ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                if (pLCSBLK->fDebug)
                {
                    // "%1d:%04X CTC: executing command %s"
                    MSGBUF( buf, "other (0x%2.2X)", pCmdFrame->bCmdCode );
                    WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, buf );
                }
                LCS_DefaultCmdProc( pLCSDEV, pCmdFrame, iLength );
                break;

            } // end switch (LCS Command Frame cmd code)

            break; // end case LCS_FRMTYP_CMD

        default:
            PTT_DEBUG( "*WRIT Unsupp frame", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
            // "%1d:%04X CTC: lcs write: unsupported frame type 0x%2.2X"
            WRMSG( HHC00937, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pLCSHDR->bType );
            ASSERT( FALSE );
            pDEVBLK->sense[0] = SENSE_EC;
            *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
            LCS_EndMWrite( pDEVBLK, nEthBytes, nEthFrames );
            PTT_TIMING( "end write",  0, 0, 0 );
            PTT_DEBUG( "WRIT EXIT         ", 000, pDEVBLK->devnum, -1 );
            return;

        } // end switch (LCS Frame type)

    } // end while (1) for IP

    LCS_EndMWrite( pDEVBLK, nEthBytes, nEthFrames ); // (performance)

    // ----------------------------------------------------------------
    //    End of LCS_Write
    // ----------------------------------------------------------------

    *pResidual = 0;
    *pUnitStat = CSW_CE | CSW_DE;

    PTT_TIMING( "end write",  0, 0, 0 );
    PTT_DEBUG( "WRIT EXIT         ", 000, pDEVBLK->devnum, -1 );
}   // End of LCS_Write

POP_GCC_WARNINGS()

// ====================================================================
//                         LCS_Startup
// ====================================================================

static void  LCS_Startup( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
    LCSSTRTFRM  Reply;
    int         iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSSTRTFRM pLCSSTRTFRM = (PLCSSTRTFRM)&Reply;
    U16         iOrigMaxFrameBufferSize;

    INIT_REPLY_FRAME( pLCSSTRTFRM, iReplyLen, pCmdFrame, iCmdLen );

    pLCSSTRTFRM->bLCSCmdHdr.bLanType      = LCS_FRMTYP_ENET;
    pLCSSTRTFRM->bLCSCmdHdr.bRelAdapterNo = pLCSDEV->bPort;

    // Save the max buffer size parameter
    iOrigMaxFrameBufferSize = pLCSDEV->iMaxFrameBufferSize;
    // If original is 0 set to compiled in buffer size
    if(!iOrigMaxFrameBufferSize) iOrigMaxFrameBufferSize=(U16)sizeof(pLCSDEV->bFrameBuffer);

    FETCH_HW( pLCSDEV->iMaxFrameBufferSize, ((PLCSSTRTFRM)pCmdFrame)->hwBufferSize );

    // Make sure it doesn't exceed our compiled maximum
    if (pLCSDEV->iMaxFrameBufferSize > sizeof(pLCSDEV->bFrameBuffer))
    {
        // "%1d:%04X CTC: lcs startup: frame buffer size 0x%4.4X '%s' compiled size 0x%4.4X: ignored"
        WRMSG(HHC00939, "W", SSID_TO_LCSS(pLCSDEV->pDEVBLK[LCS_WRITE_SUBCHANN]->ssid),
            pLCSDEV->pDEVBLK[LCS_WRITE_SUBCHANN]->devnum,
            pLCSDEV->iMaxFrameBufferSize,
            "LCS",
            (int)sizeof( pLCSDEV->bFrameBuffer ) );
        pLCSDEV->iMaxFrameBufferSize = iOrigMaxFrameBufferSize;
    }
    else
    {
        // Make sure it's not smaller than the compiled minimum size
        if (pLCSDEV->iMaxFrameBufferSize < CTC_MIN_FRAME_BUFFER_SIZE)
        {
            // "%1d:%04X CTC: lcs startup: frame buffer size 0x%4.4X '%s' compiled size 0x%4.4X: ignored"
            WRMSG(HHC00939, "W", SSID_TO_LCSS(pLCSDEV->pDEVBLK[LCS_WRITE_SUBCHANN]->ssid),
                pLCSDEV->pDEVBLK[LCS_WRITE_SUBCHANN]->devnum,
                pLCSDEV->iMaxFrameBufferSize,
                "LCS",
                CTC_MIN_FRAME_BUFFER_SIZE );
            pLCSDEV->iMaxFrameBufferSize = iOrigMaxFrameBufferSize;
        }
    }

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSSTRTFRM, iReplyLen );

    pLCSDEV->fDevStarted = 1;
}

// ====================================================================
//                         LCS_Shutdown
// ====================================================================

static void  LCS_Shutdown( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
//    char        Reply[128];
    LCSSTDFRM   Reply;
    int         iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSSTDFRM  pLCSSTDFRM = (PLCSSTDFRM)&Reply;

    INIT_REPLY_FRAME( pLCSSTDFRM, iReplyLen, pCmdFrame, iCmdLen );

    pLCSSTDFRM->bLCSCmdHdr.bLanType      = LCS_FRMTYP_ENET;
    pLCSSTDFRM->bLCSCmdHdr.bRelAdapterNo = pLCSDEV->bPort;

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSSTDFRM, iReplyLen );

    pLCSDEV->fDevStarted = 0;
}

// ====================================================================
//                       UpdatePortStarted
// ====================================================================

static void  UpdatePortStarted( int bStarted, DEVBLK* pDEVBLK, PLCSPORT pLCSPORT )
{
    PTT_DEBUG(        "GET  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    obtain_lock( &pLCSPORT->PortDataLock );
    PTT_DEBUG(        "GOT  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    {
        // The following will either caise the LCS_PortThread to start
        // reading packets or stop reading packets (fPortStarted = 1/0)

        PTT_DEBUG( "UPDTPORTSTARTED   ", bStarted, pDEVBLK->devnum, pLCSPORT->bPort );
        pLCSPORT->fPortStarted = bStarted;
    }
    PTT_DEBUG(         "REL  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    release_lock( &pLCSPORT->PortDataLock );

    if (pDEVBLK->ccwtrace || pLCSPORT->pLCSBLK->fDebug)
        // "%1d:%04X CTC: lcs triggering port %2.2X event"
        WRMSG( HHC00966, "I", SSID_TO_LCSS( pDEVBLK->ssid ), pDEVBLK->devnum, pLCSPORT->bPort );

    PTT_DEBUG(        "GET  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    obtain_lock( &pLCSPORT->PortEventLock );
    PTT_DEBUG(        "GOT  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    {
        // Wake up the LCS_PortThread...

        PTT_DEBUG(             "SIG  PortEvent    ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
        signal_condition( &pLCSPORT->PortEvent );
    }
    PTT_DEBUG(         "REL  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    release_lock( &pLCSPORT->PortEventLock );

    PTT_DEBUG( "UPDTPORT pause 150", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    USLEEP( 150*1000 );
}

// ====================================================================
//                         LCS_StartLan
// ====================================================================

static void  LCS_StartLan( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
    LCSSTRTFRM  Reply;
    int         iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSSTRTFRM pLCSSTRTFRM = (PLCSSTRTFRM)&Reply;
    PLCSPORT    pLCSPORT;
#ifdef OPTION_TUNTAP_DELADD_ROUTES
    PLCSRTE     pLCSRTE;
#endif // OPTION_TUNTAP_DELADD_ROUTES
    DEVBLK*     pDEVBLK;
    int         nIFFlags;
    U8          fStartPending = 0;


    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    pDEVBLK  = pLCSDEV->pDEVBLK[ LCS_WRITE_SUBCHANN ];
    if (!pDEVBLK) pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];  /* SNA has only one device */

    INIT_REPLY_FRAME( pLCSSTRTFRM, iReplyLen, pCmdFrame, iCmdLen );

    // Serialize access to eliminate ioctl errors
    PTT_DEBUG(        "GET  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    obtain_lock( &pLCSPORT->PortDataLock );
    PTT_DEBUG(        "GOT  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    {
        // Configure the TAP interface if used
        PTT_DEBUG( "STRTLAN if started", pLCSPORT->fPortStarted, pDEVBLK->devnum, pLCSPORT->bPort );
        if (pLCSPORT->fUsed && pLCSPORT->fPortCreated && !pLCSPORT->fPortStarted)
        {
            PTT_DEBUG( "STRTLAN started=NO", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            nIFFlags =              // Interface flags
                0
                | IFF_UP            // (interface is being enabled)
                | IFF_BROADCAST     // (interface broadcast addr is valid)
                ;

#if defined( TUNTAP_IFF_RUNNING_NEEDED )

            nIFFlags |=             // ADDITIONAL Interface flags
                0
                | IFF_RUNNING       // (interface is ALSO operational)
                ;

#endif /* defined( TUNTAP_IFF_RUNNING_NEEDED ) */

            // Enable the interface by turning on the IFF_UP flag...
            // This lets the packets start flowing...

            if (!pLCSPORT->fPreconfigured)
                VERIFY( TUNTAP_SetFlags( pLCSPORT->szNetIfName, nIFFlags ) == 0 );

            fStartPending = 1;

#ifdef OPTION_TUNTAP_DELADD_ROUTES

            // Add any needed extra routing entries the
            // user may have specified in their OAT file
            // to the host's routing table...

            if (!pLCSPORT->fPreconfigured)
            {
                for (pLCSRTE = pLCSPORT->pRoutes; pLCSRTE; pLCSRTE = pLCSRTE->pNext)
                {
                    VERIFY( TUNTAP_AddRoute( pLCSPORT->szNetIfName,
                                     pLCSRTE->pszNetAddr,
                                     pLCSRTE->pszNetMask,
                                     NULL,
                                     RTF_UP ) == 0 );
                }
            }
#endif // OPTION_TUNTAP_DELADD_ROUTES
        }
    }
    PTT_DEBUG(         "REL  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    release_lock( &pLCSPORT->PortDataLock );

#ifdef OPTION_TUNTAP_DELADD_ROUTES

    // Add a Point-To-Point routing entry to the
    // host's routing table for our interface...

    if (!pLCSPORT->fPreconfigured)
    {
        if (pLCSDEV->pszIPAddress)
        {
            VERIFY( TUNTAP_AddRoute( pLCSPORT->szNetIfName,
                             pLCSDEV->pszIPAddress,
                             "255.255.255.255",
                             NULL,
                             RTF_UP | RTF_HOST ) == 0 );
        }
    }
#endif // OPTION_TUNTAP_DELADD_ROUTES

    // PROGRAMMING NOTE: it's important to enqueue the reply frame BEFORE
    // we trigger the LCS_PortThread to start reading the adapter and
    // begin enqueuing Ethernet frames. This is so the guest receives
    // the reply to its cmd BEFORE it sees any Ethernet packets that might
    // result from its StartLAN cmd.

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSSTRTFRM, iReplyLen );

    if (fStartPending)
        UpdatePortStarted( TRUE, pDEVBLK, pLCSPORT );
}

// ====================================================================
//                         LCS_StopLan
// ====================================================================

static void  LCS_StopLan( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
    LCSSTDFRM   Reply;
    int         iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSSTDFRM  pLCSSTDFRM = (PLCSSTDFRM)&Reply;
    PLCSPORT    pLCSPORT;
#ifdef OPTION_TUNTAP_DELADD_ROUTES
    PLCSRTE     pLCSRTE;
#endif // OPTION_TUNTAP_DELADD_ROUTES
    DEVBLK*     pDEVBLK;


    pLCSPORT = &pLCSDEV->pLCSBLK->Port[ pLCSDEV->bPort ];
    pDEVBLK  =  pLCSDEV->pDEVBLK[ LCS_WRITE_SUBCHANN ];
    if (!pDEVBLK) pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];  /* SNA has only one device */

    INIT_REPLY_FRAME( pLCSSTDFRM, iReplyLen, pCmdFrame, iCmdLen );

    // Serialize access to eliminate ioctl errors
    PTT_DEBUG(        "GET  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    obtain_lock( &pLCSPORT->PortDataLock );
    PTT_DEBUG(        "GOT  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    {
        // Disable the interface by turning off the IFF_UP flag...
        if (!pLCSPORT->fPreconfigured)
            VERIFY( TUNTAP_SetFlags( pLCSPORT->szNetIfName, 0 ) == 0 );

#ifdef OPTION_TUNTAP_DELADD_ROUTES

        // Remove routing entries from host's routing table...

        // First, remove the Point-To-Point routing entry
        // we added when we brought the interface IFF_UP...

        if (!pLCSPORT->fPreconfigured)
        {
            if (pLCSDEV->pszIPAddress)
            {
                VERIFY( TUNTAP_DelRoute( pLCSPORT->szNetIfName,
                                 pLCSDEV->pszIPAddress,
                                 "255.255.255.255",
                                 NULL,
                                 RTF_HOST ) == 0 );
            }
        }

        // Next, remove any extra routing entries
        // (specified by the user in their OAT file)
        // that we may have also added...

        if (!pLCSPORT->fPreconfigured)
        {
            for (pLCSRTE = pLCSPORT->pRoutes; pLCSRTE; pLCSRTE = pLCSRTE->pNext)
            {
                VERIFY( TUNTAP_DelRoute( pLCSPORT->szNetIfName,
                                 pLCSRTE->pszNetAddr,
                                 pLCSRTE->pszNetMask,
                                 NULL,
                                 RTF_UP ) == 0 );
            }
        }
#endif // OPTION_TUNTAP_DELADD_ROUTES
    }
    PTT_DEBUG(         "REL  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    release_lock( &pLCSPORT->PortDataLock );

    // Now that the tuntap device has been stopped and the packets
    // are no longer flowing, tell the LCS_PortThread to stop trying
    // to read from the tuntap device (adapter).

    UpdatePortStarted( FALSE, pDEVBLK, pLCSPORT );

    // Now that we've stopped new packets from being added to our
    // frame buffer we can now finally enqueue our reply frame
    // to our frame buffer (so LCS_Read can return it to the guest).

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSSTDFRM, iReplyLen );
}

// ====================================================================
//                      LCS_QueryIPAssists
// ====================================================================

static void  LCS_QueryIPAssists( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
    LCSQIPFRM   Reply;
    int         iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSQIPFRM  pLCSQIPFRM = (PLCSQIPFRM)&Reply;
    PLCSPORT    pLCSPORT;

    INIT_REPLY_FRAME( pLCSQIPFRM, iReplyLen, pCmdFrame, iCmdLen );

    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];

    STORE_HW( pLCSQIPFRM->hwNumIPPairs,         _countof( pLCSPORT->MCastTab ));
    STORE_HW( pLCSQIPFRM->hwIPAssistsSupported, pLCSPORT->sIPAssistsSupported );
    STORE_HW( pLCSQIPFRM->hwIPAssistsEnabled,   pLCSPORT->sIPAssistsEnabled   );
    STORE_HW( pLCSQIPFRM->hwIPVersion,          0x0004 ); // (IPv4 only)

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSQIPFRM, iReplyLen );
}

// ====================================================================
//                         LCS_LanStats
// ====================================================================

static void  LCS_LanStats( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{

    LCSLSTFRM  Reply;
    int        iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSLSTFRM pLCSLSTFRM = (PLCSLSTFRM)&Reply;
    PLCSPORT   pLCSPORT;


    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];

    // Get and display the tap interface MAC address.
    GetIfMACAddress( pLCSPORT );

    INIT_REPLY_FRAME( pLCSLSTFRM, iReplyLen, pCmdFrame, iCmdLen );

    /* Respond with a different MAC address for the LCS side */
    /* unless the TAP mechanism is designed as such          */
    /* cf : hostopts.h for an explanation                    */
    iReplyLen = sizeof(Reply);
    memcpy( pLCSLSTFRM->MAC_Address, pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
    pLCSLSTFRM->MAC_Address[5]++;
#endif
    // FIXME: Really should read /proc/net/dev to retrieve actual stats

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSLSTFRM, iReplyLen );
}

// ====================================================================
//                       LCS_DoMulticast
// ====================================================================

static  void  LCS_DoMulticast( int ioctlcode, PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{

    LCSIPMFRM         Reply;
    int               iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSIPMFRM        pLCSIPMFRM = (PLCSIPMFRM)&Reply;
    const LCSIPMFRM*  pIPMFrame;
    LCSPORT*          pLCSPORT;
    const MAC*        pMAC;
    ifreq             ifr = {0};
    int               rc, badrc = 0, errnum = 0;
    U16               i, numpairs;
    char*             pszMAC;
    const char*       what;

    // Initialize reply frame

    INIT_REPLY_FRAME( pLCSIPMFRM, iReplyLen, pCmdFrame, iCmdLen );
    pIPMFrame = (const LCSIPMFRM*) pCmdFrame;
    pLCSPORT  = &pLCSDEV->pLCSBLK->Port[ pLCSDEV->bPort ];

    // Retrieve number of MAC addresses in their request

    FETCH_HW( numpairs, pIPMFrame->hwNumIPPairs );
    if (numpairs > MAX_IP_MAC_PAIRS)
        numpairs = MAX_IP_MAC_PAIRS;

    // If tuntap multicast assist is available, use it.
    // Otherwise keep track of guest's request manually.

    if (pLCSPORT->fDoMCastAssist)  // (manual multicast assist?)
    {
        what = (U32) SIOCADDMULTI == (U32) ioctlcode ? "MACTabAdd"
             : (U32) SIOCDELMULTI == (U32) ioctlcode ? "MACTabRem" : "???";

        for (i=0, badrc=0; i < numpairs; i++)
        {
            pMAC = &pIPMFrame->IP_MAC_Pair[i].MAC_Address;

            // Remember (or forget) this MAC for later

            if ((U32) SIOCADDMULTI == (U32) ioctlcode)
            {
                if ((rc = MACTabAdd( pLCSPORT->MCastTab, (BYTE*) pMAC, 0 )) == 0)
                {
                    pLCSPORT->nMCastCount++;

                    if (pLCSDEV->pLCSBLK->fDebug)
                    {
                        VERIFY( FormatMAC( &pszMAC, (BYTE*) pMAC ) == 0);
                        // "CTC: lcs device '%s' port %2.2X: %s %s: ok"
                        WRMSG( HHC00964, "D", pLCSPORT->szNetIfName, pLCSPORT->bPort,
                            what, pszMAC );
                        free( pszMAC );
                    }
                }
                else
                    badrc = -rc;    // (convert to errno)
            }
            else // ((U32) SIOCDELMULTI == (U32) ioctlcode)
            {
                if ((rc = MACTabRem( pLCSPORT->MCastTab, (BYTE*) pMAC )) == 0)
                {
                    pLCSPORT->nMCastCount--;

                    if (pLCSDEV->pLCSBLK->fDebug)
                    {
                        VERIFY( FormatMAC( &pszMAC, (BYTE*) pMAC ) == 0);
                        // "CTC: lcs device '%s' port %2.2X: %s %s: ok"
                        WRMSG( HHC00964, "D", pLCSPORT->szNetIfName, pLCSPORT->bPort,
                            what, pszMAC );
                        free( pszMAC );
                    }
                }
                else
                    badrc = -rc;    // (convert to errno)
            }
        }

        // Set return code and issue message if any requests failed.

        if (badrc)
        {
            errnum = badrc;  // (get errno)
            // "CTC: error in function %s: %s"
            WRMSG( HHC00940, "E", what, strerror( errnum ));
            STORE_HW( pLCSIPMFRM->bLCSCmdHdr.hwReturnCode, 0xFFFF );
        }
    }
    else // (!pLCSPORT->fDoMCastAssist): let tuntap do it for us
    {
        // Issue ioctl for each MAC address in their request

        what = (U32) SIOCADDMULTI == (U32) ioctlcode ? "SIOCADDMULTI"
             : (U32) SIOCDELMULTI == (U32) ioctlcode ? "SIOCDELMULTI" : "???";

        STRLCPY( ifr.ifr_name, pLCSPORT->szNetIfName );

#if defined( SIOCGIFHWADDR )
        for (i=0, badrc=0; i < numpairs; i++)
        {
            pMAC = &pIPMFrame->IP_MAC_Pair[i].MAC_Address;
            memcpy( ifr.ifr_hwaddr.sa_data, pMAC, sizeof( MAC ));

            if ((rc = TUNTAP_IOCtl( 0, ioctlcode, (char*) &ifr )) == 0)
            {
                if (pLCSDEV->pLCSBLK->fDebug)
                {
                    VERIFY( FormatMAC( &pszMAC, (BYTE*) pMAC ) == 0);
                    // "CTC: lcs device '%s' port %2.2X: %s %s: ok"
                    WRMSG( HHC00964, "D", pLCSPORT->szNetIfName, pLCSPORT->bPort,
                        what, pszMAC );
                    free( pszMAC );
                }
            }
            else
            {
                badrc = rc;
                errnum = HSO_errno;
            }
        }

        // Issue error message if any of the requests failed.

        if (badrc)
        {
            // "CTC: ioctl %s failed for device %s: %s; ... ignoring and continuing"
            WRMSG( HHC00941, "W", what, pLCSPORT->szNetIfName, strerror( errnum ));
            STORE_HW( pLCSIPMFRM->bLCSCmdHdr.hwReturnCode, 0xFFFF );
        }
#endif // defined( SIOCGIFHWADDR )
    }

    // Queue response back to caller

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSIPMFRM, iReplyLen );
}

// ====================================================================
//                       LCS_AddMulticast
// ====================================================================

static  void  LCS_AddMulticast( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
    LCS_DoMulticast( SIOCADDMULTI, pLCSDEV, pCmdFrame, iCmdLen );
}

// ====================================================================
//                       LCS_DelMulticast
// ====================================================================

static  void  LCS_DelMulticast( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
    LCS_DoMulticast( SIOCDELMULTI, pLCSDEV, pCmdFrame, iCmdLen );
}

// ====================================================================
//                       LCS_DefaultCmdProc
// ====================================================================

static void  LCS_DefaultCmdProc( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{
    LCSSTDFRM   Reply;
    int         iReplyLen = sizeof(Reply);  /* Used and changed by INIT_REPLY_FRAME */
    PLCSSTDFRM  pLCSSTDFRM = (PLCSSTDFRM)&Reply;

    INIT_REPLY_FRAME( pLCSSTDFRM, iReplyLen, pCmdFrame, iCmdLen );

    pLCSSTDFRM->bLCSCmdHdr.bLanType      = LCS_FRMTYP_ENET;
    pLCSSTDFRM->bLCSCmdHdr.bRelAdapterNo = pLCSDEV->bPort;

    LCS_EnqueueReplyFrame( pLCSDEV, (PLCSCMDHDR)pLCSSTDFRM, iReplyLen );
}

// ====================================================================
//                       LCS_EnqueueReplyFrame
// ====================================================================
//
// Copy a pre-built LCS Command Frame reply frame into the
// next available frame slot. Keep trying if buffer is full.
// The LCS device data lock must NOT be held when called!
//
// --------------------------------------------------------------------

static void LCS_EnqueueReplyFrame( PLCSDEV pLCSDEV, PLCSCMDHDR pReply, size_t iSize )
{
    PLCSPORT  pLCSPORT;
    DEVBLK*   pDEVBLK;

    BYTE      bPort;
    time_t    t1, t2;


    bPort = pLCSDEV->bPort;
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[ bPort ];
    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];

    // Trace command reply frame about to be enqueued...
    if (pLCSDEV->pLCSBLK->fDebug)
    {
        // HHC00923 "%1d:%04X CTC: lcs command reply enqueue"
        WRMSG( HHC00923, "D", SSID_TO_LCSS( pDEVBLK->ssid ), pDEVBLK->devnum );
        net_data_trace( pDEVBLK, (BYTE*)pReply, iSize, TO_GUEST, 'D', "reply", 0 );
    }

    PTT_DEBUG( "ENQ RepFrame ENTRY", pReply->bCmdCode, pDEVBLK->devnum, bPort );

    time( &t1 );

    PTT_TIMING( "b4 repNQ", 0, iSize, 0 );

    // While port open, not close in progress, and frame buffer full...

    while (1
        &&  pLCSPORT->fd != -1
        && !pLCSPORT->fCloseInProgress
        && LCS_DoEnqueueReplyFrame( pLCSDEV, pReply, iSize ) < 0
    )
    {
        if (pLCSDEV->pLCSBLK->fDebug)
        {
            // Limit message rate to only once every few seconds...

            time( &t2 );

            if ((t2 - t1) >= 3)     // (only once every 3 seconds)
            {
                union converter { struct { unsigned char a, b, c, d; } b; U32 i; } c;
                char  str[40];

                t1 = t2;

                c.i = ntohl( pLCSDEV->lIPAddress );
                MSGBUF( str, "%8.08X %d.%d.%d.%d", c.i, c.b.d, c.b.c, c.b.b, c.b.a );

                // "CTC: lcs device port %2.2X: STILL trying to enqueue REPLY frame to device %4.4X %s"
                WRMSG( HHC00978, "D", bPort, pLCSDEV->sAddr, str );
            }
        }
        PTT_TIMING( "*repNQ wait", 0, iSize, 0 );

        // Wait for LCS_Read to empty the buffer...

        ASSERT( ENOBUFS == errno );
        USLEEP( CTC_DELAY_USECS );
    }
    PTT_TIMING( "af repNQ", 0, iSize, 0 );
    PTT_DEBUG( "ENQ RepFrame EXIT ", pReply->bCmdCode, pDEVBLK->devnum, bPort );
}

// ====================================================================
//                       LCS_DoEnqueueReplyFrame
// ====================================================================
//
// Copy a pre-built LCS Command Frame reply frame of iSize bytes
// to the next available frame slot. Returns 0 on success, -1 and
// errno set to ENOBUFS on failure (no room (yet) in o/p buffer).
// The LCS device data lock must NOT be held when called!
//
// --------------------------------------------------------------------

static int  LCS_DoEnqueueReplyFrame( PLCSDEV pLCSDEV, PLCSCMDHDR pReply, size_t iSize )
{
    PLCSCMDHDR  pReplyCmdFrame;
    DEVBLK*     pDEVBLK;
    BYTE        bPort = pLCSDEV->bPort;

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];

    PTT_DEBUG(       "GET  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
    obtain_lock( &pLCSDEV->DevDataLock );
    PTT_DEBUG(       "GOT  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
    {
        // Ensure we dont overflow the buffer
        if ((pLCSDEV->iFrameOffset +            // Current buffer Offset
              iSize +                           // Size of reply frame
              sizeof(pReply->bLCSHdr.hwOffset)) // Size of Frame terminator
            > pLCSDEV->iMaxFrameBufferSize)     // Size of Frame buffer
        {
            PTT_DEBUG( "*DoENQRep ENOBUFS ", 000, pDEVBLK->devnum, bPort );
            PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
            release_lock( &pLCSDEV->DevDataLock );
            errno = ENOBUFS;                    // No buffer space available
            return -1;                          // (-1==failure)
        }

        // Point to next available LCS Frame slot in our buffer...
        pReplyCmdFrame = (PLCSCMDHDR)( pLCSDEV->bFrameBuffer +
                                       pLCSDEV->iFrameOffset );

        // Copy the reply frame into the frame buffer slot...
        memcpy( pReplyCmdFrame, pReply, iSize );

        // Increment buffer offset to NEXT next-available-slot...
        pLCSDEV->iFrameOffset += (U16) iSize;

        // Store offset of next frame
        STORE_HW( pReplyCmdFrame->bLCSHdr.hwOffset, pLCSDEV->iFrameOffset );

        // Mark reply pending
        PTT_DEBUG( "SET  ReplyPending ", 1, pDEVBLK->devnum, bPort );
        pLCSDEV->fReplyPending = 1;
    }
    PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
    release_lock( &pLCSDEV->DevDataLock );

    // (wake up "LCS_Read" function)
    PTT_DEBUG(       "GET  DevEventLock ", 000, pDEVBLK->devnum, bPort );
    obtain_lock( &pLCSDEV->DevEventLock );
    PTT_DEBUG(       "GOT  DevEventLock ", 000, pDEVBLK->devnum, bPort );
    {
        PTT_DEBUG(            "SIG  DevEvent     ", 000, pDEVBLK->devnum, bPort );
        signal_condition( &pLCSDEV->DevEvent );
    }
    PTT_DEBUG(        "REL  DevEventLock ", 000, pDEVBLK->devnum, bPort );
    release_lock( &pLCSDEV->DevEventLock );

    return 0;   // success
}

// ====================================================================
//                       LCS_PortThread
// ====================================================================
// This is the thread that does the actual read from the tap device.
// It waits for packets to arrive on the device and then enqueues them
// to the device input queue to be read by the LCS_Read() function the
// next time the guest issues a read CCW.
// --------------------------------------------------------------------

static void*  LCS_PortThread( void* arg)
{
    DEVBLK*     pDEVBLK;
    PLCSPORT    pLCSPORT = (PLCSPORT) arg;
    PLCSDEV     pLCSDev;
    PLCSDEV     pPrimaryLCSDEV;
    PLCSDEV     pSecondaryLCSDEV;
    PLCSDEV     pMatchingLCSDEV;
    PLCSRTE     pLCSRTE;
    PETHFRM     pEthFrame;
    PIP4FRM     pIPFrame   = NULL;
    PARPFRM     pARPFrame  = NULL;
    int         iLength;
    U32         lIPAddress;             // (network byte order)
    BYTE*       pMAC;
    BYTE        szBuff[2048];
    char        bReported = 0;
    char        bStartReported = 0;
    char        cPktType[24];
    U16         hwEthernetType;
    BYTE        bHas8022;
    BYTE        bHas8022Snap;
    int         iTraceLen;
    MAC         mac;

    pDEVBLK = pLCSPORT->pLCSBLK->pDevices->pDEVBLK[ LCS_READ_SUBCHANN ];

    pLCSPORT->pid = getpid();

    PTT_DEBUG(            "PORTHRD: ENTRY    ", 000, pDEVBLK->devnum, pLCSPORT->bPort );

    for (;;)
    {
        PTT_DEBUG(        "GET  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
        obtain_lock( &pLCSPORT->PortEventLock );
        PTT_DEBUG(        "GOT  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
        {
            // Don't read unless/until port is enabled...

            if (!pLCSPORT->fPortStarted)
            {
                if (pLCSPORT->pLCSBLK->fDebug)
                {
                    if (bStartReported)
                        // "CTC: lcs device port %2.2X: read thread: port stopped"
                        WRMSG( HHC00969, "D", pLCSPORT->bPort );

                    // "CTC: lcs device port %2.2X: read thread: waiting for start event"
                    WRMSG( HHC00967, "D", pLCSPORT->bPort );
                }
                bStartReported = 0;
            }

            while (1)
            {
                PTT_DEBUG( "PORTHRD if started", pLCSPORT->fPortStarted, pDEVBLK->devnum, pLCSPORT->bPort );
                if (0
                    || (pLCSPORT->fd < 0)
                    || pLCSPORT->fCloseInProgress
                    || pLCSPORT->fPortStarted
                )
                {
                    if ((pLCSPORT->fd < 0) || pLCSPORT->fCloseInProgress)
                        PTT_DEBUG( "PORTHRD is closing", pLCSPORT->fPortStarted, pDEVBLK->devnum, pLCSPORT->bPort );
                    else
                        PTT_DEBUG( "PORTHRD is started", pLCSPORT->fPortStarted, pDEVBLK->devnum, pLCSPORT->bPort );
                    break;
                }

                PTT_DEBUG( "WAIT PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
                timed_wait_condition_relative_usecs
                (
                    &pLCSPORT->PortEvent,       // ptr to condition to wait on
                    &pLCSPORT->PortEventLock,   // ptr to controlling lock (must be held!)
                    250*1000,                   // max #of microseconds to wait
                    NULL                        // [OPTIONAL] ptr to tod value (may be NULL)
                );
                PTT_DEBUG( "WOKE PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );

            } // end while (1)

            if (!bStartReported)
            {
                bStartReported = 1;
                if (pLCSPORT->pLCSBLK->fDebug)
                    // "CTC: lcs device port %2.2X: read thread: port started"
                    WRMSG( HHC00968, "D", pLCSPORT->bPort );
            }
        }
        PTT_DEBUG(         "REL  PortEventLock", 000, pDEVBLK->devnum, pLCSPORT->bPort );
        release_lock( &pLCSPORT->PortEventLock );

        // Exit when told...

        if ( pLCSPORT->fd < 0 || pLCSPORT->fCloseInProgress )
            break;

        // Read an IP packet from the TAP device
        PTT_TIMING( "b4 tt read", 0, 0, 0 );
        iLength = read_tuntap( pLCSPORT->fd, szBuff, sizeof( szBuff ), DEF_NET_READ_TIMEOUT_SECS );
        PTT_TIMING( "af tt read", 0, 0, iLength );

        if (iLength == 0)      // (probably EINTR; ignore)
            continue;

        // Check for other error condition
        if (iLength < 0)
        {
            if (pLCSPORT->fd < 0 || pLCSPORT->fCloseInProgress)
                break;
            // "CTC: lcs interface %s read error from port %2.2X: %s"
            WRMSG( HHC00944, "E", pLCSPORT->szNetIfName, pLCSPORT->bPort, strerror( errno ) );
            break;
        }

        // Point to ethernet frame and determine frame type
        pEthFrame = (PETHFRM)szBuff;

        GetFrameInfo( pEthFrame, &cPktType[0], &hwEthernetType, &bHas8022, &bHas8022Snap );

        if (pLCSPORT->pLCSBLK->fDebug)
        {
            // "%1d:%04X %s: port %2.2X: Receive frame of size %d bytes (with %s packet) from device %s"
            WRMSG( HHC00984, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                  pLCSPORT->bPort, iLength, cPktType, pLCSPORT->szNetIfName );
//!!            if (pLCSPORT->pLCSBLK->iTraceLen)
//!!            {
//!!                iTraceLen = iLength;
//!!                if (iTraceLen > pLCSPORT->pLCSBLK->iTraceLen)
//!!                {
//!!                    iTraceLen = pLCSPORT->pLCSBLK->iTraceLen;
//!!                    // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
//!!                    WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
//!!                                         iTraceLen, (iLength - iTraceLen) );
//!!                }
//!!                net_data_trace( pDEVBLK, szBuff, iTraceLen, TO_GUEST, 'D', "eth frame", 0 );
//!!            }
            bReported = 0;
        }

        // Perform multicast assist if necessary: discard any multicast
        // packets the guest didn't specifically register. We only need
        // to do this if tuntap said that it was unable to do so for us.

        if (1
            && pLCSPORT->fDoMCastAssist                                     // do mcast filtering ourself?
            && pLCSPORT->nMCastCount                                        // we have MACs in our table?
            && memcmp( pEthFrame->bDestMAC, mcast3, sizeof( mcast3 )) == 0  // this is a multicast frame?
            && IsMACTab( pLCSPORT->MCastTab, pEthFrame->bDestMAC ) < 0      // its MAC not in our table?
        )
        {
            if (pLCSPORT->pLCSBLK->fDebug)
                // "CTC: lcs device port %2.2X: MCAST not in table, discarding frame"
                WRMSG( HHC00945, "D", pLCSPORT->bPort );
            continue;
        }

        // Housekeeping
        pPrimaryLCSDEV   = NULL;
        pSecondaryLCSDEV = NULL;
        pMatchingLCSDEV  = NULL;

        // Attempt to find the device that this frame belongs to
        for (pLCSDev = pLCSPORT->pLCSBLK->pDevices; pLCSDev; pLCSDev = pLCSDev->pNext)
        {
            // Only process devices that are on this port
            if (pLCSDev->bPort == pLCSPORT->bPort)
            {
                // hwEthernetType indicates which protocol is encapsulated in the payload.
                // bHas8022 indicates whether the payload begins with an 802.2 LLC.
                // bHas8022Snap indicates whether the payload begins with an 802.2 LLC and SNAP.
                if (hwEthernetType == ETH_TYPE_IP)
                {
                    if (!bHas8022Snap)
                        pIPFrame   = (PIP4FRM)pEthFrame->bData;
                    else
                        pIPFrame   = (PIP4FRM)pEthFrame->bData+ETH_LLC_SNAP_SIZE;
                    lIPAddress = pIPFrame->lDstIP;  // (network byte order)

                    if (pLCSPORT->pLCSBLK->fDebug && !bReported)
                    {
                        union converter { struct { unsigned char a, b, c, d; } b; U32 i; } c;
                        char  str[40];

                        c.i = ntohl(lIPAddress);
                        MSGBUF( str, "%8.08X %d.%d.%d.%d", c.i, c.b.d, c.b.c, c.b.b, c.b.a );

                        // "CTC: lcs device port %2.2X: IPv4 frame received for %s"
                        WRMSG( HHC00946, "D", pLCSPORT->bPort, str );
                        bReported = 1;
                    }

                    // If this is an exact match use it
                    // otherwise look for primary and secondary
                    // default devices
                    if (pLCSDev->lIPAddress == lIPAddress)
                    {
                        pMatchingLCSDEV = pLCSDev;
                        break;
                    }
                    else if (pLCSDev->bType == LCSDEV_TYPE_PRIMARY)
                        pPrimaryLCSDEV = pLCSDev;
                    else if (pLCSDev->bType == LCSDEV_TYPE_SECONDARY)
                        pSecondaryLCSDEV = pLCSDev;
                }
                else if (hwEthernetType == ETH_TYPE_ARP)
                {
                    if (!bHas8022Snap)
                        pARPFrame  = (PARPFRM)pEthFrame->bData;
                    else
                        pARPFrame  = (PARPFRM)pEthFrame->bData+ETH_LLC_SNAP_SIZE;
                    lIPAddress = pARPFrame->lTargIPAddr; // (network byte order)

                    if (pLCSPORT->pLCSBLK->fDebug && !bReported)
                    {
                        union converter { struct { unsigned char a, b, c, d; } b; U32 i; } c;
                        char  str[40];

                        c.i = ntohl(lIPAddress);
                        MSGBUF( str, "%8.08X %d.%d.%d.%d", c.i, c.b.d, c.b.c, c.b.b, c.b.a );

                        // "CTC: lcs device port %2.2X: ARP frame received for %s"
                        WRMSG( HHC00947, "D", pLCSPORT->bPort, str );
                        bReported = 1;
                    }

                    // If this is an exact match use it
                    // otherwise look for primary and secondary
                    // default devices
                    if (pLCSDev->lIPAddress == lIPAddress)
                    {
                        pMatchingLCSDEV = pLCSDev;
                        break;
                    }
                    else if (pLCSDev->bType == LCSDEV_TYPE_PRIMARY)
                        pPrimaryLCSDEV = pLCSDev;
                    else if (pLCSDev->bType == LCSDEV_TYPE_SECONDARY)
                        pSecondaryLCSDEV = pLCSDev;
                }
                else if (hwEthernetType == ETH_TYPE_RARP)
                {
                    if (!bHas8022Snap)
                        pARPFrame  = (PARPFRM)pEthFrame->bData;
                    else
                        pARPFrame  = (PARPFRM)pEthFrame->bData+ETH_LLC_SNAP_SIZE;
                    pMAC = pARPFrame->bTargEthAddr;

                    if (pLCSPORT->pLCSBLK->fDebug && !bReported)
                    {
                        // "CTC: lcs device port %2.2X: RARP frame received for %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
                        WRMSG( HHC00948, "D" ,pLCSPORT->bPort ,*(pMAC+0) ,*(pMAC+1) ,*(pMAC+2) ,*(pMAC+3) ,*(pMAC+4) ,*(pMAC+5) );
                        bReported = 1;
                    }

                    // If this is an exact match use it
                    // otherwise look for primary and secondary
                    // default devices
                    memcpy( &mac, pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
                    mac[5]++;
#endif
                    if (memcmp( pMAC, &mac, IFHWADDRLEN ) == 0)
                    {
                        pMatchingLCSDEV = pLCSDev;
                        break;
                    }
                    else if (pLCSDev->bType == LCSDEV_TYPE_PRIMARY)
                        pPrimaryLCSDEV = pLCSDev;
                    else if (pLCSDev->bType == LCSDEV_TYPE_SECONDARY)
                        pSecondaryLCSDEV = pLCSDev;
                }
                else if (hwEthernetType == ETH_TYPE_SNA)
                {
                    pMAC = pEthFrame->bDestMAC;

                    if (pLCSPORT->pLCSBLK->fDebug && !bReported)
                    {
                        // "CTC: lcs device port %2.2X: SNA frame received for %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
                        WRMSG( HHC00949, "D" ,pLCSPORT->bPort ,*(pMAC+0) ,*(pMAC+1) ,*(pMAC+2) ,*(pMAC+3) ,*(pMAC+4) ,*(pMAC+5) );
                        bReported = 1;
                    }

                    // If this is an exact match use it
                    // Primary and secondary default devicea are IP only
                    memcpy( &mac, pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
                    mac[5]++;
#endif
                    if (memcmp( pMAC, &mac, IFHWADDRLEN ) == 0)
                    {
                        pMatchingLCSDEV = pLCSDev;
                        break;
                    }
                }
            }
        }

        // If the matching device is not started
        // nullify the pointer and pass frame to one
        // of the defaults if present
        if (pMatchingLCSDEV && !pMatchingLCSDEV->fDevStarted)
            pMatchingLCSDEV = NULL;

        // Match not found, check for default devices
        // If one is defined and started, use it
        if (!pMatchingLCSDEV)
        {
            if (pPrimaryLCSDEV && pPrimaryLCSDEV->fDevStarted)
            {
                pMatchingLCSDEV = pPrimaryLCSDEV;

#ifndef LCS_NO_950_952 // (HHC00950 and HHC00952 are rarely interesting)
                if (pLCSPORT->pLCSBLK->fDebug)
                    // "CTC: lcs device port %2.2X: no match found, selecting %s %4.4X"
                    WRMSG( HHC00950, "D", pLCSPORT->bPort, "primary", pMatchingLCSDEV->sAddr );
#endif // LCS_NO_950_952
            }
            else if (pSecondaryLCSDEV && pSecondaryLCSDEV->fDevStarted)
            {
                pMatchingLCSDEV = pSecondaryLCSDEV;

#ifndef LCS_NO_950_952 // (HHC00950 and HHC00952 are rarely interesting)
                if (pLCSPORT->pLCSBLK->fDebug)
                    // "CTC: lcs device port %2.2X: no match found, selecting %s %4.4X"
                    WRMSG( HHC00950, "D", pLCSPORT->bPort, "secondary", pMatchingLCSDEV->sAddr );
#endif // LCS_NO_950_952
            }
        }

        // Discard frame if no matching device was found.
        if (!pMatchingLCSDEV)
        {
            if (pLCSPORT->pLCSBLK->fDebug)
            {
                // "CTC: lcs device port %2.2X: no match found, discarding frame"
                WRMSG( HHC00951, "D", pLCSPORT->bPort );
                if (pLCSPORT->pLCSBLK->iDiscTrace)
                {
                    iTraceLen = iLength;
                    if (iTraceLen > pLCSPORT->pLCSBLK->iDiscTrace)
                    {
                        iTraceLen = pLCSPORT->pLCSBLK->iDiscTrace;
                    }
                    net_data_trace( pDEVBLK, szBuff, iTraceLen, NO_DIRECTION, 'D', "discarding", 0 );
                }
            }
            continue;
        }

        // Discard frame if the SNA device isn't receiving frames, or
        // the frames payload does not begin with an 802.2 LLC, or
        // the frames payload begins with an 802.2 LLC and SNAP.
        if (pMatchingLCSDEV->bMode == LCSDEV_MODE_SNA)
        {
            if (!pMatchingLCSDEV->fReceiveFrames || !bHas8022 || bHas8022Snap )
            {
                if (pLCSPORT->pLCSBLK->fDebug)
                {
                    // "CTC: lcs device port %2.2X: no match found, discarding frame"
                    WRMSG( HHC00951, "D", pLCSPORT->bPort );
                    if (pLCSPORT->pLCSBLK->iDiscTrace)
                    {
                        iTraceLen = iLength;
                        if (iTraceLen > pLCSPORT->pLCSBLK->iDiscTrace)
                        {
                            iTraceLen = pLCSPORT->pLCSBLK->iDiscTrace;
                        }
                        net_data_trace( pDEVBLK, szBuff, iTraceLen, NO_DIRECTION, 'D', "discarding", 0 );
                    }
                }
                continue;
            }
        }

#ifndef LCS_NO_950_952 // (HHC00950 and HHC00952 are rarely interesting)
        if (pLCSPORT->pLCSBLK->fDebug)
        {
            union converter { struct { unsigned char a, b, c, d; } b; U32 i; } c;
            char  str[40];

            c.i = ntohl(pMatchingLCSDEV->lIPAddress);
            MSGBUF( str, "%8.08X %d.%d.%d.%d", c.i, c.b.d, c.b.c, c.b.b, c.b.a );

            // "CTC: lcs device port %2.2X: enqueing frame to device %4.4X %s"
            WRMSG( HHC00952, "D", pLCSPORT->bPort, pMatchingLCSDEV->sAddr, str );
        }
#endif // LCS_NO_950_952

        if (pLCSPORT->pLCSBLK->fDebug)
        {
            if (pLCSPORT->pLCSBLK->iTraceLen)
            {
                iTraceLen = iLength;
                if (iTraceLen > pLCSPORT->pLCSBLK->iTraceLen)
                {
                    iTraceLen = pLCSPORT->pLCSBLK->iTraceLen;
                    // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                    WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                         iTraceLen, (iLength - iTraceLen) );
                }
                net_data_trace( pDEVBLK, szBuff, iTraceLen, TO_GUEST, 'D', "eth frame", 0 );
            }
        }

        // Match was found. Enqueue frame on buffer.

        if (pMatchingLCSDEV->bMode == LCSDEV_MODE_IP)
        {
            LCS_EnqueueEthFrame( pLCSPORT, pMatchingLCSDEV, szBuff, iLength );
        }
        else  //  (pMatchingLCSDEV->bMode == LCSDEV_MODE_SNA)
        {
            LCS_ProcessAccepted_SNA ( pLCSPORT, pMatchingLCSDEV, szBuff, iLength );
        }

    } // end for (;;)

    PTT_DEBUG( "PORTHRD Closing...", pLCSPORT->fPortStarted, pDEVBLK->devnum, pLCSPORT->bPort );

    // We must do the close since we were the one doing the i/o...

    VERIFY( pLCSPORT->fd == -1 || TUNTAP_Close( pLCSPORT->fd ) == 0 );

    // Housekeeping - Cleanup Port Block

    memset( pLCSPORT->MAC_Address,  0, IFHWADDRLEN );
    memset( pLCSPORT->szNetIfName, 0, IFNAMSIZ );
    memset( pLCSPORT->szMACAddress, 0, 32 );

    for (pLCSRTE = pLCSPORT->pRoutes; pLCSRTE; pLCSRTE = pLCSPORT->pRoutes)
    {
        pLCSPORT->pRoutes = pLCSRTE->pNext;
        free( pLCSRTE );
        pLCSRTE = NULL;
    }

    pLCSPORT->sIPAssistsSupported = 0;  // (reset)
    pLCSPORT->sIPAssistsEnabled   = 0;  // (reset)
    pLCSPORT->fDoCkSumOffload     = 0;  // (reset)
    pLCSPORT->fDoMCastAssist      = 0;  // (reset)

    pLCSPORT->fUsed        = 0;
    pLCSPORT->fLocalMAC    = 0;
    pLCSPORT->fPortCreated = 0;
    PTT_DEBUG( "PORTHRD started=NO", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    pLCSPORT->fPortStarted = 0;
    pLCSPORT->fRouteAdded  = 0;
    pLCSPORT->fd           = -1;

    PTT_DEBUG( "PORTHRD: EXIT     ", 000, pDEVBLK->devnum, pLCSPORT->bPort );

    return NULL;

}   // End of LCS_PortThread

// ====================================================================
//                       LCS_EnqueueEthFrame
// ====================================================================
//
// Places the provided ethernet frame in the next available frame
// slot in the adapter buffer. If buffer is full, keep trying.
// The LCS device data lock must NOT be held when called!
//
// --------------------------------------------------------------------

static void LCS_EnqueueEthFrame( PLCSPORT pLCSPORT, PLCSDEV pLCSDEV, BYTE* pData, size_t iSize )
{
    DEVBLK*   pDEVBLK;
    BYTE      bPort;
    time_t    t1, t2;


    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    bPort   = pLCSPORT->bPort;

    PTT_DEBUG( "ENQ EthFrame ENTRY", 000, pDEVBLK->devnum, bPort );

    time( &t1 );

    PTT_TIMING( "b4 enqueue", 0, iSize, 0 );

    // While port open, not close in progress, and frame buffer full...

    while (1
        &&  pLCSPORT->fd != -1
        && !pLCSPORT->fCloseInProgress
        && LCS_DoEnqueueEthFrame( pLCSPORT, pLCSDEV, pData, iSize ) < 0
    )
    {
        if (EMSGSIZE == errno)
        {
            // "CTC: lcs device port %2.2X: packet frame too big, dropped"
            WRMSG( HHC00953, "W", bPort );
            PTT_TIMING( "*enq drop", 0, iSize, 0 );
            break;
        }

        if (pLCSDEV->pLCSBLK->fDebug)
        {
            // Limit message rate to only once every few seconds...

            time( &t2 );

            if ((t2 - t1) >= 3)     // (only once every 3 seconds)
            {
                union converter { struct { unsigned char a, b, c, d; } b; U32 i; } c;
                char  str[40];

                t1 = t2;

                c.i = ntohl( pLCSDEV->lIPAddress );
                MSGBUF( str, "%8.08X %d.%d.%d.%d", c.i, c.b.d, c.b.c, c.b.b, c.b.a );

                // "CTC: lcs device port %2.2X: STILL trying to enqueue frame to device %4.4X %s"
                WRMSG( HHC00965, "D", bPort, pLCSDEV->sAddr, str );
            }
        }
        PTT_TIMING( "*enq wait", 0, iSize, 0 );

        // Wait for LCS_Read to empty the buffer...

        ASSERT( ENOBUFS == errno );
        USLEEP( CTC_DELAY_USECS );
    }
    PTT_TIMING( "af enqueue", 0, iSize, 0 );
    PTT_DEBUG( "ENQ EthFrame EXIT ", 000, pDEVBLK->devnum, bPort );
}

// ====================================================================
//                       LCS_DoEnqueueEthFrame
// ====================================================================
//
// Places the provided ethernet frame in the next available frame
// slot in the adapter buffer.
//
//   pData       points the the Ethernet packet just received
//   iSize       is the size of the Ethernet packet
//
// Returns:
//
//  0 == Success
// -1 == Failure; errno = ENOBUFS:  No buffer space available
//                        EMSGSIZE: Message too long
//
// The LCS device data lock must NOT be held when called!
//
// --------------------------------------------------------------------

static int  LCS_DoEnqueueEthFrame( PLCSPORT pLCSPORT, PLCSDEV pLCSDEV, BYTE* pData, size_t iSize )
{
    PLCSETHFRM  pLCSEthFrame;
    DEVBLK*     pDEVBLK;
    BYTE        bPort;


    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    bPort   = pLCSPORT->bPort;

    // Will frame NEVER fit into buffer??
    if (iSize > MAX_LCS_ETH_FRAME_SIZE( pLCSDEV ) || iSize > 9000)
    {
        PTT_DEBUG( "*DoENQEth EMSGSIZE", 000, pDEVBLK->devnum, bPort );
        errno = EMSGSIZE;   // Message too long
        return -1;          // (-1==failure)
    }

    PTT_DEBUG(       "GET  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
    obtain_lock( &pLCSDEV->DevDataLock );
    PTT_DEBUG(       "GOT  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
    {
        // Ensure we dont overflow the buffer
        if (( pLCSDEV->iFrameOffset +                   // Current buffer Offset
              sizeof(LCSETHFRM) +                       // Size of Frame Header
              iSize +                                   // Size of Ethernet packet
              sizeof(pLCSEthFrame->bLCSHdr.hwOffset) )  // Size of Frame terminator
            > pLCSDEV->iMaxFrameBufferSize)             // Size of Frame buffer
        {
            PTT_DEBUG( "*DoENQEth ENOBUFS ", 000, pDEVBLK->devnum, bPort );
            PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
            release_lock( &pLCSDEV->DevDataLock );
            errno = ENOBUFS;    // No buffer space available
            return -1;          // (-1==failure)
        }

        // Point to next available LCS Frame slot in our buffer
        pLCSEthFrame = (PLCSETHFRM)( pLCSDEV->bFrameBuffer +
                                     pLCSDEV->iFrameOffset );

        // Increment offset to NEXT available slot (after ours)
        pLCSDEV->iFrameOffset += (U16)(sizeof(LCSETHFRM) + iSize);

        // Plug updated offset to next frame into our frame header
        STORE_HW( pLCSEthFrame->bLCSHdr.hwOffset, pLCSDEV->iFrameOffset );

        // Finish building the LCS Ethernet Passthru frame header
        pLCSEthFrame->bLCSHdr.bType = LCS_FRMTYP_ENET;
        pLCSEthFrame->bLCSHdr.bSlot = bPort;

        // Copy Ethernet packet to LCS Ethernet Passthru frame
        memcpy( pLCSEthFrame->bData, pData, iSize );

        // Tell "LCS_Read" function that data is available for reading
        PTT_DEBUG( "SET  DataPending  ", 1, pDEVBLK->devnum, bPort );
        pLCSDEV->fDataPending = 1;
    }
    PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, bPort );
    release_lock( &pLCSDEV->DevDataLock );

    // (wake up "LCS_Read" function)
    PTT_DEBUG(       "GET  DevEventLock ", 000, pDEVBLK->devnum, bPort );
    obtain_lock( &pLCSDEV->DevEventLock );
    PTT_DEBUG(       "GOT  DevEventLock ", 000, pDEVBLK->devnum, bPort );
    {
        PTT_DEBUG(            "SIG  DevEvent     ", 000, pDEVBLK->devnum, bPort );
        signal_condition( &pLCSDEV->DevEvent );
    }
    PTT_DEBUG(        "REL  DevEventLock ", 000, pDEVBLK->devnum, bPort );
    release_lock( &pLCSDEV->DevEventLock );

    return 0;       // (success)
}

// ====================================================================
//                     lcs_halt_or_clear
// ====================================================================
// The channel is processing a Halt Subchannel or Clear Subchannel
// instruction and is notifying us of that fact so we can stop our
// LCS_Read CCW processing loop.
// --------------------------------------------------------------------

static void lcs_halt_or_clear( DEVBLK* pDEVBLK )
{
    PLCSDEV pLCSDEV = (PLCSDEV) pDEVBLK->dev_data;
    obtain_lock( &pLCSDEV->DevEventLock );
    {
        if (pLCSDEV->fReadWaiting)
        {
            pLCSDEV->fHaltOrClear = 1;
            signal_condition( &pLCSDEV->DevEvent );
        }
    }
    release_lock( &pLCSDEV->DevEventLock );
}

// ====================================================================
//                         LCS_Read
// ====================================================================
// The guest o/s is issuing a Read CCW for our LCS device. Return to
// it all available LCS Frames that we have buffered up in our buffer.
// --------------------------------------------------------------------

void  LCS_Read( DEVBLK* pDEVBLK,   U32   sCount,
                BYTE*   pIOBuf,    BYTE* pUnitStat,
                U32*    pResidual, BYTE* pMore )
{
    PLCSHDR     pLCSHdr;
    PLCSDEV     pLCSDEV = (PLCSDEV)pDEVBLK->dev_data;
    size_t      iLength = 0;
    int         iTraceLen;

    struct timespec  waittime;
    struct timeval   now;

    // FIXME: we currently don't support data-chaining but
    // probably should if real LCS devices do (I was unable
    // to determine whether they do or not). -- Fish

    PTT_DEBUG( "READ: ENTRY       ", 000, pDEVBLK->devnum, -1 );

    for (;;)
    {
        // Has anything arrived in our frame buffer yet?

        PTT_DEBUG(       "GET  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
        obtain_lock( &pLCSDEV->DevDataLock );
        PTT_DEBUG(       "GOT  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
        {
            if (pLCSDEV->fDataPending || pLCSDEV->fReplyPending)
                break;
        }
        PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
        release_lock( &pLCSDEV->DevDataLock );

        // Keep waiting for LCS Frames to arrive in our frame buffer...

        gettimeofday( &now, NULL );

        waittime.tv_sec  = now.tv_sec  + DEF_NET_READ_TIMEOUT_SECS;
        waittime.tv_nsec = now.tv_usec * 1000;

        PTT_DEBUG(       "GET  DevEventLock ", 000, pDEVBLK->devnum, -1 );
        obtain_lock( &pLCSDEV->DevEventLock );
        PTT_DEBUG(       "GOT  DevEventLock ", 000, pDEVBLK->devnum, -1 );
        {
            PTT_DEBUG( "WAIT DevEventLock ", 000, pDEVBLK->devnum, -1 );
            pLCSDEV->fReadWaiting = 1;
            timed_wait_condition( &pLCSDEV->DevEvent,
                                  &pLCSDEV->DevEventLock,
                                  &waittime );
            pLCSDEV->fReadWaiting = 0;
        }

        PTT_DEBUG(        "WOKE DevEventLock ", 000, pDEVBLK->devnum, -1 );

        // Check for channel conditions...

        if (pLCSDEV->fHaltOrClear)
        {
            *pUnitStat = 0;
            *pResidual = sCount;
            pLCSDEV->fHaltOrClear=0;

            PTT_DEBUG(    "*HALT or CLEAR*   ", *pUnitStat, pDEVBLK->devnum, sCount );

            if (pDEVBLK->ccwtrace || pLCSDEV->pLCSBLK->fDebug)
                // "%1d:%04X %s: halt or clear recognized"
                WRMSG( HHC00904, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname );
            release_lock( &pLCSDEV->DevEventLock );
            return;
        }
        release_lock( &pLCSDEV->DevEventLock );

    } // end for (;;)

    // We have frame buffer data to return to the guest...
    // (i.e. Ethernet packets or cmd reply frames)

    PTT_DEBUG( "READ using buffer ", 000, pDEVBLK->devnum, -1 );

    // Point to the end of all buffered LCS Frames...
    // (where the next Frame *would* go if there was one)

    pLCSHdr = (PLCSHDR)( pLCSDEV->bFrameBuffer +
                         pLCSDEV->iFrameOffset );

    // Mark the end of this batch of LCS Frames by setting
    // the "offset to NEXT frame" LCS Header field to zero.
    // (a zero "next Frame offset" is like an "EOF" flag)

    STORE_HW( pLCSHdr->hwOffset, 0x0000 );

    // Calculate how much data we're going to be giving them.

    // Since 'iFrameOffset' points to the next available LCS
    // Frame slot in our buffer, the total amount of LCS Frame
    // data we have is exactly that amount. We give them two
    // extra bytes however so that they can optionally chase
    // the "hwOffset" field in each LCS Frame's LCS Header to
    // eventually reach our zero hwOffset "EOF" flag).

    iLength = pLCSDEV->iFrameOffset + sizeof(pLCSHdr->hwOffset);

    // (calculate residual and set memcpy amount)

    // FIXME: we currently don't support data-chaining but
    // probably should if real LCS devices do (I was unable
    // to determine whether they do or not). -- Fish

    if (sCount < iLength)
    {
        *pMore     = 1;
        *pResidual = 0;

        iLength = sCount;

        // PROGRAMMING NOTE: As a result of the caller asking
        // for less data than we actually have available, the
        // remainder of their unread data they didn't ask for
        // will end up being silently discarded. Refer to the
        // other NOTEs and FIXME's sprinkled throughout this
        // function...
    }
    else
    {
        *pMore      = 0;
        *pResidual -= iLength;
    }

    *pUnitStat = CSW_CE | CSW_DE;

    memcpy( pIOBuf, pLCSDEV->bFrameBuffer, iLength );

    // Display up to pLCSBLK->iTraceLen bytes of the data going to the guest, if debug is active
    if (pLCSDEV->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: Present data of size %d bytes to guest"
        WRMSG(HHC00982, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, (int)iLength );
        if (pLCSDEV->pLCSBLK->iTraceLen)
        {
            iTraceLen = iLength;
            if (iTraceLen > pLCSDEV->pLCSBLK->iTraceLen)
            {
                iTraceLen = pLCSDEV->pLCSBLK->iTraceLen;
                // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, (int)(iLength - iTraceLen) );
            }
            net_data_trace( pDEVBLK, pIOBuf, iTraceLen, TO_GUEST, 'D', "data", 0 );
        }
    }

    // Reset frame buffer to empty...

    // PROGRAMMING NOTE: even though not all available data
    // may have been read by the guest, we don't currently
    // support data-chaining. Thus any unread data is always
    // discarded by resetting all of the iFrameOffset,
    // fDataPending and fReplyPending fields to 0 so that the
    // next read always grabs a new batch of LCS Frames starting
    // at the very beginning of our frame buffer again. (I was
    // unable to determine whether real LCS devices support
    // data-chaining or not, but if they do we should fix this).

    PTT_DEBUG( "READ empty buffer ", 000, pDEVBLK->devnum, -1 );
    pLCSDEV->iFrameOffset  = 0;
    pLCSDEV->fReplyPending = 0;
    pLCSDEV->fDataPending  = 0;

    PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
    release_lock( &pLCSDEV->DevDataLock );

    PTT_DEBUG( "READ: EXIT        ", 000, pDEVBLK->devnum, -1 );
}

// ====================================================================
//                         GetFrameInfo
// ====================================================================
//  To quote Wikipedia "The EtherType field is two octets long and it
//  can be used for two different purposes. Values of 1500 and below
//  mean that it is used to indicate the size of the payload in
//  octets, while values of 1536 and above indicate that it is used as
//  an EtherType, to indicate which protocol is encapsulated in the
//  payload of the frame."
//
//  When the EtherType indicates the size of the payload the frame is
//  assumed to be IEEE 802.3 layout, and the EtherType should be
//  followed by a Logical Link Control (LLC) sub-layer as defined in
//  the IEEE 802.2 standards.
//
//  The LLC will contain at least a 1-byte Destination Service Access
//  Point (DSAP), a 1-byte Source Service Access Point (SSAP), and a
//  1- or 2-byte Control. If the LLC is used in conjunction with the
//  Sub-Network Access Protocol (SNAP), the LLC will contain a
//  1-byte DSAP, a 1-byte SSAP, a 1-byte Control, a 3-byte IEEE
//  Organizationally Unique Identifier (OUI), and a 2-byte protocol
//  ID.
//
//  The cPktType area should be 24 or more characters in length.
//

void  GetFrameInfo( PETHFRM pEthFrame, char* pPktType, U16* pEthType, BYTE* pHas8022, BYTE* pHas8022Snap )
{
    char  pkttyp[24];
    U32   oui;
    U16   ethtyp;
    BYTE  ieee, snap, dsap, ssap, ctl;

    memset(pkttyp, 0, sizeof(pkttyp));
    FETCH_HW( ethtyp, pEthFrame->hwEthernetType );
    ieee = FALSE;
    snap = FALSE;
    if (ethtyp >= ETH_TYPE)
    {
             if (ethtyp == ETH_TYPE_IP   )    STRLCPY( pkttyp, "IPv4"    );
        else if (ethtyp == ETH_TYPE_IPV6 )    STRLCPY( pkttyp, "IPv6"    );
        else if (ethtyp == ETH_TYPE_ARP  )    STRLCPY( pkttyp, "ARP"     );
        else if (ethtyp == ETH_TYPE_RARP )    STRLCPY( pkttyp, "RARP"    );
        else if (ethtyp == ETH_TYPE_SNA  )    STRLCPY( pkttyp, "SNA"     );
        else                                  STRLCPY( pkttyp, "unknown" );
    }
    else
    {
        ieee = TRUE;
                                              STRLCPY( pkttyp, "802.3 "  );
        dsap = (pEthFrame->bData[0] & 0xFE);
        ssap = (pEthFrame->bData[1] & 0xFE);
        ctl =   pEthFrame->bData[2];
        FETCH_F3( oui, pEthFrame->bData+3 );
        if ( dsap == LSAP_SNAP  &&  ssap == LSAP_SNAP  &&  ctl == 0x03  &&  !oui )
        {
          snap = TRUE;
                                              STRLCAT( pkttyp, "SNAP "  );
          FETCH_HW(ethtyp, pEthFrame->bData+6 );
               if (ethtyp == ETH_TYPE_IP   )  STRLCAT( pkttyp, "IPv4"    );
          else if (ethtyp == ETH_TYPE_IPV6 )  STRLCAT( pkttyp, "IPv6"    );
          else if (ethtyp == ETH_TYPE_ARP  )  STRLCAT( pkttyp, "ARP"     );
          else if (ethtyp == ETH_TYPE_RARP )  STRLCAT( pkttyp, "RARP"    );
          else if (ethtyp == ETH_TYPE_SNA  )  STRLCAT( pkttyp, "SNA"     );
          else                                STRLCAT( pkttyp, "unknown" );
        }
        else if ( dsap == LSAP_SNA_Path_Control || ssap == LSAP_SNA_Path_Control )
        {
          ethtyp = ETH_TYPE_SNA;
                                              STRLCAT( pkttyp, "SNA"     );
        }
        else
        {
                                              STRLCAT( pkttyp, "unknown" );
        }
    }
    memcpy( pPktType, pkttyp, (strlen(pkttyp)+1) );
    *pEthType = ethtyp;
    *pHas8022 = ieee;
    *pHas8022Snap = snap;
}

// ====================================================================
//                         GetIfMACAddress
// ====================================================================

void    GetIfMACAddress( PLCSPORT pLCSPORT )
{
    BYTE*  pPortMAC   = (BYTE*) &pLCSPORT->MAC_Address;
    BYTE*  pIFaceMAC  = pPortMAC;

    /* Not all systems can return the hardware address of an interface. */

#if defined( SIOCGIFHWADDR )

    ifreq  ifr;
    {
        int    fd, rc;

        fd = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );

        if (fd == -1)
        {
            // "CTC: error in function %s: %s"
            rc = HSO_errno;
            WRMSG( HHC00940, "E", "socket()", strerror( rc ));
            return;
        }

        memset( &ifr, 0, sizeof( ifr ));
        STRLCPY( ifr.ifr_name, pLCSPORT->szNetIfName );

        rc = TUNTAP_IOCtl( fd, SIOCGIFHWADDR, (char*) &ifr );
        close( fd );

        if (rc != 0)
        {
            // "CTC: ioctl %s failed for device %s: %s; ... ignoring and continuing"
            rc = HSO_errno;
            WRMSG( HHC00941, "W", "SIOCGIFHWADDR", pLCSPORT->szNetIfName, strerror( rc ));
            return;
        }

        pIFaceMAC  = (BYTE*) ifr.ifr_hwaddr.sa_data;
    }

#endif // defined( SIOCGIFHWADDR )

    /* Report what MAC address we will really be using */
    // "CTC: lcs interface '%s' using mac %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
    WRMSG( HHC00942, "I", pLCSPORT->szNetIfName, *(pIFaceMAC+0), *(pIFaceMAC+1),
                      *(pIFaceMAC+2), *(pIFaceMAC+3), *(pIFaceMAC+4), *(pIFaceMAC+5));

    /* Issue warning if different from specified value */
    if (memcmp( pPortMAC, pIFaceMAC, IFHWADDRLEN ) != 0)
    {
        if (pLCSPORT->fLocalMAC)
        {
            // "CTC: lcs interface %s not using mac %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
            WRMSG( HHC00943, "W", pLCSPORT->szNetIfName, *(pPortMAC+0), *(pPortMAC+1),
                             *(pPortMAC+2), *(pPortMAC+3), *(pPortMAC+4), *(pPortMAC+5));
        }

        memcpy( pPortMAC, pIFaceMAC, IFHWADDRLEN );    // Sets pLCSPORT->MAC_Address

        snprintf(pLCSPORT->szMACAddress, sizeof(pLCSPORT->szMACAddress),
            "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", *(pPortMAC+0), *(pPortMAC+1),
            *(pPortMAC+2), *(pPortMAC+3), *(pPortMAC+4), *(pPortMAC+5));
    }
}

// ====================================================================
//                         ParseArgs
// ====================================================================

int  ParseArgs( DEVBLK* pDEVBLK, PLCSBLK pLCSBLK,
                int argc, char** argx )
{
    struct in_addr  addr;               // Work area for addresses
    MAC             mac;
    int             i;
    int             iDiscTrace;
    int             iTraceLen;
#if defined(OPTION_W32_CTCI)
    int             iKernBuff;
    int             iIOBuff;
#endif
    char            *argn[MAX_ARGS];
    char            **argv = argn;
    int             saw_if = 0;        /* -x (or --if) specified */
    int             saw_conf = 0;      /* Other configuration flags present */
    BYTE            bMode = LCSDEV_MODE_IP;      /* Default mode is IP */


    // Build a copy of the argv list.
    // getopt() and getopt_long() expect argv[0] to be a program name.
    // We need to shift the arguments and insert a dummy argv[0].
    if (argc > (MAX_ARGS-1))
        argc = (MAX_ARGS-1);
    for (i=0; i < argc; i++)
        argn[i+1] = argx[i];
    argc++;
    argn[0] = pDEVBLK->typname;

    // Housekeeping
    memset( &addr, 0, sizeof( struct in_addr ) );

    // Set some initial defaults
    pLCSBLK->pszTUNDevice   = strdup( DEF_NETDEV );
    pLCSBLK->pszOATFilename = NULL;
    pLCSBLK->pszIPAddress   = NULL;
#if defined( OPTION_W32_CTCI )
    pLCSBLK->iKernBuff = DEF_CAPTURE_BUFFSIZE;
    pLCSBLK->iIOBuff   = DEF_PACKET_BUFFSIZE;
#endif

    // Initialize getopt's counter. This is necessary in the case
    // that getopt was used previously for another device.
    OPTRESET();
    optind = 0;

    // Parse any optional arguments
    while (1)
    {
        int     c;

#if defined( OPTION_W32_CTCI )
  #define  LCS_OPTSTRING    "e:n:m:o:s:t:dk:i:w"
#else
  #define  LCS_OPTSTRING    "e:n:x:m:o:s:t:d"
#endif
#if defined( HAVE_GETOPT_LONG )
        int     iOpt;

        static struct option options[] =
        {
            { "mode",   required_argument, NULL, 'e' },
            { "dev",    required_argument, NULL, 'n' },
#if !defined(OPTION_W32_CTCI)
            { "if",     required_argument, NULL, 'x' },
#endif /*!defined(OPTION_W32_CTCI)*/
            { "mac",    required_argument, NULL, 'm' },
            { "oat",    required_argument, NULL, 'o' },
            { "distrc", required_argument, NULL, 's' },
            { "maxtrc", required_argument, NULL, 't' },
            { "debug",  no_argument,       NULL, 'd' },
#if defined( OPTION_W32_CTCI )
            { "kbuff",  required_argument, NULL, 'k' },
            { "ibuff",  required_argument, NULL, 'i' },
            { "swrite", no_argument,       NULL, 'w' },
#endif
            { NULL,     0,                 NULL,  0  }
        };

        c = getopt_long( argc, argv, LCS_OPTSTRING, options, &iOpt );

#else /* defined( HAVE_GETOPT_LONG ) */

        c = getopt( argc, argv, LCS_OPTSTRING );

#endif /* defined( HAVE_GETOPT_LONG ) */

        if (c == -1)
            break;

        switch (c)
        {

        case 'e':  /* Mode */
            if (strcmp( optarg, "SNA" ) == 0 )
            {
                bMode = LCSDEV_MODE_SNA;      /* Mode is SNA */
            }
            else if (strcmp( optarg, "IP" ) == 0 )
            {
                bMode = LCSDEV_MODE_IP;       /* Mode is IP (the default) */
            }
            else
            {
                // "%1d:%04X CTC: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "device mode", optarg );
                return -1;
            }
            break;

        case 'n':

            if (strlen( optarg ) > sizeof( pDEVBLK->filename ) - 1)
            {
                // "%1d:%04X CTC: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "device name", optarg );
                return -1;
            }

            pLCSBLK->pszTUNDevice = strdup( optarg );
            break;

#if !defined(OPTION_W32_CTCI)
        case 'x':  /* TAP network interface name */
            if (strlen( optarg ) > sizeof(pLCSBLK->Port[0].szNetIfName)-1)
            {
                // "%1d:%04X %s: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "TAP device name", optarg );
                return -1;
            }
            STRLCPY( pLCSBLK->Port[0].szNetIfName, optarg );
            saw_if = 1;
            break;
#endif /*!defined(OPTION_W32_CTCI)*/

        case 'm':

            if (0
                || ParseMAC( optarg, mac ) != 0 // (invalid format)
                || !(mac[0] & 0x02)             // (locally assigned MAC bit not ON)
                ||  (mac[0] & 0x01)             // (broadcast bit is ON)
            )
            {
                // "%1d:%04X %s: Option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "MAC address", optarg );
                return -1;
            }

            STRLCPY( pLCSBLK->Port[0].szMACAddress, optarg );
            memcpy( pLCSBLK->Port[0].MAC_Address, &mac, IFHWADDRLEN );
            pLCSBLK->Port[0].fLocalMAC = TRUE;
            saw_conf = 1;
            break;

        case 'o':

            pLCSBLK->pszOATFilename = strdup( optarg );
            saw_conf = 1;
            break;

        case 's':     // Size to be traced when discarding a frame. Default is 0.

            iDiscTrace = atoi( optarg );

            if ((iDiscTrace < LCS_DISC_TRACE_MINIMUM || iDiscTrace > LCS_DISC_TRACE_MAXIMUM) && iDiscTrace != LCS_DISC_TRACE_ZERO)
            {
                // "%1d:%04X CTC: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "discard trace size", optarg );
                return -1;
            }

            pLCSBLK->iDiscTrace = iDiscTrace;
            break;

        case 't':     // Size to be traced of structures or frames. Default is LCS_TRACE_LEN_DEFAULT, i.e. 128.

            iTraceLen = atoi( optarg );

            if ((iTraceLen < LCS_TRACE_LEN_MINIMUM || iTraceLen > LCS_TRACE_LEN_MAXIMUM) && iTraceLen != LCS_TRACE_LEN_ZERO)
            {
                // "%1d:%04X CTC: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "maximum trace size", optarg );
                return -1;
            }

            pLCSBLK->iTraceLen = iTraceLen;
            break;

        case 'd':

            pLCSBLK->fDebug = TRUE;
            break;

#if defined( OPTION_W32_CTCI )

        case 'k':     // Kernel Buffer Size (Windows only)

            iKernBuff = atoi( optarg );

            if (iKernBuff * 1024 < MIN_CAPTURE_BUFFSIZE    ||
                iKernBuff * 1024 > MAX_CAPTURE_BUFFSIZE)
            {
                // "%1d:%04X CTC: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "kernel buffer size", optarg );
                return -1;
            }

            pLCSBLK->iKernBuff = iKernBuff * 1024;
            break;

        case 'i':     // I/O Buffer Size (Windows only)

            iIOBuff = atoi( optarg );

            if (iIOBuff * 1024 < MIN_PACKET_BUFFSIZE    ||
                iIOBuff * 1024 > MAX_PACKET_BUFFSIZE)
            {
                // "%1d:%04X CTC: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "dll i/o buffer size", optarg );
                return -1;
            }

            pLCSBLK->iIOBuff = iIOBuff * 1024;
            break;

        case 'w':     // Single packet writes mode (Windows only)

            pLCSBLK->fNoMultiWrite = TRUE;
            break;

#endif // defined( OPTION_W32_CTCI )

        default:
            break;
        }
    }

    argc -= optind;
    argv += optind;

#if !defined(OPTION_W32_CTCI)
    /* If the -x option and one of the configuration options (e.g. the */
    /* -m or the -o options has been specified, reject the -x option.  */
    if (saw_if && saw_conf)
    {
        /* HHC00916 "%1d:%04X %s: option %s value %s invalid" */
        WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
               "TAP device name", pLCSBLK->Port[0].szNetIfName );
        return -1;
    }
#endif /*!defined(OPTION_W32_CTCI)*/

    if (argc > 1)
    {
        /* There are two or more arguments. */
        /* HHC00915 "%1d:%04X %s: incorrect number of parameters" */
        WRMSG(HHC00915, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname );
        return -1;
    }
    else if (argc == 1)
    {
        /* There is one argument, check for an IPv4 address. */
        if (inet_aton( *argv, &addr ) != 0)
        {
            /* The argument is an IPv4 address. If the -x option was specified, */
            /* it has pre-named the TAP interface that LCS will use (*nix).     */
            if ( bMode == LCSDEV_MODE_SNA )      /* Is the mode SNA? */
            {
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "IP address", *argv );
                return -1;
            }
            if ( pLCSBLK->pszIPAddress ) { free( pLCSBLK->pszIPAddress ); pLCSBLK->pszIPAddress = NULL; }
            pLCSBLK->pszIPAddress = strdup( *argv );
            pLCSBLK->Port[0].fPreconfigured = FALSE;
        } else {
#if defined(OPTION_W32_CTCI)
            WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                   "IP address", *argv );
            return -1;
#else /*defined(OPTION_W32_CTCI)*/
            /* The argument is not an IPv4 address. If the -x option was */
            /* specified, the argument shouldn't have been specified.    */
            if (saw_if ) {
                WRMSG( HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                       "IP address", *argv );
                return -1;
            }
            /* The -x option was not specified, so the argument should be the  */
            /* name of the pre-configured TAP interface that LCS will use.     */
            STRLCPY( pLCSBLK->Port[0].szNetIfName, argv[0] );
            pLCSBLK->Port[0].fPreconfigured = TRUE;
#endif /*defined(OPTION_W32_CTCI)*/
        }
    }
#if !defined(OPTION_W32_CTCI)
    else
    {
        /* There are no arguments. If the -x option was specified, it */
        /* named a pre-configured TAP interface that LCS will use.    */
        if (saw_if)
            pLCSBLK->Port[0].fPreconfigured = TRUE;
        else
            pLCSBLK->Port[0].fPreconfigured = FALSE;
    }
#endif /*!defined(OPTION_W32_CTCI)*/

    return bMode;
}

// ====================================================================
//                           BuildOAT
// ====================================================================

#define OAT_STMT_BUFSZ      255

static int  BuildOAT( char* pszOATName, PLCSBLK pLCSBLK )
{
    FILE*       fp;
    char        szBuff[OAT_STMT_BUFSZ];

    int         i;
    char        c;                      // Character work area
    char*       pszStatement = NULL;    // -> Resolved statement
    char*       pszKeyword;             // -> Statement keyword
    char*       pszOperand;             // -> Statement operand
    int         argc;                   // Number of args
    char*       argv[MAX_ARGS];         // Argument array

    PLCSPORT    pLCSPORT;
    PLCSDEV     pLCSDev;
    PLCSRTE     pLCSRTE;

    U16         sPort;
    BYTE        bMode;
    U16         sDevNum;
    BYTE        bType;
    U32         lIPAddr      = 0;       // (network byte order)
    char*       pszIPAddress = NULL;
    char*       pszNetAddr   = NULL;
    char*       pszNetMask   = NULL;
    char*       strtok_str = NULL;

    struct in_addr  addr;               // Work area for IPv4 addresses
    char        pathname[MAX_PATH];     // pszOATName in host path format


    // Open the OAT configuration file
    hostpath(pathname, pszOATName, sizeof(pathname));
    fp = fopen( pathname, "r" );
    if (!fp)
    {
        char msgbuf[MAX_PATH+80];
        MSGBUF( msgbuf, "fopen( \"%s\", \"r\" )", pathname);
        // "CTC: error in function %s: %s"
        WRMSG( HHC00940, "E", msgbuf, strerror( errno ) );
        return -1;
    }

    for (;;)
    {
        // Read next record from the OAT file
        if (!ReadOAT( pszOATName, fp, szBuff ))
        {
            fclose( fp );
            return 0;
        }

        if (pszStatement)
        {
            free( pszStatement );
            pszStatement = NULL;
        }

        // Make a copy of the OAT statement
        pszStatement = strdup( szBuff );

        /* Perform variable substitution */
        {
            char *cl;

            set_symbol( "CUU",  "$(CUU)"  );
            set_symbol( "CCUU", "$(CCUU)" );
            set_symbol( "DEVN", "$(DEVN)" );

            cl = resolve_symbol_string( pszStatement );

            if (cl)
            {
                STRLCPY( szBuff, cl );
                free( cl );
                free( pszStatement );
                pszStatement = strdup( szBuff );
            }
        }

        sPort        = 0;
        bMode        = 0;
        sDevNum      = 0;
        bType        = 0;
        pszIPAddress = NULL;
        pszNetAddr   = NULL;
        pszNetMask   = NULL;

        memset( &addr, 0, sizeof( addr ) );

        // Split the statement into keyword and first operand
        pszKeyword = strtok_r( pszStatement, " \t", &strtok_str );
        pszOperand = strtok_r( NULL,   " \t", &strtok_str );

        // Extract any arguments
        for (argc = 0;
             argc < MAX_ARGS &&
                 ( argv[argc] = strtok_r( NULL, " \t", &strtok_str ) ) != NULL &&
                 argv[argc][0] != '#';
             argc++)
                 /* nop */
                 ;

        // Clear any unused argument pointers
        for (i = argc; i < MAX_ARGS; i++)
            argv[i] = NULL;

        if (strcasecmp( pszKeyword, "HWADD" ) == 0)
        {
            if (!pszOperand        ||
                argc       != 1    ||
                sscanf( pszOperand, "%hi%c", &sPort, &c ) != 1)
            {
                // "CTC: invalid statement %s in file %s: %s"
                WRMSG( HHC00954, "E", "HWADD", pszOATName, szBuff );
                return -1;
            }

            pLCSPORT = &pLCSBLK->Port[sPort];

            if (0
                || ParseMAC( argv[0], pLCSPORT->MAC_Address ) != 0
                || !(pLCSPORT->MAC_Address[0] & 0x02)
                ||  (pLCSPORT->MAC_Address[0] & 0x01)
            )
            {
                // "CTC: invalid %s %s in statement %s in file %s: %s"
                WRMSG( HHC00955, "E", "MAC", argv[0], "HWADD", pszOATName, szBuff );

                memset( pLCSPORT->MAC_Address, 0, IFHWADDRLEN );
                return -1;
            }

            STRLCPY( pLCSPORT->szMACAddress, argv[0] );
            pLCSPORT->fLocalMAC = TRUE;
        }
        else if (strcasecmp( pszKeyword, "ROUTE" ) == 0)
        {
            if (!pszOperand        ||
                argc       != 2    ||
                sscanf( pszOperand, "%hi%c", &sPort, &c ) != 1)
            {
                // "CTC: invalid statement %s in file %s: %s"
                WRMSG( HHC00954, "E", "ROUTE", pszOATName, szBuff );
                return -1;
            }

            if (inet_aton( argv[0], &addr ) == 0)
            {
                // "CTC: invalid %s %s in statement %s in file %s: %s"
                WRMSG( HHC00955, "E", "net address", argv[0], "ROUTE", pszOATName, szBuff );
                return -1;
            }

            pszNetAddr = strdup( argv[0] );

            if (inet_aton( argv[1], &addr ) == 0)
            {
                free(pszNetAddr);
                // "CTC: invalid %s %s in statement %s in file %s: %s"
                WRMSG( HHC00955, "E", "netmask", argv[1], "ROUTE", pszOATName, szBuff );
                return -1;
            }

            pszNetMask = strdup( argv[1] );

            pLCSPORT = &pLCSBLK->Port[sPort];

            if (!pLCSPORT->pRoutes)
            {
                pLCSPORT->pRoutes = malloc( sizeof( LCSRTE ) );
                pLCSRTE = pLCSPORT->pRoutes;
            }
            else
            {
                for (pLCSRTE = pLCSPORT->pRoutes;
                     pLCSRTE->pNext;
                     pLCSRTE = pLCSRTE->pNext);

                pLCSRTE->pNext = malloc( sizeof( LCSRTE ) );
                pLCSRTE = pLCSRTE->pNext;
            }

            pLCSRTE->pszNetAddr = pszNetAddr;
            pLCSRTE->pszNetMask = pszNetMask;
            pLCSRTE->pNext      = NULL;
        }
        else // (presumed OAT file device statement)
        {
            if (!pszKeyword || !pszOperand)
            {
                // "CTC: error in file %s: missing device number or mode"
                WRMSG( HHC00956, "E", pszOATName );
                return -1;
            }

            /*                                                       */
            /* The keyword is a device address.                      */
            /*                                                       */
            /* If the operand, i.e. the mode, is IP, the device      */
            /* address can be either the even or the odd address of  */
            /* an even/odd device address pair, e.g. either 0E42 or  */
            /* 0E43. Whichever device address is specified will      */
            /* become the read device, and the other device address  */
            /* of the even/odd pair will become the write device.    */
            /* For example, if device address 0E42 is specified,     */
            /* device address 0E42 will become the read device, and  */
            /* device address 0E43 will become the write device.     */
            /* However, if device address 0E43 is specified, device  */
            /* address 0E43 will become the read device, and device  */
            /* address 0E42 will become the write device.            */
            /*                                                       */
            /* If the operand, i.e. the mode, is SNA, the device     */
            /* address can be any device address, e.g. 0E42 or 0E43. */
            /* SNA uses a single device for both read and write.     */
            /*                                                       */
            /* The following extract from an OAT illustrates the     */
            /* above. The guest will have four interfaces, two IP    */
            /* and two SNA. All four interfaces will use the same    */
            /* port, i.e the tap. The first IP interface will use    */
            /* the even device address 0E40 as the read device and   */
            /* the odd device address 0E41 as the write device.      */
            /* However, the second IP interface will use the odd     */
            /* device address 0E43 as the read device and the even   */
            /* device address 0E42 as the write device. The first    */
            /* SNA interface will use the even device address 0E44,  */
            /* and the second SNA interface will use the odd device  */
            /* address 0E45.                                         */
            /*                                                       */
            /*   0E40  IP   00  NO  192.168.1.1                      */
            /*   0E43  IP   00  NO  192.168.1.2                      */
            /*   0E44  SNA  00                                       */
            /*   0E45  SNA  00                                       */
            /*                                                       */
            if (strlen( pszKeyword ) > 4 ||
                sscanf( pszKeyword, "%hx%c", &sDevNum, &c ) != 1)
            {
                // "CTC: error in file %s: invalid %s value %s"
                WRMSG( HHC00957, "E", pszOATName, "device number", pszKeyword );
                return -1;
            }

            if (strcasecmp( pszOperand, "IP" ) == 0)
            {
                bMode = LCSDEV_MODE_IP;

                if (argc < 1)
                {
                    // "CTC: error in file %s: %s: missing port number"
                    WRMSG( HHC00958, "E", pszOATName, szBuff );
                    return -1;
                }

                if (sscanf( argv[0], "%hi%c", &sPort, &c ) != 1)
                {
                    // "CTC: error in file %s: invalid %s value %s"
                    WRMSG( HHC00957, "E", pszOATName, "port number", argv[0] );
                    return -1;
                }

                if (argc > 1)
                {
                         if (strcasecmp( argv[1], "PRI" ) == 0) bType = LCSDEV_TYPE_PRIMARY;
                    else if (strcasecmp( argv[1], "SEC" ) == 0) bType = LCSDEV_TYPE_SECONDARY;
                    else if (strcasecmp( argv[1], "NO"  ) == 0) bType = LCSDEV_TYPE_NONE;
                    else
                    {
                        // "CTC: error in file %s: %s: invalid entry starting at %s"
                        WRMSG( HHC00959, "E", pszOATName, szBuff, argv[1] );
                        return -1;
                    }

                    if (argc > 2)
                    {
                        pszIPAddress = strdup( argv[2] );

                        if (inet_aton( pszIPAddress, &addr ) == 0)
                        {
                            // "CTC: error in file %s: invalid %s value %s"
                            WRMSG( HHC00957, "E", pszOATName, "IP address", pszIPAddress );
                            return -1;
                        }

                        lIPAddr = addr.s_addr;  // (network byte order)
                    }
                }
            }
            else if (strcasecmp( pszOperand, "SNA" ) == 0)
            {
                bMode = LCSDEV_MODE_SNA;

                if (argc < 1)
                {
                    // "CTC: error in file %s: %s: missing port number"
                    WRMSG( HHC00958, "E", pszOATName, szBuff );
                    return -1;
                }

                if (sscanf( argv[0], "%hi%c", &sPort, &c ) != 1)
                {
                    // "CTC: error in file %s: invalid %s value %s"
                    WRMSG( HHC00957, "E", pszOATName, "port number", argv[0] );
                    return -1;
                }

                if (argc > 1)
                {
                    // "CTC: error in file %s: %s: SNA does not accept any arguments"
                    WRMSG( HHC00960, "E", pszOATName, szBuff );
                    return -1;
                }
            }
            else
            {
                // "CTC: error in file %s: %s: invalid mode"
                WRMSG( HHC00961, "E", pszOATName, pszOperand );
                return -1;
            }

            // Create new LCS Device...

            pLCSDev = malloc( sizeof( LCSDEV ) );
            memset( pLCSDev, 0, sizeof( LCSDEV ) );

            pLCSDev->sAddr        = sDevNum;
            pLCSDev->bMode        = bMode;
            pLCSDev->bPort        = sPort;
            pLCSDev->bType        = bType;
            pLCSDev->lIPAddress   = lIPAddr;   // (network byte order)
            pLCSDev->pszIPAddress = pszIPAddress;
            pLCSDev->pNext        = NULL;

            // Add it to end of chain...

            if (!pLCSBLK->pDevices)
                pLCSBLK->pDevices = pLCSDev; // (first link in chain)
            else
            {
                PLCSDEV pOldLastLCSDEV;
                // (find last link in chain)
                for (pOldLastLCSDEV = pLCSBLK->pDevices;
                     pOldLastLCSDEV->pNext;
                     pOldLastLCSDEV = pOldLastLCSDEV->pNext);
                // (add new link to end of chain)
                pOldLastLCSDEV->pNext = pLCSDev;
            }

            // Count it...

            if (pLCSDev->bMode == LCSDEV_MODE_IP)
                pLCSBLK->icDevices += 2;
            else
                pLCSBLK->icDevices += 1;

        } // end OAT file statement

    } // end for (;;)

    UNREACHABLE_CODE( return -1 );
}

// ====================================================================
//                           ReadOAT
// ====================================================================

static char*  ReadOAT( char* pszOATName, FILE* fp, char* pszBuff )
{
    int     c;                          // Character work area
    int     iLine = 0;                  // Statement number
    int     iLen;                       // Statement length

    while (1)
    {
        // Increment statement number
        iLine++;

        // Read next statement from OAT
        for (iLen = 0 ; ; )
        {
            // Read character from OAT
            c = fgetc( fp );

            // Check for I/O error
            if (ferror( fp ))
            {
                // "CTC: error in file %s: reading line %d: %s"
                WRMSG( HHC00962, "E", pszOATName, iLine, strerror( errno ) );
                return NULL;
            }

            // Check for end of file
            if (iLen == 0 && ( c == EOF || c == '\x1A' ))
                return NULL;

            // Check for end of line
            if (c == '\n' || c == EOF || c == '\x1A')
                break;

            // Ignore leading blanks and tabs
            if (iLen == 0 && ( c == ' ' || c == '\t' ))
                continue;

            // Ignore nulls and carriage returns
            if (c == '\0' || c == '\r')
                continue;

            // Check that statement does not overflow bufffer
            if (iLen >= OAT_STMT_BUFSZ)
            {
                // "CTC: error in file %s: line %d is too long"
                WRMSG( HHC00963, "E", pszOATName, iLine );
                exit(1);
            }

            // Append character to buffer
            pszBuff[iLen++] = c;
        }

        // Null terminate buffer
        pszBuff[ iLen ] = 0;

        // Remove trailing whitespace
        RTRIM( pszBuff );
        iLen = (int) strlen( pszBuff );

        // Ignore comments and null statements
        if (!iLen || pszBuff[0] == '*' || pszBuff[0] == '#')
            continue;

        break;
    }

    return pszBuff;
}


// ====================================================================
//                       LCS_AttnThread
// ====================================================================
//
// This is the thread that generates Attention interrupts to the guest.
// Only used when there are one or more SNA devices.
//
// --------------------------------------------------------------------

static void*  LCS_AttnThread( void* arg)
{

    PLCSBLK     pLCSBLK;
    PLCSATTN    pLCSATTN;
    PLCSATTN    pLCSATTNprev, pLCSATTNcurr, pLCSATTNnext;
    PLCSDEV     pLCSDEV;
    DEVBLK*     pDEVBLK;
    /* --------------------------------------------------------------------- */
    int         interval;              /* interval between attempts  FixMe! Configurable? */
    int         dev_attn_rc;           /* device_attention RC    */
    int         busy_waits;            /* Number of times waited for */
                                       /* a Busy condition to end    */
    /* --------------------------------------------------------------------- */


    PTT_DEBUG( "ATTNTHRD: ENTRY", 000, 000, 000 );

    /* Point to the LCSBLK and obtain the pid of this thread. */
    pLCSBLK = (PLCSBLK) arg;
    pLCSBLK->AttnPid = getpid();

    for ( ; ; )
    {

        /* */
        PTT_DEBUG( "GET  AttnEventLock", 000, 000, 000 );
        obtain_lock( &pLCSBLK->AttnEventLock );
        PTT_DEBUG( "GOT  AttnEventLock", 000, 000, 000 );
        {
            for( ; ; )
            {
                if ( pLCSBLK->fCloseInProgress )
                {
                    PTT_DEBUG( "ATTNTHRD Closing...", 000, 000, 000 );
                    break;
                }

                if ( pLCSBLK->pAttns )
                {
                    PTT_DEBUG( "ATTNTHRD Attn...", 000, 000, 000 );
                    break;
                }

                PTT_DEBUG( "WAIT AttnEventLock", 000, 000, 000 );
                timed_wait_condition_relative_usecs
                (
                    &pLCSBLK->AttnEvent,         // ptr to condition to wait on
                    &pLCSBLK->AttnEventLock,     // ptr to controlling lock (must be held!)
                    3*1000*1000,                 // max #of microseconds to wait, i.e. 3 seconds
                    NULL                         // [OPTIONAL] ptr to tod value (may be NULL)
                );
                PTT_DEBUG( "WOKE AttnEventLock", 000, 000, 000 );
            }
        }
        PTT_DEBUG( "REL  AttnEventLock", 000, 000, 000 );
        release_lock( &pLCSBLK->AttnEventLock );

        /* Exit when told... */
        if ( pLCSBLK->fCloseInProgress )
        {
            PTT_DEBUG( "ATTNTHRD Closing...", 000, 000, 000 );
            break;
        }

        /* Remove the chain of LCSATTN blocks */
        PTT_DEBUG( "GET  AttnLock", 000, 000, 000 );
        obtain_lock( &pLCSBLK->AttnLock );
        PTT_DEBUG( "GOT  AttnLock", 000, 000, 000 );
        {
            pLCSATTN = pLCSBLK->pAttns;
            pLCSBLK->pAttns = NULL;
            if ( pLCSATTN )
            {
                PTT_DEBUG( "REM  Attn (All)", pLCSATTN, 000, 000 );
            }
            else
            {
                PTT_DEBUG( "REM  Attn (Non)", 000, 000, 000 );
            }
        }
        PTT_DEBUG( "REL  AttnLock", 000, 000, 000 );
        release_lock( &pLCSBLK->AttnLock );

        /* Reverse the chain of LCSATTN blocks */
        if ( pLCSATTN )
        {
            pLCSATTNprev = NULL;
            pLCSATTNcurr = pLCSATTN;
            while( pLCSATTNcurr )
            {
                pLCSATTNnext = pLCSATTNcurr->pNext;
                pLCSATTNcurr->pNext = pLCSATTNprev;
                pLCSATTNprev = pLCSATTNcurr;
                pLCSATTNcurr = pLCSATTNnext;
            }
            pLCSATTN = pLCSATTNprev;
        }

        /* Process the chain of LCSATTN blocks */
        while ( pLCSATTN )
        {
            /* Point to the next LCSATTN block in the chain, assuming there is one */
            pLCSATTNnext = pLCSATTN->pNext;

            /* Point to the LCSDEV and the read DEVBLK for the command */
            pLCSDEV = pLCSATTN->pDevice;
            pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];

            /* Only raise an Attention if there is at least one buffer waiting to be read. */
            if (pLCSDEV->pFirstLCSIBH)
            {

                PTT_DEBUG( "PRC  Attn", pLCSATTN, pDEVBLK->devnum, 000 );

                /* --------------------------------------------------------------------- */

                interval = 100;
                dev_attn_rc = 0;       /* device_attention RC    */
                busy_waits = 0;        /* Number of times waited for */
                                       /* a Busy condition to end    */

                for( ; ; )
                {

                    // Wait a small (but increasing) amount of time.
                    USLEEP(interval);

//??                // is there still something in our frame buffer?
//??                if (!pLCSDEV->fDataPending && !pLCSDEV->fReplyPending)
//??                {
//??                    break;
//??                }

                    // Raise Attention
                    dev_attn_rc = device_attention( pDEVBLK, CSW_ATTN );
                    PTT_DEBUG( "Raise Attn   ", 000, pDEVBLK->devnum, dev_attn_rc );

                    if (pLCSDEV->pLCSBLK->fDebug)
                    {
                        char    tmp[256];
                        snprintf( (char*)tmp, 256, "device_attention rc=%d  %d  %d", dev_attn_rc, busy_waits, interval );
                        WRMSG(HHC03991, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, tmp );
                    }

                    // ATTN RC=1 means a device busy status did
                    // appear so that the signal did not work.
                    // We will retry after some (increasingly)
                    // small amount of time.
                    if ( dev_attn_rc != 1 )
                    {
                        break;
                    }

                    busy_waits++;

                    if ( busy_waits >= 20 )
                    {
                        break;
                    }

                    interval += 100;

                }   // end for ( ; ; )

                /* --------------------------------------------------------------------- */

            }

            /* Free the LCSATTN block that has just been processed */
            free (pLCSATTN);

            /* Point to the next LCSATTN block in the chain, assuming there is one */
            pLCSATTN = pLCSATTNnext;
        }  // end while (pLCSATTN)

    }  // end for ( ; ; )

    PTT_DEBUG( "ATTNTHRD: EXIT", 000, 000, 000 );

//  {                                                                          /* FixMe! Remove! */
//      char    tmp[256];                                                      /* FixMe! Remove! */
//      snprintf( (char*)tmp, 256, "LCS_AttnThread terminated" );              /* FixMe! Remove! */
//      WRMSG(HHC03984, "D", tmp );                                            /* FixMe! Remove! */
//  }                                                                          /* FixMe! Remove! */

    return NULL;
}   // End of LCS_AttnThread


// ====================================================================
//                       LCS_Write_SNA
// ====================================================================
// The guest o/s is issuing a Write CCW for our LCS device. All LCS
// Frames in its buffer which are NOT internal Command Frames will
// be immediately written to our host's adapter (via TunTap). Frames
// that are internal Command Frames however are processed internally
// and cause a "reply" frame to be enqueued to the LCS Device output
// buffer to be eventually returned back to the guest the next time
// it issues a Read CCW (satisfied by LCS_Read).
// --------------------------------------------------------------------
//
// SNA mode is, inevitably, more complicated. When the XCA is activated
// the first two things sent from VTAM are LCS commands frames, both
// prefixed with a 4-byte LCSHDR, the first an SNA Start LAN command,
// the second an SNA LAN Statistics command. The following illustrates
// an SNA Start LAN command:-
//   HHC00979D LCS: data: +0000< 00160000 41000000 00000000 00000000  ....A........... ................
//   HHC00979D LCS: data: +0010< 00000000 00000000                    ........         ........
// The first four bytes are the LCSHDR, the remaining bytes are the
// SNA Start LAN command.
//
// After that uncertainty reigns!
//
// When the 8-byte structure is in a reply to VTAM the 8-bytes are
// copied to an XCNCB, and various bits in the third byte are tested.
//

void  LCS_Write_SNA( DEVBLK* pDEVBLK,   U32   sCount,
                     BYTE*   pIOBuf,    BYTE* pUnitStat,
                     U32*    pResidual )
{

    PLCSDEV     pLCSDEV      = (PLCSDEV) pDEVBLK->dev_data;
    PLCSBLK     pLCSBLK      = pLCSDEV->pLCSBLK;
    PLCSHDR     pLCSHDR      = NULL;
    PLCSCMDHDR  pCmdFrame    = NULL;
    PLCSBAF1    pLCSBAF1     = NULL;
    PLCSBAF2    pLCSBAF2     = NULL;
    BYTE*       pIOBufStart  = NULL;
    BYTE*       pIOBufEnd    = NULL;
    U16         hwOffset     = 0;
    U16         hwPrevOffset = 0;
    U16         hwLength     = 0;
    U16         hwLenBaf1;
    U16         hwLenBaf2;
    U16         hwTypeBaf;
    int         iTraceLen;
    char        buf[32];
    char        unsupmsg[256];


    // Display up to pLCSBLK->iTraceLen bytes of the data coming from the guest, if debug is active
    if (pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: Accept data of size %d bytes from guest"
        WRMSG(HHC00981, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,  pDEVBLK->typname, (int)sCount );
        if (pLCSBLK->iTraceLen)
        {
            iTraceLen = sCount;
            if (iTraceLen > pLCSBLK->iTraceLen)
            {
                iTraceLen = pLCSBLK->iTraceLen;
                // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, (int)(sCount - iTraceLen) );
            }
            net_data_trace( pDEVBLK, pIOBuf, iTraceLen, FROM_GUEST, 'D', "data", 0 );
        }
    }

    // Process each frame in the buffer...

    PTT_DEBUG( "WSNA ENTRY        ", 000, pDEVBLK->devnum, -1 );
    PTT_TIMING( "beg write", 0, 0, 0 );

    pLCSDEV->fTuntapError = FALSE;
    pLCSDEV->iTuntapErrno = 0;

    // Check whether the Write CCW is the second of three CCW's in the
    // WCTL or SCB channel programs. The channel program contains Control
    // (0x17) or Sense Command Byte (0x14), Write (0x01) and Read (0x02)
    // CCW's. The first, or only, 8-bytes of the Write data are an LCSOCTL.
    if (pLCSDEV->bFlipFlop)
    {

        // The following is an example of the data written by a Write
        // CCW in the WCTL channel program, and occasionally in the
        // SCB channel program.
        //    +0000< 00160000 00000000 00140400 000C0C99  ................ ...............r
        //    +0010< 0003C000 00000000 01000000 0000      ..............   ..{...........
        // The first 8-bytes are the LCSOCTL. The next 20-bytes are an
        // LCSHDR and associated LCSBAF1 and LCSBAF2. The final 2-bytes
        // are the end of data marker.

        // Copy the OCTL that has just arrived
        pLCSDEV->hwOctlSize = sizeof(LCSOCTL);
        memcpy( &pLCSDEV->Octl, pIOBuf, sizeof(LCSOCTL) );

        // Point to whatever follows the OCTL
        pIOBufStart = &pIOBuf[sizeof(LCSOCTL)];
        pIOBufEnd = pIOBuf + sCount;

        //
        if (sCount > sizeof(LCSOCTL))
        {

            // The output is an OCTL followed by data

            PTT_DEBUG( "WSNA: OCTL & data ", -1, pDEVBLK->devnum, -1 );

            //
            PTT_DEBUG(       "GET  InOutLock    ", 000, pDEVBLK->devnum, -1 );
            obtain_lock( &pLCSDEV->InOutLock   );
            PTT_DEBUG(       "GOT  InOutLock    ", 000, pDEVBLK->devnum, -1 );

            //
            while (1)
            {
                // Save current offset so we can tell how big the next frame is.
                hwPrevOffset = hwOffset;

                // Point to where the LCSHDR at the start of the next frame should be.
                pLCSHDR = (PLCSHDR)( pIOBufStart + hwOffset );

                // Check that there is enough of the LCSHDR to contain the next
                // frame offset, exit the loop if there isn't.
                if ((int)(pIOBufEnd - (BYTE*)pLCSHDR) < (int)sizeof(pLCSHDR->hwOffset))
                    break;

                // Get the next frame offset, exit the loop if zero.
                FETCH_HW( hwOffset, pLCSHDR->hwOffset );
                if (hwOffset == 0)   // ("EOF")
                    break;

                // Calculate size of this LCS Frame
                hwLength = hwOffset - hwPrevOffset;

                switch (pLCSHDR->bType)
                {

                case LCS_FRMTYP_SNA:    // 0x04: LCS Baffle

                    PTT_DEBUG( "WSNA: Baffle 1    ", -1, pDEVBLK->devnum, -1 );

                    // Point to and get length of the LCSBAF1/LCSBAF2 structures.
                    pLCSBAF1 = (PLCSBAF1)( (BYTE*)pLCSHDR + sizeof(LCSHDR) );
                    FETCH_HW( hwLenBaf1, pLCSBAF1->hwLenBaf1 );
                    FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
                    pLCSBAF2 = (PLCSBAF2)( (BYTE*)pLCSBAF1 + hwLenBaf1 );

                    // Process the LCSBAF1/LCSBAF2 structures.
                    FETCH_HW( hwTypeBaf, pLCSBAF1->hwTypeBaf );
                    switch (hwTypeBaf)
                    {

                    // 0D10 is used when the connection is active for outbound frames.

                    case 0x0D10:
                        Process_0D10( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        break;

                    // 0D00 is used when the connection is active for ???.

                    case 0x0D00:
                        Process_0D00( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        break;

                    // 8C0B is used when a connection is inactivated.

                    case 0x8C0B:
                        Process_8C0B( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        break;

                    // 0C0A, 0C25, 0C22 and 8D00 are used when a connection is activated.

                    case 0x0C0A:
                        Process_0C0A( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    case 0x0C25:
                        Process_0C25( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        break;

                    case 0x0C22:
                        Process_0C22( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        break;

                    case 0x8D00:
                        Process_8D00( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        break;

                    // 0C0B is used when a connection is inactivated.

                    case 0x0C0B:
                        Process_0C0B( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    // 0C99 and 0C0D ure used when the XCA is activated.

                    case 0x0C99:
                        Process_0C99( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    case 0x0C0D:
                        Process_0C0D( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    // 0C0E and 0C98 ure used when the XCA is inactivated.

                    case 0x0C0E:
                        Process_0C0E( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    case 0x0C98:
                        Process_0C98( pLCSDEV, pLCSHDR, pLCSBAF1, pLCSBAF2, hwLenBaf1, hwLenBaf2 );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    default:
                        PTT_DEBUG( "*BAF=Unsupported! ", hwTypeBaf, pDEVBLK->devnum, -1 );
                        snprintf( unsupmsg, sizeof(unsupmsg), "LCS: lcs write: unsupported baffle type 0x%4.4X", hwTypeBaf );
                        WRMSG(HHC03984, "W", unsupmsg );  /* FixMe! Proper message number! */
                        break;

                    } // End of  switch (hwTypeBaf)

                    break;

                case LCS_FRMTYP_CMD:    // 0x00: LCS Command Frame

                    pCmdFrame = (PLCSCMDHDR)pLCSHDR;

                    PTT_DEBUG( "WSNA: Cmd frame O ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );

                    switch (pCmdFrame->bCmdCode)
                    {

                    case LCS_CMD_STOPLAN_SNA:   // Stop LAN SNA
                        PTT_DEBUG( "CMD=Stop LAN SNA  ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                        if (pLCSBLK->fDebug)
                        {
                            // "%1d:%04X CTC: executing command %s"
                            WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "stop lan sna" );
                        }
                        LCS_StopLan_SNA( pLCSDEV, pCmdFrame, (int)hwLength );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    default:
                        PTT_DEBUG( "*CMD=Unsupported! ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                        if (pLCSBLK->fDebug)
                        {
                            // "%1d:%04X CTC: executing command %s"
                            MSGBUF( buf, "other (0x%2.2X)", pCmdFrame->bCmdCode );
                            WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, buf );
                        }
                        LCS_UnsuppCmd_SNA( pLCSDEV, pCmdFrame, (int)hwLength );
                        pLCSDEV->fAttnRequired = TRUE;
                        break;

                    } // end switch (LCS Command Frame cmd code)

                    break; // end case LCS_FRMTYP_CMD

                default:
                    PTT_DEBUG( "*WRIT Unsupp frame", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                    // "%1d:%04X CTC: lcs write: unsupported frame type 0x%2.2X"
                    WRMSG( HHC00937, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pLCSHDR->bType );
                    PTT_DEBUG(        "REL  InOutLock    ", 000, pDEVBLK->devnum, -1 );
                    release_lock( &pLCSDEV->InOutLock   );
                    ASSERT( FALSE );
                    pDEVBLK->sense[0] = SENSE_EC;
                    *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
//??                LCS_EndMWrite( pDEVBLK, nEthBytes, nEthFrames );
                    PTT_TIMING( "end write",  0, 0, 0 );
                    PTT_DEBUG( "WSNA EXIT         ", 000, pDEVBLK->devnum, -1 );
                    return;

                }   // End of  switch (pLCSHDR->bType)

            } // end while (1)

            //
            PTT_DEBUG(        "REL  InOutLock    ", 000, pDEVBLK->devnum, -1 );
            release_lock( &pLCSDEV->InOutLock   );

        }
        else
        {

            // The output is an OCTL with nothing following

            PTT_DEBUG( "WSNA: OCTL only   ", -1, pDEVBLK->devnum, -1 );

        }

    }   // End of  if (pLCSDEV->bFlipFlop)

    // The Write CCW is the only CCW in the channel program. This
    // is the normal state while the Start LAN and LAN Stats
    // commands are being processed.
    else
    {

        // The output should be an LCS command, either Start LAN
        // or LAN Stats.

        // Clear the OCTL
        pLCSDEV->hwOctlSize = 0;
        memset(&pLCSDEV->Octl, 0, sizeof(LCSOCTL));

        // Point at whatever has just arrived
        pIOBufStart = pIOBuf;
        pIOBufEnd = pIOBuf + sCount;

        // The output is a command

        PTT_DEBUG( "WSNA: command     ", -1, pDEVBLK->devnum, -1 );

        //
        while (1)
        {
            // Save current offset so we can tell how big the next frame is.
            hwPrevOffset = hwOffset;

            // Point to where the LCSHDR at the start of the next frame should be.
            pLCSHDR = (PLCSHDR)( pIOBufStart + hwOffset );

            // Check that there is enough of the LCSHDR to contain the next
            // frame offset, exit loop if there isn't.
            if ((int)(pIOBufEnd - (BYTE*)pLCSHDR) < (int)sizeof(pLCSHDR->hwOffset))
                break;

            // Get the next frame offset, exit loop if zero.
            FETCH_HW( hwOffset, pLCSHDR->hwOffset );
            if (hwOffset == 0)   // ("EOF")
                break;

            // Calculate size of this LCS Frame
            hwLength = hwOffset - hwPrevOffset;

            switch (pLCSHDR->bType)
            {
            case LCS_FRMTYP_CMD:    // 0x00: LCS Command Frame

                pCmdFrame = (PLCSCMDHDR)pLCSHDR;

                PTT_DEBUG( "WSNA: Cmd frame   ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );

                switch (pCmdFrame->bCmdCode)
                {
                    //  HHC00933  =  "%1d:%04X CTC: executing command %s"

                case LCS_CMD_STRTLAN_SNA:   // Start LAN SNA
                    PTT_DEBUG( "CMD=Start LAN SNA ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                    if (pLCSBLK->fDebug)
                        WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "start lan sna" );
                    LCS_StartLan_SNA( pLCSDEV, pCmdFrame, (int)hwLength );
                    pLCSDEV->fAttnRequired = TRUE;
                    break;

                case LCS_CMD_LANSTAT_SNA:   // LAN Stats SNA
                    PTT_DEBUG( "CMD=LAN Stats SNA ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                    if (pLCSBLK->fDebug)
                        WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "lan statistics sna" );
                    LCS_LanStats_SNA( pLCSDEV, pCmdFrame, (int)hwLength );
                    pLCSDEV->fAttnRequired = TRUE;
                    pLCSDEV->fReceiveFrames = TRUE;
                    break;

                default:
                    PTT_DEBUG( "*CMD=Unsupported! ", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                    if (pLCSBLK->fDebug)
                    {
                        // "%1d:%04X CTC: executing command %s"
                        MSGBUF( buf, "other (0x%2.2X)", pCmdFrame->bCmdCode );
                        WRMSG( HHC00933, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, buf );
                    }
                    LCS_UnsuppCmd_SNA( pLCSDEV, pCmdFrame, (int)hwLength );
                    pLCSDEV->fAttnRequired = TRUE;
                    break;

                } // end switch (LCS Command Frame cmd code)

                break; // end case LCS_FRMTYP_CMD

            default:
                PTT_DEBUG( "*WRIT Unsupp frame", pCmdFrame->bCmdCode, pDEVBLK->devnum, -1 );
                // "%1d:%04X CTC: lcs write: unsupported frame type 0x%2.2X"
                WRMSG( HHC00937, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pLCSHDR->bType );
                ASSERT( FALSE );
                pDEVBLK->sense[0] = SENSE_EC;
                *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
//??            LCS_EndMWrite( pDEVBLK, nEthBytes, nEthFrames );
                PTT_TIMING( "end write",  0, 0, 0 );
                PTT_DEBUG( "WSNA EXIT         ", 000, pDEVBLK->devnum, -1 );
                return;

            } // end switch (LCS Frame type)

        } // end while (1)

    }

    // ----------------------------------------------------------------
    //    End of LCS_Write_SNA
    // ----------------------------------------------------------------

    if (!pLCSDEV->fTuntapError)
    {
        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE;
    }
    else
    {
        // "%1d:%04X CTC: error writing to file %s: %s"
        WRMSG( HHC00936, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                              pDEVBLK->filename, strerror( pLCSDEV->iTuntapErrno ) );

        pDEVBLK->sense[0] = SENSE_EC;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;

        pLCSDEV->iTuntapErrno = 0;
        pLCSDEV->fTuntapError = FALSE;
    }

    PTT_TIMING( "end write",  0, 0, 0 );
    PTT_DEBUG( "WSNA EXIT         ", 000, pDEVBLK->devnum, -1 );
}   // End of LCS_Write_SNA


// ====================================================================
//                       Process_0D10
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 15 (0x0F) and 5 (0x05) or more bytes in length.
//
void Process_0D10 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2)
{
//                   Token
//  000F0D1000426002 40000240 00FF00
//  0 1 2 3 4 5 6 7  8 9 A B  C D E
//
//             TH etc
//  0100000000 xxxxxxxx...........
//  0 1 2 3 4  5 6 7 8 9 A B C ...

    DEVBLK*   pDEVBLK;
    PLCSPORT  pLCSPORT;
    PLCSCONN  pLCSCONN;
    PETHFRM   pEthFrame;
    int       iEthLen;
    int       iLPDULen;
    LLC       llc;
    int       iTHetcLen;
    int       iTraceLen;
    BYTE      frame[1600];
    char      llcmsg[256];

    UNREFERENCED( pLCSHDR   );
    UNREFERENCED( hwLenBaf1 );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    memset( frame, 0, sizeof(frame) );                               // Clear area for ethernet fram
    pEthFrame = (PETHFRM)&frame[0];
    iEthLen = 60;                                                    // Minimum ethernet frame length

    // Find the connection block.
    pLCSCONN = find_connection_by_outbound_token( pLCSDEV, pLCSBAF1->bTokenA );
    if (!pLCSCONN)
    {
        WRMSG( HHC03984, "W", "LCSCONN not found");
        /* FixMe! Need a proper error message here! */
        return;
    }

    //
    memset( &llc, 0, sizeof(LLC) );
    llc.hwDSAP    = LSAP_SNA_Path_Control;
    llc.hwSSAP    = LSAP_SNA_Path_Control;
    llc.hwNS      = pLCSCONN->hwLocalNS;
    pLCSCONN->hwLocalNS = ((pLCSCONN->hwLocalNS + 1) & 0x7F);
    if (pLCSCONN->fIframe)
    {
         llc.hwNR  = ((pLCSCONN->hwRemoteNS + 1) & 0x7F);
    }
    llc.hwType    = Type_Information_Frame;

    // Construct Ethernet frame
    memcpy( &pEthFrame->bDestMAC, &pLCSCONN->bRemoteMAC, IFHWADDRLEN );   // Copy destination MAC address
    memcpy( &pEthFrame->bSrcMAC, &pLCSCONN->bLocalMAC, IFHWADDRLEN );     // Copy source MAC address
    iLPDULen = BuildLLC( &llc, pEthFrame->bData);                         // Build LLC PDU
    STORE_HW( pEthFrame->hwEthernetType, (U16)iLPDULen );                 // Set data length

    // Continue Ethernet frame construction if there is a TH etc.
    iTHetcLen = ( hwLenBaf2 - 5 );                                        // Calculate length of TH etc
    if ( iTHetcLen > 0 )                                                  // Any TH etc?
    {
        if ( iTHetcLen > 1493 )                                           // 1493 = 0x5D5
        {
            snprintf( llcmsg, sizeof(llcmsg), "LCS: Ignoring over long data of %d bytes!!!", iTHetcLen );
            WRMSG(HHC03984, "W", llcmsg );  /* FixMe! Proper message number! */
            return;
        }
        STORE_HW( pEthFrame->hwEthernetType, (U16)(iLPDULen + iTHetcLen) );     // Set LLC and TH etc length
        memcpy( &pEthFrame->bData[iLPDULen], &pLCSBAF2->bByte05, iTHetcLen );   // Copy TH etc
        if ( iEthLen < ((IFHWADDRLEN * 2) + 2 + iLPDULen + iTHetcLen) )
        {
            iEthLen = ((IFHWADDRLEN * 2) + 2 + iLPDULen + iTHetcLen);
        }
    }

    // Trace Ethernet frame before sending to TAP device
    if (pLCSPORT->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
        WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             pLCSDEV->bPort, iEthLen, "802.3 SNA", pLCSPORT->szNetIfName );
        if (pLCSPORT->pLCSBLK->iTraceLen)
        {
            iTraceLen = iEthLen;
            if (iTraceLen > pLCSPORT->pLCSBLK->iTraceLen)
            {
                iTraceLen = pLCSPORT->pLCSBLK->iTraceLen;
                // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, (iEthLen - iTraceLen) );
            }
            net_data_trace( pDEVBLK, (BYTE*)pEthFrame, iTraceLen, FROM_GUEST, 'D', "eth frame", 0 );
        }
    }

    // Write the Ethernet frame to the TAP device
    if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrame, iEthLen ) == iEthLen)
    {
        if (pLCSPORT->pLCSBLK->fDebug)
        {
            snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC information frame sent: CR=%u, NR=%u, NS=%u", llc.hwCR, llc.hwNR, llc.hwNS );
            WRMSG(HHC03984, "D", llcmsg );  /* FixMe! Proper message number! */
        }
    }
    else
    {
        pLCSDEV->iTuntapErrno = errno;
        pLCSDEV->fTuntapError = TRUE;
        PTT_TIMING( "*WRITE ERR", 0, iEthLen, 1 );
    }

    return;
}


// ====================================================================
//                       Process_0D00
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 15 (0x0F) and 26 (0x1A) bytes in length.
//
void Process_0D00 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2)
{
//                   Token
//  000F0D00001A6002 40000240 000000
//  0 1 2 3 4 5 6 7  8 9 A B  C D E
//
//  01000300 00000000 00000000 00000000 00000000 00000000 0000
//  0 1 2 3  4 5 6 7  8 9 A B  C ...

    DEVBLK*     pDEVBLK;
    PLCSPORT    pLCSPORT;
    PLCSCONN    pLCSCONN;
    int         iLPDULen;
    LLC         llc;
    PETHFRM     pEthFrame;
    int         iEthLen;
    BYTE        frame[64];
    char        llcmsg[256];

    UNREFERENCED( pLCSDEV   );
    UNREFERENCED( pLCSHDR   );
    UNREFERENCED( pLCSBAF1  );
    UNREFERENCED( pLCSBAF2  );
    UNREFERENCED( hwLenBaf1 );
    UNREFERENCED( hwLenBaf2 );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    memset( frame, 0, sizeof(frame) );                               // Clear area for ethernet fram
    pEthFrame = (PETHFRM)&frame[0];
    iEthLen = 60;                                                    // Minimum ethernet frame length

    // Find the connection block.
    pLCSCONN = find_connection_by_outbound_token( pLCSDEV, pLCSBAF1->bTokenA );
    if (!pLCSCONN)
    {
        WRMSG( HHC03984, "W", "LCSCONN not found");
        /* FixMe! Need a proper error message here! */
        return;
    }

    //
    FETCH_HW( pLCSCONN->hwDataSeqNum, pLCSBAF2->hwSeqNum );

    //
    memset( &llc, 0, sizeof(LLC) );
    llc.hwDSAP    = LSAP_SNA_Path_Control;
    llc.hwSSAP    = LSAP_SNA_Path_Control;
    llc.hwPF      = 1;
    llc.hwM       = M_SABME_Command;
    llc.hwType    = Type_Unnumbered_Frame;

    // Construct Ethernet frame
    memcpy( &pEthFrame->bDestMAC, &pLCSCONN->bRemoteMAC, IFHWADDRLEN );   // Copy destination MAC address
    memcpy( &pEthFrame->bSrcMAC, &pLCSCONN->bLocalMAC, IFHWADDRLEN );     // Copy source MAC address
    iLPDULen = BuildLLC( &llc, pEthFrame->bData);                         // Build LLC PDU
    STORE_HW( pEthFrame->hwEthernetType, (U16)iLPDULen );                 // Set data length

    // Trace Ethernet frame before sending to TAP device
    if (pLCSPORT->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
        WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             pLCSDEV->bPort, iEthLen, "802.3 SNA", pLCSPORT->szNetIfName );
        net_data_trace( pDEVBLK, (BYTE*)pEthFrame, iEthLen, FROM_GUEST, 'D', "eth frame", 0 );
    }

    // Write the Ethernet frame to the TAP device
    if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrame, iEthLen ) == iEthLen)
    {
        if (pLCSPORT->pLCSBLK->fDebug)
        {
            snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame sent: CR=%u, M=%s", llc.hwCR, "SABME" );
            WRMSG(HHC03984, "D", llcmsg );  /* FixMe! Proper message number! */
        }
    }
    else
    {
        pLCSDEV->iTuntapErrno = errno;
        pLCSDEV->fTuntapError = TRUE;
        PTT_TIMING( "*WRITE ERR", 0, iEthLen, 1 );
    }

    return;
}


// ====================================================================
//                       Process_8C0B
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 12 (0x0C) and 3 (0x03) bytes in length.
//
void Process_8C0B (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2)
{
//                   Token
//  000C8C0B00036002 40000240
//  0 1 2 3 4 5 6 7  8 9 A B
//
//  010000
//  0 1 2

    DEVBLK*     pDEVBLK;
    PLCSPORT    pLCSPORT;
    PLCSCONN    pLCSCONN;
    int         iLPDULen;
    LLC         llc;
    PETHFRM     pEthFrame;
    int         iEthLen;
    BYTE        frame[64];
    char        llcmsg[256];

    UNREFERENCED( pLCSHDR   );
    UNREFERENCED( pLCSBAF1  );
    UNREFERENCED( hwLenBaf1 );
    UNREFERENCED( hwLenBaf2 );
    UNREFERENCED( pLCSBAF2  );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    memset( frame, 0, sizeof(frame) );                               // Clear area for ethernet fram
    pEthFrame = (PETHFRM)&frame[0];
    iEthLen = 60;                                                    // Minimum ethernet frame length

    // Find the connection block.
    pLCSCONN = find_connection_by_outbound_token( pLCSDEV, pLCSBAF1->bTokenA );
    if (!pLCSCONN)
    {
        WRMSG( HHC03984, "W", "LCSCONN not found");
        /* FixMe! Need a proper error message here! */
        return;
    }

    //
    memset( &llc, 0, sizeof(LLC) );
    llc.hwDSAP    = LSAP_SNA_Path_Control;
    llc.hwSSAP    = LSAP_SNA_Path_Control;
    llc.hwCR      = 1;
    llc.hwPF      = 1;
    llc.hwM       = M_UA_Response;
    llc.hwType    = Type_Unnumbered_Frame;

    // Construct Ethernet frame
    memcpy( &pEthFrame->bDestMAC, &pLCSCONN->bRemoteMAC, IFHWADDRLEN );   // Copy destination MAC address
    memcpy( &pEthFrame->bSrcMAC, &pLCSCONN->bLocalMAC, IFHWADDRLEN );     // Copy source MAC address
    iLPDULen = BuildLLC( &llc, pEthFrame->bData);                         // Build LLC PDU
    STORE_HW( pEthFrame->hwEthernetType, (U16)iLPDULen );                 // Set data length

    // Trace Ethernet frame before sending to TAP device
    if (pLCSPORT->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
        WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             pLCSDEV->bPort, iEthLen, "802.3 SNA", pLCSPORT->szNetIfName );
        net_data_trace( pDEVBLK, (BYTE*)pEthFrame, iEthLen, FROM_GUEST, 'D', "eth frame", 0 );
    }

    // Write the Ethernet frame to the TAP device
    if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrame, iEthLen ) == iEthLen)
    {
        if (pLCSPORT->pLCSBLK->fDebug)
        {
            snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame sent: CR=%u, M=%s", llc.hwCR, "UA" );
            WRMSG(HHC03984, "D", llcmsg );  /* FixMe! */
        }
    }
    else
    {
        pLCSDEV->iTuntapErrno = errno;
        pLCSDEV->fTuntapError = TRUE;
        PTT_TIMING( "*WRITE ERR", 0, iEthLen, 1 );
    }

    remove_connection_from_chain( pLCSDEV, pLCSCONN );
    free_connection( pLCSDEV, pLCSCONN );

    return;
}


// ====================================================================
//                       Process_0C0A
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 16 (0x10) and 43 (0x2B) bytes in length.
//
// The outbound LCSBAF2 contains the MAC address of the remote end
// of the connection that is about to be activated. The following
// shows an LCSBAF2 with the remote MAC address starting at +3:-
//   01000800 0CCE4B47 40040000 00000000
//   00010000 02000000 00000000 00000000
//   00000000 02000000 000004
//
// The inbound LCSBAF1 and LCSBAF2 returned to VTAM are, respectively,
// 28 (0x1C) and 3 (0x03) bytes in length.
//
void Process_0C0A (PLCSDEV pLCSDEV, PLCSHDR pOutHDR, PLCSBAF1 pOutBAF1, PLCSBAF2 pOutBAF2, U16 hwLenOutBaf1, U16 hwLenOutBaf2)
{
#define INBOUND_CC0A_SIZE 36
static const BYTE Inbound_CC0A[INBOUND_CC0A_SIZE] =
                 {
                   0x00, 0x24, 0x04, 0x00,                           /* LCSHDR  */
                   0x00, 0x1C, 0xCC, 0x0A, 0x00, 0x03, 0x60, 0x01,   /* LCSBAF1 */
                   0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0xff, 0xff, 0xff, 0xff,
                   0x01, 0xff, 0xff,                                 /* LCSBAF2 */
                   0x00                                              /* Filler  */
                 };

    DEVBLK*     pDEVBLK;
    PLCSPORT    pLCSPORT;
    PLCSCONN    pLCSCONN;
    PLCSIBH     pLCSIBH;
    PLCSHDR     pInHDR;
    PLCSBAF1    pInBAF1;
    PLCSBAF2    pInBAF2;
    U16         hwLenInBaf1;
//  U16         hwLenInBaf2;

    UNREFERENCED( pOutHDR      );
    UNREFERENCED( pOutBAF1     );
    UNREFERENCED( hwLenOutBaf1 );
    UNREFERENCED( hwLenOutBaf2 );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[ pLCSDEV->bPort ];

    // Find the connection block.
    pLCSCONN = find_connection_by_remote_mac( pLCSDEV, (MAC*)&pOutBAF2->bByte03 );
    if (!pLCSCONN)  // There isn't an existing LCSCONN.
    {
        // Create the LCSCONN for the new outbound connection.
        pLCSCONN = alloc_connection( pLCSDEV );
        pLCSCONN->hwCreated = LCSCONN_CREATED_OUTBOUND;

        memcpy( &pLCSCONN->bLocalMAC, &pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
        pLCSCONN->bLocalMAC[5]++;
#endif
        memcpy( &pLCSCONN->bRemoteMAC, &pOutBAF2->bByte03, IFHWADDRLEN );

        memcpy( &pLCSCONN->bInToken, &pOutBAF1->bTokenB, sizeof(pLCSCONN->bInToken) );  // Copy Inbound token
        obtain_lock( &TokenLock );
        STORE_FW( pLCSCONN->bOutToken, uToken );                                        // Outbound token
        uToken += INCREMENT_TOKEN;
        release_lock( &TokenLock );

        if (pLCSDEV->pLCSBLK->fDebug)
        {
            WRMSG( HHC03984, "I", "Created LCSCONN Outbound");
            net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
        }

        add_connection_to_chain( pLCSDEV, pLCSCONN );
    }
    else  // There is an existing LCSCONN.
    {
        if ( pLCSCONN->hwCreated == LCSCONN_CREATED_INBOUND )  // The existing LSCCONN is for an inbound connection.
        {
            if (pLCSDEV->pLCSBLK->fDebug)
            {
                WRMSG( HHC03984, "I", "Found LCSCONN Inbound");
                net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
            }

            memcpy( &pLCSCONN->bInToken, &pOutBAF1->bTokenB, sizeof(pLCSCONN->bInToken) );  // Copy Inbound token
            obtain_lock( &TokenLock );
            STORE_FW( pLCSCONN->bOutToken, uToken );                                        // Outbound token
            uToken += INCREMENT_TOKEN;
            release_lock( &TokenLock );

            if (pLCSDEV->pLCSBLK->fDebug)
            {
                WRMSG( HHC03984, "I", "Updated LCSCONN Inbound");
                net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
            }
        }
        else  // The existing LCSCONN is for an outbound connection.
        {
            if (pLCSDEV->pLCSBLK->fDebug)
            {
                WRMSG( HHC03984, "E", "Found and released existing LCSCONN Outbound");
                net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
            }

            remove_connection_from_chain( pLCSDEV, pLCSCONN );
            free_connection( pLCSDEV, pLCSCONN );
            pLCSCONN = NULL;

            // Create the LCSCONN for the new outbound connection.
            pLCSCONN = alloc_connection( pLCSDEV );
            pLCSCONN->hwCreated = LCSCONN_CREATED_OUTBOUND;

            memcpy( &pLCSCONN->bLocalMAC, &pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
            pLCSCONN->bLocalMAC[5]++;
#endif
            memcpy( &pLCSCONN->bRemoteMAC, &pOutBAF2->bByte03, IFHWADDRLEN );

            memcpy( &pLCSCONN->bInToken, &pOutBAF1->bTokenB, sizeof(pLCSCONN->bInToken) );  // Copy Inbound token
            obtain_lock( &TokenLock );
            STORE_FW( pLCSCONN->bOutToken, uToken );                                        // Outbound token
            uToken += INCREMENT_TOKEN;
            release_lock( &TokenLock );

            if (pLCSDEV->pLCSBLK->fDebug)
            {
                WRMSG( HHC03984, "I", "Created LCSCONN Outbound");
                net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
            }

            add_connection_to_chain( pLCSDEV, pLCSCONN );
        }
    }

    //
    pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_CC0A_SIZE * 2 ) );

    memcpy( &pLCSIBH->bData, Inbound_CC0A, INBOUND_CC0A_SIZE );
    pLCSIBH->iDataLen = INBOUND_CC0A_SIZE;

    pInHDR = (PLCSHDR)&pLCSIBH->bData;
    pInBAF1 = (PLCSBAF1)( (BYTE*)pInHDR + sizeof(LCSHDR) );
    FETCH_HW( hwLenInBaf1, pInBAF1->hwLenBaf1 );
//  FETCH_HW( hwLenInBaf2, pInBAF1->hwLenBaf2 );
    pInBAF2 = (PLCSBAF2)( (BYTE*)pInBAF1 + hwLenInBaf1 );

    memcpy( &pInBAF1->bTokenA, &pLCSCONN->bInToken, sizeof(pLCSCONN->bInToken) );        // Set Inbound token
    memcpy( &pInBAF1->bTokenC, &pLCSCONN->bOutToken, sizeof(pLCSCONN->bOutToken) );      // Outbound token

    memcpy( &pInBAF2->hwSeqNum, &pOutBAF2->hwSeqNum, sizeof(pInBAF2->hwSeqNum) );

    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    return;
}


// ====================================================================
//                       Process_0C25
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 13 (0x0D) and 27 (0x1B) bytes in length.
//
void Process_0C25 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2)
{
//                   Token
//  000D0C25001B6004 400000DC 00
//  0 1 2 3 4 5 6 7  8 9 A B  C
//                         DMAC         SMAC         LLC    Filler
//  0100000000030000000000 000CCE4B4740 400074700001 000400 xx
//  0 1 2 3 4 5 6 7 8 9 A  B C D E F 0  1 2 3 4 5 6  7 8 9

    DEVBLK*   pDEVBLK;
    PLCSPORT  pLCSPORT;
    PLCSCONN  pLCSCONN;
    int       iLPDULen;
    LLC       llc;
    PETHFRM   pEthFrame;
    int       iEthLen;
    BYTE      frame[64];
    char      llcmsg[256];

    UNREFERENCED( pLCSHDR   );
    UNREFERENCED( pLCSBAF1  );
    UNREFERENCED( hwLenBaf1 );
    UNREFERENCED( hwLenBaf2 );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    memset( frame, 0, sizeof(frame) );                               // Clear area for ethernet fram
    pEthFrame = (PETHFRM)&frame[0];
    iEthLen = 60;                                                    // Minimum ethernet frame length

    // Find the connection block.
    pLCSCONN = find_connection_by_remote_mac( pLCSDEV, (MAC*)&pLCSBAF2->bByte11 );
    if (!pLCSCONN)
    {
        WRMSG( HHC03984, "W", "LCSCONN not found");
        /* FixMe! Need a proper error message here! */
        return;
    }

    //
    memset( &llc, 0, sizeof(LLC) );
    llc.hwDSAP    = pLCSBAF2->bByte23;                               // Copy LLC DSAP
    llc.hwSSAP    = pLCSBAF2->bByte24;                               // Copy LLC SSAP
    llc.hwPF      = 1;
    llc.hwM       = M_TEST_Command_or_Response;
    llc.hwType    = Type_Unnumbered_Frame;

    // Construct Ethernet frame
    memcpy( &pEthFrame->bDestMAC, &pLCSBAF2->bByte11, IFHWADDRLEN ); // Copy destination MAC address
    memcpy( &pEthFrame->bSrcMAC, &pLCSBAF2->bByte17, IFHWADDRLEN );  // Copy source MAC address
    iLPDULen = BuildLLC( &llc, pEthFrame->bData);                    // Build LLC PDU
    STORE_HW( pEthFrame->hwEthernetType, (U16)iLPDULen );            // Set data length

    // Trace Ethernet frame before sending to TAP device
    if (pLCSPORT->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
        WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             pLCSDEV->bPort, iEthLen, "802.3 SNA", pLCSPORT->szNetIfName );
        net_data_trace( pDEVBLK, (BYTE*)pEthFrame, iEthLen, FROM_GUEST, 'D', "eth frame", 0 );
    }

    // Write the Ethernet frame to the TAP device
    if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrame, iEthLen ) == iEthLen)
    {
        if (pLCSPORT->pLCSBLK->fDebug)
        {
            snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame sent: CR=%u, M=%s", llc.hwCR, "TEST" );
            WRMSG(HHC03984, "D", llcmsg );  /* FixMe! */
        }
    }
    else
    {
        pLCSDEV->iTuntapErrno = errno;
        pLCSDEV->fTuntapError = TRUE;
        PTT_TIMING( "*WRITE ERR", 0, iEthLen, 1 );
    }

    return;
}


// ====================================================================
//                       Process_0C22
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 13 (0x0D) and 26 (0x1A) or more bytes in length.
//
void Process_0C22 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2)
{
//                   Token
//  000D0C22001A6004 400000DC 00
//  0 1 2 3 4 5 6 7  8 9 A B  C
//                         DMAC         SMAC         LLC ?  TH etc.
//  0100000000030408000000 000CCE4B4740 400074700001 040400 xxxxxxxx...........
//  0 1 2 3 4 5 6 7 8 9 A  B C D E F 0  1 2 3 4 5 6  7 8 9  A B C D E F 0 1 ...
//  0100000000030808000000 080027195BFE 400074700001 040400 xxxxxxxx...........

    DEVBLK*   pDEVBLK;
    PLCSPORT  pLCSPORT;
    PLCSCONN  pLCSCONN;
    int       iLPDULen;
    LLC       llc;
    PETHFRM   pEthFrame;
    int       iEthLen;
    int       iXIDlen;
    int       iTraceLen;
    BYTE      frame[512];
    char      llcmsg[256];

    UNREFERENCED( pLCSHDR   );
    UNREFERENCED( pLCSBAF1  );
    UNREFERENCED( hwLenBaf1 );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    memset( frame, 0, sizeof(frame) );                               // Clear area for ethernet fram
    pEthFrame = (PETHFRM)&frame[0];
    iEthLen = 60;                                                    // Minimum ethernet frame length

    // Find the connection block.
    pLCSCONN = find_connection_by_remote_mac( pLCSDEV, (MAC*)&pLCSBAF2->bByte11 );
    if (!pLCSCONN)
    {
        WRMSG( HHC03984, "W", "LCSCONN not found");
        /* FixMe! Need a proper error message here! */
        return;
    }

    //
    memset( &llc, 0, sizeof(LLC) );
    llc.hwDSAP    = pLCSBAF2->bByte23;                               // Copy LLC DSAP
    llc.hwSSAP    = pLCSBAF2->bByte24;                               // Copy LLC SSAP
    if ( pLCSCONN->hwCreated == LCSCONN_CREATED_INBOUND )
    {
        llc.hwCR  = 1;
    }
    llc.hwPF      = 1;
    llc.hwM       = M_XID_Command_or_Response;
    llc.hwType    = Type_Unnumbered_Frame;

    // Construct Ethernet frame
    memcpy( &pEthFrame->bDestMAC, &pLCSBAF2->bByte11, IFHWADDRLEN ); // Copy destination MAC address
    memcpy( &pEthFrame->bSrcMAC, &pLCSBAF2->bByte17, IFHWADDRLEN );  // Copy source MAC address
    iLPDULen = BuildLLC( &llc, pEthFrame->bData);                    // Build LLC PDU
    STORE_HW( pEthFrame->hwEthernetType, (U16)iLPDULen );            // Set data length

    // Continue Ethernet frame construction if there is an XID0 or an XID3 and CV's.
    iXIDlen = ( hwLenBaf2 - 26 );                                    // Calculate length of XID0 or XID3 and CV's
    if ( iXIDlen > 0 )                                               // Any XID0 or XID3 an CV's?
    {
        STORE_HW( pEthFrame->hwEthernetType, (U16)( iLPDULen + iXIDlen ) );    // Set LLC and XID0 or XID3 and CV's length
        memcpy( &pEthFrame->bData[iLPDULen], &pLCSBAF2->bByte26, iXIDlen );    // Copy XID0 or XID3 and CV's.
        if ( iEthLen < ((IFHWADDRLEN * 2) + 2 + iLPDULen + iXIDlen) )
        {
            iEthLen = ((IFHWADDRLEN * 2) + 2 + iLPDULen + iXIDlen);
        }
    }

    // Trace Ethernet frame before sending to TAP device
    if (pLCSPORT->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
        WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             pLCSDEV->bPort, iEthLen, "802.3 SNA", pLCSPORT->szNetIfName );
        if (pLCSPORT->pLCSBLK->iTraceLen)
        {
            iTraceLen = iEthLen;
            if (iTraceLen > pLCSPORT->pLCSBLK->iTraceLen)
            {
                iTraceLen = pLCSPORT->pLCSBLK->iTraceLen;
                // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, (iEthLen - iTraceLen) );
            }
            net_data_trace( pDEVBLK, (BYTE*)pEthFrame, iTraceLen, FROM_GUEST, 'D', "eth frame", 0 );
        }
    }

    // Write the Ethernet frame to the TAP device
    if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrame, iEthLen ) == iEthLen)
    {
        if (pLCSPORT->pLCSBLK->fDebug)
        {
            snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame sent: CR=%u, M=%s", llc.hwCR, "XID" );
            WRMSG(HHC03984, "D", llcmsg );  /* FixMe! */
        }
    }
    else
    {
        pLCSDEV->iTuntapErrno = errno;
        pLCSDEV->fTuntapError = TRUE;
        PTT_TIMING( "*WRITE ERR", 0, iEthLen, 1 );
    }

    return;
}


// ====================================================================
//                       Process_8D00
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 16 (0x10) and 26 (0x1A) bytes in length.
//
//  +0000  00108D00 001A6002 40000240 00000000  ......`.@..@.... ......-. .. ....
//
void Process_8D00 (PLCSDEV pLCSDEV, PLCSHDR pLCSHDR, PLCSBAF1 pLCSBAF1, PLCSBAF2 pLCSBAF2, U16 hwLenBaf1, U16 hwLenBaf2)
{
//                   Token
//  00108D00001A6002 40000240 00000000
//  0 1 2 3 4 5 6 7  8 9 A B  C D E F
//
//  0100000000000000000000000000000000000000000000000000
//  0 1 2 3 4 5 6 7 8 9 A B C D E F 0 1 2 3 4 5 6 7 8 9

    DEVBLK*   pDEVBLK;
    PLCSPORT  pLCSPORT;
    PLCSCONN  pLCSCONN;
    PETHFRM   pEthFrame;
    int       iEthLen;
    int       iLPDULen;
    LLC       llc;
    BYTE      frame[64];
    char      llcmsg[256];

    UNREFERENCED( pLCSHDR   );
    UNREFERENCED( pLCSBAF1  );
    UNREFERENCED( pLCSBAF2  );
    UNREFERENCED( hwLenBaf1 );
    UNREFERENCED( hwLenBaf2 );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    memset( frame, 0, sizeof(frame) );                               // Clear area for ethernet fram
    pEthFrame = (PETHFRM)&frame[0];
    iEthLen = 60;                                                    // Minimum ethernet frame length

    // Find the connection block.
    pLCSCONN = find_connection_by_outbound_token( pLCSDEV, pLCSBAF1->bTokenA );
    if (!pLCSCONN)
    {
        WRMSG( HHC03984, "W", "LCSCONN not found");
        /* FixMe! Need a proper error message here! */
        return;
    }

    pLCSCONN->fIframe = FALSE;

    //
    memset( &llc, 0, sizeof(LLC) );
    llc.hwDSAP    = LSAP_SNA_Path_Control;
    llc.hwSSAP    = LSAP_SNA_Path_Control;
    llc.hwCR      = 1;
    llc.hwPF      = 1;
    llc.hwM       = M_UA_Response;
    llc.hwType    = Type_Unnumbered_Frame;

    // Construct Ethernet frame
    memcpy( &pEthFrame->bDestMAC, &pLCSCONN->bRemoteMAC, IFHWADDRLEN );   // Copy destination MAC address
    memcpy( &pEthFrame->bSrcMAC, &pLCSCONN->bLocalMAC, IFHWADDRLEN );     // Copy source MAC address
    iLPDULen = BuildLLC( &llc, pEthFrame->bData);                         // Build LLC PDU
    STORE_HW( pEthFrame->hwEthernetType, (U16)iLPDULen );                 // Set data length

    // Trace Ethernet frame before sending to TAP device
    if (pLCSPORT->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
        WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             pLCSDEV->bPort, iEthLen, "802.3 SNA", pLCSPORT->szNetIfName );
        net_data_trace( pDEVBLK, (BYTE*)pEthFrame, iEthLen, FROM_GUEST, 'D', "eth frame", 0 );
    }

    // Write the Ethernet frame to the TAP device
    if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrame, iEthLen ) == iEthLen)
    {
        if (pLCSPORT->pLCSBLK->fDebug)
        {
            snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame sent: CR=%u, M=%s", llc.hwCR, "UA" );
            WRMSG(HHC03984, "D", llcmsg );  /* FixMe! */
        }
    }
    else
    {
        pLCSDEV->iTuntapErrno = errno;
        pLCSDEV->fTuntapError = TRUE;
        PTT_TIMING( "*WRITE ERR", 0, iEthLen, 1 );
    }

    return;
}


// ====================================================================
//                       Process_0C0B
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 17 (0x11) and 7 (0x07) bytes in length.
//
// The inbound LCSBAF1 and LCSBAF2 returned to VTAM are, respectively,
// 24 (0x18) and 3 (0x03) bytes in length.
//
void Process_0C0B (PLCSDEV pLCSDEV, PLCSHDR pOutHDR, PLCSBAF1 pOutBAF1, PLCSBAF2 pOutBAF2, U16 hwLenOutBaf1, U16 hwLenOutBaf2)
{
//                   Token
//  00110C0B00076002 40000240 0000000000
//  0 1 2 3 4 5 6 7  8 9 A B  C
//
//  01000900000000
//  0 1 2 3 4 5 6

#define INBOUND_CC0B_SIZE 32
static const BYTE Inbound_CC0B[INBOUND_CC0B_SIZE] =
                 {
                   0x00, 0x20, 0x04, 0x00,                           /* LCSHDR  */
                   0x00, 0x18, 0xCC, 0x0B, 0x00, 0x03, 0x60, 0x01,   /* LCSBAF1 */
                   0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x01, 0xff, 0xff,                                 /* LCSBAF2 */
                   0x00                                              /* Filler  */
                 };

    PLCSCONN    pLCSCONN;
    PLCSIBH     pLCSIBH;
    PLCSHDR     pInHDR;
    PLCSBAF1    pInBAF1;
    PLCSBAF2    pInBAF2;
    U16         hwLenInBaf1;
//  U16         hwLenInBaf2;

    UNREFERENCED( pOutHDR      );
    UNREFERENCED( pOutBAF1     );
    UNREFERENCED( hwLenOutBaf1 );
    UNREFERENCED( hwLenOutBaf2 );

    // Find the connection block.
    // I wish I knew what bByte07 == 0x02 or 0x04 actually meant!
    if (pOutBAF1->bByte07 == 0x02)
    {
        pLCSCONN = find_connection_by_outbound_token( pLCSDEV, pOutBAF1->bTokenA );
    }
    else if (pOutBAF1->bByte07 == 0x04)
    {
        pLCSCONN = find_connection_by_inbound_token( pLCSDEV, pOutBAF1->bTokenB );
    }
    else
    {
        pLCSCONN = NULL;
    }
    if (!pLCSCONN)
    {
        WRMSG( HHC03984, "W", "LCSCONN not found");
        /* FixMe! Need a proper error message here! */
        return;
    }

    //
    pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_CC0B_SIZE * 2 ) );

    memcpy( &pLCSIBH->bData, Inbound_CC0B, INBOUND_CC0B_SIZE );
    pLCSIBH->iDataLen = INBOUND_CC0B_SIZE;

    pInHDR = (PLCSHDR)&pLCSIBH->bData;
    pInBAF1 = (PLCSBAF1)( (BYTE*)pInHDR + sizeof(LCSHDR) );
    FETCH_HW( hwLenInBaf1, pInBAF1->hwLenBaf1 );
//  FETCH_HW( hwLenInBaf2, pInBAF1->hwLenBaf2 );
    pInBAF2 = (PLCSBAF2)( (BYTE*)pInBAF1 + hwLenInBaf1 );

    memcpy( &pInBAF1->bTokenA, &pLCSCONN->bInToken, sizeof(pLCSCONN->bInToken) );        // Set Inbound token

    memcpy( &pInBAF2->hwSeqNum, &pOutBAF2->hwSeqNum, sizeof(pInBAF2->hwSeqNum) );

    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    remove_connection_from_chain( pLCSDEV, pLCSCONN );
    free_connection( pLCSDEV, pLCSCONN );

    return;
}


// ====================================================================
//                       Process_0C99                                                                                                                                                      #
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 12 (0x0C) and 3 (0x03) bytes in length.
//
// The inbound LCSBAF1 and LCSBAF2 returned to VTAM are, respectively,
// 24 (0x18) and 21 (0x15) bytes in length.
//
void Process_0C99 (PLCSDEV pLCSDEV, PLCSHDR pOutHDR, PLCSBAF1 pOutBAF1, PLCSBAF2 pOutBAF2, U16 hwLenOutBaf1, U16 hwLenOutBaf2)
{
#define INBOUND_CC99_SIZE 50
static const BYTE Inbound_CC99[INBOUND_CC99_SIZE] =
                 {
                   0x00, 0x32, 0x04, 0x00,                           /* LCSHDR  */
                   0x00, 0x18, 0xCC, 0x99, 0x00, 0x15, 0xC0, 0x00,   /* LCSBAF1 */
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x20, 0x1E, 0x00, 0x00, 0x00, 0x00,
                   0x01, 0xff, 0xff, 0x00, 0x00, 0x03, 0x00, 0x00,   /* LCSBAF2 */
                   0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
                   0x00, 0x05, 0xD5, 0x00, 0x00,                                   /* 0x5D5 = 1493 */
                   0x00                                              /* Filler  */
                 };

    PLCSBLK     pLCSBLK;
    PLCSPORT    pLCSPORT;
    PLCSIBH     pLCSIBH;
    PLCSHDR     pInHDR;
    PLCSBAF1    pInBAF1;
    PLCSBAF2    pInBAF2;
    U16         hwLenInBaf1;
//  U16         hwLenInBaf2;
    BYTE*       pInMAC;

    UNREFERENCED( pOutHDR      );
    UNREFERENCED( pOutBAF1     );
    UNREFERENCED( hwLenOutBaf1 );
    UNREFERENCED( hwLenOutBaf2 );

    pLCSBLK = pLCSDEV->pLCSBLK;
    pLCSPORT = &pLCSBLK->Port[ pLCSDEV->bPort ];

    pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_CC99_SIZE * 2 ) );

    memcpy( &pLCSIBH->bData, Inbound_CC99, INBOUND_CC99_SIZE );
    pLCSIBH->iDataLen = INBOUND_CC99_SIZE;

    pInHDR = (PLCSHDR)&pLCSIBH->bData;
    pInBAF1 = (PLCSBAF1)( (BYTE*)pInHDR + sizeof(LCSHDR) );
    FETCH_HW( hwLenInBaf1, pInBAF1->hwLenBaf1 );
//  FETCH_HW( hwLenInBaf2, pInBAF1->hwLenBaf2 );
    pInBAF2 = (PLCSBAF2)( (BYTE*)pInBAF1 + hwLenInBaf1 );

    memcpy( &pInBAF2->hwSeqNum, &pOutBAF2->hwSeqNum, sizeof(pInBAF2->hwSeqNum) );

    pInMAC = (BYTE*)pInBAF2;
    memcpy( pInMAC+9, pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
    pInMAC[9+5]++;
#endif

    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    return;
}


// ====================================================================
//                       Process_0C0D
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 34 (0x22) and 44 (0x2C) bytes in length.
//
// The inbound LCSBAF1 and LCSBAF2 returned to VTAM are, respectively,
// 44 (0x2C) and 7 (0x07) bytes in length.
//
void Process_0C0D (PLCSDEV pLCSDEV, PLCSHDR pOutHDR, PLCSBAF1 pOutBAF1, PLCSBAF2 pOutBAF2, U16 hwLenOutBaf1, U16 hwLenOutBaf2)
{
#define INBOUND_CC0D_SIZE 56
static const BYTE Inbound_CC0D[INBOUND_CC0D_SIZE] =
                 {
                   0x00, 0x38, 0x04, 0x00,                           /* LCSHDR  */
                   0x00, 0x2C, 0xCC, 0x0D, 0x00, 0x07, 0x60, 0x03,   /* LCSBAF1 */
                   0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x40, 0x00, 0x00, 0xDC, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00,
                   0x01, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,         /* LCSBAF2 */
                   0x00                                              /* Filler  */
                 };

    PLCSIBH     pLCSIBH;
    PLCSHDR     pInHDR;
    PLCSBAF1    pInBAF1;
    PLCSBAF2    pInBAF2;
    U16         hwLenInBaf1;
//  U16         hwLenInBaf2;

    UNREFERENCED( pOutHDR      );
    UNREFERENCED( pOutBAF1     );
    UNREFERENCED( hwLenOutBaf1 );
    UNREFERENCED( hwLenOutBaf2 );

    pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_CC0D_SIZE * 2 ) );

    memcpy( &pLCSIBH->bData, Inbound_CC0D, INBOUND_CC0D_SIZE );
    pLCSIBH->iDataLen = INBOUND_CC0D_SIZE;

    pInHDR = (PLCSHDR)&pLCSIBH->bData;
    pInBAF1 = (PLCSBAF1)( (BYTE*)pInHDR + sizeof(LCSHDR) );
    FETCH_HW( hwLenInBaf1, pInBAF1->hwLenBaf1 );
//  FETCH_HW( hwLenInBaf2, pInBAF1->hwLenBaf2 );
    pInBAF2 = (PLCSBAF2)( (BYTE*)pInBAF1 + hwLenInBaf1 );

    memcpy( &pInBAF2->hwSeqNum, &pOutBAF2->hwSeqNum, sizeof(pInBAF2->hwSeqNum) );

    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    return;
}


// ====================================================================
//                       Process_0C0E
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 20 (0x14) and 3 (0x03) bytes in length.
//
// The inbound LCSBAF1 and LCSBAF2 returned to VTAM are, respectively,
// 24 (0x18) and 3 (0x03) bytes in length.
//
void Process_0C0E (PLCSDEV pLCSDEV, PLCSHDR pOutHDR, PLCSBAF1 pOutBAF1, PLCSBAF2 pOutBAF2, U16 hwLenOutBaf1, U16 hwLenOutBaf2)
{
#define INBOUND_CC0E_SIZE 32
static const BYTE Inbound_CC0E[INBOUND_CC0E_SIZE] =
                 {
                   0x00, 0x20, 0x04, 0x00,                           /* LCSHDR  */
                   0x00, 0x18, 0xCC, 0x0E, 0x00, 0x03, 0x60, 0x03,   /* LCSBAF1 */
                   0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x01, 0xff, 0xff,                                 /* LCSBAF2 */
                   0x00,                                             /* Filler  */
                 };

    PLCSIBH     pLCSIBH;
    PLCSHDR     pInHDR;
    PLCSBAF1    pInBAF1;
    PLCSBAF2    pInBAF2;
    U16         hwLenInBaf1;
//  U16         hwLenInBaf2;

    UNREFERENCED( pOutHDR      );
    UNREFERENCED( pOutBAF1     );
    UNREFERENCED( hwLenOutBaf1 );
    UNREFERENCED( hwLenOutBaf2 );

    pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_CC0E_SIZE * 2 ) );

    memcpy( &pLCSIBH->bData, Inbound_CC0E, INBOUND_CC0E_SIZE );
    pLCSIBH->iDataLen = INBOUND_CC0E_SIZE;

    pInHDR = (PLCSHDR)&pLCSIBH->bData;
    pInBAF1 = (PLCSBAF1)( (BYTE*)pInHDR + sizeof(LCSHDR) );
    FETCH_HW( hwLenInBaf1, pInBAF1->hwLenBaf1 );
//  FETCH_HW( hwLenInBaf2, pInBAF1->hwLenBaf2 );
    pInBAF2 = (PLCSBAF2)( (BYTE*)pInBAF1 + hwLenInBaf1 );

    memcpy( &pInBAF2->hwSeqNum, &pOutBAF2->hwSeqNum, sizeof(pInBAF2->hwSeqNum) );

    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    return;
}


// ====================================================================
//                       Process_0C98
// ====================================================================
//
// The outbound LCSBAF1 and LCSBAF2 arriving from VTAM are usually,
// respectively, 20 (0x14) and 3 (0x03) bytes in length.
//
// The inbound LCSBAF1 and LCSBAF2 returned to VTAM are, respectively,
// 24 (0x18) and 3 (0x03) bytes in length.
//
void Process_0C98 (PLCSDEV pLCSDEV, PLCSHDR pOutHDR, PLCSBAF1 pOutBAF1, PLCSBAF2 pOutBAF2, U16 hwLenOutBaf1, U16 hwLenOutBaf2)
{
#define INBOUND_CC98_SIZE 32
static const BYTE Inbound_CC98[INBOUND_CC98_SIZE] =
                 {
                   0x00, 0x20, 0x04, 0x00,                           /* LCSHDR  */
                   0x00, 0x18, 0xCC, 0x98, 0x00, 0x03, 0xC0, 0x00,   /* LCSBAF1 */
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x01, 0xff, 0xff,                                 /* LCSBAF2 */
                   0x00                                              /* Filler  */
                 };

    PLCSIBH     pLCSIBH;
    PLCSHDR     pInHDR;
    PLCSBAF1    pInBAF1;
    PLCSBAF2    pInBAF2;
    U16         hwLenInBaf1;
//  U16         hwLenInBaf2;

    UNREFERENCED( pOutHDR      );
    UNREFERENCED( pOutBAF1     );
    UNREFERENCED( hwLenOutBaf1 );
    UNREFERENCED( hwLenOutBaf2 );

    pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_CC98_SIZE * 2 ) );

    memcpy( &pLCSIBH->bData, Inbound_CC98, INBOUND_CC98_SIZE );
    pLCSIBH->iDataLen = INBOUND_CC98_SIZE;

    pInHDR = (PLCSHDR)&pLCSIBH->bData;
    pInBAF1 = (PLCSBAF1)( (BYTE*)pInHDR + sizeof(LCSHDR) );
    FETCH_HW( hwLenInBaf1, pInBAF1->hwLenBaf1 );
//  FETCH_HW( hwLenInBaf2, pInBAF1->hwLenBaf2 );
    pInBAF2 = (PLCSBAF2)( (BYTE*)pInBAF1 + hwLenInBaf1 );

    memcpy( &pInBAF2->hwSeqNum, &pOutBAF2->hwSeqNum, sizeof(pInBAF2->hwSeqNum) );

    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    return;
}


// ====================================================================
//                         LCS_StartLan_SNA
// ====================================================================

static void  LCS_StartLan_SNA( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{

#define REPLY_STARTLAN_SNA_SIZE sizeof(LCSSTRTFRM)

    PLCSPORT    pLCSPORT;
    DEVBLK*     pDEVBLK;
    PLCSIBH     pLCSIBH;
    PLCSSTRTFRM pLCSSTRTFRM;
    int         nIFFlags;
    U8          fStartPending = 0;


    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];
    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];  /* SNA has only one device */

    // Get a buffer to hold the reply.
    pLCSIBH = alloc_lcs_buffer( pLCSDEV, REPLY_STARTLAN_SNA_SIZE + 10 );
    pLCSSTRTFRM = (PLCSSTRTFRM)&pLCSIBH->bData;
    pLCSIBH->iDataLen = REPLY_STARTLAN_SNA_SIZE;

    // Note: the command is usually 22 (0x16) bytes in length, whereas the
    // reply needs to be 24 (0x18) bytes in length. INIT_REPLY_FRAME will
    // set pLCSIBH->iDataLen equal to the length of the command.
    INIT_REPLY_FRAME( pLCSSTRTFRM, pLCSIBH->iDataLen, pCmdFrame, iCmdLen );
    pLCSIBH->iDataLen = REPLY_STARTLAN_SNA_SIZE;

    // Initialize the reply fields.
    pLCSSTRTFRM->bLCSCmdHdr.bLCSHdr.bSlot = pLCSDEV->bPort;
    pLCSSTRTFRM->bLCSCmdHdr.bInitiator    = LCS_INITIATOR_SNA;
    pLCSSTRTFRM->bLCSCmdHdr.bRelAdapterNo = pLCSDEV->bPort;
    STORE_HW( pLCSSTRTFRM->hwBufferSize, 0x0200 );                        /* >= 0x0200, == 0x02FF */
//  STORE_FW( pLCSSTRTFRM->fwReadLength, pLCSDEV->iMaxFrameBufferSize );  /* Length for Read CCW (0x0800 to 0xFFFF) */
    STORE_FW( pLCSSTRTFRM->fwReadLength, CTC_DEF_FRAME_BUFFER_SIZE );     /* Length for Read CCW (0x0800 to 0xFFFF) */

    // Add the buffer containing the reply to the chain.
    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    // Serialize access to eliminate ioctl errors
    PTT_DEBUG(        "GET  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    obtain_lock( &pLCSPORT->PortDataLock );
    PTT_DEBUG(        "GOT  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    {
        // Configure the TAP interface if used
        PTT_DEBUG( "STRTLAN if started", pLCSPORT->fPortStarted, pDEVBLK->devnum, pLCSPORT->bPort );
        if (pLCSPORT->fUsed && pLCSPORT->fPortCreated && !pLCSPORT->fPortStarted)
        {
            PTT_DEBUG( "STRTLAN started=NO", 000, pDEVBLK->devnum, pLCSPORT->bPort );
            nIFFlags =              // Interface flags
                0
                | IFF_UP            // (interface is being enabled)
                | IFF_BROADCAST     // (interface broadcast addr is valid)
                ;

#if defined( TUNTAP_IFF_RUNNING_NEEDED )

            nIFFlags |=             // ADDITIONAL Interface flags
                0
                | IFF_RUNNING       // (interface is ALSO operational)
                ;

#endif /* defined( TUNTAP_IFF_RUNNING_NEEDED ) */

            // Enable the interface by turning on the IFF_UP flag...
            // This lets the packets start flowing...

            if (!pLCSPORT->fPreconfigured)
                VERIFY( TUNTAP_SetFlags( pLCSPORT->szNetIfName, nIFFlags ) == 0 );

            fStartPending = 1;
        }
    }
    PTT_DEBUG(         "REL  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    release_lock( &pLCSPORT->PortDataLock );

    if (fStartPending)
        UpdatePortStarted( TRUE, pDEVBLK, pLCSPORT );

    pLCSDEV->fDevStarted = 1;

}

// ====================================================================
//                         LCS_LanStats_SNA
// ====================================================================

static void  LCS_LanStats_SNA( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{

#define REPLY_LANSTATS_SNA_SIZE sizeof(LCSLSSFRM)

    PLCSPORT    pLCSPORT;
    PLCSIBH     pLCSIBH;
    PLCSLSSFRM  pLCSLSSFRM;


    pLCSPORT = &pLCSDEV->pLCSBLK->Port[pLCSDEV->bPort];

    // Get and display the tap interface MAC address.
    GetIfMACAddress( pLCSPORT );

    // Get a buffer to hold the reply.
    pLCSIBH = alloc_lcs_buffer( pLCSDEV, REPLY_LANSTATS_SNA_SIZE + 10 );
    pLCSLSSFRM = (PLCSLSSFRM)&pLCSIBH->bData;
    pLCSIBH->iDataLen = REPLY_LANSTATS_SNA_SIZE;

    // Note: the command is usually 20 (0x14) bytes in length, whereas the
    // reply needs to be 26 (0x1A) bytes in length. INIT_REPLY_FRAME will
    // set pLCSIBH->iDataLen equal to the length of the command.
    INIT_REPLY_FRAME( pLCSLSSFRM, pLCSIBH->iDataLen, pCmdFrame, iCmdLen );
    pLCSIBH->iDataLen = REPLY_LANSTATS_SNA_SIZE;

    // Initialize the reply fields.
    // Respond with a different MAC address for the guest
    // side unless the TAP mechanism is designed as such
    // cf : hostopts.h for an explanation
    pLCSLSSFRM->bLCSCmdHdr.bLCSHdr.bSlot = pLCSDEV->bPort;
    pLCSLSSFRM->bLCSCmdHdr.bInitiator    = LCS_INITIATOR_SNA;
    pLCSLSSFRM->bLCSCmdHdr.bRelAdapterNo = pLCSDEV->bPort;
    pLCSLSSFRM->bUnknown1                = 0x01;  /* Count? Number of MAC addresses? */
    pLCSLSSFRM->bUnknown2                = 0x04;  /* This byte is kept by VTAM. SAP? Probably not. 0x04 works, 0x08 doesn't. */
    pLCSLSSFRM->bUnknown3                = 0x00;  /* This byte is kept by VTAM. */
    pLCSLSSFRM->bUnknown4                = 0x01;  /* No idea! */
    pLCSLSSFRM->bMACsize                 = 0x06;  /* Length of MAC address */
    memcpy( pLCSLSSFRM->MAC_Address, pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
    pLCSLSSFRM->MAC_Address[5]++;
#endif

    // Add the buffer containing the reply to the chain.
    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

}

// ====================================================================
//                         LCS_StopLan_SNA
// ====================================================================

static void  LCS_StopLan_SNA( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{

#define REPLY_STOPLAN_SNA_SIZE sizeof(LCSSTDFRM)

    PLCSPORT    pLCSPORT;
    DEVBLK*     pDEVBLK;
    PLCSIBH     pLCSIBH;
    PLCSSTDFRM  pLCSSTDFRM;


    pLCSPORT = &pLCSDEV->pLCSBLK->Port[ pLCSDEV->bPort ];
    pDEVBLK  =  pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];  /* SNA has only one device */

    // Get a buffer to hold the reply.
    pLCSIBH = alloc_lcs_buffer( pLCSDEV, REPLY_STOPLAN_SNA_SIZE + 10 );
    pLCSSTDFRM = (PLCSSTDFRM)&pLCSIBH->bData;
    pLCSIBH->iDataLen = REPLY_STOPLAN_SNA_SIZE;

    // Note: the command is usually 20 (0x14) bytes in length, and the
    // reply needs to be 20 (0x14) bytes in length. INIT_REPLY_FRAME will
    // leave pLCSIBH->iDataLen unchanged.
    INIT_REPLY_FRAME( pLCSSTDFRM, pLCSIBH->iDataLen, pCmdFrame, iCmdLen );
    pLCSIBH->iDataLen = REPLY_STOPLAN_SNA_SIZE;

    // Initialize the reply fields.
    pLCSSTDFRM->bLCSCmdHdr.bLCSHdr.bSlot = pLCSDEV->bPort;
    pLCSSTDFRM->bLCSCmdHdr.bInitiator    = LCS_INITIATOR_SNA;
    pLCSSTDFRM->bLCSCmdHdr.bRelAdapterNo = pLCSDEV->bPort;

    // Serialize access to eliminate ioctl errors
    PTT_DEBUG(        "GET  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    obtain_lock( &pLCSPORT->PortDataLock );
    PTT_DEBUG(        "GOT  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    {
        // Disable the interface by turning off the IFF_UP flag...
        if (!pLCSPORT->fPreconfigured)
            VERIFY( TUNTAP_SetFlags( pLCSPORT->szNetIfName, 0 ) == 0 );

    }
    PTT_DEBUG(         "REL  PortDataLock ", 000, pDEVBLK->devnum, pLCSPORT->bPort );
    release_lock( &pLCSPORT->PortDataLock );

    // Now that the tuntap device has been stopped and the packets
    // are no longer flowing, tell the LCS_PortThread to stop trying
    // to read from the tuntap device (adapter).

    UpdatePortStarted( FALSE, pDEVBLK, pLCSPORT );

    // Now that we've stopped new packets from being added to our
    // frame buffer we can now finally enqueue our reply frame
    // to our frame buffer (so LCS_Read can return it to the guest).
    // However, before we do that we must reset the frame buffer to
    // an empty state, losing any waiting input (though there
    // shouldn't be any!) and ensuring the enqueue proceeds smoothly.
    PTT_DEBUG(       "GET  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
    obtain_lock( &pLCSDEV->DevDataLock );
    PTT_DEBUG(       "GOT  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
    pLCSDEV->iFrameOffset  = 0;
    pLCSDEV->fReplyPending = 0;
    pLCSDEV->fDataPending  = 0;
    pLCSDEV->fPendingIctl  = 0;
    PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
    release_lock( &pLCSDEV->DevDataLock );
    remove_and_free_any_lcs_buffers_on_chain( pLCSDEV );
    remove_and_free_any_connections_on_chain( pLCSDEV );

    // Add the buffer containing the reply to the chain.
    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

    pLCSDEV->fDevStarted = 0;
}

// ====================================================================
//                       LCS_UnsuppCmd_SNA
// ====================================================================

static void  LCS_UnsuppCmd_SNA( PLCSDEV pLCSDEV, PLCSCMDHDR pCmdFrame, int iCmdLen )
{

#define REPLY_UNSUPPCMD_SNA_SIZE sizeof(LCSSTDFRM)

    PLCSIBH     pLCSIBH;
    PLCSSTDFRM  pLCSSTDFRM;


    // Get a buffer to hold the reply.
    pLCSIBH = alloc_lcs_buffer( pLCSDEV, REPLY_UNSUPPCMD_SNA_SIZE + 10 );
    pLCSSTDFRM = (PLCSSTDFRM)&pLCSIBH->bData;
    pLCSIBH->iDataLen = REPLY_UNSUPPCMD_SNA_SIZE;

    // Note: the command will be some unpredictable length, and the
    // reply will be 20 (0x14) bytes in length. INIT_REPLY_FRAME will
    // leave pLCSIBH->iDataLen in some unpredictable state.
    INIT_REPLY_FRAME( pLCSSTDFRM, pLCSIBH->iDataLen, pCmdFrame, iCmdLen );
    pLCSIBH->iDataLen = REPLY_UNSUPPCMD_SNA_SIZE;

    // Initialize the reply fields.
    pLCSSTDFRM->bLCSCmdHdr.bLCSHdr.bSlot = pLCSDEV->bPort;
    pLCSSTDFRM->bLCSCmdHdr.bInitiator    = LCS_INITIATOR_SNA;
    pLCSSTDFRM->bLCSCmdHdr.bRelAdapterNo = pLCSDEV->bPort;
    STORE_HW( pLCSSTDFRM->bLCSCmdHdr.hwReturnCode, 0xFFFF );

    // Add the buffer containing the reply to the chain.
    add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );
}

// ====================================================================
//                     LCS_ProcessAccepted_SNA
// ====================================================================
static void LCS_ProcessAccepted_SNA ( PLCSPORT pLCSPORT, PLCSDEV pLCSDEV, BYTE* pData, size_t iSize )
{
#define Inbound_4D10_Size  33
static const BYTE Inbound_4D10[Inbound_4D10_Size] =
                 {
                   0xff, 0xff, 0x04, 0x00,                           /* LCSHDR  */
                   0x00, 0x14, 0x4D, 0x10, 0x00, 0x09, 0x60, 0x01,   /* LCSBAF1 */
                   0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x40,
                   0x00, 0x00, 0xFF, 0x00,
                   0x01, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff,   /* LCSBAF2 */
                   0xff
                 };
//  00A80400  00144D10 008F6001 00000101 01000040 0000FF00  01 0001 00 00 40007470 2D00000288CA6B810031001307B0B05033078797978707
//            0 1 2 3  4 5 6 7  8 9 A B  C D E F  0 1 2 3   0  1 2  3  4  5 6 7 8  9 A B C D E F 0 1 2 3 4 5 6 7 8 9 A B C D E F

#define INBOUND_4C25_SIZE  46
static const BYTE Inbound_4C25[INBOUND_4C25_SIZE] =
                 {
                    0x00, 0x2E, 0x04, 0x00,                          /* LCSHDR  */
                    0x00, 0x0D, 0x4C, 0x25, 0x00, 0x1C, 0x60, 0x03,  /* LCSBAF1 */
                    0x00, 0x00, 0x00, 0x01, 0x00,
                    0x01, 0xff, 0xff, 0xC0, 0x00, 0x00, 0x00, 0x00,  /* LCSBAF2 */
                    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0x04,
                    0x00                                             /* Filler  */
                 };
//  002E0400  000D4C25 001C6003 00000001 00  01 0001 C0000000 00000000 00 400074700001 000CCE4B4740 0401F3 04 00
//            0 1 2 3  4 5 6 7  8 9 A B  C   0  1 2  3 4 5 6  7 8 9 A  B  C D E F 0 1  2 3 4 5 6 7  8 9 A  B  C

#define INBOUND_4C22_SIZE  29
static const BYTE Inbound_4C22[INBOUND_4C22_SIZE] =
                 {
                    0x00, 0x1D, 0x04, 0x00,                          /* LCSHDR  */
                    0x00, 0x0D, 0x4C, 0x22, 0x00, 0x0C, 0x60, 0x03,  /* LCSBAF1 */
                    0x00, 0x00, 0x00, 0x01, 0x00,
                    0x01, 0xff, 0xff, 0xC0, 0x00, 0x00, 0x00, 0xff,  /* LCSBAF2 */
                    0x08, 0x00, 0x00, 0x00
                 };
//  002C0400  000D4C22 001B6003 00000001 00  01 0002 C0000000 08080000 00 destination. source...... LLC... TH etc...
//            0 1 2 3  4 5 6 7  8 9 A B  C   0  1 2  3 4 5 6  7 8 9 A  B  C D E F 0 1  2 3 4 5 6 7  8 9 A  B C D ...

#define INBOUND_4D00_SIZE  24
static const BYTE Inbound_4D00[INBOUND_4D00_SIZE] =
                 {
                    0x00, 0x18, 0x04, 0x00,                          /* LCSHDR  */
                    0x00, 0x0C, 0x4D, 0x00, 0x00, 0x08, 0x60, 0x01,  /* LCSBAF1 */
                    0xff, 0xff, 0xff, 0xff,
                    0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00   /* LCSBAF2 */
                 };
//  00180400  000C4D00 00086001 00000101  01 0000 40000240 00
//            0 1 2 3  4 5 6 7  8 9 A B   0  1 2  3 4 5 6  7

#define INBOUND_4C0B_SIZE  32
static const BYTE Inbound_4C0B[INBOUND_4C0B_SIZE] =
                 {
                    0x00, 0x20, 0x04, 0x00,                          /* LCSHDR  */
                    0x00, 0x18, 0x4C, 0x0B, 0x00, 0x03, 0x60, 0x01,  /* LCSBAF1 */
                    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x76, 0x56, 0x00, 0x00, 0x00, 0x00,
                    0x01, 0x40, 0x00,                                /* LCSBAF2 */
                    0x00                                             /* Filler  */
                 };
//  00200400  00184C0B 00036001 00000101 00000000 00007656 00000000  014000  00
//            0 1 2 3  4 5 6 7  8 9 A B  C D E F  0 1 2 3  4 5 6 7   0  1 2  3

#define INBOUND_CD00_SIZE  32
static const BYTE Inbound_CD00[INBOUND_CD00_SIZE] =
                 {
                    0x00, 0x20, 0x04, 0x00,                          /* LCSHDR  */
                    0x00, 0x18, 0xCD, 0x00, 0x00, 0x03, 0x60, 0x01,  /* LCSBAF1 */
                    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x01, 0xff, 0xff,                                /* LCSBAF2 */
                    0x00                                             /* Filler  */
                 };
//  00200400  0018CD00 00036001 00000101  00000000 00007000 00000000  010037  00
//            0 1 2 3  4 5 6 7  8 9 A B   C D E F  1 2 3 4  5 6 7 8   0 1 2


    LLC         llc;
    int         illcsize;
    DEVBLK*     pDEVBLK;
    PLCSBLK     pLCSBLK;
    PLCSATTN    pLCSATTN;
    PETHFRM     pEthFrame;
    BYTE*       pLLCandData;
    int         iLLCandDatasize;
    int         iDatasize;
    PLCSCONN    pLCSCONN;
    PLCSIBH     pLCSIBH;
    PLCSHDR     pLCSHDR;
    PLCSBAF1    pLCSBAF1;
    PLCSBAF2    pLCSBAF2;
    U16         hwOffset;
    U16         hwLenBaf1;
    U16         hwLenBaf2;
    BYTE        fAttnRequired = FALSE;
    PETHFRM     pEthFrameOut;
    int         iEthLenOut;
    int         iLPDULenOut;
    LLC         llcout;
    BYTE        frameout[64];
    char        llcmsg[256];

    UNREFERENCED( iSize );

    pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];  /* SNA has only one device */
    pLCSBLK = pLCSDEV->pLCSBLK;

    // Point to the accepted Ethernet frame, point to the LLC
    // and data, and get the length of the LLC and data.
    pEthFrame = (PETHFRM)pData;
    pLLCandData = &pEthFrame->bData[0];
    FETCH_HW( iLLCandDatasize, pEthFrame->hwEthernetType );

    // Extract the bits and bytes from the 802.2 LLC.
    illcsize = ExtractLLC( &llc, pLLCandData, iLLCandDatasize );

//??    net_data_trace( pDEVBLK, pLLCandData, iLLCandDatasize, NO_DIRECTION, 'D', "LLC and data", 0 );
//??    net_data_trace( pDEVBLK, &illcsize, sizeof(illcsize), NO_DIRECTION, 'D', "illcsize", 0 );
//??    net_data_trace( pDEVBLK, &llc, sizeof(LLC), NO_DIRECTION, 'D', "llc", 0 );

    // Discard the frame if the 802.2 LLC appears to be questionable.
    if ( !illcsize ) goto msg970_return;

    //
    switch (llc.hwType)
    {

    // Information Frame.
    case Type_Information_Frame:

        if (pLCSBLK->fDebug)
        {
          snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC information frame received: CR=%u, NR=%u, NS=%u", llc.hwCR, llc.hwNR, llc.hwNS );
          WRMSG(HHC03984, "D", llcmsg );
        }

        // Inbound TH etc, find the connection block.
        pLCSCONN = find_connection_by_remote_mac( pLCSDEV, &pEthFrame->bSrcMAC );
        if (!pLCSCONN)
        {
            WRMSG( HHC03984, "W", "LCSCONN not found");
            /* FixMe! Need a proper error message here! */
            break;
        }

        //
        PTT_DEBUG(       "GET  InOutLock    ", 000, pDEVBLK->devnum, -1 );
        obtain_lock( &pLCSDEV->InOutLock   );
        PTT_DEBUG(       "GOT  InOutLock    ", 000, pDEVBLK->devnum, -1 );

        pLCSCONN->fIframe = TRUE;

        // Check that the remote NS value has incremented by one.
        /* FixMe! Add some code here! */

        // Save the remote NS and NR values.
        pLCSCONN->hwRemoteNS = llc.hwNS;
        pLCSCONN->hwRemoteNR = llc.hwNR;

        // Obtain a buffer in which to construct the data to be passed to VTAM.
        pLCSIBH = alloc_lcs_buffer( pLCSDEV, 2032 );

        memcpy( &pLCSIBH->bData, Inbound_4D10, Inbound_4D10_Size );
        pLCSIBH->iDataLen = Inbound_4D10_Size;

        pLCSHDR = (PLCSHDR)&pLCSIBH->bData;
        pLCSBAF1 = (PLCSBAF1)( (BYTE*)pLCSHDR + sizeof(LCSHDR) );
        FETCH_HW( hwLenBaf1, pLCSBAF1->hwLenBaf1 );
        FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
        pLCSBAF2 = (PLCSBAF2)( (BYTE*)pLCSBAF1 + hwLenBaf1 );

        //
        memcpy( pLCSBAF1->bTokenA, &pLCSCONN->bInToken, sizeof(pLCSCONN->bInToken) );      // Set Outbound token

        //
        pLCSCONN->hwDataSeqNum++;
        STORE_HW( pLCSBAF2->hwSeqNum, pLCSCONN->hwDataSeqNum );

        memcpy( &pLCSBAF2->bByte05, pLCSPORT->MAC_Address, 4 );      // Just the first 4-bytes!

        // Copy the TH etc to the buffer.
        iDatasize = (iLLCandDatasize - illcsize);
        if ( iDatasize > 0 )
        {
            memcpy( &pLCSBAF2->bByte09, &pEthFrame->bData[illcsize], iDatasize );

            pLCSIBH->iDataLen += iDatasize;

            FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
            hwLenBaf2 += iDatasize;
            STORE_HW( pLCSBAF1->hwLenBaf2, hwLenBaf2 );

            FETCH_HW( hwOffset, pLCSHDR->hwOffset );
            hwOffset += iDatasize;
            STORE_HW( pLCSHDR->hwOffset, hwOffset );
        }

        // Add a padding byte to the buffer, if necessary.
        if  ( pLCSIBH->iDataLen & 0x01 )
        {
            pLCSIBH->iDataLen++;

            FETCH_HW( hwOffset, pLCSHDR->hwOffset );
            hwOffset++;
            STORE_HW( pLCSHDR->hwOffset, hwOffset );
        }

        // Add the buffer containing the inbond TH etc to the chain.
        add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

        fAttnRequired = TRUE;

        // We have done everything to let VTAM know that data has arrived,
        // now we need to let the remote end know the data has arrived.
        memset( frameout, 0, sizeof(frameout) );       // Clear area for ethernet fram
        pEthFrameOut = (PETHFRM)&frameout[0];
        iEthLenOut = 60;                               // Minimum ethernet frame length

        //
        memset( &llcout, 0, sizeof(LLC) );
        llcout.hwDSAP    = llc.hwSSAP;                 // Copy LLC inbound SSAP as outbound DSAP
        llcout.hwSSAP    = llc.hwDSAP;                 // Copy LLC inbound DSAP as outbound SSAP
        llcout.hwCR      = 1;
        llcout.hwNR      = ((pLCSCONN->hwRemoteNS + 1) & 0x7F);
        llcout.hwSS      = SS_Receiver_Ready;
        llcout.hwType    = Type_Supervisory_Frame;

        // Construct Ethernet frame
        memcpy( &pEthFrameOut->bDestMAC, &pEthFrame->bSrcMAC, IFHWADDRLEN );  // Copy destination MAC address
        memcpy( &pEthFrameOut->bSrcMAC, &pEthFrame->bDestMAC, IFHWADDRLEN );  // Copy source MAC address
        iLPDULenOut = BuildLLC( &llcout, pEthFrameOut->bData);                // Build LLC PDU
        STORE_HW( pEthFrameOut->hwEthernetType, (U16)iLPDULenOut );           // Set data length

        // Trace Ethernet frame before sending to TAP device
        if (pLCSBLK->fDebug)
        {
            // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
            WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                 pLCSDEV->bPort, iEthLenOut, "802.3 SNA", pLCSPORT->szNetIfName );
            net_data_trace( pDEVBLK, (BYTE*)pEthFrameOut, iEthLenOut, FROM_GUEST, 'D', "eth frame", 0 );
        }

        // Write the Ethernet frame to the TAP device
        if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrameOut, iEthLenOut ) == iEthLenOut)
        {
            if (pLCSPORT->pLCSBLK->fDebug)
            {
                snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC supervisory frame sent: CR=%u, SS=%s, PF=%u, NR=%u", llcout.hwCR, "Receiver Ready", llcout.hwPF, llcout.hwNR );
                WRMSG(HHC03984, "D", llcmsg );
            }
        }
        else
        {
//??        pLCSDEV->iTuntapErrno = errno;
//??        pLCSDEV->fTuntapError = TRUE;
            PTT_TIMING( "*WRITE ERR", 0, iEthLenOut, 1 );
        }

        //
        PTT_DEBUG(        "REL  InOutLock    ", 000, pDEVBLK->devnum, -1 );
        release_lock( &pLCSDEV->InOutLock   );

        break;

    // Supervisory Frame.
    case Type_Supervisory_Frame:

        switch (llc.hwSS)
        {

        // Supervisory Frame: Receiver Ready.
        case SS_Receiver_Ready:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC supervisory frame received: CR=%u, SS=%s, PF=%u, NR=%u", llc.hwCR, "Receiver Ready", llc.hwPF, llc.hwNR );
              WRMSG(HHC03984, "D", llcmsg );
            }

            // Find the connection block.
            pLCSCONN = find_connection_by_remote_mac( pLCSDEV, &pEthFrame->bSrcMAC );
            if (!pLCSCONN)
            {
                WRMSG( HHC03984, "W", "LCSCONN not found");
                /* FixMe! Need a proper error message here! */
                break;
            }

            // ??
            if (!llc.hwPF)
            {
                break;
            }

            memset( frameout, 0, sizeof(frameout) );       // Clear area for ethernet fram
            pEthFrameOut = (PETHFRM)&frameout[0];
            iEthLenOut = 60;                               // Minimum ethernet frame length

            //
            memset( &llcout, 0, sizeof(LLC) );
            llcout.hwDSAP    = llc.hwSSAP;                 // Copy LLC inbound SSAP as outbound DSAP
            llcout.hwSSAP    = llc.hwDSAP;                 // Copy LLC inbound DSAP as outbound SSAP
            llcout.hwCR      = 1;
            llcout.hwPF      = 1;
            if (pLCSCONN->fIframe)
            {
                llcout.hwNR  = ((pLCSCONN->hwRemoteNS + 1) & 0x7F);
            }
            llcout.hwSS      = SS_Receiver_Ready;
            llcout.hwType    = Type_Supervisory_Frame;

            // Construct Ethernet frame
            memcpy( &pEthFrameOut->bDestMAC, &pEthFrame->bSrcMAC, IFHWADDRLEN );  // Copy destination MAC address
            memcpy( &pEthFrameOut->bSrcMAC, &pEthFrame->bDestMAC, IFHWADDRLEN );  // Copy source MAC address
            iLPDULenOut = BuildLLC( &llcout, pEthFrameOut->bData);                // Build LLC PDU
            STORE_HW( pEthFrameOut->hwEthernetType, (U16)iLPDULenOut );           // Set data length

            // Trace Ethernet frame before sending to TAP device
            if (pLCSBLK->fDebug)
            {
                // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
                WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     pLCSDEV->bPort, iEthLenOut, "802.3 SNA", pLCSPORT->szNetIfName );
                net_data_trace( pDEVBLK, (BYTE*)pEthFrameOut, iEthLenOut, FROM_GUEST, 'D', "eth frame", 0 );
            }

            // Write the Ethernet frame to the TAP device
            if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrameOut, iEthLenOut ) == iEthLenOut)
            {
                if (pLCSPORT->pLCSBLK->fDebug)
                {
                    snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC supervisory frame sent: CR=%u, SS=%s, PF=%u, NR=%u", llcout.hwCR, "Receiver Ready", llcout.hwPF, llcout.hwNR );
                    WRMSG(HHC03984, "D", llcmsg );
                }
            }
            else
            {
//??            pLCSDEV->iTuntapErrno = errno;
//??            pLCSDEV->fTuntapError = TRUE;
                PTT_TIMING( "*WRITE ERR", 0, iEthLenOut, 1 );
            }

            break;

        // Supervisory Frame: Receiver Not Ready.
        case SS_Receiver_Not_Ready:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC supervisory frame received: CR=%u, SS=%s, PF=%u, NR=%u", llc.hwCR, "Receiver Not Ready", llc.hwPF, llc.hwNR );
              WRMSG(HHC03984, "D", llcmsg );
            }

            break;

        // Supervisory Frame: Reject
        case SS_Reject:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC supervisory frame received: CR=%u, SS=%s, PF=%u, NR=%u", llc.hwCR, "Reject", llc.hwPF, llc.hwNR );
              WRMSG(HHC03984, "D", llcmsg );
            }

            break;

        // Supervisory Frame: Unknown.
        default:
            goto msg970_return;

        }

        break;

    // Unnumbered Frame.
    case Type_Unnumbered_Frame:

        switch (llc.hwM)
        {

        // Unnumbered Frame: DM Response (B'00011', 0x03, 0x0F or 0x1F).
        case M_DM_Response:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame received: CR=%u, M=%s", llc.hwCR, "DM" );
              WRMSG(HHC03984, "D", llcmsg );
            }

            break;

        // Unnumbered Frame: DISC Command (B'01000', 0x10, 0x43 or 0x53).
        case M_DISC_Command:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame received: CR=%u, M=%s", llc.hwCR, "DISC" );
              WRMSG(HHC03984, "D", llcmsg );
            }

            // Find the connection block.
            pLCSCONN = find_connection_by_remote_mac( pLCSDEV, &pEthFrame->bSrcMAC );
            if (!pLCSCONN)
            {
                WRMSG( HHC03984, "W", "LCSCONN not found");
                /* FixMe! Need a proper error message here! */
                break;
            }

            // Obtain a buffer in which to construct the data to be passed to VTAM.
            pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_4C0B_SIZE * 2 ) );

            memcpy( &pLCSIBH->bData, Inbound_4C0B, INBOUND_4C0B_SIZE );
            pLCSIBH->iDataLen = INBOUND_4C0B_SIZE;

            pLCSHDR = (PLCSHDR)&pLCSIBH->bData;
            pLCSBAF1 = (PLCSBAF1)( (BYTE*)pLCSHDR + sizeof(LCSHDR) );
//          FETCH_HW( hwLenBaf1, pLCSBAF1->hwLenBaf1 );
//          FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
//          pLCSBAF2 = (PLCSBAF2)( (BYTE*)pLCSBAF1 + hwLenBaf1 );

            //
            memcpy( pLCSBAF1->bTokenA, &pLCSCONN->bInToken, sizeof(pLCSCONN->bInToken) );  // Set Inbound token

            // Add the buffer containing the XID response to the chain.
            add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

            fAttnRequired = TRUE;

            break;

        // Unnumbered Frame: UA Response (B'01100', 0x18, 0x63 or 0x73).
        case M_UA_Response:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame received: CR=%u, M=%s", llc.hwCR, "UA" );
              WRMSG(HHC03984, "D", llcmsg );
            }

            // Find the connection block.
            pLCSCONN = find_connection_by_remote_mac( pLCSDEV, &pEthFrame->bSrcMAC );
            if (!pLCSCONN)
            {
                WRMSG( HHC03984, "W", "LCSCONN not found");
                /* FixMe! Need a proper error message here! */
                break;
            }

            // Obtain a buffer in which to construct the data to be passed to VTAM.
            pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_CD00_SIZE * 2 ) );

            memcpy( &pLCSIBH->bData, Inbound_CD00, INBOUND_CD00_SIZE );
            pLCSIBH->iDataLen = INBOUND_CD00_SIZE;

            pLCSHDR = (PLCSHDR)&pLCSIBH->bData;
            pLCSBAF1 = (PLCSBAF1)( (BYTE*)pLCSHDR + sizeof(LCSHDR) );
            FETCH_HW( hwLenBaf1, pLCSBAF1->hwLenBaf1 );
            FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
            pLCSBAF2 = (PLCSBAF2)( (BYTE*)pLCSBAF1 + hwLenBaf1 );

            //
            memcpy( pLCSBAF1->bTokenA, &pLCSCONN->bInToken, sizeof(pLCSCONN->bInToken) );  // Set Inbound token

            //
            STORE_HW( pLCSBAF2->hwSeqNum, pLCSCONN->hwDataSeqNum );
            pLCSCONN->hwDataSeqNum++;

            // Add the buffer containing the XID response to the chain.
            add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

            fAttnRequired = TRUE;

            break;


        // Unnumbered Frame: SABME Command (B'01111', 0x1B, 0x6F or 0x7F).
        case M_SABME_Command:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame received: CR=%u, M=%s", llc.hwCR, "SABME" );
              WRMSG(HHC03984, "D", llcmsg );
            }

            // Find the connection block.
            pLCSCONN = find_connection_by_remote_mac( pLCSDEV, &pEthFrame->bSrcMAC );
            if (!pLCSCONN)
            {
                WRMSG( HHC03984, "W", "LCSCONN not found");
                /* FixMe! Need a proper error message here! */
                break;
            }

            // Obtain a buffer in which to construct the data to be passed to VTAM.
            pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_4D00_SIZE * 2 ) );

            memcpy( &pLCSIBH->bData, Inbound_4D00, INBOUND_4D00_SIZE );
            pLCSIBH->iDataLen = INBOUND_4D00_SIZE;

            pLCSHDR = (PLCSHDR)&pLCSIBH->bData;
            pLCSBAF1 = (PLCSBAF1)( (BYTE*)pLCSHDR + sizeof(LCSHDR) );
            FETCH_HW( hwLenBaf1, pLCSBAF1->hwLenBaf1 );
            FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
            pLCSBAF2 = (PLCSBAF2)( (BYTE*)pLCSBAF1 + hwLenBaf1 );

            //
            memcpy( pLCSBAF1->bTokenA, &pLCSCONN->bInToken, sizeof(pLCSCONN->bInToken) );  // Set Inbound token

            //
            STORE_HW( pLCSBAF2->hwSeqNum, pLCSCONN->hwDataSeqNum );
//??        pLCSCONN->hwDataSeqNum++;

            memcpy( &pLCSBAF2->bByte03, &pLCSCONN->bOutToken, sizeof(pLCSCONN->bOutToken) ); // Outbound token

            // Add the buffer containing the XID response to the chain.
            add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

            fAttnRequired = TRUE;

            break;

        // Unnumbered Frame: FRMR Response (B'10001', 0x11, 0x87 or 0x97).
        //
        // A link station sends a Frame Reject response to report an error in an
        // incoming LPDU from the other link station. When you see a FRMR, the
        // station that sends the FRMR has detected an unrecoverable error. It
        // is not the cause of the error. Any frames that arrive after the FRMR
        // error has occurred are ignored until a DISC or SABME is received.
        // A FRMR response contains information about the cause of the FRMR
        // condition.
        // Bytes 0 and 1 contain the contents of the control field of the LPDU
        // which caused the frame reject. Bytes 2 and 3 contain the NS an NR
        // counts, respectively. Byte 4 contains several bits that identify the
        // type of error as shown here:
        //   0-0-0-V-Z-Y-W-X
        // The V bit indicates that the NS number carried by the control field
        // in bytes 0 and 1 is invalid. An NS is invalid if greater than or
        // equal to the last NS plus the maximum receive window size. When this
        // condition occurs, the link station sends a REJ LPDU, not a FRMR
        // response.
        // The Z bit indicates that the NR that the control field carries
        // indicated in bytes 0 and 1 does not refer to either the next I frame
        // or an I frame that has already been transmitted but not acknowledged.
        //   Note: It is all right to receive the the same NR count multiple
        //   times.
        //   The NR count is only invalid if the count references an I frame
        //   that has already been acknowledged or if the count skips ahead to
        //   one that has not been transmitted yet. The former is the most
        //   common case of this type of error. When you see this type of error,
        //   it usually means frames were received out of sequence, and you
        //   should look for the network that delivers frames out of order. It
        //   is possible that the sending link station transmitted them out of
        //   order, but very unlikely.
        // The Y bit indicates that the length of the I field in the received
        // LPDU exceeded the available buffer capacity. If this situation
        // occurs, look for problems in the end stations, not the network.
        // The X bit indicates that the LPDU contained an I field when it must
        // not have, or a FRMR response was received that did not contain 5
        // bytes. This appears to be an end station problem, not a network
        // problem.
        // The W bit indicates that an unsupported LPDU was received. This is an
        // end station problem.
        case M_FRMR_Response:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame received: CR=%u, M=%s", llc.hwCR, "FRMR" );
              WRMSG(HHC03984, "D", llcmsg );
              snprintf( llcmsg, sizeof(llcmsg), "     CF=%4.4X, NR=%u, NS=%u, V=%u, Z=%u, Y=%u, X=%u, W=%u",
                                          llc.hwCF, llc.hwNR, llc.hwNS, llc.hwV, llc.hwZ, llc.hwY, llc.hwX, llc.hwW );
              WRMSG(HHC03984, "D", llcmsg );
            }

            break;

        // Unnumbered Frame: XID Command or Response (B'10111', 0x2B, 0xAF or 0xBF).
        // Command.  A remote system has initiated the exchange of identifiers.
        //           The remote system has sent this XID command, which we will
        //           pass to VTAM, and to which VTAM will eventually send an
        //           XID response to the remote system.
        // Response. The local system has initiated the exchange of identifiers.
        //           Earlier, VTAM sent an XID command to the remote system, to
        //           which the remote system has responded with this XID response,
        //           which we will pass to VTAM.
        case M_XID_Command_or_Response:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame received: CR=%u, M=%s", llc.hwCR, "XID" );
              WRMSG(HHC03984, "D", llcmsg );
            }

            // Calculate the size of the XID0 or the XID3 and CV's.
            iDatasize = (iLLCandDatasize - illcsize);

            // Find the connection block.
            pLCSCONN = find_connection_by_remote_mac( pLCSDEV, &pEthFrame->bSrcMAC );
            if (llc.hwCR)  // Response.
            {
                if (!pLCSCONN)
                {
                    WRMSG( HHC03984, "W", "LCSCONN not found");
                    /* FixMe! Need a proper error message here! */
                    break;
                }
            }
            else  // Command.
            {
                if (!pLCSCONN)  // There isn't an existing LCSCONN.
                {
                    if ( iDatasize > 0 )  // Is there an XID0 or an XID3 and CV's?
                    {
                        WRMSG( HHC03984, "W", "LCSCONN not found");
                        /* FixMe! Need a proper error message here! */
                        break;
                    }

                    // Create the LCSCONN for the new inbound connection.
                    pLCSCONN = alloc_connection( pLCSDEV );
                    pLCSCONN->hwCreated = LCSCONN_CREATED_INBOUND;

                    memcpy( &pLCSCONN->bLocalMAC, &pLCSPORT->MAC_Address, IFHWADDRLEN );
#if !defined( OPTION_TUNTAP_LCS_SAME_ADDR )
                    pLCSCONN->bLocalMAC[5]++;
#endif
                    memcpy( &pLCSCONN->bRemoteMAC, &pEthFrame->bSrcMAC, IFHWADDRLEN );

                    if (pLCSDEV->pLCSBLK->fDebug)
                    {
                        WRMSG( HHC03984, "I", "Created LCSCONN Inbound");
                        net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
                    }

                    add_connection_to_chain( pLCSDEV, pLCSCONN );
                }
                else  // There is an existing LCSCONN.
                {
                    if ( pLCSCONN->hwCreated == LCSCONN_CREATED_INBOUND )  // The existing LSCCONN is for an inbound connection.
                    {
                        if (pLCSDEV->pLCSBLK->fDebug)
                        {
                            WRMSG( HHC03984, "I", "Found LCSCONN Inbound");
                            net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
                        }
                    }
                    else  // The existing LCSCONN is for an outbound connection.
                    {
                        if (pLCSDEV->pLCSBLK->fDebug)
                        {
                            WRMSG( HHC03984, "W", "Found existing LCSCONN Outbound, changed to LCSCONN Inbound");
                            net_data_trace( pDEVBLK, (BYTE*)pLCSCONN, sizeof(LCSCONN), NO_DIRECTION, 'D', "LCSCONN", 0 );
                        }

                        pLCSCONN->hwCreated = LCSCONN_CREATED_INBOUND;
                    }
                }
            }

            // Obtain a buffer in which to construct the data to be passed to VTAM.
            // Note: The largest XID3 and CV's seen has been less than 160-bytes.
            pLCSIBH = alloc_lcs_buffer( pLCSDEV, 496 );

            memcpy( &pLCSIBH->bData, Inbound_4C22, INBOUND_4C22_SIZE );
            pLCSIBH->iDataLen = INBOUND_4C22_SIZE;

            pLCSHDR = (PLCSHDR)&pLCSIBH->bData;
            pLCSBAF1 = (PLCSBAF1)( (BYTE*)pLCSHDR + sizeof(LCSHDR) );
            FETCH_HW( hwLenBaf1, pLCSBAF1->hwLenBaf1 );
//          FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
            pLCSBAF2 = (PLCSBAF2)( (BYTE*)pLCSBAF1 + hwLenBaf1 );

            //
            if (llc.hwCR)  // Response.
            {
                pLCSCONN->hwXIDSeqNum++;
                STORE_HW( pLCSBAF2->hwSeqNum, pLCSCONN->hwXIDSeqNum );
                pLCSBAF2->bByte07 = 0x08;  // ???
            }
            else  // Command.
            {
                pLCSDEV->hwInXIDSeqNum++;
                STORE_HW( pLCSBAF2->hwSeqNum, pLCSDEV->hwInXIDSeqNum );
                pLCSBAF2->bByte07 = 0x04;  // ???
            }

            // Copy the destination MAC address, the source MAC addresses, and the LLC to the buffer.
            memcpy( &pLCSBAF2->bByte12, &pEthFrame->bDestMAC, IFHWADDRLEN );
            memcpy( &pLCSBAF2->bByte18, &pEthFrame->bSrcMAC, IFHWADDRLEN );
            memcpy( &pLCSBAF2->bByte24, &pEthFrame->bData, 3 );

            pLCSIBH->iDataLen += (6 + 6 + 3);

            FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
            hwLenBaf2 += (6 + 6 + 3);
            STORE_HW( pLCSBAF1->hwLenBaf2, hwLenBaf2 );

            FETCH_HW( hwOffset, pLCSHDR->hwOffset );
            hwOffset += (6 + 6 + 3);
            STORE_HW( pLCSHDR->hwOffset, hwOffset );

            // Copy the XID0 or the XID3 and CV's to the buffer.
            if ( iDatasize > 0 )
            {
                memcpy( &pLCSBAF2->bByte27, &pEthFrame->bData[3], iDatasize );

                pLCSIBH->iDataLen += iDatasize;

                FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
                hwLenBaf2 += iDatasize;
                STORE_HW( pLCSBAF1->hwLenBaf2, hwLenBaf2 );

                FETCH_HW( hwOffset, pLCSHDR->hwOffset );
                hwOffset += iDatasize;
                STORE_HW( pLCSHDR->hwOffset, hwOffset );
            }

            // Add a padding byte to the buffer, if necessary.
            if  ( pLCSIBH->iDataLen & 0x01 )
            {
                pLCSIBH->iDataLen++;

                FETCH_HW( hwOffset, pLCSHDR->hwOffset );
                hwOffset++;
                STORE_HW( pLCSHDR->hwOffset, hwOffset );
            }

            // Add the buffer containing the XID response to the chain.
            add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

            fAttnRequired = TRUE;

            break;

        // Unnumbered Frame: TEST Command or Response (B'11100, 0x38, 0xE3 or 0xF3).
        case M_TEST_Command_or_Response:

            if (pLCSBLK->fDebug)
            {
              snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame received: CR=%u, M=%s", llc.hwCR, "TEST" );
              WRMSG(HHC03984, "D", llcmsg );
            }

            if (llc.hwCR)  // Response. Tell VTAM the remote system has responded to VTAM's TEST.
            {

                // XID response, find the connection block.
                pLCSCONN = find_connection_by_remote_mac( pLCSDEV, &pEthFrame->bSrcMAC );
                if (!pLCSCONN)
                {
                    WRMSG( HHC03984, "W", "LCSCONN not found");
                    /* FixMe! Need a proper error message here! */
                    break;
                }

                // Obtain a buffer in which to construct the data to be passed to VTAM.
                pLCSIBH = alloc_lcs_buffer( pLCSDEV, ( INBOUND_4C25_SIZE * 2 ) );

                memcpy( &pLCSIBH->bData, Inbound_4C25, INBOUND_4C25_SIZE );
                pLCSIBH->iDataLen = INBOUND_4C25_SIZE;

                pLCSHDR = (PLCSHDR)&pLCSIBH->bData;
                pLCSBAF1 = (PLCSBAF1)( (BYTE*)pLCSHDR + sizeof(LCSHDR) );
                FETCH_HW( hwLenBaf1, pLCSBAF1->hwLenBaf1 );
                FETCH_HW( hwLenBaf2, pLCSBAF1->hwLenBaf2 );
                pLCSBAF2 = (PLCSBAF2)( (BYTE*)pLCSBAF1 + hwLenBaf1 );

                //
                pLCSCONN->hwXIDSeqNum++;
                STORE_HW( pLCSBAF2->hwSeqNum, pLCSCONN->hwXIDSeqNum );

                // Copy the destination MAC address, the source MAC addresses, and the LLC to the buffer.
                memcpy( &pLCSBAF2->bByte12, &pEthFrame->bDestMAC, IFHWADDRLEN );
                memcpy( &pLCSBAF2->bByte18, &pEthFrame->bSrcMAC, IFHWADDRLEN );
                memcpy( &pLCSBAF2->bByte24, &pEthFrame->bData, 3 );

                // Add the buffer containing the reply to the chain.
                add_lcs_buffer_to_chain( pLCSDEV, pLCSIBH );

                fAttnRequired = TRUE;

            }
            else  // Command. Respond to the TEST from the remote system.
            {

                memset( frameout, 0, sizeof(frameout) );       // Clear area for ethernet fram
                pEthFrameOut = (PETHFRM)&frameout[0];
                iEthLenOut = 60;                               // Minimum ethernet frame length

                //
                memset( &llcout, 0, sizeof(LLC) );
                llcout.hwDSAP    = llc.hwSSAP;                       // Copy LLC command SSAP as response DSAP
                llcout.hwSSAP    = llc.hwDSAP;                       // Copy LLC command DSAP as response SSAP
                llcout.hwCR      = 1;
                llcout.hwPF      = 1;
                llcout.hwM       = M_TEST_Command_or_Response;
                llcout.hwType    = Type_Unnumbered_Frame;

                // Construct Ethernet frame
                memcpy( &pEthFrameOut->bDestMAC, &pEthFrame->bSrcMAC, IFHWADDRLEN );  // Copy destination MAC address
                memcpy( &pEthFrameOut->bSrcMAC, &pEthFrame->bDestMAC, IFHWADDRLEN );  // Copy source MAC address
                iLPDULenOut = BuildLLC( &llcout, pEthFrameOut->bData);                // Build LLC PDU
                STORE_HW( pEthFrameOut->hwEthernetType, (U16)iLPDULenOut );           // Set data length

                // Trace Ethernet frame before sending to TAP device
                if (pLCSBLK->fDebug)
                {
                    // "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
                    WRMSG(HHC00983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                         pLCSDEV->bPort, iEthLenOut, "802.3 SNA", pLCSPORT->szNetIfName );
                    net_data_trace( pDEVBLK, (BYTE*)pEthFrameOut, iEthLenOut, FROM_GUEST, 'D', "eth frame", 0 );
                }

                // Write the Ethernet frame to the TAP device
                if (TUNTAP_Write( pDEVBLK->fd, (BYTE*)pEthFrameOut, iEthLenOut ) == iEthLenOut)
                {
                    if (pLCSPORT->pLCSBLK->fDebug)
                    {
                        snprintf( llcmsg, sizeof(llcmsg), "LCS: LLC unnumbered frame sent: CR=%u, M=%s", llc.hwCR, "TEST" );
                        WRMSG(HHC03984, "D", llcmsg );
                    }
                }
                else
                {
    //??            pLCSDEV->iTuntapErrno = errno;
    //??            pLCSDEV->fTuntapError = TRUE;
                    PTT_TIMING( "*WRITE ERR", 0, iEthLenOut, 1 );
                }

            }

            break;

        // Unnumbered Frame: Unknown.
        default:
            goto msg970_return;

        }

        break;

    // Unknown frame. This should not occur!
    default:
        goto msg970_return;

    }

    if (fAttnRequired)
    {
        // It would have been nice to have simply called function
        // device_attention at this point, but the channel program
        // is still considered to be busy, and a return code of one
        // would be returned to us.

        /* Create an LCSATTN block */
        pLCSATTN = malloc( sizeof( LCSATTN ) );
        if (!pLCSATTN) return;  /* FixMe! Produce a message? */
        pLCSATTN->pNext = NULL;
        pLCSATTN->pDevice = pLCSDEV;

        /* Add LCSATTN block to start of chain */
        PTT_DEBUG( "GET  AttnLock", 000, pDEVBLK->devnum, 000 );
        obtain_lock( &pLCSBLK->AttnLock );
        PTT_DEBUG( "GOT  AttnLock", 000, pDEVBLK->devnum, 000 );
        {
            PTT_DEBUG( "ADD  Attn", pLCSATTN, pDEVBLK->devnum, 000 );
            pLCSATTN->pNext = pLCSBLK->pAttns;
            pLCSBLK->pAttns = pLCSATTN;
        }
        PTT_DEBUG( "REL  AttnLock", 000, pDEVBLK->devnum, 000 );
        release_lock( &pLCSBLK->AttnLock );

        /* Signal the LCS_AttnThread to process the LCSATTN block(s) on the chain */
        PTT_DEBUG( "GET  AttnEventLock ", 000, pDEVBLK->devnum, 000 );
        obtain_lock( &pLCSBLK->AttnEventLock );
        PTT_DEBUG( "GOT  AttnEventLock ", 000, pDEVBLK->devnum, 000 );
        {
            PTT_DEBUG( "SIG  AttnEvent", 000, pDEVBLK->devnum, 000 );
            signal_condition( &pLCSBLK->AttnEvent );
        }
        PTT_DEBUG( "REL  AttnEventLock ", 000, pDEVBLK->devnum, 000 );
        release_lock( &pLCSBLK->AttnEventLock );
    }

    return;

msg970_return:
    if (pLCSPORT->pLCSBLK->fDebug)
        // "CTC: lcs device port %2.2X: 802.2 LLC error, discarding frame"
        WRMSG( HHC00970, "W", pLCSPORT->bPort );

    return;
}

// ====================================================================
//                       LCS_Read_SNA
// ====================================================================
// The guest o/s is issuing a Read CCW for our LCS device. Return to
// it all available LCS Frames that we have buffered up in our buffer.
// --------------------------------------------------------------------

// FIXME: we currently don't support data-chaining but
// probably should if real LCS devices do (I was unable
// to determine whether they do or not). -- Fish

// FixMe: It would be much more efficent to skip the frame buffer entriely,
//        and move the data directly to the pIOBuf area.
//        We could them dispose of the frame buffer, and only allocate it
//        for IP devices.
//        We could then report that we will pass up to 65,016 (0xFDF8) bytes.

void  LCS_Read_SNA( DEVBLK* pDEVBLK,   U32   sCount,
                    BYTE*   pIOBuf,    BYTE* pUnitStat,
                    U32*    pResidual, BYTE* pMore )
{
    PLCSHDR     pLCSHdr;
    PLCSDEV     pLCSDEV = (PLCSDEV)pDEVBLK->dev_data;
    PLCSICTL    pLCSICTL;
    PLCSIBH     pLCSIBH;
    PLCSHDR     pFrameSlot;
    BYTE*       pFrameFiller;
    int         iFiller;
    int         iLength = 0;
    int         iTraceLen;
    U16         hwDataSize;
    BYTE        WantLCSICTL;
    BYTE        WantLCSIBH;


    PTT_DEBUG( "RSNA: ENTRY       ", 000, pDEVBLK->devnum, -1 );

    // Check whether the Read CCW is the third of three CCW's in the
    // WCTL channel program. The channel program contains Control
    // (0x17), Write (0x01) and Read (0x02) CCW's.
    if (pLCSDEV->bFlipFlop == WCTL)
    {
        WantLCSICTL = TRUE;
        WantLCSIBH = FALSE;
    }
    // Check whether the Read CCW is the third of three CCW's in the
    // SCB channel program, where the channel program contains Sense
    // Command Byte (0x14), Write (0x01) and Read (0x02) CCW's,
    else if (pLCSDEV->bFlipFlop == SCB)
    {
        WantLCSICTL = TRUE;
        WantLCSIBH = TRUE;
    }
    // The Read CCW is the only CCW in the channel program. This
    // is the state while the Start LAN and LAN Stats commands
    // are being processed.
    else
    {
        WantLCSICTL = FALSE;
        WantLCSIBH = TRUE;
    }

    //
    if ( WantLCSICTL )
    {
        // Prepare the ICTL
        pLCSDEV->hwIctlSize = sizeof(LCSICTL);
        memset(&pLCSDEV->Ictl, 0, sizeof(LCSICTL));
        pLCSICTL = &pLCSDEV->Ictl;
        pLCSICTL->XCNISTAT = XCNIRSVD;
    }
    else
    {
        // Clear the ICTL
        pLCSDEV->hwIctlSize = 0;
        memset(&pLCSDEV->Ictl, 0, sizeof(LCSICTL));
        pLCSICTL = NULL;
    }

    //
    if ( WantLCSICTL )
    {
        if ( !pLCSDEV->iFrameOffset )
        {
            pLCSICTL = (PLCSICTL)&pLCSDEV->bFrameBuffer;
            memcpy( pLCSICTL, &pLCSDEV->Ictl, pLCSDEV->hwIctlSize );
            pLCSDEV->iFrameOffset += pLCSDEV->hwIctlSize;
            pLCSDEV->fPendingIctl = 1;
        }
    }

    //
    if ( WantLCSIBH )
    {
        while (pLCSDEV->pFirstLCSIBH)
        {
            //
            if ( pLCSDEV->pFirstLCSIBH->iDataLen & 0x01 )
                iFiller = 1;
            else
                iFiller = 0;

            // Ensure the contents of the buffer will fit into the frame buffer
            if ((pLCSDEV->iFrameOffset +            // Current buffer Offset
                 pLCSDEV->pFirstLCSIBH->iDataLen +  // Size of inbound data
                 iFiller +                          // Size of filler
                 sizeof(pFrameSlot->hwOffset))      // Size of Frame terminator
              <= pLCSDEV->iMaxFrameBufferSize)      // Size of Frame buffer
            {
                pLCSIBH = remove_lcs_buffer_from_chain( pLCSDEV );

                // Point to next available LCS Frame slot in our buffer...
                // Copy the inbound frame into the frame buffer slot...
                // Increment buffer offset to NEXT next-available-slot...
                pFrameSlot = (PLCSHDR)( pLCSDEV->bFrameBuffer +
                                        pLCSDEV->iFrameOffset );

                memcpy( pFrameSlot, &pLCSIBH->bData, pLCSIBH->iDataLen );

                pLCSDEV->iFrameOffset += (U16) pLCSIBH->iDataLen;

                if ( iFiller )
                {
                    // Point to the end of the inbound frame just copied to
                    // the frame buffer slot and add a filler byte to the end.
                    // Increment buffer offset to NEXT next-available-slot...
                    pFrameFiller = ( pLCSDEV->bFrameBuffer +
                                     pLCSDEV->iFrameOffset );

                    pFrameFiller[0] = 0;

                    pLCSDEV->iFrameOffset += 1;
                }

                // Store offset of next frame
                if ( pLCSDEV->hwIctlSize )
                    STORE_HW( pFrameSlot->hwOffset, ( pLCSDEV->iFrameOffset - pLCSDEV->hwIctlSize ) );
                else
                    STORE_HW( pFrameSlot->hwOffset, pLCSDEV->iFrameOffset );

                free_lcs_buffer( pLCSDEV, pLCSIBH );
                pLCSIBH = NULL;

                pLCSDEV->fDataPending = 1;
            }
        }
    }

    // Point to the end of all buffered LCS Frames (where
    // the next Frame *would* go if there was one), and
    // mark the end of this batch of LCS Frames by setting
    // the "offset to NEXT frame" LCS Header field to zero
    // (a zero "next Frame offset" is like an "EOF" flag).
    if ( pLCSDEV->fDataPending )
    {
        pLCSHdr = (PLCSHDR)( pLCSDEV->bFrameBuffer +
                             pLCSDEV->iFrameOffset );
        STORE_HW( pLCSHdr->hwOffset, 0x0000 );
        pLCSDEV->iFrameOffset += 2;
    }

    //
    if ( pLCSDEV->fPendingIctl )
    {
        // Calculate how much data we're going to be giving them.
        // Since 'iFrameOffset' points to the next available LCS
        // Frame slot in our buffer, the total amount of LCS Frame
        // data we have is exactly that amount. We give them two
        // extra bytes however so that they can optionally chase
        // the "hwOffset" field in each LCS Frame's LCS Header to
        // eventually reach our zero hwOffset "EOF" flag).
        iLength = pLCSDEV->iFrameOffset;

        //
        pLCSICTL = (PLCSICTL)&pLCSDEV->bFrameBuffer;
        hwDataSize = (U16) ( pLCSDEV->iFrameOffset - pLCSDEV->hwIctlSize );
        if ( hwDataSize )
        {
//          iLength += sizeof(pLCSHdr->hwOffset);
//          hwDataSize += sizeof(pLCSHdr->hwOffset);
            STORE_HW( pLCSICTL, hwDataSize );
        }
    }
    else
    {
        // Calculate how much data we're going to be giving them.
        // Since 'iFrameOffset' points to the next available LCS
        // Frame slot in our buffer, the total amount of LCS Frame
        // data we have is exactly that amount. We give them two
        // extra bytes however so that they can optionally chase
        // the "hwOffset" field in each LCS Frame's LCS Header to
        // eventually reach our zero hwOffset "EOF" flag).
//      iLength = pLCSDEV->iFrameOffset + sizeof(pLCSHdr->hwOffset);
        iLength = pLCSDEV->iFrameOffset;
    }

    // (calculate residual and set memcpy amount)

    // FIXME: we currently don't support data-chaining but
    // probably should if real LCS devices do (I was unable
    // to determine whether they do or not). -- Fish

    if (sCount < (U32)iLength)
    {
        *pMore     = 1;
        *pResidual = 0;

        iLength = sCount;

        // PROGRAMMING NOTE: As a result of the caller asking
        // for less data than we actually have available, the
        // remainder of their unread data they didn't ask for
        // will end up being silently discarded. Refer to the
        // other NOTEs and FIXME's sprinkled throughout this
        // function...
    }
    else
    {
        *pMore      = 0;
        *pResidual -= iLength;
    }

    *pUnitStat = CSW_CE | CSW_DE;

    memcpy( pIOBuf, pLCSDEV->bFrameBuffer, iLength );

    // Display up to pLCSBLK->iTraceLen bytes of the data going to the guest, if debug is active
    if (pLCSDEV->pLCSBLK->fDebug)
    {
        // "%1d:%04X %s: Present data of size %d bytes to guest"
        WRMSG(HHC00982, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, (int)iLength );
        if (pLCSDEV->pLCSBLK->iTraceLen)
        {
            iTraceLen = iLength;
            if (iTraceLen > pLCSDEV->pLCSBLK->iTraceLen)
            {
                iTraceLen = pLCSDEV->pLCSBLK->iTraceLen;
                // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, (int)(iLength - iTraceLen) );
            }
            net_data_trace( pDEVBLK, pIOBuf, iTraceLen, TO_GUEST, 'D', "data", 0 );
        }
    }

    // Reset frame buffer to empty...

    // PROGRAMMING NOTE: even though not all available data
    // may have been read by the guest, we don't currently
    // support data-chaining. Thus any unread data is always
    // discarded by resetting all of the iFrameOffset,
    // fDataPending and fReplyPending fields to 0 so that the
    // next read always grabs a new batch of LCS Frames starting
    // at the very beginning of our frame buffer again. (I was
    // unable to determine whether real LCS devices support
    // data-chaining or not, but if they do we should fix this).

    PTT_DEBUG( "RSNA empty buffer ", 000, pDEVBLK->devnum, -1 );
    pLCSDEV->iFrameOffset  = 0;
    pLCSDEV->fReplyPending = 0;
    pLCSDEV->fDataPending  = 0;
    pLCSDEV->fPendingIctl = 0;

//??    PTT_DEBUG(        "REL  DevDataLock  ", 000, pDEVBLK->devnum, -1 );
//??    release_lock( &pLCSDEV->DevDataLock );

    PTT_DEBUG( "RSNA: EXIT        ", 000, pDEVBLK->devnum, -1 );
}   // End of  LCS_Read_SNA


// ====================================================================
// Extract the complex LLC PDU bits and bytes into simple variables.
// ====================================================================
//
// The Ethernet frame carrying the LLC PDU has the following layout:-
//   dddddddddddd ssssssssssss llll xxxxxx...
// where:
//   dddddddddddd  is the destination MAC address
//   ssssssssssss  is the source MAC address
//   llll          is the length of the LLC PDU
//   xxxxxx...     is the LLC PDU
//
// Note: An Ethernet frame can be a minimum length of 60-bytes, which
//       means there will be a minimum of 46-bytes following the
//       length (i.e. the llll). The LLC PDU has a minimum length of
//       3-bytes, hence there may be up to 43-bytes of unpredictable
//       rubbish following the PDU.
//
// On entry to this function pStart should point to the LLC PDU (i.e.
// the xxxxxx...), and iSize should contain the length of the LLC PDU
// (i.e. the llll field).
//
// On return from this function the length of the LLC in the PDU is
// returned. If the LLC appears malformed zero is returned.
//
int  ExtractLLC( PLLC pLLC, BYTE* pStart, int iSize )
{
    PLPDU     pLPDU;
    int       iLPDUsize;


    memset( pLLC, 0, sizeof(LLC) );

    pLPDU = (PLPDU)pStart;
    iLPDUsize = iSize;

    if (iLPDUsize < 3)
    {
        return 0;    // Less than 3 bytes!
    }

    pLLC->hwDSAP = (pLPDU->bDSAP[0] & 0xFE);
    pLLC->hwIG = (pLPDU->bDSAP[0] & 0x01);
    pLLC->hwSSAP = (pLPDU->bSSAP[0] & 0xFE);
    pLLC->hwCR = (pLPDU->bSSAP[0] & 0x01);

    if ((pLPDU->bControl[0] & 0x01) == 0x00)               // Control low-order bit = 0?
    {
        pLLC->hwType = Type_Information_Frame;             // LLC Information frame
    }
    else
    {
        if ((pLPDU->bControl[0] & 0x03) == 0x01)           // Control low-order 2 bits = 01?
        {
            pLLC->hwType = Type_Supervisory_Frame;         // LLC Supervisory frame
        }
        else  //  ((pLPDU->bControl[0] & 0x03) == 0x03)    // Control low-order 2 bits are = 11.
        {
            pLLC->hwType = Type_Unnumbered_Frame;          // LLC Unnumbered frame
        }
    }

    switch (pLLC->hwType)
    {
    case Type_Information_Frame:
        // Information Frame (I Frame).
        // The control field should be 2 bytes.
        // The format of the I frame contains an NS and NR count. The
        // NS count is the sequence number (modulo 128) of the LPDU
        // currently in transmission. The NR count is the sequence
        // number of the next LPDU I frame that the sender expects to
        // receive. To help you later, remember that NR means "next
        // receive." Here is the format of an information control:
        //   NS-NS-NS-NS-NS-NS-NS-0-NR-NR-NR-NR-NR-NR-P/F

        if (iLPDUsize < 4)
        {
            return 0;    // Less than 4 bytes!
        }

        memcpy( &pLLC->bLpdu, (BYTE*)pLPDU, 4);
        pLLC->hwLpduSize = 4;

        pLLC->hwNS = (pLPDU->bControl[0] >> 1);
        pLLC->hwNR = (pLPDU->bControl[1] >> 1);
        pLLC->hwPF = (pLPDU->bControl[1] & 0x01);

        iLPDUsize = 4;

        break;

    case Type_Supervisory_Frame:
        // Supervisory Frame.
        // The control field should be 2 bytes.
        // Supervisory frames perform supervisory control functions,
        // for example, to acknowledge I Frames (RR), to request
        // retransmission of I frames (REJ), and to request temporary
        // suspension (RNR) of I frames. Supervisory frames do not
        // contain an information field. Therefore, supervisory frames
        // do not affect the NS in the sending station, and so do not
        // contain an NS field. Here is the format of a supervisory
        // control:-
        //   0-0-0-0-S-S-0-1-NR-NR-NR-NR-NR-NR-NR-P/F

        if (iLPDUsize < 4)
        {
            return 0;    // Less than 4 bytes!
        }

        memcpy( &pLLC->bLpdu, (BYTE*)pLPDU, 4);
        pLLC->hwLpduSize = 4;

        pLLC->hwSS = ((pLPDU->bControl[0] & 0x0C) >> 2);
        pLLC->hwNR = (pLPDU->bControl[1] >> 1);
        pLLC->hwPF = (pLPDU->bControl[1] & 0x01);

        iLPDUsize = 4;

        break;

    case Type_Unnumbered_Frame:
        // Unnumbered Frame.
        // The control field should be 1 byte.
        // Unnumbered frames provide link control functions, for
        // example, mode setting commands and responses. In some
        // cases, unnumbered information frames can also be sent.
        // They do not contain fields for NR or NS counts. Here is
        // the format of an unnumbered control:
        //   M-M-M-P/F-M-M-1-1

        memcpy( &pLLC->bLpdu, (BYTE*)pLPDU, 3);
        pLLC->hwLpduSize = 3;

        pLLC->hwPF = ((pLPDU->bControl[0] & 0x10) >> 4);
        pLLC->hwM =  ((pLPDU->bControl[0] & 0xE0) >> 3);
        pLLC->hwM |= ((pLPDU->bControl[0] & 0x0C) >> 2);

        if (pLLC->hwM == M_FRMR_Response)
        {
            if (iLPDUsize < 8)
            {
                return 0;    // Less than 8 bytes!
            }
            memcpy( pLLC->bInfo, (BYTE*)pLPDU + 3, 5);
            pLLC->hwInfoSize = 5;

            pLLC->hwNS = (pLLC->bInfo[2] >> 1);
            pLLC->hwNR = (pLLC->bInfo[3] >> 1);
            pLLC->hwV  = ((pLLC->bInfo[4] & 0x80) >> 4);
            pLLC->hwZ  = ((pLLC->bInfo[4] & 0x08) >> 3);
            pLLC->hwY  = ((pLLC->bInfo[4] & 0x04) >> 2);
            pLLC->hwX  = ((pLLC->bInfo[4] & 0x02) >> 1);
            pLLC->hwW  = (pLLC->bInfo[4] & 0x01);
            FETCH_HW( pLLC->hwCF, pLLC->bInfo );

            iLPDUsize = 8;
        }
        else
        {
            iLPDUsize = 3;
        }

        break;

    default:
        // Unreachable!
        return 0;    // Err...

    }

    return iLPDUsize;
}

// ====================================================================
// Build the complex LLC PDU bits and bytes from simple variables.
// ====================================================================
//
// On entry to this function pStart should point to the location where
// the LLC PDU is to be built.
//
// On return from this function the length of the LLC in the PDU is
// returned. If the request is invalid zero is returned.
//
int  BuildLLC( PLLC pLLC, BYTE* pStart )
{
    PLPDU     pLPDU;
    int       iLPDUsize;
    char      bInfo[6];


    pLPDU = (PLPDU)pStart;

    pLPDU->bDSAP[0] = pLLC->hwDSAP;
    pLPDU->bDSAP[0] |= pLLC->hwIG;
    pLPDU->bSSAP[0] = pLLC->hwSSAP;
    pLPDU->bSSAP[0] |= pLLC->hwCR;

    switch (pLLC->hwType)
    {
    case Type_Unnumbered_Frame:
        // Unnumbered Frame.
        // The control field should be 1 byte.
        // Unnumbered frames provide link control functions, for
        // example, mode setting commands and responses. In some
        // cases, unnumbered information frames can also be sent.
        // They do not contain fields for NR or NS counts. Here is
        // the format of an unnumbered control:
        //   M-M-M-P/F-M-M-1-1

        pLPDU->bControl[0] = ((pLLC->hwM & 0x1C) << 3);
        pLPDU->bControl[0] |= (pLLC->hwPF << 4);
        pLPDU->bControl[0] |= ((pLLC->hwM & 0x03) << 2);
        pLPDU->bControl[0] |= 0x03;

        if (pLLC->hwM == M_FRMR_Response)
        {
            STORE_HW( bInfo, pLLC->hwCF );
            bInfo[2] =  (pLLC->hwNS << 1);
            bInfo[3] =  (pLLC->hwNR << 1);
            bInfo[4] =  (pLLC->hwV << 4);
            bInfo[4] |= (pLLC->hwZ << 3);
            bInfo[4] |= (pLLC->hwY << 2);
            bInfo[4] |= (pLLC->hwX << 1);
            bInfo[4] |= (pLLC->hwW);
            memcpy( (BYTE*)pLPDU + 3, bInfo, 5);
            iLPDUsize = 8;
        }
        else
        {
            iLPDUsize = 3;
        }

        break;

    case Type_Supervisory_Frame:
        // Supervisory Frame.
        // The control field should be 2 bytes.
        // Supervisory frames perform supervisory control functions,
        // for example, to acknowledge I Frames (RR), to request
        // retransmission of I frames (REJ), and to request temporary
        // suspension (RNR) of I frames. Supervisory frames do not
        // contain an information field. Therefore, supervisory frames
        // do not affect the NS in the sending station, and so do not
        // contain an NS field. Here is the format of a supervisory
        // control:-
        //   0-0-0-0-S-S-0-1-NR-NR-NR-NR-NR-NR-NR-P/F

        pLPDU->bControl[0] = (pLLC->hwSS << 2);
        pLPDU->bControl[0] |= 0x01;
        pLPDU->bControl[1] = (pLLC->hwNR << 1);
        pLPDU->bControl[1] |= pLLC->hwPF;

        iLPDUsize = 4;

        break;

    case Type_Information_Frame:
        // Information Frame (I Frame).
        // The control field should be 2 bytes.
        // The format of the I frame contains an NS and NR count. The
        // NS count is the sequence number (modulo 128) of the LPDU
        // currently in transmission. The NR count is the sequence
        // number of the next LPDU I frame that the sender expects to
        // receive. To help you later, remember that NR means "next
        // receive." Here is the format of an information control:
        //   NS-NS-NS-NS-NS-NS-NS-0-NR-NR-NR-NR-NR-NR-P/F

        pLPDU->bControl[0] = (pLLC->hwNS << 1);
        pLPDU->bControl[1] = (pLLC->hwNR << 1);
        pLPDU->bControl[1] |= pLLC->hwPF;

        iLPDUsize = 4;

        break;

    default:
        return 0;    // Err...

    }

    return iLPDUsize;
}


/* ------------------------------------------------------------------ */
/* alloc_lcs_buffer(): Allocate storage for an LCSIBH and data        */
/* ------------------------------------------------------------------ */
PLCSIBH  alloc_lcs_buffer( PLCSDEV pLCSDEV, int iSize )
{
    DEVBLK*    pDEVBLK;
    PLCSIBH    pLCSIBH;                // LCSIBH
    int        iBufLen;                // Buffer length
    char       etext[40];              // malloc error text


    // Allocate the buffer.
    iBufLen = sizeof(LCSIBH) + iSize;
    pLCSIBH = calloc( iBufLen, 1 );    // Allocate and clear the buffer
    if (pLCSIBH)                       // if the allocate was successful...
    {
        pLCSIBH->iAreaLen = iSize;
    }
    else                               // ohdear, the allocate was not successful...
    {
        pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
        // Report the bad news.
        MSGBUF( etext, "malloc(%d)", iBufLen );
        // HHC00900 "%1d:%04X %s: error in function %s: %s"
        WRMSG(HHC00900, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             etext, strerror(errno) );
    }

    return pLCSIBH;
}

/* ------------------------------------------------------------------ */
/* add_lcs_buffer_to_chain(): Add LCSIBH to end of chain.             */
/* ------------------------------------------------------------------ */
void  add_lcs_buffer_to_chain( PLCSDEV pLCSDEV, PLCSIBH pLCSIBH )
{

    // Prepare LCSIBH for adding to chain.
    if (!pLCSIBH)                                      // Any LCSIBH been passed?
        return;
    pLCSIBH->pNextLCSIBH = NULL;                       // Clear the pointer to next LCSIBH

    // Obtain the buffer chain lock.
    obtain_lock( &pLCSDEV->LCSIBHChainLock );

    // Add LCSIBH to end of chain.
    if (pLCSDEV->pFirstLCSIBH)                         // if there are already LCSIBHs
    {
        pLCSDEV->pLastLCSIBH->pNextLCSIBH = pLCSIBH;   // Add the LCSIBH to
        pLCSDEV->pLastLCSIBH = pLCSIBH;                // the end of the chain
    }
    else
    {
        pLCSDEV->pFirstLCSIBH = pLCSIBH;               // Make the LCSIBH
        pLCSDEV->pLastLCSIBH = pLCSIBH;                // the only LCSIBH
    }

    // Release the buffer chain lock.
    release_lock( &pLCSDEV->LCSIBHChainLock );

    return;
}

/* ------------------------------------------------------------------ */
/* remove_lcs_buffer_from_chain(): Remove LCSIBH from start of chain. */
/* ------------------------------------------------------------------ */
PLCSIBH  remove_lcs_buffer_from_chain( PLCSDEV pLCSDEV )
{
    PLCSIBH    pLCSIBH;                                // LCSIBH

    // Obtain the buffer chain lock.
    obtain_lock( &pLCSDEV->LCSIBHChainLock );

    // Point to first LCSIBH on the chain.
    pLCSIBH = pLCSDEV->pFirstLCSIBH;                   // Pointer to first LCSIBH

    // Remove the first LCSIBH from the chain, if there is one...
    if (pLCSIBH)                                       // If there is a LCSIBH
    {
        pLCSDEV->pFirstLCSIBH = pLCSIBH->pNextLCSIBH;  // Make the next the first LCSIBH
        if (!pLCSDEV->pFirstLCSIBH)                    // if there are no more LCSIBHs
        {
//          pLCSDEV->pFirstLCSIBH = NULL;              // Clear
            pLCSDEV->pLastLCSIBH = NULL;               // the chain
        }
        pLCSIBH->pNextLCSIBH = NULL;                   // Clear the pointer to next LCSIBH
    }

    // Release the buffer chain lock.
    release_lock( &pLCSDEV->LCSIBHChainLock );

    return pLCSIBH;
}

/* ------------------------------------------------------------------ */
/* remove_and_free_any_lcs_buffers_on_chain(): Remove & free LCSIBHs. */
/* ------------------------------------------------------------------ */
void  remove_and_free_any_lcs_buffers_on_chain( PLCSDEV pLCSDEV )
{
    PLCSIBH    pLCSIBH;                                // LCSIBH

    // Obtain the buffer chain lock.
    obtain_lock( &pLCSDEV->LCSIBHChainLock );

    // Remove and free the first LCSIBH on the chain, if there is one...
    while(pLCSDEV->pFirstLCSIBH)
    {
        pLCSIBH = pLCSDEV->pFirstLCSIBH;               // Pointer to first LCSIBH
        pLCSDEV->pFirstLCSIBH = pLCSIBH->pNextLCSIBH;  // Make the next the first LCSIBH
        free_lcs_buffer( pLCSDEV, pLCSIBH );           // Free the buffer
        pLCSIBH = NULL;
    }

    // Reset the chain pointers.
    pLCSDEV->pFirstLCSIBH = NULL;                      // Clear the
    pLCSDEV->pLastLCSIBH = NULL;                       // chain pointers

    // Release the buffer chain lock.
    release_lock( &pLCSDEV->LCSIBHChainLock );

    return;
}

/* ------------------------------------------------------------------ */
/* free_lcs_buffer(): Free LCSIBH.                                    */
/* ------------------------------------------------------------------ */
void  free_lcs_buffer( PLCSDEV pLCSDEV, PLCSIBH pLCSIBH )
{
    UNREFERENCED( pLCSDEV );

    free( pLCSIBH );
    return;
}


/* ------------------------------------------------------------------ */
/* alloc_connection(): Allocate storage for an LCSCONN                */
/* ------------------------------------------------------------------ */
PLCSCONN  alloc_connection( PLCSDEV pLCSDEV )
{
    DEVBLK*    pDEVBLK;
    PLCSCONN   pLCSCONN;
    char       etext[40];              // malloc text


    // Allocate the connection.                                                                                                                                                                        d
    pLCSCONN = calloc( sizeof(LCSCONN), 1 );     // Allocate and clear the connection
    if (!pLCSCONN)                               // if the allocate was not successful...
    {
        pDEVBLK = pLCSDEV->pDEVBLK[ LCS_READ_SUBCHANN ];
        // Report the bad news.
        MSGBUF( etext, "malloc(%d)", (int)sizeof(LCSCONN) );
        // HHC00900 "%1d:%04X %s: error in function %s: %s"
        WRMSG(HHC00900, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             etext, strerror(errno) );
    }
    return pLCSCONN;
}

/* ------------------------------------------------------------------ */
/* add_connection_to_chain(): Add LCSCONN to end of chain.            */
/* ------------------------------------------------------------------ */
void  add_connection_to_chain( PLCSDEV pLCSDEV, PLCSCONN pLCSCONN )
{

    // Prepare LCSCONN for adding to chain.
    if (!pLCSCONN)                                       // Any LCSCONN been passed?
        return;                                          // No, the caller is a fool
    pLCSCONN->pNextLCSCONN = NULL;                       // Clear the pointer to next LCSCONN

    // Obtain the connection chain lock.
    obtain_lock( &pLCSDEV->LCSCONNChainLock );

    // Add the LCSCONN to the start of the chain.
    pLCSCONN->pNextLCSCONN = pLCSDEV->pFirstLCSCONN;
    pLCSDEV->pFirstLCSCONN = pLCSCONN;

    // Release the connection chain lock.
    release_lock( &pLCSDEV->LCSCONNChainLock );

    return;
}

/* ------------------------------------------------------------------ */
/* find_connection_by_remote_mac(): Find LCSCONN.                     */
/* ------------------------------------------------------------------ */
PLCSCONN  find_connection_by_remote_mac( PLCSDEV pLCSDEV, MAC* pMAC )
{
    PLCSCONN   pLCSCONN;

    // Locate the LCSCONN on the chain.
    for (pLCSCONN = pLCSDEV->pFirstLCSCONN; pLCSCONN; pLCSCONN = pLCSCONN->pNextLCSCONN)
    {
        if ( ( memcmp( &pLCSCONN->bRemoteMAC, pMAC, IFHWADDRLEN ) == 0 ) )
        {
            break;
        }
    }

    return pLCSCONN;
}

/* ------------------------------------------------------------------ */
/* find_connection_by_outbound_token(): Find LCSCONN.                 */
/* ------------------------------------------------------------------ */
PLCSCONN find_connection_by_outbound_token( PLCSDEV pLCSDEV, BYTE* pToken )
{
    PLCSCONN   pLCSCONN;

    for (pLCSCONN = pLCSDEV->pFirstLCSCONN; pLCSCONN; pLCSCONN = pLCSCONN->pNextLCSCONN)
    {
        if ( memcmp( &pLCSCONN->bOutToken, pToken, sizeof(pLCSCONN->bOutToken) ) == 0 )
        {
            break;
        }
    }

    return pLCSCONN;
}

/* ------------------------------------------------------------------ */
/* find_connection_by_inbound_token(): Find LCSCONN.                  */
/* ------------------------------------------------------------------ */
PLCSCONN find_connection_by_inbound_token( PLCSDEV pLCSDEV, BYTE* pToken )
{
    PLCSCONN   pLCSCONN;

    for (pLCSCONN = pLCSDEV->pFirstLCSCONN; pLCSCONN; pLCSCONN = pLCSCONN->pNextLCSCONN)
    {
        if ( memcmp( &pLCSCONN->bInToken, pToken, sizeof(pLCSCONN->bInToken) ) == 0 )
        {
            break;
        }
    }

    return pLCSCONN;
}

//??  /* ------------------------------------------------------------------ */
//??  /* find_connection_by_token_and_mac(): Find LCSCONN.                  */
//??  /* ------------------------------------------------------------------ */
//??  PLCSCONN find_connection_by_token_and_mac( PLCSDEV pLCSDEV, BYTE* pToken, int iWhichToken, MAC* pMAC, int iWhichMAC )
//??  {
//??      PLCSCONN   pLCSCONN;
//??      int        iFoundToken ;
//??      int        iFoundMAC;
//??
//??      for (pLCSCONN = pLCSDEV->pFirstLCSCONN; pLCSCONN; pLCSCONN = pLCSCONN->pNextLCSCONN)
//??      {
//??          iFoundToken = FALSE;
//??          iFoundMAC = FALSE;
//??          // Check for the token.
//??          if (pToken)
//??          {
//??              switch (iWhichToken)
//??              {
//??              case 1:
//??                  if ( memcmp( &pLCSCONN->bToken1, pToken, sizeof(pLCSCONN->bToken1) ) == 0 )
//??                  {
//??                      iFoundToken = TRUE;
//??                  }
//??                  break;
//??              case 2:
//??                  if ( memcmp( &pLCSCONN->bToken2, pToken, sizeof(pLCSCONN->bToken2) ) == 0 )
//??                  {
//??                      iFoundToken = TRUE;
//??                  }
//??                  break;
//??              case 3:
//??                  if ( memcmp( &pLCSCONN->bToken3, pToken, sizeof(pLCSCONN->bToken3) ) == 0 )
//??                  {
//??                      iFoundToken = TRUE;
//??                  }
//??                  break;
//??              case 4:
//??                  if ( memcmp( &pLCSCONN->bToken4, pToken, sizeof(pLCSCONN->bToken4) ) == 0 )
//??                  {
//??                      iFoundToken = TRUE;
//??                  }
//??                  break;
//??              case 5:
//??                  if ( memcmp( &pLCSCONN->bToken5, pToken, sizeof(pLCSCONN->bToken5) ) == 0 )
//??                  {
//??                      iFoundToken = TRUE;
//??                  }
//??                  break;
//??              case 6:
//??                  if ( memcmp( &pLCSCONN->bToken6, pToken, sizeof(pLCSCONN->bToken6) ) == 0 )
//??                  {
//??                      iFoundToken = TRUE;
//??                  }
//??                  break;
//??              }
//??              if ( !iFoundToken )
//??              {
//??                  continue;  // On to the next LCSCONN in the chain.
//??              }
//??          }
//??          // Check for the MAC address.
//??          if (pMAC)
//??          {
//??              switch (iWhichMAC)
//??              {
//??              case 1:
//??                  if ( memcmp( &pLCSCONN->bLocalMAC, pMAC, IFHWADDRLEN ) == 0 )
//??                  {
//??                      iFoundMAC = TRUE;
//??                  }
//??                  break;
//??              case 2:
//??                  if ( memcmp( &pLCSCONN->bRemoteMAC, pMAC, IFHWADDRLEN ) == 0 )
//??                  {
//??                      iFoundMAC = TRUE;
//??                  }
//??                  break;
//??              }
//??              if ( !iFoundMAC )
//??              {
//??                  continue;  // On to the next LCSCONN in the chain.
//??              }
//??          }
//??          // The token and/or the MAC address have been found.
//??          break;
//??      }
//??
//??      return pLCSCONN;
//??  }

/* ------------------------------------------------------------------ */
/* remove_connection_from_chain(): Remove LCSCONN from chain.         */
/* ------------------------------------------------------------------ */
/* -1  LCSCONN pointer was zero.                                      */
/*  0  LCSCONN removed from chain.                                    */
/*  1  LCSCONN was not found on the chain.                            */
/* ------------------------------------------------------------------ */
int  remove_connection_from_chain( PLCSDEV pLCSDEV, PLCSCONN pLCSCONN )
{
    PLCSCONN   pCurrLCSCONN;
    PLCSCONN* ppPrevLCSCONN;
    int        rc;

    // Prepare LCSCONN for removing from chain.
    if (!pLCSCONN)                                    // Any LCSCONN been passed?
        return -1;                                    // No, the caller is a fool

    // Obtain the connection chain lock.
    obtain_lock( &pLCSDEV->LCSCONNChainLock );

    // Remove the LCSCONN from the chain.
    rc = 1;
    ppPrevLCSCONN = &pLCSDEV->pFirstLCSCONN;
    for (pCurrLCSCONN = pLCSDEV->pFirstLCSCONN; pCurrLCSCONN; pCurrLCSCONN = pCurrLCSCONN->pNextLCSCONN)
    {
        if (pCurrLCSCONN == pLCSCONN)
        {
            *ppPrevLCSCONN = pCurrLCSCONN->pNextLCSCONN;
            pCurrLCSCONN->pNextLCSCONN = NULL;
            rc = 0;
            break;
        }
        ppPrevLCSCONN = &pCurrLCSCONN->pNextLCSCONN;
    }

    // Release the connection chain lock.
    release_lock( &pLCSDEV->LCSCONNChainLock );

    return rc;
}

/* ------------------------------------------------------------------ */
/* remove_and_free_any_connections_on_chain(): Remove & free LCSCONNs */
/* ------------------------------------------------------------------ */
void  remove_and_free_any_connections_on_chain( PLCSDEV pLCSDEV )
{
    PLCSCONN    pLCSCONN;

    // Obtain the connection chain lock.
    obtain_lock( &pLCSDEV->LCSCONNChainLock );

    // Remove and free the first LCSCONN on the chain, if there is one...
    while( pLCSDEV->pFirstLCSCONN )
    {
        pLCSCONN = pLCSDEV->pFirstLCSCONN;                 // Pointer to first LCSCONN
        pLCSDEV->pFirstLCSCONN = pLCSCONN->pNextLCSCONN;   // Make the next the first LCSCONN
        free_connection( pLCSDEV, pLCSCONN );              // Free the connection
        pLCSCONN = NULL;
    }

    // Release the connection chain lock.
    release_lock( &pLCSDEV->LCSCONNChainLock );

    return;
}

/* ------------------------------------------------------------------ */
/* free_connections(): Free LCSCONN                                   */
/* ------------------------------------------------------------------ */
void  free_connection( PLCSDEV pLCSDEV, PLCSCONN pLCSCONN )
{
    UNREFERENCED( pLCSDEV );

    memset( pLCSCONN, 0x4f, sizeof(LCSCONN) );
    free( pLCSCONN );
    return;
}


// ====================================================================
//                 Device Handler Information
// ====================================================================

/* NOTE : lcs_device_hndinfo is NEVER static as it is referenced by the CTC meta driver */
DEVHND lcs_device_hndinfo =
{
        &LCS_Init,                     /* Device Initialization      */
        &LCS_ExecuteCCW,               /* Device CCW execute         */
        &LCS_Close,                    /* Device Close               */
        &LCS_Query,                    /* Device Query               */
        NULL,                          /* Device Extended Query      */
        &LCS_StartChannelProgram,      /* Device Start channel pgm   */
        &LCS_EndChannelProgram,        /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &lcs_halt_or_clear,            /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        CTC_Immed_Commands,            /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};


/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */

#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev hdt3088_LTX_hdl_ddev
#define hdl_depc hdt3088_LTX_hdl_depc
#define hdl_reso hdt3088_LTX_hdl_reso
#define hdl_init hdt3088_LTX_hdl_init
#define hdl_fini hdt3088_LTX_hdl_fini
#endif

HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
     HDL_DEPENDENCY(DEVBLK);
}
END_DEPENDENCY_SECTION

HDL_REGISTER_SECTION;       // ("Register" our entry-points)

//                 Hercules's          Our
//                 registered          overriding
//                 entry-point         entry-point
//                 name                value

#if defined( WIN32 )
  HDL_REGISTER ( debug_tt32_stats,   display_tt32_stats        );
  HDL_REGISTER ( debug_tt32_tracing, enable_tt32_debug_tracing );
#else
  UNREFERENCED( regsym );   // (HDL_REGISTER_SECTION parameter)
#endif

END_REGISTER_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE( LCS, lcs_device_hndinfo );

// ZZ the following device types should be moved to
// ZZ their own loadable modules

    HDL_DEVICE( CTCI, ctci_device_hndinfo );
    HDL_DEVICE( CTCT, ctct_device_hndinfo );
    HDL_DEVICE( CTCE, ctce_device_hndinfo );
}
END_DEVICE_SECTION

#if defined( _MSVC_ ) && defined( NO_LCS_OPTIMIZE )
  #pragma optimize( "", on )            // restore previous settings
#endif

#endif /* !defined(__SOLARIS__)  jbs 10/2007 10/2007 */
