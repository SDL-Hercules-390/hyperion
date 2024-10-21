/* HIM.C        (c) Copyright Thomas J. Valerio, 2024                */
/*              (c) Copyright Michael T. Alexander, 2024             */
/*              ESA/390 Host Interface Machine Device Handler        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

// $Id$

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for emulated       */
/* System/390 Host Interface Machine devices.                        */
/*                                                                   */
/* a "Host Interface Machine" or HIM was a homegrown subchannel      */
/* addressable Internet Protocol device that allowed the Michigan    */
/* Terminal System, a.k.a. MTS to communicate with the outside world */
/* over the Internet.                                                */
/*-------------------------------------------------------------------*/


#include "hstdinc.h"
#include "hercules.h"
#include "devtype.h"

// If set writes debug output to FD 5 otherwise uses TRACE
//#define WRITEDBG

// If ENABLE_TRACING_STMTS is undefined it will be set
// based on the debug flag, tracing only in a debug build
//#define ENABLE_TRACING_STMTS 1
#include "dbgtrace.h"

#if defined( _MSVC_ )        // MS Windows
  #include "netsupp.h"       // (networking structs and funcs)
  #define Sin_Addr sin_addr.s_addr
  #define Ip_Src ip_src
  #define Ip_Dst ip_dst
#else                        // unix/linux/Mac_OS
  #define __FAVOR_BSD
  #include <netinet/ip.h>
  #include <netinet/tcp.h>
  #include <netinet/udp.h>
  #include <netinet/in.h>
  #include <poll.h>
  #define Sin_Addr sin_addr
  #define Ip_Src ip_src.s_addr
  #define Ip_Dst ip_dst.s_addr
#endif


/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define QLEN     5


/*-------------------------------------------------------------------*/
/* This header is at the front of every subchannel read and write    */
/* operation for non-3270 devices.  It is used to communicate        */
/* between the HIM Device Support Processor code in MTS and this     */
/* HIM device emulation.  The bits are reversed from where they      */
/* appear in memory because this is a little-endian architecture     */
/*-------------------------------------------------------------------*/
struct buff_hdr {
    u_char bh_flags;
    #define BH_TN3270     0x08         /* Switch to TN3270 mode      */
    #define BH_INIT       0x10         /* Data is configuration info */
    #define BH_FINISHED   0x20         /* Interface disconnect       */
    #define BH_RNR        0x40         /* Read-Not-Ready             */
    #define BH_URGENT     0x80         /* Urgent data to be read     */
    u_char buffer_number;              /* Sequential buffer number   */
    u_short buffer_length;             /* buffer length              */
};


/*-------------------------------------------------------------------*/
/* This is the full packet header for all of the subchannel read     */
/* and write operations for non-3270 devices.  It includes the HIM   */
/* DSP buffer header defined above, as well as the IP packet header  */
/* and the TCP and UDP packet headers.                               */
/*-------------------------------------------------------------------*/
#if defined( _MSVC_ )        // MS Windows
struct packet_hdr {
    struct buff_hdr him_hdr;
    struct ip_hdr ip_header;
    union {
        struct tcp_hdr tcp_header;
        struct udp_hdr udp_header;
    } sh;
    u_char tcp_optcode;
    u_char tcp_optlen;
    u_short tcp_optval;
};
#else                        // unix/linux/Mac_OS
struct packet_hdr {
    struct buff_hdr him_hdr;
    struct ip ip_header;
    union {
        struct tcphdr tcp_header;
        struct udphdr udp_header;
    } sh;
    u_char tcp_optcode;
    u_char tcp_optlen;
    u_short tcp_optval;
};
#endif


/*-------------------------------------------------------------------*/
/* This is the format of the *reply* to the configuration command    */
/* that MTS sends out when it wants to start using a particular      */
/* subchannel. The configuration command itself is an EBCDIC string. */
/*-------------------------------------------------------------------*/
struct config_reply {
    struct buff_hdr him_hdr;
    unsigned char config_ok[2];        /* EBCDIC "Ok"                */
    u_char family;                     /* Protocol family            */
    u_char protocol;                   /* Actual Protocol            */
    u_short local_port;                /* Local port number          */
    u_char local_ip[4];                /* Local IP address           */
    char unused[2];
    u_short remote_port;
    u_char remote_ip[4];
};


/*-------------------------------------------------------------------*/
/* The I/O control block                                             */
/*-------------------------------------------------------------------*/
typedef enum {SHUTDOWN, INITIALIZED, CONNECTED, CLOSING} t_state;
struct io_cb {
    int sock;
    u_char protocol;
    t_state state;
    unsigned int passive    : 1;     /* Passive port listener        */
    unsigned int server     : 1;     /* Accepting calls on any port  */
    unsigned int rnr        : 1;     /* Read Not Ready flag          */
    unsigned int watch_sock : 1;     /* Socket watcher thread active */
    unsigned int tn3270     : 1;     /* In use by TN3270             */
    unsigned int unused_0   : 1;
    unsigned int unused_1   : 1;
    unsigned int unused_2   : 1;
    struct sockaddr_in sin;
    in_addr_t bind_addr;             /* The address to bind to, may be INADDR_ANY */
    in_addr_t our_addr;              /* The address we actually bound to */
    /* mts_header is the header to be returned to MTS for a packet read
       from the net.  The source address is the remote address and the
       destination address is MTS's address. */
    struct packet_hdr mts_header;
    enum {EMPTY, CONFIG, MSS, ACK, FIN, FINISHED} read_q[16];
    int max_q, attn_rc[4];
};


// Static variables used by the code that waits for
// incoming TCP server connections
static  LOCK  TCPServerLock;
static  int   TCPServerLockInitialized = FALSE;

// Number of HIM devices waiting for a TCP server connection
static  int   TCPServerCount = 0;
static  int   TCPServerThreadRunning = 0;

// Static variables used by the code that waits for
// incoming UDP server connections
static  LOCK  UDPServerLock;
static  int   UDPServerLockInitialized = FALSE;

// Number of HIM devices waiting for a UDP server connection
static  int   UDPServerCount = 0;
static  int   UDPServerThreadRunning = 0;

static void config_subchan( DEVBLK *dev, struct io_cb *cb_ptr, BYTE *config_data );
static int parse_config_data( struct io_cb *cb_ptr, char *config_string, int cs_len );
static int get_socket( DEVBLK *dev, int protocol, in_addr_t bind_addr, int port, struct sockaddr_in *sin, int qlen );
static int return_mss( struct io_cb *cb_ptr, struct packet_hdr *mss );
static int start_sock_thread( DEVBLK* dev );
static void* skt_thread( void* dev );
static void debug_pf( const char* __fmt, ... );
static void dumpdata( char *label, BYTE *data, int len );
static void reset_io_cb( struct io_cb *cb_ptr );

typedef struct sserver_listen_thread_args
{
    DEVBLK*        dev;
    struct io_cb*  cb_ptr;
}
SSLTA;

static void* TCP_sserver_listen_thread( void* arg );
static int add_TCP_server_listener( DEVBLK *dev, struct io_cb *cb_ptr );
static int remove_TCP_server_listener( struct io_cb *cb_ptr );
static void* UDP_sserver_listen_thread( void* arg );
static int add_UDP_server_listener( DEVBLK *dev, struct io_cb *cb_ptr );
static int remove_UDP_server_listener( struct io_cb *cb_ptr );
static void set_state( struct io_cb *cb_ptr, t_state state );


