/* TAPECOPY.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              Convert SCSI tape into AWSTAPE format                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*              Read from AWSTAPE and write to SCSI tape mods        */
/*              Copyright 2005-2009 James R. Maynard III             */

/*-------------------------------------------------------------------*/
/* This program reads a SCSI tape and produces a disk file with      */
/* each block of the tape prefixed by an AWSTAPE block header.       */
/* If no disk file name is supplied, then the program simply         */
/* prints a summary of the tape files and blocksizes.                */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#include "hercules.h"
#include "tapedev.h"
#include "scsitape.h"

#define UTILITY_NAME    "tapecopy"
#define UTILITY_DESC    "Copy SCSI tape to/from AWS tape file"

char* pgm;

/*-------------------------------------------------------------------*/
/* (if no SCSI tape support generated, do nothing)                   */
/*-------------------------------------------------------------------*/
#if !defined(OPTION_SCSI_TAPE)
// SYSBLK sysblk;
int main (int argc, char *argv[])
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);
    // "SCSI tapes are not supported with this build"
    FWRMSG( stderr, HHC02700, "E" );
    return 0;
}
#else

/*-------------------------------------------------------------------*/
/* External GUI flag...                                              */
/*-------------------------------------------------------------------*/
time_t curr_progress_time = 0;
time_t prev_progress_time = 0;
#define PROGRESS_INTERVAL_SECS  (   3   )     /* (just what it says) */


/*-------------------------------------------------------------------*/
/* Return Codes...                                                   */
/*-------------------------------------------------------------------*/
#define  RC_SUCCESS                                ( 0)
#define  RC_ERROR_BAD_ARGUMENTS                    ( 1)
#define  RC_ERROR_OPENING_SCSI_DEVICE              ( 3)
#define  RC_ERROR_OPENING_AWS_FILE                 ( 4)
#define  RC_ERROR_SETTING_SCSI_VARBLK_PROCESSING   ( 5)
#define  RC_ERROR_REWINDING_SCSI                   ( 6)
#define  RC_ERROR_OBTAINING_SCSI_STATUS            ( 7)
#define  RC_ERROR_READING_AWS_HEADER               ( 8)
#define  RC_ERROR_READING_DATA                     ( 9)
#define  RC_ERROR_AWSTAPE_BLOCK_TOO_LARGE          (10)
#define  RC_ERROR_WRITING_TAPEMARK                 (11)
#define  RC_ERROR_WRITING_OUTPUT_AWS_HEADER_BLOCK  (12)
#define  RC_ERROR_WRITING_DATA                     (13)

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
static BYTE vollbl[] = "\xE5\xD6\xD3";  /* EBCDIC characters "VOL"   */
static BYTE hdrlbl[] = "\xC8\xC4\xD9";  /* EBCDIC characters "HDR"   */
static BYTE eoflbl[] = "\xC5\xD6\xC6";  /* EBCDIC characters "EOF"   */
static BYTE eovlbl[] = "\xC5\xD6\xE5";  /* EBCDIC characters "EOV"   */

static struct mt_tape_info tapeinfo[] = MT_TAPE_INFO;

static struct mt_tape_info densinfo[] =
{
    {0x01, "NRZI (800 bpi)"              },
    {0x02, "PE (1600 bpi)"               },
    {0x03, "GCR (6250 bpi)"              },
    {0x05, "QIC-45/60 (GCR, 8000 bpi)"   },
    {0x06, "PE (3200 bpi)"               },
    {0x07, "IMFM (6400 bpi)"             },
    {0x08, "GCR (8000 bpi)"              },
    {0x09, "GCR (37871 bpi)"             },
    {0x0A, "MFM (6667 bpi)"              },
    {0x0B, "PE (1600 bpi)"               },
    {0x0C, "GCR (12960 bpi)"             },
    {0x0D, "GCR (25380 bpi)"             },
    {0x0F, "QIC-120 (GCR 10000 bpi)"     },
    {0x10, "QIC-150/250 (GCR 10000 bpi)" },
    {0x11, "QIC-320/525 (GCR 16000 bpi)" },
    {0x12, "QIC-1350 (RLL 51667 bpi)"    },
    {0x13, "DDS (61000 bpi)"             },
    {0x14, "EXB-8200 (RLL 43245 bpi)"    },
    {0x15, "EXB-8500 (RLL 45434 bpi)"    },
    {0x16, "MFM 10000 bpi"               },
    {0x17, "MFM 42500 bpi"               },
    {0x24, "DDS-2"                       },
    {0x8C, "EXB-8505 compressed"         },
    {0x90, "EXB-8205 compressed"         },
    {0,     NULL                         },
};

