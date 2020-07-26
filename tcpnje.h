/* tcpnje.h - hacked from Ivan Warren's commadpt.h by Peter Coghlan */

#ifndef __TCPNJE_H__
#define __TCPNJE_H__

#define TCPNJE_DEFAULT_PORT      175  /* Standard port for TCPNJE / VMNET     */
#define TCPNJE_DEFAULT_BUFSIZE  8192  /* Default TCPNJE / VMNET buffer size   */
#define TCPNJE_DEFAULT_LISTEN      1  /* Default is to listen                 */
#define TCPNJE_DEFAULT_CONNECT     1  /* Default is to connect                */

#define TCPNJE_DEFAULT_DEBUG     127  /* Default debug bitmask                */
#define TCPNJE_DEFAULT_TRACE   65535  /* Default trace bitmask                */

#define TCPNJE_DEFAULT_KEEPALIVE 200  /* Default maximum idle writes (~10min) */

#define TCPNJE_MAX_ERRORCOUNT      5  /* When to start suppressing error msgs */

#include "hercules.h"

struct TTC                      /* TCPNJE (aka VMNET) control record        */
{
    BYTE type[8];
    BYTE rhost[8];
    U32  rip;
    BYTE ohost[8];
    U32  oip;
    BYTE r;
};

#define SIZEOF_TTC 33           /* sizeof(struct TTC) likely to be too big  */

struct TTB                      /* TCPNJE (aka VMNET) data block header     */
{
    BYTE flags;
    BYTE unused;
    U16  length;
    U32  reserved;
};

#define SIZEOF_TTB sizeof(struct TTB)

struct TTR                      /* TCPNJE (aka VMNET) record header         */
{
    BYTE flags;
    BYTE unused;
    U16  length;
};

#define SIZEOF_TTR sizeof(struct TTR)

struct TPB                      /* Skeleton RSCS TPbuffer                  */
{
    BYTE start[2];
    BYTE bcb;
    BYTE fcs[2];
    BYTE rcb;
    BYTE srcb;
    BYTE nccidl;                /* Initial / response signon record (I/J)  */
    BYTE nccinode[8];
    BYTE ncciqual;
    BYTE nccievnt[4];           /* U32 causes compiler to align this wrong */
    BYTE nccirest[2];           /* U16 messes this one up too.             */
    BYTE nccibfsz[2];           /* U16 this one too, the one I want :-(    */
    BYTE nccilpas[8];
    BYTE nccinpas[8];
    BYTE ncciflgs;
    BYTE nccifeat[4];
};

struct RECORD                   /* TCPNJE (aka VMNET) record data & header  */
{
    struct TTR ttr;
    struct TPB tpb;
};

struct BLOCK                    /* TCPNJE (aka VMNET) buffer                */
{
    struct TTB ttb;
    struct RECORD record;
};

struct TNBUFFER                 /* TCP/IP input or output transfer buffer   */
{
    union
    {
        BYTE *address;          /* Address of buffer as byte pointer        */
        struct TTC *ttc;        /* Address of TTC at start of buffer        */
        struct TTB *ttb;        /* Address ot TTB at start of buffer        */
    } base;                     /* Addresses of beginning of buffer         */
    union
    {
        BYTE *address;          /* Next byte to go into buffer              */
        struct BLOCK *block;    /* Addresses of TTB plus record going in    */
    } inptr;                    /* Addresses of data going into buffer      */
    union
    {
        BYTE *address;          /* Next byte to be taken from buffer        */
        struct BLOCK *block;    /* Addresses of TTB plus record coming out  */
    } outptr;                   /* Addresses of data coming out of buffer   */

    size_t size;                /* TCPNJE transfer buffer size              */
    BYTE valid;                 /* Flag indicating buffer contents valid    */
};

#define TCPNJE_VERSION "TCPNJE10" /* Version of struct TCPNJE               */

