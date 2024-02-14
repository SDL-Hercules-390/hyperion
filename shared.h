/* SHARED.H     (c)Copyright Greg Smith, 2002-2012                   */
/*              Shared Device Server                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------
 * Shared device support           (c)Copyright Greg Smith, 2002-2009
 *
 * Shared device support allows multiple Hercules instances to share
 * devices.  The device will be 'local' to one instance and 'remote'
 * to all other instances.  The local instance is the  * 'server' for
 * that device and the remote instance is the 'client'.  You do not
 * have to IPL an operating system on the device server.  Any number
 * of Hercules instances can act as a server in a Hercplex ;-)
 *
 * To use a device on a remote system, instead of specifying a file
 * name on the device config statement, you specify
 *
 *     ip_address_or_name:port:devnum
 *
 * For example:
 *
 *     0100 3350 localhost:3990:0100
 *
 * which says there is a device server on the local host listening
 * on port 3990 and we want to use its 0100 device as 0100.  The
 * default port is 3990 and the default remote device number is the
 * local device number.  So we could say
 *
 *     0100 3350 localhost
 *
 * instead, providing we don't actually have a file 'localhost'.
 * Interestingly, the instance on the local host listening on 3990
 * could have a statement
 *
 *     0100 3350 192.168.200.1::0200
 *
 * which means that instance in turn will use device 0200 on the
 * server at 192.168.200.1 listening on port 3990.  The original
 * instance will have to 'hop' thru the second instance to get
 * to the real device.
 *
 * Device sharing can be 'split' between multiple instances.
 * For example, suppose instance A has
 *
 *     SHRDPORT 3990
 *     0100 3350 localhost:3991
 *     0101 3350 mvscat
 *
 * and instance B has
 *
 *     SHRDPORT 3991
 *     0100 3350 mvsres
 *     0101 3350 localhost
 *
 * Then each instance acts as both a client and as a server.
 *
 * When 'SHRDPORT' is specified, thread 'shared_server' is started
 * at the end of Hercules initialization.  In the example above,
 * neither Hercules instance can initialize their devices until the
 * server is started on each system.  In this case, the device trying
 * to access a server gets the 'connecting' bit set on in the DEVBLK
 * and the device still needs to initialize.  After the shared server
 * is started, a thread is attached for each device that is connecting
 * to complete the connection (which is the device init handler).
 *
 * TECHNICAL BS:
 *
 * There are (at least) two approaches to sharing devices.  One is to
 * execute the channel program on the server system.   The server will
 * need to request from the client system information such as the ccw
 * and the data to be written, and will need to send to the client
 * data that has been read and status information.  The second is to
 * execute the channel program on the client system.  Here the client
 * system makes requests to the server system to read and write data.
 *
 * The second approach is currently implemented.  The first approach
 * arguably emulates 'more correctly'.  However, an advantage of the
 * implemented approach is that it is easier because only the
 * client sends requests and only the server sends responses.
 *
 * Both client and server have a DEVBLK structure for the device.
 * Absurdly, perhaps, in originally designing an implementation for
 * shared devices it was not clear what type of process should be the
 * server.  It was a quantum leap forward to realize that it could
 * just be another hercules instance.
 *
 * PROTOCOL:
 * (If this section is as boring for you to read as it was for me
 *  to write then please skip to the next section ;-)
 *
 * The client sends an 8 byte request header and maybe some data:
 *
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * | cmd |flag |  devnum   |   length  |    id     |
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 *
 *             <-------- length --------->
 *             +----- .  .  .  .  . -----+
 *             |          data           |
 *             +----- .  .  .  .  . -----+
 *
 * 'cmd' identifies the client request.  The requests are:
 *
 * 0xe0  CONNECT        Connect to the server.  This requires
 *                      the server to allocate resources to
 *                      support the connection.  Typically issued
 *                      during device initialization or after being
 *                      disconnected after a network error or timeout.
 * 0xe1  DISCONNECT     Disconnect from the server.  The server
 *                      can now release the allocated resources
 *                      for the connection.  Typically issued during
 *                      device close or detach.
 * 0xe2  START          Start a channel program on the device.
 *                      If the device is busy or reserved by
 *                      another system then wait until the device
 *                      is available unless the NOWAIT flag bit
 *                      is set, then return a BUSY code.  Once
 *                      START succeeds then the device is unavailable
 *                      until the END request.
 * 0xe3  END            Channel program has ended.  Any waiters
 *                      for the device can now retry.
 * 0xe4  RESUME         Similar to START except a suspended
 *                      channel program has resumed.
 * 0xe5  SUSPEND        Similar to END except a channel program
 *                      has suspended itself.  If the channel
 *                      program is not resumed then the END
 *                      request is *not* issued.
 * 0xe6  RESERVE        Makes the device unavailable to any other
 *                      system until a RELEASE request is issued.
 *                      *Must* be issued within the scope of START/END.
 * 0xe7  RELEASE        Makes the device available to other systems
 *                      after the next END request.
 *                      *Must* be issued within the scope of
 *                      START/END.
 * 0xe8  READ           Read from a device.  A 4-byte 'record'
 *                      identifier is specified in the request
 *                      data to identify what data to read in the
 *                      device context.
 *                      *Must* be issued within the scope of START/END.
 * 0xe9  WRITE          Write to a device. A 2-byte 'offset' and
 *                      a 4-byte 'record' is specified in the request
 *                      data, followed by the data to be written.
 *                      'record' identifies what data is to be written
 *                      in the device context and 'offset' and 'length'
 *                      identify what to update in 'record'.
 *                      *Must* be issued within the scope of START/END.
 * 0xea  SENSE          Retrieves the sense information after an i/o
 *                      error has occurred on the server side.  This
 *                      is typically issued within the scope of the
 *                      channel program having the error.  Client side
 *                      sense or concurrent sense will then pick up the
 *                      sense data relevant to the i/o error.
 *                      *Must* be issued within the scope of START/END.
 * 0xeb  QUERY          Obtain device information, typically during
 *                      device initialization.
 * 0xec  COMPRESS       Negotiate compression parameters.  Notifies the
 *                      server what compression algorithms are supported
 *                      by the client and whether or not data sent back
 *                      and forth from the client or server should be
 *                      compressed or not.  Typically issued after CONNECT.
 *                      *NOTE* This action should actually be SETOPT or
 *                      some such; it was just easier to code a COMPRESS
 *                      specific SETOPT (less code).
 *
 * 'flag' qualifies the client request and varies by the request.
 *
 * 0x80  NOWAIT         For START, if the device is unavailable then
 *                      return BUSY instead of waiting for the device.
 * 0x40  QUERY          Identifies the QUERY request:
 * 0x41  DEVCHAR            Device characteristics data
 * 0x42  DEVID              Device identifier data
 * 0x43  DEVUSED            Hi used track/block (for dasdcopy)
 * 0x48  CKDCYLS            Number cylinders for CKD device
 * 0x4c  FBAORIGIN          Origin block for FBA
 * 0x4d  FBANUMBLK          Number of FBA blocks
 * 0x4e  FBABLKSIZ          Size of an FBA block
 * 0x3x  COMP           For WRITE, data is compressed at offset 'x':
 * 0x2x  BZIP2              using bzip2
 * 0x1x  LIBZ               using zlib
 * 0xxy                 For COMPRESS, identifies the compression
 *                      algorithms supported by the client (0x2y for bzip2,
 *                      0x1y for zlib, 0x3y for both) and the zlib compression
 *                      parameter 'y' for sending otherwise uncompressed data
 *                      back and forth.  If 'y' is zero (default) then no
 *                      uncompressed data is compressed between client & server.
 *
 * 'devnum' identifies the device by number on the server instance.
 *                     The device number may be different than the
 *                     device number on the client instance.
 * 'id' identifies the client to the server.  Each client has a unique
 *                     positive (non-zero) identifier.  For the initial
 *                     CONNECT request 'id' is zero.  After a successful
 *                     CONNECT, the server returns in the response header
 *                     the identifier to be used for all other requests
 *                     (including subsequent CONNECT requests).  This is
 *                     saved in dev->rmtid.
 * 'length' specifies the length of the data following the request header.
 *                     Currently length is non-zero for READ/WRITE requests.
 *
 * The server sends an 8 byte response header and maybe some data:
 *
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * |code |stat |  devnum   |   length  |    id     |
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 *
 *             <-------- length --------->
 *             +----- .  .  .  .  . -----+
 *             |          data           |
 *             +----- .  .  .  .  . -----+
 *
 * 'code' indicates the response to the request.  OK (0x00) indicates
 *                     success however other codes also indicate success
 *                     but qualified in some manner:
 * 0x80  ERROR         An error occurred.  The server provides an error
 *                     message in the data section.
 * 0x40  IOERR         An i/o error occurred during a READ/WRITE
 *                     request.  The status byte has the 'unitstat'
 *                     data.  This should signal the client to issue the
 *                     SENSE request to obtain the current sense data.
 * 0x20  BUSY          Device was not available for a START request and
 *                     the NOWAIT flag bit was turned on.
 * 0x10  COMP          Data returned is compressed.  The status byte
 *                     indicates how the data is compressed (zlib or
 *                     bzip2) and at what offset the compressed data
 *                     starts (0 .. 15).  This bit is only turned on
 *                     when both the 'code' and 'status' bytes would
 *                     otherwise be zero.
 * 0x08  PURGE         START request was issued by the client.  A list
 *                     of 'records' to be purged from local cache is
 *                     returned.  These are 'records' that have been
 *                     updated since the last START/END request from
 *                     the client by other systems.  Each record identifier
 *                     is a 4-byte field in the data segment.  The number
 *                     of records then is 'length'/4.  If the number of
 *                     records exceeds a threshold (16) then 'length'
 *                     will be zero indicating that the client should
 *                     purge all locally cached records for the device.
 *
 * 'stat' contains status information as a result of the request.
 *                     For READ/WRITE requests this contains the 'unitstat'
 *                     information if an IOERR occurred.
 *
 * 'devnum' specifies the server device number
 *
 * 'id' specifies the system identifier for the request.
 *
 * 'length' is the size of the data returned.
 *
 *
 * CACHING
 *
 * Cached records (eg CKD tracks or FBA blocks) are kept independently on
 * both the client and server sides.  Whenever the client issues a START
 * request to initiate a channel program the server will return a list
 * of records to purge from the client's cache that have been updated by
 * other clients since the last START request.  If the list is too large
 * the server will indicate that the client should purge all records for
 * the device.
 *
 * COMPRESSION
 *
 * Data that would normally be transferred uncompressed between client
 * and host can optionally be compressed by specifying the 'comp='
 * keyword on the device configuration statement or attach command.
 * For example
 *
 *     0100 3350 192.168.2.12 comp=3
 *
 * The value of the 'comp=' keyword is the zlib compression parameter
 * which should be a number between 1 .. 9.  A value closer to 1 means
 * less compression but less processor time to perform the compression.
 * A value closer to 9 means the data is compressed more but more processor
 * time is required.
 *
 * If the server is on 'localhost' then you should not specify 'comp='.
 * Otherwise you are just stealing processor time to do compression/
 * uncompression from hercules.  If the server is on a local network
 * then I would recommend specifying a low value such as 1, 2 or 3.
 * We are on a curve here, trying to trade cpu cycles for network traffic
 * to derive an optimal throughput.
 *
 * If the devices on the server are compressed devices (eg CCKD or CFBA)
 * then the 'records' (eg. track images or block groups) may be transferred
 * compressed regardless of the 'comp=' setting.  This depends on whether
 * the client supports the compression type (zlib or bzip2) of the record
 * on the server and whether the record is actually compressed in the
 * server cache.
 *
 * For example:
 *
 * Suppose on the client that you execute one or more channel programs
 * to read a record on a ckd track, update a record on the same track,
 * and then read another (or the same) record on the track.
 *
 * For the first read the server will read the track image and
 * pass it to the client as it was originally compressed in the file.
 * To update a portion of the track image the server must uncompress
 * the track image so data in it can be updated.  When the client next
 * reads from the track image, the track image is uncompressed.
 *
 * Specifying 'comp=' means that uncompressed data sent to the client
 * will be compressed.  If the data to be sent to the client is already
 * compressed then the data is sent as is, unless the client has indicated
 * that it does not support that compression algorithm.
 *
 *
 * TODO
 *
 *  1.  More doc (sorry, I got winded)
 *  2.  Delays observed during short transfers (redrive select?)
 *  3.  Better server side behaviour due to disconnect
 *  4.  etc.
 *
 *-------------------------------------------------------------------*/

