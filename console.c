/* CONSOLE.C   (C) Copyright Roger Bowler and others, 1999-2016      */
/*              Hercules Console Device Handler                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for console        */
/* devices for the Hercules ESA/390 emulator.                        */
/*                                                                   */
/* Telnet support is provided for three console device classes:      */
/*                                                                   */
/*   local non-SNA 3270 display consoles via tn3270                  */
/*   local non-SNA 3270 printers         via tn3270                  */
/*   1052 and 3215 console printer keyboards via regular telnet      */
/*                                                                   */
/* This module also takes care of the differences between the        */
/* remote 3270 and local non-SNA 3270 devices.  In particular        */
/* the support of command chaining, which is not supported on        */
/* the remote 3270 implementation on which telnet 3270 is based.     */
/*                                                                   */
/* In the local non-SNA environment a chained read or write will     */
/* continue at the buffer address where the previous command ended.  */
/* In order to achieve this, this module will keep track of the      */
/* buffer location, and adjust the buffer address on chained read    */
/* and write operations.                                             */
/*                                                                   */
/* When using VTAM with local non-SNA 3270 devices, ensure that      */
/* enough bufferspace is available when doing IND$FILE type          */
/* filetransfers.  Code IOBUF=(,3992) in ATCSTRxx, and/or BUFNUM=xxx */
/* on the LBUILD LOCAL statement defining the 3270 device.           */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "hexterns.h"
#include "devtype.h"
#include "opcode.h"
#include "sr.h"
#include "cnsllogo.h"
#include "hexdumpe.h"

/*-------------------------------------------------------------------*/
/*                 Tweakable build constants                         */
/*-------------------------------------------------------------------*/
#define BUFLEN_3270     65536           /* 3270 Send/Receive buffer  */
#define BUFLEN_1052     150             /* 1052 Send/Receive buffer  */
//#define FIX_QWS_BUG_FOR_MCS_CONSOLES    /* (see pos_to_buff_offset)  */

/*-------------------------------------------------------------------*/
/*                      TELNET DEBUGGING                             */
/*-------------------------------------------------------------------*/
//#define TN_DEBUG_NEGOTIATIONS       /* #define to log WILL/WONT/etc  */
//#define TN_DEBUG_SIMPLE_IAC         /* #define to log simple IACs    */
//#define TN_DEBUG_NOP_IAC            /* #define to log NOP IACs       */

/*-------------------------------------------------------------------*/
/*                     CONSOLE.C DEBUGGING                           */
/*-------------------------------------------------------------------*/
#define DEBUG_NOTHING       0       /* no debugging                  */
#define DEBUG_STATUS_ONLY   1       /* debug status only             */
#define DEBUG_PROGRESS_TOO  2       /* debug progress too            */
#define DEBUG_EVERYTHING    3       /* debug buffers too             */

#define DEBUG_LVL     DEBUG_NOTHING /* Choose your poison */

#if DEBUG_LVL == DEBUG_NOTHING
  #define CONERROR                          WRMSG
  #define CONDEBUG1         1 ? ((void)0) : WRMSG
  #define CONDEBUG2         1 ? ((void)0) : WRMSG
  #define DUMPBUF(_msg,_addr,_len,_ebcdic)
#elif DEBUG_LVL == DEBUG_STATUS_ONLY
  #define CONERROR                          WRMSG
  #define CONDEBUG1                         WRMSG
  #define CONDEBUG2         1 ? ((void)0) : WRMSG
  #define DUMPBUF(_msg,_addr,_len,_ebcdic)
#elif DEBUG_LVL == DEBUG_PROGRESS_TOO
  #define CONERROR                          WRMSG
  #define CONDEBUG1                         WRMSG
  #define CONDEBUG2                         WRMSG
  #define DUMPBUF(_msg,_addr,_len,_ebcdic)

#elif DEBUG_LVL == DEBUG_EVERYTHING
  #define CONERROR                          WRMSG
  #define CONDEBUG1                         WRMSG
  #define CONDEBUG2                         WRMSG
  #define DUMPBUF(  _msg,       _addr, _len, _ebcdic )  \
          dumpbuf( #_msg "D +", _addr, _len, _ebcdic )

static void dumpbuf( const char* pfx, const void* addr, int len, BYTE ebcdic )
{
    if (len)
    {
        char* dump = NULL;
        const char* ccaddr = (const char*) addr;

        if (ebcdic)
            hexdumpe16( pfx, &dump, ccaddr, 0, len, 0, 4, 4 );
        else
            hexdumpa16( pfx, &dump, ccaddr, 0, len, 0, 4, 4 );

        if (dump)
        {
            LOGMSG( "%s", dump );
            free( dump );
        }
        else
        {
            // "COMM: error in function %s: %s"
            WRMSG( HHC01034, "E", "dumpbuf()", strerror( errno ));
        }
    }
}
#endif // DEBUG_EVERYTHING

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
static BYTE  constty_immed [256] =

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

static BYTE  loc3270_immed [256] =

 /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
  { 0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,  /* 00 */      // 03, 0B, 0F
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,  /* 10 */      //     1B
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,  /* 20 */      //     2B
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,  /* 30 */      //     3B
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,  /* 40 */      //     4B
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
/*          Static variables and forward references                  */
/*-------------------------------------------------------------------*/
static int   console_cnslcnt = 0;
static void* console_connection_handler( void* arg );
static BYTE  solicit_3270_data( DEVBLK* dev, BYTE cmd );
static void  loc3270_input( TELNET* tn, const BYTE* buffer, U32 size );
static void  constty_input( TELNET* tn, const BYTE* buffer, U32 size );
static void  negotiate_ttype( TELNET* tn );

/*-------------------------------------------------------------------*/
/*                Telnet options negotiation table                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The Telopt Options table specifies which TELNET options your     */
/*  application supports both locally and/or remotely.               */
/*                                                                   */
/*  This table is comprised of telnet_telopt_t structures, one for   */
/*  each supported option.  Each entry specifies the option and      */
/*  whether it is supported locally or remotely.                     */
/*                                                                   */
/*  The first column after the option (us) denotes whether or not    */
/*  your application wishes the option to be enabled locally and     */
/*  should be set to TELNET_WILL if yes and or TELNET_WONT if not.   */
/*                                                                   */
/*  The second column (them) denotes whether or not you wish the     */
/*  option to be enabled on the remote end, and should be set to     */
/*  TELNET_DO if yes or TELNET_DONT if not.                          */
/*                                                                   */
/*  Options not defined in the table are handled automatically by    */
/*  libtelnet as TELNET_WONT and TELNET_DONT (i.e. libtelnet will    */
/*  automatically reply with a TELNET_DONT or TELNET_WONT reply      */
/*  for any TELNET_WILL or TELNET_DO sent by the remote side).       */
/*                                                                   */
/*  You should therefore only define table entries for options you   */
/*  wish to possibly support (possibly have enabled) either locally  */
/*  or remotely (us = TELNET_WILL or them = TELNET_DO).              */
/*                                                                   */
/*  Note that TELNET_WILL does not mean the option is enabled on     */
/*  your end (locally), only that you wish to enable it.  It must    */
/*  not actually be enabled until the remote end first responds to   */
/*  your TELNET_WILL request with a TELNET_DO reply (at which point  */
/*  you will receive a TELNET_EV_DO event informing you it is now    */
/*  okay to go ahead and enable that option.                         */
/*                                                                   */
/*  Correspondingly, TELNET_DO table entries for the remote side,    */
/*  indicating that you'd like that option to be enabled at their    */
/*  end, must not be treated as enabled until they first respond     */
/*  to your TELNET_DO request with a TELNET_WILL response (at which  */
/*  point you will be notified via a TELNET_EV_WILL event informing  */
/*  you that the option is now enabled on their end.                 */
/*                                                                   */
/*  Further note that TELNET_WILL/TELNET_WONT/TELNET_DO/TELNET_DONT  */
/*  table entry values are not by any means carved in stone. Any of  */
/*  the telnet options can be changed at any time by simply calling  */
/*  the telnet_negotiate() function with your new WILL/WONT/DO/DONT  */
/*  value for that option and then handling the corresponding event  */
/*  appropriately depending on how the other side responds.          */
/*                                                                   */
/*-------------------------------------------------------------------*/
static const telnet_telopt_t   telnet_opts[] =
{
    //--------------------------------------------------------------
    // PROGRAMMING NOTE: the TELNET_TELOPT_BINARY, TELNET_TELOPT_EOR
    // and TELNET_TELOPT_ECHO options are negotiated manually for each
    // connection (via the libtelnet "telnet_negotiate" function) once
    // the terminal type (TTYPE) is known. For 3270 terminals begining
    // with "IBM-" we operate in BINARY+EOR mode. For all other types
    // we don't and negotiate the TELNET_TELOPT_ECHO option instead.
    // Refer the the "negotiate_ttype" function for more information.
    //--------------------------------------------------------------

    //     (option)               (us)        (them)

    {  TELNET_TELOPT_TTYPE,   TELNET_WONT,  TELNET_DO    }, // (BOTH)

//  {  TELNET_TELOPT_ECHO,    TELNET_WONT,  TELNET_DO    }, // TTY

//  {  TELNET_TELOPT_BINARY,  TELNET_WILL,  TELNET_DO    }, // TN3270
//  {  TELNET_TELOPT_EOR,     TELNET_WILL,  TELNET_DO    }, // TN3270

    { -1, 0, 0 }    /*****  REQUIRED END OF TABLE MARKER *****/
};

/*-------------------------------------------------------------------*/
/*                (event handler helper macro)                       */
/*-------------------------------------------------------------------*/
#define LOG_WARNING_OR_ERROR_EVENT( which, tn, ev )                 \
                                                                    \
    /*  "%s COMM: libtelnet %s: %s in %s() at %s(%d): %s"  */       \
    WRMSG                                                           \
    (                                                               \
        HHC02907, "I",                                              \
        (tn)->clientid,                                             \
        (which),                                                    \
        telnet_err_name( (ev)->error.err ),                         \
        (ev)->error.func,                                           \
        (ev)->error.file,                                           \
        (ev)->error.line,                                           \
        (ev)->error.msg                                             \
    )

/*-------------------------------------------------------------------*/
/*                    Telnet event handler                           */
/*-------------------------------------------------------------------*/
static void telnet_ev_handler( telnet_t* telnet, telnet_event_t* ev,
                               void* user_data )
{
    TELNET* tn = (TELNET*) user_data;

#ifdef TN_DEBUG_NEGOTIATIONS
    switch (ev->type)
    {
    case TELNET_EV_DO:
    case TELNET_EV_DONT:
    case TELNET_EV_WILL:
    case TELNET_EV_WONT:
        // "%s COMM: negotiating %-14s %s"
        WRMSG( HHC90511, "D", tn->clientid,
            telnet_evt_name( ev->neg.type ),
            telnet_opt_name( ev->neg.telopt ));
        break;
    default:
        break;
    }
#endif // TN_DEBUG_NEGOTIATIONS

    switch (ev->type)
    {

    /* Raw data needs to be sent to peer */
    case TELNET_EV_SEND:

        // "%s COMM: sending %d bytes"
        CONDEBUG2( HHC90500, "D", tn->clientid, ev->data.size );
        DUMPBUF(   HHC90500, ev->data.buffer, ev->data.size, tn->do_tn3270 ? 1 : 0 );

        if (write_socket( tn->csock, ev->data.buffer, ev->data.size ) <= 0)
        {
            tn->send_err = TRUE;
            // "%s COMM: send() failed: %s"
            WRMSG( HHC02900, "E", tn->clientid, strerror( errno ));
        }
        break;


    /* Non-telnet data received from peer */
    case TELNET_EV_DATA:

        if (tn->do_tn3270)
            loc3270_input( tn, ev->data.buffer, ev->data.size );
        else
            constty_input( tn, ev->data.buffer, ev->data.size );
        break;


    /* Enable local option */
    case TELNET_EV_DO:

        /* Enable BINARY mode when requested */
        if (ev->neg.telopt == TELNET_TELOPT_BINARY)
        {
            tn->do_bin = 1;
        }

        /* Enable "Suppress Go Aheads" when asked */
        else if (ev->neg.telopt == TELNET_TELOPT_SGA)
        {
            ; // (ignore; we always suppress go-aheads anyway)
        }

        /* Refuse to enable TTYPE option when asked */
        else if (ev->neg.telopt == TELNET_TELOPT_TTYPE)
        {
            /* We can't enable it since we don't HAVE a Terminal Type! */
            // "%s COMM: refusing client demand to %s %s"
            WRMSG( HHC02901, "W", tn->clientid, "enable", "TTYPE" );
            telnet_negotiate( tn->ctl, TELNET_WONT, TELNET_TELOPT_TTYPE );
            tn->neg_fail = TRUE;
        }

        /* Enable EOR mode when requested */
        else if (ev->neg.telopt == TELNET_TELOPT_EOR)
        {
            tn->do_eor = 1;
        }

        /* Refuse to enable any other options we don't support */
        else
        {
            // "%s COMM: refusing client demand to %s %s"
            WRMSG( HHC02901, "W", tn->clientid, "enable",
                telnet_opt_name( ev->neg.telopt ) );
            telnet_negotiate( tn->ctl, TELNET_WONT, ev->neg.telopt );
            tn->neg_fail = TRUE;
        }
        break;


    /* Disable local option */
    case TELNET_EV_DONT:

        /* Disable BINARY mode when requested (if possible) */
        if (ev->neg.telopt == TELNET_TELOPT_BINARY)
        {
            if (tn->do_tn3270)
            {
                /* REFUSE! TN3270 mode requires it! */
                // "%s COMM: refusing client demand to %s %s"
                WRMSG( HHC02901, "W", tn->clientid, "disable", "BINARY mode" );
                telnet_negotiate( tn->ctl, TELNET_WILL, TELNET_TELOPT_BINARY );
                tn->neg_fail = TRUE;
            }
            else
            {
                /* Otherwise do as requested */
                if (tn->do_bin)
                {
                    tn->do_bin = 0;
                    telnet_negotiate( tn->ctl, TELNET_WONT, TELNET_TELOPT_BINARY );
                }
            }
        }

        /* ALWAYS refuse to disable "Suppress Go Aheads" */
        else if (ev->neg.telopt == TELNET_TELOPT_SGA)
        {
            // "%s COMM: refusing client demand to %s %s"
            WRMSG( HHC02901, "W", tn->clientid, "disable", "Suppress Go Aheads" );
            telnet_negotiate( tn->ctl, TELNET_WILL, TELNET_TELOPT_SGA );
            tn->neg_fail = TRUE;
        }

        /* This should never occur since we never said we wanted to enable
           it, but if they want to demand we disable it anyway, then fine. */
        else if (ev->neg.telopt == TELNET_TELOPT_TTYPE)
        {
            ; // (ignore)
        }

        /* Disable EOR mode when requested (if possible) */
        else if (ev->neg.telopt == TELNET_TELOPT_EOR)
        {
            if (tn->do_tn3270)
            {
                /* REFUSE! TN3270 mode requires it! */
                // "%s COMM: refusing client demand to %s %s"
                WRMSG( HHC02901, "W", tn->clientid, "disable", "EOR mode" );
                telnet_negotiate( tn->ctl, TELNET_WILL, TELNET_TELOPT_EOR );
                tn->neg_fail = TRUE;
            }
            else
            {
                /* Otherwise do as requested */
                if (tn->do_eor)
                {
                    tn->do_eor = 0;
                    telnet_negotiate( tn->ctl, TELNET_WONT, TELNET_TELOPT_EOR );
                }
            }
        }
        break;


    /* Remote option enabled */
    case TELNET_EV_WILL:

        if (ev->neg.telopt == TELNET_TELOPT_BINARY)
        {
            ; // (ignore)
        }

        else if (ev->neg.telopt == TELNET_TELOPT_SGA)
        {
            ; // (ignore; we expect them to always suppress go-aheads anyway)
        }

        /* Ask them to SEND us their TTYPE when they're willing to */
        else if (ev->neg.telopt == TELNET_TELOPT_TTYPE)
        {
            telnet_ttype_send( tn->ctl );
        }

        else if (ev->neg.telopt == TELNET_TELOPT_EOR)
        {
            ; // (ignore)
        }
        break;


    /* Remote option disabled */
    case TELNET_EV_WONT:

        /* Fail negotiations if they refused TN3270 BINARY mode */
        if (ev->neg.telopt == TELNET_TELOPT_BINARY)
        {
            if (tn->do_tn3270)
            {
                /* TN3270 mode requires it! */
                // "%s COMM: client refused to %s %s"
                WRMSG( HHC02902, "W", tn->clientid, "enable", "BINARY mode" );
                tn->neg_fail = TRUE;
            }
        }

        /* Fail negotiations if they refuse to Suppress Go Aheads */
        else if (ev->neg.telopt == TELNET_TELOPT_SGA)
        {
            // "%s COMM: client refused to %s %s"
            WRMSG( HHC02902, "W", tn->clientid, "enable", "Suppress Go Aheads" );
            tn->neg_fail = TRUE;
        }

        /* Fail negotiations if they refuse to send their TTYPE */
        else if (ev->neg.telopt == TELNET_TELOPT_TTYPE)
        {
            // "%s COMM: client refused to %s %s"
            WRMSG( HHC02902, "W", tn->clientid, "enable", "TTYPE" );
            tn->neg_fail = TRUE;
        }

        /* Fail negotiations if they refused TN3270 EOR mode */
        else if (ev->neg.telopt == TELNET_TELOPT_EOR)
        {
            if (tn->do_tn3270)
            {
                /* TN3270 mode requires it! */
                // "%s COMM: client refused to %s %s"
                WRMSG( HHC02902, "W", tn->clientid, "enable", "EOR mode" );
                tn->neg_fail = TRUE;
            }
        }
        break;


    /* TTYPE command has been received */
    case TELNET_EV_TTYPE:

        if (ev->ttype.cmd == TELNET_TTYPE_IS)
        {
            /* Save terminal type */
            STRLCPY( tn->ttype, ev->ttype.name );

            /* Finish negotiations based on terminal type */
            negotiate_ttype( tn );
        }

        /* Respond with NULL Terminal Type when asked to send it */
        else if (ev->ttype.cmd == TELNET_TTYPE_SEND)
        {
            static const char* ttype = "";
            telnet_ttype_is( telnet, ttype );
        }
        break;


    /* Respond to particular subnegotiations */
    case TELNET_EV_SUBNEGOTIATION:
        break;


    /* Generic IAC command received */
    case TELNET_EV_IAC:

#ifdef  TN_DEBUG_SIMPLE_IAC
#ifndef TN_DEBUG_NOP_IAC
        if (ev->iac.cmd != TELNET_NOP)
#endif
        // "%s COMM: received IAC %s"
        WRMSG( HHC90512, "D", tn->clientid, telnet_cmd_name( ev->iac.cmd ));
#endif

        /* End of record */
        if (ev->iac.cmd == TELNET_EOR)
        {
            if (tn->do_eor)
                tn->got_eor = TRUE;
        }

        /* Break */
        else if (ev->iac.cmd == TELNET_BREAK)
        {
            if (!tn->do_tn3270)
                tn->got_break = TRUE;
        }

        /* Interrupt Process */
        else if (ev->iac.cmd == TELNET_IP)
        {
            if (!tn->do_tn3270)
                tn->got_break = TRUE;
        }

        /* Erase character */
        else if (ev->iac.cmd == TELNET_EC)
        {
            if (!tn->do_tn3270)
            {
                if (tn->dev && tn->dev->keybdrem > 0)
                    tn->dev->keybdrem--;
            }
        }

        /* Erase line */
        else if (ev->iac.cmd == TELNET_EL)
        {
            if (!tn->do_tn3270)
            {
                if (tn->dev)
                    tn->dev->keybdrem = 0;
            }
        }
        break;


    /* Recoverable error has occured */
    case TELNET_EV_WARNING:

        LOG_WARNING_OR_ERROR_EVENT( "WARNING", tn, ev );

        /* Remember negotiation errors */
        if (ev->error.err == TELNET_ENEGOTIATION)
        {
            // "%s COMM: libtelnet negotiation error"
            WRMSG( HHC02905, "W", tn->clientid );
            tn->neg_fail = TRUE;
        }
        else
            // "%s COMM: libtelnet error"
            WRMSG( HHC02903, "W", tn->clientid );

        break;


    /* Non-recoverable error has occured */
    case TELNET_EV_ERROR:

        LOG_WARNING_OR_ERROR_EVENT( "ERROR", tn, ev );

        // "%s COMM: libtelnet FATAL error"
        WRMSG( HHC02904, "E", tn->clientid );
        break;


    default:

        // "%s COMM: Unsupported libtelnet event '%s'"
        WRMSG( HHC02906, "W", tn->clientid, telnet_evt_name( ev->type ));
        break;
    }

} /* end function telnet_ev_handler */

/*-------------------------------------------------------------------*/
/*  MinGW           (Windows, but *not* MSVC)                        */
/*-------------------------------------------------------------------*/
#if defined( WIN32 ) && !defined( _MSVC_ ) && !defined( HDL_USE_LIBTOOL )

        SYSBLK   *psysblk;
#define sysblk  (*psysblk)

#endif

/*-------------------------------------------------------------------*/
/*                 SEND DATA TO TELNET CLIENT                        */
/*-------------------------------------------------------------------*/
static BYTE sendto_client( TELNET* tn, const BYTE* buf, unsigned int len )
{
    BYTE success = TRUE;
    unsigned int sendbuf_needed;

    if (len > 0)
    {
        tn->send_err = FALSE;

        if (tn->devclass == 'K')
        {
            /* PROGRAMMING NOTE: the constty_execute_ccw() function
               that calls us ensures the buffer it passes to us is
               always null terminated so the below telnet_printf
               does not print garbage past the end of actual data.
            */
            telnet_printf( tn->ctl, "%s", buf );
        }
        else // (tn->devclass == 'D' || tn->devclass == 'P')
        {
            /* Adjust one shot send buffer */
            sendbuf_needed = (len * 2) + 2;
            if (sendbuf_needed > tn->sendbuf_size)
            {
                if (tn->sendbuf_size) free( tn->sendbuf );
                tn->sendbuf_size = sendbuf_needed;
                tn->sendbuf = (char*) malloc( tn->sendbuf_size );
            }

            telnet_send_one_shot( tn->ctl, buf, len, tn->sendbuf );
        }

        success = !tn->send_err;
    }

    return success;
}

/*-------------------------------------------------------------------*/
/*                  RAISE ATTENTION INTERRUPT                        */
/*-------------------------------------------------------------------*/
static void raise_device_attention( DEVBLK* dev, BYTE unitstat )
{
    /* NOTE: the device lock must *NOT* be held! */

#if defined( _FEATURE_INTEGRATED_3270_CONSOLE )
    if (dev == sysblk.sysgdev)
    {
        sclp_sysg_attention();
        return;
    }
#endif

    if (dev->tn->devclass != 'P' && !INITIAL_POWERON_370())
    {
        int rc = device_attention( dev, unitstat );

        // "%s COMM: device attention %s; rc=%d"
        CONDEBUG1( HHC90506, "D", dev->tn->clientid,
            (rc == 0 ? "raised" : "REJECTED"), rc );
    }
}

/*-------------------------------------------------------------------*/
/*           FINISH INITIALIZING THE CONSOLE DEVICE                  */
/*-------------------------------------------------------------------*/
static int  finish_console_init( DEVBLK* dev )
{
    UNREFERENCED( dev );

    if (!console_cnslcnt && !sysblk.cnsltid)
    {
        int rc;

        console_cnslcnt++;  // No serialisation needed just yet, as
                            // the console thread is not yet active

        if ((rc = create_thread( &sysblk.cnsltid, JOINABLE,
                                console_connection_handler, NULL,
                                CON_CONN_THREAD_NAME )))
        {
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ));
            return 1;
        }
    }
    else
        console_cnslcnt++;

    return 0;
}

