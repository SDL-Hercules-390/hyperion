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

#ifdef HAVE_PLATFORM_H 
  #include "platform.h" 
#endif
#include "telnet.h"

/*-------------------------------------------------------------------*/
/*              Helper macros for error handling                     */
/*-------------------------------------------------------------------*/
#define FATAL       1                   /* TELNET_EV_ERROR           */
#define NON_FATAL   0                   /* TELNET_EV_WARNING         */
#define ERR(t,e,f,m,...)    _error( (t), __LINE__, __FUNCTION__,  \
                                    (e), (f), (m), ## __VA_ARGS__ );

/*-------------------------------------------------------------------*/
/*             Helper for Q-method option tracking                   */
/*-------------------------------------------------------------------*/
#define Q_US(q)             (((q).state & 0xF0) >> 4)
#define Q_THEM(q)           (((q).state & 0x0F))
#define Q_MAKE(us,them)     (((us) << 4) | (them))

/*-------------------------------------------------------------------*/
/*             Helper for the negotiation routines                   */
/*-------------------------------------------------------------------*/
#define NEGOTIATION_EVENT( telnet, cmd, opt )   \
                                                \
    ev.type       = (cmd);                      \
    ev.neg.telopt = (opt);                      \
                                                \
    (telnet)->eh( (telnet), &ev, (telnet)->ud );

/*-------------------------------------------------------------------*/
/*                       Buffer sizes                                */
/*-------------------------------------------------------------------*/
static const unsigned int _buffer_sizes[] =
{
    0,
    512,
    2048,
    8192,
    16384
};
static const unsigned int _num_buffer_sizes = sizeof(_buffer_sizes)
                                            / sizeof(_buffer_sizes[0]);

/*-------------------------------------------------------------------*/
/*                   Error generation function                       */
/*-------------------------------------------------------------------*/
static telnet_error_t  _error
(
    telnet_t*       telnet,
    unsigned        line,
    const char*     func,
    telnet_error_t  err,
    int             fatal,
    const char*     fmt,
                    ...
)
{
    telnet_event_t  ev;
    char            buffer[512];
    va_list         va;

    /* Format informational text */
    va_start( va, fmt );
    vsnprintf( buffer, sizeof( buffer ), fmt, va );
    va_end( va );

    /* Send error event to the user */

    ev.error.type  =  (FATAL == fatal) ? TELNET_EV_ERROR
                                       : TELNET_EV_WARNING;
    ev.error.file  =  __FILE__;  // (duh!)
    ev.error.func  =  func;
    ev.error.line  =  line;
    ev.error.msg   =  buffer;
    ev.error.err   =  err;

    telnet->eh( telnet, &ev, telnet->ud );

    return err;
}

/*-------------------------------------------------------------------*/
/*                  Send bytes to other side                         */
/*-------------------------------------------------------------------*/
static void _send
(
    telnet_t*     telnet,   // Telnet state tracker object.
    const BYTE*   buffer,   // Pointer to byte buffer.
    unsigned int  size      // Number of bytes pointed to by buffer.
)
{
    telnet_event_t  ev;

    ev.type         =  TELNET_EV_SEND;
    ev.data.buffer  =  buffer;
    ev.data.size    =  size;

    telnet->eh( telnet, &ev, telnet->ud );
}

/*-------------------------------------------------------------------*/
/*     Send option negotiation request/response to other side        */
/*-------------------------------------------------------------------*/
static void  _send_negotiation
(
    telnet_t*   telnet,
    BYTE        cmd,
    BYTE        telopt
)
{
    BYTE  bytes[3];

    bytes[0] = TELNET_IAC;
    bytes[1] = cmd;             // (WILL/WONT/DO/DONT)
    bytes[2] = telopt;

    _send( telnet, bytes, 3 );
}

/*-------------------------------------------------------------------*/
/*                      _check_telopt                                */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Check if a particular telopt is supported.  If us is non-zero,    */
/* we check if we support it.  Otherwise we check if they support    */
/* it.  Returns 1 (true) if supported, else 0 (false/unsupported).   */
/*                                                                   */
/*-------------------------------------------------------------------*/

#define _we_support_telopt(t,o)     _check_telopt( (t), (o), 1 )
#define _they_support_telopt(t,o)   _check_telopt( (t), (o), 0 )

static int  _check_telopt
(
    telnet_t*   telnet,
    BYTE        telopt,
    int         us
)
{
    int i;

    /* If we have no telopts table, we obviously don't support it */

    if (telnet->telopts == 0)
        return 0;

    /* Loop until found or end marker reached */

    for (i=0; telnet->telopts[i].telopt != 0xFF ||
              telnet->telopts[i].us     != 0x00 ||
              telnet->telopts[i].them   != 0x00; ++i)
    {
        if (telnet->telopts[i].telopt == telopt)
        {
            if (us && telnet->telopts[i].us == TELNET_WILL)
                return 1;
            else if (!us && telnet->telopts[i].them == TELNET_DO)
                return 1;  /* Supported */
            else
                return 0;  /* Not supported */
        }
    }

    return 0;   /* Not found, so not supported */
}

