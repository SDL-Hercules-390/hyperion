/* PRINTER.C    (c) Copyright Roger Bowler, 1999-2012                */
/*              (c) Copyright Enrico Sorichetti, 2012                */
/*              ESA/390 Line Printer Device Handler                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for emulated       */
/* System/370 line printer devices with fcb support and more.        */
/*-------------------------------------------------------------------*/
/* Primary Reference:                                                */
/*    GA24-3312-09 "IBM 2821 Control Unit Component Description"     */
/* See also:                                                         */
/*    GA24-3073-8 "IBM 1403 Printer Component Description"           */
/*    GA24-3543-0 "IBM 3211 Printer and 3811 Control Unit            */
/*                                  Component Description"           */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#include "devtype.h"
#include "opcode.h"

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

/* Printer Specific : 1403 */
/* The following are considered IMMEDIATE commands : */
/* CTL-NOOP, Skip Channel 'n' Immediate, Block Data check , Allow Data Check
 * Space 1,2,3 Lines Immediate, UCS Gate Load, Load UCS Buffer & Fold,
 * Load UCS Buffer (No Fold)
 */

static BYTE printer_immed_commands[256]=
/*0 1 2 3 4 5 6 7 8 9 A B C D E F*/
{ 0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*0*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*1*/
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /*2*/
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /*3*/
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /*4*/
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /*5*/
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /*6*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*7*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*8*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*9*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*A*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*B*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*C*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*D*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, /*E*/
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0};/*F*/

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
//#define LINE_LENGTH     150
#define BUFF_SIZE       1500
#define BUFF_OVFL       150

/*-------------------------------------------------------------------*/
/*              DEFAULT 3211 FORMS CONTROL BUFFER                    */
/* First entry (entry 0) is the number of lines per page. The next   */
/* 12 entries are the line numbers assigned to channels 1 thru 12.   */
/*-------------------------------------------------------------------*/
const int FCBMASK[] = {66,1,7,13,19,25,31,37,43,63,49,55,61};

/*-------------------------------------------------------------------*/
/*               DEFAULT 1403 CARRIAGE CONTROL TAPE                  */
/* Each entry in the below table corresponds to a line on a page.    */
/* The number of entries in the array defines the length of the tape */
/* and thus the number of lines on the page. Each entry is a bitmask */
/* indicating which of the 12 possible channels are punched on that  */
/* line. It is thus possible to punch more than one channel per line */
/* and the same channel may be punched on different lines as well.   */
/* For example, both channels 9 and 12 can be punched on the same    */
/* line and multiple lines may all have the same channel 11 punch,   */
/* just like on a real 1403 printer carriage control tape.           */
/*-------------------------------------------------------------------*/
const
U16 cctape[] = {    /* carriage control tape = channel punch masks:

    0x8000   1
    0x4000   2
    0x2000   3
    0x1000   4
    0x0800   5
    0x0400   6
    0x0200   7
    0x0100   8
    0x0080   9
    0x0040  10
    0x0020  11
    0x0010  12

    The first value is the line number and the second is the channel.

    Default:  "cctape=(1=1,7=2,13=3,19=4,25=5,31=6,37=7,43=8,61=9,49=10,55=11,61=12)"
     (or):    "cctape=(1=1,61=(9,12),7=2,13=3,19=4,25=5,31=6,37=7,43=8,49=10,55=11)"

    1 2  3  4  5  6  7  8  9 10 11 12  (chan)
    1 7 13 19 25 31 37 43 61 49 55 61  (line)  (note: 9 & 12 same line!)
*/
/* 1       2       3       4       5       6       7       8       9      10        lines  */
0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000, /*  1 - 10 */
0x0000, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x0000, /* 11 - 20 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 21 - 30 */
0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000, /* 31 - 40 */
0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000, /* 41 - 50 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 51 - 60 */
0x0090, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,                                 /* 61 - 66 */
 0 }; /* (remainder of array == zeroes) */

#define LINENUM(n)  (1 + (((n)-1) % dev->lpp))

#define WRITE_LINE() \
do { \
    /* Start a new record if not data-chained from previous CCW */ \
    if ((chained & CCW_FLAGS_CD) == 0) \
    { \
        dev->bufoff = 0; \
        dev->bufres = BUFF_SIZE; \
    } /* end if(!data-chained) */ \
    if (dev->index > 1) \
    { \
        for (i=1; i < (U32)dev->index; i++) \
        { \
            dev->buf[dev->bufoff] = SPACE; \
            dev->bufoff++; \
            dev->bufres--; \
        } /* end for(i) */ \
    } /* end if ( dev->index > 1 )  */ \
    /* Calculate number of bytes to write and set residual count */ \
    num = (count < (U32)dev->bufres) ? count : (U32)dev->bufres; \
    *residual = count - num; \
    /* Copy data from channel buffer to print buffer */ \
    for (i=0; i < num; i++) \
    { \
        c = guest_to_host(iobuf[i]); \
        if (dev->fold) c = toupper(c); \
        if (c == 0) c = SPACE; \
        dev->buf[dev->bufoff] = c; \
        dev->bufoff++; \
        dev->bufres--; \
    } /* end for(i) */ \
    /* Perform end of record processing if not data-chaining */ \
    if ((flags & CCW_FLAGS_CD) == 0) \
    { \
        /* Truncate trailing blanks from print line */ \
        for (i = dev->bufoff; i > 0; i--) \
            if (dev->buf[i-1] != SPACE) break; \
        /* Write print line */ \
        write_buffer (dev, (char *)dev->buf, i, unitstat); \
        if (*unitstat != 0) return; \
        if (dev->crlf) \
        { \
            write_buffer (dev, "\r", 1, unitstat); \
            if (*unitstat != 0) return; \
        } \
        dev->sp0after = (code == 0x01) ? 1 : 0; \
    } /* end if(!data-chaining) */ \
    /* Return normal status */ \
} while(0)