/*-------------------------------------------------------------------*/
/*                 DISCONNECT TELNET CLIENT                          */
/*-------------------------------------------------------------------*/
static void  disconnect_telnet_client( TELNET* tn )
{
    /* PROGRAMMING NOTE: do not call this function once a DEVBLK
       has been chosen. Only use it to disconnect a client before
       negotiations have been completed (before a DEVBLK has been
       chosen). Once a DEVBLK has been chosen, you should use the
       disconnect_console_device function instead.
    */
    if (tn)
    {
        telnet_closesocket( tn->csock );
        telnet_free( tn->ctl );

        /* Free one shot send buffer if necessary */
        if (tn->sendbuf_size) free( tn->sendbuf );
        tn->sendbuf_size = 0;

        // "%s COMM: disconnected"
        CONDEBUG1( HHC90504, "D", tn->clientid );

        free( tn );
    }
}

/*-------------------------------------------------------------------*/
/*                DISCONNECT CONSOLE DEVICE                          */
/*-------------------------------------------------------------------*/
static void  disconnect_console_device( DEVBLK* dev )
{
    /* PROGRAMMING NOTE: this function should only be called to
       disconnect a 3270/TTY console device from a telnet client
       such as when a serious I/O error occurs. It physically
       closes the device and marks it available for reuse.
    */
    dev->connected =  0;
    dev->fd        = -1;

    disconnect_telnet_client( dev->tn );

    dev->tn        = NULL;
}

/*-------------------------------------------------------------------*/
/*             FINISH CLOSING THE CONSOLE DEVICE                     */
/*-------------------------------------------------------------------*/
static void  finish_console_close( DEVBLK* dev )
{
    /* PROGRAMMING NOTE: this function should never be called
       directly. It is a helper function for the device handler
       close function.
    */
    disconnect_console_device( dev );

    console_cnslcnt--;

    SIGNAL_CONSOLE_THREAD();

    if (!console_cnslcnt)
    {
        RELEASE_DEVLOCK( dev );
        {
            join_thread( sysblk.cnsltid, NULL);
        }
        OBTAIN_DEVLOCK( dev );

        sysblk.cnsltid = 0;
    }
}

/*-------------------------------------------------------------------*/
/*                 CLOSE THE 3270 DEVICE                             */
/*-------------------------------------------------------------------*/
static int loc3270_close_device( DEVBLK* dev )
{
    /* PROGRAMMING NOTE: this function is the device handler's
       close function. It should only be called when detaching
       a 3270 console device. To disconnect a 3270 console from
       whatever telnet client is connected to it, you should use
       the disconnect_console_device function instead.
    */
#if defined(_FEATURE_INTEGRATED_3270_CONSOLE)

    if (dev == sysblk.sysgdev)
        sysblk.sysgdev = NULL;

#endif

    finish_console_close( dev );
    return 0;
}

/*-------------------------------------------------------------------*/
/*                 CLOSE THE 1052/3215 DEVICE                        */
/*-------------------------------------------------------------------*/
static int constty_close_device( DEVBLK* dev )
{
    /* PROGRAMMING NOTE: this function is the device handler's
       close function. It should only be called when detaching
       a TTY console device. To disconnect a TTY console from
       whatever telnet client is connected to it, you should use
       the disconnect_console_device function instead.
    */
    finish_console_close (dev );
    return 0;
}

/*-------------------------------------------------------------------*/
/*                 INITIALIZE 3270 DEVICE                            */
/*-------------------------------------------------------------------*/
static int  loc3270_init_handler( DEVBLK* dev, int argc, char* argv[] )
{
    int ac = 0;
    BYTE dt;

    /* Close the existing file in case we're re-initialising */
    if (dev->fd >= 0)
        (dev->hnd->close)( dev );

    /* reset excp count */
    dev->excps = 0;

    /* Indicate that this is a console device */
    dev->console = 1;

    /* Reset device dependent flags */
    dev->connected = 0;

    /* Set number of sense bytes */
    dev->numsense = 1;

    /* Set the size of the device buffer */
    dev->bufsize = BUFLEN_3270;

    if (!sscanf( dev->typname, "%hx", &dev->devtype ))
        dev->devtype = 0x3270;

#if defined( _FEATURE_INTEGRATED_3270_CONSOLE )

    /* Extra initialisation for the SYSG console */
    if (strcasecmp( dev->typname, "SYSG" ) == 0)
    {
        if (sysblk.sysgdev != NULL)
        {
            // "%1d:%04X COMM: duplicate SYSG console definition"
            WRMSG( HHC01025, "E", LCSS_DEVNUM );
            return -1;
        }

        dev->pmcw.flag5 &= ~PMCW5_V;  // (not a regular device)
    }
#endif

    /* Initialize the device identifier bytes */

    dev->devid[0] = 0xFF;       /* First byte is always 0xFF */

    dev->devid[1] = 0x32;       /* Control unit type is 3274-1D */
    dev->devid[2] = 0x74;       /* Control unit type is 3274-1D */
    dev->devid[3] = 0x1D;       /* Control unit type is 3274-1D */

    dev->devid[4] = 0x32;       /* Device type major */
    dt = (dev->devtype & 0xFF); /* Device type minor */

    if (dt == 0x70)             /* Device statement type = 3270? */
    {
        dev->devid[5] = 0x78;   /* then it's a 3278-2 by default */
        dev->devid[6] = 0x02;   /* then it's a 3278-2 by default */
    }
    else
    {
        /* Not a 3270 display. Use the device type
           they specified on their device statement,
           e.g. 3287, and force it to be a model 1.
        */
        dev->devid[5] = dt;     /* Device statement type */
        dev->devid[6] = 0x01;   /* Model 1 */
    }
    dev->numdevid = 7;

    dev->filename[0] = 0;
    dev->acc_ipaddr  = 0;
    dev->acc_ipmask  = 0;

    if (argc > 0)   // group name?
    {
        if ('*' == argv[ac][0] && '\0' == argv[ac][1])
            ;   // NOP (not really a group name; an '*' is
                // simply used as an argument place holder)
        else
        {
            if (0
                || strlen( argv[ac] ) <= 0
                || strlen( argv[ac] ) >= 9
            )
            {
                // "%1d:%04X COMM: %s has an invalid GROUP name length or format; must be a valid luname or poolname"
                WRMSG( HHC01091, "E", LCSS_DEVNUM, argv[ac] );
                return -1;
            }
            else
            {
                char group[9];
                int  i;
                int  rc;

                strupper( group, argv[ac] );

                for (i=1, rc=0; i < (int) strlen( group ) && rc == 0; i++)
                    if (!isalnum( (unsigned char)group[i] ))
                        rc = -1;

                if (rc == 0 && isalpha( (unsigned char)group[0] ))
                    STRLCPY( dev->filename, group );
                else
                {
                    // "%1d:%04X COMM: %s has an invalid GROUP name length or format; must be a valid luname or poolname"
                    WRMSG(HHC01091, "E", LCSS_DEVNUM, argv[ac] );
                    return -1;
                }
            }
        }

        argc--;
        ac++;

        if (argc > 0)   // ip address?
        {
            if ((dev->acc_ipaddr = inet_addr( argv[ac] )) == (in_addr_t)(-1))
            {
                // "%1d:%04X COMM: option %s value %s invalid"
                WRMSG( HHC01007, "E", LCSS_DEVNUM, "IP address", argv[ac] );
                return -1;
            }
            else
            {
                argc--;
                ac++;

                if (argc <= 0)   // ip addr mask?
                    dev->acc_ipmask = (in_addr_t)(-1);
                else
                {
                    char mask[16] = {0};
                    static const char* badmask = "0.0.0.0";

                    if (0
                        || inet_pton( AF_INET, argv[ac], &dev->acc_ipmask ) <= 0
                        || str_eq( badmask, inet_ntop( AF_INET, &dev->acc_ipmask,
                                   mask, (int) sizeof( mask )))
                    )
                    {
                        // "%1d:%04X COMM: option %s value %s invalid"
                        WRMSG( HHC01007, "E", LCSS_DEVNUM, "mask", argv[ac] );
                        return -1;
                    }

                    argc--;
                    ac++;

                    if (argc > 0)   // too many args?
                    {
                        // "%1d:%04X COMM: unrecognized parameter %s"
                        WRMSG( HHC01019, "E", LCSS_DEVNUM, argv[ac] );
                        return -1;
                    }
                }
            }
        }
    }

#if defined(_FEATURE_INTEGRATED_3270_CONSOLE)

    /* Save the address of the SYSG console devblk */
    if (strcasecmp( dev->typname, "SYSG" ) == 0)
        sysblk.sysgdev = dev;
#endif

    return finish_console_init( dev );

} /* end function loc3270_init_handler */

/*-------------------------------------------------------------------*/
/*                INITIALIZE 1052/3215 DEVICE                        */
/*-------------------------------------------------------------------*/
static int  constty_init_handler( DEVBLK* dev, int argc, char* argv[] )
{
    int ac=0;

    /* Close the existing file in case we're re-initialising */
    if (dev->fd >= 0)
        (dev->hnd->close)( dev );

    /* reset excp count */
    dev->excps = 0;

    /* Indicate that this is a console device */
    dev->console = 1;

    /* Set number of sense bytes */
    dev->numsense = 1;

    /* Initialize device dependent fields */
    dev->keybdrem = 0;

    /* Set the size of the device buffer */
    dev->bufsize = BUFLEN_1052;

    /* Assume we want to prompt */
    dev->prompt1052 = 1;

    /* Is there an argument? */
    if (argc > 0)
    {
        /* Look at the argument and set noprompt flag if specified. */
        if (strcasecmp( argv[ac], "noprompt" ) == 0)
        {
            dev->prompt1052 = 0;

            ac++;
            argc--;
        }
        /* else it's a group name */
    }

    if (!sscanf( dev->typname, "%hx", &dev->devtype ))
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

    dev->filename[0] = 0;
    dev->acc_ipaddr  = 0;
    dev->acc_ipmask  = 0;

    if (argc > 0)   // group name?
    {
        if ('*' == argv[ac][0] && '\0' == argv[ac][1])
            ;   // NOP (not really a group name; an '*' is
                // simply used as an argument place holder)
        else
            STRLCPY( dev->filename, argv[ac] );

        argc--;
        ac++;

        if (argc > 0)   // ip address?
        {
            if ((dev->acc_ipaddr = inet_addr( argv[ac] )) == (in_addr_t)(-1))
            {
                // "%1d:%04X COMM: option %s value %s invalid"
                WRMSG( HHC01007, "E", LCSS_DEVNUM, "IP address", argv[ac] );
                return -1;
            }
            else
            {
                argc--;
                ac++;

                if (argc <= 0)   // ip addr mask?
                    dev->acc_ipmask = (in_addr_t)(-1);
                else
                {
                    char mask[16] = {0};
                    static const char* badmask = "0.0.0.0";

                    if (0
                        || inet_pton( AF_INET, argv[ac], &dev->acc_ipmask ) <= 0
                        || str_eq( badmask, inet_ntop( AF_INET, &dev->acc_ipmask,
                                   mask, (int) sizeof( mask )))
                    )
                    {
                        // "%1d:%04X COMM: option %s value %s invalid"
                        WRMSG( HHC01007, "E", LCSS_DEVNUM, "mask", argv[ac] );
                        return -1;
                    }

                    argc--;
                    ac++;

                    if (argc > 0)   // too many args?
                    {
                        // "%1d:%04X COMM: unrecognized parameter %s"
                        WRMSG( HHC01019, "E", LCSS_DEVNUM, argv[ac] );
                        return -1;
                    }
                }
            }
        }
    }

    return finish_console_init( dev );

} /* end function constty_init_handler */