/*-------------------------------------------------------------------*/
/*                        _env_telnet                                */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Process an ENVIRON/NEW-ENVIRON subnegotiation buffer.             */
/*                                                                   */
/* The algorithm and approach used here is kind of a hack, but       */
/* it reduces the number of memory allocations we have to make.      */
/*                                                                   */
/* We copy the bytes back into the buffer, starting at the very      */
/* beginning, which makes it easy to handle the ENVIRON ESC          */
/* escape mechanism as well as ensure the variable name and          */
/* value strings are NUL-terminated, all while fitting inside        */
/* of the original buffer.                                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int _env_telnet
(
    telnet_t*      telnet,
    BYTE           type,
    BYTE*          buffer,
    unsigned int   size
)
{
    telnet_event_t            ev;
    struct telnet_environ_t*  values = 0;

    BYTE *c, *last, *out;
    unsigned int index, count;

    /* If we have no data, just pass it through */

    if (size == 0)
        return 0;

    /* First byte must be a valid command */

    if (1
        && (unsigned)buffer[0] != TELNET_ENVIRON_SEND
        && (unsigned)buffer[0] != TELNET_ENVIRON_IS
        && (unsigned)buffer[0] != TELNET_ENVIRON_INFO
    )
    {
        ERR( telnet, TELNET_EPROTOCOL, NON_FATAL,
             "telopt %d subneg has invalid command", type );
        return 0;
    }

    /* Store ENVIRON command */
    ev.env.cmd = buffer[0];

    /* If we have no arguments, send an event with no data end return */

    if (size == 1)
    {
        ev.type       = TELNET_EV_ENVIRON;
        ev.env.values = 0;
        ev.env.size   = 0;

        telnet->eh( telnet, &ev, telnet->ud );
        return 1;
    }

    /* Very second byte must be VAR or USERVAR, if present */

    if (1
        && (unsigned)buffer[1] != TELNET_ENVIRON_VAR
        && (unsigned)buffer[1] != TELNET_ENVIRON_USERVAR
    )
    {
        ERR( telnet, TELNET_EPROTOCOL, NON_FATAL,
             "telopt %d subneg missing variable type", type );
        return 0;
    }

    /* Ensure last byte is not an escape byte (makes parsing easier) */

    if ((unsigned)buffer[size - 1] == TELNET_ENVIRON_ESC)
    {
        ERR( telnet, TELNET_EPROTOCOL, NON_FATAL,
             "telopt %d subneg ends with ESC", type );
        return 0;
    }

    /* Count arguments; each valid entry starts with VAR or USERVAR */

    for (count=0, c = buffer + 1; c < (buffer + size); ++c)
    {
        if (0
            || *c == TELNET_ENVIRON_VAR
            || *c == TELNET_ENVIRON_USERVAR
        )
        {
            ++count;
        }
        else if (*c == TELNET_ENVIRON_ESC)
        {
            ++c;  /* skip the next byte */
        }
    }

    /* Allocate argument array, bail on error */
    if ((values = (struct telnet_environ_t*) calloc( count,
            sizeof(struct telnet_environ_t))) == 0)
    {
        ERR( telnet, TELNET_ENOMEM, FATAL,
             "calloc() failed: %s", strerror( errno ));
        return 0;
    }

    /* Parse argument array strings... */

    out = buffer;
    c = buffer + 1;

    for (index = 0; index < count; ++index)
    {
        /* Remember the variable type (will be VAR or USERVAR) */

        values[index].type = *c++;

        /* Scan until we find an end-marker, and buffer up any
         * unescaped * bytes into our buffer
         */

        last = out;

        while (c < buffer + size)
        {
            /* Stop at the next variable or at the value */

            if (0
                || (unsigned)*c == TELNET_ENVIRON_VAR
                || (unsigned)*c == TELNET_ENVIRON_VALUE
                || (unsigned)*c == TELNET_ENVIRON_USERVAR
            )
                break;

            /* Buffer next byte (taking into account ESC) */

            if (*c == TELNET_ENVIRON_ESC)
                ++c;

            *out++ = *c++;
        }

        *out++ = '\0';

        /* Store the variable name we have just received */

        values[index].var    = (char*) last;
        values[index].value  = "";

        /* If we got a value, find the next end marker and
         * store the value; otherwise, store empty string */

        if (c < (buffer + size) && *c == TELNET_ENVIRON_VALUE)
        {
            ++c;
            last = out;

            while (c < buffer + size)
            {
                /* Stop when we find the start of the next variable */

                if (0
                    || (unsigned)*c == TELNET_ENVIRON_VAR
                    || (unsigned)*c == TELNET_ENVIRON_USERVAR
                )
                    break;

                /* Buffer next byte (taking into account ESC) */

                if (*c == TELNET_ENVIRON_ESC)
                    ++c;

                *out++ = *c++;
            }

            *out++ = '\0';

            /* Store the variable value */

            values[index].value = (char*) last;
        }
    }

    /* Pass values array and count to event */

    ev.type       = TELNET_EV_ENVIRON;
    ev.env.values = values;
    ev.env.size   = count;

    telnet->eh( telnet, &ev, telnet->ud );

    /* Clean up */
    free( values );
    return 1;
}