#ifndef _SHARED_H
#define _SHARED_H

/*-------------------------------------------------------------------*/
/*  NOTE:  Differing Shared VERSION levels are NOT compatible!       */
/*         Differing Shared RELEASE levels ARE     compatible.       */
/*-------------------------------------------------------------------*/

#define SHARED_VERSION              0   /* Version level  (0 .. 15)  */
#define SHARED_RELEASE              2   /* Release level  (0 .. 15)  */

/* Constraints                                                       */
#define SHARED_DEFAULT_PORT      3990   /* Default shared port       */
#define SHARED_PURGE_MAX           16   /* Max size of purge list    */
#define SHARED_MAX_MSGLEN         255   /* Max message length        */
#define SHARED_TIMEOUT            120   /* Disconnect timeout (sec)  */
#define SHARED_SELECT_WAIT_MSECS   50   /* Select timeout (msecs)    */
#define SHARED_COMPRESS_MINLEN    512   /* Min length for compression*/
#define SHARED_MAX_SYS              8   /* Max number connections    */

/* Requests                                                          */
#define SHRD_CONNECT             0xe0   /* Connect                   */
#define SHRD_DISCONNECT          0xe1   /* Disconnect                */
#define SHRD_START               0xe2   /* Start channel program     */
#define SHRD_END                 0xe3   /* End channel program       */
#define SHRD_RESUME              0xe4   /* Resume channel program    */
#define SHRD_SUSPEND             0xe5   /* Suspend channel program   */
#define SHRD_RESERVE             0xe6   /* Reserve                   */
#define SHRD_RELEASE             0xe7   /* Release                   */
#define SHRD_READ                0xe8   /* Read data                 */
#define SHRD_WRITE               0xe9   /* Write data                */
#define SHRD_SENSE               0xea   /* Sense                     */
#define SHRD_QUERY               0xeb   /* Query                     */
#define SHRD_COMPRESS            0xec   /* Compress request          */

