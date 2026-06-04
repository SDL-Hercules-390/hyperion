/* WSCNSL.C    (C) Copyright Hercules contributors, 2026             */
/*              WebSocket Console Bridge                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* RFC 6455 WebSocket transport for the existing console pipeline.   */
/*                                                                   */
/* Architecture: the WebSocket carries the same bytes the raw telnet */
/* socket would carry (websockify model). Per-connection state lives */
/* on the TELNET struct (is_websocket flag + ws_inbuf / ws_payload   */
/* buffers); the public API at the bottom of the file is the only    */
/* surface the rest of console.c interacts with.                     */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "hexterns.h"
#include "wscnsl.h"

/*-------------------------------------------------------------------*/
/*  Self-contained SHA-1 (Steve Reid, 100% Public Domain)            */
/*                                                                   */
/*  Vendored locally so the WS bridge stays free of external runtime */
/*  dependencies (the in-tree crypto/ library expects helper symbols */
/*  that only dyncrypt.so exports, which would couple module load    */
/*  order). This is the canonical 1995 reference implementation.    */
/*-------------------------------------------------------------------*/

#define WS_SHA1_DIGEST_LEN  20

typedef struct {
    U32   state[5];
    U64   count;        /* total bits processed */
    BYTE  buffer[64];
} ws_sha1_ctx;

#define WS_SHA1_ROL(v,b) (((v) << (b)) | ((v) >> (32 - (b))))

#define WS_SHA1_BLK0(i)                                                       \
    (block[i] = (((U32) buf[(i)*4    ]) << 24)                                \
              | (((U32) buf[(i)*4 + 1]) << 16)                                \
              | (((U32) buf[(i)*4 + 2]) <<  8)                                \
              |  ((U32) buf[(i)*4 + 3]))
#define WS_SHA1_BLK(i)                                                        \
    (block[i & 15] = WS_SHA1_ROL(block[(i+13) & 15] ^ block[(i+8) & 15] ^     \
                                 block[(i+ 2) & 15] ^ block[ i    & 15], 1))

#define WS_SHA1_R0(v,w,x,y,z,i)                                               \
    z += ((w & (x ^ y)) ^ y) + WS_SHA1_BLK0(i)  + 0x5A827999 + WS_SHA1_ROL(v,5); \
    w  = WS_SHA1_ROL(w, 30);
#define WS_SHA1_R1(v,w,x,y,z,i)                                               \
    z += ((w & (x ^ y)) ^ y) + WS_SHA1_BLK(i)   + 0x5A827999 + WS_SHA1_ROL(v,5); \
    w  = WS_SHA1_ROL(w, 30);
#define WS_SHA1_R2(v,w,x,y,z,i)                                               \
    z += (w ^ x ^ y)         + WS_SHA1_BLK(i)   + 0x6ED9EBA1 + WS_SHA1_ROL(v,5); \
    w  = WS_SHA1_ROL(w, 30);
#define WS_SHA1_R3(v,w,x,y,z,i)                                               \
    z += (((w | x) & y) | (w & x)) + WS_SHA1_BLK(i) + 0x8F1BBCDC + WS_SHA1_ROL(v,5); \
    w  = WS_SHA1_ROL(w, 30);
#define WS_SHA1_R4(v,w,x,y,z,i)                                               \
    z += (w ^ x ^ y)         + WS_SHA1_BLK(i)   + 0xCA62C1D6 + WS_SHA1_ROL(v,5); \
    w  = WS_SHA1_ROL(w, 30);

