/* TAPEDEV.H    (C) Copyright Ivan Warren and others, 2003-2012      */
/*              (C) Copyright TurboHercules, SAS 2011                */
/*              Tape Device Handler Structure Definitions            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This header file contains tape related structures and defines     */
/* for the Hercules ESA/390 emulator.                                */
/*-------------------------------------------------------------------*/

#ifndef _TAPEDEV_H_
#define _TAPEDEV_H_

#include "scsitape.h"       /* SCSI Tape handling functions          */
#include "htypes.h"         /* Hercules struct typedefs              */
#include "opcode.h"         /* device_attention, SETMODE, etc.       */
#include "parser.h"         /* generic parameter string parser       */

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define MAX_TAPE_BLKSIZE        2097152 /* Maximum blocksize = 2MB   */
#define TAPE_UNLOADED           "*"     /* Name for unloaded drive   */

/*-------------------------------------------------------------------*/
/* Read Device Characteristics (RDC) structure                       */
/*-------------------------------------------------------------------*/
struct TAPERDC
{
    HWORD   cutype;         //  0-1     // Control unit type
    BYTE    cumodel;        //  2       // Control unit model
    HWORD   tutype;         //  3-4     // Tape unit type
    BYTE    tumodel;        //  5       // Tape unit model

    /*---------------------------------------------------------------*/

    FWORD   feats1;         //  6-9     // Features and Facilities 1

#define TRDC_NTP        0x01000000      //  New tape product (NTP)
                                        //  (i.e. 3590 or greater)
                                        //  RDC 19-29 defined/valid
#define TRDC_RBL30      0x00800000      //  RBL-30 media info avail
#define TRDC_BLKID      0x00004000      //  Bits 0-9 of blkid 0 for locate
#define TRDC_ASYNCMSG   0x00001000      //  Asynchronous message support
#define TRDC_SIC        0x00000800      //  Special Intercept Condition
#define TRDC_CHPNOOP    0x00000400      //  Channel path no-op support
#define TRDC_LWP        0x00000200      //  Logical write-protect support
#define TRDC_EBL        0x00000100      //  Extended buffered log active
                                        //  On: 64 bytes of Format-30 data
                                        //  for ERA 2B w/ERA 50-52 support
                                        //  Off: only 32-bytes of Format-21
                                        //  data for ERA 2A/2B.
#define TRDC_ACL        0x00000080      //  Automatic Cartridge Loader
#define TRDC_IDR        0x00000040      //  Improved Data Recording
#define TRDC_SVF        0x00000020      //  Suppress Volume Fencing
#define TRDC_LIBO       0x00000010      //  Library Interface online
#define TRDC_LIBA       0x00000008      //  Library Attachment Facility

    /*---------------------------------------------------------------*/

    BYTE    devclass;       //  10      // Device class code
    BYTE    devtcode;       //  11      // Device type code

    /*---------------------------------------------------------------*/

    FWORD   feats2;         //  12-15   // Features and Facilities 2

#define TRDC_RDFWD      0x00800000      //  NTP: Read Forward support
#define TRDC_2_BLK2DCE  0x00100000      //  (unknown, but if set copies
                                        //   both maxblk/recblk to DCE)
#define TRDC_MODSET     0x00040000      //  NTP: Mode set/sense support
#define TRDC_SSC        0x00002000      //  NTP: Set System Char. support
#define TRDC_MEDSNS     0x00000400      //  NTP: Medium Sense/Char. support

    /*---------------------------------------------------------------*/

    BYTE    dtmodel;        //  16      // NTP: Device type model
    BYTE    devmfg;         //  17      // NTP: Device manufacturer
    BYTE    plantmfg;       //  18      // NTP: Plant of manufacture

    /*---------------------------------------------------------------*/
    /*  Bytes 19-29:  NTP: only valid if feat1 TRDC_NTP is on        */

    BYTE    rsrvd1;         //  19      // NTP: Reserved
    BYTE    rsrvd2;         //  20      // NTP: Reserved
    BYTE    rsrvd3;         //  21      // NTP: Reserved
    BYTE    rsrvd4;         //  22      // NTP: Reserved
    BYTE    rsrvd5;         //  23      // NTP: Reserved

    HWORD   ncutype;        //  24-25   // NTP: Control unit type
    BYTE    ncumodel;       //  26      // NTP: Control unit model

    HWORD   ntutype;        //  27-28   // NTP: Device type
    BYTE    ntumodel;       //  29      // NTP: Device model

    /*---------------------------------------------------------------*/

    BYTE    rsrvd6;         //  30      // Reserved

    /*---------------------------------------------------------------*/

    BYTE    feat5;          //  31      // Features byte 5
                                        // NTP: ERDS physical id
#define TRDC_ENCR       0x13            //  Encryption support
#define TRDC_ENCR2      0x15            //  Encryption 2 support
#define TRDC_92E5XF     0x13            //  NTP: 3592-E05 Extended features

    /*---------------------------------------------------------------*/

    BYTE    libseq[3];      //  32-34   // NTP: Library sequence number
    BYTE    libid;          //  35      // NTP: Library subsystem id
    HWORD   vldrid;         //  36-37   // NTP: Volume loader id
    BYTE    devid;          //  38      // NTP: Device id
    BYTE    rsrvd7;         //  39      // Reserved

    /*---------------------------------------------------------------*/

    BYTE    mdr;            //  40      // NTP: MDR id

#define TRDC_MDR48      0x41            //  MDR: 3480
#define TRDC_MDR49      0x42            //  MDR: 3490
#define TRDC_MDR59      0x46            //  MDR: 3590

    BYTE    obr;            //  41      // NTP: OBR id

#define TRDC_OBR48      0x80            //  OBR: 3480
#define TRDC_OBR49      0x81            //  OBR: 3490
#define TRDC_OBR59      0x83            //  OBR: 3590

    BYTE    mvscode;        //  42      // NTP: MVS Dev Code/Class/Type

    /*---------------------------------------------------------------*/

    FWORD   maxblk;         //  43-46   // NTP: Maximum block size
    FWORD   recblk;         //  47-50   // NTP: Recommended block size

    /*---------------------------------------------------------------*/

    BYTE    rsrvd8[2];      //  51-52   // Reserved
    BYTE    rsrvd9[3];      //  53-55   // Reserved
    HWORD   medid;          //  56-57   // NTP: Media id (0x0400, 0x0380)
    BYTE    medfmt;         //  58      // NTP: Media format id (0x00)
    BYTE    wrapped;        //  59      // NTP: Wrapped flag
    BYTE    rsrvd10[4];      //  60-63   // Reserved
};
typedef struct TAPERDC  TAPERDC;

CASSERT( sizeof(TAPERDC) == sizeof(((DEVBLK*)0)->devchar), tapedev_h );

