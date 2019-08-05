/* DASDCOPY64.C (C) Copyright Roger Bowler, 1999-2012                */
/*              Copy a dasd file to another dasd file                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*      This program copies a dasd file to another dasd file.        */
/*      Input file and output file may be compressed or not.         */
/*      Files may be either ckd (or cckd) or fba (or cfba) but       */
/*      file types (ckd/cckd or fba/cfba) may not be mixed.          */
/*                                                                   */
/*      Usage:                                                       */
/*              dasdcopy64 [-options] ifile [sf=sfile] ofile         */
/*                                                                   */
/*      Refer to the usage section below for details of options.     */
/*                                                                   */
/*      The program may also be invoked by one of the following      */
/*      aliases which override the default output file format:       */
/*                                                                   */
/*              ckd2cckd64 [-options] ifile            ofile         */
/*              cckd642ckd [-options] ifile [sf=sfile] ofile         */
/*              fba2cfba64 [-options] ifile            ofile         */
/*              cfba642fba [-options] ifile [sf=sfile] ofile         */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "devtype.h"
#include "opcode.h"
#include "ccwarn.h"

#define UTILITY_NAME    "dasdcopy64"
#define UTILITY_DESC    "64-bit DASD copy/convert"

int syntax( const char* pgm, const char* msgfmt, ... );
void status (int, int);
int nulltrk(BYTE *, int, int, int);

#define CKD      0x01
#define CCKD     0x02
#define FBA      0x04
#define CFBA     0x08

#define CKD64    0x10
#define CCKD64   0x20
#define FBA64    0x40
#define CFBA64   0x80

#define CKDMASK  (CKD   | CCKD   | CKD64  | CCKD64) // 0x33
#define FBAMASK  (FBA   | CFBA   | FBA64  | CFBA64) // 0xcc
#define COMPMASK (CCKD  | CFBA   | CCKD64 | CFBA64) // 0xaa
#define MASK64   (CKD64 | CCKD64 | FBA64  | CFBA64) // 0xf0

/*-------------------------------------------------------------------*/
/* Copy a dasd file to another dasd file                             */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             ckddasd=-1;             /* 1=CKD  0=FBA              */
int             rc;                     /* Return code               */
int             quiet=0;                /* 1=Don't display status    */
int             comp=255;               /* Compression algorithm     */
int             cyls=-1, blks=-1;       /* Size of output file       */
int             lfs=0;                  /* 1=Create 1 large file     */
int             alt=0;                  /* 1=Create alt cyls         */
int             r=0;                    /* 1=Replace output file     */
int             in=0, out=0;            /* Input/Output file types   */
int             fd;                     /* Input file descriptor     */
char           *ifile, *ofile;          /* -> Input/Output file names*/
char           *sfile=NULL;             /* -> Input shadow file name */
CIFBLK         *icif, *ocif;            /* -> Input/Output CIFBLK    */
DEVBLK         *idev, *odev;            /* -> Input/Output DEVBLK    */

