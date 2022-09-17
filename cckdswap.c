/* CCKDSWAP.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              Swap the 'endianness' of a CCKD file.                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module changes the `endianness' of a compressed CKD file.    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "ccwarn.h"

#define UTILITY_NAME    "cckdswap"
#define UTILITY_DESC    "Swap 'endianness' of a CCKD file"

/*-------------------------------------------------------------------*/
/* Swap the `endianness' of  cckd file                               */
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

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* parse the arguments */
    for (argc--, argv++ ; argc > 0 ; argc--, argv++)
    {
        if(**argv != '-') break;

        switch(argv[0][1])
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':  if (argv[0][2] != '\0') return syntax( pgm );
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
            FWRMSG( stderr, HHC00354, "E", LCSS_DEVNUM,
                dev->filename, "open()", strerror( errno ));
            continue;
        }

        /* read the CKD device header */
        if ((rc = read( dev->fd, &devhdr, CKD_DEVHDR_SIZE )) < CKD_DEVHDR_SIZE)
        {
            // "%1d:%04X CCKD file %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            FWRMSG( stderr, HHC00355, "E", LCSS_DEVNUM,
                dev->filename, "read()", (U64) 0, rc < 0 ? strerror( errno ) : "incomplete" );
            close( dev->fd );
            continue;
        }

        if (!is_dh_devid_typ( devhdr.dh_devid, ANY32_CMP_OR_SF_TYP ))
        {
            // "%1d:%04X CCKD file %s: not a compressed dasd file"
            FWRMSG( stderr, HHC00356, "E", LCSS_DEVNUM, dev->filename );
            close( dev->fd );
            continue;
        }

        dev->cckd64 = 0;

        /* read the compressed CKD device header */
        if ((rc = read (dev->fd, &cdevhdr, CCKD_DEVHDR_SIZE)) < CCKD_DEVHDR_SIZE)
        {
            FWRMSG( stderr, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                    "read()", (U64)CCKD_DEVHDR_POS, rc < 0 ? strerror( errno ) : "incomplete" );
            close (dev->fd);
            continue;
        }

        /* Check the OPENED bit */
        if (!force && (cdevhdr.cdh_opts & CCKD_OPT_OPENED))
        {
            // "%1d:%04X CCKD file %s: opened bit is on, use -f"
            FWRMSG( stderr, HHC00352, "E", LCSS_DEVNUM, dev->filename );
            close (dev->fd);
            continue;
        }

        /* get the byte order of the file */
        bigend = (cdevhdr.cdh_opts & CCKD_OPT_BIGEND);

        /* call chkdsk */
        if (cckd_chkdsk (dev, level) < 0)
        {
            // "%1d:%04X CCKD file %s: check disk errors"
            FWRMSG( stderr, HHC00353, "E", LCSS_DEVNUM, dev->filename );
            close (dev->fd);
            continue;
        }

        /* re-read the compressed CKD device header */
        if (lseek (dev->fd, CCKD_DEVHDR_POS, SEEK_SET) < 0)
        {
            // "%1d:%04X CCKD file %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            FWRMSG( stderr, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                    "lseek()", (U64)CCKD_DEVHDR_POS, strerror( errno ));
            close (dev->fd);
            continue;
        }
        if ((rc = read (dev->fd, &cdevhdr, CCKD_DEVHDR_SIZE)) < CCKD_DEVHDR_SIZE)
        {
            // "%1d:%04X CCKD file %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            FWRMSG( stderr, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                    "read()", (U64)CCKD_DEVHDR_POS, rc < 0 ? strerror( errno ) : "incomplete" );
            close (dev->fd);
            continue;
        }

        /* swap the byte order of the file if chkdsk didn't do it for us */
        if (bigend == (cdevhdr.cdh_opts & CCKD_OPT_BIGEND))
        {
            // "%1d:%04X CCKD file %s: converting to %s"
            WRMSG( HHC00357, "I", LCSS_DEVNUM, dev->filename,
                    (cdevhdr.cdh_opts & CCKD_OPT_BIGEND) ?
                        "little-endian" : "big-endian" );

            if (cckd_swapend (dev) < 0)
                // "%1d:%04X CCKD file %s: error during swap"
                FWRMSG( stderr, HHC00378, "E", LCSS_DEVNUM, dev->filename );
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
