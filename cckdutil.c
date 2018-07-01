/* CCKDUTIL.C   (C) Copyright Roger Bowler, 1999-2012                */
/*       ESA/390 Compressed CKD Common routines                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains functions for compressed CKD devices         */
/* used by more than 1 main program.                                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _CCKDUTIL_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "opcode.h"
#include "dasdblks.h"

/*-------------------------------------------------------------------*/
/* Space table                                                       */
/*-------------------------------------------------------------------*/
struct SPCTAB
{
    BYTE        spc_typ;                /* Type of space             */
    int         spc_val;                /* Value for space           */
    U32         spc_off;                /* Space offset              */
    U32         spc_len;                /* Space length              */
    U32         spc_siz;                /* Space size                */
};
typedef struct SPCTAB SPCTAB;

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
/* Internal functions                                                */
/*-------------------------------------------------------------------*/
static int  comp_spctab_sort(const void *a, const void *b);
static int  cdsk_spctab_sort(const void *a, const void *b);
static int  cdsk_build_free_space(SPCTAB *spctab, int s);
static int  cdsk_valid_trk (int trk, BYTE *buf, int heads, int len);

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
static BYTE  eighthexFF[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static char *spaces[] = { "none", "devhdr", "cdevhdr", "l1",  "l2",
                          "trk",  "blkgrp", "free",    "eof" };
static char *comps[]  = { "none", "zlib",   "bzip2" };

#define  gui_fprintf        if (extgui) fprintf

/*-------------------------------------------------------------------*/
/* Change the endianess of a compressed file                         */
/*-------------------------------------------------------------------*/
DLL_EXPORT int cckd_swapend (DEVBLK *dev)
{
CCKD_EXT         *cckd;                 /* -> cckd extension         */
int               fd;                   /* File descriptor           */
int               rc;                   /* Return code               */
struct stat       fst;                  /* File status buffer        */
int               i;                    /* Index                     */
int               swapend;              /* 1=swap space              */
int               len;                  /* Length                    */
off_t             off, lopos, hipos;    /* File offsets              */
CCKD_DEVHDR       cdevhdr;              /* Compressed ckd header     */
CCKD_L1ENT       *l1 = NULL;            /* Level 1 table             */
CCKD_L2ENT        l2[256];              /* Level 2 table             */
CCKD_FREEBLK      freeblk;              /* Free block                */

    /* Get fd */
    cckd = dev->cckd_ext;
    if (cckd == NULL)
        fd = dev->fd;
    else
        fd = cckd->fd[cckd->sfn];

    /* Get file size */
    if (fstat (fd, &fst) < 0)
        goto cswp_fstat_error;
    gui_fprintf (stderr, "SIZE=%"PRIu64"\n", (U64) fst.st_size);
    hipos = fst.st_size;

    /* Device header */
    off = CCKD_DEVHDR_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD_DEVHDR_SIZE;
    if ((rc = read (fd, &cdevhdr, len)) != len)
        goto cswp_read_error;
    swapend = (cdevhdr.opts & CCKD_BIGENDIAN) != cckd_endian();
    cckd_swapend_chdr (&cdevhdr);
    cdevhdr.opts |= CCKD_ORDWR;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = write (fd, &cdevhdr, len)) != len)
        goto cswp_write_error;
    if (!swapend) cckd_swapend_chdr (&cdevhdr);

    /* l1 table */
    len = cdevhdr.num_L1tab * CCKD_L1ENT_SIZE;
    if ((l1 = malloc (len)) == NULL)
        goto cswp_malloc_error;
    off = CCKD_L1TAB_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = read (fd, l1, len)) != len)
        goto cswp_read_error;
    cckd_swapend_l1 (l1, (int)cdevhdr.num_L1tab);
    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = write (fd, l1, len)) != len)
        goto cswp_write_error;
    if (!swapend) cckd_swapend_l1 (l1, (int)cdevhdr.num_L1tab);
    lopos = CCKD_L1TAB_POS + len;

    /* l2 tables */
    for (i = 0; i < cdevhdr.num_L1tab; i++)
    {
        if (l1[i] == 0    || l1[i] == 0xffffffff
         || l1[i] < lopos || l1[i] > hipos - CCKD_L2TAB_SIZE)
            continue;
        off = (off_t)l1[i];
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cswp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = CCKD_L2TAB_SIZE;
        if ((rc = read (fd, l2, len)) != len)
            goto cswp_read_error;
        cckd_swapend_l2 (l2);
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cswp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        if ((rc = write (fd, l2, len)) != len)
            goto cswp_write_error;
    }

    free (l1);
    l1 = NULL;

    /* free space */
    if (cdevhdr.free_off && cdevhdr.free_off >= lopos
     && cdevhdr.free_off <= hipos - CCKD_FREEBLK_SIZE)
    {
        off = (off_t)cdevhdr.free_off;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cswp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = CCKD_FREEBLK_SIZE;
        if ((rc = read (fd, &freeblk, len)) != len)
            goto cswp_read_error;
        if (memcmp(&freeblk, "FREE_BLK", 8) == 0)
        {
            /* New format free space */
            for (i = 0; i < cdevhdr.free_num; i++)
            {
                off += CCKD_FREEBLK_SIZE;
                if (off > hipos - CCKD_FREEBLK_SIZE)
                    break;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cswp_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = read (fd, &freeblk, len)) != len)
                    goto cswp_read_error;
                cckd_swapend_free (&freeblk);
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cswp_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = write (fd, &freeblk, len)) != len)
                    goto cswp_write_error;
            } /* for each free space */
        } /* if new format free space */
        else
        {
            /* Old format free space */
            for (i = 0; i < cdevhdr.free_num; i++)
            {
                if (off < lopos || off > hipos - CCKD_FREEBLK_SIZE)
                    break;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cswp_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = read (fd, &freeblk, len)) != len)
                    goto cswp_read_error;
                cckd_swapend_free (&freeblk);
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cswp_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = write (fd, &freeblk, len)) != len)
                    goto cswp_write_error;
                if (!swapend) cckd_swapend_free (&freeblk);
                off = (off_t)freeblk.fb_offnxt;
            } /* for each free space */
        } /* else old format free space */
    } /* if free space */

    return 0;

    /* error exits */
cswp_fstat_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                "fstat()", strerror(errno)));
    else
        WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
              "fstat()", strerror(errno));
    goto cswp_error;

cswp_lseek_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "lseek()", off, strerror(errno)));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "lseek()", off, strerror(errno));
    goto cswp_error;

cswp_read_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "read()", off, rc < 0 ? strerror(errno) : "incomplete"));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "read()", off, rc < 0 ? strerror(errno) : "incomplete");
    goto cswp_error;

cswp_write_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "write()", off, rc < 0 ? strerror(errno) : "incomplete"));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "write()", off, rc < 0 ? strerror(errno) : "incomplete");
    goto cswp_error;

cswp_malloc_error:
    {
        char buf[64];
        MSGBUF( buf, "malloc(%d)", len);
        if(dev->batch)
            fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror(errno)));
        else
             WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                   buf, strerror(errno));
        goto cswp_error;
    }
cswp_error:
    if (l1) free(l1);
    return -1;
}

/*-------------------------------------------------------------------*/
/* Swap endian - compressed device header                            */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd_swapend_chdr( CCKD_DEVHDR* cdevhdr )
{
    /* fix the compressed ckd header */
    cdevhdr->opts ^= CCKD_BIGENDIAN;

    cdevhdr->num_L1tab    = SWAP32( cdevhdr->num_L1tab    ); // CCKD64FIXME!
    cdevhdr->num_L2tab    = SWAP32( cdevhdr->num_L2tab    ); // CCKD64FIXME!
    cdevhdr->cdh_size     = SWAP32( cdevhdr->cdh_size     ); // CCKD64FIXME!
    cdevhdr->cdh_used     = SWAP32( cdevhdr->cdh_used     ); // CCKD64FIXME!
    cdevhdr->free_off     = SWAP32( cdevhdr->free_off     ); // CCKD64FIXME!
    cdevhdr->free_total   = SWAP32( cdevhdr->free_total   ); // CCKD64FIXME!
    cdevhdr->free_largest = SWAP32( cdevhdr->free_largest ); // CCKD64FIXME!
    cdevhdr->free_num     = SWAP32( cdevhdr->free_num     ); // CCKD64FIXME!
    cdevhdr->free_imbed   = SWAP32( cdevhdr->free_imbed   ); // CCKD64FIXME!
    cdevhdr->cmp_parm     = SWAP16( cdevhdr->cmp_parm     ); // CCKD64FIXME!
}

/*-------------------------------------------------------------------*/
/* Swap endian - level 1 table                                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd_swapend_l1( CCKD_L1ENT* l1, int n )
{
    int  i;
    for (i = 0; i < n; i++)
        l1[i] = SWAP32( l1[i] ); // CCKD64FIXME!
}

/*-------------------------------------------------------------------*/
/* Swap endian - level 2 table                                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd_swapend_l2( CCKD_L2ENT* l2 )
{
    int  i;
    for (i = 0; i < 256; i++)
    {
        l2[i].L2_trkoff = SWAP32( l2[i].L2_trkoff ); // CCKD64FIXME!
        l2[i].L2_len    = SWAP16( l2[i].L2_len    ); // CCKD64FIXME!
        l2[i].L2_size   = SWAP16( l2[i].L2_size   ); // CCKD64FIXME!
    }
}

/*-------------------------------------------------------------------*/
/* Swap endian - free space entry                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd_swapend_free( CCKD_FREEBLK* fb )
{
    fb->fb_offnxt = SWAP32( fb->fb_offnxt ); // CCKD64FIXME!
    fb->fb_len    = SWAP32( fb->fb_len    ); // CCKD64FIXME!
}

/*-------------------------------------------------------------------*/
/* Are we little or big endian?  From Harbison&Steele.               */
/*-------------------------------------------------------------------*/
DLL_EXPORT int cckd_endian()
{
    union
    {
        long l;
        char c[ sizeof( long )];
    }
    u;

    u.l = 1;
    return u.c[ sizeof( long ) - 1 ] == 1 ? CCKD_BIGENDIAN : 0;
}

/*-------------------------------------------------------------------
 * Remove all free space from a compressed ckd file
 *-------------------------------------------------------------------*/
