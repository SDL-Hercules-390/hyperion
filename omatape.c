/* OMATAPE.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules Tape Device Handler for OMATAPE             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Original Author: Roger Bowler                                     */
/* Prime Maintainer: Ivan Warren                                     */
/* Secondary Maintainer: "Fish" (David B. Trout)                     */

/*-------------------------------------------------------------------*/
/* This module contains the OMATAPE emulated tape format support.    */
/* The subroutines in this module are called by the general tape     */
/* device handler (tapedev.c) when the tape format is OMATAPE.       */
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/* Reference information:                                            */
/* SC53-1200 S/370 and S/390 Optical Media Attach/2 User's Guide     */
/* SC53-1201 S/370 and S/390 Optical Media Attach/2 Technical Ref    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"  /* need Hercules control blocks               */
#include "tapedev.h"   /* Main tape handler header file              */

//#define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
//#include "dbgtrace.h"                   // (Fish: DEBUGGING)

/*-------------------------------------------------------------------*/
/* Read and parse the OMA TDF tape descriptor file                   */
/*-------------------------------------------------------------------*/
int read_omadesc( DEVBLK* dev )
{
FILE*           oma;                    /* OMA tape descriptor file  */
OMATAPE_DESC*   tdftab;                 /* -> Tape descriptor array  */
int             i;                      /* Array subscript           */
int             stmt;                   /* TDF file statement number */
int             argc;                   /* Parsed TDF record argc    */
char*           argv[ 4 ];              /* Parsed TDF record argv    */
char*           tdffilenm;              /* -> Filename in TDF record */
char*           tdfformat;              /* -> Format in TDF record   */
char*           tdfreckwd;              /* -> Keyword in TDF record  */
char*           tdfblklen;              /* -> Length in TDF record   */
char            str[512];               /* Stmt. read from OMA file  */
char            pathname[ MAX_PATH + 1];/* file path in host format  */
char            buf[ MAX_PATH + 32 ];   /* Work for MSGBUF use       */
size_t          pathlen;                /* Length of TDF path name   */
U16             filecount;              /* Number of files           */
U32             blklen;                 /* Fixed block length        */
BYTE            c;                      /* Work area for sscanf      */

    /* Normalize path separators to be all '/' forward slashes */
    hostpath( pathname, dev->filename, sizeof( pathname ));

    /* Isolate the base path name of the TDF file */
    for (pathlen = strlen( pathname ); pathlen > 0; )
    {
        pathlen--;
        if (pathname[ pathlen - 1 ] == '/') break;
    }

    /* Open the TDF tape descriptor file */
    if (!(oma = fopen( pathname, "r" )))
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG( HHC00205, "E", LCSS_DEVNUM, dev->filename, "OMA", "fopen()", strerror( errno ));
        return -1;
    }

    /* Check that the first record is a @TDF header
       and count the number of records in the file. */
    for (filecount = 0; fgets( str, sizeof( str ), oma ); filecount++)
    {
        if (filecount != 0)     /* If not first record */
            continue;           /* then just count it  */

        /* Check that the first record is a @TDF header...
           Note: The OMA/2 specification defines several optional fields
           for the "@TDF" entry: "DEN1600", "DEN6250", "SIZE" and a type
           of comment introduced by a ";" character. Hercules allows for,
           but does not support, any of these.
        */
        if (0
            || memcmp( str, "@TDF", 4 ) != 0
//          || (1
//              && str[4] != '\r'
//              && str[4] != '\n'
//             )
        )
        {
            // "%1d:%04X Tape file %s, type %s: not a valid @TDF file: %s"
            WRMSG( HHC00206, "E", LCSS_DEVNUM, dev->filename, "OMA",
                "Missing or invalid @TDF header." );
            fclose( oma );
            return -1;
        }
    }

    /* Check for EOF or I/O error */
    if (ferror( oma ))
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG( HHC00205, "E", LCSS_DEVNUM, dev->filename, "OMA", "fgets()", strerror( errno ));
        fclose( oma );
        return -1;
    }

    /* Check for empty file or file with only @TDF statement */
    if (filecount < 2)
    {
        // "%1d:%04X Tape file %s, type %s: not a valid @TDF file: %s"
        WRMSG( HHC00206, "E", LCSS_DEVNUM, dev->filename, "OMA",
            "Descriptor contains zero control records." );
        fclose( oma );
        return -1;
    }

    /* Obtain storage for the tape descriptor array */
    if (!(tdftab = (OMATAPE_DESC*) malloc( filecount * sizeof( OMATAPE_DESC ))))
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG( HHC00205, "E", LCSS_DEVNUM, dev->filename, "OMA", "malloc()", strerror( errno ));
        return -1;
    }

    /* Rewind OMA file back to the beginning */
    rewind( oma );

    /* Now read and actually process each statement in the file... */
    for (filecount = 0, stmt = 1; fgets( str, sizeof( str ), oma ); stmt++)
    {
        /* (skip @TDF statement) */
        if (stmt == 1)
            continue;

        /* (remove trailing whitespace) */
        RTRIM( str );

        /* Clear the tape descriptor array entry */
        memset( &(tdftab[ filecount ]), 0, sizeof( OMATAPE_DESC ));

        /* Exit if EOT record encountered */
        if (str_caseless_eq( str, "EOT" ))
            break;

        /* TM tapemark record? */
        if (str_caseless_eq( str, "TM" ))
        {
            tdftab[ filecount++ ].format = 'X';
            continue;
        }

        /* Parse the TDF record */
        parse_args( str, 4, argv, &argc );

        tdffilenm = argv[0];            /* -> Filename in TDF record */
        tdfformat = argv[1];            /* -> Format in TDF record   */
        tdfreckwd = argv[2];            /* -> Keyword in TDF record  */
        tdfblklen = argv[3];            /* -> Length in TDF record   */

        /* Check for missing fields */
        if (0
            || !tdffilenm
            || !tdfformat
        )
        {
            // "%1d:%04X Tape file %s, type %s: line %d: %s"
            WRMSG( HHC00207, "E", LCSS_DEVNUM, dev->filename, "OMA", stmt, "filename or format missing" );
            goto omadesc_error;
        }

        /* Check that the file name is not too long */
        if (pathlen + 1 + strlen( tdffilenm ) + 1 >= sizeof( tdftab[ filecount ].filename ))
        {
            if (!strchr( tdffilenm, SPACE ))
                MSGBUF( buf, "filename %s too long",   tdffilenm );
            else
                MSGBUF( buf, "filename '%s' too long", tdffilenm );

            // "%1d:%04X Tape file %s, type %s: line %d: %s"
            WRMSG( HHC00207, "E", LCSS_DEVNUM, dev->filename, "OMA", stmt, buf );
            goto omadesc_error;
        }

        /* Convert the file name to Unix format */
        for (i=0; i < (int) strlen( tdffilenm ); i++)
        {
            if (tdffilenm[i] == '\\')
                tdffilenm[i] = '/';
        }

        /* Prefix the file name with the base path name
           ONLY if the filename is NOT an absolute path.
        */
#if defined(_MSVC_)
        if (tdffilenm[1] != ':')   // (not Windows absolute path)
#else
        if (tdffilenm[0] != '/')   // (not Unix absolute path)
#endif
        {
            /* (use memcpy since "pathlen" has already been checked above) */
            memcpy( tdftab[ filecount ].filename, dev->filename, pathlen );
            tdftab[ filecount ].filename[ pathlen ] = 0;

            /* Append path separator only if needed */
            if (tdftab[ filecount ].filename[ pathlen - 1 ] != '/')
                STRLCAT( tdftab[ filecount ].filename, "/" );
        }

        /* Save the file name in the tape descriptor array */
        STRLCAT( tdftab[ filecount ].filename, tdffilenm );

        /* Check for valid file format code */
        if (str_caseless_eq( tdfformat, "HEADERS" ))
        {
            tdftab[ filecount++ ].format = 'H';
            continue;
        }

        if (str_caseless_eq( tdfformat, "TEXT" ))
        {
            tdftab[ filecount++ ].format = 'T';
            continue;
        }

        if /* (treat UNDEFINED same as FIXED) */
        (0
            || str_caseless_eq( tdfformat, "FIXED"     )
            || str_caseless_eq( tdfformat, "UNDEFINED" )
        )
        {
            /* Check for RECSIZE keyword */
            if (0
                || !tdfreckwd
                || str_caseless_ne( tdfreckwd, "RECSIZE" )
            )
            {
                // "%1d:%04X Tape file %s, type %s: line %d: %s"
                WRMSG( HHC00207, "E", LCSS_DEVNUM, dev->filename, "OMA", stmt, "keyword RECSIZE missing" );
                goto omadesc_error;
            }

            /* Check for valid fixed block length */
            if (0
                || !tdfblklen
                || sscanf( tdfblklen, "%u%c", &blklen, &c ) != 1
                || blklen < 1
                || blklen > USHRT_MAX  // (max U16)
            )
            {
                MSGBUF( buf, "invalid record size %s", tdfblklen );

                // "%1d:%04X Tape file %s, type %s: line %d: %s"
                WRMSG( HHC00207, "E", LCSS_DEVNUM, dev->filename, "OMA", stmt, buf );
                goto omadesc_error;
            }

            /* Set format and block length in descriptor array */
            tdftab[ filecount ].format = 'F';
            tdftab[ filecount ].blklen = (U16) blklen;
            filecount++;
            continue;
        }

        /* Not a TDF format that we recognize/support */
        MSGBUF( buf, "invalid record format '%s'", tdfformat );

        // "%1d:%04X Tape file %s, type %s: line %d: %s"
        WRMSG( HHC00207, "E", LCSS_DEVNUM, dev->filename, "OMA", stmt, buf );
        goto omadesc_error;
    }
    /* end for( fgets( str, oma ) ) */

    /* Check for EOF or I/O error */
    if (ferror( oma ))
    {
        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG( HHC00205, "E", LCSS_DEVNUM, dev->filename, "OMA", "fgets()", strerror( errno ));
        goto omadesc_error;
    }

    /* Close the OMA file */
    fclose( oma );

    /* Force an EOT as last entry */
    tdftab[ filecount++ ].format = 'E';

    /* Save the file count and TDF array pointer in the device block */
    dev->omafiles = filecount;
    dev->omadesc  = tdftab;

    return 0;

