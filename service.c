/* SERVICE.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) and others 2013-2023                             */
/*              ESA/390 Service Processor                            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements service processor functions                */
/* for the Hercules ESA/390 emulator.                                */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Corrections contributed by Jan Jaeger                        */
/*      HMC system console functions by Jan Jaeger 2000-02-08        */
/*      Expanded storage support by Jan Jaeger                       */
/*      Dynamic CPU reconfiguration - Jan Jaeger                     */
/*      Suppress superflous HHC701I/HHC702I messages - Jan Jaeger    */
/*      Break syscons output if too long - Jan Jaeger                */
/*      Added CPI - Control Program Information ev. - JJ 2001-11-19  */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _SERVICE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "sr.h"
#include "service.h"

#ifndef _SERVICE_C
#define _SERVICE_C

/* Maximum event data buffer length plus one for \0 */
#define MAX_EVENT_MSG_LEN   ((_4K - 8) + 1)

/*-------------------------------------------------------------------*/
/*                non-ARCH_DEP helper functions                      */
/*              compiled only once, the first time                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* ATTEMPT to "fix" the message as best we can so it displays better */
/*-------------------------------------------------------------------*/
/* IBM s390x Linux seems to be using EBCDIC code page 500, not 37,   */
/* so if you use some other non-500 codepage setting in Hercules,    */
/* your messages will very likely NOT display correctly. Right now,  */
/* I'm using 819/500, which seems to work quite well, but 437/500    */
/* seems to work equally well, but unfortunately is not transparent. */
/*-------------------------------------------------------------------*/
static void gh534_fix( int msglen, BYTE* /*EBCDIC*/ e_msg )
{
    static bool once   = false;
    static bool ignore = false;
    static BYTE ESC    = 0;

    static const char* parms  = "0123456789:;<=>?";
//  static const char* inter  = " !\"#$%&'()*+,-./"; // (not used)
    static const char* final  = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    static       int   prmlen; // (length of above "parms" string)
    static       BYTE  subsub[3] = {0};

    BYTE  *p, *p2;   // (work)
    int i, seqlen;   // (work)

    // (message translated from guest to host)
    BYTE /*ASCII*/ a_msg[ MAX_EVENT_MSG_LEN ];

    /* Ignore messages that are "too long" */
    if (msglen >= (int) sizeof( a_msg ))
        return;

    /* For empty messages, just echo a blank line */
    if (!msglen)
    {
        LOGMSG( "\n" );
        return;
    }

    /* Note that IBM zLinux seems to use EBCDIC X'4A' for their
       ESCape character, so whatever that translates to is the
       host ASCII character that we need to treat as the "ESC"
       character that indicates the start of a Control Sequence.
       Same thing with the "SUB" character which they also use.
    */
    if (!once)
    {
        once = true;
        prmlen    = (int) strlen( parms );
        ESC       = guest_to_host( 0x4A /* CODEPAGE 500  '['  */ );
        subsub[0] = guest_to_host( 0x3F /* CODEPAGE 500 "SUB" */ );
        subsub[1] = subsub[0];
    }

    /* Convert from guest to host */
    for (i=0; i < msglen; i++)
        a_msg[i] = guest_to_host( e_msg[i] );
    a_msg[ msglen ] = 0;

    /*****************************************************************/
    /*                       REFERENCE                               */
    /*                                                               */
    /*    https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_(Con    */
    /*            trol_Sequence_Introducer)_sequences                */
    /*                                                               */
    /*    For Control Sequence Introducer, or CSI, commands,         */
    /*    the "ESC [" (written as "\e[" or "\033[" in several        */
    /*    programming and scripting languages) is followed by        */
    /*    any number (including none!) of "parameter bytes"          */
    /*    in the range 0x30-0x3F (ASCII 0-9:;<=>?), then by          */
    /*    any number of "intermediate bytes" in the range            */
    /*    0x20-0x2F (ASCII space and !"#$%&'()*+,-./), then          */
    /*    finally by a single "final byte" in the range 0x40-0x7E    */
    /*    (ASCII @A-Z[\]^_`a-z{|}~).                                 */
    /*                                                               */
    /*****************************************************************/

    for (p = a_msg; msglen;)
    {
        /* Find the start of the next control sequence */
        if (!(p = memchr( p, ESC, msglen )))
            break; // (No control sequences remain! We're done!)

        /* If the next character is NOT a "parameter byte", then it's
           obviously not a CSI. It might be some other Linux escape
           sequence though, so we need to check further.
        */
        if (!memchr( parms, *(p+1), prmlen ))
        {
            /*********************************************************/
            /*                     REFERENCE                         */
            /*                                                       */
            /*   console_codes(4) - Linux manual page                */
            /*   https://man7.org/linux/man-pages/man4/console_codes.4.html  */
            /*                                                       */
            /*   ECMA-48 CSI sequences:                              */
            /*                                                       */
            /*   The action of a CSI sequence is determined by its   */
            /*   final character:                                    */
            /*                                                       */
            /*     K  EL        Erase line (default: from cursor     */
            /*                  to end of line).                     */
            /*                                                       */
            /*        ESC [ 1 K: erase from start of line to cursor. */
            /*        ESC [ 2 K: erase whole line.                   */
            /*                                                       */
            /*********************************************************/

            /* If the next byte is a 'K', then, while this is not a
               CSI (Control Sequence Introducer), it appears to be
               an escape code that means "Erase line" (from the cursor
               to the end of line) which I'm interpreting as meaning:

               "Ignore everything that preceded this escape code
                and consider what follows this escape code as text
                that should be displayed starting from the beginning
                of the current line."
            */
            if (*(p+1) == 'K')
            {
                msglen -= (p + 2) - a_msg;
                memmove( a_msg, p + 2, msglen + 1 );
                p = a_msg;
                continue;
            }

            /* Otherwise it's not an escape sequence; skip over this
              non-ESC byte and continue looking for the next ESCape
              sequence.
            */
            ++p; --msglen;  // (skip past this non-ESC)
            continue;       // (look for the next ESC)
        }

        /* Remove the ESCape right away */
        memmove( p, p + 1, msglen + 1 );   // (+1 to include NULL)
        --msglen;

        /* Find the end of the control sequence */
        if (!(p2 = strpbrk( p, final )))
            break; // (Not found?! I guess we're done then!)

        /* Remove the entire control sequence */
        seqlen = (p2 - p) + 1;
        memmove( p, p2 + 1, (msglen - seqlen) + 1 );
        msglen -= seqlen;
    }

    /*****************************************************************/
    /*                    SPECIAL HANDLING                           */
    /*                                                               */
    /*   If the message starts with a "in progress" indicator,       */
    /*   i.e. "[   ***]" or any variation thereof, i.e. a '['        */
    /*   and one or more asterisks before the end ']', then we       */
    /*   will simply ignore that entire message and the next         */
    /*   message too.                                                */
    /*                                                               */
    /*****************************************************************/
    if (a_msg[0] == '[')
    {
        int scanlen = (int) strlen( a_msg );

        if ((p = memchr( a_msg, ']', scanlen )))
        {
            if ((scanlen = (p - a_msg)) > 0)
            {
                if (memchr( a_msg, '*', scanlen ))
                {
                    ignore = true;  // (ignore the NEXT message)
                    return;         // (and THIS message too)
                }
            }
        }
    }

    /* Ignore this message if our ignore flag is set, but ONLY if the
       current message appears to be a continuation (i.e. line wrap)
       of the previous message. Otherwise, if it appears to be a new
       message (i.e. starts with "[  OK  ]" or "        " = 8 blanks),
       then we obviously should NOT ignore it.
    */
    if (ignore)
    {
        ignore = false; // (reset flag)

        /* if NOT a new message, then ignore it as requested */
        if (1
            && a_msg[0] != '['
            && str_ne_n( a_msg, "        ", 8 )
        )
        {
            return;
        }

        /* Otherwise it's a new message. Display it. */
    }

    /* Lastly, convert any unprintable chars to spaces or underscores */
    for (i=0; a_msg[i]; i++)
    {
        if (!isprint( a_msg[i] ))
        {
            if (a_msg[i] == subsub[0])  // "SUB" character?
                a_msg[i] = '_';         // Replace with underscore
            else
                a_msg[i] = ' ';         // Otherwise with space
        }
    }

    /* And finally, show the fixed results! */
    LOGMSG( "%s\n", RTRIM( a_msg ));
}

/*-------------------------------------------------------------------*/
/*              Service processor state data                         */
/*-------------------------------------------------------------------*/
static  U32     servc_cp_recv_mask;     /* Syscons CP receive mask   */
static  U32     servc_cp_send_mask;     /* Syscons CP send mask      */
static  U32     servc_attn_pending;     /* Attention pending mask    */

static  char    servc_scpcmdstr[123+1]; /* Operator command string   */

static  U16     servc_signal_quiesce_count;
static  BYTE    servc_signal_quiesce_unit;
static  BYTE    servc_sysg_cmdcode;     /* Pending SYSG read command */

/*-------------------------------------------------------------------*/
/*      Reset the service processor to its initial state             */
/*-------------------------------------------------------------------*/
void sclp_reset()
{
    servc_cp_recv_mask = 0;
    servc_cp_send_mask = 0;
    servc_attn_pending = 0;
    servc_signal_quiesce_count = 0;
    servc_signal_quiesce_unit = 0;
    servc_sysg_cmdcode = 0;

    sysblk.servparm = 0;
}

/*-------------------------------------------------------------------*/
/*          Raise service signal external interrupt                  */
/*-------------------------------------------------------------------*/
/*     (the caller is expected to hold the interrupt lock!)          */
/*-------------------------------------------------------------------*/
void sclp_attention( U16 type )
{
    /* Set pending mask */
    servc_attn_pending |= 0x80000000 >> (type -1);

    /* Ignore request if already pending */
    if (!(IS_IC_SERVSIG && (sysblk.servparm & SERVSIG_PEND)))
    {
        /* Set event pending flag in service parameter */
        sysblk.servparm |= SERVSIG_PEND;

        /* Set service signal interrupt pending for read event data */
        ON_IC_SERVSIG;
        WAKEUP_CPUS_MASK( sysblk.waiting_mask );
    }
}

/*-------------------------------------------------------------------*/

static void* sclp_attn_thread( void* arg )
{
    U16* type = (U16*) arg;

    OBTAIN_INTLOCK( NULL );
    {
        // The VM boys appear to have made an error in not
        // allowing for asyncronous attentions to be merged
        // with pending interrupts. As such, we will wait here
        // until the pending interrupt has been cleared. *JJ

        while (IS_IC_SERVSIG)
        {
            RELEASE_INTLOCK (NULL );
            {
                sched_yield();
            }
            OBTAIN_INTLOCK( NULL );
        }

        sclp_attention( *type );
        free( type );
    }
    RELEASE_INTLOCK( NULL );
    return NULL;
}

