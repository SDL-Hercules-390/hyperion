/* CCKDDIAG.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules CCKD diagnostic tool                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* 2003-02-07 James M. Morrison initial implementation               */
/* portions borrowed from cckdcdsk & other CCKD code                 */

/*-------------------------------------------------------------------*/
/* Diagnostic tool to display various CCKD data                      */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

/* TODO: add FBA support or write cfbadiag                           */

#include "hercules.h"
#include "dasdblks.h"

#define UTILITY_NAME    "cckddiag"

/*-------------------------------------------------------------------*/
/* Exit codes                                                        */
/*-------------------------------------------------------------------*/
#define EXIT_SEEK_ERROR         1
#define EXIT_READ_ERROR         2
#define EXIT_NO_FBA_SUPPORT     3
#define EXIT_MALLOC_FAILED      4
#define EXIT_NO_CKD_DASDTAB     5
#define EXIT_NO_FBA_DASDTAB     6

/*-------------------------------------------------------------------*/
/* CKD DASD record stats structure                                   */
/*-------------------------------------------------------------------*/
typedef struct _CKD_RECSTAT {     /* CKD DASD record stats           */
        int    cc;                /* CC cylinder # (relative zero)   */
        int    hh;                /* HH head # (relative zero)       */
        int    r;                 /* Record # (relative zero)        */
        int    kl;                /* key length                      */
        int    dl;                /* data length                     */
} CKD_RECSTAT;