/*-------------------------------------------------------------------*/
/* Maximum sized tape I/O buffer...                                  */
/*-------------------------------------------------------------------*/
static BYTE buf[ MAX_TAPE_BLKSIZE ];

/*-------------------------------------------------------------------*/
/* Global variables used by main and the read/write functions        */
/*-------------------------------------------------------------------*/
int             len;                    /* Block length              */
int             prevlen;                /* Previous block length     */
int64_t         bytes_written;          /* Bytes written to o/p file */
char           *devnamein;              /* -> Input tape device name */
char           *devnameout;             /* -> Output tape device name*/
char           *filenamein;             /* -> Input AWS file name    */
char           *filenameout;            /* -> Output AWS file name   */

/*-------------------------------------------------------------------*/
/* Custom exit function...                                           */
/*-------------------------------------------------------------------*/
static void delayed_exit (int exit_code)
{
    if (RC_SUCCESS != exit_code)
        // "Abnormal termination"
        FWRMSG( stderr, HHC02701, "E" );

    /* Delay exiting is to give the system
     * time to display the error message. */
    USLEEP(100000);
    exit(exit_code);
}
#define DELAYED_EXIT(rc)  delayed_exit(rc) /* use this macro to exit */

/*-------------------------------------------------------------------*/
/* Subroutine to print tape status                                   */
/*-------------------------------------------------------------------*/
static void print_status (char *devname, long stat)
{
    char buffer[ 384 ] = {0};

    // "Tape %s: %smt_gstat 0x%8.8"PRIX32" %s"
    WRMSG( HHC02702, "I", devname, "",
        (U32) stat, gstat2str( (U32) stat, buffer, sizeof( buffer )));

} /* end function print_status */

