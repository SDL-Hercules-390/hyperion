/* HETUPD.C     (C) Copyright Leland Lucius, 2000-2012               */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*                 Update/Copy Hercules Emulated Tape                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*
|| ----------------------------------------------------------------------------
||
|| HETUPD.C     (C) Copyright Leland Lucius, 2000-2009
||              Released under terms of the Q Public License.
||
|| Copy/update Hercules Emulated Tapes while allowing various modifications
|| like enabling/disabling compression, changing compression method/level, and
|| internal chunk size.
||
|| ----------------------------------------------------------------------------
*/

#include "hstdinc.h"

#include "hercules.h"
#include "tapedev.h"
#include "hetlib.h"
#include "ftlib.h"
#include "sllib.h"
#include "herc_getopt.h"

#define UTILITY_NAME    "hetupd"
#define UTILITY_DESC    "HET Copy/Update"

/*-------------------------------------------------------------------*/
/* Maximum sized tape I/O buffer...                                  */
/*-------------------------------------------------------------------*/
static BYTE buf[ MAX_TAPE_BLKSIZE ];

/*
|| Local volatile data
*/
static int o_chunksize  = HETDFLT_CHKSIZE;
static int o_decompress = HETDFLT_DECOMPRESS;
static int o_compress   = HETDFLT_COMPRESS;
static int o_level      = HETDFLT_LEVEL;
static int o_method     = HETDFLT_METHOD;
static int o_verbose    = FALSE;
static char *o_sname    = NULL;
static char *o_dname    = NULL;
static int dorename     = FALSE;
static int i_faketape   = FALSE;
static int o_faketape   = FALSE;
static HETB *s_hetb     = NULL;
static FETB *s_fetb     = NULL;
static HETB *d_hetb     = NULL;
static FETB *d_fetb     = NULL;

/* Previous reported file position */
static off_t prevpos = 0;
/* Report progress every this many bytes */
#define PROGRESS_MASK (~0x3FFFF /* 256K */)

/*
|| Prints usage information
*/
static void
usage( char *name )
{
#if defined( HET_BZIP2 )
    char *bufbz = "                -b    use BZLIB compression\n";
#else
    char *bufbz = "";
#endif
    // "Usage: %s ...
    WRMSG( HHC02730, "I", name, bufbz );
}

/*
|| Supply "Yes" or "No"
*/
static const char *
yesno( int val )
{
    return( ( val ? "Yes" : "No" ) );
}

/*
|| Close tapes and cleanup
*/
static void
closetapes( int rc )
{

    if ( d_hetb != NULL ) het_close( &d_hetb );
    if ( s_hetb != NULL ) het_close( &s_hetb );

    if ( d_fetb != NULL ) fet_close( &d_fetb );
    if ( s_fetb != NULL ) fet_close( &s_fetb );

    if( dorename )
    {
        if( rc >= 0 )
        {
            rc = remove( o_sname );
            if ( rc < 0 )
            {
                // "File %s: Error removing file - manual intervention required"
                WRMSG( HHC02745, "I", o_sname );
            }
            else
            {
                rc = rename( o_dname, o_sname );
                if ( rc < 0 )
                    // "File %s: Error renaming file to %s - manual intervention required"
                    WRMSG( HHC02744, "I", o_dname, o_sname );
            }
        }
        else
        {
            rc = remove( o_dname );
            if ( rc < 0 )
                // "File %s: Error removing file - manual intervention required"
                WRMSG( HHC02745, "I", o_dname );
        }
    }

    return;
}

