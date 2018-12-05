/* DASDISUP.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules DASD Utilities: IEHIOSUP                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program performs the IEHIOSUP function of OS/360.            */
/* It adjusts the TTRs in the XCTL tables in each of the             */
/* Open/Close/EOV modules in SYS1.SVCLIB.                            */
/*                                                                   */
/* The command format is:                                            */
/*      dasdisup ckdfile                                             */
/* where: ckdfile is the name of the CKD image file                  */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#include "hercules.h"
#include "dasdblks.h"

#define UTILITY_NAME    "dasdisup"
#define UTILITY_DESC    "IEHIOSUP"

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define HEX00           ((BYTE)'\x00')  /* EBCDIC low value          */
#define HEX40           ((BYTE)'\x40')  /* EBCDIC space              */
#define HEXFF           ((BYTE)'\xFF')  /* EBCDIC high value         */
#define OVERPUNCH_ZERO  ((BYTE)'\xC0')  /* EBCDIC 12-0 card punch    */

/*-------------------------------------------------------------------*/
/* Definition of member information array entry                      */
/*-------------------------------------------------------------------*/
typedef struct _MEMINFO {
        BYTE    memname[8];             /* Member name (EBCDIC)      */
        BYTE    ttrtext[3];             /* TTR of first text record  */
        BYTE    dwdsize;                /* Text size in doublewords  */
        BYTE    alias;                  /* 1=This is an alias        */
        BYTE    notable;                /* 1=Member has no XCTL table*/
        BYTE    multitxt;               /* 1=Member has >1 text rcd  */
    } MEMINFO;

#define MAX_MEMBERS             1000    /* Size of member info array */

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/

/* List of first loads for Open/Close/EOV routines */
static char *firstload[] = {
        "IGC0001I",                     /* Open (SVC19)              */
        "IGC0002{",                     /* Close (SVC20)             */
        "IGC0002A",                     /* Stow (SVC21)              */
        "IGC0002B",                     /* OpenJ (SVC22)             */
        "IGC0002C",                     /* TClose (SVC23)            */
        "IGC0002I",                     /* Scratch (SVC29)           */
        "IGC0003A",                     /* FEOV (SVC31)              */
        "IGC0003B",                     /* Allocate (SVC32)          */
        "IGC0005E",                     /* EOV (SVC55)               */
        "IGC0008A",                     /* Setprt (SVC81)            */
        "IGC0008F",                     /* Atlas (SVC86)             */
        "IGC0009C",                     /* TSO (SVC93)               */
        "IGC0009D",                     /* TSO (SVC94)               */
        NULL };                         /* End of list               */

/* List of second loads for Open/Close/EOV routines */
static char *secondload[] = {
        "IFG019", "IGG019",             /* Open (SVC19)              */
        "IFG020", "IGG020",             /* Close (SVC20)             */
        "IGG021",                       /* Stow (SVC21)              */
        "IFG023", "IGG023",             /* TClose (SVC23)            */
        "IGG029",                       /* Scratch (SVC29)           */
        "IGG032",                       /* Allocate (SVC32)          */
        "IFG055", "IGG055",             /* EOV (SVC55)               */
        "IGG081",                       /* Setprt (SVC81)            */
        "IGG086",                       /* Atlas (SVC86)             */
        "IGG093",                       /* TSO (SVC93)               */
        "IGG094",                       /* TSO (SVC94)               */
        NULL };                         /* End of list               */

static int process_dirblk( CIFBLK *cif, int noext, DSXTENT extent[],
                           BYTE *dirblk, MEMINFO memtab[], int *nmem );
static int resolve_xctltab( CIFBLK *cif, int noext, DSXTENT extent[],
                            MEMINFO *memp, MEMINFO memtab[], int nmem );

