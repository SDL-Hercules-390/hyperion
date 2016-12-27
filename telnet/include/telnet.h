/*-------------------------------------------------------------------*/
/*               TELNET protocol handling library                    */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* AUTHORS:                                                          */
/*                                                                   */
/*  Sean Middleditch         <sean@sourcemud.org>                    */
/*  "Fish" (David B. Trout)  <fish@softdevlabs.com>                  */
/*                                                                   */
/*                                                                   */
/* SUMMARY:                                                          */
/*                                                                   */
/*  libtelnet is a library for handling the TELNET protocol.  It     */
/*  includes routines for parsing incoming data from a remote peer   */
/*  as well as formatting data to be sent to the remote peer.        */
/*                                                                   */
/*  libtelnet uses a callback-oriented API, allowing application-    */
/*  specific handling of various events.  The callback system is     */
/*  also used for buffering outgoing protocol data, allowing the     */
/*  application to maintain control of the actual socket connection. */
/*                                                                   */
/*  Features supported include the full TELNET protocol, Q-method    */
/*  option negotiation, and NEW-ENVIRON.                             */
/*                                                                   */
/*                                                                   */
/* CONFORMS TO:                                                      */
/*                                                                   */
/*  RFC854  Telnet Protocol Specification                            */
/*          http://www.rfc-editor.org/info/rfc854                    */
/*                                                                   */
/*  RFC855  Telnet Option Specifications                             */
/*          http://www.rfc-editor.org/info/rfc855                    */
/*                                                                   */
/*  RFC1091 Telnet terminal-type option                              */
/*          http://www.rfc-editor.org/info/rfc1091                   */
/*                                                                   */
/*  RFC1143 The Q Method of Implementing TELNET Option Negotiation   */
/*          http://www.rfc-editor.org/info/rfc1143                   */
/*                                                                   */
/*  RFC1572 Telnet Environment Option                                */
/*          http://www.rfc-editor.org/info/rfc1572                   */
/*                                                                   */
/*  RFC1571 Telnet Environment Option Interoperability Issues        */
/*          http://www.rfc-editor.org/info/rfc1571                   */
/*                                                                   */
/*                                                                   */
/* LICENSE:                                                          */
/*                                                                   */
/*  The author or authors of this code dedicate any and all          */
/*  copyright interest in this code to the public domain. We         */
/*  make this dedication for the benefit of the public at large      */
/*  and to the detriment of our heirs and successors. We intend      */
/*  this dedication to be an overt act of relinquishment in          */
/*  perpetuity of all present and future rights to this code         */
/*  under copyright law.                                             */
/*                                                                   */
/*-------------------------------------------------------------------*/

#ifndef TELNET_H
#define TELNET_H

/*-------------------------------------------------------------------*/
/*                      Tweakable constants                          */
/*-------------------------------------------------------------------*/
#ifndef LIBTN_GRACEFUL_SOCKCLOSESECS
#define LIBTN_GRACEFUL_SOCKCLOSESECS    1.0
#endif

/*-------------------------------------------------------------------*/
/*                       Windows compatibility                       */
/*-------------------------------------------------------------------*/
#if defined(_WIN32) || defined(WIN32)
   /* Normalize WIN32/_WIN32 flags */
#  if !defined(_WIN32) && defined(WIN32)
#    define _WIN32
#  elif defined(_WIN32) && !defined(WIN32)
#    define WIN32
#  endif
   /* Normalize WIN64/_WIN64 flags */
#  if defined(_WIN64) || defined(WIN64)
#    if !defined(_WIN64) && defined(WIN64)
#      define _WIN64
#    elif defined(_WIN64) && !defined(WIN64)
#      define WIN64
#    endif
#  endif
   /* Normalize DEBUG/_DEBUG flags */
#  if defined(_DEBUG) || defined(DEBUG)
#    if !defined(_DEBUG) && defined(DEBUG)
#      define _DEBUG
#    elif defined(_DEBUG) && !defined(DEBUG)
#      define DEBUG
#    endif
#  endif
#endif /* defined(_WIN32) || defined(WIN32) */

