/* CCKD.H      (C) Copyright "Fish" (David B. Trout), 2018-2019      */
/*              CCKD dasd struct and type definitions                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CCKD_H_
#define _CCKD_H_

/*-------------------------------------------------------------------*/
/*                    struct typedefs                                */
/*-------------------------------------------------------------------*/
typedef struct CKD_DEVHDR       CKD_DEVHDR;     // Device header
typedef struct CKD_TRKHDR       CKD_TRKHDR;     // Track header
typedef struct FBA_BKGHDR       FBA_BKGHDR;     // Block Group header
typedef struct CKD_RECHDR       CKD_RECHDR;     // Record header
typedef struct CCKD_DEVHDR      CCKD_DEVHDR;    // Compress device header
typedef struct CCKD_L2ENT       CCKD_L2ENT;     // Level 2 table entry
typedef struct CCKD_FREEBLK     CCKD_FREEBLK;   // Free block
typedef struct CCKD_IFREEBLK    CCKD_IFREEBLK;  // Free block (internal)
typedef struct CCKD_RA          CCKD_RA;        // Readahead queue entry
typedef struct CCKDBLK          CCKDBLK;        // Global CCKD dasd block
typedef struct CCKD_EXT         CCKD_EXT;       // CCKD Extension block
typedef struct SPCTAB           SPCTAB;         // Space table

/*-------------------------------------------------------------------*/
/*            Structure definitions for CKD headers                  */
/*-------------------------------------------------------------------*/
/*          NOTE: The dh_heads, dh_trksize and dh_highcyl            */
/*          values are always kept in LITTLE endian format.          */
/*-------------------------------------------------------------------*/
struct CKD_DEVHDR                       /* Device header             */
{
        BYTE    dh_devid[8];            /* ASCII Device Header id;
                                           see dasdblks.h for list   */
        FWORD   dh_heads;               /* CKD: heads per cylinder
                                           CFBA: number of sectors   */
        FWORD   dh_trksize;             /* CKD: track size
                                           CFBA: sector size         */
        BYTE    dh_devtyp;              /* Low byte of hex device type
                                           (x80=3380, x90=3390, etc) */
        BYTE    dh_fileseq;             /* CKD: image file sequence no.
                                           (0=only file, 1=first file
                                           of multiple files)
                                           CFBA: 0 (not used)        */
        HWORD   dh_highcyl;             /* CKD: Highest cylinder number
                                           on this file, or zero if this
                                           is the last or only file.
                                           CFBA: zero (not used)     */
        BYTE    resv[492];              /* Reserved                  */
};

struct CKD_TRKHDR {                     /* Track header              */
        BYTE    bin;                    /* Bin number                */
        HWORD   cyl;                    /* Cylinder number           */
        HWORD   head;                   /* Head number               */
};

struct FBA_BKGHDR {                     /* Block Group Header        */
        BYTE    cmp;                    /* Compression byte          */
        FWORD   blknum;                 /* Sector number             */
};

struct CKD_RECHDR {                     /* Record header             */
        HWORD   cyl;                    /* Cylinder number           */
        HWORD   head;                   /* Head number               */
        BYTE    rec;                    /* Record number             */
        BYTE    klen;                   /* Key length                */
        HWORD   dlen;                   /* Data length               */
};

#define CKD_DEVHDR_SIZE         ((ssize_t)sizeof(CKD_DEVHDR))
#define CKD_TRKHDR_SIZE         ((ssize_t)sizeof(CKD_TRKHDR))
#define FBA_BKGHDR_SIZE         ((ssize_t)sizeof(FBA_BKGHDR))
#define CKD_RECHDR_SIZE         ((ssize_t)sizeof(CKD_RECHDR))

/* Null track formats */
#define CKD_NULLTRK_FMT0        0       /* ha r0 eof eot             */
#define CKD_NULLTRK_FMT1        1       /* ha r0 eot                 */
#define CKD_NULLTRK_FMT2        2       /* linux (3390 only)         */
#define CKD_NULLTRK_FMTMAX      CKD_NULLTRK_FMT2

