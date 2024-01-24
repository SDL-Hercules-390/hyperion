/* SCSITAPE.C   (C) Copyright "Fish" (David B. Trout), 2005-2012     */
/*              Hercules SCSI tape handling module                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Original Author: "Fish" (David B. Trout)                          */
/* Prime Maintainer: "Fish" (David B. Trout)                         */
/* Secondary Maintainer: Ivan Warren                                 */

/*-------------------------------------------------------------------*/
/* This module only contains support for SCSI tapes. Please see the  */
/* 'tapedev.c' (and possibly other) source module(s) for information */
/* regarding support for other emulated tape/media formats.          */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "scsitape.h"

#if defined(OPTION_SCSI_TAPE)

/*-------------------------------------------------------------------*/
/*                 Tweakable build constants                         */
/*-------------------------------------------------------------------*/
#define SLOW_UPDATE_STATUS_TIMEOUT  MAX_NORMAL_SCSI_DRIVE_QUERY_RESPONSE_TIMEOUT_USECS
#define MAX_GSTAT_FREQ_USECS        1000000  // (once per second max)

/*-------------------------------------------------------------------*/
/*                        Debugging                                  */
/*-------------------------------------------------------------------*/
//#define  ENABLE_TRACING_STMTS   1       // Enable "TRACE" macro
//#include "dbgtrace.h"                   // Enable "TRACE" macro

/*-------------------------------------------------------------------*/
/*           (internal non-blocking SCSI tape open)                  */
/*-------------------------------------------------------------------*/
static int int_nbio_scsitape_open( const char* fn, BYTE* ro )
{
    int fd;

    /* Try opening device in read/write mode first */
    *ro = 0;
    fd = open_tape( fn, O_RDWR | O_BINARY | O_NONBLOCK );

    if (fd < 0 && EROFS == errno )
    {
        /* If that didn't work then try read-only */
        *ro = 1;
        fd = open_tape( fn, O_RDONLY | O_BINARY | O_NONBLOCK );
    }

    return fd;
}

/*-------------------------------------------------------------------*/
/*           (internal determine maximum blocksize)                  */
/*-------------------------------------------------------------------*/
static int get_max_blocksize( DEVBLK *dev, int* maxblk )
{
    //                       2M       2M-1     256K    256K-1  128K
    static int blksize[] = { 2097152, 2097151, 262144, 262143, 131072,
    //                       128K-1  64K    64K-1  32K    32K-1
                             131071, 65536, 65535, 32768, 32767 };
    struct mtop opblk;
    size_t i;
    int rc;

    // Technique: keep trying to set ever decreasing fixed-
    // length block sizes until one of them eventually works.

    for (i=0; i < _countof( blksize ); ++i)
    {
        opblk.mt_op    = MTSETBLK;
        opblk.mt_count = blksize[i];

        if (ioctl_tape( dev->fd, MTIOCTOP, (char*) &opblk ) == 0)
            break;
    }

    if (i >= _countof( blksize ))
        i  = _countof( blksize ) - 1;

    *maxblk = blksize[i];

    // (reset back to variable blocksize mode again)

    opblk.mt_op    = MTSETBLK;
    opblk.mt_count = 0;

    rc = ioctl_tape( dev->fd, MTIOCTOP, (char*) &opblk );
    return rc;
}

/*-------------------------------------------------------------------*/
/*                 Open a SCSI tape device                           */
/*                                                                   */
/* If successful, the file descriptor is stored in the device block  */
/* and the return value is zero.  Otherwise the return value is -1.  */
/*                                                                   */
/* Note that not having a tape mounted is a non-fatal error (rc==0)) */
/*                                                                   */
/* If the status indicates the tape is not mounted or a good status  */
/* cannot otherwise be obtained, the file descriptor is CLOSED but   */
/* THE RETURN CODE IS STILL == 0 !!!!                                */
/*                                                                   */
/*                       ** WARNING! **                              */
/*                                                                   */
/* The caller MUST check for a valid (non-negative) file descriptor  */
/* before trying to use it if this function returns 0 == success!!   */
/*                                                                   */
/* A success == 0 return means the device filename CAN be opened,    */
/* but not necessarily that the file can be used! If the file cannot */
/* be used (i.e. no tape mounted), the file is CLOSED but the return */
/* code is still == 0.                                               */
/*                                                                   */
/* The return code is -1 ONLY when the filename itself is invalid    */
/* and the device file thus cannot even be opened.                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
int open_scsitape (DEVBLK *dev, BYTE *unitstat, BYTE code)
{
    BYTE  ro;                               /* Read-only flag        */

    /* Is an open for this device already in progress? */
    if (dev->stape_mntdrq.link.Flink)
    {
        /* Yes. Device is good but no tape is mounted (yet) */
        build_senseX( TAPE_BSENSE_TAPEUNLOADED, dev, unitstat, code );
        return 0; // (quick exit; in progress == open success)
    }

    ASSERT( dev->fd < 0 );  // (sanity check)
    dev->fd = -1;
    dev->sstat = dev->stape_online ? 0 : GMT_DR_OPEN( -1 );

    /* Open the SCSI tape device */
    dev->fd = int_nbio_scsitape_open( dev->filename, &ro );
    dev->readonly = ro ? 1 : 0;

    /* Check for successful open */
    if (dev->fd < 0)
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "open_tape()", strerror(errno));
        build_senseX( TAPE_BSENSE_ITFERROR, dev, unitstat, code );
        return -1; // (FATAL error; device cannot be opened)
    }
    define_BOT_pos( dev );  // (always after successful open)

    /* Obtain the initial tape device/media status information */
    /* and start the mount-monitoring thread if option enabled */
    int_scsi_status_update( dev, 0 );

    /* Display status flags if debugging */
    if (dev->debug)
    {
        char buffer[ 384 ] = {0};
        // "Tape %s: %smt_gstat 0x%8.8"PRIX32" %s"
        WRMSG( HHC92702, "D", dev->filename, "open_scsitape ",
            (U32) dev->sstat, gstat2str( (U32) dev->sstat, buffer, sizeof( buffer )));
    }

    /* Asynchronous open now in progress? */
    if (dev->stape_mntdrq.link.Flink)
    {
        /* Yes. Device is good but no tape is mounted (yet) */
        build_senseX( TAPE_BSENSE_TAPEUNLOADED, dev, unitstat, code );
        return 0; // (quick exit; in progress == open success)
    }

    /* Finish up the open process... */
    if (STS_NOT_MOUNTED( dev ))
    {
        /* Intervention required if no tape is currently mounted.
           Note: we return "success" because the filename is good
           (device CAN be opened) but close the file descriptor
           since there's no tape currently mounted on the drive.*/
#if !defined( _MSVC_ )
        int fd = dev->fd;
        dev->fd = -1;
        close_tape( fd );
#endif // !_MSVC_
        build_senseX( TAPE_BSENSE_TAPEUNLOADED, dev, unitstat, code );
        return 0; // (because device file IS valid and CAN be opened)
    }

    /* Set variable length block processing to complete the open */
    if (finish_scsitape_open( dev, unitstat, code ) != 0)
    {
        /* We cannot use this device; fail the open.
           'finish_scsitape_open' has already issued
           the error message and closed the device. */
        return -1;  // (open failure)
    }
    return 0;       // (open success)

} /* end function open_scsitape */

/*-------------------------------------------------------------------*/
/*           (internal SCSI tape is mounted status)                  */
/*-------------------------------------------------------------------*/
int int_scsi_status_mounted( DEVBLK *dev, int update_status )
{
    /* If device is not even opened yet, then no tape is mounted */
    if (dev->fd < 0)
        return FALSE;

    /* Make sure status is current if requested */
    if (update_status)
        int_scsi_status_update( dev, 0 );

    /* Which status flag should we check? */
    if (dev->stape_online)
    {
        /* If "online" flag is set then tape is mounted */
        if (GMT_ONLINE( dev->sstat ))
            return TRUE;

        /* Otherwise tape is NOT mounted */
        return FALSE;
    }
    else
    {
        /* If "door open" flag is set then tape is NOT mounted */
        if (GMT_DR_OPEN( dev->sstat ))
            return FALSE;

        /* Otherwise tape IS mounted */
        return TRUE;
    }
} /* end function int_scsi_status_mounted */