DLL_EXPORT int cckd_comp (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             fd;                     /* File descriptor           */
struct stat     fst;                    /* File status buffer        */
int             rc;                     /* Return code               */
off_t           off;                    /* File offset               */
off_t           l2area;                 /* Boundary for l2 tables    */
int             len;                    /* Length                    */
int             i, j, l, n;             /* Work variables            */
int             relocate = 0;           /* 1=spaces will be relocated*/
int             l1size;                 /* l1 table size             */
U32             next;                   /* offset of next space      */
int             s;                      /* space table index         */
CKD_DEVHDR      devhdr;                 /* CKD device header         */
CCKD_DEVHDR     cdevhdr;                /* CCKD device header        */
CCKD_L1ENT     *l1=NULL;                /* ->L1tab table               */
CCKD_L2ENT    **l2=NULL;                /* ->L2tab table array         */
SPCTAB         *spctab=NULL;            /* -> space table            */
BYTE           *rbuf=NULL;              /* Relocation buffer         */
BYTE           *p;                      /* -> relocation buffer      */
int             rlen=0;                 /* Relocation buffer length  */
CCKD_L2ENT      zero_l2[256];           /* Empty l2 table (zeros)    */
CCKD_L2ENT      ff_l2[256];             /* Empty l2 table (0xff's)   */
BYTE            buf[65536*4];           /* Buffer                    */

    /*---------------------------------------------------------------
     * Get fd
     *---------------------------------------------------------------*/
    cckd = dev->cckd_ext;
    if (cckd == NULL)
        fd = dev->fd;
    else
        fd = cckd->fd[cckd->sfn];

    /*---------------------------------------------------------------
     * Get file statistics
     *---------------------------------------------------------------*/
    if (fstat (fd, &fst) < 0)
        goto comp_fstat_error;
    gui_fprintf (stderr, "SIZE=%"PRIu64"\n", (U64) fst.st_size);

    /*---------------------------------------------------------------
     * Read device header
     *---------------------------------------------------------------*/
    off = 0;
    if (lseek( fd, off, SEEK_SET ) < 0)
        goto comp_lseek_error;
    gui_fprintf( stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CKD_DEVHDR_SIZE;
    if ((rc = read( fd, &devhdr, len )) != len)
        goto comp_read_error;

    if (!(devhdrid_typ( devhdr.devhdrid ) & ANY32_CMP_OR_SF_TYP))
    {
        // "%1d:%04X CCKD file %s: not a compressed dasd file"
        if (dev->batch)
            fprintf( stdout, MSG( HHC00356, "E", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, dev->filename ));
        else
            WRMSG( HHC00356, "E", LCSS_DEVNUM,
                dev->filename );
        goto comp_error;
    }

comp_restart:

    /*---------------------------------------------------------------
     * Read compressed device header
     *---------------------------------------------------------------*/
    off = CCKD_DEVHDR_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD_DEVHDR_SIZE;
    if ((rc = read (fd, &cdevhdr, len)) != len)
        goto comp_read_error;

    /*---------------------------------------------------------------
     * Check the endianess of the file
     *---------------------------------------------------------------*/
    if ((cdevhdr.opts & CCKD_BIGENDIAN) != cckd_endian())
    {
        if(dev->batch)
            fprintf(stdout, MSG(HHC00357, "I", LCSS_DEVNUM, dev->filename,
                    cckd_endian() ? "big-endian" : "little-endian"));
        else
            WRMSG(HHC00357, "I", LCSS_DEVNUM, dev->filename,
                  cckd_endian() ? "big-endian" : "little-endian");
        if (cckd_swapend (dev) < 0)
            goto comp_error;
        else
            goto comp_restart;
    }

    /*---------------------------------------------------------------
     * Some header checks
     *---------------------------------------------------------------*/
    if ((off_t)cdevhdr.cdh_size != fst.st_size
     || cdevhdr.cdh_size   != cdevhdr.cdh_used || cdevhdr.free_off     != 0
     || cdevhdr.free_total != 0                || cdevhdr.free_largest != 0
     || cdevhdr.free_num   != 0                || cdevhdr.free_imbed   != 0)
        relocate = 1;

    /*---------------------------------------------------------------
     * Build empty l2 tables
     *---------------------------------------------------------------*/
    memset( &zero_l2, 0, CCKD_L2TAB_SIZE );
    if (cdevhdr.nullfmt != 0)
        for (i = 0; i < 256; i++)
            zero_l2[i].L2_len = zero_l2[i].L2_size = cdevhdr.nullfmt;
    memset (&ff_l2, 0xff, CCKD_L2TAB_SIZE);

    /*---------------------------------------------------------------
     * Read the l1 table
     *---------------------------------------------------------------*/
    l1size = len = cdevhdr.num_L1tab * CCKD_L1ENT_SIZE;
    if ((l1 = malloc (len)) == NULL)
        goto comp_malloc_error;
    off = CCKD_L1TAB_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = read (fd, l1, len)) != len)
        goto comp_read_error;

    /*---------------------------------------------------------------
     * Build the space table
     *---------------------------------------------------------------*/
    n = 1 + 1 + 1 + cdevhdr.num_L1tab + 1;
    for (i = 0; i < cdevhdr.num_L1tab; i++)
        if (l1[i] != 0 && l1[i] != 0xffffffff)
            n += 256;
    len = sizeof(SPCTAB);
    if ((spctab = calloc (n, len)) == NULL)
        goto comp_calloc_error;
    s = 0;
    spctab[s].spc_typ = SPCTAB_DEVHDR;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = 0;
    spctab[s].spc_len =
    spctab[s].spc_siz = CKD_DEVHDR_SIZE;
    s++;
    spctab[s].spc_typ = SPCTAB_CDEVHDR;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = CCKD_DEVHDR_POS;
    spctab[s].spc_len =
    spctab[s].spc_siz = CCKD_DEVHDR_SIZE;
    s++;
    spctab[s].spc_typ = SPCTAB_L1;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = CCKD_L1TAB_POS;
    spctab[s].spc_len =
    spctab[s].spc_siz = l1size;
    s++;
    spctab[s].spc_typ = SPCTAB_EOF;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = fst.st_size;
    spctab[s].spc_len =
    spctab[s].spc_siz = 0;
    s++;

    for (i = 0; i < cdevhdr.num_L1tab; i++)
        if (l1[i] != 0 && l1[i] != 0xffffffff)
        {
            spctab[s].spc_typ = SPCTAB_L2;
            spctab[s].spc_val = i;
            spctab[s].spc_off = l1[i];
            spctab[s].spc_len =
            spctab[s].spc_siz = CCKD_L2TAB_SIZE;
            s++;
        }
    qsort (spctab, s, sizeof(SPCTAB), comp_spctab_sort);

    /*---------------------------------------------------------------
     * Read level 2 tables
     *---------------------------------------------------------------*/
    n = cdevhdr.num_L1tab;
    len = sizeof (void *);
    if ((l2 = calloc (n, len)) == NULL)
        goto comp_calloc_error;
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
    {
        if (spctab[i].spc_typ != SPCTAB_L2) continue;
        l = spctab[i].spc_val;
        len = CCKD_L2TAB_SIZE;
        if ((l2[l] = malloc (len)) == NULL)
            goto comp_malloc_error;
        off = (off_t)spctab[i].spc_off;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto comp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        if ((rc = read (fd, l2[l], len)) != len)
            goto comp_read_error;
        for (j = 0; j < 256; j++)
        {
            if (l2[l][j].L2_trkoff == 0 || l2[l][j].L2_trkoff == 0xffffffff)
                continue;
            spctab[s].spc_typ = SPCTAB_TRK;
            spctab[s].spc_val = spctab[i].spc_val*256 + j;
            spctab[s].spc_off = l2[l][j].L2_trkoff;
            spctab[s].spc_len = l2[l][j].L2_len;
            spctab[s].spc_siz = l2[l][j].L2_size;
            s++;
        } /* for each l2 entry */
        /* check if empty l2 table */
        if (memcmp (l2[l], &zero_l2, CCKD_L2TAB_SIZE) == 0
         || memcmp (l2[l], &ff_l2,   CCKD_L2TAB_SIZE) == 0)
        {
            l1[l] = l2[l][0].L2_trkoff; /* 0x00000000 or 0xffffffff */
            spctab[i].spc_typ = SPCTAB_NONE;
            free (l2[l]);
            l2[l] = NULL;
            relocate = 1;
        }
    } /* for each space */
    qsort (spctab, s, sizeof(SPCTAB), comp_spctab_sort);
    while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;
    /* set relocate flag if last space is free space */
    if (spctab[s-2].spc_off + spctab[s-2].spc_len != spctab[s-1].spc_off)
        relocate = 1;

    /*---------------------------------------------------------------
     * relocate l2 tables in order
     *---------------------------------------------------------------*/

    /* determine l2 area */
    l2area = CCKD_L1TAB_POS + l1size;
    for (i = 0; i < cdevhdr.num_L1tab; i++)
    {
        if (l1[i] == 0 || l1[i] == 0xffffffff) continue;
        if (l1[i] != l2area)
            relocate = 1;
        l2area += CCKD_L2TAB_SIZE;
    }

    /* quick return if all l2 tables are orderered and no free space */
    if (!relocate)
    {
        for (i = 1; spctab[i].spc_typ != SPCTAB_EOF; i++)
            if (spctab[i-1].spc_off + spctab[i-1].spc_len != spctab[i].spc_off)
                break;
        if (spctab[i].spc_typ == SPCTAB_EOF)
        {
            if(dev->batch)
                fprintf(stdout, MSG(HHC00358, "I", LCSS_DEVNUM, dev->filename));
            else
                WRMSG(HHC00358, "I", LCSS_DEVNUM, dev->filename);
            goto comp_return_ok;
        }
    }

    /* file will be updated */
    cdevhdr.opts |= CCKD_ORDWR;

    /* calculate track size within the l2 area */
    for (i = rlen = 0; spctab[i].spc_off < l2area; i++)
        if (spctab[i].spc_typ == SPCTAB_TRK)
            rlen += sizeof(spctab[i].spc_val) + sizeof(spctab[i].spc_len)
                 +  spctab[i].spc_len;

    /* read any tracks in the l2area into rbuf */
    if ((len = rlen) > 0)
    {
        if ((rbuf = malloc (len)) == NULL)
            goto comp_malloc_error;
        for (i = 0, p = rbuf; spctab[i].spc_off < l2area; i++)
        {
            if (spctab[i].spc_typ != SPCTAB_TRK) continue;
            memcpy (p, &spctab[i].spc_val, sizeof(spctab[i].spc_val));
            p += sizeof(spctab[i].spc_val);
            memcpy (p, &spctab[i].spc_len, sizeof(spctab[i].spc_len));
            p += sizeof(spctab[i].spc_len);
            off = (off_t)spctab[i].spc_off;
            if (lseek (fd, off, SEEK_SET) < 0)
                goto comp_lseek_error;
            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
            len = spctab[i].spc_len;
            if ((rc = read (fd, p, len)) != len)
                goto comp_read_error;
            p += len;
            spctab[i].spc_typ = SPCTAB_NONE;
        } /* for each space in the l2 area */
        qsort (spctab, s, sizeof(SPCTAB), comp_spctab_sort);
        while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;
    } /* if any tracks to relocate */

    /* remove all l2 tables from the space table */
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        if (spctab[i].spc_typ == SPCTAB_L2)
            spctab[i].spc_typ = SPCTAB_NONE;
    qsort (spctab, s, sizeof(SPCTAB), comp_spctab_sort);
    while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;

    /* add all l2 tables at their ordered offsets */
    off = CCKD_L1TAB_POS + l1size;
    for (i = 0; i < cdevhdr.num_L1tab; i++)
    {
        if (l1[i] == 0 || l1[i] == 0xffffffff) continue;
        spctab[s].spc_typ = SPCTAB_L2;
        spctab[s].spc_val = i;
        spctab[s].spc_off = (U32)off;
        spctab[s].spc_len =
        spctab[s].spc_siz = CCKD_L2TAB_SIZE;
        s++;
        off += CCKD_L2TAB_SIZE;
    }
    qsort (spctab, s, sizeof(SPCTAB), comp_spctab_sort);
    /* set end-of-file position */
    spctab[s-1].spc_off = spctab[s-2].spc_off + spctab[s-2].spc_len;

    /*---------------------------------------------------------------
     * Perform compression
     *---------------------------------------------------------------*/

    /* move spaces left */
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
    {
        /* ignore contiguous spaces */
        if (spctab[i].spc_off + spctab[i].spc_len == spctab[i+1].spc_off)
            continue;

        /* found a gap */
        off = (off_t)spctab[i+1].spc_off;

        /* figure out how much we can read */
        for (len = 0, j = i + 1; spctab[j].spc_typ != SPCTAB_EOF; j++)
        {
            if (len + spctab[j].spc_len > sizeof(buf))
                break;
            next = spctab[j].spc_off + spctab[j].spc_len;
            spctab[j].spc_off = spctab[i].spc_off + spctab[i].spc_len + len;
            spctab[j].spc_siz = spctab[j].spc_len;
            len += spctab[j].spc_len;
            if (next != spctab[j+1].spc_off)
                break;
        } /* search for contiguous spaces */

        /* this can happen if the next space is end-of-file */
        if (len == 0)
            continue;

        /* read the image(s) to be relocated */
        if (lseek (fd, off, SEEK_SET) < 0)
            goto comp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        if ((rc = read (fd, buf, len)) != len)
            goto comp_write_error;

        /* write the images */
        off = (off_t)spctab[i].spc_off + spctab[i].spc_len;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto comp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        if ((rc = write (fd, buf, len)) != len)
            goto comp_write_error;
    }

    /* adjust the size of the file */
    spctab[s-1].spc_off = spctab[s-2].spc_off + spctab[s-2].spc_len;

    /*---------------------------------------------------------------
     * Write spaces relocated from the l2area to the end of the file
     *---------------------------------------------------------------*/
    off = (off_t)spctab[s-1].spc_off;
    p = rbuf;
    while (rlen)
    {
        spctab[s].spc_typ = SPCTAB_TRK;
        spctab[s].spc_off = (U32)off;
        memcpy (&spctab[s].spc_val, p, sizeof(spctab[s].spc_val));
        p += sizeof(spctab[s].spc_val);
        memcpy (&spctab[s].spc_len, p, sizeof(spctab[s].spc_len));
        spctab[s].spc_siz = spctab[s].spc_len;
        p += sizeof(spctab[s].spc_len);

        if (lseek (fd, off, SEEK_SET) < 0)
            goto comp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = spctab[s].spc_len;
        if ((rc = write (fd, p, len)) != len)
            goto comp_write_error;

        p += len;
        off += len;
        rlen -= len + sizeof(spctab[s].spc_val) + sizeof(spctab[s].spc_len);
        s++;
    } /* for each relocated space in l2area */

    /* adjust the space table */
    if (rbuf)
    {
        free (rbuf); rbuf = NULL;
        qsort (spctab, s, sizeof(SPCTAB), comp_spctab_sort);
        spctab[s-1].spc_off = spctab[s-2].spc_off + spctab[s-2].spc_len;
    }

    /*---------------------------------------------------------------
     * Update the device header
     *---------------------------------------------------------------*/
    cdevhdr.cdh_size =
    cdevhdr.cdh_used = spctab[s-1].spc_off;
    cdevhdr.free_off =
    cdevhdr.free_total =
    cdevhdr.free_largest =
    cdevhdr.free_num =
    cdevhdr.free_imbed = 0;
    cdevhdr.vrm[0] = CCKD_VERSION;
    cdevhdr.vrm[1] = CCKD_RELEASE;
    cdevhdr.vrm[2] = CCKD_MODLVL;

    /*---------------------------------------------------------------
     * Update the lookup tables
     *---------------------------------------------------------------*/
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        if (spctab[i].spc_typ == SPCTAB_L2)
            l1[spctab[i].spc_val] = spctab[i].spc_off;
        else if (spctab[i].spc_typ == SPCTAB_TRK)
        {
            l = spctab[i].spc_val / 256;
            j = spctab[i].spc_val % 256;
            l2[l][j].L2_trkoff  = spctab[i].spc_off;
            l2[l][j].L2_len  =
            l2[l][j].L2_size = spctab[i].spc_len;
        }

    /*---------------------------------------------------------------
     * Write the cdevhdr, l1 table and l2 tables
     *---------------------------------------------------------------*/

    /* write cdevhdr */
    off = CCKD_DEVHDR_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD_DEVHDR_SIZE;
    if ((rc = write (fd, &cdevhdr, len)) != len)
        goto comp_write_error;

    /* write l1 table */
    off = CCKD_L1TAB_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = l1size;
    if ((rc = write (fd, l1, len)) != len)
        goto comp_write_error;

    /* write l2 tables */
    for (i = 0; i < cdevhdr.num_L1tab; i++)
        if (l1[i] != 0 && l1[i] != 0xffffffff)
        {
            off = (off_t)l1[i];
            if (lseek (fd, off, SEEK_SET) < 0)
                goto comp_lseek_error;
            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
            len = CCKD_L2TAB_SIZE;
            if ((rc = write (fd, l2[i], len)) != len)
                goto comp_lseek_error;
        }

    /* truncate the file */
    off = (off_t)spctab[s-1].spc_off;
    if (off < fst.st_size)
    {
        VERIFY(!ftruncate (fd, off));
        if(dev->batch)
            fprintf(stdout, MSG(HHC00359, "I", LCSS_DEVNUM, dev->filename,
                    fst.st_size - off));
        else
            WRMSG(HHC00359, "I", LCSS_DEVNUM, dev->filename, fst.st_size - off);
    }
    else
    {
        if(dev->batch)
            fprintf(stdout, MSG(HHC00360, "I", LCSS_DEVNUM, dev->filename));
        else
            WRMSG(HHC00360, "I", LCSS_DEVNUM, dev->filename);
    }

    /*---------------------------------------------------------------
     * Return
     *---------------------------------------------------------------*/

