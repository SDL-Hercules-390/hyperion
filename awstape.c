/* AWSTAPE.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules Tape Device Handler for AWSTAPE             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Original Author: Roger Bowler                                     */
/* Prime Maintainer: Ivan Warren                                     */
/* Secondary Maintainer: "Fish" (David B. Trout)                     */

/*-------------------------------------------------------------------*/
/* This module contains the AWSTAPE emulated tape format support.    */
/* The subroutines in this module are called by the general tape     */
/* device handler (tapedev.c) when the tape format is AWSTAPE.       */
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/* Messages issued by this module are prefixed HHCTA1nn              */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _AWSTAPE_C_
#define _HDT3420_DLL_

#include "hercules.h"  /* need Hercules control blocks               */
#include "tapedev.h"   /* Main tape handler header file              */

//#define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
//#include "dbgtrace.h"                   // (Fish: DEBUGGING)

/*********************************************************************/
/* START OF ORIGINAL AWS FUNCTIONS   (ISW Additions)                 */
/*********************************************************************/

/*-------------------------------------------------------------------*/
/* Close an AWSTAPE format file                                      */
/* New Function added by ISW for consistency with other medias       */
/*-------------------------------------------------------------------*/
DLL_EXPORT void close_awstape( DEVBLK* dev )
{
    if (dev->fd >= 0)
    {
        if (!dev->batch || !dev->quiet)
            // "%1d:%04X Tape file %s, type %s: tape closed"
            WRMSG( HHC00201, "I", LCSS_DEVNUM, dev->filename, "aws" );
        close( dev->fd );
    }

    STRLCPY( dev->filename, TAPE_UNLOADED );

    dev->fd      = -1;
    dev->blockid =  0;
    dev->fenced  =  0;

    return;
}