/*-------------------------------------------------------------------*/
/*        Parse TERMINAL-TYPE command subnegotiation buffers         */
/*-------------------------------------------------------------------*/
static int _ttype_telnet
(
    telnet_t*     telnet,
    const BYTE*   buffer,
    unsigned int  size
)
{
    telnet_event_t  ev;

    /* Make sure request is not empty */
    if (size == 0)
    {
        ERR( telnet, TELNET_EPROTOCOL, NON_FATAL,
             "incomplete TERMINAL-TYPE request" );
        return 0;
    }

    /* Make sure request has valid command type */

    if (1
        && buffer[0] != TELNET_TTYPE_IS
        && buffer[0] != TELNET_TTYPE_SEND
    )
    {
        ERR( telnet, TELNET_EPROTOCOL, NON_FATAL,
             "TERMINAL-TYPE request has invalid type" );
        return 0;
    }

    /* Send proper event */
    if (buffer[0] == TELNET_TTYPE_IS)
    {
        char* name;

        /* Allocate space for name */
        if ((name = (char*) malloc( size )) == 0)
        {
            ERR( telnet, TELNET_ENOMEM, FATAL,
                 "malloc() failed: %s", strerror( errno ));
            return 0;
        }

        memcpy( name, buffer+1, size-1 );
        name[ size-1 ] = '\0';

        ev.type       = TELNET_EV_TTYPE;
        ev.ttype.cmd  = TELNET_TTYPE_IS;
        ev.ttype.name = name;

        telnet->eh( telnet, &ev, telnet->ud );

        /* Clean up */
        free( name );
    }
    else
    {
        ev.type       = TELNET_EV_TTYPE;
        ev.ttype.cmd  = TELNET_TTYPE_SEND;
        ev.ttype.name = 0;

        telnet->eh( telnet, &ev, telnet->ud );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*                      _subnegotiate                                */
/*-------------------------------------------------------------------*/
static int _subnegotiate( telnet_t* telnet)
{
    telnet_event_t  ev;

    /* Standard subnegotiation event */

    ev.type        =  TELNET_EV_SUBNEGOTIATION;
    ev.sub.telopt  =  telnet->sb_telopt;
    ev.sub.buffer  =  telnet->buffer;
    ev.sub.size    =  telnet->buffer_pos;

    telnet->eh( telnet, &ev, telnet->ud );

    switch (telnet->sb_telopt)
    {
    /* Specially handled subnegotiation telopt types */

    case TELNET_TELOPT_TTYPE:

        return _ttype_telnet( telnet,
                              telnet->buffer, telnet->buffer_pos );

    case TELNET_TELOPT_ENVIRON:
    case TELNET_TELOPT_NEW_ENVIRON:

        return _env_telnet( telnet, telnet->sb_telopt,
                            telnet->buffer, telnet->buffer_pos );

    default:

        return 0;
    }
}

/*-------------------------------------------------------------------*/
/*              Initialize a telnet state tracker                    */
/*-------------------------------------------------------------------*/
telnet_t*  telnet_init
(
    const telnet_telopt_t*  telopts,
    telnet_event_handler_t  eh,
    BYTE                    flags,
    void*                   user_data
)
{
    /* Allocate structure */
    telnet_t*  telnet  = (telnet_t*) calloc( 1, sizeof( telnet_t ));

    if (telnet == 0)
        return 0;

    /* Initialize data */
    telnet->telopts = telopts;
    telnet->eh      = eh;
    telnet->flags   = flags;
    telnet->ud      = user_data;

    /* Kick off negotiations immediately if requested */
    if (flags & TELNET_FLAG_ACTIVE_NEG)
    {
        int  i;
        for (i=0; telopts[i].telopt != 0xFF ||
                  telopts[i].us     != 0x00 ||
                  telopts[i].them   != 0x00; ++i)
        {
            if (TELNET_WILL == telopts[i].us)
                telnet_negotiate( telnet, TELNET_WILL, telopts[i].telopt );

            if (TELNET_DO == telopts[i].them)
                telnet_negotiate( telnet, TELNET_DO, telopts[i].telopt );
        }
    }

    return telnet;
}

/*-------------------------------------------------------------------*/
/*          Free up any memory allocated by a state tracker          */
/*-------------------------------------------------------------------*/
void telnet_free( telnet_t* telnet )
{
    /* Free sub-request buffer */
    if (telnet->buffer)
    {
        free( telnet->buffer );

        telnet->buffer      = 0;
        telnet->buffer_size = 0;
        telnet->buffer_pos  = 0;
    }

    /* Free negotiation queue */
    if (telnet->neg_q)
    {
        free( telnet->neg_q );

        telnet->neg_q      = 0;
        telnet->neg_q_size = 0;
    }

    /* Free the telnet structure itself */
    free( telnet );
}

/*-------------------------------------------------------------------*/
/*                Push a byte into the telnet buffer                 */
/*-------------------------------------------------------------------*/
static telnet_error_t  _buffer_byte( telnet_t* telnet, BYTE byte )
{
    BYTE* new_buffer;
    unsigned int i;

    /* Check if we're out of room */
    if (telnet->buffer_pos >= telnet->buffer_size)
    {
        /* Find the next buffer size */
        for (i=0; i < _num_buffer_sizes; ++i)
        {
            if (_buffer_sizes[i] > telnet->buffer_size)
                break;
        }

        /* Overflow -- can't grow any more */
        if (i >= _num_buffer_sizes)
        {
            ERR( telnet, TELNET_EOVERFLOW, FATAL,
                 "subnegotiation buffer size limit reached" );
            return TELNET_EOVERFLOW;
        }

        /* (Re)Allocate buffer */
        new_buffer = (BYTE*) realloc( telnet->buffer, _buffer_sizes[i]);

        if (new_buffer == 0)
        {
            ERR( telnet, TELNET_ENOMEM, FATAL, "realloc() failed" );
            return TELNET_ENOMEM;
        }

        telnet->buffer      = new_buffer;
        telnet->buffer_size = _buffer_sizes[i];
    }

    /* Push the byte, all set */
    telnet->buffer[telnet->buffer_pos++] = byte;
    return TELNET_EOK;
}

/*-------------------------------------------------------------------*/
/*              Retrieve option negotiation state                    */
/*-------------------------------------------------------------------*/
static telnet_neg_queue_t    _get_neg_qentry
(
    telnet_t*   telnet,
    BYTE        telopt
)
{
    telnet_neg_queue_t empty;
    int i;

    /* Search for entry */

    for (i=0; i < telnet->neg_q_size; ++i)
    {
        if (telnet->neg_q[i].telopt == telopt)
            return telnet->neg_q[i];
    }

    /* Not found, return empty value */

    empty.telopt  =  telopt;
    empty.state   =  0;

    return empty;
}

/*-------------------------------------------------------------------*/
/*                 Update option negotiation state                   */
/*-------------------------------------------------------------------*/
static void  _update_neg_qentry
(
    telnet_t*   telnet,
    BYTE        telopt,
    char        us,
    char        them
)
{
    telnet_neg_queue_t*  qtmp;
    int i;

    /* Search for entry */

    for (i=0; i < telnet->neg_q_size; ++i)
    {
        if (telnet->neg_q[i].telopt == telopt)
        {
            telnet->neg_q[i].state = Q_MAKE( us, them );
            return;
        }
    }

    /* We're going to need to track state for it, so grow
     * the queue by 4 (four) elements and put the telopt
     * into it; bail on allocation error.  We go by four
     * because it seems like a reasonable guess as to the
     * number of enabled options for most simple code, and
     * it allows for an acceptable number of reallocations
     * for complex code.
     */
    if ((qtmp = (telnet_neg_queue_t*) realloc( telnet->neg_q,
            sizeof(telnet_neg_queue_t) * (telnet->neg_q_size + 4))) == 0)
    {
        ERR( telnet, TELNET_ENOMEM, FATAL,
             "realloc() failed: %s", strerror( errno ));
        return;
    }

    memset( &qtmp[telnet->neg_q_size], 0, sizeof(telnet_neg_queue_t) * 4 );

    telnet->neg_q = qtmp;

    telnet->neg_q[ telnet->neg_q_size ].telopt  = telopt;
    telnet->neg_q[ telnet->neg_q_size ].state   = Q_MAKE( us, them );

    telnet->neg_q_size += 4;
}

/*-------------------------------------------------------------------*/
/*                      Send negotiation                             */
/*-------------------------------------------------------------------*/
void telnet_negotiate
(
    telnet_t*   telnet,
    BYTE        cmd,
    BYTE        telopt
)
{
    telnet_neg_queue_t  q;

    /* If we're in proxy mode, just send it now */
    if (telnet->flags & TELNET_FLAG_PROXY_MODE)
    {
        BYTE  bytes[3];

        bytes[0] = TELNET_IAC;
        bytes[1] = cmd;
        bytes[2] = telopt;

        _send( telnet, bytes, 3 );
        return;
    }

    /* Get current option states */
    q = _get_neg_qentry( telnet, telopt );

    switch (cmd)
    {
    /* Advertise willingess to support an option */
    case TELNET_WILL:

        switch (Q_US(q))
        {
        case Q_NO:

            _update_neg_qentry( telnet, telopt, Q_WANTYES, Q_THEM(q) );
            _send_negotiation ( telnet, TELNET_WILL, telopt );
            break;

        case Q_WANTNO:

            _update_neg_qentry( telnet, telopt, Q_WANTNO_OP, Q_THEM(q) );
            break;

        case Q_WANTYES_OP:

            _update_neg_qentry( telnet, telopt, Q_WANTYES, Q_THEM(q) );
            break;
        }
        break;

    /* Force turn-off of locally enabled option */
    case TELNET_WONT:

        switch (Q_US(q))
        {
        case Q_YES:

            _update_neg_qentry( telnet, telopt, Q_WANTNO, Q_THEM(q) );
            _send_negotiation ( telnet, TELNET_WONT, telopt );
            break;

        case Q_WANTYES:

            _update_neg_qentry( telnet, telopt, Q_WANTYES_OP, Q_THEM(q) );
            break;

        case Q_WANTNO_OP:

            _update_neg_qentry( telnet, telopt, Q_WANTNO, Q_THEM(q) );
            break;
        }
        break;

    /* Demand remote end enable an option */
    case TELNET_DO:

        switch (Q_THEM(q))
        {
        case Q_NO:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTYES );
            _send_negotiation ( telnet, TELNET_DO, telopt );
            break;

        case Q_WANTNO:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTNO_OP );
            break;

        case Q_WANTYES_OP:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTYES );
            break;
        }
        break;

    /* Demand remote end disable an option */
    case TELNET_DONT:

        switch (Q_THEM(q))
        {
        case Q_YES:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTNO );
            _send_negotiation ( telnet, TELNET_DONT, telopt );
            break;

        case Q_WANTYES:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTYES_OP );
            break;

        case Q_WANTNO_OP:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTNO );
            break;
        }
        break;
    }
}

