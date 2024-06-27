/* CTC_PTP.C    (C) Copyright Ian Shorter, 2011-2012                  */
/*              (C) and others 2013-2021                             */
/*              MPC Point-To-Point (PTP)                              */
/*                                                                    */
/*   Released under "The Q Public License Version 1"                  */
/*   (http://www.hercules-390.org/herclic.html) as modifications to   */
/*   Hercules.                                                        */

/* This module contains device handling functions for the             */
/* MPCPTP and/or MPCPTP6 emulated connection                          */
/*                                                                    */
/* Device module hdtptp                                               */
/*                                                                    */
/* hercules.cnf:                                                      */
/* 0E20-0E21 PTP <optional parameters>                                */
/* See README.NETWORKING for details.                                 */

#include "hstdinc.h"
#include "hercules.h"
#include "ctcadpt.h"
#include "tuntap.h"
#include "resolve.h"
#include "ctc_ptp.h"
#include "mpc.h"
#include "opcode.h"
#include "herc_getopt.h"    /* getopt dynamic linking kludge */

#if !defined( OPTION_W32_CTCI )
  #include <ifaddrs.h>
  #if defined( __sun__ )
    #include <sys/sockio.h>
  #endif
#endif

DISABLE_GCC_UNUSED_SET_WARNING;

#if defined(WIN32) && !defined(_MSVC_) && !defined(HDL_USE_LIBTOOL)
  SYSBLK *psysblk;
  #define sysblk (*psysblk)
#endif


/* ------------------------------------------------------------------ */
/* Various constants used in this module, of which the significance   */
/* isn't clear.                                                       */
/* See also the process identifiers later in the source.              */
/* ------------------------------------------------------------------ */
#define PTPHX0_SEQNUM    0x00050010  // !!! //
#define MPC_TH_UNKNOWN10 0x0FFC      // !!! //   |
#define XDATALEN1        0x0FFC      // !!! //   | Are these related?
#define LEN_OF_PAGE_ONE  4092        //     //   |


/* ------------------------------------------------------------------ */
/* Function Declarations                                              */
/* ------------------------------------------------------------------ */

static int      ptp_init( DEVBLK* pDEVBLK, int argc, char *argv[] );

static void     ptp_execute_ccw( DEVBLK* pDEVBLK,  BYTE  bCode,
                                 BYTE    bFlags,   BYTE  bChained,
                                 U32     uCount,   BYTE  bPrevCode,
                                 int     iCCWSeq,  BYTE* pIOBuf,
                                 BYTE*   pMore,    BYTE* pUnitStat,
                                 U32*    pResidual );

static int      ptp_close( DEVBLK* pDEVBLK );

static void     ptp_query( DEVBLK* pDEVBLK, char** ppszClass,
                           int     iBufLen, char*  pBuffer );

static void     ptp_write( DEVBLK* pDEVBLK,  U32  uCount,
                           int     iCCWSeq,  BYTE* pIOBuf,
                           BYTE*   pMore,    BYTE* pUnitStat,
                           U32*    pResidual );

static void     write_th( DEVBLK* pDEVBLK,  U32  uCount,
                          int     iCCWSeq,  BYTE* pIOBuf,
                          BYTE*   pMore,    BYTE* pUnitStat,
                          U32*    pResidual );

static int      write_rrh_8108( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH );

static void     ptp_halt_or_clear( DEVBLK* pDEVBLK );

static void     ptp_read( DEVBLK* pDEVBLK,  U32   uCount,
                          int     iCCWSeq,  BYTE* pIOBuf,
                          BYTE*   pMore,    BYTE* pUnitStat,
                          U32*    pResidual );

static void     read_read_buffer( DEVBLK* pDEVBLK,   U32  uCount,
                                  int     iCCWSeq,   BYTE* pIOBuf,
                                  BYTE*   pMore,     BYTE* pUnitStat,
                                  U32*    pResidual, PTPHDR* pPTPHDR );

static void     read_chain_buffer( DEVBLK* pDEVBLK,   U32  uCount,
                                   int     iCCWSeq,   BYTE* pIOBuf,
                                   BYTE*   pMore,     BYTE* pUnitStat,
                                   U32*    pResidual, PTPHDR* pPTPHDR );

static void*    ptp_read_thread( void* arg /* PTPBLK* pPTPBLK */ );

static void*    add_buffer_to_chain_and_signal_event( PTPATH* pPTPATH, PTPHDR* pPTPHDR );
static void*    add_buffer_to_chain( PTPATH* pPTPATH, PTPHDR* pPTPHDR );
static PTPHDR*  remove_buffer_from_chain( PTPATH* pPTPATH );
static void*    remove_and_free_any_buffers_on_chain( PTPATH* pPTPATH );

static PTPHDR*  alloc_ptp_buffer( DEVBLK* pDEVBLK, int iSize );
static void*    alloc_storage( DEVBLK* pDEVBLK, int iSize );


static int      parse_conf_stmt( DEVBLK* pDEVBLK, PTPBLK* pPTPBLK,
                                 int argc, char** argv );
static int      get_preconfigured_value( DEVBLK* pDEVBLK, PTPBLK* pPTPBLK );
static int      check_specified_value( DEVBLK* pDEVBLK, PTPBLK* pPTPBLK );

static int      raise_unsol_int( DEVBLK* pDEVBLK, BYTE bStatus, int iDelay );

static void*    ptp_unsol_int_thread( void* arg /* PTPINT* pPTPINT */ );

static void     ptp_get_tod_clock( BYTE* TodClock );

static void     get_subarea_address( BYTE* SAaddress );

static void     write_hx0_01( DEVBLK* pDEVBLK,  U32  uCount,
                              int     iCCWSeq,  BYTE* pIOBuf,
                              BYTE*   pMore,    BYTE* pUnitStat,
                              U32*    pResidual );

static void     write_hx0_00( DEVBLK* pDEVBLK,  U32  uCount,
                              int     iCCWSeq,  BYTE* pIOBuf,
                              BYTE*   pMore,    BYTE* pUnitStat,
                              U32*    pResidual );

static void     write_hx2( DEVBLK* pDEVBLK,  U32  uCount,
                           int     iCCWSeq,  BYTE* pIOBuf,
                           BYTE*   pMore,    BYTE* pUnitStat,
                           U32*    pResidual );

static PTPHSV*  point_CSVcv( DEVBLK* pDEVBLK, PTPHX2* pPTPHX2 );

static int      write_rrh_417E( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH );

static PTPHDR*  build_417E_cm_enable( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH,
                                      MPC_PUS* pMPC_PUS, u_int* fxSideWins );
static PTPHDR*  build_417E_cm_setup( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH );
static PTPHDR*  build_417E_cm_confirm( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH );
static PTPHDR*  build_417E_ulp_enable( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH,
                                       MPC_PUS* pMPC_PUS, u_int* fxSideWins );
static PTPHDR*  build_417E_ulp_setup( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH );
static PTPHDR*  build_417E_ulp_confirm( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH );
static PTPHDR*  build_417E_dm_act( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH );

static int      write_rrh_C17E( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH );

static int      write_rrh_C108( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH );

static PTPHDR*  build_C108_will_you_start_4( DEVBLK* pDEVBLK );
static PTPHDR*  build_C108_will_you_start_6( DEVBLK* pDEVBLK );
static PTPHDR*  build_C108_i_will_start_4( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIX, U16 uRCode );
static PTPHDR*  build_C108_i_will_start_6( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIX, U16 uRCode );
static PTPHDR*  build_C108_my_address_4( DEVBLK* pDEVBLK );
static PTPHDR*  build_C108_my_address_6( DEVBLK* pDEVBLK, u_int fLL );
static PTPHDR*  build_C108_your_address_4( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIX, U16 uRCode );
static PTPHDR*  build_C108_your_address_6( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIX, U16 uRCode );
static PTPHDR*  build_C108_will_you_stop_4( DEVBLK* pDEVBLK );
static PTPHDR*  build_C108_will_you_stop_6( DEVBLK* pDEVBLK );
static PTPHDR*  build_C108_i_will_stop_4( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIX );
static PTPHDR*  build_C108_i_will_stop_6( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIX );

#if defined(ENABLE_IPV6)
static void     build_8108_icmpv6_packets( DEVBLK* pDEVBLK );
#endif /* defined(ENABLE_IPV6) */

static void     gen_csv_sid( BYTE* pClock1, BYTE* pClock2, BYTE* pToken );

static void     shift_left_dbl( U32* even, U32* odd, int number );
static void     shift_right_dbl( U32* even, U32* odd, int number );

#if defined(ENABLE_IPV6)
static void     calculate_icmpv6_checksum( PIP6FRM pIP6FRM, BYTE* pIcmpHdr, int iIcmpLen );
#endif /* defined(ENABLE_IPV6) */


/* ------------------------------------------------------------------ */
/* Ivan Warren 20040227                                               */
/* This table is used by channel.c to determine if a CCW code is an   */
/* immediate command or not                                           */
/* The table is addressed in the DEVHND structure as 'DEVIMM immed'   */
/* 0 : Command is NOT an immediate command                            */
/* 1 : Command is an immediate command                                */
/* Note : An immediate command is defined as a command which returns  */
/* CE (channel end) during initialisation (that is, no data is        */
/* actually transfered). In this case, IL is not indicated for a CCW  */
/* Format 0 or for a CCW Format 1 when IL Suppression Mode is in      */
/* effect                                                             */
/* ------------------------------------------------------------------ */

static BYTE ptp_immed_commands[256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0, /* 1x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 2x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 3x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 4x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 5x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 6x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 7x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 8x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 9x */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* Ax */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* Bx */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* Cx */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* Dx */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* Ex */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* Fx */
};

//  0x03  No Operation
//  0x17  Control
//  0xC3  Set Extended Mode
//  0xE3  Prepare


/* ------------------------------------------------------------------ */
/* Device Handler Information Block                                   */
/* ------------------------------------------------------------------ */

DEVHND ptp_device_hndinfo =
{
        &ptp_init,                     /* Device Initialization       */
        &ptp_execute_ccw,              /* Device CCW execute          */
        &ptp_close,                    /* Device Close                */
        &ptp_query,                    /* Device Query                */
        NULL,                          /* Device Extended Query       */
        NULL,                          /* Device Start channel pgm    */
        NULL,                          /* Device End channel pgm      */
        NULL,                          /* Device Resume channel pgm   */
        NULL,                          /* Device Suspend channel pgm  */
        &ptp_halt_or_clear,            /* Device Halt channel pgm     */
        NULL,                          /* Device Read                 */
        NULL,                          /* Device Write                */
        NULL,                          /* Device Query used           */
        NULL,                          /* Device Reserve              */
        NULL,                          /* Device Release              */
        NULL,                          /* Device Attention            */
        ptp_immed_commands,            /* Immediate CCW Codes         */
        NULL,                          /* Signal Adapter Input        */
        NULL,                          /* Signal Adapter Output       */
        NULL,                          /* Signal Adapter Sync         */
        NULL,                          /* Signal Adapter Output Mult  */
        NULL,                          /* QDIO subsys desc            */
        NULL,                          /* QDIO set subchan ind        */
        NULL,                          /* Hercules suspend            */
        NULL                           /* Hercules resume             */
};


/* ------------------------------------------------------------------ */
/* Constants                                                          */
/* ------------------------------------------------------------------ */
static const BYTE VTAM_ebcdic[4] = { 0xE5,0xE3,0xC1,0xD4 };


/* ------------------------------------------------------------------ */
/* Process Identifers                                                 */
/* ------------------------------------------------------------------ */
// When the connection is being started it seems that the connecting
// VTAMs set up a pair of processes (or channels or threads or
// subtasks or paths or ...). I don't know what they should be called,
// but I have called them processes, hence process identifiers.
//
// All of the identifers that have been seen are 5-bytes in length,
// with the first byte containing 0x05, the second and third bytes
// containing 0x0001, and the fourth and fifth bytes containing a
// non-consecutive value that increases with each restart of the
// connection. Is the first byte (0x05) a length field, or something
// else entirely? Are the second and third bytes (0x0001) part of
// the identifier, or something else entirely?
//
// It has been assumed that the first byte is a length field, and
// the second to fifth bytes are a 4-byte (32-bit) identifier.
//
// Whenever a connection is being started the identifiers are copied
// from these static fields, and then the static fields are
// incremented by a fixed value. This ensures that each connection
// uses a unique set of values. Empirical evidence suggests that if
// the connection is restarted by the y-side guest and the x-side
// does not change its values the restart is not successful, perhaps
// because the y-side VTAM has a memory of the values previously
// used. Or there may be another cause yet to be discovered. Anyway,
// a VTAM to VTAM connection uses different values, so we will to.
// Additionally, if there are multiple connections to the same
// guest, the possibility of confusion for the guest is reduced if
// each connection uses different values.

static  LOCK  TokenLock;
static  int   TokenLockInitialized = FALSE;
static  U32   uTokenIssuerRm       = 0x00011001;
static  U32   uTokenCmFilter       = 0x00011002;
static  U32   uTokenCmConnection   = 0x00011003;
static  U32   uTokenUlpFilter      = 0x00011004;
static  U32   uTokenUlpConnection  = 0x00011005;

#define       INCREMENT_TOKEN        0x00000010


/* ================================================================== */
/* ptp_init()                                                         */
/* ================================================================== */

// ptp_init is called once for each of the device addresses specified
// on the configuration statement. When ptp_init is called the number
// of device addresses specified on the configuation statement or
// whether the device addresses are contiguous is unknown to ptp_init.
// ptp_init is called by function attach_device() in config.c

int  ptp_init( DEVBLK* pDEVBLK, int argc, char *argv[] )
{
    PTPBLK*    pPTPBLK;                // PTPBLK
    PTPATH*    pPTPATHre;              // PTPATH Read
    PTPATH*    pPTPATHwr;              // PTPATH Write
//  int        nIFType;                // Interface type
    int        nIFFlags;               // Interface flags
    int        rc = 0;                 // Return code
    int        i;
    char       thread_name[32];        // ptp_read_thread


//  nIFType =               // Interface type
//      0
//      | IFF_TUN           // ("TUN", not "tap")
//      | IFF_NO_PI         // (no packet info)
//      ;

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

    // Initialize fields in the DEVBLK that are referenced by commands.
    pDEVBLK->devtype = 0x3088;
    pDEVBLK->excps = 0;

    // Initialize locking for the tokens, if necessary.
    if (!TokenLockInitialized)
    {
        TokenLockInitialized = TRUE;
        initialize_lock( &TokenLock );
    }

    // PTP is a group device, with two devices in the group. The first
    // device is deemed to be the read device, the second device is deemed
    // to be the write device. (Function group_device() is in config.c)
    if (!group_device( pDEVBLK, PTP_GROUP_SIZE ))
        return 0;

    // Allocate the PTPBLK.
    pPTPBLK = alloc_storage( pDEVBLK, (int)sizeof(PTPBLK) );
    if (!pPTPBLK)
        return -1;

    // Allocate the PTPATH Read.
    pPTPATHre = alloc_storage( pDEVBLK, (int)sizeof(PTPATH) );
    if (!pPTPATHre)
    {
        free( pPTPBLK );
        return -1;
    }

    // Allocate the PTPATH Write.
    pPTPATHwr = alloc_storage( pDEVBLK, (int)sizeof(PTPATH) );
    if (!pPTPATHwr)
    {
        free( pPTPATHre );
        free( pPTPBLK );
        return -1;
    }

    // Parse configuration file statement.
    if (parse_conf_stmt( pDEVBLK, pPTPBLK, argc, (char**) argv ) != 0)
    {
        free( pPTPATHwr );
        free( pPTPATHre );
        free( pPTPBLK );
        return -1;
    }

    // Connect the DEVBLKs, the PTPATHs and the PTPBLK together.
    pPTPBLK->pPTPATHRead = pPTPATHre;                   // Make the PTPBLK point
    pPTPBLK->pPTPATHWrite = pPTPATHwr;                  // to the two PTPATHs.

    pPTPBLK->pDEVBLKRead = pDEVBLK->group->memdev[0];   // Make the PTPBLK point
    pPTPBLK->pDEVBLKWrite = pDEVBLK->group->memdev[1];  // to the two DEVBLKs.

    pPTPATHre->pPTPBLK = pPTPBLK;                       // Make each PTPATH point
    pPTPATHwr->pPTPBLK = pPTPBLK;                       // to the PTPBLK.

    pPTPATHre->pDEVBLK = pPTPBLK->pDEVBLKRead;          // Make each PTPATH point
    pPTPATHwr->pDEVBLK = pPTPBLK->pDEVBLKWrite;         // to the apprpriate DEVBLK

    pPTPBLK->pDEVBLKRead->dev_data = pPTPATHre;         // Make each DEVBLK point
    pPTPBLK->pDEVBLKWrite->dev_data = pPTPATHwr;        // to the appropriate PTPATH.

    pDEVBLK->group->grp_data = pPTPBLK;                 // Make DEVGRP point to PTPBLK

    // Initialize various fields in the DEVBLKs.
    SetSIDInfo( pPTPBLK->pDEVBLKRead, 0x3088, 0x08, 0x3088, 0x01 );
    SetSIDInfo( pPTPBLK->pDEVBLKWrite, 0x3088, 0x08, 0x3088, 0x01 );

    pPTPBLK->pDEVBLKRead->ctctype  = CTC_PTP;
    pPTPBLK->pDEVBLKRead->ctcxmode = 1;

    pPTPBLK->pDEVBLKWrite->ctctype  = CTC_PTP;
    pPTPBLK->pDEVBLKWrite->ctcxmode = 1;

    strlcpy( pPTPBLK->pDEVBLKRead->filename,
             pPTPBLK->szTUNCharDevName,
             sizeof(pPTPBLK->pDEVBLKRead->filename) );
    strlcpy( pPTPBLK->pDEVBLKWrite->filename,
             pPTPBLK->szTUNCharDevName,
             sizeof(pPTPBLK->pDEVBLKWrite->filename) );

    // Initialize various fields in the PTPATHs.
    pPTPATHre->bDLCtype = DLCTYPE_READ;      // Read path
    pPTPATHwr->bDLCtype = DLCTYPE_WRITE;    // write path

    // Initialize various fields in the PTPBLK.
    pPTPBLK->fd = -1;

    pPTPBLK->xDataLen1 = XDATALEN1;                        // !!! //

    pPTPBLK->xMaxReadLen = ( pPTPBLK->iMaxBfru * 4096 ) - 4;
    //       xMaxReadLen = 20476 (0x4FFC) when iMaxBfru = 5
    //       xMaxReadLen = 65532 (0xFFFC) when iMaxBfru = 16

    pPTPBLK->xActMTU = ( ( pPTPBLK->iMaxBfru - 1 ) * 4096 ) - 2048;
    //       xActMTU = 14336 (0x3800) when iMaxBfru = 5
    //       xActMTU = 59392 (0xE800) when iMaxBfru = 16

    ptp_get_tod_clock( pPTPBLK->xStartTime );    // x-side's start time

    for( i = 0; i <= 7; i++ )
        pPTPBLK->xFirstCsvSID2[i] = pPTPBLK->xStartTime[i] ^ 0xAA;

    get_subarea_address( pPTPBLK->xSAaddress );  // x-side's subarea address

    // Initialize locking and event mechanisms in the PTPBLK and the PTPATHs.
    initialize_lock( &pPTPBLK->ReadBufferLock );
    initialize_lock( &pPTPBLK->ReadEventLock );
    initialize_condition( &pPTPBLK->ReadEvent );
    initialize_lock( &pPTPBLK->UnsolListLock );
    initialize_lock( &pPTPBLK->UpdateLock );

    initialize_lock( &pPTPATHre->ChainLock );
    initialize_lock( &pPTPATHre->UnsolEventLock );
    initialize_condition( &pPTPATHre->UnsolEvent );

    initialize_lock( &pPTPATHwr->ChainLock );
    initialize_lock( &pPTPATHwr->UnsolEventLock );
    initialize_condition( &pPTPATHwr->UnsolEvent );

    // Create the TUN interface.
    rc = TUNTAP_CreateInterface( pPTPBLK->szTUNCharDevName,
#if defined(BUILD_HERCIFC)
                                 (pPTPBLK->fPreconfigured ? IFF_NO_HERCIFC : 0) |
#endif //defined(BUILD_HERCIFC)
                                 IFF_TUN | IFF_NO_PI,
                                 &pPTPBLK->fd,
                                 pPTPBLK->szTUNIfName );
    if (rc < 0)
    {
        // Disconnect the DEVGRP from the PTPBLK.
        pDEVBLK->group->grp_data = NULL;
        // Disconnect the DEVBLKs from the PTPATHs.
        pPTPBLK->pDEVBLKRead->dev_data = NULL;
        pPTPBLK->pDEVBLKWrite->dev_data = NULL;
        // Free the PTPATHs and PTPBLK
        free( pPTPATHwr );
        free( pPTPATHre );
        free( pPTPBLK );
        return -1;
    }
    // HHC00901 "%1d:%04X %s: interface %s, type %s opened"
    WRMSG(HHC00901, "I", SSID_TO_LCSS(pPTPBLK->pDEVBLKRead->ssid), pPTPBLK->pDEVBLKRead->devnum,
                         pPTPBLK->pDEVBLKRead->typname, pPTPBLK->szTUNIfName, "TUN" );

    // Copy the fd to make panel.c happy
    pPTPBLK->pDEVBLKRead->fd =
    pPTPBLK->pDEVBLKWrite->fd = pPTPBLK->fd;

    /* */
    if (!pPTPBLK->fPreconfigured) {

        // Set various values for the TUN interface.
#if defined(OPTION_W32_CTCI)
        {
            struct tt32ctl tt32ctl;

            memset( &tt32ctl, 0, sizeof(tt32ctl) );
            STRLCPY( tt32ctl.tt32ctl_name, pPTPBLK->szTUNIfName );

            // Set the specified driver/dll i/o buffer sizes..
            tt32ctl.tt32ctl_devbuffsize = pPTPBLK->iKernBuff;
            if (TUNTAP_IOCtl( pPTPBLK->fd, TT32SDEVBUFF, (char*)&tt32ctl ) != 0)
            {
                // HHC00902 "%1d:%04X %s: ioctl '%s' failed for device '%s': '%s'"
                WRMSG(HHC00902, "W", SSID_TO_LCSS(pPTPBLK->pDEVBLKRead->ssid),
                                pPTPBLK->pDEVBLKRead->devnum, pPTPBLK->pDEVBLKRead->typname,
                                "TT32SDEVBUFF", pPTPBLK->szTUNIfName, strerror( errno ) );
            }

            tt32ctl.tt32ctl_iobuffsize = pPTPBLK->iIOBuff;
            if (TUNTAP_IOCtl( pPTPBLK->fd, TT32SIOBUFF, (char*)&tt32ctl ) != 0)
            {
                // HHC00902 "%1d:%04X %s: ioctl '%s' failed for device '%s': '%s'"
                WRMSG(HHC00902, "W", SSID_TO_LCSS(pPTPBLK->pDEVBLKRead->ssid),
                                pPTPBLK->pDEVBLKRead->devnum, pPTPBLK->pDEVBLKRead->typname,
                                "TT32SIOBUFF", pPTPBLK->szTUNIfName, strerror( errno ) );
            }
        }

#ifdef OPTION_TUNTAP_SETMACADDR
        VERIFY( TUNTAP_SetMACAddr( pPTPBLK->szTUNIfName, pPTPBLK->szMACAddress  ) == 0 );
#endif /* OPTION_TUNTAP_SETMACADDR */

#ifdef OPTION_TUNTAP_CLRIPADDR
        VERIFY( TUNTAP_ClrIPAddr ( pPTPBLK->szTUNIfName ) == 0 );
#endif /* OPTION_TUNTAP_CLRIPADDR */
#endif /* defined(OPTION_W32_CTCI) */

        if (pPTPBLK->fIPv4Spec)
        {
            VERIFY( TUNTAP_SetIPAddr( pPTPBLK->szTUNIfName, pPTPBLK->szDriveIPAddr4 ) == 0 );

            VERIFY( TUNTAP_SetDestAddr( pPTPBLK->szTUNIfName, pPTPBLK->szGuestIPAddr4 ) == 0 );

#ifdef OPTION_TUNTAP_SETNETMASK
            VERIFY( TUNTAP_SetNetMask( pPTPBLK->szTUNIfName, pPTPBLK->szNetMask ) == 0 );
#endif /* OPTION_TUNTAP_SETNETMASK */
        }

#if defined(ENABLE_IPV6)
        if (pPTPBLK->fIPv6Spec)
        {
            VERIFY( TUNTAP_SetIPAddr6( pPTPBLK->szTUNIfName,
                                       pPTPBLK->szDriveLLAddr6,
                                       pPTPBLK->szDriveLLxSiz6 ) == 0 );

            VERIFY( TUNTAP_SetIPAddr6( pPTPBLK->szTUNIfName,
                                       pPTPBLK->szDriveIPAddr6,
                                       pPTPBLK->szDrivePfxSiz6 ) == 0 );
        }
#endif /* defined(ENABLE_IPV6) */

        VERIFY( TUNTAP_SetMTU( pPTPBLK->szTUNIfName, pPTPBLK->szMTU ) == 0 );

        VERIFY( TUNTAP_SetFlags( pPTPBLK->szTUNIfName, nIFFlags ) == 0 );

    }

    // Create the read thread.
    MSGBUF( thread_name, "%s %4.4X ReadThread",
                         pPTPBLK->pDEVBLKRead->typname,
                         pPTPBLK->pDEVBLKRead->devnum);
    rc = create_thread( &pPTPBLK->tid, JOINABLE, ptp_read_thread, pPTPBLK, thread_name );
    if (rc)
    {
        // Report the bad news.
        // HHC00102 "Error in function create_thread(): %s"
        WRMSG(HHC00102, "E", strerror(rc));
        // Close the TUN interface.
        VERIFY( pPTPBLK->fd == -1 || TUNTAP_Close( pPTPBLK->fd ) == 0 );
        pPTPBLK->fd = -1;
        // Disconnect the DEVGRP from the PTPBLK.
        pDEVBLK->group->grp_data = NULL;
        // Disconnect the DEVBLKs from the PTPATHs.
        pPTPBLK->pDEVBLKRead->dev_data = NULL;
        pPTPBLK->pDEVBLKWrite->dev_data = NULL;
        // Free the PTPATHs and PTPBLK
        free( pPTPATHwr );
        free( pPTPATHre );
        free( pPTPBLK );
        return -1;
    }
    pPTPBLK->pDEVBLKRead->tid = pPTPBLK->tid;
    pPTPBLK->pDEVBLKWrite->tid = pPTPBLK->tid;

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPCONFVALUE)
    {
#if defined(OPTION_W32_CTCI)
        // HHC03952 "%1d:%04X PTP: MAC: %s"
        WRMSG(HHC03952, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
            pPTPBLK->szMACAddress );
#endif /* defined(OPTION_W32_CTCI) */
#if defined(ENABLE_IPV6)
        if (pPTPBLK->fIPv4Spec)
        {
#endif /* defined(ENABLE_IPV6) */
            // HHC03953 "%1d:%04X PTP: IPv4: Drive %s/%s (%s): Guest %s"
            WRMSG(HHC03953, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szDriveIPAddr4,
                pPTPBLK->szDrivePfxSiz4,
                pPTPBLK->szNetMask,
                pPTPBLK->szGuestIPAddr4 );
#if defined(ENABLE_IPV6)
        }
        if (pPTPBLK->fIPv6Spec)
        {
            // HHC03954 "%1d:%04X PTP: IPv6: Drive %s/%s %s/%s: Guest %s"
            WRMSG(HHC03954, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szDriveLLAddr6,
                pPTPBLK->szDriveLLxSiz6,
                pPTPBLK->szDriveIPAddr6,
                pPTPBLK->szDrivePfxSiz6,
                pPTPBLK->szGuestIPAddr6 );
        }
#endif /* defined(ENABLE_IPV6) */
    }

    return 0;
}   /* End function  ptp_init() */


/* ================================================================== */
/* ptp_execute_ccw()                                                  */
/* ================================================================== */
// bCode, bFlags and uCount are from the executing CCW.
// bChained, bPrevCode and iCCWSeq are only meaningful for the second
// or subsequent CCWs of a chain. bChained contains 0x40, pPrevCode
// contains the opcode of the previous CCW in the chain and iCCWSeq
// contains the sequence number of the CCW in the chain (0 = first,
// 1 = second, 2 = third, etc).

void  ptp_execute_ccw( DEVBLK* pDEVBLK, BYTE  bCode,
                       BYTE    bFlags,  BYTE  bChained,
                       U32     uCount,  BYTE  bPrevCode,
                       int     iCCWSeq, BYTE* pIOBuf,
                       BYTE*   pMore,   BYTE* pUnitStat,
                       U32*    pResidual )
{

    PTPATH*   pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*   pPTPBLK  = pPTPATH->pPTPBLK;
    int       iNum;                          // Number of bytes to move

    UNREFERENCED( bFlags    );
    UNREFERENCED( bChained  );
    UNREFERENCED( bPrevCode );


    // Intervention required if the device file is not open
    if (pDEVBLK->fd < 0)
    {
        pDEVBLK->sense[0] = SENSE_IR;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPCCW)
    {
        // HHC03992 "%1d:%04X %s: Code %02X: Flags %02X: Count %08X: Chained %02X: PrevCode %02X: CCWseq %d"
        WRMSG(HHC03992, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
            bCode, bFlags, uCount, bChained, bPrevCode, iCCWSeq );
    }

    // Process depending on CCW bCode
    switch (bCode)
    {
    case 0x01:  // 0MMMMM01  WRITE
        //------------------------------------------------------------
        // WRITE
        //------------------------------------------------------------

        // Return normal status if CCW count is zero
        if (uCount == 0)
        {
            *pUnitStat = CSW_CE | CSW_DE;
            break;
        }

        // Process the Write data
        ptp_write( pDEVBLK, uCount, iCCWSeq, pIOBuf, pMore, pUnitStat, pResidual );

        break;

    case 0x02:  // MMMMMM10  READ
        /* ---------------------------------------------------------- */
        /* READ                                                       */
        /* ---------------------------------------------------------- */

        // Process the Read depending on the current State
        ptp_read( pDEVBLK, uCount, iCCWSeq, pIOBuf, pMore, pUnitStat, pResidual );

        break;

    case 0xE3:  // 11100011  PREPARE
        /* ---------------------------------------------------------- */
        /* PREPARE                                                    */
        /* ---------------------------------------------------------- */

        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0x17:  // MMMMM111  CONTROL
        /* ---------------------------------------------------------- */
        /* CONTROL                                                    */
        /* ---------------------------------------------------------- */

        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0x14:  // XXX10100  SENSE COMMAND BYTE
        /* ---------------------------------------------------------- */
        /* SENSE COMMAND BYTE                                         */
        /* ---------------------------------------------------------- */

        // We will assume that we (i.e. the x-side) raised an Attention
        // interrupt earlier and that the y-side is determining why.
        // Normally this will only occur during the handshke sequence.
        // Return CCW opcode, residual byte count and unit status.
        *pIOBuf = pPTPATH->bAttnCode;
        *pResidual = uCount - 1;
        *pUnitStat = CSW_CE | CSW_DE;

        // Clear the CCW opcode.
        pPTPATH->bAttnCode = 0x00;

        break;

    case 0x04:  // 00000100  SENSE ADAPTOR STATE
        /* ---------------------------------------------------------- */
        /* SENSE ADAPTER STATE                                        */
        /* ---------------------------------------------------------- */

        // Calculate residual byte count
        iNum = ( uCount < pDEVBLK->numsense ) ? uCount : pDEVBLK->numsense;

        *pResidual = uCount - iNum;

        if (uCount < pDEVBLK->numsense)
            *pMore = 1;

        // Copy device sense bytes to channel I/O buffer
        memcpy( pIOBuf, pDEVBLK->sense, iNum );

        // Clear the device sense bytes
        memset( pDEVBLK->sense, 0, sizeof(pDEVBLK->sense) );

        // Return unit status
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0xE4:  //  11100100  SENSE ID
        /* ---------------------------------------------------------- */
        /* SENSE ID                                                   */
        /* ---------------------------------------------------------- */

        // Calculate residual byte count
        iNum = ( uCount < pDEVBLK->numdevid ) ? uCount : pDEVBLK->numdevid;

        *pResidual = uCount - iNum;

        if (uCount < pDEVBLK->numdevid)
            *pMore = 1;

        // Copy device identifier bytes to channel I/O buffer
        memcpy( pIOBuf, pDEVBLK->devid, iNum );

        // Return unit status
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0x03:  // M0MMM011  NO OPERATION
    case 0xC3:  // 11000011  SET EXTENDED MODE
        /* ---------------------------------------------------------- */
        /* NO OPERATON & SET EXTENDED MODE                            */
        /* ---------------------------------------------------------- */

        // Return unit status
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    default:
        /* ---------------------------------------------------------- */
        /* INVALID OPERATION                                          */
        /* ---------------------------------------------------------- */

        // Set command reject sense byte, and unit check status
        pDEVBLK->sense[0] = SENSE_CR;
        *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;

        break;

    }

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPCCW)
    {
        // HHC03993 "%1d:%04X %s: Status %02X: Residual %08X: More %02X"
        WRMSG(HHC03993, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
            *pUnitStat, *pResidual, *pMore );
    }

    return;
}   /* End function  ptp_execute_ccw() */


/* ================================================================== */
/* ptp_close()                                                        */
/* ================================================================== */

int  ptp_close( DEVBLK* pDEVBLK )
{
    PTPATH*   pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*   pPTPBLK  = pPTPATH->pPTPBLK;


    // Close the device file (if not already closed)
    if (pPTPBLK->fd >= 0)
    {
        // PROGRAMMING NOTE: there's currently no way to interrupt
        // the "ptp_read_thread"s TUNTAP_Read of the adapter. Thus
        // we must simply wait for ptp_read_thread to eventually
        // notice that we're doing a close (via our setting of the
        // fCloseInProgress flag). Its TUNTAP_Read will eventually
        // timeout after a few seconds (currently 5, which is dif-
        // ferent than the PTP_READ_TIMEOUT_SECS timeout value the
        // ptp_read function uses) and will then do the close of
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

        TID tid = pPTPBLK->tid;
        pPTPBLK->fCloseInProgress = 1;  // (ask read thread to exit)
        join_thread( tid, NULL );       // (wait for thread to end)
#if defined( OPTION_FTHREADS )
        detach_thread( tid );           // (wait for thread to end)
#endif
    }

    pDEVBLK->fd = -1;           // indicate we're now closed

    return 0;
}   /* End function  ptp_close() */


/* ================================================================== */
/* ptp_query()                                                        */
/* ================================================================== */

// Note: this function is called four times every second!

void  ptp_query( DEVBLK* pDEVBLK, char** ppszClass,
                 int     iBufLen, char*  pBuffer )
{
    char filename[ PATH_MAX + 1 ];      /* full path or just name    */

    PTPATH*   pPTPATH;
    PTPBLK*   pPTPBLK;
    char*     pGuestIP4;
    char*     pDriveIP4;
#if defined(ENABLE_IPV6)
    char*     pGuestIP6;
    char*     pDriveIP6;
#endif

    BEGIN_DEVICE_CLASS_QUERY( "CTCA", pDEVBLK, ppszClass, iBufLen, pBuffer );

    pPTPATH = pDEVBLK->dev_data;

    if (!pPTPATH)
    {
        strlcpy(pBuffer,"*Uninitialized",iBufLen);
        return;
    }

    pPTPBLK = pPTPATH->pPTPBLK;

    if (strlen( pPTPBLK->szGuestIPAddr4 ))
       pGuestIP4 = pPTPBLK->szGuestIPAddr4;
    else
       pGuestIP4 = "-";

    if (strlen( pPTPBLK->szDriveIPAddr4 ))
       pDriveIP4 = pPTPBLK->szDriveIPAddr4;
    else
       pDriveIP4 = "-";

#if defined(ENABLE_IPV6)
    if (strlen( pPTPBLK->szGuestIPAddr6 ))
       pGuestIP6 = pPTPBLK->szGuestIPAddr6;
    else
       pGuestIP6 = "-";

    if (strlen( pPTPBLK->szDriveIPAddr6 ))
       pDriveIP6 = pPTPBLK->szDriveIPAddr6;
    else
       pDriveIP6 = "-";

    if (pPTPBLK->fIPv4Spec && pPTPBLK->fIPv6Spec)
    {
        snprintf( pBuffer, iBufLen, "%s %s/%s %s/%s (%s)%s IO[%"PRIu64"]",
                  pPTPBLK->pDEVBLKRead->typname,
                  pGuestIP4,
                  pDriveIP4,
                  pGuestIP6,
                  pDriveIP6,
                  pPTPBLK->szTUNIfName,
                  pPTPBLK->uDebugMask ? " -d" : "",
                  pDEVBLK->excps );
    }
    else if (pPTPBLK->fIPv4Spec)
    {
#endif /* defined(ENABLE_IPV6) */
        snprintf( pBuffer, iBufLen, "%s %s/%s (%s)%s IO[%"PRIu64"]",
                  pPTPBLK->pDEVBLKRead->typname,
                  pGuestIP4,
                  pDriveIP4,
                  pPTPBLK->szTUNIfName,
                  pPTPBLK->uDebugMask ? " -d" : "",
                  pDEVBLK->excps );
#if defined(ENABLE_IPV6)
    }
    else
    {
        snprintf( pBuffer, iBufLen, "%s %s/%s (%s)%s IO[%"PRIu64"]",
                  pPTPBLK->pDEVBLKRead->typname,
                  pGuestIP6,
                  pDriveIP6,
                  pPTPBLK->szTUNIfName,
                  pPTPBLK->uDebugMask ? " -d" : "",
                  pDEVBLK->excps );
    }
#endif /* defined(ENABLE_IPV6) */
    pBuffer[iBufLen-1] = '\0';

    return;
}   /* End function  ptp_query() */


/* ------------------------------------------------------------------ */
/* ptp_write()                                                        */
/* ------------------------------------------------------------------ */
// Input:
//      pDEVBLK   A pointer to the CTC adapter device block
//      uCount    The I/O buffer length from the read CCW
//      pIOBuf    The I/O buffer from the read CCW
//      iCCWSeq   The sequence number of the CCW in the chain
//                (0 = first, 1 = second, 2 = third, etc).
// Output:
//      pMore     Set to 1 if packet data exceeds CCW count
//      pUnitStat The CSW status (CE+DE or CE+DE+UC or CE+DE+UC+SM)
//      pResidual The CSW residual byte count

void  ptp_write( DEVBLK* pDEVBLK, U32  uCount,
                 int     iCCWSeq, BYTE* pIOBuf,
                 BYTE*   pMore,   BYTE* pUnitStat,
                 U32*    pResidual )
{
    PTPATH*    pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;
    MPC_TH*    pMPC_TH  = (MPC_TH*)pIOBuf;
    PTPHX0*    pPTPHX0  = (PTPHX0*)pIOBuf;
    PTPHX2*    pPTPHX2  = (PTPHX2*)pIOBuf;

    int        iTraceLen;
    U32        uFirst4;


    // Get the first 4-bytes of what was writen by the guest.
    FETCH_FW( uFirst4, pMPC_TH->first4 );

    // Display up to 256-bytes of data, if debug is active
    if (pPTPBLK->uDebugMask & DBGPTPDATA)
    {
        // HHC00981 "%1d:%04X %s: Accept data of size %d bytes from guest"
        WRMSG(HHC00981, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,  pDEVBLK->typname, (int)uCount );
        iTraceLen = uCount;
        if (iTraceLen > 256)
        {
            iTraceLen = 256;
            // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
            WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                 iTraceLen, (int)(uCount - iTraceLen) );
        }
        net_data_trace( pDEVBLK, pIOBuf, iTraceLen, FROM_GUEST, 'D', "data", 0 );
    }

    // Process depending on what was writen by the guest.
    if (uCount >= SIZE_TH &&
        uFirst4 == MPC_TH_FIRST4)
    {

        // Display TH etc. structured, if debug is active
        if (pPTPBLK->uDebugMask & DBGPTPEXPAND)
        {
            // HHC00981 "%1d:%04X %s: Accept data of size %d bytes from guest"
            WRMSG(HHC00981, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,  pDEVBLK->typname,(int)uCount );
            mpc_display_ptp_th_etc( pDEVBLK, pMPC_TH, FROM_GUEST, 64 );
        }

        // Process the MPC_TH
        write_th( pDEVBLK, uCount, iCCWSeq, pIOBuf, pMore, pUnitStat, pResidual );

    }
    else if (uCount >= SIZE_HX0 &&
             pPTPHX0->TH_seg == 0x00 &&
             pPTPHX0->TH_ch_flag == TH_CH_0x00)
    {

        // Process the PTPHX0 type 0x00
        write_hx0_00( pDEVBLK, uCount, iCCWSeq, pIOBuf, pMore, pUnitStat, pResidual );

    }
    else if (uCount >= SIZE_HX0 &&
             pPTPHX0->TH_seg == 0x00 &&
             pPTPHX0->TH_ch_flag == TH_CH_0x01)
    {

        // Process the PTPHX0 type 0x01
        write_hx0_01( pDEVBLK, uCount, iCCWSeq, pIOBuf, pMore, pUnitStat, pResidual );

    }
    else if (uCount >= (SIZE_HX2 + SIZE_HSV) &&
             ( pPTPHX2->Ft & XID2_FORMAT_MASK ) == 0x20 &&
             pPTPHX2->NodeID[0] == 0xFF )
    {

        // Process the PTPHX2
        write_hx2( pDEVBLK, uCount, iCCWSeq, pIOBuf, pMore, pUnitStat, pResidual );

    }
    else
    {

        // HHC03931 "%1d:%04X PTP: Accept data of size %d bytes contains unknown data"
        WRMSG(HHC03931, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, (int)uCount );

        // Display up to 128-bytes of data, if debug is not active.
        // If debug is active, the data has already been diplayed.
        if (!(pPTPBLK->uDebugMask & DBGPTPDATA))
        {
            iTraceLen = uCount;
            if (iTraceLen > 128)
            {
                iTraceLen = 128;
                // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, (int)(uCount - iTraceLen) );
            }
            net_data_trace( pDEVBLK, pIOBuf, iTraceLen, FROM_GUEST, 'I', "data", 0 );
        }

        // None of the accepted data was sucessfully processed, and it will
        // now be dropped as though it never existed. Inform the guest that
        // the data was sucessfully processed.
        *pMore     = 0;
        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE;

    }

    return;
}   /* End function  ptp_write() */


/* ------------------------------------------------------------------ */
/* write_th()                                                       */
/* ------------------------------------------------------------------ */

void  write_th( DEVBLK* pDEVBLK, U32  uCount,
                int     iCCWSeq, BYTE* pIOBuf,
                BYTE*   pMore,   BYTE* pUnitStat,
                U32*    pResidual )
{

//  PTPATH*    pPTPATH   = pDEVBLK->dev_data;
//  PTPBLK*    pPTPBLK   = pPTPATH->pPTPBLK;
    MPC_TH*    pMPC_TH   = (MPC_TH*)pIOBuf;     // MPC_TH at start of IObuf
    MPC_RRH*   pMPC_RRH  = NULL;                // MPC_RRH
    int        iForRRH;
    U32        uOffRRH;
    U16        uNumRRH;
    int        rv = 0;
    int        iWhat;
#define UNKNOWN_RRH   0
#define RRH_8108      1
#define RRH_C108      2
#define RRH_417E      3
#define RRH_C17E      4

    UNREFERENCED( uCount  );
    UNREFERENCED( iCCWSeq );


    // Get the number of MPC_RRH and the displacement from the start
    // of the MPC_TH to the first (or only) MPC_RRH.
    FETCH_HW( uNumRRH, pMPC_TH->numrrh );
    FETCH_FW( uOffRRH, pMPC_TH->offrrh );

    // Process each of the MPC_RRHs.
    for( iForRRH = 1; iForRRH <= uNumRRH; iForRRH++ )
    {

        // Point to the first or subsequent MPC_RRH.
        pMPC_RRH = (MPC_RRH*)((BYTE*)pMPC_TH + uOffRRH);

        // Decide what the RRH contains.
        iWhat = UNKNOWN_RRH;
        if (pMPC_RRH->type == RRH_TYPE_CM)
        {
            if (pMPC_RRH->proto == PROTOCOL_LAYER2)
            {
                iWhat = RRH_8108;
            }
        }
        else if (pMPC_RRH->type == RRH_TYPE_ULP)
        {
            if (pMPC_RRH->proto == PROTOCOL_UNKNOWN)
            {
                iWhat = RRH_417E;
            }
        }
        else if (pMPC_RRH->type == RRH_TYPE_IPA)
        {
            if (pMPC_RRH->proto == PROTOCOL_LAYER2)
            {
                iWhat = RRH_C108;
            }
            else if (pMPC_RRH->proto == PROTOCOL_UNKNOWN)
            {
                iWhat = RRH_C17E;
            }
        }

        // Process what the RRH contains.
        switch( iWhat )
        {

        // IP packets
        case RRH_8108:
            rv = write_rrh_8108( pDEVBLK, pMPC_TH, pMPC_RRH );
            break;

        // Exchange IP information
        case RRH_C108:
            rv = write_rrh_C108( pDEVBLK, pMPC_TH, pMPC_RRH );
            break;

        // Establish connections
        case RRH_417E:
            rv = write_rrh_417E( pDEVBLK, pMPC_TH, pMPC_RRH );
            break;

        //
        case RRH_C17E:
            rv = write_rrh_C17E( pDEVBLK, pMPC_TH, pMPC_RRH );
            break;

        // Unknown RRH
        default:
            // HHC03936 "%1d:%04X PTP: Accept data contains unknown %s"
            WRMSG(HHC03936, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "RRH" );
            mpc_display_rrh( pDEVBLK, pMPC_RRH, FROM_GUEST );
            rv = -2;
            break;

        }

        // If the MPC_RRH processing was not sucessful, let's stop.
        if (rv != 0)
            break;

        // Get the displacement from the start of the MPC_TH to the
        // next MPC_RRH. pMPC_RRH->offrrh will contain zero if this
        // is the last MPC_RRH.
        FETCH_FW( uOffRRH, pMPC_RRH->offrrh );

    }

    // Set the residual byte count and unit status depending on
    // whether the MPC_RRHs have been processed sucessfully or not.
    switch( rv )
    {

    //    0   Successful
    //        All of the accepted data was sucessfully processed.
    //   -1   No storage available
    //   -2   Data problem (i.e. incomplete IP packet)
    //        None of the accepted data was sucessfully processed,
    //        or some of the accepted data may have been sucessfully
    //        processed. Either way there is some data that was not
    //        sucessfully processed, and it will now be dropped as
    //        though it never existed. Inform the guest that the
    //        data was sucessfully processed.
    case  0:
    case -1:
    case -2:
        *pMore     = 0;
        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE;
        break;

    //   -3   The TUNTAP_Write failed
    //        Hmm...
    case -3:
        pDEVBLK->sense[0] = SENSE_EC;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        break;

    }

    return;
}   /* End function  write_th() */


/* ------------------------------------------------------------------ */
/* write_rrh_8108()                                                   */
/* ------------------------------------------------------------------ */
// Note - the Token is xTokenUlpConnection.
// In all cases that have been seen the MPC_RRH type 0x8108 is followed
// by one or more MPC_PH, which are followed by data.
// The data in a PTP message is usually, but need not be, in a single,
// contiguous lump. The length and displacement to the various pieces
// of data are described by the MPC_PHs. If there are multiple pieces
// of data (i.e. there is more than one MPC_PH), this function copies
// the multiple pieces into a single contiguous lump in a buffer.
// Return value
//    0    Successful
//   -1    No storage available for a data buffer
//   -2    Data problem (i.e. incomplete IP packet)
//   -3    The TUNTAP_Write failed

int   write_rrh_8108( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH )
{
    PTPATH*    pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;
    MPC_PH*    pMPC_PH;
    BYTE*      pData;
    int        iDataLen;
    BYTE*      pDataBuf;
    U16        uNumPH;
    U16        uOffPH;
    int        iForPH;
    U32        uLenData;
    U32        uOffData;
    BYTE*      pData1;
    PIP4FRM    pIP4FRM;
    PIP6FRM    pIP6FRM;
    U16        uPayLen;
    int        iPktLen;
    u_int      fWantPkt;
    int        iPktVer;
    char       cPktVer[8];
    int        iTraceLen;
    int        rv;


    // Get the number of MPC_PHs and point to the first (or only) MPC_PH
    // following the MPC_RRH.
    FETCH_HW( uNumPH, pMPC_RRH->numph );
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);

    // Get the length of and the pointer to the data referenced by the
    // first (or only) MPC_PH
    FETCH_F3( uLenData, pMPC_PH->lendata );
    FETCH_FW( uOffData, pMPC_PH->offdata );
    pData = (BYTE*)pMPC_TH + uOffData;

    // Get the total length of the data referenced by all of the MPC_PHs.
    FETCH_F3( iDataLen, pMPC_RRH->lenalda );

    // Check whether there is more than one MPC_PH.
    if (uNumPH == 1)
    {
        pDataBuf = NULL;
    }
    else
    {
        // More than one MPC_PH. Allocate a buffer in which all of
        // the data referenced by the MPC_PHs will be concatanated.
        pDataBuf = alloc_storage( pDEVBLK, iDataLen );   // Allocate buffer
        if (!pDataBuf)               // if the allocate was not successful...
            return -1;

        // Copy and concatanate the data referenced by the MPC_PHs.
        pData = pDataBuf;
        for( iForPH = 1; iForPH <= uNumPH; iForPH++ )
        {
            FETCH_F3( uLenData, pMPC_PH->lendata );
            FETCH_FW( uOffData, pMPC_PH->offdata );
            pData1 = (BYTE*)pMPC_TH + uOffData;

            memcpy( pData, pData1, uLenData );
            pData += uLenData;

            pMPC_PH = (MPC_PH*)((BYTE*)pMPC_PH + SIZE_PH);
        }

        // Point to the copied and concatenated data.
        pData = pDataBuf;
    }

    // pData points to and iDataLen contains the length of a contiguous
    // lump of storage that contains the data in the message. The data
    // consists of one or more IP packets.
    while( iDataLen > 0 )
    {
        pIP4FRM = (PIP4FRM)pData;
        pIP6FRM = (PIP6FRM)pData;

        // Check the IP packet version. The first 4-bits of the first
        // byte of the IP header contains the version number.
        iPktVer = ( ( pData[0] & 0xF0 ) >> 4 );
        if (iPktVer == 4)
        {
            strcpy( cPktVer, " IPv4" );
        }
        else if (iPktVer == 6)
        {
            strcpy( cPktVer, " IPv6" );
        }
        else
        {
            // Err... not IPv4 or IPv6!
            // HHC03933 "%1d:%04X PTP: Accept data for device '%s' contains IP packet with unknown IP version, data dropped"
            WRMSG(HHC03933, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                 pPTPBLK->szTUNIfName );
            iTraceLen = iDataLen;
            if (iTraceLen > 128)
            {
                iTraceLen = 128;
                // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, iDataLen - iTraceLen );
            }
            net_data_trace( pDEVBLK, pData, iTraceLen, FROM_GUEST, 'I', "data", 0 );
            rv = -2;
            break;
        }

        // Check that there is a whole IP packet, and that
        // the IP packet is no larger than the TUN interface MTU.
        if (iPktVer == 4)
        {
            if (iDataLen >= (int)sizeof(IP4FRM))   // Size of a minimal IPv4 header
            {
                // Calculate the IPv4 packet length.
                FETCH_HW( uPayLen, pIP4FRM->hwTotalLength );
                iPktLen = uPayLen;
            }
            else
            {
                iPktLen = -1;
            }
        }
        else
        {
            if (iDataLen >= (int)sizeof(IP6FRM))   // Size of an IPv6 header
            {
                // Calculate the IPv6 packet length.
                FETCH_HW( uPayLen, pIP6FRM->bPayloadLength );
                iPktLen = sizeof(IP6FRM) + uPayLen;
            }
            else
            {
                iPktLen = -1;
            }
        }
        if (iPktLen > iDataLen || iPktLen == -1)
        {
            // HHC03934 "%1d:%04X PTP: Accept data for device '%s' contains incomplete IP packet, data dropped"
            WRMSG(HHC03934, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                 pPTPBLK->szTUNIfName );
            iTraceLen = iDataLen;
            if (iTraceLen > 128)
            {
                iTraceLen = 128;
                // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, iDataLen - iTraceLen );
            }
            net_data_trace( pDEVBLK, pData, iTraceLen, FROM_GUEST, 'I', "data", 0 );
            rv = -2;
            break;
        }
        if (iPktLen > pPTPBLK->iMTU)
        {
            // HHC03935 "%1d:%04X PTP: Accept data for device '%s' contains IP packet larger than MTU, data dropped"
            WRMSG(HHC03935, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                 pPTPBLK->szTUNIfName );
            iTraceLen = iDataLen;
            if (iTraceLen > 128)
            {
                iTraceLen = 128;
                // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, iDataLen - iTraceLen );
            }
            net_data_trace( pDEVBLK, pData, iTraceLen, FROM_GUEST, 'I', "data", 0 );
            rv = -2;
            break;
        }

        // Check whether the TUN interface is ready for the IP packet.
        fWantPkt = TRUE;
        if (iPktVer == 4)
        {
            if (!pPTPBLK->fActive4)
            {
                fWantPkt = FALSE;
            }
        }
        else
        {
            if (!pPTPBLK->fActive6)
            {
                fWantPkt = FALSE;
            }
        }

        //
        if (fWantPkt)
        {
            // Trace the IP packet before sending to TUN interface
            if (pPTPBLK->uDebugMask & DBGPTPPACKET)
            {
                // HHC00910 "%1d:%04X %s: Send%s packet of size %d bytes to device %s"
                WRMSG(HHC00910, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     cPktVer, iPktLen, pPTPBLK->szTUNIfName );
                net_data_trace( pDEVBLK, pData, iPktLen, FROM_GUEST, 'D', "packet", 0 );
            }

            // Write the IP packet to the TUN interface
            rv = TUNTAP_Write( pPTPBLK->fd, pData, iPktLen );
            if (rv < 0)
            {
                // HHC00911 "%1d:%04X %s: error writing to device %s: %d %s"
                WRMSG(HHC00911, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                        pPTPBLK->szTUNIfName, errno, strerror( errno ) );
                rv = -3;
                break;
            }
        }

        rv = 0;

        // Point to the next IP packet, if there is one.
        pData += iPktLen;
        iDataLen -= iPktLen;

    }   /* while( iDataLen > 0 ) */

    // Free the data buffer, if one was used.
    if (pDataBuf)
        free( pDataBuf );

    return rv;
}   /* End function  write_rrh_8108() */

// -------------------------------------------------------------------
//                     ptp_halt_or_clear
// -------------------------------------------------------------------
// The channel is processing a Halt Subchannel or Clear Subchannel
// instruction and is notifying us of that fact so we can stop our
// ptp_read CCW processing loop.

static void ptp_halt_or_clear( DEVBLK* pDEVBLK )
{
    PTPATH*    pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;
    obtain_lock( &pPTPBLK->ReadEventLock );
    {
        if (pPTPBLK->fReadWaiting)
        {
            pPTPBLK->fHaltOrClear = 1;
            signal_condition( &pPTPBLK->ReadEvent );
        }
    }
    release_lock( &pPTPBLK->ReadEventLock );
}

/* ------------------------------------------------------------------ */
/* ptp_read()                                                         */
/* ------------------------------------------------------------------ */
// Input:
//      pDEVBLK   A pointer to the CTC adapter device block
//      uCount    The I/O buffer length from the read CCW
//      pIOBuf    The I/O buffer from the read CCW
//      iCCWSeq   The sequence number of the CCW in the chain
//                (0 = first, 1 = second, 2 = third, etc).
// Output:
//      pMore     Set to 1 if packet data exceeds CCW count
//      pUnitStat The CSW status (CE+DE or CE+DE+UC or CE+DE+UC+SM)
//      pResidual The CSW residual byte count

void  ptp_read( DEVBLK* pDEVBLK, U32  uCount,
                int     iCCWSeq, BYTE* pIOBuf,
                BYTE*   pMore,   BYTE* pUnitStat,
                U32*    pResidual )
{
    PTPATH*    pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;
    PTPHDR*    pPTPHDR  = NULL;
    int        rc       = 0;
    struct timespec waittime;
    struct timeval  now;
    BYTE       haltorclear = FALSE;

    if (pPTPATH->bDLCtype == DLCTYPE_READ)     // Read from the y-side's Read path?
    {
        // The read is from the y-side's Read path.
        for (;;)
        {
            // Return the data from a chain buffer to the guest OS.
            // There will be chain buffers on the Read path during
            // handshaking, and just after the IPv6 connection has
            // become active.

            // Remove first buffer from chain.
            pPTPHDR = remove_buffer_from_chain( pPTPATH );
            if (pPTPHDR)
            {
                // Return the data to the guest OS.
                read_chain_buffer( pDEVBLK, uCount, iCCWSeq, pIOBuf,
                                   pMore, pUnitStat, pResidual, pPTPHDR );

                // Free the buffer.
                free( pPTPHDR );
                return;
            }

            // Return the data from the read buffer to the guest OS.
            // There is a read buffer on the Read path, and the buffer
            // will contain data after the IPv4 and/or IPv6 connection
            // has become active.

            // Obtain the read buffer lock.
            obtain_lock( &pPTPBLK->ReadBufferLock );

            pPTPHDR = pPTPBLK->pReadBuffer;
            if (pPTPHDR && pPTPHDR->iDataLen > LEN_OF_PAGE_ONE)
            {
                // Return the data to the guest OS.
                read_read_buffer( pDEVBLK, uCount, iCCWSeq, pIOBuf,
                                       pMore, pUnitStat, pResidual, pPTPHDR );

                // Release the read buffer lock.
                release_lock( &pPTPBLK->ReadBufferLock );
                return;
            }

            // Release the read buffer lock.
            release_lock( &pPTPBLK->ReadBufferLock );

            // There is no data waiting to be read.
            // Calculate when to end the wait.
            gettimeofday( &now, NULL );

            waittime.tv_sec  = now.tv_sec  + PTP_READ_TIMEOUT_SECS;
            waittime.tv_nsec = now.tv_usec * 1000;

            // Obtain the event lock
            obtain_lock( &pPTPBLK->ReadEventLock );

            // Use a calculated wait
            pPTPBLK->fReadWaiting = 1;
            rc = timed_wait_condition( &pPTPBLK->ReadEvent,
                                       &pPTPBLK->ReadEventLock,
                                       &waittime );
            pPTPBLK->fReadWaiting = 0;

            // check for halt condition
            if (pPTPBLK->fHaltOrClear)
            {
                haltorclear = TRUE;
                pPTPBLK->fHaltOrClear = 0;
            }

            // Release the event lock
            release_lock( &pPTPBLK->ReadEventLock );

            // check for halt condition
            if (haltorclear)
            {
                if (pDEVBLK->ccwtrace)
                {
                    // HHC00904 "%1d:%04X %s: halt or clear recognized"
                    WRMSG(HHC00904, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname );
                }
                *pUnitStat = CSW_CE | CSW_DE;
                *pResidual = uCount;
                return;
            }
        } /* for (;;) */
    }
    else
    {
        // The read is from the y-side's Write path. There should only
        // ever be reads from the y-side's Write path while the XID2
        // exchange is in progress during handshaking.

        // Remove first buffer from chain.
        pPTPHDR = remove_buffer_from_chain( pPTPATH );
        if (pPTPHDR)
        {
            // There is a buffer on the chain waiting to be read.
            read_chain_buffer( pDEVBLK, uCount, iCCWSeq, pIOBuf,
                               pMore, pUnitStat, pResidual, pPTPHDR );

            // Free the buffer.
            free( pPTPHDR );

            return;
        }
        else
        {
            // There is no buffer on the chain waiting to be read. This
            // should not happen! Read a load of nulls.
            memset( pIOBuf, 0, (int)uCount );
            *pMore     = 0;
            *pResidual = 0;
            *pUnitStat = CSW_CE | CSW_DE;

            return;
        }

    }   /* if (pPTPATH->bDLCtype == DLCTYPE_READ ) */

    return;
}   /* End function  ptp_read() */


/* ------------------------------------------------------------------ */
/* read_read_buffer()                                                 */
/* ------------------------------------------------------------------ */
// Note: The caller must hold the PTPBLK->ReadBufferLock.

ENABLE_VS_BUG_ID_363375_BYPASS

void  read_read_buffer( DEVBLK* pDEVBLK,   U32     uCount,
                        int     iCCWSeq,   BYTE*   pIOBuf,
                        BYTE*   pMore,     BYTE*   pUnitStat,
                        U32*    pResidual, PTPHDR* pPTPHDR )
{

    PTPATH*    pPTPATH  = pDEVBLK->dev_data;   // PTPATH
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;    // PTPBLK
    MPC_TH*    pMPC_TH;                        // MPC_TH follows the PTPHDR
    MPC_RRH*   pMPC_RRH;                       // MPC_RRH follows the MPC_TH
    MPC_PH*    pMPC_PH;                        // MPC_PH follows the MPC_RRH
    int        iDataLen;
    int        iIOLen;
    int        iLength1;
    int        iLength2;
    U32        uTotalLen;
    int        iTraceLen;

    UNREFERENCED( iCCWSeq );


    // Point to the data and get its length.
    pMPC_TH = (MPC_TH*)((BYTE*)pPTPHDR + SIZE_HDR);
    pMPC_RRH = (MPC_RRH*)((BYTE*)pMPC_TH + SIZE_TH);
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + SIZE_RRH);
    iDataLen = pPTPHDR->iDataLen - LEN_OF_PAGE_ONE;

    // Set the transmission header sequence number.
    STORE_FW( pMPC_TH->seqnum, ++pPTPATH->uSeqNum );

    // Set the destination Token.
    pMPC_RRH->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRH->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Check whether all of the data that is currently in page two and
    // onwards will fit into page one.
    if (iDataLen <= (LEN_OF_PAGE_ONE - (int)(SIZE_TH + SIZE_RRH + SIZE_PH)))
    {

        // All of the data that is currently in page two and onwards
        // will fit into page one. Copy the headers and the data so
        // that the headers and the data are contiguous in the guests
        // read buffer, i.e. the layout is different to that set-up by
        // ptp_read_thread().

        // Set the residual length and unit status.
        if (uCount >= (U32)(SIZE_TH + SIZE_RRH + SIZE_PH + iDataLen))
        {
            iLength1 = (int)(SIZE_TH + SIZE_RRH + SIZE_PH);
            iLength2 = iDataLen;
            *pMore     = 0;
            *pResidual = uCount - (U32)(iLength1 + iLength2);
        }
        else
        {
            if (uCount >= (U32)(SIZE_TH + SIZE_RRH + SIZE_PH))
            {
                iLength1 = (int)(SIZE_TH + SIZE_RRH + SIZE_PH);
                iLength2 = (int)uCount - iLength1;
            }
            else
            {
                iLength1 = (int)uCount;
                iLength2 = 0;
            }
            *pMore     = 1;
            *pResidual = 0;
        }
        *pUnitStat = CSW_CE | CSW_DE;

        // Set length field in MPC_TH
        STORE_FW( pMPC_TH->length, (U32)(iLength1 + iLength2) );

        // Set length fields in MPC_RRH
        STORE_HW( pMPC_RRH->lenfida, (U16)iLength2 );
        STORE_F3( pMPC_RRH->lenalda, (U32)iLength2 );

        // Prepare MPC_PH
        pMPC_PH->locdata = PH_LOC_1;
        STORE_F3( pMPC_PH->lendata, (U32)iLength2 );
        STORE_FW( pMPC_PH->offdata, (U32)(SIZE_TH + SIZE_RRH + SIZE_PH) );

        // Copy the data to be read to the IO buffer.
        memcpy( pIOBuf, pMPC_TH, iLength1 );
        memcpy( pIOBuf + iLength1, (BYTE*)pMPC_TH + LEN_OF_PAGE_ONE, iLength2 );

        // Set the length of the data copied to the IO buffer.
        iIOLen =  iLength1 + iLength2;

    }
    else
    {

        // All of the data that is currently in page two and onwards
        // will not fit into page one. Copy the headers and the data
        // so that the headers and the data are not contiguous in the
        // guests read buffer, i.e. the layout is the same as that
        // set-up by ptp_read_thread().

        // Set the residual length and unit status.
        if (uCount >= (U32)(LEN_OF_PAGE_ONE + iDataLen))
        {
            iLength1 = LEN_OF_PAGE_ONE;
            iLength2 = iDataLen;
            *pMore     = 0;
            *pResidual = uCount - (U32)(iLength1 + iLength2);
        }
        else
        {
            if (uCount >= (U32)LEN_OF_PAGE_ONE)
            {
                iLength1 = LEN_OF_PAGE_ONE;
                iLength2 = (int)uCount - iLength1;
            }
            else
            {
                iLength1 = (int)uCount;
                iLength2 = 0;
            }
            *pMore     = 1;
            *pResidual = 0;
        }
        *pUnitStat = CSW_CE | CSW_DE;

        // Set length field in MPC_TH
        STORE_FW( pMPC_TH->length, (U32)(SIZE_TH + SIZE_RRH + SIZE_PH) );

        // Set length fields in MPC_RRH
        STORE_HW( pMPC_RRH->lenfida, 0 );
        STORE_F3( pMPC_RRH->lenalda, (U32)iLength2 );

        // Prepare MPC_PH
        pMPC_PH->locdata = PH_LOC_2;
        STORE_F3( pMPC_PH->lendata, (U32)iLength2 );
        STORE_FW( pMPC_PH->offdata, (U32)(LEN_OF_PAGE_ONE) );

        // Copy the data to be read to the IO buffer.
        memcpy( pIOBuf, pMPC_TH, iLength1 + iLength2 );

        // Set the length of the data copied to the IO buffer.
        iIOLen =  iLength1 + iLength2;

    }

    // Display TH etc. structured, if debug is active
    if (pPTPBLK->uDebugMask & DBGPTPEXPAND)
    {
        // HHC00982 "%1d:%04X %s: Present data of size %d bytes to guest"
        WRMSG(HHC00982, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, iIOLen );
        mpc_display_ptp_th_etc( pDEVBLK, (MPC_TH*)pIOBuf, TO_GUEST, 64 );
    }

    // Display up to 256-bytes of the read data, if debug is active.
    if (pPTPBLK->uDebugMask & DBGPTPDATA)
    {
        // HHC00982 "%1d:%04X %s: Present data of size %d bytes to guest"
        WRMSG(HHC00982, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, iIOLen );
        FETCH_FW( uTotalLen, pMPC_TH->length );
        iTraceLen = uTotalLen;
        if (iTraceLen > 256)
        {
            iTraceLen = 256;
            // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
            WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                 iTraceLen, (int)(uTotalLen - iTraceLen) );
        }
        net_data_trace( pDEVBLK, pIOBuf, iTraceLen, TO_GUEST, 'D', "data", 0 );
    }

    // Reset length field in PTPHDR
    pPTPHDR->iDataLen = LEN_OF_PAGE_ONE;

    // Clear length field in MPC_TH
    STORE_FW( pMPC_TH->length, 0 );

    // Clear length fields in MPC_RRH
    STORE_HW( pMPC_RRH->lenfida, 0 );
    STORE_F3( pMPC_RRH->lenalda, 0 );

    // Clear location, length and displacement fields in MPC_PH
    pMPC_PH->locdata = 0;
    STORE_F3( pMPC_PH->lendata, 0 );
    STORE_FW( pMPC_PH->offdata, 0 );

    return;
}   /* End function  read_read_buffer() */

DISABLE_VS_BUG_ID_363375_BYPASS


/* ------------------------------------------------------------------ */
/* read_chain_buffer()                                                */
/* ------------------------------------------------------------------ */

void  read_chain_buffer( DEVBLK* pDEVBLK,   U32  uCount,
                         int     iCCWSeq,   BYTE* pIOBuf,
                         BYTE*   pMore,     BYTE* pUnitStat,
                         U32*    pResidual, PTPHDR* pPTPHDR )
{
    PTPATH*    pPTPATH  = pDEVBLK->dev_data;   // PTPATH
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;    // PTPBLK
    MPC_TH*    pMPC_TH;                        // MPC_TH follows the PTPHDR
    int        iDataLen;
    int        iTraceLen;
    int        rc;
    U32        uFirst4;


    // Point to the data and get its length.
    pMPC_TH = (MPC_TH*)((BYTE*)pPTPHDR + SIZE_HDR);
    iDataLen = pPTPHDR->iDataLen;

    // Get the first 4-bytes of the data.
    FETCH_FW( uFirst4, pMPC_TH->first4 );

    // Set the residual length and unit status.
    if (uCount >= (U32)iDataLen)
    {
        *pMore     = 0;
        *pResidual = uCount - (U32)iDataLen;
    }
    else
    {
        iDataLen   = uCount;
        *pMore     = 1;
        *pResidual = 0;
    }
    *pUnitStat = CSW_CE | CSW_DE;

    // Set the transmission header sequence number, if necessary.
    if (uFirst4 == MPC_TH_FIRST4)
    {
        STORE_FW( pMPC_TH->seqnum, ++pPTPATH->uSeqNum );
    }

    // Copy the data to be read.
    memcpy( pIOBuf, pMPC_TH, iDataLen );

    // Display TH etc. structured, if debug is active
    if (uFirst4 == MPC_TH_FIRST4 && (pPTPBLK->uDebugMask & DBGPTPEXPAND))
    {
        // HHC00982 "%1d:%04X %s: Present data of size %d bytes to guest"
        WRMSG(HHC00982, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, iDataLen );
        mpc_display_ptp_th_etc( pDEVBLK, pMPC_TH, TO_GUEST, 64 );
    }

    // Display up to 256-bytes of the read data, if debug is active.
    if (pPTPBLK->uDebugMask & DBGPTPDATA)
    {
        // HHC00982 "%1d:%04X %s: Present data of size %d bytes to guest"
        WRMSG(HHC00982, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, iDataLen );
        iTraceLen = iDataLen;
        if (iTraceLen > 256)
        {
            iTraceLen = 256;
            // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
            WRMSG(HHC00980, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                 iTraceLen, iDataLen - iTraceLen );
        }
        net_data_trace( pDEVBLK, pIOBuf, iTraceLen, TO_GUEST, 'D', "data", 0 );
    }

    // When we are handshaking the sixth CCW in the chain marks the
    // end of an exchange.
    if (pPTPATH->fHandshaking && iCCWSeq == 5)
    {
        if (pPTPATH->fHandshakeCur == HANDSHAKE_ONE)     // Handshake one in progress?
        {
            // The end of the first exchange. The y-side VTAM will
            // now wait (for up to 90 seconds) for an Attention
            // interrupt, which indicates to the y-side that the
            // x-side has initiated the second exchange.
            // Raise an attention interrupt in one second.
            pPTPATH->bAttnCode = 0x17;
            rc = raise_unsol_int( pDEVBLK, CSW_ATTN, 1000 );
            if (rc)
            {
                // Any bad news has already been reported.
                // Hmm... the Attention interrupt to the y-side will not be
                // raised. The y-side's VTAM will timeout in 90 seconds.
            }
            pPTPATH->fHandshakeFin |= HANDSHAKE_ONE;     // Handshake one finished
        }
        if (pPTPATH->fHandshakeCur == HANDSHAKE_TWO)     // Handshake two in progress?
        {
            pPTPATH->fHandshakeFin |= HANDSHAKE_TWO;     // Handshake two finished
        }
        if (pPTPATH->fHandshakeCur == HANDSHAKE_THREE)   // Handshake three in progress?
        {
            pPTPATH->fHandshakeFin |= HANDSHAKE_THREE;   // Handshake three finished
        }
        // If all three exchanges have finished, handshaking is complete.
        if (pPTPATH->fHandshakeFin == HANDSHAKE_ALL)     // All handshakes finished?
        {
            pPTPATH->fHandshaking = FALSE;
            pPTPATH->fHandshakeCur = 0;
            pPTPATH->fHandshakeSta = 0;
            pPTPATH->fHandshakeFin = 0;
        }
    }

    return;
}   /* End function  read_chain_buffer() */


/* ------------------------------------------------------------------ */
/* ptp_read_thread()                                                  */
/* ------------------------------------------------------------------ */
// The ptp_read_thread() reads data from the TUN interface, stores
// the data in the path read buffer, from where the data is read by
// the read path of the MPCPTP/MPCPTP6 connection.
//
// The size of the read buffer is determined by the maximum read
// length reported by the y-side during handshaking. The y-side
// calculates its maximum read length from the MAXBFRU value
// specified in the TRLE definition. A MAXBFRU value from 1 to 16 can
// be specified. If MAXBFRU is not specified, a default value of 5 is
// used. The MAXBFRU value specifies the number of 4K buffer pages
// used to receive data. The resulting buffer size (and maximun read
// length) is number_of_pages multiplied by 4096, minus 4 (the
// 4-bytes are used for an eye-catcher of 'WrHP' at the start of the
// first page), i.e. (MAXBFRU*4096)-4 .
//
// Note: VTAM automatically substitues a value of 16 for any coded
// MAXBFRU value higher than 16 without issuing a warning message.
// Empirical evidence also suggests that VTAM substitutes a value of
// 5 (the default value) for any coded MAXBFRU value lower than 5
// without issuing a warning message. I don't think this is a bug, I
// think it is a feature. MPCPTP/MPCPTP6 connections are claimed to
// be high-performance connections, so perhaps it was decided that
// choking the connection with a small buffer size was a daft idea.
//
// The MAXBFRU value also determines the maximum MTU that will be
// used on the MPCPTP/MPCPTP6 connection. The resulting maximum MTU
// is number_of_pages minus 1, multiplied by 4096, minus 2048.
//   i.e. ((MAXBFRU-1)*4096)-2048
// Alternativley, starting with the maximum read length, the
// resulting maximum MTU is maximum_read_length minus 4092, minus
// 2048.
//   i.e. (Maximum_read_length-4092)-2048
// The maximum read length and maximum MTU (in bytes) for various
// MAXBFRU values are shown below.
//
//   MAXBFRU value     Maximum read length      Maximum MTU
//        5              20476 (0x4FFC)        14336 (0x3800)
//        8              32764 (0x7FFC)        26624 (0x6800)
//       12              49148 (0xBFFC)        43008 (0xA800)
//       16              65532 (0xFFFC)        59392 (0xE800)
//
// Suppose the x-side reports to the y-side that the x-side's maximum
// read length is 20476 (0x4FFC) bytes, the y-side will calculate
// that the actual MTU is 14336 (0x3800) bytes. If the y-side has a
// route statement that specifies an MTU of, for example, 24576
// (0x6000) bytes, the MTU specified on the route statement is
// ignored and the calculated actual MTU is used.
// Depending on the values specified for MAXBFRU and for routes, the
// MTU in use from the x-side to the y-side could be different to the
// MTU in use from the y-side to the x-side. For a real MPCPTP/
// MPCPTP6 connection this is probably a good thing, the maximum
// capacity in each direction is automatically used, and the system
// administrator at one end of the connection does not need to know
// the values in use at the other end of the connection. However, for
// this emulated MPCPTP/MPCPTP6 connection this could be a very bad
// thing. Because we are not processing the packets, we are simply
// forwarding them, we may be forwarding them from something using a
// larger MTU to something using a smaller MTU.

void*  ptp_read_thread( void* arg )
{
    PTPBLK*    pPTPBLK = (PTPBLK*) arg;
    PTPATH*    pPTPATH = pPTPBLK->pPTPATHRead; // PTPATH Read
    DEVBLK*    pDEVBLK = pPTPATH->pDEVBLK;     // DEVBLK
    BYTE*      pTunBuf;                        // TUN read buffer address
    PIP4FRM    pIP4FRM;                        // IPv4 packet in TUN read buffer
    PIP6FRM    pIP6FRM;                        // IPv6 packet in TUN read buffer
    int        iTunLen;                        // TUN read length
    int        iLength;                        // Length of data in TUN read buffer
    PTPHDR*    pPTPHDR;                        // PTPHDR of the path read buffer
    MPC_TH*    pMPC_TH;                        // MPC_TH follows the PTPHDR
//  MPC_RRH*   pMPC_RRH;                       // MPC_RRH follows the MPC_TH
//  MPC_PH*    pMPC_PH;                        // MPC_PH follows the MPC_RRH
    BYTE*      pData;                          //
    U16        uPayLen;
    int        iPktVer;
    char       cPktVer[8];
    int        iPktLen;
    int        iTraceLen;


    // Allocate the TUN read buffer.
    iTunLen = pPTPBLK->iMTU;                      // Read length and buffer size
    pTunBuf = alloc_storage( pDEVBLK, iTunLen );  // equal to the MTU size
    if (!pTunBuf)                    // if the allocate failed...
    {
        // Close the TUN interface.
        VERIFY( pPTPBLK->fd == -1 || TUNTAP_Close( pPTPBLK->fd ) == 0 );
        pPTPBLK->fd = -1;
        // Nothing else to be done.
        return NULL;
    }
    pIP6FRM = (PIP6FRM)pTunBuf;
    pIP4FRM = (PIP4FRM)pTunBuf;

    // ZZ FIXME: Try to avoid race condition at startup with hercifc
    SLEEP(10);

    pPTPBLK->pid = getpid();

    // Keep going until we have to stop.
    while( pPTPBLK->fd != -1 && !pPTPBLK->fCloseInProgress )
    {

        // Read an IP packet from the TUN interface.
        iLength = TUNTAP_Read( pPTPBLK->fd, (void*)pTunBuf, iTunLen );

        // Check for error conditions...
        if (iLength < 0)
        {
            if (!pPTPBLK->fCloseInProgress)
            {
                // HHC00912 "%1d:%04X %s: error reading from device %s: %d %s"
                WRMSG(HHC00912, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     pPTPBLK->szTUNIfName, errno, strerror( errno ) );
            }
            break;
        }

        if (iLength == 0)       // (probably EINTR; ignore)
            continue;

        // Check the IP packet version. The first 4-bits of the first
        // byte of the IP header contains the version number.
        iPktVer = ( ( pTunBuf[0] & 0xF0 ) >> 4 );
        if (iPktVer == 4)
        {
            strcpy( cPktVer, " IPv4" );
        }
        else if (iPktVer == 6)
        {
            strcpy( cPktVer, " IPv6" );
        }
        else
        {
            // Err... not IPv4 or IPv6!
            // HHC03921 "%1d:%04X PTP: Packet of size %d bytes from device '%s' has an unknown IP version, packet dropped"
            WRMSG(HHC03921, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                 iLength, pPTPBLK->szTUNIfName );
            iTraceLen = iLength;
            if (iTraceLen > 128)
            {
                iTraceLen = 128;
                // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, iLength - iTraceLen );
            }
            net_data_trace( pDEVBLK, (BYTE*)pTunBuf, iTraceLen, TO_GUEST, 'I', "data", 0 );
            continue;
        }

        // Check that a whole IP packet has been read. If an incomplete
        // packet has been read it is dropped.
        if (iPktVer == 4)
        {
            if (iLength >= (int)sizeof(IP4FRM))   // Size of a minimal IPv4 header
            {
                // Calculate the IPv4 packet length.
                FETCH_HW( uPayLen, pIP4FRM->hwTotalLength );
                iPktLen = uPayLen;
            }
            else
            {
                iPktLen = -1;
            }
        }
        else
        {
            if (iLength >= (int)sizeof(IP6FRM))   // Size of an IPv6 header
            {
                // Calculate the IPv6 packet length.
                FETCH_HW( uPayLen, pIP6FRM->bPayloadLength );
                iPktLen = sizeof(IP6FRM) + uPayLen;
            }
            else
            {
                iPktLen = -1;
            }
        }
        if (iPktLen != iLength)
        {
            // HHC03922 "%1d:%04X PTP: Packet of size %d bytes from device '%s' is not equal to the packet length of %d bytes, packet dropped"
            WRMSG(HHC03922, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                 iLength, pPTPBLK->szTUNIfName,
                                 iPktLen );
            iTraceLen = iLength;
            if (iTraceLen > 128)
            {
                iTraceLen = 128;
                // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                     iTraceLen, iLength - iTraceLen );
            }
            net_data_trace( pDEVBLK, (BYTE*)pTunBuf, iTraceLen, TO_GUEST, 'I', "data", 0 );
            continue;
        }

        // Enqueue IP packet.
        while( pPTPBLK->fd != -1 && !pPTPBLK->fCloseInProgress )
        {

            // Obtain the read buffer lock.
            obtain_lock( &pPTPBLK->ReadBufferLock );

            // Check whether the interface is ready for data from the TUN interface.
            if (iPktVer == 4)
            {
                if (!pPTPBLK->fActive4)
                {
                    // Release the read buffer lock.
                    release_lock( &pPTPBLK->ReadBufferLock );
                    break;
                }
            }
            else
            {
                if (!pPTPBLK->fActive6)
                {
                    // Release the read buffer lock.
                    release_lock( &pPTPBLK->ReadBufferLock );
                    break;
                }
            }

            // Check that there is a read buffer.
            pPTPHDR = pPTPBLK->pReadBuffer;
            if (!pPTPHDR)
            {
                // Release the read buffer lock.
                release_lock( &pPTPBLK->ReadBufferLock );
                break;
            }

            // Check whether the IP packet is larger than y-side's actual MTU.
            // If it is then it is dropped.
            if (iLength > pPTPBLK->yActMTU)
            {
                // Release the read buffer lock.
                release_lock( &pPTPBLK->ReadBufferLock );
                // HHC03923 "%1d:%04X PTP: Packet of size %d bytes from device '%s' is larger than the guests actual MTU of %d bytes, packet dropped"
                WRMSG(HHC03923, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                     iLength, pPTPBLK->szTUNIfName,
                                     (int)pPTPBLK->yActMTU );
                iTraceLen = iLength;
                if (iTraceLen > 128)
                {
                    iTraceLen = 128;
                    // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                    WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                         iTraceLen, iLength - iTraceLen );
                }
                net_data_trace( pDEVBLK, (BYTE*)pTunBuf, iTraceLen, TO_GUEST, 'I', "data", 0 );
                break;
            }

            // Check whether the IP packet will ever fit into the read buffer.
            // If it will not then it is dropped.
            if (iLength > (pPTPHDR->iAreaLen - LEN_OF_PAGE_ONE))
            {
                // Release the read buffer lock.
                release_lock( &pPTPBLK->ReadBufferLock );
                // HHC03924 "%1d:%04X PTP: Packet of size %d bytes from device '%s' is too large for read buffer area of %d bytes, packet dropped"
                WRMSG(HHC03924, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                     iLength, pPTPBLK->szTUNIfName,
                                     pPTPHDR->iAreaLen - LEN_OF_PAGE_ONE );
                iTraceLen = iLength;
                if (iTraceLen > 128)
                {
                    iTraceLen = 128;
                    // HHC00980 "%1d:%04X PTP: Data of size %d bytes displayed, data of size %d bytes not displayed"
                    WRMSG(HHC00980, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                         iTraceLen, iLength - iTraceLen );
                }
                net_data_trace( pDEVBLK, (BYTE*)pTunBuf, iTraceLen, TO_GUEST, 'I', "data", 0 );
                break;
            }

            // Check whether the IP packet will fit into the read buffer.
            if (iLength > (pPTPHDR->iAreaLen - pPTPHDR->iDataLen))
            {
                // The IP packet will not fit into the read buffer at the
                // moment, presumably there are IP packets waiting to be read.

                // Release the read buffer lock.
                release_lock( &pPTPBLK->ReadBufferLock );

                // Don't use schedyield() here; use an actual non-dispatchable
                // delay instead so as to allow another [possibly lower priority]
                // thread to 'read' (remove) the packet(s) from the read buffer.
                USLEEP( PTP_DELAY_USECS );  // (wait a bit before retrying...)

                continue;

            }
            else
            {
                // The IP packet will fit into the read buffer.

                // Display the IP packet just read, if the device group is being debugged.
                if (pPTPBLK->uDebugMask & DBGPTPPACKET)
                {
                    // HHC00913 "%1d:%04X %s: Receive%s packet of size %d bytes from device %s"
                    WRMSG(HHC00913, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                         cPktVer, iLength, pPTPBLK->szTUNIfName );
                    net_data_trace( pDEVBLK, (BYTE*)pTunBuf, iLength, TO_GUEST, 'D', "packet", 0 );
                }

                // Fix-up various pointers
                pMPC_TH = (MPC_TH*)((BYTE*)pPTPHDR + SIZE_HDR);
                pData   = (BYTE*)pMPC_TH + pPTPHDR->iDataLen;

                // Copy the IP packet from the TUN/TAP read buffer
                memcpy( pData, pTunBuf, iLength );

                // Increment length field in PTPHDR
                pPTPHDR->iDataLen += iLength;

                // Release the read buffer lock.
                release_lock( &pPTPBLK->ReadBufferLock );

                //
                obtain_lock( &pPTPBLK->ReadEventLock );
                signal_condition( &pPTPBLK->ReadEvent );
                release_lock( &pPTPBLK->ReadEventLock );

                break;

            }   /* if (iLength > (pPTPHDR->iAreaLen - pPTPHDR->iDataLen)) */

        }   /* while( pPTPBLK->fd != -1 && !pPTPBLK->fCloseInProgress ) */

    }   /* while( pPTPBLK->fd != -1 && !pPTPBLK->fCloseInProgress ) */

    // We must do the close since we were the one doing the i/o...
    VERIFY( pPTPBLK->fd == -1 || TUNTAP_Close( pPTPBLK->fd ) == 0 );
    pPTPBLK->fd = -1;

    // Release the TUN read buffer.
    free( pTunBuf );
    pTunBuf = NULL;
    iTunLen = 0;

    return NULL;
}   /* End function  ptp_read_thread() */


/* ------------------------------------------------------------------ */
/* add_buffer_to_chain_and_signal_event(): Add PTPHDR to end of chn.  */
/* ------------------------------------------------------------------ */

void*    add_buffer_to_chain_and_signal_event( PTPATH* pPTPATH, PTPHDR* pPTPHDR )
{
    PTPBLK*  pPTPBLK = pPTPATH->pPTPBLK;               // PTPBLK

    // Prepare PTPHDR for adding to chain.
    if (!pPTPHDR)                                      // Any PTPHDR been passed?
        return NULL;
    pPTPHDR->pNextPTPHDR = NULL;                       // Clear the pointer to next PTPHDR

    // Obtain the path chain lock.
    obtain_lock( &pPTPATH->ChainLock );

    // Add PTPHDR to end of chain.
    if (pPTPATH->pFirstPTPHDR)                         // if there are already PTPHDRs
    {
        pPTPATH->pLastPTPHDR->pNextPTPHDR = pPTPHDR;   // Add the PTPHDR to
        pPTPATH->pLastPTPHDR = pPTPHDR;                // the end of the chain
        pPTPATH->iNumPTPHDR++;                         // Increment number of PTPHDRs
    }
    else
    {
        pPTPATH->pFirstPTPHDR = pPTPHDR;               // Make the PTPHDR
        pPTPATH->pLastPTPHDR = pPTPHDR;                // the only PTPHDR
        pPTPATH->iNumPTPHDR = 1;                       // on the chain
    }

    // Release the path chain lock.
    release_lock( &pPTPATH->ChainLock );

    //
    obtain_lock( &pPTPBLK->ReadEventLock );
    signal_condition( &pPTPBLK->ReadEvent );
    release_lock( &pPTPBLK->ReadEventLock );

    return NULL;
}

/* ------------------------------------------------------------------ */
/* add_buffer_to_chain(): Add PTPHDR to end of chain.                 */
/* ------------------------------------------------------------------ */

void*    add_buffer_to_chain( PTPATH* pPTPATH, PTPHDR* pPTPHDR )
{

    // Prepare PTPHDR for adding to chain.
    if (!pPTPHDR)                                      // Any PTPHDR been passed?
        return NULL;
    pPTPHDR->pNextPTPHDR = NULL;                       // Clear the pointer to next PTPHDR

    // Obtain the path chain lock.
    obtain_lock( &pPTPATH->ChainLock );

    // Add PTPHDR to end of chain.
    if (pPTPATH->pFirstPTPHDR)                         // if there are already PTPHDRs
    {
        pPTPATH->pLastPTPHDR->pNextPTPHDR = pPTPHDR;   // Add the PTPHDR to
        pPTPATH->pLastPTPHDR = pPTPHDR;                // the end of the chain
        pPTPATH->iNumPTPHDR++;                         // Increment number of PTPHDRs
    }
    else
    {
        pPTPATH->pFirstPTPHDR = pPTPHDR;               // Make the PTPHDR
        pPTPATH->pLastPTPHDR = pPTPHDR;                // the only PTPHDR
        pPTPATH->iNumPTPHDR = 1;                       // on the chain
    }

    // Release the path chain lock.
    release_lock( &pPTPATH->ChainLock );

    return NULL;
}

/* ------------------------------------------------------------------ */
/* remove_buffer_from_chain(): Remove PTPHDR from start of chain.     */
/* ------------------------------------------------------------------ */

PTPHDR*  remove_buffer_from_chain( PTPATH* pPTPATH )
{

    PTPHDR*    pPTPHDR;                                // PTPHDR

    // Obtain the path chain lock.
    obtain_lock( &pPTPATH->ChainLock );

    // Point to first PTPHDR on the chain.
    pPTPHDR = pPTPATH->pFirstPTPHDR;                   // Pointer to first PTPHDR

    // Remove the first PTPHDR from the chain, if there is one...
    if (pPTPHDR)                                       // If there is a PTPHDR
    {
        pPTPATH->pFirstPTPHDR = pPTPHDR->pNextPTPHDR;  // Make the next the first PTPHDR
        pPTPATH->iNumPTPHDR--;                         // Decrement number of PTPHDRs
        pPTPHDR->pNextPTPHDR = NULL;                   // Clear the pointer to next PTPHDR
        if (!pPTPATH->pFirstPTPHDR)                    // if there are no more PTPHDRs
        {
//          pPTPATH->pFirstPTPHDR = NULL;              // Clear
            pPTPATH->pLastPTPHDR = NULL;               // the chain
            pPTPATH->iNumPTPHDR = 0;                   // pointers and count
        }
    }

    // Release the path chain lock.
    release_lock( &pPTPATH->ChainLock );

    return pPTPHDR;
}

/* ------------------------------------------------------------------ */
/* remove_and_free_any_buffers_on_chain(): Remove and free PTPHDRs.   */
/* ------------------------------------------------------------------ */

void*    remove_and_free_any_buffers_on_chain( PTPATH* pPTPATH )
{

    PTPHDR*    pPTPHDR;                                // PTPHDR

    // Obtain the path chain lock.
    obtain_lock( &pPTPATH->ChainLock );

    // Remove and free the first PTPHDR on the chain, if there is one...
    while( pPTPATH->pFirstPTPHDR != NULL )
    {
        pPTPHDR = pPTPATH->pFirstPTPHDR;               // Pointer to first PTPHDR
        pPTPATH->pFirstPTPHDR = pPTPHDR->pNextPTPHDR;  // Make the next the first PTPHDR
        free( pPTPHDR );                               // Free the message buffer
    }

    // Reset the chain pointers.
    pPTPATH->pFirstPTPHDR = NULL;                      // Clear
    pPTPATH->pLastPTPHDR = NULL;                       // the chain
    pPTPATH->iNumPTPHDR = 0;                           // pointers and count

    // Release the path chain lock.
    release_lock( &pPTPATH->ChainLock );

    return NULL;

}


/* ------------------------------------------------------------------ */
/* alloc_ptp_buffer(): Allocate storage for a PTPHDR and data         */
/* ------------------------------------------------------------------ */

PTPHDR*  alloc_ptp_buffer( DEVBLK* pDEVBLK, int iSize )
{

    PTPHDR*    pPTPHDR;                // PTPHDR
    int        iBufLen;                // Buffer length
    char       etext[40];              // malloc error text


    // Allocate the buffer.
    iBufLen = SIZE_HDR + iSize;
    pPTPHDR = malloc( iBufLen );       // Allocate the buffer
    if (!pPTPHDR)                      // if the allocate was not successful...
    {
        // Report the bad news.
        MSGBUF( etext, "malloc(%n)", &iBufLen );
        // HHC00900 "%1d:%04X %s: error in function %s: %s"
        WRMSG(HHC00900, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             etext, strerror(errno) );
        return NULL;
    }

    // Clear the buffer.
    memset( pPTPHDR, 0, iBufLen );
    pPTPHDR->iAreaLen = iSize;

    return pPTPHDR;
}


/* ------------------------------------------------------------------ */
/* alloc_storage(): Allocate storage                                  */
/* ------------------------------------------------------------------ */

void*  alloc_storage( DEVBLK* pDEVBLK, int iSize )
{

    void*      pStorPtr;               // Storage pointer
    int        iStorLen;               // Storage length
    char       etext[40];              // malloc error text


    // Allocate the storage.
    iStorLen = iSize;
    pStorPtr = malloc( iStorLen );     // Allocate the storage
    if (!pStorPtr)                     // if the allocate was not successful...
    {
        // Report the bad news.
        MSGBUF( etext, "malloc(%n)", &iStorLen );
        // HHC00900 "%1d:%04X %s: error in function %s: %s"
        WRMSG(HHC00900, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             etext, strerror(errno) );
        return NULL;
    }

    // Clear the storage.
    memset( pStorPtr, 0, iStorLen );

    return pStorPtr;
}


/* ------------------------------------------------------------------ */
/* parse_conf_stmt()                                                  */
/* ------------------------------------------------------------------ */

int  parse_conf_stmt( DEVBLK* pDEVBLK, PTPBLK* pPTPBLK,
                      int argc, char** argx )
{
    MAC             mac;               /* Work area for MAC address */
    struct in_addr  addr4;             /* Work area for IPv4 addresses */
#if defined(ENABLE_IPV6)
    struct in6_addr addr6;             /* Work area for IPv6 addresses */
#endif /* defined(ENABLE_IPV6) */
    int             iPfxSiz;           /* Work area for prefix size */
//  int             iMaxBfru;
    int             iMTU;
    int             iDebugMask;
    char            *cphost, *cpprfx;
    size_t          ilhost, ilprfx;
    uint32_t        mask;
    int             iWantFamily;
    int             iFirstFamily[2];
    int             j;
    int             rc;
#if defined(OPTION_W32_CTCI)
    int             iKernBuff;
    int             iIOBuff;
#endif /* defined(OPTION_W32_CTCI) */
    HRB             hrb;
    char            *argn[MAX_ARGS];
    char            **argv = argn;
    int             saw_if = 0;        /* -x (or --if) specified */
    int             saw_conf = 0;      /* Other configuration flags present */


    // Build a copy of the argv list.
    // getopt() and getopt_long() expect argv[0] to be a program name.
    // We need to shift the arguments and insert a dummy argv[0].
    if (argc > (MAX_ARGS-1))
        argc = (MAX_ARGS-1);
    for( j = 0; j < argc; j++ )
        argn[j+1] = argx[j];
    argc++;
    argn[0] = pDEVBLK->typname;

//  // Display the copied argv.
//  {
//      char    tmp[256];
//      int     i;
//      snprintf( (char*)tmp, 256, "Number of arguments: %d", argc );
//      WRMSG(HHC03991, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, tmp );
//      for( i = 0; i < argc; i++ )
//      {
//          snprintf( (char*)tmp, 256, "argv[%d]: %s", i, argv[i] );
//          WRMSG(HHC03991, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, tmp );
//      }
//  }

    // Housekeeping
    memset( &addr4, 0, sizeof( struct in_addr  ));
    memset( &mac,   0, sizeof(      MAC        ));
#if defined( ENABLE_IPV6 )
    memset( &addr6, 0, sizeof( struct in6_addr ));
#endif

    // Set some initial defaults
    STRLCPY( pPTPBLK->szTUNCharDevName, DEF_NETDEV );
#if defined( OPTION_W32_CTCI )
    pPTPBLK->iKernBuff = DEF_CAPTURE_BUFFSIZE;
    pPTPBLK->iIOBuff   = DEF_PACKET_BUFFSIZE;
#endif
#if defined( ENABLE_IPV6 )
    pPTPBLK->iAFamily = AF_UNSPEC;
#else
    pPTPBLK->iAFamily = AF_INET;
#endif
    STRLCPY( pPTPBLK->szMaxBfru, "5" );
    pPTPBLK->iMaxBfru = 5;
    STRLCPY( pPTPBLK->szMTU, "1500" );
    pPTPBLK->iMTU = 1500;
    STRLCPY( pPTPBLK->szDrivePfxSiz4, "32" );
    STRLCPY( pPTPBLK->szNetMask, "255.255.255.255" );
#if defined( ENABLE_IPV6 )
    STRLCPY( pPTPBLK->szDrivePfxSiz6, "128" );
    STRLCPY( pPTPBLK->szDriveLLxSiz6, "64" );
#endif

    // Initialize getopt's counter. This is necessary in the case
    // that getopt was used previously for another device.
    OPTRESET();
    optind = 0;

    // Parse any optional arguments
    while( 1 )
    {
        int     c;

#if defined(OPTION_W32_CTCI)
  #define  PTP_OPTSTRING  "n:t:d::46m:k:i:"
#else /* defined(OPTION_W32_CTCI) */
  #define  PTP_OPTSTRING  "n:x:t:d::46"
#endif /* defined(OPTION_W32_CTCI) */

#if defined(HAVE_GETOPT_LONG)
        int     iOpt;

        static struct option options[] =
        {
            { "dev",     required_argument, NULL, 'n' },
#if !defined(OPTION_W32_CTCI)
            { "if",      required_argument, NULL, 'x' },
#endif /* !defined(OPTION_W32_CTCI) */
            { "mtu",     required_argument, NULL, 't' },
            { "debug",   optional_argument, NULL, 'd' },
            { "inet",    no_argument,       NULL, '4' },
            { "inet6",   no_argument,       NULL, '6' },
#if defined(OPTION_W32_CTCI)
            { "mac",     required_argument, NULL, 'm' },
            { "kbuff",   required_argument, NULL, 'k' },
            { "ibuff",   required_argument, NULL, 'i' },
#endif /* defined(OPTION_W32_CTCI) */
            { NULL,      0,                 NULL,  0  }
        };

        c = getopt_long( argc, argv, PTP_OPTSTRING, options, &iOpt );
#else /* defined(HAVE_GETOPT_LONG) */
        c = getopt( argc, argv, PTP_OPTSTRING );
#endif /* defined(HAVE_GETOPT_LONG) */

        if (c == -1 ) // No more options found
            break;

        switch( c )
        {

        case 'n':     // Network Device

#if defined( OPTION_W32_CTCI )

            // This could be the IP or MAC address of the
            // host ethernet adapter.
            if (inet_aton( optarg, &addr4 ) == 0)
            {
                // Not an IP address, check for valid MAC
                if (ParseMAC( optarg, mac ) != 0)
                {
                    // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                    WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                          "adapter address", optarg );
                    return -1;
                }
            }
#endif
            // This is the file name of the special TUN/TAP character device
            if (strlen( optarg ) > sizeof( pPTPBLK->szTUNCharDevName )-1)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, pDEVBLK->typname, "device name", optarg );
                return -1;
            }

            STRLCPY( pPTPBLK->szTUNCharDevName, optarg );
            break;

#if !defined( OPTION_W32_CTCI )

        case 'x':     // TUN network interface name

            if (strlen( optarg ) > sizeof( pPTPBLK->szTUNIfName )-1)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, pDEVBLK->typname, "TUN device name", optarg );
                return -1;
            }

            STRLCPY( pPTPBLK->szTUNIfName, optarg );
            saw_if = 1;
            break;

#endif

        case 't':     // MTU of link (ignored if Windows) (default 1500).

            // Note: The largest MTU supported by MPCPTP or MPCPTP6
            // devices is 59392, based on the MAXBFRU specified in the
            // TRLE definition. The smallest MTU supported is equal to
            // DEFAULTSIZE, which is 576 for IPv4 and 1280 for IPv6. See
            // the manual 'z/OS Communication Server: IP Configuration
            // Reference'.
            //   MAXBFRU value  Actual MTU value
            //   -------------  ----------------------------------
            //       5            14336  ( ( 4 * 4096 ) - 2048 )
            //      16            59392  ( ( 15 * 4096 ) - 2048 )
            // This side will report a MAXBFRU value of 5 to the y-side,
            // from which the y-side will calculate an actual MTU value
            // of 14336. The MTU value that will be used for packets
            // sent from the y-side to this side will depend on the MTU
            // value specified on the y-side's route statement(s). If the
            // y-side's route statement(s) pecify an MTU value greater
            // than the actual MTU value the route statement value is
            // ignored and the actual MTU value is used. If the y-side's
            // route statement(s) specify an MTU value less than or equal
            // to the actual MTU value the route statement MTU value is
            // used. Hopefully the y-side's route statement(s) will match
            // the MTU value specified here!

            iMTU = atoi( optarg );
            if (iMTU < 576 || iMTU > 14336 ||
                strlen(optarg) > sizeof(pPTPBLK->szMTU)-1)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                      "MTU size", optarg );
                return -1;
            }
            STRLCPY( pPTPBLK->szMTU, optarg );
            pPTPBLK->iMTU = iMTU;
            saw_conf = 1;
            break;

//      case ' ':     // Maximum buffers to use (default 5).
//
//          // The number of 4K pages used by VTAM to receive data. The
//          // resulting buffer size is number_of_pages multiplied by
//          // 4096, minus 4 bytes for an eye-catcher of 'WrHP'. VTAM
//          // automatically substitues a value of 16 for any coded
//          // value higher than 16 without issuing a warning message.
//          // Note: Empirical evidence suggests that VTAM ignores any
//          // coded value lower than 5 and substitutes a value of 5
//          // (the default value) without issuing a warning message. Is
//          // this a bug, or a feature?
//
//          iMaxBfru = atoi( optarg );
//
//          if ( strlen(optarg) > sizeof(pPTPBLK->szMaxBfru)-1 )
//          {
//              // HHC00916 "%1d:%04X %s: option %s value %s invalid"
//              WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
//                    "MaxBfru number", optarg );
//              return -1;
//          }
//
//          if (iMaxBfru < 5)
//          {
//              STRLCPY( pPTPBLK->szMaxBfru, "5" );
//              pPTPBLK->iMaxBfru = 5;
//          }
//          else if (iMaxBfru > 16)
//          {
//              STRLCPY( pPTPBLK->szMaxBfru, "16" );
//              pPTPBLK->iMaxBfru = 16;
//          }
//          else
//          {
//              STRLCPY( pPTPBLK->szMaxBfru, optarg );
//              pPTPBLK->iMaxBfru = iMaxBfru;
//          }
//
//          break;

        case 'd':     // Diagnostics

            if (optarg)
            {
                iDebugMask = atoi( optarg );
                if (iDebugMask < 1 || iDebugMask > 255)
                {
                    // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                    WRMSG( HHC00916, "W", SSID_TO_LCSS( pDEVBLK->ssid ),
                        pDEVBLK->devnum, pDEVBLK->typname, "debug mask", optarg );
                    iDebugMask = DBGPTPPACKET;
                }
                pPTPBLK->uDebugMask = iDebugMask;
            }
            else
            {
                pPTPBLK->uDebugMask = DBGPTPPACKET;
            }
            break;

        case '4':     // Address family.

#if defined( ENABLE_IPV6 )
            pPTPBLK->iAFamily = AF_INET;
#endif
            break;

        case '6':     // Address family.

#if defined( ENABLE_IPV6 )
            pPTPBLK->iAFamily = AF_INET6;
#endif
            break;

#if defined( OPTION_W32_CTCI )

        case 'm':

            if (0
                || strlen(optarg) > sizeof(pPTPBLK->szMACAddress)-1
                || ParseMAC( optarg, mac ) != 0 // (invalid format)
                || !(mac[0] & 0x02)             // (locally assigned MAC bit not ON)
                ||  (mac[0] & 0x01)             // (broadcast bit is ON)
            )
            {
                // "%1d:%04X %s: Option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, pDEVBLK->typname, "MAC address", optarg );
                return -1;
            }

            STRLCPY( pPTPBLK->szMACAddress, optarg );
            break;

        case 'k':     // Kernel Buffer Size (Windows only)

            iKernBuff = atoi( optarg );

            if (iKernBuff * 1024 < MIN_CAPTURE_BUFFSIZE ||
                iKernBuff * 1024 > MAX_CAPTURE_BUFFSIZE)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, pDEVBLK->typname, "kernel buffer size", optarg );
                return -1;
            }

            pPTPBLK->iKernBuff = iKernBuff * 1024;
            break;

        case 'i':     // I/O Buffer Size (Windows only)

            iIOBuff = atoi( optarg );

            if (iIOBuff * 1024 < MIN_PACKET_BUFFSIZE ||
                iIOBuff * 1024 > MAX_PACKET_BUFFSIZE)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG( HHC00916, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                    pDEVBLK->devnum, pDEVBLK->typname, "dll i/o buffer size", optarg );
                return -1;
            }

            pPTPBLK->iIOBuff = iIOBuff * 1024;
            break;

#endif /* defined(OPTION_W32_CTCI) */

        default:  /*  Note: the variable c has a value that
                      makes default: equivalent to case '?':  */

            // HHC00918 "%1d:%04X %s: option %s unknown or specified incorrectly"
            WRMSG( HHC00918, "E", SSID_TO_LCSS( pDEVBLK->ssid ),
                pDEVBLK->devnum, pDEVBLK->typname, argv[optind-1]);
            return -1;
        }
    }

    // Shift past any options
    argc -= optind;
    argv += optind;

    // Check for correct number of arguments.
    // For *nix, there can be either:-
    // a) Two parameters (a pair of IPv4 or IPv6 addresses), or four
    //    parameters (a pair of IPv4 addresses and a pair of IPv6
    //    addresses). If the -x option has not been specified, PTP
    //    will use a TUN interface whose name is allocated by the
    //    kernel (e.g. tun0), that is configured by PTP. If the -x
    //    option has been specified, PTP will use a pre-named TUN
    //    interface. The TUN interface may have been created before
    //    PTP was started, or it may be created by PTP, but in either
    //    case the TUN interface is configured by PTP.
    // b) One parameter when the -x option has not been specified.
    //    The single parameter specifies the name of a pre-configured
    //    TUN inferface that PTP will use.
    // c) Zero parameters when the -x option has been specified. The
    //    The -x option specified the name of a pre-configured TUN
    //    inferface that PTP will use..
    // For Windows there can be:-
    // a) Two parameters (a pair of IPv4 or IPv6 addresses), or four
    //    parameters (a pair of IPv4 addresses and a pair of IPv6
    //    addresses).
//  {
//      char    tmp[256];
//      snprintf( (char*)tmp, 256, "argc %d  saw_if %d  saw_conf %d", argc, saw_if, saw_conf );
//      WRMSG( HHC03991, "I", SSID_TO_LCSS( pDEVBLK->ssid ),
//          pDEVBLK->devnum, pDEVBLK->typname, tmp );
//  }
    if (argc == 2
#if defined( ENABLE_IPV6 )
        || argc == 4
#endif
    ) /* Not pre-configured, but possibly pre-named */
    {
        pPTPBLK->fPreconfigured = FALSE;
    }

#if !defined( OPTION_W32_CTCI )

    else if (argc == 1 && !saw_if && !saw_conf) /* Pre-configured using name */
    {
        if (strlen( argv[0] ) > sizeof(pPTPBLK->szTUNIfName)-1)
        {
            // HHC00916 "%1d:%04X %s: option %s value %s invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                  "TUN device name", argv[0] );
            return -1;
        }
        STRLCPY( pPTPBLK->szTUNIfName, argv[0] );
        argc--; argv++;
        pPTPBLK->fPreconfigured = TRUE;
    }
    else if (argc == 0 && saw_if && !saw_conf) /* Pre-configured using -x option */
    {
        pPTPBLK->fPreconfigured = TRUE;
    }

#endif /* !defined(OPTION_W32_CTCI) */

    else
    {
        // HHC00915 "%1d:%04X %s: incorrect number of parameters"
        WRMSG(HHC00915, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname );
        return -1;
    }

#if defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )

    if (pPTPBLK->fPreconfigured == TRUE)
    {
        /*  Need to append the interface number to the
         *  character device name to open the requested interface.
         */
        char* s = pPTPBLK->szTUNIfName + strlen( pPTPBLK->szTUNIfName );

        while (isdigit( (unsigned char)s[-1] ))
            s--;

        STRLCAT( pPTPBLK->szTUNCharDevName, s );
    }
#endif // defined( __APPLE__ ) || defined( FREEBSD_OR_NETBSD )

    //
    iWantFamily = pPTPBLK->iAFamily;
    iFirstFamily[0] = AF_UNSPEC;
    iFirstFamily[1] = AF_UNSPEC;
    j = 0;

    // Process the remaining parameters.
    while (argc > 0)
    {
        // Guest IPv4 address.
        //  e.g. 192.168.1.1

        // Guest IPv6 address.
        //  e.g. 2001:db8:3003:1::543:210f

        cphost = *argv;                      // point to host name/IP address
        ilhost = strlen( *argv );            // calculate size of name/address (assume no prefix size)
        cpprfx = strchr( *argv, '/' );       // Point to slash character
        ilprfx = 0;                          // no prefix size
        if (cpprfx)                          // If there is a slash
        {
            ilhost = cpprfx - cphost;        // calculate length of name/address
            cpprfx++;                        // point to prefix size
            ilprfx = strlen( cpprfx );       // calculate length of prefix size
        }
        if (ilhost > (size_t)(sizeof(hrb.host)-1))
        {
            // HHC00916 "%1d:%04X %s: option %s value %s invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                  "IP address", *argv );
            return -1;
        }

        // Check whether a numeric IPv4 address has been specified.
        memset( &hrb, 0, sizeof(hrb) );
        hrb.wantafam = AF_INET;
        hrb.numeric = TRUE;
        memcpy( hrb.host, cphost, ilhost );
        rc = resolve_host( &hrb);
        if (rc == 0)
        {
            // OK, a numeric IPv4 address has been specified.
            iFirstFamily[j] = AF_INET;
            STRLCPY( pPTPBLK->szGuestIPAddr4, hrb.ipaddr );
            memcpy( &pPTPBLK->iaGuestIPAddr4, &hrb.sa.in.sin_addr, sizeof(pPTPBLK->iaGuestIPAddr4) );
        }
        else
        {
            // A numeric IPv4 address has not been specified.
#if defined(ENABLE_IPV6)
            // Check whether a numeric IPv6 address has been specified.
            memset( &hrb, 0, sizeof(hrb) );
            hrb.wantafam = AF_INET6;
            hrb.numeric = TRUE;
            if (cphost[0] == '[' && cphost[ilhost-1] == ']')
            {
                memcpy( hrb.host, cphost+1, ilhost-2 );
            }
            else
            {
                memcpy( hrb.host, cphost, ilhost );
            }
            rc = resolve_host( &hrb);
            if (rc == 0)
            {
                // OK, a numeric IPv6 address has been specified.
                iFirstFamily[j] = AF_INET6;
                STRLCPY( pPTPBLK->szGuestIPAddr6, hrb.ipaddr );
                memcpy( &pPTPBLK->iaGuestIPAddr6, &hrb.sa.in6.sin6_addr, sizeof(pPTPBLK->iaGuestIPAddr6) );
            }
            else
            {
                // A numeric IPv6 address has not been specified.
#endif /* defined(ENABLE_IPV6) */
                // Check whether a host name that resolves to the required
                // address family has been specified.
                memset( &hrb, 0, sizeof(hrb) );
                hrb.wantafam = iWantFamily;
                memcpy( hrb.host, cphost, ilhost );
                rc = resolve_host( &hrb);
                if (rc == 0)
                {
                    // OK, a host name that resolves to the required address
                    // family has been specified. If no family was specified
                    // (the -4/-6 options) whichever family was first in the
                    // resolve result is being used.
                    iFirstFamily[j] = hrb.afam;
                    if (iFirstFamily[j] == AF_INET)
                    {
                        STRLCPY( pPTPBLK->szGuestIPAddr4, hrb.ipaddr );
                        memcpy( &pPTPBLK->iaGuestIPAddr4, &hrb.sa.in.sin_addr, sizeof(pPTPBLK->iaGuestIPAddr4) );
                    }
#if defined(ENABLE_IPV6)
                    else
                    {
                        STRLCPY( pPTPBLK->szGuestIPAddr6, hrb.ipaddr );
                        memcpy( &pPTPBLK->iaGuestIPAddr6, &hrb.sa.in6.sin6_addr, sizeof(pPTPBLK->iaGuestIPAddr6) );
                    }
#endif /* defined(ENABLE_IPV6) */
                }
                else
                {
                    // Something that isn't very useful has been specifed..
                    // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                    WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                         "IP address", *argv );
                    return -1;
                }
#if defined(ENABLE_IPV6)
            }
#endif /* defined(ENABLE_IPV6) */
        }

        if (cpprfx)
        {
            if (iFirstFamily[j] == AF_INET)
            {
                // Hmm... the Guest IPv4 address was specified with a prefix size.
                {
                  char    tmp[256];
                  MSGBUF( tmp, "Prefix size specification moved from guest to drive" );
                  WRMSG(HHC03991, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, tmp );
                }
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                     "IP address", *argv );
                return -1;
            }
#if defined(ENABLE_IPV6)
            else
            {
                // Hmm... the Guest IPv6 address was specified with a prefix size.
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                     "IP address", *argv );
                return -1;
            }
#endif /* defined(ENABLE_IPV6) */
        }

        argc--; argv++;

        // Driver IPv4 address and prefix size in CIDR notation.
        //  e.g. 192.168.1.1/24
        // If the prefix size is not specified a value of 32 is assumed,
        // which is equivalent to a netmask of 255.255.255.255. If the
        // prefix size is specified it can have a value from 0 to 32.
        // The value is used to produce the equivalent netmask. For example,
        // a value of 0 will produce a netmask of 0.0.0.0, while a value of
        // 26 will produce a netmask of 255.255.255.192.

        // Driver IPv6 address and prefix size in CIDR notation.
        //  e.g. 2001:db8:3003:1::543:210f/48
        // If the prefix size is not specified a value of 128 is
        // assumed. If the prefix size is specified it can have a
        // value from 0 to 128.

        cphost = *argv;                      // point to host name/IP address
        ilhost = strlen( *argv );            // calculate size of name/address (assume no prefix)
        cpprfx = strchr( *argv, '/' );       // Point to slash character
        ilprfx = 0;                          // no prefix size
        if (cpprfx)                          // If there is a slash
        {
            ilhost = cpprfx - cphost;        // calculate length of name/address
            cpprfx++;                        // point to prefix size
            ilprfx = strlen( cpprfx );       // calculate length of prefix size
        }
        if (ilhost > (size_t)(sizeof(hrb.host)-1))
        {
            // HHC00916 "%1d:%04X %s: option %s value %s invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                  "IP address", *argv );
            return -1;
        }

        memset( &hrb, 0, sizeof(hrb) );
        hrb.wantafam = iFirstFamily[j];
#if defined(ENABLE_IPV6)
        if (iFirstFamily[j] == AF_INET6 &&
            cphost[0] == '[' && cphost[ilhost-1] == ']')
        {
            hrb.numeric = TRUE;
            memcpy( hrb.host, cphost+1, ilhost-2 );
        }
        else
        {
#endif /* defined(ENABLE_IPV6) */
            memcpy( hrb.host, cphost, ilhost );
#if defined(ENABLE_IPV6)
        }
#endif /* defined(ENABLE_IPV6) */
        rc = resolve_host( &hrb);
        if (rc != 0)
        {
            // HHC00916 "%1d:%04X %s: option %s value %s invalid"
            WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                  "IP address", *argv );
            return -1;
        }

        if (iFirstFamily[j] == AF_INET)
        {
            STRLCPY( pPTPBLK->szDriveIPAddr4, hrb.ipaddr );
            memcpy( &pPTPBLK->iaDriveIPAddr4, &hrb.sa.in.sin_addr, sizeof(pPTPBLK->iaDriveIPAddr4) );
        }
#if defined(ENABLE_IPV6)
        else
        {
            STRLCPY( pPTPBLK->szDriveIPAddr6, hrb.ipaddr );
            memcpy( &pPTPBLK->iaDriveIPAddr6, &hrb.sa.in6.sin6_addr, sizeof(pPTPBLK->iaDriveIPAddr6) );
        }
#endif /* defined(ENABLE_IPV6) */

        if (cpprfx)
        {
            if (iFirstFamily[j] == AF_INET)
            {
                if (ilprfx > (size_t)sizeof(pPTPBLK->szDrivePfxSiz4)-1)
                {
                    // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                    WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                          "prefix size", *argv );
                    return -1;
                }
                iPfxSiz = atoi( cpprfx );
                if (( iPfxSiz < 0 ) || ( iPfxSiz > 32 ))
                {
                    // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                    WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                          "prefix size", *argv );
                    return -1;
                }

                STRLCPY( pPTPBLK->szDrivePfxSiz4, cpprfx );

                switch( iPfxSiz )
                {
                case 0:
                    mask = 0x00000000;
                    break;
                case 32:
                    mask = 0xFFFFFFFF;
                    break;
                default:
                    mask = 0xFFFFFFFF ^ ( 0xFFFFFFFF >> iPfxSiz );
                    break;
                }
                addr4.s_addr = htonl(mask);

                hinet_ntop( AF_INET, &addr4, pPTPBLK->szNetMask,
                                             sizeof(pPTPBLK->szNetMask) );
            }
#if defined(ENABLE_IPV6)
            else
            {
                if (ilprfx > (size_t)sizeof(pPTPBLK->szDrivePfxSiz6)-1)
                {
                    // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                    WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                          "prefix size", *argv );
                    return -1;
                }
                iPfxSiz = atoi( cpprfx );
                if (( iPfxSiz < 0 ) || ( iPfxSiz > 128 ))
                {
                    // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                    WRMSG(HHC00916, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                          "prefix size", *argv );
                    return -1;
                }
                STRLCPY( pPTPBLK->szDrivePfxSiz6, cpprfx );
            }
#endif /* defined(ENABLE_IPV6) */
        }

        argc--; argv++;

        // Remember whether IPv4 or IPv6 addresses were specified.
        if (iFirstFamily[j] == AF_INET)
            pPTPBLK->fIPv4Spec = TRUE;
#if defined(ENABLE_IPV6)
        else
            pPTPBLK->fIPv6Spec = TRUE;
#endif /* defined(ENABLE_IPV6) */

        // Decide what address family the next pair of IP addresses should be.
#if defined(ENABLE_IPV6)
        if (iWantFamily == AF_INET)
            iWantFamily = AF_INET6;
        else if (iWantFamily == AF_INET6)
            iWantFamily = AF_INET;
        else
        {
            if (iFirstFamily[j] == AF_INET)
                iWantFamily = AF_INET6;
            else
#endif /* defined(ENABLE_IPV6) */
                iWantFamily = AF_INET;
#if defined(ENABLE_IPV6)
        }
#endif /* defined(ENABLE_IPV6) */

        j++;
    }   /* while( argc > 0 ) */

    // Good, the configuration statement had no obvious errors.
    if (pPTPBLK->fPreconfigured)
        rc = get_preconfigured_value(pDEVBLK, pPTPBLK);
    else
        rc = check_specified_value(pDEVBLK, pPTPBLK);
    if (rc != 0)
        return -1;

#if defined(OPTION_W32_CTCI)
    // If the MAC address was not specified in the configuration
    // statement, create a MAC address using pseudo-random numbers.
    if (!pPTPBLK->szMACAddress[0])
    {
        for( j = 0; j < 6; j++ )
            mac[j] = (int)((rand()/(RAND_MAX+1.0))*256);
        mac[0] &= 0xFE;  /* Clear multicast bit. */
        mac[0] |= 0x02;  /* Set local assignment bit. */

        MSGBUF
        (
            pPTPBLK->szMACAddress,
            "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
        );
    }
#endif /* defined(OPTION_W32_CTCI) */

    // That's all folks.
    return 0;
}   /* End function  parse_conf_stmt() */

/* ------------------------------------------------------------------ */
/* get_preconfigured_value()                                          */
/* ------------------------------------------------------------------ */
int  get_preconfigured_value( DEVBLK* pDEVBLK, PTPBLK* pPTPBLK )
{

#if defined(OPTION_W32_CTCI)
    // HHC03965 "%id:%04X %s; Preconfigured interface %s does not exist or is not accessible by Hercules"
    WRMSG(HHC03965, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                         pPTPBLK->szTUNIfName);
    return -1;
#else /* defined(OPTION_W32_CTCI) */
/* Extract from Linux getifaddrs/freeifaddrs man page */
//
//    struct ifaddrs {
//         struct ifaddrs  *ifa_next;    /* Next item in list */
//         char            *ifa_name;    /* Name of interface */
//         unsigned int     ifa_flags;   /* Flags from SIOCGIFFLAGS */
//         struct sockaddr *ifa_addr;    /* Address of interface */
//         struct sockaddr *ifa_netmask; /* Netmask of interface */
//         union {
//             struct sockaddr *ifu_broadaddr;
//                              /* Broadcast address of interface */
//             struct sockaddr *ifu_dstaddr;
//                              /* Point-to-point destination address */
//         } ifa_ifu;
//     #define              ifa_broadaddr ifa_ifu.ifu_broadaddr
//     #define              ifa_dstaddr   ifa_ifu.ifu_dstaddr
//         void            *ifa_data;    /* Address-specific data */
//     };
//
//   The ifa_next field contains a pointer to the next structure on the list, or
//   NULL if this is the last item of the list.
//
//   The ifa_name points to the null-terminated interface name.
//
//   The ifa_flags field contains the interface flags, as returned by the
//   SIOCGIFFLAGS ioctl(2) operation (see netdevice(7) for a list of these flags).
//
//   The ifa_addr field points to a structure containing the interface address.
//   (The sa_family subfield should be consulted to determine the format of the
//   address structure.)
//
//   The ifa_netmask field points to a structure containing the netmask associated
//   with ifa_addr, if applicable for the address family.
//
//   Depending on whether the bit IFF_BROADCAST or IFF_POINTOPOINT is set in
//   ifa_flags (only one can be set at a time), either ifa_broadaddr will contain
//   the broadcast address associated with ifa_addr (if applicable for the address
//   family) or ifa_dstaddr will contain the destination address of the point-to-
//   point interface.
//
//   The ifa_data field points to a buffer containing address-family-specific data;
//   this field may be NULL if there is no such data for this interface.
//
//   The data returned by getifaddrs() is dynamically allocated and should be freed
//   using freeifaddrs() when no longer needed.
//
//
//   On success, getifaddrs() returns zero; on error, -1 is returned, and errno is
//   set appropriately.
//
//   The addresses returned on Linux will usually be the IPv4 and IPv6 addresses
//   assigned to the interface, but also one AF_PACKET address per interface
//   containing lower-level details about the interface and its physical layer. In
//   this case, the ifa_data field may contain a pointer to a struct
//   net_device_stats, defined in <linux/netdevice.h>, which contains various
//   interface attributes and statistics.
//
    struct ifaddrs      *ifaddr;
    struct ifaddrs      *ifacur;
    int family;
    u_int                have_name = FALSE;
    struct sockaddr_in  *sin;
    struct in_addr       drive4;
    struct in_addr       guest4;
    struct in_addr       mask4;
    u_int                have_drive4 = FALSE;
    u_int                have_guest4 = FALSE;
    u_int                have_mask4 = FALSE;
#if defined(ENABLE_IPV6)
    struct sockaddr_in6 *sin6;
    struct in6_addr      addr6;
    struct in6_addr      mask6;
    struct in6_addr      adll6;
    struct in6_addr      mall6;
    u_int                have_addr6 = FALSE;
    u_int                have_mask6 = FALSE;
    u_int                have_adll6 = FALSE;
    u_int                have_mall6 = FALSE;
    struct in6_addr      work6;
#endif /* defined(ENABLE_IPV6) */
    struct {
      union {
        struct in_addr   ip4;
#if defined(ENABLE_IPV6)
        struct in6_addr  ip6;
#endif /* defined(ENABLE_IPV6) */
        unsigned int     uint[4];
      }                mask;
      unsigned int       bit;
      int                size;
    }                pfx;
    int                  fd, rc, j;
    struct hifr          hifr;


    /* */
    memset( &drive4, 0, sizeof(drive4) );
    memset( &guest4, 0, sizeof(guest4) );
    memset( &mask4, 0, sizeof(mask4) );
#if defined(ENABLE_IPV6)
    memset( &addr6, 0, sizeof(addr6) );
    memset( &mask6, 0, sizeof(mask6) );
    memset( &adll6, 0, sizeof(adll6) );
    memset( &mall6, 0, sizeof(mall6) );
#endif /* defined(ENABLE_IPV6) */

    /* Get the address information for all of the interfaces */
    if (getifaddrs(&ifaddr) == -1) {
        // HHC00900 "%1d:%04X %s: error in function %s: %s"
        WRMSG(HHC00900, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             "getifaddrs", strerror(errno) );
        return -1;
    }

    /* Process the ifaddr structure(s) for the tun */
    /* interface in the chain of ifaddr structures */
    for (ifacur = ifaddr; ifacur != NULL; ifacur = ifacur->ifa_next) {
      if (strcmp(ifacur->ifa_name, pPTPBLK->szTUNIfName) == 0) {
        have_name = TRUE;

        /* Extract info from the ifaddr structure for the tun interface */
        if (ifacur->ifa_addr != NULL) {
          family = ifacur->ifa_addr->sa_family;
          if (family == AF_INET) {
            /* If the tun device was configured with an IP command with the */
            /* form:-                                                       */
            /*   ip -f inet addr add dev tun99 192.168.1.1                  */
            /* the device address and the peer (destination) address will   */
            /* be the identical.                                            */
            /* If the tun device was configured with an IP command with the */
            /* form:-                                                       */
            /*   ip -f inet addr add dev tun99 192.168.1.1 peer 192.168.1.2 */
            /* the device address and the peer (destination) address will   */
            /* be different (obviously!).                                   */
            if (!have_drive4) {
              sin = (struct sockaddr_in*)ifacur->ifa_addr;
              memcpy( &drive4, &sin->sin_addr, sizeof(drive4) );
              have_drive4 = TRUE;
              sin = (struct sockaddr_in*)ifacur->ifa_netmask;
              memcpy( &mask4, &sin->sin_addr, sizeof(mask4) );
              have_mask4 = TRUE;
              if ((ifacur->ifa_flags & IFF_POINTOPOINT) && ifacur->ifa_dstaddr) {
                sin = (struct sockaddr_in*)ifacur->ifa_dstaddr;
                memset( &guest4, 0, sizeof(guest4) );
                if ( (memcmp(&drive4, &sin->sin_addr, sizeof(drive4)) != 0) &&
                     (memcmp(&guest4, &sin->sin_addr, sizeof(guest4)) != 0) ) {
                  memcpy( &guest4, &sin->sin_addr, sizeof(guest4) );
                  have_guest4 = TRUE;
                }
              }
            }
#if defined(ENABLE_IPV6)
          } else if (family == AF_INET6) {
            sin6 = (struct sockaddr_in6*)ifacur->ifa_addr;
            memset( work6.s6_addr, 0, 16 );
            work6.s6_addr[0] = 0xFE;
            work6.s6_addr[1] = 0x80;
            if (memcmp( &sin6->sin6_addr, &work6, 8 ) != 0) {
              if (!have_addr6) {
                sin6 = (struct sockaddr_in6*)ifacur->ifa_addr;
                memcpy( &addr6, &sin6->sin6_addr, sizeof(addr6) );
                have_addr6 = TRUE;
                sin6 = (struct sockaddr_in6*)ifacur->ifa_netmask;
                memcpy( &mask6, &sin6->sin6_addr, sizeof(mask6) );
                have_mask6 = TRUE;
              }
            } else {
              if (!have_adll6) {
                sin6 = (struct sockaddr_in6*)ifacur->ifa_addr;
                memcpy( &adll6, &sin6->sin6_addr, sizeof(adll6) );
                have_adll6 = TRUE;
                sin6 = (struct sockaddr_in6*)ifacur->ifa_netmask;
                memcpy( &mall6, &sin6->sin6_addr, sizeof(mall6) );
                have_mall6 = TRUE;
              }
            }
#endif /* defined(ENABLE_IPV6) */
          }
        } /* End of   if (ifacur->ifa_addr != NULL) */

      } /* End of  if (strcmp(ifa_name, pPTPBLK->szTUNIfName) == 0) */
    } /* End of  for (ifacur = ifaddr; ifacur != NULL; ifacur = ifacur->ifa_next) */

    /* Dispose of all of the returned ifaddrs structures */
    freeifaddrs(ifaddr);

    /* Check whether the interface exists */
    if (!have_name) {
        // HHC03965 "%id:%04X %s; Preconfigured interface %s does not exist or is not accessible by Hercules"
        WRMSG(HHC03965, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                         pPTPBLK->szTUNIfName);
        return -1;
    }

    /* Process the extracted IPv4 addresses and netmask */
    if (have_drive4 && have_mask4) {
        hinet_ntop( AF_INET, &drive4, pPTPBLK->szDriveIPAddr4, sizeof(pPTPBLK->szDriveIPAddr4) );
        memcpy( &pPTPBLK->iaDriveIPAddr4, &drive4, sizeof(pPTPBLK->iaDriveIPAddr4) );
        memcpy( &pfx.mask.ip4, &mask4, sizeof(mask4) );
        pfx.mask.uint[0] = ntohl(pfx.mask.uint[0]);
        pfx.size = 0;
        pfx.bit = 0x80000000;
        while (pfx.bit) {
          if (pfx.mask.uint[0] & pfx.bit) {
            pfx.size++;
          }
          pfx.bit >>= 1;
        }
        snprintf( pPTPBLK->szDrivePfxSiz4, sizeof(pPTPBLK->szDrivePfxSiz4), "%d", pfx.size );
        hinet_ntop( AF_INET, &mask4, pPTPBLK->szNetMask, sizeof(pPTPBLK->szNetMask) );
        if (have_guest4) {
          hinet_ntop( AF_INET, &guest4, pPTPBLK->szGuestIPAddr4, sizeof(pPTPBLK->szGuestIPAddr4) );
          memcpy( &pPTPBLK->iaGuestIPAddr4, &guest4, sizeof(pPTPBLK->iaGuestIPAddr4) );
          pPTPBLK->fPreGuestIPAddr4 = TRUE;
        }
        pPTPBLK->fIPv4Spec = TRUE;
    }
    else if (!have_drive4 && !have_mask4) {
        pPTPBLK->fIPv4Spec = FALSE;
    }
    else {
        // HHC03965 "%id:%04X %s; Preconfigured interface %s does not exist or is not accessible by Hercules"
        WRMSG(HHC03965, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                         pPTPBLK->szTUNIfName);
        return -1;
    }

#if defined(ENABLE_IPV6)
    /* Process the extracted IPv6 addresses and masks */
    if (have_addr6 && have_mask6) {
        /* Setup drive IPv6 addresses */
        hinet_ntop( AF_INET6, &addr6, pPTPBLK->szDriveIPAddr6, sizeof(pPTPBLK->szDriveIPAddr6) );
        memcpy( &pPTPBLK->iaDriveIPAddr6, &addr6, sizeof(pPTPBLK->iaDriveIPAddr6) );
        /* Setup drive IPv6 prefix length */
        memcpy( &pfx.mask.ip6, &mask6, sizeof(mask6) );
        pfx.mask.uint[0] = ntohl(pfx.mask.uint[0]);
        pfx.mask.uint[1] = ntohl(pfx.mask.uint[1]);
        pfx.mask.uint[2] = ntohl(pfx.mask.uint[2]);
        pfx.mask.uint[3] = ntohl(pfx.mask.uint[3]);
        pfx.size = 0;
        for (j = 0; j <= 3; j++) {
          if (pfx.mask.uint[j] == 0x00000000)
            break;
          if (pfx.mask.uint[j] == 0xFFFFFFFF) {
            pfx.size += 32;
          } else {
            pfx.bit = 0x80000000;
            while (pfx.bit) {
              if (pfx.mask.uint[j] & pfx.bit) {
                pfx.size++;
              }
              pfx.bit >>= 1;
            }
          }
        }
        snprintf( pPTPBLK->szDrivePfxSiz6, sizeof(pPTPBLK->szDrivePfxSiz6), "%d", pfx.size );
        if (have_adll6 && have_mall6) {
          /* Setup drive IPv6 link local address */
          hinet_ntop( AF_INET6, &adll6, pPTPBLK->szDriveLLAddr6, sizeof(pPTPBLK->szDriveLLAddr6) );
          memcpy( &pPTPBLK->iaDriveLLAddr6, &adll6, sizeof(pPTPBLK->iaDriveLLAddr6) );
          /* Setup drive IPv6 link local prefix length */
          memcpy( &pfx.mask.ip6, &mall6, sizeof(mall6) );
          pfx.mask.uint[0] = ntohl(pfx.mask.uint[0]);
          pfx.mask.uint[1] = ntohl(pfx.mask.uint[1]);
          pfx.mask.uint[2] = ntohl(pfx.mask.uint[2]);
          pfx.mask.uint[3] = ntohl(pfx.mask.uint[3]);
          pfx.size = 0;
          for (j = 0; j <= 3; j++) {
            if (pfx.mask.uint[j] == 0x00000000)
              break;
            if (pfx.mask.uint[j] == 0xFFFFFFFF) {
              pfx.size += 32;
            } else {
              pfx.bit = 0x80000000;
              while (pfx.bit) {
                if (pfx.mask.uint[j] & pfx.bit) {
                  pfx.size++;
                }
                pfx.bit >>= 1;
              }
            }
          }
          snprintf( pPTPBLK->szDriveLLxSiz6, sizeof(pPTPBLK->szDriveLLxSiz6), "%d", pfx.size );
        } else {
          // Create a Driver Link Local address using pseudo-random numbers.
          addr6.s6_addr[0] = 0xFE;
          addr6.s6_addr[1] = 0x80;
          memset( &addr6.s6_addr[2], 0, 6 );
          for( j = 8; j < 16; j++ )
              addr6.s6_addr[j] = (int)((rand()/(RAND_MAX+1.0))*256);
          hinet_ntop( AF_INET6, &addr6, pPTPBLK->szDriveLLAddr6, sizeof(pPTPBLK->szDriveLLAddr6) );
          memcpy( &pPTPBLK->iaDriveLLAddr6, &addr6, sizeof(pPTPBLK->iaDriveLLAddr6) );
        }
        pPTPBLK->fIPv6Spec = TRUE;
    }
    else if (!have_addr6 && !have_mask6 && !have_adll6 && !have_mall6) {
        pPTPBLK->fIPv6Spec = FALSE;
    }
    else {
        // HHC03965 "%id:%04X %s; Preconfigured interface %s does not exist or is not accessible by Hercules"
        WRMSG(HHC03965, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                         pPTPBLK->szTUNIfName);
        return -1;
    }
#endif /* defined(ENABLE_IPV6) */

    /* Check that either IPv4 or IPv6 addresses were extracted */
    if (!pPTPBLK->fIPv4Spec
#if defined(ENABLE_IPV6)
                       && !pPTPBLK->fIPv6Spec
#endif /* defined(ENABLE_IPV6) */
                                   )
    {
        // HHC03965 "%id:%04X %s; Preconfigured interface %s does not exist or is not accessible by Hercules"
        WRMSG(HHC03965, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                         pPTPBLK->szTUNIfName);
        return -1;
    }

    /* Obtain the MTU value */
    memset( &hifr, 0, sizeof(struct hifr) );
    strncpy( hifr.hifr_name, pPTPBLK->szTUNIfName, sizeof(hifr.hifr_name)-1 );

    fd = socket(AF_INET, SOCK_STREAM, 0);
    rc = TUNTAP_IOCtl( fd, SIOCGIFMTU, (char*)&hifr );
    close(fd);

    if (rc < 0) {
        // HHC00902 "%1d:%04X %s: ioctl '%s' failed for device '%s': '%s'"
        WRMSG(HHC00902, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                             "SIOCGIFMTU", pPTPBLK->szTUNIfName, strerror(errno) );
        return -1;
    }

    pPTPBLK->iMTU = hifr.hifr_mtu;
    snprintf( pPTPBLK->szMTU, sizeof(pPTPBLK->szMTU), "%d", hifr.hifr_mtu );

//  // HHC03953 "%1d:%04X PTP: IPv4: Drive %s/%s (%s): Guest %s"
//  WRMSG(HHC03953, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
//      pPTPBLK->szDriveIPAddr4,
//      pPTPBLK->szDrivePfxSiz4,
//      pPTPBLK->szNetMask,
//      pPTPBLK->szGuestIPAddr4 );
//  // HHC03954 "%1d:%04X PTP: IPv6: Drive %s/%s %s/%s: Guest %s"
//  WRMSG(HHC03954, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
//      pPTPBLK->szDriveLLAddr6,
//      pPTPBLK->szDriveLLxSiz6,
//      pPTPBLK->szDriveIPAddr6,
//      pPTPBLK->szDrivePfxSiz6,
//      pPTPBLK->szGuestIPAddr6 );

//  mpc_display_stuff( pDEVBLK, "sockaddr_in6", (BYTE*)sin6, sizeof(struct sockaddr_in6), ' ' );
//  mpc_display_stuff( pDEVBLK, "work6", (BYTE*)&work6, sizeof(struct in6_addr), ' ' );
//  mpc_display_stuff( pDEVBLK, "sin6_addr", (BYTE*)&sin6->sin6_addr, sizeof(addr6), ' ' );

    /* That's all folks. */
    return 0;
#endif /* defined(OPTION_W32_CTCI) */
}   /* End function  get_preconfigured_value() */

/* ------------------------------------------------------------------ */
/* check_specified_value()                                           */
/* ------------------------------------------------------------------ */
int  check_specified_value( DEVBLK* pDEVBLK, PTPBLK* pPTPBLK )
{
#if defined(ENABLE_IPV6)
    struct in6_addr addr6;             /* Work area for IPv6 addresses */
    int             fd;
    int             j;
#endif /* defined(ENABLE_IPV6) */


    // If IPv4 addresses were specified check that the same IPv4 address
    // has not been specified for the guest and driver.
    if (pPTPBLK->fIPv4Spec &&
        memcmp( &pPTPBLK->iaGuestIPAddr4, &pPTPBLK->iaDriveIPAddr4, 4 ) == 0)
    {
        // HHC03901 "%1d:%04X PTP: Guest and driver IP addresses are the same"
        WRMSG(HHC03901, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum );
        return -1;
    }

#if defined(ENABLE_IPV6)
    // If IPv6 addresses were specified check that the same IPv6 address
    // has not been specified for the guest and driver.
    if (pPTPBLK->fIPv6Spec &&
        memcmp( &pPTPBLK->iaGuestIPAddr6, &pPTPBLK->iaDriveIPAddr6, 16 ) == 0)
    {
        // HHC03901 "%1d:%04X PTP: Guest and driver IP addresses are the same"
        WRMSG(HHC03901, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum );
        return -1;
    }

    // If IPv6 addresses were specified check that IPv6 is supported on
    // this machine.
    if (pPTPBLK->fIPv6Spec)
    {
        fd = socket( AF_INET6, SOCK_DGRAM, 0 );
        if (fd < 0)
        {
            // HHC03902 "%1d:%04X PTP: Inet6 not supported"
            WRMSG(HHC03902, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum );
            return -1;
        }
        else
        {
            close( fd );
        }
    }

    // If IPv6 addresses were specified check that the MTU size is at
    // least the minimum size for an IPv6 link.
    if (pPTPBLK->fIPv6Spec &&
        pPTPBLK->iMTU < 1280)
    {
        // HHC03918 "%1d:%04X PTP: MTU changed from size %d bytes to size %d bytes"
        WRMSG(HHC03918, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pPTPBLK->iMTU, 1280 );
        STRLCPY( pPTPBLK->szMTU, "1280" );
        pPTPBLK->iMTU = 1280;
    }

    // If IPv6 addresses were specified create a Driver Link Local IPv6
    // address using pseudo-random numbers.
    if (pPTPBLK->fIPv6Spec)
    {
        addr6.s6_addr[0] = 0xFE;
        addr6.s6_addr[1] = 0x80;
        memset( &addr6.s6_addr[2], 0, 6 );
        for( j = 8; j < 16; j++ )
            addr6.s6_addr[j] = (int)((rand()/(RAND_MAX+1.0))*256);

        hinet_ntop( AF_INET6, &addr6, pPTPBLK->szDriveLLAddr6, sizeof(pPTPBLK->szDriveLLAddr6) );
        memcpy( &pPTPBLK->iaDriveLLAddr6, &addr6, sizeof(pPTPBLK->iaDriveLLAddr6) );
    }
#endif /* defined(ENABLE_IPV6) */

    // That's all folks.
    return 0;
}   /* End function  check_specified_value() */


/* ------------------------------------------------------------------ */
/* raise_unsol_int()                                                  */
/* ------------------------------------------------------------------ */
// Return value
//    0    Successful
//   -1    No storage available for a PTPINT
//   -2    The create_thread for the ptp_unsol_int_thread failed

int  raise_unsol_int( DEVBLK* pDEVBLK, BYTE bStatus, int iDelay )
{
    PTPATH*    pPTPATH  = pDEVBLK->dev_data;   // PTPATH
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;    // PTPBLK
    PTPINT*    pPTPINT;                        // PTPINT
    TID        tid;                            // ptp_unsol_int_thread thread ID
    char       thread_name[32];                // ptp_unsol_int_thread
    int        rc;                             // Return code


    // Obtain the unsolicited interrupt list lock.
    obtain_lock( &pPTPBLK->UnsolListLock );

    // Obtain a PTPINT from the LIFO linked list.
    pPTPINT = pPTPBLK->pFirstPTPINT;                   // Pointer to first PTPINT
    if (pPTPINT)                                       // If there is a PTPINT
    {
        pPTPBLK->pFirstPTPINT = pPTPINT->pNextPTPINT;  // Make the next the first PTPINT
        pPTPINT->pNextPTPINT = NULL;                   // Clear the pointer to next PTPINT
    }

    // Release the unsolicited interrupt list lock.
    release_lock( &pPTPBLK->UnsolListLock );

    // If we did not obtain a PTPINT from the LIFO linked list
    // then allocate storage for a PTPINT.
    if (!pPTPINT)                                      // If there isn't a PTPINT
    {
        pPTPINT = alloc_storage( pDEVBLK, (int)sizeof(PTPINT) );
        if (!pPTPINT)                                  // If there is no storage
        {
            // Report the bad news.
            // HHC00102 "Error in function create_thread(): %s"
            WRMSG(HHC00102, "E", "No storage available!");
            // Hmm... the interrupt to the y-side will not be raised.
            return -1;
        }
    }

    // Initialize the PTPINT.
    pPTPINT->pDEVBLK = pDEVBLK;
    pPTPINT->bStatus = bStatus;
    pPTPINT->iDelay  = iDelay;

    // Create the unsolicited interrupt thread.
    MSGBUF( thread_name, "%s %4.4X UnsolIntThread",
                         pPTPBLK->pDEVBLKRead->typname,
                         pPTPBLK->pDEVBLKRead->devnum);
    rc = create_thread( &tid, JOINABLE, ptp_unsol_int_thread, pPTPINT, thread_name );
    if (rc)
    {
        // Report the bad news.
        // HHC00102 "Error in function create_thread(): %s"
        WRMSG(HHC00102, "E", strerror(rc));
        // Hmm... the interrupt to the y-side will not be raised.
        return -2;
    }

    // Good, the thread is active.
    return 0;
}   /* End function  raise_unsol_int() */


/* ------------------------------------------------------------------ */
/* ptp_unsol_int_thread()                                             */
/* ------------------------------------------------------------------ */

void*  ptp_unsol_int_thread( void* arg )
{
    PTPINT*    pPTPINT  = (PTPINT*) arg;
    DEVBLK*    pDEVBLK  = pPTPINT->pDEVBLK;    // DEVBLK
    PTPATH*    pPTPATH  = pDEVBLK->dev_data;   // PTPATH
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;    // PTPBLK
    int        rc;
    int        i;
    int        delay_q;
    int        delay_r;
    struct timespec waittime;
    struct timeval  now;


    // Check whether the requestor wants a delay before the interrupt
    // is raised.
    if (pPTPINT->iDelay != 0)
    {
        // Wait for the number of milliseconds specified by the requestor.
        // Calculate when to end the wait.
        delay_q = pPTPINT->iDelay / 1000;
        delay_r = pPTPINT->iDelay % 1000;

        gettimeofday( &now, NULL );

        waittime.tv_sec  = now.tv_sec + delay_q;
        waittime.tv_nsec = (now.tv_usec + (delay_r * 1000)) * 1000;
        if (waittime.tv_nsec >= 1000000000)
        {
            waittime.tv_sec++;
            waittime.tv_nsec -= 1000000000;
        }

        // Obtain the path unsolicited interrupt event lock
        obtain_lock( &pPTPATH->UnsolEventLock );

        // Use a calculated wait
        rc = timed_wait_condition( &pPTPATH->UnsolEvent,
                                   &pPTPATH->UnsolEventLock,
                                   &waittime );

        // Release the path unsolicited interrupt event lock
        release_lock( &pPTPATH->UnsolEventLock );
    }

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPCCW)
    {
        // HHC03994 "%1d:%04X %s: Status %02X"
        WRMSG(HHC03994, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
            pPTPINT->bStatus );
    }

    // Raise an interrupt.
    // device_attention() (in channel.c) raises an unsolicited interrupt
    // for the specified device. Return value is 0 if successful, 1 if
    // the device is busy or pending, or 3 if subchannel is not valid
    // or not enabled.
    // If the device is busy or pending, wait for 100 milliseconds and
    // attempt to raise the interrupt again. Retry up to nine times,
    // i.e. attempt to raise the interrupt up to a total of ten times.
    rc = device_attention( pDEVBLK, pPTPINT->bStatus );
    if (rc == 1)
    {
        for( i = 0; i <= 8; i++ )
        {
            // Wait for 100 milliseconds
            // Calculate when to end the wait.
            gettimeofday( &now, NULL );

            waittime.tv_sec  = now.tv_sec;
            waittime.tv_nsec = (now.tv_usec + (100 * 1000)) * 1000;
            if (waittime.tv_nsec >= 1000000000)
            {
                waittime.tv_sec++;
                waittime.tv_nsec -= 1000000000;
            }

            // Obtain the path unsolicited interrupt event lock
            obtain_lock( &pPTPATH->UnsolEventLock );

            // Use a calculated wait
            rc = timed_wait_condition( &pPTPATH->UnsolEvent,
                                       &pPTPATH->UnsolEventLock,
                                       &waittime );

            // Release the path unsolicited interrupt event lock
            release_lock( &pPTPATH->UnsolEventLock );

            // Attempt to raise the interrupt again.
            rc = device_attention( pDEVBLK, pPTPINT->bStatus );
            if (rc != 1)
                break;
        }
    }

    // Obtain the unsolicited interrupt list lock.
    obtain_lock( &pPTPBLK->UnsolListLock );

    // Return the PTPINT to the LIFO linked list.
    pPTPINT->pNextPTPINT = pPTPBLK->pFirstPTPINT;
    pPTPBLK->pFirstPTPINT = pPTPINT;

    // Release the unsolicited interrupt list lock.
    release_lock( &pPTPBLK->UnsolListLock );

    // That's all; the interrupt has been raised, or maybe not...
    return NULL;
}   /* End function  ptp_unsol_int_thread() */


/* ------------------------------------------------------------------ */
/* ptp_get_tod_clock()                                                */
/* ------------------------------------------------------------------ */
// Note: the returned TodClock (8-bytes) is in network byte order.

void     ptp_get_tod_clock( BYTE* TodClock )
{
    REGS    *regs;
    ETOD    ETOD;
    TOD     tod;

    obtain_lock( &sysblk.cpulock[ sysblk.pcpu ]);
    {
        regs = sysblk.regs[ sysblk.pcpu ];
        etod_clock( regs, &ETOD, ETOD_standard );
        tod = ETOD2TOD( ETOD );
    }
    release_lock( &sysblk.cpulock[ sysblk.pcpu ]);

    STORE_DW( TodClock, tod );

    return;
}


/* ------------------------------------------------------------------ */
/* get_subarea_address()                                              */
/* ------------------------------------------------------------------ */
// Note: the returned SAaddress (4-bytes) is in network byte order.
// VTAM creates a 4-byte subarea address from a combination of the
// output of a STIDP instuction and the output of a STCK
// instruction. The first 2-bytes of the subarea address come from
// bits 28 to 43 of the output of the STIDP instuction, which is the
// last 4-bits of the 'CPU Identification Number' and the first
// 12-bits of the 'Machine-Type Number'. The last 2-bytes of the
// subarea address come from bits 31 to 46 of the output of the STCK
// instuction.

void     get_subarea_address( BYTE* SAaddress )
{
    REGS    *regs;
    ETOD    ETOD;
    TOD     tod;
    U64     dreg;                       /* Double word workarea       */
    U16     hreg;                       /* Half word workarea         */


    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);
    regs = sysblk.regs[sysblk.pcpu];
    etod_clock(regs, &ETOD, ETOD_standard);
    tod = ETOD2TOD(ETOD);
    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    dreg = sysblk.cpuid;

    hreg = ( ( dreg >> 20 ) & 0x000000000000FFFFULL );
    STORE_HW( SAaddress+0, hreg );
    hreg = ( ( tod >> 17 ) & 0x000000000000FFFFULL );
    STORE_HW( SAaddress+2, hreg );

    return;
}


/* ------------------------------------------------------------------ */
/* write_hx0_01()                                                     */
/* ------------------------------------------------------------------ */
// When we are handshaking the second CCW in the chain is the guest
// OS on the y-side writing an PTPHX0 type 0x00 or 0x01.

void  write_hx0_01( DEVBLK* pDEVBLK, U32  uCount,
                    int     iCCWSeq, BYTE* pIOBuf,
                    BYTE*   pMore,   BYTE* pUnitStat,
                    U32*    pResidual )
{

    PTPATH*    pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;
//  PTPHX0*    pPTPHX0  = (PTPHX0*)pIOBuf;

    UNREFERENCED( uCount  );
    UNREFERENCED( iCCWSeq );
    UNREFERENCED( pIOBuf  );


    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "In HX0" );
    }

    // PTPHX0 type 0x01's should only be written when handshaking is being
    // initiated or is in progress.
    if (!pPTPATH->fHandshaking)
    {
        // An PTPHX0 type 0x01 is being written by the y-side when handshaking
        // was not in progress on the path; we will assume that the y-side
        // has initiated handshaking and that the PTPHX0 is the start of
        // handshake one.
        pPTPATH->fHandshaking = TRUE;                  // Hanshakining in progress
        pPTPATH->fHandshakeCur = HANDSHAKE_ONE;        // Handshake one in progress
        pPTPATH->fHandshakeSta |= HANDSHAKE_ONE;       // handshake one started

        // The guest OS on the y-side has started, or restarted, the
        // device, so reset or update any necessary values in the PTPBLK.
        // Note: this section of code is executed twice, once for the read
        // path and once for the write path, though not necessarily in that
        // order. Whichever path executes the routine first will reset or
        // update the values.
        obtain_lock( &pPTPBLK->UpdateLock );
        if (pPTPBLK->xTokensUpdated == 0)              // if not updated
        {
            pPTPBLK->xTokensUpdated = 1;               // updated

            // Reset the active & terminate indicators
            if (pPTPBLK->fActive4)
            {
                // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
                WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestIPAddr4 );
            }
            pPTPBLK->fActive4 = FALSE;
            pPTPBLK->bActivate4 = 0x00;
            pPTPBLK->bTerminate4 = 0x00;
#if defined(ENABLE_IPV6)
            if (pPTPBLK->fActive6)
            {
                // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
                WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestIPAddr6 );
            }
#endif /* defined(ENABLE_IPV6) */
            pPTPBLK->fActive6 = FALSE;
            pPTPBLK->bActivate6 = 0x00;
            pPTPBLK->bTerminate6 = 0x00;
#if defined(ENABLE_IPV6)
            if (pPTPBLK->fActiveLL6)
            {
                // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
                WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestLLAddr6 );
            }
#endif /* defined(ENABLE_IPV6) */
            pPTPBLK->fActiveLL6 = FALSE;
            pPTPBLK->bActivateLL6 = 0x00;
            pPTPBLK->bTerminateLL6 = 0x00;

            // Obtain the lock for manipulating the tokens
            obtain_lock( &TokenLock );

            // Set the x-side tokens
            STORE_FW( pPTPBLK->xTokenIssuerRm, uTokenIssuerRm );
            STORE_FW( pPTPBLK->xTokenCmFilter, uTokenCmFilter );
            STORE_FW( pPTPBLK->xTokenCmConnection, uTokenCmConnection );
            STORE_FW( pPTPBLK->xTokenUlpFilter, uTokenUlpFilter );
            STORE_FW( pPTPBLK->xTokenUlpConnection, uTokenUlpConnection );

            // Increment the tokens
            uTokenIssuerRm      += INCREMENT_TOKEN;
            uTokenCmFilter      += INCREMENT_TOKEN;
            uTokenCmConnection  += INCREMENT_TOKEN;
            uTokenUlpFilter     += INCREMENT_TOKEN;
            uTokenUlpConnection += INCREMENT_TOKEN;

            // Release the lock for manipulating the tokens
            release_lock( &TokenLock );

            // Clear the y-side tokens
            pPTPBLK->yTokensCopied = 0;
            memset( pPTPBLK->yTokenIssuerRm, 0, MPC_TOKEN_LENGTH );
            memset( pPTPBLK->yTokenCmFilter, 0, MPC_TOKEN_LENGTH );
            memset( pPTPBLK->yTokenCmConnection, 0, MPC_TOKEN_LENGTH );
            memset( pPTPBLK->yTokenUlpFilter, 0, MPC_TOKEN_LENGTH );
            memset( pPTPBLK->yTokenUlpConnection, 0, MPC_TOKEN_LENGTH );

            // Reset the y-side process sequence numbers
            pPTPBLK->uSeqNumIssuer = 0;
            pPTPBLK->uSeqNumCm = 0;

            /* Clear the y-side IP address information */
            if (pPTPBLK->fPreconfigured) {
                if (!pPTPBLK->fPreGuestIPAddr4) {
                memset(pPTPBLK->szGuestIPAddr4, 0, sizeof(pPTPBLK->szGuestIPAddr4));
                memset(&pPTPBLK->iaGuestIPAddr4, 0, sizeof(pPTPBLK->iaGuestIPAddr4));
                }
#if defined(ENABLE_IPV6)
                memset(pPTPBLK->szGuestIPAddr6, 0, sizeof(pPTPBLK->szGuestIPAddr6));
                memset(&pPTPBLK->iaGuestIPAddr6, 0, sizeof(pPTPBLK->iaGuestIPAddr6));
                memset(pPTPBLK->szGuestLLAddr6, 0, sizeof(pPTPBLK->szGuestLLAddr6));
                memset(&pPTPBLK->iaGuestLLAddr6, 0, sizeof(pPTPBLK->iaGuestLLAddr6));
#endif /* defined(ENABLE_IPV6) */
            }

        }
        else
        {
            pPTPBLK->xTokensUpdated = 0;               // update next restart
        }
        release_lock( &pPTPBLK->UpdateLock );

        // The guest OS on the y-side has started, or restarted, the
        // device, so dispose of any data waiting to be read by the y-side.
        // Free any PTPHDR on the chain for the path
        remove_and_free_any_buffers_on_chain( pPTPATH );
        // Reset the message sequence number
        pPTPATH->uSeqNum = 0;
    }
    else
    {
        // An PTPHX0 type 0x01 is being written by the y-side and handshaking
        // is in progress on the path; we will assume that the y-side
        // is continuing handshaking and that the PTPHX0 is the start
        // of handshake three.
        pPTPATH->fHandshakeCur = HANDSHAKE_THREE;      // Handshake three in progress
        pPTPATH->fHandshakeSta |= HANDSHAKE_THREE;     // handshake three started
    }

    // Set residual byte count and unit status.
    *pMore = 0;
    *pResidual = 0;
    *pUnitStat = CSW_CE | CSW_DE;

    return;
}   /* End function  write_hx0_01() */


/* ------------------------------------------------------------------ */
/* write_hx0_00()                                                     */
/* ------------------------------------------------------------------ */
// When we are handshaking the second CCW in the chain is the guest
// OS on the y-side writing an PTPHX0 type 0x00 or 0x01.

void  write_hx0_00( DEVBLK* pDEVBLK, U32  uCount,
                    int     iCCWSeq, BYTE* pIOBuf,
                    BYTE*   pMore,   BYTE* pUnitStat,
                    U32*    pResidual )
{

    PTPATH*    pPTPATH  = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK  = pPTPATH->pPTPBLK;
//  PTPHX0*    pPTPHX0  = (PTPHX0*)pIOBuf;

    UNREFERENCED( uCount  );
    UNREFERENCED( iCCWSeq );
    UNREFERENCED( pIOBuf  );


    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "In HX0" );
    }

    // PTPHX0 type 0x00 should only be written when handshaking is in progress.
    if (pPTPATH->fHandshaking)
    {
        // An PTPHX0 type 0x00 is being written by the y-side and handshaking
        // is in progress on the path; we will assume that the y-side
        // is continuing handshaking and that the PTPHX0 is the start
        // of handshake two.
        pPTPATH->fHandshakeCur = HANDSHAKE_TWO;        // Handshake two in progress
        pPTPATH->fHandshakeSta |= HANDSHAKE_TWO;       // handshake two started
    }
    else
    {
        // An PTPHX0 type 0x00 is being written by the y-side and handshaking
        // is not in progress on the path; we don't understand!
    }

    // Set residual byte count and unit status.
    *pMore = 0;
    *pResidual = 0;
    *pUnitStat = CSW_CE | CSW_DE;

    return;
}   /* End function  write_hx0_00() */


/* ------------------------------------------------------------------ */
/* write_hx2()                                                        */
/* ------------------------------------------------------------------ */
// When we are handshaking the third CCW in the chain is the guest
// OS on the y-side writing an XID2. We must create the PTPHX0, XID2
// and 'VTAM' to be read by the fourth, fifth and sixth CCW's in the
// chain.

void  write_hx2( DEVBLK* pDEVBLK, U32  uCount,
                 int     iCCWSeq, BYTE* pIOBuf,
                 BYTE*   pMore,   BYTE* pUnitStat,
                 U32*    pResidual )
{

    PTPATH*    pPTPATH     = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK     = pPTPATH->pPTPBLK;
    PTPHX2*    pPTPHX2wr   = (PTPHX2*)pIOBuf;   // PTPHX2 being written
    PTPHSV*    pPTPHSVwr;                       // PTPHSV being written

    PTPHDR*    pPTPHDRx0   = NULL;              // PTPHDR before PTPHX0
    PTPHX0*    pPTPHX0re   = NULL;              // PTPHX0 to be read
    PTPHDR*    pPTPHDRx2   = NULL;              // PTPHDR before PTPHX2
    PTPHX2*    pPTPHX2re   = NULL;              // PTPHX2 to be read
    PTPHSV*    pPTPHSVre   = NULL;              // PTPHSV to be read
    PTPHDR*    pPTPHDRvt   = NULL;              // PTPHDR before 'VTAM'
    BYTE*      pPTPVTMre   = NULL;              // 'VTAM' to be read

    U16        uDataLen1;                       // Data length one
    U16        uMaxReadLen;                     // Maximum read length
    PTPHDR*    pPTPHDR;                         // PTPHDR
    U32        uNodeID;

    UNREFERENCED( uCount  );
    UNREFERENCED( iCCWSeq );
    UNREFERENCED( pIOBuf  );


    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "In HX2" );
    }

    // Point to the CSVcv following the XID2.
    pPTPHSVwr = point_CSVcv( pDEVBLK, pPTPHX2wr );

    // XID2's should only be written when handshaking is in progress.
    if (pPTPATH->fHandshaking)
    {
        // An XID2 is being written by the y-side and handshaking is
        // in progress on the path; all is well with the world.

        // Copy the start time and token from the XID2.
        // Note: this section of code is executed twice, once for the read
        // path and once for the write path, though not necessarily in that
        // order. Whichever path executes the routine first will copy the
        // values.
        if (pPTPATH->fHandshakeCur == HANDSHAKE_ONE)
        {

            obtain_lock( &pPTPBLK->UpdateLock );

            if (pPTPHX2wr->DLCtype != pPTPATH->bDLCtype )  // XID2 from expected y-side's path?
            {
                // HHC03917 "%1d:%04X PTP: Guest read and write paths mis-configured"
                WRMSG(HHC03917, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum );
            }

            if (pPTPBLK->yTokensCopied == 0)
            {

                pPTPBLK->yTokensCopied = 1;
                memcpy( pPTPBLK->yTokenIssuerRm, pPTPHX2wr->Token, MPC_TOKEN_LENGTH );
                memcpy( pPTPBLK->yStartTime, pPTPHSVwr->SID1, 8 );

                gen_csv_sid( pPTPBLK->yStartTime, pPTPBLK->xStartTime,
                             pPTPBLK->xSecondCsvSID2 );

            }

            if (pPTPHX2wr->DLCtype == DLCTYPE_READ)              // XID2 from y-side's Read path?
            {
                // Extract the data lengths from the XID2.
                FETCH_HW( uDataLen1, pPTPHX2wr->DataLen1 );      // Get y-side's data length one
                FETCH_HW( uMaxReadLen, pPTPHX2wr->MaxReadLen );  // Get y-side's maximum read length

                // Obtain the read buffer lock.
                obtain_lock( &pPTPBLK->ReadBufferLock );

                // Point to the read buffer.
                pPTPHDR = pPTPBLK->pReadBuffer;
                pPTPBLK->pReadBuffer = NULL;

                // Replace the existing read buffer if necessary.
                // (This is the buffer into which we place packets
                // received from the TUN interface, and from which
                // the y-side's read path reads the packets.)
                if (pPTPBLK->yMaxReadLen != uMaxReadLen)
                {
                    // Free the existing read buffer, if there is one.
                    if (pPTPHDR)
                        free( pPTPHDR );

                    // Allocate a new read buffer.
                    pPTPHDR = alloc_ptp_buffer( pDEVBLK, (int)uMaxReadLen );
                }

                // Initialize the existing or new read buffer, if there is one.
                if (pPTPHDR)
                {
                    MPC_TH*    pMPC_TH;        // MPC_TH follows the PTPHDR
                    MPC_RRH*   pMPC_RRH;       // MPC_RRH follows the MPC_TH
//                  MPC_PH*    pMPC_PH;        // MPC_PH follows the MPC_RRH

                    // Fix-up various pointers
                    pMPC_TH = (MPC_TH*)((BYTE*)pPTPHDR + SIZE_HDR);
                    pMPC_RRH = (MPC_RRH*)((BYTE*)pMPC_TH + SIZE_TH);
//                  pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + SIZE_RRH);

                    // Prepare PTPHDR
                    pPTPHDR->iDataLen = LEN_OF_PAGE_ONE;

                    // Clear the data area.
                    memset( pMPC_TH, 0, (int)uMaxReadLen );

                    // Prepare MPC_TH
                    STORE_FW( pMPC_TH->first4, MPC_TH_FIRST4 );
                    STORE_FW( pMPC_TH->offrrh, SIZE_TH );
                    STORE_HW( pMPC_TH->unknown10, MPC_TH_UNKNOWN10 );    // !!! //
                    STORE_HW( pMPC_TH->numrrh, 1 );

                    // Prepare MPC_RRH
                    pMPC_RRH->type = RRH_TYPE_CM;
                    pMPC_RRH->proto = PROTOCOL_LAYER2;
                    STORE_HW( pMPC_RRH->numph, 1 );
                    STORE_HW( pMPC_RRH->offph, SIZE_RRH );

                    // Prepare MPC_PH

                    // increment the read buffer generation number
                    pPTPBLK->iReadBufferGen++;
                }

                // Set the pointer to the read buffer.
                pPTPBLK->pReadBuffer = pPTPHDR;

                // Release the read buffer lock.
                release_lock( &pPTPBLK->ReadBufferLock );

                // Copy the data lengths extracted from the XID2.
                pPTPBLK->yDataLen1 = uDataLen1;
                pPTPBLK->yMaxReadLen = uMaxReadLen;
                pPTPBLK->yActMTU = ( pPTPBLK->yMaxReadLen - pPTPBLK->yDataLen1 ) - 2048;

                // An MPCPTP/MPCPTP6 connection uses an MTU that is the  smaller of
                // a) the MTU specified on a route statement, or b) the MTU calculated
                // from the maximum read length reported by the other side during
                // handshaking. For example, if the x-side's TRLE definition is using
                // the default MAXBFRU value of 5, the maximum read length reported
                // by the x-side to the y-side will be 20476 (0x4FFC) bytes, from
                // which both sides will calculate an MTU of 14336 (0x3800) bytes.
                // If the y-side has a route statement that specifies an MTU of 24576
                // (0x6000) bytes, the specified MTU is ignored and the calculated MTU
                // will be used. Depending on the values specified for MAXBFRU and for
                // route statements, the MTU in use from the x-side to the y-side
                // could be different to the MTU in use from the y-side to the x-side.
                // For a real MPCPTP/MPCPTP6 connection this is probably a good thing,
                // with the maximum capacity in each direction automatically used.
                // However, for this emulated MPCPTP/MPCPTP6 connection this could be
                // a bad thing because we are not processing the packets, we are simply
                // forwarding them, and we may be forwarding them to something that is
                // using a smaller MTU.

                // HHC03910 "%1d:%04X PTP: Hercules has maximum read length of size %d bytes and actual MTU of size %d bytes"
                WRMSG(HHC03910, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                     (int)pPTPBLK->xMaxReadLen, (int)pPTPBLK->xActMTU );
                // HHC03911 "%1d:%04X PTP: Guest has maximum read length of size %d bytes and actual MTU of size %d bytes"
                WRMSG(HHC03911, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                     (int)pPTPBLK->yMaxReadLen, (int)pPTPBLK->yActMTU );

            }

            release_lock( &pPTPBLK->UpdateLock );

        }

        // Allocate a buffer in which the PTPHX0 will be build.
        pPTPHDRx0 = alloc_ptp_buffer( pDEVBLK, SIZE_HX0 );
        if (!pPTPHDRx0)
            return;
        pPTPHX0re = (PTPHX0*)((BYTE*)pPTPHDRx0 + SIZE_HDR);

        // Allocate a buffer in which the PTPHX2 and PTPHSV will be build.
        pPTPHDRx2 = alloc_ptp_buffer( pDEVBLK, 255 );
        if (!pPTPHDRx2)
        {
            // Free the PTPHDR
            free( pPTPHDRx0 );
            return;
        }
        pPTPHX2re = (PTPHX2*)((BYTE*)pPTPHDRx2 + SIZE_HDR);
        pPTPHSVre = (PTPHSV*)((BYTE*)pPTPHDRx2 + SIZE_HDR + SIZE_HX2);

        // Allocate a buffer in which the literal 'VTAM' will be build.
        pPTPHDRvt = alloc_ptp_buffer( pDEVBLK, 4 );
        if (!pPTPHDRvt)
        {
            // Free the PTPHDRs
            free( pPTPHDRx2 );
            free( pPTPHDRx0 );
            return;
        }
        pPTPVTMre = (BYTE*)pPTPHDRvt + SIZE_HDR;

        // Prepare PTPHDRx0 and PTPHX0re
        pPTPHDRx0->iDataLen = SIZE_HX0;
        // PTPHX0
        if (pPTPATH->fHandshakeCur == HANDSHAKE_ONE ||
            pPTPATH->fHandshakeCur == HANDSHAKE_TWO)
        {
            pPTPHX0re->TH_ch_flag = TH_CH_0x01;
        }
        else
        {
            pPTPHX0re->TH_ch_flag = TH_CH_0x00;
        }
        pPTPHX0re->TH_blk_flag = TH_DATA_IS_XID;
        pPTPHX0re->TH_is_xid = TH_IS_0x01;
        STORE_FW( pPTPHX0re->TH_SeqNum, PTPHX0_SEQNUM );      // !!! //

        // Prepare PTPHDRx2 PTPHX2re and PTPHSVre
        pPTPHDRx2->iDataLen = 255;
        // XID2
        pPTPHX2re->Ft = 0x20;
        pPTPHX2re->Length = SIZE_HX2 + SIZE_HSV;
        uNodeID = ( sysblk.cpuid >> 36 ) | 0xFFF00000;
        STORE_FW( pPTPHX2re->NodeID, uNodeID );
        pPTPHX2re->LenXcv = SIZE_HX2;
        pPTPHX2re->ULPuse = 0x80;
        memcpy( pPTPHX2re->SAaddress, pPTPBLK->xSAaddress, 4 );
        if (pPTPATH->fHandshakeCur != HANDSHAKE_ONE)
        {
            pPTPHX2re->CLstatus = 0x07;
        }
        if (pPTPATH->bDLCtype == DLCTYPE_WRITE)         // Destined for the y-side's Write path?
        {
            pPTPHX2re->DLCtype = DLCTYPE_READ;          // This XID2 is from x-side's Read path
        }
        else
        {
            pPTPHX2re->DLCtype = DLCTYPE_WRITE;         // This XID2 is from x-side's Write path
        }
        if (pPTPATH->fHandshakeCur == HANDSHAKE_ONE &&  // The first exchange of handshaking and
            pPTPATH->bDLCtype == DLCTYPE_WRITE)         // destined for the y-side's Write path?
        {
            STORE_HW( pPTPHX2re->DataLen1, pPTPBLK->xDataLen1 );
            STORE_HW( pPTPHX2re->MaxReadLen, pPTPBLK->xMaxReadLen );
        }
        pPTPHX2re->MpcFlag = 0x27;
        if (pPTPATH->fHandshakeCur == HANDSHAKE_ONE)
        {
            pPTPHX2re->TokenX5 = MPC_TOKEN_X5;
            memcpy( pPTPHX2re->Token, pPTPBLK->xTokenIssuerRm, MPC_TOKEN_LENGTH );
        }
        else
        {
            pPTPHX2re->TokenX5 = MPC_TOKEN_X5;
            memcpy( pPTPHX2re->Token, pPTPBLK->yTokenIssuerRm, MPC_TOKEN_LENGTH );
        }
        // CSVcv
        pPTPHSVre->Length = SIZE_HSV;
        pPTPHSVre->Key = CSV_KEY;
        pPTPHSVre->LenSIDs = sizeof(pPTPHSVre->LenSIDs) +
                              sizeof(pPTPHSVre->SID1) + sizeof(pPTPHSVre->SID2);
        memcpy( pPTPHSVre->SID1, &pPTPBLK->xStartTime, 8 );  // x-side's start time
        if (pPTPATH->fHandshakeCur == HANDSHAKE_ONE)
        {
            memcpy( pPTPHSVre->SID2, &pPTPBLK->xFirstCsvSID2, 8 );
        }
        else
        {
            memcpy( pPTPHSVre->SID2, &pPTPBLK->xSecondCsvSID2, 8 );
        }

        // Prepare PTPHDRvt
        pPTPHDRvt->iDataLen = 4;
        memcpy( pPTPVTMre, &VTAM_ebcdic, 4 );      // 'VTAM' in EBCDIC

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "Out HX0" );
            mpc_display_description( pDEVBLK, "Out HX2" );
        }

        // Add PTPHDRs to chain.
        add_buffer_to_chain( pPTPATH, pPTPHDRx0 );
        add_buffer_to_chain( pPTPATH, pPTPHDRx2 );
        add_buffer_to_chain( pPTPATH, pPTPHDRvt );

    }
    else
    {
        // An XID2 is being written by the y-side but handshaking is
        // not in progress on the path; we don't understand!
    }

    // Set residual byte count and unit status.
    *pMore = 0;
    *pResidual = 0;
    *pUnitStat = CSW_CE | CSW_DE;

    return;
}   /* End function  write_hx2() */


/* ------------------------------------------------------------------ */
/* point_CSVcv()                                                      */
/* ------------------------------------------------------------------ */
// This function is probably overkill at present while the XID2 is
// followed immediately by the CSVcv. However, one day, there may be
// more than one cv...

PTPHSV*  point_CSVcv( DEVBLK* pDEVBLK, PTPHX2* pPTPHX2 )
{

    PTPHSV*   pPTPHSV;
    BYTE*     pCurCv;
    BYTE*     pEndCv;

    UNREFERENCED( pDEVBLK );


    // Point to the first cv, point to the end of the cv's, and
    // work down the cv's until we find the CSVcv.
    pCurCv = (BYTE*)pPTPHX2 + pPTPHX2->LenXcv;
    pEndCv = (BYTE*)pPTPHX2 + pPTPHX2->Length;

    while( pCurCv < pEndCv )
    {
        pPTPHSV = (PTPHSV*)pCurCv;
        if (pPTPHSV->Key == CSV_KEY)
            break;
        pCurCv += pPTPHSV->Length;
        pPTPHSV = NULL;
    }

    return pPTPHSV;
}   /* End function  point_CSVcv() */


/* ------------------------------------------------------------------ */
/* write_rrh_417E()                                                   */
/* ------------------------------------------------------------------ */
// Note - the Token is xTokenIssuerRm or xTokenCmConnection.
// In all cases that have been seen the MPC_RRH type 0x417E is
// followed by a single MPC_PH, which is followed by a single
// MPC_PUK, which is followed by up to four MPC_PUSs.

int   write_rrh_417E( DEVBLK* pDEVBLK, MPC_TH* pMPC_THwr, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH   = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK   = pPTPATH->pPTPBLK;
    PTPATH*    pPTPATHre = pPTPBLK->pPTPATHRead;
    PTPATH*    pPTPATHwr = pPTPBLK->pPTPATHWrite;
    MPC_PH*    pMPC_PHwr;                      // MPC_PH being written
    MPC_PUK*   pMPC_PUKwr;                     // MPC_PUK being written
    MPC_PUS*   pMPC_PUSwr;                     // MPC_PUSs being written
    U16        uOffPH;
//  U32        uLenData;
    U32        uOffData;
    u_int      fxSideWins;
    PTPHDR*    pPTPHDRre;                      // PTPHDR to be read
    int        iWhat;
#define UNKNOWN_PUK   0
#define CM_ENABLE     1
#define CM_SETUP      2
#define CM_CONFIRM    3
#define CM_DISABLE    4
#define CM_TAKEDOWN   5
#define ULP_ENABLE    6
#define ULP_SETUP     7
#define ULP_CONFIRM   8
#define ULP_DISABLE   9
#define ULP_TAKEDOWN  10
#define DM_ACT        11


    // Point to the MPC_PH.
    FETCH_HW( uOffPH, pMPC_RRHwr->offph );
    pMPC_PHwr = (MPC_PH*)((BYTE*)pMPC_RRHwr + uOffPH);

    // Get the length of and point to the data referenced by the
    // MPC_PH. The data contain a MPC_PUK and one or more MPC_PUSs.
//  FETCH_F3( uLenData, pMPC_PH->lendata );
    FETCH_FW( uOffData, pMPC_PHwr->offdata );
    pMPC_PUKwr = (MPC_PUK*)((BYTE*)pMPC_THwr + uOffData);

    // Decide what the PUK contains.
    iWhat = UNKNOWN_PUK;
    if (memcmp( pMPC_RRHwr->token, pPTPBLK->xTokenIssuerRm, MPC_TOKEN_LENGTH ) == 0)
    {
        if (pMPC_PUKwr->what == PUK_WHAT_41)
        {
            if (pMPC_PUKwr->type == PUK_TYPE_ENABLE)
            {
                iWhat = CM_ENABLE;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_SETUP)
            {
                iWhat = CM_SETUP;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_CONFIRM)
            {
                iWhat = CM_CONFIRM;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_DISABLE)
            {
                iWhat = CM_DISABLE;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_TAKEDOWN)
            {
                iWhat = CM_TAKEDOWN;
            }
        }
    }
    else if (memcmp( pMPC_RRHwr->token, pPTPBLK->xTokenCmConnection, MPC_TOKEN_LENGTH ) == 0)
    {
        if (pMPC_PUKwr->what == PUK_WHAT_41)
        {
            if (pMPC_PUKwr->type == PUK_TYPE_ENABLE)
            {
                iWhat = ULP_ENABLE;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_SETUP)
            {
                iWhat = ULP_SETUP;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_CONFIRM)
            {
                iWhat = ULP_CONFIRM;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_DISABLE)
            {
                iWhat = ULP_DISABLE;
            }
            else if (pMPC_PUKwr->type == PUK_TYPE_TAKEDOWN)
            {
                iWhat = ULP_TAKEDOWN;
            }
        }
        else if (pMPC_PUKwr->what == PUK_WHAT_43)
        {
            if (pMPC_PUKwr->type == PUK_TYPE_ACTIVE)
            {
                iWhat = DM_ACT;
            }
        }
    }

    // Process the PUK.
    switch( iWhat )
    {

    // PUK 0x4102 to xTokenIssuerRm
    // The MPC_PUK should be followed by three MPC_PUSs, the first a type
    // 0x0401, the second a type 0x0402, and the third a type 0x040c.
    case CM_ENABLE:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (Issuer) PUK 0x4102 (CM_ENABLE)" );
        }

        // Find the PUS and copy the yTokenCmFilter.
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_01 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (CM_ENABLE)", "PUS_01" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        memcpy( pPTPBLK->yTokenCmFilter, pMPC_PUSwr->vc.pus_01.token, MPC_TOKEN_LENGTH );

        // Find the PUS that contains the 'bid' value.
        // Build RRH 0x417E PUK 0x4102 to yTokenIssuerRm
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_02 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (CM_ENABLE)", "PUS_02" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        fxSideWins = FALSE;
        pPTPHDRre = build_417E_cm_enable( pDEVBLK, pMPC_RRHwr, pMPC_PUSwr, &fxSideWins );

        // Add PTPHDR to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        // If this side 'wins' the 'handedness' this side must now  send
        // a RRH 0x417E PUK 0x4104 to yTokenIssuerRm on the other side.
        // If the other side 'wins' the 'handedness' this side must now wait
        // to receive a RRH 0x417E PUK 0x4104 to xTokenIssuerRm from the
        // other side.
        if (fxSideWins)     // if the x-side wins
        {

            // Build RRH 0x417E PUK 0x4104 to yTokenIssuerRm
            pPTPHDRre = build_417E_cm_setup( pDEVBLK, pMPC_RRHwr );

            // Add PTPHDR to chain.
            add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        }

        break;

    // PUK 0x4104 to xTokenIssuerRm
    // The MPC_PUK should be followed by three MPC_PUSs, the first a type
    // 0x0404, the second a type 0x0405, and the third a type 0x0406.
    case CM_SETUP:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (Issuer) PUK 0x4104 (CM_SETUP)" );
        }

        // Find the PUS and copy the yTokenCmConnection.
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_04 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (CM_SETUP)", "PUS_04" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        memcpy( pPTPBLK->yTokenCmConnection, pMPC_PUSwr->vc.pus_04.token, MPC_TOKEN_LENGTH );

        // Build RRH 0x417E PUK 0x4106 to yTokenIssuerRm
        pPTPHDRre = build_417E_cm_confirm( pDEVBLK, pMPC_RRHwr );

        // Add PTPHDR to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        // When the y-side receives the RRH 0x417E PUK 0x4106 the control
        // process between the x-side and the y-side is active. On a connection
        // between two VTAMs, the x-side VTAM sends messages from xTokenCmFilter
        // to yTokenCmConnection, and the y-side VTAM sends messages from yTokenCmFilter
        // to xTokenCmConnection. The x-side (i.e. Hercules) will now wait for the
        // y-side VTAM to send a MSG 0x417E PUK 0x4102 to xTokenCmConnection to
        // begin the setup of the communication process.

        break;

    // PUK 0x4106 to xTokenIssuerRm
    // The MPC_PUK should be followed by three MPC_PUSs, the first a type
    // 0x0404, the second a type 0x0408, and the third a type 0x0407.
    case CM_CONFIRM:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (Issuer) PUK 0x4106 (CM_CONFIRM)" );
        }

        // Find the PUS and copy the yTokenCmConnection.
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_08 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (CM_CONFIRM)", "PUS_08" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        memcpy( pPTPBLK->yTokenCmConnection, pMPC_PUSwr->vc.pus_08.token, MPC_TOKEN_LENGTH );

        // The control process between the x-side and the y-side is active. On
        // a connection between two VTAMs, the x-side VTAM sends messages from
        // xTokenCmFilter to yTokenCmConnection, and the y-side VTAM sends messages
        // from yTokenCmFilter to xTokenCmConnection. The x-side (i.e. Hercules)
        // will now wait for the y-side VTAM to send a MSG 0x417E PUK 0x4102
        // to xTokenCmConnection to begin the setup of the communication process.

        break;

    // PUK 0x4103 to xTokenIssuerRm
    // The MPC_PUK should be followed by a single MPC_PUS, a type 0x0403.
    case CM_DISABLE:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (Issuer) PUK 0x4103 (CM_DISABLE)" );
        }

        break;

    // PUK 0x4105 to xTokenIssuerRm
    // The MPC_PUK should be followed by a single MPC_PUS, a type 0x0404.
    case CM_TAKEDOWN:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (Issuer) PUK 0x4105 (CM_TAKEDOWN)" );
        }

        // The guest OS on the y-side has stopped the device

        break;

    // PUK 0x4102 to xTokenCmConnection
    // The MPC_PUK should be followed by two MPC_PUSs, the first a type
    // 0x0401 and the second a type 0x0402.
    case ULP_ENABLE:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (CmComm) PUK 0x4102 (ULP_ENABLE)" );
        }

        // Find the PUS and copy the yTokenUlpFilter.
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_01 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (ULP_ENABLE)", "PUS_01" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        memcpy( pPTPBLK->yTokenUlpFilter, pMPC_PUSwr->vc.pus_01.token, MPC_TOKEN_LENGTH );

        // Find the PUS that contains the 'bid' value.
        // Build RRH 0x417E PUK 0x4102 to yTokenCmConnection
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_02 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (ULP_ENABLE)", "PUS_02" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        fxSideWins = FALSE;
        pPTPHDRre = build_417E_ulp_enable( pDEVBLK, pMPC_RRHwr, pMPC_PUSwr, &fxSideWins );

        // Add PTPHDR to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        // If this side 'wins' the 'handedness' this side must now send
        // a RRH 0x417E PUK 0x4104 to yTokenCmConnection on the other side.
        // If the other side 'wins' the 'handedness' this side must now wait
        // to receive a RRH 0x417E PUK 0x4104 to xTokenCmConnection from the
        // other side.
        if (fxSideWins)     // if the x-side wins
        {

            // Build RRH 0x417E PUK 0x4104 to yTokenCmConnection
            pPTPHDRre = build_417E_ulp_setup( pDEVBLK, pMPC_RRHwr );

            // Add PTPHDR to chain.
            add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        }

        break;

    // PUK 0x4104 to xTokenCmConnection
    // The MPC_PUK should be followed by four MPC_PUSs, the first a type
    // 0x0404, the second a type 0x0405, the third a type 0x0406, and
    // the fourth a type 0x0402.
    case ULP_SETUP:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (CmComm) PUK 0x4104 (ULP_SETUP)" );
        }

        // Find the PUS and copy the yTokenUlpConnection.
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_04 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (ULP_SETUP)", "PUS_04" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        memcpy( pPTPBLK->yTokenUlpConnection, pMPC_PUSwr->vc.pus_04.token, MPC_TOKEN_LENGTH );

        // Build RRH 0x417E PUK 0x4106 to yTokenCmConnection
        pPTPHDRre = build_417E_ulp_confirm( pDEVBLK, pMPC_RRHwr );

        // Add PTPHDR to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        // Build RRH 0x417E PUK 0x4360 to yTokenCmConnection
        pPTPHDRre = build_417E_dm_act( pDEVBLK, pMPC_RRHwr );

        // Add PTPHDR to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        break;

    // PUK 0x4106 to xTokenCmConnection
    // The MPC_PUK should be followed by four MPC_PUSs, the first a type
    // 0x0404, the second a type 0x0408, the third a type 0x0407, and
    // the fourth a type 0x0402.
    case ULP_CONFIRM:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (CmComm) PUK 0x4106 (ULP_CONFIRM)" );
        }

        // Find the PUS and copy the yTokenUlpConnection.
        pMPC_PUSwr = mpc_point_pus( pDEVBLK, pMPC_PUKwr, PUS_TYPE_08 );
        if (!pMPC_PUSwr)
        {
            // HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
            WRMSG(HHC03937, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK (ULP_CONFIRM)", "PUS_08" );
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );
            break;
        }
        memcpy( pPTPBLK->yTokenUlpConnection, pMPC_PUSwr->vc.pus_08.token, MPC_TOKEN_LENGTH );

        // Build RRH 0x417E PUK 0x4360 to yTokenCmConnection
        pPTPHDRre = build_417E_dm_act( pDEVBLK, pMPC_RRHwr );

        // Add PTPHDR to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

        break;

    // PUK 0x4360 to xTokenCmConnection
    // The MPC_PUK should be followed by a single MPC_PUS, a type 0x0404.
    case DM_ACT:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (CmComm) PUK 0x4360 (DM_ACT)" );
        }

        break;

    // PUK 0x4103 to xTokenCmConnection
    // The MPC_PUK should be followed by a single MPC_PUS, a type 0x0403.
    case ULP_DISABLE:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (CmComm) PUK 0x4103 (ULP_DISABLE)" );
        }

        break;

    // PUK 0x4105 to xTokenCmConnection
    // The MPC_PUK should be followed by a single MPC_PUS, a type 0x0404.
    case ULP_TAKEDOWN:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0x417E (CmComm) PUK 0x4105 (ULP_TAKEDOWN)" );
        }

        // The guest OS on the y-side has stopped the device
        if (pPTPBLK->fActive4)
        {
            // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
            WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestIPAddr4 );
        }
        pPTPBLK->fActive4 = FALSE;
        pPTPBLK->bActivate4 = 0x00;
        pPTPBLK->bTerminate4 = 0x00;
#if defined(ENABLE_IPV6)
        if (pPTPBLK->fActive6)
        {
            // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
            WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestIPAddr6 );
        }
#endif /* defined(ENABLE_IPV6) */
        pPTPBLK->fActive6 = FALSE;
        pPTPBLK->bActivate6 = 0x00;
        pPTPBLK->bTerminate6 = 0x00;
#if defined(ENABLE_IPV6)
        if (pPTPBLK->fActiveLL6)
        {
            // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
            WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestLLAddr6 );
        }
#endif /* defined(ENABLE_IPV6) */
        pPTPBLK->fActiveLL6 = FALSE;
        pPTPBLK->bActivateLL6 = 0x00;
        pPTPBLK->bTerminateLL6 = 0x00;
        // Dispose of any data waiting to be read by the y-side.
        // Free any PTPHDR on the chain for the read path
        remove_and_free_any_buffers_on_chain( pPTPATHre );
        // Reset the message sequence number
        pPTPATHre->uSeqNum = 0;
        // Free any PTPHDR on the chain for the write path
        remove_and_free_any_buffers_on_chain( pPTPATHwr );
        // Reset the message sequence number
        pPTPATHwr->uSeqNum = 0;

        break;

    // Unknown PUK
    default:

        // HHC03936 "%1d:%04X PTP: Accept data contains unknown %s"
        WRMSG(HHC03936, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK" );
        mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );

        break;

    }  /* switch( iWhat ) */


    return 0;
}   /* End function  write_rrh_417E() */


/* ------------------------------------------------------------------ */
/* build_417E_cm_enable()                                            */
/* ------------------------------------------------------------------ */
// Build RRH 0x417E PUK 0x4102 to yTokenIssuerRm

PTPHDR*  build_417E_cm_enable( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRHwr,
                             MPC_PUS* pMPC_PUSwr, u_int* fxSideWins )
{

    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    PTPHDR*    pPTPHDRre;      // PTPHDR to be read
    MPC_TH*    pMPC_THre;      // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;     // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;      // MPC_PH follows MPC_RRH
    MPC_PUK*   pMPC_PUKre;     // MPC_PUK follows MPC_PH
    MPC_PUS*   pMPC_PUSre[3];  // MPC_PUSs follow MPC_PUK
    U64        uTod;
    int        rc;

    UNREFERENCED( pMPC_RRHwr );


    // Allocate a buffer in which the response will be build.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_01 +                  // first MPC_PUS (0x0401)
               SIZE_PUS_02_A +                // second MPC_PUS (0x0402)
               SIZE_PUS_0C;                   // third MPC_PUS (0x040c)
    uLength3 = SIZE_PUK + uLength4;           // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PUKre = (MPC_PUK*)((BYTE*)pMPC_PHre + SIZE_PH);
    pMPC_PUSre[0] = (MPC_PUS*)((BYTE*)pMPC_PUKre + SIZE_PUK);
    pMPC_PUSre[1] = (MPC_PUS*)((BYTE*)pMPC_PUSre[0] + SIZE_PUS_01);
    pMPC_PUSre[2] = (MPC_PUS*)((BYTE*)pMPC_PUSre[1] + SIZE_PUS_02_A);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_ULP;
    pMPC_RRHre->proto = PROTOCOL_UNKNOWN;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_FW( pMPC_RRHre->seqnum, ++pPTPBLK->uSeqNumIssuer );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenIssuerRm, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PUKre
    STORE_HW( pMPC_PUKre->length, SIZE_PUK );
    pMPC_PUKre->what = PUK_WHAT_41;
    pMPC_PUKre->type = PUK_TYPE_ENABLE;
    STORE_HW( pMPC_PUKre->lenpus, uLength4 );

    // Prepare first MPC_PUSre
    STORE_HW( pMPC_PUSre[0]->length, SIZE_PUS_01 );
    pMPC_PUSre[0]->what = PUS_WHAT_04;
    pMPC_PUSre[0]->type = PUS_TYPE_01;
    pMPC_PUSre[0]->vc.pus_01.proto = PROTOCOL_UNKNOWN;
    pMPC_PUSre[0]->vc.pus_01.unknown05 = 0x01;           // !!! //
    pMPC_PUSre[0]->vc.pus_01.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[0]->vc.pus_01.token, pPTPBLK->xTokenCmFilter, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUSre
    // Note: the 8-byte value placed in the second MPC_PUS is important.
    // Whichever side has the highest value 'wins', and dictates the
    // 'handedness' of the RRH 0x417E exchanges. If this code 'wins'
    // and then acts like a 'loser', confusion reigns, to the extent
    // that VTAM on the y-side will not shutdown because it thinks
    // the link is still active. Presumably we could always return
    // 0xFF's, but hey...
    STORE_HW( pMPC_PUSre[1]->length, SIZE_PUS_02_A );
    pMPC_PUSre[1]->what = PUS_WHAT_04;
    pMPC_PUSre[1]->type = PUS_TYPE_02;
    ptp_get_tod_clock( pMPC_PUSre[1]->vc.pus_02.a.clock ); // x-side's time

    // Prepare third MPC_PUSre
    STORE_HW( pMPC_PUSre[2]->length, SIZE_PUS_0C );
    pMPC_PUSre[2]->what = PUS_WHAT_04;
    pMPC_PUSre[2]->type = PUS_TYPE_0C;
    pMPC_PUSre[2]->vc.pus_0C.unknown04[0] = 0x00;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[1] = 0x09;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[2] = 0x00;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[3] = 0x06;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[4] = 0x04;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[5] = 0x01;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[6] = 0x03;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[7] = 0x04;        // !!! //
    pMPC_PUSre[2]->vc.pus_0C.unknown04[8] = 0x08;        // !!! //

    // Compare the tod clock value in the MPC_PUSwr with the tod clock
    // value in the second MPC_PUSre to determine which side wins.
    rc = memcmp( pMPC_PUSwr->vc.pus_02.a.clock,
                 pMPC_PUSre[1]->vc.pus_02.a.clock,
                 sizeof(pMPC_PUSwr->vc.pus_02.a.clock) );
    if (rc < 0)
        // This should be the normal case; the other side must have
        // obtained the tod clock a few moments ago for it to be in the
        // message we recently received.
        *fxSideWins = TRUE;     // i.e. the x-side wins
    else if (rc > 0)
        // This shouldn't happen; the tod clock we have just obtained is
        // earlier than the tod clock in the message we recently received.
        // Presumably it wasn't a tod clock, or it was manipulated somehow.
        *fxSideWins = FALSE;    // i.e. the y-side wins
    else
    {
        // This shouldn't happen; the tod clock we have just obtained is
        // equal to the tod clock in the message we recently received.
        // Perhaps Hercules hasn't updated the tod clock for ages, though
        // that seems unlikely, so assume it was manipulated somehow.
        FETCH_DW( uTod, pMPC_PUSre[1]->vc.pus_02.a.clock );  // get x-side's time
        uTod += 0x0000000000000001;                          // Add a tiny amount
        STORE_DW( pMPC_PUSre[1]->vc.pus_02.a.clock, uTod );  // set x-side's time
        *fxSideWins = TRUE;                                  // the x-side wins
    }

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x417E (Issuer) PUK 0x4102 (CM_ENABLE)" );
    }

    return pPTPHDRre;
}   /* End function  build_417E_cm_enable() */

/* ------------------------------------------------------------------ */
/* build_417E_cm_setup()                                            */
/* ------------------------------------------------------------------ */
// Build RRH 0x417E PUK 0x4104 to yTokenIssuerRm

PTPHDR*  build_417E_cm_setup( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    PTPHDR*    pPTPHDRre;       // PTPHDR to be read
    MPC_TH*    pMPC_THre;       // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;      // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;       // MPC_PH follows MPC_RRH
    MPC_PUK*   pMPC_PUKre;      // MPC_PUK follows MPC_PH
    MPC_PUS*   pMPC_PUSre[3];   // MPC_PUSs follow MPC_PUK


    // Allocate a buffer.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04 +                  // first MPC_PUS (0x0404)
               SIZE_PUS_05 +                  // second MPC_PUS (0x0405)
               SIZE_PUS_06;                   // third MPC_PUS (0x0406)
    uLength3 = SIZE_PUK + uLength4;           // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PUKre = (MPC_PUK*)((BYTE*)pMPC_PHre + SIZE_PH);
    pMPC_PUSre[0] = (MPC_PUS*)((BYTE*)pMPC_PUKre + SIZE_PUK);
    pMPC_PUSre[1] = (MPC_PUS*)((BYTE*)pMPC_PUSre[0] + SIZE_PUS_04);
    pMPC_PUSre[2] = (MPC_PUS*)((BYTE*)pMPC_PUSre[1] + SIZE_PUS_05);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_ULP;
    pMPC_RRHre->proto = PROTOCOL_UNKNOWN;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_FW( pMPC_RRHre->seqnum, ++pPTPBLK->uSeqNumIssuer );
    memcpy( pMPC_RRHre->ackseq, pMPC_RRHwr->seqnum, 4 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenIssuerRm, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PUKre
    STORE_HW( pMPC_PUKre->length, SIZE_PUK );
    pMPC_PUKre->what = PUK_WHAT_41;
    pMPC_PUKre->type = PUK_TYPE_SETUP;
    STORE_HW( pMPC_PUKre->lenpus, uLength4 );

    // Prepare first MPC_PUSre
    STORE_HW( pMPC_PUSre[0]->length, SIZE_PUS_04 );
    pMPC_PUSre[0]->what = PUS_WHAT_04;
    pMPC_PUSre[0]->type = PUS_TYPE_04;
    pMPC_PUSre[0]->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[0]->vc.pus_04.token, pPTPBLK->xTokenCmConnection, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUSre
    STORE_HW( pMPC_PUSre[1]->length, SIZE_PUS_05 );
    pMPC_PUSre[1]->what = PUS_WHAT_04;
    pMPC_PUSre[1]->type = PUS_TYPE_05;
    pMPC_PUSre[1]->vc.pus_05.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[1]->vc.pus_05.token, pPTPBLK->yTokenCmFilter, MPC_TOKEN_LENGTH );

    // Prepare third MPC_PUSre
    STORE_HW( pMPC_PUSre[2]->length, SIZE_PUS_06 );
    pMPC_PUSre[2]->what = PUS_WHAT_04;
    pMPC_PUSre[2]->type = PUS_TYPE_06;
    pMPC_PUSre[2]->vc.pus_06.unknown04[0] = 0xC8;        // !!! //

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x417E (Issuer) PUK 0x4104 (CM_SETUP)" );
    }

    return pPTPHDRre;
}   /* End function  build_417E_cm_setup() */

/* ------------------------------------------------------------------ */
/* build_417E_cm_confirm()                                            */
/* ------------------------------------------------------------------ */
// Build RRH 0x417E PUK 0x4106 to yTokenIssuerRm

PTPHDR*  build_417E_cm_confirm( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    PTPHDR*    pPTPHDRre;       // PTPHDR to be read
    MPC_TH*    pMPC_THre;       // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;      // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;       // MPC_PH follows MPC_RRH
    MPC_PUK*   pMPC_PUKre;      // MPC_PUK follows MPC_PH
    MPC_PUS*   pMPC_PUSre[3];   // MPC_PUSs follow MPC_PUK


    // Allocate a buffer.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04 +                  // first MPC_PUS (0x0404)
               SIZE_PUS_08 +                  // second MPC_PUS (0x0408)
               SIZE_PUS_07;                   // third MPC_PUS (0x0407)
    uLength3 = SIZE_PUK + uLength4;           // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PUKre = (MPC_PUK*)((BYTE*)pMPC_PHre + SIZE_PH);
    pMPC_PUSre[0] = (MPC_PUS*)((BYTE*)pMPC_PUKre + SIZE_PUK);
    pMPC_PUSre[1] = (MPC_PUS*)((BYTE*)pMPC_PUSre[0] + SIZE_PUS_04);
    pMPC_PUSre[2] = (MPC_PUS*)((BYTE*)pMPC_PUSre[1] + SIZE_PUS_08);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_ULP;
    pMPC_RRHre->proto = PROTOCOL_UNKNOWN;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_FW( pMPC_RRHre->seqnum, ++pPTPBLK->uSeqNumIssuer );
    memcpy( pMPC_RRHre->ackseq, pMPC_RRHwr->seqnum, 4 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenIssuerRm, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PUKre
    STORE_HW( pMPC_PUKre->length, SIZE_PUK );
    pMPC_PUKre->what = PUK_WHAT_41;
    pMPC_PUKre->type = PUK_TYPE_CONFIRM;
    STORE_HW( pMPC_PUKre->lenpus, uLength4 );

    // Prepare first MPC_PUSre
    STORE_HW( pMPC_PUSre[0]->length, SIZE_PUS_04 );
    pMPC_PUSre[0]->what = PUS_WHAT_04;
    pMPC_PUSre[0]->type = PUS_TYPE_04;
    pMPC_PUSre[0]->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[0]->vc.pus_04.token, pPTPBLK->yTokenCmConnection, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUSre
    STORE_HW( pMPC_PUSre[1]->length, SIZE_PUS_08 );
    pMPC_PUSre[1]->what = PUS_WHAT_04;
    pMPC_PUSre[1]->type = PUS_TYPE_08;
    pMPC_PUSre[1]->vc.pus_08.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[1]->vc.pus_08.token, pPTPBLK->xTokenCmFilter, MPC_TOKEN_LENGTH );

    // Prepare third MPC_PUSre
    STORE_HW( pMPC_PUSre[2]->length, SIZE_PUS_07 );
    pMPC_PUSre[2]->what = PUS_WHAT_04;
    pMPC_PUSre[2]->type = PUS_TYPE_07;
    pMPC_PUSre[2]->vc.pus_07.unknown04[0] = 0xC8;        // !!! //

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x417E (Issuer) PUK 0x4106 (CM_CONFIRM)" );
    }

    return pPTPHDRre;
}   /* End function  build_417E_cm_confirm() */

/* ------------------------------------------------------------------ */
/* build_417E_ulp_enable()                                            */
/* ------------------------------------------------------------------ */
// Build RRH 0x417E PUK 0x4102 to yTokenCmConnection

PTPHDR*  build_417E_ulp_enable( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRHwr,
                               MPC_PUS* pMPC_PUSwr, u_int* fxSideWins )
{

    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    PTPHDR*    pPTPHDRre;       // PTPHDR to be read
    MPC_TH*    pMPC_THre;       // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;      // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;       // MPC_PH follows MPC_RRH
    MPC_PUK*   pMPC_PUKre;      // MPC_PUK follows MPC_PH
    MPC_PUS*    pMPC_PUSre[2];  // MPC_PUSs follow MPC_PUK
    int        rc;

    UNREFERENCED( pMPC_RRHwr );


    // Allocate a buffer in which the response will be build.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_01 +                  // first MPC_PUS (0x0401)
               SIZE_PUS_02_B;                 // second MPC_PUS (0x0402)
    uLength3 = SIZE_PUK + uLength4;           // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PUKre = (MPC_PUK*)((BYTE*)pMPC_PHre + SIZE_PH);
    pMPC_PUSre[0] = (MPC_PUS*)((BYTE*)pMPC_PUKre + SIZE_PUK);
    pMPC_PUSre[1] = (MPC_PUS*)((BYTE*)pMPC_PUSre[0] + SIZE_PUS_01);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_ULP;
    pMPC_RRHre->proto = PROTOCOL_UNKNOWN;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_FW( pMPC_RRHre->seqnum, ++pPTPBLK->uSeqNumCm );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenCmConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PUKre
    STORE_HW( pMPC_PUKre->length, SIZE_PUK );
    pMPC_PUKre->what = PUK_WHAT_41;
    pMPC_PUKre->type = PUK_TYPE_ENABLE;
    STORE_HW( pMPC_PUKre->lenpus, uLength4 );

    // Prepare first MPC_PUSre
    STORE_HW( pMPC_PUSre[0]->length, SIZE_PUS_01 );
    pMPC_PUSre[0]->what = PUS_WHAT_04;
    pMPC_PUSre[0]->type = PUS_TYPE_01;
    pMPC_PUSre[0]->vc.pus_01.proto = PROTOCOL_LAYER2;
    pMPC_PUSre[0]->vc.pus_01.unknown05 = 0x01;           // !!! //
    pMPC_PUSre[0]->vc.pus_01.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[0]->vc.pus_01.token, pPTPBLK->xTokenUlpFilter, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUSre
    // Note: the 40-bytes placed in the second MPC_PUS are important.
    // Whichever side has the lowest value 'wins', and dictates the
    // 'handedness' of the RRH 0x417E exchanges. If this code 'wins'
    // and then acts like a 'loser', confusion reigns.
    STORE_HW( pMPC_PUSre[1]->length, SIZE_PUS_02_B );
    pMPC_PUSre[1]->what = PUS_WHAT_04;
    pMPC_PUSre[1]->type = PUS_TYPE_02;
    pMPC_PUSre[1]->vc.pus_02.b.unknown04 = 0x02;         // !!! //
    pMPC_PUSre[1]->vc.pus_02.b.flags = 0x90;         // !!! //
    pMPC_PUSre[1]->vc.pus_02.b.unknown0A = 0x40;         // !!! //
#if defined(ENABLE_IPV6)
    if (pPTPBLK->fIPv4Spec)
    {
#endif /* defined(ENABLE_IPV6) */
        memcpy( pMPC_PUSre[1]->vc.pus_02.b.ipaddr, &pPTPBLK->iaDriveIPAddr4, 4 );
#if defined(ENABLE_IPV6)
    }
    else
    {
        pMPC_PUSre[1]->vc.pus_02.b.flags |= 0x08;
        memcpy( pMPC_PUSre[1]->vc.pus_02.b.ipaddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    }
#endif /* defined(ENABLE_IPV6) */

    // Compare the IP address in the MPC_PUSwr with the IP address in
    // the second MPC_PUSre to determine which side wins. First, check
    // whether both sides are using the same variety of IP address.
    if (( pMPC_PUSwr->vc.pus_02.b.flags & 0x08 ) == ( pMPC_PUSre[1]->vc.pus_02.b.flags & 0x08 ))
    {
        // Both sides are using the same variety of IP address.
        rc = memcmp( &pMPC_PUSwr->vc.pus_02.b.ipaddr, &pMPC_PUSre[1]->vc.pus_02.b.ipaddr, 16 );
        if (rc < 0)
            // The y-side's IP address is lower than the x-side's.
            *fxSideWins = TRUE;     // i.e. the x-side wins
        else if (rc > 0)
            // The y-side's IP address is higher than the x-side's.
            *fxSideWins = FALSE;    // i.e. the y-side wins
        else
            // This shouldn't happen; the y-side and the x-side have the
            // same IP address! Empirical evidence suggests that in
            // these circumstances each side believes that it has the
            // lower address. As a result communication between them
            // stalls, with each side waiting for the other side to send
            // the next message. Presumably the VTAM coders didn't think
            // anyone would be daft enough to give both sides the same
            // IP address, or possibly they didn't think of it at all.
            // Until a stop command is issued on the y-side nothing else
            // will happen. However, to prevent that unhappy situation
            // we will deem ourselves the winner.
            *fxSideWins = TRUE;     // i.e. the x-side wins
    }
    else
    {
        // One side is using an IPv4 address, the other an IPv6 address.
        // This is normal behaviour when one side is starting both the
        // IPv4 and IPv6 connections, and the other is only starting the
        // IPv6 connection. The side that is using the IPv4 address is
        // the winner. Check the y-side's variety of IP address.
        if (( pMPC_PUSwr->vc.pus_02.b.flags & 0x08 ) == 0x08)
            *fxSideWins = TRUE;     // i.e. the x-side wins, it's using IPv4
        else
            *fxSideWins = FALSE;    // i.e. the y-side wins, it's using IPv4
    }

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x417E (CmComm) PUK 0x4102 (ULP_ENABLE)" );
    }

    return pPTPHDRre;
}   /* End function  build_417E_ulp_enable() */

/* ------------------------------------------------------------------ */
/* build_417E_ulp_setup()                                            */
/* ------------------------------------------------------------------ */
// Build RRH 0x417E PUK 0x4104 to yTokenCmConnection

PTPHDR*  build_417E_ulp_setup( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    PTPHDR*    pPTPHDRre;       // PTPHDR to be read
    MPC_TH*    pMPC_THre;       // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;      // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;       // MPC_PH follows MPC_RRH
    MPC_PUK*   pMPC_PUKre;      // MPC_PUK follows MPC_PH
    MPC_PUS*   pMPC_PUSre[4];   // MPC_PUSs follow MPC_PUK


    // Allocate a buffer in which the response will be build.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04 +                  // first MPC_PUS (0x0404)
               SIZE_PUS_05 +                  // second MPC_PUS (0x0405)
               SIZE_PUS_06 +                  // third MPC_PUS (0x0406)
               SIZE_PUS_02_B;                 // fourth MPC_PUS (0x0402)
    uLength3 = SIZE_PUK + uLength4;           // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PUKre = (MPC_PUK*)((BYTE*)pMPC_PHre + SIZE_PH);
    pMPC_PUSre[0] = (MPC_PUS*)((BYTE*)pMPC_PUKre + SIZE_PUK);
    pMPC_PUSre[1] = (MPC_PUS*)((BYTE*)pMPC_PUSre[0] + SIZE_PUS_04);
    pMPC_PUSre[2] = (MPC_PUS*)((BYTE*)pMPC_PUSre[1] + SIZE_PUS_05);
    pMPC_PUSre[3] = (MPC_PUS*)((BYTE*)pMPC_PUSre[2] + SIZE_PUS_06);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_ULP;
    pMPC_RRHre->proto = PROTOCOL_UNKNOWN;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_FW( pMPC_RRHre->seqnum, ++pPTPBLK->uSeqNumCm );
    memcpy( pMPC_RRHre->ackseq, pMPC_RRHwr->seqnum, 4 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenCmConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PUKre
    STORE_HW( pMPC_PUKre->length, SIZE_PUK );
    pMPC_PUKre->what = PUK_WHAT_41;
    pMPC_PUKre->type = PUK_TYPE_SETUP;
    STORE_HW( pMPC_PUKre->lenpus, uLength4 );

    // Prepare first MPC_PUSre
    STORE_HW( pMPC_PUSre[0]->length, SIZE_PUS_04 );
    pMPC_PUSre[0]->what = PUS_WHAT_04;
    pMPC_PUSre[0]->type = PUS_TYPE_04;
    pMPC_PUSre[0]->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[0]->vc.pus_04.token, pPTPBLK->xTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUSre
    STORE_HW( pMPC_PUSre[1]->length, SIZE_PUS_05 );
    pMPC_PUSre[1]->what = PUS_WHAT_04;
    pMPC_PUSre[1]->type = PUS_TYPE_05;
    pMPC_PUSre[1]->vc.pus_05.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[1]->vc.pus_05.token, pPTPBLK->yTokenUlpFilter, MPC_TOKEN_LENGTH );

    // Prepare third MPC_PUSre
    STORE_HW( pMPC_PUSre[2]->length, SIZE_PUS_06 );
    pMPC_PUSre[2]->what = PUS_WHAT_04;
    pMPC_PUSre[2]->type = PUS_TYPE_06;
    pMPC_PUSre[2]->vc.pus_06.unknown04[0] = 0x40;        // !!! //

    // Prepare fourth MPC_PUSre
    STORE_HW( pMPC_PUSre[3]->length, SIZE_PUS_02_B );
    pMPC_PUSre[3]->what = PUS_WHAT_04;
    pMPC_PUSre[3]->type = PUS_TYPE_02;
    pMPC_PUSre[3]->vc.pus_02.b.unknown04 = 0x02;         // !!! //
    pMPC_PUSre[3]->vc.pus_02.b.flags = 0x90;         // !!! //
    pMPC_PUSre[3]->vc.pus_02.b.unknown0A = 0x40;         // !!! //
#if defined(ENABLE_IPV6)
    if (pPTPBLK->fIPv4Spec)
    {
#endif /* defined(ENABLE_IPV6) */
        memcpy( pMPC_PUSre[3]->vc.pus_02.b.ipaddr, &pPTPBLK->iaDriveIPAddr4, 4 );
#if defined(ENABLE_IPV6)
    }
    else
    {
        pMPC_PUSre[1]->vc.pus_02.b.flags |= 0x08;
        memcpy( pMPC_PUSre[3]->vc.pus_02.b.ipaddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    }
#endif /* defined(ENABLE_IPV6) */

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x417E (CmComm) PUK 0x4104 (ULP_SETUP)" );
    }

    return pPTPHDRre;
}   /* End function  build_417E_ulp_setup() */

/* ------------------------------------------------------------------ */
/* build_417E_ulp_confirm()                                            */
/* ------------------------------------------------------------------ */
// Build RRH 0x417E PUK 0x4106 to yTokenCmConnection

PTPHDR*  build_417E_ulp_confirm( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    PTPHDR*    pPTPHDRre;       // PTPHDR to be read
    MPC_TH*    pMPC_THre;       // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;      // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;       // MPC_PH follows MPC_RRH
    MPC_PUK*   pMPC_PUKre;      // MPC_PUK follows MPC_PH
    MPC_PUS*   pMPC_PUSre[4];   // MPC_PUSs follow MPC_PUK


    // Allocate a buffer.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04 +                  // first MPC_PUS (0x0404)
               SIZE_PUS_08 +                  // second MPC_PUS (0x0408)
               SIZE_PUS_07 +                  // third MPC_PUS (0x0407)
               SIZE_PUS_02_B;                 // fourth MPC_PUS (0x0402)
    uLength3 = SIZE_PUK + uLength4;           // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PUKre = (MPC_PUK*)((BYTE*)pMPC_PHre + SIZE_PH);
    pMPC_PUSre[0] = (MPC_PUS*)((BYTE*)pMPC_PUKre + SIZE_PUK);
    pMPC_PUSre[1] = (MPC_PUS*)((BYTE*)pMPC_PUSre[0] + SIZE_PUS_04);
    pMPC_PUSre[2] = (MPC_PUS*)((BYTE*)pMPC_PUSre[1] + SIZE_PUS_08);
    pMPC_PUSre[3] = (MPC_PUS*)((BYTE*)pMPC_PUSre[2] + SIZE_PUS_07);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_ULP;
    pMPC_RRHre->proto = PROTOCOL_UNKNOWN;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_FW( pMPC_RRHre->seqnum, ++pPTPBLK->uSeqNumCm );
    memcpy( pMPC_RRHre->ackseq, pMPC_RRHwr->seqnum, 4 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenCmConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PUKre
    STORE_HW( pMPC_PUKre->length, SIZE_PUK );
    pMPC_PUKre->what = PUK_WHAT_41;
    pMPC_PUKre->type = PUK_TYPE_CONFIRM;
    STORE_HW( pMPC_PUKre->lenpus, uLength4 );

    // Prepare first MPC_PUSre
    STORE_HW( pMPC_PUSre[0]->length, SIZE_PUS_04 );
    pMPC_PUSre[0]->what = PUS_WHAT_04;
    pMPC_PUSre[0]->type = PUS_TYPE_04;
    pMPC_PUSre[0]->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[0]->vc.pus_04.token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUSre
    STORE_HW( pMPC_PUSre[1]->length, SIZE_PUS_08 );
    pMPC_PUSre[1]->what = PUS_WHAT_04;
    pMPC_PUSre[1]->type = PUS_TYPE_08;
    pMPC_PUSre[1]->vc.pus_08.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre[1]->vc.pus_08.token, pPTPBLK->xTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare third MPC_PUSre
    STORE_HW( pMPC_PUSre[2]->length, SIZE_PUS_07 );
    pMPC_PUSre[2]->what = PUS_WHAT_04;
    pMPC_PUSre[2]->type = PUS_TYPE_07;
    pMPC_PUSre[2]->vc.pus_07.unknown04[0] = 0x40;        // !!! //

    // Prepare fourth MPC_PUSre
    STORE_HW( pMPC_PUSre[3]->length, SIZE_PUS_02_B );
    pMPC_PUSre[3]->what = PUS_WHAT_04;
    pMPC_PUSre[3]->type = PUS_TYPE_02;
    pMPC_PUSre[3]->vc.pus_02.b.unknown04 = 0x02;         // !!! //
    pMPC_PUSre[3]->vc.pus_02.b.flags = 0x90;         // !!! //
    pMPC_PUSre[3]->vc.pus_02.b.unknown0A = 0x40;         // !!! //
#if defined(ENABLE_IPV6)
    if (pPTPBLK->fIPv4Spec)
    {
#endif /* defined(ENABLE_IPV6) */
        memcpy( pMPC_PUSre[3]->vc.pus_02.b.ipaddr, &pPTPBLK->iaDriveIPAddr4, 4 );
#if defined(ENABLE_IPV6)
    }
    else
    {
        pMPC_PUSre[1]->vc.pus_02.b.flags |= 0x08;
        memcpy( pMPC_PUSre[3]->vc.pus_02.b.ipaddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    }
#endif /* defined(ENABLE_IPV6) */

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x417E (CmComm) PUK 0x4106 (ULP_CONFIRM)" );
    }

    return pPTPHDRre;
}   /* End function  build_417E_ulp_confirm() */

/* ------------------------------------------------------------------ */
/* build_417E_dm_act()                                            */
/* ------------------------------------------------------------------ */
// Build RRH 0x417E PUK 0x4360 to yTokenCmConnection

PTPHDR*  build_417E_dm_act( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    PTPHDR*    pPTPHDRre;      // PTPHDR to be read
    MPC_TH*    pMPC_THre;      // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;     // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;      // MPC_PH follows MPC_RRH
    MPC_PUK*   pMPC_PUKre;     // MPC_PUK follows MPC_PH
    MPC_PUS*   pMPC_PUSre;     // MPC_PUS follows MPC_PUK


    // Allocate a buffer in which the response will be build.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04;                    // the MPC_PUS
    uLength3 = SIZE_PUK + uLength4;            // the MPC_PUK and MPC_PUS (the data)
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;   // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;            // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PUKre = (MPC_PUK*)((BYTE*)pMPC_PHre + SIZE_PH);
    pMPC_PUSre = (MPC_PUS*)((BYTE*)pMPC_PUKre + SIZE_PUK);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_ULP;
    pMPC_RRHre->proto = PROTOCOL_UNKNOWN;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_FW( pMPC_RRHre->seqnum, ++pPTPBLK->uSeqNumCm );
    memcpy( pMPC_RRHre->ackseq, pMPC_RRHwr->seqnum, 4 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenCmConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PUKre
    STORE_HW( pMPC_PUKre->length, SIZE_PUK );
    pMPC_PUKre->what = PUK_WHAT_43;
    pMPC_PUKre->type = PUK_TYPE_ACTIVE;
    STORE_HW( pMPC_PUKre->lenpus, uLength4 );

    // Prepare first MPC_PUSre
    STORE_HW( pMPC_PUSre->length, SIZE_PUS_04 );
    pMPC_PUSre->what = PUS_WHAT_04;
    pMPC_PUSre->type = PUS_TYPE_04;
    pMPC_PUSre->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_PUSre->vc.pus_04.token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x417E (CmComm) PUK 0x4360 (DM_ACT)" );
    }

    return pPTPHDRre;
}   /* End function  build_417E_dm_act() */


/* ------------------------------------------------------------------ */
/* write_rrh_C17E()                                                   */
/* ------------------------------------------------------------------ */
// Note - the Token is xTokenIssuerRm (in all cases that have been seen).
// In all cases that have been seen the MPC_RRH type 0xC17E is followed
// by a single MPC_PH, which is followed by a single MPC_PUK, which is
// followed by two MPC_PUSs.

int   write_rrh_C17E( DEVBLK* pDEVBLK, MPC_TH* pMPC_THwr, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH   = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK   = pPTPATH->pPTPBLK;
    MPC_PH*    pMPC_PHwr;                      // MPC_PH being written
    MPC_PUK*   pMPC_PUKwr;                     // MPC_PUK being written
//  MPC_PUS*   pMPC_PUSwr;                     // MPC_PUSs being written
    U16        uOffPH;
//  U32        uLenData;
    U32        uOffData;
    int        iWhat;
#define PUK_4501      1


    // Point to the MPC_PH.
    FETCH_HW( uOffPH, pMPC_RRHwr->offph );
    pMPC_PHwr = (MPC_PH*)((BYTE*)pMPC_RRHwr + uOffPH);

    // Get the length of and point to the data referenced by the
    // MPC_PH. The data contain a MPC_PUK and one or more MPC_PUSs.
//  FETCH_F3( uLenData, pMPC_PH->lendata );
    FETCH_FW( uOffData, pMPC_PHwr->offdata );
    pMPC_PUKwr = (MPC_PUK*)((BYTE*)pMPC_THwr + uOffData);

    // Decide what the PUK contains.
    iWhat = UNKNOWN_PUK;
    if (memcmp( pMPC_RRHwr->token, pPTPBLK->xTokenIssuerRm, MPC_TOKEN_LENGTH ) == 0)
    {
        if (pMPC_PUKwr->what == PUK_WHAT_45)
        {
            if (pMPC_PUKwr->type == PUK_TYPE_01)
            {
                iWhat = PUK_4501;
            }
        }
    }

    // Process the PUK.
    switch( iWhat )
    {

    // PUK 0x4501 to xTokenIssuerRm
    // The MPC_PUK should be followed by two MPC_PUSs, the first a type
    // 0x0409 and the second a type 0x0404.
    case PUK_4501:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC17E (Issuer)" );
        }

        break;

    // Unknown PUK
    default:

        // HHC03936 "%1d:%04X PTP: Accept data contains unknown %s"
        WRMSG(HHC03936, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PUK" );
        mpc_display_rrh_and_puk( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );

        break;

    }  /* switch( iWhat ) */

    return 0;
}   /* End function  write_rrh_C17E() */


/* ------------------------------------------------------------------ */
/* write_rrh_C108()                                                   */
/* ------------------------------------------------------------------ */
// Note - the Token is xTokenUlpConnection.
// In all cases that have been seen the MPC_RRH type 0xC108 is followed
// by a single MPC_PH, which is followed by a single MPC_PIX.

int   write_rrh_C108( DEVBLK* pDEVBLK, MPC_TH* pMPC_THwr, MPC_RRH* pMPC_RRHwr )
{
    PTPATH*    pPTPATH   = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK   = pPTPATH->pPTPBLK;
    PTPATH*    pPTPATHre = pPTPBLK->pPTPATHRead;
    MPC_PH*    pMPC_PHwr;
    MPC_PIX*   pMPC_PIXwr;
    U32        uOffData;
//  U32        uLenData;
    U16        uOffPH;
#if defined(ENABLE_IPV6)
    u_int      fLL;
    struct in6_addr addr6;
#endif /* defined(ENABLE_IPV6) */
    struct in_addr  addr4;
    char       cIPaddr[48];
    PTPHDR*    pPTPHDRr1;
    PTPHDR*    pPTPHDRr2;
#if defined(ENABLE_IPV6)
    PTPHDR*    pPTPHDRr3;
#endif /* defined(ENABLE_IPV6) */
    int        iWhat;
#define UNKNOWN_PIX          0
#define WILL_YOU_START_IPV4  1
#define WILL_YOU_START_IPV6  2
#define I_WILL_START_IPV4    3
#define I_WILL_START_IPV6    4
#define MY_ADDRESS_IPV4      5
#define MY_ADDRESS_IPV6      6
#define YOUR_ADDRESS_IPV4    7
#define YOUR_ADDRESS_IPV6    8
#define WILL_YOU_STOP_IPV4   9
#define WILL_YOU_STOP_IPV6   10
#define I_WILL_STOP_IPV4     11
#define I_WILL_STOP_IPV6     12


    // Point to the MPC_PH.
    FETCH_HW( uOffPH, pMPC_RRHwr->offph );
    pMPC_PHwr = (MPC_PH*)((BYTE*)pMPC_RRHwr + uOffPH);

    // Get the length of and point to the data referenced by the
    // MPC_PH. The data is a MPC_PIX.
//  FETCH_F3( uLenData, pMPC_PH->lendata );
    FETCH_FW( uOffData, pMPC_PHwr->offdata );
    pMPC_PIXwr = (MPC_PIX*)((BYTE*)pMPC_THwr + uOffData);

    // Decide what the PIX contains.
    iWhat = UNKNOWN_PIX;
    if (pMPC_PIXwr->action == PIX_START)
    {
        if (pMPC_PIXwr->askans == PIX_ASK)
        {
            if (pMPC_PIXwr->iptype == PIX_IPV4)
                iWhat = WILL_YOU_START_IPV4;
            else if (pMPC_PIXwr->iptype == PIX_IPV6)
                iWhat = WILL_YOU_START_IPV6;
        }
        else if (pMPC_PIXwr->askans == PIX_ANSWER)
        {
            if (pMPC_PIXwr->iptype == PIX_IPV4)
                iWhat = I_WILL_START_IPV4;
            else if (pMPC_PIXwr->iptype == PIX_IPV6)
                iWhat = I_WILL_START_IPV6;
        }
    }
    else if (pMPC_PIXwr->action == PIX_ADDRESS)
    {
        if (pMPC_PIXwr->askans == PIX_ASK)
        {
            if (pMPC_PIXwr->iptype == PIX_IPV4)
                iWhat = MY_ADDRESS_IPV4;
            else if (pMPC_PIXwr->iptype == PIX_IPV6)
                iWhat = MY_ADDRESS_IPV6;
        }
        if (pMPC_PIXwr->askans == PIX_ANSWER)
        {
            if (pMPC_PIXwr->iptype == PIX_IPV4)
                iWhat = YOUR_ADDRESS_IPV4;
            else if (pMPC_PIXwr->iptype == PIX_IPV6)
                iWhat = YOUR_ADDRESS_IPV6;
        }
    }
    else if (pMPC_PIXwr->action == PIX_STOP)
    {
        if (pMPC_PIXwr->askans == PIX_ASK)
        {
            if (pMPC_PIXwr->iptype == PIX_IPV4)
                iWhat = WILL_YOU_STOP_IPV4;
            else if (pMPC_PIXwr->iptype == PIX_IPV6)
                iWhat = WILL_YOU_STOP_IPV6;
        }
        else if (pMPC_PIXwr->askans == PIX_ANSWER)
        {
            if (pMPC_PIXwr->iptype == PIX_IPV4)
                iWhat = I_WILL_STOP_IPV4;
            else if (pMPC_PIXwr->iptype == PIX_IPV6)
                iWhat = I_WILL_STOP_IPV6;
        }
    }

    // Process what the PIX contains.
    switch( iWhat )
    {

    case WILL_YOU_START_IPV4:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) Will you start IPv4?" );
        }

        // Remember the activation status.
        pPTPBLK->bActivate4 |= HEASKEDME_START;

        // Build RRH 0xC108 PIX 0x0101 to yTokenUlpConnection (I will start IPv4)
        pPTPHDRr1 = build_C108_i_will_start_4( pDEVBLK, pMPC_PIXwr, 0 );
        if (!pPTPHDRr1)
            break;

        // Remember the activation status.
        pPTPBLK->bActivate4 |= IANSWEREDHIM_START;

        // Add PTPHDRs to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );

        // We have said we will start IPv4, but if IPv4 was not specified
        // that's all we will do. The y-side will wait patiently for our
        // will you start IPv4, which will never arrive. He's quite happy,
        // he just assumes the device/link has not been started on this side
        // and might be at sometime in the future.
        if (pPTPBLK->fIPv4Spec)
        {
            for( ; ; )
            {

                // Build RRH 0xC108 PIX 0x0180 to yTokenUlpConnection (Will you start IPv4?)
                pPTPHDRr1 = build_C108_will_you_start_4( pDEVBLK );
                if (!pPTPHDRr1)
                {
                    break;
                }

                // Build RRH 0xC108 PIX 0x1180 to yTokenUlpConnection (My address IPv4)
                pPTPHDRr2 = build_C108_my_address_4( pDEVBLK );
                if (!pPTPHDRr2)
                {
                    free( pPTPHDRr1 );
                    break;
                }

                // Remember the activation status.
                pPTPBLK->bActivate4 |= IASKEDHIM_START;
                pPTPBLK->bActivate4 |= ITOLDHIMMY_ADDRESS;

                // Add PTPHDRs to chain.
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr2 );

                break;
            }
        }

        break;

    case WILL_YOU_START_IPV6:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) Will you start IPv6?" );
        }

        // Remember the activation status.
        pPTPBLK->bActivate6 |= HEASKEDME_START;
        pPTPBLK->bActivateLL6 |= HEASKEDME_START;

        // Build RRH 0xC108 PIX 0x0101 to yTokenUlpConnection (I will start IPv6)
        pPTPHDRr1 = build_C108_i_will_start_6( pDEVBLK, pMPC_PIXwr, 0 );
        if (!pPTPHDRr1)
            break;

        // Remember the activation status.
        pPTPBLK->bActivate6 |= IANSWEREDHIM_START;
        pPTPBLK->bActivateLL6 |= IANSWEREDHIM_START;

        // Add PTPHDR to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );

#if defined(ENABLE_IPV6)
        // We have said we will start IPv6, but if IPv6 was not specified
        // that's all we will do. The y-side will wait patiently for our
        // will you start IPv6, which will never arrive. He's quite happy,
        // he just assumes the interface has not been started on this side
        // and might be at sometime in the future.
        if (pPTPBLK->fIPv6Spec)
        {
            for( ; ; )
            {

                // Build RRH 0xC108 PIX 0x0180 to yTokenUlpConnection (Will you start IPv6?)
                pPTPHDRr1 = build_C108_will_you_start_6( pDEVBLK );
                if (!pPTPHDRr1)
                    break;

                // Build RRH 0xC108 PIX 0x1180 to yTokenUlpConnection (My address IPv6)
                pPTPHDRr2 = build_C108_my_address_6( pDEVBLK, TRUE );  // Link local
                if (!pPTPHDRr2)
                {
                    free( pPTPHDRr1 );
                    break;
                }

                // Build RRH 0xC108 PIX 0x1180 to yTokenUlpConnection (My address IPv6)
                pPTPHDRr3 = build_C108_my_address_6( pDEVBLK, FALSE );
                if (!pPTPHDRr3)
                {
                    free( pPTPHDRr2 );
                    free( pPTPHDRr1 );
                    break;
                }

                // Remember the activation status.
                pPTPBLK->bActivate6 |= IASKEDHIM_START;
                pPTPBLK->bActivateLL6 |= IASKEDHIM_START;
                pPTPBLK->bActivate6 |= ITOLDHIMMY_ADDRESS;
                pPTPBLK->bActivateLL6 |= ITOLDHIMMY_ADDRESS;

                // Add PTPHDRs to chain.
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr2 );
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr3 );

                break;
            }
        }
#endif /* defined(ENABLE_IPV6) */

        break;

    case I_WILL_START_IPV4:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) I will start IPv4" );
        }

        // Remember the activation status.
        pPTPBLK->bActivate4 |= HEANSWEREDME_START;

        // Check whether the connection is active.
        if (pPTPBLK->bActivate4 == WEAREACTIVE)
        {
            // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
            WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestIPAddr4 );
            pPTPBLK->fActive4 = TRUE;
            pPTPBLK->bActivate4 = 0x00;
            pPTPBLK->bTerminate4 = 0x00;
        }

        break;

    case I_WILL_START_IPV6:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) I will start IPv6" );
        }

        // Remember the activation status.
        pPTPBLK->bActivate6 |= HEANSWEREDME_START;
        pPTPBLK->bActivateLL6 |= HEANSWEREDME_START;

#if defined(ENABLE_IPV6)
        // Check whether the connection is active.
        if (pPTPBLK->bActivateLL6 == WEAREACTIVE)
        {
            // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
            WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestLLAddr6 );
            pPTPBLK->fActiveLL6 = TRUE;
            pPTPBLK->bActivateLL6 = 0x00;
            pPTPBLK->bTerminateLL6 = 0x00;
            if (pPTPBLK->fActive6)
            {
                build_8108_icmpv6_packets( pDEVBLK );
            }
        }
        if (pPTPBLK->bActivate6 == WEAREACTIVE)
        {
            // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
            WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestIPAddr6 );
            pPTPBLK->fActive6 = TRUE;
            pPTPBLK->bActivate6 = 0x00;
            pPTPBLK->bTerminate6 = 0x00;
            if (pPTPBLK->fActiveLL6)
            {
                build_8108_icmpv6_packets( pDEVBLK );
            }
        }
#endif /* defined(ENABLE_IPV6) */

        break;

    case MY_ADDRESS_IPV4:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
          mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) My address IPv4" );
        }

        // The y-side is telling us about his IPv4 address, which means
        // IPv4 is defined in the guest and that the device/link is started.
        // However, if IPv4 was not specified on the config statement, or
        // the pre-configured TUN interface, we are not interested. We will
        // respond to his message to keep him happy but that is all.
        if (pPTPBLK->fIPv4Spec)
        {
          // Check for the guests IPv4 address.
          if (memcmp( pMPC_PIXwr->ipaddr, &pPTPBLK->iaGuestIPAddr4, 4 ) == 0)
          {
            // The y-side has told us his IPv4 address and it is the
            // guest address specified on the config statement or the
            // pre-configured TUN interface, which is of course good news.
            // Remember the activation status.
            pPTPBLK->bActivate4 |= HETOLDMEHIS_ADDRESS;

            // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv4)
            pPTPHDRr1 = build_C108_your_address_4( pDEVBLK, pMPC_PIXwr, 0 );
            if (!pPTPHDRr1)
                break;

            // Remember the activation status.
            pPTPBLK->bActivate4 |= IANSWEREDHIS_ADDRESS;

            // Add PTPHDR to chain.
            add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
          }
          else
          {
            // Hmm... the y-side has told us his IPv4 address and it wasn't
            // the guest IPv4 address specified on the config statement
            // or the pre-configured TUN interface. This could happen if
            // if have been told the wrong address on the config statement,
            // or we are using a pre-configured TUN interface that did not
            // specify the guest IPv4 address.
            // Perhaps the guest and driver addresses were transposed.
            if (memcmp( pMPC_PIXwr->ipaddr, &pPTPBLK->iaDriveIPAddr4, 4 ) == 0)
            {
                // Looks like the guest and driver IPv4 addresses were transposed.
                hinet_ntop( AF_INET, &pMPC_PIXwr->ipaddr, cIPaddr, sizeof(cIPaddr) );
                // HHC03912 "%1d:%04X PTP: Guest has the driver IP address '%s'"
                WRMSG(HHC03912, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, cIPaddr );

                // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv4)
                pPTPHDRr1 = build_C108_your_address_4( pDEVBLK, pMPC_PIXwr, 12 );
                if (!pPTPHDRr1)
                    break;

                // Add PTPHDR to chain.
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
            }
            else
            {
              // Hmm... the y-side has told us his IPv4 address and it wasn't
              // the guest or driver IPv4 address specified on the config
              // statement or the pre-configured TUN interface.
              // Check whether we are using a pre-configured TUN interface
              // that did not specify the guest IPv4 address, and that we
              // have not already been told an address.
              memset(&addr4, 0, sizeof(addr4));
              if (pPTPBLK->fPreconfigured && !pPTPBLK->fPreGuestIPAddr4 &&
                  ( memcmp( &addr4, &pPTPBLK->iaGuestIPAddr4, sizeof(addr4) ) == 0 )) {

                // We are using a pre-configured TUN interface that didn't
                // specify the guest IPv4 address. Hooray, the y-side has
                // told us his IPv4 address, something we didn't know, but
                // need to. Copy the y-side's IPv4 address.
                memcpy( &pPTPBLK->iaGuestIPAddr4, &pMPC_PIXwr->ipaddr,
                        sizeof(pPTPBLK->iaGuestIPAddr4) );
                hinet_ntop( AF_INET, &pMPC_PIXwr->ipaddr, pPTPBLK->szGuestIPAddr4,
                                                          sizeof(pPTPBLK->szGuestIPAddr4) );

                // Remember the activation status.
                pPTPBLK->bActivate4 |= HETOLDMEHIS_ADDRESS;

                // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv4)
                pPTPHDRr1 = build_C108_your_address_4( pDEVBLK, pMPC_PIXwr, 0 );
                if (!pPTPHDRr1)
                    break;

                // Remember the activation status.
                pPTPBLK->bActivate4 |= IANSWEREDHIS_ADDRESS;

                // Add PTPHDR to chain.
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
              }
              else
              {
                // The guest has an IPv4 address we know nothing about. The guest
                // can only have one IPv4 address associated with a link.
                hinet_ntop( AF_INET, &pMPC_PIXwr->ipaddr, cIPaddr, sizeof(cIPaddr) );
                // HHC03913 "%1d:%04X PTP: Guest has IP address '%s'"
                WRMSG(HHC03913, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                     cIPaddr );

                // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv4)
                pPTPHDRr1 = build_C108_your_address_4( pDEVBLK, pMPC_PIXwr, 0 );
                if (!pPTPHDRr1)
                    break;

                // Add PTPHDR to chain.
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
              }
            }
          }
        }
        else
        {
          // IPv4 was not specified on the config statement or the
          // preconfigured TUN interface, but the guest has informed
          // us of his IPv4 address. Reply but otherwise ignore.

          // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv4)
          pPTPHDRr1 = build_C108_your_address_4( pDEVBLK, pMPC_PIXwr, 0 );
          if (!pPTPHDRr1)
               break;

          // Add PTPHDR to chain.
          add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
        }

        // Check whether the connection is active.
        if (pPTPBLK->bActivate4 == WEAREACTIVE)
        {
            // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
            WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestIPAddr4 );
            pPTPBLK->fActive4 = TRUE;
            pPTPBLK->bActivate4 = 0x00;
            pPTPBLK->bTerminate4 = 0x00;
        }

        break;

    case MY_ADDRESS_IPV6:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) My address IPv6" );
        }

#if defined(ENABLE_IPV6)
        // Check whether the y-side is telling us about a Link Local address.
        fLL = FALSE;
        memset( addr6.s6_addr, 0, 16 );
        addr6.s6_addr[0] = 0xFE;
        addr6.s6_addr[1] = 0x80;
        if (memcmp( pMPC_PIXwr->ipaddr, &addr6, 8 ) == 0)
            fLL = TRUE;

        // The y-side is telling us about his IPv6 address(es), which means
        // IPv6 is defined in the guest and that the interface is started.
        // However, if IPv6 was not specified on the config statement, or
        // the pre-configured TUN interface, we are not interested. We will
        // respond to his message to keep him happy but that is all.
        if (pPTPBLK->fIPv6Spec)
        {
          if (!fLL)
          {
            // The y-side has told us about an address that is not his Link
            // Local address. Check whether the guests IPv6 is the address
            // that was specified on the config statement.
            if (memcmp( pMPC_PIXwr->ipaddr, &pPTPBLK->iaGuestIPAddr6, 16 ) == 0)
            {
              // The y-side has told us his IPv6 address and it is the guest
              // address specified on the config statement, which is of course
              // good news. Remember the activation status.
              pPTPBLK->bActivate6 |= HETOLDMEHIS_ADDRESS;

              // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv6)
              pPTPHDRr1 = build_C108_your_address_6( pDEVBLK, pMPC_PIXwr, 0 );
              if (!pPTPHDRr1)
                  break;

              // Remember the activation status.
              pPTPBLK->bActivate6 |= IANSWEREDHIS_ADDRESS;

              // Add PTPHDR to chain.
              add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
            }
            else
            {
              // Hmm... the y-side has told us his IPv6 address and it wasn't
              // the guest IPv6 address specified on the config statement. This
              // could happen; the guest can have multiple IPv6 addresses, or
              // we are using a pre-configured TUN interface and we don't know
              // the guest IPv6 address, or we have been told the wrong address.
              // Perhaps the guest and driver IPv6 addresses were transposed on
              // the config statement.
              if (memcmp( pMPC_PIXwr->ipaddr, &pPTPBLK->iaDriveIPAddr6, 16 ) == 0)
              {
                // Looks like the guest and driver IPv6 addresses were transposed.
                hinet_ntop( AF_INET, &pMPC_PIXwr->ipaddr, cIPaddr, sizeof(cIPaddr) );
                // HHC03912 "%1d:%04X PTP: Guest has the driver IP address '%s'"
                WRMSG(HHC03912, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                     cIPaddr );

                // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv6)
                pPTPHDRr1 = build_C108_your_address_6( pDEVBLK, pMPC_PIXwr, 12 );
                if (!pPTPHDRr1)
                    break;

                // Add PTPHDR to chain.
                add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
              }
              else
              {
                // Hmm... the y-side has told us an IPv6 address and it wasn't
                // the guest or driver IPv4 address specified on the config
                // statement or the pre-configured TUN interface.
                // Check whether we are using a pre-configured TUN interface,
                // and that we have not already been told an address.
                memset(&addr6, 0, sizeof(addr6));
                if (pPTPBLK->fPreconfigured &&
                    ( memcmp( &addr6, &pPTPBLK->iaGuestIPAddr6, sizeof(addr6) ) == 0 )) {

                  // We are using a pre-configured TUN interface. Hooray, the
                  // y-side has told us his IPv6 address, something we didn't
                  // know but need to. Copy the y-side's IPv6 address.
                  memcpy( &pPTPBLK->iaGuestIPAddr6, &pMPC_PIXwr->ipaddr,
                          sizeof(pPTPBLK->iaGuestIPAddr6) );
                  hinet_ntop( AF_INET6, &pMPC_PIXwr->ipaddr, pPTPBLK->szGuestIPAddr6,
                                                             sizeof(pPTPBLK->szGuestIPAddr6) );

                  // The y-side has told us his IPv6 address and it is the guest
                  // address specified on the config statement, which is of course
                  // good news. Remember the activation status.
                  pPTPBLK->bActivate6 |= HETOLDMEHIS_ADDRESS;

                  // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv6)
                  pPTPHDRr1 = build_C108_your_address_6( pDEVBLK, pMPC_PIXwr, 0 );
                  if (!pPTPHDRr1)
                      break;

                  // Remember the activation status.
                  pPTPBLK->bActivate6 |= IANSWEREDHIS_ADDRESS;

                  // Add PTPHDR to chain.
                  add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
                }
                else
                {
                  // The guest has an IPv6 address we know nothing about. The guest
                  // can have multiple IPv6 addresses associated with an interface.
                  hinet_ntop( AF_INET6, &pMPC_PIXwr->ipaddr, cIPaddr, sizeof(cIPaddr) );
                  // HHC03914 "%1d:%04X PTP: Guest has IP address '%s'"
                  WRMSG(HHC03914, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                                       cIPaddr );

                  // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv6)
                  pPTPHDRr1 = build_C108_your_address_6( pDEVBLK, pMPC_PIXwr, 0 );
                  if (!pPTPHDRr1)
                      break;

                  // Add PTPHDR to chain.
                  add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
                }
              }
            }
          }
          else
          {
            // The y-side has told us about his Link Local address.
            // Remember the activation status.
            pPTPBLK->bActivateLL6 |= HETOLDMEHIS_ADDRESS;

            // Copy the y-side's Link Local address.
            memcpy( &pPTPBLK->iaGuestLLAddr6, pMPC_PIXwr->ipaddr, 16 );
            hinet_ntop( AF_INET6, &pPTPBLK->iaGuestLLAddr6,
                                  pPTPBLK->szGuestLLAddr6,
                                  sizeof(pPTPBLK->szGuestLLAddr6) );

            // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv6)
            pPTPHDRr1 = build_C108_your_address_6( pDEVBLK, pMPC_PIXwr, 0 );
            if (!pPTPHDRr1)
                break;

            // Remember the activation status.
            pPTPBLK->bActivateLL6 |= IANSWEREDHIS_ADDRESS;

            // Add PTPHDR to chain.
            add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
          }
        }
        else
        {
#endif /* defined(ENABLE_IPV6) */
          // IPv6 was not specified on the config statement, but the guest
          // has informed us of an IPv6 address. Reply but otherwise ignore.

          // Build RRH 0xC108 PIX 0x1101 to yTokenUlpConnection (Your address IPv6)
          pPTPHDRr1 = build_C108_your_address_6( pDEVBLK, pMPC_PIXwr, 0 );
          if (!pPTPHDRr1)
              break;

          // Add PTPHDR to chain.
            add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
#if defined(ENABLE_IPV6)
        }

        //
        if (pPTPBLK->fIPv6Spec)
        {
            // Check whether the connection is active.
            if (pPTPBLK->bActivateLL6 == WEAREACTIVE)
            {
                // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
                WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestLLAddr6 );
                pPTPBLK->fActiveLL6 = TRUE;
                pPTPBLK->bActivateLL6 = 0x00;
                pPTPBLK->bTerminateLL6 = 0x00;
                if (pPTPBLK->fActive6)
                    build_8108_icmpv6_packets( pDEVBLK );
            }
            if (pPTPBLK->bActivate6 == WEAREACTIVE)
            {
                // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
                WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestIPAddr6 );
                pPTPBLK->fActive6 = TRUE;
                pPTPBLK->bActivate6 = 0x00;
                pPTPBLK->bTerminate6 = 0x00;
                if (pPTPBLK->fActiveLL6)
                    build_8108_icmpv6_packets( pDEVBLK );
            }
        }
#endif /* defined(ENABLE_IPV6) */

        break;

    case YOUR_ADDRESS_IPV4:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) Your address IPv4" );
        }

        // Remember the activation status.
        pPTPBLK->bActivate4 |= HEANSWEREDMY_ADDRESS;

        // Check whether the connection is active.
        if (pPTPBLK->bActivate4 == WEAREACTIVE)
        {
            // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
            WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                pPTPBLK->szGuestIPAddr4 );
            pPTPBLK->fActive4 = TRUE;
            pPTPBLK->bActivate4 = 0x00;
            pPTPBLK->bTerminate4 = 0x00;
        }

        break;

    case YOUR_ADDRESS_IPV6:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) Your address IPv6" );
        }

#if defined(ENABLE_IPV6)
        // Check whether the y-side is telling us about a Link Local address.
        fLL = FALSE;
        memset( addr6.s6_addr, 0, 16 );
        addr6.s6_addr[0] = 0xFE;
        addr6.s6_addr[1] = 0x80;
        if (memcmp( pMPC_PIXwr->ipaddr, &addr6, 8 ) == 0)
            fLL = TRUE;

        //
        if (!fLL)
        {
            // The y-side has told us about our IPv6 address.
            // Remember the activation status.
            pPTPBLK->bActivate6 |= HEANSWEREDMY_ADDRESS;

            // Check whether the connection is active.
            if (pPTPBLK->bActivate6 == WEAREACTIVE)
            {
                // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
                WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestIPAddr6 );
                pPTPBLK->fActive6 = TRUE;
                pPTPBLK->bActivate6 = 0x00;
                pPTPBLK->bTerminate6 = 0x00;
                if (pPTPBLK->fActiveLL6)
                    build_8108_icmpv6_packets( pDEVBLK );
            }
        }
        else
        {
            // The y-side has told us about our Link Local address.
            // Remember the activation status.
            pPTPBLK->bActivateLL6 |= HEANSWEREDMY_ADDRESS;

            // Check whether the connection is active.
            if (pPTPBLK->bActivateLL6 == WEAREACTIVE)
            {
                // HHC03915 "%1d:%04X PTP: Connection active to guest IP address '%s'"
                WRMSG(HHC03915, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestLLAddr6 );
                pPTPBLK->fActiveLL6 = TRUE;
                pPTPBLK->bActivateLL6 = 0x00;
                pPTPBLK->bTerminateLL6 = 0x00;
                if (pPTPBLK->fActive6)
                    build_8108_icmpv6_packets( pDEVBLK );
            }
        }
#endif /* defined(ENABLE_IPV6) */

        break;

    case WILL_YOU_STOP_IPV4:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) Will you stop IPv4?" );
        }

        // Remember the termination status.
        pPTPBLK->bTerminate4 |= HEASKEDME_STOP;

        // Build RRH 0xC108 PIX 0x0180 to yTokenUlpConnection (I will stop IPv4)
        pPTPHDRr1 = build_C108_i_will_stop_4( pDEVBLK, pMPC_PIXwr );
        if (!pPTPHDRr1)
            break;

        // Build RRH 0xC108 PIX 0x0180 to yTokenUlpConnection (Will you stop IPv4)
        pPTPHDRr2 = build_C108_will_you_stop_4( pDEVBLK );
        if (!pPTPHDRr2)
        {
            free( pPTPHDRr1 );
            break;
        }

        // Remember the termination status.
        pPTPBLK->bTerminate4 |= IANSWEREDHIM_STOP;
        pPTPBLK->bTerminate4 |= IASKEDHIM_STOP;

        // Add PTPHDRs to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr2 );

        break;

    case WILL_YOU_STOP_IPV6:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) Will you stop IPv6?" );
        }

        // Remember the termination status.
        pPTPBLK->bTerminate6 |= HEASKEDME_STOP;
        pPTPBLK->bTerminateLL6 |= HEASKEDME_STOP;

        // Build RRH 0xC108 PIX 0x0180 to yTokenUlpConnection (I will stop IPv6)
        pPTPHDRr1 = build_C108_i_will_stop_6( pDEVBLK, pMPC_PIXwr );
        if (!pPTPHDRr1)
            break;

        // Build RRH 0xC108 PIX 0x0180 to yTokenUlpConnection (Will you stop IPv6)
        pPTPHDRr2 = build_C108_will_you_stop_6( pDEVBLK );
        if (!pPTPHDRr2)
        {
            free( pPTPHDRr1 );
            break;
        }

        // Remember the termination status.
        pPTPBLK->bTerminate6 |= IANSWEREDHIM_STOP;
        pPTPBLK->bTerminate6 |= IASKEDHIM_STOP;
        pPTPBLK->bTerminateLL6 |= IANSWEREDHIM_STOP;
        pPTPBLK->bTerminateLL6 |= IASKEDHIM_STOP;

        // Add PTPHDRs to chain.
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr1 );
        add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRr2 );

        break;

    case I_WILL_STOP_IPV4:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) I will stop IPv4" );
        }

        // Remember the termination status.
        pPTPBLK->bTerminate4 |= HEANSWEREDME_STOP;

        // Check whether the connection is terminated.
        if (pPTPBLK->bTerminate4 == WEARETERMINATED)
        {
            // The guest OS on the y-side has stopped the device.
            if (pPTPBLK->fActive4)
            {
                // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
                WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestIPAddr4 );
            }
            pPTPBLK->fActive4 = FALSE;
            pPTPBLK->bActivate4 = 0x00;
            pPTPBLK->bTerminate4 = 0x00;
        }

        break;

    case I_WILL_STOP_IPV6:

        // Display various information, maybe
        if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
        {
            mpc_display_description( pDEVBLK, "In RRH 0xC108 (UlpComm) I will stop IPv6" );
        }

        // Remember the termination status.
        pPTPBLK->bTerminate6 |= HEANSWEREDME_STOP;
        pPTPBLK->bTerminateLL6 |= HEANSWEREDME_STOP;

        // Check whether the connection is terminated.
        if (pPTPBLK->bTerminate6 == WEARETERMINATED)
        {
            // The guest OS on the y-side has stopped the device.
#if defined(ENABLE_IPV6)
            if (pPTPBLK->fActive6)
            {
                // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
                WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestIPAddr6 );
            }
#endif /* defined(ENABLE_IPV6) */
            pPTPBLK->fActive6 = FALSE;
            pPTPBLK->bActivate6 = 0x00;
            pPTPBLK->bTerminate6 = 0x00;
        }
        if (pPTPBLK->bTerminateLL6 == WEARETERMINATED)
        {
            // The guest OS on the y-side has stopped the device.
#if defined(ENABLE_IPV6)
            if (pPTPBLK->fActiveLL6)
            {
                // HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address '%s'"
                WRMSG(HHC03916, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum,
                    pPTPBLK->szGuestLLAddr6 );
            }
#endif /* defined(ENABLE_IPV6) */
            pPTPBLK->fActiveLL6 = FALSE;
            pPTPBLK->bActivateLL6 = 0x00;
            pPTPBLK->bTerminateLL6 = 0x00;
        }

        break;

    default:

        // HHC03936 "%1d:%04X PTP: Accept data contains unknown %s"
        WRMSG(HHC03936, "W", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "PIX" );
        mpc_display_rrh_and_pix( pDEVBLK, pMPC_THwr, pMPC_RRHwr, FROM_GUEST );

        break;

    }

    return 0;
}   /* End function  write_rrh_C108() */


/* ------------------------------------------------------------------ */
/* build_C108_will_you_start_4()                                      */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0180 (Will you start IPv4?)

PTPHDR*  build_C108_will_you_start_4( DEVBLK* pDEVBLK )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_START;
    pMPC_PIXre->askans = PIX_ASK;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV4;
    STORE_HW( pMPC_PIXre->idnum, ++pPTPBLK->uIdNum );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) Will you start IPv4?" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_will_you_start_4() */

/* ------------------------------------------------------------------ */
/* build_C108_will_you_start_6()                                      */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0180 (Will you start IPv6?)

PTPHDR*  build_C108_will_you_start_6( DEVBLK* pDEVBLK )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_START;
    pMPC_PIXre->askans = PIX_ASK;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV6;
    STORE_HW( pMPC_PIXre->idnum, ++pPTPBLK->uIdNum );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) Will you start IPv6?" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_will_you_start_6() */

/* ------------------------------------------------------------------ */
/* build_C108_i_will_start_4()                                        */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0101 (I will start IPv4)

PTPHDR*  build_C108_i_will_start_4( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIXwr, U16 uRCode )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the message will be build.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_START;
    pMPC_PIXre->askans = PIX_ANSWER;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV4;
    memcpy( pMPC_PIXre->idnum, pMPC_PIXwr->idnum, sizeof(pMPC_PIXre->idnum) );
    STORE_HW( pMPC_PIXre->rcode, uRCode );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) I will start IPv4" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_i_will_start_4() */

/* ------------------------------------------------------------------ */
/* build_C108_i_will_start_6()                                        */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0101 (I will start IPv6)

PTPHDR*  build_C108_i_will_start_6( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIXwr, U16 uRCode )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_START;
    pMPC_PIXre->askans = PIX_ANSWER;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV6;
    memcpy( pMPC_PIXre->idnum, pMPC_PIXwr->idnum, sizeof(pMPC_PIXre->idnum) );
    STORE_HW( pMPC_PIXre->rcode, uRCode );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) I will start IPv6" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_i_will_start_6() */

/* ------------------------------------------------------------------ */
/* build_C108_my_address_4()                                          */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x1180 (My address IPv4)

PTPHDR*  build_C108_my_address_4( DEVBLK* pDEVBLK )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_ADDRESS;
    pMPC_PIXre->askans = PIX_ASK;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV4;
    STORE_HW( pMPC_PIXre->idnum, ++pPTPBLK->uIdNum );
    memcpy( pMPC_PIXre->ipaddr, &pPTPBLK->iaDriveIPAddr4, 4 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) My address IPv4" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_my_address_4() */

/* ------------------------------------------------------------------ */
/* build_C108_my_address_6()                                          */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x1180 (My address IPv6)

PTPHDR*  build_C108_my_address_6( DEVBLK* pDEVBLK, u_int fLL )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_ADDRESS;
    pMPC_PIXre->askans = PIX_ASK;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV6;
    STORE_HW( pMPC_PIXre->idnum, ++pPTPBLK->uIdNum );
    if (fLL)
    {
#if defined(ENABLE_IPV6)
        memcpy( pMPC_PIXre->ipaddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    }
    else
    {
        memcpy( pMPC_PIXre->ipaddr, &pPTPBLK->iaDriveIPAddr6, 16 );
#endif /* defined(ENABLE_IPV6) */
    }

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) My address IPv6" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_my_address_6() */

/* ------------------------------------------------------------------ */
/* build_C108_your_address_4()                                        */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x1101 (Your address IPv4)

PTPHDR*  build_C108_your_address_4( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIXwr, U16 uRCode )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_ADDRESS;
    pMPC_PIXre->askans = PIX_ANSWER;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV4;
    memcpy( pMPC_PIXre->idnum, pMPC_PIXwr->idnum, sizeof(pMPC_PIXre->idnum) );
    STORE_HW( pMPC_PIXre->rcode, uRCode );
    memcpy( pMPC_PIXre->ipaddr, pMPC_PIXwr->ipaddr, 4 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) Your address IPv4" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_your_address_4() */

/* ------------------------------------------------------------------ */
/* build_C108_your_address_6()                                        */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x1101 (Your address IPv6)

PTPHDR*  build_C108_your_address_6( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIXwr, U16 uRCode )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the first reply will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_ADDRESS;
    pMPC_PIXre->askans = PIX_ANSWER;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV6;
    memcpy( pMPC_PIXre->idnum, pMPC_PIXwr->idnum, sizeof(pMPC_PIXre->idnum) );
    STORE_HW( pMPC_PIXre->rcode, uRCode );
    memcpy( pMPC_PIXre->ipaddr, pMPC_PIXwr->ipaddr, 16 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) Your address IPv6" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_your_address_6() */

/* ------------------------------------------------------------------ */
/* build_C108_will_you_stop_4()                                       */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0280 (Will you stop IPv4?)

PTPHDR*  build_C108_will_you_stop_4( DEVBLK* pDEVBLK )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_STOP;
    pMPC_PIXre->askans = PIX_ASK;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV4;
    STORE_HW( pMPC_PIXre->idnum, ++pPTPBLK->uIdNum );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) Will you stop IPv4?" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_will_you_stop_4() */

/* ------------------------------------------------------------------ */
/* build_C108_will_you_stop_6()                                       */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0280 (Will you stop IPv6?)

PTPHDR*  build_C108_will_you_stop_6( DEVBLK* pDEVBLK )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_STOP;
    pMPC_PIXre->askans = PIX_ASK;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV6;
    STORE_HW( pMPC_PIXre->idnum, ++pPTPBLK->uIdNum );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) Will you stop IPv6?" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_will_you_stop_6() */

/* ------------------------------------------------------------------ */
/* build_C108_i_will_stop_4()                                         */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0201 (I will stop IPv4)

PTPHDR*  build_C108_i_will_stop_4( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIXwr )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_STOP;
    pMPC_PIXre->askans = PIX_ANSWER;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV4;
    memcpy( pMPC_PIXre->idnum, pMPC_PIXwr->idnum, sizeof(pMPC_PIXre->idnum) );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) I will stop IPv4" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_i_will_stop_4() */

/* ------------------------------------------------------------------ */
/* build_C108_i_will_stop_6()                                         */
/* ------------------------------------------------------------------ */
// Build RRH 0xC108 PIX 0x0201 (I will stop IPv6)

PTPHDR*  build_C108_i_will_stop_6( DEVBLK* pDEVBLK, MPC_PIX* pMPC_PIXwr )
{
    PTPATH*    pPTPATH      = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK      = pPTPATH->pPTPBLK;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    MPC_PIX*   pMPC_PIXre;    // MPC_PIX follows MPC_PH


    // Allocate a buffer in which the response will be build.
    // Note: the largest reply will be 88 bytes.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return NULL;

    // Fix-up various lengths
    uLength3 = SIZE_PIX;                     // the MPC_PIX
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;          // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pMPC_PIXre = (MPC_PIX*)((BYTE*)pMPC_PHre + SIZE_PH);

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_IPA;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare MPC_PIXre
    pMPC_PIXre->action = PIX_STOP;
    pMPC_PIXre->askans = PIX_ANSWER;
    pMPC_PIXre->numaddr = PIX_ONEADDR;
    pMPC_PIXre->iptype = PIX_IPV6;
    memcpy( pMPC_PIXre->idnum, pMPC_PIXwr->idnum, sizeof(pMPC_PIXre->idnum) );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0xC108 (UlpComm) I will stop IPv6" );
    }

    return pPTPHDRre;
}   /* End function  build_C108_i_will_stop_6() */


#if defined(ENABLE_IPV6)
/* ------------------------------------------------------------------ */
/* build_8108_icmpv6_packets()                                        */
/* ------------------------------------------------------------------ */

void  build_8108_icmpv6_packets( DEVBLK* pDEVBLK )
{
    PTPATH*    pPTPATH   = pDEVBLK->dev_data;
    PTPBLK*    pPTPBLK   = pPTPATH->pPTPBLK;
    PTPATH*    pPTPATHre = pPTPBLK->pPTPATHRead;
    U32        uLength1;
    U32        uLength2;
    U32        uLength3;
    U16        uLength4;
    U16        uLength5;
    U16        uLength6;
    PTPHDR*    pPTPHDRre;     // PTPHDR to be read
    MPC_TH*    pMPC_THre;     // MPC_TH follows PTPHDR
    MPC_RRH*   pMPC_RRHre;    // MPC_RRH follows MPC_TH
    MPC_PH*    pMPC_PHre;     // MPC_PH follows MPC_RRH
    PIP6FRM    pIP6FRMre;     // IPv6 header
    BYTE*      pHopOpt;       // Hop-by-Hop Options follows IPv6 header
    BYTE*      pIcmpHdr;      // ICMPv6 header follows IPv6 header or Hop-by-Hop


    // Allocate a buffer in which the ICMPv6 Neighbor Advertisment message
    // will be built. Note: the message will be 128 bytes.
    // The source address is the drive link local address, the destination
    // address is the Link-Local Scope All Nodes multicast address, i.e.
    // FF02:0:0:0:0:0:0:1.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return;

    // Fix-up various lengths
    uLength6 = 24;                            // the ICMPv6 packet
    uLength5 = 0;                             // no Hop-by-Hop Options
    uLength4 = sizeof(IP6FRM);                // the IPv6 header
    uLength3 = uLength4 + uLength6;           // the data
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pIP6FRMre = (IP6FRM*)((BYTE*)pMPC_PHre + SIZE_PH);
    pHopOpt   = NULL;
    pIcmpHdr  = (BYTE*)pIP6FRMre->bPayload;

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_CM;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare IP6FRMre, i.e. IPv6 header
    pIP6FRMre->bVersTCFlow[0] = 0x60;
    STORE_HW( pIP6FRMre->bPayloadLength, uLength6 );
    pIP6FRMre->bNextHeader = 0x3A;
    pIP6FRMre->bHopLimit = 0xFF;
    memcpy( pIP6FRMre->bSrcAddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    pIP6FRMre->bDstAddr[0]  = 0xFF;
    pIP6FRMre->bDstAddr[1]  = 0x02;
    pIP6FRMre->bDstAddr[15] = 0x01;

    // Prepare ICMPv6 packet
    pIcmpHdr[0] = 0x88;
    pIcmpHdr[4] = 0x20;
    memcpy( pIcmpHdr+8, &pPTPBLK->iaDriveLLAddr6, 16 );

    // Calculate and set the ICMPv6 checksum
    calculate_icmpv6_checksum( pIP6FRMre, pIcmpHdr, (int)uLength6 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x8108 (UlpComm) Neighbor advertisment" );
    }

    // Add PTPHDR to chain.
    add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );


    // Allocate a buffer in which the ICMPv6 Router Solicitation message
    // will be built. Note: the message will be 120 bytes.
    // The source address is the drive link local address, the destination
    // address is the Link-Local Scope All Routers multicast address, i.e.
    // FF02:0:0:0:0:0:0:2.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return;

    // Fix-up various lengths
    uLength6 = 16;                            // the ICMPv6 packet
    uLength5 = 0;                             // no Hop-by-Hop Options
    uLength4 = sizeof(IP6FRM);                // the IPv6 header
    uLength3 = uLength4 + uLength6;           // the data
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pIP6FRMre = (IP6FRM*)((BYTE*)pMPC_PHre + SIZE_PH);
    pHopOpt   = NULL;
    pIcmpHdr  = (BYTE*)pIP6FRMre->bPayload;

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_CM;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare IP6FRMre, i.e. IPv6 header
    pIP6FRMre->bVersTCFlow[0] = 0x60;
    STORE_HW( pIP6FRMre->bPayloadLength, uLength6 );
    pIP6FRMre->bNextHeader = 0x3A;
    pIP6FRMre->bHopLimit = 0xFF;
    memcpy( pIP6FRMre->bSrcAddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    pIP6FRMre->bDstAddr[0]  = 0xFF;
    pIP6FRMre->bDstAddr[1]  = 0x02;
    pIP6FRMre->bDstAddr[15] = 0x02;

    // Prepare ICMPv6 packet
    pIcmpHdr[0] = 0x85;
    pIcmpHdr[8] = 0x01;
    pIcmpHdr[9] = 0x01;

    // Calculate and set the ICMPv6 checksum
    calculate_icmpv6_checksum( pIP6FRMre, pIcmpHdr, (int)uLength6 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x8108 (UlpComm) Router solicitation" );
    }

    // Add PTPHDR to chain.
    add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );


    // Allocate a buffer in which the ICMPv6 Neighbor Advertisment message
    // will be built. Note: the message will be 128 bytes.
    // The source address is the drive address, the destination address
    // is the Link-Local Scope All Nodes multicast address, i.e.
    // FF02:0:0:0:0:0:0:1.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return;

    // Fix-up various lengths
    uLength6 = 24;                            // the ICMPv6 packet
    uLength5 = 0;                             // no Hop-by-Hop Options
    uLength4 = sizeof(IP6FRM);                // the IPv6 header
    uLength3 = uLength4 + uLength6;           // the data
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;  // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;           // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pIP6FRMre = (IP6FRM*)((BYTE*)pMPC_PHre + SIZE_PH);
    pHopOpt   = NULL;
    pIcmpHdr  = (BYTE*)pIP6FRMre->bPayload;

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_CM;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare IP6FRMre, i.e. IPv6 header
    pIP6FRMre->bVersTCFlow[0] = 0x60;
    STORE_HW( pIP6FRMre->bPayloadLength, uLength6 );
    pIP6FRMre->bNextHeader = 0x3A;
    pIP6FRMre->bHopLimit = 0xFF;
    memcpy( pIP6FRMre->bSrcAddr, &pPTPBLK->iaDriveIPAddr6, 16 );
    pIP6FRMre->bDstAddr[0]  = 0xFF;
    pIP6FRMre->bDstAddr[1]  = 0x02;
    pIP6FRMre->bDstAddr[15] = 0x01;

    // Prepare ICMPv6 packet
    pIcmpHdr[0] = 0x88;
    pIcmpHdr[4] = 0x20;
    memcpy( pIcmpHdr+8, &pPTPBLK->iaDriveIPAddr6, 16 );

    // Calculate and set the ICMPv6 checksum
    calculate_icmpv6_checksum( pIP6FRMre, pIcmpHdr, (int)uLength6 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x8108 (UlpComm) Neighbor advertisment" );
    }

    // Add PTPHDR to chain.
    add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );


    // Allocate a buffer in which the ICMPv6 Group Membership Report message
    // will be built. Note: the message will be 136 bytes.
    // The source address is the drive link local address, the destination
    // address is the Link-Local Scope Selected-Node multicast address for
    // the drive link local address, i.e. FF02:0:0:0:0:1:FFxx:xxxx.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return;

    // Fix-up various lengths
    uLength6 = 24;                              // the ICMPv6 packet
    uLength5 = 8;                               // the Hop-by-Hop Options
    uLength4 = sizeof(IP6FRM);                  // the IPv6 header
    uLength3 = uLength4 + uLength5 + uLength6;  // the data
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;    // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;             // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pIP6FRMre = (IP6FRM*)((BYTE*)pMPC_PHre + SIZE_PH);
    pHopOpt   = (BYTE*)pIP6FRMre->bPayload;
    pIcmpHdr  = pHopOpt + uLength5;

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_CM;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare IP6FRMre, i.e. IPv6 header
    pIP6FRMre->bVersTCFlow[0] = 0x60;
    STORE_HW( pIP6FRMre->bPayloadLength, uLength5 + uLength6 );
    pIP6FRMre->bNextHeader = 0x00;
    pIP6FRMre->bHopLimit = 0x01;
    memcpy( pIP6FRMre->bSrcAddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    pIP6FRMre->bDstAddr[0]  = 0xFF;
    pIP6FRMre->bDstAddr[1]  = 0x02;
    pIP6FRMre->bDstAddr[11] = 0x01;
    pIP6FRMre->bDstAddr[12] = 0xFF;
    memcpy( pIP6FRMre->bDstAddr+13, &pPTPBLK->iaDriveLLAddr6+13, 3 );

    // Prepare Hop-by-Hop Options
    pHopOpt[0] = 0x3A;
    pHopOpt[2] = 0x05;
    pHopOpt[3] = 0x02;

    // Prepare ICMPv6 packet
    pIcmpHdr[0] = 0x83;
    pIcmpHdr[8]  = 0xFF;
    pIcmpHdr[9]  = 0x02;
    pIcmpHdr[19] = 0x01;
    pIcmpHdr[20] = 0xFF;
    memcpy( pIcmpHdr+21, &pPTPBLK->iaDriveLLAddr6+13, 3 );

    // Calculate and set the ICMPv6 checksum
    calculate_icmpv6_checksum( pIP6FRMre, pIcmpHdr, (int)uLength6 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x8108 (UlpComm) Group membership report" );
    }

    // Add PTPHDR to chain.
    add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );


    // Allocate a buffer in which the ICMPv6 Group Membership Report message
    // will be built. Note: the message will be 136 bytes.
    // The source address is the drive link local address, the destination
    // address is the Link-Local Scope Selected-Node multicast address for
    // the drive address, i.e. FF02:0:0:0:0:1:FFyy:yyyy.
    pPTPHDRre = alloc_ptp_buffer( pDEVBLK, 256 );
    if (!pPTPHDRre)
        return;

    // Fix-up various lengths
    uLength6 = 24;                              // the ICMPv6 packet
    uLength5 = 8;                               // the Hop-by-Hop Options
    uLength4 = sizeof(IP6FRM);                  // the IPv6 header
    uLength3 = uLength4 + uLength5 + uLength6;  // the data
    uLength2 = SIZE_TH + SIZE_RRH + SIZE_PH;    // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;             // the MPC_TH/MPC_RRH/MPC_PH and data

    // Fix-up various pointers
    pMPC_THre = (MPC_TH*)((BYTE*)pPTPHDRre + SIZE_HDR);
    pMPC_RRHre = (MPC_RRH*)((BYTE*)pMPC_THre + SIZE_TH);
    pMPC_PHre = (MPC_PH*)((BYTE*)pMPC_RRHre + SIZE_RRH);
    pIP6FRMre = (IP6FRM*)((BYTE*)pMPC_PHre + SIZE_PH);
    pHopOpt   = (BYTE*)pIP6FRMre->bPayload;
    pIcmpHdr  = pHopOpt + uLength5;

    // Prepare PTPHDRre
    pPTPHDRre->iDataLen = uLength1;

    // Prepare MPC_THre
    STORE_FW( pMPC_THre->first4, MPC_TH_FIRST4 );
    STORE_FW( pMPC_THre->offrrh, SIZE_TH );
    STORE_FW( pMPC_THre->length, uLength1 );
    STORE_HW( pMPC_THre->unknown10, MPC_TH_UNKNOWN10 );      // !!! //
    STORE_HW( pMPC_THre->numrrh, 1 );

    // Prepare MPC_RRHre
    pMPC_RRHre->type = RRH_TYPE_CM;
    pMPC_RRHre->proto = PROTOCOL_LAYER2;
    STORE_HW( pMPC_RRHre->numph, 1 );
    STORE_HW( pMPC_RRHre->offph, SIZE_RRH );
    STORE_HW( pMPC_RRHre->lenfida, (U16)uLength3 );
    STORE_F3( pMPC_RRHre->lenalda, uLength3 );
    pMPC_RRHre->tokenx5 = MPC_TOKEN_X5;
    memcpy( pMPC_RRHre->token, pPTPBLK->yTokenUlpConnection, MPC_TOKEN_LENGTH );

    // Prepare MPC_PHre
    pMPC_PHre->locdata = PH_LOC_1;
    STORE_F3( pMPC_PHre->lendata, uLength3 );
    STORE_FW( pMPC_PHre->offdata, uLength2 );

    // Prepare IP6FRMre, i.e. IPv6 header
    pIP6FRMre->bVersTCFlow[0] = 0x60;
    STORE_HW( pIP6FRMre->bPayloadLength, uLength5 + uLength6 );
    pIP6FRMre->bNextHeader = 0x00;
    pIP6FRMre->bHopLimit = 0x01;
    memcpy( pIP6FRMre->bSrcAddr, &pPTPBLK->iaDriveLLAddr6, 16 );
    pIP6FRMre->bDstAddr[0]  = 0xFF;
    pIP6FRMre->bDstAddr[1]  = 0x02;
    pIP6FRMre->bDstAddr[11] = 0x01;
    pIP6FRMre->bDstAddr[12] = 0xFF;
    memcpy( pIP6FRMre->bDstAddr+13, &pPTPBLK->iaDriveIPAddr6+13, 3 );

    // Prepare Hop-by-Hop Options
    pHopOpt[0] = 0x3A;
    pHopOpt[2] = 0x05;
    pHopOpt[3] = 0x02;

    // Prepare ICMPv6 packet
    pIcmpHdr[0] = 0x83;
    pIcmpHdr[8]  = 0xFF;
    pIcmpHdr[9]  = 0x02;
    pIcmpHdr[19] = 0x01;
    pIcmpHdr[20] = 0xFF;
    memcpy( pIcmpHdr+21, &pPTPBLK->iaDriveIPAddr6+13, 3 );

    // Calculate and set the ICMPv6 checksum
    calculate_icmpv6_checksum( pIP6FRMre, pIcmpHdr, (int)uLength6 );

    // Display various information, maybe
    if (pPTPBLK->uDebugMask & DBGPTPUPDOWN)
    {
        mpc_display_description( pDEVBLK, "Out RRH 0x8108 (UlpComm) Group membership report" );
    }

    // Add PTPHDR to chain.
    add_buffer_to_chain_and_signal_event( pPTPATHre, pPTPHDRre );

    return;
}   /* End function  build_8108_icmpv6_packets() */
#endif /* defined(ENABLE_IPV6) */


/* ------------------------------------------------------------------ */
/* gen_csv_sid()                                                      */
/* ------------------------------------------------------------------ */
// If this function is called multiple times with the same input clock
// values the output token value will always be the same.
// For example:-
//   if Clock1 is always       C5620E1F FC2FA000
//   and Clock2 is always      C5620DD2 5E806000
//   the token will always be  76152E37 8A370460
// but:-
//   if Clock1 is always       C5620DD2 5E806000
//   and Clock2 is always      C5620E1F FC2FA000
//   the Token will always be  734EE30C 0854474F

// Input:  pClock1  An 8-byte tod clock value
//         pClock2  An 8-byte tod clock value
// Output: pToken   An 8-byte token

void     gen_csv_sid( BYTE* pClock1, BYTE* pClock2, BYTE* pToken )
{

    static const U32 XorConstant[512] =
                             { 0x00404100, 0x00000000, 0x00004000, 0x00404101,
                               0x00404001, 0x00004101, 0x00000001, 0x00004000,
                               0x00000100, 0x00404100, 0x00404101, 0x00000100,
                               0x00400101, 0x00404001, 0x00400000, 0x00000001,
                               0x00000101, 0x00400100, 0x00400100, 0x00004100,
                               0x00004100, 0x00404000, 0x00404000, 0x00400101,
                               0x00004001, 0x00400001, 0x00400001, 0x00004001,
                               0x00000000, 0x00000101, 0x00004101, 0x00400000,
                               0x00004000, 0x00404101, 0x00000001, 0x00404000,
                               0x00404100, 0x00400000, 0x00400000, 0x00000100,
                               0x00404001, 0x00004000, 0x00004100, 0x00400001,
                               0x00000100, 0x00000001, 0x00400101, 0x00004101,
                               0x00404101, 0x00004001, 0x00404000, 0x00400101,
                               0x00400001, 0x00000101, 0x00004101, 0x00404100,
                               0x00000101, 0x00400100, 0x00400100, 0x00000000,
                               0x00004001, 0x00004100, 0x00000000, 0x00404001,
                               0x20042008, 0x20002000, 0x00002000, 0x00042008,
                               0x00040000, 0x00000008, 0x20040008, 0x20002008,
                               0x20000008, 0x20042008, 0x20042000, 0x20000000,
                               0x20002000, 0x00040000, 0x00000008, 0x20040008,
                               0x00042000, 0x00040008, 0x20002008, 0x00000000,
                               0x20000000, 0x00002000, 0x00042008, 0x20040000,
                               0x00040008, 0x20000008, 0x00000000, 0x00042000,
                               0x00002008, 0x20042000, 0x20040000, 0x00002008,
                               0x00000000, 0x00042008, 0x20040008, 0x00040000,
                               0x20002008, 0x20040000, 0x20042000, 0x00002000,
                               0x20040000, 0x20002000, 0x00000008, 0x20042008,
                               0x00042008, 0x00000008, 0x00002000, 0x20000000,
                               0x00002008, 0x20042000, 0x00040000, 0x20000008,
                               0x00040008, 0x20002008, 0x20000008, 0x00040008,
                               0x00042000, 0x00000000, 0x20002000, 0x00002008,
                               0x20000000, 0x20040008, 0x20042008, 0x00042000,
                               0x00000082, 0x02008080, 0x00000000, 0x02008002,
                               0x02000080, 0x00000000, 0x00008082, 0x02000080,
                               0x00008002, 0x02000002, 0x02000002, 0x00008000,
                               0x02008082, 0x00008002, 0x02008000, 0x00000082,
                               0x02000000, 0x00000002, 0x02008080, 0x00000080,
                               0x00008080, 0x02008000, 0x02008002, 0x00008082,
                               0x02000082, 0x00008080, 0x00008000, 0x02000082,
                               0x00000002, 0x02008082, 0x00000080, 0x02000000,
                               0x02008080, 0x02000000, 0x00008002, 0x00000082,
                               0x00008000, 0x02008080, 0x02000080, 0x00000000,
                               0x00000080, 0x00008002, 0x02008082, 0x02000080,
                               0x02000002, 0x00000080, 0x00000000, 0x02008002,
                               0x02000082, 0x00008000, 0x02000000, 0x02008082,
                               0x00000002, 0x00008082, 0x00008080, 0x02000002,
                               0x02008000, 0x02000082, 0x00000082, 0x02008000,
                               0x00008082, 0x00000002, 0x02008002, 0x00008080,
                               0x40200800, 0x40000820, 0x40000820, 0x00000020,
                               0x00200820, 0x40200020, 0x40200000, 0x40000800,
                               0x00000000, 0x00200800, 0x00200800, 0x40200820,
                               0x40000020, 0x00000000, 0x00200020, 0x40200000,
                               0x40000000, 0x00000800, 0x00200000, 0x40200800,
                               0x00000020, 0x00200000, 0x40000800, 0x00000820,
                               0x40200020, 0x40000000, 0x00000820, 0x00200020,
                               0x00000800, 0x00200820, 0x40200820, 0x40000020,
                               0x00200020, 0x40200000, 0x00200800, 0x40200820,
                               0x40000020, 0x00000000, 0x00000000, 0x00200800,
                               0x00000820, 0x00200020, 0x40200020, 0x40000000,
                               0x40200800, 0x40000820, 0x40000820, 0x00000020,
                               0x40200820, 0x40000020, 0x40000000, 0x00000800,
                               0x40200000, 0x40000800, 0x00200820, 0x40200020,
                               0x40000800, 0x00000820, 0x00200000, 0x40200800,
                               0x00000020, 0x00200000, 0x00000800, 0x00200820,
                               0x00000040, 0x00820040, 0x00820000, 0x10800040,
                               0x00020000, 0x00000040, 0x10000000, 0x00820000,
                               0x10020040, 0x00020000, 0x00800040, 0x10020040,
                               0x10800040, 0x10820000, 0x00020040, 0x10000000,
                               0x00800000, 0x10020000, 0x10020000, 0x00000000,
                               0x10000040, 0x10820040, 0x10820040, 0x00800040,
                               0x10820000, 0x10000040, 0x00000000, 0x10800000,
                               0x00820040, 0x00800000, 0x10800000, 0x00020040,
                               0x00020000, 0x10800040, 0x00000040, 0x00800000,
                               0x10000000, 0x00820000, 0x10800040, 0x10020040,
                               0x00800040, 0x10000000, 0x10820000, 0x00820040,
                               0x10020040, 0x00000040, 0x00800000, 0x10820000,
                               0x10820040, 0x00020040, 0x10800000, 0x10820040,
                               0x00820000, 0x00000000, 0x10020000, 0x10800000,
                               0x00020040, 0x00800040, 0x10000040, 0x00020000,
                               0x00000000, 0x10020000, 0x00820040, 0x10000040,
                               0x08000004, 0x08100000, 0x00001000, 0x08101004,
                               0x08100000, 0x00000004, 0x08101004, 0x00100000,
                               0x08001000, 0x00101004, 0x00100000, 0x08000004,
                               0x00100004, 0x08001000, 0x08000000, 0x00001004,
                               0x00000000, 0x00100004, 0x08001004, 0x00001000,
                               0x00101000, 0x08001004, 0x00000004, 0x08100004,
                               0x08100004, 0x00000000, 0x00101004, 0x08101000,
                               0x00001004, 0x00101000, 0x08101000, 0x08000000,
                               0x08001000, 0x00000004, 0x08100004, 0x00101000,
                               0x08101004, 0x00100000, 0x00001004, 0x08000004,
                               0x00100000, 0x08001000, 0x08000000, 0x00001004,
                               0x08000004, 0x08101004, 0x00101000, 0x08100000,
                               0x00101004, 0x08101000, 0x00000000, 0x08100004,
                               0x00000004, 0x00001000, 0x08100000, 0x00101004,
                               0x00001000, 0x00100004, 0x08001004, 0x00000000,
                               0x08101000, 0x08000000, 0x00100004, 0x08001004,
                               0x00080000, 0x81080000, 0x81000200, 0x00000000,
                               0x00000200, 0x81000200, 0x80080200, 0x01080200,
                               0x81080200, 0x00080000, 0x00000000, 0x81000000,
                               0x80000000, 0x01000000, 0x81080000, 0x80000200,
                               0x01000200, 0x80080200, 0x80080000, 0x01000200,
                               0x81000000, 0x01080000, 0x01080200, 0x80080000,
                               0x01080000, 0x00000200, 0x80000200, 0x81080200,
                               0x00080200, 0x80000000, 0x01000000, 0x00080200,
                               0x01000000, 0x00080200, 0x00080000, 0x81000200,
                               0x81000200, 0x81080000, 0x81080000, 0x80000000,
                               0x80080000, 0x01000000, 0x01000200, 0x00080000,
                               0x01080200, 0x80000200, 0x80080200, 0x01080200,
                               0x80000200, 0x81000000, 0x81080200, 0x01080000,
                               0x00080200, 0x00000000, 0x80000000, 0x81080200,
                               0x00000000, 0x80080200, 0x01080000, 0x00000200,
                               0x81000000, 0x01000200, 0x00000200, 0x80080000,
                               0x04000410, 0x00000400, 0x00010000, 0x04010410,
                               0x04000000, 0x04000410, 0x00000010, 0x04000000,
                               0x00010010, 0x04010000, 0x04010410, 0x00010400,
                               0x04010400, 0x00010410, 0x00000400, 0x00000010,
                               0x04010000, 0x04000010, 0x04000400, 0x00000410,
                               0x00010400, 0x00010010, 0x04010010, 0x04010400,
                               0x00000410, 0x00000000, 0x00000000, 0x04010010,
                               0x04000010, 0x04000400, 0x00010410, 0x00010000,
                               0x00010410, 0x00010000, 0x04010400, 0x00000400,
                               0x00000010, 0x04010010, 0x00000400, 0x00010410,
                               0x04000400, 0x00000010, 0x04000010, 0x04010000,
                               0x04010010, 0x04000000, 0x00010000, 0x04000410,
                               0x00000000, 0x04010410, 0x00010010, 0x04000010,
                               0x04010000, 0x04000400, 0x04000410, 0x00000000,
                               0x04010410, 0x00010400, 0x00010400, 0x00000410,
                               0x00000410, 0x00010010, 0x04000000, 0x04010400 };

    static const U32 OrConstant[24] =
                             { 0x80000000, 0x40000000, 0x20000000, 0x10000000,
                               0x08000000, 0x04000000, 0x00800000, 0x00400000,
                               0x00200000, 0x00100000, 0x00080000, 0x00040000,
                               0x00008000, 0x00004000, 0x00002000, 0x00001000,
                               0x00000800, 0x00000400, 0x00000080, 0x00000040,
                               0x00000020, 0x00000010, 0x00000008, 0x00000004 };

    BYTE  XorClocks[8];
    U32   XorResult[2];
    U32   OrResult[32];
    U32   wv[16];              // Work Values
    int   i, j;


    // Xor together the two input clock values.
    for( i = 0; i <= 7; i++ )
        XorClocks[i] = pClock2[i] ^ pClock1[i];

    // Process the input pClock1 value. The first 7 bits of each of the 8 bytes
    // of the value are tested and, if the bit is on, several OrConstant[] values
    // are or'ed into several OrRresult[] values. Before that however, ensure the
    // OrRresult[] values contain nothing.
    for( i = 0; i <= 31; i++ )
        OrResult[i] = 0;

    // Process the STCK value.
    if ((pClock1[0] & 0x80 ) == 0x80)
    {
        OrResult[0]  |= OrConstant[11];
        OrResult[4]  |= OrConstant[2];
        OrResult[7]  |= OrConstant[10];
        OrResult[9]  |= OrConstant[2];
        OrResult[10] |= OrConstant[1];
        OrResult[12] |= OrConstant[7];
        OrResult[15] |= OrConstant[4];
        OrResult[18] |= OrConstant[3];
        OrResult[20] |= OrConstant[10];
        OrResult[23] |= OrConstant[1];
        OrResult[25] |= OrConstant[11];
        OrResult[26] |= OrConstant[9];
        OrResult[29] |= OrConstant[3];
        OrResult[31] |= OrConstant[7];
    }
    if ((pClock1[0] & 0x40 ) == 0x40)
    {
        OrResult[1]  |= OrConstant[6];
        OrResult[2]  |= OrConstant[1];
        OrResult[4]  |= OrConstant[7];
        OrResult[7]  |= OrConstant[4];
        OrResult[8]  |= OrConstant[6];
        OrResult[13] |= OrConstant[8];
        OrResult[14] |= OrConstant[4];
        OrResult[17] |= OrConstant[11];
        OrResult[18] |= OrConstant[9];
        OrResult[21] |= OrConstant[3];
        OrResult[22] |= OrConstant[11];
        OrResult[25] |= OrConstant[5];
        OrResult[26] |= OrConstant[8];
        OrResult[28] |= OrConstant[0];
        OrResult[31] |= OrConstant[2];
    }
    if ((pClock1[0] & 0x20 ) == 0x20)
    {
        OrResult[0]  |= OrConstant[3];
        OrResult[5]  |= OrConstant[8];
        OrResult[6]  |= OrConstant[4];
        OrResult[9]  |= OrConstant[0];
        OrResult[10] |= OrConstant[5];
        OrResult[13] |= OrConstant[7];
        OrResult[17] |= OrConstant[5];
        OrResult[18] |= OrConstant[8];
        OrResult[20] |= OrConstant[0];
        OrResult[23] |= OrConstant[6];
        OrResult[27] |= OrConstant[9];
        OrResult[30] |= OrConstant[6];
    }
    if ((pClock1[0] & 0x10 ) == 0x10)
    {
        OrResult[0]  |= OrConstant[21];
        OrResult[3]  |= OrConstant[22];
        OrResult[4]  |= OrConstant[14];
        OrResult[7]  |= OrConstant[16];
        OrResult[10] |= OrConstant[15];
        OrResult[12] |= OrConstant[20];
        OrResult[14] |= OrConstant[12];
        OrResult[17] |= OrConstant[19];
        OrResult[18] |= OrConstant[18];
        OrResult[21] |= OrConstant[18];
        OrResult[23] |= OrConstant[17];
        OrResult[25] |= OrConstant[20];
        OrResult[26] |= OrConstant[13];
        OrResult[30] |= OrConstant[17];
    }
    if ((pClock1[0] & 0x08 ) == 0x08)
    {
        OrResult[0]  |= OrConstant[13];
        OrResult[2]  |= OrConstant[23];
        OrResult[4]  |= OrConstant[17];
        OrResult[7]  |= OrConstant[22];
        OrResult[8]  |= OrConstant[14];
        OrResult[11] |= OrConstant[16];
        OrResult[14] |= OrConstant[15];
        OrResult[19] |= OrConstant[13];
        OrResult[21] |= OrConstant[19];
        OrResult[22] |= OrConstant[18];
        OrResult[25] |= OrConstant[18];
        OrResult[27] |= OrConstant[17];
        OrResult[29] |= OrConstant[20];
        OrResult[31] |= OrConstant[14];
    }
    if ((pClock1[0] & 0x04 ) == 0x04)
    {
        OrResult[0]  |= OrConstant[18];
        OrResult[3]  |= OrConstant[15];
        OrResult[4]  |= OrConstant[16];
        OrResult[6]  |= OrConstant[19];
        OrResult[9]  |= OrConstant[14];
        OrResult[10] |= OrConstant[23];
        OrResult[12] |= OrConstant[17];
        OrResult[15] |= OrConstant[22];
        OrResult[17] |= OrConstant[12];
        OrResult[19] |= OrConstant[23];
        OrResult[20] |= OrConstant[22];
        OrResult[23] |= OrConstant[21];
        OrResult[27] |= OrConstant[13];
        OrResult[29] |= OrConstant[19];
    }
    if ((pClock1[0] & 0x02 ) == 0x02)
    {
        OrResult[1]  |= OrConstant[21];
        OrResult[2]  |= OrConstant[15];
        OrResult[4]  |= OrConstant[20];
        OrResult[6]  |= OrConstant[12];
        OrResult[11] |= OrConstant[15];
        OrResult[12] |= OrConstant[16];
        OrResult[14] |= OrConstant[19];
        OrResult[17] |= OrConstant[20];
        OrResult[18] |= OrConstant[13];
        OrResult[22] |= OrConstant[21];
        OrResult[25] |= OrConstant[12];
        OrResult[27] |= OrConstant[23];
        OrResult[28] |= OrConstant[22];
    }
    if ((pClock1[1] & 0x80 ) == 0x80)
    {
        OrResult[1]  |= OrConstant[7];
        OrResult[2]  |= OrConstant[11];
        OrResult[5]  |= OrConstant[5];
        OrResult[6]  |= OrConstant[8];
        OrResult[8]  |= OrConstant[0];
        OrResult[11] |= OrConstant[6];
        OrResult[15] |= OrConstant[9];
        OrResult[17] |= OrConstant[4];
        OrResult[18] |= OrConstant[6];
        OrResult[23] |= OrConstant[8];
        OrResult[24] |= OrConstant[4];
        OrResult[27] |= OrConstant[0];
        OrResult[28] |= OrConstant[5];
        OrResult[31] |= OrConstant[3];
    }
    if ((pClock1[1] & 0x40 ) == 0x40)
    {
        OrResult[1]  |= OrConstant[2];
        OrResult[3]  |= OrConstant[6];
        OrResult[7]  |= OrConstant[9];
        OrResult[10] |= OrConstant[3];
        OrResult[12] |= OrConstant[10];
        OrResult[15] |= OrConstant[1];
        OrResult[16] |= OrConstant[4];
        OrResult[19] |= OrConstant[0];
        OrResult[20] |= OrConstant[5];
        OrResult[23] |= OrConstant[7];
        OrResult[26] |= OrConstant[2];
        OrResult[29] |= OrConstant[10];
        OrResult[30] |= OrConstant[0];
    }
    if ((pClock1[1] & 0x20 ) == 0x20)
    {
        OrResult[0]  |= OrConstant[6];
        OrResult[2]  |= OrConstant[3];
        OrResult[4]  |= OrConstant[10];
        OrResult[7]  |= OrConstant[1];
        OrResult[9]  |= OrConstant[11];
        OrResult[10] |= OrConstant[9];
        OrResult[13] |= OrConstant[3];
        OrResult[14] |= OrConstant[11];
        OrResult[18] |= OrConstant[2];
        OrResult[21] |= OrConstant[10];
        OrResult[23] |= OrConstant[2];
        OrResult[24] |= OrConstant[1];
        OrResult[26] |= OrConstant[7];
        OrResult[29] |= OrConstant[4];
    }
    if ((pClock1[1] & 0x10 ) == 0x10)
    {
        OrResult[0]  |= OrConstant[17];
        OrResult[2]  |= OrConstant[21];
        OrResult[5]  |= OrConstant[12];
        OrResult[7]  |= OrConstant[23];
        OrResult[8]  |= OrConstant[22];
        OrResult[11] |= OrConstant[21];
        OrResult[15] |= OrConstant[13];
        OrResult[16] |= OrConstant[12];
        OrResult[21] |= OrConstant[15];
        OrResult[22] |= OrConstant[16];
        OrResult[24] |= OrConstant[19];
        OrResult[27] |= OrConstant[14];
        OrResult[28] |= OrConstant[23];
    }
    if ((pClock1[1] & 0x08 ) == 0x08)
    {
        OrResult[1]  |= OrConstant[14];
        OrResult[2]  |= OrConstant[13];
        OrResult[6]  |= OrConstant[21];
        OrResult[9]  |= OrConstant[12];
        OrResult[11] |= OrConstant[23];
        OrResult[12] |= OrConstant[22];
        OrResult[15] |= OrConstant[21];
        OrResult[16] |= OrConstant[15];
        OrResult[18] |= OrConstant[20];
        OrResult[20] |= OrConstant[12];
        OrResult[25] |= OrConstant[15];
        OrResult[26] |= OrConstant[16];
        OrResult[28] |= OrConstant[19];
        OrResult[31] |= OrConstant[20];
    }
    if ((pClock1[1] & 0x04 ) == 0x04)
    {
        OrResult[2]  |= OrConstant[18];
        OrResult[5]  |= OrConstant[18];
        OrResult[7]  |= OrConstant[17];
        OrResult[9]  |= OrConstant[20];
        OrResult[10] |= OrConstant[13];
        OrResult[14] |= OrConstant[21];
        OrResult[17] |= OrConstant[22];
        OrResult[18] |= OrConstant[14];
        OrResult[21] |= OrConstant[16];
        OrResult[24] |= OrConstant[15];
        OrResult[26] |= OrConstant[20];
        OrResult[28] |= OrConstant[12];
        OrResult[31] |= OrConstant[19];
    }
    if ((pClock1[1] & 0x02 ) == 0x02)
    {
        OrResult[3]  |= OrConstant[21];
        OrResult[7]  |= OrConstant[13];
        OrResult[9]  |= OrConstant[19];
        OrResult[10] |= OrConstant[18];
        OrResult[13] |= OrConstant[18];
        OrResult[15] |= OrConstant[17];
        OrResult[16] |= OrConstant[19];
        OrResult[19] |= OrConstant[14];
        OrResult[20] |= OrConstant[23];
        OrResult[22] |= OrConstant[17];
        OrResult[25] |= OrConstant[22];
        OrResult[26] |= OrConstant[14];
        OrResult[29] |= OrConstant[16];
        OrResult[30] |= OrConstant[22];
    }
    if ((pClock1[2] & 0x80 ) == 0x80)
    {
        OrResult[1]  |= OrConstant[3];
        OrResult[3]  |= OrConstant[7];
        OrResult[6]  |= OrConstant[2];
        OrResult[9]  |= OrConstant[10];
        OrResult[11] |= OrConstant[2];
        OrResult[12] |= OrConstant[1];
        OrResult[14] |= OrConstant[7];
        OrResult[17] |= OrConstant[9];
        OrResult[20] |= OrConstant[3];
        OrResult[22] |= OrConstant[10];
        OrResult[25] |= OrConstant[1];
        OrResult[27] |= OrConstant[11];
        OrResult[28] |= OrConstant[9];
        OrResult[30] |= OrConstant[5];
    }
    if ((pClock1[2] & 0x40 ) == 0x40)
    {
        OrResult[0]  |= OrConstant[0];
        OrResult[3]  |= OrConstant[2];
        OrResult[4]  |= OrConstant[1];
        OrResult[6]  |= OrConstant[7];
        OrResult[9]  |= OrConstant[4];
        OrResult[10] |= OrConstant[6];
        OrResult[15] |= OrConstant[8];
        OrResult[17] |= OrConstant[1];
        OrResult[19] |= OrConstant[11];
        OrResult[20] |= OrConstant[9];
        OrResult[23] |= OrConstant[3];
        OrResult[24] |= OrConstant[11];
        OrResult[27] |= OrConstant[5];
        OrResult[28] |= OrConstant[8];
        OrResult[31] |= OrConstant[10];
    }
    if ((pClock1[2] & 0x20 ) == 0x20)
    {
        OrResult[2]  |= OrConstant[6];
        OrResult[7]  |= OrConstant[8];
        OrResult[8]  |= OrConstant[4];
        OrResult[11] |= OrConstant[0];
        OrResult[12] |= OrConstant[5];
        OrResult[15] |= OrConstant[7];
        OrResult[16] |= OrConstant[11];
        OrResult[19] |= OrConstant[5];
        OrResult[20] |= OrConstant[8];
        OrResult[22] |= OrConstant[0];
        OrResult[25] |= OrConstant[6];
        OrResult[29] |= OrConstant[9];
        OrResult[31] |= OrConstant[4];
    }
    if ((pClock1[2] & 0x10 ) == 0x10)
    {
        OrResult[2]  |= OrConstant[17];
        OrResult[5]  |= OrConstant[22];
        OrResult[6]  |= OrConstant[14];
        OrResult[9]  |= OrConstant[16];
        OrResult[12] |= OrConstant[15];
        OrResult[14] |= OrConstant[20];
        OrResult[17] |= OrConstant[13];
        OrResult[19] |= OrConstant[19];
        OrResult[20] |= OrConstant[18];
        OrResult[23] |= OrConstant[18];
        OrResult[25] |= OrConstant[17];
        OrResult[27] |= OrConstant[20];
        OrResult[28] |= OrConstant[13];
        OrResult[30] |= OrConstant[23];
    }
    if ((pClock1[2] & 0x08 ) == 0x08)
    {
        OrResult[1]  |= OrConstant[20];
        OrResult[3]  |= OrConstant[14];
        OrResult[4]  |= OrConstant[23];
        OrResult[6]  |= OrConstant[17];
        OrResult[9]  |= OrConstant[22];
        OrResult[10] |= OrConstant[14];
        OrResult[13] |= OrConstant[16];
        OrResult[17] |= OrConstant[21];
        OrResult[21] |= OrConstant[13];
        OrResult[23] |= OrConstant[19];
        OrResult[24] |= OrConstant[18];
        OrResult[27] |= OrConstant[18];
        OrResult[29] |= OrConstant[17];
        OrResult[30] |= OrConstant[19];
    }
    if ((pClock1[2] & 0x04 ) == 0x04)
    {
        OrResult[1]  |= OrConstant[19];
        OrResult[5]  |= OrConstant[15];
        OrResult[6]  |= OrConstant[16];
        OrResult[8]  |= OrConstant[19];
        OrResult[11] |= OrConstant[14];
        OrResult[12] |= OrConstant[23];
        OrResult[14] |= OrConstant[17];
        OrResult[16] |= OrConstant[21];
        OrResult[19] |= OrConstant[12];
        OrResult[21] |= OrConstant[23];
        OrResult[22] |= OrConstant[22];
        OrResult[25] |= OrConstant[21];
        OrResult[29] |= OrConstant[13];
        OrResult[30] |= OrConstant[12];
    }
    if ((pClock1[2] & 0x02 ) == 0x02)
    {
        OrResult[0]  |= OrConstant[22];
        OrResult[4]  |= OrConstant[15];
        OrResult[6]  |= OrConstant[20];
        OrResult[8]  |= OrConstant[12];
        OrResult[13] |= OrConstant[15];
        OrResult[14] |= OrConstant[16];
        OrResult[17] |= OrConstant[17];
        OrResult[19] |= OrConstant[20];
        OrResult[20] |= OrConstant[13];
        OrResult[24] |= OrConstant[21];
        OrResult[27] |= OrConstant[12];
        OrResult[29] |= OrConstant[23];
        OrResult[31] |= OrConstant[16];
    }
    if ((pClock1[3] & 0x80 ) == 0x80)
    {
        OrResult[0]  |= OrConstant[5];
        OrResult[3]  |= OrConstant[3];
        OrResult[4]  |= OrConstant[11];
        OrResult[7]  |= OrConstant[5];
        OrResult[8]  |= OrConstant[8];
        OrResult[10] |= OrConstant[0];
        OrResult[13] |= OrConstant[6];
        OrResult[16] |= OrConstant[7];
        OrResult[19] |= OrConstant[4];
        OrResult[20] |= OrConstant[6];
        OrResult[25] |= OrConstant[8];
        OrResult[26] |= OrConstant[4];
        OrResult[29] |= OrConstant[0];
        OrResult[30] |= OrConstant[9];
    }
    if ((pClock1[3] & 0x40 ) == 0x40)
    {
        OrResult[1]  |= OrConstant[10];
        OrResult[2]  |= OrConstant[0];
        OrResult[5]  |= OrConstant[6];
        OrResult[9]  |= OrConstant[9];
        OrResult[12] |= OrConstant[3];
        OrResult[14] |= OrConstant[10];
        OrResult[17] |= OrConstant[8];
        OrResult[18] |= OrConstant[4];
        OrResult[21] |= OrConstant[0];
        OrResult[22] |= OrConstant[5];
        OrResult[25] |= OrConstant[7];
        OrResult[28] |= OrConstant[2];
        OrResult[30] |= OrConstant[8];
    }
    if ((pClock1[3] & 0x20 ) == 0x20)
    {
        OrResult[1]  |= OrConstant[4];
        OrResult[4]  |= OrConstant[3];
        OrResult[6]  |= OrConstant[10];
        OrResult[9]  |= OrConstant[1];
        OrResult[11] |= OrConstant[11];
        OrResult[12] |= OrConstant[9];
        OrResult[15] |= OrConstant[3];
        OrResult[17] |= OrConstant[7];
        OrResult[20] |= OrConstant[2];
        OrResult[23] |= OrConstant[10];
        OrResult[25] |= OrConstant[2];
        OrResult[26] |= OrConstant[1];
        OrResult[28] |= OrConstant[7];
        OrResult[31] |= OrConstant[9];
    }
    if ((pClock1[3] & 0x10 ) == 0x10)
    {
        OrResult[0]  |= OrConstant[23];
        OrResult[4]  |= OrConstant[21];
        OrResult[7]  |= OrConstant[12];
        OrResult[9]  |= OrConstant[23];
        OrResult[10] |= OrConstant[22];
        OrResult[13] |= OrConstant[21];
        OrResult[16] |= OrConstant[20];
        OrResult[18] |= OrConstant[12];
        OrResult[23] |= OrConstant[15];
        OrResult[24] |= OrConstant[16];
        OrResult[26] |= OrConstant[19];
        OrResult[29] |= OrConstant[14];
        OrResult[30] |= OrConstant[13];
    }
    if ((pClock1[3] & 0x08 ) == 0x08)
    {
        OrResult[0]  |= OrConstant[19];
        OrResult[3]  |= OrConstant[20];
        OrResult[4]  |= OrConstant[13];
        OrResult[8]  |= OrConstant[21];
        OrResult[11] |= OrConstant[12];
        OrResult[13] |= OrConstant[23];
        OrResult[14] |= OrConstant[22];
        OrResult[18] |= OrConstant[15];
        OrResult[20] |= OrConstant[20];
        OrResult[22] |= OrConstant[12];
        OrResult[27] |= OrConstant[15];
        OrResult[28] |= OrConstant[16];
        OrResult[31] |= OrConstant[17];
    }
    if ((pClock1[3] & 0x04 ) == 0x04)
    {
        OrResult[0]  |= OrConstant[12];
        OrResult[3]  |= OrConstant[19];
        OrResult[4]  |= OrConstant[18];
        OrResult[7]  |= OrConstant[18];
        OrResult[9]  |= OrConstant[17];
        OrResult[11] |= OrConstant[20];
        OrResult[12] |= OrConstant[13];
        OrResult[16] |= OrConstant[17];
        OrResult[19] |= OrConstant[22];
        OrResult[20] |= OrConstant[14];
        OrResult[23] |= OrConstant[16];
        OrResult[26] |= OrConstant[15];
        OrResult[28] |= OrConstant[20];
        OrResult[31] |= OrConstant[13];
    }
    if ((pClock1[3] & 0x02 ) == 0x02)
    {
        OrResult[1]  |= OrConstant[16];
        OrResult[2]  |= OrConstant[22];
        OrResult[5]  |= OrConstant[21];
        OrResult[9]  |= OrConstant[13];
        OrResult[11] |= OrConstant[19];
        OrResult[12] |= OrConstant[18];
        OrResult[15] |= OrConstant[18];
        OrResult[16] |= OrConstant[16];
        OrResult[18] |= OrConstant[19];
        OrResult[21] |= OrConstant[14];
        OrResult[22] |= OrConstant[23];
        OrResult[24] |= OrConstant[17];
        OrResult[27] |= OrConstant[22];
        OrResult[28] |= OrConstant[14];
        OrResult[31] |= OrConstant[23];
    }
    if ((pClock1[4] & 0x80 ) == 0x80)
    {
        OrResult[0]  |= OrConstant[9];
        OrResult[2]  |= OrConstant[5];
        OrResult[5]  |= OrConstant[7];
        OrResult[8]  |= OrConstant[2];
        OrResult[11] |= OrConstant[10];
        OrResult[13] |= OrConstant[2];
        OrResult[14] |= OrConstant[1];
        OrResult[19] |= OrConstant[9];
        OrResult[22] |= OrConstant[3];
        OrResult[24] |= OrConstant[10];
        OrResult[27] |= OrConstant[1];
        OrResult[29] |= OrConstant[11];
        OrResult[31] |= OrConstant[0];
    }
    if ((pClock1[4] & 0x40 ) == 0x40)
    {
        OrResult[0]  |= OrConstant[8];
        OrResult[3]  |= OrConstant[10];
        OrResult[5]  |= OrConstant[2];
        OrResult[6]  |= OrConstant[1];
        OrResult[8]  |= OrConstant[7];
        OrResult[11] |= OrConstant[4];
        OrResult[12] |= OrConstant[6];
        OrResult[16] |= OrConstant[10];
        OrResult[19] |= OrConstant[1];
        OrResult[21] |= OrConstant[11];
        OrResult[22] |= OrConstant[9];
        OrResult[25] |= OrConstant[3];
        OrResult[26] |= OrConstant[11];
        OrResult[29] |= OrConstant[5];
        OrResult[30] |= OrConstant[2];
    }
    if ((pClock1[4] & 0x20 ) == 0x20)
    {
        OrResult[1]  |= OrConstant[9];
        OrResult[3]  |= OrConstant[4];
        OrResult[4]  |= OrConstant[6];
        OrResult[9]  |= OrConstant[8];
        OrResult[10] |= OrConstant[4];
        OrResult[13] |= OrConstant[0];
        OrResult[14] |= OrConstant[5];
        OrResult[17] |= OrConstant[3];
        OrResult[18] |= OrConstant[11];
        OrResult[21] |= OrConstant[5];
        OrResult[22] |= OrConstant[8];
        OrResult[24] |= OrConstant[0];
        OrResult[27] |= OrConstant[6];
        OrResult[30] |= OrConstant[7];
    }
    if ((pClock1[4] & 0x10 ) == 0x10)
    {
        OrResult[1]  |= OrConstant[1];
        OrResult[2]  |= OrConstant[4];
        OrResult[5]  |= OrConstant[0];
        OrResult[6]  |= OrConstant[5];
        OrResult[9]  |= OrConstant[7];
        OrResult[12] |= OrConstant[2];
        OrResult[15] |= OrConstant[10];
        OrResult[16] |= OrConstant[0];
        OrResult[19] |= OrConstant[6];
        OrResult[23] |= OrConstant[9];
        OrResult[26] |= OrConstant[3];
        OrResult[28] |= OrConstant[10];
        OrResult[31] |= OrConstant[8];
    }
    if ((pClock1[4] & 0x08 ) == 0x08)
    {
        OrResult[1]  |= OrConstant[17];
        OrResult[2]  |= OrConstant[19];
        OrResult[5]  |= OrConstant[14];
        OrResult[6]  |= OrConstant[23];
        OrResult[8]  |= OrConstant[17];
        OrResult[11] |= OrConstant[22];
        OrResult[12] |= OrConstant[14];
        OrResult[15] |= OrConstant[16];
        OrResult[16] |= OrConstant[22];
        OrResult[19] |= OrConstant[21];
        OrResult[23] |= OrConstant[13];
        OrResult[25] |= OrConstant[19];
        OrResult[26] |= OrConstant[18];
        OrResult[29] |= OrConstant[18];
        OrResult[30] |= OrConstant[16];
    }
    if ((pClock1[4] & 0x04 ) == 0x04)
    {
        OrResult[1]  |= OrConstant[13];
        OrResult[2]  |= OrConstant[12];
        OrResult[7]  |= OrConstant[15];
        OrResult[8]  |= OrConstant[16];
        OrResult[10] |= OrConstant[19];
        OrResult[13] |= OrConstant[14];
        OrResult[14] |= OrConstant[23];
        OrResult[18] |= OrConstant[21];
        OrResult[21] |= OrConstant[12];
        OrResult[23] |= OrConstant[23];
        OrResult[24] |= OrConstant[22];
        OrResult[27] |= OrConstant[21];
        OrResult[30] |= OrConstant[20];
    }
    if ((pClock1[4] & 0x02 ) == 0x02)
    {
        OrResult[1]  |= OrConstant[23];
        OrResult[3]  |= OrConstant[16];
        OrResult[6]  |= OrConstant[15];
        OrResult[8]  |= OrConstant[20];
        OrResult[10] |= OrConstant[12];
        OrResult[15] |= OrConstant[15];
        OrResult[17] |= OrConstant[18];
        OrResult[19] |= OrConstant[17];
        OrResult[21] |= OrConstant[20];
        OrResult[22] |= OrConstant[13];
        OrResult[26] |= OrConstant[21];
        OrResult[29] |= OrConstant[12];
        OrResult[30] |= OrConstant[14];
    }
    if ((pClock1[5] & 0x80 ) == 0x80)
    {
        OrResult[1]  |= OrConstant[0];
        OrResult[2]  |= OrConstant[9];
        OrResult[5]  |= OrConstant[3];
        OrResult[6]  |= OrConstant[11];
        OrResult[9]  |= OrConstant[5];
        OrResult[10] |= OrConstant[8];
        OrResult[12] |= OrConstant[0];
        OrResult[15] |= OrConstant[6];
        OrResult[16] |= OrConstant[1];
        OrResult[18] |= OrConstant[7];
        OrResult[21] |= OrConstant[4];
        OrResult[22] |= OrConstant[6];
        OrResult[27] |= OrConstant[8];
        OrResult[28] |= OrConstant[4];
        OrResult[31] |= OrConstant[11];
    }
    if ((pClock1[5] & 0x40 ) == 0x40)
    {
        OrResult[0]  |= OrConstant[2];
        OrResult[2]  |= OrConstant[8];
        OrResult[4]  |= OrConstant[0];
        OrResult[7]  |= OrConstant[6];
        OrResult[11] |= OrConstant[9];
        OrResult[14] |= OrConstant[3];
        OrResult[19] |= OrConstant[8];
        OrResult[20] |= OrConstant[4];
        OrResult[23] |= OrConstant[0];
        OrResult[24] |= OrConstant[5];
        OrResult[27] |= OrConstant[7];
        OrResult[31] |= OrConstant[5];
    }
    if ((pClock1[5] & 0x20 ) == 0x20)
    {
        OrResult[0]  |= OrConstant[7];
        OrResult[3]  |= OrConstant[9];
        OrResult[6]  |= OrConstant[3];
        OrResult[8]  |= OrConstant[10];
        OrResult[11] |= OrConstant[1];
        OrResult[13] |= OrConstant[11];
        OrResult[14] |= OrConstant[9];
        OrResult[16] |= OrConstant[5];
        OrResult[19] |= OrConstant[7];
        OrResult[22] |= OrConstant[2];
        OrResult[25] |= OrConstant[10];
        OrResult[27] |= OrConstant[2];
        OrResult[28] |= OrConstant[1];
    }
    if ((pClock1[5] & 0x10 ) == 0x10)
    {
        OrResult[1]  |= OrConstant[8];
        OrResult[3]  |= OrConstant[1];
        OrResult[5]  |= OrConstant[11];
        OrResult[6]  |= OrConstant[9];
        OrResult[9]  |= OrConstant[3];
        OrResult[10] |= OrConstant[11];
        OrResult[13] |= OrConstant[5];
        OrResult[14] |= OrConstant[8];
        OrResult[17] |= OrConstant[10];
        OrResult[19] |= OrConstant[2];
        OrResult[20] |= OrConstant[1];
        OrResult[22] |= OrConstant[7];
        OrResult[25] |= OrConstant[4];
        OrResult[26] |= OrConstant[6];
        OrResult[30] |= OrConstant[10];
    }
    if ((pClock1[5] & 0x08 ) == 0x08)
    {
        OrResult[0]  |= OrConstant[16];
        OrResult[3]  |= OrConstant[17];
        OrResult[5]  |= OrConstant[20];
        OrResult[6]  |= OrConstant[13];
        OrResult[10] |= OrConstant[21];
        OrResult[13] |= OrConstant[12];
        OrResult[15] |= OrConstant[23];
        OrResult[17] |= OrConstant[16];
        OrResult[20] |= OrConstant[15];
        OrResult[22] |= OrConstant[20];
        OrResult[24] |= OrConstant[12];
        OrResult[29] |= OrConstant[15];
        OrResult[31] |= OrConstant[18];
    }
    if ((pClock1[5] & 0x04 ) == 0x04)
    {
        OrResult[0]  |= OrConstant[20];
        OrResult[3]  |= OrConstant[13];
        OrResult[5]  |= OrConstant[19];
        OrResult[6]  |= OrConstant[18];
        OrResult[9]  |= OrConstant[18];
        OrResult[11] |= OrConstant[17];
        OrResult[13] |= OrConstant[20];
        OrResult[14] |= OrConstant[13];
        OrResult[16] |= OrConstant[23];
        OrResult[18] |= OrConstant[17];
        OrResult[21] |= OrConstant[22];
        OrResult[22] |= OrConstant[14];
        OrResult[25] |= OrConstant[16];
        OrResult[28] |= OrConstant[15];
    }
    if ((pClock1[5] & 0x02 ) == 0x02)
    {
        OrResult[0]  |= OrConstant[14];
        OrResult[3]  |= OrConstant[23];
        OrResult[4]  |= OrConstant[22];
        OrResult[7]  |= OrConstant[21];
        OrResult[11] |= OrConstant[13];
        OrResult[13] |= OrConstant[19];
        OrResult[14] |= OrConstant[18];
        OrResult[17] |= OrConstant[15];
        OrResult[18] |= OrConstant[16];
        OrResult[20] |= OrConstant[19];
        OrResult[23] |= OrConstant[14];
        OrResult[24] |= OrConstant[23];
        OrResult[26] |= OrConstant[17];
        OrResult[29] |= OrConstant[22];
        OrResult[31] |= OrConstant[12];
    }
    if ((pClock1[6] & 0x80 ) == 0x80)
    {
        OrResult[1]  |= OrConstant[11];
        OrResult[3]  |= OrConstant[0];
        OrResult[4]  |= OrConstant[5];
        OrResult[7]  |= OrConstant[7];
        OrResult[10] |= OrConstant[2];
        OrResult[13] |= OrConstant[10];
        OrResult[15] |= OrConstant[2];
        OrResult[17] |= OrConstant[6];
        OrResult[21] |= OrConstant[9];
        OrResult[24] |= OrConstant[3];
        OrResult[26] |= OrConstant[10];
        OrResult[29] |= OrConstant[1];
        OrResult[30] |= OrConstant[4];
    }
    if ((pClock1[6] & 0x40 ) == 0x40)
    {
        OrResult[1]  |= OrConstant[5];
        OrResult[2]  |= OrConstant[2];
        OrResult[5]  |= OrConstant[10];
        OrResult[7]  |= OrConstant[2];
        OrResult[8]  |= OrConstant[1];
        OrResult[10] |= OrConstant[7];
        OrResult[13] |= OrConstant[4];
        OrResult[14] |= OrConstant[6];
        OrResult[16] |= OrConstant[3];
        OrResult[18] |= OrConstant[10];
        OrResult[21] |= OrConstant[1];
        OrResult[23] |= OrConstant[11];
        OrResult[24] |= OrConstant[9];
        OrResult[27] |= OrConstant[3];
        OrResult[28] |= OrConstant[11];
    }
    if ((pClock1[6] & 0x20 ) == 0x20)
    {
        OrResult[2]  |= OrConstant[7];
        OrResult[5]  |= OrConstant[4];
        OrResult[6]  |= OrConstant[6];
        OrResult[11] |= OrConstant[8];
        OrResult[12] |= OrConstant[4];
        OrResult[15] |= OrConstant[0];
        OrResult[16] |= OrConstant[9];
        OrResult[19] |= OrConstant[3];
        OrResult[20] |= OrConstant[11];
        OrResult[23] |= OrConstant[5];
        OrResult[24] |= OrConstant[8];
        OrResult[26] |= OrConstant[0];
        OrResult[29] |= OrConstant[6];
        OrResult[30] |= OrConstant[1];
    }
    if ((pClock1[6] & 0x10 ) == 0x10)
    {
        OrResult[0]  |= OrConstant[10];
        OrResult[3]  |= OrConstant[8];
        OrResult[4]  |= OrConstant[4];
        OrResult[7]  |= OrConstant[0];
        OrResult[8]  |= OrConstant[5];
        OrResult[11] |= OrConstant[7];
        OrResult[14] |= OrConstant[2];
        OrResult[16] |= OrConstant[8];
        OrResult[18] |= OrConstant[0];
        OrResult[21] |= OrConstant[6];
        OrResult[25] |= OrConstant[9];
        OrResult[28] |= OrConstant[3];
    }
    if ((pClock1[6] & 0x08 ) == 0x08)
    {
        OrResult[1]  |= OrConstant[18];
        OrResult[2]  |= OrConstant[16];
        OrResult[4]  |= OrConstant[19];
        OrResult[7]  |= OrConstant[14];
        OrResult[8]  |= OrConstant[23];
        OrResult[10] |= OrConstant[17];
        OrResult[13] |= OrConstant[22];
        OrResult[14] |= OrConstant[14];
        OrResult[17] |= OrConstant[23];
        OrResult[18] |= OrConstant[22];
        OrResult[21] |= OrConstant[21];
        OrResult[25] |= OrConstant[13];
        OrResult[27] |= OrConstant[19];
        OrResult[28] |= OrConstant[18];
        OrResult[31] |= OrConstant[15];
    }
    if ((pClock1[6] & 0x04 ) == 0x04)
    {
        OrResult[2]  |= OrConstant[20];
        OrResult[4]  |= OrConstant[12];
        OrResult[9]  |= OrConstant[15];
        OrResult[10] |= OrConstant[16];
        OrResult[12] |= OrConstant[19];
        OrResult[15] |= OrConstant[14];
        OrResult[16] |= OrConstant[13];
        OrResult[20] |= OrConstant[21];
        OrResult[23] |= OrConstant[12];
        OrResult[25] |= OrConstant[23];
        OrResult[26] |= OrConstant[22];
        OrResult[29] |= OrConstant[21];
        OrResult[30] |= OrConstant[15];
    }
    if ((pClock1[6] & 0x02 ) == 0x02)
    {
        OrResult[1]  |= OrConstant[12];
        OrResult[2]  |= OrConstant[14];
        OrResult[5]  |= OrConstant[16];
        OrResult[8]  |= OrConstant[15];
        OrResult[10] |= OrConstant[20];
        OrResult[12] |= OrConstant[12];
        OrResult[16] |= OrConstant[18];
        OrResult[19] |= OrConstant[18];
        OrResult[21] |= OrConstant[17];
        OrResult[23] |= OrConstant[20];
        OrResult[24] |= OrConstant[13];
        OrResult[28] |= OrConstant[21];
        OrResult[31] |= OrConstant[22];
    }
    if ((pClock1[7] & 0x80 ) == 0x80)
    {
        OrResult[0]  |= OrConstant[4];
        OrResult[3]  |= OrConstant[11];
        OrResult[4]  |= OrConstant[9];
        OrResult[7]  |= OrConstant[3];
        OrResult[8]  |= OrConstant[11];
        OrResult[11] |= OrConstant[5];
        OrResult[12] |= OrConstant[8];
        OrResult[14] |= OrConstant[0];
        OrResult[17] |= OrConstant[2];
        OrResult[18] |= OrConstant[1];
        OrResult[20] |= OrConstant[7];
        OrResult[23] |= OrConstant[4];
        OrResult[24] |= OrConstant[6];
        OrResult[29] |= OrConstant[8];
        OrResult[31] |= OrConstant[1];
    }
    if ((pClock1[7] & 0x40 ) == 0x40)
    {
        OrResult[3]  |= OrConstant[5];
        OrResult[4]  |= OrConstant[8];
        OrResult[6]  |= OrConstant[0];
        OrResult[9]  |= OrConstant[6];
        OrResult[13] |= OrConstant[9];
        OrResult[16] |= OrConstant[6];
        OrResult[21] |= OrConstant[8];
        OrResult[22] |= OrConstant[4];
        OrResult[25] |= OrConstant[0];
        OrResult[26] |= OrConstant[5];
        OrResult[29] |= OrConstant[7];
        OrResult[30] |= OrConstant[11];
    }
    if ((pClock1[7] & 0x20 ) == 0x20)
    {
        OrResult[0]  |= OrConstant[1];
        OrResult[5]  |= OrConstant[9];
        OrResult[8]  |= OrConstant[3];
        OrResult[10] |= OrConstant[10];
        OrResult[13] |= OrConstant[1];
        OrResult[15] |= OrConstant[11];
        OrResult[17] |= OrConstant[0];
        OrResult[18] |= OrConstant[5];
        OrResult[21] |= OrConstant[7];
        OrResult[24] |= OrConstant[2];
        OrResult[27] |= OrConstant[10];
        OrResult[29] |= OrConstant[2];
        OrResult[31] |= OrConstant[6];
    }
    if ((pClock1[7] & 0x10 ) == 0x10)
    {
        OrResult[2]  |= OrConstant[10];
        OrResult[5]  |= OrConstant[1];
        OrResult[7]  |= OrConstant[11];
        OrResult[8]  |= OrConstant[9];
        OrResult[11] |= OrConstant[3];
        OrResult[12] |= OrConstant[11];
        OrResult[15] |= OrConstant[5];
        OrResult[16] |= OrConstant[2];
        OrResult[19] |= OrConstant[10];
        OrResult[21] |= OrConstant[2];
        OrResult[22] |= OrConstant[1];
        OrResult[24] |= OrConstant[7];
        OrResult[27] |= OrConstant[4];
        OrResult[28] |= OrConstant[6];
        OrResult[30] |= OrConstant[3];
    }
    if ((pClock1[7] & 0x08 ) == 0x08)
    {
        OrResult[1]  |= OrConstant[15];
        OrResult[3]  |= OrConstant[18];
        OrResult[5]  |= OrConstant[17];
        OrResult[7]  |= OrConstant[20];
        OrResult[8]  |= OrConstant[13];
        OrResult[12] |= OrConstant[21];
        OrResult[15] |= OrConstant[12];
        OrResult[16] |= OrConstant[14];
        OrResult[19] |= OrConstant[16];
        OrResult[22] |= OrConstant[15];
        OrResult[24] |= OrConstant[20];
        OrResult[26] |= OrConstant[12];
        OrResult[30] |= OrConstant[18];
    }
    if ((pClock1[7] & 0x04 ) == 0x04)
    {
        OrResult[0]  |= OrConstant[15];
        OrResult[5]  |= OrConstant[13];
        OrResult[7]  |= OrConstant[19];
        OrResult[8]  |= OrConstant[18];
        OrResult[11] |= OrConstant[18];
        OrResult[13] |= OrConstant[17];
        OrResult[15] |= OrConstant[20];
        OrResult[17] |= OrConstant[14];
        OrResult[18] |= OrConstant[23];
        OrResult[20] |= OrConstant[17];
        OrResult[23] |= OrConstant[22];
        OrResult[24] |= OrConstant[14];
        OrResult[27] |= OrConstant[16];
        OrResult[31] |= OrConstant[21];
    }
    if ((pClock1[7] & 0x02 ) == 0x02)
    {
        OrResult[1]  |= OrConstant[22];
        OrResult[3]  |= OrConstant[12];
        OrResult[5]  |= OrConstant[23];
        OrResult[6]  |= OrConstant[22];
        OrResult[9]  |= OrConstant[21];
        OrResult[13] |= OrConstant[13];
        OrResult[15] |= OrConstant[19];
        OrResult[19] |= OrConstant[15];
        OrResult[20] |= OrConstant[16];
        OrResult[22] |= OrConstant[19];
        OrResult[25] |= OrConstant[14];
        OrResult[26] |= OrConstant[23];
        OrResult[28] |= OrConstant[17];
        OrResult[30] |= OrConstant[21];
    }

    // Process the 32 OrResult[] values and the XorClocks value
    // to produce 2 XorResult[] values.
    FETCH_FW( wv[3], XorClocks+0 );
    FETCH_FW( wv[4], XorClocks+4 );
    j = 30;                              // Index to last two OrResult[] values.
    wv[0] = 252;                         // 252 = 0x000000fc
    wv[9] = 16;                          //  16 = 0x00000010
    wv[2] = wv[3];
    wv[5] = wv[4];
    shift_right_dbl( &wv[2], &wv[3], 1 );
    shift_right_dbl( &wv[4], &wv[5], 1 );

    for( i = 0; i <= 15; i++ )
    {
        wv[2] = wv[5];
        wv[6] = wv[5];
        wv[7] = wv[5];
        shift_right_dbl( &wv[6], &wv[7], 28 );
        wv[5] ^= OrResult[j];
        wv[7] ^= OrResult[j + 1];
        shift_left_dbl( &wv[4], &wv[5], 8 );
        shift_left_dbl( &wv[6], &wv[7], 8 );
        wv[1] = wv[4];
        wv[14] = wv[6];
        shift_left_dbl( &wv[4], &wv[5], 8 );
        shift_left_dbl( &wv[6], &wv[7], 8 );
        wv[1] &= wv[0];
        wv[14] &= wv[0];
        wv[4] &= wv[0];
        wv[6] &= wv[0];
        wv[3] ^= XorConstant[0 + ( wv[1] / 4)];
        wv[3] ^= XorConstant[64 + ( wv[14] / 4)];
        wv[3] ^= XorConstant[128 + ( wv[4] / 4 )];
        wv[3] ^= XorConstant[192 + ( wv[6] / 4 )];
        shift_left_dbl( &wv[4], &wv[5], 8 );
        shift_left_dbl( &wv[6], &wv[7], 8 );
        wv[1] = wv[4];
        wv[14] = wv[6];
        shift_left_dbl( &wv[4], &wv[5], 8 );
        shift_left_dbl( &wv[6], &wv[7], 8 );
        wv[1] &= wv[0];
        wv[14] &= wv[0];
        wv[4] &= wv[0];
        wv[6] &= wv[0];
        wv[3] ^= XorConstant[256 + ( wv[1] / 4 )];
        wv[3] ^= XorConstant[320 + ( wv[14] / 4 )];
        wv[3] ^= XorConstant[384 + ( wv[4] / 4 )];
        wv[3] ^= XorConstant[448 + ( wv[6] / 4 )];
        wv[5] = wv[3];
        wv[3] = wv[2];
        j = j - 2;                       // Index to previous two OrResult[] values.
    }

    wv[6] = wv[5];
    wv[7] = wv[6];
    shift_left_dbl( &wv[2], &wv[3], 1 );
    shift_left_dbl( &wv[6], &wv[7], 1 );
    wv[3] = wv[6];
    wv[4] = wv[2];
    XorResult[0] = wv[3];
    XorResult[1] = wv[4];

    // Return the 2 XorResult[] values in the output pToken value.
    STORE_FW( pToken+0, XorResult[0] );
    STORE_FW( pToken+4, XorResult[1] );

    return;
}   /* End function  gen_csv_sid() */

/* ------------------------------------------------------------------ */
/* shift_left_dbl() and shift_right_dl()                              */
/* ------------------------------------------------------------------ */
// These two functions emulate the action of the S/390 SLDL and SRDL
// machine instructions. The machine instructions treat an even-odd
// pair of 32-bit registers as a 64-bit value to be shifted left or
// right the specified number of bits. The machine instructions only
// require the even numbered register to be specified; these emulations
// require both 32-bit values to be specified.

void  shift_left_dbl( U32* even, U32* odd, int number )
{
    U64     dw;

    dw = *even;              // Combine the two
    dw <<= 32;               // U32 values into
    dw |= *odd;              // one U64 value

    dw <<= number;           // Shift left the appropriate number

    *odd = (U32)dw;          // Separate the U64
    dw >>= 32;               // value into
    *even = (U32)dw;         // two U32 values.

    return;
}   /* End function  shift_left_dbl() */

void  shift_right_dbl( U32* even, U32* odd, int number )
{
    U64     dw;

    dw = *even;              // Combine the two
    dw <<= 32;               // U32 values into
    dw |= *odd;              // one U64 value

    dw >>= number;           // Shift right the appropriate number

    *odd = (U32)dw;          // Separate the U64
    dw >>= 32;               // value into
    *even = (U32)dw;         // two U32 values.

    return;
}   /* End function  shift_right_dbl() */


#if defined(ENABLE_IPV6)
/* ------------------------------------------------------------------ */
/* calculate_icmpv6_checksum()                                        */
/* ------------------------------------------------------------------ */
// This is not a general purpose function, it is solely for calculating
// the checksum of ICMPv6 packets sent to the guest on the y-side. The
// following restriction apply:-
// - If the ICMPv6 header and message has a length that is an odd number
//   of bytes, they must be followed by a byte containing zero.
// - Any existing checksum in the ICMPv6 header will be over-written.
//
// The ICMPv6 header and message layout is:-
//   byte  0    Type: specifies the format of the message.
//   byte  1    Code: further qualifies the message
//   bytes 2-3  Checksum.
//   byte  4-n  Message.
//
// The Hop-by-Hop Options extension header layout is:-
//   byte  0    Next Header: will be 58 (0x3a), ICMPv6 header.
//   byte  1    Header Extension Length: the header's overall length
//              0 = 8-bytes, 1 = 16-bytes, 2 = 24-bytes, etc.
//   byte  2-n  One or more option fields.
//

void  calculate_icmpv6_checksum( PIP6FRM pIP6FRM, BYTE* pIcmpHdr, int iIcmpLen )
{
    BYTE*      pBytePtr;
    int        i,j;
    U16        uTwobytes;            // Two bytes of data
    U32        uHighhalf;            // High-order half of the checksum
    U32        uChecksum;            // The checksum
    BYTE       bPseudoHeader[40];    //  0-15  Source address
                                     // 16-31  Destination address
                                     // 32-35  Upper-layer packet length
                                     // 36-38  zero
                                     //   39   Next Header (i.e. 58 (0x3a))


    // Clear the checksum in the ICMP header before calculating the checksum.
    STORE_HW( pIcmpHdr+2, 0x0000 );

    // Construct the Psuedo-Header for the checksum calcuation.
    memcpy( bPseudoHeader+0, pIP6FRM->bSrcAddr, 16 );
    memcpy( bPseudoHeader+16, pIP6FRM->bDstAddr, 16 );
    STORE_FW( bPseudoHeader+32, iIcmpLen );
    for( i = 36; i <= 38; i++ )
        bPseudoHeader[i] = 0x00;
    bPseudoHeader[39] = 0x3A;

    // Calculate the checksum.
    uChecksum = 0;
    for( i = 0; i <= 38; i += 2 )
    {
        FETCH_HW( uTwobytes, bPseudoHeader+i );
        uChecksum += uTwobytes;
    }
    pBytePtr = pIcmpHdr;                   // Point to the ICMPv6 header.
    j = iIcmpLen;                          // Get the length of the
    j++;                                   // ICMPv6 packet rounded up
    j &= 0xFFFFFFFE;                       // to the next multiple of two.
    for( i = 0; i <= j - 2; i += 2 )
    {
        FETCH_HW( uTwobytes, pBytePtr );
        uChecksum += uTwobytes;
        pBytePtr += 2;
    }
    uHighhalf = uChecksum >> 16;           // Get the high-order half
                                           // of the checksum value.
    uChecksum &= 0x0000FFFF;               // Get the low-order half.
    uChecksum += uHighhalf;                // Add the high-order half
                                           // to the low-order half.
    uHighhalf = uChecksum >> 16;           // Get the high-order half
                                           // of the checksum value again.
    uChecksum &= 0x0000FFFF;               // Get the low-order half.
    uChecksum += uHighhalf;                // Add the high-order half
                                           // to the low-order half again
                                           // to include any carry.
    uChecksum ^= 0xFFFFFFFF;               // Complement the bits.
    uChecksum &= 0x0000FFFF;               // Get a clean checksum.
    uTwobytes = uChecksum;                 // Copy to a two-byte value.

    // Set the checksum in the ICMP header.
    STORE_HW( pIcmpHdr+2, uTwobytes );

    return;
}   /* End function  calculate_icmpv6_checksum() */
#endif /* defined(ENABLE_IPV6) */


/* ------------------------------------------------------------------ */
/* HDL stuff                                                          */
/* ------------------------------------------------------------------ */

/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */

#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev hdtptp_LTX_hdl_ddev
#define hdl_depc hdtptp_LTX_hdl_depc
#define hdl_reso hdtptp_LTX_hdl_reso
#define hdl_init hdtptp_LTX_hdl_init
#define hdl_fini hdtptp_LTX_hdl_fini
#endif


HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY( HERCULES );
     HDL_DEPENDENCY( DEVBLK );
     HDL_DEPENDENCY( SYSBLK );
     HDL_DEPENDENCY( REGS );
}
END_DEPENDENCY_SECTION


HDL_RESOLVER_SECTION;
{
   #if defined( WIN32 ) && !defined( _MSVC_ ) && !defined( HDL_USE_LIBTOOL )
     #undef sysblk
     HDL_RESOLVE_SYMPTR( psysblk, sysblk );
     HDL_RESOLVE( etod_clock );
     HDL_RESOLVE( device_attention );
   #else
     UNREFERENCED( getsym );    // (HDL_RESOLVER_SECTION parameter)
   #endif
}
END_RESOLVER_SECTION


HDL_REGISTER_SECTION;
{
    //              Hercules's          Our
    //              registered          overriding
    //              entry-point         entry-point
    //              name                value

  #if defined( OPTION_W32_CTCI )
    HDL_REGISTER ( debug_tt32_stats,   display_tt32_stats        );
    HDL_REGISTER ( debug_tt32_tracing, enable_tt32_debug_tracing );
  #else
    UNREFERENCED( regsym );     // (HDL_REGISTER_SECTION parameter)
  #endif
}
END_REGISTER_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE( PTP, ptp_device_hndinfo );
}
END_DEVICE_SECTION