/*-------------------------------------------------------------------*/
/* Rewinds an AWS Tape format file                                   */
/* New Function added by ISW for consistency with other medias       */
/*-------------------------------------------------------------------*/
DLL_EXPORT int rewind_awstape (DEVBLK *dev,BYTE *unitstat,BYTE code)
{
    off_t rcoff;
    rcoff=lseek(dev->fd,0,SEEK_SET);
    if(rcoff<0)
    {
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
/* Determines if a tape has passed a virtual EOT marker              */
/* New Function added by ISW for consistency with other medias       */
/*-------------------------------------------------------------------*/
DLL_EXPORT int passedeot_awstape (DEVBLK *dev)
{
    if( dev->nxtblkpos == 0 )
    {
        dev->eotwarning = 0;
        return 0;
    }
    if( dev->tdparms.maxsize == 0 )
    {
        dev->eotwarning = 0;
        return 0;
    }
    if( dev->nxtblkpos + dev->eotmargin > dev->tdparms.maxsize )
    {
        dev->eotwarning = 1;
        return 1;
    }
    dev->eotwarning = 0;
    return 0;
}

/*********************************************************************/
/* START OF ORIGINAL RB AWS FUNCTIONS                                */
/*********************************************************************/

/*-------------------------------------------------------------------*/
/* Open an AWSTAPE format file                                       */
/*                                                                   */
/* If successful, the file descriptor is stored in the device block  */
/* and the return value is zero.  Otherwise the return value is -1.  */
/*-------------------------------------------------------------------*/
DLL_EXPORT int open_awstape( DEVBLK* dev, BYTE* unitstat, BYTE code )
{
    int   rc = -1;                      /* Return code               */
    char  pathname[MAX_PATH];           /* file path in host format  */

    /* Check for no tape in drive */

    if (strcmp( dev->filename, TAPE_UNLOADED ) == 0)
    {
        build_senseX( TAPE_BSENSE_TAPEUNLOADED, dev, unitstat, code );
        return -1;
    }

    /* Open the AWSTAPE file */

    hostpath( pathname, dev->filename, sizeof( pathname ));

    /* If NOT read-only... */

    if (!dev->tdparms.logical_readonly)
    {
        /* Presume file exists and open it with read-write access */
        rc = HOPEN( pathname, O_RDWR | O_BINARY,
            S_IRUSR | S_IWUSR | S_IRGRP );

        /* If that didn't work and the auto-create option is on,
           then try to automatically create the file for them.
        */
        if (rc < 0 && sysblk.auto_tape_create)
        {
            /* (use 'O_CREAT' this time) */

            rc = HOPEN( pathname, O_RDWR | O_BINARY | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP );

            if (rc >= 0)
            {
                int tmp_fd = dev->fd;
                int ret_code = 0;

                dev->fd = rc;

                if (!dev->batch || !dev->quiet)
                    // "%1d:%04X Tape file %s, type %s: tape created"
                    WRMSG( HHC00235, "I", SSID_TO_LCSS( dev->ssid ),
                           dev->devnum, dev->filename, "aws" );

                /* Write two tapemarks */

                ret_code = write_awsmark( dev, unitstat, code );

                if (ret_code >= 0)
                    ret_code = write_awsmark( dev, unitstat, code );

                if (ret_code < 0)
                {
                    dev->fd = tmp_fd;
                    rc = ret_code;
                }
            }
        }
    }

    /* If file is read-only then try again with read-only access */
    if (0
        || dev->tdparms.logical_readonly
        || (rc < 0 && (EROFS  == errno || EACCES == errno))
    )
    {
        dev->readonly = 1;
        rc = HOPEN( pathname, O_RDONLY | O_BINARY, S_IRUSR | S_IRGRP );
    }

    /* Check for open failure */
    if (rc < 0)
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG( HHC00205, "E", LCSS_DEVNUM,
            dev->filename, "aws", "open()", strerror( errno ));

        STRLCPY( dev->filename, TAPE_UNLOADED );
        build_senseX( TAPE_BSENSE_TAPELOADFAIL, dev, unitstat, code );
        return -1;
    }

    /* Open success. Save file descriptor and rewind to load-point */

    dev->fd = rc;
    rc = rewind_awstape( dev, unitstat, code );
    return rc;

} /* end function open_awstape */

/*-------------------------------------------------------------------*/
/* Read an AWSTAPE block header                                      */
/*                                                                   */
/* If successful, return value is zero, and buffer contains header.  */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int readhdr_awstape (DEVBLK *dev, off_t blkpos,
                     AWSTAPE_BLKHDR *buf, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return code from lseek()  */

    /* Reposition file to the requested block header */
    rcoff = lseek (dev->fd, blkpos, SEEK_SET);
    if (rcoff < 0)
    {
        /* Handle seek error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "lseek()", blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }

    /* Read the 6-byte block header */
    rc = read (dev->fd, buf, sizeof(AWSTAPE_BLKHDR));

    /* Handle read error condition */
    if (rc < 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "read()", blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }

    /* Handle end of file (uninitialized tape) condition */
    if (rc == 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "readhdr_awstape()", blkpos, "end of file (uninitialized tape)");

        /* Set unit exception with tape indicate (end of tape) */
        build_senseX(TAPE_BSENSE_EMPTYTAPE,dev,unitstat,code);
        return -1;
    }

    /* Handle end of file within block header */
    if (rc < (int)sizeof(AWSTAPE_BLKHDR))
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "readhdr_awstape()", blkpos, "unexpected end of file");

        build_senseX(TAPE_BSENSE_BLOCKSHORT,dev,unitstat,code);
        return -1;
    }

    /* Successful return */
    return 0;

} /* end function readhdr_awstape */