/*-------------------------------------------------------------------*/
/* Just some shorter names                                           */
/*-------------------------------------------------------------------*/
#define M48         (TRDC_MDR48)
#define M49         (TRDC_MDR49)
#define M59         (TRDC_MDR59)

#define O48         (TRDC_OBR48)
#define O49         (TRDC_OBR49)
#define O59         (TRDC_OBR59)

#define BLK64       ((64 * 1024)-1)
#define BLK256      (256 * 1024)
#define BLKMAX      (MAX_TAPE_BLKSIZE)

#define FEAT1_3480  (TRDC_LWP | TRDC_ACL | TRDC_IDR)
#define FEAT1_3490  (TRDC_LWP | TRDC_ACL | TRDC_IDR)

#define FEAT2_3480  (0)
#define FEAT2_3490  (0)

#define FEAT1_3590  (0                      \
                     | TRDC_NTP             \
                     | TRDC_BLKID           \
                     | TRDC_SIC             \
                     | TRDC_CHPNOOP         \
                     | TRDC_LWP             \
                     | TRDC_ACL             \
                     | TRDC_IDR             \
                    )

#define FEAT2_3590  (0                      \
                     | TRDC_RDFWD           \
                     | TRDC_2_BLK2DCE       \
                     | TRDC_MEDSNS          \
                    )

/*-------------------------------------------------------------------*/
/* Helper macros to check RDC for feature support / enablement       */
/*-------------------------------------------------------------------*/
static inline bool is_tape_feat( const BYTE* feats, U32 flags )
{
    U32 work;
    memcpy( &work, feats, sizeof( U32 ));
    return (CSWAP32( work ) & flags) ? true : false;
}

#define TAPERDC_FEATS1          &dev->devchar[0] + offsetof( TAPERDC, feats1 )
#define TAPERDC_FEATS2          &dev->devchar[0] + offsetof( TAPERDC, feats2 )

#define SIC_SUPPORTED()         is_tape_feat( TAPERDC_FEATS1, TRDC_SIC    )
#define IDR_SUPPORTED()         is_tape_feat( TAPERDC_FEATS1, TRDC_IDR    )
#define SVF_ENABLED()           is_tape_feat( TAPERDC_FEATS1, TRDC_SVF    )
#define RDFWD_SUPPORTED()       is_tape_feat( TAPERDC_FEATS2, TRDC_RDFWD  )
#define MEDSNS_SUPPORTED()      is_tape_feat( TAPERDC_FEATS2, TRDC_MEDSNS )


/*-------------------------------------------------------------------*/
/* Definitions for 3420/3480 sense bytes                             */
/*-------------------------------------------------------------------*/
#define SENSE1_TAPE_NOISE       0x80    /* Noise                     */
#define SENSE1_TAPE_TUA         0x40    /* TU Status A (ready)       */
#define SENSE1_TAPE_TUB         0x20    /* TU Status B (not ready)   */
#define SENSE1_TAPE_7TRK        0x10    /* 7-track feature           */
#define SENSE1_TAPE_RSE         0x10    /* Record sequence error     */
#define SENSE1_TAPE_LOADPT      0x08    /* Tape is at load point     */
#define SENSE1_TAPE_WRT         0x04    /* Tape is in write status   */
#define SENSE1_TAPE_FP          0x02    /* File protect status       */
#define SENSE1_TAPE_NCA         0x01    /* Not capable               */

#define SENSE4_TAPE_EOT         0x20    /* Tape indicate (EOT)       */

#define SENSE5_TAPE_SRDCHK      0x08    /* Start read check          */
#define SENSE5_TAPE_PARTREC     0x04    /* Partial record            */

#define SENSE7_TAPE_LOADFAIL    0x01    /* Load failure              */

/*-------------------------------------------------------------------*/
/* ISW : Internal error types used to build Device Dependent Sense   */
/*-------------------------------------------------------------------*/
#define TAPE_BSENSE_TAPEUNLOADED   0    /* I/O Attempted but no tape loaded */
#define TAPE_BSENSE_TAPELOADFAIL   1    /* I/O and load failed       */
#define TAPE_BSENSE_READFAIL       2    /* Error reading block       */
#define TAPE_BSENSE_WRITEFAIL      3    /* Error writing block       */
#define TAPE_BSENSE_BADCOMMAND     4    /* The CCW code is not known
                                           or sequence error         */
#define TAPE_BSENSE_INCOMPAT       5    /* The CCW code is known
                                           but is not unsupported    */
#define TAPE_BSENSE_WRITEPROTECT   6    /* Write CCW code was issued
                                           to a read-only media      */
#define TAPE_BSENSE_EMPTYTAPE      7    /* A read was issued but the
                                           tape is empty             */
#define TAPE_BSENSE_ENDOFTAPE      8    /* A read was issued past the
                                           end of the tape or a write
                                           was issued and there is no
                                           space left on the tape    */
#define TAPE_BSENSE_LOADPTERR      9    /* BSF/BSR/RdBW attempted
                                           from BOT                  */
#define TAPE_BSENSE_FENCED         10   /* Media damaged - unload
                                           or /reload required       */
#define TAPE_BSENSE_BADALGORITHM   11   /* Bad compression - HET
                                           tape compressed with an
                                           unsuported method         */
#define TAPE_BSENSE_RUN_SUCCESS    12   /* Rewind Unload success     */
#define TAPE_BSENSE_STATUSONLY     13   /* No exception occured      */
#define TAPE_BSENSE_LOCATEERR      14   /* Can't find block or TM    */
#define TAPE_BSENSE_READTM         15   /* A Tape Mark was read      */

#define TAPE_BSENSE_BLOCKSHORT     17   /* Short Tape block */
#define TAPE_BSENSE_ITFERROR       18   /* Interface error (SCSI
                                           driver unexpected err)    */
#define TAPE_BSENSE_REWINDFAILED   19   /* Rewind operation failed   */
#define TAPE_BSENSE_UNSOLICITED    20   /* Sense without UC          */

/*-------------------------------------------------------------------*/
/* Definitions for 3480 and later commands                           */
/*-------------------------------------------------------------------*/
/* Format control byte for Load Display command */
#define FCB_FS                  0xE0    /* Function Select bits...   */
#define FCB_FS_READYGO          0x00    /* Display msg until motion, */
                                        /* or until msg is updated   */
#define FCB_FS_UNMOUNT          0x20    /* Display msg until unloaded*/
#define FCB_FS_MOUNT            0x40    /* Display msg until loaded  */
#define FCB_FS_RESET_DISPLAY    0x80    /* Reset display (clear Host */
                                        /* msg; replace w/Unit msg)  */
#define FCB_FS_NOP              0x60    /* No-op                     */
#define FCB_FS_UMOUNTMOUNT      0xE0    /* Display msg 1 until tape  */
                                        /* is unloaded, then msg 2   */
                                        /* until tape is loaded      */