#define SPACE_LINES() \
do { \
    /* Space the requested number of lines */ \
    write_buffer(dev, nls, coun, unitstat); \
    if (*unitstat == 0) \
        *unitstat = CSW_CE | CSW_DE; \
    /* \
      Unit Exception (CSW_UX) is set (along with device \
      end) when any carriage spacing operation (but not \
      skipping) senses that channel 12 has been passed. \
      In other words, the write and space, and space \
      immediate commands will cause the Unit Exception \
      bit to be set when channel 12 is passed, but the \
      write and skip, skip immediate, and write without \
      spacing commands will NOT cause the Unit Exception \
      bit to be set. See page 36 of GA24-3312-09 "IBM \
      2821 Control Unit Component Description". \
    */ \
    dev->currline += coun;    /* increment but don't fix */ \
    if (dev->currline > dev->chan12line) \
        *unitstat |= CSW_UX; \
    dev->currline = LINENUM( dev->currline ); /* now fix */ \
} while(0)

#define SKIP_TO_CHAN() \
do { \
    U8  found = FALSE; /* TRUE/FALSE whether channel was found */ \
    if (dev->devtype == 0x1403) \
    { \
        /* 1403: Search carriage control tape for channel punch */ \
        int line; \
        U16 chan_mask = 0x8000 >> (chan - 1); \
        for (line=LINENUM(dev->currline+1); !(dev->cctape[line-1] & chan_mask) && line != dev->currline; line=LINENUM(line+1)) \
            ; /* nop */ \
        if (dev->cctape[line-1] & chan_mask) \
        { \
            found = TRUE; \
            dev->destline = line; \
        } \
    } \
    else /* Not 1403 (e.g. 3211): Search FCB */ \
    { \
        int line; \
        for (line=LINENUM(dev->currline+1); dev->fcb[line] != chan && line != dev->currline; line=LINENUM(line+1)) \
            ; /* nop */ \
        if (dev->fcb[line] == chan)  \
        { \
            found = TRUE; \
            dev->destline = line; \
        } \
    } \
    if (found) \
    { \
        /* If this is a skip immediate to the same channel \
           as the one we're already positioned at and nothing \
           has been printed on this line yet, then don't skip \
           since we're already positioned where they want to \
           be. Otherwise if something was already printed on \
           the current line or we're not on the channel they \
           want, then we need to perform the skip immediate. \
           See page 25 of GA24-3312-09 "IBM 2821 Control Unit \
           Component Description". \
        */ \
        if (1 \
            && dev->skpimmed \
            && dev->destline == dev->currline \
            && !dev->sp0after \
        ) \
        { \
            *unitstat = CSW_CE | CSW_DE; \
            dev->sp0after = 0; \
            return; \
        } \
        /* We need to perform this skip to channel */ \
        dev->sp0after = 0; \
        if (dev->destline <= dev->currline || dev->ffpend) \
        { \
            dev->ffpend = 0; \
            write_buffer (dev, "\f", 1, unitstat); \
            if (*unitstat != 0) return; \
            dev->currline = 1; \
        } \
        for (; dev->currline < dev->destline; dev->currline++ ) \
        { \
            write_buffer (dev, "\n", 1, unitstat); \
            if (*unitstat != 0) return; \
        } \
        *unitstat = CSW_CE | CSW_DE; \
        return; \
    } \
    dev->sp0after = 0; \
    /* channel not found */ \
    if (dev->fcbcheck) \
    { \
        dev->sense[0] = SENSE_EC; \
        *unitstat = CSW_CE | CSW_DE | CSW_UC; \
        return; \
    } \
    else \
    { \
        if ((code & 0x02) != 0) \
        { \
            write_buffer (dev, "\n", 1, unitstat); \
            if (*unitstat != 0) return; \
        } \
    } \
} while (0)

static void* spthread ( void* dev );        /*  (forward reference)  */

/*-------------------------------------------------------------------*/
/* Dump the FCB info                                                 */
/*-------------------------------------------------------------------*/
static void fcb_dump(DEVBLK* dev, char *buf, unsigned int buflen)
{
    int i;
    char wrk[16];
    char sep[1];
    sep[0] = '=';
    snprintf(buf, buflen-1, "LOADED lpi=%d index=%d lpp=%d fcb", dev->lpi, dev->index, dev->lpp );
    for (i = 1; i <= dev->lpp; i++)
    {
        if (dev->fcb[i] != 0)
        {
            MSGBUF( wrk, "%c%d:%d",sep[0], i, dev->fcb[i]);
            sep[0] = ',';
            if (strlen(buf) + strlen(wrk) >= buflen - 4)
            {
                /* Too long, truncate it */
                strlcat(buf, ",...", buflen);
                return;
            }
            strlcat(buf, wrk, buflen);
        }
    }
    return;
}

/*-------------------------------------------------------------------*/
/* Sockdev "OnConnection" callback function                          */
/*-------------------------------------------------------------------*/
static int onconnect_callback (DEVBLK* dev)
{
    TID tid;
    int rc;

    rc = create_thread( &tid, DETACHED, spthread, dev, NULL );
    if(rc)
    {
        // "Error in function create_thread(): %s"
        WRMSG( HHC00102, "E", strerror( rc ) );
        return 0;
    }
    return 1;
}

