/* DASDSEQ.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright James M. Morrison, 2001-2010           */
/*              Hercules Supported DASD definitions                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Code borrowed from dasdpdsu Copyright 1999-2009 Roger Bowler      */
/* Changes and additions Copyright 2001-2009, James M. Morrison      */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*                            dasdseq                                */
/*                                                                   */
/*  This program retrieves a sequential (DSORG=PS) dataset from      */
/*  a Hercules CKD/CCKD volume.  The input file is assumed to be     */
/*  encoded in the EBCDIC character set.                             */
/*                                                                   */
/*  We don't use some of the regular Hercules dasd routines because  */
/*  we want the DSCB as read from dasd so we can check some of the   */
/*  file attributes (such as DSORG, RECFM, LRECL).                   */
/*                                                                   */
/*  Dasdseq now uses the same case for the output file as the        */
/*  user specifies on the command line.  Prior versions always       */
/*  used upper case, which seems unnecessarily loud.                 */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"

#define UTILITY_NAME    "dasdseq"
#define UTILITY_DESC    "Sequential DSN unload"

//---------------------------------------------------------------------
//                          Structs
//---------------------------------------------------------------------

typedef struct _DASD_VOL_LABEL
{
        /* dasd cyl 0 trk 0 record 3                            */
        /* identifies volser, owner, and VTOC location          */
        /* recorded in EBCDIC; 80 bytes in length               */

        BYTE    vollabi[3];             // c'VOL'
        BYTE    volno;                  // volume label sequence #
        BYTE    volserno[6];            // volume serial
        BYTE    security;               // security field, set to 0xc0
        BYTE    volvtoc[5];             // CCHHR of VTOC's F4DSCB
        BYTE    resv1[21];              // reserved; should be left blank
        BYTE    volowner[14];           // volume owner
        BYTE    resv2[29];              // reserved; should be left blank
} DASD_VOL_LABEL;


#ifndef MAX_EXTENTS                     // see getF3dscb for notes
#define MAX_EXTENTS     123             // maximum supported dataset extents
#endif

typedef struct _DADSM {
        DASD_VOL_LABEL  volrec;                 // volume label record
        FORMAT4_DSCB    f4buf;                  // F4 DSCB
        DSXTENT         f4ext;                  // VTOC extent info
        FORMAT3_DSCB    f3buf;                  // F3 DSCB
        FORMAT1_DSCB    f1buf;                  // F1 DSCB
        int             f1numx;                 // # valid dataset extents
        DSXTENT         f1ext[MAX_EXTENTS];     // dsn extent info
} DADSM;

//---------------------------------------------------------------------
//                       Function prototypes
//---------------------------------------------------------------------

void showhelp   (   char            *pgm    );
int parsecmd    (   int              argc,
                    char           **argv,
                    DADSM           *dadsm,
                    char            *pgm    );
void sayext     (   int              max,
                    DSXTENT         *extent );
void showf1     (   FORMAT1_DSCB    *f1dscb,
                    DSXTENT          extent[],
                    int              verbose );
int fbcopy      (   FILE            *fout,
                    CIFBLK          *cif,
                    DADSM           *dadsm,
                    int              tran,
                    int              verbose );
void makext     (   int              i,
                    int              heads,
                    DSXTENT         *extent,
                    int              startcyl,
                    int              starttrk,
                    int              size   );
int getlabel    (   CIFBLK          *cif,
                    DASD_VOL_LABEL  *glbuf,
                    int              verbose );
int getF1dscb   (   CIFBLK          *cif,
                    char            *pdsn[],
                    FORMAT1_DSCB    *f1dscb,
                    DSXTENT         *vtocext[],
                    int              verbose );
int getF3dscb   (   CIFBLK          *cif,
                    BYTE            *f3cchhr,
                    FORMAT3_DSCB    *f3dscb,
                    int              verbose );
int getF4dscb   (   CIFBLK          *cif,
                    FORMAT4_DSCB    *f4dscb,
                    DASD_VOL_LABEL  *volrec,
                    DSXTENT         *vtocx,
                    int              verbose );
int dadsm_setup (   CIFBLK          *cif,
                    char            *pdsn[],
                    DADSM           *dadsm,
                    int              verbose );

//---------------------------------------------------------------------
//                           Globals
//---------------------------------------------------------------------

int     local_verbose = 0;      // verbose setting
int     copy_verbose = 0;       // verbose setting for copyfile
char    *din;                   // dasd image filename
char    *sfn;                   // shadow file parm
int     absvalid = 0;           // 1 = -abs specified, use CCHH not dsn
char    *argdsn;                // MVS dataset name
int     expert = 0;             // enable -abs help
bool    tran_ascii  = false;    // true = ascii output to terminal
bool    record_mode = false;    // true = prefix records w/ftp-like
                                // prefix: X'80'+XL2'lrecl'.
#ifdef DEBUG
int     debug = 1;              // enable debug code
#else
int     debug = 0;              // disable debug code
#endif

//---------------------------------------------------------------------
//                            showhelp
//---------------------------------------------------------------------

