/* CON1052.C    (c)Copyright Jan Jaeger, 2004-2012                   */
/*              (C) and others 2013-2023                             */
/*              Emulated Console Printer Keyboard on Hercules HMC    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for integrated     */
/* 1052 and 3215 console printer keyboard devices for the Hercules   */
/* ESA/390 emulator.                                                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "sr.h"

/*-------------------------------------------------------------------*/
/*                 Tweakable build constants                         */
/*-------------------------------------------------------------------*/
#define BUFLEN_1052     150             /* 1052 Send/Receive buffer  */

/*-------------------------------------------------------------------*/
/*                     Forward reference                             */
/*-------------------------------------------------------------------*/
static DEVHND con1052_device_hndinfo;

/*-------------------------------------------------------------------*/
/* Ivan Warren 20040227                                              */
/*                                                                   */
/* This table is used by channel.c to determine if a CCW code        */
/* is an immediate command or not.                                   */
/*                                                                   */
/* The table is addressed in the DEVHND structure as 'DEVIMM immed'  */
/*                                                                   */
/*     0:  ("false")  Command is *NOT* an immediate command          */
/*     1:  ("true")   Command *IS* an immediate command              */
/*                                                                   */
/* Note: An immediate command is defined as a command which returns  */
/* CE (channel end) during initialization (that is, no data is       */
/* actually transfered). In this case, IL is not indicated for a     */
/* Format 0 or Format 1 CCW when IL Suppression Mode is in effect.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static BYTE  con1052_immed [256] =

 /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
  { 0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,  /* 00 */      // 03, 0B
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 10 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 20 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 30 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 40 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 50 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 60 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 70 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 80 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 90 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* A0 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* B0 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* C0 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* D0 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* E0 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; /* F0 */

/*-------------------------------------------------------------------*/
/*  MinGW       (Windows, but *not* MSVC)                            */
/*-------------------------------------------------------------------*/
#if defined( WIN32 ) && !defined( _MSVC_ ) && !defined( HDL_USE_LIBTOOL )

        SYSBLK   *psysblk;
#define sysblk  (*psysblk)
#endif

/*-------------------------------------------------------------------*/
/*                   Default Command Prefixes                        */
/*-------------------------------------------------------------------*/
static const char default_pfxs[] =
{
    0x2F,       //      /       slash
    0x60,       //      `       backtick
    0x3D,       //      =       equals
    0x7E,       //      ~       tilde
    0x40,       //      @       at sign
    0x24,       //      $       dollar
    0x25,       //      %       percent
    0x5E,       //      ^       caret
    0x26,       //      &       ampersand
    0x5F,       //      _       underline
    0x3A,       //      :       colon
    0x3F,       //      ?       question
    0x30,       //      0       zero
    0x31,       //      1       one
    0x32,       //      2       two
    0x33,       //      3       three
    0x34,       //      4       four
    0x35,       //      5       five
    0x36,       //      6       six
    0x37,       //      7       seven
    0x38,       //      8       eight
    0x39,       //      9       nine
};

static char used_pfxs[ sizeof( default_pfxs ) ] = {0};

