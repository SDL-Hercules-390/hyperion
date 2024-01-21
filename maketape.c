/* MAKETAPE.C   (c) Copyright Jay Moseley, CCP 2000                        */
/*              Create AWSTAPE image file                                  */
/*                                                                         */
/* This program is placed in the public domain and may be freely used      */
/* and incorporated into derived works so long as attribution to the       */
/* original authorship remains in any distributed copies of the C source.  */
/* /Jay Moseley/ January, 2008                                             */

/*-------------------------------------------------------------------------*/
/* Reads one or more line sequential ASCII files and creates one or more   */
/* datasets contained on an AWSTAPE image file.  It was first created by   */
/* Jay Moseley and later enhanced by Paul Edwards to add support for the   */
/* BINARY, NLTAPE and ANSI keywords.  Refer to the following URL:          */
/*   http://www.jaymoseley.com/hercules/awstape_utilities/mawstape.htm     */
/* Fish then converted it into a Hercules tape utility, fixing some bugs   */
/* along the way and adding support for the CODEPAGE and other options.    */
/*-------------------------------------------------------------------------*/

// Reference:

// https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_71/rzatb/vdefn.htm
// https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_71/rzatb/hdr1d.htm
// https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzatb/hdr2d.htm
// https://www-01.ibm.com/support/docview.wss?uid=isg3T1000174

#include "hstdinc.h"
#include "hercules.h"
#include "tapedev.h"

#define UTILITY_NAME    "maketape"
#define UTILITY_DESC    "Create an .AWS tape file from data"

#define MAXFILES        50         /* Maximum files per run                */
#define MAXLRECL        32767      /* Maximum record length                */
#define MAXAWSBUFF      65535      /* Maximum AWS buffer size              */
#define MAXTAPEDSNLEN   17         /* Maximum tape dataset name length     */

#define outputGLOB      0          /* output single dataset                */
#define outputUNIQUE   -1          /* each i/p becomes separate o/p        */

#define RC_SYNTAX       1          /* No arguments given                   */
#define RC_ARG_ERRS     2          /* Invalid/missing argument(s)          */
#define RC_OPEN_OUT     3          /* Open output file error               */
#define RC_HDR_IOERR    4          /* I/O error writing AWS header         */
#define RC_DATA_IOERR   5          /* I/O error writing data block         */
#define RC_WTM_IOERR    6          /* I/O error writing tapemark           */
#define RC_OPEN_INPUT   7          /* Open input file error                */
#define RC_READ_IOERR   8          /* Error reading input file             */

/*-------------------------------------------------------------------------*/
/*                        Global variables                                 */
/*-------------------------------------------------------------------------*/

AWSTAPE_BLKHDR awshdr = {0};       /* AWSTAPE block header                 */

char*  pInFileID    = NULL;        /* -> input file name                   */
char*  pVolSer      = NULL;        /* -> volume serial number              */
char*  datasetName  = NULL;        /* -> dataset name                      */
char*  outFileID    = NULL;        /* -> output file name                  */
char*  pLRECL       = NULL;        /* -> LRECL option                      */
char*  pBLOCK       = NULL;        /* -> BLOCK option                      */
char*  pCODEPAGE    = NULL;        /* -> CODEPAGE option                   */

char*  mode         = "r";         /* mode for input files to be opened in */

int   lrecl         = 80;          /* input/output LRECL                   */
int   blkfactor     = 1;           /* output blocking factor               */
int   outGLOBBING   = outputGLOB;  /* output single dataset                */

bool  binary        = false;       /* whether input files are binary       */
bool  nltape        = false;       /* whether this is a non-labelled tape  */
bool  ansi          = false;       /* whether this is an ANSI-label tape   */

int   inFileSeq;                   /* used to index inFileID               */
int   totfiles;                    /* Total files in inFileID              */
int   blkSize;                     /* physical block size                  */
int   blkCount;                    /* block count for EOF1                 */

FILE*  inMeta;                     /* pointer to meta file stream          */
FILE*  inData;                     /* pointer to input data file stream    */
FILE*  outf;                       /* output file                          */

char  julianToday[5+1];            /* today's date in julian format        */
char  volSer[6];                   /* volume serial number (no end NULL!)  */