void showhelp(char *pgm)
{
    char part1[256];
    char part2[512];
    char part3[4096];
    char part4[1024];

    if (expert)
    {
        strlcpy( part1,
            "[-debug] [-expert] [-ascii] [-record] image [sf=shadow] [attr] filespec\n"
            "HHC02660I   -debug    optional - Enables debug mode, additional debug help appears.\n"
            , sizeof( part1 ));
    }
    else
    {
        strlcpy( part1,
            "[-expert] [-ascii] [-record] image [sf=shadow] filespec\n"
            , sizeof( part1 ));
    }

    strlcpy( part2,
        "HHC02660I   -expert   optional - Additional help describes expert operands.\n"
        "HHC02660I   -ascii    optional - translate input to ASCII, trimming trailing blanks,\n"
        "HHC02660I                        displaying results to stdout.\n"
        "HHC02660I   -record   optional - Prefix each o/p record with X'80', XL2'lrecl'.\n"
        "HHC02660I                        Note: mutually exclusive with -ascii option.\n"
        "HHC02660I   image     required - [path/]filename of dasd image file (dasd volume).\n"
        "HHC02660I   shadow    optional - [path/]filename of shadow file (note sf=).\n"
        , sizeof( part2 ));

    if (expert)
    {
        MSGBUF( part3,
            "HHC02660I   ALL EXPERT FACILITIES ARE EXPERIMENTAL!\n"
            "HHC02660I   attr      optional - dataset attributes (only useful with -abs).\n"
            "HHC02660I             attr syntax: [-recfm fb] [-lrecl aa]\n"
            "HHC02660I             -recfm designates RECFM, reserved for future support.\n"
            "HHC02660I             fb - fixed, blocked (only RECFM currently supported).\n"
            "HHC02660I             -lrecl designates dataset LRECL.\n"
            "HHC02660I             aa - decimal logical record length (default 80).\n"
            "HHC02660I                  Blocksize need not be specified; dasdseq handles whatever\n"
            "HHC02660I                  block size comes off the volume.\n"
            "HHC02660I   filespec  required (optional sub-operands in the following order):\n"
            "HHC02660I                  [-heads xx]\n"
            "HHC02660I                  [-abs cc hh tt] [...] [-abs cc hh tt ]\n"
            "HHC02660I                  filename\n"
            "HHC02660I             When -abs is -not- specified,\n"
            "HHC02660I                  Filename specifies the MVS DSORG=PS dataset on the volume.\n"
            "HHC02660I                  The dasd image volume containing the dataset must have a valid VTOC\n"
            "HHC02660I                  structure, and a F1 DSCB describing the dataset.\n"
            "HHC02660I                  Specifying -debug will (eventually) display extent information.\n"
            "HHC02660I             When -abs is specified, each -abs group specifies one dataset extent.\n"
            "HHC02660I                  For multi-extent datasets, -abs groups may be repeated as needed,\n"
            "HHC02660I                  in the order in which the dataset's extents occur.\n"
            "HHC02660I                  A maximum of %d extents are supported.\n"
            "HHC02660I                  No VTOC structure is implied, a F1 DSCB will not be sought.\n"
            "HHC02660I                  Dasdseq will frequently report 'track not found in extent table'\n"
            "HHC02660I                  (along with a message from fbcopy about rc -1 from convert_tt)\n"
            "HHC02660I                  due to potentially missing EOF markers in the extent, and the\n"
            "HHC02660I                  fact that the F1 DSCB DS1LSTAR field is not valid.\n"
            "HHC02660I                  Check your output file before you panic.\n"
            "HHC02660I                  fbcopy ignores EOF in case you are attempting to recovery PDS\n"
            "HHC02660I                  member(s) from a damaged dasd volume, preferring to wait until\n"
            "HHC02660I                  all tracks in the extent have been processed.\n"
            "HHC02660I                  Tracks containing PDS members may have more than one EOF per track.\n"
            "HHC02660I                  Expect a lot of associated manual effort with -abs.\n"
            "HHC02660I             -heads defines # tracks per cylinder on device.\n"
            "HHC02660I             xx - decimal number of heads per cylinder on device\n"
            "HHC02660I                  default xx = 15 (valid for 3380s, 3390s).\n"
            "HHC02660I             -abs indicates the beginning of each extent's location in terms of\n"
            "HHC02660I                  absolute dasd image location.\n"
            "HHC02660I             cc - decimal cylinder number (relative to zero).\n"
            "HHC02660I             hh - decimal head number (relative to zero).\n"
            "HHC02660I             tt - decimal number of tracks in extent.\n"
            "HHC02660I             filename will be the filename of the output file in the current directory.\n"
            "HHC02660I                  output filename will be in the same case as the command line filename.\n"
            , MAX_EXTENTS );
    }
    else
    {
        strlcpy( part3,
            "HHC02660I   filespec  required - MVS dataset name of DSORG=PS dataset, output filename.\n"
            , sizeof( part3 ));
    }

    if (debug)
    {
        strlcpy( part4,
            "HHC02660I Debugging options (at end of dasdseq command):\n"
            "HHC02660I   [verbose [x [y [z]]]]\n"
            "HHC02660I   verbose   debug output level (default = 0 when not specified).\n"
            "HHC02660I       x  main program (default = 1 when verbose specified).\n"
            "HHC02660I       y  copyfile + showf1.\n"
            "HHC02660I       z  dasdutil.\n"
            "HHC02660I       Higher numbers produces more output."
            , sizeof( part4 ));
    }
    else
        STRLCPY( part4, "" );

    // HHC02660 "Usage: %s %s%s%s%s"
    WRMSG( HHC02660, "I", pgm, part1, part2, part3, part4 );

}

//---------------------------------------------------------------------
//    parse command line and place results into program globals
//---------------------------------------------------------------------

int parsecmd(int argc, char **argv, DADSM *dadsm, char *pgm)
{
        int     util_verbose = 0;       // Hercules dasdutil.c diagnostic level
        int     heads = 15;             // # heads per cylinder on device
        int     extnum = 0;             // extent number for makext()
        int     abscyl = 0;             // absolute CC (default 0)
        int     abshead = 0;            // absolute HH (default 0)
        int     abstrk = 1;             // absolute tracks (default 1)
        int     lrecl = 80;             // default F1 DSCB lrecl
        char    msgbuf[128];            // message work area

    //  Usage: dasdseq [-debug] [-expert] [-ascii] [-record] image [sf=shadow] [attr] filespec

    argv++;  // skip dasdseq command argv[0]

    if ((*argv) && (strcasecmp(*argv, "-debug") == 0))
    {
        argv++;
        debug = 1;
        // "%s mode enabled"
        WRMSG( HHC02672, "D", "Debug" );
    }

    if ((*argv) && (strcasecmp(*argv, "-expert") == 0))
    {
        argv++;
        expert = 1;
        // "% mode enabled"
        if (debug) WRMSG( HHC02672, "I", "EXPERT" );
    }

    if ((*argv) && (strcasecmp(*argv, "-ascii") == 0))
    {
        argv++;
        tran_ascii = true;
        // "%s mode enabled"
        if (debug) WRMSG( HHC02672, "I", "ASCII translation" );
    }
    else if ((*argv) && (strcasecmp(*argv, "-record") == 0))
    {
        argv++;
        record_mode = true;
        // "%s mode enabled"
        if (debug) WRMSG( HHC02672, "I", "RECORD" );
    }

    if (*argv) din = *argv++;           // dasd image filename

    if (debug)
    {
        // "%s file%s '%s'"
        WRMSG( HHC02673, "D", "Image", "", din );
    }

    if (*argv && strlen(*argv) > 3 && !memcmp(*argv, "sf=", 3))
    {
        sfn = *argv++;  // shadow file parm
    }
    else
        sfn = NULL;

    if (debug)
    {
        // "%s file%s '%s'"
        WRMSG( HHC02673, "D", "SHADOW", "(s)", sfn );
    }

    dadsm->f1buf.ds1recfm =
                RECFM_FORMAT_F | RECFM_BLOCKED; // recfm FB for fbcopy

    if ((*argv) && (strcasecmp(*argv, "-recfm") == 0))
    {
        argv++;  // skip -recfm

        if ((*argv) && (strcasecmp(*argv, "fb") == 0))
        {
            argv++;  // skip fb

            // "%s=%s"
            if (debug) WRMSG( HHC02674, "D", "RECFM", "FB" );
        }
        else
        {
            // "Invalid argument: %s"
            argv++;  // skip bad recfm
            MSGBUF( msgbuf, "-recfm %s", *argv );
            WRMSG( HHC02465, "I", msgbuf);
        }
    }

    if ((*argv) && (strcasecmp(*argv, "-lrecl") == 0))
    {
        argv++;                            // skip -lrecl
        if (*argv) lrecl = atoi(*argv++);  // lrecl value

        if (debug)
        {
            // "%s=%d"
            WRMSG( HHC02675, "D", "LRECL", lrecl );
        }
    }

    dadsm->f1buf.ds1lrecl[0] = lrecl >> 8; // for fbcopy
    dadsm->f1buf.ds1lrecl[1] = lrecl - ((lrecl >> 8) << 8);

    if ((*argv) && (strcasecmp(*argv, "-heads") == 0))
    {
        argv++;                            // skip -heads
        if (*argv) heads = atoi(*argv++);  // heads value
    }

    if (debug)
    {
        // "%s=%d"
        WRMSG( HHC02675, "D", "HEADS", heads );
    }

    if ((*argv) && (strcasecmp(*argv, "-abs") == 0))
    {
        absvalid = 1;  // CCHH valid

        while ((*argv) && (strcasecmp(*argv, "-abs") == 0))
        {
            argv++;                               // skip -abs
            abscyl = 0; abshead = 0; abstrk = 1;  // defaults

            if (*argv) abscyl = atoi(*argv++);    // abs cc
            if (*argv) abshead = atoi(*argv++);   // abs hh
            if (*argv) abstrk = atoi(*argv++);    // abs tracks

            // Build extent entry for -abs group
            makext(extnum, heads,
                    (DSXTENT *) &dadsm->f1ext, abscyl, abshead, abstrk);
            extnum++;
            dadsm->f1buf.ds1noepv = extnum;  // for fbcopy

            if (debug)
            {
                // "Absolute CC %d HH %d [%04X%04X] Track %d/%X"
                WRMSG( HHC02676, "D", abscyl, abshead, abscyl, abshead, abstrk, abstrk);
            }

            if (extnum > MAX_EXTENTS)
            {
                // "Requested number of extents %d exceeds maximum %d; utility ends"
                FWRMSG( stderr, HHC02494, "S", extnum, MAX_EXTENTS);
                exit(3);
            }
        }
        // if (debug) sayext(MAX_EXTENTS, dadsm->f1ext);// show extent table
    }

    if (debug)
    {
        // "%s"
        WRMSG( HHC02677, "D", "Completed Format 1 DSCB:" );
        data_dump(&dadsm->f1buf, sizeof(FORMAT1_DSCB));
    }

    if (*argv) argdsn = *argv++;  // [MVS dataset name/]output filename

    if (debug)
    {
        // "Dataset '%s'"
        WRMSG( HHC02678, "D", argdsn );
    }

    // support deprecated 'ascii' operand
    if ((*argv) && (
                (strcasecmp(*argv, "ascii") == 0) ||
                (strcasecmp(*argv, "-ascii") == 0)
                )
        )
    {
        argv++;
        tran_ascii = true;
        // "%s mode enabled"
        if (debug) WRMSG( HHC02672, "D", "ASCII translation" );
    }

    set_verbose_util(0);                // default util verbosity

    if ((*argv) && (strcasecmp(*argv, "verbose") == 0))
    {
        local_verbose = 1;
        argv++;

        if (*argv) local_verbose = atoi(*argv++);
        if (*argv) copy_verbose = atoi(*argv++);

        if (*argv)
        {
            util_verbose = atoi(*argv++);
            set_verbose_util(util_verbose);
            if (debug)
            {
                // "Utility verbose %d"
                WRMSG( HHC02679, "D", util_verbose );
            }
        }
    }

    //  If the user specified expert mode without -abs, give help & exit
    //  Additionally, if the user has "extra" parms, show help & exit
    //  No "extraneous parms" message is issued, since some of the code
    //  above forces *argv to be true when it wants help displayed

    if ((argc < 3) || (*argv) || ((expert) && (!absvalid)))
    {
        showhelp(pgm);                     // show syntax before bailing
        exit(2);
    }

    return 0;
}