omadesc_error:

    free( tdftab );
    fclose( oma );
    return -1;

} /* end function read_omadesc */

/*-------------------------------------------------------------------*/
/* Open the OMATAPE file defined by the current file number          */
/*                                                                   */
/* The OMA tape descriptor file is read if necessary.                */
/* If successful, the file descriptor is stored in the device block  */
/* and the return value is zero.  Otherwise the return value is -1.  */
/*-------------------------------------------------------------------*/
int open_omatape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             fd;                     /* File descriptor integer   */
int             rc;                     /* Return code               */
OMATAPE_DESC   *omadesc;                /* -> OMA descriptor entry   */
char            pathname[MAX_PATH];     /* file path in host format  */

     /* Check for no tape in drive */
     if (!strcmp (dev->filename, TAPE_UNLOADED))
     {
         build_senseX(TAPE_BSENSE_TAPEUNLOADED,dev,unitstat,code);
         return -1;
     }

    /* Read the OMA descriptor file if necessary */
    if (dev->omadesc == NULL)
    {
        rc = read_omadesc (dev);
        if (rc < 0)
        {
            build_senseX(TAPE_BSENSE_TAPELOADFAIL,dev,unitstat,code);
            return -1;
        }
        dev->blockid = 0;
    }

    dev->fenced = 0;

    /* Unit exception if beyond end of tape */
    /* ISW: CHANGED PROCESSING - RETURN UNDEFINITE Tape Marks  */
    /*       NOTE: The last entry in the TDF table is ALWAYS   */
    /*              an EOT Condition                           */
    /*              This is ensured by the TDF reading routine */
    if(dev->curfilen>dev->omafiles)
    {
        dev->curfilen=dev->omafiles;
        return(0);
    }

    /* Point to the current file entry in the OMA descriptor table */
    omadesc = (OMATAPE_DESC*)(dev->omadesc);
    omadesc += (dev->curfilen-1);
    if(omadesc->format=='X')
    {
        return 0;
    }
    if(omadesc->format=='E')
    {
        return 0;
    }

    /* Open the OMATAPE file */
    hostpath(pathname, omadesc->filename, sizeof(pathname));
    fd = HOPEN (pathname, O_RDONLY | O_BINARY);

    /* Check for successful open */
    if (fd < 0 || lseek (fd, 0, SEEK_END) > LONG_MAX)
    {
        if (fd >= 0)            /* (if open was successful, then it) */
            errno = EOVERFLOW;  /* (must have been a lseek overflow) */

        // "%1d:%04X Tape file %s, type %s: error in function %s: %s"
        WRMSG (HHC00205, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "open()", strerror(errno));

        if (fd >= 0)
            close(fd);          /* (close the file if it was opened) */

        build_senseX(TAPE_BSENSE_TAPELOADFAIL,dev,unitstat,code);
        return -1;
    }

    /* OMA tapes are always read-only */
    dev->readonly = 1;

    /* Store the file descriptor in the device block */
    dev->fd = fd;
    return 0;

} /* end function open_omatape */