/*-------------------------------------------------------------------*/
/*             Process received WILL/WONT/DO/DONT                    */
/*-------------------------------------------------------------------*/
static void _process_received_negotiation
(
    telnet_t*   telnet,
    BYTE        telopt
)
{
    telnet_event_t      ev;
    telnet_neg_queue_t  q;

    /* In PROXY mode, just pass it thru and do nothing */

    if (telnet->flags & TELNET_FLAG_PROXY_MODE)
    {
        switch (telnet->state)
        {
        default:
        /* We're only interested in WILL/WONT/DO/DONT */
        break;
        case TELNET_STATE_WILL:  NEGOTIATION_EVENT( telnet, TELNET_EV_WILL, telopt ); break;
        case TELNET_STATE_WONT:  NEGOTIATION_EVENT( telnet, TELNET_EV_WONT, telopt ); break;
        case TELNET_STATE_DO:    NEGOTIATION_EVENT( telnet, TELNET_EV_DO,   telopt ); break;
        case TELNET_STATE_DONT:  NEGOTIATION_EVENT( telnet, TELNET_EV_DONT, telopt ); break;
        }
        return;
    }

    /* Lookup the current state of the option */
    q = _get_neg_qentry( telnet, telopt );

    /* Start processing... */
    switch (telnet->state)
    {
    default:
        /* We're only interested in WILL/WONT/DO/DONT */
        break;

    /* Request to enable option on remote end or confirm DO */

    case TELNET_STATE_WILL:

        switch (Q_THEM(q))
        {
        case Q_NO:

            if (_they_support_telopt( telnet, telopt ))
            {
                _update_neg_qentry( telnet, telopt, Q_US(q), Q_YES );
                _send_negotiation ( telnet, TELNET_DO,      telopt );
                NEGOTIATION_EVENT ( telnet, TELNET_EV_WILL, telopt );
            }
            else
                _send_negotiation ( telnet, TELNET_DONT,    telopt );
            break;

        case Q_WANTNO:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_NO );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WONT, telopt );

            ERR( telnet, TELNET_ENEGOTIATION, NON_FATAL,
                 "DONT answered by WILL" );
            break;

        case Q_WANTNO_OP:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_YES );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WILL, telopt );

            ERR( telnet, TELNET_ENEGOTIATION, NON_FATAL,
                 "DONT answered by WILL" );
            break;

        case Q_WANTYES:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_YES );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WILL, telopt );
            break;

        case Q_WANTYES_OP:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTNO );
            _send_negotiation ( telnet, TELNET_DONT,    telopt );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WILL, telopt );
            break;
        }
        break;

    /* Request to disable option on remote end, confirm DONT, reject DO */

    case TELNET_STATE_WONT:

        switch (Q_THEM(q))
        {
        case Q_YES:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_NO );
            _send_negotiation ( telnet, TELNET_DONT,    telopt );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WONT, telopt );
            break;

        case Q_WANTNO:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_NO );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WONT, telopt );
            break;

        case Q_WANTNO_OP:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_WANTYES );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_DO, telopt );
            break;

        case Q_WANTYES:
        case Q_WANTYES_OP:

            _update_neg_qentry( telnet, telopt, Q_US(q), Q_NO );
            break;
        }
        break;

    /* Request to enable option on local end or confirm WILL */

    case TELNET_STATE_DO:

        switch (Q_US(q))
        {
        case Q_NO:

            if (_we_support_telopt( telnet, telopt ))
            {
                _update_neg_qentry( telnet, telopt, Q_YES, Q_THEM(q) );
                _send_negotiation ( telnet, TELNET_WILL,  telopt );
                NEGOTIATION_EVENT ( telnet, TELNET_EV_DO, telopt );
            }
            else
                _send_negotiation ( telnet, TELNET_WONT, telopt );
            break;

        case Q_WANTNO:

            _update_neg_qentry( telnet, telopt, Q_NO, Q_THEM(q) );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_DONT, telopt );

            ERR( telnet, TELNET_ENEGOTIATION, NON_FATAL,
                 "WONT answered by DO" );
            break;

        case Q_WANTNO_OP:

            _update_neg_qentry( telnet, telopt, Q_YES, Q_THEM(q) );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_DO, telopt );

            ERR( telnet, TELNET_ENEGOTIATION, NON_FATAL,
                 "WONT answered by DO" );
            break;

        case Q_WANTYES:

            _update_neg_qentry( telnet, telopt, Q_YES, Q_THEM(q) );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_DO, telopt );
            break;

        case Q_WANTYES_OP:

            _update_neg_qentry( telnet, telopt, Q_WANTNO, Q_THEM(q) );
            _send_negotiation ( telnet, TELNET_WONT,  telopt );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_DO, telopt );
            break;
        }
        break;

    /* Request to disable option on local end, confirm WONT, reject WILL */

    case TELNET_STATE_DONT:

        switch (Q_US(q))
        {
        case Q_YES:

            _update_neg_qentry( telnet, telopt, Q_NO, Q_THEM(q) );
            _send_negotiation ( telnet, TELNET_WONT,    telopt );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_DONT, telopt );
            break;

        case Q_WANTNO:

            _update_neg_qentry( telnet, telopt, Q_NO, Q_THEM(q) );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WONT, telopt );
            break;

        case Q_WANTNO_OP:

            _update_neg_qentry( telnet, telopt, Q_WANTYES, Q_THEM(q) );
            _send_negotiation ( telnet, TELNET_WILL,    telopt );
            NEGOTIATION_EVENT ( telnet, TELNET_EV_WILL, telopt );
            break;

        case Q_WANTYES:
        case Q_WANTYES_OP:

            _update_neg_qentry( telnet, telopt, Q_NO, Q_THEM(q) );
            break;
        }
        break;
    }
}