static void ws_sha1_transform( U32 state[5], const BYTE buf[64] )
{
    U32 a, b, c, d, e;
    U32 block[16];

    a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];

    WS_SHA1_R0(a,b,c,d,e, 0); WS_SHA1_R0(e,a,b,c,d, 1);
    WS_SHA1_R0(d,e,a,b,c, 2); WS_SHA1_R0(c,d,e,a,b, 3);
    WS_SHA1_R0(b,c,d,e,a, 4); WS_SHA1_R0(a,b,c,d,e, 5);
    WS_SHA1_R0(e,a,b,c,d, 6); WS_SHA1_R0(d,e,a,b,c, 7);
    WS_SHA1_R0(c,d,e,a,b, 8); WS_SHA1_R0(b,c,d,e,a, 9);
    WS_SHA1_R0(a,b,c,d,e,10); WS_SHA1_R0(e,a,b,c,d,11);
    WS_SHA1_R0(d,e,a,b,c,12); WS_SHA1_R0(c,d,e,a,b,13);
    WS_SHA1_R0(b,c,d,e,a,14); WS_SHA1_R0(a,b,c,d,e,15);
    WS_SHA1_R1(e,a,b,c,d,16); WS_SHA1_R1(d,e,a,b,c,17);
    WS_SHA1_R1(c,d,e,a,b,18); WS_SHA1_R1(b,c,d,e,a,19);
    WS_SHA1_R2(a,b,c,d,e,20); WS_SHA1_R2(e,a,b,c,d,21);
    WS_SHA1_R2(d,e,a,b,c,22); WS_SHA1_R2(c,d,e,a,b,23);
    WS_SHA1_R2(b,c,d,e,a,24); WS_SHA1_R2(a,b,c,d,e,25);
    WS_SHA1_R2(e,a,b,c,d,26); WS_SHA1_R2(d,e,a,b,c,27);
    WS_SHA1_R2(c,d,e,a,b,28); WS_SHA1_R2(b,c,d,e,a,29);
    WS_SHA1_R2(a,b,c,d,e,30); WS_SHA1_R2(e,a,b,c,d,31);
    WS_SHA1_R2(d,e,a,b,c,32); WS_SHA1_R2(c,d,e,a,b,33);
    WS_SHA1_R2(b,c,d,e,a,34); WS_SHA1_R2(a,b,c,d,e,35);
    WS_SHA1_R2(e,a,b,c,d,36); WS_SHA1_R2(d,e,a,b,c,37);
    WS_SHA1_R2(c,d,e,a,b,38); WS_SHA1_R2(b,c,d,e,a,39);
    WS_SHA1_R3(a,b,c,d,e,40); WS_SHA1_R3(e,a,b,c,d,41);
    WS_SHA1_R3(d,e,a,b,c,42); WS_SHA1_R3(c,d,e,a,b,43);
    WS_SHA1_R3(b,c,d,e,a,44); WS_SHA1_R3(a,b,c,d,e,45);
    WS_SHA1_R3(e,a,b,c,d,46); WS_SHA1_R3(d,e,a,b,c,47);
    WS_SHA1_R3(c,d,e,a,b,48); WS_SHA1_R3(b,c,d,e,a,49);
    WS_SHA1_R3(a,b,c,d,e,50); WS_SHA1_R3(e,a,b,c,d,51);
    WS_SHA1_R3(d,e,a,b,c,52); WS_SHA1_R3(c,d,e,a,b,53);
    WS_SHA1_R3(b,c,d,e,a,54); WS_SHA1_R3(a,b,c,d,e,55);
    WS_SHA1_R3(e,a,b,c,d,56); WS_SHA1_R3(d,e,a,b,c,57);
    WS_SHA1_R3(c,d,e,a,b,58); WS_SHA1_R3(b,c,d,e,a,59);
    WS_SHA1_R4(a,b,c,d,e,60); WS_SHA1_R4(e,a,b,c,d,61);
    WS_SHA1_R4(d,e,a,b,c,62); WS_SHA1_R4(c,d,e,a,b,63);
    WS_SHA1_R4(b,c,d,e,a,64); WS_SHA1_R4(a,b,c,d,e,65);
    WS_SHA1_R4(e,a,b,c,d,66); WS_SHA1_R4(d,e,a,b,c,67);
    WS_SHA1_R4(c,d,e,a,b,68); WS_SHA1_R4(b,c,d,e,a,69);
    WS_SHA1_R4(a,b,c,d,e,70); WS_SHA1_R4(e,a,b,c,d,71);
    WS_SHA1_R4(d,e,a,b,c,72); WS_SHA1_R4(c,d,e,a,b,73);
    WS_SHA1_R4(b,c,d,e,a,74); WS_SHA1_R4(a,b,c,d,e,75);
    WS_SHA1_R4(e,a,b,c,d,76); WS_SHA1_R4(d,e,a,b,c,77);
    WS_SHA1_R4(c,d,e,a,b,78); WS_SHA1_R4(b,c,d,e,a,79);

    state[0] += a; state[1] += b; state[2] += c;
    state[3] += d; state[4] += e;
}

