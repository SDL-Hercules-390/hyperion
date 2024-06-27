/* HETMAP.C     (C) Copyright Leland Lucius, 2000-2012               */
/*              (C) Copyright Paul F. Gorlinsky, 2010                */
/*              (C) Copyright TurboHercules, SAS, 2011               */
/*         Displays information about a Hercules Emulated Tape       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*
|| ----------------------------------------------------------------------------
||
|| HETMAP.C     (C) Copyright Leland Lucius, 2000-2009
||              (C) Copyright Paul F Gorlinsky 2010 (SLANAL changes)
||              Released under terms of the Q Public License.
||
|| Displays information about the structure of a Hercules Emulated Tape.
||
|| ----------------------------------------------------------------------------
*/

#include "hstdinc.h"

#include "hercules.h"
#include "hetlib.h"
#include "ftlib.h"
#include "sllib.h"
#include "herc_getopt.h"

#define UTILITY_NAME    "hetmap"
#define UTILITY_DESC    "AWS, HET and FakeTape map program"

/*
|| Local Types
*/
typedef unsigned char                   UInt8;
typedef signed char                     SInt8;
typedef unsigned short                  UInt16;
typedef signed short                    SInt16;

#if __LP64__
typedef unsigned int                    UInt32;
typedef signed int                      SInt32;
#else
typedef unsigned long                   UInt32;
typedef signed long                     SInt32;
#endif

/*
|| The MS Visual C/C++ compiler uses __int64 instead of long long.
*/
#if defined(_MSC_VER) && !defined(__MWERKS__) && defined(_M_IX86)
typedef   signed __int64                SInt64;
typedef unsigned __int64                UInt64;
#else
typedef   signed long long              SInt64;
typedef unsigned long long              UInt64;
#endif

typedef unsigned char                   Boolean;

/*
|| Local Defines
*/
#ifdef bytes_per_line
#undef bytes_per_line
#endif
#define bytes_per_line 32
/*      ^------- number of bytes to convert on each line; multiples of 4 only
 *
 */
#ifdef dmax_bytes_dsply
#undef dmax_bytes_dsply
#endif
#define dmax_bytes_dsply 1024
/*      ^------- maximum number of bytes from prtbuf to convert anything > 4
 *
 */

#define DO_FOREVER  while(1)

/*      memset replacements
 */
#define BLANK_OUT(a, b) { memset(a, ' ', b - 1); a[b-1] = '\0'; }
#define ZERO_OUT(a, b)  memset((void*)(a), 0, (size_t)(b))
#define TERMINATE(a)    a[sizeof(a)-1] = '\0'

/*----------------------------------------------------------------------*
 *      Translate tables, ebcdic-ascii, ebcdic-printable.ascii,         *
 *      ascii-printable.ascii                                           *
 *—————————————————————————————————————————————————————————————————————-*/
#define TranslateTables
static char
ebcdic_to_ascii[] = {
    "\x00\x01\x02\x03\xA6\x09\xA7\x7F\xA9\xB0\xB1\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\xB2\x0A\x08\xB7\x18\x19\x1A\xB8\xBA\x1D\xBB\x1F"
    "\xBD\xC0\x1C\xC1\xC2\x0A\x17\x1B\xC3\xC4\xC5\xC6\xC7\x05\x06\x07"
    "\xC8\xC9\x16\xCB\xCC\x1E\xCD\x04\xCE\xD0\xD1\xD2\x14\x15\xD3\xFC"
    "\x20\xD4\x83\x84\x85\xA0\xD5\x86\x87\xA4\xD6\x2E\x3C\x28\x2B\xD7"
    "\x26\x82\x88\x89\x8A\xA1\x8C\x8B\x8D\xD8\x21\x24\x2A\x29\x3B\x5E"
    "\x2D\x2F\xD9\x8E\xDB\xDC\xDD\x8F\x80\xA5\x7C\x2C\x25\x5F\x3E\x3F"
    "\xDE\x90\xDF\xE0\xE2\xE3\xE4\xE5\xE6\x60\x3A\x23\x40\x27\x3D\x22"
    "\xE7\x61\x62\x63\x64\x65\x66\x67\x68\x69\xAE\xAF\xE8\xE9\xEA\xEC"
    "\xF0\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\xF1\xF2\x91\xF3\x92\xF4"
    "\xF5\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\xAD\xA8\xF6\x5B\xF7\xF8"
    "\x9B\x9C\x9D\x9E\x9F\xB5\xB6\xAC\xAB\xB9\xAA\xB3\xBC\x5D\xBE\xBF"
    "\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49\xCA\x93\x94\x95\xA2\xCF"
    "\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xDA\x96\x81\x97\xA3\x98"
    "\x5C\xE1\x53\x54\x55\x56\x57\x58\x59\x5A\xFD\xEB\x99\xED\xEE\xEF"
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xFE\xFB\x9A\xF9\xFA\xFF"
};

