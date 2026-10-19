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
#define KBQLEN_1052     16              /* Max unread input lines    */
#define ATTN_TRIES_1052 100             /* Attention retries, 1ms    */
#define ATTN_STALE_1052 2               /* Secs until an attention
                                           without READ is ignored   */
#define ATTN_WATCH_1052 250000          /* Usecs between checks      */

/*-------------------------------------------------------------------*/
/*  Keyboard input lines not yet read by the guest. While any are    */
/*  queued, the guest is told by an attention interrupt, and told    */
/*  again once it has read: a READ that leaves lines behind ends     */
/*  with attention in its ending status, since the device is still   */
/*  busy with that READ and an unsolicited one would be rejected.    */
/*                                                                   */
/*  Attentions are not counted against READs. A guest may answer     */
/*  two with one READ, or ignore one altogether (MVS does during     */
/*  NIP). An attention still unanswered after a few seconds, and no  */
/*  longer pending, is taken as ignored: the oldest line, which the  */
/*  guest did not want, is discarded with a message, and the guest   */
/*  is told about the lines behind it with another attention.        */
/*-------------------------------------------------------------------*/
typedef struct KBQ1052
{
    int     head;                       /* Index of oldest line      */
    int     count;                      /* Number of lines queued    */
    int     attn;                       /* 1=Attention presented (or
                                           being presented) and not
                                           answered by a READ yet    */
    u_int   attngen;                    /* Attention generation      */
    struct timeval attntime;            /* When it was presented     */
    int     watching;                   /* 1=Watch thread running    */
    int     len [ KBQLEN_1052 ];        /* Length of each line       */
    BYTE    line[ KBQLEN_1052 ][ BUFLEN_1052 ]; /* EBCDIC input      */
}
KBQ1052;

/*-------------------------------------------------------------------*/
/*                     Forward reference                             */
/*-------------------------------------------------------------------*/
static DEVHND con1052_device_hndinfo;
static void   con1052_attention( DEVBLK *dev );

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
/* actually transferred). In this case, IL is not indicated for a    */
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

    p = memchr( sysblk.cmd_pfxs, dev->filename[0], sysblk.num_pfxs );

    if (p)
    {
        i = (p - sysblk.cmd_pfxs);
        sysblk.used_pfxs[i] = FALSE;
    }

    /* Discard any input the guest has not read */
    free( dev->dev_data );
    dev->dev_data = NULL;

    dev->fd = -1;   /* device has now been closed */

    return 0;
}

/*-------------------------------------------------------------------*/
/*            QUERY THE 1052/3215 DEVICE DEFINITION                  */
/*-------------------------------------------------------------------*/
static void con1052_query_device( DEVBLK *dev, char **devclass,
                                  int buflen, char *buffer )
{
    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    ASSERT( IS_INTEGRATED_CONS( dev )); // (sanity check)

    BEGIN_DEVICE_CLASS_QUERY( "CON", dev, devclass, buflen, buffer );

    snprintf( buffer, buflen, "*syscons cmdpref '%s' %sprompt IO[%"PRIu64"]",
        filename, dev->prompt1052 ? "" : "no", dev->excps );
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

    /* Indicate that this is NOT a console device */
    dev->console = 0;

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
        /* Locate an unused command prefix */
        p = memchr( sysblk.used_pfxs, 0, sysblk.num_pfxs );

        if (!p)
        {
            // "%1d:%04X COMM: default command prefixes exhausted"
            WRMSG( HHC01085, "E", LCSS_DEVNUM );
            return -1;
        }

        i = (p - sysblk.used_pfxs);
        sysblk.used_pfxs[i] = TRUE;
        dev->filename[0] = sysblk.cmd_pfxs[i];
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
                // "%1d:%04X COMM: device %1d:%04X already using prefix '%s'"
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
    p = memchr( sysblk.cmd_pfxs, our_pfx[0], sysblk.num_pfxs );

    if (p)
    {
        i = (p - sysblk.cmd_pfxs);
        sysblk.used_pfxs[i] = TRUE;
    }

    /* Allocate the keyboard input queue */
    if (!(dev->dev_data = calloc( 1, sizeof( KBQ1052 ))))
    {
        // "%1d:%04X %s: Error in function %s: %s"
        WRMSG( HHC00900, "E", LCSS_DEVNUM, dev->typname, "calloc()",
            strerror( errno ));
        return -1;
    }

    /* PROGRAMMING NOTE: it is CRITICAL that we set our file
       descriptor integer to a non-zero value greater than 2
       (even though we'll never use it) in order to force the
       config.c "detach_devblk" function to call our 'close'
       function so we can mark our prefix as being once again
       available for reuse.
    */
    dev->fd = INTEGRATED_CONS_FD;  /* CRITICAL! See above! */

    return 0;
}

