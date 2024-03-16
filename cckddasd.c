/* CCKDDASD.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Greg Smith, 2002-2012                  */
/*              (C) and others 2013-2022                             */
/*                                                                   */
/*              CCKD (Compressed CKD) Device Handler                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device functions for compressed emulated     */
/* count-key-data direct access storage devices.                     */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _CCKDDASD_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "dasdblks.h"
#include "cckddasd.h"
#include "ccwarn.h"

DISABLE_GCC_UNUSED_SET_WARNING;

/*-------------------------------------------------------------------*/
/*                       Global Variables                            */
/*-------------------------------------------------------------------*/

DLL_EXPORT  CCKDBLK  cckdblk;       /* cckd global area */

char*         compname   [] = { "none", "zlib", "bzip2" };
CCKD_L2ENT    empty_l2   [ CKD_NULLTRK_FMTMAX + 1 ][256] = {0};
CCKD64_L2ENT  empty64_l2 [ CKD_NULLTRK_FMTMAX + 1 ][256] = {0};

DLL_EXPORT int gctab[5] =   /* Garbage Collection parameters */
{
    4096,                   /* critical  50%   - 100%    */
    2048,                   /* severe    25%   -  50%    */
    1024,                   /* moderate  12.5% -  25%    */
    512,                    /* light      6.3% -  12.5%  */
    256                     /* none       0%   -   6.3%  */
};

/*-------------------------------------------------------------------*/
/* CCKD global initialization                                        */
/*-------------------------------------------------------------------*/
int cckd_dasd_init( int argc, BYTE* argv[] )
{
    int  i, j;                          /* Loop indexes              */

    UNREFERENCED(argc);
    UNREFERENCED(argv);

    if (memcmp( &cckdblk.id, CCKDBLK_ID, sizeof( cckdblk.id )) == 0)
        return 0;

    /* Clear the cckdblk */

    memset( &cckdblk, 0, sizeof( cckdblk ));

    /* Initialize locks and conditions */

    memcpy( &cckdblk.id, CCKDBLK_ID, sizeof( cckdblk.id ));

    initialize_lock( &cckdblk.gclock  );
    initialize_lock( &cckdblk.ralock  );
    initialize_lock( &cckdblk.wrlock  );
    initialize_lock( &cckdblk.devlock );
    initialize_lock( &cckdblk.trclock );

    initialize_condition( &cckdblk.gccond   );
    initialize_condition( &cckdblk.racond   );
    initialize_condition( &cckdblk.wrcond   );
    initialize_condition( &cckdblk.devcond  );
    initialize_condition( &cckdblk.termcond );

    /* Initialize trace table */

    cckdblk.itrace     = calloc( CCKD_DEF_NUM_TRACE, sizeof( CCKD_ITRACE ));
    cckdblk.itracep    = cckdblk.itrace;
    cckdblk.itracex    = cckdblk.itrace + CCKD_DEF_NUM_TRACE;
    cckdblk.itracen    = CCKD_DEF_NUM_TRACE;
    cckdblk.itracec    = 0;

    /* Initialize some other variables */

    cckdblk.ranbr      = CCKD_DEF_RA_SIZE;
    cckdblk.ramax      = CCKD_DEF_RA;
    cckdblk.wrmax      = CCKD_DEF_WRITER;
    cckdblk.gcmax      = CCKD_DEF_GCOL;
    cckdblk.gcint      = CCKD_DEF_GCINT;
    cckdblk.gcparm     = CCKD_DEF_GCPARM;
    cckdblk.readaheads = CCKD_DEF_READAHEADS;
    cckdblk.freepend   = CCKD_DEF_FREEPEND;

#if defined( HAVE_ZLIB )
    cckdblk.comps     |= CCKD_COMPRESS_ZLIB;
#endif
#if defined( CCKD_BZIP2 )
    cckdblk.comps     |= CCKD_COMPRESS_BZIP2;
#endif
    cckdblk.comp       = 0xff;
    cckdblk.compparm   = -1;

    /* Set the writer thread's priority just BELOW the CPU threads'
       in order to minimize any potential impact from compression.
    */
    cckdblk.wrprio = sysblk.cpuprio - 1;

    /* Initialize the readahead queue */

    cckdblk.ra1st  = -1;
    cckdblk.ralast = -1;
    cckdblk.rafree =  0;

    for (i=0; i < cckdblk.ranbr; i++)
        cckdblk.ra[i].ra_idxnxt = i + 1;

    cckdblk.ra[cckdblk.ranbr - 1].ra_idxnxt = -1;

    /* Clear the empty L2 tables */

    for (i=0; i <= CKD_NULLTRK_FMTMAX; i++)
        for (j=0; j < 256; j++)
        {
            empty_l2   [i][j] . L2_trkoff = 0;
            empty_l2   [i][j] . L2_len    = i;
            empty_l2   [i][j] . L2_size   = i;

            empty64_l2 [i][j] . L2_trkoff = 0;
            empty64_l2 [i][j] . L2_len    = i;
            empty64_l2 [i][j] . L2_size   = i;
        }

    return 0;

} /* end function cckd_dasd_init */

/*-------------------------------------------------------------------*/
/* Perform global CCKD dasd termination (if appropriate)             */
/*-------------------------------------------------------------------*/
void cckd_dasd_term_if_appropriate()
{
    int max;

    /* Check if it's time to terminate yet */
    obtain_lock( &cckdblk.devlock );
    {
        if (cckdblk.dev1st)
        {
            /* Not time yet; return without doing anything */
            release_lock( &cckdblk.devlock );
            return;
        }
        /* cckdblk.dev1st == NULL: time to globally terminate */
    }
    release_lock( &cckdblk.devlock );

    /* Terminate all readahead threads... */
    obtain_lock( &cckdblk.ralock );
    {
        max = cckdblk.ramax;    /* Save current value */
        cckdblk.ramax = 0;      /* signal   all threads to terminate */
        while (cckdblk.ras)     /* wait for all threads to terminate */
        {
            broadcast_condition( &cckdblk.racond );
            wait_condition( &cckdblk.termcond, &cckdblk.ralock );
        }
        cckdblk.ramax = max;    /* Restore orignal value */
    }
    release_lock( &cckdblk.ralock );

    /* Terminate all garbage collection threads... */
    obtain_lock( &cckdblk.gclock );
    {
        max = cckdblk.gcmax;    /* Save current value */
        cckdblk.gcmax = 0;      /* signal   all threads to terminate */
        while (cckdblk.gcs)     /* wait for all threads to terminate */
        {
            broadcast_condition( &cckdblk.gccond );
            wait_condition( &cckdblk.termcond, &cckdblk.gclock );
        }
        cckdblk.gcmax = max;    /* Restore orignal value */
    }
    release_lock( &cckdblk.gclock );

    /* Terminate all writer threads... */
    obtain_lock( &cckdblk.wrlock );
    {
        max = cckdblk.termwr;   /* Save current value */
        cckdblk.termwr = 1;     /* signal   all threads to terminate */
        while (cckdblk.wrs)     /* wait for all threads to terminate */
        {
            broadcast_condition( &cckdblk.wrcond );
            wait_condition( &cckdblk.termcond, &cckdblk.wrlock );
        }
        cckdblk.termwr = max;    /* Restore orignal value */
    }
    release_lock( &cckdblk.wrlock );

} /* end function cckd_dasd_term */

/*-------------------------------------------------------------------*/
/* Compressed CKD dasd initialization                                */
/*-------------------------------------------------------------------*/
int cckd_dasd_init_handler ( DEVBLK *dev, int argc, char *argv[] )
{
CCKD_EXT    *cckd;                      /* -> cckd extension         */
DEVBLK      *dev2;                      /* -> device in cckd queue   */
int          i;                         /* Counter                   */
int          fdflags;                   /* File flags                */
char         buf[32];                   /* Work buffer                      */

    UNREFERENCED( argc );
    UNREFERENCED( argv );

    /* Initialize the global cckd block if necessary */
    if (memcmp( &cckdblk.id, CCKDBLK_ID, sizeof( cckdblk.id )))
        cckd_dasd_init( 0, NULL );

    /* Obtain area for cckd extension */
    dev->cckd_ext = cckd = cckd_calloc( dev, "ext", 1, sizeof( CCKD_EXT ));
    if (cckd == NULL)
        return -1;

    /* Initialize locks and conditions */

    initialize_lock( &cckd->cckdiolock );
    MSGBUF( buf,    "&cckd->cckdiolock %1d:%04X", LCSS_DEVNUM );
    set_lock_name(   &cckd->cckdiolock, buf );

    initialize_lock( &cckd->filelock );
    MSGBUF( buf,    "&cckd->filelock %1d:%04X", LCSS_DEVNUM );
    set_lock_name(   &cckd->filelock, buf );

    initialize_condition( &cckd->cckdiocond );

    /* Initialize some variables */
    obtain_lock( &cckd->filelock );
    {
        cckd->L1idx = cckd->sfx = cckd->L2_active = -1;
        dev->cache = cckd->free_idx1st = -1;
        cckd->fd[0] = dev->fd;
        fdflags = get_file_accmode_flags( dev->fd );
        cckd->open[0] = (fdflags & O_RDWR) ? CCKD_OPEN_RW : CCKD_OPEN_RO;
        for (i = 1; i <= CCKD_MAX_SF; i++)
        {
            cckd->fd[i] = -1;
            cckd->open[i] = CCKD_OPEN_NONE;
        }
        cckd->cckd_maxsize = CCKD_MAXSIZE;

        /* call the chkdsk function */
        if (cckd_chkdsk (dev, 0) < 0)
            return -1;

        /* Perform initial read */
        if (cckd_read_init (dev) < 0)
            return -1;
        if (cckd->fbadasd) dev->ckdtrksz = CFBA_BLKGRP_SIZE;

        /* open the shadow files */
        if (cckd_sf_init (dev) < 0)
        {
            WRMSG (HHC00300, "E", LCSS_DEVNUM);
            return -1;
        }

        /* Update the device handler routines */
        if (cckd->ckddasd)
            dev->hnd = &cckd_dasd_device_hndinfo;
        else
            dev->hnd = &cfba_dasd_device_hndinfo;
    }
    release_lock( &cckd->filelock );

    /* Insert the device into the cckd device queue */
    cckd_lock_devchain(1);
    for (cckd = NULL, dev2 = cckdblk.dev1st; dev2; dev2 = cckd->devnext)
        cckd = dev2->cckd_ext;
    if (cckd) cckd->devnext = dev;
    else cckdblk.dev1st = dev;
    cckd_unlock_devchain();

    cckdblk.batch = dev->batch;

    if (cckdblk.batch)
    {
        cckdblk.nostress = 1;
        cckdblk.freepend = 0;
        cckdblk.linuxnull = 1;
    }

    return 0;
} /* end function cckd_dasd_init_handler */

/*-------------------------------------------------------------------*/
/* Close a Compressed CKD Device                                     */
/*-------------------------------------------------------------------*/
int cckd_dasd_close_device (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc, i;                  /* Return code, Loop index   */

    if (dev->cckd64)
        return cckd64_dasd_close_device( dev );

    cckd = dev->cckd_ext;

    /* Wait for readaheads to finish */
    obtain_lock(&cckdblk.ralock);
    cckd->stopping = 1;
    while (cckd->ras)
    {
        release_lock(&cckdblk.ralock);
        USLEEP(1);
        obtain_lock(&cckdblk.ralock);
    }
    release_lock(&cckdblk.ralock);

    /* Flush the cache and wait for the writes to complete */
    obtain_lock( &cckd->cckdiolock );
    {
        cckd->stopping = 1;
        cckd_flush_cache( dev );
        while (cckd->wrpending || cckd->cckdioact)
        {
            cckd->cckdwaiters++;
            rc = timed_wait_condition_relative_usecs(
                &cckd->cckdiocond, &cckd->cckdiolock, 1000000, NULL );
            cckd->cckdwaiters--;
            cckd_flush_cache( dev );
            if (EINTR == rc)
                continue;
            /* Prevent rare but possible hang at shutdown */
            if (1
                && ETIMEDOUT == rc
                && (cckd->wrpending || cckd->cckdioact)
                && sysblk.shutdown
            )
            {
                CCKD_TRACE( "closing device while wrpending=%d cckdioact=%d",
                    cckd->wrpending, cckd->cckdioact );
                // "%1d:%04X CCKD file %s: closing device while wrpending=%d cckdioact=%d"
                WRMSG( HHC00381, "W", LCSS_DEVNUM, dev->filename,
                    cckd->wrpending, cckd->cckdioact );
                break;
            }
        }
        broadcast_condition( &cckd->cckdiocond );
        cckd_purge_cache( dev );
        cckd_purge_l2( dev );
        dev->bufcur = -1;
        dev->cache  = -1;
        if (cckd->newbuf)
            cckd_free( dev, "newbuf", cckd->newbuf );
    }
    release_lock( &cckd->cckdiolock );

    /* Remove the device from the cckd queue */
    cckd_lock_devchain(1);
    if (dev == cckdblk.dev1st) cckdblk.dev1st = cckd->devnext;
    else
    {
        DEVBLK *dev2 = cckdblk.dev1st;
        CCKD_EXT *cckd2 = dev2->cckd_ext;
        while (cckd2->devnext != dev)
        {
           dev2 = cckd2->devnext; cckd2 = dev2->cckd_ext;
        }
        cckd2->devnext = cckd->devnext;
    }
    cckd_unlock_devchain();

    /* harden the file */
    obtain_lock( &cckd->filelock );
    {
        cckd_harden (dev);

        /* close the shadow files */
        for (i = 1; i <= cckd->sfn; i++)
        {
            cckd_close (dev, i);
            cckd->open[i] = 0;
        }

        /* free the level 1 tables */
        for (i = 0; i <= cckd->sfn; i++)
            cckd->L1tab[i] = cckd_free (dev, "l1", cckd->L1tab[i]);

        /* reset the device handler */
        if (cckd->ckddasd)
            dev->hnd = &ckd_dasd_device_hndinfo;
        else
            dev->hnd = &fba_dasd_device_hndinfo;

        /* write some statistics */
        if (!dev->batch && !cckdblk.nosfd)
            cckd_sf_stats (dev);
    }
    release_lock( &cckd->filelock );

    /* If no more devices then perform global termination */
    cckd_dasd_term_if_appropriate();

    /* Destroy the cckd extension's locks and conditions */
    destroy_lock( &cckd->cckdiolock );
    destroy_lock( &cckd->filelock );
    destroy_condition( &cckd->cckdiocond );

    /* free the cckd extension itself */
    dev->cckd_ext = cckd_free (dev, "ext", cckd);

    if (dev->dasdsfn) free (dev->dasdsfn);
    dev->dasdsfn = NULL;

    close (dev->fd);
    dev->fd = -1;

    dev->buf = NULL;
    dev->bufsize = 0;

    /* return to caller */
    return 0;
} /* end function cckd_dasd_close_device */

/*-------------------------------------------------------------------*/
/* Compressed ckd start/resume channel program                       */
/*-------------------------------------------------------------------*/
void cckd_dasd_start (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
U16             devnum = 0;             /* Last active device number */
int             trk = 0;                /* Last active track         */

    if (dev->cckd64)
    {
        cckd64_dasd_start( dev );
        return;
    }

    cckd = dev->cckd_ext;

    CCKD_TRACE( "start i/o file[%d] bufcur %d cache[%d]",
                cckd->sfn, dev->bufcur, dev->cache);

    /* Reset buffer offsets */
    dev->bufoff = 0;
    dev->bufoffhi = cckd->ckddasd ? dev->ckdtrksz : CFBA_BLKGRP_SIZE;

    /* Check for merge */
    obtain_lock(&cckd->cckdiolock);
    if (cckd->merging)
    {
        CCKD_TRACE( "start i/o waiting for merge%s","");
        while (cckd->merging)
        {
            cckd->cckdwaiters++;
            wait_condition (&cckd->cckdiocond, &cckd->cckdiolock);
            cckd->cckdwaiters--;
        }
        dev->bufcur = dev->cache = -1;
    }
    cckd->cckdioact = 1;

    cache_lock(CACHE_DEVBUF);

    if (dev->cache >= 0)
        CCKD_CACHE_GETKEY(dev->cache, devnum, trk);

    /* Check if previous active entry is still valid and not busy */
    if (dev->cache >= 0 && dev->devnum == devnum && dev->bufcur == trk
     && !(cache_getflag(CACHE_DEVBUF, dev->cache) & CCKD_CACHE_IOBUSY))
    {
        /* Make the entry active again */
        cache_setflag (CACHE_DEVBUF, dev->cache, ~0, CCKD_CACHE_ACTIVE);

        /* If the entry is pending write then change it to `updated' */
        if (cache_getflag(CACHE_DEVBUF, dev->cache) & CCKD_CACHE_WRITE)
        {
            cache_setflag (CACHE_DEVBUF, dev->cache, ~CCKD_CACHE_WRITE, CCKD_CACHE_UPDATED);
            cckd->wrpending--;
            if (cckd->cckdwaiters && !cckd->wrpending)
                broadcast_condition (&cckd->cckdiocond);
        }
    }
    else
        dev->bufcur = dev->cache = -1;

    cache_unlock (CACHE_DEVBUF);

    release_lock (&cckd->cckdiolock);

    return;

} /* end function cckd_dasd_start */

/*-------------------------------------------------------------------*/
/* Compressed ckd end/suspend channel program                        */
/*-------------------------------------------------------------------*/
void cckd_dasd_end (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */

    if (dev->cckd64)
    {
        cckd64_dasd_end( dev );
        return;
    }

    cckd = dev->cckd_ext;

    /* Update length if previous image was updated */
    if (dev->bufupd && dev->bufcur >= 0 && dev->cache >= 0)
    {
        dev->buflen = cckd_trklen (dev, dev->buf);
        cache_setval (CACHE_DEVBUF, dev->cache, dev->buflen);
    }

    dev->bufupd = 0;

    CCKD_TRACE( "end i/o bufcur %d cache[%d] waiters %d",
                dev->bufcur, dev->cache, cckd->cckdwaiters);

    obtain_lock (&cckd->cckdiolock);

    cckd->cckdioact = 0;

    /* Make the current entry inactive */
    if (dev->cache >= 0)
    {
        cache_lock (CACHE_DEVBUF);
        cache_setflag (CACHE_DEVBUF, dev->cache, ~CCKD_CACHE_ACTIVE, 0);
        cache_unlock (CACHE_DEVBUF);
    }

    /* Cause writers to start after first update */
    if (cckd->updated && (cckdblk.wrs == 0 || cckd->cckdwaiters != 0))
        cckd_flush_cache (dev);
    else if (cckd->cckdwaiters)
        broadcast_condition (&cckd->cckdiocond);

    release_lock (&cckd->cckdiolock);

} /* end function cckd_dasd_end */

/*-------------------------------------------------------------------*/
/* Open a cckd file                                                  */
/*                                                                   */
/* If O_CREAT is not set and mode is non-zero then the error message */
/* will be supressed.                                                */
/*-------------------------------------------------------------------*/
int cckd_open (DEVBLK *dev, int sfx, int flags, mode_t mode)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             err;                    /* 1 = issue error message   */
char            pathname[MAX_PATH];     /* file path in host format  */

    cckd = dev->cckd_ext;

    err = !((flags & O_CREAT) == 0 && mode != 0);

    if (cckd->fd[sfx] >= 0)
        cckd_close (dev, sfx);

    hostpath(pathname, cckd_sf_name (dev, sfx), sizeof(pathname));
    cckd->fd[sfx] = HOPEN (pathname, flags, mode);
    if (sfx == 0) dev->fd = cckd->fd[sfx];

    if (cckd->fd[sfx] >= 0)
        cckd->open[sfx] = flags & O_RDWR ? CCKD_OPEN_RW :
                          cckd->open[sfx] == CCKD_OPEN_RW ?
                          CCKD_OPEN_RD : CCKD_OPEN_RO;
    else
    {
        if (err)
        {
            WRMSG (HHC00301, "E", LCSS_DEVNUM, sfx, cckd_sf_name (dev, sfx),
                    "open()", strerror(errno));
            CCKD_TRACE( "file[%d] fd[%d] open %s error flags %8.8x mode %8.8x",
                        sfx, cckd->fd[sfx], cckd_sf_name (dev, sfx), flags, mode);
            cckd_print_itrace ();
        }
        cckd->open[sfx] = CCKD_OPEN_NONE;
    }

    CCKD_TRACE( "file[%d] fd[%d] open %s, flags %8.8x mode %8.8x",
                sfx, cckd->fd[sfx], cckd_sf_name (dev, sfx), flags, mode);

    return cckd->fd[sfx];

} /* end function cckd_open */

/*-------------------------------------------------------------------*/
/* Close a cckd file                                                 */
/*-------------------------------------------------------------------*/
int cckd_close (DEVBLK *dev, int sfx)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc = 0;                 /* Return code               */

    cckd = dev->cckd_ext;

    CCKD_TRACE( "file[%d] fd[%d] close %s",
                sfx, cckd->fd[sfx], cckd_sf_name(dev, sfx));

    if (cckd->fd[sfx] >= 0)
        rc = close (cckd->fd[sfx]);

    if (rc < 0)
    {
        WRMSG (HHC00301, "E", LCSS_DEVNUM, sfx, cckd_sf_name (dev, sfx),
                              "close()", strerror(errno));
        cckd_print_itrace ();
    }

    cckd->fd[sfx] = -1;
    if (sfx == 0) dev->fd = -1;

    return rc;

} /* end function cckd_close */

/*-------------------------------------------------------------------*/
/* Read from a cckd file                                             */
/*-------------------------------------------------------------------*/
int cckd_read( DEVBLK* dev, int sfx, off_t off, void* buf, unsigned int len )
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */

    cckd = dev->cckd_ext;

    CCKD_TRACE( "file[%d] fd[%d] read, off 0x%16.16"PRIx64" len %d",
                sfx, cckd->fd[ sfx ], off, len );

    /* Seek to specified offset */
    if (lseek( cckd->fd[ sfx ], off, SEEK_SET ) < 0)
    {
        // "%1d:%04X CCKD file[%d] %s: error in function %s at offset 0x%16.16"PRIX64": %s"
        WRMSG( HHC00302, "E", LCSS_DEVNUM, sfx, cckd_sf_name( dev, sfx ),
            "lseek()", off, strerror( errno ));
        cckd_print_itrace();
        return -1;
    }

    /* Read the data */
    rc = read( cckd->fd[ sfx ], buf, len );
    if (rc < (int)len)
    {
        if (rc < 0)
            // "%1d:%04X CCKD file[%d] %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            WRMSG (HHC00302, "E", LCSS_DEVNUM, sfx, cckd_sf_name (dev, sfx),
                "read()", off, strerror(errno));
        else
        {
            char buf[128];
            MSGBUF( buf, "read incomplete: read %d, expected %d", rc, len );
            // "%1d:%04X CCKD file[%d] %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            WRMSG( HHC00302, "E", LCSS_DEVNUM, sfx, cckd_sf_name( dev, sfx ),
                "read()", off, buf);
        }
        cckd_print_itrace ();
        return -1;
    }

    return rc;

} /* end function cckd_read */

/*-------------------------------------------------------------------*/
/* Write to a cckd file                                              */
/*-------------------------------------------------------------------*/
int cckd_write( DEVBLK* dev, int sfx, off_t off, void* buf, unsigned int len )
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc = 0;                 /* Return code               */

    cckd = dev->cckd_ext;

    CCKD_TRACE( "file[%d] fd[%d] write, off 0x%16.16"PRIx64" len %d",
                sfx, cckd->fd[ sfx ], off, len );

    /* Seek to specified offset */
    if (lseek( cckd->fd[ sfx ], off, SEEK_SET ) < 0)
    {
        // "%1d:%04X CCKD file[%d] %s: error in function %s at offset 0x%16.16"PRIX64": %s"
        WRMSG( HHC00302, "E", LCSS_DEVNUM, sfx, cckd_sf_name( dev, sfx ),
            "lseek()", off, strerror( errno ));
        cckd_print_itrace();
        return -1;
    }

    /* Write the data */
    rc = write( cckd->fd[ sfx ], buf, len );
    if (rc < (int)len)
    {
        if (rc < 0)
            // "%1d:%04X CCKD file[%d] %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            WRMSG( HHC00302, "E", LCSS_DEVNUM, sfx, cckd_sf_name( dev, sfx ),
                "write()", off, strerror( errno ));
        else
        {
            char buf[128];
            MSGBUF( buf, "write incomplete: write %d, expected %d", rc, len );
            // "%1d:%04X CCKD file[%d] %s: error in function %s at offset 0x%16.16"PRIX64": %s"
            WRMSG( HHC00302, "E", LCSS_DEVNUM, sfx, cckd_sf_name( dev, sfx ),
                "write()", off, buf );
        }
        cckd_print_itrace();
        return -1;
    }

    return rc;

} /* end function cckd_write */

/*-------------------------------------------------------------------*/
/* Truncate a cckd file                                              */
/*-------------------------------------------------------------------*/
int cckd_ftruncate(DEVBLK *dev, int sfx, off_t off)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */

    cckd = dev->cckd_ext;

    CCKD_TRACE( "file[%d] fd[%d] ftruncate, off 0x%16.16"PRIx64,
                sfx, cckd->fd[sfx], off);

    /* Truncate the file */
    if (ftruncate (cckd->fd[sfx], off) < 0)
    {
        WRMSG (HHC00302, "E", LCSS_DEVNUM, sfx, cckd_sf_name (dev, sfx),
                    "ftruncate()", off, strerror(errno));
        cckd_print_itrace ();
        return -1;
    }

    return 0;

} /* end function cckd_ftruncate */

/*-------------------------------------------------------------------*/
/*                          malloc                                   */
/*-------------------------------------------------------------------*/
void* cckd_malloc( DEVBLK* dev, char* id, size_t size )
{
    void* p = NULL;

    if (size)
        p = malloc( size );
    CCKD_TRACE( "%s malloc %p len %ld", id, p, (long) size );

    if (!p)
    {
        char buf[64];
        MSGBUF( buf, "malloc( %d )", (int) size );
        // "%1d:%04X CCKD file: error in function %s: %s"
        WRMSG( HHC00303, "E", LCSS_DEVNUM, buf, strerror( errno ));
        cckd_print_itrace();
    }

    return p;
}

/*-------------------------------------------------------------------*/
/*                          calloc                                   */
/*-------------------------------------------------------------------*/
void* cckd_calloc( DEVBLK* dev, char* id, size_t n, size_t size )
{
    void* p = NULL;

    if (n && size)
        p = calloc( n, size );
    CCKD_TRACE( "%s calloc %p len %ld", id, p, n * (long) size );

    if (!p)
    {
        char buf[64];
        MSGBUF( buf, "calloc( %d, %d )", (int) n, (int) size );
        // "%1d:%04X CCKD file: error in function %s: %s"
        WRMSG( HHC00303, "E", LCSS_DEVNUM, buf, strerror( errno ));
        cckd_print_itrace();
    }

    return p;
}

/*-------------------------------------------------------------------*/
/*                          realloc                                  */
/*-------------------------------------------------------------------*/
void* cckd_realloc( DEVBLK *dev, char *id, void* p, size_t size )
{
    void* p2 = NULL;

    // shut up GCC 12 warning about using a pointer after realloc()
    char p_string[33];
    MSGBUF( p_string, "%p", p );

    if (size)
        p2 = realloc( p, size );
    CCKD_TRACE( "%s realloc %s len %ld", id, p_string, (long) size );

    if (!p2)
    {
        char buf[64];
        MSGBUF( buf, "realloc( %s, %d )", p_string, (int) size );
        // "%1d:%04X CCKD file: error in function %s: %s"
        WRMSG( HHC00303, "E", LCSS_DEVNUM, buf, strerror( errno ));
        cckd_print_itrace();
    }

    return p2;
}

/*-------------------------------------------------------------------*/
/*                          free                                     */
/*-------------------------------------------------------------------*/
void* cckd_free( DEVBLK* dev, char* id, void* p )
{
    CCKD_TRACE( "%s free %p", id, p );
    if (p) free( p );
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Compressed ckd read track image                                   */
/*-------------------------------------------------------------------*/
int cckd_read_track (DEVBLK *dev, int trk, BYTE *unitstat)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             len;                    /* Compressed length         */
BYTE           *newbuf;                 /* Uncompressed buffer       */
int             cache;                  /* New active cache entry    */

    cckd = dev->cckd_ext;

    /* Update length if previous image was updated */
    if (dev->bufupd && dev->bufcur >= 0 && dev->cache >= 0)
    {
        dev->buflen = cckd_trklen (dev, dev->buf);
        cache_setval (CACHE_DEVBUF, dev->cache, dev->buflen);
    }

    /* Reset buffer offsets */
    dev->bufoff = 0;
    dev->bufoffhi = dev->ckdtrksz;

    /* Check if reading the same track image */
    if (trk == dev->bufcur && dev->cache >= 0)
    {
        /* Track image may be compressed */
        if ((dev->buf[0] & CCKD_COMPRESS_MASK) != 0
         && (dev->buf[0] & dev->comps) == 0)
        {
            len = cache_getval(CACHE_DEVBUF, dev->cache);
            newbuf = cckd_uncompress (dev, dev->buf, len, dev->ckdtrksz, trk);
            if (newbuf == NULL) {
                ckd_build_sense (dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0);
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                dev->bufcur = dev->cache = -1;
                return -1;
            }
            cache_setbuf (CACHE_DEVBUF, dev->cache, newbuf, dev->ckdtrksz);
            dev->buf     = newbuf;
            dev->buflen  = cckd_trklen (dev, newbuf);
            cache_setval (CACHE_DEVBUF, dev->cache, dev->buflen);
            dev->bufsize = cache_getlen (CACHE_DEVBUF, dev->cache);
            dev->bufupd  = 0;
            CCKD_TRACE( "read  trk   %d uncompressed len %d",
                        trk, dev->buflen);
        }

        dev->comp = dev->buf[0] & CCKD_COMPRESS_MASK;
        if (dev->comp != 0) dev->compoff = CKD_TRKHDR_SIZE;

        return 0;
    }

    CCKD_TRACE( "read  trk   %d (%s)", trk, "asynchronous");

    /* read the new track */
    dev->bufupd = 0;
    cache = cckd_read_trk (dev, trk, 0, unitstat);
    if (cache < 0)
    {
        dev->bufcur = dev->cache = -1;
        return -1;
    }

    dev->cache    = cache;
    dev->buf      = cache_getbuf (CACHE_DEVBUF, dev->cache, 0);
    dev->bufcur   = trk;
    dev->bufoff   = 0;
    dev->bufoffhi = dev->ckdtrksz;
    dev->buflen   = cache_getval (CACHE_DEVBUF, dev->cache);
    dev->bufsize  = cache_getlen (CACHE_DEVBUF, dev->cache);

    dev->comp = dev->buf[0] & CCKD_COMPRESS_MASK;
    if (dev->comp != 0) dev->compoff = CKD_TRKHDR_SIZE;

    /* If the image is compressed then call ourself recursively
       to cause the image to get uncompressed */
    if (dev->comp != 0 && (dev->comp & dev->comps) == 0)
        rc = cckd_read_track (dev, trk, unitstat);
    else
        rc = 0;

    return rc;
} /* end function cckd_read_track */