char  inFileID[ MAXFILES ][ FILENAME_MAX + 5 ];  /* array of MAXFILES files
                                                    to read/include        */
bool  inBINopt[ MAXFILES ];        /* array of "BIN" flags for each file   */

char  buf[ MAXAWSBUFF ];           /* output AWSTAPE buffer                */

/*-------------------------------------------------------------------------*/
/*                           Error exit                                    */
/*-------------------------------------------------------------------------*/
static void ErrExit( int rc )
{
    exit( rc );                 /* (common breakpoint location) */
}

/*-------------------------------------------------------------------------*/
/*       build julian date from system date for standard label             */
/*-------------------------------------------------------------------------*/
static void julianDate()
{
time_t      timer;
struct tm*  tm;
char        year[3+1];             /* y2k compensation             */

    timer = time( NULL );          /* get time of day from system  */
    tzset();                       /* read TZ environment variable */
    tm = localtime( &timer );      /* convert to date/time block   */

    MSGBUF( year, "%3.3i", tm->tm_year );
    strncpy( julianToday, year+1, 2 );
    snprintf(  julianToday+2, 4, "%3.3i", tm->tm_yday );
}

/*-------------------------------------------------------------------------*/
/*                VOLSER / DATASET validation routines                     */
/*-------------------------------------------------------------------------*/
static bool valid_alpha( char ch )
{
    return
    (1
        && 'A' <= ch
        && 'Z' >= ch
    );
}
static bool valid_numeric( char ch )
{
    return
    (1
        && '0' <= ch
        && '9' >= ch
    );
}
static bool valid_alphanumeric( char ch )
{
    return
    (0
        || valid_alpha   ( ch )
        || valid_numeric ( ch )
    );
}
static bool valid_national( char ch )
{
    return
    (0
        || '@' == ch
        || '$' == ch
        || '#' == ch
    );
}
static bool valid_volser( const char* volser )
{
    int i, k = (int) strlen( volser );
    if (k < 1 || k > 6)
        return false;
    for (i=0; i < k; i++)
    {
        if (!(0
            || valid_alphanumeric ( volser[i] )
            || valid_national     ( volser[i] )
        ))
            return false;
    }
    return true;
}
static bool valid_dsn_first( char ch )
{
    return
    (0
        || valid_alpha    ( ch )
        || valid_national ( ch )
    );
}
static bool valid_dsn_remaining( char ch )
{
    return
    (0
        || valid_alphanumeric ( ch )
        || valid_national     ( ch )
        || '-'       ==       ( ch )
    );
}
static bool valid_dsn_segment( const char* segment )
{
    int i, k = (int) strlen( segment );
    if (0
        || k < 1
        || k > 8
        || !valid_dsn_first( segment[0] )
    )
        return false;
    for (i=1; i < k; i++)
        if (!valid_dsn_remaining( segment[i] ))
            return false;
    return true;
}
static bool valid_dataset( const char* dataset )
{
    char dsn[ MAXTAPEDSNLEN + 1 ]; char* segment;
    int k = (int) strlen( dataset );
    if (k < 1 || k > MAXTAPEDSNLEN)
        return false;
    STRLCPY( dsn, dataset );
    segment = strtok( dsn, "." );
    while (segment)
    {
        if (!valid_dsn_segment( segment ))
            return false;
        segment = strtok( NULL, "." );
    }
    return true;
}

