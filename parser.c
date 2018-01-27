/* PARSER.C     (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules Simple parameter parser                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _PARSER_C_
#define _HUTIL_DLL_

#include "hercules.h"
#include "parser.h"

#if !defined( NULL )
#define NULL 0
#endif

/*

    NAME
            parser - parse parameter strings

    SYNOPSIS
            #include "parser.h"

            int parser( PARSER *pp, char *str, void *res )

    DESCRIPTION
            The parser() function breaks the str parameter into a keyword
            and value using the "=" as a delimiter.  The pp table is then
            scanned for the keyword.  If found and the table entry specifies
            a format string, then parser() will use sscanf() to store the
            value at the location specifed by res.

            The PARSER table entries consist of a string pointer that
            designates the keyword and a string pointer that designates the
            format of the associated value.  The format may be set to NULL
            if the keyword does not take a value.  The table must be
            terminated with an entry that specifies NULL for the keyword
            string.

    RETURN VALUE
            If no errors are detected then the returned value will be the
            index+1 of the matching table entry.  The res argument will be
            updated only when a format string is specified.

            If an entry couldn't be found, zero will be returned and res
            will remain untouched.

            If an entry is found, but an error is detected while processing
            the str parameter then a negative value is returned.  This value
            will be the index-1 of the matching table entry.

    EXAMPLE
            #include "parser.h"
            #include <stdlib.h>

            PARSER ptab[] =
            {
                { "switchkey", NULL            },
                { "numkey",    "%d"            },
                { "strkey",    PARSER_STR_TYPE },  // *NEVER* use just "%s" !!
                {NULL,NULL}  // (end of table)
            };

            int main( int argc, char *argv[] )
            {
                int rc;
                union
                {
                    int num;
                    char str[ MAX_PARSER_STRLEN + 1 ];  // (+1 for null terminator)
                } res;

                while( --argc )
                {
                    rc = parser( &ptab[0], argv[ argc ], &res );
                    printf( "parser() rc = %d\n", rc );
                    if( rc < 0 )
                    {
                        printf( "error parsing keyword: %s\n",
                            ptab[ abs( rc  1 ) ].key );
                    }
                    else if( rc == 0 )
                    {
                        printf( "unrecognized keyword: %s\n",
                            argv[ argc ] );
                    }
                    else
                    {
                        switch( rc )
                        {
                            case 1:
                                printf( "found switchkey\n" );
                            break;
                            case 2:
                                printf( "numkey value is: %d\n", res.num );
                            break;
                            case 3:
                                printf( "strkey value is: %s\n", res.str );
                            break;
                        }
                    }
                }
            }

    SEE ALSO
            sscanf(3)

*/

DLL_EXPORT int
parser( PARSER *pp, char *str, void *res )
{
    int ndx;
    char *key;
    char *val;
    char *strtok_str = NULL;

    ndx = 1;
    key = strtok_r(  str, "=", &strtok_str );
    val = strtok_r( NULL, "=", &strtok_str  );

    while( pp->key != NULL )
    {
        if( strcasecmp( key, pp->key ) == 0 )
        {
            if( pp->fmt == NULL )
            {
                if( val != NULL )
                {
                    return( -ndx );
                }
            }
            else
            {
                if( val == NULL )
                {
                    return( -ndx );
                }

                if( sscanf( val, pp->fmt, res ) != 1 )
                {
                    return( -ndx );
                }
            }

            return( ndx );
        }
        pp++;
        ndx++;
    }

    return( 0 );
}