/*-------------------------------------------------------------------*/
/*        DISCARD THE LINE OF AN ATTENTION THE GUEST IGNORED         */
/*-------------------------------------------------------------------*/
static int con1052_stale_attn( DEVBLK *dev, KBQ1052 *kbq, int inccw,
                               char *discarded )
{
    struct timeval  now, age;
    int             i;

    /* NOTE: the device lock must be held! */

    /* A guest answers an attention within milliseconds. One still
       unanswered after seconds, and no longer pending, was ignored;
       left standing, it would keep every queued line waiting. Within
       a channel program the device is busy, but no attention can be
       pending then either.
    */
    if (0
        || !kbq->attn
        || (!inccw && (dev->busy || IOPENDING( dev )))
    )
        return 0;

    gettimeofday( &now, NULL );
    timeval_subtract( &kbq->attntime, &now, &age );

    if (age.tv_sec < ATTN_STALE_1052)
        return 0;

    kbq->attn = 0;

    /* Discard the oldest line rather than present it later. Typed
       ahead of a guest that was not ready for it, it could end up as
       the answer to a question asked afterwards: a reply "R 00,..."
       meant for one WTOR reaching the next one numbered 00. Without
       the queue, the next line entered overwrote it.
    */
    if (!kbq->count)
        return 0;

    for (i=0; i < kbq->len[ kbq->head ]; i++)
        discarded[i] = guest_to_host( kbq->line[ kbq->head ][i] );
    discarded[i] = '\0';

    kbq->head = (kbq->head + 1) % KBQLEN_1052;
    kbq->count--;

    return 1;
}

/*-------------------------------------------------------------------*/
/*       WATCH AN OUTSTANDING ATTENTION WHILE INPUT IS QUEUED        */
/*-------------------------------------------------------------------*/
static void* con1052_watch_thread( void *arg )
{
    DEVBLK  *dev = (DEVBLK*) arg;
    KBQ1052 *kbq;
    int      ignored;
    int      dropped;
    char     discarded[ BUFLEN_1052 + 1 ];

    /* Present another attention if the guest ignored the last one
       while input is still queued. Nothing else may happen on the
       device that would: no input entered, no channel program run.
    */
    for (;;)
    {
        USLEEP( ATTN_WATCH_1052 );

        OBTAIN_DEVLOCK( dev );
        {
            /* Done once the input is read, or the device closed */
            if (0
                || !(kbq = dev->dev_data)
                || !kbq->count
                || !kbq->attn
            )
            {
                if (kbq)
                    kbq->watching = 0;
                RELEASE_DEVLOCK( dev );
                return NULL;
            }

            dropped = con1052_stale_attn( dev, kbq, FALSE, discarded );
            ignored = !kbq->attn;
        }
        RELEASE_DEVLOCK( dev );

        if (dropped)
            // "'%s' input for console %1d:%04X discarded, ignored by the guest: \"%s\""
            WRMSG( HHC00008, "W", dev->filename, LCSS_DEVNUM, discarded );

        if (ignored)
            con1052_attention( dev );
    }

    UNREACHABLE_CODE( return NULL );
}

