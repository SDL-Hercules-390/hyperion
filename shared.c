/* SHARED.C     (c)Copyright Greg Smith, 2002-2012                   */
/*              Shared Device Server                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

DISABLE_GCC_WARNING( "-Wunused-function" )

#define _HERCULES_SHARED_C
#define _SHARED_C_
#define _HDASD_DLL_
#include "hercules.h"
#include "opcode.h"
#include "devtype.h"

#define FBA_BLKGRP_SIZE (120*512)

/*-------------------------------------------------------------------*/
/* Definitions for sense data format codes and message codes         */
/*-------------------------------------------------------------------*/
#define FORMAT_0                0       /* Program or System Checks  */
#define FORMAT_1                1       /* Device Equipment Checks   */
#define FORMAT_2                2       /* 3990 Equipment Checks     */
#define FORMAT_3                3       /* 3990 Control Checks       */
#define FORMAT_4                4       /* Data Checks               */
#define FORMAT_5                5       /* Data Check + Displacement */
#define FORMAT_6                6       /* Usage Stats/Overrun Errors*/
#define FORMAT_7                7       /* Device Control Checks     */
#define FORMAT_8                8       /* Device Equipment Checks   */
#define FORMAT_9                9       /* Device Rd/Wrt/Seek Checks */
#define FORMAT_F                15      /* Cache Storage Checks      */
#define MESSAGE_0               0       /* Message 0                 */
#define MESSAGE_1               1       /* Message 1                 */
#define MESSAGE_2               2       /* Message 2                 */
#define MESSAGE_3               3       /* Message 3                 */
#define MESSAGE_4               4       /* Message 4                 */
#define MESSAGE_5               5       /* Message 5                 */
#define MESSAGE_6               6       /* Message 6                 */
#define MESSAGE_7               7       /* Message 7                 */
#define MESSAGE_8               8       /* Message 8                 */
#define MESSAGE_9               9       /* Message 9                 */
#define MESSAGE_A               10      /* Message A                 */
#define MESSAGE_B               11      /* Message B                 */
#define MESSAGE_C               12      /* Message C                 */
#define MESSAGE_D               13      /* Message D                 */
#define MESSAGE_E               14      /* Message E                 */
#define MESSAGE_F               15      /* Message F                 */

#if defined(OPTION_SHARED_DEVICES)

DEVHND  shared_ckd_device_hndinfo;
DEVHND  shared_fba_device_hndinfo;