struct TCPNJE
{
    DEVBLK  *dev;               /* the devblk to which this dev is attached */
    BYTE    blockname[8];       /* Name/version of this device structure    */
    BYTE    dialdata[32];       /* Dial data information                    */
    BYTE    lnode[8];           /* Local NJE node name for TCPNJE           */
    BYTE    rnode[8];           /* Remote NJE node name for TCPNJE          */
    in_addr_t lhost;            /* Local listening address                  */
    in_addr_t rhost;            /* Remote connection IP address             */
    TID    thread;              /* Thread which performs socket I/O         */
    COND   ipc;                 /* I/O <-> thread IPC condition EVB         */
    COND   ipc_halt;            /* I/O <-> thread IPC HALT special EVB      */
    LOCK   lock;                /* TCPNJE lock to serialise socket access   */
    struct TNBUFFER ttcactbuf;  /* TTC structure buffer for active opens    */
    struct TNBUFFER ttcpasbuf;  /* TTC structure buffer for passive opens   */
    struct TNBUFFER tcpinbuf;   /* TTB/TTR input buffer structure           */
    struct TNBUFFER tcpoutbuf;  /* TTB/TTR output buffer structure          */
    S32    ackcount;            /* Outgoing buffers not yet ACKed count     */
    U32    inbuffcount;         /* Incoming TPbuffer count (statistics only)*/
    U32    inbytecount;         /* Incoming data count (statistics only)    */
    U32    outbuffcount;        /* Outgoing TPbuffer count (statistics only)*/
    U32    outbytecount;        /* Outgoing data count (statistics only)    */
    U32    idlewrites;          /* Idle write count for keepalive purposes  */
    U32    maxidlewrites;       /* Maximum number of idle writes allowed    */
    int    pipe[2];             /* pipe used for I/O to thread signaling    */
    int    timeout;             /* Current Timeout                          */
    int    activeopendelay;     /* Sort-of random outgoing connection delay */
    int    rto;                 /* Configured Read Time-Out                 */
    int    cto;                 /* Configured Connect Time-Out              */
    int    sfd;                 /* Communication socket FD                  */
    int    lfd;                 /* Listen socket for incoming connections   */
    int    afd;                 /* Socket FD for active TCPNJE OPEN         */
    int    pfd;                 /* Socket FD for passive TCPNJE OPEN        */
    int    trace;               /* Trace level bitmask                      */
    int    debug;               /* Debug level bitmask                      */
    enum   {
           CLOSED=0,            /* No connection of any sort open           */
           TCPLISTEN,           /* Passive listen in progress               */
           TCPCONSNT,           /* Outgoing connection attempt in progress  */
           TCPCONACT,           /* Outgoing TCP connection connected        */
           TCPCONPAS,           /* Incoming TCP connection connected        */
           NJEOPNSNT,           /* TCPNJE OPEN sent (usually NJE primary)   */
           NJEACKSNT,           /* TCPNJE ACK sent (usually NJE secondary)  */
           NJEACKRCD,           /* TCPNJE ACK received (usually NJE primary)*/
           NJECONPRI,           /* NJE connection achieved (NJE primary)    */
           NJECONSEC            /* NJE connection achieved (NJE secondary)  */
    }      state;               /* Connection state                         */
    U16    lport;               /* Local listening TCP port                 */
    U16    rport;               /* Remote TCP Port                          */
    U16    dialcount;           /* data count for dial                      */
    U16    tpbufsize;           /* TPbufsize negotiated by the two RSCSs    */
    BYTE   curpending;          /* Current pending operation                */
    BYTE   fastopen;            /* FASTOPEN flag specified for this stream  */
    BYTE   listening;           /* Listening attempted or in progress       */
    u_int  enabled:1;           /* An ENABLE CCW has been sucesfully issued */
    u_int  eibmode:1;           /* EIB Setmode issued                       */
    u_int  dialin:1;            /* This is a SWITCHED DIALIN line           */
    u_int  dialout:1;           /* This is a SWITCHED DIALOUT line          */
    u_int  have_thread:1;       /* the TCPNJE socket I/O thread is running  */
    u_int  dolisten:1;          /* Start a listen                           */
    u_int  haltpending:1;       /* Request issued to halt current CCW       */
    u_int  waitabit:1;          /* RSCS sent out FCS with wait-a-bit bit set*/
    u_int  holdincoming:1;      /* RSCS sent out FCS with a stream negated  */
    u_int  holdoutgoing:1;      /* Flag need to stop RSCS from sending data */
    u_int  resetoutbcb:1;       /* Reset outgoing BCB next time one goes out*/
    u_int  synnakreceived:1;    /* SYN NAK received on earlier read         */
    u_int  synnaksent:1;        /* SYN NAK sent on earlier write            */
    u_int  callissued:1;        /* The connect out for the DIAL/ENABLE      */
                                /* has already been issued                  */
    u_int  signoff:1;           /* Send signoff to RSCS at next read        */
    u_int  datalostcond:1;      /* Data Lost Condition Raised               */
    u_int  listen:1;            /* This is a listening device               */
    u_int  connect:1;           /* This is a connecting device              */
};