/* Response codes                                                    */
#define SHRD_OK                  0x00   /* Success                   */
#define SHRD_ERROR               0x80   /* Failure                   */
#define SHRD_IOERR               0x40   /* I/O error                 */
#define SHRD_BUSY                0x20   /* Resource is busy          */
#define SHRD_COMP                0x10   /* Data is compressed        */
#define SHRD_PURGE               0x08   /* Purge list provided       */

/* Error responses                                                   */
#define SHRD_ERROR_INVALID       0xf0   /* Invalid request           */
#define SHRD_ERROR_BADVERS       0xf1   /* Version mismatch          */
#define SHRD_ERROR_NOTINIT       0xf2   /* Device not initialized    */
#define SHRD_ERROR_NOTCONN       0xf3   /* Not connected to device   */
#define SHRD_ERROR_NOTAVAIL      0xf4   /* No available SHRD         */
#define SHRD_ERROR_NOMEM         0xf5   /* Out of memory             */
#define SHRD_ERROR_NOTACTIVE     0xf6   /* Not device owner          */
#define SHRD_ERROR_NODEVICE      0xf7   /* No such device            */
#define SHRD_ERROR_CONNECTED     0xf8   /* Already connected         */

/* Flags                                                             */
#define SHRD_NOWAIT              0x80   /* Don't wait if busy        */
#define SHRD_QUERY_REQUEST       0x40   /* Query request             */
#define SHRD_COMP_MASK           0x30   /* Mask to detect compression*/
#define SHRD_COMP_OFF            0x0f   /* Offset to compressed data */
#define SHRD_COMP_MAX_OFF          15   /* Max offset allowed        */
#define SHRD_LIBZ                0x01   /* Compressed using zlib     */
#define SHRD_BZIP2               0x02   /* Compressed using bzip2    */

