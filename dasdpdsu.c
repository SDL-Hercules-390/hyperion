/* DASDPDSU.C   (c) Copyright Roger Bowler, 1999-2012                */
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
/*      dasdpdsu ckdfile dsname [ascii]                              */
/* where: ckdfile is the name of the CKD image file                  */
/*        dsname is the name of the PDS to be unloaded               */
/*        ascii is an optional keyword which will cause the members  */
/*        to be unloaded as ASCII variable length text files.        */
/* Each member is copied to a file memname.mac in the current        */
/* working directory. If the ascii keyword is not specified then     */
/* the members are unloaded as fixed length binary files.            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#include "hercules.h"
#include "dasdblks.h"

#define UTILITY_NAME    "dasdpdsu"

static int process_member( CIFBLK *, int, DSXTENT *, char *, BYTE * );
static int process_dirblk( CIFBLK *, int, DSXTENT *, BYTE * );

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
static BYTE asciiflag = 0;              /* 1=Translate to ASCII      */
static  BYTE eighthexFF[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

/*-------------------------------------------------------------------*/
/* DASDPDSU main entry point                                         */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
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

    INITIALIZE_UTILITY( UTILITY_NAME,  "PDS unload", &pgm );

    /* Check the number of arguments */
    if (argc < 3 || argc > 5)
    {
        FWRMSG( stderr, HHC02463, "I", pgm );
        return -1;
    }

    /* The first argument is the name of the CKD image file */
    fname = argv[1];

    /* The next argument may be the shadow file name */
    if (!memcmp (argv[2], "sf=", 3))
    {
        sfname = argv[2];
        i = 1;
    }

    /* The second argument is the dataset name */
    memset (dsnama, 0, sizeof(dsnama));
    strncpy (dsnama, argv[2|+i], sizeof(dsnama)-1);
    string_to_upper (dsnama);

    /* The third argument is an optional keyword */
    if (argc > 3+i && argv[3+i] != NULL)
    {
        if (strcasecmp(argv[3+i], "ascii") == 0)
            asciiflag = 1;
        else
        {
            FWRMSG( stderr, HHC02465, "E", argv[3+i] );
            FWRMSG( stderr, HHC02463, "I", argv[0] );
            return -1;
        }
    }

    /* Open the CKD image file */
    cif = open_ckd_image (fname, sfname, O_RDONLY|O_BINARY, IMAGE_OPEN_NORMAL);
    if (cif == NULL) return -1;

    /* Build the extent array for the requested dataset */
    rc = build_extent_array (cif, dsnama, extent, &noext);
    if (rc < 0) return -1;

#ifdef EXTERNALGUI
    /* Calculate ending relative track */
    if (extgui)
    {
        int bcyl;  /* Extent begin cylinder     */
        int btrk;  /* Extent begin head         */
        int ecyl;  /* Extent end cylinder       */
        int etrk;  /* Extent end head           */
        int trks;  /* total tracks in dataset   */
        int i;     /* loop control              */

        for (i = 0, trks = 0; i < noext; i++)
        {
            bcyl = (extent[i].xtbcyl[0] << 8) | extent[i].xtbcyl[1];
            btrk = (extent[i].xtbtrk[0] << 8) | extent[i].xtbtrk[1];
            ecyl = (extent[i].xtecyl[0] << 8) | extent[i].xtecyl[1];
            etrk = (extent[i].xtetrk[0] << 8) | extent[i].xtetrk[1];

            trks += (((ecyl * cif->heads) + etrk) - ((bcyl * cif->heads) + btrk)) + 1;
        }

        EXTGUIMSG( "ETRK=%d\n", trks-1 );
    }
#endif /*EXTERNALGUI*/

    /* Point to the start of the directory */
    trk = 0;
    rec = 1;

    /* Read the directory */
    while (1)
    {
        EXTGUIMSG( "CTRK=%d\n", trk );

        /* Convert relative track to cylinder and head */
        rc = convert_tt (trk, noext, extent, cif->heads, &cyl, &head);
        if (rc < 0) return -1;

        /* Read a directory block */
        WRMSG( HHC02466, "I", cyl, head, rec );

        rc = read_block (cif, cyl, head, rec,
                        NULL, NULL, &blkptr, &len);
        if (rc < 0) return -1;

        /* Move to next track if block not found */
        if (rc > 0)
        {
            trk++;
            rec = 1;
            continue;
        }

        /* Exit at end of directory */
        if (len == 0) break;

        /* Copy the directory block */
        memcpy (dirblk, blkptr, sizeof(dirblk));

        /* Process each member in the directory block */
        rc = process_dirblk (cif, noext, extent, dirblk);
        if (rc < 0) return -1;
        if (rc > 0) break;

        /* Point to the next directory block */
        rec++;

    } /* end while */

    WRMSG( HHC02467, "I" );

    /* Close the CKD image file and exit */
    rc = close_ckd_image (cif);
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
process_member (CIFBLK *cif, int noext, DSXTENT extent[],
                char *memname, BYTE *ttr)
{
int             rc;                     /* Return code               */
U16             len;                    /* Record length             */
u_int           trk;                    /* Relative track number     */
U32             cyl;                    /* Cylinder number           */
U8              head;                   /* Head number               */
U8              rec;                    /* Record number             */
BYTE           *buf;                    /* -> Data block             */
FILE           *ofp;                    /* Output file pointer       */
char            ofname[256];            /* Output file name          */
int             offset;                 /* Offset of record in buffer*/
char            card[81];               /* Logical record (ASCIIZ)   */
char            pathname[MAX_PATH];     /* ofname in host format     */

    /* Build the output file name */
    memset (ofname, 0, sizeof(ofname));
    strncpy (ofname, memname, 8);
    string_to_lower (ofname);
    strlcat (ofname, ".mac", sizeof(ofname));

    /* Open the output file */
    hostpath(pathname, ofname, sizeof(pathname));
    ofp = fopen (pathname, (asciiflag? "w" : "wb"));
    if (ofp == NULL)
    {
        FWRMSG( stderr, HHC02468, "E", ofname, "fopen", strerror( errno ));
        return -1;
    }

    /* Point to the start of the member */
    trk = (ttr[0] << 8) | ttr[1];
    rec = ttr[2];

    WRMSG( HHC02469, "I", memname, trk, rec );

    /* Read the member */
    while (1)
    {
        /* Convert relative track to cylinder and head */
        rc = convert_tt (trk, noext, extent, cif->heads, &cyl, &head);
        if (rc < 0) return -1;

//      fprintf (stderr,
//              "CCHHR=%4.4X%4.4X%2.2X\n",
//              cyl, head, rec);

        /* Read a data block */
        rc = read_block (cif, cyl, head, rec, NULL, NULL, &buf, &len);
        if (rc < 0) return -1;

        /* Move to next track if record not found */
        if (rc > 0)
        {
            trk++;
            rec = 1;
            continue;
        }

        /* Exit at end of member */
        if (len == 0) break;

        /* Check length of data block */
        if (len % 80 != 0)
        {
            FWRMSG( stderr, HHC02470, "E", len, cyl, head, rec );
            return -1;
        }

        /* Process each record in the data block */
        for (offset = 0; offset < len; offset += 80)
        {
            if (asciiflag)
            {
                make_asciiz (card, sizeof(card), buf + offset, 72);
                fprintf (ofp, "%s\n", card);
            }
            else
            {
                fwrite (buf+offset, 80, 1, ofp);
            }

            if (ferror(ofp))
            {
                FWRMSG( stderr, HHC02468, "E",
                    ofname, "fwrite", strerror( errno ));
                return -1;
            }
        } /* end for(offset) */

        /* Point to the next data block */
        rec++;

    } /* end while */

    /* Close the output file and exit */
    fclose (ofp);
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
process_dirblk (CIFBLK *cif, int noext, DSXTENT extent[], BYTE *dirblk)
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
    dirrem = (dirptr[0] << 8) | dirptr[1];
    if (dirrem < 2 || dirrem > 256)
    {
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
        if (memcmp(dirent->pds2name, eighthexFF, 8) == 0)
            return +1;

        /* Extract the member name */
        make_asciiz (memname, sizeof(memname), dirent->pds2name, 8);

        /* Process the member */
        rc = process_member (cif, noext, extent, memname,
                            dirent->pds2ttrp);
        if (rc < 0) return -1;

        /* Load the user data halfword count */
        k = dirent->pds2indc & PDS2INDC_LUSR;

        /* Point to next directory entry */
        size = 12 + k*2;
        dirptr += size;
        dirrem -= size;
    }

    return 0;
} /* end function process_dirblk */

