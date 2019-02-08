/* TAPEMAP.C   (C) Copyright Jay Maynard, 2000-2012                  */
/*              Map AWSTAPE format tape image                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program reads an AWSTAPE format tape image file and produces */
/* a map of the tape, printing any standard label records it finds.  */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#include "hercules.h"
#include "tapedev.h"

#define UTILITY_NAME    "tapemap"
#define UTILITY_DESC    "tape map"

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
/* TAPEMAP main entry point                                          */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             i;                      /* Array subscript           */
int             len;                    /* Block length              */
char           *filename;               /* -> Input file name        */
int             infd = -1;              /* Input file descriptor     */
int             fileno;                 /* Tape file number          */
int             blkcount;               /* Block count               */
int             curblkl;                /* Current block length      */
int             minblksz;               /* Minimum block size        */
int             maxblksz;               /* Maximum block size        */
int64_t         file_bytes;             /* File byte count           */
BYTE            labelrec[81];           /* Standard label (ASCIIZ)   */
AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE block header      */
char            pathname[MAX_PATH];     /* file path in host format  */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* The only argument is the tape image file name */
    if (argc == 2 && argv[1] != NULL)
    {
        filename = argv[1];
    }
    else
    {
        // "Usage: %s filename"
        WRMSG( HHC02726, "I", pgm );
        exit (1);
    }

    /* Open the tape device */
    hostpath(pathname, filename, sizeof(pathname));
    infd = HOPEN (pathname, O_RDONLY | O_BINARY);
    if (infd < 0)
    {
        // "Tape %s: Error opening: errno=%d: %s"
        FWRMSG( stderr, HHC02715, "E", filename, errno, strerror( errno ));
        exit (2);
    }

    /* Read blocks from the input file and report on them */
    fileno = 1;
    blkcount = 0;
    minblksz = 0;
    maxblksz = 0;
    file_bytes = 0;
    len = 0;

    while (1)
    {
        if (extgui)
        {
            /* Report progress every nnnK */
            if( ( curpos & PROGRESS_MASK ) != ( prevpos & PROGRESS_MASK ) )
            {
                prevpos = curpos;
                EXTGUIMSG( "IPOS=%ld\n", curpos );
            }
        }

        /* Read a block from the tape */
        len = read (infd, buf, sizeof(AWSTAPE_BLKHDR));

        if (extgui)
            curpos += len;

        if (len < 0)
        {
            // "File %s: Error reading %s header: rc=%d, errno=%d: %s"
            FWRMSG( stderr, HHC02707, "E", filename, "AWSTAPE", len, errno, strerror( errno ));
            exit (3);
        }

        /* Did we finish too soon? */
        if ((len > 0) && (len < (int)sizeof(AWSTAPE_BLKHDR)))
        {
            // "File %s: Error, incomplete %s header"
            FWRMSG( stderr, HHC02741, "E", filename, "AWSTAPE" );
            exit(4);
        }

        /* Check for end of tape. */
        if (len == 0)
        {
            // "End of tape"
            WRMSG( HHC02704, "I" );
            break;
        }

        /* Parse the block header */
        memcpy(&awshdr, buf, sizeof(AWSTAPE_BLKHDR));

        /* Tapemark? */
        if ((awshdr.flags1 & AWSTAPE_FLAG1_TAPEMARK) != 0)
        {
            /* Print summary of current file */
            // "File No. %u: Blocks=%u, Bytes=%"PRId64", Block size min=%u, max=%u, avg=%u"
            WRMSG( HHC02721, "I", fileno, blkcount, file_bytes,
                         minblksz, maxblksz,
                         (blkcount ? ((int)file_bytes/blkcount) : 0 ));

            /* Reset counters for next file */
            fileno++;
            minblksz = 0;
            maxblksz = 0;
            blkcount = 0;
            file_bytes = 0;

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

            if (extgui)
                curpos += len;

            if (len < 0)
            {
                // "File %s: Error reading %s data block: rc=%d, errno=%d: %s"
                FWRMSG( stderr, HHC02709, "E", filename, "AWSTAPE", len, errno, strerror( errno ));
                exit (5);
            }

            /* Did we finish too soon? */
            if ((len > 0) && (len < curblkl))
            {
                // "File %s: Error, incomplete final data block: expected %d bytes, read %d"
                FWRMSG( stderr, HHC02742, "E", filename, curblkl, len );
                exit(6);
            }

            /* Check for end of tape */
            if (len == 0)
            {
                // "File %s: Error, %s header block without data"
                FWRMSG( stderr, HHC02743, "E", filename, "AWSTAPE" );
                exit(7);
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

    } /* end while */

    /* Close files and exit */
    close (infd);

    return 0;

} /* end function main */
