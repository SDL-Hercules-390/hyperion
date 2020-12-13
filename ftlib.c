/* FTLIB.C      (C) Copyright TurboHercules, SAS 2010-2011           */
/*              Hercules Tape Function Lib for FAKETAPE              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains the FAKETAPE emulated tape format library.   */
/* Note: this library was built from the original FAKETAPE.C source. */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Reference information:                                            */
/* FSIMS100 Faketape manual                                          */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _FTLIB_C_
#define _HTAPE_DLL_

#include "hercules.h"   /* need Hercules control blocks              */
#include "ftlib.h"      /* Main faketape header file                 */

// Local constant data  NOTE: Keep in sync with hetlib.c

static const char *fet_errstr[] =
{
    "No error",                         // FETE_OK
    "File error",                       // FETE_ERROR
    "Tapemark read",                    // FETE_TAPEMARK
    "Beginning of tape",                // FETE_BOT
    "End of tape",                      // FETE_EOT
    "BOR not found",                    // FETE_BADBOR
    "EOR not found",                    // FETE_BADEOR
    "Unexpected tapemark",              // FETE_BADMARK
    "Buffer not big enough",            // FETE_OVERFLOW
    "Premature EOF",                    // FETE_PREMEOF
    "Decompression error",
    "Unknown compression method",
    "Compression error",
    "Specified length to big",          // FETE_BADLEN
    "Write protected",                  // FETE_PROTECTED
    "Bad function code passed",         // FETE_BADFUNC
    "Bad compression method",
    "Bad compression level",
    "Bad write chunk size",
    "Invalid direction specified",      // FETE_BADDIR
    "Insufficient memory",              // FETE_NOMEM
    "Couldn't read block header",       // FETE_BADHDR
    "Inconsistent compression flags",
    "Block is short",                   // FETE_BADBLOCK
    "Location error",                   // FETE_BADLOC
    "Invalid error code",               // unknown error
};
#define FET_ERRSTR_MAX ( sizeof( fet_errstr) / sizeof( fet_errstr[ 0 ] ) )

/*-------------------------------------------------------------------*/
/* Open a FAKETAPE format file                                       */
/*                                                                   */
/* If successful, the file descriptor is stored in the device block  */
/* and the return value is zero.  Otherwise the return value is < 0. */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_open ( FETB **fetb, char *filename, int flags )
{
int             rc = -1;                /* Return code               */
char            pathname[MAX_PATH];     /* file path in host format  */
FETB           *tfetb;
char           *omode;
int             oflags;


    /* Open the FAKETAPE file */
    *fetb = NULL;

    hostpath(pathname, filename, sizeof(pathname));

    /*
    || Allocate a new FETB
    */
    tfetb = calloc( 1, sizeof( FETB ) );
    if( tfetb == NULL )
    {
        return( FETE_NOMEM );
    }

    /* Initialize FETB */
    tfetb->fd = -1;

    /*
    || clear FETOPEN_CREATE if FETOPEN_READONLY is specified
    */
    if(flags & FETOPEN_READONLY)
    {
        flags&=~FETOPEN_CREATE;
    }
    /*
    || Translate HET create flag to filesystem flag
    */
    oflags = ( ( flags & FETOPEN_CREATE ) ? O_CREAT : 0 );

    omode = "r+b";
    if( !( flags & FETOPEN_READONLY ) )
    {
        tfetb->fd = HOPEN( pathname, O_RDWR | O_BINARY | oflags, S_IRUSR | S_IWUSR | S_IRGRP );
    }

    /* If file is read-only, attempt to open again */
    if ( ( flags & FETOPEN_READONLY ) || (tfetb->fd < 0 && (EROFS == errno || EACCES == errno)))
    {
        omode = "rb";
        tfetb->writeprotect = TRUE;
        tfetb->fd = HOPEN( pathname, O_RDONLY | O_BINARY, S_IRUSR | S_IRGRP );
    }

    /*
    || Error out if both opens failed
    */
    if( tfetb->fd == -1 )
    {
        free( tfetb );
        return( FETE_ERROR );
    }

    /*
    || Associate stream with file descriptor
    */
    tfetb->fh = fdopen( tfetb->fd, omode );

    if( tfetb->fh == NULL )
    {
        rc = errno;
        close( tfetb->fd );
        errno = rc;
        free( tfetb );
        return( FETE_ERROR );
    }

    /*
    || If uninitialized tape, write 2 tapemarks to make it a valid NL tape
    */
    rc = fet_read_header( tfetb, 0L, NULL, NULL );
    if( rc < 0 && rc != FETE_TAPEMARK )
    {
        if( rc != FETE_EOT )
        {
            return( rc );
        }

        rc = fet_tapemark( tfetb );
        if( rc < 0 )
        {
            return( rc );
        }

        rc = fet_tapemark( tfetb );
        if( rc < 0 )
        {
            return( rc );
        }

        tfetb->created = TRUE;
    }
    else
        tfetb->created = FALSE;

    /*
    || Reposition tape to load point
    */
    rc = fet_rewind( tfetb );
    if( rc < 0 )
    {
        return( rc );
    }

    /*
    || Give the caller the new FETB
    */
    *fetb = tfetb;

    return( 0 );

} /* end function fet_open */