/*-------------------------------------------------------------------*/
/* Read a block header from an OMA tape file in OMA headers format   */
/*                                                                   */
/* If successful, return value is zero, and the current block        */
/* length and previous and next header offsets are returned.         */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int readhdr_omaheaders (DEVBLK *dev, OMATAPE_DESC *omadesc,
                        long blkpos, S32 *pcurblkl, S32 *pprvhdro,
                        S32 *pnxthdro, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return code from lseek()  */
int             padding;                /* Number of padding bytes   */
OMATAPE_BLKHDR  omahdr;                 /* OMATAPE block header      */
S32             curblkl;                /* Length of current block   */
S32             prvhdro;                /* Offset of previous header */
S32             nxthdro;                /* Offset of next header     */

    /* Seek to start of block header */
    rcoff = lseek (dev->fd, blkpos, SEEK_SET);
    if (rcoff < 0)
    {
        /* Handle seek error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "lseek()", (off_t)blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }

    /* Read the 16-byte block header */
    rc = read (dev->fd, &omahdr, sizeof(omahdr));

    /* Handle read error condition */
    if (rc < 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "read()", (off_t)blkpos,
                strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }

    /* Handle end of file within block header */
    if (rc < (int)sizeof(omahdr))
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "readhdr_omaheaders()", (off_t)blkpos, "unexpected end of file");

        /* Set unit check with data check and partial record */
        build_senseX(TAPE_BSENSE_BLOCKSHORT,dev,unitstat,code);
        return -1;
    }

    /* Extract the current block length and previous header offset */
    curblkl = (S32)(((U32)(omahdr.curblkl[3]) << 24)
                    | ((U32)(omahdr.curblkl[2]) << 16)
                    | ((U32)(omahdr.curblkl[1]) << 8)
                    | omahdr.curblkl[0]);
    prvhdro = (S32)((U32)(omahdr.prvhdro[3]) << 24)
                    | ((U32)(omahdr.prvhdro[2]) << 16)
                    | ((U32)(omahdr.prvhdro[1]) << 8)
                    | omahdr.prvhdro[0];

    /* Check for valid block header */
    if (curblkl < -1 || curblkl == 0 || curblkl > MAX_TAPE_BLKSIZE
        || memcmp(omahdr.omaid, "@HDF", 4) != 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "readhdr_omaheaders()", (off_t)blkpos, "invalid block header");

        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }

    /* Calculate the number of padding bytes which follow the data */
    padding = (16 - (curblkl & 15)) & 15;

    /* Calculate the offset of the next block header */
    nxthdro = blkpos + sizeof(OMATAPE_BLKHDR) + curblkl + padding;

    /* Return current block length and previous/next header offsets */
    *pcurblkl = curblkl;
    *pprvhdro = prvhdro;
    *pnxthdro = nxthdro;
    return 0;

} /* end function readhdr_omaheaders */

