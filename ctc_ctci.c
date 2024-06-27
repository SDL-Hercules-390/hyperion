/* CTC_CTCI.C   (C) Copyright Roger Bowler, 2000-2012                */
/*              (C) Copyright James A. Pierson, 2002-2009            */
/*              (C) Copyright "Fish" (David B. Trout), 2002-2009     */
/*              (C) Copyright Fritz Elfert, 2001-2009                */
/*              Hercules IP Channel-to-Channel Support (CTCI)        */
/*                                                                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*********************************************************************/
/* Please keep to this terminology:                                  */
/*                                                                   */
/* *  A  character  special device file is a file system object, for */
/*    example  /dev/net/tun.  It can be opened if permissions allow. */
/*    The ls command shows such objects:                             */
/*    crw-rw---- 1 root staff 10, 200 Nov 24 15:01 /dev/net/tun      */
/*    Permissions are set when the special device file is created by */
/*    mknod  or  by  a udev rule invoked as a result of creating the */
/*    object.                                                        */
/*                                                                   */
/* *  A network interface represents an IP adapter, real or virtual. */
/*    Interfaces  are  displayed  and  controlled  by  the  ifconfig */
/*    command (to be replaced by the ip command).                    */
#if 0
   tun0   Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00
          inet addr:192.168.2.65  P-t-P:10.0.0.33  Mask:255.255.255.255
          UP POINTOPOINT RUNNING NOARP MULTICAST  MTU:1500  Metric:1
          RX packets:70834 errors:0 dropped:0 overruns:0 frame:0
          TX packets:47 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:100
          RX bytes:4262823 (4.0 MiB)  TX bytes:2602 (2.5 KiB)
#endif
/*                                                                   */
/* Opening the character special file obtains a file handle that can */
/* be  associated  with  an  interface.   No special permissions are */
/* required  to  open  the  character special file, as you cannot do */
/* anything  with the file descriptor until you associate it with an */
/* interface.   The  operation  of  associating a newly created file */
/* descriptor  with  interface  requires  root privileges unless the */
/* interface  was  created  by, e.g., openvpn which can set the user */
/* and  group  that  my  associate  the interface.  So this is where */
/* hercifc comes into play.                                          */
/*                                                                   */
/* A preconfigured interface is assumed when only the interface name */
/* is specified.                                                     */
/*********************************************************************/

#include "hstdinc.h"

/* jbs 10/27/2007 added _SOLARIS_   silly typo fixed 01/18/08 when looked at this again */
#if !defined(__SOLARIS__)

#include "hercules.h"
#include "ctcadpt.h"
#include "tuntap.h"
#include "opcode.h"
/* getopt dynamic linking kludge */
#include "herc_getopt.h"

/*-------------------------------------------------------------------*/
/* Ivan Warren 20040227                                              */
/*                                                                   */
/* This table is used by channel.c to determine if a CCW code        */
/* is an immediate command or not.                                   */
/*                                                                   */
/* The table is addressed in the DEVHND structure as 'DEVIMM immed'  */
/*                                                                   */
/*     0:  ("false")  Command is *NOT* an immediate command          */
/*     1:  ("true")   Command *IS* an immediate command              */
/*                                                                   */
/* Note: An immediate command is defined as a command which returns  */
/* CE (channel end) during initialization (that is, no data is       */
/* actually transfered). In this case, IL is not indicated for a     */
/* Format 0 or Format 1 CCW when IL Suppression Mode is in effect.   */
/*                                                                   */
/*-------------------------------------------------------------------*/