/*-------------------------------------------------------------------*/
/*             CLOSE THE 1052/3215 DEVICE HANDLER                    */
/*-------------------------------------------------------------------*/
static int con1052_close_device( DEVBLK *dev )
{
    /* If the first character of our command prefix matches
       one of the characters in our list of default command
       prefixes, then mark that command prefix character as
       being unused and once again available for reuse.
    */
    char* p;
    size_t i;

    p = memchr( default_pfxs, dev->filename[0], sizeof( default_pfxs ));

    if (p)
    {
        i = (p - default_pfxs);
        used_pfxs[i] = FALSE;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*            QUERY THE 1052/3215 DEVICE DEFINITION                  */
/*-------------------------------------------------------------------*/
static void con1052_query_device( DEVBLK *dev, char **devclass,
                                  int buflen, char *buffer )
{
    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "CON", dev, devclass, buflen, buffer );

    snprintf( buffer, buflen, "*syscons cmdpref(%s)%s IO[%"PRIu64"]",
        filename, !dev->prompt1052 ? " noprompt" : "", dev->excps );

}

/*-------------------------------------------------------------------*/
/*         INITIALIZE THE 1052/3215 DEVICE HANDLER                   */
/*-------------------------------------------------------------------*/
static int con1052_init_handler( DEVBLK *dev, int argc, char *argv[] )
{
    int      ac;
    DEVBLK  *our_dev, *their_dev;
    char    *our_pfx, *their_pfx, *p;
    size_t   our_pfxlen, their_pfxlen, shorter_pfxlen, i;

    /* For re-initialisation, close the existing file, if any */
    if (dev->fd >= 0)
        (dev->hnd->close)( dev );

    /* reset excp count */
    dev->excps = 0;

    /* Integrated console is always connected */
    dev->connected = 1;

    /* Set number of sense bytes */
    dev->numsense = 1;

    /* Initialize device dependent fields */
    dev->keybdrem = 0;

    /* Set length of console buffer reserving extra room for
       possible newline character and null string terminator.
    */
    dev->bufsize = BUFLEN_1052 +1 +1;

    /* Assume we want to prompt */
    dev->prompt1052 = 1;

    /* Initialize command prefix string to undefined */
    STRLCPY( dev->filename, "" );

    /* Determine device type. Default to 1052 */
    if (!sscanf( dev->typname, "%hx", &dev->devtype))
        dev->devtype = 0x1052;

    /* Initialize the device identifier bytes */
    dev->devid[0] = 0xFF;
    dev->devid[1] = dev->devtype >> 8;
    dev->devid[2] = dev->devtype & 0xFF;
    dev->devid[3] = 0x00;
    dev->devid[4] = dev->devtype >> 8;
    dev->devid[5] = dev->devtype & 0xFF;
    dev->devid[6] = 0x00;
    dev->numdevid = 7;

    /* Process optional arguments */
    for (ac=0; argc > 0; ac++, argc--)
    {
        if (strcasecmp( argv[ac], "noprompt" ) == 0)
            dev->prompt1052 = 0;
        else
            STRLCPY( dev->filename, argv[ac] );
    }

    /* Set default command prefix if one wasn't specified */
    if (!dev->filename[0])
    {
        p = memchr( used_pfxs, 0, sizeof( used_pfxs ));

        if (!p)
        {
            // "%1d:%04X COMM: default command prefixes exhausted"
            WRMSG( HHC01085, "E", LCSS_DEVNUM );
            return -1;
        }

        i = (p - used_pfxs);
        used_pfxs[i] = TRUE;
        dev->filename[0] = default_pfxs[i];
        dev->filename[1] = 0;
    }

    /* Check if valid command prefix: cannot be a subset or superset
       of any other console printer keyboard device's command prefix.
    */
    our_dev     = dev;
    our_pfx     = our_dev->filename;
    our_pfxlen  = strlen( our_pfx );

    for (their_dev = sysblk.firstdev; their_dev; their_dev = their_dev->nextdev)
    {
        if (1
            && their_dev != our_dev
            && their_dev->allocated
            && their_dev->hnd == &con1052_device_hndinfo
        )
        {
            their_pfx      = their_dev->filename;
            their_pfxlen   = strlen( their_pfx );
            shorter_pfxlen = min( their_pfxlen, our_pfxlen );

            if (strncmp( our_pfx, their_pfx, shorter_pfxlen ) == 0)
            {
                // "%1d:%04X COMM: device %1d:%04X already using prefix %s"
                WRMSG( HHC01086, "E",
                    SSID_TO_LCSS( our_dev   -> ssid ), our_dev   -> devnum,
                    SSID_TO_LCSS( their_dev -> ssid ), their_dev -> devnum,
                    their_pfx
                );
                return -1;
            }
        }
    }

    /* If the first character of our command prefix matches
       one of the characters in our list of default command
       prefixes, then mark that command prefix character as
       being used and no longer available for use.
    */
    p = memchr( default_pfxs, our_pfx[0], sizeof( default_pfxs ));

    if (p)
    {
        i = (p - default_pfxs);
        used_pfxs[i] = TRUE;
    }

    /* PROGRAMMING NOTE: it is CRITICAL that we set our file
       descriptor integer to a non-zero value greater than 2
       (even though we'll never use it) in order to force the
       config.c "detach_devblk" function to call our 'close'
       function so we can mark our prefix as being once again
       available for reuse.
    */
    dev->fd = 999;  /* CRITICAL! See above Programming Note */

    return 0;
}

/*-------------------------------------------------------------------*/
/*        CONSOLE PRINTER KEYBOARD PANEL COMMAND FUNCTION            */
/*-------------------------------------------------------------------*/
static void* con1052_panel_command( char *cmd )
{
    DEVBLK  *dev;
    char    *input;
    int      i;
    size_t   pfxlen;

    void* (*next_panel_command_handler)( char *cmd );

    /* Scan device chain looking for integrated console printer keyboard
       devices whose command prefix string (kept in dev->filename field)
       matches the command prefix used in the command that was entered.
    */
    for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
    {
        if (1
            && dev->allocated
            && dev->hnd == &con1052_device_hndinfo
            && (pfxlen = strlen( dev->filename )) > 0
            && strncasecmp( cmd, dev->filename, pfxlen ) == 0
        )
        {
            /* Echo that they typed to the Hercules console */
            LOGMSG( "%s%s\n", "", cmd );

            /* Convert ASCII input to EBCDIC */
            input = cmd + pfxlen;

            for (i=0; i < dev->bufsize && input[i] != '\0'; i++)
                dev->buf[i] = isprint( input[i] ) ?
                        host_to_guest( input[i] ) : ' ';

            /* Update number of bytes in keyboard buffer */
            dev->keybdrem = i;
            dev->buflen   = i;

            /* Wakup the channel if it's waiting for input
               or present unsolicited attention interrupt.
            */
            obtain_lock( &dev->lock );

            if (dev->kbwaiters)
            {
                signal_condition( &dev->kbcond );
                release_lock( &dev->lock );
            }
            else
            {
                release_lock( &dev->lock );
                device_attention( dev, CSW_ATTN );
            }
            return NULL;
        }
    }

    /* The entered command wasn't meant for us. Pass it
       on to the next command handler, if there is one.
    */
    next_panel_command_handler = hdl_next( &con1052_panel_command );

    if (next_panel_command_handler)
        return next_panel_command_handler( cmd );

    return NULL;
}

/*-------------------------------------------------------------------*/
/*           EXECUTE A 1052/3215 CHANNEL COMMAND WORD                */
/*-------------------------------------------------------------------*/
static void con1052_execute_ccw( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual )
{
U32     len;                            /* Length of data            */
U32     num;                            /* Number of bytes to move   */
BYTE    c;                              /* Print character           */

    UNREFERENCED( chained );
    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq );

    /* Unit check with intervention required if no client connected */
    if (dev->connected == 0 && !IS_CCW_SENSE( code ))
    {
        dev->sense[0] = SENSE_IR;
        *unitstat = CSW_UC;
        return;
    }

    /* Process depending on CCW opcode */
    switch (code) {

    case 0x01:
    /*---------------------------------------------------------------*/
    /* WRITE, NO CARRIER RETURN                                      */
    /*---------------------------------------------------------------*/

    case 0x09:
    /*---------------------------------------------------------------*/
    /* WRITE, AUTO CARRIER RETURN                                    */
    /*---------------------------------------------------------------*/

        /* Calculate number of bytes to write and set residual count */
        num = (count < BUFLEN_1052) ? count : BUFLEN_1052;
        *residual = count - num;

        /* Translate channel buffer data to ASCII */
        for (len = 0; len < num; len++)
        {
            c = guest_to_host( iobuf[len] );
            if (1
                && c != 0x0d
                && c != 0x0a
                && !isprint(c)
            )
                c = ' ';
            iobuf[len] = c;
        }

        /* Perform end of record processing if not data-chaining */
        if ((flags & CCW_FLAGS_CD) == 0)
        {
            /* CCW opcode == Write, Auto Carrier Return? */
            if (code == 0x09)
                iobuf[len++] = '\n';
        }

        /* Append string terminator */
        iobuf[len] = '\0';

        /* Process multiline messages */
        {
            char* str = (char*) iobuf;
            char* nl;

            while (str && *str)
            {
                nl = strchr( str, '\n' );

                if (nl) *nl++ = 0;

                LOGMSG( "%s%s\n", dev->filename, str );

                str = nl;
            }
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x03:
    /*---------------------------------------------------------------*/
    /* CONTROL NO-OPERATION                                          */
    /*---------------------------------------------------------------*/
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x0A:
    /*---------------------------------------------------------------*/
    /* READ INQUIRY                                                  */
    /*---------------------------------------------------------------*/

        /* Solicit console input if no data in the device buffer */
        if (!dev->keybdrem)
        {
            /* Display prompting message on console if allowed */
            if (dev->prompt1052)
                // "Enter input for console %1d:%04X"
                WRMSG( HHC00010, "A", LCSS_DEVNUM );

            obtain_lock( &dev->lock );
            {
                dev->kbwaiters++;
                wait_condition( &dev->kbcond, &dev->lock );
                dev->kbwaiters--;
            }
            release_lock( &dev->lock );
        }

        /* Calculate number of bytes to move and residual byte count */
        len = dev->keybdrem;
        num = (count < len) ? count : len;
        *residual = count - num;
        if (count < len) *more = 1;

        /* Copy data from device buffer to channel buffer */
        memcpy( iobuf, dev->buf, num );

        /* If data chaining is specified, save remaining data */
        if ((flags & CCW_FLAGS_CD) && len > count)
        {
            memmove( dev->buf, dev->buf + count, len - count );
            dev->keybdrem = len - count;
        }
        else
        {
            dev->keybdrem = 0;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x0B:
    /*---------------------------------------------------------------*/
    /* AUDIBLE ALARM                                                 */
    /*---------------------------------------------------------------*/
        // "RRR...RING...GGG!\a"
        WRMSG( HHC00009, "I" );

        // *residual = 0;

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
        memcpy( iobuf, dev->sense, num );

        /* Clear the device sense bytes */
        memset( dev->sense, 0, sizeof( dev->sense ));

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
        memcpy( iobuf, dev->devid, num );

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

} /* end function con1052_execute_ccw */

/*-------------------------------------------------------------------*/
/*         HERCULES DYNAMIC LOADER (HDL) CONTROL SECTIONS            */
/*-------------------------------------------------------------------*/
static DEVHND  con1052_device_hndinfo =
{
        &con1052_init_handler,         /* Device Initialization      */
        &con1052_execute_ccw,          /* Device CCW execute         */
        &con1052_close_device,         /* Device Close               */
        &con1052_query_device,         /* Device Query               */
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
        con1052_immed,                 /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

/*-------------------------------------------------------------------*/
/*            Libtool static name collision resolution               */
/*-------------------------------------------------------------------*/
/*   Note : lt_dlopen will look for symbol & modulename_LTX_symbol   */
/*-------------------------------------------------------------------*/
#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev hdt1052c_LTX_hdl_ddev
#define hdl_depc hdt1052c_LTX_hdl_depc
#define hdl_reso hdt1052c_LTX_hdl_reso
#define hdl_init hdt1052c_LTX_hdl_init
#define hdl_fini hdt1052c_LTX_hdl_fini
#endif

/*-------------------------------------------------------------------*/
/*         HERCULES DYNAMIC LOADER (HDL) CONTROL SECTIONS            */
/*-------------------------------------------------------------------*/

HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY( HERCULES );
     HDL_DEPENDENCY( DEVBLK );
     HDL_DEPENDENCY( SYSBLK );
}
END_DEPENDENCY_SECTION

#if defined( WIN32 ) && !defined( HDL_USE_LIBTOOL ) && !defined(_MSVC_ )
#undef sysblk
HDL_RESOLVER_SECTION;
{
    HDL_RESOLVE_SYMPTR( psysblk, sysblk );
    HDL_RESOLVE( panel_command );
}
END_RESOLVER_SECTION
#endif

HDL_DEVICE_SECTION;
{
    HDL_DEVICE( 1052-C, con1052_device_hndinfo );
    HDL_DEVICE( 3215-C, con1052_device_hndinfo );
}
END_DEVICE_SECTION


HDL_REGISTER_SECTION;
{
   HDL_REGISTER( panel_command, con1052_panel_command );
}
END_REGISTER_SECTION