/*-------------------------------------------------------------------*/
/* Compressed ckd update track image                                 */
/*-------------------------------------------------------------------*/
int cckd_update_track (DEVBLK *dev, int trk, int off,
                       BYTE *buf, int len, BYTE *unitstat)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */

    cckd = dev->cckd_ext;

    /* Error if opened read-only */
    if (dev->ckdrdonly && cckd->sfn == 0)
    {
        ckd_build_sense (dev, SENSE_EC, SENSE1_WRI, 0,FORMAT_1, MESSAGE_0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        dev->bufcur = dev->cache = -1;
        return -1;
    }

    /* If the track is not current or compressed then read it.
       `dev->comps' is set to zero forcing the read routine to
       uncompress the image.                                     */
    if (trk != dev->bufcur || (dev->buf[0] & CCKD_COMPRESS_MASK) != 0)
    {
        dev->comps = 0;
        rc = (dev->hnd->read) (dev, trk, unitstat);
        if (rc < 0)
        {
            dev->bufcur = dev->cache = -1;
            return -1;
        }
    }

    /* Invalid track format if going past buffer end */
    if (off + len > dev->ckdtrksz)
    {
        ckd_build_sense (dev, 0, SENSE1_ITF, 0, 0, 0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        dev->bufcur = dev->cache = -1;
        return -1;
    }

    /* Copy the data into the buffer */
    if (buf && len > 0) memcpy (dev->buf + off, buf, len);

    CCKD_TRACE( "updt  trk   %d offset %d length %d",
                trk, off, len);

    /* Update the cache entry */
    cache_setflag (CACHE_DEVBUF, dev->cache, ~0, CCKD_CACHE_UPDATED | CCKD_CACHE_USED);
    cckd->updated = 1;

    /* Notify the shared server of the update */
    if (!dev->bufupd)
    {
        dev->bufupd = 1;
        shared_update_notify (dev, trk);
    }

    return len;

} /* end function cckd_update_track */

/*-------------------------------------------------------------------*/
/* Return used cylinders                                             */
/*-------------------------------------------------------------------*/
int cckd_used (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             L1idx, l2x;             /* Lookup table indexes      */
int             sfx;                    /* Shadow file suffix        */
CCKD_L2ENT      l2;                     /* Copied level 2 entry      */

    if (dev->cckd64)
        return cckd64_used( dev );

    cckd = dev->cckd_ext;

    obtain_lock( &cckd->filelock );
    {
        /* Find the last used level 1 table entry */
        for (L1idx = cckd->cdevhdr[0].num_L1tab - 1; L1idx > 0; L1idx--)
        {
            sfx = cckd->sfn;
            while (cckd->L1tab[sfx][L1idx] == CCKD_MAXSIZE && sfx > 0) sfx--;
            if (cckd->L1tab[sfx][L1idx]) break;
        }

        /* Find the last used level 2 table entry */
        for (l2x = 255; l2x >= 0; l2x--)
        {
            rc = cckd_read_l2ent (dev, &l2, L1idx * 256 + l2x);
            if (rc < 0 || l2.L2_trkoff != 0) break;
        }
    }
    release_lock( &cckd->filelock );

    return (L1idx * 256 + l2x + dev->ckdheads) / dev->ckdheads;
}

/*-------------------------------------------------------------------*/
/* Compressed fba read block(s)                                      */
/*-------------------------------------------------------------------*/
int cfba_read_block (DEVBLK *dev, int blkgrp, BYTE *unitstat)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             cache;                  /* New active cache entry    */
BYTE           *cbuf;                   /* -> cache buffer           */
BYTE           *newbuf;                 /* Uncompressed buffer       */
int             len;                    /* Compressed length         */
int             maxlen;                 /* Size for cache entry      */

    cckd = dev->cckd_ext;

    if (dev->cache >= 0)
        cbuf = cache_getbuf (CACHE_DEVBUF, dev->cache, 0);
    else
        cbuf = NULL;
    maxlen = CFBA_BLKGRP_SIZE + CKD_TRKHDR_SIZE;

    /* Return if reading the same track image */
    if (blkgrp == dev->bufcur && dev->cache >= 0)
    {
        /* Block group image may be compressed */
        if ((cbuf[0] & CCKD_COMPRESS_MASK) != 0
         && (cbuf[0] & dev->comps) == 0)
        {
            len = cache_getval(CACHE_DEVBUF, dev->cache) + CKD_TRKHDR_SIZE;
            newbuf = cckd_uncompress (dev, cbuf, len, maxlen, blkgrp);
            if (newbuf == NULL) {
                dev->sense[0] = SENSE_EC;
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                dev->bufcur = dev->cache = -1;
                return -1;
            }
            cache_setbuf (CACHE_DEVBUF, dev->cache, newbuf, maxlen);
            cbuf = newbuf;
            dev->buf     = newbuf + CKD_TRKHDR_SIZE;
            dev->buflen  = CFBA_BLKGRP_SIZE;
            cache_setval (CACHE_DEVBUF, dev->cache, dev->buflen);
            dev->bufsize = cache_getlen (CACHE_DEVBUF, dev->cache);
            dev->bufupd  = 0;
            CCKD_TRACE( "read bkgrp  %d uncompressed len %d",
                        blkgrp, dev->buflen);
        }

        dev->comp = cbuf[0] & CCKD_COMPRESS_MASK;

        return 0;
    }

    CCKD_TRACE( "read blkgrp  %d (%s)", blkgrp, "asynchronous");

    /* Read the new blkgrp */
    dev->bufupd = 0;
    cache = cckd_read_trk (dev, blkgrp, 0, unitstat);
    if (cache < 0)
    {
        dev->bufcur = dev->cache = -1;
        return -1;
    }
    dev->cache    = cache;
    cbuf          = cache_getbuf (CACHE_DEVBUF, dev->cache, 0);
    dev->buf      = cbuf + CKD_TRKHDR_SIZE;
    dev->bufcur   = blkgrp;
    dev->bufoff   = 0;
    dev->bufoffhi = CFBA_BLKGRP_SIZE;
    dev->buflen   = CFBA_BLKGRP_SIZE;
    cache_setval  (CACHE_DEVBUF, dev->cache, dev->buflen);
    dev->bufsize  = cache_getlen (CACHE_DEVBUF, dev->cache);
    dev->comp     = cbuf[0] & CCKD_COMPRESS_MASK;

    /* If the image is compressed then call ourself recursively
       to cause the image to get uncompressed.  This is because
      `bufcur' will match blkgrp and `comps' won't match `comp' */
    if (dev->comp != 0 && (dev->comp & dev->comps) == 0)
        rc = cfba_read_block (dev, blkgrp, unitstat);
    else
        rc = 0;

    return rc;
} /* end function cfba_read_block */

/*-------------------------------------------------------------------*/
/* Compressed fba write block(s)                                     */
/*-------------------------------------------------------------------*/
int cfba_write_block (DEVBLK *dev, int blkgrp, int off,
                      BYTE *buf, int len, BYTE *unitstat)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
BYTE           *cbuf;                   /* -> cache buffer           */

    cckd = dev->cckd_ext;

    if (dev->cache >= 0)
        cbuf = cache_getbuf (CACHE_DEVBUF, dev->cache, 0);
    else
        cbuf = NULL;

    /* Read the block group if it's not current or compressed.
       `dev->comps' is set to zero forcing the read routine to
       uncompress the image.                                   */
    if (blkgrp != dev->bufcur || (cbuf[0] & CCKD_COMPRESS_MASK) != 0)
    {
        dev->comps = 0;
        rc = (dev->hnd->read) (dev, blkgrp, unitstat);
        if (rc < 0)
        {
            dev->bufcur = dev->cache = -1;
            return -1;
        }
    }

    /* Copy the data into the buffer */
    if (buf) memcpy (dev->buf + off, buf, len);

    /* Update the cache entry */
    cache_setflag (CACHE_DEVBUF, dev->cache, ~0, CCKD_CACHE_UPDATED|CCKD_CACHE_USED);
    cckd->updated = 1;

    /* Notify the shared server of the update */
    if (!dev->bufupd)
    {
        dev->bufupd = 1;
        shared_update_notify (dev, blkgrp);
    }

    return len;

} /* end function cfba_write_block */

/*-------------------------------------------------------------------*/
/* Return used blocks                                                */
/*-------------------------------------------------------------------*/
int cfba_used (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             L1idx, l2x;             /* Lookup table indexes      */
int             sfx;                    /* Shadow file suffix        */
CCKD_L2ENT      l2;                     /* Copied level 2 entry      */

    if (dev->cckd64)
        return cfba64_used( dev );

    cckd = dev->cckd_ext;

    obtain_lock( &cckd->filelock );
    {
        /* Find the last used level 1 table entry */
        for (L1idx = cckd->cdevhdr[0].num_L1tab - 1; L1idx > 0; L1idx--)
        {
            sfx = cckd->sfn;
            while (cckd->L1tab[sfx][L1idx] == CCKD_MAXSIZE && sfx > 0) sfx--;
            if (cckd->L1tab[sfx][L1idx]) break;
        }

        /* Find the last used level 2 table entry */
        for (l2x = 255; l2x >= 0; l2x--)
        {
            rc = cckd_read_l2ent (dev, &l2, L1idx * 256 + l2x);
            if (rc < 0 || l2.L2_trkoff != 0) break;
        }
    }
    release_lock( &cckd->filelock );

    return (L1idx * 256 + l2x + CFBA_BLKS_PER_GRP) / CFBA_BLKS_PER_GRP;
}

/*-------------------------------------------------------------------*/
/* Read a track image                                                */
/*                                                                   */
/* This routine can be called by the i/o thread (`ra' == 0) or       */
/* by readahead threads (0 < `ra' <= cckdblk.ramax).                 */
/*                                                                   */
/*-------------------------------------------------------------------*/
int cckd_read_trk(DEVBLK *dev, int trk, int ra, BYTE *unitstat)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             fnd;                    /* Cache index for hit       */
int             lru;                    /* Oldest unused cache index */
int             len;                    /* Length of track image     */
int             maxlen;                 /* Length for buffer         */
int             curtrk = -1;            /* Current track (at entry)  */
U16             devnum;                 /* Device number             */
U32             oldtrk;                 /* Stolen track number       */
U32             flag;                   /* Cache flag                */
BYTE           *buf;                    /* Read buffer               */

    if (dev->cckd64)
        return cckd64_read_trk( dev, trk, ra, unitstat );

    cckd = dev->cckd_ext;

    CCKD_TRACE( "%d rdtrk     %d", ra, trk);

    maxlen = cckd->ckddasd ? dev->ckdtrksz
                           : CFBA_BLKGRP_SIZE + CKD_TRKHDR_SIZE;

    if (!ra) obtain_lock (&cckd->cckdiolock);

    cache_lock (CACHE_DEVBUF);

    /* Inactivate the old entry */
    if (!ra)
    {
        curtrk = dev->bufcur;
        if (dev->cache >= 0)
            cache_setflag(CACHE_DEVBUF, dev->cache, ~CCKD_CACHE_ACTIVE, 0);
        dev->bufcur = dev->cache = -1;
    }

cckd_read_trk_retry:

    /* scan the cache array for the track */
    fnd = cache_lookup (CACHE_DEVBUF, CCKD_CACHE_SETKEY(dev->devnum, trk), &lru);

    /* check for cache hit */
    if (fnd >= 0)
    {
        if (ra) /* readahead doesn't care about a cache hit */
        {   cache_unlock (CACHE_DEVBUF);
            return fnd;
        }

        /* Mark the new entry active */
        cache_setflag(CACHE_DEVBUF, fnd, ~0, CCKD_CACHE_ACTIVE | CCKD_CACHE_USED);
        cache_setage(CACHE_DEVBUF, fnd);

        /* If the entry is pending write then change it to `updated' */
        if (cache_getflag(CACHE_DEVBUF, fnd) & CCKD_CACHE_WRITE)
        {
            cache_setflag(CACHE_DEVBUF, fnd, ~CCKD_CACHE_WRITE, CCKD_CACHE_UPDATED);
            cckd->wrpending--;
            if (cckd->cckdwaiters && !cckd->wrpending)
                broadcast_condition (&cckd->cckdiocond);
        }
        buf = cache_getbuf(CACHE_DEVBUF, fnd, 0);

        cache_unlock (CACHE_DEVBUF);

        CCKD_TRACE( "%d rdtrk[%d] %d cache hit buf %p:%2.2x%2.2x%2.2x%2.2x%2.2x",
                    ra, fnd, trk, buf, buf[0], buf[1], buf[2], buf[3], buf[4]);

        cckdblk.stats_switches++;  cckd->switches++;
        cckdblk.stats_cachehits++; cckd->cachehits++;

        /* if read/write is in progress then wait for it to finish */
        while (cache_getflag(CACHE_DEVBUF, fnd) & CCKD_CACHE_IOBUSY)
        {
            cckdblk.stats_iowaits++;
            CCKD_TRACE( "%d rdtrk[%d] %d waiting for %s", ra, fnd, trk,
                        cache_getflag(CACHE_DEVBUF, fnd) & CCKD_CACHE_READING ?
                        "read" : "write");
            cache_setflag (CACHE_DEVBUF, fnd, ~0, CCKD_CACHE_IOWAIT);
            cckd->cckdwaiters++;
            wait_condition (&cckd->cckdiocond, &cckd->cckdiolock);
            cckd->cckdwaiters--;
            cache_setflag (CACHE_DEVBUF, fnd, ~CCKD_CACHE_IOWAIT, 0);
            CCKD_TRACE( "%d rdtrk[%d] %d io wait complete",
                        ra, fnd, trk);
        }

        release_lock (&cckd->cckdiolock);

        /* Asynchrously schedule readaheads */
        if (curtrk > 0 && trk > curtrk && trk <= curtrk + 2)
            cckd_readahead (dev, trk);

        return fnd;

    } /* cache hit */

    CCKD_TRACE( "%d rdtrk[%d] %d cache miss", ra, lru, trk);

    /* If no cache entry was stolen, then flush all outstanding writes.
       This requires us to release our locks.  cache_wait should be
       called with only the cache_lock held.  Fortunately, cache waits
       occur very rarely. */
    if (lru < 0) /* No available entry to be stolen */
    {
        CCKD_TRACE( "%d rdtrk[%d] %d no available cache entry",
                    ra, lru, trk);
        cache_unlock (CACHE_DEVBUF);
        if (!ra) release_lock (&cckd->cckdiolock);
        cckd_flush_cache_all();
        cache_lock (CACHE_DEVBUF);
        cckdblk.stats_cachewaits++;
        cache_wait (CACHE_DEVBUF);
        if (!ra)
        {
            cache_unlock (CACHE_DEVBUF);
            obtain_lock (&cckd->cckdiolock);
            cache_lock (CACHE_DEVBUF);
        }
        goto cckd_read_trk_retry;
    }

    CCKD_CACHE_GETKEY(lru, devnum, oldtrk);
    if (devnum != 0)
    {
        CCKD_TRACE( "%d rdtrk[%d] %d dropping %4.4X:%d from cache",
                    ra, lru, trk, devnum, oldtrk);
        if (!(cache_getflag(CACHE_DEVBUF, lru) & CCKD_CACHE_USED))
        {
            cckdblk.stats_readaheadmisses++;  cckd->misses++;
        }
    }

    /* Initialize the entry */
    cache_setkey(CACHE_DEVBUF, lru, CCKD_CACHE_SETKEY(dev->devnum, trk));
    cache_setflag(CACHE_DEVBUF, lru, 0, CCKD_CACHE_READING);
    cache_setage(CACHE_DEVBUF, lru);
    cache_setval(CACHE_DEVBUF, lru, 0);
    if (!ra)
    {
        cckdblk.stats_switches++; cckd->switches++;
        cckdblk.stats_cachemisses++;
        cache_setflag(CACHE_DEVBUF, lru, ~0, CCKD_CACHE_ACTIVE|CCKD_CACHE_USED);
    }
    cache_setflag(CACHE_DEVBUF, lru, ~CACHE_TYPE,
                  cckd->ckddasd ? DEVBUF_TYPE_CCKD : DEVBUF_TYPE_CFBA);
    buf = cache_getbuf(CACHE_DEVBUF, lru, maxlen);

    CCKD_TRACE( "%d rdtrk[%d] %d buf %p len %d",
                ra, lru, trk, buf, cache_getlen(CACHE_DEVBUF, lru));

    cache_unlock (CACHE_DEVBUF);

    if (!ra) release_lock (&cckd->cckdiolock);

    /* Asynchronously schedule readaheads */
    if (!ra && curtrk > 0 && trk > curtrk && trk <= curtrk + 2)
        cckd_readahead (dev, trk);

    /* Clear the buffer if batch mode */
    if (dev->batch) memset(buf, 0, maxlen);

    /* Read the track image */
    obtain_lock( &cckd->filelock );
    {
        len = cckd_read_trkimg (dev, buf, trk, unitstat);
    }
    release_lock( &cckd->filelock );

    cache_setval (CACHE_DEVBUF, lru, len);

    obtain_lock (&cckd->cckdiolock);

    /* Turn off the READING bit */
    cache_lock (CACHE_DEVBUF);
    flag = cache_setflag(CACHE_DEVBUF, lru, ~CCKD_CACHE_READING, 0);
    cache_unlock (CACHE_DEVBUF);

    /* Wakeup other thread waiting for this read */
    if (cckd->cckdwaiters && (flag & CCKD_CACHE_IOWAIT))
    {   CCKD_TRACE( "%d rdtrk[%d] %d signalling read complete",
                    ra, lru, trk);
        broadcast_condition (&cckd->cckdiocond);
    }

    release_lock (&cckd->cckdiolock);

    if (ra)
    {
        cckdblk.stats_readaheads++; cckd->readaheads++;
    }

    CCKD_TRACE( "%d rdtrk[%d] %d complete buf %p:%2.2x%2.2x%2.2x%2.2x%2.2x",
                ra, lru, trk, buf, buf[0], buf[1], buf[2], buf[3], buf[4]);

    if (cache_busy_percent(CACHE_DEVBUF) > 80) cckd_flush_cache_all();

    return lru;

} /* end function cckd_read_trk */

/*-------------------------------------------------------------------*/
/* Schedule asynchronous readaheads                                  */
/*-------------------------------------------------------------------*/
void cckd_readahead (DEVBLK *dev, int trk)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             i, r;                   /* Indexes                   */
TID             tid;                    /* Readahead thread id       */
int             rc;

    cckd = dev->cckd_ext;

    if (cckdblk.ramax < 1 || cckdblk.readaheads < 1)
        return;

    obtain_lock (&cckdblk.ralock);

    /* Scan the cache to see if the tracks are already there */
    memset( cckd->ralkup, 0, sizeof(cckd->ralkup) );
    cckd->ratrk = trk;
    cache_lock(CACHE_DEVBUF);
    cache_scan(CACHE_DEVBUF, cckd_readahead_scan, dev);
    cache_unlock(CACHE_DEVBUF);

    /* Scan the queue to see if the tracks are already there */
    for (r = cckdblk.ra1st; r >= 0; r = cckdblk.ra[r].ra_idxnxt)
        if (cckdblk.ra[r].ra_dev == dev)
        {
            i = cckdblk.ra[r].ra_trk - trk;
            if (i > 0 && i <= cckdblk.readaheads)
                cckd->ralkup[i-1] = 1;
        }

    /* Queue the tracks to the readahead queue */
    for (i = 1; i <= cckdblk.readaheads && cckdblk.rafree >= 0; i++)
    {
        if (cckd->ralkup[i-1]) continue;
        if (trk + i >= dev->ckdtrks) break;
        r = cckdblk.rafree;
        cckdblk.rafree = cckdblk.ra[r].ra_idxnxt;
        if (cckdblk.ralast < 0)
        {
            cckdblk.ra1st = cckdblk.ralast = r;
            cckdblk.ra[r].ra_idxprv = cckdblk.ra[r].ra_idxnxt = -1;
        }
        else
        {
            cckdblk.ra[cckdblk.ralast].ra_idxnxt = r;
            cckdblk.ra[r].ra_idxprv = cckdblk.ralast;
            cckdblk.ra[r].ra_idxnxt = -1;
            cckdblk.ralast = r;
        }
        cckdblk.ra[r].ra_trk = trk + i;
        cckdblk.ra[r].ra_dev = dev;
    }

    /* Schedule the readahead if any are pending */
    if (cckdblk.ra1st >= 0)
    {
        if (cckdblk.rawaiting)
            signal_condition(&cckdblk.racond);
        else if (cckdblk.ras < cckdblk.ramax)
        {
            /* Schedule a new read-ahead thread  */
            if (!cckdblk.batch || cckdblk.batchml > 1)
                // "Starting thread %s, active=%d, started=%d, max=%d"
                WRMSG(HHC00107, "I", CCKD_RA_THREAD_NAME "()",
                    cckdblk.raa, cckdblk.ras, cckdblk.ramax);

            ++cckdblk.ras;

            /* Release lock across thread create to prevent interlock  */
            release_lock(&cckdblk.ralock);
            {
                rc = create_thread( &tid, JOINABLE, cckd_ra, NULL, CCKD_RA_THREAD_NAME );
            }
            obtain_lock(&cckdblk.ralock);

            if (rc)
            {
                // "Error in function create_thread() for %s %d of %d: %s"
                WRMSG(HHC00106, "E", CCKD_RA_THREAD_NAME "()",
                    cckdblk.ras-1, cckdblk.ramax, strerror(rc));

                --cckdblk.ras;
            }
        }
    }

    release_lock (&cckdblk.ralock);

} /* end function cckd_readahead */

int cckd_readahead_scan (int *answer, int ix, int i, void *data)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
U16             devnum;                 /* Cached device number      */
U32             trk;                    /* Cached track              */
DEVBLK         *dev = data;             /* -> device block           */
int             k;                      /* Index                     */

    UNREFERENCED(answer);
    UNREFERENCED(ix);
    cckd = dev->cckd_ext;
    CCKD_CACHE_GETKEY(i, devnum, trk);
    if (devnum == dev->devnum)
    {
        k = (int)trk - cckd->ratrk;
        if (k > 0 && k <= cckdblk.readaheads)
            cckd->ralkup[k-1] = 1;
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* Asynchronous readahead thread                                     */
/*-------------------------------------------------------------------*/
void* cckd_ra (void* arg)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
DEVBLK         *dev;                    /* Readahead devblk          */
int             trk;                    /* Readahead track           */
int             ra;                     /* Readahead index           */
int             r;                      /* Readahead queue index     */
TID             tid;                    /* Readahead thread id       */
char            threadname[40];
int             rc;
int             ras;

    UNREFERENCED(arg);

    obtain_lock(&cckdblk.ralock);

    ra = ++cckdblk.raa;  /* increment nr ra threads dispatched */
    MSGBUF( threadname, CCKD_RA_THREAD_NAME " thread %d", ra );

    /* Return if too many already started */
    if (ra > cckdblk.ramax)
    {
        --cckdblk.ras;  /* decrement count started */
        --cckdblk.raa;  /* decrement count running */

        if (!cckdblk.ramax)
        {
            if (!cckdblk.batch || cckdblk.batchml > 1)
                // "Thread id "TIDPAT", prio %d, name '%s' ended"
                LOG_THREAD_END( threadname );
        }
        else
            if (!cckdblk.batch || cckdblk.batchml > 0)
                // "Ending thread "TIDPAT" %s, pri=%d, started=%d, max=%d exceeded"
                WRMSG( HHC00108, "W", TID_CAST( thread_id()), threadname,
                    get_thread_priority(), ra, cckdblk.ramax );

        release_lock( &cckdblk.ralock );
        signal_condition( &cckdblk.termcond );
        return NULL;
    }

    if (!cckdblk.batch || cckdblk.batchml > 1)
        // "Thread id "TIDPAT", prio %d, name '%s' started"
        LOG_THREAD_BEGIN( threadname );

    while (ra <= cckdblk.ramax)   /* continue until ramax=0 (shutdown) or max reduced by command line */
    {
        if (cckdblk.ra1st < 0)
        {
            cckdblk.rawaiting++;
            wait_condition (&cckdblk.racond, &cckdblk.ralock);
            cckdblk.rawaiting--;
        }

        /* Possibly shutting down if no writes pending */
        if (cckdblk.ra1st < 0)
            continue;

        r = cckdblk.ra1st;
        trk = cckdblk.ra[r].ra_trk;
        dev = cckdblk.ra[r].ra_dev;

        cckd = dev->cckd_ext;

        /* Requeue the 1st entry to the readahead free queue */
        cckdblk.ra1st = cckdblk.ra[r].ra_idxnxt;
        if (cckdblk.ra[r].ra_idxnxt > -1)
            cckdblk.ra[cckdblk.ra[r].ra_idxnxt].ra_idxprv = -1;
        else cckdblk.ralast = -1;
        cckdblk.ra[r].ra_idxnxt = cckdblk.rafree;
        cckdblk.rafree = r;

        /* Schedule the other readaheads if any are still pending */
        if (cckdblk.ra1st)
        {
            if (cckdblk.rawaiting)
                signal_condition (&cckdblk.racond);
            else if (cckdblk.ras < cckdblk.ramax)
            {
                /* Schedule a new read-ahead thread  */
                if (!cckdblk.batch || cckdblk.batchml > 1)
                    // "Starting thread %s, active=%d, started=%d, max=%d"
                    WRMSG(HHC00107, "I", CCKD_RA_THREAD_NAME "() from " CCKD_RA_THREAD_NAME "()",
                        cckdblk.raa, cckdblk.ras, cckdblk.ramax);

                ++cckdblk.ras;

                /* Release lock across thread create to prevent interlock  */
                release_lock(&cckdblk.ralock);
                {
                    rc = create_thread( &tid, JOINABLE, cckd_ra, NULL, CCKD_RA_THREAD_NAME );
                }
                obtain_lock(&cckdblk.ralock);

                if (rc)
                {
                    // "Error in function create_thread() for %s %d of %d: %s"
                    WRMSG(HHC00106, "E", CCKD_RA_THREAD_NAME "() from " CCKD_RA_THREAD_NAME "()",
                        cckdblk.ras-1, cckdblk.ramax, strerror(rc));
                    --cckdblk.ras;
                }
            }
        }

        if (!cckd || cckd->stopping || cckd->merging) continue;

        cckd->ras++;

        release_lock (&cckdblk.ralock);
        {
            /* Read the readahead track */
            cckd_read_trk (dev, trk, ra, NULL);
        }
        obtain_lock (&cckdblk.ralock);

        cckd->ras--;
    }

    if (!cckdblk.batch || cckdblk.batchml > 1)
        // "Thread id "TIDPAT", prio %d, name '%s' ended"
        LOG_THREAD_END( threadname );

    --cckdblk.ras;
    --cckdblk.raa;

    ras = cckdblk.ras;

    release_lock( &cckdblk.ralock );

    if (!ras)
        signal_condition( &cckdblk.termcond );

    return NULL;
} /* end thread cckd_ra_thread */

/*-------------------------------------------------------------------*/
/* Flush updated cache entries for a device                          */
/*                                                                   */
/* Caller holds the cckd->cckdiolock                                 */
/* cckdblk.wrlock then cache_lock is obtained and released           */
/*-------------------------------------------------------------------*/
void cckd_flush_cache(DEVBLK *dev)
{
int             rc;                     /* Return code               */
TID             tid;                    /* Writer thread id          */

    if (dev->cckd64)
    {
        cckd64_flush_cache( dev );
        return;
    }

    /* Scan cache for updated cache entries */
    obtain_lock (&cckdblk.wrlock);
    cache_lock (CACHE_DEVBUF);
    cache_scan (CACHE_DEVBUF, cckd_flush_cache_scan, dev);
    cache_unlock (CACHE_DEVBUF);

    /* Schedule the writer if any writes are pending */
    if (cckdblk.wrpending)
    {
        if (cckdblk.wrwaiting)
            signal_condition (&cckdblk.wrcond);
        else if (cckdblk.wrs < cckdblk.wrmax)
        {
            /* Schedule a new writer thread  */
            if (!cckdblk.batch || cckdblk.batchml > 1)
                // "Starting thread %s, active=%d, started=%d, max=%d"
                WRMSG(HHC00107, "I", CCKD_WR_THREAD_NAME "()",
                    cckdblk.wra, cckdblk.wrs, cckdblk.wrmax);

            ++cckdblk.wrs;

            /* Release lock across thread create to prevent interlock  */
            release_lock(&cckdblk.wrlock);
            {
                rc = create_thread( &tid, JOINABLE, cckd_writer, NULL, CCKD_WR_THREAD_NAME );
            }
            obtain_lock(&cckdblk.wrlock);

            if (rc)
            {
                // "Error in function create_thread() for %s %d of %d: %s"
                WRMSG(HHC00106, "E", CCKD_WR_THREAD_NAME "()",
                    cckdblk.wrs-1, cckdblk.wrmax, strerror(rc));

                --cckdblk.wrs;
            }
        }
    }

    release_lock (&cckdblk.wrlock);
}