//---------------------------------------------------------------------
//                            sayext
//---------------------------------------------------------------------

void sayext(int max, DSXTENT *extent)
{
    int     i;
    // "     EXTENT --begin-- ---end---"
    WRMSG( HHC02481, "I");
    // "TYPE NUMBER CCCC HHHH CCCC HHHH"
    WRMSG( HHC02482, "I");
    for (i=0; i < max; i++)
    {
        U32 bcyl = (extent[i].xtbcyl[0] << 8) | extent[i].xtbcyl[1];
        U16 btrk = (extent[i].xtbtrk[0] << 8) | extent[i].xtbtrk[1];
        U32 ecyl = (extent[i].xtecyl[0] << 8) | extent[i].xtecyl[1];
        U16 etrk = (extent[i].xtetrk[0] << 8) | extent[i].xtetrk[1];
        // "  %02X   %02X   %04X %04X %04X %04X"
        WRMSG( HHC02483, "I",
            extent[i].xttype, extent[i].xtseqn, bcyl, btrk, ecyl, etrk );
    }
}

//---------------------------------------------------------------------
//              Display selected F1 DSCB information
//---------------------------------------------------------------------

void showf1(    FORMAT1_DSCB    *f1dscb,
                DSXTENT         extent[],
                int             verbose)
{
    int     i, dsorg, lrecl, blksize, volseq, x, y, num_extents;
    char    volser[sizeof(f1dscb->ds1dssn) + 1];
    char    dsn[sizeof(f1dscb->ds1dsnam) + 1];
    char    txtcredt[9];                            // creation date
    char    txtexpdt[9] = "(n/a)";                  // expiration date
    char    txtscr[20];
    char    txtsyscd[14];
    char    txtdsorg[5] = "";                       // dsorg text
    char    txtrecfm[5] = "";                       // recfm text

    if (verbose > 2)
    {
        // "%s"
        WRMSG( HHC02681, "I", "Format 1 DSCB:" );
        data_dump(f1dscb, sizeof(FORMAT1_DSCB));
    }

    make_asciiz(dsn, sizeof(dsn),
                f1dscb->ds1dsnam, sizeof(f1dscb->ds1dsnam));
    make_asciiz(volser, sizeof(volser),
                f1dscb->ds1dssn, sizeof(f1dscb->ds1dssn));
    volseq = (f1dscb->ds1volsq[0] << 8) | (f1dscb->ds1volsq[1]);
    x = f1dscb->ds1credt[0] + 1900;
    y = (f1dscb->ds1credt[1] << 8) | f1dscb->ds1credt[2];

    MSGBUF( txtcredt, "%4.4d", x );
    STRLCAT( txtcredt, "." );

    MSGBUF( txtscr, "%3.3d", y );
    STRLCAT( txtcredt, txtscr );

    if (f1dscb->ds1expdt[0] || f1dscb->ds1expdt[1] || f1dscb->ds1expdt[2])
    {
        x = f1dscb->ds1expdt[0] + 1900;
        y = (f1dscb->ds1expdt[1] << 8) | f1dscb->ds1expdt[2];

        MSGBUF( txtexpdt, "%4.4d", x );
        STRLCAT( txtexpdt, "." );

        MSGBUF( txtscr, ".%3.3d", y );
        STRLCAT( txtexpdt, txtscr );
    }

    num_extents = f1dscb->ds1noepv;
    //  Field ignored: ds1nobdb (# bytes used in last PDS dir blk)
    make_asciiz(txtsyscd, sizeof(txtsyscd),
                f1dscb->ds1syscd, sizeof(f1dscb->ds1syscd));

    dsorg = (f1dscb->ds1dsorg[0] << 8) | (f1dscb->ds1dsorg[1]);

    if (dsorg & (DSORG_IS * 256))               STRLCPY( txtdsorg, "IS" );
    if (dsorg & (DSORG_PS * 256))               STRLCPY( txtdsorg, "PS" );
    if (dsorg & (DSORG_DA * 256))               STRLCPY( txtdsorg, "DA" );
    if (dsorg & (DSORG_PO * 256))               STRLCPY( txtdsorg, "PO" );
    if (dsorg &  DSORG_AM)                      STRLCPY( txtdsorg, "VS" );
    if (txtdsorg[0] == '\0')                    STRLCPY( txtdsorg, "??" );

    if (dsorg & (DSORG_U * 256))                STRLCAT( txtdsorg, "U" );

    if (f1dscb->ds1recfm & RECFM_FORMAT_F)      STRLCPY( txtrecfm, "F" );
    if (f1dscb->ds1recfm & RECFM_FORMAT_V)      STRLCPY( txtrecfm, "V" );
    if ((f1dscb->ds1recfm & RECFM_FORMAT_U) == RECFM_FORMAT_U)
                                                STRLCPY( txtrecfm, "U" );

    if (f1dscb->ds1recfm & RECFM_BLOCKED)       STRLCAT( txtrecfm, "B" );
    if (f1dscb->ds1recfm & RECFM_SPANNED)       STRLCAT( txtrecfm, "S" );
    if (f1dscb->ds1recfm & RECFM_CTLCHAR_A)     STRLCAT( txtrecfm, "A" );
    if (f1dscb->ds1recfm & RECFM_CTLCHAR_M)     STRLCAT( txtrecfm, "M" );
    if (f1dscb->ds1recfm & RECFM_TRKOFLOW)      STRLCAT( txtrecfm, "T" );

    //  Field ignored: ds1optcd (option codes, same as in DCB)

    blksize = (f1dscb->ds1blkl[0]  << 8) | f1dscb->ds1blkl[1];
    lrecl   = (f1dscb->ds1lrecl[0] << 8) | f1dscb->ds1lrecl[1];

    //  Field ignored: ds1keyl (key length)
    //  Field ignored: ds1rkp (relative key position)
    //  Field ignored: ds1dsind (data set indicators)
    //  Field ignored: ds1scalo (secondary allocation)
    //  Field ignored: ds1lstar (pointer to last written block; ttr)
    //  Field ignored: ds1trbal (bytes remaining on last track used)
    //  Extent information was passed to us, so we ignore what's in F1DSCB

    // "Dataset %s on volume %s sequence %d"
    // "Creation Date: %s; Expiration Date: %s"
    // "Dsorg=%s recfm=%s lrecl=%d blksize=%d"
    // "System code %s"

    WRMSG( HHC02485, "I", dsn, volser, volseq );
    WRMSG( HHC02486, "I", txtcredt, txtexpdt );
    WRMSG( HHC02487, "I", txtdsorg, txtrecfm, lrecl, blksize );
    WRMSG( HHC02488, "I", txtsyscd );

    if (verbose > 1)
    {
        int bcyl, btrk, ecyl, etrk;

        // "Dataset has %d extent(s)"
        WRMSG( HHC02680, "I", num_extents );

        if (verbose > 2)
            data_dump((void *)extent, sizeof(DSXTENT) * MAX_EXTENTS);

        // "%s"
        WRMSG( HHC02681, "I", "Extent Information:" );
        WRMSG( HHC02681, "I", "     EXTENT --begin-- ---end---" );
        WRMSG( HHC02681, "I", "TYPE NUMBER CCCC HHHH CCCC HHHH" );

        for (i=0; i < num_extents; i++)
        {
            bcyl = (extent[i].xtbcyl[0] << 8) | extent[i].xtbcyl[1];
            btrk = (extent[i].xtbtrk[0] << 8) | extent[i].xtbtrk[1];
            ecyl = (extent[i].xtecyl[0] << 8) | extent[i].xtecyl[1];
            etrk = (extent[i].xtetrk[0] << 8) | extent[i].xtetrk[1];

            // "  %02X   %02X   %04X %04X %04X %04X"
            WRMSG( HHC02682, "I", extent[i].xttype, extent[i].xtseqn,
                bcyl, btrk, ecyl, etrk );
        }
    }
}