#define FCB_AM                  0x10    /* Alternate between msg 1/2 */
#define FCB_BM                  0x08    /* Blink message             */
#define FCB_M2                  0x04    /* Display only message 2    */
#define FCB_RESV                0x02    /* (reserved)                */
#define FCB_AL                  0x01    /* Activate AutoLoader on    */
                                        /* mount/unmount messages    */
/* Mode Set commands */
#define MSET_WRITE_IMMED        0x20    /* Tape-Write-Immediate mode */
#define MSET_SUPVR_INHIBIT      0x10    /* Supervisor Inhibit mode   */
#define MSET_IDRC               0x08    /* IDRC mode                 */

/* Path state byte for Sense Path Group ID command */
#define SPG_PATHSTAT            0xC0    /* Pathing status bits...    */
#define SPG_PATHSTAT_RESET      0x00    /* ...reset                  */
#define SPG_PATHSTAT_RESV       0x40    /* ...reserved bit setting   */
#define SPG_PATHSTAT_UNGROUPED  0x80    /* ...ungrouped              */
#define SPG_PATHSTAT_GROUPED    0xC0    /* ...grouped                */
#define SPG_PARTSTAT            0x30    /* Partitioning status bits..*/
#define SPG_PARTSTAT_IENABLED   0x00    /* ...implicitly enabled     */
#define SPG_PARTSTAT_RESV       0x10    /* ...reserved bit setting   */
#define SPG_PARTSTAT_DISABLED   0x20    /* ...disabled               */
#define SPG_PARTSTAT_XENABLED   0x30    /* ...explicitly enabled     */
#define SPG_PATHMODE            0x08    /* Path mode bit...          */
#define SPG_PATHMODE_SINGLE     0x00    /* ...single path mode       */
#define SPG_PATHMODE_RESV       0x08    /* ...reserved bit setting   */
#define SPG_RESERVED            0x07    /* Reserved bits, must be 0  */

/* Function control byte for Set Path Group ID command */
#define SPG_SET_MULTIPATH       0x80    /* Set multipath mode        */
#define SPG_SET_COMMAND         0x60    /* Set path command bits...  */
#define SPG_SET_ESTABLISH       0x00    /* ...establish group        */
#define SPG_SET_DISBAND         0x20    /* ...disband group          */
#define SPG_SET_RESIGN          0x40    /* ...resign from group      */
#define SPG_SET_COMMAND_RESV    0x60    /* ...reserved bit setting   */
#define SPG_SET_RESV            0x1F    /* Reserved bits, must be 0  */

/* Perform Subsystem Function order byte for PSF command             */
#define PSF_ORDER_PRSD          0x18    /* Prep for Read Subsys Data */
#define PSF_ACTION_SSD_ATNMSG   0x03    /* ..Attention Message       */
#define PSF_ORDER_SSIC          0x1B    /* Set Special Intercept Cond*/
#define PSF_ORDER_MNS           0x1C    /* Message Not Supported     */
#define PSF_ORDER_AFEL          0x80    /* Activate Forced Error Log.*/
#define PSF_ORDER_DFEL          0x81    /* Deact. Forced Error Log.  */
#define PSF_ACTION_FEL_IMPLICIT 0x01    /* ..Implicit (De)Activate   */
#define PSF_ACTION_FEL_EXPLICIT 0x02    /* ..Explicit (De)Activate   */
#define PSF_ORDER_AAC           0x82    /* Activate Access Control   */
#define PSF_ORDER_DAC           0x83    /* Deact. Access Control     */
#define PSF_ACTION_AC_LWP       0x80    /* ..Logical Write Protect   */
#define PSF_ACTION_AC_DCD       0x10    /* ..Data Compaction Default */
#define PSF_ACTION_AC_DCR       0x02    /* ..Data Check Recovery     */
#define PSF_ACTION_AC_ER        0x01    /* ..Extended Recovery       */
#define PSF_ORDER_RVF           0x90    /* Reset Volume Fenced       */
#define PSF_ORDER_PIN_DEV       0xA1    /* Pin Device                */
#define PSF_ACTION_PIN_CU0      0x00    /* ..Control unit 0          */
#define PSF_ACTION_PIN_CU1      0x01    /* ..Control unit 1          */
#define PSF_ORDER_UNPIN_DEV     0xA2    /* Unpin Device              */
#define PSF_FLAG_ZERO           0x00    /* Must be zero for all ord. */

/* Control Access Function Control                                   */
#define CAC_FUNCTION            0xC0    /* Function control bits     */
#define CAC_SET_PASSWORD        0x00    /* ..Set Password            */
#define CAC_COND_ENABLE         0x80    /* ..Conditional Enable      */
#define CAC_COND_DISABLE        0x40    /* ..Conditional Disable     */

/*-------------------------------------------------------------------*/
/* Definitions for tape device type field in device block            */
/*-------------------------------------------------------------------*/
#define TAPEDEVT_UNKNOWN        0       /* AWSTAPE format disk file  */
#define TAPEDEVT_AWSTAPE        1       /* AWSTAPE format disk file  */
#define TAPEDEVT_OMATAPE        2       /* OMATAPE format disk files */
#define TAPEDEVT_SCSITAPE       3       /* Physical SCSI tape        */
#define TAPEDEVT_HETTAPE        4       /* HET format disk file      */
#define TAPEDEVT_FAKETAPE       5       /* Flex FakeTape disk format */
#define TAPEDEVT_DWTVF          6       /* DWTVF disk format         */

#define TTYPSTR(i) (i==1?"AWS":i==2?"OMA":i==3?"SCSI":i==4?"HET":i==5?"FAKE":i==6?"DWTVF":"UNKNOWN")

/*-------------------------------------------------------------------*/
/* Fish - macros for checking SCSI tape device-independent status    */
/*-------------------------------------------------------------------*/
#if defined(OPTION_SCSI_TAPE)
#define STS_TAPEMARK(dev)       GMT_SM      ( (dev)->sstat )
#define STS_EOF(dev)            GMT_EOF     ( (dev)->sstat )
#define STS_BOT(dev)            GMT_BOT     ( (dev)->sstat )
#define STS_EOT(dev)            GMT_EOT     ( (dev)->sstat )
#define STS_EOD(dev)            GMT_EOD     ( (dev)->sstat )
#define STS_WR_PROT(dev)        GMT_WR_PROT ( (dev)->sstat )
#define STS_ONLINE(dev)         GMT_ONLINE  ( (dev)->sstat )
#define STS_MOUNTED(dev)        int_scsi_status_mounted((dev),0)
#define STS_NOT_MOUNTED(dev)    (!STS_MOUNTED(dev))
#endif

#define  AUTOLOAD_WAIT_FOR_TAPEMOUNT_INTERVAL_SECS  (5) /* (default) */