/*-------------------------------------------------------------------*/
/* Subroutine to print usage message                                 */
/*-------------------------------------------------------------------*/
static int print_usage()
{
    // "HHC02760I Usage..."

#define MSGNUM  "HHC02760I "

    char usage[4096];

#if defined( _MSVC_ )
    char* devname = "/dev\" or \"\\\\.\\Tape";
#else
    char* devname = "/dev";
#endif

    MSGBUF( usage,

//       1...5...10...15...20...25...30...35...40...45...50...55...60...65...70...75...80
        "Usage:\n"
        MSGNUM "\n"
        MSGNUM "   %s  [tapedrive] [awsfile] or\n"
        MSGNUM "   %s  [awsfile] [tapedrive]\n"
        MSGNUM "\n"
        MSGNUM "Where:\n"
        MSGNUM "\n"
        MSGNUM "   tapedrive    specifies the device filename of the SCSI tape drive.\n"
        MSGNUM "                Must begin with \"%s\" to be recognized.\n"
        MSGNUM "   awsfile      specifies the filename of the AWS emulated tape file.\n"
        MSGNUM "\n"
        MSGNUM "The first filename is the input; the second is the output.\n"
        MSGNUM "\n"
        MSGNUM "If the input file is a SCSI tape, it is read and processed until physical EOD\n"
        MSGNUM "(end-of-data) is reached (i.e. it does not stop whenever multiple tapemarks or\n"
        MSGNUM "filemarks are read; it continues processing until the SCSI tape drive says\n"
        MSGNUM "there is no more data on the tape). The resulting AWS eumulated tape O/P file,\n"
        MSGNUM "when specified for the filename on a Hercules tape device configuration\n"
        MSGNUM "statement, can then be used instead in order for the Hercules guest O/S to\n"
        MSGNUM "read the exact same data without having to have a SCSI tape drive physically\n"
        MSGNUM "attached to the host system. This allows you to easily transfer SCSI tape data\n"
        MSGNUM "to other systems that may not have SCSI tape drives attached to them by simply\n"
        MSGNUM "using the AWS emulated tape file instead, or allows systems without SCSI tape\n"
        MSGNUM "drives to create tape output (in the form of an AWS tape file) which can then\n"
        MSGNUM "be copied to a SCSI tape (via %s) by a system which DOES have one.\n"
        MSGNUM "\n"
        MSGNUM "The possible return codes and their meaning are:\n"
        MSGNUM "\n"
        MSGNUM "   %2d           Successful completion.\n"
        MSGNUM "   %2d           Invalid arguments or no arguments given.\n"
        MSGNUM "   %2d           Unable to open SCSI tape drive device file.\n"
        MSGNUM "   %2d           Unable to open AWS emulated tape file.\n"
        MSGNUM "   %2d           Unrecoverable I/O error setting variable length block\n"
        MSGNUM "                processing for SCSI tape device.\n"
        MSGNUM "   %2d           Unrecoverable I/O error rewinding SCSI tape device.\n"
        MSGNUM "   %2d           Unrecoverable I/O error obtaining status of SCSI device.\n"
        MSGNUM "   %2d           Unrecoverable I/O error reading block header\n"
        MSGNUM "                from AWS emulated tape file.\n"
        MSGNUM "   %2d           Unrecoverable I/O error reading data block.\n"
        MSGNUM "   %2d           AWS emulated tape block size too large.\n"
        MSGNUM "   %2d           Unrecoverable I/O error writing tapemark.\n"
        MSGNUM "   %2d           Unrecoverable I/O error writing block header\n"
        MSGNUM "                to AWS emulated tape file.\n"
        MSGNUM "   %2d           Unrecoverable I/O error writing data block.\n"

        ,pgm
        ,pgm
        ,devname
        ,pgm
        ,RC_SUCCESS
        ,RC_ERROR_BAD_ARGUMENTS
        ,RC_ERROR_OPENING_SCSI_DEVICE
        ,RC_ERROR_OPENING_AWS_FILE
        ,RC_ERROR_SETTING_SCSI_VARBLK_PROCESSING

        ,RC_ERROR_REWINDING_SCSI
        ,RC_ERROR_OBTAINING_SCSI_STATUS
        ,RC_ERROR_READING_AWS_HEADER
        ,RC_ERROR_READING_DATA
        ,RC_ERROR_AWSTAPE_BLOCK_TOO_LARGE
        ,RC_ERROR_WRITING_TAPEMARK

        ,RC_ERROR_WRITING_OUTPUT_AWS_HEADER_BLOCK

        ,RC_ERROR_WRITING_DATA
    );

    WRMSG( HHC02760, "I", usage );

    return RC_ERROR_BAD_ARGUMENTS;

} /* end function print_usage */

/*-------------------------------------------------------------------*/
/* Subroutine to obtain SCSI tape status...                          */
/*                                                                   */
/* Return value:     0  ==  normal                                   */
/*                  +1  ==  end-of-tape                              */
/*                  -1  ==  error                                    */
/*-------------------------------------------------------------------*/
static int obtain_status (char *devname, int devfd, struct mtget* mtget)
{
int rc;                                 /* Return code               */

    rc = ioctl_tape (devfd, MTIOCGET, (char*)mtget);
    if (rc < 0)
    {
        if (1
            && EIO == errno
            && (0
                || GMT_EOD( mtget->mt_gstat )
                || GMT_EOT( mtget->mt_gstat )
            )
        )
            return +1;

        // "Tape %s: Error reading status: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02703, "E", devname, rc, errno, strerror( errno ));
        return -1;
    }

    if (GMT_EOD( mtget->mt_gstat ) ||
        GMT_EOT( mtget->mt_gstat ))
        return +1;

    return 0;
} /* end function obtain_status */