CKDDEV         *ckd=NULL;               /* -> CKD device table entry */
FBADEV         *fba=NULL;               /* -> FBA device table entry */
int             i, n, max;              /* Loop index, limits        */
BYTE            unitstat;               /* Device unit status        */
U32             imgtyp;                 /* Dasd file image type      */
U64             fba_bytes_remaining=0;  /* FBA bytes to be copied    */
int             nullfmt = CKD_NULLTRK_FMT0; /* Null track format     */
char            pathname[MAX_PATH];     /* file path in host format  */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    if (strcasecmp(pgm, "ckd2cckd64") == 0)
    {
        in  = CKD64;
        out = CCKD64;
    }
    else if (strcasecmp(pgm, "cckd642ckd") == 0)
    {
        in  = CCKD64;
        out = CKD64;
    }
    else if (strcasecmp(pgm, "fba2cfba64") == 0)
    {
        in  = FBA64;
        out = CFBA64;
    }
    else if (strcasecmp(pgm, "cfba642fba") == 0)
    {
        in  = CFBA64;
        out = FBA64;
    }

    /* Process the arguments */
    for (argc--, argv++ ; argc > 0 ; argc--, argv++)
    {
        if (argv[0][0] != '-') break;
        if (strcmp(argv[0], "-h") == 0)
        {
            syntax( pgm, NULL );
            return 0;
        }
        else if (strcmp(argv[0], "-q") == 0
              || strcmp(argv[0], "-quiet") == 0)
            quiet = 1;
        else if (strcmp(argv[0], "-r") == 0)
            r = 1;
#ifdef CCKD_COMPRESS_ZLIB
        else if (strcmp(argv[0], "-z") == 0)
            comp = CCKD_COMPRESS_ZLIB;
#endif
#ifdef CCKD_COMPRESS_BZIP2
        else if (strcmp(argv[0], "-bz2") == 0)
            comp = CCKD_COMPRESS_BZIP2;
#endif
        else if (strcmp(argv[0], "-0") == 0)
            comp = CCKD_COMPRESS_NONE;
        else if ((strcmp(argv[0], "-cyl") == 0
               || strcmp(argv[0], "-cyls") == 0) && cyls < 0)
        {
            if (argc < 2 || (cyls = atoi(argv[1])) < 0)
                return syntax( pgm, "invalid %s argument: %s",
                    "-cyls", argc < 2 ? "(missing)" : argv[1] );
            argc--; argv++;
        }
        else if ((strcmp(argv[0], "-blk") == 0
               || strcmp(argv[0], "-blks") == 0) && blks < 0)
        {
            if (argc < 2 || (blks = atoi(argv[1])) < 0)
                return syntax( pgm, "invalid %s argument: %s",
                    "-blks", argc < 2 ? "(missing)" : argv[1] );
            argc--; argv++;
        }
        else if (strcmp(argv[0], "-a") == 0
              || strcmp(argv[0], "-alt") == 0
              || strcmp(argv[0], "-alts") == 0)
            alt = 1;
        else if (strcmp(argv[0], "-lfs") == 0)
            lfs = 1;
        else if (out == 0 && strcmp(argv[0], "-o") == 0)
        {
            if (argc < 2)
                return syntax( pgm, "invalid %s argument: %s",
                    "-o", "(missing)" );
            if (out != 0)
                return syntax( pgm, "invalid %s argument: %s",
                    "-o", "already previously specified" );

                 if (strcasecmp( argv[1], "ckd"    ) == 0) out = CKD;
            else if (strcasecmp( argv[1], "cckd"   ) == 0) out = CCKD;
            else if (strcasecmp( argv[1], "fba"    ) == 0) out = FBA;
            else if (strcasecmp( argv[1], "cfba"   ) == 0) out = CFBA;
            else if (strcasecmp( argv[1], "ckd64"  ) == 0) out = CKD64;
            else if (strcasecmp( argv[1], "cckd64" ) == 0) out = CCKD64;
            else if (strcasecmp( argv[1], "fba64"  ) == 0) out = FBA64;
            else if (strcasecmp( argv[1], "cfba64" ) == 0) out = CFBA64;
            else
                return syntax( pgm, "invalid %s argument: %s",
                    "-o", argv[1] );

            argc--; argv++;
        }
        else
            return syntax( pgm, "unrecognized/unsupported option: %s",
                argv[0] );
    }

    /* Get the file names:
       input-file [sf=shadow-file] output-file   */
    if (argc < 2)
        return syntax( pgm, "%s", "missing input-file specification" );
    if (argc > 3)
        return syntax( pgm, "extraneous parameter: %s", argv[2] );
    ifile = argv[0];
    if (argc < 3)
        ofile = argv[1];
    else
    {
        if (strlen(argv[1]) < 4 || memcmp(argv[1], "sf=", 3) != 0)
            return syntax( pgm, "invalid shadow file specification: %s",
                argv[1] );

        sfile = argv[1];
        ofile = argv[2];
    }

    /* If we don't know what the input file is then find out */
    if (!in)
    {
        BYTE buf[8];

        hostpath( pathname, ifile, sizeof( pathname ));

        if ((fd = HOPEN( pathname, O_RDONLY | O_BINARY )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "open()", strerror( errno ));
            return -1;
        }

        if ((rc = read( fd, buf, 8 )) < 8)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "read()", strerror( errno ));
            return -1;
        }

        imgtyp = dh_devid_typ( buf );

             if (imgtyp & CKD_P370_TYP) in = CKD;
        else if (imgtyp & CKD_C370_TYP) in = CCKD;
        else if (imgtyp & FBA_P370_TYP) in = FBA;
        else if (imgtyp & FBA_C370_TYP) in = CFBA;
        else if (imgtyp & CKD_P064_TYP) in = CKD64;
        else if (imgtyp & CKD_C064_TYP) in = CCKD64;
        else if (imgtyp & FBA_P064_TYP) in = FBA64;
        else if (imgtyp & FBA_C064_TYP) in = CFBA64;
        else
        {
            // "Dasd image file format unsupported or unrecognized: %s"
            FWRMSG( stderr, HHC02424, "E", ifile );
            close( fd );
            return syntax( pgm, NULL );
        }

        close( fd );
    }

    /* If we don't know what the output file type is
       then derive it from the input file type */
    if (out == 0)
    {
        switch (in) {
                case CKD:
                case CKD64:
                    if (!lfs)
                    {
                        out = CCKD64;
                    }
                    else
                    {
                        out = CKD64;
                    }
                    break;
                case CCKD:
                case CCKD64:
                    if (comp == 255)
                    {
                         out = CKD64;
                    }
                    else
                    {
                        out = CCKD64;
                    }
                    break;
                case FBA:
                case FBA64:
                    if (!lfs)
                    {
                        out = CFBA64;
                    }
                    else
                    {
                        out = FBA64;
                    }
                    break;
                case CFBA:
                case CFBA64:
                    if (comp == 255)
                    {
                        out = FBA64;
                    }
                    else
                    {
                        out = CFBA64;
                    }
                    break;
        }
    }

    /* Set default compression if out file is to be compressed */
    if (comp == 255 && (out & COMPMASK))