comp_return_ok:

    rc = 0;

comp_return:

    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));

    if (rbuf) free(rbuf);
    if (l2)
    {
        for (i = 0; i < cdevhdr.num_L1tab; i++)
            if (l2[i])
                free (l2[i]);
        free (l2);
    }
    if (l1) free (l1);
    if (spctab) free (spctab);

    return rc;

    /*---------------------------------------------------------------
     * Error exits
     *---------------------------------------------------------------*/

comp_fstat_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                "fstat()", strerror(errno)));
    else
        WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
              "fstat()", strerror(errno));
    goto comp_error;

comp_lseek_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "lseek()", off, strerror(errno)));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "lseek()", off, strerror(errno));
    goto comp_error;

comp_read_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "read()", off, rc < 0 ? strerror(errno) : "incomplete"));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "read()", off, rc < 0 ? strerror(errno) : "incomplete");
    goto comp_error;

comp_write_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "write()", off, rc < 0 ? strerror(errno) : "incomplete"));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "write()", off, rc < 0 ? strerror(errno) : "incomplete");
    goto comp_error;

comp_malloc_error:
    {
        char buf[64];
        MSGBUF( buf, "malloc(%d)", len);
        if(dev->batch)
            fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror(errno)));
        else
            WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                  buf, strerror(errno));
        goto comp_error;
    }
comp_calloc_error:
    {
        char buf[64];
        MSGBUF( buf, "calloc(%d)", n * len);
        if(dev->batch)
            fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror(errno)));
        else
            WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                  buf, strerror(errno));
        goto comp_error;
    }
comp_error:

    rc = -1;
    goto comp_return;

} /* cckd_comp() */

/*-------------------------------------------------------------------
 * cckd_comp() space table sort
 *-------------------------------------------------------------------*/
static int comp_spctab_sort(const void *a, const void *b)
{
const SPCTAB *x = a, *y = b;

         if (x->spc_typ == SPCTAB_NONE) return +1;
    else if (y->spc_typ == SPCTAB_NONE) return -1;
    else if (x->spc_typ == SPCTAB_EOF)  return +1;
    else if (y->spc_typ == SPCTAB_EOF)  return -1;
    else if (x->spc_off < y->spc_off)   return -1;
    else                                return +1;
}

/*-------------------------------------------------------------------
 * Perform check function on a compressed ckd file
 *
 * check levels
 *    -1 devhdr, cdevhdr, l1 table
 *     0 devhdr, cdevhdr, l1 table, l2 tables
 *     1 devhdr, cdevhdr, l1 table, l2 tables, free spaces
 *     2 devhdr, cdevhdr, l1 table, l2 tables, free spaces, trkhdrs
 *     3 devhdr, cdevhdr, l1 table, l2 tables, free spaces, trkimgs
 *     4 devhdr, cdevhdr. Build everything else from recovery
 *-------------------------------------------------------------------*/
