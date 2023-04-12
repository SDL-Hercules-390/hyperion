/* TXT2CARD.C   (C) Copyright "Fish" (David B. Trout), 2023          */
/*              Copy text file from one code page to another         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*                          TXT2CARD                                 */
/*                                                                   */
/*  This program reads an ASCII text file and translates the text    */
/*  to EBCDIC according to the specified code page. The input file   */
/*  format is variable length text records. Both CR/LF or LF only    */
/*  line ending are supported. As lines are read they are padded     */
/*  with blanks to 80 characters long or truncated to a maximum of   */
/*  80 characters if longer. They are then translated to EBCDIC      */
/*  according to the specified code page and written to the output   */
/*  file as fixed length 80 byte binary records.                     */
/*                                                                   */
/*       txt2card  codepage  ifile  ofile                            */
/*                                                                   */
/*  If the input file or output file name contains any spaces it     */
/*  should be surrounded with double quotes. The specified codepage  */
/*  must be one of the translation code pages supported by Hercules. */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define UTILITY_NAME    "txt2card"
#define UTILITY_DESC    "Translate text file to EBCDIC utility"

static FILE*   ifile = NULL;
static FILE*   ofile = NULL;

/*-------------------------------------------------------------------*/
/*                         my_getline                                */
/*-------------------------------------------------------------------*/
ssize_t  my_getline( char** buf, size_t* bufsiz, FILE* f )
{
    char*   bufptr = NULL;
    char*   p = bufptr;
    size_t  size;
    int     c;

    if (0
        || !buf
        || !f
        || !bufsiz
    )
    {
        return -1;
    }

    bufptr = *buf;
    size = *bufsiz;

    if ((c = fgetc( f )) == EOF)
        return -1;

    if (!bufptr)
    {
        if (!(bufptr = malloc( 128 )))
            return -1;
        size = 128;
    }

    p = bufptr;

    while (c != EOF)
    {
        if (1
            && (p - bufptr + 1) > (ssize_t)size
            && !(bufptr = realloc( bufptr, (size += 128) ))
        )
            return -1;

        *p++ = c;

        if (c == '\n')
            break;

        c = fgetc( f );
    }

    *p++ = '\0';
    *buf = bufptr;
    *bufsiz = size;

    return p - bufptr - 1;
}

/*-------------------------------------------------------------------*/
/*                            MAIN                                   */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
    char*   pgm = NULL;
    char    ipath[ MAX_PATH ]  = {0};
    char    opath[ MAX_PATH ]  = {0};
    BYTE    cardbuf[ 80 ];
    char*   linebuf = NULL;
    size_t  bufsiz = 0;
    int     rc = 0;

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    if (argc != 4)
    {
        // Incorrect number of arguments"
        FWRMSG( stderr, HHC03301, "E" );
        FWRMSG( stderr, HHC03300, "I" );
        exit( -1 );
    }

    if (!valid_codepage_name( argv[1] ))
    {
        // "Invalid/unsupported codepage"
        FWRMSG( stderr, HHC03302, "E" );
        FWRMSG( stderr, HHC03300, "I" );
        exit( -1 );
    }

    set_codepage( argv[1] );
    hostpath( ipath, argv[2], sizeof( ipath ));
    hostpath( opath, argv[3], sizeof( opath ));

#if defined( _MSVC_ )
#define INFILE_MODE    "rtS"     // ('t' = text, 'S' = cached sequential access)
#else
#define INFILE_MODE    "r"
#endif

    /* Open the input file */
    if (!(ifile = fopen( ipath, INFILE_MODE )))
    {
        // "Error opening \"%s\": %s"
        FWRMSG( stderr, HHC03303, "E", ipath, strerror( errno ));
        exit( -1 );
    }
    /* Open the output file */
    if (!(ofile = fopen( opath, "wb" )))
    {
        // "Error opening \"%s\": %s"
        FWRMSG( stderr, HHC03303, "E", opath, strerror( errno ));
        exit( -1 );
    }

    /* Read ASCII input, translate to EBCDIC, write output */
    while (my_getline( &linebuf, &bufsiz, ifile ) >= 0)
    {
        rtrim( linebuf, "\n" );
        str_host_to_guest( linebuf, cardbuf, sizeof( cardbuf ));

        if (fwrite( cardbuf, sizeof( cardbuf ), 1, ofile ) != 1)
            break;
    }

    /* Check for I/O error */
    if (!feof( ifile ))
    {
        // "I/O error on file \"%s\": %s"
        FWRMSG( stderr, HHC03304, "E", argv[2], strerror( errno ) );
        rc = -1;
    }

    if (ferror( ofile ))
    {
        // "I/O error on file \"%s\": %s"
        FWRMSG( stderr, HHC03304, "E", argv[3], strerror( errno ) );
        rc = -1;
    }

    /* Close input and output */
    fclose( ifile );
    fclose( ofile );

    /* Exit with normal or i/o error return code */
    return rc;
}