/*-------------------------------------------------------------------------*/
/*             parse command line arguments and validate                   */
/*-------------------------------------------------------------------------*/
static void parseCommand( int argc, char* argv[] )
{
int  i;                            /* index to parse arguments             */
bool errors;                       /* indicate missing/invalid arguments   */

    /* if no parameters supplied, display syntax and exit */
    if (argc <= 1)
    {
        WRMSG( HHC02770, "I" );
        ErrExit( RC_SYNTAX );
    }

    errors = false;

    /* copy parameter values into global variables */
    for (i = 1; i < argc; i++)
    {
        /* looking for variations on INPUT: keyword */
        if (0
            || !strcasecmp( argv[i], "input"  )
            || !strcasecmp( argv[i], "input:" )
            || !strcasecmp( argv[i], "in"     )
            || !strcasecmp( argv[i], "in:"    )
            || !strcasecmp( argv[i], "i"      )
            || !strcasecmp( argv[i], "i:"     )
        )
        {
            if (i >= (argc-1))
            {
                // "%s missing following %s parameter"
                FWRMSG( stderr, HHC02771, "E", "<input file name>", argv[i] );
                errors = true;
            }
            else
                pInFileID = argv[(++i)];
            continue;
        }

        /* looking for variations on OUTPUT: keyword */
        if (0
            || !strcasecmp( argv[i], "output"  )
            || !strcasecmp( argv[i], "output:" )
            || !strcasecmp( argv[i], "out"     )
            || !strcasecmp( argv[i], "out:"    )
            || !strcasecmp( argv[i], "o"       )
            || !strcasecmp( argv[i], "o:"      )
        )
        {
            if (i >= (argc-1))
            {
                // "%s missing following %s parameter"
                FWRMSG( stderr, HHC02771, "E", "<output file name>", argv[i] );
                errors = true;
            }
            else
                outFileID = argv[(++i)];
            continue;
        }

        /* looking for variations on VOLSER: keyword */
        if (0
            || !strcasecmp( argv[i], "volser"  )
            || !strcasecmp( argv[i], "volser:" )
            || !strcasecmp( argv[i], "vol"     )
            || !strcasecmp( argv[i], "vol:"    )
            || !strcasecmp( argv[i], "v"       )
            || !strcasecmp( argv[i], "v:"      )
        )
        {
            if (i >= (argc-1))
            {
                // "%s missing following %s parameter"
                FWRMSG( stderr, HHC02771, "E", "<serial number>", argv[i] );
                errors = true;
            }
            else
                pVolSer = argv[(++i)];
            continue;
        }

        /* looking for variations on DATASET: keyword */
        if (0
            || !strcasecmp( argv[i], "dataset"  )
            || !strcasecmp( argv[i], "dataset:" )
            || !strcasecmp( argv[i], "data"     )
            || !strcasecmp( argv[i], "data:"    )
            || !strcasecmp( argv[i], "dsn"      )
            || !strcasecmp( argv[i], "dsn:"     )
            || !strcasecmp( argv[i], "d"        )
            || !strcasecmp( argv[i], "d:"       )
        )
        {
            if (i >= (argc-1))
            {
                // "%s missing following %s parameter"
                FWRMSG( stderr, HHC02771, "E", "<dataset name>", argv[i] );
                errors = true;
            }
            else
                datasetName = argv[(++i)];
            continue;
        }

        /* looking for variations on LRECL: keyword */
        if (0
            || !strcasecmp( argv[i], "lrecl"  )
            || !strcasecmp( argv[i], "lrecl:" )
            || !strcasecmp( argv[i], "l"      )
            || !strcasecmp( argv[i], "l:"     )
        )
        {
            if (i >= (argc-1))
            {
                // "%s missing following %s parameter"
                FWRMSG( stderr, HHC02771, "E", "<record length>", argv[i] );
                errors = true;
            }
            else
                pLRECL = argv[(++i)];
            continue;
        }

        /* looking for variations on BLOCK: keyword */
        if (0
            || !strcasecmp( argv[i], "block"  )
            || !strcasecmp( argv[i], "block:" )
            || !strcasecmp( argv[i], "blk"    )
            || !strcasecmp( argv[i], "blk:"   )
            || !strcasecmp( argv[i], "b"      )
            || !strcasecmp( argv[i], "b:"     )
        )
        {
            if (i >= (argc-1))
            {
                // "%s missing following %s parameter"
                FWRMSG( stderr, HHC02771, "E", "<blocking factor>", argv[i] );
                errors = true;
            }
            else
                pBLOCK = argv[(++i)];
            continue;
        }

        /* looking for variations on CODEPAGE: keyword */
        if (0
            || !strcasecmp( argv[i], "codepage"  )
            || !strcasecmp( argv[i], "codepage:" )
            || !strcasecmp( argv[i], "cp"    )
            || !strcasecmp( argv[i], "cp:"   )
            || !strcasecmp( argv[i], "c"      )
            || !strcasecmp( argv[i], "c:"     )
        )
        {
            if (i >= (argc-1))
            {
                // "%s missing following %s parameter"
                FWRMSG( stderr, HHC02771, "E", "<code page>", argv[i] );
                errors = true;
            }
            else
                pCODEPAGE = argv[(++i)];
            continue;
        }

        /* looking for variations on UNIQUE keyword */
        if (0
            || !strcasecmp( argv[i], "unique" )
            || !strcasecmp( argv[i], "u"      )
        )
        {
            outGLOBBING = outputUNIQUE;
            continue;
        }

        /* looking for variations on BINARY keyword */
        if (0
            || !strcasecmp( argv[i], "binary" )
            || !strcasecmp( argv[i], "bin"    )
        )
        {
            mode = "rb";
            binary = true;
            continue;
        }

        /* looking for variations on NLTAPE keyword */
        if (0
            || !strcasecmp( argv[i], "nltape" )
            || !strcasecmp( argv[i], "nl"     )
            || !strcasecmp( argv[i], "n"      )
        )
        {
            nltape = true;
            continue;
        }

        /* looking for variations on ANSI keyword */
        if (0
            || !strcasecmp( argv[i], "ansi" )
            || !strcasecmp( argv[i], "a"    )
        )
        {
            ansi = true;
            continue;
        }

        // "unsupported parameter: %s"
        FWRMSG( stderr, HHC02780, "E", argv[i] );
        errors = true;
        continue;

    } /* for args */

    /* Validate required argument values... */

    /* initialize input file path table to null values */
    for (inFileSeq = 0; inFileSeq < MAXFILES; inFileSeq++)
    {
        STRLCPY( inFileID[ inFileSeq ], "" );
        inBINopt[ inFileSeq ] = binary;
    }

    /* if single input file specified */
    if (strncasecmp( pInFileID, "@", 1 ) != 0)
    {
        if (!(inData = fopen( pInFileID, "r" )))
        {
            // "Error opening %s file '%s': %s"
            FWRMSG( stderr, HHC02772, "E", "input", pInFileID, strerror( errno ));
            errors = true;
        }
        else
        {
            fclose( inData );
            inData = NULL;
            totfiles = 1;
            STRLCPY( inFileID[0], pInFileID );
            inBINopt[0] = binary;
        }
    }
    else /* specified file is a @meta file */
    {
        if (!(inMeta = fopen( pInFileID + 1, "r" )))
        {
            // "Error opening %s file '%s': %s"
            FWRMSG( stderr, HHC02772, "E", "input (meta)", pInFileID, strerror( errno ));
            errors = true;
        }
        else
        {
            int k;
            bool binflag;
            const char* pFilename;

            i = 0;
            inFileSeq = 0;

            /* read meta file, validate file names and copy to table */
            while (1)
            {
                i++;

                /* exit while when EOF reached on META file */
                if (!fgets( buf, FILENAME_MAX + 5, inMeta ))
                    break;

                /* replace newline character if present */
                k = (int) strlen( buf );
                if (k >= 1 && buf[k-1] == '\n')
                    buf[k-1] = 0;

                /* Check for optional BIN flag */
                if (str_caseless_eq_n( buf, "BIN ", 4 ))
                {
                    pFilename = &buf[4];
                    binflag = true;
                }
                else
                {
                    pFilename = &buf[0];
                    binflag = binary;   // (default)
                }


                if (!(inData = fopen( pFilename, "r" )))
                {
                    // "Error opening %s file %i '%s': %s"
                    FWRMSG( stderr, HHC02773, "E", "included input", i, pFilename, strerror( errno ));
                    errors = true;
                }
                else
                {
                    fclose( inData );
                    inData = NULL;

                    if (inFileSeq >= MAXFILES)
                    {
                        // "number of files in %s exceeds %d; excess ignored"
                        FWRMSG( stderr, HHC02774, "W", pInFileID, MAXFILES );
                        break;
                    }
                    else
                    {
                        STRLCPY( inFileID[ inFileSeq ], pFilename );
                        inBINopt[ inFileSeq ] = binflag;
                        inFileSeq++;
                    }
                }
            } /* while (reading through meta file) */

            fclose( inMeta );
            inMeta = NULL;
            totfiles = inFileSeq;

        } /* else (meta file open succeeded) */
    } /* else (is a meta file) */

    if (!strlen( inFileID[0] ))
    {
        // "required %s omitted or not found"
        FWRMSG( stderr, HHC02775, "E", "INPUT: <input file name>" );
        errors = true;
    }

    if (!outFileID)
    {
        // "required %s omitted or not found"
        FWRMSG( stderr, HHC02775, "E", "OUTPUT: <output file name>" );
        errors = true;
    }

    if (!nltape && !pVolSer)
    {
        // "required %s omitted or not found"
        FWRMSG( stderr, HHC02775, "E", "VOLSER: <volume serial number>" );
        errors = true;
    }
    else if (!nltape)
    {
        if (!valid_volser( pVolSer ))
        {
            // "invalid %s parameter: %s"
            FWRMSG( stderr, HHC02781, "E", "VOLSER", pVolSer );
            errors = true;
        }
        else // (save and blank pad)
        {
            for (i=0; i < 6 && pVolSer[i]; i++)
                volSer[i] = pVolSer[i];
            for (; i < 6; i++)
                volSer[i] = ' ';
        }
    }
    else if (nltape && pVolSer)
    {
        // "Parameter %s ignored due to NLTAPE option"
        FWRMSG( stderr, HHC02783, "W", "VOLSER" );
    }

    if (!nltape && !datasetName)
    {
        // "required %s omitted or not found"
        FWRMSG( stderr, HHC02775, "E", "DATASET: <dataset name>" );
        errors = true;
    }
    else if (!nltape)
    {
        if (!valid_dataset( datasetName ))
        {
            // "invalid %s parameter: %s"
            FWRMSG( stderr, HHC02781, "E", "DATASET", datasetName );
            errors = true;
        }
    }
    else if (nltape && datasetName)
    {
        // "Parameter %s ignored due to NLTAPE option"
        FWRMSG( stderr, HHC02783, "W", "DATASET" );
    }

    /* Validate optional argument values... */

    if (nltape && ansi)
    {
        // "Parameter %s ignored due to NLTAPE option"
        FWRMSG( stderr, HHC02783, "W", "ANSI" );
    }

    if (pLRECL)
    {
        bool valid = true; int k = (int) strlen( pLRECL );
        for (i=0; i < k; i++)
            if (!(valid = valid_numeric( pLRECL[i] )))
                break;
        if (valid)
        {
            // 18 is the minimum tape blocksize.
            // anything less is considered noise.
            sscanf( pLRECL, "%5u", &lrecl );
            valid = (lrecl >= 18 && lrecl <= MAXLRECL);
        }
        if (!valid)
        {
            // "invalid %s parameter: %s"
            FWRMSG( stderr, HHC02781, "E", "LRECL", pLRECL );
            errors = true;
        }
    }

    if (pBLOCK)
    {
        bool valid = true; int k = (int) strlen( pBLOCK );
        for (i=0; i < k; i++)
            if (!(valid = valid_numeric( pBLOCK[i] )))
                break;
        if (valid)
        {
            sscanf( pBLOCK, "%5u", &blkfactor );
            valid = (blkfactor >= 1 && blkfactor <= MAXLRECL);
        }
        if (!valid)
        {
            // "invalid %s parameter: %s"
            FWRMSG( stderr, HHC02781, "E", "BLOCK", pBLOCK );
            errors = true;
        }
    }

    if (pCODEPAGE)
    {
        if (!valid_codepage_name( pCODEPAGE ))
        {
            // "invalid %s parameter: %s"
            FWRMSG( stderr, HHC02781, "E", "CODEPAGE", pCODEPAGE );
            errors = true;
        }
        else
            set_codepage( pCODEPAGE );
    }
    else
        set_codepage( NULL );

    /* Verify lrecl * blocking factor doesn't exceed maximum block size */

    if (((U64)lrecl * blkfactor) > (U64)MAXAWSBUFF)
    {
        // "LRECL %i and BLOCK %i exceeds maximum AWS blocksize of %i"
        FWRMSG( stderr, HHC02782, "E", lrecl, blkfactor, MAXAWSBUFF );
        errors = true;
    }

    if (errors)
        ErrExit( RC_ARG_ERRS );

} /* parseCommand */

