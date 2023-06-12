/* DMAP2HRC.C   (C) Copyright Jay Maynard, 2001-2012                 */
/*              Convert P/390 DEVMAP to Hercules config file         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program reads a P/390 DEVMAP file and extracts the device    */
/* definitions from it, then writes them to the standard output in   */
/* the format Hercules uses for its .cnf file.                       */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define UTILITY_NAME    "dmap2hrc"
#define UTILITY_DESC    "Convert P/390 DEVMAP to Hercules"

/*-------------------------------------------------------------------*/
/* Structure definition for DEVMAP controller record                 */
/*-------------------------------------------------------------------*/
typedef struct _DEVMAP_CTLR {
        BYTE    channel;                /* High order dev addr byte  */
        BYTE    name[8];                /* Name of controller program*/
        BYTE    lowdev;                 /* Low addr byte first dev   */
        BYTE    highdev;                /* Low addr byte last dev    */
        BYTE    filler1;                /* Fill byte                 */
        BYTE    type[4];                /* Type of controller        */
        BYTE    flags;                  /* Flag byte                 */
        BYTE    filler2[47];            /* More filler bytes         */
} DEVMAP_CTLR;

/*-------------------------------------------------------------------*/
/* Structure definition for DEVMAP device record                     */
/*-------------------------------------------------------------------*/
typedef struct _DEVMAP_DEV {
        BYTE    highaddr;               /* High order dev addr byte  */
        BYTE    lowaddr;                /* Low order dev addr byte   */
        char    type[4];                /* Type of device            */
    union {
        struct {                        /* Disk devices:             */
            BYTE filler1[4];            /* filler                    */
            BYTE volser[6];             /* Volume serial             */
            BYTE filler2[2];            /* more filler               */
            char filename[45];          /* name of file on disk      */
            BYTE flags;                 /* flag byte                 */
        } disk;
        struct {                        /* Other devices:            */
            BYTE filler1[7];            /* fill bytes                */
            char filename[50];          /* device filename           */
            BYTE flags;                 /* flag byte                 */
        } other;
    } parms;
} DEVMAP_DEV;

/*-------------------------------------------------------------------*/
/* DEVMAP2CNF main entry point                                       */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             i;                      /* Array subscript           */
int             len;                    /* Length of actual read     */
char           *filename;               /* -> Input file name        */
int             infd = -1;              /* Input file descriptor     */
DEVMAP_CTLR     controller;             /* Controller record         */
DEVMAP_DEV      device;                 /* Device record             */
char            output_type[5];         /* Device type to print      */
char           *output_filename;        /* -> filename to print      */
int             more_devices;           /* More devices this ctlr?   */
char            pathname[MAX_PATH];     /* file path in host format  */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* The only argument is the DEVMAP file name */
    if (argc == 2 && argv[1] != NULL)
    {
        filename = argv[1];
    }
    else
    {
        // "Usage: ..."
        WRMSG( HHC02645, "I", pgm );
        exit (1);
    }

    /* Open the devmap file */
    hostpath(pathname, filename, sizeof(pathname));
    infd = HOPEN (pathname, O_RDONLY | O_BINARY);
    if (infd < 0)
    {
        // "Error opening %s: %s"
        FWRMSG( stderr, HHC02646, "E", filename, strerror( errno ));
        exit (2);
    }

    /* Skip the file header */
    for (i = 0; i < 9; i++)
    {
        len = read (infd, (void *)&controller, sizeof(DEVMAP_CTLR));
        if (len < 0)
        {
            // "Error reading %s record%s from %s: %s"
            FWRMSG( stderr, HHC02647, "E", "header", "s", filename, strerror( errno ));
            exit (3);
        }
    }

    /* Read records from the input file and convert them */
    while (1)
    {
        /* Read a controller record. */
        len = read (infd, (void *)&controller, sizeof(DEVMAP_CTLR));
        if (len < 0)
        {
            // "Error reading %s record%s from %s: %s"
            FWRMSG( stderr, HHC02647, "E", "controller", "", filename, strerror( errno ));
            exit (4);
        }

        /* Did we finish too soon? */
        if ((len > 0) && (len < (int)sizeof(DEVMAP_CTLR)))
        {
            // "Incomplete %s record on %s"
            FWRMSG( stderr, HHC02648, "E", "controller", filename );
            exit(5);
        }

        /* Check for end of file. */
        if (len == 0)
        {
            // "End of input file."
            FWRMSG( stderr, HHC02649, "E" );
            break;
        }

        /* Read devices on this controller. */
        more_devices = 1;
        while (more_devices)
        {

            /* Read a device record. */
            len = read (infd, (void *)&device, sizeof(DEVMAP_DEV));
            if (len < 0)
            {
                // "Error reading %s record%s from %s: %s"
                FWRMSG( stderr, HHC02647, "E", "device", "", filename, strerror( errno ));
                exit (6);
            }

            /* Did we finish too soon? */
            if ((len > 0) && (len < (int)sizeof(DEVMAP_DEV)))
            {
                // "Incomplete %s record on %s"
                FWRMSG( stderr, HHC02648, "E", "device", filename );
                exit(7);
            }

            /* Check for end of file. */
            if (len == 0)
            {
                // Premature end of input file"
                FWRMSG( stderr, HHC02650, "E" );
                exit(8);
            }

            /* Is this the dummy device record at the end of the controller's
               set of devices? */
            if (strncmp(device.type,"    ",4) == 0)
            {
                more_devices = 0;
                break;
            }

            /* It's a real device. Fix the type so Hercules can use it and
               locate the output filename. */
            STRLCPY( output_type, device.type );
            if (isprint((unsigned char)device.parms.disk.volser[0]))
                output_filename = device.parms.disk.filename;
            else output_filename = device.parms.other.filename;

            if (strncmp(device.type, "3278", 4) == 0)
            {
                STRLCPY( output_type, "3270" );
                output_filename = "";
            }
            if (strncmp(device.type, "2540", 4) == 0)
                STRLCPY( output_type, "3505" );

            /* Emit the Hercules config file entry. */
            len = (int) strlen( output_filename );
            printf("%02X%02X    %s%s%s\n",
                device.highaddr,
                device.lowaddr,
                output_type,
                len ?          "    " : "",
                len ? output_filename : ""
            );

        } /* end while more_devices) */

    } /* end while (1) */

    /* Close files and exit */
    close (infd);

    return 0;

} /* end function main */