/*-------------------------------------------------------------------*/

static void sclp_attn_async( U16 type )
{
    if (!IS_IC_SERVSIG)
    {
        sclp_attention( type );
    }
    else
    {
        int rc;
        TID sclp_attn_tid;
        U16* typ;
        typ = malloc( sizeof( U16 ));
        *typ = type;
        rc = create_thread( &sclp_attn_tid, &sysblk.detattr,
                            sclp_attn_thread, typ, "attn_thread" );
        if (rc)
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ));
    }
}

/*-------------------------------------------------------------------*/

static U32 sclp_attn_pending( U16 type )
{
U32 pending;

    if (type)
    {
        pending = servc_attn_pending & (0x80000000 >> (type -1));
        servc_attn_pending &= ~pending;
    }
    else
    {
        pending = servc_attn_pending;
    }

    return pending;
}

/*-------------------------------------------------------------------*/
/*                   Issue SCP command                               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This function is called from the control panel when the operator  */
/* enters an HMC system console SCP command or SCP priority message. */
/*                                                                   */
/* The command is queued for processing by the SCLP_READ_EVENT_DATA  */
/* service call, and a service signal interrupt is made pending.     */
/*                                                                   */
/* Input:                                                            */
/*      command Null-terminated ASCII command string                 */
/*      priomsg 0=SCP command, 1=SCP priority message                */
/*                                                                   */
/*-------------------------------------------------------------------*/
int scp_command( const char* command, bool priomsg, bool echo, bool mask )
{
    /* Error if disabled for priority messages */
    if (priomsg && !SCLP_RECV_ENABLED( PRIOR ))
    {
        // "SCLP console not receiving %s"
        WRMSG( HHC00002, "E", "priority commands" );
        return -1;
    }

    /* Error if disabled for commands */
    if (!priomsg && !SCLP_RECV_ENABLED( OPCMD ))
    {
        // "SCLP console not receiving %s"
        WRMSG( HHC00002, "E", "operator commands" );
        return -1;
    }

    /* Error if command string is missing */
    if (!command[0])
    {
        // "Empty SCP command issued"
        WRMSG( HHC00003, "E" );
        return -1;
    }

    /* Echo command to HMC console iuf requested */
    if (echo)
    {
        const char* cmd = mask ? "(suppressed)" : command;

        // "SCP %scommand: %s"
        WRMSG( HHC00160, "I", priomsg ? "priority " : "", cmd );
    }

    /* Obtain the interrupt lock */
    OBTAIN_INTLOCK( NULL );

    /* Save command string and message type for read event data */
    STRLCPY( servc_scpcmdstr, command );

    /* Raise attention service signal */
    sclp_attention( priomsg ? SCCB_EVD_TYPE_PRIOR : SCCB_EVD_TYPE_OPCMD );

    /* Release the interrupt lock */
    RELEASE_INTLOCK( NULL );

    return 0;

} /* end function scp_command */

/*-------------------------------------------------------------------*/

static void sclp_opcmd_event( SCCB_HEADER* sccb, U16 type )
{
    static const BYTE const1_template[] =
    {
        0x13,0x10,                      /* MDS message unit          */
        0x00,0x25,0x13,0x11,            /* MDS routine info          */
             0x0E,0x81,                 /* origin location name      */
                  0x03,0x01,0x00,       /* Net ID                    */
                  0x03,0x02,0x00,       /* NAU Name                  */
                  0x06,0x03,0x00,0x00,0x00,0x00,  /* Appl id         */
             0x0E,0x82,                 /* Destinition location name */
                  0x03,0x01,0x00,       /* Net ID                    */
                  0x03,0x02,0x00,       /* NAU Name                  */
                  0x06,0x03,0x00,0x00,0x00,0x00,  /* Appl id         */
             0x05,0x90,0x00,0x00,0x00,  /* Flags (MDS type = req)    */
        0x00,0x0C,0x15,0x49,            /* Agent unit-of-work        */
             0x08,0x01,                 /* Requestor loc name        */
                  0x03,0x01,0x00,       /* Requestor Net ID          */
                  0x03,0x02,0x00        /* Requestor Node ID         */
    };

    static const BYTE const2_template[] =
    {
        0x12,0x12,                      /* CP-MSU                    */
        0x00,0x12,0x15,0x4D,            /* RTI                       */
             0x0E,0x06,                 /* Name List                 */
                  0x06,0x10,0x00,0x03,0x00,0x00,  /* Cascaded
                                                       resource list */
                  0x06,0x60,0xD6,0xC3,0xC6,0xC1,  /* OAN (C'OCFA')   */
        0x00,0x04,0x80,0x70             /* Operate request           */
    };

    static const BYTE const3_template[] =
    {
        0x13,0x20                       /* Text data                 */
    };

    static const BYTE const4_template =
    {
        0x31                            /* Self-defining             */
    };

    static const BYTE const5_template =
    {
        0x30                            /* Text data                 */
    };

U16 sccb_len;
U16 evd_len;
int event_msglen;
int i;

SCCB_EVD_HDR* evd_hdr = (SCCB_EVD_HDR*)(sccb    + 1 );
SCCB_EVD_BK*  evd_bk  = (SCCB_EVD_BK*) (evd_hdr + 1 );
BYTE*         evd_msg = (BYTE*)        (evd_bk  + 1 );

    /* Get SCCB length */
    FETCH_HW( sccb_len, sccb->length  );

    /* Command length */
    event_msglen = (int)strlen( servc_scpcmdstr );

    /* Calculate required EVD length */
    evd_len = event_msglen + (int)sizeof( SCCB_EVD_HDR ) + (int)sizeof( SCCB_EVD_BK );

    /* Set response code X'75F0' if SCCB length exceeded */
    if ((evd_len + sizeof( SCCB_HEADER )) > sccb_len)
    {
        sccb->reas = SCCB_REAS_EXCEEDS_SCCB;
        sccb->resp = SCCB_RESP_EXCEEDS_SCCB;
        return;
    }

    /* Zero all fields */

    /* Update SCCB length field if variable request */
    if (sccb->type & SCCB_TYPE_VARIABLE)
    {
        /* Set new SCCB length */
        sccb_len = evd_len + sizeof( SCCB_HEADER );
        STORE_HW( sccb->length, sccb_len  );
        sccb->type &= ~SCCB_TYPE_VARIABLE;
    }

    /* Set length in event header */
    STORE_HW( evd_hdr->totlen, evd_len );

    /* Set type in event header */
    evd_hdr->type = type;

    /* Set message length in event data block */
    i = evd_len - sizeof( SCCB_EVD_HDR );
    STORE_HW( evd_bk->msglen, i  );

    memcpy( evd_bk->const1, const1_template,
                    sizeof( const1_template ));
    i -=            sizeof( const1_template ) + 2;
    STORE_HW( evd_bk->cplen, i  );

    memcpy( evd_bk->const2, const2_template,
                    sizeof( const2_template ));
    i -=            sizeof( const2_template ) + 2;
    STORE_HW( evd_bk->tdlen, i  );

    memcpy( evd_bk->const3, const3_template,
                    sizeof( const3_template ));
    i -=            sizeof( const3_template ) + 2;

    evd_bk->sdtlen = i;
    evd_bk->const4 = const4_template;

    i -= 2;
    evd_bk->tmlen = i;
    evd_bk->const5 = const5_template;

    /* Copy and translate command */
    for (i=0; i < event_msglen; i++)
        evd_msg[i] = host_to_guest( servc_scpcmdstr[i] );

    /* Set response code X'0020' in SCCB header */
    sccb->reas = SCCB_REAS_NONE;
    sccb->resp = SCCB_RESP_COMPLETE;
}

/*-------------------------------------------------------------------*/

static void sclp_cpident( SCCB_HEADER* sccb )
{
    SCCB_EVD_HDR*  evd_hdr = (SCCB_EVD_HDR*) (sccb    + 1);
    SCCB_CPI_BK*   cpi_bk  = (SCCB_CPI_BK*)  (evd_hdr + 1);

    char systype[9], sysname[9], sysplex[9];
    U64  syslevel;
    int i;

    if (*(cpi_bk->system_type )) set_systype( cpi_bk->system_type  );
    if (*(cpi_bk->system_name )) set_sysname( cpi_bk->system_name  );
    if (*(cpi_bk->sysplex_name)) set_sysplex( cpi_bk->sysplex_name );

    for (i=0; i < 8; i++)
    {
        systype[i] = guest_to_host( cpi_bk->system_type [i] );
        sysname[i] = guest_to_host( cpi_bk->system_name [i] );
        sysplex[i] = guest_to_host( cpi_bk->sysplex_name[i] );
    }

    systype[8] = sysname[8] = sysplex[8] = 0;

    for (i=7; i >= 0 && systype[i] == ' '; i--) systype[i] = 0;
    for (i=7; i >= 0 && sysname[i] == ' '; i--) sysname[i] = 0;
    for (i=7; i >= 0 && sysplex[i] == ' '; i--) sysplex[i] = 0;

    FETCH_DW( syslevel, cpi_bk->system_level );

    // "Control program identification: type %s, name %s, sysplex %s, level %"PRIX64
    WRMSG( HHC00004, "I", systype, sysname, sysplex, syslevel );

    {
        char buf[128];

        MSGBUF( buf, "%"PRIX64, syslevel );

        set_symbol( "SYSTYPE",  systype );
        set_symbol( "SYSNAME",  sysname );
        set_symbol( "SYSPLEX",  sysplex );
        set_symbol( "SYSLEVEL", buf     );
    }

    losc_check( systype );

    /* Indicate Event Processed */
    evd_hdr->flag |= SCCB_EVD_FLAG_PROC;

    /* Set response code X'0020' in SCCB header */
    sccb->reas = SCCB_REAS_NONE;
    sccb->resp = SCCB_RESP_COMPLETE;
}

/*-------------------------------------------------------------------*/
/*       Test whether SCP is enabled for QUIESCE signal              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This function tests whether the SCP is willing to be notified     */
/* of a system shutdown via the SCLP_READ_EVENT_DATA service call.   */
/*                                                                   */
/* Return code:                                                      */
/*      Zero = SCP not receiving quiesce event notification          */
/*      Non-zero = SCP ready to receive quiesce event notification   */
/*                                                                   */
/*-------------------------------------------------------------------*/
int can_signal_quiesce()
{
    return SCLP_RECV_ENABLED( SIGQ );
}

/*-------------------------------------------------------------------*/
/*    Test whether SCP is enabled to receive Operator Commands       */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This function tests whether the SCP is willing to receive         */
/* an operator command via the SCLP_READ_EVENT_DATA service call.    */
/*                                                                   */
/* Return code:                                                      */
/*      Zero = SCP not receiving Operator Commands                   */
/*      Non-zero = SCP ready to receive Operator Commands            */
/*                                                                   */
/*-------------------------------------------------------------------*/
int can_send_command()
{
    return SCLP_RECV_ENABLED( OPCMD );
}