static void ws_sha1_init( ws_sha1_ctx* ctx )
{
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count    = 0;
}

static void ws_sha1_update( ws_sha1_ctx* ctx, const BYTE* data, size_t len )
{
    size_t i = (size_t)((ctx->count >> 3) & 63);
    size_t j;

    ctx->count += (U64) len << 3;

    if (i + len >= 64)
    {
        j = 64 - i;
        memcpy( ctx->buffer + i, data, j );
        ws_sha1_transform( ctx->state, ctx->buffer );
        for (; j + 63 < len; j += 64)
            ws_sha1_transform( ctx->state, data + j );
        i = 0;
    }
    else
        j = 0;

    memcpy( ctx->buffer + i, data + j, len - j );
}

static void ws_sha1_final( BYTE digest[ WS_SHA1_DIGEST_LEN ], ws_sha1_ctx* ctx )
{
    BYTE finalcount[8];
    BYTE c;
    int  i;

    for (i = 0; i < 8; i++)
        finalcount[i] = (BYTE)((ctx->count >> ((7 - i) * 8)) & 0xFF);

    c = 0x80;
    ws_sha1_update( ctx, &c, 1 );
    c = 0x00;
    while ((ctx->count & 504) != 448)
        ws_sha1_update( ctx, &c, 1 );
    ws_sha1_update( ctx, finalcount, 8 );

    for (i = 0; i < WS_SHA1_DIGEST_LEN; i++)
        digest[i] = (BYTE)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);

    memset( ctx, 0, sizeof( *ctx ));
}

/*-------------------------------------------------------------------*/
/*                  Tweakable build constants                        */
/*-------------------------------------------------------------------*/
#define WS_HANDSHAKE_BUFLEN     8192        /* Max HTTP request size */
#define WS_INBUF_INIT           4096        /* Initial recv scratch  */
#define WS_INBUF_MAX            (1 << 20)   /* Hard cap (1 MiB)      */
#define WS_PAYLOAD_INIT         4096        /* Initial payload FIFO  */
#define WS_PAYLOAD_MAX          (1 << 20)   /* Hard cap (1 MiB)      */
#define WS_GUID                 "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* RFC 6455 opcodes */
#define WS_OP_CONT              0x0
#define WS_OP_TEXT              0x1
#define WS_OP_BINARY            0x2
#define WS_OP_CLOSE             0x8
#define WS_OP_PING              0x9
#define WS_OP_PONG              0xA

/*-------------------------------------------------------------------*/
/*                      Base64 (encode-only)                         */
/*-------------------------------------------------------------------*/
static const char b64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Encode 'len' bytes into 'out' (caller-provided, must be >= 4*((len+2)/3)+1
   bytes). Always null-terminates. Returns number of chars written
   (excluding the null). */
static size_t b64_encode( const BYTE* in, size_t len, char* out )
{
    size_t i = 0, o = 0;
    while (i + 3 <= len)
    {
        U32 v = ((U32)in[i] << 16) | ((U32)in[i+1] << 8) | (U32)in[i+2];
        out[o++] = b64_alphabet[(v >> 18) & 0x3F];
        out[o++] = b64_alphabet[(v >> 12) & 0x3F];
        out[o++] = b64_alphabet[(v >>  6) & 0x3F];
        out[o++] = b64_alphabet[ v        & 0x3F];
        i += 3;
    }
    if (i < len)
    {
        U32 v = (U32)in[i] << 16;
        if (i + 1 < len) v |= (U32)in[i+1] << 8;
        out[o++] = b64_alphabet[(v >> 18) & 0x3F];
        out[o++] = b64_alphabet[(v >> 12) & 0x3F];
        out[o++] = (i + 1 < len) ? b64_alphabet[(v >> 6) & 0x3F] : '=';
        out[o++] = '=';
    }
    out[o] = '\0';
    return o;
}