static BYTE CTCI_Immed_Commands[256]=
{ 0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// ====================================================================
// Declarations
// ====================================================================

static void     ctc_halt_or_clear( DEVBLK* pDEVBLK );

static void*    CTCI_ReadThread( void* arg /*PCTCBLK pCTCBLK */ );

static int      CTCI_EnqueueIPFrame( DEVBLK* pDEVBLK,
                                     BYTE*   pData, size_t iSize );

static int      ParseArgs( DEVBLK* pDEVBLK, PCTCBLK pCTCBLK,
                           int argc, char** argv );

// --------------------------------------------------------------------
// Device Handler Information Block
// --------------------------------------------------------------------

DEVHND ctci_device_hndinfo =
{
        &CTCI_Init,                    /* Device Initialization      */
        &CTCI_ExecuteCCW,              /* Device CCW execute         */
        &CTCI_Close,                   /* Device Close               */
        &CTCI_Query,                   /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &ctc_halt_or_clear,            /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        CTCI_Immed_Commands,           /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

// ====================================================================
//
// ====================================================================

//
// CTCI_Init
//

#define CTC_DEVICES_IN_GROUP   2  // a read and write device

int  CTCI_Init( DEVBLK* pDEVBLK, int argc, char *argv[] )
{
    CTCBLK          wblk;
    PCTCBLK         pWrkCTCBLK = &wblk; // Working CTCBLK
    PCTCBLK         pDevCTCBLK = NULL;  // Device  CTCBLK
    int             rc = 0;             // Return code
    int             nIFFlags;           // Interface flags
    char            thread_name[32];    // CTCI_ReadThread

    nIFFlags =               // Interface flags
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

    pDEVBLK->devtype = 0x3088;
    pDEVBLK->excps = 0;

    // CTC is a group device
    if(!group_device(pDEVBLK, CTC_DEVICES_IN_GROUP))
        return 0;

    // Housekeeping
    memset( pWrkCTCBLK, 0, sizeof( CTCBLK ) );

    // Parse configuration file statement
    if( ParseArgs( pDEVBLK, pWrkCTCBLK, argc, (char**)argv ) != 0 )
        return -1;

    // Allocate the device CTCBLK and copy parsed information.

    pDevCTCBLK = malloc( sizeof( CTCBLK ) );

    if( !pDevCTCBLK )
    {
        char buf[40];
        MSGBUF(buf, "malloc(%d)", (int)sizeof(CTCBLK));
        // "%1d:%04X %s: error in function %s: %s"
        WRMSG(HHC00900, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI", buf, strerror(errno) );
        return -1;
    }

    memcpy( pDevCTCBLK, pWrkCTCBLK, sizeof( CTCBLK ) );

    // New format has only one device statement for both addresses
    // We need to dynamically allocate the read device block

    pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ] = pDEVBLK->group->memdev[0];
    pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ] = pDEVBLK->group->memdev[1];

    pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ]->dev_data = pDevCTCBLK;
    pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ]->dev_data = pDevCTCBLK;

    SetSIDInfo( pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ], 0x3088, 0x08, 0x3088, 0x01 );
    SetSIDInfo( pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ], 0x3088, 0x08, 0x3088, 0x01 );

    pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ]->ctctype  = CTC_CTCI;
    pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ]->ctcxmode = 1;

    pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ]->ctctype  = CTC_CTCI;
    pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ]->ctcxmode = 1;

    pDevCTCBLK->sMTU                = atoi( pDevCTCBLK->szMTU );
    pDevCTCBLK->iMaxFrameBufferSize = sizeof(pDevCTCBLK->bFrameBuffer);

    initialize_lock( &pDevCTCBLK->Lock );
    initialize_lock( &pDevCTCBLK->EventLock );
    initialize_condition( &pDevCTCBLK->Event );

    // Give both Herc devices a reasonable name...

    STRLCPY( pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ]->filename, pDevCTCBLK->szTUNCharDevName );
    STRLCPY( pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ]->filename, pDevCTCBLK->szTUNCharDevName );

    /* It  might  be  tempting  to  add IFF_TUN_EXCL to the flags to */
    /* avoid  a  race,  but it does not work like open exclusive; it */
    /* would appear that the bit is permanent so that hercifc cannot */
    /* configure the interface.                                      */
    rc = TUNTAP_CreateInterface( pDevCTCBLK->szTUNCharDevName,
#if defined(BUILD_HERCIFC)
                                 (pDevCTCBLK->fPreconfigured ? IFF_NO_HERCIFC : 0) |
#endif // __HERCIFC_H_
                                 IFF_TUN | IFF_NO_PI,
                                 &pDevCTCBLK->fd,
                                 pDevCTCBLK->szTUNIfName );

    if( rc < 0 ) return -1;

    // HHC00901 "%1d:%04X %s: interface %s, type %s opened"
    WRMSG(HHC00901, "I", SSID_TO_LCSS(pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->ssid), pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->devnum,
                         pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->typname,
                         pDevCTCBLK->szTUNIfName, "TUN");

    if (!pDevCTCBLK->fPreconfigured)
    {

#if defined(OPTION_W32_CTCI)
        // Set the specified driver/dll i/o buffer sizes..
        {
            struct tt32ctl tt32ctl;

            memset( &tt32ctl, 0, sizeof(tt32ctl) );
            STRLCPY( tt32ctl.tt32ctl_name, pDevCTCBLK->szTUNIfName );

            tt32ctl.tt32ctl_devbuffsize = pDevCTCBLK->iKernBuff;
            if( TUNTAP_IOCtl( pDevCTCBLK->fd, TT32SDEVBUFF, (char*)&tt32ctl ) != 0  )
            {
                // "%1d:%04X %s: ioctl '%s' failed for device '%s': '%s'"
                WRMSG(HHC00902, "W", SSID_TO_LCSS(pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->ssid), pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->devnum,
                      pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->typname,
                      "TT32SDEVBUFF", pDevCTCBLK->szTUNIfName, strerror( errno ) );
            }

            tt32ctl.tt32ctl_iobuffsize = pDevCTCBLK->iIOBuff;
            if( TUNTAP_IOCtl( pDevCTCBLK->fd, TT32SIOBUFF, (char*)&tt32ctl ) != 0  )
            {
                // "%1d:%04X %s: ioctl '%s' failed for device '%s': '%s'"
                WRMSG(HHC00902, "W", SSID_TO_LCSS(pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->ssid), pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->devnum,
                      pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->typname,
                      "TT32SIOBUFF", pDevCTCBLK->szTUNIfName, strerror( errno ) );
            }
        }

#ifdef OPTION_TUNTAP_SETMACADDR
        if( !pDevCTCBLK->szMACAddress[0] )   // (if MAC address unspecified)
        {
            in_addr_t  wrk_guest_ip_addr;
            MAC        wrk_guest_mac_addr;

            if ((in_addr_t)-1 != (wrk_guest_ip_addr = inet_addr( pDevCTCBLK->szGuestIPAddr )))
            {
                build_herc_iface_mac ( wrk_guest_mac_addr, (const BYTE*) &wrk_guest_ip_addr );

                MSGBUF( pDevCTCBLK->szMACAddress,
                        "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
                        ,wrk_guest_mac_addr[0]
                        ,wrk_guest_mac_addr[1]
                        ,wrk_guest_mac_addr[2]
                        ,wrk_guest_mac_addr[3]
                        ,wrk_guest_mac_addr[4]
                        ,wrk_guest_mac_addr[5] );
            }
        }

        TRACE
        (
            "** CTCI_Init: %4.4X (%s): IP \"%s\"  -->  default MAC \"%s\"\n"

            ,pDevCTCBLK->pDEVBLK[CTC_READ_SUBCHANN]->devnum
            ,pDevCTCBLK->szTUNIfName
            ,pDevCTCBLK->szGuestIPAddr
            ,pDevCTCBLK->szMACAddress
        );

        VERIFY( TUNTAP_SetMACAddr ( pDevCTCBLK->szTUNIfName, pDevCTCBLK->szMACAddress  ) == 0 );
#endif

#ifdef OPTION_TUNTAP_CLRIPADDR
        VERIFY( TUNTAP_ClrIPAddr  ( pDevCTCBLK->szTUNIfName ) == 0 );
#endif
#endif

        VERIFY( TUNTAP_SetIPAddr  ( pDevCTCBLK->szTUNIfName, pDevCTCBLK->szDriveIPAddr ) == 0 );

        VERIFY( TUNTAP_SetDestAddr( pDevCTCBLK->szTUNIfName, pDevCTCBLK->szGuestIPAddr ) == 0 );

#ifdef OPTION_TUNTAP_SETNETMASK
        VERIFY( TUNTAP_SetNetMask ( pDevCTCBLK->szTUNIfName, pDevCTCBLK->szNetMask     ) == 0 );
#endif

        VERIFY( TUNTAP_SetMTU     ( pDevCTCBLK->szTUNIfName, pDevCTCBLK->szMTU         ) == 0 );

        VERIFY( TUNTAP_SetFlags   ( pDevCTCBLK->szTUNIfName, nIFFlags                  ) == 0 );

    }

    // Copy the fd to make panel.c happy
    pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ]->fd =
    pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ]->fd = pDevCTCBLK->fd;

    MSGBUF(thread_name, "CTCI %4.4X ReadThread", pDEVBLK->devnum);
    rc = create_thread( &pDevCTCBLK->tid, JOINABLE, CTCI_ReadThread, pDevCTCBLK, thread_name );
    if(rc)
    {
       // "Error in function create_thread(): %s"
       WRMSG(HHC00102, "E", strerror(rc));
    }

    pDevCTCBLK->pDEVBLK[ CTC_READ_SUBCHANN  ]->tid = pDevCTCBLK->tid;
    pDevCTCBLK->pDEVBLK[ CTC_WRITE_SUBCHANN ]->tid = pDevCTCBLK->tid;

    return 0;
}