//---------------------------------------------------------------------
//                Calculate lstartrack value
//---------------------------------------------------------------------

u_int  calc_lstartrack( FORMAT1_DSCB* f1dscb, int verbose )
{
    u_int        numtracks;
    const char*  ds_format;

    // Extended:  TTTT   =   ds1trbal(2), high-order 2 bytes of ds1lstar
    // Large:      TTT   =   ds1ttthi(1), high-order 2 bytes of ds1lstar
    // Normal:      TT   =                high-order 2 bytes of ds1lstar

    if (f1dscb->ds1smsfg & DS1STRP)
    {
        ds_format = "Extended";
        numtracks = (f1dscb->ds1trbal[0] << 24) | (f1dscb->ds1trbal[1] << 16)
            | (f1dscb->ds1lstar[0] << 8) | (f1dscb->ds1lstar[1] << 0);
    }
    else if (f1dscb->ds1flag1 & DS1LARGE)
    {
        ds_format = "Large";
        numtracks = (f1dscb->ds1ttthi << 16)
            | (f1dscb->ds1lstar[0] << 8) | (f1dscb->ds1lstar[1] << 0);
    }
    else
    {
        ds_format = "Normal";
        numtracks = 0
            | (f1dscb->ds1lstar[0] << 8) | (f1dscb->ds1lstar[1] << 0);
    }

    if (verbose)
    {
        // "Data set format is %s"
        WRMSG( HHC02696, "I", ds_format );
    }

    return numtracks;
}

//---------------------------------------------------------------------
//  Copy DSORG=PS RECFM=F[B] dataset to output file
//
//  Input:
//      fout            FILE * ("wb" for EBCDIC)
//      cif             dasdutil control block
//      f1dscb          F1 DSCB
//      extent          dataset extent array
//      tran            0 = ebcdic output, 1 = ascii output
//                      ascii output will have trailing blanks removed
//      verbose         0 = no status messages
//                      1 = status messages
//                      > 1 debugging messages
//                      > 2 dump read record
//                      > 3 dump written record
//                      > 4 dump read record from input buffer
//  Output:
//      File written, messages displayed on stderr
//  Returns -1 on error, else returns # records written
//  Notes:
//      Caller is responsible for opening and closing fout.
//
//      The F1 DSCB's DS1LSTAR field is used to determine EOF (absent a prior
//      EOF being encountered), since some datasets don't have an EOF marker
//      present.  On my MVSRES, for instance, SYS1.BRODCAST has no EOF marker.
//
//      2003-01-14 jmm DS1LSTAR may be zero; if so, ignore DS1LSTAR and
//      hope for valid EOF.
//---------------------------------------------------------------------

