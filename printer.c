/* PRINTER.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Enrico Sorichetti, 2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2017          */
/*              Line Printer Device Handler                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for emulated       */
/* System/370 line printer devices with FCB support and more.        */
/*-------------------------------------------------------------------*/
/* Reference:                                                        */
/*    GA24-3073-8 "IBM 1403 Printer Component Description"           */
/*    GA24-3312-9 "IBM 2821 Control Unit Component Description"      */
/*    GA33-1529-0 "IBM 3203 Printer Model 5 Component Description"   */
/*    GA24-3543-0 "IBM 3211 Printer and 3811 Control Unit            */
/*                                  Component Description"           */
/*    https://www.ibm.com/support/knowledgecenter/SSLTBW_2.1.0       */
/*           /com.ibm.zos.v2r1.idas300/fcbim.htm                     */
/*    https://www.ibm.com/support/knowledgecenter/SSLTBW_2.1.0       */
/*           /com.ibm.zos.v2r1.idas300/s3270.htm                     */
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
/* The following are considered IMMEDIATE commands for the 1403 and  */
/* 3211 printers: CTL-NOOP, Space Lines Immediate, Skip to Channel   */
/* Immediate, Block Data check, Allow Data Check, Load UCS Buffer,   */
/* Load UCS Buffer (No Fold), Diagnostic Gate, UCS Gate Load, Raise  */
/* Cover, Fold, Unfold.                                              */
/*-------------------------------------------------------------------*/

static BYTE  printer_immed_commands[ 256 ] =
/*0 1 2 3 4 5 6 7 8 9 A B C D E F*/
{ 0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0, // 0:    03,  0B,  07
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // 1:    13,  1B
  0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, // 2:    23
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 3:
  0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, // 4:    43
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 5:
  0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0, // 6:         6B
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // 7:    73,  7B
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // 8:    83,  8B
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // 9:    93,  9B
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // A:    A3,  AB
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // B:    B3,  BB
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // C:    C3,  CB
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // D:    D3,  DB
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0, // E:    E3,  EB
  0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0};// F:    F3,  FB

/*-------------------------------------------------------------------*/
/* The 3203-5 printer however, implements ALL control commands as    */
/* non-immediate commands, meaning you must specify the SLI bit in   */
/* your CCW or else you will get an incorrect length error.          */
/*-------------------------------------------------------------------*/

static BYTE  prt3203_immed_commands[ 256 ] =
/*0 1 2 3 4 5 6 7 8 9 A B C D E F*/
{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 1:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 2:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 3:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 4:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 5:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 6:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 7:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 8:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 9:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // A:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // B:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // C:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // D:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // E:
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};// F:

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define BUFF_SIZE       MAX(MAX(MAX_FCBSIZE,MAX_UCBSIZE),MAX_PLBSIZE)
#define LINENUM(n)      (1 + (((n)-1) % dev->lpp))
#define CCWOP2LINES(op) ((op)/8)
#define CCWOP2CHAN(op)  (((op)-128)/8)
#define ISSKPIMMED(op)  (((op) & 0x80) ? 1 : 0)
#define FCBSIZE(dev)    ((0x3211 == (dev)->devtype) ? FCBSIZE_3211 : FCBSIZE_OTHER)
#define UCBSIZE(dev)    ((0x1403 == (dev)->devtype) ? UCBSIZE_1403 : \
                         (0x3203 == (dev)->devtype) ? UCBSIZE_3203 : \
                                                      UCBSIZE_3211 /* (presumed) */ )
#define PLBSIZE(dev)    ((0x3211 == (dev)->devtype) ? PLBSIZE_3211 : PLBSIZE_OTHER)
#define MIN_PLB_INDEX   (-31)
#define MAX_PLB_INDEX   (+31)
#define MAX_CHAN_STOPS  (30)    // (or +1 if stop on last line)

/*-------------------------------------------------------------------*/
/*             DEFAULT 3203/3211 FORMS CONTROL BUFFERS               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  First entry (entry 0) is the number of lines per page. The next  */
/*  12 entries are the line numbers assigned to channels 1 thru 12.  */
/*                                                                   */
/*-------------------------------------------------------------------*/

