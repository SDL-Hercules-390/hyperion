/* CCKDSWAP.C   (C) Copyright Roger Bowler, 1999-2012                */
/*       Swap the `endianess' of a compressed CKD file.              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module changes the `endianess' of a compressed CKD file.     */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"

#define UTILITY_NAME    "cckdswap"

/*-------------------------------------------------------------------*/
/* Swap the `endianess' of  cckd file                                */
/*-------------------------------------------------------------------*/

int syntax( const char* pgm );

int main ( int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
CKD_DEVHDR      devhdr;                 /* CKD device header         */
CCKD_DEVHDR     cdevhdr;                /* Compressed CKD device hdr */
int             level = 0;              /* Chkdsk level              */
int             force = 0;              /* 1=swap if OPENED bit on   */
int             rc;                     /* Return code               */
int             i;                      /* Index                     */
int             bigend;                 /* 1=big-endian file         */
DEVBLK          devblk;                 /* DEVBLK                    */
DEVBLK         *dev=&devblk;            /* -> DEVBLK                 */

    INITIALIZE_UTILITY( UTILITY_NAME, "Swap 'endianess' of a CCKD file", &pgm );

    /* parse the arguments */
    for (argc--, argv++ ; argc > 0 ; argc--, argv++)
    {
        if(**argv != '-') break;

        switch(argv[0][1])
        {
            case '0':
            case '1':
            case '2':
            case '3':  if (argv[0][2] != '\0') return syntax( pgm );
                       level = (argv[0][1] & 0xf);
                       break;
            case 'f':  if (argv[0][2] != '\0') return syntax( pgm );
                       force = 1;
                       break;
            default:   return syntax( pgm );
        }
    }

    if (argc < 1) return syntax( pgm );

    for (i=0; i < argc; i++)
    {
        memset( dev, 0, sizeof( DEVBLK ));
        dev->batch = 1;

        /* open the input file */
        hostpath( dev->filename, argv[i], sizeof( dev->filename ));

        if ((dev->fd = HOPEN( dev->filename, O_RDWR | O_BINARY )) < 0)
        {
            // "%1d:%04X CCKD file %s: error in function %s: %s"
            FWRMSG( stderr, HHC00354, "E", SSID_TO_LCSS( dev->ssid ), dev->devnum,
                dev->filename, "open()", strerror( errno ));
            continue;
        }

        /* read the CKD device header */
        if ((rc = read( dev->fd, &devhdr, CKD_DEVHDR_SIZE )) < CKD_DEVHDR_SIZE)
        {
            // "%1d:%04X CCKD file %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            FWRMSG( stderr, HHC00355, "E", SSID_TO_LCSS( dev->ssid ), dev->devnum,
                dev->filename, "read()", (U64) 0, rc < 0 ? strerror( errno ) : "incomplete" );
            close( dev->fd );
            continue;
        }

        if (!is_devhdrid_typ( devhdr.devhdrid, ANY_CMP_TYP ))
        {
            // "%1d:%04X CCKD file %s: not a compressed dasd file"
            FWRMSG( stderr, HHC00356, "E", SSID_TO_LCSS( dev->ssid ), dev->devnum, dev->filename );
            close( dev->fd );
            continue;
        }

        /* read the compressed CKD device header */
        if ((rc = read (dev->fd, &cdevhdr, CCKD_DEVHDR_SIZE)) < CCKD_DEVHDR_SIZE)
        {
            FWRMSG( stderr, HHC00355, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename,
                    "read()", (U64)CCKD_DEVHDR_POS, rc < 0 ? strerror( errno ) : "incomplete" );
            close (dev->fd);
            continue;
        }

        /* Check the OPENED bit */
        if (!force && (cdevhdr.opts & CCKD_OPENED))
        {
            FWRMSG( stderr, HHC00352, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename );
            close (dev->fd);
            continue;
        }

        /* get the byte order of the file */
        bigend = (cdevhdr.opts & CCKD_BIGENDIAN);

        /* call chkdsk */
        if (cckd_chkdsk (dev, level) < 0)
        {
            FWRMSG( stderr, HHC00353, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename );
            close (dev->fd);
            continue;
        }

        /* re-read the compressed CKD device header */
        if (lseek (dev->fd, CCKD_DEVHDR_POS, SEEK_SET) < 0)
        {
            FWRMSG( stderr, HHC00355, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename,
                    "lseek()", (U64)CCKD_DEVHDR_POS, strerror( errno ));
            close (dev->fd);
            continue;
        }
        if ((rc = read (dev->fd, &cdevhdr, CCKD_DEVHDR_SIZE)) < CCKD_DEVHDR_SIZE)
        {
            FWRMSG( stderr, HHC00355, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename,
                    "read()", (U64)CCKD_DEVHDR_POS, rc < 0 ? strerror( errno ) : "incomplete" );
            close (dev->fd);
            continue;
        }

        /* swap the byte order of the file if chkdsk didn't do it for us */
        if (bigend == (cdevhdr.opts & CCKD_BIGENDIAN))
        {
            WRMSG( HHC00357, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename,
                    cckd_endian() ? "big-endian" : "little-endian" );
            if (cckd_swapend (dev) < 0)
                FWRMSG( stderr, HHC00378, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename );

        }

        close (dev->fd);
    } /* for each arg */

    return 0;
} /* end main */

int syntax( const char* pgm )
{
    WRMSG( HHC02495, "I", pgm );
    return -1;
} /* end function syntax */