/*-------------------------------------------------------------------*/
/* Read a block from an AWSTAPE format file                          */
/*                                                                   */
/* The block may be formed of one or more block segments each        */
/* preceded by an AWSTAPE block header. The ENDREC flag in the       */
/* block header indicates the final segment of the block.            */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the return value is zero, and the         */
/* current file number in the device block is incremented.           */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int read_awstape (DEVBLK *dev, BYTE *buf, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE block header      */
off_t           blkpos;                 /* Offset of block header    */
int             blklen = 0;             /* Total length of block     */
U16             seglen;                 /* Data length of segment    */

    /* Initialize current block position */
    blkpos = dev->nxtblkpos;

    /* Read block segments until end of block */
    do
    {
        /* Read the 6-byte block header */
        rc = readhdr_awstape (dev, blkpos, &awshdr, unitstat,code);
        if (rc < 0) return -1;

        /* Extract the segment length from the block header */
        seglen = ((U16)(awshdr.curblkl[1]) << 8)
                    | awshdr.curblkl[0];

        /* Calculate the offset of the next block segment */
        blkpos += sizeof(awshdr) + seglen;

        /* Check that block length will not exceed buffer size */
        if (blklen + seglen > MAX_TAPE_BLKSIZE)
        {
            // "%1d:%04X Tape file %s, type %s: block length %d exceeds maximum at offset 0x%16.16"PRIX64
            WRMSG (HHC00202, "E", LCSS_DEVNUM, dev->filename, "aws",
                    (int)MAX_TAPE_BLKSIZE, blkpos);

            /* Set unit check with data check */
            build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
            return -1;
        }

        /* Check that tapemark blocksize is zero */
        if ((awshdr.flags1 & AWSTAPE_FLAG1_TAPEMARK)
            && blklen + seglen > 0)
        {
            // "%1d:%04X Tape file %s, type %s: invalid tapemark at offset 0x%16.16"PRIX64
            WRMSG (HHC00203, "E", LCSS_DEVNUM, dev->filename, "aws", blkpos);

            /* Set unit check with data check */
            build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
            return -1;
        }

        /* Exit loop if this is a tapemark */
        if (awshdr.flags1 & AWSTAPE_FLAG1_TAPEMARK)
            break;

        /* Read data block segment from tape file */
        rc = read (dev->fd, buf+blklen, seglen);

        /* Handle read error condition */
        if (rc < 0)
        {
            // "%1d:%04X Tape file %s, type %s: error in function %s, offset 0x%16.16"PRIX64": %s"
            WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "read()", blkpos, strerror(errno));

            /* Set unit check with equipment check */
            build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
            return -1;
        }

        /* Handle end of file within data block */
        if (rc < seglen)
        {
            // "%1d:%04X Tape file %s, type %s: error in function %s, offset 0x%16.16"PRIX64": %s"
            WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "read_awstape()", blkpos, "end of file within data block");

            /* Set unit check with data check and partial record */
            build_senseX(TAPE_BSENSE_BLOCKSHORT,dev,unitstat,code);
            return -1;
        }

        /* Accumulate the total block length */
        blklen += seglen;

    } while ((awshdr.flags1 & AWSTAPE_FLAG1_ENDREC) == 0);

    /* Calculate the offsets of the next and previous blocks */
    dev->prvblkpos = dev->nxtblkpos;
    dev->nxtblkpos = blkpos;

    /* Increment the block number */
    dev->blockid++;

    /* Increment file number and return zero if tapemark was read */
    if (blklen == 0)
    {
        dev->curfilen++;
        return 0; /* UX will be set by caller */
    }

    /* Return block length */
    return blklen;

} /* end function read_awstape */

