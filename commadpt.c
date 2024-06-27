/* COMMADPT.C   (C) Copyright Roger Bowler & Others, 2002-2012       */
/*              (C) Copyright, MHP, 2007-2008 (see below)            */
/*              Hercules Communication Line Driver                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Hercules Communication Line Driver                                */
/* (c) 1999-2010 Roger Bowler & Others                               */
/* Use of this program is governed by the QPL License                */
/* Original Author : Ivan Warren                                     */
/* Prime Maintainer : Ivan Warren                                    */
/*-------------------------------------------------------------------*/


/* ********************************************************************

   TTY Mode Additions (C) Copyright, 2007 MHP <ikj1234i at yahoo dot com>

   Feb. 2007- Add support for 2703 Telegraph Terminal Control Type II
   (for use with TTY ASR 33/35 or compatible terminals).

   To enable TTY mode for a particular port, specify ``LNCTL=ASYNC'' in the
   hercules conf file.

   The host sends and receives bytes directly in ASCII, but bits are
   reversed from standard ASCII bit order.  Lookup tables are used to
   perform the reversals on each byte, to make even parity for sending
   toward the host, and to skip certain noxious characters emitted by
   TSO/TCAM.

   This code contains minimal TELNET support.  It accepts TELNET
   IP (Interrupt Process) which is mapped and transmitted to the host
   as a BREAK (ATTN) sequence.  All TELNET commands are responded
   negatively.

   *****************************************************************

   2741 Mode Additions (C) Copyright, 2008 MHP <ikj1234i at yahoo dot com>

   2741 mode is now working, though imperfectly as of yet.

   There is a new param (term=tty|2741) in the conf file to select termtype.

   Specify code=ebcd for EBCD, or code=corr for correspondence code.
   Also code=none to disable all translation.  The code= option applies to
   2741 mode only.

   Another new param (skip=) is a byte string, specified in hex, to specify
   "garbage" code points that are to be suppressed in output processing.
   This allows distinct lists to be used for different terminal types.

   Automatic translation to Uppercase is enabled by setting uctrans=yes .
   This is for both 2741 and TTY terminals.

   You can specify lnctl=tele2 for TTY and lnctl=ibm1 for 2741.

   Samples:
     0045 2703 lport=32003 dial=IN lnctl=ibm1 term=2741 skip=5EDE code=ebcd
     0045 2703 lport=32003 dial=IN lnctl=tele2 uctrans=yes term=tty skip=88C9DF

   For TCAM, if you are zapping device UCB's, the following type values
   seem to work:
   55 10 40 13 - 2741
   51 10 40 53 - TTY (3335)

   When running TCAM in 2741 mode if you experience A00 abends, the following
   zap (to member IEDQTCAM in 'SYS1.LINKLIB') may help

   NAME IEDQTCAM IEDQKA01
   VER 0E38 012C
   REP 0E38 0101

   The 2741 mode enables ordinary remote ASCII TELNET clients to connect.
   The translate tables were lifted from IEDQ27 and IEDQ28.  Instead of
   translating directly between ASCII and the host's 2741-correspondence code,
   we add an intermediate EBCDIC step.  This enables use of all pre-
   existing tables so I don't have to code any new ones :)

   Also, 2740 OS consoles should work.  It may be necessary to modify the
   driver to end READ CCWs even when no data has been received (see comment)

   *****************************************************************

   Some TTY Fixes - 2012-01-30 MHP <ikj1234i at yahoo dot com>

   This async/2703 driver release contains several bug fixes and minor
   enhancements (primarily to fix windows telnet clients).  In addition
   there are three new configuration parameters (iskip=, bs=dumb, break=dumb)

   When using windows telnet, it's recommended to set bs=dumb and break=dumb .

   The new iskip= option is analogous to the skip= option, except that
   it chooses input ASCII characters to suppress (the skip= option is used
   to suppress characters in output processing).  Note that while both options
   are entered as hex bytes, the skip= option uses S/370 mainframe code
   points (either byte-reversed ASCII for TTY or correspondence code/EBCD for
   2741), whereas the iskip= option requires ASCII code points.  Here's an
   example:
   0045 2703 lport=32003 dial=IN lnctl=tele2 uctrans=yes term=tty skip=88C9DF iskip=0A
   Here's an example, for windows telnet clients
   0046 2703 lport=32003 dial=IN lnctl=tele2 uctrans=yes term=tty skip=88C9DF iskip=0A bs=dumb break=dumb

   *****************************************************************

   driver mods for APL\360 December 2012 MHP <ikj1234i at yahoo dot com>
   - circumvention for several race conditions
   - new "eol" parameter specifies a byte value (ASCII), default 0x0D,
     which when received marks the end of the input line
   - new "prepend" and "append" parameters to specify zero to four
     bytes to be prepended and appended (respectively) to input lines
     that have been received from terminals before being sent to the
     mainframe OS.  Typical use is to add Circle D and C around each
     input transmission (2741's for APL\360).  Bytes must be specified in
     S/370 channel format, not in ASCII.
   - new terminal type "rxvt4apl" with 8-bit and character translation
     support for rxvt4apl in 2741 mode. Use the following conf definition entry
     0402 2703 dial=in lport=57413 lnctl=ibm1 term=rxvt4apl skip=5EDE code=ebcd
        iskip=0D0A prepend=16 append=5B1F eol=0A binary=yes crlf=yes sendcr=yes
          [all on a single line]
   - negotiation to telnet binary mode when using rxvt4apl ("binary" parameter)
   - send CR back to terminal when input line received ("sendcr" parameter)
   - option to map 2741 NL to TTY CRLF sequence ("crlf" parameter)
   - increase Hercules MAX_ARGS

******************************************************************** */

#include "hstdinc.h"
#include "hercules.h"
#include "devtype.h"
#include "parser.h"
#include "commadpt.h"

#if defined(WIN32) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)
  SYSBLK *psysblk;
  #define sysblk (*psysblk)
#endif

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