/*-------------------------------------------------------------------*/
/* Parse "host:port" string; return malloc'ed struct sockaddr_in ptr */
/*-------------------------------------------------------------------*/
static struct sockaddr_in* parse_sockspec( const char* sockspec )
{
    struct sockaddr_in*  sin;

    char*  workspec;
    char*  host;
    char*  port;

    if (!sockspec)
        return NULL;

    // Make working copy so we can modify it

    if (0
        || !(workspec = strdup( sockspec ))
        || !(sin = malloc( sizeof( struct sockaddr_in )))
    )
    {
        if (workspec)
            free( workspec );
        // "Out of memory"
        WRMSG( HHC00152, "E" );
        return NULL;
    }

    sin->sin_family = AF_INET;

    // Parse spec into separate host and port components

    if (!(port = strchr( workspec, ':' )))
    {
        // Only the port number was specified
        host = NULL;
        port = workspec;
    }
    else
    {
        // Both host and port were given
        host = workspec;
        *port++ = '\0';
    }

    // Convert host to ipaddr

    if (!host)
        sin->sin_addr.s_addr = INADDR_ANY;
    else
    {
        // (get ipaddr via host name)
        struct hostent* hostent = gethostbyname( host );

        if (!hostent)
        {
            // "COMM: unable to determine %s from %s"
            WRMSG( HHC01016, "I", "IP address", host );
            free( workspec );
            free( sin );
            return NULL;
        }

        memcpy( &sin->sin_addr, *hostent->h_addr_list, sizeof( sin->sin_addr ));
    }

    // Convert port to number

    if (isdigit( (unsigned char)*port ))
        sin->sin_port = htons( atoi( port ));
    else
    {
        // (get port number via service name)
        struct servent* servent = getservbyname( port, "tcp" );

        if (!servent)
        {
            // "COMM: unable to determine %s from %s"
            WRMSG( HHC01016, "I", "port number", port );
            free( workspec );
            free( sin );
            return NULL;
        }

        sin->sin_port = servent->s_port;
    }

    free( workspec );
    return sin;
}

/*-------------------------------------------------------------------*/
/*             3270 CCW opcodes and buffer orders                    */
/*-------------------------------------------------------------------*/

/* 3270 local commands (CCWs) */
#define L3270_EAU       0x0F            /* Erase All Unprotected     */
#define L3270_EW        0x05            /* Erase/Write               */
#define L3270_EWA       0x0D            /* Erase/Write Alternate     */
#define L3270_RB        0x02            /* Read Buffer               */
#define L3270_RM        0x06            /* Read Modified             */
#define L3270_WRT       0x01            /* Write                     */
#define L3270_WSF       0x11            /* Write Structured Field    */

#define L3270_NOP       0x03            /* No Operation              */
#define L3270_SELRM     0x0B            /* Select RM                 */
#define L3270_SELRB     0x1B            /* Select RB                 */
#define L3270_SELRMP    0x2B            /* Select RMP                */
#define L3270_SELRBP    0x3B            /* Select RBP                */
#define L3270_SELWRT    0x4B            /* Select WRT                */
#define L3270_SENSE     0x04            /* Sense                     */
#define L3270_SENSEID   0xE4            /* Sense ID                  */

/* 3270 remote commands (CCWs) */
#define R3270_EAU       0x6F            /* Erase All Unprotected     */
#define R3270_EW        0xF5            /* Erase/Write               */
#define R3270_EWA       0x7E            /* Erase/Write Alternate     */
#define R3270_RB        0xF2            /* Read Buffer               */
#define R3270_RM        0xF6            /* Read Modified             */
#define R3270_RMA       0x6E            /* Read Modified All         */
#define R3270_WRT       0xF1            /* Write                     */
#define R3270_WSF       0xF3            /* Write Structured Field    */

/* 3270 orders */
#define O3270_SBA       0x11            /* Set Buffer Address        */
#define O3270_SF        0x1D            /* Start Field               */
#define O3270_SFE       0x29            /* Start Field Extended      */
#define O3270_SA        0x28            /* Set Attribute             */
#define O3270_IC        0x13            /* Insert Cursor             */
#define O3270_MF        0x2C            /* Modify Field              */
#define O3270_PT        0x05            /* Program Tab               */
#define O3270_RA        0x3C            /* Repeat to Address         */
#define O3270_EUA       0x12            /* Erase Unprotected to Addr */
#define O3270_GE        0x08            /* Graphic Escape            */

/* Inbound structured fields */
#define SF3270_AID      0x88            /* Aid value of inbound SF   */
#define SF3270_3270DS   0x80            /* SFID of 3270 datastream SF*/

/*-------------------------------------------------------------------*/
/*    ADVANCE TO NEXT CHAR OR ORDER IN A 3270 DATA STREAM BUFFER     */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      buf     Buffer containing 3270 data stream                   */
/*      poff    Ptr to buffer offset   of current character or order */
/*      ppos    Ptr to screen position of current character or order */
/* Output:                                                           */
/*      poff    Updated buffer offset   of next character or order   */
/*      ppos    Updated screen position of next character or order   */
/*-------------------------------------------------------------------*/
static void  next_3270_pos( const BYTE* buf, int* poff, int* ppos )
{
    int i;

    /* Copy the offset and advance the offset by 1 byte */

    i = (*poff)++;

    /*  Advance the offset past the argument bytes
        and update the screen position accordingly
    */
    switch (buf[i])
    {
    /*  The Repeat to Address order has 3 argument bytes (or in case
        of a Graphics Escape 4 bytes) and sets the screen position
    */
    case O3270_RA:

        *poff += (buf[i+3] == O3270_GE) ? 4 : 3;

        if ((buf[i+1] & 0xC0) == 0x00)
            *ppos = (buf[i+1] << 8)
                  |  buf[i+2];
        else
            *ppos = ((buf[i+1] & 0x3F) << 6)
                  |  (buf[i+2] & 0x3F);
        break;

    /*  The Start Field Extended and Modify Field orders have
        a count byte followed by a variable number of type-
        attribute pairs, and advance the screen position by 1
    */
    case O3270_SFE:
    case O3270_MF:

        *poff += (1 + (2 * buf[i+1]));

        (*ppos)++;
        break;

    /*  The Set Buffer Address and Erase Unprotected to Address
        orders have 2 argument bytes and set the screen position
    */
    case O3270_SBA:
    case O3270_EUA:

        *poff += 2;

        if ((buf[i+1] & 0xC0) == 0x00)
            *ppos =  (buf[i+1] << 8)
                  |   buf[i+2];
        else
            *ppos = ((buf[i+1] & 0x3F) << 6)
                  |  (buf[i+2] & 0x3F);
        break;

    /*  The Set Attribute order has 2 argument bytes and
        does not change the screen position
    */
    case O3270_SA:

        *poff += 2;
        break;

    /*  Insert Cursor and Program Tab have no argument
        bytes and do not change the screen position
    */
    case O3270_IC:
    case O3270_PT:

        break;

    /*  The Start Field and Graphics Escape orders have one
        argument byte, and advance the screen position by 1
    */
    case O3270_SF:
    case O3270_GE:

        (*poff)++;
        (*ppos)++;
        break;

    /* All other characters advance the screen position by 1 */
    default:

        (*ppos)++;
        break;

    } /* end switch */

} /* end function next_3270_pos */

/*-------------------------------------------------------------------*/
/*               GET END-OF-BUFFER SCREEN POSITION                   */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      pos     Current screen position                              */
/*      buf     Pointer to the byte in the 3270 data stream buffer   */
/*              corresponding to the current screen position         */
/*      size    Number of bytes remaining in screen buffer           */
/* Output:                                                           */
/*      Screen position corresponding to the end of the buffer       */
/*-------------------------------------------------------------------*/
static int  end_of_buf_pos( int pos,
                            const BYTE* buf, int size )
{
    int eob_pos = pos;
    int offset  = 0;
    while (offset < size)
        next_3270_pos( buf, &offset, &eob_pos );
    return eob_pos;
}

/*-------------------------------------------------------------------*/
/*            GET BUFFER OFFSET FOR GIVEN SCREEN POSITION            */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      pos     Screen position whose buffer offset is desired       */
/*      buf     Buffer containing an inbound 3270 data stream        */
/*      size    Number of bytes there are in the buffer              */
/* Return value:                                                     */
/*      Offset into buffer of the character or order corresponding   */
/*      to the given screen position or zero if position not found.  */
/*-------------------------------------------------------------------*/
static int  pos_to_buff_offset( int pos,
                                const BYTE* buf, int size )
{
    int  wpos;                          /* Working screen position   */
    int  woff;                          /* Working offset in buffer  */

    /* Screen position 0 is at offset 3 into the device buffer,
       immediately following the AID and cursor address bytes. */

    wpos = 0;
    woff = 3;

    /* Locate the desired screen position... */

    while (woff < size)       /* While not at or past end of buff */
    {
        if (wpos >= pos)      /* Desired screen position reached? */
        {
#ifdef FIX_QWS_BUG_FOR_MCS_CONSOLES

            /* There is a bug in QWS3270 when used to emulate an
               MCS console with EAB.  At position 1680 the Read
               Buffer contains two 6-byte SFE orders (12 bytes)
               preceding the entry area, whereas MCS expects the
               entry area to start 4 bytes after screen position
               1680 in the buffer.  The bypass is to add 8 to the
               calculated buffer offset if this appears to be an
               MCS console read buffer command
            */
            if (1
                && pos         == 1680          // 0x0690
                && buf[woff+0] == O3270_SFE
                && buf[woff+6] == O3270_SFE
            )
            {
                LOGMSG("*** OLD: wpos: %d (0x%4.4X), woff: %d (0x%4.4X)\n",
                    wpos, wpos, woff, woff );

                woff += 8;

                LOGMSG("*** NEW: wpos: %d (0x%4.4X), woff: %d (0x%4.4X)\n",
                    wpos, wpos, woff, woff );
        }
#endif /* FIX_QWS_BUG_FOR_MCS_CONSOLES */

            return woff;      /* Return calculated buffer offset  */
        }

        /* Advance to next character or order, updating the
           buffer offset and screen position as we go... */

        next_3270_pos( buf, &woff, &wpos );
    }

    /*  Return offset zero if the position cannot be determined */

    return 0;

} /* end function pos_to_buff_offset */

/*-------------------------------------------------------------------*/
/*                        build_logo                                 */
/*-------------------------------------------------------------------*/

/* build_logo constants */

#define SF_ATTR_PROTECTED   0x20
#define SF_ATTR_NUMERIC     0x10
#define SF_ATTR_WDISPNSEL   0x00
#define SF_ATTR_WDISPSEL    0x04
#define SF_ATTR_HIGHLIGHT   0x08
#define SF_ATTR_INVISIBLE   0x0C
#define SF_ATTR_MDT         0x01

#define ALIGN_NONE          0
#define ALIGN_CENTER        1
#define ALIGN_LEFT          2
#define ALIGN_RIGHT         3

#define LOGO_BUFSZ_INCR     256

/* build_logo helper functions */

static BYTE* buffer_addchar   ( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE c );
static BYTE* buffer_addstring ( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE* str );
static BYTE* buffer_addsba    ( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE line, BYTE col );
static BYTE* buffer_addsf     ( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE attr );

/*-------------------------------------------------------------------*/
/*                     build_logo function                           */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Build a default logo screen buffer or one based on passed data.   */
/*                                                                   */
/* If logo_stmts is NULL the built-in internal default logo screen   */
/* is built. Otherwise logo_stmts must point to an array of string   */
/* pointers and 'num_stmts' should be set to the number of pointers  */
/* in that array. The strings in the 'logo_stmts' array must conform */
/* to the format as described in the README.HERCLOGO file.           */
/*                                                                   */
/* 'buflen' is the address of a size_t value that will be updated to */
/* size of the returned buffer if successful.                        */
/*                                                                   */
/* 'errmsg' is the address of a char pointer that should be updated  */
/* with the address of a dynamically generated error message should  */
/* an error occur. Otherwise it is not used. If returned, the caller */
/* should 'free' it once they are done using it. NULL may be passed  */
/* if error messages are not wanted.                                 */
/*                                                                   */
/* Returns a char pointer to the screen buffer if successful,        */
/* and 'buflen' is set to the length of the buffer.                  */
/*                                                                   */
/* Returns NULL on failure in which case 'buflen' is undefined.      */
/*                                                                   */
/*-------------------------------------------------------------------*/
static BYTE* build_logo( char** logo_stmts, size_t num_stmts,
                         size_t* buflen, const char** errmsg )
{
    char     msgbuf[ 128 ];             /* Used only if error oocurs */

    BYTE    *bfr        = NULL;
    char    *stmt       = NULL;
    char    *msg        = NULL;
    char    *strtok_str = NULL;

    char    *verb;
    char    *rest;
    char    *wrk;

    size_t   len = 0;
    size_t   alen = 0;
    size_t   stmt_len;
    size_t   i,j;

    BYTE     line, col;
    BYTE     attr;
    BYTE     align;

    if (errmsg)
        *errmsg = NULL;

    if (!logo_stmts)
    {
        logo_stmts = herclogo; /* use built-in default */
        num_stmts = sizeof( herclogo ) / sizeof( char* );
    }

    /* Allocate a working screen buffer and initialize
       it with an Erase/Write and Restore Keyboard WCC.
    */
    if (0
        || !(bfr = malloc( alen = LOGO_BUFSZ_INCR ))
        || !(bfr = buffer_addchar( bfr, &len, &alen, 0xf5 ))
        || !(bfr = buffer_addchar( bfr, &len, &alen, 0x42 ))
    )
        goto outofmem_error;

    /* Process logo statements and build screen buffer... */

    attr   =  SF_ATTR_PROTECTED;
    align  =  ALIGN_NONE;
    line   =  0;
    col    =  0;

    for (i=0, stmt = NULL, msg = NULL; bfr && i < num_stmts; i++)
    {
        if (stmt)
            free( stmt );

        /* Copy logo statement to dynamically allocated
           buffer so we can freely resolve any symbols. */

        stmt_len = strlen( logo_stmts[i] ) + 1;

        if (!(stmt = malloc( stmt_len )))
            goto outofmem_error;

        strlcpy( stmt, logo_stmts[i], stmt_len );

        wrk = resolve_symbol_string( stmt );
        if (wrk)
        {
            free( stmt );
            stmt = wrk;
        }

        if (stmt[0] != '@')
        {
            /* Perform any previously requested alignment
               before processing this plain text string */

            switch (align)
            {
                case ALIGN_NONE:
                    /* (nothing to do here) */
                    break;

                case ALIGN_LEFT:

                    col = 0;
                    break;

                case ALIGN_RIGHT:

                    col = (int) strlen( stmt );

                    if (col < 80)
                        col = 80 - col;
                    else
                        col = 0;
                    break;

                case ALIGN_CENTER:

                    col = (int) strlen( stmt );

                    if (col < 80)
                        col = (80 - col) / 2;
                    break;

                default:
                    msg = "LOGIC ERROR!";
                    goto error_exit;
            }

            /* Append a Buffer Address order for requested alignment,
               insert the Start Field attribute for the current field,
               and insert the field itself (the current statement).
            */
            if (0
                || !(bfr = buffer_addsba    ( bfr, &len, &alen, line, col ))
                || !(bfr = buffer_addsf     ( bfr, &len, &alen, attr ))
                || !(bfr = buffer_addstring ( bfr, &len, &alen, prt_host_to_guest
                                             ((BYTE*) stmt, (BYTE*) stmt, strlen( stmt )) ))
            )
                goto outofmem_error;

            /* Update line/col variables to match current position */
            if (align == ALIGN_NONE)
            {
                col += strlen( stmt );
                col++;
            }
            else /* (position to beginning of next line) */
            {
                col = 0;
                line++;
            }

            /* Go on to next logo statement */
            continue;
        }

        /* Process special logo '@' order */

        verb = strtok_r( stmt," \t", &strtok_str );
        rest = strtok_r( NULL," \t", &strtok_str );

        /* Set Buffer Address? */

        if (strcasecmp( verb, "@sba" ) == 0)
        {
            if (rest != NULL)
            {
                /* Parse "Y,X" (line,col) values */

                wrk = strtok_r( rest, ",", &strtok_str );
                line = atoi( wrk );

                wrk = strtok_r( NULL, ",", &strtok_str );

                if (wrk != NULL)
                    col = atoi( wrk );
                else
                {
                    msg = "missing @sba ..,X value";
                    goto error_exit;
                }
            }
            else
            {
                msg = "missing @sba Y,X values";
                goto error_exit;
            }
        }

        /* Start Field attribute? */

        else if (strcasecmp( verb, "@sf" ) == 0)
        {
            attr = SF_ATTR_PROTECTED;

            if (rest != NULL)
            {
                for (j=0; rest[j] != 0; j++)
                {
                    switch( rest[j] )
                    {
                        case 'h':
                        case 'H':
                            attr |= SF_ATTR_HIGHLIGHT;
                            break;
                        case 'p':
                        case 'P':
                            attr |= SF_ATTR_PROTECTED;
                            break;
                        case 'i':
                        case 'I':
                            attr &=~ SF_ATTR_PROTECTED;
                            break;
                        default:
                            MSGBUF( msgbuf, "unrecognized @sf value: %c", rest[j] );
                            msg = msgbuf;
                            goto error_exit;
                    }
                }
            }
        }

        /* Start a New Line? */

        else if (strcasecmp( verb, "@nl" ) == 0)
        {
            line++;
            col = 0;
        }

        /* Align next field? */

        else if (strcasecmp( verb, "@align" ) == 0)
        {
            align = ALIGN_NONE;

            if (rest != NULL)
            {
                if      (strcasecmp( rest, "none"   ) == 0) align = ALIGN_NONE;
                else if (strcasecmp( rest, "left"   ) == 0) align = ALIGN_LEFT;
                else if (strcasecmp( rest, "right"  ) == 0) align = ALIGN_RIGHT;
                else if (strcasecmp( rest, "center" ) == 0) align = ALIGN_CENTER;
                else
                {
                    MSGBUF( msgbuf, "unrecognized @align value: %s", rest );
                    msg = msgbuf;
                    goto error_exit;
                }
            }
        }

        /* It's none of those */

        else
        {
            MSGBUF( msgbuf, "unrecognized order: %s", verb );
            msg = msgbuf;
            goto error_exit;
        }

    } /* end for (i=0; bfr && i < num_stmts; i++) */

    if (stmt)
        free( stmt );

    *buflen = len;

    return bfr;

outofmem_error:

    msg = "out of memory";

error_exit:

    if (stmt)
        free( stmt );

    if (bfr)
        free( bfr );

    if (errmsg)
        *errmsg = strdup( msg );

    return NULL;

} /* end function build_logo */