/*-------------------------------------------------------------------*/
/* Write a block to an AWSTAPE format file                           */
/*                                                                   */
/* The block may be formed of one or more block segments each        */
/* preceded by an AWSTAPE block header. The ENDREC flag in the       */
/* block header indicates the final segment of the block.            */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int write_awstape (DEVBLK *dev, const BYTE *buf, U32 blklen,
                        BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return code from lseek()  */
AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE chunk header      */
off_t           blkpos;                 /* Offset of chunk header    */
U16             chksize;                /* Length of current chunk   */
U16             prvblkl;                /* Length of previous chunk  */

    /* Initialize current block position and previous block length */
    blkpos = dev->nxtblkpos;
    prvblkl = 0;

    /* Determine previous block length if not at start of tape */
    if (dev->nxtblkpos > 0)
    {
        /* Reread the previous block header */
        rc = readhdr_awstape (dev, dev->prvblkpos, &awshdr, unitstat,code);
        if (rc < 0) return -1;

        /* Extract the block length from the block header */
        prvblkl = ((U16)(awshdr.curblkl[1]) << 8)
                    | awshdr.curblkl[0];

        /* Recalculate the offset of the next block */
        blkpos = dev->prvblkpos + sizeof(awshdr) + prvblkl;
    }

    /* Reposition file to the new block header */
    rcoff = lseek (dev->fd, blkpos, SEEK_SET);
    if (rcoff < 0)
    {
        /* Handle seek error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "lseek()", blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }

    /* ISW: Determine if we are passed maxsize */
    if(dev->tdparms.maxsize>0)
    {
        if((off_t)(dev->nxtblkpos+blklen+sizeof(awshdr)) > dev->tdparms.maxsize)
        {
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            return -1;
        }
    }
    /* ISW: End of virtual physical EOT determination */

    /* Initialize starting values */
    chksize = dev->tdparms.chksize;
    awshdr.flags1 = AWSTAPE_FLAG1_NEWREC;
    awshdr.flags2 = 0;

    /* Keep writing chunks until the entire block has been written */
    do
    {
        /* Adjust chunksize and flags if this is last/only chunk */
        if ((U32)chksize >= blklen)
        {
            chksize = (U16) blklen;
            awshdr.flags1 |= AWSTAPE_FLAG1_ENDREC;
        }

        /* Build the 6-byte chunk header */
        awshdr.curblkl[0] = chksize & 0xFF;
        awshdr.curblkl[1] = (chksize >> 8) & 0xFF;
        awshdr.prvblkl[0] = prvblkl & 0xFF;
        awshdr.prvblkl[1] = (prvblkl >> 8) & 0xFF;

        /* Write the chunk header */
        rc = write (dev->fd, &awshdr, sizeof(awshdr));
        if (rc < (int)sizeof(awshdr))
        {
            // "%1d:%04X Tape file %s, type %s: error in function %s, offset 0x%16.16"PRIX64": %s"
            WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename,
                "aws", "write()", blkpos, strerror(errno));
            if (ENOSPC == errno)
            {
                /* Disk FULL */
                build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
                return -1;
            }

            /* Set unit check with equipment check */
            build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
            return -1;
        }

        /* Now write the chunk itself */
        rc = write (dev->fd, buf, chksize);
        if (rc < (int)chksize)
        {
            // "%1d:%04X Tape file %s, type %s: error in function %s, offset 0x%16.16"PRIX64": %s"
            WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename,
                "aws", "write()", blkpos + sizeof(awshdr), strerror(errno));
            if (ENOSPC == errno)
            {
                /* Disk FULL */
                build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
                return -1;
            }

            /* Set unit check with equipment check */
            build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
            return -1;
        }

        /* Adjust the offsets of the previous and next blocks */
        dev->prvblkpos = blkpos;
        blkpos += sizeof(awshdr) + chksize;
        dev->nxtblkpos = blkpos;

        /* The next chunk (if there is one) won't be the first */
        awshdr.flags1 &= ~AWSTAPE_FLAG1_NEWREC;

        /* Adjust buffer pointer and block bytes remaining */
        buf    += chksize;
        blklen -= chksize;
        prvblkl = chksize;
    }
    while (blklen);

    dev->blockid++;

    /* Set new physical EOF */
    do rc = ftruncate( dev->fd, dev->nxtblkpos );
    while (EINTR == rc);

    /* Handle write error condition */
    if (rc != 0)
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s, offset 0x%16.16"PRIX64": %s"
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename,
            "aws", "ftruncate()", blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        return -1;
    }

    /* Return normal status */
    return 0;

} /* end function write_awstape */