static BYTE commadpt_immed_command[256]=
{ 0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

COMMADPT_PEND_TEXT;     /* Defined in commadpt.h                     */
                        /* Defines commadpt_pendccw_text array       */

/*---------------------------------------------------------------*/
/* PARSER TABLES                                                 */
/*---------------------------------------------------------------*/
static PARSER ptab[]={
    { "lport",    PARSER_STR_TYPE },
    { "lhost",    PARSER_STR_TYPE },
    { "rport",    PARSER_STR_TYPE },
    { "rhost",    PARSER_STR_TYPE },
    { "dial",     PARSER_STR_TYPE },
    { "rto",      PARSER_STR_TYPE },
    { "pto",      PARSER_STR_TYPE },
    { "eto",      PARSER_STR_TYPE },
    { "switched", PARSER_STR_TYPE },
    { "lnctl",    PARSER_STR_TYPE },
    { "term",     PARSER_STR_TYPE },
    { "code",     PARSER_STR_TYPE },
    { "uctrans",  PARSER_STR_TYPE },
    { "skip",     PARSER_STR_TYPE },
    { "iskip",    PARSER_STR_TYPE },
    { "bs",       PARSER_STR_TYPE },
    { "break",    PARSER_STR_TYPE },
    { "prepend",  PARSER_STR_TYPE },
    { "append",   PARSER_STR_TYPE },
    { "eol",      PARSER_STR_TYPE },
    { "crlf",     PARSER_STR_TYPE },
    { "sendcr",   PARSER_STR_TYPE },
    { "binary",   PARSER_STR_TYPE },
    { "ka",       PARSER_STR_TYPE },
    { "crlf2cr",  PARSER_STR_TYPE },
    {NULL,NULL}  /* (end of table) */
};

/* The following must be in the same sequence as the above 'ptab' table */
enum {
    COMMADPT_KW_LPORT=1,
    COMMADPT_KW_LHOST,
    COMMADPT_KW_RPORT,
    COMMADPT_KW_RHOST,
    COMMADPT_KW_DIAL,
    COMMADPT_KW_READTO,
    COMMADPT_KW_POLLTO,
    COMMADPT_KW_ENABLETO,
    COMMADPT_KW_SWITCHED,
    COMMADPT_KW_LNCTL,
    COMMADPT_KW_TERM,
    COMMADPT_KW_CODE,
    COMMADPT_KW_UCTRANS,
    COMMADPT_KW_SKIP,
    COMMADPT_KW_ISKIP,
    COMMADPT_KW_BS,
    COMMADPT_KW_BREAK,
    COMMADPT_KW_PREPEND,
    COMMADPT_KW_APPEND,
    COMMADPT_KW_EOL,
    COMMADPT_KW_CRLF,
    COMMADPT_KW_SENDCR,
    COMMADPT_KW_BINARY,
    COMMADPT_KW_KA,
    COMMADPT_KW_CRLF2CR,
} commadpt_kw;

static BYTE telnet_binary[6] = { 0xff, 0xfd, 0x00, 0xff, 0xfb, 0x00 };

BYTE overstrike_2741_pairs[] = {
    0x93, 0xA6, /* nor */
    0xC9, 0xCC, /* rotate */
    0xCC, 0xCF, /* log */
    0xC9, 0xEE, /* grade down */
    0xC3, 0xE7, /* lamp */
    0xC5, 0xC6, /* quote quad */
    0x93, 0xA6, /* nand */
    0xA3, 0xCC, /* transpose */
    0xA6, 0xEE, /* locked fn */
    0xC9, 0xF0, /* grade up */
    0x76, 0xC5, /* exclamation */
    0xCA, 0xE4, /* ibeam */
    0xC6, 0xE1, /* domino */
};

BYTE overstrike_rxvt4apl_chars[] = {
    /* must match overstrike_2741_pairs */
    0xe5,
    0xe8,
    0x89,
    0x9d,
    0xa6,
    0x97,
    0xea,
    0xed,
    0xa1, /* locked fn - no exact match for this */
    0x93,
    0x21,
    0x84,
    0x98,
};

static BYTE overstrike_map [256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0,
    0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static BYTE rxvt4apl_from_2741[256] = {
    0x3f, 0x20, 0x31, 0x3f, 0x32, 0x3f, 0x3f, 0x33, 0x34, 0x3f, 0x3f, 0x35, 0x3f, 0x36, 0x37, 0x3f,
    0x38, 0x3f, 0x3f, 0x39, 0x3f, 0x30, 0x5d, 0x3f, 0x3f, 0x3f, 0x95, 0x3f, 0x96, 0x3f, 0x3f, 0x04,
    0x90, 0x3f, 0x3f, 0x2f, 0x3f, 0x53, 0x54, 0x3f, 0x3f, 0x55, 0x56, 0x3f, 0x57, 0x3f, 0x3f, 0x58,
    0x3f, 0x59, 0x5a, 0x3f, 0x3f, 0x3f, 0x3f, 0x2c, 0x84, 0x3f, 0x3f, 0x0a, 0x3f, 0x17, 0x1b, 0x3f,
    0x2b, 0x3f, 0x3f, 0x4a, 0x3f, 0x4b, 0x4c, 0x3f, 0x3f, 0x4d, 0x4e, 0x3f, 0x4f, 0x3f, 0x3f, 0x50,
    0x3f, 0x51, 0x52, 0x3f, 0x3f, 0x3f, 0x3f, 0x5b, 0x9d, 0x3f, 0x3f, 0x0d, 0x3f, 0x08, 0x87, 0x3f,
    0x3f, 0x92, 0x41, 0x3f, 0x42, 0x3f, 0x3f, 0x43, 0x44, 0x3f, 0x3f, 0x45, 0x3f, 0x46, 0x47, 0x3f,
    0x48, 0x3f, 0x3f, 0x49, 0x3f, 0x3f, 0x2e, 0x3f, 0x3f, 0x3f, 0x09, 0x3f, 0x86, 0x3f, 0x3f, 0x7f,
    0x3f, 0x20, 0x9a, 0x3f, 0xfd, 0x3f, 0x3f, 0x3c, 0xf3, 0x3f, 0x3f, 0x3d, 0x3f, 0xf2, 0x3e, 0x3f,
    0x86, 0x3f, 0x3f, 0xfa, 0x3f, 0x5e, 0x29, 0x3f, 0x3f, 0x3f, 0x95, 0x3f, 0x96, 0x3f, 0x3f, 0x3f,
    0x85, 0x3f, 0x3f, 0x5c, 0x3f, 0x8d, 0x7e, 0x3f, 0x3f, 0x8b, 0xfc, 0x3f, 0xf7, 0x3f, 0x3f, 0x83,
    0x3f, 0x8c, 0x82, 0x3f, 0x3f, 0x3f, 0x3f, 0x3b, 0x84, 0x3f, 0x3f, 0x0a, 0x3f, 0x17, 0x1b, 0x3f,
    0x2d, 0x3f, 0x3f, 0xf8, 0x3f, 0x27, 0x95, 0x3f, 0x3f, 0x7c, 0xe7, 0x3f, 0xf9, 0x3f, 0x3f, 0x2a,
    0x3f, 0x3f, 0xfb, 0x3f, 0x3f, 0x3f, 0x3f, 0x28, 0x9d, 0x3f, 0x3f, 0x0d, 0x3f, 0x08, 0x87, 0x3f,
    0x3f, 0xf6, 0xe0, 0x3f, 0xe6, 0x3f, 0x3f, 0xef, 0x8f, 0x3f, 0x3f, 0xee, 0x3f, 0x5f, 0xec, 0x3f,
    0x91, 0x3f, 0x3f, 0xe2, 0x3f, 0x3f, 0x3a, 0x3f, 0x3f, 0x3f, 0x09, 0x3f, 0x86, 0x3f, 0x3f, 0x7f,
};

static BYTE rxvt4apl_to_2741[256] = {
    0x88, 0x88, 0x88, 0x88, 0x1f, 0x88, 0x88, 0x88, 0x5d, 0x7a, 0x3b, 0x88, 0x88, 0x5b, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x5e, 0x88, 0x88, 0x88, 0x88, 0x3e, 0x88, 0x88, 0x88, 0x88,
    0x01, 0xb7, 0x96, 0x16, 0x57, 0x8b, 0x61, 0xc5, 0xd7, 0x96, 0xcf, 0x40, 0x37, 0xc0, 0x76, 0x23,
    0x15, 0x02, 0x04, 0x07, 0x08, 0x0b, 0x0d, 0x0e, 0x10, 0x13, 0xf6, 0xb7, 0x87, 0x8b, 0x8e, 0xd1,
    0x20, 0xe2, 0xe4, 0xe7, 0xe8, 0xeb, 0xed, 0xee, 0xf0, 0xf3, 0xc3, 0xc5, 0xc6, 0xc9, 0xca, 0xcc,
    0xcf, 0xd1, 0xd2, 0xa5, 0xa6, 0xa9, 0xaa, 0xac, 0xaf, 0xb1, 0xb2, 0x57, 0xa3, 0x16, 0x95, 0xed,
    0x88, 0x62, 0x64, 0x67, 0x68, 0x6b, 0x6d, 0x6e, 0x70, 0x73, 0x43, 0x45, 0x46, 0x49, 0x4a, 0x4c,
    0x4f, 0x51, 0x52, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x2f, 0x31, 0x32, 0x88, 0xc9, 0x88, 0xa6, 0x88,
    0x88, 0x88, 0xb2, 0xaf, 0x88, 0xa0, 0x90, 0x88, 0x88, 0x88, 0x88, 0xa9, 0xb1, 0xa5, 0x88, 0xe8,
    0x20, 0xf0, 0x61, 0x88, 0x88, 0xc6, 0x88, 0x88, 0x88, 0x88, 0x82, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0xe2, 0x88, 0xf3, 0x88, 0x88, 0x88, 0xe4, 0xca, 0x88, 0x88, 0x88, 0x88, 0xee, 0x88, 0xeb, 0xe7,
    0x88, 0x88, 0x8d, 0x88, 0x88, 0x88, 0xe1, 0xac, 0xc3, 0xcc, 0x93, 0xd2, 0xaa, 0x84, 0x88, 0x88,
};

/* 2741 EBCD code tables */
/* directly copied from mvs src file iedq27 */
static BYTE xlate_table_ebcd_toebcdic[256] = {
    0x3F, 0x40, 0xF1, 0x3F, 0xF2, 0x3F, 0x3F, 0xF3, 0xF4, 0x3F, 0x3F, 0xF5, 0x3F, 0xF6, 0xF7, 0x3F,
    0xF8, 0x3F, 0x3F, 0xF9, 0x3F, 0xF0, 0x7B, 0x3F, 0x3F, 0x3F, 0x35, 0x3F, 0x36, 0x3F, 0x3F, 0x37,
    0x7C, 0x3F, 0x3F, 0x61, 0x3F, 0xA2, 0xA3, 0x3F, 0x3F, 0xA4, 0xA5, 0x3F, 0xA6, 0x3F, 0x3F, 0xA7,
    0x3F, 0xA8, 0xA9, 0x3F, 0x3F, 0x3F, 0x3F, 0x6B, 0x24, 0x3F, 0x3F, 0x25, 0x3F, 0x26, 0x27, 0x3F,
    0x60, 0x3F, 0x3F, 0x91, 0x3F, 0x92, 0x93, 0x3F, 0x3F, 0x94, 0x95, 0x3F, 0x96, 0x3F, 0x3F, 0x97,
    0x3F, 0x98, 0x99, 0x3F, 0x3F, 0x3F, 0x3F, 0x5B, 0x14, 0x3F, 0x3F, 0x15, 0x3F, 0x16, 0x17, 0x3F,
    0x3F, 0x50, 0x81, 0x3F, 0x82, 0x3F, 0x3F, 0x83, 0x84, 0x3F, 0x3F, 0x85, 0x3F, 0x86, 0x87, 0x3F,
    0x88, 0x3F, 0x3F, 0x89, 0x3F, 0x3F, 0x4B, 0x3F, 0x3F, 0x3F, 0x05, 0x3F, 0x06, 0x3F, 0x3F, 0x07,
    0x3F, 0x40, 0x7E, 0x3F, 0x4C, 0x3F, 0x3F, 0x5E, 0x7A, 0x3F, 0x3F, 0x6C, 0x3F, 0x7D, 0x6E, 0x3F,
    0x5C, 0x3F, 0x3F, 0x4D, 0x3F, 0x5D, 0x7F, 0x3F, 0x3F, 0x3F, 0x35, 0x3F, 0x36, 0x3F, 0x3F, 0x3F,
    0x4A, 0x3F, 0x3F, 0x6F, 0x3F, 0xE2, 0xE3, 0x3F, 0x3F, 0xE4, 0xE5, 0x3F, 0xE6, 0x3F, 0x3F, 0xE7,
    0x3F, 0xE8, 0xE9, 0x3F, 0x3F, 0x3F, 0x3F, 0x4F, 0x24, 0x3F, 0x3F, 0x25, 0x3F, 0x26, 0x27, 0x3F,
    0x6D, 0x3F, 0x3F, 0xD1, 0x3F, 0xD2, 0xD3, 0x3F, 0x3F, 0xD4, 0xD5, 0x3F, 0xD6, 0x3F, 0x3F, 0xD7,
    0x3F, 0xD8, 0xD9, 0x3F, 0x3F, 0x3F, 0x3F, 0x5A, 0x14, 0x3F, 0x3F, 0x15, 0x3F, 0x16, 0x17, 0x3F,
    0x3F, 0x4E, 0xC1, 0x3F, 0xC2, 0x3F, 0x3F, 0xC3, 0xC4, 0x3F, 0x3F, 0xC5, 0x3F, 0xC6, 0xC7, 0x3F,
    0xC8, 0x3F, 0x3F, 0xC9, 0x3F, 0x3F, 0x5F, 0x3F, 0x3F, 0x3F, 0x05, 0x3F, 0x06, 0x3F, 0x3F, 0x07
};

static BYTE xlate_table_ebcd_fromebcdic[256] = {
    0x88, 0x88, 0x88, 0x88, 0x88, 0x7A, 0x7C, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x5B, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x58, 0x5B, 0x5D, 0x5E, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x38, 0x3B, 0x88, 0x3E, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x5E, 0x88, 0x88, 0x88, 0x1C, 0x1F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x01, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xA0, 0x76, 0x84, 0x93, 0xE1, 0xB7,
    0x61, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xD7, 0x57, 0x90, 0x95, 0x87, 0xF6,
    0x40, 0x23, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x37, 0x8B, 0xC0, 0x8E, 0xA3,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x16, 0x20, 0x8D, 0x82, 0x96,
    0x88, 0x62, 0x64, 0x67, 0x68, 0x6B, 0x6D, 0x6E, 0x70, 0x73, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x43, 0x45, 0x46, 0x49, 0x4A, 0x4C, 0x4F, 0x51, 0x52, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x25, 0x26, 0x29, 0x2A, 0x2C, 0x2F, 0x31, 0x32, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0xE2, 0xE4, 0xE7, 0xE8, 0xEB, 0xED, 0xEE, 0xF0, 0xF3, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0xC3, 0xC5, 0xC6, 0xC9, 0xCA, 0xCC, 0xCF, 0xD1, 0xD2, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0xA5, 0xA6, 0xA9, 0xAA, 0xAC, 0xAF, 0xB1, 0xB2, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x15, 0x02, 0x04, 0x07, 0x08, 0x0B, 0x0D, 0x0E, 0x10, 0x13, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88
};

/* 2741 correspondence code tables */
/* directly copied from mvs src file iedq28 */
static BYTE xlate_table_cc_toebcdic[256] = {
    0x3F, 0x40, 0xF1, 0x3F, 0xF2, 0x3F, 0x3F, 0xF3, 0xF5, 0x3F, 0x3F, 0xF7, 0x3F, 0xF6, 0xF8, 0x3F,
    0xF4, 0x3F, 0x3F, 0xF0, 0x3F, 0xA9, 0xF9, 0x3F, 0x3F, 0x34, 0x35, 0x3F, 0x36, 0x3F, 0x3F, 0x37,
    0xA3, 0x3F, 0x3F, 0xA7, 0x3F, 0x95, 0xA4, 0x3F, 0x3F, 0x85, 0x84, 0x3F, 0x92, 0x3F, 0x3F, 0x83,
    0x3F, 0x93, 0x88, 0x3F, 0x3F, 0x3F, 0x3F, 0x82, 0x24, 0x3F, 0x3F, 0x25, 0x3F, 0x26, 0x27, 0x3F,
    0x5A, 0x3F, 0x3F, 0x94, 0x3F, 0x4B, 0xA5, 0x3F, 0x3F, 0x7D, 0x99, 0x3F, 0x89, 0x3F, 0x3F, 0x81,
    0x3F, 0x96, 0xA2, 0x3F, 0x3F, 0x3F, 0x3F, 0xA6, 0x14, 0x3F, 0x3F, 0x15, 0x3F, 0x16, 0x17, 0x3F,
    0x3F, 0x91, 0x87, 0x3F, 0x7E, 0x3F, 0x3F, 0x86, 0x97, 0x3F, 0x3F, 0x5E, 0x3F, 0x98, 0x6B, 0x3F,
    0x61, 0x3F, 0x3F, 0xA8, 0x3F, 0x3F, 0x60, 0x3F, 0x3F, 0x04, 0x05, 0x3F, 0x06, 0x3F, 0x3F, 0x07,
    0x3F, 0x40, 0x4F, 0x3F, 0x7C, 0x3F, 0x3F, 0x7B, 0x6C, 0x3F, 0x3F, 0x50, 0x3F, 0x4C, 0x5C, 0x3F,
    0x5B, 0x3F, 0x3F, 0x5D, 0x3F, 0xE9, 0x4D, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x36, 0x3F, 0x3F, 0x37,
    0xE3, 0x3F, 0x3F, 0xE7, 0x3F, 0xD5, 0xE4, 0x3F, 0x3F, 0xC5, 0xC4, 0x3F, 0xD2, 0x3F, 0x3F, 0xC3,
    0x3F, 0xD3, 0xC8, 0x3F, 0x3F, 0x3F, 0x3F, 0xC2, 0x24, 0x3F, 0x3F, 0x25, 0x3F, 0x3F, 0x27, 0x3F,
    0x6E, 0x3F, 0x3F, 0xD4, 0x3F, 0x4B, 0xE5, 0x3F, 0x3F, 0x7F, 0xD9, 0x3F, 0xC9, 0x3F, 0x3F, 0xC1,
    0x3F, 0xD6, 0xE2, 0x3F, 0x3F, 0x3F, 0x3F, 0xE6, 0x14, 0x3F, 0x3F, 0x15, 0x3F, 0x16, 0x3F, 0x3F,
    0x3F, 0xD1, 0xC7, 0x3F, 0x4E, 0x3F, 0x3F, 0xC6, 0xD7, 0x3F, 0x3F, 0x7A, 0x3F, 0xD8, 0x6B, 0x3F,
    0x6F, 0x3F, 0x3F, 0xE8, 0x3F, 0x3F, 0x6D, 0x3F, 0x3F, 0x3F, 0x05, 0x3F, 0x06, 0x3F, 0x3F, 0x3F,
};

static BYTE xlate_table_cc_fromebcdic[256] = {
    0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0x7A, 0x7C, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0x5B, 0xEB, 0xEB,
    0xEB, 0xEB, 0xEB, 0xEB, 0x58, 0x5B, 0x5D, 0x5E, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0xEB, 0xEB, 0xEB, 0x38, 0x3B, 0xEB, 0x3E, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0xEB, 0x5E, 0xEB, 0x19, 0x1A, 0x1C, 0x1F, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0x01, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0x45, 0x8D, 0x96, 0xE4, 0x82,
    0x8B, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0x40, 0x90, 0x8E, 0x93, 0x6B, 0xEB,
    0x76, 0x70, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0x6E, 0x88, 0xF6, 0xC0, 0xF0,
    0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0x87, 0x84, 0x49, 0x64, 0xC9,
    0xEB, 0x4F, 0x37, 0x2F, 0x2A, 0x29, 0x67, 0x62, 0x32, 0x4C, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0x61, 0x2C, 0x31, 0x43, 0x25, 0x51, 0x68, 0x6D, 0x4A, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0xEB, 0x52, 0x20, 0x26, 0x46, 0x57, 0x23, 0x73, 0x15, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0xCF, 0xB7, 0xAF, 0xAA, 0xA9, 0xE7, 0xE2, 0xB2, 0xCC, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0xE1, 0xAC, 0xB1, 0xC3, 0xA5, 0xD1, 0xE8, 0xED, 0xCA, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0xEB, 0xEB, 0xD2, 0xA0, 0xA6, 0xC6, 0xD7, 0xA3, 0xF3, 0x95, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
    0x13, 0x02, 0x04, 0x07, 0x10, 0x08, 0x0D, 0x0B, 0x0E, 0x16, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB, 0xEB,
};

#define CIRCLE_C 0x1F
#define CIRCLE_D 0x16

static BYTE byte_parity_table [128] = {
/* value: 0 = even parity, 1 = odd parity */
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

static void logdump(char *txt,DEVBLK *dev,BYTE *bfr,size_t sz)
{
    char buf[128];
    char byte[5];
    size_t i;
    if ( !dev->ccwtrace )
    {
        return;
    }
    WRMSG(HHC01064,"D",
            SSID_TO_LCSS(dev->ssid),
            dev->devnum,
            txt,
            dev->commadpt->in_textmode?"YES":"NO",
            dev->commadpt->in_xparmode?"YES":"NO",
            dev->commadpt->xparwwait?"YES":"NO");
    WRMSG(HHC01049, "D",LCSS_DEVNUM,txt,(u_int)sz,(u_int)sz);
    buf[0] = 0;
    for( i=0; i<sz; i++ )
    {
        if( i%16 == 0 )
        {
            if( i !=0 )
            {
                WRMSG(HHC01050,"D",LCSS_DEVNUM,txt,buf);
                buf[0] = 0;
            }
            MSGBUF(buf, "%04X:", (unsigned) i);
        }
        if( i%4 == 0 )
        {
            STRLCAT( buf, " " );
        }
        MSGBUF(byte, "%02X",bfr[i]);
        STRLCAT( buf, byte );
    }
    WRMSG(HHC01050,"D",LCSS_DEVNUM,txt,buf);
}

/*-------------------------------------------------------------------*/
/* Handler utility routines                                          */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Buffer ring management                                            */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Buffer ring management : Init a buffer ring                       */
/*-------------------------------------------------------------------*/
void commadpt_ring_init(COMMADPT_RING *ring,size_t sz,int trace)
{
    ring->bfr=malloc(sz);
    ring->sz=sz;
    ring->hi=0;
    ring->lo=0;
    ring->havedata=0;
    ring->overflow=0;
    if(trace)
    {
        WRMSG(HHC01065,"D",
            ring,
            ring->bfr,
            "allocated");
    }
}

/*-------------------------------------------------------------------*/
/* Buffer ring management : Free a buffer ring                       */
/*-------------------------------------------------------------------*/
static void commadpt_ring_terminate(COMMADPT_RING *ring,int trace)
{
    if(trace)
    {
        WRMSG(HHC01065,"D",
            ring,
            ring->bfr,
              "freed");
    }
    if(ring->bfr!=NULL)
    {
        free(ring->bfr);
        ring->bfr=NULL;
    }
    ring->sz=0;
    ring->hi=0;
    ring->lo=0;
    ring->havedata=0;
    ring->overflow=0;
}

/*-------------------------------------------------------------------*/
/* Buffer ring management : Flush a buffer ring                      */
/*-------------------------------------------------------------------*/
static void commadpt_ring_flush(COMMADPT_RING *ring)
{
    ring->hi=0;
    ring->lo=0;
    ring->havedata=0;
    ring->overflow=0;
}

/*-------------------------------------------------------------------*/
/* Buffer ring management : Queue a byte in the ring                 */
/*-------------------------------------------------------------------*/
static inline void commadpt_ring_push(COMMADPT_RING *ring,BYTE b)
{
    ring->bfr[ring->hi++]=b;
    if(ring->hi>=ring->sz)
    {
        ring->hi=0;
    }
    if(ring->hi==ring->lo)
    {
        ring->overflow=1;
    }
    ring->havedata=1;
}

/*-------------------------------------------------------------------*/
/* Buffer ring management : Queue a byte array in the ring           */
/*-------------------------------------------------------------------*/
static inline void commadpt_ring_pushbfr(COMMADPT_RING *ring,BYTE *b,size_t sz)
{
    size_t i;
    for(i=0;i<sz;i++)
    {
        commadpt_ring_push(ring,b[i]);
    }
}

/*-------------------------------------------------------------------*/
/* Buffer ring management : Retrieve a byte from the ring            */
/*-------------------------------------------------------------------*/
static inline BYTE commadpt_ring_pop(COMMADPT_RING *ring)
{
    register BYTE b;
    b=ring->bfr[ring->lo++];
    if(ring->lo>=ring->sz)
    {
        ring->lo=0;
    }
    if(ring->hi==ring->lo)
    {
        ring->havedata=0;
    }
    return b;
}

/*-------------------------------------------------------------------*/
/* Buffer ring management : Retrive a byte array from the ring       */
/*-------------------------------------------------------------------*/
static inline size_t commadpt_ring_popbfr(COMMADPT_RING *ring,BYTE *b,size_t sz)
{
    size_t i;
    for(i=0;i<sz && ring->havedata;i++)
    {
        b[i]=commadpt_ring_pop(ring);
    }
    return i;
}

/*-------------------------------------------------------------------*/
/* Free all private structures and buffers  (and release CA lock)    */
/*-------------------------------------------------------------------*/
static void commadpt_clean_device( DEVBLK* dev )
{
    if (!dev)
    {
        /*
         * Shouldn't happen.. But during shutdown, some weird
         * things happen !
         */
        return;
    }

    if (dev->commadpt)
    {
        commadpt_ring_terminate( &dev->commadpt->inbfr,   dev->ccwtrace );
        commadpt_ring_terminate( &dev->commadpt->outbfr,  dev->ccwtrace );
        commadpt_ring_terminate( &dev->commadpt->rdwrk,   dev->ccwtrace );
        commadpt_ring_terminate( &dev->commadpt->pollbfr, dev->ccwtrace );
        commadpt_ring_terminate( &dev->commadpt->ttybuf,  dev->ccwtrace );

        /* release the CA lock */
        release_lock( &dev->commadpt->lock );

        free( dev->commadpt );
        dev->commadpt = NULL;

        if (dev->ccwtrace)
        {
            // "%1d:%04X COMM: clean: %s"
            WRMSG( HHC01052, "D", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, "control block freed" );
        }
    }
    else
    {
        if (dev->ccwtrace)
        {
            // "%1d:%04X COMM: clean: %s"
            WRMSG( HHC01052, "D", SSID_TO_LCSS( dev->ssid ),
                dev->devnum, "control block not freed: not allocated" );
        }
    }
}

/*-------------------------------------------------------------------*/
/* Allocate initial private structures                               */
/*-------------------------------------------------------------------*/
static int commadpt_alloc_device(DEVBLK *dev)
{
    dev->commadpt=malloc(sizeof(COMMADPT));
    if(dev->commadpt==NULL)
    {
        char buf[40];
        MSGBUF(buf, "malloc(%d)", (int)sizeof(COMMADPT));
        WRMSG(HHC01000, "E",LCSS_DEVNUM, buf, strerror(errno));
        return -1;
    }
    memset(dev->commadpt, 0, sizeof(COMMADPT) );
    commadpt_ring_init(&dev->commadpt->inbfr,4096,dev->ccwtrace);
    commadpt_ring_init(&dev->commadpt->outbfr,4096,dev->ccwtrace);
    commadpt_ring_init(&dev->commadpt->pollbfr,4096,dev->ccwtrace);
    commadpt_ring_init(&dev->commadpt->rdwrk,65536,dev->ccwtrace);
    commadpt_ring_init(&dev->commadpt->ttybuf,TTYLINE_SZ,dev->ccwtrace);
    dev->commadpt->dev=dev;
    return 0;
}

/*-------------------------------------------------------------------*/
/* Parsing utilities                                                 */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* commadpt_getport : returns a port number or -1                    */
/*-------------------------------------------------------------------*/
static int commadpt_getport(char *txt)
{
    int pno;
    struct servent *se;
    pno=atoi(txt);
    if(pno==0)
    {
        se=getservbyname(txt,"tcp");
        if(se==NULL)
        {
            return -1;
        }
        pno=se->s_port;
    }
    return(pno);
}

/*-------------------------------------------------------------------*/
/* commadpt_getaddr : set an in_addr_t if ok, else return -1         */
/*-------------------------------------------------------------------*/
static int commadpt_getaddr(in_addr_t *ia,char *txt)
{
    struct hostent *he;
    he=gethostbyname(txt);
    if(he==NULL)
    {
        return(-1);
    }
    memcpy(ia,he->h_addr_list[0],4);
    return(0);
}

/*-------------------------------------------------------------------*/
/* commadpt_connout : make a tcp outgoing call                       */
/* return values : 0 -> call succeeded or initiated                  */
/*                <0 -> call failed                                  */
/*-------------------------------------------------------------------*/
static int commadpt_connout(COMMADPT *ca)
{
    int rc;
    char        wbfr[256];
    struct      sockaddr_in     sin;
    struct      in_addr intmp;
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=ca->rhost;
    sin.sin_port=htons(ca->rport);
    if(socket_is_socket(ca->sfd))
    {
        close_socket(ca->sfd);
        ca->connect=0;
    }
    ca->sfd=socket(AF_INET,SOCK_STREAM,0);
    /* set socket to NON-blocking mode */
    socket_set_blocking_mode(ca->sfd,0);
    rc=connect(ca->sfd,(struct sockaddr *)&sin,sizeof(sin));
    if(rc<0)
    {
#if defined(_MSVC_)
        if(HSO_errno==HSO_EWOULDBLOCK)
#else /* defined(_MSVC_) */
        if(HSO_errno==HSO_EINPROGRESS)
#endif /* defined(_MSVC_) */
        {
            return(0);
        }
        else
        {
            VERIFY(!strerror_r(HSO_errno,wbfr,256));
            intmp.s_addr=ca->rhost;
            WRMSG(HHC01001, "I",
                    SSID_TO_LCSS(ca->dev->ssid),
                    ca->devnum,
                    inet_ntoa(intmp),
                    ca->rport,
                    wbfr);
            close_socket(ca->sfd);
            ca->connect=0;
            return(-1);
        }
    }
    ca->connect=1;
    return(0);
}

/*-------------------------------------------------------------------*/
/* commadpt_initiate_userdial : interpret DIAL data and initiate call*/
/* return values : 0 -> call succeeded or initiated                  */
/*                <0 -> call failed                                  */
/*-------------------------------------------------------------------*/
static int     commadpt_initiate_userdial(COMMADPT *ca)
{
    int dotcount;       /* Number of seps (the 4th is the port separator) */
    int i;              /* work                                           */
    int cur;            /* Current section                                */
    in_addr_t   destip; /* Destination IP address                         */
    U16 destport;       /* Destination TCP port                           */
    int incdata;        /* Incorrect dial data found                      */
    int goteon;         /* EON presence flag                              */

   /* See the DIAL CCW portion in execute_ccw for dial format information */

    incdata=0;
    goteon=0;
    dotcount=0;
    cur=0;
    destip=0;
    for(i=0;i<ca->dialcount;i++)
    {
        if(goteon)
        {
            /* EON MUST be last data byte */
            if(ca->dev->ccwtrace)
            {
                WRMSG(HHC01066,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum);
            }
            incdata=1;
            break;
        }
        switch(ca->dialdata[i]&0x0f)
        {
            case 0x0d:  /* SEP */
                if(dotcount<4)
                {
                    if(cur>255)
                    {
                        incdata=1;
                        if(ca->dev->ccwtrace)
                        {
                            WRMSG(HHC01067,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum,dotcount+1);
                            WRMSG(HHC01068,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum,cur,cur);
                        }
                        break;
                    }
                    destip<<=8;
                    destip+=cur;
                    cur=0;
                    dotcount++;
                }
                else
                {
                    incdata=1;
                    if(ca->dev->ccwtrace)
                    {
                        WRMSG(HHC01069,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum);
                    }
                    break;
                }
                break;
            case 0x0c: /* EON */
                goteon=1;
                break;

                /* A,B,E,F not valid */
            case 0x0a:
            case 0x0b:
            case 0x0e:
            case 0x0f:
                incdata=1;
                if(ca->dev->ccwtrace)
                {
                    WRMSG(HHC01070,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum,ca->dialdata[i]);
                }
                break;
            default:
                cur*=10;
                cur+=ca->dialdata[i]&0x0f;
                break;
        }
        if(incdata)
        {
            break;
        }
    }
    if(incdata)
    {
        return -1;
    }
    if(dotcount<4)
    {
        if(ca->dev->ccwtrace)
        {
            WRMSG(HHC01071,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum,dotcount);
        }
        return -1;
    }
    if(cur>65535)
    {
        if(ca->dev->ccwtrace)
        {
            WRMSG(HHC01072,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum,cur);
        }
        return -1;
    }
    destport=cur;
    /* Update RHOST/RPORT */
    ca->rport=destport;
    ca->rhost=destip;
    return(commadpt_connout(ca));
}

static void connect_message(int sfd, int devnum, int term, int binary_opt)
{
struct      sockaddr_in client;
socklen_t   namelen;
char       *ipaddr;
char        msgtext[256];

    namelen = sizeof(client);
    (void)getpeername (sfd, (struct sockaddr *)&client, &namelen);
    ipaddr = inet_ntoa(client.sin_addr);

    MSGBUF( msgtext,
            MSG( HHC01073, "I", ipaddr, (int)ntohs(client.sin_port),
                           devnum, (term == COMMADPT_TERM_TTY) ? "TTY" : "2741" ) );

    VERIFY(0 <= write(sfd, msgtext, (u_int)strlen(msgtext)));
    VERIFY(2 == write(sfd, "\r\n", 2));

    WRMSG(HHC01073,"I", ipaddr, (int)ntohs(client.sin_port), devnum, (term == COMMADPT_TERM_TTY) ? "TTY" : "2741");

    if (binary_opt)
        VERIFY(sizeof(telnet_binary) == write(sfd, telnet_binary, sizeof(telnet_binary)));

    return;
}

/*-------------------------------------------------------------------*/
/* Communication Thread - Read socket data (after POLL request       */
/*-------------------------------------------------------------------*/
static int commadpt_read_poll(COMMADPT *ca)
{
    BYTE b;
    int rc;
    while((rc=read_socket(ca->sfd,&b,1))>0)
    {
        if(b==0x32)
        {
            continue;
        }
        if(b==0x37)
        {
            return(1);
        }
    }
    if(rc>0)
    {
        /* Store POLL IX in bfr followed by byte */
        commadpt_ring_push(&ca->inbfr,ca->pollix);
        commadpt_ring_push(&ca->inbfr,b);
        return(2);
    }
    return(0);
}

static void commadpt_read_tty(COMMADPT *ca, BYTE * bfr, int len)
{
    BYTE        bfr3[3];
    BYTE        c;
    BYTE        dump_buf[TTYLINE_SZ];
    int         dump_bp=0;
    BYTE        tty_buf[TTYLINE_SZ];
    int         tty_bp=0;
    int i1;
    u_int j;
    int crflag = 0;
    for (i1 = 0; i1 < len; i1++)
    {
        c = (unsigned char) bfr[i1];
        if (ca->telnet_opt)
        {
            ca->telnet_opt = 0;
            if(ca->dev->ccwtrace)
                WRMSG(HHC01053,"D",
                        SSID_TO_LCSS(ca->dev->ssid),
                        ca->dev->devnum,
                        ca->telnet_cmd, c);
            if (c == 0x00 && ca->binary_opt)
                continue; /* for binary: assume it's a response, so don't answer */
            bfr3[0] = 0xff;  /* IAC */
        /* set won't/don't for all received commands */
            bfr3[1] = (ca->telnet_cmd == 0xfd) ? 0xfc : 0xfe;
            bfr3[2] = c;
            if(ca->dev->ccwtrace)
                WRMSG(HHC01054,"D",
                        SSID_TO_LCSS(ca->dev->ssid),
                        ca->dev->devnum,
                        bfr3[1], bfr3[2]);
            commadpt_ring_pushbfr(&ca->outbfr,bfr3,3);
            continue;
        }
        if (ca->telnet_iac)
        {
            ca->telnet_iac = 0;
            if(ca->dev->ccwtrace)
                WRMSG(HHC01055,"D",
                        SSID_TO_LCSS(ca->dev->ssid),
                        ca->dev->devnum,
                        c);
            switch (c)
            {
                case 0xFB:  /* TELNET WILL option cmd */
                case 0xFD:  /* TELNET DO option cmd */
                    ca->telnet_opt = 1;
                    ca->telnet_cmd = c;
                    break;
                case 0xF4:  /* TELNET interrupt */
                    if (!ca->telnet_int)
                    {
                        ca->telnet_int = 1;
                        commadpt_ring_flush(&ca->ttybuf);
                        commadpt_ring_flush(&ca->inbfr);
                        commadpt_ring_flush(&ca->rdwrk);
                        commadpt_ring_flush(&ca->outbfr);
                    }
                    break;
            }
            continue;
        }
        if (c == 0xFF)
        {  /* TELNET IAC */
            ca->telnet_iac = 1;
            continue;
        }
        else
        {
            ca->telnet_iac = 0;
        }
        if (c == ca->eol_char) { // char was CR ?
            crflag = 1;
        }
        if (c == 0x03 && ca->dumb_break)
        {  /* Ctrl-C */
            ca->telnet_int = 1;
            commadpt_ring_flush(&ca->ttybuf);
            commadpt_ring_flush(&ca->inbfr);
            commadpt_ring_flush(&ca->rdwrk);
            commadpt_ring_flush(&ca->outbfr);
            continue;
        }
        commadpt_ring_push(&ca->ttybuf,c);
    }
    if (crflag)
    {   /* process complete line, perform editing and translation, etc. */
        if (ca->prepend_length)
        {
            for (i1 = 0; i1 < ca->prepend_length; i1++) {
                tty_buf[tty_bp++] = ca->prepend_bytes[i1];
                if (tty_bp >= TTYLINE_SZ)
                    tty_bp = TTYLINE_SZ - 1;   // prevent buf overflow
            }
        }
        while (ca->ttybuf.havedata)
        {
            c = commadpt_ring_pop(&ca->ttybuf);
            if ((c & 0x7f) == 0x08 && ca->dumb_bs)   // backspace editing
            {
                if (tty_bp > 0)
                    tty_bp --;
                continue;
            }
            if (ca->input_byte_skip_table[c])
                continue;   // skip this byte per cfg

            if (1
                && ca->crlf2cr_opt      /* dhd: CRLF2CR option? */
                && (c & 0x7f) == 0x0a   /* dhd: is this a <LF>? */
                && tty_bp > 0           /* dhd: and something in buffer? */
            )
            {
                if (0xb1 == tty_buf[tty_bp-1])  /* dhd: <LF> follows <CR>? */
                    continue;                   /* dhd: skip <LF> to avoid problems */
            }

            if (!(ca->rxvt4apl || !ca->code_table_fromebcdic))
            { /* tty33 and 2741 emulation are 7-bit, code=none and rxvt4apl want 8 bit */
                c &= 0x7f;  // make 7 bit ASCII
            }
            if  (ca->uctrans && c >= 'a' && c <= 'z')
            {
                c = toupper( (unsigned char)c );     /* make uppercase */
            }
            /* now map the character from ASCII into proper S/370 byte format */
            if (ca->term == COMMADPT_TERM_TTY)
            {
                if (byte_parity_table[(unsigned int)(c & 0x7f)])
                    c |= 0x80;     // make even parity

                c = reverse_bits( c );
            }
            else
            {   /* 2741 */
                if (ca->rxvt4apl)
                {
                    if (overstrike_map[c] == 1)
                    {
                        for (j = 0; j < sizeof(overstrike_rxvt4apl_chars); j++)
                        {
                            if (c == overstrike_rxvt4apl_chars[j])
                            {
                                tty_buf[tty_bp++] = overstrike_2741_pairs[j*2];
                                if (tty_bp >= TTYLINE_SZ)
                                    tty_bp = TTYLINE_SZ - 1;   // prevent buf overflow
                                tty_buf[tty_bp++] = 0xDD;      // 2741 backspace
                                if (tty_bp >= TTYLINE_SZ)
                                    tty_bp = TTYLINE_SZ - 1;   // prevent buf overflow
                                c = overstrike_2741_pairs[ (j*2) + 1];
                            }
                        }
                    }
                    else
                    {
                        c = rxvt4apl_to_2741[c];
                    }
                }
                else if (ca->code_table_fromebcdic)
                {
                    c = host_to_guest(c & 0x7f);  // first translate to EBCDIC
                    c = ca->code_table_fromebcdic[ c ];   // then to 2741 code
                }
            }
            tty_buf[tty_bp++] = c;
            if (tty_bp >= TTYLINE_SZ)
                tty_bp = TTYLINE_SZ - 1;   // prevent buf overflow
        }
        if (ca->append_length)
        {
            for (i1 = 0; i1 < ca->append_length; i1++) {
                tty_buf[tty_bp++] = ca->append_bytes[i1];
                if (tty_bp >= TTYLINE_SZ)
                    tty_bp = TTYLINE_SZ - 1;   // prevent buf overflow
            }
        }
        if (tty_bp > 0) {
            for (i1 = 0; i1 < tty_bp; i1++) {
                commadpt_ring_push(&ca->rdwrk, tty_buf[i1]);
                dump_buf[dump_bp++] = tty_buf[i1];
                if (dump_bp >= TTYLINE_SZ) dump_bp = TTYLINE_SZ - 1;
            }
        }
        logdump("RCV2",ca->dev,dump_buf,dump_bp);
        ca->eol_flag = 1; // set end of line flag
        if (ca->sendcr_opt)
        {
            /* move carriage to left margin */
            commadpt_ring_push(&ca->outbfr,0x0d);
        }
    } /* end of if(crflag) */
}

/*-------------------------------------------------------------------*/
/* Communication Thread - Read socket data                           */
/*-------------------------------------------------------------------*/
static void commadpt_read(COMMADPT *ca)
{
BYTE    bfr[256];
int     gotdata;
int     rc;

    gotdata=0;
    for (;;)
    {
     /* if (IS_BSC_LNCTL(ca))
        {
            rc=read_socket(ca->sfd,bfr,256);
        }
        else
        { */
            /* read_socket has changed from 3.04 to 3.06 - async needs old way */
            /* is BSC similarly broken? */
            /* --> Yes, it is! I propose to fully remove the if/else construct     */
            /*                 i.e. to handle BSC and async identically here (JW)  */
#ifdef _MSVC_
            rc=recv(ca->sfd,bfr,256,0);
#else
            rc=read(ca->sfd,bfr,256);
#endif
     /* } */
        if (rc <= 0)
            break;
        logdump("RECV",ca->dev,bfr,rc);
        if (IS_ASYNC_LNCTL(ca))
        {
            commadpt_read_tty(ca, bfr, rc);
        }
        else
        {
            commadpt_ring_pushbfr(&ca->inbfr,bfr,(size_t)rc);
        }  /* end of else (async) */
        gotdata=1;
    }
    if(!gotdata)
    {
        if(ca->connect)
        {
            ca->connect=0;
            close_socket(ca->sfd);
            ca->sfd=-1;
            if(ca->curpending!=COMMADPT_PEND_IDLE)
            {
                ca->curpending=COMMADPT_PEND_IDLE;
                signal_condition(&ca->ipc);
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/* Communication Thread - Set TimeOut                                */
/*-------------------------------------------------------------------*/
static struct timeval *commadpt_setto(struct timeval *tv,int tmo)
{
    if(tmo!=0)
    {
        if(tmo<0)
        {
            tv->tv_sec=0;
            tv->tv_usec=1;
        }
        else
        {
            tv->tv_sec=tmo/1000;
            tv->tv_usec=(tmo%1000)*1000;
        }
        return(tv);
    }
    return(NULL);
}

/*-------------------------------------------------------------------*/
/* Communication Thread - Set TCP Keepalive                          */
/*-------------------------------------------------------------------*/
static void SET_COMM_KEEPALIVE( int tempfd, COMMADPT* ca )
{
    if (ca->kaidle && ca->kaintv && ca->kacnt)
    {
#if !defined( HAVE_BASIC_KEEPALIVE )

        UNREFERENCED( tempfd );

        // "%1d:%04X COMM: This build of Hercules does not support TCP keepalive"
        WRMSG( HHC01094, "E", SSID_TO_LCSS( ca->dev->ssid ), ca->devnum );

#else // basic, partial or full: must attempt setting keepalive

        int rc, idle, intv, cnt;

  #if !defined( HAVE_FULL_KEEPALIVE ) && !defined( HAVE_PARTIAL_KEEPALIVE )

        // "%1d:%04X COMM: This build of Hercules has only basic TCP keepalive support"
        WRMSG( HHC01095, "W", SSID_TO_LCSS( ca->dev->ssid ), ca->devnum );

  #elif !defined( HAVE_FULL_KEEPALIVE )

        // "%1d:%04X COMM: This build of Hercules has only partial TCP keepalive support"
        WRMSG( HHC01096, "W", SSID_TO_LCSS( ca->dev->ssid ), ca->devnum );

  #endif // (basic or partial)

        /* Try setting the values first */
        rc = set_socket_keepalive( tempfd,
            ca->kaidle, ca->kaintv, ca->kacnt );

        if (rc < 0)
        {
            // "%1d:%04X COMM: error in function %s: %s"
            WRMSG( HHC01000, "E",
                SSID_TO_LCSS( ca->dev->ssid ), ca->devnum,
                "set_socket_keepalive()", strerror( HSO_errno ));
            return;
        }

        /* Issue partial success warning if needed */
        if (rc > 0)
            // "%1d:%04X COMM: Not all TCP keepalive settings honored"
            WRMSG( HHC01092, "W",
                SSID_TO_LCSS( ca->dev->ssid ), ca->devnum );

        /* Now see which values the system actually accepted */
        if (get_socket_keepalive( tempfd, &idle, &intv, &cnt ) < 0)
        {
            // "%1d:%04X COMM: error in function %s: %s"
            WRMSG( HHC01000, "E",
                SSID_TO_LCSS( ca->dev->ssid ), ca->devnum,
                "get_socket_keepalive()", strerror( HSO_errno ));
            return;
        }

        /* Save values actually being used for later */
        ca->kaidle = idle;
        ca->kaintv = intv;
        ca->kacnt  = cnt;

        // "%1d:%04X COMM: Keepalive: (%d,%d,%d)"
        WRMSG( HHC01093, "I",
            SSID_TO_LCSS( ca->dev->ssid ), ca->devnum,
            ca->kaidle, ca->kaintv, ca->kacnt );

#endif // (KEEPALIVE)
    }
}

/*-------------------------------------------------------------------*/
/* Communication Thread main loop                                    */
/*-------------------------------------------------------------------*/
static void *commadpt_thread(void *vca)
{
    COMMADPT    *ca;            /* Work CA Control Block Pointer     */
    int        sockopt;         /* Used for setsocketoption          */
    struct sockaddr_in sin;     /* bind socket address structure     */
    int devnum;                 /* device number copy for convenience*/
    int rc;                     /* return code from various rtns     */
    struct timeval tv;          /* select timeout structure          */
    struct timeval *seltv;      /* ptr to the timeout structure      */
    fd_set      rfd,wfd,xfd;    /* SELECT File Descriptor Sets       */
    BYTE        pipecom;        /* Byte read from IPC pipe           */
    int tempfd;                 /* Temporary FileDesc holder         */
    BYTE b;                     /* Work data byte                    */
    int writecont;              /* Write contention active           */
    int soerr;                  /* getsockopt SOERROR value          */
    socklen_t   soerrsz;        /* Size for getsockopt               */
    int maxfd;                  /* highest FD for select             */
    int ca_shutdown;            /* Thread shutdown internal flag     */
    int init_signaled;          /* Thread initialisation signaled    */
    int pollact;                /* A Poll Command is in progress     */
    int i;                      /* Ye Old Loop Counter               */
    char threadname[40];

    /*---------------------END OF DECLARES---------------------------*/

    /* fetch the commadpt structure */
    ca=(COMMADPT *)vca;

    /* Obtain the CA lock */
    obtain_lock(&ca->lock);

    /* get a work copy of devnum (for messages) */
    devnum=ca->devnum;

    /* reset shutdown flag */
    ca_shutdown=0;

    init_signaled=0;

    /* Set server thread priority; ignore any errors */
    SET_THREAD_PRIORITY( sysblk.srvprio, sysblk.qos_user_initiated );

    MSGBUF(threadname, "%1d:%04X communication thread", SSID_TO_LCSS(ca->dev->ssid), devnum);
    LOG_THREAD_BEGIN( threadname );

    pollact=0;  /* Initialise Poll activity flag */

    /* Determine if we should listen */
    /* if this is a DIAL=OUT only line, no listen is necessary */
    if(ca->dolisten)
    {
        /* Create the socket for a listen */
        ca->lfd=socket(AF_INET,SOCK_STREAM,0);
        if(!socket_is_socket(ca->lfd))
        {
            WRMSG(HHC01002, "E",SSID_TO_LCSS(ca->dev->ssid),devnum,strerror(HSO_errno));
            ca->have_cthread=0;
            release_lock(&ca->lock);
            return NULL;
        }
        /* Turn blocking I/O off */
        /* set socket to NON-blocking mode */
        socket_set_blocking_mode(ca->lfd,0);

        /* Reuse the address regardless of any */
        /* spurious connection on that port    */
        sockopt=1;
        setsockopt(ca->lfd,SOL_SOCKET,SO_REUSEADDR,(GETSET_SOCKOPT_T*)&sockopt,sizeof(sockopt));

        /* Bind the socket */
        sin.sin_family=AF_INET;
        sin.sin_addr.s_addr=ca->lhost;
        sin.sin_port=htons(ca->lport);
        while(1)
        {
            rc=bind(ca->lfd,(struct sockaddr *)&sin,sizeof(sin));
            if(rc<0)
            {
                if(HSO_errno==HSO_EADDRINUSE)
                {
                    WRMSG(HHC01003, "W",SSID_TO_LCSS(ca->dev->ssid),devnum,ca->lport);
                    /*
                     * Check for a shutdown condition on entry
                     */
                    if(ca->curpending==COMMADPT_PEND_SHUTDOWN)
                    {
                        ca_shutdown=1;
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                        break;
                    }

                    /* Set to wait 5 seconds or input on the IPC pipe */
                    /* whichever comes 1st                            */
                    if(!init_signaled)
                    {
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                        init_signaled=1;
                    }

                    FD_ZERO(&rfd);
                    FD_ZERO(&wfd);
                    FD_ZERO(&xfd);
                    FD_SET(ca->pipe[1],&rfd);
                    tv.tv_sec=5;
                    tv.tv_usec=0;

                    release_lock(&ca->lock);
                    rc=select(ca->pipe[1]+1,&rfd,&wfd,&xfd,&tv);
                    obtain_lock(&ca->lock);
                    /*
                     * Check for a shutdown condition again after the sleep
                     */
                    if(ca->curpending==COMMADPT_PEND_SHUTDOWN)
                    {
                        ca_shutdown=1;
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                        break;
                    }
                    if(rc!=0)
                    {
                        /* Ignore any other command at this stage */
                        VERIFY(0 <= read_pipe(ca->pipe[1],&b,1));
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                    }
                }
                else
                {
                    WRMSG(HHC01000, "E",SSID_TO_LCSS(ca->dev->ssid),devnum,"bind()",strerror(HSO_errno));
                    ca_shutdown=1;
                    break;
                }
            }
            else
            {
                break;
            }
        }
        /* Start the listen */
        if(!ca_shutdown)
        {
            listen(ca->lfd,10);
            WRMSG(HHC01004, "I",
                    SSID_TO_LCSS(ca->dev->ssid),
                    devnum,
                    ca->lport);
            ca->listening=1;
        }
    }
    if(!init_signaled)
    {
        ca->curpending=COMMADPT_PEND_IDLE;
        signal_condition(&ca->ipc);
        init_signaled=1;
    }

    /* The MAIN select loop */
    /* It will listen on the following sockets : */
    /* ca->lfd : The listen socket */
    /* ca->sfd :
     *         read : When a read, prepare or DIAL command is in effect
     *        write : When a write contention occurs
     * ca->pipe[0] : Always
     *
     * A 3 Seconds timer is started for a read operation
     */

    while(!ca_shutdown)
    {
        FD_ZERO(&rfd);
        FD_ZERO(&wfd);
        FD_ZERO(&xfd);
        maxfd=0;
        if(ca->listening)
        {
                FD_SET(ca->lfd,&rfd);
                maxfd=maxfd<ca->lfd?ca->lfd:maxfd;
        }
        seltv=NULL;
        if(ca->dev->ccwtrace)
        {
            WRMSG(HHC01074,"D",
                            SSID_TO_LCSS(ca->dev->ssid),
                            devnum,
                            commadpt_pendccw_text[ca->curpending]);
        }
        writecont=0;
        switch(ca->curpending)
        {
            case COMMADPT_PEND_SHUTDOWN:
                ca_shutdown=1;
                break;
            case COMMADPT_PEND_IDLE:
                break;
            case COMMADPT_PEND_READ:
                if(!ca->connect)
                {
                    ca->curpending=COMMADPT_PEND_IDLE;
                    signal_condition(&ca->ipc);
                    break;
                }
                if(ca->inbfr.havedata || ca->eol_flag)
                {
                    if (ca->term == COMMADPT_TERM_2741) {
                        USLEEP(10000);
                    }
                    ca->curpending=COMMADPT_PEND_IDLE;
                    signal_condition(&ca->ipc);
                    break;
                }
                seltv=commadpt_setto(&tv,ca->rto);
                FD_SET(ca->sfd,&rfd);
                maxfd=maxfd<ca->sfd?ca->sfd:maxfd;
                break;
            case COMMADPT_PEND_POLL:
                /* Poll active check - provision for write contention */
                /* pollact will be reset when NON syn data is received*/
                /* or when the read times out                         */
                /* Also prevents WRITE from exiting early             */
                if(!pollact && !writecont)
                {
                    int gotenq;

                    pollact=1;
                    gotenq=0;
                    /* Send SYN+SYN */
                    commadpt_ring_push(&ca->outbfr,0x32);
                    commadpt_ring_push(&ca->outbfr,0x32);
                    /* Fill the Output ring with POLL Data */
                    /* Up to 7 chars or ENQ                */
                    for(i=0;i<7;i++)
                    {
                        if(!ca->pollbfr.havedata)
                        {
                            break;
                        }
                        ca->pollused++;
                        b=commadpt_ring_pop(&ca->pollbfr);
                        if(b!=0x2D)
                        {
                            commadpt_ring_push(&ca->outbfr,b);
                        }
                        else
                        {
                            gotenq=1;
                            break;
                        }
                    }
                    if(!gotenq)
                    {
                        if(ca->dev->ccwtrace)
                        {
                            WRMSG(HHC01075,"D",SSID_TO_LCSS(ca->dev->ssid),devnum);
                        }
                        ca->badpoll=1;
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                        break;
                    }
                    b=commadpt_ring_pop(&ca->pollbfr);
                    ca->pollix=b;
                    seltv=commadpt_setto(&tv,ca->pto);
                }
                if(!writecont && ca->pto!=0)
                {
                    /* Set tv value (have been set earlier) */
                    seltv=&tv;
                    /* Set to read data still               */
                    FD_SET(ca->sfd,&rfd);
                    maxfd=maxfd<ca->sfd?ca->sfd:maxfd;
                }
                /* FALLTHRU */
                /* DO NOT BREAK - Continue with WRITE processing */
            case COMMADPT_PEND_WRITE:
                if(!writecont)
                {
                    while(ca->outbfr.havedata)
                    {
                        b=commadpt_ring_pop(&ca->outbfr);
                        if(ca->dev->ccwtrace)
                        {
                                WRMSG(HHC01076,"D",SSID_TO_LCSS(ca->dev->ssid),ca->devnum,b);
                        }
                        rc=write_socket(ca->sfd,&b,1);
                        if(rc!=1)
                        {
                            if(0
#ifndef WIN32
                                || EAGAIN == errno
#endif
                                || HSO_EWOULDBLOCK == HSO_errno
                            )
                            {
                                /* Contending for write */
                                writecont=1;
                                FD_SET(ca->sfd,&wfd);
                                maxfd=maxfd<ca->sfd?ca->sfd:maxfd;
                                break;
                            }
                            else
                            {
                                close_socket(ca->sfd);
                                ca->sfd=-1;
                                ca->connect=0;
                                ca->curpending=COMMADPT_PEND_IDLE;
                                signal_condition(&ca->ipc);
                                break;
                            }
                        }
                    }
                }
                else
                {
                        FD_SET(ca->sfd,&wfd);
                        maxfd=maxfd<ca->sfd?ca->sfd:maxfd;
                }
                if(!writecont && !pollact)
                {
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                        break;
                }
                break;
            case COMMADPT_PEND_DIAL:
                if(ca->connect)
                {
                    ca->curpending=COMMADPT_PEND_IDLE;
                    signal_condition(&ca->ipc);
                    break;
                }
                rc=commadpt_initiate_userdial(ca);
                if(rc!=0 || (rc==0 && ca->connect))
                {
                    ca->curpending=COMMADPT_PEND_IDLE;
                    signal_condition(&ca->ipc);
                    break;
                }
                FD_SET(ca->sfd,&wfd);
#if defined(_MSVC_)
                FD_SET(ca->sfd,&xfd);
#endif /* defined(_MSVC_) */
                maxfd=maxfd<ca->sfd?ca->sfd:maxfd;
                break;
            case COMMADPT_PEND_ENABLE:
                if(ca->connect)
                {
                    ca->curpending=COMMADPT_PEND_IDLE;
                    signal_condition(&ca->ipc);
                    break;
                }
                switch(ca->dialin+ca->dialout*2)
                {
                    case 0: /* DIAL=NO */
                        /* callissued is set here when the call */
                        /* actually failed. But we want to time */
                        /* a bit for program issuing ENABLES in */
                        /* a tight loop                         */
                        if(ca->callissued)
                        {
                            seltv=commadpt_setto(&tv,ca->eto);
                            break;
                        }
                        /* Issue a Connect out */
                        rc=commadpt_connout(ca);
                        if(rc==0)
                        {
                            /* Call issued */
                            if(ca->connect)
                            {
                                /* Call completed already */
                                ca->curpending=COMMADPT_PEND_IDLE;
                                signal_condition(&ca->ipc);
                            }
                            else
                            {
                                /* Call initiated - FD will be ready */
                                /* for writing when the connect ends */
                                /* getsockopt/SOERROR will tell if   */
                                /* the call was sucessfull or not    */
                                FD_SET(ca->sfd,&wfd);
#if defined(_MSVC_)
                                FD_SET(ca->sfd,&xfd);
#endif /* defined(_MSVC_) */
                                maxfd=maxfd<ca->sfd?ca->sfd:maxfd;
                                ca->callissued=1;
                            }
                        }
                        /* Call did not succeed                                 */
                        /* Manual says : on a leased line, if DSR is not up     */
                        /* the terminate enable after a timeout.. That is       */
                        /* what the call just did (although the time out        */
                        /* was probably instantaneous)                          */
                        /* This is the equivalent of the comm equipment         */
                        /* being offline                                        */
                        /*       INITIATE A 3 SECOND TIMEOUT                    */
                        /* to prevent OSes from issuing a loop of ENABLES       */
                        else
                        {
                            seltv=commadpt_setto(&tv,ca->eto);
                        }
                        break;
                    default:
                    case 3: /* DIAL=INOUT */
                    case 1: /* DIAL=IN */
                        /* Wait forever */
                        break;
                    case 2: /* DIAL=OUT */
                        /* Makes no sense                               */
                        /* line must be enabled through a DIAL command  */
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                        break;
                }
                /* For cases not DIAL=OUT, the listen is already started */
                break;

                /* The CCW Executor says : DISABLE */
            case COMMADPT_PEND_DISABLE:
                if(ca->connect)
                {
                    close_socket(ca->sfd);
                    ca->sfd=-1;
                    ca->connect=0;
                }
                ca->curpending=COMMADPT_PEND_IDLE;
                signal_condition(&ca->ipc);
                break;

                /* A PREPARE has been issued */
            case COMMADPT_PEND_PREPARE:
                if(!ca->connect || ca->inbfr.havedata)
                {
                    ca->curpending=COMMADPT_PEND_IDLE;
                    signal_condition(&ca->ipc);
                    break;
                }
                FD_SET(ca->sfd,&rfd);
                maxfd=maxfd<ca->sfd?ca->sfd:maxfd;
                break;

                /* Don't know - shouldn't be here anyway */
            default:
                break;
        }

        /* If the CA is shutting down, exit the loop now */
        if(ca_shutdown)
        {
            ca->curpending=COMMADPT_PEND_IDLE;
            signal_condition(&ca->ipc);
            break;
        }

        /* Set the IPC pipe in the select */
        FD_SET(ca->pipe[0],&rfd);

        /* The the MAX File Desc for Arg 1 of SELECT */
        maxfd=maxfd<ca->pipe[0]?ca->pipe[0]:maxfd;
        maxfd++;

        /* Release the CA Lock before the select - all FDs addressed by the select are only */
        /* handled by the thread, and communication from CCW Executor/others to this thread */
        /* is via the pipe, which queues the info                                           */
        release_lock(&ca->lock);

        if(ca->dev->ccwtrace)
        {
                WRMSG(HHC01077,"D",SSID_TO_LCSS(ca->dev->ssid),devnum,maxfd,commadpt_pendccw_text[ca->curpending]);
        }
        rc=select(maxfd,&rfd,&wfd,&xfd,seltv);

        if(ca->dev->ccwtrace)
        {
                WRMSG(HHC01078,"D",SSID_TO_LCSS(ca->dev->ssid),devnum,rc);
        }
        /* Get the CA lock back */
        obtain_lock(&ca->lock);

        if(rc==-1)
        {
            if(errno==EINTR)
                continue;  /* thanks Fish! */
            WRMSG(HHC01000, "E",SSID_TO_LCSS(ca->dev->ssid),devnum,"select()",strerror(HSO_errno));
            break;
        }

        /* Select timed out */
        if(rc==0)
        {
            pollact=0;  /* Poll not active */
            if(ca->dev->ccwtrace)
            {
                WRMSG(HHC01079,"D",SSID_TO_LCSS(ca->dev->ssid),devnum);
            }
            /* Reset Call issued flag */
            ca->callissued=0;

            /* timeout condition */
            signal_condition(&ca->ipc);
            ca->curpending=COMMADPT_PEND_IDLE;
            continue;
        }

        if(FD_ISSET(ca->pipe[0],&rfd))
        {
            rc=read_pipe(ca->pipe[0],&pipecom,1);
            if(rc==0)
            {
                if(ca->dev->ccwtrace)
                {
                        WRMSG(HHC01080,"D",SSID_TO_LCSS(ca->dev->ssid),devnum);
                }
                /* Pipe closed : terminate thread & release CA */
                ca_shutdown=1;
                break;
            }
            if(ca->dev->ccwtrace)
            {
                WRMSG(HHC01081,"D",SSID_TO_LCSS(ca->dev->ssid),devnum,pipecom);
            }
            switch(pipecom)
            {
                case 0: /* redrive select */
                        /* occurs when a new CCW is being executed */
                    break;
                case 1: /* Halt current I/O */
                    ca->callissued=0;
                    if(ca->curpending==COMMADPT_PEND_DIAL)
                    {
                        close_socket(ca->sfd);
                        ca->sfd=-1;
                    }
                    ca->curpending=COMMADPT_PEND_IDLE;
                    ca->haltpending=1;
                    signal_condition(&ca->ipc);
                    signal_condition(&ca->ipc_halt);    /* Tell the halt initiator too */
                    break;
                default:
                    break;
            }
            continue;
        }
        if(ca->connect)
        {
            if(FD_ISSET(ca->sfd,&rfd))
            {
                int dopoll;
                dopoll=0;
                if(ca->dev->ccwtrace)
                {
                        WRMSG(HHC01082,"D",SSID_TO_LCSS(ca->dev->ssid),devnum);
                }
                if(pollact && IS_BSC_LNCTL(ca))
                {
                    switch(commadpt_read_poll(ca))
                    {
                        case 0: /* Only SYNs received */
                                /* Continue the timeout */
                            dopoll=1;
                            break;
                        case 1: /* EOT Received */
                            /* Send next poll sequence */
                            pollact=0;
                            dopoll=1;
                            break;
                        case 2: /* Something else received */
                            /* Index byte already stored in inbfr */
                            /* read the remaining data and return */
                            ca->pollsm=1;
                            dopoll=0;
                            break;
                        default:
                            /* Same as 0 */
                            dopoll=1;
                            break;
                    }
                }
                if(IS_ASYNC_LNCTL(ca) || !dopoll)
                {
                    commadpt_read(ca);
                    if(IS_ASYNC_LNCTL(ca) && !ca->eol_flag && !ca->telnet_int) {
                        /* async: EOL char not yet received and not attn: no data to read */
                        /* ... just remain in COMMADPT_PEND_READ state ... */
                    } else {
                        ca->curpending=COMMADPT_PEND_IDLE;
                        signal_condition(&ca->ipc);
                    }
                    continue;
                }
            }
        }
        if(ca->sfd>=0)
        {
#if defined(_MSVC_)
            if(FD_ISSET(ca->sfd,&wfd) || FD_ISSET(ca->sfd,&xfd))
#else /* defined(_MSVC_) */
            if(FD_ISSET(ca->sfd,&wfd))
#endif /* defined(_MSVC_) */
            {
                if(ca->dev->ccwtrace)
                {
                        WRMSG(HHC01083,"D",SSID_TO_LCSS(ca->dev->ssid),devnum);
                }
                switch(ca->curpending)
                {
                    case COMMADPT_PEND_DIAL:
                    case COMMADPT_PEND_ENABLE:  /* Leased line enable call case */
                    soerrsz=sizeof(soerr);
                    getsockopt(ca->sfd,SOL_SOCKET,SO_ERROR,(GETSET_SOCKOPT_T*)&soerr,&soerrsz);
#if defined(_MSVC_)
                    if(FD_ISSET(ca->sfd,&wfd))
#else /* defined(_MSVC_) */
                    if(soerr==0)
#endif /* defined(_MSVC_) */
                    {
                        ca->connect=1;
                    }
                    else
#if defined(_MSVC_)
                    if(FD_ISSET(ca->sfd,&xfd))
#else /* defined(_MSVC_) */
                    if(soerr!=0)
#endif /* defined(_MSVC_) */
                    {
                        WRMSG(HHC01005, "W",SSID_TO_LCSS(ca->dev->ssid),devnum,commadpt_pendccw_text[ca->curpending],strerror(soerr));
                        if(ca->curpending==COMMADPT_PEND_ENABLE)
                        {
                            /* Ensure top of the loop doesn't restart a new call */
                            /* but starts a 3 second timer instead               */
                            ca->callissued=1;
                        }
                        ca->connect=0;
                        close_socket(ca->sfd);
                        ca->sfd=-1;
                    }
                    signal_condition(&ca->ipc);
                    ca->curpending=COMMADPT_PEND_IDLE;
                    break;

                    case COMMADPT_PEND_WRITE:
                    writecont=0;
                    break;

                    default:
                    break;
                }
                continue;
            }
        }
        /* Test for incoming call */
        if(ca->listening)
        {
            if(FD_ISSET(ca->lfd,&rfd))
            {
                WRMSG(HHC01006, "I",SSID_TO_LCSS(ca->dev->ssid),devnum);
                tempfd=accept(ca->lfd,NULL,0);
                if(tempfd<0)
                {
                    continue;
                }
                /* If the line is already connected, just close */
                /* this call                                    */
                if(ca->connect)
                {
                    close_socket(tempfd);
                    continue;
                }
                /* Turn non-blocking I/O on */
                /* set socket to NON-blocking mode */
                socket_set_blocking_mode(tempfd,0);

                /* Check the line type & current operation */

                /* if DIAL=IN or DIAL=INOUT or DIAL=NO */
                if(ca->dialin || (ca->dialin+ca->dialout==0))
                {
                    /* check if ENABLE is in progress */
                    if(ca->curpending==COMMADPT_PEND_ENABLE)
                    {
                        /* Accept the call, indicate the line */
                        /* is connected and notify CCW exec   */
                        ca->curpending=COMMADPT_PEND_IDLE;
                        ca->connect=1;

                        /* dhd - Try to detect dropped connections */
                        SET_COMM_KEEPALIVE( tempfd, ca );

                        ca->sfd=tempfd;
                        signal_condition(&ca->ipc);
                        if (IS_ASYNC_LNCTL(ca)) {
                            connect_message(ca->sfd, ca->devnum, ca->term, ca->binary_opt);
                        }
                        continue;
                    }
                    /* if this is a leased line, accept the */
                    /* call anyway                          */
                    if(ca->dialin==0)
                    {
                        ca->connect=1;

                        /* dhd - Try to detect dropped connections */
                        SET_COMM_KEEPALIVE( tempfd, ca );

                        ca->sfd=tempfd;
                        if (IS_ASYNC_LNCTL(ca)) {
                            connect_message(ca->sfd, ca->devnum, ca->term, ca->binary_opt);
                        }
                        continue;
                    }
                }
                /* All other cases : just reject the call */
                close_socket(tempfd);
            }
        }
    }
    ca->curpending=COMMADPT_PEND_CLOSED;
    /* Check if we already signaled the init process  */
    if(!init_signaled)
    {
        signal_condition(&ca->ipc);
    }
    /* The CA is shutting down - terminate the thread */
    /* NOTE : the requestor was already notified upon */
    /*        detection of PEND_SHTDOWN. However      */
    /*        the requestor will only run when the    */
    /*        lock is released, because back          */
    /*        notification was made while holding     */
    /*        the lock                                */
    LOG_THREAD_END( threadname );
    release_lock(&ca->lock);
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Wakeup the comm thread                                            */
/* Code : 0 -> Just wakeup the thread to redrive the select          */
/* Code : 1 -> Halt the current executing I/O                        */
/*-------------------------------------------------------------------*/
static void commadpt_wakeup(COMMADPT *ca,BYTE code)
{
    VERIFY(1 == write_pipe(ca->pipe[1],&code,1));
}

/*-------------------------------------------------------------------*/
/* Wait for a copndition from the thread                             */
/* MUST HOLD the CA lock                                             */
/*-------------------------------------------------------------------*/
static void commadpt_wait(DEVBLK *dev)
{
    COMMADPT *ca;
    ca=dev->commadpt;
    wait_condition(&ca->ipc,&ca->lock);
}

/*-------------------------------------------------------------------*/
/* Halt currently executing I/O command                              */
/*-------------------------------------------------------------------*/
static void commadpt_halt_or_clear( DEVBLK* dev )
{
    if (dev->busy)
    {
        obtain_lock( &dev->commadpt->lock );
        {
            commadpt_wakeup( dev->commadpt, 1 );

            /* Due to the mysteries of the host OS scheduling */
            /* the wait_condition may or may not exit after   */
            /* the CCW executor thread relinquishes control   */
            /* This however should not be of any concern      */
            /*                                                */
            /* but returning from the wait guarantees that    */
            /* the working thread will (or has) notified      */
            /* the CCW executor to terminate the current I/O  */

            wait_condition( &dev->commadpt->ipc_halt, &dev->commadpt->lock );
            dev->commadpt->haltprepare = 1; /* part of APL\360 2741 race cond I circumvention */
        }
        release_lock( &dev->commadpt->lock );
    }
}

/* The following 3 MSG functions ensure only 1 (one)  */
/* hardcoded instance exist for the same numbered msg */
/* that is issued on multiple situations              */
static void msg01007e(DEVBLK *dev,char *kw,char *kv)
{
    // "%1d:%04X COMM: option %s value %s invalid"
    WRMSG(HHC01007, "E",LCSS_DEVNUM,kw,kv);
}
static void msg01008e(DEVBLK *dev,char *dialt,char *kw)
{
    // "%1d:%04X COMM: missing parameter: DIAL(%s) and %s not specified"
    WRMSG(HHC01008, "E",LCSS_DEVNUM,dialt,kw);
}
static void msg01009w(DEVBLK *dev,char *dialt,char *kw,char *kv)
{
    // "%1d:%04X COMM: conflicting parameter: DIAL(%s) and %s=%s specified"
    WRMSG(HHC01009, "W",LCSS_DEVNUM,dialt,kw,kv);
    // "%1d:%04X COMM: RPORT parameter ignored"
    WRMSG(HHC01010, "I",LCSS_DEVNUM);
}

/*-------------------------------------------------------------------*/
/* Device Initialization                                             */
/*-------------------------------------------------------------------*/
static int commadpt_init_handler (DEVBLK *dev, int argc, char *argv[])
{
    char    thread_name[33];
    int     i,j;
    int     ix;
    int     rc;
    int     pc;             /* Parse code */
    int     errcnt;
    struct  in_addr in_temp;
    char   *dialt;
    char    fmtbfr[65];
    int     etospec;        /* ETO= Specified */
    union   { int num; char text[MAX_PARSER_STRLEN+1];  /* (+1 for null terminator) */ } res;
    char    bf[4];

    /* For re-initialisation, close the existing file, if any */
    if (dev->fd >= 0)
        (dev->hnd->close)(dev);

    dev->excps = 0;

    dev->devtype=0x2703;
    if(dev->ccwtrace)
    {
        WRMSG(HHC01058,"D",LCSS_DEVNUM);
    }

    rc=commadpt_alloc_device(dev);
    if(rc<0)
    {
        WRMSG(HHC01011, "I",LCSS_DEVNUM);
        return(-1);
    }
    if(dev->ccwtrace)
    {
        WRMSG(HHC01059,"D",LCSS_DEVNUM);
    }
    errcnt=0;
    /*
     * Initialise ports & hosts
    */
    dev->commadpt->sfd=-1;
    dev->commadpt->lport=0;
    dev->commadpt->rport=0;
    dev->commadpt->lhost=INADDR_ANY;
    dev->commadpt->rhost=INADDR_NONE;
    dev->commadpt->dialin=0;
    dev->commadpt->dialout=1;
    dev->commadpt->rto=3000;        /* Read Time-Out in milis */
    dev->commadpt->pto=3000;        /* Poll Time-out in milis */
    dev->commadpt->eto=10000;       /* Enable Time-out in milis */
    dev->commadpt->kaidle = sysblk.kaidle;
    dev->commadpt->kaintv = sysblk.kaintv;
    dev->commadpt->kacnt  = sysblk.kacnt;
    dev->commadpt->lnctl=COMMADPT_LNCTL_BSC;
    dev->commadpt->term=COMMADPT_TERM_TTY;
    dev->commadpt->uctrans=FALSE;
    dev->commadpt->code_table_toebcdic   = xlate_table_ebcd_toebcdic;
    dev->commadpt->code_table_fromebcdic = xlate_table_ebcd_fromebcdic;
    memset(dev->commadpt->byte_skip_table, 0, sizeof(dev->commadpt->byte_skip_table) );
    memset(dev->commadpt->input_byte_skip_table, 0, sizeof(dev->commadpt->input_byte_skip_table) );
    dev->commadpt->dumb_bs=0;
    dev->commadpt->dumb_break=0;
    dev->commadpt->prepend_length = 0;
    dev->commadpt->append_length = 0;
    dev->commadpt->rxvt4apl = 0;
    dev->commadpt->overstrike_flag = 0;
    dev->commadpt->crlf_opt = 0;
    dev->commadpt->sendcr_opt = 0;
    dev->commadpt->binary_opt = 0;
    dev->commadpt->crlf2cr_opt = 0;
    dev->commadpt->eol_char = 0x0d;   // default is ascii CR
    memset(dev->commadpt->prepend_bytes, 0, sizeof(dev->commadpt->prepend_bytes));
    memset(dev->commadpt->append_bytes, 0, sizeof(dev->commadpt->append_bytes));
    etospec=0;

    for(i=0;i<argc;i++)
    {
        pc=parser(ptab,argv[i],&res);
        if(pc<0)
        {
            WRMSG(HHC01012, "E",LCSS_DEVNUM,argv[i]);
            errcnt++;
            continue;
        }
        if(pc==0)
        {
            WRMSG(HHC01012, "E",LCSS_DEVNUM,argv[i]);
            errcnt++;
            continue;
        }
        switch(pc)
        {
            case COMMADPT_KW_LPORT:
                rc=commadpt_getport(res.text);
                if(rc<0)
                {
                    errcnt++;
                    msg01007e(dev,"LPORT",res.text);
                    break;
                }
                dev->commadpt->lport=rc;
                break;
            case COMMADPT_KW_LHOST:
                if(strcmp(res.text,"*")==0)
                {
                    dev->commadpt->lhost=INADDR_ANY;
                    break;
                }
                rc=commadpt_getaddr(&dev->commadpt->lhost,res.text);
                if(rc!=0)
                {
                    msg01007e(dev,"LHOST",res.text);
                    errcnt++;
                }
                break;
            case COMMADPT_KW_RPORT:
                rc=commadpt_getport(res.text);
                if(rc<0)
                {
                    errcnt++;
                    msg01007e(dev,"RPORT",res.text);
                    break;
                }
                dev->commadpt->rport=rc;
                break;
            case COMMADPT_KW_RHOST:
                if(strcmp(res.text,"*")==0)
                {
                    dev->commadpt->rhost=INADDR_NONE;
                    break;
                }
                rc=commadpt_getaddr(&dev->commadpt->rhost,res.text);
                if(rc!=0)
                {
                    msg01007e(dev,"RHOST",res.text);
                    errcnt++;
                }
                break;
            case COMMADPT_KW_READTO:
                dev->commadpt->rto=atoi(res.text);
                break;
            case COMMADPT_KW_POLLTO:
                dev->commadpt->pto=atoi(res.text);
                break;
            case COMMADPT_KW_ENABLETO:
                dev->commadpt->eto=atoi(res.text);
                etospec=1;
                break;
            case COMMADPT_KW_LNCTL:
                if(strcasecmp(res.text,"tele2")==0
                || strcasecmp(res.text,"ibm1")==0 )
                {
                    dev->commadpt->lnctl = COMMADPT_LNCTL_ASYNC;
                    dev->commadpt->rto=28000;        /* Read Time-Out in milis */
                }
                else if(strcasecmp(res.text,"bsc")==0)
                {
                    dev->commadpt->lnctl = COMMADPT_LNCTL_BSC;
                }
                else
                {
                    msg01007e(dev,"LNCTL",res.text);
                }
                break;
            case COMMADPT_KW_TERM:
                if(strcasecmp(res.text,"tty")==0)
                {
                    dev->commadpt->term = COMMADPT_TERM_TTY;
                }
                else if(strcasecmp(res.text,"2741")==0)
                {
                    dev->commadpt->term = COMMADPT_TERM_2741;
                }
                else if(strcasecmp(res.text,"rxvt4apl")==0)
                {
                    dev->commadpt->term = COMMADPT_TERM_2741;
                    dev->commadpt->rxvt4apl = 1;
                }
                else
                {
                    msg01007e(dev,"TERM",res.text);
                }
                break;
            case COMMADPT_KW_CODE:
                if(strcasecmp(res.text,"corr")==0)
                {
                    dev->commadpt->code_table_toebcdic   = xlate_table_cc_toebcdic;
                    dev->commadpt->code_table_fromebcdic = xlate_table_cc_fromebcdic;
                }
                else if(strcasecmp(res.text,"ebcd")==0)
                {
                    dev->commadpt->code_table_toebcdic   = xlate_table_ebcd_toebcdic;
                    dev->commadpt->code_table_fromebcdic = xlate_table_ebcd_fromebcdic;
                }
                else if(strcasecmp(res.text,"none")==0)
                {
                    dev->commadpt->code_table_toebcdic   = NULL;
                    dev->commadpt->code_table_fromebcdic = NULL;
                }
                else
                {
                    msg01007e(dev,"CODE",res.text);
                }
                break;
            case COMMADPT_KW_CRLF:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->crlf_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->crlf_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"CRLF",res.text);
                }
                break;
            case COMMADPT_KW_SENDCR:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->sendcr_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->sendcr_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"SENDCR",res.text);
                }
                break;
            case COMMADPT_KW_BINARY:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->binary_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->binary_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"BINARY",res.text);
                }
                break;
            case COMMADPT_KW_UCTRANS:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->uctrans = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->uctrans = TRUE;
                }
                else
                {
                    msg01007e(dev,"UCTRANS",res.text);
                }
                break;
            case COMMADPT_KW_EOL:
                if  (strlen(res.text) < 2)
                    break;
                bf[0] = res.text[0];
                bf[1] = res.text[1];
                bf[2] = 0;
                sscanf(bf, "%x", &ix);
                dev->commadpt->eol_char = ix;
                break;
            case COMMADPT_KW_SKIP:
                if  (strlen(res.text) < 2)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->byte_skip_table[ix] = 1;
                }
                break;
            case COMMADPT_KW_PREPEND:
                if  (strlen(res.text) != 2 && strlen(res.text) != 4
                  && strlen(res.text) != 6 && strlen(res.text) != 8)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->prepend_bytes[j>>1] = ix;
                }
                dev->commadpt->prepend_length = strlen(res.text) >> 1;
                break;
            case COMMADPT_KW_APPEND:
                if  (strlen(res.text) != 2 && strlen(res.text) != 4
                  && strlen(res.text) != 6 && strlen(res.text) != 8)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->append_bytes[j>>1] = ix;
                }
                dev->commadpt->append_length = strlen(res.text) >> 1;
                break;
            case COMMADPT_KW_ISKIP:
                if  (strlen(res.text) < 2)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->input_byte_skip_table[ix] = 1;
                }
                break;
            case COMMADPT_KW_BS:
                if(strcasecmp(res.text,"dumb")==0) {
                    dev->commadpt->dumb_bs = 1;
                }
                break;
            case COMMADPT_KW_BREAK:
                if(strcasecmp(res.text,"dumb")==0)
                    dev->commadpt->dumb_break = 1;
                break;
            case COMMADPT_KW_SWITCHED:
            case COMMADPT_KW_DIAL:
                if(strcasecmp(res.text,"yes")==0 || strcmp(res.text,"1")==0 || strcasecmp(res.text,"inout")==0)
                {
                    dev->commadpt->dialin=1;
                    dev->commadpt->dialout=1;
                    break;
                }
                if(strcasecmp(res.text,"no")==0 || strcmp(res.text,"0")==0)
                {
                    dev->commadpt->dialin=0;
                    dev->commadpt->dialout=0;
                    break;
                }
                if(strcasecmp(res.text,"in")==0)
                {
                    dev->commadpt->dialin=1;
                    dev->commadpt->dialout=0;
                    break;
                }
                if(strcasecmp(res.text,"out")==0)
                {
                    dev->commadpt->dialin=0;
                    dev->commadpt->dialout=1;
                    break;
                }
                WRMSG(HHC01013, "E",LCSS_DEVNUM,res.text);
                dev->commadpt->dialin=0;
                dev->commadpt->dialout=0;
                break;
            case COMMADPT_KW_KA:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->kaidle=0;
                    dev->commadpt->kaintv=0;
                    dev->commadpt->kacnt=0;
                }
                else
                {
                    int idle=-1,intv=-1,cnt=-1;
                    if (parse_conkpalv(res.text,&idle,&intv,&cnt)==0
                        || (idle==0 && intv==0 && cnt==0))
                    {
                        dev->commadpt->kaidle = idle;
                        dev->commadpt->kaintv = intv;
                        dev->commadpt->kacnt  = cnt;
                    }
                    else
                    {
                        msg01007e(dev,"KA",res.text);
                        errcnt++;
                    }
                }
                break;
            case COMMADPT_KW_CRLF2CR:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->crlf2cr_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->crlf2cr_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"CRLF2CR",res.text);
                }
                break;
            default:
                break;
        }
    }
    /*
     * Check parameters consistency
     * when DIAL=NO :
     *     lport must not be 0
     *     lhost may be anything
     *     rport must not be 0
     *     rhost must not be INADDR_NONE
     * when DIAL=IN or DIAL=INOUT
     *     lport must NOT be 0
     *     lhost may be anything
     *     rport MUST be 0
     *     rhost MUST be INADDR_NONE
     * when DIAL=OUT
     *     lport MUST be 0
     *     lhost MUST be INADDR_ANY
     *     rport MUST be 0
     *     rhost MUST be INADDR_NONE
    */
    switch(dev->commadpt->dialin+dev->commadpt->dialout*2)
    {
        case 0:
            dialt="NO";
            break;
        case 1:
            dialt="IN";
            break;
        case 2:
            dialt="OUT";
            break;
        case 3:
            dialt="INOUT";
            break;
        default:
            dialt="*ERR*";
            break;
    }
    switch(dev->commadpt->dialin+dev->commadpt->dialout*2)
    {
        case 0: /* DIAL = NO */
            dev->commadpt->eto=0;
            if(dev->commadpt->lport==0)
            {
                msg01008e(dev,dialt,"LPORT");
                errcnt++;
            }
            if(dev->commadpt->rport==0)
            {
                msg01008e(dev,dialt,"RPORT");
                errcnt++;
            }
            if(dev->commadpt->rhost==INADDR_NONE)
            {
                msg01008e(dev,dialt,"RHOST");
                errcnt++;
            }
            if(etospec)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->eto);
                msg01009w(dev,dialt,"ETO",fmtbfr);
                errcnt++;
            }
            dev->commadpt->eto=0;
            break;
        case 1: /* DIAL = IN */
        case 3: /* DIAL = INOUT */
            if(dev->commadpt->lport==0)
            {
                msg01008e(dev,dialt,"LPORT");
                errcnt++;
            }
            if(dev->commadpt->rport!=0)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->rport);
                msg01009w(dev,dialt,"RPORT",fmtbfr);
            }
            if(dev->commadpt->rhost!=INADDR_NONE)
            {
                in_temp.s_addr=dev->commadpt->rhost;
                msg01009w(dev,dialt,"RHOST",inet_ntoa(in_temp));
                dev->commadpt->rhost=INADDR_NONE;
            }
            break;
        case 2: /* DIAL = OUT */
            if(dev->commadpt->lport!=0)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->lport);
                msg01009w(dev,dialt,"LPORT",fmtbfr);
                dev->commadpt->lport=0;
            }
            if(dev->commadpt->rport!=0)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->rport);
                msg01009w(dev,dialt,"RPORT",fmtbfr);
                dev->commadpt->rport=0;
            }
            if(dev->commadpt->lhost!=INADDR_ANY)    /* Actually it's more like INADDR_NONE */
            {
                in_temp.s_addr=dev->commadpt->lhost;
                msg01009w(dev,dialt,"LHOST",inet_ntoa(in_temp));
                dev->commadpt->lhost=INADDR_ANY;
            }
            if(dev->commadpt->rhost!=INADDR_NONE)
            {
                in_temp.s_addr=dev->commadpt->rhost;
                msg01009w(dev,dialt,"RHOST",inet_ntoa(in_temp));
                dev->commadpt->rhost=INADDR_NONE;
            }
            break;
    }
    if(errcnt>0)
    {
        WRMSG(HHC01014, "I",LCSS_DEVNUM);
        return -1;
    }
    in_temp.s_addr=dev->commadpt->lhost;
    in_temp.s_addr=dev->commadpt->rhost;
    dev->bufsize=256;
    dev->numsense=2;
    memset(dev->sense, 0, sizeof(dev->sense));

    /* Initialise various flags & statuses */
    dev->commadpt->enabled=0;
    dev->commadpt->connect=0;
    dev->fd=100;    /* Ensures 'close' function called */
    dev->commadpt->devnum=dev->devnum;

    dev->commadpt->telnet_opt=0;
    dev->commadpt->telnet_iac=0;
    dev->commadpt->telnet_int=0;
    dev->commadpt->eol_flag=0;
    dev->commadpt->telnet_cmd=0;

    dev->commadpt->haltpending=0;
    dev->commadpt->haltprepare=0;

    /* Initialize the device identifier bytes */
    dev->numdevid = sysblk.legacysenseid ? 7 : 0;
    dev->devid[0] = 0xFF;
    dev->devid[1] = dev->devtype >> 8;
    dev->devid[2] = dev->devtype & 0xFF;
    dev->devid[3] = 0x00;
    dev->devid[4] = dev->devtype >> 8;
    dev->devid[5] = dev->devtype & 0xFF;
    dev->devid[6] = 0x00;

    /* Initialize the CA lock */
    initialize_lock(&dev->commadpt->lock);

    /* Initialise thread->I/O & halt initiation EVB */
    initialize_condition(&dev->commadpt->ipc);
    initialize_condition(&dev->commadpt->ipc_halt);

    /* Allocate I/O -> Thread signaling pipe */
    VERIFY(!create_pipe(dev->commadpt->pipe));

    /* Obtain the CA lock */
    obtain_lock(&dev->commadpt->lock);

    /* Indicate listen required if DIAL!=OUT */
    if(dev->commadpt->dialin ||
            (!dev->commadpt->dialin && !dev->commadpt->dialout))
    {
        dev->commadpt->dolisten=1;
    }
    else
    {
        dev->commadpt->dolisten=0;
    }

    /* Start the async worker thread */

    /* Set thread-name for debugging purposes */
    MSGBUF(thread_name, "commadpt %4.4X thread", dev->devnum);
    thread_name[sizeof(thread_name)-1]=0;

    dev->commadpt->curpending=COMMADPT_PEND_TINIT;
    rc = create_thread(&dev->commadpt->cthread,DETACHED,commadpt_thread,dev->commadpt,thread_name);
    if(rc)
    {
        WRMSG(HHC00102, "E", strerror(rc));
        release_lock(&dev->commadpt->lock);
        return -1;
    }
    commadpt_wait(dev);
    if(dev->commadpt->curpending!=COMMADPT_PEND_IDLE)
    {
        WRMSG(HHC01015, "E",LCSS_DEVNUM);
        /* Release the CA lock */
        release_lock(&dev->commadpt->lock);
        return -1;
    }
    dev->commadpt->have_cthread=1;

    /* Release the CA lock */
    release_lock(&dev->commadpt->lock);
    /* Indicate succesfull completion */
    return 0;
}