enum {
    TCPNJE_PEND_IDLE=0,         /* NO CCW currently executing               */
    TCPNJE_PEND_READ,           /* A READ CCW is running                    */
    TCPNJE_PEND_WRITE,          /* A WRITE CCW is running                   */
    TCPNJE_PEND_CONNECT,        /* An attempt to connect to the             */
                                /* link partner is in progress              */
    TCPNJE_PEND_DIAL,           /* A DIAL CCW is running                    */
    TCPNJE_PEND_DISABLE,        /* A DISABLE CCW is running                 */
    TCPNJE_PEND_PREPARE,        /* A PREPARE CCW is running                 */
    TCPNJE_PEND_WAIT,           /* A wait-a-bit is in progress              */
    TCPNJE_PEND_TINIT,          /* Worker thread initialisation             */
    TCPNJE_PEND_CLOSED,         /* Worker thread closing down               */
    TCPNJE_PEND_SHUTDOWN        /* Worker thread exiting                    */
} tcpnje_pendccw;

#define DECLARE_TCPNJE_PENDING static const char *tcpnje_pendccw_text[] = {\
    "IDLE",\
    "READ",\
    "WRITE",\
    "CONNECT",\
    "DIAL",\
    "DISABLE",\
    "PREPARE",\
    "WAIT",\
    "TINIT",\
    "CLOSED",\
    "SHUTDOWN"}

#define DECLARE_TCPNJE_STATE static const char *tcpnje_state_text[] = {\
    "CLOSED",\
    "TCPLISTEN",\
    "TCPCONSNT",\
    "TCPCONACT",\
    "TCPCONPAS",\
    "NJEOPNSNT",\
    "NJEACKSNT",\
    "NJEACKRCD",\
    "NJECONPRI",\
    "NJECONSEC"}

#define DECLARE_TCPNJE_OPEN static const BYTE TCPNJE_OPEN[] = \
    {0xd6, 0xd7, 0xc5, 0xd5, 0x40, 0x40, 0x40, 0x40}

#define DECLARE_TCPNJE_ACK  static const BYTE TCPNJE_ACK[]  = \
    {0xc1, 0xc3, 0xd2, 0x40, 0x40, 0x40, 0x40, 0x40}

#define DECLARE_TCPNJE_NAK  static const BYTE TCPNJE_NAK[]  = \
    {0xd5, 0xc1, 0xd2, 0x40, 0x40, 0x40, 0x40, 0x40}

#define SOH "\001"
#define STX "\002"
#define DLE "\020"
#define ETB "\046"
#define ENQ "\055"
#define SYN "\062"
#define NAK "\075"
#define ACK0 "\160"

#endif