/*-------------------------------------------------------------------*/
/* Write a tapemark to an AWSTAPE format file                        */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int write_awsmark (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return code from lseek()  */
AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE block header      */
off_t           blkpos;                 /* Offset of block header    */
U16             prvblkl;                /* Length of previous block  */

    /* Initialize current block position and previous block length */
    blkpos = dev->nxtblkpos;
    prvblkl = 0;

    /* Determine previous block length if not at start of tape */
    if (dev->nxtblkpos > 0)
    {
        /* Reread the previous block header */
        rc = readhdr_awstape (dev, dev->prvblkpos, &awshdr, unitstat,code);
        if (rc < 0) return -1;

        /* Extract the block length from the block header */
        prvblkl = ((U16)(awshdr.curblkl[1]) << 8)
                    | awshdr.curblkl[0];

        /* Recalculate the offset of the next block */
        blkpos = dev->prvblkpos + sizeof(awshdr) + prvblkl;
    }

    /* Reposition file to the new block header */
    rcoff = lseek (dev->fd, blkpos, SEEK_SET);
    if (rcoff < 0)
    {
        /* Handle seek error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "lseek()", blkpos, strerror(errno));

        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }
    /* ISW: Determine if we are passed maxsize */
    if(dev->tdparms.maxsize>0)
    {
        if((off_t)(dev->nxtblkpos+sizeof(awshdr)) > dev->tdparms.maxsize)
        {
            build_senseX(TAPE_BSENSE_ENDOFTAPE,dev,unitstat,code);
            return -1;
        }
    }
    /* ISW: End of virtual physical EOT determination */

    /* Build the 6-byte block header */
    awshdr.curblkl[0] = 0;
    awshdr.curblkl[1] = 0;
    awshdr.prvblkl[0] = prvblkl & 0xFF;
    awshdr.prvblkl[1] = (prvblkl >>8) & 0xFF;
    awshdr.flags1 = AWSTAPE_FLAG1_TAPEMARK;
    awshdr.flags2 = 0;

    /* Write the block header */
    rc = write (dev->fd, &awshdr, sizeof(awshdr));
    if (rc < (int)sizeof(awshdr))
    {
        /* Handle write error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "write()", blkpos, strerror(errno));

        build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        return -1;
    }

    dev->blockid++;

    /* Calculate the offsets of the next and previous blocks */
    dev->nxtblkpos = blkpos + sizeof(awshdr);
    dev->prvblkpos = blkpos;

    /* Set new physical EOF */
    do rc = ftruncate( dev->fd, dev->nxtblkpos );
    while (EINTR == rc);

    if (rc != 0)
    {
        /* Handle write error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, dev->filename, "aws", "ftruncate()", blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        return -1;
    }

    /* Return normal status */
    return 0;

} /* end function write_awsmark */

/*-------------------------------------------------------------------*/
/* Synchronize an AWSTAPE format file  (i.e. flush buffers to disk)  */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int sync_awstape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
    /* Unit check if tape is write-protected */
    if (dev->readonly)
    {
        build_senseX(TAPE_BSENSE_WRITEPROTECT,dev,unitstat,code);
        return -1;
    }

    /* Perform sync. Return error on failure. */
    if (fdatasync( dev->fd ) < 0)
    {
        /* Log the error */
        WRMSG (HHC00205, "E", LCSS_DEVNUM, dev->filename, "aws", "fdatasync()", strerror(errno));
        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_WRITEFAIL,dev,unitstat,code);
        return -1;
    }

    /* Return normal status */
    return 0;

} /* end function sync_awstape */

/*-------------------------------------------------------------------*/
/* Forward space over next block of AWSTAPE format file              */
/*                                                                   */
/* If successful, return value is the length of the block skipped.   */
/* If the block skipped was a tapemark, the return value is zero,    */
/* and the current file number in the device block is incremented.   */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int fsb_awstape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE block header      */
off_t           blkpos;                 /* Offset of block header    */
int             blklen = 0;             /* Total length of block     */
U16             seglen;                 /* Data length of segment    */

    /* Initialize current block position */
    blkpos = dev->nxtblkpos;

    /* Read block segments until end of block */
    do
    {
        /* Read the 6-byte block header */
        rc = readhdr_awstape (dev, blkpos, &awshdr, unitstat,code);
        if (rc < 0) return -1;

        /* Extract the block length from the block header */
        seglen = ((U16)(awshdr.curblkl[1]) << 8)
                    | awshdr.curblkl[0];

        /* Calculate the offset of the next block segment */
        blkpos += sizeof(awshdr) + seglen;

        /* Accumulate the total block length */
        blklen += seglen;

        /* Exit loop if this is a tapemark */
        if (awshdr.flags1 & AWSTAPE_FLAG1_TAPEMARK)
            break;

    } while ((awshdr.flags1 & AWSTAPE_FLAG1_ENDREC) == 0);

    /* Calculate the offsets of the next and previous blocks */
    dev->prvblkpos = dev->nxtblkpos;
    dev->nxtblkpos = blkpos;

    /* Increment current file number if tapemark was skipped */
    if (blklen == 0)
        dev->curfilen++;

    dev->blockid++;

    /* Return block length or zero if tapemark */
    return blklen;

} /* end function fsb_awstape */

