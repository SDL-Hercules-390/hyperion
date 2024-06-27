/* HETTAPE.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules Tape Device Handler for HETTAPE             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Original Author: Leland Lucius                                    */
/* Prime Maintainer: Ivan Warren                                     */
/* Secondary Maintainer: "Fish" (David B. Trout)                     */

/*-------------------------------------------------------------------*/
/* This module contains the HET emulated tape format support.        */
/* The subroutines in this module are called by the general tape     */
/* device handler (tapedev.c) when the tape format is HETTAPE.       */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HETTAPE_C_
#define _HDT3420_DLL_

#include "hercules.h"  /* need Hercules control blocks               */
#include "tapedev.h"   /* Main tape handler header file              */

//#define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
//#include "dbgtrace.h"                   // (Fish: DEBUGGING)

/*-------------------------------------------------------------------*/
/* Open an HET format file                                           */
/*                                                                   */
/* If successful, the het control blk is stored in the device block  */
/* and the return value is zero.  Otherwise the return value is -1.  */
/*                                                                   */
/* NOTE: called with dev->lock held by either hsccmd.c "devinit_cmd" */
/*       or by tapeccws.c "0x4B = SET DIAGNOSE" ccw function.        */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int open_het( DEVBLK* dev, BYTE* unitstat, BYTE code )
{
    int   rc;                           /* Return code               */
    char  pathname[MAX_PATH];           /* file path in host format  */

    /* Check for no tape in drive */

    if (strcmp( dev->filename, TAPE_UNLOADED ) == 0)
    {
        build_senseX( TAPE_BSENSE_TAPEUNLOADED, dev, unitstat, code );
        return -1;
    }

    /* Open the HET file */

    hostpath( pathname, dev->filename, sizeof( pathname ));

    rc = het_open( &dev->hetb, pathname,
                   dev->tdparms.logical_readonly ? HETOPEN_READONLY
                   : sysblk.auto_tape_create ? HETOPEN_CREATE : 0 );

    if (rc >= 0)
    {
        /* Keep file descriptor and handle synchronized */
        dev->fd = dev->hetb->fd;
        dev->fh = dev->hetb->fh;

        if (dev->hetb->writeprotect)
            dev->readonly = 1;

        rc = het_cntl( dev->hetb, HETCNTL_SET | HETCNTL_COMPRESS,
            dev->tdparms.compress );

        if (rc >= 0)
        {
            rc = het_cntl( dev->hetb, HETCNTL_SET | HETCNTL_METHOD,
                dev->tdparms.method );

            if (rc >= 0)
            {
                rc = het_cntl( dev->hetb, HETCNTL_SET | HETCNTL_LEVEL,
                    dev->tdparms.level );

                if (rc >= 0)
                {
                    rc = het_cntl( dev->hetb, HETCNTL_SET | HETCNTL_CHUNKSIZE,
                        dev->tdparms.chksize );
                }
            }
        }
    }

    /* Check for open failure */

    if (rc < 0)
    {
        int save_errno = errno;
        het_close( &dev->hetb );
        dev->fd = -1;
        dev->fh = NULL;
        errno = save_errno;

        {
            char msgbuf[128];
            MSGBUF( msgbuf, "Het error '%s': '%s'",
                het_error( rc ), strerror( errno ));

            // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
            WRMSG( HHC00205, "E", LCSS_DEVNUM,
                    dev->filename, "het", "het_open()", msgbuf );
        }

        STRLCPY( dev->filename, TAPE_UNLOADED );
        build_senseX( TAPE_BSENSE_TAPELOADFAIL, dev, unitstat, code );
        return -1;
    }

    /* Open success */

    if (sysblk.auto_tape_create && dev->hetb->created)
    {
        if (!dev->batch || !dev->quiet)
            // "%1d:%04X Tape file %s, type %s: tape created"
            WRMSG( HHC00235, "I", LCSS_DEVNUM, dev->filename, "het" );
    }
    return 0;

} /* end function open_het */