/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
static int him_init_handler( DEVBLK *dev, int argc, char *argv[] )
{
    struct io_cb *cb_ptr;

    if ( argc > 1 )
        return -1;

    // Initialize locking for the server data, if necessary.
    if ( !TCPServerLockInitialized )
    {
        TCPServerLockInitialized = TRUE;
        initialize_lock( &TCPServerLock );
    }
    if ( !UDPServerLockInitialized )
    {
        UDPServerLockInitialized = TRUE;
        initialize_lock( &UDPServerLock );
    }

    /* If this is a reinit and the previous incarnation
       is a server waiting for a call, teminate the wait. */
    if ( dev->reinit )
    {
        cb_ptr = (struct io_cb *)dev->dev_data;
        if ( cb_ptr->state == INITIALIZED &&
             cb_ptr->server && cb_ptr->sock <= 0 )
        {
            if ( cb_ptr->protocol == IPPROTO_TCP )
                remove_TCP_server_listener( cb_ptr );
            else
                remove_UDP_server_listener( cb_ptr );
        }
    }
    /* Should set dev->devtype to something, but what?
       It must be a hex number equal to an IBM model number. */
    dev->devtype = 0;

    /* Set length of buffer */
    dev->bufsize = 2048;

    /* Set number of sense bytes */
    dev->numsense = 1;

    /* Initialize the device identifier bytes */
    dev->devid[0] = 0xFF;
    dev->devid[1] = 0x32; /* Control unit type is 3274-1d */
    dev->devid[2] = 0x74;
    dev->devid[3] = 0x1d;
    dev->devid[4] = dev->devtype >> 8;
    dev->devid[5] = dev->devtype & 0xFF;
    dev->devid[6] = 0x01;
    dev->numdevid = 7;

    dev->himdev   = 1;

    if ( dev->reinit )
        cb_ptr = (struct io_cb *)dev->dev_data;
    else
        dev->dev_data = cb_ptr = malloc( sizeof( struct io_cb ) );

    memset( (char *) dev->dev_data, '\0', sizeof( struct io_cb ) );

    /* The first optional parameter is the IP address to bind to */
    if ( argc >= 1 )
    {
        struct in_addr addr;
        if ( inet_aton(argv[0], &addr) < 1 )
        {
            /* "Invalid %s parameter: %s" */
            WRMSG( HHC02781, "E", "IP address", argv[0]);
            return -1;
        }
        cb_ptr->bind_addr = addr.s_addr;
    }
    else
    {
        /* Bind to any address, will get set to actual address
           after the bind succeeds */
        cb_ptr->bind_addr = INADDR_ANY;
    }

    /* Not bound yet: */
    cb_ptr->our_addr = INADDR_ANY;

    debug_pf( "Device %s at %04X initialized, version = %s %s\n",
        dev->typname, dev->devnum, __TIME__, __DATE__ );

    /* Activate I/O tracing */
    // dev->ccwtrace = 1;

    return 0;

} /* end function him_init_handler */


/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void him_query_device( DEVBLK *dev, char **devclass,
                int buflen, char *buffer )
{
    char  filename[ PATH_MAX + 1 + 3 ];
    struct io_cb *cb_ptr = (struct io_cb *)dev->dev_data;
    struct in_addr addr;

    BEGIN_DEVICE_CLASS_QUERY( "HIM", dev, devclass, buflen, buffer );

    addr.s_addr = cb_ptr->our_addr;

    snprintf( buffer, buflen-1, "%s%s IO[%" I64_FMT "u]",
                inet_ntoa(addr),
                dev->stopdev    ? " (stopped)" : "",
                dev->excps );

} /* end function him_query_device */


/*-------------------------------------------------------------------*/
/* Halt the device                                                   */
/*-------------------------------------------------------------------*/
static void him_halt_device( DEVBLK *dev )
{
    struct timeval tv;
    char   ts_buf[64];
    time_t secs;

    gettimeofday( &tv, NULL ); secs = tv.tv_sec;
    strftime( ts_buf, sizeof( ts_buf ), "%H:%M:%S", localtime( &secs ) );
    debug_pf( " %s.%06d -- devnum %04X HALT\n", ts_buf, tv.tv_usec, dev->devnum );

} /* end function him_halt_device */


/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/*-------------------------------------------------------------------*/
static int him_close_device( DEVBLK *dev )
{
    struct io_cb *  cb_ptr;                 /* I/O Control Block pointer */

    dev->stopdev = FALSE;

    /* If it is a server waiting for a call, terminate the wait */
    cb_ptr = (struct io_cb *) dev->dev_data;
    reset_io_cb( cb_ptr );

    /* Free the I/O Control Block */
    free( dev->dev_data );

    debug_pf( "Device termination successful\n" );

    return 0;
} /* end function him_close_device */


/*-------------------------------------------------------------------*/
/* Do channel program end processing                                 */
/*-------------------------------------------------------------------*/
static void him_cpe_device( DEVBLK *dev )
{
    UNREFERENCED( dev );

} /* end function him_cpe_device */