/*
|| Copy source to dest
*/
static int
copytape( void )
{
    int rc;

    while( TRUE )
    {
        if( extgui )
        {
            off_t curpos;
            /* Report progress every nnnK */
            if ( i_faketape )
                curpos = ftell( s_fetb->fh );
            else
                curpos = ftell( s_hetb->fh );
            if( ( curpos & PROGRESS_MASK ) != ( prevpos & PROGRESS_MASK ) )
            {
                prevpos = curpos;
                EXTGUIMSG( "IPOS=%"PRId64"\n", (U64)curpos );
            }
        }

        if ( i_faketape )
            rc = fet_read( s_fetb, buf );
        else
            rc = het_read( s_hetb, buf );
        if( rc == HETE_EOT )                // FAKETAPE and HETTAPE share codes
        {
            rc = 0;
            break;
        }

        if( rc == HETE_TAPEMARK )
        {
            if ( o_faketape )
                rc = fet_tapemark( d_fetb );
            else
                rc = het_tapemark( d_hetb );
            if( rc < 0 )
            {
                // "Error in function %s: %s"
                if ( o_faketape )
                    FWRMSG( stderr, HHC00075, "E", "fet_tapemark()", fet_error( rc ) );
                else
                    FWRMSG( stderr, HHC00075, "E", "het_tapemark()", het_error( rc ) );
                break;
            }
            continue;
        }

        if( rc < 0 )
        {
            // "Error in function %s: %s"
            if ( i_faketape )
                FWRMSG( stderr, HHC00075, "E", "fet_read()", fet_error( rc ) );
            else
                FWRMSG( stderr, HHC00075, "E", "het_read()", het_error( rc ) );
            break;
        }

        if ( o_faketape )
            rc = fet_write( d_fetb, buf, rc );
        else
            rc = het_write( d_hetb, buf, rc );
        if( rc < 0 )
        {
            // "Error in function %s: %s"
            if ( o_faketape )
                FWRMSG( stderr, HHC00075, "E", "fet_write()", fet_error( rc ) );
            else
                FWRMSG( stderr, HHC00075, "E", "het_write()", het_error( rc ) );
            break;
        }
    }

    return( rc );
}

/*
|| Open HET tapes and set options
*/
static int
opentapes( void )
{
    int rc;

    if ( ( rc = (int)strlen( o_sname ) ) > 4 &&
        ( rc = strcasecmp( &o_sname[rc-4], ".fkt" ) ) == 0 )
    {
        i_faketape = TRUE;
    }

    if ( i_faketape )
        rc = fet_open( &s_fetb, o_sname, FETOPEN_READONLY );
    else
        rc = het_open( &s_hetb, o_sname, HETOPEN_READONLY );
    if( rc < 0 )
    {
        dorename = FALSE;
        if ( o_verbose )
            // "File %s: Error opening: errno=%d: %s"
            FWRMSG( stderr, HHC02720, "E", o_sname, rc, het_error( rc ) );
        goto exit;
    }

    if ( ( rc = (int)strlen( o_dname ) ) > 4 &&
        ( rc = strcasecmp( &o_dname[rc-4], ".fkt" ) ) == 0 )
    {
        o_faketape = TRUE;
    }

    if ( o_faketape )
        rc = fet_open( &d_fetb, o_dname, FETOPEN_CREATE );
    else
        rc = het_open( &d_hetb, o_dname, HETOPEN_CREATE );
    if( rc < 0 )
    {
        dorename = FALSE;
        if ( o_verbose )
            // "File %s: Error opening: errno=%d: %s"
            FWRMSG( stderr, HHC02720, "E", o_dname, rc, het_error( rc ) );
        goto exit;
    }

    if ( !i_faketape )
    {
        if ( o_verbose )
            // "HET: Setting option %s to %s"
            FWRMSG( stderr, HHC02755, "I", "decompress", yesno( o_decompress ) );

        rc = het_cntl( s_hetb, HETCNTL_SET | HETCNTL_DECOMPRESS, o_decompress );
        if( rc < 0 )
        {
            if ( o_verbose )
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "het_cntl()", het_error( rc ) );
            goto exit;
        }
    }

    if ( !o_faketape )
    {
        if ( o_verbose )
            // "HET: Setting option %s to %s"
            WRMSG( HHC02755, "I", "compress", yesno( o_compress ) );

        rc = het_cntl( d_hetb, HETCNTL_SET | HETCNTL_COMPRESS, o_compress );
        if( rc < 0 )
        {
            if ( o_verbose )
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "het_cntl()", het_error( rc ) );
            goto exit;
        }

        if ( o_verbose )
        {
            char msgbuf[16];
            MSGBUF( msgbuf, "%d", o_method );
            // "HET: Setting option %s to %s"
            WRMSG( HHC02755, "I", "method", msgbuf );
        }

        rc = het_cntl( d_hetb, HETCNTL_SET | HETCNTL_METHOD, o_method );
        if( rc < 0 )
        {
            if ( o_verbose )
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "het_cntl()", het_error( rc ) );
            goto exit;
        }

        if ( o_verbose )
        {
            char msgbuf[16];
            MSGBUF( msgbuf, "%d", o_level );
            // "HET: Setting option %s to %s"
            WRMSG( HHC02755, "I", "level", msgbuf );
        }

        rc = het_cntl( d_hetb, HETCNTL_SET | HETCNTL_LEVEL, o_level );
        if( rc < 0 )
        {
            if ( o_verbose )
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "het_cntl()", het_error( rc ) );
            goto exit;
        }

        if ( o_verbose )
        {
            char msgbuf[16];
            MSGBUF( msgbuf, "%d", o_chunksize );
            // "HET: Setting option %s to %s"
            WRMSG( HHC02755, "I", "chunksize", msgbuf );
        }

        rc = het_cntl( d_hetb, HETCNTL_SET | HETCNTL_CHUNKSIZE, o_chunksize );
        if( rc < 0 )
        {
            if ( o_verbose )
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "het_cntl()", het_error( rc ) );
            goto exit;
        }
    }

    if( o_verbose )
    {
        char msgbuf[128];

        // HHC02757 = "HET: %s"

        MSGBUF( msgbuf, "Source             : %s", o_sname );
        WRMSG( HHC02757, "I", msgbuf );
        MSGBUF( msgbuf, "Destination        : %s", o_dname );
        WRMSG( HHC02757, "I", msgbuf );
        if ( !i_faketape )
        {
            MSGBUF( msgbuf, "Decompress source  : %s", yesno( het_cntl( s_hetb, HETCNTL_DECOMPRESS, 0 ) ) );
            WRMSG( HHC02757, "I", msgbuf );
        }
        if ( !o_faketape )
        {
            MSGBUF( msgbuf, "Compress dest      : %s", yesno( het_cntl( d_hetb, HETCNTL_COMPRESS, 0 ) ) );
            WRMSG( HHC02757, "I", msgbuf );
            MSGBUF( msgbuf, "Compression method : %d", het_cntl( d_hetb, HETCNTL_METHOD, 0 ) );
            WRMSG( HHC02757, "I", msgbuf );
            MSGBUF( msgbuf, "Compression level  : %d", het_cntl( d_hetb, HETCNTL_LEVEL, 0 ) );
            WRMSG( HHC02757, "I", msgbuf );
        }
    }