const int fcbmask_hardware [ MAX_FCBSIZE ] = {66, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//                           channel:             1  2  3  4  5  6  7  8  9 10 11 12
const int fcbmask_legacy   [ MAX_FCBSIZE ] = {66, 1, 7,13,19,25,31,37,43,63,49,55,61};
//                           channel:             1  2  3  4  5  6  7  8  9 10 11 12
const int fcbmask_fcb2std2 [ MAX_FCBSIZE ] = {66, 1, 7,13,19,25,31,37,43,49,55,61,63};
//                           channel:             1  2  3  4  5  6  7  8  9 10 11 12
const int fcbmask_fcb2id1  [ MAX_FCBSIZE ] = {88, 1, 8,15,22,29,36,43,50,57,64,71,78};

/*-------------------------------------------------------------------*/
/*             DEFAULT 1403-ONLY CARRIAGE CONTROL TAPE               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Each entry in the below table corresponds to a line on a page.   */
/*  The first entry (index 0) is for line 1, the second (index 1)    */
/*  is for line 2, etc.                                              */
/*                                                                   */
/*  Each entry is a bitmask indicating which of the 12 possible      */
/*  channels that are punched on that line.  It is thus possible     */
/*  to not only specify the same channel punch on different lines    */
/*  but also to have more than one channel punch on the same line.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Table entry channel numbers, for reference:                      */
/*                                                                   */
/*    0x8000   1      0x1000   4      0x0200   7      0x0040  10     */
/*    0x4000   2      0x0800   5      0x0100   8      0x0020  11     */
/*    0x2000   3      0x0400   6      0x0080   9      0x0010  12     */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*
    PROGRAMMMING NOTE:  the below cctape value SHOULD match
                        the same corresponding fcb value!
*/
const U16 cctape_legacy[ MAX_FCBSIZE ] = {
/* 1       2       3       4       5       6       7       8       9      10        lines  */
0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000, /*  1 - 10 */
0x0000, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x0000, /* 11 - 20 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 21 - 30 */
0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000, /* 31 - 40 */
0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000, /* 41 - 50 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 51 - 60 */
0x0010, 0x0000, 0x0080, 0x0000, 0x0000, 0x0000, };                              /* 61 - 66 */
/*
    PROGRAMMMING NOTE:  the below cctape value SHOULD match
                        the same corresponding fcb value!
*/
const U16 cctape_fcb2std2[ MAX_FCBSIZE ] = {
/* 1       2       3       4       5       6       7       8       9      10        lines  */
0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000, /*  1 - 10 */
0x0000, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x0000, /* 11 - 20 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 21 - 30 */
0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000, /* 31 - 40 */
0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0080, 0x0000, /* 41 - 50 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 51 - 60 */
0x0020, 0x0000, 0x0010, 0x0000, 0x0000, 0x0000, };                              /* 61 - 66 */
/*
    PROGRAMMMING NOTE:  the below cctape value SHOULD match
                        the same corresponding fcb value!
*/
const U16 cctape_fcb2id1[ MAX_FCBSIZE ] = {
/* 1       2       3       4       5       6       7       8       9      10        lines  */
0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, /*  1 - 10 */
0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 11 - 20 */
0x0000, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000, /* 21 - 30 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000, 0x0000, 0x0000, 0x0000, /* 31 - 40 */
0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, /* 41 - 50 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0080, 0x0000, 0x0000, 0x0000, /* 51 - 60 */
0x0000, 0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 61 - 70 */
0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0010, 0x0000, 0x0000, /* 71 - 80 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, };              /* 81 - 88 */

/*-------------------------------------------------------------------*/
/* Predefined fcb/cctape values                                      */
/*-------------------------------------------------------------------*/
typedef const int    FCBMASK, *PFCBMASK;
typedef const U16    CCTAPE,  *PCCTAPE;

#define FCBTYPE_FCB         0
#define FCBTYPE_CCTAPE      1

typedef struct _FCBTAB
{
    const BYTE  type;       // FCBTYPE_FCB or FCBTYPE_CCTAPE
    const char* name;       // "legacy", "fcb2std2", etc.
    const void* image;      // PFCBMASK or PCCTAPE
}
FCBTAB, *PFCBTAB;

static FCBTAB fcbtab[] =    // table of predefined fcb/cctape names
{
    { FCBTYPE_FCB,    "legacy",   fcbmask_legacy   },
    { FCBTYPE_FCB,    "fcb2std",  fcbmask_fcb2std2 },
    { FCBTYPE_FCB,    "fcb2id1",  fcbmask_fcb2id1  },
    { FCBTYPE_FCB,    "hardware", fcbmask_hardware },

    { FCBTYPE_CCTAPE, "legacy",   cctape_legacy    },
    { FCBTYPE_CCTAPE, "fcb2std2", cctape_fcb2std2  },
    { FCBTYPE_CCTAPE, "fcb2id1",  cctape_fcb2id1   },
};

/*-------------------------------------------------------------------*/
/*  Write data to printer.    Return 0 if successful, else unitstat. */
/*-------------------------------------------------------------------*/
static BYTE write_buffer( DEVBLK* dev, const char* buf, int len, BYTE* unitstat )
{
int rc;

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
                // "%1d:%04X Printer: client %s, IP %s disconnected from device %s"
                WRMSG( HHC01100, "I", LCSS_DEVNUM,
                    dev->bs->clientname, dev->bs->clientip, dev->bs->spec );
            }

            /* Set unit check with intervention required */
            dev->sense[0] = SENSE_IR;
            return *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
    }
    else /* regular print file */
    {
        /* Write data to the printer file, check for error */
        if ((rc = write( dev->fd, buf, len )) < len)
        {
            // "%1d:%04X %s: error in function %s: %s"
            WRMSG( HHC01250, "E", LCSS_DEVNUM,
                   "Printer", "write()",
                   rc < 0 ? strerror( errno )
                          : "incomplete record written");

            /* Set Equipment Check */
            dev->sense[0] = SENSE_EC;
            return *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
    }

    return 0;   /* Successful completion */
}

/*-------------------------------------------------------------------*/
/*          SpaceLines        Return 0 if successful, else unitstat. */
/*-------------------------------------------------------------------*/
static BYTE SpaceLines( DEVBLK* dev, BYTE* unitstat, int spaceamt )
{
    const char* buf;
    int         len;
    int         newline, oldline = dev->currline;

    ASSERT( spaceamt >= 0 && spaceamt <= 3 );   // (sanity check)

    if (!(len = spaceamt))
    {
        buf = "\r";
        len = 1;
    }
    else
        buf = "\n\n\n";

    /* Space the requested number of lines */
    if (write_buffer( dev, buf, len, unitstat ) != 0)
        return *unitstat;  /* I/O error */

    /* Start with normal status */
    *unitstat = CSW_CE | CSW_DE;

    /* Bump current line number and save */
    newline = oldline + spaceamt;
    dev->currline = LINENUM( newline );

    /* Check if channel 12 was passed */
    if (1
        &&            dev->chan12line
        && oldline <  dev->chan12line
        && newline >= dev->chan12line
    )
    {
        *unitstat |= CSW_UX;    /* Unit Exception */
    }

    /* Check if channel 9 was passed */
    if (1
        &&            dev->chan9line
        && oldline <  dev->chan9line
        && newline >= dev->chan9line
    )
    {
        /* Set unit check with channel-9 */
        *unitstat |= CSW_UC;
        dev->sense[0] = SENSE_CH9;
    }

    /* Return success or failure */
    return (*unitstat & CSW_UC) ? *unitstat : 0;
}

/*-------------------------------------------------------------------*/
/*          WriteLine         Return 0 if successful, else unitstat. */
/*-------------------------------------------------------------------*/
static BYTE WriteLine
(
    DEVBLK* dev,
    BYTE    code,
    BYTE    flags,
    BYTE    chained,
    U32     count,
    BYTE*   iobuf,
    BYTE*   unitstat,
    U32*    residual
)
{
    U32  i, num;
    BYTE c;

    /* Start a new record if not data-chained from previous CCW */
    if (!(chained & CCW_FLAGS_CD))
    {
        dev->bufoff = 0;
        dev->bufres = BUFF_SIZE;
        memset( dev->plb, 0, sizeof( dev->plb ));
    }

    /* Handle indexing. For compatibility with the 3211 Printer,
       the optional indexing byte is accepted but ignored by the
       3203-5.  Positive indexing (indent) is handled here.
       Negative indexing (chop) is handled further below.
    */
    if (dev->index > 0 && dev->devtype != 0x3203)
    {
        for (i=1; i < (U32) dev->index; i++)
        {
            dev->buf[ dev->bufoff ] = SPACE;
            dev->bufoff++;
            dev->bufres--;
        }
    }

    /* Calculate number of bytes to write and set residual count */
    num = (count < (U32) dev->bufres) ? count : (U32) dev->bufres;
    *residual = count - num;

    /* Copy data from the channel buffer to the device buffer.
       Negative indexing (chop) is handled here.
    */
    i = (U32) ((dev->index < 0 && dev->devtype != 0x3203) ?
               -dev->index : 0); /* Chop if requested */
    for (; i < num; i++)
    {
        /* Copy print line to PLB */
        dev->plb[ dev->bufoff ] = iobuf[i];

        /* Translate EBCDIC to ASCII */
        c = guest_to_host( dev->plb[ dev->bufoff ] );

        if (!c)
            c = SPACE;
        else if (dev->fold)
            c = toupper((unsigned char)c);

        /* Copy to device output buffer */
        dev->buf[ dev->bufoff ] = c;

        dev->bufoff++;
        dev->bufres--;
    }

    /* Perform end of record processing if not data-chaining */
    if (!(flags & CCW_FLAGS_CD))
    {
        /* (trim trailing blanks...) */

        for (i = dev->bufoff; i > 0; i--)
            if (dev->buf[ i - 1 ] != SPACE)
                break;

        /* Print the line unless this is a Diagnostic Write.
           Diagnostic Writes update the Print Line buffer (PLB)
           but otherwise don't actually print anything nor move
           the carriage. They're like Write Without Spacing but
           without the write.
        */
        if (code != 0x05)    /* If NOT diagnostic write... */
        {
            /* Write print line... */
            if (write_buffer( dev, (char*) dev->buf, i, unitstat ) != 0)
                return *unitstat; /* (I/O error) */

            if (dev->crlf)
                if (write_buffer( dev, "\r", 1, unitstat ) != 0)
                    return *unitstat; /* (I/O error) */
        }

        dev->sp0after = (code == 0x01) ? 1 : 0;
    }

    return 0;   /* Successful completion */
}

/*-------------------------------------------------------------------*/
/*          SkipToChannel     Return 0 if successful, else unitstat. */
/*-------------------------------------------------------------------*/
static BYTE SkipToChannel
(
    DEVBLK* dev,
    BYTE    code,
    BYTE*   unitstat,
    int     chan
)
{
    int  line, destline;
    U8   found  = FALSE;     /* TRUE/FALSE whether channel was found */

    UNREFERENCED( code );

    if (dev->devtype == 0x1403)
    {
        /* 1403: Search carriage control tape for channel punch */

        U16  chan_mask  = 0x8000 >> (chan - 1);

        for (line = LINENUM( dev->currline + 1);
            !(dev->cctape[ line-1 ] & chan_mask) && line != dev->currline;
            line = LINENUM( line+1 )
        )
            ;   /* (do nothing) */

        if (dev->cctape[ line-1 ] & chan_mask)
        {
            found = TRUE;
            destline = line;
        }
    }
    else /* 3203/3211: Search FCB image for channel number */
    {
        for (line = LINENUM( dev->currline + 1 );
            dev->fcb[ line ] != chan && line != dev->currline;
            line = LINENUM( line+1 )
        )
            ;   /* (do nothing) */

        if (dev->fcb[ line ] == chan)
        {
            found = TRUE;
            destline = line;
        }
    }

    if (found)
    {
        /*  Write and skip:

              If a 'write and skip' command orders the carriage to go
              to the channel at which it is presently located, the
              carriage moves until that channel is again detected.

            Skip immediate:

              If the carriage is at the channel, the command does not
              cause  any carriage motion unless the previous command
              was 'write without spacing'.
        */
        if (1
            && dev->skpimmed
            && destline == dev->currline
            && !dev->sp0after
        )
        {
            /* Already at desired channel; do nothing. */
            *unitstat = CSW_CE | CSW_DE;
            dev->sp0after = 0;
        }
        else /* We need to perform this skip to channel */
        {
            dev->sp0after = 0;

            if (destline <= dev->currline || dev->ffpend)
            {
                dev->ffpend = 0;

                if (write_buffer( dev, "\f", 1, unitstat ) != 0)
                    return *unitstat; /* (I/O error) */

                dev->currline = 1;
            }

            for (; dev->currline < destline; dev->currline++ )
                if (write_buffer( dev, "\n", 1, unitstat ) != 0)
                    return *unitstat; /* (I/O error) */
        }
    }
    else /* Channel not found! */
    {
        if (dev->devtype == 0x1403)
        {
            /*  PROGRAMMING NOTE: skipping to a channel that did not
            **  exist on a 1403 printer's carriage control tape would
            **  cause the printer to begin spewing page after page of
            **  paper as the printer kept looking for the non-existent
            **  channel punch missing from the carriage control tape
            **  until the operator eventually noticed and pressed the
            **  printer STOP button causing an Intervention Required.
            */
            dev->stopdev = TRUE;
            dev->sense[0] = SENSE_IR;
        }
        else // 3211, 3203-5
        {
            /* This condition automatically halts the skip
               at the SECOND occurrence of FCB address one. */

            /* TECHNIQUE: first, calculate the number of lines we
               need to advance to reach line number 1 and then print
               that many newlines. Then simply print a number of new-
               lines equivalent to the number of lines per page thus
               keeping us positioned on line number 1 of the FCB.
            */
            int newlines = (dev->lpp - dev->currline) + 1;

            for (line=0; line < newlines; line++)
                if (write_buffer( dev, "\n", 1, unitstat ) != 0)
                    return *unitstat; /* (I/O error) */

            newlines = dev->lpp;

            for (line=0; line < newlines; line++)
                if (write_buffer( dev, "\n", 1, unitstat ) != 0)
                    return *unitstat; /* (I/O error) */

            /* Set the appropriate SENSE bits */
            dev->sense[0] = SENSE_EC + SENSE_DC;
            dev->sense[1] = SENSE1_LPC;
        }

        dev->sp0after = 0;
        dev->ffpend = 0;

        /* Return unit check status */
        return *unitstat = CSW_CE | CSW_DE | CSW_UC;
    }

    /* Set normal ending status */
    *unitstat = CSW_CE | CSW_DE;

    return 0;   /* Successful completion */
}

/*-------------------------------------------------------------------*/
/*          DoSpaceOrSkip     Return 0 if successful, else unitstat. */
/*-------------------------------------------------------------------*/
static BYTE DoSpaceOrSkip( DEVBLK* dev, BYTE code, BYTE* unitstat )
{
    BYTE rc;

    dev->skpimmed = ISSKPIMMED( code );

    if (code <= 0x80)
        rc = SpaceLines( dev, unitstat, CCWOP2LINES( code ));
    else
        rc = SkipToChannel( dev, code, unitstat, CCWOP2CHAN( code ));

    return rc ? *unitstat : 0;
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
            // "%1d:%04X Printer: client %s, IP %s disconnected from device %s"
            WRMSG (HHC01100, "I", LCSS_DEVNUM,
                   dev->bs->clientname, dev->bs->clientip, dev->bs->spec);
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

/*-------------------------------------------------------------------*/
/* A new fcb has been loaded                                         */
/*-------------------------------------------------------------------*/
static void on_new_fcb( DEVBLK* dev )
{
    int line, chan1line;

    /* Display FCB just loaded if verbose messages enabled */
    if (MLVL( VERBOSE ))
    {
        char fcbfmt[256];
        char loaded[256];

        FormatFCB( fcbfmt, sizeof( fcbfmt ),
            dev->index, dev->lpi, dev->lpp, dev->fcb );

        MSGBUF( loaded, "LOADED %s", fcbfmt );

        // "%1d:%04X %s"
        WRMSG( HHC02210, "I", SSID_TO_LCSS( dev->ssid ),
            dev->devnum, loaded );
    }

    /* Scan new fcb and: 1) save new channel-9 and channel-12 line
       numbers, and 2) issue a warning if channel-1 wasn't defined.
    */
    dev->currline   = 1;    // (reset due to new FCB load)
    dev->chan9line  = 0;    // (reset due to new FCB load)
    dev->chan12line = 0;    // (reset due to new FCB load)

    for (chan1line=0, line=1; line <= dev->lpp; line++)
    {
        if      (dev->fcb[ line ] ==  1)      chan1line  = line;
        else if (dev->fcb[ line ] ==  9) dev->chan9line  = line;
        else if (dev->fcb[ line ] == 12) dev->chan12line = line;
    }

    if (!chan1line)
    {
        // "%1d:%04X Printer: channel 1 is undefined"
        WRMSG( HHC01111, "W", LCSS_DEVNUM );
    }
}

/*-------------------------------------------------------------------*/
/* A new cctape has been loaded                                      */
/*-------------------------------------------------------------------*/
static void on_new_cctape( DEVBLK* dev )
{
    int line, chan1line;

    /* Scan new cctape and: 1) save new channel-9 and channel-12 line
       numbers, and 2) issue a warning if channel-1 wasn't defined.
    */
    dev->chan9line  = 0;    // (reset due to new cctape)
    dev->chan12line = 0;    // (reset due to new cctape)
    dev->currline   = 1;    // (reset due to new cctape)

    for (line=0, chan1line=0; line < (int) _countof( dev->cctape); line++)
    {
        if      (dev->cctape[ line ] & 0x8000)      chan1line  = line+1;
        else if (dev->cctape[ line ] & 0x0080) dev->chan9line  = line+1;
        else if (dev->cctape[ line ] & 0x0010) dev->chan12line = line+1;
    }

    if (!chan1line)
    {
        // "%1d:%04X Printer: channel 1 is undefined"
        WRMSG( HHC01111, "W", LCSS_DEVNUM );
    }
}

/*-------------------------------------------------------------------*/
/*          LoadUCB           Return 0 if successful, else unitstat. */
/*-------------------------------------------------------------------*/
static BYTE LoadUCB
(
    DEVBLK* dev,
    U32     count,
    BYTE*   iobuf,
    BYTE*   more,
    BYTE*   unitstat,
    U32*    residual
)
{
    U32 num, ucbsize = UCBSIZE( dev );

    UNREFERENCED( more );

    /* Calculate residual byte count */
    num = (count < ucbsize) ? count : ucbsize;
    *residual = count - num;

    /* Make sure enough data was provided */
    if (count < ucbsize)
    {
        /* Set unit check with SENSE = Load Check */
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        dev->sense[0] = SENSE_LDCK;
        return *unitstat;
    }

    /* Load new UCB */
    memcpy( dev->ucb, iobuf, ucbsize );

    return 0;
}

/*-------------------------------------------------------------------*/
/* Validate defined cctape value with respect to defined lpp value   */
/*-------------------------------------------------------------------*/
static BYTE valid_cctape( DEVBLK* dev )
{
    int line;
    int cctape_size = FCBSIZE( dev );

    /* PROGRAMMING NOTE: we cannot use dev->lpp for the exit condition
       since we need to always scan the entire cctape since they might
       have defined channel stops beyond the dev->lpp entry, which is
       the very condition we're supposed to detect.
    */
    for (line=0; line < cctape_size; line++)
    {
        if (!dev->cctape[ line ])
            continue;

        if ((line+1) > dev->lpp)
        {
            // "%1d:%04X Printer: incompatible '%s' and 'lpp' values detected"
            WRMSG( HHC01113, "E", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, "cctape" );
            return FALSE;
        }
    }

    return TRUE;
}

/*-------------------------------------------------------------------*/
/* Validate defined fcb value with respect to defined lpp value     */
/*-------------------------------------------------------------------*/
static BYTE valid_fcb( DEVBLK* dev )
{
    int line;
    int fcbsize = FCBSIZE( dev );
    int chanstops = MAX_CHAN_STOPS;

    /* PROGRAMMING NOTE: we cannot use dev->lpp for the exit condition
       because we need to always scan the entire fcb since they might
       have defined channel stops defined beyond the dev->lpp entry,
       which is one of the very conditions we're supposed to detect.
    */
    for (line=1; line < fcbsize; line++)
    {
        if (!dev->fcb[ line ])
            continue;

        if (line > dev->lpp)
        {
            // "%1d:%04X Printer: incompatible '%s' and 'lpp' values detected"
            WRMSG( HHC01113, "E", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, "fcb" );
            return FALSE;
        }

        /* PROGRAMMING NOTE: a maximum of 30 channel codes can be
           specified if the end of sheet is not coded in the same
           byte as a channel code; a maximum of 31 channel codes
           can be specified if the end of sheet code is coded in
           the same byte as a channel code. Thus we only decrement
           our counter if this is not the last line.
        */
        if (line != dev->lpp && --chanstops < 0)
        {
            // "%1d:%04X Printer: invalid fcb: maximum channel codes exceeded"
            WRMSG( HHC01112, "E", SSID_TO_LCSS( dev->ssid ),
                dev->devnum );
            return FALSE;
        }
    }

    return TRUE;
}

/*-------------------------------------------------------------------*/
/* Format interpretation of first two sense bytes                    */
/*-------------------------------------------------------------------*/
static void format_sense( const DEVBLK* dev, char* buf, size_t bufsz )
{
    snprintf( buf, bufsz, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"

        , (dev->sense[0] & SENSE_CR   ) ? "CMDREJ " : ""
        , (dev->sense[0] & SENSE_IR   ) ? "INTREQ " : ""
        , (dev->sense[0] & SENSE_BOC  ) ? "BUSCK "  : ""
        , (dev->sense[0] & SENSE_EC   ) ? "EQPCK "  : ""
        , (dev->sense[0] & SENSE_DC   ) ? "DATAC "  : ""
        , (dev->sense[0] & SENSE_OR   ) ? "OVRUN "  : ""
        , (dev->sense[0] & SENSE_LDCK ) ? "LOADCK " : ""
        , (dev->sense[0] & SENSE_CH9  ) ? "CHAN9 "  : ""

        , (dev->sense[1] & SENSE1_PER ) ? "--- "    : ""
        , (dev->sense[1] & SENSE1_PRTC) ? "PRTCK "  : ""
        , (dev->sense[1] & SENSE1_QUAL) ? "QUAL "   : ""
        , (dev->sense[1] & SENSE1_LPC ) ? "POSCK "  : ""
        , (dev->sense[1] & SENSE1_FORM) ? "FORMCK " : ""
        , (dev->sense[1] & SENSE1_CS  ) ? "CMDSUP " : ""
        , (dev->sense[1] & SENSE1_MECH) ? "MECHM "  : ""
        , (dev->sense[1] & SENSE1_IE  ) ? "--- "    : ""
    );
}

// (forward reference)
static int open_printer( DEVBLK* dev );

/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
static int printer_init_handler( DEVBLK* dev, int argc, char *argv[] )
{
char *ptr;                              /* Work variable for parsing */
char *nxt;                              /* Work variable for parsing */
int   iarg, i, j;                       /* Some array subscripts     */
U8    sockdev = FALSE;                  /* TRUE == is socket device  */
int   fcbsize;                          /* FCB size for this devtype */

    dev->sns = format_sense;            /* Sense formatting fuction  */

    /* For re-initialisation, close the existing file, if any, and raise attention */
    if (dev->fd >= 0)
    {
        (dev->hnd->close)( dev );

        RELEASE_DEVLOCK( dev );
        {
            device_attention( dev, CSW_DE );
        }
        OBTAIN_DEVLOCK( dev );
    }

    dev->excps = 0;

    /* Forcibly disconnect anyone already currently connected */
    if (dev->bs && !unbind_device_ex( dev, 1 ))
        return -1; // (error msg already issued)

    /* The first argument is the file name */
    if (argc == 0 || strlen( argv[0] ) >= sizeof( dev->filename ))
    {
        // "%1d:%04X Printer: file name missing or invalid"
        WRMSG( HHC01101, "E", LCSS_DEVNUM );
        return -1;
    }

    /* Save the file name in the device block */
    hostpath( dev->filename, argv[0], sizeof( dev->filename ));

    /* Save the device type */
    sscanf( dev->typname, "%hx", &dev->devtype );
    if (1
        && 0x1403 != dev->devtype
        && 0x3203 != dev->devtype
        && 0x3211 != dev->devtype
    )
    {
        // "%1d:%04X Printer: unsupported device type %04X"
        WRMSG( HHC01105, "E", LCSS_DEVNUM,
            dev->devtype );
        return -1;
    }
    fcbsize = FCBSIZE( dev ); // (now that we know the device type)

    /* Initialize the device identifier bytes */
    dev->devid[0] = 0xFF;
    dev->devid[1] = 0x28; /* Control unit type is 2821-1 */
    dev->devid[2] = 0x21;
    dev->devid[3] = 0x01;
    dev->devid[4] = dev->devtype >> 8;
    dev->devid[5] = dev->devtype & 0xFF;
    dev->devid[6] = 0x01;
    dev->numdevid = 7;

    /* Set number of sense bytes */
    dev->numsense = 2;

    /* Initialize device dependent fields */
    dev->fd       = -1;
    dev->crlf     = 0;
    dev->stopdev  = FALSE;
    dev->append   = 0;
    dev->ispiped  = (dev->filename[0] == '|');

    /* Set length of print buffer */
    dev->bufsize = BUFF_SIZE;
    dev->bufres  = BUFF_SIZE;
    dev->bufoff  = 0;

    /* Initialize Forms Control Buffer and Carriage Control Tape */
    dev->lpi      = 6;
    dev->index    = 0;
    dev->diaggate = 0;
    dev->fold     = 0;
    dev->fcbcheck = 1;  /* (DEPRECATED!) */
    dev->ffpend   = 0;
    dev->sp0after = 0;
    dev->currline = 1;

    memcpy( dev->fcb,    fcbmask_legacy, sizeof( dev->fcb    ));
    memcpy( dev->cctape, cctape_legacy,  sizeof( dev->cctape ));

    dev->lpp = dev->fcb[0];

    if (dev->fcbname)
    {
        free( dev->fcbname );
        dev->fcbname = NULL;
    }
    dev->fcbname = strdup( "legacy" );

    /* Process the driver arguments...
       PLEASE TRY TO KEEP THE BELOW IN ALPHABETICAL ORDER.
    */
    for (iarg=1; iarg < argc; iarg++)
    {
        if (strcasecmp( argv[iarg], "append" ) == 0)
        {
            dev->append = 1;
            continue;
        }

        if (strncasecmp( "cctape=", argv[iarg], 7 ) == 0)
        {
            int line, chan, found;
            if (dev->devtype != 0x1403)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG( HHC01109, "E", LCSS_DEVNUM,
                    "option 'cctape'", dev->devtype );
                return -1;
            }
            /*  "cctape=(ll=cc,ll=(cc,cc)...)"
                "cctape=name"
            */
            ptr = argv[iarg] + 7;  /* get past "cctape=" */
            if (ptr[0] != '(')
            {
                /* Doesn't look like they're trying to define a custom
                   cctape. See if they specified a cctape name instead.
                */
                FCBTAB* table = fcbtab;

                for (i=0, found=0; i < (int) _countof( fcbtab ); i++)
                {
                    if (FCBTYPE_CCTAPE != table[i].type)
                        continue;

                    /* Is this the one they want? */
                    if (strcasecmp( table[i].name, ptr ) == 0)
                    {
                        found = 1;
                        memcpy( dev->cctape, table[i].image, sizeof( dev->cctape ));
                        if (dev->fcbname) free( dev->fcbname );
                        dev->fcbname = strdup( table[i].name );
                        break;
                    }
                }
                if (!found)
                {
                    j = 8;
                    // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                    WRMSG( HHC01103, "E", LCSS_DEVNUM,
                        iarg + 1, argv[ iarg ], j );
                    return -1;
                }
            }
            else
            {
                ptr++;  /* get past '(' */
                memset( dev->cctape, 0, sizeof( dev->cctape ));
                errno = 0;
                /* Keep parsing until closing ')' found */
                while (ptr[0] && ptr[0] != ')')
                {
                    line = (int) strtoul( ptr, &nxt, 10 );
                    if (0
                        || errno != 0
                        || *nxt != '='
                        || line < 1
                        || line > fcbsize
                    )
                    {
                        j = ptr+1 - argv[ iarg ];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG( HHC01103, "E", LCSS_DEVNUM,
                            iarg + 1, argv[ iarg ], j );
                        return -1;
                    }
                    /* get past '=' to first chan# or '(' start of chan# list */
                    ptr = nxt + 1;
                    if (ptr[0] == '(')  /* start of chan# list? */
                    {
                        ptr++;  /* point to '(' list begin */
                        /* Keep parsing chan#s until ')' end of list */
                        while (ptr[0] && ptr[0] != ')')
                        {
                            chan = (int) strtoul( ptr, &nxt, 10 );
                            if (0
                                || errno != 0
                                || (*nxt != ',' && *nxt != ')')
                                || chan < 1
                                || chan > 12
                            )
                            {
                                j = ptr+1 - argv[ iarg ];
                                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                                WRMSG( HHC01103, "E", LCSS_DEVNUM,
                                    iarg + 1, argv[ iarg ], j );
                                return -1;
                            }
                            dev->cctape[ line-1 ] |= 0x8000 >> (chan - 1);
                            if ((ptr = nxt)[0] == ',')
                                ptr++; /* get past ',' to next chan# */

                        } /* while (ptr[0] && ptr[0] != ')') */

                        /* Check for proper ')' end of list */
                        if (ptr[0] != ')')
                        {
                            j = ptr+1 - argv[iarg];
                            // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                            WRMSG( HHC01103, "E", LCSS_DEVNUM,
                                iarg + 1, argv[ iarg ], j );
                            return -1;
                        }
                        nxt = ptr+1; /* get past ')' to maybe ',' */
                    }
                    else /* ptr --> chan# or something else */
                    {
                        chan = (int) strtoul( ptr, &nxt, 10 );
                        if (0
                            || errno != 0
                            || (*nxt != ',' && *nxt != ')')
                            || chan < 1
                            || chan > 12
                        )
                        {
                            j = ptr+1 - argv[ iarg ];
                            // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                            WRMSG( HHC01103, "E", LCSS_DEVNUM,
                                iarg + 1, argv[ iarg ], j );
                            return -1;
                        }
                        dev->cctape[ line-1 ] |= 0x8000 >> (chan - 1);
                    }
                    if ((ptr = nxt)[0] == ',')
                        ptr++; /* get past ',' to next line# */

                } /* while (ptr[0] && ptr[0] != ')') */

                /* Check for proper closing ')' found and nothing after it */
                if (ptr[0] != ')' || (ptr += 1)[0] != 0)
                {
                    j = ptr+1 - argv[ iarg ];
                    // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                    WRMSG( HHC01103, "E", LCSS_DEVNUM,
                        iarg + 1, argv[ iarg ], j );
                    return -1;
                }
            }
            on_new_cctape( dev );
            continue;
        } /* "cctape=" */

        if (strcasecmp( argv[ iarg ], "crlf" ) == 0)
        {
            dev->crlf = 1;
            continue;
        }

        if (strncasecmp( "fcb=", argv[ iarg ], 4 ) == 0)
        {
            int line, chan, found;
            if (dev->devtype == 0x1403)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG( HHC01109, "E", LCSS_DEVNUM,
                    "option 'fcb'", dev->devtype );
                return -1;
            }
            memset( dev->fcb, 0, sizeof( dev->fcb ));

            /*  "fcb=fcb=cc:ll,..."
                "fcb=fcb=ppnn..."
                "fcb=name"
            */
            /* check for new format */
            if (strstr( argv[ iarg ], ":" ))
            {
                /* ':" found == new format ==> "fcb=cc:ll,..." */
                ptr = argv[ iarg ] + 4; /* get past "fcb=" */
                dev->fcb[0] = dev->lpp;
                while (*ptr)
                {
                    // Parse channel number
                    errno = 0;
                    chan = (int) strtoul( ptr, &nxt, 10 );
                    if (0
                        || errno != 0
                        || *nxt != ':'
                        || nxt == ptr
                        || chan < 0
                        || chan > 12
                    )
                    {
                        j = ptr+1 - argv[ iarg ];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG( HHC01103, "E", LCSS_DEVNUM,
                            iarg + 1, argv[ iarg ], j );
                        return -1;
                    }

                    ptr = nxt + 1;  // (next token)

                    // Parse line number
                    errno = 0;
                    line = (int) strtoul( ptr, &nxt, 10 );
                    if (0
                        || errno != 0
                        || (*nxt != ',' && *nxt != 0)
                        || nxt == ptr
                        || line < 0
                        || line > dev->lpp
                        || dev->fcb[ line ] != 0
                    )
                    {
                        j = ptr+1 - argv[ iarg ];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG( HHC01103, "E", LCSS_DEVNUM,
                            iarg + 1, argv[ iarg ], j );
                        return -1;
                    }

                    /* Register channel in FCB only if it's being defined */
                    if (line)
                        dev->fcb[ line ] = chan;

                    // Are we done parsing yet?
                    if (*nxt == 0)
                        break;

                    ptr = nxt + 1;  // (next token)
                }
            }
            else
            {
                /* ':" NOT found. old format or fcb name */
                char c;
                ptr = argv[ iarg ] + 4; /* get past "fcb=" */
                if (strlen( ptr ) != 26 || !isdigit( (unsigned char)*ptr ))
                {
                    /* It doesn't appear they're defining a custom
                       fcb. See if they gave us an fcb name instead.
                    */
                    FCBTAB* table = fcbtab;

                    for (i=0, found=0; i < (int) _countof( fcbtab ); i++)
                    {
                        if (FCBTYPE_FCB != table[i].type)
                            continue;

                        /* Is this the one they want? */
                        if (strcasecmp( table[i].name, ptr ) == 0)
                        {
                            found = 1;
                            memcpy( dev->fcb, table[i].image, sizeof( dev->fcb ));
                            if (dev->fcbname) free( dev->fcbname );
                            dev->fcbname = strdup( table[i].name );
                            break;
                        }
                    }
                    if (!found)
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG( HHC01103, "E", LCSS_DEVNUM,
                            iarg + 1, argv[ iarg ], j );
                        return -1;
                    }
                    dev->lpp = dev->fcb[0];
                }
                else
                {
                    /* old format ==> "fcb=66010713192531374361495561" */
                    errno = 0;
                    c = ptr[2];
                    ptr[2] = 0;
                    dev->lpp = (int) strtoul( ptr, &nxt, 10 );
                    ptr[2] = c;
                    if (0
                        || errno != 0
                        || nxt != (ptr+2)
                        || dev->lpp < 1
                        || dev->lpp > fcbsize
                    )
                    {
                        j = ptr+1 - argv[iarg];
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG( HHC01103, "E", LCSS_DEVNUM,
                            iarg + 1, argv[ iarg ], j );
                        return -1;
                    }
                    dev->fcb[0] = dev->lpp;
                    chan = 0;
                    for (ptr += 2; c; ptr += 2)
                    {
                        c = ptr[2];
                        ptr[2] = 0;
                        line = (int) strtoul( ptr, &nxt, 10 );
                        ptr[2] = c;
                        if (0
                            || errno != 0
                            || nxt != (ptr+2)
                            || line < 0
                            || line > dev->lpp
                        )
                        {
                            j = ptr+1 - argv[ iarg ];
                            // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                            WRMSG( HHC01103, "E", LCSS_DEVNUM,
                                iarg + 1, argv[ iarg ], j );
                            return -1;
                        }
                        chan += 1;
                        if (chan > 12)
                        {
                            j = ptr+1 - argv[ iarg ];
                            // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                            WRMSG( HHC01103, "E", LCSS_DEVNUM,
                                iarg + 1, argv[ iarg ], j );
                            return -1;
                        }

                        /* Register channel in FCB only if it's being defined */
                        if (line)
                            dev->fcb[line] = chan;
                    }
                    if (chan != 12)
                    {
                        j = 5;
                        // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                        WRMSG( HHC01103, "E", LCSS_DEVNUM,
                            iarg + 1, argv[ iarg ], j );
                        return -1;
                    }
                }
            }
            on_new_fcb( dev );
            continue;
        } /* "fcb=" */

        /* (DEPRECATED!) */
        if (strcasecmp( argv[ iarg ], "fcbcheck" ) == 0)
        {
            if (dev->devtype == 0x1403)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG( HHC01109, "E", LCSS_DEVNUM,
                    "option 'fcbcheck'", dev->devtype );
                return -1;
            }

            // "%1d:%04X %s: option '%s' has been deprecated"
            WRMSG( HHC01251, "W", LCSS_DEVNUM,
                "Printer", "fcbcheck" );

            dev->fcbcheck = 1;
            continue;
        }

        if (strncasecmp( "index=", argv[ iarg ], 6 ) == 0)
        {
            if (dev->devtype == 0x1403)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG( HHC01109, "E", LCSS_DEVNUM,
                    "option 'index'", dev->devtype );
                return -1;
            }
            ptr = argv[ iarg ] + 6;
            errno = 0;
            dev->index = (int) strtoul( ptr, &nxt, 10 );
            if (0
                || errno != 0
                || nxt == ptr
                || *nxt != 0
                || dev->index < MIN_PLB_INDEX
                || dev->index > MAX_PLB_INDEX
            )
            {
                j = ptr+1 - argv[iarg];
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG( HHC01103, "E", LCSS_DEVNUM,
                    iarg + 1, argv[ iarg ], j );
                return -1;
            }
            if (dev->devtype == 0x3203)
            {
                // "%1d:%04X Printer: Indexing accepted and ignored for 3203-5"
                WRMSG( HHC01110, "W", LCSS_DEVNUM );
            }
            continue;
        }

        if (strncasecmp( "lpi=", argv[ iarg ], 4 ) == 0)
        {
            ptr = argv[ iarg ] + 4;
            errno = 0;
            dev->lpi = (int) strtoul( ptr, &nxt, 10 );
            if (0
                || errno != 0
                || nxt == ptr
                || *nxt != 0
                || (dev->lpi != 6 && dev->lpi != 8)
            )
            {
                j = ptr+1 - argv[iarg];
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG( HHC01103, "E", LCSS_DEVNUM,
                    iarg + 1, argv[ iarg ], j );
                return -1;
            }
            continue;
        }

        if (strncasecmp( "lpp=", argv[ iarg ], 4 ) == 0)
        {
            ptr = argv[ iarg ] + 4;
            errno = 0;
            dev->lpp = (int) strtoul( ptr, &nxt, 10 );
            if (0
                || errno != 0
                || nxt == ptr
                || *nxt != 0
                || dev->lpp >= fcbsize
            )
            {
                j = ptr + 1 - argv[ iarg ];
                // "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
                WRMSG( HHC01103, "E", LCSS_DEVNUM,
                    iarg + 1, argv[ iarg ], j );
                return -1;
            }
            continue;
        }

        if (strcasecmp( argv[ iarg ], "noclear" ) == 0)
        {
            dev->append = 1;
            // "%1d:%04X %s: option '%s' has been deprecated"
            WRMSG( HHC01251, "W", LCSS_DEVNUM,
                "Printer", "noclear" );
            continue;
        }

        /* (DEPRECATED!) */
        if (strcasecmp( argv[ iarg ], "nofcbcheck" ) == 0)
        {
            if (dev->devtype == 0x1403)
            {
                // HHC01109 "%1d:%04X Printer: option %s incompatible with device type %04X"
                WRMSG( HHC01109, "E", LCSS_DEVNUM,
                    "option 'nofcbcheck'", dev->devtype );
                return -1;
            }

            // "%1d:%04X %s: option '%s' has been deprecated"
            WRMSG( HHC01251, "W", LCSS_DEVNUM,
                "Printer", "nofcbcheck" );

            //dev->fcbcheck = 0;      // IGNORED
            dev->fcbcheck = 1;      // FORCED
            continue;
        }

        /* sockdev means the device file is actually
           a connected socket instead of a disk file.
           The file name is the socket_spec (host:port)
           to listen for connections on.
        */
        if (!dev->ispiped && strcasecmp( argv[ iarg ], "sockdev" ) == 0)
        {
            sockdev = TRUE;
            continue;
        }

        // "%1d:%04X Printer: argument %d parameter '%s' is invalid"
        WRMSG( HHC01102, "E", LCSS_DEVNUM,
            iarg + 1, argv[ iarg ]);
        return -1;

    } /* for (iarg = 1; iarg < argc; iarg++) Process the driver arguments */

    /* Perform final fcb/cctape validation: for fcb only, verify there
       are not more than 30 channel stops defined. For both cctape and
       fcb, verify no channel stop is on a line greater than the lpp.
    */
    if (0x1403 == dev->devtype)
    {
        if (!valid_cctape( dev ))
            return -1;  // (error msg already issued)
    }
    else // 3203 or 3211
    {
        if (!valid_fcb( dev ))
            return -1;  // (error msg already issued)
    }

    /* Check for incompatible options... */

    if (sockdev && dev->crlf)
    {
        // "%1d:%04X Printer: option %s is incompatible"
        WRMSG( HHC01104, "E", LCSS_DEVNUM,
            "sockdev/crlf" );
        return -1;
    }

    if (sockdev && dev->append)
    {
        // "%1d:%04X Printer: option %s is incompatible"
        WRMSG( HHC01104, "E", LCSS_DEVNUM,
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
    if (!sockdev && open_printer( dev ) != 0)
        return -1;  // (error msg already issued)

    return 0;
} /* end function printer_init_handler */

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void printer_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "PRT", dev, devclass, buflen, buffer );

    snprintf (buffer, buflen, "%s%s%s%s%s IO[%"PRIu64"]",
                 filename,
                (dev->bs      ? " sockdev"   : ""),
                (dev->crlf    ? " crlf"      : ""),
                (dev->append  ? " append"    : ""),
                (dev->stopdev ? " (stopped)" : ""),
                 dev->excps );

} /* end function printer_query_device */

