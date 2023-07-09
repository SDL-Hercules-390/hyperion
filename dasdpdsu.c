/* DASDPDSU.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules DASD Utilities: PDS unloader                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program unloads members of a partitioned dataset from        */
/* a virtual DASD volume and copies each member to a flat file.      */
/*                                                                   */
/* The command format is:                                            */
/*      dasdpdsu ckdfile [sf=shadow-file-name] dsname [ASCII] [odir] */
/* where: ckdfile is the name of the CKD image file                  */
/*        dsname is the name of the PDS to be unloaded               */
/*        ASCII is an optional keyword which will cause the members  */
/*        to be unloaded as ASCII variable length text files         */
/*        odir is the directory where the output should be written   */
/*        if not specified o/p is written to the current directory   */
/* Each member is copied to a file memname.mac in the current        */
/* working directory. If the ascii keyword is not specified then     */
/* the members are unloaded as fixed length binary files.            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"

#define UTILITY_NAME    "dasdpdsu"
#define UTILITY_DESC    "PDS unload"

static int process_member( CIFBLK *, int, DSXTENT *, char *, BYTE * );
static int process_dirblk( CIFBLK *, int, DSXTENT *, BYTE * );

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
static BYTE asciiflag = 0;              /* 1=Translate to ASCII      */
static char odir[ MAX_PATH ] = {0};     /* output directory name     */

/*-------------------------------------------------------------------*/
/* Syntax help                                                       */
/*-------------------------------------------------------------------*/
static int syntax( const char* pgm )
{
    // "Usage: %s ckdfile [sf=shadow-file-name] pdsname [ASCII] [odir]"
    FWRMSG( stderr, HHC02975, "I", pgm );
    return -1;
}