#define CKD_NULLTRK_SIZE0       (CKD_TRKHDR_SIZE + CKD_R0_SIZE + CKD_R0_DLEN + CKD_EOF_SIZE + CKD_ENDTRK_SIZE)
#define CKD_NULLTRK_SIZE1       (CKD_TRKHDR_SIZE + CKD_R0_SIZE + CKD_R0_DLEN                + CKD_ENDTRK_SIZE)
#define CKD_NULLTRK_SIZE2       (CKD_TRKHDR_SIZE + CKD_R0_SIZE + CKD_R0_DLEN + (12 * (CKD_RECHDR_SIZE + CKD_NULL_FMT2_DLEN)) + CKD_ENDTRK_SIZE)

/*-------------------------------------------------------------------*/
/*     Structure definitions for Compressed CCKD/CFBA devices        */
/*-------------------------------------------------------------------*/
/*    NOTE: The num_L1tab, num_L2tab, cyls, cdh_size, cdh_used,      */
/*    free_off, free_total, free_largest, free_num, free_imbed,      */
/*    and cmp_parm fields are kept in LITTLE endian format.          */
/*-------------------------------------------------------------------*/
struct CCKD_DEVHDR                      /* Compress device header    */
{
/*  0 */BYTE             cdh_vrm[3];    /* Version Release Modifier  */
/*  3 */BYTE             cdh_opts;      /* Options byte              */
/*  4 */S32              num_L1tab;     /* Number of L1tab entries   */
/*  8 */S32              num_L2tab;     /* Number of L2tab entries   */
/* 12 */U32              cdh_size;      /* File size                 */
/* 16 */U32              cdh_used;      /* File used                 */
/* 20 */U32              free_off;      /* Offset to free space      */
/* 24 */U32              free_total;    /* Total free space          */
/* 28 */U32              free_largest;  /* Largest free space        */
/* 32 */S32              free_num;      /* Number free spaces        */
/* 36 */U32              free_imbed;    /* Imbedded free space       */
/* 40 */FWORD            cdh_cyls;      /* CCKD: Cylinders on device
                                           CFBA: Sectors   on device */
/* 44 */BYTE             cdh_nullfmt;   /* Null track format         */
/* 45 */BYTE             cmp_algo;      /* Compression algorithm     */
/* 46 */S16              cmp_parm;      /* Compression parameter     */
/* 48 */BYTE             resv2[464];    /* Reserved                  */
};

#define CCKD_VERSION           0
#define CCKD_RELEASE           3
#define CCKD_MODLVL            1

#define CCKD_OPT_BIGEND        0x02  /* file in BIG endian format    */
#define CCKD_OPT_SPERRS        0x20  /* Space errors detected        */
#define CCKD_OPT_OPENRW        0x40  /* Opened R/W since last chkdsk */
#define CCKD_OPT_OPENED        0x80

#define CCKD_COMPRESS_NONE     0x00
#define CCKD_COMPRESS_ZLIB     0x01
#define CCKD_COMPRESS_BZIP2    0x02
#define CCKD_COMPRESS_MASK     0x03

#define CCKD_STRESS_MINLEN     4096
#if defined( HAVE_ZLIB )
#define CCKD_STRESS_COMP       CCKD_COMPRESS_ZLIB
#else
#define CCKD_STRESS_COMP       CCKD_COMPRESS_NONE
#endif
#define CCKD_STRESS_PARM1      4
#define CCKD_STRESS_PARM2      2

struct CCKD_L2ENT {                     /* Level 2 table entry       */

/* NOTE: all fields are numeric and always in LITTLE endian format.  */

        U32              L2_trkoff;     /* Offset to track image     */
        U16              L2_len;        /* Track length              */
        U16              L2_size;       /* Track size  (size >= len) */
};

struct CCKD_FREEBLK {                   /* Free block (file)         */

/* NOTE: all fields are numeric and always in LITTLE endian format.  */

        U32              fb_offnxt;     /* Offset to next free blk   */
        U32              fb_len;        /* Length this free blk      */
};

struct CCKD_IFREEBLK {                  /* Free block (internal)     */

/* NOTE: because this control block is an INTERNAL control block     */
/* which does not actually exist in the emulated dasd image file     */
/* (i.e. it is only used internally), all of its fields are always   */
/* in *HOST* endian format (little endian on little endian hosts     */
/* and big endian on big endian hosts).                              */

        U32              ifb_offnxt;    /* Offset to next free blk   */
        U32              ifb_len;       /* Length this free blk      */
        int              ifb_idxprv;    /* Index to prev free blk    */
        int              ifb_idxnxt;    /* Index to next free blk    */
        int              ifb_pending;   /* 1=Free pending (don't use)*/
};