/*-------------------------------------------------------------------*/
/* Backspace to previous block of AWSTAPE format file                */
/*                                                                   */
/* If successful, return value is the length of the block.           */
/* If the block is a tapemark, the return value is zero,             */
/* and the current file number in the device block is decremented.   */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int bsb_awstape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE block header      */
U16             curblkl;                /* Length of current block   */
U16             prvblkl;                /* Length of previous block  */
off_t           blkpos;                 /* Offset of block header    */

    /* Unit check if already at start of tape */
    if (dev->nxtblkpos == 0)
    {
        build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        return -1;
    }

    /* Backspace to previous block position */
    blkpos = dev->prvblkpos;

    /* Read the 6-byte block header */
    rc = readhdr_awstape (dev, blkpos, &awshdr, unitstat,code);
    if (rc < 0) return -1;

    /* Extract the block lengths from the block header */
    curblkl = ((U16)(awshdr.curblkl[1]) << 8)
                | awshdr.curblkl[0];
    prvblkl = ((U16)(awshdr.prvblkl[1]) << 8)
                | awshdr.prvblkl[0];

    /* Calculate the offset of the previous block */
    dev->prvblkpos = blkpos - sizeof(awshdr) - prvblkl;
    dev->nxtblkpos = blkpos;

    /* Decrement current file number if backspaced over tapemark */
    if (curblkl == 0)
        dev->curfilen--;

    dev->blockid--;

    /* Return block length or zero if tapemark */
    return curblkl;

} /* end function bsb_awstape */

/*-------------------------------------------------------------------*/
/* Forward space to next logical file of AWSTAPE format file         */
/*                                                                   */
/* For AWSTAPE files, the forward space file operation is achieved   */
/* by forward spacing blocks until positioned just after a tapemark. */
/*                                                                   */
/* If successful, return value is zero, and the current file number  */
/* in the device block is incremented by fsb_awstape.                */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int fsf_awstape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    while (1)
    {
        /* Forward space over next block */
        rc = fsb_awstape (dev, unitstat,code);
        if (rc < 0) return -1;

        /* Exit loop if spaced over a tapemark */
        if (rc == 0) break;

    } /* end while */

    /* Return normal status */
    return 0;

} /* end function fsf_awstape */

/*-------------------------------------------------------------------*/
/* Backspace to previous logical file of AWSTAPE format file         */
/*                                                                   */
/* For AWSTAPE files, the backspace file operation is achieved       */
/* by backspacing blocks until positioned just before a tapemark     */
/* or until positioned at start of tape.                             */
/*                                                                   */
/* If successful, return value is zero, and the current file number  */
/* in the device block is decremented by bsb_awstape.                */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int bsf_awstape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */

    while (1)
    {
        /* Exit if now at start of tape */
        if (dev->nxtblkpos == 0)
        {
            build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
            return -1;
        }

        /* Backspace to previous block position */
        rc = bsb_awstape (dev, unitstat,code);
        if (rc < 0) return -1;

        /* Exit loop if backspaced over a tapemark */
        if (rc == 0) break;

    } /* end while */

    /* Return normal status */
    return 0;

} /* end function bsf_awstape */

/*********************************************************************/
/*  END OF ORIGINAL RB AWS FUNCTIONS                                 */
/*********************************************************************/