/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
static void him_execute_ccw( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual )
{
    int    i;                           /* Loop counter              */
    int    num;                         /* Number of bytes to move   */
    int    readlen, writelen, temp_sock;
    struct io_cb *cb_ptr;               /* I/O Control Block pointer */
    struct packet_hdr *buff_ptr;
    struct pollfd read_chk;
    unsigned int sinlen = sizeof( struct sockaddr_in );

    UNREFERENCED( flags );
    UNREFERENCED( chained );
    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq );

    /* if ( code == 1 || code == 2 ) */
    {
        struct timeval tv;
        char   ts_buf[64];
        time_t secs;

        gettimeofday( &tv, NULL ); secs = tv.tv_sec;
        strftime( ts_buf, sizeof( ts_buf ), "%H:%M:%S", localtime( &secs ) );
        debug_pf( " %s.%06d -- devnum %04X opcode %02X\n", ts_buf, tv.tv_usec, dev->devnum, code );
    }

    /* Copy I/O Control Block and Channel I/O buffer pointers */
    cb_ptr = (struct io_cb *) dev->dev_data;
    buff_ptr = (struct packet_hdr *) iobuf;

    /* Process depending on CCW opcode */
    switch( code ) {

    case 0x01:      /* Write_Ccw */
    /*---------------------------------------------------------------*/
    /* WRITE - process data from channel                             */
    /*---------------------------------------------------------------*/

        *residual = 0;
        *unitstat = CSW_CE | CSW_DE;

        debug_pf( "data from MTS       DevNum = %04X\n", dev->devnum );
        dumpdata( "", iobuf, (count < 96 ? count : 96) );
        if ( count > 44 && cb_ptr->protocol == IPPROTO_TCP )
            debug_pf( "%.*s\n", count - 44, &((char *) iobuf)[44] );

        if ( buff_ptr->him_hdr.bh_flags & BH_FINISHED )
        {
            for ( i = 0; cb_ptr->read_q[i] != EMPTY; i++ ) ;
            cb_ptr->read_q[i] = FINISHED;

        }
        else if ( cb_ptr->state == CONNECTED && buff_ptr->him_hdr.bh_flags & BH_RNR )
        {
            debug_pf( "-----  RNR Flag = ON received.\n" );

            cb_ptr->watch_sock = 0;
            cb_ptr->rnr = 1;
            *unitstat |= CSW_UX;

        }
        else if ( cb_ptr->rnr && !( buff_ptr->him_hdr.bh_flags & BH_RNR ) )
        {
            debug_pf( "-----  RNR Flag = OFF received.\n" );

            start_sock_thread( dev );
            cb_ptr->rnr = 0;

        }
        else if ( buff_ptr->him_hdr.bh_flags & BH_INIT )
        {
            config_subchan( dev, cb_ptr, iobuf );

            /* Save the config reply to dev->buf so it will be there for the read ccw */
            readlen = ntohs( buff_ptr->him_hdr.buffer_length ) + sizeof( struct buff_hdr );
            memcpy( dev->buf, buff_ptr, readlen );

            for ( i = 0; cb_ptr->read_q[i] != EMPTY; i++ ) ;
            cb_ptr->read_q[i] = CONFIG;

            *unitstat |= CSW_ATTN;

        }
        else if ( cb_ptr->protocol == IPPROTO_UDP )
        {
            if ( ntohs( buff_ptr->him_hdr.buffer_length ) > 4 )
            {
                cb_ptr->sin.sin_port = buff_ptr->sh.udp_header.uh_dport;
                cb_ptr->sin.Sin_Addr = buff_ptr->ip_header.ip_dst;
                writelen = ntohs( buff_ptr->him_hdr.buffer_length ) - 28;

                if ( sendto( cb_ptr->sock, &((char *) buff_ptr)[32], writelen, 0,
                    (struct sockaddr *)&cb_ptr->sin, sizeof( struct sockaddr_in ) ) < 0 )
                    // ""%1d:%04X HIM: Error in function %s: %s"
                    WRMSG( HHC01150, "E", LCSS_DEVNUM, "sendto()", strerror( HSO_errno ));
            }
        }
        else                                 /* must be a TCP packet */
        {
            /* If this is an unconnected TCP subchannel then the     */
            /* first packet is the signal that we should get         */
            /* connected.  The first packet also contains the        */
            /* destination address that we need to connect.          */

            if ( cb_ptr->state == INITIALIZED )
            {
                cb_ptr->mts_header.ip_header.ip_src =
                    cb_ptr->sin.Sin_Addr = buff_ptr->ip_header.ip_dst;
                cb_ptr->mts_header.sh.tcp_header.th_sport =
                    cb_ptr->sin.sin_port = buff_ptr->sh.tcp_header.th_dport;
/* don't set the dest addr in mts_header, the TCP DSP doesn't
   set the source address in the buffer correctly and it's already been set. */
#if 0
                cb_ptr->mts_header.ip_header.ip_dst = buff_ptr->ip_header.ip_src;
                cb_ptr->mts_header.sh.tcp_header.th_dport =
                    buff_ptr->sh.tcp_header.th_sport;
#endif

                if ( connect( cb_ptr->sock,
                    (struct sockaddr *)&cb_ptr->sin, sizeof( struct sockaddr_in ) ) < 0 )
                    /* TODO: If connect fails signal error to MTS */
                    // ""%1d:%04X HIM: Error in function %s: %s"
                    WRMSG( HHC01150, "E", LCSS_DEVNUM, "connect()", strerror( HSO_errno ));

                set_state( cb_ptr, CONNECTED );

                /* Queue an MSS acknowledgement */
                for ( i = 0; cb_ptr->read_q[i] != EMPTY; i++ ) ;
                cb_ptr->read_q[i] = MSS;

                *unitstat |= CSW_ATTN;

            }
            else if ( ntohs( buff_ptr->him_hdr.buffer_length ) > 4 )
            {
                int offset, window, ack_seq;

                offset = ( ( buff_ptr->ip_header.ip_hl +
                    buff_ptr->sh.tcp_header.th_off ) * 4 ) + 4;

                writelen = ntohs( buff_ptr->him_hdr.buffer_length ) - offset + 4;
                cb_ptr->mts_header.sh.tcp_header.th_ack =
                    htonl( ntohl( cb_ptr->mts_header.sh.tcp_header.th_ack ) + writelen );

                if ( writelen > 0 )
                {
                    if ( cb_ptr->state == CONNECTED )
                    {
                        i = write_socket( cb_ptr->sock, &((char *) buff_ptr)[offset], writelen );

                        window = ntohs( cb_ptr->mts_header.sh.tcp_header.th_win );
                        ack_seq = ntohl( cb_ptr->mts_header.sh.tcp_header.th_ack );

                        if ( (window - (ack_seq % window)) < (writelen + 4096) )
                        {
                            for ( i = 0; cb_ptr->read_q[i] != EMPTY; i++ ) ;
                            cb_ptr->read_q[i] = ACK;
                        }
                    }
                }

                /* else */ if ( buff_ptr->sh.tcp_header.th_flags & TH_FIN )
                {
                    if ( cb_ptr->state == CONNECTED )
                    {
                        for ( i = 0; cb_ptr->read_q[i] != EMPTY; i++ ) ;
                        cb_ptr->read_q[i] = FIN;
                        set_state( cb_ptr, CLOSING );
                    }

                    for ( i = 0; cb_ptr->read_q[i] != EMPTY; i++ ) ;
                    cb_ptr->read_q[i] = FINISHED;
                }
            }
        }

        break;


    case 0x02:      /* Read_Buffer_Ccw */
    case 0x06:      /* Read_Modified_Ccw (not used) */
    /*---------------------------------------------------------------*/
    /* READ - Send data to channel                                   */
    /*---------------------------------------------------------------*/

        readlen = 0;
        *residual = count;
        *unitstat = CSW_CE | CSW_DE;

        read_chk.fd = cb_ptr->sock;
        read_chk.events = POLLIN;

        if ( cb_ptr->read_q[0] != EMPTY )
        {       /* Data that needs to be sent to MTS has been queued */
            /* Record the maximum size of the read queue */
            for ( i = 0; cb_ptr->read_q[i] != EMPTY; i++ ) ;
            cb_ptr->max_q = i > cb_ptr->max_q ? i : cb_ptr->max_q;

            switch( cb_ptr->read_q[0] ) {
            case CONFIG:    /* The config command reply was left in dev->buf */
                readlen = ntohs( ((struct buff_hdr *) dev->buf)->buffer_length )
                    + sizeof( struct buff_hdr );

                memcpy( iobuf, dev->buf, readlen );
                break;

            case MSS:
                readlen = return_mss( cb_ptr, buff_ptr );
                break;

            case ACK:
                cb_ptr->mts_header.him_hdr.buffer_number++;
                cb_ptr->mts_header.ip_header.ip_id =
                    htons( ntohs( cb_ptr->mts_header.ip_header.ip_id ) + 1 );
                memcpy( buff_ptr, &cb_ptr->mts_header, 44 );
                readlen = 44;
                break;

            case FIN:
                cb_ptr->mts_header.him_hdr.buffer_number++;
                cb_ptr->mts_header.ip_header.ip_id =
                    htons( ntohs( cb_ptr->mts_header.ip_header.ip_id ) + 1 );
                memcpy( buff_ptr, &cb_ptr->mts_header, 44 );
                readlen = 44;

                buff_ptr->sh.tcp_header.th_flags |= TH_FIN;

                if ( cb_ptr->state == CONNECTED )
                    set_state( cb_ptr, CLOSING );

                break;

            case FINISHED:
                debug_pf( "At subchannel %04X CLOSE:\n  maximum read_q size = %d\n",
                    dev->devnum, cb_ptr->max_q );
                debug_pf( "  device attention rc count = %d, %d, %d, %d\n", cb_ptr->attn_rc[0],
                    cb_ptr->attn_rc[1], cb_ptr->attn_rc[2], cb_ptr->attn_rc[3] );

                cb_ptr->mts_header.him_hdr.buffer_number++;
                cb_ptr->mts_header.him_hdr.bh_flags = BH_FINISHED;
                cb_ptr->mts_header.him_hdr.buffer_length = 0;
                memcpy( buff_ptr, &cb_ptr->mts_header, 4 );
                readlen = 4;

                reset_io_cb(cb_ptr);

            default: ;

            } /* end switch( cb_ptr->read_q[0] ) */

            /* Remove first entry from queue, a NOP on a closed connection */
            for ( i = 0; i < 15; i++ )
                cb_ptr->read_q[i] = cb_ptr->read_q[i+1];

            *residual -= readlen;

        }
        else if ( cb_ptr->state == CLOSING )
        {
            *unitstat |= CSW_UX;
            debug_pf( " ------ READ ccw, STATE = CLOSING\n" );

        }
        else if ( !poll( &read_chk, 1, 10 ) )  /* i.e. no data available from the socket */
        {
            *unitstat |= CSW_UX;

        }
        else if ( cb_ptr->protocol == IPPROTO_UDP )
        {
            cb_ptr->mts_header.him_hdr.buffer_number++;
            cb_ptr->mts_header.ip_header.ip_id =
                htons( ntohs( cb_ptr->mts_header.ip_header.ip_id ) + 1 );
            memcpy( buff_ptr, &cb_ptr->mts_header, 32 );

            readlen = recvfrom( cb_ptr->sock, &((char *) buff_ptr)[32], 1460, 0,
                (struct sockaddr *)&cb_ptr->sin, &sinlen );

            buff_ptr->him_hdr.buffer_length =
                buff_ptr->ip_header.ip_len = htons( readlen + 28 );

            buff_ptr->ip_header.ip_src = cb_ptr->sin.Sin_Addr;
            buff_ptr->sh.udp_header.uh_sport = cb_ptr->sin.sin_port;

            *residual -= readlen + 32;

        }
        else if ( cb_ptr->passive && !cb_ptr->server &&
                  cb_ptr->state == INITIALIZED )
        {
            temp_sock = cb_ptr->sock;
            cb_ptr->sock = accept( temp_sock, (struct sockaddr *)&cb_ptr->sin, &sinlen );

            (void) close_socket( temp_sock );
            set_state( cb_ptr, CONNECTED );

            getpeername( cb_ptr->sock, (struct sockaddr *)&cb_ptr->sin, &sinlen );
            cb_ptr->mts_header.ip_header.ip_src = cb_ptr->sin.Sin_Addr;
            cb_ptr->mts_header.sh.tcp_header.th_sport = cb_ptr->sin.sin_port;

            *residual -= return_mss( cb_ptr, buff_ptr );

            debug_pf( "just accepted call on socket %d for socket %d\n", temp_sock, cb_ptr->sock );

        }
        else if ( cb_ptr->state == CONNECTED ) /* A UDP connection will never be in this state */
        {
            cb_ptr->mts_header.him_hdr.buffer_number++;
            cb_ptr->mts_header.ip_header.ip_id =
                htons( ntohs( cb_ptr->mts_header.ip_header.ip_id ) + 1 );
            memcpy( buff_ptr, &cb_ptr->mts_header, 44 );

            buff_ptr->sh.tcp_header.th_flags |= TH_PUSH;
            readlen = recv( cb_ptr->sock, &((char *) buff_ptr)[44], 1460, 0 );

            if ( readlen > 0 )
            {
                cb_ptr->mts_header.sh.tcp_header.th_seq =
                    htonl( ntohl( cb_ptr->mts_header.sh.tcp_header.th_seq ) + readlen);
                buff_ptr->him_hdr.buffer_length =
                    buff_ptr->ip_header.ip_len = htons( readlen + 40 );

                *residual -= ( readlen + 44 );

            }
            else if ( readlen == 0 )
            {
                buff_ptr->sh.tcp_header.th_flags |= TH_FIN;
                set_state( cb_ptr, CLOSING );

                *residual -= 44;

            }
            else
            {
                debug_pf( " --- cb_ptr->state == CONNECTED, read rc = %i, errno = %d\n",
                   readlen, errno );
                dumpdata( "I/O cb", (BYTE *)cb_ptr, sizeof( struct io_cb ) );

                buff_ptr->sh.tcp_header.th_flags |= TH_RST;
                *residual -= 44;
                *unitstat |= CSW_UC;
            }
        }
        else
        {
            *unitstat |= CSW_UX;

            debug_pf( "READ ccw, STATE = %d\n", cb_ptr->state );

        }

        /* don't start the socket thread if there is no socket.
           This happens when starting a server connection. */
        if ( cb_ptr->state != SHUTDOWN && !cb_ptr->watch_sock &&
             cb_ptr->sock > 0 )
            start_sock_thread( dev );

        /* debug_pf(" chained = %02X, prevcode = %02X, ccwseq = %i\n",
            chained, prevcode, ccwseq); */

        if ( *residual != count )      /* i.e. we are returning data */
        {
            debug_pf( "data  to  MTS       DevNum = %04X\n", dev->devnum );
            dumpdata( "", iobuf, 44 );
            if ( readlen > 0 && cb_ptr->protocol == IPPROTO_TCP )
                debug_pf( "%.*s\n", readlen, &((char *) iobuf)[44] );
        }

        break;


    case 0x0B:      /* Select_Read_Modified_CCw (not used) */
    case 0x1B:      /* Select_Read_Buffer_Ccw */
    case 0x4B:      /* Select_Write_Ccw (not used) */
    /*---------------------------------------------------------------*/
    /* Various select commands whch we ignore                        */
    /*---------------------------------------------------------------*/

        *residual = 0;
        *unitstat = CSW_CE | CSW_DE;
        break;


    case 0x03:
    /*---------------------------------------------------------------*/
    /* CONTROL NO-OPERATION                                          */
    /*---------------------------------------------------------------*/

        *residual = 0;
        *unitstat = CSW_CE | CSW_DE;
        break;


    case 0x2B:
    /*---------------------------------------------------------------*/
    /* CONTROL WAIT, FOR REALLY LONG TIME                            */
    /*---------------------------------------------------------------*/

        /* Wait for a really long time, as in several minutes */
        /* Used for testing HALT device entry point           */

        for ( i = 1; i < 120; i++ )
        {
            sleep ( 1 );
            if ( cb_ptr->unused_0 )
                break;

        }

        cb_ptr->unused_0 = 0;
        debug_pf( "------- Exited CONTROL-WAIT after %d seconds\n", i );

        *residual = 0;
        *unitstat = CSW_CE | CSW_DE;
        break;


    case 0x04:
    /*---------------------------------------------------------------*/
    /* SENSE                                                         */
    /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = ( count < dev->numsense ) ? count : dev->numsense;
        *residual = count - num;
        if ( count < dev->numsense ) *more = 1;

        /* Copy device sense bytes to channel I/O buffer */
        memcpy( iobuf, dev->sense, num );

        /* Clear the device sense bytes */
        memset( dev->sense, 0, sizeof( dev->sense ) );

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;


    case 0xE4:
    /*---------------------------------------------------------------*/
    /* SENSE ID                                                      */
    /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = ( count < dev->numdevid ) ? count : dev->numdevid;
        *residual = count - num;
        if ( count < dev->numdevid ) *more = 1;

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


    } /* end switch( code ) */

 /* debug_pf( "------- devnum: %04X, Returning status = %02X, after call = %02X\n",
        dev->devnum, *unitstat, code ); */

} /* end function him_execute_ccw */