/*-------------------------------------------------------------------*/
/*                   _process_received_data                          */
/*-------------------------------------------------------------------*/
static void _process_received_data
(
    telnet_t*     telnet,
    const BYTE*   buffer,
    unsigned int  size
)
{
    telnet_event_t  ev;
    BYTE            byte;
    unsigned int    start, i;

    for (i=0, start = 0; i < size; ++i)
    {
        byte = buffer[i];

        switch (telnet->state)
        {
        /* Regular data */
        case TELNET_STATE_DATA:

            /* On an IAC byte, pass through all pending bytes
             * and switch states
             */
            if (byte == TELNET_IAC)
            {
                if (i > start)
                {
                    ev.type        = TELNET_EV_DATA;
                    ev.data.buffer = buffer + start;
                    ev.data.size   = i - start;

                    telnet->eh( telnet, &ev, telnet->ud );
                }
                telnet->state = TELNET_STATE_IAC;
            }
            break;

        /* IAC command */
        case TELNET_STATE_IAC:

            switch (byte)
            {
            /* Subnegotiation */
            case TELNET_SB:
                telnet->state = TELNET_STATE_SB;
                break;

            /* Negotiation commands */
            case TELNET_WILL:
                telnet->state = TELNET_STATE_WILL;
                break;

            case TELNET_WONT:
                telnet->state = TELNET_STATE_WONT;
                break;

            case TELNET_DO:
                telnet->state = TELNET_STATE_DO;
                break;

            case TELNET_DONT:
                telnet->state = TELNET_STATE_DONT;
                break;

            /* IAC escaping */
            case TELNET_IAC:
                /* Event */
                ev.type        = TELNET_EV_DATA;
                ev.data.buffer = &byte;
                ev.data.size   = 1;

                telnet->eh( telnet, &ev, telnet->ud );

                /* State update */
                start = i + 1;
                telnet->state = TELNET_STATE_DATA;
                break;

            /* Some other command */
            default:
                /* Event */
                ev.type    = TELNET_EV_IAC;
                ev.iac.cmd = byte;

                telnet->eh( telnet, &ev, telnet->ud );

                /* State update */
                start = i + 1;
                telnet->state = TELNET_STATE_DATA;
            }
            break;

        /* Negotiation commands */
        case TELNET_STATE_WILL:
        case TELNET_STATE_WONT:
        case TELNET_STATE_DO:
        case TELNET_STATE_DONT:

            _process_received_negotiation( telnet, byte );

            start = i + 1;
            telnet->state = TELNET_STATE_DATA;
            break;

        /* Subnegotiation -- determine subnegotiation telopt */
        case TELNET_STATE_SB:

            telnet->sb_telopt  = byte;
            telnet->buffer_pos = 0;
            telnet->state      = TELNET_STATE_SB_DATA;
            break;

        /* Subnegotiation -- buffer bytes until end request */
        case TELNET_STATE_SB_DATA:

            /* IAC command in subnegotiation -- either IAC SE or IAC IAC */
            if (byte == TELNET_IAC)
            {
                telnet->state = TELNET_STATE_SB_DATA_IAC;
            }
            /* Buffer the byte, or bail if we can't */
            else if (_buffer_byte( telnet, byte ) != TELNET_EOK)
            {
                start = i + 1;
                telnet->state = TELNET_STATE_DATA;
            }
            break;

        /* IAC escaping inside a subnegotiation */
        case TELNET_STATE_SB_DATA_IAC:

            switch (byte)
            {
            /* End subnegotiation */
            case TELNET_SE:

                /* Return to default state */
                start = i + 1;
                telnet->state = TELNET_STATE_DATA;

                /* Process subnegotiation */
                if (_subnegotiate( telnet ) != 0)
                {
                    telnet_recv( telnet, &buffer[start], size - start );
                    return;
                }
                break;

            /* Escaped IAC byte */
            case TELNET_IAC:

                /* Push IAC into buffer */
                if (_buffer_byte( telnet, TELNET_IAC ) != TELNET_EOK)
                {
                    start = i + 1;
                    telnet->state = TELNET_STATE_DATA;
                }
                else
                {
                    telnet->state = TELNET_STATE_SB_DATA;
                }
                break;

            /* Something else -- protocol error.  Attempt to process
             * content in subnegotiation buffer, then evaluate the
             * given command as an IAC code.
             */
            default:

                ERR( telnet, TELNET_EPROTOCOL, NON_FATAL,
                     "unexpected byte after IAC inside SB: %d", byte );

                /* Enter IAC state */
                start = i + 1;
                telnet->state = TELNET_STATE_IAC;

                /* Process subnegotiation */
                if (_subnegotiate( telnet ) != 0)
                {
                    telnet_recv( telnet, &buffer[start], size - start );
                    return;
                }
                else
                {
                    /* Pecursive call to get the current input byte processed
                     * as a regular IAC command.  we could use a goto, but
                     * that would be gross.
                     */
                    _process_received_data(telnet, &byte, 1);
                }
                break;
            }
            break;
        }
    }

    /* Pass through any remaining bytes */
    if (telnet->state == TELNET_STATE_DATA && i > start)
    {
        ev.type        = TELNET_EV_DATA;
        ev.data.buffer = buffer + start;
        ev.data.size   = i - start;

        telnet->eh( telnet, &ev, telnet->ud );
    }
}