/*-------------------------------------------------------------------*/
/*       Read HTTP request headers, terminating on CRLFCRLF          */
/*-------------------------------------------------------------------*/
/* Returns total bytes read on success, -1 on error. Buffer is null- */
/* terminated. The caller's request arrives in one TCP burst from    */
/* every browser/curl/wscat I've seen, but we still loop to be safe. */
/*-------------------------------------------------------------------*/
static int read_http_headers( int csock, char* buf, size_t bufsize )
{
    size_t total = 0;
    int rc;

    while (total + 1 < bufsize)
    {
        rc = recv( csock, buf + total, (int)(bufsize - 1 - total), 0 );
        if (rc <= 0)
            return -1;

        total += rc;
        buf[total] = '\0';

        /* Look for end-of-headers marker */
        if (total >= 4 && strstr( buf, "\r\n\r\n" ))
            return (int) total;
    }
    /* Headers larger than our buffer -- bail */
    return -1;
}

/*-------------------------------------------------------------------*/
/*  Locate a header value (case-insensitive name match, RFC 7230)    */
/*-------------------------------------------------------------------*/
/* Returns malloc'd, trimmed value string or NULL if not found.      */
/*-------------------------------------------------------------------*/
static char* find_header_value( const char* headers, const char* name )
{
    size_t namelen = strlen( name );
    const char* p = headers;
    const char* eol;
    const char* val;
    const char* end;
    char* out;
    size_t vallen;

    /* Skip request line */
    eol = strstr( p, "\r\n" );
    if (!eol)
        return NULL;
    p = eol + 2;

    while (*p && p[0] != '\r')
    {
        eol = strstr( p, "\r\n" );
        if (!eol)
            return NULL;

        if ((size_t)(eol - p) > namelen
            && strncasecmp( p, name, namelen ) == 0
            && p[namelen] == ':')
        {
            val = p + namelen + 1;
            while (val < eol && (*val == ' ' || *val == '\t'))
                val++;
            end = eol;
            while (end > val && (end[-1] == ' ' || end[-1] == '\t'))
                end--;

            vallen = (size_t)(end - val);
            out = (char*) malloc( vallen + 1 );
            if (!out) return NULL;
            memcpy( out, val, vallen );
            out[vallen] = '\0';
            return out;
        }
        p = eol + 2;
    }
    return NULL;
}

/*-------------------------------------------------------------------*/
/*           Compute Sec-WebSocket-Accept response value             */
/*-------------------------------------------------------------------*/
static void compute_accept_key( const char* client_key, char* accept_out )
{
    ws_sha1_ctx ctx;
    BYTE digest[ WS_SHA1_DIGEST_LEN ];

    ws_sha1_init( &ctx );
    ws_sha1_update( &ctx, (const BYTE*) client_key, strlen( client_key ));
    ws_sha1_update( &ctx, (const BYTE*) WS_GUID,    sizeof( WS_GUID ) - 1 );
    ws_sha1_final( digest, &ctx );

    b64_encode( digest, WS_SHA1_DIGEST_LEN, accept_out );
}

/*-------------------------------------------------------------------*/
/*       Send a small HTTP error response and close the socket       */
/*-------------------------------------------------------------------*/
static void send_http_error( int csock, const char* status_line )
{
    char resp[256];
    int n = snprintf( resp, sizeof( resp ),
        "HTTP/1.1 %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n", status_line );
    if (n > 0)
        write_socket( csock, resp, n );
}