DLL_EXPORT int cckd_chkdsk(DEVBLK *dev, int level)
{
CCKD_EXT       *cckd;                   /* -> ckd extension          */
int             fd;                     /* file descriptor           */
struct stat     fst;                    /* file status information   */
int             fdflags;                /* file descriptor flags     */
S64             maxsize;                /* max cckd file size        */
int             ro;                     /* 1=file opened read-only   */
int             f, i, j, l, n;          /* work integers             */
int             L1idx, l2x;             /* l1, l2 table indexes      */
U32             imgtyp;                 /* Dasd image type           */
BYTE            compmask[256];          /* compression byte mask
                                           00 - supported
                                           0x - valid, not supported
                                           ff - invalid              */
off_t           off;                    /* file offset               */
int             len;                    /* length to read            */
int             rc;                     /* function return code      */
int             comp;                   /* trkhdr compression byte[0]*/
int             cyl;                    /* trkhdr cyl      bytes[1-2]*/
int             head;                   /* trkhdr head     bytes[3-4]*/
int             trk;                    /* trkhdr calculated trk     */
int             cyls;                   /* number cylinders          */
int             heads;                  /* number heads/cylinder     */
int             trks;                   /* number tracks             */
unsigned int    trksz;                  /* track size                */
int             blks;                   /* number fba blocks         */
int             blkgrp;                 /* current block group nbr   */
int             blkgrps;                /* number fba block groups   */
unsigned int    blkgrpsz;               /* fba block group size      */
int             trktyp;                 /* track type (TRK, BLKGRP)  */
int             ckddasd=0;              /* 1=ckd                     */
int             fbadasd=0;              /* 1= fba                    */
int             shadow=0;               /* 0xff=shadow file          */
int             hdrerr=0;               /* non-zero: header errors   */
int             fsperr=0;               /* 1=rebuild free space      */
int             comperrs=0;             /* 1=unsupported comp found  */
int             recovery=0;             /* 1=perform track recovery  */
int             valid;                  /* 1=valid trk recovered     */
int             l1size;                 /* size of l1 table          */
int             swapend=0;              /* 1=call cckd_swapend       */
U32             lopos, hipos;           /* low/high file positions   */
int             pass;                   /* recovery pass number (fba)*/
int             s;                      /* space table index         */
SPCTAB         *spctab=NULL;            /* -> space table            */
BYTE           *l2errs=NULL;            /* l2 error table            */
BYTE           *rcvtab=NULL;            /* recovered tracks          */
CKD_DEVHDR      devhdr;                 /* device header             */
CCKD_DEVHDR     cdevhdr;                /* compressed device header  */
CCKD_DEVHDR     cdevhdr2;               /* compressed device header 2*/
CCKD_L1ENT     *l1=NULL;                /* -> level 1 table          */
CCKD_L2ENT      l2ent;                  /* level 2 entry             */
CCKD_L2ENT      l2tab[256];             /* level 2 table             */
CCKD_L2ENT    **l2=NULL;                /* -> level 2 table array    */
CCKD_L2ENT      empty_l2[256];          /* Empty l2 table            */
CCKD_FREEBLK    freeblk;                /* free block                */
CCKD_FREEBLK   *fsp=NULL;               /* free blocks (new format)  */
BYTE            buf[4*65536];           /* buffer                    */

    /* Get fd */
    cckd = dev->cckd_ext;
    if (cckd == NULL)
        fd = dev->fd;
    else
        fd = cckd->fd[cckd->sfn];

    /* Get some file information */
    if ( fstat (fd, &fst) < 0 )
        goto cdsk_fstat_error;
    gui_fprintf (stderr, "SIZE=%"PRIu64"\n", (U64) fst.st_size);
    hipos = fst.st_size;
    maxsize = sizeof(off_t) == 4 ? 0x7fffffffll : 0xffffffffll;
    fdflags = get_file_accmode_flags(fd);
    ro = (fdflags & O_RDWR) == 0;

    /* Build table for compression byte test */
    memset (compmask, 0xff, 256);
    compmask[0] = 0;
#if defined( HAVE_ZLIB )
    compmask[CCKD_COMPRESS_ZLIB] = 0;
#else
    compmask[CCKD_COMPRESS_ZLIB] = 1;
#endif
#if defined( CCKD_BZIP2 )
    compmask[CCKD_COMPRESS_BZIP2] = 0;
#else
    compmask[CCKD_COMPRESS_BZIP2] = 2;
#endif

    /*---------------------------------------------------------------
     * Header checks
     *---------------------------------------------------------------*/

    /* Read the device header */
    off = 0;
    if ( lseek (fd, off, SEEK_SET) < 0)
        goto cdsk_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CKD_DEVHDR_SIZE;
    if ((rc = read (fd, &devhdr, len)) != len)
        goto cdsk_read_error;

    /* Device header checks */
    imgtyp = devhdrid_typ( devhdr.devhdrid );

         if (imgtyp & CKD32_CMP_OR_SF_TYP) ckddasd = 1;
    else if (imgtyp & FBA32_CMP_OR_SF_TYP) fbadasd = 1;
    else
    {
        // "%1d:%04X CCKD file %s: not a compressed dasd file"
        if (dev->batch)
            fprintf( stdout, MSG( HHC00356, "E", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, dev->filename ));
        else
            WRMSG( HHC00356, "E", LCSS_DEVNUM,
                dev->filename );
        goto cdsk_error;
    }

    if (imgtyp & ANY32_SF_TYP)
        shadow = 0xff;

    trktyp = ckddasd ? SPCTAB_TRK : SPCTAB_BLKGRP;

    /* Read the cckd device header */
    off = CCKD_DEVHDR_POS;
    if ( lseek (fd, off, SEEK_SET) < 0)
        goto cdsk_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD_DEVHDR_SIZE;
    if ((rc = read (fd, &cdevhdr, len)) != len)
        goto cdsk_read_error;

    /* Endianess check */
    if ((cdevhdr.opts & CCKD_BIGENDIAN) != cckd_endian())
    {
        if (!ro)
        {
            if(dev->batch)
                fprintf(stdout, MSG(HHC00357, "I", LCSS_DEVNUM, dev->filename,
                        cckd_endian() ? "big-endian" : "little-endian"));
            else
                WRMSG(HHC00357, "I", LCSS_DEVNUM, dev->filename,
                      cckd_endian() ? "big-endian" : "little-endian");
            if (cckd_swapend (dev) < 0)
                goto cdsk_error;
            if (level < 0) level = 0;
            swapend = 0;
        }
        else
            swapend = 1;
        cckd_swapend_chdr (&cdevhdr);
    }

    /* ckd checks */
    if (ckddasd)
    {
        CKDDEV *ckd;

        heads = (devhdr.heads[3] << 24)
              + (devhdr.heads[2] << 16)
              + (devhdr.heads[1] <<  8)
              + (devhdr.heads[0]);
        cyls  = (cdevhdr.cyls[3] << 24)
              + (cdevhdr.cyls[2] << 16)
              + (cdevhdr.cyls[1] <<  8)
              + (cdevhdr.cyls[0]);
        trks  = heads * cyls;
        trksz = (devhdr.trksize[3] << 24)
              + (devhdr.trksize[2] << 16)
              + (devhdr.trksize[1] <<  8)
              + (devhdr.trksize[0]);

        /* ckd dasd lookup */
        ckd = dasd_lookup (DASD_CKDDEV, NULL, devhdr.dvtyp, 0);
        if (ckd == NULL)
        {
            if(dev->batch)
                fprintf(stdout, MSG(HHC00361, "E", LCSS_DEVNUM, dev->filename,
                        devhdr.dvtyp, cyls));
            else
                WRMSG(HHC00361, "E", LCSS_DEVNUM, dev->filename,
                      devhdr.dvtyp, cyls);
             goto cdsk_error;
        }

        /* track size check */
        n = sizeof(CKD_TRKHDR)
          + sizeof(CKD_RECHDR) + 8        /* r0 length */
          + sizeof(CKD_RECHDR) + ckd->r1  /* max data length */
          + sizeof(eighthexFF);
        n = ((n+511)/512)*512;
        if ((int)trksz != n)
        {
            if(dev->batch)
                fprintf(stdout, MSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                        "track size", trksz, n));
            else
                WRMSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                      "track size", trksz, n);
             goto cdsk_error;
        }

        /* number of heads check */
        if (heads != ckd->heads)
        {
            if(dev->batch)
                fprintf(stdout, MSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                        "number of heads", heads, ckd->heads));
            else
                WRMSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                      "number of heads", heads, ckd->heads);
             goto cdsk_error;
        }
    } /* if (ckddasd) */

    /* fba checks */
    else
    {
        /* Note: cyls & heads are setup for ckd type hdr checks */
        blks = (cdevhdr.cyls[3] << 24)
             + (cdevhdr.cyls[2] << 16)
             + (cdevhdr.cyls[1] <<  8)
             + (cdevhdr.cyls[0]);
        trks = blks / CFBA_BLKGRP_BLKS;
        if (blks % CFBA_BLKGRP_BLKS) trks++;
        trksz = CFBA_BLKGRP_SIZE + CKD_TRKHDR_SIZE;
        heads = 65536;
        cyls = trks / heads;
        if (trks % heads) cyls++;
    }

    /* fba variables */
    blkgrps = trks;
    blkgrpsz = trksz;

    /* `num_L1tab' check */
    n = trks / 256;
    if (trks % 256) n++;

    if (cdevhdr.num_L1tab != n && cdevhdr.num_L1tab != n + 1)
    {
        if(dev->batch)
            fprintf(stdout, MSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                    "num_L1tab", cdevhdr.num_L1tab, n));
        else
            WRMSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                  "num_L1tab", cdevhdr.num_L1tab, n);
        goto cdsk_error;
    }
    l1size = cdevhdr.num_L1tab * CCKD_L1ENT_SIZE;
    if (CCKD_L1TAB_POS + l1size > fst.st_size)
    {
        if(dev->batch)
            fprintf(stdout, MSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                    "file length to contain L1 table", (int)fst.st_size, (int)CCKD_L1TAB_POS + l1size));
        else
            WRMSG(HHC00362, "E", LCSS_DEVNUM, dev->filename,
                  "file length to contain L1 table", (int)fst.st_size, (int)CCKD_L1TAB_POS + l1size);
        goto cdsk_error;
    }

    /* check level 2 if SPERRS bit on */
    if (!ro && level < 2 && (cdevhdr.opts & CCKD_SPERRS))
    {
        level = 2;
        if(dev->batch)
            fprintf(stdout, MSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level));
        else
            WRMSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level);
    }

    /* cdevhdr inconsistencies check */
    hdrerr  = 0;
    hdrerr |= fst.st_size != (off_t)cdevhdr.cdh_size && cdevhdr.cdh_size != cdevhdr.free_off ? 0x0001 : 0;
    hdrerr |= cdevhdr.cdh_size !=      cdevhdr.free_total  +  cdevhdr.cdh_used               ? 0x0002 : 0;
    hdrerr |= cdevhdr.free_largest  >  cdevhdr.free_total  -  cdevhdr.free_imbed             ? 0x0004 : 0;
    hdrerr |= cdevhdr.free_off == 0 && cdevhdr.free_num    != 0                              ? 0x0008 : 0;
    hdrerr |= cdevhdr.free_off == 0 && cdevhdr.free_total  != cdevhdr.free_imbed             ? 0x0010 : 0;
    hdrerr |= cdevhdr.free_off != 0 && cdevhdr.free_total  == 0                              ? 0x0020 : 0;
    hdrerr |= cdevhdr.free_off != 0 && cdevhdr.free_num    == 0                              ? 0x0040 : 0;
    hdrerr |= cdevhdr.free_num == 0 && cdevhdr.free_total  != cdevhdr.free_imbed             ? 0x0080 : 0;
    hdrerr |= cdevhdr.free_num != 0 && cdevhdr.free_total  <= cdevhdr.free_imbed             ? 0x0100 : 0;
    hdrerr |= cdevhdr.free_imbed    >  cdevhdr.free_total                                    ? 0x0200 : 0;

    /* Additional checking if header errors */
    if (hdrerr != 0)
    {
        if(dev->batch)
            fprintf(stdout, MSG(HHC00363, "W", LCSS_DEVNUM, dev->filename, hdrerr));
        else
            WRMSG(HHC00363, "W", LCSS_DEVNUM, dev->filename, hdrerr);
        if (level < 1)
        {
            level = 1;
            if(dev->batch)
                fprintf(stdout, MSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level));
            else
                WRMSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level);
        }
    }

    /* Additional checking if not properly closed */
    if (level < 1 && (cdevhdr.opts & CCKD_OPENED))
    {
        level = 1;
        if(dev->batch)
            fprintf(stdout, MSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level));
        else
            WRMSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level);
    }

    /* Additional checking if last opened for read/write */
    if (level < 0 && (cdevhdr.opts & CCKD_ORDWR))
        level = 0;

    /* Set check level -1 */
    if (level == 0 && !dev->batch && !hdrerr
     && (cdevhdr.opts & (CCKD_OPENED|CCKD_SPERRS)) == 0
     && ((cdevhdr.opts & (CCKD_ORDWR)) == 0 || ro))
        level = -1;

    /* Build empty l2 table */
    memset (&empty_l2, shadow, CCKD_L2TAB_SIZE);
    if (shadow == 0 && cdevhdr.nullfmt != 0)
        for (i = 0; i < 256; i++)
            empty_l2[i].L2_size = empty_l2[i].L2_size = cdevhdr.nullfmt;

    /*---------------------------------------------------------------
     * read the level 1 table
     *---------------------------------------------------------------*/
    len = l1size;
    if ((l1 = malloc (len)) == NULL)
        goto cdsk_error;
    off = CCKD_L1TAB_POS;
    if ( lseek (fd, off, SEEK_SET) < 0)
        goto cdsk_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = read (fd, l1, len)) != len)
        goto cdsk_read_error;
    if (swapend) cckd_swapend_l1 (l1, (int)cdevhdr.num_L1tab);
    lopos = CCKD_L1TAB_POS + l1size;

    /*---------------------------------------------------------------
     * initialize the space table
     *---------------------------------------------------------------*/

    /* find number of non-null l1 entries */
    for (i = n = 0; i < cdevhdr.num_L1tab; i++)
        if (l1[i] != 0 && l1[i] != 0xffffffff)
            n++;

    if (level >= 4) n = cdevhdr.num_L1tab;

    /* calculate max possible space table entries */
    n = 1 + 1 + 1                    // devhdr, cdevhdr, l1tab
      + n                            // l2tabs
      + (n * 256)                    // trk/blk images
      + (1 + n + (n * 256) + 1)      // max possible free spaces
      + 1;                           // end-of-file

    /* obtain the space table */
    len = sizeof(SPCTAB);
    if ((spctab = calloc (n, len)) == NULL)
        goto cdsk_calloc_error;

    /* populate the table with what we have */
    s = 0;

    /* devhdr */
    spctab[s].spc_typ = SPCTAB_DEVHDR;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = 0;
    spctab[s].spc_len =
    spctab[s].spc_siz = CKD_DEVHDR_SIZE;
    s++;
    /* cdevhdr */
    spctab[s].spc_typ = SPCTAB_CDEVHDR;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = CCKD_DEVHDR_POS;
    spctab[s].spc_len =
    spctab[s].spc_siz = CCKD_DEVHDR_SIZE;
    s++;
    /* l1 table */
    spctab[s].spc_typ = SPCTAB_L1;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = CCKD_L1TAB_POS;
    spctab[s].spc_len =
    spctab[s].spc_siz = l1size;
    s++;
    /* l2 tables */
    for (i = 0; i < cdevhdr.num_L1tab && level < 4; i++)
    {
        if (l1[i] == 0 || l1[i] == 0xffffffff) continue;
        spctab[s].spc_typ = SPCTAB_L2;
        spctab[s].spc_val = i;
        spctab[s].spc_off = l1[i];
        spctab[s].spc_len =
        spctab[s].spc_siz = CCKD_L2TAB_SIZE;
        s++;
    }
    /* end-of-file */
    spctab[s].spc_typ = SPCTAB_EOF;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = (U32)fst.st_size;
    spctab[s].spc_len =
    spctab[s].spc_siz = 0;
    s++;

    qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);

    /*---------------------------------------------------------------
     * Quick return if level -1
     *---------------------------------------------------------------*/

    if (level < 0)
    {
        int err = 0;
        /* check for overlaps */
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
            if (spctab[i].spc_off + spctab[i].spc_siz > spctab[i+1].spc_off)
                err = 1;
        /* exit if no errors */
        if (!err) goto cdsk_return_ok;
    }

    /*---------------------------------------------------------------
     * obtain the l2errs table and recovery table
     *---------------------------------------------------------------*/

    len = sizeof(BYTE);

    n = cdevhdr.num_L1tab;
    if ((l2errs = calloc (n, len)) == NULL)
        goto cdsk_calloc_error;

    n = trks;
    if ((rcvtab = calloc (n, len)) == NULL)
        goto cdsk_calloc_error;

    /*---------------------------------------------------------------
     * Special processing for level 4 (recover everything)
     *---------------------------------------------------------------*/

    if (level == 4)
    {
        memset (l2errs, 1, cdevhdr.num_L1tab);
        memset (rcvtab, 1, trks);
        goto cdsk_recovery;
    }

    /*---------------------------------------------------------------
     * Read the level 2 tables
     *---------------------------------------------------------------*/

    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
    {
        if (spctab[i].spc_typ != SPCTAB_L2
         || spctab[i].spc_off < lopos || spctab[i].spc_off > hipos)
            continue;

        off = spctab[i].spc_off;
        if ( lseek (fd, off, SEEK_SET) < 0 )
            goto cdsk_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = CCKD_L2TAB_SIZE;
        if ((rc = read (fd, l2tab, len)) != len)
            goto cdsk_read_error;
        if (swapend) cckd_swapend_l2 (l2tab);

        /* add trks/blkgrps to the space table */
        for (j = 0; j < 256; j++)
        {
            if (l2tab[j].L2_trkoff != 0 && l2tab[j].L2_trkoff != 0xffffffff)
            {
                spctab[s].spc_typ = trktyp;
                spctab[s].spc_val = spctab[i].spc_val * 256 + j;
                spctab[s].spc_off = l2tab[j].L2_trkoff;
                spctab[s].spc_len = l2tab[j].L2_len;
                spctab[s].spc_siz = l2tab[j].L2_size;
                s++;
            }
        }
    }
    qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);

    /*---------------------------------------------------------------
     * Consistency checks.
     *
     * The space table is now populated with everything but free
     * space.  Therefore we can infer what the free space should
     * be (ie gaps between allocated spaces).
     *---------------------------------------------------------------*/

    lopos = CCKD_L1TAB_POS + l1size;
    hipos = fst.st_size;

    /* Make adjustment if new format free space is at the end */
    len = spctab[s-1].spc_off - (spctab[s-2].spc_off + spctab[s-2].spc_siz);
    if (len > 0
     && cdevhdr.cdh_size == cdevhdr.free_off
     && cdevhdr.cdh_size + len == spctab[s-1].spc_off)
    {
        spctab[s-1].spc_off -= len;
        hipos -= len;
    }

    memset( &cdevhdr2, 0, CCKD_DEVHDR_SIZE );
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
    {
        /* Calculate gap size */
        len = spctab[i+1].spc_off - (spctab[i].spc_off + spctab[i].spc_siz);

        /* Update space statistics */
        cdevhdr2.cdh_size += spctab[i].spc_siz + len;
        cdevhdr2.cdh_used += spctab[i].spc_len;
        if (len > 0)
        {
            cdevhdr2.free_num++;
            cdevhdr2.free_total += len;
            if (cdevhdr2.free_largest < (U32)len)
                cdevhdr2.free_largest = (U32)len;
        }
        if (spctab[i].spc_typ == trktyp)
        {
            cdevhdr2.free_total += spctab[i].spc_siz - spctab[i].spc_len;
            cdevhdr2.free_imbed += spctab[i].spc_siz - spctab[i].spc_len;
        }

        /* ignore devhdr, cdevhdr and l1 (these are `out of bounds') */
        if (spctab[i].spc_typ == SPCTAB_DEVHDR
         || spctab[i].spc_typ == SPCTAB_CDEVHDR
         || spctab[i].spc_typ == SPCTAB_L1
           )
            continue;

        /* check if the space is out of bounds */
        valid = (off_t)spctab[i].spc_off >= lopos
             && (off_t)spctab[i].spc_off + spctab[i].spc_siz <= hipos;

        /* Overlap check */
        if (len < 0 || !valid)
        {
            char space1[32], space2[32];
            recovery = 1;

            /* issue error message */
            j = MSGBUF(space1, "%s", spaces[spctab[i].spc_typ]);
            if (spctab[i].spc_val >= 0)
                sprintf(space1+j, "[%d]", spctab[i].spc_val);
            j = MSGBUF(space2, "%s", spaces[spctab[i+1].spc_typ]);
            if (spctab[i+1].spc_val >= 0)
                sprintf(space2+j, "[%d]", spctab[i+1].spc_val);

            if (!valid)
            {
                if(dev->batch)
                    fprintf(stdout, MSG(HHC00365, "W", LCSS_DEVNUM, dev->filename,
                            space1, spctab[i].spc_off, spctab[i].spc_siz));
                else
                    WRMSG(HHC00365, "W", LCSS_DEVNUM, dev->filename,
                          space1, spctab[i].spc_off, spctab[i].spc_siz);
            }
            else
            {
                if(dev->batch)
                    fprintf(stdout, MSG(HHC00366, "W", LCSS_DEVNUM, dev->filename,
                            space1, spctab[i].spc_off, spctab[i].spc_siz, space2, spctab[i+1].spc_off));
                else
                    WRMSG(HHC00366, "W", LCSS_DEVNUM, dev->filename,
                          space1, spctab[i].spc_off, spctab[i].spc_siz, space2, spctab[i+1].spc_off);
            }

            /* setup recovery */
            if (spctab[i].spc_typ == SPCTAB_L2)
            {
                l2errs[spctab[i].spc_val] = 1;
                /* Mark all tracks for the l2 for recovery */
                memset (rcvtab + (spctab[i].spc_val*256), 1, 256);
            }
            else if (spctab[i].spc_typ == trktyp)
                rcvtab[spctab[i].spc_val] = 1;

            if (spctab[i+1].spc_typ == SPCTAB_L2 && valid)
            {
                l2errs[spctab[i+1].spc_val] = 1;
                memset (rcvtab + (spctab[i+1].spc_val*256), 1, 256);
            }
            else if (spctab[i+1].spc_typ == trktyp && valid)
                rcvtab[spctab[i+1].spc_val] = 1;

        } /* if overlap or out of bounds */

        /* Check image l2 entry consistency */
        else if (spctab[i].spc_typ == trktyp
         && (spctab[i].spc_len < CKD_TRKHDR_SIZE
          || spctab[i].spc_len > spctab[i].spc_siz
          || spctab[i].spc_len > trksz))
        {
            recovery = 1;

            /* issue error message */
            if(dev->batch)
                fprintf(stdout, MSG(HHC00367, "W", LCSS_DEVNUM, dev->filename,
                        spaces[trktyp], spctab[i].spc_val, spctab[i].spc_len, spctab[i].spc_siz));
            else
                WRMSG(HHC00367, "W", LCSS_DEVNUM, dev->filename,
                      spaces[trktyp], spctab[i].spc_val, spctab[i].spc_len, spctab[i].spc_siz);

            /* setup recovery */
            rcvtab[spctab[i].spc_val] = 1;
        } /* if inconsistent l2 */
    } /* for each space */

    /* remove any l2 tables or tracks in error from the space table */
    for (i = 0; recovery && spctab[i].spc_typ != SPCTAB_EOF; i++)
        if ((spctab[i].spc_typ == SPCTAB_L2 && l2errs[spctab[i].spc_val])
         || (spctab[i].spc_typ == trktyp    && rcvtab[spctab[i].spc_val]))
            spctab[i].spc_typ = SPCTAB_NONE;

    /* overlaps are serious */
    if (recovery && level < 3)
    {
        level = 3;
        if(dev->batch)
            fprintf(stdout, MSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level));
        else
            WRMSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level);
    }

    /* Rebuild free space if any errors */
    if (recovery || hdrerr
     || cdevhdr.cdh_size     != cdevhdr2.cdh_size
     || cdevhdr.cdh_used     != cdevhdr2.cdh_used
     || cdevhdr.free_num     != cdevhdr2.free_num
     || cdevhdr.free_largest != cdevhdr2.free_largest
     || cdevhdr.free_total   != cdevhdr2.free_total
     || cdevhdr.free_imbed   != cdevhdr2.free_imbed
       )
        fsperr = 1;

    /*---------------------------------------------------------------
     * read the free space
     *---------------------------------------------------------------*/

    lopos = CCKD_L1TAB_POS + l1size;
    hipos = fst.st_size;

    if (level >= 1 && !fsperr)
    {
        while (cdevhdr.free_off) // `while' so code can break
        {
            fsperr = 1;  // be pessimistic
            fsp = NULL;

            /* Read the free space */
            off = (off_t)cdevhdr.free_off;
            len = CCKD_FREEBLK_SIZE;
            if (off < lopos || off + CCKD_FREEBLK_SIZE > hipos
             || lseek (fd, off, SEEK_SET) < 0
             || (rc = read (fd, &freeblk, len)) != len)
                break;

            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));

            if (memcmp (&freeblk, "FREE_BLK", 8) == 0)
            {
                /* new format free space */
                len = cdevhdr.free_num * CCKD_FREEBLK_SIZE;
                if ((fsp = malloc(len)) == NULL
                 || (rc = read (fd, fsp, len)) != len)
                    break;

                for (i = 0; i < cdevhdr.free_num; i++)
                {
                    if (swapend) cckd_swapend_free (&fsp[i]);
                    spctab[s].spc_typ = SPCTAB_FREE;
                    spctab[s].spc_val = -1;
                    spctab[s].spc_off = fsp[i].fb_offnxt;
                    spctab[s].spc_len =
                    spctab[s].spc_siz = fsp[i].fb_len;
                    /* Free space should be ascending */
                    if (spctab[s].spc_off < lopos
                     || spctab[s].spc_off + spctab[s].spc_siz > hipos)
                        break;
                    lopos = spctab[s].spc_off + spctab[s].spc_siz;
                    s++;
                } /* for each free space */
                if (i >= cdevhdr.free_num)
                    fsperr = 0;
            } /* new format free space */
            else
            {
                /* old format free space */
                off = (off_t)cdevhdr.free_off;
                len = CCKD_FREEBLK_SIZE;
                for (i = 0; i < cdevhdr.free_num; i++)
                {
                    if (off < lopos || off > hipos) break;
                    if (lseek (fd, off, SEEK_SET) < 0)
                        goto cdsk_lseek_error;
                    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                    if ((rc = read (fd, &freeblk, len)) != len)
                        goto cdsk_read_error;
                    if (swapend) cckd_swapend_free (&freeblk);
                    spctab[s].spc_typ = SPCTAB_FREE;
                    spctab[s].spc_val = -1;
                    spctab[s].spc_off = (U32)off;
                    spctab[s].spc_len =
                    spctab[s].spc_siz = freeblk.fb_len;
                    s++;
                    lopos = off + freeblk.fb_len;
                    off = (off_t)freeblk.fb_offnxt;
                }
                if (i >= cdevhdr.free_num && freeblk.fb_offnxt == 0)
                    fsperr = 0;
            } /* if old format free space */

            if (fsp) free(fsp);
            fsp = NULL;

            /* Check for gaps/overlaps */
            qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);
            for (i = 0; !fsperr && spctab[i].spc_typ != SPCTAB_EOF; i++)
                if (spctab[i].spc_off + spctab[i].spc_siz != spctab[i+1].spc_off)
                    fsperr = 1;
            break;
        } /* while (cdevhdr.free_off) */
    } /* if (level >= 1 && !fsperr) */

    if (fsperr)
    {
        if(dev->batch)
            fprintf(stdout, MSG(HHC00368, "W", LCSS_DEVNUM, dev->filename));
        else
            WRMSG(HHC00368, "W", LCSS_DEVNUM, dev->filename);
    }

    /*---------------------------------------------------------------
     * Read track headers/images
     *---------------------------------------------------------------*/

