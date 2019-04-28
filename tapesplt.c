/* TAPESPLT.C  (C) Copyright Jay Maynard, 2000-2012                  */
/*              Split AWSTAPE format tape image                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program reads an AWSTAPE format tape image file and produces */
/* output files containing pieces of it, controlled by command line  */
/* options.                                                          */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#include "hercules.h"
#include "tapedev.h"

#define UTILITY_NAME    "tapesplt"
#define UTILITY_DESC    "Split AWS tape into pieces"

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
static BYTE vollbl[] = "\xE5\xD6\xD3";  /* EBCDIC characters "VOL"   */
static BYTE hdrlbl[] = "\xC8\xC4\xD9";  /* EBCDIC characters "HDR"   */
static BYTE eoflbl[] = "\xC5\xD6\xC6";  /* EBCDIC characters "EOF"   */
static BYTE eovlbl[] = "\xC5\xD6\xE5";  /* EBCDIC characters "EOV"   */
static BYTE buf[ MAX_TAPE_BLKSIZE ];

/* Report progress every this many bytes */
#define PROGRESS_MASK (~0x3FFFF /* 256K */)
/* How many bytes we've read so far. */
long  curpos = 0;
long  prevpos = 0;

/*-------------------------------------------------------------------*/
/* tapesplt main entry point                                        */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
int             len;                    /* Block length              */
char           *infilename;             /* -> Input file name        */
char           *outfilename;            /* -> Current out file name  */
int             infd = -1;              /* Input file descriptor     */
int             outfd = -1;             /* Current out file desc     */
int             fileno;                 /* Tape file number          */
int             blkcount;               /* Block count               */
int             curblkl;                /* Current block length      */
int             minblksz;               /* Minimum block size        */
int             maxblksz;               /* Maximum block size        */
int64_t         file_bytes;             /* File byte count           */
int             outfilenum;             /* Current out file# in argv */
int             outfilecount;           /* Current # files copied    */
int             files2copy;             /* Current # files to copy   */
BYTE            labelrec[81];           /* Standard label (ASCIIZ)   */
AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE block header      */
char            pathname[MAX_PATH];     /* file path in host format  */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* The only argument is the tape image file name */
    if (argc > 3 && argv[1] != NULL)
    {
        infilename = argv[1];
    }
    else
    {
        // "Usage: %s ...
        WRMSG( HHC02725, "I", pgm );
        exit (1);
    }

    /* Open the tape device */
    hostpath(pathname, infilename, sizeof(pathname));
    infd = HOPEN (pathname, O_RDONLY | O_BINARY);
    if (infd < 0)
    {
        // "Tape %s: Error opening: errno=%d: %s"
        FWRMSG( stderr, HHC02715, "E", infilename, errno, strerror( errno ));
        exit (2);
    }

    /* Copy blocks from input to output files */
    fileno = 1;
    blkcount = 0;
    minblksz = 0;
    maxblksz = 0;
    file_bytes = 0;
    len = 0;

    for (outfilenum = 2; outfilenum < argc; outfilenum += 2)
    {
        outfilename = argv[outfilenum];
        // "File %s: writing output file"
        WRMSG( HHC02740, "I", outfilename );
        hostpath(pathname, outfilename, sizeof(pathname));
        outfd = HOPEN (pathname, O_WRONLY | O_CREAT | O_BINARY | O_TRUNC,
                        S_IRUSR | S_IWUSR | S_IRGRP);

        if (outfd < 0)
        {
            // "Tape %s: Error opening: errno=%d: %s"
            FWRMSG( stderr, HHC02715, "E", outfilename, errno, strerror( errno ));
            exit (3);
        }

        if (outfilenum == argc)
        {
            /* count not specified for last file, so use big number */
            files2copy = 32767;
        }
        else
        {
            files2copy = atoi(argv[outfilenum + 1]);
        }

        /* Copy just that many files */
        for (outfilecount = 0; outfilecount < files2copy; )
        {
            /* Read a block from the tape */
            len = read (infd, buf, sizeof(AWSTAPE_BLKHDR));
            if (len < 0)
            {
                // "File %s: Error reading %s header: rc=%d, errno=%d: %s"
                FWRMSG( stderr, HHC02707, "E", infilename, "AWSTAPE", len, errno, strerror( errno ));
                exit (4);
            }

            /* Did we finish too soon? */
            if ((len > 0) && (len < (int)sizeof(AWSTAPE_BLKHDR)))
            {
                // "File %s: Error, incomplete %s header"
                FWRMSG( stderr, HHC02741, "E", infilename, "AWSTAPE" );
                exit(5);
            }

            if (extgui)
            {
                curpos += len;
                /* Report progress every nnnK */
                if( ( curpos & PROGRESS_MASK ) != ( prevpos & PROGRESS_MASK ) )
                {
                    prevpos = curpos;
                    EXTGUIMSG( "IPOS=%ld\n", curpos );
                }
            }

            /* Check for end of tape. */
            if (len == 0)
            {
                // "End of tape"
                WRMSG( HHC02704, "I" );
                break;
            }

            /* Copy the header to the output file. */
            rc = write(outfd, buf, sizeof(AWSTAPE_BLKHDR));
            if (rc < (int)sizeof(AWSTAPE_BLKHDR))
            {
                // "File %s: Error writing %s header: rc=%d, errno=%d: %s"
                FWRMSG( stderr, HHC02711, "E", outfilename, "AWSTAPE", rc, errno, strerror( errno ));
                exit(6);
            }

            /* Parse the block header */
            memcpy(&awshdr, buf, sizeof(AWSTAPE_BLKHDR));

            /* Tapemark? */
            if ((awshdr.flags1 & AWSTAPE_FLAG1_TAPEMARK) != 0)
            {
                /* Print summary of current file */
                if (blkcount)
                    // "File No. %u: Blocks=%u, Bytes=%"PRId64", Block size min=%u, max=%u, avg=%u"
                    WRMSG( HHC02721, "I", fileno, blkcount, file_bytes, minblksz, maxblksz, (int)file_bytes/blkcount );

                /* Reset counters for next file */
                fileno++;
                minblksz = 0;
                maxblksz = 0;
                blkcount = 0;
                file_bytes = 0;

                /* Count the file we just copied. */
                outfilecount++;

            }
            else /* if(tapemark) */
            {
                /* Count blocks and block sizes */
                blkcount++;
                curblkl = awshdr.curblkl[0] + (awshdr.curblkl[1] << 8);
                if (curblkl > maxblksz) maxblksz = curblkl;
                if (minblksz == 0 || curblkl < minblksz) minblksz = curblkl;

                file_bytes += curblkl;

                /* Read the data block. */
                len = read (infd, buf, curblkl);
                if (len < 0)
                {
                    // "File %s: Error reading %s data block: rc=%d, errno=%d: %s"
                    FWRMSG( stderr, HHC02709, "E", infilename, "AWSTAPE", rc, errno, strerror( errno ));
                    exit (7);
                }

                /* Did we finish too soon? */
                if ((len > 0) && (len < curblkl))
                {
                    // "File %s: Error, incomplete final data block: expected %d bytes, read %d"
                    FWRMSG( stderr, HHC02742, "E", infilename, curblkl, len );
                    exit(8);
                }

                /* Check for end of tape */
                if (len == 0)
                {
                    // "File %s: Error, %s header block without data"
                    FWRMSG( stderr, HHC02743, "E", infilename, "AWSTAPE" );
                    exit(9);
                }

                if (extgui)
                {
                    curpos += len;
                    /* Report progress every nnnK */
                    if( ( curpos & PROGRESS_MASK ) != ( prevpos & PROGRESS_MASK ) )
                    {
                        prevpos = curpos;
                        EXTGUIMSG( "IPOS=%ld\n", curpos );
                    }
                }

                /* Copy the header to the output file. */
                rc = write(outfd, buf, len);
                if (rc < len)
                {
                    // "File %s: Error writing %s data block: rc=%d, errno=%d: %s"
                    FWRMSG( stderr, HHC02712, "E", outfilename, "AWSTAPE", rc, errno, strerror( errno ));
                    exit(10);
                }

                /* Print standard labels */
                if (len == 80 && blkcount < 4
                    && (memcmp(buf, vollbl, 3) == 0
                        || memcmp(buf, hdrlbl, 3) == 0
                        || memcmp(buf, eoflbl, 3) == 0
                        || memcmp(buf, eovlbl, 3) == 0))
                {
                    for (i=0; i < 80; i++)
                        labelrec[i] = guest_to_host(buf[i]);
                    labelrec[i] = '\0';
                    // "Tape Label: %s"
                    WRMSG( HHC02722, "I", labelrec );
                }

            } /* end if(tapemark) */

        } /* end for(outfilecount) */

        close(outfd);

    } /* end for(outfilenum) */

    /* Close files and exit */
    close (infd);

    return 0;

} /* end function main */
