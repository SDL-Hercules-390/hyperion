/* DASDUTIL64.C (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules DASD Utilities: Common subroutines          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains common subroutines used by DASD utilities    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DASDUTIL64_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "dasdblks.h"
#include "devtype.h"
#include "opcode.h"
#include "ccwarn.h"

/*-------------------------------------------------------------------*/
/* Subroutine to open a CKD image file                               */
/* Input:                                                            */
/*      fname    CKD image file name                                 */
/*      sfname   Shadow-File option string  (e.g. "sf=shadow_*.xxx") */
/*      omode    Open mode: O_RDONLY or O_RDWR                       */
/*      option   IMAGE_OPEN_NORMAL, IMAGE_OPEN_DASDCOPY, etc.        */
/*                                                                   */
/* The CKD image file is opened, a track buffer is obtained,         */
/* and a CKD image file descriptor structure is built.               */
/* Return value is a pointer to the CKD image file descriptor        */
/* structure if successful, or NULL if unsuccessful.                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT CIFBLK* open_ckd64_image (char *fname, char *sfname, int omode,
                       int option)
{
int             fd;                     /* File descriptor           */
int             rc;                     /* Return code               */
int             iLen;                   /* Record length             */
CKD_DEVHDR      devhdr;                 /* CKD device header         */
CIFBLK         *cif;                    /* CKD image file descriptor */
DEVBLK         *dev;                    /* CKD device block          */
CKDDEV         *ckd;                    /* CKD DASD table entry      */
char           *rmtdev;                 /* Possible remote device    */
char           *argv[2];                /* Arguments to              */
int             argc=0;                 /*                           */
char            sfxname[FILENAME_MAX*2];/* Suffixed file name        */
char            typname[64];
char            pathname[MAX_PATH];     /* file path in host format  */

    /* Obtain storage for the file descriptor structure */
    cif = (CIFBLK*) calloc (1,sizeof(CIFBLK));
    if (cif == NULL)
    {
        char buf[40];
        MSGBUF(buf, "calloc(1,%d)", (int)sizeof(CIFBLK));
        FWRMSG( stderr, HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid),
            cif->devblk.devnum, fname, buf, strerror( errno ));
        return NULL;
    }

    /* Initialize the devblk */
    dev = &cif->devblk;
    dev->cckd64 = 1;
    if ((omode & O_RDWR) == 0) dev->ckdrdonly = 1;
    dev->fd = -1;
    dev->batch = 1;
    dev->dasdcopy  = (option & IMAGE_OPEN_DASDCOPY) ? 1 : 0;
    dev->quiet     = (option & IMAGE_OPEN_QUIET)    ? 1 : 0;

    /* If the filename has a `:' then it may be a remote device */
    if ((rmtdev = strchr(fname, ':')))
    {
        /* Verify port number follows colon */
        char *p;
        for (p = rmtdev + 1; *p && *p != ':'; p++)
        {
            if (!isdigit((unsigned char)*p))  /* (port numbers are always numeric) */
            {
                /* Not a port number ==> not really a remote device */
                rmtdev = NULL;
                break;
            }
        }
    }

    /* Read the device header so we can determine the device type */
    STRLCPY( sfxname, fname );
    hostpath(pathname, sfxname, sizeof(pathname));
    fd = HOPEN (pathname, omode);
    if (fd < 0)
    {
        /* If no shadow file name was specified, then try opening the
           file with the file sequence number in the name */
        if (sfname == NULL)
        {
            ptrdiff_t i;
            char *s,*suffix;

            /* Look for last slash marking end of directory name */
            s = strrchr (fname, PATHSEPC);
            if (s == NULL) s = fname;

            /* Insert suffix before first dot in file name, or
               append suffix to file name if there is no dot.
               If the filename already has a place for the suffix
               then use that. */
            s = strchr (s, '.');
            if (s != NULL)
            {
                i = s - fname;
                if (i > 2 && fname[i-2] == '_')
                    suffix = sfxname + i - 1;
                else
                {
                    strlcpy( sfxname + i, "_1", sizeof(sfxname)-(size_t)i );
                    STRLCAT( sfxname, fname + i );
                    suffix = sfxname + i + 1;
                }
            }
            else
            {
                if (strlen(sfxname) < 2 || sfxname[strlen(sfxname)-2] != '_')
                    STRLCAT (sfxname, "_1" );
                suffix = sfxname + strlen(sfxname) - 1;
            }
            *suffix = '1';
            hostpath(pathname, sfxname, sizeof(pathname));
            fd = HOPEN (pathname, omode);
        }
        if (fd < 0 && rmtdev == NULL)
        {
            FWRMSG( stderr, HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid),
                cif->devblk.devnum, cif->fname, "open()", strerror( errno ));
            free (cif);
            return NULL;
        }
        else if (fd < 0) STRLCPY( sfxname, fname );
    }

    /* If not a possible remote device, check the dasd header
       and set the device type */
    if (fd >= 0)
    {
        iLen = read(fd, &devhdr, CKD_DEVHDR_SIZE);
        if (iLen < 0)
        {
            FWRMSG( stderr, HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid),
                cif->devblk.devnum, cif->fname, "read()", strerror( errno ));
            close (fd);
            free (cif);
            return NULL;
        }
        close (fd);

        /* Error if no device header or not CKD non-shadow type */
        if (0
            || iLen < CKD_DEVHDR_SIZE
            || !(dh_devid_typ( devhdr.dh_devid ) & CKD64_CMP_OR_NML_TYP)
        )
        {
            if (dh_devid_typ( devhdr.dh_devid ) & CKD32_CMP_OR_NML_TYP)
            {
                dev->cckd64 = 0;
                free( cif );
                return open_ckd_image( fname, sfname, omode, option );
            }

            // "%1d:%04X CKD file %s: ckd header invalid"
            FWRMSG( stderr, HHC00406, "E", SSID_TO_LCSS( cif->devblk.ssid ),
                cif->devblk.devnum, cif->fname );
            free( cif );
            return NULL;
        }

        /* Set the device type */
        ckd = dasd_lookup (DASD_CKDDEV, NULL, devhdr.dh_devtyp, 0);
        if (ckd == NULL)
        {
            // "%1d:%04X CKD file %s: DASD table entry not found for devtype 0x%2.2X"
            FWRMSG( stderr, HHC00451, "E", SSID_TO_LCSS(cif->devblk.ssid),
                cif->devblk.devnum, cif->fname, devhdr.dh_devtyp );
            free (cif);
            return NULL;
        }
        dev->devtype = ckd->devt;
        MSGBUF(typname, "%4.4X", dev->devtype);
        dev->typname=typname;   /* Makes HDL Happy */
    }

    /* Set the device handlers */
    dev->hnd = &ckd_dasd_device_hndinfo;

    /* Set the device number */
    dev->devnum = next_util_devnum();

    /* Build arguments for ckd_dasd_init_handler */
    argv[0] = sfxname;
    argc++;
    if (sfname != NULL)
    {
        argv[1] = sfname;
        argc++;
    }

    /* Call the device handler initialization function */
    rc = (dev->hnd->init)(dev, argc, argv);
    if (rc < 0)
    {
        // "%1d:%04X CKD file %s: initialization failed"
        FWRMSG( stderr, HHC00452, "E", SSID_TO_LCSS(cif->devblk.ssid),
            cif->devblk.devnum, cif->fname ? cif->fname : "(null)" );
        free (cif);
        return NULL;
    }

    /* Call the device start exit */
    if (dev->hnd->start) (dev->hnd->start) (dev);

    /* Set CIF fields */
    cif->fname = fname;
    cif->fd = dev->fd;

    /* Extract the number of heads and the track size */
    cif->heads = dev->ckdheads;
    FETCH_LE_FW( cif->trksz, devhdr.dh_trksize );

    if (is_verbose_util())
    {
       FWRMSG( stdout, HHC00453, "I", SSID_TO_LCSS(cif->devblk.ssid),
           cif->devblk.devnum, cif->fname, cif->heads, cif->trksz );
    }

    /* Indicate that the track buffer is empty */
    cif->curcyl = -1;
    cif->curhead = -1;
    cif->trkmodif = 0;

    return cif;
} /* end function open_ckd64_image */