cdsk_space_check:

    if (level >= 2)
    {
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        {
            if (spctab[i].spc_typ != trktyp) continue;

            /* read the header or image depending on the check level */
            off = spctab[i].spc_off;
            if ( lseek (fd, off, SEEK_SET) < 0 )
                goto cdsk_lseek_error;
            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
            len = level < 3 ? CKD_TRKHDR_SIZE : spctab[i].spc_len;
            if ((rc = read (fd, buf, len)) != len)
                goto cdsk_read_error;

            /* Extract header info */
            comp = buf[0];
            cyl  = fetch_hw (buf + 1);
            head = fetch_hw (buf + 3);
            trk  = cyl * heads + head;

            /* Validate header info */
            if (compmask[comp] == 0xff
             || cyl >= cyls || head >= heads
             || trk != spctab[i].spc_val)
            {
                if(dev->batch)
                    fprintf(stdout, MSG(HHC00369, "W", LCSS_DEVNUM, dev->filename,
                            spaces[trktyp], spctab[i].spc_val, off,
                            buf[0],buf[1],buf[2],buf[3],buf[4]));
                else
                    WRMSG(HHC00369, "W", LCSS_DEVNUM, dev->filename,
                          spaces[trktyp], spctab[i].spc_val, off,
                          buf[0],buf[1],buf[2],buf[3],buf[4]);

                /* recover this track */
                rcvtab[spctab[i].spc_val] = recovery = 1;
                spctab[i].spc_typ = SPCTAB_NONE;

                /* Force level 3 checking */
                if (level < 3)
                {
                    level = 3;
                    if(dev->batch)
                        fprintf(stdout, MSG(HHC00364, "W", LCSS_DEVNUM,
                                dev->filename, level));
                    else
                        WRMSG(HHC00364, "W", LCSS_DEVNUM, dev->filename, level);
                    goto cdsk_space_check;
                }
                continue;
            } /* if invalid header info */

            /* Check if compression supported */
            if (compmask[comp])
            {
                comperrs = 1;
                if(dev->batch)
                    fprintf(stdout, MSG(HHC00370, "W", LCSS_DEVNUM, dev->filename,
                            spaces[trktyp], trk, comps[compmask[comp]]));
                else
                    WRMSG(HHC00370, "W", LCSS_DEVNUM, dev->filename,
                          spaces[trktyp], trk, comps[compmask[comp]]);
                continue;
            }

            /* Validate the space if check level 3 */
            if (level > 2)
            {
                if (!cdsk_valid_trk (trk, buf, heads, len))
                {
                    if(dev->batch)
                        fprintf(stdout, MSG(HHC00371, "W", LCSS_DEVNUM, dev->filename,
                                spaces[trktyp], trk, off, len));
                    else
                        WRMSG(HHC00371, "W", LCSS_DEVNUM, dev->filename,
                              spaces[trktyp], trk, off, len);

                    /* recover this track */
                    rcvtab[trk] = recovery = 1;
                    spctab[i].spc_typ = SPCTAB_NONE;
                } /* if invalid space */
                else
                    rcvtab[trk] = 0;
            } /* if level > 2 */
        } /* for each space */
    } /* if (level >= 2) */

    /*---------------------------------------------------------------
     * Recovery
     *---------------------------------------------------------------*/