/*-------------------------------------------------------------------*/
/*  Finish SCSI Tape open:  sets variable length block i/o mode...   */
/*  Returns 0 == success, -1 = failure.                              */
/*-------------------------------------------------------------------*/
/*  THIS FUNCTION IS AN INTEGRAL PART OF TAPE OPEN PROCESSING!       */
/*  If this function fails then the overall tape device open fails!  */
/*-------------------------------------------------------------------*/
int finish_scsitape_open( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
    int rc;                             /* Return code               */
    int oflags;                         /* re-open flags             */

    /* Switch over to BLOCKING-mode I/O... (without O_NONBLOCK flag) */

    close_tape( dev->fd );
    oflags = O_BINARY | (dev->readonly ? O_RDONLY : O_RDWR);

    if ((dev->fd = open_tape( dev->filename, oflags )) < 0)
    {
        /* Device cannot be used; fail the open */
        int save_errno = errno;
        rc = dev->fd;
        dev->fd = -1;
        close_tape( rc );
        errno = save_errno;
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "finish_scsitape_open(reopen)",
                strerror(errno));
        build_senseX(TAPE_BSENSE_ITFERROR,dev,unitstat,code);
        return -1; /* (fatal error) */
    }

    /* Display status flags if debugging */
    if (dev->debug)
    {
        char buffer[ 384 ] = {0};
        VERIFY( int_scsi_status_mounted( dev, 1 ));  /* sanity check */
        // "Tape %s: %smt_gstat 0x%8.8"PRIX32" %s"
        WRMSG( HHC92702, "D", dev->filename, "finish_scsitape_open ",
            (U32) dev->sstat, gstat2str( (U32) dev->sstat, buffer, sizeof( buffer )));
    }

    /* Since a tape was just mounted, reset the blockid back to zero */

    dev->blockid = 0;
    dev->fenced = 0;

    /*  Determine maximum I/O size and set the tape device to variable
        length block mode. We must do this regardless of whether we'll
        be writing to to tape or not.
    */
    rc = get_max_blocksize( dev, &dev->bufsize );

    if (rc < 0)
    {
        /* Device cannot be used; fail the open */
        int save_errno = errno;
        rc = dev->fd;
        dev->fd = -1;
        close_tape( rc );
        errno = save_errno;
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "ioctl_tape(MTSETBLK)", strerror(errno));
        build_senseX(TAPE_BSENSE_ITFERROR,dev,unitstat,code);
        return -1; /* (fatal error) */
    }

#if defined( HAVE_DECL_MTEWARN ) && HAVE_DECL_MTEWARN

    // Try to request EOM/EOT (end-of-media/tape) early-warning

    // Note: if it fails, oh well. There's no need to scare the
    // user with a warning message. We'll either get the warning
    // or we won't. Either way there's nothing we can do about it.
    // We did the best we could.

    if (!STS_WR_PROT( dev ))
    {
        struct mtop opblk;

        opblk.mt_op    = MTEWARN;
        opblk.mt_count = dev->eotmargin;

        ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

        // (ignore any error; it either worked or it didn't)
    }

#endif // defined( HAVE_DECL_MTEWARN ) && HAVE_DECL_MTEWARN

    return 0;  /* (success) */

} /* end function finish_scsitape_open */

/*-------------------------------------------------------------------*/
/* Close SCSI tape device file                                       */
/*-------------------------------------------------------------------*/
void close_scsitape(DEVBLK *dev)
{
    int rc = 0;

    obtain_lock( &sysblk.stape_lock );

    // Remove drive from SCSIMOUNT thread's work queue...

    if (                     dev->stape_mntdrq.link.Flink) {
        RemoveListEntry(    &dev->stape_mntdrq.link );
        InitializeListLink( &dev->stape_mntdrq.link );
    }

    // Remove drive from the STATUS thread's work queue...

    if (                     dev->stape_statrq.link.Flink) {
        RemoveListEntry(    &dev->stape_statrq.link );
        InitializeListLink( &dev->stape_statrq.link );
    }

    // Close the file if it's open...
    if (dev->fd >= 0)
    {
        if (dev->stape_close_rewinds)
        {
            struct mtop opblk;
//          opblk.mt_op    = MTLOAD;    // (not sure which is more correct)
            opblk.mt_op    = MTREW;
            opblk.mt_count = 1;

            if ((rc = ioctl_tape ( dev->fd, MTIOCTOP, (char*)&opblk)) != 0)
            {
                // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
                WRMSG (HHC00205, "E", LCSS_DEVNUM,
                        dev->filename, "scsi", "ioctl_tape(MTREW)", strerror(errno));
            }
        }

        // Close tape drive...
        close_tape( dev->fd );

        dev->fd        = -1;
        dev->blockid   = -1;
        dev->curfilen  =  0;
        dev->nxtblkpos =  0;
        dev->prvblkpos = -1;
    }

    dev->sstat  = dev->stape_online ? 0 : GMT_DR_OPEN(-1); // (forced)
    dev->fenced = (rc < 0 && !SVF_ENABLED()) ? 1 : 0;

    release_lock( &sysblk.stape_lock );

} /* end function close_scsitape */

/*-------------------------------------------------------------------*/
/* Read a block from a SCSI tape device                              */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the return value is zero, and the         */
/* current file number in the device block is incremented.           */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int read_scsitape (DEVBLK *dev, BYTE *buf, BYTE *unitstat,BYTE code)
{
int  rc;

    rc = read_tape( dev->fd, buf, dev->bufsize );

    if (rc >= 0)
    {
        dev->blockid++;

        /* Increment current file number if tapemark was read */
        if (rc == 0)
            dev->curfilen++;

        /* Return block length or zero if tapemark  */
        return rc;
    }

    /* Handle read error condition */

    // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
    WRMSG (HHC00205, "E", LCSS_DEVNUM,
            dev->filename, "scsi", "read_tape()", strerror(errno));

    if ( STS_NOT_MOUNTED( dev ) )
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    else
        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);

    return -1;

} /* end function read_scsitape */

/*-------------------------------------------------------------------*/
/* Write a block to a SCSI tape device                               */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int write_scsitape (DEVBLK *dev, const BYTE *buf, U32 len,
                    BYTE *unitstat, BYTE code)
{
int  rc;
int  save_errno;

    /* Write data block to SCSI tape device */

    rc = write_tape (dev->fd, buf, len);

#if defined( _MSVC_ )
    if (errno == ENOSPC)
        dev->eotwarning = 1;
#endif

    if (rc >= (int)len)
    {
        dev->blockid++;
        return 0;
    }

    /*         LINUX EOM BEHAVIOUR WHEN WRITING

      When the end of medium early warning is encountered,
      the current write is finished and the number of bytes
      is returned. The next write returns -1 and errno is
      set to ENOSPC. To enable writing a trailer, the next
      write is allowed to proceed and, if successful, the
      number of bytes is returned. After this, -1 and the
      number of bytes are alternately returned until the
      physical end of medium (or some other error) occurs.
    */

    if (errno == ENOSPC)
    {
        int_scsi_status_update( dev, 0 );

        rc = write_tape (dev->fd, buf, len);

        if (rc >= (int)len)
        {
            dev->eotwarning = 1;
            dev->blockid++;
            return 0;
        }
    }

    /* Handle write error condition... */

    save_errno = errno;
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "write_tape()", strerror(errno));

        int_scsi_status_update( dev, 0 );
    }
    errno = save_errno;

    if ( STS_NOT_MOUNTED( dev ) )
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    else
    {
        if (errno == EIO)
        {
            if(STS_EOT(dev))
                build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            else
                build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        }
        else
            build_senseX(TAPE_BSENSE_ITFERROR,dev,unitstat,code);
    }

    return -1;

} /* end function write_scsitape */

/*-------------------------------------------------------------------*/
/* Write a tapemark to a SCSI tape device                            */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int write_scsimark (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int  rc, save_errno;

    /* Write tape mark to SCSI tape */

    rc = int_write_scsimark( dev );

#if defined( _MSVC_ )
    if (errno == ENOSPC)
        dev->eotwarning = 1;
#endif

    if (rc >= 0)
        return 0;

    /*         LINUX EOM BEHAVIOUR WHEN WRITING

      When the end of medium early warning is encountered,
      the current write is finished and the number of bytes
      is returned. The next write returns -1 and errno is
      set to ENOSPC. To enable writing a trailer, the next
      write is allowed to proceed and, if successful, the
      number of bytes is returned. After this, -1 and the
      number of bytes are alternately returned until the
      physical end of medium (or some other error) occurs.
    */

    if (errno == ENOSPC)
    {
        int_scsi_status_update( dev, 0 );

        if (int_write_scsimark( dev ) >= 0)
        {
            dev->eotwarning = 1;
            return 0;
        }
    }

    /* Handle write error condition... */

    save_errno = errno;
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "write_scsimark()", strerror(errno));

        int_scsi_status_update( dev, 0 );
    }
    errno = save_errno;

    if ( STS_NOT_MOUNTED( dev ) )
    {
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    }
    else
    {
        switch(errno)
        {
        case EIO:
            if(STS_EOT(dev))
                build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            else
                build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
            break;
        case ENOSPC:
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            break;
        default:
            build_senseX(TAPE_BSENSE_ITFERROR,dev,unitstat,code);
            break;
        }
    }

    return -1;

} /* end function write_scsimark */

