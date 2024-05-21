/* DASDINIT.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules DASD Utilities: DASD image builder          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program creates a disk image file and initializes it as      */
/* a blank FBA or CKD DASD volume.                                   */
/*                                                                   */
/* The program is invoked from the shell prompt using the command:   */
/*                                                                   */
/*      dasdinit [-options] filename devtype[-model] [volser] [size] */
/*                                                                   */
/* options    options:                                               */
/*              -a      include alternate cylinders                  */
/*                      (ignored if size specified manually)         */
/*              -z      build compressed device using zlib           */
/*              -bz2    build compressed device using bzip2          */
/*              -0      build compressed device with no compression  */
/*              -r      "raw" init (bypass VOL1 & IPL track fmt)     */
/*              -b      build disabled wait PSW as BC-mode PSW (if   */
/*                      not specified, default is EC-mode PSW)       */
/*              -m      build disabled wait PSW enabled for          */
/*                      machine checks (default is disabled for      */
/*                      machine checks)                              */
/*              -linux  format dasd using linux null track format    */
/*                                                                   */
/*              Note:  To get a disabled wait PSW that matches       */
/*                     what this program originally created,         */
/*                     you must specify the "-b" and "-m"            */
/*                     options.  The revised defaults, if            */
/*                     those switches are not specified, is an       */
/*                     EC-mode PSW disabled for machine checks.      */
/*                                                                   */
/* filename     is the name of the disk image file to be created     */
/*              (this program will not overwrite an existing file)   */
/*                                                                   */
/* devtype      is the emulated device type.                         */
/*              CKD: 2305, 2311, 2314, 3330, 3350, 3375, 3380,       */
/*                   3390, 9345                                      */
/*              FBA: 0671, 3310, 3370, 9313, 9332, 9335, 9336        */
/*                                                                   */
/* model        is the device model number and implies the device    */
/*              size. If specified, then size shouldn't be specified.*/
/*                                                                   */
/* volser       is the volume serial number (1-6 characters)         */
/*              (only if '-r' option not used)                       */
/*                                                                   */
/* size         is the size of the device (in cylinders for CKD      */
/*              devices, or in 512-byte sectors for FBA devices).    */
/*              Shouldn't be specified if model is specified.        */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "ccwarn.h"

#define UTILITY_NAME    "dasdinit"
#define UTILITY_DESC    "DASD image file creation program"

static void argexit( int code, char* m, const char* pgm );