/*-------------------------------------------------------------------*/
/* Read a block from an OMA tape file in OMA headers format          */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the file is closed, the current file      */
/* number in the device block is incremented so that the next file   */
/* will be opened by the next CCW, and the return value is zero.     */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int read_omaheaders (DEVBLK *dev, OMATAPE_DESC *omadesc,
                        BYTE *buf, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
long            blkpos;                 /* Offset to block header    */
S32             curblkl;                /* Length of current block   */
S32             prvhdro;                /* Offset of previous header */
S32             nxthdro;                /* Offset of next header     */

    /* Read the 16-byte block header */
    blkpos = dev->nxtblkpos;
    rc = readhdr_omaheaders (dev, omadesc, blkpos, &curblkl,
                            &prvhdro, &nxthdro, unitstat,code);
    if (rc < 0) return -1;

    /* Update the offsets of the next and previous blocks */
    dev->nxtblkpos = nxthdro;
    dev->prvblkpos = blkpos;

    /* Increment file number and return zero if tapemark */
    if (curblkl == -1)
    {
        close (dev->fd);
        dev->fd = -1;
        dev->curfilen++;
        dev->nxtblkpos = 0;
        dev->prvblkpos = -1;
        return 0;
    }

    /* Read data block from tape file */
    rc = read (dev->fd, buf, curblkl);

    /* Handle read error condition */
    if (rc < 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "read()", (off_t)blkpos,
                strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }

    /* Handle end of file within data block */
    if (rc < curblkl)
    {
        WRMSG(HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "read_omaheaders()", (off_t)blkpos, "unexpected end of file");

        /* Set unit check with data check and partial record */
        build_senseX(TAPE_BSENSE_BLOCKSHORT,dev,unitstat,code);
        return -1;
    }

    /* Return block length */
    return curblkl;

} /* end function read_omaheaders */