cdsk_recovery:

    if (recovery || level == 4)
    {
     U32 flen, fpos;

        /*-----------------------------------------------------------
         * Phase 1 -- recover trk/blkgrp images
         *-----------------------------------------------------------*/
        /*
         * Reset the end-of-file pos to the file size
         * It might have been changed if new format free space
         * occurred at the end of the file.
         */
        qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);
        while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;
        spctab[s-1].spc_off = fst.st_size;

        /* count number tracks to be recovered */
        for (i = n = 0; i < trks; i++)
            if (rcvtab[i] == 1)
               n++;

        /*-----------------------------------------------------------
         * ckd recovery
         *-----------------------------------------------------------*/
        if (ckddasd)
        {
            /* recovery loop */
            s = cdsk_build_free_space (spctab, s);
            for (f = 0; spctab[f].spc_typ != SPCTAB_EOF && n; )
            {
                /* next free space if too small */
                if (spctab[f].spc_typ != SPCTAB_FREE
                 || spctab[f].spc_siz <= CKD_TRKHDR_SIZE+8)
                {
                    for (f = f + 1; spctab[f].spc_typ != SPCTAB_EOF; f++)
                        if (spctab[f].spc_typ == SPCTAB_FREE)
                            break;
                    continue;
                }

                fpos = spctab[f].spc_off;
                flen = spctab[f].spc_siz;

                /* length to read */
                len = flen < sizeof(buf) ? flen : sizeof(buf);

                /* read the free space */
                off = (off_t)fpos;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = read (fd, buf, len)) != len)
                    goto cdsk_read_error;

                /* Scan the space for a trkhdr */
                for (i = 0; i < len - (CKD_TRKHDR_SIZE+8); i++)
                {
                    /* Check compression byte */
                    if (compmask[buf[i]])
                        continue;

                    /* Fetch possible trkhdr */
                    comp = buf[i];
                    cyl  = fetch_hw (buf + i + 1);
                    head = fetch_hw (buf + i + 3);
                    trk  = cyl * heads + head;

                    /* Validate possible trkhdr */
                    if (cyl >= cyls || head >= heads || rcvtab[trk] != 1)
                        continue;

                    /* Quick validation for compress none */
                    if (comp == CCKD_COMPRESS_NONE
                     && (fetch_hw (buf + i +  5) != cyl   // r0 cyl
                      || fetch_hw (buf + i +  7) != head  // r0 head
                      || buf[i +  9] != 0                 // r0 record
                      || buf[i + 10] != 0                 // r0 key length
                      || fetch_hw (buf + i + 11) != 8     // r0 data length
                        )
                       )
                        continue;

                    /* Quick validation for zlib */
                    else if (comp == CCKD_COMPRESS_ZLIB
                     && fetch_hw(buf + i + 5) % 31 != 0)
                        continue;

                    /* Quick validation for bzip2 */
                    else if (comp == CCKD_COMPRESS_BZIP2
                     && (buf[i+5] != 'B' || buf[i+6] != 'Z'))
                        continue;
                    /*
                     * If we are in `borrowed space' then start over
                     * with the current position at the beginning
                     */
                    if (flen > (U32)len && i > len - (int)trksz)
                        break;

                    /* Checks for comp none */
                    if (comp == CCKD_COMPRESS_NONE)
                    {
                        l = len - i;
                        if ((l = cdsk_valid_trk (trk, buf+i, heads, -l)))
                            goto cdsk_ckd_recover;
                        else
                             continue;
                    }

                    /* Check short `length' */
                    if (flen == (U32)len && (l = len - i) <= 1024)
                    {
                        if (cdsk_valid_trk (trk, buf+i, heads, l))
                        {
                            while (cdsk_valid_trk (trk, buf+i, heads, --l));
                            l++;
                            goto cdsk_ckd_recover;
                        }
                    }

                    /* Scan for next trkhdr */
                    for (j = i + CKD_TRKHDR_SIZE+8;
                         j <= len - (CKD_TRKHDR_SIZE+8);
                         j++)
                    {
                        if (j - i > (int)trksz) break;

                        if (compmask[buf[j]] != 0
                         || fetch_hw(buf+j+1) >= cyls
                         || fetch_hw(buf+j+3) >= heads)
                            continue;

                        /* check uncompressed hdr */
                        if (buf[j] == CCKD_COMPRESS_NONE
                         && (fetch_hw (buf+j+5) != fetch_hw(buf+j+1)
                          || fetch_hw (buf+j+7) != fetch_hw(buf+j+3)
                          || buf[j+9] != 0      || buf[j+10] != 0
                          || fetch_hw(buf+j+11) != 8))
                                continue;
                        /* check zlib compressed header */
                        else if (buf[j] == CCKD_COMPRESS_ZLIB
                         && fetch_hw(buf + j + 5) % 31 != 0)
                                continue;
                        /* check bzip2 compressed header */
                        else if (buf[j] == CCKD_COMPRESS_BZIP2
                         && (buf[j+5] != 'B' || buf[j+6] != 'Z'))
                                continue;

                        /* check to possible trkhdr */
                        l = j - i;
                        if (cdsk_valid_trk (trk, buf+i, heads, l))
                        {
#if 0
                            while (cdsk_valid_trk (trk, buf+i, heads, --l));
                            l++;
#endif
                            goto cdsk_ckd_recover;
                        }

                    } /* scan for next trkhdr */

                    /* Check `length' */
                    if (flen == (U32)len && (l = len - i) <= (int)trksz)
                    {
                        if (cdsk_valid_trk (trk, buf+i, heads, l))
                        {
                            while (cdsk_valid_trk (trk, buf+i, heads, --l));
                            l++;
                            goto cdsk_ckd_recover;
                        }
                    }

                    /* Scan all lengths */
                    for (l = CKD_TRKHDR_SIZE+8; i + l <= len; l++)
                    {
                        if (l > (int)trksz)
                            break;
                        if (cdsk_valid_trk (trk, buf+i, heads, l))
                            goto cdsk_ckd_recover;
                    } /* for all lengths */

                    continue;

cdsk_ckd_recover:
                    if(dev->batch)
                        fprintf(stdout, MSG(HHC00372, "I", LCSS_DEVNUM, dev->filename,
                                spaces[trktyp], trk, off + i, l));
                    else
                        WRMSG(HHC00372, "I", LCSS_DEVNUM, dev->filename,
                              spaces[trktyp], trk, off + i, l);
                    n--;
                    rcvtab[trk] = 2;

                    /* add recovered track to the space table */
                    spctab[s].spc_typ = trktyp;
                    spctab[s].spc_val = trk;
                    spctab[s].spc_off = fpos + i;
                    spctab[s].spc_len =
                    spctab[s].spc_siz = l;
                    s++;
                    /*
                     * adjust `i' knowing it will be incremented
                     * in the `for' loop above.
                     */
                    i += l - 1;
                } /* for each byte in the free space */

                /* Adjust the free space for what we processed */
                spctab[f].spc_off += i;
                spctab[f].spc_len -= i;
                spctab[f].spc_siz -= i;

            } /* for each free space */

        } /* if ckddasd */

        /*-----------------------------------------------------------
         * fba recovery
         *-----------------------------------------------------------*/

        /*
         * FBA blkgrps are harder to recover than CKD tracks because
         * there is not any information within the blkgrp itself to
         * validate (unlike a track, which has count fields that
         * terminate in an end-of-track marker).
         *
         * On the first pass we recover all compressed blkgrps since
         * these are readily validated (they must uncompress to a
         * certain size, CFBA_BLKGRP_SIZE+CKD_TRKHDR_SIZE).  We
         * also recover uncompressed blkgrps if they are followed by
         * a valid trkhdr (and don't occur to close to the beginning
         * of the file).
         *
         * On the second pass we recover all uncompressed blkgrps
         * that weren't recovered in the first pass.  The only
         * criteria is that the compression byte is zero and the
         * 4 byte blkgrp number is in range and there are at least
         * CFBA_BLKGRP_SIZE bytes following.
         */

        for (pass = 0; fbadasd && pass < 2; pass++)
        {
            lopos = CCKD_L1TAB_POS + (cdevhdr.num_L1tab * 4);
            if (pass == 0)
                lopos += (cdevhdr.num_L1tab * CCKD_L2TAB_SIZE);

            /* recovery loop */
            s = cdsk_build_free_space (spctab, s);
            for (f = 0; spctab[f].spc_typ != SPCTAB_EOF && n > 0; )
            {
                U32 flen, fpos;

                /* next free space if too small */
                if (spctab[f].spc_typ != SPCTAB_FREE
                 || spctab[f].spc_siz <= CKD_TRKHDR_SIZE+8
                 || (pass == 1 && spctab[f].spc_siz < blkgrpsz))
                {
                    for (f = f + 1; spctab[f].spc_typ != SPCTAB_EOF; f++)
                        if (spctab[f].spc_typ == SPCTAB_FREE)
                            break;
                    continue;
                }

                fpos = spctab[f].spc_off;
                flen = spctab[f].spc_siz;
                /*
                 * calculate length to read
                 * if flen > len then we only read part of the space
                 */
                len = flen < sizeof(buf) ? flen : sizeof(buf);

                /* read the free space */
                off = (off_t)fpos;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = read (fd, buf, len)) != len)
                    goto cdsk_read_error;

                /* Scan the space */
                for (i = 0; i < len - (CKD_TRKHDR_SIZE+8); i++)
                {
                    /* For pass 1 the size left must be at least blkgrpsz */
                    if (pass == 1 && len - i < (int)blkgrpsz)
                        break;

                    /* Check compression byte */
                    if ((pass == 0 && compmask[buf[i]])
                     || (pass == 1 && buf[i] != CCKD_COMPRESS_NONE))
                        continue;

                    /* Fetch possible trkhdr */
                    comp = buf[i];
                    blkgrp = fetch_fw (buf + i + 1);

                    /* Validate possible trkhdr */
                    if (blkgrp < 0 || blkgrp >= blkgrps
                     || rcvtab[blkgrp] != 1)
                        continue;

                    /* Validation for compress none */
                    if (comp == CCKD_COMPRESS_NONE
                     && flen == (U32)len && len - i < (int)blkgrpsz)
                        continue;

                    /* Quick validation for zlib */
                    else if (comp == CCKD_COMPRESS_ZLIB
                     && fetch_hw(buf + i + 5) % 31 != 0)
                        continue;

                    /* Quick validation for bzip2 */
                    else if (comp == CCKD_COMPRESS_BZIP2
                     && (buf[i+5] != 'B' || buf[i+6] != 'Z'))
                        continue;
                    /*
                     * If we are in `borrowed space' then start over
                     * with the current position at the beginning
                     */
                    if (flen > (U32)len && i > len - (int)blkgrpsz)
                        break;

                    /* Checks for comp none */
                    if (comp == CCKD_COMPRESS_NONE)
                    {
                        l = blkgrpsz;
                        if (len - i < (int)blkgrpsz || fpos + i < lopos)
                            continue;
                        if (len - i == (int)blkgrpsz && flen == (U32)len)
                            goto cdsk_fba_recover;
                        /* Pass 0 checks */
                        if (pass == 0
                         && (len - i - l < CKD_TRKHDR_SIZE+8
                          || compmask[buf[i+l]]
                          || fetch_fw (buf+i+l+1) >= (unsigned int)blkgrps)
                           )
                            continue;
                        goto cdsk_fba_recover;
                    }

                    /* The tests below are for pass 0 only */
                    if (pass == 1)
                        continue;

                    /* Check short `length' */
                    if (flen == (U32)len && (l = len - i) <= 1024)
                    {
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, l))
                        {
                            while (cdsk_valid_trk (blkgrp, buf+i, heads, --l));
                            l++;
                            goto cdsk_fba_recover;
                        }
                    }

                    /* Scan for next trkhdr */
                    for (j = i + CKD_TRKHDR_SIZE+8;
                         j <= len - (CKD_TRKHDR_SIZE+8);
                         j++)
                    {
                        if (j - i > (int)blkgrpsz) break;

                        if (compmask[buf[j]] != 0
                         || fetch_fw(buf+j+1) >= (unsigned int)blkgrps)
                            continue;

                        /* check zlib compressed header */
                        if (buf[j] == CCKD_COMPRESS_ZLIB
                         && fetch_hw(buf + j + 5) % 31 != 0)
                            continue;

                        /* check bzip2 compressed header */
                        else if (buf[j] == CCKD_COMPRESS_BZIP2
                         && (buf[j+5] != 'B' || buf[j+6] != 'Z'))
                                continue;

                        /* check to possible trkhdr */
                        l = j - i;
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, l))
                        {
#if 0
                            while (cdsk_valid_trk (blkgrp, buf+i, heads, --l));
                            l++;
#endif
                            goto cdsk_fba_recover;
                        }

                    } /* scan for next trkhdr */

                    /* Check `length' */
                    l = len - i;
                    if (flen == (U32)len && l <= (int)blkgrpsz)
                    {
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, l))
                        {
                            while (cdsk_valid_trk (blkgrp, buf+i, heads, --l));
                            l++;
                            goto cdsk_fba_recover;
                        }
                    }

                    /* Scan all lengths */
                    for (l = CKD_TRKHDR_SIZE+8; i + l <= len; l++)
                    {
                        if (l > (int)blkgrpsz)
                            break;
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, l))
                            goto cdsk_fba_recover;
                    } /* for all lengths */

                    continue;

