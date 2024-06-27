/* CKDDASD64.C  (C) Copyright Roger Bowler, 1999-2012                */
/*              ESA/390 CKD Direct Access Storage Device Handler     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for emulated       */
/* count-key-data direct access storage devices.                     */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Reference information:                                            */
/* GA32-0099 IBM 3990 Storage Control Reference (Models 1, 2, and 3) */
/* GA32-0274 IBM 3990,9390 Storage Control Reference                 */
/* GC26-7006 IBM RAMAC Array Subsystem Reference                     */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Write Update Key and Data CCW by Jan Jaeger                  */
/*      Track overflow support added by Jay Maynard                  */
/*      Track overflow fixes by Jay Maynard, suggested by Valery     */
/*        Pogonchenko                                                */
/*      Track overflow write fix by Roger Bowler, thanks to Valery   */
/*        Pogonchenko and Volker Bandke             V1.71 16/01/2001 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _CKDDASD64_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "devtype.h"
#include "sr.h"
#include "dasdblks.h"
#include "ccwarn.h"
#include "cckddasd.h"

/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
int ckd64_dasd_init_handler ( DEVBLK *dev, int argc, char *argv[] )
{
int             rc;                     /* Return code               */
struct stat     statbuf;                /* File information          */
CKD_DEVHDR      devhdr;                 /* Device header             */
CCKD64_DEVHDR   cdevhdr;                /* Compressed device header  */
int             i;                      /* Loop index                */
BYTE            fileseq;                /* File sequence number      */
char           *sfxptr;                 /* -> Last char of file name */
char            sfxchar;                /* Last char of file name    */
int             heads;                  /* #of heads in CKD file     */
int             trksize;                /* Track size of CKD file    */
int             trks;                   /* #of tracks in CKD file    */
int             cyls;                   /* #of cylinders in CKD file */
int             highcyl;                /* Highest cyl# in CKD file  */
char           *cu = NULL;              /* Specified control unit    */
int             cckd=0;                 /* 1 if compressed CKD       */
char         filename[FILENAME_MAX+3];  /* work area for display     */
BYTE            serial[12+1] = {0};     /* Dasd serial number        */

    dev->rcd = &dasd_build_ckd_config_data;

    /* For re-initialisation, close the existing file, if any */
    if (dev->fd >= 0)
        (dev->hnd->close)(dev);

    if(!sscanf(dev->typname,"%hx",&(dev->devtype)))
        dev->devtype = 0x3380;

    /* The first argument is the file name */
    if (argc == 0 || strlen(argv[0]) >= sizeof(dev->filename))
    {
        // "%1d:%04X CKD file: name missing or invalid filename length"
        WRMSG( HHC00400, "E", LCSS_DEVNUM );
        return -1;
    }

    /* reset excps count */
    dev->excps = 0;

    /* Save the file name in the device block */
    hostpath(dev->filename, argv[0], sizeof(dev->filename));

    if (strchr(dev->filename, SPACE) == NULL)
    {
        MSGBUF(filename, "%s", dev->filename);
    }
    else
    {
        MSGBUF(filename, "'%s'", dev->filename);
    }

#if defined( OPTION_SHARED_DEVICES )
    /* Device is shareable */
    dev->shareable = 1;