/*-------------------------------------------------------------------*/
/* (internal 'write_scsimark' helper function)                       */
/*-------------------------------------------------------------------*/
int int_write_scsimark (DEVBLK *dev)        // (internal function)
{
int  rc;
struct mtop opblk;

    opblk.mt_op    = MTWEOF;
    opblk.mt_count = 1;

    rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

    if (rc >= 0)
    {
        /* Increment current file number since tapemark was written */
        /*dev->curfilen++;*/ /* (CCW processor handles this automatically
                             so there's no need for us to do it here) */

        /* (tapemarks count as block identifiers too!) */
        dev->blockid++;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Synchronize a SCSI tape device   (i.e. commit its data to tape)   */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int sync_scsitape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int  rc;
int  save_errno;
struct mtop opblk;

    /*
        GA32-0566-02 ("IBM Tape Device Drivers - Programming
        Reference"):

        STIOCQRYPOS

        "[...] A write filemark of count 0 is always issued to
         the drive, which flushes all data from the buffers to
         the tape media. After the write filemark completes, the
         query is issued."

        Write Tapemark

        "[...] The WriteTapemark entry point may also be called
         with the dwTapemarkCount parameter set to 0 and the
         bImmediate parameter set to FALSE. This has the effect
         of committing any uncommitted data written by previous
         WriteFile calls ... to the media."
    */

    opblk.mt_op    = MTWEOF;
    opblk.mt_count = 0;             // (zero to force a commit)

    if ((rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk)) >= 0)
    {
#if defined( _MSVC_ )
        if (errno == ENOSPC)
            dev->eotwarning = 1;
#endif
        return 0;       // (success)
    }

    /*         LINUX EOM BEHAVIOUR WHEN WRITING

      When the end of medium early warning is encountered,
      the current write is finished and the number of bytes
      is returned. The next write returns -1 and errno is
      set to ENOSPC. To enable writing a trailer, the next
      write is allowed to proceed and, if successful, the
      number of bytes is returned. After this, -1 and the
      number of bytes are alternately returned until the
      physical end of medium (or some other error) occurs.
    */

    if (errno == ENOSPC)
    {
        int_scsi_status_update( dev, 0 );

        opblk.mt_op    = MTWEOF;
        opblk.mt_count = 0;         // (zero to force a commit)

        if ((rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk)) >= 0)
        {
            dev->eotwarning = 1;
            return 0;
        }
    }

    /* Handle write error condition... */

    save_errno = errno;
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
            dev->filename, "scsi", "ioctl_tape(MTWEOF)", strerror(errno));

        int_scsi_status_update( dev, 0 );
    }
    errno = save_errno;

    if ( STS_NOT_MOUNTED( dev ) )
    {
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    }
    else
    {
        switch(errno)
        {
        case EIO:
            if(STS_EOT(dev))
                build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            else
                build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
            break;
        case ENOSPC:
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            break;
        default:
            build_senseX(TAPE_BSENSE_ITFERROR,dev,unitstat,code);
            break;
        }
    }

    return -1;

} /* end function sync_scsitape */

/*-------------------------------------------------------------------*/
/* Forward space over next block of SCSI tape device                 */
/*                                                                   */
/* If successful, return value is +1.                                */
/* If the block skipped was a tapemark, the return value is zero,    */
/* and the current file number in the device block is incremented.   */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int fsb_scsitape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int  rc;
int  save_errno;
struct mtop opblk;

    /* Forward space block on SCSI tape */

    opblk.mt_op    = MTFSR;
    opblk.mt_count = 1;

    rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

    if ( rc >= 0 )
    {
        dev->blockid++;
        /* Return +1 to indicate forward space successful */
        return +1;
    }

    /* Check for spacing over a tapemark... */

    save_errno = errno;
    {
        int_scsi_status_update( dev, 0 );
    }
    errno = save_errno;

    // PROGRAMMING NOTE: please see the "Programming Note" in the
    // 'bsb_scsitape' function regarding usage of the 'EOF' status
    // to detect spacing over tapemarks.

    if ( EIO == errno && STS_EOF(dev) ) // (fwd-spaced over tapemark?)
    {
        dev->curfilen++;
        dev->blockid++;
        /* Return 0 to indicate tapemark was spaced over */
        return 0;
    }

    /* Bonafide forward space block error ... */

    save_errno = errno;
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "ioctl_tape(MTFSR)", strerror(errno));
    }
    errno = save_errno;

    if ( STS_NOT_MOUNTED( dev ) )
    {
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    }
    else
    {
        switch(errno)
        {
        case EIO:
            if(STS_EOT(dev))
                build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            else
                build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
            break;
        case ENOSPC:
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            break;
        default:
            build_senseX(TAPE_BSENSE_ITFERROR,dev,unitstat,code);
            break;
        }
    }

    return -1;

} /* end function fsb_scsitape */

/*-------------------------------------------------------------------*/
/* Backspace to previous block of SCSI tape device                   */
/*                                                                   */
/* If successful, return value is +1.                                */
/* If the block is a tapemark, the return value is zero,             */
/* and the current file number in the device block is decremented.   */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int bsb_scsitape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int  rc;
int  save_errno;
struct mtop opblk;
struct mtget starting_mtget;

    /* PROGRAMMING NOTE: There is currently no way to distinguish
    ** between a "normal" backspace-block error and a "backspaced-
    ** into-loadpoint" i/o error, since the only error indication
    ** we get [in response to a backspace block attempt] is simply
    ** 'EIO'. (Interrogating the status AFTER the fact (to see if
    ** we're positioned at loadpoint) doesn't tell us whether we
    ** were already positioned at loadpoint *before* the error was
    ** was encountered or whether we're only positioned at load-
    ** point because we *did* in fact backspace over the very first
    ** block on the tape (and are thus now, after the fact, sitting
    ** at loadpoint because we *did* backspace over a block but it
    ** just got an error for some reason).
    **
    ** Thus, we have absolutely no choice here but to retrieve the
    ** status BEFORE we attempt the i/o to see if we're ALREADY at
    ** loadpoint. If we are, then we immediately return an error
    ** ("backspaced-into-loadpoint") *without* even attemting the
    ** i/o at all. If we're *not* already sitting at loadpoint how-
    ** ever, then we go ahead an attempt the i/o and then check for
    ** an error afterwards.
    */

    /* Obtain tape status before backward space... */
    int_scsi_status_update( dev, 0 );

    /* (save the current status before the i/o in case of error) */
    memcpy( &starting_mtget, &dev->mtget, sizeof( struct mtget ) );

    /* Unit check if already at start of tape */
    if ( STS_BOT( dev ) )
    {
        dev->eotwarning = 0;
        build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        return -1;
    }

    /* Attempt the backspace i/o...*/
    opblk.mt_op    = MTBSR;
    opblk.mt_count = 1;

    rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

    if ( rc >= 0 )
    {
        dev->blockid--;
        /* Return +1 to indicate backspace successful */
        return +1;
    }

    /* Retrieve new status after the [supposed] i/o error... */
    save_errno = errno;
    {
        int_scsi_status_update( dev, 0 );
    }
    errno = save_errno;

    /* Check for backspacing over tapemark... */

    /* PROGRAMMING NOTE: on Windows, our scsi tape driver (w32stape.c)
    ** sets 'EOF' status whenever a tapemark is spaced over in EITHER
    ** direction (forward OR backward), whereas *nix operating systems
    ** do not. They set 'EOF' status only when FORWARD spacing over a
    ** tapemark but not when BACKSPACING over one.
    **
    ** (Apparently the EOF status was actually meant to mean that the
    ** tape is "PHYSICALLY POSITIONED PAST [physical] eof" (i.e. past
    ** an "eof marker" (i.e. a tapemark)) and nothing more. That is to
    ** say, it is apparently NOT meant to mean a tapemark was passed
    ** over, but rather only that you're "POSITIONED PAST" a tapemark.)
    **
    ** Therefore since 'EOF' status will thus *NEVER* be set whenever
    ** a tapemark is spaced over in the *BACKWARD* direction [on non-
    ** Windows operating systems], we need some other means of distin-
    ** guishing between true backspace-block i/o errors and ordinary
    ** spacing over a tapemark (which is NOT an i/o error but which
    ** *is* an "out of the ordinary" (unit exception) type of event).
    **
    ** Extensive research on this issue has revealed the *ONLY* semi-
    ** reliable means of distinguishing between them is by checking
    ** the "file#" and "block#" fields of the status structure after
    ** the supposed i/o error. If the file# is one less than it was
    ** before and the block# is -1, then a tapemark was simply spaced
    ** over. If the file# and block# is anything else however, then
    ** the originally reported error was a bonafide i/o error (i.e.
    ** the original backspace-block (MTBSR) actually *failed*).
    **
    ** I say "semi-reliable" because comments seem to indicate that
    ** the "file#" and "block#" fields of the mtget status structure
    ** "are not always used". The best that I can tell however, is
    ** most *nix operating systems *do* seem to maintain them. Thus,
    ** for now, we're going to rely on their accuracy since without
    ** them there's really no way whatsoever to distingish between
    ** a normal backspacing over a tapemark unit exception condition
    ** and a bonafide i/o error (other than doing our own SCSI i/o
    ** of course (which we don't support (yet))). -- Fish, May 2008
    */
    if ( EIO == errno )
    {
#if defined( _MSVC_ )

        /* Windows always sets 'EOF' status whenever a tapemark is
           spaced over in EITHER direction (forward OR backward) */

        if ( STS_EOF(dev) )     /* (passed over tapemark?) */

#else // !defined( _MSVC_ )

        /* Unix-type systems unfortunately do NOT set 'EOF' whenever
           backspacing over a tapemark (see PROGRAMMING NOTE above),
           so we need to check the status struct's file# and block#
           fields instead... */

        /* (passed over tapemark?) */
        if (1
            && dev->mtget.mt_fileno == (starting_mtget.mt_fileno - 1)
            && dev->mtget.mt_blkno == -1
        )
#endif // defined( _MSVC_ )
        {
            dev->curfilen--;
            dev->blockid--;
            /* Return 0 to indicate tapemark was spaced over */
            return 0;
        }
    }

    /* Bonafide backspace block i/o error ... */
    save_errno = errno;
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "ioctl_tape(MTBSR)", strerror(errno));
    }
    errno = save_errno;

    if ( STS_NOT_MOUNTED( dev ) )
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    else
    {
        if ( EIO == errno && STS_BOT(dev) )
        {
            dev->eotwarning = 0;
            build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        }
        else
            build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
    }

    return -1;

} /* end function bsb_scsitape */