/*-------------------------------------------------------------------*/
/* Close an HET format file                                          */
/*                                                                   */
/* The HET file is close and all device block fields reinitialized.  */
/*-------------------------------------------------------------------*/
DLL_EXPORT void close_het( DEVBLK* dev )
{
    if (dev->fd >= 0)
    {
        if (!dev->batch || !dev->quiet)
            // "%1d:%04X Tape file %s, type %s: tape closed"
            WRMSG( HHC00201, "I", LCSS_DEVNUM, dev->filename, "het" );
    }

    het_close( &dev->hetb );
    STRLCPY( dev->filename, TAPE_UNLOADED );

    dev->fh      = NULL;
    dev->fd      = -1;
    dev->blockid =  0;
    dev->fenced  =  0;
}

/*-------------------------------------------------------------------*/
/* Rewind HET format file                                            */
/*                                                                   */
/* The HET file is close and all device block fields reinitialized.  */
/*-------------------------------------------------------------------*/
DLL_EXPORT int rewind_het(DEVBLK *dev,BYTE *unitstat,BYTE code)
{
int rc;
    rc = het_rewind (dev->hetb);
    if (rc < 0)
    {
        /* Handle seek error condition */
        char msgbuf[128];
        MSGBUF( msgbuf, "Het error '%s': '%s'", het_error(rc), strerror(errno));
        WRMSG (HHC00205, "E", LCSS_DEVNUM, dev->filename, "het", "het_rewind()", msgbuf);

        build_senseX(TAPE_BSENSE_REWINDFAILED,dev,unitstat,code);
        return -1;
    }
    dev->nxtblkpos=0;
    dev->prvblkpos=-1;
    dev->curfilen=1;
    dev->blockid=0;
    dev->fenced = 0;
    return 0;
}

/*-------------------------------------------------------------------*/
/* Read a block from an HET format file                              */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the return value is zero, and the         */
/* current file number in the device block is incremented.           */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int read_het (DEVBLK *dev, BYTE *buf, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    rc = het_read (dev->hetb, buf);
    if (rc < 0)
    {
        /* Increment file number and return zero if tapemark was read */
        if (rc == HETE_TAPEMARK)
        {
            dev->curfilen++;
            dev->blockid++;
            return 0;
        }

        /* Handle end of file (uninitialized tape) condition */
        if (rc == HETE_EOT)
        {
            WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "het", "het_read()", (off_t)dev->hetb->cblk, "end of file (uninitialized tape)");

            /* Set unit exception with tape indicate (end of tape) */
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            return -1;
        }
        {
            char msgbuf[128];
            MSGBUF( msgbuf, "Het error '%s': '%s'", het_error(rc), strerror(errno));
            WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "het", "het_read()", (off_t)dev->hetb->cblk, msgbuf);
        }
        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }
    dev->blockid++;
    /* Return block length */
    return rc;

} /* end function read_het */