#if defined(__cplusplus)
extern "C" {
#endif

/*-------------------------------------------------------------------*/
/*                       Telnet state codes                          */
/*-------------------------------------------------------------------*/
enum telnet_state_t
{
    TELNET_STATE_DATA = 0,
    TELNET_STATE_IAC,
    TELNET_STATE_WILL,
    TELNET_STATE_WONT,
    TELNET_STATE_DO,
    TELNET_STATE_DONT,
    TELNET_STATE_SB,
    TELNET_STATE_SB_DATA,
    TELNET_STATE_SB_DATA_IAC
};

/*-------------------------------------------------------------------*/
/*                         Event codes                               */
/*-------------------------------------------------------------------*/
enum telnet_event_code_t
{
    TELNET_EV_DATA = 0,         /* non-telnet data received from peer*/
    TELNET_EV_SEND,             /* raw data needs to be sent to peer */
    TELNET_EV_IAC,              /* generic IAC command received      */
    TELNET_EV_WILL,             /* WILL option negotiation received  */
    TELNET_EV_WONT,             /* WONT option neogitation received  */
    TELNET_EV_DO,               /* DO option negotiation received    */
    TELNET_EV_DONT,             /* DONT option negotiation received  */
    TELNET_EV_SUBNEGOTIATION,   /* sub-negotiation data received     */
    TELNET_EV_TTYPE,            /* TTYPE command has been received   */
    TELNET_EV_ENVIRON,          /* ENVIRON command has been received */
    TELNET_EV_WARNING,          /* recoverable error has occured     */
    TELNET_EV_ERROR             /* non-recoverable error has occured */
};

/*-------------------------------------------------------------------*/
/*                   Event warning/error codes                       */
/*-------------------------------------------------------------------*/
enum telnet_error_t
{
    TELNET_EOK = 0,             /* no error                          */
    TELNET_ENOMEM,              /* memory allocation failure         */
    TELNET_EOVERFLOW,           /* data exceeds buffer size          */
    TELNET_EPROTOCOL,           /* invalid sequence of special bytes */
    TELNET_ENEGOTIATION,        /* negotiation failed                */
};

/*-------------------------------------------------------------------*/
/*                         Typedefs                                  */
/*-------------------------------------------------------------------*/
typedef struct telnet_t                 telnet_t;
typedef struct telnet_telopt_t          telnet_telopt_t;
typedef union  telnet_event_t           telnet_event_t;
typedef enum   telnet_event_code_t      telnet_event_code_t;
typedef enum   telnet_error_t           telnet_error_t;
typedef enum   telnet_state_t           telnet_state_t;
typedef struct telnet_neg_queue_t       telnet_neg_queue_t;

/*-------------------------------------------------------------------*/
/*                       Event handler                               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  This is the type of function that must be passed to              */
/*  telnet_init() when creating a new telnet object.  The            */
/*  function will be invoked once for every event generated          */
/*  by the libtelnet protocol parser.                                */
/*                                                                   */
/*-------------------------------------------------------------------*/
typedef void (*telnet_event_handler_t)
(
    telnet_t*        telnet,     // Telnet object that caused the event
    telnet_event_t*  event,      // Event struct with event details
    void*            user_data   // User-supplied pointer
);

/*-------------------------------------------------------------------*/
/*                     Telnet state tracker                          */
/*-------------------------------------------------------------------*/
struct telnet_t
{
    telnet_state_t              state;          /* current state */
    BYTE                        flags;          /* option flags */

    const telnet_telopt_t*      telopts;        /* telopt support table */
    BYTE                        sb_telopt;      /* current subnegotiation telopt */

    BYTE*                       buffer;         /* sub-negotiation buffer */
    unsigned int                buffer_size;    /* current size of the buffer */
    unsigned int                buffer_pos;     /* current buffer write position (also length of buffer data) */

    struct telnet_neg_queue_t*  neg_q;          /* option negotiation state queue */
    BYTE                        neg_q_size;     /* entries in queue */

    telnet_event_handler_t      eh;             /* event handler */
    void*                       ud;             /* user data */
};

/*-------------------------------------------------------------------*/
/*               Option negotiation state queue entry                */
/*-------------------------------------------------------------------*/
struct telnet_neg_queue_t
{
    BYTE    telopt;
    BYTE    state;
};

/*-------------------------------------------------------------------*/
/*                    Option negotiation states                      */
/*-------------------------------------------------------------------*/
#define Q_NO                    0
#define Q_YES                   1
#define Q_WANTNO                2
#define Q_WANTYES               3
#define Q_WANTNO_OP             4
#define Q_WANTYES_OP            5

/*-------------------------------------------------------------------*/
/*                Telnet commands and special values                 */
/*-------------------------------------------------------------------*/
#define TELNET_EOF              236         /* End Of File           */
#define TELNET_SUSP             237         /* Suspend Process       */
#define TELNET_ABORT            238         /* Abort Process         */
#define TELNET_EOR              239         /* End Of Record         */
#define TELNET_SE               240         /* Subnegotiation End    */
#define TELNET_NOP              241         /* No Operation          */
#define TELNET_DM               242         /* Data Mark             */
#define TELNET_BREAK            243         /* Break                 */
#define TELNET_IP               244         /* Interrupt Process     */
#define TELNET_AO               245         /* Abort Output          */
#define TELNET_AYT              246         /* Are You There?        */
#define TELNET_EC               247         /* Erase Character       */
#define TELNET_EL               248         /* Erase Line            */
#define TELNET_GA               249         /* Go Ahead              */
#define TELNET_SB               250         /* Subnegotiation Begin  */
#define TELNET_WILL             251         /* Will                  */
#define TELNET_WONT             252         /* Won't                 */
#define TELNET_DO               253         /* Do                    */
#define TELNET_DONT             254         /* Don't                 */
#define TELNET_IAC              255         /* Interpret As Command  */

/*-------------------------------------------------------------------*/
/*                        Telnet options                             */
/*-------------------------------------------------------------------*/
#define TELNET_TELOPT_BINARY           0    /* Binary                */
#define TELNET_TELOPT_ECHO             1    /* Echo                  */
#define TELNET_TELOPT_RCP              2    /* Reconnect Process     */
#define TELNET_TELOPT_SGA              3    /* Suppress Go Ahead     */
#define TELNET_TELOPT_NAMS             4    /* Negotiate Message Size*/
#define TELNET_TELOPT_STATUS           5    /* Status                */
#define TELNET_TELOPT_TM               6    /* Timing Mark           */
#define TELNET_TELOPT_RCTE             7    /* Remote Controlled Transmission and Echo    */
#define TELNET_TELOPT_NAOL             8    /* Negotiate About Output Line width          */
#define TELNET_TELOPT_NAOP             9    /* Negotiate About Output Page size           */
#define TELNET_TELOPT_NAOCRD           10   /* Negotiate About CR Disposition             */
#define TELNET_TELOPT_NAOHTS           11   /* Negotiate About Horizontal Tabstops        */
#define TELNET_TELOPT_NAOHTD           12   /* Negotiate About Horizontal Tab Disposition */
#define TELNET_TELOPT_NAOFFD           13   /* Negotiate About FormFeed Disposition       */
#define TELNET_TELOPT_NAOVTS           14   /* Negotiate About Vertical Tab Stops         */
#define TELNET_TELOPT_NAOVTD           15   /* Negotiate About Vertical Tab Disposition   */
#define TELNET_TELOPT_NAOLFD           16   /* Negotiate About Output LF Disposition      */
#define TELNET_TELOPT_XASCII           17   /* Extended ASCII        */
#define TELNET_TELOPT_LOGOUT           18   /* Logout                */
#define TELNET_TELOPT_BM               19   /* Byte Macro            */
#define TELNET_TELOPT_DET              20   /* Data Entry Terminal   */
#define TELNET_TELOPT_SUPDUP           21   /* SUPDUP Protocol       */
#define TELNET_TELOPT_SUPDUPOUTPUT     22   /* SUPDUP Output         */
#define TELNET_TELOPT_SNDLOC           23   /* Send Location         */
#define TELNET_TELOPT_TTYPE            24   /* Terminal Type         */
#define TELNET_TELOPT_EOR              25   /* End Of Record         */
#define TELNET_TELOPT_TUID             26   /* TACACS User Id.       */
#define TELNET_TELOPT_OUTMRK           27   /* Output Marking        */
#define TELNET_TELOPT_TTYLOC           28   /* Terminal Location     */
#define TELNET_TELOPT_3270REGIME       29   /* 3270 Regime           */
#define TELNET_TELOPT_X3PAD            30   /*  X.3 PAD              */
#define TELNET_TELOPT_NAWS             31   /* Negotiate Window Size */
#define TELNET_TELOPT_TSPEED           32   /* Terminal Speed        */
#define TELNET_TELOPT_LFLOW            33   /* Toggle Flow Control   */
#define TELNET_TELOPT_LINEMODE         34   /* Linemode option       */
#define TELNET_TELOPT_XDISPLOC         35   /* X Display Location    */
#define TELNET_TELOPT_ENVIRON          36   /* Environ. vars (old)   */
#define TELNET_TELOPT_AUTHENTICATION   37   /* Authentication        */
#define TELNET_TELOPT_ENCRYPT          38   /* Encrypt               */
#define TELNET_TELOPT_NEW_ENVIRON      39   /* Environ. vars (new)   */
#define TELNET_TELOPT_EXOPL            255  /* Extended-Options-list */

/*-------------------------------------------------------------------*/
/*                     TERMINAL-TYPE codes                           */
/*-------------------------------------------------------------------*/
#define TELNET_TTYPE_IS             0
#define TELNET_TTYPE_SEND           1

/*-------------------------------------------------------------------*/
/*                  MAXIMUM TERMINAL-TYPE LENGTH                     */
/*-------------------------------------------------------------------*/
#define TELNET_MAX_TTYPE_LEN        40      /*    (per RFC1091)      */

/*-------------------------------------------------------------------*/
/*                    NEW-ENVIRON/ENVIRON codes                      */
/*-------------------------------------------------------------------*/
#define TELNET_ENVIRON_IS           0
#define TELNET_ENVIRON_SEND         1
#define TELNET_ENVIRON_INFO         2

#define TELNET_ENVIRON_VAR          0
#define TELNET_ENVIRON_VALUE        1
#define TELNET_ENVIRON_ESC          2
#define TELNET_ENVIRON_USERVAR      3

/*-------------------------------------------------------------------*/
/*                   telnet_init option flags                        */
/*-------------------------------------------------------------------*/
#define TELNET_FLAG_PASSIVE_NEG     (0)
#define TELNET_FLAG_ACTIVE_NEG      (1 << 7)
#define TELNET_FLAG_PROXY_MODE      (1 << 6)

/*-------------------------------------------------------------------*/
/*                  Environ command information                      */
/*-------------------------------------------------------------------*/
struct telnet_environ_t
{
    BYTE    type;   /* either TELNET_ENVIRON_VAR or TELNET_ENVIRON_USERVAR */
    char*   var;    /* name of the variable being set */
    char*   value;  /* value of variable being set; empty string if no value */
};

/*-------------------------------------------------------------------*/
/*                       Event information                           */
/*-------------------------------------------------------------------*/
union telnet_event_t
{
    /*
     * The type field will determine which of the other event
     * structure fields have been filled in.  For instance, if
     * the event type is TELNET_EV_TTYPE, then the ttype event
     * field (and ONLY the ttype event field) will be filled in.
     */
    enum telnet_event_code_t        type;       /* event type */


    /*
     * Data event: for DATA and SEND events
     */
    struct data_t
    {
        enum telnet_event_code_t    type;       /* alias for type            */
        const BYTE*                 buffer;     /* byte buffer               */
        unsigned int                size;       /* number of bytes in buffer */
    }
    data;


    /*
     * WARNING and ERROR events
     */
    struct zz_error_t
    {
        enum telnet_event_code_t    type;       /* alias for type               */
        const char*                 file;       /* file the error occured in     */
        const char*                 func;       /* function the error occured in */
        const char*                 msg;        /* error message string          */
        int                         line;       /* line of file error occured on */
        telnet_error_t              err;        /* error code                    */
    }
    error;


    /*
     * Command event: for IAC
     */
    struct iac_t
    {
        enum telnet_event_code_t    type;       /* alias for type          */
        BYTE                        cmd;        /* telnet command received */
    }
    iac;


    /*
     * Negotiation event: WILL, WONT, DO, DONT
     */
    struct negotiate_t
    {
        enum telnet_event_code_t    type;       /* alias for type          */
        BYTE                        telopt;     /* option being negotiated */
    }
    neg;


    /*
     * Subnegotiation event
     */
    struct subnegotiate_t
    {
        enum telnet_event_code_t    type;       /* alias for type              */
        const BYTE*                 buffer;     /* data of sub-negotiation     */
        unsigned int                size;       /* number of bytes in buffer   */
        BYTE                        telopt;     /* option code for negotiation */
    }
    sub;


    /*
     * TTYPE event
     */
    struct ttype_t
    {
        enum telnet_event_code_t    type;       /* alias for type        */
        BYTE                        cmd;        /* TELNET_TTYPE_IS or    */
                                                /* TELNET_TTYPE_SEND     */
        const char*                 name;       /* term name ('IS' only) */
    }
    ttype;


    /*
     * ENVIRON/NEW-ENVIRON event
     */
    struct environ_t
    {
        enum telnet_event_code_t    type;       /* alias for type    */
  const struct telnet_environ_t*    values;     /* var values array  */
        unsigned int                size;       /* number of values  */
        BYTE                        cmd;        /* SEND, IS, or INFO */
    }
    env;
};

/*-------------------------------------------------------------------*/
/*         Telopt options table     (-1,0,0 == end of table)         */
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
/*  you that the option is now enabled on their end).                */
/*                                                                   */
/*  Further note that TELNET_WILL/TELNET_WONT/TELNET_DO/TELNET_DONT  */
/*  table entry values are not by any means carved in stone. Any of  */
/*  the telnet options can be changed at any time by simply calling  */
/*  the telnet_negotiate() function with your new WILL/WONT/DO/DONT  */
/*  value for that option and then handling the corresponding event  */
/*  appropriately depending on how the other side responded.         */
/*                                                                   */
/*-------------------------------------------------------------------*/
struct telnet_telopt_t
{
    BYTE    telopt;                     /* TELOPT option code        */
    BYTE    us;                         /* TELNET_WILL, TELNET_WONT  */
    BYTE    them;                       /* TELNET_DO, TELNET_DONT    */
};

/*-------------------------------------------------------------------*/
/*                Initialize a telnet state tracker                  */
/*-------------------------------------------------------------------*/
/*
 * This function initializes a new state tracker, which is used
 * for all other libtelnet functions.  Each connection must have
 * its own telnet state tracker object.  Returns pointer to telent
 * state tracker object.
 */
extern telnet_t*  telnet_init
(
    const telnet_telopt_t*   telopts,    // Table of TELNET options the application supports.
    telnet_event_handler_t   eh,         // Event handler function called for every event.
    BYTE                     flags,      // 0 or TELNET_FLAG_PROXY_MODE.
    void*                    user_data   // Optional data pointer that will be passsed to eh.
);

/*-------------------------------------------------------------------*/
/*        Free up any memory allocated by a state tracker            */
/*-------------------------------------------------------------------*/
/*
 * This function must be called when a telnet state tracker is no
 * longer needed (such as after the connection has been closed) to
 * release any memory resources used by the state tracker.
 *
 *    telnet    Telnet state tracker object.
 */
extern void telnet_free( telnet_t* telnet );

/*-------------------------------------------------------------------*/
/*            Push buffer of bytes into the state tracker            */
/*-------------------------------------------------------------------*/
/*
 * Passes one or more bytes to the telnet state tracker for
 * protocol parsing.  The byte buffer is most often going to be
 * the buffer that recv() was called for while handling the
 * connection.
 */
extern void telnet_recv
(
    telnet_t*     telnet,   // Telnet state tracker object.
    const BYTE*   buffer,   // Pointer to byte buffer.
    unsigned int  size      // Number of bytes pointed to by buffer.
);

/*-------------------------------------------------------------------*/
/*                    Send a telnet command                          */
/*-------------------------------------------------------------------*/
extern void telnet_iac
(
    telnet_t*      telnet,      // Telnet state tracker object
    BYTE           cmd          // Command to send
);

/*-------------------------------------------------------------------*/
/*                  Send negotiation command                         */
/*-------------------------------------------------------------------*/
/*
 * Internally, libtelnet uses RFC1143 option negotiation rules.
 * The negotiation commands sent with this function may be ignored
 * if they are determined to be redundant.
 */
extern void telnet_negotiate
(
    telnet_t*      telnet,  // Telnet state tracker object.
    BYTE           cmd,     // TELNET_WILL, TELNET_WONT, TELNET_DO, or TELNET_DONT.
    BYTE           opt      // One of the TELNET_TELOPT_* values.
);

/*-------------------------------------------------------------------*/
/*                   Send non-command data                           */
/*-------------------------------------------------------------------*/
extern void telnet_send
(
    telnet_t*     telnet,       // Telnet state tracker object.
    const BYTE*   buffer,       // Buffer of bytes to send.
    unsigned int  size          // Number of bytes to send.
);

/*-------------------------------------------------------------------*/
/*              Begin a sub-negotiation command                      */
/*-------------------------------------------------------------------*/
/*
 * Sends IAC SB followed by the telopt code.  All subsequent data
 * sent will be part of the sub-negotiation, until telnet_finish_sb()
 * is called.
 */
extern void telnet_begin_sb
(
    telnet_t*      telnet,      // Telnet state tracker object.
    BYTE           telopt       // One of the TELNET_TELOPT_* values.
);

/*-------------------------------------------------------------------*/
/*              Finish a sub-negotiation command                     */
/*-------------------------------------------------------------------*/
/*
 * This must be called after a call to telnet_begin_sb() to finish
 * a sub-negotiation command.
 *
 *    telnet    Telnet state tracker object.
 */
#define telnet_finish_sb( telnet )  telnet_iac( (telnet), TELNET_SE )

/*-------------------------------------------------------------------*/
/*             Send a complete subnegotiation buffer                 */
/*-------------------------------------------------------------------*/
/*
 * Equivalent to:
 *
 *      telnet_begin_sb  ( telnet, telopt );
 *      telnet_send      ( telnet, buffer, size );
 *      telnet_finish_sb ( telnet );
 */
extern void telnet_subnegotiation
(
    telnet_t*      telnet,      // Telnet state tracker format.
    BYTE           telopt,      // One of the TELNET_TELOPT_* values.
    const BYTE*    buffer,      // Byte buffer for sub-negotiation data.
    unsigned int   size         // Number of bytes to use for sub-negotiation data.
);

/*-------------------------------------------------------------------*/
/*                     Send formatted data                           */
/*-------------------------------------------------------------------*/
/*
 * This function is a wrapper around telnet_send().  It allows
 * using printf-style formatting.
 *
 * Additionally, this function will translate \r to the CR NUL
 * construct and \n with CR LF, as well as automatically escaping
 * IAC bytes like telnet_send().
 *
 *    telnet    Telnet state tracker object.
 *    fmt       Format string.
 *
 * Returns:  Number of bytes sent.
 */
extern int telnet_printf ( telnet_t* telnet, const char* fmt, ...)         ATTR_PRINTF( 2, 3 );
extern int telnet_vprintf( telnet_t* telnet, const char* fmt, va_list va ) ATTR_PRINTF( 2, 0 );

/*-------------------------------------------------------------------*/
/*                     Send formatted data                           */
/*-------------------------------------------------------------------*/
/*
 * This behaves identically to telnet_printf(), except that the
 * \r and \n characters are not translated. The IAC byte is still
 * escaped as normal with telnet_send().
 *
 *    telnet    Telnet state tracker object.
 *    fmt       Format string.
 *
 * Returns:  Number of bytes sent.
 */
extern int telnet_raw_printf ( telnet_t* telnet, const char* fmt, ... )        ATTR_PRINTF( 2, 3 );
extern int telnet_raw_vprintf( telnet_t* telnet, const char* fmt, va_list va ) ATTR_PRINTF( 2, 0 );

/*-------------------------------------------------------------------*/
/*    Begin a new set of NEW-ENVIRON values to request or send       */
/*-------------------------------------------------------------------*/
/*
 * This function will begin the sub-negotiation block for sending
 * or requesting NEW-ENVIRON values.
 *
 * The telnet_finish_newenviron() macro must be called after this
 * function to terminate the NEW-ENVIRON command.
 *
 *    telnet        Telnet state tracker object.
 *    type          One of TELNET_ENVIRON_SEND, TELNET_ENVIRON_IS,
 *                  or TELNET_ENVIRON_INFO.
 */
extern void telnet_begin_newenviron( telnet_t* telnet, BYTE type );

/*-------------------------------------------------------------------*/
/*           Send a NEW-ENVIRON variable name or value               */
/*-------------------------------------------------------------------*/
/*
 * This can only be called between calls to telnet_begin_newenviron()
 * and telnet_finish_newenviron().
 *
 *    telnet        Telnet state tracker object.
 *    type          One of TELNET_ENVIRON_VAR, TELNET_ENVIRON_USERVAR,
 *                  or TELNET_ENVIRON_VALUE.
 *    string        Variable name or value.
 */
extern void telnet_newenviron_value
(
    telnet_t*      telnet,
    BYTE           type,
    const char*    string
);

/*-------------------------------------------------------------------*/
/*                  Finish a NEW-ENVIRON command                     */
/*-------------------------------------------------------------------*/
/*
 * This must be called after a call to telnet_begin_newenviron()
 * to finish a NEW-ENVIRON variable list.
 *
 *    telnet        Telnet state tracker object.
 */
#define telnet_finish_newenviron( telnet )    telnet_finish_sb((telnet))

/*-------------------------------------------------------------------*/
/*              Send the TERMINAL-TYPE SEND command                  */
/*-------------------------------------------------------------------*/
/*
 * Sends the sequence IAC TERMINAL-TYPE SEND.
 *
 *    telnet        Telnet state tracker object.
 */
extern void telnet_ttype_send( telnet_t* telnet );

/*-------------------------------------------------------------------*/
/*              Send the TERMINAL-TYPE IS command                    */
/*-------------------------------------------------------------------*/
/*
 * Sends the sequence IAC TERMINAL-TYPE IS <string>.
 *
 * According to the RFC, the recipient of a TERMINAL-TYPE SEND
 * shall send the next possible terminal-type the client supports.
 *
 * Upon sending the type, the client should switch modes to begin
 * acting as the terminal type it just sent.
 *
 * The server may continue sending TERMINAL-TYPE SEND until it
 * receives a terminal type it understands.  To indicate to the
 * server that it has reached the end of the available options,
 * the client must send the last terminal type a second time.
 *
 * When the server receives the same terminal type twice in a row,
 * it knows it has seen all available terminal types.
 *
 * After the last terminal type is sent, if the client receives
 * another TERMINAL-TYPE SEND command, it must begin enumerating
 * the available terminal types from the very beginning.
 *
 * This allows the server to scan the available types for a
 * preferred terminal type and, if none is found, to then ask
 * the client to switch to an acceptable alternative.
 *
 * Note that if the client only supports a single terminal type,
 * then simply sending that one type in response to every SEND
 * will satisfy the behavior requirements.
 *
 *    telnet        Telnet state tracker object.
 *    ttype         Name of the terminal-type being sent.
 */
extern void telnet_ttype_is( telnet_t* telnet, const char* ttype );

/*-------------------------------------------------------------------*/
/*                  Miscellaneous functions                          */
/*-------------------------------------------------------------------*/
extern const char*  telnet_cmd_name( BYTE cmd );
extern const char*  telnet_opt_name( BYTE opt );
extern const char*  telnet_evt_name( telnet_event_code_t evt );
extern const char*  telnet_err_name( telnet_error_t err );
extern int          telnet_closesocket( int sock ); /* graceful close */

#if defined(__cplusplus)
}
#endif

#endif /* TELNET_H */