/*-------------------------------------------------------------------*/
/* Forward space to next file of SCSI tape device                    */
/*                                                                   */
/* If successful, the return value is zero, and the current file     */
/* number in the device block is incremented.                        */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int fsf_scsitape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int  rc;
int  save_errno;
struct mtop opblk;

    /* Forward space file on SCSI tape */

    opblk.mt_op    = MTFSF;
    opblk.mt_count = 1;

    rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

    /* Since we have no idea how many blocks we've skipped over
       (as a result of doing the forward-space file), we now have
       no clue as to what the proper current blockid should be.
    */
    dev->blockid = -1;      // (actual position now unknown!)

    if ( rc >= 0 )
    {
        dev->curfilen++;
        return 0;
    }

    /* Handle error condition */
    if (!SVF_ENABLED())
        dev->fenced = 1;    // (actual position now unknown!)

    save_errno = errno;
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "ioctl_tape(MTFSF)", strerror(errno));
    }
    errno = save_errno;

    if ( STS_NOT_MOUNTED( dev ) )
    {
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    }
    else
    {
        switch(errno)
        {
        case EIO:
            if(STS_EOT(dev))
                build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            else
                build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
            break;
        case ENOSPC:
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            break;
        default:
            build_senseX(TAPE_BSENSE_ITFERROR,dev,unitstat,code);
            break;
        }
    }

    return -1;

} /* end function fsf_scsitape */

/*-------------------------------------------------------------------*/
/* Backspace to previous file of SCSI tape device                    */
/*                                                                   */
/* If successful, the return value is zero, and the current file     */
/* number in the device block is decremented.                        */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int bsf_scsitape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int  rc;
int  save_errno;
struct mtop opblk;

    /* PROGRAMMING NOTE: There is currently no way to distinguish
    ** between a "normal" backspace-file error and a "backspaced-
    ** into-loadpoint" i/o error, since the only error indication
    ** we get [in response to a backspace file attempt] is simply
    ** 'EIO'. (Interrogating the status AFTER the fact (to see if
    ** we're positioned at loadpoint) doesn't tell us whether we
    ** were already positioned at loadpoint *before* the error was
    ** was encountered or whether we're only positioned ar load-
    ** point because we *did* in fact backspace over a BOT tape-
    ** mark on the tape (and are thus now, after the fact, sitting
    ** at loadpoint because we *did* backspace over a tape-mark
    ** but it just got an error for some reason).
    **
    ** Thus, we have absolutely no choice here but to retrieve the
    ** status BEFORE we attempt the i/o to see if we're ALREADY at
    ** loadpoint. If we are, then we immediately return an error
    ** ("backspaced-into-loadpoint") *without* even attemting the
    ** i/o at all. If we're *not* already sitting at loadpoint how-
    ** ever, then we go ahead an attempt the i/o and then check for
    ** an error afterwards.
    */

    /* Obtain tape status before backward space... (no choice!) */
    int_scsi_status_update( dev, 0 );

    /* Unit check if already at start of tape */
    if ( STS_BOT( dev ) )
    {
        dev->eotwarning = 0;
        build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        return -1;
    }

    /* Attempt the backspace i/o...*/

    opblk.mt_op    = MTBSF;
    opblk.mt_count = 1;

    rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

    /* Since we have no idea how many blocks we've skipped over
       (as a result of doing the back-space file), we now have
       no clue as to what the proper current blockid should be.
    */
    dev->blockid = -1;      // (actual position now unknown!)

    if ( rc >= 0 )
    {
        dev->curfilen--;
        return 0;
    }

    /* Handle error condition */
    if (!SVF_ENABLED())
        dev->fenced = 1;    // (actual position now unknown!)

    save_errno = errno;
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "ioctl_tape(MTBSF)", strerror(errno));
    }
    errno = save_errno;

    if ( STS_NOT_MOUNTED( dev ) )
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    else
    {
        if ( EIO == errno && STS_BOT(dev) )
        {
            dev->eotwarning = 0;
            build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        }
        else
            build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
    }

    return -1;

} /* end function bsf_scsitape */

/*-------------------------------------------------------------------*/
/* Rewind an SCSI tape device                                        */
/*-------------------------------------------------------------------*/
int rewind_scsitape(DEVBLK *dev,BYTE *unitstat,BYTE code)
{
int rc;
/* int  save_errno; */
struct mtop opblk;

//  opblk.mt_op    = MTLOAD;    // (not sure which is more correct)
    opblk.mt_op    = MTREW;
    opblk.mt_count = 1;

    rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

    if ( rc >= 0 )
    {
        dev->sstat |= GMT_BOT( -1 );  // (forced)
        dev->blockid = 0;
        dev->curfilen = 0;
        dev->fenced = 0;
        return 0;
    }

    if (!SVF_ENABLED())
        dev->fenced = 1;    // (because the rewind failed)
    dev->blockid  = -1;     // (because the rewind failed)
    dev->curfilen = -1;     // (because the rewind failed)

    // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
    WRMSG (HHC00205, "E", LCSS_DEVNUM,
            dev->filename, "scsi", "ioctl_tape(MTREW)", strerror(errno));

    if ( STS_NOT_MOUNTED( dev ) )
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    else
        build_senseX(TAPE_BSENSE_REWINDFAILED,dev,unitstat,code);

    return -1;

} /* end function rewind_scsitape */

/*-------------------------------------------------------------------*/
/* Rewind Unload a SCSI tape device (and CLOSE it too!)              */
/*-------------------------------------------------------------------*/
void int_scsi_rewind_unload(DEVBLK *dev, BYTE *unitstat, BYTE code )
{
int rc;
struct mtop opblk;

//  opblk.mt_op    = MTUNLOAD;  // (not sure which is more correct)
    opblk.mt_op    = MTOFFL;
    opblk.mt_count = 1;

    rc = ioctl_tape (dev->fd, MTIOCTOP, (char*)&opblk);

    if ( rc >= 0 )
    {
        dev->fenced = 0;

        if (dev->ccwtrace)
            // "%1d:%04X Tape file %s, type %s: tape unloaded"
            WRMSG (HHC00210, "I", LCSS_DEVNUM, dev->filename, "scsi");

        // PR# tape/88: no sense with 'close_scsitape'
        // attempting a rewind if the tape is unloaded!

        dev->stape_close_rewinds = 0;   // (skip rewind attempt)

        close_scsitape( dev ); // (required for REW UNLD)
        return;
    }

    if (!SVF_ENABLED())
        dev->fenced = 1;// (because the rewind-unload failed)
    dev->curfilen = -1; // (because the rewind-unload failed)
    dev->blockid  = -1; // (because the rewind-unload failed)

    // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
    WRMSG (HHC00205, "E", LCSS_DEVNUM,
            dev->filename, "scsi", "ioctl_tape(MTOFFL)", strerror( errno ) );

    if ( STS_NOT_MOUNTED( dev ) )
        build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
    else
        build_senseX(TAPE_BSENSE_REWINDFAILED,dev,unitstat,code);

} /* end function int_scsi_rewind_unload */