cdsk_fba_recover:
                    if(dev->batch)
                        fprintf(stdout, MSG(HHC00372, "I", LCSS_DEVNUM, dev->filename,
                                spaces[trktyp], blkgrp, off + i, l));
                    else
                        WRMSG(HHC00372, "I", LCSS_DEVNUM, dev->filename,
                              spaces[trktyp], blkgrp, off + i, l);
                    n--;
                    rcvtab[blkgrp] = 2;

                    /* Enable recovery of comp 0 blkgrps for pass 0 */
                    if (fpos + i < lopos)
                        lopos = fpos + i;

                    /* add recovered block group to the space table */
                    spctab[s].spc_typ = trktyp;
                    spctab[s].spc_val = blkgrp;
                    spctab[s].spc_off = fpos + i;
                    spctab[s].spc_len =
                    spctab[s].spc_siz = l;
                    s++;
                    /*
                     * adjust `i' knowing it will be incremented
                     * in the `for' loop above.
                     */
                    i += l - 1;
                } /* for each byte in the free space */

                /* Adjust the free space for what we processed */
                spctab[f].spc_off += i;
                spctab[f].spc_len -= i;
                spctab[f].spc_siz -= i;

            } /* for each free space */

        } /* if fbadasd */

        for (i = n = 0; i < trks; i++)
            if (rcvtab[i] == 2)
                n++;

        if(dev->batch)
            fprintf(stdout, MSG(HHC00373, "I", LCSS_DEVNUM, dev->filename,
                    n, spaces[trktyp]));
        else
            WRMSG(HHC00373, "I", LCSS_DEVNUM, dev->filename,
                  n, spaces[trktyp]);

        /*-----------------------------------------------------------
         * Phase 2 -- rebuild affected l2 tables
         *-----------------------------------------------------------*/

        /*
         * Make sure there's at least one non-zero `rcvtab' entry
         * for l2 tables in `l2errs'.  Space validation may have
         * turned off all `rcvtab' entries for an l2.
         */
        for (i = 0; i < cdevhdr.num_L1tab; i++)
            if (l2errs[i])
                rcvtab[i*256] = 1;

        /* Get storage for the l2 table array */
        n = cdevhdr.num_L1tab;
        len = sizeof(void *);
        if ((l2 = calloc (n, len)) == NULL)
            goto cdsk_calloc_error;

        /* Get storage for the rebuilt l2 tables */
        len = CCKD_L2TAB_SIZE;
        for (i = 0; i < trks; i++)
        {
            L1idx = i / 256;
            if (rcvtab[i] != 0 && l2[L1idx] == NULL)
            {
                if ((l2[L1idx] = malloc (len)) == NULL)
                    goto cdsk_malloc_error;
                l1[L1idx] = shadow ? 0xffffffff : 0;
                memcpy (l2[L1idx], &empty_l2, len);
            }
        }

        /* Rebuild the l2 tables */
        qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        {
            if (spctab[i].spc_typ == SPCTAB_L2 && l2[spctab[i].spc_val])
                spctab[i].spc_typ = SPCTAB_NONE;
            else if (spctab[i].spc_typ == trktyp && l2[spctab[i].spc_val/256])
            {
                L1idx = spctab[i].spc_val / 256;
                l2x = spctab[i].spc_val % 256;
                l2[L1idx][l2x].L2_trkoff = spctab[i].spc_off;
                l2[L1idx][l2x].L2_len   = spctab[i].spc_len;
                l2[L1idx][l2x].L2_size  = spctab[i].spc_siz;
            }
        } /* for each space */
        qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);
        while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;

        /* Look for empty l2 tables */
        for (i = 0; i < cdevhdr.num_L1tab; i++)
            if (l2[i] != NULL
             && memcmp (l2[i], &empty_l2, CCKD_L2TAB_SIZE) == 0)
            {
                free (l2[i]);
                l2[i] = NULL;
            }
        /*
         * `s-1' indexes the SPCTAB_EOF space table entry.
         * Set its `pos' to the maximum allowed value to ensure
         * there will be free space for the rebuilt l2 tables.
         */
        spctab[s-1].spc_off = (U32)maxsize;

        /* Build the free space */
        s = cdsk_build_free_space (spctab, s);

        /* Find space for the rebuilt l2 tables */
        for (i = j = 0; i < cdevhdr.num_L1tab; i++)
        {
            if (l2[i] == NULL) continue;

            /* find a free space */
            for ( ; spctab[j].spc_typ != SPCTAB_EOF; j++)
                if (spctab[j].spc_typ == SPCTAB_FREE
                 && spctab[j].spc_siz >= CCKD_L2TAB_SIZE)
                     break;

            /* weird error if no space */
            if (spctab[j].spc_typ == SPCTAB_EOF)
            {
                if(dev->batch)
                    fprintf(stdout, MSG(HHC00374, "E", LCSS_DEVNUM, dev->filename));
                else
                    WRMSG(HHC00374, "E", LCSS_DEVNUM, dev->filename);
                goto cdsk_error;
            }

            /* add l2 space */
            l1[i]             = spctab[j].spc_off;
            spctab[s].spc_typ = SPCTAB_L2;
            spctab[s].spc_val = i;
            spctab[s].spc_off = spctab[j].spc_off;
            spctab[s].spc_len =
            spctab[s].spc_siz = CCKD_L2TAB_SIZE;
            s++;

            /* adjust the free space */
            spctab[j].spc_off += CCKD_L2TAB_SIZE;
            spctab[j].spc_len -= CCKD_L2TAB_SIZE;
            spctab[j].spc_siz -= CCKD_L2TAB_SIZE;
        } /* for each l2 table */


        /*-----------------------------------------------------------
         * Phase 3 -- write l1 and l2 tables
         *-----------------------------------------------------------*/

        if (ro)
        {
            if(dev->batch)
                fprintf(stdout, MSG(HHC00375, "W", LCSS_DEVNUM, dev->filename,
                        "file opened read-only"));
            else
                WRMSG(HHC00375, "W", LCSS_DEVNUM, dev->filename,
                      "file opened read-only");
              goto cdsk_error;
        }
        if (comperrs)
        {
            if(dev->batch)
                fprintf(stdout, MSG(HHC00375, "W", LCSS_DEVNUM, dev->filename,
                        "missing compression"));
            else
                WRMSG(HHC00375, "W", LCSS_DEVNUM, dev->filename,
                      "missing compression");
            goto cdsk_error;
        }

        /* Write the l1 table */
        off = CCKD_L1TAB_POS;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cdsk_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = l1size;
        if ((rc = write (fd, l1, len)) != len)
            goto cdsk_write_error;

        /* Write l2 tables */
        qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        {
            L1idx = spctab[i].spc_val;
            if (spctab[i].spc_typ != SPCTAB_L2 || l2[L1idx] == NULL)
                continue;
            off = (off_t)l1[L1idx];
            if (lseek (fd, off, SEEK_SET) < 0)
                goto cdsk_lseek_error;
            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
            len = CCKD_L2TAB_SIZE;
            if ((rc = write (fd, l2[L1idx], len)) != len)
                goto cdsk_write_error;
            free (l2[L1idx]);
            l2[L1idx] = NULL;
        } /* for each space */

        /* Free recovery related storage */
        if (l2)
        {
            for (i = 0; i < cdevhdr.num_L1tab; i++)
                if (l2[i])
                    free (l2[i]);
            free (l2); l2 = NULL;
        }
        free (l2errs); l2errs = NULL;
        free (rcvtab); rcvtab = NULL;

        /* Ensure we do free space recovery */
        fsperr = 1;

    } /* if (recovery || level >= 4) */

    /*---------------------------------------------------------------
     * Rebuild free space
     *---------------------------------------------------------------*/

    if (fsperr && ro)
    {
        if(dev->batch)
              fprintf(stdout, MSG(HHC00376, "W", LCSS_DEVNUM, dev->filename));
          else
              WRMSG(HHC00376, "W", LCSS_DEVNUM, dev->filename);
    }
    else if (fsperr)
    {
        /*-----------------------------------------------------------
         * Phase 1 -- build the free space
         *            make sure the last space isn't free space and
         *            that each free space is long enough (8 bytes).
         *-----------------------------------------------------------*/

cdsk_fsperr_retry:

        s = cdsk_build_free_space (spctab, s);

        /*
         * spctab[s-1] is the SPCTAB_EOF entry.
         * if spctab[s-2] is SPCTAB_FREE then discard it
         */
        if (spctab[s-2].spc_typ == SPCTAB_FREE)
        {
            spctab[s-1].spc_typ = SPCTAB_NONE;
            spctab[s-2].spc_typ = SPCTAB_EOF;
            spctab[s-2].spc_val = -1;
            spctab[s-2].spc_len =
            spctab[s-2].spc_siz = 0;
            s--;
        }
        /*
         * Check for short free spaces.
         * If found, shift left until the next free space or eof.
         */
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
            if (spctab[i].spc_typ == SPCTAB_FREE
             && spctab[i].spc_siz < CCKD_FREEBLK_SIZE)
                break;
        if (spctab[i].spc_typ != SPCTAB_EOF)
        {
            /* Shift following space left */
            l = spctab[i++].spc_siz;
            while (spctab[i].spc_typ != SPCTAB_FREE && spctab[i].spc_typ != SPCTAB_EOF)
            {
                /* Read the space and write shifted to the left */
                off = (off_t)spctab[i].spc_off;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                len = spctab[i].spc_siz;
                if ((rc = read (fd, buf, len)) != len)
                    goto cdsk_read_error;
                off -= l;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = write (fd, buf, len)) != len)
                    goto cdsk_write_error;
                spctab[i].spc_off -= l;

                /* Update the l2 or l1 table entry */
                if (spctab[i].spc_typ == trktyp)
                {
                    L1idx = spctab[i].spc_val/256;
                    l2x = spctab[i].spc_val%256;
                    off = (off_t)l1[L1idx] + l2x * CCKD_L2ENT_SIZE;
                    if (lseek (fd, off, SEEK_SET) < 0)
                        goto cdsk_lseek_error;
                    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                    len = CCKD_L2ENT_SIZE;
                    if ((rc = read (fd, &l2ent, len)) != len)
                        goto cdsk_read_error;
                    l2ent.L2_trkoff -= l;
                    if (lseek (fd, off, SEEK_SET) < 0)
                        goto cdsk_lseek_error;
                    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                    if ((rc = write (fd, &l2ent, len)) != len)
                        goto cdsk_write_error;
                } /* trk/blkgrp relocated */
                else if (spctab[i].spc_typ == SPCTAB_L2)
                    l1[spctab[i].spc_val] -= l;
                i++;
            } /* while not FREE space or EOF */
            goto cdsk_fsperr_retry;
        } /* if short free space found */

        /*-----------------------------------------------------------
         * Phase 2 -- rebuild free space statistics
         *-----------------------------------------------------------*/

        cdevhdr.vrm[0] = CCKD_VERSION;
        cdevhdr.vrm[1] = CCKD_RELEASE;
        cdevhdr.vrm[2] = CCKD_MODLVL;

        cdevhdr.cdh_size        = cdevhdr.cdh_used         = cdevhdr.free_off =
        cdevhdr.free_total  = cdevhdr.free_largest =
        cdevhdr.free_num = cdevhdr.free_imbed   = 0;
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
            if (spctab[i].spc_typ == SPCTAB_FREE)
            {
                cdevhdr.cdh_size += spctab[i].spc_siz;
                if (spctab[i].spc_siz > cdevhdr.free_largest)
                    cdevhdr.free_largest = spctab[i].spc_siz;
                cdevhdr.free_total += spctab[i].spc_siz;
                cdevhdr.free_num++;
            }
            else
            {
                cdevhdr.cdh_size += spctab[i].spc_siz;
                cdevhdr.cdh_used += spctab[i].spc_len;
                cdevhdr.free_total += spctab[i].spc_siz - spctab[i].spc_len;
                cdevhdr.free_imbed += spctab[i].spc_siz - spctab[i].spc_len;
             }

        /*-----------------------------------------------------------
         * Phase 3 -- write the free space
         *-----------------------------------------------------------*/

        if (cdevhdr.free_num)
        {
            /* size needed for new format free space */
            len = (cdevhdr.free_num+1) * CCKD_FREEBLK_SIZE;

            /* look for existing free space to fit new format free space */
            for (i = off = 0; !off && spctab[i].spc_typ != SPCTAB_EOF; i++)
                if (spctab[i].spc_typ == SPCTAB_FREE && len <= (int)spctab[i].spc_siz)
                    off = (off_t)spctab[i].spc_off;

            /* if no applicable space see if we can append to the file */
            if (!off && maxsize - cdevhdr.cdh_size >= len)
                off = (off_t)cdevhdr.cdh_size;

            /* get free space buffer */
            if (off && (fsp = malloc (len)) == NULL)
                off = 0;

            if (off)
            {
                /* new format free space */
                memcpy (fsp, "FREE_BLK", 8);
                for (i = 0, j = 1; spctab[i].spc_typ != SPCTAB_EOF; i++)
                    if (spctab[i].spc_typ == SPCTAB_FREE)
                    {
                        fsp[j].fb_offnxt = spctab[i].spc_off;
                        fsp[j++].fb_len = spctab[i].spc_siz;
                    }

                /* Write the free space */
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = write (fd, fsp, len)) != len)
                    goto cdsk_write_error;
                cdevhdr.free_off = (U32)off;

                free (fsp);
                fsp = NULL;
            } /* new format free space */
            else
            {
                /* old format free space */
                len = CCKD_FREEBLK_SIZE;
                for (i = 0; spctab[i].spc_typ != SPCTAB_FREE; i++);
                cdevhdr.free_off = spctab[i].spc_off;
                off = (off_t)spctab[i].spc_off;
                freeblk.fb_offnxt = 0;
                freeblk.fb_len = spctab[i].spc_siz;
                for (i = i + 1; spctab[i].spc_typ != SPCTAB_EOF; i++)
                    if (spctab[i].spc_typ == SPCTAB_FREE)
                    {
                        freeblk.fb_offnxt = spctab[i].spc_off;
                        if (lseek (fd, off, SEEK_SET) < 0)
                            goto cdsk_lseek_error;
                        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                        if (write (fd, &freeblk, len) != len)
                            goto cdsk_write_error;
                        off = (off_t)spctab[i].spc_off;
                        freeblk.fb_offnxt = 0;
                        freeblk.fb_len = spctab[i].spc_len;
                    }
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if (write (fd, &freeblk, len) != len)
                    goto cdsk_write_error;
            } /* old format free space */
        } /* if (cdevhdr.free_num) */

        /* Write cdevhdr and l1 table */
        off = CCKD_DEVHDR_POS;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cdsk_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = CCKD_DEVHDR_SIZE;
        if (write (fd, &cdevhdr, len) != len)
            goto cdsk_write_error;

        off = CCKD_L1TAB_POS;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cdsk_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = l1size;
        if (write (fd, l1, len) != len)
            goto cdsk_write_error;

        /* Truncate the file */
        off = (off_t)cdevhdr.cdh_size;
        if (cdevhdr.free_off == cdevhdr.cdh_size)
            off += (cdevhdr.free_num+1) * CCKD_FREEBLK_SIZE;
        rc = ftruncate (fd, off);

        if(dev->batch)
            fprintf(stdout, MSG(HHC00377, "I", LCSS_DEVNUM, dev->filename));
        else
            WRMSG(HHC00377, "I", LCSS_DEVNUM, dev->filename);

    } /* if (fsperr) */

    /*---------------------------------------------------------------
     * Return
     *---------------------------------------------------------------*/