static char *commadpt_lnctl_names[]={
    "NONE",
    "BSC",
    "ASYNC"
};

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void commadpt_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    char filename[ PATH_MAX + 1 ];      /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "LINE", dev, devclass, buflen, buffer );

    snprintf(buffer,buflen,"%s STA=%s CN=%s, EIB=%s OP=%s IO[%"PRIu64"]",
            commadpt_lnctl_names[dev->commadpt->lnctl],
            dev->commadpt->enabled?"ENA":"DISA",
            dev->commadpt->connect?"YES":"NO",
            dev->commadpt->eibmode?"YES":"NO",
            commadpt_pendccw_text[dev->commadpt->curpending],
            dev->excps );
}

/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/* Invoked by HERCULES shutdown & DEVINIT processing                 */
/*-------------------------------------------------------------------*/
static int commadpt_close_device( DEVBLK* dev )
{
    if (dev->ccwtrace)
    {
        // "%1d:%04X COMM: closing down"
        WRMSG( HHC01060, "D", LCSS_DEVNUM );
    }

    /* Terminate current I/O thread if necessary */
    if (dev->busy)
        commadpt_halt_or_clear( dev );

    /* Obtain the CA lock */
    obtain_lock( &dev->commadpt->lock );

    /* Terminate worker thread if it is still up */
    if (dev->commadpt->have_cthread)
    {
        dev->commadpt->curpending   = COMMADPT_PEND_SHUTDOWN;

        commadpt_wakeup( dev->commadpt, 0 );
        commadpt_wait( dev );

        dev->commadpt->cthread      = (TID) -1;
        dev->commadpt->have_cthread = 0;
    }


    /* Free all work storage */
    /* The CA lock will be released by the cleanup routine */
    commadpt_clean_device( dev );  // also does "release_lock( &dev->commadpt->lock );"

    /* Indicate to hercules the device is no longer opened */
    dev->fd = -1;

    if (dev->ccwtrace)
    {
        // "%1d:%04X COMM: closed down"
        WRMSG( HHC01061, "D", LCSS_DEVNUM );
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
static void commadpt_execute_ccw (DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual)
{
U32 num;                        /* Work : Actual CCW transfer count                   */
BYTE    b;                      /* Input processing work variable : Current character */
BYTE    setux;                  /* EOT kludge */
BYTE    turnxpar;               /* Write contains turn to transparent mode */
int     i;                      /* work */
u_int   j;                      /* work */
BYTE    gotdle;                 /* Write routine DLE marker */
BYTE    b1, b2;                 /* 2741 overstrike rewriting */
    UNREFERENCED(flags);
    UNREFERENCED(chained);
    UNREFERENCED(prevcode);
    UNREFERENCED(ccwseq);

    *residual = 0;
    /*
     * Obtain the COMMADPT lock
     */
    if(dev->ccwtrace)
    {
        WRMSG(HHC01063,"D",LCSS_DEVNUM,code);
    }
    obtain_lock(&dev->commadpt->lock);
    if(code != 0x06) /* for any command other than PREPARE */
    {
        dev->commadpt->haltprepare = 0;
    }
    switch (code)
    {
        /*---------------------------------------------------------------*/
        /* CONTROL NO-OP                                                 */
        /*---------------------------------------------------------------*/
        case 0x03:
            *residual=0;
            *unitstat=CSW_CE|CSW_DE;
            break;

        /*---------------------------------------------------------------*/
        /* BASIC SENSE                                                   */
        /*---------------------------------------------------------------*/
        case 0x04:
            num=count<dev->numsense?count:dev->numsense;
            *more=count<dev->numsense?1:0;
            memcpy(iobuf,dev->sense,num);
            *residual=count-num;
            *unitstat=CSW_CE|CSW_DE;
            break;

        /*---------------------------------------------------------------*/
        /* SENSE ID                                                      */
        /*---------------------------------------------------------------*/
        case 0xE4:
            /* Calculate residual byte count */
            num = (count < dev->numdevid) ? count : dev->numdevid;
            *residual = count - num;
            *more = count < dev->numdevid ? 1 : 0;

            /* Copy device identifier bytes to channel I/O Buffer */
            memcpy (iobuf, dev->devid, num);

            /* Return unit status */
            *unitstat = CSW_CE | CSW_DE;
            break;

        /*---------------------------------------------------------------*/
        /* ENABLE                                                        */
        /*---------------------------------------------------------------*/
        case 0x27:
            if(dev->commadpt->dialin+dev->commadpt->dialout*2==2)
            {
                /* Enable makes no sense on a dial out only line */
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_IR;
                dev->sense[1]=0x2E; /* Simulate Failed Call In */
                break;
            }
            if(dev->commadpt->connect)
            {
                /* Already connected */
                dev->commadpt->enabled=1;
                *unitstat=CSW_CE|CSW_DE;
                break;
            }
            dev->commadpt->curpending=COMMADPT_PEND_ENABLE;
            commadpt_wakeup(dev->commadpt,0);
            commadpt_wait(dev);
            /* Is the device still there? If not, for example when
               Hercules is shutting down, continuing here will cause a segfault */
            if (!dev) break;
            if (!dev->commadpt) break;
            /* If the line is not connected now, then ENABLE failed */
            if(dev->commadpt->connect)
            {
                *unitstat=CSW_CE|CSW_DE;
                dev->commadpt->enabled=1;
                /* Clean the input buffer */
                commadpt_ring_flush(&dev->commadpt->inbfr);
                break;
            }
            if(dev->commadpt->haltpending)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UX;
                dev->commadpt->haltpending=0;
                break;
            }
            if(dev->commadpt->dialin)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_IR;
                dev->sense[1]=0x2e;
            }
            else
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_IR;
                dev->sense[1]=0x21;
            }
            break;

        /*---------------------------------------------------------------*/
        /* DISABLE                                                       */
        /*---------------------------------------------------------------*/
        case 0x2F:
            /* Reset some flags */
            dev->commadpt->xparwwait=0;
            commadpt_ring_flush(&dev->commadpt->inbfr);      /* Flush buffers */
            commadpt_ring_flush(&dev->commadpt->outbfr);      /* Flush buffers */
            commadpt_ring_flush(&dev->commadpt->ttybuf);      /* Flush buffers */

            if((!dev->commadpt->dialin && !dev->commadpt->dialout) || !dev->commadpt->connect)
            {
                *unitstat=CSW_CE|CSW_DE;
                dev->commadpt->enabled=0;
                break;
            }
            dev->commadpt->curpending=COMMADPT_PEND_DISABLE;
            commadpt_wakeup(dev->commadpt,0);
            commadpt_wait(dev);
            dev->commadpt->enabled=0;
            *unitstat=CSW_CE|CSW_DE;
            break;
        /*---------------------------------------------------------------*/
        /* SET MODE                                                      */
        /*---------------------------------------------------------------*/
        case 0x23:
            /* Transparent Write Wait State test */
            if(dev->commadpt->xparwwait)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                return;
            }
            num=1;
            *residual=count-num;
            *unitstat=CSW_CE|CSW_DE;
            if(dev->ccwtrace)
            {
                WRMSG(HHC01084,"D",LCSS_DEVNUM,iobuf[0]&0x40 ? "EIB":"NO EIB");
            }
            dev->commadpt->eibmode=(iobuf[0]&0x40)?1:0;
            break;
        /*---------------------------------------------------------------*/
        /* POLL Command                                                  */
        /*---------------------------------------------------------------*/
        case 0x09:
            /* Transparent Write Wait State test */
            if(dev->commadpt->xparwwait)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                return;
            }
            /* Save POLL data */
            commadpt_ring_flush(&dev->commadpt->pollbfr);
            commadpt_ring_pushbfr(&dev->commadpt->pollbfr,iobuf,count);
            /* Set some utility variables */
            dev->commadpt->pollused=0;
            dev->commadpt->badpoll=0;
            /* Tell thread */
            dev->commadpt->curpending=COMMADPT_PEND_POLL;
            commadpt_wakeup(dev->commadpt,0);
            commadpt_wait(dev);
            /* Flush the output & poll rings */
            commadpt_ring_flush(&dev->commadpt->outbfr);
            commadpt_ring_flush(&dev->commadpt->pollbfr);
            /* Check for HALT */
            if(dev->commadpt->haltpending)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UX;
                dev->commadpt->haltpending=0;
                break;
            }
            /* Check for bad poll data */
            if(dev->commadpt->badpoll)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=0x08;
                dev->sense[1]=0x84;
                break;
            }
            /* Determine remaining length */
            *residual=count-dev->commadpt->pollused;
            /* Determine if SM should be set (succesfull or unsucessfull POLLs) */
            /* exhausting poll data when all stations reported NO data          */
            /* does not set Status Modifier                                     */
            *unitstat=CSW_CE|CSW_DE|(dev->commadpt->pollsm?CSW_SM:0);
            /* NOTE : The index byte (and rest) are in the Input Ring */
            break;

        /*---------------------------------------------------------------*/
        /* DIAL                                                          */
        /* Info on DIAL DATA :                                           */
        /* Dial character formats :                                      */
        /*                        x x x x 0 0 0 0 : 0                    */
        /*                            ........                           */
        /*                        x x x x 1 0 0 1 : 9                    */
        /*                        x x x x 1 1 0 0 : SEP                  */
        /*                        x x x x 1 1 0 1 : EON                  */
        /* EON is ignored                                                */
        /* format is : AAA/SEP/BBB/SEP/CCC/SEP/DDD/SEP/PPPP              */
        /*          where A,B,C,D,P are numbers from 0 to 9              */
        /* This perfoms an outgoing call to AAA.BBB.CCC.DDD port PPPP    */
        /*---------------------------------------------------------------*/
        case 0x29:
            /* The line must have dial-out capability */
            if(!dev->commadpt->dialout)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                dev->sense[1]=0x04;
                break;
            }
            /* The line must be disabled */
            if(dev->commadpt->enabled)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                dev->sense[1]=0x05;
                break;
            }
            num=count>sizeof(dev->commadpt->dialdata) ? sizeof(dev->commadpt->dialdata) : count;
            memcpy(dev->commadpt->dialdata,iobuf,num);
            dev->commadpt->curpending=COMMADPT_PEND_DIAL;
            commadpt_wakeup(dev->commadpt,0);
            commadpt_wait(dev);
            *residual=count-num;
            if(dev->commadpt->haltpending)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UX;
                dev->commadpt->haltpending=0;
                break;
            }
            if(!dev->commadpt->connect)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_IR;
                dev->commadpt->enabled=0;
            }
            else
            {
                *unitstat=CSW_CE|CSW_DE;
                dev->commadpt->enabled=1;
            }
            break;

        /*---------------------------------------------------------------*/
        /* READ                                                          */
        /*---------------------------------------------------------------*/
        case 0x02:
        case 0x0a:          /* also INHIBIT */
            setux=0;
            /* Check the line is enabled */
            if(!dev->commadpt->enabled)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                dev->sense[1]=0x06;
                break;
            }
            /* Transparent Write Wait State test */
            if(dev->commadpt->xparwwait)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                break;
            }
            /* Check for any remaining data in read work buffer */
            /* for async, we allow all reads to wait (even if data is available now) */
            /* (APL\360 2741 race cond III circumvention) see APLSASUP label UNRZ19 */
            if(dev->commadpt->readcomp && IS_BSC_LNCTL(dev->commadpt))
            {
                if (dev->commadpt->rdwrk.havedata)
                {
                    num=(U32)commadpt_ring_popbfr(&dev->commadpt->rdwrk,iobuf,count);
                    if(dev->commadpt->rdwrk.havedata)
                    {
                        *more=1;
                    }
                    *residual=count-num;
                    *unitstat=CSW_CE|CSW_DE;
                    break;
                }
            }
            if(IS_ASYNC_LNCTL(dev->commadpt) && dev->commadpt->telnet_int)
            {
                dev->commadpt->telnet_int = 0;
                *residual=count;
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_IR;
                break;
            }
            /* Catch a race condition.                                                           */
            /* TCAM likes to issue halt I/O as a matter of routine, and it expects to get back a */
            /* unit exception along with the normal channel end + device end.                    */
            /* Sometimes the halt I/O loses the race (with the write CCW) and we catch up here.  */
            if(IS_ASYNC_LNCTL(dev->commadpt) && dev->commadpt->haltpending)
            {
                dev->commadpt->haltpending = 0;
                *residual=0;
                *unitstat=CSW_CE|CSW_DE|CSW_UX;
                break;
            }

            if(dev->commadpt->datalostcond)
            {
                dev->commadpt->datalostcond=0;
                commadpt_ring_flush(&dev->commadpt->inbfr);
                *residual=count;
                *unitstat=CSW_CE|CSW_DE;
                break;
            }
            dev->commadpt->readcomp=0;
            *unitstat=0;
            num=0;
                /* The following is the BIG READ ROUTINE MESS */
                /* the manual's indications on when to exit   */
                /* a read and what to transfer to the main    */
                /* storage is fuzzy (at best)                 */
                /*                                            */
                /* The line input can be in 3 possible        */
                /* conditions :                               */
                /*     Transparent Text Mode                  */
                /*     Text Mode                              */
                /*     none of the above (initial status)     */
                /* transition from one mode to the other is   */
                /* also not very well documented              */
                /* so the following code is based on          */
                /* empirical knowledge and some interpretation*/
                /* also... the logic should probably be       */
                /* rewritten                                  */

                /* We will remain in READ state with the thread */
                /* as long as we haven't met a read ending condition */
            while(1)
            {
                /* READ state */
                dev->commadpt->curpending=COMMADPT_PEND_READ;
                /* Tell worker thread */
                commadpt_wakeup(dev->commadpt,0);
                /* Wait for some data */
                commadpt_wait(dev);

                /* If we are not connected, the read fails */
                if(!dev->commadpt->connect)
                {
                    *unitstat=CSW_DE|CSW_CE|CSW_UC;
                    dev->sense[0]=SENSE_IR;
                    break;
                }

                /* If the I/O was halted - indicate Unit Check */
                if(dev->commadpt->haltpending)
                {
                    *unitstat=CSW_CE|CSW_DE|CSW_UX;
                    dev->commadpt->haltpending=0;
                    break;
                }

                if (IS_ASYNC_LNCTL(dev->commadpt) && dev->commadpt->telnet_int)
                {
                    dev->commadpt->telnet_int = 0;
                    *residual=count;
                    *unitstat=CSW_CE|CSW_DE|CSW_UC;
                    dev->sense[0]=SENSE_IR;
                    break;
                }

                /* If no data is present - 3 seconds have passed without */
                /* receiving data (or a SYNC)                            */
                /* (28 seconds for LNCTL_ASYNC)                          */
                /* INHIBIT command does not time out                     */
                /* eol_flag set means data is present */
                if(!dev->commadpt->inbfr.havedata && code != 0x0a && !dev->commadpt->eol_flag)
                {
                    *unitstat=CSW_DE|CSW_CE|CSW_UC;
                    dev->sense[0]=0x01;
                    dev->sense[1]=0xe3;
                    break;
                }
                if (IS_BSC_LNCTL(dev->commadpt))
                {
                    /* Start processing data flow here */
                    /* Pop bytes until we run out of data or */
                    /* until the processing indicates the read */
                    /* should now terminate */
                    while( dev->commadpt->inbfr.havedata
                            && !dev->commadpt->readcomp)
                    {
                        /* fetch 1 byte from the input ring */
                        b=commadpt_ring_pop(&dev->commadpt->inbfr);
                        if(!dev->commadpt->gotdle)
                        {
                            if(b==0x10)
                            {
                                dev->commadpt->gotdle=1;
                                continue;
                            }
                        }
                        if(dev->commadpt->in_textmode)
                        {
                            if(dev->commadpt->in_xparmode)
                            {
                                /* TRANSPARENT MODE READ */
                                if(dev->commadpt->gotdle)
                                {
                                    switch(b)
                                    {
                                        case 0x10:
                                            commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                            break;
                                        case 0x32:
                                            break;
                                        case 0x1F: /* ITB - Exit xparent, set EIB - do NOT exit read yet */
                                            dev->commadpt->in_xparmode=0;
                                            commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                            commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                            if(dev->commadpt->eibmode)
                                            {
                                                commadpt_ring_push(&dev->commadpt->rdwrk,0);
                                            }
                                            break;
                                        case 0x26: /* ETB - Same as ITB but DO exit read now */
                                            dev->commadpt->in_xparmode=0;
                                            commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                            commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                            if(dev->commadpt->eibmode)
                                            {
                                                commadpt_ring_push(&dev->commadpt->rdwrk,0);
                                            }
                                            dev->commadpt->readcomp=1;
                                            break;
                                        case 0x03: /* ETX - Same as ETB */
                                            dev->commadpt->in_xparmode=0;
                                            commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                            commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                            if(dev->commadpt->eibmode)
                                            {
                                                commadpt_ring_push(&dev->commadpt->rdwrk,0);
                                            }
                                            dev->commadpt->readcomp=1;
                                            break;
                                        case 0x2D: /* ENQ */
                                            dev->commadpt->in_xparmode=0;
                                            dev->commadpt->in_textmode=0;
                                            commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                            commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                            dev->commadpt->readcomp=1;
                                            break;
                                        default:
                                            commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                            commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                            break;
                                    }
                                }
                                else
                                {
                                    commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                }
                            }
                            else
                            {
                                if(b!=0x32)
                                {
                                     /* TEXT MODE READ */
                                    if(dev->commadpt->gotdle)
                                    {
                                        switch(b)
                                        {
                                            case 0x02: /* STX */
                                            dev->commadpt->in_xparmode=1;
                                            break;
                                            case 0x2D: /* ENQ */
                                            dev->commadpt->readcomp=1;
                                            break;
                                            default:
                                                if((b&0xf0)==0x60 || (b&0xf0)==0x70)
                                                {
                                                    dev->commadpt->readcomp=1;
                                                }
                                                break;
                                        }
                                        commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                        commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                    }
                                    else
                                    {
                                        switch(b)
                                        {
                                            case 0x2D:      /* ENQ */
                                                dev->commadpt->readcomp=1;
                                                dev->commadpt->in_textmode=0;
                                                commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                                break;
                                            case 0x3D:      /* NAK */
                                                dev->commadpt->readcomp=1;
                                                commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                                break;
                                            case 0x26:      /* ETB */
                                            case 0x03:      /* ETX */
                                                dev->commadpt->readcomp=1;
                                                dev->commadpt->in_textmode=0;
                                                commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                                if(dev->commadpt->eibmode)
                                                {
                                                    commadpt_ring_push(&dev->commadpt->rdwrk,0);
                                                }
                                                break;
                                            case 0x1F:      /* ITB */
                                                commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                                if(dev->commadpt->eibmode)
                                                {
                                                    commadpt_ring_push(&dev->commadpt->rdwrk,0);
                                                }
                                                break;
                                            default:
                                                commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                                break;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if(b!=0x32)
                            {
                                if(dev->commadpt->gotdle)
                                {
                                    if((b & 0xf0) == 0x60 || (b&0xf0)==0x70)
                                    {
                                        commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                        commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                        dev->commadpt->readcomp=1;
                                    }
                                    else
                                    {
                                        if(b==0x02)
                                        {
                                            commadpt_ring_push(&dev->commadpt->rdwrk,0x10);
                                            commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                            dev->commadpt->in_textmode=1;
                                            dev->commadpt->in_xparmode=1;
                                        }
                                    }
                                }
                                else
                                {
                                    switch(b)
                                    {
                                        case 0x37:  /* EOT */
                                            setux=1;
                                            dev->commadpt->readcomp=1;
                                            break;
                                        case 0x01:
                                        case 0x02:
                                            dev->commadpt->in_textmode=1;
                                            break;
                                        case 0x2D: /* ENQ */
                                            dev->commadpt->readcomp=1;
                                            break;
                                        case 0x3D: /* NAK */
                                            dev->commadpt->readcomp=1;
                                            break;
                                        default:
                                            break;
                                    }
                                    commadpt_ring_push(&dev->commadpt->rdwrk,b);
                                }
                            }
                        }
                        dev->commadpt->gotdle=0;
                    } /* END WHILE - READ FROM DATA BUFFER */
                } /* end of if (bsc) */
                /* If readcomp is set, then we may exit the read loop */
                if(dev->commadpt->readcomp || dev->commadpt->eol_flag)
                {
                    if (dev->commadpt->rdwrk.havedata || dev->commadpt->eol_flag)
                    {
                        num=(U32)commadpt_ring_popbfr(&dev->commadpt->rdwrk,iobuf,count);
                        if(dev->commadpt->rdwrk.havedata)
                        {
                            *more=1;
                        }
                        *residual=count-num;
                        *unitstat=CSW_CE|CSW_DE|(setux?CSW_UX:0);
                        logdump("Read",dev,iobuf,num);
                        if(IS_ASYNC_LNCTL(dev->commadpt)&& !dev->commadpt->rdwrk.havedata && *residual > 0)
                            dev->commadpt->eol_flag = 0;
                        break;
                    }
                }
            } /* END WHILE - READ FROM THREAD */
            break;

        /*---------------------------------------------------------------*/
        /* WRITE                                                         */
        /*---------------------------------------------------------------*/
        case 0x01:
        case 0x0d:       /* also CCW=BREAK */
                logdump("Writ",dev,iobuf,count);
                *residual=count;

                /* Check if we have an opened path */
                if(!dev->commadpt->connect)
                {
                    *unitstat=CSW_CE|CSW_DE|CSW_UC;
                    dev->sense[0]=SENSE_IR;
                    break;
                }

                /* Check if the line has been enabled */
                if(!dev->commadpt->enabled)
                {
                    *unitstat=CSW_CE|CSW_DE|CSW_UC;
                    dev->sense[0]=SENSE_CR;
                    break;
                }

                dev->commadpt->haltpending = 0; /* circumvent APL\360 2741 race cond II */
                /* read 1 byte to check for pending input */
                i=read_socket(dev->commadpt->sfd,&b,1);
                if (IS_ASYNC_LNCTL(dev->commadpt))
                {
                    if(i>0)
                    {
                        logdump("RCV0",dev,&b,1);
                        commadpt_read_tty(dev->commadpt,&b,1);
                    }
                }
                else
                {
                    if(i>0)
                    {
                    /* Push it in the communication input buffer ring */
                        commadpt_ring_push(&dev->commadpt->inbfr,b);
                    }
                    /* Set UX on write if line has pending inbound data */
                    if(dev->commadpt->inbfr.havedata)
                    {
                        dev->commadpt->datalostcond=1;
                        *unitstat=CSW_CE|CSW_DE|CSW_UX;
                        break;
                    }
                }  /* end of else (async) */
                /*
                 * Fill in the Write Buffer
                 */

                /* To start : not transparent mode, no DLE received yet */
                turnxpar=0;
                gotdle=0;
                if(IS_ASYNC_LNCTL(dev->commadpt) && dev->commadpt->telnet_int
                   /* ugly hack for TSO ATTN to fix IEA000I 0C3,IOE,01,0E40,40008900002C,,,TCAM */
                   && !(iobuf[0] == 0xdf && iobuf[1] == 0xdf && iobuf[2] == 0xdf && count == 3))
                {
                       dev->commadpt->telnet_int = 0;
                       *residual=count;
                       *unitstat=CSW_CE|CSW_DE|CSW_UC;
                       dev->sense[0]=SENSE_IR;
                       break;
                }

                /* Scan the I/O buffer */
                for(i=0;(U32)i<count;i++)
                {
                    /* Get 1 byte */
                    b=iobuf[i];

                    if (IS_ASYNC_LNCTL(dev->commadpt))
                    {
                        if (dev->commadpt->byte_skip_table[b])
                        continue;
                        if (dev->commadpt->term == COMMADPT_TERM_TTY)
                        {
                            b = reverse_bits( b ) & 0x7f;
                        }
                        else
                        { /* 2741 */
                            if (count == 1 && b == CIRCLE_D)
                            {
                                b = 0x00; /* map initial Circle-D to NUL */
                            }
                            else if (dev->commadpt->rxvt4apl)
                            {
                                if (dev->commadpt->overstrike_flag == 1 && (b & 0x7f) == 0x5d)
                                { /* char is another backspace but overstrike was expected */
                                    dev->commadpt->overstrike_flag = 0;
                                    dev->commadpt->saved_char = b;
                                    b = rxvt4apl_from_2741[b];
                                }
                                else if (dev->commadpt->overstrike_flag == 1)
                                {
                                    dev->commadpt->overstrike_flag = 0;
                                    if (((u_int)dev->commadpt->saved_char) > ((u_int)b))
                                    {
                                        b1 = b;
                                        b2 = dev->commadpt->saved_char;
                                    }
                                    else
                                    {
                                        b1 = dev->commadpt->saved_char;
                                        b2 = b;
                                    }
                                    b = '?';
                                    for (j = 0; j < sizeof(overstrike_2741_pairs); j+=2) {
                                        if (overstrike_2741_pairs[j] == b1 && overstrike_2741_pairs[j+1] == b2) {
                                            b = overstrike_rxvt4apl_chars[j>>1];
                                        }
                                    }
                                }
                                else if ((b & 0x7f) == 0x5d /* 2741 backspace */
                                      && (dev->commadpt->saved_char & 0x7f) != 0x5d
                                      && (dev->commadpt->saved_char & 0x7f) != 0x3b
                                      && (dev->commadpt->saved_char & 0x7f) != 0x7f)
                                {
                                    dev->commadpt->overstrike_flag = 1;
                                    b = rxvt4apl_from_2741[b];
                                }
                                else
                                {
                                    dev->commadpt->overstrike_flag = 0;
                                    dev->commadpt->saved_char = b;
                                    b = rxvt4apl_from_2741[b];
                                    if (b == 0x0d && dev->commadpt->crlf_opt) /* ascii CR? */
                                    {   /* 2741 NL has been mapped to CR, we need to append LF to this (sigh) */
                                        commadpt_ring_push(&dev->commadpt->outbfr,b);
                                        b = 0x0a;
                                    }
                                }
                            }
                            else if (dev->commadpt->code_table_toebcdic)
                            {
                                b = dev->commadpt->code_table_toebcdic[b];  // first translate to EBCDIC
                                b = guest_to_host(b) & 0x7f; // then EBCDIC to ASCII
                            }
                        }
                    }
                    else
                    {   /* line is BSC */
                        /* If we are in transparent mode, we must double the DLEs */
                        if(turnxpar)
                        {
                            /* Check for a DLE */
                            if(b==0x10)
                            {
                                /* put another one in the output buffer */
                                commadpt_ring_push(&dev->commadpt->outbfr,0x10);
                            }
                        }
                        else        /* non transparent mode */
                        {
                            if(b==0x10)
                            {
                                gotdle=1;   /* Indicate we have a DLE for next pass */
                            }
                            else
                            {
                                /* If there was a DLE on previous pass */
                                if(gotdle)
                                {
                                    /* check for DLE/STX */
                                    if(b==0x02)
                                    {
                                        /* Indicate transparent mode on */
                                        turnxpar=1;
                                    }
                                   gotdle=0;
                                }
                                else
                                {
                                    if((b==0x03) || (b==0x26))
                                    {
                                        commadpt_ring_push(&dev->commadpt->outbfr,b);
                                        break;
                                    }
                                }
                            }
                        }
                    }  /* end of else (async) */
                    /* Put the current byte on the output ring */
                    commadpt_ring_push(&dev->commadpt->outbfr,b);
                }
            if (IS_BSC_LNCTL(dev->commadpt))
            {
                /* If we had a DLE/STX, the line is now in Transparent Write Wait state */
                /* meaning that no CCW codes except Write, No-Op, Sense are allowed     */
                /* (that's what the manual says.. I doubt DISABLE is disallowed)        */
                /* Anyway.. The program will have an opportunity to turn XPARENT mode   */
                /* off on the next CCW.                                                 */
                /* CAVEAT : The manual doesn't say if the line remains in transparent   */
                /*          Write Wait state if the next CCW doesn't start with DLE/ETX */
                /*          or DLE/ITB                                                  */
                if(turnxpar)
                {
                    dev->commadpt->xparwwait=1;
                }
                else
                {
                    dev->commadpt->xparwwait=0;
                }
            }  /* end of if(bsc line) */
            /* Indicate to the worker thread the current operation is OUTPUT */
            dev->commadpt->curpending=COMMADPT_PEND_WRITE;

            /* All bytes written out - residual = 0 */
            *residual=0;

            /* Wake-up the worker thread */
            commadpt_wakeup(dev->commadpt,0);

            /* Wait for operation completion */
            commadpt_wait(dev);

            /* Check if the line is still connected */
            if(!dev->commadpt->connect)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_IR;
                break;
            }

            /* Check if the I/O was interrupted */
            if(dev->commadpt->haltpending)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UX;
                dev->commadpt->haltpending = 0;
                break;
            }
            *unitstat=CSW_CE|CSW_DE;
            break;

        /*---------------------------------------------------------------*/
        /* PREPARE                                                       */
        /* NOTE : DO NOT SET RESIDUAL to 0 : Otherwise, channel.c        */
        /*        will reflect a channel prot check - residual           */
        /*        should indicate NO data was transfered for this        */
        /*        pseudo-read operation                                  */
        /*---------------------------------------------------------------*/
        case 0x06:
            *residual=count;
            /* PREPARE not allowed unless line is enabled */
            if(!dev->commadpt->enabled)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                dev->sense[1]=0x06;
                break;
            }

            if(IS_ASYNC_LNCTL(dev->commadpt) && dev->commadpt->haltprepare)
            {  /* circumvent APL\360 2741 race cond I */
                *unitstat=CSW_CE|CSW_DE|CSW_UX;
                break;
            } /* end of if(async) */

            if(IS_ASYNC_LNCTL(dev->commadpt) && dev->commadpt->telnet_int)
            {
                dev->commadpt->telnet_int = 0;
                *unitstat=CSW_CE|CSW_DE;
                if(dev->commadpt->haltpending)
                {
                    dev->commadpt->haltpending=0;
                    *unitstat |= CSW_UX;
                }
                break;
            } /* end of if(async) */

            /* Transparent Write Wait State test */
            if(dev->commadpt->xparwwait)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_CR;
                return;
            }

            /* If data is present, prepare ends immediatly */
            if(dev->commadpt->inbfr.havedata)
            {
                *unitstat=CSW_CE|CSW_DE;
                break;
            }

            /* Indicate to the worker thread to notify us when data arrives */
            dev->commadpt->curpending=COMMADPT_PEND_PREPARE;

            /* Wakeup worker thread */
            commadpt_wakeup(dev->commadpt,0);

            /* Wait for completion */
            commadpt_wait(dev);

            /* If I/O was halted (this one happens often) */
            if(dev->commadpt->haltpending)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UX;
                dev->commadpt->haltpending=0;
                break;
            }

            /* Check if the line is still connected */
            if(!dev->commadpt->connect)
            {
                *unitstat=CSW_CE|CSW_DE|CSW_UC;
                dev->sense[0]=SENSE_IR;
                break;
            }

            /* Normal Prepare exit condition - data is present in the input buffer */
            *unitstat=CSW_CE|CSW_DE;
            dev->commadpt->telnet_int = 0;
            break;

        default:
        /*---------------------------------------------------------------*/
        /* INVALID OPERATION                                             */
        /*---------------------------------------------------------------*/
            /* Set command reject sense byte, and unit check status */
            *unitstat=CSW_CE+CSW_DE+CSW_UC;
            dev->sense[0]=SENSE_CR;
            break;

    }
    /* Is the device still there? If not, for example when
       Hercules is shutting down, releasing the lock may cause a segfault */
    if (dev)
        if (dev->commadpt)
            release_lock(&dev->commadpt->lock);
}