#ifdef CCKD_COMPRESS_ZLIB
        comp = CCKD_COMPRESS_ZLIB;
#else
        comp = CCKD_COMPRESS_NONE;
#endif

    /* Perform sanity checks on the options... */

    if (sfile && !(in & COMPMASK))        return syntax( pgm, "%s",
        "shadow files invalid if input not compressed" );

    if (comp != 255 && !(out & COMPMASK)) return syntax( pgm, "%s",
        "compress type invalid for uncompressed output" );

    if (lfs && (out & COMPMASK))          return syntax( pgm, "%s",
        "-lfs invalid if output is compressed" );

    if (cyls >= 0 && (in & FBAMASK ))     return syntax( pgm, "%s",
        "-cyls invalid for fba input" );

    if (blks >= 0 && (in & CKDMASK ))     return syntax( pgm, "%s",
        "-blks invalid for ckd input" );

    if (alt && (in & FBAMASK ))           return syntax( pgm, "%s",
        "-a invalid for fba input" );

    if (0
        || ((in & CKDMASK) && !(out & CKDMASK ))
        || ((in & FBAMASK) && !(out & FBAMASK ))
    )
        return syntax( pgm, "%s",
            "cannot copy ckd to fba or vice versa" );

    /* Set the type of processing (ckd or fba) */
    ckddasd = (in & CKDMASK);

    /* Open the input file */
    if (ckddasd)
    {
        if (in & MASK64)
            icif = open_ckd64_image( ifile, sfile, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL );
        else
            icif = open_ckd_image  ( ifile, sfile, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL );
    }
    else // fba
    {
        if (in & MASK64)
            icif = open_fba64_image( ifile, sfile, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL );
        else
            icif = open_fba_image  ( ifile, sfile, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL );
    }
    if (icif == NULL)
    {
        // "Failed opening %s"
        FWRMSG( stderr, HHC02403, "E", ifile );
        return -1;
    }
    idev = &icif->devblk;
    if (idev->oslinux) nullfmt = CKD_NULLTRK_FMT2;

    /* Calculate the number of tracks or blocks to copy */
    if (ckddasd)
    {
        if (cyls < 0) cyls = idev->ckdcyls;
        else if (cyls == 0) cyls = (idev->hnd->used)(idev);
        ckd = dasd_lookup (DASD_CKDDEV, NULL, idev->devtype, 0);
        if (ckd == NULL)
        {
            // "CKD lookup failed: device type %04X cyls %d"
            FWRMSG( stderr, HHC02430, "E",
                     idev->devtype, cyls );
            close_image_file (icif);
            return -1;
        }
        if (cyls <= ckd->cyls && alt) cyls = ckd->cyls + ckd->altcyls;
        n = cyls * idev->ckdheads;
        max = idev->ckdtrks;
        if (max < n && out == CCKD) n = max;
    }
    else // fba
    {
        fba_bytes_remaining = (U64)((S64)idev->fbanumblk * idev->fbablksiz);
        if (blks < 0) blks = idev->fbanumblk;
        else if (blks == 0) blks = (idev->hnd->used)(idev);
        fba = dasd_lookup (DASD_FBADEV, NULL, idev->devtype, 0);
        if (fba == NULL)
        {
            // "FBA lookup failed: blks %d"
            FWRMSG( stderr, HHC02431, "E", blks );
            close_image_file (icif);
            return -1;
        }

        n = blks;
        max = idev->fbanumblk;

        if (max < n && out == CFBA)
            n = max;

        n =   (n   + CFBA_BLKS_PER_GRP - 1) / CFBA_BLKS_PER_GRP;
        max = (max + CFBA_BLKS_PER_GRP - 1) / CFBA_BLKS_PER_GRP;
    }

    /* Create the output file */
    if (ckddasd)
    {
        if (out & MASK64)
            rc = create_ckd64( ofile, idev->devtype, idev->ckdheads,
                               ckd->r1, cyls, "", (BYTE)comp, (BYTE)lfs,
                               (BYTE)(1+r), (BYTE)nullfmt, 0, 1, 0 );
        else
            rc = create_ckd  ( ofile, idev->devtype, idev->ckdheads,
                               ckd->r1, cyls, "", (BYTE)comp, (BYTE)lfs,
                               (BYTE)(1+r), (BYTE)nullfmt, 0, 1, 0 );
    }
    else // fba
    {
        if (out & MASK64)
            rc = create_fba64( ofile, idev->devtype, fba->size,
                               blks, "", (BYTE) comp, lfs, 1+r, 0 );
        else
            rc = create_fba  ( ofile, idev->devtype, fba->size,
                               blks, "", (BYTE) comp, lfs, 1+r, 0 );
    }
    if (rc < 0)
    {
        // "Failed creating %s"
        FWRMSG( stderr, HHC02432, "E", ofile );
        close_image_file (icif);
        return -1;
    }

    /* Open the output file */
    if (ckddasd)
    {
        if (out & MASK64)
            ocif = open_ckd64_image( ofile, NULL, O_RDWR | O_BINARY, IMAGE_OPEN_DASDCOPY );
        else
            ocif = open_ckd_image  ( ofile, NULL, O_RDWR | O_BINARY, IMAGE_OPEN_DASDCOPY );
    }
    else // fba
    {
        if (out & MASK64)
            ocif = open_fba64_image( ofile, NULL, O_RDWR | O_BINARY, IMAGE_OPEN_DASDCOPY );
        else
            ocif = open_fba_image  ( ofile, NULL, O_RDWR | O_BINARY, IMAGE_OPEN_DASDCOPY );
    }
    if (ocif == NULL)
    {
        // "Failed opening %s"
        FWRMSG( stderr, HHC02403, "E", ofile );
        close_image_file (icif);
        return -1;
    }
    odev = &ocif->devblk;

    /* Notify GUI of total #of tracks or blocks being copied... */
    EXTGUIMSG( "TRKS=%d\n", n );

    /* Copy the files */

    if (!extgui)
        if (!quiet)
            printf ( "  %3d%% %7d of %d", 0, 0, n );

    for (i = 0; i < n; i++)
    {
        /* Read a track or block */
        if (ckddasd)
        {
            if (i < max)
                rc = (idev->hnd->read)(idev, i, &unitstat);
            else
            {
                memset (idev->buf, 0, idev->ckdtrksz);
                rc = nulltrk(idev->buf, i, idev->ckdheads, nullfmt);
            }
        }
        else
        {
            if (i < max)
                rc = (idev->hnd->read)(idev, i, &unitstat);
            else
            {
                memset (idev->buf, 0, CFBA_BLKGRP_SIZE);
                rc = 0;
            }
        }
        if (rc < 0)
        {
            // "Read error on file %s: %s %d stat=%2.2X, null %s substituted"
            FWRMSG( stderr, HHC02433, "E",
                     ifile, ckddasd ? "track" : "block", i, unitstat,
                     ckddasd ? "track" : "block" );
            if (ckddasd)
                nulltrk(idev->buf, i, idev->ckdheads, nullfmt);
            else
                memset (idev->buf, 0, CFBA_BLKGRP_SIZE);
            if (!quiet)
            {
                if (!extgui)
                    printf ( "  %3d%% %7d of %d", 0, 0, n );
                status (i, n);
            }
        }

        /* Write the track or block just read... */

        if (ckddasd)
        {
            rc = (odev->hnd->write)(odev, i, 0, idev->buf,
                      idev->ckdtrksz, &unitstat);
        }
        else
        {
            if (fba_bytes_remaining >= (U64)idev->buflen)
            {
                rc = (odev->hnd->write)(odev,  i, 0, idev->buf,
                          idev->buflen, &unitstat);
                fba_bytes_remaining -= (U64)idev->buflen;
            }
            else
            {
                ASSERT(fba_bytes_remaining > 0 && (i+1) >= n);
                rc = (odev->hnd->write)(odev,  i, 0, idev->buf,
                          (int)fba_bytes_remaining, &unitstat);
                fba_bytes_remaining = 0;
            }
        }
        if (rc < 0)
        {
            // "Write error on file %s: %s %d stat=%2.2X"
            FWRMSG( stderr, HHC02434, "E",
                     ofile, ckddasd ? "track" : "block", i, unitstat );
            /* IMPORTANT PROGRAMMING NOTE: please note that the output
               file's track is written directly from the INPUT's device
               buffer. This means when we are done, we must close the
               output file first, which flushes the output of the last
               track that was written, which as explained, requires that
               the INPUT file's device buffer to still be valid.
            */
            close_image_file( ocif );   /* Close output file FIRST! */
            close_image_file( icif );   /* Close input file SECOND! */
            return -1;
        }

        /* Update the status indicator */
        if (!quiet) status (i+1, n);
    }

    /* IMPORTANT PROGRAMMING NOTE: please note that the output
       file's track is written directly from the INPUT's device
       buffer. This means when we are done, we must close the
       output file first, which flushes the output of the last
       track that was written, which as explained, requires that
       the INPUT file's device buffer to still be valid.
    */
    close_image_file( ocif );   /* Close output file FIRST! */
    close_image_file( icif );   /* Close input file SECOND! */

    if (!extgui)
        if (!quiet)
            printf ( "\r" );

    if (sfile)
        // "Shadow file data successfully merged into output"
        WRMSG( HHC02595, "I" );

    // "DASD operation completed"
    WRMSG( HHC02423, "I" );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Build a null track image                                          */