int fbcopy(     FILE            *fout,
                CIFBLK          *cif,
                DADSM           *dadsm,
                int             tran,
                int             verbose)
{
    FORMAT1_DSCB    *f1dscb = &dadsm->f1buf;
    DSXTENT         extent[MAX_EXTENTS];
    int     rc;
    u_int   trk = 0, trkconv = 999;
    U8      rec = 1;
    U32     cyl = 0;
    U8      head = 0;
    int     rc_rb;
    U16     len, offset;
    int     rc_copy = 0;
    int     recs_written = 0, lrecl, num_extents;
    u_int   lstartrack = 0, lstarrec = 0;
    bool    lstarvalid = false;
    BYTE    *buffer;
    char    *pascii = NULL;
    char    zdsn[sizeof(f1dscb->ds1dsnam) + 1];     // ascii dsn
    char    msgbuf[128];                            // message work area
    BYTE    prefix[3] = {0x80,0x00,0x00};           // record prefix

    // Kludge to avoid rewriting this code (for now):
    memcpy(&extent, (void *)&(dadsm->f1ext), sizeof(extent));

    num_extents = f1dscb->ds1noepv;
    lrecl = (f1dscb->ds1lrecl[0] << 8) | (f1dscb->ds1lrecl[1]);

    if (absvalid)
    {
        STRLCPY( zdsn, argdsn );

        // "%s"
        if (debug) WRMSG( HHC02677, "D", "absvalid" );
    }
    else
    {
        make_asciiz(zdsn, sizeof(zdsn),
                f1dscb->ds1dsnam, sizeof(f1dscb->ds1dsnam));

        // Valid ds1lstar value?
        if (0
            || (f1dscb->ds1lstar[0] != 0)
            || (f1dscb->ds1lstar[1] != 0)
            || (f1dscb->ds1lstar[2] != 0)
        )
        {
            // Calculate last track
            lstartrack = calc_lstartrack( f1dscb, verbose );
            lstarrec = f1dscb->ds1lstar[2];
            lstarvalid = true; // DS1LSTAR valid
        }
    }

    if (debug)
    {
        // HHC02674 "%s=%s"
        // HHC02675 "%s=%d"

        WRMSG( HHC02674, "D", "zdsn",        zdsn        );
        WRMSG( HHC02675, "D", "num_extents", num_extents );
        WRMSG( HHC02675, "D", "lrecl",       lrecl       );

        // "%s"
        WRMSG( HHC02677, "D", "Format 1 DSCB:" );
        data_dump(f1dscb, sizeof(FORMAT1_DSCB));

        sayext( num_extents, (void*) &extent );
    }

    if (verbose)    // DS1LSTAR = last block written TTR
    {
        // HHC02683 "DS1LSTAR[%02X%02X%02X] lstartrack(%d) lstarrec(%d) lstarvalid(%d)"
        WRMSG( HHC02683, "I",f1dscb->ds1lstar[0], f1dscb->ds1lstar[1],
            f1dscb->ds1lstar[2], lstartrack, lstarrec, lstarvalid );
    }

    if (tran)   // need ASCII translation buffer?
    {
        pascii = malloc(lrecl + 1);

        if (pascii == NULL)
        {
            // "%s: unable to allocate ASCII buffer"
            FWRMSG( stderr, HHC02489, "E", "fbcopy()" );
            return -1;
        }
    }

    if (lstarvalid)
        EXTGUIMSG( "TRKS=%d\n", lstartrack );

    while (1)   // output records until something stops us
    {
        if (lstarvalid)
            EXTGUIMSG( "TRK=%d\n", trk );

        //  Honor DS1LSTAR when valid

        if ((lstarvalid) && (trk == lstartrack) && (rec > lstarrec))
        {
            if (verbose)
            {
                // "%s"
                WRMSG( HHC02681, "I", "DS1LSTAR indicates EOF" );
                // HHC02684 "DS1LSTAR[%02X%02X%02X] track(%02X) record(%02X)"
                WRMSG( HHC02684, "I", f1dscb->ds1lstar[0], f1dscb->ds1lstar[1],
                    f1dscb->ds1lstar[2], trk, rec );
            }

            rc_copy = recs_written;
            break;
        }

        //  Convert TT to CCHH for upcoming read_block call

        if (trkconv != trk)     // avoid converting for each block
        {
            trkconv = trk;      // current track converted

            rc = convert_tt(trk, num_extents, extent, cif->heads, &cyl, &head);

            if (rc < 0)
            {
                // "%s: convert_tt() track %5.5d/x'%04X', rc %d"
                FWRMSG( stderr, HHC02490, "E", "fbcopy()", trk, trk, rc );
                if (absvalid)
                    rc_copy = recs_written;
                else
                    rc_copy = -1;
                break;
            }

            if (verbose > 1)
            {
                // "Convert TT %5.5d/x'%04X' CCHH[%04X%04X]"
                WRMSG( HHC02685, "I", trk, trk, cyl, head );
            }
        }

        //  Read block from dasd

        if (verbose > 2)
        {
            // "Reading track %5.5d/x'%04X' record %d/x'%X' CCHHR[%04X%04X%02X]"
            WRMSG( HHC02686, "I", trk, trk, rec, rec, cyl, head, rec );
        }

        rc_rb = read_block(cif, cyl, head, rec, NULL, NULL, &buffer, &len);

        if (rc_rb < 0)          // error
        {
            // "In %s: function %s rc %d%s"
            MSGBUF( msgbuf, ", error reading '%s'", zdsn );
            FWRMSG( stderr, HHC02477, "E", "fbcopy()",
                            "read_block()", rc_rb, msgbuf );
            rc_copy = -1;
            break;
        }

        //  Handle end of track return from read_block

        if (rc_rb > 0)      // end of track
        {
            if (verbose > 2)
            {
                // "%s track %5.5d/x'%04X' rec %d"
                WRMSG( HHC02687, "I", "End of", trk, trk, rec );
            }
            trk++;          // next track
            rec = 1;        // record 1 on new track
            continue;
        }

        //  Check for dataset EOF

        if (len == 0)       // EOF
        {
            if (verbose)
            {
                // "%s track %5.5d/x'%04X' rec %d"
                WRMSG( HHC02687, "I", "EOF", trk, trk, rec );
            }

            if (absvalid)   // capture as much -abs data as possible
            {
                if (verbose)
                {
                    // "%s"
                    WRMSG( HHC02681, "I", "Ignoring -abs EOF" );
                }
            }
            else
            {
                rc_copy = recs_written;
                break;
            }
        }

        if (verbose > 3)
        {
            // "read %d bytes"
            WRMSG( HHC02688, "I", len );
        }

        if (verbose > 2)
        {
            // "%s"
            WRMSG( HHC02681, "I", "BEGIN OF BUFFER DUMP >" );
            data_dump( buffer, len );
            WRMSG( HHC02681, "I", "< END OF BUFFER DUMP" );
        }

        //  Deblock input dasd block, write records to output file

        for (offset=0; offset < len; offset += lrecl)
        {
            if (verbose > 3)
            {
                // "offset %d length %d rec %d"
                WRMSG( HHC02689, "I", offset, lrecl, recs_written );
            }

            if (tran)   // ASCII display
            {
                memset( pascii, 0, lrecl + 1 );
                make_asciiz( pascii, lrecl + 1, buffer + offset, lrecl );

                if (verbose > 4)
                {
                    // "buffer offset %d rec %d:"
                    WRMSG( HHC02690, "I", offset, rec );
                    data_dump( buffer + offset, lrecl );
                }

                // "ascii> '%s'"
                WRMSG( HHC02691, "I", pascii );

                if (verbose > 3)
                    data_dump( pascii, lrecl );
            }
            else    // EBCDIC output
            {
                if (verbose > 3)
                {
                    // "%s"
                    WRMSG( HHC02681, "I", "EBCDIC buffer:" );
                    data_dump( buffer + offset, lrecl );
                }

                if (record_mode)
                {
                    prefix[1] = (lrecl >> 8) & 0xFF;
                    prefix[2] = (lrecl >> 0) & 0xFF;

                    fwrite( prefix, 3, 1, fout );
                }

                if (!ferror( fout ))
                    fwrite( buffer + offset, lrecl, 1, fout );

                if (ferror( fout ))
                {
                    // "File %s; %s error: %s"
                    FWRMSG( stderr, HHC02468, "E", zdsn, "fwrite()", strerror( errno ));
                    rc_copy = -1;
                    break;
                }
            }

            recs_written++;  // (or displayed if -ascii)
        }
        /* end for (...)  */

        if (rc_copy != 0)
            break;
        else
            rec++;          // next record on track
    }
    /* end while (1) */

    if (pascii)
        free(pascii);       // release ASCII conversion buffer

    return rc_copy;
}

//---------------------------------------------------------------------
//    Place extent information into appropriate extent table slot
//---------------------------------------------------------------------

void makext(
        int i,                  // extent #
        int heads,              // # heads per cylinder on device
        DSXTENT *extent,        // extent table entry
        int startcyl,           // start cylinder
        int starttrk,           // start track
        int size)               // extent size in tracks
{
    int endcyl = ((startcyl * heads) + starttrk + size - 1) / heads;
    int endtrk = ((startcyl * heads) + starttrk + size - 1) % heads;

    if (i > (MAX_EXTENTS - 1))
    {
        // "%s: extent number parameter invalid %d; utility ends"
        FWRMSG( stderr, HHC02491, "S", "makext()", i );
        exit(4);
    }

    extent[i].xttype = 1;       // extent type
    extent[i].xtseqn = i;       // extent # (relative zero)

    // begin cyl

    extent[i].xtbcyl[0] = startcyl >> 8;
    extent[i].xtbcyl[1] = startcyl - ((startcyl / 256) * 256);
    extent[i].xtbtrk[0] = starttrk >> 8;
    extent[i].xtbtrk[1] = starttrk - ((starttrk / 256) * 256);

    // end cyl

    extent[i].xtecyl[0] = endcyl >> 8;
    extent[i].xtecyl[1] = endcyl - ((endcyl / 256) * 256);
    extent[i].xtetrk[0] = endtrk >> 8;
    extent[i].xtetrk[1] = endtrk - ((endtrk / 256) * 256);  // end track
    return;
}