struct CCKD_RA {                        /* Readahead queue entry     */
        DEVBLK          *ra_dev;        /* Readahead device          */
        int              ra_trk;        /* Readahead track           */
        int              ra_idxprv;     /* Index to prev entry       */
        int              ra_idxnxt;     /* Index to next entry       */
};

typedef  U32          CCKD_L1ENT;       /* Level 1 table entry       */
typedef  CCKD_L1ENT   CCKD_L1TAB[];     /* Level 1 table             */
typedef  CCKD_L2ENT   CCKD_L2TAB[256];  /* Level 2 table             */
typedef  char         CCKD_TRACE[128];  /* Trace table entry         */

#define CCKD_DEVHDR_SIZE       ((ssize_t)sizeof(CCKD_DEVHDR))
#define CCKD_L1ENT_SIZE        ((ssize_t)sizeof(CCKD_L1ENT))
#define CCKD_L2ENT_SIZE        ((ssize_t)sizeof(CCKD_L2ENT))
#define CCKD_L2TAB_SIZE        ((ssize_t)sizeof(CCKD_L2TAB))
#define CCKD_FREEBLK_SIZE      ((ssize_t)sizeof(CCKD_FREEBLK))
#define CCKD_IFREEBLK_SIZE     ((ssize_t)sizeof(CCKD_IFREEBLK))

#define CCKD_DEVHDR_POS        (CKD_DEVHDR_SIZE)
#define CCKD_L1TAB_POS         ((CCKD_DEVHDR_POS)+(CCKD_DEVHDR_SIZE))

/* Flag bits */
#define CCKD_SIZE_EXACT        0x01     /* Space obtained is exact   */
#define CCKD_SIZE_ANY          0x02     /* Space can be any size     */
#define CCKD_L2SPACE           0x04     /* Space for a l2 table      */

/* adjustable values */

#define CCKD_FREE_MIN_SIZE     96       /* Minimum free space size   */
#define CCKD_FREE_MIN_INCR     32       /* Added for each 1024 spaces*/
#define CCKD_COMPRESS_MIN      512      /* Track images smaller than
                                           this won't be compressed  */
#define CCKD_MAX_SF            8        /* Maximum number of shadow
                                           files: 0 to 9 [0 disables
                                           shadow file support]      */
#define CCKD_MAX_READAHEADS    16       /* Max readahead trks        */
#define CCKD_MAX_RA_SIZE       16       /* Readahead queue size      */
#define CCKD_MAX_RA            9        /* Max readahead threads     */
#define CCKD_MAX_WRITER        9        /* Max writer threads        */
#define CCKD_MAX_GCOL          1        /* Max garbage collectors    */
#define CCKD_DEF_TRACE         3        /* Def nbr trace entries     */
#define CCKD_MAX_TRACE         200000   /* Max nbr trace entries     */
#define CCKD_MAX_FREEPEND      4        /* Max free pending cycles   */

#define CCKD_MIN_READAHEADS    0        /* Min readahead trks        */
#define CCKD_MIN_RA            0        /* Min readahead threads     */
#define CCKD_MIN_WRITER        1        /* Min writer threads        */
#define CCKD_MIN_GCOL          0        /* Min garbage collectors    */

#define CCKD_DEF_RA_SIZE       4        /* Readahead queue size      */
#define CCKD_DEF_RA            2        /* Default number readaheads */
#define CCKD_DEF_WRITER        2        /* Default number writers    */
#define CCKD_DEF_GCOL          1        /* Default number garbage
                                              collectors             */
#define CCKD_DEF_GCOLWAIT     10        /* Default wait (seconds)    */
#define CCKD_DEF_GCOLPARM      0        /* Default adjustment parm   */
#define CCKD_DEF_READAHEADS    2        /* Default nbr to read ahead */
#define CCKD_DEF_FREEPEND     -1        /* Default freepend cycles   */

/*-------------------------------------------------------------------*/
/*                   Global CCKD dasd block                          */
/*-------------------------------------------------------------------*/
struct CCKDBLK {                        /* Global cckd dasd block    */
        BYTE             id[8];         /* "CCKDBLK "                */
#define CCKDBLK_ID      "CCKDBLK "      /* "CCKDBLK "                */
        DEVBLK          *dev1st;        /* 1st device in cckd queue  */
        unsigned int     batch:1,       /* 1=called in batch mode    */
                         debug:1,       /* 1=CCW trace debug msgs    */
                         sfmerge:1,     /* 1=sf-* merge              */
                         sfforce:1;     /* 1=sf-* force              */
        int              sflevel;       /* sfk xxxx level            */
        int              batchml;       /* message level for batch ops  */