/*-------------------------------------------------------------------*/
/*                  ws_handshake (public API)                        */
/*-------------------------------------------------------------------*/
int ws_handshake( int csock )
{
    char    headers [ WS_HANDSHAKE_BUFLEN ];
    char    accept_key [ 64 ];
    char    response   [ 512 ];
    char*   key   = NULL;
    char*   upgrade = NULL;
    int     rc;
    int     n;

    rc = read_http_headers( csock, headers, sizeof( headers ));
    if (rc < 0)
    {
        // "%s COMM: WebSocket handshake failed: %s"
        WRMSG( HHC02918, "E", "client", "could not read HTTP request" );
        return -1;
    }

    /* Sanity: must be a GET request */
    if (strncmp( headers, "GET ", 4 ) != 0)
    {
        send_http_error( csock, "405 Method Not Allowed" );
        return -1;
    }

    /* Must have "Upgrade: websocket" header */
    upgrade = find_header_value( headers, "Upgrade" );
    if (!upgrade || strcasecmp( upgrade, "websocket" ) != 0)
    {
        free( upgrade );
        send_http_error( csock, "400 Bad Request" );
        return -1;
    }
    free( upgrade );

    /* Sec-WebSocket-Key is mandatory */
    key = find_header_value( headers, "Sec-WebSocket-Key" );
    if (!key || strlen( key ) == 0)
    {
        free( key );
        send_http_error( csock, "400 Bad Request" );
        return -1;
    }

    compute_accept_key( key, accept_key );
    free( key );

    n = snprintf( response, sizeof( response ),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key );

    if (n <= 0 || n >= (int) sizeof( response ))
        return -1;

    if (write_socket( csock, response, n ) != n)
        return -1;

    return 0;
}

/*-------------------------------------------------------------------*/
/*                Buffer growth helpers (FIFO byte queues)           */
/*-------------------------------------------------------------------*/
/* Ensure buf has room for 'extra' more bytes (after the existing    */
/* 'used' bytes). Returns 0 on success, -1 on cap-exceeded.          */
/*-------------------------------------------------------------------*/
static int ensure_buf( BYTE** buf, size_t* size, size_t used,
                       size_t extra, size_t cap )
{
    size_t need = used + extra;
    size_t newsz;

    if (need <= *size)
        return 0;
    if (need > cap)
        return -1;

    newsz = *size ? *size : 64;
    while (newsz < need)
        newsz <<= 1;
    if (newsz > cap)
        newsz = cap;

    {
        BYTE* p = (BYTE*) realloc( *buf, newsz );
        if (!p) return -1;
        *buf  = p;
        *size = newsz;
    }
    return 0;
}

/* Drop the first 'n' bytes from a FIFO byte queue. */
static void drop_prefix( BYTE* buf, size_t* used, size_t n )
{
    if (n >= *used)
    {
        *used = 0;
        return;
    }
    memmove( buf, buf + n, *used - n );
    *used -= n;
}

/*-------------------------------------------------------------------*/
/*               Build and send a single WS frame                    */
/*-------------------------------------------------------------------*/
/* Server frames are unmasked. Returns 0 on success, -1 on error.    */
/*-------------------------------------------------------------------*/
static int ws_send_frame( int csock, BYTE opcode,
                          const void* payload, size_t len )
{
    BYTE   hdr[ 14 ];           /* max header size (no mask) */
    size_t hdrlen = 0;
    BYTE*  out;
    int    rc;
    int    total;

    hdr[0] = 0x80 | (opcode & 0x0F);    /* FIN=1 */

    if (len < 126)
    {
        hdr[1] = (BYTE) len;
        hdrlen = 2;
    }
    else if (len < 65536)
    {
        hdr[1] = 126;
        hdr[2] = (BYTE)((len >> 8) & 0xFF);
        hdr[3] = (BYTE)( len       & 0xFF);
        hdrlen = 4;
    }
    else
    {
        U64 l64 = (U64) len;
        hdr[1]  = 127;
        hdr[2]  = (BYTE)((l64 >> 56) & 0xFF);
        hdr[3]  = (BYTE)((l64 >> 48) & 0xFF);
        hdr[4]  = (BYTE)((l64 >> 40) & 0xFF);
        hdr[5]  = (BYTE)((l64 >> 32) & 0xFF);
        hdr[6]  = (BYTE)((l64 >> 24) & 0xFF);
        hdr[7]  = (BYTE)((l64 >> 16) & 0xFF);
        hdr[8]  = (BYTE)((l64 >>  8) & 0xFF);
        hdr[9]  = (BYTE)( l64        & 0xFF);
        hdrlen = 10;
    }

    /* One contiguous write avoids the risk of a partial-frame on the
       wire if the second write_socket happened to fail mid-stream. */
    total = (int)(hdrlen + len);
    out = (BYTE*) malloc( total );
    if (!out)
        return -1;
    memcpy( out, hdr, hdrlen );
    if (len)
        memcpy( out + hdrlen, payload, len );

    rc = write_socket( csock, out, total );
    free( out );

    return (rc == total) ? 0 : -1;
}

