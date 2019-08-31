/* DASDSER.C   (C) Copyright "Fish" (David B. Trout), 2019           */
/*              Hercules DASD Serial Number Utility                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program displays or writes (updates) the serial number of    */
/* an existing dasd image file.                                      */
/*                                                                   */
/*      dasdser  filename               (query)                      */
/*      dasdser  filename  serial       (update)                     */
/*                                                                   */
/* filename     is the base CKD dasd image file to be updated.       */
/*              Only base CKD dasd image types are supported         */
/*              (CKD, CKD64, CCKD and CCKD64). Shadow files and      */
/*              FBA image types are not supported.                   */
/*                                                                   */
/* [serial]     is the optional 12-digit numeric serial number       */
/*              to be assigned to the specified dasd image file.     */
/*              If not specified the current serial is reported.     */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "ccwarn.h"

#define UTILITY_NAME    "dasdser"
#define UTILITY_DESC    "DASD serial number utility"

static int retcode( int rc )
{
    exit( rc );     // (convenient place to set breakpoint)
}

/*-------------------------------------------------------------------*/
/* DASDSER program main entry point                                  */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
char*       pgm;                        /* Less any extension (.ext) */
BYTE        serial[12+1] = {0};         /* Serial number to assign   */
CKD_DEVHDR  devhdr;                     /* DASD Image Device Header  */
int         fd;                         /* File descriptor           */
int         rc;                         /* Return code               */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Display help if needed or requested */
    if (argc < 2 || argc > 3)
    {
        // "Usage: ..."
        FWRMSG( stderr, HHC03100, "I", pgm );
        return retcode( -1 );
    }

    /* New serial number to be assigned? */
    if (argc > 2)
    {
        /* Valid 12-digit numeric serial number? */
        if (0
            || !is_numeric( argv[2] )
            || strlen( argv[2] ) != sizeof( devhdr.dh_serial )
        )
        {
            // "Invalid %s \"%s\""
            FWRMSG( stderr, HHC03101, "E", "serial", argv[2] );
            return retcode( -1 );
        }

        /* Save serial number */
        memcpy( serial, argv[2], sizeof( devhdr.dh_serial ));
        serial[12] = 0;
    }

    /* Open the dasd image file */
    if ((fd = open( argv[1], O_BINARY | (argc > 2 ? O_RDWR : O_RDONLY))) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC03102, "E", "open()", strerror( errno ));
        return retcode( -1 );
    }

    /* Read the device header */
    if ((rc = read( fd, &devhdr, sizeof( devhdr ))) < (int) sizeof( devhdr ))
    {
        if (rc < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC03102, "E", "read()", strerror( errno ));
        }
        else
        {
            // "Unsupported dasd image file format"
            FWRMSG( stderr, HHC03103, "E" );
        }
        return retcode( -1 );
    }

    /* Normal (uncompressed) FBA types don't have device headers! */
    if (!is_dh_devid_typ( devhdr.dh_devid, CKD_CMP_OR_NML_TYP ))
    {
        // "Unsupported dasd image file format"
        FWRMSG( stderr, HHC03103, "E" );
        return retcode( -1 );
    }

    /* Set new serial number or display current serial number... */

    if (argc > 2) // (assign/update/set?)
    {
        /* Seek back to beginning of file where the device header is */
        if (lseek( fd, 0, SEEK_SET ) != 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC03102, "E", "lseek()", strerror( errno ));
            return retcode( -1 );
        }

        /* Assign/update/set new serial number */
        memcpy( devhdr.dh_serial, serial, sizeof( devhdr.dh_serial ));

        /* Rewrite the device header with the new serial number */
        if ((rc = write( fd, &devhdr, sizeof( devhdr ))) != sizeof( devhdr ))
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC03102, "E", "write()", strerror( errno ));
            return retcode( -1 );
        }

        // "%-14s set to %s"
        WRMSG( HHC02204, "I", "SERIAL NUMBER", (const char*) serial );
    }
    else /* Display existing serial number */
    {
        memcpy( serial, devhdr.dh_serial, sizeof( devhdr.dh_serial ));
        serial[12] = 0;

        // "%-14s: %s"
        WRMSG( HHC02203, "I", "SERIAL NUMBER", (const char*) serial );
    }

    /* Close the file */
    close( fd );

    /* DONE! */
    return retcode( 0 );
}