/*-------------------------------------------------------------------*/
/* Global data areas                                                 */
/*-------------------------------------------------------------------*/
static  BYTE eighthexFF[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

static CCKD_L1ENT     *l1        = NULL;    /* L1TAB                 */
static CCKD_L2ENT     *l2        = NULL;    /* L2TAB                 */
static void           *tbuf      = NULL;    /* track header & data   */
static void           *bulk      = NULL;    /* bulk data buffer      */
static int             fd        = 0;       /* File descriptor       */
static int             debug     = 0;       /* disable debug code    */
static int             pausesnap = 0;       /* 1 = pause after snap  */

/*-------------------------------------------------------------------*/
/* print syntax                                                      */
/*-------------------------------------------------------------------*/
static int syntax(char *pgm)
{
    // "Usage: %s ...
    WRMSG( HHC02600, "I", pgm );
    return -1;
}

/*-------------------------------------------------------------------*/
/* clean - cleanup before exit                                       */
/*-------------------------------------------------------------------*/
static void clean()
{
    close(fd);
    free(l1);                          /* L1TAB buffer               */
    free(l2);                          /* L2TAB buffer               */
    free(tbuf);                        /* track and header buffer    */
    free(bulk);                        /* offset data buffer         */
}

/*-------------------------------------------------------------------*/
/* makbuf - allocate buffer, handle errors (exit if any)             */
/*-------------------------------------------------------------------*/
static void *makbuf(int len, char *label)
{
    void *p;

    if (!(p = malloc( len )))
    {
        // "From %s: Storage allocation of size %d using %s failed"
        FWRMSG( stderr, HHC02602, "S", label, len, "malloc()" );
        clean();
        exit( EXIT_MALLOC_FAILED );
    }
    if (debug)
    {
        // "MAKBUF() malloc %s buffer of %d bytes at %p"
        WRMSG( HHC90400, "D", label, len, p);
    }
    return p;
}

/*-------------------------------------------------------------------*/
/* readpos - common lseek and read invocation with error handling    */
/* This code exits on error rather than return to caller.            */
/*-------------------------------------------------------------------*/
static int readpos
(
    int           fd,           /* opened CCKD image file            */
    void*         buf,          /* buffer of size len                */
    off_t         offset,       /* read offset into CCKD image       */
    unsigned int  len           /* length of data to read            */
)
{
    int rc;

    if (debug)
    {
        // "READPOS seeking %d (0x%8.8X)"
        WRMSG( HHC90401, "D", (int) offset, (unsigned int) offset );
    }
    if (lseek( fd, offset, SEEK_SET ) < 0)
    {
        // "lseek() to pos 0x%08x error: %s"
        FWRMSG( stderr, HHC02603, "S",
            (unsigned int) offset, strerror( errno ));
        clean();
        exit( EXIT_SEEK_ERROR );
    }

    if (debug)
    {
        // "READPOS reading buf addr %p length %d (0x%X)"
        WRMSG( HHC90402, "D", buf, len, len);
    }

    rc = read( fd, buf, len);

    if (rc < (int) len )
    {
        if (rc < 0)
        {
            // "%s%s"
            FWRMSG( stderr, HHC02604, "S", "read() error: ", strerror( errno ));
        }
        else if (rc == 0)
        {
            // "%s%s"
            FWRMSG( stderr, HHC02604, "S", "read() error: ", "unexpected EOF" );
        }
        else
        {
            // "%s%s"
            FWRMSG( stderr, HHC02604, "S", "read() error: ", "short block" );
        }

        clean();
        exit( EXIT_READ_ERROR );
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* decomptrk - decompress track data                                 */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  ibuf      points at CKDDASD_TRKHDR header followed by track data */
/*  ibuflen   specifies length of TRKHDR and data                    */
/*                                                                   */
/*  Returns   length of decompressed data or -1 on error.            */
/*                                                                   */
/*  This code based on decompression logic in cdsk_valid_trk.        */
/*-------------------------------------------------------------------*/
static int decomptrk
(
    BYTE*  ibuf,                /* input buffer address              */
    int    ibuflen,             /* input buffer length               */
    BYTE*  obuf,                /* output buffer address             */
    int    obuflen,             /* output buffer length              */
    int    heads,               /* >=0 means CKD, else FBA           */
    int    trk,                 /* relative track or block number    */
    char*  emsg                 /* addr of 81 byte msg buf or NULL   */
)
{
#if defined( HAVE_LIBZ ) || defined( CCKD_BZIP2 )
    int             rc;         /* Return code                       */
#endif
    unsigned int    bufl;       /* Buffer length                     */
#ifdef CCKD_BZIP2
    unsigned int    ubufl;      /* when size_t != unsigned int       */
#endif

#if !defined( HAVE_LIBZ ) && !defined( CCKD_BZIP2 )
    UNREFERENCED(heads);
    UNREFERENCED(trk);
    UNREFERENCED(emsg);
#endif

    memset(obuf, 0, obuflen);

    /* Uncompress the track/block image */
    switch (ibuf[0] & CCKD_COMPRESS_MASK)
    {

    case CCKD_COMPRESS_NONE:
        bufl = (ibuflen < obuflen) ? ibuflen : obuflen;
        memcpy (obuf, ibuf, bufl);
        break;

#ifdef HAVE_LIBZ
    case CCKD_COMPRESS_ZLIB:
        memcpy (obuf, ibuf, CKDDASD_TRKHDR_SIZE);
        bufl = obuflen - CKDDASD_TRKHDR_SIZE;
        rc = uncompress(&obuf[CKDDASD_TRKHDR_SIZE],
                         (void *)&bufl,
                         &ibuf[CKDDASD_TRKHDR_SIZE],
                         ibuflen);
        if (rc != Z_OK)
        {
            if (emsg)
            {
                char msg[81];

                MSGBUF(msg, "%s %d uncompress error, rc=%d;"
                         "%2.2x%2.2x%2.2x%2.2x%2.2x",
                         heads >= 0 ? "trk" : "blk", trk, rc,
                         ibuf[0], ibuf[1], ibuf[2], ibuf[3], ibuf[4]);
                memcpy(emsg, msg, 81);
            }
            return -1;
        }
        bufl += CKDDASD_TRKHDR_SIZE;
        break;
#endif

#ifdef CCKD_BZIP2
    case CCKD_COMPRESS_BZIP2:
        memcpy(obuf, ibuf, CKDDASD_TRKHDR_SIZE);
        ubufl = obuflen - CKDDASD_TRKHDR_SIZE;
        rc = BZ2_bzBuffToBuffDecompress
        (
            (char *)&obuf[CKDDASD_TRKHDR_SIZE],
            &ubufl,
            (char *)&ibuf[CKDDASD_TRKHDR_SIZE],
            ibuflen, 0, 0
        );
        if (rc != BZ_OK)
        {
            if (emsg)
            {
                char msg[81];

                MSGBUF(msg, "%s %d decompress error, rc=%d;"
                         "%2.2x%2.2x%2.2x%2.2x%2.2x",
                         heads >= 0 ? "trk" : "blk", trk, rc,
                         ibuf[0], ibuf[1], ibuf[2], ibuf[3], ibuf[4]);
                memcpy(emsg, msg, 81);
            }
            return -1;
        }
        bufl=ubufl;
        bufl += CKDDASD_TRKHDR_SIZE;
        break;
#endif

    default:
        return -1;

    } /* switch (buf[0] & CCKD_COMPRESS_MASK) */
    return bufl;
}

/*-------------------------------------------------------------------*/
/* show_ckd_count - display CKD dasd record COUNT field              */
/* RECHDR is stored in big-endian byte order.                        */
/*-------------------------------------------------------------------*/
static BYTE *show_ckd_count(CKDDASD_RECHDR *rh, int trk)
{
int     cc, hh, r, kl, dl;
BYTE    *past;

    cc = (rh->cyl[0] << 8) | (rh->cyl[1]);
    hh = (rh->head[0] << 8) | (rh->head[1]);
    r  = rh->rec;
    kl = rh->klen;
    dl = (rh->dlen[0] << 8) | (rh->dlen[1]);

    // "Track %d COUNT cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] kl[%d] dl[%d]"
    WRMSG( HHC02605, "I", trk, cc, cc, hh, hh, r, r, kl, dl );

    past = (BYTE *)rh + sizeof(CKDDASD_RECHDR);
    return past;
}

/*-------------------------------------------------------------------*/
/* show_ckd_key - display CKD dasd record KEY field                  */
/*-------------------------------------------------------------------*/
static BYTE *show_ckd_key(CKDDASD_RECHDR *rh, BYTE *buf, int trk, int hexdump)
{
    if (hexdump && rh->klen)
    {
        // "Track %d rec[%02X/%d] %s[%d]:"
        printf("\n");
        WRMSG( HHC02606, "I", trk, rh->rec, rh->rec, "kl", rh->klen );
        data_dump( buf, rh->klen );
        printf("\n");
    }
    return (BYTE *)buf + rh->klen;   /* skip past KEY field */
}

/*-------------------------------------------------------------------*/
/* show_ckd_data - display CKD dasd record DATA field                */
/*-------------------------------------------------------------------*/
static BYTE *show_ckd_data(CKDDASD_RECHDR *rh, BYTE *buf, int trk, int hexdump)
{
    int dl;

    dl = (rh->dlen[0] << 8) | (rh->dlen[1]);

    if (hexdump && dl)
    {
        // "Track %d rec[%02X/%d] %s[%d]:"
        printf("\n");
        WRMSG( HHC02606, "I", trk, rh->rec, rh->rec, "dl", dl );
        data_dump( buf, dl );
        printf("\n");
    }
    return buf + dl;   /* skip past DATA field */
}

/*-------------------------------------------------------------------*/
/* snap - display msg, dump data                                     */
/*-------------------------------------------------------------------*/
static void snap( int comp, void *data, int len )
{
    if (comp)
    {
        // "SHOWTRK Compressed track header and data:"
        WRMSG( HHC90403, "D" );
        data_dump( data, len );
    }
    else
    {
        // "SHOWTRK Decompressed track header and data:"
        WRMSG( HHC90404, "D" );
        data_dump( data, len );
    }

    if (pausesnap)
    {
        // "%s"
        printf("\n");
        WRMSG( HHC02601, "A", "Press enter to continue" );
        getc( stdin );
    }
}

/*-------------------------------------------------------------------*/
/* showtrk - display track data                                      */
/* This code mimics selected logic in cdsk_valid_trk.                */
/*-------------------------------------------------------------------*/
static void showtrk
(
    CKDDASD_TRKHDR*  buf,   /* track header ptr                      */
    int  imglen,            /* TRKHDR + track user data length       */
    int  trk,               /* relative track number                 */
    int  hexdump            /* 1=dump key & data blks; 0=don't       */
)
{
    BYTE             buf2[64*1024];     /* max uncompressed buffer   */
    char             msg[81];           /* error message buffer      */
    CKDDASD_RECHDR*  rh;                /* CCKD COUNT field          */
    BYTE*            bufp;
    int              len;

    if (debug)
    {
        // dump compressed block
        snap( 1, buf, imglen );
        printf("\n");
    }

    len = decomptrk
    (
        (BYTE *)buf,        /* input buffer address            */
        imglen,             /* input buffer length             */
        buf2,               /* output buffer address           */
        sizeof(buf2),       /* output buffer length            */
        1,                  /* >=0 means CKD, else FBA         */
        trk,                /* relative track or block number  */
        msg                 /* addr of message buffer          */
    );

    if (debug)
    {
        snap( 0, buf2, len );
        printf("\n");
    }

    bufp = &buf2[sizeof(CKDDASD_TRKHDR)];

    while (bufp < &buf2[sizeof(buf2)])
    {
        rh = (CKDDASD_RECHDR *)bufp;

        if (memcmp((BYTE *)rh, &eighthexFF, 8) == 0)
        {
            // "%s"
            WRMSG( HHC02601, "I", "End of track" );
            break;
        }

        bufp = show_ckd_count ( rh, trk);
        bufp = show_ckd_key   ( rh, bufp, trk, hexdump );
        bufp = show_ckd_data  ( rh, bufp, trk, hexdump );
    }
}

/*-------------------------------------------------------------------*/
/* offtify - given decimal or hex input string, return off_t         */
/*-------------------------------------------------------------------*/
/* Locale independent, does not check for overflow                   */
/* References <ctype.h> and <string.h>                               */
/* Based on code in P. J. Plauger's "The Standard C Library"         */
/* See page 34, in Chapter 2 (ctype.h)                               */
/*-------------------------------------------------------------------*/
static off_t offtify(char *s)
{
    static const char  xd[]  = { "0123456789abcdefABCDEF" };
    static const char  xv[]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                10, 11, 12, 13, 14, 15,
                                10, 11, 12, 13, 14, 15 };
    off_t   v;
    char*   p;

    p = s;
    if ( (*s == '0') && (*(s+1) == 'x') )
    {
        s = s + 2;

        for (v = 0; isxdigit(*s); ++s)
            v = (v << 4) + xv[strchr(xd, *s) - xd];

        if (debug)
        {
            // "OFFTIFY hex string '%s' = 0x%16.16"PRIX64", dec %"PRId64"."
            WRMSG( HHC90405, "D", p, (U64) v, (U64) v );
        }
        return v;
    }
    else /* decimal input */
    {
        v = (off_t) atoll(s);

        if (debug)
        {
            // "OFFTIFY dec string '%s' = 0x%16.16"PRIX64", dec %"PRId64"."
            WRMSG( HHC90406, "D", p, (U64) v, (U64) v );
        }
        return v;
    }
}

/*-------------------------------------------------------------------*/
/* Main function for CCKD diagnostics                                */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
int             cckd_diag_rc = 0;       /* Program return code       */

char*           pgm;                    /* less any extension (.ext) */
char*           fn;                     /* File name                 */

CKDDASD_DEVHDR  devhdr;                 /* [C]CKD device hdr         */
CCKDDASD_DEVHDR cdevhdr;                /* Compressed CKD device hdr */

CKDDEV*         ckd          = NULL;    /* CKD DASD table entry      */
FBADEV*         fba          = NULL;    /* FBA DASD table entry      */

int             cmd_devhdr   = 0;       /* display DEVHDR            */
int             cmd_cdevhdr  = 0;       /* display CDEVHDR           */
int             cmd_l1tab    = 0;       /* display L1TAB             */
int             cmd_l2tab    = 0;       /* display L2TAB             */
int             cmd_trkdata  = 0;       /* display track data        */
int             cmd_hexdump  = 0;       /* display track data (hex)  */

int             cmd_offset   = 0;       /* 1 = display data at       */
int             op_offset    = 0;       /* op_offset of length       */
int             op_length    = 0;       /* op_length                 */

int             cmd_cchh     = 0;       /* 1 = display CCHH data     */
int             op_cc        = 0;       /* CC = cylinder             */
int             op_hh        = 0;       /* HH = head                 */

int             cmd_tt       = 0;       /* 1 = display TT data       */
int             op_tt        = 0;       /* relative track #          */

int             heads        = 0;       /* Heads per cylinder        */
int             trk          = 0;
int             imglen       = 0;       /* track length              */

int             l1ndx        = 0;
int             l2ndx        = 0;

off_t           l2taboff     = 0;       /* offset to assoc. L2 table */
off_t           trkhdroff    = 0;       /* offset to assoc. trk hdr  */

int             ckddasd;                /* 1=CKD dasd  0=FBA dasd    */
int             swapend;                /* 1 = New endianess doesn't
                                             match machine endianess */
int             n;                      /* work                      */
char            pathname[MAX_PATH];     /* file path in host format  */

    INITIALIZE_UTILITY( UTILITY_NAME, "CCKD diagnostic program", &pgm );

    /* parse the arguments */

    argc--; argv++;

    while (argc > 0)
    {
        if (**argv != '-')
            break;

        switch(argv[0][1])
        {
            case 'v':  if (argv[0][2] != '\0') return syntax (pgm);
                       return 0;

            case 'd':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_devhdr = 1;
                       break;

            case 'c':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_cdevhdr = 1;
                       break;

            case '1':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_l1tab = 1;
                       break;

            case '2':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_l2tab = 1;
                       break;

            case 'a':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_cchh = 1;

                       argc--; argv++;
                       op_cc = offtify(*argv);

                       argc--; argv++;
                       op_hh = offtify(*argv);
                       break;

            case 'r':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_tt = 1;

                       argc--; argv++;
                       op_tt = offtify(*argv);
                       break;

            case 'o':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_offset = 1;

                       argc--; argv++;
                       op_offset = offtify(*argv);

                       argc--; argv++;
                       op_length = offtify(*argv);
                       break;

            case 't':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_trkdata = 1;
                       break;

            case 'x':  if (argv[0][2] != '\0') return syntax (pgm);
                       cmd_hexdump = 1;
                       cmd_trkdata = 1;
                       break;

            case 'g':  if (argv[0][2] != '\0') return syntax (pgm);
                       debug = 1;
                       break;

            default:   return syntax (pgm);
        }

        argc--; argv++;
    }

    if (argc != 1)
        return syntax (pgm);

    fn = argv[0];

    /* open the file */
    hostpath( pathname, fn, sizeof( pathname ));
    fd = HOPEN( pathname, O_RDONLY | O_BINARY );

    if (fd < 0)
    {
        // "error opening file %s: %s"
        FWRMSG( stderr, HHC02607, "E", fn, strerror( errno ));
        return -1;
    }

    /*---------------------------------------------------------------*/
    /* display DEVHDR - first 512 bytes of dasd image                */
    /*---------------------------------------------------------------*/
    readpos(fd, &devhdr, 0, sizeof(devhdr));

    if (cmd_devhdr)
    {
        // "%s - %d (decimal) bytes:"
        printf("\n");
        WRMSG( HHC02614, "I", "DEVHDR", (int) sizeof( devhdr ));
        data_dump( &devhdr, sizeof( devhdr ));
        printf("\n");
    }

    /*---------------------------------------------------------------*/
    /* Determine CKD or FBA device type                              */
    /*---------------------------------------------------------------*/
    if (0
        || memcmp( devhdr.devid, "CKD_C370", 8) == 0
        || memcmp( devhdr.devid, "CKD_S370", 8) == 0
    )
    {
        ckddasd = 1;

        ckd = dasd_lookup(DASD_CKDDEV, NULL, devhdr.devtype, 0);

        if (!ckd)
        {
            // "DASD table entry not found for devtype 0x%2.2X"
            FWRMSG( stderr, HHC02608, "S", devhdr.devtype );
            clean();
            exit( EXIT_NO_CKD_DASDTAB );
        }
    }
    else
    {
        if (0
            || memcmp( devhdr.devid, "FBA_C370", 8) == 0
            || memcmp( devhdr.devid, "FBA_S370", 8) == 0
        )
        {
            ckddasd = 0;

            fba = dasd_lookup(DASD_FBADEV, NULL, devhdr.devtype, 0);

            if (!fba)
            {
                // "DASD table entry not found for devtype 0x%2.2X"
                FWRMSG( stderr, HHC02608, "S", DEFAULT_FBA_TYPE );
                clean();
                exit( EXIT_NO_FBA_DASDTAB );
            }
        }
        else
        {
            // "%s%s"
            FWRMSG( stderr, HHC02604, "E", "incorrect header id", "" );
            clean();
            return -1;
        }
    }

    /*---------------------------------------------------------------*/
    /* Set up device characteristics                                 */
    /*---------------------------------------------------------------*/
    if (ckddasd)
    {
        heads = ((U32)(devhdr.heads[3]) << 24)
              | ((U32)(devhdr.heads[2]) << 16)
              | ((U32)(devhdr.heads[1]) << 8)
              | (U32)(devhdr.heads[0]);

        if (debug)
        {
            // "%s device has %d heads/cylinder"
            WRMSG( HHC90407, "D", ckd->name, heads);
        }
    }

    /*---------------------------------------------------------------*/
    /* display CDEVHDR - follows DEVHDR                              */
    /*---------------------------------------------------------------*/
    readpos(fd, &cdevhdr, CKDDASD_DEVHDR_SIZE, sizeof(cdevhdr));

    if (cmd_cdevhdr)
    {
        // "%s - %d (decimal) bytes:"
        printf("\n");
        WRMSG( HHC02614, "I", "CDEVHDR", (int) sizeof( cdevhdr ));
        data_dump( &cdevhdr, sizeof( cdevhdr ));
        printf("\n");
    }

    /*---------------------------------------------------------------*/
    /* Find machine endian-ness                                      */
    /* cckd_endian() returns 1 for big-endian machines               */
    /*---------------------------------------------------------------*/
    swapend = (cckd_endian() !=
               ((cdevhdr.options & CCKD_BIGENDIAN) != 0));

    /*---------------------------------------------------------------*/
    /* display L1TAB - follows CDEVHDR                               */
    /*---------------------------------------------------------------*/
    /* swap numl1tab if needed */
    n = cdevhdr.numl1tab;

    if (swapend)
        cckd_swapend4((char *)&n);

    l1 = makbuf(n * CCKD_L1ENT_SIZE, "L1TAB");

    readpos(fd, l1, CCKD_L1TAB_POS, n * CCKD_L1ENT_SIZE);

    /* L1TAB itself is NOT adjusted for endian-ness */
    if (cmd_l1tab)
    {
        // "%s - %d (decimal) bytes:"
        printf("\n");
        WRMSG( HHC02614, "I", "L1TAB", (int) (n * CCKD_L1ENT_SIZE) );
        data_dump( l1, n * CCKD_L1ENT_SIZE );
        printf("\n");
    }

    /*---------------------------------------------------------------*/
    /* display OFFSET, LENGTH data                                   */
    /*---------------------------------------------------------------*/
    if (cmd_offset)
    {
        bulk = makbuf(op_length, "BULK");

        readpos(fd, bulk, op_offset, op_length);

        // "%s offset %d (0x%8.8X); length %d (0x%8.8X) bytes%s"
        printf("\n");
        WRMSG( HHC02613, "I", "IMAGE",
            op_offset, op_offset, op_length, op_length, ":" );
        data_dump( bulk, op_length );
        printf("\n");

        free(bulk);
        bulk = NULL;
    }

    /*---------------------------------------------------------------*/
    /* FBA isn't supported here because I don't know much about FBA  */
    /*---------------------------------------------------------------*/
    if ( (!ckddasd) && ((cmd_cchh) || (cmd_tt)) )
    {
        // "%s%s"
        FWRMSG( stderr, HHC02604, "S", "CCHH/reltrk not supported for FBA", "" );
        clean();
        exit( EXIT_NO_FBA_SUPPORT );
    }

    /*---------------------------------------------------------------*/
    /* Setup CCHH or relative track request                          */
    /*---------------------------------------------------------------*/
    if (ckddasd)
    {
        if (cmd_tt)
        {
            trk = op_tt;
            op_cc = trk / heads;
            op_hh = trk % heads;
        }
        else
        {
            trk = (op_cc * heads) + op_hh;
        }

        l1ndx = trk / cdevhdr.numl2tab;
        l2ndx = trk % cdevhdr.numl2tab;
        l2taboff = l1[l1ndx];

        if (swapend)
            cckd_swapend4((char *)&l2taboff);
    }

    /*---------------------------------------------------------------*/
    /* display CKD CCHH or relative track data                       */
    /*---------------------------------------------------------------*/
    if ((cmd_cchh) || (cmd_tt))
    {
        // "CC %d HH %d = reltrk %d; L1 index = %d, L2 index = %d"
        WRMSG( HHC02609, "I", op_cc, op_hh, trk, l1ndx, l2ndx );

        // "L1 index %d = L2TAB offset %d (0x%8.8X)"
        WRMSG( HHC02610, "I", l1ndx, (int) l2taboff, (int) l2taboff );

        l2 = makbuf(cdevhdr.numl2tab * sizeof(CCKD_L2ENT), "L2TAB");

        readpos(fd, l2, l2taboff,
                cdevhdr.numl2tab * sizeof(CCKD_L2ENT));

        if (cmd_l2tab)
        {
            // "%s - %d (decimal) bytes:"
            printf("\n");
            WRMSG( HHC02614, "I", "L2TAB",
                (int) (cdevhdr.numl2tab * sizeof( CCKD_L2ENT )) );
            data_dump( l2, (cdevhdr.numl2tab * sizeof( CCKD_L2ENT )));
        }

        // "L2 index %d = L2TAB entry: %d bytes:"
        printf("\n");
        WRMSG( HHC02611, "I", l2ndx, (int) sizeof( CCKD_L2ENT ));
        data_dump( &l2[l2ndx], sizeof( CCKD_L2ENT ));
        printf("\n");

        trkhdroff = l2[l2ndx].pos;
        imglen = l2[l2ndx].len;

        if (swapend)
        {
            cckd_swapend4((char *)&trkhdroff);
            cckd_swapend4((char *)&imglen);
        }

        // "%s offset %d (0x%8.8X); length %d (0x%8.8X) bytes%s"
        WRMSG( HHC02613, "I", "TRKHDR",
            (int) trkhdroff, (int) trkhdroff, imglen, imglen, "" );

        tbuf = makbuf(imglen, "TRKHDR+DATA");
        readpos(fd, tbuf, trkhdroff, imglen);

        // HHC02612 "TRKHDR track %d:"
        printf("\n");
        WRMSG( HHC02612, "I", trk );
        data_dump( tbuf, sizeof( CKDDASD_TRKHDR ));
        printf("\n");

        if (cmd_trkdata)
            showtrk(tbuf, imglen, trk, cmd_hexdump);

        free(l2);
        free(tbuf);

        l2 = NULL;
        tbuf = NULL;
    }

    /* Close file, exit */
    clean();
    return cckd_diag_rc;
}