/*-------------------------------------------------------------------*/
int nulltrk(BYTE *buf, int trk, int heads, int nullfmt)
{
int             i;                      /* Loop counter              */
CKD_TRKHDR     *trkhdr;                 /* -> Track header           */
CKD_RECHDR     *rechdr;                 /* -> Record header          */
U32             cyl;                    /* Cylinder number           */
U32             head;                   /* Head number               */
BYTE            r;                      /* Record number             */
BYTE           *pos;                    /* -> Next position in buffer*/

    /* cylinder and head calculations */
    cyl = trk / heads;
    head = trk % heads;

    /* Build the track header */
    trkhdr = (CKD_TRKHDR*)buf;
    trkhdr->bin = 0;
    store_hw(&trkhdr->cyl,  (U16) cyl);
    store_hw(&trkhdr->head, (U16) head);
    pos = buf + CKD_TRKHDR_SIZE;

    /* Build record zero */
    r = 0;
    rechdr = (CKD_RECHDR*)pos;
    pos += CKD_RECHDR_SIZE;
    store_hw(&rechdr->cyl,  (U16) cyl);
    store_hw(&rechdr->head, (U16) head);
    rechdr->rec = r;
    rechdr->klen = 0;
    store_hw(&rechdr->dlen, CKD_R0_DLEN);
    pos +=                  CKD_R0_DLEN;
    r++;

    /* Specific null track formatting */
    if (nullfmt == CKD_NULLTRK_FMT0)
    {
        rechdr = (CKD_RECHDR*)pos;
        pos += CKD_RECHDR_SIZE;

        store_hw(&rechdr->cyl,  (U16) cyl);
        store_hw(&rechdr->head, (U16) head);
        rechdr->rec = r;
        rechdr->klen = 0;
        store_hw(&rechdr->dlen, 0);
        r++;
    }
    else if (nullfmt == CKD_NULLTRK_FMT2)
    {
        for (i = 0; i < 12; i++)
        {
            rechdr = (CKD_RECHDR*)pos;
            pos += CKD_RECHDR_SIZE;

            store_hw(&rechdr->cyl,  (U16) cyl);
            store_hw(&rechdr->head, (U16) head);
            rechdr->rec = r;
            rechdr->klen = 0;
            store_hw(&rechdr->dlen, CKD_NULL_FMT2_DLEN );
            r++;
            pos +=                  CKD_NULL_FMT2_DLEN;
        }
    }

    /* Build the end of track marker */
    memcpy (pos, &CKD_ENDTRK, CKD_ENDTRK_SIZE);
    pos +=                    CKD_ENDTRK_SIZE;

    return 0;
}