/*-------------------------------------------------------------------*/
/* DASDISUP main entry point                                         */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
U16             len;                    /* Record length             */
U32             cyl;                    /* Cylinder number           */
U8              head;                   /* Head number               */
U8              rec;                    /* Record number             */
u_int           trk;                    /* Relative track number     */
char           *fname;                  /* -> CKD image file name    */
char           *sfname;                 /* -> CKD shadow file name   */
int             noext;                  /* Number of extents         */
DSXTENT         extent[16];             /* Extent descriptor array   */
BYTE           *blkptr;                 /* -> PDS directory block    */
CIFBLK         *cif;                    /* CKD image file descriptor */
MEMINFO        *memtab;                 /* -> Member info array      */
int             nmem = 0;               /* Number of array entries   */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Check the number of arguments */
    if (argc < 2 || argc > 3)
    {
        // "Usage: %s ...
        WRMSG( HHC02463, "I", pgm );
        return -1;
    }

    /* The first argument is the name of the CKD image file */
    fname = argv[1];

    /* The next argument, if there, is the name of the shadow file */
    if (argc > 2) sfname = argv[2];
    else sfname = NULL;

    /* Obtain storage for the member information array */
    memtab = (MEMINFO*) calloc( MAX_MEMBERS, sizeof(MEMINFO) );
    if (memtab == NULL)
    {
        char buf[80];
        MSGBUF( buf, "calloc(%d,%d)", MAX_MEMBERS, (int)(sizeof( MEMINFO )));
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", buf, strerror( errno ));
        return -1;
    }

    /* Open the CKD image file */
    cif = open_ckd_image (fname, sfname, O_RDWR|O_BINARY, IMAGE_OPEN_NORMAL);
    if (cif == NULL) return -1;

    /* Build the extent array for the SVCLIB dataset */
    rc = build_extent_array (cif, "SYS1.SVCLIB", extent, &noext);
    if (rc < 0) return -1;

    /* Point to the start of the directory */
    trk = 0;
    rec = 1;

    /* Read the directory */
    while (1)
    {
        /* Convert relative track to cylinder and head */
        rc = convert_tt (trk, noext, extent, cif->heads, &cyl, &head);
        if (rc < 0) return -1;

        /* Read a directory block */
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

        /* Extract information for each member in directory block */
        rc = process_dirblk (cif, noext, extent, blkptr,
                            memtab, &nmem);
        if (rc < 0) return -1;
        if (rc > 0) break;

        /* Point to the next directory block */
        rec++;

    } /* end while */

    // "End of directory, %d members selected"
    WRMSG( HHC02464, "I", nmem );

    EXTGUIMSG( "NMEM=%d\n", nmem );

    /* Read each member and resolve the embedded TTRs */
    for (i = 0; i < nmem; i++)
    {
        EXTGUIMSG( "MEM=%d\n", i );
        rc = resolve_xctltab (cif, noext, extent, memtab+i,
                                memtab, nmem);

    } /* end for(i) */

    /* Close the CKD image file and exit */
    rc = close_ckd_image (cif);
    free (memtab);
    return rc;

} /* end function main */