/*-------------------------------------------------------------------*/
/* DASDPDSU main entry point                                         */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
char           *pgm;                    /* less any extension (.ext) */
int             rc;                     /* Return code               */
int             i=0;                    /* Arument index             */
U16             len;                    /* Record length             */
U32             cyl;                    /* Cylinder number           */
U8              head;                   /* Head number               */
U8              rec;                    /* Record number             */
u_int           trk;                    /* Relative track number     */
char           *fname;                  /* -> CKD image file name    */
char           *sfname=NULL;            /* -> CKD shadow file name   */
char            dsnama[45];             /* Dataset name (ASCIIZ)     */
int             noext;                  /* Number of extents         */
DSXTENT         extent[16];             /* Extent descriptor array   */
BYTE           *blkptr;                 /* -> PDS directory block    */
BYTE            dirblk[256];            /* Copy of directory block   */
CIFBLK         *cif;                    /* CKD image file descriptor */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Check the number of arguments */
    if (argc < 3 || argc > 6)
        return syntax( pgm );

    /* The first argument is the name of the CKD image file */
    fname = argv[1];

    /* The next argument may be the shadow file name */
    if (!memcmp (argv[2], "sf=", 3))
    {
        if (argc < 4)
        {
            // "%s missing"
            FWRMSG( stderr, HHC02578, "E", "required 'pdsname' argument" );
            return syntax( pgm );
        }

        sfname = argv[2];
        i = 1;
    }

    /* The second argument is the dataset name */
    STRLCPY( dsnama, argv[2+i] );
    string_to_upper( dsnama );

    /* The third argument is an optional keyword */
    if (argc > (3+i) && argv[3+i])
    {
        if (strcasecmp( argv[3+i], "ASCII" ) == 0)
            asciiflag = 1;
        else
            i = -1;
    }

    /* The forth (or third!) argument is an optional output directory */
    if (argc > (4+i) && argv[4+i])
    {
        struct stat statbuf;
        char pathname[MAX_PATH], c = 0;
        int dirlen = 0;

        /* Copy argument to work */
        STRLCPY( odir, argv[4+i] );

#if defined( _MSVC_ )
        /* stat(): If path contains the location of a directory,
           it cannot contain a trailing backslash.  If it does,
           -1 will be returned and errno will be set to ENOENT. */
        dirlen = (int) strlen( odir );
        if (0
            || odir[ dirlen - 1 ] == '/'
            || odir[ dirlen - 1 ] == '\\'
        )
        {
            c = odir[ dirlen - 1 ];   // (remember)
            odir[ dirlen - 1 ] = 0;   // (remove it)
        }
#endif
        /* Valid directory name specified? */
        if (0
            || stat( odir, &statbuf ) != 0
            || !S_ISDIR( statbuf.st_mode )
        )
        {
            // "Invalid argument: %s"
            FWRMSG( stderr, HHC02465, "E", argv[4+i] );
            return syntax( pgm );
        }

        /* (restore trailing path separator if removed) */
        if (c)
            odir[ dirlen - 1 ] = c;

        /* Convert output directory to host format */
        hostpath( pathname, odir, sizeof( pathname ));
        STRLCPY( odir, pathname );

        /* Append path separator if needed */
        if (odir[ strlen( odir ) - 1 ] != PATHSEPC)
            STRLCAT( odir, PATHSEPS );
    }

    /* Open the CKD image file */
    if (!(cif = open_ckd_image( fname, sfname, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL )))
        return -1;

    /* Build the extent array for the requested dataset */
    if ((rc = build_extent_array( cif, dsnama, extent, &noext )) < 0)
        return -1;

    /* Calculate ending relative track */
    if (extgui)
    {
        int bcyl;  /* Extent begin cylinder     */
        int btrk;  /* Extent begin head         */
        int ecyl;  /* Extent end cylinder       */
        int etrk;  /* Extent end head           */
        int trks;  /* total tracks in dataset   */
        int i;     /* loop control              */

        for (i=0, trks = 0; i < noext; i++)
        {
            bcyl = fetch_hw( extent[i].xtbcyl );
            btrk = fetch_hw( extent[i].xtbtrk );
            ecyl = fetch_hw( extent[i].xtecyl );
            etrk = fetch_hw( extent[i].xtetrk );

            trks += (((ecyl * cif->heads) + etrk) - ((bcyl * cif->heads) + btrk)) + 1;
        }

        EXTGUIMSG( "ETRK=%d\n", trks-1 );
    }

    /* Point to the start of the directory */
    trk = 0;
    rec = 1;

    /* Read the directory */
    while (1)
    {
        EXTGUIMSG( "CTRK=%d\n", trk );

        /* Convert relative track to cylinder and head */
        if ((rc = convert_tt( trk, noext, extent, cif->heads, &cyl, &head )) < 0)
            return -1;

        /* Read a directory block */
        WRMSG( HHC02466, "I", cyl, head, rec );

        if ((rc = read_block( cif, cyl, head, rec, NULL, NULL, &blkptr, &len )) < 0)
            return -1;

        /* Move to next track if block not found */
        if (rc > 0)
        {
            trk++;
            rec = 1;
            continue;
        }

        /* Exit at end of directory */
        if (!len)
            break;

        /* Copy the directory block */
        memcpy( dirblk, blkptr, sizeof( dirblk ));

        /* Process each member in the directory block */
        if ((rc = process_dirblk( cif, noext, extent, dirblk )) < 0)
            return -1;

        /* Exit at end of directory */
        if (rc > 0)
            break;

        /* Point to the next directory block */
        rec++;

    } /* end while */

    WRMSG( HHC02467, "I" );

    /* Close the CKD image file and exit */
    rc = close_ckd_image( cif );
    return rc;

} /* end function main */

