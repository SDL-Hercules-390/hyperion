/* CCKDUTIL64.C (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2022                             */
/*              CCKD64 (Compressed CKD64) Common routines            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains functions for compressed CKD devices         */
/* used by more than 1 main program.                                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _CCKDUTIL64_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "opcode.h"
#include "dasdblks.h"
#include "ccwarn.h"

/*-------------------------------------------------------------------*/
/* Internal functions                                                */
/*-------------------------------------------------------------------*/
static int  comp_spctab64_sort(const void *a, const void *b);
static int  cdsk_spctab64_sort(const void *a, const void *b);
static int  cdsk_build_free_space64(SPCTAB64 *spctab, int s);

/*-------------------------------------------------------------------*/
/* Helper macro                                                      */
/*-------------------------------------------------------------------*/
#define  gui_fprintf        if (extgui) fprintf

/*-------------------------------------------------------------------*/
/* Toggle the endianness of a compressed file                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int cckd64_swapend (DEVBLK *dev)
{
CCKD64_EXT       *cckd;                 /* -> cckd extension         */
int               fd;                   /* File descriptor           */
int               rc;                   /* Return code               */
struct stat       fst;                  /* File status buffer        */
int               i;                    /* Index                     */
int               swapend;              /* 1=swap space              */
int               len;                  /* Length                    */
U64               off, lopos, hipos;    /* File offsets              */
CCKD64_DEVHDR     cdevhdr;              /* Compressed ckd header     */
CCKD64_L1ENT     *l1 = NULL;            /* Level 1 table             */
CCKD64_L2ENT      l2[256];              /* Level 2 table             */
CCKD64_FREEBLK    freeblk;              /* Free block                */

    if (!dev->cckd64)
        return cckd_swapend( dev );

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
    off = CCKD64_DEVHDR_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD64_DEVHDR_SIZE;
    if ((rc = read (fd, &cdevhdr, len)) != len)
        goto cswp_read_error;

    swapend = (cdevhdr.cdh_opts & CCKD_OPT_BIGEND) != cckd_def_opt_bigend();

    cckd64_swapend_chdr (&cdevhdr);

    cdevhdr.cdh_opts |= CCKD_OPT_OPENRW;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = write (fd, &cdevhdr, len)) != len)
        goto cswp_write_error;

    if (!swapend)
        cckd64_swapend_chdr (&cdevhdr);

    /* l1 table */
    len = cdevhdr.num_L1tab * CCKD64_L1ENT_SIZE;
    if ((l1 = malloc (len)) == NULL)
        goto cswp_malloc_error;
    off = CCKD64_L1TAB_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = read (fd, l1, len)) != len)
        goto cswp_read_error;

    cckd64_swapend_l1 (l1, cdevhdr.num_L1tab);

    if (lseek (fd, off, SEEK_SET) < 0)
        goto cswp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((rc = write (fd, l1, len)) != len)
        goto cswp_write_error;

    if (!swapend)
        cckd64_swapend_l1 (l1, cdevhdr.num_L1tab);

    lopos = CCKD64_L1TAB_POS + len;

    /* l2 tables */
    for (i = 0; i < cdevhdr.num_L1tab; i++)
    {
        if (0
            || l1[i] == CCKD64_NOSIZE
            || l1[i] == CCKD64_MAXSIZE
            || l1[i] < lopos
            || l1[i] > hipos - CCKD64_L2TAB_SIZE
        )
            continue;

        off = l1[i];
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cswp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = CCKD64_L2TAB_SIZE;
        if ((rc = read (fd, l2, len)) != len)
            goto cswp_read_error;

        cckd64_swapend_l2 (l2);

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
     && cdevhdr.free_off <= hipos - CCKD64_FREEBLK_SIZE)
    {
        off = cdevhdr.free_off;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cswp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = CCKD64_FREEBLK_SIZE;
        if ((rc = read (fd, &freeblk, len)) != len)
            goto cswp_read_error;
        if (memcmp(&freeblk, "FREE_BLK", 8) == 0)
        {
            /* New format free space */
            for (i = 0; i < cdevhdr.free_num; i++)
            {
                off += CCKD64_FREEBLK_SIZE;
                if (off > hipos - CCKD64_FREEBLK_SIZE)
                    break;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cswp_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = read (fd, &freeblk, len)) != len)
                    goto cswp_read_error;

                cckd64_swapend_free (&freeblk);

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
                if (off < lopos || off > hipos - CCKD64_FREEBLK_SIZE)
                    break;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cswp_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = read (fd, &freeblk, len)) != len)
                    goto cswp_read_error;

                cckd64_swapend_free (&freeblk);

                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cswp_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((rc = write (fd, &freeblk, len)) != len)
                    goto cswp_write_error;

                if (!swapend)
                    cckd64_swapend_free (&freeblk);

                off = freeblk.fb_offnxt;
            } /* for each free space */
        } /* else old format free space */
    } /* if free space */

    return 0;

    /* error exits */
cswp_fstat_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                "fstat()", strerror( errno ));
    else
        WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
              "fstat()", strerror( errno ));
    goto cswp_error;

cswp_lseek_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "lseek()", off, strerror( errno ));
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "lseek()", off, strerror( errno ));
    goto cswp_error;

cswp_read_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "read()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "read()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    goto cswp_error;

cswp_write_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "write()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
              "write()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    goto cswp_error;

cswp_malloc_error:
    {
        char buf[64];
        MSGBUF( buf, "malloc(%d)", len);
        if(dev->batch)
            FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
        else
             WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
        goto cswp_error;
    }
cswp_error:
    if (l1) free(l1);
    return -1;
}

/*-------------------------------------------------------------------*/
/* UNCONDITIONAL endian swap of compressed device header fields      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd64_swapend_chdr( CCKD64_DEVHDR* cdevhdr )
{
    /* fix the compressed ckd header */
    cdevhdr->cdh_opts ^= CCKD_OPT_BIGEND;

    cdevhdr->num_L1tab    = SWAP32( cdevhdr->num_L1tab    );
    cdevhdr->num_L2tab    = SWAP32( cdevhdr->num_L2tab    );
    cdevhdr->cdh_size     = SWAP64( cdevhdr->cdh_size     );
    cdevhdr->cdh_used     = SWAP64( cdevhdr->cdh_used     );
    cdevhdr->free_off     = SWAP64( cdevhdr->free_off     );
    cdevhdr->free_total   = SWAP64( cdevhdr->free_total   );
    cdevhdr->free_largest = SWAP64( cdevhdr->free_largest );
    cdevhdr->free_num     = SWAP64( cdevhdr->free_num     );
    cdevhdr->free_imbed   = SWAP64( cdevhdr->free_imbed   );
    cdevhdr->cmp_parm     = SWAP16( cdevhdr->cmp_parm     );
}

/*-------------------------------------------------------------------*/
/* UNCONDITIONAL endian swap of level 1 table entries                */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd64_swapend_l1( CCKD64_L1ENT* l1, int num_L1tab )
{
    int  i;
    for (i = 0; i < num_L1tab; i++)
        l1[i] = SWAP64( l1[i] );
}

/*-------------------------------------------------------------------*/
/* UNCONDITIONAL endian swap of all level 2 table entries fields     */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd64_swapend_l2( CCKD64_L2ENT* l2 )
{
    int  i;
    for (i = 0; i < 256; i++)
    {
        l2[i].L2_trkoff = SWAP64( l2[i].L2_trkoff );
        l2[i].L2_len    = SWAP16( l2[i].L2_len    );
        l2[i].L2_size   = SWAP16( l2[i].L2_size   );
    }
}

/*-------------------------------------------------------------------*/
/* UNCONDITIONAL endian swap of all free space entry fields          */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd64_swapend_free( CCKD64_FREEBLK* fb )
{
    fb->fb_offnxt = SWAP64( fb->fb_offnxt );
    fb->fb_len    = SWAP64( fb->fb_len    );
}

/*-------------------------------------------------------------------
 * Remove all free space from a compressed ckd file
 *-------------------------------------------------------------------*/