/*-------------------------------------------------------------------*/
/*                build_logo helper functions...                     */
/*-------------------------------------------------------------------*/

static BYTE *buffer_addchar( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE c )
{
    size_t  len   =  *buflen;
    size_t  alen  =  *alloc_len;

    if (len >= alen)
    {
        if (!alen)
        {
            alen = LOGO_BUFSZ_INCR;             // (initialize)

            if (!(bfr = malloc( alen )))        // (allocate)
                return NULL;
        }
        else
        {
            alen += LOGO_BUFSZ_INCR;            // (increment)

            if (!(bfr = realloc( bfr,alen )))   // (re-allocate)
                return NULL;
        }
    }

    bfr[ len++ ] = c;

    *alloc_len  =  alen;
    *buflen     =  len;

    return bfr;
}

/*-------------------------------------------------------------------*/

static BYTE* buffer_addstring( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE* str )
{
    size_t i;

    for (i=0; str[i] != 0; i++)
    {
        if (!(bfr = buffer_addchar( bfr, buflen, alloc_len, str[i] )))
            return NULL;
    }
    return bfr;
}

/*-------------------------------------------------------------------*/
/* 12 bit 3270 buffer address code conversion table                  */
/*-------------------------------------------------------------------*/
static BYTE sba_code[] =
{
    "\x40\xC1\xC2\xC3\xC4\xC5\xC6\xC7"
    "\xC8\xC9\x4A\x4B\x4C\x4D\x4E\x4F"
    "\x50\xD1\xD2\xD3\xD4\xD5\xD6\xD7"
    "\xD8\xD9\x5A\x5B\x5C\x5D\x5E\x5F"
    "\x60\x61\xE2\xE3\xE4\xE5\xE6\xE7"
    "\xE8\xE9\x6A\x6B\x6C\x6D\x6E\x6F"
    "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7"
    "\xF8\xF9\x7A\x7B\x7C\x7D\x7E\x7F"
};

static BYTE* buffer_addsba( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE line, BYTE col )
{
    unsigned int pos = (line * 80) + col;

    if (!(bfr = buffer_addchar( bfr, buflen, alloc_len, 0x11 )))
        return NULL;

    if (!(bfr = buffer_addchar( bfr, buflen, alloc_len, sba_code[ pos >> 6 ])))
        return NULL;

    if (!(bfr = buffer_addchar( bfr, buflen, alloc_len, sba_code[ pos & 0x3f ])))
        return NULL;

    return bfr;
}

/*-------------------------------------------------------------------*/

static BYTE* buffer_addsf( BYTE* bfr, size_t* buflen, size_t* alloc_len, BYTE attr )
{
    if (!(bfr = buffer_addchar( bfr, buflen, alloc_len, 0x1d )))
        return NULL;

    if (!(bfr = buffer_addchar( bfr, buflen, alloc_len, sba_code[ attr & 0x3f ])))
        return NULL;

    return bfr;
}

/*-------------------------------------------------------------------*/
/*  SUBROUTINE TO READ/SAVE THE HECULES LOGO DEFINITION STATEMENTS   */
/*-------------------------------------------------------------------*/
static void init_logo()
{
    const char*  p; /* pointer to logo filename */
    int          rc = 0;
    char         fn[FILENAME_MAX] = {0};

    if (sysblk.logofile == NULL) /* LogoFile NOT passed in command line */
    {
        if (!(p = get_symbol("HERCLOGO")) || !*p)
            p = "herclogo.txt";
    }
    else
        p = sysblk.logofile;

    hostpath( fn, p, sizeof(fn) );

    rc = readlogo(fn);

    if ( rc == -1 && strcasecmp(fn,basename(fn)) == 0
               && strlen(sysblk.hercules_pgmpath) > 0 )
    {
        char altfn[FILENAME_MAX];
        char pathname[MAX_PATH];

        memset(altfn, 0, sizeof(altfn));

        MSGBUF(altfn,"%s%c%s", sysblk.hercules_pgmpath, PATHSEPC, fn);

        hostpath( pathname, altfn, sizeof(pathname));

        rc = readlogo(pathname);
    }
}

/*-------------------------------------------------------------------*/
/*                 REDRIVE CONSOLE PSELECT                           */
/*-------------------------------------------------------------------*/
static void  constty_redrive_pselect( DEVBLK* dev )
{
    UNREFERENCED( dev );
    SIGNAL_CONSOLE_THREAD();
}
static void  loc3270_redrive_pselect( DEVBLK* dev )
{
    dev->readpending = 0;
    dev->rlen3270    = 0;
    SIGNAL_CONSOLE_THREAD();
}

/*-------------------------------------------------------------------*/
/*              Halt device / Clear Subchannel                       */
/*-------------------------------------------------------------------*/
static void constty_halt_or_clear( DEVBLK* dev )
{
    constty_redrive_pselect( dev );
}
static void loc3270_halt_or_clear( DEVBLK* dev )
{
    loc3270_redrive_pselect( dev );
}

/*-------------------------------------------------------------------*/
/*      Hercules Suspend/Resume text units for 3270 devices          */
/*-------------------------------------------------------------------*/
#define SR_DEV_3270_BUF          ( SR_DEV_3270 | 0x001 )
#define SR_DEV_3270_EWA          ( SR_DEV_3270 | 0x002 )
#define SR_DEV_3270_POS          ( SR_DEV_3270 | 0x003 )

/*-------------------------------------------------------------------*/
/*          Hercules Suspend Routine for 3270 devices                */
/*-------------------------------------------------------------------*/
static int  loc3270_hsuspend( DEVBLK* dev, void* file )
{
    size_t  rc, len;
    BYTE    buf[ BUFLEN_3270 ];

    if (!dev->connected)
        return 0;

    SR_WRITE_VALUE( file, SR_DEV_3270_POS, dev->pos3270, sizeof( dev->pos3270 ));
    SR_WRITE_VALUE( file, SR_DEV_3270_EWA, dev->ewa3270, 1);

    OBTAIN_DEVLOCK( dev );
    {
        rc = solicit_3270_data( dev, R3270_RB );

        if (1
            && rc == 0
            && dev->rlen3270 > 0
            && dev->rlen3270 <= sizeof( buf )
        )
        {
            len = dev->rlen3270;
            memcpy( buf, dev->buf, len );
        }
        else
            len = 0;
    }
    RELEASE_DEVLOCK( dev );

    if (len)
        SR_WRITE_BUF( file, SR_DEV_3270_BUF, buf, (int) len );

    return 0;
}

/*------------------------------------------------------------------ -*/
/*          Hercules Resume Routine for 3270 devices                */
/*-------------------------------------------------------------------*/
static int  loc3270_hresume( DEVBLK* dev, void* file )
{
    size_t  ewa3270, key, len;
    size_t  rbuflen = 0, pos = 0;

    BYTE*   rbuf = NULL;
    BYTE    buf[ BUFLEN_3270 ];

    do
    {
        SR_READ_HDR( file, key, len );

        switch (key)
        {
        case SR_DEV_3270_POS:

            SR_READ_VALUE( file, len, &pos, sizeof( pos ));
            break;

        case SR_DEV_3270_EWA:

            SR_READ_VALUE( file, len, &ewa3270, sizeof( ewa3270 ));
            dev->ewa3270 = (u_int) ewa3270;
            break;

        case SR_DEV_3270_BUF:

            rbuflen = len;
            rbuf    = malloc( len );

            if (rbuf == NULL)
            {
                char buf[40];
                MSGBUF( buf, "malloc(%d)", (int) len );
                // "%1d:%04X COMM: error in function %s: %s"
                WRMSG( HHC01000, "E", LCSS_DEVNUM, buf,
                      strerror( errno ));
                return 0;
            }

            SR_READ_BUF( file, rbuf, rbuflen );
            break;

        default:

            SR_READ_SKIP( file, len );
            break;
        }
    }
    while ((key & SR_DEV_MASK) == SR_DEV_3270);

    /* Dequeue any I/O interrupts for this device */
    DEQUEUE_IO_INTERRUPT( &dev->ioint );
    DEQUEUE_IO_INTERRUPT( &dev->pciioint );
    DEQUEUE_IO_INTERRUPT( &dev->attnioint );

    /* Restore the 3270 screen image if connected and buf was provided */
    if (1
        && dev->connected
        && rbuf
        && rbuflen > 3
    )
    {
        OBTAIN_DEVLOCK( dev );
        {
            /* Construct buffer to send to the 3270 */
            len = 0;
            buf[len++] = dev->ewa3270 ? R3270_EWA : R3270_EW;
            buf[len++] = 0xC2;

            memcpy( &buf[len], &rbuf[3], rbuflen - 3 );
            len += rbuflen - 3;

            buf[len++] = O3270_SBA;
            buf[len++] = rbuf[1];
            buf[len++] = rbuf[2];
            buf[len++] = O3270_IC;

            /* Restore the 3270 screen */
            sendto_client( dev->tn, buf, len );
            dev->pos3270 = (int) pos;
        }
        RELEASE_DEVLOCK( dev );
    }

    if (rbuf)
        free( rbuf );

    return 0;

} /* end function loc3270_hresume */

/*-------------------------------------------------------------------*/
/*            QUERY THE 1052/3215 DEVICE DEFINITION                  */
/*-------------------------------------------------------------------*/
static void constty_query_device( DEVBLK* dev, char** devclass,
                                  int buflen, char* buffer )
{
    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "CON", dev, devclass, buflen, buffer );

    if (dev->connected)
    {
        snprintf( buffer, buflen,

            "%s%s IO[%"PRIu64"]",

            inet_ntoa(dev->ipaddr),
            dev->prompt1052 ? "" : " noprompt",
            dev->excps );
    }
    else
    {
        char  acc[64];

        if (dev->acc_ipaddr || dev->acc_ipmask)
        {
            char  ip   [32];
            char  mask [32];
            struct in_addr  inaddr;

            inaddr.s_addr = dev->acc_ipaddr;
            MSGBUF( ip, "%s", inet_ntoa( inaddr ));

            inaddr.s_addr = dev->acc_ipmask;
            MSGBUF( mask, "%s", inet_ntoa( inaddr ));

            MSGBUF( acc, "%s mask %s", ip, mask );
        }
        else
            acc[0] = 0;

        if (filename[0])
        {
            snprintf( buffer, buflen,

                "GROUP=%s%s%s%s IO[%"PRIu64"]",

                filename,
                !dev->prompt1052 ? " noprompt" : "",
                acc[0] ? " " : "", acc,
                dev->excps );
        }
        else
        {
            if (acc[0])
            {
                if (!dev->prompt1052)
                    snprintf( buffer, buflen,

                        "noprompt %s IO[%"PRIu64"]",

                        acc, dev->excps );
                else
                    snprintf( buffer, buflen, "* %s", acc );
            }
            else
            {
                if (!dev->prompt1052)
                    snprintf( buffer, buflen,

                        "noprompt IO[%"PRIu64"]",

                        dev->excps );
                else
                    snprintf( buffer, buflen,

                        "IO[%"PRIu64"]",

                        dev->excps );
            }
        }
    }

} /* end function constty_query_device */

/*-------------------------------------------------------------------*/
/*             QUERY THE 3270 DEVICE DEFINITION                      */
/*-------------------------------------------------------------------*/
static void loc3270_query_device( DEVBLK* dev, char** devclass,
                                  int buflen, char* buffer )
{
    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "DSP", dev, devclass, buflen, buffer );

    if (dev->connected)
    {
        snprintf( buffer, buflen, "%s IO[%"PRIu64"]",
            inet_ntoa( dev->ipaddr ), dev->excps );
    }
    else
    {
        char  acc[64];

        if (dev->acc_ipaddr || dev->acc_ipmask)
        {
            char  ip   [32];
            char  mask [32];
            struct in_addr  inaddr;

            inaddr.s_addr = dev->acc_ipaddr;
            MSGBUF( ip, "%s", inet_ntoa( inaddr ));

            inaddr.s_addr = dev->acc_ipmask;
            MSGBUF( mask, "%s", inet_ntoa( inaddr ));

            MSGBUF( acc, "%s mask %s", ip, mask );
        }
        else
            acc[0] = 0;

        if (filename[0])
        {
            snprintf( buffer, buflen,
                "GROUP=%s%s%s IO[%"PRIu64"]",
                filename, acc[0] ? " " : "", acc, dev->excps );
        }
        else
        {
            if (acc[0])
            {
                snprintf( buffer, buflen,
                    "* %s IO[%"PRIu64"]", acc, dev->excps );
            }
            else
            {
                snprintf( buffer, buflen,
                    "* IO[%"PRIu64"]", dev->excps );
            }
        }
    }

} /* end function loc3270_query_device */

/*-------------------------------------------------------------------*/
/*                      Terminal Types Table                         */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The following table defines the display terminal types that we    */
/* support and their corresponding model and extended attributes     */
/* interpretation. For example, during terminal-type negotiations    */
/* the remote client identifies itself as a "IBM-3477-FC" terminal   */
/* type, then we know to interpret it as the equivalent of model 5   */
/* terminal type with 27 rows and 132 columns.                       */
/*                                                                   */
/*-------------------------------------------------------------------*/

struct TTTAB                            /* TN3270 Terminal Types     */
{
    const char*  ttype;                 /* Telnet Terminal Type      */
    BYTE         model;                 /* Model (2,3,4,5,X)         */
    BYTE         extatr;                /* Extended Attributes       */
};
typedef struct TTTAB   TTTAB;

// Ref: https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.halz001/telnetdevicestatement.htm
// Ref: https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_72/rzaiw/rzaiwemultypetrouble.htm
// Ref: https://www.iana.org/assignments/terminal-type-names/terminal-type-names.txt

static TTTAB  tttab[] =
{
    {  "IBM-DYNAMIC",     'X',   TRUE   },
    {  "IBM-3179-2",      '2',   FALSE  },
    {  "IBM-3180-2",      '5',   FALSE  },  // Yes, '5' !!
    {  "IBM-3277-2",      '2',   FALSE  },

    {  "IBM-3278-2",      '2',   FALSE  },
    {  "IBM-3278-3",      '3',   FALSE  },
    {  "IBM-3278-4",      '4',   FALSE  },
    {  "IBM-3278-5",      '5',   FALSE  },

    {  "IBM-3278-2-E",    '2',   TRUE   },
    {  "IBM-3278-3-E",    '3',   TRUE   },
    {  "IBM-3278-4-E",    '4',   TRUE   },
    {  "IBM-3278-5-E",    '5',   TRUE   },

    {  "IBM-3279-2",      '2',   FALSE  },
    {  "IBM-3279-3",      '3',   FALSE  },
    {  "IBM-3279-4",      '4',   FALSE  },
    {  "IBM-3279-5",      '5',   FALSE  },

    {  "IBM-3279-2-E",    '2',   TRUE   },
    {  "IBM-3279-3-E",    '3',   TRUE   },
    {  "IBM-3279-4-E",    '4',   TRUE   },
    {  "IBM-3279-5-E",    '5',   TRUE   },
};

