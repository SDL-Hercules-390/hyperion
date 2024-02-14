/* SHARED.C     (c)Copyright Greg Smith, 2002-2012                   */
/*              Shared Device Server                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

#define _SHARED_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "opcode.h"
#include "devtype.h"
#include "ccwarn.h"
#include "dasdblks.h"

DISABLE_GCC_UNUSED_SET_WARNING;

#if defined( OPTION_SHARED_DEVICES )

/*-------------------------------------------------------------------
 *                    Global variables
 *-------------------------------------------------------------------*/

DEVHND  shared_ckd_device_hndinfo;
DEVHND  shared_fba_device_hndinfo;

/*-------------------------------------------------------------------
 * Update notify - called by device handlers for sharable devices
 *-------------------------------------------------------------------*/
int shared_update_notify (DEVBLK *dev, int block)
{
int      i, j;                          /* Indexes                   */

    /* Return if no remotes are connected */
    if (dev->shrdconn == 0)
        return 0;

    for (i = 0; i < SHARED_MAX_SYS; i++)
    {

        /* Ignore the entry if it doesn't exist or if it's ours
           our if it's already maxed out */
        if (dev->shrd[i] == NULL || dev->shrd[i]->id == dev->shioactive
         || dev->shrd[i]->purgen < 0)
            continue;

        /* Check if the block is already entered */
        for (j = 0; j < dev->shrd[i]->purgen; j++)
            if (fetch_fw(dev->shrd[i]->purge[j]) == (U32)block) break;

        /* Add the block if it's not already there */
        if (j >= dev->shrd[i]->purgen)
        {
            if (dev->shrd[i]->purgen >= SHARED_PURGE_MAX)
                dev->shrd[i]->purgen = -1;
            else
                store_fw (dev->shrd[i]->purge[dev->shrd[i]->purgen++],
                          block);
           SHRDTRACE("notify %d added for id=%d, n=%d",
                   block, dev->shrd[i]->id, dev->shrd[i]->purgen);
        }

    } /* for each possible remote system */

    return 0;

} /* shared_update_notify */


/*-------------------------------------------------------------------
 * CKD init exit (client side)
 *-------------------------------------------------------------------*/
int shared_ckd_init (DEVBLK *dev, int argc, char *argv[] )
{
int      rc;                            /* Return code               */
int      i;                             /* Loop index                */
int      retry;                         /* 1=Connection being retried*/
char    *ipname;                        /* Remote name or address    */
char    *port = NULL;                   /* Remote port               */
char    *rmtnum = NULL;                 /* Remote device number      */
struct   hostent *he;                   /* -> hostent structure      */
char    *kw;                            /* Argument keyword          */
char    *op;                            /* Argument operand          */
BYTE     c;                             /* Used for parsing          */
char    *cu = NULL;                     /* Specified control unit    */
FWORD    cyls;                          /* Remote number cylinders   */
char    *p, buf[1024];                  /* Work buffer               */
char    *strtok_str = NULL;             /* last position             */

    /* Process the arguments */
    if (!(retry = dev->connecting))
    {
        dev->connected = 0;             /* SHRD_CONNECT not done yet */

        if (argc < 1 || strlen(argv[0]) >= sizeof(buf))
            return -1;
        STRLCPY( buf, argv[0] );

        /* First argument is 'ipname:port:devnum' */
        ipname = buf;
        if (strchr(ipname,'/') || strchr(ipname,'\\'))
            return -1;
        p = strchr (buf, ':');
        if (p)
        {
            *p = '\0';
            port = p + 1;
            p = strchr (port, ':');
        }
        if (p)
        {
            *p = '\0';
            rmtnum = p + 1;
        }

#if defined( HAVE_SYS_UN_H )
        if ( strcmp (ipname, "localhost") == 0)
            dev->localhost = 1;
        else
#endif
        {
            if ( (he = gethostbyname (ipname)) == NULL )
                return -1;
            memcpy(&dev->rmtaddr, he->h_addr_list[0], sizeof(dev->rmtaddr));
        }

        if (port && strlen(port))
        {
            if (sscanf(port, "%hu%c", &dev->rmtport, &c) != 1)
                return -1;
        }
        else
            dev->rmtport = SHARED_DEFAULT_PORT;

        if (rmtnum && strlen(rmtnum))
        {
            if (strlen (rmtnum) > 4
             || sscanf (rmtnum, "%hx%c", &dev->rmtnum, &c) != 1)
                return -1;
        }
        else
            dev->rmtnum = dev->devnum;

        /* Process the remaining arguments */
        for (i = 1; i < argc; i++)
        {
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
                    // "%1d:%04X Shared: CKD file: 'fakewrite' invalid without 'readonly'"
                    WRMSG( HHC00745, "E", LCSS_DEVNUM );
                    return -1;
                }

                dev->ckdfakewr = 1;
                continue;
            }
            if (strlen (argv[i]) > 3
             && memcmp("cu=", argv[i], 3) == 0)
            {
                kw = strtok_r (argv[i], "=", &strtok_str );
                op = strtok_r (NULL, " \t", &strtok_str );
                cu = op;
                continue;
            }
#if defined( HAVE_ZLIB )
            if (strlen (argv[i]) > 5
             && memcmp("comp=", argv[i], 5) == 0)
            {
                kw = strtok_r (argv[i], "=", &strtok_str );
                op = strtok_r (NULL, " \t", &strtok_str );
                dev->rmtcomp = atoi (op);
                if (dev->rmtcomp < 0 || dev->rmtcomp > 9)
                    dev->rmtcomp = 0;
                continue;
            }
#endif
            // "Shared: parameter %s in argument %d is invalid"
            WRMSG( HHC00700, "S", argv[i], i + 1 );
            return -1;
        }
    }

    /* Set suported compression */
    dev->rmtcomps = 0;
#if defined( HAVE_ZLIB )
    dev->rmtcomps |= SHRD_LIBZ;
#endif
#if defined( CCKD_BZIP2 )
    dev->rmtcomps |= SHRD_BZIP2;
#endif

    /* Update the device handler vector */
    dev->hnd = &shared_ckd_device_hndinfo;

    dev->connecting = 1;