/* Query Types                                                       */
#define SHRD_DEVCHAR             0x41   /* Device characteristics    */
#define SHRD_DEVID               0x42   /* Device identifier         */
#define SHRD_USED                0x43   /* Device usage              */
#define SHRD_SERIAL              0x44   /* Device serial number      */
#define SHRD_CKDCYLS             0x48   /* CKD number cylinders      */
#define SHRD_FBAORIGIN           0x4c   /* FBA origin                */
#define SHRD_FBANUMBLK           0x4d   /* FBA number blocks         */
#define SHRD_FBABLKSIZ           0x4e   /* FBA block size            */

typedef char SHRD_TRACE[128];           /* Trace entry               */

/*-------------------------------------------------------------------*/
struct SHRD                             /* Device Sharing ctl. blk.  */
{
        int     id;                     /* Identifier                */
        int     fd;                     /* Socket                    */
        char   *ipaddr;                 /* IP addr of connected peer */
        time_t  time;                   /* Time last request         */
        int     release;                /* Client release level      */
        int     comp;                   /* Compression parameter     */
        int     comps;                  /* Compression supported     */
        unsigned  pending:1,            /* 1=Request pending         */
                waiting:1,              /* 1=Waiting for device      */
                havehdr:1,              /* 1=Header already read     */
                disconnect:1;           /* 1=Disconnect device       */
        DBLWRD  hdr;                    /* Header                    */
        int     purgen;                 /* Number purge entries      */
        FWORD   purge[SHARED_PURGE_MAX];/* Purge list                */
};
/*-------------------------------------------------------------------*/
struct SHRD_HDR                         /* Device Sharing msg header */
{
        BYTE    cmd;                    /* 0 Command                 */
        BYTE    code;                   /* 1 Flags and Codes         */
        U16     devnum;                 /* 2 Device number           */
        U16     len;                    /* 4 Data length             */
        U16     id;                     /* 6 Identifier              */
                                        /* length is 8 bytes EXACTLY */
};
/*-------------------------------------------------------------------*/