/*-------------------------------------------------------------------*/
/* Subroutine to open a FBA image file                               */
/* Input:                                                            */
/*      fname    FBA image file name                                 */
/*      sfname   Shadow-File option string  (e.g. "sf=shadow_*.xxx") */
/*      omode    Open mode: O_RDONLY or O_RDWR                       */
/*      option   IMAGE_OPEN_NORMAL, IMAGE_OPEN_DASDCOPY, etc.        */
/*                                                                   */
/* The FBA image file is opened, a track buffer is obtained,         */
/* and a FBA image file descriptor structure is built.               */
/* Return value is a pointer to the FBA image file descriptor        */
/* structure if successful, or NULL if unsuccessful.                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT CIFBLK* open_fba64_image (char *fname, char *sfname, int omode,
                        int option)
{
int             rc;                     /* Return code               */
CIFBLK         *cif;                    /* FBA image file descriptor */
DEVBLK         *dev;                    /* FBA device block          */
FBADEV         *fba;                    /* FBA DASD table entry      */
char           *argv[2];                /* Arguments to              */
int             argc=0;                 /*  device open              */

    /* Obtain storage for the file descriptor structure */
    cif = (CIFBLK*) calloc( 1, sizeof(CIFBLK) );
    if (cif == NULL)
    {
        char buf[40];
        MSGBUF(buf, "calloc(1,%d)", (int)sizeof(CIFBLK));
        FWRMSG( stderr, HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid),
            cif->devblk.devnum, fname, buf, strerror( errno ));
        return NULL;
    }

    /* Initialize the devblk */
    dev = &cif->devblk;
    dev->fd = -1;
    dev->cckd64 = 1;
    if ((omode & O_RDWR) == 0) dev->ckdrdonly = 1;
    dev->batch = 1;
    dev->dasdcopy  = (option & IMAGE_OPEN_DASDCOPY) ? 1 : 0;
    dev->quiet     = (option & IMAGE_OPEN_QUIET)    ? 1 : 0;

    /* Set the device type */
    fba = dasd_lookup (DASD_FBADEV, NULL, DEFAULT_FBA_TYPE, 0);
    if (fba == NULL)
    {
        FWRMSG( stderr, HHC00451, "E", SSID_TO_LCSS(cif->devblk.ssid),
            cif->devblk.devnum, fname, DEFAULT_FBA_TYPE );
        free (cif);
        return NULL;
    }
    dev->devtype = fba->devt;

    /* Set the device handlers */
    dev->hnd = &fba_dasd_device_hndinfo;

    /* Set the device number */
    dev->devnum = next_util_devnum();

    /* Build arguments for fba_dasd_init_handler */
    argv[0] = fname;
    argc++;
    if (sfname != NULL)
    {
        argv[1] = sfname;
        argc++;
    }

    /* Call the device handler initialization function */
    rc = (dev->hnd->init)(dev, argc, argv);
    if (rc < 0)
    {
        // "%1d:%04X CKD file %s: initialization failed"
        FWRMSG( stderr, HHC00452, "E", SSID_TO_LCSS(cif->devblk.ssid),
            cif->devblk.devnum, fname ? fname : "(null)" );
        free (cif);
        return NULL;
    }

    /* Set CIF fields */
    cif->fname = fname;
    cif->fd = dev->fd;

    /* Extract the number of sectors and the sector size */
    cif->heads = dev->fbanumblk;
    cif->trksz = dev->fbablksiz;
    if (is_verbose_util())
    {
       FWRMSG( stdout, HHC00454, "I", SSID_TO_LCSS(cif->devblk.ssid),
           cif->devblk.devnum, fname, cif->heads, cif->trksz );
    }

    /* Indicate that the track buffer is empty */
    cif->curcyl = -1;
    cif->curhead = -1;
    cif->trkmodif = 0;

    return cif;
} /* end function open_fba64_image */