/*-------------------------------------------------------------------*/
/* Thread to monitor the sockdev remote print spooler connection     */
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
        char thread_name[32];
        thread_name[sizeof(thread_name)-1] = 0;
        snprintf( thread_name, sizeof(thread_name)-1,
            "spthread %1d:%04X", SSID_TO_LCSS(dev->ssid), dev->devnum );
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

    obtain_lock( &dev->lock );

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
        // "%1d:%04X Printer: client %s, ip %s disconnected from device %s"
        WRMSG (HHC01100, "I", SSID_TO_LCSS(dev->ssid), dev->devnum,
               dev->bs->clientname, dev->bs->clientip, dev->bs->spec);
    }

    release_lock( &dev->lock );

    return NULL;

} /* end function spthread */

/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
static int printer_init_handler (DEVBLK *dev, int argc, char *argv[])
{
char *ptr;                              /* Work variable for parsing */
char *nxt;                              /* Work variable for parsing */
int   iarg,i,j;                         /* Some array subscripts     */
U8    sockdev = FALSE;                  /* TRUE == is socket device  */

    /* For re-initialisation, close the existing file, if any, and raise attention */
    if (dev->fd >= 0)
    {
        (dev->hnd->close)(dev);

        release_lock (&dev->lock);
        device_attention (dev, CSW_DE);
        obtain_lock (&dev->lock);
    }

    dev->excps = 0;

    /* Forcibly disconnect anyone already currently connected */
    if (dev->bs && !unbind_device_ex(dev,1))
        return -1; // (error msg already issued)

    /* The first argument is the file name */
    if (argc == 0 || strlen(argv[0]) >= sizeof(dev->filename))
    {
        // "%1d:%04X Printer: file name missing or invalid"
        WRMSG (HHC01101, "E", SSID_TO_LCSS(dev->ssid), dev->devnum);
        return -1;
    }

    /* Save the file name in the device block */
    hostpath(dev->filename, argv[0], sizeof(dev->filename));

    if(!sscanf(dev->typname,"%hx",&(dev->devtype)))
        dev->devtype = 0x3211;

    /* Initialize device dependent fields */
    dev->fd = -1;
    dev->diaggate = 0;
    dev->fold = 0;
    dev->crlf = 0;
    dev->stopdev = FALSE;
    dev->notrunc = 0;
    dev->ispiped = (dev->filename[0] == '|');


    /* initialize the new fields for FCB+ support */
    dev->rawcc = 0;
    dev->fcbcheck = 1;
    dev->ffpend = 0;
    dev->sp0after = 0;

    dev->prevline = 1;
    dev->currline = 1;
    dev->destline = 1;

    dev->lpi = 6;
    dev->index = 0;
    dev->fcbisdef = 0;
    dev->lpp = FCBMASK[0];
    dev->chan12line = FCBMASK[12];

    /* Initialize Carriage Control Tape and FCB */
    memset( &dev->cctape, 0, sizeof( dev->cctape ));
    memset( &dev->fcb,    0, sizeof( dev->fcb    ));
    for (i=0; i < arraysize( cctape ); i++)
    {
        const U16 chan12mask = 0x8000 >> (12 - 1);
        dev->cctape[i] = cctape[i];
        if (cctape[i] & chan12mask)
            ASSERT( FCBMASK[12] == (i+1));
    }
    for (i=1; i <= 12; i++)
    {
        if (FCBMASK[i] != 0)
            dev->fcb[FCBMASK[i]] = i;
    }

    /* Process the driver arguments */
    for (iarg = 1; iarg < argc; iarg++)
    {
        if (strcasecmp(argv[iarg], "crlf") == 0)
        {
            dev->crlf = 1;
            continue;
        }

        /* sockdev means the device file is actually
           a connected socket instead of a disk file.
           The file name is the socket_spec (host:port)
           to listen for connections on.
        */
        if (!dev->ispiped && strcasecmp(argv[iarg], "sockdev") == 0)
        {
            sockdev = TRUE;
            continue;
        }

        if (strcasecmp(argv[iarg], "noclear") == 0)
        {
            dev->notrunc = 1;
            continue;
        }

        if (strcasecmp(argv[iarg], "rawcc") == 0)
        {
            dev->rawcc = 1;
            continue;
        }

        if (strcasecmp(argv[iarg], "nofcbcheck") == 0)
        {
            if (dev->devtype != 0x3211)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG (HHC01109, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "option 'nofcbcheck'", dev->devtype);
                return -1;
            }
            dev->fcbcheck = 0;
            continue;
        }

        if (strcasecmp(argv[iarg], "fcbcheck") == 0)
        {
            if (dev->devtype != 0x3211)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG (HHC01109, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "option 'fcbcheck'", dev->devtype);
                return -1;
            }
            dev->fcbcheck = 1;
            continue;
        }

        if (strncasecmp("lpi=", argv[iarg], 4) == 0)
        {
            ptr = argv[iarg]+4;
            errno = 0;
            dev->lpi = (int) strtoul(ptr,&nxt,10);
            if (errno != 0 || nxt == ptr || *nxt != 0 || ( dev->lpi != 6 && dev->lpi != 8 ) )
            {
                j = ptr+1 - argv[iarg];
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                return -1;
            }
            continue;
        }

        if (strncasecmp("lpp=", argv[iarg], 4) == 0)
        {
            ptr = argv[iarg]+4;
            errno = 0;
            dev->lpp = (int) strtoul(ptr,&nxt,10);
            if (errno != 0 || nxt == ptr || *nxt != 0 || dev->lpp > FCBSIZE)
            {
                j = ptr+1 - argv[iarg];
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                return -1;
            }
            continue;
        }

        if (strncasecmp("index=", argv[iarg], 6) == 0)
        {
            if (dev->devtype != 0x3211 )
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG (HHC01109, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "option 'index'", dev->devtype);
                return -1;
            }
            ptr = argv[iarg]+6;
            errno = 0;
            dev->index = (int) strtoul(ptr,&nxt,10);
            if (errno != 0 || nxt == ptr || *nxt != 0 || ( dev->index < 0 || dev->index > 15) )
            {
                j = ptr+1 - argv[iarg];
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                return -1;
            }
            continue;
        }

        if (strncasecmp("fcb=", argv[iarg], 4) == 0)
        {
            int line, chan;
            if (dev->devtype != 0x3211)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG (HHC01109, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "option 'fcb'", dev->devtype);
                return -1;
            }
            for (line = 0; line <= FCBSIZE; line++)  dev->fcb[line] = 0;
            /* check for simple mode */
            if ( strstr(argv[iarg],":") )
            {
                /* ':" found  ==> new mode ==> "fcb=ll:cc,..." */
                ptr = argv[iarg]+4;
                while (*ptr)
                {
                    // Parse line number
                    errno = 0;
                    line = (int) strtoul(ptr,&nxt,10);
                    if (errno != 0 || *nxt != ':' || nxt == ptr || line > dev->lpp || dev->fcb[line] != 0 )
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                        return -1;
                    }

                    ptr = nxt + 1;  // (next token)

                    // Parse channel number
                    errno = 0;
                    chan = (int) strtoul(ptr,&nxt,10);
                    if (errno != 0 || (*nxt != ',' && *nxt != 0) || nxt == ptr || chan < 1 || chan > 12 )
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                        return -1;
                    }

                    // Save the line number for this channel
                    dev->fcb[line] = chan;

                    // Are we done parsing yet?
                    if ( *nxt == 0 )
                        break;

                    ptr = nxt + 1;  // (next token)
                }
            }
            else
            {
                /* ':" NOT found  ==> old mode ==> "fcb=66010713192531374361495561" */
                char c;
                ptr = argv[iarg]+4;
                if (strlen(ptr) != 26)
                {
                    j = ptr+1 - argv[iarg];
                    // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                    WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                    return -1;
                }
                errno = 0;
                c = ptr[2];
                ptr[2] = 0;
                dev->lpp = (int) strtoul(ptr,&nxt,10);
                ptr[2] = c;
                if (errno != 0 || nxt != (ptr+2) || dev->lpp < 1 || dev->lpp > FCBSIZE)
                {
                    j = ptr+1 - argv[iarg];
                    // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                    WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                    return -1;
                }
                chan = 0;
                for (ptr+=2; c; ptr+=2)
                {
                    c = ptr[2];
                    ptr[2] = 0;
                    line = (int) strtoul(ptr,&nxt,10);
                    ptr[2] = c;
                    if (errno != 0 || nxt != (ptr+2) || line < 1 || line > dev->lpp)
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                        return -1;
                    }
                    chan += 1;
                    if (chan > 12)
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                        return -1;
                    }
                    dev->fcb[line] = chan;
                }
                if (chan != 12)
                {
                    j = 5;
                    // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                    WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                    return -1;
                }
            }

            continue;
        } /* "fcb=" */

        if (strncasecmp("cctape=", argv[iarg], 7) == 0)
        {
            int line, chan;
            if (dev->devtype != 0x1403)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG (HHC01109, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "option 'cctape'", dev->devtype);
                return -1;
            }
            /* Format:  "cctape=(line=chan,line=(chan,chan)...)"  */
            ptr = argv[iarg]+7;  /* get past "cctape=" */
            if (ptr[0] != '(')
            {
                j = 8;
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                return -1;
            }
            ptr++;  /* get past '(' */
            memset( &dev->cctape, 0, sizeof( dev->cctape ));
            errno = 0;
            /* Keep parsing until closing ')' found */
            while (ptr[0] && ptr[0] != ')')
            {
                line = (int) strtoul(ptr,&nxt,10);
                if (errno != 0 || *nxt != '=' || line < 1 || line > dev->lpp)
                {
                    j = ptr+1 - argv[iarg];
                    // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                    WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                    return -1;
                }
                /* get past '=' to first chan# or '(' start of chan# list */
                ptr = nxt+1;
                if (ptr[0] == '(')  /* start of chan# list? */
                {
                    ptr++;  /* point to '(' list begin */
                    /* Keep parsing chan#s until ')' end of list */
                    while (ptr[0] && ptr[0] != ')')
                    {
                        chan = (int) strtoul(ptr,&nxt,10);
                        if (errno != 0 || (*nxt != ',' && *nxt != ')') || chan < 1 || chan > 12)
                        {
                            j = ptr+1 - argv[iarg];
                            // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                            WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                            return -1;
                        }
                        dev->cctape[line-1] |= 0x8000 >> (chan - 1);
                        if ((ptr=nxt)[0] == ',')
                            ptr++; /* get past ',' to next chan# */

                    } /* while (ptr[0] && ptr[0] != ')') */

                    /* Check for proper ')' end of list */
                    if (ptr[0] != ')')
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                        return -1;
                    }
                    nxt = ptr+1; /* get past ')' to maybe ',' */
                }
                else /* ptr --> chan# or something else */
                {
                    chan = (int) strtoul(ptr,&nxt,10);
                    if (errno != 0 || (*nxt != ',' && *nxt != ')') || chan < 1 || chan > 12)
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                        return -1;
                    }
                    dev->cctape[line-1] |= 0x8000 >> (chan - 1);
                }
                if ((ptr=nxt)[0] == ',')
                    ptr++; /* get past ',' to next line# */

            } /* while (ptr[0] && ptr[0] != ')') */

            /* Check for proper closing ')' found and nothing after it */
            if (ptr[0] != ')' || (ptr+=1)[0] != 0)
            {
                j = ptr+1 - argv[iarg];
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG (HHC01103, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg], j);
                return -1;
            }

            continue;
        } /* "cctape=" */


        /* INSERT NEW DEVICE OPTIONS HERE... */


        // "%1d:%04X Printer: argument %d parameter '%s' is invalid"
        WRMSG (HHC01102, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, iarg + 1, argv[iarg]);
        return -1;

    } /* for (iarg = 1; iarg < argc; iarg++) Process the driver arguments */

    /* Check for incompatible options... */

    if (sockdev && dev->crlf)
    {
        // "%1d:%04X Printer: option %s is incompatible"
        WRMSG (HHC01104, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "sockdev/crlf");
        return -1;
    }

    if (sockdev && dev->notrunc)
    {
        // "%1d:%04X Printer: option %s is incompatible"
        WRMSG (HHC01104, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "sockdev/noclear");
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

    /* Set length of print buffer */
//  dev->bufsize = LINE_LENGTH + 8;
    dev->bufsize = BUFF_SIZE + BUFF_OVFL;
    dev->bufres = BUFF_SIZE;
    dev->bufoff = 0;

    /* Set number of sense bytes */
    dev->numsense = 1;

    /* Initialize the device identifier bytes */
    dev->devid[0] = 0xFF;
    dev->devid[1] = 0x28; /* Control unit type is 2821-1 */
    dev->devid[2] = 0x21;
    dev->devid[3] = 0x01;
    dev->devid[4] = dev->devtype >> 8;
    dev->devid[5] = dev->devtype & 0xFF;
    dev->devid[6] = 0x01;
    dev->numdevid = 7;

    /* Activate I/O tracing */
//  dev->ccwtrace = 1;

    return 0;
} /* end function printer_init_handler */

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void printer_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    BEGIN_DEVICE_CLASS_QUERY( "PRT", dev, devclass, buflen, buffer );

    snprintf (buffer, buflen-1, "%s%s%s%s%s%s%s IO[%"PRIu64"]",
                 dev->filename,
                (dev->bs      ? " sockdev"   : ""),
                (dev->crlf    ? " crlf"      : ""),
                (dev->notrunc ? " noclear"   : ""),
                (dev->rawcc   ? " rawcc"     : ""),
                (dev->devtype == 0x3211 ?
                (dev->fcbcheck ? " fcbcheck" : " nofcbcheck") : ""),
                (dev->stopdev ? " (stopped)" : ""),
                dev->excps );

} /* end function printer_query_device */

