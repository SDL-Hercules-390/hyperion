/* DASDBLKS.H   (C) Copyright Roger Bowler, 1999-2012                */
/*              DASD control block structures                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This header file contains definitions of OS Data Management       */
/* control block structures for use by the Hercules DASD utilities.  */
/* It also contains function prototypes for the DASD utilities.      */
/*-------------------------------------------------------------------*/

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* typedef forward references                                        */
/*-------------------------------------------------------------------*/
typedef  struct  VOL1_CKD       VOL1_CKD;       // VOL1 layout for CKD
typedef  struct  VOL1_FBA       VOL1_FBA;       // VOL1 layout for FBA

typedef  struct  FORMAT1_DSCB   FORMAT1_DSCB;   // DSCB1: Dataset descriptor
typedef  struct  FORMAT3_DSCB   FORMAT3_DSCB;   // DSCB3: Additional extents
typedef  struct  FORMAT4_DSCB   FORMAT4_DSCB;   // DSCB4: VTOC descriptor
typedef  struct  FORMAT5_DSCB   FORMAT5_DSCB;   // DSCB5: Free space map

typedef  struct  F5AVEXT        F5AVEXT;        // Available extent in DSCB5
typedef  struct  DSXTENT        DSXTENT;        // Dataset extent descriptor

typedef  struct  PDSDIR         PDSDIR;         // PDS directory entry
typedef  struct  CIFBLK         CIFBLK;         // CKD image file descriptor

typedef  struct  COPYR1         COPYR1;         // IEBCOPY header record 1
typedef  struct  COPYR2         COPYR2;         // IEBCOPY header record 2
typedef  struct  DATABLK        DATABLK;        // IEBCOPY unload data rec

#define  MAX_TRACKS   32767

/*-------------------------------------------------------------------*/
/*  VOL1 label and IPL text constants                                */
/*-------------------------------------------------------------------*/
static const BYTE VOL1_KEYA[4]  = {'V', 'O', 'L', '1' };   /* ASCII  "VOL1" */
static const BYTE VOL1_KEY[4]   = {0xE5,0xD6,0xD3,0xF1};   /* EBCDIC "VOL1" */
static const BYTE IPL1_KEY[4]   = {0xC9,0xD7,0xD3,0xF1};   /* EBCDIC "IPL1" */
static const BYTE IPL2_KEY[4]   = {0xC9,0xD7,0xD3,0xF2};   /* EBCDIC "IPL2" */

#define HERC_OWNERA "** HERCULES **"  /* (ASCII, 14-bytes, centered) */

#define VOL1_KEYLEN       4
#define IPL1_KEYLEN       4
#define IPL2_KEYLEN       4

#define VOLSER_LEN        6
#define VOL1_DATALEN     80
#define IPL1_DATALEN     24
#define IPL2_DATALEN    144

