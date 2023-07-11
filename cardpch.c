/* CARDPCH.C    (C) Copyright Roger Bowler, 1999-2010                */
/*              ESA/390 Card Punch Device Handler                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for emulated       */
/* System/370 card punch devices.                                    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#include "devtype.h"
#include "sockdev.h"

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define CARD_LENGTH     80
#define HEX40           ((BYTE)0x40)

/*-------------------------------------------------------------------*/
/* Subroutine to write data to the card punch                        */
/*-------------------------------------------------------------------*/
static void
write_buffer (DEVBLK *dev, BYTE *buf, int len, BYTE *unitstat)
{
int             rc;                     /* Return code               */

    if (dev->bs)    /* socket device? */
    {
        /* Write data to socket, check for error */
        if ((rc = write_socket( dev->fd, buf, len )) < len)
        {
            /* Close the connection */
            if (dev->fd != -1)
            {
                int fd = dev->fd;
                dev->fd = -1;
                close_socket( fd );
                // "%1d:%04X Card: client %s, IP %s disconnected from device %s"
                WRMSG( HHC01211, "I", LCSS_DEVNUM, dev->bs->clientname, dev->bs->clientip, dev->bs->spec );
            }

            /* Set unit check with intervention required */
            dev->sense[0] = SENSE_IR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
    }
    else
    {
        /* Write data to the output file */
        rc = write (dev->fd, buf, len);

        /* Equipment check if error writing to output file */
        if (rc < len)
        {
            // "%1d:%04X %s: error in function %s: %s"
            WRMSG( HHC01250, "E", LCSS_DEVNUM,
                   "Card", "write()", errno == 0 ? "incomplete"
                                                 : strerror( errno ));
            dev->sense[0] = SENSE_EC;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
    }

} /* end function write_buffer */

/*-------------------------------------------------------------------*/
/* Thread to monitor the sockdev remote punch spooler connection     */
/*-------------------------------------------------------------------*/
static void* spthread (void* arg)
{
    DEVBLK* dev = (DEVBLK*) arg;
    BYTE byte;
    fd_set readset, errorset;
    struct timeval tv;
    int rc, fd = dev->fd;           // (save original fd)

    /* Fix thread name */
    {
        char    thread_name[16];
        MSGBUF( thread_name, "spthread %1d:%04X", LCSS_DEVNUM );
        SET_THREAD_NAME( thread_name );
    }

    // Looooop...  until shutdown or disconnect...

    // PROGRAMMING NOTE: we do our select specifying an immediate
    // timeout to prevent our select from holding up (slowing down)
    // the device thread (which does the actual writing of data to
    // the client). The only purpose for our thread even existing
    // is to detect a severed connection (i.e. to detect when the
    // client disconnects)...

    while ( !sysblk.shutdown && dev->fd == fd )
    {
        if (dev->busy)
        {
            SLEEP(3);
            continue;
        }

        FD_ZERO( &readset );
        FD_ZERO( &errorset );

        FD_SET( fd, &readset );
        FD_SET( fd, &errorset );

        tv.tv_sec = 0;
        tv.tv_usec = 0;

        rc = select( fd+1, &readset, NULL, &errorset, &tv );

        if (rc < 0)
            break;

        if (rc == 0)
        {
            SLEEP(3);
            continue;
        }

        if (FD_ISSET( fd, &errorset ))
            break;

        // Read and ignore any data they send us...
        // Note: recv should complete immediately
        // as we know data is waiting to be read.

        ASSERT( FD_ISSET( fd, &readset ) );

        rc = recv( fd, &byte, sizeof(byte), 0 );

        if (rc <= 0)
            break;
    }

    OBTAIN_DEVLOCK( dev );
    {
        // PROGRAMMING NOTE: the following tells us whether we detected
        // the error or if the device thread already did. If the device
        // thread detected it while we were sleeping (and subsequently
        // closed the connection) then we don't need to do anything at
        // all; just exit. If we were the ones that detected the error
        // however, then we need to close the connection so the device
        // thread can learn of it...

        if (dev->fd == fd)
        {
            dev->fd = -1;
            close_socket( fd );
            // "%1d:%04X Card: client %s, IP %s disconnected from device %s"
            WRMSG( HHC01211, "I", LCSS_DEVNUM, dev->bs->clientname, dev->bs->clientip, dev->bs->spec );
        }
    }
    RELEASE_DEVLOCK( dev );

    return NULL;

} /* end function spthread */

/*-------------------------------------------------------------------*/
/* Sockdev "OnConnection" callback function                          */
/*-------------------------------------------------------------------*/
static int onconnect_callback (DEVBLK* dev)
{
    TID tid;
    int rc;

    rc = create_thread( &tid, DETACHED, spthread, dev, "spthread" );
    if (rc)
    {
        // "Error in function create_thread(): %s"
        WRMSG( HHC00102, "E", strerror( rc ) );
        return 0;
    }
    return 1;
}

// (forward reference)
static int open_punch( DEVBLK* dev );

/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
static int cardpch_init_handler( DEVBLK* dev, int argc, char* argv[] )
{
int     i;                              /* Array subscript           */
bool    sockdev = false;

    /* Close the existing file, if any */
    if (dev->fd >= 0)
    {
        (dev->hnd->close)( dev );

        RELEASE_DEVLOCK( dev );
        {
            device_attention( dev, CSW_DE );
        }
        OBTAIN_DEVLOCK( dev );
    }

    /* The first argument is the file name */
    if (argc == 0)
    {
        // "%1d:%04X Card: filename is missing"
        WRMSG( HHC01208, "E", LCSS_DEVNUM );
        return -1;
    }

    if (strlen( argv[0] ) >= sizeof( dev->filename ))
    {
        // "%1d:%04X Card: filename %s too long, maximum length is %d"
        WRMSG( HHC01201, "E", LCSS_DEVNUM, argv[0], (int) sizeof( dev->filename ) - 1 );
        return -1;
    }

    /* Save the file name in the device block */
    hostpath( dev->filename, argv[0], sizeof( dev->filename ));

    /* Initialize the device type */
    if (!sscanf( dev->typname, "%hx", &dev->devtype ))
        dev->devtype = 0x3525;

    /* Initialize the device identifier bytes */
    dev->devid[0] = 0xFF;
    if (0x3525 == dev->devtype)
    {
        dev->devid[1] = 0x35;
        dev->devid[2] = 0x05;
    }
    else
    {
        dev->devid[1] = 0x28; /* Control unit type is 2821-1 */
        dev->devid[2] = 0x21;
    }
    dev->devid[3] = 0x01;
    dev->devid[4] = dev->devtype >> 8;
    dev->devid[5] = dev->devtype & 0xFF;
    dev->devid[6] = 0x01;
    dev->numdevid = 7;

    /* Set number of sense bytes */
    dev->numsense = 1;

    /* Set length of buffer */
    dev->bufsize = CARD_LENGTH + 2;

    /* Initialize device dependent fields */
    dev->fd      = -1;
    dev->append  = 0;
    dev->ascii   = 0;
    dev->cardpos = 0;
    dev->cardrem = CARD_LENGTH;
    dev->crlf    = 0;
    dev->excps   = 0;
    dev->stopdev = FALSE;

    /* Process the driver arguments */
    for (i=1; i < argc; i++)
    {
        if (strcasecmp( argv[i], "append" ) == 0)
        {
            dev->append = 1;
            continue;
        }

        if (strcasecmp( argv[i], "ascii" ) == 0)
        {
            dev->ascii = 1;
            continue;
        }

        if (strcasecmp( argv[i], "crlf" ) == 0)
        {
            dev->crlf = 1;
            continue;
        }

        if (strcasecmp( argv[i], "ebcdic" ) == 0)
        {
            dev->ascii = 0;
            continue;
        }

        if (strcasecmp( argv[i], "noclear" ) == 0)
        {
            dev->append = 1;
            // "%1d:%04X %s: option '%s' has been deprecated"
            WRMSG( HHC01251, "W", LCSS_DEVNUM,
                "Card", "noclear" );
            continue;
        }

        /* sockdev means the device file is actually
           a connected socket instead of a disk file.
           The file name is the socket_spec (host:port)
           to listen for connections on.
        */
        if (strcasecmp( argv[i], "sockdev" ) == 0)
        {
            sockdev = true;
            continue;
        }

        // "%1d:%04X Card: parameter %s in argument %d is invalid"
        WRMSG( HHC01209, "E", LCSS_DEVNUM, argv[i], i+1 );
        return -1;

    } /* end for (i=1; i < argc; i++) Process the driver arguments */

    /* Check for incompatible options... */

    if (sockdev && dev->crlf)
    {
        // "%1d:%04X Card: option %s is incompatible"
        WRMSG( HHC01210, "E", LCSS_DEVNUM,
            "sockdev/crlf" );
        return -1;
    }

    if (sockdev && dev->append)
    {
        // "%1d:%04X Card: option %s is incompatible"
        WRMSG( HHC01210, "E", LCSS_DEVNUM,
            "sockdev/noclear" );
        return -1;
    }

    /* If socket device, create a listening socket
       to accept connections on.
    */
    if (sockdev && !bind_device_ex( dev,
        dev->filename, onconnect_callback, dev ))
    {
        return -1;  // (error msg already issued)
    }

    /* Open the device file right away */
    if (!sockdev && open_punch( dev ) != 0)
        return -1;  // (error msg already issued)

    return 0;
} /* end function cardpch_init_handler */

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void cardpch_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "PCH", dev, devclass, buflen, buffer );

    snprintf (buffer, buflen, "%s%s%s%s%s%s IO[%"PRIu64"]",
                filename,
                (dev->ascii                ? " ascii"     : " ebcdic"),
                (dev->bs                   ? " sockdev"   : ""),
                ((dev->ascii && dev->crlf) ? " crlf"      : ""),
                (dev->append               ? " append"    : ""),
                (dev->stopdev              ? " (stopped)" : ""),
                dev->excps );

} /* end function cardpch_query_device */