        BYTE             comps;         /* Supported compressions    */
        BYTE             comp;          /* Override compression      */
        int              compparm;      /* Override compression parm */

        LOCK             gclock;        /* Garbage collector lock    */
        COND             gccond;        /* Garbage collector cond    */
        int              gcs;           /* Number garbage collector threads started */
        int              gca;           /* Number garbage collector threads active */
        int              gcmax;         /* Max garbage collectors    */
        int              gcwait;        /* Wait time in seconds      */
        int              gcparm;        /* Adjustment parm           */

        LOCK             wrlock;        /* I/O lock                  */
        COND             wrcond;        /* I/O condition             */
        int              wrpending;     /* Number writes pending     */
        int              wrwaiting;     /* Number writers waiting    */
        int              wrs;           /* Number writer threads started  */
        int              wra;           /* Number writer threads active  */
        int              wrmax;         /* Max writer threads        */
        int              wrprio;        /* Writer thread priority    */

        LOCK             ralock;        /* Readahead lock            */
        COND             racond;        /* Readahead condition       */
        int              ras;           /* Number readahead threads started */
        int              raa;           /* Number readadead threads active */
        int              ramax;         /* Max readahead threads     */
        int              rawaiting;     /* Number threads waiting    */
        int              ranbr;         /* Readahead queue size      */
        int              readaheads;    /* Nbr tracks to read ahead  */
        CCKD_RA          ra[CCKD_MAX_RA_SIZE];    /* Readahead queue */
        int              ra1st;         /* First readahead entry     */
        int              ralast;        /* Last readahead entry      */
        int              rafree;        /* Free readahead entry      */

        LOCK             devlock;       /* Device chain lock         */
        COND             devcond;       /* Device chain condition    */
        int              devusers;      /* Number shared users       */
        int              devwaiters;    /* Number of waiters         */

        int              freepend;      /* Number freepend cycles    */
        int              nosfd;         /* 1=No stats rpt at close   */
        int              nostress;      /* 1=No stress writes        */
        int              linuxnull;     /* 1=Always check nulltrk    */
        int              fsync;         /* 1=Perform fsync()         */
        COND             termcond;      /* Termination condition     */

        U64              stats_switches;       /* Switches           */
        U64              stats_cachehits;      /* Cache hits         */
        U64              stats_cachemisses;    /* Cache misses       */
        U64              stats_readaheads;     /* Readaheads         */
        U64              stats_readaheadmisses;/* Readahead misses   */
        U64              stats_iowaits;        /* Waits for i/o      */
        U64              stats_cachewaits;     /* Waits for cache    */
        U64              stats_stresswrites;   /* Writes under stress*/
        U64              stats_l2cachehits;    /* L2 cache hits      */
        U64              stats_l2cachemisses;  /* L2 cache misses    */
        U64              stats_l2reads;        /* L2 reads           */
        U64              stats_reads;          /* Number reads       */
        U64              stats_readbytes;      /* Bytes read         */
        U64              stats_writes;         /* Number writes      */
        U64              stats_writebytes;     /* Bytes written      */
        U64              stats_gcolmoves;      /* Spaces moved       */
        U64              stats_gcolbytes;      /* Bytes moved        */

        CCKD_TRACE      *itrace;        /* Internal trace table      */
        CCKD_TRACE      *itracep;       /* Current pointer           */
        CCKD_TRACE      *itracex;       /* End of trace table        */
        int              itracen;       /* Number of entries         */

        int              bytemsgs;      /* Limit for `byte 0' msgs   */
};

/*-------------------------------------------------------------------*/
/*                   CCKD Extension Block                            */
/*-------------------------------------------------------------------*/
struct CCKD_EXT {                       /* Ext for compressed ckd    */
        DEVBLK          *devnext;       /* cckd device queue         */

        unsigned int     ckddasd:1,     /* 1=CKD dasd                */
                         fbadasd:1,     /* 1=FBA dasd                */
                         cckdioact:1,   /* 1=Channel program active  */
                         bufused:1,     /* 1=newbuf was used         */
                         updated:1,     /* 1=Update occurred         */
                         merging:1,     /* 1=File merge in progress  */
                         stopping:1,    /* 1=Device is closing       */
                         notnull:1,     /* 1=Device has track images */
                         L2ok:1,        /* 1=All l2s below bounds    */
                         sfmerge:1,     /* 1=sf-xxxx merge           */
                         sfforce:1;     /* 1=sf-xxxx force           */

