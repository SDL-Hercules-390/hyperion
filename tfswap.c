/* TFSWAP.C     (C) Copyright "Fish" (David B. Trout), 2023          */
/*              Swap Trace File Endianness Utility                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*  This program reads a Hercules intruction trace file and swaps    */
/*  the endianness of each field in each record so that it can be    */
/*  processed more efficiently by the tfprint utility.               */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define UTILITY_NAME    "tfswap"
#define UTILITY_DESC    "Swap Trace File Endianness Utility"

static FILE*   infile = NULL;
static double  filesize = 0;

/*-------------------------------------------------------------------*/
/*             Show them their command line arguments                */
/*-------------------------------------------------------------------*/
static void print_args( int argc, char* argv[] )
{
    // PROGRAMMING NOTE: I know of no other way to accomplish this!

    int i; bool hasblanks;

    printf( "\n" );

    /* If debug mode, show filename and line number prefix
       the same way the logmsg.c "vfwritemsg" function does. */
    if (MLVL( DEBUG ))
    {
        static char wrk    [ MLVL_DEBUG_PFXLEN + 2 ];
        static char prefix [ MLVL_DEBUG_PFXLEN + 2 ];

        MSGBUF( wrk, "%s(%d)", TRIMLOC( __FILE__ ), __LINE__ );
        MSGBUF( prefix, MLVL_DEBUG_PFXFMT, wrk );
        printf( "%s", prefix );
    }

    // PROGRAMMING NOTE: we purposely do *NOT* try to print everything
    // into a buffer and then do one printf of that buffer since we do
    // not know how big that buffer should be. Some arguments could be
    // quite long! (Yeah, yeah, we COULD use malloc/realloc much like
    // BFR_VSNPRINTF does, but that would be overkill IMHO).

    // HHC03217 "Args: %s"
    printf( "HHC03217I Args:" );

    for (i=1; i < argc; ++i)
    {
        printf( " " );  // (separate arguments with a blank)

        if ((hasblanks = strpbrk( argv[i], WHITESPACE ) ? true : false))
            printf( "\"" ); // (enclose within quotes if needed)

        printf( "%s", argv[i] );  // (the argument, as-is)

        if (hasblanks)
            printf( "\"" );
    }

    printf( "\n\n" );
}

/*-------------------------------------------------------------------*/
/*             Issue Periodic Progress Messages                      */
/*-------------------------------------------------------------------*/
static void show_file_progress()
{
    off_t   currpos;
    double  percent;

    if ((currpos = ftell( infile )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC00075, "E", "ftell", strerror( errno ));
        exit( -1 );
    }

    percent = ((double) currpos) / filesize;
    percent *= 100.0;

    if (extgui)
        fprintf( stderr, "PCT=%.1f\n", percent );
    else if (isatty( fileno( stderr )))
        fprintf( stderr, "%.1f%% of file processed...\r", percent );
}

/*-------------------------------------------------------------------*/
/*                            MAIN                                   */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
    char*   pgm                     = NULL;
    FILE*   outfile                 = NULL;
    TFSYS*  sys                     = NULL;
    TFHDR*  hdr                     = NULL;
    BYTE*   rec                     = NULL;
    U64     recnum                  = 0;
    size_t  bytes_read              = 0;
    int     amt                     = 0;
    bool    doendswap               = false;
    bool    file_big_endian         = false;
    U16     curr                    = 0;
    U16     msgnum                  = 0;

    char    inpath [ MAX_PATH ]     = {0};
    char    outpath[ MAX_PATH ]     = {0};
    BYTE    iobuff[ _64_KILOBYTE ]  = {0};

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Show them the arguments they entered */
    print_args( argc, argv );

    if (argc != 3)
    {
        // "Usage:  tfswap  infile  outfile\n"
        FWRMSG( stderr, HHC03250, "E" );
        exit( -1 );
    }

    hostpath( inpath,  argv[1], sizeof( inpath  ));
    hostpath( outpath, argv[2], sizeof( outpath ));

