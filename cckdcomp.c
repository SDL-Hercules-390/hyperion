/* CCKDCOMP.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Greg Smith, 2002-2012                  */
/*              CCKD Compression Utility                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Remove all free space on a compressed ckd file                    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "ccwarn.h"

#define UTILITY_NAME    "cckdcomp"
#define UTILITY_DESC    "CCKD compress program"

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
CKD_DEVHDR      devhdr;                 /* CKD device header         */
CCKD_DEVHDR     cdevhdr;                /* Compressed CKD device hdr */
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

    for (i = 0; i < argc; i++)
    {
        memset (dev, 0, sizeof(DEVBLK));
        dev->batch = 1;

        /* open the file */
        hostpath(dev->filename, argv[i], sizeof(dev->filename));
        dev->fd = HOPEN (dev->filename, O_RDWR|O_BINARY);
        if (dev->fd < 0)
        {
            // "%1d:%04X CCKD file %s: error in function %s: %s"
            FWRMSG( stderr, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    "open()", strerror( errno ));
            continue;
        }

        /* Read the device header */
        rc = read (dev->fd, &devhdr, CKD_DEVHDR_SIZE);
        if (rc < (int)CKD_DEVHDR_SIZE)
        {
            const char* emsg = "CKD header incomplete";
            if (rc < 0)
                emsg = strerror( errno );

            // "%1d:%04X CCKD file %s: error in function %s: %s"
            FWRMSG( stderr, HHC00354, "E", LCSS_DEVNUM, dev->filename,
                    "read()", emsg );
            close( dev->fd );
            continue;
        }

        /* Check the device header identifier */
        if (!is_dh_devid_typ( devhdr.dh_devid, ANY32_CMP_OR_SF_TYP ))
        {
            // "Dasd image file format unsupported or unrecognized: %s"
            FWRMSG( stderr, HHC02424, "E", dev->filename );
            close( dev->fd );
            continue;
        }
        dev->cckd64 = 0;

        /* Check CCKD_OPT_OPENED bit if -f not specified */
        if (!force)
        {
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
            if (cdevhdr.cdh_opts & CCKD_OPT_OPENED)
            {
                // "%1d:%04X CCKD file %s: opened bit is on, use -f"
                FWRMSG( stderr, HHC00352, "E", LCSS_DEVNUM, dev->filename );
                close (dev->fd);
                continue;
            }
        } /* if (!force) */

        /* call chkdsk */
        if (cckd_chkdsk (dev, level) < 0)
        {
            FWRMSG( stderr, HHC00353, "E", LCSS_DEVNUM, dev->filename );
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