/*-------------------------------------------------------------------*/
/*                Send QUIESCE signal to SCP                         */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This function is called during system shutdown to notify the SCP  */
/* that a shutdown event is occurring. The shutdown event is queued  */
/* for processing by the SCLP_READ_EVENT_DATA service call, and a    */
/* service signal interrupt is made pending.                         */
/*                                                                   */
/* Input:                                                            */
/*      count and unit values to be returned by SCLP_READ_EVENT_DATA */
/*                                                                   */
/*-------------------------------------------------------------------*/
int signal_quiesce( U16 count, BYTE unit )
{
    /* Error if disabled for commands */
    if (!SCLP_RECV_ENABLED( SIGQ ))
    {
        // "SCLP console not receiving %s"
        WRMSG( HHC00002, "E", "quiesce signals" );
        return -1;
    }

    OBTAIN_INTLOCK( NULL );
    {
        /* Save delay values for signal shutdown event read */
        servc_signal_quiesce_count = count;
        servc_signal_quiesce_unit  = unit;

        sclp_attention( SCCB_EVD_TYPE_SIGQ );
    }
    RELEASE_INTLOCK( NULL );

    return 0;
}

/*-------------------------------------------------------------------*/

static void sclp_sigq_event( SCCB_HEADER* sccb )
{
U16           sccb_len;
U16           evd_len;
SCCB_EVD_HDR* evd_hdr = (SCCB_EVD_HDR*)( sccb    + 1 );
SCCB_SGQ_BK*  sgq_bk  = (SCCB_SGQ_BK*) ( evd_hdr + 1 );

    FETCH_HW( sccb_len, sccb->length );
    evd_len = sizeof( SCCB_EVD_HDR ) + sizeof( SCCB_SGQ_BK );

    /* Set response code X'75F0' if SCCB length exceeded */
    if ((evd_len + sizeof( SCCB_HEADER )) > sccb_len)
    {
        sccb->reas = SCCB_REAS_EXCEEDS_SCCB;
        sccb->resp = SCCB_RESP_EXCEEDS_SCCB;
        return;
    }

    /* Zero all fields */
    memset( evd_hdr, 0, evd_len );

    /* Update SCCB length field if variable request */
    if (sccb->type & SCCB_TYPE_VARIABLE)
    {
        /* Set new SCCB length */
        sccb_len = evd_len + sizeof( SCCB_HEADER );
        STORE_HW( sccb->length, sccb_len );
        sccb->type &= ~SCCB_TYPE_VARIABLE;
    }

    /* Set length in event header */
    STORE_HW( evd_hdr->totlen, evd_len );

    /* Set type in event header */
    evd_hdr->type = SCCB_EVD_TYPE_SIGQ;

    STORE_HW( sgq_bk->count, servc_signal_quiesce_count );
    sgq_bk->unit = servc_signal_quiesce_unit;

    /* Set response code X'0020' in SCCB header */
    sccb->reas = SCCB_REAS_NONE;
    sccb->resp = SCCB_RESP_COMPLETE;
}

#if defined( _FEATURE_INTEGRATED_3270_CONSOLE )
/*-------------------------------------------------------------------*/
/*              Write data to the SYSG console                       */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The datastream to be written to the SYSG console is in the SCCB   */
/* immediately following the Event Data Header. It consists of a     */
/* one-byte local 3270 CCW command code, followed by a 3270 WCC,     */
/* followed by 3270 orders and data.                                 */
/*                                                                   */
/* Input:                                                            */
/*      sccb    Address of SCCB                                      */
/*      evd_hdr Address of event data header within SCCB             */
/* Output:                                                           */
/*      Reason and response codes are set in the SCCB                */
/*                                                                   */
/*-------------------------------------------------------------------*/
static void sclp_sysg_write( SCCB_HEADER* sccb )
{
SCCB_EVD_HDR* evd_hdr = (SCCB_EVD_HDR*)(sccb+1);
U16           evd_len;                  /* SCCB event data length    */
U16           sysg_len;                 /* SYSG output data length   */
DEVBLK*       dev;                      /* -> SYSG console devblk    */
BYTE*         sysg_data;                /* -> SYSG output data       */
BYTE          unitstat;                 /* Unit status               */
BYTE          more = 0;                 /* Flag for device handler   */
U32           residual;                 /* Residual data count       */
BYTE         cmdcode;                   /* 3270 read/write command   */

    /* Calculate the address and length of the 3270 datastream */
    FETCH_HW( evd_len, evd_hdr->totlen );
    sysg_data = (BYTE*)(evd_hdr+1);
    sysg_len  = evd_len - sizeof( SCCB_EVD_HDR );

    /* The first data byte is the 3270 command code */
    cmdcode = *sysg_data;

    /* Look for the SYSG console device block */
    dev = sysblk.sysgdev;
    if (dev == NULL)
    {
        PTT_ERR("*SERVC", (U32)cmdcode, (U32)sysg_len, 0 );

        /* Set response code X'05F0' in SCCB header */
        sccb->reas = SCCB_REAS_IMPROPER_RSC;
        sccb->resp = SCCB_RESP_REJECT;
        return;
    }

    /* If it is a read CCW then save the command until READ_EVENT_DATA */
    if (IS_CCW_READ( cmdcode ))
    {

        servc_sysg_cmdcode = cmdcode;

        /* Indicate Event Processed */
        evd_hdr->flag |= SCCB_EVD_FLAG_PROC;

        /* Generate a service call interrupt to trigger READ_EVENT_DATA */
        sclp_attn_async( SCCB_EVD_TYPE_SYSG );

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
        return;
    }
    else
    {
        servc_sysg_cmdcode = 0x00;

        /* Execute the 3270 command in data block */
        /* dev->hnd->exec points to loc3270_execute_ccw */
        (dev->hnd->exec)( dev, /*ccw opcode*/ cmdcode,
            /*flags*/ CCW_FLAGS_SLI, /*chained*/0,
            /*count*/ sysg_len - 1,
            /*prevcode*/ 0, /*ccwseq*/ 0, /*iobuf*/ sysg_data+1,
            &more, &unitstat, &residual );

        /* Indicate Event Processed */
        evd_hdr->flag |= SCCB_EVD_FLAG_PROC;

        /* If unit check occured, set response code X'0040' */
        if (unitstat & CSW_UC)
        {
            PTT_ERR("*SERVC", (U32)more, (U32)unitstat, residual );

            /* Set response code X'0040' in SCCB header */
            sccb->reas = SCCB_REAS_NONE;
            sccb->resp = SCCB_RESP_BACKOUT;
            return;
        }

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
    }
}

/*-------------------------------------------------------------------*/
/*              Read data from the SYSG console                      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* If the SYSG console has data to send, copy it into the SCCB       */
/* immediately following the Event Data Header. The data consists    */
/* of a 3270 AID byte, followed by a two-byte 3270 cursor address,   */
/* followed by 3270 orders and data.                                 */
/*                                                                   */
/* Output:                                                           */
/*      Data, reason and response codes are set in the SCCB          */
/*                                                                   */
/*-------------------------------------------------------------------*/
static void sclp_sysg_poll( SCCB_HEADER* sccb )
{
SCCB_EVD_HDR*  evd_hdr = (SCCB_EVD_HDR*)(sccb+1);
U16            sccblen;                 /* SCCB total length         */
U16            evd_len;                 /* SCCB event data length    */
U16            sysg_len;                /* SYSG input data length    */
DEVBLK*        dev;                     /* -> SYSG console devblk    */
BYTE*          sysg_data;               /* -> SYSG input data        */
BYTE*          sysg_cmd;                /* -> SYSG input data        */
BYTE           unitstat;                /* Unit status               */
BYTE           more = 0;                /* Flag for device handler   */
U32            residual;                /* Residual data count       */

    dev = sysblk.sysgdev;
    if (dev != NULL)
    {
        /* Zeroize the event data header */
        memset( evd_hdr, 0, sizeof( SCCB_EVD_HDR ));

        /* Calculate maximum data length */
        FETCH_HW( sccblen, sccb->length );
        evd_len = sccblen - sizeof( SCCB_HEADER );
        sysg_data = (BYTE*)(evd_hdr+1);
        sysg_len = evd_len - sizeof( SCCB_EVD_HDR );

        /* Insert flag byte before the 3270 input data */
        sysg_cmd  = sysg_data;
        sysg_len  -= 1;
        sysg_data += 1;

        /* Execute previously saved 3270 read command */
        if (servc_sysg_cmdcode)
        {
            *sysg_cmd = 0x00;

            /* Execute a 3270 read-modified command */
            /* dev->hnd->exec points to loc3270_execute_ccw */
            (dev->hnd->exec) (dev, /*ccw opcode*/ servc_sysg_cmdcode,
                /*flags*/ CCW_FLAGS_SLI, /*chained*/0,
                /*count*/ sysg_len,
                /*prevcode*/ 0, /*ccwseq*/ 0, /*iobuf*/ sysg_data,
                &more, &unitstat, &residual );

            servc_sysg_cmdcode = 0;

            /* Set response code X'0040' if unit check occurred */
            if (unitstat & CSW_UC)
            {
                PTT_ERR("*SERVC", (U32)more, (U32)unitstat, residual );

                /* Set response code X'0040' in SCCB header */
                sccb->reas = SCCB_REAS_NONE;
                sccb->resp = SCCB_RESP_BACKOUT;
                return;
            }

            /* Set response code X'75F0' if SCCB length exceeded */
            if (more)
            {
                PTT_ERR("*SERVC", (U32)more, (U32)unitstat, residual );

                sccb->reas = SCCB_REAS_EXCEEDS_SCCB;
                sccb->resp = SCCB_RESP_EXCEEDS_SCCB;
                return;
            }

            /* Calculate actual length read */
            sysg_len -= residual;
            evd_len = sizeof( SCCB_EVD_HDR ) + sysg_len + 1;

            /* Set response code X'0020' in SCCB header */
            sccb->reas = SCCB_REAS_NONE;
            sccb->resp = SCCB_RESP_COMPLETE;
        }
        else
        {
            evd_len = sizeof( SCCB_EVD_HDR ) + 1;
            *sysg_cmd = 0x80;

            /* Set response code X'0020' in SCCB header */
            sccb->reas = SCCB_REAS_NONE;
            sccb->resp = SCCB_RESP_COMPLETE;
        }

        /* Update SCCB length field if variable request */
        if (sccb->type & SCCB_TYPE_VARIABLE)
        {
            /* Set new SCCB length */
            sccblen = evd_len + sizeof( SCCB_HEADER );
            STORE_HW( sccb->length, sccblen );
            sccb->type &= ~SCCB_TYPE_VARIABLE;
        }

        /* Set length in event header */
        STORE_HW( evd_hdr->totlen, evd_len );

        /* Set type in event header */
        evd_hdr->type = SCCB_EVD_TYPE_SYSG;
    }
}