/*-------------------------------------------------------------------*/
/*             Push some bytes into the state tracker                */
/*-------------------------------------------------------------------*/
void telnet_recv
(
    telnet_t*     telnet,
    const BYTE*   buffer,
    unsigned int  size
)
{
    _process_received_data( telnet, buffer, size );
}

/*-------------------------------------------------------------------*/
/*                     Send an IAC command                           */
/*-------------------------------------------------------------------*/
void telnet_iac( telnet_t* telnet, BYTE cmd )
{
    BYTE  bytes[2];

    bytes[0] = TELNET_IAC;
    bytes[1] = cmd;

    _send( telnet, bytes, 2 );
}

/*-------------------------------------------------------------------*/
/*           Send non-command data (escapes IAC bytes)               */
/*-------------------------------------------------------------------*/
void telnet_send
(
    telnet_t*     telnet,
    const BYTE*   buffer,
    unsigned int  size
)
{
    unsigned int i, k;

    for (i=0, k=0; i < size; ++i)
    {
        /* Dump prior portion of text, send escaped bytes */
        if (buffer[i] == TELNET_IAC)
        {
            /* Dump prior text if any */
            if (i > k)
                _send( telnet, buffer + k, i - k );

            k = i + 1;

            /* Send escape */
            telnet_iac( telnet, TELNET_IAC );
        }
    }

    /* Send whatever portion of buffer is left */
    if (i > k)
        _send( telnet, buffer + k, i - k );
}

/*-------------------------------------------------------------------*/
/*                 Send subnegotiation header                        */
/*-------------------------------------------------------------------*/
void telnet_begin_sb( telnet_t* telnet, BYTE telopt )
{
    BYTE  sb[3];

    sb[0] = TELNET_IAC;
    sb[1] = TELNET_SB;
    sb[2] = telopt;

    _send( telnet, sb, 3 );
}


/*-------------------------------------------------------------------*/
/*                  Send complete subnegotiation                     */
/*-------------------------------------------------------------------*/
void telnet_subnegotiation
(
    telnet_t*      telnet,
    BYTE           telopt,
    const BYTE*    buffer,
    unsigned int   size
)
{
    BYTE  bytes[5];

    bytes[0] = TELNET_IAC;
    bytes[1] = TELNET_SB;
    bytes[2] = telopt;

    bytes[3] = TELNET_IAC;
    bytes[4] = TELNET_SE;

    _send( telnet, bytes, 3 );

    telnet_send( telnet, buffer, size );

    _send( telnet, bytes + 3, 2 );
}

/*-------------------------------------------------------------------*/
/*  Send formatted data with automatic \r, \n and IAC translation    */
/*-------------------------------------------------------------------*/
int telnet_vprintf( telnet_t* telnet, const char* fmt, va_list va )
{
    char  buffer[1024];
    char* output = buffer;

    int rs, i, k;

    /* Format untranslated data buffer */

    rs = vsnprintf( buffer, sizeof( buffer ), fmt, va );

    /* PROGRAMMING NOTE: we use (rs*2)+1 to ensure that
       we have room to insert extra bytes if necessary */

    if (((rs*2)+1) > (int) sizeof( buffer ))
    {
        va_list  va2 = {0};
        va_copy( va2, va );

        output = (char*) malloc( (rs*2)+1 );

        if (output == 0)
        {
            ERR( telnet, TELNET_ENOMEM, FATAL,
                 "malloc() failed: %s", strerror( errno ));
            return -1;
        }

        rs = vsnprintf( output, rs+1, fmt, va2 );
        va_end( va2 );
    }

    va_end( va );

    /* Translate data buffer (insert special bytes where needed) */

    for (i=0; i < rs; ++i)
    {
        /* Special character? */
        if (0
            || output[i] == (char) TELNET_IAC
            || output[i] == '\r'
            || output[i] == '\n'
        )
        {
            /* Make room for the byte we need to insert */
            if ((k = (rs - i) - 1) > 0)
                memmove( &output[i+2], &output[i+1], k );

            /* IAC -> IAC IAC */
            if (output[i] == (char) TELNET_IAC)
                output[i+1] = (char) TELNET_IAC;

            /* Automatic translation of \r -> CRNUL */
            else if (output[i] == '\r')
                output[i+1] = 0;

            /* Automatic translation of \n -> CRLF */
            else if (output[i] == '\n')
            {
                output[i]   = '\r';
                output[i+1] = '\n';
            }

            rs++;   /* (account for inserted byte) */
            i++;    /* (skip past inserted byte) */
        }
    }

    /* Send entire translated buffer in one fell swoop */
    if (rs > 0)
        _send( telnet, (BYTE*) output, rs );

    /* Free allocated memory, if any */
    if (output != buffer)
        free( output );

    return rs;
}

/*-------------------------------------------------------------------*/
/*                   (see telnet_vprintf)                            */
/*-------------------------------------------------------------------*/
int telnet_printf( telnet_t* telnet, const char* fmt, ... )
{
    int rs;
    va_list   va;
    va_start( va, fmt );

    rs = telnet_vprintf( telnet, fmt, va );

    va_end( va );
    return rs;
}

/*-------------------------------------------------------------------*/
/*           Send formatted data through telnet_send                 */
/*-------------------------------------------------------------------*/
int telnet_raw_vprintf( telnet_t* telnet, const char* fmt, va_list va )
{
    char buffer[1024];
    char *output = buffer;
    int rs;

    /* Format... */

    va_list  va2 = {0};
    va_copy( va2, va );

    rs = vsnprintf( buffer, sizeof( buffer ), fmt, va );

    if (rs >= (int) sizeof( buffer ))
    {
        output = (char*) malloc( rs + 1 );

        if (output == 0)
        {
            ERR( telnet, TELNET_ENOMEM, FATAL,
                 "malloc() failed: %s", strerror( errno ));
            return -1;
        }

        rs = vsnprintf( output, rs + 1, fmt, va2 );
    }

    va_end( va2 );
    va_end( va );

    /* Send... */

    telnet_send( telnet, (BYTE*) output, rs );

    /* Free allocated memory, if any */
    if (output != buffer)
        free( output );

    return rs;
}