int cckd_flush_cache_scan (int *answer, int ix, int i, void *data)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
U16             devnum;                 /* Cached device number      */
U32             trk;                    /* Cached track              */
DEVBLK         *dev = data;             /* -> device block           */

    UNREFERENCED(answer);
    cckd = dev->cckd_ext;
    CCKD_CACHE_GETKEY(i, devnum, trk);
    if ((cache_getflag(ix,i) & CACHE_BUSY) == CCKD_CACHE_UPDATED
     && dev->devnum == devnum)
    {
        cache_setflag (ix, i, ~CCKD_CACHE_UPDATED, CCKD_CACHE_WRITE);
        ++cckd->wrpending;
        ++cckdblk.wrpending;
        CCKD_TRACE( "flush file[%d] cache[%d] %4.4X trk %d",
                    cckd->sfn, i, devnum, trk);
    }
    return 0;
}

void cckd_flush_cache_all()
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
DEVBLK         *dev = NULL;             /* -> device block           */

    cckd_lock_devchain(0);
    for (dev = cckdblk.dev1st; dev; dev = cckd->devnext)
    {
        cckd = dev->cckd_ext;
        obtain_lock (&cckd->cckdiolock);
        if (!cckd->merging && !cckd->stopping)
            cckd_flush_cache(dev);
        release_lock (&cckd->cckdiolock);
    }
    cckd_unlock_devchain();
}

/*-------------------------------------------------------------------*/
/* Purge cache entries for a device                                  */
/*                                                                   */
/* Caller holds the cckdiolock                                       */
/* cache_lock is obtained and released                               */
/*-------------------------------------------------------------------*/
void cckd_purge_cache(DEVBLK *dev)
{
    if (dev->cckd64)
    {
        cckd64_purge_cache( dev );
        return;
    }

    /* Scan cache and purge entries */
    cache_lock (CACHE_DEVBUF);
    cache_scan (CACHE_DEVBUF, cckd_purge_cache_scan, dev);
    cache_unlock (CACHE_DEVBUF);
}
int cckd_purge_cache_scan (int *answer, int ix, int i, void *data)
{
U16             devnum;                 /* Cached device number      */
U32             trk;                    /* Cached track              */
DEVBLK         *dev = data;             /* -> device block           */

    UNREFERENCED(answer);
    CCKD_CACHE_GETKEY(i, devnum, trk);
    if (dev->devnum == devnum)
    {
        cache_release (ix, i, 0);
        CCKD_TRACE( "purge cache[%d] %4.4X trk %d purged",
                    i, devnum, trk);
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* Writer thread                                                     */
/*-------------------------------------------------------------------*/
void* cckd_writer( void* arg )
{
int             writer;                 /* Writer identifier         */
int             o;                      /* Cache entry found         */
TID             tid;                    /* Writer thead id           */
char            threadname[40];
int             rc;
int             wrs;

    UNREFERENCED( arg );

    /* Set the writer thread's priority just BELOW the CPU threads'
       in order to minimize any potential impact from compression.
    */
    if (!cckdblk.batch)
    {
        cckdblk.wrprio = sysblk.cpuprio - 1;
        SET_THREAD_PRIORITY( cckdblk.wrprio, sysblk.qos_user_initiated );
    }

    obtain_lock( &cckdblk.wrlock );

    writer = ++cckdblk.wra;
    MSGBUF( threadname, CCKD_WR_THREAD_NAME " thread %d", writer );

    /* Return with message if too many already started */
    if (writer > cckdblk.wrmax)
    {
        --cckdblk.wrs;  /* decrease threads started */
        --cckdblk.wra;  /* decrease threads active  */

        if (cckdblk.termwr) /* choose thread termination message  */
        {
            if (!cckdblk.batch || cckdblk.batchml > 1)
              // "Thread id "TIDPAT", prio %d, name '%s' ended"
              LOG_THREAD_END( threadname  );
        }
        else
            if (!cckdblk.batch || cckdblk.batchml > 0)
                // "Ending thread "TIDPAT" %s, pri=%d, started=%d, max=%d exceeded"
                WRMSG( HHC00108, "W", TID_CAST( thread_id()), threadname,
                    get_thread_priority(), writer, cckdblk.wrmax );

        release_lock( &cckdblk.wrlock );
        signal_condition( &cckdblk.termcond );/* shutting down */
        return NULL;
    }

    if (!cckdblk.batch || cckdblk.batchml > 1)
        // "Thread id "TIDPAT", prio %d, name '%s' started"
        LOG_THREAD_BEGIN( threadname  );

    while (!cckdblk.termwr && (writer <= cckdblk.wrmax || cckdblk.wrpending))
    {
        /* Wait (but not forever!) for work */
        if (cckdblk.wrpending == 0)
        {
            cckdblk.wrwaiting++;
            {
                timed_wait_condition_relative_usecs( &cckdblk.wrcond, &cckdblk.wrlock, 1000000, NULL );
            }
            cckdblk.wrwaiting--;
        }

        /* Scan the cache for the oldest pending write */
        cache_lock( CACHE_DEVBUF );
        {
            o = cache_scan( CACHE_DEVBUF, cckd_writer_scan, NULL );

            /* Possibly shutting down if no writes pending */
            if (o < 0)
            {
                cache_unlock( CACHE_DEVBUF );
                cckdblk.wrpending = 0;
                continue;
            }

            /* We will process this cache entry. Clear flags to prevent
               any other writer threads from trying to process it too. */
            cache_setflag( CACHE_DEVBUF, o, ~CCKD_CACHE_WRITE, CCKD_CACHE_WRITING );
        }
        cache_unlock (CACHE_DEVBUF);

        /* Schedule the other writers if any writes are still pending */

        cckdblk.wrpending--;

        if (cckdblk.wrpending)
        {
            if (cckdblk.wrwaiting)
            {
                signal_condition( &cckdblk.wrcond );
            }
            else if (cckdblk.wrs < cckdblk.wrmax)
            {
                /* Schedule a new writer thread */

                if (!cckdblk.batch || cckdblk.batchml > 1)
                    // "Starting thread %s, active=%d, started=%d, max=%d"
                    WRMSG( HHC00107, "I", CCKD_WR_THREAD_NAME "() from " CCKD_WR_THREAD_NAME "()",
                        cckdblk.wra, cckdblk.wrs, cckdblk.wrmax );

                ++cckdblk.wrs;

                /* Release lock across thread create to prevent interlock  */
                release_lock( &cckdblk.wrlock );
                {
                    rc = create_thread( &tid, JOINABLE, cckd_writer, NULL, CCKD_WR_THREAD_NAME );
                }
                obtain_lock( &cckdblk.wrlock );

                if (rc)
                {
                    // "Error in function create_thread() for %s %d of %d: %s"
                    WRMSG( HHC00106, "E", CCKD_WR_THREAD_NAME "() from " CCKD_WR_THREAD_NAME "()",
                        cckdblk.wrs-1, cckdblk.wrmax, strerror( rc ));

                    --cckdblk.wrs;
                }
            }
        }

        /* Write the updated track image */
        release_lock( &cckdblk.wrlock );
        {
            cckd_writer_write( writer, o );
        }
        obtain_lock( &cckdblk.wrlock );
    }
    /* end while (writer <= cckdblk.wrmax || cckdblk.wrpending) */

    if (!cckdblk.batch || cckdblk.batchml > 1)
        // "Thread id "TIDPAT", prio %d, name '%s' ended"
        LOG_THREAD_END( threadname  );

    cckdblk.wrs--;
    cckdblk.wra--;

    wrs = cckdblk.wrs;

    release_lock( &cckdblk.wrlock );

    if (!wrs)
        signal_condition( &cckdblk.termcond );

    return NULL;
} /* end thread cckd_writer */

int cckd_writer_scan( int* o, int ix, int i, void* data )
{
    UNREFERENCED( data );

    if (1
        && (cache_getflag( ix, i ) & DEVBUF_TYPE_COMP)
        && (cache_getflag( ix, i ) & CCKD_CACHE_WRITE)
        && (0
            || *o == -1
            || cache_getage( ix, i ) < cache_getage( ix, *o )
           )
    )
        *o = i;

    return 0;
}

/*-------------------------------------------------------------------*/
/* cckd writer thread helper:   write the cached track image         */
/*-------------------------------------------------------------------*/
void cckd_writer_write( int writer, int o )
{
TID             tid;                    /* Writer thead id           */
CCKD_EXT*       cckd;                   /* -> cckd extension         */
DEVBLK*         dev;                    /* Device block              */
U16             devnum;                 /* Device number             */
int             rc;                     /* (work) return code        */
int             trk;                    /* Track number              */
BYTE*           buf;                    /* Buffer                    */
BYTE*           bufp;                   /* Buffer to be written      */
int             len, bufl;              /* Buffer lengths            */
int             comp;                   /* Compression algorithm     */
int             parm;                   /* Compression parameter     */
U32             flag;                   /* Cache flag                */
BYTE            buf2[ 64*1024 ];        /* 64K Compress buffer       */

    /* Prepare to compress */
    CCKD_CACHE_GETKEY( o, devnum, trk );
    dev = cckd_find_device_by_devnum( devnum );

    if (dev->cckd64)
    {
        cckd64_writer_write( writer, o );
        return;
    }

    cckd = dev->cckd_ext;
    buf  = cache_getbuf( CACHE_DEVBUF, o, 0 );
    len  = cckd_trklen( dev, buf );

    comp = len < CCKD_COMPRESS_MIN ? CCKD_COMPRESS_NONE :
         cckdblk.comp == 0xff ? cckd->cdevhdr[ cckd->sfn ].cmp_algo
                              : cckdblk.comp;

    parm = cckdblk.compparm < 0 ? cckd->cdevhdr[ cckd->sfn ].cmp_parm
                                : cckdblk.compparm;

    CCKD_TRACE( "%d wrtrk[%d] %d len %d buf %p:%2.2x%2.2x%2.2x%2.2x%2.2x",
                writer, o, trk, len, buf, buf[0], buf[1],buf[2],buf[3],buf[4] );

    /* Compress the image if not null */
    if ((len = cckd_check_null_trk( dev, buf, trk, len )) > CKD_NULLTRK_FMTMAX)
    {
        /* Stress adjustments */
        if (1
            && !cckdblk.nostress
            && (0
                || cache_waiters ( CACHE_DEVBUF )
                || cache_busy    ( CACHE_DEVBUF ) > 90
               )
        )
        {
            cckdblk.stats_stresswrites++;

            comp = len < CCKD_STRESS_MINLEN ? CCKD_COMPRESS_NONE
                                            : CCKD_STRESS_COMP;

            parm = cache_busy(CACHE_DEVBUF) <= 95 ? CCKD_STRESS_PARM1
                                                  : CCKD_STRESS_PARM2;
        }

        /* Compress the track image */
        CCKD_TRACE( "%d wrtrk[%d] %d comp %s parm %d",
                    writer, o, trk, compname[ comp ], parm );

        bufp = (BYTE*) &buf2;
        bufl = cckd_compress( dev, &bufp, buf, len, comp, parm );

        CCKD_TRACE( "%d wrtrk[%d] %d compressed length %d",
                    writer, o, trk, bufl );
    }
    else
    {
        bufp = buf;
        bufl = len;
    }

    obtain_lock( &cckd->filelock );
    {
        /* Turn on read-write header bits if not already on */
        if (!(cckd->cdevhdr[ cckd->sfn ].cdh_opts & CCKD_OPT_OPENED))
        {
            cckd->cdevhdr[ cckd->sfn ].cdh_opts |= (CCKD_OPT_OPENED | CCKD_OPT_OPENRW);
            cckd_write_chdr( dev );
        }

        /* Write the track image */
        cckd_write_trkimg( dev, bufp, bufl, trk, CCKD_SIZE_ANY );
    }
    release_lock( &cckd->filelock );

    /* Schedule the garbage collector */
    obtain_lock( &cckdblk.gclock );/* ensure read integrity for gc count */
    {
        if (cckdblk.gcint > 0 && cckdblk.gcs < cckdblk.gcmax)
        {
            /* Schedule a new garbage collector thread */

            if (!cckdblk.batch || cckdblk.batchml > 1)
                // "Starting thread %s, active=%d, started=%d, max=%d"
                WRMSG( HHC00107, "I", CCKD_GC_THREAD_NAME "()",
                    cckdblk.gca, cckdblk.gcs, cckdblk.gcmax );

            ++cckdblk.gcs;

            /* Release lock across thread create to prevent interlock  */
            release_lock( &cckdblk.gclock );
            {
                rc = create_thread( &tid, JOINABLE, cckd_gcol, NULL, CCKD_GC_THREAD_NAME );
            }
            obtain_lock( &cckdblk.gclock );

            if (rc)
            {
                // "Error in function create_thread() for %s %d of %d: %s"
                WRMSG( HHC00106, "E", CCKD_GC_THREAD_NAME "()",
                    cckdblk.gcs-1, cckdblk.gcmax, strerror( rc ));

                --cckdblk.gcs;
            }
        }
    }
    release_lock( &cckdblk.gclock );

    obtain_lock( &cckd->cckdiolock );
    {
        cache_lock( CACHE_DEVBUF );
        {
            flag = cache_setflag( CACHE_DEVBUF, o, ~CCKD_CACHE_WRITING, 0 );
        }
        cache_unlock( CACHE_DEVBUF );

        cckd->wrpending--;

        if (1
            && cckd->cckdwaiters
            && (0
                || (flag & CCKD_CACHE_IOWAIT)
                || !cckd->wrpending
               )
        )
        {
            CCKD_TRACE( "writer[%d] cache[%2.2d] %d signalling write complete",
                       writer, o, trk );

            broadcast_condition( &cckd->cckdiocond );
        }
    }
    release_lock( &cckd->cckdiolock );

    CCKD_TRACE( "%d wrtrk[%2.2d] %d complete flags:%8.8x",
                writer, o, trk, cache_getflag( CACHE_DEVBUF, o ));

} /* end function cckd_writer_write */

#if defined( DEBUG_FREESPACE )
/*-------------------------------------------------------------------*/
/* Debug routine for checking the free space array                   */
/*-------------------------------------------------------------------*/
void cckd_chk_space(DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx;                    /* Shadow file index         */
int             err = 0, n = 0, i, p;
size_t          largest=0;
size_t          total=0;
off_t           fpos;

    if (dev->cckd64)
    {
        cckd64_chk_space( dev );
        return;
    }

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    p = -1;
    fpos = cckd->cdevhdr[sfx].free_off;
    for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
    {
        n++; total += cckd->ifb[i].ifb_len;
        if (n > cckd->free_count) break;
        if (cckd->ifb[i].ifb_idxprv != p)
            err = 1;
        if (cckd->ifb[i].ifb_idxnxt >= 0)
        {
            if (fpos + cckd->ifb[i].ifb_len > cckd->ifb[i].ifb_offnxt)
                err = 1;
        }
        else
        {
            if (fpos + cckd->ifb[i].ifb_len > cckd->cdevhdr[sfx].cdh_size)
                err = 1;
        }
        if (cckd->ifb[i].ifb_pending == 0 && cckd->ifb[i].ifb_len > largest)
            largest = cckd->ifb[i].ifb_len;
        fpos = cckd->ifb[i].ifb_offnxt;
        p = i;
    }

    if (err
     || (cckd->cdevhdr[sfx].free_off != 0 && cckd->cdevhdr[sfx].free_num == 0)
     || (cckd->cdevhdr[sfx].free_off == 0 && cckd->cdevhdr[sfx].free_num != 0)
     || (n != cckd->cdevhdr[sfx].free_num)
     || (total != cckd->cdevhdr[sfx].free_total - cckd->cdevhdr[sfx].free_imbed)
     || (cckd->free_idxlast != p)
     || (largest != cckd->cdevhdr[sfx].free_largest)
    )
    {
        CCKD_TRACE( "cdevhdr[%d] size   %10d used   %10d free   0x%8.8x",
                    sfx,cckd->cdevhdr[sfx].cdh_size,cckd->cdevhdr[sfx].cdh_used,
                    cckd->cdevhdr[sfx].free_off);
        CCKD_TRACE( "           nbr   %10d total  %10d imbed  %10d largest %10d",
                    cckd->cdevhdr[sfx].free_num,
                    cckd->cdevhdr[sfx].free_total,cckd->cdevhdr[sfx].free_imbed,
                    cckd->cdevhdr[sfx].free_largest);
        CCKD_TRACE( "free %p nbr %d 1st %d last %d avail %d",
                    cckd->ifb,cckd->free_count,cckd->free_idx1st,
                    cckd->free_idxlast,cckd->free_idxavail);
        CCKD_TRACE( "found nbr %d total %ld largest %ld",n,(long)total,(long)largest);
        fpos = cckd->cdevhdr[sfx].free_off;
        for (n = 0, i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
        {
            if (++n > cckd->free_count) break;
            CCKD_TRACE( "%4d: [%4d] prev[%4d] next[%4d] pos %16.16"PRIx64" len %8d %16.16"PRIx64" pend %d",
                        n, i, cckd->ifb[i].ifb_idxprv, cckd->ifb[i].ifb_idxnxt,
                        fpos, cckd->ifb[i].ifb_len,
                        fpos + cckd->ifb[i].ifb_len, cckd->ifb[i].ifb_pending);
            fpos = cckd->ifb[i].ifb_offnxt;
        }
        cckd_print_itrace();
    }
} /* end function cckd_chk_space */
#endif // defined( DEBUG_FREESPACE )

/*-------------------------------------------------------------------*/
/* Get file space                                                    */
/*-------------------------------------------------------------------*/
off_t cckd_get_space(DEVBLK *dev, int *size, int flags)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             i,p,n;                  /* Free space indexes        */
int             len2;                   /* Other lengths             */
off_t           fpos;                   /* Free space offset         */
unsigned int    flen;                   /* Free space size           */
int             sfx;                    /* Shadow file index         */
int             len;                    /* Requested length          */

    if (dev->cckd64)
        return cckd64_get_space( dev, size, flags );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    len = *size;

    if (flags & CCKD_L2SPACE)
    {
        flags |= CCKD_SIZE_EXACT;
        len = *size = CCKD_L2TAB_SIZE;
    }

    CCKD_TRACE( "get_space len %d largest %d flags 0x%2.2x",
                len, cckd->cdevhdr[sfx].free_largest, flags);

    if (len <= CKD_NULLTRK_FMTMAX)
        return 0;

    if (!cckd->ifb)
        cckd_read_fsp (dev);

    len2 = len + CCKD_FREEBLK_SIZE;

    /* Get space at the end if no space is large enough */
    if (len2 > (int)cckd->cdevhdr[sfx].free_largest
     && len != (int)cckd->cdevhdr[sfx].free_largest)
    {

cckd_get_space_atend:

        fpos = (off_t)cckd->cdevhdr[sfx].cdh_size;
        if ((U64)fpos > (cckd->cckd_maxsize - len))
        {
            // "%1d:%04X CCKD file[%d] %s: get space error, size exceeds %"PRId64"M"
            WRMSG (HHC00304, "E", LCSS_DEVNUM, sfx, cckd_sf_name (dev, sfx),
                (S64) (cckd->cckd_maxsize >> 20) + 1);
            return -1;
        }
        cckd->cdevhdr[sfx].cdh_size += len;
        cckd->cdevhdr[sfx].cdh_used += len;

        CCKD_TRACE( "get_space atend 0x%16.16"PRIx64" len %d",fpos, len);

        return fpos;
    }

    /* Scan free space chain */
    fpos = (off_t)cckd->cdevhdr[sfx].free_off;
    for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
    {
        if (cckd->ifb[i].ifb_pending == 0
         && (len2 <= (int)cckd->ifb[i].ifb_len || len == (int)cckd->ifb[i].ifb_len)
         && ((flags & CCKD_L2SPACE) || (U64)fpos >= cckd->L2_bounds))
            break;
        fpos = (off_t)cckd->ifb[i].ifb_offnxt;
    }

    /* This can happen if largest comes before L2_bounds */
    if (i < 0) goto cckd_get_space_atend;

    flen = cckd->ifb[i].ifb_len;
    p = cckd->ifb[i].ifb_idxprv;
    n = cckd->ifb[i].ifb_idxnxt;

    /*
     * If `CCKD_SIZE_ANY' bit is set and the left over space is small
     * enough, then use the entire free space
     */
    if ((flags & CCKD_SIZE_ANY) && flen <= cckd->free_minsize)
        *size = (int)flen;

    /* Remove the new space from free space */
    if (*size < (int)flen)
    {
        cckd->ifb[i].ifb_len -= *size;
        if (p >= 0)
            cckd->ifb[p].ifb_offnxt += *size;
        else
            cckd->cdevhdr[sfx].free_off += *size;
    }
    else
    {
        cckd->cdevhdr[sfx].free_num--;

        /* Remove the free space entry from the chain */
        if (p >= 0)
        {
            cckd->ifb[p].ifb_offnxt = cckd->ifb[i].ifb_offnxt;
            cckd->ifb[p].ifb_idxnxt = n;
        }
        else
        {
            cckd->cdevhdr[sfx].free_off = cckd->ifb[i].ifb_offnxt;
            cckd->free_idx1st = n;
        }

        if (n >= 0)
            cckd->ifb[n].ifb_idxprv = p;
        else
            cckd->free_idxlast = p;

        /* Add entry to the available queue */
        cckd->ifb[i].ifb_idxnxt = cckd->free_idxavail;
        cckd->free_idxavail = i;
    }

    /* Find the largest free space if we got the largest */
    if (flen >= cckd->cdevhdr[sfx].free_largest)
    {
        int i;
        cckd->cdevhdr[sfx].free_largest = 0;
        for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
            if (cckd->ifb[i].ifb_len > cckd->cdevhdr[sfx].free_largest
             && cckd->ifb[i].ifb_pending == 0)
                cckd->cdevhdr[sfx].free_largest = cckd->ifb[i].ifb_len;
    }

    /* Update free space stats */
    cckd->cdevhdr[sfx].cdh_used += len;
    cckd->cdevhdr[sfx].free_total -= len;

    cckd->cdevhdr[sfx].free_imbed += *size - len;

    CCKD_TRACE( "get_space found 0x%16.16"PRIx64" len %d size %d",
                fpos, len, *size);

    return fpos;

} /* end function cckd_get_space */

/*-------------------------------------------------------------------*/
/* Release file space                                                */
/*-------------------------------------------------------------------*/
void cckd_rel_space(DEVBLK *dev, off_t pos, int len, int size)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx;                    /* Shadow file index         */
off_t           ppos, npos;             /* Prev/next free offsets    */
int             i, p, n;                /* Free space indexes        */
int             pending;                /* Calculated pending value  */
int             fsize = size;           /* Free space size           */

    if (dev->cckd64)
    {
        cckd64_rel_space( dev, pos, len, size );
        return;
    }

    if (len <= CKD_NULLTRK_FMTMAX || pos == CCKD_NOSIZE || pos == CCKD_MAXSIZE)
        return;

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    CCKD_TRACE( "rel_space offset 0x%16.16"PRIx64" len %d size %d",
                pos, len, size);

    if (!cckd->ifb) cckd_read_fsp (dev);

    CCKD_CHK_SPACE(dev);

    /* Scan free space chain */
    ppos = -1;
    npos = cckd->cdevhdr[sfx].free_off;
    for (p = -1, n = cckd->free_idx1st; n >= 0; n = cckd->ifb[n].ifb_idxnxt)
    {
        if (pos < npos) break;
        ppos = npos;
        npos = cckd->ifb[n].ifb_offnxt;
        p = n;
    }

    /* Calculate the `pending' value */
    pending = cckdblk.freepend >= 0 ? cckdblk.freepend : 1 + (1 - cckdblk.fsync);

    /* If possible use previous adjacent free space otherwise get an available one */
    if (p >= 0 && ppos + cckd->ifb[p].ifb_len == pos && cckd->ifb[p].ifb_pending == pending)
    {
        cckd->ifb[p].ifb_len += size;
        fsize = cckd->ifb[p].ifb_len;
    }
    else
    {
        /* Increase the size of the free space array if necessary */
        if (cckd->free_idxavail < 0)
        {
            int new_free_count = cckd->free_count + CCKD_IFB_ENTS_INCR;
            if (!(cckd->ifb = cckd_realloc( dev, "ifb", cckd->ifb, new_free_count * CCKD_IFREEBLK_SIZE )))
                return;
            cckd->free_idxavail = cckd->free_count;
            cckd->free_count = new_free_count;
            for (i = cckd->free_idxavail; i < cckd->free_count; i++)
                cckd->ifb[i].ifb_idxnxt = i + 1;
            cckd->ifb[i-1].ifb_idxnxt = -1;
            cckd->free_minsize = CCKD_MIN_FREESIZE( cckd->free_count );
        }

        /* Get an available free space entry */
        i = cckd->free_idxavail;
        cckd->free_idxavail = cckd->ifb[i].ifb_idxnxt;
        cckd->cdevhdr[sfx].free_num++;

        /* Update the new entry */
        cckd->ifb[i].ifb_idxprv = p;
        cckd->ifb[i].ifb_idxnxt = n;
        cckd->ifb[i].ifb_len = size;
        cckd->ifb[i].ifb_pending = pending;

        /* Update the previous entry */
        if (p >= 0)
        {
            cckd->ifb[i].ifb_offnxt = cckd->ifb[p].ifb_offnxt;
            cckd->ifb[p].ifb_offnxt = (U32)pos;
            cckd->ifb[p].ifb_idxnxt = i;
        }
        else
        {
            cckd->ifb[i].ifb_offnxt = cckd->cdevhdr[sfx].free_off;
            cckd->cdevhdr[sfx].free_off = (U32)pos;
            cckd->free_idx1st = i;
        }

        /* Update the next entry */
        if (n >= 0)
            cckd->ifb[n].ifb_idxprv = i;
        else
            cckd->free_idxlast = i;
    }

    /* Update the free space statistics */
    cckd->cdevhdr[sfx].cdh_used -= len;
    cckd->cdevhdr[sfx].free_total += len;
    cckd->cdevhdr[sfx].free_imbed -= size - len;
    if (!pending && (U32)fsize > cckd->cdevhdr[sfx].free_largest)
        cckd->cdevhdr[sfx].free_largest = (U32)fsize;

    CCKD_CHK_SPACE(dev);

} /* end function cckd_rel_space */

/*-------------------------------------------------------------------*/
/* Flush pending free space                                          */
/*-------------------------------------------------------------------*/
void cckd_flush_space(DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             p,i,n;                  /* Free free space indexes   */
int             sfx;                    /* Shadow file index         */
U32             ppos, pos;              /* Free space offsets        */

    if (dev->cckd64)
    {
        cckd64_flush_space( dev );
        return;
    }

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    CCKD_TRACE( "flush_space nbr %d",cckd->cdevhdr[sfx].free_num);

    /* Make sure the free space chain is built */
    if (!cckd->ifb) cckd_read_fsp (dev);

    CCKD_CHK_SPACE(dev);

    if (cckd->cdevhdr[sfx].free_num == 0 || cckd->cdevhdr[sfx].free_off == 0)
    {
        cckd->cdevhdr[sfx].free_num = cckd->cdevhdr[sfx].free_off = 0;
        cckd->free_idx1st = cckd->free_idxlast = cckd->free_idxavail = -1;
    }

    pos = cckd->cdevhdr[sfx].free_off;
    ppos = p = -1;
    cckd->cdevhdr[sfx].free_num = cckd->cdevhdr[sfx].free_largest = 0;
    for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
    {
        /* Decrement the pending count */
        if (cckd->ifb[i].ifb_pending)
            --cckd->ifb[i].ifb_pending;

        /* Combine adjacent free spaces */
        while (pos + cckd->ifb[i].ifb_len == cckd->ifb[i].ifb_offnxt)
        {
            n = cckd->ifb[i].ifb_idxnxt;
            if (cckd->ifb[n].ifb_pending > cckd->ifb[i].ifb_pending + 1
             || cckd->ifb[n].ifb_pending < cckd->ifb[i].ifb_pending)
                break;
            cckd->ifb[i].ifb_offnxt  = cckd->ifb[n].ifb_offnxt;
            cckd->ifb[i].ifb_len += cckd->ifb[n].ifb_len;
            cckd->ifb[i].ifb_idxnxt = cckd->ifb[n].ifb_idxnxt;
            cckd->ifb[n].ifb_idxnxt = cckd->free_idxavail;
            cckd->free_idxavail = n;
            n = cckd->ifb[i].ifb_idxnxt;
            if (n >= 0)
                cckd->ifb[n].ifb_idxprv = i;

        }
        ppos = pos;
        pos = cckd->ifb[i].ifb_offnxt;
        cckd->cdevhdr[sfx].free_num++;
        if (cckd->ifb[i].ifb_len > cckd->cdevhdr[sfx].free_largest
         && !cckd->ifb[i].ifb_pending)
            cckd->cdevhdr[sfx].free_largest = cckd->ifb[i].ifb_len;
        p = i;
    }
    cckd->free_idxlast = p;

    CCKD_TRACE( "rel_flush_space nbr %d (after merge)",
                cckd->cdevhdr[sfx].free_num);

    /* If the last free space is at the end of the file then release it */
    if (p >= 0 && ppos + cckd->ifb[p].ifb_len == cckd->cdevhdr[sfx].cdh_size
     && !cckd->ifb[p].ifb_pending)
    {
        i = p;
        p = cckd->ifb[i].ifb_idxprv;

        CCKD_TRACE( "file[%d] rel_flush_space atend 0x%16.16"PRIx64" len %d",
                    sfx, ppos, cckd->ifb[i].ifb_len);

        /* Remove the entry from the chain */
        if (p >= 0)
        {
            cckd->ifb[p].ifb_offnxt = 0;
            cckd->ifb[p].ifb_idxnxt = -1;
        }
        else
        {
            cckd->cdevhdr[sfx].free_off = 0;
            cckd->free_idx1st = -1;
        }
        cckd->free_idxlast = p;

        /* Add the entry to the available chain */
        cckd->ifb[i].ifb_idxnxt = cckd->free_idxavail;
        cckd->free_idxavail = i;

        /* Update the device header */
        cckd->cdevhdr[sfx].cdh_size -= cckd->ifb[i].ifb_len;
        cckd->cdevhdr[sfx].free_total -= cckd->ifb[i].ifb_len;
        cckd->cdevhdr[sfx].free_num--;
        if (cckd->ifb[i].ifb_len >= cckd->cdevhdr[sfx].free_largest)
        {
            cckd->cdevhdr[sfx].free_largest = 0;
            for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
                if (cckd->ifb[i].ifb_len > cckd->cdevhdr[sfx].free_largest
                 && cckd->ifb[i].ifb_pending == 0)
                    cckd->cdevhdr[sfx].free_largest = cckd->ifb[i].ifb_len;
        }

        /* Truncate the file */
        cckd_ftruncate (dev, sfx, (off_t)cckd->cdevhdr[sfx].cdh_size);

    } /* Release space at end of the file */

    CCKD_CHK_SPACE(dev);

} /* end function cckd_flush_space */

/*-------------------------------------------------------------------*/
/* Read compressed dasd header                                       */
/*-------------------------------------------------------------------*/
int cckd_read_chdr (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx;                    /* File index                */

    if (dev->cckd64)
        return cckd64_read_chdr( dev );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    CCKD_TRACE( "file[%d] read_chdr", sfx);

    memset(&cckd->cdevhdr[sfx], 0, CCKD_DEVHDR_SIZE);

    /* Read the device header */
    if (cckd_read (dev, sfx, CKD_DEVHDR_SIZE, &cckd->cdevhdr[sfx], CCKD_DEVHDR_SIZE) < 0)
        return -1;

    /* Check endian format */
    cckd->swapend[sfx] = 0;
    if ((cckd->cdevhdr[sfx].cdh_opts & CCKD_OPT_BIGEND) != cckd_def_opt_bigend())
    {
        if (cckd->open[sfx] == CCKD_OPEN_RW)
        {
            if (cckd_swapend (dev) < 0)
                return -1;
            cckd_swapend_chdr (&cckd->cdevhdr[sfx]);
        }
        else
        {
            cckd->swapend[sfx] = 1;
            cckd_swapend_chdr (&cckd->cdevhdr[sfx]);
        }
    }

    /* Set default null format */
    if (cckd->cdevhdr[sfx].cdh_nullfmt > CKD_NULLTRK_FMTMAX)
        cckd->cdevhdr[sfx].cdh_nullfmt = 0;

    if (cckd->cdevhdr[sfx].cdh_nullfmt == 0 && dev->oslinux && dev->devtype == 0x3390)
        cckd->cdevhdr[sfx].cdh_nullfmt = CKD_NULLTRK_FMT2;

    if (cckd->cdevhdr[sfx].cdh_nullfmt == CKD_NULLTRK_FMT2)
        dev->oslinux = 1;

    return 0;

} /* end function cckd_read_chdr */

/*-------------------------------------------------------------------*/
/* Write compressed dasd header                                      */
/*-------------------------------------------------------------------*/
int cckd_write_chdr (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx;                    /* File index                */

    if (dev->cckd64)
        return cckd64_write_chdr( dev );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    CCKD_TRACE( "file[%d] write_chdr", sfx);

    /* Set version.release.modlvl */
    cckd->cdevhdr[sfx].cdh_vrm[0] = CCKD_VERSION;
    cckd->cdevhdr[sfx].cdh_vrm[1] = CCKD_RELEASE;
    cckd->cdevhdr[sfx].cdh_vrm[2] = CCKD_MODLVL;

    if (cckd_write (dev, sfx, CCKD_DEVHDR_POS, &cckd->cdevhdr[sfx], CCKD_DEVHDR_SIZE) < 0)
        return -1;

    return 0;

} /* end function cckd_write_chdr */

/*-------------------------------------------------------------------*/
/* Read the level 1 table                                            */
/*-------------------------------------------------------------------*/
int cckd_read_l1 (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx;                    /* File index                */
int             len;                    /* Length of level 1 table   */
int             i;                      /* Work integer              */

    if (dev->cckd64)
        return cckd64_read_l1( dev );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    CCKD_TRACE( "file[%d] read_l1 offset 0x%"PRIx64,
                sfx, (U64)CCKD_L1TAB_POS);

    /* Free the old level 1 table if it exists */
    cckd->L1tab[sfx] = cckd_free (dev, "l1", cckd->L1tab[sfx]);

    /* Allocate the level 1 table */
    len = cckd->cdevhdr[sfx].num_L1tab * CCKD_L1ENT_SIZE;
    if ((cckd->L1tab[sfx] = cckd_malloc (dev, "l1", len)) == NULL)
        return -1;
    if ( sfx )
        memset(cckd->L1tab[sfx], 0xFF, len);
    else
        memset(cckd->L1tab[sfx], 0, len);

    /* Read the level 1 table */
    if (cckd_read (dev, sfx, CCKD_L1TAB_POS, cckd->L1tab[sfx], len) < 0)
        return -1;

    /* Fix endianness */
    if (cckd->swapend[sfx])
        cckd_swapend_l1 (cckd->L1tab[sfx], cckd->cdevhdr[sfx].num_L1tab);

    /* Determine bounds */
    cckd->L2_bounds = CCKD_L1TAB_POS + len;
    for (i = 0; i < cckd->cdevhdr[sfx].num_L1tab; i++)
        if (cckd->L1tab[sfx][i] != CCKD_NOSIZE && cckd->L1tab[sfx][i] != CCKD_MAXSIZE)
            cckd->L2_bounds += CCKD_L2TAB_SIZE;

    /* Check if all l2 tables are within bounds */
    cckd->L2ok = 1;
    for (i = 0; i < cckd->cdevhdr[sfx].num_L1tab && cckd->L2ok; i++)
        if (cckd->L1tab[sfx][i] != CCKD_NOSIZE && cckd->L1tab[sfx][i] != CCKD_MAXSIZE)
            if (cckd->L1tab[sfx][i] > cckd->L2_bounds - CCKD_L2TAB_SIZE)
                cckd->L2ok = 0;

    return 0;

} /* end function cckd_read_l1 */

/*-------------------------------------------------------------------*/
/* Write the level 1 table                                           */
/*-------------------------------------------------------------------*/
int cckd_write_l1 (DEVBLK *dev)
{
CCKD_EXT        *cckd;                  /* -> cckd extension         */
int             sfx;                    /* File index                */
int             len;                    /* Length of level 1 table   */

    if (dev->cckd64)
        return cckd64_write_l1( dev );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;
    len = cckd->cdevhdr[sfx].num_L1tab * CCKD_L1ENT_SIZE;

    CCKD_TRACE( "file[%d] write_l1 0x%"PRIx64" len %d",
                sfx, (U64)CCKD_L1TAB_POS, len);

    if (cckd_write (dev, sfx, CCKD_L1TAB_POS, cckd->L1tab[sfx], len) < 0)
        return -1;

    return 0;

} /* end function cckd_write_l1 */

/*-------------------------------------------------------------------*/
/* Update a level 1 table entry                                      */
/*-------------------------------------------------------------------*/
int cckd_write_l1ent (DEVBLK *dev, int L1idx)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx;                    /* File index                */
off_t           off;                    /* Offset to l1 entry        */

    if (dev->cckd64)
        return cckd64_write_l1ent( dev, L1idx );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;
    off = (off_t)(CCKD_L1TAB_POS + L1idx * CCKD_L1ENT_SIZE);

    CCKD_TRACE( "file[%d] write_l1ent[%d] , 0x%16.16"PRIx64,
                sfx, L1idx, off);

    if (cckd_write (dev, sfx, off, &cckd->L1tab[sfx][L1idx], CCKD_L1ENT_SIZE) < 0)
        return -1;

    return 0;

} /* end function cckd_write_l1ent */