/*---------------------------------------------------------------*/
/* DEVICE FUNCTION POINTERS                                      */
/*---------------------------------------------------------------*/

static DEVHND comadpt_device_hndinfo =
{
        &commadpt_init_handler,        /* Device Initialization      */
        &commadpt_execute_ccw,         /* Device CCW execute         */
        &commadpt_close_device,        /* Device Close               */
        &commadpt_query_device,        /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &commadpt_halt_or_clear,       /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        commadpt_immed_command,        /* Immediate CCW Codes        */
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
#define hdl_ddev hdt2703_LTX_hdl_ddev
#define hdl_depc hdt2703_LTX_hdl_depc
#define hdl_reso hdt2703_LTX_hdl_reso
#define hdl_init hdt2703_LTX_hdl_init
#define hdl_fini hdt2703_LTX_hdl_fini
#endif


HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
     HDL_DEPENDENCY(DEVBLK);
     HDL_DEPENDENCY(SYSBLK);
}
END_DEPENDENCY_SECTION


#if defined(WIN32) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)
  #undef sysblk
  HDL_RESOLVER_SECTION;
  {
    HDL_RESOLVE_SYMPTR( psysblk, sysblk );
  }
  END_RESOLVER_SECTION
#endif


HDL_DEVICE_SECTION;
{
    HDL_DEVICE(2703, comadpt_device_hndinfo );
}
END_DEVICE_SECTION