/*-------------------------------------------------------------------*/
/*      Handle attention interrupt from the SYSG console             */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This function is called by console.c when it receives input       */
/* from the SYSG console. It sets the SYSG read flag and raises      */
/* a service signal external interrupt, which should prompt the      */
/* SCP to issue a SCLP_READ_EVENT_DATA service call to retrieve      */
/* the input data.                                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT void sclp_sysg_attention()
{

    OBTAIN_INTLOCK( NULL );
    {
        sclp_attn_async( SCCB_EVD_TYPE_SYSG );
    }
    RELEASE_INTLOCK( NULL );
}
#endif /* defined( _FEATURE_INTEGRATED_3270_CONSOLE ) */

/*-------------------------------------------------------------------*/

#if defined( _FEATURE_INTEGRATED_ASCII_CONSOLE )

static int sclp_sysa_write( SCCB_HEADER* sccb )
{
SCCB_EVD_HDR* evd_hdr = (SCCB_EVD_HDR*)(sccb+1);
U16           evd_len;
U16           sysa_len;
BYTE*         sysa_data;
int           i;

    LOGMSG( "SYSA write:" );
    FETCH_HW( evd_len, evd_hdr->totlen );
    sysa_data = (BYTE*)(evd_hdr+1);
    sysa_len = evd_len - sizeof( SCCB_EVD_HDR );

    for (i=0; i < sysa_len; i++)
    {
        if (!(i & 15))
            LOGMSG("\n          %4.4X:", i);
        LOGMSG(" %2.2X", sysa_data[i]);
    }

    if (i & 15)
        LOGMSG("\n");

    /* Indicate Event Processed */
    evd_hdr->flag |= SCCB_EVD_FLAG_PROC;

    /* Set response code X'0020' in SCCB header */
    sccb->reas = SCCB_REAS_NONE;
    sccb->resp = SCCB_RESP_COMPLETE;  // maybe this needs to be INFO

    //sclp_attention( SCCB_EVD_TYPE_VT220 );

    return 0; // write ok
}

/*-------------------------------------------------------------------*/

static int sclp_sysa_poll( SCCB_HEADER* sccb )
{
SCCB_EVD_HDR* evd_hdr = (SCCB_EVD_HDR*)(sccb+1);

    UNREFERENCED( sccb );
    UNREFERENCED( evd_hdr );

    LOGMSG( "VT220 poll\n" );
}

#endif /* defined( _FEATURE_INTEGRATED_ASCII_CONSOLE ) */

/*-------------------------------------------------------------------*/
/*             Suspend and resume functions                          */
/*-------------------------------------------------------------------*/

#define SR_SYS_SERVC_RECVMASK    ( SR_SYS_SERVC | 0x001 )
#define SR_SYS_SERVC_SENDMASK    ( SR_SYS_SERVC | 0x002 )
#define SR_SYS_SERVC_PENDING     ( SR_SYS_SERVC | 0x003 )
#define SR_SYS_SERVC_SCPCMD      ( SR_SYS_SERVC | 0x004 )
#define SR_SYS_SERVC_SQC         ( SR_SYS_SERVC | 0x005 )
#define SR_SYS_SERVC_SQU         ( SR_SYS_SERVC | 0x006 )
#define SR_SYS_SERVC_PARM        ( SR_SYS_SERVC | 0x007 )

/*-------------------------------------------------------------------*/

int servc_hsuspend(void* file)
{
    SR_WRITE_VALUE(file, SR_SYS_SERVC_RECVMASK, servc_cp_recv_mask, sizeof( servc_cp_recv_mask ));
    SR_WRITE_VALUE(file, SR_SYS_SERVC_SENDMASK, servc_cp_send_mask, sizeof( servc_cp_send_mask ));
    SR_WRITE_VALUE(file, SR_SYS_SERVC_PENDING, servc_attn_pending, sizeof( servc_attn_pending ));
    SR_WRITE_STRING(file, SR_SYS_SERVC_SCPCMD,  servc_scpcmdstr);
    SR_WRITE_VALUE(file, SR_SYS_SERVC_SQC,      servc_signal_quiesce_count,
                                         sizeof( servc_signal_quiesce_count ));
    SR_WRITE_VALUE(file, SR_SYS_SERVC_SQU,      servc_signal_quiesce_unit,
                                         sizeof( servc_signal_quiesce_unit ));
    SR_WRITE_VALUE(file, SR_SYS_SERVC_PARM,     sysblk.servparm,
                                         sizeof(sysblk.servparm));
    return 0;
}

/*-------------------------------------------------------------------*/

int servc_hresume(void* file)
{
    size_t key, len;

    sclp_reset();
    do {
        SR_READ_HDR(file, key, len);
        switch (key) {
        case SR_SYS_SERVC_RECVMASK:
            SR_READ_VALUE(file, len, &servc_cp_recv_mask, sizeof( servc_cp_recv_mask ));
            break;
        case SR_SYS_SERVC_SENDMASK:
            SR_READ_VALUE(file, len, &servc_cp_send_mask, sizeof( servc_cp_send_mask ));
            break;
        case SR_SYS_SERVC_PENDING:
            SR_READ_VALUE(file, len, &servc_attn_pending, sizeof( servc_attn_pending ));
            break;
        case SR_SYS_SERVC_SCPCMD:
            if ( len <= sizeof( servc_scpcmdstr ) )
                SR_READ_STRING(file, servc_scpcmdstr, len);
            else
                SR_READ_SKIP(file, len);
            break;
        case SR_SYS_SERVC_SQC:
            SR_READ_VALUE(file, len, &servc_signal_quiesce_count,
                              sizeof( servc_signal_quiesce_count ));
            break;
        case SR_SYS_SERVC_SQU:
            SR_READ_VALUE(file, len, &servc_signal_quiesce_unit,
                              sizeof( servc_signal_quiesce_unit ));
            break;
        case SR_SYS_SERVC_PARM:
            SR_READ_VALUE(file, len, &sysblk.servparm, sizeof(sysblk.servparm));
            break;
        default:
            SR_READ_SKIP(file, len);
            break;
        }
    } while ((key & SR_SYS_MASK) == SR_SYS_SERVC);
    return 0;
}

#endif /* _SERVICE_C */

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

#if defined( FEATURE_SERVICE_PROCESSOR )
/*-------------------------------------------------------------------*/
/*     Architecture-dependent service processor bit strings          */
/*-------------------------------------------------------------------*/
BYTE  ARCH_DEP( scpinfo_ifm )[8] =
{
                        0
                        | SCCB_IFM0_CHANNEL_PATH_INFORMATION
                        | SCCB_IFM0_CHANNEL_PATH_SUBSYSTEM_COMMAND
//                      | SCCB_IFM0_CHANNEL_PATH_RECONFIG
//                      | SCCB_IFM0_CPU_INFORMATION
#if defined( FEATURE_CPU_RECONFIG )
                        | SCCB_IFM0_CPU_RECONFIG
#endif
                        ,
                        0
//                      | SCCB_IFM1_SIGNAL_ALARM
//                      | SCCB_IFM1_WRITE_OPERATOR_MESSAGE
//                      | SCCB_IFM1_STORE_STATUS_ON_LOAD
//                      | SCCB_IFM1_RESTART_REASONS
//                      | SCCB_IFM1_INSTRUCTION_ADDRESS_TRACE_BUFFER
                        | SCCB_IFM1_LOAD_PARAMETER
                        ,
                        0
//                      | SCCB_IFM2_REAL_STORAGE_INCREMENT_RECONFIG
//                      | SCCB_IFM2_REAL_STORAGE_ELEMENT_INFO
//                      | SCCB_IFM2_REAL_STORAGE_ELEMENT_RECONFIG
//                      | SCCB_IFM2_COPY_AND_REASSIGN_STORAGE
#if defined( FEATURE_EXPANDED_STORAGE )
                        | SCCB_IFM2_EXTENDED_STORAGE_USABILITY_MAP
#endif
//                      | SCCB_IFM2_EXTENDED_STORAGE_ELEMENT_INFO
//                      | SCCB_IFM2_EXTENDED_STORAGE_ELEMENT_RECONFIG
                        ,
                        0
#if defined( FEATURE_S370_S390_VECTOR_FACILITY ) && defined( FEATURE_CPU_RECONFIG )
                        | SCCB_IFM3_VECTOR_FEATURE_RECONFIG
#endif
#if defined( FEATURE_SYSTEM_CONSOLE )
                        | SCCB_IFM3_READ_WRITE_EVENT_FEATURE
#endif
//                      | SCCB_IFM3_READ_RESOURCE_GROUP_INFO
                        ,
                        0, 0, 0, 0
};

/*-------------------------------------------------------------------*/

BYTE  ARCH_DEP( scpinfo_cfg )[6] =
{
// ----------------BYTE 0 -------------------------------------------

                        0
#if defined( FEATURE_HYPERVISOR )
//                      | SCCB_CFG0_LOGICALLY_PARTITIONED
#endif
#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
                        | SCCB_CFG0_SUPPRESSION_ON_PROTECTION
#endif
//                      | SCCB_CFG0_INITIATE_RESET
#if defined( FEATURE_CHSC )
                        | SCCB_CFG0_STORE_CHANNEL_SUBSYS_CHARACTERISTICS
#endif
#if defined( FEATURE_MOVE_PAGE_FACILITY_2 )
                        | SCCB_CFG0_MVPG_FOR_ALL_GUESTS
#endif
#if defined( FEATURE_FAST_SYNC_DATA_MOVER )
    /* The Fast Sync Data Mover facility is simply a flag which
       indicates that the MVPG instruction performs faster than
       the Asynchronous Data Mover facility (see GA22-1030-03) */
                        | SCCB_CFG0_FAST_SYNCHRONOUS_DATA_MOVER
#endif
                        ,


// ----------------BYTE 1 -------------------------------------------

                        0
//                      | SCCB_CFG1_CSLO
                        ,


// ----------------BYTE 2 -------------------------------------------

                        0
//                      | SCCB_CFG2_DEVICE_ACTIVE_ONLY_MEASUREMENT
#if defined( FEATURE_CALLED_SPACE_IDENTIFICATION )
                        | SCCB_CFG2_CALLED_SPACE_IDENTIFICATION
#endif
#if defined( FEATURE_CHECKSUM_INSTRUCTION )
                        | SCCB_CFG2_CHECKSUM_INSTRUCTION
#endif
                        ,


// ----------------BYTE 3 -------------------------------------------

                        0
#if defined( FEATURE_RESUME_PROGRAM )
                        | SCCB_CFG3_RESUME_PROGRAM
#endif
#if defined( FEATURE_PERFORM_LOCKED_OPERATION )
                        | SCCB_CFG3_PERFORM_LOCKED_OPERATION
#endif
#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
                        | SCCB_CFG3_IMMEDIATE_AND_RELATIVE
#endif
#if defined( FEATURE_COMPARE_AND_MOVE_EXTENDED )
                        | SCCB_CFG3_COMPARE_AND_MOVE_EXTENDED
#endif
#if defined( FEATURE_BRANCH_AND_SET_AUTHORITY )
                        | SCCB_CFG3_BRANCH_AND_SET_AUTHORITY
#endif
#if defined( FEATURE_BASIC_FP_EXTENSIONS )
                        | SCCB_CFG3_EXTENDED_FLOATING_POINT
#endif
/*ZZ*/                  | SCCB_CFG3_EXTENDED_LOGICAL_COMPUTATION_FACILITY
                        ,


// ----------------BYTE 4 -------------------------------------------

                        0
#if defined( FEATURE_EXTENDED_TOD_CLOCK )
                        | SCCB_CFG4_EXTENDED_TOD_CLOCK
#endif
#if defined( FEATURE_EXTENDED_TRANSLATION_FACILITY_1 )
                        | SCCB_CFG4_EXTENDED_TRANSLATION
#endif
#if defined( FEATURE_000_N3_INSTR_FACILITY )
                        | SCCB_CFG4_LOAD_REVERSED_FACILITY
#endif
#if defined( FEATURE_016_EXT_TRANSL_FACILITY_2 )
                        | SCCB_CFG4_EXTENDED_TRANSLATION_FACILITY2
#endif
#if defined( FEATURE_STORE_SYSTEM_INFORMATION )
                        | SCCB_CFG4_STORE_SYSTEM_INFORMATION
#endif
//                      | SCCB_CFG4_LPAR_CLUSTERING
                        | SCCB_CFG4_IFA_FACILITY
                        ,


// ----------------BYTE 5 -------------------------------------------

                        0
#if defined( FEATURE_009_SENSE_RUN_STATUS_FACILITY )
                        | SCCB_CFG5_SENSE_RUNNING_STATUS
#endif
};