/*-------------------------------------------------------------------*/
/* Initial read                                                      */
/*-------------------------------------------------------------------*/
int cckd_read_init (DEVBLK *dev)
{
    CCKD_EXT*       cckd;               /* -> cckd extension         */
    int             sfx;                /* File index                */
    CKD_DEVHDR      devhdr;             /* Device header             */
    U32             imgtyp;             /* Dasd device image type    */

    if (dev->cckd64)
        return cckd64_read_init( dev );

    cckd = dev->cckd_ext;
    sfx  = cckd->sfn;

    CCKD_TRACE( "file[%d] read_init", sfx );

    /* Read the device header */
    if (cckd_read( dev, sfx, 0, &devhdr, CKD_DEVHDR_SIZE ) < 0)
        return -1;

    /* Check the device hdr */
    imgtyp = dh_devid_typ( devhdr.dh_devid );

         if (!sfx && (imgtyp & CKD_C370_TYP)) cckd->ckddasd = 1;
    else if (!sfx && (imgtyp & FBA_C370_TYP)) cckd->fbadasd = 1;
    else if (1
            && !(sfx && (imgtyp & CKD32_SF_TYP) && cckd->ckddasd)
            && !(sfx && (imgtyp & FBA32_SF_TYP) && cckd->fbadasd)
    )
    {
        // "%1d:%04X CCKD file[%d] %s: device header id error"
        WRMSG( HHC00305, "E", LCSS_DEVNUM,
            sfx, cckd_sf_name( dev, sfx ));
        return -1;
    }

    /* Read the compressed header */
    if (cckd_read_chdr( dev ) < 0)
        return -1;

    /* Read the level 1 table */
    if (cckd_read_l1( dev ) < 0)
        return -1;

    return 0;
} /* end function cckd_read_init */

/*-------------------------------------------------------------------*/
/* Read free space                                                   */
/*-------------------------------------------------------------------*/
int cckd_read_fsp( DEVBLK* dev )
{
CCKD_EXT*       cckd;                   /* -> cckd extension         */
off_t           fpos;                   /* Free space offset         */
int             sfx;                    /* File index                */
int             i;                      /* Index                     */
CCKD_FREEBLK    freeblk;                /* First freeblk read        */

    if (dev->cckd64)
        return cckd64_read_fsp( dev );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    CCKD_TRACE( "file[%d] read_fsp number %d",
                sfx, cckd->cdevhdr[sfx].free_num );

    cckd->ifb = cckd_free( dev, "ifb", cckd->ifb );

    cckd->free_count    =  0;
    cckd->free_idx1st   = -1;
    cckd->free_idxlast  = -1;
    cckd->free_idxavail = -1;

    /* Get storage for the internal free space chain
     * in a multiple of 1024 entries
     */
    i = (int) ROUND_UP( cckd->cdevhdr[sfx].free_num, CCKD_IFB_ENTS_INCR );
    if (!(cckd->ifb = cckd_calloc( dev, "ifb", i, CCKD_IFREEBLK_SIZE )))
        return -1;

    cckd->free_count = i;

    /* Build the doubly linked internal free space chain */
    if (cckd->cdevhdr[sfx].free_num)
    {
        cckd->free_idx1st = 0;

        /* Read the first freeblk to determine old/new format */
        fpos = (off_t)cckd->cdevhdr[sfx].free_off;
        if (cckd_read (dev, sfx, fpos, &freeblk, CCKD_FREEBLK_SIZE) < 0)
            return -1;

        if (memcmp(&freeblk, "FREE_BLK", 8) == 0)
        {
            /* new format free space */
            CCKD_FREEBLK *fsp;
            U32 ofree = cckd->cdevhdr[sfx].free_off;
            int n = cckd->cdevhdr[sfx].free_num * CCKD_FREEBLK_SIZE;
            if ((fsp = cckd_malloc (dev, "fsp", n)) == NULL)
                return -1;
            fpos += CCKD_FREEBLK_SIZE;
            if (cckd_read (dev, sfx, fpos, fsp, n) < 0)
                return -1;
            for (i = 0; i < cckd->cdevhdr[sfx].free_num; i++)
            {
                if (i == 0)
                    cckd->cdevhdr[sfx].free_off = fsp[i].fb_offnxt;
                else
                    cckd->ifb[i-1].ifb_offnxt = fsp[i].fb_offnxt;
                cckd->ifb[i].ifb_offnxt  = 0;
                cckd->ifb[i].ifb_len  = fsp[i].fb_len;
                cckd->ifb[i].ifb_idxprv = i - 1;
                cckd->ifb[i].ifb_idxnxt = i + 1;
            }
            cckd->ifb[i-1].ifb_idxnxt = -1;
            cckd->free_idxlast = i-1;
            fsp = cckd_free (dev, "fsp", fsp);

            /* truncate if new format free space was at the end */
            if (ofree == cckd->cdevhdr[sfx].cdh_size)
            {
                fpos = (off_t)cckd->cdevhdr[sfx].cdh_size;
                cckd_ftruncate(dev, sfx, fpos);
            }
        } /* new format free space */
        else
        {
            /* old format free space */
            fpos = (off_t)cckd->cdevhdr[sfx].free_off;
            for (i = 0; i < cckd->cdevhdr[sfx].free_num; i++)
            {
                if (cckd_read (dev, sfx, fpos, &cckd->ifb[i], CCKD_FREEBLK_SIZE) < 0)
                    return -1;
                cckd->ifb[i].ifb_idxprv = i - 1;
                cckd->ifb[i].ifb_idxnxt = i + 1;
                fpos = (off_t)cckd->ifb[i].ifb_offnxt;
            }
            cckd->ifb[i-1].ifb_idxnxt = -1;
            cckd->free_idxlast = i-1;
        } /* old format free space */
    } /* if (cckd->cdevhdr[sfx].free_num) */

    /* Build singly linked chain of available free space entries */
    if (cckd->cdevhdr[sfx].free_num < cckd->free_count)
    {
        cckd->free_idxavail = cckd->cdevhdr[sfx].free_num;
        for (i = cckd->free_idxavail; i < cckd->free_count; i++)
            cckd->ifb[i].ifb_idxnxt = i + 1;
        cckd->ifb[i-1].ifb_idxnxt = -1;
    }

    /* Set minimum free space size */
    cckd->free_minsize = CCKD_MIN_FREESIZE( cckd->free_count );
    return 0;

} /* end function cckd_read_fsp */

/*-------------------------------------------------------------------*/
/* Write the free space                                              */
/*-------------------------------------------------------------------*/
int cckd_write_fsp (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
off_t           fpos;                   /* Free space offset         */
U32             ppos;                   /* Previous free space offset*/
int             sfx;                    /* File index                */
int             i, j, n;                /* Work variables            */
int             rc;                     /* Return code               */
CCKD_FREEBLK   *fsp = NULL;             /* -> new format free space  */

    if (dev->cckd64)
        return cckd64_write_fsp( dev );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;

    if (!cckd->ifb)
        return 0;

    CCKD_TRACE( "file[%d] write_fsp number %d",
                sfx, cckd->cdevhdr[sfx].free_num);

    /* get rid of pending free space */
    for (i = 0; i < CCKD_MAX_FREEPEND; i++)
        cckd_flush_space(dev);

    /* sanity checks */
    if (cckd->cdevhdr[sfx].free_num == 0 || cckd->cdevhdr[sfx].free_off == 0)
    {
        cckd->cdevhdr[sfx].free_num = cckd->cdevhdr[sfx].free_off = 0;
        cckd->free_idx1st = cckd->free_idxlast = cckd->free_idxavail = -1;
    }

    /* Write any free spaces */
    if (cckd->cdevhdr[sfx].free_off)
    {
        /* size needed for new format free space */
        n = (cckd->cdevhdr[sfx].free_num+1) * CCKD_FREEBLK_SIZE;

        /* look for existing free space to fit new format free space */
        fpos = 0;
        for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
            if (n <= (int)cckd->ifb[i].ifb_len)
                break;
        if (i >= 0)
            fpos = cckd->ifb[i].ifb_idxprv < 0
                 ? (off_t)cckd->cdevhdr[sfx].free_off
                 : (off_t)cckd->ifb[cckd->ifb[i].ifb_idxprv].ifb_offnxt;

        /* if no applicable space see if we can append to the file */
        if (fpos == 0 && (cckd->cckd_maxsize - cckd->cdevhdr[sfx].cdh_size) >= (U64)n)
            fpos = (off_t)cckd->cdevhdr[sfx].cdh_size;

        if (fpos && (fsp = cckd_malloc (dev, "fsp", n)) == NULL)
            fpos = 0;

        if (fpos)
        {
            /* New format free space */
            memcpy (&fsp[0], "FREE_BLK", 8);
            ppos = cckd->cdevhdr[sfx].free_off;
            for (i = cckd->free_idx1st, j = 1; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
            {
                fsp[j].fb_offnxt = ppos;
                fsp[j++].fb_len = cckd->ifb[i].ifb_len;
                ppos = cckd->ifb[i].ifb_offnxt;
            }
            rc = cckd_write (dev, sfx, fpos, fsp, n);
            fsp = cckd_free (dev, "fsp", fsp);
            if (rc < 0)
                return -1;
            cckd->cdevhdr[sfx].free_off = (U32)fpos;
        } /* new format free space */
        else
        {
            /* Old format free space */
            for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
            {
                if (cckd_write (dev, sfx, fpos, &cckd->ifb[i], CCKD_FREEBLK_SIZE) < 0)
                    return -1;
                fpos = (off_t)cckd->ifb[i].ifb_offnxt;
            }
        } /* old format free space */
    } /* if (cckd->cdevhdr[sfx].free_off) */

    /* Free the free space array */
    cckd->ifb = cckd_free (dev, "ifb", cckd->ifb);
    cckd->free_count = 0;
    cckd->free_idx1st = cckd->free_idxlast = cckd->free_idxavail = -1;

    return 0;

} /* end function cckd_write_fsp */

/*-------------------------------------------------------------------*/
/* Read a new level 2 table                                          */
/*-------------------------------------------------------------------*/
int cckd_read_l2 (DEVBLK *dev, int sfx, int L1idx)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
off_t           off;                    /* L2 file offset            */
int             fnd;                    /* Found cache               */
int             lru;                    /* Oldest available cache    */
CCKD_L2ENT     *buf;                    /* -> Cache buffer           */
int             i;                      /* Loop index                */
int             nullfmt;                /* Null track format         */

    if (dev->cckd64)
        return cckd64_read_l2( dev, sfx, L1idx );

    cckd = dev->cckd_ext;
    nullfmt = cckd->cdevhdr[cckd->sfn].cdh_nullfmt;

    CCKD_TRACE( "file[%d] read_l2 %d active %d %d %d",
                sfx, L1idx, cckd->sfx, cckd->L1idx, cckd->L2_active);

    /* Return if table is already active */
    if (sfx == cckd->sfx && L1idx == cckd->L1idx) return 0;

    cache_lock(CACHE_L2);

    /* Inactivate the previous entry */
    if (cckd->L2_active >= 0)
        cache_setflag(CACHE_L2, cckd->L2_active, ~L2_CACHE_ACTIVE, 0);
    cckd->L2tab = NULL;
    cckd->L2_active = cckd->sfx = cckd->L1idx = -1;

    /* scan the cache array for the l2tab */
    fnd = cache_lookup (CACHE_L2, L2_CACHE_SETKEY(sfx, dev->devnum, L1idx), &lru);

    /* check for level 2 cache hit */
    if (fnd >= 0)
    {
        CCKD_TRACE( "l2[%d,%d] cache[%d] hit", sfx, L1idx, fnd);
        cache_setflag (CACHE_L2, fnd, 0, L2_CACHE_ACTIVE);
        cache_setage (CACHE_L2, fnd);
        cckdblk.stats_l2cachehits++;
        cache_unlock (CACHE_L2);
        cckd->sfx = sfx;
        cckd->L1idx = L1idx;
        cckd->L2tab = cache_getbuf(CACHE_L2, fnd, 0);
        cckd->L2_active = fnd;
        return 1;
    }

    CCKD_TRACE( "l2[%d,%d] cache[%d] miss", sfx, L1idx, lru);

    /* Steal an entry if all are busy */
    if (lru < 0) lru = cckd_steal_l2();

    /* Make the entry active */
    cache_setkey (CACHE_L2, lru, L2_CACHE_SETKEY(sfx, dev->devnum, L1idx));
    cache_setflag (CACHE_L2, lru, 0, L2_CACHE_ACTIVE);
    cache_setage (CACHE_L2, lru);
    buf = cache_getbuf(CACHE_L2, lru, CCKD_L2TAB_SIZE);
    cckdblk.stats_l2cachemisses++;
    cache_unlock (CACHE_L2);
    if (buf == NULL) return -1;

    /* Check for null table */
    if (cckd->L1tab[sfx][L1idx] == 0)
    {
        memset(buf, 0, CCKD_L2TAB_SIZE);
        if (nullfmt)
            for (i = 0; i < 256; i++)
                buf[i].L2_len = buf[i].L2_size = nullfmt;
        CCKD_TRACE( "l2[%d,%d] cache[%d] null fmt[%d]", sfx, L1idx, lru, nullfmt);
    }
    else if (cckd->L1tab[sfx][L1idx] == CCKD_MAXSIZE)
    {
        memset(buf, 0xff, CCKD_L2TAB_SIZE);
        CCKD_TRACE( "l2[%d,%d] cache[%d] null 0xff", sfx, L1idx, lru);
    }
    /* Read the new level 2 table */
    else
    {
        off = (off_t)cckd->L1tab[sfx][L1idx];
        if (cckd_read (dev, sfx, off, buf, CCKD_L2TAB_SIZE) < 0)
        {
            cache_lock(CACHE_L2);
            cache_setflag(CACHE_L2, lru, 0, 0);
            cache_unlock(CACHE_L2);
            return -1;
        }

        if (cckd->swapend[sfx])
            cckd_swapend_l2 (buf);

        CCKD_TRACE( "file[%d] cache[%d] l2[%d] read offset 0x%8.8"PRIx32,
                    sfx, lru, L1idx, cckd->L1tab[sfx][L1idx]);

        cckd->L2_reads[sfx]++;
        cckd->totl2reads++;
        cckdblk.stats_l2reads++;
    }

    cckd->sfx = sfx;
    cckd->L1idx = L1idx;
    cckd->L2tab = buf;
    cckd->L2_active = lru;

    return 0;

} /* end function cckd_read_l2 */

/*-------------------------------------------------------------------*/
/* Purge all l2tab cache entries for a given device                  */
/*-------------------------------------------------------------------*/
void cckd_purge_l2 (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */

    if (dev->cckd64)
    {
        cckd64_purge_l2( dev );
        return;
    }

    cckd = dev->cckd_ext;

    CCKD_TRACE( "purge_l2%s", "");

    cache_lock (CACHE_L2);
    cckd->L2_active = cckd->sfx = cckd->L1idx = -1;
    cckd->L2tab = NULL;
    cache_scan (CACHE_L2, cckd_purge_l2_scan, dev);
    cache_unlock (CACHE_L2);
}
int cckd_purge_l2_scan (int *answer, int ix, int i, void *data)
{
U16             sfx;                    /* Cached suffix             */
U16             devnum;                 /* Cached device number      */
U32             L1idx;                  /* Cached level 1 index      */
DEVBLK         *dev = data;             /* -> device block           */

    UNREFERENCED(answer);
    L2_CACHE_GETKEY(i, sfx, devnum, L1idx);
    if (dev == NULL || devnum == dev->devnum)
    {
        CCKD_TRACE( "purge l2cache[%d] %4.4X sfx %d ix %d purged",
                    i, devnum, sfx, L1idx);
        cache_release(ix, i, 0);
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* Steal an l2tab cache entry                                        */
/*-------------------------------------------------------------------*/
int cckd_steal_l2 ()
{
DEVBLK         *dev;                    /* -> device block           */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             i;                      /* Stolen cache index        */
U16             sfx;                    /* Cached suffix             */
U16             devnum;                 /* Cached device number      */
U32             L1idx;                  /* Cached level 1 index      */

    i = cache_scan (CACHE_L2, cckd_steal_l2_scan, NULL);
    L2_CACHE_GETKEY(i, sfx, devnum, L1idx);
    dev = cckd_find_device_by_devnum(devnum);
    if (dev->cckd64)
        return cckd64_steal_l2();
    cckd = dev->cckd_ext;
    cckd->L2_active = cckd->sfx = cckd->L1idx = -1;
    cckd->L2tab = NULL;
    cache_release(CACHE_L2, i, 0);
    return i;
}
int cckd_steal_l2_scan (int *answer, int ix, int i, void *data)
{
    UNREFERENCED(data);
    if (*answer < 0) *answer = i;
    else if (cache_getage(ix, i) < cache_getage(ix, *answer))
        *answer = i;
    return 0;
}

/*-------------------------------------------------------------------*/
/* Write the current level 2 table                                   */
/*-------------------------------------------------------------------*/
int cckd_write_l2 (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx,L1idx;              /* Lookup table indices      */
off_t           off, old_off;           /* New/old L2 file offsets   */
int             size = CCKD_L2TAB_SIZE; /* L2 table size             */
int             fix;                    /* Null format type          */

    if (dev->cckd64)
        return cckd64_write_l2( dev );

    cckd = dev->cckd_ext;
    sfx = cckd->sfn;
    L1idx = cckd->L1idx;
    fix = cckd->cdevhdr[sfx].cdh_nullfmt;
    cckd->L2ok = 0;

    CCKD_TRACE( "file[%d] write_l2 %d", sfx, L1idx);

    if (sfx < 0 || L1idx < 0) return -1;

    old_off = (off_t)cckd->L1tab[sfx][L1idx];

    if (cckd->L1tab[sfx][L1idx] == CCKD_NOSIZE || cckd->L1tab[sfx][L1idx] == CCKD_MAXSIZE)
        cckd->L2_bounds += CCKD_L2TAB_SIZE;

    /* Write the L2 table if it's not empty */
    if (memcmp( cckd->L2tab, &empty_l2[fix], CCKD_L2TAB_SIZE ))
    {
        if ((off = cckd_get_space( dev, &size, CCKD_L2SPACE )) < 0)
            return -1;
        if (cckd_write( dev, sfx, off, cckd->L2tab, CCKD_L2TAB_SIZE ) < 0)
            return -1;
    }
    else
    {
        off = 0;
        cckd->L2_bounds -= CCKD_L2TAB_SIZE;
    }

    /* Free the old L2 space */
    cckd_rel_space( dev, old_off, CCKD_L2TAB_SIZE, CCKD_L2TAB_SIZE );

    /* Update level 1 table */
    cckd->L1tab[sfx][L1idx] = (U32)off;

    if (cckd_write_l1ent( dev, L1idx ) < 0)
        return -1;

    return 0;

} /* end function cckd_write_l2 */

/*-------------------------------------------------------------------*/
/* Return a level 2 entry                                            */
/*-------------------------------------------------------------------*/
int cckd_read_l2ent (DEVBLK *dev, CCKD_L2ENT *l2, int trk)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx,L1idx,l2x;          /* Lookup table indices      */

    if (dev->cckd64)
        return cckd64_read_l2ent( dev, (CCKD64_L2ENT*) l2, trk );

    cckd = dev->cckd_ext;

    L1idx = trk >> 8;
    l2x = trk & 0xff;

    if (l2 != NULL) l2->L2_trkoff = l2->L2_len = l2->L2_size = 0;

    for (sfx = cckd->sfn; sfx >= 0; sfx--)
    {
        CCKD_TRACE( "file[%d] l2[%d,%d] trk[%d] read_l2ent 0x%x",
                    sfx, L1idx, l2x, trk, cckd->L1tab[sfx][L1idx]);

        /* Continue if l2 table not in this file */
        if (cckd->L1tab[sfx][L1idx] == CCKD_MAXSIZE)
            continue;

        /* Read l2 table from this file */
        if (cckd_read_l2 (dev, sfx, L1idx) < 0)
            return -1;

        /* Exit loop if track is in this file */
        if (cckd->L2tab[l2x].L2_trkoff != CCKD_MAXSIZE)
            break;
    }

    CCKD_TRACE( "file[%d] l2[%d,%d] trk[%d] read_l2ent 0x%x %d %d",
                sfx, L1idx, l2x, trk, sfx >= 0 ? cckd->L2tab[l2x].L2_trkoff : 0,
                sfx >= 0 ? cckd->L2tab[l2x].L2_len : 0,
                sfx >= 0 ? cckd->L2tab[l2x].L2_size : 0);

    if (l2 != NULL && sfx >= 0)
    {
        l2->L2_trkoff  = cckd->L2tab[l2x].L2_trkoff;
        l2->L2_len  = cckd->L2tab[l2x].L2_len;
        l2->L2_size = cckd->L2tab[l2x].L2_size;
    }

    return sfx;

} /* end function cckd_read_l2ent */

/*-------------------------------------------------------------------*/
/* Update a level 2 entry                                            */
/*-------------------------------------------------------------------*/
int cckd_write_l2ent (DEVBLK *dev,  CCKD_L2ENT *l2, int trk)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx,L1idx,l2x;          /* Lookup table indices      */
off_t           off;                    /* L2 entry offset           */

    if (dev->cckd64)
        return cckd64_write_l2ent( dev, (CCKD64_L2ENT*) l2, trk );

    cckd = dev->cckd_ext;

    /* Error return if no available level 2 table */
    if (!cckd->L2tab) return -1;

    sfx = cckd->sfn;
    L1idx = trk >> 8;
    l2x = trk & 0xff;

    /* Copy the new entry if passed */
    if (l2) memcpy (&cckd->L2tab[l2x], l2, CCKD_L2ENT_SIZE);

    CCKD_TRACE( "file[%d] l2[%d,%d] trk[%d] write_l2ent 0x%x %d %d",
                sfx, L1idx, l2x, trk,
                cckd->L2tab[l2x].L2_trkoff, cckd->L2tab[l2x].L2_len, cckd->L2tab[l2x].L2_size);

    /* If no level 2 table for this file, then write a new one */
    if (cckd->L1tab[sfx][L1idx] == CCKD_NOSIZE || cckd->L1tab[sfx][L1idx] == CCKD_MAXSIZE)
        return cckd_write_l2 (dev);

    /* Write the level 2 table entry */
    off = (off_t)(cckd->L1tab[sfx][L1idx] + l2x * CCKD_L2ENT_SIZE);
    if (cckd_write (dev, sfx, off, &cckd->L2tab[l2x], CCKD_L2ENT_SIZE) < 0)
        return -1;

    return 0;
} /* end function cckd_write_l2ent */