/*-------------------------------------------------------------------*/
/* Structure definition for HET/AWS/OMA tape block headers           */
/*-------------------------------------------------------------------*/
/*
 * The integer fields in the HET, AWSTAPE and OMATAPE headers are
 * encoded in the Intel format (i.e. the bytes of the integer are held
 * in reverse order).  For this reason the integers are defined as byte
 * arrays, and the bytes are fetched individually in order to make
 * the code portable across architectures which use either the Intel
 * format or the S/370 format.
 *
 * Block length fields contain the length of the emulated tape block
 * and do not include the length of the header.
 *
 * For the AWSTAPE and HET formats:
 * - the first block has a previous block length of zero
 * - a tapemark is indicated by a header with a block length of zero
 *   and a flag byte of X'40'
 *
 * For the OMATAPE format:
 * - the first block has a previous header offset of X'FFFFFFFF'
 * - a tapemark is indicated by a header with a block length of
 *   X'FFFFFFFF'
 * - each block is followed by padding bytes if necessary to ensure
 *   that the next header starts on a 16-byte boundary
 *
 */
typedef struct _AWSTAPE_BLKHDR
{  /*
    * PROGRAMMING NOTE: note that for AWS tape files, the "current
    * chunk size" comes FIRST and the "previous chunk size" comes
    * second. This is the complete opposite from the way it is for
    * Flex FakeTape. Also note that for AWS the size fields are in
    * LITTLE endian binary whereas for Flex FakeTape they're a BIG
    * endian ASCII hex-string.
    */
    HWORD   curblkl;                    /* Length of this block      */
    HWORD   prvblkl;                    /* Length of previous block  */
    BYTE    flags1;                     /* Flags byte 1 (see below)  */
    BYTE    flags2;                     /* Flags byte 2              */

    /* Definitions for AWSTAPE_BLKHDR flags byte 1 */
#define  AWSTAPE_FLAG1_NEWREC     0x80  /* Start of new record       */
#define  AWSTAPE_FLAG1_TAPEMARK   0x40  /* Tape mark                 */
#define  AWSTAPE_FLAG1_ENDREC     0x20  /* End of record             */
}
AWSTAPE_BLKHDR;

/*-------------------------------------------------------------------*/
/* Structure definition for OMA block header                         */
/*-------------------------------------------------------------------*/
typedef struct _OMATAPE_BLKHDR
{
    FWORD   curblkl;                    /* Length of this block      */
    FWORD   prvhdro;                    /* Offset of previous block
                                           header from start of file */
    FWORD   omaid;                      /* OMA identifier (contains
                                           ASCII characters "@HDF")  */
    FWORD   resv;                       /* Reserved                  */
}
OMATAPE_BLKHDR;

/*-------------------------------------------------------------------*/
/* Structure definition for OMA tape descriptor array                */
/*-------------------------------------------------------------------*/
typedef struct _OMATAPE_DESC
{
    char    filename[MAX_PATH+1];       /* Filename of data file     */
    char    format;                     /* H=HEADERS,T=TEXT,F=FIXED,X=Tape Mark */
    U16     blklen;                     /* Fixed block length        */
}
OMATAPE_DESC;

/*-------------------------------------------------------------------*/
/* Tape Auto-Loader table entry                                      */
/*-------------------------------------------------------------------*/
struct TAPEAUTOLOADENTRY
{
    char  *filename;
    int    argc;                /* max entries == AUTOLOADER_MAX     */
    char **argv;                /* max entries == AUTOLOADER_MAX     */
};

/*-------------------------------------------------------------------*/
/* Tape AUTOMOUNT CCWS directory control                             */
/*-------------------------------------------------------------------*/
struct TAMDIR
{
    TAMDIR    *next;            /* ptr to next entry or NULL         */
    char      *dir;             /* resolved directory value          */
    int        len;             /* strlen(dir)                       */
    int        rej;             /* 1 == reject, 0 == accept          */
};

/*-------------------------------------------------------------------*/
/* Generic media-handler-call parameters block                       */
/*-------------------------------------------------------------------*/
typedef struct _GENTMH_PARMS
{
    int      action;        // action code  (i.e. "what to do")
    DEVBLK*  dev;           // -> device block
    BYTE*    unitstat;      // -> unit status
    BYTE     code;          // CCW opcode
    // TODO: define whatever additional arguments may be needed...
}
GENTMH_PARMS;

/*-------------------------------------------------------------------*/
/* Generic media-handler-call action codes                           */
/*-------------------------------------------------------------------*/
#define  GENTMH_SCSI_ACTION_UPDATE_STATUS       (0)
//efine  GENTMH_AWS_ACTION_xxxxx...             (x)
//efine  GENTMH_HET_ACTION_xxxxx...             (x)
//efine  GENTMH_OMA_ACTION_xxxxx...             (x)

/*-------------------------------------------------------------------*/
/* Tape media I/O function vector table layout                       */
/*-------------------------------------------------------------------*/
struct TAPEMEDIA_HANDLER
{
    int  (*generic)    (GENTMH_PARMS*);                       // (generic call)
    int  (*open)       (DEVBLK*,                              BYTE *unitstat, BYTE code);
    void (*close)      (DEVBLK*);
    int  (*read)       (DEVBLK*,       BYTE *buf,             BYTE *unitstat, BYTE code);
    int  (*write)      (DEVBLK*, const BYTE *buf, U32 blklen, BYTE *unitstat, BYTE code);
    int  (*rewind)     (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*bsb)        (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*fsb)        (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*bsf)        (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*fsf)        (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*wtm)        (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*sync)       (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*dse)        (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*erg)        (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*tapeloaded) (DEVBLK*,                              BYTE *unitstat, BYTE code);
    int  (*passedeot)  (DEVBLK*);

    /* readblkid o/p values are returned in BIG-ENDIAN guest format  */
    int  (*readblkid)  (DEVBLK*, BYTE* logical, BYTE* physical);

    /* locateblk i/p value is passed in little-endian host format  */
    int  (*locateblk)  (DEVBLK*, U32 blockid,           BYTE *unitstat, BYTE code);
};

/*-------------------------------------------------------------------*/
/* Functions defined in TAPEDEV.C                                    */
/*-------------------------------------------------------------------*/
#ifdef _TAPEDEV_C_
static int   tapedev_init_handler   (DEVBLK *dev, int argc, char *argv[]);
static int   tapedev_close_device   (DEVBLK *dev );
static void  tapedev_query_device   (DEVBLK *dev, char **devclass, int buflen, char *buffer);
#endif

extern void  autoload_init          (DEVBLK *dev, int ac,   char **av);
extern int   autoload_mount_first   (DEVBLK *dev);
extern int   autoload_mount_next    (DEVBLK *dev);
extern void  autoload_close         (DEVBLK *dev);
extern void  autoload_global_parms  (DEVBLK *dev, int argc, char *argv[]);
extern void  autoload_clean_entry   (DEVBLK *dev, int ix);
extern void  autoload_tape_entry    (DEVBLK *dev, int argc, char *argv[]);
extern int   autoload_mount_tape    (DEVBLK *dev, int alix);