/*-------------------------------------------------------------------*/
/* Subroutine to process a member                                    */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      noext   Number of extents in dataset                         */
/*      extent  Dataset extent array                                 */
/*      memname Member name (ASCIIZ)                                 */
/*      ttr     Member TTR                                           */
/*                                                                   */
/* Return value is 0 if successful, or -1 if error                   */
/*-------------------------------------------------------------------*/
static int
process_member( CIFBLK* cif, int noext, DSXTENT extent[],
                char* memname, BYTE* ttr )
{
int             rc;                     /* Return code               */
U16             len;                    /* Record length             */
u_int           trk;                    /* Relative track number     */
U32             cyl;                    /* Cylinder number           */
U8              head;                   /* Head number               */
U8              rec;                    /* Record number             */
BYTE           *buf;                    /* -> Data block             */
FILE           *ofp;                    /* Output file pointer       */
char            ofname[13] = {0};       /* Output file name          */
int             offset;                 /* Offset of record in buffer*/
char            card[81];               /* Logical record (ASCIIZ)   */
char            workpath[MAX_PATH];     /* full path of ofname       */
char            pathname[MAX_PATH];     /* ofname in host format     */

    /* Build the output file name */
    strncpy( ofname, memname, sizeof(ofname) );
    ofname[8]=0; /* force string termination */
    string_to_lower( ofname );
    STRLCAT( ofname, ".mac" );

    /* Build the full path of the output filename */
    STRLCPY( workpath, odir );
    STRLCAT( workpath, ofname );

    /* Convert output path to host format */
    hostpath( pathname, workpath, sizeof( pathname ));

    /* Open the output file */
    if (!(ofp = fopen( pathname, (asciiflag? "w" : "wb"))))
    {
        // "File %s; %s error: %s"
        FWRMSG( stderr, HHC02468, "E", ofname, "fopen", strerror( errno ));
        return -1;
    }

    /* Point to the start of the member */
    trk = fetch_hw( ttr );
    rec = ttr[2];

    // "Member %s TTR %04X%02X"
    WRMSG( HHC02469, "I", memname, trk, rec );

    /* Read the member */
    while (1)
    {
        /* Convert relative track to cylinder and head */
        if ((rc = convert_tt( trk, noext, extent, cif->heads, &cyl, &head )) < 0)
            return -1;

        TRACE( "CCHHR=%4.4X%4.4X%2.2X\n", cyl, head, rec );

        /* Read a data block */
        if ((rc = read_block( cif, cyl, head, rec, NULL, NULL, &buf, &len )) < 0)
            return -1;

        /* Move to next track if record not found */
        if (rc > 0)
        {
            trk++;
            rec = 1;
            continue;
        }

        /* Exit at end of member */
        if (!len)
            break;

        /* Check length of data block */
        if (len % 80 != 0)
        {
            // "Invalid block length %d at CCHHR %04X%04X%02X"
            FWRMSG( stderr, HHC02470, "E", len, cyl, head, rec );
            return -1;
        }

        /* Process each record in the data block */
        for (offset = 0; offset < len; offset += 80)
        {
            if (asciiflag)
            {
                make_asciiz( card, sizeof( card ), buf + offset, 72 );
                fprintf( ofp, "%s\n", card );
            }
            else
                fwrite( buf+offset, 80, 1, ofp );

            if (ferror( ofp ))
            {
                // "File %s; %s error: %s"
                FWRMSG( stderr, HHC02468, "E",
                    ofname, "fwrite", strerror( errno ));
                return -1;
            }
        } /* end for(offset) */

        /* Point to the next data block */
        rec++;

    } /* end while */

    /* Close the output file and exit */
    fclose( ofp );
    return 0;

} /* end function process_member */

/*-------------------------------------------------------------------*/
/* Subroutine to process a directory block                           */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      noext   Number of extents in dataset                         */
/*      extent  Dataset extent array                                 */
/*      dirblk  Pointer to directory block                           */
/*                                                                   */
/* Return value is 0 if OK, +1 if end of directory, or -1 if error   */
/*-------------------------------------------------------------------*/
static int
process_dirblk( CIFBLK* cif, int noext, DSXTENT extent[], BYTE* dirblk )
{
int             rc;                     /* Return code               */
int             size;                   /* Size of directory entry   */
int             k;                      /* Userdata halfword count   */
BYTE           *dirptr;                 /* -> Next byte within block */
int             dirrem;                 /* Number of bytes remaining */
PDSDIR         *dirent;                 /* -> Directory entry        */
char            memname[9];             /* Member name (ASCIIZ)      */

    /* Load number of bytes in directory block */
    dirptr = dirblk;
    dirrem = fetch_hw( dirptr );
    if (dirrem < 2 || dirrem > 256)
    {
        // "Directory block byte count is invalid"
        FWRMSG( stderr, HHC02400, "E" );
        return -1;
    }

    /* Point to first directory entry */
    dirptr += 2;
    dirrem -= 2;

    /* Process each directory entry */
    while (dirrem > 0)
    {
        /* Point to next directory entry */
        dirent = (PDSDIR*)dirptr;

        /* Test for end of directory */
        if (memcmp( dirent->pds2name, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
            return +1;

        /* Extract the member name */
        make_asciiz( memname, sizeof( memname ), dirent->pds2name, 8 );

        /* Process the member */
        if ((rc = process_member( cif, noext, extent, memname, dirent->pds2ttrp )) < 0)
            return -1;

        /* Load the user data halfword count */
        k = dirent->pds2indc & PDS2INDC_LUSR;

        /* Point to next directory entry */
        size = 12 + (k * 2);

        dirptr += size;
        dirrem -= size;
    }

    return 0;
} /* end function process_dirblk */