/*-------------------------------------------------------------------*/
/* Read a track image                                                */
/*-------------------------------------------------------------------*/
int cckd_read_trkimg (DEVBLK *dev, BYTE *buf, int trk, BYTE *unitstat)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             sfx;                    /* File index                */
CCKD_L2ENT      l2;                     /* Level 2 entry             */

    cckd = dev->cckd_ext;

    CCKD_TRACE( "trk[%d] read_trkimg", trk);

    /* Read level 2 entry for the track */
    if ((sfx = cckd_read_l2ent (dev, &l2, trk)) < 0)
        goto cckd_read_trkimg_error;

    /* Read the track image or build a null track image */
    if (l2.L2_trkoff != 0)
    {
        rc = cckd_read (dev, sfx, (off_t)l2.L2_trkoff, buf, (size_t)l2.L2_len);
        if (rc < 0)
            goto cckd_read_trkimg_error;

        cckd->reads[sfx]++;
        cckd->totreads++;
        cckdblk.stats_reads++;
        cckdblk.stats_readbytes += rc;
        if (cckd->notnull == 0 && trk > 1) cckd->notnull = 1;
    }
    else
        rc = cckd_null_trk (dev, buf, trk, l2.L2_len);

    /* Validate the track image */
    if (cckd_cchh (dev, buf, trk) < 0)
        goto cckd_read_trkimg_error;

    return rc;

cckd_read_trkimg_error:

    if (unitstat)
    {
        ckd_build_sense (dev, SENSE_EC, 0, 0, FORMAT_1, MESSAGE_0);
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
    }

    return cckd_null_trk (dev, buf, trk, 0);

} /* end function cckd_read_trkimg */

/*-------------------------------------------------------------------*/
/* Write a track image                                               */
/*-------------------------------------------------------------------*/
int cckd_write_trkimg (DEVBLK *dev, BYTE *buf, int len, int trk, int flags)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
off_t           off;                    /* File offset               */
CCKD_L2ENT      l2, oldl2;              /* Level 2 entries           */
int             sfx,L1idx,l2x;          /* Lookup table indices      */
int             after = 0;              /* 1=New track after old     */
int             size;                   /* Size of new track         */

    if (dev->cckd64)
        return cckd64_write_trkimg( dev, buf, len, trk, flags );

    cckd = dev->cckd_ext;

    sfx = cckd->sfn;
    L1idx = trk >> 8;
    l2x = trk & 0xff;

    CCKD_TRACE( "file[%d] trk[%d] write_trkimg len %d buf %p:%2.2x%2.2x%2.2x%2.2x%2.2x",
                sfx, trk, len, buf, buf[0], buf[1], buf[2], buf[3], buf[4]);

    /* Validate the new track image */
    if (cckd_cchh (dev, buf, trk) < 0)
        return -1;

    /* Get the level 2 table for the track in the active file */
    if (cckd_read_l2 (dev, sfx, L1idx) < 0)
        return -1;

    /* Save the level 2 entry for the track */
    oldl2.L2_trkoff = cckd->L2tab[l2x].L2_trkoff;
    oldl2.L2_len    = cckd->L2tab[l2x].L2_len;
    oldl2.L2_size   = cckd->L2tab[l2x].L2_size;
    CCKD_TRACE( "file[%d] trk[%d] write_trkimg oldl2 0x%x %d %d",
                sfx, trk, oldl2.L2_trkoff, oldl2.L2_len, oldl2.L2_size);

    /* Check if writing a null track */
    len = cckd_check_null_trk(dev, buf, trk, len);

    if (len > CKD_NULLTRK_FMTMAX)
    {
        /* Get space for the track image */
        size = len;
        if ((off = cckd_get_space (dev, &size, flags)) < 0)
            return -1;

        l2.L2_trkoff = (U32)off;
        l2.L2_len    = (U16)len;
        l2.L2_size   = (U16)size;

        if (1
            && oldl2.L2_trkoff != CCKD_NOSIZE
            && oldl2.L2_trkoff != CCKD_MAXSIZE
            && oldl2.L2_trkoff < l2.L2_trkoff
        )
            after = 1;

        /* Write the track image */
        if ((rc = cckd_write (dev, sfx, off, buf, len)) < 0)
            return -1;

        cckd->writes[sfx]++;
        cckd->totwrites++;
        cckdblk.stats_writes++;
        cckdblk.stats_writebytes += rc;
    }
    else
    {
        l2.L2_trkoff = 0;
        l2.L2_len = l2.L2_size = (U16)len;
    }

    /* Update the level 2 entry */
    if (cckd_write_l2ent (dev, &l2, trk) < 0)
        return -1;

    /* Release the previous space */
    cckd_rel_space (dev, (off_t)oldl2.L2_trkoff, (int)oldl2.L2_len, (int)oldl2.L2_size);

    /* `after' is 1 if the new offset is after the old offset */
    return after;

} /* end function cckd_write_trkimg */

/*-------------------------------------------------------------------*/
/* Harden the file                                                   */
/*-------------------------------------------------------------------*/
int cckd_harden(DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc=0;                   /* Return code               */

    if (dev->cckd64)
        return cckd64_harden( dev );

    cckd = dev->cckd_ext;

    if ((dev->ckdrdonly && cckd->sfn == 0)
     || cckd->open[cckd->sfn] != CCKD_OPEN_RW)
        return 0;

    CCKD_TRACE( "file[%d] harden", cckd->sfn);

    /* Write the compressed device header */
    if (cckd_write_chdr (dev) < 0)
        rc = -1;

    /* Write the level 1 table */
    if (cckd_write_l1 (dev) < 0)
        rc = -1;

    /* Write the free space chain */
    if (cckd_write_fsp (dev) < 0)
        rc = -1;

    /* Re-write the compressed device header */
    cckd->cdevhdr[cckd->sfn].cdh_opts &= ~CCKD_OPT_OPENED;
    if (cckd_write_chdr (dev) < 0)
        rc = -1;

    if (cckdblk.fsync)
        fdatasync (cckd->fd[cckd->sfn]);

    return rc;
} /* cckd_harden */

/*-------------------------------------------------------------------*/
/* Return length of an uncompressed track image                      */
/*-------------------------------------------------------------------*/
int cckd_trklen( DEVBLK* dev, BYTE* buf )
{
    CCKD_EXT*  cckd;                    /* -> cckd extension         */
    int        size;                    /* Track size                */

    if ((cckd = dev->cckd_ext)->fbadasd)
        return CKD_TRKHDR_SIZE + CFBA_BLKGRP_SIZE;

    size = ckd_tracklen( dev, buf );

    /* check for missing end-of-track indicator */
    if (0
        || size > dev->ckdtrksz
        || memcmp( &buf[ size - CKD_ENDTRK_SIZE ], &CKD_ENDTRK, CKD_ENDTRK_SIZE ) != 0
    )
    {
        CKD_TRKHDR*  trkhdr  = (CKD_TRKHDR*) buf;

        // "%1d:%04X CCKD file[%d] %s: trklen error for BCCHH = %2.2x%4.4x%4.4x"
        WRMSG( HHC00306, "E", LCSS_DEVNUM, cckd->sfn, cckd_sf_name( dev, cckd->sfn ),
               trkhdr->bin, fetch_hw( trkhdr->cyl ), fetch_hw( trkhdr->head ));

        size = -1;
    }

    return size;
}

/*-------------------------------------------------------------------*/
/* Build a null track                                                */
/*-------------------------------------------------------------------*/
int cckd_null_trk(DEVBLK *dev, BYTE *buf, int trk, int nullfmt)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             i;                      /* Loop counter              */
CKD_TRKHDR     *trkhdr;                 /* -> Track header           */
CKD_RECHDR     *rechdr;                 /* -> Record header          */
U32             cyl;                    /* Cylinder number           */
U32             head;                   /* Head number               */
BYTE            r;                      /* Record number             */
BYTE           *pos;                    /* -> Next position in buffer*/
int             len;                    /* Length of null track      */

    if (dev->cckd64)
        return cckd64_null_trk( dev, buf, trk, nullfmt );

    cckd = dev->cckd_ext;

    if (nullfmt < 0 || nullfmt > CKD_NULLTRK_FMTMAX)
        nullfmt = cckd->cdevhdr[cckd->sfn].cdh_nullfmt;

    // FIXME
    // Compatibility check for nullfmt bug and linux -- 18 May 2005
    // Remove at some reasonable date in the future
    else if (nullfmt == 0
     && cckd->cdevhdr[cckd->sfn].cdh_nullfmt == CKD_NULLTRK_FMT2)
        nullfmt = CKD_NULLTRK_FMT2;

    if (cckd->ckddasd)
    {

        /* cylinder and head calculations */
        cyl = trk / dev->ckdheads;
        head = trk % dev->ckdheads;

        /* Build the track header */
        trkhdr = (CKD_TRKHDR*)buf;
        trkhdr->bin = 0;
        store_hw( &trkhdr->cyl,  (U16) cyl  );
        store_hw( &trkhdr->head, (U16) head );
        pos = buf + CKD_TRKHDR_SIZE;

        /* Build record zero (R0): KL=0, DL=8 */
        r = 0;
        rechdr = (CKD_RECHDR*)pos;
        pos += CKD_RECHDR_SIZE;
        store_hw( &rechdr->cyl,  (U16) cyl  );
        store_hw( &rechdr->head, (U16) head );
        rechdr->rec = r;
        rechdr->klen = 0;
        store_hw( &rechdr->dlen, CKD_R0_DLEN );
        memset( pos, 0,          CKD_R0_DLEN );
        pos +=                   CKD_R0_DLEN;
        r++;

        /* Specific null track formatting */
        if (nullfmt == CKD_NULLTRK_FMT0)
        {
            rechdr = (CKD_RECHDR*)pos;
            pos += CKD_RECHDR_SIZE;

            /* Build record one (R1): EOF record (KL=0, DL=0) */
            store_hw( &rechdr->cyl,  (U16) cyl  );
            store_hw( &rechdr->head, (U16) head );
            rechdr->rec = r;
            rechdr->klen = 0;
            store_hw(&rechdr->dlen, 0);
            r++;
        }
        else if (nullfmt == CKD_NULLTRK_FMT2)
        {
            /* Build 12 binary-0 data blocks, 4K each (KL=0, DL=4K) */
            for (i = 0; i < 12; i++)
            {
                rechdr = (CKD_RECHDR*)pos;
                pos += CKD_RECHDR_SIZE;

                store_hw( &rechdr->cyl,  (U16) cyl);
                store_hw( &rechdr->head, (U16) head);
                rechdr->rec = r;
                rechdr->klen = 0;
                store_hw( &rechdr->dlen, CKD_NULL_FMT2_DLEN );
                r++;
                memset( pos, 0,          CKD_NULL_FMT2_DLEN );
                pos +=                   CKD_NULL_FMT2_DLEN;
            }
        }

        /* Build the end of track marker */
        memcpy( pos, &CKD_ENDTRK, CKD_ENDTRK_SIZE );
        pos +=                    CKD_ENDTRK_SIZE;
        len = (int)(pos - buf);
    }
    else
    {
        len = CKD_TRKHDR_SIZE + CFBA_BLKGRP_SIZE;
        memset( buf, 0, len );
        store_fw( buf+1, trk );
    }

    CCKD_TRACE( "null_trk %s %d format %d size %d",
                cckd->ckddasd ? "trk" : "blkgrp", trk, nullfmt, len);

    return len;

} /* end function cckd_null_trk */

/*-------------------------------------------------------------------*/
/* Return a number 0 .. CKD_NULLTRK_FMTMAX if track is null          */
/* else return the original length                                   */
/*-------------------------------------------------------------------*/
int cckd_check_null_trk (DEVBLK *dev, BYTE *buf, int trk, int len)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
BYTE            buf2[65536];            /* Null track buffer         */

    if (dev->cckd64)
        return cckd64_check_null_trk( dev, buf, trk, len );

    cckd = dev->cckd_ext;
    rc = len;

    if (len == CKD_NULLTRK_SIZE0)
        rc = CKD_NULLTRK_FMT0;
    else if (len == CKD_NULLTRK_SIZE1)
        rc = CKD_NULLTRK_FMT1;
    else if (len == CKD_NULLTRK_SIZE2 && dev->oslinux
          && (!cckd->notnull || cckdblk.linuxnull))
    {
         cckd_null_trk (dev, buf2, trk, 0);
         if (memcmp(buf, buf2, len) == 0)
            rc = CKD_NULLTRK_FMT2;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Verify a track/block header and return track/block number         */
/*-------------------------------------------------------------------*/
int cckd_cchh (DEVBLK *dev, BYTE *buf, int trk)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
U16             cyl;                    /* Cylinder                  */
U16             head;                   /* Head                      */
int             t;                      /* Calculated track          */
BYTE            badcomp=0;              /* 1=Unsupported compression */

    cckd = dev->cckd_ext;

    /* CKD dasd header verification */
    if (cckd->ckddasd)
    {
        cyl = fetch_hw (buf + 1);
        head = fetch_hw (buf + 3);
        t = cyl * dev->ckdheads + head;

        if (1
            && cyl  < dev->ckdcyls
            && head < dev->ckdheads
            && (trk == -1 || t == trk)
        )
        {
            if (buf[0] & ~cckdblk.comps)
            {
                if (buf[0] & ~CCKD_COMPRESS_MASK)
                {
                    if (cckdblk.bytemsgs++ < 10)
                        // "%1d:%04X CCKD file[%d] %s: invalid byte 0 trk %d, buf %2.2x%2.2x%2.2x%2.2x%2.2x"
                        WRMSG (HHC00307, "E", LCSS_DEVNUM, cckd->sfn,
                            cckd_sf_name (dev, cckd->sfn), t, buf[0],buf[1],buf[2],buf[3],buf[4]);
                    buf[0] &= CCKD_COMPRESS_MASK;
                }
            }
            if (buf[0] & ~cckdblk.comps)
                badcomp = 1;
            else
                return t;
        }
    }
    /* FBA dasd header verification */
    else
    {
        t = fetch_fw (buf + 1);
        if (t < dev->fbanumblk && (trk == -1 || t == trk))
        {
            if (buf[0] & ~cckdblk.comps)
            {
                if (buf[0] & ~CCKD_COMPRESS_MASK)
                {
                    // "%1d:%04X CCKD file[%d] %s: invalid byte 0 blkgrp %d, buf %2.2x%2.2x%2.2x%2.2x%2.2x"
                    WRMSG (HHC00308, "E", LCSS_DEVNUM, cckd->sfn,
                            cckd_sf_name (dev, cckd->sfn), t, buf[0],buf[1],buf[2],buf[3],buf[4]);
                    buf[0] &= CCKD_COMPRESS_MASK;
                }
            }
            if (buf[0] & ~cckdblk.comps)
                badcomp = 1;
            else
                return t;
        }
    }

    if (badcomp)
    {
        // "%1d:%04X CCKD file[%d] %s: invalid %s hdr %s %d: %s compression unsupported"
        WRMSG (HHC00309, "E", LCSS_DEVNUM, cckd->sfn, cckd_sf_name (dev, cckd->sfn),
                cckd->ckddasd ? "trk" : "blk",
                cckd->ckddasd ? "trk" : "blk", t, compname[buf[0]]);
    }
    else
    {
        // "%1d:%04X CCKD file[%d] %s: invalid %s hdr %s %d buf %p:%2.2x%2.2x%2.2x%2.2x%2.2x"
        WRMSG (HHC00310, "E", LCSS_DEVNUM, cckd->sfn, cckd_sf_name (dev, cckd->sfn),
                cckd->ckddasd ? "trk" : "blk",
                cckd->ckddasd ? "trk" : "blk", trk,
                buf, buf[0], buf[1], buf[2], buf[3], buf[4]);
        cckd_print_itrace ();
    }

    return -1;
} /* end function cckd_cchh */

/*-------------------------------------------------------------------*/
/* Validate a track image                                            */
/*-------------------------------------------------------------------*/
int cckd_validate (DEVBLK *dev, BYTE *buf, int trk, int len)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             r;                      /* Record number             */
int             sz;                     /* Track size                */
int             vlen;                   /* Validation length         */
int             kl, dl;                 /* Key/Data lengths          */
CKD_RECHDR      rn;                     /* Record-n (r0, r1 ... rn)  */

    cckd = dev->cckd_ext;

    if (!buf || len < 0)
        return -1;

    CCKD_TRACE( "validating %s %d len %d %2.2x%2.2x%2.2x%2.2x%2.2x "
                "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                cckd->ckddasd ? "trk" : "blkgrp", trk, len,
                buf[0], buf[ 1], buf[ 2], buf[ 3], buf[4], // trkhdr
                buf[5], buf[ 6], buf[ 7], buf[ 8],         // r0 (0-3)
                buf[9], buf[10], buf[11], buf[12] );       // r0 (4-7)

    /* FBA dasd check */
    if (cckd->fbadasd)
    {
        if (!len || len == CKD_TRKHDR_SIZE + CFBA_BLKGRP_SIZE)
            return len;
        CCKD_TRACE( "validation failed: bad length%s", "" );
        return -1;
    }

    /* validate record 0 */

    memcpy( &rn, &buf[CKD_TRKHDR_SIZE], CKD_R0_SIZE );
    rn.cyl[0] &= 0x7f; /* fix for ovflow */

    if (0
        || rn.rec  != 0
        || rn.klen != 0
        || fetch_hw( rn.dlen ) != CKD_R0_DLEN
    )
    {
        CCKD_TRACE( "validation failed: bad r0%s", "" );
        return -1;
    }

    /* validate records 1 thru n */

    vlen = len > 0 ? len : dev->ckdtrksz;

    for (r = 1, sz = CKD_TRKHDR_SIZE + (CKD_R0_SIZE + CKD_R0_DLEN);
        sz +  CKD_ENDTRK_SIZE <= vlen;
        sz += CKD_RECHDR_SIZE + kl + dl, r++)
    {
        memcpy( &rn, &buf[sz], CKD_RECHDR_SIZE );

        if (memcmp( &rn, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
            break;

        kl = rn.klen;
        dl = fetch_hw( rn.dlen );

        if (rn.rec == 0 || sz + CKD_RECHDR_SIZE + kl + dl >= vlen)
        {
            CCKD_TRACE( "validation failed: bad r%d "
                        "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                        r, buf[sz+0], buf[sz+1], buf[sz+2], buf[sz+3],
                           buf[sz+4], buf[sz+5], buf[sz+6], buf[sz+7] );
             return -1;
        }
    }

    sz += CKD_ENDTRK_SIZE;

    if ((len > 0 && sz != len) || sz > vlen)
    {
        CCKD_TRACE( "validation failed: no eot%s", "" );
        return -1;
    }

    return sz;

} /* end function cckd_validate */

/*-------------------------------------------------------------------*/
/* Return shadow file name                                           */
/*-------------------------------------------------------------------*/
char *cckd_sf_name (DEVBLK *dev, int sfx)
{
    /* Return base file name if index is 0 */
    if (sfx == 0)
        return dev->filename;

    /* Error if no shadow file name specified or number exceeded */
    if (dev->dasdsfn == NULL || sfx > CCKD_MAX_SF)
        return NULL;

    /* Set the suffix character in the shadow file name */
    if (sfx > 0)
        *dev->dasdsfx = (char)('0' + sfx);
    else
        *dev->dasdsfx = '*';

    return dev->dasdsfn;

} /* end function cckd_sf_name */

/*-------------------------------------------------------------------*/
/* Parse the shadow file name option                                 */
/*-------------------------------------------------------------------*/
void cckd_sf_parse_sfn( DEVBLK* dev, char* sfn )
{
    char pathname[MAX_PATH];
    if ('\"' == sfn[0]) sfn++;
    hostpath(pathname, sfn, sizeof(pathname));
    dev->dasdsfn = strdup(pathname);
    if (dev->dasdsfn)
    {
        /*
        **         Set the pointer to the suffix character
        **
        ** The shadow file name should have spot where the shadow file
        ** number will be set.  This is either the character preceding
        ** the last period after the last slash (i.e. the last character
        ** of the filename, immediately before the file extension), or
        ** the very last character of the filename itself if the file
        ** extension was not specified.
        */
        char* fn;
        if ((fn = strrchr( dev->dasdsfn, PATHSEPC )) != NULL)
            fn++;
        else
            fn = dev->dasdsfn;

        dev->dasdsfx = strrchr( fn, '.' );  /* Find last period */

        if (dev->dasdsfx == NULL)
            dev->dasdsfx = dev->dasdsfn + strlen(dev->dasdsfn);

        dev->dasdsfx--;     /* Shadow file number will go here */
    }
}

/*-------------------------------------------------------------------*/
/* Initialize shadow files                                           */
/*-------------------------------------------------------------------*/
int cckd_sf_init (DEVBLK *dev)
{
CKD_DEVHDR      devhdr;                 /* Device header             */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             i;                      /* Index                     */
struct stat     st;                     /* stat() buffer             */
char            pathname[MAX_PATH];     /* file path in host format  */

    if (dev->cckd64)
        return cckd64_sf_init( dev );

    cckd = dev->cckd_ext;

    /* return if no shadow files */
    if (dev->dasdsfn == NULL) return 0;

    /* Check for shadow file name collision */
    for (i = 1; i <= CCKD_MAX_SF && dev->dasdsfn != NULL; i++)
    {
     DEVBLK       *dev2;
     CCKD_EXT     *cckd2;
     int           j;

        for (dev2 = cckdblk.dev1st; dev2; dev2 = cckd2->devnext)
        {
            cckd2 = dev2->cckd_ext;
            if (dev2 == dev) continue;
            for (j = 0; j <= CCKD_MAX_SF && dev2->dasdsfn != NULL; j++)
            {
                if (strcmp (cckd_sf_name(dev, i),cckd_sf_name(dev2, j)) == 0)
                {
                    // "%1d:%04X CCKD file[%d] %s: shadow file name collides with %1d:%04X file[%d] %s"
                    WRMSG (HHC00311, "E", LCSS_DEVNUM,  i, cckd_sf_name(dev,  i),
                                         SSID_TO_LCSS(dev2->ssid), dev2->devnum, j, cckd_sf_name(dev2, j));
                    return -1;
                }
            }
        }
    }

    /* open all existing shadow files */
    for (cckd->sfn = 1; cckd->sfn <= CCKD_MAX_SF; cckd->sfn++)
    {
        /* If no more shadow files remaining then we're done */
        hostpath( pathname, cckd_sf_name( dev, cckd->sfn ), sizeof( pathname ));
        if (stat( pathname, &st ) < 0)
            break;

        /* Try to open the shadow file read-write then read-only */
        if (    cckd_open( dev, cckd->sfn, O_RDWR   | O_BINARY, 1 ) < 0)
            if (cckd_open( dev, cckd->sfn, O_RDONLY | O_BINARY, 0 ) < 0)
                break;

        /* Make sure shadow file type matches base image type */
        if (cckd_read( dev, cckd->sfn, 0, &devhdr, CKD_DEVHDR_SIZE ) < 0)
            return -1;
        if (!is_dh_devid_typ( devhdr.dh_devid, ANY32_SF_TYP ))
        {
            // "%1d:%04X CCKD file[%d] %s: cckd/64 format differs from base"
            WRMSG( HHC00351, "E", LCSS_DEVNUM, cckd->sfn, pathname );
            return -1;
        }

        /* Call the chkdsk function */
        if ((rc = cckd_chkdsk( dev, 0 )) < 0)
            return -1;

        /* Perform initial read */
        rc = cckd_read_init( dev );
    }

    /* Backup to the last opened file number */
    cckd->sfn--;

    /* If the last file was opened read-only then create a new one */
    if (cckd->open[cckd->sfn] == CCKD_OPEN_RO)
    {
        /* but ONLY IF not explicit batch utility READ-ONLY open */
        if (!(1
              && dev->batch
              && dev->ckdrdonly
        ))
        {
            /* NOT explicit batch utility read-only open: create new shadow file */
            if (cckd_sf_new(dev) < 0)
                return -1;
        }
    }

    /* Re-open previous rdwr files rdonly */
    for (i = 0; i < cckd->sfn; i++)
    {
        if (cckd->open[i] == CCKD_OPEN_RO) continue;
        if (cckd_open (dev, i, O_RDONLY|O_BINARY, 0) < 0)
        {
            // "%1d:%04X CCKD file[%d] %s: error re-opening readonly: %s"
            WRMSG (HHC00312, "E", LCSS_DEVNUM, i, cckd_sf_name(dev, i), strerror(errno));
            return -1;
        }
    }

    return 0;

} /* end function cckd_sf_init */

/*-------------------------------------------------------------------*/
/* Create a new shadow file                                          */
/*-------------------------------------------------------------------*/
int cckd_sf_new (DEVBLK *dev)
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             l1size;                 /* Size of level 1 table     */
CKD_DEVHDR      devhdr;                 /* Device header             */

    if (dev->cckd64)
        return cckd64_sf_new( dev );

    cckd = dev->cckd_ext;

    CCKD_TRACE( "file[%d] sf_new %s", cckd->sfn+1,
                cckd_sf_name(dev, cckd->sfn+1) ?
                (char *)cckd_sf_name(dev, cckd->sfn+1) : "(none)");

    /* Error if no shadow file name */
    if (dev->dasdsfn == NULL)
    {
        WRMSG (HHC00313, "E", LCSS_DEVNUM, cckd->sfn+1);
        return -1;
    }

    /* Error if max number of shadow files exceeded */
    if (cckd->sfn+1 == CCKD_MAX_SF)
    {
        WRMSG (HHC00314, "E", LCSS_DEVNUM, cckd->sfn+1, dev->dasdsfn);
        return -1;
    }

    /* Harden the current file */
    cckd_harden (dev);

    /* Open the new shadow file */
    if (cckd_open(dev, cckd->sfn+1, O_RDWR|O_CREAT|O_EXCL|O_BINARY,
                                      S_IRUSR | S_IWUSR | S_IRGRP) < 0)
        return -1;

    /* Read previous file's device header */
    if (cckd_read (dev, cckd->sfn, 0, &devhdr, CKD_DEVHDR_SIZE) < 0)
        goto sf_new_error;

    /* Make sure identifier is CKD_S370 or FBA_S370 */
    devhdr.dh_devid[4] = 'S';

    /* Write new file's device header */
    if (cckd_write (dev, cckd->sfn+1, 0, &devhdr, CKD_DEVHDR_SIZE) < 0)
        goto sf_new_error;

    /* Build the compressed device header */
    memcpy (&cckd->cdevhdr[cckd->sfn+1], &cckd->cdevhdr[cckd->sfn], CCKD_DEVHDR_SIZE);
    l1size = cckd->cdevhdr[cckd->sfn+1].num_L1tab * CCKD_L1ENT_SIZE;
    cckd->cdevhdr[cckd->sfn+1].cdh_size =
    cckd->cdevhdr[cckd->sfn+1].cdh_used = CKD_DEVHDR_SIZE + CCKD_DEVHDR_SIZE + l1size;
    cckd->cdevhdr[cckd->sfn+1].free_off =
    cckd->cdevhdr[cckd->sfn+1].free_total =
    cckd->cdevhdr[cckd->sfn+1].free_largest =
    cckd->cdevhdr[cckd->sfn+1].free_num =
    cckd->cdevhdr[cckd->sfn+1].free_imbed = 0;

    /* Init the level 1 table */
    if ((cckd->L1tab[cckd->sfn+1] = cckd_malloc (dev, "l1", l1size)) == NULL)
        goto sf_new_error;
    memset (cckd->L1tab[cckd->sfn+1], 0xff, l1size);

    /* Make the new file active */
    cckd->sfn++;

    /* Harden the new file */
    if (cckd_harden (dev) < 0)
    {
        cckd->sfn--;
        goto sf_new_error;
    }

    /* Re-read the l1 to set L2_bounds, L2ok */
    cckd_read_l1 (dev);

    return 0;

sf_new_error:
    cckd->L1tab[cckd->sfn+1] = cckd_free(dev, "l1", cckd->L1tab[cckd->sfn+1]);
    cckd_close (dev, cckd->sfn+1);
    cckd->open[cckd->sfn+1] = CCKD_OPEN_NONE;
    unlink (cckd_sf_name (dev, cckd->sfn+1));

    /* Re-read the l1 to set L2_bounds, L2ok */
    cckd_read_l1 (dev);

    return -1;

} /* end function cckd_sf_new */