DLL_EXPORT int cckd64_comp (DEVBLK *dev)
{
CCKD64_EXT     *cckd;                   /* -> cckd extension         */
int             fd;                     /* File descriptor           */
struct stat     fst;                    /* File status buffer        */
int             rc;                     /* Return code               */
U64             off;                    /* File offset               */
U64             l2area;                 /* Boundary for l2 tables    */
U64             len;                    /* Length                    */
int             i, j, l, n;             /* Work variables            */
BYTE            relocate = 0;           /* 1=spaces will be relocated*/
U64             l1size;                 /* l1 table size             */
U64             next;                   /* offset of next space      */
int             s;                      /* space table index         */
CKD_DEVHDR      devhdr;                 /* CKD device header         */
CCKD64_DEVHDR   cdevhdr;                /* CCKD device header        */
CCKD64_L1ENT   *l1=NULL;                /* ->L1tab table             */
CCKD64_L2ENT  **l2=NULL;                /* ->L2tab table array       */
SPCTAB64       *spctab=NULL;            /* -> space table            */
BYTE           *rbuf=NULL;              /* Relocation buffer         */
BYTE           *p;                      /* -> relocation buffer      */
U64             rlen=0;                 /* Relocation buffer length  */
CCKD64_L2ENT    zero_l2[256];           /* Empty l2 table (zeros)    */
CCKD64_L2ENT    ff_l2[256];             /* Empty l2 table (0xff's)   */
BYTE            buf[256*1024];          /* 256K Buffer               */

    if (!dev->cckd64)
        return cckd_comp( dev );

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
    if ((U64)(rc = read( fd, &devhdr, (unsigned int) len )) != len)
        goto comp_read_error;

    dev->cckd64 = 1;
    if (!(dh_devid_typ( devhdr.dh_devid ) & ANY64_CMP_OR_SF_TYP))
    {
        if (dh_devid_typ( devhdr.dh_devid ) & ANY32_CMP_OR_SF_TYP)
        {
            dev->cckd64 = 0;
            return cckd_comp( dev );
        }

        // "%1d:%04X CCKD file %s: not a compressed dasd file"
        if (dev->batch)
            FWRMSG( stdout, HHC00356, "E", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, dev->filename );
        else
            WRMSG( HHC00356, "E", LCSS_DEVNUM, dev->filename );
        goto comp_error;
    }

comp_restart:

    /*---------------------------------------------------------------
     * Read compressed device header
     *---------------------------------------------------------------*/
    off = CCKD64_DEVHDR_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD64_DEVHDR_SIZE;
    if ((U64)(rc = read (fd, &cdevhdr, (unsigned int) len)) != len)
        goto comp_read_error;

    /*---------------------------------------------------------------
     * Check the endianness of the file
     *---------------------------------------------------------------*/
    if ((cdevhdr.cdh_opts & CCKD_OPT_BIGEND) != cckd_def_opt_bigend())
    {
        if(dev->batch)
            // "%1d:%04X CCKD file %s: converting to %s"
            FWRMSG( stdout, HHC00357, "I", LCSS_DEVNUM, dev->filename,
                    (cdevhdr.cdh_opts & CCKD_OPT_BIGEND) ?
                        "little-endian" : "big-endian" );
        else
            // "%1d:%04X CCKD file %s: converting to %s"
            WRMSG( HHC00357, "I", LCSS_DEVNUM, dev->filename,
                   (cdevhdr.cdh_opts & CCKD_OPT_BIGEND) ?
                    "little-endian" : "big-endian" );

        if (cckd64_swapend (dev) < 0)
            goto comp_error;
        else
            goto comp_restart;
    }

    /*---------------------------------------------------------------
     * Some header checks
     *---------------------------------------------------------------*/
    if (cdevhdr.cdh_size   != (U64)fst.st_size
     || cdevhdr.cdh_size   != cdevhdr.cdh_used || cdevhdr.free_off     != 0
     || cdevhdr.free_total != 0                || cdevhdr.free_largest != 0
     || cdevhdr.free_num   != 0                || cdevhdr.free_imbed   != 0)
        relocate = 1;

    /*---------------------------------------------------------------
     * Build empty l2 tables
     *---------------------------------------------------------------*/
    memset( &zero_l2, 0, CCKD64_L2TAB_SIZE );
    if (cdevhdr.cdh_nullfmt != 0)
        for (i = 0; i < 256; i++)
            zero_l2[i].L2_len = zero_l2[i].L2_size = cdevhdr.cdh_nullfmt;
    memset (&ff_l2, 0xff, CCKD64_L2TAB_SIZE);

    /*---------------------------------------------------------------
     * Read the l1 table
     *---------------------------------------------------------------*/
    l1size = len = cdevhdr.num_L1tab * CCKD64_L1ENT_SIZE;
    if ((l1 = malloc((size_t)len)) == NULL)
        goto comp_malloc_error;
    off = CCKD64_L1TAB_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((U64)(rc = read (fd, l1, (unsigned int) len)) != len)
        goto comp_read_error;

    /*---------------------------------------------------------------
     * Build the space table
     *---------------------------------------------------------------*/
    n = 1 + 1 + 1 + cdevhdr.num_L1tab + 1;
    for (i = 0; i < cdevhdr.num_L1tab; i++)
        if (l1[i] != CCKD64_NOSIZE && l1[i] != CCKD64_MAXSIZE)
            n += 256;
    len = sizeof(SPCTAB64);
    if ((spctab = calloc(n, (size_t)len)) == NULL)
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
    spctab[s].spc_off = CCKD64_DEVHDR_POS;
    spctab[s].spc_len =
    spctab[s].spc_siz = CCKD64_DEVHDR_SIZE;
    s++;
    spctab[s].spc_typ = SPCTAB_L1;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = CCKD64_L1TAB_POS;
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
        if (l1[i] != CCKD64_NOSIZE && l1[i] != CCKD64_MAXSIZE)
        {
            spctab[s].spc_typ = SPCTAB_L2;
            spctab[s].spc_val = i;
            spctab[s].spc_off = l1[i];
            spctab[s].spc_len =
            spctab[s].spc_siz = CCKD64_L2TAB_SIZE;
            s++;
        }
    qsort (spctab, s, sizeof(SPCTAB64), comp_spctab64_sort);

    /*---------------------------------------------------------------
     * Read level 2 tables
     *---------------------------------------------------------------*/
    n = cdevhdr.num_L1tab;
    len = sizeof (void *);
    if ((l2 = calloc(n, (size_t)len)) == NULL)
        goto comp_calloc_error;
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
    {
        if (spctab[i].spc_typ != SPCTAB_L2) continue;
        l = spctab[i].spc_val;
        len = CCKD64_L2TAB_SIZE;
        if ((l2[l] = malloc((size_t)len)) == NULL)
            goto comp_malloc_error;
        off = spctab[i].spc_off;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto comp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        if ((U64)(rc = read (fd, l2[l], (unsigned int) len)) != len)
            goto comp_read_error;
        for (j = 0; j < 256; j++)
        {
            if (l2[l][j].L2_trkoff == CCKD64_NOSIZE || l2[l][j].L2_trkoff == CCKD64_MAXSIZE)
                continue;
            spctab[s].spc_typ = SPCTAB_TRK;
            spctab[s].spc_val = spctab[i].spc_val*256 + j;
            spctab[s].spc_off = l2[l][j].L2_trkoff;
            spctab[s].spc_len = l2[l][j].L2_len;
            spctab[s].spc_siz = l2[l][j].L2_size;
            s++;
        } /* for each l2 entry */
        /* check if empty l2 table */
        if (memcmp (l2[l], &zero_l2, CCKD64_L2TAB_SIZE) == 0
         || memcmp (l2[l], &ff_l2,   CCKD64_L2TAB_SIZE) == 0)
        {
            l1[l] = l2[l][0].L2_trkoff; /* CCKD64_NOSIZE or CCKD64_MAXSIZE */
            spctab[i].spc_typ = SPCTAB_NONE;
            free (l2[l]);
            l2[l] = NULL;
            relocate = 1;
        }
    } /* for each space */
    qsort (spctab, s, sizeof(SPCTAB64), comp_spctab64_sort);
    while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;
    /* set relocate flag if last space is free space */
    if (spctab[s-2].spc_off + spctab[s-2].spc_len != spctab[s-1].spc_off)
        relocate = 1;

    /*---------------------------------------------------------------
     * relocate l2 tables in order
     *---------------------------------------------------------------*/

    /* determine l2 area */
    l2area = CCKD64_L1TAB_POS + l1size;
    for (i = 0; i < cdevhdr.num_L1tab; i++)
    {
        if (l1[i] == CCKD64_NOSIZE || l1[i] == CCKD64_MAXSIZE)
            continue;
        if (l1[i] != l2area)
            relocate = 1;
        l2area += CCKD64_L2TAB_SIZE;
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
                FWRMSG( stdout, HHC00358, "I", LCSS_DEVNUM, dev->filename );
            else
                WRMSG( HHC00358, "I", LCSS_DEVNUM, dev->filename );
            goto comp_return_ok;
        }
    }

    /* file will be updated */
    cdevhdr.cdh_opts |= CCKD_OPT_OPENRW;

    /* calculate track size within the l2 area */
    for (i = 0, rlen = 0; spctab[i].spc_off < l2area; i++)
        if (spctab[i].spc_typ == SPCTAB_TRK)
            rlen += sizeof(spctab[i].spc_val) + sizeof(spctab[i].spc_len)
                 +  spctab[i].spc_len;

    /* read any tracks in the l2area into rbuf */
    if ((len = rlen) > 0)
    {
        if ((rbuf = malloc((size_t)len)) == NULL)
            goto comp_malloc_error;
        for (i = 0, p = rbuf; spctab[i].spc_off < l2area; i++)
        {
            if (spctab[i].spc_typ != SPCTAB_TRK) continue;
            memcpy (p, &spctab[i].spc_val, sizeof(spctab[i].spc_val));
            p += sizeof(spctab[i].spc_val);
            memcpy (p, &spctab[i].spc_len, sizeof(spctab[i].spc_len));
            p += sizeof(spctab[i].spc_len);
            off = spctab[i].spc_off;
            if (lseek (fd, off, SEEK_SET) < 0)
                goto comp_lseek_error;
            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
            len = spctab[i].spc_len;
            if ((U64)(rc = read (fd, p, (unsigned int) len)) != len)
                goto comp_read_error;
            p += len;
            spctab[i].spc_typ = SPCTAB_NONE;
        } /* for each space in the l2 area */
        qsort (spctab, s, sizeof(SPCTAB64), comp_spctab64_sort);
        while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;
    } /* if any tracks to relocate */

    /* remove all l2 tables from the space table */
    for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        if (spctab[i].spc_typ == SPCTAB_L2)
            spctab[i].spc_typ = SPCTAB_NONE;
    qsort (spctab, s, sizeof(SPCTAB64), comp_spctab64_sort);
    while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;

    /* add all l2 tables at their ordered offsets */
    off = CCKD64_L1TAB_POS + l1size;
    for (i = 0; i < cdevhdr.num_L1tab; i++)
    {
        if (l1[i] == CCKD64_NOSIZE || l1[i] == CCKD64_MAXSIZE)
            continue;
        spctab[s].spc_typ = SPCTAB_L2;
        spctab[s].spc_val = i;
        spctab[s].spc_off = off;
        spctab[s].spc_len =
        spctab[s].spc_siz = CCKD64_L2TAB_SIZE;
        s++;
        off += CCKD64_L2TAB_SIZE;
    }
    qsort (spctab, s, sizeof(SPCTAB64), comp_spctab64_sort);
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
        off = spctab[i+1].spc_off;

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
        if ((U64)(rc = read (fd, buf, (unsigned int) len)) != len)
            goto comp_write_error;

        /* write the images */
        off = spctab[i].spc_off + spctab[i].spc_len;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto comp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        if ((U64)(rc = write (fd, buf, (unsigned int) len)) != len)
            goto comp_write_error;
    }

    /* adjust the size of the file */
    spctab[s-1].spc_off = spctab[s-2].spc_off + spctab[s-2].spc_len;

    /*---------------------------------------------------------------
     * Write spaces relocated from the l2area to the end of the file
     *---------------------------------------------------------------*/
    off = spctab[s-1].spc_off;
    p = rbuf;
    while (rlen)
    {
        spctab[s].spc_typ = SPCTAB_TRK;
        spctab[s].spc_off = off;
        memcpy (&spctab[s].spc_val, p, sizeof(spctab[s].spc_val));
        p += sizeof(spctab[s].spc_val);
        memcpy (&spctab[s].spc_len, p, sizeof(spctab[s].spc_len));
        spctab[s].spc_siz = spctab[s].spc_len;
        p += sizeof(spctab[s].spc_len);

        if (lseek (fd, off, SEEK_SET) < 0)
            goto comp_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = spctab[s].spc_len;
        if ((U64)(rc = write (fd, p, (unsigned int) len)) != len)
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
        qsort (spctab, s, sizeof(SPCTAB64), comp_spctab64_sort);
        spctab[s-1].spc_off = spctab[s-2].spc_off + spctab[s-2].spc_len;
    }

    /*---------------------------------------------------------------
     * Update the device header
     *---------------------------------------------------------------*/
    cdevhdr.cdh_size     =
    cdevhdr.cdh_used     = spctab[s-1].spc_off;

    cdevhdr.free_off     =
    cdevhdr.free_total   =
    cdevhdr.free_largest =
    cdevhdr.free_num     =
    cdevhdr.free_imbed   = 0;

    cdevhdr.cdh_vrm[0] = CCKD_VERSION;
    cdevhdr.cdh_vrm[1] = CCKD_RELEASE;
    cdevhdr.cdh_vrm[2] = CCKD_MODLVL;

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
            l2[l][j].L2_size = (U16) spctab[i].spc_len;
        }

    /*---------------------------------------------------------------
     * Write the cdevhdr, l1 table and l2 tables
     *---------------------------------------------------------------*/

    /* write cdevhdr */
    off = CCKD64_DEVHDR_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD64_DEVHDR_SIZE;
    if ((U64)(rc = write (fd, &cdevhdr, (unsigned int) len)) != len)
        goto comp_write_error;

    /* write l1 table */
    off = CCKD64_L1TAB_POS;
    if (lseek (fd, off, SEEK_SET) < 0)
        goto comp_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = l1size;
    if ((U64)(rc = write (fd, l1, (unsigned int) len)) != len)
        goto comp_write_error;

    /* write l2 tables */
    for (i = 0; i < cdevhdr.num_L1tab; i++)
        if (l1[i] != CCKD64_NOSIZE && l1[i] != CCKD64_MAXSIZE)
        {
            off = l1[i];
            if (lseek (fd, off, SEEK_SET) < 0)
                goto comp_lseek_error;
            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
            len = CCKD64_L2TAB_SIZE;
            if ((U64)(rc = write (fd, l2[i], (unsigned int) len)) != len)
                goto comp_lseek_error;
        }

    /* truncate the file */
    off = spctab[s-1].spc_off;
    if (off < (U64) fst.st_size)
    {
        VERIFY(!ftruncate (fd, off));
        if(dev->batch)
            FWRMSG( stdout, HHC00359, "I", LCSS_DEVNUM, dev->filename,
                    fst.st_size - off );
        else
            WRMSG( HHC00359, "I", LCSS_DEVNUM, dev->filename, fst.st_size - off );
    }
    else
    {
        if(dev->batch)
            FWRMSG( stdout, HHC00360, "I", LCSS_DEVNUM, dev->filename );
        else
            WRMSG( HHC00360, "I", LCSS_DEVNUM, dev->filename );
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
        FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                "fstat()", strerror( errno ));
    else
        WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
               "fstat()", strerror( errno ));
    goto comp_error;