#if defined( _MSVC_ )
#define INFILE_MODE    "rbS"     // ('S' = cached sequential access)
#else
#define INFILE_MODE    "rb"
#endif

    /* Open the input file */
    if (!(infile = fopen( inpath, INFILE_MODE )))
    {
        // "Error opening \"%s\": %s"
        FWRMSG( stderr, HHC03251, "E", inpath, strerror( errno ));
        exit( -1 );
    }

    /* Save input file size */
    if (fseek( infile, 0, SEEK_END ) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC00075, "E", "fseek", strerror( errno ));
        exit( -1 );
    }
    else
    {
        off_t  off_size;    // (file size)

        if ((off_size = ftell( infile )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "ftell", strerror( errno ));
            exit( -1 );
        }
        else
        {
            if (fseek( infile, 0, SEEK_SET ) < 0)
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "fseek", strerror( errno ));
                exit( -1 );
            }
            else
                filesize = (double) off_size;
        }
    }

    /* Open the output file */
    if (!(outfile = fopen( outpath, "wb" )))
    {
        // "Error opening \"%s\": %s"
        FWRMSG( stderr, HHC03251, "E", outpath, strerror( errno ));
        exit( -1 );
    }

    /* Read Trace File TFSYS record */
    if ((bytes_read = fread( iobuff, 1, sizeof( TFSYS ), infile )) != sizeof( TFSYS ))
    {
        // "Error reading trace file: %s"
        FWRMSG( stderr, HHC03252, "E", strerror( errno ));
        exit( -1 );
    }

    if (bytes_read < sizeof( TFSYS ))
    {
        // "Truncated %s record; aborting"
        FWRMSG( stderr, HHC03253, "E", "TFSYS" );
        exit( -1 );
    }

    /* Point to the TFSYS record we just read */
    sys = (TFSYS*) iobuff;

    /* Validate the TFSYS record */
    if (0
        || sys->ffmt[0] != '%'
        || sys->ffmt[1] != 'T'
        || sys->ffmt[2] != 'F'
    )
    {
        // "File does not start with TFSYS record; aborting"
        FWRMSG( stderr, HHC03212, "E" );
        exit( -1 );
    }

    if (0
        || sys->ffmt[3] < '0'
        || sys->ffmt[3] > TF_FMT
    )
    {
        // "Unsupported Trace File format: %%TF%c"
        FWRMSG( stderr, HHC03213, "E", sys->ffmt[3] );
        exit( -1 );
    }

    /* Save the endianness of the file */
    file_big_endian = sys->bigend ? true : false;

    /* Determine whether file is already in our endian format or not */
    doendswap = tf_are_swaps_needed( sys );

    /* Swap the endianness of the TFSYS record and write it out */
    tf_swap_sys( sys );
    if (fwrite( sys, 1, sizeof( TFSYS ), outfile ) != sizeof( TFSYS ))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC00075, "E", "fwrite()", strerror( errno ));
        exit( -1 );
    }

    /* Now read, swap, and write all the other records in the file */

    hdr = (TFHDR*) iobuff;

    while (1)
    {
        /* Read just TFHDR for now, so we can identify record */
        if ((bytes_read = fread( hdr, 1, sizeof( TFHDR ), infile )) != sizeof( TFHDR ))
        {
            /* Stop when EOF reached */
            if (!bytes_read)
                break;

            // "Error reading trace file: %s"
            FWRMSG( stderr, HHC03252, "E", strerror( errno ));
            exit( -1 );
        }

        /* Make sure we have a complete header to work with */
        if (bytes_read < sizeof( TFHDR ))
        {
            // "Truncated %s record; aborting"
            FWRMSG( stderr, HHC03253, "E", "TFHDR" );
            exit( -1 );
        }

        /* Save needed TFHDR fields in OUR native endian format */
        curr   = doendswap ? SWAP16( hdr->curr   ) : hdr->curr;
        msgnum = doendswap ? SWAP16( hdr->msgnum ) : hdr->msgnum;

        /* Swap the TFHDR we just read */
        tf_swap_hdr( hdr );

        /* Finish reading in the remainder of the record */
        rec = (BYTE*) (hdr + 1);
        amt = curr - sizeof( TFHDR );

        if ((bytes_read = fread( rec, 1, amt, infile )) != (size_t)amt)
        {
            /* Stop when EOF reached */
            if (!bytes_read)
                break;

            /* "Error reading trace file: %s" */
            FWRMSG( stderr, HHC03252, "E", strerror( errno ));
            exit( -1 );
        }

        if (bytes_read < (size_t)amt)
        {
            /* "Truncated %s record; aborting" */
            static char buf[8];
            MSGBUF( buf, "%"PRIu16, msgnum );
            FWRMSG( stderr, HHC03253, "E", buf );
            exit( -1 );
        }

        /* Swap the TF record just read */
        tf_swap_rec( hdr, msgnum );

        /* Write out the swapped record */
        amt += sizeof( TFHDR );
        if (fwrite( hdr, 1, amt, outfile ) != (size_t)amt)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "fwrite()", strerror( errno ));
            exit( -1 );
        }

        /* Issue periodic progress messages */
        if (!(++recnum % 1000))
            show_file_progress();
    }

    /* DONE! (That was easy!) */
    fclose( infile );
    fclose( outfile );

    // "File successfully swapped from %s endian to %s endian"
    FWRMSG( stdout, HHC03254, "I",  file_big_endian ? "BIG" : "little",
                                   !file_big_endian ? "BIG" : "little" );
    return 0;
}