/*-------------------------------------------------------------------*/
/* Write a block to an HET format file                               */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int write_het( DEVBLK* dev, const BYTE* buf, U32 blklen,
                          BYTE* unitstat, BYTE code )
{
int             rc;                     /* Return code               */
off_t           cursize;                /* Current size for size chk */

    if (dev->hetb->writeprotect)
    {
        build_senseX( TAPE_BSENSE_WRITEPROTECT, dev, unitstat, code );
            return -1;
    }

    /* Check if we have already violated the size limit */
    if (dev->tdparms.maxsize > 0)
    {
        if ((cursize = het_tell( dev->hetb )) >= dev->tdparms.maxsize)
        {
            build_senseX( TAPE_BSENSE_ENDOFTAPE, dev, unitstat, code );
            return -1;
        }
    }

    /* Write the data block */
    if ((rc = het_write( dev->hetb, buf, blklen )) < 0)
    {
        /* Handle write error condition */
        char msgbuf[128];
        MSGBUF( msgbuf, "Het error '%s': '%s'", het_error( rc ), strerror( errno ));
        WRMSG( HHC00204, "E", LCSS_DEVNUM, dev->filename, "het", "het_write()", (off_t) dev->hetb->cblk, msgbuf );

        /* Set unit check with equipment check */
        build_senseX( TAPE_BSENSE_WRITEFAIL, dev, unitstat, code );
        return -1;
    }

    /* Check if we have violated the maxsize limit */
    /* Also check if we are passed EOT marker */
    if (dev->tdparms.maxsize > 0)
    {
        if ((cursize = het_tell( dev->hetb )) > dev->tdparms.maxsize)
        {
            if (!dev->batch || !dev->quiet)
                // "%1d:%04X Tape file %s, type %s: maximum tape capacity exceeded"
                WRMSG( HHC00208, "I", LCSS_DEVNUM, dev->filename, "het" );

            if (dev->tdparms.strictsize)
            {
                if (!dev->batch || !dev->quiet)
                    // "%1d:%04X Tape file %s, type %s: maximum tape capacity enforced"
                    WRMSG( HHC00209, "I", LCSS_DEVNUM, dev->filename, "het" );

                het_bsb( dev->hetb );
                cursize = het_tell( dev->hetb );
                VERIFY( 0 == ftruncate( dev->hetb->fd, cursize ));
                dev->hetb->truncated = TRUE; /* SHOULD BE IN HETLIB */
            }

            build_senseX( TAPE_BSENSE_ENDOFTAPE, dev, unitstat, code );
            return -1;
        }
    }

    /* Return normal status */
    dev->blockid++;

    return 0;

} /* end function write_het */

/*-------------------------------------------------------------------*/
/* Write a tapemark to an HET format file                            */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int write_hetmark( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
int             rc;                     /* Return code               */

    if ( dev->hetb->writeprotect )
    {
        build_senseX(TAPE_BSENSE_WRITEPROTECT,dev,unitstat,code);
        return -1;
    }

    /* Write the tape mark */
    rc = het_tapemark (dev->hetb);
    if (rc < 0)
    {
        /* Handle error condition */
        char msgbuf[128];
        MSGBUF( msgbuf, "Het error '%s': '%s'", het_error(rc), strerror(errno));
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "het", "het_tapemark()", (off_t)dev->hetb->cblk, msgbuf);

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        return -1;
    }

    /* Return normal status */
    dev->blockid++;

    return 0;

} /* end function write_hetmark */

/*-------------------------------------------------------------------*/
/* Synchronize a HET format file   (i.e. flush its buffers to disk)  */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int sync_het(DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    /* Perform the flush */
    rc = het_sync (dev->hetb);
    if (rc < 0)
    {
        /* Handle error condition */
        if (HETE_PROTECTED == rc)
            build_senseX(TAPE_BSENSE_WRITEPROTECT,dev,unitstat,code);
        else
        {
            WRMSG (HHC00205, "E", LCSS_DEVNUM, dev->filename, "het", "het_sync()", strerror(errno));
            build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        }
        return -1;
    }

    /* Return normal status */
    return 0;

} /* end function sync_het */

/*-------------------------------------------------------------------*/
/* Forward space over next block of an HET format file               */
/*                                                                   */
/* If successful, return value +1.                                   */
/* If the block skipped was a tapemark, the return value is zero,    */
/* and the current file number in the device block is incremented.   */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int fsb_het (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    /* Forward space one block */
    rc = het_fsb (dev->hetb);

    if (rc < 0)
    {
        /* Increment file number and return zero if tapemark was read */
        if (rc == HETE_TAPEMARK)
        {
            dev->blockid++;
            dev->curfilen++;
            return 0;
        }
        {
            char msgbuf[128];
            MSGBUF( msgbuf, "Het error '%s': '%s'", het_error(rc), strerror(errno));

            if (dev->hetb) // (could be null! see GitHub Issue #515!)
                // "%1d:%04X Tape file %s, type %s: error in function %s, offset 0x%16.16"PRIX64": %s"
                WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "het", "het_fsb()", (off_t)dev->hetb->cblk, msgbuf);
            else
                // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
                WRMSG( HHC00205, "E", LCSS_DEVNUM, dev->filename, "het", "het_fsb()", msgbuf );
        }
        /* Set unit check with equipment check */
        if(rc==HETE_EOT)
        {
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
        }
        else
        {
            build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        }
        return -1;
    }

    dev->blockid++;

    /* Return +1 to indicate forward space successful */
    return +1;

} /* end function fsb_het */