static DEVHND him_device_hndinfo = {
    &him_init_handler,                 /* Device Initialisation      */
    &him_execute_ccw,                  /* Device CCW execute         */
    &him_close_device,                 /* Device Close               */
    &him_query_device,                 /* Device Query               */
    NULL,                              /* Device Extended Query      */
    NULL,                              /* Device Start channel pgm   */
    &him_cpe_device,                   /* Device End channel pgm     */
    NULL,                              /* Device Resume channel pgm  */
    NULL,                              /* Device Suspend channel pgm */
    &him_halt_device,                  /* Device Halt channel pgm    */
    NULL,                              /* Device Read                */
    NULL,                              /* Device Write               */
    NULL,                              /* Device Query used          */
    NULL,                              /* Device Reserve             */
    NULL,                              /* Device Release             */
    NULL,                              /* Device Attention           */
    NULL,                              /* Immediate CCW Codes        */
    NULL,                              /* Signal Adapter Input       */
    NULL,                              /* Signal Adapter Output      */
    NULL,                              /* Signal Adapter Sync        */
    NULL,                              /* Signal Adapter Output Mult */
    NULL,                              /* QDIO subsys desc           */
    NULL,                              /* QDIO set subchan ind       */
    NULL,                              /* Hercules suspend           */
    NULL                               /* Hercules resume            */
};


/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */
#if !defined(HDL_BUILD_SHARED) && defined(HDL_USE_LIBTOOL)
#define hdl_ddev hdttcph_LTX_hdl_ddev
#define hdl_depc hdttcph_LTX_hdl_depc
#define hdl_reso hdttcph_LTX_hdl_reso
#define hdl_init hdttcph_LTX_hdl_init
#define hdl_fini hdttcph_LTX_hdl_fini
#endif


HDL_DEPENDENCY_SECTION;
{
    HDL_DEPENDENCY(HERCULES);
    HDL_DEPENDENCY(DEVBLK);
    HDL_DEPENDENCY(SYSBLK);
}
END_DEPENDENCY_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE(AUSC, him_device_hndinfo );
    HDL_DEVICE(UDPH, him_device_hndinfo );
    HDL_DEVICE(TLNT, him_device_hndinfo );
    HDL_DEVICE(TCPH, him_device_hndinfo );
}
END_DEVICE_SECTION


/*-------------------------------------------------------------------*/
/* When MTS wants to start using a particular subchannel it sends    */
/* out an EBCDIC character string that indicates how the subchannel  */
/* will be used.  This configuration command indicates the type of   */
/* connection, the protocol, whether it will be an active or passive */
/* connection, address information for the local and foreign         */
/* sockets, and whether this is a telnet server subchannel or not.   */
/* This routine uses this information to initialize the subchannel   */
/* for further use.                                                  */
/*-------------------------------------------------------------------*/
static void config_subchan( DEVBLK *dev, struct io_cb *cb_ptr, BYTE *config_data )
{
    int cd_len;
    struct config_reply *reply_ptr;
    static unsigned char Ok[] = {0xd6, 0x92},           /* in EBCDIC */
        Failed[] = {0xc6, 0x81, 0x89, 0x93, 0x85, 0x84};

    cd_len = ntohs( ((struct buff_hdr *) config_data)->buffer_length );

    /* Build the reply right on top of the configuration data */
    reply_ptr = (struct config_reply *) config_data;

    if ( cb_ptr->state != SHUTDOWN )
    {
        /* This really should be an error, but MTS is bad about not properly
           closing Him devices when it's done with them (especially MSource
           devices */
           debug_pf( "Config record in active state, reseting device\n" );
           reset_io_cb( cb_ptr );

    }

    if ( !parse_config_data( cb_ptr, (char *) &config_data[4], cd_len) )
    {
        if ( cb_ptr->sock > 0 )
            (void) close_socket( cb_ptr->sock );
        goto failed;

    }
    else
    {                              /* Set up socket for non-servers. */
        if ( !cb_ptr->server )
        {
            cb_ptr->sock =
                get_socket( dev, cb_ptr->protocol, cb_ptr->bind_addr,
                    cb_ptr->mts_header.sh.tcp_header.th_dport,
                    &cb_ptr->sin, cb_ptr->passive ? QLEN : 0 );
            if ( cb_ptr->sock < 0 )
                goto failed;

            /* Save the address we're bound to */
            cb_ptr->mts_header.ip_header.Ip_Dst =
                cb_ptr->our_addr = cb_ptr->sin.sin_addr.s_addr;
            /*  Set the destination port in the MTS header as well   */
            cb_ptr->mts_header.sh.tcp_header.th_dport = cb_ptr->sin.sin_port;

        }
        else if ( cb_ptr->server &&
            cb_ptr->mts_header.sh.tcp_header.th_dport == 0 )

        {
            // A server listening on port zero
            if ( cb_ptr->protocol == IPPROTO_UDP )
            {
                if ( add_UDP_server_listener( dev, cb_ptr ) < 0 )
                {
                    goto failed;

                }
            }
            else {
                if ( add_TCP_server_listener( dev, cb_ptr ) < 0 )
                {
                    goto failed;

                }
            }
        }

        /* Finish initializing the configuration command reply       */
        memset( (char *) reply_ptr, '\0', sizeof( struct config_reply ) );
        reply_ptr->him_hdr.bh_flags = BH_INIT;
        reply_ptr->him_hdr.buffer_number = 1;
        reply_ptr->him_hdr.buffer_length =
            htons( sizeof( struct config_reply ) - sizeof( struct buff_hdr ) );

        memcpy( reply_ptr->config_ok, Ok, 2 );        /* EBCDIC "Ok" */
        reply_ptr->family = AF_UNIX;
        reply_ptr->protocol = cb_ptr->protocol;
        reply_ptr->local_port = cb_ptr->mts_header.sh.tcp_header.th_dport;
        /* reply_ptr->local_ip = cb_ptr->mts_header.ip_header.ip_dst; */
        memcpy( reply_ptr->local_ip, &cb_ptr->mts_header.ip_header.ip_dst, 4 );

        set_state( cb_ptr, INITIALIZED );
    }
    return;

failed:
    reset_io_cb(cb_ptr);

    reply_ptr->him_hdr.bh_flags = BH_INIT;
    reply_ptr->him_hdr.buffer_number = 1;
    reply_ptr->him_hdr.buffer_length = htons( 6 );
    memcpy( reply_ptr->config_ok, Failed, 6 ); /* EBCDIC "Failed" */
    return;

} /* end function config_subchan */