/*-------------------------------------------------------------------*/
/*              START WATCHING AN OUTSTANDING ATTENTION              */
/*-------------------------------------------------------------------*/
static void con1052_watch( DEVBLK *dev, KBQ1052 *kbq )
{
    TID      tid;

    /* NOTE: the device lock must be held! */

    if (!kbq->watching)
    {
        kbq->watching = 1;

        if (create_thread( &tid, DETACHED, con1052_watch_thread,
                dev, "con1052_watch_thread" ) != 0)
            kbq->watching = 0;
    }
}

/*-------------------------------------------------------------------*/
/*           PRESENT ATTENTION FOR THE QUEUED INPUT LINES            */
/*-------------------------------------------------------------------*/
static void con1052_attention( DEVBLK *dev )
{
    KBQ1052 *kbq;
    u_int    gen;
    int      tries;
    int      rc;

    /* NOTE: the device lock must *NOT* be held! */

    /* The attention is rejected while the device is busy or still
       has an interrupt pending, which a console the guest is writing
       to usually is only for a moment. Retry briefly. If that fails,
       the ending status of the next channel program for the device
       presents it instead.
    */
    for (tries=0; ; tries++)
    {
        /* Claim the attention before presenting it, unless one is
           outstanding, the guest is waiting to read anyway, or has
           read everything meanwhile.
        */
        OBTAIN_DEVLOCK( dev );
        {
            if (0
                || !(kbq = dev->dev_data)
                || !kbq->count
                || kbq->attn
                || dev->kbwaiters
            )
            {
                RELEASE_DEVLOCK( dev );
                return;
            }
            kbq->attn = 1;
            gen = ++kbq->attngen;
            gettimeofday( &kbq->attntime, NULL );
        }
        RELEASE_DEVLOCK( dev );

        rc = device_attention( dev, CSW_ATTN );

        OBTAIN_DEVLOCK( dev );
        {
            if ((kbq = dev->dev_data) && kbq->attngen == gen)
            {
                /* Presented: see that it is answered */
                if (rc == 0)
                    con1052_watch( dev, kbq );

                /* Not presented: give the claim back, unless a READ
                   has answered it meanwhile and another been claimed
                */
                else
                    kbq->attn = 0;
            }
        }
        RELEASE_DEVLOCK( dev );

        if (rc != 1 || tries >= ATTN_TRIES_1052)
            return;

        USLEEP( 1000 );
    }
}