/*-------------------------------------------------------------------*/
/* Add a shadow file  (sf+)                                          */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* cckd_sf_add( void* data )
{
DEVBLK         *dev = data;             /* -> DEVBLK                 */
CCKD_EXT       *cckd;                   /* -> cckd extension         */

    if (dev == NULL)
    {
    int n = 0;
        for (dev=sysblk.firstdev; dev; dev=dev->nextdev)
            if (dev->cckd_ext)
            {
                WRMSG (HHC00315, "I", LCSS_DEVNUM );
                cckd_sf_add (dev);
                n++;
            }
        WRMSG(HHC00316, "I", n );
        return NULL;
    }

    cckd = dev->cckd_ext;
    if (!cckd)
    {
        WRMSG (HHC00317, "E", LCSS_DEVNUM);
        return NULL;
    }

    /* Schedule updated track entries to be written */
    obtain_lock (&cckd->cckdiolock);
    if (cckd->merging)
    {
        release_lock (&cckd->cckdiolock);
        WRMSG (HHC00318, "W", LCSS_DEVNUM, cckd->sfn, cckd_sf_name (dev, cckd->sfn));
        return NULL;
    }
    cckd->merging = 1;
    cckd_flush_cache (dev);
    while (cckd->wrpending || cckd->cckdioact)
    {
        cckd->cckdwaiters++;
        wait_condition (&cckd->cckdiocond, &cckd->cckdiolock);
        cckd->cckdwaiters--;
        cckd_flush_cache (dev);
    }
    cckd_purge_cache (dev); cckd_purge_l2 (dev);
    dev->bufcur = dev->cache = -1;
    release_lock (&cckd->cckdiolock);

    /* Obtain control of the file */
    obtain_lock( &cckd->filelock );
    {
        /* Harden the current file */
        cckd_harden (dev);

        /* Create a new shadow file */
        if (cckd_sf_new (dev) < 0) {
            WRMSG (HHC00319, "E", LCSS_DEVNUM, cckd->sfn+1,
                     cckd_sf_name(dev, cckd->sfn+1)?cckd_sf_name(dev, cckd->sfn+1):"(null)");
            goto cckd_sf_add_exit;
        }

        /* Re-open the previous file if opened read-write */
        if (cckd->open[cckd->sfn-1] == CCKD_OPEN_RW)
            cckd_open (dev, cckd->sfn-1, O_RDONLY|O_BINARY, 0);

        WRMSG (HHC00320, "I", LCSS_DEVNUM, cckd->sfn, cckd_sf_name (dev, cckd->sfn));

cckd_sf_add_exit:

        /* Re-read the l1 to set L2_bounds, L2ok */
        cckd_read_l1 (dev);
    }
    release_lock( &cckd->filelock );

    obtain_lock (&cckd->cckdiolock);
    cckd->merging = 0;
    if (cckd->cckdwaiters)
        broadcast_condition (&cckd->cckdiocond);
    release_lock (&cckd->cckdiolock);

    cckd_sf_stats (dev);
    return NULL;
} /* end function cckd_sf_add */

/*-------------------------------------------------------------------*/
/* Remove a shadow file  (sf-)                                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* cckd_sf_remove( void* data )
{
DEVBLK         *dev = data;             /* -> DEVBLK                 */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             from_sfx, to_sfx;       /* From/to file index        */
int             fix;                    /* nullfmt index             */
int             add = 0;                /* 1=Add shadow file back    */
int             l2updated = 0;          /* 1=L2 table was updated    */
int             i,j;                    /* Loop indexes              */
int             merge, force;           /* Flags                     */
off_t           pos;                    /* File offset               */
unsigned int    len;                    /* Length to read/write      */
int             size;                   /* Image size                */
int             trk = -1;               /* Track being read/written  */
CCKD_L2ENT      from_l2[256],           /* Level 2 tables            */
                to_l2[256];
CCKD_L2ENT      new_l2;                 /* New level 2 table entry   */
BYTE            buf[64*1024];           /* Buffer                    */

    /* NULL DEVBLK == all devices? */
    if (!dev)
    {
    int n = 0;
        merge = cckdblk.sfmerge;
        force = cckdblk.sfforce;
        cckdblk.sfmerge = cckdblk.sfforce = 0;
        for (dev=sysblk.firstdev; dev; dev=dev->nextdev)
        {
            if ((cckd = dev->cckd_ext)) /* Compressed device? */
            {
                if (cckd->sfn) /* Any active shadow file(s)? */
                {
                    // "%1d:%04X CCKD file: merging shadow files..."
                    WRMSG( HHC00321, "I", LCSS_DEVNUM );
                    cckd->sfmerge = merge;
                    cckd->sfforce = force;
                    cckd_sf_remove( dev );
                    n++;
                }
            }
        }
        // "CCKD file number of devices processed: %d"
        WRMSG( HHC00316, "I", n );
        return NULL;
    }

    /* Specific device... */

    if (dev->cckd64)
        return cckd64_sf_remove( data );

    if (!(cckd = dev->cckd_ext))
    {
        // "%1d:%04X CCKD file: device is not a cckd device"
        WRMSG( HHC00317, "W", LCSS_DEVNUM );
        return NULL;
    }

    if (!cckd->sfn)
    {
        // "%1d:%04X CCKD file: device has no shadow files"
        WRMSG( HHC00390, "W", LCSS_DEVNUM );
        return NULL;
    }

    /* Set flags */
    merge = cckd->sfmerge || cckd->sfforce;
    force = cckd->sfforce;
    cckd->sfmerge = cckd->sfforce = 0;

    CCKD_TRACE( "merge starting: %s %s",
                merge ? "merge" : "nomerge", force ? "force" : "");

    /* Schedule updated track entries to be written */
    obtain_lock( &cckd->cckdiolock );
    {
        if (cckd->merging)
        {
            release_lock( &cckd->cckdiolock );
            // "%1d:%04X CCKD file[%d] %s: error merging shadow file, sf command busy on device"
            WRMSG( HHC00322, "W", LCSS_DEVNUM, cckd->sfn, cckd_sf_name(dev, cckd->sfn) );
            return NULL;
        }
        cckd->merging = 1;
        cckd_flush_cache (dev);
        while (cckd->wrpending || cckd->cckdioact)
        {
            cckd->cckdwaiters++;
            wait_condition( &cckd->cckdiocond, &cckd->cckdiolock );
            cckd->cckdwaiters--;
            cckd_flush_cache( dev );
        }
        cckd_purge_cache( dev );
        cckd_purge_l2( dev );
        dev->bufcur = dev->cache = -1;
    }
    release_lock( &cckd->cckdiolock );

    obtain_lock( &cckd->filelock );

    if (!cckd->sfn)
    {
        release_lock( &cckd->filelock );
        // "%1d:%04X CCKD file[%d] %s: cannot remove base file"
        WRMSG( HHC00323, "E", LCSS_DEVNUM, cckd->sfn, cckd_sf_name(dev, cckd->sfn) );
        cckd->merging = 0;
        return NULL;
    }

    from_sfx = cckd->sfn;
    to_sfx   = cckd->sfn - 1;
    fix = cckd->cdevhdr[ to_sfx ].cdh_nullfmt;

    /* Harden the `from' file */
    if (cckd_harden( dev ) < 0)
    {
        // "%1d:%04X CCKD file[%d] %s: shadow file not merged: file[%d] %s%s"
        WRMSG( HHC00324, "E", LCSS_DEVNUM, from_sfx, cckd_sf_name( dev, cckd->sfx ),
               from_sfx, "not hardened", "" );
        goto sf_remove_exit;
    }

    /* Attempt to re-open the `to' file read-write */
    cckd_close( dev, to_sfx );

    if (to_sfx > 0 || !dev->ckdrdonly || force)
        cckd_open( dev, to_sfx, O_RDWR | O_BINARY, 1 );

    if (cckd->fd[to_sfx] < 0)
    {
        /* `from' file can't be opened read-write */
        cckd_open( dev, to_sfx, O_RDONLY | O_BINARY, 0 );

        if (merge)
        {
            // "%1d:%04X CCKD file[%d] %s: shadow file not merged: file[%d] %s%s"
            WRMSG( HHC00324, "E", LCSS_DEVNUM, from_sfx, cckd_sf_name( dev, from_sfx ),
                                 to_sfx, "cannot be opened read-write",
                                 to_sfx == 0 && dev->ckdrdonly && !force ? ", try 'force'" : "" );
            goto sf_remove_exit;
        }
        else
           add = 1;
    }
    else
    {
        /* `from' file opened read-write */
        cckd->sfn = to_sfx;

        if (cckd_chkdsk( dev, 0 ) < 0)
        {
            cckd->sfn = from_sfx;
            // "%1d:%04X CCKD file[%d] %s: shadow file not merged: file[%d] %s%s"
            WRMSG( HHC00324, "E", LCSS_DEVNUM, to_sfx, cckd_sf_name( dev, to_sfx ),
                                 to_sfx, "check failed", "" );
            goto sf_remove_exit;
        }
    }

    cckd->sfn = to_sfx;

    /* Perform backwards merge */
    if (merge)
    {
        CCKD_TRACE( "merging to file[%d]", to_sfx );

        /* Make the target file the active file */
        cckd->sfn = to_sfx;
        cckd->cdevhdr[to_sfx].cdh_opts |= (CCKD_OPT_OPENED | CCKD_OPT_OPENRW);

        /* Loop for each level 1 table entry */
        for (i = 0; i < cckd->cdevhdr[from_sfx].num_L1tab; i++)
        {
            l2updated = 0;
            /* Continue if from L2 doesn't exist */
            if (cckd->L1tab[from_sfx][i] == CCKD_MAXSIZE
             || (cckd->L1tab[from_sfx][i] == 0 && cckd->L1tab[to_sfx][i] == 0))
                continue;

            trk = i*256;

            /* Read `from' l2 table */
            if (cckd->L1tab[from_sfx][i] == 0)
                memset( &from_l2, 0, CCKD_L2TAB_SIZE );
            else
            {
                pos = (off_t)cckd->L1tab[from_sfx][i];
                if (cckd_read( dev, from_sfx, pos, &from_l2, CCKD_L2TAB_SIZE ) < 0)
                    goto sf_merge_error;
            }

            /* Read `to' l2 table */
            if (cckd->L1tab[to_sfx][i] == 0)
                memset( &to_l2, 0, CCKD_L2TAB_SIZE );
            else if (cckd->L1tab[to_sfx][i] == CCKD_MAXSIZE)
                memset (&to_l2, 0xff, CCKD_L2TAB_SIZE);
            else
            {
                pos = (off_t)cckd->L1tab[to_sfx][i];
                if (cckd_read( dev, to_sfx, pos, &to_l2, CCKD_L2TAB_SIZE ) < 0)
                    goto sf_merge_error;
            }

            /* Loop for each level 2 table entry */
            for (j = 0; j < 256; j++)
            {
                trk = i*256 + j;
                /* Continue if from L2 entry doesn't exist */
                if (from_l2[j].L2_trkoff == CCKD_MAXSIZE
                 || (from_l2[j].L2_trkoff == 0 && to_l2[j].L2_trkoff == 0))
                    continue;

                /* Read the `from' track/blkgrp image */
                len = (int)from_l2[j].L2_len;
                if (len > CKD_NULLTRK_FMTMAX)
                {
                    pos = (off_t)from_l2[j].L2_trkoff;
                    if (cckd_read( dev, from_sfx, pos, buf, len ) < 0)
                        goto sf_merge_error;

                    /* Get space for the `to' track/blkgrp image */
                    size = len;
                    if ((pos = cckd_get_space (dev, &size, CCKD_SIZE_EXACT)) < 0)
                        goto sf_merge_error;

                    new_l2.L2_trkoff = (U32)pos;
                    new_l2.L2_len    = (U16)len;
                    new_l2.L2_size   = (U16)size;

                    /* Write the `to' track/blkgrp image */
                    if (cckd_write( dev, to_sfx, pos, buf, len ) < 0)
                        goto sf_merge_error;
                }
                else
                {
                    new_l2.L2_trkoff = 0;
                    new_l2.L2_len = new_l2.L2_size = (U16)len;
                }

                /* Release space occupied by old `to' entry */
                cckd_rel_space( dev, (off_t)to_l2[j].L2_trkoff, (int)to_l2[j].L2_len,
                                                                (int)to_l2[j].L2_size );

                /* Update `to' l2 table entry */
                l2updated = 1;
                to_l2[j].L2_trkoff  = new_l2.L2_trkoff;
                to_l2[j].L2_len  = new_l2.L2_len;
                to_l2[j].L2_size = new_l2.L2_size;
            } /* for each level 2 table entry */

            /* Update the `to' level 2 table */
            if (l2updated)
            {
                l2updated = 0;
                pos = (off_t)cckd->L1tab[to_sfx][i];
                if (memcmp( &to_l2, &empty_l2[fix], CCKD_L2TAB_SIZE ) == 0)
                {
                    cckd_rel_space( dev, pos, CCKD_L2TAB_SIZE, CCKD_L2TAB_SIZE );
                    pos = 0;
                }
                else
                {
                    size = CCKD_L2TAB_SIZE;
                    if (pos == CCKD_NOSIZE || pos == CCKD_MAXSIZE)
                        if ((pos = cckd_get_space( dev, &size, CCKD_L2SPACE )) < 0)
                            goto sf_merge_error;
                    if (cckd_write( dev, to_sfx, pos, &to_l2, CCKD_L2TAB_SIZE ) < 0)
                        goto sf_merge_error;
                } /* `to' level 2 table not null */

                /* Update the level 1 table index */
                cckd->L1tab[to_sfx][i] = (U32)pos;

                /* Flush free space */
                cckd_flush_space( dev );

            } /* Update level 2 table */

        } /* For each level 1 table entry */

        /* Validate the merge */
        cckd_harden( dev );
        cckd_chkdsk( dev, 0 );
        cckd_read_init( dev );

    } /* if merge */

    /* Remove the old file */
    cckd_close( dev, from_sfx );
    cckd->L1tab[from_sfx] = cckd_free( dev, "l1", cckd->L1tab[ from_sfx ]);
    memset( &cckd->cdevhdr[from_sfx], 0, CCKD_DEVHDR_SIZE );
    rc = unlink (cckd_sf_name (dev, from_sfx));

    /* adjust the stats */
    cckd->reads   [to_sfx] += cckd->reads   [from_sfx];
    cckd->writes  [to_sfx] += cckd->writes  [from_sfx];
    cckd->L2_reads[to_sfx] += cckd->L2_reads[from_sfx];

    cckd->reads   [from_sfx] = 0;
    cckd->writes  [from_sfx] = 0;
    cckd->L2_reads[from_sfx] = 0;

    /* Add the file back if necessary */
    if (add) rc = cckd_sf_new( dev ) ;

    // "%1d:%04X CCKD file[%d] %s: shadow file successfully %s"
    WRMSG( HHC00325, "I", LCSS_DEVNUM, from_sfx, cckd_sf_name( dev, from_sfx ),
                   merge ? "merged" : add ? "re-added" : "removed" );

sf_remove_exit:

    /* Re-read the l1 to set L2_bounds, L2ok */
    cckd_read_l1( dev );

    release_lock( &cckd->filelock );

    obtain_lock( &cckd->cckdiolock );
    {
        cckd_purge_cache( dev );
        cckd_purge_l2( dev );
        dev->bufcur = dev->cache = -1;
        cckd->merging = 0;
        if (cckd->cckdwaiters)
            broadcast_condition( &cckd->cckdiocond );
        CCKD_TRACE( "merge complete%s", "" );
    }
    release_lock( &cckd->cckdiolock );

    cckd_sf_stats( dev );
    return NULL;

sf_merge_error:

    if (trk < 0)
        // "%1d:%04X CCKD file[%d] %s: shadow file not merged, error during merge"
        WRMSG( HHC00326, "E", LCSS_DEVNUM, from_sfx, cckd_sf_name( dev, from_sfx ));
    else
        // "%1d:%04X CCKD file[%d] %s: shadow file not merged, error processing trk(%d)"
        WRMSG( HHC00327, "E", LCSS_DEVNUM, from_sfx, cckd_sf_name( dev, from_sfx ), trk );

    if (l2updated && cckd->L1tab[ to_sfx ][i] && cckd->L1tab[ to_sfx ][i] != CCKD_SHADOW_NO_OFFSET)
    {
        l2updated = 0;
        pos = (off_t)cckd->L1tab[to_sfx][i];
        cckd_write( dev, to_sfx, pos, &to_l2, CCKD_L2TAB_SIZE );
    }
    cckd_harden( dev );
    cckd_chkdsk( dev, 2 );
    cckd->sfn = from_sfx;
    cckd_harden( dev );
    cckd_chkdsk( dev, 2 );
    goto sf_remove_exit;

} /* end function cckd_sf_remove */

/*-------------------------------------------------------------------*/
/* Check and compress a shadow file  (sfc)                           */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* cckd_sf_comp( void* data )
{
DEVBLK         *dev = data;             /* -> DEVBLK                 */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */

    /* NULL DEVBLK == all devices? */
    if (!dev)
    {
    int n = 0;
        for (dev=sysblk.firstdev; dev; dev=dev->nextdev)
        {
            if (dev->cckd_ext)  /* Is this a compressed device? */
            {
                // "%1d:%04X CCKD file: compressing shadow files..."
                WRMSG( HHC00328, "I", LCSS_DEVNUM );
                cckd_sf_comp( dev );
                n++;
            }
        }
        // "CCKD file number of devices processed: %d"
        WRMSG ( HHC00316, "I", n );
        return NULL;
    }

    if (dev->cckd64)
        return cckd64_sf_comp( data );

    if (!(cckd = dev->cckd_ext))
    {
        // "%1d:%04X CCKD file: device is not a cckd device"
        WRMSG( HHC00317, "W",  LCSS_DEVNUM );
        return NULL;
    }

    /* schedule updated track entries to be written */
    obtain_lock( &cckd->cckdiolock );
    {
        if (cckd->merging)
        {
            release_lock( &cckd->cckdiolock );
            // "%1d:%04X CCKD file[%d] %s: error compressing shadow file, sf command busy on device"
            WRMSG( HHC00329, "W",  LCSS_DEVNUM, cckd->sfn, cckd_sf_name(dev, cckd->sfn) );
            return NULL;
        }
        cckd->merging = 1;
        cckd_flush_cache( dev );
        while (cckd->wrpending || cckd->cckdioact)
        {
            cckd->cckdwaiters++;
            wait_condition( &cckd->cckdiocond, &cckd->cckdiolock );
            cckd->cckdwaiters--;
            cckd_flush_cache( dev );
        }
        cckd_purge_cache( dev );
        cckd_purge_l2( dev );
        dev->bufcur = dev->cache = -1;
    }
    release_lock( &cckd->cckdiolock );

    /* obtain control of the file */
    obtain_lock( &cckd->filelock );
    {
        /* harden the current file */
        cckd_harden( dev );

        /* Call the compress function */
        rc = cckd_comp( dev );

        /* Perform initial read */
        rc = cckd_read_init( dev );
    }
    release_lock( &cckd->filelock );

    obtain_lock( &cckd->cckdiolock );
    {
        cckd->merging = 0;
        if (cckd->cckdwaiters)
            broadcast_condition( &cckd->cckdiocond );
    }
    release_lock( &cckd->cckdiolock );

    /* Display the shadow file statistics */
    cckd_sf_stats( dev );

    return NULL;
} /* end function cckd_sf_comp */

/*-------------------------------------------------------------------*/
/* Check a shadow file  (sfk)                                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* cckd_sf_chk( void* data )
{
DEVBLK         *dev = data;             /* -> DEVBLK                 */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             rc;                     /* Return code               */
int             level = 2;              /* Check level               */

    /* NULL DEVBLK == all devices? */
    if (!dev)
    {
    int n = 0;
        level = cckdblk.sflevel;
        cckdblk.sflevel = 0;
        for (dev=sysblk.firstdev; dev; dev=dev->nextdev)
        {
            if ((cckd = dev->cckd_ext)) /* Compressed device? */
            {
                // "%1d:%04X CCKD file: checking level %d..."
                WRMSG( HHC00330, "I", LCSS_DEVNUM, level );
                cckd->sflevel = level;
                cckd_sf_chk( dev );
                n++;
            }
        }
        // "CCKD file number of devices processed: %d"
        WRMSG( HHC00316, "I", n );
        return NULL;
    }

    if (dev->cckd64)
        return cckd64_sf_chk( data );

    if (!(cckd = dev->cckd_ext))
    {
        // "%1d:%04X CCKD file: device is not a cckd device"
        WRMSG( HHC00317, "W",  LCSS_DEVNUM );
        return NULL;
    }

    level = cckd->sflevel;
    cckd->sflevel = 0;

    /* schedule updated track entries to be written */
    obtain_lock( &cckd->cckdiolock );
    {
        if (cckd->merging)
        {
            release_lock( &cckd->cckdiolock );
            // "%1d:%04X CCKD file[%d] %s: shadow file check failed, sf command busy on device"
            WRMSG( HHC00331, "W", LCSS_DEVNUM, cckd->sfn, cckd_sf_name( dev, cckd->sfn ));
            return NULL;
        }

        cckd->merging = 1;
        cckd_flush_cache( dev );

        while (cckd->wrpending || cckd->cckdioact)
        {
            cckd->cckdwaiters++;
            {
                wait_condition( &cckd->cckdiocond, &cckd->cckdiolock );
            }
            cckd->cckdwaiters--;
            cckd_flush_cache( dev );
        }
        cckd_purge_cache( dev );
        cckd_purge_l2( dev );
        dev->bufcur = -1;
        dev->cache  = -1;
    }
    release_lock( &cckd->cckdiolock );

    /* obtain control of the file */
    obtain_lock( &cckd->filelock );
    {
        /* harden the current file */
        cckd_harden( dev );

        /* Call the chkdsk function */
        rc = cckd_chkdsk( dev, level );

        /* Perform initial read */
        rc = cckd_read_init( dev );
    }
    release_lock( &cckd->filelock );

    obtain_lock( &cckd->cckdiolock );
    {
        cckd->merging = 0;
        if (cckd->cckdwaiters)
            broadcast_condition( &cckd->cckdiocond );
    }
    release_lock( &cckd->cckdiolock );

    /* Display the shadow file statistics */
    cckd_sf_stats( dev );

    return NULL;
} /* end function cckd_sf_chk */

/*-------------------------------------------------------------------*/
/* Display shadow file statistics   (sfd)                            */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* cckd_sf_stats( void* data )
{
DEVBLK         *dev = data;             /* -> DEVBLK                 */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
struct stat     st = {0};               /* File information          */
int             i;                      /* Index                     */
char           *ost[] = {"  ", "ro", "rd", "rw"};
U64             usize=0,ufree=0;        /* Total size, free space    */
int             free_count=0;           /* Total number free spaces  */

    /* NULL DEVBLK == all devices? */
    if (!dev)
    {
    int n = 0;
        for (dev=sysblk.firstdev; dev; dev=dev->nextdev)
        {
            if (dev->cckd_ext)  /* Is this a compressed device? */
            {
                // "%1d:%04X CCKD file: display cckd statistics"
                WRMSG( HHC00332, "I", LCSS_DEVNUM );
                cckd_sf_stats( dev );
                n++;
            }
        }
        // "CCKD file number of devices processed: %d"
        WRMSG( HHC00316, "I", n );
        return NULL;
    }

    if (dev->cckd64)
        return cckd64_sf_stats( data );

    if (!(cckd = dev->cckd_ext))
    {
        // "%1d:%04X CCKD file: device is not a cckd device"
        WRMSG( HHC00317, "W", LCSS_DEVNUM );
        return NULL;
    }

    /* Calculate totals */
    if (fstat( cckd->fd[0], &st ) != 0)
    {
        // "Error in function %s: %s"
        WRMSG( HHC00075, "E", "fstat()", strerror( errno ));
        memset( &st, 0, sizeof( st ));
    }
    for (i=0; i <= cckd->sfn; i++)
    {
        if (!i) usize = st.st_size;
        else usize += cckd->cdevhdr[i].cdh_size;
        ufree += cckd->cdevhdr[i].free_total;
        free_count += cckd->cdevhdr[i].free_num;
    }

    /* header */

    // "%1d:%04X   32/64       size free  nbr st   reads  writes l2reads    hits switches"
    WRMSG (HHC00333, "I", LCSS_DEVNUM);

    if (cckd->readaheads || cckd->misses)
    // "%1d:%04X                                                      readaheads   misses"
    WRMSG (HHC00334, "I", LCSS_DEVNUM);

    // "%1d:%04X ------------------------------------------------------------------------"
    WRMSG (HHC00335, "I", LCSS_DEVNUM);

    /* total statistics */

    // "%1d:%04X [*] %s %11.11"PRId64" %3.3"PRId64"%% %4.4"PRId64"    %7.7d %7.7d %7.7d %7.7d  %7.7d"
    WRMSG( HHC00336, "I", LCSS_DEVNUM, dev->cckd64 ? "64" : "32", usize

        , usize ? (ufree * 100) / usize : 999
        , (S64)free_count
        , cckd->totreads
        , cckd->totwrites
        , cckd->totl2reads
        , cckd->cachehits
        , cckd->switches
    );

    if (cckd->readaheads || cckd->misses)
    // "%1d:%04X                                                         %7.7d  %7.7d"
    WRMSG (HHC00337, "I", LCSS_DEVNUM,
            cckd->readaheads, cckd->misses);

    /* base file statistics */

    // "%1d:%04X %s"
    WRMSG (HHC00338, "I", LCSS_DEVNUM, dev->filename);

    // "%1d:%04X [0] %s %11.11"PRId64" %3.3"PRId64"%% %4.4"PRId64" %s %7.7d %7.7d %7.7d"
    WRMSG( HHC00339, "I", LCSS_DEVNUM, dev->cckd64 ? "64" : "32", st.st_size

        , st.st_size ?
          ((S64)(cckd->cdevhdr[0].free_total * 100) / st.st_size) : 999
        ,  (S64) cckd->cdevhdr[0].free_num
        , ost[cckd->open[0]]
        , cckd->reads[0]
        , cckd->writes[0]
        , cckd->L2_reads[0]
    );

    if (dev->dasdsfn != NULL && CCKD_MAX_SF > 0)
        // "%1d:%04X %s"
        WRMSG (HHC00340, "I", LCSS_DEVNUM, cckd_sf_name(dev, -1));

    /* shadow file statistics */
    for (i = 1; i <= cckd->sfn; i++)
    {
        // "%1d:%04X [%d] %s %11.11"PRId64" %3.3"PRId64"%% %4.4"PRId64" %s %7.7d %7.7d %7.7d"
        WRMSG( HHC00341, "I", LCSS_DEVNUM, i, dev->cckd64 ? "64" : "32", (S64)cckd->cdevhdr[i].cdh_size

            , cckd->cdevhdr[i].cdh_size ?
              ((S64)(cckd->cdevhdr[i].free_total * 100) / cckd->cdevhdr[i].cdh_size) : 999
            ,  (S64) cckd->cdevhdr[i].free_num
            , ost[cckd->open[i]]
            , cckd->reads[i]
            , cckd->writes[i]
            , cckd->L2_reads[i]
        );
    }

    return NULL;
} /* end function cckd_sf_stats */

/*-------------------------------------------------------------------*/
/* Lock CCKD device chain                                            */
/*-------------------------------------------------------------------*/
void cckd_lock_devchain( int flag )
{
    obtain_lock( &cckdblk.devlock );
    {
        while (( flag && cckdblk.devusers != 0)
            || (!flag && cckdblk.devusers <  0))
        {
            cckdblk.devwaiters++;
            {
#if FALSE
                {
                    struct timespec  tm;
                    struct timeval   now;
                    int              timeout;

                    gettimeofday( &now, NULL );
                    tm.tv_sec = now.tv_sec + 2;
                    tm.tv_nsec = now.tv_usec * 1000;
                    timeout = timed_wait_condition( &cckdblk.devcond, &cckdblk.devlock, &tm );
                    if (timeout) cckd_print_itrace();
                }
#else
                wait_condition( &cckdblk.devcond, &cckdblk.devlock );
#endif
            }
            cckdblk.devwaiters--;
        }

        if (flag) cckdblk.devusers--;
        else      cckdblk.devusers++;
    }
    release_lock( &cckdblk.devlock );
}

/*-------------------------------------------------------------------*/
/* Unlock CCKD device chain                                          */
/*-------------------------------------------------------------------*/
void cckd_unlock_devchain()
{
    obtain_lock( &cckdblk.devlock );
    {
        if (cckdblk.devusers < 0) cckdblk.devusers++;
        else                      cckdblk.devusers--;

        if (!cckdblk.devusers && cckdblk.devwaiters)
            signal_condition( &cckdblk.devcond );
    }
    release_lock( &cckdblk.devlock );
}

/*-------------------------------------------------------------------*/
/* Start garbage collector                                           */
/*-------------------------------------------------------------------*/
void cckd_gcstart()
{
    DEVBLK*    dev;
    CCKD_EXT*  cckd;
    TID tid;
    int flag = 0;
    int rc;

    cckd_lock_devchain(0);
    {
        for (dev = cckdblk.dev1st; dev; dev = cckd->devnext)
        {
            cckd = dev->cckd_ext;

            if (dev->cckd64)
                continue;

            obtain_lock( &cckd->filelock );
            {
                if (cckd->cdevhdr[ cckd->sfn ].free_total)
                {
                    cckd->cdevhdr[ cckd->sfn ].cdh_opts |= (CCKD_OPT_OPENED | CCKD_OPT_OPENRW);
                    cckd_write_chdr( dev );
                    flag = 1;
                }
            }
            release_lock( &cckd->filelock );
        }
    }
    cckd_unlock_devchain();

    /* Schedule the garbage collector */
    if (flag && cckdblk.gcs < cckdblk.gcmax)
    {
        /* ensure read integrity for gc count */
        obtain_lock( &cckdblk.gclock );
        {
            if (cckdblk.gcs < cckdblk.gcmax)
            {
                /* Schedule a new garbage collector thread  */
                if (!cckdblk.batch || cckdblk.batchml > 1)
                    // "Starting thread %s, active=%d, started=%d, max=%d"
                    WRMSG( HHC00107, "I", CCKD_GC_THREAD_NAME "() by command line",
                        cckdblk.gca, cckdblk.gcs, cckdblk.gcmax );

                ++cckdblk.gcs;

                /* Release lock across thread create to prevent interlock  */
                release_lock( &cckdblk.gclock );
                {
                    rc = create_thread( &tid, JOINABLE, cckd_gcol, NULL, CCKD_GC_THREAD_NAME );
                }
                obtain_lock( &cckdblk.gclock );

                if (rc)
                {
                    // "Error in function create_thread() for %s %d of %d: %s"
                    WRMSG( HHC00106, "E", CCKD_GC_THREAD_NAME "() by command line",
                        cckdblk.gcs-1, cckdblk.gcmax, strerror( rc ));

                    --cckdblk.gcs;
                }
            }
        }
        release_lock( &cckdblk.gclock );
    }
}