static char
ebcdic_to_printable_ascii[] = {
/*     x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xA  xB  xC  xD  xE  xF           */
    "\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"  /*  0x  */
    "\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"  /*  1x  */
    "\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"  /*  2x  */
    "\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"  /*  3x  */
    "\x20\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x3C\x28\x2B\x3C"  /*  4x  */
    "\x26\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x24\x2A\x29\x3B\x5E"  /*  5x  */
    "\x2D\x2F\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x7C\x2C\x25\x5F\x3E\x3F"  /*  6x  */
    "\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x60\x3A\x23\x40\x27\x3D\x22"  /*  7x  */
    "\x2E\x61\x62\x63\x64\x65\x66\x67\x68\x69\x2E\x2E\x2E\x2E\x2E\x2E"  /*  8x  */
    "\x2E\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x2E\x2E\x2E\x2E\x2E\x2E"  /*  9x  */
    "\x2E\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\x2E\x2E\x2E\x5B\x2E\x2E"  /*  Ax  */
    "\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x5D\x2E\x2E"  /*  Bx  */
    "\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49\x2E\x2E\x2E\x2E\x2E\x2E"  /*  Cx  */
    "\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x2E\x2E\x2E\x2E\x2E\x2E"  /*  Dx  */
    "\x5C\x2E\x53\x54\x55\x56\x57\x58\x59\x5A\x2E\x2E\x2E\x2E\x2E\x2E"  /*  Ex  */
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x2E\x2E\x2E\x2E\x2E\x2E"  /*  Fx  */
};

static char
ascii_to_printable_ascii[] = {
    /*   x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xA  xB  xC  xD  xE  xF     */
    /*0x*/"\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"
    /*1x*/"\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"
    /*2x*/"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F"
    /*3x*/"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F"
    /*4x*/"\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
    /*5x*/"\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F"
    /*6x*/"\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F"
    /*7x*/"\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x2E"
    /*8x*/"\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"
    /*9x*/"\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E\x2E"
    /*Ax*/"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F"
    /*Bx*/"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F"
    /*Cx*/"\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
    /*Dx*/"\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F"
    /*Ex*/"\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F"
    /*Fx*/"\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x2E"
};
#undef TranslateTables

/*----------------------------------------------------------------------*
 *      prototypes                                                      *
 *—————————————————————————————————————————————————————————————————————-*/
#define Prototypes
int     main                            ( int, char * [] );
void    Print_Dataset                   ( SInt32, SInt32 );
void    Print_Label                     ( SInt32 );
void    Print_Label_Tapemap             ( SInt32 );
void    Print_Usage                     ( const char* pgm );

static Boolean  Print_Standard_Labels   ( void );
static SInt32   Print_Block_Data        ( SInt32 );
#undef Prototypes

/*
|| Local constant data
*/
static char sep[] = "---------------------\n";
/* Previous reported file position */
static off_t prevpos = 0;
/* Report progress every this many bytes */
#define PROGRESS_MASK (~0x3FFFF /* 256K */)

static UInt32   gLastFileSeqSL  = 0;    /* Last SL file number  */
static char     gStdLblBuffer[81];
static char     gMltVolSet[7];
static char     gMltVolSeq[5];
static BYTE     gBuffer[ HETMAX_BLOCKSIZE + 1 ];
static UInt32   gBlkCount   = 0;    /* Block count                  */
static UInt32   gPrevBlkCnt = 0;    /* Block count                  */
static SInt32   gLength     = 0;    /* Block length                 */
static SInt32   gLenPrtd    = 0;    /* amount of data print for cur */
static SInt32   max_bytes_dsply = dmax_bytes_dsply;

