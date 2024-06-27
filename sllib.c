/* SLLIB.C      (C) Copyright Leland Lucius, 2000-2012               */
/*             Library for managing Standard Label tapes.            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _SLLIB_C_
#define _HTAPE_DLL_

#include "hercules.h"
#include "sllib.h"

/*
|| Local constant data
*/

/*
|| Label IDs in EBCDIC
*/
static const char *sl_elabs[] =
{
    "\x00\x00\x00", /* Placeholder              */
    "\xE5\xD6\xD3", /* EBCDIC characters "VOL"  */
    "\xC8\xC4\xD9", /* EBCDIC characters "HDR"  */
    "\xE4\xC8\xD3", /* EBCDIC characters "UHL"  */
    "\xC5\xD6\xC6", /* EBCDIC characters "EOF"  */
    "\xC5\xD6\xE5", /* EBCDIC characters "EOV"  */
    "\xE4\xE3\xD3", /* EBCDIC characters "UTL"  */
};
#define SL_ELABS_MAX ( sizeof( sl_elabs ) / sizeof( sl_elabs[ 0 ] ) )

/*
|| Label IDs in ASCII
*/
static const char *sl_alabs[] =
{
    "\x00\x00\x00", /* Placeholder              */
    "\x56\x4f\x4c", /* ASCII characters "VOL"   */
    "\x48\x44\x52", /* ASCII characters "HDR"   */
    "\x55\x48\x4c", /* ASCII characters "UHL"   */
    "\x45\x4f\x46", /* ASCII characters "EOF"   */
    "\x45\x4f\x56", /* ASCII characters "EOV"   */
    "\x55\x54\x4c", /* ASCII characters "UTL"   */
};
#define SL_ALABS_MAX ( sizeof( sl_alabs ) / sizeof( sl_alabs[ 0 ] ) )

/*
|| Minimum and maximum ranges for each label type
*/
static const struct
{
    int min;
    int max;
}
sl_ranges[] =
{
    { 0, 0 },       /* Placeholder              */
    { 1, 1 },       /* ASCII characters "VOL"   */
    { 1, 2 },       /* ASCII characters "HDR"   */
    { 1, 8 },       /* ASCII characters "UHL"   */
    { 1, 2 },       /* ASCII characters "EOF"   */
    { 1, 2 },       /* ASCII characters "EOV"   */
    { 1, 8 },       /* ASCII characters "UTL"   */
};

/*
|| Text descriptions for errors
*/
static const char *sl_errstr[] =
{
    "No error",
    "Block size out of range",
    "Data set sequence out of range",
    "Invalid expiration date",
    "Missing or invalid job name",
    "Missing or invalid record length",
    "Owner string invalid or too long",
    "Missing or invalid record format",
    "Missing or invalid step name",
    "Invalid recording technique",
    "Volume sequence out of range",
    "Missing or invalid volume serial",
    "User data too long",
    "Label type invalid",
    "Label number invalid",
    "Invalid error code",
};
#define SL_ERRSTR_MAX ( sizeof( sl_errstr) / sizeof( sl_errstr[ 0 ] ) )

/*
|| Valid characters for a Standard Label
|| (from: SC26-4565-01 "3.4 Label Definition and Organization")
*/
static const char
sl_cset[] =
{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !\"%&'()*+,-./:;<=>?_"
};

/*
|| Valid record formats
*/
static const struct
{
    char *recfm;
    char f;
    char b;
    char c;
}
valfm[] =
{
    { "U",    'U', ' ', ' ' },
    { "UA",   'U', ' ', 'A' },
    { "UM",   'U', ' ', 'M' },
    { "F",    'F', ' ', ' ' },
    { "FA",   'F', ' ', 'A' },
    { "FM",   'F', ' ', 'M' },
    { "FB",   'F', 'B', ' ' },
    { "FBA",  'F', 'B', 'A' },
    { "FBM",  'F', 'B', 'M' },
    { "FS",   'F', 'S', ' ' },
    { "FSA",  'F', 'S', 'A' },
    { "FSM",  'F', 'S', 'M' },
    { "FBS",  'F', 'R', ' ' },
    { "FBSA", 'F', 'R', 'A' },
    { "FBSM", 'F', 'R', 'M' },
    { "V",    'V', ' ', ' ' },
    { "VA",   'V', ' ', 'A' },
    { "VM",   'V', ' ', 'M' },
    { "VB",   'V', 'B', ' ' },
    { "VBA",  'V', 'B', 'A' },
    { "VBM",  'V', 'B', 'M' },
    { "VS",   'V', 'S', ' ' },
    { "VSA",  'V', 'S', 'A' },
    { "VSM",  'V', 'S', 'M' },
    { "VBS",  'V', 'R', ' ' },
    { "VBSA", 'V', 'R', 'A' },
    { "VBSM", 'V', 'R', 'M' },
};
#define VALFMCNT ( sizeof( valfm ) / sizeof( valfm[ 0 ] ) )