/*-------------------------------------------------------------------*/
/* Read a block from SCSI tape                                       */
/*-------------------------------------------------------------------*/
int read_scsi_tape (int devfd, void *buf, size_t bufsize, struct mtget* mtget)
{
    int rc;
    int save_errno;

    len = read_tape (devfd, buf, bufsize);
    if (len < 0)
    {
        /* Determine whether end-of-tape has been read */
        save_errno = errno;
        ASSERT( devnamein );
        rc = obtain_status (devnamein, devfd, mtget);
        if (rc == +1)
        {
            // "End of tape"
            WRMSG( HHC02704, "I" );
            errno = save_errno;
            return(-1);
        }
        // "Tape %s: Error reading tape: errno=%d: %s"
        FWRMSG( stderr, HHC02705, "E", devnamein, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_READING_DATA );
    }

    return(len);
} /* end function read_scsi_tape */

/*-------------------------------------------------------------------*/
/* Read a block from AWSTAPE disk file                               */
/*-------------------------------------------------------------------*/
int read_aws_disk (int diskfd, void *buf, size_t bufsize)
{
    AWSTAPE_BLKHDR  awshdr;                 /* AWSTAPE block header      */
    int             rc;
    unsigned int    count_read = 0;
    unsigned int    blksize;
    int             end_block;
    BYTE           *bufptr = buf;

    while (1)
    {
        /* Read block header */
        rc = read (diskfd, &awshdr, sizeof(AWSTAPE_BLKHDR));
        if (rc == 0)
        {
            // "File %s: End of %s input"
            WRMSG( HHC02706, "I", filenamein, "AWS emulated tape" );
            return (-1);
        }
        if (rc < (int)sizeof(AWSTAPE_BLKHDR))
        {
            // "File %s: Error reading %s header: rc=%d, errno=%d: %s"
            FWRMSG( stderr, HHC02707, "E", filenamein, "AWS eumulated tape file",
                rc, errno, strerror( errno ));
            DELAYED_EXIT( RC_ERROR_READING_AWS_HEADER );
        } /* end if(rc) */

        /* Interpret the block header */
        blksize = ((int)awshdr.curblkl[1] << 8) + awshdr.curblkl[0];
        end_block = (awshdr.flags1 & AWSTAPE_FLAG1_ENDREC) != 0;

        /* If this is a tapemark, return immediately */
        if (blksize == 0)
            return (0);

        /* Check maximum block length */
        if ((count_read + blksize) > bufsize)
        {
            // "File %s: Block too large for %s tape: block size=%d, maximum=%d"
            FWRMSG( stderr, HHC02708, "E", filenamein, "AWS emulated",
                count_read+blksize, (int)bufsize );
            DELAYED_EXIT( RC_ERROR_AWSTAPE_BLOCK_TOO_LARGE );
        } /* end if(count) */

        /* Read data block */
        rc = read (diskfd, bufptr, blksize);
        if (rc < (int)blksize)
        {
            // "File %s: Error reading %s data block: rc=%d, errno=%d: %s"
            FWRMSG( stderr, HHC02709, "E", filenamein, "AWS emulated tape",
                rc, errno, strerror( errno ));
            DELAYED_EXIT( RC_ERROR_READING_DATA );
        } /* end if(rc) */

        bufptr += blksize;
        count_read += blksize;
        if (end_block)
            break;
    }

    return(count_read);

} /* end function read_aws_disk */

/*-------------------------------------------------------------------*/
/* Write a block to SCSI tape                                        */
/*-------------------------------------------------------------------*/
int write_scsi_tape (int devfd, void *buf, size_t len)
{
    int                 rc;

    rc = write_tape (devfd, buf, len);
    if (rc < (int)len)
    {
        // "Tape %s: Error writing data block: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02710, "E", devnameout,
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_WRITING_DATA );
    } /* end if(rc) */

    bytes_written += rc;

    return(rc);

} /* end function write_scsi_tape */