/*-------------------------------------------------------------------*/
/* Subroutine to open the printer file or pipe                       */
/*-------------------------------------------------------------------*/
static int open_printer( DEVBLK* dev )
{
pid_t           pid;                    /* Child process identifier  */
int             open_flags;             /* File open flags           */
#if !defined( _MSVC_ )
int             pipefd[2];              /* Pipe descriptors          */
#endif
int             rc;                     /* Return code               */
off_t           filesize = 0;           /* file size for ftruncate   */

    /* Regular open if 1st char of filename is not vertical bar */
    if (!dev->ispiped)
    {
        int    fd;

        /* Socket printer? */
        if (dev->bs)
            return (dev->fd < 0 ? -1 : 0);

        /* Normal printer */
        open_flags = O_BINARY | O_WRONLY | O_CREAT /* | O_SYNC */;

        if (!dev->append)
            open_flags |= O_TRUNC;

        if ((fd = HOPEN( dev->filename, open_flags, S_IRUSR | S_IWUSR | S_IRGRP )) < 0)
        {
            // "%1d:%04X %s: error in function %s: %s"
            WRMSG( HHC01250, "E", LCSS_DEVNUM,
                "Printer", "HOPEN()", strerror( errno ));
            return -1;
        }

        if (dev->append)
        {
            if ((filesize = lseek( fd, 0, SEEK_END )) < 0)
            {
                // "%1d:%04X %s: error in function %s: %s"
                WRMSG( HHC01250, "E", LCSS_DEVNUM,
                    "Printer", "lseek()", strerror( errno ));
                return -1;
            }
        }

        /* Save file descriptor in device block and return */
        dev->fd = fd;

        /* Set new physical EOF */
        do rc = ftruncate( dev->fd, filesize );
        while (EINTR == rc);

        return 0;
    }

    /* Filename is in format |xxx, set up pipe to program xxx */

#if defined( _MSVC_ )

    /* "Poor man's" fork... */
    pid = w32_poor_mans_fork ( dev->filename+1, &dev->fd );
    if (pid < 0)
    {
        // "%1d:%04X %s: error in function %s: %s"
        WRMSG( HHC01250, "E", LCSS_DEVNUM,
            "Printer", "fork()", strerror( errno ));
        return -1;
    }

    // "%1d:%04X Printer: pipe receiver with pid %d starting"
    WRMSG( HHC01106, "I", LCSS_DEVNUM, pid );
    dev->ptpcpid = pid;

#else /* !defined( _MSVC_ ) */

    /* Create a pipe */
    if ((rc = create_pipe( pipefd )) < 0)
    {
        // "%1d:%04X %s: error in function %s: %s"
        WRMSG( HHC01250, "E", LCSS_DEVNUM,
            "Printer", "create_pipe()", strerror( errno ));
        return -1;
    }

    /* Fork a child process to receive the pipe data */
    if ((pid = fork()) < 0)
    {
        // "%1d:%04X COMM: outgoing call failed during %s command: %s"
        WRMSG( HHC01005, "E", LCSS_DEVNUM,
            "fork()", strerror( errno ));
        close_pipe( pipefd[0] );
        close_pipe( pipefd[1] );
        return -1;
    }

    /* The child process executes the pipe receiver program... */
    if (pid == 0)
    {
        // "%1d:%04X Printer: pipe receiver with pid %d starting"
        WRMSG( HHC01106, "I", LCSS_DEVNUM, getpid() );

        /* Close the write end of the pipe */
        close_pipe( pipefd[1] );

        /* Duplicate the read end of the pipe onto STDIN */
        if (pipefd[0] != STDIN_FILENO)
        {
            if ((rc = dup2( pipefd[0], STDIN_FILENO )) != STDIN_FILENO)
            {
                // "%1d:%04X %s: error in function %s: %s"
                WRMSG( HHC01250, "E", LCSS_DEVNUM,
                    "Printer", "dup2()", strerror( errno ));
                close_pipe( pipefd[0] );
                _exit( 127 );
            }
        } /* end if (pipefd[0] != STDIN_FILENO) */

        /* Close the original descriptor now duplicated to STDIN */
        close_pipe( pipefd[0] );

        /* Redirect stderr (screen) to hercules log task */
        dup2( STDOUT_FILENO, STDERR_FILENO );

        /* Relinquish any ROOT authority before calling shell */
        SETMODE( TERM );

        /* Execute the specified pipe receiver program */
        rc = system( dev->filename+1 );

        if (rc == 0)
        {
            // "%1d:%04X Printer: pipe receiver with pid %d terminating"
            WRMSG( HHC01107, "I", LCSS_DEVNUM,
                getpid() );
        }
        else
        {
            // "%1d:%04X Printer: unable to execute file %s: %s"
            WRMSG( HHC01108, "E", LCSS_DEVNUM,
                dev->filename+1, strerror( errno ));
        }

        /* The child process terminates using _exit instead of exit
           to avoid invoking the panel atexit cleanup routine */
        _exit( rc );

    } /* end if (pid == 0) */

    /* The parent process continues as the pipe sender */

    /* Close the read end of the pipe */
    close_pipe( pipefd[0] );

    /* Save pipe write descriptor in the device block */
    dev->fd = pipefd[1];
    dev->ptpcpid = pid;

#endif /* defined( _MSVC_ ) */

    return 0;

} /* end function open_printer */

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
#endif /* defined( _MSVC_ ) */
        // "%1d:%04X Printer: pipe receiver with pid %d terminating"
        WRMSG (HHC01107, "I", LCSS_DEVNUM, dev->ptpcpid);
        dev->ptpcpid = 0;
    }
    else
    {
        if (dev->bs)
        {
            close_socket (fd);
            // "%1d:%04X Printer: client %s, IP %s disconnected from device %s"
            WRMSG (HHC01100, "I", LCSS_DEVNUM, dev->bs->clientname, dev->bs->clientip, dev->bs->spec);
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
    int  rc;
    U32  num, fcbsize, ucbsize, plbsize;

    UNREFERENCED( prevcode );

    fcbsize = FCBSIZE( dev );
    ucbsize = UCBSIZE( dev );
    plbsize = PLBSIZE( dev );

    /* Reset flags at start of CCW chain */
    if (!ccwseq)
        dev->diaggate = 0;

    /* Open the device file if necessary */
    if (dev->fd < 0 && !IS_CCW_SENSE( code ))
        rc = open_printer( dev );
    else
    {
        /* If printer stopped, return intervention required */
        if (dev->stopdev && !IS_CCW_SENSE( code ))
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
    switch (code)
    {
    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC READ PLB                                           */
    /*---------------------------------------------------------------*/
    case 0x02:

        /* Calculate residual byte count */
        num = (count < plbsize) ? count : plbsize;
        *residual = count - num;
        if (count < plbsize) *more = 1;

        /* Copy PLB to channel I/O buffer */
        memcpy( iobuf, dev->plb, num );

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    /*---------------------------------------------------------------*/
    /* NOP                                                           */
    /*---------------------------------------------------------------*/
    case 0x03:

        /* No Operation */
        *unitstat = CSW_CE | CSW_DE;
        break;

    /*---------------------------------------------------------------*/
    /* SENSE                                                         */
    /*---------------------------------------------------------------*/
    case 0x04:

        /* Calculate residual byte count */
        num = (count < dev->numsense) ? count : dev->numsense;
        *residual = count - num;
        if (count < dev->numsense) *more = 1;

        /* Copy device sense bytes to channel I/O buffer */
        memcpy( iobuf, dev->sense, num );

        /* Clear the device sense bytes */
        memset( dev->sense, 0, sizeof( dev->sense ));

        /* Return normal status */
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

        if (WriteLine( dev, code, flags, chained, count, iobuf, unitstat, residual ) == 0)
        {
            if (!(flags & CCW_FLAGS_CD))
                DoSpaceOrSkip( dev, code, unitstat );
            else
                *unitstat = CSW_CE | CSW_DE;
        }
        break;

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

        DoSpaceOrSkip( dev, code, unitstat );
        break;

    /*---------------------------------------------------------------*/
    /* 1403 DIAGNOSTIC WRITE                                         */
    /*---------------------------------------------------------------*/
               case 0x0D:
    case 0x15: case 0x1D:
               case 0x8D:
    case 0x95: case 0x9D:
    case 0xA5: case 0xAD:
    case 0xB5: case 0xBD:
    case 0xC5: case 0xCD:
    case 0xD5: case 0xDD:
    case 0xE5:

        if (dev->devtype != 0x1403)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* PURPOSELY FALL THROUGH to case 0x05: */
        /* FALLTHRU */

    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC WRITE                                              */
    /*---------------------------------------------------------------*/
    case 0x05:

        /* Diagnostic write is not supported on the 3203 */
        if (dev->devtype == 0x3203)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            /* Calculate number of bytes to write and set residual count */
            num = (count < plbsize) ? count : plbsize;
            *residual = count - num;

            /* Clear PLB of any previously printed data */
            memset( dev->plb, 0, sizeof( dev->plb ));

            /* Copy channel I/O buffer to PLB */
            memcpy( dev->plb, iobuf, num );

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC CHECK READ                                         */
    /*---------------------------------------------------------------*/
    case 0x06:

        /*      (Note: "PLB" = "Print Line Buffer")

        This command transfers unique check information stored
        in each addressable position of the PLB to the channel.
        It is normally used for diagnostic purposes, such as fault
        isolation to an individual print position.  Bit positions
        (zero) (0) through four (4) on bus-in are not used.  Bit
        position five (5) signifies that the print line complete
        (PLC) bit was set, indicating that a print compare was
        completed for the corresponding position or that the PLB
        contained a blank or null code.  Bit position six (6)
        signifies that the print error check (PEC) bit had been set
        due to invalid parity or a hammer check for the corres-
        ponding print position.  Bit seven (7) signifies that
        invalid parity was detected for the nine data bits in that
        PLB position.
        */
        if (!dev->diaggate)
        {
            /* Calculate residual byte count */
            num = (count < plbsize) ? count : plbsize;
            *residual = count - num;
            if (count < plbsize) *more = 1;

            /* Copy requested Check Read bytes to channel I/O buffer */
            memset( iobuf, 0x04, num );
        }
        else /* (dev->diaggate) */
        {
            /*
            When the 'check read' command is given after a 'diagnostic
            gate' command, the eight bits of the FCB pointer indicating
            the position of the current line in the forms control buffer
            are transferred to the channel.  The maximum data transfer
            length is 1.  The value of the FCB pointer is encoded in the
            same way as the 3811, as follows:

             * Internally, the FCB position is stored in 'zero origin'
               notation (the first line is line 0, the second line is
               line 1, etc...

             * A value of two is added to the zero origin notatIon.

             * The hexadecimal notation of the resultant one-byte value
               is reversed (bits 7, 6, 5, 4, 3, 2, 1, 0 become bits 0, 1,
               2, 3, 4, 5, 6, 7 respectively).

            For example, FCB pointer value hex '40' is encoded for the
            first line of the form; hex 'C0' is encoded for the second
            line, etc...
            */
            iobuf[0] = (BYTE)(dev->currline - 1);   // zero origin
            iobuf[0] += 2;                          // plus 2
            iobuf[0] = reverse_bits( iobuf[0] );    // reverse bits

            /* Calculate residual byte count */
            num = (count < 1) ? count : 1;
            *residual = count - num;
            if (count < 1) *more = 1;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC GATE                                               */
    /*---------------------------------------------------------------*/
    case 0x07:

        /* Command reject if 1403. Otherwise treated as NOP */
        if (dev->devtype == 0x1403)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Set diagnostic gate flag */
        dev->diaggate = 1;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC READ UCS BUFFER                                    */
    /*---------------------------------------------------------------*/
    case 0x0A:

        /* Reject if 1403 or not preceded by DIAGNOSTIC GATE */
        if (dev->devtype == 0x1403 || !dev->diaggate)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            /* Calculate residual byte count */
            num = (count < ucbsize) ? count : ucbsize;
            *residual = count - num;
            if (count < ucbsize) *more = 1;

            /* Copy requested UCB bytes to channel I/O buffer */
            memcpy( iobuf, dev->ucb, num );

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* DIAGNOSTIC READ FCB                                           */
    /*---------------------------------------------------------------*/
    case 0x12:

        /* Reject if 1403 or not preceded by DIAGNOSTIC GATE */
        if (dev->devtype == 0x1403 || !dev->diaggate)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            U32 i = 0;      // iobuf index
            U32 f;          // FCB index
            U32 k;          // work count
            BYTE b;         // work byte

            /* Calculate residual byte count */
            num = (count < fcbsize) ? count : fcbsize;
            *residual = count - num;
            if (count < fcbsize) *more = 1;

            /* Copy requested FCB bytes to channel I/O buffer */
            if (dev->index)
            {
                if (dev->index < 0)
                    b = 0xC0 | ((BYTE)-dev->index);
                else
                    b = 0x80 | ((BYTE)+dev->index);

                iobuf[i++] = b;
            }
            for (k=dev->lpp, f=1; f <= k && i < num; f++)
            {
                b = (BYTE) dev->fcb[f];
                if (0
                    || (f == 1 && dev->lpi == 8)
                    ||  f == k
                )
                    b |= 0x10;
                iobuf[i++] = b;
            }
            while (i < num)
                iobuf[i++] = 0;

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* UNFOLD                                                        */
    /*---------------------------------------------------------------*/
    case 0x23:

        if (dev->devtype == 0x1403)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            /* Reset fold indicator */
            dev->fold = 0;

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* FOLD                                                          */
    /*---------------------------------------------------------------*/
    case 0x43:

        if (dev->devtype == 0x1403)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            /* Set fold indicator */
            dev->fold = 1;

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* LOAD FORMS CONTROL BUFFER                                     */
    /*---------------------------------------------------------------*/
    case 0x63:

        if (dev->devtype == 0x1403)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            /*  The first byte of the FCB image is either the print
            **  position indexing byte, or the lines-per-inch byte.
            **
            **  The print position indexing byte is optional and, when
            **  used, precedes the lines-per-inch byte.
            **
            **  The 3203-5, 3262-5, 4245, 4248, and 6262-14 printers
            **  accept and discard the index byte if it is present,
            **  because they do not support the indexing feature.
            **
            **  A description of the print position indexing feature
            **  and its use can be found in the publication GA24-3543
            **  "IBM 3211 Printer, 3216 Interchangeable Train
            **  Cartridge, and 3811 Printer Control Unit Component
            **  Description and Operator's Guide".
            **
            **  The special index flag contains X'80' plus a binary
            **  index value, from 1 to 32 (the default is 1).  This
            **  index value sets the left margin: 1 indicates flush-
            **  left; any other value indicates a line indented
            **  the specified number of spaces. FISH NOTE: it appears
            **  a negative value sets a negative margin shifting the
            **  print line 'n' columns to the left thus chopping off
            **  the print line at that column (i.e. the first n chars
            **  of the print line are dropped and printing begins in
            **  column 1 with the N'th character of the print line).
            **
            **  The form image begins with the first line of the page,
            **  and if the X'1n' flag bit is set, indicates an 8 lines
            **  per inch form. The default when the X'1n' flag bit is
            **  not set indicates a normal 6 lines per inch form.
            **
            **  All remaining bytes (lines) must contain X'0n', except
            **  the last byte, which must be X'1n'.  The letter n can
            **  be a hexadecimal value from 1 to C, representing a
            **  channel (one to 12), or it can be 0, which means no
            **  channel is indicated for that line.
            */

            int  savedfcb[ MAX_FCBSIZE ];

            U32  i = 0;     // iobuf index
            U32  f;         // FCB index
            int  k;         // work counter
            BYTE b;         // work byte

            /* Load Check if X'20' bit on in first byte of FCB */
            if (iobuf[0] & 0x20)
            {
                /* Set Unit Check, SENSE = Load Check */
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                dev->sense[0] = SENSE_LDCK;
                *residual = count;
                break;
            }

            /* Save old fcb and clear to zero; new one being loaded */
            memcpy( savedfcb, dev->fcb, sizeof( savedfcb ));
            memset( dev->fcb,     0,    sizeof( dev->fcb ));

            /* First byte of FCB is optional indexing byte */
            dev->index = 0;
            b = iobuf[i];

            /* Was indexing option specified? */
            if (b & 0x80)
            {
                dev->index = (b & 0x1F);    /* Save indexing value   */
                i++;                        /* Consume optional byte */

                if (b & 0x40)               /* Negative indexing?    */
                    dev->index *= -1;       /* Then make it negative */

                dev->fcb[0] = dev->index;   /* Save FCB index value  */
            }

            /* Process FCB bytes for line=channel... */
            dev->lpi = 6;
            dev->lpp = 0;

            for (k=MAX_CHAN_STOPS, f=1; f < fcbsize && i < count; f++)
            {
                /* Consume the first or next FCB byte */
                b = iobuf[i++];

                /* Save channel number for this line */
                dev->fcb[f] = (b & (0xFF-0x10));

                /* Specifed channel greater than 12? */
                if (dev->fcb[f] > 12)
                    break;

                /* PROGRAMMING NOTE: a maximum of 30 channel codes
                   can be specified if the end of sheet is not
                   coded in the same byte as a channel code; a
                   maximum of 31 channel codes can be specified
                   if the end of sheet code is coded in the same
                   byte as a channel code. Thus the order of the
                   below two tests is important: we MUST check for
                   end of sheet FIRST (to skip decrementing our
                   channel code counter 'k') and then AFTERWARDS
                   decrement the counter if a channel was coded.
                */
                /* Flag bit set on first line of FCB means 8 LPI.
                   Otherwise flag bit set means last line of FCB.
                */
                if (b & 0x10)
                {
                    if (f == 1)
                        dev->lpi = 8;
                    else
                    {
                        dev->lpp = f;
                        break;
                    }
                }

                /* Count channel codes. Abort if max exceeded. */
                if (dev->fcb[f])
                    if (--k < 0)
                        break;
            }

            /* Any errors detected? */
            if (k < 0 || !dev->lpp)
            {
                /* Set Unit Check, SENSE = Load Check */
                *unitstat = CSW_CE | CSW_DE | CSW_UC;
                dev->sense[0] = SENSE_LDCK;

                /* Calculate residual */
                *residual = count - i;

                /* Restore original FCB */
                memcpy( dev->fcb, savedfcb, sizeof( dev->fcb ));
            }
            else // success
            {
                /* Return normal status */
                *unitstat = CSW_CE | CSW_DE;

                /* Calculate residual byte count */
                if (dev->devtype == 0x3211)
                {
                    /* Note: The 3211 printer does not set incorrect length
                       on 'load FCB'. This is *not* mentioned anywhere in
                       the 3211 manual (GA24-3543), but *is* mentioned in
                       Appendix C of the 3203-5 manual (GA33-1529, updated
                       by TNL GN33-1732) where it describes the differences
                       between the 3203-5 and other printers.
                    */
                    *residual = 0;    /* prevent incorrect length */
                }
                else
                    *residual = count - i;

                /* A new fcb has just been loaded */
                on_new_fcb( dev );
            }
        }
        break;

    /*---------------------------------------------------------------*/
    /* RAISE COVER                                                   */
    /*---------------------------------------------------------------*/
    case 0x6B:

        if (dev->devtype == 0x1403)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else /* 3203, 3211 */
        {
            /* 3203 == accepted but no action occurs */
            /* 3211 == accepted and raises the cover */

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* BLOCK DATA CHECK                                              */
    /*---------------------------------------------------------------*/
    case 0x73:

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    /*---------------------------------------------------------------*/
    /* ALLOW DATA CHECK                                              */
    /*---------------------------------------------------------------*/
    case 0x7B:

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    /*---------------------------------------------------------------*/
    /* SKIP TO CHANNEL 0 IMMEDIATE                                   */
    /*---------------------------------------------------------------*/
    case 0x83:

        if (dev->devtype != 0x3211)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* SENSE ID                                                      */
    /*---------------------------------------------------------------*/
    case 0xE4:

        /* SENSE ID is only supported if LEGACYSENSEID is ON */
        if (sysblk.legacysenseid)
        {
            /* Calculate residual byte count */
            num = (count < dev->numdevid) ? count : dev->numdevid;
            *residual = count - num;
            if (count < dev->numdevid) *more = 1;

            /* Copy device identifier bytes to channel I/O buffer */
            memcpy( iobuf, dev->devid, num );

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        else
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        break;

    /*---------------------------------------------------------------*/
    /* UCS GATE LOAD                                                 */
    /*---------------------------------------------------------------*/
    case 0xEB:

        /* Command reject if not 1403 */
        if (dev->devtype != 0x1403)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            /* Set diagnostic gate flag */
            dev->diaggate = 1;

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        break;

    /*---------------------------------------------------------------*/
    /* LOAD UCS BUFFER AND FOLD                                      */
    /*---------------------------------------------------------------*/
    case 0xF3:

        /* Reject if not 1403 or not preceded by UCS GATE LOAD */
        if (dev->devtype != 0x1403 || !dev->diaggate)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            if (LoadUCB( dev, count, iobuf, more, unitstat, residual ) == 0)
            {
                /* Set fold indicator */
                dev->fold = 1;
                dev->ffpend = 1;

                /* Return normal status */
                *unitstat = CSW_CE | CSW_DE;
            }
        }
        break;

    /*---------------------------------------------------------------*/
    /* LOAD UCS BUFFER  (NO FOLD implied)                            */
    /*---------------------------------------------------------------*/
    case 0xFB:

        /* For 1403, command reject if not chained to UCS GATE LOAD */
        if (dev->devtype == 0x1403 && !dev->diaggate)
        {
            /* Command Reject */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {
            if (LoadUCB( dev, count, iobuf, more, unitstat, residual ) == 0)
            {
                /* Reset fold indicator */
                dev->fold = 0;
                dev->ffpend = 1;

                /* Return normal status */
                *unitstat = CSW_CE | CSW_DE;
            }
        }
        break;

    /*---------------------------------------------------------------*/
    /* INVALID OPERATION                                             */
    /*---------------------------------------------------------------*/
    default:

        /* Set command reject sense byte, and unit check status */
        dev->sense[0] = SENSE_CR;
        *unitstat = CSW_UC;

    } /* end switch(code) */

} /* end function printer_execute_ccw */


static DEVHND printer_device_hndinfo =
{
        &printer_init_handler,         /* Device Initialization      */
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
DEVHND prt3203_device_hndinfo = {
        &printer_init_handler,         /* Device Initialization      */
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
        prt3203_immed_commands,        /* Immediate CCW Codes        */
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
#define hdl_ddev hdt1403_LTX_hdl_ddev
#define hdl_depc hdt1403_LTX_hdl_depc
#define hdl_reso hdt1403_LTX_hdl_reso
#define hdl_init hdt1403_LTX_hdl_init
#define hdl_fini hdt1403_LTX_hdl_fini
#endif

HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
     HDL_DEPENDENCY(DEVBLK);
}
END_DEPENDENCY_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE( 1403, printer_device_hndinfo );
    HDL_DEVICE( 3203, prt3203_device_hndinfo );
    HDL_DEVICE( 3211, printer_device_hndinfo );
}
END_DEVICE_SECTION