//
// CTCI_ExecuteCCW
//
void  CTCI_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                       BYTE    bFlags,  BYTE  bChained,
                       U32     sCount,  BYTE  bPrevCode,
                       int     iCCWSeq, BYTE* pIOBuf,
                       BYTE*   pMore,   BYTE* pUnitStat,
                       U32*    pResidual )
{
    int             iNum;               // Number of bytes to move
    BYTE            bOpCode;            // CCW opcode with modifier
                                        //   bits masked off

    UNREFERENCED( bFlags    );
    UNREFERENCED( bChained  );
    UNREFERENCED( bPrevCode );
    UNREFERENCED( iCCWSeq   );

    // Intervention required if the device file is not open
    if( pDEVBLK->fd < 0 &&
        !IS_CCW_SENSE( bCode ) &&
        !IS_CCW_CONTROL( bCode ) )
    {
        pDEVBLK->sense[0] = SENSE_IR;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    // Mask off the modifier bits in the CCW bOpCode
    if( ( bCode & 0x07 ) == 0x07 )
        bOpCode = 0x07;
    else if( ( bCode & 0x03 ) == 0x02 )
        bOpCode = 0x02;
    else if( ( bCode & 0x0F ) == 0x0C )
        bOpCode = 0x0C;
    else if( ( bCode & 0x03 ) == 0x01 )
        bOpCode = pDEVBLK->ctcxmode ? ( bCode & 0x83 ) : 0x01;
    else if( ( bCode & 0x1F ) == 0x14 )
        bOpCode = 0x14;
    else if( ( bCode & 0x47 ) == 0x03 )
        bOpCode = 0x03;
    else if( ( bCode & 0xC7 ) == 0x43 )
        bOpCode = 0x43;
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
        if( sCount == 0 )
        {
            *pUnitStat = CSW_CE | CSW_DE;
            break;
        }

        CTCI_Write( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual );
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
        CTCI_Read( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual, pMore );
        break;

    case 0x07:  // MMMMM111  CTL
        // -----------------------------------------------------------
        // CONTROL
        // -----------------------------------------------------------

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
        // -----------------------------------------------------------

        // Command reject if in basic mode
        if( pDEVBLK->ctcxmode == 0 )
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
        // -----------------------------------------------------------

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

    case 0x14:  // XXX10100  SCB
        // -----------------------------------------------------------
        // SENSE COMMAND BYTE
        // -----------------------------------------------------------

        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x04:  // 00000100  SENSE
      // -----------------------------------------------------------
      // SENSE
      // -----------------------------------------------------------

        // Command reject if in basic mode
        if( pDEVBLK->ctcxmode == 0 )
        {
            pDEVBLK->sense[0] = SENSE_CR;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        // Calculate residual byte count
        iNum = ( sCount < pDEVBLK->numsense ) ?
            sCount : pDEVBLK->numsense;

        *pResidual = sCount - iNum;

        if( sCount < pDEVBLK->numsense )
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

        if( sCount < pDEVBLK->numdevid )
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
}

// -------------------------------------------------------------------
// CTCI_Close
// -------------------------------------------------------------------

int  CTCI_Close( DEVBLK* pDEVBLK )
{
    /* DEVBLK* pDEVBLK2; */
    PCTCBLK pCTCBLK  = (PCTCBLK)pDEVBLK->dev_data;

    // Close the device file (if not already closed)
    if( pCTCBLK->fd >= 0 )
    {
        // PROGRAMMING NOTE: there's currently no way to interrupt
        // the "CTCI_ReadThread"s TUNTAP_Read of the adapter. Thus
        // we must simply wait for CTCI_ReadThread to eventually
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

        TID tid = pCTCBLK->tid;
        pCTCBLK->fCloseInProgress = 1;  // (ask read thread to exit)
        join_thread( tid, NULL );       // (wait for thread to end)
#if defined( OPTION_FTHREADS )
        detach_thread( tid );           // only needed for Fish threads
#endif
    }

    pDEVBLK->fd = -1;           // indicate we're now closed

    return 0;
}


// -------------------------------------------------------------------
// CTCI_Query
// -------------------------------------------------------------------

void  CTCI_Query( DEVBLK* pDEVBLK, char** ppszClass,
                  int     iBufLen, char*  pBuffer )
{
    char filename[ PATH_MAX + 1 ];      /* full path or just name    */

    CTCBLK*  pCTCBLK;
    char*    pGuestIP;
    char*    pDriveIP;

    BEGIN_DEVICE_CLASS_QUERY( "CTCA", pDEVBLK, ppszClass, iBufLen, pBuffer );

    pCTCBLK  = (CTCBLK*) pDEVBLK->dev_data;

    if(!pCTCBLK)
    {
        strlcpy(pBuffer,"*Uninitialized",iBufLen);
        return;
    }

    if (strlen( pCTCBLK->szGuestIPAddr ))
       pGuestIP = pCTCBLK->szGuestIPAddr;
    else
       pGuestIP = "-";

    if (strlen( pCTCBLK->szDriveIPAddr ))
       pDriveIP = pCTCBLK->szDriveIPAddr;
    else
       pDriveIP = "-";

    snprintf( pBuffer, iBufLen, "CTCI %s/%s (%s)%s IO[%"PRIu64"]",
              pGuestIP,
              pDriveIP,
              pCTCBLK->szTUNIfName,
              pCTCBLK->fDebug ? " -d" : "",
              pDEVBLK->excps );
    pBuffer[iBufLen-1] = '\0';

    return;
}

// -------------------------------------------------------------------
//                     ctc_halt_or_clear
// -------------------------------------------------------------------
// The channel is processing a Halt Subchannel or Clear Subchannel
// instruction and is notifying us of that fact so we can stop our
// CTCI_Read CCW processing loop.

static void ctc_halt_or_clear( DEVBLK* pDEVBLK )
{
    PCTCBLK pCTCBLK = (PCTCBLK) pDEVBLK->dev_data;
    obtain_lock( &pCTCBLK->EventLock );
    {
        if (pCTCBLK->fReadWaiting)
        {
            pCTCBLK->fHaltOrClear = 1;
            signal_condition( &pCTCBLK->Event );
        }
    }
    release_lock( &pCTCBLK->EventLock );
}

// -------------------------------------------------------------------
// CTCI_Read
// -------------------------------------------------------------------
//
// Once an IP frame is received by the Read Thread, it is enqueued
// on the device frame buffer for presentation to the host program.
// The residual byte count is set to indicate the amount of the buffer
// which was not filled.
//
// For details regarding the actual buffer layout, please refer to
// the comments preceding the CTCI_ReadThread function.

// Input:
//      pDEVBLK   A pointer to the CTC adapter device block
//      sCount    The I/O buffer length from the read CCW
//      pIOBuf    The I/O buffer from the read CCW
//
// Output:
//      pUnitStat The CSW status (CE+DE or CE+DE+UC or CE+DE+UC+SM)
//      pResidual The CSW residual byte count
//      pMore     Set to 1 if packet data exceeds CCW count
//

void  CTCI_Read( DEVBLK* pDEVBLK,   U32   sCount,
                 BYTE*   pIOBuf,    BYTE* pUnitStat,
                 U32*    pResidual, BYTE* pMore )
{
    PCTCBLK     pCTCBLK  = (PCTCBLK)pDEVBLK->dev_data;
    PCTCIHDR    pFrame   = NULL;
    size_t      iLength  = 0;
    int         rc       = 0;
    BYTE        haltorclear = FALSE;

    for ( ; ; )
    {
        obtain_lock( &pCTCBLK->Lock );

        if (!pCTCBLK->fDataPending)
        {
            release_lock( &pCTCBLK->Lock );
            {
                struct timespec waittime;
                struct timeval  now;

                gettimeofday( &now, NULL );

                waittime.tv_sec  = now.tv_sec  + DEF_NET_READ_TIMEOUT_SECS;
                waittime.tv_nsec = now.tv_usec * 1000;

                obtain_lock( &pCTCBLK->EventLock );
                pCTCBLK->fReadWaiting = 1;
                rc = timed_wait_condition( &pCTCBLK->Event,
                                           &pCTCBLK->EventLock,
                                           &waittime );
                pCTCBLK->fReadWaiting = 0;
            }
            // check for halt condition
            if (pCTCBLK->fHaltOrClear)
            {
                haltorclear = TRUE;
                pCTCBLK->fHaltOrClear = 0;
            }
            release_lock( &pCTCBLK->EventLock );

            // check for halt condition
            if (haltorclear)
            {
                if (pDEVBLK->ccwtrace)
                {
                    // "%1d:%04X %s: halt or clear recognized"
                    WRMSG( HHC00904, "I", SSID_TO_LCSS( pDEVBLK->ssid ),
                        pDEVBLK->devnum, "CTCI" );
                }

                *pUnitStat = CSW_CE | CSW_DE;
                *pResidual = sCount;
                return;
            }

            if (rc == ETIMEDOUT || rc == EINTR)
                continue;

            obtain_lock( &pCTCBLK->Lock );
        }

        // Sanity check
        if( pCTCBLK->iFrameOffset == 0 )
        {
            release_lock( &pCTCBLK->Lock );
            continue;
        }

        // Fix-up frame pointer and terminate block
        pFrame = (PCTCIHDR)( pCTCBLK->bFrameBuffer +
                  sizeof( CTCIHDR ) +
                  pCTCBLK->iFrameOffset );

        STORE_HW( pFrame->hwOffset, 0x0000 );

        // (fix for day-1 bug offered by Vince Weaver [vince@deater.net])
//      iLength = pCTCBLK->iFrameOffset + sizeof( CTCIHDR ) + 2;
        iLength = pCTCBLK->iFrameOffset + sizeof( CTCIHDR );

        if( sCount < iLength )
        {
            *pMore     = 1;
            *pResidual = 0;

            iLength    = sCount;
        }
        else
        {
            *pMore      = 0;
            *pResidual -= (U16)iLength;
        }

        *pUnitStat = CSW_CE | CSW_DE;

        memcpy( pIOBuf, pCTCBLK->bFrameBuffer, iLength );

        if( pCTCBLK->fDebug )
        {
            // "%1d:%04X %s: Present data of size %d bytes to guest"
            WRMSG(HHC00982, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI", (int)iLength);
            net_data_trace( pDEVBLK, pCTCBLK->bFrameBuffer, (int)iLength, '>', 'D', "data", 0 );
        }

        // Reset frame buffer
        pCTCBLK->iFrameOffset  = 0;
        pCTCBLK->fDataPending  = 0;

        release_lock( &pCTCBLK->Lock );

        return;
    }
}

// -------------------------------------------------------------------
// CTCI_Write
// -------------------------------------------------------------------
//
// For details regarding the actual buffer layout, please refer to
// the comments preceding the CTCI_ReadThread function.
//

void  CTCI_Write( DEVBLK* pDEVBLK,   U32   sCount,
                  BYTE*   pIOBuf,    BYTE* pUnitStat,
                  U32*    pResidual )
{
    PCTCBLK    pCTCBLK  = (PCTCBLK)pDEVBLK->dev_data;
    PCTCIHDR   pFrame;                  // -> Frame header
    PCTCISEG   pSegment;                // -> Segment in buffer
    U16        sOffset;                 // Offset of next frame
    U16        sSegLen;                 // Current segment length
    U16        sDataLen;                // Length of IP Frame data
    int        iPos;                    // Offset into buffer
    U16        i;                       // Array subscript
    int        rc;                      // Return code
    BYTE       szStackID[33];           // VSE IP stack identity
    U32        iStackCmd;               // VSE IP stack command
    U16        hwType;                  // Ethernet packet type

    // Check that CCW count is sufficient to contain block header
    if( sCount < sizeof( CTCIHDR ) )
    {
        // "%1d:%04X CTC: write CCW count %u is invalid"
        WRMSG(HHC00906, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, sCount );

        pDEVBLK->sense[0] = SENSE_DC;
        *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;

        return;
    }

    //
    if( pCTCBLK->fDebug )
    {
        // HHC00981 "%1d:%04X %s: Accept data of size %d bytes from guest"
        WRMSG(HHC00981, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,  pDEVBLK->typname, (int)sCount );
        net_data_trace( pDEVBLK, pIOBuf, (int)sCount, '<', 'D', "data", 0 );
    }

    // Fix-up frame pointer
    pFrame = (PCTCIHDR)pIOBuf;

    // Extract the frame length from the header
    FETCH_HW( sOffset, pFrame->hwOffset );

    // Check for special VSE TCP/IP stack command packet
    // The following example is from a VSE 5.1 system
    //   +0000  00280026 C9E5E2C5 C3E3C3C1 F0F14BF0   ....IVSECTCA01.0
    //   +0010  F540C640 F0F261F2 F661F0F8 F1F84BF2   5 F 02/26/0818.2
    //   +0020  F4404040 09010001                     4   ....
    if( sCount == 40 )
    {
        if( sOffset == 0 || sOffset == 40 )
        {
            pSegment = (PCTCISEG)(pIOBuf + sizeof( CTCIHDR ));
            FETCH_HW( hwType, pSegment->hwType );
            if (hwType != ETH_TYPE_IP)
            {
                // Extract the 32-byte stack identity string
                for( i = 0;
                     i < sizeof( szStackID ) - 1 && i < sCount - 4;
                     i++)
                    szStackID[i] = guest_to_host( pIOBuf[i+4] );
                szStackID[i] = '\0';

                // Extract the stack command word
                FETCH_FW( iStackCmd, *((FWORD*)&pIOBuf[36]) );

                // Display stack command and discard the packet
                // "%1d:%04X CTC: interface command: '%s' %8.8X"
                WRMSG(HHC00907, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, szStackID, iStackCmd );

                *pUnitStat = CSW_CE | CSW_DE;
                *pResidual = 0;

                return;
            }
        }
    }

    // Check for special L/390 initialization packet
    if( sOffset == 0 )
    {
        // Return normal status and discard the packet
        *pUnitStat = CSW_CE | CSW_DE;
        *pResidual = 0;

        return;
    }

    // Adjust the residual byte count
    *pResidual -= sizeof( CTCIHDR );

    // Process each segment in the buffer
    for( iPos  = sizeof( CTCIHDR );
         iPos  < sOffset;
         iPos += sSegLen )
    {
        // Check that the segment is fully contained within the block
        if( iPos + sizeof( CTCISEG ) > sOffset )
        {
            // "%1d:%04X CTC: incomplete write buffer segment header at offset %4.4X"
            WRMSG(HHC00908, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, iPos );

            pDEVBLK->sense[0] = SENSE_DC;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            return;
        }

        // Fix-up segment header in the I/O buffer
        pSegment = (PCTCISEG)(pIOBuf + iPos);

        // Extract the segment length from the segment header
        FETCH_HW( sSegLen, pSegment->hwLength );

        // Check that the segment length is valid
        if( ( sSegLen        < sizeof( CTCISEG ) ) ||
            ( (U32)iPos + sSegLen > sOffset      ) ||
            ( (U32)iPos + sSegLen > sCount       ) )
        {
            // "%1d:%04X CTC: invalid write buffer segment length %u at offset %4.4X"
            WRMSG(HHC00909, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, sSegLen, iPos );

            pDEVBLK->sense[0] = SENSE_DC;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            return;
        }

        // Calculate length of IP frame data
        sDataLen = sSegLen - sizeof( CTCISEG );

        // Trace the IP packet before sending to TUN device
        if( pCTCBLK->fDebug )
        {
            // "%1d:%04X %s: send%s packet of size %d bytes to device %s"
            WRMSG(HHC00910, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                                 "", sDataLen, pCTCBLK->szTUNIfName );
            net_data_trace( pDEVBLK, pSegment->bData, sDataLen, '<', 'D', "packet", 0 );
        }

        // Write the IP packet to the TUN/TAP interface
        rc = TUNTAP_Write( pCTCBLK->fd, pSegment->bData, sDataLen );

        if( rc < 0 )
        {
            // "%1d:%04X %s: error writing to device %s: %d %s"
            WRMSG(HHC00911, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                                 pCTCBLK->szTUNIfName, errno, strerror( errno ));
            pDEVBLK->sense[0] = SENSE_EC;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            return;
        }

        // Adjust the residual byte count
        *pResidual -= sSegLen;

        // We are done if current segment satisfies CCW count
        if( (U32)iPos + sSegLen == sCount )
        {
            *pResidual -= sSegLen;
            *pUnitStat = CSW_CE | CSW_DE;
            return;
        }
    }

    // Set unit status and residual byte count
    *pUnitStat = CSW_CE | CSW_DE;
    *pResidual = 0;
}

// --------------------------------------------------------------------
// CTCI_ReadThread
// --------------------------------------------------------------------
//
// When an IP frame is received from the TUN/TAP interface, the frame
// is enqueued on the device frame buffer.
//
// The device frame buffer is a chain of blocks. The first 2 bytes of
// a block (CTCIHDR) specify the offset in the buffer of the next block.
// The final block in indicated by a CTCIHDR offset value of 0x0000.
//
// Within each block, each IP frame is preceeded by a segment header
// (CTCISEG). This segment header has a 2 byte length field that
// specifies the length of the segment (including the segment header),
// a 2 byte frame type field (always 0x0800 = IPv4), and a 2 byte
// reserved area (always 0000), followed by the actual frame data.
//
// The CTCI_ReadThread reads the IP frame, then CTCI_EnqueueIPFrame
// function is called to add it to the frame buffer (which precedes
// each one with a CTCISEG and adjusts the block header (CTCIHDR)
// offset value as appropriate.
//
// Oddly, it is the CTCI_Read function (called by CCW processing in
// response to a guest SIO request) that adds the CTCIHDR with the
// 000 offset value marking the end of the buffer's chain of blocks,
// and not the CTCI_EnqueueIPFrame nor the CTCI_ReadThread as would
// be expected.
//
// Also note that the iFrameOffset field in the CTCI device's CTCBLK
// control block is the offset from the end of the buffer's first
// CTCIHDR to where the end-of-chain CTCIHDR is, and is identical to
// all of the queued CTCISEG's hwLength fields added together.
//

static void*  CTCI_ReadThread( void* arg )
{
    PCTCBLK  pCTCBLK = (PCTCBLK) arg;
    DEVBLK*  pDEVBLK = pCTCBLK->pDEVBLK[CTC_READ_SUBCHANN];
    int      iLength;
    BYTE     szBuff[2048];

    // ZZ FIXME: Try to avoid race condition at startup with hercifc
#if defined(BUILD_HERCIFC)
    SLEEP(10);
#endif

    pCTCBLK->pid = getpid();

    while( pCTCBLK->fd != -1 && !pCTCBLK->fCloseInProgress )
    {
        // Read frame from the TUN/TAP interface
        iLength = read_tuntap( pCTCBLK->fd, szBuff, sizeof( szBuff ), DEF_NET_READ_TIMEOUT_SECS );

        // Check for error condition
        if( iLength < 0 )
        {
            if( !pCTCBLK->fCloseInProgress )
            {
                // "%1d:%04X %s: error reading from device %s: %d %s"
                WRMSG(HHC00912, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                                     pCTCBLK->szTUNIfName, errno, strerror( errno ));
            }
            break;
        }

        if( iLength == 0 )      // (probably EINTR; ignore)
            continue;

        if( pCTCBLK->fDebug )
        {
            // "%1d:%04X %s: receive%s packet of size %d bytes from device %s"
            WRMSG(HHC00913, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                                 "", iLength, pCTCBLK->szTUNIfName );
            net_data_trace( pDEVBLK, szBuff, iLength, '>', 'D', "packet", 0 );
        }

        // Enqueue frame on buffer, if buffer is full, keep trying
        while( CTCI_EnqueueIPFrame( pDEVBLK, szBuff, iLength ) < 0
            && pCTCBLK->fd != -1 && !pCTCBLK->fCloseInProgress )
        {
            if( EMSGSIZE == errno )     // (if too large for buffer)
            {
                if( pCTCBLK->fDebug )
                {
                    // "%1d:%04X CTC: packet frame too big, dropped"
                    WRMSG(HHC00914, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum );
                }
                break;                  // (discard it...)
            }

            ASSERT( ENOBUFS == errno );

            // Don't use sched_yield() here; use an actual non-dispatchable
            // delay instead so as to allow another [possibly lower priority]
            // thread to 'read' (remove) some packet(s) from our frame buffer.
            USLEEP( CTC_DELAY_USECS );  // (wait a bit before retrying...)
        }
    }

    // We must do the close since we were the one doing the i/o...

    VERIFY( pCTCBLK->fd == -1 || TUNTAP_Close( pCTCBLK->fd ) == 0 );
    pCTCBLK->fd = -1;

    return NULL;
}

// --------------------------------------------------------------------
// CTCI_EnqueueIPFrame
// --------------------------------------------------------------------
//
// Places the provided IP frame in the next available frame slot in
// the adapter buffer. For details regarding the actual buffer layout
// please refer to the comments preceding the CTCI_ReadThread function.
//
// Returns:
//
//  0 == Success
// -1 == Failure; errno = ENOBUFS:  No buffer space available
//                        EMSGSIZE: Message too long
//
static int  CTCI_EnqueueIPFrame( DEVBLK* pDEVBLK,
                                 BYTE*   pData, size_t iSize )
{
    PCTCIHDR pFrame;
    PCTCISEG pSegment;
    PCTCBLK  pCTCBLK = (PCTCBLK)pDEVBLK->dev_data;

    // Will frame NEVER fit into buffer??
    if( iSize > MAX_CTCI_FRAME_SIZE( pCTCBLK ) || iSize > 9000 )
    {
        errno = EMSGSIZE;   // Message too long
        return -1;          // (-1==failure)
    }

    obtain_lock( &pCTCBLK->Lock );

    // Ensure we dont overflow the buffer
    if( ( pCTCBLK->iFrameOffset +         // Current buffer Offset
          sizeof( CTCIHDR ) +             // Size of Block Header
          sizeof( CTCISEG ) +             // Size of Segment Header
          iSize +                         // Size of Ethernet packet
          sizeof(pFrame->hwOffset) )      // Size of Block terminator
        > pCTCBLK->iMaxFrameBufferSize )  // Size of Frame buffer
    {
        release_lock( &pCTCBLK->Lock );
        errno = ENOBUFS;    // No buffer space available
        return -1;          // (-1==failure)
    }

    // Fix-up Frame pointer
    pFrame = (PCTCIHDR)pCTCBLK->bFrameBuffer;

    // Fix-up Segment pointer
    pSegment = (PCTCISEG)( pCTCBLK->bFrameBuffer +
                           sizeof( CTCIHDR ) +
                           pCTCBLK->iFrameOffset );

    // Initialize segment
    memset( pSegment, 0, iSize + sizeof( CTCISEG ) );

    // Increment offset
    pCTCBLK->iFrameOffset += (U16)(sizeof( CTCISEG ) + iSize);

    // Update next frame offset
    STORE_HW( pFrame->hwOffset,
              pCTCBLK->iFrameOffset + sizeof( CTCIHDR ) );

    // Store segment length
    STORE_HW( pSegment->hwLength, (U16)(sizeof( CTCISEG ) + iSize) );

    // Store Frame type
    STORE_HW( pSegment->hwType, ETH_TYPE_IP );

    // Copy data
    memcpy( pSegment->bData, pData, iSize );

    // Mark data pending
    pCTCBLK->fDataPending = 1;

    release_lock( &pCTCBLK->Lock );

    obtain_lock( &pCTCBLK->EventLock );
    signal_condition( &pCTCBLK->Event );
    release_lock( &pCTCBLK->EventLock );

    return 0;       // (0==success)
}

//
// ParseArgs
//

static int  ParseArgs( DEVBLK* pDEVBLK, PCTCBLK pCTCBLK,
                       int argc, char** argx )
{
    int             saw_if = 0;       /* -if specified               */
    int             saw_conf = 0; /* Other configuration flags present */
    struct in_addr  addr;               // Work area for addresses
    int             iMTU;
    int             i;
    MAC             mac;                // Work area for MAC address
#if defined(OPTION_W32_CTCI)
    int             iKernBuff;
    int             iIOBuff;
#endif
    char          *argn[MAX_ARGS];
    char         **argv = argn;

    // Copy argv (as this routine needs to make local changes)
    for(i=0; i<argc && i<MAX_ARGS;i++)
        argn[i]=argx[i];

    // Housekeeping
    memset( &addr, 0, sizeof( struct in_addr ) );
    memset( &mac, 0, sizeof( MAC ) );

    // Set some initial defaults
    STRLCPY( pCTCBLK->szMTU,     "1500" );
    STRLCPY( pCTCBLK->szNetMask, "255.255.255.255" );
    STRLCPY( pCTCBLK->szTUNCharDevName, DEF_NETDEV );

#if defined( OPTION_W32_CTCI )
    pCTCBLK->iKernBuff = DEF_CAPTURE_BUFFSIZE;
    pCTCBLK->iIOBuff   = DEF_PACKET_BUFFSIZE;
#endif

    // Initialize getopt's counter. This is necessary in the case
    // that getopt was used previously for another device.

    OPTRESET();
    optind      = 0;
    // Check for correct number of arguments
    if( argc < 1 )
    {
        // "%1d:%04X CTC: incorrect number of parameters"
        WRMSG(HHC00915, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI");
        return -1;
    }
    // Compatibility with old format configuration files needs to be
    // maintained. Old format statements have the tun character device
    // name as the second argument on Linux.
    if (strncasecmp( argv[0], "/", 1 ) == 0)
    {
        pCTCBLK->fOldFormat = 1;
    }
    else
    {
        // Build new argv list.
        // getopt_long used to work on old format configuration statements
        // because LCS was the first argument passed to the device
        // initialization routine (and was interpreted by getopt*
        // as the program name and ignored). Now that argv[0] is a valid
        // argument, we need to shift the arguments and insert a dummy
        // argv[0];

        // Don't allow us to exceed the allocated storage (sanity check)
        if( argc > (MAX_ARGS-1) )
            argc = (MAX_ARGS-1);

        for( i = argc; i > 0; i-- )
            argv[i] = argv[i - 1];

        argc++;
        argv[0] = pDEVBLK->typname;
    }

    // Parse any optional arguments if not old format
    while( !pCTCBLK->fOldFormat )
    {
        int     c;

#if defined( OPTION_W32_CTCI )
  #define  CTCI_OPTSTRING  "n:k:i:m:t:s:d"
#else
  #define  CTCI_OPTSTRING  "n:x:t:s:d"
#endif

#if defined(HAVE_GETOPT_LONG)
        int     iOpt;

        static struct option options[] =
        {
            { "dev",     required_argument, NULL, 'n' },
#if !defined( OPTION_W32_CTCI )
            { "tundev",  required_argument, NULL, 'x' },
            { "if",      required_argument, NULL, 'x' },
#endif
#if defined( OPTION_W32_CTCI )
            { "kbuff",   required_argument, NULL, 'k' },
            { "ibuff",   required_argument, NULL, 'i' },
            { "mac",     required_argument, NULL, 'm' },
#endif
            { "mtu",     required_argument, NULL, 't' },
            { "netmask", required_argument, NULL, 's' },
            { "debug",   no_argument,       NULL, 'd' },
            { NULL,      0,                 NULL,  0  }
        };

        c = getopt_long( argc, argv, CTCI_OPTSTRING, options, &iOpt );
#else /* defined(HAVE_GETOPT_LONG) */
        c = getopt( argc, argv, CTCI_OPTSTRING );
#endif /* defined(HAVE_GETOPT_LONG) */

        if (c == -1)  // No more options found
            break;

        switch (c)
        {
        case 'n':     // Network Device (special character device)

#if defined( OPTION_W32_CTCI )

            // This could be the IP or MAC address of the
            // host ethernet adapter.
            if (!inet_aton( optarg, &addr ))
            {
                // Not an IP address, check for valid MAC
                if (ParseMAC( optarg, mac ) != 0)
                {
                    // "%1d:%04X CTC: option '%s' value '%s' invalid"
                    WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                        pDEVBLK->devnum, "CTCI", "adapter address", optarg );
                    return -1;
                }
            }

#endif // defined( OPTION_W32_CTCI )

            // This is the file name of the special TUN/TAP character device
            if (strlen( optarg ) > sizeof( pCTCBLK->szTUNCharDevName ) - 1)
            {
                // "%1d:%04X CTC: option '%s' value '%s' invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, "CTCI", "device name", optarg );
                return -1;
            }

            STRLCPY( pCTCBLK->szTUNCharDevName, optarg );
            break;

#if !defined( OPTION_W32_CTCI )

        case 'x':     // TUN network interface name

            if (strlen( optarg ) > sizeof( pCTCBLK->szTUNIfName )-1)
            {
                // HHC00916 "%1d:%04X %s: option '%s' value '%s' invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, "CTCI", "TUN device name", optarg );
                return -1;
            }

            STRLCPY( pCTCBLK->szTUNIfName, optarg );
            saw_if = 1;
            break;
#endif

#if defined( OPTION_W32_CTCI )

        case 'k':     // Kernel Buffer Size (Windows only)

            iKernBuff = atoi( optarg );

            if (iKernBuff * 1024 < MIN_CAPTURE_BUFFSIZE    ||
                iKernBuff * 1024 > MAX_CAPTURE_BUFFSIZE)
            {
                // "%1d:%04X CTC: option '%s' value '%s' invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, "CTCI", "kernel buffer size", optarg );
                return -1;
            }

            pCTCBLK->iKernBuff = iKernBuff * 1024;
            break;

        case 'i':     // I/O Buffer Size (Windows only)

            iIOBuff = atoi( optarg );

            if (iIOBuff * 1024 < MIN_PACKET_BUFFSIZE    ||
                iIOBuff * 1024 > MAX_PACKET_BUFFSIZE)
            {
                // "%1d:%04X %s: option '%s' value '%s' invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, "CTCI", "dll i/o buffer size", optarg );
                return -1;
            }

            pCTCBLK->iIOBuff = iIOBuff * 1024;
            break;

        case 'm':

            if (0
                || ParseMAC( optarg, mac ) != 0 // (invalid format)
                || !(mac[0] & 0x02)             // (locally assigned MAC bit not ON)
                ||  (mac[0] & 0x01)             // (broadcast bit is ON)
            )
            {
                // "%1d:%04X %s: Option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, "CTCI", "MAC address", optarg );
                return -1;
            }

            STRLCPY( pCTCBLK->szMACAddress, optarg );
            saw_conf = 1;
            break;

#endif // defined( OPTION_W32_CTCI )

        case 't':     // MTU of point-to-point link (ignored if Windows)

            iMTU = atoi( optarg );

            if (iMTU < 46 || iMTU > 65536)
            {
                // "%1d:%04X CTC: option '%s' value '%s' invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),\
                    pDEVBLK->devnum, "CTCI", "MTU size", optarg );
                return -1;
            }

            STRLCPY( pCTCBLK->szMTU, optarg );
            saw_conf = 1;
            break;

        case 's':     // Netmask of point-to-point link

            if (!inet_aton( optarg, &addr ))
            {
                // "%1d:%04X CTC: option '%s' value '%s' invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, "CTCI", "netmask", optarg );
                return -1;
            }

            STRLCPY( pCTCBLK->szNetMask, optarg );
            saw_conf = 1;
            break;

        case 'd':     // Diagnostics

            pCTCBLK->fDebug = TRUE;
            break;

        default:
            break;
        }
    }

    // Shift past any options
    argc -= optind;
    argv += optind;

    i = 0;

    if( !pCTCBLK->fOldFormat )
    {
        // New format.
        // For *nix, there can be either:-
        // a) Two parameters (a pair of IPv4 addresses). If the -x option
        //    has not been specified, CTCI will use a TUN interface whose
        //    name is allocated by the kernel (e.g. tun0), that is
        //    configured by CTCI. If the -x option has been specified,
        //    CTCI will use a pre-named TUN interface. The TUN interface
        //    may have been created before CTCI was started, or it may be
        //    created by CTCI, but in either case the TUN interface is
        //    configured by CTCI.
        // b) One parameter when the -x option has not been specified.
        //    The single parameter specifies the name of a pre-configured
        //    TUN inferface that CTCI will use.
        // c) Zero parameters when the -x option has been specified. The
        //    The -x option specified the name of a pre-configured TUN
        //    inferface that CTCI will use..
        // For Windows there can be:-
        // a) Two parameters (a pair of IPv4 addresses).
        if (argc == 2 ) /* Not pre-configured, but possibly pre-named */
        {
            // Guest IP Address
            if( inet_aton( *argv, &addr ) == 0 )
            {
                // "%1d:%04X CTC: option '%s' value '%s' invalid"
                WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                      "IP address", *argv );
                return -1;
            }
            STRLCPY( pCTCBLK->szGuestIPAddr, *argv );
            argc--; argv++;

            // Driver IP Address
            if( inet_aton( *argv, &addr ) == 0 )
            {
                // "%1d:%04X CTC: option '%s' value '%s' invalid"
                WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                      "IP address", *argv );
                return -1;
            }
            STRLCPY( pCTCBLK->szDriveIPAddr, *argv );
            argc--; argv++;

            pCTCBLK->fPreconfigured = FALSE;
        }
#if !defined( OPTION_W32_CTCI )
        else if (argc == 1 && !saw_if && !saw_conf) /* Pre-configured using name */
        {
            if( strlen( *argv ) > sizeof(pCTCBLK->szTUNIfName)-1 )
            {
                // HHC00916 "%1d:%04X %s: option '%s' value '%s' invalid"
                WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                      "TUN device name", optarg );
                return -1;
            }
            STRLCPY( pCTCBLK->szTUNIfName, *argv );
            argc--; argv++;

            pCTCBLK->fPreconfigured = TRUE;
        }
        else if (argc == 0 && saw_if && !saw_conf) /* Pre-configured using -x option */
        {
            pCTCBLK->fPreconfigured = TRUE;
        }
#endif /* !defined( OPTION_W32_CTCI ) */
        else
        {
            // "%1d:%04X CTC: incorrect number of parameters"
            WRMSG(HHC00915, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI");
            return -1;
        }

#if defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )
        if (TRUE == pCTCBLK->fPreconfigured)
        {
            /* Need  to append the interface number to the character */
            /* device name to open the requested interface.          */

            char * s = pCTCBLK->szTUNIfName + strlen(pCTCBLK->szTUNIfName);

            while(isdigit((unsigned char)s[- 1])) s--;
            STRLCAT( pCTCBLK->szTUNCharDevName, s );
        }
#endif
    }
    else // if( pCTCBLK->fOldFormat )
    {
#if !defined( OPTION_W32_CTCI )
        // All arguments are non-optional in linux old-format
        // Old format has 5 and only 5 arguments
        if( argc != 5 )
        {
            // "%1d:%04X CTC: incorrect number of parameters"
            WRMSG(HHC00915, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI");
            return -1;
        }

        // TUN/TAP Device
        if( **argv != '/' ||
            strlen( *argv ) > sizeof( pCTCBLK->szTUNCharDevName ) - 1 )
        {
            // "%1d:%04X CTC: option '%s' value '%s' invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                  "device name", *argv );
            return -1;
        }

        STRLCPY( pCTCBLK->szTUNCharDevName, *argv );

        argc--; argv++;

        // MTU Size
        iMTU = atoi( *argv );

        if( iMTU < 46 || iMTU > 65536 )
        {
            // "%1d:%04X CTC: option '%s' value '%s' invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                  "MTU size", *argv );
            return -1;
        }

        STRLCPY( pCTCBLK->szMTU, *argv );
        argc--; argv++;

        // Guest IP Address
        if( inet_aton( *argv, &addr ) == 0 )
        {
            // "%1d:%04X CTC: option '%s' value '%s' invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                  "IP address", *argv );
            return -1;
        }

        STRLCPY( pCTCBLK->szGuestIPAddr, *argv );

        argc--; argv++;

        // Driver IP Address
        if( inet_aton( *argv, &addr ) == 0 )
        {
            // "%1d:%04X CTC: option '%s' value '%s' invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                  "IP address", *argv );
            return -1;
        }

        STRLCPY( pCTCBLK->szDriveIPAddr, *argv );

        argc--; argv++;

        // Netmask
        if( inet_aton( *argv, &addr ) == 0 )
        {
            // "%1d:%04X CTC: option '%s' value '%s' invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                  "netmask", *argv );
            return -1;
        }

        STRLCPY( pCTCBLK->szNetMask, *argv );

        argc--; argv++;

        if( argc > 0 )
        {
            // "%1d:%04X CTC: incorrect number of parameters"
            WRMSG(HHC00915, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI");
            return -1;
        }

#else // defined( OPTION_W32_CTCI )

        // There are 2 non-optional arguments in the Windows old-format:
        //   Guest IP address and Gateway address.
        // There are also 2 additional optional arguments:
        //   Kernel buffer size and I/O buffer size.

        while (argc > 0)
        {
            switch (i)
            {
            // Non-optional arguments
            case 0:

                // Guest IP Address
                if( inet_aton( *argv, &addr ) == 0 )
                {
                    // "%1d:%04X CTC: option '%s' value '%s' invalid"
                    WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                          "IP address", *argv );
                    return -1;
                }

                STRLCPY( pCTCBLK->szGuestIPAddr, *argv );

                argc--; argv++;

                // Destination (Gateway) Address
                if( inet_aton( *argv, &addr ) == 0 )
                {
                    // Not an IP address, check for valid MAC
                    if( ParseMAC( *argv, mac ) != 0 )
                    {
                        // "%1d:%04X CTC: option '%s' value '%s' invalid"
                        WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTCI",
                              "MAC address", *argv );
                        return -1;
                    }
                }

                STRLCPY( pCTCBLK->szTUNCharDevName, *argv );

                // Kludge: This may look strange at first, but with
                // TunTap32, only the last 3 bytes of the "driver IP
                // address" is actually used. It's purpose is to
                // generate a unique MAC for the virtual interface.
                // Thus, having the same address for the adapter and
                // destination is not an issue. This used to be
                // generated from the guest IP address, I screwed up
                // TunTap32 V2. (JAP)
                // This also fixes the confusing error messages from
                // TunTap.c when a MAC is given for this argument.

                STRLCPY( pCTCBLK->szDriveIPAddr, pCTCBLK->szGuestIPAddr );
                argc--; argv++; i++;
                continue;

            // Optional arguments from here on:

            case 1: // Kernel Buffer Size

                iKernBuff = atoi( *argv );

                if (iKernBuff * 1024 < MIN_CAPTURE_BUFFSIZE ||
                    iKernBuff * 1024 > MAX_CAPTURE_BUFFSIZE)
                {
                    // "%1d:%04X CTC: option '%s' value '%s' invalid"
                    WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                        pDEVBLK->devnum, "CTCI", "kernel buffer size", *argv );
                    return -1;
                }

                pCTCBLK->iKernBuff = iKernBuff * 1024;
                argc--; argv++; i++;
                continue;

            case 2: // I/O Buffer Size

                iIOBuff = atoi( *argv );

                if (iIOBuff * 1024 < MIN_PACKET_BUFFSIZE ||
                    iIOBuff * 1024 > MAX_PACKET_BUFFSIZE)
                {
                    // "%1d:%04X CTC: option '%s' value '%s' invalid"
                    WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                        pDEVBLK->devnum, "CTCI", "dll i/o buffer size", *argv );
                    return -1;
                }

                pCTCBLK->iIOBuff = iIOBuff * 1024;
                argc--; argv++; i++;
                continue;

            default:

                // "%1d:%04X CTC: incorrect number of parameters"
                WRMSG( HHC00915, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, "CTCI" );
                return -1;
            }
        }
#endif // !defined( OPTION_W32_CTCI )
    }

    return 0;
}
#endif /* !defined(__SOLARIS__)   jbs */