        int              sflevel;       /* sfk xxxx level            */

        LOCK             filelock;      /* File lock                 */
        LOCK             cckdiolock;    /* I/O lock                  */
        COND             cckdiocond;    /* I/O condition             */

        U64              cckd_maxsize;  /* Maximum file size         */

        int              cckdwaiters;   /* Number I/O waiters        */
        int              wrpending;     /* Number writes pending     */
        int              ras;           /* Number readaheads active  */
        int              sfn;           /* Number active shadow files*/

        int              sfx;           /* Active level 2 file index */
        int              L1idx;         /* Active level 2 table index*/
        CCKD_L2ENT      *L2tab;         /* Active level 2 table      */
        int              L2_active;     /* Active level 2 cache entry*/
        U64              L2_bounds;     /* L2 tables boundary        */

        int              active;        /* Active cache entry        */
        BYTE            *newbuf;        /* Uncompressed buffer       */
        unsigned int     freemin;       /* Minimum free space size   */
        CCKD_IFREEBLK   *ifb;           /* Internal free space chain */

        int              free_count;    /* Number free space entries */
        int              free_idx1st;   /* Index of 1st entry        */
        int              free_idxlast;  /* Index of last entry       */
        int              free_idxavail; /* Index of available entry  */

        int              lastsync;      /* Time of last sync         */

        int              ralkup[CCKD_MAX_RA_SIZE];/* Lookup table    */

        int              ratrk;         /* Track to readahead        */
        unsigned int     totreads;      /* Total nbr trk reads       */
        unsigned int     totwrites;     /* Total nbr trk writes      */
        unsigned int     totl2reads;    /* Total nbr l2 reads        */

        unsigned int     cachehits;     /* Cache hits                */

        unsigned int     readaheads;    /* Number trks read ahead    */
        unsigned int     switches;      /* Number trk switches       */
        unsigned int     misses;        /* Number readahead misses   */

        int              fd[CCKD_MAX_SF+1];      /* File descriptors */
        BYTE             swapend[CCKD_MAX_SF+1]; /* Swap endian flag */
        BYTE             open[CCKD_MAX_SF+1];    /* Open flag        */
        int              reads[CCKD_MAX_SF+1];   /* Nbr track reads  */
        int              L2_reads[CCKD_MAX_SF+1];/* Nbr l2 reads     */
        int              writes[CCKD_MAX_SF+1];  /* Nbr track writes */
        CCKD_L1ENT      *L1tab[CCKD_MAX_SF+1];   /* Level 1 tables   */
        CCKD_DEVHDR      cdevhdr[CCKD_MAX_SF+1]; /* cckd device hdr  */
};

#define CCKD_OPEN_NONE         0
#define CCKD_OPEN_RO           1
#define CCKD_OPEN_RD           2
#define CCKD_OPEN_RW           3

/*-------------------------------------------------------------------*/
/*                         Space table                               */
/*-------------------------------------------------------------------*/
struct SPCTAB
{
    BYTE        spc_typ;                /* Type of space             */
    int         spc_val;                /* Value for space           */
    U32         spc_off;                /* Space offset              */
    U32         spc_len;                /* Space length              */
    U32         spc_siz;                /* Space size                */
};

#define SPCTAB_NONE     0               /* Ignore this space entry   */
#define SPCTAB_DEVHDR   1               /* Space is device header    */
#define SPCTAB_CDEVHDR  2               /* Space is compressed hdr   */
#define SPCTAB_L1       3               /* Space is level 1 table    */
#define SPCTAB_L2       4               /* Space is level 2 table    */
#define SPCTAB_TRK      5               /* Space is track image      */
#define SPCTAB_BLKGRP   6               /* Space is blkgrp image     */
#define SPCTAB_FREE     7               /* Space is free block       */
#define SPCTAB_EOF      8               /* Space is end-of-file      */

/*-------------------------------------------------------------------*/
/*                 CCKD64 structs and definitions                    */
/*-------------------------------------------------------------------*/

#include "cckd64.h"      /* 64-bit CCKD64 structs/constants */

#endif // _CCKD_H_