/*-------------------------------------------------------------------*/
/* Subroutine to create a CKD DASD image file                        */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      fseqn    Sequence number of this file (1=first)              */
/*      devtype  Device type                                         */
/*      heads    Number of heads per cylinder                        */
/*      trksize  DADS image track length                             */
/*      buf      -> Track image buffer                               */
/*      start    Starting cylinder number for this file              */
/*      end      Ending cylinder number for this file                */
/*      volcyls  Total number of cylinders on volume                 */
/*      serial   Physical serial number                              */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*               Will be 0xff if device is not to be compressed.     */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      nullfmt  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip special track 0 handling)    */
/*      flagECmode        1  set EC mode bit in wait PSW             */
/*                        0  don't set EC mode bit in wait PSW       */
/*      flagMachinecheck  1  set machine-check-enabled flag          */
/*                           in wait PSW                             */
/*                        0  don't set machine-check-enabled flag    */
/*                           in wait PSW                             */
/*-------------------------------------------------------------------*/
static int
create_ckd64_file (char *fname, BYTE fseqn, U16 devtype, U32 heads,
                  U32 trksize, BYTE *buf, U32 start, U32 end,
                  U32 volcyls, const char* serial, char *volser, BYTE comp, BYTE dasdcopy,
                  BYTE nullfmt, BYTE rawflag,
                  BYTE flagECmode, BYTE flagMachinecheck)
{
int             rc;                     /* Return code               */
U64             rcoff;                  /* Return value from lseek() */
int             fd;                     /* File descriptor           */
int             i;                      /* Loop counter              */
int             n;                      /* Loop delimiter            */
CKD_DEVHDR      devhdr;                 /* Device header             */
CCKD64_DEVHDR   cdevhdr;                /* Compressed device header  */
CCKD64_L1ENT   *l1=NULL;                /* -> Primary lookup table   */
CCKD64_L2ENT    l2[256];                /* Secondary lookup table    */
CKD_TRKHDR     *trkhdr;                 /* -> Track header           */
CKD_RECHDR     *rechdr;                 /* -> Record header          */
U32             cyl;                    /* Cylinder number           */
U32             head;                   /* Head number               */
U32             trk = 0;                /* Track number              */
U32             trks;                   /* Total number tracks       */
BYTE            r;                      /* Record number             */
BYTE           *pos;                    /* -> Next position in buffer*/
U64             cpos = 0;               /* Offset into cckd file     */
u_int           len = 0;                /* Length used in track      */
U64             keylen;                 /* Length of keys            */
U64             ipl1len;                /* Length of IPL1 data       */
U64             ipl2len;                /* Length of IPL2 data       */
U64             vol1len;                /* Length of VOL1 data       */
BYTE            fileseq;                /* CKD header sequence number*/
U16             highcyl;                /* CKD header high cyl number*/
int             x=O_EXCL;               /* Open option               */
CKDDEV         *ckdtab;                 /* -> CKD table entry        */
char            pathname[MAX_PATH];     /* file path in host format  */

    keylen  = IPL1_KEYLEN;              /* (all are the same length) */
    ipl1len = IPL1_DATALEN;
    ipl2len = IPL2_DATALEN;
    vol1len = VOL1_DATALEN;

    /* Locate the CKD dasd table entry */
    ckdtab = dasd_lookup (DASD_CKDDEV, NULL, devtype, 0);
    if (ckdtab == NULL)
    {
        FWRMSG( stderr, HHC00415, "E", 0, 0, fname, devtype );
        return -1;
    }

    /* Set file sequence number to zero if this is the only file */
    if (fseqn == 1 && end + 1 == volcyls)
        fileseq = 0;
    else
        fileseq = fseqn;

    /* Set high cylinder number to zero if this is the last file */
    if (end + 1 == volcyls)
        highcyl = 0;
    else
        highcyl = (U16) end;
    cyl = end - start + 1;

    /* Special processing for ckd and dasdcopy */
    if (comp == 0xFF && dasdcopy)
    {
        highcyl = (U16) end;
        if (end + 1 == volcyls)
            fileseq = 0xff;
    }

    trks = volcyls * heads;

    /* if `dasdcopy' > 1 then we can replace the existing file */
    if (dasdcopy > 1) x = 0;

    /* Create the DASD image file */
    hostpath(pathname, fname, sizeof(pathname));
    fd = HOPEN (pathname, O_WRONLY | O_CREAT | x | O_BINARY,
                S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd < 0)
    {
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname, "open()", strerror( errno ));
        return -1;
    }

    /* Create the device header */
    memset( &devhdr, 0, CKD_DEVHDR_SIZE );
    memcpy( devhdr.dh_serial, serial, sizeof( devhdr.dh_serial ));

    if (comp == 0xff) memcpy( devhdr.dh_devid, dh_devid_str( CKD_P064_TYP ), 8 );
    else              memcpy( devhdr.dh_devid, dh_devid_str( CKD_C064_TYP ), 8 );

    STORE_LE_FW( devhdr.dh_heads,   heads   );
    STORE_LE_FW( devhdr.dh_trksize, trksize );

    devhdr.dh_devtyp  = devtype & 0xFF;
    devhdr.dh_fileseq = fileseq;

    STORE_LE_HW( devhdr.dh_highcyl, highcyl );

    /* Write the device header */
    rc = write (fd, &devhdr, CKD_DEVHDR_SIZE);
    if (rc < (int)CKD_DEVHDR_SIZE)
    {
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname, "write()",
                errno ? strerror( errno ) : "incomplete" );
        return -1;
    }

    /* Build a compressed CKD file */
    if (comp != 0xff)
    {
        /* Create the compressed device header */
        memset( &cdevhdr, 0, CCKD64_DEVHDR_SIZE );

        cdevhdr.cdh_vrm[0] = CCKD_VERSION;
        cdevhdr.cdh_vrm[1] = CCKD_RELEASE;
        cdevhdr.cdh_vrm[2] = CCKD_MODLVL;

        if (cckd_def_opt_bigend())
            cdevhdr.cdh_opts |= CCKD_OPT_BIGEND;

        cdevhdr.cdh_opts     |= CCKD_OPT_OPENRW;
        cdevhdr.num_L1tab = (volcyls * heads + 255) / 256;
        cdevhdr.num_L2tab = 256;

        STORE_LE_FW( cdevhdr.cdh_cyls, volcyls );

        cdevhdr.cmp_algo    = comp;
        cdevhdr.cmp_parm    = -1;
        cdevhdr.cdh_nullfmt = nullfmt;

        /* Write the compressed device header */
        rc = write (fd, &cdevhdr, CCKD64_DEVHDR_SIZE);
        if (rc < (int)            CCKD64_DEVHDR_SIZE)
        {
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                    "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }

        /* Create the primary lookup table */
        l1 = calloc (cdevhdr.num_L1tab, CCKD64_L1ENT_SIZE);
        if (l1 == NULL)
        {
            char buf[40];
            MSGBUF( buf, "calloc(%d,%d)", (int)cdevhdr.num_L1tab, (int)CCKD64_L1ENT_SIZE);
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname, buf, strerror( errno ));
            return -1;
        }
        l1[0] = CCKD64_L1TAB_POS + cdevhdr.num_L1tab * CCKD64_L1ENT_SIZE;

        /* Write the primary lookup table */
        rc = write (fd, l1, cdevhdr.num_L1tab * CCKD64_L1ENT_SIZE);
        if (rc < (int)     (cdevhdr.num_L1tab * CCKD64_L1ENT_SIZE))
        {
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                    "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }

        /* Create the secondary lookup table */
        memset (&l2, 0, CCKD64_L2TAB_SIZE);

        /* Write the seondary lookup table */
        rc = write (fd, &l2, CCKD64_L2TAB_SIZE);
        if (rc < (int)       CCKD64_L2TAB_SIZE)
        {
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                    "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }

        cpos = l1[0] + CCKD64_L2TAB_SIZE;
    }

    if (!dasdcopy)
    {
        /* Write each cylinder */
        for (cyl = start; cyl <= end; cyl++)
        {
            /* Display progress message every 10 cylinders */
            if (cyl && !(cyl % 10))
            {
                if (extgui)
                    fprintf( stderr, "CYL=%u\n", cyl );
                else
                    fprintf( stderr, "Writing cylinder %u\r", cyl );
            }

            for (head = 0; head < heads; head++)
            {
                /* Clear the track to zeroes */
                memset (buf, 0, trksize);

                /* Build the track header */
                trkhdr = (CKD_TRKHDR*) buf;
                trkhdr->bin = 0;
                store_hw( trkhdr->cyl,  (U16) cyl  );
                store_hw( trkhdr->head, (U16) head );
                pos = buf + CKD_TRKHDR_SIZE;

                /* Build record zero */
                r = 0;
                rechdr = (CKD_RECHDR*) pos;
                pos += CKD_RECHDR_SIZE;
                store_hw( rechdr->cyl,  (U16) cyl  );
                store_hw( rechdr->head, (U16) head );
                          rechdr->rec  = r;
                          rechdr->klen = 0;
                store_hw( rechdr->dlen, CKD_R0_DLEN );
                pos += CKD_R0_DLEN;
                r++;

                /* Track 0 contains IPL records and volume label */
                if (!rawflag && fseqn == 1 && trk == 0)
                {
                    /* Build the IPL1 record */
                    rechdr = (CKD_RECHDR*) pos;
                    pos += CKD_RECHDR_SIZE;

                    store_hw( rechdr->cyl,  (U16) cyl  );
                    store_hw( rechdr->head, (U16) head );
                              rechdr->rec  = r;
                              rechdr->klen = (BYTE) keylen;
                    store_hw( rechdr->dlen,  (U16) ipl1len );
                    r++;

                    memcpy( pos, IPL1_KEY, (size_t)keylen );
                    pos += keylen;

                    /* Copy model IPL PSW and CCWs */
                    memcpy( pos,                                            noiplpsw,  sizeof( noiplpsw  ));
                    memcpy( pos + sizeof( noiplpsw ),                       noiplccw1, sizeof( noiplccw1 ));
                    memcpy( pos + sizeof( noiplpsw ) + sizeof( noiplccw1 ), noiplccw2, sizeof( noiplccw2 ));

                    /* Set EC mode flag in wait PSW if requested */
                    if (flagECmode)
                        *(pos+1) = 0x08 | *(pos+1);

                    /* Set machine-check-enabled mask in PSW if requested */
                    if (flagMachinecheck)
                        *(pos+1) = 0x04 | *(pos+1);

                    pos += ipl1len;

                    /* Build the IPL2 record */
                    rechdr = (CKD_RECHDR*) pos;
                    pos += CKD_RECHDR_SIZE;
                    store_hw( rechdr->cyl,  (U16) cyl  );
                    store_hw( rechdr->head, (U16) head );
                              rechdr->rec  = r;
                              rechdr->klen = (BYTE) keylen;
                    store_hw( rechdr->dlen,  (U16) ipl2len );
                    r++;

                    memcpy( pos, IPL2_KEY, (size_t)keylen );
                    pos += keylen;
                    pos += ipl2len;

                    /* Build the VOL1 record */
                    rechdr = (CKD_RECHDR*) pos;
                    pos += CKD_RECHDR_SIZE;
                    store_hw( rechdr->cyl,  (U16) cyl  );
                    store_hw( rechdr->head, (U16) head );
                              rechdr->rec  = r;
                              rechdr->klen = (BYTE) keylen;
                    store_hw( rechdr->dlen,  (U16) vol1len );
                    r++;

                    memcpy( pos, VOL1_KEY, (size_t)keylen );
                    pos += keylen;

                    /* Build the VOL1 label */
                    build_vol1( pos, volser, NULL, true );
                    pos += vol1len;

                    /* 9 4096 data blocks for linux volume */
                    if (nullfmt == CKD_NULLTRK_FMT2)
                    {
                        for (i = 0; i < 9; i++)
                        {
                            rechdr = (CKD_RECHDR*)pos;
                            pos += CKD_RECHDR_SIZE;

                            store_hw( rechdr->cyl,  (U16) cyl  );
                            store_hw( rechdr->head, (U16) head );
                                      rechdr->rec  = r;
                                      rechdr->klen = 0;
                            store_hw( rechdr->dlen, CKD_NULL_FMT2_DLEN );
                            pos +=                  CKD_NULL_FMT2_DLEN;
                            r++;
                        }
                    }
                } /* end if(trk == 0) */

                /* Track 1 for linux contains an empty VTOC */
                else if (fseqn == 1 && trk == 1 && nullfmt == CKD_NULLTRK_FMT2)
                {
                    /* build format 4 dscb */
                    rechdr = (CKD_RECHDR*) pos;
                    pos += CKD_RECHDR_SIZE;

                    /* track 1 record 1 count */
                    store_hw( rechdr->cyl,  (U16) cyl  );
                    store_hw( rechdr->head, (U16) head );
                              rechdr->rec  = r;
                              rechdr->klen = 44;
                    store_hw( rechdr->dlen, 96 );
                    r++;

                    /* track 1 record 1 key */
                    memset (pos, 0x04, 44);
                    pos += 44;

                    /* track 1 record 1 data */
                    memset (pos, 0, 96);
                    pos[0] = 0xf4;                            // DS4IDFMT
                    store_hw(pos + 6, 10);                    // DS4DSREC
                    pos[14] = trks > 65535 ? 0xa0 : 0;        // DS4VTOCI
                    pos[15] = 1;                              // DS4NOEXT
                    store_hw(pos+18, (U16) volcyls);          // DS4DSCYL
                    store_hw(pos+20, (U16) heads);            // DS4DSTRK
                    store_hw(pos+22, ckdtab->len);            // DS4DEVTK
                    pos[27] = 0x30;                           // DS4DEVFG
                    pos[30] = 0x0c;                           // DS4DEVDT
                    pos[61] = 0x01;                           // DS4VTOCE + 00
                    pos[66] = 0x01;                           // DS4VTOCE + 05
                    pos[70] = 0x01;                           // DS4VTOCE + 09
                    pos[81] = trks > 65535 ? 7 : 0;           // DS4EFLVL
                    pos[85] = trks > 65535 ? 1 : 0;           // DS4EFPTR + 03
                    pos[86] = trks > 65535 ? 3 : 0;           // DS4EFPTR + 04
                    pos += 96;

                    /* build format 5 dscb */
                    rechdr = (CKD_RECHDR*)pos;
                    pos += CKD_RECHDR_SIZE;

                    /* track 1 record 1 count */
                    store_hw( rechdr->cyl,  (U16) cyl  );
                    store_hw( rechdr->head, (U16) head );
                              rechdr->rec  = r;
                              rechdr->klen = 44;
                    store_hw( rechdr->dlen, 96 );
                    r++;

                    /* track 1 record 2 key */
                    memset (pos, 0x05, 4);                    // DS5KEYID
                    memset (pos+4, 0, 40);
                    if (trks <= 65535)
                    {
                        store_hw(pos+4, 2);                   // DS5AVEXT + 00
                        store_hw(pos+6, (U16)(volcyls - 1));  // DS5AVEXT + 02
                        pos[8] = (BYTE)(heads - 2);           // DS5AVEXT + 04
                    }
                    pos += 44;

                    /* track 1 record 2 data */
                    memset (pos, 0, 96);
                    pos[0] = 0xf5;                            // DS5FMTID
                    pos += 96;

                    /* build format 7 dscb */
                    if (trks > 65535)
                    {
                        rechdr = (CKD_RECHDR*) pos;
                        pos += CKD_RECHDR_SIZE;

                        /* track 1 record 3 count */
                        store_hw( rechdr->cyl,  (U16) cyl  );
                        store_hw( rechdr->head, (U16) head );
                                  rechdr->rec  = r;
                                  rechdr->klen = 44;
                        store_hw( rechdr->dlen, 96 );
                        r++;

                        /* track 1 record 2 key */
                        memset (pos, 0x07, 4);                // DS7KEYID
                        memset (pos+4, 0, 40);
                        store_fw(pos+4, 2);                   // DS7EXTNT + 00
                        store_fw(pos+8, trks - 1);            // DS7EXTNT + 04
                        pos += 44;

                        /* track 1 record 2 data */
                        memset (pos, 0, 96);
                        pos[0] = 0xf7;                        // DS7FMTID
                        pos += 96;
                    }

                    n = 12 - r + 1;
                    for (i = 0; i < n; i++)
                    {
                        rechdr = (CKD_RECHDR*) pos;
                        pos += CKD_RECHDR_SIZE;
                        store_hw( rechdr->cyl,  (U16) cyl  );
                        store_hw( rechdr->head, (U16) head );
                                  rechdr->rec  = r;
                                  rechdr->klen = 44;
                        store_hw( rechdr->dlen, 96 );
                        pos += 140;
                        r++;
                    }
                }

                /* Specific null track formatting */
                else if (nullfmt == CKD_NULLTRK_FMT0)
                {
                    rechdr = (CKD_RECHDR*) pos;
                    pos += CKD_RECHDR_SIZE;
                    store_hw( rechdr->cyl,  (U16) cyl  );
                    store_hw( rechdr->head, (U16) head );
                              rechdr->rec  = r;
                              rechdr->klen = 0;
                    store_hw( rechdr->dlen, 0 );
                    r++;
                }
                else if (nullfmt == CKD_NULLTRK_FMT2)
                {
                    /* Other linux tracks have 12 4096 data records */
                    for (i = 0; i < 12; i++)
                    {
                        rechdr = (CKD_RECHDR*) pos;
                        pos += CKD_RECHDR_SIZE;
                        store_hw( rechdr->cyl,  (U16) cyl  );
                        store_hw( rechdr->head, (U16) head );
                                  rechdr->rec  = r;
                                  rechdr->klen = 0;
                        store_hw( rechdr->dlen, CKD_NULL_FMT2_DLEN );
                        pos +=                  CKD_NULL_FMT2_DLEN;
                        r++;
                    }
                }

                /* End-of-track marker */
                memcpy( pos, &CKD_ENDTRK, CKD_ENDTRK_SIZE );
                pos +=                    CKD_ENDTRK_SIZE;

                /* Calculate length to write */
                if (comp == 0xff)
                    len = (int)trksize;
                else
                {
                    len = (int)(pos - buf);
                    l2[trk].L2_trkoff = cpos;
                    l2[trk].L2_len    = l2[trk].L2_size  = (U16) len;
                    cpos += len;
                }

                /* Write the track to the file */
                rc = write (fd, buf, len);
                if (rc != (int)len)
                {
                    FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                            "write()", errno ? strerror( errno ) : "incomplete" );
                    return -1;
                }

                /* Exit if compressed disk and current track is 1 */
                if (comp != 0xff && trk == 1) break;

                trk++;

            } /* end for(head) */

            /* Exit if compressed disk */
            if (comp != 0xff) break;

        } /* end for(cyl) */

    } /* `dasdcopy' bit is off */
    else
        cyl = end + 1;

    /* Complete building the compressed file */
    if (comp != 0xff)
    {
        cdevhdr.cdh_size = cdevhdr.cdh_used = cpos;

        /* Rewrite the compressed device header */
        rcoff = lseek (fd, CKD_DEVHDR_SIZE, SEEK_SET);
        if ((S64)rcoff == -1)
        {
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                    "lseek()", strerror( errno ));
            return -1;
        }
        rc = write (fd, &cdevhdr, CCKD64_DEVHDR_SIZE);
        if (rc < (int)            CCKD64_DEVHDR_SIZE)
        {
          FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                  "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }

        /* Rewrite the secondary lookup table */
        rcoff = lseek (fd, l1[0], SEEK_SET);
        if ((S64)rcoff == -1)
        {
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                    "lseek()", strerror( errno ));
            return -1;
        }
        rc = write (fd, &l2, CCKD64_L2TAB_SIZE);
        if (rc < (int)       CCKD64_L2TAB_SIZE)
        {
          FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                  "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }
        rc = ftruncate(fd, cdevhdr.cdh_size);

        free (l1);
        cyl = volcyls;
    }

    /* Close the DASD image file */
    rc = close (fd);
    if (rc < 0)
    {
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "close()", strerror( errno ));
        return -1;
    }

    /* Display completion message */
    // "%1d:%04X CKD64 file %s: %u %s successfully written"
    FWRMSG( stdout, HHC00471, "I", 0, 0, fname,
            cyl - start, "cylinders" );
    return 0;

} /* end function create_ckd64_file */