/*-------------------------------------------------------------------*/
/* Write a block to AWSTAPE disk file                                */
/*-------------------------------------------------------------------*/
int write_aws_disk (int diskfd, void *buf, size_t len)
{
    AWSTAPE_BLKHDR  awshdr;             /* AWSTAPE block header      */
    int             rc;

    /* Build the block header */
    awshdr.curblkl[0] =   len            & 0xFF;
    awshdr.curblkl[1] = ( len     >> 8 ) & 0xFF;
    awshdr.prvblkl[0] =   prevlen        & 0xFF;
    awshdr.prvblkl[1] = ( prevlen >> 8 ) & 0xFF;
    awshdr.flags1     = 0
                        | AWSTAPE_FLAG1_NEWREC
                        | AWSTAPE_FLAG1_ENDREC
                        ;
    awshdr.flags2     = 0;

    /* Write block header to output file */
    rc = write (diskfd, &awshdr, sizeof(AWSTAPE_BLKHDR));
    if (rc < (int)sizeof(AWSTAPE_BLKHDR))
    {
        // "File %s: Error writing %s header: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02711, "E", filenameout, "AWS emulated tape file",
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_WRITING_OUTPUT_AWS_HEADER_BLOCK );
    } /* end if(rc) */

    bytes_written += rc;

    /* Write data block to output file */
#if defined( _MSVC_ )
    rc = write (diskfd, buf, (u_int)len);
#else
    rc = write (diskfd, buf, len);
#endif
    if (rc < (int)len)
    {
        // "File %s: Error writing %s data block: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02712, "E", filenameout, "AWS emulated tape",
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_WRITING_DATA );
    } /* end if(rc) */

    bytes_written += rc;

    return(rc);
} /* end function write_aws_disk */

/*-------------------------------------------------------------------*/
/* Write a tapemark to SCSI tape                                     */
/*-------------------------------------------------------------------*/
int write_tapemark_scsi_tape (int devfd)
{
    struct mtop     opblk;              /* Area for MTIOCTOP ioctl   */
    int             rc;

    opblk.mt_op = MTWEOF;
    opblk.mt_count = 1;
    rc = ioctl_tape (devfd, MTIOCTOP, (char*)&opblk);
    if (rc < 0)
    {
        // "Tape %s: Error writing tapemark: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02713, "E", devnameout,
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_WRITING_TAPEMARK );
    }
    return(rc);

} /* end function write_tapemark_scsi_tape */

/*-------------------------------------------------------------------*/
/* Write a tapemark to AWSTAPE disk file                             */
/*-------------------------------------------------------------------*/
int write_tapemark_aws_disk (int diskfd)
{
    AWSTAPE_BLKHDR  awshdr;             /* AWSTAPE block header      */
    int             rc;

    /* Build block header for tape mark */
    awshdr.curblkl[0] = 0;
    awshdr.curblkl[1] = 0;
    awshdr.prvblkl[0] =   prevlen        & 0xFF;
    awshdr.prvblkl[1] = ( prevlen >> 8 ) & 0xFF;
    awshdr.flags1     = AWSTAPE_FLAG1_TAPEMARK;
    awshdr.flags2     = 0;

    /* Write block header to output file */
    rc = write (diskfd, &awshdr, sizeof(AWSTAPE_BLKHDR));
    if (rc < (int)sizeof(AWSTAPE_BLKHDR))
    {
        // "File %s: Error writing %s tapemark: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02714, "E", filenameout, "AWS emulated",
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_WRITING_TAPEMARK );
    } /* end if(rc) */

    bytes_written += rc;
    return(rc);
} /* end function write_tapemark_aws_disk */