/*-------------------------------------------------------------------*/
/* Garbage Collection thread                                         */
/*                                                                   */
/*  While provision is made in the code that initiates cckd_gcol     */
/*  for execution of more than one concurrent Garbage Collector,     */
/*  the thread runs with mutex cckdblk.gclock held at all times      */
/*  and is therefore limited to one concurrent thread.  We'll leave  */
/*  the initiation code as-is in anticipation of the day when we     */
/*  might get to a max of one collector per compressed device.       */
/*                                                                   */
/*-------------------------------------------------------------------*/
void* cckd_gcol(void* arg)
{
int             gcol;                   /* Identifier                */
DEVBLK         *dev;                    /* -> device block           */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
struct timeval  tv_now;                 /* Time-of-day (as timeval)  */
time_t          tt_now;                 /* Time-of-day (as time_t)   */
struct timespec tm;                     /* Time-of-day to wait       */
int             gcs;

    UNREFERENCED( arg );

    gettimeofday (&tv_now, NULL);

    obtain_lock (&cckdblk.gclock);

    gcol = ++cckdblk.gca;

    /* Return without messages if too many already started */
    if (gcol > cckdblk.gcmax)
    {
        --cckdblk.gcs;
        --cckdblk.gca;

        if (!cckdblk.gcs)
        {
            if (!cckdblk.batch || cckdblk.batchml > 1)
                // "Thread id "TIDPAT", prio %d, name '%s' ended"
                LOG_THREAD_END( CCKD_GC_THREAD_NAME  );
        }
        else
            if (!cckdblk.batch || cckdblk.batchml > 0)
                // "Ending thread "TIDPAT" %s, pri=%d, started=%d, max=%d exceeded"
                WRMSG( HHC00108, "W", TID_CAST( thread_id()), CCKD_GC_THREAD_NAME,
                    get_thread_priority(), cckdblk.gcs, cckdblk.gcmax );

        release_lock( &cckdblk.gclock );
        signal_condition( &cckdblk.termcond );  /* signal if last gcol thread ending before init. */
        return NULL;        /* back to the shadows again  */
    }

    if (!cckdblk.batch || cckdblk.batchml > 1)
        // "Thread id "TIDPAT", prio %d, name '%s' started"
        LOG_THREAD_BEGIN( CCKD_GC_THREAD_NAME  );

    while (gcol <= cckdblk.gcmax)
    {
        // "Begin CCKD garbage collection"
        if (cckdblk.gcmsgs)
            WRMSG( HHC00382, "I" );

        /* Perform collection on each device */
        cckd_lock_devchain(0);
        {
            for (dev = cckdblk.dev1st; dev; dev = cckd->devnext)
            {
                cckd = dev->cckd_ext;
                cckd_gcol_dev( dev, &tv_now );
            }
        }
        cckd_unlock_devchain();

        // "End CCKD garbage collection"
        if (cckdblk.gcmsgs)
            WRMSG( HHC00383, "I" );

        /* If we're in manual on-demand mode, then we're done. */
        if (cckdblk.gcint <= 0)
            break;

        /* Otherwise, wait a bit before starting the next cycle */

        // Get the time of day again for cckd_gcol_dev's file sync check
        gettimeofday (&tv_now, NULL);
        tt_now = tv_now.tv_sec + ((tv_now.tv_usec + 500000)/1000000);
        CCKD_TRACE( CCKD_GC_THREAD_NAME " wait %d seconds at %s",
                    cckdblk.gcint, ctime (&tt_now));

        tm.tv_sec = tv_now.tv_sec + cckdblk.gcint;
        tm.tv_nsec = tv_now.tv_usec * 1000;
        timed_wait_condition( &cckdblk.gccond, &cckdblk.gclock, &tm );
    }

    if (!cckdblk.batch || cckdblk.batchml > 1)
        // "Thread id "TIDPAT", prio %d, name '%s' ended"
        LOG_THREAD_END( CCKD_GC_THREAD_NAME  );

    cckdblk.gcs--;
    cckdblk.gca--;

    gcs = cckdblk.gcs;

    release_lock( &cckdblk.gclock );

    if (!gcs)
        signal_condition( &cckdblk.termcond );

    return NULL;
} /* end thread cckd_gcol */

/*-------------------------------------------------------------------*/
/* Report compression ratios for all CCKD devices                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd_gc_rpt_states()
{
    if (cckdblk.gcmsgs)     // (only if they're interested)
    {
        CCKD_EXT*  cckd;                /* -> cckd extension         */
        DEVBLK*    dev;                 /* -> device                 */

        obtain_lock( &cckdblk.devlock );
        {
            for (dev = cckdblk.dev1st; dev; dev = cckd->devnext)
            {
                cckd = dev->cckd_ext;
                cckd_gc_rpt_state( dev );
            }
        }
        release_lock( &cckdblk.devlock );
    }
}

/*-------------------------------------------------------------------*/
/* Report Garbage Collection state for a given CCKD device           */
/*-------------------------------------------------------------------*/
void cckd_gc_rpt_state( DEVBLK* dev )
{
    CCKD_EXT*  cckd;
    int        gc;

    if (dev->cckd64)
    {
        cckd64_gc_rpt_state( dev );
        return;
    }

    cckd = dev->cckd_ext;

    /* Retrieve and report garbage collector state, but ONLY if
       the image is over 100MB in size. This prevents "scaring"
       the user about SEVERELY fragmented files when the file
       is too small to be much of a concern, as is usually the
       case with e.g. shadow files.
    */
    if (cckd->cdevhdr->cdh_size < (100 * _1M))
        return;

    gc = cckd_gc_state( dev );

    switch (gc)
    {
        case 0:     // critical!
        case 1:     // severe

            // "%1d:%04X CCKD%s image %s is SEVERELY fragmented!"
            WRMSG( HHC00387, "W", LCSS_DEVNUM, "",
                TRIMLOC( cckd_sf_name( dev, cckd->sfn )));
            break;

        case 2:     // moderate

            // "%1d:%04X CCKD%s image %s is moderately fragmented"
            WRMSG( HHC00388, "W", LCSS_DEVNUM, "",
                TRIMLOC( cckd_sf_name( dev, cckd->sfn )));
            break;

        case 3:     // light

            // "%1d:%04X CCKD%s image %s is slightly fragmented"
            WRMSG( HHC00389, "I", LCSS_DEVNUM, "",
                TRIMLOC( cckd_sf_name( dev, cckd->sfn )));
            break;

        default:    // less than light

            break;  // (don't bother reporting it)
    }
}

/*-------------------------------------------------------------------*/
/* Return Garbage Collection State for a given CCKD device           */
/*-------------------------------------------------------------------*/
int cckd_gc_state( DEVBLK* dev )
{
    CCKD_EXT*  cckd;                    /* -> cckd extension         */
    S64        size, fsiz;              /* File size, free size      */
    int        gc;                      /* Garbage collection state  */

    if (dev->cckd64)
        return cckd64_gc_state( dev );

    cckd = dev->cckd_ext;

    /* Determine garbage state */
    size = (S64) cckd->cdevhdr[ cckd->sfn ].cdh_size;
    fsiz = (S64) cckd->cdevhdr[ cckd->sfn ].free_total;

    if      (fsiz >= (size = size/2)) gc = 0; // critical   50% - 100%
    else if (fsiz >= (size = size/2)) gc = 1; // severe     25% - 50%
    else if (fsiz >= (size = size/2)) gc = 2; // moderate 12.5% - 25%
    else if (fsiz >= (size = size/2)) gc = 3; // light     6.3% - 12.5%
    else                              gc = 4; // none        0% - 6.3%

    /* Adjust the state based on the number of free spaces */
    if (cckd->cdevhdr[ cckd->sfn ].free_num >  800 && gc > 0) gc--;
    if (cckd->cdevhdr[ cckd->sfn ].free_num > 1800 && gc > 0) gc--;
    if (cckd->cdevhdr[ cckd->sfn ].free_num > 3000)           gc = 0;

    return gc;
}

/*-------------------------------------------------------------------*/
/* Perform garbage collection for a given CCKD device                */
/*-------------------------------------------------------------------*/
void cckd_gcol_dev( DEVBLK* dev, struct timeval* tv_now )
{
int             rc;                     /* Return code               */
CCKD_EXT       *cckd;                   /* -> cckd extension         */
U64             size;                   /* Percolate size            */
int             gc;                     /* Garbage collection state  */

    if (dev->cckd64)
    {
        cckd64_gcol_dev( dev, tv_now );
        return;
    }

    cckd = dev->cckd_ext;

    obtain_lock (&cckd->cckdiolock);
    {
        /* Bypass if merging or stopping */
        if (cckd->merging || cckd->stopping)
        {
            release_lock (&cckd->cckdiolock);
            return;
        }

        /* Bypass if not opened read-write */
        if (cckd->open[cckd->sfn] != CCKD_OPEN_RW)
        {
            release_lock (&cckd->cckdiolock);
            return;
        }

        /* Free newbuf if it hasn't been used */
        if (!cckd->cckdioact && !cckd->bufused && cckd->newbuf)
            cckd->newbuf = cckd_free (dev, "newbuf", cckd->newbuf);

        cckd->bufused = 0;

        /* If OPENED bit not on then flush if updated */
        if (!(cckd->cdevhdr[cckd->sfn].cdh_opts & CCKD_OPT_OPENED))
        {
            if (cckd->updated) cckd_flush_cache (dev);
            release_lock (&cckd->cckdiolock);
            return;
        }

        /* Determine garbage state */
        gc = cckd_gc_state( dev );

        /* Set the size */
        if (cckdblk.gcparm > 0) size = gctab[gc] << cckdblk.gcparm;
        else if (cckdblk.gcparm < 0) size = gctab[gc] >> abs(cckdblk.gcparm);
        else size = gctab[gc];

        if (size > cckd->cdevhdr[cckd->sfn].cdh_used >> SHIFT_1K)
            size = cckd->cdevhdr[cckd->sfn].cdh_used >> SHIFT_1K;
        if (size < 64)
            size = 64;
    }
    release_lock (&cckd->cckdiolock);

    /* Call the garbage collector */
    cckd_gc_percolate( dev, size );

    /* Schedule any updated tracks to be written */
    obtain_lock (&cckd->cckdiolock);
    {
        cckd_flush_cache (dev);

        while (cckdblk.fsync && cckd->wrpending)
        {
            cckd->cckdwaiters++;
            {
                wait_condition (&cckd->cckdiocond, &cckd->cckdiolock);
            }
            cckd->cckdwaiters--;
        }
    }
    release_lock (&cckd->cckdiolock);

    /* Sync the file */
    if (cckdblk.fsync && cckd->lastsync + 10 <= tv_now->tv_sec)
    {
        obtain_lock( &cckd->filelock );
        {
            rc = fdatasync (cckd->fd[cckd->sfn]);
            cckd->lastsync = tv_now->tv_sec;
        }
        release_lock( &cckd->filelock );
    }

    /* Flush the free space */
    if (cckd->cdevhdr[cckd->sfn].free_num)
    {
        obtain_lock( &cckd->filelock );
        {
            cckd_flush_space (dev);
        }
        release_lock( &cckd->filelock );
    }
}

/*-------------------------------------------------------------------*/
/* Garbage Collection error handler helper functions/macros          */
/*-------------------------------------------------------------------*/
static int cckd_gc_perc_error( DEVBLK* dev, CCKD_EXT* cckd, unsigned int moved,
                               const char* file, int line )
{
    cckd_trace( file, line, dev, "gcperc exiting due to error, moved %u", moved );
    release_lock( &cckd->filelock );
    return moved;
}
#define GC_PERC_ERROR()     cckd_gc_perc_error( dev, cckd, moved, __FILE__, __LINE__ )

static int cckd_gc_perc_space_error( DEVBLK* dev, CCKD_EXT* cckd, off_t upos, int i, BYTE* buf, int moved,
                                     const char* file, int line )
{
    // "%1d:%04X CCKD file[%d] %s: %s(%d): offset 0x%16.16"PRIx64" unknown space %2.2x%2.2x%2.2x%2.2x%2.2x"

    WRMSG( HHC00342, "E", LCSS_DEVNUM,
            cckd->sfn, cckd_sf_name( dev, cckd->sfn ), TRIMLOC( file ), line,
            upos + i, buf[i], buf[i+1],buf[i+2], buf[i+3], buf[i+4]);

    cckd->cdevhdr[ cckd->sfn ].cdh_opts |= CCKD_OPT_SPERRS;
    cckd_print_itrace();
    return cckd_gc_perc_error( dev, cckd, moved, file, line );
}
#define GC_PERC_SPACE_ERROR()   cckd_gc_perc_space_error( dev, cckd, upos, i, buf, moved, __FILE__, __LINE__)