/*-------------------------------------------------------------------*/
/* Subroutine to create a CKD DASD image                             */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      devtype  Device type                                         */
/*      heads    Number of heads per cylinder                        */
/*      maxdlen  Maximum R1 record data length                       */
/*      volcyls  Total number of cylinders on volume                 */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*               Will be 0xff if device is not to be compressed.     */
/*      lfs      build large (uncompressed) file (if supported)      */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      nullfmt  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip special track 0 handling)    */
/*      flagECmode        1  set EC mode bit in wait PSW             */
/*                        0  don't set EC mode bit in wait PSW       */
/*      flagMachinecheck  1  set machine-check-enabled flag          */
/*                           in wait PSW                             */
/*                        0  don't set machine-check-enabled flag    */
/*                           in wait PSW                             */
/*                                                                   */
/* If the total number of cylinders exceeds the capacity of a 2GB    */
/* file, then multiple CKD image files will be created, with the     */
/* suffix _1, _2, etc suffixed to the specified file name.           */
/* Otherwise a single file is created without a suffix.              */
/*-------------------------------------------------------------------*/
DLL_EXPORT int create_ckd64( char *fname, U16 devtype, U32 heads,
                             U32 maxdlen, U32 volcyls, char *volser,
                             BYTE comp, BYTE lfs, BYTE dasdcopy,
                             BYTE nullfmt, BYTE rawflag,
                             BYTE flagECmode, BYTE flagMachinecheck )
{
int             rc;                     /* Return code               */
char            *s;                     /* String pointer            */
BYTE            fileseq;                /* File sequence number      */
char            sfname[FILENAME_MAX];   /* Suffixed name of this file*/
char            *suffix;                /* -> Suffix character       */
U32             endcyl;                 /* Last cylinder of this file*/
U32             cyl;                    /* Cylinder number           */
U32             cylsize;                /* Cylinder size in bytes    */
BYTE           *buf;                    /* -> Track data buffer      */
U32             mincyls;                /* Minimum cylinder count    */
U32             maxcyls;                /* Maximum cylinder count    */
U32             maxcpif;                /* Maximum number of cylinders
                                           in each CKD image file    */
U32             trksize;                /* DASD image track length   */
char            serial[ sizeof_member( CKD_DEVHDR, dh_serial ) + 1 ] = {0};

    /* Compute the DASD image track length */
    trksize = CKD_TRKHDR_SIZE
            + CKD_RECHDR_SIZE + CKD_R0_DLEN
            + CKD_RECHDR_SIZE + maxdlen
            + CKD_ENDTRK_SIZE;
    trksize = ROUND_UP( trksize, 512 );

    /* Compute minimum and maximum number of cylinders */
    cylsize = trksize * heads;
    mincyls = 1;
    if (comp == 0xff && !lfs)
    {
        maxcpif = (0x7fffffff - CKD_DEVHDR_SIZE + 1) / cylsize;
        maxcyls = maxcpif * CKD_MAXFILES;
    }
    else
        maxcpif = maxcyls = volcyls;

    if (maxcyls > 65536)
    {
        maxcyls = 65536;
        FWRMSG( stderr, HHC00467, "W", "cylinders", maxcyls );
    }

    /* Check for valid number of cylinders */
    if (volcyls < mincyls || volcyls > maxcyls)
    {
        if (comp == 0xff && !lfs)
        {
            char    msgbuf[128];

#if defined( HAVE_ZLIB ) && defined( CCKD_BZIP2 )
            char   *pszcomp     = " or zlib/bzip2 compression";
#elif defined( HAVE_ZLIB )
            char   *pszcomp     = " or zlib compression";
#elif defined( CCKD_BZIP2 )
            char   *pszcomp     = " or bzip2 compression";
#else
            char   *pszcomp     = "";
#endif
            char   *pszopt;

            FWRMSG( stderr, HHC00466, "W", maxcyls, "cylinders", CKD_MAXFILES );

            if ( strlen(pszcomp) > 0 )
                pszopt = "related options";
            else
                pszopt = "option";

            MSGBUF(msgbuf, "-lfs%s %s", pszcomp, pszopt );
            FWRMSG( stderr, HHC00468, "I", msgbuf );
        }

        FWRMSG( stderr, HHC00461, "E", 0, 0, fname,
                "cylinder", volcyls, mincyls, maxcyls );
        return -1;
    }

    /* Obtain track data buffer */
    buf = malloc(trksize);
    if (buf == NULL)
    {
        char buf[40];
        MSGBUF( buf, "malloc(%u)", trksize);
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                buf, strerror( errno ));
        return -1;
    }

    /* Display progress message */
    // "%1d:%04X CKD64 file %s: creating %4.4X volume %s: %u cyls, %u trks/cyl, %u bytes/track"
    FWRMSG( stdout, HHC00472, "I", 0, 0, fname,
            devtype, rawflag ? "" : volser, volcyls, heads, trksize );

    /* Copy the unsuffixed DASD image file name */
    STRLCPY( sfname, fname );
    suffix = NULL;

    /* Create the suffixed file name if volume will exceed 2GB */
    if (volcyls > maxcpif)
    {
        /* Look for last slash marking end of directory name */
        s = strrchr( fname, PATHSEPC);
        if (s == NULL) s = fname;

        /* Insert suffix before first dot in file name, or
           append suffix to file name if there is no dot.
           If the filename already has a place for the suffix
           then use that. */
        s = strchr (s, '.');
        if (s != NULL)
        {
            ptrdiff_t i = (s - fname);
            if (i > 2 && fname[i-2] == '_')
                suffix = sfname + i - 1;
            else
            {
                strlcpy( sfname + i, "_1", sizeof(sfname)-(size_t)i );
                STRLCAT( sfname, fname + i );
                suffix = sfname + i + 1;
            }
        }
        else
        {
            if (strlen(sfname) < 2 || sfname[strlen(sfname)-2] == '_')
                STRLCAT( sfname, "_1" );
            suffix = sfname + strlen(sfname) - 1;
        }
    }

    /* Generate a random serial number for this new dasd */
    gen_dasd_serial( serial );

    /* Create the DASD image files */
    for (cyl = 0, fileseq = 1; cyl < volcyls;
            cyl += maxcpif, fileseq++)
    {
        /* Insert the file sequence number in the file name */
        if (suffix)
        {
            if ( fileseq <= 9 )
                *suffix = '0' + fileseq;
            else
                *suffix = 'A' - 10 + fileseq;
        }

        /* Calculate the ending cylinder for this file */
        if (cyl + maxcpif < volcyls)
            endcyl = cyl + maxcpif - 1;
        else
            endcyl = volcyls - 1;

        /* Create a CKD DASD image file */
        rc = create_ckd64_file (sfname, fileseq, devtype, heads,
                    trksize, buf, cyl, endcyl, volcyls, serial, volser,
                    comp, dasdcopy, nullfmt, rawflag,
                    flagECmode, flagMachinecheck);
        if (rc < 0) return -1;
    }

    /* Release data buffer */
    free (buf);

    return 0;
} /* end function create_ckd64 */