/*-------------------------------------------------------------------*/
/* Backspace to previous block of an HET format file                 */
/*                                                                   */
/* If successful, return value will be +1.                           */
/* If the block is a tapemark, the return value is zero,             */
/* and the current file number in the device block is decremented.   */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int bsb_het (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    /* Back space one block */
    rc = het_bsb (dev->hetb);
    if (rc < 0)
    {
        /* Increment file number and return zero if tapemark was read */
        if (rc == HETE_TAPEMARK)
        {
            dev->blockid--;
            dev->curfilen--;
            return 0;
        }

        /* Unit check if already at start of tape */
        if (rc == HETE_BOT)
        {
            build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
            return -1;
        }
        {
            char msgbuf[128];
            MSGBUF( msgbuf, "Het error '%s': '%s'", het_error(rc), strerror(errno));
            WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename,
                   "het", "het_bsb()", (off_t)dev->hetb->cblk, msgbuf);
        }
        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }

    dev->blockid--;

    /* Return +1 to indicate back space successful */
    return +1;

} /* end function bsb_het */

/*-------------------------------------------------------------------*/
/* Forward space to next logical file of HET format file             */
/*                                                                   */
/* If successful, return value is zero, and the current file number  */
/* in the device block is incremented.                               */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int fsf_het (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    /* Forward space to start of next file */
    rc = het_fsf (dev->hetb);
    if (rc < 0)
    {
        char msgbuf[128];
        MSGBUF( msgbuf, "Het error '%s': '%s'", het_error(rc), strerror(errno));
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "het", "het_fsf()", (off_t)dev->hetb->cblk, msgbuf);

        if(rc==HETE_EOT)
        {
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
        }
        else
        {
            build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        }
        return -1;
    }

    /* Maintain position */
    dev->blockid = rc;
    dev->curfilen++;

    /* Return success */
    return 0;

} /* end function fsf_het */

/*-------------------------------------------------------------------*/
/* Check HET file is passed the allowed EOT margin                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int passedeot_het (DEVBLK *dev)
{
off_t cursize;
    if(dev->fd>0)
    {
        if(dev->tdparms.maxsize>0)
        {
            cursize=het_tell(dev->hetb);
            if(cursize+dev->eotmargin>dev->tdparms.maxsize)
            {
                dev->eotwarning = 1;
                return 1;
            }
        }
    }
    dev->eotwarning = 0;
    return 0;
}

/*-------------------------------------------------------------------*/
/* Backspace to previous logical file of HET format file             */
/*                                                                   */
/* If successful, the return value is zero and the current file      */
/* number in the device block is decremented.                        */
/*                                                                   */
/* If error, the return value is -1 and unitstat is set to CE+DE+UC  */
/*                                                                   */
/* In either case the current block number in the HET control block  */
/* is always updated.                                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int bsf_het (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    /* Error if already at load point */
    if (dev->curfilen == 1 && dev->hetb->cblk == 0)
    {
        build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        return -1;
    }

    /* Attempt the backspace file */
    rc = het_bsf (dev->hetb);

    /* Return code from het_bsf will always be negative */
    ASSERT( rc < 0 );

    /* Maintain position */
    dev->blockid = dev->hetb->cblk;

    /* Check for true I/O error */
    if (rc < 0 && (rc != HETE_TAPEMARK && rc != HETE_BOT))
    {
        char msgbuf[128];
        MSGBUF( msgbuf, "Het error '%s': '%s'", het_error(rc), strerror(errno));
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "het", "het_bsf()", (off_t)dev->hetb->cblk, msgbuf);

        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }

    /* Error if we backspaced into load point */
    if (rc == HETE_BOT)
    {
        build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        return -1;
    }

    /* Otherwise verify the only other possibility */
    ASSERT( rc == HETE_TAPEMARK );

    /* Maintain position */
    dev->curfilen--;

    /* Return success */
    return 0;

} /* end function bsf_het */