/*-------------------------------------------------------------------*/
/* DASDINIT program main entry point                                 */
/*-------------------------------------------------------------------*/
int main ( int argc, char *argv[] )
{
char   *pgm;                            /* less any extension (.ext) */
int     altcylflag = 0;                 /* Alternate cylinders flag  */
int     rawflag = 0;                    /* Raw format flag           */
int     volsize_argnum = 4;             /* argc value of size option */
int     flagECmode = 1;                 /* IPL PSW mode flag         */
int     flagMachinecheck   = 0;         /* IPL PSW machine check flag*/
U32     size = 0;                       /* Volume size               */
U32     altsize = 0;                    /* Alternate cylinders       */
U32     heads = 0;                      /* Number of tracks/cylinder */
U32     maxdlen = 0;                    /* Maximum R1 data length    */
U32     sectsize = 0;                   /* Sector size               */
U16     devtype = 0;                    /* Device type               */
BYTE    comp = 0xff;                    /* Compression algoritm      */
BYTE    type = 0;                       /* C=CKD, F=FBA              */
char    fname[1024];                    /* File name                 */
char    volser[6+1];                    /* Volume serial number      */
BYTE    c;                              /* Character work area       */
CKDDEV *ckd;                            /* -> CKD device table entry */
FBADEV *fba;                            /* -> FBA device table entry */
int     lfs = 0;                        /* 1 = Build large file      */
int     nullfmt = CKD_NULLTRK_FMT1;     /* Null track format type    */
int     rc;                             /* Return code               */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Display help if needed or requested */
    if (argc <= 1 || (argc == 2 && !strcmp(argv[1], "-v")))
        argexit( -1, NULL, pgm );

    /* Process optional arguments */
    for ( ; argc > 1 && argv[1][0] == '-'; argv++, argc--)
    {
        if (strcmp("0", &argv[1][1]) == 0)
            comp = CCKD_COMPRESS_NONE;
#if defined( HAVE_ZLIB )
        else if (strcmp("z", &argv[1][1]) == 0)
            comp = CCKD_COMPRESS_ZLIB;
#endif
#if defined( CCKD_BZIP2 )
        else if (strcmp("bz2", &argv[1][1]) == 0)
            comp = CCKD_COMPRESS_BZIP2;
#endif
        else if (strcmp("a", &argv[1][1]) == 0)
            altcylflag = 1;
        else if (strcmp("r", &argv[1][1]) == 0)
            rawflag = 1;
        else if (strcmp("lfs", &argv[1][1]) == 0 && sizeof(off_t) > 4)
            lfs = 1;
        else if (strcmp("linux", &argv[1][1]) == 0)
            nullfmt = CKD_NULLTRK_FMT2;
        else if (strcmp("b", &argv[1][1]) == 0)
            flagECmode = 0;
        else if (strcmp("m", &argv[1][1]) == 0)
            flagMachinecheck = 1;
        else argexit( 0, argv[1], pgm );
    }

    /* Check remaining number of arguments */
    if (argc < (rawflag ? 3 : 4) || argc > (rawflag ? 4 : 5))
        argexit( 5, NULL, pgm );

    /* The first argument is the file name */
    if (argv[1] == NULL || strlen(argv[1]) == 0
        || strlen(argv[1]) > sizeof(fname)-1)
        argexit( 1, argv[1], pgm );

    STRLCPY( fname, argv[1] );

    /* The second argument is the device type.
       Model number may also be specified */
    if (argv[2] == NULL)
        argexit( 2, argv[2], pgm );
    ckd = dasd_lookup (DASD_CKDDEV, argv[2], 0, 0);
    if (ckd != NULL)
    {
        type = 'C';
        devtype = ckd->devt;
        size = ckd->cyls;
        altsize = ckd->altcyls;
        heads = ckd->heads;
        maxdlen = ckd->r1;
    }
    else
    {
        fba = dasd_lookup (DASD_FBADEV, argv[2], 0, 0);
        if (fba != NULL)
        {
            type = 'F';
            devtype = fba->devt;
            size = fba->blks;
            altsize = 0;
            sectsize = fba->size;
        }
    }

    if (!type)
        /* Specified model not found */
        argexit( 2, argv[2], pgm );

    /* If -r option specified, then there is not volume serial
       argument and volume size argument is actually argument
       number 3 and not argument number 4 as otherwise */
    if (rawflag)
        volsize_argnum = 3;
    else
    {
        volsize_argnum = 4;

        /* The third argument is the volume serial number */
        if (argv[3] == NULL || strlen(argv[3]) == 0
            || strlen(argv[3]) > sizeof(volser)-1)
            argexit( 3, argv[3], pgm );

        STRLCPY( volser, argv[3] );
        string_to_upper (volser);
    }

    /* The fourth argument (or third for -r) is the volume size */
    if (argc > volsize_argnum)
    {
        S32 signed_size = 0;    /* SIGNED Volume size */

        if (argc > (volsize_argnum+1))
            argexit( 5, NULL, pgm );

        if (0
            || !argv      [volsize_argnum]
            || strlen(argv[volsize_argnum]) == 0
            || sscanf(argv[volsize_argnum], "%d%c", &signed_size, &c) != 1
            || (type == 'C' && (signed_size < 1 || signed_size > 65536))
            || (type == 'F' && signed_size < 64)
        )
            argexit( 4, argv[volsize_argnum], pgm );

        size = (U32) signed_size;
        altcylflag = 0;
    }

    /* `-linux' only supported for 3390 device type */
    if (nullfmt == CKD_NULLTRK_FMT2 && devtype != 0x3390)
        argexit( 6, NULL, pgm );

    if (altcylflag)
        size += altsize;

    /* Warn if filename extension doesn't jive with compress option */
    {
        const char* dot = strrchr( fname, '.' );

        if (dot && dot != fname)
        {
            char ext[8];
            bool bWarn = false;

            STRLCPY( ext, dot+1 );

            if (comp != 0xff) // Compression?
            {
                if (type == 'C') // CCKD?
                {
                    if (1
                        && str_caseless_ne_n( "cckd",   ext, 4 )
                        && str_caseless_ne_n( "cckd32", ext, 6 )
                    )
                        bWarn = true;
                }
                else // Compressed FBA
                {
                    if (1
                        && str_caseless_ne_n( "cfba",   ext, 4 )
                        && str_caseless_ne_n( "cfba32", ext, 6 )
                    )
                        bWarn = true;
                }
            }
            else // Normal uncompressed
            {
                if (type == 'C') // CKD?
                {
                    if (1
                        && str_caseless_ne_n( "ckd",   ext, 3 )
                        && str_caseless_ne_n( "ckd32", ext, 5 )
                    )
                        bWarn = true;
                }
                else // FBA
                {
                    if (1
                        && str_caseless_ne_n( "fba",   ext, 3 )
                        && str_caseless_ne_n( "fba32", ext, 5 )
                    )
                        bWarn = true;
                }
            }

            if (bWarn)
            {
                // "filename extension does not reflect compression option"
                FWRMSG( stderr, HHC02443, "W" );
            }
        }
    }

    /* Create the device */
    if (type == 'C')
        rc = create_ckd (fname, devtype, heads, maxdlen, size, volser,
                        comp, lfs, 0, nullfmt, rawflag, flagECmode, flagMachinecheck);
    else
        rc = create_fba (fname, devtype, sectsize, size, volser, comp,
                        lfs, 0, rawflag);

    /* Display completion message */
    if (rc == 0)
    {
        // "DASD operation completed"
        WRMSG( HHC02423, "I" );
    } else {
        // "DASD operation failed"
        FWRMSG( stderr, HHC02449, "E" );
    }

    return rc;

} /* end function main */