/*-------------------------------------------------------------------*/
/*                  (see telnet_raw_vprintf)                         */
/*-------------------------------------------------------------------*/
int telnet_raw_printf( telnet_t* telnet, const char* fmt, ... )
{
    int rs;
    va_list   va;
    va_start( va, fmt );

    rs = telnet_raw_vprintf( telnet, fmt, va );

    va_end( va );
    return rs;
}

/*-------------------------------------------------------------------*/
/*                Begin NEW-ENVIRON subnegotation                    */
/*-------------------------------------------------------------------*/
void telnet_begin_newenviron( telnet_t* telnet, BYTE cmd )
{
    telnet_begin_sb( telnet, TELNET_TELOPT_NEW_ENVIRON );
    telnet_send( telnet, &cmd, 1 );
}

/*-------------------------------------------------------------------*/
/*                  Send a NEW-ENVIRON value                         */
/*-------------------------------------------------------------------*/
void telnet_newenviron_value
(
    telnet_t*      telnet,
    BYTE           type,
    const char*    string
)
{
    telnet_send( telnet, &type, 1 );

    if (string != 0)
        telnet_send( telnet, (BYTE*) string, (unsigned int) strlen( string ));
}

/*-------------------------------------------------------------------*/
/*                Send TERMINAL-TYPE SEND command                    */
/*-------------------------------------------------------------------*/
void telnet_ttype_send( telnet_t* telnet)
{
    static const BYTE ttype_send[] =
    {
        TELNET_IAC,
        TELNET_SB,
        TELNET_TELOPT_TTYPE,
        TELNET_TTYPE_SEND,
        TELNET_IAC,
        TELNET_SE
    };

    _send( telnet, ttype_send, sizeof( ttype_send ));
}

/*-------------------------------------------------------------------*/
/*                Send TERMINAL-TYPE IS command                      */
/*-------------------------------------------------------------------*/
void telnet_ttype_is( telnet_t* telnet, const char* ttype )
{
    static const BYTE ttype_is[] =
    {
        TELNET_IAC,
        TELNET_SB,
        TELNET_TELOPT_TTYPE,
        TELNET_TTYPE_IS
    };

    _send( telnet, (BYTE*) ttype_is, sizeof( ttype_is ));
    _send( telnet, (BYTE*) ttype, (unsigned int) strlen( ttype ));

    telnet_finish_sb( telnet );
}

/*-------------------------------------------------------------------*/
/*                Return name of telnet command                      */
/*-------------------------------------------------------------------*/
const char*  telnet_cmd_name( BYTE cmd )
{
    switch (cmd)
    {
        default:
        {
            static char buffer[32];
            snprintf( buffer, sizeof( buffer ),
                "TELNET_??? (%d)", (int) cmd );
            return buffer;
        }
        case TELNET_EOF   : return "TELNET_EOF";
        case TELNET_SUSP  : return "TELNET_SUSP";
        case TELNET_ABORT : return "TELNET_ABORT";
        case TELNET_EOR   : return "TELNET_EOR";
        case TELNET_SE    : return "TELNET_SE";
        case TELNET_NOP   : return "TELNET_NOP";
        case TELNET_DM    : return "TELNET_DM";
        case TELNET_BREAK : return "TELNET_BREAK";
        case TELNET_IP    : return "TELNET_IP";
        case TELNET_AO    : return "TELNET_AO";
        case TELNET_AYT   : return "TELNET_AYT";
        case TELNET_EC    : return "TELNET_EC";
        case TELNET_EL    : return "TELNET_EL";
        case TELNET_GA    : return "TELNET_GA";
        case TELNET_SB    : return "TELNET_SB";
        case TELNET_WILL  : return "TELNET_WILL";
        case TELNET_WONT  : return "TELNET_WONT";
        case TELNET_DO    : return "TELNET_DO";
        case TELNET_DONT  : return "TELNET_DONT";
        case TELNET_IAC   : return "TELNET_IAC";
    }
}

/*-------------------------------------------------------------------*/
/*                Return name of telnet option                       */
/*-------------------------------------------------------------------*/
const char*  telnet_opt_name( BYTE opt )
{
    switch (opt)
    {
    default:
    {
        static char buffer[32];
        snprintf( buffer, sizeof( buffer ),
            "TELNET_TELOPT_????? (%d)", (int) opt );
        return buffer;
    }
    case TELNET_TELOPT_BINARY          :  return "TELNET_TELOPT_BINARY";
    case TELNET_TELOPT_ECHO            :  return "TELNET_TELOPT_ECHO";
    case TELNET_TELOPT_RCP             :  return "TELNET_TELOPT_RCP";
    case TELNET_TELOPT_SGA             :  return "TELNET_TELOPT_SGA";
    case TELNET_TELOPT_NAMS            :  return "TELNET_TELOPT_NAMS";
    case TELNET_TELOPT_STATUS          :  return "TELNET_TELOPT_STATUS";
    case TELNET_TELOPT_TM              :  return "TELNET_TELOPT_TM";
    case TELNET_TELOPT_RCTE            :  return "TELNET_TELOPT_RCTE";
    case TELNET_TELOPT_NAOL            :  return "TELNET_TELOPT_NAOL";
    case TELNET_TELOPT_NAOP            :  return "TELNET_TELOPT_NAOP";
    case TELNET_TELOPT_NAOCRD          :  return "TELNET_TELOPT_NAOCRD";
    case TELNET_TELOPT_NAOHTS          :  return "TELNET_TELOPT_NAOHTS";
    case TELNET_TELOPT_NAOHTD          :  return "TELNET_TELOPT_NAOHTD";
    case TELNET_TELOPT_NAOFFD          :  return "TELNET_TELOPT_NAOFFD";
    case TELNET_TELOPT_NAOVTS          :  return "TELNET_TELOPT_NAOVTS";
    case TELNET_TELOPT_NAOVTD          :  return "TELNET_TELOPT_NAOVTD";
    case TELNET_TELOPT_NAOLFD          :  return "TELNET_TELOPT_NAOLFD";
    case TELNET_TELOPT_XASCII          :  return "TELNET_TELOPT_XASCII";
    case TELNET_TELOPT_LOGOUT          :  return "TELNET_TELOPT_LOGOUT";
    case TELNET_TELOPT_BM              :  return "TELNET_TELOPT_BM";
    case TELNET_TELOPT_DET             :  return "TELNET_TELOPT_DET";
    case TELNET_TELOPT_SUPDUP          :  return "TELNET_TELOPT_SUPDUP";
    case TELNET_TELOPT_SUPDUPOUTPUT    :  return "TELNET_TELOPT_SUPDUPOUTPUT";
    case TELNET_TELOPT_SNDLOC          :  return "TELNET_TELOPT_SNDLOC";
    case TELNET_TELOPT_TTYPE           :  return "TELNET_TELOPT_TTYPE";
    case TELNET_TELOPT_EOR             :  return "TELNET_TELOPT_EOR";
    case TELNET_TELOPT_TUID            :  return "TELNET_TELOPT_TUID";
    case TELNET_TELOPT_OUTMRK          :  return "TELNET_TELOPT_OUTMRK";
    case TELNET_TELOPT_TTYLOC          :  return "TELNET_TELOPT_TTYLOC";
    case TELNET_TELOPT_3270REGIME      :  return "TELNET_TELOPT_3270REGIME";
    case TELNET_TELOPT_X3PAD           :  return "TELNET_TELOPT_X3PAD";
    case TELNET_TELOPT_NAWS            :  return "TELNET_TELOPT_NAWS";
    case TELNET_TELOPT_TSPEED          :  return "TELNET_TELOPT_TSPEED";
    case TELNET_TELOPT_LFLOW           :  return "TELNET_TELOPT_LFLOW";
    case TELNET_TELOPT_LINEMODE        :  return "TELNET_TELOPT_LINEMODE";
    case TELNET_TELOPT_XDISPLOC        :  return "TELNET_TELOPT_XDISPLOC";
    case TELNET_TELOPT_ENVIRON         :  return "TELNET_TELOPT_ENVIRON";
    case TELNET_TELOPT_AUTHENTICATION  :  return "TELNET_TELOPT_AUTHENTICATION";
    case TELNET_TELOPT_ENCRYPT         :  return "TELNET_TELOPT_ENCRYPT";
    case TELNET_TELOPT_NEW_ENVIRON     :  return "TELNET_TELOPT_NEW_ENVIRON";
    case TELNET_TELOPT_EXOPL           :  return "TELNET_TELOPT_EXOPL";
    }
}