/*-------------------------------------------------------------------*/
/* Read a block from an OMA tape file in fixed block format          */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the file is closed, the current file      */
/* number in the device block is incremented so that the next file   */
/* will be opened by the next CCW, and the return value is zero.     */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int read_omafixed (DEVBLK *dev, OMATAPE_DESC *omadesc,
                        BYTE *buf, BYTE *unitstat,BYTE code)
{
off_t           rcoff;                  /* Return code from lseek()  */
int             blklen;                 /* Block length              */
long            blkpos;                 /* Offset of block in file   */

    /* Initialize current block position */
    blkpos = dev->nxtblkpos;

    /* Seek to new current block position */
    rcoff = lseek (dev->fd, blkpos, SEEK_SET);
    if (rcoff < 0)
    {
        /* Handle seek error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "lseek()", (off_t)blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }

    /* Read fixed length block or short final block */
    blklen = read (dev->fd, buf, omadesc->blklen);

    /* Handle read error condition */
    if (blklen < 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "read()", (off_t)blkpos,
                strerror(errno));

        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }

    /* At end of file return zero to indicate tapemark */
    if (blklen == 0)
    {
        close (dev->fd);
        dev->fd = -1;
        dev->curfilen++;
        dev->nxtblkpos = 0;
        dev->prvblkpos = -1;
        return 0;
    }

    /* Calculate the offsets of the next and previous blocks */
    dev->nxtblkpos = blkpos + blklen;
    dev->prvblkpos = blkpos;

    /* Return block length, or zero to indicate tapemark */
    return blklen;

} /* end function read_omafixed */

/*-------------------------------------------------------------------*/
/* Read a block from an OMA tape file in ASCII text format           */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the file is closed, the current file      */
/* number in the device block is incremented so that the next file   */
/* will be opened by the next CCW, and the return value is zero.     */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*                                                                   */
/* The buf parameter points to the I/O buffer during a read          */
/* operation, or is NULL for a forward space block operation.        */
/*-------------------------------------------------------------------*/
int read_omatext (DEVBLK *dev, OMATAPE_DESC *omadesc,
                        BYTE *buf, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return code from lseek()  */
int             num;                    /* Number of characters read */
int             pos;                    /* Position in I/O buffer    */
long            blkpos;                 /* Offset of block in file   */
BYTE            c;                      /* Character work area       */

    /* Initialize current block position */
    blkpos = dev->nxtblkpos;

    /* Seek to new current block position */
    rcoff = lseek (dev->fd, blkpos, SEEK_SET);
    if (rcoff < 0)
    {
        /* Handle seek error condition */
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "lseek()", (off_t)blkpos, strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }

    /* Read data from tape file until end of line */
    for (num = 0, pos = 0; ; )
    {
        rc = read (dev->fd, &c, 1);
        if (rc < 1) break;

        /* Treat X'1A' as end of file */
        if (c == '\x1A')
        {
            rc = 0;
            break;
        }

        /* Count characters read */
        num++;

        /* Ignore carriage return character */
        if (c == '\r') continue;

        /* Exit if newline character */
        if (c == '\n') break;

        /* Ignore characters in excess of I/O buffer length */
        if (pos >= MAX_TAPE_BLKSIZE) continue;

        /* Translate character to EBCDIC and copy to I/O buffer */
        if (buf != NULL)
            buf[pos] = host_to_guest(c);

        /* Count characters copied or skipped */
        pos++;

    } /* end for(num) */

    /* At end of file return zero to indicate tapemark */
    if (rc == 0 && num == 0)
    {
        close (dev->fd);
        dev->fd = -1;
        dev->curfilen++;
        dev->nxtblkpos = 0;
        dev->prvblkpos = -1;
        return 0;
    }

    /* Handle read error condition */
    if (rc < 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "read()", (off_t)blkpos,
                strerror(errno));

        build_senseX(TAPE_BSENSE_READFAIL,dev,unitstat,code);
        return -1;
    }

    /* Check for block not terminated by newline */
    if (rc < 1)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "read_omatext()", (off_t)blkpos, "unexpected end of file");

        /* Set unit check with data check and partial record */
        build_senseX(TAPE_BSENSE_BLOCKSHORT,dev,unitstat,code);
        return -1;
    }

    /* Check for invalid zero length block */
    if (pos == 0)
    {
        WRMSG (HHC00204, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "read_omatext()", (off_t)blkpos, "invalid block header");

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_BLOCKSHORT,dev,unitstat,code);
        return -1;
    }

    /* Calculate the offsets of the next and previous blocks */
    dev->nxtblkpos = blkpos + num;
    dev->prvblkpos = blkpos;

    /* Return block length */
    return pos;

} /* end function read_omatext */