/*-------------------------------------------------------------------*/
/* Subroutine to display command syntax and exit                     */
/*-------------------------------------------------------------------*/
static void argexit( int code, char* m, const char* pgm )
{
    // HHC02445: "Invalid, unsupported or missing %s: %s"

    switch (code) {
    case 0:
        FWRMSG( stderr, HHC02445, "E", "option",      m ? m : "(null)" );
        break;
    case 1:
        FWRMSG( stderr, HHC02445, "E", "filename",    m ? m : "(null)" );
        break;
    case 2:
        FWRMSG( stderr, HHC02445, "E", "device type", m ? m : "(null)" );
        break;
    case 3:
        FWRMSG( stderr, HHC02445, "E", "volser",      m ? m : "(null)" );
        break;
    case 4:
        FWRMSG( stderr, HHC02445, "E", "size",        m ? m : "(null)" );
        break;
    case 5:
        FWRMSG( stderr, HHC02446, "E" );
        break;
    case 6:
        FWRMSG( stderr, HHC02447, "E" );
        break;
    default:
        {
            // HHC02448 "Usage: dasdinit ...."

#if defined( HAVE_ZLIB )
        char *bufz = "HHC02448I   -z        build compressed dasd image file using zlib\n";
#else
        char *bufz = "";
#endif

#if defined( CCKD_BZIP2 )
        char *bufbz = "HHC02448I   -bz2      build compressed dasd image file using bzip2\n";
#else
        char *bufbz = "";
#endif

        char* buflfs = "";

            if (sizeof(off_t) > 4)
                buflfs = "HHC02448I   -lfs      build a large (uncompressed) dasd file (if supported)\n";

            WRMSG( HHC02448, "I", pgm, bufz, bufbz, buflfs );
        }
        break;
    }

    exit(code);

} /* end function argexit */