/*-------------------------------------------------------------------*/
/*                Return name of telnet event                        */
/*-------------------------------------------------------------------*/
const char*  telnet_evt_name( telnet_event_code_t evt )
{
    switch (evt)
    {
    default:
    {
        static char buffer[32];
        snprintf( buffer, sizeof( buffer ),
            "TELNET_EV_????? (%d)", (int) evt );
        return buffer;
    }
    case TELNET_EV_DATA            :  return "TELNET_EV_DATA";
    case TELNET_EV_SEND            :  return "TELNET_EV_SEND";
    case TELNET_EV_IAC             :  return "TELNET_EV_IAC";
    case TELNET_EV_WILL            :  return "TELNET_EV_WILL";
    case TELNET_EV_WONT            :  return "TELNET_EV_WONT";
    case TELNET_EV_DO              :  return "TELNET_EV_DO";
    case TELNET_EV_DONT            :  return "TELNET_EV_DONT";
    case TELNET_EV_SUBNEGOTIATION  :  return "TELNET_EV_SUBNEGOTIATION";
    case TELNET_EV_TTYPE           :  return "TELNET_EV_TTYPE";
    case TELNET_EV_ENVIRON         :  return "TELNET_EV_ENVIRON";
    case TELNET_EV_WARNING         :  return "TELNET_EV_WARNING";
    case TELNET_EV_ERROR           :  return "TELNET_EV_ERROR";
    }
}

/*-------------------------------------------------------------------*/
/*                Return name of telnet error                        */
/*-------------------------------------------------------------------*/
const char*  telnet_err_name( telnet_error_t err )
{
    switch (err)
    {
    default:
    {
        static char buffer[32];
        snprintf( buffer, sizeof( buffer ),
            "TELNET_E????? (%d)", (int) err );
        return buffer;
    }
    case TELNET_EOK           :  return "TELNET_EOK";
    case TELNET_ENOMEM        :  return "TELNET_ENOMEM";
    case TELNET_EOVERFLOW     :  return "TELNET_EOVERFLOW";
    case TELNET_EPROTOCOL     :  return "TELNET_EPROTOCOL";
    case TELNET_ENEGOTIATION  :  return "TELNET_ENEGOTIATION";
    }
}

/*-------------------------------------------------------------------*/
/*                     _select_socket                                */
/*-------------------------------------------------------------------*/
static int _select_socket( int sock, double secs )
{
    /* Returns 0 == success, -1 == error w/errno */

    int             rc;
    fd_set          rdset;
    struct timeval  tv;

    FD_ZERO( &rdset );
    FD_SET( sock, &rdset );

    tv.tv_sec  = (long)   secs;
    tv.tv_usec = (long) ((secs - tv.tv_sec) * 1000000);

    rc = select( sock + 1, &rdset, 0, 0, &tv );

    return rc;
}

/*-------------------------------------------------------------------*/
/*                     _shut_socket                                  */
/*-------------------------------------------------------------------*/
static int _shut_socket( int sock, double max_secs )
{
    /* Returns 0 == success, -1 == error w/errno */

    int rc, i;
    char buf[ 128 ];

    /* You should always say "Goodbye" before hanging up */
    if ((rc = shutdown( sock, 1 )) < 0) /* SD_SEND, SHUT_WR */
        return rc;

#define SELECT_SECS     0.100
#define MAX_LOOPS       (max_secs / SELECT_SECS)

    /* Now wait for them to also say "Goodbye"... */
    for (i=0; i < MAX_LOOPS; i++)
    {
        if (0
            || (rc = _select_socket( sock, SELECT_SECS )) <= 0
            || (rc = recv( sock, buf, sizeof( buf ), 0 )) <= 0
        )
        {
            /* It should now be safe to hang up */
            return rc;
        }
    }

    /* Timeout waiting for them to acknowledge our goodbye */

#if defined(_WIN32)
    errno = WSAETIMEDOUT;
#else
    errno = ETIMEDOUT;
#endif

    return -1;
}

/*-------------------------------------------------------------------*/
/*                    telnet_closesocket                             */
/*-------------------------------------------------------------------*/
int telnet_closesocket( int sock )  /* graceful close */
{
    /* Returns 0 == success, -1 == error w/errno */

    int shut_rc  = _shut_socket( sock, LIBTN_GRACEFUL_SOCKCLOSESECS );

#ifdef _MSC_VER
    int close_rc = closesocket( sock ); /* Windows */
#else
    int close_rc = close( sock );       /* Linux */
#endif

    return close_rc != 0 ? close_rc : shut_rc;
}