comp_lseek_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "lseek()", off, strerror( errno ));
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
               "lseek()", off, strerror( errno ));
    goto comp_error;

comp_read_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "read()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
               "read()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    goto comp_error;

comp_write_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "write()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
               "write()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    goto comp_error;

comp_malloc_error:
    {
        char buf[64];
        MSGBUF( buf, "malloc(%"PRId64")", len);
        if(dev->batch)
            FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
        else
            WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
                   buf, strerror( errno ));
        goto comp_error;
    }
comp_calloc_error:
    {
        char buf[64];
        MSGBUF( buf, "calloc(%"PRId64")", n * len);
        if(dev->batch)
            FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
        else
            WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
                   buf, strerror( errno ));
        goto comp_error;
    }
comp_error:

    rc = -1;
    goto comp_return;

} /* cckd64_comp() */

/*-------------------------------------------------------------------
 * cckd64_comp() space table sort
 *-------------------------------------------------------------------*/
static int comp_spctab64_sort(const void *a, const void *b)
{
const SPCTAB64 *x = a, *y = b;

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
DLL_EXPORT int cckd64_chkdsk(DEVBLK *dev, int level)
{
CCKD64_EXT     *cckd;                   /* -> ckd extension          */
int             fd;                     /* file descriptor           */
struct stat     fst;                    /* file status information   */
int             fdflags;                /* file descriptor flags     */
U64             cckd_maxsize;           /* max cckd file size        */
int             ro;                     /* 1=file opened read-only   */
S64             f, i, j, l, n;          /* work integers             */
int             L1idx, l2x;             /* l1, l2 table indexes      */
U32             imgtyp;                 /* Dasd image type           */
BYTE            compmask[256];          /* compression byte mask
                                           00 - supported
                                           0x - valid, not supported
                                           ff - invalid              */
U64             off;                    /* file offset               */
U64             len;                    /* length to read            */
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
BYTE            trktyp;                 /* track type (TRK, BLKGRP)  */
BYTE            ckddasd=0;              /* 1=ckd                     */
BYTE            fbadasd=0;              /* 1= fba                    */
BYTE            shadow=0;               /* 0xff=shadow file          */
BYTE            hdrerr=0;               /* non-zero: header errors   */
BYTE            fsperr=0;               /* 1=rebuild free space      */
BYTE            comperrs=0;             /* 1=unsupported comp found  */
BYTE            recovery=0;             /* 1=perform track recovery  */
BYTE            valid;                  /* 1=valid trk recovered     */
int             l1size;                 /* size of l1 table          */
BYTE            swapend=0;              /* 1=call cckd_swapend       */
U64             lopos, hipos;           /* low/high file positions   */
int             pass;                   /* recovery pass number (fba)*/
int             s;                      /* space table index         */
SPCTAB64       *spctab=NULL;            /* -> space table            */
BYTE           *l2errs=NULL;            /* l2 error table            */
BYTE           *rcvtab=NULL;            /* recovered tracks          */
CKD_DEVHDR      devhdr;                 /* device header             */
CCKD64_DEVHDR   cdevhdr;                /* compressed device header  */
CCKD64_DEVHDR   cdevhdr2;               /* compressed device header 2*/
CCKD64_L1ENT   *l1=NULL;                /* -> level 1 table          */
CCKD64_L2ENT    l2ent;                  /* level 2 entry             */
CCKD64_L2ENT    l2tab[256];             /* level 2 table             */
CCKD64_L2ENT  **l2=NULL;                /* -> level 2 table array    */
CCKD64_L2ENT    empty_l2[256];          /* Empty l2 table            */
CCKD64_FREEBLK  freeblk;                /* free block                */
CCKD64_FREEBLK *fsp=NULL;               /* free blocks (new format)  */
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
    cckd_maxsize = CCKD64_MAXSIZE;
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
    if ((U64)(rc = read (fd, &devhdr, (unsigned int) len)) != len)
        goto cdsk_read_error;

    /* Device header checks */
    dev->cckd64 = 1;
    imgtyp = dh_devid_typ( devhdr.dh_devid );

         if (imgtyp & CKD64_CMP_OR_SF_TYP) ckddasd = 1;
    else if (imgtyp & FBA64_CMP_OR_SF_TYP) fbadasd = 1;
    else
    {
        if (imgtyp & ANY32_CMP_OR_SF_TYP)
        {
            dev->cckd64 = 0;
            return cckd_chkdsk( dev, level );
        }

        // "%1d:%04X CCKD file %s: not a compressed dasd file"
        if (dev->batch)
            FWRMSG( stdout, HHC00356, "E", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, dev->filename );
        else
            WRMSG( HHC00356, "E", LCSS_DEVNUM, dev->filename );
        goto cdsk_error;
    }

    if (imgtyp & ANY64_SF_TYP)
        shadow = 0xff;

    trktyp = ckddasd ? SPCTAB_TRK : SPCTAB_BLKGRP;

    /* Read the cckd device header */
    off = CCKD64_DEVHDR_POS;
    if ( lseek (fd, off, SEEK_SET) < 0)
        goto cdsk_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    len = CCKD64_DEVHDR_SIZE;
    if ((U64)(rc = read (fd, &cdevhdr, (unsigned int) len)) != len)
        goto cdsk_read_error;

    /* Endianess check */
    if ((cdevhdr.cdh_opts & CCKD_OPT_BIGEND) != cckd_def_opt_bigend())
    {
        if (!ro)
        {
            if(dev->batch)
                // "%1d:%04X CCKD file %s: converting to %s"
                FWRMSG( stdout, HHC00357, "I", LCSS_DEVNUM, dev->filename,
                        (cdevhdr.cdh_opts & CCKD_OPT_BIGEND) ?
                            "little-endian" : "big-endian" );
            else
                // "%1d:%04X CCKD file %s: converting to %s"
                WRMSG( HHC00357, "I", LCSS_DEVNUM, dev->filename,
                       (cdevhdr.cdh_opts & CCKD_OPT_BIGEND) ?
                        "little-endian" : "big-endian" );

            if (cckd64_swapend (dev) < 0)
                goto cdsk_error;

            if (level < 0)
                level = 0;

            swapend = 0;
        }
        else
            swapend = 1;

        cckd64_swapend_chdr (&cdevhdr);
    }

    /* ckd checks */
    if (ckddasd)
    {
        CKDDEV *ckd;

        FETCH_LE_FW( heads,  devhdr.dh_heads   );
        FETCH_LE_FW( cyls,  cdevhdr.cdh_cyls   );
        FETCH_LE_FW( trksz,  devhdr.dh_trksize );

        trks = heads * cyls;

        /* ckd dasd lookup */
        ckd = dasd_lookup (DASD_CKDDEV, NULL, devhdr.dh_devtyp, 0);
        if (ckd == NULL)
        {
            if(dev->batch)
                // "%1d:%04X CCKD file %s: dasd lookup error type %02X cylinders %d"
                FWRMSG( stdout, HHC00361, "E", LCSS_DEVNUM, dev->filename,
                        devhdr.dh_devtyp, cyls );
            else
                // "%1d:%04X CCKD file %s: dasd lookup error type %02X cylinders %d"
                WRMSG( HHC00361, "E", LCSS_DEVNUM, dev->filename,
                       devhdr.dh_devtyp, cyls );
             goto cdsk_error;
        }

        /* track size check */
        n = CKD_TRKHDR_SIZE
          + CKD_R0_SIZE + CKD_R0_DLEN  /* r0 length */
          + CKD_RECHDR_SIZE + ckd->r1  /* max data length */
          + CKD_ENDTRK_SIZE;
        n = ((n+511)/512)*512;
        if ((S64)trksz != n)
        {
            if(dev->batch)
                // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
                FWRMSG( stdout, HHC00362, "E", LCSS_DEVNUM, dev->filename,
                        "track size", (U64)trksz, n );
            else
                // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
                WRMSG( HHC00362, "E", LCSS_DEVNUM, dev->filename,
                       "track size", (U64)trksz, n );
             goto cdsk_error;
        }

        /* number of heads check */
        if (heads != ckd->heads)
        {
            if(dev->batch)
                // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
                FWRMSG( stdout, HHC00362, "E", LCSS_DEVNUM, dev->filename,
                        "number of heads", (U64)heads, (U64)ckd->heads );
            else
                // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
                WRMSG( HHC00362, "E", LCSS_DEVNUM, dev->filename,
                      "number of heads", (U64)heads, (U64)ckd->heads );
             goto cdsk_error;
        }
    } /* if (ckddasd) */

    /* fba checks */
    else
    {
        /* Note: cyls & heads are setup for ckd type hdr checks */
        FETCH_LE_FW( blks, cdevhdr.cdh_cyls );

        trks = blks / CFBA_BLKS_PER_GRP;
        if (   blks % CFBA_BLKS_PER_GRP ) trks++;

        trksz = CKD_TRKHDR_SIZE + CFBA_BLKGRP_SIZE;
        heads = 65536;

        cyls = trks / heads;
        if (   trks % heads) cyls++;
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
            // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
            FWRMSG( stdout, HHC00362, "E", LCSS_DEVNUM, dev->filename,
                    "num_L1tab", (S64)cdevhdr.num_L1tab, n );
        else
            // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
            WRMSG( HHC00362, "E", LCSS_DEVNUM, dev->filename,
                   "num_L1tab", (S64)cdevhdr.num_L1tab, n );
        goto cdsk_error;
    }
    l1size = cdevhdr.num_L1tab * CCKD64_L1ENT_SIZE;
    if (CCKD64_L1TAB_POS + l1size > fst.st_size)
    {
        if(dev->batch)
            // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
            FWRMSG( stdout, HHC00362, "E", LCSS_DEVNUM, dev->filename,
                    "file length to contain L1 table", fst.st_size, (S64)CCKD64_L1TAB_POS + l1size );
        else
            // "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
            WRMSG( HHC00362, "E", LCSS_DEVNUM, dev->filename,
                  "file length to contain L1 table", fst.st_size, (S64)CCKD64_L1TAB_POS + l1size );
        goto cdsk_error;
    }

    /* check level 2 if SPERRS bit on */
    if (!ro && level < 2 && (cdevhdr.cdh_opts & CCKD_OPT_SPERRS))
    {
        level = 2;
        // "%1d:%04X CCKD file %s: forcing check level %d"
        if(dev->batch)
            FWRMSG( stdout, HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
        else
            WRMSG( HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
    }

    /* cdevhdr inconsistencies check */
    hdrerr  = 0;
    hdrerr |= (U64)fst.st_size != cdevhdr.cdh_size && cdevhdr.cdh_size != cdevhdr.free_off   ? 0x0001 : 0;
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
        // "%1d:%04X CCKD file %s: cdevhdr inconsistencies found, code %4.4X"
        if(dev->batch)
            FWRMSG( stdout, HHC00363, "W", LCSS_DEVNUM, dev->filename, hdrerr );
        else
            WRMSG( HHC00363, "W", LCSS_DEVNUM, dev->filename, hdrerr );
        if (level < 1)
        {
            level = 1;
            // "%1d:%04X CCKD file %s: forcing check level %d"
            if(dev->batch)
                FWRMSG( stdout, HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
            else
                WRMSG( HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
        }
    }

    /* Additional checking if not properly closed */
    if (level < 1 && (cdevhdr.cdh_opts & CCKD_OPT_OPENED))
    {
        level = 1;
        // "%1d:%04X CCKD file %s: forcing check level %d"
        if(dev->batch)
            FWRMSG( stdout, HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
        else
            WRMSG( HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
    }

    /* Additional checking if last opened for read/write */
    if (level < 0 && (cdevhdr.cdh_opts & CCKD_OPT_OPENRW))
        level = 0;

    /* Set check level -1 */
    if (level == 0 && !dev->batch && !hdrerr
     && (cdevhdr.cdh_opts & (CCKD_OPT_OPENED | CCKD_OPT_SPERRS)) == 0
     && ((cdevhdr.cdh_opts & (CCKD_OPT_OPENRW)) == 0 || ro))
        level = -1;

    /* Build empty l2 table */
    memset (&empty_l2, shadow, CCKD64_L2TAB_SIZE);
    if (shadow == 0 && cdevhdr.cdh_nullfmt != 0)
        for (i = 0; i < 256; i++)
            empty_l2[i].L2_len = empty_l2[i].L2_size = cdevhdr.cdh_nullfmt;

    /*---------------------------------------------------------------
     * read the level 1 table
     *---------------------------------------------------------------*/
    len = l1size;
    if ((l1 = malloc((size_t)len)) == NULL)
        goto cdsk_error;
    off = CCKD64_L1TAB_POS;
    if ( lseek (fd, off, SEEK_SET) < 0)
        goto cdsk_lseek_error;
    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
    if ((U64)(rc = read (fd, l1, (unsigned int) len)) != len)
        goto cdsk_read_error;
    if (swapend) cckd64_swapend_l1 (l1, cdevhdr.num_L1tab);
    lopos = CCKD64_L1TAB_POS + l1size;

    /*---------------------------------------------------------------
     * initialize the space table
     *---------------------------------------------------------------*/

    /* find number of non-null l1 entries */
    for (i = n = 0; i < cdevhdr.num_L1tab; i++)
        if (l1[i] != CCKD64_NOSIZE && l1[i] != CCKD64_MAXSIZE)
            n++;

    if (level >= 4) n = cdevhdr.num_L1tab;

    /* calculate max possible space table entries */
    n = 1 + 1 + 1                    // devhdr, cdevhdr, l1tab
      + n                            // l2tabs
      + (n * 256)                    // trk/blk images
      + (1 + n + (n * 256) + 1)      // max possible free spaces
      + 1;                           // end-of-file

    /* obtain the space table */
    len = sizeof(SPCTAB64);
    if ((spctab = calloc((size_t)n, (size_t)len)) == NULL)
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
    spctab[s].spc_off = CCKD64_DEVHDR_POS;
    spctab[s].spc_len =
    spctab[s].spc_siz = CCKD64_DEVHDR_SIZE;
    s++;
    /* l1 table */
    spctab[s].spc_typ = SPCTAB_L1;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = CCKD64_L1TAB_POS;
    spctab[s].spc_len =
    spctab[s].spc_siz = l1size;
    s++;
    /* l2 tables */
    for (i = 0; i < cdevhdr.num_L1tab && level < 4; i++)
    {
        if (l1[i] == CCKD64_NOSIZE || l1[i] == CCKD64_MAXSIZE)
            continue;
        spctab[s].spc_typ = SPCTAB_L2;
        spctab[s].spc_val = (int) i;
        spctab[s].spc_off = l1[i];
        spctab[s].spc_len =
        spctab[s].spc_siz = CCKD64_L2TAB_SIZE;
        s++;
    }
    /* end-of-file */
    spctab[s].spc_typ = SPCTAB_EOF;
    spctab[s].spc_val = -1;
    spctab[s].spc_off = (U64)fst.st_size;
    spctab[s].spc_len =
    spctab[s].spc_siz = 0;
    s++;

    qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);

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
    if ((l2errs = calloc((size_t)n, (size_t)len)) == NULL)
        goto cdsk_calloc_error;

    n = trks;
    if ((rcvtab = calloc((size_t)n, (size_t)len)) == NULL)
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
        len = CCKD64_L2TAB_SIZE;
        if ((U64)(rc = read (fd, l2tab, (unsigned int) len)) != len)
            goto cdsk_read_error;
        if (swapend) cckd64_swapend_l2 (l2tab);

        /* add trks/blkgrps to the space table */
        for (j = 0; j < 256; j++)
        {
            if (l2tab[j].L2_trkoff != CCKD64_NOSIZE && l2tab[j].L2_trkoff != CCKD64_MAXSIZE)
            {
                spctab[s].spc_typ = trktyp;
                spctab[s].spc_val = (int)((S64)spctab[i].spc_val * 256 + j);
                spctab[s].spc_off = l2tab[j].L2_trkoff;
                spctab[s].spc_len = l2tab[j].L2_len;
                spctab[s].spc_siz = l2tab[j].L2_size;
                s++;
            }
        }
    }
    qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);

    /*---------------------------------------------------------------
     * Consistency checks.
     *
     * The space table is now populated with everything but free
     * space.  Therefore we can infer what the free space should
     * be (ie gaps between allocated spaces).
     *---------------------------------------------------------------*/

    lopos = CCKD64_L1TAB_POS + l1size;
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

    memset( &cdevhdr2, 0, CCKD64_DEVHDR_SIZE );
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
            if (cdevhdr2.free_largest < len)
                cdevhdr2.free_largest = len;
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
        valid = spctab[i].spc_off >= lopos
             && spctab[i].spc_off + spctab[i].spc_siz <= hipos;

        /* Overlap check */
        if ((S64)len < 0 || !valid)
        {
            char space1[32], space2[32];
            recovery = 1;

            /* issue error message */
            j = MSGBUF(space1, "%s", spc_typ_to_str( spctab[i].spc_typ ));
            if (spctab[i].spc_val >= 0)
                sprintf(space1+j, "[%d]", spctab[i].spc_val);
            j = MSGBUF(space2, "%s", spc_typ_to_str( spctab[i+1].spc_typ ));
            if (spctab[i+1].spc_val >= 0)
                sprintf(space2+j, "[%d]", spctab[i+1].spc_val);

            if (!valid)
            {
                if(dev->batch)
                    // "%1d:%04X CCKD file %s: %s offset 0x%16.16"PRIX64" len %"PRId64" is out of bounds"
                    FWRMSG( stdout, HHC00365, "W", LCSS_DEVNUM, dev->filename,
                            space1, spctab[i].spc_off, spctab[i].spc_siz );
                else
                    // "%1d:%04X CCKD file %s: %s offset 0x%16.16"PRIX64" len %"PRId64" is out of bounds"
                    WRMSG( HHC00365, "W", LCSS_DEVNUM, dev->filename,
                           space1, spctab[i].spc_off, spctab[i].spc_siz );
            }
            else
            {
                if(dev->batch)
                    // "%1d:%04X CCKD file %s: %s offset 0x%16.16"PRIX64" len %"PRId64" overlaps %s offset 0x%"PRIX64
                    FWRMSG( stdout, HHC00366, "W", LCSS_DEVNUM, dev->filename,
                            space1, spctab[i].spc_off, spctab[i].spc_siz, space2, spctab[i+1].spc_off );
                else
                    // "%1d:%04X CCKD file %s: %s offset 0x%16.16"PRIX64" len %"PRId64" overlaps %s offset 0x%"PRIX64
                    WRMSG( HHC00366, "W", LCSS_DEVNUM, dev->filename,
                           space1, spctab[i].spc_off, spctab[i].spc_siz, space2, spctab[i+1].spc_off );
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
                // "%1d:%04X CCKD file %s: %s[%d] l2 inconsistency: len %"PRId64", size %"PRId64
                FWRMSG( stdout, HHC00367, "W", LCSS_DEVNUM, dev->filename,
                        spc_typ_to_str( trktyp ), spctab[i].spc_val, spctab[i].spc_len, spctab[i].spc_siz );
            else
                // "%1d:%04X CCKD file %s: %s[%d] l2 inconsistency: len %"PRId64", size %"PRId64
                WRMSG( HHC00367, "W", LCSS_DEVNUM, dev->filename,
                       spc_typ_to_str( trktyp ), spctab[i].spc_val, spctab[i].spc_len, spctab[i].spc_siz );

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
        // "%1d:%04X CCKD file %s: forcing check level %d"
        level = 3;
        if(dev->batch)
            FWRMSG( stdout, HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
        else
            WRMSG( HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
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

    lopos = CCKD64_L1TAB_POS + l1size;
    hipos = fst.st_size;

    if (level >= 1 && !fsperr)
    {
        while (cdevhdr.free_off) // `while' so code can break
        {
            fsperr = 1;  // be pessimistic
            fsp = NULL;

            /* Read the free space */
            off = cdevhdr.free_off;
            len = CCKD64_FREEBLK_SIZE;
            if (off < lopos || off + CCKD64_FREEBLK_SIZE > hipos
             || lseek (fd, off, SEEK_SET) < 0
             || (U64)(rc = read (fd, &freeblk, (unsigned int) len)) != len)
                break;

            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));

            if (memcmp (&freeblk, "FREE_BLK", 8) == 0)
            {
                /* new format free space */
                len = cdevhdr.free_num * CCKD64_FREEBLK_SIZE;
                if ((fsp = malloc((size_t)len)) == NULL
                 || (U64)(rc = read (fd, fsp, (unsigned int) len)) != len)
                    break;

                for (i = 0; i < cdevhdr.free_num; i++)
                {
                    if (swapend) cckd64_swapend_free (&fsp[i]);
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
                off = cdevhdr.free_off;
                len = CCKD64_FREEBLK_SIZE;
                for (i = 0; i < cdevhdr.free_num; i++)
                {
                    if (off < lopos || off > hipos) break;
                    if (lseek (fd, off, SEEK_SET) < 0)
                        goto cdsk_lseek_error;
                    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                    if ((U64)(rc = read (fd, &freeblk, (unsigned int) len)) != len)
                        goto cdsk_read_error;
                    if (swapend) cckd64_swapend_free (&freeblk);
                    spctab[s].spc_typ = SPCTAB_FREE;
                    spctab[s].spc_val = -1;
                    spctab[s].spc_off = off;
                    spctab[s].spc_len =
                    spctab[s].spc_siz = freeblk.fb_len;
                    s++;
                    lopos = off + freeblk.fb_len;
                    off = freeblk.fb_offnxt;
                }
                if (i >= cdevhdr.free_num && freeblk.fb_offnxt == 0)
                    fsperr = 0;
            } /* if old format free space */

            if (fsp) free(fsp);
            fsp = NULL;

            /* Check for gaps/overlaps */
            qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);
            for (i = 0; !fsperr && spctab[i].spc_typ != SPCTAB_EOF; i++)
                if (spctab[i].spc_off + spctab[i].spc_siz != spctab[i+1].spc_off)
                    fsperr = 1;
            break;
        } /* while (cdevhdr.free_off) */
    } /* if (level >= 1 && !fsperr) */

    if (fsperr)
    {
        // "%1d:%04X CCKD file %s: free space errors detected"
        if(dev->batch)
            FWRMSG( stdout, HHC00368, "W", LCSS_DEVNUM, dev->filename );
        else
            WRMSG( HHC00368, "W", LCSS_DEVNUM, dev->filename );
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
            if ((U64)(rc = read (fd, buf, (unsigned int) len)) != len)
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
                    FWRMSG( stdout, HHC00369, "W", LCSS_DEVNUM, dev->filename,
                            spc_typ_to_str( trktyp ), spctab[i].spc_val, off,
                            buf[0],buf[1],buf[2],buf[3],buf[4] );
                else
                    WRMSG( HHC00369, "W", LCSS_DEVNUM, dev->filename,
                           spc_typ_to_str( trktyp ), spctab[i].spc_val, off,
                           buf[0],buf[1],buf[2],buf[3],buf[4] );

                /* recover this track */
                rcvtab[spctab[i].spc_val] = recovery = 1;
                spctab[i].spc_typ = SPCTAB_NONE;

                /* Force level 3 checking */
                if (level < 3)
                {
                    level = 3;
                    // "%1d:%04X CCKD file %s: forcing check level %d"
                    if(dev->batch)
                        FWRMSG( stdout, HHC00364, "W", LCSS_DEVNUM,
                                dev->filename, level );
                    else
                        WRMSG( HHC00364, "W", LCSS_DEVNUM, dev->filename, level );
                    goto cdsk_space_check;
                }
                continue;
            } /* if invalid header info */

            /* Check if compression supported */
            if (compmask[comp])
            {
                comperrs = 1;
                if(dev->batch)
                    FWRMSG( stdout, HHC00370, "W", LCSS_DEVNUM, dev->filename,
                            spc_typ_to_str( trktyp ), trk, comp_to_str( compmask[ comp ]));
                else
                    WRMSG( HHC00370, "W", LCSS_DEVNUM, dev->filename,
                           spc_typ_to_str( trktyp ), trk, comp_to_str( compmask[ comp ]));
                continue;
            }

            /* Validate the space if check level 3 */
            if (level > 2)
            {
                if (!cdsk_valid_trk (trk, buf, heads, (int) len))
                {
                    if(dev->batch)
                        // "%1d:%04X CCKD file %s: %s[%d] offset 0x%16.16"PRIX64" len %"PRId64" validation error"
                        FWRMSG( stdout, HHC00371, "W", LCSS_DEVNUM, dev->filename,
                                spc_typ_to_str( trktyp ), trk, off, len );
                    else
                        // "%1d:%04X CCKD file %s: %s[%d] offset 0x%16.16"PRIX64" len %"PRId64" validation error"
                        WRMSG( HHC00371, "W", LCSS_DEVNUM, dev->filename,
                               spc_typ_to_str( trktyp ), trk, off, len );

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
     U64 flen, fpos;

        /*-----------------------------------------------------------
         * Phase 1 -- recover trk/blkgrp images
         *-----------------------------------------------------------*/
        /*
         * Reset the end-of-file pos to the file size
         * It might have been changed if new format free space
         * occurred at the end of the file.
         */
        qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);
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
            s = cdsk_build_free_space64 (spctab, s);
            for (f = 0; spctab[f].spc_typ != SPCTAB_EOF && n; )
            {
                /* next free space if too small */
                if (spctab[f].spc_typ != SPCTAB_FREE
                 || spctab[f].spc_siz <= CKD_MIN_TRKSIZE)
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
                off = fpos;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((U64)(rc = read (fd, buf, (unsigned int) len)) != len)
                    goto cdsk_read_error;

                /* Scan the space for a trkhdr */
                for (i = 0; i < (S64)(len - CKD_MIN_TRKSIZE); i++)
                {
                    CKD_TRKHDR  trkhdr;
                    CKD_R0      r0;

                    /* Check compression byte */
                    if (compmask[buf[i]])
                        continue;

                    /* Fetch possible trkhdr */
                    memcpy( &trkhdr, &buf[i], CKD_TRKHDR_SIZE );
                    comp =           trkhdr.bin;
                    cyl  = fetch_hw( trkhdr.cyl );
                    head = fetch_hw( trkhdr.head );
                    trk  = (cyl * heads) + head;

                    /* Validate possible trkhdr */
                    if (0
                        || cyl  >= cyls
                        || head >= heads
                        || rcvtab[trk] != 1
                    )
                        continue;

                    /* Quick validation for compress none */
                    if (comp == CCKD_COMPRESS_NONE)
                    {
                        memcpy( &r0, &buf[i+CKD_TRKHDR_SIZE], CKD_R0_SIZE );

                        if (0
                            || fetch_hw( r0.cyl  ) != cyl         // r0 cyl
                            || fetch_hw( r0.head ) != head        // r0 head
                            ||           r0.rec    != 0           // r0 record
                            ||           r0.klen   != 0           // r0 key length
                            || fetch_hw( r0.dlen ) != CKD_R0_DLEN // r0 data length
                        )
                            continue;
                    }

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
                    if (flen > len && i > (S64)(len - trksz))
                        break;

                    /* Checks for comp none */
                    if (comp == CCKD_COMPRESS_NONE)
                    {
                        l = len - i;
                        if ((l = cdsk_valid_trk (trk, buf+i, heads, (int) -l)))
                            goto cdsk_ckd_recover;
                        else
                             continue;
                    }

                    /* Check short `length' */
                    if (flen == len && (l = len - i) <= 1024)
                    {
                        if (cdsk_valid_trk (trk, buf+i, heads, (int) l))
                        {
                            while (cdsk_valid_trk (trk, buf+i, heads, (int) --l));
                            l++;
                            goto cdsk_ckd_recover;
                        }
                    }

                    /* Scan for next trkhdr */
                    for (j = i +          CKD_MIN_TRKSIZE;
                         j <= (S64)(len - CKD_MIN_TRKSIZE);
                         j++)
                    {
                        if (j - i > (S64) trksz)
                            break;

                        if (compmask[buf[j]] != 0)
                            continue;

                        memcpy( &trkhdr, &buf[j], CKD_TRKHDR_SIZE );
                        if (0
                            || fetch_hw( trkhdr.cyl  ) >= cyls
                            || fetch_hw( trkhdr.head ) >= heads
                        )
                            continue;

                        /* check uncompressed hdr */
                        if (comp == CCKD_COMPRESS_NONE)
                        {
                            memcpy( &r0, &buf[i+CKD_TRKHDR_SIZE], CKD_R0_SIZE );
                            if (0
                                || fetch_hw( r0.cyl  ) != fetch_hw( trkhdr.cyl  )
                                || fetch_hw( r0.head ) != fetch_hw( trkhdr.head )
                                ||           r0.rec    != 0
                                ||           r0.klen   != 0
                                || fetch_hw( r0.dlen ) != CKD_R0_DLEN
                            )
                                continue;
                        }

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
                        if (cdsk_valid_trk (trk, buf+i, heads, (int) l))
                        {
#if 0
                            while (cdsk_valid_trk (trk, buf+i, heads, (int) --l));
                            l++;
#endif
                            goto cdsk_ckd_recover;
                        }

                    } /* scan for next trkhdr */

                    /* Check `length' */
                    if (flen == len && (l = (S64)len - i) <= (S64)trksz)
                    {
                        if (cdsk_valid_trk (trk, buf+i, heads, (int) l))
                        {
                            while (cdsk_valid_trk (trk, buf+i, heads, (int) --l));
                            l++;
                            goto cdsk_ckd_recover;
                        }
                    }

                    /* Scan all lengths */
                    for (l = CKD_MIN_TRKSIZE; i + l <= (S64) len; l++)
                    {
                        if (l > (S64)trksz)
                            break;
                        if (cdsk_valid_trk (trk, buf+i, heads, (int) l))
                            goto cdsk_ckd_recover;
                    } /* for all lengths */

                    continue;

cdsk_ckd_recover:
                    if(dev->batch)
                        // "%1d:%04X CCKD file %s: %s[%d] recovered offset 0x%16.16"PRIX64" len %"PRId64
                        FWRMSG( stdout, HHC00372, "I", LCSS_DEVNUM, dev->filename,
                                spc_typ_to_str( trktyp ), trk, off + i, l );
                    else
                        // "%1d:%04X CCKD file %s: %s[%d] recovered offset 0x%16.16"PRIX64" len %"PRId64
                        WRMSG( HHC00372, "I", LCSS_DEVNUM, dev->filename,
                               spc_typ_to_str( trktyp ), trk, off + i, l );
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
         * a valid trkhdr (and don't occur too close to the beginning
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
            lopos = CCKD64_L1TAB_POS + (cdevhdr.num_L1tab * sizeof( CCKD64_L1ENT ));
            if (pass == 0)
                lopos += (cdevhdr.num_L1tab * CCKD64_L2TAB_SIZE);

            /* recovery loop */
            s = cdsk_build_free_space64 (spctab, s);
            for (f = 0; spctab[f].spc_typ != SPCTAB_EOF && n > 0; )
            {
                U64 flen, fpos;

                /* next free space if too small */
                if (spctab[f].spc_typ != SPCTAB_FREE
                 || spctab[f].spc_siz <= CKD_MIN_TRKSIZE
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
                off = fpos;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((U64)(rc = read (fd, buf, (unsigned int) len)) != len)
                    goto cdsk_read_error;

                /* Scan the space */
                for (i = 0; i < (S64)(len - CKD_MIN_TRKSIZE); i++)
                {
                    /* For pass 1 the size left must be at least blkgrpsz */
                    if (pass == 1 && len - i < (U64)blkgrpsz)
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
                     && flen == len && len - i < (U64)blkgrpsz)
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
                    if (flen > len && i > (S64)(len - blkgrpsz))
                        break;

                    /* Checks for comp none */
                    if (comp == CCKD_COMPRESS_NONE)
                    {
                        l = blkgrpsz;
                        if (len - i < (U64)blkgrpsz || fpos + i < lopos)
                            continue;
                        if (len - i == (U64)blkgrpsz && flen == len)
                            goto cdsk_fba_recover;
                        /* Pass 0 checks */
                        if (pass == 0
                         && (len - i - l < CKD_MIN_TRKSIZE
                          || compmask[buf[i+l]]
                          || fetch_fw (buf+i+l+1) >= (U32)blkgrps)
                           )
                            continue;
                        goto cdsk_fba_recover;
                    }

                    /* The tests below are for pass 0 only */
                    if (pass == 1)
                        continue;

                    /* Check short `length' */
                    if (flen == len && (l = len - i) <= 1024)
                    {
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, (int) l))
                        {
                            while (cdsk_valid_trk (blkgrp, buf+i, heads, (int) --l));
                            l++;
                            goto cdsk_fba_recover;
                        }
                    }

                    /* Scan for next trkhdr */
                    for (j = i +          CKD_MIN_TRKSIZE;
                         j <= (S64)(len - CKD_MIN_TRKSIZE);
                         j++)
                    {
                        if (j - i > (S64)blkgrpsz) break;

                        if (compmask[buf[j]] != 0
                         || fetch_fw(buf+j+1) >= (U32)blkgrps)
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
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, (int) l))
                        {
#if 0
                            while (cdsk_valid_trk (blkgrp, buf+i, heads, (int) --l));
                            l++;
#endif
                            goto cdsk_fba_recover;
                        }

                    } /* scan for next trkhdr */

                    /* Check `length' */
                    l = len - i;
                    if (flen == len && l <= (S64)blkgrpsz)
                    {
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, (int) l))
                        {
                            while (cdsk_valid_trk (blkgrp, buf+i, heads, (int) --l));
                            l++;
                            goto cdsk_fba_recover;
                        }
                    }

                    /* Scan all lengths */
                    for (l = CKD_MIN_TRKSIZE; i + l <= (S64)len; l++)
                    {
                        if (l > (S64)blkgrpsz)
                            break;
                        if (cdsk_valid_trk (blkgrp, buf+i, heads, (int) l))
                            goto cdsk_fba_recover;
                    } /* for all lengths */

                    continue;

cdsk_fba_recover:
                    if(dev->batch)
                        // "%1d:%04X CCKD file %s: %s[%d] recovered offset 0x%16.16"PRIX64" len %"PRId64

                        FWRMSG( stdout, HHC00372, "I", LCSS_DEVNUM, dev->filename,
                                spc_typ_to_str( trktyp ), blkgrp, off + i, l );
                    else
                        // "%1d:%04X CCKD file %s: %s[%d] recovered offset 0x%16.16"PRIX64" len %"PRId64
                        WRMSG( HHC00372, "I", LCSS_DEVNUM, dev->filename,
                               spc_typ_to_str( trktyp ), blkgrp, off + i, l );
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
            // "%1d:%04X CCKD file %s: %"PRId64" %s images recovered"
            FWRMSG( stdout, HHC00373, "I", LCSS_DEVNUM, dev->filename,
                    n, spc_typ_to_str( trktyp ));
        else
            // "%1d:%04X CCKD file %s: %"PRId64" %s images recovered"
            WRMSG( HHC00373, "I", LCSS_DEVNUM, dev->filename,
                   n, spc_typ_to_str( trktyp ));

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
        if ((l2 = calloc((size_t)n, (size_t)len)) == NULL)
            goto cdsk_calloc_error;

        /* Get storage for the rebuilt l2 tables */
        len = CCKD64_L2TAB_SIZE;
        for (i = 0; i < trks; i++)
        {
            L1idx = (int)(i / 256);
            if (rcvtab[i] != 0 && l2[L1idx] == NULL)
            {
                if ((l2[L1idx] = malloc((size_t)len)) == NULL)
                    goto cdsk_malloc_error;
                l1[L1idx] = shadow ? CCKD64_SHADOW_NO_OFFSET : CCKD64_BASE_NO_OFFSET;
                memcpy( l2[L1idx], &empty_l2, (size_t)len );
            }
        }

        /* Rebuild the l2 tables */
        qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        {
            if (spctab[i].spc_typ == SPCTAB_L2 && l2[spctab[i].spc_val])
                spctab[i].spc_typ = SPCTAB_NONE;
            else if (spctab[i].spc_typ == trktyp && l2[spctab[i].spc_val/256])
            {
                L1idx = spctab[i].spc_val / 256;
                l2x = spctab[i].spc_val % 256;
                l2[L1idx][l2x].L2_trkoff = spctab[i].spc_off;
                l2[L1idx][l2x].L2_len    = (U16) spctab[i].spc_len;
                l2[L1idx][l2x].L2_size   = (U16) spctab[i].spc_siz;
            }
        } /* for each space */
        qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);
        while (spctab[s-1].spc_typ == SPCTAB_NONE) s--;

        /* Look for empty l2 tables */
        for (i = 0; i < cdevhdr.num_L1tab; i++)
            if (l2[i] != NULL
             && memcmp (l2[i], &empty_l2, CCKD64_L2TAB_SIZE) == 0)
            {
                free (l2[i]);
                l2[i] = NULL;
            }
        /*
         * `s-1' indexes the SPCTAB_EOF space table entry.
         * Set its `pos' to the maximum allowed value to ensure
         * there will be free space for the rebuilt l2 tables.
         */
        spctab[s-1].spc_off = cckd_maxsize;

        /* Build the free space */
        s = cdsk_build_free_space64 (spctab, s);

        /* Find space for the rebuilt l2 tables */
        for (i = j = 0; i < cdevhdr.num_L1tab; i++)
        {
            if (l2[i] == NULL) continue;

            /* find a free space */
            for ( ; spctab[j].spc_typ != SPCTAB_EOF; j++)
                if (spctab[j].spc_typ == SPCTAB_FREE
                 && spctab[j].spc_siz >= CCKD64_L2TAB_SIZE)
                     break;

            /* weird error if no space */
            if (spctab[j].spc_typ == SPCTAB_EOF)
            {
                if(dev->batch)
                    FWRMSG( stdout, HHC00374, "E", LCSS_DEVNUM, dev->filename );
                else
                    WRMSG( HHC00374, "E", LCSS_DEVNUM, dev->filename );
                goto cdsk_error;
            }

            /* add l2 space */
            l1[i]             = spctab[j].spc_off;
            spctab[s].spc_typ = SPCTAB_L2;
            spctab[s].spc_val = (int) i;
            spctab[s].spc_off = spctab[j].spc_off;
            spctab[s].spc_len =
            spctab[s].spc_siz = CCKD64_L2TAB_SIZE;
            s++;

            /* adjust the free space */
            spctab[j].spc_off += CCKD64_L2TAB_SIZE;
            spctab[j].spc_len -= CCKD64_L2TAB_SIZE;
            spctab[j].spc_siz -= CCKD64_L2TAB_SIZE;
        } /* for each l2 table */


        /*-----------------------------------------------------------
         * Phase 3 -- write l1 and l2 tables
         *-----------------------------------------------------------*/

        if (ro)
        {
            if(dev->batch)
                FWRMSG( stdout, HHC00375, "W", LCSS_DEVNUM, dev->filename,
                        "file opened read-only" );
            else
                WRMSG( HHC00375, "W", LCSS_DEVNUM, dev->filename,
                       "file opened read-only" );
              goto cdsk_error;
        }
        if (comperrs)
        {
            if(dev->batch)
                FWRMSG( stdout, HHC00375, "W", LCSS_DEVNUM, dev->filename,
                        "missing compression" );
            else
                WRMSG( HHC00375, "W", LCSS_DEVNUM, dev->filename,
                       "missing compression" );
            goto cdsk_error;
        }

        /* Write the l1 table */
        off = CCKD64_L1TAB_POS;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cdsk_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = l1size;
        if ((U64)(rc = write (fd, l1, (unsigned int) len)) != len)
            goto cdsk_write_error;

        /* Write l2 tables */
        qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);
        for (i = 0; spctab[i].spc_typ != SPCTAB_EOF; i++)
        {
            L1idx = spctab[i].spc_val;
            if (spctab[i].spc_typ != SPCTAB_L2 || l2[L1idx] == NULL)
                continue;
            off = l1[L1idx];
            if (lseek (fd, off, SEEK_SET) < 0)
                goto cdsk_lseek_error;
            gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
            len = CCKD64_L2TAB_SIZE;
            if ((U64)(rc = write (fd, l2[L1idx], (unsigned int) len)) != len)
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
              FWRMSG( stdout, HHC00376, "W", LCSS_DEVNUM, dev->filename );
          else
              WRMSG( HHC00376, "W", LCSS_DEVNUM, dev->filename );
    }
    else if (fsperr)
    {
        /*-----------------------------------------------------------
         * Phase 1 -- build the free space
         *            make sure the last space isn't free space and
         *            that each free space is long enough (8 bytes).
         *-----------------------------------------------------------*/

cdsk_fsperr_retry:

        s = cdsk_build_free_space64 (spctab, s);

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
             && spctab[i].spc_siz < CCKD64_FREEBLK_SIZE)
                break;
        if (spctab[i].spc_typ != SPCTAB_EOF)
        {
            /* Shift following space left */
            l = spctab[i++].spc_siz;
            while (spctab[i].spc_typ != SPCTAB_FREE && spctab[i].spc_typ != SPCTAB_EOF)
            {
                /* Read the space and write shifted to the left */
                off = spctab[i].spc_off;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                len = spctab[i].spc_siz;
                if ((U64)(rc = read (fd, buf, (unsigned int) len)) != len)
                    goto cdsk_read_error;
                off -= l;
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((U64)(rc = write (fd, buf, (unsigned int) len)) != len)
                    goto cdsk_write_error;
                spctab[i].spc_off -= l;

                /* Update the l2 or l1 table entry */
                if (spctab[i].spc_typ == trktyp)
                {
                    L1idx = spctab[i].spc_val/256;
                    l2x = spctab[i].spc_val%256;
                    off = l1[L1idx] + l2x * CCKD64_L2ENT_SIZE;
                    if (lseek (fd, off, SEEK_SET) < 0)
                        goto cdsk_lseek_error;
                    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                    len = CCKD64_L2ENT_SIZE;
                    if ((U64)(rc = read (fd, &l2ent, (unsigned int) len)) != len)
                        goto cdsk_read_error;
                    l2ent.L2_trkoff -= l;
                    if (lseek (fd, off, SEEK_SET) < 0)
                        goto cdsk_lseek_error;
                    gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                    if ((U64)(rc = write (fd, &l2ent, (unsigned int) len)) != len)
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

        cdevhdr.cdh_vrm[0] = CCKD_VERSION;
        cdevhdr.cdh_vrm[1] = CCKD_RELEASE;
        cdevhdr.cdh_vrm[2] = CCKD_MODLVL;

        cdevhdr.cdh_size     =
        cdevhdr.cdh_used     =
        cdevhdr.free_off     =
        cdevhdr.free_total   =
        cdevhdr.free_largest =
        cdevhdr.free_num     =
        cdevhdr.free_imbed   = 0;

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
                cdevhdr.cdh_size   += spctab[i].spc_siz;
                cdevhdr.cdh_used   += spctab[i].spc_len;
                cdevhdr.free_total += spctab[i].spc_siz - spctab[i].spc_len;
                cdevhdr.free_imbed += spctab[i].spc_siz - spctab[i].spc_len;
             }

        /*-----------------------------------------------------------
         * Phase 3 -- write the free space
         *-----------------------------------------------------------*/

        if (cdevhdr.free_num)
        {
            /* size needed for new format free space */
            len = (cdevhdr.free_num+1) * CCKD64_FREEBLK_SIZE;

            /* look for existing free space to fit new format free space */
            for (i = 0, off = 0; !off && spctab[i].spc_typ != SPCTAB_EOF; i++)
                if (spctab[i].spc_typ == SPCTAB_FREE && len <= spctab[i].spc_siz)
                    off = spctab[i].spc_off;

            /* if no applicable space see if we can append to the file */
            if (!off && cckd_maxsize - cdevhdr.cdh_size >= len)
                off = cdevhdr.cdh_size;

            /* get free space buffer */
            if (off && (fsp = malloc((size_t)len)) == NULL)
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
                if ((U64)(rc = write (fd, fsp, (unsigned int) len)) != len)
                    goto cdsk_write_error;
                cdevhdr.free_off = off;

                free (fsp);
                fsp = NULL;
            } /* new format free space */
            else
            {
                /* old format free space */
                len = CCKD64_FREEBLK_SIZE;
                for (i = 0; spctab[i].spc_typ != SPCTAB_FREE; i++);
                cdevhdr.free_off = spctab[i].spc_off;
                off = spctab[i].spc_off;
                freeblk.fb_offnxt = 0;
                freeblk.fb_len = spctab[i].spc_siz;
                for (i = i + 1; spctab[i].spc_typ != SPCTAB_EOF; i++)
                    if (spctab[i].spc_typ == SPCTAB_FREE)
                    {
                        freeblk.fb_offnxt = spctab[i].spc_off;
                        if (lseek (fd, off, SEEK_SET) < 0)
                            goto cdsk_lseek_error;
                        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                        if ((U64)write (fd, &freeblk, (unsigned int) len) != len)
                            goto cdsk_write_error;
                        off = spctab[i].spc_off;
                        freeblk.fb_offnxt = 0;
                        freeblk.fb_len = spctab[i].spc_len;
                    }
                if (lseek (fd, off, SEEK_SET) < 0)
                    goto cdsk_lseek_error;
                gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
                if ((U64)write (fd, &freeblk, (unsigned int) len) != len)
                    goto cdsk_write_error;
            } /* old format free space */
        } /* if (cdevhdr.free_num) */

        /* Write cdevhdr and l1 table */
        off = CCKD64_DEVHDR_POS;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cdsk_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = CCKD64_DEVHDR_SIZE;
        if ((U64)write (fd, &cdevhdr, (unsigned int) len) != len)
            goto cdsk_write_error;

        off = CCKD64_L1TAB_POS;
        if (lseek (fd, off, SEEK_SET) < 0)
            goto cdsk_lseek_error;
        gui_fprintf (stderr, "POS=%"PRIu64"\n", (U64) lseek( fd, 0, SEEK_CUR ));
        len = l1size;
        if ((U64)write (fd, l1, (unsigned int) len) != len)
            goto cdsk_write_error;

        /* Truncate the file */
        off = cdevhdr.cdh_size;
        if (cdevhdr.free_off == cdevhdr.cdh_size)
            off += (cdevhdr.free_num+1) * CCKD64_FREEBLK_SIZE;
        rc = ftruncate (fd, off);

        if(dev->batch)
            FWRMSG( stdout, HHC00377, "I", LCSS_DEVNUM, dev->filename );
        else
            WRMSG( HHC00377, "I", LCSS_DEVNUM, dev->filename );

    } /* if (fsperr) */

    /*---------------------------------------------------------------
     * Return
     *---------------------------------------------------------------*/

cdsk_return_ok:

    rc = recovery ? 2 : fsperr ? 1 : 0;

    if (!ro && (cdevhdr.cdh_opts & (CCKD_OPT_OPENRW | CCKD_OPT_OPENED | CCKD_OPT_SPERRS)))
    {
        cdevhdr.cdh_opts &= ~(CCKD_OPT_OPENED | CCKD_OPT_SPERRS);

        /* Set version.release.modlvl */
        cdevhdr.cdh_vrm[0] = CCKD_VERSION;
        cdevhdr.cdh_vrm[1] = CCKD_RELEASE;
        cdevhdr.cdh_vrm[2] = CCKD_MODLVL;

        off = CCKD64_DEVHDR_POS;
        if (lseek (fd, CCKD64_DEVHDR_POS, SEEK_SET) >= 0)
            VERIFY(CCKD64_DEVHDR_SIZE == write (fd, &cdevhdr, CCKD64_DEVHDR_SIZE));
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
        FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                "fstat()", strerror( errno ));
    else
        WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
               "fstat()", strerror( errno ));
    goto cdsk_error;

cdsk_lseek_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "lseek()", off, strerror (errno ));
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
               "lseek()", off, strerror( errno ));
    goto cdsk_error;

cdsk_read_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "read()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
               "read()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    goto cdsk_error;

cdsk_write_error:
    if(dev->batch)
        FWRMSG( stdout, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                "write()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    else
        WRMSG( HHC00355, "E", LCSS_DEVNUM, dev->filename,
               "write()", off, rc < 0 ? strerror( errno ) : "incomplete" );
    goto cdsk_error;

cdsk_malloc_error:
    {
        char buf[64];
        MSGBUF( buf, "malloc(%"PRId64")", len);
        if(dev->batch)
            FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
        else
             WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
    }
    goto cdsk_error;

cdsk_calloc_error:
    {
        char buf[64];
        MSGBUF( buf, "calloc(%"PRId64")", n * len);
        if(dev->batch)
            FWRMSG( stdout, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
        else
             WRMSG( HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    buf, strerror( errno ));
    }
    goto cdsk_error;

cdsk_error:
    rc = -1;
    goto cdsk_return;

} /* end function cckd64_chkdsk */

/*-------------------------------------------------------------------
 * cckd64_chkdsk() space table sort
 *-------------------------------------------------------------------*/
static int cdsk_spctab64_sort(const void *a, const void *b)
{
const SPCTAB64 *x = a, *y = b;

         if (x->spc_typ == SPCTAB_NONE) return +1;
    else if (y->spc_typ == SPCTAB_NONE) return -1;
    else if (x->spc_typ == SPCTAB_EOF)  return +1;
    else if (y->spc_typ == SPCTAB_EOF)  return -1;
    else if (x->spc_off < y->spc_off)   return -1;
    else                                return +1;
} /* end function cdsk_spctab64_sort */

/*-------------------------------------------------------------------*/
/* Build free space in the space table                               */
/*-------------------------------------------------------------------*/
static int cdsk_build_free_space64(SPCTAB64 *spctab, int s)
{
int i;

    for (i = 0; i < s; i++)
        if (spctab[i].spc_typ == SPCTAB_FREE)
            spctab[i].spc_typ = SPCTAB_NONE;
    qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);
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
    qsort (spctab, s, sizeof(SPCTAB64), cdsk_spctab64_sort);
    return s;
}