/*-------------------------------------------------------------------*/
/* Read a block from an OMA - Selection of format done here          */
/*                                                                   */
/* If successful, return value is block length read.                 */
/* If a tapemark was read, the file is closed, the current file      */
/* number in the device block is incremented so that the next file   */
/* will be opened by the next CCW, and the return value is zero.     */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*                                                                   */
/* The buf parameter points to the I/O buffer during a read          */
/* operation, or is NULL for a forward space block operation.        */
/*-------------------------------------------------------------------*/
int read_omatape (DEVBLK *dev,
                        BYTE *buf, BYTE *unitstat,BYTE code)
{
int len;
OMATAPE_DESC *omadesc;
    omadesc = (OMATAPE_DESC*)(dev->omadesc);
    omadesc += (dev->curfilen-1);

    switch (omadesc->format)
    {
    default:
    case 'H':
        len = read_omaheaders (dev, omadesc, buf, unitstat,code);
        break;
    case 'F':
        len = read_omafixed (dev, omadesc, buf, unitstat,code);
        break;
    case 'T':
        len = read_omatext (dev, omadesc, buf, unitstat,code);
        break;
    case 'X':
        len=0;
        dev->curfilen++;
        break;
    case 'E':
        len=0;
        break;
    } /* end switch(omadesc->format) */

    if (len >= 0)
        dev->blockid++;

    return len;
}

/*-------------------------------------------------------------------*/
/* Forward space to next file of OMA tape device                     */
/*                                                                   */
/* For OMA tape devices, the forward space file operation is         */
/* achieved by closing the current file, and incrementing the        */
/* current file number in the device block, which causes the         */
/* next file will be opened when the next CCW is processed.          */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int fsf_omatape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
    UNREFERENCED(unitstat);
    UNREFERENCED(code);

    /* Close the current OMA file */
    if (dev->fd >= 0)
        close (dev->fd);
    dev->fd = -1;
    dev->nxtblkpos = 0;
    dev->prvblkpos = -1;

    /* Increment the current file number */
    dev->curfilen++;

    /* Return normal status */
    return 0;

} /* end function fsf_omatape */

/*-------------------------------------------------------------------*/
/* Forward space over next block of OMA file in OMA headers format   */
/*                                                                   */
/* If successful, return value is the length of the block skipped.   */
/* If the block skipped was a tapemark, the return value is zero,    */
/* the file is closed, and the current file number in the device     */
/* block is incremented so that the next file belonging to the OMA   */
/* tape will be opened when the next CCW is executed.                */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int fsb_omaheaders (DEVBLK *dev, OMATAPE_DESC *omadesc,
                        BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
long            blkpos;                 /* Offset of block header    */
S32             curblkl;                /* Length of current block   */
S32             prvhdro;                /* Offset of previous header */
S32             nxthdro;                /* Offset of next header     */

    /* Initialize current block position */
    blkpos = dev->nxtblkpos;

    /* Read the 16-byte block header */
    rc = readhdr_omaheaders (dev, omadesc, blkpos, &curblkl,
                            &prvhdro, &nxthdro, unitstat,code);
    if (rc < 0) return -1;

    /* Check if tapemark was skipped */
    if (curblkl == -1)
    {
        /* Close the current OMA file */
        if (dev->fd >= 0)
            close (dev->fd);
        dev->fd = -1;
        dev->nxtblkpos = 0;
        dev->prvblkpos = -1;

        /* Increment the file number */
        dev->curfilen++;

        /* Return zero to indicate tapemark */
        return 0;
    }

    /* Update the offsets of the next and previous blocks */
    dev->nxtblkpos = nxthdro;
    dev->prvblkpos = blkpos;

    /* Return block length */
    return curblkl;

} /* end function fsb_omaheaders */

/*-------------------------------------------------------------------*/
/* Forward space over next block of OMA file in fixed block format   */
/*                                                                   */
/* If successful, return value is the length of the block skipped.   */
/* If already at end of file, the return value is zero,              */
/* the file is closed, and the current file number in the device     */
/* block is incremented so that the next file belonging to the OMA   */
/* tape will be opened when the next CCW is executed.                */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int fsb_omafixed (DEVBLK *dev, OMATAPE_DESC *omadesc,
                        BYTE *unitstat,BYTE code)
{
off_t           eofpos;                 /* Offset of end of file     */
off_t           blkpos;                 /* Offset of current block   */
int             curblkl;                /* Length of current block   */

    /* Initialize current block position */
    blkpos = dev->nxtblkpos;

    /* Seek to end of file to determine file size */
    eofpos = lseek (dev->fd, 0, SEEK_END);
    if (eofpos < 0 || eofpos >= LONG_MAX)
    {
        /* Handle seek error condition */
        if ( eofpos >= LONG_MAX) errno = EOVERFLOW;
        WRMSG (HHC00205, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "lseek()", strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        return -1;
    }

    /* Check if already at end of file */
    if (blkpos >= eofpos)
    {
        /* Close the current OMA file */
        if (dev->fd >= 0)
            close (dev->fd);
        dev->fd = -1;
        dev->nxtblkpos = 0;
        dev->prvblkpos = -1;

        /* Increment the file number */
        dev->curfilen++;

        /* Return zero to indicate tapemark */
        return 0;
    }

    /* Calculate current block length */
    curblkl = (int)(eofpos - blkpos);
    if (curblkl > omadesc->blklen)
        curblkl = omadesc->blklen;

    /* Update the offsets of the next and previous blocks */
    dev->nxtblkpos = (long)(blkpos + curblkl);
    dev->prvblkpos = (long)(blkpos);

    /* Return block length */
    return curblkl;

} /* end function fsb_omafixed */

/*-------------------------------------------------------------------*/
/* Forward space to next block of OMA file                           */
/*                                                                   */
/* If successful, return value is the length of the block skipped.   */
/* If forward spaced over end of file, return value is 0.            */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int fsb_omatape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
OMATAPE_DESC   *omadesc;                /* -> OMA descriptor entry   */

    /* Point to the current file entry in the OMA descriptor table */
    omadesc = (OMATAPE_DESC*)(dev->omadesc);
    omadesc += (dev->curfilen-1);

    /* Forward space block depending on OMA file type */
    switch (omadesc->format)
    {
    default:
    case 'H':
        rc = fsb_omaheaders (dev, omadesc, unitstat,code);
        break;
    case 'F':
        rc = fsb_omafixed (dev, omadesc, unitstat,code);
        break;
    case 'T':
        rc = read_omatext (dev, omadesc, NULL, unitstat,code);
        break;
    } /* end switch(omadesc->format) */

    if (rc >= 0) dev->blockid++;

    return rc;

} /* end function fsb_omatape */