//---------------------------------------------------------------------
//     getlabel - retrieve label record from dasd volume image
//---------------------------------------------------------------------
//
//  Input:
//      cif             ptr to opened CIFBLK of dasd image
//      glbuf           ptr to 80 byte buffer provided by caller
//      verbose         0 = no status messages
//                      1 = status messages
//                      > 1 debugging messages
//
//  Output:
//      glbuf           dasd volume label record
//
//  Returns:            0 OK, else error
//
//  Notes:
//      The volume label record always resides at CCHHR 0000 0000 03.
//      The dasd volume label record contains the CCHHR of the VTOC.
//      The volume label record is copied to the caller's buffer.
//---------------------------------------------------------------------

int getlabel(
                CIFBLK          *cif,
                DASD_VOL_LABEL  *glbuf,
                int             verbose)
{
    int     rc;
    U16     len;
    void    *plabel;

    if (verbose)
    {
        // "Reading volume label..."
        WRMSG( HHC02661, "I" );
    }

    rc = read_block(cif, 0, 0, 3, NULL, NULL, (void *) &plabel, &len);
    if (rc)
    {
        // "Error reading volume label, rc %d"
        FWRMSG( stderr, HHC02663, "E", rc );
        return 1;
    }
    if (len != sizeof(DASD_VOL_LABEL))
    {
        // "Error: volume label is %d bytes long, not 80 bytes long"
        FWRMSG( stderr, HHC02664, "E", len );
        return 2;
    }

    memcpy((void *)glbuf, plabel, sizeof(DASD_VOL_LABEL));

    if (verbose > 1)
    {
        // "%s"
        WRMSG( HHC02681, "I", "Volume label:" );
        data_dump(glbuf, len);
    }
    return 0;
}

//---------------------------------------------------------------------
//  getF4dscb - retrieve Format 4 DSCB - VTOC self-descriptor record
//---------------------------------------------------------------------
//
//  Input:
//      cif             ptr to opened CIFBLK of dasd image containing dataset
//      f4dscb          ptr to F4 DSCB buffer (key & data)
//      volrec          ptr to buffer containing volume label rec
//      vtocx           ptr to VTOC extent array (one extent only)
//      verbose         0 = no status messages
//                      1 = status messages
//                      > 1 debugging messages
//
//  Output:
//      f4buf           F4 DSCB in buffer (44 byte key, 96 bytes of data)
//      vtocx           VTOC extent array updated
//
//  Returns:            0 OK, else error
//
//  Notes:
//      There should only be one F4 DSCB in the VTOC, and it should always be
//      the first record in the VTOC.  The F4 provides VTOC extent information,
//      anchors free space DSCBs, and provides information about the device on
//      which the VTOC resides.
//---------------------------------------------------------------------

int getF4dscb(
                CIFBLK          *cif,
                FORMAT4_DSCB    *f4dscb,
                DASD_VOL_LABEL  *volrec,
                DSXTENT         *vtocx,
                int             verbose)
{
    char    vtockey[sizeof(f4dscb->ds4keyid)];
    void    *f4key, *f4data;
    U8      f4kl;
    U16     f4dl;
    int     rc;
    U32     cyl;
    U8      head, rec;

    //  Extract VTOC's CCHHR from volume label

    cyl = (volrec->volvtoc[0] << 8) | volrec->volvtoc[1];
    head = (volrec->volvtoc[2] << 8) | volrec->volvtoc[3];
    rec = volrec->volvtoc[4];

    if (verbose > 1)
    {
        // "VTOC F4 at cyl %d head %d rec %d"
        WRMSG( HHC02665, "I", cyl, head, rec);
    }

    //  Read VTOC's Format 4 DSCB (VTOC self-descriptor)

    if (verbose)
    {
        // "Reading VTOC F4 DSCB..."
        WRMSG( HHC02666, "I" );
    }

    rc = read_block( cif, cyl, head, rec, (void*) &f4key, &f4kl, (void*) &f4data, &f4dl );
    if (rc)
    {
        // "Error reading F4 DSCB, rc %d"
        FWRMSG( stderr, HHC02667, "E", rc );
        return 1;
    }

    //  Verify correct key and data length

    if ((f4kl != sizeof(f4dscb->ds4keyid)) ||
        (f4dl != (sizeof(FORMAT4_DSCB) - sizeof(f4dscb->ds4keyid))))
    {
        // "Erroneous key length %d or data length %d"
        FWRMSG( stderr, HHC02668, "E", f4kl, f4dl );
        return 2;
    }

    //  Return data to caller

    memcpy((void *) &f4dscb->ds4keyid, f4key, f4kl);    // copy F4 key into buffer
    memcpy((void *) &f4dscb->ds4fmtid, f4data, f4dl);   // copy F4 data into buffer
    memcpy((void *) vtocx, (void *)&f4dscb->ds4vtoce,
        sizeof(f4dscb->ds4vtoce));                      // copy VTOC extent entry
    if (verbose > 1)
    {
        // "%s"
        WRMSG( HHC02681, "I", "F4 DSCB:" );
        data_dump((void *) f4dscb, sizeof(FORMAT4_DSCB));
    }

    //  Verify DS4FMTID byte = x'F4', DS4KEYID key = x'04', and DS4NOEXT = x'01'
    //  Do this after copying data to caller's buffer so we can use struct fields
    //  rather than having to calculate offset to verified data; little harm done
    //  if it doesn't verify since we're toast if they're bad.

    memset(vtockey, 0x04, sizeof(vtockey));
    if ((f4dscb->ds4fmtid != 0xf4) ||
        (f4dscb->ds4noext != 0x01) ||
        (memcmp(&f4dscb->ds4keyid, vtockey, sizeof(vtockey))))
    {
        // "VTOC format id byte invalid (DS4IDFMT) %2.2X,\n"
        // "VTOC key invalid, or multi-extent VTOC"
        FWRMSG( stderr, HHC02670, "E", f4dscb->ds4fmtid );
        return 3;
    }

    //  Display VTOC extent info (always one extent, never more)

    if (verbose > 1)
    {
        // "VTOC start CCHH=%2.2X%2.2X %2.2X%2.2X end CCHH=%2.2X%2.2X %2.2X%2.2X"
        WRMSG( HHC02671, "I",
            vtocx->xtbcyl[0], vtocx->xtbcyl[1],
            vtocx->xtbtrk[0], vtocx->xtbtrk[1],
            vtocx->xtecyl[0], vtocx->xtecyl[1],
            vtocx->xtetrk[0], vtocx->xtetrk[1]);
    }
    return 0;
}

//---------------------------------------------------------------------
//  getF1dscb - retrieve Format 1 DSCB
//---------------------------------------------------------------------
//
//  Input:
//      cif             ptr to opened CIFBLK of dasd image containing dataset
//      zdsn            ASCII null-terminated dataset name
//      f1dscb          ptr to F1 DSCB buffer (key & data)
//      vtocext         ptr to VTOC's extent info
//      verbose         0 = no status messages
//                      1 = status messages
//                      > 1 debugging messages
//
//  Output:
//      f1buf           F1 DSCB (44 byte key, 96 byte data)
//
//  Returns: 0 OK, else error
//
//  Notes:  The F1 DSCB describes the MVS dataset's physical and logical attributes
//          such as RECFM, LRECL, BLKSIZE, and where on the volume the dataset
//          resides (the extent information).  The first 3 possible extents are
//          described in the F1 DSCB.  If additional extents are allocated, they
//          are described by F3 DSCBs referred to by the F1 DSCB.
//---------------------------------------------------------------------