/*-------------------------------------------------------------------*/
/* TAPECOPY main entry point                                         */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
int             devfd;                  /* Tape file descriptor      */
int             diskfd = -1;            /* Disk file descriptor      */
int             fileno;                 /* Tape file number          */
int             blkcount;               /* Block count               */
int             totalblks = 0;          /* Block count               */
int             minblksz;               /* Minimum block size        */
int             maxblksz;               /* Maximum block size        */
struct mtop     opblk;                  /* Area for MTIOCTOP ioctl   */
long            density;                /* Tape density code         */
BYTE            labelrec[81];           /* Standard label (ASCIIZ)   */
int64_t         bytes_read;             /* Bytes read from i/p file  */
int64_t         file_bytes;             /* Byte count for curr file  */
char            pathname[MAX_PATH];     /* file name in host format  */
struct mtget    mtget;                  /* Area for MTIOCGET ioctl   */
struct mtpos    mtpos;                  /* Area for MTIOCPOS ioctl   */
int             is3590 = 0;             /* 1 == 3590, 0 == 3480/3490 */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* The first argument is the input file name
       (either AWS disk file or SCSI tape device)
    */
    if ((argc < 2) || (argv[1] == NULL))
        DELAYED_EXIT( print_usage());

    if (0
        || ( strlen( argv[1] ) > 5 && strnfilenamecmp( argv[1], "/dev/",   5 ) == 0 )
#if defined(_MSVC_)
        || ( strlen( argv[1] ) > 5 && strnfilenamecmp( argv[1], "\\dev\\", 5 ) == 0 )
#endif
        || ( strlen( argv[1] ) > 4 && strnfilenamecmp( argv[1], "\\\\.\\", 4 ) == 0 )
    )
    {
        devnamein = argv[1];
        filenamein = NULL;
    }
    else
    {
        filenamein = argv[1];
        devnamein = NULL;
    }

    /* The second argument is the output file name
       (either AWS disk file or SCSI tape device)
    */
    if (argc > 2 && argv[2] )
    {
        if (0
            || ( strlen( argv[2] ) > 5 && strnfilenamecmp( argv[2], "/dev/",   5 ) == 0 )
#if defined(_MSVC_)
            || ( strlen( argv[2] ) > 5 && strnfilenamecmp( argv[2], "\\dev\\",   5 ) == 0 )
#endif
            || ( strlen( argv[2] ) > 4 && strnfilenamecmp( argv[2], "\\\\.\\", 4 ) == 0 )
        )
        {
            devnameout = argv[2];
            filenameout = NULL;
        }
        else
        {
            filenameout = argv[2];
            devnameout = NULL;
        }
    }
    else
        DELAYED_EXIT( print_usage());

    /* Check input arguments and disallow tape-to-tape or disk-to-disk copy */
    if ((!devnamein && !devnameout) || (!filenamein && !filenameout))
        DELAYED_EXIT( print_usage());

    /* Open the SCSI tape device */
    if (devnamein)
    {
        hostpath( pathname, devnamein, sizeof(pathname) );
        devfd = open_tape (pathname, O_RDONLY|O_BINARY);
    }
    else // (devnameout)
    {
        hostpath( pathname, devnameout, sizeof(pathname) );
        devfd = open_tape (pathname, O_RDWR|O_BINARY);
    }
    if (devfd < 0)
    {
        // "Tape %s: Error opening: errno=%d: %s"
        FWRMSG( stderr, HHC02715, "E", (devnamein ? devnamein : devnameout),
            errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_OPENING_SCSI_DEVICE );
    }

    USLEEP(50000);

    /* Set the tape device to process variable length blocks */
    opblk.mt_op = MTSETBLK;
    opblk.mt_count = 0;
    rc = ioctl_tape (devfd, MTIOCTOP, (char*)&opblk);
    if (rc < 0)
    {
        // "Tape %s: Error setting attributes: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02716, "E", (devnamein ? devnamein : devnameout),
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_SETTING_SCSI_VARBLK_PROCESSING );
    }

    USLEEP(50000);

    /* Rewind the tape to the beginning */
    opblk.mt_op = MTREW;
    opblk.mt_count = 1;
    rc = ioctl_tape (devfd, MTIOCTOP, (char*)&opblk);
    if (rc < 0)
    {
        // "Tape %s: Error rewinding: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02717, "E", (devnamein ? devnamein : devnameout),
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_REWINDING_SCSI );
    }

    USLEEP(50000);

    /* Obtain the tape status */
    rc = obtain_status ((devnamein ? devnamein : devnameout), devfd, &mtget);
    if (rc < 0)
        DELAYED_EXIT( RC_ERROR_OBTAINING_SCSI_STATUS );

    /* Display tape status information */
    for (i = 0; tapeinfo[i].t_type != 0
                && tapeinfo[i].t_type != mtget.mt_type; i++)
        /* empty */ ;

    {
        char msgbuf[64];

        if (tapeinfo[i].t_name)
            MSGBUF( msgbuf, ": %s", tapeinfo[i].t_name);
        else
            MSGBUF( msgbuf, " code: 0x%lX", mtget.mt_type);

        // "Tape %s: Device type%s"
        WRMSG( HHC02718, "I", (devnamein ? devnamein : devnameout), msgbuf );
    }

    density = (mtget.mt_dsreg & MT_ST_DENSITY_MASK)
                >> MT_ST_DENSITY_SHIFT;

    for (i = 0; densinfo[i].t_type != 0
                && densinfo[i].t_type != density; i++)
         /* empty */ ;

    {
        char msgbuf[64];

        if (densinfo[i].t_name)
            MSGBUF( msgbuf, ": %s", densinfo[i].t_name );
        else
            MSGBUF( msgbuf, " code: 0x%lX", density );

        // "Tape %s: Device density%s"
        WRMSG( HHC02719, "I", (devnamein ? devnamein : devnameout), msgbuf );
    }

    if (mtget.mt_gstat != 0)
        print_status ((devnamein ? devnamein : devnameout), mtget.mt_gstat);

    /* Open the disk file */
    if (filenamein)
    {
        hostpath( pathname, filenamein, sizeof(pathname) );
        diskfd = HOPEN (pathname, O_RDONLY | O_BINARY);
    }
    else
    {
        hostpath( pathname, filenameout, sizeof(pathname) );
        diskfd = HOPEN (pathname, O_WRONLY | O_CREAT | O_BINARY,
                        S_IRUSR | S_IWUSR | S_IRGRP);
    }
    if (diskfd < 0)
    {
        // "File %s: Error opening: errno=%d: %s"
        FWRMSG( stderr, HHC02720, "E", (filenamein ? filenamein : filenameout),
            errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_OPENING_AWS_FILE );
    }

    /* Copy blocks from input to output */
    fileno = 1;
    blkcount = 0;
    totalblks = 0;
    minblksz = 0;
    maxblksz = 0;
    len = 0;
    bytes_read = 0;
    bytes_written = 0;
    file_bytes = 0;

    // Notify the GUI of the high-end of the copy-progress range...
    if ( extgui )
    {
        // Retrieve BOT block-id...
        VERIFY( 0 == ioctl_tape( devfd, MTIOCPOS, (char*)&mtpos ) );

        is3590 = ((mtpos.mt_blkno & 0x7F000000) != 0x01000000) ? 1 : 0;

        if (!is3590)
        {
            // The seg# portion the SCSI tape physical
            // block-id number values ranges from 1 to 95...
            EXTGUIMSG( "BLKS=%d\n", 95 );
        }
        else
        {
            // FIXME: 3590s (e.g. Magstar) use 32-bit block addressing,
            // and thus its block-id does not contain a seg# value, so
            // we must use some other technique. For now, we'll simply
            // presume the last block on the tape is block# 0x003FFFFF
            // (just to keep things simple).

            EXTGUIMSG( "BLKS=%d\n", 0x003FFFFF );
        }

        // Init time of last issued progress message
        prev_progress_time = time( NULL );
    }

    /* Perform the copy... */

    while (1)
    {
        /* Issue a progress message every few seconds... */
        if ( extgui )
        {
            if ( ( curr_progress_time = time( NULL ) ) >=
                ( prev_progress_time + PROGRESS_INTERVAL_SECS ) )
            {
                prev_progress_time = curr_progress_time;
                if ( ioctl_tape( devfd, MTIOCPOS, (char*)&mtpos ) == 0 )
                {
                    if (!is3590)
                        EXTGUIMSG( "BLK=%ld\n", (mtpos.mt_blkno >> 24) & 0x0000007F );
                    else
                        EXTGUIMSG( "BLK=%ld\n", mtpos.mt_blkno );
                }
            }
        }

        /* Save previous block length */
        prevlen = len;

        /* Read a block */
        if (devnamein)
            len = read_scsi_tape(devfd, buf, sizeof(buf), &mtget);
        else
            len = read_aws_disk(diskfd, buf, sizeof(buf));

        /* If returned with -1, end of tape; errors are handled by the
            read functions themselves */
        if (len < 0)
            break;

        /* Check for tape mark */
        if (len == 0)
        {
            /* Write tape mark to output file */
            if (filenameout)
                write_tapemark_aws_disk(diskfd);
            else
                write_tapemark_scsi_tape(devfd);

            /* Print summary of current file */
            if (blkcount)
            {
                ASSERT( file_bytes ); // (sanity check)

                // "File No. %u: Blocks=%u, Bytes=%"PRId64", Block size min=%u, max=%u, avg=%u"
                WRMSG( HHC02721, "I", fileno, blkcount, file_bytes, minblksz, maxblksz, (int)file_bytes/blkcount );
            }
            else
            {
                ASSERT( !file_bytes ); // (sanity check)
            }

            /* Show the 'tapemark' AFTER the above file summary since
               that's the actual physical sequence of events; i.e. the
               file data came first THEN it was followed by a tapemark
            */
            WRMSG( HHC02731, "I" );     // "(tapemark)"

            /* Reset counters for next file */
            if (blkcount)
                fileno++;
            minblksz = 0;
            maxblksz = 0;
            blkcount = 0;
            file_bytes = 0;
            continue;
        }

        /* Count blocks and block sizes */
        blkcount++;
        totalblks++;
        bytes_read += len;
        file_bytes += len;
        if (len > maxblksz) maxblksz = len;
        if (minblksz == 0 || len < minblksz) minblksz = len;

        /* Print standard labels */
        if (1
            && blkcount < 4
            && len == 80
            && (0
                || memcmp( buf, vollbl, 3 ) == 0
                || memcmp( buf, hdrlbl, 3 ) == 0
                || memcmp( buf, eoflbl, 3 ) == 0
                || memcmp( buf, eovlbl, 3 ) == 0
               )
        )
        {
            for (i=0; i < 80; i++)
                labelrec[i] = guest_to_host(buf[i]);
            labelrec[i] = '\0';
            // "Tape Label: %s"
            WRMSG( HHC02722, "I", labelrec );
        }
        else
        {
            ASSERT(blkcount);

            if (!extgui)
            {
                char msgbuf[100];
                // "File No. %u: Block %u"
                MSGBUF( msgbuf, MSG_C( HHC02723, "I", fileno, blkcount ));
                printf( "%s\r", msgbuf );
            }
        }

        /* Write block to output file */
        if (filenameout)
            write_aws_disk(diskfd, buf, len);
        else
            write_scsi_tape(devfd, buf, len);

    } /* end while */

    /* Print run totals, close files, and exit... */

    // "Successful completion"
    WRMSG( HHC02724, "I" );

    // "Bytes read:    %"PRId64" (%3.1f MB), Blocks=%u, avg=%u"
    WRMSG( HHC02732, "I",
        bytes_read,
        (double) (bytes_read + HALF_MEGABYTE) / (double) ONE_MEGABYTE,
        totalblks,
        totalblks ? (int) bytes_read/totalblks : -1 );

    // "Bytes written: %"PRId64" (%3.1f MB)"
    WRMSG( HHC02733, "I",
        bytes_written,
        (double) (bytes_written + HALF_MEGABYTE) / (double) ONE_MEGABYTE );

    close (diskfd);

    /* Rewind the tape back to the beginning again before exiting */

    opblk.mt_op = MTREW;
    opblk.mt_count = 1;

    rc = ioctl_tape (devfd, MTIOCTOP, (char*)&opblk);

    if (rc < 0)
    {
        // "Tape %s: Error rewinding: rc=%d, errno=%d: %s"
        FWRMSG( stderr, HHC02717, "E", (devnamein ? devnamein : devnameout),
            rc, errno, strerror( errno ));
        DELAYED_EXIT( RC_ERROR_REWINDING_SCSI );
    }

    close_tape (devfd);

    DELAYED_EXIT( RC_SUCCESS );
    UNREACHABLE_CODE( return RC_SUCCESS );

} /* end function main */

#endif /* defined(OPTION_SCSI_TAPE) */
