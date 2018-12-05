/* CCKDCDSK64.C (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Greg Smith, 2002-2012                  */
/*              CCKD64 dasd image verification utility.              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Perform check function on a compressed ckd file                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "ccwarn.h"

#define UTILITY_NAME    "cckdcdsk64"
#define UTILITY_DESC    "CCKD64 dasd image verification"

int syntax( const char* pgm );

/*-------------------------------------------------------------------*/
/* Main function for stand-alone chkdsk                              */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             i;                      /* Index                     */
int             rc;                     /* Return code               */
int             maxrc=0;                /* Worst return code         */
int             level=1;                /* Chkdsk level checking     */
int             ro=0;                   /* 1=Open readonly           */
int             force=0;                /* 1=Check if OPENED bit on  */
CKD_DEVHDR      devhdr;                 /* CKD device header         */
CCKD64_DEVHDR   cdevhdr;                /* Compressed CKD device hdr */
DEVBLK          devblk;                 /* DEVBLK                    */
DEVBLK         *dev;                    /* DEVBLK pointer            */

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
            case 'r':  if (argv[0][2] == 'o' && argv[0][3] == '\0')
                           ro = 1;
                       else return syntax( pgm );
                       break;
            default:   return syntax( pgm );
        }
    }

    if (argc < 1) return syntax( pgm );

    dev = &devblk;

    for (i = 0; i < argc; i++)
    {
        memset (dev, 0, sizeof(DEVBLK));
        dev->batch = 1;

        /* open the file */
        hostpath(dev->filename, argv[i], sizeof(dev->filename));
        dev->fd = HOPEN (dev->filename, ro ? O_RDONLY|O_BINARY : O_RDWR|O_BINARY);
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
        if (!is_dh_devid_typ( devhdr.dh_devid, ANY_CMP_OR_SF_TYP ))
        {
            // "Dasd image file format unsupported or unrecognized: %s"
            FWRMSG( stderr, HHC02424, "E", dev->filename );
            close( dev->fd );
            continue;
        }
        dev->cckd64 = 1;

        /* Check CCKD_OPT_OPENED bit if -f not specified */
        if (!force)
        {
            /* PROGRAMMING NOTE: even though we're reading a CKD64
               "CCKD64_DEVHDR" compressed device header here (and
               not a regular "CCKD_DEVHDR" compressed device header)
               (even though our input file might NOT be a CKD64 file),
               the following is still safe to do since the ONLY field
               we are interested in is the 'cdh_opts' options flags
               field, which is identical for both structs.
            */
            if (lseek (dev->fd, CCKD64_DEVHDR_POS, SEEK_SET) < 0)
            {
                // "%1d:%04X CCKD file %s: error in function %s at offset 0x%16.16"PRIX64": %s"
                FWRMSG( stderr, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                        "lseek()", (U64)CCKD64_DEVHDR_POS, strerror( errno ));
                close (dev->fd);
                continue;
            }
            if ((rc = read (dev->fd, &cdevhdr, CCKD64_DEVHDR_SIZE)) < CCKD64_DEVHDR_SIZE)
            {
                // "%1d:%04X CCKD file %s: error in function %s at offset 0x%16.16"PRIX64": %s"
                FWRMSG( stderr, HHC00355, "E", LCSS_DEVNUM, dev->filename,
                        "read()", (U64)CCKD64_DEVHDR_POS, rc < 0 ? strerror( errno ) : "incomplete" );
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

        // "%1d:%04X CCKD file %s: starting %s level %d%s..."
        WRMSG( HHC00379, "I", LCSS_DEVNUM, dev->filename,
            pgm, level, ro ? " (read-only)" : "" );

        if (is_dh_devid_typ( devhdr.dh_devid, ANY64_CMP_OR_SF_TYP ))
            rc = cckd64_chkdsk( dev, level );
        else
            rc = cckd_chkdsk( dev, level );

        close (dev->fd);

        // "%1d:%04X CCKD file %s: %s level %d complete: rc=%d"
        WRMSG( HHC00380, rc ? "W" : "I", LCSS_DEVNUM, dev->filename,
            pgm, level, rc );

        /* Save worst return code */
        if (maxrc >= 0)
            if (rc < 0 || rc > maxrc)
                maxrc = rc;

    } /* for each arg */

    return maxrc;
}

/*-------------------------------------------------------------------*/
/* print syntax                                                      */
/*-------------------------------------------------------------------*/
int syntax( const char* pgm )
{
    WRMSG( HHC02411, "I", pgm );
    return -1;
}
