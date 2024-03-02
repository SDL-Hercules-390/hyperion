/* CKDDASD.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
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
/* SA22-1025 IBM Subsystem Reference Guide        (E7 Prefix CCW)    */
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

#define _CKDDASD_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "devtype.h"
#include "sr.h"
#include "dasdblks.h"
#include "cckddasd.h"
#include "ccwarn.h"

/*-------------------------------------------------------------------*/
/* Bit definitions for Define Extent File Mask                       */
/*-------------------------------------------------------------------*/
#define CKDMASK_WRCTL           0xC0    /* Write control bits...     */
#define CKDMASK_WRCTL_INHWR0    0x00    /* ...inhibit write HA/R0    */
#define CKDMASK_WRCTL_INHWRT    0x40    /* ...inhibit all writes     */
#define CKDMASK_WRCTL_ALLWRU    0x80    /* ...write update only      */
#define CKDMASK_WRCTL_ALLWRT    0xC0    /* ...allow all writes       */
#define CKDMASK_RESV            0x20    /* Reserved bits - must be 0 */
#define CKDMASK_SKCTL           0x18    /* Seek control bits...      */
#define CKDMASK_SKCTL_ALLSKR    0x00    /* ...allow all seek/recalib */
#define CKDMASK_SKCTL_CYLHD     0x08    /* ...allow seek cyl/head    */
#define CKDMASK_SKCTL_HEAD      0x10    /* ...allow seek head only   */
#define CKDMASK_SKCTL_INHSMT    0x18    /* ...inhibit seek and MT    */
#define CKDMASK_AAUTH           0x06    /* Access auth bits...       */
#define CKDMASK_AAUTH_NORMAL    0x00    /* ...normal authorization   */
#define CKDMASK_AAUTH_DSF       0x02    /* ...device support auth    */
#define CKDMASK_AAUTH_DIAG      0x04    /* ...diagnostic auth        */
#define CKDMASK_AAUTH_DSFNCR    0x06    /* ...device support with no
                                              correction or retry    */
#define CKDMASK_PCI_FETCH       0x01    /* PCI fetch mode            */

/*-------------------------------------------------------------------*/
/* Bit definitions for Define Extent Global Attributes               */
/*-------------------------------------------------------------------*/
#define CKDGATR_ARCH            0xC0    /* Architecture mode...      */
#define CKDGATR_ARCH_ECKD       0xC0    /* ...extended CKD mode      */
#define CKDGATR_CKDCONV         0x20    /* CKD conversion mode       */
#define CKDGATR_SSOP            0x1C    /* Subsystem operation mode..*/
#define CKDGATR_SSOP_NORMAL     0x00    /* ...normal cache           */
#define CKDGATR_SSOP_BYPASS     0x04    /* ...bypass cache           */
#define CKDGATR_SSOP_INHIBIT    0x08    /* ...inhibit cache loading  */
#define CKDGATR_SSOP_SEQACC     0x0C    /* ...sequential access      */
#define CKDGATR_SSOP_LOGGING    0x10    /* ...logging mode           */
#define CKDGATR_USE_CACHE_FW    0x02    /* Use cache fast write      */
#define CKDGATR_INH_DASD_FW     0x01    /* Inhibit DASD fast write   */

/*-------------------------------------------------------------------*/
/* Bit definitions for Locate operation byte                         */
/*-------------------------------------------------------------------*/
#define CKDOPER_ORIENTATION     0xC0    /* Orientation bits...       */
#define CKDOPER_ORIENT_COUNT    0x00    /* ...orient to count        */
#define CKDOPER_ORIENT_HOME     0x40    /* ...orient to home address */
#define CKDOPER_ORIENT_DATA     0x80    /* ...orient to data area    */
#define CKDOPER_ORIENT_INDEX    0xC0    /* ...orient to index        */
#define CKDOPER_CODE            0x3F    /* Operation code bits...    */
#define CKDOPER_ORIENT          0x00    /* ...orient                 */
#define CKDOPER_WRITE           0x01    /* ...write data             */
#define CKDOPER_FORMAT          0x03    /* ...format write           */
#define CKDOPER_RDDATA          0x06    /* ...read data              */
#define CKDOPER_WRTANY          0x09    /* ...write any              */
#define CKDOPER_RDANY           0x0A    /* ...read any               */
#define CKDOPER_WRTTRK          0x0B    /* ...write track            */
#define CKDOPER_RDTRKS          0x0C    /* ...read tracks            */
#define CKDOPER_RDTSET          0x0E    /* ...read track set         */
#define CKDOPER_READ16          0x16    /* ...read(16)               */
#define CKDOPER_EXTOP           0x3F    /* ...extended operation     */

/*-------------------------------------------------------------------*/
/* Bit definitions for Locate auxiliary byte                         */
/*-------------------------------------------------------------------*/
#define CKDLAUX_TLFVALID        0x80    /* TLF field is valid        */
#define CKDLAUX_RESV            0x7E    /* Reserved bits - must be 0 */
#define CKDLAUX_RDCNTSUF        0x01    /* Suffixed read count CCW   */

/*-------------------------------------------------------------------*/
/* Definitions for ckdorient field in device block                   */
/*-------------------------------------------------------------------*/
#define CKDORIENT_NONE          0       /* Orientation unknown       */
#define CKDORIENT_INDEX         1       /* Oriented after track hdr  */
#define CKDORIENT_COUNT         2       /* Oriented after count field*/
#define CKDORIENT_KEY           3       /* Oriented after key field  */
#define CKDORIENT_DATA          4       /* Oriented after data field */
#define CKDORIENT_EOT           5       /* Oriented after end of trk */

/* Path state byte for Sense Path Group ID command */
#define SPG_PATHSTAT            0xC0    /* Pathing status bits...    */
#define SPG_PATHSTAT_RESET      0x00    /* ...reset                  */
#define SPG_PATHSTAT_RESV       0x40    /* ...reserved bit setting   */
#define SPG_PATHSTAT_UNGROUPED  0x80    /* ...ungrouped              */
#define SPG_PATHSTAT_GROUPED    0xC0    /* ...grouped                */
#define SPG_PATHSTAT_RESERVED   0x30    /* ...to channel + pg members*/
#define SPG_PARTSTAT            0x30    /* Partitioning status bits..*/
#define SPG_PARTSTAT_IENABLED   0x00    /* ...implicitly enabled     */
#define SPG_PARTSTAT_RESV       0x10    /* ...reserved bit setting   */
#define SPG_PARTSTAT_DISABLED   0x20    /* ...disabled               */
#define SPG_PARTSTAT_XENABLED   0x30    /* ...explicitly enabled     */
#define SPG_PATHMODE            0x08    /* Path mode bit...          */
#define SPG_PATHMODE_SINGLE     0x00    /* ...single path mode       */
#define SPG_PATHMODE_RESV       0x08    /* ...reserved bit setting   */
#define SPG_RESERVED            0x07    /* Reserved bits, must be 0  */

/* Function control byte for Set Path Group ID command */
#define SPG_SET_MULTIPATH       0x80    /* Set multipath mode        */
#define SPG_SET_COMMAND         0x60    /* Set path command bits...  */
#define SPG_SET_ESTABLISH       0x00    /* ...establish group        */
#define SPG_SET_DISBAND         0x20    /* ...disband group          */
#define SPG_SET_RESIGN          0x40    /* ...resign from group      */
#define SPG_SET_COMMAND_RESV    0x60    /* ...reserved bit setting   */
#define SPG_SET_RESV            0x1F    /* Reserved bits, must be 0  */

/*-------------------------------------------------------------------*/
/* Bit definitions for Diagnostic Control subcommand byte            */
/*-------------------------------------------------------------------*/
#define DIAGCTL_INHIBIT_WRITE   0x02    /* Inhibit Write             */
#define DIAGCTL_SET_GUAR_PATH   0x04    /* Set Guaranteed Path       */
#define DIAGCTL_ENABLE_WRITE    0x08    /* Enable Write              */
#define DIAGCTL_3380_TC_MODE    0x09    /* 3380 Track Compat Mode    */
#define DIAGCTL_INIT_SUBSYS     0x0B    /* Diagnostic Init Subsys    */
#define DIAGCTL_UNFENCE         0x0C    /* Unfence                   */
#define DIAGCTL_ACCDEV_UNKCOND  0x0F    /* Access Device Unknown Cond*/
#define DIAGCTL_MAINT_RESERVE   0x10    /* Media Maintenance Reserve */
#define DIAGCTL_MAINT_RELEASE   0x11    /* Media Maintenance Release */
#define DIAGCTL_MAINT_QUERY     0x12    /* Media Maintenance Query   */

/*-------------------------------------------------------------------*/
/* Definitions for Read Configuration Data command                   */
/*-------------------------------------------------------------------*/
#define CONFIG_DATA_SIZE        256     /* Number of bytes returned
                                           by Read Config Data CCW   */

/*
 * ISW20060207
 * EXTENT_CHECK0 is just to shut up a stupid gcc 4 warning..
 * It doesn't hurt otherwise
 * EXTENT_CHECK0(dev) is the same as EXTENT_CHECK(dev,0,0)
 */
#define EXTENT_CHECK0( _dev )                                         \
    (0                                                                \
     ||  (_dev)->ckdxbcyl >  0                                        \
     || ((_dev)->ckdxbcyl == 0 && (_dev)->ckdxbhead > 0)              \
    )

#define EXTENT_BEGIN_CHECK( _dev, _cyl, _head )                       \
    (0                                                                \
     ||  (_cyl) <  (_dev)->ckdxbcyl                                   \
     || ((_cyl) == (_dev)->ckdxbcyl && (_head) < (_dev)->ckdxbhead)   \
    )

#define EXTENT_END_CHECK( _dev, _cyl, _head )                         \
    (0                                                                \
     ||  (_cyl) >  (_dev)->ckdxecyl                                   \
     || ((_cyl) == (_dev)->ckdxecyl && (_head) > (_dev)->ckdxehead)   \
    )

#define EXTENT_CHECK( _dev, _cyl, _head )                             \
    (0                                                                \
     || EXTENT_BEGIN_CHECK( (_dev), (_cyl), (_head) )                 \
     || EXTENT_END_CHECK(   (_dev), (_cyl), (_head) )                 \
    )

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
static const BYTE   eighthex00[]    = {0x00,0x00,0x00,0x00,
                                       0x00,0x00,0x00,0x00};