/*-------------------------------------------------------------------*/
/* Subroutine to create an FBA DASD image file                       */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      devtype  Device type                                         */
/*      sectsz   Sector size                                         */
/*      sectors  Number of sectors                                   */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*               Will be 0xff if device is not to be compressed.     */
/*      lfs      build large (uncompressed) file (if supported)      */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip sector 1 VOL1 processing)    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int create_fba64( char *fname, U16 devtype, U32 sectsz,
                             U32 sectors, char *volser, BYTE comp,
                             int lfs, int dasdcopy, int rawflag )
{
int             rc;                     /* Return code               */
int             fd;                     /* File descriptor           */
U32             sectnum;                /* Sector number             */
BYTE           *buf;                    /* -> Sector data buffer     */
U32             minsect;                /* Minimum sector count      */
U32             maxsect;                /* Maximum sector count      */
int             x=O_EXCL;               /* Open option               */
char            pathname[MAX_PATH];     /* file path in host format  */

    /* Special processing for compressed fba */
    if (comp != 0xff)
    {
        rc = create_compressed_fba64( fname, devtype, sectsz, sectors,
                                      volser, comp, lfs, dasdcopy, rawflag );
        return rc;
    }

    /* Compute minimum and maximum number of sectors */
    minsect = 64;
    maxsect = 0x80000000 / sectsz;

    /* Check for valid number of sectors */
    if (sectors < minsect || (!lfs && sectors > maxsect))
    {
        if (!lfs)
            FWRMSG( stderr, HHC00521, "W", maxsect, "sectors" );

        FWRMSG( stderr, HHC00461, "E", 0, 0, fname,
                "sector", sectors, minsect, maxsect );
        return -1;
    }

    /* Obtain sector data buffer */
    buf = malloc(sectsz);
    if (buf == NULL)
    {
        char buf[40];
        MSGBUF( buf, "malloc(%u)", sectsz);
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                buf, strerror( errno ));
        return -1;
    }

    /* Display progress message */
    // "%1d:%04X FBA64 file %s: creating %4.4X volume %s: %u sectors, %u bytes/sector"
    FWRMSG( stdout, HHC00473, "I", 0, 0, fname,
            devtype, rawflag ? "" : volser, sectors, sectsz );

    /* if `dasdcopy' > 1 then we can replace the existing file */
    if (dasdcopy > 1) x = 0;

    /* Create the DASD image file */
    hostpath(pathname, fname, sizeof(pathname));
    fd = HOPEN (pathname, O_WRONLY | O_CREAT | x | O_BINARY,
                S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd < 0)
    {
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "open()", strerror( errno ));
        return -1;
    }

    /* If the `dasdcopy' bit is on then simply allocate the space */
    if (dasdcopy)
    {
        U64 sz = (U64)((S64)sectors * sectsz);
        sz = ROUND_UP( sz, CFBA_BLKGRP_SIZE );
        // "This might take a while... Please wait..."
        FWRMSG( stdout, HHC00475, "I" );
        rc = ftruncate (fd, sz);
        if (rc < 0)
        {
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "ftruncate()", strerror( errno ));
            return -1;
        }
    }
    /* Write each sector */
    else
    {
        for (sectnum = 0; sectnum < sectors; sectnum++)
        {
            /* Clear the sector to zeroes */
            memset (buf, 0, sectsz);

            /* Sector 1 contains the volume label */
            if (!rawflag && sectnum == 1)
            {
                memcpy( buf, VOL1_KEY, sizeof( VOL1_KEY ));
                convert_to_ebcdic (buf+4, 6, volser);
            } /* end if(sectnum==1) */

            /* Display progress message every 100 sectors */
            if ((sectnum % 100) == 0)
            {
                if (extgui)
                    fprintf( stderr, "BLK=%u\n", sectnum );
                else
                    fprintf( stderr, "Writing sector %u\r", sectnum );
            }

            /* Write the sector to the file */
            rc = write (fd, buf, sectsz);
            if (rc < (int)sectsz)
            {
                FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                        "write()", errno ? strerror( errno ) : "incomplete" );
                return -1;
            }
        } /* end for(sectnum) */
    } /* `dasdcopy' bit is off */

    /* Close the DASD image file */
    rc = close (fd);
    if (rc < 0)
    {
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "close()", strerror( errno ));
        return -1;
    }

    /* Release data buffer */
    free (buf);

    /* Display completion message */
    // "%1d:%04X CKD64 file %s: %u %s successfully written"
    FWRMSG( stdout, HHC00471, "I", 0, 0, fname, sectors, "sectors" );

    return 0;
} /* end function create_fba64 */