typedef struct SHRD       SHRD;         /* (just a shorter name)     */
typedef struct SHRD_HDR   SHRD_HDR;     /* (just a shorter name)     */

#define SHRD_HDR_SIZE          sizeof( DBLWRD )
CASSERT( sizeof( SHRD_HDR ) == sizeof( DBLWRD ), shared_h );

/*-------------------------------------------------------------------*/

#define SHRD_SET_HDR( _buf, _cmd, _code, _devnum, _id, _len )        \
                                                                     \
    do                                                               \
    {                                                                \
        SHRD_HDR* shdr = (SHRD_HDR*)  (_buf);                        \
                                                                     \
        shdr->cmd  =                  (_cmd);                        \
        shdr->code =                  (_code);                       \
                                                                     \
        store_hw( &shdr->devnum, (U16)(_devnum) );                   \
        store_hw( &shdr->len,    (U16)(_len)    );                   \
        store_hw( &shdr->id,     (U16)(_id)     );                   \
    }                                                                \
    while (0)

/*-------------------------------------------------------------------*/

#define SHRD_GET_HDR( _buf, _cmd, _code, _devnum, _id, _len )        \
                                                                     \
    do                                                               \
    {                                                                \
        SHRD_HDR* shdr = (SHRD_HDR*) (_buf);                         \
                                                                     \
        (_cmd)    = shdr->cmd;                                       \
        (_code)   = shdr->code;                                      \
                                                                     \
        (_devnum) = (U16) fetch_hw( &shdr->devnum );                 \
        (_len)    = (int) fetch_hw( &shdr->len    );                 \
        (_id)     = (int) fetch_hw( &shdr->id     );                 \
    }                                                                \
    while (0)

/*-------------------------------------------------------------------*/