/*
|| EBCDIC -> ASCII translation table
*/
static const unsigned char
sl_etoatab[] =
{
    "\x00\x01\x02\x03\xA6\x09\xA7\x7F\xA9\xB0\xB1\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\xB2\xB4\x08\xB7\x18\x19\x1A\xB8\xBA\x1D\xBB\x1F"
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

/*
|| ASCII -> EBCDIC translation table
*/
static const unsigned char
sl_atoetab[] =
{
    "\x00\x01\x02\x03\x37\x2D\x2E\x2F\x16\x05\x25\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\x3C\x3D\x32\x26\x18\x19\x1A\x27\x22\x1D\x35\x1F"
    "\x40\x5A\x7F\x7B\x5B\x6C\x50\x7D\x4D\x5D\x5C\x4E\x6B\x60\x4B\x61"
    "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\x7A\x5E\x4C\x7E\x6E\x6F"
    "\x7C\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xD1\xD2\xD3\xD4\xD5\xD6"
    "\xD7\xD8\xD9\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xAD\xE0\xBD\x5F\x6D"
    "\x79\x81\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96"
    "\x97\x98\x99\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xC0\x6A\xD0\xA1\x07"
    "\x68\xDC\x51\x42\x43\x44\x47\x48\x52\x53\x54\x57\x56\x58\x63\x67"
    "\x71\x9C\x9E\xCB\xCC\xCD\xDB\xDD\xDF\xEC\xFC\xB0\xB1\xB2\xB3\xB4"
    "\x45\x55\xCE\xDE\x49\x69\x04\x06\xAB\x08\xBA\xB8\xB7\xAA\x8A\x8B"
    "\x09\x0A\x14\xBB\x15\xB5\xB6\x17\x1B\xB9\x1C\x1E\xBC\x20\xBE\xBF"
    "\x21\x23\x24\x28\x29\x2A\x2B\x2C\x30\x31\xCA\x33\x34\x36\x38\xCF"
    "\x39\x3A\x3B\x3E\x41\x46\x4A\x4F\x59\x62\xDA\x64\x65\x66\x70\x72"
    "\x73\xE1\x74\x75\x76\x77\x78\x80\x8C\x8D\x8E\xEB\x8F\xED\xEE\xEF"
    "\x90\x9A\x9B\x9D\x9F\xA0\xAC\xAE\xAF\xFD\xFE\xFB\x3F\xEA\xFA\xFF"
};

/*==DOC==

    NAME
            sl_atoe - Translate input buffer from ASCII to EBCDIC

    SYNOPSIS
            #include "sllib.h"

            char *sl_atoe( void *dbuf, void *sbuf, int slen )

    DESCRIPTION
            Translates, and optionally copies, "sbuf" from ASCII to
            EBCDIC for "slen" characters.

            If "dbuf" is specified as NULL, then "sbuf" is translated in
            place.  Otherwise, "dbuf" specifies the buffer where the
            translated characters will be stored.

    RETURN VALUE
            The return value will be either "sbuf" or "dbuf" depending on
            whether "dbuf" was passed as NULL.

    EXAMPLE
            //
            // Convert buffer
            //

            #include "sllib.h"

            unsigned char ascii[] =
                "\x48\x65\x72\x63\x75\x6c\x65\x73\x2e\x2e\x2e"
                "\x20\x52\x65\x73\x75\x72\x72\x65\x63\x74\x69"
                "\x6e\x67\x20\x74\x68\x65\x20\x64\x69\x6e\x6f"
                "\x73\x61\x75\x72\x73\x21";
            unsigned char ebcdic[ sizeof( ascii ) ];

            int main( int argc, char *argv[] )
            {
                int len;
                int i;

                len = strlen( ascii );

                sl_atoe( ebcdic, ascii, len );

                printf( "ascii string:  " );

                for( i = 0 ; i < len ; i++ )
                {
                    printf( "%02x ", ascii[ i ] );
                }

                printf( "\nebcdic string: " );

                for( i = 0 ; i < len ; i++ )
                {
                    printf( "%02x ", ebcdic[ i ] );
                }

                printf( "\n" );

                return( 0 );
            }

    SEE ALSO

==DOC==*/

DLL_EXPORT
char *
sl_atoe( void *dbuf, void *sbuf, int slen )
{
    unsigned char *sptr;
    unsigned char *dptr;

    sptr = sbuf;
    dptr = dbuf;

    if( dptr == NULL )
    {
        dptr = sptr;
    }

    while( slen > 0 )
    {
        slen--;
        dptr[ slen ] = sl_atoetab[ sptr[ slen ] ];
    }

    return( (char *)dptr );
}

/*==DOC==

    NAME
            sl_etoa - Translate input buffer from EBCDIC to ASCII

    SYNOPSIS
            #include "sllib.h"

            char *sl_etoa( void *dbuf, void *sbuf, int slen )

    DESCRIPTION
            Translates, and optionally copies, "sbuf" from EBCDIC to
            ASCII for "slen" characters.

            If "dbuf" is specified as NULL, then "sbuf" is translated in
            place.  Otherwise, "dbuf" specifies the buffer where the
            translated characters will be stored.

    RETURN VALUE
            The return value will be either "sbuf" or "dbuf" depending on
            whether "dbuf" was passed as NULL.

    EXAMPLE
            //
            // Convert buffer
            //

            #include "sllib.h"

            unsigned char ebcdic[] =
                "\xc8\x85\x99\x83\xa4\x93\x85\xa2\x4b\x4b\x4b"
                "\x40\xd9\x85\xa2\xa4\x99\x99\x85\x83\xa3\x89"
                "\x95\x87\x40\xa3\x88\x85\x40\x84\x89\x95\x96"
                "\xa2\x81\xa4\x99\xa2\x5a";
            unsigned char ascii[ sizeof( ebcdic ) ];

            int main( int argc, char *argv[] )
            {
                int len;
                int i;

                len = strlen( ebcdic );

                sl_etoa( ascii, ebcdic, len );

                printf( "ebcdic string:  " );

                for( i = 0 ; i < len ; i++ )
                {
                    printf( "%02x ", ebcdic[ i ] );
                }

                printf( "\nascii string: " );

                for( i = 0 ; i < len ; i++ )
                {
                    printf( "%02x ", ascii[ i ] );
                }

                printf( "\n" );

                return( 0 );
            }

    SEE ALSO

==DOC==*/

DLL_EXPORT
char *
sl_etoa( void *dbuf, void *sbuf, int slen )
{
    BYTE *sptr;
    BYTE *dptr;

    sptr = sbuf;
    dptr = dbuf;

    if( dptr == NULL )
    {
        dptr = sptr;
    }

    while ( slen > 0 )
    {
        slen--;
        dptr[ slen ] = sl_etoatab[ sptr[ slen ] ];
    }

    return( (char *)dptr );
}

/*==DOC==

    NAME
            sl_islabel - Determines if passed data represents a standard label

    SYNOPSIS
            #include "sllib.h"

            int sl_islabel( SLLABEL *dlab, void *buf, int len )

    DESCRIPTION
            This function performs several tests to determine if the "buf"
            parameter points to a valid standard label.  The "len" parameter
            must be the length of the data pointed to by the "buf" parameter.
            If "dlab" does not contain NULL, then the data pointed to by "buf"
            will be converted to ASCII and placed at the "dlab" location.

    RETURN VALUE
            TRUE is returned if the data appears to be a standard label.
            Otherwise, FALSE is returned.

    NOTES
            The input label may be in either ASCII or EBCDIC.

            Currently, the tests are quite trival, but they may become more
            strict.

    EXAMPLE
            //
            // Test validity of a label
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                SLLABEL sllab = { 0 };

                printf( "Label is%s valid\n",
                    ( sl_islabel( NULL, &sllab, sizeof( sllab ) ? "" : " not" ) );

                sl_vol1( &sllab, "HET001", "HERCULES" );

                printf( "Label is: %s valid\n",
                    ( sl_islabel( NULL, &sllab, sizeof( sllab ) ? "" : " not" ) );

                return( 0 );
            }

    SEE ALSO
        sl_vol1()

==DOC==*/

DLL_EXPORT
int
sl_islabel( SLLABEL *lab, void *buf, int len )
{
    int i;
    int num;
    unsigned char *ptr;

    if( len != sizeof( SLLABEL ) )
    {
        return FALSE;
    }

    for( i = 1 ; i < (int)SL_ELABS_MAX ; i++ )
    {
        if( memcmp( sl_elabs[ i ], buf, 3 ) == 0 )
        {
            ptr = buf;
            num = ptr[ 3 ] - (unsigned char) '\xF0';
            if( ( num >= sl_ranges[ i ].min ) && ( num <= sl_ranges[ i ].max ) )
            {
                if( lab != NULL )
                {
                    sl_etoa( lab, buf, len );
                }
                return( TRUE );
            }
        }

        if( memcmp( sl_alabs[ i ], buf, 3 ) == 0 )
        {
            ptr = buf;
            num = ptr[ 3 ] - (unsigned char) '\x30';
            if( ( num >= sl_ranges[ i ].min ) && ( num <= sl_ranges[ i ].max ) )
            {
                if( lab != NULL )
                {
                    memcpy( lab, buf, len );
                }

                return( TRUE );
            }
        }
    }

    return( FALSE );
}

/*==DOC==

    NAME
            sl_istype - Verifies data is of specified standard label type

    SYNOPSIS
            #include "sllib.h"

            int sl_istype( void *buf, int type, int num )

    DESCRIPTION
            This function verifies that the data pointed to by the "buf"
            parameter contains a standard label as determined by the "type"
            and "num" parameters.

            The "type" parameter can be one of the "SLT_*" defines found in
            the "sllib.h" header file.

            The "num" parameter further defines the type and is usually 1
            or 2.  However, 0 may be specified to only test using the "type"
            parameter.

    RETURN VALUE
            TRUE is returned if the data is of the given type.  Otherwise,
            FALSE is returned.

    NOTES
            The input data may be in either ASCII or EBCDIC.

            This routine is "usually" not called directly by user programs.
            The "sl_is*" macros in "sllib.h" should be used instead.

    EXAMPLE
            //
            // Determine if data is a VOL1 or HDR2 label.
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                SLLABEL sllab = { 0 };

                sl_vol1( &sllab, "HET001", "HERCULES" );

                printf( "Label is%s a VOL1\n",
                    ( sl_istype( &sllab, SLT_VOL, 1 ) ? "" : " not" ) );

                printf( "Label is%s a HDR2\n",
                    ( sl_istype( &sllab, SLT_HDR, 2 ) ? "" : " not" ) );

                return( 0 );
            }

    SEE ALSO
        sl_vol1()

==DOC==*/

DLL_EXPORT
int
sl_istype( void *buf, int type, int num )
{
    unsigned char *ptr;

    ptr = buf;

    /*
    || Check EBCDIC table
    */
    if( memcmp( buf, sl_elabs[ type ], 3 ) == 0 )
    {
        if( ( num == 0 ) || ( ptr[ 3 ] == ( ( (unsigned char) '\xF0' ) + num ) ) )
        {
            return( TRUE );
        }
    }

    /*
    || Check ASCII table
    */
    if( memcmp( buf, sl_alabs[ type ], 3 ) == 0 )
    {
        if( ( num == 0 ) || ( ptr[ 3 ] == ( ( (unsigned char) '\x30') + num ) ) )
        {
            return( TRUE );
        }
    }

    return( FALSE );
}

/*==DOC==

    NAME
            sl_fmtdate - Converts dates to/from SL format

    SYNOPSIS
            #include "sllib.h"

            char *sl_fmtdate( char *dest, char *src, int fromto )

    DESCRIPTION
            Converts the "src" date from or to the SL format and places the
            result at the "dest" location.  If the "src" parameter is specified
            as NULL, then the current date will automatically be supplied.

            The "fromto" parameter controls the type of conversion.  Specify
            FALSE to convert to SL format and TRUE from convert from SL format.

            When converting to the SL format, the "src" parameter must contain
            a valid Julian date in one of the following formats:
                YYDDD
                YY.DDD
                YYYYDDD
                YYYY.DDD

    RETURN VALUE
            If "src" contains an invalid date, then NULL will be returned.
            Otherwise, the "dest" value is returned.

    EXAMPLE
            //
            // Convert julian date to SL format
            //

            #include "sllib.h"

            char sldate[ SL_DATELEN ];
            char jdate[] = "1998.212";

            int main( int argc, char *argv[] )
            {

                sl_fmtdate( sldate, jdate, FALSE );

                printf( "Julian date : %s\n", jdate );
                printf( "SL date     : %-6.6s\n", sldate );

                return( 0 );
            }

    SEE ALSO

==DOC==*/

DLL_EXPORT
char *
sl_fmtdate( char *dest, char *src, int fromto )
{
    char wbuf[ 9 ];
    char sbuf[ 9 ];
    char *ptr;
    time_t curtime;
    struct tm tm;
    int ret;

    /*
    || If source represents an SL date, then convert it to julian
    */
    if( fromto )
    {
        if( src == NULL )
        {
            return( NULL );
        }

        if( src[ 5 ] == '0' )
        {
            dest[ 0 ] = src[ 1 ];
            dest[ 1 ] = src[ 2 ];
        }
        else if( src[ 0 ] == ' ' )
        {
            dest[ 0 ] = '1';
            dest[ 1 ] = '9';
        }
        else
        {
            dest[ 0 ] = '2';
            dest[ 1 ] = src[ 0 ];
        }

        memcpy( &dest[ 2 ], &src[ 1 ] , 2 );
        dest[ 4 ] = '.';
        memcpy( &dest[ 5 ], &src[ 3 ] , 3 );
    }
    else
    {
        /*
        || Supply current date if source is null
        */
        if( src == NULL )
        {
            time( &curtime );
            strftime( sbuf, sizeof( sbuf ), "%Y%j", localtime( &curtime ) );
            src = sbuf;
        }

        /*
        || Base initial guess at format on length of src date
        */
        switch( strlen( src ) )
        {
            case 5:
                ptr = "%2u%3u";
            break;

            case 6:
                ptr = "%2u.%3u";
            break;

            case 7:
                ptr = "%4u%3u";
            break;

            case 8:
                ptr = "%4u.%3u";
            break;

            default:
                return( NULL );
        }

        /*
        || Convert src to "tm" format
        */
        ret = sscanf( src, ptr, &tm.tm_year, &tm.tm_yday );
        if( ret != 2 || tm.tm_yday < 1 || tm.tm_yday > 366 )
        {
            return( NULL );
        }
        tm.tm_yday--;

        /*
        || Now, convert to SL tape format
        */
        strftime( wbuf, sizeof( wbuf ), "%Y%j", &tm );
        if( tm.tm_year < 100 )
        {
            /*
            || 1900s are indicated by a blank.
            */
            wbuf[ 1 ] = ' ';
        }

        /*
        || Finally, copy SL date to destination
        */
        memcpy( dest, &wbuf[ 1 ], 6 );
    }

    /*
    || Return dest pointer
    */
    return( dest );
}

/*==DOC==

    NAME
            sl_fmtlab - Transforms an SL label from raw to cooked format

    SYNOPSIS
            #include "sllib.h"

            void sl_fmtlab( SLFMT *fmt, SLLABEL *lab )

    DESCRIPTION
            Converts the SL label specified by "lab" into a "cooked" format
            that's easier to process.  Text descriptions are supplied for
            each field are also supplied.

    RETURN VALUE
            Nothing is returned.

    NOTES
            The input label may be in either ASCII or EBCDIC.  It will be
            converted to ASCII before building the cooked version.

            The first two fields of the SLFMT structure are arrays that contain
            the text description and value for each field based on the label
            type.  The arrays are terminated with NULL pointers.

    EXAMPLE
            //
            // Convert an SL label to cooked format
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                SLFMT slfmt;
                SLLABEL sllab;
                int i;

                sl_vol1( &sllab, "HET001", "HERCULES" );

                sl_fmtlab( &slfmt, &sllab );

                for( i = 0 ; slfmt.key[ i ] != NULL ; i++ )
                {
                    printf("%-20.20s: '%s'\n", slfmt.key[ i ] , slfmt.val[ i ] );
                }

                return( 0 );
            }

    SEE ALSO
        sl_vol1()

==DOC==*/

#define lab2fmt( i1, f2, l3, k4 ) \
            fmt->key[ i1 ] = k4; \
            fmt->val[ i1 ] = fmt->f2; \
            memcpy( fmt->f2, lab->f2, l3 );
DLL_EXPORT
void
sl_fmtlab( SLFMT *fmt, SLLABEL *lab )
{
    SLLABEL templab;

    /*
    || Initialize
    */
    memset( fmt, 0, sizeof( SLFMT ) );

    /*
    || If label appears to be EBCDIC, convert to ASCII before processing
    */
    if( sl_islabel( &templab, lab, sizeof( SLLABEL ) ) == FALSE )
    {
        return;
    }
    lab = &templab;

    /*
    || Store label type (combine ID and NUM)
    */
    fmt->key[ 0 ] = "Label";
    fmt->val[ 0 ] = fmt->type;
    memcpy( fmt->type, lab->id, 4 );

    /*
    || Build remaining fields based on label type
    */
    if( memcmp( lab->id, "VOL", 3 ) == 0 )
    {
        if( lab->num[ 0 ] == '1' )
        {
            lab2fmt(  1, slvol.volser,      6,  "Volume Serial"         );
            lab2fmt(  2, slvol.access,      1,  "Volume Access"         );
            lab2fmt(  3, slvol.owner,       10, "Owner Code"            );
        }
    }
    else if( ( memcmp( lab->id, "HDR", 3 ) == 0 ) ||
             ( memcmp( lab->id, "EOF", 3 ) == 0 ) ||
             ( memcmp( lab->id, "EOV", 3 ) == 0 ) )
    {
        if( lab->num[ 0 ] == '1' )
        {
            lab2fmt( 1,  slds1.dsid,        17, "Dataset ID"            );
            lab2fmt( 2,  slds1.volser,      6,  "Volume Serial"         );
            lab2fmt( 3,  slds1.volseq,      4,  "Volume Sequence"       );
            lab2fmt( 4,  slds1.dsseq,       4,  "Dataset Sequence"      );
            lab2fmt( 5,  slds1.genno,       4,  "GDG Number"            );
            lab2fmt( 6,  slds1.verno,       2,  "GDG Version"           );
            lab2fmt( 7,  slds1.crtdt,       6,  "Creation Date"         );
            lab2fmt( 8,  slds1.expdt,       6,  "Expiration Date"       );
            lab2fmt( 9,  slds1.dssec,       1,  "Dataset Security"      );
            lab2fmt( 10, slds1.blklo,       6,  "Block Count Low"       );
            lab2fmt( 11, slds1.syscd,       13, "System Code"           );
            lab2fmt( 12, slds1.blkhi,       4,  "Block Count High"      );
        }
        else if( lab->num[ 0 ] == '2' )
        {
            lab2fmt( 1,  slds2.recfm,       1,  "Record Format"         );
            lab2fmt( 2,  slds2.blksize,     5,  "Block Size"            );
            lab2fmt( 3,  slds2.lrecl,       5,  "Record Length"         );
            lab2fmt( 4,  slds2.den,         1,  "Density"               );
            lab2fmt( 5,  slds2.dspos,       1,  "Dataset Position"      );
            lab2fmt( 6,  slds2.jobid,       17, "Job/Step ID"           );
            lab2fmt( 7,  slds2.trtch,       2,  "Recording Technique"   );
            lab2fmt( 8,  slds2.ctrl,        1,  "Control Character"     );
            lab2fmt( 9,  slds2.blkattr,     1,  "Block Attribute"       );
            lab2fmt( 10, slds2.devser,      6,  "Device Serial"         );
            lab2fmt( 11, slds2.ckptid,      1,  "Checkpoint ID"         );
            lab2fmt( 12, slds2.lblkln,      10, "Large Block Length"    );
        }
    }
    else if( memcmp( lab->id, "USR", 3 ) == 0 )
    {
        lab2fmt(  1, slusr.data,            76, "User Data"             );
    }

    return;
}
#undef lab2fmt

/*==DOC==

    NAME
            sl_vol - Generate a volume label

    SYNOPSIS
            #include "sllib.h"

            int sl_vol( SLLABEL *lab,
                        char *volser,
                        char *owner )

    DESCRIPTION
            This function builds a volume label based on the parameters
            provided and places it at the location pointed to by the "lab"
            parameter in EBCDIC.

            The remaining parameters correspond to fields within the label
            and are converted to EBCDIC before storing.

            The "owner" parameter may be specified as NULL, in which case
            blanks are supplied.

    RETURN VALUE
            The return value will be >= 0 if no errors are detected.

            If an error is detected, then the return value will be < 0 and
            will be one of the following:

            SLE_VOLSER          Missing or invalid volume serial
            SLE_OWNER           Owner string too long

    NOTES
            This routine is normally accessed using the supplied "sl_vol1"
            macro.

            Only the "most common" label fields have corresponding parameters
            so the user must supply any other desired values.

    EXAMPLE
            //
            // Create a VOL1 label
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                SLFMT slfmt;
                SLLABEL sllab;
                int i;

                sl_vol( &sllab, "HET001", "HERCULES" );

                sl_fmtlab( &slfmt, &sllab );

                for( i = 0 ; slfmt.key[ i ] != NULL ; i++ )
                {
                    printf("%-20.20s: '%s'\n", slfmt.key[ i ] , slfmt.val[ i ] );
                }

                return( 0 );
            }

    SEE ALSO
        sl_fmtlab()

==DOC==*/

DLL_EXPORT
int
sl_vol( SLLABEL *lab,
        char *volser,
        char *owner )
{
    size_t len;

    /*
    || Initialize
    */
    memset( lab, ' ', sizeof( SLLABEL ) );

    /*
    || Label ID
    */
    memcpy( lab->id, sl_alabs[ SLT_VOL ], 3 );

    /*
    || Label number
    */
    lab->num[ 0 ] = '1';

    /*
    || Volser
    */
    if( volser == NULL )
    {
        return( SLE_VOLSER );
    }

    len = strlen( volser );
    if( ( len > 6 ) || ( strspn( volser, sl_cset ) != len ) )
    {
        return( SLE_VOLSER );
    }

    memcpy( lab->slvol.volser, volser, len );

    /*
    || Owner
    */
    if( owner != NULL )
    {
        len = strlen( owner );
        if( len > 10 )
        {
            return( SLE_OWNER );
        }
        memcpy( lab->slvol.owner, owner, len );
    }

    /*
    || Convert to EBCDIC
    */
    sl_atoe( NULL, lab, sizeof( SLLABEL ) );

    return 0;
}

/*==DOC==

    NAME
            sl_ds1 - Generate a data set label 1

    SYNOPSIS
            #include "sllib.h"

            int sl_ds1( SLLABEL *lab,
                        int type,
                        char *dsn,
                        char *volser,
                        int volseq,
                        int dsseq,
                        char *expdt,
                        int blocks )

    DESCRIPTION
            This function builds a data set label 1 based on the parameters
            provided and places it at the location pointed to by the "lab"
            parameter in EBCDIC.

            The "type" parameter must be "SLT_HDR", "SLT_EOF", or "SLT_EOV".

            The remaining parameters correspond to fields within the label
            and are converted to EBCDIC before storing.

            The "dsn" parameter may be set to "SL_INITDSN" if "SLT_HDR" is
            specified for the "type" parameter.  This will create an IEHINITT
            format HDR1 label.

            The "blocks" parameter is forced to 0 for "SLT_HDR" types.

    RETURN VALUE
            The return value will be >= 0 if no errors are detected.

            If an error is detected, then the return value will be < 0 and
            will be one of the following:

            SLE_INVALIDTYPE     Invalid label type specified
            SLE_VOLSER          Missing or invalid volume serial
            SLE_OWNER           Owner string too long

    NOTES
            This routine is normally accessed using the supplied "sl_hdr1",
            "sl_eof1", or "sl_eov1" macros.

            Only the "most common" label fields have corresponding parameters
            so the user must supply any other desired values.

    EXAMPLE
            //
            // Create a EOF1 label
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                SLFMT slfmt;
                SLLABEL sllab;
                int i;

                sl_ds1( &sllab,
                        SLT_EOF,
                        "HERCULES.TAPE.G0010V00",
                        "HERC01",
                        1,
                        1,
                        "2001.321",
                        289 );

                sl_fmtlab( &slfmt, &sllab );

                for( i = 0 ; slfmt.key[ i ] != NULL ; i++ )
                {
                    printf("%-20.20s: '%s'\n", slfmt.key[ i ] , slfmt.val[ i ] );
                }

                return( 0 );
            }

    SEE ALSO
        sl_fmtlab()

==DOC==*/

DLL_EXPORT
int
sl_ds1( SLLABEL *lab,
        int type,
        char *dsn,
        char *volser,
        int volseq,
        int dsseq,
        char *expdt,
        int blocks )
{
    int gdg;
    size_t len;
    size_t ndx;
    char wbuf[ 80 ];

    /*
    || Initialize
    */
    memset( lab, ' ', sizeof( SLLABEL ) );

    /*
    || Label ID
    */
    if( ( type != SLT_HDR ) && ( type != SLT_EOF ) && ( type != SLT_EOV ) )
    {
        return( SLE_INVALIDTYPE );
    }
    memcpy( lab->id, sl_alabs[ type ], 3 );

    /*
    || Label number
    */
    lab->num[ 0 ] = '1';

    /*
    || Special IEHINITT dataset name?
    */
    if( ( type == SLT_HDR ) && ( strcmp( dsn, SL_INITDSN ) == 0 ) )
    {
        memset( &lab->slds1, '0', sizeof( lab->slds1 ) );
        sl_atoe( NULL, lab, sizeof( SLLABEL ) );
        return( 0 );
    }

    /*
    || Dataset ID
    */
    ndx = 0;
    len = strlen( dsn );
    if( len > 17 )
    {
        ndx = len - 17;
        len = 17;
    }
    memcpy( lab->slds1.dsid, &dsn[ ndx ], len );

    /*
    || GDG generation and version
    */
    if( len > 9 )
    {
        gdg  = 0;
        gdg += (          dsn[ len - 9 ]   == '.' );
        gdg += (          dsn[ len - 8 ]   == 'G' );
        gdg += ( isdigit( (unsigned char)dsn[ len - 7 ] ) !=  0  );
        gdg += ( isdigit( (unsigned char)dsn[ len - 6 ] ) !=  0  );
        gdg += ( isdigit( (unsigned char)dsn[ len - 5 ] ) !=  0  );
        gdg += ( isdigit( (unsigned char)dsn[ len - 4 ] ) !=  0  );
        gdg += (          dsn[ len - 3 ]   == 'V' );
        gdg += ( isdigit( (unsigned char)dsn[ len - 2 ] ) !=  0  );
        gdg += ( isdigit( (unsigned char)dsn[ len - 1 ] ) !=  0  );

        if( gdg == 9 )
        {
            memcpy( lab->slds1.genno, &dsn[ len - 7 ], 4 );
            memcpy( lab->slds1.verno, &dsn[ len - 2 ], 2 );
        }
    }

    /*
    || Volser
    */
    len = strlen( volser );
    if( len > 6 )
    {
        return( SLE_VOLSER );
    }
    memcpy( lab->slds1.volser, volser, len );

    /*
    || Volume sequence
    */
    if( volseq > 9999 )
    {
        return( SLE_VOLSEQ );
    }
    sprintf( wbuf, "%04u", volseq );
    memcpy( lab->slds1.volseq, wbuf, 4 );

    /*
    || Dataset sequence
    */
    if( dsseq > 9999 )
    {
        return( SLE_DSSEQ );
    }
    sprintf( wbuf, "%04u", dsseq );
    memcpy( lab->slds1.dsseq, wbuf, 4 );

    /*
    || Creation Date
    */
    sl_fmtdate( lab->slds1.crtdt, NULL, FALSE );

    /*
    || Expiration Date
    */
    if( sl_fmtdate( lab->slds1.expdt, expdt, FALSE ) == NULL )
    {
        return( SLE_EXPDT );
    }

    /*
    || Dataset security
    */
    memset( lab->slds1.dssec, '0', 1 );

    /*
    || Block count - low
    */
    if( type == SLT_HDR )
    {
        blocks = 0;
    }
    sprintf( wbuf, "%010u", blocks );
    memcpy( lab->slds1.blklo, &wbuf[ 4 ], 6 );

    /*
    || System code
    */
    memcpy( lab->slds1.syscd, "IBM OS/VS 370", 13 );

    /*
    || Block count - high
    */
    sprintf( wbuf, "%10u", blocks );
    memcpy( lab->slds1.blkhi, wbuf, 4 );

    /*
    || Convert to EBCDIC
    */
    sl_atoe( NULL, lab, sizeof( SLLABEL ) );

    return 0;
}

/*==DOC==

    NAME
            sl_ds2 - Generate a data set label 2

    SYNOPSIS
            #include "sllib.h"

            int sl_ds2( SLLABEL *lab,
                        int type,
                        char *recfm,
                        int lrecl,
                        int blksize,
                        char *jobname,
                        char *stepname,
                        char *trtch )

    DESCRIPTION
            This function builds a data set label 2 based on the parameters
            provided and places it at the location pointed to by the "lab"
            parameter in EBCDIC.

            The "type" parameter must be "SLT_HDR", "SLT_EOF", or "SLT_EOV".

            The remaining parameters correspond to fields within the label
            and are converted to EBCDIC before storing.

            The "recfm" parameter may be one of the following:
                F       FS      V       VS      U
                FA      FSA     VA      VSA     UA
                FM      FSM     VM      VSM     UM
                FB      FBS     VB      VBS
                FBA     FBSA    VBA     VBSA
                FBM     FBSM    VBM     VBSM

            The "trtch" parameter may be blank or one of the following:
                T       C       E       ET      P

    RETURN VALUE
            The return value will be >= 0 if no errors are detected.

            If an error is detected, then the return value will be < 0 and
            will be one of the following:

            SLE_INVALIDTYPE     Invalid label type specified
            SLE_RECFM           Missing or invalid record format
            SLE_LRECL           Invalid record length
            SLE_BLKSIZE         Block size out of range
            SLE_JOBNAME         Missing or invalid job name
            SLE_STEPNAME        Missing or invalid step name
            SLE_TRTCH           Invalid recording technique

    NOTES
            This routine is normally accessed using the supplied "sl_hdr1",
            "sl_eof1", or "sl_eov1" macros.

            Only the "most common" label fields have corresponding parameters
            so the user must supply any other desired values.

    EXAMPLE
            //
            // Create a EOV2 label
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                SLFMT slfmt;
                SLLABEL sllab;
                int i;

                sl_ds2( &sllab,
                        SLT_EOF,
                        "FB",
                        80,
                        32720,
                        "HERCJOB",
                        "HERCSTEP",
                        "P" );

                sl_fmtlab( &slfmt, &sllab );

                for( i = 0 ; slfmt.key[ i ] != NULL ; i++ )
                {
                    printf("%-20.20s: '%s'\n", slfmt.key[ i ] , slfmt.val[ i ] );
                }

                return( 0 );
            }

    SEE ALSO
        sl_fmtlab()

==DOC==*/

DLL_EXPORT
int
sl_ds2( SLLABEL *lab,
        int type,
        char *recfm,
        int lrecl,
        int blksize,
        char *jobname,
        char *stepname,
        char *trtch )
{
    int i;
    size_t len;
    char wbuf[ 80 ];

    /*
    || Initialize
    */
    memset( lab, ' ', sizeof( SLLABEL ) );

    /*
    || Label ID
    */
    if( ( type != SLT_HDR ) && ( type != SLT_EOF ) && ( type != SLT_EOV ) )
    {
        return( SLE_INVALIDTYPE );
    }
    memcpy( lab->id, sl_alabs[ type ], 3 );

    /*
    || Label number
    */
    lab->num[ 0 ] = '1';

    /*
    || Record format/Block Attribute/Control
    */
    if( recfm == NULL )
    {
        return( SLE_RECFM );
    }

    for( i = 0 ; i < (int)VALFMCNT ; i++ )
    {
        if( strcmp( recfm, valfm[ i ].recfm ) == 0 )
        {
            break;
        }
    }

    if( i == VALFMCNT )
    {
        return( SLE_RECFM );
    }

    lab->slds2.recfm[ 0 ]   = valfm[ i ].f;
    lab->slds2.blkattr[ 0 ] = valfm[ i ].b;
    lab->slds2.ctrl[ 0 ]    = valfm[ i ].c;

    /*
    || Block size
    */
    if( blksize == 0 )
    {
        return( SLE_BLKSIZE );
    }

    if( blksize > 32760 )
    {
        sprintf( wbuf, "%10u", blksize );
        memcpy( lab->slds2.lblkln, wbuf, 10 );
        memcpy( lab->slds2.blksize, "00000", 5 );
    }
    else
    {
        sprintf( wbuf, "%05u", blksize );
        memcpy( lab->slds2.blksize, wbuf, 5 );
    }

    /*
    || Logical record length
    */
    switch( lab->slds2.recfm[ 0 ] )
    {
        case 'F':
            if( ( valfm[ i ].b == 'S' ) || ( valfm[ i ].b == ' ' ) )
            {
                if( lrecl != blksize )
                {
                    return( SLE_LRECL );
                }
            }
            else
            {
                if( ( blksize % lrecl ) != 0 )
                {
                    return( SLE_LRECL );
                }
            }
        break;

        case 'V':
            if( valfm[ i ].b == ' ' )
            {
                if( ( lrecl + 4 ) != blksize )
                {
                    return( SLE_LRECL );
                }
            }
            else if( valfm[ i ].b == 'B' )
            {
                if( ( lrecl + 4 ) > blksize )
                {
                    return( SLE_LRECL );
                }
            }
        break;

        case 'U':
            if( lrecl != 0 )
            {
                return( SLE_LRECL );
            }
        break;
    }
    sprintf( wbuf, "%05u", lrecl );
    memcpy( lab->slds2.lrecl, wbuf, 5 );

    /*
    || Jobname and stepname
    */
    if( jobname != NULL )
    {
        if( stepname == NULL )
        {
            return( SLE_STEPNAME );
        }

        len = strlen( jobname );
        if( len > 8 )
        {
            return( SLE_JOBNAME );
        }

        len = strlen( stepname );
        if( len > 8 )
        {
            return( SLE_STEPNAME );
        }
    }
    else
    {
        if( stepname != NULL )
        {
            return( SLE_JOBNAME );
        }
    }
    sprintf( wbuf, "%-8.8s/%-8.8s", jobname, stepname );
    memcpy( lab->slds2.jobid, wbuf, 17 );

    /*
    || Density
    */
    lab->slds2.den[ 0 ] = '0';

    /*
    || Dataset position
    */
    lab->slds2.dspos[ 0 ] = '0';

    /*
    || Tape recording technique
    */
    if( trtch != NULL )
    {
        len = strlen( trtch );
        if( len < 1 || len > 2 )
        {
            return( SLE_TRTCH );
        }

        switch( trtch[ 0 ] )
        {
            case 'T': case 'C': case 'P': case ' ':
                lab->slds2.trtch[ 0 ] = trtch[ 0 ];
            break;

            case 'E':
                lab->slds2.trtch[ 0 ] = trtch[ 0 ];
                if( len == 2 )
                {
                    if( trtch[ 1 ] != 'T' )
                    {
                        return( SLE_TRTCH );
                    }
                    lab->slds2.trtch[ 1 ] = trtch[ 1 ];
                }
            break;

            default:
                return( SLE_TRTCH );
        }
    }

    /*
    || Device serial number
    */
    sprintf( wbuf, "%06u", rand() );
    memcpy( lab->slds2.devser, wbuf, 6 );

    /*
    || Checkpoint dataset identifier
    */
    lab->slds2.ckptid[ 0 ] = ' ';

    /*
    || Convert to EBCDIC
    */
    sl_atoe( NULL, lab, sizeof( SLLABEL ) );

    return 0;
}

/*==DOC==

    NAME
            sl_usr - Generate a user label

    SYNOPSIS
            #include "sllib.h"

            int sl_usr( SLLABEL *lab,
                        int type,
                        int num,
                        char *data )

    DESCRIPTION
            This function builds a user label based on the parameters provided
            and places it at the location pointed to by the "lab" parameter in
            EBCDIC.

            The "type" parameter must be "SLT_UHL" or "SLT_UTL" and the "num"
            parameter must be 1 through 8.

            The remaining parameter corresponds to fields within the label
            and is converted to EBCDIC before storing.

    RETURN VALUE
            The return value will be >= 0 if no errors are detected.

            If an error is detected, then the return value will be < 0 and
            will be one of the following:

            SLE_DATA            Missing or invalid user data

    NOTES
            This routine is normally accessed using the supplied "sl_uhl*"
            or "sl_utl*" macros.

    EXAMPLE
            //
            // Create a UHL6 label
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                SLFMT slfmt;
                SLLABEL sllab;
                int i;

                sl_usr( &sllab,
                        SLT_EOF,
                        6,
                        "Hercules Emulated Tape" );

                sl_fmtlab( &slfmt, &sllab );

                for( i = 0 ; slfmt.key[ i ] != NULL ; i++ )
                {
                    printf("%-20.20s: '%s'\n", slfmt.key[ i ] , slfmt.val[ i ] );
                }

                return( 0 );
            }

    SEE ALSO
        sl_fmtlab()

==DOC==*/

DLL_EXPORT
int
sl_usr( SLLABEL *lab,
        int type,
        int num,
        char *data )
{
    size_t len;

    /*
    || Initialize
    */
    memset( lab, ' ', sizeof( SLLABEL ) );

    /*
    || Label ID
    */
    if( ( type != SLT_UHL ) && ( type != SLT_UTL ) )
    {
        return( SLE_INVALIDTYPE );
    }
    memcpy( lab->id, sl_elabs[ type ], 3 );

    /*
    || Label number
    */
    if( ( num < 1 ) || ( num > 8 ) )
    {
        return( SLE_INVALIDNUM );
    }
    lab->num[ 0 ] = '0' + num;

    /*
    || User data
    */
    if( data == NULL )
    {
        return( SLE_DATA );
    }

    len = strlen( data );
    if( len == 0 || len > 76 )
    {
        return( SLE_DATA );
    }
    memcpy( lab->slusr.data, data, len );

    /*
    || Convert to EBCDIC
    */
    sl_atoe( NULL, lab, sizeof( SLLABEL ) );

    return 0;
}

/*==DOC==

    NAME
            sl_error - Returns a text message for an SL error code

    SYNOPSIS
            #include "sllib.h"

            char *sl_error( int rc )

    DESCRIPTION
            Simply returns a pointer to a string that describes the error
            code passed in the "rc" parameter.

    RETURN VALUE
            The return value is always valid and no errors are returned.

    EXAMPLE
            //
            // Print text for SLE_DSSEQ.
            //

            #include "sllib.h"

            int main( int argc, char *argv[] )
            {
                printf( "SLLIB error: %d = %s\n",
                    SLE_DSSEQ,
                    sl_error( SLE_DSSEQ ) );

                return( 0 );
            }

    SEE ALSO

==DOC==*/

DLL_EXPORT
const char *
sl_error( int rc )
{
    /*
    || If not an error just return the "OK" string
    */
    if( rc >= 0 )
    {
        rc = 0;
    }

    /*
    || Turn it into an index
    */
    rc = -rc;

    /*
    || Within range?
    */
    if( rc >= (int)SL_ERRSTR_MAX )
    {
        rc = SL_ERRSTR_MAX - 1;
    }

    /*
    || Return string
    */
    return( sl_errstr[ rc ] );
}