/*-------------------------------------------------------------------*/
/* Close a FAKETAPE format file                                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_close ( FETB **fetb )
{
    if( *(fetb) != NULL )
    {
        if( (*fetb)->fh != NULL )
        {
            fclose( (*fetb)->fh );
        }
        free( *(fetb) );
    }
    *fetb = NULL;
    return( 0 );
}

/*-------------------------------------------------------------------*/
/* Read a FAKETAPE block header                                      */
/*                                                                   */
/* If successful, return value is zero, and prvblkl and curblkl are  */
/* set to the previous and current block lengths respectively.       */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/* and prvblkl and curblkl are undefined. Either or both of prvblkl  */
/* and/or curblkl may be NULL.                                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_read_header( FETB *fetb, off_t blkpos,
                 U16* pprvblkl, U16* pcurblkl )
{
int             rc;                     /* Return code               */
FAKETAPE_BLKHDR fakehdr;                /* FakeTape block header     */
char            sblklen[5];             /* work for converting hdr   */
int             prvblkl;                /* Previous block length     */
int             curblkl;                /* Current block length      */
int             xorblkl;                /* XOR check of block lens   */

    /* Reposition file to the requested block header */
    rc = fseek( fetb->fh, blkpos, SEEK_SET );
    if (rc < 0)
    {
        return FETE_ERROR;
    }

    /* Read the 12-ASCII-hex-character block header */
    rc = (int)fread( &fakehdr, 1, sizeof(FAKETAPE_BLKHDR), fetb->fh );

    /* Handle read error condition */
    if (rc < 0)
    {
        return FETE_ERROR;
    }

    /* Handle end of file (uninitialized tape) condition */
    if (rc == 0)
    {
        return FETE_EOT;
    }

    /* Handle end of file within block header */
    if (rc < (int)sizeof(FAKETAPE_BLKHDR))
    {
        return FETE_BADHDR;
    }

    /* Convert the ASCII-hex-character block lengths to binary */
    strncpy( sblklen, fakehdr.sprvblkl, 4 ); sblklen[4] = 0; sscanf( sblklen, "%x", &prvblkl );
    strncpy( sblklen, fakehdr.scurblkl, 4 ); sblklen[4] = 0; sscanf( sblklen, "%x", &curblkl );
    strncpy( sblklen, fakehdr.sxorblkl, 4 ); sblklen[4] = 0; sscanf( sblklen, "%x", &xorblkl );

    /* Verify header integrity using the XOR header field */
    if ( (prvblkl ^ curblkl) != xorblkl )
    {
        return FETE_ERROR;
    }

    /* Return the converted value(s) to the caller */
    if (pprvblkl) *pprvblkl = prvblkl;
    if (pcurblkl) *pcurblkl = curblkl;

    /* Successful return */
    return FETE_OK;

} /* end function fet_read_header */