/*-------------------------------------------------------------------*/
/* Subroutine to open the printer file or pipe                       */
/*-------------------------------------------------------------------*/
static int
open_printer (DEVBLK *dev)
{
pid_t           pid;                    /* Child process identifier  */
int             open_flags;             /* File open flags           */
#if !defined( _MSVC_ )
int             pipefd[2];              /* Pipe descriptors          */
int             rc;                     /* Return code               */
#endif

    /* Regular open if 1st char of filename is not vertical bar */
    if (!dev->ispiped)
    {
        int fd;

        /* Socket printer? */
        if (dev->bs)
            return (dev->fd < 0 ? -1 : 0);

        /* Normal printer */
        open_flags = O_BINARY | O_WRONLY | O_CREAT /* | O_SYNC */;
        if (!dev->notrunc)
            open_flags |= O_TRUNC;
        fd = HOPEN (dev->filename, open_flags,
                    S_IRUSR | S_IWUSR | S_IRGRP);
        if (fd < 0)
        {
            // "%1d:%04X Printer: error in function %s: %s"
            WRMSG (HHC01105, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "open()", strerror(errno));
            return -1;
        }

        /* Save file descriptor in device block and return */
        dev->fd = fd;
        return 0;
    }

    /* Filename is in format |xxx, set up pipe to program xxx */

#if defined( _MSVC_ )

    /* "Poor man's" fork... */
    pid = w32_poor_mans_fork ( dev->filename+1, &dev->fd );
    if (pid < 0)
    {
        // "%1d:%04X Printer: error in function %s: %s"
        WRMSG (HHC01105, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "fork()", strerror(errno));
        return -1;
    }

    // "%1d:%04X Printer: pipe receiver with pid %d starting"
    WRMSG (HHC01106, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, pid);
    dev->ptpcpid = pid;

#else /* !defined( _MSVC_ ) */

    /* Create a pipe */
    rc = create_pipe (pipefd);
    if (rc < 0)
    {
        // "%1d:%04X Printer: error in function %s: %s"
        WRMSG (HHC01105, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "create_pipe()", strerror(errno));
        return -1;
    }

    /* Fork a child process to receive the pipe data */
    pid = fork();
    if (pid < 0)
    {
        // "%1d:%04X COMM: outgoing call failed during %s command: %s"
        WRMSG (HHC01005, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "fork()", strerror(errno));
        close_pipe ( pipefd[0] );
        close_pipe ( pipefd[1] );
        return -1;
    }

    /* The child process executes the pipe receiver program... */
    if (pid == 0)
    {
        // "%1d:%04X Printer: pipe receiver with pid %d starting"
        WRMSG (HHC01106, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, getpid());

        /* Close the write end of the pipe */
        close_pipe ( pipefd[1] );

        /* Duplicate the read end of the pipe onto STDIN */
        if (pipefd[0] != STDIN_FILENO)
        {
            rc = dup2 (pipefd[0], STDIN_FILENO);
            if (rc != STDIN_FILENO)
            {
                // "%1d:%04X Printer: error in function %s: %s"
                WRMSG (HHC01105, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "dup2()", strerror(errno));
                close_pipe ( pipefd[0] );
                _exit(127);
            }
        } /* end if(pipefd[0] != STDIN_FILENO) */

        /* Close the original descriptor now duplicated to STDIN */
        close_pipe ( pipefd[0] );

        /* Redirect stderr (screen) to hercules log task */
        dup2(STDOUT_FILENO, STDERR_FILENO);

        /* Relinquish any ROOT authority before calling shell */
        SETMODE(TERM);

        /* Execute the specified pipe receiver program */
        rc = system (dev->filename+1);

        if (rc == 0)
        {
            // "%1d:%04X Printer: pipe receiver with pid %d terminating"
            WRMSG (HHC01107, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, getpid());
        }
        else
        {
            // "%1d:%04X Printer: unable to execute file %s: %s"
            WRMSG (HHC01108, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->filename+1, strerror(errno));
        }

        /* The child process terminates using _exit instead of exit
           to avoid invoking the panel atexit cleanup routine */
        _exit(rc);

    } /* end if(pid==0) */

    /* The parent process continues as the pipe sender */

    /* Close the read end of the pipe */
    close_pipe ( pipefd[0] );

    /* Save pipe write descriptor in the device block */
    dev->fd = pipefd[1];
    dev->ptpcpid = pid;

#endif /* defined( _MSVC_ ) */

    return 0;

} /* end function open_printer */