/*-------------------------------------------------------------------*/
/* Subroutine to process a directory block                           */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      noext   Number of extents in dataset                         */
/*      extent  Dataset extent array                                 */
/*      dirblk  Pointer to directory block                           */
/* Input/output:                                                     */
/*      memtab  Member information array                             */
/*      nmem    Number of entries in member information array        */
/*                                                                   */
/* Directory information for each member is extracted from the       */
/* directory block and saved in the member information array.        */
/*                                                                   */
/* Return value is 0 if OK, +1 if end of directory, or -1 if error   */
/*-------------------------------------------------------------------*/
static int
process_dirblk (CIFBLK *cif, int noext, DSXTENT extent[],
                BYTE *dirblk, MEMINFO memtab[], int *nmem)
{
int             n;                      /* Member array subscript    */
int             i;                      /* Array subscript           */
int             totlen;                 /* Total module length       */
int             txtlen;                 /* Length of 1st text block  */
int             size;                   /* Size of directory entry   */
int             k;                      /* Userdata halfword count   */
BYTE           *dirptr;                 /* -> Next byte within block */
int             dirrem;                 /* Number of bytes remaining */
PDSDIR         *dirent;                 /* -> Directory entry        */
char            memnama[9];             /* Member name (ASCIIZ)      */

    UNREFERENCED(cif);
    UNREFERENCED(noext);
    UNREFERENCED(extent);

    /* Load number of bytes in directory block */
    dirptr = dirblk;
    dirrem = (dirptr[0] << 8) | dirptr[1];
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
    for (n = *nmem; dirrem > 0; )
    {
        /* Point to next directory entry */
        dirent = (PDSDIR*)dirptr;

        /* Test for end of directory */
        if (memcmp( dirent->pds2name, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
            return +1;

        /* Load the user data halfword count */
        k = dirent->pds2indc & PDS2INDC_LUSR;

        /* Point to next directory entry */
        size = 12 + k*2;
        dirptr += size;
        dirrem -= size;

        /* Extract the member name */
        make_asciiz (memnama, sizeof(memnama), dirent->pds2name, 8);
        if (dirent->pds2name[7] == OVERPUNCH_ZERO)
            memnama[7] = '{';

        /* Find member in first load table */
        for (i = 0; firstload[i] != NULL; i++)
            if (strcmp(memnama, firstload[i]) == 0) break;

        /* If not in first table, find in second table */
        if (firstload[i] == NULL)
        {
            for (i = 0; secondload[i] != NULL; i++)
                if (memcmp(memnama, secondload[i], 6) == 0) break;

            /* If not in second table then skip member */
            if (secondload[i] == NULL)
            {
                // "Member %s type %s skipped"
                WRMSG( HHC02450, "I", memnama,
                        ((dirent->pds2indc & PDS2INDC_ALIAS) ?
                        "alias" : "member" ));
                continue;
            }

        } /* end if(firstload[i]==NULL) */

        /* Check that member information array is not full */
        if (n >= MAX_MEMBERS)
        {
            // "Too many members"
            FWRMSG( stderr, HHC02451, "W" );
            return -1;
        }

        /* Check that user data contains at least 1 TTR */
        if (((dirent->pds2indc & PDS2INDC_NTTR) >> PDS2INDC_NTTR_SHIFT)
                < 1)
        {
            // "Member %s has TTR count zero"
            FWRMSG( stderr, HHC02452, "E", memnama );
            return -1;
        }

        /* Extract the total module length */
        totlen = (dirent->pds2usrd[10] << 16)
                | (dirent->pds2usrd[11] << 8)
                | dirent->pds2usrd[12];

        /* Extract the length of the first text block */
        txtlen = (dirent->pds2usrd[13] << 8)
                | dirent->pds2usrd[14];

        /* Save member information in the array */
        memcpy (memtab[n].memname, dirent->pds2name, 8);
        memcpy (memtab[n].ttrtext, dirent->pds2usrd + 0, 3);
        memtab[n].dwdsize = totlen / 8;

        /* Flag the member if it is an alias */
        memtab[n].alias = (dirent->pds2indc & PDS2INDC_ALIAS) ? 1 : 0;

        /* Flag member if 7th character of name is non-numeric */
        memtab[n].notable = (memnama[6] < '0' || memnama[6] > '9') ?
                                                                1 : 0;

        /* Check that the member has a single text record */
        memtab[n].multitxt = ((dirent->pds2usrd[8] & 0x01) == 0 || totlen != txtlen) ? 1 : 0;
        if (memtab[n].multitxt)
        {
            // "Member %s is not a single text record"
            FWRMSG( stderr, HHC02453, "W", memnama );
        }

        /* Check that the total module length does not exceed X'7F8' */
        if (totlen > 255*8)
        {
            // "Member %s size %04X exceeds limit 07F8"
            FWRMSG( stderr, HHC02454, "W", memnama, totlen );
        }

        /* Check that the total module length is a multiple of 8 */
        if (totlen & 0x7)
        {
            // "Member %s size %04X is not a multiple of 8"
            FWRMSG( stderr, HHC02455, "W", memnama, totlen );
        }

        /* Increment number of entries in table */
        *nmem = ++n;

    } /* end for */

    return 0;

} /* end function process_dirblk */

/*-------------------------------------------------------------------*/
/* Subroutine to resolve TTRs embedded within a member               */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      noext   Number of extents in dataset                         */
/*      extent  Dataset extent array                                 */
/*      memp    Array entry for member whose TTRs are to be resolved */
/*      memtab  Member information array                             */
/*      nmem    Number of entries in member information array        */
/*                                                                   */
/* Return value is 0 if OK, -1 if error                              */
/*-------------------------------------------------------------------*/
static int
resolve_xctltab (CIFBLK *cif, int noext, DSXTENT extent[],
                MEMINFO *memp, MEMINFO memtab[], int nmem)
{
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
U16             len;                    /* Record length             */
U32             cyl;                    /* Cylinder number           */
U8              head;                   /* Head number               */
U8              rec;                    /* Record number             */
u_int           trk;                    /* Relative track number     */
int             xctloff;                /* Offset to XCTL table      */
int             warn;                   /* 1=Flag TTRL difference    */
BYTE           *blkptr;                 /* -> Text record data       */
char            memnama[9];             /* Member name (ASCIIZ)      */
BYTE            svcnum[3];              /* SVC number (EBCDIC)       */
BYTE            prefix[3];              /* IGG/IFG prefix (EBCDIC)   */
BYTE            refname[8];             /* Referred name (EBCDIC)    */
char            refnama[9];             /* Referred name (ASCIIZ)    */

    /* Extract the member name */
    make_asciiz (memnama, sizeof(memnama), memp->memname, 8);
    if (memp->memname[7] == OVERPUNCH_ZERO)
        memnama[7] = '{';

    /* Skip the member if it is an alias */
    if (memp->alias)
    {
        // "Member %s type %s skipped"
        WRMSG( HHC02450, "I", memnama, "alias" );
        return 0;
    }

    /* Skip the member if it has no XCTL table */
    if (memp->notable)
    {
        // "Member %s type %s skipped"
        WRMSG( HHC02450, "I", memnama, "member" );
        return 0;
    }

    /* Error if member is not a single text record */
    if (memp->multitxt)
    {
        // "Member %s is not a single text record"
        FWRMSG( stderr, HHC02444, "E", memnama );
        return -1;
    }

    /* Convert relative track to cylinder and head */
    trk = (memp->ttrtext[0] << 8) | memp->ttrtext[1];
    rec = memp->ttrtext[2];
    rc = convert_tt (trk, noext, extent, cif->heads, &cyl, &head);
    if (rc < 0)
    {
        // "Member %s has invalid TTR %04X%02X"
        FWRMSG( stderr, HHC02456, "E", memnama, trk, rec );
        return -1;
    }

    // "Member %s text record TTR %04X%02X CCHHR %04X%04X%02X in progress"
    WRMSG( HHC02457, "I", memnama, trk, rec, cyl, head, rec );

    /* Read the text record */
    rc = read_block (cif, cyl, head, rec,
                    NULL, NULL, &blkptr, &len);
    if (rc != 0)
    {
        // "Member %s error reading TTR %04X%02X"
        FWRMSG( stderr, HHC02458, "E", memnama, trk, rec );
        return -1;
    }

    /* Check for incorrect length record */
    if (len < 8 || len > 1024 || (len & 0x7))
    {
        // "Member %s TTR %04X%02X text record length %X is invalid"
        FWRMSG( stderr, HHC02459, "E", memnama, trk, rec, len );
        return -1;
    }

    /* Check that text record length matches directory entry */
    if (len != memp->dwdsize * 8)
    {
        // "Member %s TTR %04X%02X text record length %X does not match %X in directory"
        FWRMSG( stderr, HHC02460, "E",
                memnama, trk, rec, len, memp->dwdsize * 8 );
        return -1;
    }

    /* Extract the SVC number and the XCTL table offset
       from the last 4 bytes of the text record */
    memcpy (svcnum, blkptr + len - 4, sizeof(svcnum));
    xctloff = blkptr[len-1] * 8;

    /* For the first load of SVC 19, 20, 23, and 55, and for
       IFG modules, the table is in two parts.  The parts are
       separated by a X'FFFF' delimiter.  The first part refers
       to IFG modules, the second part refers to IGG modules */
    if ((memcmp(memnama, "IGC", 3) == 0
         && (memcmp(svcnum, "\xF0\xF1\xF9", 3) == 0
            || memcmp(svcnum, "\xF0\xF2\xF0", 3) == 0
            || memcmp(svcnum, "\xF0\xF2\xF3", 3) == 0
            || memcmp(svcnum, "\xF0\xF5\xF5", 3) == 0))
        || memcmp(memnama, "IFG", 3) == 0)
        convert_to_ebcdic (prefix, sizeof(prefix), "IFG");
    else
        convert_to_ebcdic (prefix, sizeof(prefix), "IGG");

    /* Process each entry in the XCTL table */
    while (1)
    {
        /* Exit at end of XCTL table */
        if (blkptr[xctloff] == HEX00 && blkptr[xctloff+1] == HEX00)
            break;

        /* Switch prefix at end of first part of table */
        if (blkptr[xctloff] == HEXFF && blkptr[xctloff+1] == HEXFF)
        {
            xctloff += 2;
            convert_to_ebcdic (prefix, sizeof(prefix), "IGG");
            continue;
        }

        /* Error if XCTL table overflows text record */
        if (xctloff >= len - 10)
        {
            // "Member %s TTR %04X%02X XCTL table improperly terminated"
            FWRMSG( stderr, HHC02461, "E", memnama, trk, rec );
            return -1;
        }

        /* Skip this entry if the suffix is blank */
        if (blkptr[xctloff] == HEX40
            && blkptr[xctloff+1] == HEX40)
        {
            xctloff += 6;
            continue;
        }

        /* Build the name of the member referred to */
        memcpy (refname, prefix, 3);
        memcpy (refname + 3, svcnum, 3);
        memcpy (refname + 6, blkptr+xctloff, 2);
        make_asciiz (refnama, sizeof(refnama), refname, 8);

        /* Find the referred member in the member array */
        for (i = 0; i < nmem; i++)
        {
            if (memcmp(memtab[i].memname, refname, 8) == 0)
                break;
        } /* end for(i) */

        /* Loop if member not found */
        if (i == nmem)
        {
            char buf[80];
            MSGBUF( buf, " member '%s' not found", refnama);

            /* Display XCTL table entry */
            // "Member %s %s TTRL %02X%02X%02X%02X: %s"
            WRMSG( HHC02462, "I", memnama, refnama,
                blkptr[xctloff+2], blkptr[xctloff+3],
                blkptr[xctloff+4], blkptr[xctloff+5], buf );

            xctloff += 6;
            continue;
        }

        /* Loop if TTRL in the XCTL table matches actual TTRL */
        if (memcmp(blkptr+xctloff+2, memtab[i].ttrtext, 3) == 0
            && blkptr[xctloff+5] == memtab[i].dwdsize)
        {
            /* Display XCTL table entry */
            // "Member %s %s TTRL %02X%02X%02X%02X: %s"
            WRMSG( HHC02462, "I", memnama, refnama,
                blkptr[xctloff+2], blkptr[xctloff+3],
                blkptr[xctloff+4], blkptr[xctloff+5], "" );

            xctloff += 6;
            continue;
        }

        /* Flag entries whose L differs */
        if (blkptr[xctloff+5] != memtab[i].dwdsize)
            warn = 1;
        else
            warn = 0;

        /* Replace TTRL in the XCTL table by the actual TTRL */
        memcpy (blkptr+xctloff+2, memtab[i].ttrtext, 3);
        blkptr[xctloff+5] = memtab[i].dwdsize;

        {
            char buf[80];
            MSGBUF( buf, " replaced by TTRL=%02X%02X%02X%02X %s",
                blkptr[xctloff+2], blkptr[xctloff+3],
                blkptr[xctloff+4], blkptr[xctloff+5],
                (warn ? "****" : ""));
            /* Display XCTL table entry */
            // "Member %s %s TTRL %02X%02X%02X%02X: %s"
            WRMSG( HHC02462, "I", memnama, refnama,
                blkptr[xctloff+2], blkptr[xctloff+3],
                blkptr[xctloff+4], blkptr[xctloff+5], buf );
        }

        /* Flag the track as modified to force rewrite */
        cif->trkmodif = 1;

        /* Point to next entry in XCTL table */
        xctloff += 6;

    } /* end while */

    return 0;
} /* end function resolve_xctltab */