/*-------------------------------------------------------------------*/
/* Backspace to previous file of OMA tape device                     */
/*                                                                   */
/* If the current file number is 1, then backspace file simply       */
/* closes the file, setting the current position to start of tape.   */
/* Otherwise, the current file is closed, the current file number    */
/* is decremented, the new file is opened, and the new file is       */
/* repositioned to just before the tape mark at the end of the file. */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*-------------------------------------------------------------------*/
int bsf_omatape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
off_t           pos;                    /* File position             */
OMATAPE_DESC   *omadesc;                /* -> OMA descriptor entry   */
S32             curblkl;                /* Length of current block   */
S32             prvhdro;                /* Offset of previous header */
S32             nxthdro;                /* Offset of next header     */

    /* Close the current OMA file */
    if (dev->fd >= 0)
        close (dev->fd);
    dev->fd = -1;
    dev->nxtblkpos = 0;
    dev->prvblkpos = -1;

    /* Exit with tape at load point if currently on first file */
    if (dev->curfilen <= 1)
    {
        build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        return -1;
    }

    /* Decrement current file number */
    dev->curfilen--;

    /* Point to the current file entry in the OMA descriptor table */
    omadesc = (OMATAPE_DESC*)(dev->omadesc);
    omadesc += (dev->curfilen-1);

    /* Open the new current file */
    rc = open_omatape (dev, unitstat,code);
    if (rc < 0) return rc;

    /* Reposition before tapemark header at end of file, or
       to end of file for fixed block or ASCII text files */
    pos = 0;
    if ( 'H' == omadesc->format )
        pos -= sizeof(OMATAPE_BLKHDR);

    pos = lseek (dev->fd, pos, SEEK_END);
    if (pos < 0)
    {
        /* Handle seek error condition */
        WRMSG (HHC00205, "E", LCSS_DEVNUM, omadesc->filename, "OMA", "lseek()", strerror(errno));

        /* Set unit check with equipment check */
        build_senseX(TAPE_BSENSE_LOCATEERR,dev,unitstat,code);
        dev->sense[0] = SENSE_EC;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return -1;
    }
    dev->nxtblkpos = pos;
    dev->prvblkpos = -1;

    /* Determine the offset of the previous block */
    switch (omadesc->format)
    {
    case 'H':
        /* For OMA headers files, read the tapemark header
           and extract the previous block offset */
        rc = readhdr_omaheaders (dev, omadesc, pos, &curblkl,
                                &prvhdro, &nxthdro, unitstat,code);
        if (rc < 0) return -1;
        dev->prvblkpos = prvhdro;
        break;
    case 'F':
        /* For OMA fixed block files, calculate the previous block
           offset allowing for a possible short final block */
        pos = (pos + omadesc->blklen - 1) / omadesc->blklen;
        dev->prvblkpos = (pos > 0 ? (pos - 1) * omadesc->blklen : -1);
        break;
    case 'T':
        /* For OMA ASCII text files, the previous block is unknown */
        dev->prvblkpos = -1;
        break;
    } /* end switch(omadesc->format) */

    /* Return normal status */
    return 0;

} /* end function bsf_omatape */

