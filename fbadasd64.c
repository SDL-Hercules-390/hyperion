/* FBADASD64.C  (C) Copyright Roger Bowler, 1999-2012                */
/*              ESA/390 FBA Direct Access Storage Device Handler     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for emulated       */
/* fixed block architecture direct access storage devices.           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      0671 device support by Jay Maynard                           */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _FBADASD64_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "dasdblks.h"  // (need #define DEFAULT_FBA_TYPE)
#include "sr.h"
#include "cckddasd.h"
#include "ccwarn.h"

DISABLE_GCC_UNUSED_SET_WARNING;

/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
int fba64_dasd_init_handler ( DEVBLK *dev, int argc, char *argv[] )
{
int     rc;                             /* Return code               */
struct  stat statbuf;                   /* File information          */
int     startblk;                       /* Device origin block number*/
int     numblks;                        /* Device block count        */
BYTE    c;                              /* Character work area       */
char   *cu = NULL;                      /* Specified control unit    */
int     cfba = 0;                       /* 1 = Compressed fba        */
int     i;                              /* Loop index                */
CKD_DEVHDR      devhdr;                 /* Device header             */
CCKD64_DEVHDR   cdevhdr;                /* Compressed device header  */

    /* For re-initialisation, close the existing file, if any */
    if (dev->fd >= 0)
        (dev->hnd->close)(dev);

    if (!dev->typname || !sscanf(dev->typname,"%hx",&(dev->devtype)))
        dev->devtype = DEFAULT_FBA_TYPE;

    /* reset excps count */
    dev->excps = 0;

    /* The first argument is the file name */
    if (argc == 0 || strlen(argv[0]) >= sizeof(dev->filename))
    {
        // "%1d:%04X FBA file: name missing or invalid filename length"
        WRMSG( HHC00500, "E", LCSS_DEVNUM);
        return -1;
    }

    /* Save the file name in the device block */
    hostpath(dev->filename, argv[0], sizeof(dev->filename));

#if defined( OPTION_SHARED_DEVICES )
    /* Device is shareable */
    dev->shareable = 1;
#endif // defined( OPTION_SHARED_DEVICES )

    /* Check for possible remote device */
    if (stat(dev->filename, &statbuf) < 0)
    {
        rc = shared_fba_init ( dev, argc, argv);
        if (rc < 0)
        {
            // "%1d:%04X FBA file %s not found or invalid"
            WRMSG( HHC00501, "E", LCSS_DEVNUM, dev->filename );
            return -1;
        }
        else
            return rc;
    }

    /* Open the device file */
    dev->fd = HOPEN (dev->filename, O_RDWR|O_BINARY);
    if (dev->fd < 0)
    {
        dev->fd = HOPEN (dev->filename, O_RDONLY|O_BINARY);
        if (dev->fd < 0)
        {
            // "%1d:%04X FBA file %s: error in function %s: %s"
            WRMSG( HHC00502, "E", LCSS_DEVNUM,
                   dev->filename, "open()", strerror( errno ));
            return -1;
        }
    }

    /* Read the first block to see if it's compressed */
    rc = read (dev->fd, &devhdr, CKD_DEVHDR_SIZE);
    if (rc < (int)               CKD_DEVHDR_SIZE)
    {
        /* Handle read error condition */
        if (rc < 0)
            // "%1d:%04X FBA file %s: error in function %s: %s"
            WRMSG( HHC00502, "E", LCSS_DEVNUM,
                   dev->filename, "read()", strerror( errno ));
        else
            // "%1d:%04X FBA file %s: error in function %s: %s"
            WRMSG( HHC00502, "E", LCSS_DEVNUM,
                   dev->filename, "read()", "unexpected end of file" );
        close (dev->fd);
        dev->fd = -1;
        return -1;
    }

    /* Processing for compressed fba64 dasd */
    if (is_dh_devid_typ( devhdr.dh_devid, FBA_C064_TYP ))
    {
        dev->cckd64 = 1;

        cfba = 1;

        /* Read the compressed device header */
        rc = read (dev->fd, &cdevhdr, CCKD64_DEVHDR_SIZE);
        if (rc < (int)                CCKD64_DEVHDR_SIZE)
        {
            /* Handle read error condition */
            if (rc < 0)
                // "%1d:%04X FBA file %s: error in function %s: %s"
                WRMSG( HHC00502, "E", LCSS_DEVNUM,
                       dev->filename, "read()", strerror( errno ));
            else
                // "%1d:%04X FBA file %s: error in function %s: %s"
                WRMSG( HHC00502, "E", LCSS_DEVNUM,
                       dev->filename, "read()", "unexpected end of file" );
            close (dev->fd);
            dev->fd = -1;
            return -1;
        }

        /* Set block size, device origin, and device size in blocks */
        dev->fbablksiz = 512;
        dev->fbaorigin = 0;
        FETCH_LE_FW( dev->fbanumblk, cdevhdr.cdh_cyls );

        /* process the remaining arguments */
        for (i = 1; i < argc; i++)
        {
            if (strlen (argv[i]) > 3
             && memcmp ("sf=", argv[i], 3) == 0)
            {
                /* Parse the shadow file name parameter */
                cckd_sf_parse_sfn( dev, argv[i]+3 );
                continue;
            }
            if (strlen (argv[i]) > 3
             && memcmp("cu=", argv[i], 3) == 0)   /* support for cu= added but  */
            {                                     /* is ignored for the present */
                cu = argv[i]+3;
                continue;
            }

            // "%1d:%04X %s file: parameter %s in argument %d is invalid"
            WRMSG( HHC00503, "E", LCSS_DEVNUM, FBATYP( cfba, 1 ), argv[i], i + 1 );
            return -1;
        }
    }

    /* Processing for regular fba dasd */
    else
    {
        dev->cckd64 = 1;

        /* Determine the device size */
        rc = fstat (dev->fd, &statbuf);
        if (rc < 0)
        {
            // "%1d:%04X FBA file %s: error in function %s: %s"
            WRMSG( HHC00502, "E", LCSS_DEVNUM,
                   dev->filename, "fstat()", strerror( errno ));
            close (dev->fd);
            dev->fd = -1;
            return -1;
        }
#if defined(OPTION_FBA_BLKDEVICE) && defined(BLKGETSIZE)
        if(S_ISBLK(statbuf.st_mode))
        {
            rc=ioctl(dev->fd,BLKGETSIZE,&statbuf.st_size);
            if(rc<0)
            {
                // "%1d:%04X FBA file %s: error in function %s: %s"
                WRMSG( HHC00502, "E", LCSS_DEVNUM,
                       dev->filename, "ioctl()", strerror( errno ));
                close (dev->fd);
                dev->fd = -1;
                return -1;
            }
            dev->fbablksiz = 512;
            dev->fbaorigin = 0;
            dev->fbanumblk = statbuf.st_size;
            if (!dev->quiet)
                // "%1d:%04X FBA file %s: REAL FBA opened"
                WRMSG( HHC00504, "I", LCSS_DEVNUM, dev->filename );
        }
        else
#endif // defined(OPTION_FBA_BLKDEVICE) && defined(BLKGETSIZE)
        {
            /* Set block size, device origin, and device size in blocks */
            dev->fbablksiz = 512;
            dev->fbaorigin = 0;
            dev->fbanumblk = (int)(statbuf.st_size / dev->fbablksiz);
        }

        /* The second argument SHOULD be the device origin block number */
        if (argc >= 2)
        {
            if (1
                && str_ne_n( argv[1], "sf=", 3 )
                && sscanf( argv[1], "%u%c", &startblk, &c ) == 1
                && startblk < dev->fbanumblk
            )
            {
                dev->fbaorigin =  startblk;
                dev->fbanumblk -= startblk;
            }
            else // (error)
            {
                if (str_ne_n( argv[1], "sf=", 3 ))
                {
                    // "%1d:%04X %s file %s: invalid device origin block number %s"
                    WRMSG( HHC00505, "E", LCSS_DEVNUM, FBATYP( cfba, 1 ), dev->filename, argv[1] );
                }
                else
                {
                    // "%1d:%04X %s file %s: shadow files not supported for %s dasd"
                    WRMSG( HHC00469, "E", LCSS_DEVNUM, FBATYP( cfba, 1 ),
                        dev->filename, FBATYP( cfba, 1 ) );
                }

                close( dev->fd );
                dev->fd = -1;
                return -1;
            }
        }

        /* The third argument is the device block count */
        if (argc >= 3 && strcmp(argv[2],"*") != 0)
        {
            if (sscanf(argv[2], "%u%c", &numblks, &c) != 1
             || numblks > dev->fbanumblk)
            {
                // "%1d:%04X %s file %s: invalid device block count %s"
                WRMSG( HHC00506, "E", LCSS_DEVNUM, FBATYP( cfba, 1 ), dev->filename, argv[2] );
                close (dev->fd);
                dev->fd = -1;
                return -1;
            }
            dev->fbanumblk = numblks;
        }
    }

    dev->fbaend = (dev->fbaorigin + dev->fbanumblk) * dev->fbablksiz;

    if (!dev->quiet)
        // "%1d:%04X %s file %s: origin %"PRId64", blks %d"
        WRMSG( HHC00507, "I", LCSS_DEVNUM, FBATYP( cfba, 1 ),
               dev->filename, dev->fbaorigin, dev->fbanumblk );

    /* Set number of sense bytes */
    dev->numsense = 24;

    /* Locate the FBA dasd table entry */
    dev->fbatab = dasd_lookup (DASD_FBADEV, NULL, dev->devtype, dev->fbanumblk);
    if (dev->fbatab == NULL)
    {
        // "%1d:%04X %s file %s: device type %4.4X not found in dasd table"
        WRMSG( HHC00508, "E", LCSS_DEVNUM, FBATYP( cfba, 1 ), dev->filename, dev->devtype );
        close (dev->fd);
        dev->fd = -1;
        return -1;
    }

    /* Build the dh_devid area */
    dev->numdevid = dasd_build_fba_devid (dev->fbatab,(BYTE *)&dev->devid);

    /* Build the devchar area */
    dev->numdevchar = dasd_build_fba_devchar (dev->fbatab,
                                 (BYTE *)&dev->devchar,dev->fbanumblk);

    /* Initialize current blkgrp and cache entry */
    dev->bufcur = dev->cache = -1;

    /* Activate I/O tracing */
//  dev->ccwtrace = 1;

    /* Call the compressed init handler if compressed fba */
    if (cfba)
        return cckd64_dasd_init_handler (dev, argc, argv);

    return 0;
} /* end function fba64_dasd_init_handler */