/*-------------------------------------------------------------------*/
/*  Try to parse one frame off tn->ws_inbuf into tn->ws_payload      */
/*-------------------------------------------------------------------*/
/* Returns: 1 = a frame was consumed (more may remain);              */
/*          0 = need more bytes;                                     */
/*         -1 = protocol error / connection should close.            */
/*-------------------------------------------------------------------*/
static int parse_one_frame( TELNET* tn )
{
    BYTE*  in    = tn->ws_inbuf;
    size_t avail = tn->ws_inbuf_used;
    BYTE   b0, b1, opcode;
    int    masked;
    U64    payload_len;
    size_t hdrlen;
    BYTE   mask[4];
    size_t i;

    if (avail < 2)
        return 0;

    b0      = in[0];
    b1      = in[1];
    opcode  = b0 & 0x0F;
    masked  = (b1 & 0x80) ? 1 : 0;
    payload_len = b1 & 0x7F;
    hdrlen  = 2;

    if (payload_len == 126)
    {
        if (avail < 4) return 0;
        payload_len = ((U64) in[2] << 8) | (U64) in[3];
        hdrlen = 4;
    }
    else if (payload_len == 127)
    {
        if (avail < 10) return 0;
        payload_len =
              ((U64) in[2] << 56)
            | ((U64) in[3] << 48)
            | ((U64) in[4] << 40)
            | ((U64) in[5] << 32)
            | ((U64) in[6] << 24)
            | ((U64) in[7] << 16)
            | ((U64) in[8] <<  8)
            | ((U64) in[9]      );
        hdrlen = 10;
    }

    /* RFC 6455: client→server frames MUST be masked */
    if (!masked)
        return -1;

    if (avail < hdrlen + 4)
        return 0;
    memcpy( mask, in + hdrlen, 4 );
    hdrlen += 4;

    /* Reject payloads that don't fit our cap */
    if (payload_len > (U64) WS_PAYLOAD_MAX)
        return -1;

    if (avail < hdrlen + payload_len)
        return 0;

    /* Full frame is here. Process by opcode. */
    switch (opcode)
    {
        case WS_OP_CONT:
        case WS_OP_TEXT:
        case WS_OP_BINARY:
        {
            if (ensure_buf( &tn->ws_payload, &tn->ws_payload_size,
                            tn->ws_payload_used, (size_t) payload_len,
                            WS_PAYLOAD_MAX ) != 0)
                return -1;

            /* Unmask payload directly into ws_payload tail */
            for (i = 0; i < (size_t) payload_len; i++)
            {
                tn->ws_payload[ tn->ws_payload_used + i ] =
                    in[ hdrlen + i ] ^ mask[ i & 3 ];
            }
            tn->ws_payload_used += (size_t) payload_len;
            break;
        }

        case WS_OP_PING:
        {
            /* Echo back as PONG with same (unmasked) payload */
            BYTE small[125];
            BYTE* tmp = NULL;
            BYTE* payload = NULL;
            int   rc;

            if (payload_len <= sizeof( small ))
                payload = small;
            else
            {
                tmp = (BYTE*) malloc( (size_t) payload_len );
                if (!tmp) return -1;
                payload = tmp;
            }
            for (i = 0; i < (size_t) payload_len; i++)
                payload[i] = in[ hdrlen + i ] ^ mask[ i & 3 ];

            rc = ws_send_frame( tn->csock, WS_OP_PONG,
                                payload, (size_t) payload_len );
            if (tmp) free( tmp );
            if (rc < 0) return -1;
            break;
        }

        case WS_OP_PONG:
            /* Unsolicited pong: discard */
            break;

        case WS_OP_CLOSE:
            /* Reply with close (echo the same code if present) and
               signal end-of-stream to the caller. */
            if (!tn->ws_close_sent)
            {
                BYTE close_pl[125];
                size_t close_len = 0;
                if (payload_len >= 2 && payload_len <= sizeof( close_pl ))
                {
                    for (i = 0; i < (size_t) payload_len; i++)
                        close_pl[i] = in[ hdrlen + i ] ^ mask[ i & 3 ];
                    close_len = (size_t) payload_len;
                }
                ws_send_frame( tn->csock, WS_OP_CLOSE, close_pl, close_len );
                tn->ws_close_sent = 1;
            }
            return -1;          /* Treat as connection closed */

        default:
            /* Reserved / unknown opcode */
            return -1;
    }

    /* Drop this frame's bytes from the input buffer */
    drop_prefix( tn->ws_inbuf, &tn->ws_inbuf_used, hdrlen + (size_t) payload_len );
    return 1;
}