/*-------------------------------------------------------------------*/

BYTE  ARCH_DEP( scpinfo_cfg11 ) =

                            0
#if defined( FEATURE_PER3 )
                            | SCCB_CFGB_PER_3
#endif
                            | SCCB_CFGB_LIST_DIRECTED_IPL
                            ;

/*-------------------------------------------------------------------*/

BYTE  ARCH_DEP( scpinfo_cpf )[12] =
{
                            0
#if defined( FEATURE_SIE )
#if defined( _370 ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                            | SCCB_CPF0_SIE_370_MODE
#endif
                            | SCCB_CPF0_SIE_XA_MODE
#endif /* defined( FEATURE_SIE ) */
//                          | SCCB_CPF0_SIE_SET_II_370_MODE
#if defined( FEATURE_IO_ASSIST )
                            | SCCB_CPF0_SIE_SET_II_XA_MODE
#endif
#if defined( FEATURE_SIE )
                            | SCCB_CPF0_SIE_NEW_INTERCEPT_FORMAT
#endif
#if defined( FEATURE_STORAGE_KEY_ASSIST )
                            | SCCB_CPF0_STORAGE_KEY_ASSIST
#endif
#if defined( _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
                            | SCCB_CPF0_MULTIPLE_CONTROLLED_DATA_SPACE
#endif
                            ,
                            0
#if defined( FEATURE_IO_ASSIST )
                            | SCCB_CPF1_IO_INTERPRETATION_LEVEL_2
#endif
#if defined( FEATURE_SIE )
                            | SCCB_CPF1_GUEST_PER_ENHANCED
#endif
//                          | SCCB_CPF1_SIGP_INTERPRETATION_ASSIST
#if defined( FEATURE_STORAGE_KEY_ASSIST )
                            | SCCB_CPF1_RCP_BYPASS_FACILITY
#endif
#if defined( FEATURE_REGION_RELOCATE )
                            | SCCB_CPF1_REGION_RELOCATE_FACILITY
#endif
#if defined( FEATURE_EXPEDITED_SIE_SUBSET )
                            | SCCB_CPF1_EXPEDITE_TIMER_PROCESSING
#endif
                            ,
                            0
#if defined( FEATURE_CRYPTO )
                            | SCCB_CPF2_CRYPTO_FEATURE_ACCESSED
#endif
#if defined( FEATURE_EXPEDITED_SIE_SUBSET )
                            | SCCB_CPF2_EXPEDITE_RUN_PROCESSING
#endif
                            ,
                            0
#if defined( FEATURE_PRIVATE_SPACE )
                            | SCCB_CPF3_PRIVATE_SPACE_FEATURE
                            | SCCB_CPF3_FETCH_ONLY_BIT
#endif
#if defined( FEATURE_PER2 )
                            | SCCB_CPF3_PER2_INSTALLED
#endif
                            ,
                            0
#if defined( FEATURE_PER2 )
                            | SCCB_CPF4_OMISSION_GR_ALTERATION_370
#endif
                            ,
                            0
#if defined( FEATURE_WAITSTATE_ASSIST )
                            | SCCB_CPF5_GUEST_WAIT_STATE_ASSIST
#endif
                            ,
                            0, 0, 0, 0, 0, 0
};

/*-------------------------------------------------------------------*/

static const U32  ARCH_DEP( sclp_recv_mask ) =

                              0
                              | EVDMASK(MSG)
                              | EVDMASK(PRIOR)
#if defined( FEATURE_SCEDIO )
                              | EVDMASK(SCEDIO)
#endif
#if defined( FEATURE_HARDWARE_LOADER )
                              | EVDMASK(HWL)
                              | EVDMASK(SDIAS)
#endif
#if defined( FEATURE_INTEGRATED_ASCII_CONSOLE )
                              | EVDMASK(VT220)
#endif
#if defined( FEATURE_INTEGRATED_3270_CONSOLE )
                              | EVDMASK(SYSG)
#endif
                              | EVDMASK(CPIDENT)
                              ;

/*-------------------------------------------------------------------*/

static const U32  ARCH_DEP( sclp_send_mask ) =

                              0
                              | EVDMASK(OPCMD)
                              | EVDMASK(STATECH)
                              | EVDMASK(PRIOR)
                              | EVDMASK(SIGQ)
#if defined( FEATURE_SCEDIO )
                              | EVDMASK(SCEDIO)
#endif
#if defined( _FEATURE_HARDWARE_LOADER )
                              | EVDMASK(HWL)
                              | EVDMASK(SDIAS)
#endif
#if defined( FEATURE_INTEGRATED_ASCII_CONSOLE )
                              | EVDMASK(VT220)
#endif
#if defined( FEATURE_INTEGRATED_3270_CONSOLE )
                              | EVDMASK(SYSG)
#endif
                              | EVDMASK(CPCMD)
                              ;

/*-------------------------------------------------------------------*/
/* B220 SERVC - Service Call                                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( service_call )
{
U32             r1, r2;                 /* Values of R fields        */
U32             sclp_command;           /* SCLP command code         */
U32             sccb_real_addr;         /* SCCB real address         */
int             i;                      /* Array subscripts          */
U32             realinc;                /* Storage size in increments*/
U32             incsizemb;              /* Increment size in MB      */
U32             sccb_absolute_addr;     /* Absolute address of SCCB  */
U16             sccblen;                /* Length of SCCB            */
SCCB_HEADER*    sccb;                   /* -> SCCB header            */
SCCB_SCP_INFO*  sccbscp;                /* -> SCCB SCP information   */
SCCB_CPU_INFO*  sccbcpu;                /* -> SCCB CPU information   */

#if defined( FEATURE_MPF_INFO )
SCCB_MPF_INFO*  sccbmpf;                /* -> SCCB MPF information   */
#endif

#if defined( FEATURE_CHANNEL_SUBSYSTEM )
SCCB_CHP_INFO*  sccbchp;                /* -> SCCB channel path info */
#else
SCCB_CHSET_INFO* sccbchp;               /* -> SCCB channel path info */
#endif

SCCB_CSI_INFO*  sccbcsi;                /* -> SCCB channel subsys inf*/
U16             offset;                 /* Offset from start of SCCB */

#if defined( FEATURE_CHANNEL_SUBSYSTEM )
DEVBLK*         dev;                    /* Used to find CHPIDs       */
U32             chpbyte;                /* Offset to byte for CHPID  */
U32             chpbit;                 /* Bit number for CHPID      */
#endif

#if defined( FEATURE_SYSTEM_CONSOLE )
SCCB_EVT_MASK*  evd_mask;               /* Event mask                */
SCCB_EVD_HDR*   evd_hdr;                /* Event header              */
U16             evd_len;                /* Length of event data      */
SCCB_MCD_BK*    mcd_bk;                 /* Message Control Data      */
U16             mcd_len;                /* Length of MCD             */
SCCB_OBJ_HDR*   obj_hdr;                /* Object Header             */
U16             obj_len;                /* Length of Object          */
U16             obj_type;               /* Object type               */
SCCB_MTO_BK*    mto_bk;                 /* Message Text Object       */
BYTE*           evd_msg;                /* Message Text pointer      */
int             event_msglen;           /* Message Text length       */
BYTE            message[MAX_EVENT_MSG_LEN];/* Maximum event data buffer
                                           length plus one for \0    */
U32             masklen;                /* Length of event mask      */
U32             old_cp_recv_mask;       /* Masks before write event  */
U32             old_cp_send_mask;       /*              mask command */
#endif /* defined( FEATURE_SYSTEM_CONSOLE ) */

#if defined( FEATURE_EXPANDED_STORAGE )
SCCB_XST_MAP*   sccbxmap;               /* Xstore usability map      */
U32             xstincnum;              /* Number of expanded storage
                                                         increments  */
U32             xstblkinc;              /* Number of expanded storage
                                               blocks per increment  */
BYTE*           xstmap;                 /* Xstore bitmap, zero means
                                                           available */
