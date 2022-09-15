/* CCKD64.H    (C) Copyright "Fish" (David B. Trout), 2018-2022      */
/*              CCKD64 dasd struct and type definitions              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CCKD64_H_
#define _CCKD64_H_

#include "cckd.h"           // (need CCKD_MAX_SF, etc)

/*-------------------------------------------------------------------*/
/*  IMPORTANT PROGRAMMING NOTE: for whatever reason, base dasd       */
/*  image files use a L2_trkoff value of zero in their L1tab entry   */
/*  for non-existent tracks, whereas shadow files use a value of -1. */
/*  To ensure we remain 100% compatible with all non-CCKD64 versions */
/*  of Hercules, we MUST enforce this same inconsistent use design.  */
/*-------------------------------------------------------------------*/

#define CCKD64_NOSIZE               ((U64)0)
#define CCKD64_MAXSIZE              ((U64)MAX_OFFSET_T)

#define CCKD64_BASE_NO_OFFSET       CCKD64_NOSIZE
#define CCKD64_SHADOW_NO_OFFSET     CCKD64_MAXSIZE

/*-------------------------------------------------------------------*/
/*                    struct typedefs                                */
/*-------------------------------------------------------------------*/
typedef struct CCKD64_DEVHDR    CCKD64_DEVHDR;  // Compress device header
typedef struct CCKD64_L2ENT     CCKD64_L2ENT;   // Level 2 table entry
typedef struct CCKD64_FREEBLK   CCKD64_FREEBLK; // Free block
typedef struct CCKD64_IFREEBLK  CCKD64_IFREEBLK;// Free block (internal)
typedef struct CCKD64_EXT       CCKD64_EXT;     // CCKD Extension block
typedef struct SPCTAB64         SPCTAB64;       // Space table

/*-------------------------------------------------------------------*/
/*                   Record layouts and sizes                        */
/*-------------------------------------------------------------------*/
#define CKD_R0       CKD_RECHDR         /* Record-0 count field      */
#define CKD_EOFREC   CKD_RECHDR         /* An END-OF-FILE record is     \
                                           a count field with klen      \
                                           of zero and dlen of zero,    \
                                           i.e. a zero length record */
static
const   CKD_RECHDR   CKD_ENDTRK  =      /* END-OF-TRACK marker is a     \
                                           count field (i.e. record     \
                                           header) consisting of...  */ \
    {{0xff,0xff},{0xff,0xff},0xff,0xff,{0xff,0xff}}; /* ...all 0xffs */

#define CKD_R0_SIZE             ((S64)sizeof(CKD_R0))
#define CKD_R0_DLEN             (8)
#define CKD_EOF_SIZE            ((S64)sizeof(CKD_EOFREC))
#define CKD_ENDTRK_SIZE         ((S64)sizeof(CKD_ENDTRK))
#define CKD_MIN_TRKSIZE         (CKD_TRKHDR_SIZE + CKD_ENDTRK_SIZE)
#define CKD_NULL_FMT2_DLEN      (4096)

/*-------------------------------------------------------------------*/
/*   Structure definitions for Compressed CCKD64/CFBA64 devices      */
/*-------------------------------------------------------------------*/
/*    NOTE: The num_L1tab, num_L2tab, cyls, cdh_size, cdh_used,      */
/*    free_off, free_total, free_largest, free_num, free_imbed,      */
/*    and cmp_parm fields are kept in LITTLE endian format.          */
/*-------------------------------------------------------------------*/
struct CCKD64_DEVHDR                    /* Compress device header    */
{
/*  0 */BYTE             cdh_vrm[3];    /* Version Release Modifier  */
/*  3 */BYTE             cdh_opts;      /* Options byte              */
/*  4 */S32              num_L1tab;     /* Number of L1tab entries   */
/*  8 */S32              num_L2tab;     /* Number of L2tab entries   */
/* 12 */FWORD            cdh_cyls;      /* CCKD: Cylinders on device
                                           CFBA: Sectors   on device */
/* 16 */U64              cdh_size;      /* File size                 */
/* 24 */U64              cdh_used;      /* File used                 */
/* 32 */U64              free_off;      /* Offset to free space      */
/* 40 */U64              free_total;    /* Total free space          */
/* 48 */U64              free_largest;  /* Largest free space        */
/* 56 */S64              free_num;      /* Number free spaces        */
/* 64 */U64              free_imbed;    /* Imbedded free space       */
/* 72 */BYTE             cdh_nullfmt;   /* Null track format         */
/* 73 */BYTE             cmp_algo;      /* Compression algorithm     */
/* 74 */S16              cmp_parm;      /* Compression parameter     */
/* 76 */BYTE             resv2[436];    /* Reserved                  */
};