/*-------------------------------------------------------------------*/
/* Read a block from a FAKETAPE format file                          */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the return value is zero, and the         */
/* current file number in the device block is incremented.           */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_read ( FETB *fetb, void *buf )
{
int             rc;                     /* Return code               */
off_t           blkpos;                 /* Offset of block header    */
U16             curblkl;                /* Current block length      */

    /* Initialize current block position */
    blkpos = fetb->nxtblkpos;

    /* Read the block header to obtain the current block length */
    rc = fet_read_header( fetb, blkpos, NULL, &curblkl );
    if ( rc == FETE_EOT ) return FETE_EOT;
    if (rc < 0)
        return FETE_BADHDR; /* (error message already issued) */

    /* Calculate the offset of the next block header */
    blkpos += sizeof(FAKETAPE_BLKHDR) + curblkl;

    /* If not a tapemark, read the data block */
    if (curblkl > 0)
    {
        rc = (int)fread( buf, 1, curblkl, fetb->fh );

        /* Handle read error condition */
        if (rc < 0)
        {
            return FETE_ERROR;
        }

        /* Handle end of file within data block */
        if (rc < curblkl)
        {
            return FETE_BADBLOCK;
        }
    }

    /* Calculate the offsets of the next and previous blocks */
    fetb->prvblkpos = fetb->nxtblkpos;
    fetb->nxtblkpos = blkpos;

    /* Increment the block number */
    fetb->blockid++;

    /* Increment file number and return zero if tapemark was read */
    if (curblkl == 0)
    {
        fetb->curfilen++;
        return FETE_TAPEMARK; /* UX will be set by caller */
    }

    /* Return block length */
    return curblkl;

} /* end function fet_read */

/*-------------------------------------------------------------------*/
/* Write a FAKETAPE block header                                     */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/

DLL_EXPORT int
fet_write_header ( FETB *fetb, off_t blkpos,
                   U16 prvblkl, U16 curblkl )
{
int             rc;                     /* Return code               */
FAKETAPE_BLKHDR fakehdr;                /* FAKETAPE block header     */
char    temp[5];

    /* Position file to where block header is to go */
    rc = fseek( fetb->fh, blkpos, SEEK_SET );
    if (rc < 0)
    {
        return FETE_BADLOC;
    }

    /* Build the 12-ASCII-hex-character block header */
    MSGBUF( temp, "%4.4X", prvblkl );
    memcpy( fakehdr.sprvblkl, temp, sizeof(fakehdr.sprvblkl));
    MSGBUF( temp, "%4.4X", curblkl );
    memcpy( fakehdr.scurblkl, temp, sizeof(fakehdr.scurblkl));
    MSGBUF( temp, "%4.4X", prvblkl ^ curblkl );
    memcpy( fakehdr.sxorblkl, temp, sizeof(fakehdr.sxorblkl));

    /* Write the block header */
    rc = (int)fwrite( &fakehdr, 1, sizeof(FAKETAPE_BLKHDR), fetb->fh );
    if (rc < (int)sizeof(FAKETAPE_BLKHDR))
    {
        if(errno==ENOSPC)
        {
            /* Disk FULL */
            return FETE_EOT;
        }

        return FETE_ERROR;
    }

    return 0;

} /* end function fet_write_header */

/*-------------------------------------------------------------------*/
/* Write a block to a FAKETAPE format file                           */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_write( FETB *fetb, void *buf, U16 blklen )
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return code from lseek()  */
off_t           blkpos;                 /* Offset of block header    */
U16             prvblkl;                /* Length of previous block  */

    /* Initialize current block position and previous block length */
    blkpos = fetb->nxtblkpos;
    prvblkl = 0;

    /* Determine previous block length if not at start of tape */
    if (fetb->nxtblkpos > 0)
    {
        /* Retrieve the previous block length */
        rc = fet_read_header( fetb, fetb->prvblkpos, NULL, &prvblkl );
        if (rc < 0) return FETE_ERROR;

        /* Recalculate the offset of the next block */
        blkpos = fetb->prvblkpos + sizeof(FAKETAPE_BLKHDR) + prvblkl;
    }

    /* Reposition file to the new block header */
    rc = fseek( fetb->fh, blkpos, SEEK_SET );
    if (rc < 0)
    {
        return FETE_BADLOC;
    }
    else
        rcoff = ftell( fetb->fh );

    if(fetb->maxsize>0)
    {
        if((off_t)(fetb->nxtblkpos+blklen+sizeof(FAKETAPE_BLKHDR)) > fetb->maxsize )
        {
            return FETE_EOT;
        }
    }

    /* Write the block header */
    rc = fet_write_header( fetb, rcoff, prvblkl, blklen );
    if (rc < 0)
        return FETE_EOT; /* (error message already issued) */

    /* Calculate the offsets of the next and previous blocks */
    fetb->nxtblkpos = blkpos + sizeof(FAKETAPE_BLKHDR) + blklen;
    fetb->prvblkpos = blkpos;

    /* Write the data block */
    rc = (int)fwrite( buf, 1, blklen, fetb->fh);
    if (rc < blklen)
    {
        if(errno==ENOSPC)
        {
            /* Disk FULL */
            return FETE_ERROR;
        }

        return FETE_EOT;
    }

    /* Increment the block number */
    fetb->blockid++;

    /* Set new physical EOF */


    do rc = ftruncate( fetb->fd, fetb->nxtblkpos );
    while (EINTR == rc);

    if (rc != 0)
    {
        return FETE_EOT;
    }

    /* Return normal status */
    return 0;

} /* end function fet_write */

