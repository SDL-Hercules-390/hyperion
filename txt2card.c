/* TXT2CARD.C   (C) Copyright "Fish" (David B. Trout), 2023-2024     */
/*              Translate ASCII text file to EBCDIC                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*                          TXT2CARD                                 */
/*                                                                   */
/*  This program reads an ASCII text file and translates the text    */
/*  to EBCDIC according to the specified code page, and then either  */
/*  writes it to the specified output file, or transmits it to  the  */
/*  specified Hot Card Reader socket, as fixed length 80 byte binary */
/*  records:                                                         */
/*                                                                   */
/*       txt2card  codepage  ifile  {ofile | host:port}              */
/*                                                                   */
/*  The input file format is variable length text records. Both LF   */
/*  only or CR/LF line ending are supported. As lines are read they  */
/*  are padded with blanks to 80 characters long or truncated to a   */
/*  maximum of 80 characters if longer.                              */
/*                                                                   */
/*  If the input file or output file name contains any spaces, it    */
/*  should be enclosed within double quotes. The specified codepage  */
/*  must be one of the translation code pages supported by Hercules. */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define UTILITY_NAME    "txt2card"
#define UTILITY_DESC    "Translate ASCII text file to EBCDIC card deck utility"

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
    char*   colon   = NULL;
    size_t  bufsiz = 0;
    int     rc = 0;
    int     sd = 0;

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

    /* Determine if output is to a file or to a socket */

    if (!(colon = strchr( &argv[3][2], ':' )))
    {
        // Output is to a file

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
    }
    else // Output is to a socket
    {
        int                 cards  = 0;
        int                 port   = 0;
        struct sockaddr_in  sin;
        fd_set              writefds;
        struct timeval      tv;

        /* Open the input file */
        if (!(ifile = fopen( ipath, INFILE_MODE )))
        {
            // "Error opening \"%s\": %s"
            FWRMSG( stderr, HHC03303, "E", ipath, strerror( errno ));
            exit( -1 );
        }

        /* Create output socket */

        socket_init();

        if ((sd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0)
        {
            // "Socket creation error: %s"
            FWRMSG( stderr, HHC03308, "E", strerror( HSO_errno ));
            socket_deinit();
            exit( -1 );
        }

        /* Temporarily set it to non-blocking mode for the duration
           of the connect so we can specify a connect timeout value */

        if (socket_set_blocking_mode( sd, false ) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "socket_set_blocking_mode()", strerror( HSO_errno ));
            goto socket_error_exit;
        }

        /* Resolve the ip-number or hostname and port number */

        if ((port = atoi( colon+1 )) < 1024 || port > 65535)
        {
            // "Invalid socket specification: %s"
            FWRMSG( stderr, HHC03305, "E", argv[3] );
            goto socket_error_exit;
        }

        memset( &sin, 0, sizeof( sin ));

        sin.sin_family  = AF_INET;
        sin.sin_port    = htons( port );

        *colon = 0;
        {
            /* Is host an IP number or name? */

            if (is_numeric( argv[3] ))
            {
                sin.sin_addr.s_addr = inet_addr( argv[3] );
            }
            else
            {
                struct hostent* he = gethostbyname( argv[3] );

                if (!he)
                {
                    // "Invalid socket specification: %s"
                    *colon = ':';
                    FWRMSG( stderr, HHC03305, "E", argv[3] );
                    goto socket_error_exit;
                }

                memcpy( &sin.sin_addr, *he->h_addr_list, sizeof( sin.sin_addr ));
            }
        }
        *colon = ':';

        /* Try to connect to the Hercules reader */

        if (connect( sd, (struct sockaddr*) &sin, sizeof( sin )) < 0)
        {
#if defined(_MSVC_)
            if (HSO_errno != HSO_EWOULDBLOCK)
#else
            if (HSO_errno != HSO_EINPROGRESS)
#endif
            {
                // "Error connecting to %s"
                FWRMSG( stderr, HHC03309, "E", argv[3] );
                goto socket_error_exit;
            }

            /* Wait for connect to complete */

            FD_ZERO( &writefds );
            FD_SET( sd, &writefds );

            tv.tv_sec   = 3;    // (should be PLENTY of time!)
            tv.tv_usec  = 0;

            if ((rc = select( sd+1, NULL, &writefds, NULL, &tv )) <= 0)
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "select()", strerror( HSO_errno ));
                goto socket_error_exit;
            }
        }

        /* Reset the socket back to blocking mode */

        if (socket_set_blocking_mode( sd, true ) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "socket_set_blocking_mode()", strerror( HSO_errno ));
            goto socket_error_exit;
        }

        /* Read ASCII input, translate to EBCDIC, write output */
        rc = 0;
        while (my_getline( &linebuf, &bufsiz, ifile ) >= 0)
        {
            cards++;
            rtrim( linebuf, "\n" );
            str_host_to_guest( linebuf, cardbuf, sizeof( cardbuf ));

            if (write_socket( sd, cardbuf, 80 ) < 80)
            {
                // "Transmission error on socket: %s"
                FWRMSG( stderr, HHC03306, "E", strerror( HSO_errno ));
                rc = -1;
                break;
            }
        }

        /* Close input */

        fclose( ifile );

        /* GRACEFULLY close the output socket */
        {
            int rc2;
            shutdown( sd, SHUT_WR );
            do rc2 = read_socket( sd, cardbuf, 1 );
            while (rc2 > 0);
            close_socket( sd );
        }

        socket_deinit();

        if (rc == 0)
            // "%d cards submitted"
            FWRMSG( stdout, HHC03307, "I", cards );
    }

    /* Exit with normal or i/o error return code */
    return rc;

socket_error_exit:

    close_socket( sd );
    socket_deinit();
    exit( -1 );
}