extern void* autoload_wait_for_tapemount_thread (void *db);

extern int   gettapetype            (DEVBLK *dev, char **short_descr);
extern int   gettapetype_byname     (DEVBLK *dev);
extern int   gettapetype_bydata     (DEVBLK *dev);

extern int   mountnewtape           (DEVBLK *dev, int argc, char **argv);
extern void  GetDisplayMsg          (DEVBLK *dev, char *msgbfr, size_t  lenbfr);
extern int   IsAtLoadPoint          (DEVBLK *dev);
extern int   ReqAutoMount           (DEVBLK *dev);
extern void  UpdateDisplay          (DEVBLK *dev);
extern int   return_false1          (DEVBLK *dev);
extern int   write_READONLY5        (DEVBLK *dev, const BYTE *bfr, U32 blklen, BYTE *unitstat, BYTE code);
extern int   is_tapeloaded_filename (DEVBLK *dev,                              BYTE *unitstat, BYTE code);
extern int   write_READONLY         (DEVBLK *dev,                              BYTE *unitstat, BYTE code);
extern int   no_operation           (DEVBLK *dev,                              BYTE *unitstat, BYTE code);
extern int   readblkid_virtual      (DEVBLK*, BYTE* logical, BYTE* physical);
extern int   locateblk_virtual      (DEVBLK*, U32 blockid,                     BYTE *unitstat, BYTE code);
extern int   generic_tmhcall        (GENTMH_PARMS*);

/*-------------------------------------------------------------------*/
/* Functions (and data areas) defined in TAPECCWS.C                  */
/*-------------------------------------------------------------------*/
typedef void TapeSenseFunc( int, DEVBLK*, BYTE*, BYTE );    // (sense handling function)

#define  TAPEDEVTYPELIST_ENTRYSIZE  (5)    // #of int's per 'TapeDevtypeList' table entry

extern int             TapeDevtypeList[];
extern BYTE*           TapeCommandTable[];
extern TapeSenseFunc*  TapeSenseTable[];
//tern BYTE            TapeCommandsXXXX[256]...
extern BYTE            TapeImmedOther[];
extern BYTE            TapeImmed3590[];

extern int   TapeCommandIsValid     (BYTE code, U16 devtype, BYTE *rustat);
extern void  tapedev_execute_ccw    (DEVBLK *dev, BYTE code, BYTE flags,
                                     BYTE chained, U32 count, BYTE prevcode, int ccwseq,
                                     BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual);
extern int   load_display           (DEVBLK *dev, BYTE *buf, U16 count);

extern void  build_senseX           (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);
extern void  build_sense_3410       (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);
extern void  build_sense_3420       (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);
extern void  build_sense_3410_3420  (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);
extern void  build_sense_3480_etal  (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);
extern void  build_sense_3490       (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);
extern void  build_sense_3590       (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);
extern void  build_sense_Streaming  (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode);

/*-------------------------------------------------------------------*/
/* Calculate I/O Residual                                            */
/*-------------------------------------------------------------------*/
#define RESIDUAL_CALC(_data_len)         \
    len = (_data_len);                   \
    num = (count < len) ? count : len;   \
    *residual = count - num;             \
    if (count < len) *more = 1

/*-------------------------------------------------------------------*/
/* Assign a unique Message Id for this asynchronous I/O if needed    */
/*-------------------------------------------------------------------*/
#if defined(OPTION_SCSI_TAPE)
  #define INCREMENT_MESSAGEID(_dev)   \
    if ((_dev)->SIC_active)           \
        (_dev)->msgid++
#else
  #define INCREMENT_MESSAGEID(_dev)
#endif // defined(OPTION_SCSI_TAPE)