int    shared_update_notify (DEVBLK *dev, int block);
int    shared_ckd_init (DEVBLK *dev, int argc, char *argv[] );
int    shared_fba_init (DEVBLK *dev, int argc, char *argv[] );

SHR_DLL_IMPORT void* shared_server( void* arg );
SHR_DLL_IMPORT int   shrd_cmd( int argc, char* argv[], char* cmdline );
SHR_DLL_IMPORT void  shared_print_trace_table();

/*-------------------------------------------------------------------*/

#ifdef _SHARED_C_
static int     shared_ckd_close ( DEVBLK *dev );
static int     shared_fba_close (DEVBLK *dev);
static void    shared_start(DEVBLK *dev);
static void    shared_end (DEVBLK *dev);
static int     shared_ckd_read (DEVBLK *dev, int trk, BYTE *unitstat);
static int     shared_ckd_write (DEVBLK *dev, int trk, int off,
                      BYTE *buf, int len, BYTE *unitstat);
static int     shared_ckd_trklen (DEVBLK *dev, BYTE *buf);
static int     shared_used (DEVBLK *dev);
static void    shared_reserve (DEVBLK *dev);
static void    shared_release (DEVBLK *dev);
static int     clientWrite (DEVBLK *dev, int block);
static void    clientPurge (DEVBLK *dev, int n, void *buf);
static int     clientPurgescan (int *answer, int ix, int i, void *data);
static int     clientConnect (DEVBLK *dev, int retry);
static int     clientRequest (DEVBLK *dev, BYTE *buf, int len, int cmd,
                      int flags, int *code, int *status);
static int     clientSend (DEVBLK *dev, BYTE *hdr, BYTE *buf, int buflen);
static int     clientRecv (DEVBLK *dev, BYTE *hdr, BYTE *buf, int buflen);
static int     recvData(int sock, BYTE *hdr, BYTE *buf, int buflen, bool server_req);
static void    serverRequest (DEVBLK *dev, int ix, BYTE *hdr, BYTE *buf);
static int     serverLocate (DEVBLK *dev, int id, int *avail);
static int     serverId (DEVBLK *dev);
static int     serverError (DEVBLK *dev, int ix, int code, int status,
                      char *msg);
static int     serverSend (DEVBLK *dev, int ix, BYTE *hdr, BYTE *buf,
                      int buflen);
static bool    serverDisconnectable (DEVBLK *dev, int ix);
static void    serverDisconnect (DEVBLK *dev, int ix);
static char   *clientip (int sock);
static DEVBLK *findDevice (U16 devnum);
static void   *serverConnect (void *psock);
static void    shrdhdrtrc( DEVBLK* dev, const char* msg, const BYTE* hdr,
                          const char* msg2 );
static void    shrdtrc( DEVBLK* dev, const char* fmt, ... ) ATTR_PRINTF(2,3);
static void    shrdgentrc(           const char* fmt, ... ) ATTR_PRINTF(1,2);
static void    shrdtrclog_locked( const char* tracemsg );
static const char* shrdcmd2str( const BYTE cmd );
static void    shared_print_trace_table_locked();
#endif /* _SHARED_C_ */

/*-------------------------------------------------------------------*/

#define OBTAIN_SHRDLOCK()               obtain_lock(  &sysblk.shrdlock )
#define RELEASE_SHRDLOCK()              release_lock( &sysblk.shrdlock )

#define SHRDTRACE(    fmt, ... )        shrdtrc( dev, fmt, ## __VA_ARGS__ )
#define SHRDGENTRACE( fmt, ... )        shrdgentrc(   fmt, ## __VA_ARGS__ )

#define SHRDHDRTRACE(  msg, hdr )       shrdhdrtrc( dev, msg, hdr, 0    )
#define SHRDHDRTRACE2( msg, hdr, msg2 ) shrdhdrtrc( dev, msg, hdr, msg2 )

#define OBTAIN_SHRDTRACE_LOCK()         obtain_lock(  &sysblk.shrdtracelock )
#define RELEASE_SHRDTRACE_LOCK()        release_lock( &sysblk.shrdtracelock )

/*-------------------------------------------------------------------*/

#endif /* _SHARED_H */