/*
|| Standard main
*/
int
main( int argc, char *argv[] )
{
    HETB *hetb;
    FETB *fetb;
    char *i_filename;
    int   i_faketape = FALSE;
    SInt32  rc;
    SInt32  fileno;
    SInt32  i;
    U32  uminsz;
    U32  umaxsz;
    U32  ubytes;
    U32  cminsz;
    U32  cmaxsz;
    U32  cbytes;
    U32  totblocks;
    U64  totubytes;
    U64  totcbytes;
    U32  opts = 0;
    SInt32  lResidue    = max_bytes_dsply;  /* amount of space left to print */
    char *pgm;

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

#define O_ALL               0xC0
#define O_FILES             0X80
#define O_LABELS            0X40
#define O_DATASETS          0X20
#define O_TAPEMAP_OUTPUT    0x10
#define O_TAPEMAP_INVOKED   0x08
#define O_SLANAL_OUT        0x04

    if (! (opts & O_TAPEMAP_INVOKED) )
    {

        opts = O_ALL;

        while( TRUE )
        {
            rc = getopt( argc, argv, "ab:dfhlst" );
            if( rc == -1 )
                break;

            switch( rc )
            {
                case 'a':
                    opts = O_ALL;
                    break;
                case 'b':
                    max_bytes_dsply = atoi( optarg );
                    if ( max_bytes_dsply < 256 ) max_bytes_dsply = 256;
                    else
                    {
                        int i;
                        i = max_bytes_dsply % 4;
                        max_bytes_dsply += (4-i);
                    }
                    opts = O_SLANAL_OUT;
                    break;
                case 'd':
                    opts = O_DATASETS;
                    break;
                case 'f':
                    opts = O_FILES;
                    break;
                case 'h':
                    Print_Usage( pgm );
                    return 1;
                case 'l':
                    opts = O_LABELS;
                    break;
                case 's':
                    opts = O_SLANAL_OUT;
                    break;
                case 't':
                    opts = O_TAPEMAP_OUTPUT;
                    break;
                default:
                    Print_Usage( pgm );
                    return 1;
            }
        }

    }  // end if (! (opts & O_TAPEMAP_INVOKED) )

    argc -= optind;
    if( argc != 1 )
    {
        Print_Usage( pgm );
        exit( 1 );
    }

    if( opts & O_ALL )
    {
        printf( "%s", sep );
        printf( "%-20.20s: %s\n", "Filename", argv[ optind ] );
    }

    i_filename = argv[ optind ];

    if ( ( rc = (int)strlen( i_filename ) ) > 4 && ( rc = strcasecmp( &i_filename[rc-4], ".fkt" ) ) == 0 )
    {
        i_faketape = TRUE;
    }

    if ( i_faketape )
        rc = fet_open( &fetb, i_filename, FETOPEN_READONLY );
    else
        rc = het_open( &hetb, i_filename, HETOPEN_READONLY );
    if( rc < 0 )
    {
        if ( i_faketape )
        {
            printf( "fet_open() returned %d\n", (int)rc );
            fet_close( &fetb );
        }
        else
        {
            printf( "het_open() returned %d\n", (int)rc );
            het_close( &hetb );
        }
        exit( 1 );
    }

    BLANK_OUT ( gStdLblBuffer, sizeof ( gStdLblBuffer ) );

    fileno = 0;
    gBlkCount = 0;

    uminsz = 0;
    umaxsz = 0;
    ubytes = 0;
    cminsz = 0;
    cmaxsz = 0;
    cbytes = 0;

    totblocks = 0;
    totubytes = 0;
    totcbytes = 0;

    while( TRUE )
    {
        if( extgui )
        {
            off_t curpos;
            /* Report progress every nnnK */
            if ( i_faketape )
                curpos = ftell( fetb->fh );
            else
                curpos = ftell( hetb->fh );
            if( ( curpos & PROGRESS_MASK ) != ( prevpos & PROGRESS_MASK ) )
            {
                prevpos = curpos;
                EXTGUIMSG( "IPOS=%"PRId64"\n", (U64)curpos );
            }
        }

        if ( i_faketape)
        {
            rc = fet_read( fetb, gBuffer );
        }
        else
            rc = het_read( hetb, gBuffer );
        if( rc == HETE_EOT )                    // FETE and HETE enums are the same
        {
            if( opts & O_TAPEMAP_OUTPUT )
            {
                printf ("End of tape.\n");
            }
            break;
        }

        if( rc == HETE_TAPEMARK )
        {
            fileno += 1;

            lResidue = max_bytes_dsply;

            if( opts & O_TAPEMAP_OUTPUT )
            {
                printf ("File %d: Blocks=%d, block size min=%d, max=%d\n",
                        (int)fileno, (int)gBlkCount, (int)uminsz, (int)umaxsz      );
            }

            if( opts & O_FILES )
            {
                printf ( "%s", sep );
                printf ( "%-20.20s: %d\n", "File #", (int)fileno );
                printf ( "%-20.20s: %d\n", "Blocks", (int)gBlkCount );
                printf ( "%-20.20s: %d\n", "Min Blocksize", (int)uminsz );
                printf ( "%-20.20s: %d\n", "Max Blocksize", (int)umaxsz );
                if ( !i_faketape )
                {
                    printf ( "%-20.20s: %d\n", "Uncompressed bytes", (int)ubytes );
                    printf ( "%-20.20s: %d\n", "Min Blocksize-Comp", (int)cminsz );
                    printf ( "%-20.20s: %d\n", "Max Blocksize-Comp", (int)cmaxsz );
                    printf ( "%-20.20s: %d\n", "Compressed bytes", (int)cbytes );
                }
            }

            totblocks += gBlkCount;
            totubytes += ubytes;
            totcbytes += cbytes;

            gPrevBlkCnt = gBlkCount;
            gBlkCount = 0;

            uminsz = 0;
            umaxsz = 0;
            ubytes = 0;
            cminsz = 0;
            cmaxsz = 0;
            cbytes = 0;

            continue;
        }

        if( rc < 0 )
        {
            if ( i_faketape )
                printf ( "fet_read() returned %d\n", (int)rc );
            else
                printf ( "het_read() returned %d\n", (int)rc );
            break;
        }

        gBlkCount += 1;
        if ( !i_faketape )
        {
            ubytes += hetb->ublksize;
            cbytes += hetb->cblksize;

            if( uminsz == 0 || hetb->ublksize < uminsz ) uminsz = hetb->ublksize;
            if( hetb->ublksize > umaxsz ) umaxsz = hetb->ublksize;
            if( cminsz == 0 || hetb->cblksize < cminsz ) cminsz = hetb->cblksize;
            if( hetb->cblksize > cmaxsz ) cmaxsz = hetb->cblksize;
        }

        if ( rc >= 80 )
        {
            for (i=0; i < 80; i++)
            {
                gStdLblBuffer[i] = ebcdic_to_ascii[gBuffer[i]];
            }
            gStdLblBuffer[i] = '\0';
        }

        if( opts & O_LABELS )
        {
            Print_Label( rc );
        }

        if( opts & O_TAPEMAP_OUTPUT )
        {
            Print_Label_Tapemap( rc );
        }

        if( opts & O_DATASETS )
        {
            Print_Dataset( rc, fileno );
        }

        if( opts & O_SLANAL_OUT )
        {
            gLength = rc;

            if ( gLength == 80 )
            {
                if ( 0
                    || memcmp ( gStdLblBuffer, "HDR", 3 ) == 0
                    || memcmp ( gStdLblBuffer, "EOF", 3 ) == 0
                    || memcmp ( gStdLblBuffer, "VOL", 3 ) == 0
                    || memcmp ( gStdLblBuffer, "EOV", 3 ) == 0
                    || memcmp ( gStdLblBuffer, "UHL", 3 ) == 0
                    || memcmp ( gStdLblBuffer, "UTL", 3 ) == 0 )
                {
                    if ( !Print_Standard_Labels (  ) )
                    {
                        if ( gBlkCount <= 10 && ( gBlkCount == 1 || ( gBlkCount > 1 && lResidue > bytes_per_line ) ) )
                        {
                            gLenPrtd = ( gBlkCount == 1 ? ( gLength <= max_bytes_dsply ? gLength : max_bytes_dsply ) :
                                        ( gLength <= lResidue ? gLength : lResidue ) );
                            lResidue -= Print_Block_Data ( gLenPrtd );
                        }
                    }
                }
                else
                {
                    if ( gBlkCount <= 10 && ( gBlkCount == 1 || ( gBlkCount > 1 && lResidue > bytes_per_line ) ) )
                    {
                        gLenPrtd = ( gBlkCount == 1 ? ( gLength <= max_bytes_dsply ? gLength : max_bytes_dsply ) :
                                    ( gLength <= lResidue ? gLength : lResidue ) );
                        lResidue -= Print_Block_Data ( gLenPrtd );
                    }
                }
            }
            else
            {
                if ( ( gBlkCount <= 10 ) && ( ( gBlkCount == 1 ) || ( ( gBlkCount > 1 ) && ( lResidue > bytes_per_line ) ) ) )
                {
                    gLenPrtd = ( gBlkCount == 1 ? ( gLength <= max_bytes_dsply ? gLength : max_bytes_dsply ) :
                                ( gLength <= lResidue ? gLength : lResidue ) );
                    lResidue -= Print_Block_Data ( gLenPrtd );
                }
            }
        }
    }

    if( opts & O_FILES )
    {
        printf ( "%s", sep );
        printf ( "%-20.20s:\n", "Summary" );
        printf ( "%-20.20s: %d\n", "Files", (int)fileno );
        printf ( "%-20.20s: %d\n", "Blocks", (int)totblocks );
        if ( !i_faketape )
        {
            printf ( "%-20.20s: %llu\n", "Uncompressed bytes", (unsigned long long)totubytes );
            printf ( "%-20.20s: %llu\n", "Compressed bytes", (unsigned long long)totcbytes );
            printf ( "%-20.20s: %llu\n", "Reduction", (unsigned long long)(totubytes - totcbytes) );
        }
    }

    if ( i_faketape )
        fet_close( &fetb );
    else
        het_close( &hetb );

    return 0;
}