/*-------------------------------------------------------------------*/
/* This routine resets the HIM data to the intial state              */
/*-------------------------------------------------------------------*/
static void reset_io_cb( struct io_cb *cb_ptr )
{
    in_addr_t bind_addr;

    // If this is a server waiting for a call, terminate the wait
    if ( cb_ptr->server && cb_ptr->state == INITIALIZED )
    {
        if ( cb_ptr->protocol == IPPROTO_TCP )
            remove_TCP_server_listener( cb_ptr );
        else
            remove_UDP_server_listener( cb_ptr );
    }

    bind_addr = cb_ptr->bind_addr;
    if ( cb_ptr->sock > 0 )
        (void) close_socket( cb_ptr->sock );

    memset( (char *) cb_ptr, '\0', sizeof( struct io_cb ) );
    cb_ptr->sock = 0;
    cb_ptr->bind_addr = bind_addr;
    cb_ptr->state = SHUTDOWN;

} /* end function reset_io_cb */


/*-------------------------------------------------------------------*/
/* This routine uses the configuration string that MTS sends to      */
/* initialize the TCP/IP header in the I/O control block. An example */
/* configuration string might look like this:                        */
/*                                                                   */
/*    type=internet protocol=tcp active local_socket=(0,0.0.0.0)     */
/*-------------------------------------------------------------------*/
static int parse_config_data( struct io_cb *cb_ptr,
    char *config_string, int cs_len )
{
    char *lhs_token, *rhs_token = NULL, *echo_rhs = NULL;
    int port, i, j, success = 1;
    in_addr_t ip_addr = INADDR_ANY;

    enum lhs_codes {LHS_TYPE, LHS_PROTOCOL, LHS_ACTIVE, LHS_PASSIVE,
                    LHS_LOCALSOCK, LHS_FOREIGNSOCK, LHS_SERVER};

    static char *lhs_tbl[] = {
        "type",         "protocol",       "active",    "passive",
        "local_socket", "foreign_socket", "server"};

    /*---------------------------------------------------------------*/
    /* Build an MTS TCP/IP header                                    */
    /*---------------------------------------------------------------*/

    cb_ptr->mts_header.him_hdr.buffer_number = 1;
    cb_ptr->mts_header.him_hdr.buffer_length = htons( 40 );

    cb_ptr->mts_header.ip_header.ip_v = IPVERSION;
    cb_ptr->mts_header.ip_header.ip_hl = 5;
    cb_ptr->mts_header.ip_header.ip_len = htons( 40 );
    cb_ptr->mts_header.ip_header.ip_id = htons( 1 );
    cb_ptr->mts_header.ip_header.ip_ttl = 58;
    cb_ptr->mts_header.ip_header.ip_p = IPPROTO_TCP;
    cb_ptr->mts_header.ip_header.Ip_Dst = cb_ptr->our_addr;

    cb_ptr->mts_header.sh.tcp_header.th_seq = htonl( 1 );
    cb_ptr->mts_header.sh.tcp_header.th_off = 5;
    cb_ptr->mts_header.sh.tcp_header.th_flags = TH_ACK;
    cb_ptr->mts_header.sh.tcp_header.th_win = htons( 6 * 4096 );

    /*---------------------------------------------------------------*/
    /* Now, convert the EBCDIC configuration command that MTS just   */
    /* sent to ASCII, parse the string and use that information to   */
    /* update the MTS TCP/IP header.                                 */
    /*---------------------------------------------------------------*/

    config_string[cs_len] = '\0';

    while ( --cs_len >= 0 )
        config_string[cs_len] = tolower( guest_to_host( (u_char)config_string[cs_len] ) );

    lhs_token = strtok( config_string, " =" );

    do {
        for ( i = 0; ( strcmp( lhs_token, lhs_tbl[i] ) != 0 ) && i < LHS_SERVER; i++ ) ;

        switch( i ) {
        case LHS_TYPE:
            echo_rhs = rhs_token = strtok( NULL, " =" );
            break;

        case LHS_PROTOCOL:
            echo_rhs = rhs_token = strtok( NULL, " =" );
            cb_ptr->mts_header.ip_header.ip_p = cb_ptr->protocol =
                strcmp(rhs_token, "udp") == 0 ? IPPROTO_UDP : IPPROTO_TCP;
            break;

        case LHS_ACTIVE:
        case LHS_PASSIVE:
            echo_rhs = rhs_token = NULL;
            cb_ptr->passive = ( i == LHS_PASSIVE );
            break;

        case LHS_LOCALSOCK:
        case LHS_FOREIGNSOCK:
            echo_rhs = rhs_token = strtok( NULL, " =" );
            rhs_token++;
            port = strtol( rhs_token, &rhs_token, 10 );

            for ( j = 0; j < 4; j++ )
            {
                rhs_token++;
                ip_addr = ( ip_addr << 8 ) | strtol( rhs_token, &rhs_token, 10 );
            }

            if ( i == LHS_LOCALSOCK )     /* Set local socket values */
            {
                /* use provided address, address we're bound to, or address
                   requested in device config, whichever is specified. */
                cb_ptr->mts_header.ip_header.Ip_Dst =
                   ip_addr != INADDR_ANY ? ip_addr :
                   (cb_ptr->our_addr != INADDR_ANY ? cb_ptr->our_addr :
                   cb_ptr->bind_addr);
                cb_ptr->mts_header.sh.tcp_header.th_dport = htons( port );
            }
            else                        /* Set foreign socket values */
            {
                cb_ptr->mts_header.ip_header.Ip_Src = ip_addr;
                cb_ptr->mts_header.sh.tcp_header.th_sport = htons( port );
            }
            break;

        case LHS_SERVER:
            echo_rhs = rhs_token = NULL;
            cb_ptr->server = 1;
            break;
        } /* end switch( i ) */

        if ( echo_rhs == NULL )
            debug_pf( " %s, no right hand side\n", lhs_token );

        else
            debug_pf( " %s = %s\n", lhs_token, echo_rhs );

    } while ( (lhs_token = strtok( NULL, " =" )) );

    return success;

} /* end function parse_config_data */


/*-------------------------------------------------------------------*/
/* Get_Socket - allocate & bind a socket using TCP or UDP            */
/* Returns -2 for adrress in use error and -1 for any other error    */
/*-------------------------------------------------------------------*/
static int get_socket( DEVBLK *dev, int protocol, in_addr_t bind_addr, int port,
    struct sockaddr_in *sin, int qlen )
{
    /* int protocol;              * protocol to use ("IPPROTO_TCP" or "IPPROTO_UDP") *
       int port;                  * Port number to use (in net order) or       *
                                    0 for any port                             *
       in_addr_t bind_addr        * Address to bind ot or INADDR_ANY           *
       struct sockaddr_in *sin;   * will be returned with assigned port        *
       int qlen;                  * maximum length of the server request queue */
       int rc;

    int s, socktype, optval;  /* socket descriptor and socket type   */
    struct sockaddr_in our_sin;
    unsigned int sinlen = sizeof( struct sockaddr_in );

    memset( (char *)&our_sin, '\0', sizeof( struct sockaddr_in ) );
    our_sin.sin_family = AF_INET;
    our_sin.sin_port = port;
    our_sin.sin_addr.s_addr = bind_addr;

    /* Use protocol to choose a socket type */
    socktype = protocol == IPPROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;