/*-------------------------------------------------------------------*/
/* Functions defined in AWSTAPE.C                                    */
/*-------------------------------------------------------------------*/
AWSTAPE_DLL_IMPORT int  open_awstape      (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT void close_awstape     (DEVBLK *dev);
AWSTAPE_DLL_IMPORT int  passedeot_awstape (DEVBLK *dev);
AWSTAPE_DLL_IMPORT int  rewind_awstape    (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  write_awsmark     (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  sync_awstape      (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  fsb_awstape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  bsb_awstape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  fsf_awstape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  bsf_awstape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  readhdr_awstape   (DEVBLK *dev, off_t blkpos, AWSTAPE_BLKHDR *buf,
                                           BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  read_awstape      (DEVBLK *dev, BYTE *buf,
                                           BYTE *unitstat, BYTE code);
AWSTAPE_DLL_IMPORT int  write_awstape     (DEVBLK *dev, const BYTE *buf, U32 blklen,
                                           BYTE *unitstat, BYTE code);

/*-------------------------------------------------------------------*/
/* Functions defined in FAKETAPE.C                                   */
/*-------------------------------------------------------------------*/
extern int  open_faketape      (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern void close_faketape     (DEVBLK *dev);
extern int  passedeot_faketape (DEVBLK *dev);
extern int  rewind_faketape    (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  write_fakemark     (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  sync_faketape      (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  fsb_faketape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  bsb_faketape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  fsf_faketape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  bsf_faketape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  readhdr_faketape   (DEVBLK *dev, off_t blkpos,
                                             U16* pprvblkl, U16* pcurblkl,
                                             BYTE *unitstat, BYTE code);
extern int  writehdr_faketape  (DEVBLK *dev, off_t blkpos,
                                             U16   prvblkl, U16   curblkl,
                                             BYTE *unitstat, BYTE code);
extern int  read_faketape      (DEVBLK *dev, BYTE *buf,
                                             BYTE *unitstat, BYTE code);
extern int  write_faketape     (DEVBLK *dev, const BYTE *buf, U32 blklen,
                                             BYTE *unitstat, BYTE code);

/*-------------------------------------------------------------------*/
/* Functions defined in HETTAPE.C                                    */
/*-------------------------------------------------------------------*/
HETTAPE_DLL_IMPORT int  open_het      (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT void close_het     (DEVBLK *dev);
HETTAPE_DLL_IMPORT int  passedeot_het (DEVBLK *dev);
HETTAPE_DLL_IMPORT int  rewind_het    (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  write_hetmark (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  sync_het      (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  fsb_het       (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  bsb_het       (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  fsf_het       (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  bsf_het       (DEVBLK *dev, BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  read_het      (DEVBLK *dev, BYTE *buf,
                                       BYTE *unitstat, BYTE code);
HETTAPE_DLL_IMPORT int  write_het     (DEVBLK *dev, const BYTE *buf, U32 blklen,
                                       BYTE *unitstat, BYTE code);

/*-------------------------------------------------------------------*/
/* Functions defined in OMATAPE.C                                    */
/*-------------------------------------------------------------------*/
extern int  open_omatape       (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern void close_omatape      (DEVBLK *dev);
extern void close_omatape2     (DEVBLK *dev);
extern int  rewind_omatape     (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  fsb_omatape        (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  bsb_omatape        (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  fsf_omatape        (DEVBLK *dev, BYTE *unitstat, BYTE code);
extern int  bsf_omatape        (DEVBLK *dev, BYTE *unitstat, BYTE code);

extern int  read_omadesc       (DEVBLK *dev);
extern int  fsb_omaheaders     (DEVBLK *dev, OMATAPE_DESC *omadesc,            BYTE *unitstat, BYTE code);
extern int  fsb_omafixed       (DEVBLK *dev, OMATAPE_DESC *omadesc,            BYTE *unitstat, BYTE code);
extern int  read_omaheaders    (DEVBLK *dev, OMATAPE_DESC *omadesc, BYTE *buf, BYTE *unitstat, BYTE code);
extern int  read_omafixed      (DEVBLK *dev, OMATAPE_DESC *omadesc, BYTE *buf, BYTE *unitstat, BYTE code);
extern int  read_omatext       (DEVBLK *dev, OMATAPE_DESC *omadesc, BYTE *buf, BYTE *unitstat, BYTE code);
extern int  read_omatape       (DEVBLK *dev,                        BYTE *buf, BYTE *unitstat, BYTE code);
extern int  readhdr_omaheaders (DEVBLK *dev, OMATAPE_DESC *omadesc,
                                             long blkpos, S32 *pcurblkl,
                                             S32 *pprvhdro, S32 *pnxthdro,     BYTE *unitstat, BYTE code);

/*-------------------------------------------------------------------*/
/*                Tape ERA, HRA and SENSE constants                  */
/*-------------------------------------------------------------------*/
/* For 3480/3490 tape drives HRA was an assumed function of the OS.  */
/* For 3590 (NTP) tape drives HRA is no longer assumed. The labels   */
/* here are the 3480/3590 labels but the values are NTP values.      */
/* Refer to sense byte 2 for additional information.                 */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Host Recovery Action (HRA)    (these are the 3590 codes           */
/*-------------------------------------------------------------------*/

#define TAPE_HRA_PERMANENT_ERROR             0x00
#define TAPE_HRA_RETRY                       0x80
#define TAPE_HRA_DDR                         0x00       // Same as error for VT
#define TAPE_HRA_RESUME                      0x40
#define TAPE_HRA_OPERATOR_INTERVENTION       0xC0

// Sense byte 0

#define  TAPE_SNS0_CMDREJ     0x80          // Command Reject
#define  TAPE_SNS0_INTVREQ    0x40          // Intervention Required
#define  TAPE_SNS0_BUSCHK     0x20          // Bus-out Check
#define  TAPE_SNS0_EQUIPCHK   0x10          // Equipment Check
#define  TAPE_SNS0_DATACHK    0x08          // Data check
#define  TAPE_SNS0_OVERRUN    0x04          // Overrun
#define  TAPE_SNS0_DEFUNITCK  0x02          // Deferred Unit Check
#define  TAPE_SNS0_ASSIGNED   0x01          // Assigned Elsewhere

// Sense byte 1

#define  TAPE_SNS1_LOCFAIL    0x80          // Locate Failure
#define  TAPE_SNS1_ONLINE     0x40          // Drive Online to CU
#define  TAPE_SNS1_RSRVD      0x20          // Reserved
#define  TAPE_SNS1_RCDSEQ     0x10          // Record Sequence Error
#define  TAPE_SNS1_BOT        0x08          // Beginning of Tape
#define  TAPE_SNS1_WRTMODE    0x04          // Write Mode
#define  TAPE_SNS1_FILEPROT   0x02          // Write Protect
#define  TAPE_SNS1_NOTCAPBL   0x01          // Not Capable

//  Sense byte 2 (NTP)
//  Log code is in byte 2 (3-4), BRAC is in byte 2 (0-1)

#define  TAPE_SNS2_NTP_BRAC_00_PERM_ERR     0x00      // BRAC 00 - PERM ERR
#define  TAPE_SNS2_NTP_BRAC_01_CONTINUE     0x40      // BRAC 01 - Continue ( RESUME )
#define  TAPE_SNS2_NTP_BRAC_10_REISSUE      0x80      // BRAC 10 - Reissue  ( RETRY  )
#define  TAPE_SNS2_NTP_BRAC_11_DEFER_REISS  0xC0      // BRAC 11 - Deferred Reissue ( I/R ? )
#define  TAPE_SNS2_NTP_LOG_CD0_NO_LOG       0x00
#define  TAPE_SNS2_NTP_LOG_CD1_TEMP_OBR     0x08
#define  TAPE_SNS2_NTP_LOG_CD2_PERM_OBR     0x10
#define  TAPE_SNS2_NTP_LOG_CD3_A3           0x18

#define  TAPE_SNS2_REPORTING_CHAN_PATH      0xF0      // Interface in the first 4 bits
#define  TAPE_SNS2_REPORTING_CHAN_A         0x20      // Channel A Interface
#define  TAPE_SNS2_REPORTING_CHAN_B         0x40      // Channel B Interface
#define  TAPE_SNS2_REPORTING_CU             0x00      // Always 0 (ZERO) Bit 4
#define  TAPE_SNS2_ACL_ACTIVE               0x04      // AutoLoader in SYS MODE and has Cart
#define  TAPE_SNS2_SYNCMODE                 0x02      // Tape Synchronous Mode
#define  TAPE_SNS2_POSITION                 0x01      // Tape Positioning

/*-------------------------------------------------------------------*/
/* Sense byte 3:  Error Recovery Action (ERA)                        */
/*-------------------------------------------------------------------*/

#define  TAPE_ERA_00_UNSOLICITED_SENSE           0x00

#define  TAPE_ERA_21_DATA_STREAMING_NOT_OPER     0x21
#define  TAPE_ERA_22_PATH_EQUIPMENT_CHECK        0x22
#define  TAPE_ERA_23_READ_DATA_CHECK             0x23
#define  TAPE_ERA_24_LOAD_DISPLAY_CHECK          0x24
#define  TAPE_ERA_25_WRITE_DATA_CHECK            0x25
#define  TAPE_ERA_26_READ_OPPOSITE               0x26
#define  TAPE_ERA_27_COMMAND_REJECT              0x27
#define  TAPE_ERA_28_WRITE_ID_MARK_CHECK         0x28
#define  TAPE_ERA_29_FUNCTION_INCOMPATIBLE       0x29
#define  TAPE_ERA_2A_UNSOL_ENVIRONMENTAL_DATA    0x2A
#define  TAPE_ERA_2B_ENVIRONMENTAL_DATA_PRESENT  0x2B
#define  TAPE_ERA_2C_PERMANENT_EQUIPMENT_CHECK   0x2C
#define  TAPE_ERA_2D_DATA_SECURE_ERASE_FAILURE   0x2D
#define  TAPE_ERA_2E_NOT_CAPABLE_BOT_ERROR       0x2E

#define  TAPE_ERA_30_WRITE_PROTECTED             0x30
#define  TAPE_ERA_31_TAPE_VOID                   0x31
#define  TAPE_ERA_32_TENSION_LOST                0x32
#define  TAPE_ERA_33_LOAD_FAILURE                0x33
#define  TAPE_ERA_34_UNLOAD_FAILURE              0x34
#define  TAPE_ERA_34_MANUAL_UNLOAD               0x34
#define  TAPE_ERA_35_DRIVE_EQUIPMENT_CHECK       0x35
#define  TAPE_ERA_36_END_OF_DATA                 0x36
#define  TAPE_ERA_37_TAPE_LENGTH_ERROR           0x37
#define  TAPE_ERA_38_PHYSICAL_END_OF_TAPE        0x38
#define  TAPE_ERA_39_BACKWARD_AT_BOT             0x39
#define  TAPE_ERA_3A_DRIVE_SWITCHED_NOT_READY    0x3A
#define  TAPE_ERA_3A_DRIVE_RESET_BY_OPERATOR     0x3A
#define  TAPE_ERA_3B_MANUAL_REWIND_OR_UNLOAD     0x3B
#define  TAPE_ERA_3B_VOLUME_REMOVE_BY_OPERATOR   0x3B
#define  TAPE_ERA_3C_VOLUME_MANUALLY_UNLOADED    0x3C

#define  TAPE_ERA_40_OVERRUN                     0x40
#define  TAPE_ERA_40_DEVICE_DEFERRED_ACCESS      0x40
#define  TAPE_ERA_41_RECORD_SEQUENCE_ERROR       0x41
#define  TAPE_ERA_41_BLOCK_ID_SEQUENCE_ERROR     0x41
#define  TAPE_ERA_42_DEGRADED_MODE               0x42
#define  TAPE_ERA_43_DRIVE_NOT_READY             0x43
#define  TAPE_ERA_43_INTERVENTION_REQ            0x43
#define  TAPE_ERA_44_LOCATE_BLOCK_FAILED         0x44
#define  TAPE_ERA_45_DRIVE_ASSIGNED_ELSEWHERE    0x45
#define  TAPE_ERA_46_DRIVE_NOT_ONLINE            0x46
#define  TAPE_ERA_47_VOLUME_FENCED               0x47
#define  TAPE_ERA_48_UNSOL_INFORMATIONAL_DATA    0x48
#define  TAPE_ERA_48_CONTROLLING_COMP_RETRY_REQ  0x48
#define  TAPE_ERA_49_BUS_OUT_CHECK               0x49
#define  TAPE_ERA_49_BUS_OUT_PARITY              0x49
#define  TAPE_ERA_4A_CU_ERP_FAILURE              0x4A
#define  TAPE_ERA_4B_CU_AND_DRIVE_INCOMPATIBLE   0x4B
#define  TAPE_ERA_4C_RECOVERED_CHECKONE_FAILURE  0x4C
#define  TAPE_ERA_4D_RESETTING_EVENT             0x4D
#define  TAPE_ERA_4E_MAX_BLOCKSIZE_EXCEEDED      0x4E
#define  TAPE_ERA_4F_DEVICE_CONTROLLER_INCOMP    0x4F

#define  TAPE_ERA_50_READ_BUFFERED_LOG           0x50
#define  TAPE_ERA_50_BUFFERED_LOG_OVERFLOW       0x50
#define  TAPE_ERA_51_BUFFERED_LOG_END_OF_VOLUME  0x51
#define  TAPE_ERA_51_END_OF_VOLUME_PROCESSING    0x51
#define  TAPE_ERA_52_END_OF_VOLUME_COMPLETE      0x52
#define  TAPE_ERA_53_GLOBAL_COMMAND_INTERCEPT    0x53
#define  TAPE_ERA_54_TEMP_CHNL_INTERFACE_ERROR   0x54
#define  TAPE_ERA_55_PERM_CHNL_INTERFACE_ERROR   0x55
#define  TAPE_ERA_56_CHNL_PROTOCOL_ERROR         0x56
#define  TAPE_ERA_57_GLOBAL_STATUS_INTERCEPT     0x57
#define  TAPE_ERA_57_ATTENTION_INTERCEPT         0x57
#define  TAPE_ERA_5A_TAPE_LENGTH_INCOMPAT        0x5A
#define  TAPE_ERA_5B_FORMAT_3480_XF_INCOMPAT     0x5B
#define  TAPE_ERA_5C_FORMAT_3480_2_XF_INCOMPAT   0x5C
#define  TAPE_ERA_5D_TAPE_LENGTH_VIOLATION       0x5D
#define  TAPE_ERA_5E_COMPACT_ALGORITHM_INCOMPAT  0x5E

//  3490/3590/NTP IN AN AUTOMATED LIBRARY SYSTEM

#define  TAPE_ERA_60_LIB_ATT_FAC_EQ_CHK          0x60
#define  TAPE_ERA_62_LIB_MGR_OFFLINE_TO_SUBSYS   0x62
#define  TAPE_ERA_63_LIB_MGR_CU_INCOMPAT         0x63
#define  TAPE_ERA_64_LIB_VOLSER_IN_USE           0x64
#define  TAPE_ERA_65_LIB_VOLUME_RESERVED         0x65
#define  TAPE_ERA_66_LIB_VOLSER_NOT_IN_LIB       0x66
#define  TAPE_ERA_67_LIB_CATEGORY_EMPTY          0x67
#define  TAPE_ERA_68_LIB_ORDER_SEQ_CHK           0x68
#define  TAPE_ERA_69_LIB_OUTPUT_STATIONS_FULL    0x69
#define  TAPE_ERA_6B_LIB_VOLUME_MISPLACED        0x6B
#define  TAPE_ERA_6C_LIB_MISPLACED_VOLUME_FOUND  0x6C
#define  TAPE_ERA_6D_LIB_DRIVE_NOT_UNLOADED      0x6D
#define  TAPE_ERA_6E_LIB_INACCESS_VOLUME_REST    0x6E
#define  TAPE_ERA_6F_LIB_OPTICS_FAILURE          0x6F
#define  TAPE_ERA_70_LIB_MGR_EQ_CHK              0x70
#define  TAPE_ERA_71_LIB_EQ_CHK                  0x71
#define  TAPE_ERA_72_LIB_NOT_CAP_MANUAL_MODE     0x72
#define  TAPE_ERA_73_LIB_INTERVENTION_REQ        0x73
#define  TAPE_ERA_74_LIB_INFORMATION_DATA        0x74
#define  TAPE_ERA_75_LIB_VOLSER_INACCESS         0x75
#define  TAPE_ERA_76_LIB_ALL_CELLS_FULL          0x76
#define  TAPE_ERA_77_LIB_DUP_VOLSER_EJECTED      0x77
#define  TAPE_ERA_78_LIB_DUP_VOLSER_LEFT_IN_STAT 0x78
#define  TAPE_ERA_79_LIB_UNREADABLE_INVLD_VOLSER 0x79
#define  TAPE_ERA_7A_LIB_READ_STATISTICS         0x7A
#define  TAPE_ERA_7B_LIB_VOLUME_MAN_EJECTED      0x7B
#define  TAPE_ERA_7C_LIB_OUT_OF_CLEANER_VOLUMES  0x7C
#define  TAPE_ERA_7D_LIB_VOLUME_EXPORTED         0x7D
#define  TAPE_ERA_7F_LIB_CATEGORY_IN_USE         0x7F

#define  TAPE_ERA_80_LIB_UNEXPECTED_VOLUME_EJECT 0x80
#define  TAPE_ERA_81_LIB_IO_STATION_DOOR_OPEN    0x81
#define  TAPE_ERA_82_LIB_MGR_PROG_EXCEPTION      0x82
#define  TAPE_ERA_83_LIB_DRIVE_EXCEPTION         0x83
#define  TAPE_ERA_84_LIB_DRIVE_FAILURE           0x84
#define  TAPE_ERA_85_LIB_SMOKE_DETECTION_ALERT   0x85
#define  TAPE_ERA_86_LIB_ALL_CATEGORYS_RESERVED  0x86
#define  TAPE_ERA_87_LIB_DUP_VOLSER_ADDITION     0x87
#define  TAPE_ERA_88_LIB_DAMAGE_CART_EJECTED     0x88

#define  TAPE_ERA_91_LIB_VOLUME_INACCESSIBLE     0x91

// Sense byte 3 for NTP (3590) TAPES

#define  TAPE_ERA_C0_RAC_USE_BRAC                0xC0
#define  TAPE_ERA_C1_RAC_FENCE_DEVICE            0xC1
#define  TAPE_ERA_C2_RAC_FENCH_DEVICE_PATH       0xC2
#define  TAPE_ERA_C6_RAC_LONG_BUSY               0xC6
#define  TAPE_ERA_D2_RAC_READ_ALT                0xD2

// Sense byte 4

#define  TAPE_SNS4_3420_TAPE_INDICATE         0x20      // EOT FOUND
#define  TAPE_SNS4_3480_FORMAT_MODE           0xC0
#define  TAPE_SNS4_3480_FORMAT_MODE_XF        0x80
#define  TAPE_SNS4_3490_FORMAT_MODE           0x00
#define  TAPE_SNS4_3490_FORMAT_MODE_RSVD      0x40
#define  TAPE_SNS4_3490_FORMAT_MODE_IDRC      0x80
#define  TAPE_SNS4_3490_FORMAT_MODE_SPECIAL   0xC0

#define  TAPE_SNS4_3480_HO_CHAN_LOG_BLK_ID    0x3F      // 22-bits for BLK ID

// Sense byte 5

#define  TAPE_SNS5_3480_MO_CHAN_LOG_BLK_ID    0xFF

// Sense bytes 4-5 for NTP: Byte 4 is Reason Code (RC)
//                          Byte 5 is Reason Qualifer Code (RQC)

#define  TAPE_SNS4_5_NTP_RC_11_UA_RQC_11_DEV_LOG      0x1110      // UNIT ATTENTION/Device Log

#define  TAPE_SNS4_5_NTP_RC_12_LA_RQC_11_DEV_CLEANED  0x1211      // LIBRARY ATTENTION/Device CLEANED
#define  TAPE_SNS4_5_NTP_RC_12_LA_RQC_12_DEV_QUIESCED 0x1212      // LIBRARY ATTENTION/Device QUIESCE
#define  TAPE_SNS4_5_NTP_RC_12_LA_RQC_13_DEV_RESUMED  0x1213      // LIBRARY ATTENTION/Device RESUMED

#define  TAPE_SNS4_5_NTP_RC_20_CR_RQC_00              0x2000      // COMMAND REJECT

#define  TAPE_SNS4_5_NTP_RC_22_PE_RQC_30_GBL_CMD      0x2230      // PROTECTION EXCEPTION/Global Command
#define  TAPE_SNS4_5_NTP_RC_22_PE_RQC_31_GBL_STATUS   0x2231      // PROTECTION EXCEPTION/Global Status

#define  TAPE_SNS4_5_NTP_RC_30_BE_RQC_12_EOV          0x3012      // BOUNDARY EXCEPTION/End of Volume

#define  TAPE_SNS4_5_NTP_RC_50_DC_RQC_50_NO_FMT_BOV   0x5050      // DATA CHECK/No Formatting at BOV
#define  TAPE_SNS4_5_NTP_RC_50_DC_RQC_50_NO_FMT       0x5051      // DATA CHECK/No Formatting Past BOV

#define  TAPE_SNS4_5_NTP_RC_40_OE_RQC_10_MED_NOT_LD   0x4010      // OPERATIONAL EXCEPTION/Medium Not Loaded
#define  TAPE_SNS4_5_NTP_RC_40_OE_RQC_11_DRV_NOT_RDY  0x4011      // OPERATIONAL EXCEPTION/Drive Not Ready
#define  TAPE_SNS4_5_NTP_RC_40_OE_RQC_12_DEV_LONG_BSY 0x4012      // OPERATIONAL EXCEPTION/Device long busy
#define  TAPE_SNS4_5_NTP_RC_40_OE_RQC_20_LDR_IR       0x4020      // OPERATIONAL EXCEPTION/Loader Interv Req'd

// Sense byte 6

#define  TAPE_SNS6_3480_LO_CHAN_LOG_BLK_ID    0xFF

// Sense byte 7

#define  TAPE_SNS7_TAPE_SECURITY_ERASE_CMD    0x08
#define  TAPE_SNS7_FMT_20_3480                0x20 // DRIVE AND CU ERROR INFORMATION
#define  TAPE_SNS7_FMT_21_3480_READ_BUF_LOG   0x21 // BUFFERED LOG DATA WHEN NO IDRC is installed
#define  TAPE_SNS7_FMT_30_3480_READ_BUF_LOG   0x30 // BUFFERED LOG DATA WHEN IDRC is installed
#define  TAPE_SNS7_FMT_22_3480_EOV_STATS      0x22
#define  TAPE_SNS7_FMT_23_ALT                 0x23
#define  TAPE_SNS7_FMT_50_NTP                 0x50
#define  TAPE_SNS7_FMT_51_NTP                 0x51
#define  TAPE_SNS7_FMT_70_3490                0x70
#define  TAPE_SNS7_FMT_71_3490                0x71

#endif // _TAPEDEV_H_