/*-------------------------------------------------------------------*/
/* Erase Gap                                                         */
/*-------------------------------------------------------------------*/
int erg_scsitape( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
#if defined( OPTION_SCSI_ERASE_GAP )
int rc;

    if (!dev->stape_no_erg)
    {
        struct mtop opblk;

        opblk.mt_op    = MTERASE;
        opblk.mt_count = 0;         // (zero means "short" erase-gap)

        rc = ioctl_tape( dev->fd, MTIOCTOP, (char*)&opblk );

#if defined( _MSVC_ )
        if (errno == ENOSPC)
            dev->eotwarning = 1;
#endif

        if ( rc < 0 )
        {
            /*         LINUX EOM BEHAVIOUR WHEN WRITING

              When the end of medium early warning is encountered,
              the current write is finished and the number of bytes
              is returned. The next write returns -1 and errno is
              set to ENOSPC. To enable writing a trailer, the next
              write is allowed to proceed and, if successful, the
              number of bytes is returned. After this, -1 and the
              number of bytes are alternately returned until the
              physical end of medium (or some other error) occurs.
            */

            if (errno == ENOSPC)
            {
                int_scsi_status_update( dev, 0 );

                opblk.mt_op    = MTERASE;
                opblk.mt_count = 0;         // (zero means "short" erase-gap)

                if ( (rc = ioctl_tape( dev->fd, MTIOCTOP, (char*)&opblk )) >= 0 )
                    dev->eotwarning = 1;
            }

            if ( rc < 0)
            {
                // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
                WRMSG (HHC00205, "E", LCSS_DEVNUM,
                        dev->filename, "scsi", "ioctl_tape(MTERASE)", strerror(errno));
                build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
                return -1;
            }
        }
    }

    return 0;       // (success)

#else // !defined( OPTION_SCSI_ERASE_GAP )

    UNREFERENCED ( dev );
    UNREFERENCED ( code );
    UNREFERENCED ( unitstat );

    return 0;       // (treat as nop)

#endif // defined( OPTION_SCSI_ERASE_GAP )

} /* end function erg_scsitape */

/*-------------------------------------------------------------------*/
/* Data Security Erase                                               */
/*-------------------------------------------------------------------*/
int dse_scsitape( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
#if defined( OPTION_SCSI_ERASE_TAPE )

    struct mtop opblk;

    opblk.mt_op    = MTERASE;
    opblk.mt_count = 1;         // (one means "long" erase-tape)

    if ( ioctl_tape( dev->fd, MTIOCTOP, (char*)&opblk ) < 0 )
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM,
                dev->filename, "scsi", "ioctl_tape(MTERASE)", strerror(errno));
        build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        return -1;
    }

    return 0;       // (success)

#else // !defined( OPTION_SCSI_ERASE_TAPE )

    UNREFERENCED ( dev );
    UNREFERENCED ( code );
    UNREFERENCED ( unitstat );

    return 0;       // (treat as nop)

#endif // defined( OPTION_SCSI_ERASE_TAPE )

} /* end function dse_scsitape */

/*-------------------------------------------------------------------*/
/*                   readblkid_scsitape                              */
/*-------------------------------------------------------------------*/
/* Output values are returned in BIG-ENDIAN guest format...          */
/*-------------------------------------------------------------------*/
int readblkid_scsitape ( DEVBLK* dev, BYTE* logical, BYTE* physical )
{
    // ZZ FIXME: The two blockid fields that READ BLOCK ID
    // are returning are the "Channel block ID" and "Device
    // block ID" fields, which correspond directly to the
    // SCSI "First block location" and "Last block location"
    // fields (as returned by a READ POSITION scsi command),
    // so we really SHOULD be doing our own direct scsi i/o
    // for ourselves so we can retrieve BOTH of those values
    // directly from the real/actual physical device itself,
    // but until we can add code to Herc to do that, we must
    // return the same value for each since ioctl(MTIOCPOS)
    // only returns us one value (the logical position) and
    // not both that we really prefer...

    // (And for the record, we want the "Channel block ID"
    // value, also known as the SCSI "First block location"
    // value, also known as the >>LOGICAL<< value and *NOT*
    // the absolute/physical device-relative value)

    struct  mtpos  mtpos;
    BYTE    blockid[4];

    if (ioctl_tape( dev->fd, MTIOCPOS, (char*) &mtpos ) < 0 )
    {
        /* Informative ERROR message if tracing */

        int save_errno = errno;
        {
            // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
            if (dev->ccwtrace)
                WRMSG(HHC90205, "D"
                    ,SSID_TO_LCSS(dev->ssid)
                    ,dev->devnum
                    ,dev->filename
                    ,"scsi", "ioctl_tape(MTTELL)"
                    ,strerror(errno)
                    );
        }
        errno = save_errno;

        return  -1;     // (errno should already be set)
    }

    // Convert MTIOCPOS value to guest BIG-ENDIAN format...

    mtpos.mt_blkno = CSWAP32( mtpos.mt_blkno );     // (guest <- host)

    // Handle emulated vs. physical tape-device block-id format issue...

    blockid_actual_to_emulated( dev, (BYTE*)&mtpos.mt_blkno, blockid );

    // Until we can add code to Herc to do direct SCSI i/o (so that
    // we can retrieve BOTH values directly from the device itself),
    // we have no choice but to return the same value for each since
    // the ioctl(MTIOCPOS) call only returns the logical value and
    // not also the physical value that we wish it would...

    if (logical)  memcpy( logical,  &blockid[0], 4 );
    if (physical) memcpy( physical, &blockid[0], 4 );

    return 0;       // (success)

} /* end function readblkid_scsitape */

/*-------------------------------------------------------------------*/
/*                    locateblk_scsitape                             */
/*-------------------------------------------------------------------*/
/* Input value is passed in little-endian host format...             */
/*-------------------------------------------------------------------*/
int locateblk_scsitape ( DEVBLK* dev, U32 blockid, BYTE *unitstat, BYTE code )
{
    int rc;
    struct mtop  mtop;

    UNREFERENCED( unitstat );                   // (not used)
    UNREFERENCED(   code   );                   // (not used)

    // Convert the passed host-format blockid value into the proper
    // 32-bit vs. 22-bit guest-format the physical device expects ...

    blockid = CSWAP32( blockid );               // (guest <- host)

    blockid_emulated_to_actual( dev, (BYTE*)&blockid, (BYTE*)&mtop.mt_count );

    mtop.mt_count = CSWAP32( mtop.mt_count );   // (host <- guest)
    mtop.mt_op    = MTSEEK;

    // Ask the actual hardware to do an actual physical locate...

    if ((rc = ioctl_tape( dev->fd, MTIOCTOP, (char*)&mtop )) < 0)
    {
        int save_errno = errno;
        {
            // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
            if (dev->ccwtrace)
                WRMSG(HHC00205, "W"
                    ,SSID_TO_LCSS(dev->ssid)
                    ,dev->devnum
                    ,dev->filename
                    ,"scsi", "ioctl_tape(MTSEEK)"
                    ,strerror(errno)
                    );
        }
        errno = save_errno;
    }

    return rc;
}

/*********************************************************************/
/**                                                                 **/
/**                BLOCK-ID ADJUSTMENT FUNCTIONS                    **/
/**                                                                 **/
/*********************************************************************/
/*
    The following conversion functions compensate for the fact that
    the emulated device type might actually be completely different
    from the actual real [SCSI] device being used for the emulation.

    That is to say, the actual SCSI device being used may actually
    be a 3590 type device but is defined in Hercules as a 3480 (or
    vice-versa). Thus while the device actually behaves as a 3590,
    we need to emulate 3480 functionality instead (and vice-versa).

    For 3480/3490 devices, the block ID has the following format:

     __________ ________________________________________________
    | Bit      | Description                                    |
    |__________|________________________________________________|
    | 0        | Direction Bit                                  |
    |          |                                                |
    |          | 0      Wrap 1                                  |
    |          | 1      Wrap 2                                  |
    |__________|________________________________________________|
    | 1-7      | Segment Number                                 |
    |__________|________________________________________________|
    | 8-9      | Format Mode                                    |
    |          |                                                |
    |          | 00     3480 format                             |
    |          | 01     3480-2 XF format                        |
    |          | 10     3480 XF format                          |
    |          | 11     Reserved                                |
    |          |                                                |
    |          | Note:  The 3480 format does not support IDRC.  |
    |__________|________________________________________________|
    | 10-31    | Logical Block Number                           |
    |__________|________________________________________________|

    For 3480's and 3490's, first block recorded on the tape has
    a block ID value of X'01000000', whereas for 3590 devices the
    block ID is a full 32 bits and the first block on the tape is
    block ID x'00000000'.

    For the 32-bit to 22-bit (and vice versa) conversion, we're
    relying on (hoping really!) that an actual 32-bit block-id value
    will never actually exceed 30 bits (1-bit wrap + 7-bit segment#
    + 22-bit block-id) since we perform the conversion by simply
    splitting the low-order 30 bits of a 32-bit block-id into a sep-
    arate 8-bit (wrap and segment#) and 22-bit (block-id) fields,
    and then shifting them into their appropriate position (and of
    course combining/appending them for the opposite conversion).

    As such, this of course implies that we are thus treating the
    wrap bit and 7-bit segment number values of a 3480/3490 "22-bit
    format" blockid as simply the high-order 8 bits of an actual
    30-bit physical blockid (which may or may not work properly on
    actual SCSI hardware depending on how[*] it handles inaccurate
    blockid values).


    -----------------

 [*]  Most(?) [SCSI] devices treat the blockid value used in a
    Locate CCW as simply an "approximate location" of where the
    block in question actually resides on the physical tape, and
    will, after positioning itself to the *approximate* physical
    location of where the block is *believed* to reside, proceed
    to then perform the final positioning at low-speed based on
    its reading of its actual internally-recorded blockid values.

    Thus, even when the supplied Locate block-id value is wrong,
    the Locate should still succeed, albeit less efficiently since
    it may be starting at a physical position quite distant from
    where the actual block is actually physically located on the
    actual media.
*/