/*
 || Print terse dataset information (from VOL1/EOF1/EOF2)
 */
void
Print_Dataset( SInt32 len, SInt32 fileno )
{
    SLLABEL lab;
    SLFMT fmt;
    char crtdt[ 9 ];
    char expdt[ 9 ];
    char recfm[ 4 ];

    if( sl_islabel( &lab, gBuffer, len ) == FALSE )
    {
        return;
    }

    sl_fmtlab( &fmt, &lab );

    if( sl_isvol( gBuffer, 1 ) )
    {
        printf ( "vol=%-17.17s  owner=%s\n\n",
               fmt.slvol.volser,
               fmt.slvol.owner);
    }
    else if( sl_iseof( gBuffer, 1 ) )
    {
        printf ( "seq=%-17d  file#=%d\n",
               atoi( fmt.slds1.dsseq ),
               (int)fileno );
        printf ( "dsn=%-17.17s  crtdt=%-8.8s  expdt=%-8.8s  blocks=%d\n",
               fmt.slds1.dsid,
               sl_fmtdate( crtdt, fmt.slds1.crtdt, TRUE ),
               sl_fmtdate( expdt, fmt.slds1.expdt, TRUE ),
               atoi( fmt.slds1.blkhi ) * 1000000 + atoi( fmt.slds1.blklo ) );
    }
    else if( sl_iseof( gBuffer, 2 ) )
    {
        recfm[ 0 ] = '\0';
        STRLCAT( recfm, fmt.slds2.recfm   );
        STRLCAT( recfm, fmt.slds2.blkattr );
        STRLCAT( recfm, fmt.slds2.ctrl    );
        printf ( "job=%17.17s  recfm=%-3.3s       lrecl=%-5d     blksize=%-5d\n\n",
               fmt.slds2.jobid,
               recfm,
               atoi( fmt.slds2.lrecl ),
               atoi( fmt.slds2.blksize ) );
    }

    return;
}