/*-------------------------------------------------------------------------*/
/*    write data buffer to output file preceeded by AWS header block       */
/*-------------------------------------------------------------------------*/
static void writeBuffer( int bufferLen )
{
    /* set up fields in header block */
    awshdr.prvblkl[0] = awshdr.curblkl[0];
    awshdr.prvblkl[1] = awshdr.curblkl[1];

    awshdr.curblkl[0] = (bufferLen     ) & 0xFF;
    awshdr.curblkl[1] = (bufferLen >> 8) & 0xFF;

    awshdr.flags1 = AWSTAPE_FLAG1_NEWREC | AWSTAPE_FLAG1_ENDREC;
    awshdr.flags2 = 0;

    /* write header block to AWSTAPE image file */
    if (fwrite( &awshdr, 1, sizeof( awshdr ), outf ) != sizeof( awshdr ))
    {
        // "Error writing %s to file' %s': %s"
        FWRMSG( stderr, HHC02776, "E", "header", outFileID, strerror( errno ));
        ErrExit( RC_HDR_IOERR );
    }

    /* convert data in i/o buffer from ASCII to EBCDIC */
    if (!binary)
        buf_host_to_guest( (BYTE*)buf, (BYTE*)buf, bufferLen );

    /* write data block to AWSTAPE image file */
    if (fwrite( buf, 1, bufferLen, outf ) != (size_t)bufferLen)
    {
        // "Error writing %s to file' %s': %s"
        FWRMSG( stderr, HHC02776, "E", "data record", outFileID, strerror( errno ));
        ErrExit( RC_DATA_IOERR );
    }
}