struct CCKD64_L2ENT {                   /* Level 2 table entry       */

/* NOTE: all fields are numeric and always in LITTLE endian format.  */

        U64              L2_trkoff;     /* Offset to track image     */
        U16              L2_len;        /* Track length              */
        U16              L2_size;       /* Track size  (size >= len) */
        U32              L2_pad;        /* (padding/reserved)        */
};

struct CCKD64_FREEBLK {                 /* Free block (file)         */

/* NOTE: all fields are numeric and always in LITTLE endian format.  */

        U64              fb_offnxt;     /* Offset to NEXT free blk   */
        U64              fb_len;        /* Length of THIS free blk    */
};

struct CCKD64_IFREEBLK {                /* Free block (internal)     */

/* NOTE: because this control block is an INTERNAL control block     */
/* which does not actually exist in the emulated dasd image file     */
/* (i.e. it is only used internally), all of its fields are always   */
/* in *HOST* endian format (little endian on little endian hosts     */
/* and big endian on big endian hosts).                              */

        U64              ifb_offnxt;    /* Offset to NEXT free blk   */
        U64              ifb_len;       /* Length of THIS free blk   */

        int              ifb_idxprv;    /* Index to prev free blk    */
        int              ifb_idxnxt;    /* Index to next free blk    */
        int              ifb_pending;   /* 1=Free pending (don't use)*/
        int              ifb_pad;       /* (padding/reserved)        */
};

/*-------------------------------------------------------------------*/
/*                    struct sizes, etc.                             */
/*-------------------------------------------------------------------*/
typedef  U64          CCKD64_L1ENT;     /* Level 1 table entry       */
typedef  CCKD64_L1ENT CCKD64_L1TAB[];   /* Level 1 table             */
typedef  CCKD64_L2ENT CCKD64_L2TAB[256];/* Level 2 table             */

#define CCKD_TRACE_SIZE        ((S64)sizeof(CCKD_ITRACE))
#define CCKD64_DEVHDR_SIZE     ((S64)sizeof(CCKD64_DEVHDR))
#define CCKD64_L1ENT_SIZE      ((S64)sizeof(CCKD64_L1ENT))
#define CCKD64_L2ENT_SIZE      ((S64)sizeof(CCKD64_L2ENT))
#define CCKD64_L2TAB_SIZE      ((S64)sizeof(CCKD64_L2TAB))
#define CCKD64_FREEBLK_SIZE    ((S64)sizeof(CCKD64_FREEBLK))
#define CCKD64_IFREEBLK_SIZE   ((S64)sizeof(CCKD64_IFREEBLK))

#define CCKD64_DEVHDR_POS      (CKD_DEVHDR_SIZE)
#define CCKD64_L1TAB_POS       ((CCKD64_DEVHDR_POS)+(CCKD64_DEVHDR_SIZE))

#define FBA_SECTOR_SIZE        512
#define CFBA_BLKS_PER_GRP      120
#define CFBA_BLKGRP_SIZE       (FBA_SECTOR_SIZE * CFBA_BLKS_PER_GRP)

/*-------------------------------------------------------------------*/
/*                   CCKD Extension Block                            */
/*-------------------------------------------------------------------*/
struct CCKD64_EXT {                     /* Ext for compressed ckd    */
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
        CCKD64_L2ENT    *L2tab;         /* Active level 2 table      */
        int              L2_active;     /* Active level 2 cache entry*/
        U64              L2_bounds;     /* L2 tables boundary        */

        int              active;        /* Active cache entry        */
        BYTE            *newbuf;        /* Uncompressed buffer       */

        CCKD64_IFREEBLK *ifb;           /* Internal free space chain */
        int              free_count;    /* Number of entries in chain*/
        int              free_idx1st;   /* Index of 1st entry        */
        int              free_idxlast;  /* Index of last entry       */
        int              free_idxavail; /* Index of available entry  */
        unsigned int     free_minsize;  /* Minimum free space size   */

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
        CCKD64_L1ENT    *L1tab[CCKD_MAX_SF+1];   /* Level 1 tables   */
        CCKD64_DEVHDR    cdevhdr[CCKD_MAX_SF+1]; /* cckd device hdr  */
};

/*-------------------------------------------------------------------*/
/*                         Space table                               */
/*-------------------------------------------------------------------*/
struct SPCTAB64
{
    BYTE        spc_typ;                /* Type of space             */
    int         spc_val;                /* Value for space           */
    int         spc_val2;               /* Another value for space   */
    U64         spc_off;                /* Space offset              */
    U64         spc_len;                /* Space length              */
    U64         spc_siz;                /* Space size                */
};

#endif // _CCKD64_H_