static const BYTE   iplpsw [8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const BYTE   iplccw1[8] = {0x06, 0x00, 0x3A, 0x98, 0x60, 0x00, 0x00, 0x60};
static const BYTE   iplccw2[8] = {0x08, 0x00, 0x3A, 0x98, 0x00, 0x00, 0x00, 0x00};

static const BYTE noiplpsw [8] = {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F};
static const BYTE noiplccw1[8] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static const BYTE noiplccw2[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const BYTE ipl2data[43] = {0x07, 0x00, 0x3A, 0xB8, 0x40, 0x00, 0x00, 0x06,
                                  0x31, 0x00, 0x3A, 0xBE, 0x40, 0x00, 0x00, 0x05,
                                  0x08, 0x00, 0x3A, 0xA0, 0x00, 0x00, 0x00, 0x00,
                                  0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x7f, 0xff,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BBCCHH
                                  0x00, 0x00, 0x00, 0x00, 0x04};      // CCHHR

/*-------------------------------------------------------------------*/
/* Layout of a standard VOL1 label for CKD dasd                      */
/*-------------------------------------------------------------------*/
struct VOL1_CKD
{
    /* VOL1 label format for CKD devices:

      Dec    Len     Type         Description
       0      4      EBCDIC       Always "VOL1"
       4      6      EBCDIC       Volume Serial number
      10      1      Hex          Security byte (X'C0')
      11      5      Hex          CCHHR of the VTOC
      16     21      Reserved     EBCDIC X'40' spaces
      37     14      EBCDIC       Owner code for LVTOC listing
      51     29      Reserved     EBCDIC X'40' spaces
    */
    BYTE   vol1[ IPL1_KEYLEN ];   // Always "VOL1"
    BYTE   volser[ VOLSER_LEN ];  // Volume Serial number
    BYTE   security;              // Security byte (X'C0')
    HWORD  vtoc_CC;               // CCHHR of the VTOC
    HWORD  vtoc_HH;               // CCHHR of the VTOC
    BYTE   vtoc_R;                // CCHHR of the VTOC
    BYTE   rsrvd3[21];            // EBCDIC X'40' spaces
    BYTE   owner[14];             // Owner code for LVTOC listing
    BYTE   rsrvd4[29];            // EBCDIC X'40' spaces
};

/*-------------------------------------------------------------------*/
/* Layout of a standard VOL1 label for FBA dasd                      */
/*-------------------------------------------------------------------*/
struct VOL1_FBA
{
    /* VOL1 label format for FBA devices:

      Dec    Len     Type         Description
       0      4      EBCDIC       Always "VOL1"
       4      6      EBCDIC       Volume Serial number
      10      1      Hex          Security byte (X'C0')
      11      5      Unsigned     Sector number of the VTOC
      16      5      Reserved     EBCDIC X'40' spaces
      21      4      Unsigned     VTOC Control Interval size
      25      4      Unsigned     VTOC sectors per Control Interval
      29      4      Unsigned     VTOC DSCB slots per Control Interval
      33      4      Reserved     EBCDIC X'40' spaces
      37     14      EBCDIC       Volume Owner and Address
      51     29      Reserved     EBCDIC X'40' spaces
    */
    BYTE   vol1[ IPL1_KEYLEN ];   // Always "VOL1"
    BYTE   volser[ VOLSER_LEN ];  // Volume Serial number
    BYTE   security;              // Security byte (X'C0')
    BYTE   rsrvd1;                // X'00'
    FWORD  vtoc_block;            // Sector number of the VTOC
    BYTE   rsrvd2[5];             // EBCDIC X'40' spaces
    FWORD  vtoc_cisz;             // VTOC Control Interval size (bytes)
    FWORD  vtoc_seci;             // VTOC sectors per Control Interval
    FWORD  vtoc_slci;             // VTOC DSCB slots per Control Interval
    BYTE   rsrvd3[4];             // EBCDIC X'40' spaces
    BYTE   owner[14];             // Owner code for LVTOC listing
    BYTE   rsrvd4[29];            // EBCDIC X'40' spaces
};

/*-------------------------------------------------------------------*/
/* Definition of DSCB records in VTOC                                */
/*-------------------------------------------------------------------*/
struct DSXTENT {                        /* Dataset extent descriptor */
        BYTE    xttype;                 /* Extent type               */
        BYTE    xtseqn;                 /* Extent sequence number    */
        HWORD   xtbcyl;                 /* Extent begin cylinder     */
        HWORD   xtbtrk;                 /* Extent begin track        */
        HWORD   xtecyl;                 /* Extent end cylinder       */
        HWORD   xtetrk;                 /* Extent end track          */
};

/* Bit definitions for extent type */
#define XTTYPE_UNUSED           0x00    /* Unused extent descriptor  */
#define XTTYPE_DATA             0x01    /* Data extent               */
#define XTTYPE_OVERFLOW         0x02    /* Overflow extent           */
#define XTTYPE_INDEX            0x04    /* Index extent              */
#define XTTYPE_USERLBL          0x40    /* User label extent         */
#define XTTYPE_SHARCYL          0x80    /* Shared cylinders          */
#define XTTYPE_CYLBOUND         0x81    /* Extent on cyl boundary    */

struct FORMAT1_DSCB                     /* DSCB1: Dataset descriptor */
{
/*00*/  BYTE    ds1dsnam[44];           /* Key (44 byte dataset name)*/
/*2C*/  BYTE    ds1fmtid;               /* Format identifier (0xF1)  */
/*2D*/  BYTE    ds1dssn[6];             /* Volume serial number      */
/*33*/  HWORD   ds1volsq;               /* Volume sequence number    */
/*35*/  BYTE    ds1credt[3];            /* Dataset creation date...
                                           ...byte 0: Binary year-1900
                                           ...bytes 1-2: Binary day  */
/*38*/  BYTE    ds1expdt[3];            /* Dataset expiry date       */
/*3B*/  BYTE    ds1noepv;               /* Number of extents         */
/*3C*/  BYTE    ds1bodbd;               /* #bytes used in last dirblk*/
/*3D*/  BYTE    ds1flag1;               /* Flags byte                */
#define DS1LARGE                0x08    /* Large format data set     */

/*3E*/  BYTE    ds1syscd[13];           /* System code (IBMOSVS2)    */
/*4B*/  BYTE    ds1refdt[3];            /* Last reference date ('YDD'
                                           or 0 if not maintained).
                                           Add 1900 to 'Y' for year. */
/*4E*/  BYTE    ds1smsfg;               /* SMS indicators            */
#define DS1STRP                 0x04    /* Extended-format data set  */

/*4F*/  BYTE    ds1scext[3];            /* Secondary space extension */
/*52*/  BYTE    ds1dsorg[2];            /* Dataset organization      */
/*54*/  BYTE    ds1recfm;               /* Record format             */
/*55*/  BYTE    ds1optcd;               /* Option codes              */
/*56*/  HWORD   ds1blkl;                /* Block length              */
/*58*/  HWORD   ds1lrecl;               /* Logical record length     */
/*5A*/  BYTE    ds1keyl;                /* Key length                */
/*5B*/  HWORD   ds1rkp;                 /* Relative key position     */
/*5D*/  BYTE    ds1dsind;               /* Dataset indicators        */
/*5E*/  FWORD   ds1scalo;               /* Secondary allocation...
                                           ...byte 0: Allocation units
                                           ...bytes 1-3: Quantity    */
/*62*/  BYTE    ds1lstar[3];            /* Last used TTR (see below) */

/*                     PROGRAMMING NOTE

    For NORMAL datasets, the last block pointer (TTR) is in the
    DS1LSTAR field. So the size of the dataset in number of tracks
    is in the first two bytes (TT) of DS1LSTAR. (The last right-
    most byte of DS1LSTAR being the 'R' part of the 'TTR'.)

    For DSNTYPE=LARGE however, the size of the dataset is 3 bytes
    in size (i.e. TTT, not just TT). So you use the first two
    high-order bytes of DS1LSTAR (just like you do for for normal
    format datasets), but in addition to that, the high-order byte
    of the 3-byte TTT is kept in the DS1TTTHI field.
   
    For DSNTYPE=EXTENDED, the size of the dataset in tracks is of
    course 4 bytes in size (TTTT), with the low-order 2 bytes of
    that 4-byte TTTT coming from the high-order two bytes of the
    DS1LSTAR field (just for for normal/large format datasets),
    but the two HIGH-order bytes of the 4-byte TTTT is in DS1TRBAL.

            SUMMARY OF DATASET SIZE IN NUMBER OF TRACKS:

    Normal:      TT   =                high-order 2 bytes of ds1lstar
    Large:      TTT   =   ds1ttthi(1), high-order 2 bytes of ds1lstar
    Extended:  TTTT   =   ds1trbal(2), high-order 2 bytes of ds1lstar

*/
/*65*/  HWORD   ds1trbal;               /* Normal/Large: bytes unused
                                           on last track.
                                           Extended: high-order bytes
                                           of TTTTTR.                */
/*67*/  BYTE    resv1;                  /* Reserved                  */
/*68*/  BYTE    ds1ttthi;               /* Large format: high-order
                                           byte of TTTR.             */
/*69*/  DSXTENT ds1ext1;                /* First extent descriptor   */
/*73*/  DSXTENT ds1ext2;                /* Second extent descriptor  */
/*7D*/  DSXTENT ds1ext3;                /* Third extent descriptor   */
/*87*/  BYTE    ds1ptrds[5];            /* CCHHR of F2 or F3 DSCB    */
/*8C*/                                  /* Total len/size: 140 bytes */
};

/* Bit definitions for ds1dsind */
#define DS1DSIND_LASTVOL        0x80    /* Last volume of dataset    */
#define DS1DSIND_RACFIND        0x40    /* RACF indicated            */
#define DS1DSIND_BLKSIZ8        0x20    /* Blocksize multiple of 8   */
#define DS1DSIND_PASSWD         0x10    /* Password protected        */
#define DS1DSIND_WRTPROT        0x04    /* Write protected           */
#define DS1DSIND_UPDATED        0x02    /* Updated since last backup */
#define DS1DSIND_SECCKPT        0x01    /* Secure checkpoint dataset */

/* Bit definitions for ds1optcd */
#define DS1OPTCD_ICFDSET        0x80    /* Dataset in ICF catalog    */
#define DS1OPTCD_ICFCTLG        0x40    /* ICF catalog               */

/* Bit definitions for ds1scalo byte 0 */
#define DS1SCALO_UNITS          0xC0    /* Allocation units...       */
#define DS1SCALO_UNITS_ABSTR    0x00    /* ...absolute tracks        */
#define DS1SCALO_UNITS_BLK      0x40    /* ...blocks                 */
#define DS1SCALO_UNITS_TRK      0x80    /* ...tracks                 */
#define DS1SCALO_UNITS_CYL      0xC0    /* ...cylinders              */
#define DS1SCALO_CONTIG         0x08    /* Contiguous space          */
#define DS1SCALO_MXIG           0x04    /* Maximum contiguous extent */
#define DS1SCALO_ALX            0x02    /* Up to 5 largest extents   */
#define DS1SCALO_ROUND          0x01    /* Round to cylinders        */

struct FORMAT3_DSCB {                   /* DSCB3: Additional extents */
        BYTE    ds3keyid[4];            /* Key (4 bytes of 0x03)     */
        DSXTENT ds3extnt[4];            /* Four extent descriptors   */
        BYTE    ds3fmtid;               /* Format identifier (0xF3)  */
        DSXTENT ds3adext[9];            /* Nine extent descriptors   */
        BYTE    ds3ptrds[5];            /* CCHHR of next F3 DSCB     */
};

struct FORMAT4_DSCB {                   /* DSCB4: VTOC descriptor    */
        BYTE    ds4keyid[44];           /* Key (44 bytes of 0x04)    */
        BYTE    ds4fmtid;               /* Format identifier (0xF4)  */
        BYTE    ds4hpchr[5];            /* CCHHR of highest F1 DSCB  */
        HWORD   ds4dsrec;               /* Number of format 0 DSCBs  */
        BYTE    ds4hcchh[4];            /* CCHH of next avail alt trk*/
        HWORD   ds4noatk;               /* Number of avail alt tracks*/
        BYTE    ds4vtoci;               /* VTOC indicators           */
        BYTE    ds4noext;               /* Number of extents in VTOC */
        BYTE    resv1[2];               /* Reserved                  */
        FWORD   ds4devsz;               /* Device size (CCHH)        */
        HWORD   ds4devtk;               /* Device track length       */
        BYTE    ds4devi;                /* Non-last keyed blk overhd */
        BYTE    ds4devl;                /* Last keyed block overhead */
        BYTE    ds4devk;                /* Non-keyed block difference*/
        BYTE    ds4devfg;               /* Device flags              */
        HWORD   ds4devtl;               /* Device tolerance          */
        BYTE    ds4devdt;               /* Number of DSCBs per track */
        BYTE    ds4devdb;               /* Number of dirblks/track   */
        DBLWRD  ds4amtim;               /* VSAM timestamp            */
        BYTE    ds4vsind;               /* VSAM indicators           */
        HWORD   ds4vscra;               /* CRA track location        */
        DBLWRD  ds4r2tim;               /* VSAM vol/cat timestamp    */
        BYTE    resv2[5];               /* Reserved                  */
        BYTE    ds4f6ptr[5];            /* CCHHR of first F6 DSCB    */
        DSXTENT ds4vtoce;               /* VTOC extent descriptor    */
        BYTE    resv3[25];              /* Reserved                  */
};

/* Bit definitions for ds4vtoci */
#define DS4VTOCI_DOS            0x80    /* Format 5 DSCBs not valid  */
#define DS4VTOCI_DOSSTCK        0x10    /* DOS stacked pack          */
#define DS4VTOCI_DOSCNVT        0x08    /* DOS converted pack        */
#define DS4VTOCI_DIRF           0x40    /* VTOC contains errors      */
#define DS4VTOCI_DIRFCVT        0x20    /* DIRF reclaimed            */

/* Bit definitions for ds4devfg */
#define DS4DEVFG_TOL            0x01    /* Tolerance factor applies to
                                           all but last block of trk */

struct F5AVEXT {                       /* Available extent in DSCB5 */
        HWORD   btrk;                   /* Extent begin track address*/
        HWORD   ncyl;                   /* Number of full cylinders  */
        BYTE    ntrk;                   /* Number of odd tracks      */
};

struct FORMAT5_DSCB {                   /* DSCB5: Free space map     */
        BYTE    ds5keyid[4];            /* Key (4 bytes of 0x05)     */
        F5AVEXT ds5avext[8];            /* First 8 available extents */
        BYTE    ds5fmtid;               /* Format identifier (0xF5)  */
        F5AVEXT ds5mavet[18];           /* 18 more available extents */
        BYTE    ds5ptrds[5];            /* CCHHR of next F5 DSCB     */
};

/*-------------------------------------------------------------------*/
/* Definitions of DSORG and RECFM fields                             */
/*-------------------------------------------------------------------*/
/* Bit settings for dataset organization byte 0 */
#define DSORG_IS                0x80    /* Indexed sequential        */
#define DSORG_PS                0x40    /* Physically sequential     */
#define DSORG_DA                0x20    /* Direct access             */
#define DSORG_PO                0x02    /* Partitioned organization  */
#define DSORG_U                 0x01    /* Unmovable                 */

/* Bit settings for dataset organization byte 1 */
#define DSORG_AM                0x08    /* VSAM dataset              */

/* Bit settings for record format */
#define RECFM_FORMAT            0xC0    /* Bits 0-1=Record format    */
#define RECFM_FORMAT_V          0x40    /* ...variable length        */
#define RECFM_FORMAT_F          0x80    /* ...fixed length           */
#define RECFM_FORMAT_U          0xC0    /* ...undefined length       */
#define RECFM_TRKOFLOW          0x20    /* Bit 2=Track overflow      */
#define RECFM_BLOCKED           0x10    /* Bit 3=Blocked             */
#define RECFM_SPANNED           0x08    /* Bit 4=Spanned or standard */
#define RECFM_CTLCHAR           0x06    /* Bits 5-6=Carriage control */
#define RECFM_CTLCHAR_A         0x04    /* ...ANSI carriage control  */
#define RECFM_CTLCHAR_M         0x02    /* ...Machine carriage ctl.  */

/*-------------------------------------------------------------------*/
/* Definition of PDS directory entry                                 */
/*-------------------------------------------------------------------*/
struct PDSDIR {                         /* PDS directory entry       */
        BYTE    pds2name[8];            /* Member name               */
        BYTE    pds2ttrp[3];            /* TTR of first block        */
        BYTE    pds2indc;               /* Indicator byte            */
        BYTE    pds2usrd[62];           /* User data (0-31 halfwords)*/
};

/* Bit definitions for PDS directory indicator byte */
#define PDS2INDC_ALIAS          0x80    /* Bit 0: Name is an alias   */
#define PDS2INDC_NTTR           0x60    /* Bits 1-2: User TTR count  */
#define PDS2INDC_NTTR_SHIFT     5       /* Shift count for NTTR      */
#define PDS2INDC_LUSR           0x1F    /* Bits 3-7: User halfwords  */

/*-------------------------------------------------------------------*/
/* Text unit keys for transmit/receive                               */
/*-------------------------------------------------------------------*/
#define INMDDNAM        0x0001          /* DDNAME for the file       */
#define INMDSNAM        0x0002          /* Name of the file          */
#define INMMEMBR        0x0003          /* Member name list          */
#define INMSECND        0x000B          /* Secondary space quantity  */
#define INMDIR          0x000C          /* Directory space quantity  */
#define INMEXPDT        0x0022          /* Expiration date           */
#define INMTERM         0x0028          /* Data transmitted as msg   */
#define INMBLKSZ        0x0030          /* Block size                */
#define INMDSORG        0x003C          /* File organization         */
#define INMLRECL        0x0042          /* Logical record length     */
#define INMRECFM        0x0049          /* Record format             */
#define INMTNODE        0x1001          /* Target node name/number   */
#define INMTUID         0x1002          /* Target user ID            */
#define INMFNODE        0x1011          /* Origin node name/number   */
#define INMFUID         0x1012          /* Origin user ID            */
#define INMLREF         0x1020          /* Date last referenced      */
#define INMLCHG         0x1021          /* Date last changed         */
#define INMCREAT        0x1022          /* Creation date             */
#define INMFVERS        0x1023          /* Origin vers# of data fmt  */
#define INMFTIME        0x1024          /* Origin timestamp          */
#define INMTTIME        0x1025          /* Destination timestamp     */
#define INMFACK         0x1026          /* Originator request notify */
#define INMERRCD        0x1027          /* RECEIVE command error code*/
#define INMUTILN        0x1028          /* Name of utility program   */
#define INMUSERP        0x1029          /* User parameter string     */
#define INMRECCT        0x102A          /* Transmitted record count  */
#define INMSIZE         0x102C          /* File size in bytes        */
#define INMFFM          0x102D          /* Filemode number           */
#define INMNUMF         0x102F          /* #of files transmitted     */
#define INMTYPE         0x8012          /* Dataset type              */

/*-------------------------------------------------------------------*/
/* Definitions of IEBCOPY header records                             */
/*-------------------------------------------------------------------*/
struct COPYR1 {                         /* IEBCOPY header record 1   */
        BYTE    uldfmt;                 /* Unload format             */
        BYTE    hdrid[3];               /* Header identifier         */
        HWORD   ds1dsorg;               /* Dataset organization      */
        HWORD   ds1blkl;                /* Block size                */
        HWORD   ds1lrecl;               /* Logical record length     */
        BYTE    ds1recfm;               /* Record format             */
        BYTE    ds1keyl;                /* Key length                */
        BYTE    ds1optcd;               /* Option codes              */
        BYTE    ds1smsfg;               /* SMS indicators            */
        HWORD   uldblksz;               /* Block size of container   */
                                        /* Start of DEVTYPE fields   */
        FWORD   ucbtype;                /* Original device type      */
        FWORD   maxblksz;               /* Maximum block size        */
        HWORD   cyls;                   /* Number of cylinders       */
        HWORD   heads;                  /* Number of tracks/cylinder */
        HWORD   tracklen;               /* Track length              */
        HWORD   overhead;               /* Block overhead            */
        BYTE    keyovhead;              /* Keyed block overhead      */
        BYTE    devflags;               /* Flags                     */
        HWORD   tolerance;              /* Tolerance factor          */
                                        /* End of DEVTYPE fields     */
        HWORD   hdrcount;               /* Number of header records
                                           (if zero, then 2 headers) */
        BYTE    resv1;                  /* Reserved                  */
        BYTE    ds1refd[3];             /* Last reference date       */
        BYTE    ds1scext[3];            /* Secondary space extension */
        BYTE    ds1scalo[4];            /* Secondary allocation      */
        BYTE    ds1lstar[3];            /* Last track used TTR       */
        HWORD   ds1trbal;               /* Last track balance        */
        HWORD   resv2;                  /* Reserved                  */
};

/* Bit settings for unload format byte */
#define COPYR1_ULD_FORMAT       0xC0    /* Bits 0-1=unload format... */
#define COPYR1_ULD_FORMAT_OLD   0x00    /* ...old format             */
#define COPYR1_ULD_FORMAT_PDSE  0x40    /* ...PDSE format            */
#define COPYR1_ULD_FORMAT_ERROR 0x80    /* ...error during unload    */
#define COPYR1_ULD_FORMAT_XFER  0xC0    /* ...transfer format        */
#define COPYR1_ULD_PROGRAM      0x10    /* Bit 3=Contains programs   */
#define COPYR1_ULD_PDSE         0x01    /* Bit 7=Contains PDSE       */

/* Bit settings for header identifier */
#define COPYR1_HDRID    "\xCA\x6D\x0F"  /* Constant value for hdrid  */

struct COPYR2 {                         /* IEBCOPY header record 2   */
        BYTE    debbasic[16];           /* Last 16 bytes of basic
                                           section of original DEB   */
        BYTE    debxtent[16][16];       /* First 16 extent descriptors
                                           from original DEB         */
        FWORD   resv;                   /* Reserved                  */
};

/*-------------------------------------------------------------------*/
/* Definition of data record block in IEBCOPY unload file            */
/*-------------------------------------------------------------------*/
struct DATABLK {                        /* IEBCOPY unload data rec   */
        FWORD   header;                 /* Reserved                  */
        HWORD   cyl;                    /* Cylinder number           */
        HWORD   head;                   /* Head number               */
        BYTE    rec;                    /* Record number             */
        BYTE    klen;                   /* Key length                */
        HWORD   dlen;                   /* Data length               */
#define MAX_DATALEN      32767
        BYTE    kdarea[MAX_DATALEN];    /* Key and data area         */
};

/*-------------------------------------------------------------------*/
/* Internal structures used by DASD utility functions                */
/*-------------------------------------------------------------------*/
struct CIFBLK {                         /* CKD image file descriptor */
        char   *fname;                  /* -> CKD image file name    */
        int     fd;                     /* CKD image file descriptor */
        u_int   trksz;                  /* CKD image track size      */
        BYTE   *trkbuf;                 /* -> Track buffer           */
        u_int   curcyl;                 /* Cylinder number of track
                                           currently in track buffer */
        u_int   curhead;                /* Head number of track
                                           currently in track buffer */
        int     trkmodif;               /* 1=Track has been modified */
        u_int   heads;                  /* Tracks per cylinder       */
        DEVBLK  devblk;                 /* Device Block              */
};

/*-------------------------------------------------------------------*/
/* Functions in module dasdutil.c                                    */
/*-------------------------------------------------------------------*/

CCKD_DLL_IMPORT DEVHND  cckd_dasd_device_hndinfo;
CCKD_DLL_IMPORT DEVHND  cfba_dasd_device_hndinfo;

DUT_DLL_IMPORT void gen_dasd_serial( BYTE* serial );
DUT_DLL_IMPORT void build_vol1( void* buf, const char* volser, const char* owner, bool ckddasd );

DUT_DLL_IMPORT void data_dump      ( void* addr, unsigned int len );
DUT_DLL_IMPORT void data_dump_ascii( void* addr, unsigned int len );

DUT_DLL_IMPORT void data_dump_offset      ( void* addr, unsigned int len, unsigned int offset );
DUT_DLL_IMPORT void data_dump_offset_ascii( void* addr, unsigned int len, unsigned int offset );

DUT_DLL_IMPORT int  read_track (CIFBLK *cif, U32 cyl, U8 head);

int  rewrite_track (CIFBLK *cif);

DUT_DLL_IMPORT int  read_block (CIFBLK *cif, U32 cyl, U8 head, U8 rec,
        BYTE **keyptr, U8 *keylen, BYTE **dataptr, U16 *datalen);

DUT_DLL_IMPORT int  search_key_equal (CIFBLK *cif, BYTE *key, U8 keylen, u_int noext,
        DSXTENT extent[], U32 *cyl, U8 *head, U8 *rec);

DUT_DLL_IMPORT int  convert_tt (u_int tt, u_int noext, DSXTENT extent[], U8 heads,
        U32 *cyl, U8 *head);

#define IMAGE_OPEN_NORMAL   0x00000000
#define IMAGE_OPEN_DASDCOPY 0x00000001
#define IMAGE_OPEN_QUIET    0x00000002  /* (no msgs) */

DUT_DLL_IMPORT   CIFBLK* open_ckd_image   (char *fname, char *sfname, int omode, int option);
DUT_DLL_IMPORT   CIFBLK* open_fba_image   (char *fname, char *sfname, int omode, int option);
DUT64_DLL_IMPORT CIFBLK* open_ckd64_image (char *fname, char *sfname, int omode, int option);
DUT64_DLL_IMPORT CIFBLK* open_fba64_image (char *fname, char *sfname, int omode, int option);

DUT_DLL_IMPORT int  close_ckd_image (CIFBLK *cif);

#define close_image_file(cif)   close_ckd_image((cif))

DUT_DLL_IMPORT int  build_extent_array (CIFBLK *cif, char *dsnama, DSXTENT extent[],
        int *noext);

DUT_DLL_IMPORT int  capacity_calc (CIFBLK *cif, int used, int keylen, int datalen,
        int *newused, int *trkbaln, int *physlen, int *kbconst,
        int *lbconst, int *nkconst, BYTE*devflag, int *tolfact,
        int *maxdlen, int *numrecs, int *numhead, int *numcyls);

DUT_DLL_IMPORT   int create_ckd  ( char *fname, U16 devtype, U32 heads, U32 maxdlen,
        U32 volcyls, char *volser, BYTE comp, BYTE lfs, BYTE dasdcopy,
        BYTE nullfmt, BYTE rawflag, BYTE flagECmode, BYTE flagMachinecheck );
DUT64_DLL_IMPORT int create_ckd64( char *fname, U16 devtype, U32 heads, U32 maxdlen,
        U32 volcyls, char *volser, BYTE comp, BYTE lfs, BYTE dasdcopy,
        BYTE nullfmt, BYTE rawflag, BYTE flagECmode, BYTE flagMachinecheck );

DUT_DLL_IMPORT   int create_fba  ( char *fname, U16 devtype, U32 sectsz, U32 sectors,
        char *volser, BYTE comp, int lfs, int dasdcopy, int rawflag );
DUT64_DLL_IMPORT int create_fba64( char *fname, U16 devtype, U32 sectsz, U32 sectors,
        char *volser, BYTE comp, int lfs, int dasdcopy, int rawflag );

int create_compressed_fba  ( char *fname, U16 devtype, U32 sectsz,
        U32 sectors, char *volser, BYTE comp, int lfs, int dasdcopy,
        int rawflag );
int create_compressed_fba64( char *fname, U16 devtype, U32 sectsz,
        U32 sectors, char *volser, BYTE comp, int lfs, int dasdcopy,
        int rawflag );

int get_verbose_util(void);

DUT_DLL_IMPORT void set_verbose_util( bool v );
DUT_DLL_IMPORT bool is_verbose_util();
DUT_DLL_IMPORT int  next_util_devnum();

DUT_DLL_IMPORT bool valid_dsname( const char* dsname );

DUT_DLL_IMPORT int ckd_tracklen( DEVBLK* dev, BYTE* buf );

int cdsk_valid_trk( int trk, BYTE* buf, int heads, int len );

#define DEFAULT_FBA_TYPE    0x3370

/*-------------------------------------------------------------------*/
/*     Dasd image file classification masks and functions            */
/*-------------------------------------------------------------------*/

#define CKD_P370_TYP    0x80000000      // "CKD_P370" (P=Normal)
#define CKD_C370_TYP    0x40000000      // "CKD_C370" (C=Compressed)
#define CKD_S370_TYP    0x20000000      // "CKD_S370" (S=Shadow)

#define CKD_P064_TYP    0x00800000      // "CKD_P064" (64-bit filesize)
#define CKD_C064_TYP    0x00400000      // "CKD_C064"        "
#define CKD_S064_TYP    0x00200000      // "CKD_S064"        "

#define FBA_P370_TYP    0x00008000      // "FBA_P370" (same for FBA)
#define FBA_C370_TYP    0x00004000      // "FBA_C370"        "
#define FBA_S370_TYP    0x00002000      // "FBA_S370"        "

#define FBA_P064_TYP    0x00000080      // "FBA_P064" (same for FBA)
#define FBA_C064_TYP    0x00000040      // "FBA_C064"        "
#define FBA_S064_TYP    0x00000020      // "FBA_S064"        "

/*-------------------------------------------------------------------*/
/*                 Original 32-bit filesize types                    */
/*-------------------------------------------------------------------*/

#define CKD32_NML_TYP           (CKD_P370_TYP)
#define CKD32_SF_TYP            (CKD_S370_TYP)

#define FBA32_NML_TYP           (FBA_P370_TYP)
#define FBA32_SF_TYP            (FBA_S370_TYP)

#define CKD32_CMP_NO_SF_TYP     (CKD_C370_TYP)
#define FBA32_CMP_NO_SF_TYP     (FBA_C370_TYP)

/*-------------------------------------------------------------------*/

#define CKD32_CMP_OR_SF_TYP     ((CKD32_CMP_NO_SF_TYP) | (CKD32_SF_TYP))
#define FBA32_CMP_OR_SF_TYP     ((FBA32_CMP_NO_SF_TYP) | (FBA32_SF_TYP))

#define CKD32_CMP_OR_NML_TYP    ((CKD32_CMP_NO_SF_TYP) | (CKD32_NML_TYP))
#define FBA32_CMP_OR_NML_TYP    ((FBA32_CMP_NO_SF_TYP) | (FBA32_NML_TYP))

#define ANY32_CMP_NO_SF_TYP     ((CKD32_CMP_NO_SF_TYP) | (FBA32_CMP_NO_SF_TYP))
#define ANY32_CMP_OR_SF_TYP     ((CKD32_CMP_OR_SF_TYP) | (FBA32_CMP_OR_SF_TYP))

#define ANY32_NML_TYP           ((CKD32_NML_TYP) | (FBA32_NML_TYP))
#define ANY32_SF_TYP            ((CKD32_SF_TYP)  | (FBA32_SF_TYP))
#define ANY32_NML_CMP_SF_TYP    ((ANY32_NML_TYP) | (ANY32_CMP_OR_SF_TYP))

/*-------------------------------------------------------------------*/
/*                     New 64-bit filesize types                     */
/*-------------------------------------------------------------------*/

#define CKD64_NML_TYP           (CKD_P064_TYP)
#define CKD64_SF_TYP            (CKD_S064_TYP)

#define FBA64_NML_TYP           (FBA_P064_TYP)
#define FBA64_SF_TYP            (FBA_S064_TYP)

#define CKD64_CMP_NO_SF_TYP     (CKD_C064_TYP)
#define FBA64_CMP_NO_SF_TYP     (FBA_C064_TYP)

/*-------------------------------------------------------------------*/

#define CKD64_CMP_OR_SF_TYP     ((CKD64_CMP_NO_SF_TYP) | (CKD64_SF_TYP))
#define FBA64_CMP_OR_SF_TYP     ((FBA64_CMP_NO_SF_TYP) | (FBA64_SF_TYP))

#define CKD64_CMP_OR_NML_TYP    ((CKD64_CMP_NO_SF_TYP) | (CKD64_NML_TYP))
#define FBA64_CMP_OR_NML_TYP    ((FBA64_CMP_NO_SF_TYP) | (FBA64_NML_TYP))

#define ANY64_CMP_NO_SF_TYP     ((CKD64_CMP_NO_SF_TYP) | (FBA64_CMP_NO_SF_TYP))
#define ANY64_CMP_OR_SF_TYP     ((CKD64_CMP_OR_SF_TYP) | (FBA64_CMP_OR_SF_TYP))

#define ANY64_NML_TYP           ((CKD64_NML_TYP) | (FBA64_NML_TYP))
#define ANY64_SF_TYP            ((CKD64_SF_TYP)  | (FBA64_SF_TYP))
#define ANY64_NML_CMP_SF_TYP    ((ANY64_NML_TYP) | (ANY64_CMP_OR_SF_TYP))

/*-------------------------------------------------------------------*/
/*            Generic EITHER 32-bit/64-bit filesize types            */
/*-------------------------------------------------------------------*/

#define CKD_NML_TYP             (CKD_P370_TYP | CKD_P064_TYP)
#define CKD_SF_TYP              (CKD_S370_TYP | CKD_S064_TYP)

#define FBA_NML_TYP             (FBA_P370_TYP | FBA_P064_TYP)
#define FBA_SF_TYP              (FBA_S370_TYP | FBA_S064_TYP)

#define CKD_CMP_NO_SF_TYP       (CKD_C370_TYP | CKD_C064_TYP)
#define FBA_CMP_NO_SF_TYP       (FBA_C370_TYP | FBA_C064_TYP)

/*-------------------------------------------------------------------*/

#define CKD_CMP_OR_SF_TYP       ((CKD_CMP_NO_SF_TYP) | (CKD_SF_TYP))
#define FBA_CMP_OR_SF_TYP       ((FBA_CMP_NO_SF_TYP) | (FBA_SF_TYP))

#define CKD_CMP_OR_NML_TYP      ((CKD_CMP_NO_SF_TYP) | (CKD_NML_TYP))
#define FBA_CMP_OR_NML_TYP      ((FBA_CMP_NO_SF_TYP) | (FBA_NML_TYP))

#define ANY_CMP_NO_SF_TYP       ((CKD_CMP_NO_SF_TYP) | (FBA_CMP_NO_SF_TYP))
#define ANY_CMP_OR_SF_TYP       ((CKD_CMP_OR_SF_TYP) | (FBA_CMP_OR_SF_TYP))

#define ANY_NML_TYP             ((CKD_NML_TYP) | (FBA_NML_TYP))
#define ANY_SF_TYP              ((CKD_SF_TYP)  | (FBA_SF_TYP))
#define ANY_NML_CMP_SF_TYP      ((ANY_NML_TYP) | (ANY_CMP_OR_SF_TYP))

/*-------------------------------------------------------------------*/

DUT_DLL_IMPORT const char*  dh_devid_str( U32 typmsk );
DUT_DLL_IMPORT U32          dh_devid_typ( BYTE* dh_devid );
DUT_DLL_IMPORT bool      is_dh_devid_typ( BYTE* dh_devid, U32 typmsk );

/*-------------------------------------------------------------------*/

#define CKDTYP( _cmp, _64 )     ((_64) ? ((_cmp) ? "CCKD64" : "CKD64") \
                                       : ((_cmp) ? "CCKD"   : "CKD"))

#define FBATYP( _cmp, _64 )     ((_64) ? ((_cmp) ? "CFBA64" : "FBA64") \
                                       : ((_cmp) ? "CFBA"   : "FBA"))

/*-------------------------------------------------------------------*/