/*-------------------------------------------------------------------*/
/* Write a tapemark to a FAKETAPE format file                        */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_tapemark( FETB *fetb )
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return code from lseek()  */
off_t           blkpos;                 /* Offset of block header    */
U16             prvblkl;                /* Length of previous block  */

    /* Initialize current block position and previous block length */
    blkpos = fetb->nxtblkpos;
    prvblkl = 0;

    /* Determine previous block length if not at start of tape */
    if (fetb->nxtblkpos > 0)
    {
        /* Retrieve the previous block length */
        rc = fet_read_header( fetb, fetb->prvblkpos, NULL, &prvblkl );
        if (rc < 0)
            return rc;

        /* Recalculate the offset of the next block */
        blkpos = fetb->prvblkpos + sizeof(FAKETAPE_BLKHDR) + prvblkl;
    }

    /* Reposition file to the new block header */
    rc = fseek( fetb->fh, blkpos, SEEK_SET );
    if (rc < 0)
    {
        return FETE_BADLOC;
    }
    else
        rcoff = ftell( fetb->fh );

    if(fetb->maxsize>0)
    {
        if((off_t)(fetb->nxtblkpos+sizeof(FAKETAPE_BLKHDR)) > fetb->maxsize)
        {
            return FETE_EOT;
        }
    }

    /* Write the block header */
    rc = fet_write_header( fetb, rcoff, prvblkl, 0 );
    if (rc < 0)
        return rc; /* (error message already issued) */

    /* Increment the block number */
    fetb->blockid++;

    /* Calculate the offsets of the next and previous blocks */
    fetb->nxtblkpos = blkpos + sizeof(FAKETAPE_BLKHDR);
    fetb->prvblkpos = blkpos;

    /* Set new physical EOF */
    rc = ftell( fetb->fh );

    do rc = ftruncate( fetb->fd, fetb->nxtblkpos );
    while (EINTR == rc);

    if (rc != 0)
    {
        return FETE_PROTECTED;
    }

    /* Return normal status */
    return 0;

} /* end function fet_tapemark */

/*-------------------------------------------------------------------*/
/* Synchronize a FAKETAPE format file  (i.e. flush buffers to disk)  */
/*                                                                   */
/* If successful, return value is zero.                              */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_sync( FETB *fetb )
{
    /* Unit check if tape is write-protected */
    if (fetb->writeprotect)
    {
        return FETE_PROTECTED;
    }

    /* Perform sync. Return error on failure. */
    if (fdatasync( fetb->fd ) < 0)
    {
        return FETE_ERROR;
    }

    /* Return normal status */
    return 0;

} /* end function fet_sync */

/*-------------------------------------------------------------------*/
/* Forward space over next block of a FAKETAPE format file           */
/*                                                                   */
/* If successful, return value is the length of the block skipped.   */
/* If the block skipped was a tapemark, the return value is zero,    */
/* and the current file number in the device block is incremented.   */
/* If error, return value is < 0.                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_fsb( FETB *fetb )
{
int             rc;                     /* Return code               */
off_t           blkpos;                 /* Offset of block header    */
U16             blklen;                 /* Block length              */

    /* Initialize current block position */
    blkpos = fetb->nxtblkpos;

    /* Read the block header to obtain the current block length */
    rc = fet_read_header( fetb, blkpos, NULL, &blklen );
    if (rc < 0)
        return rc; /* (error message already issued) */

    /* Calculate the offset of the next block */
    blkpos += sizeof(FAKETAPE_BLKHDR) + blklen;

    /* Calculate the offsets of the next and previous blocks */
    fetb->prvblkpos = fetb->nxtblkpos;
    fetb->nxtblkpos = blkpos;

    /* Increment current file number if tapemark was skipped */
    if (blklen == 0)
        fetb->curfilen++;

    /* Increment the block number */
    fetb->blockid++;

    /* Return block length or zero if tapemark */
    return blklen;

} /* end function fet_fsb */