/*-------------------------------------------------------------------------*/
/*          build standard labels and call writeBuffer to write            */
/*-------------------------------------------------------------------------*/
static void writeLabel( char ltype, char lseq )
{
char    dsn[ MAXTAPEDSNLEN + 1 ];  /* temporary dataset name variable      */
char    fseq[4+1];                 /* temporary file sequence variable     */
int     i, j;                      /* used to index through string vars    */

    /* clear buffer to spaces to build label */
    for (i=0; i < 80; i++)
        buf[i] = ' ';

    if (ltype == 'H')
        STRLCPY( buf, "HDR" );     /* HeaDeR label requested */
    else
        STRLCPY( buf, "EOF" );     /* EndOfFile label requested */

    buf[3] = lseq;                 /* insert passed number into label */

    /* copy dataset name parameter to work variable, at conclusion j will
       contain the index to the first blank past the end of the label text */
    STRLCPY( dsn, datasetName );
    for (i = j = 0; i < MAXTAPEDSNLEN; i++)
    {
        if (i && !j && !dsn[i])    /* first NULL reached yet? */
            j = i;                 /* save pos of first blank */
        if (j)                     /* pad? */
            dsn[i] = ' ';          /* yes */
    }
    dsn[ MAXTAPEDSNLEN ] = 0;      /* NULL terminate string */

    /* if each file is to be output discretely, manufacture unique dsn */
    if (outGLOBBING == outputUNIQUE)
    {
        /* copy input file sequence number to work variable */
        MSGBUF( fseq, "%4.4u", (inFileSeq+1));

        if (!j || j > 11)
        {
            dsn[11] = '.';
            dsn[12] = 'F';
            memcpy( dsn+13, fseq, 4 );
        }
        else
        {
            dsn[j] = '.'; j++;
            dsn[j] = 'F'; j++;
            memcpy( dsn+j, fseq, 4 );
        }
    }
    else
        STRLCPY( fseq, "0001" );         /* only 1 file on output tape */

    /* format label based upon passed sequence number */
    if (lseq == '1')                     /* -------- label #1 -------- */
    {
        memcpy( buf+ 4, dsn, MAXTAPEDSNLEN); /* dataset name          */
        memcpy( buf+21, volSer,  6);    /* volume serial number       */
        memcpy( buf+27, "0001",  4);    /* volume sequence number     */
        memcpy( buf+31, fseq,    4);    /* file number                */
        memcpy( buf+35, "    ",  4);    /* generation number          */
        memcpy( buf+39, "  ",    2);    /* version number             */
        memcpy( buf+42, julianToday, 5);/* creation date              */
        memcpy( buf+48, "99365", 5);    /* expiration date            */

        if (ansi)
            buf[53] = ' ';
        else
            buf[53] = '0';               /* security - 0 = none        */

        if (ltype == 'H')
            memcpy( buf+54, "000000", 6 ); /* no blk count in hdr */
        else
            snprintf( buf+54, 7, "%6.6i", blkCount );
    }
    else                                  /* ------- label #2 -------- */
    {
        buf[4] = 'F';                     /* record format - F = fixed */
        snprintf( buf+5,  6, "%5.5i", blkSize );
        snprintf( buf+10, 6, "%5.5i", lrecl );
        buf[15] = '3';                    /* density                   */
        buf[16] = '0';                    /* dataset position          */
        memcpy( buf+17, "HERCULES/MAKETAPE", 17 );/* job/step creating*/
        if (blkfactor != 1)
            buf[38] = 'B';                /* blocked records           */
    }

    /* ensure that label fields only contain upper case */
    for (i=0; i < 80; i++)
        buf[i] = toupper( (unsigned char)buf[i] );

    if (!nltape)
    {
        bool save = binary;
        binary = ansi;
        writeBuffer( 80 );
        binary = save;
    }

    /* show dataset info when last end label written for each file */
    if (ltype == 'E' && lseq == '2')
        // "Wrote %i blocks to AWSTAPE file: '%s' (Seq #%.4s Dataset:'%.17s')"
        WRMSG( HHC02778, "I", blkCount, outFileID, fseq, dsn );

} /* writeLabel */