/*-------------------------------------------------------------------*/
/*                  blockid_emulated_to_actual                       */
/*-------------------------------------------------------------------*/
/* Locate CCW helper: convert guest-supplied 3480 or 3590 blockid    */
/*                    to the actual SCSI hardware blockid format     */
/* Both I/P AND O/P are presumed to be in BIG-ENDIAN guest format    */
/*-------------------------------------------------------------------*/
void blockid_emulated_to_actual
(
    DEVBLK  *dev,           // ptr to Hercules device
    BYTE    *emu_blkid,     // ptr to i/p 4-byte block-id in guest storage
    BYTE    *act_blkid      // ptr to o/p 4-byte block-id for actual SCSI i/o
)
{
    if ( TAPEDEVT_SCSITAPE != dev->tapedevt )
    {
        memcpy( act_blkid, emu_blkid, 4 );
        return;
    }

#if defined(OPTION_SCSI_TAPE)
    if (0x3590 == dev->devtype)
    {
        // 3590 being emulated; guest block-id is full 32-bits...

        if (dev->stape_blkid_32)
        {
            // SCSI using full 32-bit block-ids too. Just copy as-is...

            memcpy( act_blkid, emu_blkid, 4 );
        }
        else
        {
            // SCSI using 22-bit block-ids. Use low-order 30 bits
            // of 32-bit guest-supplied blockid and convert it
            // into a "22-bit format" blockid value for SCSI...

            blockid_32_to_22 ( emu_blkid, act_blkid );
        }
    }
    else // non-3590 being emulated; guest block-id is 22-bits...
    {
        if (dev->stape_blkid_32)
        {
            // SCSI using full 32-bit block-ids. Extract the wrap,
            // segment# and 22-bit blockid bits from the "22-bit
            // format" guest-supplied blockid value and combine
            // (append) them into a contiguous low-order 30 bits
            // of a 32-bit blockid value for SCSI to use...

            blockid_22_to_32 ( emu_blkid, act_blkid );
        }
        else
        {
            // SCSI using 22-bit block-ids too. Just copy as-is...

            memcpy( act_blkid, emu_blkid, 4 );
        }
    }
#endif /* defined(OPTION_SCSI_TAPE) */

} /* end function blockid_emulated_to_actual */

/*-------------------------------------------------------------------*/
/*                  blockid_actual_to_emulated                       */
/*-------------------------------------------------------------------*/
/* Read Block Id CCW helper:  convert an actual SCSI block-id        */
/*                            to guest emulated 3480/3590 format     */
/* Both i/p and o/p are presumed to be in big-endian guest format    */
/*-------------------------------------------------------------------*/
void blockid_actual_to_emulated
(
    DEVBLK  *dev,           // ptr to Hercules device (for 'devtype')
    BYTE    *act_blkid,     // ptr to i/p 4-byte block-id from actual SCSI i/o
    BYTE    *emu_blkid      // ptr to o/p 4-byte block-id in guest storage
)
{
    if ( TAPEDEVT_SCSITAPE != dev->tapedevt )
    {
        memcpy( emu_blkid, act_blkid, 4 );
        return;
    }

#if defined(OPTION_SCSI_TAPE)
    if (dev->stape_blkid_32)
    {
        // SCSI using full 32-bit block-ids...
        if (0x3590 == dev->devtype)
        {
            // Emulated device is a 3590 too. Just copy as-is...
            memcpy( emu_blkid, act_blkid, 4 );
        }
        else
        {
            // Emulated device using 22-bit format. Convert...
            blockid_32_to_22 ( act_blkid, emu_blkid );
        }
    }
    else
    {
        // SCSI using 22-bit format block-ids...
        if (0x3590 == dev->devtype)
        {
            // Emulated device using full 32-bit format. Convert...
            blockid_22_to_32 ( act_blkid, emu_blkid );
        }
        else
        {
            // Emulated device using 22-bit format too. Just copy as-is...
            memcpy( emu_blkid, act_blkid, 4 );
        }
    }
#endif /* defined(OPTION_SCSI_TAPE) */

} /* end function blockid_actual_to_emulated */

/*-------------------------------------------------------------------*/
/*                     blockid_32_to_22                              */
/*-------------------------------------------------------------------*/
/* Convert a 3590 32-bit blockid into 3480 "22-bit format" blockid   */
/* Both i/p and o/p are presumed to be in big-endian guest format    */
/*-------------------------------------------------------------------*/
void blockid_32_to_22 ( BYTE *in_32blkid, BYTE *out_22blkid )
{
    out_22blkid[0] = ((in_32blkid[0] << 2) & 0xFC) | ((in_32blkid[1] >> 6) & 0x03);
    out_22blkid[1] = in_32blkid[1] & 0x3F;
    out_22blkid[2] = in_32blkid[2];
    out_22blkid[3] = in_32blkid[3];
}

/*-------------------------------------------------------------------*/
/*                     blockid_22_to_32                              */
/*-------------------------------------------------------------------*/
/* Convert a 3480 "22-bit format" blockid into a 3590 32-bit blockid */
/* Both i/p and o/p are presumed to be in big-endian guest format    */
/*-------------------------------------------------------------------*/
void blockid_22_to_32 ( BYTE *in_22blkid, BYTE *out_32blkid )
{
    out_32blkid[0] = (in_22blkid[0] >> 2) & 0x3F;
    out_32blkid[1] = ((in_22blkid[0] << 6) & 0xC0) | (in_22blkid[1] & 0x3F);
    out_32blkid[2] = in_22blkid[2];
    out_32blkid[3] = in_22blkid[3];
}

/*********************************************************************/
/**                                                                 **/
/**           INTERNAL STATUS & AUTOMOUNT FUNCTIONS                 **/
/**                                                                 **/
/*********************************************************************/

/* Forward references...                                             */
static int   int_scsi_status_wait     ( DEVBLK* dev, int usecs );
static void* get_stape_status_thread  ( void* notused );

