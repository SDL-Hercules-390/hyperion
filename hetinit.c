/* HETINIT.C    (C) Copyright Leland Lucius, 2000-2012               */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*           Creates IEHINITT or NL format Hercules Emulated Tapes   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*
|| ----------------------------------------------------------------------------
||
|| HETINIT.C    (C) Copyright Leland Lucius, 2000-2009
||              Released under terms of the Q Public License.
||
|| Creates IEHINITT or NL format Hercules Emulated Tapes.
||
|| ----------------------------------------------------------------------------
*/

#include "hstdinc.h"

#include "hercules.h"
#include "hetlib.h"
#include "ftlib.h"
#include "sllib.h"
#include "herc_getopt.h"

#define UTILITY_NAME    "hetinit"
#define UTILITY_DESC    "HET IEHINITT"

/*
|| Prints usage information
*/
static void
usage( char *pgm )
{
    // "Usage: %s ...
    WRMSG( HHC02729, "I", pgm );
}

/*
|| Subroutine to convert a null-terminated string to upper case
*/
void het_string_to_upper (char *source)
{
int i;

    for (i = 0; source[i] != '\0'; i++)
        source[i] = toupper((unsigned char)source[i]);
}

/*
|| Standard main() function
*/
int
main( int argc, char *argv[] )
{
    char           *pgm;                    /* less any extension (.ext) */
    int             rc;
    SLLABEL         lab;
    HETB           *hetb;                   /* used for aws and het tapes*/
    FETB           *fetb;                   /* used for faketapes        */
    int             o_iehinitt;
    int             o_nl;
    int             o_compress;
    int             o_faketape;
    char           *o_filename;
    char           *o_owner;
    char           *o_volser;

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    hetb = NULL;
    fetb = NULL;
    o_filename = NULL;
    o_faketape = FALSE;
    o_iehinitt = TRUE;
    o_nl = FALSE;
    o_compress = TRUE;
    o_owner = NULL;
    o_volser = NULL;

    while( TRUE )
    {
        rc = getopt( argc, argv, "dhin" );
        if( rc == -1 )
        {
            break;
        }

        switch( rc )
        {
            case 'd':
                o_compress = FALSE;
            break;

            case 'h':
                usage( pgm );
                goto exit;

            case 'i':
                o_iehinitt = TRUE;
                o_nl = FALSE;
            break;

            case 'n':
                o_iehinitt = FALSE;
                o_nl = TRUE;
            break;

            default:
                usage( pgm );
                goto exit;
        }
    }

    argc -= optind;

    if( argc < 1 )
    {
        usage( pgm );
        goto exit;
    }

    o_filename = argv[ optind ];

    if ( ( rc = (int)strlen( o_filename ) ) > 4 && ( rc = strcasecmp( &o_filename[rc-4], ".fkt" ) ) == 0 )
    {
        o_faketape = TRUE;
    }

    if( o_iehinitt )
    {
        if( argc == 2 )
        {
            o_volser = argv[ optind + 1 ];
        }
        else if( argc == 3 )
        {
            o_volser = argv[ optind + 1 ];
            o_owner = argv[ optind + 2 ];
        }
        else
        {
            usage( pgm );
            goto exit;
        }
    }

    if( o_nl )
    {
        if( argc != 1 )
        {
            usage( pgm );
            goto exit;
        }
    }

    if( o_volser )
        het_string_to_upper( o_volser );

    if( o_owner )
        het_string_to_upper( o_owner );

    if ( o_faketape )
    {
        rc = fet_open( &fetb, o_filename, FETOPEN_CREATE );
        if ( rc < 0 )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "fet_open()", fet_error( rc ) );
            goto exit;
        }

    }
    else
    {
        rc = het_open( &hetb, o_filename, HETOPEN_CREATE );
        if( rc < 0 )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "het_open()", het_error( rc ) );
            goto exit;
        }

        rc = het_cntl( hetb, HETCNTL_SET | HETCNTL_COMPRESS, o_compress );
        if( rc < 0 )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "het_cntl()", het_error( rc ) );
            goto exit;
        }
    }

    if( o_iehinitt )
    {
        rc = sl_vol1( &lab, o_volser, o_owner );
        if( rc < 0 )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "sl_vol1()", sl_error( rc ) );
            goto exit;
        }

        if ( o_faketape )
            rc = fet_write( fetb, &lab, (U16)sizeof( lab ) );
        else
            rc = het_write( hetb, &lab, sizeof( lab ) );
        if( rc < 0 )
        {
            if ( o_faketape )
                FWRMSG( stderr, HHC00075, "E", "fet_write() for VOL1", fet_error( rc ) );
            else
                FWRMSG( stderr, HHC00075, "E", "het_write() for VOL1", het_error( rc ) );
            goto exit;
        }

        rc = sl_hdr1( &lab, SL_INITDSN, NULL, 0, 0, NULL, 0 );
        if( rc < 0 )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "sl_hdr1()", sl_error( rc ) );
            goto exit;
        }

        if ( o_faketape )
            rc = fet_write( fetb, &lab, (U16)sizeof(lab) );
        else
            rc = het_write( hetb, &lab, sizeof( lab ) );
        if( rc < 0 )
        {
            if ( o_faketape )
                FWRMSG( stderr, HHC00075, "E", "fet_write() for HDR1", fet_error( rc ) );
            else
                FWRMSG( stderr, HHC00075, "E", "het_write() for HDR1", het_error( rc ) );
            goto exit;
        }

    }
    else if( o_nl )
    {
        if ( o_faketape )
            rc = fet_tapemark( fetb );
        else
            rc = het_tapemark( hetb );
        if( rc < 0 )
        {
            if ( o_faketape )
                FWRMSG( stderr, HHC00075, "E", "fet_tapemark()", fet_error( rc ) );
            else
                FWRMSG( stderr, HHC00075, "E", "het_tapemark()", het_error( rc ) );
            goto exit;
        }
    }

    if ( o_faketape )
        rc = fet_tapemark( fetb );
    else
        rc = het_tapemark( hetb );
    if( rc < 0 )
    {
        if ( o_faketape )
            FWRMSG( stderr, HHC00075, "E", "fet_tapemark()", fet_error( rc ) );
        else
            FWRMSG( stderr, HHC00075, "E", "het_tapemark()", het_error( rc ) );
        goto exit;
    }

exit:
    if ( o_faketape)
        fet_close( &fetb );
    else
        het_close( &hetb );

    return( rc < 0 );
}