/*-------------------------------------------------------------------*/
/* Subroutine to write data to the printer                           */
/*-------------------------------------------------------------------*/
static void
write_buffer (DEVBLK *dev, char *buf, int len, BYTE *unitstat)
{
int rc;

    /* Write data to the printer file */
    if (dev->bs)
    {
        /* (socket printer) */
        rc = write_socket (dev->fd, buf, len);

        /* Check for socket error */
        if (rc < len)
        {
            /* Close the connection */
            if (dev->fd != -1)
            {
                int fd = dev->fd;
                dev->fd = -1;
                close_socket( fd );
                // "%1d:%04X Printer: client %s, ip %s disconnected from device %s"
                WRMSG (HHC01100, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->bs->clientname, dev->bs->clientip, dev->bs->spec);
            }

            /* Set unit check with intervention required */
            dev->sense[0] = SENSE_IR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
    }
    else
    {
        /* Write data to the printer file */
        rc = write (dev->fd, buf, len);

        /* Equipment check if error writing to printer file */
        if (rc < len)
        {
            // "%1d:%04X Printer: error in function %s: %s"
            WRMSG (HHC01105, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "write()",
                   rc < 0 ? strerror(errno) : "incomplete record written");
            dev->sense[0] = SENSE_EC;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
    }

} /* end function write_buffer */

/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/*-------------------------------------------------------------------*/
static int printer_close_device ( DEVBLK *dev )
{
int fd = dev->fd;

    if (fd == -1)
        return 0;

    dev->fd      = -1;
    dev->stopdev =  FALSE;

    /* Close the device file */
    if ( dev->ispiped )
    {
#if !defined( _MSVC_ )
        close_pipe (fd);
#else /* defined( _MSVC_ ) */
        close (fd);
        // "%1d:%04X Printer: pipe receiver with pid %d terminating"
        WRMSG (HHC01107, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->ptpcpid);
#endif /* defined( _MSVC_ ) */
        dev->ptpcpid = 0;
    }
    else
    {
        if (dev->bs)
        {
            close_socket (fd);
            // "%1d:%04X Printer: client %s, ip %s disconnected from device %s"
            WRMSG (HHC01100, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, dev->bs->clientname, dev->bs->clientip, dev->bs->spec);
        }
        else
        {
            /* Regular printer */
            close (fd);
        }
    }

    return 0;
} /* end function printer_close_device */

/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
static void printer_execute_ccw (DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual)
{
int             rc = 0;                 /* Return code               */
U32             i;                      /* Loop counter              */
U32             num;                    /* Number of bytes to move   */
char           *eor;                    /* -> end of record string   */
char           *nls = "\n\n\n";         /* -> new lines              */
BYTE            c;                      /* Print character           */
char            hex[3];                 /* for hex conversion        */
char            wbuf[150];

    /* Reset flags at start of CCW chain */
    if (chained == 0)
        dev->diaggate = 0;

    /* Open the device file if necessary */
    if (dev->fd < 0 && !IS_CCW_SENSE(code))
        rc = open_printer (dev);
    else
    {
        /* If printer stopped, return intervention required */
        if (dev->stopdev && !IS_CCW_SENSE(code))
            rc = -1;
        else
            rc = 0;
    }

    if (rc < 0)
    {
        /* Set unit check with intervention required */
        dev->sense[0] = SENSE_IR;
        *unitstat = CSW_UC;
        return;
    }

    /* Reset skip immediate flag before processing CCW opcode */
    dev->skpimmed = 0;

    /* Process depending on CCW opcode */
    switch (code) {

    /*---------------------------------------------------------------*/
    /* NOP                                                           */
    /*---------------------------------------------------------------*/
    case 0x03: /* No Operation                   */

        *unitstat = CSW_CE | CSW_DE;
        break;

    /*---------------------------------------------------------------*/
    /* WRITE DATA THEN MOVE CARRIAGE COMMANDS                        */
    /*---------------------------------------------------------------*/
    case 0x01:  /*  Write Without Spacing         */
    case 0x09:  /*  Write and Space 1 Line        */
    case 0x11:  /*  Write and Space 2 Lines       */
    case 0x19:  /*  Write and Space 3 Lines       */
    case 0x89:  /*  Write and Skip to Channel 1   */
    case 0x91:  /*  Write and Skip to Channel 2   */
    case 0x99:  /*  Write and Skip to Channel 3   */
    case 0xA1:  /*  Write and Skip to Channel 4   */
    case 0xA9:  /*  Write and Skip to Channel 5   */
    case 0xB1:  /*  Write and Skip to Channel 6   */
    case 0xB9:  /*  Write and Skip to Channel 7   */
    case 0xC1:  /*  Write and Skip to Channel 8   */
    case 0xC9:  /*  Write and Skip to Channel 9   */
    case 0xD1:  /*  Write and Skip to Channel 10  */
    case 0xD9:  /*  Write and Skip to Channel 11  */
    case 0xE1:  /*  Write and Skip to Channel 12  */

        if (dev->rawcc)
        {
            sprintf(hex,"%02x",code);
            write_buffer(dev, hex, 2, unitstat);
            if (*unitstat != 0) return;
            WRITE_LINE();
            write_buffer(dev, "\n", 1, unitstat);
            if (*unitstat == 0)
                *unitstat = CSW_CE | CSW_DE;
            return;
        }

        WRITE_LINE();
        if ((flags & CCW_FLAGS_CD) == 0)
        {
            if (code <= 0x80) /* line control? */
            {
                int coun = code / 8;
                if (coun == 0)
                {
                    write_buffer(dev, "\r", 1, unitstat);
                    if (*unitstat == 0)
                        *unitstat = CSW_CE | CSW_DE;
                    return;
                }
                SPACE_LINES();
                return;
            }
            else /* code > 0x80: chan control */
            {
                int chan = (code - 128) / 8;
                if (chan == 1)
                {
                    write_buffer(dev, "\r", 1, unitstat);
                    if (*unitstat != 0)
                        return;
                }
                SKIP_TO_CHAN();
                if (*unitstat == 0)
                    *unitstat = CSW_CE | CSW_DE;
                return;
            }

        }
        *unitstat = CSW_CE | CSW_DE;
        return;

    /*---------------------------------------------------------------*/
    /* CONTROL IMMEDIATE COMMANDS                                    */
    /*---------------------------------------------------------------*/
    case 0x0B:  /*  Space 1 Line       Immediate  */
    case 0x13:  /*  Space 2 Lines      Immediate  */
    case 0x1B:  /*  Space 3 Lines      Immediate  */
    case 0x8B:  /*  Skip to Channel 1  Immediate  */
    case 0x93:  /*  Skip to Channel 2  Immediate  */
    case 0x9B:  /*  Skip to Channel 3  Immediate  */
    case 0xA3:  /*  Skip to Channel 4  Immediate  */
    case 0xAB:  /*  Skip to Channel 5  Immediate  */
    case 0xB3:  /*  Skip to Channel 6  Immediate  */
    case 0xBB:  /*  Skip to Channel 7  Immediate  */
    case 0xC3:  /*  Skip to Channel 8  Immediate  */
    case 0xCB:  /*  Skip to Channel 9  Immediate  */
    case 0xD3:  /*  Skip to Channel 10 Immediate  */
    case 0xDB:  /*  Skip to Channel 11 Immediate  */
    case 0xE3:  /*  Skip to Channel 12 Immediate  */

        dev->skpimmed = (code & 0x80) ? 1 : 0;

        if (dev->rawcc)
        {
            sprintf(hex,"%02x",code);
            write_buffer(dev, hex, 2, unitstat);
            if (*unitstat != 0) return;
            eor = (dev->crlf) ? "\r\n" : "\n";
            write_buffer(dev, eor, (int)strlen(eor), unitstat);
            if (*unitstat == 0)
                *unitstat = CSW_CE | CSW_DE;
            return;
        }

        if (code <= 0x80) /* line control? */
        {
            int coun = code / 8;
            SPACE_LINES();
        }
        else /* code > 0x80: chan control */
        {
            int chan = (code - 128) / 8;
            SKIP_TO_CHAN();
            if (*unitstat == 0)
                *unitstat = CSW_CE | CSW_DE;
        }
        return;

    case 0x63:
    /*---------------------------------------------------------------*/
    /* LOAD FORMS CONTROL BUFFER                                     */
    /*---------------------------------------------------------------*/
        if (dev->devtype == 0x1403)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }
        if (dev->rawcc)
        {
            sprintf(hex,"%02x",code);
            write_buffer(dev, hex, 2, unitstat);
            if (*unitstat != 0) return;
            for (i = 0; i < count; i++)
            {
                sprintf(hex,"%02x",iobuf[i]);
                dev->buf[i*2] = hex[0];
                dev->buf[i*2+1] = hex[1];
            } /* end for(i) */
            write_buffer(dev, (char *)dev->buf, i*2, unitstat);
            if (*unitstat != 0) return;
            eor = (dev->crlf) ? "\r\n" : "\n";
            write_buffer(dev, eor, (int)strlen(eor), unitstat);
            if (*unitstat != 0) return;
        }
        else
        {
            U32 i = 0;
            int j = 1;
            int more = 1;
            for (i = 0; i <= FCBSIZE; i++) dev->fcb[i] = 0;
            i = 0;

            dev->lpi = 6;
            dev->index = 0;
            i = 0;
            if (iobuf[0] & 0xc0)
            {
                /* First byte is a print position index */
                if ((iobuf[0] & 0xc0) == 0x80)
                    /* Indexing right */
                    dev->index = iobuf[0] & 0x1f;
                else
                    /* Indexing left */
                    dev->index = - (iobuf[0] & 0x1f);
                i = 1;
            }

            for (; i < count && j <= FCBSIZE && more; i++, j++)
            {
                dev->fcb[i] = iobuf[i] & 0x0f;
                if (dev->fcb[j] > 12)
                {
                    *residual = count - i;
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_CC;
                    return;
                }

                if (iobuf[i] & 0x10)
                {
                    /* Flag bit is on */
                    if (j == 1)
                        /* Flag bit in first byte means eight lines per inch */
                        dev->lpi = 8;
                    else
                        more = 0;
                }
            }
            if (more)
            {
                /* No flag in last byte or too many bytes */
                *residual = count - i;
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                dev->sense[0] = SENSE_CC;
                return;
            }
            *residual = count - i;
            dev->lpp = j - 1;

            fcb_dump(dev, wbuf, 150);
            // "%1d:%04X %s"
            WRMSG(HHC02210, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, wbuf );
        }
        /* Return normal status */
        *residual = 0;
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x06:
    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC CHECK READ                                         */
    /*---------------------------------------------------------------*/
        /* If not 1403, reject if not preceded by DIAGNOSTIC GATE */
        if (dev->devtype != 0x1403 && !dev->diaggate)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x07:
    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC GATE                                               */
    /*---------------------------------------------------------------*/
        /* Command reject if 1403, or if chained to another CCW
           except a no-operation at the start of the CCW chain */
        if (dev->devtype == 0x1403 || ccwseq > 1
            || (chained && prevcode != 0x03))
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set diagnostic gate flag */
        dev->diaggate = 1;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x0A:
    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC READ UCS BUFFER                                    */
    /*---------------------------------------------------------------*/
        /* Reject if 1403 or not preceded by DIAGNOSTIC GATE */
        if (dev->devtype == 0x1403 || !dev->diaggate)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x12:
    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC READ FCB                                           */
    /*---------------------------------------------------------------*/
        /* Reject if 1403 or not preceded by DIAGNOSTIC GATE */
        if (dev->devtype == 0x1403 || !dev->diaggate)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }
        /*
            The read FCB command, normally for diagnostic purposes,
            transfers the carriage codes to the channel, where they
            may be analyzed or compared. The number of data bytes
            transferred is controlled by the count field in the CCW.

            For example, to check the carriage setting in the first
            address position for the presence or absence of a flag
            bit to determine the carriage space setting of six or
            eight lines per inch, set the count field to 1.

            NOTE: To maintain vertical forms alignment, issue the
            read FCB command, only after skipping to a channel code
            stored in FCB address one. If read FCB is given at any
            other position, the form becomes misaligned because the
            FCB returns to address position one.
        */
        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x23:
    /*---------------------------------------------------------------*/
    /* UNFOLD                                                        */
    /*---------------------------------------------------------------*/
        if (dev->devtype == 0x1403)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }
        dev->fold = 0;
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x43:
    /*---------------------------------------------------------------*/
    /* FOLD                                                          */
    /*---------------------------------------------------------------*/
        if (dev->devtype == 0x1403)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }
        dev->fold = 1;
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x73:
    /*---------------------------------------------------------------*/
    /* BLOCK DATA CHECK                                              */
    /*---------------------------------------------------------------*/
    /*
        *residual = 0;
    */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x7B:
    /*---------------------------------------------------------------*/
    /* ALLOW DATA CHECK                                              */
    /*---------------------------------------------------------------*/
    /*
        *residual = 0;
    */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xEB:
    /*---------------------------------------------------------------*/
    /* UCS GATE LOAD                                                 */
    /*---------------------------------------------------------------*/
        /* Command reject if not first command in chain */
        if (chained != 0)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xF3:
    /*---------------------------------------------------------------*/
    /* LOAD UCS BUFFER AND FOLD                                      */
    /*---------------------------------------------------------------*/
        /* For 1403, command reject if not chained to UCS GATE */
        /* Also allow ALLOW DATA CHECK to get TSS/370 working  */
        /* -- JRM 11/28/2007 */
        if (dev->devtype == 0x1403 &&
            ((prevcode != 0xEB) && (prevcode != 0x7B)))
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set fold indicator and return normal status */
        dev->fold = 1;
        dev->ffpend = 1;
    /*
        *residual = 0;
    */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0xFB:
    /*---------------------------------------------------------------*/
    /* LOAD UCS BUFFER (NO FOLD)                                     */
    /*---------------------------------------------------------------*/
        /* For 1403, command reject if not chained to UCS GATE */
        if (dev->devtype == 0x1403 && prevcode != 0xEB)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Reset fold indicator and return normal status */
        dev->fold = 0;
        dev->ffpend = 1;

    /*
        *residual = 0;
    */
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

        /* SENSE ID is only supported if LEGACYSENSEID is ON;
         * otherwise, fall through to invalid operation.
         */
        if (sysblk.legacysenseid)
        {
            /* Calculate residual byte count */
            num = (count < dev->numdevid) ? count : dev->numdevid;
            *residual = count - num;
            if (count < dev->numdevid) *more = 1;

            /* Copy device identifier bytes to channel I/O buffer */
            memcpy (iobuf, dev->devid, num);

            /* Return unit status */
            *unitstat = CSW_CE | CSW_DE;
            break;
        }

    default:
    /*---------------------------------------------------------------*/
    /* INVALID OPERATION                                             */
    /*---------------------------------------------------------------*/
        /* Set command reject sense byte, and unit check status */
        dev->sense[0] = SENSE_CR;
        *unitstat = CSW_UC;

    } /* end switch(code) */

} /* end function printer_execute_ccw */


#if defined(OPTION_DYNAMIC_LOAD)
static
#endif
DEVHND printer_device_hndinfo = {
        &printer_init_handler,         /* Device Initialisation      */
        &printer_execute_ccw,          /* Device CCW execute         */
        &printer_close_device,         /* Device Close               */
        &printer_query_device,         /* Device Query               */
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
        printer_immed_commands,        /* Immediate CCW Codes        */
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
#if !defined(HDL_BUILD_SHARED) && defined(HDL_USE_LIBTOOL)
#define hdl_ddev hdt1403_LTX_hdl_ddev
#define hdl_depc hdt1403_LTX_hdl_depc
#define hdl_reso hdt1403_LTX_hdl_reso
#define hdl_init hdt1403_LTX_hdl_init
#define hdl_fini hdt1403_LTX_hdl_fini
#endif

#if defined(OPTION_DYNAMIC_LOAD)
HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
     HDL_DEPENDENCY(DEVBLK);
}
END_DEPENDENCY_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE(1403, printer_device_hndinfo );
    HDL_DEVICE(3211, printer_device_hndinfo );
}
END_DEVICE_SECTION
#endif