/*-------------------------------------------------------------------*/
/*        CONSOLE PRINTER KEYBOARD PANEL COMMAND FUNCTION            */
/*-------------------------------------------------------------------*/
static void* con1052_panel_command( char *cmd )
{
    DEVBLK  *dev;
    KBQ1052 *kbq;
    char    *input;
    int      i;
    int      queued;
    int      slot;
    int      waiters;
    int      attn = 0;
    int      dropped;
    char     discarded[ BUFLEN_1052 + 1 ];
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
            /* Get past command prefix to the actual command */
            input = cmd + pfxlen;

            /* Echo what they typed to the Hercules console */
            // "'%s' input entered for console %1d:%04X: \"%s\""
            WRMSG( HHC00013, "I", dev->filename, LCSS_DEVNUM, input );
            LOGMSG( "%s\n", input );

            OBTAIN_DEVLOCK( dev );
            {
                /* The device may have been detached meanwhile */
                if (!(kbq = dev->dev_data))
                {
                    RELEASE_DEVLOCK( dev );
                    return NULL;
                }

                /* An attention the guest ignored discards its line */
                dropped = con1052_stale_attn( dev, kbq, FALSE, discarded );

                /* Keep the input queued until the guest reads it.
                   Replacing an unread line would lose it silently.
                */
                queued = kbq->count;
                if (queued < KBQLEN_1052)
                {
                    slot = (kbq->head + queued) % KBQLEN_1052;

                    /* Convert ASCII input to EBCDIC */
                    for (i=0; i < BUFLEN_1052 && input[i] != '\0'; i++)
                        kbq->line[slot][i] = isprint( (unsigned char)input[i] ) ?
                                host_to_guest( input[i] ) : ' ';

                    kbq->len[slot] = i;
                    kbq->count++;

                    /* Count the input, even an empty one: it is what a
                       waiting READ INQUIRY waits for, since empty input
                       leaves keybdrem at zero.
                    */
                    dev->kbinput++;

                    /* Wakup the channel if it's waiting for input */
                    waiters = dev->kbwaiters;
                    if (waiters)
                        signal_condition( &dev->kbcond );

                    /* Otherwise present an attention interrupt,
                       unless one is outstanding: the READ that
                       answers it ends with another if lines remain.
                    */
                    attn = (!waiters && !kbq->attn);
                }
            }
            RELEASE_DEVLOCK( dev );

            if (dropped)
                // "'%s' input for console %1d:%04X discarded, ignored by the guest: \"%s\""
                WRMSG( HHC00008, "W", dev->filename, LCSS_DEVNUM, discarded );

            if (queued >= KBQLEN_1052)
                // "'%s' input for console %1d:%04X discarded, %d lines not yet read: \"%s\""
                WRMSG( HHC00016, "W", dev->filename, LCSS_DEVNUM, queued, input );

            /* Present unsolicited attention interrupt */
            else if (attn)
                con1052_attention( dev );

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
u_int   kbinput;                        /* Keyboard input count      */
KBQ1052 *kbq;                           /* Keyboard input queue      */
int     dropped;                        /* 1=Input line discarded    */
char    discarded[ BUFLEN_1052 + 1 ];   /* The discarded input line  */

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
                && !isprint((unsigned char)c)
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

        OBTAIN_DEVLOCK( dev );

        kbq = dev->dev_data;

        /* Take the next input line unless data chaining continues one */
        if (!dev->keybdrem)
        {
            /* Solicit console input if none is queued */
            if (!kbq->count)
            {
                /* Register as a waiter BEFORE prompting. The prompt can be
                   answered (by an HAO rule, for instance) before we would
                   otherwise get to wait, and that input must not be lost.
                */
                kbinput = dev->kbinput;
                dev->kbwaiters++;

                /* Display prompting message on console if allowed */
                if (dev->prompt1052)
                {
                    RELEASE_DEVLOCK( dev );
                    // "Enter '%s' input for console %1d:%04X"
                    WRMSG( HHC00010, "A", dev->filename, LCSS_DEVNUM );
                    OBTAIN_DEVLOCK( dev );
                }

                /* Wait for input to arrive, unless it already has */
                while (dev->kbinput == kbinput)
                    wait_condition( &dev->kbcond, &dev->lock );

                dev->kbwaiters--;
            }

            /* Move the oldest line into the device buffer */
            if (kbq->count)
            {
                len = kbq->len[ kbq->head ];
                memcpy( dev->buf, kbq->line[ kbq->head ], len );
                dev->keybdrem = len;
                dev->buflen   = len;

                kbq->head = (kbq->head + 1) % KBQLEN_1052;
                kbq->count--;

                /* The outstanding attention, if any, is answered */
                kbq->attn = 0;
            }
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

        RELEASE_DEVLOCK( dev );

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

    /* Input lines still waiting without an outstanding attention
       get one with the ending status of the channel program. Either
       this READ answered the last one, or the device was busy when
       the attention was due, so an unsolicited one was rejected.
    */
    if (1
        && *unitstat == (CSW_CE | CSW_DE)
        && !(flags & (CCW_FLAGS_CC | CCW_FLAGS_CD))
    )
    {
        OBTAIN_DEVLOCK( dev );
        {
            kbq = dev->dev_data;

            dropped = con1052_stale_attn( dev, kbq, TRUE, discarded );

            if (kbq->count && !kbq->attn)
            {
                kbq->attn = 1;
                kbq->attngen++;
                gettimeofday( &kbq->attntime, NULL );
                con1052_watch( dev, kbq );
                *unitstat |= CSW_ATTN;
            }
        }
        RELEASE_DEVLOCK( dev );

        if (dropped)
            // "'%s' input for console %1d:%04X discarded, ignored by the guest: \"%s\""
            WRMSG( HHC00008, "W", dev->filename, LCSS_DEVNUM, discarded );
    }

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