    /* Allocate a socket */
    s = socket( PF_INET, socktype, 0 );
    if ( s < 0 )
    {
        // ""%1d:%04X HIM: Error in function %s: %s"
        WRMSG( HHC01150, "E", LCSS_DEVNUM, "socket()", strerror( HSO_errno ));
        return -1;
    }

    /* Set REUSEADDR option */
    optval = 4;
    if ( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, (GETSET_SOCKOPT_T*)&optval, sizeof( optval ) ) < 0 )
    {
        // ""%1d:%04X HIM: Error in function %s: %s"
        WRMSG( HHC01150, "E", LCSS_DEVNUM, "setsockopt()", strerror( HSO_errno ));
        close_socket(s);
        return -1;
    }

    /* Bind the socket */
    if ( bind( s, (struct sockaddr *)&our_sin, sizeof( struct sockaddr_in ) ) < 0 )
    {
        // ""%1d:%04X HIM: Error in function %s: %s"
        WRMSG( HHC01150, "E", LCSS_DEVNUM, "bind()", strerror( HSO_errno ));
        rc = HSO_errno;
        close_socket(s);
        return rc == HSO_EADDRINUSE ? -2 : -1;
    }

    /* Retrieve complete socket info */
    if ( getsockname( s, (struct sockaddr *)&our_sin, &sinlen ) < 0 )
    {
        // ""%1d:%04X HIM: Error in function %s: %s"
        WRMSG( HHC01150, "E", LCSS_DEVNUM, "getsockname()", strerror( HSO_errno ));
        close_socket(s);
        return -1;
    }
    else
        debug_pf( "In get_socket(), port = %d\n", ntohs(our_sin.sin_port) );

    if ( socktype == SOCK_STREAM && qlen && listen( s, qlen ) < 0 )
    {
        // ""%1d:%04X HIM: Error in function %s: %s"
        WRMSG( HHC01150, "E", LCSS_DEVNUM, "listen()", strerror( HSO_errno ));
        close_socket(s);
        return -1;
    }

    if ( sin != NULL )
        memcpy( sin, (char *)&our_sin, sizeof( struct sockaddr_in ) );

    return s;

} /* end function get_socket */


/*-------------------------------------------------------------------*/
/* Set up a Maximum Segment Size (MSS) acknowledgement               */
/*-------------------------------------------------------------------*/
static int return_mss( struct io_cb *cb_ptr, struct packet_hdr *mss )
{
    cb_ptr->mts_header.him_hdr.buffer_number++;
    cb_ptr->mts_header.ip_header.ip_id =
        htons( ntohs( cb_ptr->mts_header.ip_header.ip_id ) + 1 );

    *mss = cb_ptr->mts_header;
    mss->him_hdr.buffer_length = mss->ip_header.ip_len =
        htons( sizeof( struct packet_hdr ) - sizeof( struct buff_hdr ) );
    mss->ip_header.ip_ttl = MAXTTL;
    mss->sh.tcp_header.th_off = 6;
    mss->sh.tcp_header.th_flags |= TH_SYN;
    mss->tcp_optcode = TCPOPT_MAXSEG;
    mss->tcp_optlen = 4;
    mss->tcp_optval = htons( 1460 );

    return sizeof( struct packet_hdr );

} /* end function return_mss */


/*-------------------------------------------------------------------*/
/* Start a thread to watch for incoming data on our IP socket        */
/*-------------------------------------------------------------------*/
static int start_sock_thread( DEVBLK* dev )
{
    TID tid;
    int rc;

    ((struct io_cb *)dev->dev_data)->watch_sock = 1;

    rc = create_thread( &tid, DETACHED, skt_thread, dev, "him_data" );
    if ( rc )
    {
        WRMSG( HHC00102, "E", strerror( rc ) );
        return 0;
    }
    return 1;

} /* end function start_sock_thread */


/*-------------------------------------------------------------------*/
/* Thread to monitor our IP socket for incoming data                 */
/*-------------------------------------------------------------------*/
static void* skt_thread( void* arg )
{
    int rc, poll_timer, sleep_timer;
    struct pollfd read_chk;
    DEVBLK* dev = (DEVBLK *) arg;

    /* Fix thread name */
    {
        char thread_name[16];
        thread_name[sizeof( thread_name )-1] = 0;
        snprintf( thread_name, sizeof( thread_name )-1,
            "skth_%1d:%04X", SSID_TO_LCSS(dev->ssid), dev->devnum );
        SET_THREAD_NAME( thread_name );
    }

    read_chk.fd = ((struct io_cb *)dev->dev_data)->sock;
    read_chk.events = POLLIN;
    poll_timer = 10;       /* milliseconds */
    sleep_timer = 10000;   /* microseconds */

    while ( ((struct io_cb *)dev->dev_data)->watch_sock )
        if ( !((struct io_cb *)dev->dev_data)->rnr && poll(&read_chk, 1, poll_timer) > 0 )
        {
            rc = device_attention (dev, CSW_ATTN);
            ((struct io_cb *)dev->dev_data)->attn_rc[rc]++;
            ((struct io_cb *)dev->dev_data)->watch_sock = 0;
            break;
        }
        else
            usleep( sleep_timer );

    return NULL;

} /* end function skt_thread */


/*-------------------------------------------------------------------*/
/* Increment the count of TCP server devices listening               */
/*-------------------------------------------------------------------*/
static int add_TCP_server_listener( DEVBLK *dev, struct io_cb *cb_ptr )
{
    /* Increment the count of the number of HIM devices waiting
       for a server TCP connection and start the listener thread if
       it is not running.
    */

    TID tid;
    int rc;

    obtain_lock( &TCPServerLock );

    // Increment the count of devices listening
    TCPServerCount++;

    // Start the listener thread if it is not running.
    if ( !TCPServerThreadRunning )
    {
        SSLTA* sslta = malloc( sizeof( SSLTA ));

        sslta->dev    = dev;
        sslta->cb_ptr = cb_ptr;

        /* First one, start the thread to listen for a connection */
        rc = create_thread( &tid, DETACHED, TCP_sserver_listen_thread,
                     sslta, "TCP_listener" );
        if ( rc )
        {
            release_lock( &TCPServerLock );
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ) );
            free( sslta );
            return 0;
        }
        TCPServerThreadRunning = 1;
    }

    release_lock( &TCPServerLock );
    return 1;

} /* end function add_TCP_server_listener */


/*-------------------------------------------------------------------*/
/* Increment the count of UDP server devices listening               */
/*-------------------------------------------------------------------*/
static int add_UDP_server_listener( DEVBLK *dev, struct io_cb *cb_ptr )
{
    /* Increment the count of the number of HIM devices waiting
       for a server UDP connection and start the listener thread if
       it is not running.
    */

    TID tid;
    int rc;

    obtain_lock( &UDPServerLock );

    // Increment the count of devices listening
    UDPServerCount++;

    // Start the listener thread if it is not running.
    if ( !UDPServerThreadRunning )
    {
        SSLTA* sslta = malloc( sizeof( SSLTA ));

        sslta->dev    = dev;
        sslta->cb_ptr = cb_ptr;

        /* First one, start the thread to listen for a connection */
        rc = create_thread( &tid, DETACHED, UDP_sserver_listen_thread,
                     sslta, "UDP_listener" );
        if ( rc )
        {
            release_lock( &UDPServerLock );
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ) );
            free( sslta );
            return 0;
        }
        UDPServerThreadRunning = 1;
    }

    release_lock( &UDPServerLock );
    return 1;

} /* end function add_UDP_server_listener */


/*-------------------------------------------------------------------*/
/* Change the state of the connection                                */
/*-------------------------------------------------------------------*/
static void set_state( struct io_cb *cb_ptr, t_state state )
{
    /* If this is a server that is listening for a
       connection let the listener thread know it's gone */
    if ( cb_ptr->server &&
         cb_ptr->state == INITIALIZED && state != INITIALIZED )
    {
        if ( cb_ptr->protocol == IPPROTO_TCP )
            remove_TCP_server_listener( cb_ptr );
        else
            remove_UDP_server_listener( cb_ptr );
    }

    cb_ptr->state = state;

} /* end function set_state */


/*-------------------------------------------------------------------*/
/* Decrement the count of TCP server devices listening               */
/*-------------------------------------------------------------------*/
static int remove_TCP_server_listener( struct io_cb *cb_ptr )
{
    UNREFERENCED( cb_ptr );

    /* Decrement the count of HIM devices waiting for a server connection.
       If the count goes to zero the listener thread will notice and stop.
    */

    obtain_lock( &TCPServerLock );

    // Decrement the count of devices listening
    TCPServerCount--;

    // Don't let it be less than zero
    if ( TCPServerCount < 0 )
        TCPServerCount = 0;

    release_lock( &TCPServerLock );
    return 1;

} /* end function remove_TCP_server_listener */