/*-------------------------------------------------------------------*/
/*               Function to search above table                      */
/*-------------------------------------------------------------------*/
static bool lookup_ttype( const char* ttype, BYTE* model, BYTE* extatr )
{
    size_t i;
    for (i=0; i < _countof( tttab ); i++)
    {
        if (str_caseless_eq( tttab[i].ttype, ttype ))
        {
            *model  = tttab[i].model;
            *extatr = tttab[i].extatr;
            return true;
        }
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*                 Negotiate Client Terminal Type                    */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This subroutine negotiates the terminal type with the client      */
/* and uses the terminal type to determine whether the client        */
/* is to be supported as a 3270 display console or as a 1052/3215    */
/* printer-keyboard console.                                         */
/*                                                                   */
/* Terminal types whose first four characters are not "IBM-" are     */
/* handled as printer-keyboard consoles using telnet line mode.      */
/* Display terminals are always negotiated into telnet tn3270 mode.  */
/* The only supported 3270 printer type is "IBM-3287-1".             */
/*                                                                   */
/* Valid display terminal types are defined in the preceding table.  */
/* The model code defines the screen size, where 1=24x80, 2=24x80,   */
/* 3=32x80, 4=43x80, and 5=27x132.                                   */
/*                                                                   */
/* An optional device number suffix (example: IBM-3278@01F) may      */
/* be specified to request allocation to a specific device number.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static void negotiate_ttype( TELNET* tn )
{
    char* suffix;

    /* Check terminal type for optional device-number/group suffix */
    tn->devnum = 0xFFFF;
    if ((suffix = strchr( tn->ttype, '@' )) != NULL)
    {
        U16   devnum;
        char  c;

        *suffix++ = 0;  // (suffix is not part of terminal type)

        if (sscanf( suffix, "%hx%c", &devnum, &c ) == 1)
            tn->devnum = devnum;
        else if (strlen( suffix ) < sizeof( tn->tgroup ))
            STRLCPY( tn->tgroup, suffix );
    }

    /* Check for non-IBM terminal type (presumed TTY) */
    if (strncasecmp( tn->ttype, "IBM-", 4 ) != 0)
    {
        /* TTY: we don't care about BINARY or EOR mode. We can
           support it being either way (on or off) when their
           terminal type is a TTY, so don't bother asking them
           to disable or enable it. Whatever it's currently set
           to is fine with us.
        */
        tn->do_tn3270 = 0;
        tn->devclass = 'K';

        /* Request the client to go into line-at-a-time mode by telling
           the client we won't be echoing characters back to it and that
           it should instead do all echoing itself.  This should cause
           it to stop sending us one character at a time and instead to
           only send us complete lines always ending with a <CR><LF>.

           This has the desired side effect of also enabling both local
           echoing and local EDITING on the client side as well.  That
           is to say, if the user presses the backspace key, the client
           should erase the previous character from both the screen and
           from its local internal buffer as well.  Then when the enter
           key is then pressed, the line it sends to us is sent already
           without those characters so Hercules needs to do nothing.
        */
        telnet_negotiate( tn->ctl, TELNET_WONT, TELNET_TELOPT_ECHO );
        telnet_negotiate( tn->ctl, TELNET_DO,   TELNET_TELOPT_ECHO );
        return;
    }

    /* TN3270 mode requires both BINARY mode and END-OF-RECORD mode */
    tn->do_tn3270 = 1;
    tn->do_bin    = 1;
    tn->do_eor    = 1;

    telnet_negotiate( tn->ctl, TELNET_WILL, TELNET_TELOPT_BINARY );
    telnet_negotiate( tn->ctl, TELNET_DO,   TELNET_TELOPT_BINARY );
    telnet_negotiate( tn->ctl, TELNET_WILL, TELNET_TELOPT_EOR );
    telnet_negotiate( tn->ctl, TELNET_DO,   TELNET_TELOPT_EOR );

    /* No sense in going any further if TN3270 mode was refused */
    if (tn->neg_fail)
        return;

    /* Printer? */
    if (strcasecmp( tn->ttype, "IBM-3287-1" ) == 0)
    {
        tn->devclass = 'P';
        tn->model    = '5';     // (132 columns)
        tn->extatr   = FALSE;
        return;
    }

    /* Otherwise determine terminal type attributes from our table */
    if (lookup_ttype( tn->ttype, &tn->model, &tn->extatr ))
    {
        tn->devclass = 'D';
        return;
    }

    /* The current terminal type wasn't in our table and is therefore
       unacceptable to us. Tell them to try again by sending us their
       NEXT terminal type in their list. If NONE of the types in their
       list are acceptable to us (detected by their wrapping around
       and sending us the first entry in their list again), then fail
       the terminal type negotiations.
    */
    if (str_caseless_eq( tn->ttype, tn->tt1st ))  /* Tried them all? */
    {
        // "%s COMM: No acceptable terminal types"
        WRMSG( HHC02916, "E", tn->clientid );
        tn->neg_fail = TRUE;
        return;
    }

    // "%s COMM: unsupported terminal type: %s"
    WRMSG( HHC02910, "W", tn->clientid, tn->ttype );

    /* Save this terminal type if it's the first entry in their list */
    if (!tn->tt1st[0])
        STRLCPY( tn->tt1st, tn->ttype );

    /* Continue negotiating... */
    tn->ttype[0] = 0; // (prevent premature 'connect_client' loop exit)

    /* Tell them to send us the NEXT terminal type in their list... */
    telnet_ttype_send( tn->ctl );

} /* end function negotiate_ttype */

/*-------------------------------------------------------------------*/
/*                3270 TELNET_EV_DATA event handler                  */
/*-------------------------------------------------------------------*/
static void loc3270_input( TELNET* tn, const BYTE* buffer, U32 size )
{
    DEVBLK *dev;
    U32     amt;

    /* Discard client data until negotiations are complete */
    if (!tn->neg_done)
    {
        // "%s COMM: discarding premature data"
        WRMSG( HHC02911, "W", tn->clientid );
        return;
    }

    /* Initialize DEVBLK pointer */
    dev = tn->dev;

    /* Calculate amount to copy */
    amt = min( dev->bufsize - dev->rlen3270, size );

    /* Copy received data into device buffer */
    memcpy( dev->buf + dev->rlen3270, buffer, amt );

    /* Update number of bytes in receive buffer */
    dev->rlen3270 += amt;

    /* Check for buffer overflow */
    if (size > amt)
    {
        char clientid[32];
        MSGBUF( clientid, "%1d:%04X",
            LCSS_DEVNUM );
        // "%s COMM: buffer overflow"
        WRMSG( HHC02912, "E", clientid );
        tn->overflow = TRUE;
        dev->rlen3270 = 0;
    }
}

/*-------------------------------------------------------------------*/
/*            console/tty TELNET_EV_DATA event handler               */
/*-------------------------------------------------------------------*/
static void constty_input( TELNET* tn, const BYTE* buffer, U32 size )
{
    DEVBLK *dev;
    U32 i;

    /* Discard client data until negotiations are complete */
    if (!tn->neg_done)
    {
        // "%s COMM: discarding premature data"
        WRMSG( HHC02911, "W", tn->clientid );
        return;
    }

    /* Initialize DEVBLK pointer */
    dev = tn->dev;

    /* Check for break indication (IAC BRK or IAC IP) */
    if (tn->got_break)
    {
        dev->keybdrem = 0;
        return;
    }

    /* Copy received bytes to keyboard buffer */
    for (i=0; i < size; i++)
    {
        /* Check for buffer overflow */
        if (dev->keybdrem >= dev->bufsize)
        {
            char clientid[32];
            MSGBUF( clientid, "%1d:%04X",
                LCSS_DEVNUM );
            // "%s COMM: buffer overflow"
            WRMSG( HHC02912, "E", clientid );
            tn->overflow = TRUE;
            dev->keybdrem = 0;
            return;
        }

        /* Copy character to keyboard buffer */
        dev->buf[ dev->keybdrem++ ] = buffer[i];

        /* Check for carriage return */
        if (1
            &&           dev->keybdrem >= 2
            && dev->buf[ dev->keybdrem  - 2 ] == '\r'
            && dev->buf[ dev->keybdrem  - 1 ] == '\0'
        )
        {
            dev->keybdrem = 0;
            continue;
        }

        /* Check for CRLF (end of record) */
        if (1
            &&           dev->keybdrem >= 2
            && dev->buf[ dev->keybdrem  - 2 ] == '\r'
            && dev->buf[ dev->keybdrem  - 1 ] == '\n'
        )
        {
            tn->got_eor = TRUE;
            break;
        }

    } /* end for(i) */

    /* Check for unexpected data following CRLF */
    if (tn->got_eor && i < (size - 1))
    {
        char clientid[32];
        MSGBUF( clientid, "%1d:%04X",
            LCSS_DEVNUM );
        // "%s COMM: buffer overrun"
        WRMSG( HHC02913, "E", clientid );
        tn->overrun = TRUE;
        dev->keybdrem = 0;
    }

} /* end function constty_input */

/*-------------------------------------------------------------------*/
/*       SUBROUTINE TO RECEIVE 3270 DATA FROM THE CLIENT             */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The caller MUST hold the device lock.                             */
/*                                                                   */
/* This subroutine receives bytes from the client and appends them   */
/* to any data already in the 3270 receive buffer.                   */
/*                                                                   */
/* If zero bytes are received, this means the client has closed the  */
/* connection, and attention and unit check status is returned.      */
/*                                                                   */
/* If the buffer is filled before receiving end of record, then      */
/* attention and unit check status is returned.                      */
/*                                                                   */
/* If the data accumulated in the buffer does not yet constitute a   */
/* complete record, then zero status is returned, and a further      */
/* call must be made to this subroutine when more data is available. */
/*                                                                   */
/* Otherwise a normal attention status is returned to indicate data  */
/* is available to be read from the device.                          */
/*                                                                   */
/*-------------------------------------------------------------------*/
static BYTE  recv_3270_data( DEVBLK* dev )
{
int     rc;                             /* Return code               */
BYTE    buf[ BUFLEN_3270 ];             /* Temporary recv() buffer   */

    /* If there is a complete data record already in the buffer
       then discard it before reading more data */
    if (dev->readpending)
    {
        dev->rlen3270 = 0;
        dev->readpending = 0;
    }

    dev->tn->got_eor  = FALSE;
    dev->tn->neg_fail = FALSE;
    dev->tn->send_err = FALSE;
    dev->tn->overflow = FALSE;

    /* Read bytes from client */
    rc = recv( dev->fd, buf, sizeof( buf ), 0 );

    /* Check for I/O error */
    if (rc < 0)
    {
        if (HSO_ECONNRESET == HSO_errno)
            // "%1d:%04X COMM: client %s devtype %4.4X: connection reset"
            WRMSG( HHC01090, "I", LCSS_DEVNUM,
                  inet_ntoa(dev->ipaddr), dev->devtype );
        else
            // "%s COMM: recv() failed: %s"
            CONERROR( HHC90507, "D", dev->tn->clientid, strerror( HSO_errno ));
        if (HSO_EAGAIN == HSO_errno)
            // non blocking call and no data
            return 0;
        else
        {
            dev->sense[0] = SENSE_EC;
            return (CSW_ATTN | CSW_UC);
        }
    }

    /* If zero bytes were received then client has closed connection */
    if (rc == 0)
    {
        // "%1d:%04X COMM: client %s devtype %4.4X: connection closed by client"
        WRMSG( HHC01022, "I", LCSS_DEVNUM,
              inet_ntoa(dev->ipaddr), dev->devtype );
        dev->sense[0] = SENSE_IR;
        return (CSW_ATTN | CSW_UC | CSW_DE);
    }

    // "%s COMM: received %d bytes"
    CONDEBUG2( HHC90501, "D", dev->tn->clientid, rc );
    DUMPBUF(   HHC90501, buf, rc, 1 );

    /* Pass received bytes to libtelnet for processing */
    telnet_recv( dev->tn->ctl, buf, rc );

    /* Check results for any errors */
    if (0
        || dev->tn->neg_fail
        || dev->tn->send_err
        || dev->tn->overflow
    )
    {
        dev->sense[0] = SENSE_DC;
        return (CSW_ATTN | CSW_UC);
    }

    /* Return zero status if record is incomplete */
    if (!dev->tn->got_eor)
        return 0;

    /* Set the read pending indicator and return attention status */
    dev->readpending = 1;
    return (CSW_ATTN);

} /* end function recv_3270_data */

/*-------------------------------------------------------------------*/
/*        SUBROUTINE TO SOLICIT 3270 DATA FROM THE CLIENT            */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The caller MUST hold the device lock.                             */
/*                                                                   */
/* This subroutine is called by loc3270_execute_ccw as a result of   */
/* processing a Read Buffer CCW, or as a result of processing a Read */
/* Modified CCW when no data is waiting in the 3270 read buffer.     */
/*                                                                   */
/* This subroutine sends a Read or Read Modified command to the      */
/* client and then receives the data into the 3270 receive buffer,   */
/* waiting until the client sends end of record.                     */
/*                                                                   */
/* TN3270 clients that fail to flush their buffer until the user     */
/* first presses an attention key are not supported since such       */
/* behavior causes this routine to hang.                             */
/*                                                                   */
/* Since this routine is only called while a channel program is      */
/* active on the device, we can rely on the dev->busy flag to        */
/* prevent the connection thread from issuing a read and capturing   */
/* the incoming data intended for this routine.                      */
/*                                                                   */
/* Returns zero status if successful, or unit check if error.        */
/*                                                                   */
/*-------------------------------------------------------------------*/
static BYTE  solicit_3270_data( DEVBLK* dev, BYTE cmd )
{
    int prev_rlen3270;
    BYTE status;

    /* Clear the inbound buffer of any unsolicited
       data accumulated by the connection thread */
    dev->rlen3270    = 0;
    dev->readpending = 0;
    dev->tn->got_eor = FALSE;

    /* Send the 3270 read command to the client */

    telnet_3270_cmd( dev->tn->ctl, cmd );

    if (dev->tn->send_err)
    {
        /* Close the connection if an error occurred */
        disconnect_console_device( dev );
        dev->sense[0] = SENSE_DC;
        return (CSW_UC);
    }

    /* Receive response data from the client */
    do
    {
        prev_rlen3270 = dev->rlen3270;
        status = recv_3270_data( dev );

        // "%s COMM: recv_3270_data: %d bytes received"
        CONDEBUG2( HHC90502, "D", dev->tn->clientid,
            dev->rlen3270 - prev_rlen3270 );
    }
    while (status == 0);

    /* Close the connection if an error occurred */
    if (status & CSW_UC)
    {
        disconnect_console_device( dev );
        dev->sense[0] = SENSE_DC;
        return (CSW_UC);
    }

    /* Return zero status to indicate response received */
    return 0;

} /* end function solicit_3270_data */

/*-------------------------------------------------------------------*/
/*     SUBROUTINE TO RECEIVE 1052/3215 DATA FROM THE CLIENT          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This subroutine receives keyboard input characters from the       */
/* client, and appends the characters to any data already in the     */
/* keyboard buffer.                                                  */
/*                                                                   */
/* If zero bytes are received, this means the client has closed the  */
/* connection, and attention and unit check status is returned.      */
/*                                                                   */
/* If the buffer is filled before receiving end of record, then      */
/* attention and unit check status is returned.                      */
/*                                                                   */
/* If a break indication (control-C, IAC BRK, or IAC IP) is          */
/* received, the attention and unit exception status is returned.    */
/*                                                                   */
/* If CRLF has not yet been received, then zero status is returned,  */
/* and a further call must be made to this subroutine when more      */
/* data is available.                                                */
/*                                                                   */
/* Otherwise the CRLF is discarded, the data in the keyboard buffer  */
/* is translated to EBCDIC and attention status is returned.         */
/*                                                                   */
/*-------------------------------------------------------------------*/
static BYTE  recv_1052_data( DEVBLK* dev )
{
int     num;                            /* Number of bytes received  */
BYTE    buf[BUFLEN_1052];               /* Receive buffer            */

    dev->tn->got_eor   = FALSE;
    dev->tn->got_break = FALSE;
    dev->tn->neg_fail  = FALSE;
    dev->tn->send_err  = FALSE;
    dev->tn->overflow  = FALSE;
    dev->tn->overrun   = FALSE;

    /* Read bytes from client */
    num = recv( dev->fd, buf, sizeof( buf ), 0 );

    /* Return unit check if error on receive */
    if (num < 0)
    {
        // "%s COMM: recv() failed: %s"
        CONERROR( HHC90507, "D", dev->tn->clientid, strerror( HSO_errno ));

        if (HSO_EAGAIN == HSO_errno)
            // non blocking call and no data
            return 0;
        else
        {
            dev->sense[0] = SENSE_EC;
            return (CSW_ATTN | CSW_UC);
        }
    }

    /* If zero bytes were received then client has closed connection */
    if (num == 0)
    {
        WRMSG( HHC01022, "I", LCSS_DEVNUM,
               inet_ntoa(dev->ipaddr), dev->devtype );
        dev->sense[0] = SENSE_IR;
        return (CSW_ATTN | CSW_UC);
    }

    // "%s COMM: received %d bytes"
    CONDEBUG2( HHC90501, "D", dev->tn->clientid, num );
    DUMPBUF(   HHC90501, buf, num, 0 );

    /* Pass received data to libtelnet for processing */
    telnet_recv( dev->tn->ctl, buf, num );

    /* Check results for any errors */
    if (0
        || dev->tn->neg_fail
        || dev->tn->send_err
        || dev->tn->overflow
    )
    {
        dev->keybdrem = 0;
        dev->sense[0] = SENSE_EC;       /* Equipment Check */
        return (CSW_ATTN | CSW_UC);
    }

    if (dev->tn->overrun)
    {
        dev->keybdrem = 0;
        dev->sense[0] = SENSE_OR;       /* Overrun */
        return (CSW_ATTN | CSW_UC);
    }

    /* If a break indication (Ctrl-C, IAC BRK, or IAC IP) is received,
       then attention and unit exception status is returned.
    */
    if (dev->tn->got_break)
    {
        dev->tn->got_break = FALSE;
        dev->keybdrem = 0;
        return (CSW_ATTN | CSW_UX);
    }

    /* Return zero status if CRLF was not yet received */
    if (!dev->tn->got_eor)
        return 0;

    /* Strip off the CRLF sequence */
    dev->keybdrem -= 2;

    /* Translate the keyboard buffer to EBCDIC */
    prt_host_to_guest( dev->buf, dev->buf, dev->keybdrem );

    /* Return attention status */
    return (CSW_ATTN);

} /* end function recv_1052_data */

/*-------------------------------------------------------------------*/
/*               NEW CLIENT CONNECTION THREAD                        */
/*-------------------------------------------------------------------*/
static void* connect_client (void* pArg)
{
TELNET                 *tn;             /* Telnet Control Block      */
telnet_t               *ctl;            /* libtelnet state tracker   */
int                     csock;          /* Client socket             */

int                     rc;             /* Return code               */
DEVBLK                 *dev;            /* -> Device block           */
size_t                  len = 0;        /* Data length               */

struct sockaddr_in      client;         /* Client address structure  */
socklen_t               namelen;        /* Length of client structure*/
char                   *clientip;       /* Addr of client ip address */
char                    buf[1920];      /* Work buffer               */
char                    orig_cid[32];   /* e.g. "client 123"         */

char                    hvermsg[256];   /* "Hercules version ..."    */
char                    hostmsg[256];   /* "Running on..."           */
char                    procsmsg[256];  /* #of processors string     */

char                    rejmsg[256];    /* Rejection message         */
char                    devmsg[256];    /* Device message            */

BYTE                   *logobfr;        /* Constructed logo screen   */
size_t                  logoheight;     /* Logo file number of lines */

    /* Initialize some local variables */
    tn    = (TELNET*) pArg;
    csock = tn->csock;
    ctl   = tn->ctl;

    // "%s COMM: connection received"
    WRMSG( HHC02915, "I", tn->clientid );

    // "Hercules version %s, built on %s %s"
    // (Note: "MSG_C" used so message does NOT end with newline)
    MSGBUF( hvermsg, MSG_C( HHC01027, "I", VERSION, __DATE__, __TIME__ ));

    // "LP=%d, Cores=%d, CPUs=%d",
    // or "MP=%d", or "UP" or ""
    if (1
        && hostinfo.num_packages      !=  0
        && hostinfo.num_physical_cpu  !=  0
        && hostinfo.num_logical_cpu   !=  0
    )
    {
        MSGBUF( procsmsg, "LP=%d, Cores=%d, CPUs=%d"
            , hostinfo.num_logical_cpu
            , hostinfo.num_physical_cpu
            , hostinfo.num_packages
        );
    }
    else
    {
        if (hostinfo.num_procs > 1)
            MSGBUF(  procsmsg, "MP=%d", hostinfo.num_procs );
        else if (hostinfo.num_procs == 1)
            STRLCPY( procsmsg, "UP" );
        else
            STRLCPY( procsmsg, "" );
    }

    // "Running on %s (%s-%s.%s %s %s)"
    // (Note: "MSG_C" used so message does NOT end with newline)
    MSGBUF( hostmsg, MSG_C( HHC01031, "I"
        , hostinfo.nodename
        , hostinfo.sysname
        , hostinfo.release
        , hostinfo.version
        , hostinfo.machine
        , procsmsg)
    );

    /* Obtain the client's IP address */
    namelen = sizeof( client );
    rc = getpeername( csock, (struct sockaddr*) &client, &namelen );

    // "%s COMM: received connection from %s"
    clientip = strdup( inet_ntoa( client.sin_addr ));
    CONDEBUG1( HHC90505, "D", tn->clientid, clientip );

    /* Dr. Hans-Walter Latz -- binary xfer mode performance tweak
       See: http://www.unixguide.net/network/socketfaq/2.16.shtml
    */
    if (disable_nagle(csock) < 0)
        // "%s COMM: setsockopt() failed: %s"
        CONERROR( HHC90510, "D", tn->clientid, strerror( HSO_errno ));

    /* Determine client's terminal type by reading from the socket
       and passing all data to libtelnet so it can pass it to our
       telnet event handler until we eventually learn their TTYPE.
       Note that during this process any non-telnet client data we
       may receive is discarded by our event handler as premature;
       we shouldn't be receiving data from the client until we ask
       for it which we never do until negotiations are complete.
    */
    do
    {
        /* Read client telnet negotiation data */
        if ((rc = recv( csock, buf, sizeof( buf ), 0 )) > 0)
        {
            // "%s COMM: received %d bytes"
            CONDEBUG2( HHC90501, "D", tn->clientid, rc );
            DUMPBUF(   HHC90501, buf, rc, tn->do_tn3270 ? 1 : 0 );

            /* Pass data to libtelnet to pass to our event handler */
            telnet_recv( ctl, (BYTE*) buf, rc );
        }
        else /* recv() error or connection closed */
        {
            if (rc == 0)
                // "%s COMM: connection closed during negotiations"
                WRMSG( HHC02908, "E", tn->clientid );
            else
                // "%s COMM: recv() error during negotiations: %s"
                WRMSG( HHC02909, "E", tn->clientid, strerror( errno ));

            /* Discard client connection */
            disconnect_telnet_client( tn );
            if (clientip)
                free( clientip );
            return NULL;
        }
    }
    while (1
        && !tn->ttype[0]        /* (what we're really interested in) */
        && !tn->neg_fail
        && !tn->send_err
        && !tn->overflow
    );

    /* Abort connection on error */
    if (0
        || tn->neg_fail
        || tn->send_err
        || tn->overflow
    )
    {
        /* (error message already issued) */
        disconnect_telnet_client( tn );
        if (clientip)
            free( clientip );
        return NULL;
    }

    /* Look for an available console device */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        if (sysblk.sysgport)
        {
            /* Loop if they want the SYSG device and this isn't it,
               or they DON'T want the SYSG device and this IS it. */
            if (0
                || ( tn->sysg && dev != sysblk.sysgdev)
                || (!tn->sysg && dev == sysblk.sysgdev)
            )
                continue;
        }

        /* Loop if the device is invalid */
        if ( !dev->allocated )
            continue;

        /* Loop if non-matching device type */
        if (tn->devclass == 'D' && dev->devtype != 0x3270)
            continue;

        if (tn->devclass == 'P' && dev->devtype != 0x3287)
            continue;

        if (tn->devclass == 'K' && dev->devtype != 0x1052
                                && dev->devtype != 0x3215)
            continue;

        /* Loop if a specific device number was requested and
           this device is not the requested device number */
        if (tn->devnum != 0xFFFF && dev->devnum != tn->devnum)
            continue;  /* (not our device) */

        /* Loop if no specific device number was requested, and
           either a group was requested OR the device is in a group,
           and the device group does not match the requested group */
        if (tn->devnum == 0xFFFF && (tn->tgroup[0] || dev->filename[0]))
        {
            if (strncasecmp( tn->tgroup, dev->filename, sizeof( tn->tgroup )) != 0)
                continue;  /* (not our terminal group) */
        }

        OBTAIN_DEVLOCK( dev );
        {
            /* Test for available device */
            if (dev->connected != 0)
            {
                /* Release the device lock */
                RELEASE_DEVLOCK( dev );
                continue;
            }

            /* Check ipaddr mask to see if client allowed on this device */
            if ((client.sin_addr.s_addr & dev->acc_ipmask) != dev->acc_ipaddr)
            {
                RELEASE_DEVLOCK( dev );         /* Release device lock   */
                if (0xFFFF == tn->devnum )      /* Non-specific devnum?  */
                    continue;                   /* Then keep looking     */
                dev = NULL;                     /* Else specific devnum, */
                break;                          /* but it's unavailable  */
            }

            /* --- WE FOUND OUR DEVBLK --- */

            /* Claim this device for the client */
            tn->dev = dev;      /* tn  -->  dev */
            dev->tn = tn;       /* tn  <--  dev */

            STRLCPY( orig_cid, tn->clientid );
            MSGBUF( tn->clientid, "%1d:%04X", LCSS_DEVNUM );
            // "%s COMM: %s negotiations complete; ttype = '%s'"
            WRMSG( HHC02914, "I", tn->clientid, orig_cid, tn->ttype );

            dev->connected = 1;
            dev->fd        = csock;
            dev->ipaddr    = client.sin_addr;
            dev->mod3270   = tn->model;
            dev->eab3270   = tn->extatr ? 1 : 0;

            /* Reset the console device */
            dev->readpending = 0;
            dev->rlen3270    = 0;
            dev->keybdrem    = 0;
            dev->tn->got_eor = FALSE;

            memset( &dev->scsw,    0, sizeof(SCSW) );
            memset( &dev->pciscsw, 0, sizeof(SCSW) );

            dev->busy = dev->reserved = dev->suspended =
            dev->pending = dev->pcipending = dev->attnpending = 0;
        }
        RELEASE_DEVLOCK( dev );
        break;

    } /* end for(dev) */

    /* Reject the connection if no available console device */
    if (dev == NULL)
    {
        /* Build rejection message to log to HMC */
        if (tn->devnum == 0xFFFF)
        {
            if (!tn->tgroup[0])
            {
                // "Connection rejected, no available %s device"
                // (Note: "MSG_C" used so message does NOT end with newline)
                MSGBUF( rejmsg, MSG_C( HHC01028, "E",
                    (tn->devclass == 'D' ? "3270" :
                    (tn->devclass == 'P' ? "3287" : "1052 or 3215"))));
            }
            else
            {
                // "Connection rejected, no available %s device in the %s group"
                // (Note: "MSG_C" used so message does NOT end with newline)
                MSGBUF( rejmsg, MSG_C( HHC01029, "E",
                    (tn->devclass == 'D' ? "3270" :
                    (tn->devclass == 'P' ? "3287" : "1052 or 3215")), tn->tgroup));
            }
        }
        else
        {
            // "Connection rejected, device %04X unavailable"
            // (Note: "MSG_C" used so message does NOT end with newline)
            MSGBUF( rejmsg, MSG_C( HHC01030, "I", tn->devnum ));
        }

        /* Log rejection message to HMC */
        LOGMSG( "%s\n", rejmsg );

        /* Build rejection message to send back to client */
        if (tn->devclass == 'D')
        {
            len = MSGBUF
            (
                buf,

                "\xF5\x40"
                "\x11\x40\x40\x1D\x60%s"        /* First line   */
                "\x11\xC1\x50\x1D\x60%s"        /* Second line  */
                "\x11\xC2\x60\x1D\x60%s",       /* Third line   */

                prt_host_to_guest( (BYTE*) hvermsg, (BYTE*) hvermsg, strlen( hvermsg )),
                prt_host_to_guest( (BYTE*) hostmsg, (BYTE*) hostmsg, strlen( hostmsg )),
                prt_host_to_guest( (BYTE*) rejmsg,  (BYTE*) rejmsg,  strlen( rejmsg  ))
            );
        }
        else if (tn->devclass == 'K')
        {
            len = MSGBUF
            (
                buf,

                "%s\n"          /* First line   */
                "%s\n"          /* Second line  */
                "%s\n",         /* Third line   */

                hvermsg,
                hostmsg,
                rejmsg
            );
        }

        /* Send the connection rejection message back to the client */
        if (tn->devclass != 'P')
            sendto_client( tn, (BYTE*) buf, len );

        /* Give them time to read the message before disconnecting */
        SLEEP(10);

        /* Close the connection and terminate the thread */
        disconnect_telnet_client( tn );
        if (clientip)
            free( clientip );
        return NULL;
    }

    // "%1d:%04X COMM: client %s devtype %4.4X: connected"
    // (Note: "MSG_C" used so message does NOT end with newline)
    MSGBUF( devmsg, MSG_C( HHC01018, "I",
              LCSS_DEVNUM, clientip, dev->devtype ));
    LOGMSG( "%s\n", devmsg );

    /* Negotiations are complete; we can begin accepting data now */
    tn->neg_done = TRUE;

    /* Initialize size of send buffer for one shot send operations */
    tn->sendbuf_size = 0;

    /* Try to detect dropped connections */
    set_socket_keepalive( csock, sysblk.kaidle, sysblk.kaintv, sysblk.kacnt );

    /* Construct Hercules logo or connected message */

    logobfr = NULL;

    if (tn->devclass == 'D')
    {
        /* Construct display terminal logo */

#if defined( _FEATURE_INTEGRATED_3270_CONSOLE )
        if  (dev == sysblk.sysgdev)
            STRLCPY( buf, "SYSG" );
        else
#endif
        MSGBUF( buf, "%4.4X", dev->devnum );           set_symbol( "DEVN",    buf );
                                                       set_symbol( "CCUU",    buf );
        MSGBUF( buf, "%3.3X", dev->devnum );           set_symbol( "CUU",     buf );
        MSGBUF( buf, "%4.4X", dev->subchan );          set_symbol( "SUBCHAN", buf );
        MSGBUF( buf, "%d", SSID_TO_LCSS( dev->ssid )); set_symbol( "CSS",     buf );

        if (sysblk.herclogo != NULL)
        {
            logobfr = build_logo( sysblk.herclogo, sysblk.logolines, &len, NULL );
        }
        else // (use hard coded built-in default logo)
        {
            logoheight = sizeof( herclogo ) / sizeof( char* );
            logobfr    = build_logo( herclogo, logoheight, &len, NULL );
        }
    }
    else if (tn->devclass == 'K')
    {
        /* Construct keyboard device connected message */

        len = MSGBUF
        (
            buf,                /* (just a convenient buffer) */

            "%s\n"              /* First line   */
            "%s\n"              /* Second line  */
            "%s\n",             /* Third line   */

            hvermsg,
            hostmsg,
            devmsg
        );
        logobfr = (BYTE*) buf;
    }

    /* Send the Hercules logo or connected message to the client */
    if (tn->devclass != 'P' && logobfr)
        sendto_client( tn, logobfr, len );

    if (logobfr && logobfr != (BYTE*) buf)
        free( logobfr );

    if (clientip)
        free( clientip );

    /* Raise attention interrupt for the device */
    raise_device_attention( dev, CSW_DE );

    /* Signal connection thread to redrive its pselect loop */
    SIGNAL_CONSOLE_THREAD();

    return NULL;

} /* end function connect_client */

