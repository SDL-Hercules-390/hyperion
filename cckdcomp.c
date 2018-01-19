/* CCKDCOMP.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Greg Smith, 2002-2012                  */
/*       Perform chkdsk for a Compressed CKD Direct Access Storage   */
/*       Device file.                                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Remove all free space on a compressed ckd file                    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define UTILITY_NAME    "cckdcomp"

int syntax( const char* pgm );

/*-------------------------------------------------------------------*/
/* Main function for stand-alone compress                            */
/*-------------------------------------------------------------------*/

int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             i;                      /* Index                     */
int             rc;                     /* Return code               */
int             level=-1;               /* Level for chkdsk          */
int             force=0;                /* 1=Compress if OPENED set  */
CCKDDASD_DEVHDR cdevhdr;                /* Compressed CKD device hdr */
DEVBLK          devblk;                 /* DEVBLK                    */
DEVBLK         *dev=&devblk;            /* -> DEVBLK                 */

    INITIALIZE_UTILITY( UTILITY_NAME, "CCKD compress program", &pgm );

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

    for (i = 0; i < argc; i++)
    {
        memset (dev, 0, sizeof(DEVBLK));
        dev->batch = 1;

        /* open the file */
        hostpath(dev->filename, argv[i], sizeof(dev->filename));
        dev->fd = HOPEN (dev->filename, O_RDWR|O_BINARY);
        if (dev->fd < 0)
        {
            FWRMSG( stderr, HHC00354, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename,
                    "open()", strerror( errno ));
            continue;
        }

        /* Check CCKD_OPENED bit if -f not specified */
        if (!force)
        {
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
            if (cdevhdr.options & CCKD_OPENED)
            {
                FWRMSG( stderr, HHC00352, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename );
                close (dev->fd);
                continue;
            }
        } /* if (!force) */

        /* call chkdsk */
        if (cckd_chkdsk (dev, level) < 0)
        {
            FWRMSG( stderr, HHC00353, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename );
            close (dev->fd);
            continue;
        }

        /* call compress */
        rc = cckd_comp (dev);

        close (dev->fd);

    } /* for each arg */

    return 0;
}

/*-------------------------------------------------------------------*/
/* print syntax                                                      */
/*-------------------------------------------------------------------*/

int syntax( const char* pgm )
{
    WRMSG( HHC02497, "I", pgm );
    return -1;
}