/*
 || Print all label fields
 */
void
Print_Label( SInt32 len )
{
    SLLABEL lab;
    SLFMT fmt;
    SInt32 i;

    if( sl_islabel( &lab, gBuffer, len ) == FALSE )
    {
        return;
    }

    sl_fmtlab( &fmt, &lab );

    printf ( "%s", sep );

    for( i = 0; fmt.key[ i ] != NULL; i++ )
    {
        printf ("%-20.20s: '%s'\n", fmt.key[ i ] , fmt.val[ i ] );
    }

    return;
}

/*
 || Print label fields in TAPEMAP format
 */
void
Print_Label_Tapemap( SInt32 len )
{
    SLLABEL lab;
    char labelrec[81];
    SInt32 i;

    if( sl_islabel( &lab, gBuffer, len ) == FALSE )
    {
        return;
    }
    for (i=0; i < 80; i++)
    {
        labelrec[i] = guest_to_host(gBuffer[i]);
    }
    labelrec[i] = '\0';
    printf ("%s\n", labelrec);
}

/*
 || Prints usage information
 */
void
Print_Usage( const char* pgm )
{
    if (strcasecmp( pgm, "tapemap" ))
    {
        // "Usage: %s ...
        WRMSG( HHC02758, "I", pgm ); // HHC02758 = tapemap
    }
    else
    {
        // "Usage: %s ...
        WRMSG( HHC02759, "I", pgm ); // HHC02759 = hetmap
    }
}

/*----------------------------------------------------------------------*
 *      prototypes                                                      *
 *—————————————————————————————————————————————————————————————————————-*/