/*-------------------------------------------------------------------*/
/*        Automatic pselect timeout adjustment for IND$FILE          */
/*-------------------------------------------------------------------*/

struct timespec     slowpoll    = { 0, 100000000 };         /* 100ms */
struct timespec     fastpoll    = { 0,   1000000 };         /*   1ms */
struct timespec*    timeout     = &slowpoll;       /* default = slow */

struct timeval  previo = {0,0};              /* TOD of last read I/O */
struct timeval  currio, diffio;              /* Work variables       */

static void calcto()    // (calculate timeout)
{
    struct timespec* prev_timeout = timeout;
    gettimeofday( &currio, NULL );
    timeval_subtract( &previo, &currio, &diffio );
    timeout = (diffio.tv_sec  > 0 || diffio.tv_usec > 100000) ?
        &slowpoll : &fastpoll;
    if (MLVL( DEBUG ))
        if (timeout != prev_timeout)
            WRMSG( HHC02917, "D", &slowpoll == prev_timeout ?
                "slow to FAST" : "FAST to slow" );
}

static void consio()    // (console read)
{
    calcto();
    memcpy( &previo, &currio, sizeof( struct timeval ));
}

static void consto()    // (select timeout)
{
    calcto();
}

/*-------------------------------------------------------------------*/
/*      Obtain a new console_connection_handler listening socket     */
/*-------------------------------------------------------------------*/
static int get_listening_socket( const char* stmt, const char* typ, const char* port )
{
int                    rc = 0;          /* Return code               */
int                    lsock;           /* Socket for listening      */
int                    optval;          /* Argument for setsockopt   */
struct sockaddr_in    *server;          /* Server address structure  */

    /* Obtain a socket */
    lsock = socket( AF_INET, SOCK_STREAM, 0 );

    if (lsock < 0)
    {
        // "COMM: error in function %s: %s"
        WRMSG( HHC01034, "E", "socket()", strerror( HSO_errno ));
        return -1;
    }

    /* Allow previous instance of socket to be reused */
    optval = 1;
    rc = setsockopt( lsock, SOL_SOCKET, SO_REUSEADDR,
                     (GETSET_SOCKOPT_T*) &optval, sizeof( optval ));
    if (rc < 0)
    {
        // "COMM: error in function %s: %s"
        WRMSG( HHC01034, "E", "setsockopt()", strerror( HSO_errno ));
        close_socket( lsock );
        return -1;
    }

    /* Prepare the sockaddr structure for the bind */
    if (!(server = parse_sockspec( port )))
    {
        char msgbuf[64];
        MSGBUF( msgbuf, "%s = %s", stmt, port );
        // "COMM: invalid parameter %s"
        WRMSG( HHC01017, "E", msgbuf );
        close_socket( lsock );
        return -1;
    }

    /* Attempt to bind the socket to the port */
    do
    {
        rc = bind( lsock, (struct sockaddr*) server,
            sizeof( struct sockaddr_in ));

        if (rc == 0 || HSO_errno != HSO_EADDRINUSE)
            break;

        // "Waiting for port %u to become free for console connections"
        WRMSG( HHC01023, "W", ntohs( server->sin_port ));
        SLEEP( 10 );
    }
    while (console_cnslcnt > 0);

    if (rc != 0)
    {
        // "COMM: error in function %s: %s"
        WRMSG( HHC01034, "E", "bind()", strerror( HSO_errno ));
        free( server );
        close_socket( lsock );
        return -1;
    }

    /* Put the socket into listening state */
    if ((rc = listen ( lsock, 10 )) < 0)
    {
        // "COMM: error in function %s: %s"
        WRMSG( HHC01034, "E", "listen()", strerror( HSO_errno ));
        free( server );
        close_socket( lsock );
        return -1;
    }

    // "Waiting for %sconsole connections on port %u"
    WRMSG( HHC01024, "I", typ, ntohs( server->sin_port ));
    free( server );
    return lsock;
}