init_retry:

    do {
        rc = clientConnect (dev, retry);
        if (rc < 0)
        {
            // "%1d:%04X Shared: connect pending to file %s"
            WRMSG( HHC00701, "W", LCSS_DEVNUM, dev->filename );
            if (retry) SLEEP(5);
        }
    } while (retry && rc < 0);

    /* Return if unable to connect */
    if (rc < 0) return 0;

    dev->ckdnumfd = 1;
    dev->ckdfd[0] = dev->fd;

    /* Get the number of cylinders */
    rc = clientRequest (dev, cyls, 4, SHRD_QUERY, SHRD_CKDCYLS, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc != 4)
    {
        // "%1d:%04X Shared: error retrieving cylinders"
        WRMSG( HHC00702, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->ckdcyls = fetch_fw (cyls);

    /* Get the device characteristics */
    rc = clientRequest (dev, dev->devchar, sizeof(dev->devchar),
                        SHRD_QUERY, SHRD_DEVCHAR, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc == 0 || rc > (int)sizeof(dev->devchar))
    {
        // "%1d:%04X Shared: error retrieving device characteristics"
        WRMSG( HHC00703, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->numdevchar = rc;

    /* Get number of heads from devchar */
    dev->ckdheads = fetch_hw (dev->devchar + 14);

    /* Calculate number of tracks */
    dev->ckdtrks = dev->ckdcyls * dev->ckdheads;
    dev->ckdhitrk[0] = dev->ckdtrks;

    /* Check the device type */
    if (dev->devtype == 0)
        dev->devtype = fetch_hw (dev->devchar + 3);
    else if (dev->devtype != fetch_hw (dev->devchar + 3))
    {
        // "%1d:%04X Shared: remote device %04X is a %04X"
        WRMSG( HHC00704, "S", LCSS_DEVNUM, dev->rmtnum, fetch_hw( dev->devchar + 3 ));
        return -1;
    }

    /* Get the device id */
    rc = clientRequest (dev, dev->devid, sizeof(dev->devid),
                        SHRD_QUERY, SHRD_DEVID, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc == 0 || rc > (int)sizeof(dev->devid))
    {
        // "%1d:%04X Shared: error retrieving device id"
        WRMSG( HHC00705, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->numdevid = rc;

    /* Get the serial number if the server supports such a query */

    if (dev->rmtver <  SHARED_VERSION ||
       (dev->rmtver == SHARED_VERSION &&
        dev->rmtrel <  SHARED_RELEASE))
    {
        /* Generate a random serial number */
        gen_dasd_serial( dev->serial );
    }
    else /* (server SHOULD support the SHRD_SERIAL query) */
    {
        rc = clientRequest (dev, dev->serial, sizeof(dev->serial),
                            SHRD_QUERY, SHRD_SERIAL, NULL, NULL);
        if (rc < 0)
            goto init_retry;
        else if (rc == 0 || rc > (int)sizeof(dev->serial))
        {
            // "%1d:%04X Shared: error retrieving serial number"
            WRMSG( HHC00716, "S", LCSS_DEVNUM );
            return -1;
        }
    }

    /* Indicate no active track */
    dev->cache = dev->bufcur = -1;
    dev->buf = NULL;

    /* Set number of sense bytes */
    dev->numsense = 32;

    /* Locate the CKD dasd table entry */
    dev->ckdtab = dasd_lookup (DASD_CKDDEV, NULL, dev->devtype, dev->ckdcyls);
    if (dev->ckdtab == NULL)
    {
        // "%1d:%04X Shared: device type %4.4X not found in dasd table"
        WRMSG( HHC00706, "S", LCSS_DEVNUM, dev->devtype );
        return -1;
    }

    /* Set the track size */
    dev->ckdtrksz = (dev->ckdtab->r1 + 511) & ~511;

    /* Locate the CKD control unit dasd table entry */
    dev->ckdcu = dasd_lookup (DASD_CKDCU, cu ? cu : dev->ckdtab->cu, 0, 0);
    if (dev->ckdcu == NULL)
    {
        // "%1d:%04X Shared: control unit %s not found in dasd table"
        WRMSG( HHC00707, "S", LCSS_DEVNUM, cu ? cu : dev->ckdtab->cu );
        return -1;
    }

    /* Set flag bit if 3990 controller */
    if (dev->ckdcu->devt == 0x3990)
        dev->ckd3990 = 1;

    /* Clear the DPA */
    memset(dev->pgid, 0, sizeof(dev->pgid));

    /* Request the channel to merge data chained write CCWs into
       a single buffer before passing data to the device handler */
    dev->cdwmerge = 1;

    /* Purge the cache */
    clientPurge (dev, 0, NULL);

    /* Log the device geometry */
    if (!dev->batch)
        // "%1d:%04X Shared: file %s: model %s cyls %d heads %d tracks %d trklen %d"
        WRMSG( HHC00708, "I", LCSS_DEVNUM, dev->filename, dev->ckdtab->name,
            dev->ckdcyls, dev->ckdheads, dev->ckdtrks, dev->ckdtrksz );

    dev->connecting = 0;

    return 0;
} /* shared_ckd_init */

/*-------------------------------------------------------------------
 * CKD close exit (client side)
 *-------------------------------------------------------------------*/
static int shared_ckd_close ( DEVBLK *dev )
{
    /* Purge the cached entries */
    clientPurge (dev, 0, NULL);

    /* Disconnect and close */
    if (dev->fd >= 0)
    {
        clientRequest (dev, NULL, 0, SHRD_DISCONNECT, 0, NULL, NULL);
        close_socket (dev->fd);
        dev->fd = -1;
    }

    return 0;
} /* shared_ckd_close */

/*-------------------------------------------------------------------
 * FBA init exit (client side)
 *-------------------------------------------------------------------*/
int shared_fba_init (DEVBLK *dev, int argc, char *argv[] )
{
int      rc;                            /* Return code               */
int      i;                             /* Loop index                */
int      retry;                         /* 1=Connection being retried*/
char    *ipname;                        /* Remote name or address    */
char    *port = NULL;                   /* Remote port               */
char    *rmtnum = NULL;                 /* Remote device number      */
struct   hostent *he;                   /* -> hostent structure      */
char    *kw;                            /* Argument keyword          */
char    *op;                            /* Argument operand          */
char     c;                             /* Work for sscanf           */
FWORD    origin;                        /* FBA origin                */
FWORD    numblks;                       /* FBA number blocks         */
FWORD    blksiz;                        /* FBA block size            */
char    *p, buf[1024];                  /* Work buffer               */
#if defined( HAVE_ZLIB )
char    *strtok_str = NULL;             /* last token                */
#endif

    /* Process the arguments */
    if (!(retry = dev->connecting))
    {
        dev->connected = 0;             /* SHRD_CONNECT not done yet */

        kw = op = NULL;

        if (argc < 1 || strlen(argv[0]) >= sizeof(buf))
            return -1;
        STRLCPY( buf, argv[0] );

        /* First argument is 'ipname:port:devnum' */
        ipname = buf;
        p = strchr (buf, ':');
        if (p)
        {
            *p = '\0';
            port = p + 1;
            p = strchr (port, ':');
        }
        if (p)
        {
            *p = '\0';
            rmtnum = p + 1;
        }

        if ( (he = gethostbyname (ipname)) == NULL )
            return -1;
        memcpy(&dev->rmtaddr, he->h_addr_list[0], sizeof(dev->rmtaddr));

        if (port)
        {
            if (sscanf(port, "%hu%c", &dev->rmtport, &c) != 1)
                return -1;
        }
        else
            dev->rmtport = SHARED_DEFAULT_PORT;

        if (rmtnum)
        {
            if (strlen (rmtnum) > 4
             || sscanf (rmtnum, "%hx%c", &dev->rmtnum, &c) != 0)
                return -1;
        }
        else
            dev->rmtnum = dev->devnum;

        /* Process the remaining arguments */
        rc = 0;
        for (i = 1; i < argc; i++)
        {
#if defined( HAVE_ZLIB )
            if (strlen (argv[i]) > 5
             && memcmp("comp=", argv[i], 5) == 0)
            {
                kw = strtok_r (argv[i], "=", &strtok_str );
                op = strtok_r (NULL, " \t", &strtok_str );
                dev->rmtcomp = atoi (op);
                if (dev->rmtcomp < 0 || dev->rmtcomp > 9)
                    dev->rmtcomp = 0;
                continue;
            }
#endif
            // "Shared: parameter %s in argument %d is invalid"
            WRMSG( HHC00700, "S", argv[i], i + 1 );
            rc = -1;
        }
        if (rc)
            return rc;
    }

    /* Set suported compression */
    dev->rmtcomps = 0;
#if defined( HAVE_ZLIB )
    dev->rmtcomps |= SHRD_LIBZ;
#endif
#if defined( CCKD_BZIP2 )
    dev->rmtcomps |= SHRD_BZIP2;
#endif

    /* Update the device handler vector */
    dev->hnd = &shared_fba_device_hndinfo;

    dev->connecting = 1;

init_retry:

    do {
        rc = clientConnect (dev, retry);
        if (rc < 0)
        {
            // "%1d:%04X Shared: connect pending to file %s"
            WRMSG( HHC00701, "W", LCSS_DEVNUM, dev->filename );
            if (retry) SLEEP(5);
        }
    } while (retry && rc < 0);

    /* Return if unable to connect */
    if (rc < 0) return 0;

    /* Get the fba origin */
    rc = clientRequest (dev, origin, 4, SHRD_QUERY, SHRD_FBAORIGIN, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc != 4)
    {
        // "%1d:%04X Shared: error retrieving fba origin"
        WRMSG( HHC00709, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->fbaorigin = fetch_fw (origin);

    /* Get the number of blocks */
    rc = clientRequest (dev, numblks, 4, SHRD_QUERY, SHRD_FBANUMBLK, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc != 4)
    {
        // "%1d:%04X Shared: error retrieving fba number blocks"
        WRMSG( HHC00710, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->fbanumblk = fetch_fw (numblks);

    /* Get the block size */
    rc = clientRequest (dev, blksiz, 4, SHRD_QUERY, SHRD_FBABLKSIZ, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc != 4)
    {
        // "%1d:%04X Shared: error retrieving fba block size"
        WRMSG( HHC00711, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->fbablksiz = fetch_fw (blksiz);
    dev->fbaend = (dev->fbaorigin + dev->fbanumblk) * dev->fbablksiz;

    /* Get the device id */
    rc = clientRequest (dev, dev->devid, sizeof(dev->devid),
                        SHRD_QUERY, SHRD_DEVID, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc == 0 || rc > (int)sizeof(dev->devid))
    {
        // "%1d:%04X Shared: error retrieving device id"
        WRMSG( HHC00705, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->numdevid = rc;

    /* Check the device type */
    if (dev->devtype != fetch_hw (dev->devid + 4))
    {
        // "%1d:%04X Shared: remote device %04X is a %04X"
        WRMSG( HHC00704, "S", LCSS_DEVNUM, dev->rmtnum, fetch_hw( dev->devid + 4 ));
        return -1;
    }

    /* Get the device characteristics */
    rc = clientRequest (dev, dev->devchar, sizeof(dev->devchar),
                        SHRD_QUERY, SHRD_DEVCHAR, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc == 0 || rc > (int)sizeof(dev->devchar))
    {
        // "%1d:%04X Shared: error retrieving device characteristics"
        WRMSG( HHC00703, "S", LCSS_DEVNUM );
        return -1;
    }
    dev->numdevchar = rc;

    /* Get the serial number if the server supports such a query */

    if (dev->rmtver <  SHARED_VERSION ||
       (dev->rmtver == SHARED_VERSION &&
        dev->rmtrel <  SHARED_RELEASE))
    {
        /* Generate a random serial number */
        gen_dasd_serial( dev->serial );
    }
    else /* (server SHOULD support the SHRD_SERIAL query) */
    {
        rc = clientRequest (dev, dev->serial, sizeof(dev->serial),
                            SHRD_QUERY, SHRD_SERIAL, NULL, NULL);
        if (rc < 0)
            goto init_retry;
        else if (rc == 0 || rc > (int)sizeof(dev->serial))
        {
            // "%1d:%04X Shared: error retrieving serial number"
            WRMSG( HHC00716, "S", LCSS_DEVNUM );
            return -1;
        }
    }

    /* Indicate no active track */
    dev->cache = dev->bufcur = -1;
    dev->buf = NULL;

    /* Set number of sense bytes */
    dev->numsense = 32;

    /* Locate the FBA dasd table entry */
    dev->fbatab = dasd_lookup (DASD_FBADEV, NULL, dev->devtype, dev->fbanumblk);
    if (dev->fbatab == NULL)
    {
        // "%1d:%04X Shared: device type %4.4X not found in dasd table"
        WRMSG( HHC00706, "S", LCSS_DEVNUM, dev->devtype );
        return -1;
    }

    /* Purge the cache */
    clientPurge (dev, 0, NULL);

    /* Log the device geometry */
    // "%1d:%04X Shared: file %s: model %s origin %"PRId64" blks %d"
    WRMSG( HHC00712, "I", LCSS_DEVNUM, dev->filename, dev->fbatab->name,
        dev->fbaorigin, dev->fbanumblk );

    dev->connecting = 0;

    return 0;
}

/*-------------------------------------------------------------------
 * FBA close exit (client side)
 *-------------------------------------------------------------------*/
static int shared_fba_close (DEVBLK *dev)
{
    /* Purge the cached entries */
    clientPurge (dev, 0, NULL);

    /* Disconnect and close */
    if (dev->fd >= 0)
    {
        clientRequest (dev, NULL, 0, SHRD_DISCONNECT, 0, NULL, NULL);
        close_socket (dev->fd);
        dev->fd = -1;
    }

    return 0;
}

/*-------------------------------------------------------------------
 * Start I/O exit (client side)
 *-------------------------------------------------------------------*/
static void shared_start(DEVBLK *dev)
{
int      rc;                            /* Return code               */
U16      devnum;                        /* Cache device number       */
int      trk;                           /* Cache track number        */
int      code;                          /* Response code             */
BYTE     buf[SHARED_PURGE_MAX * 4];     /* Purge list                */

    SHRDTRACE("start cur %d cache %d",dev->bufcur,dev->cache);

    /* Send the START request */
    rc = clientRequest (dev, buf, sizeof(buf),
                        SHRD_START, 0, &code, NULL);
    if (rc < 0)
    {
        // "%1d:%04X Shared: error during channel program start"
        WRMSG( HHC00713, "E", LCSS_DEVNUM );
        clientPurge (dev, 0, NULL);
        dev->cache = dev->bufcur = -1;
        dev->buf = NULL;
        return;
    }

    /* Check for purge */
    if (code & SHRD_PURGE)
    {
        if (rc / 4 > SHARED_PURGE_MAX) rc = 0;
        clientPurge (dev, rc / 4, buf);
    }

    /* Make previous active entry active again */
    if (dev->cache >= 0)
    {
        cache_lock (CACHE_DEVBUF);
        SHRD_CACHE_GETKEY (dev->cache, devnum, trk);
        if (dev->devnum == devnum && dev->bufcur == trk)
            cache_setflag(CACHE_DEVBUF, dev->cache, ~0, SHRD_CACHE_ACTIVE);
        else
        {
            dev->cache = dev->bufcur = -1;
            dev->buf = NULL;
        }
        cache_unlock (CACHE_DEVBUF);
    }
} /* shared_start */

/*-------------------------------------------------------------------
 * End I/O exit (client side)
 *-------------------------------------------------------------------*/
static void shared_end (DEVBLK *dev)
{
int      rc;                            /* Return code               */

    SHRDTRACE("end cur %d cache %d",dev->bufcur,dev->cache);

    /* Write the previous active entry if it was updated */
    if (dev->bufupd)
        clientWrite (dev, dev->bufcur);
    dev->bufupd = 0;

    /* Mark the active entry inactive */
    if (dev->cache >= 0)
    {
        cache_lock (CACHE_DEVBUF);
        cache_setflag (CACHE_DEVBUF, dev->cache, ~SHRD_CACHE_ACTIVE, 0);
        cache_unlock (CACHE_DEVBUF);
    }

    /* Send the END request */
    rc = clientRequest (dev, NULL, 0, SHRD_END, 0, NULL, NULL);
    if (rc < 0)
    {
        // "%1d:%04X Shared: error during channel program end"
        WRMSG( HHC00714, "E", LCSS_DEVNUM );
        clientPurge (dev, 0, NULL);
        dev->cache = dev->bufcur = -1;
        dev->buf = NULL;
        return;
    }
} /* shared_end */

/*-------------------------------------------------------------------
 * Shared ckd read track exit (client side)
 *-------------------------------------------------------------------*/
static int shared_ckd_read (DEVBLK *dev, int trk, BYTE *unitstat)
{
int      rc;                            /* Return code               */
int      retries = 10;                  /* Number read retries       */
int      cache;                         /* Lookup index              */
int      lru;                           /* Available index           */
int      len;                           /* Response length           */
int      id;                            /* Response id               */
int      status;                        /* Response status           */
BYTE    *buf;                           /* Cache buffer              */
BYTE     code;                          /* Response code             */
U16      devnum;                        /* Response device number    */
BYTE     hdr[SHRD_HDR_SIZE + 4];        /* Read request header       */

    /* Return if reading the same track image */
    if (trk == dev->bufcur && dev->cache >= 0)
    {
        dev->bufoff = 0;
        dev->bufoffhi = dev->ckdtrksz;
        return 0;
    }

    SHRDTRACE( "ckd read trk %d", trk );

    /* Write the previous active entry if it was updated */
    if (dev->bufupd)
        clientWrite (dev, dev->bufcur);
    dev->bufupd = 0;

    /* Reset buffer offsets */
    dev->bufoff = 0;
    dev->bufoffhi = dev->ckdtrksz;

    cache_lock (CACHE_DEVBUF);

    /* Inactivate the previous image */
    if (dev->cache >= 0)
        cache_setflag (CACHE_DEVBUF, dev->cache, ~SHRD_CACHE_ACTIVE, 0);
    dev->cache = dev->bufcur = -1;

cache_retry:

    /* Lookup the track in the cache */
    cache = cache_lookup (CACHE_DEVBUF, SHRD_CACHE_SETKEY(dev->devnum, trk), &lru);

    /* Process cache hit */
    if (cache >= 0)
    {
        cache_setflag (CACHE_DEVBUF, cache, ~0, SHRD_CACHE_ACTIVE);
        cache_unlock (CACHE_DEVBUF);
        dev->cachehits++;
        dev->cache = cache;
        dev->buf = cache_getbuf (CACHE_DEVBUF, cache, 0);
        dev->bufcur = trk;
        dev->bufoff = 0;
        dev->bufoffhi = dev->ckdtrksz;
        dev->buflen = shared_ckd_trklen (dev, dev->buf);
        dev->bufsize = cache_getlen (CACHE_DEVBUF, cache);
        SHRDTRACE( "ckd read trk %d cache hit %d", trk, dev->cache );
        return 0;
    }

    /* Special processing if no available cache entry */
    if (lru < 0)
    {
        SHRDTRACE( "ckd read trk %d cache wait", trk );
        dev->cachewaits++;
        cache_wait (CACHE_DEVBUF);
        goto cache_retry;
    }

    /* Process cache miss */
    SHRDTRACE( "ckd read trk %d cache miss %d", trk, dev->cache );
    dev->cachemisses++;
    cache_setflag (CACHE_DEVBUF, lru, 0, SHRD_CACHE_ACTIVE|DEVBUF_TYPE_SCKD);
    cache_setkey (CACHE_DEVBUF, lru, SHRD_CACHE_SETKEY(dev->devnum, trk));
    cache_setage (CACHE_DEVBUF, lru);
    buf = cache_getbuf (CACHE_DEVBUF, lru, dev->ckdtrksz);

    cache_unlock (CACHE_DEVBUF);

read_retry:

    /* Send the read request for the track to the remote host */
    SHRD_SET_HDR (hdr, SHRD_READ, 0, dev->rmtnum, dev->rmtid, 4);
    store_fw (hdr + SHRD_HDR_SIZE, trk);
    rc = clientSend (dev, hdr, NULL, 0);
    if (rc < 0)
    {
        ckd_build_sense (dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        // "%1d:%04X Shared: remote error reading track %d"
        WRMSG( HHC00715, "E", LCSS_DEVNUM, trk );
        return -1;
    }

    /* Read the track from the remote host */
    rc = clientRecv (dev, hdr, buf, dev->ckdtrksz);
    SHRD_GET_HDR( hdr, code, status, devnum, id, len );
    if (rc < 0 || code & SHRD_ERROR)
    {
        if (rc < 0 && retries--) goto read_retry;
        ckd_build_sense (dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        // "%1d:%04X Shared: remote error reading track %d"
        WRMSG( HHC00715, "E", LCSS_DEVNUM, trk );
        return -1;
    }

    /* Read the sense data if an i/o error occurred */
    if (code & SHRD_IOERR)
        clientRequest (dev, dev->sense, dev->numsense,
                      SHRD_SENSE, 0, NULL, NULL);

    /* Read complete */
    dev->cache = lru;
    dev->buf = cache_getbuf (CACHE_DEVBUF, lru, 0);
    dev->bufcur = trk;
    dev->bufoff = 0;
    dev->bufoffhi = dev->ckdtrksz;
    dev->buflen = shared_ckd_trklen (dev, dev->buf);
    dev->bufsize = cache_getlen (CACHE_DEVBUF, lru);
    dev->buf[0] = 0;

    return 0;
} /* shared_ckd_read */

/*-------------------------------------------------------------------
 * Shared ckd write track exit (client side)
 *-------------------------------------------------------------------*/
static int shared_ckd_write (DEVBLK *dev, int trk, int off,
                             BYTE *buf, int len, BYTE *unitstat)
{
int      rc;                            /* Return code               */

    /* Immediately return if fake writing */
    if (dev->ckdfakewr)
        return len;

    /* Error if opened read-only */
    if (dev->ckdrdonly)
    {
        ckd_build_sense (dev, SENSE_EC, SENSE1_WRI, 0,
                        FORMAT_1, MESSAGE_0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    SHRDTRACE( "ckd write trk %d off %d len %d", trk, off, len );

    /* If the track is not current then read it */
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
        ckd_build_sense (dev, 0, SENSE1_ITF, 0, 0, 0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }

    /* Copy the data into the buffer */
    if (buf) memcpy (dev->buf + off, buf, len);

    /* Set low and high updated offsets */
    if (!dev->bufupd || off < dev->bufupdlo)
        dev->bufupdlo = off;
    if (dev->bufoff + len > dev->bufupdhi)
        dev->bufupdhi = off + len;

    /* Indicate track image has been modified */
    if (!dev->bufupd)
    {
        dev->bufupd = 1;
        shared_update_notify (dev, trk);
    }

    return len;
} /* shared_ckd_write */

/*-------------------------------------------------------------------
 * Return track image length
 *-------------------------------------------------------------------*/
static int shared_ckd_trklen (DEVBLK *dev, BYTE *buf)
{
int             sz;                     /* Size so far               */

    for (sz = CKD_TRKHDR_SIZE;
         memcmp( buf + sz, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) != 0; )
    {
        /* add length of count, key, and data fields */
        sz += CKD_RECHDR_SIZE +
                buf[sz+5] +
                (buf[sz+6] << 8) + buf[sz+7];
        if (sz > dev->ckdtrksz - 8) break;
    }

    /* add length for end-of-track indicator */
    sz += CKD_RECHDR_SIZE;

    if (sz > dev->ckdtrksz)
        sz = dev->ckdtrksz;

    return sz;
}

/*-------------------------------------------------------------------
 * Shared usage exit (client side)
 *-------------------------------------------------------------------*/
static int shared_used (DEVBLK *dev)
{
int      rc;                            /* Return code               */
FWORD    usage;                         /* Usage buffer              */

    /* Get usage information */
    rc = clientRequest (dev, usage, 4, SHRD_USED, 0, NULL, NULL);
    if (rc != 4)
    {
        // "%1d:%04X Shared: error retrieving usage information"
        WRMSG( HHC00717, "E", LCSS_DEVNUM );
        return -1;
    }
    return fetch_fw (usage);
} /* shared_used */

/*-------------------------------------------------------------------
 * Shared reserve exit (client side)
 *-------------------------------------------------------------------*/
static void shared_reserve (DEVBLK *dev)
{
int      rc;                            /* Return code               */

    /* Issue reserve request */
    rc = clientRequest (dev, NULL, 0, SHRD_RESERVE, 0, NULL, NULL);

} /* shared_reserve */

/*-------------------------------------------------------------------
 * Shared release exit (client side)
 *-------------------------------------------------------------------*/
static void shared_release (DEVBLK *dev)
{
int      rc;                            /* Return code               */

    /* Issue release request */
    rc = clientRequest (dev, NULL, 0, SHRD_RELEASE, 0, NULL, NULL);

} /* shared_release */

/*-------------------------------------------------------------------
 * Write to host
 *
 * NOTE - writes are deferred until a switch occurs or the
 *        channel program ends.  We are called from either the
 *        read exit or the end channel program exit.
 *-------------------------------------------------------------------*/
static int clientWrite (DEVBLK *dev, int block)
{
int         rc;                         /* Return code               */
int         retries = 10;               /* Number write retries      */
int         len;                        /* Data length               */
BYTE        hdr[SHRD_HDR_SIZE + 2 + 4]; /* Write header              */
BYTE        code;                       /* Response code             */
int         status;                     /* Response status           */
int         id;                         /* Response identifier       */
U16         devnum;                     /* Response device number    */
BYTE        errmsg[SHARED_MAX_MSGLEN+1];/* Error message             */

    /* Calculate length to write */
    len = dev->bufupdhi - dev->bufupdlo;
    if (len <= 0 || dev->bufcur < 0)
    {
        dev->bufupdlo = dev->bufupdhi = 0;
        return 0;
    }

    SHRDTRACE("write rcd %d off %d len %d",block,dev->bufupdlo,len);

write_retry:

    /* The write request contains a 2 byte offset and 4 byte id,
       followed by the data */
    SHRD_SET_HDR (hdr, SHRD_WRITE, 0, dev->rmtnum, dev->rmtid, len + 6);
    store_hw (hdr + SHRD_HDR_SIZE, dev->bufupdlo);
    store_fw (hdr + SHRD_HDR_SIZE + 2, block);

    rc = clientSend (dev, hdr, dev->buf + dev->bufupdlo, len);
    if (rc < 0)
    {
        // "%1d:%04X Shared: error writing track %d"
        WRMSG( HHC00718, "E", LCSS_DEVNUM, dev->bufcur );
        dev->bufupdlo = dev->bufupdhi = 0;
        clientPurge (dev, 0, NULL);
        return -1;
    }

    /* Get the response */
    rc = clientRecv (dev, hdr, errmsg, sizeof(errmsg));
    SHRD_GET_HDR (hdr, code, status, devnum, id, len);
    if (rc < 0 || (code & SHRD_ERROR) || (code & SHRD_IOERR))
    {
        if (rc < 0 && retries--) goto write_retry;
        // "%1d:%04X Shared: remote error writing track %d %2.2X-%2.2X"
        WRMSG( HHC00719, "E", LCSS_DEVNUM, dev->bufcur, code, status );
        dev->bufupdlo = dev->bufupdhi = 0;
        clientPurge (dev, 0, NULL);
        return -1;
    }

    dev->bufupdlo = dev->bufupdhi = 0;
    return rc;
} /* clientWrite */

/*-------------------------------------------------------------------
 * Purge cache entries (client side)
 *-------------------------------------------------------------------*/
static void clientPurge (DEVBLK *dev, int n, void *buf)
{
    cache_lock(CACHE_DEVBUF);
    dev->rmtpurgen = n;
    dev->rmtpurge = (FWORD *)buf;
    cache_scan (CACHE_DEVBUF, clientPurgescan, dev);
    cache_unlock(CACHE_DEVBUF);
}
static int clientPurgescan (int *answer, int ix, int i, void *data)
{
U16             devnum;                 /* Cached device number      */
int             trk;                    /* Cached track              */
int             p;                      /* Purge index               */
DEVBLK         *dev = data;             /* -> device block           */

    UNREFERENCED(answer);
    SHRD_CACHE_GETKEY(i, devnum, trk);
    if (devnum == dev->devnum)
    {
        if (dev->rmtpurgen == 0) {
            cache_release (ix, i, 0);
            SHRDTRACE("purge %d",trk);
        }
        else
        {
            for (p = 0; p < dev->rmtpurgen; p++)
            {
                if (trk == (int)fetch_fw (dev->rmtpurge[p]))
                {
                    SHRDTRACE("purge %d",trk);
                    cache_release (ix, i, 0);
                    break;
                }
            }
        }
    }
    return 0;
} /* clientPurge */

/*-------------------------------------------------------------------
 * Connect to the server (client side)
 *-------------------------------------------------------------------*/
static int clientConnect( DEVBLK* dev, int retry )
{
int                rc;                  /* Return code               */
struct sockaddr*   server;              /* -> Server descriptor      */
int                len;                 /* Length server descriptor  */
struct sockaddr_in iserver;             /* inet server descriptor    */
#if defined( HAVE_SYS_UN_H )
struct sockaddr_un userver;             /* Unix server descriptor    */
#endif
int                retries = 10;        /* Number of retries         */
HWORD              id;                  /* Returned identifier       */
HWORD              comp;                /* Returned compression parm */

    SHRDTRACE( "Beg clientConnect sequence for dev %4.4x retry=%d", dev->devnum, retry );

    do
    {
        /* Close previous connection */
        if (dev->fd >= 0)
            close_socket( dev->fd );

        /* Get a new socket */
        if (dev->localhost)
        {
#if defined( HAVE_SYS_UN_H )
            dev->fd = socket( AF_UNIX, SOCK_STREAM, 0 );
#else
            dev->fd = -1;
#endif
            dev->ckdfd[0] = dev->fd;

            if (dev->fd < 0)
            {
                // "%1d:%04X Shared: error in function %s: %s"
                WRMSG( HHC00720, "E", LCSS_DEVNUM, "socket()", strerror( HSO_errno ));
                return -1;
            }
#if defined( HAVE_SYS_UN_H )
            userver.sun_family = AF_UNIX;
            sprintf( userver.sun_path, "/tmp/hercules_shared.%d", dev->rmtport );
            server = (struct sockaddr*) &userver;
            len = sizeof( userver );
#endif
        }
        else
        {
            dev->fd = socket( AF_INET, SOCK_STREAM, 0 );
            dev->ckdfd[0] = dev->fd;

            if (dev->fd < 0)
            {
                // "%1d:%04X Shared: error in function %s: %s"
                WRMSG( HHC00720, "E", LCSS_DEVNUM, "socket()", strerror( HSO_errno ));
                return -1;
            }

            iserver.sin_family = AF_INET;
            iserver.sin_port   = htons( dev->rmtport );
            memcpy( &iserver.sin_addr.s_addr, &dev->rmtaddr, sizeof( struct in_addr ));
            server = (struct sockaddr*) &iserver;
            len = sizeof( iserver );
        }

        /* Connect to the server */
        store_hw( id, dev->rmtid );
        rc = connect( dev->fd, server, len );

        if (rc < 0)
            SHRDTRACE( "connect rc=%d errno=%d %s", rc, HSO_errno, strerror( HSO_errno ));
        else
            SHRDTRACE( "connect rc=%d", rc );

        if (rc >= 0)
        {
            /* Request device connection (if we haven't done so yet) */
            if (!dev->connected)
            {
                int flag = (SHARED_VERSION << 4) | SHARED_RELEASE;
                rc = clientRequest( dev, id, 2, SHRD_CONNECT, flag, NULL, &flag );
                if (rc >= 0)
                {
                    dev->connected = 1;             // (SHRD_CONNECT success)
                    dev->rmtid  = fetch_hw( id );   // (must only do ONCE!!!)
                    dev->rmtver = flag >> 4;        // (save server version)
                    dev->rmtrel = flag & 0x0f;      // (save server release)

                    if (!dev->batch)
                        if (MLVL( VERBOSE ))
                            // "%1d:%04X Shared: connected to v%d.%d server id %d file %s"
                            WRMSG( HHC00721, "I", LCSS_DEVNUM, dev->rmtver,
                                dev->rmtrel, dev->rmtid, dev->filename );
                    /*
                     * Negotiate compression - top 4 bits have the compression
                     * algorithms we support (00010000 -> libz; 00100000 ->bzip2,
                     * 00110000 -> both) and the bottom 4 bits indicates the
                     * libz parm we want to use when sending data back & forth.
                     * If the server returns '0' back, then we won't use libz to
                     * compress data to the server.  What the 'compression
                     * algorithms we support' means is that if the data source is
                     * cckd or cfba then the server doesn't have to uncompress
                     * the data for us if we support the compression algorithm.
                     */
                    if (dev->rmtcomp || dev->rmtcomps)
                    {
                        rc = clientRequest( dev, comp, 2, SHRD_COMPRESS,
                                   (dev->rmtcomps << 4) | dev->rmtcomp, NULL, NULL );
                        if (rc >= 0)
                            dev->rmtcomp = fetch_hw( comp );
                    }
                }
            }
        }
        else if (!retry)
            // "%1d:%04X Shared: error in connect to file %s: %s"
            WRMSG( HHC00722, "E", LCSS_DEVNUM, dev->filename, strerror( HSO_errno ));

        if (rc < 0 && retry)
            USLEEP( 20000 );    // (20ms between connect retries?!)
    }
    while (rc < 0 && retry && retries--);

    SHRDTRACE( "end clientConnect sequence for dev %4.4x retry=%d", dev->devnum, retry );

    return rc;

} /* clientConnect */

/*-------------------------------------------------------------------
 * Send request to host and get the response
 *
 * No data is sent on the request, buf gets the response.
 * If an uncorrectable connection error occurs -1 is returned.
 * Otherwise *code and *status is set from the response header
 *
 * Since 'buf' may be NULL or not very long, response data is
 * received in a temporary buffer.  This enables us to receive
 * an error message from the remote system.
 *-------------------------------------------------------------------*/
static int clientRequest (DEVBLK *dev, BYTE *buf, int len, int cmd,
                          int flags, int *code, int *status)
{
int      rc;                            /* Return code               */
int      retries = 10;                  /* Number retries            */
BYTE     rcode;                         /* Request return code       */
BYTE     rstatus;                       /* Request return status     */
U16      rdevnum;                       /* Request return devnum     */
int      rid;                           /* Request return id         */
int      rlen;                          /* Request return length     */
BYTE     hdr[SHRD_HDR_SIZE];            /* Header                    */
BYTE     temp[256];                     /* Temporary buffer          */

retry:

    /* Send the request */
    SHRD_SET_HDR(hdr, cmd, flags, dev->rmtnum, dev->rmtid, 0);
    SHRDHDRTRACE( "client request", hdr );
    rc = clientSend (dev, hdr, NULL, 0);
    if (rc < 0) return rc;

    /* Receive the response */
    rc = clientRecv (dev, hdr, temp, sizeof(temp));

    /* Retry recv errors */
    if (rc < 0)
    {
        if (1
            && cmd != SHRD_CONNECT
            && cmd != SHRD_DISCONNECT
            && retries--
        )
        {
            SLEEP (1);
            clientConnect (dev, 1);
            goto retry;
        }
        return -1;
    }

    /* Set code and status */
    SHRD_GET_HDR(hdr, rcode, rstatus, rdevnum, rid, rlen);
    SHRDHDRTRACE( "client response", hdr );

    if (code)   *code   = rcode;
    if (status) *status = rstatus;

    /* Copy the data into the caller's buffer */
    if (buf && len > 0 && rlen > 0)
        memcpy (buf, temp, len < rlen ? len : rlen);

    return rlen;
} /* clientRequest */

/*-------------------------------------------------------------------
 * Send a request to the host
 *
 * 'buf' may be NULL
 * 'buflen' is the length in 'buf' (should be 0 if 'buf' is NULL)
 * 'hdr' may contain additional data; this is detected by the
 *       difference between 'buflen' and the length in the header
 *
 * If 'buf' is adjacent to 'hdr' then 'buf' should be NULL
 *
 *-------------------------------------------------------------------*/
static int clientSend( DEVBLK* dev, BYTE* hdr, BYTE* buf, int buflen )
{
int      rc;                            /* Return code               */
BYTE     cmd;                           /* Header command            */
BYTE     flag;                          /* Header flags              */
U16      devnum;                        /* Header device number      */
int      len;                           /* Header length             */
int      id;                            /* Header identifier         */
int      hdrlen;                        /* Header length + other data*/
int      off;                           /* Offset to buffer data     */
BYTE*    sendbuf;                       /* Send buffer               */
int      sendlen;                       /* Send length               */
BYTE     cbuf[SHRD_HDR_SIZE + 65536];   /* Combined buffer           */
const char* trcmsg;                     /* Header trace message      */

    /* Save original hdr values */
    SHRD_GET_HDR( hdr, cmd, flag, devnum, id, len );

    /* Connect to remote device on server if needed */
    if (dev->fd < 0)
    {
        if (SHRD_DISCONNECT == cmd)
            return -1; // (already disconnected!)

        if ((rc = clientConnect( dev, 1 )) < 0)
            return -1;

        /* Update values due to clientConnect */
        id = dev->rmtid;
        SHRD_SET_HDR( hdr, cmd, flag, devnum, id, len );
    }

    /* Make buf, buflen consistent if no additional data to be sent  */
    if (buf == NULL) buflen = 0;
    else if (buflen == 0) buf = NULL;

    /* Calculate length of header, may contain additional data */
    hdrlen = SHRD_HDR_SIZE + (len - buflen);
    off = len - buflen;

#if defined( HAVE_ZLIB )
    /* Compress the buf */
    if (1
        && dev->rmtcomp != 0
        && flag == 0
        && off <= SHRD_COMP_MAX_OFF
        && buflen >= SHARED_COMPRESS_MINLEN
    )
    {
        unsigned long newlen;
        newlen = 65536 - hdrlen;
        memcpy( cbuf, hdr, hdrlen );
        rc = compress2( cbuf + hdrlen, &newlen,
                        buf, buflen, dev->rmtcomp );
        if (rc == Z_OK && (int)newlen < buflen)
        {
            cmd |= SHRD_COMP;
            flag = (SHRD_LIBZ << 4) | off;
            hdr = cbuf;
            hdrlen += newlen;
            buf = NULL;
            buflen = 0;
        }
    }
#endif

    /* Combine header and data unless there's no buffer */
    if (buflen == 0)
    {
        sendbuf = hdr;
        sendlen = hdrlen;
    }
    else
    {
        memcpy( cbuf, hdr, hdrlen );
        memcpy( cbuf + hdrlen, buf, buflen );
        sendbuf = cbuf;
        sendlen = hdrlen + buflen;
    }

    len = (sendlen - SHRD_HDR_SIZE);
    trcmsg = "client send";

retry:

    /* Update hdr with final values */
    SHRD_SET_HDR( sendbuf, cmd, flag, devnum, id, len );

    /* Trace what we'll be sending */
    if (cmd & SHRD_COMP)
        SHRDHDRTRACE2( trcmsg, sendbuf, "(compressed)" );
    else
        SHRDHDRTRACE(  trcmsg, sendbuf );

    /* Send the header and data */
    if ((rc = send( dev->fd, sendbuf, sendlen, 0 )) < 0)
    {
        /* Trace the socket error */
        SHRDTRACE( "send rc=%d errno=%d %s", rc, HSO_errno, strerror( HSO_errno ));

        /* If the request we're trying to send to the server
           is a disconnect request, it doesn't make much sense
           to go to the time and effort to re-connect to the
           server only to tell it we're about to DISCONNECT!
        */
        if (SHRD_DISCONNECT != cmd)
        {
            if ((rc = clientConnect( dev, 0 )) >= 0)
            {
                /* Pickup new rmtid due to clientConnect */
                id = dev->rmtid;
                trcmsg = "client send retry";
                goto retry;
            }
        }
    }

    /* Process return code */
    if (rc < 0 && SHRD_DISCONNECT != cmd)
    {
        // "%1d:%04X Shared: error in send for %2.2X-%2.2X: %s"
        WRMSG( HHC00723, "E", LCSS_DEVNUM, cmd, flag, strerror( HSO_errno ));
        return -1;
    }

    return rc;

} /* clientSend */

/*-------------------------------------------------------------------
 * Receive a response (client side)
 *-------------------------------------------------------------------*/
static int clientRecv (DEVBLK *dev, BYTE *hdr, BYTE *buf, int buflen)
{
int      rc;                            /* Return code               */
BYTE     code;                          /* Response code             */
BYTE     status;                        /* Response status           */
U16      devnum;                        /* Response device number    */
int      id;                            /* Response identifier       */
int      len;                           /* Response length           */
static const bool server_req = true;

    /* Clear the header to zeroes */
    memset( hdr, 0, SHRD_HDR_SIZE );

    /* Return error if not connected */
    if (dev->fd < 0)
    {
        // "%1d:%04X Shared: not connected to file %s"
        WRMSG( HHC00724, "E", LCSS_DEVNUM, dev->filename );
        return -1;
    }

    /* Receive the header */
    rc = recvData (dev->fd, hdr, buf, buflen, !server_req );
    if (rc < 0)
    {
        if (rc != -1 && rc != -HSO_ECONNABORTED)
            // "%1d:%04X Shared: error in receive: %s"
            WRMSG( HHC00725, "E", LCSS_DEVNUM, strerror( -rc ));
        return rc;
    }

    SHRD_GET_HDR(hdr, code, status, devnum, id, len);
    SHRDHDRTRACE( "client recv", hdr );

    /* Handle remote logical error */
    if (code & SHRD_ERROR)
    {
        // "%1d:%04X Shared: remote error %2.2X-%2.2X: %s"
        WRMSG( HHC00726, "E", LCSS_DEVNUM, code, status, buf );
        len = 0;
    }

    /* Reset code/status if response was compressed */
    if (len > 0 && code == SHRD_COMP)
    {
        code = SHRD_OK;
        status = 0;
    }

    /* Reset the header */
    SHRD_SET_HDR(hdr, code, status, devnum, id, len);

    return len;
} /* clientRecv */

/*-------------------------------------------------------------------
 *             Receive data (server or client)
 *-------------------------------------------------------------------
 *
 * Returns:
 *
 *   > 0     number of bytes received
 *
 *     0     no data available
 *
 *    -1     data decompression error
 *
 *   < 1     a socket error has occurred; the return code is the
 *           negative of the socket error code (e.g. -ECONNRESET),
 *           so you should use "strerror( -rc )" to report it.
 *
 *-------------------------------------------------------------------*/
static int recvData(int sock, BYTE *hdr, BYTE *buf, int buflen, bool server_req)
{
int                     rc;             /* Return code               */
int                     rlen;           /* Data length to recv       */
int                     recvlen;        /* Total length              */
BYTE                   *recvbuf;        /* Receive buffer            */
BYTE                    cmd;            /* Header command            */
BYTE                    flag;           /* Header flags              */
U16                     devnum;         /* Header device number      */
int                     id;             /* Header identifier         */
int                     len;            /* Header length             */
int                     comp = 0;       /* Compression type          */
int                     off = 0;        /* Offset to compressed data */
DEVBLK                 *dev = NULL;     /* For 'SHRDTRACE'             */
BYTE                    cbuf[65536];    /* Compressed buffer         */

    /* Receive the header */
    for (recvlen = 0; recvlen < (int)SHRD_HDR_SIZE; recvlen += rc)
    {
        rc = recv (sock, hdr + recvlen, SHRD_HDR_SIZE - recvlen, 0);
        if (rc < 0)
            return -HSO_errno;
        else if (rc == 0)
            return -HSO_ECONNABORTED;
    }

    SHRD_GET_HDR (hdr, cmd, flag, devnum, id, len);
    SHRDHDRTRACE( "recvData", hdr );

    /* Return if no data */
    if (len == 0) return 0;

    /* Check for compressed data */
    if ((server_req && (cmd & SHRD_COMP))
     || (!server_req && cmd == SHRD_COMP))
    {
        comp = (flag & SHRD_COMP_MASK) >> 4;
        off = flag & SHRD_COMP_OFF;
        cmd &= ~SHRD_COMP;
        flag = 0;
        recvbuf = cbuf;
        rlen = len;
    }
    else
    {
        recvbuf = buf;
        rlen = buflen < len ? buflen : len;
    }

    /* Receive the data */
    for (recvlen = 0; recvlen < rlen; recvlen += rc)
    {
        rc = recv (sock, recvbuf + recvlen, len - recvlen, 0);
        if (rc < 0)
            return -HSO_errno;
        else if (rc == 0)
            return -HSO_ECONNABORTED;
    }

    /* Flush any remaining data */
    for (; rlen < len; rlen += rc)
    {
        BYTE buf[256];
        rc = recv (sock, buf, len - rlen < 256 ? len - rlen : 256, 0);
        if (rc < 0)
            return -HSO_errno;
        else if (rc == 0)
            return -HSO_ECONNABORTED;
    }

    /* Check for compression */
    if (comp == SHRD_LIBZ) {
#if defined( HAVE_ZLIB )
        unsigned long newlen;

        if (off > 0)
            memcpy (buf, cbuf, off);
        newlen = buflen - off;
        rc = uncompress(buf + off, &newlen, cbuf + off, len - off);
        if (rc == Z_OK)
            recvlen = (int)newlen + off;
        else
        {
            // "Shared: decompress error %d offset %d length %d"
            WRMSG( HHC00727, "E", rc, off, len - off );
            recvlen = -1;
        }
#else
        // "Shared: data compressed using method %s is unsupported"
        WRMSG( HHC00728, "E", "libz" );
        recvlen = -1;
#endif
    }
    else if (comp == SHRD_BZIP2)
    {
#if defined( CCKD_BZIP2 )
        unsigned int newlen;

        if (off > 0)
            memcpy (buf, cbuf, off);

        newlen = buflen - off;
        rc = BZ2_bzBuffToBuffDecompress((void *)(buf + off), &newlen, (void *)(cbuf + off), len - off, 0, 0);
        if (rc == BZ_OK)
            recvlen = (int)newlen + off;
        else
        {
            // "Shared: decompress error %d offset %d length %d"
            WRMSG( HHC00727, "E", rc, off, len - off );
            recvlen = -1;
        }
#else
        // "Shared: data compressed using method %s is unsupported"
        WRMSG( HHC00728, "E", "bzip2" );
        recvlen = -1;
#endif
    }

    if (recvlen > 0)
    {
        SHRD_SET_HDR (hdr, cmd, flag, devnum, id, recvlen);
        if (comp)
            SHRDHDRTRACE2( "recvData", hdr, "(uncompressed)" );
    }

    return recvlen;

} /* recvData */

/*-------------------------------------------------------------------
 * Convert shared command code to string for tracing purposes
 *-------------------------------------------------------------------*/
static const char* shrdcmd2str( const BYTE cmd )
{
    switch (cmd)
    {
        // Requests
        case SHRD_CONNECT:          return "CONNECT ";
        case SHRD_DISCONNECT:       return "DISCONNE";
        case SHRD_START:            return "START   ";
        case SHRD_END:              return "END     ";
        case SHRD_RESUME:           return "RESUME  ";
        case SHRD_SUSPEND:          return "SUSPEND ";
        case SHRD_RESERVE:          return "RESERVE ";
        case SHRD_RELEASE:          return "RELEASE ";
        case SHRD_READ:             return "READ    ";
        case SHRD_WRITE:            return "WRITE   ";
        case SHRD_SENSE:            return "SENSE   ";
        case SHRD_QUERY:            return "QUERY   ";
        case SHRD_COMPRESS:         return "COMPRESS";

        // Response codes
        case SHRD_OK:               return "OK      ";
        case SHRD_ERROR:            return "ERROR   ";
        case SHRD_IOERR:            return "IOERR   ";
        case SHRD_BUSY:             return "BUSY    ";
        case SHRD_COMP:             return "COMP    ";
        case SHRD_PURGE:            return "PURGE   ";

        // Error responses
        case SHRD_ERROR_INVALID:    return "INVALID ";
        case SHRD_ERROR_BADVERS:    return "BADVERS ";
        case SHRD_ERROR_NOTINIT:    return "NOTINIT ";
        case SHRD_ERROR_NOTCONN:    return "NOTCONN ";
        case SHRD_ERROR_NOTAVAIL:   return "NOTAVAIL";
        case SHRD_ERROR_NOMEM:      return "NOMEM   ";
        case SHRD_ERROR_NOTACTIVE:  return "NOTACTIV";
        case SHRD_ERROR_NODEVICE:   return "NODEVICE";
        case SHRD_ERROR_CONNECTED:  return "ECONNECT";

        default:                    return "????????";
    }
}

/*-------------------------------------------------------------------
 * Process a request (server side)
 *-------------------------------------------------------------------*/
static void serverRequest (DEVBLK *dev, int ix, BYTE *hdr, BYTE *buf)
{
int      rc;                            /* Return code               */
int      i;                             /* Loop index                */
BYTE     cmd;                           /* Header command            */
BYTE     flag;                          /* Header flags              */
U16      devnum;                        /* Header device number      */
int      id;                            /* Header identifier         */
int      len;                           /* Header length             */
int      code;                          /* Response code             */
int      rcd;                           /* Record to read/write      */
int      off;                           /* Offset into record        */
char     trcmsg[32];

    /* Extract header information */
    SHRD_GET_HDR (hdr, cmd, flag, devnum, id, len);
    MSGBUF( trcmsg, "server request [%d]", ix );
    SHRDHDRTRACE( trcmsg, hdr );

    /* Save time of last request (for connection timeout purposes) */
    dev->shrd[ix]->time = time( NULL );

    /* Process the request */
    switch (cmd) {

    case SHRD_CONNECT:
        if (dev->connecting)
        {
            serverError (dev, ix, SHRD_ERROR_NOTINIT, cmd,
                         "device not initialized");
            break;
        }
        if ((flag >> 4) != SHARED_VERSION)
        {
            serverError (dev, ix, SHRD_ERROR_BADVERS, cmd,
                         "shared version mismatch");
            break;
        }
        dev->shrd[ix]->release = flag & 0x0f;
        SHRD_SET_HDR (hdr, 0, (SHARED_VERSION << 4) | SHARED_RELEASE, dev->devnum, id, 2);
        store_hw (buf, id);
        serverSend (dev, ix, hdr, buf, 2);
        break;

    case SHRD_DISCONNECT:
        SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 0);
        serverSend (dev, ix, hdr, NULL, 0);
        dev->shrd[ix]->disconnect = 1;

        OBTAIN_DEVLOCK( dev );
        {
            /* Make the device available if this system active on it */
            if (dev->shioactive == id)
            {
                if (!dev->suspended)
                {
                    dev->busy = 0;
                    dev->shioactive = DEV_SYS_NONE;
                }
                else
                    dev->shioactive = DEV_SYS_LOCAL;
                if (dev->shiowaiters)
                    signal_condition (&dev->shiocond);
            }
        }
        RELEASE_DEVLOCK( dev );
        break;

    case SHRD_START:
    case SHRD_RESUME:

        OBTAIN_DEVLOCK( dev );
        {
            /* If the device is suspended locally then grab it */
            if (dev->shioactive == DEV_SYS_LOCAL && dev->suspended && !dev->reserved)
                dev->shioactive = id;

            /* Check if the device is busy */
            if (dev->shioactive != id && dev->shioactive != DEV_SYS_NONE)
            {
                SHRDTRACE( "server request busy id=%d shioactive=%d reserved=%d",
                        id, dev->shioactive, dev->reserved );
                /* If the 'nowait' bit is on then respond 'busy' */
                if (flag & SHRD_NOWAIT)
                {
                    RELEASE_DEVLOCK( dev );
                    SHRD_SET_HDR (hdr, SHRD_BUSY, 0, dev->devnum, id, 0);
                    serverSend (dev, ix, hdr, NULL, 0);
                    break;
                }

                dev->shrd[ix]->waiting = 1;

                /* Wait while the device is busy by the local system */
                while (dev->shioactive == DEV_SYS_LOCAL && !dev->suspended)
                {
                    dev->shiowaiters++;
                    wait_condition (&dev->shiocond, &dev->lock);
                    dev->shiowaiters--;
                }

                /* Return with the 'waiting' bit on if busy by a remote system */
                if (dev->shioactive != DEV_SYS_NONE && dev->shioactive != DEV_SYS_LOCAL)
                {
                    RELEASE_DEVLOCK( dev );
                    break;
                }

                dev->shrd[ix]->waiting = 0;
            }

            /* Make this system active on the device */
            dev->shioactive = id;
            dev->busy = 1;
            sysblk.shrdcount++;
            SHRDTRACE( "server request active id=%d", id );

            /* Increment excp count */
            dev->excps++;
        }
        RELEASE_DEVLOCK( dev );

        /* Call the i/o start or resume exit */
        if (cmd == SHRD_START && dev->hnd->start)
            (dev->hnd->start) (dev);
        else if (cmd == SHRD_RESUME && dev->hnd->resume)
            (dev->hnd->resume) (dev);

        /* Get the purge list */
        if (dev->shrd[ix]->purgen == 0)
            code = len = 0;
        else
        {
            code = SHRD_PURGE;
            if (dev->shrd[ix]->purgen < 0)
                len = 0;
            else
                len = 4 * dev->shrd[ix]->purgen;
        }

        /* Send the response */
        SHRD_SET_HDR (hdr, code, 0, dev->devnum, id, len);
        rc = serverSend (dev, ix, hdr, (BYTE *)dev->shrd[ix]->purge, len);
        if (rc >= 0)
            dev->shrd[ix]->purgen = 0;
        break;

    case SHRD_END:
    case SHRD_SUSPEND:
        /* Must be active on the device for this command */
        if (dev->shioactive != id)
        {
            serverError (dev, ix, SHRD_ERROR_NOTACTIVE, cmd,
                         "not active on this device");
            break;
        }

        /* Call the I/O end/suspend exit */
        if (cmd == SHRD_END && dev->hnd->end)
            (dev->hnd->end) (dev);
        else if (cmd == SHRD_SUSPEND && dev->hnd->suspend)
            (dev->hnd->suspend) (dev);

        OBTAIN_DEVLOCK( dev );
        {
            /* Make the device available if it's not reserved */
            if (!dev->reserved)
            {
                /* If locally suspended then return the device to local */
                if (dev->suspended)
                {
                    dev->shioactive = DEV_SYS_LOCAL;
                    dev->busy = 1;
                }
                else
                {
                    dev->shioactive = DEV_SYS_NONE;
                    dev->busy = 0;
                }

                /* Reset any 'waiting' bits */
                for (i = 0; i < SHARED_MAX_SYS; i++)
                    if (dev->shrd[i])
                        dev->shrd[i]->waiting = 0;

                /* Notify any waiters */
                if (dev->shiowaiters)
                    signal_condition (&dev->shiocond);
            }
            SHRDTRACE( "server request inactive id=%d", id );
        }
        RELEASE_DEVLOCK( dev );

        /* Send response back */
        SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 0);
        serverSend (dev, ix, hdr, NULL, 0);
        break;

    case SHRD_RESERVE:
        /* Must be active on the device for this command */
        if (dev->shioactive != id)
        {
            serverError (dev, ix, SHRD_ERROR_NOTACTIVE, cmd,
                         "not active on this device");
            break;
        }

        OBTAIN_DEVLOCK( dev );
        {
            dev->reserved = 1;
        }
        RELEASE_DEVLOCK( dev );

        SHRDTRACE( "server request reserved id=%d", id );

        /* Call the I/O reserve exit */
        if (dev->hnd->reserve) (dev->hnd->reserve) (dev);

        /* Send response back */
        SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 0);
        serverSend (dev, ix, hdr, NULL, 0);

        break;

    case SHRD_RELEASE:
        /* Must be active on the device for this command */
        if (dev->shioactive != id)
        {
            serverError (dev, ix, SHRD_ERROR_NOTACTIVE, cmd,
                         "not active on this device");
            break;
        }

        /* Call the I/O release exit */
        if (dev->hnd->release) (dev->hnd->release) (dev);

        OBTAIN_DEVLOCK( dev );
        {
            dev->reserved = 0;
        }
        RELEASE_DEVLOCK( dev );

        SHRDTRACE( "server request released id=%d", id );

        /* Send response back */
        SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 0);
        serverSend (dev, ix, hdr, NULL, 0);

        break;

    case SHRD_READ:
        /* Must be active on the device for this command */
        if (dev->shioactive != id)
        {
            serverError (dev, ix, SHRD_ERROR_NOTACTIVE, cmd,
                         "not active on this device");
            break;
        }

        /* Set the compressions client is willing to accept */
        dev->comps = dev->shrd[ix]->comps;
        dev->comp = dev->compoff = 0;

        /* Call the I/O read exit */
        rcd = (int)fetch_fw (buf);
        rc = (dev->hnd->read) (dev, rcd, &flag);
        SHRDTRACE( "server request read rcd %d flag %2.2x rc=%d",
                rcd, flag, rc );

        if (rc < 0)
            code = SHRD_IOERR;
        else
        {
            code = dev->comp ? SHRD_COMP : 0;
            flag = (dev->comp << 4) | dev->compoff;
        }

        /* Reset compression stuff */
        dev->comps = dev->comp = dev->compoff = 0;

        SHRD_SET_HDR (hdr, code, flag, dev->devnum, id, dev->buflen);
        serverSend (dev, ix, hdr, dev->buf, dev->buflen);

        break;

    case SHRD_WRITE:
        /* Must be active on the device for this command */
        if (dev->shioactive != id)
        {
            serverError (dev, ix, SHRD_ERROR_NOTACTIVE, cmd,
                         "not active on this device");
            break;
        }

        /* Call the I/O write exit */
        off = fetch_hw (buf);
        rcd = fetch_fw (buf + 2);

        rc = (dev->hnd->write) (dev, rcd, off, buf + 6, len - 6, &flag);
        SHRDTRACE( "server request write rcd %d off %d len %d flag %2.2x rc=%d",
                rcd, off, len - 6, flag, rc );

        if (rc < 0)
            code = SHRD_IOERR;
        else
            code = 0;

        /* Send response back */
        SHRD_SET_HDR (hdr, code, flag, dev->devnum, id, 0);
        serverSend (dev, ix, hdr, NULL, 0);

        break;

    case SHRD_SENSE:
        /* Must be active on the device for this command */
        if (dev->shioactive != id)
        {
            serverError (dev, ix, SHRD_ERROR_NOTACTIVE, cmd,
                         "not active on this device");
            break;
        }

        /* Send the sense */
        SHRD_SET_HDR (hdr, 0, CSW_CE | CSW_DE, dev->devnum, id, dev->numsense);
        serverSend (dev, ix, hdr, dev->sense, dev->numsense);
        memset (dev->sense, 0, sizeof(dev->sense));
        dev->sns_pending = 0;
        break;

    case SHRD_QUERY:
        switch (flag) {

        case SHRD_USED:
            if (dev->hnd->used)
                rc = (dev->hnd->used) (dev);
            else
                rc = 0;
            store_fw (buf, rc);
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 4);
            serverSend (dev, ix, hdr, buf, 4);
            break;

        case SHRD_DEVCHAR:
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, dev->numdevchar);
            serverSend (dev, ix, hdr, dev->devchar, dev->numdevchar);
            break;

        case SHRD_DEVID:
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, dev->numdevid);
            serverSend (dev, ix, hdr, dev->devid, dev->numdevid);
            break;

        case SHRD_SERIAL:
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, sizeof( dev->serial ));
            serverSend (dev, ix, hdr, dev->serial, (U32)sizeof( dev->serial ));
            break;

        case SHRD_CKDCYLS:
            store_fw (buf, dev->ckdcyls);
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 4);
            serverSend (dev, ix, hdr, buf, 4);
            break;

        case SHRD_FBAORIGIN:
            store_dw( buf, dev->fbaorigin );
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 4);
            serverSend (dev, ix, hdr, buf, 4);
            break;

        case SHRD_FBANUMBLK:
            store_fw (buf, dev->fbanumblk);
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 4);
            serverSend (dev, ix, hdr, buf, 4);
            break;

        case SHRD_FBABLKSIZ:
            store_fw (buf, dev->fbablksiz);
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 4);
            serverSend (dev, ix, hdr, buf, 4);
            break;

        default:
            serverError (dev, ix, SHRD_ERROR_INVALID, cmd,
                         "invalid query request");
            break;
        } /* switch (flag) for SHRD_QUERY */
        break;

    case SHRD_COMPRESS:
#if defined( HAVE_ZLIB )
        dev->shrd[ix]->comp = (flag & 0x0f);
        store_hw (buf, dev->shrd[ix]->comp);
#else
        store_hw (buf, 0);
#endif
        dev->shrd[ix]->comps = (flag & 0xf0) >> 4;
        SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 2);
        serverSend (dev, ix, hdr, buf, 2);
        break;

    default:
        serverError (dev, ix, SHRD_ERROR_INVALID, cmd,
                     "invalid request");
        break;
    } /* switch (cmd) */
} /* serverRequest */

/*-------------------------------------------------------------------
 * Locate the SHRD block for a socket (server side). Returns index.
 *-------------------------------------------------------------------*/
static int serverLocate( DEVBLK* dev, int id, int* avail )
{
    int i;

    if (avail)
        *avail = -1;

    for (i=0; i < SHARED_MAX_SYS; i++)
    {
        // Active slot?
        if (dev->shrd[i])
        {
            // Is this the one they want?
            if (dev->shrd[i]->id == id)
                return i;
        }
        else // (available slot)
        {
            if (avail && *avail < 0)
                *avail = i;
        }
    }

    return -1;
}

/*-------------------------------------------------------------------
 * Return a new Identifier (server side)
 *-------------------------------------------------------------------*/
static int serverId (DEVBLK *dev)
{
int      i;                             /* Loop index                */
int      id;                            /* Identifier                */

    do {
        dev->shrdid += 1;
        dev->shrdid &= 0xffff;
        if (dev->shrdid == DEV_SYS_LOCAL
         || dev->shrdid == DEV_SYS_NONE)
            dev->shrdid = 1;
        id = dev->shrdid;

        for (i = 0; i < SHARED_MAX_SYS; i++)
            if (dev->shrd[i] && dev->shrd[i]->id == id)
                break;

    } while (i < SHARED_MAX_SYS);

    return id;
} /* serverId */

/*-------------------------------------------------------------------
 * Respond with an error message (server side)
 *-------------------------------------------------------------------*/
static int serverError (DEVBLK *dev, int ix, int code, int status,
                        char *msg)
{
    int rc;                             /* Return code               */
    size_t len;                         /* Message length            */
    BYTE hdr[SHRD_HDR_SIZE];            /* Header                    */

    /* Get message length */
    len = strlen(msg) + 1;
    if (len > SHARED_MAX_MSGLEN)
        len = SHARED_MAX_MSGLEN;

    SHRD_SET_HDR( hdr, code, status, dev ? dev->devnum : 0,
                  ix < 0 ? 0 : dev->shrd[ix]->id, (U16) len );

    SHRDTRACE( "SERVER ERROR! %2.2x %2.2x: %s", code, status, msg );

    rc = serverSend( dev, ix, hdr, (BYTE*) msg, (int) len );
    return rc;

} /* serverError */

/*-------------------------------------------------------------------
 * Send data (server side)
 *-------------------------------------------------------------------*/
static int serverSend (DEVBLK *dev, int ix, BYTE *hdr, BYTE *buf,
                       int buflen)
{
int      rc;                            /* Return code               */
int      sock;                          /* Socket number             */
BYTE     code;                          /* Header code               */
BYTE     status;                        /* Header status             */
U16      devnum;                        /* Header device number      */
int      id;                            /* Header identifier         */
int      len;                           /* Header length             */
int      hdrlen;                        /* Header length + other data*/
BYTE    *sendbuf = NULL;                /* Send buffer               */
int      sendlen;                       /* Send length               */
BYTE     cbuf[SHRD_HDR_SIZE + 65536];   /* Combined buffer           */

    /* Make buf, buflen consistent if no additional data to be sent  */
    if (buf == NULL) buflen = 0;
    else if (buflen == 0) buf = NULL;

    /* Calculate length of header, may contain additional data */
    SHRD_GET_HDR(hdr, code, status, devnum, id, len);
    hdrlen = SHRD_HDR_SIZE + (len - buflen);
    sendlen = hdrlen + buflen;

    /* Check if buf is adjacent to the header */
    if (buf && hdr + hdrlen == buf)
    {
        hdrlen += buflen;
        buf = NULL;
        buflen = 0;
    }

    /* Send only the header buffer if 'buf' is empty */
    if (buflen == 0)  sendbuf = hdr;

    /* Get socket number; if 'ix' < 0 we don't have a device yet */
    if (ix >= 0)
        sock = dev->shrd[ix]->fd;
    else
    {
        sock = -ix;
        dev = NULL;
    }

    SHRDHDRTRACE( "server send", hdr );

#if defined( HAVE_ZLIB )
    /* Compress the buf */
    if (ix >= 0 && dev->shrd[ix]->comp != 0
     && code == SHRD_OK && status == 0
     && hdrlen - SHRD_HDR_SIZE <= SHRD_COMP_MAX_OFF
     && buflen >= SHARED_COMPRESS_MINLEN)
    {
        unsigned long newlen;
        int off = hdrlen - SHRD_HDR_SIZE;
        sendbuf = cbuf;
        newlen = sizeof(cbuf) - hdrlen;
        memcpy (cbuf, hdr, hdrlen);
        rc = compress2 (cbuf + hdrlen, &newlen,
                        buf, buflen, dev->shrd[ix]->comp);
        if (rc == Z_OK && (int)newlen < buflen)
        {
            /* Setup to use the compressed buffer */
            sendlen = hdrlen + newlen;
            buflen = 0;
            code = SHRD_COMP;
            status = (SHRD_LIBZ << 4) | off;
            SHRD_SET_HDR (cbuf, code, status, devnum, id, (U16)(newlen + off));
            SHRDHDRTRACE2( "server send", cbuf, "(compressed)" );
        }
    }
#endif

    /* Build combined (hdr + data) buffer */
    if (buflen > 0)
    {
        sendbuf = cbuf;
        memcpy (cbuf, hdr, hdrlen);
        memcpy (cbuf + hdrlen, buf, buflen);
    }

    /* Send the combined header and data */
    rc = send (sock, sendbuf, sendlen, 0);

    /* Process return code */
    if (rc < 0)
    {
        // "%1d:%04X Shared: error in send id %d: %s"
        WRMSG( HHC00729, "E", LCSS_DEVNUM, id, strerror( HSO_errno ));
        dev->shrd[ix]->disconnect = 1;
    }

    return rc;

} /* serverSend */

/*-------------------------------------------------------------------
 * Determine if a client can be disconnected (server side)
 *-------------------------------------------------------------------*/
static bool serverDisconnectable( DEVBLK* dev, int ix )
{
    return
    (1
        && !dev->shrd[ix]->waiting
        && !dev->shrd[ix]->pending
        && dev->shioactive != dev->shrd[ix]->id
    );
}

/*-------------------------------------------------------------------
 * Disconnect a client (server side)
 * dev->lock *must* be held
 *-------------------------------------------------------------------*/
static void serverDisconnect (DEVBLK *dev, int ix)
{
    int id;                             /* Client identifier         */
    int i;                              /* Loop index                */

    id = dev->shrd[ix]->id;

//FIXME: Handle a disconnected busy client better
//       Perhaps a disconnect timeout value... this will
//       give the client time to reconnect.

    /* If the device is active by the client then extricate it.
       This is *not* a good situation */
    if (dev->shioactive == id)
    {
        // "%1d:%04X Shared: busy client being removed id %d %s"
        WRMSG( HHC00730, "W", LCSS_DEVNUM, id, dev->reserved ? "reserved" : "" );

        /* Call the I/O release exit if reserved by this client */
        if (dev->reserved && dev->hnd->release)
            (dev->hnd->release) (dev);

        /* Call the channel program end exit */
        if (dev->hnd->end)
            (dev->hnd->end) (dev);

        /* Reset any 'waiting' bits */
        for (i = 0; i < SHARED_MAX_SYS; i++)
            if (dev->shrd[i])
                dev->shrd[i]->waiting = 0;

        /* Make the device available */
        if (dev->suspended) {
            dev->shioactive = DEV_SYS_LOCAL;
            dev->busy = 1;
        }
        else
        {
            dev->shioactive = DEV_SYS_NONE;
            dev->busy = 0;
        }

        /* Notify any waiters */
        if (dev->shiowaiters)
            signal_condition (&dev->shiocond);
    }

    if (MLVL( VERBOSE ))
        // "%1d:%04X Shared: %s disconnected id %d"
        WRMSG( HHC00731, "I", LCSS_DEVNUM, dev->shrd[ix]->ipaddr, id );

    /* Release the SHRD block */
    close_socket (dev->shrd[ix]->fd);
    free (dev->shrd[ix]->ipaddr);
    free (dev->shrd[ix]);
    dev->shrd[ix] = NULL;

    dev->shrdconn--;
} /* serverDisconnect */

/*-------------------------------------------------------------------
 * Return client ip
 *-------------------------------------------------------------------*/
static char *clientip (int sock)
{
int                     rc;             /* Return code               */
struct sockaddr_in      client;         /* Client address structure  */
socklen_t               namelen;        /* Length of client structure*/

    namelen = sizeof(client);
    rc = getpeername (sock, (struct sockaddr *)&client, &namelen);
    return inet_ntoa(client.sin_addr);

} /* clientip */

/*-------------------------------------------------------------------
 * Find device by device number
 *-------------------------------------------------------------------*/
static DEVBLK *findDevice (U16 devnum)
{
DEVBLK      *dev;                       /* -> Device block           */

    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        if (dev->devnum == devnum) break;
    return dev;

} /* findDevice */

/*-------------------------------------------------------------------
 * Connect a new client     This is essentially the device thread
 *-------------------------------------------------------------------*/
static void *serverConnect (void *psock)
{
int             csock;                  /* Connection socket         */
int             rc;                     /* Return code               */
BYTE            cmd;                    /* Request command           */
BYTE            flag;                   /* Request flag              */
U16             devnum;                 /* Request device number     */
int             id;                     /* Request id                */
int             len;                    /* Request data length       */
int             ix;                     /* Client index              */
DEVBLK         *dev=NULL;               /* -> Device block           */
time_t          now;                    /* Current time              */
fd_set          selset;                 /* Read bit map for select   */
int             maxfd;                  /* Max fd for select         */
struct timeval  wait = {0};             /* Wait time for select      */
BYTE            hdr[SHRD_HDR_SIZE + 65536];  /* Header + buffer      */
BYTE           *buf = hdr + SHRD_HDR_SIZE;   /* Buffer               */
char           *ipaddr = NULL;          /* IP addr of connected peer */
char            threadname[16] = {0};
static const bool server_req = true;

    // We are (or will be) the "dev->shrdtid" thread...

    csock = *(int*)psock;
    free( psock );
    ipaddr = clientip( csock );

    SHRDTRACE( "server connect %s sock %d", ipaddr, csock );

    rc = recvData( csock, hdr, buf, 65536, server_req );
    if (rc < 0)
    {
        // "Shared: connect to IP %s failed"
        WRMSG( HHC00732, "E", ipaddr );
        close_socket( csock );
        return NULL;
    }

    SHRD_GET_HDR( hdr, cmd, flag, devnum, id, len );

    /* Error if not a connect request */
    if (id == 0 && cmd != SHRD_CONNECT)
    {
        serverError( NULL, -csock, SHRD_ERROR_NOTCONN, cmd,
                     "not a connect request" );
        close_socket( csock );
        return NULL;
    }

    /* Locate the device */
    if (!(dev = findDevice( devnum )))
    {
        serverError( NULL, -csock, SHRD_ERROR_NODEVICE, cmd,
                     "device not found" );
        close_socket( csock );
        return NULL;
    }

    /* Obtain the device lock */
    OBTAIN_DEVLOCK( dev );

    /* Find an available slot for the connection */
    if ((rc = serverLocate( dev, id, &ix )) >= 0)
    {
        RELEASE_DEVLOCK( dev );
        serverError( NULL, -csock, SHRD_ERROR_NODEVICE, cmd,
                     "already connected" );
        close_socket( csock );
        return NULL;
    }

    /* Error if no available slot */
    if (ix < 0)
    {
        RELEASE_DEVLOCK( dev );
        serverError( NULL, -csock, SHRD_ERROR_NOTAVAIL, cmd,
                     "too many connections" );
        close_socket( csock );
        return NULL;
    }

    /* Obtain SHRD block */
    if (!(dev->shrd[ix] = calloc( sizeof( SHRD ), 1 )))
    {
        RELEASE_DEVLOCK( dev );
        serverError( NULL, -csock, SHRD_ERROR_NOMEM, cmd,
                     "calloc() failure" );
        close_socket( csock );
        return NULL;
    }

    /* Initialize the SHRD block */
    dev->shrd[ix]->pending = 1;
    dev->shrd[ix]->havehdr = 1;
    if (id == 0) id = serverId( dev );
    dev->shrd[ix]->id      = id;
    dev->shrd[ix]->fd      = csock;
    dev->shrd[ix]->ipaddr  = strdup(ipaddr);
    dev->shrd[ix]->time    = time (NULL);
    dev->shrd[ix]->purgen  = -1;
    dev->shrdconn++;
    SHRD_SET_HDR( dev->shrd[ix]->hdr, cmd, flag, devnum, id, len );

    if (MLVL( VERBOSE ))
        // "%1d:%04X Shared: %s connected id %d"
        WRMSG( HHC00733, "I", LCSS_DEVNUM, ipaddr, id );

    /* Return if device thread already active */
    if (dev->shrdtid)
    {
        RELEASE_DEVLOCK( dev );
        return NULL;
    }

    /* This thread will be the shared device thread (dev->shrdtid) */

    dev->shrdtid = thread_id();

    MSGBUF( threadname, "shrd dev %1d:%04X", LCSS_DEVNUM );
    if (MLVL( VERBOSE ))
        LOG_THREAD_BEGIN( threadname  );

    /* Keep looping while there are still clients connected to our device */
    /* PROGRAMMING NOTE: the below loop runs with the device lock held! */
    while (dev->shrdconn)
    {
        FD_ZERO( &selset );
        maxfd = -1;

        /* Get the current time */
        now = time( NULL );

        /* For each remote system connected to our device... */
        for (ix = 0; ix < SHARED_MAX_SYS; ix++)
        {
            /* Is there a client at this slot? */
            if (dev->shrd[ix])
            {
                /* Stop as soon as we find a pending request */
                if (dev->shrd[ix]->pending && !dev->shrd[ix]->waiting)
                    break; // (go process pending request)

                /* Disconnect if not a valid socket */
                if (!socket_is_socket( dev->shrd[ix]->fd ))
                    dev->shrd[ix]->disconnect = 1;

                /* See if the connection can be timed out */
                else if (1
                    && (now - dev->shrd[ix]->time) > SHARED_TIMEOUT
                    && serverDisconnectable( dev, ix )
                )
                    dev->shrd[ix]->disconnect = 1;

                /* Disconnect if the disconnect bit is set */
                if (dev->shrd[ix]->disconnect)
                    serverDisconnect( dev, ix );

                /* Otherwise set the fd if not waiting */
                else if (!dev->shrd[ix]->waiting)
                {
                    FD_SET( dev->shrd[ix]->fd, &selset );

                    if (dev->shrd[ix]->fd >= maxfd)
                        maxfd = dev->shrd[ix]->fd + 1;

                    SHRDTRACE( "select set %d id=%d",
                        dev->shrd[ix]->fd, dev->shrd[ix]->id );
                }
            }
        }

        /* Wait for request if all pending requests were processed */
        if (ix >= SHARED_MAX_SYS)
        {
            /* If our select set is empty, there are not any clients
               connected to our device (and dev->shrdconn SHOULD now
               be zero) and thus we have nothing to do. If we don't
               have any clients connected to our device then we are
               serving no purpose so we should just exit our thread.
            */
            if (maxfd < 0)      // (if nothing to select)
                continue;       // (then exit our thread)

            /* Wait for a client to send us a request */
            wait.tv_sec = 0;
            wait.tv_usec = SHARED_SELECT_WAIT_MSECS * 1000;

            RELEASE_DEVLOCK( dev );
            {
                dev->shrdwait = 1;
                {
                    rc = select( maxfd, &selset, NULL, NULL, &wait );
                }
                dev->shrdwait = 0;

                SHRDTRACE("serverConnect: select rc %d", rc );
            }
            OBTAIN_DEVLOCK( dev );

            /* Timeout; no one has any requests for us at this time */
            if (rc == 0)
                continue;

            /* Did the select fail? */
            if (rc < 0 )
            {
                /* Try again if temporary error */
                if (HSO_errno == HSO_EINTR || HSO_errno == HSO_EBADF)
                    continue;

                // "Shared: error in function %s: %s"
                WRMSG( HHC00735, "E", "select()", strerror( HSO_errno ));
                break; // (exit our thread)
            }

            /* One or more sockets in our select set is ready, meaning
               one or more remote clients connected to our device has
               a request it needs us to process. Loop through our list
               of connected clients and set the 'pending' request bit
               for each one whose socket that select said is FD_ISSET.
            */
            for (ix = 0; ix < SHARED_MAX_SYS; ix++)
            {
                if (1
                    /* Is there a client at this slot? */
                    && dev->shrd[ix]
                    /* Did select() say its socket is set? */
                    && FD_ISSET( dev->shrd[ix]->fd, &selset )
                )
                {
                    /* Indicate this client has a pending request */
                    dev->shrd[ix]->pending = 1;
                    SHRDTRACE("select isset %d id=%d",
                        dev->shrd[ix]->fd, dev->shrd[ix]->id );
                }
            }
            continue; // (re-iterate to process all pending requests)
        }

        /* Found a pending request */
        RELEASE_DEVLOCK( dev );
        {
            SHRDTRACE("select ready %d id=%d",
                dev->shrd[ix]->fd, dev->shrd[ix]->id );

            if (dev->shrd[ix]->havehdr)
            {
                /* Copy the saved start/resume packet */
                memcpy( hdr, dev->shrd[ix]->hdr, SHRD_HDR_SIZE );
                dev->shrd[ix]->havehdr = 0;
                dev->shrd[ix]->waiting = 0;
            }
            else
            {
                /* Read the request packet */
                if ((rc = recvData( dev->shrd[ix]->fd, hdr, buf, 65536, 1 )) < 0)
                {
                    // "%1d:%04X Shared: error in receive from %s id %d"
                    WRMSG( HHC00734, "E", LCSS_DEVNUM, dev->shrd[ix]->ipaddr, dev->shrd[ix]->id );
                    dev->shrd[ix]->disconnect = 1;
                    dev->shrd[ix]->pending = 0;
                    OBTAIN_DEVLOCK( dev );
                    continue;
                }
            }

            /* Process the request */
            serverRequest( dev, ix, hdr, buf );
        }
        OBTAIN_DEVLOCK( dev );

        /* If the 'waiting' bit is on then the start/resume request
           failed because the device is busy on some other remote
           system.  We only need to save the header because the data
           is ignored for start/resume.
        */
        if (dev->shrd[ix]->waiting)
        {
            memcpy( dev->shrd[ix]->hdr, hdr, SHRD_HDR_SIZE );
            dev->shrd[ix]->havehdr = 1;
        }
        else
            dev->shrd[ix]->pending = 0;

    } /* while (dev->shrdconn) */

    dev->shrdtid = 0;
    RELEASE_DEVLOCK( dev );

    if (MLVL( VERBOSE ))
        LOG_THREAD_END( threadname  );

    return NULL;

} /* serverConnect */

/*-------------------------------------------------------------------
 * Trace routine for tracing SHRD_HDR
 *-------------------------------------------------------------------*/
static void shrdhdrtrc( DEVBLK* dev, const char* msg, const BYTE* hdr,
                                     const char* msg2 )
{
    BYTE cmd, code; U16 devnum; int id, len; char buf[4];
    SHRD_GET_HDR( hdr, cmd, code, devnum, id, len );
    MSGBUF( buf, "%2.2x", cmd );
    SHRDTRACE( "%-18s : %s(%2.2x) %2.2x dev %4.4x id %d len %d%s%s",
        msg, shrdcmd2str( cmd ), cmd, code, devnum, id, len,
        msg2 ? " " : "", msg2 ? msg2 : "" );
}

/*-------------------------------------------------------------------
 * General trace routine for shared devices
 *-------------------------------------------------------------------*/
static void shrdtrc( DEVBLK* dev, const char* fmt, ... )
{
    bool            tracing;
    struct timeval  tv;
    SHRD_TRACE      tracemsg;
    va_list         vl;
    char            buf[32];

    /* If the device is being traced or stepped, we also print a
       trace message (WITHOUT a timestamp) directly to the panel.
       Otherwise we build a trace message WITH a timestamp prefix
       and enter it into out trace table if one exists. If neither
       is true (not tracing or stepping AND no trace table) then
       there's nothing for us to do so we return immediately.
    */
    tracing = (dev && dev->ccwtrace);

    OBTAIN_SHRDTRACE_LOCK();

    if (!tracing && !sysblk.shrdtrace)
    {
        RELEASE_SHRDTRACE_LOCK();
        return;  // (nothing for us to do!)
    }

    ASSERT( tracing || sysblk.shrdtrace );

    /* Build the timestamp portion of the trace message */
    gettimeofday( &tv, NULL );
    FormatTIMEVAL( &tv, buf, sizeof( buf ));
    STRLCPY( tracemsg, buf + 11 ); // (skip "YYYY-MM-DD ")

    /* Append the devnum to the trace message */
    MSGBUF( buf, "  %4.4X  ", dev ? dev->devnum : 0 );
    STRLCAT( tracemsg, buf );

    /* Now format the rest of the trace message following that part */
    va_start( vl, fmt );
    vsnprintf( (char*) tracemsg + strlen( tracemsg ),
        sizeof( tracemsg ) - strlen( tracemsg ), fmt, vl );

    /* Log the trace message directly to the panel (WITHOUT the
       timestamp prefix) if the device is being traced/stepped. */
    if (tracing)
        // "Shared:  %s"
        WRMSG( HHC00743, "I", tracemsg + 16 ); // (skip "HH:MM:SS.uuuuuu ")

    /* Copy the trace message into the trace table (if it exists) */
    shrdtrclog_locked( tracemsg );

    RELEASE_SHRDTRACE_LOCK();
}

/*-------------------------------------------------------------------
 * General non-device-specific trace routine
 *-------------------------------------------------------------------*/
static void shrdgentrc( const char* fmt, ... )
{
    struct timeval  tv;
    SHRD_TRACE      tracemsg;
    va_list         vl;
    char            buf[32];

    OBTAIN_SHRDTRACE_LOCK();

    if (!sysblk.shrdtrace)
    {
        RELEASE_SHRDTRACE_LOCK();
        return;  // (nothing for us to do!)
    }

    ASSERT( sysblk.shrdtrace );

    /* Build the timestamp portion of the trace message */
    gettimeofday( &tv, NULL );
    FormatTIMEVAL( &tv, buf, sizeof( buf ));
    STRLCPY( tracemsg, buf + 11 ); // (skip "YYYY-MM-DD ")

    /* Now format the rest of the trace message following that part */
    va_start( vl, fmt );
    vsnprintf( (char*) tracemsg + strlen( tracemsg ),
        sizeof( tracemsg ) - strlen( tracemsg ), fmt, vl );

    /* Copy the trace message into the trace table (if it exists) */
    shrdtrclog_locked( tracemsg );

    RELEASE_SHRDTRACE_LOCK();
}

/*-------------------------------------------------------------------
 * Add the trace message to the trace table (lock held)
 *-------------------------------------------------------------------*/
static void shrdtrclog_locked( const char* tracemsg )
{
    /* Copy the trace message into the trace table (if it exists) */
    if (sysblk.shrdtrace)
    {
        /* Grab pointer to next available table entry and then
           bump the pointer to the next entry for next time.
        */
        SHRD_TRACE* currmsg = sysblk.shrdtracep++;

        if (sysblk.shrdtracep >= sysblk.shrdtracex)
            sysblk.shrdtracep  = sysblk.shrdtrace;

        /* Copy message (WITH timestamp) into trace table */
        strlcpy( (char*) currmsg, (const char*) tracemsg, sizeof( SHRD_TRACE ));
    }
}

/*-------------------------------------------------------------------
 *              shutdown_shared_server
 *-------------------------------------------------------------------*/
DLL_EXPORT void shutdown_shared_server( void* unused )
{
    OBTAIN_SHRDLOCK();
    {
        shutdown_shared_server_locked( unused );
    }
    RELEASE_SHRDLOCK();
}

/*-------------------------------------------------------------------
 *              shutdown_shared_server_locked
 *-------------------------------------------------------------------*/
DLL_EXPORT void shutdown_shared_server_locked( void* unused )
{
    UNREFERENCED( unused );

    if (sysblk.shrdport || sysblk.shrdtid)
    {
        sysblk.shrdport = 0;

        if (sysblk.shrdtid)
            wait_condition( &sysblk.shrdcond, &sysblk.shrdlock );
    }
    ASSERT( !sysblk.shrdport && !sysblk.shrdtid );
}

/*-------------------------------------------------------------------
 * Shared device server thread: accept connect from remote client
 *-------------------------------------------------------------------*/
DLL_EXPORT void* shared_server( void* arg )
{
bool                    shutdown=false; /* shutdown flag             */
int                     rc = -32767;    /* Return code               */
int                     hi;             /* Hi fd for select          */
int                     lsock;          /* inet socket for listening */
int                     usock;          /* unix socket for listening */
int                     rsock;          /* Ready socket              */
int                     csock;          /* Socket for conversation   */
int                    *psock;          /* Pointer to socket         */
struct sockaddr_in      server;         /* Server address structure  */
#if defined( HAVE_SYS_UN_H )
struct sockaddr_un      userver;        /* Unix address structure    */
#endif
int                     optval;         /* Argument for setsockopt   */
fd_set                  selset;         /* Read bit map for select   */
TID                     tid;            /* Negotiation thread id     */
char                    threadname[16] = {0};
struct timeval          timeout = {0};

    // We are the "sysblk.shrdtid" thread...

    UNREFERENCED( arg );

    /* Display thread started message on control panel */
    MSGBUF( threadname, "shrd srvr %d.%d", SHARED_VERSION, SHARED_RELEASE );
    LOG_THREAD_BEGIN( threadname  );

    /* Obtain a internet socket */
    if ((lsock = socket( AF_INET, SOCK_STREAM, 0 )) < 0)
    {
        // "Shared: error in function %s: %s"
        WRMSG( HHC00735, "E", "socket()", strerror( HSO_errno ));
        return NULL;
    }

    /* Obtain a unix socket */
#if defined( HAVE_SYS_UN_H )
    if ((usock = socket( AF_UNIX, SOCK_STREAM, 0 )) < 0)
    {
        // "Shared: error in function %s: %s"
        WRMSG( HHC00735, "W", "socket()", strerror( HSO_errno ));
    }
#else
    usock = -1;
#endif

    /* Allow previous instance of socket to be reused */
    optval = 1;
    setsockopt( lsock, SOL_SOCKET, SO_REUSEADDR,
                (GETSET_SOCKOPT_T*) &optval, sizeof( optval ));

    /* Prepare the sockaddr structure for the bind */
    memset( &server, 0, sizeof(server) );
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port        = htons( sysblk.shrdport );

    /* Attempt to bind the internet socket to the port */
    while (sysblk.shrdport)
    {
        rc = bind( lsock, (struct sockaddr*) &server, sizeof( server ));
        if (rc == 0 || HSO_errno != HSO_EADDRINUSE)
            break;
        // "Shared: waiting for port %u to become free"
        WRMSG( HHC00736, "W", sysblk.shrdport );
        SLEEP( 10 );
    } /* end while */

    if (rc != 0 || !sysblk.shrdport)
    {
        // "Shared: error in function %s: %s"
        WRMSG( HHC00735, "E", "bind()", strerror( HSO_errno ));
        close_socket( lsock );
        close_socket( usock );
        return NULL;
    }

#if defined( HAVE_SYS_UN_H )
    /* Bind the unix socket */
    if (usock >= 0)
    {
        userver.sun_family = AF_UNIX;
        sprintf( userver.sun_path, "/tmp/hercules_shared.%d", sysblk.shrdport );
        unlink( userver.sun_path );
        fchmod( usock, 0700 );

        rc = bind( usock, (struct sockaddr*) &userver, sizeof( userver ));

        if (rc < 0)
        {
            // "Shared: error in function %s: %s"
            WRMSG( HHC00735, "W", "bind()", strerror( HSO_errno ));
            close( usock );
            usock = -1;
        }
    }
#endif // defined( HAVE_SYS_UN_H )

    /* Put the sockets into listening state */
    rc = listen( lsock, SHARED_MAX_SYS );

    if (rc < 0)
    {
        // "Shared: error in function %s: %s"
        WRMSG( HHC00735, "E", "listen()", strerror( HSO_errno ));
        close_socket( lsock );
        close_socket( usock );
        return NULL;
    }

    if (usock >= 0)
    {
        rc = listen( usock, SHARED_MAX_SYS );

        if (rc < 0)
        {
            // "Shared: error in function %s: %s"
            WRMSG( HHC00735, "W", "listen()", strerror( HSO_errno ));
            close_socket( usock );
            usock = -1;
        }
    }

    csock = -1;

    if (lsock < usock)
        hi = usock + 1;
    else
        hi = lsock + 1;

    // "Shared: waiting for shared device requests on port %u"
    WRMSG( HHC00737, "I", sysblk.shrdport );

    /* Define shared server thread shutdown routine */
    hdl_addshut( "shutdown_shared_server", shutdown_shared_server, NULL );

    /* Handle connection requests and attention interrupts */
    while (1)
    {
        /* Continue running (looping) as long as shrdport is defined */
        OBTAIN_SHRDLOCK();
        {
            shutdown = (sysblk.shrdport ? false : true);
        }
        RELEASE_SHRDLOCK();

        if (shutdown)
            break;

        /* Initialize the select parameters */
        FD_ZERO( &selset );
        FD_SET( lsock, &selset );

        if (usock >= 0)
            FD_SET( usock, &selset );

        /* Wait for a file descriptor to become ready */
        timeout.tv_sec  = 0;
        timeout.tv_usec = SHARED_SELECT_WAIT_MSECS * 1000;
        rc = select( hi, &selset, NULL, NULL, &timeout );

        SHRDGENTRACE("shared_server: select rc %d", rc );

        if (rc == 0)
            continue;

        if (rc < 0 )
        {
            if (HSO_errno == HSO_EINTR)
                continue;

            // "Shared: error in function %s: %s"
            WRMSG( HHC00735, "E", "select()", strerror( HSO_errno ));
            break;
        }

        /* If a client connection request has arrived then accept it */
        if (FD_ISSET( lsock, &selset ))
            rsock = lsock;
        else if (usock >= 0 && FD_ISSET( usock, &selset ))
            rsock = usock;
        else
            rsock = -1;

        /* Accept the connection and create conversation socket */
        if (rsock > 0)
        {
            if ((csock = accept( rsock, NULL, NULL )) < 0)
            {
                // "Shared: error in function %s: %s"
                WRMSG( HHC00735, "E", "accept()", strerror( HSO_errno ));
                continue;
            }

            if (!(psock = malloc( sizeof( csock ))))
            {
                char buf[40];
                MSGBUF( buf, "malloc(%d)", (int) sizeof( csock ));
                // "Shared: error in function %s: %s"
                WRMSG( HHC00735, "E", buf, strerror( HSO_errno ));
                close_socket( csock );
                continue;
            }
            *psock = csock;

            /* Create a thread to complete the client connection */
            rc = create_thread( &tid, DETACHED,
                                serverConnect, psock, "serverConnect" );
            if (rc)
            {
                // "Error in function create_thread(): %s"
                WRMSG( HHC00102, "E", strerror( rc ));
                close_socket( csock );
            }

        } /* end if(rsock) */

    } /* end while (1) */

    /* Remove shut entry so we can do a new 'hdl_addshut' next time */
    if (!sysblk.shutdown)
        hdl_delshut( shutdown_shared_server, NULL );

    /* Close the listening sockets */
    close_socket( lsock );

#if defined( HAVE_SYS_UN_H )
    if (usock >= 0)
    {
        close_socket( usock );
        unlink( userver.sun_path );
    }
#endif

    /* Notify "shutdown_shared_server" that we've exited */
    OBTAIN_SHRDLOCK();
    {
        sysblk.shrdtid = 0;
        signal_condition( &sysblk.shrdcond );
    }
    RELEASE_SHRDLOCK();

    LOG_THREAD_END( threadname  );

    return NULL;

} /* end function shared_server */

/*-------------------------------------------------------------------
 * Define number of Shared Device Server trace table entries
 *-------------------------------------------------------------------*/
DLL_EXPORT int shrd_cmd( int argc, char* argv[], char* cmdline )
{
    char buf[256];
    char *kw, *op, c;
    char *strtok_str = NULL;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Get keyword and operand */
    if (0
        ||  argc < 1
        ||  argc > 2
        || (argc > 1 && strlen( argv[1] ) >= _countof( buf ))
    )
    {
        // "Shared: invalid or missing argument"
        WRMSG( HHC00738, "E" );
        return -1;
    }

    if (argc < 2)
    {
        OBTAIN_SHRDTRACE_LOCK();
        {
            MSGBUF( buf, "TRACE=%d DTAX=%d", sysblk.shrdtracen, sysblk.shrddtax );
        }
        RELEASE_SHRDTRACE_LOCK();
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );
        return 0;
    }

    // Format: "SHRD [TRACE[=nnnn]]" where nnnn is #of table entries.
    // Enter the command with no argument to display the current value.
    // Use "shrd trace" by itself to print the current table.

    STRLCPY( buf, argv[1] );

    kw = strtok_r( buf, "=",    &strtok_str );
    op = strtok_r( NULL, " \t", &strtok_str );

    if (!kw)
    {
        // "Shared: invalid or missing keyword"
        WRMSG( HHC00739, "E" );
        return -1;
    }

    if (strcasecmp( kw, "TRACE" ) == 0)
    {
        SHRD_TRACE*  shrdtrace;
        SHRD_TRACE*  shrdtracep;
        SHRD_TRACE*  shrdtracex;
        int          shrdtracen;

        OBTAIN_SHRDTRACE_LOCK();

        shrdtrace  = sysblk.shrdtrace;       // (ptr to beginning of table)
        shrdtracep = sysblk.shrdtracep;      // (ptr to current/next entry)
        shrdtracex = sysblk.shrdtracex;      // (ptr past the end of table)

        /* Print trace table if no TRACE operand was specified */
        if (!op)
        {
            shared_print_trace_table_locked();
            RELEASE_SHRDTRACE_LOCK();
            return 0;
        }

        /* Operand specified: get size of requested table */
        if (sscanf( op, "%d%c", &shrdtracen, &c ) != 1)
        {
            // "Shared: invalid or missing value %s"
            WRMSG( HHC00740, "E", op );
            RELEASE_SHRDTRACE_LOCK();
            return -1;
        }

        /* Free existing table */
        free( sysblk.shrdtrace );
        sysblk.shrdtrace  = NULL;
        sysblk.shrdtracex = NULL;
        sysblk.shrdtracep = NULL;
        sysblk.shrdtracen = 0;

        /* Allocate new table */
        if (shrdtracen > 0)
        {
            if (!(shrdtrace = calloc( (size_t) shrdtracen, sizeof( SHRD_TRACE ))))
            {
                char buf[40];
                MSGBUF( buf, "calloc(%d, %d)", (int) shrdtracen, (int) sizeof( SHRD_TRACE ));
                // "Shared: error in function %s: %s"
                WRMSG( HHC00735, "E", buf, strerror( errno ));
                RELEASE_SHRDTRACE_LOCK();
                return -1;
            }

            sysblk.shrdtracen = shrdtracen;
            sysblk.shrdtracex = shrdtrace + shrdtracen;
            sysblk.shrdtrace  = shrdtrace;
            sysblk.shrdtracep = shrdtrace;
        }

        if (MLVL( VERBOSE ))
        {
            // "%-14s set to %s"
            MSGBUF( buf, "TRACE=%d", sysblk.shrdtracen );
            WRMSG( HHC02204, "I", argv[0], buf );
        }

        RELEASE_SHRDTRACE_LOCK();
        return 0;
    }

    if (strcasecmp( kw, "DTAX" ) == 0)  // Dump Table At Exit
    {
        int dtax;

        if (!op)
        {
            // "Shared: invalid or missing value %s"
            WRMSG( HHC00740, "E", kw );
            return -1;
        }
        if (0
            || sscanf( op, "%d%c", &dtax, &c ) != 1
            || (dtax != 0 && dtax != 1)
        )
        {
            // "Shared: invalid or missing value %s"
            WRMSG( HHC00740, "E", op );
            return -1;
        }

        OBTAIN_SHRDTRACE_LOCK();
        {
            sysblk.shrddtax = dtax ? true : false;
        }
        RELEASE_SHRDTRACE_LOCK();

        // "%-14s set to %s"
        MSGBUF( buf, "DTAX=%d", sysblk.shrddtax );
        WRMSG( HHC02204, "I", argv[0], buf );

        return 0;
    }

    // "Shared: invalid or missing keyword %s"
    WRMSG( HHC00741, "E", kw );
    return -1;
}

/*-------------------------------------------------------------------
 * Print Shared Device Server trace table entries
 *-------------------------------------------------------------------*/
DLL_EXPORT void shared_print_trace_table()
{
    OBTAIN_SHRDTRACE_LOCK();
    {
        shared_print_trace_table_locked();
    }
    RELEASE_SHRDTRACE_LOCK();
}

/*-------------------------------------------------------------------
 * Print Shared Device Server trace table entries
 *-------------------------------------------------------------------*/
static void shared_print_trace_table_locked()
{
    /* Does a trace table exist? */
    if (sysblk.shrdtrace)
    {
        SHRD_TRACE* current = sysblk.shrdtracep;
        bool printed = false;

        do
        {
            if (current[0][0])
            {
                // "Shared:  %s"
                WRMSG( HHC00743, "I", (char*) current );
                printed = true;
            }

            if (++current >= sysblk.shrdtracex)
                current = sysblk.shrdtrace;
        }
        while (current != sysblk.shrdtracep);

        if (!printed)
        {
            // "Shared:  %s"
            WRMSG( HHC00743, "I", "(none)" );
        }

        /* Now that it's been printed, reset it to empty */
        memset( sysblk.shrdtrace, 0, sysblk.shrdtracen * sizeof( SHRD_TRACE ));
    }
    else
        // "Shared:  %s"
        WRMSG( HHC00743, "I", "(NULL)" );
}

DEVHND shared_ckd_device_hndinfo = {
        &shared_ckd_init,              /* Device Initialization      */
        &ckd_dasd_execute_ccw,         /* Device CCW execute         */
        &shared_ckd_close,             /* Device Close               */
        &ckd_dasd_query_device,        /* Device Query               */
        NULL,                          /* Device Extended Query      */
        &shared_start,                 /* Device Start channel pgm   */
        &shared_end,                   /* Device End channel pgm     */
        &shared_start,                 /* Device Resume channel pgm  */
        &shared_end,                   /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        &shared_ckd_read,              /* Device Read                */
        &shared_ckd_write,             /* Device Write               */
        &shared_used,                  /* Device Query used          */
        &shared_reserve,               /* Device Reserve             */
        &shared_release,               /* Device Release             */
        NULL,                          /* Device Attention           */
        NULL,                          /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        &ckd_dasd_hsuspend,            /* Hercules suspend           */
        &ckd_dasd_hresume              /* Hercules resume            */
};

DEVHND shared_fba_device_hndinfo = {
        &shared_fba_init,              /* Device Initialization      */
        &fba_dasd_execute_ccw,         /* Device CCW execute         */
        &shared_fba_close,             /* Device Close               */
        &fba_dasd_query_device,        /* Device Query               */
        NULL,                          /* Device Extended Query      */
        &shared_start,                 /* Device Start channel pgm   */
        &shared_end,                   /* Device End channel pgm     */
        &shared_start,                 /* Device Resume channel pgm  */
        &shared_end,                   /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        &shared_ckd_read,              /* Device Read                */
        &shared_ckd_write,             /* Device Write               */
        &shared_used,                  /* Device Query used          */
        &shared_reserve,               /* Device Reserve             */
        &shared_release,               /* Device Release             */
        NULL,                          /* Device Attention           */
        NULL,                          /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        &fba_dasd_hsuspend,            /* Hercules suspend           */
        &fba_dasd_hresume              /* Hercules resume            */
};

#else // !defined( OPTION_SHARED_DEVICES )

int shared_update_notify( DEVBLK* dev, int block )
{
 UNREFERENCED( dev   );
 UNREFERENCED( block );
 return 0;
}
int shared_ckd_init( DEVBLK* dev, int argc, char* argv[] )
{
 UNREFERENCED( dev  );
 UNREFERENCED( argc );
 UNREFERENCED( argv );
 return -1;
}
int shared_fba_init( DEVBLK* dev, int argc, char* argv[] )
{
 UNREFERENCED( dev  );
 UNREFERENCED( argc );
 UNREFERENCED( argv );
 return -1;
}
void* shared_server( void* arg )
{
 UNREFERENCED( arg );
 // "Shared: OPTION_SHARED_DEVICES not defined"
 WRMSG( HHC00742, "E" );
 return NULL;
}
int shrd_cmd( int argc, char* argv[], char* cmdline )
{
 UNREFERENCED( argc    );
 UNREFERENCED( argv    );
 UNREFERENCED( cmdline );
 // "Shared: OPTION_SHARED_DEVICES not defined"
 WRMSG( HHC00742, "E" );
 return -1;
}
#endif /* defined( OPTION_SHARED_DEVICES ) */