#endif // defined( OPTION_SHARED_DEVICES )

    /* Check for possible remote device */
    if (stat(dev->filename, &statbuf) < 0)
    {
        rc = shared_ckd_init ( dev, argc, argv);
        if (rc < 0)
        {
            // "%1d:%04X CKD file %s: open error: not found"
            WRMSG( HHC00401, "E", LCSS_DEVNUM, filename );
            return -1;
        }
        else
            return rc;
    }

    /* No active track or cache entry */
    dev->bufcur = dev->cache = -1;

    /* Locate and save the last character of the file name */
    sfxptr = strrchr (dev->filename, PATHSEPC);
    if (sfxptr == NULL) sfxptr = dev->filename + 1;
    sfxptr = strchr (sfxptr, '.');
    if (sfxptr == NULL) sfxptr = dev->filename + strlen(dev->filename);
    sfxptr--;
    sfxchar = *sfxptr;

    /* process the remaining arguments */
    for (i = 1; i < argc; i++)
    {
        if (strcasecmp ("lazywrite", argv[i]) == 0)
        {
            dev->ckdnolazywr = 0;
            continue;
        }
        if (strcasecmp ("nolazywrite", argv[i]) == 0)
        {
            dev->ckdnolazywr = 1;
            continue;
        }
        if (strcasecmp ("fulltrackio", argv[i]) == 0 ||
            strcasecmp ("fulltrkio",   argv[i]) == 0 ||
            strcasecmp ("ftio",        argv[i]) == 0)
        {
            dev->ckdnolazywr = 0;
            continue;
        }
        if (strcasecmp ("nofulltrackio", argv[i]) == 0 ||
            strcasecmp ("nofulltrkio",   argv[i]) == 0 ||
            strcasecmp ("noftio",        argv[i]) == 0)
        {
            dev->ckdnolazywr = 1;
            continue;
        }
        if (strcasecmp ("readonly", argv[i]) == 0 ||
            strcasecmp ("rdonly",   argv[i]) == 0 ||
            strcasecmp ("ro",       argv[i]) == 0)
        {
            dev->ckdrdonly = 1;
            continue;
        }
        if (strcasecmp ("fakewrite", argv[i]) == 0 ||
            strcasecmp ("fakewrt",   argv[i]) == 0 ||
            strcasecmp ("fw",        argv[i]) == 0)
        {
            if (!dev->ckdrdonly)
            {
                // "%1d:%04X CKD file: 'fakewrite' invalid without 'readonly'"
                WRMSG( HHC00443, "E", LCSS_DEVNUM );
                return -1;
            }

            dev->ckdfakewr = 1;
            continue;
        }
        if (strlen (argv[i]) > 3 &&
            memcmp ("sf=", argv[i], 3) == 0)
        {
            /* Parse the shadow file name parameter */
            cckd_sf_parse_sfn( dev, argv[i]+3 );
            continue;
        }
        if (strlen (argv[i]) > 3
         && memcmp("cu=", argv[i], 3) == 0)
        {
            cu = argv[i]+3;
            continue;
        }
        if (strlen (argv[i]) > 4
         && memcmp("ser=", argv[i], 4) == 0)
        {
            if (1
                && is_numeric( argv[i]+4 )
                && strlen( argv[i]+4 ) == sizeof( dev->serial )
            )
            {
                memcpy( serial, argv[i]+4, sizeof( dev->serial ));
                continue;
            }
        }

        // "%1d:%04X CKD file: parameter %s in argument %d is invalid"
        WRMSG( HHC00402, "E", LCSS_DEVNUM, argv[i], i + 1 );
        return -1;
    }

    /* Initialize the total tracks and cylinders */
    dev->ckdtrks = 0;
    dev->ckdcyls = 0;

    /* Open all of the CKD image files which comprise this volume */
    for (fileseq = 1;;)
    {
        /* Open the CKD image file */
        dev->fd = HOPEN (dev->filename, dev->ckdrdonly ?
                        O_RDONLY|O_BINARY : O_RDWR|O_BINARY);
        if (dev->fd < 0)
        {
            /* Try read-only if shadow file present */
            if (!dev->ckdrdonly && dev->dasdsfn != NULL)
                dev->fd = HOPEN (dev->filename, O_RDONLY|O_BINARY);
            if (dev->fd < 0)
            {
                // "%1d:%04X CKD file %s: error in function %s: %s"
                WRMSG( HHC00404, "E", LCSS_DEVNUM,
                                 filename, "open()", strerror( errno ));
                return -1;
            }
        }

        /* If shadow file, only one base file is allowed */
        if (fileseq > 1 && dev->dasdsfn != NULL)
        {
            // "%1d:%04X CKD file %s: only one base file is allowed"
            WRMSG( HHC00405, "E", LCSS_DEVNUM, filename );
            return -1;
        }

        /* Determine the device size */
        rc = fstat (dev->fd, &statbuf);
        if (rc < 0)
        {
            // "%1d:%04X CKD file %s: error in function %s: %s"
            WRMSG( HHC00404, "E", LCSS_DEVNUM, filename, "fstat()", strerror( errno ));
            return -1;
        }

        /* Read the device header */
        rc = read (dev->fd, &devhdr, CKD_DEVHDR_SIZE);
        if (rc < (int)CKD_DEVHDR_SIZE)
        {
            if (rc < 0)
                // "%1d:%04X CKD file %s: error in function %s: %s"
                WRMSG( HHC00404, "E", LCSS_DEVNUM,
                       filename, "read()", strerror( errno ));
            else
                // "%1d:%04X CKD file %s: error in function %s: %s"
                WRMSG( HHC00404, "E", LCSS_DEVNUM,
                       filename, "read()", "CKD header incomplete" );
            return -1;
        }

        /* Save the serial number */
        if (serial[0]) // (override?)
            memcpy( dev->serial, serial, sizeof( dev->serial ));
        else
            memcpy( dev->serial, devhdr.dh_serial, sizeof( dev->serial ));
        {
            static const BYTE nulls[12] = {0};
            if (memcmp( dev->serial, nulls, 12 ) == 0)
                gen_dasd_serial( dev->serial );
        }

        /* Check the device header identifier */
        dev->cckd64 = 1;
        if (!is_dh_devid_typ( devhdr.dh_devid, CKD64_CMP_OR_NML_TYP ))
        {
            if (is_dh_devid_typ( devhdr.dh_devid, CKD32_CMP_OR_NML_TYP ))
            {
                dev->cckd64 = 0;
                close( dev->fd );
                dev->fd = -1;
                return ckd_dasd_init_handler( dev, argc, argv );
            }

            // "%1d:%04X CKD file %s: ckd header invalid"
            WRMSG( HHC00406, "E", LCSS_DEVNUM, filename );
            return -1;
        }

        if (is_dh_devid_typ( devhdr.dh_devid, CKD_C064_TYP ))
        {
            cckd = 1;

            if (fileseq != 1)
            {
                // "%1d:%04X %s file %s: only 1 CCKD file allowed"
                WRMSG( HHC00407, "E", LCSS_DEVNUM, CKDTYP( cckd, 1 ), filename );
                return -1;
            }
        }

        if (dev->ckdrdonly)
            if (!dev->quiet)
                // "%1d:%04X %s file %s: opened r/o%s"
                WRMSG( HHC00476, "I", LCSS_DEVNUM, CKDTYP( cckd, 1 ),
                    filename, dev->ckdfakewr ? " with fake writing" : "" );

        /* Read the compressed device header */
        if (cckd)
        {
            rc = read (dev->fd, &cdevhdr, CCKD64_DEVHDR_SIZE);
            if (rc < (int)                CCKD64_DEVHDR_SIZE)
            {
                if (rc < 0)
                {
                    // "%1d:%04X CKD file %s: error in function %s: %s"
                    WRMSG( HHC00404, "E", LCSS_DEVNUM,
                           filename, "read()", strerror( errno ));
                }
                else
                {
                    // "%1d:%04X CKD file %s: error in function %s: %s"
                    WRMSG( HHC00404, "E", LCSS_DEVNUM,
                           filename, "read()", "CCKD header incomplete" );
                }
                return -1;
            }
        }

        /* Extract fields from device header */
        FETCH_LE_FW( heads,   devhdr.dh_heads   );
        FETCH_LE_FW( trksize, devhdr.dh_trksize );
        FETCH_LE_HW( highcyl, devhdr.dh_highcyl );

        if (cckd == 0)
        {
            if (dev->dasdcopy == 0)
            {
                trks = (int)((statbuf.st_size - CKD_DEVHDR_SIZE) / trksize);
                cyls = trks / heads;
                if (fileseq == 1 && highcyl == cyls)
                {
                    devhdr.dh_fileseq = 0;
                    highcyl = 0;
                }
            }
            else
            {
                /*
                 * For dasdcopy we get the number of cylinders and tracks from
                 * the highcyl in the device header.  The last file will have
                 * a sequence number of 0xFF.
                 */
                cyls = highcyl - dev->ckdcyls + 1;
                trks = cyls * heads;
                if (devhdr.dh_fileseq == 0xFF)
                {
                    devhdr.dh_fileseq = (fileseq == 1 ? 0 : fileseq);
                    highcyl = 0;
                    store_hw( devhdr.dh_highcyl, 0 );
                    lseek (dev->fd, 0, SEEK_SET);
                    rc = write (dev->fd, &devhdr, CKD_DEVHDR_SIZE);
                }
            }
        }
        else
        {
            FETCH_LE_FW( cyls, cdevhdr.cdh_cyls );
            trks = cyls * heads;
        }

        /* Check for correct file sequence number */
        if (devhdr.dh_fileseq != fileseq
            && !(devhdr.dh_fileseq == 0 && fileseq == 1))
        {
            // "%1d:%04X %s file %s: ckd file out of sequence or bad size"
            WRMSG( HHC00408, "E", LCSS_DEVNUM, CKDTYP( cckd, 1 ), filename );
            return -1;
        }

        if (devhdr.dh_fileseq > 0)
        {
            if (!dev->quiet)
                // "%1d:%04X %s file %s: seq %02d cyls %6d-%-6d"
                WRMSG( HHC00409, "I", LCSS_DEVNUM, CKDTYP( cckd, 1 ),
                       filename, devhdr.dh_fileseq, dev->ckdcyls,
                       (highcyl > 0 ? highcyl : dev->ckdcyls + cyls - 1));
        }

        /* Save device geometry of first file, or check that device
           geometry of subsequent files matches that of first file */
        if (fileseq == 1)
        {
            dev->ckdheads = heads;
            dev->ckdtrksz = trksize;
        }
        else if (heads != dev->ckdheads || trksize != dev->ckdtrksz)
        {
            // "%1d:%04X %s file %s: found heads %d trklen %d, expected heads %d trklen %d"
            WRMSG( HHC00410, "E", LCSS_DEVNUM, CKDTYP( cckd, 1 ),
                   filename, heads, trksize, dev->ckdheads, dev->ckdtrksz );
            return -1;
        }

        /* Consistency check device header */
        if (cckd == 0 && dev->dasdcopy == 0 && (cyls * heads != trks
            || ((off_t)trks * trksize) + CKD_DEVHDR_SIZE
                            != statbuf.st_size
            || (highcyl != 0 && highcyl != dev->ckdcyls + cyls - 1)))
        {
            // "%1d:%04X %s file %s: ckd header inconsistent with file size"
            WRMSG( HHC00411, "E", LCSS_DEVNUM, CKDTYP( cckd, 1 ), filename );
            return -1;
        }

        /* Check for correct high cylinder number */
        if (highcyl != 0 && highcyl != dev->ckdcyls + cyls - 1)
        {
            // "%1d:%04X %s file %s: ckd header high cylinder incorrect"
            WRMSG( HHC00412, "E", LCSS_DEVNUM, CKDTYP( cckd, 1 ), filename );
            return -1;
        }

        /* Accumulate total volume size */
        dev->ckdtrks += trks;
        dev->ckdcyls += cyls;

        /* Save file descriptor and high track number */
        dev->ckdfd[fileseq-1] = dev->fd;
        dev->ckdhitrk[fileseq-1] = dev->ckdtrks;
        dev->ckdnumfd = fileseq;

        /* Exit loop if this is the last file */
        if (highcyl == 0) break;

        /* Increment the file sequence number */
        fileseq++;

        /* Alter the file name suffix ready for the next file */
        if ( fileseq <= 9 )
            *sfxptr = '0' + fileseq;
        else
            *sfxptr = 'A' - 10 + fileseq;

        /* Check that maximum files has not been exceeded */
        if (fileseq > CKD_MAXFILES)
        {
            // "%1d:%04X %s file %s: maximum CKD files exceeded: %d"
            WRMSG( HHC00413, "E", LCSS_DEVNUM, CKDTYP( cckd, 1 ), filename, CKD_MAXFILES );
            return -1;
        }

    } /* end for(fileseq) */

    /* Restore the last character of the file name */
    *sfxptr = sfxchar;

    /* Locate the CKD dasd table entry */
    dev->ckdtab = dasd_lookup (DASD_CKDDEV, NULL, dev->devtype, dev->ckdcyls);
    if (dev->ckdtab == NULL)
    {
        // "%1d:%04X CKD file %s: device type %4.4X not found in dasd table"
        WRMSG( HHC00415, "E", LCSS_DEVNUM,
               filename, dev->devtype );
        return -1;
    }

    /* Log the device geometry */
    if (!dev->quiet)
        // "%1d:%04X %s file %s: model %s cyls %d heads %d tracks %d trklen %d"
        WRMSG( HHC00470, "I", LCSS_DEVNUM, CKDTYP( cckd, 1 ), filename, dev->ckdtab->name,
               dev->ckdcyls, dev->ckdheads, dev->ckdtrks, dev->ckdtrksz );

    /* Locate the CKD control unit dasd table entry */
    dev->ckdcu = dasd_lookup (DASD_CKDCU, cu ? cu : dev->ckdtab->cu, 0, 0);
    if (dev->ckdcu == NULL)
    {
        // "%1d:%04X %s file %s: control unit %s not found in dasd table"
        WRMSG( HHC00416, "E", LCSS_DEVNUM, CKDTYP( cckd, 1 ),
               filename, cu ? cu : dev->ckdtab->cu );
        return -1;
    }

    /* Set number of sense bytes according to controller specification */
    dev->numsense = dev->ckdcu->senselength;

    /* Set flag bit if 3990 controller */
    if (dev->ckdcu->devt == 0x3990)
        dev->ckd3990 = 1;

    /* Build the dh_devid area */
    dev->numdevid = dasd_build_ckd_devid (dev->ckdtab, dev->ckdcu,
                                          (BYTE *)&dev->devid);

    /* Build the devchar area */
    dev->numdevchar = dasd_build_ckd_devchar (dev->ckdtab, dev->ckdcu,
                                  (BYTE *)&dev->devchar, dev->ckdcyls);

    /* Clear the DPA */
    memset(dev->pgid, 0, sizeof(dev->pgid));

    /* Activate I/O tracing */
//  dev->ccwtrace = 1;

    /* Request the channel to merge data chained write CCWs into
       a single buffer before passing data to the device handler */
    dev->cdwmerge = 1;

    /* default for device cache is on */
    dev->devcache = TRUE;

    if (!cckd) return 0;
    else return cckd64_dasd_init_handler(dev, argc, argv);

} /* end function ckd64_dasd_init_handler */