int getF1dscb(
                CIFBLK          *cif,
                char            *pdsn[],
                FORMAT1_DSCB    *f1dscb,
                DSXTENT         *vtocext[],
                int             verbose)
{
    char    zdsn[sizeof(f1dscb->ds1dsnam) + 1];     // zASCII dsn
    BYTE    edsn[sizeof(f1dscb->ds1dsnam)];         // EBCDIC dsn
    void    *f1key, *f1data;
    U8      f1kl;
    U16     f1dl;
    U32     cyl;
    U8      head;
    U8      rec;
    int     rc;
    int     vtocextents = 1;        // VTOC has only one extent

    //  Locate dataset's F1 DSCB

    memset(zdsn, 0, sizeof(zdsn));
    STRLCPY( zdsn, *pdsn );
    string_to_upper( zdsn );
    convert_to_ebcdic( edsn, sizeof( edsn ), zdsn );
    if (verbose)
    {
        // "searching VTOC for '%s'"
        WRMSG( HHC02692, "I", zdsn );
    }
    rc = search_key_equal(cif, edsn, sizeof(edsn),
                        vtocextents, (DSXTENT *)vtocext,
                        &cyl, &head, &rec);
    if (rc)
    {
        if (verbose)
        {
            // "search_key_equal rc %d"
            WRMSG( HHC02693, "I", rc );
            // "%s"
            WRMSG( HHC02681, "I", "KEY:" );
            data_dump(edsn, sizeof(edsn));
        }
        if (rc == 1)
        {
            // "Dataset %s not found"
            FWRMSG( stderr, HHC02476, "E", zdsn);
        }
        return 1;
    }

    //  Read F1 DSCB describing dataset

    if (verbose)
    {
        // "%s"
        WRMSG( HHC02681, "I", "reading Format 1 DSCB" );
    }
    rc = read_block(cif, cyl, head, rec,
                (void *)&f1key, &f1kl,
                (void *) &f1data, &f1dl);
    if (rc)
    {
        // "In %s: function %s rc %d%s"
        FWRMSG( stderr, HHC02477, "E", "getF1dscb()", "read_block()", rc,
                            ", attempting to read Format 1 DSCB");
        return 2;
    }

    //  Return data to caller

    if ((f1kl == sizeof(f1dscb->ds1dsnam)) &&
        (f1dl == (sizeof(FORMAT1_DSCB) - sizeof(f1dscb->ds1dsnam))))
    {
        memcpy((void *) &f1dscb->ds1dsnam,
                f1key, f1kl);           // copy F1 key to buffer
        memcpy((void *) &f1dscb->ds1fmtid,
                f1data, f1dl);          // copy F1 data to buffer
    }
    else
    {
        // "Length invalid for KEY %d or DATA %d%s"
        FWRMSG( stderr, HHC02478, "E", f1kl, f1dl, " received for Format 1 DSCB");
        return 3;
    }

    if (verbose > 1)
    {
        // "%s"
        WRMSG( HHC02681, "I", "Format 1 DSCB:" );
        data_dump((void *) f1dscb, sizeof(FORMAT1_DSCB));
    }

    //  Verify DS1FMTID byte = x'F1'
    //  Do this after copying data to caller's buffer so we can use struct fields
    //  rather than having to calculate offset to verified data; little harm done
    //  if it doesn't verify since we're toast if it's bad.

    if (f1dscb->ds1fmtid != 0xf1)
    {
        // "In %s: x'F%s' expected for DS%sIDFMT, received x'%02X'"
        FWRMSG( stderr, HHC02479, "E", "getF1dscb()", "1", "1", f1dscb->ds1fmtid);
        return 4;
    }
    return 0;
}

//---------------------------------------------------------------------
//  getF3dscb - Retrieve Format 3 DSCB
//---------------------------------------------------------------------
//
//  Input:
//      cif             ptr to opened CIFBLK of dasd image containing dataset
//      f3cchhr         CCHHR of F3 DSCB to be read (key & data)
//      f3dscb          ptr to F3 DSCB buffer
//      verbose         0 = no status messages
//                      1 = status messages
//                      > 1 debugging messages
//
//  Output:
//      f3buf           F3 DSCB (44 byte key, 96 byte data)
//
//  Returns:            0 OK, else error
//
//  Notes:      The F3 DSCB describes additional dataset extents beyond those
//              described by the F1 DSCB.  Each F3 DSCB describes 13 extents.
//              Physical sequential datasets are limited to 16 extents on each
//              volume, extended format datasets are limited to 123 extents on
//              each volume.  Dasdseq doesn't provide explicit support for
//              multi-volume datasets.
//
//              Note there is extent information embedded in the key.
//
//              If you want support for > 16 extents, you will have to recompile
//              dasdseq after changing MAX_EXTENTS.
//
//  Warning:    I haven't tested the "chase the F3 chain" code, as I have no
//              reasonable way to do so.  The highest level of MVS I can run under
//              Hercules is MVS38j.
//---------------------------------------------------------------------

int getF3dscb(
                CIFBLK          *cif,
                BYTE            *f3cchhr,
                FORMAT3_DSCB    *f3dscb,
                int             verbose)
{
    int     rc;
    U32     cyl;
    U8      head, rec;
    void    *f3key, *f3data;
    U8      f3kl;
    U16     f3dl;

    cyl = (f3cchhr[0] << 8) | f3cchhr[1];
    head = (f3cchhr[2] << 8) | f3cchhr[3];
    rec = f3cchhr[4];

    if (verbose)
    {
        // "reading Format 3 DSCB - CCHHR %04X%04X%02X"
        WRMSG( HHC02662, "I", cyl, head, rec );
    }

    rc = read_block (cif, cyl, head, rec,
                (void *)&f3key, &f3kl,
                (void *)&f3data, &f3dl);
    if (rc)
    {
        // "In %s: function %s rc %d%s"
        FWRMSG( stderr, HHC02477, "E", "getF3dscb()", "read_block()", rc,
                            ", attempting to read Format 3 DSCB");
        return 1;
    }
    if ((f3kl != 44) || (f3dl != 96))
    {
        // "Length invalid for KEY %d or DATA %d%s"
        FWRMSG( stderr, HHC02478, "E", f3kl, f3dl, " received for Format 3 DSCB");
        return 2;
    }
    memcpy((void *) &f3dscb->ds3keyid,
                f3key, f3kl);           // copy F3 key to buffer
    memcpy((void *) ((BYTE*)f3dscb + f3kl),
                f3data, f3dl);          // copy F3 data to buffer

    if (verbose > 1)
    {
        // "%s"
        WRMSG( HHC02681, "I", "Format 3 DSCB:" );
        data_dump((void *) f3dscb, sizeof(FORMAT3_DSCB));
    }

    //  Verify DS3FMTID byte = x'F3'
    //  Do this after copying data to caller's buffer so we can use struct fields
    //  rather than having to calculate offset to verified data; little harm done
    //  if it doesn't verify since we're toast if it's bad.

    if (f3dscb->ds3fmtid != 0xf3)
    {
        // "In %s: x'F%s' expected for DS%sIDFMT, received x'%02X'"
        FWRMSG( stderr, HHC02479, "E", "getF3dscb()", "3", "3", f3dscb->ds3fmtid);
        return 2;
    }
    return 0;
}

//---------------------------------------------------------------------
//  dadsm_setup - retrieve volume label & DSCBs sufficient to describe dataset
//---------------------------------------------------------------------
//
//      This routine reads the volume label rec, the VTOC F4 DSCB, the F1 DSCB
//      for the dataset, and any F3 DSCB(s) associated with the dataset.
//      Constructs extent array describing space allocated to the dataset.
//
//  Input:
//      cif             ptr to opened CIFBLK of dasd image containing dataset
//      pdsn            ptr to ASCII null-terminated dataset name
//      dadsm           ptr to DADSM workarea
//      verbose         0 = no status messages
//                      1 = status messages
//                      > 1 debugging messages
//
//  Output:
//      dadsm           DADSM workarea
//
//  Returns: 0 OK, else error
//
//  Notes:
//---------------------------------------------------------------------