cdsk_return_ok:

    rc = recovery ? 2 : fsperr ? 1 : 0;

    if (!ro && (cdevhdr.opts & (CCKD_ORDWR|CCKD_OPENED|CCKD_SPERRS)))
    {
        /*
         * Leave the ORDWR bit on for now.  This will prevent
         * old-format free space releases from doing a -1 check
         * on a file that has new-format free space
         */
#if 0
        cdevhdr.opts &= ~(CCKD_ORDWR|CCKD_OPENED|CCKD_SPERRS);
#else
        cdevhdr.opts &= ~(CCKD_OPENED|CCKD_SPERRS);
#endif
        /* Set version.release.modlvl */
        cdevhdr.vrm[0] = CCKD_VERSION;
        cdevhdr.vrm[1] = CCKD_RELEASE;
        cdevhdr.vrm[2] = CCKD_MODLVL;

        off = CCKD_DEVHDR_POS;
        if (lseek (fd, CCKD_DEVHDR_POS, SEEK_SET) >= 0)
            VERIFY(CCKD_DEVHDR_SIZE == write (fd, &cdevhdr, CCKD_DEVHDR_SIZE));
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    }

cdsk_return:

    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));

    /* free all space */
    if (l1)     free (l1);
    if (spctab) free (spctab);
    if (l2errs) free (l2errs);
    if (rcvtab) free (rcvtab);
    if (fsp)    free (fsp);
    if (l2)
    {
        for (i = 0; i < cdevhdr.num_L1tab; i++)
            if (l2[i]) free (l2[i]);
        free (l2);
    }

    return rc;

    /*---------------------------------------------------------------
     * Error exits
     *---------------------------------------------------------------*/

cdsk_fstat_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                "fstat()", strerror(errno)));
    else
        WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
              "fstat()", strerror(errno));
    goto cdsk_error;

cdsk_lseek_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "lseek()", off, strerror(errno)));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "lseek()", off, strerror(errno));
    goto cdsk_error;

cdsk_read_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "read()", off, rc < 0 ? strerror(errno) : "incomplete"));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "read()", off, rc < 0 ? strerror(errno) : "incomplete");
    goto cdsk_error;

cdsk_write_error:
    if(dev->batch)
        fprintf(stdout, MSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "write()", off, rc < 0 ? strerror(errno) : "incomplete"));
    else
        WRMSG(HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "write()", off, rc < 0 ? strerror(errno) : "incomplete");
    goto cdsk_error;

cdsk_malloc_error:
    {
        char buf[64];
        MSGBUF( buf, "malloc(%d)", len);
        if(dev->batch)
            fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror(errno)));
        else
             WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                   buf, strerror(errno));
    }
    goto cdsk_error;

cdsk_calloc_error:
    {
        char buf[64];
        MSGBUF( buf, "calloc(%d)", n * len);
        if(dev->batch)
            fprintf(stdout, MSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror(errno)));
        else
             WRMSG(HHC00354, "E", LCSS_DEVNUM, dev->filename,
                   buf, strerror(errno));
    }
    goto cdsk_error;

cdsk_error:
    rc = -1;
    goto cdsk_return;

} /* end function cckd_chkdsk */

/*-------------------------------------------------------------------
 * cckd_chkdsk() space table sort
 *-------------------------------------------------------------------*/
static int cdsk_spctab_sort(const void *a, const void *b)
{
const SPCTAB *x = a, *y = b;

         if (x->spc_typ == SPCTAB_NONE) return +1;
    else if (y->spc_typ == SPCTAB_NONE) return -1;
    else if (x->spc_typ == SPCTAB_EOF)  return +1;
    else if (y->spc_typ == SPCTAB_EOF)  return -1;
    else if (x->spc_off < y->spc_off)   return -1;
    else                                return +1;
} /* end function cdsk_spctab_sort */

/*-------------------------------------------------------------------*/
/* Build free space in the space table                               */
/*-------------------------------------------------------------------*/
static int cdsk_build_free_space(SPCTAB *spctab, int s)
{
int i;

    for (i = 0; i < s; i++)
        if (spctab[i].spc_typ == SPCTAB_FREE)
            spctab[i].spc_typ = SPCTAB_NONE;
    qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);
    while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        if (spctab[i].spc_off + spctab[i].spc_siz < spctab[i+1].spc_off)
        {
            spctab[s].spc_typ = SPCTAB_FREE;
            spctab[s].spc_val = -1;
            spctab[s].spc_off = spctab[i].spc_off + spctab[i].spc_siz;
            spctab[s].spc_len =
            spctab[s].spc_siz = spctab[i+1].spc_off - spctab[s].spc_off;
            s++;
        }
    qsort (spctab, s, sizeof(SPCTAB), cdsk_spctab_sort);
    return s;
}

/*-------------------------------------------------------------------*/
/* Validate a track image                                            */
/*                                                                   */
/* If `len' is negative and compression is CCKD_COMPRESS_NONE then   */
/* `len' indicates a buffer size containing the track image and the  */
/* value returned is the actual track image length                   */
/*-------------------------------------------------------------------*/
static int cdsk_valid_trk (int trk, BYTE *buf, int heads, int len)
{
int             i;                      /* Index                     */
int             len2;                   /* Positive `len'            */
int             kl, dl;                 /* Key/Data lengths          */
BYTE           *bufp;                   /* Buffer pointer            */
int             bufl;                   /* Buffer length             */
#if defined( HAVE_ZLIB )
uLongf          zlen;
#endif
#if defined( CCKD_BZIP2 )
unsigned int    bz2len;
#endif
#if defined( HAVE_ZLIB ) || defined( CCKD_BZIP2 )
int             rc;                     /* Return code               */
BYTE            buf2[65536];            /* Uncompressed buffer       */
#endif

    /* Negative len only allowed for comp none */
    len2 = len > 0 ? len : -len;

    if (len2 < CKD_TRKHDR_SIZE + 8)
        return 0;

    /* Uncompress the track/block image */
    switch (buf[0]) {

    case CCKD_COMPRESS_NONE:
        bufp = buf;
        bufl = len2;
        break;

#if defined( HAVE_ZLIB )
    case CCKD_COMPRESS_ZLIB:
        if (len < 0) return 0;
        bufp = (BYTE *)buf2;
        memcpy (buf2, buf, CKD_TRKHDR_SIZE);
        zlen = sizeof(buf2) - CKD_TRKHDR_SIZE;
        rc = uncompress (buf2 + CKD_TRKHDR_SIZE, &zlen,
                         buf + CKD_TRKHDR_SIZE, len - CKD_TRKHDR_SIZE);
        if (rc != Z_OK)
            return 0;
        bufl = (int)zlen + CKD_TRKHDR_SIZE;
        break;
#endif

#if defined( CCKD_BZIP2 )
    case CCKD_COMPRESS_BZIP2:
        if (len < 0) return 0;
        bufp = (BYTE *)buf2;
        memcpy (buf2, buf, CKD_TRKHDR_SIZE);
        bz2len = sizeof(buf2) - CKD_TRKHDR_SIZE;
        rc = BZ2_bzBuffToBuffDecompress ( (char *)&buf2[CKD_TRKHDR_SIZE], &bz2len,
                         (char *)&buf[CKD_TRKHDR_SIZE], len - CKD_TRKHDR_SIZE, 0, 0);
        if (rc != BZ_OK)
            return 0;
        bufl = (int)bz2len + CKD_TRKHDR_SIZE;
        break;
#endif

    default:
        return 0;

    } /* switch (buf[0]) */

    /* fba check */
    if (heads == 65536)
    {
        if (bufl != CFBA_BLKGRP_SIZE + CKD_TRKHDR_SIZE)
            return 0;
        else
            return len > 0 ? len : bufl;
    }
    /* Check length */
    if (bufl <= 5 + 8 + 8 + 8 + 8) return 0;
    /* Check ha */
    if (fetch_hw(bufp + 1) != trk / heads
     || fetch_hw(bufp + 3) != trk % heads)
        return 0;
    /* Check r0 */
    if (fetch_hw(bufp + 1) != fetch_hw(bufp + 5)
     || fetch_hw(bufp + 3) != fetch_hw(bufp + 7)
     || bufp[9] != 0       || bufp[10] != 0
     || fetch_hw(bufp +11) != 8)
        return 0;
    /* Check user records */
    for (i = 21; i < bufl - 8; i += 8 + kl + dl)
    {
        if (fetch_hw(bufp + i + 2) >= heads || bufp[i + 4] == 0)
            break;
        kl = bufp[i + 5];
        dl = fetch_hw(bufp + i + 6);
    }
    if (len < 0) bufl = i + 8;
    if (i != bufl - 8 || memcmp(bufp + i, eighthexFF, 8))
        return 0;
    return len > 0 ? len : bufl;
} /* end function cdsk_valid_trk */