static Boolean
Print_Standard_Labels (void )
{
    SInt32                  i = 0;
    Boolean                 rc = FALSE;
    char                    lLblType[4];
    char                    lLblNum[2];
    SInt32                  iLblType = 0;

    sscanf ( gStdLblBuffer, "%3c%1c%*76c", lLblType, lLblNum );
    TERMINATE(lLblType);
    TERMINATE(lLblNum);

    if ( isdigit( (unsigned char)lLblNum[0] ) )
    { if ( lLblNum[0] < '1' || lLblNum[0] > '9' ) return ( FALSE ); } /* this should be transportable to EBCDIC machines */
    else
        return ( FALSE );

    if ( strcmp ( lLblType, "VOL" ) == 0 )      iLblType = 1;       /* VOL  */

    if ( ( strcmp ( lLblType, "HDR" ) == 0 ) ||
        ( strcmp ( lLblType, "EOF" ) == 0 ) ||
        ( strcmp ( lLblType, "EOV" ) == 0 ) )   iLblType = 2;       /* HDR | EOF | EOV  */

    if ( ( strcmp ( lLblType, "UHL" ) == 0 ) ||
        ( strcmp ( lLblType, "UTL" ) == 0 ) )   iLblType = 3;       /* UHL | UTL    User Labels */

    switch ( iLblType )
    {
        case    1:
        {
            char    volser[7];                              /* ( 5-10) Volume ID                */
            char    vsec[2]                                 /* (   11) Volume Accessability     */
            /* 5 bytes  */                                  /* (12-16) VTOC Pointer (not used)  */
            ;/* 21 bytes */                                 /* (17-37) Reserved                 */
            char    owner[15]                               /* (38-51) Owner ID                 */
            ;/* 29 bytes */                                 /* (52-80) Reserved                 */

            /*       1...5...10...15...20...25...30...35...40...45...50...55...60...65...70...75...80
             *       VOL1volsr|sRESERVED-----------------|owner--------|RESERVED--------------------|
             */
            sscanf ( gStdLblBuffer, "%*3c%*1c%6c%1c%*5c%*21c%14c%*29c", volser, vsec, owner );

            TERMINATE(volser);      /* Null terminate the arrays */
            TERMINATE(vsec);
            TERMINATE(owner);

            printf ( "\n%-2s", "" );

            if ( atoi( lLblNum ) == 1 )
                printf ( "Standard Label Tape\n\n%-4s"
                        "VolSer: %-10s"
                        "Owner: %s\n", "", volser, owner );
            else
                printf ( "%-4s %1s %-s\n", lLblType, lLblNum, &gStdLblBuffer[4] );
        }
            rc = TRUE;
            break;

        case    2:
        {
            switch ( atoi( lLblNum ) )
            {
                case 1:
                {
                    char    fid[18];                /* ( 5-21) rightmost 17 char of file Identifier (dataset name DSN)  */
                    char    afset[7];               /* (22-27) Aggregate volume ID (volser 1st vol)                     */
                    char    afseq[5];               /* (28-31) Aggregate volume seq (multi-volume)                      */
                    char    fseq[5];                /* (32-35) file seq number 0001-9999 < x'6F'xxxxxx 1-64000(bin)     */
                    char    gen[5];                 /* (36-39) generation number    (not used)                          */
                    char    gver[3];                /* (40-41) generation version   (not used)                          */
                    char    cdate[7]                /* (42-47) creation date                                            */
                    ;                               /*         cyyddd;c = blank 19, 0 = 20, 1 = 21; jdate               */
                    char    edate[7];               /* (48-53) expiration date                                          */
                    char    fsec[2]                 /* (   54) File Security        (not used)                          */
                    ;                               /*              0=none,1=pwd-R-W-Del,3=pwd-W-Del                    */
                    char    bcnt[7];                /* (55-60) block count (blockcnt % 1000000) (HDR=0) (EOF/EOV)       */
                    char    impid[14]               /* (61-73) System Code (IBMOS400|IBM OS/VS 370)                     */
                    ;                               /* (74-76) Reserved                                                 */
                    char    ebcnt[5];               /* (77-80) extended block count (blockcnt / 1000000)(EOF/EOV)       */

                    /*       1...5...10...15...20...25...30...35...40...45...50...55...60...65...70...75...80
                     *       HDR1DSNAME----------|afvst|afs|fsq|---|-|cdate|edate||bcnt-|syscode-----|RR|ebct
                     *       {EOF}                                 n/a         fsec^
                     *       {EOV}
                     */
                    sscanf( gStdLblBuffer, "%*3c%*1c%17c%6c%4c%4c%4c%2c%6c%6c%1c%6c%13c%*3c%4c",
                           fid, afset, afseq, fseq, gen, gver, cdate, edate, fsec, bcnt, impid, ebcnt);

                    TERMINATE(fid);             /* NULL Terminate the arrays */
                    TERMINATE(afset);
                    TERMINATE(afseq);
                    TERMINATE(fseq);
                    TERMINATE(gen);
                    TERMINATE(gver);
                    TERMINATE(cdate);
                    TERMINATE(edate);
                    TERMINATE(fsec);
                    TERMINATE(bcnt);
                    TERMINATE(impid);
                    TERMINATE(ebcnt);

                    if ( lLblType[0] == 'E' )
                    {
                        for ( i = 0; i < 4; i++ ) { if ( !isdigit( (unsigned char)ebcnt[i] ) ) ebcnt[i] = '0'; }
                        ebcnt[4] = '\0';
                    }
                    else
                        if ( atoi( lLblNum ) == 1 )
                            printf ("\f");

                    if ( fseq[0] == '?' )       /* this is the indicator that IBM uses for seq no > 9999 ebcdic x'6f' */
                    {
                        fseq[0] = '\x00';
                        gLastFileSeqSL = ( ( fseq[0]    << 24 ) & 0xff000000 )
                        |( ( fseq[1]    << 16 ) & 0x00ff0000 )
                        |( ( fseq[2]    << 8  ) & 0x0000ff00 )
                        |( ( fseq[3]          ) & 0x000000ff );
                    }
                    else
                        gLastFileSeqSL = (UInt32)atol( fseq );

                    printf ( "\n%-4s"
                            "SL File Seq: %-4d%-3s"
                            "DSNAME: %-20s"
                            , "", atoi( fseq ), "", fid );

                    printf ( "Created: " );
                    if ( cdate[0] == ' ' )
                        if ( (int)( atol( &cdate[1] ) / 1000l ) < 1967 )
                            printf ( "20" );
                        else
                            printf ( "19" );
                    else
                        printf ( "2%1c", cdate[0] );
                    printf ( "%02d.%03d%-3s", (int)( atol( &cdate[1] ) / 1000l ), atoi( &cdate[3] ), "" );

                    if ( strcmp ( &edate[1], "00000" ) !=0 )
                    {
                        printf ( "Expires: " );
                        if ( atoi ( &edate[3] ) == 0 )
                            printf ( "TMS-%-5s", &edate[1] );
                        else
                        {
                            if ( edate[0] == ' ' )
                                if ( (int)( atol( &edate[1] ) / 1000l ) < 1967 )
                                    printf ( "20" );
                                else
                                    printf ( "19" );
                            else
                                printf ( "2%1c", edate[0] );
                            printf ( "%02d.%03d%-1s", (int)( atol( &edate[1] ) / 1000l ), atoi( &edate[3] ), "" );
                        }
                    }
                    else
                        printf ( "%-9s", "NO EXPDT" );

                    printf ( "%-3sSystem: %s\n", "", impid );

                    if ( gStdLblBuffer[0] == 'E' )
                    {
                        UInt64   lBlockCnt  = (UInt64)(atol( bcnt ) % 1000000l) + (UInt64)(atol( ebcnt ) * 1000000l);
                        printf ( "%-4sBlock Count: "
                                "Expected %llu; "
                                "Actual %d",
                                "", lBlockCnt, (int)gPrevBlkCnt );
                        if ( lBlockCnt == (UInt64)gPrevBlkCnt )
                            printf ( "\n" );
                        else
                            printf ( "%-4s---> BLOCK COUNT MISMATCH <---\n", "" );
                    }
                    else
                    {
                        gMltVolSet[0] = '\0';
                        gMltVolSeq[0] = '\0';
                        strcpy ( gMltVolSet, afset );
                        strcpy ( gMltVolSeq, afseq );
                    }
                }
                    break;

                case 2:
                {
                    char    fmt[2];                             /* (    5) Format F=fixed;V=variable;U=unblock              */
                    char    bsize[6];                           /* ( 6-10) Block Size 1-32767 (>32767 see large block size) */
                    char    rsize[6];                           /* (11-15) Record Size                                      */
                    char    tden[2];                            /* (   16) Density of tape 3=1600,4=6250,5=3200,blank=others */
                    char    mltv[2];                            /* (   17) Multi-volume switch 1/0 2nd + tape seq num       */
                    char    jname[9]                            /* (18-25) Job Name creating tape                           */
                    ;/* 1 byte */                               /* (   26) '/' Separator                                    */
                    char    sname[9];                           /* (27-34) Step Name creating tape                          */
                    char    rtech[3];                           /* (35-36) Adv. Recording tech. blank=none;'P '=IDRC        */
                    char    pcchr[2];                           /* (   37) Printer Control Char A=ANSI;M=machine            */
                    char    battr[2]                            /* (   38) Block Attr B=blkd;S=Spanned(V)|Std(F);R=B&S      */
                    ;/* 3 bytes */                              /* (39-47) Reserved                                         */
                    char    ckpt[2]                             /* (   48) Chkpt Data Set ID; C=secure CKPT dsn;blank - not */
                    ;/* 22 chars */                             /* (49-70) Reserved                                         */
                    char    lbsiz[11];                          /* (71-80) Large Block Size > 32767                         */

                    char    tmp[10];
                    char    dcb[80];

                    /*       1...5...10...15...20...25...30...35...40...45...50...55...60...65...70...75...80
                     *       HDR2|bsiz|rsiz|||jname--|/sname--|r||R|RR000000|RESERVED-------------|lbsize---|
                     *      {EOF}^-- FORMAT |^- MULTI-VOL        | ^- BLK'D ^- CKPT
                     *      {EOV}           ^-- DENSITY          ^-CC {A|M| }
                     */
                    sscanf( gStdLblBuffer,
                           "%*3c"               /*  3 HDR | EOF | EOV                           */
                           "%*1c"               /*  1 1-9                                       */
                           "%1c"                /*  1 fmt                                       */
                           "%5c"                /*  5 bsize                                     */
                           "%5c"                /*  5 rsize                                     */
                           "%1c"                /*  1 tden                                      */
                           "%1c"                /*  1 mltv      Multi-volume switch indicator   */
                           "%8c"                /*  8 jname                                     */
                           "%*1c"               /*  1 '/'                                       */
                           "%8c"                /*  8 sname                                     */
                           "%2c"                /*  2 rtech                                     */
                           "%1c"                /*  1 cc        A | M                           */
                           "%*1c"               /*  1 reserved                                  */
                           "%1c"                /*  1 battr     B | S | BS | ' '                */
                           "%*2c"               /*  2 reserved                                  */
                           "%*6c"               /*  6 Device Serial number or 6 blanks          */
                           "%1c"                /*  1 ckpt      Checkpoint Data Set Id          */
                           "%*22c"              /* 22 reserved                                  */
                           "%10c"               /* 10 lbsize    large block size (> 32767)      */
                           , fmt, bsize, rsize, tden, mltv, jname, sname, rtech, pcchr, battr, ckpt, lbsiz);

                    TERMINATE(fmt);         /* NULL terminate the arrays */
                    TERMINATE(bsize);
                    TERMINATE(rsize);
                    TERMINATE(tden);
                    TERMINATE(mltv);
                    TERMINATE(jname);
                    TERMINATE(sname);
                    TERMINATE(rtech);
                    TERMINATE(pcchr);
                    TERMINATE(battr);
                    TERMINATE(ckpt);
                    TERMINATE(lbsiz);

                    if ( gStdLblBuffer[0] == 'H' )
                    {
                        tmp[0] = dcb[0] = '\0';

                        printf ( "%-4sCreated by: Job %-8s; Step %-11s%-6s"
                                , "", jname, sname, "" );

                        STRLCAT( dcb, "DCB=(RECFM=" );

                        STRLCAT( dcb, fmt );            /* first character of the RECFM F|V|U                   */
                                                        /* next 'S' means SPANNED for 'V' and STANDARD for 'F'  */
                        if ( battr[0] == 'R' )          /* next 1 or 2 (if = 'R') characters B|S|R|' '          */
                            STRLCAT( dcb, "BS" );       /* 'R' = both B & S together                            */
                        else
                            if ( battr[0] != ' ' )
                                STRLCAT( dcb, battr );  /* just the B|S if not 'R' - blank is not included      */

                        if ( pcchr[0] != ' ' )
                            STRLCAT( dcb, pcchr );      /* last is the printer carriage control type A|M        */
                                                        /* A = ANSI and M = Machine                             */
                        STRLCAT( dcb, ",LRECL=" );
                        sprintf( tmp, "%d", atoi( rsize ) );
                        STRLCAT( dcb, tmp );

                        STRLCAT( dcb, ",BLKSIZE=" );
                        if ( lbsiz[0] == '0' )
                            sprintf ( tmp, "%ld", atol( lbsiz ) );
                        else
                            sprintf ( tmp, "%d", atoi( bsize ) );
                        STRLCAT( dcb, tmp );

                        STRLCAT( dcb, ")" );

                        printf ( "%-51s", dcb );

                        if ( mltv[0] == '1' || atoi( gMltVolSeq ) > 1 )
                            printf ( "\n%-4sTape is part %d of multi-volume set %s\n", "", atoi( gMltVolSeq ), gMltVolSet );

                        if ( rtech[0] == 'P' )
                            printf ( "%-3sCompression: IDRC", "" );

                        printf ( "\n" );
                    }
                    else
                        printf( "======================================"
                                "======================================"
                                "======================================"
                                "======================================\n" );
                }
                    break;

                default:
                    printf ( "%-4s %1d %-s\n", lLblType, *lLblNum, &gStdLblBuffer[4] );
                    break;
            }
        }
            rc = TRUE;
            break;

        case        3:
            printf ( "%-4s %1d %-s\n", lLblType, *lLblNum, &gStdLblBuffer[4] );
            rc = TRUE;
            break;

        default:
            rc = FALSE;
            break;
    }

    ZERO_OUT ( gStdLblBuffer, sizeof( gStdLblBuffer ) );
    return ( rc );

} /* end function Print_Standard_Labels */