/*-------------------------------------------------------------------*/
/* get_stape_status_thread                                           */
/*-------------------------------------------------------------------*/
static void* get_stape_status_thread( void* notused )
{
    LIST_ENTRY*   pListEntry;
    STSTATRQ*     req;
    DEVBLK*       dev = NULL;
    struct mtget  mtget;
    int           timeout;
    int           rc;

    UNREFERENCED( notused );

    // PROGRAMMING NOTE: it is EXTREMELY IMPORTANT that the status-
    // retrieval thread (i.e. ourselves) be set to a priority that
    // is HIGHER than what the device-threads are currently set to
    // in order to prevent their request for new/updated status from
    // erroneously timing out (thereby misleading them to mistakenly
    // believe no tape is mounted when in acuality there is!). The
    // issue is, the caller only waits for so long for us to return
    // the status to them so we better ensure we return it to them
    // in a timely fashion else they be mislead to believe there's
    // no tape mounted (since, by virtue of their request having
    // timed out, they presume no tape is mounted since the retrieval
    // took too long (which only occurs whenever (duh!) there's no
    // tape mounted!)). Thus, if there *is* a tape mounted, we better
    // be DARN sure to return them the status as quickly as possible
    // in order to prevent their wait from timing out. We ensure this
    // by setting our own priority HIGHER than theirs.

    SET_THREAD_PRIORITY( sysblk.devprio + 1, sysblk.qos_user_interactive );
    UNREFERENCED(rc);

    // "Thread id "TIDPAT", prio %2d, name %s started"
    LOG_THREAD_BEGIN( SCSISTAT_THREAD_NAME  );

    obtain_lock( &sysblk.stape_lock );

    do
    {
        sysblk.stape_getstat_busy = 1;
        broadcast_condition( &sysblk.stape_getstat_cond );

        // Process all work items currently in our queue...

        while (!IsListEmpty( &sysblk.stape_status_link ) && !sysblk.shutdown)
        {
            pListEntry = RemoveListHead( &sysblk.stape_status_link );
            InitializeListLink( pListEntry );
            req = CONTAINING_RECORD( pListEntry, STSTATRQ, link );
            dev = req->dev;

            // Status queries limited GLOBALLY to one per second,
            // since there's no way of knowing whether a drive is
            // on the same or different bus as the other drive(s).

            for
            (
                timeout = 0
                ;
                1
                    && !sysblk.shutdown
                    && sysblk.stape_query_status_tod.tv_sec
                    && !(timeout = timed_wait_condition_relative_usecs
                        (
                            &sysblk.stape_getstat_cond,
                            &sysblk.stape_lock,
                            MAX_GSTAT_FREQ_USECS,
                            &sysblk.stape_query_status_tod
                        ))
                ;
            );

            if (!sysblk.shutdown)
            {
                // Query drive status...

                // Since this may take quite a while to do if there's no tape
                // mounted, we release the lock before attempting to retrieve
                // the status and then re-acquire it afterwards...

                release_lock( &sysblk.stape_lock );
                {
                    define_BOT_pos( dev );  // (always before MTIOCGET)

                    // NOTE: the following may take up to *>10<* seconds to
                    // complete on Windows whenever there's no tape mounted,
                    // but apparently only with certain hardware. On a fast
                    // quad-cpu Windows 2003 Server system with an Adaptec
                    // AHA2944UW SCSI control for example, it completes right
                    // away (i.e. IMMEDIATELY), whereas on a medium dual-proc
                    // Windows 2000 Server system with TEKRAM SCSI controller
                    // it takes  *>> 10 <<*  seconds!...

                    if (0 == ioctl_tape( dev->fd, MTIOCGET, (char*)&mtget ))
                    {
                        memcpy( &dev->mtget, &mtget, sizeof( mtget ));
                    }
                }
                obtain_lock( &sysblk.stape_lock );

                broadcast_condition( &dev->stape_sstat_cond );
                gettimeofday( &sysblk.stape_query_status_tod, NULL );
            }

        } // end while (!IsListEmpty)...

        if (!sysblk.shutdown)
        {
            // Sleep until more/new work arrives...

            sysblk.stape_getstat_busy = 0;
            broadcast_condition( &sysblk.stape_getstat_cond );
            wait_condition( &sysblk.stape_getstat_cond,
                            &sysblk.stape_lock );
        }
    }
    while (!sysblk.shutdown);

    // (discard all work items since we're going away)

    while (!IsListEmpty( &sysblk.stape_status_link ))
    {
        pListEntry = RemoveListHead( &sysblk.stape_status_link );
        InitializeListLink( pListEntry );
    }

    // "Thread id "TIDPAT", prio %2d, name %s ended"
    LOG_THREAD_END( SCSISTAT_THREAD_NAME  );

    sysblk.stape_getstat_busy = 0;
    sysblk.stape_getstat_tid = 0;
    broadcast_condition( &sysblk.stape_getstat_cond );
    release_lock( &sysblk.stape_lock );

    return NULL;

} /* end function get_stape_status_thread */

/*-------------------------------------------------------------------*/
/* int_scsi_status_wait                                              */
/*-------------------------------------------------------------------*/
static
int int_scsi_status_wait( DEVBLK* dev, int usecs )
{
    int rc;

    if (unlikely( dev->fd < 0 ))    // (has drive been opened yet?)
        return -1;                  // (cannot proceed until it is)

    obtain_lock( &sysblk.stape_lock );

    // Create the status retrieval thread if it hasn't been yet.
    // We do the actual retrieval of the status in a worker thread
    // because retrieving the status from a drive that doesn't have
    // a tape mounted may take a long time (at least on Windows).

    if (unlikely( !sysblk.stape_getstat_tid ))
    {
        int rc;
        VERIFY
        (
            (rc = create_thread
            (
                &sysblk.stape_getstat_tid,
                JOINABLE,
                get_stape_status_thread,
                NULL,
                SCSISTAT_THREAD_NAME
            ))
            == 0
        );
        if (rc)
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ));
    }

    // Add our request to its work queue if needed...

    if (!dev->stape_statrq.link.Flink)
    {
        InsertListTail( &sysblk.stape_status_link, &dev->stape_statrq.link );
    }

    // Wake up the status retrieval thread (if needed)...

    if (!sysblk.stape_getstat_busy)
    {
        broadcast_condition( &sysblk.stape_getstat_cond );
    }

    // Wait only so long for the status to be updated...

    rc = timed_wait_condition_relative_usecs
    (
        &dev->stape_sstat_cond,     // ptr to condition to wait on
        &sysblk.stape_lock,         // ptr to controlling lock (must be held!)
        usecs,                      // max #of microseconds to wait
        NULL                        // [OPTIONAL] ptr to tod value (may be NULL)
    );

    release_lock( &sysblk.stape_lock );
    return rc;
} /* end function int_scsi_status_wait */

/*-------------------------------------------------------------------*/
/* Check if a SCSI tape is positioned past the EOT reflector or not  */
/*-------------------------------------------------------------------*/
int passedeot_scsitape( DEVBLK *dev )
{
    return dev->eotwarning;     // (1==past EOT reflector; 0==not)
}

/*-------------------------------------------------------------------*/
/* Determine if the tape is Ready   (tape drive door status)         */
/* Returns:  true/false:  1 = ready,   0 = NOT ready                 */
/*-------------------------------------------------------------------*/
int is_tape_mounted_scsitape( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
    UNREFERENCED(unitstat);
    UNREFERENCED(code);

    /* Update tape mounted status */
    int_scsi_status_update( dev, 1 ); // (safe/fast internal call)

    return ( STS_MOUNTED( dev ) );
} /* end function driveready_scsitape */

/*-------------------------------------------------------------------*/
/* Force a manual status refresh/update      (DANGEROUS!)            */
/*-------------------------------------------------------------------*/
int update_status_scsitape( DEVBLK *dev )   // (external tmh call)
{
    //                  * *  WARNING!  * *

    // PROGRAMMING NOTE: do NOT call this function indiscriminately,
    // as doing so COULD cause improper functioning of the guest o/s!

    // How? Simple: if there's already a tape job running on the guest
    // using the tape drive and we just so happen to request a status
    // update at the precise moment a guest i/o encounters a tapemark,
    // it's possible for US to receive the "tapemark" status and thus
    // cause the guest to end up NOT SEEING the tapemark! Therefore,
    // you should ONLY call this function whenever the current status
    // indicates there's no tape mounted. If the current status says
    // there *is* a tape mounted, you must NOT call this function!

    // If the current status says there's a tape mounted and the user
    // knows this to be untrue (e.g. they manually unloaded it maybe)
    // then to kick off the auto-scsi-mount thread they must manually
    // issue the 'devinit' command themselves. We CANNOT presume that
    // a "mounted" status is bogus. We can ONLY safely presume that a
    // "UNmounted" status may possibly be bogus. Thus we only ask for
    // a status refresh if the current status is "not mounted" but we
    // purposely do NOT force a refresh if the status is "mounted"!!

    if ( STS_NOT_MOUNTED( dev ) )           // (if no tape mounted)
        int_scsi_status_update( dev, 0 );   // (then probably safe)

    return 0;
} /* end function update_status_scsitape */

/*-------------------------------------------------------------------*/
/* Update SCSI tape status (and display it if CCW tracing is active) */
/*-------------------------------------------------------------------*/
void int_scsi_status_update( DEVBLK* dev, int mountstat_only )
{
    create_automount_thread( dev );     // (only if needed of course)

    // PROGRAMMING NOTE: only normal i/o requests (as well as the
    // scsi_tapemountmon_thread thread whenever AUTO_SCSI_MOUNT is
    // enabled and active) ever actually call us with mountstat_only
    // set to zero (in order to update our actual status value).
    //
    // Thus if we're called with a non-zero mountstat_only argument
    // (meaning all the caller is interested in is whether or not
    // there's a tape mounted on the drive (which only the panel
    // and GUI threads normally do (and which they do continuously
    // whenever they do do it!))) then we simply return immediately
    // so as to cause the caller to continue using whatever status
    // happens to already be set for the drive (which should always
    // be accurate).
    //
    // This prevents us from continuously "banging on the drive"
    // asking for the status when in reality the status we already
    // have should already be accurate (since it is updated after
    // every i/o or automatically by the auto-mount thread)

    if (likely(mountstat_only))           // (if only want mount status)
        return;                           // (then current should be ok)

    // Update status...

    if (likely(STS_MOUNTED( dev )))
    {
        // According to our current status value there is a tape mounted,
        // so we should wait for a full/complete/accurate status update,
        // regardless of however long that may take...

        int rc;
        while (ETIMEDOUT == (rc = int_scsi_status_wait( dev,
            MAX_GSTAT_FREQ_USECS + (2 * SLOW_UPDATE_STATUS_TIMEOUT) )))
        {
            if (dev->ccwtrace)
            {
                // "%1d:%04X Tape status retrieval timeout"
                WRMSG( HHC00243, "W", LCSS_DEVNUM );
            }
        }
    }
    else
    {
        // No tape is mounted (or so we believe). Attempt to retrieve
        // an updated tape status value, but if we cannot do so within
        // a reasonable period of time (SLOW_UPDATE_STATUS_TIMEOUT),
        // then continue using whatever our current tape status is...

        int_scsi_status_wait( dev, SLOW_UPDATE_STATUS_TIMEOUT );
    }

    create_automount_thread( dev );     // (in case status changed)

    /* Display tape status if tracing is active */
    if (unlikely( dev->ccwtrace ))
    {
        char sstat_str[ 384 ] = {0};
        gstat2str( (U32) dev->sstat, sstat_str, sizeof( sstat_str ));

        // "%1d:%04X Tape file %s, type scsi status %s, sstat 0x%8.8"PRIX32": %s"
        WRMSG( HHC00211, "I"
            , SSID_TO_LCSS(dev->ssid)
            , dev->devnum
            , ( (dev->filename[0]) ? (dev->filename)  : ("(undefined)") )
            , ( (dev->fd   <   0 ) ? (   "closed"  )  : (   "opened"  ) )
            , (U32) dev->sstat
            , sstat_str
        );

        if ( STS_BOT(dev) )
            dev->eotwarning = 0;
    }

} /* end function int_scsi_status_update */