/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
int ckd_dasd_init_handler ( DEVBLK *dev, int argc, char *argv[] )
{
int             rc;                     /* Return code               */
struct stat     statbuf;                /* File information          */
CKD_DEVHDR      devhdr;                 /* Device header             */
CCKD_DEVHDR     cdevhdr;                /* Compressed device header  */
int             i;                      /* Loop index                */
int             fileseq;                /* File sequence number      */
char           *sfxptr;                 /* -> Last char of file name */
char            sfxchar;                /* Last char of file name    */
int             heads;                  /* #of heads in CKD file     */
int             trksize;                /* Track size of CKD file    */
int             trks;                   /* #of tracks in CKD file    */
int             cyls;                   /* #of cylinders in CKD file */
int             highcyl;                /* Highest cyl# in CKD file  */
char           *cu = NULL;              /* Specified control unit    */
bool            cckd = false;           /* true if compressed CCKD   */
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

    if (!strchr( dev->filename, SPACE ))
        MSGBUF( filename, "%s", dev->filename );
    else
        MSGBUF( filename, "'%s'", dev->filename );

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
        dev->cckd64 = 0;
        if (!is_dh_devid_typ( devhdr.dh_devid, CKD32_CMP_OR_NML_TYP ))
        {
            if (is_dh_devid_typ( devhdr.dh_devid, CKD64_CMP_OR_NML_TYP ))
            {
                dev->cckd64 = 1;
                close( dev->fd );
                dev->fd = -1;

                if (is_dh_devid_typ( devhdr.dh_devid, CKD_P064_TYP ))
                {
                    cckd = false;

                    if (dev->dasdsfn || dev->dasdsfx)
                    {
                        // "%1d:%04X %s file %s: shadow files not supported for %s dasd"
                        WRMSG( HHC00469, "E", LCSS_DEVNUM, CKDTYP( cckd, dev->cckd64 ),
                            filename, CKDTYP( cckd, dev->cckd64 ) );
                        return -1;
                    }
                }
                return ckd64_dasd_init_handler( dev, argc, argv );
            }

            // "%1d:%04X CKD file %s: ckd header invalid"
            WRMSG( HHC00406, "E", LCSS_DEVNUM, filename );
            return -1;
        }

        if (is_dh_devid_typ( devhdr.dh_devid, CKD_C370_TYP ))
        {
            cckd = true;

            if (fileseq != 1)
            {
                // "%1d:%04X %s file %s: only 1 CCKD file allowed"
                WRMSG( HHC00407, "E", LCSS_DEVNUM, CKDTYP( cckd, 0 ), filename );
                return -1;
            }
        }

        if (!cckd && (dev->dasdsfn || dev->dasdsfx))
        {
            // "%1d:%04X %s file %s: shadow files not supported for %s dasd"
            WRMSG( HHC00469, "E", LCSS_DEVNUM, CKDTYP( cckd, dev->cckd64 ),
                filename, CKDTYP( cckd, dev->cckd64 ) );
            return -1;
        }

        if (dev->ckdrdonly)
            if (!dev->quiet)
                // "%1d:%04X %s file %s: opened r/o%s"
                WRMSG( HHC00403, "I", LCSS_DEVNUM, CKDTYP( cckd, 0 ),
                    filename, dev->ckdfakewr ? " with fake writing" : "" );

        /* Read the compressed device header */
        if (cckd)
        {
            rc = read (dev->fd, &cdevhdr, CCKD_DEVHDR_SIZE);
            if (rc < (int)                CCKD_DEVHDR_SIZE)
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

        if (!cckd)
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
            WRMSG( HHC00408, "E", LCSS_DEVNUM, CKDTYP( cckd, 0 ), filename );
            return -1;
        }

        if (devhdr.dh_fileseq > 0)
        {
            if (!dev->quiet)
                // "%1d:%04X %s file %s: seq %02d cyls %6d-%-6d"
                WRMSG( HHC00409, "I", LCSS_DEVNUM, CKDTYP( cckd, 0 ),
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
            WRMSG( HHC00410, "E", LCSS_DEVNUM, CKDTYP( cckd, 0 ),
                   filename, heads, trksize, dev->ckdheads, dev->ckdtrksz );
            return -1;
        }

        /* Consistency check device header */
        if (!cckd && dev->dasdcopy == 0 && (cyls * heads != trks
            || ((off_t)trks * trksize) + CKD_DEVHDR_SIZE
                            != statbuf.st_size
            || (highcyl != 0 && highcyl != dev->ckdcyls + cyls - 1)))
        {
            // "%1d:%04X %s file %s: ckd header inconsistent with file size"
            WRMSG( HHC00411, "E", LCSS_DEVNUM, CKDTYP( cckd, 0 ), filename );
            return -1;
        }

        /* Check for correct high cylinder number */
        if (highcyl != 0 && highcyl != dev->ckdcyls + cyls - 1)
        {
            // "%1d:%04X %s file %s: ckd header high cylinder incorrect"
            WRMSG( HHC00412, "E", LCSS_DEVNUM, CKDTYP( cckd, 0 ), filename );
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
            WRMSG( HHC00413, "E", LCSS_DEVNUM, CKDTYP( cckd, 0 ), filename, CKD_MAXFILES );
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
        WRMSG( HHC00414, "I", LCSS_DEVNUM, CKDTYP( cckd, 0 ), filename, dev->ckdtab->name,
               dev->ckdcyls, dev->ckdheads, dev->ckdtrks, dev->ckdtrksz );

    /* Locate the CKD control unit dasd table entry */
    dev->ckdcu = dasd_lookup (DASD_CKDCU, cu ? cu : dev->ckdtab->cu, 0, 0);
    if (dev->ckdcu == NULL)
    {
        // "%1d:%04X %s file %s: control unit %s not found in dasd table"
        WRMSG( HHC00416, "E", LCSS_DEVNUM, CKDTYP( cckd, 0 ),
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
    else return cckd_dasd_init_handler(dev, argc, argv);

} /* end function ckd_dasd_init_handler */


/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
void ckd_dasd_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    char       filename[ PATH_MAX + 1 ];/* full path or just name    */
    CCKD_EXT*  cckd;                    /* CCKD Extension Block      */

    BEGIN_DEVICE_CLASS_QUERY( "DASD", dev, devclass, buflen, buffer );

    cckd = dev->cckd_ext;

    if (!cckd)
    {
        if ( dev->ckdnumfd > 1)
        {
            snprintf( buffer, buflen, "%s%s %s%s[%d cyls] [%d segs] IO[%"PRIu64"]",
                      dev->cckd64 ? "*64* " : "",
                      filename,
                      dev->ckdrdonly ? "ro " : "",
                      dev->ckdfakewr ? "fw " : "",
                      dev->ckdcyls,
                      dev->ckdnumfd,
                      dev->excps );
        }
        else
        {
            snprintf( buffer, buflen, "%s%s %s%s[%d cyls] IO[%"PRIu64"]",
                      dev->cckd64 ? "*64* " : "",
                      filename,
                      dev->ckdrdonly ? "ro " : "",
                      dev->ckdfakewr ? "fw " : "",
                      dev->ckdcyls,
                      dev->excps );
        }
    }
    else
    {
        snprintf( buffer, buflen, "%s%s %s%s[%d cyls] [%d sfs] IO[%"PRIu64"]",
                  dev->cckd64 ? "*64* " : "",
                  filename,
                  dev->ckdrdonly ? "ro " : "",
                  dev->ckdfakewr ? "fw " : "",
                  dev->ckdcyls,
                  cckd->sfn,
                  dev->excps );
    }

} /* end function ckd_dasd_query_device */

/*-------------------------------------------------------------------*/
/* Release cache entries                                             */
/*-------------------------------------------------------------------*/
int ckddasd_purge_cache (int *answer, int ix, int i, void *data)
{
U16             devnum;                 /* Cached device number      */
int             trk;                    /* Cached track              */
DEVBLK         *dev = data;             /* -> device block           */

    UNREFERENCED(answer);
    UNREFERENCED( trk ); // (silence "set but not used" warning)
    CKD_CACHE_GETKEY(i, devnum, trk);
    if (dev->devnum == devnum)
        cache_release (ix, i, CACHE_FREEBUF);
    return 0;
}


static int ckd_dasd_read_track (DEVBLK *dev, int trk, BYTE *unitstat);
/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/*-------------------------------------------------------------------*/
int ckd_dasd_close_device ( DEVBLK *dev )
{
int     i;                              /* Index                     */
BYTE    unitstat;                       /* Unit Status               */

    /* Write the last track image if it's modified */
    (dev->hnd->read) (dev, -1, &unitstat);

    /* Free the cache */
    cache_lock(CACHE_DEVBUF);
    cache_scan(CACHE_DEVBUF, ckddasd_purge_cache, dev);
    cache_unlock(CACHE_DEVBUF);

    if (!dev->batch)
        if (!dev->quiet)
            // "%1d:%04X CKD file %s: cache hits %d, misses %d, waits %d"
            WRMSG( HHC00417, "I", LCSS_DEVNUM,
                   dev->filename, dev->cachehits, dev->cachemisses, dev->cachewaits );

    /* Close all of the CKD image files */
    for (i = 0; i < dev->ckdnumfd; i++)
        if (dev->ckdfd[i] > 2)
            close (dev->ckdfd[i]);

    dev->buf = NULL;
    dev->bufsize = 0;

    return 0;
} /* end function ckd_dasd_close_device */


/*-------------------------------------------------------------------*/
/* Read a track image at CCHH                                        */
/*-------------------------------------------------------------------*/
static
int ckd_read_cchh (DEVBLK *dev, int cyl, int head, BYTE *unitstat)
{
int             rc;                     /* Return code               */
int             trk;                    /* Track number              */

    /* Command reject if seek position is outside volume */
    if (cyl >= dev->ckdcyls || head >= dev->ckdheads)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Calculate the track number */
    trk = cyl * dev->ckdheads + head;

    /* Call the read exit */
    rc = (dev->hnd->read) (dev, trk, unitstat);

    return rc;
} /* end function ckd_read_cchh */

/*-------------------------------------------------------------------*/
/* Return adjusted track image length                                */
/*-------------------------------------------------------------------*/
int ckd_trklen (DEVBLK *dev, BYTE *buf)
{
    int sz = ckd_tracklen( dev, buf );

    if (sz > dev->ckdtrksz)
        sz = dev->ckdtrksz;

    return sz;
}

/*-------------------------------------------------------------------*/
/* Read a track image                                                */
/*-------------------------------------------------------------------*/
static
int ckd_dasd_read_track (DEVBLK *dev, int trk, BYTE *unitstat)
{
int             rc;                     /* Return code               */
int             cyl = 0;                /* Cylinder                  */
int             head = 0;               /* Head                      */
U64             offset;                 /* File offsets              */
int             i,o,f;                  /* Indexes                   */
CKD_TRKHDR     *trkhdr;                 /* -> New track header       */

    // "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cur trk %d"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0424( dev, trk );
    else
        LOGDEVTR( HHC00424, "I", dev->filename, trk, dev->bufcur );

    /* Calculate cylinder and head */
    if (dev->ckdheads) /* (might be 0 if init failed!) */
    {
        cyl  = trk / dev->ckdheads;
        head = trk % dev->ckdheads;
    }

    /* Reset buffer offsets */
    dev->bufoff = 0;
    dev->bufoffhi = dev->ckdtrksz;

    /* Return if reading the same track image */
    if (trk >= 0 && trk == dev->bufcur)
        return 0;

    /* Write the previous track image if modified */
    if (dev->bufupd)
    {
        // "Thread "TIDPAT" %1d:%04X CKD file %s: read track updating track %d"
        if (dev->ccwtrace && sysblk.traceFILE)
            tf_0425( dev );
        else
            LOGDEVTR( HHC00425, "I", dev->filename, dev->bufcur );

        dev->bufupd = 0;

        /* Seek to the old track image offset */
        offset = (dev->ckdtrkoff + dev->bufupdlo);
        offset = lseek (dev->fd, offset, SEEK_SET);
        if ((S64)offset < 0)
        {
            /* Handle seek error condition */
            // "%1d:%04X CKD file %s: error in function %s: %s"
            WRMSG( HHC00404, "E", LCSS_DEVNUM,
                   dev->filename, "lseek()", strerror( errno ));
            ckd_build_sense( dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            cache_lock(CACHE_DEVBUF);
            cache_setflag(CACHE_DEVBUF, dev->cache, ~CKD_CACHE_ACTIVE, 0);
            cache_unlock(CACHE_DEVBUF);
            dev->bufupdlo = dev->bufupdhi = 0;
            dev->bufcur = dev->cache = -1;
            return -1;
        }

        /* Write the portion of the track image that was modified */
        rc = write (dev->fd, &dev->buf[dev->bufupdlo],
                    dev->bufupdhi - dev->bufupdlo);
        if (rc < dev->bufupdhi - dev->bufupdlo)
        {
            /* Handle write error condition */
            // "%1d:%04X CKD file %s: error in function %s: %s"
            WRMSG( HHC00404, "E", LCSS_DEVNUM,
                   dev->filename, "write()", strerror( errno ));
            ckd_build_sense( dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            cache_lock(CACHE_DEVBUF);
            cache_setflag(CACHE_DEVBUF, dev->cache, ~CKD_CACHE_ACTIVE, 0);
            cache_unlock(CACHE_DEVBUF);
            dev->bufupdlo = dev->bufupdhi = 0;
            dev->bufcur = dev->cache = -1;
            return -1;
        }

        dev->bufupdlo = dev->bufupdhi = 0;
    }

    cache_lock (CACHE_DEVBUF);

    /* Make the previous cache entry inactive */
    if (dev->cache >= 0)
        cache_setflag(CACHE_DEVBUF, dev->cache, ~CKD_CACHE_ACTIVE, 0);
    dev->bufcur = dev->cache = -1;

    /* Return on special case when called by the close handler */
    if (trk < 0)
    {
        cache_unlock (CACHE_DEVBUF);
        return 0;
    }

ckd_read_track_retry:

    /* Search the cache */
    i = cache_lookup (CACHE_DEVBUF, CKD_CACHE_SETKEY(dev->devnum, trk), &o);

    /* Cache hit */
    if (i >= 0)
    {
        cache_setflag(CACHE_DEVBUF, i, ~0, CKD_CACHE_ACTIVE);
        cache_setage(CACHE_DEVBUF, i);
        cache_unlock(CACHE_DEVBUF);

        // "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cache hit, using cache[%d]"
        if (dev->ccwtrace && sysblk.traceFILE)
            tf_0426( dev, trk, i );
        else
            LOGDEVTR( HHC00426, "I", dev->filename, trk, i );

        dev->cachehits++;
        dev->cache = i;
        dev->buf = cache_getbuf(CACHE_DEVBUF, dev->cache, 0);
        dev->bufcur = trk;
        dev->bufoff = 0;
        dev->bufoffhi = dev->ckdtrksz;
        dev->buflen = ckd_trklen (dev, dev->buf);
        dev->bufsize = cache_getlen(CACHE_DEVBUF, dev->cache);

        /* Set the file descriptor */
        for (f = 0; f < dev->ckdnumfd; f++)
            if (trk < dev->ckdhitrk[f]) break;
        dev->fd = dev->ckdfd[f];

        /* Calculate the track offset */
        dev->ckdtrkoff = (U64)(CKD_DEVHDR_SIZE +
             ((U64)(trk - (f ? dev->ckdhitrk[f-1] : 0))) * dev->ckdtrksz);

        return 0;
     }

    /* Wait if no available cache entry */
    if (o < 0)
    {
        // "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d no available cache entry, waiting"
        if (dev->ccwtrace && sysblk.traceFILE)
            tf_0427( dev, trk );
        else
            LOGDEVTR( HHC00427, "I", dev->filename, trk );

        dev->cachewaits++;
        cache_wait(CACHE_DEVBUF);
        goto ckd_read_track_retry;
    }

    /* Cache miss */

    // "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cache miss, using cache[%d]"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0428( dev, trk, o );
    else
        LOGDEVTR( HHC00428, "I", dev->filename, trk, o );

    dev->cachemisses++;

    /* Make this cache entry active */
    cache_setkey (CACHE_DEVBUF, o, CKD_CACHE_SETKEY(dev->devnum, trk));
    cache_setflag(CACHE_DEVBUF, o, 0, CKD_CACHE_ACTIVE|DEVBUF_TYPE_CKD);
    cache_setage (CACHE_DEVBUF, o);
    dev->buf = cache_getbuf(CACHE_DEVBUF, o, dev->ckdtrksz);
    cache_unlock (CACHE_DEVBUF);

    /* Set the file descriptor */
    for (f = 0; f < dev->ckdnumfd; f++)
        if (trk < dev->ckdhitrk[f]) break;
    dev->fd = dev->ckdfd[f];

    /* Calculate the track offset */
    dev->ckdtrkoff = (U64)(CKD_DEVHDR_SIZE +
         ((U64)(trk - (f ? dev->ckdhitrk[f-1] : 0))) * dev->ckdtrksz);

    // "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d reading file %d offset %"PRId64" len %d"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0429( dev, trk, f+1 );
    else
        LOGDEVTR( HHC00429, "I", dev->filename, trk, f+1, dev->ckdtrkoff, dev->ckdtrksz );

    /* Seek to the track image offset */
    offset = dev->ckdtrkoff;
    offset = lseek (dev->fd, offset, SEEK_SET);
    if ((S64)offset < 0)
    {
        /* Handle seek error condition */
        // "%1d:%04X CKD file %s: error in function %s: %s"
        WRMSG( HHC00404, "E", LCSS_DEVNUM,
               dev->filename, "lseek()", strerror( errno ));
        ckd_build_sense( dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        dev->bufcur = dev->cache = -1;
        cache_lock(CACHE_DEVBUF);
        cache_release(CACHE_DEVBUF, o, 0);
        cache_unlock(CACHE_DEVBUF);
        return -1;
    }

    /* Read the track image */
    if (dev->dasdcopy == 0)
    {
        rc = read (dev->fd, dev->buf, dev->ckdtrksz);
        if (rc < dev->ckdtrksz)
        {
            /* Handle read error condition */
            // "%1d:%04X CKD file %s: error in function %s: %s"
            WRMSG( HHC00404, "E", LCSS_DEVNUM,
                   dev->filename, "read()", (rc < 0 ? strerror( errno ) : "unexpected end of file" ));
            ckd_build_sense( dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            dev->bufcur = dev->cache = -1;
            cache_lock(CACHE_DEVBUF);
            cache_release(CACHE_DEVBUF, o, 0);
            cache_unlock(CACHE_DEVBUF);
            return -1;
        }
    }
    else
    {
        trkhdr = (CKD_TRKHDR*)dev->buf;
        trkhdr->bin = 0;
        store_hw( trkhdr->cyl,  cyl  );
        store_hw( trkhdr->head, head );
        memset (dev->buf + CKD_TRKHDR_SIZE, 0xFF, 8);
    }

    /* Validate the track header */

    // "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d trkhdr %02X %02X%02X %02X%02X"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0430( dev, trk );
    else
        LOGDEVTR( HHC00430, "I", dev->filename, trk,
            dev->buf[0], dev->buf[1], dev->buf[2], dev->buf[3], dev->buf[4] );

    trkhdr = (CKD_TRKHDR*)dev->buf;
    if (0
        || trkhdr->bin              != 0
        || fetch_hw( trkhdr->cyl  ) != cyl
        || fetch_hw( trkhdr->head ) != head
    )
    {
        // "%1d:%04X CKD file %s: invalid track header for cyl %d head %d %02X %02X%02X %02X%02X"
        WRMSG( HHC00418, "E", LCSS_DEVNUM,
               dev->filename, cyl, head, trkhdr->bin,
               trkhdr->cyl[0], trkhdr->cyl[1],
               trkhdr->head[0], trkhdr->head[1] );
        ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        dev->bufcur = dev->cache = -1;
        cache_lock(CACHE_DEVBUF);
        cache_release(CACHE_DEVBUF, o, 0);
        cache_unlock(CACHE_DEVBUF);
        return -1;
    }

    dev->cache = o;
    dev->buf = cache_getbuf(CACHE_DEVBUF, dev->cache, 0);
    dev->bufcur = trk;
    dev->bufoff = 0;
    dev->bufoffhi = dev->ckdtrksz;
    dev->buflen = ckd_trklen (dev, dev->buf);
    dev->bufsize = cache_getlen(CACHE_DEVBUF, dev->cache);

    return 0;
} /* end function ckdread_read_track */


/*-------------------------------------------------------------------*/
/* Update a track image                                              */
/*-------------------------------------------------------------------*/
static
int ckd_dasd_update_track (DEVBLK *dev, int trk, int off,
                      BYTE *buf, int len, BYTE *unitstat)
{
int             rc;                     /* Return code               */

    /* Immediately return if fake writing */
    if (dev->ckdfakewr)
        return len;

    /* Error if opened read-only */
    if (dev->ckdrdonly)
    {
        ckd_build_sense( dev, SENSE_EC, SENSE1_WRI, 0, FORMAT_1, MESSAGE_0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Read the track if it's not current */
    if (trk != dev->bufcur)
    {
        rc = (dev->hnd->read) (dev, trk, unitstat);
        if (rc < 0)
        {
            dev->bufcur = dev->cache = -1;
            return -1;
        }
    }

    /* Invalid track format if going past buffer end */
    if (off + len > dev->bufoffhi)
    {
        ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Copy the data into the buffer */
    if (buf) memcpy (dev->buf + off, buf, len);

    /* Set low and high updated offsets */
    if (!dev->bufupd || off < dev->bufupdlo)
        dev->bufupdlo = off;
    if (off + len > dev->bufupdhi)
        dev->bufupdhi = off + len;

    /* Indicate track image has been modified */
    if (!dev->bufupd)
    {
        dev->bufupd = 1;
        shared_update_notify (dev, trk);
    }

    return len;
} /* end function ckd_update_track */

/*-------------------------------------------------------------------*/
/* CKD start/resume channel program                                  */
/*-------------------------------------------------------------------*/
void ckd_dasd_start (DEVBLK *dev)
{
    /* Reset buffer offsets */
    dev->bufoff = 0;
    dev->bufoffhi = dev->ckdtrksz;
}

/*-------------------------------------------------------------------*/
/* CKD end/suspend channel program                                   */
/*-------------------------------------------------------------------*/
void ckd_dasd_end (DEVBLK *dev)
{
BYTE    unitstat;                       /* Unit Status               */

    /* Write the last track image if it's modified */
    (dev->hnd->read) (dev, -1, &unitstat);
}

/*-------------------------------------------------------------------*/
/* Return used cylinders                                             */
/*-------------------------------------------------------------------*/
int ckd_dasd_used (DEVBLK *dev)
{
    return dev->ckdcyls;
}

/*-------------------------------------------------------------------*/
/* Hercules suspend/resume text unit key values                      */
/*-------------------------------------------------------------------*/
#define SR_DEV_CKD_BUFCUR       ( SR_DEV_CKD | 0x001 )
#define SR_DEV_CKD_BUFOFF       ( SR_DEV_CKD | 0x002 )
#define SR_DEV_CKD_CURCYL       ( SR_DEV_CKD | 0x003 )
#define SR_DEV_CKD_CURHEAD      ( SR_DEV_CKD | 0x004 )
#define SR_DEV_CKD_CURREC       ( SR_DEV_CKD | 0x005 )
#define SR_DEV_CKD_CURKL        ( SR_DEV_CKD | 0x006 )
#define SR_DEV_CKD_ORIENT       ( SR_DEV_CKD | 0x007 )
#define SR_DEV_CKD_CUROPER      ( SR_DEV_CKD | 0x008 )
#define SR_DEV_CKD_CURDL        ( SR_DEV_CKD | 0x009 )
#define SR_DEV_CKD_REM          ( SR_DEV_CKD | 0x00a )
#define SR_DEV_CKD_POS          ( SR_DEV_CKD | 0x00b )
#define SR_DEV_CKD_DXBLKSZ      ( SR_DEV_CKD | 0x010 )
#define SR_DEV_CKD_DXBCYL       ( SR_DEV_CKD | 0x011 )
#define SR_DEV_CKD_DXBHEAD      ( SR_DEV_CKD | 0x012 )
#define SR_DEV_CKD_DXECYL       ( SR_DEV_CKD | 0x013 )
#define SR_DEV_CKD_DXEHEAD      ( SR_DEV_CKD | 0x014 )
#define SR_DEV_CKD_DXFMASK      ( SR_DEV_CKD | 0x015 )
#define SR_DEV_CKD_DXGATTR      ( SR_DEV_CKD | 0x016 )
#define SR_DEV_CKD_LRTRANLF     ( SR_DEV_CKD | 0x020 )
#define SR_DEV_CKD_LROPER       ( SR_DEV_CKD | 0x021 )
#define SR_DEV_CKD_LRAUX        ( SR_DEV_CKD | 0x022 )
#define SR_DEV_CKD_LRCOUNT      ( SR_DEV_CKD | 0x023 )
#define SR_DEV_CKD_3990         ( SR_DEV_CKD | 0x040 )
#define SR_DEV_CKD_XTDEF        ( SR_DEV_CKD | 0x041 )
#define SR_DEV_CKD_SETFM        ( SR_DEV_CKD | 0x042 )
#define SR_DEV_CKD_LOCAT        ( SR_DEV_CKD | 0x043 )
#define SR_DEV_CKD_SPCNT        ( SR_DEV_CKD | 0x044 )
#define SR_DEV_CKD_SEEK         ( SR_DEV_CKD | 0x045 )
#define SR_DEV_CKD_SKCYL        ( SR_DEV_CKD | 0x046 )
#define SR_DEV_CKD_RECAL        ( SR_DEV_CKD | 0x047 )
#define SR_DEV_CKD_RDIPL        ( SR_DEV_CKD | 0x048 )
#define SR_DEV_CKD_XMARK        ( SR_DEV_CKD | 0x049 )
#define SR_DEV_CKD_HAEQ         ( SR_DEV_CKD | 0x04a )
#define SR_DEV_CKD_IDEQ         ( SR_DEV_CKD | 0x04b )
#define SR_DEV_CKD_KYEQ         ( SR_DEV_CKD | 0x04c )
#define SR_DEV_CKD_WCKD         ( SR_DEV_CKD | 0x04d )
#define SR_DEV_CKD_TRKOF        ( SR_DEV_CKD | 0x04e )
#define SR_DEV_CKD_SSI          ( SR_DEV_CKD | 0x04f )
#define SR_DEV_CKD_WRHA         ( SR_DEV_CKD | 0x050 )

/*-------------------------------------------------------------------*/
/* Hercules suspend                                                  */
/*-------------------------------------------------------------------*/
int ckd_dasd_hsuspend( DEVBLK* dev, void* file )
{
    if (dev->bufcur >= 0)
    {
        SR_WRITE_VALUE( file, SR_DEV_CKD_BUFCUR, dev->bufcur, sizeof( dev->bufcur ));
        SR_WRITE_VALUE( file, SR_DEV_CKD_BUFOFF, dev->bufoff, sizeof( dev->bufoff ));
    }

    SR_WRITE_VALUE( file, SR_DEV_CKD_CURCYL,   dev->ckdcurcyl,  sizeof( dev->ckdcurcyl  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_CURHEAD,  dev->ckdcurhead, sizeof( dev->ckdcurhead ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_CURREC,   dev->ckdcurrec,  sizeof( dev->ckdcurrec  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_CURKL,    dev->ckdcurkl,   sizeof( dev->ckdcurkl   ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_ORIENT,   dev->ckdorient,  sizeof( dev->ckdorient  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_CUROPER,  dev->ckdcuroper, sizeof( dev->ckdcuroper ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_CURDL,    dev->ckdcurdl,   sizeof( dev->ckdcurdl   ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_REM,      dev->ckdrem,     sizeof( dev->ckdrem     ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_POS,      dev->ckdpos,     sizeof( dev->ckdpos     ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_DXBLKSZ,  dev->ckdxblksz,  sizeof( dev->ckdxblksz  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_DXBCYL,   dev->ckdxbcyl,   sizeof( dev->ckdxbcyl   ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_DXBHEAD,  dev->ckdxbhead,  sizeof( dev->ckdxbhead  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_DXECYL,   dev->ckdxecyl,   sizeof( dev->ckdxecyl   ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_DXEHEAD,  dev->ckdxehead,  sizeof( dev->ckdxehead  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_DXFMASK,  dev->ckdfmask,   sizeof( dev->ckdfmask   ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_DXGATTR,  dev->ckdxgattr,  sizeof( dev->ckdxgattr  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_LRTRANLF, dev->ckdltranlf, sizeof( dev->ckdltranlf ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_LROPER,   dev->ckdloper,   sizeof( dev->ckdloper   ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_LRAUX,    dev->ckdlaux,    sizeof( dev->ckdlaux    ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_LRCOUNT,  dev->ckdlcount,  sizeof( dev->ckdlcount  ));
    SR_WRITE_VALUE( file, SR_DEV_CKD_3990,     dev->ckd3990,                  1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_XTDEF,    dev->ckdxtdef,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_SETFM,    dev->ckdsetfm,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_LOCAT,    dev->ckdlocat,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_SPCNT,    dev->ckdspcnt,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_SEEK,     dev->ckdseek,                  1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_SKCYL,    dev->ckdskcyl,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_RECAL,    dev->ckdrecal,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_RDIPL,    dev->ckdrdipl,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_XMARK,    dev->ckdxmark,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_HAEQ,     dev->ckdhaeq,                  1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_IDEQ,     dev->ckdideq,                  1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_KYEQ,     dev->ckdkyeq,                  1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_WCKD,     dev->ckdwckd,                  1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_TRKOF,    dev->ckdtrkof,                 1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_SSI,      dev->ckdssi,                   1          );
    SR_WRITE_VALUE( file, SR_DEV_CKD_WRHA,     dev->ckdwrha,                  1          );

    return 0;
}

/*-------------------------------------------------------------------*/
/* Hercules resume                                                   */
/*-------------------------------------------------------------------*/
int ckd_dasd_hresume(DEVBLK *dev, void *file)
{
u_int   rc;
size_t  key, len;
BYTE byte;

    do {
        SR_READ_HDR(file, key, len);
        switch (key) {
        case SR_DEV_CKD_BUFCUR:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            rc = (dev->hnd->read) ? (dev->hnd->read)(dev, rc, &byte) : -1;
            if ((int)rc < 0) return -1;
            break;
        case SR_DEV_CKD_BUFOFF:
            SR_READ_VALUE(file, len, &dev->bufoff, sizeof(dev->bufoff));
            break;
        case SR_DEV_CKD_CURCYL:
            SR_READ_VALUE(file, len, &dev->ckdcurcyl, sizeof(dev->ckdcurcyl));
            break;
        case SR_DEV_CKD_CURHEAD:
            SR_READ_VALUE(file, len, &dev->ckdcurhead, sizeof(dev->ckdcurhead));
            break;
        case SR_DEV_CKD_CURREC:
            SR_READ_VALUE(file, len, &dev->ckdcurrec, sizeof(dev->ckdcurrec));
            break;
        case SR_DEV_CKD_CURKL:
            SR_READ_VALUE(file, len, &dev->ckdcurkl, sizeof(dev->ckdcurkl));
            break;
        case SR_DEV_CKD_ORIENT:
            SR_READ_VALUE(file, len, &dev->ckdorient, sizeof(dev->ckdorient));
            break;
        case SR_DEV_CKD_CUROPER:
            SR_READ_VALUE(file, len, &dev->ckdcuroper, sizeof(dev->ckdcuroper));
            break;
        case SR_DEV_CKD_CURDL:
            SR_READ_VALUE(file, len, &dev->ckdcurdl, sizeof(dev->ckdcurdl));
            break;
        case SR_DEV_CKD_REM:
            SR_READ_VALUE(file, len, &dev->ckdrem, sizeof(dev->ckdrem));
            break;
        case SR_DEV_CKD_POS:
            SR_READ_VALUE(file, len, &dev->ckdpos, sizeof(dev->ckdpos));
            break;
        case SR_DEV_CKD_DXBLKSZ:
            SR_READ_VALUE(file, len, &dev->ckdxblksz, sizeof(dev->ckdxblksz));
            break;
        case SR_DEV_CKD_DXBCYL:
            SR_READ_VALUE(file, len, &dev->ckdxbcyl, sizeof(dev->ckdxbcyl));
            break;
        case SR_DEV_CKD_DXBHEAD:
            SR_READ_VALUE(file, len, &dev->ckdxbhead, sizeof(dev->ckdxbhead));
            break;
        case SR_DEV_CKD_DXECYL:
            SR_READ_VALUE(file, len, &dev->ckdxecyl, sizeof(dev->ckdxecyl));
            break;
        case SR_DEV_CKD_DXEHEAD:
            SR_READ_VALUE(file, len, &dev->ckdxehead, sizeof(dev->ckdxehead));
            break;
        case SR_DEV_CKD_DXFMASK:
            SR_READ_VALUE(file, len, &dev->ckdfmask, sizeof(dev->ckdfmask));
            break;
        case SR_DEV_CKD_DXGATTR:
            SR_READ_VALUE(file, len, &dev->ckdxgattr, sizeof(dev->ckdxgattr));
            break;
        case SR_DEV_CKD_LRTRANLF:
            SR_READ_VALUE(file, len, &dev->ckdltranlf, sizeof(dev->ckdltranlf));
            break;
        case SR_DEV_CKD_LROPER:
            SR_READ_VALUE(file, len, &dev->ckdloper, sizeof(dev->ckdloper));
            break;
        case SR_DEV_CKD_LRAUX:
            SR_READ_VALUE(file, len, &dev->ckdlaux, sizeof(dev->ckdlaux));
            break;
        case SR_DEV_CKD_LRCOUNT:
            SR_READ_VALUE(file, len, &dev->ckdltranlf, sizeof(dev->ckdltranlf));
            break;
        case SR_DEV_CKD_3990:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckd3990 = rc;
            break;
        case SR_DEV_CKD_XTDEF:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdxtdef = rc;
            break;
        case SR_DEV_CKD_SETFM:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdsetfm = rc;
            break;
        case SR_DEV_CKD_LOCAT:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdlocat = rc;
            break;
        case SR_DEV_CKD_SPCNT:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdspcnt = rc;
            break;
        case SR_DEV_CKD_SEEK:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdseek = rc;
            break;
        case SR_DEV_CKD_SKCYL:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdskcyl = rc;
            break;
        case SR_DEV_CKD_RECAL:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdrecal = rc;
            break;
        case SR_DEV_CKD_RDIPL:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdrdipl = rc;
            break;
        case SR_DEV_CKD_XMARK:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdxmark = rc;
            break;
        case SR_DEV_CKD_HAEQ:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdhaeq = rc;
            break;
        case SR_DEV_CKD_IDEQ:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdideq = rc;
            break;
        case SR_DEV_CKD_KYEQ:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdkyeq = rc;
            break;
        case SR_DEV_CKD_WCKD:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdwckd = rc;
            break;
        case SR_DEV_CKD_TRKOF:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdtrkof = rc;
            break;
        case SR_DEV_CKD_SSI:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdssi = rc;
            break;
        case SR_DEV_CKD_WRHA:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->ckdwrha = rc;
            break;
        default:
            SR_READ_SKIP(file, len);
            break;
        } /* switch (key) */
    } while ((key & SR_DEV_MASK) == SR_DEV_CKD);
    return 0;
}

/*-------------------------------------------------------------------*/
/* Build sense data                                                  */
/*-------------------------------------------------------------------*/
void ckd_build_sense( DEVBLK *dev, BYTE sense0, BYTE sense1,
                      BYTE sense2, BYTE format, BYTE message )
{
int shift;  /* num of bits to shift left 'high cyl' in sense6 */

    /* Clear the sense bytes */
    memset( dev->sense, 0, sizeof(dev->sense) );

    /* Sense bytes 0-2 are specified by caller */
    dev->sense[0] = sense0;
    dev->sense[1] = sense1;
    dev->sense[2] = sense2;

    /* Sense byte 3 contains the residual locate record count
       if imprecise ending is indicated in sense byte 1 */
    if (sense1 & SENSE1_IE)
    {
        if (dev->ckdtrkof)
            dev->sense[3] = (BYTE) dev->ckdcuroper;
        else
            dev->sense[3] = dev->ckdlcount;
    }

    /* Sense byte 4 is the physical device address */
    dev->sense[4] = 0;

    if (dev->devtype == 0x2305)
    {
        /*             0x40=ONLINE             0x04=END OF CYL */
        dev->sense[3] = (((dev->sense[1]) & 0x20) >> 3) | 0x40;
    }
    if (dev->devtype == 0x2311)
    {
        /* 0x80=READY, 0x40=ONLINE 0x08=ONLINE 0x04=END OF CYL */
        dev->sense[3] = (((dev->sense[1]) & 0x20) >> 3) | 0xC8;
    }
    if (dev->devtype == 0x2314)
    {
        /*             0x40=ONLINE             0x04=END OF CYL */
        dev->sense[3] = (((dev->sense[1]) & 0x20) >> 3) | 0x40;
    }
    if (dev->devtype == 0x3330)
    {
        /* bits 0-1 = controller address */
        /* bits 2-7: drive A = 111000, B = 110001, ... H = 000111 */
        dev->sense[4] = (dev->devnum & 0x07) | ((~(dev->devnum) & 0x07) << 3);
    }
    if (dev->devtype == 0x3340)
    {
        /* X'01' = 35 MB drive, X'02' = 70 MB drive  (same as 'model') */
        /* X'80' RPS feature installed */
        dev->sense[2] |= 0x80 | dev->devid[6];  /* RPS + model */

        /* drive A = bit 0 (0x80), ... drive H = bit 7 (0x01) */
        dev->sense[4] =  0x80 >> (dev->devnum & 0x07);
    }
    if (dev->devtype == 0x3350)
    {
        /* drive 0 = bit 0 (0x80), ... drive 7 = bit 7 (0x01) */
        dev->sense[4] =  0x80 >> (dev->devnum & 0x07);
    }
    if (dev->devtype == 0x3375)
    {
        /* bits 3-4 = controller address, bits 5-7 = device address */
        dev->sense[4] = dev->devnum & 0x07;
    }
    if (dev->devtype == 0x3380)
    {
        /* bits 4-7 = device address */
        dev->sense[4] = dev->devnum & 0x0F;
    }

    /* Sense byte 5 contains bits 8-15 of the cylinder address
       and sense byte 6 contains bits 4-7 of the cylinder address,
       followed by bits 12-15 of the head address -- UNLESS...
       the device has more than 4095 cylinders, in which case
       sense bytes 5 and 6 both contain X'FF'.
    */
    if (dev->ckdcyls > 4095)
    {
        dev->sense[5] = 0xFF;
        dev->sense[6] = 0xFF;
    }
    else
    {
        if (0
            || (dev->devtype == 0x2311 )
            || (dev->devtype == 0x2314 )
            || (dev->devtype == 0x2305 )
        )
        {
            /* (do nothing) */
        }
        else
        {
            dev->sense[5] = dev->ckdcurcyl & 0xFF;

            /* sense byte 6 bits     c = cyl high byte, h=head    */
            /*                          0 1 2 3 4 5 6 7   shift   */
            /* 3330-1                   - c - h h h h h      6    */
            /* 3330-11 3350             - c c h h h h h      5    */
            /* 3340                     - c c - h h h h      5    */
            /* 3375                     c c - - h h h h      6    */
            /* 3380                     c c c c h h h h      4    */
            switch (dev->devtype)
            {
                case 0x3330:

                    if (dev->devid[6] == 0x01)
                        shift = 6;        /* 3330-1  */
                    else
                        shift = 5;        /* 3330-11 */
                    break;

                case 0x3340:
                case 0x3350:

                    shift = 5;
                    break;

                case 0x3375:

                    shift = 6;
                    break;

                default:

                    shift = 4;
                    break;
            }

            dev->sense[6] = (BYTE)(( (dev->ckdcurcyl >> 8) << shift )
                          | (dev->ckdcurhead & 0x1F));
        }
    }

    /* Sense byte 7 contains the format code and message type */
    dev->sense[7] = (format << 4) | (message & 0x0F);

    /* Sense bytes 8-23 depend on the format code */
    switch (format)
    {
        case FORMAT_4: /* Data check */
        case FORMAT_5: /* Data check with displacement information */

            /* Sense bytes 8-12 contain the CCHHR of the record in error */
            store_hw( &dev->sense[ 8], (U16)dev->ckdcurcyl  );
            store_hw( &dev->sense[10], (U16)dev->ckdcurhead );
            dev->sense[12] = (BYTE) dev->ckdcurrec;
            break;
    }

    /* Sense byte 27 bit 0 indicates 24-byte compatibility sense data*/
    dev->sense[27] = 0x80;

    /* Sense bytes 29-30 contain the cylinder address */
    store_hw( &dev->sense[29], (U16)dev->ckdcurcyl  );

    /* Sense byte 31 contains the head address */
    dev->sense[31] = dev->ckdcurhead & 0xFF;

} /* end function ckd_build_sense */

/*-------------------------------------------------------------------*/
/* Seek to a specified cylinder and head                             */
/*-------------------------------------------------------------------*/
static int ckd_seek ( DEVBLK *dev, int cyl, int head,
                      CKD_TRKHDR *trkhdr, BYTE *unitstat )
{
int             rc;                     /* Return code               */

    // "Thread "TIDPAT" %1d:%04X CKD file %s: seeking to cyl %d head %d"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0431( dev, cyl, head );
    else
        LOGDEVTR( HHC00431, "I", dev->filename, cyl, head );

    /* Read the track image */
    rc = ckd_read_cchh (dev, cyl, head, unitstat);
    if (rc < 0) return -1;

    /* Set device orientation fields */
    dev->ckdcurcyl = cyl;
    dev->ckdcurhead = head;
    dev->ckdcurrec = 0;
    dev->ckdcurkl = 0;
    dev->ckdcurdl = 0;
    dev->ckdrem = 0;
    dev->ckdorient = CKDORIENT_INDEX;

    /* Copy the track header */
    if (trkhdr) memcpy (trkhdr, &dev->buf[dev->bufoff], CKD_TRKHDR_SIZE);

    /* Increment offset past the track header */
    dev->bufoff += CKD_TRKHDR_SIZE;

    return 0;
} /* end function ckd_seek */


/*-------------------------------------------------------------------*/
/* Advance to next track for multitrack operation                    */
/*-------------------------------------------------------------------*/
static int mt_advance ( DEVBLK *dev, BYTE *unitstat, int trks )
{
int             rc;                     /* Return code               */
int             cyl;                    /* Next cyl for multitrack   */
int             head;                   /* Next head for multitrack  */

    /* File protect error if not within domain of Locate Record
       and file mask inhibits seek and multitrack operations */
    if (dev->ckdlcount == 0 &&
        (dev->ckdfmask & CKDMASK_SKCTL) == CKDMASK_SKCTL_INHSMT)
    {
        // "Thread "TIDPAT" %1d:%04X CKD file %s: error: MT advance: locate record %d file mask %02X"
        if (dev->ccwtrace && sysblk.traceFILE)
            tf_0432( dev );
        else
            LOGDEVTR( HHC00432, "E", dev->filename, dev->ckdlcount, dev->ckdfmask );

        if (dev->ckdtrkof)
            ckd_build_sense( dev, 0, SENSE1_FP | SENSE1_IE, 0, 0, 0 );
        else
           ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* End of cylinder error if not within domain of Locate Record
       and current track is last track of cylinder */
    if (dev->ckdlcount == 0
        && dev->ckdcurhead + trks >= dev->ckdheads)
    {
        if (dev->ckdtrkof)
            ckd_build_sense( dev, 0, SENSE1_EOC | SENSE1_IE, 0, 0, 0 );
        else
            ckd_build_sense( dev, 0, SENSE1_EOC, 0, 0, 0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Advance to next track */
    cyl  = dev->ckdcurcyl;
    head = dev->ckdcurhead + trks;

    while (head >= dev->ckdheads)
    {
        head -= dev->ckdheads;
        cyl++;
    }

    // "Thread "TIDPAT" %1d:%04X CKD file %s: MT advance to cyl(%d) head(%d)"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0433( dev, cyl, head );
    else
        LOGDEVTR( HHC00433, "I", dev->filename, cyl, head );

    /* File protect error if next track is outside the
       limits of the device or outside the defined extent */
    if (EXTENT_CHECK( dev, cyl, head ))
    {
        if (dev->ckdtrkof)
            ckd_build_sense( dev, 0, SENSE1_FP | SENSE1_IE, 0, 0, 0 );
        else
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Seek to next track */
    rc = ckd_seek( dev, cyl, head, NULL, unitstat );
    if (rc < 0)
        return -1;

    /* Successful return */
    return 0;

} /* end function mt_advance */

/*-------------------------------------------------------------------*/
/* Read count field                                                  */
/*-------------------------------------------------------------------*/
static int ckd_read_count ( DEVBLK *dev, BYTE code,
                            CKD_RECHDR *rechdr, BYTE *unitstat)
{
int             rc;                     /* Return code               */
bool            skipr0 = false;         /* true=Skip record zero     */
int             cyl;                    /* Cylinder number for seek  */
int             head;                   /* Head number for seek      */
char           *orient[] = {"none", "index", "count", "key", "data", "eot"};

    /* Skip record 0 for all operations EXCEPT:
       READ TRACK, READ R0, SEARCH ID EQUAL, SEARCH ID HIGH, SEARCH
       ID EQUAL OR HIGH, LOCATE RECORD, and WRITE CKD NEXT TRACK.
       Otherwise, if it *is* any of the above, then DON'T skip R0. */
    if (1
        &&  code         != 0xDE // READ TRACK
        && (code & 0x7F) != 0x16 // READ RECORD ZERO
        && (code & 0x7F) != 0x31 // SEARCH ID EQUAL
        && (code & 0x7F) != 0x51 // SEARCH ID HIGH
        && (code & 0x7F) != 0x71 // SEARCH ID EQUAL OR HIGH
        &&  code         != 0x47 // LOCATE RECORD
        &&  code         != 0x4B // LOCATE RECORD EXTENDED
        &&  code         != 0x9D // WRITE COUNT KEY AND DATA NEXT TRACK
        &&  code         != 0xE7 // PREFIX
    )
        skipr0 = true;

    // "Thread "TIDPAT" %1d:%04X CKD file %s: read count orientation %s"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0434( dev );
    else
        LOGDEVTR( HHC00434, "I", dev->filename, orient[dev->ckdorient] );

    /* If orientation is at End-Of_Track then a multi-track advance
       failed previously during synchronous I/O */
    if (dev->ckdorient == CKDORIENT_EOT)
    {
        rc = mt_advance( dev, unitstat, 1 );
        if (rc < 0)
            return -1;
    }

    /* Search for next count field */
    for ( ; ; )
    {
        /* If oriented to count or key field, skip key and data */
        if (dev->ckdorient == CKDORIENT_COUNT)
            dev->bufoff += dev->ckdcurkl + dev->ckdcurdl;
        else if (dev->ckdorient == CKDORIENT_KEY)
            dev->bufoff += dev->ckdcurdl;

        /* Make sure we don't copy past the end of the buffer */
        if (dev->bufoff + CKD_RECHDR_SIZE >= dev->bufoffhi)
        {
            /* Handle error condition */
            // "%1d:%04X CKD file %s: error attempting to read past end of track %d %d"
            WRMSG( HHC00419, "E", LCSS_DEVNUM,
                   dev->filename, dev->bufoff, dev->bufoffhi );

            /* Set unit check with equipment check */
            ckd_build_sense( dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return -1;
        }

        /* Copy the record header (count field) */
        memcpy (rechdr, &dev->buf[dev->bufoff], CKD_RECHDR_SIZE);
        dev->bufoff += CKD_RECHDR_SIZE;

        /* Set the device orientation fields */
        dev->ckdcurrec = rechdr->rec;
        dev->ckdrem = 0;
        dev->ckdorient = CKDORIENT_COUNT;
        dev->ckdcurkl = rechdr->klen;
        dev->ckdcurdl = (rechdr->dlen[0] << 8) + rechdr->dlen[1];

        if(dev->ckdcyls < 32768)
            dev->ckdtrkof = (rechdr->cyl[0] == 0xFF) ? 0 : rechdr->cyl[0] >> 7;
        else
            dev->ckdtrkof = 0;

        // "Thread "TIDPAT" %1d:%04X CKD file %s: cyl %d head %d record %d kl %d dl %d of %d"
        if (dev->ccwtrace && sysblk.traceFILE)
            tf_0435( dev );
        else
            LOGDEVTR( HHC00435, "I",  dev->filename,
                      dev->ckdcurcyl, dev->ckdcurhead, dev->ckdcurrec,
                      dev->ckdcurkl,  dev->ckdcurdl,   dev->ckdtrkof );

        /* Skip record zero if user data record required */
        if (skipr0 && rechdr->rec == 0)
            continue;

        /* Test for logical end of track and exit if not */
        if (memcmp( rechdr, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) != 0)
            break;

        dev->ckdorient = CKDORIENT_EOT;

        /* For READ TRACK or READ MULTIPLE CKD, return with the
           end of track marker in the record header field */
        if (0
            || code == 0xDE // READ TRACK
            || code == 0x5E // READ MULTIPLE COUNT KEY AND DATA
        )
            break;

        /*  ((( Something to do with LR/LRE Read Count Suffix?? )))  */
        if (1
            && IS_CCW_MTRACK( code )
            && (dev->ckdlaux & CKDLAUX_RDCNTSUF)
            && (0
                || dev->ckdlcount == 1
                || (0
                    || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDANY
                    || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTANY
                   )
               )
            &&  dev->ckdfcoun
        )
        {
            memcpy( rechdr, dev->ckdfcwrk, CKD_RECHDR_SIZE );

            cyl  = fetch_hw( rechdr->cyl );
            head = fetch_hw( rechdr->head );

            if ((rc = ckd_seek( dev, cyl, head, NULL, unitstat )) < 0)
                return -1;

            continue;
        }

        /* End of track found, so terminate with no record found
           error if this is a LOCATE RECORD or WRITE CKD NEXT TRACK
           command; or if this is the second end of track in this
           channel program without an intervening read of the home
           address or data area and without an intervening write,
           sense, or control command --
           -- except when multitrack READ or SEARCH [KEY?] command
           operates outside the domain of a locate record
        */
        if (0
            || code == 0x47 // LOCATE RECORD
            || code == 0x4B // LOCATE RECORD EXTENDED
            || code == 0xE7 // PREFIX
            || code == 0x9D // WRITE COUNT KEY AND DATA NEXT TRACK
            || (1
                && dev->ckdxmark
                && !(1
                     && dev->ckdlcount == 0
                     && (0
                         || (1
                             && IS_CCW_READ( code )
                             && IS_CCW_MTRACK( code )
                            )
                         || code == 0xA9 // SEARCH KEY EQUAL
                         || code == 0xC9 // SEARCH KEY HIGH
                         || code == 0xE9 // SEARCH KEY EQUAL OR HIGH
                        )
                    )
               )
        )
        {
            ckd_build_sense( dev, 0, SENSE1_NRF, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return -1;
        }

        /* Test for multitrack operation */
        if (!IS_CCW_MTRACK( code ))
        {
            /* If non-multitrack, return to start of current track */
            cyl = dev->ckdcurcyl;
            head = dev->ckdcurhead;
            rc = ckd_seek (dev, cyl, head, NULL, unitstat);
            if (rc < 0)
                return -1;

            /* Set index marker found flag */
            dev->ckdxmark = 1;
        }
        else // (multitrack)
        {
            /* If multitrack, attempt to advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0)
                return -1;

            /* Set index marker flag if non-search command */
            if (1
                && (code & 0x7F) != 0x31 // SEARCH ID EQUAL
                && (code & 0x7F) != 0x51 // SEARCH ID HIGH
                && (code & 0x7F) != 0x71 // SEARCH ID EQUAL OR HIGH
                && (code & 0x7F) != 0x29 // SEARCH KEY EQUAL
                && (code & 0x7F) != 0x49 // SEARCH KEY HIGH
                && (code & 0x7F) != 0x69 // SEARCH KEY EQUAL OR HIGH
            )
                dev->ckdxmark = 1;
        }

    } /* end for */

    return 0;

} /* end function ckd_read_count */


/*-------------------------------------------------------------------*/
/* Read key field                                                    */
/*-------------------------------------------------------------------*/
static int ckd_read_key ( DEVBLK *dev, BYTE code,
                BYTE *buf, BYTE *unitstat)
{
int             rc;                     /* Return code               */
CKD_RECHDR      rechdr;                 /* CKD record header         */

    /* If not oriented to count field, read next count field */
    if (dev->ckdorient != CKDORIENT_COUNT)
    {
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) return rc;
    }

    // "Thread "TIDPAT" %1d:%04X CKD file %s: read key %d bytes"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0436( dev );
    else
        LOGDEVTR( HHC00436, "I", dev->filename, dev->ckdcurkl );

    /* Read key field */
    if (dev->ckdcurkl > 0)
    {
        if (dev->bufoffhi - dev->bufoff < dev->ckdcurkl)
        {
            /* Handle error condition */
            // "%1d:%04X CKD file %s: error attempting to read past end of track %d %d"
            WRMSG( HHC00419, "E", LCSS_DEVNUM,
                   dev->filename, dev->bufoff, dev->bufoffhi );

            /* Set unit check with equipment check */
            ckd_build_sense( dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return -1;
        }

        memcpy (buf, &dev->buf[dev->bufoff], dev->ckdcurkl);
        dev->bufoff += dev->ckdcurkl;
    }

    /* Set the device orientation fields */
    dev->ckdrem = 0;
    dev->ckdorient = CKDORIENT_KEY;

    return 0;
} /* end function ckd_read_key */


/*-------------------------------------------------------------------*/
/* Read data field                                                   */
/*-------------------------------------------------------------------*/
static int ckd_read_data ( DEVBLK *dev, BYTE code,
                BYTE *buf, BYTE *unitstat)
{
int             rc;                     /* Return code               */
CKD_RECHDR      rechdr;                 /* Record header             */

    /* If not oriented to count or key field, read next count field */
    if (dev->ckdorient != CKDORIENT_COUNT
        && dev->ckdorient != CKDORIENT_KEY)
    {
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) return rc;
    }

    /* If oriented to count field, skip the key field */
    if (dev->ckdorient == CKDORIENT_COUNT)
        dev->bufoff += dev->ckdcurkl;

    // "Thread "TIDPAT" %1d:%04X CKD file %s: read data %d bytes"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0437( dev );
    else
        LOGDEVTR( HHC00437, "I", dev->filename, dev->ckdcurdl );

    /* Read data field */
    if (dev->ckdcurdl > 0)
    {
        if (dev->bufoff + dev->ckdcurdl >= dev->bufoffhi)
        {
            /* Handle error condition */
            // "%1d:%04X CKD file %s: error attempting to read past end of track %d %d"
            WRMSG( HHC00419, "E", LCSS_DEVNUM,
                   dev->filename, dev->bufoff, dev->bufoffhi );

            /* Set unit check with equipment check */
            ckd_build_sense( dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return -1;
        }
        memcpy (buf, &dev->buf[dev->bufoff], dev->ckdcurdl);
        dev->bufoff += dev->ckdcurdl;
    }

    /* Set the device orientation fields */
    dev->ckdrem = 0;
    dev->ckdorient = CKDORIENT_DATA;

    return 0;
} /* end function ckd_read_data */


/*-------------------------------------------------------------------*/
/* Erase remainder of track                                          */
/*-------------------------------------------------------------------*/
static int ckd_erase ( DEVBLK *dev, BYTE *buf, int len, int *size,
                BYTE *unitstat)
{
int             rc;                     /* Return code               */
CKD_RECHDR      rechdr;                 /* CKD record header         */
int             keylen;                 /* Key length                */
int             datalen;                /* Data length               */
int             ckdlen;                 /* Count+key+data length     */

    /* If oriented to count or key field, skip key and data */
    if (dev->ckdorient == CKDORIENT_COUNT)
        dev->bufoff += dev->ckdcurkl + dev->ckdcurdl;
    else if (dev->ckdorient == CKDORIENT_KEY)
        dev->bufoff += dev->ckdcurdl;

    /* Copy the count field from the buffer */
    memset( &rechdr, 0,          CKD_RECHDR_SIZE);
    memcpy( &rechdr, buf, (len < CKD_RECHDR_SIZE) ?
                           len : CKD_RECHDR_SIZE);

    /* Extract the key length and data length */
    keylen = rechdr.klen;
    datalen = (rechdr.dlen[0] << 8) + rechdr.dlen[1];

    /* Calculate total count key and data size */
    ckdlen = CKD_RECHDR_SIZE + keylen + datalen;

    /* Check that there is enough space on the current track to
       contain the complete erase plus an end of track marker */
    if (dev->bufoff + ckdlen + CKD_ENDTRK_SIZE >= dev->bufoffhi)
    {
        /* Unit check with invalid track format */
        ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Logically erase rest of track by writing end of track marker */
    rc = (dev->hnd->write)( dev, dev->bufcur, dev->bufoff, (BYTE*) &CKD_ENDTRK, CKD_ENDTRK_SIZE, unitstat );
    if (rc < 0) return -1;

    /* Return total count key and data size */
    *size = ckdlen;

    /* Set the device orientation fields */
    dev->ckdrem = 0;
    dev->ckdorient = CKDORIENT_DATA;

    return 0;
} /* end function ckd_erase */


/*-------------------------------------------------------------------*/
/* Write count key and data fields                                   */
/*-------------------------------------------------------------------*/
static int ckd_write_ckd ( DEVBLK *dev, BYTE *buf, int len,
                BYTE *unitstat, BYTE trk_ovfl)
{
int             rc;                     /* Return code               */
CKD_RECHDR      rechdr;                 /* CKD record header         */
BYTE            recnum;                 /* Record number             */
BYTE            keylen;                 /* Key length                */
U16             datalen;                /* Data length               */
U32             ckdlen;                 /* Count+key+data length     */

    /* If oriented to count or key field, skip key and data */
    if (dev->ckdorient == CKDORIENT_COUNT)
        dev->bufoff += dev->ckdcurkl + dev->ckdcurdl;
    else if (dev->ckdorient == CKDORIENT_KEY)
        dev->bufoff += dev->ckdcurdl;

    /* Copy the count field from the buffer */
    memset( &rechdr, 0,          CKD_RECHDR_SIZE );
    memcpy( &rechdr, buf, (len < CKD_RECHDR_SIZE) ?
                           len : CKD_RECHDR_SIZE);

    /* Extract the record number, key length and data length */
    recnum  =            rechdr.rec;
    keylen  =            rechdr.klen;
    datalen =  fetch_hw( rechdr.dlen );

    /* Calculate total count key and data size */
    ckdlen = CKD_RECHDR_SIZE + keylen + datalen;

    if ((int)(dev->bufoff + ckdlen + CKD_ENDTRK_SIZE) >= dev->bufoffhi)
    {
        /* Unit check with invalid track format */
        ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Pad the I/O buffer with zeroes if necessary */
    while (len < (int)ckdlen) buf[len++] = '\0';

    // "Thread "TIDPAT" %1d:%04X CKD file %s: writing cyl %d head %d record %d kl %d dl %d"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0438( dev, recnum, keylen, datalen );
    else
        LOGDEVTR( HHC00438, "I",  dev->filename,
                  dev->ckdcurcyl, dev->ckdcurhead, recnum, keylen, datalen );

    /* Set track overflow flag if called for */
    if (trk_ovfl)
    {
        // "Thread "TIDPAT" %1d:%04X CKD file %s: setting track overflow flag for cyl %d head %d record %d"
        if (dev->ccwtrace && sysblk.traceFILE)
            tf_0439( dev, recnum );
        else
            LOGDEVTR( HHC00439, "I",  dev->filename,
                      dev->ckdcurcyl, dev->ckdcurhead, recnum );
        buf[0] |= 0x80;
    }

    /* Write count key and data */
    rc = (dev->hnd->write) (dev, dev->bufcur, dev->bufoff, buf, ckdlen, unitstat);
    if (rc < 0) return -1;
    dev->bufoff += ckdlen;

    /* Clear track overflow flag if we set it above */
    if (trk_ovfl)
    {
        buf[0] &= 0x7F;
    }

    /* Logically erase rest of track by writing end of track marker */
    rc = (dev->hnd->write)( dev, dev->bufcur, dev->bufoff, (BYTE*) &CKD_ENDTRK, CKD_ENDTRK_SIZE, unitstat );
    if (rc < 0) return -1;

    /* Set the device orientation fields */
    dev->ckdcurrec = recnum;
    dev->ckdcurkl = keylen;
    dev->ckdcurdl = datalen;
    dev->ckdrem = 0;
    dev->ckdorient = CKDORIENT_DATA;
    dev->ckdtrkof = dev->ckdcyls < 32768 ? trk_ovfl & 1 : 0;

    return 0;
} /* end function ckd_write_ckd */


/*-------------------------------------------------------------------*/
/* Write key and data fields                                         */
/*-------------------------------------------------------------------*/
static int ckd_write_kd ( DEVBLK *dev, BYTE *buf, int len,
                BYTE *unitstat)
{
int             rc;                     /* Return code               */
int             kdlen;                  /* Key+data length           */

    /* Unit check if not oriented to count area */
    if (dev->ckdorient != CKDORIENT_COUNT)
    {
        // "%1d:%04X CKD file %s: error write kd orientation"
        WRMSG( HHC00420, "E", LCSS_DEVNUM, dev->filename );
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Calculate total key and data size */
    kdlen = dev->ckdcurkl + dev->ckdcurdl;

    /* Pad the I/O buffer with zeroes if necessary */
    while (len < kdlen) buf[len++] = '\0';

    // "Thread "TIDPAT" %1d:%04X CKD file %s: updating cyl %d head %d record %d kl %d dl %d"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0440( dev );
    else
        LOGDEVTR( HHC00440, "I",  dev->filename,
                  dev->ckdcurcyl, dev->ckdcurhead, dev->ckdcurrec,
                  dev->ckdcurkl,  dev->ckdcurdl );

    /* Write key and data */
    rc = (dev->hnd->write) (dev, dev->bufcur, dev->bufoff, buf, kdlen, unitstat);
    if (rc < 0) return -1;
    dev->bufoff += kdlen;

    /* Set the device orientation fields */
    dev->ckdrem = 0;
    dev->ckdorient = CKDORIENT_DATA;

    return 0;
} /* end function ckd_write_kd */


/*-------------------------------------------------------------------*/
/* Write data field                                                  */
/*-------------------------------------------------------------------*/
static int ckd_write_data ( DEVBLK *dev, BYTE *buf, int len,
                BYTE *unitstat)
{
int             rc;                     /* Return code               */

    /* Unit check if not oriented to count or key areas */
    if (dev->ckdorient != CKDORIENT_COUNT
        && dev->ckdorient != CKDORIENT_KEY)
    {
        // "%1d:%04X CKD file %s: error write data orientation"
        WRMSG( HHC00421, "E", LCSS_DEVNUM, dev->filename );
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* If oriented to count field, skip the key field */
    if (dev->ckdorient == CKDORIENT_COUNT)
        dev->bufoff += dev->ckdcurkl;

    /* Pad the I/O buffer with zeroes if necessary */
    while (len < dev->ckdcurdl) buf[len++] = '\0';

    // "Thread "TIDPAT" %1d:%04X CKD file %s: updating cyl %d head %d record %d dl %d"
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_0441( dev );
    else
        LOGDEVTR( HHC00441, "I",  dev->filename,
                  dev->ckdcurcyl, dev->ckdcurhead, dev->ckdcurrec, dev->ckdcurdl );

    /* Write data */
    rc = (dev->hnd->write) (dev, dev->bufcur, dev->bufoff, buf, dev->ckdcurdl, unitstat);
    if (rc < 0) return -1;
    dev->bufoff += dev->ckdcurdl;

    /* Set the device orientation fields */
    dev->ckdrem = 0;
    dev->ckdorient = CKDORIENT_DATA;

    return 0;
} /* end function ckd_write_data */

/*-------------------------------------------------------------------*/
/* Forward references to static helper functions                     */
/*-------------------------------------------------------------------*/
static bool PerformSubsystemFunction
(
    DEVBLK*  dev,
    BYTE     code,
    BYTE     flags,
    BYTE     chained,
    U32      count,
    BYTE     prevcode,
    int      ccwseq,
    BYTE*    iobuf,
    BYTE*    more,
    BYTE*    unitstat,
    U32*     residual
);
static bool LocateRecordExtended
(
    DEVBLK*  dev,
    BYTE     code,
    BYTE     flags,
    BYTE     chained,
    U32      count,
    BYTE     prevcode,
    int      ccwseq,
    BYTE*    iobuf,
    BYTE*    more,
    BYTE*    unitstat,
    U32*     residual
);
static bool DefineExtent
(
    DEVBLK*  dev,
    BYTE     code,
    BYTE     flags,
    BYTE     chained,
    U32      count,
    BYTE     prevcode,
    int      ccwseq,
    BYTE*    iobuf,
    BYTE*    more,
    BYTE*    unitstat,
    U32*     residual
);

/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
void ckd_dasd_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual )
{
int             rc;                     /* Return code               */
int             j;                      /* Loop index                */
CKD_TRKHDR      trkhdr;                 /* CKD track header (HA)     */
CKD_RECHDR      rechdr;                 /* CKD record header (count) */
U32             size;                   /* Number of bytes available */
U32             num;                    /* Number of bytes to move   */
U32             offset;                 /* Offset into buf for I/O   */
BYTE            bin;                    /* Bin number                */
U16             cyl;                    /* Cylinder number           */
U16             head;                   /* Head number               */
BYTE            cchhr[5];               /* Search argument           */
BYTE            sector;                 /* Sector number             */
BYTE            key[256];               /* Key for search operations */
BYTE            trk_ovfl;               /* == 1 if track ovfl write  */

    /* If this is a data-chained READ, then return any data remaining
       in the buffer which was not used by the previous CCW */
    if (chained & CCW_FLAGS_CD)
    {
        memmove (iobuf, iobuf + dev->ckdpos, dev->ckdrem);
        num = (count < dev->ckdrem) ? count : dev->ckdrem;
        *residual = count - num;
        if (count < dev->ckdrem) *more = 1;
        dev->ckdrem -= (U16)num;
        dev->ckdpos  = (U16)num;
        *unitstat = CSW_CE | CSW_DE;
        return;
    }

    /* Command reject if data chaining and command is not READ */
    if ((flags & CCW_FLAGS_CD) && !IS_CCW_READ( code ))
    {
        // "%1d:%04X CKD file %s: data chaining not supported for non-read CCW %02X"
        WRMSG( HHC00422, "E", LCSS_DEVNUM, dev->filename, code );
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    /* Reset flags at start of CCW chain */
    if (chained == 0)
    {
        dev->ckdlocat = 0;
        dev->ckdspcnt = 0;
        dev->ckdseek = 0;
        dev->ckdskcyl = 0;
        dev->ckdrecal = 0;
        dev->ckdrdipl = 0;
        dev->ckdfmask = 0;
        dev->ckdxmark = 0;
        dev->ckdhaeq = 0;
        dev->ckdideq = 0;
        dev->ckdkyeq = 0;
        dev->ckdwckd = 0;
        dev->ckdlcount = 0;
        dev->ckdextcd = 0x00;
        dev->ckdfcoun = 0;
        dev->ckdlmask = 0;
        dev->ckdtrkof = 0;
        /* ISW20030819-1 : Clear Write HA flag */
        dev->ckdwrha = 0;
        dev->ckdssdlen = 0;

        /* Set initial define extent parameters */
        dev->ckdxtdef = 0;
        dev->ckdsetfm = 0;
        dev->ckdfmask = 0;
        dev->ckdxgattr = 0;
        dev->ckdxblksz = 0;
        dev->ckdxbcyl = 0;
        dev->ckdxbhead = 0;
        dev->ckdxecyl  = (U16)(dev->ckdcyls  - 1);
        dev->ckdxehead = (U16)(dev->ckdheads - 1);
    }

    /* Reset index marker flag if sense or control command,
       or any write command (other search ID or search key),
       or any read command except read sector --
       -- and except single track Read Count */
    if (0
        || IS_CCW_SENSE( code )
        || IS_CCW_CONTROL( code )
        || (1
            && IS_CCW_WRITE( code )
            && (code & 0x7F) != 0x31    // SEARCH ID EQUAL
            && (code & 0x7F) != 0x51    // SEARCH ID HIGH
            && (code & 0x7F) != 0x71    // SEARCH ID EQUAL OR HIGH
            && (code & 0x7F) != 0x29    // SEARCH KEY EQUAL
            && (code & 0x7F) != 0x49    // SEARCH KEY HIGH
            && (code & 0x7F) != 0x69    // SEARCH KEY EQUAL OR HIGH
           )
        || (1
            && IS_CCW_READ( code )
            &&  code         != 0x12    // READ COUNT
            && (code & 0x7F) != 0x22    // READ SECTOR
           )
    )
        dev->ckdxmark = 0;

    /* Note current operation for track overflow sense byte 3 */
    dev->ckdcuroper = (IS_CCW_READ( code )) ? 6 :
        ((IS_CCW_WRITE( code )) ? 5 : 0);

    /* If subsystem data has been prepared in the channel buffer by
       a previous Perform Subsystem Function command, generate a
       command reject if next command is not Read Subsystem Data */
    if (1
        && dev->ckdssdlen > 0
        && code != 0x3E // READ SUBSYSTEM DATA
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    /* If within Locate Record Extended domain and not RT command
       reject with status that includes Unit Check (Command Reject,
       format X'02', Invalid Command Sequence) */
    if (1
        && dev->ckdlmask
        && code != 0xDE // READ TRACK
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0,FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    /* Process depending on CCW opcode */
    switch (code) {

    case 0x02:
    /*---------------------------------------------------------------*/
    /* READ IPL                                                      */
    /*---------------------------------------------------------------*/
        /* Command reject if preceded by a Define Extent or
           Set File Mask, or within the domain of a Locate Record */
        if (dev->ckdxtdef || dev->ckdsetfm || dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* No more define extend allowed */
        dev->ckdxtdef = 1;
        dev->ckdsetfm = 1;

        /* Set locate record parameters */
        dev->ckdloper = CKDOPER_ORIENT_DATA | CKDOPER_RDDATA;
        dev->ckdlaux = 0;
        dev->ckdlcount = 2;
        dev->ckdltranlf = 0;

        /* Seek to start of cylinder zero track zero */
        rc = ckd_seek (dev, 0, 0, &trkhdr, unitstat);
        if (rc < 0) break;

        /* Read count field for first record following R0 */
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) break;

        /* Calculate number of bytes to read and set residual count */
        size = dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Read data field */
        rc = ckd_read_data (dev, code, iobuf, unitstat);
        if (rc < 0) break;

        /* Set command processed flag */
        dev->ckdrdipl = 1;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return unit exception if data length is zero */
        if (dev->ckdcurdl == 0)
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
        else
            *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x03:
    /*---------------------------------------------------------------*/
    /* CONTROL NO-OPERATION                                          */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record,   */
        /* except if Read IPL                          2012-08-14    */
        if (1
            && dev->ckdlcount > 0
            && dev->prevcode != 0x02 // READ IPL
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x17:
    /*---------------------------------------------------------------*/
    /* RESTORE                                                       */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xA6:
    case 0xB6:
//FIXME: 0xA6/0xB6 ccw is undoc'd.  We are treating it as 0x86 except
//       we will allow a DX ccw to follow.
    case 0x06:
    case 0x86:
    /*---------------------------------------------------------------*/
    /* READ DATA                                                     */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (1
            &&  dev->ckd3990
            && !dev->ckdseek
            && !dev->ckdskcyl
            && !dev->ckdlocat
            && !dev->ckdrdipl
            && !dev->ckdrecal
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!(0
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDANY
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }

            /* If Locate Record/Extended specified multi-track read,
               then command reject if this isn't a multi-track read
            */
            if (1
                && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                && !IS_CCW_MTRACK( code )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }

        }

        /* If not oriented to count or key field, read next count */
        if (dev->ckdorient != CKDORIENT_COUNT
            && dev->ckdorient != CKDORIENT_KEY)
        {
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;
        }

        /* Calculate number of bytes to read and set residual count */
        size = dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;
        offset = 0;

        /* Read data field */
        rc = ckd_read_data (dev, code, iobuf, unitstat);
        if (rc < 0) break;

        /* If track overflow, keep reading */
        while (dev->ckdtrkof)
        {
            /* Advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;

            /* Read the first count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Skip the key field if present */
            if (dev->ckdcurkl > 0)
                dev->bufoff += dev->ckdcurkl;

            /* Set offset into buffer for this read */
            offset += num;

            /* Account for size of this overflow record */
            size = dev->ckdcurdl;
            num = (*residual < size) ? *residual : size;
            if (*residual < size) *more = 1;
            else *more = 0;
            *residual -= num;

            /* Read the next data field */
            rc = ckd_read_data (dev, code, iobuf+offset, unitstat);
            if (rc < 0) break;
        }

        /* Bail out if track overflow produced I/O error */
        if (rc < 0) break;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return unit exception if data length is zero */
        if (dev->ckdcurdl == 0)
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
        else
            *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x0E:
    case 0x8E:
    /*---------------------------------------------------------------*/
    /* READ KEY AND DATA                                             */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            /*
             * 3990 reference says LRE CKDOPER_RDANY "must be followed
             * by a sequence of multi-track Read Count, Read Count Key
             * and Data, or Read Data commands".  That is, it doesn't
             * mention Read Key and Data.
             */
            if (!(0
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
               /* || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDANY */
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* If not oriented to count field, read next count */
        if (dev->ckdorient != CKDORIENT_COUNT)
        {
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;
        }

        /* Calculate number of bytes to read and set residual count */
        size = dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;
        offset = dev->ckdcurkl;

        /* Read key field */
        rc = ckd_read_key (dev, code, iobuf, unitstat);
        if (rc < 0) break;

        /* Read data field */
        rc = ckd_read_data (dev, code, iobuf+dev->ckdcurkl, unitstat);
        if (rc < 0) break;

        /* If track overflow, keep reading */
        while (dev->ckdtrkof)
        {
            /* Advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;

            /* Read the first count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Skip the key field if present */
            if (dev->ckdcurkl > 0)
                dev->bufoff += dev->ckdcurkl;

            /* Set offset into buffer for this read */
            offset += num;

            /* Account for size of this overflow record */
            size = dev->ckdcurdl;
            num = (*residual < size) ? *residual : size;
            if (*residual < size) *more = 1;
            else *more = 0;
            *residual -= num;

            /* Read the next data field */
            rc = ckd_read_data (dev, code, iobuf+offset, unitstat);
            if (rc < 0) break;
        }

        /* Bail out if track overflow produced I/O error */
        if (rc < 0) break;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return unit exception if data length is zero */
        if (dev->ckdcurdl == 0)
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
        else
            *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x12:
    case 0x92:
    /*---------------------------------------------------------------*/
    /* READ COUNT                                                    */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!(0
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDANY
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                  || (1
                      && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
                      && (dev->ckdlaux & CKDLAUX_RDCNTSUF)
                      &&  dev->ckdlcount == 1
                     )
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* Read next count field */
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) break;

        /* Calculate number of bytes to read and set residual count */
        size = CKD_RECHDR_SIZE;
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Copy count field to I/O buffer */
        memcpy (iobuf, &rechdr, CKD_RECHDR_SIZE);

        /* Save first Read Count if we haven't already */
        if (!dev->ckdfcoun && dev->ckdlaux & CKDLAUX_RDCNTSUF)
        {
            memcpy( dev->ckdfcwrk, &rechdr, CKD_RECHDR_SIZE );
            dev->ckdfcoun = 1;
        }

        /* Turn off track overflow flag in read record header */
        if(dev->ckdcyls < 32768)
            *iobuf &= 0x7F;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x16:
    case 0x96:
    /*---------------------------------------------------------------*/
    /* READ RECORD ZERO                                              */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!(0
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
                  || (1
                      && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                      && (0
                          || (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_HOME
                          || (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_INDEX
                         )
                     )
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* For multitrack operation outside domain of a Locate Record,
           attempt to advance to the next track before reading R0 */
        if (IS_CCW_MTRACK( code ) && dev->ckdlcount == 0)
        {
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;
        }

        /* Seek to beginning of track */
        rc = ckd_seek (dev, dev->ckdcurcyl, dev->ckdcurhead, &trkhdr, unitstat);
        if (rc < 0) break;

        /* Read the count field for record zero */
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) break;

        /* Calculate number of bytes to read and set residual count */
        size = CKD_RECHDR_SIZE + dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Copy count field to I/O buffer */
        memcpy (iobuf, &rechdr, CKD_RECHDR_SIZE);

        /* Turn off track overflow flag in read record header */
        if(dev->ckdcyls < 32768)
            *iobuf &= 0x7F;

        /* Read key field */
        rc = ckd_read_key (dev, code,
                            iobuf + CKD_RECHDR_SIZE, unitstat);
        if (rc < 0) break;

        /* Read data field */
        rc = ckd_read_data (dev, code,
                            iobuf + CKD_RECHDR_SIZE + dev->ckdcurkl,
                            unitstat);
        if (rc < 0) break;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return unit exception if data length is zero */
        if (dev->ckdcurdl == 0)
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
        else
            *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x0A:
    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC READ HOME ADDRESS                                  */
    /*---------------------------------------------------------------*/
    /*                                                               */
    /*  The Diagnostic RHA is for handling the Skip Displacement     */
    /*  and similar functions for DASD surface management. This      */
    /*  isn't relevant for Hercules DASD emulation of course, so     */
    /*  it is only being implemented to allow ICKDSF to run without  */
    /*  receiving and reporting any CMDREJ errors. The matching      */
    /*  DIAGNOSTIC WRITE HOME ADDRESS isn't used at all by ICKDSF    */
    /*  INIT on error-free volumes and isn't implemented at all.     */
    /*                                                               */
    /*---------------------------------------------------------------*/
    {
        int HAoff;                  /* Offset to where Flag+CC+HH
                                       appears in the returned data  */

        /* Command is not available on older machinery */
        if (!(0
              || (dev->ckd3990)
              || (dev->ckdcu->devt == 0x3880)
              || (dev->ckdcu->devt == 0x2105)
             )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* For 3880, only 3375 and 3380 use this command, but we also
           accept 3390 too, since this is a bypass for allowing ICKDSF
           to run under TurnKey MVS.
        */
        if ((dev->ckdcu->devt == 0x3880) && !(0
            || (dev->devtype == 0x3375)
            || (dev->devtype == 0x3380)
            || (dev->devtype == 0x3390)   // (for Turnkey MVS)
        ))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command. The same
           requirement is in place for 3880 with FC 3005. The 2105 and
           later machines however, are not (yet?) considered.
        */
        if ((dev->ckd3990 || dev->ckdcu->devt == 0x3880)
            && dev->ckdseek  == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!((dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA ||
                 ((dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16 &&
                  (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_INDEX))
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }
        else // (outside LR)
        {
            /* Check that the file mask allows the operation... */

            /* For 3880, diagnostic commands must be allowed.
               Bit 5 permits indicates Diagnostic commands.
               Bit 6 will additionally inhibit some internal
                     recovery for data checks.
            */
            if (dev->ckdcu->devt == 0x3880)
            {
                if ((dev->ckdfmask & CKDMASK_AAUTH_DIAG) != CKDMASK_AAUTH_DIAG)
                {
                    ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_5 );
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    break;
                }
            }

            /* For 3990, diagnostic commands must be allowed.
               Bits 5+6 should be '10'. */
            if (dev->ckd3990)
            {
                if (!(1
                      && ((dev->ckdfmask & CKDMASK_AAUTH_DIAG) == CKDMASK_AAUTH_DIAG)
                      && ((dev->ckdfmask & CKDMASK_AAUTH_DSF) == 0)
                     )
                )
                {
                    ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_5 );
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    break;
                }
            }
        }

        /* Seek to beginning of track */
        rc = ckd_seek( dev, dev->ckdcurcyl, dev->ckdcurhead, &trkhdr, unitstat );
        if (rc < 0) break;

        /* Calculate number of bytes and set residual */
        if (dev->devtype == 0x3375)
        {
            size = 27;      // (3375 complete HA length)
            HAoff = 18;     // (offset to Flag field)
        }
        else
        {
            size = 28;      // (3380 and 3390 complete HA length)
            HAoff = 19;     // (offset to Flag field)
        }

        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Copy the classical HA fields to the I/O buffer.
           Only Flag+CC+HH will contain data. The rest is zeros.
           Definitely not correct data, but if written back,
           it would at least be ignored!
        */
        memset( iobuf, 0, size );
        memcpy( &iobuf[ HAoff ], &trkhdr, CKD_TRKHDR_SIZE );

        /* Save size and offset of data not used by this CCW */
        dev->ckdpos = (U16)(num);
        dev->ckdrem = (U16)(size - num);

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;
    }

    case 0x1A:
    case 0x9A:
    /*---------------------------------------------------------------*/
    /* READ HOME ADDRESS                                             */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!(0
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
                  || (1
                      && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                      && (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_INDEX
                     )
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* For multitrack operation outside domain of a Locate Record,
           attempt to advance to the next track before reading HA */
        if (IS_CCW_MTRACK( code ) && dev->ckdlcount == 0)
        {
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;
        }

        /* Seek to beginning of track */
        rc = ckd_seek (dev, dev->ckdcurcyl, dev->ckdcurhead, &trkhdr, unitstat);
        if (rc < 0) break;

        /* Calculate number of bytes to read and set residual count */
        size = CKD_TRKHDR_SIZE;
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Copy home address field to I/O buffer */
        memcpy (iobuf, &trkhdr, CKD_TRKHDR_SIZE);

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x19:
    /*---------------------------------------------------------------*/
    /* WRITE HOME ADDRESS                                            */
    /*---------------------------------------------------------------*/
        /*
           The traditional HA of IBM DASDs contains a flag byte and
           2+2 bytes of CC and HH. The flag byte is used to indicate
           the track location of primary or alternate plus the track
           condition of good or bad. Since Hercules DASD emulation
           does not need this concept, the flag byte is always zero,
           so the IBM HA and Hercules 'track header' have identical
           contents.
        */
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek  == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            /* The ECKD chaining requirement is Format Write operation,
               orient to Home Address. Command Reject if it's not.
            */
            if (!((dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT &&
                  (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_HOME))
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* File protected if file mask does not allow Write HA */
        if ((dev->ckdfmask & CKDMASK_WRCTL) != CKDMASK_WRCTL_ALLWRT)
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Seek to beginning of track */
        rc = ckd_seek (dev, dev->ckdcurcyl, dev->ckdcurhead, &trkhdr, unitstat);
        if (rc < 0) break;

        /* ISW20030819-1 Indicate WRHA performed */
        dev->ckdwrha = 1;

        /* Calculate number of bytes to write and set residual count */
        switch (dev->devtype)
        {
            case 0x3340: size =  7; break;
            case 0x3350: size = 11; break;
            default:     size =  5; break;  /* Standard HA length */
        }
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x1E:
    case 0x9E:
    /*---------------------------------------------------------------*/
    /* READ COUNT KEY AND DATA                                       */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!(0
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDANY
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* Read next count field */
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) break;

        /* Calculate number of bytes to read and set residual count */
        size = CKD_RECHDR_SIZE + dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;
        offset = CKD_RECHDR_SIZE + dev->ckdcurkl;

        /* Copy count field to I/O buffer */
        memcpy (iobuf, &rechdr, CKD_RECHDR_SIZE);

        /* Turn off track overflow flag in read record header */
        if(dev->ckdcyls < 32768)
            *iobuf &= 0x7F;

        /* Read key field */
        rc = ckd_read_key (dev, code,
                            iobuf + CKD_RECHDR_SIZE, unitstat);
        if (rc < 0) break;

        /* Read data field */
        rc = ckd_read_data (dev, code,
                            iobuf + CKD_RECHDR_SIZE + dev->ckdcurkl,
                            unitstat);
        if (rc < 0) break;

        /* If track overflow, keep reading */
        while (dev->ckdtrkof)
        {
            /* Advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;

            /* Read the first count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Skip the key field if present */
            if (dev->ckdcurkl > 0)
                dev->bufoff += dev->ckdcurkl;

            /* Set offset into buffer for this read */
            offset += num;

            /* Account for size of this overflow record */
            size = dev->ckdcurdl;
            num = (*residual < size) ? *residual : size;
            if (*residual < size) *more = 1;
            else *more = 0;
            *residual -= num;

            /* Read the next data field */
            rc = ckd_read_data (dev, code, iobuf+offset, unitstat);
            if (rc < 0) break;
        }

        /* Bail out if track overflow produced I/O error */
        if (rc < 0) break;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return unit exception if data length is zero */
        if (dev->ckdcurdl == 0)
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
        else
            *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x5E:
    /*---------------------------------------------------------------*/
    /* READ MULTIPLE COUNT KEY AND DATA                              */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Read records into the I/O buffer until end of track */
        for (size = 0; ; )
        {
            /* Read next count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Exit if end of track marker was read */
            if (memcmp( &rechdr, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
                break;

            /* Copy count field to I/O buffer */
            memcpy (iobuf + size, &rechdr, CKD_RECHDR_SIZE);
            size += CKD_RECHDR_SIZE;

            /* Turn off track overflow flag */
            if(dev->ckdcyls < 32768)
                *(iobuf + size) &= 0x7F;

            /* Read key field */
            rc = ckd_read_key (dev, code, iobuf + size, unitstat);
            if (rc < 0) break;
            size += dev->ckdcurkl;

            /* Read data field */
            rc = ckd_read_data (dev, code, iobuf + size, unitstat);
            if (rc < 0) break;
            size += dev->ckdcurdl;

        } /* end for(size) */

        /* Set the residual count */
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0xDE:
    /*---------------------------------------------------------------*/
    /* READ TRACK                                                    */
    /*---------------------------------------------------------------*/
        /* Command reject if not within the domain of a Locate Record
           that specifies a read tracks operation */
        if (dev->ckdlcount == 0
         || (((dev->ckdloper & CKDOPER_CODE) != CKDOPER_RDTRKS)
          && ((dev->ckdloper & CKDOPER_CODE) != CKDOPER_RDTSET)))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not chained from a Locate Record
           command or from another Read Track command */
        if (0
            || chained == 0
            || (1
                && prevcode != 0x47 // LOCATE RECORD
                && prevcode != 0x4B // LOCATE RECORD EXTENDED
                && prevcode != 0xDE // READ TRACK
                && prevcode != 0xE7 // PREFIX
               )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Advance to next track if chained from previous read track */
        if (prevcode == 0xDE) // READ TRACK
        {
            j = 1;
            /* Skip tracks while hi bit off in ckdlmask */
            if (dev->ckdlmask)
            {
                while (!(dev->ckdlmask & 0x8000))
                {
                    j++;
                    dev->ckdlmask <<= 1;
                }
            }
            rc = mt_advance (dev, unitstat, j);
            if (rc < 0) break;
        }

        /* Shift read track set mask left a bit */
        dev->ckdlmask <<= 1;

        /* Read each record on the track into the I/O buffer */
        for (size = 0; ; )
        {
            /* Read next count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Copy count field to I/O buffer */
            memcpy (iobuf + size, &rechdr, CKD_RECHDR_SIZE);
            size += CKD_RECHDR_SIZE;

            /* Turn off track overflow flag */
            if(dev->ckdcyls < 32768)
                *(iobuf+size) &= 0x7F;

            /* Exit if end of track marker was read */
            if (memcmp( &rechdr, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
                break;

            /* Read key field */
            rc = ckd_read_key (dev, code, iobuf + size, unitstat);
            if (rc < 0) break;
            size += dev->ckdcurkl;

            /* Read data field */
            rc = ckd_read_data (dev, code, iobuf + size, unitstat);
            if (rc < 0) break;
            size += dev->ckdcurdl;

        } /* end for(size) */

        /* Set the residual count */
        num = (count < size) ? count : size;
        *residual = count - num;
        if (count < size) *more = 1;

        /* Save size and offset of data not used by this CCW */
        dev->ckdrem = (U16)(size - num);
        dev->ckdpos = (U16)(       num);

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0xE7: // Prefix
    /*---------------------------------------------------------------*/
    /* PREFIX        (SA22-1025 IBM Subsystem Reference Guide)       */
    /*---------------------------------------------------------------*/

        *residual = 0;  // ZZ FIXME (Fish: always?! I question this!)

        // Command reject if count is less than minimum
        if (count < 64)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        // Extract Format, Validity and Auxiliary bytes
        dev->ckdformat    = iobuf[0];
        dev->ckdvalid     = iobuf[1];
        dev->ckdauxiliary = iobuf[3];

        // Validate Format byte
        if (1
            && dev->ckdformat != PFX_F_DX
            && dev->ckdformat != PFX_F_DX_LRE
            && dev->ckdformat != PFX_F_DX_PSF
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        switch (dev->ckdformat)
        {
        case PFX_F_DX:

            /* Process only the Define Extent fields */

            if (DefineExtent( dev, code, flags, chained, count, prevcode,
                              ccwseq, iobuf, more, unitstat, residual ))
            {
                prevcode = 0x63; // DEFINE EXTENT
                *unitstat = CSW_CE | CSW_DE;
            }
            break;

        case PFX_F_DX_LRE:

            /* Process both Define Extent and Locate Record Extended fields */

            if (DefineExtent( dev, code, flags, chained, count, prevcode,
                              ccwseq, iobuf, more, unitstat, residual ))
            {
                prevcode = 0x63; // DEFINE EXTENT
                if (LocateRecordExtended( dev, code, flags, chained, count, prevcode,
                                          ccwseq, iobuf, more, unitstat, residual ))
                {
                    prevcode = 0x4B; // LOCATE RECORD EXTENDED
                    *unitstat = CSW_CE | CSW_DE;
                }
            }
            break;

        case PFX_F_DX_PSF:

            /* Process both Define Extent and Perform Subsystem Function fields */

            if (DefineExtent( dev, code, flags, chained, count, prevcode,
                              ccwseq, iobuf, more, unitstat, residual ))
            {
                if (PerformSubsystemFunction( dev, code, flags, chained, count, prevcode,
                                              ccwseq, iobuf, more, unitstat, residual ))
                {
                    *unitstat = CSW_CE | CSW_DE;
                }
            }
            break;

        default:
            CRASH();  // LOGIC ERROR!
            break;
        }
        break;

    case 0x27: // PERFORM SUBSYSTEM FUNCTION (maybe! see below!)
    case 0xF7: // DSOP: DEFINE SUBSYSTEM OPERATION (IBM RAMAC Array, GC26-7006-01)
    /*---------------------------------------------------------------*/
    /* PERFORM SUBSYSTEM FUNCTION                                    */
    /*---------------------------------------------------------------*/
        /* If the control unit is not a 3990 then CCW code 0x27 is
           treated as a SEEK AND SET SECTOR (Itel 7330 controller) */
        if (dev->ckd3990 == 0)
            goto seek_0x27;

        if (PerformSubsystemFunction( dev, code, flags, chained, count, prevcode,
                                      ccwseq, iobuf, more, unitstat, residual ))
        {
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    seek_0x27: /* SEEK AND SET SECTOR (Itel 7330 controller only) */
    case 0x07: /* SEEK */
    case 0x0B: /* SEEK CYLINDER */
    case 0x1B: /* SEEK HEAD */
    /*---------------------------------------------------------------*/
    /* SEEK                                                          */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* For 3990, command reject if Seek Head not preceded by Seek,
           Seek Cylinder, Locate Record, Read IPL, or Recalibrate */
        if (1
            && dev->ckd3990
            && code == 0x1B // SEEK HEAD
            && dev->ckdseek  == 0
            && dev->ckdskcyl == 0
            && dev->ckdlocat == 0
            && dev->ckdrdipl == 0
            && dev->ckdrecal == 0
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* File protected if file mask does not allow requested seek */
        if (0
            || (1
                && (0
                    || code == 0x07 // SEEK
                    || code == 0x27 // PERFORM SUBSYSTEM FUNCTION
                    )
                && (dev->ckdfmask & CKDMASK_SKCTL) != CKDMASK_SKCTL_ALLSKR
               )
            || (1
                && code == 0x0B // SEEK CYLINDER
                && (dev->ckdfmask & CKDMASK_SKCTL) != CKDMASK_SKCTL_ALLSKR
                && (dev->ckdfmask & CKDMASK_SKCTL) != CKDMASK_SKCTL_CYLHD
               )
            || (1
                && code == 0x1B // SEEK HEAD
                && (dev->ckdfmask & CKDMASK_SKCTL) == CKDMASK_SKCTL_INHSMT
               )
        )
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set residual count */
        num = (count < 6) ? count : 6;
        *residual = count - num;

        /* Command reject if count is less than 6 */
        if (count < 6)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Extract the BBCCHH seek address from the I/O buffer */
        bin = (iobuf[0] << 8) | iobuf[1];
        cyl = (iobuf[2] << 8) | iobuf[3];
        head = (iobuf[4] << 8) | iobuf[5];

        /* For Seek Head, use the current cylinder number */
        if (code == 0x1B) // SEEK HEAD
            cyl = (U16) dev->ckdcurcyl;

        /* Command reject if seek address is invalid */
        if (bin != 0 || cyl >= dev->ckdcyls || head >= dev->ckdheads)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* File protected if outside defined extent */
        if ( EXTENT_CHECK(dev, cyl, head) )
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Seek to specified cylinder and head */
        rc = ckd_seek (dev, cyl, head, &trkhdr, unitstat);
        if (rc < 0) break;

        /* Set command processed flag */
        dev->ckdseek = 1;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x13:
    /*---------------------------------------------------------------*/
    /* RECALIBRATE                                                   */
    /*---------------------------------------------------------------*/
        /* Command reject if recalibrate is issued to a 3390 */
        if (dev->devtype == 0x3390)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* File protected if the file mask does not allow recalibrate,
           or if the file mask specifies diagnostic authorization */
        if ((dev->ckdfmask & CKDMASK_SKCTL) != CKDMASK_SKCTL_ALLSKR
            || (dev->ckdfmask & CKDMASK_AAUTH) == CKDMASK_AAUTH_DIAG)
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* File protected if cyl 0 head 0 is outside defined extent */
        if ( EXTENT_CHECK0(dev) )
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Seek to cylinder 0 head 0 */
        rc = ckd_seek (dev, 0, 0, &trkhdr, unitstat);
        if (rc < 0) break;

        /* Set command processed flag */
        dev->ckdrecal = 1;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x1F:
    /*---------------------------------------------------------------*/
    /* SET FILE MASK                                                 */
    /*---------------------------------------------------------------*/
        /* Command reject if preceded by a Define Extent or
           Set File Mask, or within the domain of a Locate Record */
        if (dev->ckdxtdef || dev->ckdsetfm || dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set residual count */
        num = (count < 1) ? count : 1;
        *residual = count - num;

        /* Command reject if count is less than 1 */
        if (count < 1)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Extract the file mask from the I/O buffer */
        dev->ckdfmask = iobuf[0];

        // "Thread "TIDPAT" %1d:%04X CKD file %s: set file mask %02X"
        if (dev->ccwtrace && sysblk.traceFILE)
            tf_0442( dev );
        else
            LOGDEVTR( HHC00442, "I", dev->filename, dev->ckdfmask );

        /* Command reject if file mask is invalid */
        if ((dev->ckdfmask & CKDMASK_RESV) != 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set command processed flag */
        dev->ckdsetfm = 1;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x22:
    /*---------------------------------------------------------------*/
    /* READ SECTOR                                                   */
    /*---------------------------------------------------------------*/
        /* Command reject if non-RPS device */
        if (dev->ckdtab->sectors == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set residual count */
        num = (count < 1) ? count : 1;
        *residual = count - num;
        if (count < 1) *more = 1;

        /* Return sector number in I/O buffer */
        iobuf[0] = 0;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x23:
    /*---------------------------------------------------------------*/
    /* SET SECTOR                                                    */
    /*---------------------------------------------------------------*/
        /* Command reject if non-RPS device */
        if (dev->ckdtab->sectors == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set residual count */
        num = (count < 1) ? count : 1;
        *residual = count - num;

        /* Command reject if count is less than 1 */
        if (count < 1)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x29: case 0xA9: /* SEARCH KEY EQUAL */
    case 0x49: case 0xC9: /* SEARCH KEY HIGH */
    case 0x69: case 0xE9: /* SEARCH KEY EQUAL OR HIGH */
    /*---------------------------------------------------------------*/
    /* SEARCH KEY                                                    */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Read next key */
        rc = ckd_read_key (dev, code, key, unitstat);
        if (rc < 0) break;

        /* Calculate number of compare bytes and set residual count */
        num = (count < (U32)dev->ckdcurkl) ?
               count : (U32)dev->ckdcurkl;
        *residual = count - num;

        /* Nothing to compare if key length is zero */
        if (dev->ckdcurkl == 0)
        {
            *unitstat = CSW_CE | CSW_DE;
            break;
        }

        /* Compare key with search argument */
        rc = memcmp(key, iobuf, num);

        /* Return status modifier if compare result matches */
        if (((code & 0x20) && (rc == 0))
            || ((code & 0x40) && (rc > 0)))
            *unitstat = CSW_SM | CSW_CE | CSW_DE;
        else
            *unitstat = CSW_CE | CSW_DE;

        /* If the search was successful, trace the first 8 bytes of
           the key, which will usually be a dataset name or member
           name and can provide useful debugging information */
        if ((*unitstat & CSW_SM) && dev->ckdkeytrace
            && isprint((unsigned char)guest_to_host(iobuf[0])))
        {
            BYTE module[45];
            size_t kl = MIN( (size_t)num, sizeof( module ) - 1 );

            // "Thread "TIDPAT" %1d:%04X CKD file %s: search key %s"
            if (sysblk.traceFILE)
                tf_0423( dev, kl, iobuf );
            else
            {
                str_guest_to_host( iobuf, module, (u_int)kl );
                LOGDEVTR( HHC00423, "I", dev->filename, RTRIM( module ));
            }
        }

        /* Set flag if entire key was equal for SEARCH KEY EQUAL */
        if (1
            && rc == 0
            && num == (U32)dev->ckdcurkl
            && (code & 0x7F) == 0x29 // SEARCH KEY EQUAL
        )
            dev->ckdkyeq = 1;
        else
            dev->ckdkyeq = 0;

        break;

    case 0x31: case 0xB1: /* SEARCH ID EQUAL */
    case 0x51: case 0xD1: /* SEARCH ID HIGH */
    case 0x71: case 0xF1: /* SEARCH ID EQUAL OR HIGH */
    /*---------------------------------------------------------------*/
    /* SEARCH ID                                                     */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Read next count field */
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) break;

        /* Calculate number of compare bytes and set residual count */
        num = (count < 5) ? count : 5;
        *residual = count - num;

        /* Turn off track overflow flag in record header if present */
        if(dev->ckdcyls < 32768)
            rechdr.cyl[0] &= 0x7F;

        /* Compare count with search argument */
        rc = memcmp(&rechdr, iobuf, num);

        /* Return status modifier if compare result matches */
        if (((code & 0x20) && (rc == 0))
            || ((code & 0x40) && (rc > 0)))
            *unitstat = CSW_SM | CSW_CE | CSW_DE;
        else
            *unitstat = CSW_CE | CSW_DE;

        /* Set flag if entire id compared equal for SEARCH ID EQUAL */
        if (1
            && rc == 0
            && num == 5
            && (code & 0x7F) == 0x31 // SEARCH ID EQUAL
        )
            dev->ckdideq = 1;
        else
            dev->ckdideq = 0;

        break;

    case 0x39:
    case 0xB9:
    /*---------------------------------------------------------------*/
    /* SEARCH HOME ADDRESS EQUAL                                     */
    /*---------------------------------------------------------------*/
        /* For 3990, command reject if not preceded by Seek, Seek Cyl,
           Locate Record, Read IPL, or Recalibrate command */
        if (dev->ckd3990
            && dev->ckdseek == 0 && dev->ckdskcyl == 0
            && dev->ckdlocat == 0 && dev->ckdrdipl == 0
            && dev->ckdrecal == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* For multitrack operation, advance to next track */
        if (IS_CCW_MTRACK( code ))
        {
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;
        }

        /* Seek to beginning of track */
        rc = ckd_seek (dev, dev->ckdcurcyl, dev->ckdcurhead, &trkhdr, unitstat);
        if (rc < 0) break;

        /* Calculate number of compare bytes and set residual count */
        num = (count < 4) ? count : 4;
        *residual = count - num;

        /* Compare CCHH portion of track header with search argument */
        rc = memcmp(&(trkhdr.cyl), iobuf, num);

        /* Return status modifier if compare result matches */
        if (rc == 0)
            *unitstat = CSW_SM | CSW_CE | CSW_DE;
        else
            *unitstat = CSW_CE | CSW_DE;

        /* Set flag if entire home address compared equal */
        if (rc == 0 && num == 4)
            dev->ckdhaeq = 1;
        else
            dev->ckdhaeq = 0;

        break;

    case 0x05:
    /*---------------------------------------------------------------*/
    /* WRITE DATA                                                    */
    /*---------------------------------------------------------------*/
        /* Command reject if the current track is in the DSF area */
        if (dev->ckdcurcyl >= dev->ckdcyls)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not within the domain of a Locate Record
           and not preceded by either a Search ID Equal or Search Key
           Equal that compared equal on all bytes */
           /*INCOMPLETE*/ /*Write CKD allows intervening Read/Write
             key and data commands, Write Data does not!!! Rethink
             the handling of these flags*/
        if (dev->ckdlcount == 0 && dev->ckdideq == 0
            && dev->ckdkyeq == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if file mask inhibits all write commands */
        if ((dev->ckdfmask & CKDMASK_WRCTL) == CKDMASK_WRCTL_INHWRT)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!(0
                  ||     (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK
                  || (1
                      && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
                      && dev->ckdlcount == ((dev->ckdlaux & CKDLAUX_RDCNTSUF) ? 2 : 1)
                     )
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }

            /* If not operating in CKD conversion mode, check that the
               data length is equal to the transfer length factor,
               except when writing a R0 data area under the control
               of a Locate Record Write Track operation, in which
               case a transfer length factor of 8 is used instead */
            if ((dev->ckdxgattr & CKDGATR_CKDCONV) == 0)
            {
                if (1
                    && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK
                    && dev->ckdcurrec == 0
                )
                    num = 8;
                else
                    num = dev->ckdltranlf;

                if (dev->ckdcurdl != num)
                {
                    /* Unit check with invalid track format */
                    ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    break;
                }
            }
        } /* end if(ckdlcount) */

        /* If data length is zero, terminate with unit exception */
        if (dev->ckdcurdl == 0)
        {
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
            break;
        }

        /* Calculate number of bytes written and set residual count */
        size = dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Write data */
        rc = ckd_write_data (dev, iobuf, num, unitstat);
        if (rc < 0) break;

        /* If lrcount=1 & r0 then erase rest of the track */
        if (1
            && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK
            && dev->ckdcurrec == 0
            && dev->ckdlcount == 1
        )
        {
            /* Write end of track marker */
            rc = ckd_erase (dev, iobuf, count, (int*)&size, unitstat);
            if (rc < 0) break;

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
            break;
        }

        /* If track overflow, keep writing */
        offset = 0;
        while (dev->ckdtrkof)
        {
            /* Advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;

            /* Read the first count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Set offset into buffer for this write */
            offset += size;

            /* Account for size of this overflow record */
            size = dev->ckdcurdl;
            num = (*residual < size) ? *residual : size;
            *residual -= num;

            /* Write the next data field */
            rc = ckd_write_data (dev, iobuf+offset, num, unitstat);
            if (rc < 0) break;
        }

        /* Bail out if track overflow produced I/O error */
        if (rc < 0) break;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0xA5:
    case 0xB5:
//FIXME: 0xA5/0xB5 ccw is undoc'd.  We are treating it as 0x85 except
//       we will allow a DX ccw to follow.
    case 0x85:
    /*---------------------------------------------------------------*/
    /* WRITE UPDATE DATA                                             */
    /*---------------------------------------------------------------*/
        /* Command reject if not within the domain of a Locate Record
           that specifies the Write Data operation code */
        if (dev->ckdlcount == 0
         || ((dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRITE
          && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRTANY))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Orient to next user record count field */
        if (dev->ckdorient != CKDORIENT_COUNT
            || dev->ckdcurrec == 0)
        {
            /* Read next count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;
        }

        /* If not operating in CKD conversion mode, check that the
           data length is equal to the transfer length factor */
        if ((dev->ckdxgattr & CKDGATR_CKDCONV) == 0)
        {
            if (dev->ckdcurdl != dev->ckdltranlf)
            {
                /* Unit check with invalid track format */
                ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* If data length is zero, terminate with unit exception */
        if (dev->ckdcurdl == 0)
        {
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
            break;
        }

        /* Calculate number of bytes written and set residual count */
        size = dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Write data */
        rc = ckd_write_data (dev, iobuf, num, unitstat);
        if (rc < 0) break;

        /* If track overflow, keep writing */
        offset = 0;
        while (dev->ckdtrkof)
        {
            /* Advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;

            /* Read the first count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Set offset into buffer for this write */
            offset += size;

            /* Account for size of this overflow record */
            size = dev->ckdcurdl;
            num = (*residual < size) ? *residual : size;
            *residual -= num;

            /* Write the next data field */
            rc = ckd_write_data (dev, iobuf+offset, num, unitstat);
            if (rc < 0) break;
        }

        /* Bail out if track overflow produced I/O error */
        if (rc < 0) break;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x0D:
    /*---------------------------------------------------------------*/
    /* WRITE KEY AND DATA                                            */
    /*---------------------------------------------------------------*/
        /* Command reject if the current track is in the DSF area */
        if (dev->ckdcurcyl >= dev->ckdcyls)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not within the domain of a Locate Record
           and not preceded by a Search ID Equal that compared equal
           on all bytes */
           /*INCOMPLETE*/ /*Write CKD allows intervening Read/Write
             key and data commands, Write Key Data does not!!! Rethink
             the handling of these flags*/
        if (dev->ckdlcount == 0 && dev->ckdideq == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if file mask inhibits all write commands */
        if ((dev->ckdfmask & CKDMASK_WRCTL) == CKDMASK_WRCTL_INHWRT)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!(0
                  ||     (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK
                  || (1
                      && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
                      && dev->ckdlcount == ((dev->ckdlaux & CKDLAUX_RDCNTSUF) ? 2 : 1)
                     )
                 )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }

            /* If not operating in CKD conversion mode, check that the
               key + data length equals the transfer length factor */
            if ((dev->ckdxgattr & CKDGATR_CKDCONV) == 0
                && dev->ckdcurkl + dev->ckdcurdl != dev->ckdltranlf)
            {
                /* Unit check with invalid track format */
                ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        } /* end if(ckdlcount) */

        /* If data length is zero, terminate with unit exception */
        if (dev->ckdcurdl == 0)
        {
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
            break;
        }

        /* Calculate number of bytes written and set residual count */
        size = dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Write key and data */
        rc = ckd_write_kd (dev, iobuf, num, unitstat);
        if (rc < 0) break;

        /* If track overflow, keep writing */
        offset = dev->ckdcurkl;
        while (dev->ckdtrkof)
        {
            /* Advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;

            /* Read the first count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Set offset into buffer for this write */
            offset += size;

            /* Account for size of this overflow record */
            size = dev->ckdcurdl;
            num = (*residual < size) ? *residual : size;
            *residual -= num;

            /* Write the next data field */
            rc = ckd_write_data (dev, iobuf+offset, num, unitstat);
            if (rc < 0) break;
        }

        /* Bail out if track overflow produced I/O error */
        if (rc < 0) break;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x8D:
    /*---------------------------------------------------------------*/
    /* WRITE UPDATE KEY AND DATA                                     */
    /*---------------------------------------------------------------*/
        /* Command reject if not within the domain of a Locate Record
           that specifies the Write Data operation code */
        if (dev->ckdlcount == 0
            || (dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRITE)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Orient to next user record count field */
        if (dev->ckdorient != CKDORIENT_COUNT
            || dev->ckdcurrec == 0)
        {
            /* Read next count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;
        }

        /* If not operating in CKD conversion mode, check that the
           data length is equal to the transfer length factor */
        if ((dev->ckdxgattr & CKDGATR_CKDCONV) == 0)
        {
            if ((dev->ckdcurkl + dev->ckdcurdl) != dev->ckdltranlf)
            {
                /* Unit check with invalid track format */
                ckd_build_sense( dev, 0, SENSE1_ITF, 0, 0, 0 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* If data length is zero, terminate with unit exception */
        if (dev->ckdcurdl == 0)
        {
            *unitstat = CSW_CE | CSW_DE | CSW_UX;
            break;
        }

        /* Calculate number of bytes written and set residual count */
        size = dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Write key and data */
        rc = ckd_write_kd (dev, iobuf, num, unitstat);
        if (rc < 0) break;

        /* If track overflow, keep writing */
        offset = dev->ckdcurkl;
        while (dev->ckdtrkof)
        {
            /* Advance to next track */
            rc = mt_advance (dev, unitstat, 1);
            if (rc < 0) break;

            /* Read the first count field */
            rc = ckd_read_count (dev, code, &rechdr, unitstat);
            if (rc < 0) break;

            /* Set offset into buffer for this write */
            offset += size;

            /* Account for size of this overflow record */
            size = dev->ckdcurdl;
            num = (*residual < size) ? *residual : size;
            *residual -= num;

            /* Write the next data field */
            rc = ckd_write_data (dev, iobuf+offset, num, unitstat);
            if (rc < 0) break;
        }

        /* Bail out if track overflow produced I/O error */
        if (rc < 0) break;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x11:
    /*---------------------------------------------------------------*/
    /* ERASE                                                         */
    /*---------------------------------------------------------------*/
        /* Command reject if the current track is in the DSF area */
        if (dev->ckdcurcyl >= dev->ckdcyls)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not within the domain of a Locate Record
           and not preceded by either a Search ID Equal or Search Key
           Equal that compared equal on all bytes, or a Write R0 or
           Write CKD not within the domain of a Locate Record */
        if (dev->ckdlcount == 0 && dev->ckdideq == 0
            && dev->ckdkyeq == 0 && dev->ckdwckd == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if file mask does not permit Write CKD */
        if ((dev->ckdfmask & CKDMASK_WRCTL) != CKDMASK_WRCTL_ALLWRT
            && (dev->ckdfmask & CKDMASK_WRCTL) != CKDMASK_WRCTL_INHWR0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if ((dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRTTRK)
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* Write end of track marker */
        rc = ckd_erase (dev, iobuf, count, (int*)&size, unitstat);
        if (rc < 0) break;

        /* Calculate number of bytes used and set residual count */
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x15:
    /*---------------------------------------------------------------*/
    /* WRITE RECORD ZERO                                             */
    /*---------------------------------------------------------------*/
        /* Command reject if the current track is in the DSF area */
        if (dev->ckdcurcyl >= dev->ckdcyls)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not within the domain of a Locate Record
           and not preceded by either a Search Home Address that
           compared equal on all 4 bytes, or a Write Home Address not
           within the domain of a Locate Record */
        /* ISW20030819-1 : Added check for previously issued WRHA */
        if (dev->ckdlcount == 0 && dev->ckdhaeq == 0 && dev->ckdwrha == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if file mask does not permit Write R0 */
        if ((dev->ckdfmask & CKDMASK_WRCTL) != CKDMASK_WRCTL_ALLWRT)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!((dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT
                    && ((dev->ckdloper & CKDOPER_ORIENTATION)
                                == CKDOPER_ORIENT_HOME
                          || (dev->ckdloper & CKDOPER_ORIENTATION)
                                == CKDOPER_ORIENT_INDEX
                       )))
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* Write R0 count key and data */
        rc = ckd_write_ckd (dev, iobuf, count, unitstat, 0);
        if (rc < 0) break;

        /* Calculate number of bytes written and set residual count */
        size = CKD_RECHDR_SIZE + dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        /* Set flag if Write R0 outside domain of a locate record */
        if (dev->ckdlcount == 0)
            dev->ckdwckd = 1;
        else
            dev->ckdwckd = 0;

        break;

    case 0x1D: /* WRITE CKD */
    case 0x01: /* WRITE SPECIAL CKD */
    /*---------------------------------------------------------------*/
    /* WRITE COUNT KEY AND DATA                                      */
    /*---------------------------------------------------------------*/
        /* Command reject if the current track is in the DSF area */
        if (dev->ckdcurcyl >= dev->ckdcyls)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if previous command was a Write R0 that
           assigned an alternate track - not implemented */

        /* Command reject if not within the domain of a Locate Record
           and not preceded by either a Search ID Equal or Search Key
           Equal that compared equal on all bytes, or a Write R0 or
           Write CKD not within the domain of a Locate Record */
        if (dev->ckdlcount == 0 && dev->ckdideq == 0
            && dev->ckdkyeq == 0 && dev->ckdwckd == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if file mask does not permit Write CKD */
        if ((dev->ckdfmask & CKDMASK_WRCTL) != CKDMASK_WRCTL_ALLWRT
            && (dev->ckdfmask & CKDMASK_WRCTL) != CKDMASK_WRCTL_INHWR0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if WRITE SPECIAL CKD to a 3380 or 3390 */
        if ((code == 0x01) // WRITE SPECIAL CKD
            && ((dev->devtype == 0x3380) || (dev->devtype == 0x3390)))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check operation code if within domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            if (!((dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT
                  || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK))
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }
        }

        /* Set track overflow flag if WRITE SPECIAL CKD */
        trk_ovfl = (1
                    && code == 0x01 // WRITE SPECIAL CKD
                    && dev->ckdcyls < 32768
                   )
                   ? 1 : 0;

        /* Write count key and data */
        rc = ckd_write_ckd (dev, iobuf, count, unitstat, trk_ovfl);
        if (rc < 0) break;

        /* Calculate number of bytes written and set residual count */
        size = CKD_RECHDR_SIZE + dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        /* Set flag if Write CKD outside domain of a locate record */
        if (dev->ckdlcount == 0)
            dev->ckdwckd = 1;
        else
            dev->ckdwckd = 0;

        break;

    case 0x9D:
    /*---------------------------------------------------------------*/
    /* WRITE COUNT KEY AND DATA NEXT TRACK                           */
    /*---------------------------------------------------------------*/
        /* Command reject if not within the domain of a Locate Record
           that specifies a format write operation */
        if (dev->ckdlcount == 0
            || (dev->ckdloper & CKDOPER_CODE) != CKDOPER_FORMAT)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not chained from a Write CKD or
           another Write CKD Next Track command */
        if (chained == 0
            || (prevcode != 0x1D && prevcode != 0x9D))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Advance to next track */
        rc = mt_advance (dev, unitstat, 1);
        if (rc < 0) break;

        /* Read the count field for record zero */
        rc = ckd_read_count (dev, code, &rechdr, unitstat);
        if (rc < 0) break;

        /* Write count key and data */
        rc = ckd_write_ckd (dev, iobuf, count, unitstat, 0);
        if (rc < 0) break;

        /* Calculate number of bytes written and set residual count */
        size = CKD_RECHDR_SIZE + dev->ckdcurkl + dev->ckdcurdl;
        num = (count < size) ? count : size;
        *residual = count - num;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;

        break;

    case 0x47:
        /*---------------------------------------------------------------*/
        /* LOCATE RECORD                                                 */
        /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = (count < 16) ? count : 16;
        *residual = count - num;

        /* Control information length must be at least 16 bytes */
        if (count < 16)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record,
           or not preceded by a Define Extent or Read IPL command */
        if (dev->ckdlcount > 0
            || (dev->ckdxtdef == 0 && dev->ckdrdipl == 0))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Byte 0 contains the locate record operation byte */
        dev->ckdloper = iobuf[0];

        /* Validate the locate record operation code (bits 2-7) */
        if (!(0
              ||  (dev->ckdloper & CKDOPER_CODE) == CKDOPER_ORIENT
              ||  (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
              ||  (dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT
              ||  (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
              ||  (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK
              ||  (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDTRKS
              ||  (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
             )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check for valid combination of orientation and opcode */
        if (0
            || (1
                && (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_HOME
                && !(0
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_ORIENT
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDTRKS
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                    )
               )
            || (1
                && (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_DATA
                && !(0
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_ORIENT
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDDATA
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                    )
               )
            || (1
                && (dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_INDEX
                && !(0
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                    )
               )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Check for write operation on a read only disk */
        if (1
            &&  dev->ckdrdonly  // (base image is read-only)
            && !dev->ckdfakewr  // (and fake write option not specified)
            && !dev->dasdsfn    // (and no shadow file specified)
            && (0
                || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
                || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT
                || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK
               )
        )
        {
            ckd_build_sense( dev, SENSE_EC, SENSE1_WRI, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Byte 1 contains the locate record auxiliary byte */
        dev->ckdlaux = iobuf[1];

        /* Validate the auxiliary byte */
        if (0
            || (dev->ckdlaux & CKDLAUX_RESV) != 0
            || (1
                && (dev->ckdlaux & CKDLAUX_RDCNTSUF)
                && !(0
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
                     || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_READ16
                    )
               )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Byte 2 must contain zeroes */
        if (iobuf[2] != 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Byte 3 contains the locate record domain count */
        dev->ckdlcount = iobuf[3];

        /* Validate the locate record domain count */
        if (0
            || (1
                && (dev->ckdloper & CKDOPER_CODE) == CKDOPER_ORIENT
                &&  dev->ckdlcount != 0
               )
            || (1
                && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_ORIENT
                &&  dev->ckdlcount == 0
               )
            || ((1
                 && dev->ckdlaux & CKDLAUX_RDCNTSUF)
                 && dev->ckdlcount < 2
               )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Bytes 4-7 contain the seek address */
        cyl = (iobuf[4] << 8) | iobuf[5];
        head = (iobuf[6] << 8) | iobuf[7];

        /* Command reject if seek address is not valid */
        if (cyl >= dev->ckdcyls || head >= dev->ckdheads)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* File protect error if seek address is outside extent */
        if (EXTENT_CHECK(dev, cyl, head))
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Bytes 8-12 contain the search argument */
        memcpy(cchhr, &iobuf[8], 5);

        /* Byte 13 contains the sector number */
        sector = iobuf[13];

        /* Command reject if sector number is not valid */
        if (sector != 0xFF && sector >= dev->ckdtab->sectors)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Bytes 14-15 contain the transfer length factor */
        dev->ckdltranlf = (iobuf[14] << 8) | iobuf[15];

        /* Validate the transfer length factor */
        if (0
            || (1
                && (dev->ckdlaux & CKDLAUX_TLFVALID) == 0
                &&  dev->ckdltranlf != 0
               )
            || (1
                && (dev->ckdlaux & CKDLAUX_TLFVALID) != 0
                &&  dev->ckdltranlf == 0
               )
            || dev->ckdltranlf > dev->ckdxblksz
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* If transfer length factor is not supplied then use
           the blocksize from the define extent command */
        if ((dev->ckdlaux & CKDLAUX_TLFVALID) == 0)
            dev->ckdltranlf = dev->ckdxblksz;

        /* Reset/initialize our saved first Read Count */
        dev->ckdfcoun = 0;
        memset( dev->ckdfcwrk, 0x00, sizeof( dev->ckdfcwrk ));

        /* Seek to the required track */
        rc = ckd_seek(dev, cyl, head, &trkhdr, unitstat);
        if (rc < 0)
            break;

        /* Set normal status */
        *unitstat = CSW_CE | CSW_DE;

        /* Perform search according to specified orientation */
        switch ((dev->ckdloper & CKDOPER_ORIENTATION)) {

        case CKDOPER_ORIENT_HOME:
            /* For home orientation, compare the search CCHH
               with the CCHH in the track header */
            if (memcmp(&(trkhdr.cyl), cchhr, 4) != 0)
            {
                ckd_build_sense( dev, 0, SENSE1_NRF, 0, 0, 0 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
            }
            break;

        case CKDOPER_ORIENT_COUNT:
        case CKDOPER_ORIENT_DATA:

            /* For count or data orientation, search the track
               for a count field matching the specified CCHHR */

            while (1)
            {
                /* Read next count field and exit at end of track
                   with sense data indicating no record found */
                rc = ckd_read_count( dev, code, &rechdr, unitstat );
                if (rc < 0)
                    break; // (end of track)

                /* Turn off track overflow flag */
                if (dev->ckdcyls < 32768)
                    rechdr.cyl[0] &= 0x7F;

                /* Compare the count field with the search CCHHR */
                if (memcmp( &rechdr, cchhr, 5 ) == 0)
                    break; // (found it!)

            } /* end while */

        } /* end switch(CKDOPER_ORIENTATION) */

        /* Exit if search ended with error status */
        if (*unitstat != (CSW_CE | CSW_DE))
            break;

        /* We found what we were looking for.
           Reorient past data if data orientation is specified
        */
        if ((dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_DATA)
        {
            /* Skip past key and data fields */
            dev->bufoff += dev->ckdcurkl + dev->ckdcurdl;

            /* Set the device orientation fields */
            dev->ckdrem = 0;
            dev->ckdorient = CKDORIENT_DATA;
        }

        /* Set locate record flag and return normal status */
        dev->ckdlocat = 1;
        break;

    case 0x4B:
    /*---------------------------------------------------------------*/
    /* LOCATE RECORD EXTENDED                                        */
    /*---------------------------------------------------------------*/

        if (LocateRecordExtended( dev, code, flags, chained, count, prevcode,
                                  ccwseq, iobuf, more, unitstat, residual ))
        {
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    case 0x63:
    /*---------------------------------------------------------------*/
    /* DEFINE EXTENT                                                 */
    /*---------------------------------------------------------------*/

        if (DefineExtent( dev, code, flags, chained, count, prevcode,
                          ccwseq, iobuf, more, unitstat, residual ))
        {
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    case 0x64:
    /*---------------------------------------------------------------*/
    /* READ DEVICE CHARACTERISTICS                                   */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not 3380 or 3390 or 9345 */
        if ((dev->devtype != 0x3380)
         && (dev->devtype != 0x3390)
         && (dev->devtype != 0x9345))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Calculate residual byte count */
        num = (count < dev->numdevchar) ? count : dev->numdevchar;
        *residual = count - num;
        if (count < dev->numdevchar) *more = 1;

        /* Copy device characteristics bytes to channel buffer */
        memcpy (iobuf, dev->devchar, num);

        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x3E:
    /*---------------------------------------------------------------*/
    /* READ SUBSYSTEM DATA                                           */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record,
           or if subsystem data has not been prepared in the channel
           buffer by a previous Perform Subsystem Function command */
        if (dev->ckdlcount > 0 || dev->ckdssdlen == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Calculate residual byte count */
        num = (count < dev->ckdssdlen) ? count : dev->ckdssdlen;
        *residual = count - num;
        if (count < dev->ckdssdlen) *more = 1;

        /* Subsystem data is already in the channel buffer, so
           just return channel end and device end */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x5B:
    /*---------------------------------------------------------------*/
    /* SUSPEND MULTIPATH RECONNECTION                                */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xF3:
    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC CONTROL                                            */
    /*---------------------------------------------------------------*/
        /* Command reject if SSI active */
        if(dev->ckdssi)
        {
            /* Mark Set Special Intercept inactive */
            dev->ckdssi = 0;
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_F );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Calculate residual byte count */
        num = (count < 4) ? count : 4;
        *residual = count - num;

        /* Control information length must be at least 4 bytes */
        if (count < 4)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if byte 0 does not contain a valid
           subcommand code, or if bytes 2-3 are not zero */
        if (!(iobuf[0] == DIAGCTL_MAINT_QUERY
//            || iobuf[0] == DIAGCTL_MAINT_RESERVE
//            || iobuf[0] == DIAGCTL_MAINT_RELEASE
//            || iobuf[0] == DIAGCTL_INHIBIT_WRITE
//            || iobuf[0] == DIAGCTL_SET_GUAR_PATH
//            || iobuf[0] == DIAGCTL_ENABLE_WRITE
//            || iobuf[0] == DIAGCTL_3380_TC_MODE
//            || iobuf[0] == DIAGCTL_INIT_SUBSYS
//            || iobuf[0] == DIAGCTL_UNFENCE
//            || iobuf[0] == DIAGCTL_ACCDEV_UNKCOND
             ) || iobuf[2] != 0 || iobuf[3] != 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if file mask does not specify
           diagnostic or device support authorization */
        if ((dev->ckdfmask & CKDMASK_AAUTH) == CKDMASK_AAUTH_NORMAL)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_5 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x94:
    /*---------------------------------------------------------------*/
    /* DEVICE RELEASE                                                */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record, or
           preceded by Define Extent, Space Count, or Set File Mask,
           or (for 3390 only) preceded by Read IPL */
        if (dev->ckdlcount > 0
            || dev->ckdxtdef || dev->ckdspcnt || dev->ckdsetfm
            || (dev->ckdrdipl && dev->devtype == 0x3390))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Call the release exit and mark the device not reserved */
        if (dev->hnd->release) (dev->hnd->release) (dev);

        OBTAIN_DEVLOCK( dev );
        {
            dev->reserved = 0;
        }
        RELEASE_DEVLOCK( dev );

        /* Perform the operation of a sense command */
        goto sense;

    case 0x14: /* UNCONDITIONAL RESERVE */
    case 0xB4: /* DEVICE RESERVE */
    /*---------------------------------------------------------------*/
    /* DEVICE RESERVE                                                */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record,
           or indeed if preceded by any command at all apart from
           Suspend Multipath Reconnection */
        if (dev->ckdlcount > 0
            || ccwseq > 1
            || (chained && prevcode != 0x5B))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Mark the device reserved and call the reserve exit */

        OBTAIN_DEVLOCK( dev );
        {
            dev->reserved = 1;
        }
        RELEASE_DEVLOCK( dev );

        if (dev->hnd->reserve) (dev->hnd->reserve) (dev);

        /* Perform the operation of a sense command */
        goto sense;

    case 0x04:
    /*---------------------------------------------------------------*/
    /* SENSE                                                         */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

    sense:
        /* If sense bytes are cleared then build sense */
        if ((dev->sense[0] == 0) & (dev->sense[1] == 0))
            ckd_build_sense( dev, 0, 0, 0, 0, 0 );

        /* Calculate residual byte count */
        num = (count < dev->numsense) ? count : dev->numsense;
        *residual = count - num;
        if (count < dev->numsense) *more = 1;

        /* Copy device sense bytes to channel I/O buffer */
        memcpy (iobuf, dev->sense, num);

        /* Clear the device sense bytes */
        memset( dev->sense, 0, sizeof(dev->sense) );

        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xE4:
    /*---------------------------------------------------------------*/
    /* SENSE ID                                                      */
    /*---------------------------------------------------------------*/

        /* If numdevid is 0, then 0xE4 Sense ID is not supported */
        if (dev->numdevid == 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Calculate residual byte count */
        num = (count < dev->numdevid) ? count : dev->numdevid;
        *residual = count - num;
        if (count < dev->numdevid) *more = 1;

        /* Copy device identifier bytes to channel I/O buffer */
        memcpy (iobuf, dev->devid, num);

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x34:
    /*---------------------------------------------------------------*/
    /* SENSE PATH GROUP ID                                           */
    /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = (count < 12) ? count : 12;
        *residual = count - num;
        if (count < 12) *more = 1;

        /* Byte 0 is the path group state byte */
        iobuf[0] = SPG_PATHSTAT_RESET
                | SPG_PARTSTAT_IENABLED
                | SPG_PATHMODE_SINGLE;

        /* If reserved then bits 2 & 3 must be set */
        if (dev->reserved)
        {
            iobuf[0] |= SPG_PATHSTAT_RESERVED;
        }

        /* Bytes 1-11 contain the path group identifier */
        memcpy (iobuf+1, dev->pgid, 11);

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xAF:
    /*---------------------------------------------------------------*/
    /* SET PATH GROUP ID                                             */
    /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = (count < 12) ? count : 12;
        *residual = count - num;

        /* Control information length must be at least 12 bytes */
        if (count < 12)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Byte 0 is the path group state byte */
        if ((iobuf[0] & SPG_SET_COMMAND) == SPG_SET_ESTABLISH)
        {
            /* Only accept the new pathgroup id when
               1) it has not yet been set (ie contains zeros) or
               2) It is set, but we are setting the same value */
            if(memcmp(dev->pgid,
                 "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 11)
              && memcmp(dev->pgid, iobuf+1, 11))
            {
                dev->sense[0] = SENSE_CR;
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                break;
            }

            /* Bytes 1-11 contain the path group identifier */
            memcpy (dev->pgid, iobuf+1, 11);
        }

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x54:
    /*---------------------------------------------------------------*/
    /* SENSE SUBSYSTEM STATUS                                        */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record,
           or if chained from any command unless the preceding command
           is Read Device Characteristics, Read Configuration Data, or
           a Suspend Multipath Reconnection command that was the first
           command in the chain */
        if (dev->ckdlcount > 0
            || (chained && prevcode != 0x64 && prevcode != 0xFA
                && prevcode != 0x5B)
            || (chained && prevcode == 0x5B && ccwseq > 1))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Build the basic subsystem status data in the I/O area */
        num = dasd_build_ckd_subsys_status (dev, iobuf, count);

        /* Calculate residual byte count */
        *residual = count < num ? 0 : count - num;
        *more = count < num;

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xA4:
    /*---------------------------------------------------------------*/
    /* READ AND RESET BUFFERED LOG                                   */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record,
           or if chained from any command unless the preceding command
           is Read Device Characteristics, Read Configuration Data, or
           a Suspend Multipath Reconnection command that was the first
           command in the chain */
        if (dev->ckdlcount > 0
            || (chained && prevcode != 0x64 && prevcode != 0xFA
                && prevcode != 0x5B)
            || (chained && prevcode == 0x5B && ccwseq > 1))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Calculate residual byte count */
        num = (count < 32) ? count : 32;
        *residual = count - num;
        if (count < 32) *more = 1;

        /* Build the buffered error log in the I/O area */
        memset (iobuf, 0, 32);

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x87:
    /*---------------------------------------------------------------*/
    /* Set Subsystem Mode                                            */
    /*---------------------------------------------------------------*/

        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Command reject if not a cached device, first in chain, or */
        /* immediately preceded by Suspend Multipath Connection      */
        /*                                                           */
        /* TBD: Add first in chain and Suspend Multipath check       */
        /*                                                           */
        if ((dev->ckdcu->devt != 0x3990 && dev->ckdcu->devt != 0x2105)
            || (dev->ckdcu->model & 0x07) == 0x02)      /* 3990-1/2  */
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Calculate residual byte count */
        num = (count < 2) ? count : 2;
        *residual = count - num;

        /* Control information length must be at least 2 bytes */
        if (count < 2)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* TBD / MGD:   Complete checks for Set Subsystem Mode      */
        /*              covering message required flag and read     */
        /*              message id check                            */

        /* Validate operands -- Refer to 2105 validation sequence   */
        if (((iobuf[0] & 0x02) != 0) || ((iobuf[1] & 0x07) != 0)    ||
            ((iobuf[1] & 0x18) != 0) /* zero unless in TPF mode */  ||
            ((iobuf[0] & 0xE0) > 0xA0)                              ||
            ((iobuf[0] & 0x1C) > 0x14)                              ||
            ((iobuf[1] & 0xE0) > 0xA0)                              ||
            ((iobuf[1] & 0x18) == 0x18) /* TPF reserved */          ||
            (((iobuf[0] & 0x10) != 0) && (
               ((iobuf[0] & 0xE0) >  0x80) ||
               ((iobuf[0] & 0xE0) <  0x40) ||
               ((iobuf[0] & 0x1C) != 0x10) ||
               ((iobuf[1] & 0xE0) >  0xA0) ||
               ((iobuf[1] & 0xE0) <  0x40) ||
               ((iobuf[1] & 0xE0) == 0x60)))                        ||
            (((iobuf[0] & 0xE0) != 0) && (
               ((iobuf[0] & 0x1C) != 0) || (iobuf[1] != 0)))        ||
            (((iobuf[0] & 0x1C) != 0) && (iobuf[1] != 0)))
        {

            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* TBD / Future:        Cache Fast Write Data Control        */

        /* Treat as NOP and Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xFA:
    /*---------------------------------------------------------------*/
    /* READ CONFIGURATION DATA                                       */
    /*---------------------------------------------------------------*/
        /* Command reject if within the domain of a Locate Record */
        if (dev->ckdlcount > 0)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Build the configuration data area */
        num = dev->rcd( dev, iobuf, count );

        /* Calculate residual byte count */
        *residual = count < num ? 0 : count - num;
        *more = count < num;

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    default:
    /*---------------------------------------------------------------*/
    /* INVALID OPERATION                                             */
    /*---------------------------------------------------------------*/
        /* Set command reject sense byte, and unit check status */
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;

    } /* end switch(code) */

    /* Reset the flags which ensure correct positioning for write
       commands */

    /* Reset search HA flag if command was not SEARCH HA EQUAL
       or WRITE HA */
    if (1
        && (code & 0x7F) != 0x39 // SEARCH HOME ADDRESS EQUAL
        && (code & 0x7F) != 0x19 // WRITE HOME ADDRESS
    )
        dev->ckdhaeq = 0;

    /* Reset search id flag if command was not SEARCH ID EQUAL,
       READ/WRITE KEY AND DATA, or READ/WRITE DATA */
    if (1
        && (code & 0x7F) != 0x31 // SEARCH ID EQUAL
        && (code & 0x7F) != 0x0E // READ KEY AND DATA
        && (code & 0x7F) != 0x0D // WRITE KEY AND DATA
        && (code & 0x7F) != 0x06 // READ DATA
        && (code & 0x7F) != 0x05 // WRITE DATA
    )
        dev->ckdideq = 0;

    /* Reset search key flag if command was not SEARCH KEY EQUAL
       or READ/WRITE DATA */
    if (1
        && (code & 0x7F) != 0x29 // SEARCH KEY EQUAL
        && (code & 0x7F) != 0x06 // READ DATA
        && (code & 0x7F) != 0x05 // WRITE DATA
    )
        dev->ckdkyeq = 0;

    /* Reset write CKD flag if command was not WRITE R0 or WRITE CKD */
    if (1
        && code != 0x15 // WRITE RECORD ZERO
        && code != 0x1D // WRITE CKD
    )
        dev->ckdwckd = 0;

    /* If within the domain of a locate record then decrement the
       count of CCWs remaining to be processed within the domain */
    if (1
        && dev->ckdlcount > 0
        && code != 0x47 // LOCATE RECORD
        && code != 0x4B // LOCATE RECORD EXTENDED
        && code != 0xE7 // PREFIX
    )
    {
        /* Decrement the count of CCWs remaining in the domain */
        if (code == 0x92) // READ COUNT
        {
            if (!(flags & CCW_FLAGS_CC))
                dev->ckdlcount--;
            else
                if (dev->ckdextcd != 0x0a && dev->ckdxcode != 0x0e)
                    dev->ckdlcount--;
        }
        else
            dev->ckdlcount--;

        /* Command reject with incomplete domain if CCWs remain
           but command chaining is not specified */
        if (1
            && dev->ckdlcount > 0
            && (flags & CCW_FLAGS_CC) == 0
            && code != 0x02 // READ IPL
        )
        {
            ckd_build_sense( dev, SENSE_CR | SENSE_OC, 0, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
    } /* end if(ckdlcount) */

} /* end function ckd_dasd_execute_ccw */

#if defined( GCC_VERSION ) && GCC_VERSION >= 110000 /* gcc >= 11.0.0 */
PUSH_GCC_WARNINGS()
DISABLE_GCC_WARNING( "-Wmaybe-uninitialized" )
#endif

/*-------------------------------------------------------------------*/
/* PERFORM SUBSYSTEM FUNCTION    helper function                     */
/*-------------------------------------------------------------------*/
static bool PerformSubsystemFunction
(
    DEVBLK*  dev,
    BYTE     code,
    BYTE     flags,
    BYTE     chained,
    U32      count,
    BYTE     prevcode,
    int      ccwseq,
    BYTE*    iobuf,
    BYTE*    more,
    BYTE*    unitstat,
    U32*     residual
)
{
    int    i;
    U32    num;
    BYTE*  orig_iobuf;
    BYTE   ccwdata[ IOBUF_MINSIZE ];

    /* If E7 Prefix CCW, move the iobuff data to where this function
       expects it */
    if (code == 0xE7)
    {
        memmove( ccwdata, &iobuf[62], sizeof( ccwdata ) - 62 );
        count -= (count >= 62 ? 62 : count);
        orig_iobuf = iobuf; // (save for later!)
        iobuf = &ccwdata[0];
    }

    UNREFERENCED( more );

    /* Command reject if within the domain of a Locate Record */
    if (dev->ckdlcount > 0)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Use the order code to determine the required count */
    num = (count < 2) ? 2 :
          (iobuf[0] == 0x10) ? 14 :
          (iobuf[0] == 0x11 || iobuf[0] == 0x18) ? 12 :
          (iobuf[0] == 0x12) ? 5 :
          (iobuf[0] == 0x13 || iobuf[0] == 0x14) ? 4 :
          (iobuf[0] == 0x16) ? 4 :
          (iobuf[0] == 0x1D) ? 66 :
          (iobuf[0] == 0xB0) ? 4 :
          2;

    /* Command reject if count is less than required */
    if (count < num)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Set residual count */
    *residual = count - num;

#if 0
    /* Command reject if SSI active */
    if (dev->ckdssi)
    {
        /* Reset SSI condition */
        dev->ckdssi = 0;
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_F );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }
#endif

    /* Process depending on order code in byte 0 of data */
    switch (iobuf[0]) {

    case 0x18: /* Prepare for Read Subsystem Data */

        // Reject invalid parameter bytes...

        if (0
            //---------------------------------------------------
            //  Flag byte must be zero
            //---------------------------------------------------
            || iobuf[1]

            //---------------------------------------------------
            //  If byte 6 == 01 then byte 7 can only be 00 or FF.
            //  For any other byte 6 value, byte 7 must be 00.
            //---------------------------------------------------

            || (iobuf[6] == 0x01 && (iobuf[7] != 0x00 && iobuf[7] != 0xff))
            || (iobuf[6] != 0x01 && (iobuf[7] != 0x00))

            //---------------------------------------------------
            //  If byte 6 == 00, 02, 04, or 06-FF, then
            //  bytes 8-11 are reserved and must be zero.
            //---------------------------------------------------
            || ((0
                 || iobuf[6] == 0x00
                 || iobuf[6] == 0x02
                 || iobuf[6] == 0x04
                 || iobuf[6] >= 0x06    // (06 through FF)
                )
                && mem_ne( &iobuf[8], eighthex00, 4 )
               )

            //---------------------------------------------------
            //  If byte 6 == 01, then byte 8 must be 00 or 01
            //  and bytes 9-11 are reserved and must be zero.
            //---------------------------------------------------
            || (1
                && iobuf[6] == 0x01
                && (0
                    || (iobuf[8] != 0x00 && iobuf[8] != 0x01)
                    || mem_ne( &iobuf[9], eighthex00, 3 )
                   )
               )

            //---------------------------------------------------
            //  If byte 6 == 1C, bytes 2-3 must be zero.
            //  Otherwise bytes 2-5 must always be zero.
            //---------------------------------------------------
            || (iobuf[6] == 0x1C && mem_ne( &iobuf[2], eighthex00, 2 ))
            || (iobuf[6] != 0x1C && mem_ne( &iobuf[2], eighthex00, 4 ))
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        /* Process suborder code in byte 6 of data */
        switch (iobuf[6]) {

        case 0x00: /* Storage path status */
            /* Prepare storage path status record */
            memset (iobuf, 0, 16);
            iobuf[0] = 0xC0; /* Storage path valid and attached */
            iobuf[1] = 0x80; /* Logical paths configured bitmap */
            iobuf[2] = 0x00; /* Channels enabled bitmap */
            iobuf[3] = 0x00; /* Channels fenced bitmap */
            iobuf[16] = 1;   /* #logical paths thru cluster 0 */

            /* Indicate the length of subsystem data prepared */
            dev->ckdssdlen = (dev->ckdcu->code == 0x15) ? 24 : 16;
            break;

        case 0x01: /* Subsystem statistics */
            /* Indicate the length of subsystem data prepared */
            dev->ckdssdlen = (iobuf[8]==0x00) ? 96 : 192;

            /* Prepare subsystem statistics record */
            memset (iobuf, 0, dev->ckdssdlen);
            iobuf[1] = dev->devnum & 0xFF;
            store_hw( &iobuf[94], myssid );
            break;

        case 0x03:  /* Read attention message for this path-group for
                       the addressed device. Returns a "No Message"
                       message */
            /*------------------------------------------------------*/
            /* PROGRAMMING NOTE: 2013/01/09 Fish                    */
            /* According to GA32-0274 IBM 3990,9390 Storage Control */
            /* Reference the Read Attention Message response should */
            /* be 11 bytes for a 3990-6 with Message Format byte 2  */
            /* being 0x02 (3990-6/ESS message) with byte 9 and 10   */
            /* containing additional response information such as a */
            /* bit map indicating which physical subsystem SPs are  */
            /* caching. Since Hercules doesn't support a compatible */
            /* form of caching and an equivalent SP caching bit map */
            /* isn't maintained we return a 9 byte response instead */
            /* with byte 2 = 0x00 (No Message) since an 11 byte re- */
            /* sponse with zeros in bytes 9 and 10 causes problems  */
            /* with certain operating systems. Returning a 9-byte   */
            /* response with byte 2 = 0x00 instead is therefore the */
            /* safest approach to take since most operating systems */
            /* seem to accept such a response without complaint.    */
            /*------------------------------------------------------*/
            iobuf[0] = 0x00;               /* Message...            */
            iobuf[1] = 0x09;               /* ...Length             */
            iobuf[2] = 0x00;               /* Format: "No message"  */
            iobuf[3] = 0x00;               /* Message code: n/a     */
            memcpy (iobuf+4, iobuf+8, 4);  /* Copy same message Id
                                              from bytes 8-11       */
            iobuf[8] = 0x00;               /* Flags = 00            */
            dev->ckdssdlen = 9;            /* Len of prepared data  */
            break;

        case 0x0E: /* Unit address configuration */
            /* Prepare unit address configuration record */
            memset (iobuf, 0, 512);
            /* 256 pairs (UA type, base UA) */

            /* Indicate the length of subsystem data prepared */
            dev->ckdssdlen = 512;
            break;

        case 0x1C: /* Query Host Access */
            /*------------------------------------------------------*/
            /* PROGRAMMING NOTE: 2017-02-14 Ian                     */
            /*                                                      */
            /* IBM OA40720 PROBLEM DESCRIPTION says:-               */
            /*                                                      */
            /*   This provides new function support for DS8870      */
            /*   Query Host Access function                         */
            /*                                                      */
            /*   The MVS System DEVSERV QDASD command was enhanced  */
            /*   to support a new keyword 'QHA' which allows user   */
            /*   to query host access information for a CKD volume. */
            /*   The host access information includes a list of     */
            /*   Path Group IDs or systems that have a device       */
            /*   grouped (online) and/or ungrouped (offline) or     */
            /*   Reserved. In addition, the Path Status Flags, z/OS */
            /*   Sysplex name, and Maximum number of Cylinders per  */
            /*   Volume supported by the host are also returned for */
            /*   each Path Group entry.                             */
            /*                                                      */
            /* Linux support for QHA was added to kernel 4.6 with   */
            /* patch 8712731 in March 2016.                         */
            /*                                                      */
            /* iobuf[4]-[5] contains the device being queried.      */
            /* Other than the Linux kernel source (see structures   */
            /* dasd_psf_query_host_access and                       */
            /* dasd_ckd_host_information in                         */
            /* drivers/s390/block/dasd_eckd.h) I haven't discovered */
            /* any documentation explaining the contents of the     */
            /* response.                                            */
            /*------------------------------------------------------*/

#define QHA_RESPONSE_SIZE  16+4+32   /* 16 = the first 16-bytes of the  */
                                 /*      dasd_psf_query_host_access */
                                 /*      structure.                 */
                                 /*  4 = the first 4-bytes of the   */
                                 /*      dasd_ckd_host_information  */
                                 /*      structure.                 */
                                 /* 32 = an arbitrary entry length  */

            /* Indicate the length of prepared host access response */
            dev->ckdssdlen = QHA_RESPONSE_SIZE;

            /* Prepare host access response */
            memset (iobuf, 0, QHA_RESPONSE_SIZE);

            iobuf[17] = 0x20;   /* Entry size = 32 */
            iobuf[18] = 0x00;   /* Entry...        */
            iobuf[19] = 0x01;   /* ...count = 1    */

            break;

        case 0x41: /* Feature codes */
            /* Prepare feature codes record */
            memset (iobuf, 0, 256);

            /* Indicate the length of subsystem data prepared */
            dev->ckdssdlen = 256;
            break;

        default: /* Unknown suborder code */
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;

        } /* end switch(iobuf[6]) */

        break;

    case 0x1B: /* Set Special Intercept Condition */

        /* Command reject if not the first command in the chain
           or indeed if preceded by any command at all apart from
           Suspend Multipath Reconnection */
        if (0
            || ccwseq > 1
            || (1
                && chained
                && prevcode != 0x5B // SUSPEND MULTIPATH RECONNECTION
               )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        /* Command reject if flag byte is not zero */
        if (iobuf[1] != 0x00)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        /* Command reject if any command is chained from this command */
        if (flags & CCW_FLAGS_CC)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        /* Mark Set Special Intercept inactive */
        dev->ckdssi = 1;

        break;

    case 0x1D: /* Set Subsystem Characteristics */

        /* Command reject if flag byte is not zero */
        if (iobuf[1] != 0x00)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        break;

    case 0xB0: /* Set Interface Identifier */

        /* Command reject if flag byte bits 0-5 are not zero
           or bits 6-7 are 11 or 10 */
        if ((iobuf[1] & 0xFE) != 0x00)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        /* Prepare subsystem data (node descriptor record) */
        memset (iobuf, 0, 96);

        /* Bytes 0-31 contain the subsystem node descriptor */
        store_fw(&iobuf[0], 0x00000100);
        sprintf ((char *)&iobuf[4], "00%4.4X   HRCZZ000000000001",
                            dev->ckdcu->devt);
        for (i = 4; i < 30; i++)
            iobuf[i] = host_to_guest(iobuf[i]);

        /* Bytes 32-63 contain node qualifier data */
        store_fw( &iobuf[32], 0x00000000 ); // flags+zeros
        store_fw( &iobuf[40], 0x00000000 );
        store_fw( &iobuf[40], 0x41010000 ); // start range
        store_fw( &iobuf[44], 0x41010001 ); // end range
        store_fw( &iobuf[48], 0x41010010 ); // start range
        store_fw( &iobuf[52], 0x41010011 ); // end range

        /* Bytes 64-95 contain a 2nd subsystem node descriptor */
        iobuf[64] = 0x00;

        /* Indicate the length of subsystem data prepared */
        dev->ckdssdlen = (iobuf[1] & 0x03) ? 32 : 96;

        break;

    default: /* Unknown order code */

        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;

    } /* end switch(iobuf[0]) */

    /* Exit if unit check has already been set */
    if (*unitstat & CSW_UC)
        return false;

    /* If E7 Prefix CCW, move the prepared subsystem data (if any)
       back to where the channel expects it.
    */
    if (code == 0xE7 && dev->ckdssdlen)
        memmove( &orig_iobuf[0], &iobuf[0], dev->ckdssdlen );

    /* Return normal status */
    *unitstat = CSW_CE | CSW_DE;
    return true;

} /* end function PerformSubsystemFunction */

#if defined( GCC_VERSION ) && GCC_VERSION >= 110000 /* gcc >= 11.0.0 */
POP_GCC_WARNINGS()
#endif

/*-------------------------------------------------------------------*/
/* LOCATE RECORD EXTENDED    helper function                         */
/*-------------------------------------------------------------------*/
static bool LocateRecordExtended
(
    DEVBLK*  dev,
    BYTE     code,
    BYTE     flags,
    BYTE     chained,
    U32      count,
    BYTE     prevcode,
    int      ccwseq,
    BYTE*    iobuf,
    BYTE*    more,
    BYTE*    unitstat,
    U32*     residual
)
{
    int         rc;                     /* Return code               */
    int         i, j;                   /* Loop index                */
    bool        validate = true;        /* Validate all fields       */
    CKD_TRKHDR  trkhdr;                 /* CKD track header (HA)     */
    CKD_RECHDR  rechdr;                 /* CKD record header (count) */
    U32         num;                    /* Number of bytes to move   */
    U16         cyl;                    /* Cylinder number           */
    U16         head;                   /* Head number               */
    BYTE        cchhr[5];               /* Search argument           */
    BYTE        binzero[5];             /* Binary zeros              */
    BYTE        ccwdata[ IOBUF_MINSIZE ];

    /* If E7 Prefix CCW, move the iobuff data to where this function
       expects it */
    if (code == 0xE7)
    {
        memmove( ccwdata, &iobuf[44], sizeof( ccwdata ) - 44 );
        count -= (count >= 44 ? 44 : count);
        iobuf = &ccwdata[0];

        /* Don't bother validating any of the Locate Record Extended
           fields if the PFX Auxiliary Byte flag says they're valid. */
        if (dev->ckdauxiliary & PFX_A_VALID)
            validate = false;
    }

    UNREFERENCED( flags    );
    UNREFERENCED( chained  );
    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq   );
    UNREFERENCED( more     );

    /* LRE only valid for 3990-3 or 3990-6 (or greater) */
    if (0
        || dev->ckdcu->devt != 0x3990
        || !(0
             || MODEL3( dev->ckdcu )
             || MODEL6( dev->ckdcu )
            )
    )
    {
        /* Set command reject sense byte, and unit check status */
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_1 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * The Storage Director initially requests 20 bytes of parameters
     * from the channel; if the channel provides fewer than 20 bytes,
     * execution is terminated with status that includes unit check
     * (Command Reject, format X'03', CCW byte count less than required).
     */
    num = (count < 20) ? count : 20;
    *residual = count - num;

    if (count < 20)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * If Locate Record Extended is received within a Locate Record
     * domain, execution is terminated with status that includes unit
     * check (Command Reject, format X'02', Invalid Command Sequence).
     */
    if (dev->ckdlcount > 0)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * If Locate Record Extended was not preceded by a Define Extent
     * or Read IPL command in the same channel program, execution is
     * terminated with status that includes unit check (Command Reject,
     * format X'02', Invalid Command Sequence). If any other operation
     * is specified, the command is terminated with status that
     * includes unit check (Command Reject, format X'02', Invalid
     * Command Sequence).
     */
    //FIXME not sure what that last sentence means
    if (dev->ckdxtdef == 0 && dev->ckdrdipl == 0)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Byte 0 contains the locate record operation byte */
    dev->ckdloper = iobuf[0];

    /* Validate the locate record operation code (byte 0 bits 2-7) */
    if (1
        && validate
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRITE
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_FORMAT
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRTTRK
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_RDTRKS
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_READ16
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_RDDATA
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_EXTOP
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Validate the locate record extended operation code (byte 17) */
    if ((dev->ckdloper & CKDOPER_CODE) == CKDOPER_EXTOP)
    {
        if (1
            && validate
            && iobuf[17] != CKDOPER_WRTANY
            && iobuf[17] != CKDOPER_RDANY
            && iobuf[17] != CKDOPER_RDTSET
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        dev->ckdloper &= CKDOPER_ORIENTATION;
        dev->ckdloper |= iobuf[17];
    }
    else if (validate && iobuf[17] != 0)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Check for write operation on a read only disk */
    // ZZ FIXME (Not sure if this is right here)
    if (1
        &&  dev->ckdrdonly  // (base image is read-only)
        && !dev->ckdfakewr  // (and fake write option not specified)
        && !dev->dasdsfn    // (and no shadow file specified)
        && (0
            || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRITE
            || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTANY
            || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_FORMAT
            || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTTRK
           )
    )
    {
        ckd_build_sense( dev, SENSE_EC, SENSE1_WRI, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * Check for valid combination of orientation and opcode
     *
     * +------------------------------------------------+
     * | Operation Code          Orientation      Byte  |
     * |                      Cnt  HA Data Index    17  |
     * +------------------------------------------------+
     * | Write Data            01   x   81     x    00  |
     * | Format Write          03  43    x    C3    00  |
     * | Write Track           0B   x    x     x    00  |
     * | Read Tracks           0C  4C    x     x    00  |
     * | Read                  16  56   96    D6    00  |
     * | Read (E7 Prefix)      06  56   96    D6    00  |
     * | Write Any             3F   x    x     x    09  |
     * | Read Any              3F   x    x     x    0A  |
     * | Read Trackset         3F  7F    x     x    0E  |
     * +------------------------------------------------+
     * | Note:  x - Combination is not valid.           |
     * +------------------------------------------------+
     *
     * Table: valid orientation + operation code values
     */
    if (1
        &&  validate
        &&  dev->ckdloper != 0x01
        &&  dev->ckdloper != 0x03
        &&  dev->ckdloper != 0x0B
        &&  dev->ckdloper != 0x0C
        && (dev->ckdloper != 0x16 && code == 0x4B)
        && (dev->ckdloper != 0x06 && code == 0xE7) // E7 Prefix

        &&  dev->ckdloper != 0x43
        &&  dev->ckdloper != 0x4C
        &&  dev->ckdloper != 0x56

        &&  dev->ckdloper != 0x81
        &&  dev->ckdloper != 0x96

        &&  dev->ckdloper != 0xC3
        &&  dev->ckdloper != 0xD6

        &&  dev->ckdloper != 0x09
        &&  dev->ckdloper != 0x0A
        &&  dev->ckdloper != 0x0E
        &&  dev->ckdloper != 0x4E
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * Byte 1 is the Auxiliary Byte
     * bit 0 = 0 : Bytes 14-15 are unused
     *         1 : Bytes 14-15 contain a TLF that overrides the
     *             blocksize specified by the DX parameter.
     * bits 1-6  : Must be zero
     *             If any of these bits are '1', the LRE is terminated
     *             with status that includes unit check (Command Reject,
     *             format X'04', Invalid Parameter).
     * bit 7 = 0 : No Read Count CCW is suffixed to the LR domain
     *         1 : A Read Count CCW is suffixed to the LR domain
     */
    if (validate && (iobuf[1] & CKDLAUX_RESV) != 0)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * A Read Count command may only be suffixed to the domain of a LRE
     * that specifies a Write Data (01), Write Any (09), Read Any (0A),
     * or Read (16/06) operation code; if bit 7 = '1' when any other
     * Operation code is specified, Locate Record Extended is terminated
     * with status that includes unit check (Command Reject, format
     * X'04', Invalid Parameter).
     */
    if (1
        && validate
        && (iobuf[1] & CKDLAUX_RDCNTSUF)
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRITE
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRTANY
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_RDANY
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_READ16
        && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_RDDATA
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    dev->ckdlaux = iobuf[1];

    /* Byte 2 must contain zeroes */
    if (validate && iobuf[2] != 0)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * Byte 3 is the Count parameter. In general, the count parameter
     * specifies the number of records, or tracks to be operated on by
     * data transfer commands that follow Locate Record Extended.
     * Specific interpretation of the Count parameter depends upon the
     * operation code in byte 0.
     *
     * The Count must be nonzero. If Read Count Suffixing is specified
     * in a Locate Record, the count must be greater than 1. If the
     * Count is invalid, Locate Record Extended is terminated with
     * status that includes unit check (Command Reject, format X'04',
     * Invalid Parameter).
     */
    if (validate)
    {
        if (0
            || iobuf[3] == 0
            || (1
                && (dev->ckdlaux & CKDLAUX_RDCNTSUF)
                && iobuf[3] < 2
               )
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }
    }

    dev->ckdlcount = iobuf[3];

    /*
     * The value in bytes 4-7 must be a valid track address for the
     * device and must be within the extent boundaries specified by the
     * preceding Define Extent command.
     *
     * If the Seek Address is not valid for the device or if the Extended
     * Operation code is Write Any or Read Any and the seek address does
     * not specify a primary track, Locate Record Extended is terminated
     * with status that includes unit check (Command Reject, format X'04',
     * Invalid Parameter). If the Seek Address is not within the defined
     * extent, Locate Record Extended is terminated with status that
     * includes unit check (File Protected).
     */
    cyl  = fetch_hw( &iobuf[4] );
    head = fetch_hw( &iobuf[6] );

    if (validate)
    {
        if (0
            || cyl  >= dev->ckdcyls
            || head >= dev->ckdheads
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }
    }

    /* Make sure the Seek Address is within the defined begin and
       end of extent, but only if the LRE operation is NOT Read Any
       or Write Any.

       If the LRE is Read Any or Write Any, then since the specified
       track will never be left (i.e. the Read Any or Write Any never
       leaves the track, always returning back to the start of the
       track instead), we only need to make sure the Seek Address is
       within the defined BEGIN of extent since the END of extent is
       never reached in this case.
    */
    if (validate)
    {
        if (0
            || (1
                && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_RDANY
                && (dev->ckdloper & CKDOPER_CODE) != CKDOPER_WRTANY
                && EXTENT_CHECK( dev, cyl, head )
               )
            || (1
                && (0
                    || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_RDANY
                    || (dev->ckdloper & CKDOPER_CODE) == CKDOPER_WRTANY
                   )
                && EXTENT_BEGIN_CHECK( dev, cyl, head )
               )
        )
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }
    }

    /*
     * Bytes 8-12 specify a value to be used as a search argument for
     * the Locate Record Extended search operation.
     *
     * When the operation specified in byte 0 does not require
     * orientation to a specific record, no search operation is
     * performed and bytes 8-12 are ignored. When Home Address
     * orientation is specified, byte 12 is ignored.
     */
    memcpy( cchhr, &iobuf[8], 5 );

    /*
     * Byte 13 contains a sector number to which the device is to be
     * positioned before the Storage Director establishes orientation.
     *
     * The sector number must be within the range of valid sector
     * numbers for the device. If the sector number is invalid, Locate
     * Record Extended is terminated with status that includes unit
     * check (Command Reject, format X'04', Invalid Parameter).
     *
     * A value of X'FF' is valid and specifies that sector positioning
     * is not to be performed prior to establishing orientation.
     */
    if (1
        && validate
        && iobuf[13] != 0xFF
        && iobuf[13] >= dev->ckdtab->sectors
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * When byte 1, bit 0 is '0', bytes 14-15 must contain zeros; if
     * bytes 14-15 are not zero, Locate Record Extended is terminated
     * with status that includes unit check (Command Reject, format
     * X'04', Invalid Parameter).
     *
     * When byte 1 bit 0 is '1', bytes 14-15 contain a Transfer Length
     * Factor (TLF). The Transfer Length Factor must be non-zero; if it
     * is zero, Locate Record Extended is terminated with status that
     * includes unit check (Command Reject, format X'04', Invalid
     * Parameter).
     *
     * If the Transfer Length Factor value is greater than the value
     * specified (or implied) in the Define Extent Blocksize parameter,
     * Locate Record Extended is terminated with status that includes
     * unit check (Command Reject, format X'04', Invalid Parameter).
     *
     * The Storage Director uses the TLF to determine the number of
     * data bytes to be requested from the channel for each write
     * command that follows a Locate Record Extended that specified the
     * Write Data (01) Operation code. The product of the value in
     * bytes 14-15 and the count parameter is used to determine the
     * total number of bytes to be transferred by data transfer commands
     * that are executed within the domain of a Locate Record Extended
     * that specified the Format Write (03), Write Track (0B), or
     * Read (16/06) Operation codes.
     *
     * The TLF value is not retained by the Storage Director after the
     * expiration of the Locate Record domain.
     *
     * If Locate Record Extended does not specify a Transfer Length
     * Factor, the Storage Director will use the value from the Define
     * Extent Blocksize parameter for any required data transfer length
     * calculation.
     */
    if (validate)
    {
        if (0
            || (!(dev->ckdlaux & CKDLAUX_TLFVALID) &&  fetch_hw( &iobuf[14] ))
            || ( (dev->ckdlaux & CKDLAUX_TLFVALID) && !fetch_hw( &iobuf[14] ))
            || fetch_hw( &iobuf[14] ) > dev->ckdxblksz
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }
    }

    if ((dev->ckdlaux & CKDLAUX_TLFVALID) == 0)
        dev->ckdltranlf = dev->ckdxblksz;
    else
        dev->ckdltranlf = fetch_hw( &iobuf[14] );

    /*
     * Bytes 18-19 contain an unsigned 16-bit binary value that
     * specifies the total number of extended parameter bytes. The
     * format and content of the Extended Parameters are defined by
     * the Extended Operation code.
     *
     * The length for 3990 Mod 6 or 9390 for the Extended Operation
     * codes must be consistent with the Extended Operation code in
     * byte 17 as follows:
     *   09  0001
     *   0A  0001
     *   0E  0001 or 0002
     *
     * If the operation code is any code other than those defined, the
     * extended parameter length count must be zero. If these conditions
     * are not met the Locate Record Extended is terminated with status
     * that includes unit check (Command Reject, format X'04', Invalid
     * Parameter).
     */
    num = fetch_hw( &iobuf[18] );

    if (validate)
    {
        if (0
            || (1
                && iobuf[17] != CKDOPER_WRTANY
                && iobuf[17] != CKDOPER_RDANY
                && iobuf[17] != CKDOPER_RDTSET
                && num
               )
            || (iobuf[17] == CKDOPER_WRTANY && num != 1)
            || (iobuf[17] == CKDOPER_RDANY  && num != 1)
            || (iobuf[17] == CKDOPER_RDTSET && num != 1 && num != 2)
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }
    }

    /*
     * Request the extended parameter bytes from the channel. If the
     * channel provides fewer bytes, execution is terminated with status
     * that includes unit check (Command Reject, format X'03', CCW byte
     * count less than required).
     */
    if (count < 20 + num)
    {
        *residual = 0;
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    *residual -= num;

    if (num > 0)
        if (iobuf[20] & 0x01)
            dev->ckdlaux |= CKDLAUX_RDCNTSUF;

    /* Reset/initialize our saved first Read Count */
    dev->ckdfcoun = 0;
    memset( dev->ckdfcwrk, 0x00, sizeof( dev->ckdfcwrk ));

    /* Save the Extended Operation byte */
    dev->ckdextcd = iobuf[17];

    /*
     * For Read Any (0x0A) or Write Any (0x09) the extended length
     * must be one and the extended parameter value (set size) must
     * be one.  Otherwise the Locate Record Extended command is
     * terminated with status that includes unit check (Command
     * Reject, format X'04', Invalid Parameter).
     */
    if (0
        || (dev->ckdextcd == CKDOPER_WRTANY && iobuf[20] != 1)
        || (dev->ckdextcd == CKDOPER_RDANY  && iobuf[20] != 1)
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /*
     * Read Trackset - X'0E': The Read Trackset Operation Code prepares
     * the Storage Director to transfer all records from one or more
     * tracks to the channel. The tracks to be transferred are
     * specified by the Extended Parameter and the number of tracks to
     * be transferred is specified by the Count Parameter (byte 3).
     *
     * The maximum length of the Extended Parameter is specified in byte
     * 43 of the Device Characteristics Information.
     *
     * The Extended Parameter contains a bit map that represents a set
     * of sequentially addressed tracks within the defined extent. Each
     * bit in the parameter represent one track. A '1' bit indicates the
     * data associated with the corresponding track is to be read. A '0'
     * bit indicates the track is to be skipped.
     *
     * The first bit must be a '1' and represents the track whose
     * address is specified in the Seek Address parameter (bytes 4-7).
     * Subsequent bits represent consecutively addressed tracks in
     * ascending order. If the first bit is not a '1', the Locate Record
     * Extended command is terminated with status that includes unit
     * check (Command Reject, format X'04', Invalid Parameter).
     *
     * The number of '1' bits in the bit map must be equal to the value
     * in the count parameter (byte 3); otherwise Locate Record Extended
     * is terminated with status that includes unit check (Command
     * Reject, format X'04', Invalid Parameter).
     *
     * All tracks in the bit map represented by the '1' bits must be
     * contained within the defined extent; otherwise the Locate Record
     * Extended command is terminated with status that includes unit
     * check (File Protected).
     *
     * Track access is initiated using the Seek Address and Sector
     * Number parameters.
     *
     * When track access is completed, the search operation specified by
     * the Search Argument and the orientation modifiers (byte 0, bits
     * 0-1) is performed.
     *
     * Locate Record Extended must be followed by the number of Read
     * Track commands specified in the count parameter (byte 3). If any
     * other command sequence is detected within the Locate Record
     * domain, the non-conforming command will be rejected with status
     * that includes Unit Check (Command Reject, format X'02', Invalid
     * Command Sequence).
     */
    if (iobuf[17] == CKDOPER_RDTSET)
    {
        U16 lastcyl, lasthead;
        U16 mask = iobuf[20] << 8;

        if (num > 1)
            mask |= iobuf[21];

        if (validate && !(mask & 0x8000))
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        dev->ckdlmask = mask;

        /*
         * Count the one bits in mask.  There are elegant but obscure
         * ways to do this but just keeping it simple here.  Plus we
         * also figure out the last track we will read.
         */
        for (i=j=0; mask; mask <<= 1)
        {
            j++;

            if (mask & 0x8000)
                i++;
        }

        /* Number of one bits must match count */
        if (validate && i != dev->ckdlcount)
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }

        /* Check extent of last track to be read */
        lastcyl  = cyl;
        lasthead = (U16)(head + j - 1);

        while (lasthead >= dev->ckdheads)
        {
            lastcyl++;
            lasthead -= (U16) dev->ckdheads;
        }

        if (validate && EXTENT_CHECK( dev, lastcyl, lasthead ))
        {
            ckd_build_sense( dev, 0, SENSE1_FP, 0, 0, 0 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }
    }

    /* Seek to the required track */
    rc = ckd_seek( dev, cyl, head, &trkhdr, unitstat );
    if (rc < 0)
        return false;

    /* Set normal status */
    *unitstat = CSW_CE | CSW_DE;

    /* Perform search according to specified orientation */
    switch (dev->ckdloper & CKDOPER_ORIENTATION)
    {
        case CKDOPER_ORIENT_HOME:

            /* For home orientation, compare the search CCHH
               with the CCHH in the track header */
            if (memcmp( &(trkhdr.cyl), cchhr, 4 ) != 0)
            {
                ckd_build_sense( dev, 0, SENSE1_NRF, 0, 0, 0 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                return false;
            }
            break;

        case CKDOPER_ORIENT_COUNT:
        case CKDOPER_ORIENT_DATA:

            /* For count or data orientation, search the track
               for a count field matching the specified CCHHR */
            memset( binzero, 0x00, 5 );

            while (1)
            {
                /* Read next count field and exit at end of track
                   with sense data indicating no record found */
                rc = ckd_read_count( dev, code, &rechdr, unitstat );
                if (rc < 0)
                    return false; // (end of track)

                /* Turn off track overflow flag */
                if (dev->ckdcyls < 32768)
                    rechdr.cyl[0] &= 0x7F;

                /* For extended op code skip r0 */
                if ((iobuf[0] & CKDOPER_CODE) == CKDOPER_EXTOP)
                {
                    if (rechdr.rec != 0)
                        break;
                }

                if (memcmp( cchhr, binzero, 5 ) == 0)
                    break;

                /* Compare the count field with the search CCHHR */
                else if (memcmp( &rechdr, cchhr, 5 ) == 0)
                    break; // (we found it!)

            } /* end while */
    } /* end switch(CKDOPER_ORIENTATION) */

    /* Exit if search ended with error status */
    if (*unitstat != (CSW_CE | CSW_DE))
        return false;

    /* Reorient past data if data orientation is specified */
    if ((dev->ckdloper & CKDOPER_ORIENTATION) == CKDOPER_ORIENT_DATA)
    {
        /* Skip past key and data fields */
        dev->bufoff += dev->ckdcurkl + dev->ckdcurdl;

        /* Set the device orientation fields */
        dev->ckdrem = 0;
        dev->ckdorient = CKDORIENT_DATA;
    }

    /* Set locate record flag and return normal status */
    dev->ckdlocat = 1;
    *unitstat = CSW_CE | CSW_DE;
    return true;

} /* end function LocateRecordExtended */

#if defined( GCC_VERSION ) && GCC_VERSION >= 110000 /* gcc >= 11.0.0 */
PUSH_GCC_WARNINGS()
DISABLE_GCC_WARNING( "-Wmaybe-uninitialized" )
#endif

/*-------------------------------------------------------------------*/
/* DEFINE EXTENT    helper function                                  */
/*-------------------------------------------------------------------*/
static bool DefineExtent
(
    DEVBLK*  dev,
    BYTE     code,
    BYTE     flags,
    BYTE     chained,
    U32      count,
    BYTE     prevcode,
    int      ccwseq,
    BYTE*    iobuf,
    BYTE*    more,
    BYTE*    unitstat,
    U32*     residual
)
{
    BYTE* orig_e7_iobuf;
    bool  validate = true;
    U32   num;
    U16   bcyl, bhead, ecyl, ehead, xblksz;
    BYTE  fmask, xgattr;
    BYTE  ccwdata[ IOBUF_MINSIZE ];

    /* If Prefix CCW, move the iobuff data to where this function
       expects it */
    if (code == 0xE7)
    {
        orig_e7_iobuf = iobuf;
        memmove( ccwdata, &iobuf[12], sizeof( ccwdata ) - 12 );
        count -= (count >= 12 ? 12 : count);
        iobuf = &ccwdata[0];

        /* Don't do ANYTHING if no Define Extent was provided */
        if (!(dev->ckdvalid & PFX_V_DX_VALID))
        {
            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
            return true;
        }

        /* Don't validate ANY Define Extent fields if asked not to */
        if (dev->ckdauxiliary & PFX_A_VALID)  // ("trust me"?)
            validate = false;                 // (whatever you say!)
    }

    UNREFERENCED( flags    );
    UNREFERENCED( chained  );
    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq   );
    UNREFERENCED( more     );

    /* Calculate residual byte count */
    num = (count < 16) ? count : 16;
    *residual = count - num;

    /* Control information length must be at least 16 bytes */
    if (count < 16)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_3 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Command reject if within the domain of a Locate Record, or
       preceded by Define Extent, Space Count, or Set File Mask,
       or (for 3390 only) preceded by Read IPL */
    if (dev->ckdlcount > 0
#if 0
        || dev->ckdxtdef
#endif
        || dev->ckdsetfm
        || dev->ckdspcnt
        || (dev->ckdrdipl && dev->devtype == 0x3390)
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Bytes 0-1 contain the file mask and global attributes */
    fmask = iobuf[0];
    xgattr = iobuf[1];

    if (1
        && validate
        && dev->ckdxtdef
        && (dev->ckdfmask != fmask || dev->ckdxgattr != xgattr)
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    dev->ckdfmask = fmask;
    dev->ckdxgattr = xgattr;

    /* Validate the global attributes byte bits 0-1 */
    if (validate && (dev->ckdxgattr & CKDGATR_ARCH) != CKDGATR_ARCH_ECKD)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Validate the file mask */
    if (validate && (dev->ckdfmask & CKDMASK_RESV) != 0)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Bytes 2-3 contain the extent block size */
    xblksz = (iobuf[2] << 8) | iobuf[3];

    /* If extent block size is zero then use the maximum R0
       record length (as returned in device characteristics
       bytes 44 and 45) plus 8 */
    if (xblksz == 0)
        xblksz = dev->ckdtab->r0 + 8;

    if (1
        && validate
        && dev->ckdxtdef
        && dev->ckdxblksz != xblksz
    )
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_2 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    dev->ckdxblksz = xblksz;

    /* Validate the extent block */
    if (validate && dev->ckdxblksz > dev->ckdtab->r0 + 8)
    {
        ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return false;
    }

    /* Bytes 4-6 must contain zeroes */
    if (validate)
    {
        if (0
            || iobuf[4] != 0
            || iobuf[5] != 0
            || iobuf[6] != 0
        )
        {
            ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, MESSAGE_4 );
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            return false;
        }
    }

    /* Bytes 8-11 contain the extent begin cylinder and head */
    bcyl  = (iobuf[8]  << 8) | iobuf[9];
    bhead = (iobuf[10] << 8) | iobuf[11];

    /* Bytes 12-15 contain the extent end cylinder and head */
    ecyl  = (iobuf[12] << 8) | iobuf[13];
    ehead = (iobuf[14] << 8) | iobuf[15];

    if (validate)
    {
        /* Validate the extent description by checking that the
           ending track is not less than the starting track and
           the extent does not exceed the already defined extent.

           Exception: If an upcoming LRE specifies either a Read
           Any or Write Any extended operation then only validate
           the Beginning of Extent, as the End of Extent will not
           ever be used in such a situation.
        */

#define E7_PFX_LEN          12
#define E7_DX_LEN           32
#define E7_LRE_IOBUF_LOC    (E7_PFX_LEN+E7_DX_LEN)
#define LRE(n)              (E7_LRE_IOBUF_LOC+n)

        if (1
            && code == 0xE7 // PREFIX
            && dev->ckdformat == PFX_F_DX_LRE
            && (orig_e7_iobuf[LRE(0)] & CKDOPER_CODE) == CKDOPER_EXTOP
            && (0
                || (orig_e7_iobuf[LRE(17)] & CKDOPER_CODE) == CKDOPER_RDANY
                || (orig_e7_iobuf[LRE(17)] & CKDOPER_CODE) == CKDOPER_WRTANY
               )
        )
        {
            /* Upcoming LRE Read Any or Write Any: only validate the
               Beginning of Extent value, as the End of Extent value
               will never be used.
            */
            if (EXTENT_BEGIN_CHECK( dev, bcyl, bhead ))
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, dev->ckdxtdef ? MESSAGE_2 : MESSAGE_4 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                return false;
            }
        }
        else
        {
            /* Normal case: validate BOTH the Beginning of Extent
               *and* End of Extent values.
            */
            if (0
                || bcyl > ecyl
                || (bcyl == ecyl && bhead > ehead)
                || EXTENT_CHECK( dev, bcyl, bhead )
                || EXTENT_CHECK( dev, ecyl, ehead )
            )
            {
                ckd_build_sense( dev, SENSE_CR, 0, 0, FORMAT_0, dev->ckdxtdef ? MESSAGE_2 : MESSAGE_4 );
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                return false;
            }
        }
    }

    /* Define the new extent */
    dev->ckdxbcyl  = bcyl;
    dev->ckdxbhead = bhead;
    dev->ckdxecyl  = ecyl;
    dev->ckdxehead = ehead;

    /* Set extent defined flag and return normal status */
    dev->ckdxtdef = 1;

    *unitstat = CSW_CE | CSW_DE;
    return true;

} /* end function DefineExtent */

#if defined( GCC_VERSION ) && GCC_VERSION >= 110000 /* gcc >= 11.0.0 */
POP_GCC_WARNINGS()
#endif

DLL_EXPORT  DEVHND  ckd_dasd_device_hndinfo =
{
    &ckd_dasd_init_handler,           /* Device Initialization      */
    &ckd_dasd_execute_ccw,            /* Device CCW execute         */
    &ckd_dasd_close_device,           /* Device Close               */
    &ckd_dasd_query_device,           /* Device Query               */
    NULL,                             /* Device Extended Query      */
    &ckd_dasd_start,                  /* Device Start channel pgm   */
    &ckd_dasd_end,                    /* Device End channel pgm     */
    &ckd_dasd_start,                  /* Device Resume channel pgm  */
    &ckd_dasd_end,                    /* Device Suspend channel pgm */
    NULL,                             /* Device Halt channel pgm    */
    &ckd_dasd_read_track,             /* Device Read                */
    &ckd_dasd_update_track,           /* Device Write               */
    &ckd_dasd_used,                   /* Device Query used          */
    NULL,                             /* Device Reserve             */
    NULL,                             /* Device Release             */
    NULL,                             /* Device Attention           */
    NULL,                             /* Immediate CCW Codes        */
    NULL,                             /* Signal Adapter Input       */
    NULL,                             /* Signal Adapter Ouput       */
    NULL,                             /* Signal Adapter Sync        */
    NULL,                             /* Signal Adapter Output Mult */
    NULL,                             /* QDIO subsys desc           */
    NULL,                             /* QDIO set subchan ind       */
    &ckd_dasd_hsuspend,               /* Hercules suspend           */
    &ckd_dasd_hresume                 /* Hercules resume            */
};