/*-------------------------------------------------------------------*/
/*        CONSOLE CONNECTION AND ATTENTION HANDLER THREAD            */
/*-------------------------------------------------------------------*/
static void* console_connection_handler( void* arg )
{
int           rc = 0;                   /* Return code               */
int           lsock;                    /* Console listening socket  */
int           lsock2 = 0;               /* SYSG listening socket     */
int           csock;                    /* Socket for conversation   */
bool          sysg = false;             /* SYSG port connection      */
fd_set        readset;                  /* Read bit map for pselect  */
int           maxfd;                    /* Highest fd for pselect    */
int           scan_complete;            /* DEVBLK scan complete      */
int           scan_retries;             /* DEVBLK scan retries       */
TID           tidneg;                   /* Negotiation thread id     */
DEVBLK*       dev;                      /* -> Device block           */
BYTE          unitstat;                 /* Status after receive data */
TELNET*       tn;                       /* Telnet Control Block      */
const char*   curr_cnslport;            /* Current sysblk.cnslport   */
const char*   curr_sysgport = NULL;     /* Current sysblk.sysgport   */

int prev_rlen3270;

    UNREFERENCED( arg );

    /* Set server thread priority; ignore any errors */
    SET_THREAD_PRIORITY( sysblk.srvprio, sysblk.qos_user_interactive );

    // "Thread id "TIDPAT", prio %2d, name %s started"
    LOG_THREAD_BEGIN( CON_CONN_THREAD_NAME  );

    /* Get information about this system */
    init_hostinfo( NULL );

    /* If logo hasn't been built yet, build it now */
    if (sysblk.herclogo == NULL)
        init_logo();

    /* Save starting sysblk.cnslport value
       and create starting listening socket */
    curr_cnslport = strdup( sysblk.cnslport );
    lsock = get_listening_socket( "CNSLPORT", "", sysblk.cnslport );

    if (sysblk.sysgport)
    {
        /* Save starting sysblk.sysgport value
           and create starting listening socket */
        curr_sysgport = strdup( sysblk.sysgport );
        lsock2 = get_listening_socket( "SYSGPORT", "SYSG ", sysblk.sysgport );
    }

    /* Handle connection requests and attention interrupts */
    while (console_cnslcnt > 0)
    {
        /* Did they set a new CNSLPORT value? */
        if (strcmp( curr_cnslport, sysblk.cnslport ) != 0)
        {
            /* Close the current listening socket, save
               the new CNSLPORT value and obtain a fresh
               listening socket. */
            close_socket( lsock );
            free( curr_cnslport );
            curr_cnslport = strdup( sysblk.cnslport );
            lsock = get_listening_socket( "CNSLPORT", "", sysblk.cnslport );
        }

        /* Did they set a new SYSGPORT value? */
        if (0
            || (sysblk.sysgport && !curr_sysgport)
            || (sysblk.sysgport && strcmp( curr_sysgport, sysblk.sysgport ) != 0)
        )
        {
            /* Close the current listening socket, save
               the new SYSGPORT value and obtain a fresh
               listening socket. */
            if (lsock2) close_socket( lsock2 );
            free( curr_sysgport );
            curr_sysgport  = strdup( sysblk.sysgport );
            lsock2 = get_listening_socket( "SYSGPORT", "SYSG ", sysblk.sysgport );
        }

        /* Initialize scan flags */
        scan_complete = TRUE;
        scan_retries = 0;

        /* Build pselect() read set */
        for (;;)
        {
            /* Initialize the pselect parameters */
            FD_ZERO( &readset );

            FD_SET( lsock, &readset );  // (normal local 3270 devices)
            maxfd = lsock;              // (normal local 3270 devices)

            /* If SYSGPORT defined and SYSG not connected yet... */
            if (lsock2 && sysblk.sysgdev && !sysblk.sysgdev->connected)
            {
                FD_SET( lsock2, &readset );     // (add SYSG port too)
                maxfd = MAX( lsock, lsock2 );   // (then adjust maxfd)
            }

            SUPPORT_WAKEUP_CONSOLE_SELECT_VIA_PIPE( maxfd, &readset );

            /* FIXME: Incorrectly running chain that may have a DEVBLK
             *        removed while in flight. For example, a device may
             *        be deleted between the loading of the nextdev and
             *        before the check for dev != NULL. This will result
             *        in an invalid storage reference and may result in
             *        a crash.
             */
            for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
            {
                if (!dev->allocated ||
                    !dev->console)
                    continue;

                /* Try to obtain the device lock */
                if (try_obtain_lock( &dev->lock ))
                {
                    /* Unable to obtain device lock. Abort current scan
                       and retry our device block scan again if we have
                       not yet reached our retry limit. Otherwise if we
                       have reached our retry limit, do a normal obtain
                       to wait for the lock since the sched_yield we did
                       further below failed to accomplish what we hoped.
                    */
                    if (scan_retries < 10)
                    {
                        /* Try the scan over again from the beginning */
                        scan_complete = FALSE;
                        break;
                    }

                    /* Our sched_yield that we do when our retry limit
                       is reached failed to accomplish our objective.
                       Do a normal obtain_lock to wait forever for the
                       lock to be acquired.
                    */
                    OBTAIN_DEVLOCK( dev );
                }

                if (dev->console && dev->connected)
                {
                    ASSERT( dev->fd >= 0 ); /* (sanity check) */

                    /* Add it to our read set only if it's
                       not busy nor interrupt pending
                    */
                    if (1
                        && (!dev->busy || (dev->scsw.flag3 & SCSW3_AC_SUSP))
                        && !IOPENDING( dev )
                        && !(dev->scsw.flag3 & SCSW3_SC_PEND)
                    )
                    {
                        FD_SET( dev->fd, &readset );
                        if (dev->fd > maxfd)
                            maxfd = dev->fd;
                    }

                } /* end if connected console */

                RELEASE_DEVLOCK( dev );

            } /* end scan DEVBLK chain */

            /* Entire DEVBLK chain scanned without any problems? */
            if (scan_complete)
                break;

            /* Lock conflict; wait a moment before trying again */
            if (++scan_retries >= 10)
                sched_yield();

            /* Reset scan flag and try again */
            scan_complete = TRUE;

        } /* end build pselect() read set */

        /* Wait for a file descriptor to become ready.
         *
         * We use the POSIX pselect() function (and NOT the select()
         * function used in previous versions of Hercules) to address
         * several issues, including:
         *
         *     1)  Timeout values, as previously used in the console.c
         *         select() statements, are now constants.
         *
         *     2)  While debugging with POSIX, the need arose to manage
         *         interrupts for temporary debug code, and to avoid
         *         both real and induced deadlocks with other tasks
         *         (including the debuggers).
         *
         * For clarity, and rather than make additional botched attempts
         * to debug with pselect() and then release the final code with
         * the original select(), we now ALWAYS use pselect() with a NULL
         * interrupt mask instead which should be functionally equivalent
         * to select() anyway.
         */
        rc = pselect( maxfd+1, &readset, NULL, NULL, timeout, NULL );

        /* Clear the pipe signal if necessary */
        RECV_CONSOLE_THREAD_PIPE_SIGNAL();

        /* Check for thread exit condition */
        if (console_cnslcnt <= 0)
            break;

        /* Check for select timeout */
        if (rc == 0)
        {
            consto();
            continue;
        }

        /* Log pselect error */
        if (rc < 0 )
        {
            int select_errno = HSO_errno; // (preserve orig errno)
            static int issue_errmsg = 1;  // (prevents msgs flood)

            if (EBADF == select_errno)
            {
                // Don't issue message more frequently
                // than once every second or so, just in
                // case the condition that's causing it
                // keeps reoccurring over and over...

                static struct timeval  prev = {0,0};
                       struct timeval  curr;
                       struct timeval  diff;

                gettimeofday( &curr, NULL );
                timeval_subtract( &prev, &curr, &diff );

                // Has it been longer than one second
                // since we last issued this message?

                if (diff.tv_sec >= 1)
                {
                    issue_errmsg = 1;
                    prev.tv_sec  = curr.tv_sec;
                    prev.tv_usec = curr.tv_usec;
                }
                else
                    issue_errmsg = 0;   // (prevents msgs flood)
            }
            else
                issue_errmsg = 1;

            if (issue_errmsg && EINTR != select_errno)
            {
                // "COMM: pselect() failed: %s"
                CONERROR( HHC90508, "D", strerror( select_errno ));
                USLEEP( 50000 ); // (wait a bit; maybe it'll fix itself??)
            }
            continue;

        } /* end log pselect error */

        /* Accept incoming client connections */
        if (0
            || (lsock2 && FD_ISSET( lsock2, &readset ))
            || FD_ISSET( lsock,  &readset )
        )
        {
            /* Accept a connection and create conversation socket */
            if (lsock2 && FD_ISSET( lsock2, &readset ))
            {
                csock = accept( lsock2, NULL, NULL );
                sysg = true;
            }
            else
            {
                csock = accept( lsock,  NULL, NULL );
                sysg = false;
            }

            if (csock < 0)
            {
                // (use same technique as pselect error above)

                int accept_errno = HSO_errno; // (preserve orig errno)
                static int issue_errmsg = 1;  // (prevents msgs flood)

                if (HSO_EMFILE == accept_errno)
                {
                    // Don't issue message more frequently
                    // than once every second or so, just in
                    // case the condition that's causing it
                    // keeps reoccurring over and over...

                    static struct timeval  prev = {0,0};
                           struct timeval  curr;
                           struct timeval  diff;

                    gettimeofday( &curr, NULL );
                    timeval_subtract( &prev, &curr, &diff );

                    // Has it been longer than one second
                    // since we last issued this message?

                    if (diff.tv_sec >= 1)
                    {
                        issue_errmsg = 1;
                        prev.tv_sec  = curr.tv_sec;
                        prev.tv_usec = curr.tv_usec;
                    }
                    else
                        issue_errmsg = 0;   // (prevents msgs flood)
                }
                else
                    issue_errmsg = 1;

                if (issue_errmsg && EINTR != accept_errno)
                {
                    // "COMM: accept() failed: %s"
                    CONERROR( HHC90509, "D", strerror( accept_errno ));
                    USLEEP( 50000 ); // (wait a bit; maybe it'll fix itself??)
                }
                continue;
            }

            /* Allocate Telnet Control Block for this client */
            if (!(tn = (TELNET*) calloc( 1, sizeof( TELNET ))))
            {
                // "Out of memory"
                WRMSG( HHC00152, "E" );
                telnet_closesocket( csock );
            }
            else
            {
                static U32 clid = 0;
                tn->csock = csock;
                tn->sysg  = sysg;
                MSGBUF( tn->clientid, "client %u", clid++ );

                /* Initialize libtelnet package */
                tn->ctl = telnet_init( telnet_opts,
                    telnet_ev_handler, TELNET_FLAG_ACTIVE_NEG, tn );

                if (!tn->ctl)
                {
                    // "Out of memory"
                    WRMSG( HHC00152, "E" );
                    free( tn );
                    telnet_closesocket( csock );
                }
                else
                {
                    /* Create a thread to complete the client connection */
                    rc = create_thread( &tidneg, DETACHED,
                                connect_client, tn, CONN_CLI_THREAD_NAME );
                    if (rc)
                    {
                        // "Error in function create_thread(): %s"
                        WRMSG( HHC00102, "E", strerror( rc ));

                        telnet_free( tn->ctl );
                        free( tn );
                        telnet_closesocket( csock );
                    }
                }
            }
        } /* end accept incoming client connections */

        /* Initialize scan flags */
        scan_complete = TRUE;
        scan_retries = 0;

        /* Check connected consoles for available data */
        for (;;)
        {
            /* FIXME: Incorrectly running chain that may have a DEVBLK
             *        removed while in flight. For example, a device may
             *        be deleted between the loading of the nextdev and
             *        before the check for dev != NULL. This will result
             *        in an invalid storage reference and may result in
             *        a crash.
             */
            for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
            {
                /* Skip devices which aren't valid connected consoles */
                if (0
                    || !dev->allocated
                    || !dev->console
                    || !dev->connected
                )
                    continue;

                /* Try to obtain the device lock */
                if (try_obtain_lock( &dev->lock ))
                {
                    /* Unable to obtain device lock. Abort current scan
                       and retry our device block scan again if we have
                       not yet reached our retry limit. Otherwise if we
                       have reached our retry limit, do a normal obtain
                       to wait for the lock since the sched_yield we did
                       further below failed to accomplish what we hoped.
                    */
                    if (scan_retries < 10)
                    {
                        /* Try the scan over again from the beginning */
                        scan_complete = FALSE;
                        break;
                    }

                    /* Our sched_yield that we do when our retry limit
                       is reached failed to accomplish our objective.
                       Do a normal obtain_lock to wait forever for the
                       lock to be acquired.
                    */
                    OBTAIN_DEVLOCK( dev );
                }

                /* Test for valid connected console with data available.
                 * Note this test must be done with the device lock held.
                 */
                if (0
                    || !dev->allocated
                    || !dev->console
                    || !dev->connected
                    || (dev->busy && !(dev->scsw.flag3 & SCSW3_AC_SUSP))
                    || (dev->scsw.flag3 & SCSW3_SC_PEND)
                    || IOPENDING( dev )
                    || !FD_ISSET( dev->fd, &readset )
                    || sysblk.cnslpipe_flag
                )
                {
                    RELEASE_DEVLOCK( dev );
                    continue;
                }

                consio();

                /* Receive console input data from the client */

                /* Make the first call to recv below non-blocking
                   in case pselect lied to us and there isn't any
                   data available.  If we do multiple recv's then
                   the subsequent ones are blocking. See the linux
                   man page for select(2) for more info.
                */
                socket_set_blocking_mode( dev->fd, 0 );
                if ((dev->devtype == 0x3270) ||
                    (dev->devtype == 0x3287))
                {
                    do
                        {
                            prev_rlen3270 = dev->rlen3270;
                            unitstat = recv_3270_data( dev );

                            // "%s COMM: recv_3270_data: %d bytes received"
                            CONDEBUG2( HHC90502, "D", dev->tn->clientid,
                                dev->rlen3270 - prev_rlen3270 );
                            /* If we do another recv, make it blocking.
                               Otherwise we might just spin. */
                            socket_set_blocking_mode( dev->fd, 1 );
                        }
                        while ((unitstat == 0) && dev->rlen3270);

                    dev->readpending = 3;
                }
                else
                {
                    unitstat = recv_1052_data( dev );
                    socket_set_blocking_mode( dev->fd, 1 );
                }

                /* Close the connection if an error occurred */
                if (unitstat & CSW_UC)
                {
                    disconnect_console_device( dev );
                    RELEASE_DEVLOCK( dev );
                    continue;
                }

                /* Release the device lock */
                RELEASE_DEVLOCK( dev );

                if ((dev->devtype != 0x3270) &&
                    (dev->devtype != 0x3287))
                    raise_device_attention( dev, unitstat );
                else
                /* Raise attention interrupt for device, but only
                   if we actually received any 3270 data.  Telnet
                   keepalive messages for example, arrive as pure
                   telnet control messages which, once processed,
                   result in no actual 3270 client data remaining.
                */
                if (dev->rlen3270)
                    raise_device_attention( dev, unitstat );

            } /* end scan DEVBLK chain */

            /* Entire DEVBLK chain scanned without any problems? */
            if (scan_complete)
                break;

            /* Lock conflict; wait a moment before trying again */
            if (++scan_retries >= 10)
                sched_yield();

            /* Reset scan flag and try again */
            scan_complete = TRUE;

        } /* end for(;;) check connected consoles for available data */

    } /* end while (console_cnslcnt > 0) */

    free( curr_cnslport );
    free( curr_sysgport );

    /* Initialize scan flags */
    scan_complete = TRUE;
    scan_retries = 0;

    /* Close all connected consoles */
    for (;;)
    {
        /* FIXME: Incorrectly running chain that may have a DEVBLK
         *        removed while in flight. For example, a device may
         *        be deleted between the loading of the nextdev and
         *        before the check for dev != NULL. This will result
         *        in an invalid storage reference and may result in a
         *        crash.
         */
        for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        {
            /* Try to obtain the device lock */
            if (try_obtain_lock( &dev->lock ))
            {
                /* Unable to obtain device lock. Abort current scan
                   and retry our device block scan again if we have
                   not yet reached our retry limit. Otherwise if we
                   have reached our retry limit, do a normal obtain
                   to wait for the lock since the sched_yield we did
                   further below failed to accomplish what we hoped.
                */
                if (scan_retries < 10)
                {
                    /* Try the scan over again from the beginning */
                    scan_complete = FALSE;
                    break;
                }

                /* Our sched_yield that we do when our retry limit
                   is reached failed to accomplish our objective.
                   Do a normal obtain_lock to wait forever for the
                   lock to be acquired.
                */
                OBTAIN_DEVLOCK( dev );
            }

            /* Close console if still connected */
            if (1
                && dev->allocated
                && dev->console
                && dev->connected
            )
                disconnect_console_device( dev );

            /* Release the device lock */
            RELEASE_DEVLOCK( dev );

        } /* end scan DEVBLK chain */

        /* Entire DEVBLK chain scanned without any problems? */
        if (scan_complete)
            break;

        /* Lock conflict; wait a moment before trying again */
        if (++scan_retries >= 10)
            sched_yield();

        /* Reset scan flag and try again */
        scan_complete = TRUE;

    } /* end close all connected consoles */

    /* Close the listening sockets */
    close_socket( lsock  );
    if (lsock2) close_socket( lsock2 );

    // "Thread id "TIDPAT", prio %2d, name %s ended"
    LOG_THREAD_END( CON_CONN_THREAD_NAME  );

    return NULL;

} /* end function console_connection_handler */