/*-------------------------------------------------------------------*/
/* Backspace to previous block of OMA file                           */
/*                                                                   */
/* If successful, return value is +1.                                */
/* If current position is at start of a file, then a backspace file  */
/* operation is performed to reset the position to the end of the    */
/* previous file, and the return value is zero.                      */
/* If error, return value is -1 and unitstat is set to CE+DE+UC      */
/*                                                                   */
/* Note that for ASCII text files, the previous block position is    */
/* known only if the previous CCW was a read or a write, so any      */
/* attempt to issue more than one consecutive backspace block on     */
/* an ASCII text file will fail with unit check status.              */
/*-------------------------------------------------------------------*/
int bsb_omatape (DEVBLK *dev, BYTE *unitstat,BYTE code)
{
int             rc;                     /* Return code               */
OMATAPE_DESC   *omadesc;                /* -> OMA descriptor entry   */
long            blkpos;                 /* Offset of block header    */
S32             curblkl;                /* Length of current block   */
S32             prvhdro = 0;            /* Offset of previous header */
S32             nxthdro;                /* Offset of next header     */

    /* Point to the current file entry in the OMA descriptor table */
    omadesc = (OMATAPE_DESC*)(dev->omadesc);
    omadesc += (dev->curfilen-1);

    /* Backspace file if current position is at start of file */
    if (dev->nxtblkpos == 0)
    {
        /* Unit check if already at start of tape */
        if (dev->curfilen <= 1)
        {
            build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
            return -1;
        }

        /* Perform backspace file operation */
        rc = bsf_omatape (dev, unitstat,code);
        if (rc < 0) return -1;

        dev->blockid--;

        /* Return zero to indicate tapemark detected */
        return 0;
    }

    /* Unit check if previous block position is unknown */
    if (dev->prvblkpos < 0)
    {
        build_senseX(TAPE_BSENSE_LOADPTERR,dev,unitstat,code);
        return -1;
    }

    /* Backspace to previous block position */
    blkpos = dev->prvblkpos;

    /* Determine new previous block position */
    switch (omadesc->format)
    {
    case 'H':
        /* For OMA headers files, read the previous block header to
           extract the block length and new previous block offset */
        rc = readhdr_omaheaders (dev, omadesc, blkpos, &curblkl,
                                &prvhdro, &nxthdro, unitstat,code);
        if (rc < 0) return -1;
        break;
    case 'F':
        /* For OMA fixed block files, calculate the new previous
           block offset by subtracting the fixed block length */
        if (blkpos >= omadesc->blklen)
            prvhdro = blkpos - omadesc->blklen;
        else
            prvhdro = -1;
        break;
    case 'T':
        /* For OMA ASCII text files, new previous block is unknown */
        prvhdro = -1;
        break;
    } /* end switch(omadesc->format) */

    /* Update the offsets of the next and previous blocks */
    dev->nxtblkpos = blkpos;
    dev->prvblkpos = prvhdro;

    dev->blockid--;

    /* Return +1 to indicate backspace successful */
    return +1;

} /* end function bsb_omatape */

/*-------------------------------------------------------------------*/
/* Close an OMA tape file set                                        */
/*                                                                   */
/* All errors are ignored                                            */
/*-------------------------------------------------------------------*/
void close_omatape2(DEVBLK *dev)
{
    if (dev->fd >= 0)
    {
        WRMSG (HHC00201, "I", LCSS_DEVNUM, dev->filename, "OMA");
        close (dev->fd);
    }
    dev->fd=-1;
    if (dev->omadesc != NULL)
    {
        free (dev->omadesc);
        dev->omadesc = NULL;
    }

    /* Reset the device dependent fields */
    dev->nxtblkpos=0;
    dev->prvblkpos=-1;
    dev->curfilen=1;
    dev->blockid=0;
    dev->fenced = 0;
    dev->omafiles = 0;
    return;
}

/*-------------------------------------------------------------------*/
/* Close an OMA tape file set                                        */
/*                                                                   */
/* All errors are ignored                                            */
/* Change the filename to '*' - unloaded                             */
/* TAPE REALLY UNLOADED                                              */
/*-------------------------------------------------------------------*/
void close_omatape(DEVBLK *dev)
{
    close_omatape2(dev);
    STRLCPY( dev->filename, TAPE_UNLOADED );
    dev->blockid = 0;
    dev->fenced = 0;
    return;
}

/*-------------------------------------------------------------------*/
/* Rewind an OMA tape file set                                       */
/*                                                                   */
/* All errors are ignored                                            */
/*-------------------------------------------------------------------*/
int rewind_omatape(DEVBLK *dev,BYTE *unitstat,BYTE code)
{
    UNREFERENCED(unitstat);
    UNREFERENCED(code);
    close_omatape2(dev);
    dev->fenced = 0;
    return 0;
}