/*-------------------------------------------------------------------*/
/* Drain as many complete frames as currently sit in ws_inbuf.       */
/* Returns 0 on success, -1 on protocol error (caller should close). */
/*-------------------------------------------------------------------*/
static int parse_all_frames( TELNET* tn )
{
    int rc;
    do {
        rc = parse_one_frame( tn );
        if (rc < 0) return -1;
    } while (rc == 1);
    return 0;
}

/*-------------------------------------------------------------------*/
/*                  ws_recv (public API)                             */
/*-------------------------------------------------------------------*/
int ws_recv( TELNET* tn, void* buf, int nbytes )
{
    int   rc;
    int   take;
    BYTE  scratch[ 4096 ];

    /* Plain socket: pass through */
    if (!tn || !tn->is_websocket)
        return recv( tn ? tn->csock : -1, buf, nbytes, 0 );

    for (;;)
    {
        /* If we have buffered payload, deliver it first */
        if (tn->ws_payload_used > 0)
        {
            take = (int) tn->ws_payload_used;
            if (take > nbytes) take = nbytes;
            memcpy( buf, tn->ws_payload, take );
            drop_prefix( tn->ws_payload, &tn->ws_payload_used, take );
            return take;
        }

        /* Otherwise, pull more bytes from the socket */
        rc = recv( tn->csock, scratch, (int) sizeof( scratch ), 0 );
        if (rc < 0)
            return -1;          /* errno preserved (EAGAIN, etc.)    */
        if (rc == 0)
            return 0;           /* peer closed the connection        */

        if (ensure_buf( &tn->ws_inbuf, &tn->ws_inbuf_size,
                        tn->ws_inbuf_used, (size_t) rc,
                        WS_INBUF_MAX ) != 0)
            return -1;
        memcpy( tn->ws_inbuf + tn->ws_inbuf_used, scratch, (size_t) rc );
        tn->ws_inbuf_used += (size_t) rc;

        if (parse_all_frames( tn ) < 0)
            return -1;
        /* loop: if the new bytes produced payload, next iteration
           returns it; otherwise we recv() again. */
    }
}

/*-------------------------------------------------------------------*/
/*                  ws_send (public API)                             */
/*-------------------------------------------------------------------*/
int ws_send( TELNET* tn, const void* buf, int nbytes )
{
    if (!tn)
        return -1;
    if (!tn->is_websocket)
        return write_socket( tn->csock, buf, nbytes );

    if (nbytes <= 0)
        return 0;

    if (ws_send_frame( tn->csock, WS_OP_BINARY, buf, (size_t) nbytes ) < 0)
        return -1;
    return nbytes;
}

/*-------------------------------------------------------------------*/
/*                  ws_free_state (public API)                       */
/*-------------------------------------------------------------------*/
void ws_free_state( TELNET* tn )
{
    if (!tn || !tn->is_websocket)
        return;

    if (!tn->ws_close_sent)
    {
        /* Best-effort close; ignore failures (peer may already be gone) */
        ws_send_frame( tn->csock, WS_OP_CLOSE, NULL, 0 );
        tn->ws_close_sent = 1;
    }

    if (tn->ws_inbuf)   free( tn->ws_inbuf );
    if (tn->ws_payload) free( tn->ws_payload );
    tn->ws_inbuf        = NULL;
    tn->ws_payload      = NULL;
    tn->ws_inbuf_size   = 0;
    tn->ws_inbuf_used   = 0;
    tn->ws_payload_size = 0;
    tn->ws_payload_used = 0;
    tn->is_websocket    = 0;
}