int dadsm_setup(
                CIFBLK          *cif,
                char            *pdsn[],
                DADSM           *dadsm,
                int             verbose)
{
    DSXTENT         *f1x;
    BYTE            *pcchhr;
    int             numx = MAX_EXTENTS;     // # extent slots available
    int             rc;

    //  Read dasd volume label record

    rc = getlabel(cif, &dadsm->volrec, verbose);
    if (rc) return rc;

    //  Read F4 DSCB, save VTOC extent info

    rc = getF4dscb(cif, &dadsm->f4buf, &dadsm->volrec, &dadsm->f4ext, verbose);
    if (rc) return rc;

    //  Read F1 DSCB, save first three extents from F1 DSCB

    rc = getF1dscb(cif, pdsn, &dadsm->f1buf, (void *)&dadsm->f4ext, verbose);
    if (rc) return rc;

    f1x = &dadsm->f1ext[0];                     // @ extent # 0
    numx -= 3;                                  // will use 3 slots (if available)
    if (numx < 0)
    {
        // "In %s: extent slots exhausted; maximum supported is %d"
        FWRMSG( stderr, HHC02480, "E", "dadsm_setup()", MAX_EXTENTS );
        return 1;
    }
    memcpy(f1x, &dadsm->f1buf.ds1ext1, sizeof(DSXTENT) * 3);
    f1x += 3;                                   // @ extent # 3
    dadsm->f1numx = dadsm->f1buf.ds1noepv;      // # extents alloc'd to dataset
    if (dadsm->f1numx < 4)
    {
        if (verbose > 1)
        {
            // "%d extent(s) found; all are in Format 1 DSCB"
            WRMSG( HHC02669, "I", dadsm->f1numx );
        }
        return 0;
    }

    //  When more than 3 extents, get additional extent info from F3 DSCB(s).
    //  Chase the F3 chain starting with the CCHHR in the F1, accumulating
    //  extent information for the dataset as we progress.

    pcchhr = (BYTE *)&dadsm->f1buf.ds1ptrds;            // @ F1 ptr to F3
    while (pcchhr[0] || pcchhr[1] || pcchhr[2] || pcchhr[3] || pcchhr[4])
    {
        rc = getF3dscb(cif, pcchhr, &dadsm->f3buf, verbose);
        if (rc) return rc;
        numx -= 4;                              // use extent slots
        if (numx < 0)
        {
            // "In %s: extent slots exhausted; maximum supported is %d"
            FWRMSG( stderr, HHC02480, "E", "dadsm_setup()", MAX_EXTENTS );
            return 2;
        }
        memcpy(f1x, &dadsm->f3buf.ds3extnt[0], sizeof(DSXTENT) * 4);
        f1x += 4;
        numx -= 9;                              // use extent slots
        if (numx < 0)
        {
            // "In %s: extent slots exhausted; maximum supported is %d"
            FWRMSG( stderr, HHC02480, "E", "dadsm_setup()", MAX_EXTENTS );
            return 3;
        }
        memcpy(f1x, &dadsm->f3buf.ds3adext[0], sizeof(DSXTENT) * 9);
        f1x += 9;
        pcchhr = (BYTE *)&dadsm->f3buf.ds3ptrds;        // @ next F3 CCHHR
    }
    return 0;
}

//---------------------------------------------------------------------
//                            Main
//---------------------------------------------------------------------

int main(int argc, char **argv)
{
    char           *pgm;                    /* less any extension (.ext) */
    DADSM           dadsm;                  // DADSM workarea
    FILE           *fout = NULL;            // output file
    CIFBLK         *cif;
    int             dsn_recs_written = 0;
    int             bail;
    int             dsorg;
    int             rc;
    char            pathname[MAX_PATH];

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    if (debug) WRMSG( HHC02677, "D", "DEBUG enabled" );

    //  Parse command line

    memset(&dadsm, 0, sizeof(dadsm));           // init DADSM workarea
    rc = parsecmd(argc, argv, &dadsm, pgm);
    if (rc) exit(rc);

    //  Open CKD image

    cif = open_ckd_image(din, sfn, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL);
    if (!cif || cif == NULL )
        return -1;

    //  Unless -abs specified (in which case trust the expert user):
    //  Retrieve extent information for the dataset
    //  Display dataset attributes
    //  Verify dataset has acceptable attributes

    if (!absvalid)
    {
        rc = dadsm_setup(cif, &argdsn, &dadsm, local_verbose);
        if (rc)
        {
            close_ckd_image(cif);
            exit(rc);
        }
        if (local_verbose)
        {
            FWRMSG( stderr, HHC02484, "I", "" );
            showf1(&dadsm.f1buf, dadsm.f1ext, copy_verbose);
            FWRMSG( stderr, HHC02484, "I", "" );
        }
        bail = 1;
        dsorg = (dadsm.f1buf.ds1dsorg[0] << 8) | (dadsm.f1buf.ds1dsorg[1]);
        if (dsorg & (DSORG_PS * 256))
        {
            if ((dadsm.f1buf.ds1recfm & RECFM_FORMAT) == RECFM_FORMAT_F)
                bail = 0;
            if ((dadsm.f1buf.ds1recfm & RECFM_FORMAT) == RECFM_FORMAT_V)
            {
                bail = 1;               // not yet
                // "Dataset is not RECFM %s; utility ends"
                FWRMSG( stderr, HHC02472, "S", "F[B]" );
            }
        }
        else
        {
            // "Dataset is not DSORG %s; utility ends"
            FWRMSG( stderr, HHC02473, "S", "PS" );
        }
        if (bail)
        {
            close_ckd_image(cif);
            exit(21);
        }
    }

    //  Open output file (EBCDIC requires binary open)

    if (!tran_ascii) // EBCDIC
    {
        hostpath(pathname, argdsn, sizeof(pathname));

        fout = fopen(pathname, "wb");

        if (fout == NULL)
        {
            // "File %s; %s error: %s"
            FWRMSG( stderr, HHC02468, "E", argdsn, "fopen()", strerror( errno ));
            close_ckd_image(cif);
            exit(22);
        }
        if (local_verbose)
        {
            // "writing %s"
            WRMSG( HHC02694, "I", argdsn );
        }
    }

    //  Write dasd data to output file (or just display it if -ascii)

    dsn_recs_written = fbcopy(fout, cif, &dadsm, tran_ascii, copy_verbose);

    if (dsn_recs_written == -1)
    {
        // "Error processing %s"
        FWRMSG( stderr, HHC02474, "E", argdsn );
    }
    else
    {
        // Records %s %s: %d"
        WRMSG( HHC02475, "I",
            tran_ascii ? "displayed on" : "written to",
            tran_ascii ? "terminal" : argdsn,
            dsn_recs_written );
    }

    //  Close output file, input dasd image and return to caller

    if (!tran_ascii) // EBCDIC
    {
        fclose(fout);

        if (local_verbose > 2)
        {
            // "Closed output file %s"
            WRMSG( HHC02695, "I", argdsn );
        }
    }

    if (local_verbose > 3)
    {
        // "%s"
        WRMSG( HHC02681, "I", "CIFBLK:" );
        data_dump((void *) cif, sizeof(CIFBLK));
    }

    close_ckd_image(cif);

    if (local_verbose > 2 )
    {
        // "%s"
        WRMSG( HHC02681, "I", "Closed image file" );
    }

    return rc;
}