/*-------------------------------------------------------------------*/
/* Decrement the count of UDP server devices listening               */
/*-------------------------------------------------------------------*/
static int remove_UDP_server_listener( struct io_cb *cb_ptr )
{
    UNREFERENCED( cb_ptr );

    /* Decrement the count of HIM devices waiting for a server connection.
       If the count goes to zero the listener thread will notice and stop.
    */

    obtain_lock( &UDPServerLock );

    // Decrement the count of devices listening
    UDPServerCount--;

    // Don't let it be less than zero
    if ( UDPServerCount < 0 )
        UDPServerCount = 0;

    release_lock( &UDPServerLock );
    return 1;

} /* end function remove_UDP_server_listener */


/*-------------------------------------------------------------------*/
/* Thread to listen for incoming TCP server connections              */
/*-------------------------------------------------------------------*/
static void* TCP_sserver_listen_thread( void* arg )
{
    /* Table of ports to listen on.  This should agree with the
       contents of HOST:SPVT_LCS*SQ on MTS.  This maps port numbers
       to MTS server names.  If a connection is made on a port not
       in this table a server named "TCPnnn" will be started.

       Here is the info from SPVT and NSERVTBL for TCP ports

       TCP ports in SPVT
        7       Echo
        9       Discard
        20      FTP-Data
        23      Telnet
        25      SMTP    POST:SMTP-CMD
        79      Finger  SERV:FINGER
        109     POP2    POP:POP2.CMD (commented out)
        110     POP3    POP:POP3.CMD
        143     Imap2   POP:IMAP2.CMD (commented out)
        220     Imap3   POP:IMAP3.CMD (commented out)
        1010    (blank server name) LPDO
        1011    (blank server name) LPDO
        1309    (blank server name) LPDO
        2110    POP3Test POP:POP3.TEST
        3217    (blank server name) LPDO
        4242    Testing

        TCP Ports not in SPVT
        2025            POST,POST:SMTP-TEST
        2026            MOC.:check_vitals
        2110            POP:POP3.TEST (Duplicate)
        9998            ASRV:ACCSERV*C

    */

    #define TCP_PORT_COUNT 17
    u_short ports[TCP_PORT_COUNT] = {23, 25, 79, 109, 110, 143, 220, 1010,
                    1011, 1309, 2110, 3217, 4242, 2025, 2026, 2110, 9998};
    int sockets[TCP_PORT_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int max_socket = 0;
    int i, j;

    fd_set socket_set;      /* set of sockets to listen on */

    /* Retrieve arguments */
    SSLTA* sslta = arg;
    DEVBLK *dev           = sslta->dev;
    struct io_cb *cb_ptr  = sslta->cb_ptr;
    free( sslta );

    max_socket = 0;
    FD_ZERO( &socket_set );
    for ( i = 0; i < TCP_PORT_COUNT; i++ )
    {
        int s;
        s = get_socket( dev, IPPROTO_TCP, cb_ptr->bind_addr, htons( ports[i] ),
                        NULL, QLEN );
        if  ( s < 0 )
        {
            /* Couldn't get a socket, skip it, a message will have sent */
            sockets[i] = 0;
        }
        else
        {
            sockets[i] = s;
            if ( s > max_socket )
                max_socket = s;
            FD_SET( s, &socket_set );
            /* Make it non-blocking since we don't want to wait while
               we are holding a lock */
            socket_set_blocking_mode( s, 0 );
        }
    }

    /* Listen for an incoming connection until there are no more servers */
    for ( ; ; )
    {
        struct timespec slowpoll = { 0, 100000000 };         /* 100ms */
        fd_set listen_set;
        int rc;
        struct io_cb *cb_ptr;

        /* See if we should quit */
        obtain_lock( &TCPServerLock );
        if ( TCPServerCount == 0 )
        {
            /* We're done, clean up and go home */
            for ( i = 0; i < TCP_PORT_COUNT; i++ )
            {
                if ( sockets[i] != 0 )
                    (void) close_socket( sockets[i] );
                sockets[i] = 0;
            }
            TCPServerThreadRunning = 0;
            release_lock( &TCPServerLock );
            break;  // and exit

        }

        release_lock( &TCPServerLock );

        listen_set = socket_set;
        rc = pselect ( max_socket+1, &listen_set, NULL, NULL, &slowpoll, NULL);

        if ( rc == 0 )
            /* Nothing ready */
            continue;

        if ( rc < 0 )
        {
            // ""%1d:%04X HIM: Error in function %s: %s"
            WRMSG( HHC01150, "E", LCSS_DEVNUM, "pselect()", strerror( HSO_errno ));
            usleep( 50000 ); // (wait a bit; maybe it'll fix itself??)
            continue;

        }

        /* See which ports have pending connections */
        for ( i = 0; i < TCP_PORT_COUNT; i++ )
        {
            if ( FD_ISSET( sockets[i], &listen_set ) )
            {
                /* Pending connection, find a HIM device for it */
                DEVBLK *dev;
                int csock;
                struct sockaddr_in our_sin;
                unsigned int sinlen = sizeof( struct sockaddr_in );

                for ( dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev )
                {
                    if ( dev->allocated && dev->himdev )
                    {
                        /* Seems to be a HIM device, take a closer look. */
                        if ( try_obtain_lock( &dev->lock ) )
                            /* Couldn't lock device, skip it */
                            continue;
                        /* Now that it's locked see if it is a server HIM
                           device waiting for a TCP connection */
                        cb_ptr = (struct io_cb *) dev->dev_data;
                        if ( dev->allocated &&
                             dev->himdev &&
                             cb_ptr->server &&
                             cb_ptr->passive &&
                             cb_ptr->state == INITIALIZED &&
                             cb_ptr->protocol == IPPROTO_TCP &&
                             cb_ptr->sock <= 0
                             )
                            break;      // from device loop
                        release_lock( &dev->lock );
                    }
                }   // End of device loop

                if ( dev == 0 )
                {
                    /* couldn't find a device.  This shouldn't happen, but if
                       it does just try again later. */
                    usleep( 100000); /* wait 1/10 second */
                    break;           /* back to the port loop */
                }

                /* Accept the connection and assign the socket to
                   the waiting HIM device.  The device is locked. */
                csock = accept( sockets[i],
                                (struct sockaddr *)&cb_ptr->sin,
                                &sinlen);

                if ( csock < 0 )
                {
                    /* accept failed, see why */
                    int accept_errno = HSO_errno; // (preserve orig errno)
                    if ( EINTR != accept_errno && HSO_EWOULDBLOCK != accept_errno )
                    {
                        // ""%1d:%04X HIM: Error in function %s: %s"
                        WRMSG( HHC01150, "E", LCSS_DEVNUM, "accept()", strerror( accept_errno ));
                        usleep( 100000 );
                    }
                    release_lock( &dev->lock );
                    continue;   // Loop over ports

                }

                /* have a connection socket, do something with it */
                set_state( cb_ptr, CONNECTED );
                cb_ptr->sock = csock;

                // Save the address and port of the remote host
                cb_ptr->mts_header.ip_header.ip_src = cb_ptr->sin.Sin_Addr;
                cb_ptr->mts_header.sh.tcp_header.th_sport = cb_ptr->sin.sin_port;
                // Also save the address and port of our end
                if ( getsockname( csock, (struct sockaddr *)&our_sin, &sinlen ) < 0 )
                {
                    // ""%1d:%04X HIM: Error in function %s: %s"
                    WRMSG( HHC01150, "W", LCSS_DEVNUM, "getsockname()", strerror( HSO_errno ));
                }

                cb_ptr->mts_header.ip_header.ip_dst = our_sin.Sin_Addr;
                cb_ptr->mts_header.sh.tcp_header.th_dport = our_sin.sin_port;

                /* Queue an MSS acknowledgement */
                for ( j = 0; cb_ptr->read_q[j] != EMPTY; j++ ) ;
                cb_ptr->read_q[j] = MSS;

                /* Unlock the device and queue an attention interrupt */
                release_lock( &dev->lock );
                /* This MTS job is no longer waiting for a call. */
                remove_TCP_server_listener(cb_ptr);
                rc = device_attention (dev, CSW_ATTN);
                ((struct io_cb *)dev->dev_data)->attn_rc[rc]++;
            }
        }
    }

    return 0;

} /* end function TCP_sserver_listen_thread */


/*-------------------------------------------------------------------*/
/* Thread to listen for incoming UDP server connections              */
/*-------------------------------------------------------------------*/
static void* UDP_sserver_listen_thread( void* arg )
{
    /* Table of ports to listen on.  This should agree with the
       contents of HOST:SPVT_LCS*SQ on MTS.  This maps port numbers
       to MTS server names.  If a connection is made on a port not
       in this table a server named "UDPnnn" will be started.

       Here is the info from SPVT and NSERVTBL for UDP ports

        (None in SPVT)
        7               UDP.:ECHO*C
        9               UDP.:DISCARD*C
        11              UDP.:USERS*C
        13              UDP.:DAYTIME*C
        15              UDP.:NETSTAT*C
        17              UDP.:QUOTE*C
        19              UDP.:CHARGEN*C
        37              UDP.:TIME*C
        42              UDP.:NAMESERV*C
        53              UDP.:DOMAIN*C
        59              UDP.:MACEFSIP*C
        69              UDP.:TFTP*C
        79              UDP.:FINGER*C
        123             UDP.:NTP*C
        129             UDP.:PWDGEN*C
        424             AUTH,AUTH:UDPSERV
    */

    #define UDP_PORT_COUNT 15
    u_short ports[UDP_PORT_COUNT] = {7, 9, 11, 13, 15, 17, 19, 37, 42, 53, 59,
                                    69, 79, 123, 129};
    int sockets[UDP_PORT_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int max_socket = 0;
    int i;
    int retry_count;

    fd_set socket_set;      /* set of sockets to listen on */

    /* Retrieve arguments */
    SSLTA* sslta = arg;
    DEVBLK *dev           = sslta->dev;
    struct io_cb *cb_ptr  = sslta->cb_ptr;
    free( sslta );

    max_socket = 0;
    FD_ZERO( &socket_set );
    retry_count = 0;

    /* Listen for an incoming connection until there are no more servers */
    for ( ; ; )
    {
        /* See if we should quit */
        obtain_lock( &UDPServerLock );
        if ( UDPServerCount == 0 )
        {
            /* We're done, clean up and go home */
            for ( i=0; i<UDP_PORT_COUNT; i++ )
            {
                if ( sockets[i] != 0 )
                    (void) close_socket( sockets[i] );
                sockets[i] = 0;
            }
            UDPServerThreadRunning = 0;
            release_lock( &UDPServerLock );
            break;  // and exit
        }
        release_lock( &UDPServerLock );

        /* get a socket for any missing port */
        for ( i = 0; i < UDP_PORT_COUNT; i++ )
        {
            if ( sockets[i] == 0 || (sockets[i] < 0 && retry_count <= 0) ) {
                int s;
                s = get_socket( dev, IPPROTO_UDP, cb_ptr->bind_addr, htons( ports[i] ),
                                NULL, QLEN );
                if  ( s < 0 )
                {
                    /* Couldn't get a socket, skip it, a message will have sent */
                    if ( s == -1 )
                        /* A fatal error, don't try again soon */
                        sockets[i] = -1;
                    else
                        /* Already have a socket for this port. */
                        sockets[i] = 0;
                }
                else
                {
                    sockets[i] = s;
                    if ( s>max_socket )
                        max_socket = s;
                    FD_SET( s, &socket_set );
                    /* Make it non-blocking since we don't want to wait while
                       we are holding a lock */
                    socket_set_blocking_mode( s, 0 );
                }
            }
        }
        /* Adjust the retry count that controls how oftern we retry failures */
        if ( --retry_count < 0 )
            retry_count = 10;

        struct timespec slowpoll = { 0, 100000000 };         /* 100ms */
        fd_set listen_set;
        int rc;
        struct io_cb *cb_ptr;

        listen_set = socket_set;
        rc = pselect ( max_socket+1, &listen_set, NULL, NULL, &slowpoll, NULL );

        if ( rc == 0 )
            /* Nothing ready */
            continue;

        if ( rc < 0 )
        {
            // ""%1d:%04X HIM: Error in function %s: %s"
            WRMSG( HHC01150, "E", LCSS_DEVNUM, "pselect()", strerror( HSO_errno ));
            usleep( 50000 ); // (wait a bit; maybe it'll fix itself??)
            continue;

        }

        /* See which ports have pending input */
        for ( i = 0; i < UDP_PORT_COUNT; i++ )
        {
            if ( sockets[i] > 0 && FD_ISSET( sockets[i], &listen_set ) )
            {
                /* incoming data, find a HIM device for it */
                DEVBLK *dev;
                int csock;
                struct sockaddr_in our_sin;
                unsigned int sinlen = sizeof( struct sockaddr_in );
                char shortbuf[32];

                for ( dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev )
                {
                    if ( dev->allocated && dev->himdev )
                    {
                        /* Seems to be a HIM device, take a closer look. */
                        if ( try_obtain_lock( &dev->lock ) )
                            /* Couldn't lock device, skip it */
                            continue;
                        /* Now that it's locked see if it is a server HIM
                           device waiting for a UDP connection */
                        cb_ptr = (struct io_cb *) dev->dev_data;
                        if ( dev->allocated &&
                             dev->himdev &&
                             cb_ptr->server &&
                             cb_ptr->state == INITIALIZED &&
                             cb_ptr->protocol == IPPROTO_UDP &&
                             cb_ptr->sock <= 0 )
                            break;      // from device loop
                        release_lock( &dev->lock );
                    }
                }   // End of device loop

                if ( dev == 0 )
                {
                    /* couldn't find a device.  This shouldn't happen, but if
                       it does just try again later. */
                    usleep( 100000); /* wait 1/10 second */
                    break;           /* back to the port loop */
                }

                csock = sockets[i];
                sockets[i] = 0;
                /* have a connection socket, do something with it */
                set_state( cb_ptr, CONNECTED );
                cb_ptr->sock = csock;

                // Peek at the first packet since it contains the
                // IP address and port number of the source
                (void) recvfrom( cb_ptr->sock, shortbuf, 32, MSG_PEEK,
                    (struct sockaddr *)&cb_ptr->sin, &sinlen );

                // Save the address and port of the remote host
                cb_ptr->mts_header.ip_header.ip_src = cb_ptr->sin.Sin_Addr;
                cb_ptr->mts_header.sh.udp_header.uh_sport = cb_ptr->sin.sin_port;
                // Also save the address and port of our end
                if ( getsockname( csock, (struct sockaddr *)&our_sin, &sinlen ) < 0 )
                {
                    // ""%1d:%04X HIM: Error in function %s: %s"
                    WRMSG( HHC01150, "W", LCSS_DEVNUM, "getsockname()", strerror( HSO_errno ));
                }
                cb_ptr->mts_header.ip_header.ip_dst = our_sin.Sin_Addr;
                cb_ptr->mts_header.sh.udp_header.uh_dport = our_sin.sin_port;

                /* Unlock the device and queue an attention interrupt */
                release_lock( &dev->lock );
                /* This MTS job is no longer waiting for a call. */
                remove_UDP_server_listener(cb_ptr);
                rc = device_attention (dev, CSW_ATTN);
                ((struct io_cb *)dev->dev_data)->attn_rc[rc]++;

            }
        }
    }

    return 0;

} /* end function UDP_sserver_listen_thread */


/*-------------------------------------------------------------------*/
/* Used for dumping debugging data in a formatted hexadecimal form   */
/*-------------------------------------------------------------------*/
static void dumpdata( char *label, BYTE *data, int len )
{
#if _ENABLE_TRACING_STMTS_IMPL
    char *hex = "0123456789ABCDEF", ascii_hex[80];
    int index = 0, space_chk = 0;

    if ( strlen(label) > 0 )
        debug_pf( "%s: \n", label );

    if ( len > 256 )
    {
        debug_pf( "Dumpdata len = %i, will be truncated\n", len );
        len = 256;
    }

    while ( len-- > 0 )
    {
        ascii_hex[index++] = hex[(*data >> 4) & 0xF];
        ascii_hex[index++] = hex[*data & 0xF];

        space_chk++;
        if ( space_chk % 4 == 0 )
            ascii_hex[index++] = ' ';

        if ( index > 71 )
        {
            ascii_hex[index] = '\0';
            debug_pf( "%s\n", ascii_hex );
            index = space_chk = 0;
        }
        data++;

    }

    ascii_hex[index] = '\0';
    if ( strlen(ascii_hex) > 0 )
        debug_pf( "%s\n", ascii_hex );
#else
    UNREFERENCED( label );
    UNREFERENCED( data );
    UNREFERENCED( len );
#endif
} /* end function dumpdata */


/*-------------------------------------------------------------------*/
/* Used for writing debug output                                     */
/*-------------------------------------------------------------------*/
static void debug_pf( const char* __fmt, ... )
{
#if _ENABLE_TRACING_STMTS_IMPL
    char write_buf[2000];             /* big enough for an IP packet */
    int writebuf_len;
    va_list arglist;

    va_start( arglist, __fmt );
    writebuf_len = vsprintf( write_buf, __fmt, arglist );
    va_end( arglist );

    #ifdef WRITEDBG
      write( 5, write_buf, writebuf_len );
    #else
      TRACE( "%s", write_buf );
    #endif
#else
    UNREFERENCED( __fmt );
#endif

} /* end function debug_pf */
