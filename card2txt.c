/* CARD2TXT.C   (C) Copyright "Fish" (David B. Trout), 2023-2024     */
/*              Translate EBCDIC card deck to ASCII text file        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*                          CARD2TXT                                 */
/*                                                                   */
/*  This program reads a fixed 80-byte EBCDIC card deck, translates  */
/*  each record to ASCII according to the specified codepage, and    */
/*  then writes it to the specified output file.                     */
/*                                                                   */
/*       card2txt  codepage  ifile  ofile                            */
/*                                                                   */
/*  The input file format is 80 character fixed-length EBCDIC card   */
/*  images. The output is variable length ASCII text records. Both   */
/*  LF only and CR/LF line ending are supported, and depend on the   */
/*  host system where card2txt is running. When running on Windows,  */
/*  the output file record will end with CR/LF. Otherwise they will  */
/*  end with LF only.                                                */
/*                                                                   */
/*  If the input file or output file name contains any spaces, it    */
/*  should be enclosed within double quotes. The specified codepage  */
/*  must be one of the translation code pages supported by Hercules. */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define UTILITY_NAME    "card2txt"
#define UTILITY_DESC    "Translate EBCDIC card deck to ASCII text file utility"

static FILE*   ifile = NULL;
static FILE*   ofile = NULL;

/*-------------------------------------------------------------------*/
/*                            MAIN                                   */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
    char*   pgm = NULL;
    char    ipath[ MAX_PATH ]  = {0};
    char    opath[ MAX_PATH ]  = {0};
    BYTE    cardbuf[ 81 ]; // EBCDIC (guest)
    BYTE    linebuf[ 81 ]; // ASCII (host)
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

    /* Open the input file */
    if (!(ifile = fopen( ipath, "rb" )))  // (binary)
    {
        // "Error opening \"%s\": %s"
        FWRMSG( stderr, HHC03303, "E", ipath, strerror( errno ));
        exit( -1 );
    }

#if defined( _MSVC_ )
  #define OUTFILE_MODE    "wt"     // ('t' = text: \n ==> \r\n)
#else
  #define OUTFILE_MODE    "w"
#endif


    /* Open the output file */
    if (!(ofile = fopen( opath, OUTFILE_MODE )))
    {
        // "Error opening \"%s\": %s"
        FWRMSG( stderr, HHC03303, "E", opath, strerror( errno ));
        exit( -1 );
    }

    /* Read EBCDIC input, translate to ASCII, write output */
    while (fread( cardbuf, 80, 1, ifile ) == 1)
    {
        cardbuf[80] = 0;
        str_guest_to_host( cardbuf, linebuf, 80 );
        linebuf[80] = 0;

        RTRIM( linebuf );         // (remove trailing blanks)
        STRLCAT( linebuf, "\n" ); // (append linefeed)

        if (fwrite( linebuf, strlen( linebuf ), 1, ofile ) != 1)
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