/*-------------------------------------------------------------------*/
/* Subroutine to create a compressed FBA DASD image file             */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      devtype  Device type                                         */
/*      sectsz   Sector size                                         */
/*      sectors  Number of sectors                                   */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*      lfs      build large (uncompressed) file (if supported)      */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip sector 1 VOL1 processing)    */
/*-------------------------------------------------------------------*/
int create_compressed_fba64( char* fname, U16 devtype, U32 sectsz,
                             U32 sectors, char* volser, BYTE comp,
                             int lfs, int dasdcopy, int rawflag )
{
    int              rc;                /* Return code               */
    U64              rcoff;             /* Return value from lseek() */
    int              fd;                /* File descriptor           */
    CKD_DEVHDR       devhdr;            /* Device header             */
    CCKD64_DEVHDR    cdevhdr;           /* Compressed device header  */
    FBA_BKGHDR*      blkghdr;           /* Block Group Header        */
    int              blkgrps;           /* Number block groups       */
    int              num_L1tab, l1tabsz;/* Level 1 entries, size     */
    CCKD64_L1ENT*    l1;                /* Level 1 table pointer     */
    CCKD64_L2ENT     l2[256];           /* Level 2 table             */
    unsigned long    len2;              /* Compressed buffer length  */
#if defined( HAVE_ZLIB )
    BYTE             buf2[256];         /* Compressed buffer         */
#endif
    BYTE             buf[65536];        /* Buffer                    */
    int              x = O_EXCL;        /* Open option               */
    char             pathname[MAX_PATH];/* file path in host format  */

    UNREFERENCED( lfs );

    /* Calculate the size of the level 1 table */
    blkgrps   = (sectors / CFBA_BLKS_PER_GRP) + 1;
    num_L1tab = (blkgrps + 255) / 256;
    l1tabsz   = num_L1tab * CCKD64_L1ENT_SIZE;

    if (l1tabsz > 65536)
    {
        // "%1d:%04X CKD file %s: file size too large: %"PRIu64" [%d]"
        FWRMSG( stderr, HHC00464, "E", 0, 0, fname,
                ((U64)sectors * sectsz), num_L1tab );
        return -1;
    }

    /* if 'dasdcopy' > 1 then we can replace the existing file */
    if (dasdcopy > 1)
        x = 0;

    /* Create the DASD image file */
    hostpath( pathname, fname, sizeof( pathname ));
    fd = HOPEN( pathname,
        0
            | O_WRONLY
            | O_CREAT
            | x
            | O_BINARY
        ,
        0
            | S_IRUSR
            | S_IWUSR
            | S_IRGRP
    );

    if (fd < 0)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "open()", strerror( errno ));
        return -1;
    }

    /* Display progress message */
    // "%1d:%04X FBA64 file %s: creating %4.4X compressed volume %s: %u sectors, %u bytes/sector"
    FWRMSG( stdout, HHC00474, "I", 0, 0, fname,
        devtype, rawflag ? "" : volser, sectors, sectsz );

    /* Create the device header */
    memset( &devhdr, 0, CKD_DEVHDR_SIZE );
    gen_dasd_serial( devhdr.dh_serial );
    memcpy(  devhdr.dh_devid, dh_devid_str( FBA_C064_TYP ), 8 );

    STORE_LE_FW( devhdr.dh_heads,   sectors );
    STORE_LE_FW( devhdr.dh_trksize, sectsz  );

    devhdr.dh_devtyp  = devtype & 0xFF;
    devhdr.dh_fileseq = 0;

    STORE_LE_HW( devhdr.dh_highcyl, 0 );

    /* Write the device header */
    rc = write( fd, &devhdr, CKD_DEVHDR_SIZE );
    if (rc < (int) CKD_DEVHDR_SIZE)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "write()", errno ? strerror( errno ) : "incomplete" );
        return -1;
    }

    /* Build and Write the compressed device header */
    memset( &cdevhdr, 0, CCKD64_DEVHDR_SIZE );

    cdevhdr.cdh_vrm[0] = CCKD_VERSION;
    cdevhdr.cdh_vrm[1] = CCKD_RELEASE;
    cdevhdr.cdh_vrm[2] = CCKD_MODLVL;

    if (cckd_def_opt_bigend())
        cdevhdr.cdh_opts |= CCKD_OPT_BIGEND;

    cdevhdr.cdh_opts     |= CCKD_OPT_OPENRW;
    cdevhdr.num_L1tab = num_L1tab;
    cdevhdr.num_L2tab = 256;

    STORE_LE_FW( cdevhdr.cdh_cyls, sectors );

    cdevhdr.cmp_algo  = comp;
    cdevhdr.cmp_parm  = -1;

    rc = write( fd, &cdevhdr, CCKD64_DEVHDR_SIZE);
    if (rc < (int)            CCKD64_DEVHDR_SIZE)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "write()", errno ? strerror( errno ) : "incomplete" );
        return -1;
    }

    /* Build and Write the level 1 table */
    l1 = (CCKD64_L1ENT*) &buf;
    memset( l1, 0, l1tabsz );
    l1[0] = CKD_DEVHDR_SIZE + CCKD64_DEVHDR_SIZE + l1tabsz;
    rc = write( fd, l1, l1tabsz );
    if (rc < l1tabsz)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "write()", errno ? strerror( errno ) : "incomplete" );
        return -1;
    }

    /* Build and Write the 1st level 2 table */
    memset( &l2, 0, CCKD64_L2TAB_SIZE );
    l2[0].L2_trkoff = CKD_DEVHDR_SIZE + CCKD64_DEVHDR_SIZE
        + l1tabsz + CCKD64_L2TAB_SIZE;
    rc = write( fd, &l2, CCKD64_L2TAB_SIZE);
    if (rc < (int)       CCKD64_L2TAB_SIZE)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "write()", errno ? strerror( errno ) : "incomplete" );
        return -1;
    }

    /* Clear the first block group's image data to binary zeros */
    memset( &buf, 0, FBA_BKGHDR_SIZE + CFBA_BLKGRP_SIZE );

    /* Build the "Track Header" (FBA Block Group Header) */
    blkghdr = (FBA_BKGHDR*) &buf[0]; /* (--> block group header) */
    blkghdr->cmp = CCKD_COMPRESS_NONE;   /* (until we know for sure) */
    store_fw( blkghdr->blknum, 0 );      /* (group's starting block) */

    /* Build the VOL1 label if requested */
    if (!rawflag)
    {
        /* The VOL1 label is at physical sector number 1 */
        VOL1_FBA* fbavol1 = (VOL1_FBA*) &buf[ FBA_BKGHDR_SIZE + sectsz ];
        build_vol1( fbavol1, volser, NULL, false );
    }

    /* Write the 1st block group */