/*-------------------------------------------------------------------*/
/* Garbage Collection -- Percolate algorithm                         */
/*-------------------------------------------------------------------*/
int cckd_gc_percolate( DEVBLK* dev, U64 size )
{
CCKD_EXT       *cckd;                   /* -> cckd extension         */
bool            didmsg = false;         /* HHC00384 issued           */
int             rc;                     /* Return code               */
unsigned int    moved = 0;              /* Space moved               */
int             after = 0, a;           /* New space after old       */
int             sfx;                    /* File index                */
int             i, j, k;                /* Indexes                   */
int             flags;                  /* Write trkimg flags        */
off_t           fpos, upos;             /* File offsets              */
unsigned int    flen, ulen, len;        /* Lengths                   */
int             trk;                    /* Track number              */
int             L1idx, l2x;             /* Table Indexes             */
CCKD_L2ENT      l2;                     /* Copied level 2 entry      */
BYTE            buf[256*1024];          /* Buffer                    */

    if (dev->cckd64)
        return cckd64_gc_percolate( dev, size );

    cckd = dev->cckd_ext;
    size = size << SHIFT_1K;

    if (cckd->cdevhdr[ cckd->sfn ].cdh_opts & CCKD_OPT_SPERRS)
    {
        // "Skipping garbage collection for CCKD%s file[%d] %1d:%04X %s due to space errors"
        if (cckdblk.gcmsgs)
            WRMSG( HHC00385, "I", "", cckd->sfn, LCSS_DEVNUM, cckd_sf_name( dev, cckd->sfn ));
        return moved;
    }

    /* Debug */
    OBTAIN_TRACE_LOCK();
    if (cckdblk.itracen)
    {
        RELEASE_TRACE_LOCK();

        CCKD_TRACE( "gcperc size %d 1st 0x%x nbr %d largest %u",
                    size, cckd->cdevhdr[cckd->sfn].free_off,
                    cckd->cdevhdr[cckd->sfn].free_num,
                    cckd->cdevhdr[cckd->sfn].free_largest );

        fpos = (off_t) cckd->cdevhdr[cckd->sfn].free_off;

        for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
        {
            CCKD_TRACE( "gcperc free[%4d]:%8.8x end %8.8x len %10d%cpend %d",
                        i,(int)fpos,(int)(fpos+cckd->ifb[i].ifb_len),(int)cckd->ifb[i].ifb_len,
                        fpos+(int)cckd->ifb[i].ifb_len == (int)cckd->ifb[i].ifb_offnxt ?
                                '*' : ' ',cckd->ifb[i].ifb_pending );

            fpos = cckd->ifb[i].ifb_offnxt;
        }
    }
    else
        RELEASE_TRACE_LOCK();

    if (!cckd->L2ok)
        cckd_gc_l2(dev, buf);

    /* garbage collection cycle... */
    while (moved < size && after < 4)
    {
        obtain_lock (&cckd->filelock);
        sfx = cckd->sfn;

        /* Exit if no more free space */
        if (cckd->cdevhdr[sfx].free_total == 0)
        {
            release_lock( &cckd->filelock );
            return moved;
        }

        /* Make sure the free space chain is built */
        if (!cckd->ifb) cckd_read_fsp (dev);

        /* Find a space to start with */
        k = -1;
        upos = ulen = flen = 0;
        fpos = cckd->cdevhdr[sfx].free_off;

        /* First non-pending free space */
        for (i = cckd->free_idx1st; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
        {
            if (!cckd->ifb[i].ifb_pending)
            {
                flen += cckd->ifb[i].ifb_len;
                break;
            }
            fpos = cckd->ifb[i].ifb_offnxt;
        }

        /* Continue to largest if non-zero `after' */
        for ( ; i >= 0 && after; i = cckd->ifb[i].ifb_idxnxt)
        {
            k = i;
            if (!cckd->ifb[i].ifb_pending) flen += cckd->ifb[i].ifb_len;
            if (cckd->ifb[i].ifb_len == cckd->cdevhdr[sfx].free_largest)
                break;
            fpos = cckd->ifb[i].ifb_offnxt;
        }

        /* Skip following free spaces */
        for ( ; i >= 0; i = cckd->ifb[i].ifb_idxnxt)
        {
            if (!cckd->ifb[i].ifb_pending) flen += cckd->ifb[i].ifb_len;
            if (fpos + cckd->ifb[i].ifb_len != cckd->ifb[i].ifb_offnxt) break;
            fpos = cckd->ifb[i].ifb_offnxt;
        }

        /* Space preceding largest if largest is at the end */
        if (i < 0 && k >= 0)
        {
            if (!cckd->ifb[k].ifb_pending) flen -= cckd->ifb[i].ifb_len;
            for (i = cckd->ifb[k].ifb_idxprv; i >= 0; i = cckd->ifb[i].ifb_idxprv)
            {
                fpos = cckd->ifb[i].ifb_idxprv >= 0
                     ? cckd->ifb[cckd->ifb[i].ifb_idxprv].ifb_offnxt
                     : cckd->cdevhdr[sfx].free_off;
                if (fpos + cckd->ifb[i].ifb_len < cckd->ifb[i].ifb_offnxt) break;
                if (!cckd->ifb[i].ifb_pending) flen -= cckd->ifb[i].ifb_len;
            }
        }

        /* Calculate the offset/length of the used space.
         * If only imbedded free space is left, then start
         * with the first used space that is not an l2 table.
         */
        if (i >= 0)
        {
            upos = fpos + cckd->ifb[i].ifb_len;
            ulen = (unsigned int)((cckd->ifb[i].ifb_offnxt ? cckd->ifb[i].ifb_offnxt : cckd->cdevhdr[sfx].cdh_size) - upos);
        }
        else if (!cckd->cdevhdr[sfx].free_num && cckd->cdevhdr[sfx].free_imbed)
        {
            upos = (off_t)(CCKD_L1TAB_POS + cckd->cdevhdr[sfx].num_L1tab * CCKD_L1ENT_SIZE);
            while (1)
            {
                for (i = 0; i < cckd->cdevhdr[sfx].num_L1tab; i++)
                    if (cckd->L1tab[sfx][i] == (U32)upos)
                       break;
                if (i >= cckd->cdevhdr[sfx].num_L1tab)
                    break;
                upos += CCKD_L2TAB_SIZE;
            }
            ulen = (unsigned int)(cckd->cdevhdr[sfx].cdh_size - upos);
        }

        /* Return if no applicable used space */
        if (ulen == 0)
        {
            CCKD_TRACE( "gcperc no applicable space, moved %u", moved);
            release_lock( &cckd->filelock );
            return moved;
        }

        /* Reduce ulen size to minimize `after' relocations */
        if (ulen > flen + 65536) ulen = flen + 65536;
        if (ulen > sizeof(buf))  ulen = sizeof(buf);

        CCKD_TRACE( "gcperc selected space 0x%16.16"PRIx64" len %d", upos, ulen);

        if (cckd_read (dev, sfx, upos, buf, ulen) < 0)
            return GC_PERC_ERROR();

        /* Process each space in the buffer */
        flags = cckd->cdevhdr[sfx].free_num < 100 ? CCKD_SIZE_EXACT : CCKD_SIZE_ANY;
        for (i = a = 0; i + CKD_TRKHDR_SIZE <= (int)ulen; i += len)
        {
            /* Check for level 2 table */
            for (j = 0; j < cckd->cdevhdr[sfx].num_L1tab; j++)
                if (cckd->L1tab[sfx][j] == (U32)(upos + i)) break;

            if (!didmsg)
            {
                // "Collecting garbage for CCKD%s file[%d] %1d:%04X %s..."
                if (cckdblk.gcmsgs)
                    WRMSG( HHC00384, "I", "", cckd->sfn, LCSS_DEVNUM, cckd_sf_name( dev, cckd->sfn ));
                didmsg = true;
            }

            if (j < cckd->cdevhdr[sfx].num_L1tab)
            {
                /* Moving a level 2 table */
                len = CCKD_L2TAB_SIZE;
                if (i + len > ulen) break;
                CCKD_TRACE( "gcperc move l2tab[%d] at pos 0x%16.16"PRIx64" len %d",
                            j, upos + i, len);

                /* Make the level 2 table active */
                if (cckd_read_l2 (dev, sfx, j) < 0)
                    return GC_PERC_ERROR();

                /* Write the level 2 table */
                if (cckd_write_l2 (dev) < 0)
                    return GC_PERC_ERROR();
            }
            else
            {
                /* Moving a track image */
                if ((trk = cckd_cchh (dev, buf + i, -1)) < 0)
                    return GC_PERC_SPACE_ERROR();

                L1idx = trk >> 8;
                l2x = trk & 0xff;

                /* Read the lookup entry for the track */
                if (cckd_read_l2ent (dev, &l2, trk) < 0)
                    return GC_PERC_ERROR();
                if (l2.L2_trkoff != (U32)(upos + i))
                    return GC_PERC_SPACE_ERROR();
                len = (int)l2.L2_size;
                if (i + l2.L2_len > (int)ulen) break;

                CCKD_TRACE( "gcperc move trk %d at pos 0x%16.16"PRIx64" len %hu",
                            trk, upos + i, l2.L2_len);

                /* Relocate the track image somewhere else */
                if ((rc = cckd_write_trkimg (dev, buf + i, (int)l2.L2_len, trk, flags)) < 0)
                    return GC_PERC_ERROR();
                a += rc;
            }
        } /* for each space in the used space */

        /* Set `after' to 1 if first time space was relocated after */
        after += after ? a : (a > 0);
        moved += i;

        cckdblk.stats_gcolmoves++;
        cckdblk.stats_gcolbytes += i;

        release_lock( &cckd->filelock );

    } /* while (moved < size) */

    CCKD_TRACE( "gcperc moved %d 1st 0x%x nbr %u", moved,
                cckd->cdevhdr[cckd->sfn].free_off,cckd->cdevhdr[cckd->sfn].free_num);
    // "Collected %u bytes of garbage for CCKD%s file[%d] %1d:%04X %s..."
    if (cckdblk.gcmsgs)
        WRMSG( HHC00386, "I", moved, "", cckd->sfn, LCSS_DEVNUM, cckd_sf_name( dev, cckd->sfn ));
    return moved;
} /* end function cckd_gc_percolate */

/*-------------------------------------------------------------------*/
/* Garbage Collection -- Reposition level 2 tables                   */
/*                                                                   */
/* General idea is to relocate all level 2 tables as close to the    */
/* beginning of the file as possible.  This can help speed up, for   */
/* example, chkdsk processing.                                       */
/*                                                                   */
/* If any level 2 tables reside outside of the bounds (that is, if   */
/* any level 2 table could be moved closer to the beginning of the   */
/* file) then first we relocate all track images within the bounds.  */
/* Note that cckd_get_space will not allocate space within the       */
/* the bounds for track images.  Next we try to relocate all level 2 */
/* tables outside the bounds.  This may take a few iterations for    */
/* the freed space within the bounds to become non-pending.          */
/*                                                                   */
/* The bounds can change as level 2 tables are added or removed.     */
/* cckd_read_l1 sets the bounds and they are adjusted by             */
/* cckd_write_l2.                                                    */
/*-------------------------------------------------------------------*/
int cckd_gc_l2(DEVBLK *dev, BYTE *buf)
{
#if 1 // (deprecate this function!)

    UNREFERENCED( dev );
    UNREFERENCED( buf );

    return 0;

#else // (deprecate this function!)

CCKD_EXT       *cckd;                   /* -> cckd extension         */
int             sfx;                    /* Shadow file index         */
int             i, j;                   /* Work variables            */
int             trk;                    /* Track number              */
int             len;                    /* Track length              */
off_t           pos, fpos;              /* File offsets              */

    if (dev->cckd64)
        return cckd64_gc_l2( dev, buf );

    cckd = dev->cckd_ext;

    obtain_lock (&cckd->filelock);
    sfx = cckd->sfn;

    if (cckd->L2ok || cckd->cdevhdr[cckd->sfn].free_total == 0)
        goto cckd_gc_l2_exit;

    /* Find any level 2 table out of bounds */
    for (i = 0; i < cckd->cdevhdr[sfx].num_L1tab; i++)
        if (cckd->L1tab[sfx][i] != CCKD_NOSIZE && cckd->L1tab[sfx][i] != CCKD_MAXSIZE
         && cckd->L2_bounds - CCKD_L2TAB_SIZE < (U64)cckd->L1tab[sfx][i])
            break;

    /* Return OK if no l2 tables out of bounds */
    if (i >= cckd->cdevhdr[sfx].num_L1tab)
        goto cckd_gc_l2_exit_ok;

    /* Relocate all track images within the bounds */
    pos = CCKD_L1TAB_POS + (cckd->cdevhdr[sfx].num_L1tab * CCKD_L1ENT_SIZE);
    i = cckd->free_idx1st;
    fpos = (off_t)cckd->cdevhdr[sfx].free_off;
    while ((U64)pos < cckd->L2_bounds)
    {
        if (i >= 0 && pos == fpos)
        {
            pos +=        cckd->ifb[i].ifb_len;
            fpos = (off_t)cckd->ifb[i].ifb_offnxt;
            i    =        cckd->ifb[i].ifb_idxnxt;
            j = 0;
        }
        else
        {
            for (j = 0; j < cckd->cdevhdr[sfx].num_L1tab; j++)
                if (pos == (off_t)cckd->L1tab[sfx][j])
                {
                    pos += CCKD_L2TAB_SIZE;
                    break;
                }
        }
        if (j >= cckd->cdevhdr[sfx].num_L1tab)
        {
            /* Found a track to relocate */
            if (cckd_read (dev, sfx, pos, buf, CKD_TRKHDR_SIZE) < 0)
                goto cckd_gc_l2_exit;
            if ((trk = cckd_cchh (dev, buf, -1)) < 0)
                goto cckd_gc_l2_exit;
            CCKD_TRACE( "gc_l2 relocate trk[%d] offset 0x%x", trk, pos);
            if ((len = cckd_read_trkimg (dev, buf, trk, NULL)) < 0)
               goto cckd_gc_l2_exit;
            if (cckd_write_trkimg (dev, buf, len, trk, CCKD_SIZE_EXACT) < 0)
               goto cckd_gc_l2_exit;
            /* Start over */
            pos = CCKD_L1TAB_POS + (cckd->cdevhdr[sfx].num_L1tab * CCKD_L1ENT_SIZE);
            i = cckd->free_idx1st;
            fpos = (off_t)cckd->cdevhdr[sfx].free_off;
        }
    }

    do {
        /* Find a level 2 table to relocate */

        i = cckd->free_idx1st;
        fpos = (off_t) cckd->cdevhdr[sfx].free_off;

        CCKD_TRACE( "gc_l2 first free[%d] pos 0x%x len %d pending %d",
                    i, (int)fpos, i >= 0 ? (int)cckd->ifb[i].ifb_len : -1,
                    i >= 0 ? cckd->ifb[i].ifb_pending : -1 );

        if (i < 0 || (U64) fpos >= cckd->L2_bounds || cckd->ifb[i].ifb_pending)
            goto cckd_gc_l2_exit;

        CCKD_TRACE( "gc_l2 bounds 0x%"PRIx64" sfx %d num_L1tab %d",
            cckd->L2_bounds, sfx, cckd->cdevhdr[sfx].num_L1tab );

        if ( cckd->ifb[i].ifb_len <  CCKD_L2TAB_SIZE
         || (cckd->ifb[i].ifb_len != CCKD_L2TAB_SIZE
          && cckd->ifb[i].ifb_len <  CCKD_L2TAB_SIZE + CCKD_FREEBLK_SIZE)
        )
        {
            for (i=0; i < cckd->cdevhdr[sfx].num_L1tab; i++)
            {
                if (fpos + cckd->ifb[i].ifb_len == (off_t)cckd->L1tab[sfx][i])
                    break;
            }
        }
        else
        {
            for (i=0; i < cckd->cdevhdr[sfx].num_L1tab; i++)
            {
                if (cckd->L2_bounds - CCKD_L2TAB_SIZE < (U64)cckd->L1tab[sfx][i]
                 && cckd->L1tab[sfx][i] != CCKD_MAXSIZE)
                    break;
            }
        }

        if (i < cckd->cdevhdr[sfx].num_L1tab)
        {
            CCKD_TRACE( "gc_l2 relocate l2[%d] pos 0x%x",
                        i, cckd->L1tab[sfx][i] );

            if (cckd_read_l2( dev, sfx, i ) < 0)
                goto cckd_gc_l2_exit;

            if (cckd_write_l2( dev ) < 0)
                goto cckd_gc_l2_exit;
        }
    }
    while (i < cckd->cdevhdr[sfx].num_L1tab);

cckd_gc_l2_exit:
    release_lock (&cckd->filelock);
    return 0;

cckd_gc_l2_exit_ok:
    CCKD_TRACE( "gc_l2 ok%s", "" );
    cckd->L2ok = 1;
    goto cckd_gc_l2_exit;

#endif // (deprecate this function!)
}

/*-------------------------------------------------------------------*/
/* Find device by devnum                                             */
/*-------------------------------------------------------------------*/
DEVBLK *cckd_find_device_by_devnum (U16 devnum)
{
DEVBLK       *dev;
CCKD_EXT     *cckd;

    cckd_lock_devchain (0);
    for (dev = cckdblk.dev1st; dev; dev = cckd->devnext)
    {
        if (dev->devnum == devnum) break;
        cckd = dev->cckd_ext;
    }
    cckd_unlock_devchain ();
    return dev;
} /* end function cckd_find_device_by_devnum */

/*-------------------------------------------------------------------*/
/* Uncompress a track image                                          */
/*-------------------------------------------------------------------*/
BYTE *cckd_uncompress (DEVBLK *dev, BYTE *from, int len, int maxlen,
                       int trk)
{
CCKD_EXT       *cckd;
BYTE           *to = NULL;                /* Uncompressed buffer     */
int             newlen;                   /* Uncompressed length     */
BYTE            comp;                     /* Compression type        */

    cckd = dev->cckd_ext;

    CCKD_TRACE( "uncompress comp %d len %d maxlen %d trk %d",
                from[0] & CCKD_COMPRESS_MASK, len, maxlen, trk);

    /* Extract compression type */
    comp = (from[0] & CCKD_COMPRESS_MASK);

    /* Get a buffer to uncompress into */
    if (comp != CCKD_COMPRESS_NONE && cckd->newbuf == NULL)
    {
        cckd->newbuf = cckd_malloc (dev, "newbuf", maxlen);
        if (cckd->newbuf == NULL)
            return NULL;
    }

    /* Uncompress the track image */
    switch (comp) {

    case CCKD_COMPRESS_NONE:
        newlen = cckd_trklen (dev, from);
        to = from;
        break;
    case CCKD_COMPRESS_ZLIB:
        to = cckd->newbuf;
        newlen = cckd_uncompress_zlib (dev, to, from, len, maxlen);
        break;
    case CCKD_COMPRESS_BZIP2:
        to = cckd->newbuf;
        newlen = cckd_uncompress_bzip2 (dev, to, from, len, maxlen);
        break;
    default:
        newlen = -1;
        break;
    }

    /* Validate the uncompressed track image */
    newlen = cckd_validate (dev, to, trk, newlen);

    /* Return if successful */
    if (newlen > 0)
    {
        if (to != from)
        {
            cckd->newbuf = from;
            cckd->bufused = 1;
        }
        return to;
    }

    /* Get a buffer now if we haven't gotten one */
    if (cckd->newbuf == NULL)
    {
        cckd->newbuf = cckd_malloc (dev, "newbuf2", maxlen);
        if (cckd->newbuf == NULL)
            return NULL;
    }

    /* Try each uncompression routine in turn */

    /* uncompressed */
    newlen = cckd_trklen   (dev, from);
    newlen = cckd_validate (dev, from, trk, newlen);
    if (newlen > 0)
        return from;

    /* zlib compression */
    to = cckd->newbuf;
    newlen = cckd_uncompress_zlib (dev, to, from, len, maxlen);
    newlen = cckd_validate        (dev, to, trk, newlen);
    if (newlen > 0)
    {
        cckd->newbuf = from;
        cckd->bufused = 1;
        return to;
    }

    /* bzip2 compression */
    to = cckd->newbuf;
    newlen = cckd_uncompress_bzip2 (dev, to, from, len, maxlen);
    newlen = cckd_validate         (dev, to, trk, newlen);
    if (newlen > 0)
    {
        cckd->newbuf = from;
        cckd->bufused = 1;
        return to;
    }

    // "%1d:%04X CCKD file[%d] %s: uncompress error trk %d: %2.2x%2.2x%2.2x%2.2x%2.2x"
    WRMSG (HHC00343, "E",
            LCSS_DEVNUM, cckd->sfn, cckd_sf_name(dev, cckd->sfn), trk,
            from[0], from[1], from[2], from[3], from[4]);
    if (comp & ~cckdblk.comps)
        WRMSG (HHC00344, "E",
                LCSS_DEVNUM, cckd->sfn, cckd_sf_name(dev, cckd->sfn), compname[comp]);
    return NULL;
}

/*-------------------------------------------------------------------*/
/* cckd_uncompress_zlib                                              */
/*-------------------------------------------------------------------*/
int cckd_uncompress_zlib (DEVBLK *dev, BYTE *to, BYTE *from, int len, int maxlen)
{
#if defined( HAVE_ZLIB )
unsigned long newlen;
int rc;

    UNREFERENCED(dev);
    memcpy (to, from, CKD_TRKHDR_SIZE);
    newlen = maxlen - CKD_TRKHDR_SIZE;
    rc = uncompress(&to[CKD_TRKHDR_SIZE], &newlen,
                &from[CKD_TRKHDR_SIZE], len - CKD_TRKHDR_SIZE);
    if (rc == Z_OK)
    {
        newlen += CKD_TRKHDR_SIZE;
        to[0] = 0;
    }
    else
        newlen = -1;

    CCKD_TRACE( "uncompress zlib newlen %d rc %d",(int)newlen,rc);

    return (int)newlen;
#else
    UNREFERENCED(dev);
    UNREFERENCED(to);
    UNREFERENCED(from);
    UNREFERENCED(len);
    UNREFERENCED(maxlen);
    return -1;
#endif
}

/*-------------------------------------------------------------------*/
/* cckd_uncompress_bzip2                                             */
/*-------------------------------------------------------------------*/
int cckd_uncompress_bzip2 (DEVBLK *dev, BYTE *to, BYTE *from, int len, int maxlen)
{
#if defined( CCKD_BZIP2 )
unsigned int newlen;
int rc;

    UNREFERENCED(dev);
    memcpy (to, from, CKD_TRKHDR_SIZE);
    newlen = maxlen - CKD_TRKHDR_SIZE;
    rc = BZ2_bzBuffToBuffDecompress (
                (void *)&to[CKD_TRKHDR_SIZE], &newlen,
                (void *)&from[CKD_TRKHDR_SIZE], len - CKD_TRKHDR_SIZE,
                0, 0);
    if (rc == BZ_OK)
    {
        newlen += CKD_TRKHDR_SIZE;
        to[0] = 0;
    }
    else
        newlen = -1;

    CCKD_TRACE( "uncompress bz2 newlen %d rc %d",newlen,rc);

    return (int)newlen;
#else
    UNREFERENCED(dev);
    UNREFERENCED(to);
    UNREFERENCED(from);
    UNREFERENCED(len);
    UNREFERENCED(maxlen);
    return -1;
#endif
}

/*-------------------------------------------------------------------*/
/* Compress a track image                                            */
/*-------------------------------------------------------------------*/
int cckd_compress (DEVBLK *dev, BYTE **to, BYTE *from, int len,
                   int comp, int parm)
{
int newlen;

    switch (comp) {
    case CCKD_COMPRESS_NONE:
        newlen = cckd_compress_none (dev, to, from, len, parm);
        break;
    case CCKD_COMPRESS_ZLIB:
        newlen = cckd_compress_zlib (dev, to, from, len, parm);
        break;
    case CCKD_COMPRESS_BZIP2:
        newlen = cckd_compress_bzip2 (dev, to, from, len, parm);
        break;
    default:
        newlen = cckd_compress_bzip2 (dev, to, from, len, parm);
        break;
    }
    return newlen;
}

/*-------------------------------------------------------------------*/
/* cckd_compress_none                                                */
/*-------------------------------------------------------------------*/
int cckd_compress_none (DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm)
{
    UNREFERENCED(dev);
    UNREFERENCED(parm);
    *to = from;
    from[0] = CCKD_COMPRESS_NONE;
    return len;
}

/*-------------------------------------------------------------------*/
/* cckd_compress_zlib                                                */
/*-------------------------------------------------------------------*/
int cckd_compress_zlib (DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm)
{
#if defined( HAVE_ZLIB )
unsigned long newlen;
int rc;
BYTE *buf;

    UNREFERENCED(dev);
    buf = *to;
    from[0] = CCKD_COMPRESS_NONE;
    memcpy (buf, from, CKD_TRKHDR_SIZE);
    buf[0] = CCKD_COMPRESS_ZLIB;
    newlen = 65535 - CKD_TRKHDR_SIZE;
    rc = compress2 (&buf[CKD_TRKHDR_SIZE], &newlen,
                    &from[CKD_TRKHDR_SIZE], len - CKD_TRKHDR_SIZE,
                    parm);
    newlen += CKD_TRKHDR_SIZE;
    if (rc != Z_OK || (int)newlen >= len)
    {
        *to = from;
        newlen = len;
    }
    return (int)newlen;
#else

#if defined( CCKD_BZIP2 )
    return cckd_compress_bzip2 (dev, to, from, len, parm);
#else
    return cckd_compress_none (dev, to, from, len, parm);
#endif

#endif
}
int cckd_compress_bzip2 (DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm)
{
#if defined( CCKD_BZIP2 )
unsigned int newlen;
int rc;
BYTE *buf;

    UNREFERENCED(dev);
    buf = *to;
    from[0] = CCKD_COMPRESS_NONE;
    memcpy (buf, from, CKD_TRKHDR_SIZE);
    buf[0] = CCKD_COMPRESS_BZIP2;
    newlen = 65535 - CKD_TRKHDR_SIZE;
    rc = BZ2_bzBuffToBuffCompress (
                    (void *)&buf[CKD_TRKHDR_SIZE], &newlen,
                    (void *)&from[CKD_TRKHDR_SIZE], len - CKD_TRKHDR_SIZE,
                    parm >= 1 && parm <= 9 ? parm : 5, 0, 0);
    newlen += CKD_TRKHDR_SIZE;
    if (rc != BZ_OK || newlen >= (unsigned int)len)
    {
        *to = from;
        newlen = len;
    }
    return newlen;
#else
    return cckd_compress_zlib (dev, to, from, len, parm);
#endif
}

/*-------------------------------------------------------------------*/
/* cckd command help                                                 */
/*-------------------------------------------------------------------*/
void cckd_command_help()
{
    int i;
    char *help[] =
    {
        "Command parameters for cckd:"
        , ""
        , "  help          Display help message"
        , "  stats         Display cckd statistics"
        , "  opts          Display cckd options"
        , ""

        //    ***  Please keep these in alphabetical order!  ***

        , "  comp=<n>      Override compression                 (-1,0,1,2)"
        , "  compparm=<n>  Override compression parm            (-1 ... 9)"
        , "  debug=<n>     Enable CCW tracing debug messages      (0 or 1)"
        , "  dtax=<n>      Dump cckd trace table at exit          (0 or 1)"
        , "  freepend=<n>  Set free pending cycles              (-1 ... 4)"
        , "  fsync=<n>     Enable fsync                           (0 or 1)"
        , "  gcint=<n>     Set garbage collector interval (sec) ( 0 .. 60)"
        , "  gcmsgs=<n>    Display garbage collector messages     (0 or 1)"
        , "  gcparm=<n>    Set garbage collector parameter      (-8 ... 8)"
        , "  gcstart=<n>   Start garbage collector                (0 or 1)"
        , "  linuxnull=<n> Check for null linux tracks            (0 or 1)"
        , "  nosfd=<n>     Disable stats report at close          (0 or 1)"
        , "  nostress=<n>  Disable stress writes                  (0 or 1)"
        , "  ra=<n>        Set number readahead threads         ( 1 ... 9)"
        , "  raq=<n>       Set readahead queue size             ( 0 .. 16)"
        , "  rat=<n>       Set number tracks to read ahead      ( 0 .. 16)"
        , "  trace=<n>     Set trace table size             (0 ... 200000)"
        , "  wr=<n>        Set number writer threads            ( 1 ... 9)"

        , NULL
    };

    for (i=0; help[i] != NULL; i++ )
        WRMSG(HHC00345, "I", help[i] );

} /* end function cckd_command_help */

/*-------------------------------------------------------------------*/
/* cckd command stats                                                */
/*-------------------------------------------------------------------*/
void cckd_command_opts()
{
    char msgbuf[256];

    MSGBUF( msgbuf, "cckd opts:"

        // ***  Please keep these in alphabetical order!  ***

        " "   "comp=%d"
        ","   "compparm=%d"
        ","   "debug=%d"
        ","   "dtax=%d"
        ","   "freepend=%d"
        ","   "fsync=%d"
        ","   "gcint=%d"
        ","   "gcmsgs=%d"

        , cckdblk.comp == 0xff ? -1 : cckdblk.comp
        , cckdblk.compparm
        , cckdblk.debug
        , cckdblk.dtax
        , cckdblk.freepend
        , cckdblk.fsync
        , cckdblk.gcint
        , cckdblk.gcmsgs
    );
    WRMSG( HHC00346, "I", msgbuf );

    MSGBUF( msgbuf, "          "

        // ***  Please keep these in alphabetical order!  ***

        " "   "gcparm=%d"
        ","   "linuxnull=%d"
        ","   "nosfd=%d"
        ","   "nostress=%d"
        ","   "ra=%d"
        ","   "raq=%d"
        ","   "rat=%d"
        ","   "trace=%d"
        ","   "wr=%d"

        , cckdblk.gcparm
        , cckdblk.linuxnull
        , cckdblk.nosfd
        , cckdblk.nostress
        , cckdblk.ramax
        , cckdblk.ranbr
        , cckdblk.readaheads
        , cckdblk.itracen
        , cckdblk.wrmax
    );
    WRMSG( HHC00346, "I", msgbuf );

    return;
} /* end function cckd_command_opts */

/*-------------------------------------------------------------------*/
/* cckd command stats                                                */
/*-------------------------------------------------------------------*/
void cckd_command_stats()
{
    char msgbuf[128];

    WRMSG( HHC00347, "I", "cckd stats:" );

    MSGBUF( msgbuf, "  reads....%10"PRId64" Kbytes...%10"PRId64,
                    cckdblk.stats_reads, cckdblk.stats_readbytes >> SHIFT_1K );
    WRMSG( HHC00347, "I", msgbuf );

    MSGBUF( msgbuf, "  writes...%10"PRId64" Kbytes...%10"PRId64,
                    cckdblk.stats_writes, cckdblk.stats_writebytes >> SHIFT_1K );
    WRMSG( HHC00347, "I", msgbuf );

    MSGBUF( msgbuf, "  readaheads%9"PRId64" misses...%10"PRId64,
                    cckdblk.stats_readaheads, cckdblk.stats_readaheadmisses );
    WRMSG( HHC00347, "I", msgbuf );

    MSGBUF( msgbuf, "  switches.%10"PRId64" l2 reads.%10"PRId64" strs wrt.%10"PRId64,
                    cckdblk.stats_switches, cckdblk.stats_l2reads, cckdblk.stats_stresswrites );
    WRMSG( HHC00347, "I", msgbuf );

    MSGBUF( msgbuf, "  cachehits%10"PRId64" misses...%10"PRId64,
                    cckdblk.stats_cachehits, cckdblk.stats_cachemisses );
    WRMSG( HHC00347, "I", msgbuf );

    MSGBUF( msgbuf, "  l2 hits..%10"PRId64" misses...%10"PRId64,
                    cckdblk.stats_l2cachehits, cckdblk.stats_l2cachemisses );
    WRMSG( HHC00347, "I", msgbuf );

    MSGBUF( msgbuf, "  waits............   i/o......%10"PRId64" cache....%10"PRId64,
                    cckdblk.stats_iowaits, cckdblk.stats_cachewaits );
    WRMSG( HHC00347, "I", msgbuf );

    MSGBUF( msgbuf, "  garbage collector   moves....%10"PRId64" Kbytes...%10"PRId64,
                    cckdblk.stats_gcolmoves, cckdblk.stats_gcolbytes >> SHIFT_1K );
    WRMSG( HHC00347, "I", msgbuf );

    return;
} /* end function cckd_command_stats */

/*-------------------------------------------------------------------*/
/* cckd_dtax return Dump Table At Exit setting                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool cckd_dtax()
{
    return cckdblk.dtax ? true : false;
}

/*-------------------------------------------------------------------*/
/* cckd command processor                                            */
/*-------------------------------------------------------------------*/
DLL_EXPORT int cckd_command( char* op, int cmd )
{
char  *kw, *p, c, buf[256];
int   val, opts = 0;
int   rc;

    /* Display help for null operand */
    if (!op)
    {
        if (cmd && memcmp( &cckdblk.id, CCKDBLK_ID, sizeof( cckdblk.id )) == 0)
            cckd_command_help();
        return 0;
    }

    STRLCPY( buf, op );
    op = buf;

    /* Initialize the global cckd block if necessary */
    if (memcmp( &cckdblk.id, CCKDBLK_ID, sizeof( cckdblk.id )))
        cckd_dasd_init( 0, NULL );

    while (op)
    {
        /* Operands are delimited by commas */
        kw = op;
        op = strchr( op, ',' );
        if (op) *op++ = 0;

        /* Check for "keyword=value" */
        if ((p = strchr( kw, '=' )))
        {
            *p++ = 0;
            c = 0;
            rc = sscanf( p, "%d%c", &val, &c );
        }

        /* If p == NULL, then just keyword syntax (no "=value") */
        if (!p)
        {
            /* Parse the keyword... */

            // Display cckd help
            if (CMD( kw, HELP, 4 ))
            {
                if (!cmd) return 0;
                cckd_command_help();
            }
            // Display current cckd statistics
            else if (CMD( kw, STATS, 4 ))
            {
                if (!cmd) return 0;
                cckd_command_stats();
            }
            // Display current cckd options
            else if (CMD( kw, OPTS, 4 ))
            {
                if (!cmd) return 0;
                cckd_command_opts();
            }
            else
            {
                // "CCKD file: invalid cckd keyword: %s"
                WRMSG( HHC00349, "E", kw );
                cckd_command_help();
                return -1;
            }
        }
        /* If rc != 1 || c != 0 then either a non-numeric "=value"
           was skipped (sscanf %d failed) and/or something follows
           the numeric "=value" (sscanf %c succeeded)
        */
        else if (rc != 1 || c != 0)
        {
            // "CCKD file: invalid numeric value %s"
            WRMSG( HHC00350, "E", p );
            return -1;
        }
        /* If rc == 1 && c == 0, then "keyword=value" syntax */
        /* Please keep the below tests in alphabetical order! */

        // Compression to be used
        else if (CMD( kw, COMP, 4 ))
        {
            if (val < -1 || (val > 0 && (val & ~cckdblk.comps)))
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else switch (val)
            {
            case -1:
            case CCKD_COMPRESS_NONE:
            case CCKD_COMPRESS_ZLIB:
            case CCKD_COMPRESS_BZIP2:
                cckdblk.comp = val < 0 ? 0xff : val;
                opts = 1;
                break;
            default: /* unsupported algorithm */
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
        }
        // Compression parameter to be used
        else if (CMD( kw, COMPPARM, 8 ))
        {
            if (val < -1 || val > 9)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.compparm = val;
                opts = 1;
            }
        }
        // Turn CCW tracing debug messages on or off
        else if (CMD( kw, DEBUG, 5 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.debug = val;
                opts = 1;
            }
        }
        // Dump Table At Exit
        else if (CMD( kw, DTAX, 4 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.dtax = val;
                opts = 1;
            }
        }
        // Set the free pending value
        else if (CMD( kw, FREEPEND, 8 ))
        {
            if (val < -1 || val > CCKD_MAX_FREEPEND)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.freepend = val;
                opts = 1;
            }
        }
        // Turn fsync on or off
        else if (CMD( kw, FSYNC, 5 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.fsync = val;
                opts = 1;
            }
        }
        // Garbage collection interval
        else if (CMD( kw, GCINT, 5 ))
        {
            if (val < CCKD_MIN_GCINT || val > CCKD_MAX_GCINT)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.gcint = val;
                opts = 1;
            }
        }
        // Garbage collector messages
        else if (CMD( kw, GCMSGS, 6 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.gcmsgs = val;
                opts = 1;
            }
        }
        // Garbage collection parameter
        else if (CMD( kw, GCPARM, 6 ))
        {
            if (val < CCKD_MIN_GCPARM || val > CCKD_MAX_GCPARM)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.gcparm = val;
                opts = 1;
            }
        }
        // Start garbage collector
        else if (CMD( kw, GCSTART, 7 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else if (val == 1)
            {
                cckd_gcstart();
                cckd64_gcstart();
            }
        }
        // Check for null linux tracks
        else if (CMD( kw, LINUXNULL, 5 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.linuxnull = val;
                opts = 1;
            }
        }
        // Turn off stats report at close
        else if (CMD( kw, NOSFD, 5 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.nosfd = val;
                opts = 1;
            }
        }
        // Turn stress writes on or off
        else if (CMD( kw, NOSTRESS, 8 ))
        {
            if (val < 0 || val > 1)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.nostress = val;
                opts = 1;
            }
        }
        // Number readahead threads
        else if (CMD( kw, RA, 2 ))
        {
            if (val < CCKD_MIN_RA || val > CCKD_MAX_RA)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.ramax = val;
                opts = 1;
            }
        }
        // Readahead queue size
        else if (CMD( kw, RAQ, 3 ))
        {
            if (val < 0 || val > CCKD_MAX_RA_SIZE)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.ranbr = val;
                opts = 1;
            }
        }
        // Number of tracks to readahead
        else if (CMD( kw, RAT, 3 ))
        {
            if (val < 0 || val > CCKD_MAX_RA_SIZE)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.readaheads = val;
                opts = 1;
            }
        }
        // Number of trace table entries
        else if (CMD( kw, TRACE, 5 ))
        {
            if (val < 0 || val > CCKD_MAX_NUM_TRACE)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                OBTAIN_TRACE_LOCK();
                {
                    CCKD_ITRACE* itrace = cckdblk.itrace;

                    /* Tell others there is no table anymore */
                    cckdblk.itracen = 0;
                    cckdblk.itracec = 0;
                    cckdblk.itrace  = NULL;
                    cckdblk.itracep = NULL;
                    cckdblk.itracex = NULL;

                    /* Discard existing trace table */
                    free( itrace );

                    /* Allocate new table (if requested) */
                    if (val > 0)
                    {
                        if (!(itrace = calloc( val, sizeof( CCKD_ITRACE ))))
                        {
                            char buf[64];
                            MSGBUF( buf, "calloc( %d, %d )",
                                val, (int) sizeof( CCKD_ITRACE ));
                            // "%1d:%04X CCKD file: error in function %s: %s"
                            WRMSG( HHC00303, "E", 0,0, buf, strerror( errno ));
                            val = 0;
                        }
                    }
                    else
                        itrace = NULL;

                    /* Update global variables */
                    cckdblk.itrace  = itrace;
                    cckdblk.itracep = itrace;
                    cckdblk.itracex = itrace + val;
                    cckdblk.itracen = val;
                    cckdblk.itracec = 0;
                    opts = 1;
                }
                RELEASE_TRACE_LOCK();
            }
        }
        // Number writer threads
        else if (CMD( kw, WR, 2 ))
        {
            if (val < CCKD_MIN_WRITER || val > CCKD_MAX_WRITER)
            {
                // "CCKD file: value %d invalid for %s"
                WRMSG( HHC00348, "E", val, kw );
                return -1;
            }
            else
            {
                cckdblk.wrmax = val;
                opts = 1;
            }
        }
        // Unknown option
        else
        {
            // "CCKD file: invalid cckd keyword: %s"
            WRMSG( HHC00349, "E", kw );
            if (!cmd) return -1;
            cckd_command_help();
            op = NULL;
        }
    }
    /* end while (op) */

    if (cmd && opts)
        cckd_command_opts();

    return 0;
} /* end function cckd_command */

/*-------------------------------------------------------------------*/
/* Print internal trace                                              */
/*-------------------------------------------------------------------*/
DLL_EXPORT void cckd_print_itrace()
{

    // "CCKD file: internal cckd trace"
    WRMSG( HHC00399, "I" );

    OBTAIN_TRACE_LOCK();
    if (cckdblk.itracen)                    // have trace table?
    {
        CCKD_ITRACE*  pbeg;                 // start of trace table
        CCKD_ITRACE*  pcur;                 // current working entry
        int i;                              // loop counter

        pbeg = cckdblk.itrace;              // save beginning of table
        pcur = cckdblk.itracep;             // "current" (oldest) entry

        if (pcur >= cckdblk.itracex)        // past end of table?
            pcur = pbeg;                    // then wrap to begin

        for (i=0; i < cckdblk.itracec; ++i) // print all used entries
        {
            if (*(const char*)pcur)         // print entry if not empty
                // "%s"
                WRMSG( HHC00398, "I", (const char*) pcur );

            if (++pcur >= cckdblk.itracex)  // next entry; if past end
                pcur = pbeg;                // then wrap to begin
        }

        // Clear all table entries and reset pointers
        memset( pbeg, 0, cckdblk.itracen * sizeof( CCKD_ITRACE ));
        cckdblk.itracep = pbeg;
        cckdblk.itrace  = pbeg;
        cckdblk.itracec = 0;
    }
    RELEASE_TRACE_LOCK();

} /* end function cckd_print_itrace */

/*-------------------------------------------------------------------*/
/* Write internal trace entry                                        */
/*-------------------------------------------------------------------*/
void cckd_trace( const char* func, int line, DEVBLK* dev, char* fmt, ... )
{
    va_list vl;
    char trcmsg[ CCKD_TRACE_SIZE ];

    /* Quick exit if trace messages not enabled or no trace table */
    if (!(dev && (cckdblk.debug || cckdblk.itrace)))
        return;

    /* Build the cckd trace message */
    va_start( vl, fmt );
    if (vsnprintf( trcmsg, sizeof( trcmsg ), fmt, vl ) < 0)
        return;
    va_end( vl );

    /* Add the message to our internal trace table (if we have one) */
    OBTAIN_TRACE_LOCK();
    if (cckdblk.itracen)
    {
        CCKD_ITRACE*    itracep;            // (trace table entry ptr)
        struct timeval  timeval;            // (microsecond accuracy)
        time_t          todsecs;            // (#of secs since epoch)
        char            todwrk[32];         // (work)
        char            trcpfx[64];         // "HHHHHHHH @ hh:mm:ss.uuuuuu n:CCUU"

        /* Build TID+TOD+CUU trace message prefix string */

        gettimeofday( &timeval, NULL );     // (microsecond accuracy)

        todsecs = timeval.tv_sec;
        STRLCPY( todwrk, ctime( &todsecs ));// "Day Mon dd hh:mm:ss yyyy\n"
        todwrk[19] = 0;                     // "Day Mon dd hh:mm:ss"

        MSGBUF( trcpfx,

            TIDPAT" @ %s.%6.6ld %1d:%04X",  // "HHHHHHHH @ hh:mm:ss.uuuuuu n:CCUU"

            TID_CAST(hthread_self()),       // "HHHHHHHH" (TIDPAT)
            todwrk + 11,                    // "hh:mm:ss" (%s)
            (long int)timeval.tv_usec,      // "uuuuuu"   (%6.6ld
            LCSS_DEVNUM                     // "n:CCUU"   (%1d:%04X)
        );

        /* Print trace message directly into trace table entry */

        itracep = cckdblk.itracep++;        // (grab table entry)

        if (itracep >= cckdblk.itracex)     // (past end of table?)
        {
            itracep = cckdblk.itrace;       // (wrap to beginning)
            cckdblk.itracep = itracep + 1;  // (curr = next entry)
        }

        if (cckdblk.itracec < cckdblk.itracen) // (less than all used?)
            cckdblk.itracec++;                 // (count entries used)

        snprintf( (char*) itracep, CCKD_TRACE_SIZE, "%s %s(%d): %s",
            trcpfx, func, line, trcmsg );
    }
    RELEASE_TRACE_LOCK();

    /* Log the trace entry if requested */
    if (cckdblk.debug)
    {
        if (dev && dev->ccwtrace)
            // "%1d:%04X %s"
            WRMSG( HHC00396, "I", LCSS_DEVNUM, trcmsg );
    }

} /* end function cckd_trace */

DLL_EXPORT DEVHND cckd_dasd_device_hndinfo = {
        &ckd_dasd_init_handler,        /* Device Initialization      */
        &ckd_dasd_execute_ccw,         /* Device CCW execute         */
        &cckd_dasd_close_device,       /* Device Close               */
        &ckd_dasd_query_device,        /* Device Query               */
        NULL,                          /* Device Extended Query      */
        &cckd_dasd_start,              /* Device Start channel pgm   */
        &cckd_dasd_end,                /* Device End channel pgm     */
        &cckd_dasd_start,              /* Device Resume channel pgm  */
        &cckd_dasd_end,                /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        &cckd_read_track,              /* Device Read                */
        &cckd_update_track,            /* Device Write               */
        &cckd_used,                    /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        NULL,                          /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Ouput       */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        &ckd_dasd_hsuspend,            /* Hercules suspend           */
        &ckd_dasd_hresume              /* Hercules resume            */
};

DLL_EXPORT DEVHND cfba_dasd_device_hndinfo = {
        &fba_dasd_init_handler,        /* Device Initialization      */
        &fba_dasd_execute_ccw,         /* Device CCW execute         */
        &cckd_dasd_close_device,       /* Device Close               */
        &fba_dasd_query_device,        /* Device Query               */
        NULL,                          /* Device Extended Query      */
        &cckd_dasd_start,              /* Device Start channel pgm   */
        &cckd_dasd_end,                /* Device End channel pgm     */
        &cckd_dasd_start,              /* Device Resume channel pgm  */
        &cckd_dasd_end,                /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        &cfba_read_block,              /* Device Read                */
        &cfba_write_block,             /* Device Write               */
        &cfba_used,                    /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        NULL,                          /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Ouput       */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        &fba_dasd_hsuspend,            /* Hercules suspend           */
        &ckd_dasd_hresume              /* Hercules resume            */
};