exit:

    return( rc );
}

/*
|| Standard main
*/
int
main( int argc, char *argv[] )
{
    char  *pgm;                    /* less any extension (.ext) */
    char   toname[ MAX_PATH ];
    int    rc;

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    while( TRUE )
    {
#if defined( HET_BZIP2 )
        rc = getopt( argc, argv, "bc:dhrsvz0123456789" );
#else
        rc = getopt( argc, argv, "c:dhrsvz0123456789" );
#endif /* defined( HET_BZIP2 ) */
        if( rc == -1 )
        {
            break;
        }

        switch( rc )
        {
            case '1': case '2': case '3': case '4': /* Compression level     */
            case '5': case '6': case '7': case '8':
            case '9':
                o_level = ( rc - '0' );
            break;

#if defined( HET_BZIP2 )
            case 'b':                               /* Use BZLIB compression */
                o_method = HETMETH_BZLIB;
                o_compress = TRUE;
                o_decompress = TRUE;
            break;
#endif /* defined( HET_BZIP2 ) */

            case 'c':                               /* Chunk size           */
                o_chunksize = atoi( optarg );
            break;

            case 'd':                               /* Decompress           */
                o_compress = FALSE;
                o_decompress = TRUE;
            break;

            case 'h':                               /* Print usage          */
                usage( pgm );
                return 1;

            case 'r':                               /* Rechunk              */
                o_compress = FALSE;
                o_decompress = FALSE;
            break;

            case 's':                               /* Strict HET spec      */
                o_chunksize = 4096;
                o_compress = FALSE;
                o_decompress = TRUE;
            break;

            case 'v':                               /* Be chatty            */
                o_verbose = TRUE;
            break;

            case 'z':                               /* Use ZLIB compression */
                o_method = HETMETH_ZLIB;
                o_compress = TRUE;
                o_decompress = TRUE;
            break;

            default:                                /* Print usage          */
                usage( pgm );
                return 1;
        }
    }

    argc -= optind;

    switch( argc )
    {
        case 1:
            sprintf( toname, "%s.%010d", argv[ optind ], rand() );
            o_dname = toname;
            dorename = TRUE;
        break;

        case 2:
            o_dname = argv[ optind + 1 ];
        break;

        default:
            usage( pgm );
            return 1;
    }
    o_sname = argv[ optind ] ;

    rc = opentapes();
    if( rc < 0 )
    {
        // "HET: HETLIB reported error %s files; %s"
        FWRMSG( stderr, HHC02756, "E", "opening", het_error( rc ) );
    }
    else
    {
        rc = copytape();
        if( rc < 0 )
        {
            // "HET: HETLIB reported error %s files; %s"
            FWRMSG( stderr, HHC02756, "E", "copying", het_error( rc ) );
        }
    }

    closetapes( rc );

    return 0;
}