/*-------------------------------------------------------------------*/
/* Backspace to previous block of a FAKETAPE format file             */
/*                                                                   */
/* If successful, return value is the length of the block.           */
/* If the block is a tapemark, the return value is zero,             */
/* and the current file number in the device block is decremented.   */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_bsb( FETB *fetb )
{
int             rc;                     /* Return code               */
U16             curblkl;                /* Length of current block   */
U16             prvblkl;                /* Length of previous block  */
off_t           blkpos;                 /* Offset of block header    */

    /* Unit check if already at start of tape */
    if (fetb->nxtblkpos == 0)
    {
        return FETE_BOT;
    }

    /* Backspace to previous block position */
    blkpos = fetb->prvblkpos;

    /* Read the block header to obtain the block lengths */
    rc = fet_read_header( fetb, blkpos, &prvblkl, &curblkl );
    if (rc < 0)
        return rc; /* (error message already issued) */

    /* Calculate the offset of the previous block */
    fetb->prvblkpos = blkpos - sizeof(FAKETAPE_BLKHDR) - prvblkl;
    fetb->nxtblkpos = blkpos;

    /* Decrement current file number if backspaced over tapemark */
    if (curblkl == 0)
        fetb->curfilen--;

    /* Decrement the block number */
    fetb->blockid--;

    /* Return block length or zero if tapemark */
    return curblkl;

} /* end function fet_bsb */

/*-------------------------------------------------------------------*/
/* Forward space to next logical file of a FAKETAPE format file      */
/*                                                                   */
/* For FAKETAPE files, the forward space file operation is achieved  */
/* by forward spacing blocks until positioned just after a tapemark. */
/*                                                                   */
/* If successful, return value is zero, and the current file number  */
/* in the device block is incremented by fet_fsb.                    */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_fsf( FETB *fetb )
{
int             rc;                     /* Return code               */

    while (1)
    {
        /* Forward space over next block */
        rc = fet_fsb( fetb );
        if (rc < 0)
            return rc; /* (error message already issued) */

        /* Exit loop if spaced over a tapemark */
        if (rc == 0)
            break;

    } /* end while */

    /* Return normal status */
    return 0;

} /* end function fet_fsf */

/*-------------------------------------------------------------------*/
/* Backspace to previous logical file of a FAKETAPE format file      */
/*                                                                   */
/* For FAKETAPE files, the backspace file operation is achieved      */
/* by backspacing blocks until positioned just before a tapemark     */
/* or until positioned at start of tape.                             */
/*                                                                   */
/* If successful, return value is zero, and the current file number  */
/* in the device block is decremented by fet_bsb.                    */
/* If error, return value is < 0                                     */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_bsf( FETB *fetb )
{
int             rc;                     /* Return code               */

    while (1)
    {
        /* Exit if now at start of tape */
        if (fetb->nxtblkpos == 0)
        {
            return FETE_BOT;
        }

        /* Backspace to previous block position */
        rc = fet_bsb( fetb );
        if (rc < 0)
            return rc; /* (error message already issued) */

        /* Exit loop if backspaced over a tapemark */
        if (rc == 0)
            break;

    } /* end while */

    /* Return normal status */
    return 0;

} /* end function fet_bsf */


/*-------------------------------------------------------------------*/
/* Rewinds a FAKETAPE format file                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_rewind ( FETB *fetb )
{
int         rc;

    rc = fseek( fetb->fh, 0L, SEEK_SET );
    if ( rc < 0 )
    {
        return FETE_ERROR;
    }

    fetb->nxtblkpos=0;
    fetb->prvblkpos=-1;
    fetb->curfilen=1;
    fetb->blockid=0;
    return 0;
}

/*-------------------------------------------------------------------*/
/* Determines if a FAKETAPE has passed a virtual EOT marker          */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
fet_passedeot ( FETB *fetb )
{
    if(fetb->nxtblkpos==0)
        fetb->eotwarning = FALSE;
    else
        if(fetb->maxsize==0)
            fetb->eotwarning = TRUE;
    else
        if(fetb->nxtblkpos+fetb->eotmargin > fetb->maxsize)
            fetb->eotwarning = TRUE;
    else
        fetb->eotwarning = FALSE;
    return fetb->eotwarning;
}

DLL_EXPORT const char *
fet_error( int rc )
{
    // no error, return OK
    if( rc > -1 )
        rc = 0;

    rc *= -1;       // make positive for indexing

    if( rc >= (int)FET_ERRSTR_MAX )     // detect mismatch errors vs error strings
        rc = FET_ERRSTR_MAX - 1;

    return( fet_errstr[ rc ] );
}