#endif /* defined( FEATURE_EXPANDED_STORAGE ) */

    RRE( inst, regs, r1, r2 );

    PER_ZEROADDR_CHECK( regs, r2 );
    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    SIE_INTERCEPT( regs );

    PTT_INF("SERVC", regs->GR_L(r1), regs->GR_L(r2), regs->psw.IA_L );

    /* R1 is SCLP command word */
    sclp_command = regs->GR_L(r1);

    /* R2 is real address of service call control block */
    sccb_real_addr = regs->GR_L(r2);

    /* Program check if SCCB is not on a doubleword boundary */
    if (sccb_real_addr & 0x00000007)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Program check if SCCB falls outside of main storage */
    if (sccb_real_addr > (regs->mainlim - sizeof( SCCB_HEADER )))
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

    /* Obtain the absolute address of the SCCB */
    sccb_absolute_addr = APPLY_PREFIXING( sccb_real_addr, regs->PX );

    /* Specification Exception if SCCB not below 2GB */
    if (sccb_absolute_addr >= (0x80000000 - sizeof( SCCB_HEADER )))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Point to service call control block */
    sccb = (SCCB_HEADER*)(regs->mainstor + sccb_absolute_addr);

    /* Load SCCB length from header */
    FETCH_HW( sccblen, sccb->length  );

    /* Set the main storage reference bit */
    ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_REF );

    /* Specification Exception if SCCB size less than 8 */
    if (sccblen < sizeof( SCCB_HEADER ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Program check if SCCB falls outside of main storage,
       isn't below 2GB or overlaps low core or prefix area. */
    {
        U64 sccb_first_byte = (U64) sccb_absolute_addr;
        U64 sccb_last_byte  = (U64) sccb_absolute_addr + sccblen - 1;

        if (0
            || sccb_first_byte < PSA_SIZE
            || sccb_last_byte > 0x80000000
            || (sccb_first_byte >= regs->PX && sccb_first_byte < (regs->PX + PSA_SIZE))
            || (sccb_last_byte  >= regs->PX && sccb_last_byte  < (regs->PX + PSA_SIZE))
        )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

        if (sccb_last_byte > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );
    }

    /* Obtain lock if immediate response is not requested */
    if (!(sccb->flag & SCCB_FLAG_SYNC)
        || (sclp_command & SCLP_COMMAND_CLASS) == 0x01)
    {
        /* Obtain the interrupt lock */
        OBTAIN_INTLOCK( regs );

        /* If a service signal is pending then return condition
           code 2 to indicate that service processor is busy */
        if (IS_IC_SERVSIG && (sysblk.servparm & SERVSIG_ADDR))
        {
            RELEASE_INTLOCK( regs );
            regs->psw.cc = 2;
            return;
        }
    }

    /* Test SCLP command word */
    switch (sclp_command & SCLP_COMMAND_MASK)
    {
    case SCLP_READ_IFL_INFO:

        /* READ_IFL_INFO is only valid for processor type IFL */
        if (sysblk.ptyp[ regs->cpuad ] != SCCB_PTYP_IFL)
            goto invalidcmd;
        else
            goto read_scpinfo;

    case SCLP_READ_SCP_INFO:

        /* READ_SCP_INFO is only valid for processor type CP */
        if (sysblk.ptyp[ regs->cpuad ] != SCCB_PTYP_CP)
        {
            // "The configuration has been placed into a system check-
            //  stop state because of an incompatible service call"
            WRMSG( HHC00005, "W" );
            goto docheckstop;
            /*
             * Replace the following 2 lines with
             * goto invalidcmd
             * if this behavior is not satisfactory
             * ISW 20081221
             */
        }

    read_scpinfo:

        /* Set the main storage change bit */
        ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_CHANGE );

        /* Set response code X'0100' if SCCB crosses a page boundary */
        if ((sccb_absolute_addr & STORAGE_KEY_PAGEMASK) !=
           ((sccb_absolute_addr + sccblen - 1) & STORAGE_KEY_PAGEMASK))
        {
            sccb->reas = SCCB_REAS_NOT_PGBNDRY;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Set response code X'0300' if SCCB length
           is insufficient to contain SCP info */
        if ( sccblen < sizeof( SCCB_HEADER ) + sizeof( SCCB_SCP_INFO )
                + (sizeof( SCCB_CPU_INFO ) * sysblk.maxcpu))
        {
            sccb->reas = SCCB_REAS_TOO_SHORT;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Point to SCCB data area following SCCB header */
        sccbscp = (SCCB_SCP_INFO*)(sccb+1);
        memset( sccbscp, 0, sizeof( SCCB_SCP_INFO ));

        /* Set main storage size in SCCB...
         *
         * PROGRAMMING NOTE: Hercules can support main storage sizes
         * up to slightly less than 16 EB (16384 PB = 16777216 TB),
         * even if the host operating system cannot.
         *
         * The guest architecural limit however is constrained by the
         * width of the realinum and realiszm SCCB fields (number of
         * increments and increment size in MB) which are only 16 bits
         * and 8 bits wide respectively. Thus the guest's maximum
         * storage size is architecturally limited to slightly less
         * than 16 TB (65535 increments * 255 MB increment size).
         *
         * This means if our main storage size is >= 64GB we must set
         * the increment size to a value which ensures the resulting
         * number of increments remains <= 65535.
         */

        ASSERT( sysblk.mainsize <= MAX_SCP_STORSIZE );
        incsizemb = (sysblk.mainsize + (MAX_1MINCR_STORSIZE - 1)) / MAX_1MINCR_STORSIZE;
        realinc = sysblk.mainsize / (incsizemb << SHIFT_MEGABYTE);

        STORE_HW( sccbscp->realinum, realinc );
        sccbscp->realiszm = (incsizemb & 0xFF);
        sccbscp->realbszk = 4;
        STORE_HW( sccbscp->realiint, 1 );

#if defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* SIE supports the full address range */
        sccbscp->maxvm = 0;
        /* realiszm is valid */
        STORE_FW( sccbscp->grzm, 0 );
        /* Number of storage increments installed in esame mode */
        STORE_DW( sccbscp->grnmx, realinc );
#endif

#if defined( FEATURE_EXPANDED_STORAGE )
        /* Set expanded storage size in SCCB */
        xstincnum = sysblk.xpndsize /
                    (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
        STORE_FW( sccbscp->xpndinum, xstincnum );
        xstblkinc = XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT;
        STORE_FW( sccbscp->xpndsz4K, xstblkinc );
#endif

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )
        /* Set the Vector section size in the SCCB */
        STORE_HW( sccbscp->vectssiz, VECTOR_SECTION_SIZE );
        /* Set the Vector partial sum number in the SCCB */
        STORE_HW( sccbscp->vectpsum, VECTOR_PARTIAL_SUM_NUMBER );
#endif
        /* Set CPU array count and offset in SCCB */
        STORE_HW( sccbscp->numcpu, sysblk.maxcpu );
        offset = sizeof( SCCB_HEADER ) + sizeof( SCCB_SCP_INFO );
        STORE_HW( sccbscp->offcpu, offset );

#if defined( FEATURE_MPF_INFO )
        /* Set MPF array count and offset in SCCB */
        STORE_HW( sccbscp->nummpf, sysblk.maxcpu-1 );
#endif
        offset += (U16)sizeof( SCCB_CPU_INFO ) * sysblk.maxcpu;
        STORE_HW( sccbscp->offmpf, offset );

        /* Set HSA array count and offset in SCCB */
        STORE_HW( sccbscp->numhsa, 0 );

#if defined( FEATURE_MPF_INFO )
        offset += (U16)sizeof( SCCB_MPF_INFO ) * sysblk.maxcpu-1;
#endif
        STORE_HW( sccbscp->offhsa, offset );

        /* Build the MPF information array after the CPU info */
        /* Move IPL load parameter to SCCB */
        get_loadparm( sccbscp->loadparm );

        /* Set installed features bit mask in SCCB */
        memcpy( sccbscp->ifm, ARCH_DEP( scpinfo_ifm ), sizeof( sccbscp->ifm ));

        memcpy( sccbscp->cfg, ARCH_DEP( scpinfo_cfg ), sizeof( sccbscp->cfg ));
        /* sccbscp->cfg11 = ARCH_DEP( scpinfo_cfg11 ); */

        /* Turn off bits for facilities that aren't enabled */
        if (!FACILITY_ENABLED( 016_EXT_TRANSL_2, regs ))
            sccbscp->cfg[4] &= ~SCCB_CFG4_EXTENDED_TRANSLATION_FACILITY2;

        if (!FACILITY_ENABLED( 009_SENSE_RUN_STATUS, regs ))
            sccbscp->cfg[5] &= ~SCCB_CFG5_SENSE_RUNNING_STATUS;

        /* Turn on additioal bits for facilities that ARE enabled */
        if (0
#if defined( _FEATURE_HYPERVISOR )
            || FACILITY_ENABLED( HERC_LOGICAL_PARTITION, regs )
#endif
#if defined( _FEATURE_EMULATE_VM )
            || FACILITY_ENABLED( HERC_VIRTUAL_MACHINE, regs )
#endif
        )
            sccbscp->cfg[0] |= SCCB_CFG0_LOGICALLY_PARTITIONED;

#if defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (FACILITY_ENABLED( 001_ZARCH_INSTALLED, regs ))
            sccbscp->cfg[5] |= SCCB_CFG5_ESAME;
#endif

        /* Build the CPU information array after the SCP info */
        sccbcpu = (SCCB_CPU_INFO*)(sccbscp+1);

        for (i=0; i < sysblk.maxcpu; i++, sccbcpu++)
        {
            memset( sccbcpu, 0, sizeof( SCCB_CPU_INFO ));
            sccbcpu->cpa = i;
            sccbcpu->tod = 0;
            memcpy( sccbcpu->cpf, ARCH_DEP( scpinfo_cpf ), sizeof( sccbcpu->cpf ));
            sccbcpu->ptyp = sysblk.ptyp[i];

#if defined( FEATURE_CRYPTO )
//          sccbcpu->ksid = SCCB_KSID_CRYPTO_UNIT_ID;
#endif

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )

            if (IS_CPU_ONLINE(i) && sysblk.regs[i]->vf->online)
                sccbcpu->cpf[2] |= SCCB_CPF2_VECTOR_FEATURE_INSTALLED;
            if (IS_CPU_ONLINE(i) && sysblk.regs[i]->vf->online)
                sccbcpu->cpf[2] |= SCCB_CPF2_VECTOR_FEATURE_CONNECTED;
            if (!IS_CPU_ONLINE(i))
                sccbcpu->cpf[2] |= SCCB_CPF2_VECTOR_FEATURE_STANDBY_STATE;
#endif
        }

#if defined( FEATURE_MPF_INFO )

        /* Define machine capacity */
        STORE_FW( sccbscp->rcci, 10000 );

        /* Fill in the MP Factors array */
        sccbmpf = (SCCB_MPF_INFO*)(sccbcpu);
        get_mpfactors((BYTE*)sccbmpf);
#endif

        /* Set response code X'0010' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_INFO;
        break;

docheckstop:

        ARCH_DEP( checkstop_all_cpus )( regs );
        RELEASE_INTLOCK( regs );
        longjmp( regs->progjmp, SIE_NO_INTERCEPT );
        UNREACHABLE_CODE( return );

    case SCLP_READ_CHP_INFO:

        /* Set the main storage change bit */
        ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_CHANGE );

        /* Set response code X'0100' if SCCB crosses a page boundary */
        if ((sccb_absolute_addr & STORAGE_KEY_PAGEMASK) !=
           ((sccb_absolute_addr + sccblen - 1) & STORAGE_KEY_PAGEMASK))
        {
            sccb->reas = SCCB_REAS_NOT_PGBNDRY;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Set response code X'0300' if SCCB length
           is insufficient to contain channel path info */
        if ( sccblen < sizeof( SCCB_HEADER ) + sizeof( SCCB_CHP_INFO ))
        {
            sccb->reas = SCCB_REAS_TOO_SHORT;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

#if defined( FEATURE_S370_CHANNEL )

        /* Point to SCCB data area following SCCB header */
        sccbchp = (SCCB_CHSET_INFO*)(sccb+1);
        memset( sccbchp, 0, sizeof( SCCB_CHSET_INFO ));
#else
        /* Point to SCCB data area following SCCB header */
        sccbchp = (SCCB_CHP_INFO*)(sccb+1);
        memset( sccbchp, 0, sizeof( SCCB_CHP_INFO ));
#endif

#if defined( FEATURE_CHANNEL_SUBSYSTEM )

        /* Identify CHPIDs installed, standby, and online */
        for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        {
            for (i=0; i < 8; i++)
            {
                chpbyte = dev->pmcw.chpid[i] / 8;
                chpbit  = dev->pmcw.chpid[i] % 8;

                if ( ((0x80 >> i) & dev->pmcw.pim))
                {
                    sccbchp->installed  [ chpbyte ] |= 0x80 >> chpbit;

                    if (dev->pmcw.flag5 & PMCW5_V)
                        sccbchp->online [ chpbyte ] |= 0x80 >> chpbit;
                    else
                        sccbchp->standby[ chpbyte ] |= 0x80 >> chpbit;
                }
            }
        }
#endif

#if defined( FEATURE_S370_CHANNEL )

        /* For S/370, initialize identifiers for channel set 0A */
        for (i=0; i < 16; i++)
        {
            sccbchp->chanset0a[ 2*i + 0 ] = 0x80;
            sccbchp->chanset0a[ 2*i + 1 ] = i;
        }

        /* Set the channel set configuration byte */
        sccbchp->csconfig = 0xC0;

#endif

        /* Set response code X'0010' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_INFO;
        break;

    case SCLP_READ_CSI_INFO:

        /* Set the main storage change bit */
        ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_CHANGE );

        /* Set response code X'0100' if SCCB crosses a page boundary */
        if ((sccb_absolute_addr & STORAGE_KEY_PAGEMASK) !=
            ((sccb_absolute_addr + sccblen - 1) & STORAGE_KEY_PAGEMASK))
        {
            sccb->reas = SCCB_REAS_NOT_PGBNDRY;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Set response code X'0300' if SCCB length
           is insufficient to contain channel path info */
        if ( sccblen < sizeof( SCCB_HEADER ) + sizeof( SCCB_CSI_INFO ))
        {
            sccb->reas = SCCB_REAS_TOO_SHORT;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Point to SCCB data area following SCCB header */
        sccbcsi = (SCCB_CSI_INFO*)(sccb+1);
        memset( sccbcsi, 0, sizeof( SCCB_CSI_INFO ));

        sccbcsi->csif[0] =
            0
#if defined( FEATURE_CANCEL_IO_FACILITY )
            | SCCB_CSI0_CANCEL_IO_REQUEST_FACILITY
#endif
            | SCCB_CSI0_CONCURRENT_SENSE_FACILITY
            ;

        /* Set response code X'0010' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_INFO;
        break;

#if defined( FEATURE_SYSTEM_CONSOLE )

    case SCLP_WRITE_EVENT_DATA:

        /* Set the main storage change bit */
        ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_CHANGE );

        /* Set response code X'0100' if SCCB crosses a page boundary */
        if ((sccb_absolute_addr & STORAGE_KEY_PAGEMASK) !=
            ((sccb_absolute_addr + sccblen - 1) & STORAGE_KEY_PAGEMASK))
        {
            sccb->reas = SCCB_REAS_NOT_PGBNDRY;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Point to SCCB data area following SCCB header */
        evd_hdr = (SCCB_EVD_HDR*)(sccb+1);
        FETCH_HW( evd_len, evd_hdr->totlen );

        switch (evd_hdr->type)
        {
        case SCCB_EVD_TYPE_MSG:
        case SCCB_EVD_TYPE_PRIOR:

            while (sccblen > sizeof( SCCB_HEADER ))
            {
                FETCH_HW( evd_len, evd_hdr->totlen );

                /* Point to the Message Control Data Block */
                mcd_bk = (SCCB_MCD_BK*)(evd_hdr+1);
                FETCH_HW( mcd_len, mcd_bk->length );

                obj_hdr = (SCCB_OBJ_HDR*)(mcd_bk+1);

                while (mcd_len > sizeof( SCCB_MCD_BK ))
                {
                    FETCH_HW( obj_len, obj_hdr->length );

                    if (obj_len == 0)
                    {
                        sccb->reas = SCCB_REAS_BUFF_LEN_ERR;
                        sccb->resp = SCCB_RESP_BUFF_LEN_ERR;
                        break;
                    }

                    FETCH_HW( obj_type, obj_hdr->type );

                    if (obj_type == SCCB_OBJ_TYPE_MESSAGE)
                    {
                        mto_bk = (SCCB_MTO_BK*)(obj_hdr+1);
                        evd_msg = (BYTE*)(mto_bk+1);
                        event_msglen = obj_len -
                                (sizeof( SCCB_OBJ_HDR ) + sizeof( SCCB_MTO_BK ));

                        if (event_msglen < 0)
                        {
                            sccb->reas = SCCB_REAS_BUFF_LEN_ERR;
                            sccb->resp = SCCB_RESP_BUFF_LEN_ERR;
                            break;
                        }

                        /* Make sure we don't overflow our buffer! */
                        if (event_msglen >= (int) sizeof( message ) - 1)
                        {
                            int trunc_len = (int) sizeof( message ) - 1;
                            // "Overly long %d byte SCP message truncated to %d bytes"
                            WRMSG( HHC00159, "W", event_msglen, trunc_len );
                            event_msglen = trunc_len;
                        }

                        /* Print line unless it is a response prompt */
                        if (!(mto_bk->ltflag[0] & SCCB_MTO_LTFLG0_PROMPT))
                        {
                            static bool s390x_linux_detected = false;
                            static bool is_s390x_linux = false;

#if 0 // debug: save raw EBCDIC message for test program
static bool once = false;
static FILE* efile;
U16 u16_len = (U16) event_msglen;
U16 big_endian_u16_len = CSWAP16( u16_len );
if (!once)
{
    once = true;
    efile = fopen( "e_msgs.dat", "wb" );
}
fwrite( &big_endian_u16_len, 1, 2, efile );
fwrite( evd_msg, 1, u16_len, efile );
fflush( efile );
#endif // debug: save raw EBCDIC message for test program

                            /* Try to auto-detect if this is s390x Linux */
                            if (!s390x_linux_detected)
                            {
                                /* Look for EBCDIC 'ESC' character */
                                if (event_msglen && memchr( evd_msg, 0x4A, event_msglen ))
                                {
                                    s390x_linux_detected = true;
                                    is_s390x_linux = true;
                                }
                            }

                            /* If this is s390x Linux... */
                            if (is_s390x_linux)
                            {
                                /* Try to remove terminal escape sequences.
                                   Note: also does LOGMSG as appropriate. */
                                gh534_fix( event_msglen, evd_msg );
                            }
                            else /* Normal processing: show message as-is */
                            {
                                for (i=0; i < event_msglen; i++)
                                {
                                    message[i] = isprint( guest_to_host( evd_msg[i] )) ?
                                        guest_to_host( evd_msg[i] ) : ' ';
                                }
                                message[i] = '\0';
                                LOGMSG("%s\n",message);
                            }
                        }
                    }

                    mcd_len -= obj_len;
                    obj_hdr=(SCCB_OBJ_HDR *)((BYTE*)obj_hdr + obj_len);
                }

                /* Indicate Event Processed */
                evd_hdr->flag |= SCCB_EVD_FLAG_PROC;

                sccblen -= evd_len;
                evd_hdr = (SCCB_EVD_HDR *)((BYTE*)evd_hdr + evd_len);
            }

            /* Set response code X'0020' in SCCB header */
            sccb->reas = SCCB_REAS_NONE;
            sccb->resp = SCCB_RESP_COMPLETE;
            break;

        case SCCB_EVD_TYPE_CPIDENT:
            sclp_cpident(sccb);
            break;

#if defined( FEATURE_SCEDIO )

        case SCCB_EVD_TYPE_SCEDIO:
            ARCH_DEP( sclp_scedio_request )( sccb );
            break;
#endif

#if defined( _FEATURE_HARDWARE_LOADER )

        case SCCB_EVD_TYPE_HWL:
            ARCH_DEP( sclp_hwl_request )( sccb );
            break;

        case SCCB_EVD_TYPE_SDIAS:
            ARCH_DEP( sclp_sdias_request )( sccb );
            break;
#endif

#if defined( FEATURE_INTEGRATED_3270_CONSOLE )

        case SCCB_EVD_TYPE_SYSG:
            sclp_sysg_write( sccb );
            break;
#endif

#if defined( FEATURE_INTEGRATED_ASCII_CONSOLE )

        case SCCB_EVD_TYPE_VT220:
            sclp_sysa_write( sccb );
            break;
#endif

        default:

            PTT_ERR("*SERVC", regs->GR_L(r1), regs->GR_L(r2), evd_hdr->type );

            if (HDC3( debug_sclp_unknown_event, evd_hdr, sccb, regs ))
                break;

            /* Set response code X'73F0' in SCCB header */
            sccb->reas = SCCB_REAS_SYNTAX_ERROR;
            sccb->resp = SCCB_RESP_SYNTAX_ERROR;
            break;
        }
        break;

    case SCLP_READ_EVENT_DATA:

        /* Set the main storage change bit */
        ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_CHANGE );

        /* Set response code X'0100' if SCCB crosses a page boundary */
        if ((sccb_absolute_addr & STORAGE_KEY_PAGEMASK) !=
           ((sccb_absolute_addr + sccblen - 1) & STORAGE_KEY_PAGEMASK))
        {
            sccb->reas = SCCB_REAS_NOT_PGBNDRY;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Point to SCCB data area following SCCB header */
        evd_hdr = (SCCB_EVD_HDR*)(sccb+1);

        if (SCLP_RECV_ENABLED(               PRIOR ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_PRIOR ))
        {
            sclp_opcmd_event(sccb, SCCB_EVD_TYPE_PRIOR);
            break;
        }

        if (SCLP_RECV_ENABLED(               OPCMD ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_OPCMD ))
        {
            sclp_opcmd_event(sccb, SCCB_EVD_TYPE_OPCMD);
            break;
        }

#if defined( FEATURE_SCEDIO )

        if (SCLP_RECV_ENABLED(               SCEDIO ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_SCEDIO ))
        {
            ARCH_DEP( sclp_scedio_event )( sccb );
            break;
        }
#endif

#if defined( _FEATURE_HARDWARE_LOADER )

        if (SCLP_RECV_ENABLED(               HWL ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_HWL ))
        {
            ARCH_DEP( sclp_hwl_event )( sccb );
            break;
        }

        if (SCLP_RECV_ENABLED(               SDIAS ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_SDIAS ))
        {
            ARCH_DEP( sclp_sdias_event )( sccb );
            break;
        }
#endif

#if defined( FEATURE_INTEGRATED_3270_CONSOLE )

        if (SCLP_RECV_ENABLED(               SYSG ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_SYSG ))
        {
            sclp_sysg_poll( sccb );
            break;
        }
#endif

#if defined( FEATURE_INTEGRATED_ASCII_CONSOLE )

        if (SCLP_RECV_ENABLED(               VT220 ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_VT220 ))
        {
            sclp_sysa_poll( sccb );
            break;
        }
#endif

        if (SCLP_RECV_ENABLED(               SIGQ ) &&
            sclp_attn_pending( SCCB_EVD_TYPE_SIGQ ))
        {
            sclp_sigq_event( sccb );
            break;
        }

        PTT_ERR("*SERVC", regs->GR_L(r1), regs->GR_L(r2), regs->psw.IA_L );

        if (HDC3( debug_sclp_event_data, evd_hdr, sccb, regs ))
            break;

        /* Set response code X'62F0' if events are pending but suppressed */
        if (sclp_attn_pending( 0 ))
        {
            sccb->reas = SCCB_REAS_EVENTS_SUP;
            sccb->resp = SCCB_RESP_EVENTS_SUP;
            break;
        }
        else
        {
            /* Set response code X'60F0' if no outstanding events */
            sccb->reas = SCCB_REAS_NO_EVENTS;
            sccb->resp = SCCB_RESP_NO_EVENTS;
        }
        break;

    case SCLP_WRITE_EVENT_MASK:

        /* Set the main storage change bit */
        ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_CHANGE );

        /* Set response code X'0100' if SCCB crosses a page boundary */
        if ((sccb_absolute_addr & STORAGE_KEY_PAGEMASK) !=
           ((sccb_absolute_addr + sccblen - 1) & STORAGE_KEY_PAGEMASK))
        {
            sccb->reas = SCCB_REAS_NOT_PGBNDRY;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Point to SCCB data area following SCCB header */
        evd_mask = (SCCB_EVT_MASK*)(sccb+1);

        /* Get length of single mask field */
        FETCH_HW( masklen, evd_mask->length );

        /* Save old mask settings in order to suppress superflous messages */
        old_cp_recv_mask = servc_cp_recv_mask & ARCH_DEP( sclp_send_mask ) & SCCB_EVENT_CONS_RECV_MASK;
        old_cp_send_mask = servc_cp_send_mask & ARCH_DEP( sclp_recv_mask ) & SCCB_EVENT_CONS_SEND_MASK;

        for (i=0; i < 4; i++)
        {
            servc_cp_recv_mask <<= 8;
            servc_cp_send_mask <<= 8;

            if ((U32)i < masklen)
            {
                servc_cp_recv_mask |= evd_mask->masks[ i+(0*masklen) ];
                servc_cp_send_mask |= evd_mask->masks[ i+(1*masklen) ];
            }
        }

        if (0
            || (servc_cp_recv_mask & ~ARCH_DEP( sclp_recv_mask ))
            || (servc_cp_send_mask & ~ARCH_DEP( sclp_send_mask ))
        )
        {
            HDC3( debug_sclp_unknown_event_mask, evd_mask, sccb, regs );
        }

        /* Write the events that we support back */
        memset( &evd_mask->masks[ 2*masklen ], 0, 2 * masklen );

        for (i=0; (i < 4) && ((U32)i < masklen); i++)
        {
            evd_mask->masks[ i+(2*masklen) ] |= (ARCH_DEP( sclp_recv_mask ) >> ((3-i)*8)) & 0xFF;
            evd_mask->masks[ i+(3*masklen) ] |= (ARCH_DEP( sclp_send_mask ) >> ((3-i)*8)) & 0xFF;
        }

        /* Issue message only when supported mask has changed */
        if (0
            || (servc_cp_recv_mask & ARCH_DEP( sclp_send_mask ) & SCCB_EVENT_CONS_RECV_MASK) != old_cp_recv_mask
            || (servc_cp_send_mask & ARCH_DEP( sclp_recv_mask ) & SCCB_EVENT_CONS_SEND_MASK) != old_cp_send_mask
        )
        {
            if (0
                || (servc_cp_recv_mask & SCCB_EVENT_CONS_RECV_MASK) != 0
                || (servc_cp_send_mask & SCCB_EVENT_CONS_SEND_MASK) != 0
            )
                // "SCLP console interface %s"
                WRMSG( HHC00006, "I", "active" );
            else
                // "SCLP console interface %s"
                WRMSG( HHC00006, "I", "inactive" );
        }

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
        break;

#endif /* defined( FEATURE_SYSTEM_CONSOLE ) */

#if defined( FEATURE_EXPANDED_STORAGE )

   case SCLP_READ_XST_MAP:

        /* Set the main storage change bit */
        ARCH_DEP( or_storage_key )( sccb_absolute_addr, STORKEY_CHANGE );

        /* Set response code X'0100' if SCCB crosses a page boundary */
        if ((sccb_absolute_addr & STORAGE_KEY_PAGEMASK) !=
            ((sccb_absolute_addr + sccblen - 1) & STORAGE_KEY_PAGEMASK))
        {
            sccb->reas = SCCB_REAS_NOT_PGBNDRY;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Calculate number of blocks per increment */
        xstblkinc = XSTORE_INCREMENT_SIZE / XSTORE_PAGESIZE;

        /* Set response code X'0300' if SCCB length
           is insufficient to contain xstore info */
        if ( sccblen < sizeof( SCCB_HEADER ) + sizeof( SCCB_XST_MAP )
                + xstblkinc/8)
        {
            sccb->reas = SCCB_REAS_TOO_SHORT;
            sccb->resp = SCCB_RESP_BLOCK_ERROR;
            break;
        }

        /* Point to SCCB data area following SCCB header */
        sccbxmap = (SCCB_XST_MAP*)(sccb+1);

        /* Verify expanded storage increment number */
        xstincnum = sysblk.xpndsize /
                    (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
        FETCH_FW( i, sccbxmap->incnum );

        if (i < 1 || (U32)i > xstincnum)
        {
            sccb->reas = SCCB_REAS_INVALID_RSC;
            sccb->resp = SCCB_RESP_REJECT;
            break;
        }

        /* Point to bitmap */
        xstmap = (BYTE*)(sccbxmap+1);

        /* Set all blocks available */
        memset( xstmap, 0, xstblkinc/8);

        /* Set response code X'0010' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_INFO;
        break;

#endif /* defined( FEATURE_EXPANDED_STORAGE ) */

#if defined( FEATURE_CPU_RECONFIG )

    case SCLP_CONFIGURE_CPU:

        i = (sclp_command & SCLP_RESOURCE_MASK) >> SCLP_RESOURCE_SHIFT;

        /* Return invalid resource in parm if target does not exist */
        if (i >= sysblk.maxcpu)
        {
            sccb->reas = SCCB_REAS_INVALID_RSCP;
            sccb->resp = SCCB_RESP_REJECT;
            break;
        }

        /* Add cpu to the configuration */
        configure_cpu(i);

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
        break;

    case SCLP_DECONFIGURE_CPU:

        i = (sclp_command & SCLP_RESOURCE_MASK) >> SCLP_RESOURCE_SHIFT;

        /* Return invalid resource in parm if target does not exist */
        if (i >= sysblk.maxcpu)
        {
            sccb->reas = SCCB_REAS_INVALID_RSCP;
            sccb->resp = SCCB_RESP_REJECT;
            break;
        }

        /* Take cpu out of the configuration */
        deconfigure_cpu(i);

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
        break;

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )

    case SCLP_DISCONNECT_VF:

        i = (sclp_command & SCLP_RESOURCE_MASK) >> SCLP_RESOURCE_SHIFT;

        /* Return invalid resource in parm if target does not exist */
        if (i >= sysblk.maxcpu || !IS_CPU_ONLINE(i))
        {
            sccb->reas = SCCB_REAS_INVALID_RSCP;
            sccb->resp = SCCB_RESP_REJECT;
            break;
        }

        if (sysblk.regs[i]->vf->online)
            // "Processor %s%02X: vector facility configured %s"
            WRMSG( HHC00821, "I", PTYPSTR(i), i, "offline" );

        /* Take the VF out of the configuration */
        sysblk.regs[i]->vf->online = 0;

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
        break;

    case SCLP_CONNECT_VF:

        i = (sclp_command & SCLP_RESOURCE_MASK) >> SCLP_RESOURCE_SHIFT;

        /* Return invalid resource in parm if target does not exist */
        if (i >= sysblk.maxcpu)
        {
            sccb->reas = SCCB_REAS_INVALID_RSCP;
            sccb->resp = SCCB_RESP_REJECT;
            break;
        }

        /* Return improper state if associated cpu is offline */
        if (!IS_CPU_ONLINE(i))
        {
            sccb->reas = SCCB_REAS_IMPROPER_RSC;
            sccb->resp = SCCB_RESP_REJECT;
            break;
        }

        if (!sysblk.regs[i]->vf->online)
            // "Processor %s%02X: vector facility configured %s"
            WRMSG( HHC00821, "I", PTYPSTR(i), i, "online" );

        /* Mark the VF online to the CPU */
        sysblk.regs[i]->vf->online = 1;

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
        break;

#endif /* defined( FEATURE_S370_S390_VECTOR_FACILITY ) */

#endif /* defined( FEATURE_CPU_RECONFIG ) */

    default:
    invalidcmd:

        PTT( PTT_CL_INF | PTT_CL_ERR, "*SERVC", regs->GR_L(r1), regs->GR_L(r2), regs->psw.IA_L );

        if (HDC3( debug_sclp_unknown_command, sclp_command, sccb, regs ))
            break;

        /* Set response code X'01F0' for invalid SCLP command */
        sccb->reas = SCCB_REAS_INVALID_CMD;
        sccb->resp = SCCB_RESP_REJECT;
        break;

    } /* end switch (sclp_command) */

    /* If immediate response is requested, return condition code 1 */
    if ((sccb->flag & SCCB_FLAG_SYNC)
        && (sclp_command & SCLP_COMMAND_CLASS) != 0x01)
    {
        regs->psw.cc = 1;
        return;
    }

    /* Set service signal external interrupt pending */
    sysblk.servparm &= ~SERVSIG_ADDR;
    sysblk.servparm |= sccb_absolute_addr;
    ON_IC_SERVSIG;

    /* Release the interrupt lock */
    RELEASE_INTLOCK( regs );

    /* Set condition code 0 */
    regs->psw.cc = 0;

    RETURN_INTCHECK( regs );
} /* end function service_call */


#endif /* defined( FEATURE_SERVICE_PROCESSOR ) */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "service.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "service.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#endif /* !defined( _GEN_ARCH ) */