/*-------------------------------------------------------------------*/
/* EXECUTE A 3270 CHANNEL COMMAND WORD                               */
/*-------------------------------------------------------------------*/
static void
loc3270_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual )
{
int             rc;                     /* Return code               */
U32             num;                    /* Number of bytes to copy   */
U32             len;                    /* Data length               */
int             aid;                    /* First read: AID present   */
U32             off;                    /* Offset in device buffer   */
BYTE            cmd;                    /* tn3270 command code       */
BYTE            buf[BUFLEN_3270];       /* tn3270 write buffer       */

    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq );

    /* Clear the current screen position at start of CCW chain */
    if (!chained)
        dev->pos3270 = 0;

    /* Unit check with intervention required if no client connected */
    if (!dev->connected && !IS_CCW_SENSE( code ))
    {
        dev->sense[0] = SENSE_IR;

        // NO!  *unitstat = CSW_CE | CSW_DE | CSW_UC;

        /*  ISW3274DR:  as per manual GA23-0218-11: "3174 Establishment
            Controller: Functional Description", section 3.1.3.2.2:
            "Solicited Status", Table 5-6: "Initial Status Conditions
            (Non-SNA)", the proper Intervention Required status is
            Unit Check (UC) only.
        */
        *unitstat = CSW_UC;    // (see above)
        return;
    }

    /* Process depending on CCW opcode */
    switch (code) {

    case L3270_NOP:
    /*---------------------------------------------------------------*/
    /* CONTROL NO-OPERATION                                          */
    /*---------------------------------------------------------------*/
        /* Reset the buffer address */
        dev->pos3270 = 0;

        *unitstat = CSW_CE | CSW_DE;
        break;

    case L3270_SELRM:
    case L3270_SELRB:
    case L3270_SELRMP:
    case L3270_SELRBP:
    case L3270_SELWRT:
    /*---------------------------------------------------------------*/
    /* SELECT                                                        */
    /*---------------------------------------------------------------*/
        /* Reset the buffer address */
        dev->pos3270 = 0;

    /*
        *residual = 0;
    */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case L3270_EAU:
    /*---------------------------------------------------------------*/
    /* ERASE ALL UNPROTECTED                                         */
    /*---------------------------------------------------------------*/
        dev->pos3270 = 0;
        cmd = R3270_EAU;
        goto write;

    case L3270_WRT:
    /*---------------------------------------------------------------*/
    /* WRITE                                                         */
    /*---------------------------------------------------------------*/
        cmd = R3270_WRT;
        goto write;

    case L3270_EW:
    /*---------------------------------------------------------------*/
    /* ERASE/WRITE                                                   */
    /*---------------------------------------------------------------*/
        dev->pos3270 = 0;
        cmd = R3270_EW;
        dev->ewa3270 = 0;
        goto write;

    case L3270_EWA:
    /*---------------------------------------------------------------*/
    /* ERASE/WRITE ALTERNATE                                         */
    /*---------------------------------------------------------------*/
        dev->pos3270 = 0;
        cmd = R3270_EWA;
        dev->ewa3270 = 1;
        goto write;

    case L3270_WSF:
    /*---------------------------------------------------------------*/
    /* WRITE STRUCTURED FIELD                                        */
    /*---------------------------------------------------------------*/
        /* Process WSF command if device has extended attributes */
        if (dev->eab3270)
        {
            dev->pos3270 = 0;
            cmd = R3270_WSF;
            goto write;
        }

        /* Operation check, device does not have extended attributes */
        dev->sense[0] = SENSE_OC;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        break;

    write:
    /*---------------------------------------------------------------*/
    /* All write commands, and the EAU control command, come here    */
    /*---------------------------------------------------------------*/
        /* Initialize the data length */
        len = 0;

        /* Calculate number of bytes to move and residual byte count */
        num = sizeof(buf) / 2;
        num = (count < num) ? count : num;
        if (cmd == R3270_EAU) num = 0;
        *residual = count - num;

        /* Move the 3270 command code to the first byte of the buffer
           unless data-chained from previous CCW
        */
        if (!(chained & CCW_FLAGS_CD))
        {
            buf[len++] = cmd;

            /* If this is a chained write then we start at the
               current buffer address, not the cursor address,
               and if the datastream's first action isn't already
               a positioning action, we also insert an SBA order
               to reposition to that buffer address.
            */
            if (1
                && (chained && R3270_WRT == cmd) // (chained write?)
                && dev->pos3270 != 0             // (not positioned at beginning?)
                && (1                            // (position wasn't specified?)
                    && iobuf[1] != O3270_SBA
                    && iobuf[1] != O3270_RA
                    && iobuf[1] != O3270_EUA
                   )
            )
            {
                /* Copy the write control character and adjust buffer */
                buf[len++] = *iobuf++;
                num--;

                /* Insert the SBA order */
                buf[len++] = O3270_SBA;

                if (dev->pos3270 < 4096)
                {
                    buf[len++] = sba_code[ dev->pos3270 >> 6 ];
                    buf[len++] = sba_code[ dev->pos3270 & 0x3F ];
                }
                else // (pos3270 >= 4096)
                {
                    buf[len++] = dev->pos3270 >> 8;
                    buf[len++] = dev->pos3270 & 0xFF;
                }
            }
        }

        /* Save the screen position at completion of the write.
           This is necessary in case a Read Buffer command is
           chained from another write or read.  Note that this
           does NOT apply for the Write Structured Field cmd.
        */
        if (cmd != R3270_WSF)
        {
            dev->pos3270 = end_of_buf_pos
            (
                dev->pos3270,
                //                           CD       not CD
                !(chained & CCW_FLAGS_CD) ? iobuf+1 : iobuf,
                !(chained & CCW_FLAGS_CD) ? num-1   : num
            );
        }

        /* Copy data from channel buffer to device buffer */
        memcpy( buf + len, iobuf, num );
        len += num;

        /* Send the data to the client */
        if (!sendto_client( dev->tn, buf, len ))
        {
            /* Return with Unit Check status if the send failed */
            dev->sense[0] = SENSE_DC;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case L3270_RB:
    /*---------------------------------------------------------------*/
    /* READ BUFFER                                                   */
    /*---------------------------------------------------------------*/

        OBTAIN_DEVLOCK( dev );
        {
            /* AID is only present during the first read */
            aid = dev->readpending != 2;
            if (dev->readpending == 3)
            {
                dev->readpending = 1;
                aid = 3;
            }
            /* Receive buffer data from client if not data chained */
            if (!(chained & CCW_FLAGS_CD))
            {
                /* Send read buffer command to client and await response */
                rc = solicit_3270_data (dev, R3270_RB);
                if (rc & CSW_UC)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    RELEASE_DEVLOCK( dev );
                    break;
                }

                /* Set AID in buffer flag */
                aid = 1;

                /* Save the AID of the current inbound transmission */
                dev->aid3270 = dev->buf[0];

                if (dev->pos3270 != 0 && dev->aid3270 != SF3270_AID)
                {
                    /* Find offset in buffer of current screen position */
                    off = pos_to_buff_offset( dev->pos3270,
                        dev->buf, dev->rlen3270 );

                    /* Shift out unwanted characters from buffer */
                    num = (dev->rlen3270 > off ? dev->rlen3270 - off : 0);
                    memmove (dev->buf + 3, dev->buf + off, num);
                    dev->rlen3270 = 3 + num;
                }
            }

            else aid = 0;

            /* Calculate number of bytes to move and residual byte count */
            len = dev->rlen3270;
            num = (count < len) ? count : len;
            *residual = count - num;
            if (count < len) *more = 1;

            /*  Save the screen position at completion of the read.
               This is necessary in case a Read Buffer command is chained
               from another write or read.
            */
            if (dev->aid3270 != SF3270_AID)
            {
                dev->pos3270 = end_of_buf_pos( dev->pos3270,
                    //    (aid)        (!aid)
                    aid ? dev->buf+3 : dev->buf,
                    aid ? num-3      : num
                );
            }

            /* Indicate that the AID bytes have been skipped */
            if (dev->readpending == 1)
                dev->readpending = 2;

            /* Copy data from device buffer to channel buffer */
            memcpy (iobuf, dev->buf, num);

            /* If data chaining is specified, save remaining data */
            if ((flags & CCW_FLAGS_CD) && len > count)
            {
                memmove (dev->buf, dev->buf + count, len - count);
                dev->rlen3270 = len - count;
            }
            else
            {
                dev->rlen3270 = 0;
                dev->readpending = 0;
            }

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        RELEASE_DEVLOCK( dev );

        /* Signal connection thread to redrive its pselect loop */
        SIGNAL_CONSOLE_THREAD();

        break;

    case L3270_RM:
    /*---------------------------------------------------------------*/
    /* READ MODIFIED                                                 */
    /*---------------------------------------------------------------*/

        OBTAIN_DEVLOCK( dev );
        {
            /* AID is only present during the first read */
            aid = dev->readpending != 2;
            if (dev->readpending == 3)
            {
                dev->readpending = 1;
                aid = 3;
            }

            /* If not data chained from previous Read Modified CCW,
               and if the connection thread has not already accumulated
               a complete Read Modified record in the inbound buffer,
               then solicit a Read Modified operation at the client
            */
            if ((chained & CCW_FLAGS_CD) == 0
                && !dev->readpending)
            {
                /* Send read modified command to client, await response */
                rc = solicit_3270_data (dev, R3270_RM);
                if (rc & CSW_UC)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    RELEASE_DEVLOCK( dev );
                    break;
                }

                /* Set AID in buffer flag */
                aid = 1;

                dev->aid3270 = dev->buf[0];

                if (dev->pos3270 != 0 && dev->aid3270 != SF3270_AID)
                {
                    /* Find offset in buffer of current screen position */
                    off = pos_to_buff_offset( dev->pos3270,
                        dev->buf, dev->rlen3270 );

                    /* Shift out unwanted characters from buffer */
                    num = (dev->rlen3270 > off ? dev->rlen3270 - off : 0);
                    memmove (dev->buf + 3, dev->buf + off, num);
                    dev->rlen3270 = 3 + num;
                }
            }

            else aid = 0;

            /* Calculate number of bytes to move and residual byte count */
            len = dev->rlen3270;
            num = (count < len) ? count : len;
            *residual = count - num;
            if (count < len) *more = 1;

            /*  Save the screen position at completion of the read.
               This is necessary in case a Read Buffer command is chained
               from another write or read.
            */
            if (dev->aid3270 != SF3270_AID)
            {
                dev->pos3270 = end_of_buf_pos( dev->pos3270,
                    //    (aid)        (!aid)
                    aid ? dev->buf+3 : dev->buf,
                    aid ? num-3      : num
                );
            }

            /* Indicate that the AID bytes have been skipped */
            if (dev->readpending == 1)
                dev->readpending = 2;

            /* Copy data from device buffer to channel buffer */
            memcpy (iobuf, dev->buf, num);

            /* If data chaining is specified, save remaining data */
            if ((flags & CCW_FLAGS_CD) && len > count)
            {
                memmove (dev->buf, dev->buf + count, len - count);
                dev->rlen3270 = len - count;
            }
            else
            {
                dev->rlen3270 = 0;
                dev->readpending = 0;
            }

            /* Set normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        RELEASE_DEVLOCK( dev );

        /* Signal connection thread to redrive its pselect loop */
        SIGNAL_CONSOLE_THREAD();

        break;

    case L3270_SENSE:
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
        memset( dev->sense, 0, sizeof(dev->sense) );

        /* Reset the buffer address */
        dev->pos3270 = 0;

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case L3270_SENSEID:
    /*---------------------------------------------------------------*/
    /* SENSE ID                                                      */
    /*---------------------------------------------------------------*/
        /* Calculate residual byte count */
        num = (count < dev->numdevid) ? count : dev->numdevid;
        *residual = count - num;
        if (count < dev->numdevid) *more = 1;

        /* Copy device identifier bytes to channel I/O buffer */
        memcpy (iobuf, dev->devid, num);

        /* Reset the buffer address */
        dev->pos3270 = 0;

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

} /* end function loc3270_execute_ccw */


/*-------------------------------------------------------------------*/
/* EXECUTE A 1052/3215 CHANNEL COMMAND WORD                          */
/*-------------------------------------------------------------------*/
static void
constty_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual )
{
U32     len;                            /* Length of data            */
U32     num;                            /* Number of bytes to move   */
BYTE    c;                              /* Print character           */
BYTE    stat;                           /* Unit status               */

    UNREFERENCED( chained );
    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq );

    /* Unit check with intervention required if no client connected */
    if (dev->connected == 0 && !IS_CCW_SENSE(code))
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
    {
        /* PROGRAMMING NOTE: we use (dev->bufsize-2) to ensure we
           always have room to append a '\n' newline as well as a
           null byte to terminate the buffer. This is needed since
           sendto_client() always calls telnet_printf() for console
           tty devices to ensure that any CR and/or LF get properly
           translated by libtelnet before being sent to the client.
        */
        U32 bufsize = (U32)(dev->bufsize-2);   /* reserve extra room */

        /* Calculate number of bytes to write and set residual count */
        num = (count < bufsize) ? count : bufsize;
        *residual = count - num;

        /* Translate data in channel buffer to ASCII */
        for (len = 0; len < num; len++)
        {
            c = guest_to_host( iobuf[len] );
            /* Leave it up to guest OS to filter characters. */
            iobuf[len] = c;
        } /* end for(len) */

        ASSERT(len == num);

        /* Perform end of record processing if not data-chaining */
        if ((flags & CCW_FLAGS_CD) == 0)
        {
            /* Append newline if required */
            if (code == 0x09)
                iobuf[len++] = '\n';
        }

        /* Ensure buffer is always null terminated */
        iobuf[len] = 0;

        /* Send the data to the client (via telnet_printf) */
        if (!sendto_client( dev->tn, iobuf, len ))
        {
            /* Return with Unit Check status if the send failed */
            dev->sense[0] = SENSE_EC;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;
    }

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
            {
                snprintf( (char*)dev->buf, dev->bufsize,
                        // "%1d:%04X COMM: enter console input"
                        // (Note: "MSG" macro used so message DOES end with newline)
                        MSG( HHC01026, "A", LCSS_DEVNUM)
                );
                len = (int) strlen( (char*)dev->buf );

                /* Send the data to the client */
                if (!sendto_client( dev->tn, dev->buf, len ))
                {
                    /* Return with Unit Check status if the send failed */
                    dev->sense[0] = SENSE_EC;
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    break;
                }
            }

            /* Accumulate client input data into device buffer */
            do
            {
                /* Receive client data and increment dev->keybdrem */
                stat = recv_1052_data( dev );
            }
            while (stat == 0);

            /* Exit if error status */
            if (stat != CSW_ATTN)
            {
                *unitstat = (CSW_CE | CSW_DE) | (stat & ~CSW_ATTN);
                break;
            }
        }

        /* Calculate number of bytes to move and residual byte count */
        len = dev->keybdrem;
        num = (count < len) ? count : len;
        *residual = count - num;
        if (count < len) *more = 1;

        /* Copy data from device buffer to channel buffer */
        memcpy (iobuf, dev->buf, num);

        /* If data chaining is specified, save remaining data */
        if ((flags & CCW_FLAGS_CD) && len > count)
        {
            memmove (dev->buf, dev->buf + count, len - count);
            dev->keybdrem = len - count;
        }
        else
            dev->keybdrem = 0;

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        break;

    case 0x0B:
    /*---------------------------------------------------------------*/
    /* AUDIBLE ALARM                                                 */
    /*---------------------------------------------------------------*/
    {
        static BYTE bell[2] = {'\a','\0'};  // "Ding!" (ring the bell)

        if (!sendto_client( dev->tn, bell, 1 ))
        {
            /* Return with Unit Check status if the send failed */
            dev->sense[0] = SENSE_EC;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        *unitstat = CSW_CE | CSW_DE;
        break;
    }

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
        memset( dev->sense, 0, sizeof(dev->sense) );

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

} /* end function constty_execute_ccw */


/*-------------------------------------------------------------------*/
/*            Libtool static name collision resolution               */
/*-------------------------------------------------------------------*/
/*   Note : lt_dlopen will look for symbol & modulename_LTX_symbol   */
/*-------------------------------------------------------------------*/

#if defined( HDL_USE_LIBTOOL )
#define  hdl_ddev   hdt3270_LTX_hdl_ddev
#define  hdl_depc   hdt3270_LTX_hdl_depc
#define  hdl_reso   hdt3270_LTX_hdl_reso
#define  hdl_init   hdt3270_LTX_hdl_init
#define  hdl_fini   hdt3270_LTX_hdl_fini
#endif

/*-------------------------------------------------------------------*/
/*                 DEVICE HANDLER ENTRY-POINTS                       */
/*-------------------------------------------------------------------*/

static DEVHND  constty_device_hndinfo  =
{
        &constty_init_handler,         /* Device Initialization      */
        &constty_execute_ccw,          /* Device CCW execute         */
        &constty_close_device,         /* Device Close               */
        &constty_query_device,         /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        &constty_redrive_pselect,      /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &constty_halt_or_clear,        /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        constty_immed,                 /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

static DEVHND  loc3270_device_hndinfo  =
{
        &loc3270_init_handler,         /* Device Initialization      */
        &loc3270_execute_ccw,          /* Device CCW execute         */
        &loc3270_close_device,         /* Device Close               */
        &loc3270_query_device,         /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        &loc3270_redrive_pselect,      /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &loc3270_halt_or_clear,        /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        loc3270_immed,                 /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        &loc3270_hsuspend,             /* Hercules suspend           */
        &loc3270_hresume               /* Hercules resume            */
};

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

#if defined( WIN32 ) && !defined( HDL_USE_LIBTOOL ) && !defined( _MSVC_ )

#undef sysblk

HDL_RESOLVER_SECTION;
{
    HDL_RESOLVE_SYMPTR( psysblk, sysblk );
}
END_RESOLVER_SECTION
#endif

HDL_DEVICE_SECTION
{
    HDL_DEVICE( 1052, constty_device_hndinfo );
    HDL_DEVICE( 3215, constty_device_hndinfo );
    HDL_DEVICE( 3270, loc3270_device_hndinfo );
    HDL_DEVICE( 3287, loc3270_device_hndinfo );

#if defined(_FEATURE_INTEGRATED_3270_CONSOLE)
    HDL_DEVICE( SYSG, loc3270_device_hndinfo );
#endif

}
END_DEVICE_SECTION