#if defined( HAVE_ZLIB )
    len2 = sizeof( buf2 );
    if (1
        && CCKD_COMPRESS_ZLIB == (comp & CCKD_COMPRESS_MASK)
        && Z_OK == (rc = compress2( &buf2[0], &len2, &buf[ FBA_BKGHDR_SIZE ],
                                    CFBA_BLKGRP_SIZE, Z_DEFAULT_COMPRESSION ))
    )
    {
        blkghdr->cmp = CCKD_COMPRESS_ZLIB;

        /* Write out the FBA Block Group Header separately (since it
           was NOT compressed) followed by the compressed block group
           data (which WAS compressed)
        */
        rc = write( fd, &buf, FBA_BKGHDR_SIZE );
        if (rc < (int) FBA_BKGHDR_SIZE)
        {
            // "%1d:%04X CKD file %s: error in function %s: %s"
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }

        /* Now write out the compressed block group data (the sectors) */
        rc = write( fd, &buf2, len2 );
        if (rc < (int) len2)
        {
            // "%1d:%04X CKD file %s: error in function %s: %s"
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }
    }
    else
#endif // defined( HAVE_ZLIB )
    {
        len2 = CFBA_BLKGRP_SIZE;
        blkghdr->cmp = CCKD_COMPRESS_NONE;

        /* Write out both the FBA Block Group Header and the Block Group
           Data itself (i.e. all of the block group sectors) in one I/O.
        */
        rc = write( fd, &buf, FBA_BKGHDR_SIZE + len2 );
        if (rc < (int)(FBA_BKGHDR_SIZE + len2))
        {
            // "%1d:%04X CKD file %s: error in function %s: %s"
            FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "write()", errno ? strerror( errno ) : "incomplete" );
            return -1;
        }
    }

    /* Update the L2 table entry for this block group */
    l2[0].L2_len = l2[0].L2_size = (U16)(FBA_BKGHDR_SIZE + len2);

    /* Update compressed device header too */
    cdevhdr.cdh_size = cdevhdr.cdh_used = CKD_DEVHDR_SIZE +
                   CCKD64_DEVHDR_SIZE + l1tabsz + CCKD64_L2TAB_SIZE +
                   FBA_BKGHDR_SIZE + len2;

    /* Re-write the compressed device header */
    if ((S64)(rcoff = lseek( fd, CKD_DEVHDR_SIZE, SEEK_SET )) < 0)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
                "lseek()", strerror( errno ));
        return -1;
    }
    rc = write( fd, &cdevhdr, CCKD64_DEVHDR_SIZE);
    if (rc < (int)            CCKD64_DEVHDR_SIZE)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "write()", errno ? strerror( errno ) : "incomplete" );
        return -1;
    }

    /* Re-write the 1st level 2 table */
    if ((S64)(rcoff = lseek( fd, CKD_DEVHDR_SIZE + CCKD64_DEVHDR_SIZE + l1tabsz, SEEK_SET )) < 0)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "lseek()", strerror( errno ));
        return -1;
    }
    rc = write( fd, &l2, CCKD64_L2TAB_SIZE);
    if (rc < (int)       CCKD64_L2TAB_SIZE)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "write()", errno ? strerror( errno ) : "incomplete" );
        return -1;
    }

    /* Close the DASD image file */
    if ((rc = close( fd )) < 0)
    {
        // "%1d:%04X CKD file %s: error in function %s: %s"
        FWRMSG( stderr, HHC00404, "E", 0, 0, fname,
            "close()", strerror( errno ));
        return -1;
    }

    /* Display completion message */
    // "%1d:%04X CKD64 file %s: %u %s successfully written"
    FWRMSG( stdout, HHC00471, "I", 0, 0, fname,
        sectors, "sectors" );
    return 0;
} /* end function create_compressed_fba64 */