/*-------------------------------------------------------------------*/
/* Display command syntax                                            */
/*-------------------------------------------------------------------*/
int syntax( const char* pgm, const char* msgfmt, ... )
{
    int zlib  = 0;
    int bzip2 = 0;
    int lfs   = 0;

    char zbuf  [80];
    char bzbuf [80];
    char lfsbuf[80];

    zbuf  [0] = 0;
    bzbuf [0] = 0;
    lfsbuf[0] = 0;

    /* Show them their syntax error... */
    if (msgfmt)
    {
        const int  chunksize  = 128;
        int        rc         = -1;
        int        buffsize   =  0;
        char*      msgbuf     = NULL;
        va_list    vargs;

        do
        {
            if (msgbuf) free( msgbuf );
            if (!(msgbuf = malloc( buffsize += chunksize )))
                BREAK_INTO_DEBUGGER();

            va_end(   vargs );
            va_start( vargs, msgfmt );

            rc = vsnprintf( msgbuf, buffsize, msgfmt, vargs );
        }
        while (rc < 0 || rc >= buffsize);

        // "Syntax error: %s"
        FWRMSG( stderr, HHC02594, "E", msgbuf );
        free( msgbuf );
    }

#ifdef CCKD_COMPRESS_ZLIB
    zlib = 1;
#endif

#ifdef CCKD_COMPRESS_BZIP2
    bzip2 = 1;
#endif

    if (sizeof(off_t) > 4)
        lfs = 1;

#define HHC02435I  "HHC02435I "     // ckd2cckd64
#define HHC02436I  "HHC02436I "     // cckd642ckd
#define HHC02437I  "HHC02437I "     // fba2cfba64
#define HHC02438I  "HHC02438I "     // cfba642fba
#define HHC02439I  "HHC02439I "     // dasdcopy64

#define Z_HELP     "  -z       compress using zlib [default]"
#define BZ_HELP    "  -bz2     compress using bzip2"
#define LFS_HELP   "  -lfs     create single large output file"

    /* Display help information... */
    if (strcasecmp( pgm,                   "ckd2cckd64"  ) == 0)
    {
        if (zlib)  MSGBUF(  zbuf, "%s%s\n", HHC02435I,  Z_HELP );
        if (bzip2) MSGBUF( bzbuf, "%s%s\n", HHC02435I, BZ_HELP );
        WRMSG(                              HHC02435, "I", zbuf, bzbuf );
    }
    else if (strcasecmp( pgm,             "cckd642ckd"   ) == 0)
    {
        if (lfs) MSGBUF( lfsbuf, "%s%s\n", HHC02436I, LFS_HELP );
        WRMSG(                             HHC02436, "I", lfsbuf );
    }
    else if (strcasecmp( pgm,              "fba2cfba64"  ) == 0)
    {
        if (zlib)  MSGBUF(  zbuf, "%s%s\n", HHC02437I,  Z_HELP );
        if (bzip2) MSGBUF( bzbuf, "%s%s\n", HHC02437I, BZ_HELP );
        WRMSG(                              HHC02437, "I", zbuf, bzbuf );
    }
    else if (strcasecmp( pgm,             "cfba642fba"   ) == 0)
    {
        if (lfs) MSGBUF( lfsbuf, "%s%s\n", HHC02438I, LFS_HELP );
        WRMSG(                             HHC02438, "I", lfsbuf );
    }
    else
    {
        if (zlib)  MSGBUF(   zbuf, "%s%s\n", HHC02439I,   Z_HELP );
        if (bzip2) MSGBUF(  bzbuf, "%s%s\n", HHC02439I,  BZ_HELP );
        if (lfs)   MSGBUF( lfsbuf, "%s%s\n", HHC02439I, LFS_HELP );
        WRMSG(                               HHC02439, "I", pgm, zbuf, bzbuf, lfsbuf,
            "CKD, CKD64, CCKD, CCKD64, FBA, FBA64, CFBA, CFBA64" );
    }

    return -1;

}

/*-------------------------------------------------------------------*/
/* Display progress status                                           */
/*-------------------------------------------------------------------*/
void status (int i, int n)
{
    if (extgui)
    {
        UNREFERENCED( n );
        if (i % 100)
            return;
        EXTGUIMSG( "TRK=%d\n", i );
        return;
    }
    else
    {
        static char indic[] = "|/-\\";
//      if (i % 101 != 1)
//          return;
        printf ("\r%c %3d%% %7d", indic[i%4], (int)((i*100.0)/n), i);
    }
}