/*-------------------------------------------------------------------*/
/*                  ASYNCHRONOUS TAPE OPEN                           */
/* SCSI tape tape-mount monitoring thread (monitors for tape mounts) */
/* Auto-started by 'int_scsi_status_update' when it notices there is */
/* no tape mounted on whatever device it's checking the status of,   */
/* or by the ReqAutoMount function for unsatisfied mount requests.   */
/*-------------------------------------------------------------------*/
void create_automount_thread( DEVBLK* dev )
{
    //               AUTO-SCSI-MOUNT
    //
    // If no tape is currently mounted on this device,
    // kick off the tape mount monitoring thread that
    // will monitor for tape mounts (if it doesn't al-
    // ready still exist)...

    obtain_lock( &sysblk.stape_lock );

    // Is scsimount enabled?

    if (likely( sysblk.auto_scsi_mount_secs ))
    {
        // Create thread if needed...

        if (unlikely( !sysblk.stape_mountmon_tid ))
        {
            int rc;
            VERIFY
            (
                (rc = create_thread
                (
                    &sysblk.stape_mountmon_tid,
                    DETACHED,
                    scsi_tapemountmon_thread,
                    NULL,
                    SCSIMOUNT_THREAD_NAME
                ))
                == 0
            );
            if (rc)
                // "Error in function create_thread(): %s"
                WRMSG( HHC00102, "E", strerror( rc ));
        }

        // Enable it for our drive if needed...

        if (STS_NOT_MOUNTED( dev ))
        {
            if (!dev->stape_mntdrq.link.Flink)
            {
                InsertListTail( &sysblk.stape_mount_link, &dev->stape_mntdrq.link );
            }
        }
    }

    release_lock( &sysblk.stape_lock );
}

/*-------------------------------------------------------------------*/
/*                  ASYNCHRONOUS TAPE OPEN                           */
/*-------------------------------------------------------------------*/
/* AUTO_SCSI_MOUNT thread...                                         */
/*-------------------------------------------------------------------*/
void *scsi_tapemountmon_thread( void *notused )
{
    struct timeval  now;
    int             timeout, fd;
    LIST_ENTRY*     pListEntry;
    STMNTDRQ*       req;
    DEVBLK*         dev = NULL;

    UNREFERENCED( notused );

    // "Thread id "TIDPAT", prio %d, name '%s' started"
    LOG_THREAD_BEGIN( SCSIMOUNT_THREAD_NAME  );

    obtain_lock( &sysblk.stape_lock );

    while (sysblk.auto_scsi_mount_secs && !sysblk.shutdown)
    {
        // Wait for automount interval to expire...

        gettimeofday( &now, NULL );

        for
        (
            timeout = 0
            ;
            1
                && !sysblk.shutdown
                && sysblk.auto_scsi_mount_secs
                && !(timeout = timed_wait_condition_relative_usecs
                    (
                        &sysblk.stape_getstat_cond,
                        &sysblk.stape_lock,
                        sysblk.auto_scsi_mount_secs * 1000000,
                        &now
                    ))
            ;
        );

        if (sysblk.auto_scsi_mount_secs && !sysblk.shutdown)
        {
            // Process all work items...

            pListEntry = sysblk.stape_mount_link.Flink;

            while (pListEntry != &sysblk.stape_mount_link)
            {
                req = CONTAINING_RECORD( pListEntry, STMNTDRQ, link );
                dev = req->dev;
                pListEntry  = pListEntry->Flink;

                // Open drive if needed...

                if ((fd = dev->fd) < 0)
                {
                    BYTE ro;

                    fd = int_nbio_scsitape_open( dev->filename, &ro );
                    dev->readonly = ro ? 1 : 0;

                    // Check for successful open

                    if (fd < 0)
                    {
                        // "%1d:%04X Tape file %s, type %s: Error opening: errno=%d: %s"
                        WRMSG( HHC00213, "E", LCSS_DEVNUM,
                                dev->filename, "scsi", errno, strerror( errno ));
                        continue; // (go on to next drive)
                    }

                    define_BOT_pos( dev );  // (always after successful open)
                    dev->fd = fd;           // (so far so good)
                }

                // Retrieve the current status...

                // PLEASE NOTE that we must do this WITHOUT holding the stape_lock
                // since the 'int_scsi_status_update' and sub-functions all expect
                // the lock to NOT be held so that THEY can then attempt to acquire
                // it when needed...

                release_lock( &sysblk.stape_lock );
                {
                    int_scsi_status_update( dev, 0 );
                }
                obtain_lock( &sysblk.stape_lock );

                // (check again for shutdown)

                if (sysblk.shutdown || !sysblk.auto_scsi_mount_secs)
                    break;

                // Has a tape [finally] been mounted yet??

                if (STS_NOT_MOUNTED( dev ))
                {
#if !defined( _MSVC_ )
                    dev->fd = -1;
                    close_tape( fd );
#endif
                    continue; // (go on to next drive)
                }

                // Yes, remove completed work item...

                RemoveListEntry( &dev->stape_mntdrq.link );
                InitializeListLink( &dev->stape_mntdrq.link );

                // Finish the open drive process (set drive to variable length
                // block processing mode, etc)...

                // PLEASE NOTE that we must do this WITHOUT holding the stape_lock
                // since the 'finish_scsitape_open' and sub-functions all expect
                // the lock to NOT be held so that THEY can then attempt to acquire
                // it when needed...

                release_lock( &sysblk.stape_lock );
                {
                    if ( finish_scsitape_open( dev, NULL, 0 ) == 0 )
                    {
                        // Notify the guest that the tape has now been loaded by
                        // presenting an unsolicited device attention interrupt...

                        device_attention( dev, CSW_DE );
                    }
                }
                obtain_lock( &sysblk.stape_lock );

            } // end for (all work items)...

        } // end if (sysblk.auto_scsi_mount_secs && !sysblk.shutdown)

    } // end while (sysblk.auto_scsi_mount_secs && !sysblk.shutdown)

    // (discard all work items since we're going away)

    while (!IsListEmpty( &sysblk.stape_mount_link ))
    {
        pListEntry = RemoveListHead( &sysblk.stape_mount_link );
        InitializeListLink( pListEntry );

        // (remove from the STATUS thread's work queue too!)

        req = CONTAINING_RECORD( pListEntry, STMNTDRQ, link );
        dev = req->dev;

        if (                     dev->stape_statrq.link.Flink) {
            RemoveListEntry(    &dev->stape_statrq.link );
            InitializeListLink( &dev->stape_statrq.link );
        }
    }

    // "Thread id "TIDPAT", prio %d, name '%s' ended"
    LOG_THREAD_END( SCSIMOUNT_THREAD_NAME  );

    sysblk.stape_mountmon_tid = 0;  // (we're going away)
    release_lock( &sysblk.stape_lock );
    return NULL;

} /* end function scsi_tapemountmon_thread */

/*-------------------------------------------------------------------*/
/* Tell driver (if needed) what a BOT position looks like...         */
/*-------------------------------------------------------------------*/
void define_BOT_pos( DEVBLK *dev )
{
#ifdef _MSVC_

    // PROGRAMMING NOTE: Need to tell 'w32stape.c' here the
    // information it needs to detect physical BOT (load-point).

    // This is not normally needed as most drivers determine
    // it for themselves based on the type (manufacturer/model)
    // of tape drive being used, but since I haven't added the
    // code to 'w32stape.c' to do that yet (involves talking
    // directly to the SCSI device itself) we thus, for now,
    // need to pass that information directly to 'w32stape.c'
    // ourselves...

    U32 msk  = 0xFF3FFFFF;      // (3480/3490 default)
    U32 bot  = 0x01000000;      // (3480/3490 default)

    if ( dev->stape_blkid_32 )
    {
        msk  = 0xFFFFFFFF;      // (3590 default)
        bot  = 0x00000000;      // (3590 default)
    }

    VERIFY( 0 == w32_define_BOT( dev->fd, msk, bot ) );

#else
    UNREFERENCED(dev);
#endif // _MSVC_
}
#endif // defined(OPTION_SCSI_TAPE)