static  BYTE eighthexFF[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

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
           shrdtrc(dev,"notify %d added for id=%d, n=%d\n",
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

    retry = dev->connecting;

    /* Process the arguments */
    if (!retry)
    {
        if (argc < 1 || strlen(argv[0]) >= sizeof(buf))
            return -1;
        strlcpy( buf, argv[0], sizeof(buf) );

        /* First argument is `ipname:port:devnum' */
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
#ifdef HAVE_LIBZ
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
            WRMSG (HHC00700, "S", argv[i], i + 1);
            return -1;
        }
    }

    /* Set suported compression */
    dev->rmtcomps = 0;
#ifdef HAVE_LIBZ
    dev->rmtcomps |= SHRD_LIBZ;
#endif
#ifdef CCKD_BZIP2
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
            WRMSG (HHC00701, "W", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename);
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
        WRMSG (HHC00702, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
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
        WRMSG (HHC00703, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
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
        WRMSG (HHC00704, "S", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->rmtnum, fetch_hw (dev->devchar + 3));
        return -1;
    }

    /* Get the device id */
    rc = clientRequest (dev, dev->devid, sizeof(dev->devid),
                        SHRD_QUERY, SHRD_DEVID, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc == 0 || rc > (int)sizeof(dev->devid))
    {
        WRMSG (HHC00705, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
        return -1;
    }
    dev->numdevid = rc;

    /* Indicate no active track */
    dev->cache = dev->bufcur = -1;
    dev->buf = NULL;

    /* Set number of sense bytes */
    dev->numsense = 32;

    /* Locate the CKD dasd table entry */
    dev->ckdtab = dasd_lookup (DASD_CKDDEV, NULL, dev->devtype, dev->ckdcyls);
    if (dev->ckdtab == NULL)
    {
        WRMSG (HHC00706, "S", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->devtype);
        return -1;
    }

    /* Set the track size */
    dev->ckdtrksz = (dev->ckdtab->r1 + 511) & ~511;

    /* Locate the CKD control unit dasd table entry */
    dev->ckdcu = dasd_lookup (DASD_CKDCU, cu ? cu : dev->ckdtab->cu, 0, 0);
    if (dev->ckdcu == NULL)
    {
        WRMSG (HHC00707, "S", SSID_TO_LCSS(dev->ssid), dev->devnum, cu ? cu : dev->ckdtab->cu);
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
    WRMSG (HHC00708, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename, dev->ckdcyls,
            dev->ckdheads, dev->ckdtrks, dev->ckdtrksz);

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
#ifdef HAVE_LIBZ
char    *strtok_str = NULL;             /* last token                */
#endif /*HAVE_LIBZ*/

    retry = dev->connecting;

    /* Process the arguments */
    if (!retry)
    {

        kw = op = NULL;

        if (argc < 1 || strlen(argv[0]) >= sizeof(buf))
            return -1;
        strlcpy( buf, argv[0], sizeof(buf) );

        /* First argument is `ipname:port:devnum' */
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
#ifdef HAVE_LIBZ
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
            WRMSG (HHC00700, "S", argv[i], i + 1);
            rc = -1;
        }
        if (rc)
            return rc;
    }

    /* Set suported compression */
    dev->rmtcomps = 0;
#ifdef HAVE_LIBZ
    dev->rmtcomps |= SHRD_LIBZ;
#endif
#ifdef CCKD_BZIP2
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
            WRMSG (HHC00701, "W", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename);
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
        WRMSG (HHC00709, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
        return -1;
    }
    dev->fbaorigin = fetch_fw (origin);

    /* Get the number of blocks */
    rc = clientRequest (dev, numblks, 4, SHRD_QUERY, SHRD_FBANUMBLK, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc != 4)
    {
        WRMSG (HHC00710, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
        return -1;
    }
    dev->fbanumblk = fetch_fw (numblks);

    /* Get the block size */
    rc = clientRequest (dev, blksiz, 4, SHRD_QUERY, SHRD_FBABLKSIZ, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc != 4)
    {
        WRMSG (HHC00711, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
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
        WRMSG (HHC00705, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
        return -1;
    }
    dev->numdevid = rc;

    /* Check the device type */
    if (dev->devtype != fetch_hw (dev->devid + 4))
    {
        WRMSG (HHC00704, "S", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->rmtnum, fetch_hw (dev->devid + 4));
        return -1;
    }

    /* Get the device characteristics */
    rc = clientRequest (dev, dev->devchar, sizeof(dev->devchar),
                        SHRD_QUERY, SHRD_DEVCHAR, NULL, NULL);
    if (rc < 0)
        goto init_retry;
    else if (rc == 0 || rc > (int)sizeof(dev->devchar))
    {
        WRMSG (HHC00703, "S", SSID_TO_LCSS(dev->ssid), dev->devnum);
        return -1;
    }
    dev->numdevchar = rc;

    /* Indicate no active track */
    dev->cache = dev->bufcur = -1;
    dev->buf = NULL;

    /* Set number of sense bytes */
    dev->numsense = 32;

    /* Locate the FBA dasd table entry */
    dev->fbatab = dasd_lookup (DASD_FBADEV, NULL, dev->devtype, dev->fbanumblk);
    if (dev->fbatab == NULL)
    {
        WRMSG (HHC00706, "S", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->devtype);
        return -1;
    }

    /* Purge the cache */
    clientPurge (dev, 0, NULL);

    /* Log the device geometry */
    WRMSG (HHC00712, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename, (int)dev->fbaorigin, dev->fbanumblk);

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

    shrdtrc(dev,"start cur %d cache %d\n",dev->bufcur,dev->cache);

    /* Send the START request */
    rc = clientRequest (dev, buf, sizeof(buf),
                        SHRD_START, 0, &code, NULL);
    if (rc < 0)
    {
        WRMSG(HHC00713, "E", SSID_TO_LCSS(dev->ssid), dev->devnum);
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

    shrdtrc(dev,"end cur %d cache %d\n",dev->bufcur,dev->cache);

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
        WRMSG(HHC00714, "E", SSID_TO_LCSS(dev->ssid), dev->devnum);
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
BYTE    *buf;                           /* Cache buffer              */
BYTE     code;                          /* Response code             */
U16      devnum;                        /* Response device number    */
BYTE     hdr[SHRD_HDR_SIZE + 4];        /* Read request header       */

    /* Initialize the unit status */
    *unitstat = 0;

    /* Return if reading the same track image */
    if (trk == dev->bufcur && dev->cache >= 0)
    {
        dev->bufoff = 0;
        dev->bufoffhi = dev->ckdtrksz;
        return 0;
    }

    shrdtrc(dev,"ckd_read trk %d\n",trk);

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
        shrdtrc(dev,"ckd_read trk %d cache hit %d\n",trk,dev->cache);
        return 0;
    }

    /* Special processing if no available cache entry */
    if (lru < 0)
    {
        shrdtrc(dev,"ckd_read trk %d cache wait\n",trk);
        dev->cachewaits++;
        cache_wait (CACHE_DEVBUF);
        goto cache_retry;
    }

    /* Process cache miss */
    shrdtrc(dev,"ckd_read trk %d cache miss %d\n",trk,dev->cache);
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
        WRMSG(HHC00715, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, trk);
        return -1;
    }

    /* Read the track from the remote host */
    rc = clientRecv (dev, hdr, buf, dev->ckdtrksz);
    SHRD_GET_HDR (hdr, code, *unitstat, devnum, id, len);
    if (rc < 0 || code & SHRD_ERROR)
    {
        if (rc < 0 && retries--) goto read_retry;
        ckd_build_sense (dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        WRMSG(HHC00715, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, trk);
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

    shrdtrc(dev,"ckd_write trk %d off %d len %d\n",trk,off,len);

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

    for (sz = CKDDASD_TRKHDR_SIZE;
         memcmp (buf + sz, &eighthexFF, 8) != 0; )
    {
        /* add length of count, key, and data fields */
        sz += CKDDASD_RECHDR_SIZE +
                buf[sz+5] +
                (buf[sz+6] << 8) + buf[sz+7];
        if (sz > dev->ckdtrksz - 8) break;
    }

    /* add length for end-of-track indicator */
    sz += CKDDASD_RECHDR_SIZE;

    if (sz > dev->ckdtrksz)
        sz = dev->ckdtrksz;

    return sz;
}

#if 0
Perhaps these functions should have not been static.  They are
dead code and identified as such.

#if defined( OPTION_SHARED_DEVICES ) && defined(FBA_SHARED)
/*-------------------------------------------------------------------
 * Shared fba read block exit (client side)
 *-------------------------------------------------------------------*/
static int shared_fba_read (DEVBLK *dev, int blkgrp, BYTE *unitstat)
{
int      rc;                            /* Return code               */
int      retries = 10;                  /* Number read retries       */
int      i, o;                          /* Cache indexes             */
BYTE     code;                          /* Response code             */
U16      devnum;                        /* Response device number    */
int      len;                           /* Response length           */
int      id;                            /* Response id               */
BYTE     hdr[SHRD_HDR_SIZE + 4];        /* Read request header       */


    /* Return if reading the same block group */
    if (blkgrp >= 0 && blkgrp == dev->bufcur)
        return 0;

    shrdtrc(dev,"fba_read blkrp %d\n",blkgrp);

    /* Write the previous active entry if it was updated */
    if (dev->bufupd)
        clientWrite (dev, dev->bufcur);
    dev->bufupd = 0;

    /* Reset buffer offsets */
    dev->bufoff = 0;
    dev->bufoffhi = FBA_BLKGRP_SIZE;

    cache_lock (CACHE_DEVBUF);

    /* Make the previous cache entry inactive */
    if (dev->cache >= 0)
        cache_setflag(CACHE_DEVBUF, dev->cache, ~FBA_CACHE_ACTIVE, 0);
    dev->bufcur = dev->cache = -1;

cache_retry:

    /* Search the cache */
    i = cache_lookup (CACHE_DEVBUF, FBA_CACHE_SETKEY(dev->devnum, blkgrp), &o);

    /* Cache hit */
    if (i >= 0)
    {
        cache_setflag(CACHE_DEVBUF, dev->cache, ~0, FBA_CACHE_ACTIVE);
        cache_setage(CACHE_DEVBUF, dev->cache);
        cache_unlock(CACHE_DEVBUF);
        dev->cachehits++;
        dev->cache = i;
        dev->buf = cache_getbuf(CACHE_DEVBUF, dev->cache, 0);
        dev->bufcur = blkgrp;
        dev->bufoff = 0;
        dev->bufoffhi = shared_fba_blkgrp_len (dev, blkgrp);
        dev->buflen = shared_fba_blkgrp_len (dev, blkgrp);
        dev->bufsize = cache_getlen(CACHE_DEVBUF, dev->cache);
        shrdtrc(dev,"fba_read blkgrp %d cache hit %d\n",blkgrp,dev->cache);
        return 0;
    }

    /* Wait if no available cache entry */
    if (o < 0)
    {
        shrdtrc(dev,"fba_read blkgrp %d cache wait\n",blkgrp);
        dev->cachewaits++;
        cache_wait(CACHE_DEVBUF);
        goto cache_retry;
    }

    /* Cache miss */
    shrdtrc(dev,"fba_read blkgrp %d cache miss %d\n",blkgrp,dev->cache);
    dev->cachemisses++;
    cache_setflag(CACHE_DEVBUF, o, 0, FBA_CACHE_ACTIVE|DEVBUF_TYPE_SFBA);
    cache_setkey (CACHE_DEVBUF, o, FBA_CACHE_SETKEY(dev->devnum, blkgrp));
    cache_setage (CACHE_DEVBUF, o);
    dev->buf = cache_getbuf(CACHE_DEVBUF, o, FBA_BLKGRP_SIZE);

    cache_unlock (CACHE_DEVBUF);

read_retry:

    /* Send the read request for the blkgrp to the remote host */
    SHRD_SET_HDR (hdr, SHRD_READ, 0, dev->rmtnum, dev->rmtid, 4);
    store_fw (hdr + SHRD_HDR_SIZE, blkgrp);
    rc = clientSend (dev, hdr, NULL, 0);
    if (rc < 0)
    {
        dev->sense[0] = SENSE_EC;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        WRMSG(HHC00716, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, blkgrp);
        return -1;
    }

    /* Read the blkgrp from the remote host */
    rc = clientRecv (dev, hdr, dev->buf, FBA_BLKGRP_SIZE);
    SHRD_GET_HDR (hdr, code, *unitstat, devnum, id, len);
    if (rc < 0 || code & SHRD_ERROR)
    {
        if (rc < 0 && retries--) goto read_retry;
        dev->sense[0] = SENSE_EC;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        WRMSG(HHC00716, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, blkgrp);
        return -1;
    }

    /* Read the sense data if an i/o error occurred */
    if (code & SHRD_IOERR)
        clientRequest (dev, dev->sense, dev->numsense,
                      SHRD_SENSE, 0, NULL, NULL);

    dev->cache = o;
    dev->buf = cache_getbuf(CACHE_DEVBUF, dev->cache, 0);
    dev->buf[0] = 0;
    dev->bufcur = blkgrp;
    dev->bufoff = 0;
    dev->bufoffhi = shared_fba_blkgrp_len (dev, blkgrp);
    dev->buflen = shared_fba_blkgrp_len (dev, blkgrp);
    dev->bufsize = cache_getlen(CACHE_DEVBUF, dev->cache);

    return 0;
}

/*-------------------------------------------------------------------
 * Shared fba write block exit (client side)
 *-------------------------------------------------------------------*/
static int shared_fba_write (DEVBLK *dev, int blkgrp, int off,
                             BYTE *buf, int len, BYTE *unitstat)
{
int             rc;                     /* Return code               */

    /* Read the block group */
    if (blkgrp != dev->bufcur)
    {
        rc = (dev->hnd->read) (dev, blkgrp, unitstat);
        if (rc < 0) {
            dev->bufcur = dev->cache = -1;
            return -1;
        }
    }

    /* Copy to the device buffer */
    if (buf) memcpy (dev->buf + off, buf, len);

    /* Update high/low offsets */
    if (!dev->bufupd || off < dev->bufupdlo)
        dev->bufupdlo = off;
    if (off + len > dev-> bufupdhi)
        dev->bufupdhi = off + len;

    /* Indicate block group has been modified */
    if (!dev->bufupd)
    {
        dev->bufupd = 1;
        shared_update_notify (dev, blkgrp);
    }

    return len;
}

/*-------------------------------------------------------------------*/
/* Calculate length of an FBA block group                            */
/*-------------------------------------------------------------------*/
static int shared_fba_blkgrp_len (DEVBLK *dev, int blkgrp)
{
off_t   offset;                         /* Offset of block group     */

    offset = blkgrp * FBA_BLKGRP_SIZE;
    if (dev->fbaend - offset < FBA_BLKGRP_SIZE)
        return (int)(dev->fbaend - offset);
    else
        return FBA_BLKGRP_SIZE;
}

#endif /* FBA_SHARED */
#endif

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
        WRMSG (HHC00717, "E", SSID_TO_LCSS(dev->ssid), dev->devnum);
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

    shrdtrc(dev,"write rcd %d off %d len %d\n",block,dev->bufupdlo,len);

write_retry:

    /* The write request contains a 2 byte offset and 4 byte id,
       followed by the data */
    SHRD_SET_HDR (hdr, SHRD_WRITE, 0, dev->rmtnum, dev->rmtid, len + 6);
    store_hw (hdr + SHRD_HDR_SIZE, dev->bufupdlo);
    store_fw (hdr + SHRD_HDR_SIZE + 2, block);

    rc = clientSend (dev, hdr, dev->buf + dev->bufupdlo, len);
    if (rc < 0)
    {
        WRMSG(HHC00718, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->bufcur);
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
        WRMSG(HHC00719, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->bufcur, code, status);
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
            shrdtrc(dev,"purge %d\n",trk);
        }
        else
        {
            for (p = 0; p < dev->rmtpurgen; p++)
            {
                if (trk == (int)fetch_fw (dev->rmtpurge[p]))
                {
                    shrdtrc(dev,"purge %d\n",trk);
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
static int clientConnect (DEVBLK *dev, int retry)
{
int                rc;                  /* Return code               */
struct sockaddr   *server;              /* -> server descriptor      */
int                flag;                /* Flags (version | release) */
int                len;                 /* Length server descriptor  */
struct sockaddr_in iserver;             /* inet server descriptor    */
#if defined( HAVE_SYS_UN_H )
struct sockaddr_un userver;             /* unix server descriptor    */
#endif
int                retries = 10;        /* Number of retries         */
HWORD              id;                  /* Returned identifier       */
HWORD              comp;                /* Returned compression parm */

    do {

        /* Close previous connection */
        if (dev->fd >= 0) close_socket (dev->fd);

        /* Get a socket */
        if (dev->localhost)
        {
#if defined( HAVE_SYS_UN_H )
            dev->fd = dev->ckdfd[0] = socket (AF_UNIX, SOCK_STREAM, 0);
#else // !defined( HAVE_SYS_UN_H )
            dev->fd = dev->ckdfd[0] = -1;
#endif // defined( HAVE_SYS_UN_H )
            if (dev->fd < 0)
            {
                WRMSG (HHC00720, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "socket()", strerror(HSO_errno));
                return -1;
            }
#if defined( HAVE_SYS_UN_H )
            userver.sun_family = AF_UNIX;
            sprintf(userver.sun_path, "/tmp/hercules_shared.%d", dev->rmtport);
            server = (struct sockaddr *)&userver;
            len = sizeof(userver);
#endif // !defined( HAVE_SYS_UN_H )
        }
        else
        {
            dev->fd = dev->ckdfd[0] = socket (AF_INET, SOCK_STREAM, 0);
            if (dev->fd < 0)
            {
                WRMSG (HHC00720, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "socket()", strerror(HSO_errno));
                return -1;
            }
            iserver.sin_family      = AF_INET;
            iserver.sin_port        = htons(dev->rmtport);
            memcpy(&iserver.sin_addr.s_addr,&dev->rmtaddr,sizeof(struct in_addr));
            server = (struct sockaddr *)&iserver;
            len = sizeof(iserver);
        }

        /* Connect to the server */
        store_hw (id, dev->rmtid);
        rc = connect (dev->fd, server, len);
        shrdtrc(dev,"connect rc=%d errno=%d\n",rc, HSO_errno);
        if (rc >= 0)
        {
            if (!dev->batch)
              WRMSG(HHC00721, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename);

            /* Request device connection */
            flag = (SHARED_VERSION << 4) | SHARED_RELEASE;
            rc = clientRequest (dev, id, 2, SHRD_CONNECT, flag, NULL, &flag);
            if (rc >= 0)
            {
                dev->rmtid = fetch_hw (id);
                dev->rmtrel = flag & 0x0f;
            }

            /*
             * Negotiate compression - top 4 bits have the compression
             * algorithms we support (00010000 -> libz; 00100000 ->bzip2,
             * 00110000 -> both) and the bottom 4 bits indicates the
             * libz parm we want to use when sending data back & forth.
             * If the server returns `0' back, then we won't use libz to
             * compress data to the server.  What the `compression
             * algorithms we support' means is that if the data source is
             * cckd or cfba then the server doesn't have to uncompress
             * the data for us if we support the compression algorithm.
             */

            if (rc >= 0 && (dev->rmtcomp || dev->rmtcomps))
            {
                rc = clientRequest (dev, comp, 2, SHRD_COMPRESS,
                           (dev->rmtcomps << 4) | dev->rmtcomp, NULL, NULL);
                if (rc >= 0)
                    dev->rmtcomp = fetch_hw (comp);
            }

        }
        else if (!retry)
            WRMSG(HHC00722, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename, strerror(HSO_errno));

        if (rc < 0 && retry) usleep (20000);

    } while (retry && retries-- && rc < 0);

    return rc;

} /* clientConnect */

/*-------------------------------------------------------------------
 * Send request to host and get the response
 *
 * No data is sent on the request, buf gets the response.
 * If an uncorrectable connection error occurs -1 is returned.
 * Otherwise *code and *status is set from the response header
 *
 * Since `buf' may be NULL or not very long, response data is
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

retry :

    /* Send the request */
    SHRD_SET_HDR(hdr, cmd, flags, dev->rmtnum, dev->rmtid, 0);
    shrdtrc(dev,"client_request %2.2x %2.2x %2.2x %d\n",
            cmd,flags,dev->rmtnum,dev->rmtid);
    rc = clientSend (dev, hdr, NULL, 0);
    if (rc < 0) return rc;

    /* Receive the response */
    rc = clientRecv (dev, hdr, temp, sizeof(temp));

    /* Retry recv errors */
    if (rc < 0)
    {
        if (cmd != SHRD_CONNECT && retries--)
        {
            SLEEP (1);
            clientConnect (dev, 1);
            goto retry;
        }
        return -1;
    }

    /* Set code and status */
    SHRD_GET_HDR(hdr, rcode, rstatus, rdevnum, rid, rlen);
    shrdtrc(dev,"client_response %2.2x %2.2x %2.2x %d %d\n",
            rcode,rstatus,rdevnum,rid,rlen);
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
 * `buf' may be NULL
 * `buflen' is the length in `buf' (should be 0 if `buf' is NULL)
 * `hdr' may contain additional data; this is detected by the
 *       difference between `buflen' and the length in the header
 *
 * If `buf' is adjacent to `hdr' then `buf' should be NULL
 *
 *-------------------------------------------------------------------*/
static int clientSend (DEVBLK *dev, BYTE *hdr, BYTE *buf, int buflen)
{
int      rc;                            /* Return code               */
BYTE     cmd;                           /* Header command            */
BYTE     flag;                          /* Header flags              */
U16      devnum;                        /* Header device nu          */
int      len;                           /* Header length             */
int      id;                            /* Header identifier         */
int      hdrlen;                        /* Header length + other data*/
int      off;                           /* Offset to buffer data     */
BYTE    *sendbuf;                       /* Send buffer               */
int      sendlen;                       /* Send length               */
BYTE     cbuf[SHRD_HDR_SIZE + 65536];   /* Combined buffer           */

    /* Make buf, buflen consistent if no additional data to be sent  */
    if (buf == NULL) buflen = 0;
    else if (buflen == 0) buf = NULL;

    /* Calculate length of header, may contain additional data */
    SHRD_GET_HDR(hdr, cmd, flag, devnum, id, len);
    shrdtrc(dev,"client_send %2.2x %2.2x %2.2x %d %d\n",
             cmd,flag,devnum,id,len);
    hdrlen = SHRD_HDR_SIZE + (len - buflen);
    off = len - buflen;

    if (dev->fd < 0)
    {
        rc = clientConnect (dev, 1);
        if (rc < 0) return -1;
    }

#ifdef HAVE_LIBZ
    /* Compress the buf */
    if (dev->rmtcomp != 0
     && flag == 0 && off <= SHRD_COMP_MAX_OFF
     && buflen >= SHARED_COMPRESS_MINLEN)
    {
        unsigned long newlen;
        newlen = 65536 - hdrlen;
        memcpy (cbuf, hdr, hdrlen);
        rc = compress2 (cbuf + hdrlen, &newlen,
                        buf, buflen, dev->rmtcomp);
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
        memcpy (cbuf, hdr, hdrlen);
        memcpy (cbuf + hdrlen, buf, buflen);
        sendbuf = cbuf;
        sendlen = hdrlen + buflen;
    }

    SHRD_SET_HDR(sendbuf, cmd, flag, devnum, id, (U16)(sendlen - SHRD_HDR_SIZE));

    if (cmd & SHRD_COMP)
        shrdtrc(dev,"client_send %2.2x %2.2x %2.2x %d %d (compressed)\n",
                cmd, flag, devnum, id, (int)(sendlen - SHRD_HDR_SIZE));

retry:

    /* Send the header and data */
    rc = send (dev->fd, sendbuf, sendlen, 0);
    if (rc < 0)
    {
        rc = clientConnect (dev, 0);
        if (rc >= 0) goto retry;
    }

    /* Process return code */
    if (rc < 0)
    {
        WRMSG(HHC00723, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, cmd, flag, strerror(HSO_errno));
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

    /* Clear the header to zeroes */
    memset( hdr, 0, SHRD_HDR_SIZE );

    /* Return error if not connected */
    if (dev->fd < 0)
    {
        WRMSG(HHC00724, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename);
        return -1;
    }

    /* Receive the header */
    rc = recvData (dev->fd, hdr, buf, buflen, 0);
    if (rc < 0)
    {
        if (rc != -ENOTCONN)
            WRMSG(HHC00725, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, strerror(-rc));
        return rc;
    }
    SHRD_GET_HDR(hdr, code, status, devnum, id, len);

    shrdtrc(dev,"client_recv %2.2x %2.2x %2.2x %d %d\n",
             code,status,devnum,id,len);

    /* Handle remote logical error */
    if (code & SHRD_ERROR)
    {
        WRMSG(HHC00726, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, code, status, buf);
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
 * Receive data (server or client)
 *-------------------------------------------------------------------*/
static int recvData(int sock, BYTE *hdr, BYTE *buf, int buflen, int server)
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
DEVBLK                 *dev = NULL;     /* For `shrdtrc'             */
BYTE                    cbuf[65536];    /* Compressed buffer         */


    /* Receive the header */
    for (recvlen = 0; recvlen < (int)SHRD_HDR_SIZE; recvlen += rc)
    {
        rc = recv (sock, hdr + recvlen, SHRD_HDR_SIZE - recvlen, 0);
        if (rc < 0)
            return -HSO_errno;
        else if (rc == 0)
            return -HSO_ENOTCONN;
    }
    SHRD_GET_HDR (hdr, cmd, flag, devnum, id, len);

    shrdtrc(dev,"recvData    %2.2x %2.2x %2.2x %d %d\n",
             cmd, flag, devnum, id, len);

    /* Return if no data */
    if (len == 0) return 0;

    /* Check for compressed data */
    if ((server && (cmd & SHRD_COMP))
     || (!server && cmd == SHRD_COMP))
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
            return -HSO_ENOTCONN;
    }

    /* Flush any remaining data */
    for (; rlen < len; rlen += rc)
    {
        BYTE buf[256];
        rc = recv (sock, buf, len - rlen < 256 ? len - rlen : 256, 0);
        if (rc < 0)
            return -HSO_errno;
        else if (rc == 0)
            return -HSO_ENOTCONN;
    }

    /* Check for compression */
    if (comp == SHRD_LIBZ) {
#ifdef HAVE_LIBZ
        unsigned long newlen;

        if (off > 0)
            memcpy (buf, cbuf, off);
        newlen = buflen - off;
        rc = uncompress(buf + off, &newlen, cbuf + off, len - off);
        if (rc == Z_OK)
            recvlen = (int)newlen + off;
        else
        {
            WRMSG(HHC00727, "E", rc, off, len - off);
            recvlen = -1;
        }
#else
        WRMSG(HHC00728, "E", "libz");
        recvlen = -1;
#endif
    }
    else if (comp == SHRD_BZIP2)
    {
#ifdef CCKD_BZIP2
        unsigned int newlen;

        if (off > 0)
            memcpy (buf, cbuf, off);

        newlen = buflen - off;
        rc = BZ2_bzBuffToBuffDecompress((void *)(buf + off), &newlen, (void *)(cbuf + off), len - off, 0, 0);
        if (rc == BZ_OK)
            recvlen = (int)newlen + off;
        else
        {
            WRMSG(HHC00727, "E", rc, off, len - off);
            recvlen = -1;
        }
#else
        WRMSG(HHC00728, "E", "bzip2");
        recvlen = -1;
#endif
    }

    if (recvlen > 0)
    {
        SHRD_SET_HDR (hdr, cmd, flag, devnum, id, recvlen);
        if (comp)
            shrdtrc(dev,"recvData    %2.2x %2.2x %2.2x %d %d (uncompressed)\n",
                     cmd, flag, devnum, id, recvlen);
    }

    return recvlen;

} /* recvData */

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

    /* Extract header information */
    SHRD_GET_HDR (hdr, cmd, flag, devnum, id, len);

    shrdtrc(dev,"server_request [%d] %2.2x %2.2x %2.2x %d %d\n",
             ix, cmd, flag, devnum, id, len);

    dev->shrd[ix]->time = time (NULL);

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

        obtain_lock (&dev->lock);

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

        release_lock (&dev->lock);
        break;

    case SHRD_START:
    case SHRD_RESUME:

        obtain_lock (&dev->lock);

        /* If the device is suspended locally then grab it */
        if (dev->shioactive == DEV_SYS_LOCAL && dev->suspended && !dev->reserved)
            dev->shioactive = id;

        /* Check if the device is busy */
        if (dev->shioactive != id && dev->shioactive != DEV_SYS_NONE)
        {
            shrdtrc(dev,"server_request busy id=%d shioactive=%d reserved=%d\n",
                    id,dev->shioactive,dev->reserved);
            /* If the `nowait' bit is on then respond `busy' */
            if (flag & SHRD_NOWAIT)
            {
                release_lock (&dev->lock);
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

            /* Return with the `waiting' bit on if busy by a remote system */
            if (dev->shioactive != DEV_SYS_NONE && dev->shioactive != DEV_SYS_LOCAL)
            {
                release_lock (&dev->lock);
                break;
            }

            dev->shrd[ix]->waiting = 0;
        }

        /* Make this system active on the device */
        dev->shioactive = id;
        dev->busy = 1;
        sysblk.shrdcount++;
        shrdtrc(dev,"server_request active id=%d\n", id);

        release_lock(&dev->lock);

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

        obtain_lock (&dev->lock);

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

            /* Reset any `waiting' bits */
            for (i = 0; i < SHARED_MAX_SYS; i++)
                if (dev->shrd[i])
                    dev->shrd[i]->waiting = 0;

            /* Notify any waiters */
            if (dev->shiowaiters)
                signal_condition (&dev->shiocond);
        }
        shrdtrc(dev,"server_request inactive id=%d\n", id);

        release_lock (&dev->lock);

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

        obtain_lock (&dev->lock);
        dev->reserved = 1;
        release_lock (&dev->lock);

        shrdtrc(dev,"server_request reserved id=%d\n", id);

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

        obtain_lock (&dev->lock);
        dev->reserved = 0;
        release_lock (&dev->lock);

        shrdtrc(dev,"server_request released id=%d\n", id);

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
        shrdtrc(dev,"server_request read rcd %d flag %2.2x rc=%d\n",
                rcd, flag, rc);

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
        shrdtrc(dev,"server_request write rcd %d off %d len %d flag %2.2x rc=%d\n",
                rcd, off, len - 6, flag, rc);

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

        case SHRD_CKDCYLS:
            store_fw (buf, dev->ckdcyls);
            SHRD_SET_HDR (hdr, 0, 0, dev->devnum, id, 4);
            serverSend (dev, ix, hdr, buf, 4);
            break;

        case SHRD_FBAORIGIN:
            store_fw (buf, dev->fbaorigin);
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
#ifdef HAVE_LIBZ
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
 * Locate the SHRD block for a socket (server side)
 *-------------------------------------------------------------------*/
static int serverLocate (DEVBLK *dev, int id, int *avail)
{
int      i;                             /* Loop index                */

    if (avail) *avail = -1;
    for (i = 0; i < SHARED_MAX_SYS; i++)
    {
        if (dev->shrd[i])
        {
            if (dev->shrd[i]->id == id)
                return i;
        }
        else if (avail && *avail < 0)
            *avail = i;
    }
    return -1;
} /* serverLocate */

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
int rc;                                 /* Return code               */
size_t len;                             /* Message length            */
BYTE hdr[SHRD_HDR_SIZE];                /* Header                    */

    /* Get message length */
    len = strlen(msg) + 1;
    if (len > SHARED_MAX_MSGLEN)
        len = SHARED_MAX_MSGLEN;

    SHRD_SET_HDR (hdr, code, status, dev ? dev->devnum : 0,
                  ix < 0 ? 0 : dev->shrd[ix]->id, (U16)len);

    shrdtrc(dev,"server_error %2.2x %2.2x: %s\n", code, status, msg);

    rc = serverSend (dev, ix, hdr, (BYTE *)msg, (int)len);
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

    /* Send only the header buffer if `buf' is empty */
    if (buflen == 0)  sendbuf = hdr;

    /* Get socket number; if `ix' < 0 we don't have a device yet */
    if (ix >= 0)
        sock = dev->shrd[ix]->fd;
    else
    {
        sock = -ix;
        dev = NULL;
    }

    shrdtrc(dev,"server_send %2.2x %2.2x %2.2x %d %d\n",
            code, status, devnum, id, len);

#ifdef HAVE_LIBZ
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
            SHRD_SET_HDR (cbuf, code, status, devnum, id, newlen + off);
            shrdtrc(dev,"server_send %2.2x %2.2x %2.2x %d %d (compressed)\n",
                   code,status,devnum,id,(int)newlen+off);
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
        WRMSG(HHC00729, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, id, strerror(HSO_errno));
        dev->shrd[ix]->disconnect = 1;
    }

    return rc;

} /* serverSend */

/*-------------------------------------------------------------------
 * Determine if a client can be disconnected (server side)
 *-------------------------------------------------------------------*/
static int serverDisconnectable (DEVBLK *dev, int ix) {

    if (dev->shrd[ix]->waiting || dev->shrd[ix]->pending
     || dev->shioactive == dev->shrd[ix]->id)
        return 0;
    else
        return 1;
} /* serverDisconnectable */

/*-------------------------------------------------------------------
 * Disconnect a client (server side)
 * dev->lock *must* be held
 *-------------------------------------------------------------------*/
static void serverDisconnect (DEVBLK *dev, int ix) {
int id;                                 /* Client identifier         */
int i;                                  /* Loop index                */

    id = dev->shrd[ix]->id;

//FIXME: Handle a disconnected busy client better
//       Perhaps a disconnect timeout value... this will
//       give the client time to reconnect.

    /* If the device is active by the client then extricate it.
       This is *not* a good situation */
    if (dev->shioactive == id)
    {
        WRMSG(HHC00730, "W", SSID_TO_LCSS(dev->ssid), dev->devnum, id, dev->reserved ? "reserved" : "");

        /* Call the I/O release exit if reserved by this client */
        if (dev->reserved && dev->hnd->release)
            (dev->hnd->release) (dev);

        /* Call the channel program end exit */
        if (dev->hnd->end)
            (dev->hnd->end) (dev);

        /* Reset any `waiting' bits */
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

    WRMSG(HHC00731, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->shrd[ix]->ipaddr, id);

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
 * Connect a new client
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
struct timeval  wait;                   /* Wait time for select      */
BYTE            hdr[SHRD_HDR_SIZE + 65536];  /* Header + buffer      */
BYTE           *buf = hdr + SHRD_HDR_SIZE;   /* Buffer               */
char           *ipaddr = NULL;          /* IP addr of connected peer */
char            threadname[40];

    csock = *(int*)psock;
    free (psock);
    ipaddr = clientip(csock);

    shrdtrc(dev,"server_connect %s sock %d\n",ipaddr,csock);

    rc = recvData(csock, hdr, buf, 65536, 1);
    if (rc < 0)
    {
        WRMSG(HHC00732, "E", ipaddr);
        close_socket (csock);
        return NULL;
    }
    SHRD_GET_HDR (hdr, cmd, flag, devnum, id, len);

    /* Error if not a connect request */
    if (id == 0 && cmd != SHRD_CONNECT)
    {
        serverError (NULL, -csock, SHRD_ERROR_NOTCONN, cmd,
                     "not a connect request");
        close_socket (csock);
        return NULL;
    }

    /* Locate the device */
    dev = findDevice (devnum);

    /* Error if device not found */
    if (dev == NULL)
    {
        serverError (NULL, -csock, SHRD_ERROR_NODEVICE, cmd,
                     "device not found");
        close_socket (csock);
        return NULL;
    }

    /* Obtain the device lock */
    obtain_lock (&dev->lock);

    /* Find an available slot for the connection */
    rc = serverLocate (dev, id, &ix);

    /* Error if already connected */
    if (rc >= 0)
    {
        release_lock (&dev->lock);
        serverError (NULL, -csock, SHRD_ERROR_NODEVICE, cmd,
                     "already connected");
        close_socket (csock);
        return NULL;
    }

    /* Error if no available slot */
    if (ix < 0)
    {
        release_lock (&dev->lock);
        serverError (NULL, -csock, SHRD_ERROR_NOTAVAIL, cmd,
                     "too many connections");
        close_socket (csock);
        return NULL;
    }

    /* Obtain SHRD block */
    dev->shrd[ix] = calloc (sizeof(SHRD), 1);

    /* Error if not obtained */
    if (dev->shrd[ix] == NULL)
    {
        release_lock (&dev->lock);
        serverError (NULL, -csock, SHRD_ERROR_NOMEM, cmd,
                     "calloc() failure");
        close_socket (csock);
        return NULL;
    }

    /* Initialize the SHRD block */
    dev->shrd[ix]->pending = 1;
    dev->shrd[ix]->havehdr = 1;
    if (id == 0) id = serverId (dev);
    dev->shrd[ix]->id = id;
    dev->shrd[ix]->fd = csock;
    dev->shrd[ix]->ipaddr = strdup(ipaddr);
    dev->shrd[ix]->time = time (NULL);
    dev->shrd[ix]->purgen = -1;
    dev->shrdconn++;
    SHRD_SET_HDR (dev->shrd[ix]->hdr, cmd, flag, devnum, id, len);

    WRMSG (HHC00733, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, ipaddr, id);

    /* Return if device thread already active */
    if (dev->shrdtid)
    {
        if (dev->shrdwait)
        {
            signal_thread (dev->shrdtid, SIGUSR2);
        }
        release_lock (&dev->lock);
        return NULL;
    }
    dev->shrdtid = thread_id();

    /* This thread will be the shared device thread */
    MSGBUF(threadname, "Shared device(%1d:%04X)", SSID_TO_LCSS(dev->ssid), dev->devnum);
    WRMSG (HHC00100, "I", thread_id(), get_thread_priority(0), threadname);

    while (dev->shrdconn)
    {
        FD_ZERO (&selset);
        maxfd = -1;

        /* Get the current time */
        now = time (NULL);

        for (ix = 0; ix < SHARED_MAX_SYS; ix++)
        {
            if (dev->shrd[ix])
            {
                /* Exit loop if pending and not waiting */
                if (dev->shrd[ix]->pending && !dev->shrd[ix]->waiting)
                    break;

                /* Disconnect if not a valid socket */
                if ( !socket_is_socket( dev->shrd[ix]->fd ) )
                    dev->shrd[ix]->disconnect = 1;

                /* See if the connection can be timed out */
                else if (now - dev->shrd[ix]->time > SHARED_TIMEOUT
                 && serverDisconnectable (dev, ix))
                    dev->shrd[ix]->disconnect = 1;

                /* Disconnect if the disconnect bit is set */
                if (dev->shrd[ix]->disconnect)
                    serverDisconnect (dev, ix);

                /* Otherwise set the fd if not waiting */
                else if (!dev->shrd[ix]->waiting)
                {
                    FD_SET (dev->shrd[ix]->fd, &selset);
                    if (dev->shrd[ix]->fd >= maxfd)
                        maxfd = dev->shrd[ix]->fd + 1;
                    shrdtrc(dev,"select   set %d id=%d\n",dev->shrd[ix]->fd,dev->shrd[ix]->id);
                }
            }

        }

        /* Wait for a request if no pending requests */
        if (ix >= SHARED_MAX_SYS)
        {
            /* Exit thread if nothing to select */
            if (maxfd < 0) continue;

            /* Wait for a file descriptor to become busy */
            wait.tv_sec = 10; /*SHARED_SELECT_WAIT;*/
            wait.tv_usec = 0;
            release_lock (&dev->lock);

            dev->shrdwait = 1;
            rc = select ( maxfd, &selset, NULL, NULL, &wait );
            dev->shrdwait = 0;

            obtain_lock (&dev->lock);

            shrdtrc(dev,"select rc %d\n",rc);

            if (rc == 0) continue;

            if (rc < 0 )
            {
                if (HSO_errno == HSO_EINTR || HSO_errno == HSO_EBADF) continue;
                WRMSG(HHC00735, "E", "select()", strerror(HSO_errno));
                break;
            }

            /* Find any pending requests */
            for (ix = 0; ix < SHARED_MAX_SYS; ix++)
            {
                if (dev->shrd[ix]
                 && FD_ISSET(dev->shrd[ix]->fd, &selset))
                {
                    dev->shrd[ix]->pending = 1;
                    shrdtrc(dev,"select isset %d id=%d\n",dev->shrd[ix]->fd,dev->shrd[ix]->id);
                }
            }
            continue;
        }

        /* Found a pending request */
        release_lock (&dev->lock);

        shrdtrc(dev,"select ready %d id=%d\n",dev->shrd[ix]->fd,dev->shrd[ix]->id);

        if (dev->shrd[ix]->havehdr)
        {
            /* Copy the saved start/resume packet */
            memcpy (hdr, dev->shrd[ix]->hdr, SHRD_HDR_SIZE);
            dev->shrd[ix]->havehdr = dev->shrd[ix]->waiting = 0;
        }
        else
        {
            /* Read the request packet */
            rc = recvData (dev->shrd[ix]->fd, hdr, buf, 65536, 1);
            if (rc < 0)
            {
                WRMSG(HHC00734, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->shrd[ix]->ipaddr, dev->shrd[ix]->id);
                dev->shrd[ix]->disconnect = 1;
                dev->shrd[ix]->pending = 0;
                obtain_lock (&dev->lock);
                continue;
            }
        }

        /* Process the request */
        serverRequest (dev, ix, hdr, buf);

        obtain_lock (&dev->lock);

        /* If the `waiting' bit is on then the start/resume request
           failed because the device is busy on some other remote
           system.  We only need to save the header because the data
           is ignored for start/resume.
        */
        if (dev->shrd[ix]->waiting)
        {
            memcpy (dev->shrd[ix]->hdr, hdr, SHRD_HDR_SIZE);
            dev->shrd[ix]->havehdr = 1;
        }
        else
            dev->shrd[ix]->pending = 0;
    }

    dev->shrdtid = 0;
    release_lock (&dev->lock);

    WRMSG(HHC00101, "I", thread_id(), get_thread_priority(0), threadname);

    return NULL;

} /* serverConnect */

/*-------------------------------------------------------------------
 * General trace routine for shared devices
 *-------------------------------------------------------------------*/
static void shrdtrc (DEVBLK *dev, char *msg, ...)
{
int             dt;
struct timeval  tv;
SHRD_TRACE      s;
va_list         vl;

    dt = (dev != NULL && (dev->ccwtrace||dev->ccwstep));
    if (dt == 0 && sysblk.shrdtrace == 0) return;

    va_start(vl,msg);
    gettimeofday(&tv, NULL);
    sprintf ((char *)s,
            "%6.6ld" "." "%6.6ld %4.4X:",
            (long)tv.tv_sec, (long)tv.tv_usec, dev ? dev->devnum : 0);
    vsnprintf ((char *)s + strlen(s), sizeof(s) - strlen(s),
            msg, vl);
    if (dt)
    {
        WRMSG(HHC00743, "I", s+14);
    }
    if (sysblk.shrdtrace)
    {
        SHRD_TRACE *p = sysblk.shrdtracep++;
        if (p >= sysblk.shrdtracex)
        {
            p = sysblk.shrdtrace;
            sysblk.shrdtracep = p + 1;
        }
        if (p) memcpy(p, s, sizeof(*p));
    }

} /* shrdtrc */


/*-------------------------------------------------------------------
 * Shared device server
 *-------------------------------------------------------------------*/
LOCK shrdlock;
COND shrdcond;

static void shared_device_manager_shutdown(void * unused)
{
    UNREFERENCED(unused);

    if(sysblk.shrdport)
    {
        sysblk.shrdport = 0;

        if (sysblk.shrdtid)
        {
            signal_thread (sysblk.shrdtid, SIGUSR2);

            obtain_lock(&shrdlock);
            timed_wait_condition_relative_usecs(&shrdcond,&shrdlock,2*1000*1000,NULL);
            release_lock(&shrdlock);
        }
    }
}

DLL_EXPORT void *shared_server (void *arg)
{
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
char                    threadname[40];

    UNREFERENCED(arg);

    MSGBUF(threadname, "Shared device server %d.%d", SHARED_VERSION, SHARED_RELEASE);

    /* Display thread started message on control panel */
    WRMSG (HHC00100, "I", thread_id(), get_thread_priority(0), threadname);

    /* Obtain a internet socket */
    lsock = socket (AF_INET, SOCK_STREAM, 0);

    if (lsock < 0)
    {
        WRMSG(HHC00735, "E", "socket()", strerror(HSO_errno));
        return NULL;
    }

    /* Obtain a unix socket */
#if defined( HAVE_SYS_UN_H )
    usock = socket (AF_UNIX, SOCK_STREAM, 0);
    if (usock < 0)
    {
        WRMSG(HHC00735, "W", "socket()", strerror(HSO_errno));
    }
#else
    usock = -1;
#endif

    /* Allow previous instance of socket to be reused */
    optval = 1;
    setsockopt (lsock, SOL_SOCKET, SO_REUSEADDR,
                (GETSET_SOCKOPT_T*)&optval, sizeof(optval));

    /* Prepare the sockaddr structure for the bind */
    memset( &server, 0, sizeof(server) );
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = sysblk.shrdport;
    server.sin_port = htons(server.sin_port);

    /* Attempt to bind the internet socket to the port */
    while (sysblk.shrdport)
    {
        rc = bind (lsock, (struct sockaddr *)&server, sizeof(server));
        if (rc == 0 || HSO_errno != HSO_EADDRINUSE) break;
        WRMSG (HHC00736, "W", sysblk.shrdport);
        SLEEP(10);
    } /* end while */

    if (rc != 0 || !sysblk.shrdport)
    {
        WRMSG(HHC00735, "E", "bind()", strerror(HSO_errno));
        close_socket(lsock); close_socket(usock);
        return NULL;
    }

#if defined( HAVE_SYS_UN_H )
    /* Bind the unix socket */
    if (usock >= 0)
    {
        userver.sun_family = AF_UNIX;
        sprintf(userver.sun_path, "/tmp/hercules_shared.%d", sysblk.shrdport);
        unlink(userver.sun_path);
        fchmod (usock, 0700);

        rc = bind (usock, (struct sockaddr *)&userver, sizeof(userver));

        if (rc < 0)
        {
            WRMSG(HHC00735, "W", "bind()", strerror(errno));
            close(usock);
            usock = -1;
        }
    }
#endif // defined( HAVE_SYS_UN_H )

    /* Put the sockets into listening state */
    rc = listen (lsock, SHARED_MAX_SYS);

    if (rc < 0)
    {
        WRMSG(HHC00735, "E", "listen()", strerror(HSO_errno));
        close_socket(lsock); close_socket(usock);
        return NULL;
    }

    if (usock >= 0)
    {
        rc = listen (usock, SHARED_MAX_SYS);

        if (rc < 0)
        {
            WRMSG(HHC00735, "W", "listen()", strerror(HSO_errno));
            close_socket(usock);
            usock = -1;
        }
    }

    csock = -1;
    if (lsock < usock)
        hi = usock + 1;
    else
        hi = lsock + 1;

    WRMSG(HHC00737, "I", sysblk.shrdport);

    initialize_lock (&shrdlock);
    initialize_condition(&shrdcond);

    hdl_adsc("shared_device_manager_shutdown",shared_device_manager_shutdown, NULL);

    /* Handle connection requests and attention interrupts */
    while (sysblk.shrdport)
    {
        /* Initialize the select parameters */
        FD_ZERO (&selset);
        FD_SET (lsock, &selset);
        if (usock >= 0)
            FD_SET (usock, &selset);

        /* Wait for a file descriptor to become ready */
        rc = select ( hi, &selset, NULL, NULL, NULL );

        if (rc == 0) continue;

        if (rc < 0 )
        {
            if (HSO_errno == HSO_EINTR) continue;
            WRMSG(HHC00735, "E", "select()", strerror(HSO_errno));
            break;
        }

        /* If a client connection request has arrived then accept it */
        if (FD_ISSET(lsock, &selset))
            rsock = lsock;
        else if (usock >= 0 && FD_ISSET(usock, &selset))
            rsock = usock;
        else
            rsock = -1;

        if (rsock > 0)
        {
            /* Accept a connection and create conversation socket */
            csock = accept (rsock, NULL, NULL);
            if (csock < 0)
            {
                WRMSG(HHC00735, "E", "accept()", strerror(HSO_errno));
                continue;
            }

            psock = malloc (sizeof (csock));
            if (psock == NULL)
            {
                char buf[40];
                MSGBUF(buf, "malloc(%d)", (int)sizeof(csock));
                WRMSG(HHC00735, "E", buf, strerror(HSO_errno));
                close_socket (csock);
                continue;
            }
            *psock = csock;

            /* Create a thread to complete the client connection */
            rc = create_thread (&tid, DETACHED,
                                serverConnect, psock, "serverConnect");
            if(rc)
            {
                WRMSG(HHC00102, "E", strerror(rc));
                close_socket (csock);
            }

        } /* end if(rsock) */


    } /* end while */

    /* Close the listening sockets */
    close_socket (lsock);
#if defined( HAVE_SYS_UN_H )
    if (usock >= 0)
    {
        close_socket (usock);
        unlink(userver.sun_path);
    }
#endif

    signal_condition(&shrdcond);

    if ( !sysblk.shutdown )
        hdl_rmsc(shared_device_manager_shutdown, NULL);

    sysblk.shrdtid = 0;

    WRMSG (HHC00101, "I", thread_id(), get_thread_priority(0), threadname);

    return NULL;

} /* end function shared_server */

/*-------------------------------------------------------------------
 * Shared device command processor
 *-------------------------------------------------------------------*/
DLL_EXPORT int shared_cmd(int argc, char *argv[], char *cmdline)
{
    char buf[256];
    char *kw, *op, c;
    char *strtok_str = NULL;

    UNREFERENCED(cmdline);

    /* Get keyword and operand */
    if (argc != 2 || strlen(argv[1]) > 255)
    {
        WRMSG (HHC00738, "E");
        return 0;
    }
    strlcpy( buf, argv[1], sizeof(buf) );
    kw = strtok_r (buf, "=", &strtok_str );
    op = strtok_r (NULL, " \t", &strtok_str );

    if (kw == NULL)
    {
        WRMSG (HHC00739, "E");
        return 0;
    }

    if (strcasecmp(kw, "trace") == 0)
    {
        int n;
        SHRD_TRACE *s, *p, *x, *i;
        s = sysblk.shrdtrace;
        p = sysblk.shrdtracep;
        x = sysblk.shrdtracex;
        n = sysblk.shrdtracen;

        /* Get a new trace table if an operand was specified */
        if (op)
        {
            if (sscanf (op, "%d%c", &n, &c) != 1)
            {
                WRMSG (HHC00740, "E", op);
                return 0;
            }
            if (s != NULL)
            {
                sysblk.shrdtrace = sysblk.shrdtracex = sysblk.shrdtracep = NULL;
                SLEEP (1);
                free (s);
            }
            sysblk.shrdtrace = sysblk.shrdtracex = sysblk.shrdtracep = NULL;
            sysblk.shrdtracen = 0;
            if (n > 0)
            {
                s = calloc( (size_t)n, sizeof(SHRD_TRACE) );
                if (s == NULL)
                {
                    char buf[40];
                    MSGBUF(buf, "calloc(%d, %d)", (int)n, (int)sizeof(SHRD_TRACE) );
                    WRMSG (HHC00735, "E", buf, strerror(errno));
                    return 0;
                }
                sysblk.shrdtracen = n;
                sysblk.shrdtrace = sysblk.shrdtracep = s;
                sysblk.shrdtracex = s + n;
            }
            return 0;
        }
        /* Print the trace table */
        sysblk.shrdtrace = sysblk.shrdtracex = sysblk.shrdtracep = NULL;
        i = p;
        SLEEP(1);
        do {
            if (i[0] != '\0') WRMSG(HHC00743, "I", (char *)i);
            if (++i >= x) i = s;
        } while (i != p);
        memset(s, 0, n * sizeof(SHRD_TRACE) );
        sysblk.shrdtrace = s;
        sysblk.shrdtracep = s;
        sysblk.shrdtracex = x;
        sysblk.shrdtracen = n;
    }
    else
    {
        WRMSG (HHC00741, "E", kw);
        return 0;
    }
    return 0;
}

DEVHND shared_ckd_device_hndinfo = {
        &shared_ckd_init,              /* Device Initialisation      */
        &ckddasd_execute_ccw,          /* Device CCW execute         */
        &shared_ckd_close,             /* Device Close               */
        &ckddasd_query_device,         /* Device Query               */
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
        &ckddasd_hsuspend,             /* Hercules suspend           */
        &ckddasd_hresume               /* Hercules resume            */
};

DEVHND shared_fba_device_hndinfo = {
        &shared_fba_init,              /* Device Initialisation      */
        &fbadasd_execute_ccw,          /* Device CCW execute         */
        &shared_fba_close,             /* Device Close               */
        &fbadasd_query_device,         /* Device Query               */
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
        &fbadasd_hsuspend,             /* Hercules suspend           */
        &fbadasd_hresume               /* Hercules resume            */
};

#else // !defined( OPTION_SHARED_DEVICES )

int shared_update_notify (DEVBLK *dev, int block)
{
 UNREFERENCED(dev);
 UNREFERENCED(block);
 return 0;
}
int shared_ckd_init (DEVBLK *dev, int argc, char *argv[] )
{
 UNREFERENCED(dev);
 UNREFERENCED(argc);
 UNREFERENCED(argv);
 return -1;
}
int shared_fba_init (DEVBLK *dev, int argc, char *argv[] )
{
 UNREFERENCED(dev);
 UNREFERENCED(argc);
 UNREFERENCED(argv);
 return -1;
}
void *shared_server (void *arg)
{
 UNREFERENCED(arg);
 WRMSG (HHC00742, "E");
 return NULL;
}
int shared_cmd(int argc, char *argv[], char *cmdline)
{
 UNREFERENCED(argc);
 UNREFERENCED(argv);
 UNREFERENCED(cmdline);
 WRMSG (HHC00742, "E");
 return 0;
}
#endif /* defined(OPTION_SHARED_DEVICES) */