/*-------------------------------------------------------------------------*/
/*                 write tape mark record to AWS image file                */
/*-------------------------------------------------------------------------*/
static void writeTapeMark()
{
    /* set fields in header block for tape mark */
    awshdr.prvblkl[0] = awshdr.curblkl[0];
    awshdr.prvblkl[1] = awshdr.curblkl[1];

    awshdr.curblkl[0] = 0;
    awshdr.curblkl[1] = 0;

    awshdr.flags1 = AWSTAPE_FLAG1_TAPEMARK;
    awshdr.flags2 = 0;

    /* write header block to AWSTAPE image file */
    if (fwrite( &awshdr, 1, sizeof( awshdr ), outf ) != sizeof( awshdr ))
    {
        // "Error writing %s to file' %s': %s"
        FWRMSG( stderr, HHC02776, "E", "tape mark", outFileID, strerror( errno ));
        ErrExit( RC_WTM_IOERR );
    }
}

/*-------------------------------------------------------------------------*/
/*                              MAIN                                       */
/*-------------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
char   *pgm;                       /* less any extension (.ext)            */
int     i;                         /* used to index string vars            */
int     offset;                    /* used for building blocked records    */
int     lastrec = 0;               /* Size of last record if binary        */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    // "%s (C) copyright Jay Moseley, CCP 2000"
    WRMSG( HHC02779, "I", pgm );

    /* validate and extract submitted parameter values */
    parseCommand( argc, argv );

    /* build julian date for standard labels */
    julianDate();

    /* open the AWSTAPE image file */
    if (!(outf = fopen( outFileID, "wb" )))
    {
        // "Error opening %s file '%s': %s"
        FWRMSG( stderr, HHC02772, "E", "output", outFileID, strerror( errno ));
        ErrExit( RC_OPEN_OUT );
    }

    /* build and write VOLume 1 label */
    if (!nltape)
    {
        bool save;

        for (i=0; i < 80; i++)
            buf[i] = ' ';

        STRLCPY( buf, "VOL1" );
        strncpy( buf+4, volSer, 6 );

        if (!ansi)
            buf[10] = '0';

        /* ensure that label fields only contain upper case */
        for (i=0; i < 80; i++)
            buf[i] = toupper( (unsigned char)buf[i] );

        if (ansi)
            buf[79] = '1';

        save = binary;
        {
            binary = ansi;
            writeBuffer( 80 );
        }
        binary = save;
    }

    EXTGUIMSG( "FILES=%d\n", totfiles );

    inFileSeq = 0;                   /* index of input files        */
    blkSize = lrecl * blkfactor;     /* compute physical block size */
    offset = 0;                      /* intrablock pointer          */

    /* if single output file specified, write header labels */
    if ((outGLOBBING == outputGLOB) && !nltape)
    {
        writeLabel( 'H', '1' );
        writeLabel( 'H', '2' );
        writeTapeMark();
    }

    /* loop for processing all specified input files */
    while (1)
    {
        EXTGUIMSG( "FILE=%d\n", inFileSeq+1 );

        // "Processing input from: '%s'"
        WRMSG( HHC02777, "I", inFileID[ inFileSeq ]);
        binary = inBINopt[ inFileSeq ];
        mode = binary ? "rb" : "r";

        if (!(inData = fopen( inFileID[ inFileSeq ], mode )))
        {
            // "Error opening %s file %i '%s': %s"
            FWRMSG( stderr, HHC02773, "E", "input", inFileSeq, inFileID[ inFileSeq ], strerror( errno ));
            ErrExit( RC_OPEN_INPUT );
        }

        /* if multiple output files specified, write header labels */
        if (!nltape && (outGLOBBING == outputUNIQUE))
        {
            writeLabel( 'H', '1' );
            writeLabel( 'H', '2' );
            writeTapeMark();
        }

        /* loop for processing records from current input file */
        while (1)
        {
            if (binary)
            {
                i = fread( &buf[ offset ], 1, lrecl, inData );

                if (i < 1)      /* (error or EOF) */
                {
                    if (ferror( inData ))
                    {
                        // "Error reading from input file' %s': %s"
                        FWRMSG( stderr, HHC02784, "E", inFileID[ inFileSeq ], strerror( errno ));
                        ErrExit( RC_READ_IOERR );
                    }
                    break; /* (normal EOF) */
                }

                /* Last/final input record? */
                if (i < lrecl)
                    lastrec = i;
            }
            else
            {
                /* exit inner loop when EOF reached on data file */
                if (!fgets( &buf[ offset ], MAXLRECL, inData ))
                {
                    if (ferror( inData ))
                    {
                        // "Error reading from input file' %s': %s"
                        FWRMSG( stderr, HHC02784, "E", inFileID[ inFileSeq ], strerror( errno ));
                        ErrExit( RC_READ_IOERR );
                    }
                    break; /* (normal EOF) */
                }

                /* copy i/p to o/p buffer until newline or LRECL reached */
                for (i=0; i < lrecl && buf[offset+i] != '\n'; i++);

                /* if i/p record length < LRECL, pad o/p buffer w/spaces */
                for ( ; i < lrecl ; i++)
                    buf[ offset+i ] = ' ';
            }

            /* update block pointer and check for end of block */
            offset += lastrec ? lastrec : lrecl;

            if (lastrec || offset >= blkSize)
            {
                writeBuffer( offset );
                blkCount++;
                offset = 0;
            }

            lastrec = 0;

        } /* while (processing current input data file) */

        fclose( inData );              /* close input data file */

        /* write trailing labels for multiple output files */
        if (outGLOBBING == outputUNIQUE)
        {
            /* if partial block at end of input data file, write it */
            if (offset > 0)
            {
                writeBuffer( offset );
                blkCount++;
                offset = 0;
            }

            if (!nltape)
            {
                writeTapeMark();
                writeLabel( 'E', '1' );
                writeLabel( 'E', '2' );
            }

            writeTapeMark();
            blkCount = 0;
        } /* if (writing separate files) */

        inFileSeq++;    /* increment to next input file ID */

        /* exit while if file ID blank or max input files processed */
        if (0
            || inFileSeq >= MAXFILES
            || !strlen( inFileID[ inFileSeq ])
        )
            break;

    } /* while (loop processing all input data files */

    /* if partial block at end of last input file, write it */
    if (offset > 0)
    {
        writeBuffer( offset );
        blkCount++;
    }

    /* write trailing labels for single output file */
    if (outGLOBBING == outputGLOB)
    {
        writeTapeMark();

        if (!nltape)
        {
            writeLabel( 'E', '1' );
            writeLabel( 'E', '2' );
            writeTapeMark();
        }
    } /* if (writing single output file) */

    /* write closing tape mark for end of volume indication */
    writeTapeMark();

    /* Close output file and exit */
    fclose( outf );

    /* Success! */
    return 0;

} /* end main */