/*----------------------------------------------------------------------------*
 *      Print <= 1K of Data in Printable HEX, the ASCII Char, the EBCDIC Char *
 *————————————————————————————————————————————————————————————————————------—-*/
static SInt32
Print_Block_Data    ( SInt32 prtlen )
{
    SInt32      B, I, J, K, Kr, Kl, Cl;
    char*       pAsciiBuf;
    char*       pAscii;
    char*       pEbcdicBuf;
    char*       pEbcdic;
    char*       pSpaces;
    char        lPadding[bytes_per_line + 1];
    SInt32      lLenSpace   = ( ( bytes_per_line / 4 ) * 9 ) + ( bytes_per_line / 16 ) - 7;
    SInt32      lAmt2Prt    = 0;
    static char lASCII[7] = "ASCII ";
    static char lEBCDIC[7] = "EBCDIC";

    if ( gBlkCount == 1 )
    {   if ( prtlen >= max_bytes_dsply )
        lAmt2Prt = max_bytes_dsply;
    else
        lAmt2Prt = prtlen;
    }
    else
        lAmt2Prt = prtlen;

    pAsciiBuf   = malloc ( max_bytes_dsply );
    pEbcdicBuf  = malloc ( max_bytes_dsply );
    pAscii      = malloc ( bytes_per_line + 1 );
    pEbcdic     = malloc ( bytes_per_line + 1 );

    pSpaces = malloc( lLenSpace + 12 );
    BLANK_OUT ( pSpaces, lLenSpace + 12 );

    ZERO_OUT ( lPadding, sizeof ( lPadding ) );

    for ( B = 0; B < (SInt32)lAmt2Prt; B++ )
    {
        pAsciiBuf[B] = ascii_to_printable_ascii[(int)gBuffer[B]];
        pEbcdicBuf[B] = ebcdic_to_printable_ascii[(int)gBuffer[B]];
    }

    Kl = bytes_per_line;

    K  = lAmt2Prt / Kl;             /* how many lines             */
    Kr = lAmt2Prt % Kl;             /* how many on line last line */

    B = 0;

    J = ( ( bytes_per_line - sizeof ( lASCII ) + 1 ) / 2 ) + ( ( bytes_per_line - sizeof ( lASCII ) + 1 ) % 2) + 1;

    BLANK_OUT ( lPadding, J );

    printf ( "\n\nADDR%-4sBLOCK %-2d BYTES %-6d%s*%s%s%s|%s%s%s*\n"
            , ""
            , (int)gBlkCount
            , (int)lAmt2Prt
            , &pSpaces[25]
            , lPadding
            , lEBCDIC
            , lPadding
            , lPadding
            , lASCII
            , lPadding );

    for ( I = 0; I <= K; I++ )      /* number of lines to do */
    {
        if ( I == K )
        {
            if ( Kr == 0 ) continue;
            Kl = Kr;
        }

        printf ( "%04X    ", (int)B );

        BLANK_OUT ( pAscii,  bytes_per_line + 1 );
        BLANK_OUT ( pEbcdic, bytes_per_line + 1 );

        memcpy( pAscii,  &pAsciiBuf [B], Kl ); pAscii [ bytes_per_line ] = '\0';
        memcpy( pEbcdic, &pEbcdicBuf[B], Kl ); pEbcdic[ bytes_per_line ] = '\0';

        for ( Cl = 4, J = 0; J < Kl; J++ )
        {
            printf( "%02X", gBuffer[B++] );
            Cl += 2;
            if ( ( ( J + 1 ) % 16 ) == 0 ) { Cl++; printf ( " " ); }
            if ( ( ( J + 1 ) % 4 ) == 0 ) { Cl++; printf ( " " ); }
        }

        printf ( "%s*%s|%s*\n", ( pSpaces + Cl ), pEbcdic, pAscii );
    }

    printf ( "\n" );

    return ( lAmt2Prt );
}