/*-------------------------------------------------------------------*/
/* Open the punch file                                               */
/*-------------------------------------------------------------------*/
static int open_punch( DEVBLK* dev )
{
int             rc;                     /* Return code               */
int             open_flags;             /* File open flags           */
off_t           filesize = 0;           /* file size for ftruncate   */

    /* Socket punch? */
    if (dev->bs)
        return (dev->fd < 0 ? -1 : 0);

    open_flags = O_WRONLY | O_CREAT /* | O_SYNC */ |  O_BINARY;

    if (!dev->append)
        open_flags |= O_TRUNC;

    if ((dev->fd = HOPEN( dev->filename, open_flags, S_IRUSR | S_IWUSR | S_IRGRP )) < 0)
    {
        // "%1d:%04X %s: error in function %s: %s"
        WRMSG( HHC01250, "E", LCSS_DEVNUM,
            "Card", "HOPEN()", strerror( errno ));
        return -1;
    }

    if (dev->append)
    {
        if ((filesize = lseek( dev->fd, 0, SEEK_END )) < 0)
        {
            // "%1d:%04X %s: error in function %s: %s"
            WRMSG( HHC01250, "E", LCSS_DEVNUM,
                "Card", "lseek()", strerror( errno ));
            return -1;
        }
    }

    /* Set new physical EOF */
    do rc = ftruncate( dev->fd, filesize );
    while (EINTR == rc);

    return 0;
}

/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/*-------------------------------------------------------------------*/
static int cardpch_close_device( DEVBLK* dev )
{
    /* Close the device file */
    if (dev->fd >= 0)
    {
        /* Socket punch? */
        if (dev->bs)
        {
            close_socket( dev->fd );
            // "%1d:%04X Card: client %s, IP %s disconnected from device %s"
            WRMSG( HHC01211, "I", LCSS_DEVNUM, dev->bs->clientname, dev->bs->clientip, dev->bs->spec );
        }
        else
            close( dev->fd );
    }

    dev->fd = -1;
    dev->stopdev = FALSE;

    return 0;
} /* end function cardpch_close_device */

/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
static void cardpch_execute_ccw (DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual)
{
U32             i;                      /* Loop counter              */
U32             num;                    /* Number of bytes to move   */
BYTE            c;                      /* Output character          */

    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq );

    /* If punch stopped, return intervention required */
    if (dev->stopdev && !IS_CCW_SENSE( code ))
    {
        /* Set unit check with intervention required */
        dev->sense[0] = SENSE_IR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    /* Process depending on CCW opcode */
    switch (code) {

    case 0x01:
    case 0x41:
    case 0x81:
    /*---------------------------------------------------------------*/
    /* WRITE, FEED, SELECT STACKER                                   */
    /*---------------------------------------------------------------*/
        /* Start a new record if not data-chained from previous CCW */
        if ((chained & CCW_FLAGS_CD) == 0)
        {
            dev->cardpos = 0;
            dev->cardrem = CARD_LENGTH;

        } /* end if(!data-chained) */

        /* Calculate number of bytes to write and set residual count */
        num = (count < (U32)dev->cardrem) ? count : (U32)dev->cardrem;
        *residual = count - num;

        /* Copy data from channel buffer to card buffer */
        for (i = 0; i < num; i++)
        {
            c = iobuf[i];

            if (dev->ascii)
            {
                c = guest_to_host(c);
            }

            dev->buf[dev->cardpos] = c;
            dev->cardpos++;
            dev->cardrem--;
        } /* end for(i) */

        /* Perform end of record processing if not data-chaining */
        if ((flags & CCW_FLAGS_CD) == 0)
        {
            if (dev->ascii)
            {
                /* Truncate trailing blanks from card buffer */
                for (i = dev->cardpos; i > 0; i--)
                    if (dev->buf[i-1] != SPACE) break;

                /* Append carriage return and line feed */
                if (dev->crlf) dev->buf[i++] = '\r';
                dev->buf[i++] = '\n';
            }
            else
            {
                /* Pad card image with blanks */
                for (i = dev->cardpos; i < CARD_LENGTH; i++)
                    dev->buf[i] = HEX40;
            }

            /* Write card image */
            write_buffer (dev, dev->buf, i, unitstat);
            if (*unitstat != 0) break;

        } /* end if(!data-chaining) */

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x03:
    /*---------------------------------------------------------------*/
    /* CONTROL NO-OPERATION                                          */
    /*---------------------------------------------------------------*/
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x04:
    /*---------------------------------------------------------------*/
    /* SENSE                                                         */
    /*---------------------------------------------------------------*/
        /* Calculate residual byte count */
        num = (count < dev->numsense) ? count : dev->numsense;
        *residual = count - num;
        if (count < dev->numsense) *more = 1;

        /* Copy device sense bytes to channel I/O buffer */
        memcpy (iobuf, dev->sense, num);

        /* Clear the device sense bytes */
        memset (dev->sense, 0, sizeof(dev->sense));

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xE4:
    /*---------------------------------------------------------------*/
    /* SENSE ID                                                      */
    /*---------------------------------------------------------------*/
        /* Calculate residual byte count */
        num = (count < dev->numdevid) ? count : dev->numdevid;
        *residual = count - num;
        if (count < dev->numdevid) *more = 1;

        /* Copy device identifier bytes to channel I/O buffer */
        memcpy (iobuf, dev->devid, num);

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    default:
    /*---------------------------------------------------------------*/
    /* INVALID OPERATION                                             */
    /*---------------------------------------------------------------*/
        /* Set command reject sense byte, and unit check status */
        dev->sense[0] = SENSE_CR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;

    } /* end switch(code) */

} /* end function cardpch_execute_ccw */


static DEVHND cardpch_device_hndinfo =
{
        &cardpch_init_handler,         /* Device Initialization      */
        &cardpch_execute_ccw,          /* Device CCW execute         */
        &cardpch_close_device,         /* Device Close               */
        &cardpch_query_device,         /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        NULL,                          /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */

#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev hdt3525_LTX_hdl_ddev
#define hdl_depc hdt3525_LTX_hdl_depc
#define hdl_reso hdt3525_LTX_hdl_reso
#define hdl_init hdt3525_LTX_hdl_init
#define hdl_fini hdt3525_LTX_hdl_fini
#endif

HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
     HDL_DEPENDENCY(DEVBLK);
}
END_DEPENDENCY_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE(3525, cardpch_device_hndinfo );
}
END_DEVICE_SECTION
