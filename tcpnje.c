/*-------------------------------------------------------------------*/
/* Hercules TCPNJE / Bisync Line Driver based on commadpt:           */
/*-------------------------------------------------------------------*/
/* Hercules Communication Line Driver                                */
/* (c) 1999-2006 Roger Bowler & Others                               */
/* Use of this program is governed by the QPL License                */
/* Original Author : Ivan Warren                                     */
/*-------------------------------------------------------------------*/
/* To a Hercules guest OS, this code impelements a subset of a 2703  */
/* BISYNC communications adaptor.  To the outside world, it appears  */
/* as a system which implements (most of) the TCPNJE protocol, also  */
/* known as VMNET protocol described here:                           */
/*                                                                   */
/*   http://www.nic.funet.fi/pub/netinfo/CREN/brfc0002.text          */
/*                                                                   */
/* This code implements in Hercules the functions provided by the    */
/* VMNET virtual machine described in the link above.  This allows a */
/* modified version of the RSCS which ships with VM/370 running on   */
/* Hercules to implement NJE links over TCP/IP to other similarly    */
/* equipped VM/370 on Hercules installations.  Because the TCP/IP    */
/* interface is implemented within Hercules, there is no requirement */
/* for TCP/IP capability on VM/370.                                  */
/*                                                                   */
/* Original Author of commadpt: Ivan Warren                          */
/* TCPNJE hacked from commadpt by Peter Coghlan and integrated into  */
/* SDL Hercules Hyperion by Ivan Warren.                             */
/*                                                                   */
/* PLEASE DO NOT CONTACT PETER COGHLAN FOR SUPPORT! This is a SDL    */
/* Hercules Hyperion implementation that is NOT supported by Peter!  */
/* If you need support or wish to report a bug, please do so via     */
/* the official SDL Hercules Hyperions GitHub Issues web page at:    */
/*                                                                   */
/*    https://github.com/SDL-Hercules-390/hyperion/issues            */
/*                                                                   */
/* For more information regarding this implementation please refer   */
/* to our README.TCPNJE.md document.                                 */
/*-------------------------------------------------------------------*/
/* TCPNJE version 1.00                                               */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define HYPERION_DEVHND_FORMAT
#define OPTION_DYNAMIC_LOAD

#include "hercules.h"
#include "devtype.h"
#include "parser.h"
#include "tcpnje.h"

#if defined(WIN32) && defined(OPTION_DYNAMIC_LOAD) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)
  SYSBLK *psysblk;
  #define sysblk (*psysblk)
#endif

 /*-------------------------------------------------------------------*/
 /* Ivan Warren 20040227                                              */
 /* This table is used by channel.c to determine if a CCW code is an  */
 /* immediate command or not                                          */
 /* The tape is addressed in the DEVHND structure as 'DEVIMM immed'   */
 /* 0 : Command is NOT an immediate command                           */
 /* 1 : Command is an immediate command                               */
 /* Note : An immediate command is defined as a command which returns */
 /* CE (channel end) during initialisation (that is, no data is       */
 /* actually transfered. In this case, IL is not indicated for a CCW  */
 /* Format 0 or for a CCW Format 1 when IL Suppression Mode is in     */
 /* effect                                                            */
 /*-------------------------------------------------------------------*/

static BYTE tcpnje_immed_command[256] =
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

DECLARE_TCPNJE_PENDING; /* Declares tcpnje_pendccw_text array    */

DECLARE_TCPNJE_STATE;   /* Declares tcpnje_state_text array      */

DECLARE_TCPNJE_OPEN;    /* Declares TCPNJE_OPEN array            */

DECLARE_TCPNJE_ACK;     /* Declares TCPNJE_ACK array             */

DECLARE_TCPNJE_NAK;     /* Declares TCPNJE_NAK array             */

#define DBGMSG(_level, ...) if ((tn->dev->ccwtrace && (_level & tn->trace)) || (_level & tn->debug)) logmsg(__VA_ARGS__)

/*---------------------------------------------------------------*/
/* PARSER TABLES                                                 */
/*---------------------------------------------------------------*/
static PARSER ptab[] = {
    {"lport", "%s"},
    {"lhost", "%s"},
    {"rport", "%s"},
    {"rhost", "%s"},
    {"dial", "%s"},
    {"rto", "%s"},
    {"cto", "%s"},
    {"keepalive", "%s"},
    {"switched", "%s"},
    {"lnode", "%s"},
    {"rnode", "%s"},
    {"debug", "%s"},
    {"trace", "%s"},
    {"bufsize", "%s"},
    {"listen", "%s"},
    {"connect", "%s"},
    {NULL, NULL}
};

enum {
    TCPNJE_KW_LPORT = 1,
    TCPNJE_KW_LHOST,
    TCPNJE_KW_RPORT,
    TCPNJE_KW_RHOST,
    TCPNJE_KW_DIAL,
    TCPNJE_KW_READTO,
    TCPNJE_KW_CONNECTTO,
    TCPNJE_KW_KEEPALIVE,
    TCPNJE_KW_SWITCHED,
    TCPNJE_KW_LNODE,
    TCPNJE_KW_RNODE,
    TCPNJE_KW_DEBUG,
    TCPNJE_KW_TRACE,
    TCPNJE_KW_BUFSIZE,
    TCPNJE_KW_LISTEN,
    TCPNJE_KW_CONNECT
} tcpnje_kw;

static void logdump(char *txt, DEVBLK *dev, BYTE *bfr, size_t sz)
{
    struct TCPNJE *tn;
    size_t i, j;
    BYTE character;

    tn = (struct TCPNJE *) dev->commadpt;

    if (!(dev->ccwtrace && (tn->trace & 8192)) && !(tn->debug & 8192))
    {
        return;
    }

    logmsg("HHCTN101D %4.4X:%s\n",
            dev->devnum, txt);
    logmsg("HHCTN102D %4.4X:%s : Dump of %d (%x) byte(s)\n",
            dev->devnum, txt, (int)sz, (int)sz);
    for(i = 0; i < sz; i += 16)
    {
        logmsg("HHCTN103D %4.4X:%s : %4.4X:",
                dev->devnum, txt, (unsigned int)i);
        for(j = 0; (j < 16) && ((i + j) < sz); j++)
        {
            if ((j % 4) == 0)
            {
                logmsg(" ");
            }
            logmsg("%2.2X", bfr[i + j]);
        }
        for(; j < 17; j++)
        {
            if ((j % 4) == 0)
            {
                logmsg(" ");
            }
            logmsg("  ");
        }
        for(j = 0; (j < 16) && ((i + j) < sz); j++)
        {
            character = guest_to_host(bfr[i + j]);
            if (!isprint((unsigned char)character)) character = '.';
            logmsg("%c", character);
        }
        logmsg("\n");
    }
}
/*-------------------------------------------------------------------*/
/* Handler utility routines                                          */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Make ebcdic names etc displayable.  Drop trailing blanks.         */
/*-------------------------------------------------------------------*/
static char *guest_to_host_string(char *string, size_t length, const BYTE *ebcdic)
{
    u_int i;

    for (i = 0; i < length - 1; i++)
    {
        string[i] = guest_to_host(ebcdic[i]);

        if (string[i] == ' ')
            string[i] = '\0';
        else if (!isprint((unsigned char)string[i]))
            string[i] = '.';
    }

    string[length - 1] = '\0';

    return string;
}
/*-------------------------------------------------------------------*/
/* Free all private structures and buffers                           */
/*-------------------------------------------------------------------*/
static void tcpnje_clean_device(DEVBLK *dev)
{
    struct TCPNJE *tn;

    if (!dev)
    {
        /*
         * Shouldn't happen.. But during shutdown, some weird
         * things happen !
         */
        return;
    }

    tn = (struct TCPNJE *) dev->commadpt;

    if (tn != NULL)
    {
        if (tn->ttcpasbuf.base.address != NULL)
        {
            free(tn->ttcpasbuf.base.address);
            tn->ttcpasbuf.base.address = NULL;
        }
        if (tn->ttcactbuf.base.address != NULL)
        {
            free(tn->ttcactbuf.base.address);
            tn->ttcactbuf.base.address = NULL;
        }
        if (tn->tcpinbuf.base.address != NULL)
        {
            free(tn->tcpinbuf.base.address);
            tn->tcpinbuf.base.address = NULL;
        }
        if (tn->tcpoutbuf.base.address != NULL)
        {
            free(tn->tcpoutbuf.base.address);
            tn->tcpoutbuf.base.address = NULL;
        }
        /* release the TCPNJE lock */
        release_lock(&tn->lock);

        free(tn);

        dev->commadpt = NULL;

        if (dev->ccwtrace)
        {
            logmsg("HHCTN104D %4.4X:TCPNJE - control block freed\n",
                    dev->devnum);
        }
    }
    else
    {
        logmsg("HHCTN023E %4.4X:TCPNJE - control block not freed : not allocated\n",
                dev->devnum);
    }
    return;
}

/*-------------------------------------------------------------------*/
/* Allocate initial private structures                               */
/*-------------------------------------------------------------------*/
static int tcpnje_alloc_device(DEVBLK *dev)
{
    struct TCPNJE *tn;

    dev->commadpt = malloc(sizeof(struct TCPNJE));
    if (dev->commadpt == NULL)
    {
        logmsg("HHCTN020E %4.4X:TCPNJE - memory allocation failure for main control block\n",
                dev->devnum);
        return -1;
    }

    tn = (struct TCPNJE *) dev->commadpt;

    memset(tn, 0, sizeof(struct TCPNJE));

    memcpy(tn->blockname, TCPNJE_VERSION, sizeof(tn->blockname));

    tn->dev = dev;

    return 0;
}
/*-------------------------------------------------------------------*/
/* Parsing utilities                                                 */
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/* tcpnje_getport : returns a port number or -1                      */
/*-------------------------------------------------------------------*/
static int tcpnje_getport(char *txt)
{
    int pno;
    struct servent *se;
    pno = atoi(txt);
    if (pno == 0)
    {
        se = getservbyname(txt, "tcp");
        if (se == NULL)
        {
            return -1;
        }
        pno = se->s_port;
    }
    return(pno);
}
/*-------------------------------------------------------------------*/
/* tcpnje_getaddr : set an in_addr_t if ok, else return -1           */
/*-------------------------------------------------------------------*/
static int tcpnje_getaddr(in_addr_t *ia, char *txt)
{
    struct hostent *he;
    struct in_addr in;

    if (inet_aton(txt, &in))
    {
        memcpy(ia, &in.s_addr, sizeof(in.s_addr));
    }
    else
    {
        he = gethostbyname(txt);
        if (he == NULL)
        {
            return(-1);
        }
        memcpy(ia, he->h_addr, he->h_length);
    }
    return(0);
}
/*-------------------------------------------------------------------*/
/* tcpnje_listen : Start listening for incoming connections          */
/* return values :  0 -> Normal completion.                          */
/*                 -1 -> socket() failed.                            */
/*                 -2 -> socket_is_socket() failed.                  */
/*                 -3 -> socket_set_blocking_mode() failed.          */
/*                 -4 -> bind() failed (address/port in use).        */
/*                 -5 -> bind() failed (no access to privileged port)*/
/*                 -6 -> bind() failed (some other reason).          */
/*                 -7 -> listen() failed.                            */
/*                 >0 -> nothing done due to listen parm being zero  */
/*-------------------------------------------------------------------*/
static int tcpnje_listen(struct TCPNJE *tn)
{
    struct sockaddr_in sin;     /* bind socket address structure     */
    struct in_addr intmp;       /* to print ip address in error msgs */
    int savederrno;
    int sockopt;                /* Used for setsocketoption          */
    int rc;                     /* return code from various rtns     */

    if (!tn->listen) return(999);

    intmp.s_addr = tn->lhost;   /* To display ip addresses in msgs   */

    /* Indicate that we are at least pretending to listen */
    tn->listening = 1;
    if (tn->state < TCPLISTEN) tn->state = TCPLISTEN;

    /* Create the socket for a listen */
    tn->lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tn->lfd < 0)
    {
        DBGMSG(4, "HHCTN003E %4.4X:TCPNJE - cannot obtain socket for incoming calls : %s\n",
                tn->dev->devnum, strerror(HSO_errno));
        return -1;
    }

    if (!socket_is_socket(tn->lfd))
    {
        DBGMSG(4, "HHCTN028E %4.4X:TCPNJE - cannot use socket obtained for incoming calls : %s\n",
                tn->dev->devnum, strerror(HSO_errno));
        close_socket(tn->lfd);
        tn->lfd = -1;
        return -2;
    }

    /* Turn blocking I/O off */
    /* set socket to NON-blocking mode */
    rc = socket_set_blocking_mode(tn->lfd, 0);
    if (rc < 0)
    {
        DBGMSG(4, "HHCTN029E %4.4X:TCPNJE - error setting socket for incoming calls to non-blocking : %s\n",
                tn->dev->devnum, strerror(HSO_errno));
        close_socket(tn->lfd);
        tn->lfd = -1;
        return -3;
    }

    /* Reuse the address regardless of any */
    /* spurious connection on that port    */
    sockopt = 1;
    rc = setsockopt(tn->lfd, SOL_SOCKET, SO_REUSEADDR, (GETSET_SOCKOPT_T*)&sockopt, sizeof(sockopt));
    if (rc < 0)
    {
        DBGMSG(4, "HHCTN030W %4.4X:TCPNJE - unable to set SO_REUSEADDR option on listening socket: %s\n",
                tn->dev->devnum, strerror(HSO_errno));
    }

    /* Bind the socket */
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = tn->lhost;
    sin.sin_port = htons(tn->lport);

    /* Caller may want to listen on a privileged port such as 175, the standard TCPNJE/VMNET port */
    if (tn->lport < 1024) SETMODE(ROOT);

    rc = bind(tn->lfd, (struct sockaddr *)&sin, sizeof(sin));
    savederrno = HSO_errno;

    if (tn->lport < 1024) SETMODE(USER);

    if (rc < 0)
    {
        if (savederrno == HSO_EADDRINUSE)
        {
            DBGMSG(32, "HHCTN004W %4.4X:TCPNJE - listener: address/port combination %s:%d currently in use\n",
                        tn->dev->devnum, inet_ntoa(intmp), tn->lport);
            close_socket(tn->lfd);
            tn->lfd = -1;
            return -4;
        }
        else if
#if defined(__VMS)
                (savederrno == EPERM)
#else /* __VMS */
                (savederrno == HSO_EACCES)
#endif /* __VMS */
        {
            DBGMSG(32, "HHCTN031W %4.4X:TCPNJE - no permission to bind privileged port %d for listen\n",
                        tn->dev->devnum, tn->lport);
            close_socket(tn->lfd);
            tn->lfd = -1;
            return -5;
        }
        else
        {
            DBGMSG(4, "HHCTN018W %4.4X:TCPNJE - bind for incoming connections to %s:%d failed: %s\n",
                         tn->dev->devnum, inet_ntoa(intmp), tn->lport, strerror(savederrno));
            close_socket(tn->lfd);
            tn->lfd = -1;
            return -6;
        }
    }

    /* Start the listen */
    if (listen(tn->lfd, 10))
    {
        DBGMSG(4, "HHCTN032W %4.4X:TCPNJE - listen on %d:%s for incoming TCP connections failed: %s\n",
                    tn->dev->devnum, tn->lport, inet_ntoa(intmp), strerror(HSO_errno));
        close_socket(tn->lfd);
        tn->lfd = -1;
        rc = -7;
    }

    DBGMSG(128, "HHCTN005I %4.4X:TCPNJE - listening on %s:%d for incoming connections\n",
            tn->dev->devnum, inet_ntoa(intmp), tn->lport);

    if (tn->state < TCPLISTEN) tn->state = TCPLISTEN;

    /* Indicate that we are listening for real */
    tn->listening = 2;

    return 0;
}
/*-------------------------------------------------------------------*/
/* tcpnje_connout : make a tcp outgoing call                         */
/* return values : 0 -> call succeeded or initiated                  */
/*                <0 -> call failed                                  */
/*                >0 -> nothing done due to connect parm being zero  */
/*-------------------------------------------------------------------*/
static int tcpnje_connout(struct TCPNJE *tn)
{
    int rc;
    struct sockaddr_in     sin;
    struct in_addr intmp;
    char lnodestring[9], rnodestring[9];

    if (!tn->connect) return(999);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = tn->rhost;
    sin.sin_port = htons(tn->rport);

    /* Are we randomly delaying to avoid colliding with other end's open attempts? */
    if (tn->activeopendelay && tn->listen)
    {
        DBGMSG(256, "HHCTN033I %4.4X:TCPNJE - delaying link %s - %s active open for %d attempt(s)\n",
                     tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
                                 guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode), tn->activeopendelay);
        if (tn->activeopendelay > 3) USLEEP(1000);
        tn->activeopendelay--;

        /* Pretend we failed to connect */
        return(-1);
    }

    if (tn->rhost == INADDR_NONE)
    {
        DBGMSG(2, "HHCTN034W %4.4X:TCPNJE - cannot make outgoing connection.  Remote ip address not specified\n",
                tn->dev->devnum);
        return(-1);
    }

    if (socket_is_socket(tn->afd))
    {
        DBGMSG(1, "HHCTN035W %4.4X:TCPNJE - closing outgoing socket as it is unexpectedly open\n",
                tn->dev->devnum);
        close_socket(tn->afd);
    }
    tn->afd = socket(AF_INET, SOCK_STREAM, 0);
    /* set socket to NON-blocking mode */
    rc = socket_set_blocking_mode(tn->afd, 0);
    if (rc < 0)
    {
        DBGMSG(4, "HHCTN036E %4.4X:TCPNJE - error setting socket for outgoing calls to non-blocking : %s\n",
                tn->dev->devnum, strerror(HSO_errno));
        return(-1);
    }
    intmp.s_addr = tn->rhost;
    /* Last chance to avoid connecting both ways at the same time */
    if (tn->state < TCPCONACT)
    {
        DBGMSG(128, "HHCTN037I %4.4X:TCPNJE - connecting out to %s:%d for link %s - %s\n",
                tn->dev->devnum, inet_ntoa(intmp), tn->rport,
                guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
                guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode));

        rc = connect(tn->afd, (struct sockaddr *)&sin, sizeof(sin));
        if (tn->state < TCPCONSNT) tn->state = TCPCONSNT;
        if (rc < 0)
        {
#if defined(_MSVC_)
            if (HSO_errno == HSO_EWOULDBLOCK)
#else /* defined(_MSVC_) */
            if (HSO_errno == HSO_EINPROGRESS)
#endif /* defined(_MSVC_) */
            {
                return(0);
            }
            else
            {
                intmp.s_addr = tn->rhost;
                DBGMSG(32, "HHCTN001W %4.4X:TCPNJE - connect out to %s:%d failed for link %s - %s : %s\n",
                        tn->dev->devnum, inet_ntoa(intmp), tn->rport,
                        guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
                        guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode), strerror(HSO_errno));
                close_socket(tn->afd);
                tn->afd = -1;
                if (tn->state == TCPCONSNT)
                {
                    tn->state = tn->listening ? TCPLISTEN : CLOSED;
                }
                return(-1);
            }
        }
        DBGMSG(128, "HHCTN038I %4.4X:TCPNJE - connected out to %s:%d for link %s - %s\n",
                tn->dev->devnum, inet_ntoa(intmp), tn->rport,
                guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
                guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode));
        tn->state = TCPCONACT;
    }
    disable_nagle(tn->afd);
    return(0);
}
/*-------------------------------------------------------------------*/
/* tcpnje_initiate_userdial : interpret DIAL data and initiate call  */
/* return values : 0 -> call succeeded or initiated                  */
/*                <0 -> call failed                                  */
/*-------------------------------------------------------------------*/
static int     tcpnje_initiate_userdial(struct TCPNJE *tn)
{
    int dotcount;       /* Number of seps (the 4th is the port separator) */
    int i;              /* work                                           */
    int cur;            /* Current section                                */
    in_addr_t   destip; /* Destination IP address                         */
    U16 destport;       /* Destination TCP port                           */
    int incdata;        /* Incorrect dial data found                      */
    int goteon;         /* EON presence flag                              */

   /* See the DIAL CCW portion in execute_ccw for dial format information */

    incdata = 0;
    goteon = 0;
    dotcount = 0;
    cur = 0;
    destip = 0;
    for(i = 0; i < tn->dialcount; i++)
    {
        if (goteon)
        {
            /* EON MUST be last data byte */

            DBGMSG(2, "HHCTN039E %4.4X:TCPNJE - found data beyond EON\n",
                    tn->dev->devnum);

            incdata = 1;
            break;
        }
        switch(tn->dialdata[i] & 0x0f)
        {
            case 0x0d:  /* SEP */
                if (dotcount < 4)
                {
                    if (cur > 255)
                    {
                        incdata = 1;

                        DBGMSG(2, "HHCTN040E %4.4X:TCPNJE - found incorrect IP address section at position %d\n",
                                tn->dev->devnum, dotcount + 1);
                        DBGMSG(2, "HHCTN041E %4.4X:TCPNJE - %d greater than 255\n",
                                tn->dev->devnum, cur);
                        break;
                    }
                    destip <<= 8;
                    destip += cur;
                    cur = 0;
                    dotcount++;
                }
                else
                {
                    incdata = 1;

                    DBGMSG(2, "HHCTN042E %4.4X:TCPNJE - too many separators in dial data\n",
                            tn->dev->devnum);

                    break;
                }
                break;
            case 0x0c: /* EON */
                goteon = 1;
                break;

                /* A,B,E,F not valid */
            case 0x0a:
            case 0x0b:
            case 0x0e:
            case 0x0f:
                incdata = 1;

                DBGMSG(2, "HHCTN043E %4.4X:TCPNJE - incorrect dial data byte %2.2x\n",
                        tn->dev->devnum, tn->dialdata[i]);

                break;
            default:
                cur *= 10;
                cur += tn->dialdata[i]&0x0f;
                break;
        }
        if (incdata)
        {
            break;
        }
    }
    if (incdata)
    {
        return -1;
    }
    if (dotcount < 4)
    {

        DBGMSG(2, "HHCTN044E %4.4X:TCPNJE - not enough separators (only %d found) in dial data\n",
                tn->dev->devnum, dotcount);

        return -1;
    }
    if (cur > 65535)
    {

        DBGMSG(2, "HHCTN045E %4.4X:TCPNJE - destination TCP port %d exceeds maximum of 65535\n",
                tn->dev->devnum, cur);

        return -1;
    }
    destport = cur;
    /* Update RHOST/RPORT */
    tn->rport = destport;
    tn->rhost = destip;
    return(tcpnje_connout(tn));
}
/*-------------------------------------------------------------------*/
/* Wakeup the TCPNJE thread                                          */
/* Code : 0 -> Just wakeup the thread to redrive the select          */
/* Code : 1 -> Halt the current executing I/O                        */
/* Code : 2 -> Pick up incoming connection from another device       */
/*-------------------------------------------------------------------*/
static void tcpnje_wakeup(struct TCPNJE *tn, BYTE code)
{
    if (write_pipe( tn->pipe[1], &code, 1 ) < 0)
    {
        // "Error in function %s: %s"
        WRMSG( HHC04000, "W", "write_pipe", strerror( errno ));
    }
}
/*-------------------------------------------------------------------*/
/* TCPNJE close connection to remote link partner                    */
/*                                                                   */
/* Close down connection to link partner as gracefully as possible   */
/* either because link partner has gone away or a serious error has  */
/* occurred attempting to contact it.                                */
/*-------------------------------------------------------------------*/
static void tcpnje_close(int fd, struct TCPNJE *tn)
{
    if (fd >= 0)
    {
        close_socket(fd);

        if (fd == tn->pfd)
        {
            tn->pfd = -1;
            if (tn->state == TCPCONPAS) tn->state = tn->listening ? TCPLISTEN : CLOSED;
        }
        else if (fd == tn->afd)
        {
            tn->afd = -1;
            tn->state = tn->listening ? TCPLISTEN : CLOSED;
        }
        else if (fd == tn->sfd)
        {
            tn->sfd = -1;

            /* Is RSCS connected to a remote link partner? */
            if (tn->state>NJEACKRCD)
            {
                /* Send RSCS a signoff the next time it issues a read */
                tn->signoff = 1;
            }
            tn->state = tn->listening ? TCPLISTEN : CLOSED;

            /* If an operation is in progress, abort it and advise CCW exec  */
            if (tn->curpending != TCPNJE_PEND_IDLE)
            {
                tn->curpending = TCPNJE_PEND_IDLE;
                signal_condition(&tn->ipc);
            }
        }
    }

    return;
}
/*-------------------------------------------------------------------*/
/* TCPNJE Read socket data in worker thread                          */
/*                                                                   */
/* Read up to the number of bytes given by wanted, adding to those   */
/* read in a previous call if necessary.  Update buffer->inptr to    */
/* indicate how many bytes have been read so far including that      */
/* read by previous calls.                                           */
/*                                                                   */
/* Returns: <0 if not as much as requested is returned on this call  */
/*          =0 if exactly the amount of data is returned on this call*/
/*          >0 if the amount required this time was already returned */
/*                                                                   */
/* This routine is designed to be used on a non-blocking socket. If  */
/* the required data is not returned on the first call, go and call  */
/* select() to wait for it.  When select() says more data has come   */
/* in, call this routine again without changing any arguments.  Keep */
/* doing this until all the required data has arrived in. Only then, */
/* alter the arguments to get the next block of data.                */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int tcpnje_read(int fd, struct TNBUFFER *buffer, size_t wanted, struct TCPNJE *tn)
{
    ssize_t done, count;

    if (wanted > buffer->size)
    {
        DBGMSG(1, "HHCTN046E %4.4X:TCPNJE - no room in input buffer for %d bytes requested. Stopping link.\n",
                tn->dev->devnum, (int)wanted);
        tcpnje_close(fd, tn);
    }
    count = buffer->inptr.address - buffer->base.address;
    if (count >= (ssize_t) wanted) return 1;

    done = recv(fd, buffer->inptr.address, wanted - count, 0);

    if (done > 0)
    {
        logdump("Fm net", tn->dev, buffer->inptr.address, done);
        buffer->inptr.address += done;
    }
    else
    {
        if (done < 0 && (
#ifndef WIN32
            EAGAIN == errno ||
#endif
            HSO_EWOULDBLOCK == HSO_errno))
        {
            /* Do as close to nothing as possible */
            ;
        }
        else
        {
            if (done == 0)
            {
                DBGMSG(4, "HHCTN047W %4.4X:TCPNJE - connection unexpectedly closed by remote peer.\n",
                        tn->dev->devnum);
            }
            else
            {
                DBGMSG(4, "HHCTN048E %4.4X:TCPNJE - error reading from socket: %s\n",
                        tn->dev->devnum, strerror(HSO_errno));
            }
            tcpnje_close(fd, tn);
        }
    }
    return -((wanted - (buffer->inptr.address - buffer->base.address)) > 0);
}
/*-------------------------------------------------------------------*/
/* TCPNJE Write socket data in worker thread                         */
/*                                                                   */
/* Write a TCPNJE block to the network.  Handle the case where the   */
/* write does not succeed due to insufficient network buffers and    */
/* leave everything set up to retry the write later.                 */
/*                                                                   */
/* Returns: <0 Permanent error.                                      */
/*          =0 Write successful.                                     */
/*          >0 Temporary error. Some data may have been written.     */
/*             Later retry may write more data.                      */
/*                                                                   */
/* Ideally, this routine would use write_socket().  However, since   */
/* v3.06, write_socket() loses count of how many bytes have been     */
/* written to a non-blocking socket which has run out of buffers so  */
/* using it would make it impossible to retry failed writes later.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int tcpnje_write(int fd, struct TNBUFFER *buffer, struct TCPNJE *tn)
{
    int count, part, done, savederrno;

    /* Find how much data we want to write this time around */
    count = buffer->inptr.address - buffer->outptr.address;
    part = count;

    while(part > 0)
    {
        done = send(fd, buffer->outptr.address, part, 0);

        if (done < 0) break;

        logdump("To net", tn->dev, buffer->outptr.address, done);

        buffer->outptr.address += done;
        part -= done;
    }

    savederrno = HSO_errno;   /* In case logmsg() mangles errno */

    DBGMSG(128, "HHCTN105D %4.4X:TCPNJE - wrote %d out of %d bytes\n",
            tn->dev->devnum, count - part, count);

    /* Did the whole buffer get written? */
    if (part > 0)
    {
        if (0
#ifndef WIN32
             || EAGAIN == savederrno
#endif
             || HSO_EWOULDBLOCK == savederrno
           )
        {
            /* Contending for write on main data socket? */
            if (!tn->holdoutgoing && (fd == tn->sfd))
            {
                DBGMSG(128, "HHCTN106D %4.4X:TCPNJE - holding outgoing data transmission due to write contention\n",
                        tn->dev->devnum);
                tn->holdoutgoing = 1;
            }
            return 1;
        }
        else
        {
            DBGMSG(4, "HHCTN049E %4.4X:TCPNJE - Attempt to write %d bytes to output socket only wrote %d bytes. Error: %s\n",
                    tn->dev->devnum, count, count - part, strerror(savederrno));
            tcpnje_close(fd, tn);
            return -1;
        }
    }

    /* If write was completed successfully after previous contention or other */
    /* reason for holding outgoing data on main data socket, resume sending.  */
    if (tn->holdoutgoing && (fd == tn->sfd))
    {
        DBGMSG(128, "HHCTN107D %4.4X:TCPNJE - resuming outgoing data transmission after successful network write\n",
                tn->dev->devnum);

        tn->holdoutgoing = 0;
    }

    /* Reset pointers to the beginning of the buffer for next time around */
    buffer->outptr.address = buffer->base.address;
    buffer->inptr.address = buffer->base.address;

    return 0;
}
/*-------------------------------------------------------------------*/
/* Send TCPNJE control record (OPEN/ACK/NAK)                         */
/*-------------------------------------------------------------------*/
static void tcpnje_ttc(int fd, const BYTE *type, int reason, struct TCPNJE *tn)
{
    struct in_addr intmp;
    struct sockaddr_in sockname;
#if defined(_MSVC_)
    int socknamelen;
#else /* defined(_MSVC_) */
    socklen_t socknamelen;
#endif /* defined(_MSVC_) */
    int rc;
    char typestring[9], lnodestring[9], rnodestring[9];

    memcpy(&tn->ttcactbuf.base.ttc->type, type, sizeof(tn->ttcactbuf.base.ttc->type));

    if (!memcmp(TCPNJE_OPEN, type, sizeof(tn->ttcactbuf.base.ttc->type)))
    {
        /* Get information required to fill in TTC for OPEN */
        memcpy(&tn->ttcactbuf.base.ttc->rhost, tn->lnode, sizeof(tn->ttcactbuf.base.ttc->rhost));
        memcpy(&tn->ttcactbuf.base.ttc->ohost, tn->rnode, sizeof(tn->ttcactbuf.base.ttc->ohost));

        /* Get ip address of this end of the connection */
        socknamelen = sizeof(sockname);
        rc = getsockname(fd, (struct sockaddr *) &sockname, &socknamelen);
        if (!rc)
        {
            tn->ttcactbuf.base.ttc->rip = sockname.sin_addr.s_addr;
        }
        else
        {
            /* Try using the address from the configuration if it looks reasonable */
            if (tn->lhost != INADDR_NONE)
            {
                tn->ttcactbuf.base.ttc->rip = tn->lhost;
            }
            else
            {
                tn->ttcactbuf.base.ttc->rip = INADDR_ANY;
            }
            intmp.s_addr = tn->ttcactbuf.base.ttc->rip;
            DBGMSG(4, "HHCTN050W %4.4X:TCPNJE - Error obtaining local ip address for TCPNJE OPEN: %s. Using %s\n",
                    tn->dev->devnum, strerror(HSO_errno), inet_ntoa(intmp));
        }

        /* Get ip address of remote end of the connection */
        socknamelen = sizeof(sockname);
        rc = getpeername(fd, (struct sockaddr *) &sockname, &socknamelen);
        if (!rc)
        {
            tn->ttcactbuf.base.ttc->oip = sockname.sin_addr.s_addr;
        }
        else
        {
            /* Try using the address from the configuration if it looks reasonable */
            if (tn->rhost != INADDR_NONE)
            {
                tn->ttcactbuf.base.ttc->oip = tn->rhost;
            }
            else
            {
                tn->ttcactbuf.base.ttc->oip = INADDR_ANY;
            }
            intmp.s_addr = tn->ttcactbuf.base.ttc->oip;
            DBGMSG(4, "HHCTN051W %4.4X:TCPNJE - Error obtaining remote ip address for TCPNJE OPEN: %s. Using %s\n",
                    tn->dev->devnum, strerror(HSO_errno), inet_ntoa(intmp));
        }
    }
    else
    {
        /* For ACK or NAK, use switched around information from the TTC we are replying to */
        memcpy(&tn->ttcactbuf.base.ttc->rhost, tn->ttcpasbuf.base.ttc->ohost, sizeof(tn->ttcactbuf.base.ttc->rhost));
        tn->ttcactbuf.base.ttc->rip = tn->ttcpasbuf.base.ttc->oip;
        memcpy(&tn->ttcactbuf.base.ttc->ohost, tn->ttcpasbuf.base.ttc->rhost, sizeof(tn->ttcactbuf.base.ttc->ohost));
        tn->ttcactbuf.base.ttc->oip = tn->ttcpasbuf.base.ttc->rip;
    }

    tn->ttcactbuf.base.ttc->r = reason;

    DBGMSG(256, "HHCTN108D %4.4X:TCPNJE - sending TCPNJE %s for link %s - %s",
                    tn->dev->devnum, guest_to_host_string(typestring, sizeof(typestring), type),
                    guest_to_host_string(lnodestring, sizeof(lnodestring), tn->ttcactbuf.base.ttc->rhost),
                    guest_to_host_string(rnodestring, sizeof(rnodestring), tn->ttcactbuf.base.ttc->ohost));

    if (!memcmp(TCPNJE_NAK, type, sizeof(tn->ttcactbuf.base.ttc->type)))
    {
        DBGMSG(256, " reason %d\n", reason);
    }
    else
    {
        DBGMSG(256, "\n");
    }

    tn->ttcactbuf.inptr.address = tn->ttcactbuf.base.address + SIZEOF_TTC;
    tn->ttcactbuf.outptr.address = tn->ttcactbuf.base.address;

    rc = tcpnje_write(fd, &tn->ttcactbuf, tn);
    if (rc)
    {
        u_int i;
        BYTE text[sizeof(tn->ttcactbuf.base.ttc->type) + 1];
        for(i = 0; i < sizeof(tn->ttcactbuf.base.ttc->type); i++)
        {
            text[i] = guest_to_host(type[i]);
            if (text[i] == ' ')
            {
                text[i] = '\0';
            }
        }
        text[i] = '\0';
        DBGMSG(8, "HHCTN052E %4.4X:TCPNJE - Error writing %s TTC to network: %s\n",
                tn->dev->devnum, text, strerror(HSO_errno));
    }

    return;
}
/*-------------------------------------------------------------------*/
/* TCPNJE Thread - Set TimeOut                                       */
/*-------------------------------------------------------------------*/
static struct timeval *tcpnje_setto(struct timeval *tv, int tmo)
{
    if (tmo != 0)
    {
        if (tmo < 0)
        {
            tv->tv_sec = 0;
            tv->tv_usec = 1;
        }
        else
        {
            tv->tv_sec = tmo / 1000;
            tv->tv_usec = (tmo % 1000) * 1000;
        }
        return(tv);
    }
    return(NULL);
}
/*-------------------------------------------------------------------*/
/* Process incoming TCPNJE request (OPEN)                            */
/*-------------------------------------------------------------------*/
static void tcpnje_process_request(struct TNBUFFER *buffer, struct TCPNJE *tn)
{
    char typestring[9], lnodestring[9], rnodestring[9];

    DBGMSG(256, "HHCTN109D %4.4X:TCPNJE - processing TCPNJE %s received for link %s - %s",
                  tn->dev->devnum, guest_to_host_string(typestring, sizeof(typestring), buffer->base.ttc->type),
                              guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                              guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));

    DBGMSG(256, "\n");

    /* Is this really an OPEN request? */
    if (memcmp(buffer->base.ttc->type, TCPNJE_OPEN, sizeof(buffer->base.ttc->type)))
    {
        struct sockaddr_in sockname;
        int rc;

#if defined(_MSVC_)
        int socknamelen;
#else /* defined(_MSVC_) */
        socklen_t socknamelen;
#endif /* defined(_MSVC_) */

        DBGMSG(8, "HHCTN123D %4.4X:TCPNJE - unrecognised TCPNJE control statement \"%s\"", tn->dev->devnum,
                      guest_to_host_string(typestring, sizeof(typestring), buffer->base.ttc->type));

        /* Get ip address and port of remote end of the connection */
        socknamelen = sizeof(sockname);
        rc = getpeername(tn->pfd, (struct sockaddr *) &sockname, &socknamelen);
        if (!rc)
        {
            DBGMSG(8, " received from %s:%d\n", inet_ntoa(sockname.sin_addr), ntohs(sockname.sin_port));
        }
        else
        {
            DBGMSG(8, "\n");
        }

        close_socket(tn->pfd);
        tn->pfd = -1;
        return;
    }

    /* Do the names of both ends specified in the OPEN request correspond with what we have? */
    if ((memcmp(tn->rnode, buffer->base.ttc->rhost, sizeof(buffer->base.ttc->rhost))) ||
        (memcmp(tn->lnode, buffer->base.ttc->ohost, sizeof(buffer->base.ttc->ohost))))
    {
        /* One or both doesn't match. Maybe some other TCPNJE device has the specified link? */
        DEVBLK *otherdev;
        int found = 0;

        DBGMSG(256, "HHCTN110D %4.4X:TCPNJE - link %s - %s is not handled by this device. Checking other TCPNJE devices.\n",
                      tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                  guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));

        /* Check other devices */
        for (otherdev = sysblk.firstdev; otherdev; otherdev = otherdev->nextdev)
        {
            struct TCPNJE *othertn;

            othertn = (struct TCPNJE *) otherdev->commadpt;

            /* Is this a TCPNJE device? (Do not force match of last character of blockname which is the minor version) */
            if (otherdev->allocated && ((otherdev->devtype == 0x2703) || (otherdev->devtype == 0x3088)) && otherdev->commadpt &&
                !memcmp(othertn->blockname, TCPNJE_VERSION, sizeof(othertn->blockname) - 1))
            {
                /* Have we found ourselves, which we've already checked? */
                if (othertn == tn) continue;

                /* Is the other device listening for incoming connections? */
                if (othertn->state >= TCPLISTEN)
                {
                    /* Are names of both ends of the link correct? */
                    if ((memcmp(buffer->base.ttc->rhost, othertn->rnode, sizeof(buffer->base.ttc->rhost))) ||
                        (memcmp(buffer->base.ttc->ohost, othertn->lnode, sizeof(buffer->base.ttc->ohost))))
                    {
                        /* No. Skip on to next device. */
                        continue;
                    }

                    DBGMSG(256, "HHCTN111D %4.4X:TCPNJE - processing TCPNJE OPEN received for link %s - %s on device %4.4X\n",
                              tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                          guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost),
                              otherdev->devnum);

                    obtain_lock(&othertn->lock);

                    /* Is the other device's link already connected? */
                    if (othertn->state >= NJECONPRI)
                    {
                        DBGMSG(256, "HHCTN112D %4.4X:TCPNJE - rejecting incoming TCPNJE OPEN for link %s - %s on device %4.4X : already connected\n",
                              tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                          guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost),
                              otherdev->devnum);
                        /* No. Reply with NAK.  Reason code 2 */
                        tcpnje_ttc(tn->pfd, TCPNJE_NAK, 2, tn);
                        close_socket(tn->pfd);
                        tn->pfd = -1;

                        /* Also kill the link which is assumed to be already failed but unnoticed */
                        DBGMSG(256, "HHCTN113D %4.4X:TCPNJE - closing link %s - %s on device %4.4X due to unexpected TCPNJE OPEN received for same\n",
                              tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                          guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost),
                              otherdev->devnum);

                        tcpnje_close(othertn->sfd, othertn);

                        release_lock(&othertn->lock);

                        if (tn->state == TCPCONPAS) tn->state = tn->listening ? TCPLISTEN : CLOSED;
                        return;
                    }

                    /* Is the other device already doing an active open? */
                    if ((othertn->state == TCPCONSNT) || (othertn->state == TCPCONACT) ||
                        (othertn->state == NJEOPNSNT) || (othertn->state == NJEACKRCD))
                    {
                        release_lock(&othertn->lock);
                        DBGMSG(256, "HHCTN114D %4.4X:TCPNJE - rejecting incoming TCPNJE OPEN for link %s - %s on device %4.4X : already doing active open\n",
                              tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                          guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost),
                              otherdev->devnum);
                        /* No. Reply with NAK.  Reason code 3 */
                        tcpnje_ttc(tn->pfd, TCPNJE_NAK, 3, tn);
                        close_socket(tn->pfd);
                        tn->pfd = -1;
                        close_socket(tn->afd);
                        tn->afd = -1;
                        tn->state = tn->listening ? TCPLISTEN : CLOSED;
                        return;
                    }

                    /* Is the other device already processing an incoming connection? */
                    if (othertn->pfd >= 0)
                    {
                        /* Drop it! */
                        DBGMSG(256, "HHCTN115D %4.4X:TCPNJE - Interrupting incoming connection in progress on device %4.4X\n",
                                 tn->dev->devnum, otherdev->devnum);
                        close_socket(othertn->pfd);
                        othertn->pfd = -1;
                    }

                    /* Send TCPNJE ACK on behalf of other device */
                    tcpnje_ttc(tn->pfd, TCPNJE_ACK, 0, tn);
                    othertn->state = NJEACKSNT;

                    /* TCPNJE OPEN sequence complete. Transfer connection to main I/O code. */
                    othertn->sfd = tn->pfd;
                    tn->pfd = -1;

                    /* Prepare to receive the first TTB */
                    othertn->tcpinbuf.inptr.address = othertn->tcpinbuf.base.address;

                    DBGMSG(256, "HHCTN053I %4.4X:TCPNJE - passing TCPNJE OPEN for link %s - %s to device %4.4X\n",
                              tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                          guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost),
                              otherdev->devnum);

                    /* Poke the other device to pick up the connection */
                    tcpnje_wakeup(othertn, 2);

                    release_lock(&othertn->lock);

                    /* Meanwhile, we go back to whatever we were doing */
                    if (tn->state == TCPCONPAS) tn->state = tn->listening ? TCPLISTEN : CLOSED;

                    found = 1;

                    break;
                }
            }
        }

        if (!found)
        {
            DBGMSG(256, "HHCTN116D %4.4X:TCPNJE - rejecting TCPNJE OPEN for unrecognised or inactive link %s - %s\n",
                              tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                          guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));

            /* No luck finding link anywhere. Reply with NAK.  Reason code 1. */
            tcpnje_ttc(tn->pfd, TCPNJE_NAK, 1, tn);

            close_socket(tn->pfd);
            tn->pfd = -1;
            if (tn->state == TCPCONPAS) tn->state = tn->listening ? TCPLISTEN : CLOSED;
        }

        return;
    }

    /* Is the link already connected? */
    if (tn->state >= NJECONPRI)
    {
        DBGMSG(256, "HHCTN117D %4.4X:TCPNJE - rejecting incoming TCPNJE OPEN for link %s - %s : already connected\n",
                       tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                   guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));

        /* No. Reply with NAK.  Reason code 2 */
        tcpnje_ttc(tn->pfd, TCPNJE_NAK, 2, tn);
        close_socket(tn->pfd);
        tn->pfd = -1;

        /* Also kill the link which is assumed to be already failed but unnoticed */
        DBGMSG(256, "HHCTN118D %4.4X:TCPNJE - closing link %s - %s due to unexpected TCPNJE OPEN received for same\n",
                       tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                   guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));
        tcpnje_close(tn->sfd, tn);

        return;
    }

    /* Are we already doing an active open? */
    if ((tn->state == TCPCONSNT) || (tn->state == TCPCONACT) || (tn->state == NJEOPNSNT) || (tn->state == NJEACKRCD))
    {
        DBGMSG(256, "HHCTN119D %4.4X:TCPNJE - rejecting incoming TCPNJE OPEN for link %s - %s : already doing active open\n",
                       tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                   guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));

        /* No. Reply with NAK.  Reason code 3 */
        tcpnje_ttc(tn->pfd, TCPNJE_NAK, 3, tn);
        close_socket(tn->pfd);
        tn->pfd = -1;
        close_socket(tn->afd);
        tn->afd = -1;
        tn->state = tn->listening ? TCPLISTEN : CLOSED;
        return;
    }

    /* Are we expecting a TCPNJE OPEN for this link? */
    if (tn->state != TCPCONPAS)
    {
        DBGMSG(256, "HHCTN120D %4.4X:TCPNJE - ignoring unexpected TCPNJE OPEN for link %s - %s while in connection state: %s\n",
                       tn->dev->devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                   guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost),
                                   tcpnje_state_text[tn->state]);
        tcpnje_ttc(tn->pfd, TCPNJE_NAK, 3, tn);
        close_socket(tn->pfd);
        tn->pfd = -1;
        close_socket(tn->afd);
        tn->afd = -1;
        tn->state = tn->listening ? TCPLISTEN : CLOSED;
        return;
    }

    /* Send TCPNJE ACK */
    tcpnje_ttc(tn->pfd, TCPNJE_ACK, 0, tn);

    tn->state = NJEACKSNT;

    /* TCPNJE OPEN sequence complete. Transfer connection to main I/O code. */
    tn->sfd = tn->pfd;
    tn->pfd = -1;

    /* Prepare to receive the first TTB */
    tn->tcpinbuf.inptr.address = tn->tcpinbuf.base.address;

    return;
}

/*-------------------------------------------------------------------*/
/* Process incoming TCPNJE ACK or NAK reply                          */
/*-------------------------------------------------------------------*/
static void tcpnje_process_reply(struct TNBUFFER *buffer, struct TCPNJE *tn)
{
    char typestring[9], lnodestring[9], rnodestring[9];

    DBGMSG(256, "HHCTN109D %4.4X:TCPNJE - processing TCPNJE %s received for link %s - %s",
                  tn->dev->devnum, guest_to_host_string(typestring, sizeof(typestring), buffer->base.ttc->type),
                              guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                              guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));

    if (!memcmp(buffer->base.ttc->type, TCPNJE_NAK, sizeof(buffer->base.ttc->type)))
    {
        DBGMSG(256, " reason code %d\n", buffer->base.ttc->r);
    }
    else
    {
        DBGMSG(256, "\n");
    }

    /* Do the names of both ends of the link correspond with an OPEN we would have sent out? */
    if ((memcmp(tn->rnode, buffer->base.ttc->rhost, sizeof(buffer->base.ttc->rhost))) ||
        (memcmp(tn->lnode, buffer->base.ttc->ohost, sizeof(buffer->base.ttc->ohost))))
    {
        DBGMSG(256, "HHCTN121D %4.4X:TCPNJE - ignoring TCPNJE %s received for unrecognised link %s - %s\n",
                      tn->dev->devnum, guest_to_host_string(typestring, sizeof(typestring), buffer->base.ttc->type),
                                  guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                  guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));
        close_socket(tn->afd);
        tn->afd = -1;
        tn->state = tn->listening ? TCPLISTEN : CLOSED;
        return;
    }

    /* Are we expecting a TCPNJE ACK or NAK ie has a TCPNJE OPEN been sent out for this link? */
    if (tn->state != NJEOPNSNT)
    {
        DBGMSG(256, "HHCTN122D %4.4X:TCPNJE - ignoring unexpected TCPNJE %s received for link %s - %s\n",
                      tn->dev->devnum, guest_to_host_string(typestring, sizeof(typestring), buffer->base.ttc->type),
                                  guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                  guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));

        close_socket(tn->afd);
        tn->afd = -1;
        tn->state = tn->listening ? TCPLISTEN : CLOSED;
        return;
    }

    if (!memcmp(buffer->base.ttc->type, TCPNJE_ACK, sizeof(buffer->base.ttc->type)))
    {
        tn->state = NJEACKRCD;

        /* TCPNJE OPEN sequence complete. Transfer connection to main I/O code. */
        tn->sfd = tn->afd;

        /* Prepare to receive the first TTB */
        tn->tcpinbuf.inptr.address = tn->tcpinbuf.base.address;
    }
    else if (!memcmp(buffer->base.ttc->type, TCPNJE_NAK, sizeof(buffer->base.ttc->type)))
    {
        /* Was NAK because of unrecognised link or other end was also trying an active open? */
        if (buffer->base.ttc->r != 2)
        {
            struct timeval timenow;

            /* Make sort-of random number of from current time */
            gettimeofday(&timenow, NULL);

            /* Only retry active open after randomish number of attempts */
            tn->activeopendelay = (timenow.tv_sec + timenow.tv_usec) & 3;

            /* However, if we got bashed by a NAK 3, restart the listener and
               give active connecting a good long pause */
            if (buffer->base.ttc->r == 3)
            {
                tn->activeopendelay = 20;
                close_socket(tn->lfd);
                tn->lfd = -1;
                tn->listening = 0;
            }
        }

        close_socket(tn->afd);
        tn->afd = -1;
        tn->state = tn->listening ? TCPLISTEN : CLOSED;
    }
    else
    {
        DBGMSG(8, "HHCTN123D %4.4X:TCPNJE - unrecognised or unexpected TCPNJE control statement \"%s\" received for link %s - %s\n",
                      tn->dev->devnum, guest_to_host_string(typestring, sizeof(typestring), buffer->base.ttc->type),
                                  guest_to_host_string(lnodestring, sizeof(lnodestring), buffer->base.ttc->ohost),
                                  guest_to_host_string(rnodestring, sizeof(rnodestring), buffer->base.ttc->rhost));
        close_socket(tn->afd);
        tn->state = tn->listening ? TCPLISTEN : CLOSED;
    }

    /* Whatever happened, we are finished with this socket file descriptor now */
    tn->afd = -1;

    return;

}

/*-------------------------------------------------------------------*/
/* TCPNJE Thread main loop                                           */
/*-------------------------------------------------------------------*/
static void *tcpnje_thread(void *vtn)
{
    struct TCPNJE *tn;                 /* Work TN Control Block Pointer     */
    int devnum;                 /* device number copy for convenience*/
    int rc;                     /* return code from various rtns     */
    int selectcount;            /* Count of reasons select() returned*/
    int tempfd;                 /* FileDesc to accept connections    */
    int writecont;              /* Write contention active           */
    int soerror;                /* getsockopt SOERROR value          */
    int maxfd;                  /* highest FD for select             */
    int tn_shutdown;            /* Thread shutdown internal flag     */
    int init_signaled;          /* Thread initialisation signaled    */
    int TTBlength = 0;          /* Length of TTB in host byte order  */
    int eintrcount = 0;         /* Number of times EINTR occured     */
    int errorcount067 = 0;      /* Number of times HHCTN067E issued  */
    int errorcount100 = 0;      /* Number of times HHCTN100E issued  */
    struct sockaddr_in remaddr;                /* For accept()       */
    unsigned int remlength = sizeof(remaddr);  /* also for accept()  */
    struct      in_addr intmp;  /* To print ip address in error msgs */
    socklen_t   soerrsize;      /* Size for getsockopt               */
    struct timeval tv;          /* select timeout structure          */
    struct timeval tvcopy;      /* copy of select timeout structure  */
    struct timeval *seltv;      /* ptr to the timeout structure      */
    fd_set      rfd, wfd, xfd;  /* SELECT File Descriptor Sets       */
    BYTE        pipecom;        /* Byte read from IPC pipe           */
    char lnodestring[9];        /* Displayable local node name       */
    char rnodestring[9];        /* Displayable remote node name      */
    /*---------------------END OF DECLARES---------------------------*/

    /* fetch the TCPNJE structure */
    tn = (struct TCPNJE *)vtn;

    /* Obtain the TCPNJE lock */
    obtain_lock(&tn->lock);

    /* get a work copy of devnum (for messages) */
    devnum = tn->dev->devnum;

    /* reset shutdown flag */
    tn_shutdown = 0;

    init_signaled = 0;

    DBGMSG(1, "HHCTN002I %4.4X:TCPNJE - networking thread "TIDPAT" started for link %s - %s\n",
            devnum, TID_CAST(thread_id()),
            guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
            guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode));

    if (!init_signaled)
    {
        tn->curpending = TCPNJE_PEND_IDLE;
        signal_condition(&tn->ipc);
        init_signaled = 1;
    }

    /* The MAIN select loop */
    /* It will listen on the following sockets : */
    /* tn->lfd : The listen socket */
    /* tn->sfd :
     *         read : When a connect, read, prepare or DIAL command is in effect
     *        write : When a write contention occurs
     * tn->pipe[0] : Always
     *
     * A 3 Seconds timer is started for a read operation
     */

    writecont = 0;             /* Ensure write contention flag is not set */

    while(!tn_shutdown)
    {
        FD_ZERO(&rfd);
        FD_ZERO(&wfd);
        FD_ZERO(&xfd);
        maxfd = 0;
        seltv = NULL;

        DBGMSG(512, "HHCTN124D %4.4X:TCPNJE - top of loop - Operation = %s\n",
                devnum, tcpnje_pendccw_text[tn->curpending]);

        switch(tn->curpending)
        {
            case TCPNJE_PEND_SHUTDOWN:
                tn_shutdown = 1;
                break;
            case TCPNJE_PEND_IDLE:
                break;
            case TCPNJE_PEND_READ:
                /* Flag that we don't have a complete buffer yet */
                tn->tcpinbuf.valid = 0;

                /* If we're not connected, we're not going to get any data */
                if (tn->state < TCPCONACT)
                {
                    tn->curpending = TCPNJE_PEND_IDLE;
                    signal_condition(&tn->ipc);
                }
                /* If we are connected but don't have any data, get some */
                else
                {
                    /* Be sure not to set bits for connections which are gone */
                    if (tn->afd >= 0)
                    {
                        FD_SET(tn->afd, &rfd);
                        maxfd = maxfd < tn->afd ? tn->afd : maxfd;
                    }
                    if (tn->sfd >= 0)
                    {
                        FD_SET(tn->sfd, &rfd);
                        maxfd = maxfd < tn->sfd ? tn->sfd : maxfd;
                    }
                    /* Set timeout */
                    seltv = tcpnje_setto(&tv, tn->timeout);
                }
                break;
            case TCPNJE_PEND_WRITE:
                rc = tcpnje_write(tn->sfd, &tn->tcpoutbuf, tn);
                if (rc > 0)
                {
                    /* Write blocked.  Flag retry required. */
                    writecont = 1;
                }
                else
                {
                    /* Write succeeded or error occurred */
                    writecont = 0;
                }

                /* Advise CCW exec to move on whether write completed or not */
                tn->curpending = TCPNJE_PEND_IDLE;
                signal_condition(&tn->ipc);
                break;
            case TCPNJE_PEND_DIAL:
                if (tn->state >= TCPCONSNT)
                {
                    tn->curpending = TCPNJE_PEND_IDLE;
                    signal_condition(&tn->ipc);
                    break;
                }
                rc = tcpnje_initiate_userdial(tn);
                if (rc != 0 || (rc == 0 && tn->state >= TCPCONSNT))
                {
                    tn->curpending = TCPNJE_PEND_IDLE;
                    signal_condition(&tn->ipc);
                    break;
                }
                FD_SET(tn->sfd, &wfd);
                maxfd = maxfd < tn->sfd ? tn->sfd : maxfd;
#if defined(_MSVC_)
                FD_SET(tn->sfd, &xfd);
#endif /* defined(_MSVC_) */
                break;
            case TCPNJE_PEND_CONNECT:
                /* If connection is not yet open, reset everything to starting values first */
                if (tn->state == CLOSED)
                {
                    /* Initialise output buffer pointers */
                    tn->tcpoutbuf.outptr.address = tn->tcpoutbuf.base.address;
                    tn->tcpoutbuf.inptr.address = tn->tcpoutbuf.base.address;
                    /* Initialise input buffer pointer */
                    tn->tcpinbuf.outptr.address = tn->tcpinbuf.base.address;
                    /* Initialise input buffer valid flag */
                    tn->tcpinbuf.valid = 0;
                    /* Reset the input suspended due to FCS flag */
                    tn->holdincoming = 0;
                    /* Reset output suspended due to write contention */
                    tn->holdoutgoing = 0;
                    /* Reset FASTOPEN issued for stream n */
                    tn->fastopen = 0;
                    /* Reset wait-a-bit bit set flag */
                    tn->waitabit = 0;
                    /* Reset the reset BCB flag */
                    tn->resetoutbcb = 0;
                    /* Reset the SYN NAK received / sent flags */
                    tn->synnakreceived = 0;
                    tn->synnaksent = 0;
                    /* Reset the outgoing buffers not yet ACKed count */
                    tn->ackcount = 0;
                    /* Reset the send signoff to RSCS flag */
                    tn->signoff = 0;
                    /* Clear idle writes counter */
                    tn->idlewrites = 0;
                    /* Reset data count statistics */
                    tn->inbuffcount = 0;
                    tn->inbytecount = 0;
                    tn->outbuffcount = 0;
                    tn->outbytecount = 0;
                    /* Reset counts of various errors */
                    errorcount067 = 0;
                    errorcount100 = 0;
                    /* Estimate buffer size to use until RSCS negotiates it */
                    tn->tpbufsize = tn->tcpoutbuf.size/2;
                }
                /* Are we supposed to be listening for incoming connections? */
                /* if this is a DIAL=OUT only line, no listen is necessary */
                if (tn->dolisten && (tn->listening != 2))
                {
                    rc = tcpnje_listen(tn);

                    /* Was a shutdown signalled while we were trying to set up listening port? */
                    if (tn->curpending == TCPNJE_PEND_SHUTDOWN)
                    {
                        tn_shutdown = 1;
                        break;
                    }

                    /* Put up with something going wrong with the listening port for now.
                       If the outgoing call succeeds, it won't be needed anyway.           */

                }
                /* Are we already connected? */
                if (tn->state >= NJEACKSNT)
                {
                    /* This is as far as we can go without READ & WRITE */
                    tn->curpending = TCPNJE_PEND_IDLE;
                    signal_condition(&tn->ipc);
                    break;
                }
                /* Set a timeout in case we don't get connected */
                seltv = tcpnje_setto(&tv, tn->cto);
                switch(tn->dialin + tn->dialout * 2)
                {
                    case 0: /* DIAL=NO */
                        /* callissued is set here when the call */
                        /* actually failed. But we want to time */
                        /* a bit for program issuing WRITES in  */
                        /* a tight loop                         */
                        if (tn->callissued)
                        {
                            seltv = tcpnje_setto(&tv, tn->cto);
                            break;
                        }
                        /* Do not try to connect now if already connecting */
                        if (tn->state < TCPCONSNT)
                        {
                            /* Issue a Connect out */
                            DBGMSG(128, "HHCTN054I %4.4X:TCPNJE - making outgoing leased line connection\n",
                                    devnum);
                            rc = tcpnje_connout(tn);
                            if (rc == 0)
                            {
                                /* Call issued */
                                if (tn->state == TCPCONACT)
                                {
                                    /* Call completed immediately.  Send TCPNJE OPEN request */
                                    tcpnje_ttc(tn->afd, TCPNJE_OPEN, 0, tn);
                                    tn->state = NJEOPNSNT;
                                    /* Prepare to receive incoming TCPNJE ACK */
                                    tn->ttcactbuf.inptr.address = tn->ttcactbuf.base.address;
                                }
                                else if (tn->state == TCPCONSNT)
                                {
                                    /* Call initiated - FD will be ready */
                                    /* for writing when the connect ends */
                                    /* getsockopt/SOERROR will tell if   */
                                    /* the call was sucessfull or not    */
                                    FD_SET(tn->afd, &wfd);
#if defined(_MSVC_)
                                    FD_SET(tn->afd, &xfd);
#endif /* defined(_MSVC_) */
                                    maxfd = maxfd < tn->afd ? tn->afd : maxfd;
                                    tn->callissued = 1;
                                }
                                else
                                {
                                    DBGMSG(1, "HHCTN055W %4.4X:TCPNJE - unexpected state after outgoing call: %s\n",
                                            devnum, tcpnje_state_text[tn->state]);
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
                            /* to prevent OSes from issuing a loop of WRITES       */
                            else
                            {
                                if (rc != 999)
                                    DBGMSG(32, "HHCTN007W %4.4X:TCPNJE - outgoing connection for link %s - %s failed or deferred\n",
                                            devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
                                                    guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode));
                                seltv = tcpnje_setto(&tv, tn->cto);
                            }
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

                        /* Signal connect has completed */
                        tn->curpending = TCPNJE_PEND_IDLE;
                        signal_condition(&tn->ipc);
                        break;
                /* For cases not DIAL=OUT, the listen is already started */
                }

                /* If we are waiting on TCPNJE ACK. tell select()*/
                if (tn->state == NJEOPNSNT)
                {
                    FD_SET(tn->afd, &rfd);
                    maxfd = maxfd < tn->afd ? tn->afd : maxfd;
                }
                break;

                /* The CCW Executor says : DISABLE */
            case TCPNJE_PEND_DISABLE:
                if (tn->listening > 1)
                {
                    DBGMSG(128, "HHCTN056I %4.4X:TCPNJE - closing listening socket due to DISABLE\n",
                            devnum);
                    close_socket(tn->lfd);
                    tn->lfd = -1;
                }
                tn->listening = 0;

                if (tn->state >= TCPCONSNT)
                {
                    DBGMSG(128, "HHCTN057I %4.4X:TCPNJE - closing connection socket due to DISABLE\n",
                            devnum);
                    close_socket(tn->pfd);
                    tn->pfd = -1;
                    close_socket(tn->afd);
                    tn->afd = -1;
                    close_socket(tn->sfd);
                    tn->sfd = -1;
                }
                tn->state = CLOSED;
                tn->curpending = TCPNJE_PEND_IDLE;
                signal_condition(&tn->ipc);
                break;

                /* A PREPARE has been issued */
            case TCPNJE_PEND_PREPARE:
                if ((tn->state < TCPCONACT) || tn->tcpinbuf.valid)
                {
                    tn->curpending = TCPNJE_PEND_IDLE;
                    signal_condition(&tn->ipc);
                    break;
                }
                break;
                /* RSCS has sent out an FCS with the wait-a-bit bit set */
            case TCPNJE_PEND_WAIT:
                /* Set time out */
                seltv = tcpnje_setto(&tv, tn->rto);
                break;
                /* Don't know - shouldn't be here anyway */
            default:
                break;
        }

        /* If TCPNJE is shutting down, exit the loop now */
        if (tn_shutdown)
        {
            tn->curpending = TCPNJE_PEND_IDLE;
            signal_condition(&tn->ipc);
            break;
        }

        /* Set the IPC pipe in the select() */
        FD_SET(tn->pipe[0], &rfd);
        maxfd = maxfd < tn->pipe[0] ? tn->pipe[0] : maxfd;

        /* If we are actually listening for connections, tell select() */
        if (tn->listening > 1)
        {
            FD_SET(tn->lfd, &rfd);
            maxfd = maxfd < tn->lfd ? tn->lfd : maxfd;

            /* A TCPNJE OPEN might arrive any time an incoming connection is active. Tell select() */
            if (tn->pfd >= 0)
            {
                FD_SET(tn->pfd, &rfd);
                maxfd = maxfd < tn->pfd ? tn->pfd : maxfd;
            }
        }

        /* If we are waiting for a write contention to clear, tell select() to watch for it. */
        if (writecont && tn->sfd >= 0)
        {
            FD_SET(tn->sfd, &wfd);
            maxfd = maxfd < tn->sfd ? tn->sfd : maxfd;
        }

        /* The the MAX File Desc for Arg 1 of SELECT */
        maxfd++;

        DBGMSG(512, "HHCTN125D %4.4X:TCPNJE - Entering select(). Operation: %s\n",
                devnum, tcpnje_pendccw_text[tn->curpending]);

        /* Release the TN Lock before the select - all FDs addressed by the select are only */
        /* handled by the thread, and communication from CCW Executor/others to this thread */
        /* is via the pipe, which queues the info                                           */
        release_lock(&tn->lock);

        /* Linux may mangle the timeout value so grab a copy for when we need it later */
        tvcopy = tv;

        selectcount = select(maxfd, &rfd, &wfd, &xfd, seltv);

        /* Get the TCPNJE lock back */
        obtain_lock(&tn->lock);

        DBGMSG(512, "HHCTN126D %4.4X:TCPNJE - select() returned %d\n",
                devnum, selectcount);

        if (selectcount == -1)
        {
            if (errno == EINTR)
            {
                eintrcount++;
                if ((eintrcount % 1000) == 0)
                {
                    DBGMSG(1, "HHCTN058W %4.4X:TCPNJE - select() unexpectedly interrupted %d times in a row\n",
                              devnum, eintrcount);
                }
                continue;
            }
            DBGMSG(1, "HHCTN006E %4.4X:TCPNJE - select() error : %s\n", devnum, strerror(HSO_errno));
            break;
        }
        eintrcount = 0;

        /* Select timed out */
        if (selectcount == 0)
        {
            DBGMSG(512, "HHCTN127D %4.4X:TCPNJE - select() timeout after %ld seconds %ld microseconds\n",
                        devnum, tvcopy.tv_sec, (long int)tvcopy.tv_usec);

            /* Reset Call issued flag */
            tn->callissued = 0;

            /* timeout condition */
            signal_condition(&tn->ipc);
            tn->curpending = TCPNJE_PEND_IDLE;

            /* If nothing else triggered select() to return, there is not much point in checking anything else now */
            continue;
        }

        if (selectcount && FD_ISSET(tn->pipe[0], &rfd))
        {
            /* One of the causes of select() returning accounted for */
            selectcount--;

            rc = read_pipe(tn->pipe[0], &pipecom, 1);
            if (rc == 0)
            {
                DBGMSG(512, "HHCTN128D %4.4X:TCPNJE - IPC Pipe closed\n", devnum);

                /* Pipe closed : terminate thread & release TCPNJE lock */
                tn_shutdown = 1;
                /* Exit the main while loop containing select() */
                break;
            }

            DBGMSG(512, "HHCTN129D %4.4X:TCPNJE - IPC Pipe Data ; code = %d\n", devnum, pipecom);

            switch(pipecom)
            {
                case 0: /* redrive select */
                        /* occurs when a new CCW is being executed */
                    break;
                case 1: /* Halt current I/O */
                    tn->callissued = 0;
                    if (tn->curpending == TCPNJE_PEND_DIAL)
                    {
                        DBGMSG(128, "HHCTN130D %4.4X:TCPNJE - Closing socket due to halt\n",
                                devnum);
                        close_socket(tn->sfd);
                        tn->sfd = -1;
                        tn->state = tn->listening ? TCPLISTEN : CLOSED;
                    }

                    if (tn->curpending != TCPNJE_PEND_DISABLE)
                    {
                        /* I'm not sure if it's supposed to be possible to halt a DISABLE CCW and if it is, whether
                           the disable should return with UX set or not.  From observation, it appears that allowing
                           a DISABLE to be halted (at least in the case where UX is not set) may cause RSCS to think
                           the line has been disabled when it has not.  Therefore, I am going to pretend that the
                           DISABLE had already completed by the time the time the halt was processed.               */

                        tn->curpending = TCPNJE_PEND_IDLE;
                        tn->haltpending = 1;
                        signal_condition(&tn->ipc);
                    }

                    signal_condition(&tn->ipc_halt);    /* Tell the halt initiator */
                    break;

                case 2: /* TCPNJE OPEN for this device received by listener on another device */
                    DBGMSG(256, "HHCTN059I %4.4X:TCPNJE - TCPNJE OPEN redirected from another device. Connection state: %s\n",
                              devnum, tcpnje_state_text[tn->state]);
                    break;
                default:
                    break;
            }
        }

        if (selectcount && (tn->sfd >= 0) && FD_ISSET(tn->sfd, &wfd))
        {
            if (writecont)
            {
                DBGMSG(128, "HHCTN131D %4.4X:TCPNJE - Write buffer space available.  Retrying last write.\n",
                        devnum);

                /* One of the causes of select() returning accounted for */
                selectcount--;

                rc = tcpnje_write(tn->sfd, &tn->tcpoutbuf, tn);
                if (rc == 0)
                {
                    /* Write completed successfully */
                    writecont = 0;
                }
            }
        }

        /* Did a connection attempt complete? */
        if (selectcount && (tn->afd >= 0) && (FD_ISSET(tn->afd, &wfd)
#if defined(_MSVC_)
                                          ||  FD_ISSET(tn->afd, &xfd)
#endif /* defined(_MSVC_) */
                                                                     ))
        {
            DBGMSG(256, "HHCTN132D %4.4X:TCPNJE - connection event\n", devnum);

            /* One of the causes of select() returning accounted for */
            selectcount--;

            switch(tn->curpending)
            {
                case TCPNJE_PEND_DIAL:
                case TCPNJE_PEND_CONNECT:  /* Leased line connect case */

                soerrsize = sizeof(soerror);
                getsockopt(tn->afd, SOL_SOCKET, SO_ERROR, (GETSET_SOCKOPT_T*)&soerror, &soerrsize);

#if defined(_MSVC_)
                if (FD_ISSET(tn->afd, &wfd))
#else /* defined(_MSVC_) */
                if (soerror == 0)
#endif /* defined(_MSVC_) */
                {
                    if (tn->state == TCPCONSNT)
                    {
                        tn->state = TCPCONACT;
                        DBGMSG(128, "HHCTN133D %4.4X:TCPNJE - outgoing call connected for link %s - %s\n",
                                devnum, guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
                                        guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode));

                        /* Connect successful. Send TCPNJE OPEN request. */
                        tcpnje_ttc(tn->afd, TCPNJE_OPEN, 0, tn);
                        tn->state = NJEOPNSNT;
                        /* Prepare to receive incoming TCPNJE ACK */
                        tn->ttcactbuf.inptr.address = tn->ttcactbuf.base.address;
                    }
                    else
                    {
                        DBGMSG(1, "HHCTN060W %4.4X:TCPNJE - unexpected state %s after outgoing call connected\n",
                                devnum, tcpnje_state_text[tn->state]);
                    }
                }
                else
#if defined(_MSVC_)
                if (FD_ISSET(tn->afd, &xfd))
#else /* defined(_MSVC_) */
                if (soerror != 0)
#endif /* defined(_MSVC_) */
                {
                    intmp.s_addr = tn->rhost;
                    DBGMSG(32, "HHCTN061W %4.4X:TCPNJE - outgoing call to %s:%d for link %s - %s failed: %s\n",
                        devnum, inet_ntoa(intmp), tn->rport,
                        guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
                        guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode), strerror(soerror));
                    if (tn->curpending == TCPNJE_PEND_CONNECT)
                    {
                        /* Ensure top of the loop doesn't restart a new call */
                        /* but starts a 3 second timer instead               */
                        tn->callissued = 1;
                    }
                    close_socket(tn->afd);
                    tn->afd = -1;
                    if (tn->state == TCPCONSNT)
                    {
                        tn->state = tn->listening ? TCPLISTEN : CLOSED;
                    }
                    signal_condition(&tn->ipc);
                    tn->curpending = TCPNJE_PEND_IDLE;
                }
                break;

                default:
                break;
            }
        }

        /* Are we expecting real data rather than TCPNJE connection overhead? */
        if (selectcount && (tn->state >= NJEACKSNT) && (tn->sfd >= 0) && FD_ISSET(tn->sfd, &rfd))
        {
            DBGMSG(128, "HHCTN134D %4.4X:TCPNJE - inbound data. Connection state: %s\n",
                    devnum, tcpnje_state_text[tn->state]);

            /* One of the causes of select() returning accounted for */
            selectcount--;

            rc = tcpnje_read(tn->sfd, &tn->tcpinbuf, SIZEOF_TTB, tn);

            /* Have we read in a complete TTB yet? */
            if (rc == 0)
            {
                /* We now have the exact number of bytes in the TTB.
                   Get the size of the whole block from it.           */
                TTBlength = ntohs(tn->tcpinbuf.base.ttb->length);

                DBGMSG(2048, "HHCTN135D %4.4X:TCPNJE incoming TTB, length %d. Connection state %s\n",
                            devnum, TTBlength, tcpnje_state_text[tn->state]);
            }

            if (rc >= 0)
            {
                /* We have at least the TTB and possibly more.
                   Now ensure the block is completely read in */
                rc = tcpnje_read(tn->sfd, &tn->tcpinbuf, TTBlength, tn);

                DBGMSG(2048, "HHCTN136D %4.4X:TCPNJE - bytes required %d - read so far %ld. Connection state %s\n",
                        devnum, TTBlength, tn->tcpinbuf.inptr.address - tn->tcpinbuf.base.address, tcpnje_state_text[tn->state]);

                if (rc == 0)
                {
                    /* We have now received a complete TCPNJE buffer so advise
                       CCW executor that there is now data available to read. */
                    tn->tcpinbuf.valid = 1;

                    tn->curpending = TCPNJE_PEND_IDLE;
                    signal_condition(&tn->ipc);

                    DBGMSG(2048, "HHCTN137D %4.4X:TCPNJE - TTB read complete. Connection state %s\n",
                            devnum, tcpnje_state_text[tn->state]);

                    /* Prepare to receive next incoming TTB */
                    tn->tcpinbuf.inptr.address = tn->tcpinbuf.base.address;
                }
            }
        }

        /* Any incoming TCPNJE requests? */
        if (selectcount && (tn->pfd >= 0) && FD_ISSET(tn->pfd, &rfd))
        {
            DBGMSG(256, "HHCTN138D %4.4X:TCPNJE - passive open TCPNJE protocol traffic. Connection state: %s\n",
                devnum, tcpnje_state_text[tn->state]);

            /* One of the causes of select() returning accounted for */
            selectcount--;

            /* Receive the incoming TCPNJE request */
            rc = tcpnje_read(tn->pfd, &tn->ttcpasbuf, SIZEOF_TTC, tn);

            /* Did we get the complete TTC? If not, wait for more before doing anything */
            if (rc == 0)
            {
                /* Deal with the TCPNJE OPEN or whatever request */
                tcpnje_process_request(&tn->ttcpasbuf, tn);

                /* Reset buffer pointer for next time something arrives */
                tn->ttcpasbuf.inptr.address = tn->ttcpasbuf.base.address;
            }
            else if (rc > 0)
            {
                if (errorcount100 < TCPNJE_MAX_ERRORCOUNT)
                {
                    DBGMSG(2, "HHCTN100E %4.4X:TCPNJE - Excess connection traffic. Connection state: %s\n",
                        devnum, tcpnje_state_text[tn->state]);
                }
                else if (errorcount100 == TCPNJE_MAX_ERRORCOUNT)
                {
                    DBGMSG(1, "HHCTN099W %4.4X:TCPNJE - repeating messages suppressed.\n",
                                devnum);
                }

                errorcount100++;
            }
        }

        /* Any incoming TCPNJE replies */
        if (selectcount && (tn->afd >=0) && FD_ISSET(tn->afd, &rfd))
        {
            DBGMSG(256, "HHCTN139D %4.4X:TCPNJE - active open TCPNJE protocol traffic. Connection state: %s\n",
                devnum, tcpnje_state_text[tn->state]);

            /* One of the causes of select() returning accounted for */
            selectcount--;

            /* Receive the incoming TCPNJE reply */
            rc = tcpnje_read(tn->afd, &tn->ttcactbuf, SIZEOF_TTC, tn);

            /* Did we get the complete TTC? If not, wait for more before doing anything */
            if (rc == 0)
            {
                /* Process the incoming TCPNJE ACK, NAK or whatever */
                tcpnje_process_reply(&tn->ttcactbuf, tn);

                /* Reset buffer pointer for next time something arrives */
                tn->ttcpasbuf.inptr.address = tn->ttcpasbuf.base.address;
            }
        }

        /* Has an incoming call arrived? */
        while (selectcount && (tn->listening > 1) && FD_ISSET(tn->lfd, &rfd))
        {
            /* This while block is really an if block with multiple exits */

            /* One of the causes of select() returning accounted for */
            selectcount--;

            /* Incoming connection to listener.  Not much choice but to accept it */
            tempfd = accept(tn->lfd, (struct sockaddr *)&remaddr, &remlength);
            if (tempfd < 0)
            {
                DBGMSG(4, "HHCTN062E %4.4X:TCPNJE - incoming connection - accept failed: %s\n",
                        devnum, strerror(HSO_errno));
                break;
            }

            /* Try to find out where the call is coming from */
            if (remlength == sizeof(remaddr))
            {
                DBGMSG(128, "HHCTN008I %4.4X:TCPNJE - incoming connection from %s:%d\n",
                        devnum, inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port));
            }
            else
            {
                DBGMSG(128, "HHCTN063I %4.4X:TCPNJE - incoming connection\n",
                        devnum);
            }
            /* Check the line type & current operation */

            /* if DIAL=IN or DIAL=INOUT or DIAL=NO */
            if (tn->dialin || (tn->dialin + tn->dialout == 0))
            {
                /* Are we already dealing with an incoming connection? */
                if (tn->pfd >= 0)
                {
                    /* Let's deal with the existing one first - shouldn't take long anyway. */
                    DBGMSG(512, "HHCTN064W %4.4X:TCPNJE - rejecting incoming connection due to connection already in progress\n",
                            devnum);
                    close_socket(tempfd);
                    break;
                }

                /* Turn non-blocking I/O on */
                /* set socket to NON-blocking mode */
                rc = socket_set_blocking_mode(tempfd, 0);
                if (rc < 0)
                {
                   DBGMSG(4, "HHCTN065E %4.4X:TCPNJE - error setting socket for incoming call to non-blocking : %s\n",
                                    tn->dev->devnum, strerror(HSO_errno));
                   close_socket(tempfd);
                   break;
                }

                tn->pfd = tempfd;
                disable_nagle(tn->pfd);

                /* Don't mess up any existing connection in case this one is not for us or doesn't work out */
                if (tn->state == TCPLISTEN) tn->state = TCPCONPAS;

                /* Prepare to receive incoming TCPNJE OPEN */
                tn->ttcpasbuf.inptr.address = tn->ttcpasbuf.base.address;

                /* if this is a leased line, accept the */
                /* call anyway                          */
                if (tn->dialin == 0)
                {
                   break;
                }
            }
            /* All other cases : just reject the call */
            DBGMSG(512, "HHCTN066W %4.4X:TCPNJE - rejecting unexpected incoming call\n",
                    devnum);
            close_socket(tempfd);

            break;
        }

        /* All the causes of select() returning should be dealt with by now */
        if (selectcount)
        {
            if (errorcount067 < TCPNJE_MAX_ERRORCOUNT)
            {
                /* Something unexpected has gone wrong, as opposed to something expected */

                DBGMSG(1, "HHCTN067E %4.4X:TCPNJE - possible logic error.  Outstanding count from select(): %d\n",
                            devnum, selectcount);

                /* Lets try to diagnose some possible causes of this anomaly */

                if ((tn->sfd >= 0) && FD_ISSET(tn->sfd, &wfd))
                    DBGMSG(1, "HHCTN068W %4.4X:TCPNJE - unexpected return from select() due to write event on data connection\n",
                            devnum);

                if ((tn->pfd >= 0) && FD_ISSET(tn->pfd, &rfd))
                    DBGMSG(1, "HHCTN069W %4.4X:TCPNJE - unexpected connection traffic received on incoming connection\n",
                            devnum);

                if ((tn->afd >=0) && FD_ISSET(tn->afd, &rfd))
                    DBGMSG(1, "HHCTN070W %4.4X:TCPNJE - unexpected connection traffic received on outgoing connection\n",
                            devnum);

                if ((tn->sfd >= 0) && FD_ISSET(tn->sfd, &rfd))
                    DBGMSG(1, "HHCTN071W %4.4X:TCPNJE - traffic received on data connection when not in connected state\n",
                            devnum);

                if ((tn->lfd >= 0) && FD_ISSET(tn->lfd, &rfd))
                    DBGMSG(1, "HHCTN072W %4.4X:TCPNJE - traffic received on listener port when not listening\n",
                            devnum);

                /* If it wasn't one of the above, it was probably a socket file descriptor
                   that was closed and set to -1.  Who knows which one and how though.      */
            }
            else if (errorcount067 == TCPNJE_MAX_ERRORCOUNT)
            {
                DBGMSG(1, "HHCTN099W %4.4X:TCPNJE - repeating messages suppressed.\n",
                            devnum);
            }

            errorcount067++;
        }
    }

    /* If thread is exiting due to an error, release any I/O thread waiting on it, otherwise it will hang forever */
    if ((tn->curpending != TCPNJE_PEND_IDLE) && (tn->curpending != TCPNJE_PEND_SHUTDOWN))
    {
        signal_condition(&tn->ipc);
    }

    tn->curpending = TCPNJE_PEND_CLOSED;
    /* Check if we already signaled the init process  */
    if (!init_signaled)
    {
        signal_condition(&tn->ipc);
    }
    /* TCPNJE is shutting down - terminate the thread */
    /* NOTE : the requestor was already notified upon */
    /*        detection of PEND_SHTDOWN. However      */
    /*        the requestor will only run when the    */
    /*        lock is released, because back          */
    /*        notification was made while holding     */
    /*        the lock                                */
    logmsg("HHCTN009I %4.4X:TCPNJE - networking thread terminated\n",
            devnum);
    tn->have_thread = 0;
    release_lock(&tn->lock);
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Wait for a condition from the thread                              */
/* MUST HOLD the TCPNJE lock                                         */
/*-------------------------------------------------------------------*/
static void tcpnje_wait(DEVBLK *dev)
{
    struct TCPNJE *tn;

    tn = (struct TCPNJE *) dev->commadpt;
    wait_condition(&tn->ipc, &tn->lock);
}

/*-------------------------------------------------------------------*/
/* Wakeup thread and then wait for it to do something                */
/* MUST HOLD the TCPNJE lock                                         */
/*-------------------------------------------------------------------*/
static int tcpnje_wakeup_and_wait(struct TCPNJE *tn, BYTE code)
{
    /* No point in bothering the thread if it is not there */
    if (tn->have_thread)
    {
        tcpnje_wakeup(tn, code);
        wait_condition(&tn->ipc, &tn->lock);
    }

    return tn->have_thread;
}

/*-------------------------------------------------------------------*/
/* Halt currently executing I/O command                              */
/*-------------------------------------------------------------------*/
static void    tcpnje_halt(DEVBLK *dev)
{
    struct TCPNJE *tn;

    if (!dev->busy)
    {
        return;
    }

    tn = (struct TCPNJE *) dev->commadpt;

    obtain_lock(&tn->lock);
    tcpnje_wakeup(tn, 1);
    /* Due to the mysteries of the host OS scheduling */
    /* the wait_condition may or may not exit after   */
    /* the CCW executor thread relinquishes control   */
    /* This however should not be of any concern      */
    /*                                                */
    /* but returning from the wait guarantees that    */
    /* the working thread will (or has) notified      */
    /* the CCW executor to terminate the current I/O  */
    wait_condition(&tn->ipc_halt, &tn->lock);
    release_lock(&tn->lock);
}
/* The following 5 MSG functions ensure only 1 (one)  */
/* hardcoded instance exist for the same numbered msg */
/* that is issued on multiple situations              */
static void msg013e(struct TCPNJE *tn, char *kw, char *kv)
{
        DBGMSG(2, "HHCTN013E %4.4X:TCPNJE - incorrect %s specification %s\n",
                tn->dev->devnum, kw, kv);
}
#if 0
static void msg015e(struct TCPNJE *tn, char *dialt, char *kw)
{
        DBGMSG(2, "HHCTN015E %4.4X:TCPNJE - missing parameter : DIAL=%s and %s not specified\n",
                tn->dev->devnum, dialt, kw);
}
static void msg016w017i(struct TCPNJE *tn, char *dialt, char *kw, char *kv)
{
        DBGMSG(2, "HHCTN016W %4.4X:TCPNJE - conflicting parameter : DIAL=%s and %s=%s specified\n",
                tn->dev->devnum, dialt, kw, kv);
        DBGMSG(2, "HHCTN017I %4.4X:TCPNJE - %s parameter ignored\n",
                tn->dev->devnum, kw);
}
#endif
static void msg073w(struct TCPNJE *tn, char *kv)
{
        DBGMSG(2, "HHCTN073W %4.4X:TCPNJE - DIAL/SWITCHED=%s is not currently supported by TCPNJE\n",
                tn->dev->devnum, kv);
}
static void msg074e(struct TCPNJE *tn, char *kw)
{
        DBGMSG(2, "HHCTN074E %4.4X:TCPNJE - %s has not been specified\n",
                tn->dev->devnum, kw);
}
/*-------------------------------------------------------------------*/
/* Device Initialisation                                             */
/*-------------------------------------------------------------------*/
static int tcpnje_init_handler(DEVBLK *dev, int argc, char *argv[])
{
    char thread_name[32];
    int i;
    u_int j;
    int rc;
    int pc; /* Parse code */
    int errcnt;
#if 0
    struct in_addr in_temp;
    char    *dialt;
    char        fmtbfr[64];
#endif
#if 0
    int ctospec;        /* CTO= Specified */
#endif
    struct TCPNJE *tn;
    union {
        int num;
        char text[80];
    } res;
        dev->devtype = 0x2703;
        if (dev->ccwtrace)
        {
                logmsg("HHCTN140D %4.4X:TCPNJE - initialisation starting\n",
                        dev->devnum);
        }

        /* Request that channel.c does not combine buffers of data chained write CCWs.
           Sometimes, it does it anyway, whether we like it or not :-(
           Unfortunately, no method is provided to find out whether it will or not :-(
                                                                                         */
        dev->cdwmerge = 0;

        rc = tcpnje_alloc_device(dev);
        if (rc < 0)
        {
                logmsg("HHCTN010E %4.4X:TCPNJE - initialisation not performed\n",
                        dev->devnum);
            return(-1);
        }
        tn = (struct TCPNJE *) dev->commadpt;

        DBGMSG(512, "HHCTN141D %4.4X:TCPNJE - Initialisation: Control block allocated\n",
                        dev->devnum);

        errcnt = 0;
        /*
         * Initialise ports & hosts
        */
        tn->pfd = -1;
        tn->afd = -1;
        tn->sfd = -1;
        tn->lport = TCPNJE_DEFAULT_PORT;
        tn->rport = TCPNJE_DEFAULT_PORT;
        tn->lhost = INADDR_ANY;
        tn->rhost = INADDR_NONE;
        tn->dialin = 0;
        tn->dialout = 0;
        tn->rto = 3000;        /* Read Time-Out in milis */
        tn->cto = 30000;       /* Connect Time-out in milis */
        tn->debug = TCPNJE_DEFAULT_DEBUG;      /* Debug level bitmask */
        tn->trace = TCPNJE_DEFAULT_TRACE;      /* Trace level bitmask */
        tn->maxidlewrites = TCPNJE_DEFAULT_KEEPALIVE;
        dev->bufsize = TCPNJE_DEFAULT_BUFSIZE;
        tn->listen = TCPNJE_DEFAULT_LISTEN;
        tn->connect = TCPNJE_DEFAULT_CONNECT;

        for(i = 0; i < 8; i++)
        {
            tn->lnode[i] = host_to_guest(' '); /* NJE local node name */
            tn->rnode[i] = host_to_guest(' '); /* NJE remote node name */
        }
#if 0
        ctospec = 0;
#endif

        if (argc < 1)
        {
            msg074e(tn, "device type");
            errcnt++;
        }
        else if (!strcmp(argv[0], "2703") || !strcasecmp(argv[0], "BSC") || !strcasecmp(argv[0], "BISYNC"))
        {
            dev->devtype = 0x2703;
        }
        else if (!strcmp(argv[0], "3088") || !strcasecmp(argv[0], "CTC") || !strcasecmp(argv[0], "CTCA"))
        {
            dev->devtype = 0x3088;
            DBGMSG(2, "HHCTN075E %4.4X:TCPNJE - device type %s is not yet supported\n",
                        dev->devnum, argv[0]);
            errcnt++;
        }
        else
        {
            DBGMSG(2, "HHCTN076E %4.4X:TCPNJE - device type %s is not valid\n",
                        dev->devnum, argv[0]);
            errcnt++;
        }

        for(i = 1; i < argc; i++)
        {
            pc = parser(ptab, argv[i], &res);
            if (pc < 0)
            {
                DBGMSG(2, "HHCTN011E %4.4X:TCPNJE - error parsing %s\n",
                        dev->devnum, argv[i]);
                errcnt++;
                continue;
            }
            if (pc == 0)
            {
                DBGMSG(2, "HHCTN012E %4.4X:TCPNJE - unrecognized parameter %s\n",
                        dev->devnum, argv[i]);
                errcnt++;
                continue;
            }
            switch(pc)
            {
                case TCPNJE_KW_LPORT:
                    rc = tcpnje_getport(res.text);
                    if (rc < 0)
                    {
                        errcnt++;
                        msg013e(tn, "LPORT", res.text);
                        break;
                    }
                    tn->lport = rc;
                    break;
                case TCPNJE_KW_LHOST:
                    if (strcmp(res.text, "*") == 0)
                    {
                        tn->lhost = INADDR_ANY;
                        break;
                    }
                    rc = tcpnje_getaddr(&tn->lhost, res.text);
                    if (rc != 0)
                    {
                        msg013e(tn, "LHOST", res.text);
                        errcnt++;
                    }
                    break;
                case TCPNJE_KW_RPORT:
                    rc = tcpnje_getport(res.text);
                    if (rc < 0)
                    {
                        errcnt++;
                        msg013e(tn, "RPORT", res.text);
                        break;
                    }
                    tn->rport = rc;
                    break;
                case TCPNJE_KW_RHOST:
                    if (strcmp(res.text, "*") == 0)
                    {
                        tn->rhost = INADDR_NONE;
                        break;
                    }
                    rc = tcpnje_getaddr(&tn->rhost, res.text);
                    if (rc != 0)
                    {
                        msg013e(tn, "RHOST", res.text);
                        errcnt++;
                    }
                    break;
                case TCPNJE_KW_READTO:
                    tn->rto = atoi(res.text);
                    break;
                case TCPNJE_KW_CONNECTTO:
                    tn->cto = atoi(res.text);
#if 0
                    ctospec = 1;
#endif
                    break;
                case TCPNJE_KW_KEEPALIVE:
                    tn->maxidlewrites = atoi(res.text);
                    break;
                case TCPNJE_KW_SWITCHED:
                case TCPNJE_KW_DIAL:
                    if (strcasecmp(res.text, "yes") == 0 || strcmp(res.text, "1") == 0 || strcasecmp(res.text, "inout") == 0)
                    {
                        msg073w(tn, res.text);
                        tn->dialin = 1;
                        tn->dialout = 1;
                        break;
                    }
                    if (strcasecmp(res.text, "no") == 0 || strcmp(res.text, "0") == 0)
                    {
                        tn->dialin = 0;
                        tn->dialout = 0;
                        break;
                    }
                    if (strcasecmp(res.text, "in") == 0)
                    {
                        msg073w(tn, res.text);
                        tn->dialin = 1;
                        tn->dialout = 0;
                        break;
                    }
                    if (strcasecmp(res.text, "out") == 0)
                    {
                        msg073w(tn, res.text);
                        tn->dialin = 0;
                        tn->dialout = 1;
                        break;
                    }
                    DBGMSG(2, "HHCTN014E %4.4X:TCPNJE - incorrect switched/dial specification %s; defaulting to DIAL=NO\n",
                            dev->devnum, res.text);
                    tn->dialin = 0;
                    tn->dialout = 0;
                    break;
                case TCPNJE_KW_LNODE:
                    if (strlen(res.text) > 8)
                    {
                        msg013e(tn, "LNODE", res.text);
                        errcnt++;
                        break;
                    }
                    for(j = 0; j < strlen(res.text); j++)
                    {
                        tn->lnode[j] = host_to_guest(toupper((unsigned char)res.text[j]));
                    }
                    break;
                case TCPNJE_KW_RNODE:
                    if (strlen(res.text) > 8)
                    {
                        msg013e(tn, "RNODE", res.text);
                        errcnt++;
                        break;
                    }
                    for(j = 0; j < strlen(res.text); j++)
                    {
                        tn->rnode[j] = host_to_guest(toupper((unsigned char)res.text[j]));
                    }
                    break;
                case TCPNJE_KW_DEBUG:
                    if (atoi(res.text) < 1)
                    {
                        msg013e(tn, "DEBUG", res.text);
                        errcnt++;
                        break;
                    }
                    tn->debug = atoi(res.text);
                    break;
                case TCPNJE_KW_TRACE:
                    tn->trace = atoi(res.text);
                    break;
                case TCPNJE_KW_BUFSIZE:
                    if (atoi(res.text) < 1024)
                    {
                        msg013e(tn, "BUFSIZE", res.text);
                        errcnt++;
                        break;
                    }
                    dev->bufsize = atoi(res.text);
                    break;
                case TCPNJE_KW_LISTEN:
                    tn->listen = atoi(res.text);
                    break;
                case TCPNJE_KW_CONNECT:
                    tn->connect = atoi(res.text);
                    break;
                default:
                    break;
            }
        }

#if 0
        /* lport, rport and lhost are defaulted and rhost is never essential (incoming connections will work)
           so these checks are not required */

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
        switch(tn->dialin + tn->dialout * 2)
        {
                case 0:
                    dialt = "NO";
                    break;
                case 1:
                    dialt = "IN";
                    break;
                case 2:
                    dialt = "OUT";
                    break;
                case 3:
                    dialt = "INOUT";
                    break;
                default:
                    dialt = "*ERR*";
                    break;
        }
        switch(tn->dialin + tn->dialout * 2)
        {
            case 0: /* DIAL = NO */
                if (tn->lport == 0)
                {
                    msg015e(tn, dialt, "LPORT");
                    errcnt++;
                }
                if (tn->rport == 0)
                {
                    msg015e(tn, dialt, "RPORT");
                    errcnt++;
                }
                if (tn->rhost == INADDR_NONE)
                {
                    msg015e(tn, dialt, "RHOST");
                    errcnt++;
                }
                break;
            case 1: /* DIAL = IN */
            case 3: /* DIAL = INOUT */
                if (tn->lport == 0)
                {
                    msg015e(tn, dialt, "LPORT");
                    errcnt++;
                }
                if (tn->rport != 0)
                {
                    snprintf(fmtbfr, sizeof(fmtbfr), "%d", tn->rport);
                    msg016w017i(tn, dialt, "RPORT", fmtbfr);
                }
                if (tn->rhost != INADDR_NONE)
                {
                    in_temp.s_addr = tn->rhost;
                    msg016w017i(tn, dialt, "RHOST", inet_ntoa(in_temp));
                    tn->rhost = INADDR_NONE;
                }
                break;
            case 2: /* DIAL = OUT */
                if (tn->lport != 0)
                {
                    snprintf(fmtbfr, sizeof(fmtbfr), "%d", tn->lport);
                    msg016w017i(tn, dialt, "LPORT", fmtbfr);
                    tn->lport = 0;
                }
                if (tn->rport != 0)
                {
                    snprintf(fmtbfr, sizeof(fmtbfr), "%d", tn->rport);
                    msg016w017i(tn, dialt, "RPORT", fmtbfr);
                    tn->rport = 0;
                }
                if (tn->lhost != INADDR_ANY)    /* Actually it's more like INADDR_NONE */
                {
                    in_temp.s_addr = tn->lhost;
                    msg016w017i(tn, dialt, "LHOST", inet_ntoa(in_temp));
                    tn->lhost = INADDR_ANY;
                }
                if (tn->rhost != INADDR_NONE)
                {
                    in_temp.s_addr = tn->rhost;
                    msg016w017i(tn, dialt, "RHOST", inet_ntoa(in_temp));
                    tn->rhost = INADDR_NONE;
                }
                break;
        }
#endif
        if (tn->rnode[0] == host_to_guest(' '))
        {
            msg074e(tn, "RNODE");
            errcnt++;
        }
        if (tn->lnode[0] == host_to_guest(' '))
        {
            msg074e(tn, "LNODE");
            errcnt++;
        }
        if (errcnt > 0)
        {
            DBGMSG(2, "HHCTN021I %4.4X:TCPNJE - initialisation failed due to previous errors\n",
                    dev->devnum);
            return -1;
        }

        /* Allocate device buffers */

        tn->ttcpasbuf.size = SIZEOF_TTC;
        tn->ttcpasbuf.base.address = malloc(tn->ttcpasbuf.size);
        if (tn->ttcpasbuf.base.address == NULL)
        {
            logmsg("HHCTN024E %4.4X:TCPNJE - memory allocation failure for TCPNJE TTC passive open buffer\n",
                    dev->devnum);
            return -1;
        }

        tn->ttcactbuf.size = SIZEOF_TTC;
        tn->ttcactbuf.base.address = malloc(tn->ttcactbuf.size);
        if (tn->ttcactbuf.base.address == NULL)
        {
            logmsg("HHCTN025E %4.4X:TCPNJE - memory allocation failure for TCPNJE TTC active open buffer\n",
                    dev->devnum);
            return -1;
        }

        tn->tcpinbuf.size = dev->bufsize;
        tn->tcpinbuf.base.address = malloc(tn->tcpinbuf.size);
        if (tn->tcpinbuf.base.address == NULL)
        {
            logmsg("HHCTN026E %4.4X:TCPNJE - memory allocation failure for TCPNJE TTB/TTR input buffer\n",
                    dev->devnum);
            return -1;
        }

        tn->tcpoutbuf.size = dev->bufsize;
        tn->tcpoutbuf.base.address = malloc(tn->tcpoutbuf.size);
        if (tn->tcpoutbuf.base.address == NULL)
        {
            logmsg("HHCTN027E %4.4X:TCPNJE - memory allocation failure for TCPNJE TTB/TTR output buffer\n",
                    dev->devnum);
            return -1;
        }

        dev->numsense = 2;
        memset(dev->sense, 0, sizeof(dev->sense));

        /* Initialise various flags & statuses */
        tn->enabled = 0;
        tn->state = CLOSED;
        dev->fd = 100;    /* Ensures 'close' function called by Hercules at device detach time */

        /* Initialize the device identifier bytes */
        dev->numdevid = sysblk.legacysenseid ? 7 : 0;
        dev->devid[0] = 0xFF;
        dev->devid[1] = dev->devtype >> 8;
        dev->devid[2] = dev->devtype & 0xFF;
        dev->devid[3] = 0x00;
        dev->devid[4] = dev->devtype >> 8;
        dev->devid[5] = dev->devtype & 0xFF;
        dev->devid[6] = 0x00;

        /* Initialize the TCPNJE lock */
        initialize_lock(&tn->lock);

        /* Initialise thread->I/O & halt initiation EVB */
        initialize_condition(&tn->ipc);
        initialize_condition(&tn->ipc_halt);

        /* Allocate I/O -> Thread signaling pipe */
        if (create_pipe( tn->pipe ) < 0)
        {
            // "Error in function %s: %s"
            WRMSG( HHC04000, "W", "create_pipe", strerror( errno ));
        }

#if !defined(HYPERION_DEVHND_FORMAT)
        /* Point to the halt routine for HDV/HIO/HSCH handling */
        dev->halt_device = tcpnje_halt;
#endif /* !HYPERION_DEVHND_FORMAT */

        /* Obtain the TCPNJE lock */
        obtain_lock(&tn->lock);

        /* Indicate listen required if DIAL != OUT */
        if (tn->dialin ||
                (!tn->dialin && !tn->dialout))
        {
            tn->dolisten = 1;
        }
        else
        {
            tn->dolisten = 0;
        }

        /* Start the async worker thread */

        /* Set thread-name for debugging purposes */
        MSGBUF(thread_name,
                 "tcpnje %4.4X thread", dev->devnum);
        thread_name[sizeof(thread_name) - 1] = 0;

        tn->curpending = TCPNJE_PEND_TINIT;
        rc = create_thread(&tn->thread, DETACHED, tcpnje_thread, tn, thread_name);
        if (rc)
        {
            logmsg("HHCTN022E TCPNJE - error creating communiction thread: %s\n", strerror(rc));
            release_lock(&tn->lock);
            return -1;
        }
        tcpnje_wait(dev);
        if (tn->curpending != TCPNJE_PEND_IDLE)
        {
            DBGMSG(1, "HHCTN019E %4.4X:TCPNJE communication thread did not initialise\n",
                    dev->devnum);
            /* Release the TCPNJE lock */
            release_lock(&tn->lock);
            return -1;
        }
        tn->have_thread = 1;

        /* Release the TCPNJE lock */
        release_lock(&tn->lock);
        /* Indicate succesfull completion */
        return 0;
}

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void tcpnje_query_device(DEVBLK *dev, char **class,
                int buflen, char *buffer)
{
#if 0
    int dialstatus;
#endif
    struct in_addr intemp;
    struct TCPNJE *tn;
    char rnodestring[9], lnodestring[9];
    char    filename[ PATH_MAX + 1 ];

    tn = (struct TCPNJE *) dev->commadpt;

#if 0
    dialstatus = tn->dialin + tn->dialout * 2;
#endif
    intemp.s_addr = tn->rhost;

    BEGIN_DEVICE_CLASS_QUERY( "LINE", dev, class, buflen, buffer);

    snprintf(buffer, buflen, "TCPNJE %s %s RH=%s RP=%d RN=%s LP=%d LN=%s IN=%d OUT=%d OP=%s",
            tn->enabled ? "ENAB" : "DISA",
            tcpnje_state_text[tn->state],
#if 0
            dialstatus == 1 ? "IN" : (dialstatus == 2 ? "OUT" : "NO"),
#endif
            inet_ntoa(intemp),
            tn->rport,
            guest_to_host_string(rnodestring, sizeof(rnodestring), tn->rnode),
            tn->lport,
            guest_to_host_string(lnodestring, sizeof(lnodestring), tn->lnode),
            tn->inbytecount,
            tn->outbytecount,
            tcpnje_pendccw_text[tn->curpending]);
}

/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/* Invoked by HERCULES shutdown & DEVINIT processing                 */
/*-------------------------------------------------------------------*/
static int tcpnje_close_device(DEVBLK *dev)
{
    struct TCPNJE *tn;

    tn = (struct TCPNJE *) dev->commadpt;

    if (dev->ccwtrace)
    {
        DBGMSG(1, "HHCTN142D %4.4X:TCPNJE - closing down\n", dev->devnum);
    }

    /* Attempt to gracefully close connection to remote link partner */
    tcpnje_close(tn->sfd, tn);

    /* Terminate current I/O thread if necessary */
    if (dev->busy)
    {
        tcpnje_halt(dev);
    }

    /* Obtain the TCPNJE lock */
    obtain_lock(&tn->lock);

    /* Terminate worker thread if it is still up */
    if (tn->have_thread)
    {
        tn->curpending = TCPNJE_PEND_SHUTDOWN;
        tcpnje_wakeup_and_wait(tn, 0);
        tn->thread = (TID) - 1;
        tn->have_thread = 0;
    }


    /* Free all work storage */
    /* The TCPNJE lock will be released by the cleanup routine */
    tcpnje_clean_device(dev);

    /* Indicate to hercules the device is no longer opened */
    dev->fd = -1;

    if (dev->ccwtrace)
    {
        logmsg("HHCTN143D %4.4X:TCPNJE - closed down\n",
                dev->devnum);
    }
    return 0;
}
/*-------------------------------------------------------------------*/
/* TCPNJE flush - send accumulated output buffer                     */
/*                                                                   */
/* Add a null TTR to the end of the current output buffer and fill   */
/* in the values in the TTB at it's beginning.  Then send the        */
/* completed TCPNJE buffer out to the TCP/IP network.                */
/*                                                                   */
/* This routine should be called whenever the output buffer has got  */
/* sufficiently full that the next record to be written may not fit  */
/* in it.  It should be called for every record if an NJE signon has */
/* not yet happened in order that signon is not obstructed.  To      */
/* avoid excessive delays, it should also be called whenever RSCS    */
/* has finished sending out data for the moment.  This situation is  */
/* indicated when RSCS starts sending out DLE ACK0 sequences or null */
/* buffers.  If RSCS had real data to send it would send the data    */
/* instead of either of these items.                                 */
/*-------------------------------------------------------------------*/
static void tcpnje_flush(struct TCPNJE *tn)
{
    /* Increment idle writes counter in case this is one.  It will   */
    /* get cleared shortly if it turns out not to be.                */
    tn->idlewrites++;

    /* Is there actually anything there to send out or is it about   */
    /* time we sent an empty TCPNJE block for keepalive?             */
    if ((tn->tcpoutbuf.inptr.address != tn->tcpoutbuf.base.address) ||
         (tn->idlewrites > tn->maxidlewrites))
    {
        /* If the holdoutgoing flag is set, the current buffer has   */
        /* already begun to be transmitted but has not been fully    */
        /* sent yet.  Therefore the terminating TTR has already been */
        /* added and the TTB has been filled in.  Just signal the    */
        /* worker thread to have another go at completing the        */
        /* transmission of the buffer and hope for the best.         */
        if (!tn->holdoutgoing)
        {
            /* Add an all zeros TTR to the end of the output buffer  */
            tn->tcpoutbuf.inptr.block->record.ttr.flags = 0;
            tn->tcpoutbuf.inptr.block->record.ttr.unused = 0;
            tn->tcpoutbuf.inptr.block->record.ttr.length = 0;

            /* Account for the size of initial TTB and the final TTR */
            tn->tcpoutbuf.inptr.address += SIZEOF_TTB + SIZEOF_TTR;

            /* Populate the TTB at the start of the output buffer    */
            tn->tcpoutbuf.base.ttb->flags = 0;
            tn->tcpoutbuf.base.ttb->unused = 0;
            /* Store accumulated buffer count in network byte order  */
            tn->tcpoutbuf.base.ttb->length =
                htons(tn->tcpoutbuf.inptr.address - tn->tcpoutbuf.base.address);
            tn->tcpoutbuf.base.ttb->reserved = 0;

            /* Place output pointer at the start of the buffer       */
            tn->tcpoutbuf.outptr.address = tn->tcpoutbuf.base.address;
        }

        /* Reset keepalive counter */
        tn->idlewrites = 0;
        tn->curpending = TCPNJE_PEND_WRITE;

        /* Wake-up the worker thread and wait for WRITE to complete */
        tcpnje_wakeup_and_wait(tn, 0);

    }
    return;
}

/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
#if defined(HYPERION_DEVHND_FORMAT)
static void tcpnje_execute_ccw(DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual)
#else  /* HYPERION_DEVHND_FORMAT */
static void tcpnje_execute_ccw(DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U16 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U16 *residual)
#endif /* HYPERION_DEVHND_FORMAT */
{
struct  TCPNJE *tn;          /* Pointer to device dependent block    */
struct  TPB    *tpb;         /* Pointer to RSCS TPbuffer in iobuf    */
U32     num;                 /* Work : Actual CCW transfer count     */
U32     TTRlength;           /* TTR length in host byte order        */
BYTE    nullbuffer[] = {0x10, 0x02, 0x90, 0x8f, 0xcf, 0x00, 0x10, 0x26};
BYTE    signoff[] =    {0x10, 0x02, 0x90, 0x8f, 0xcf,
                                          0xf0, 0xc2, 0x00, 0x10, 0x26};
    UNREFERENCED(flags);
    UNREFERENCED(chained);
    UNREFERENCED(prevcode);
    UNREFERENCED(ccwseq);
    *residual = 0;

    tn = (struct TCPNJE *) dev->commadpt;
    /*
     * The following pointer is used in READ and WRITE to locate various
     * items in iobuf which normally contains an RSCS TPbuffer.
     */
    tpb = (struct TPB *)iobuf;
    /*
     * Obtain the TCPNJE lock
     */

    DBGMSG(1024, "HHCTN144D %4.4X:TCPNJE CCW Execute opcode %2.2X\n",
            dev->devnum, code);

    obtain_lock(&tn->lock);
    switch (code) {
        /*---------------------------------------------------------------*/
        /* CONTROL NO-OP                                                 */
        /*---------------------------------------------------------------*/
        case 0x03:
                DBGMSG(1024, "HHCTN145D %4.4X:TCPNJE CCW CONTROL NO-OP\n",
                        dev->devnum);

                *residual = 0;
                *unitstat = CSW_CE | CSW_DE;
                break;

        /*---------------------------------------------------------------*/
        /* BASIC SENSE                                                   */
        /*---------------------------------------------------------------*/
        case 0x04:
                DBGMSG(1024, "HHCTN146D %4.4X:TCPNJE CCW SENSE\n",
                        dev->devnum);

                num = count < dev->numsense ? count : dev->numsense;
                *more = count < dev->numsense ? 1 : 0;
                memcpy(iobuf, dev->sense, num);
                /* I'm assuming sense bytes should be cleared once read. */
                /* They don't seem to get cleared anywhere else and its  */
                /* very confusing when old sense bytes which no longer   */
                /* apply get presented on the Hercules console.          */
                memset(dev->sense, 0, dev->numsense);
                *residual = count - num;
                *unitstat = CSW_CE | CSW_DE;
                break;

        /*---------------------------------------------------------------*/
        /* SENSE ID                                                      */
        /*---------------------------------------------------------------*/
        case 0xE4:
                DBGMSG(1024, "HHCTN147D %4.4X:TCPNJE CCW SENSE ID\n",
                        dev->devnum);

                /* Calculate residual byte count */
                num = (count  <  dev->numdevid) ? count : dev->numdevid;
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
                DBGMSG(1024, "HHCTN148D %4.4X:TCPNJE CCW ENABLE\n",
                        dev->devnum);

                if (tn->dialin + tn->dialout * 2 == 2)
                {
                    /* Enable makes no sense on a dial out only line */
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0x2E; /* Simulate Failed Call In */
                    break;
                }
                if (tn->haltpending)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UX;
                    tn->haltpending = 0;
                    break;
                }
                if (!tn->enabled)
                {
                    tn->activeopendelay = 0;
                    tn->enabled = 1;
                }
                *unitstat = CSW_CE | CSW_DE;
                break;

        /*---------------------------------------------------------------*/
        /* DISABLE                                                       */
        /*---------------------------------------------------------------*/
        case 0x2F:
                DBGMSG(1024, "HHCTN149D %4.4X:TCPNJE CCW DISABLE\n",
                        dev->devnum);

                if (tn->state == CLOSED)
                {
                    *unitstat = CSW_CE | CSW_DE;
                    tn->enabled = 0;
                    break;
                }
                tn->curpending = TCPNJE_PEND_DISABLE;

                /* Tell worker thread to go execute DISABLE and wait for it to complete */
                if (!tcpnje_wakeup_and_wait(tn, 0))
                {
                    /* If thread is not running, indicate something is wrong */
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0;
                    break;
                }

                tn->enabled = 0;
                *unitstat = CSW_CE | CSW_DE;
                break;
        /*---------------------------------------------------------------*/
        /* SET MODE                                                      */
        /*---------------------------------------------------------------*/
        case 0x23:
                DBGMSG(1024, "HHCTN150D %4.4X:TCPNJE CCW SET MODE %s\n",
                        dev->devnum, iobuf[0] & 0x40 ? "EIB" : "NO EIB");

                num = 1;
                *residual = count - num;
                *unitstat = CSW_CE | CSW_DE;

                tn->eibmode = (iobuf[0] & 0x40) ? 1 : 0;
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
        /* NOTE:  DIAL is mostly as it was in commadpt with tweaks to    */
        /*        adapt it to the TCPNJE environment.  It is most        */
        /*        unlikely to function correctly without further work.   */
        /*        I will look into getting it working if interest is     */
        /*        expressed in making use of it.                         */
        /*---------------------------------------------------------------*/
        case 0x29:
                DBGMSG(1024, "HHCTN151D %4.4X:TCPNJE - CCW DIAL\n",
                        dev->devnum);

                /* The line must have dial-out capability */
                if (!tn->dialout)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_CR;
                    dev->sense[1] = 0x04;
                    break;
                }
                /* The line must be disabled */
                if (tn->enabled)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_CR;
                    dev->sense[1] = 0x05;
                    break;
                }

                DBGMSG(1, "HHCTN077W %4.4X:TCPNJE - DIAL operation is not currently supported by TCPNJE\n",
                        dev->devnum);

                num = count > sizeof(tn->dialdata) ? sizeof(tn->dialdata) : count;
                memcpy(tn->dialdata, iobuf, num);
                tn->curpending = TCPNJE_PEND_DIAL;

                /* Tell worker thread to DIAL and wait for it to complete */
                if (!tcpnje_wakeup_and_wait(tn, 0))
                {
                    /* If thread is not running, indicate something is wrong */
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0;
                    break;
                }

                *residual = count - num;
                if (tn->haltpending)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UX;
                    tn->haltpending = 0;
                    break;
                }
                if (tn->state < NJECONPRI)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0;
                    tn->enabled = 0;
                }
                else
                {
                    *unitstat = CSW_CE | CSW_DE;
                    tn->enabled = 1;
                }
                break;

        /*---------------------------------------------------------------*/
        /* READ                                                          */
        /*---------------------------------------------------------------*/
        case 0x02:
                DBGMSG(1024, "HHCTN152D %4.4X:TCPNJE CCW READ count %d\n",
                        dev->devnum, count);

                /*  Read strategy is complicated in order to avoid       */
                /*  delaying transmission of files being sent in one     */
                /*  direction while the link is idle in the other        */
                /*  direction, to avoid files being sent in opposite     */
                /*  directions at the same time interfering with each    */
                /*  other or getting delayed and to avoid wasting        */
                /*  resources in tight loops when the link is idle in    */
                /*  both directions.  It is also necessary to implement  */
                /*  flow control.  This is how it is supposed to work:   */
                /*                                                       */
                /*  - Check for any data remaining in input buffer from  */
                /*    a previous read.                                   */
                /*  - If no data, using a very short timeout, check for  */
                /*    incoming data not yet read from the TCP/IP stack.  */
                /*  - If no data and outgoing buffers have been written  */
                /*    out but not acknowledged, fake an acknowledgement. */
                /*  - If there was no outgoing data to acknowledge, try  */
                /*    reading from the network with a longer timeout.    */
                /*  - If the read times out and the link is connected    */
                /*    and signed on, fake an acknowledgement anyway.     */
                /*  - If link is not signed on, fail with timeout error. */
                /*                                                       */
                /*  For each buffer presented to RSCS, alter the FCS     */
                /*  according to whether outgoing data from RSCS can be  */
                /*  accepted at the moment.                              */

                /* Check the line is enabled */
                if (!tn->enabled)
                {
                    *residual = count;
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_CR;
                    dev->sense[1] = 0x06;
                    break;
                }

                /* Data lost condition cleared by read (if it was ever set)  */
                tn->datalostcond = 0;

                /* If link partner has gone away, tell RSCS to stop link.    */
                if (tn->signoff)
                {
                    num = count < sizeof(signoff) ? count : sizeof(signoff);
                    memcpy(iobuf, signoff, num);
                    if (tn->holdoutgoing)
                    {
                        /* Outgoing buffers are full.  Fix FCS to stop RSCS sending more */
                        tpb->fcs[0] = 0xc0;
                        tpb->fcs[1] = 0x80;
                    }

                    DBGMSG(2048, "HHCTN078I %4.4X:TCPNJE READ - inserting signoff BCB: %2.2X FCS: %2.2X%2.2X\n",
                            dev->devnum, tpb->bcb, tpb->fcs[0], tpb->fcs[1]);

                    if (count < sizeof(signoff))
                    {
                        *more = 1;
                    }
                    *residual = count - num;
                    *unitstat = CSW_CE | CSW_DE;
                    logdump("Read S", dev, iobuf, num);
                    tn->signoff = 0;
                    break;
                }

                /* Check if input is suspended due to receipt of an FCS from */
                /* RSCS with one or more streams disabled or the wait-a-bit  */
                /* bit set.  If so, don't pass any incoming data this time.  */
                if (tn->holdincoming)
                {
                    /* Reply to RSCS with a null buffer instead of real data.*/
                    /* Use a null buffer instead of DLE ACK0 in order that   */
                    /* we do not inadvertently tell RSCS to send more data   */
                    /* out when outgoing data is held due to lack of buffers.*/
                    num = count < sizeof(nullbuffer) ? count : sizeof(nullbuffer);
                    memcpy(iobuf, nullbuffer, num);
                    if (tn->holdoutgoing)
                    {
                        /* Outgoing buffers are full.  Fix FCS to stop RSCS sending more */
                        tpb->fcs[0] = 0xc0;
                        tpb->fcs[1] = 0x80;
                    }

                    DBGMSG(2048, "HHCTN153D %4.4X:TCPNJE READ - inserting incoming null buffer BCB: %2.2X FCS: %2.2X%2.2X because incoming data is currently held\n",
                            dev->devnum, tpb->bcb, tpb->fcs[0], tpb->fcs[1]);

                    if (count < sizeof(nullbuffer))
                    {
                        *more = 1;
                    }
                    *residual = count - num;
                    *unitstat = CSW_CE | CSW_DE;
                    logdump("Read H", dev, iobuf, num);
                    break;
                }

                /* Check for any remaining data in the input buffer from */
                /* a previous read operation.  There likely will be some */
                /* because each TTB can contain multiple TTRs.  We need  */
                /* to get these sent on to RSCS before we accept more    */
                /* data from the network, otherwise we will rapidly run  */
                /* out of buffer space.                                  */
                if (!tn->tcpinbuf.valid || (tn->tcpinbuf.valid &&
                   (tn->tcpinbuf.outptr.block->record.ttr.length == 0)))
                {
                    /* Nothing in the local buffer.  Do a quick    */
                    /* poll to see if there is any incoming data   */
                    /* waiting in the TCP/IP stack buffer.         */
                    tn->curpending = TCPNJE_PEND_READ;
                    /* Set minimum timeout to ensure response ASAP */
                    tn->timeout = -1;

                    /* Tell worker thread to get more data and wait for it to do this */
                    if (!tcpnje_wakeup_and_wait(tn, 0))
                    {
                        /* If thread is not running, indicate something is wrong */
                        *unitstat = CSW_CE | CSW_DE | CSW_UC;
                        dev->sense[0] = SENSE_IR;
                        dev->sense[1] = 0;
                        break;
                    }

                    /* Point to the first record in the new buffer */
                    if (tn->tcpinbuf.valid)
                    {
                        tn->tcpinbuf.outptr.address =
                            tn->tcpinbuf.base.address;
                    }

                    /* If the I/O was halted - indicate Unit Exception */
                    if (tn->haltpending)
                    {
                        *residual = count;
                        *unitstat = CSW_CE | CSW_DE | CSW_UX;
                        tn->haltpending = 0;
                        break;
                    }
                }

                /* If there wasn't any data waiting in incoming network  */
                /* buffers but we are connected and signed on, check if  */
                /* there are any outgoing buffers which have not yet     */
                /* been ACKed to RSCS.  If so, use a null buffer to      */
                /* present an ACK to RSCS and avoid delay due to timeout.*/

                if (!tn->tcpinbuf.valid || (tn->tcpinbuf.valid &&
                   (tn->tcpinbuf.outptr.block->record.ttr.length == 0)))
                {
                    if ((tn->state > NJEACKRCD) && (tn->ackcount > 0))
                    {
                        /* Present null buffer to RSCS to avoid timeout  */
                        num = count < sizeof(nullbuffer) ? count : sizeof(nullbuffer);
                        memcpy(iobuf, nullbuffer, num);
                        if (tn->holdoutgoing)
                        {
                            /* Outgoing buffers are full.  Fix FCS to stop RSCS sending more */
                            tpb->fcs[0] = 0xc0;
                            tpb->fcs[1] = 0x80;
                        }

                        DBGMSG(2048, "HHCTN154D %4.4X:TCPNJE READ - inserting incoming null buffer BCB: %2.2X FCS: %2.2X%2.2X in lieu of ACK\n",
                                dev->devnum, tpb->bcb, tpb->fcs[0], tpb->fcs[1]);

                        if (count < sizeof(nullbuffer))
                        {
                            *more = 1;
                        }
                        *residual = count - num;
                        *unitstat = CSW_CE | CSW_DE;
                        logdump("Read A", dev, iobuf, num);
                        /* Count this response as one ACK */
                        tn->ackcount--;
                        break;
                    }
                    else
                    {
                        /* If not connected and signed on or RSCS has not */
                        /* sent out anything that has not been ACKed, we  */
                        /* just have to wait for real data to arrive or   */
                        /* time out after a number of seconds.            */
                        tn->curpending = TCPNJE_PEND_READ;
                        /* Set normal read timeout (typically 3 seconds)  */
                        tn->timeout = tn->rto;

                        /* Tell worker thread to get more data.  Wait for it to do this. */
                        if (!tcpnje_wakeup_and_wait(tn, 0))
                        {
                            /* If thread is not running, indicate something is wrong */
                            *unitstat = CSW_CE | CSW_DE | CSW_UC;
                            dev->sense[0] = SENSE_IR;
                            dev->sense[1] = 0;
                            break;
                        }

                        /* Point to the first record in the new buffer    */
                        if (tn->tcpinbuf.valid)
                        {
                            tn->tcpinbuf.outptr.address =
                                tn->tcpinbuf.base.address;
                        }

                        /* If the I/O was halted - indicate Unit Exception */
                        if (tn->haltpending)
                        {
                            *residual = count;
                            *unitstat = CSW_CE | CSW_DE | CSW_UX;
                            tn->haltpending = 0;
                            break;
                        }
                    }
                }

                /* If there is still no data is present - some seconds    */
                /* have passed without receiving data from the network.   */
                if (!tn->tcpinbuf.valid || (tn->tcpinbuf.valid &&
                   (tn->tcpinbuf.outptr.block->record.ttr.length == 0)))
                {
                    /* If we are connected and signed on, as last resort  */
                    /* fake an incoming DLE ACK0 or null buffer instead   */
                    /* of failing with a timeout which will cause grief.  */
                    if (tn->state > NJEACKRCD)
                    {
                        /* Present null buffer to RSCS to avoid timeout  */
                        num = count < sizeof(nullbuffer) ? count : sizeof(nullbuffer);
                        memcpy(iobuf, nullbuffer, num);
                        if (tn->holdoutgoing)
                        {
                            /* Outgoing buffers are full.  Fix FCS to stop RSCS sending more */
                            tpb->fcs[0] = 0xc0;
                            tpb->fcs[1] = 0x80;
                        }

                        DBGMSG(2048, "HHCTN155D %4.4X:TCPNJE READ - inserting incoming null buffer BCB: %2.2X FCS: %2.2X%2.2X in lieu of timeout\n",
                                    dev->devnum, tpb->bcb, tpb->fcs[0], tpb->fcs[1]);

                        if (count < sizeof(nullbuffer))
                        {
                            *more = 1;
                        }
                        *residual = count - num;
                        *unitstat = CSW_CE | CSW_DE;
                        logdump("Read T", dev, iobuf, num);
                    }
                    else
#if 0
                    {
                        /* No options left. Fail with timeout. */
                        *residual = count;
                        *unitstat = CSW_DE | CSW_CE | CSW_UC;
                        dev->sense[0] = 0x01;
                        dev->sense[1] = 0xe3;
                    }
#else
                    {
                        /* Try replying with DLE NAK (somewhat randomly chosen as something
                           that is not DLE ACK0) instead of timing out.  This results in
                           less whinges on the Hercules console due to device unit checks
                           without making RSCS think it has successfully connected.        */

                        num = count < 2 ? count : 2;
                        memcpy(iobuf, DLE NAK, num);

                        if (count < 2)
                        {
                            *more = 1;
                        }
                        *residual = count - num;
                        *unitstat = CSW_CE | CSW_DE;
                        logdump("Read N", dev, iobuf, num);
                    }
#endif
                    break;
                }

                /* At this point there is data available.  */
                /* Get the length of the next record.      */
                TTRlength = ntohs(tn->tcpinbuf.outptr.block->record.ttr.length);

                /* Now copy the actual record from the input buffer into iobuf */
                num = count < TTRlength ? count : TTRlength;
                memcpy(iobuf, &tn->tcpinbuf.outptr.block->record.tpb, num);

                /* Check if we snagged an incoming NCC J response signon  */
                /* record and modify the connection state accordingly.    */
                /* Also check NCC I initial signon records in case the    */
                /* other end has requested use of the PREPARE mode option */
                /* which is not currently supported and if so, reset the  */
                /* bit corresponding to that option in case the RSCS at   */
                /* our end decides to go with it.  If the same option is  */
                /* set in an incoming J record, something is badly wrong. */
                /* Also take note of the buffer size specified on an      */
                /* incoming J record as this is the buffer size the two   */
                /* link ends have negotiated to use.  */
                if ((TTRlength > 6) && (tn->tcpinbuf.outptr.block->record.tpb.rcb == 0xf0))
                {
                    if ((tn->tcpinbuf.outptr.block->record.tpb.srcb == 0xc9) &&
                       (((tn->state == NJEACKSNT) && !tn->synnaksent) ||
                        ((tn->state == NJEACKRCD) && tn->synnaksent)))
                    {
                        char nodestring[9];

                        /* SRCB == I Initial signon record received */
                        if (TTRlength > (5 + 0x0b))
                        {
                            DBGMSG(64, "HHCTN079I %4.4X:TCPNJE - received initial signon request for RSCS link %s - connection state %s\n",
                                dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring),
                                tn->tcpinbuf.outptr.block->record.tpb.nccinode), tcpnje_state_text[tn->state]);
                        }
                        if (TTRlength > (5 + 0x13))
                        {
                            U16 tpbufsize;

                            /* The following contortion is due to improper alignment in I/J signon records */
                            tpbufsize = (tn->tcpinbuf.outptr.block->record.tpb.nccibfsz[0] << 8)
                                       + tn->tcpinbuf.outptr.block->record.tpb.nccibfsz[1];
                            DBGMSG(64, "HHCTN080I %4.4X:TCPNJE - received TPbuffer size requested by RSCS link %s: %d\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring),
                                    tn->tcpinbuf.outptr.block->record.tpb.nccinode), tpbufsize);
                        }
                        /* If the other end has requested the PREPARE feature, reset it as we can't do that */
                        if ((TTRlength > (5 + 0x25)) && (tn->tcpinbuf.outptr.block->record.tpb.nccifeat[0] & 0x80))
                        {
                            tn->tcpinbuf.outptr.block->record.tpb.nccifeat[0] &= 0x7f;
                            DBGMSG(64, "HHCTN081I %4.4X:TCPNJE - resetting request by RSCS link %s for unsupported PREPARE protocol\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring),
                                    tn->tcpinbuf.outptr.block->record.tpb.nccinode));
                        }
                    }
                    else if ((tn->tcpinbuf.outptr.block->record.tpb.srcb == 0xd1) &&
                            (((tn->state == NJEACKRCD) && !tn->synnakreceived) ||
                             ((tn->state == NJEACKSNT) && tn->synnakreceived)))

                    {
                        char nodestring[9];

                        /* SRCB == J Response signon record received */
                        tn->state = NJECONPRI;
                        if (TTRlength > (5 + 0x0b))
                        {
                            DBGMSG(64, "HHCTN082I %4.4X:TCPNJE - received response signon for RSCS link %s - connection state %s\n",
                                dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring),
                                tn->tcpinbuf.outptr.block->record.tpb.nccinode), tcpnje_state_text[tn->state]);
                        }
                        if (TTRlength > (5 + 0x13))
                        {
                            /* The following contortion is due to improper alignment in I/J signon records */
                            tn->tpbufsize = (tn->tcpinbuf.outptr.block->record.tpb.nccibfsz[0] << 8)
                                                    + tn->tcpinbuf.outptr.block->record.tpb.nccibfsz[1];
                            DBGMSG(64, "HHCTN083I %4.4X:TCPNJE - received TPbuffer size negotiated by RSCS link %s: %d\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring),
                                    tn->tcpinbuf.outptr.block->record.tpb.nccinode), tn->tpbufsize);
                        }
                        /* If the other end set the prepare flag, it's going against our request */
                        if ((TTRlength > (5 + 0x25)) && (tn->tcpinbuf.outptr.block->record.tpb.nccifeat[0] & 0x80))
                        {
                            DBGMSG(16, "HHCTN084E %4.4X:TCPNJE - attempt by RSCS link %s to force use of unsupported PREPARE protocol\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring),
                                    tn->tcpinbuf.outptr.block->record.tpb.nccinode));
                            tn->tcpinbuf.outptr.block->record.tpb.nccifeat[0] &= 0x7f;
                        }
                    }
                }

                /* If this record contains an FCS, the VMNET equipvelant at the other end will have acted  */
                /* on it already.  If it requests transmission be stopped and we pass it on to our RSCS,   */
                /* we will get all hung up so we should set the FCS to cause RSCS to proceed as normal.    */
                /* On the other hand, if our outgoing TCP/IP buffers are full, we need to set the FCS so   */
                /* that RSCS doesn't attempt to send out any more data for now.                            */
                if (TTRlength >= 6)
                {
                    if (tn->holdoutgoing)
                    {
                        tpb->fcs[0] = 0xc0;
                        tpb->fcs[1] = 0x80;
                    }
                    else
                    {
                        tpb->fcs[0] = 0x8f;
                        tpb->fcs[1] = 0xcf;
                    }
                    /* Also check for FASTOPEN flag set and if so, note which stream it has been applied to */
                    if ((tn->tcpinbuf.outptr.block->record.ttr.flags&0x80) && (num >= 7))
                    {
                        DBGMSG(16, "HHCTN085W %4.4X:TCPNJE READ - Link partner issued FASTOPEN request for stream %2.2X.  Implementation at this end is incomplete.\n",
                                dev->devnum, tpb->srcb);
                        tn->fastopen = tpb->srcb;
                    }
                }

                /* Point to next record after this one */
                tn->tcpinbuf.outptr.address += SIZEOF_TTR + TTRlength;

                /* Count the data received.  Some overhead is included */
                /* and some data is not counted due to compression.    */

                if (num > 2)
                {
                    tn->inbytecount += num;
                    tn->inbuffcount++;
                }

                /* If the buffer started with DLE STX, fake DLE ETB after it. */
                if (!memcmp(tpb->start, DLE STX, 2) && ((num + 2) <= count))
                {
                    DBGMSG(4096, "HHCTN170D %4.4X:TCPNJE READ - adding DLE ETB to end of buffer\n", dev->devnum);

                    iobuf[num++] = *DLE;
                    iobuf[num++] = *ETB;
                    TTRlength += 2;
                }

                if (count < TTRlength)
                {
                    *more = 1;
                }

                *residual = count - num;
                *unitstat = CSW_CE | CSW_DE;

                logdump("Read  ", dev, iobuf, num);

                /* Check for SYN NAK indicating the other end wants to be secondary */
                if ((TTRlength == 2) && !memcmp(tpb->start, SYN NAK, 2))
                {
                    tn->synnakreceived = 1;

                    DBGMSG(4096, "HHCTN169D %4.4X:TCPNJE READ - incoming SYN NAK - Connection state: %s\n",
                           dev->devnum, tcpnje_state_text[tn->state]);

                    break;
                }

                /* Receipt of this buffer will serve as ACK for an outgoing buffer */
                if (tn->ackcount > 0)
                {
                    tn->ackcount--;
                }

                break;

        /*---------------------------------------------------------------*/
        /* WRITE                                                         */
        /*---------------------------------------------------------------*/
        case 0x01:
                DBGMSG(1024, "HHCTN156D %4.4X:TCPNJE CCW WRITE count %d\n",
                        dev->devnum, count);

                /* Write strategy is as follows:                         */
                /*                                                       */
                /* - Check FCS in buffer coming from RSCS and flag it    */
                /*   if RSCS does not want incoming data for now.        */
                /* - Drop outgoing null buffers, SYN SYN SYN SYN or      */
                /*   DLE ETB sequences. Also drop DLE ACK0 if signed on. */
                /* - If the data to be written doesn't fit in the output */
                /*   buffer, send the buffer.                            */
                /* - Put the data to be sent in the buffer.              */
                /* - If the remaining space in the buffer is less than   */
                /*   the size of a TPbuffer, send the buffer.            */
                /* - If RSCS has gone idle, send the buffer.             */
                /* - If a keepalive is due, send the (empty) buffer.     */

                logdump("Write ", dev, iobuf, count);

                /* Check if the line has been enabled */
                if (!tn->enabled)
                {
                    *residual = count;
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_CR;
                    dev->sense[1] = 0;
                    break;
                }

#if defined(TCPNJE_CDWMERGE_KLUDGE)
                /* Check for a write of SYN SYN SYN SYN that channel.c has already appended
                   a chained write to, despite being told not to by dev->cdwmerge = 0.       */
                if  ((count > 4) && !memcmp(iobuf, SYN SYN SYN SYN, 4))
                {
                    DBGMSG(4096, "HHCTN157D %4.4X:TCPNJE WRITE - kludge dropping outgoing SYN SYN SYN SYN. Connection state: %s\n",
                            dev->devnum, tcpnje_state_text[tn->state]);

                    tpb = (struct TPB *)&iobuf[4];
                    count -= 4;
                }
#endif /* TCPNJE_CDWMERGE_KLUDGE */

                /* Perform various checks on outgoing buffers that contain BCB and FCS bytes */
                if (count >= 6)
                {
                    /* Check FCS in case RSCS is trying to stop incoming data */
                    /* 8FCF => all is good.  Anything else => bad.            */
                    if ((tpb->fcs[0] == 0x8f) && (tpb->fcs[1] == 0xcf))
                    {
                        if (tn->holdincoming)
                        {
                            DBGMSG(2048, "HHCTN158D %4.4X:TCPNJE WRITE - Resuming incoming data due to outgoing FCS %2.2X%2.2X\n",
                                    dev->devnum, tpb->fcs[0], tpb->fcs[1]);
                        }
                        tn->holdincoming = 0;
                    }
                    else
                    {
                        if (!tn->holdincoming)
                        {
                            DBGMSG(2048, "HHCTN159D %4.4X:TCPNJE WRITE - Holding incoming data due to outgoing FCS %2.2X%2.2X\n",
                                    dev->devnum, tpb->fcs[0], tpb->fcs[1]);
                        }
                        tn->holdincoming = 1;
                    }
                    /* Is the wait-a-bit bit set? */
                    if (tpb->fcs[0] & 0x40)
                    {
                        tn->waitabit = 1;

                        DBGMSG(2048, "HHCTN160D %4.4X:TCPNJE WRITE - RSCS sent FCS %2.2X%2.2X with wait-a-bit bit set\n",
                                dev->devnum, tpb->fcs[0], tpb->fcs[1]);

#if 0
                        /* Ask the worker thread to wait a few seconds */
                        /* Holding up the write while we wait probably */
                        /* does not help RSCS to deal with incoming    */
                        /* data but it'll do until I think of          */
                        /* something better to do.                     */
                        tn->curpending = TCPNJE_PEND_WAIT;

                        /* Wake-up the worker thread */
                        tcpnje_wakeup_and_wait(tn, 0);

                        tn->waitabit = 0;
#endif
                    }
                    /* If this is a null buffer, no need to send it to the other end */
                    if (count == 6)
                    {
                        DBGMSG(2048, "HHCTN161D %4.4X:TCPNJE WRITE - Dropping outgoing null buffer BCB: %2.2X FCS: %2.2X%2.2X\n",
                                dev->devnum, tpb->bcb, tpb->fcs[0], tpb->fcs[1]);

                        *residual = 0;
                        *unitstat = CSW_CE | CSW_DE;
                        /* Flag that the next outgoing BCB will not be what the other end expects    */
                        tn->resetoutbcb = 1;

                        /* RSCS has no real data to send out if it sent a null buffer.  In order to  */
                        /* avoid causing delays, we should send the accumulated output buffer out    */
                        /* now instead of waiting around until RSCS has something else to send out.  */
                        tcpnje_flush(tn);
                        break;
                    }
                    /* If we have already dropped something containing a BCB, reset the BCB this time */
                    if (tn->resetoutbcb)
                    {
                        /* I am not very clear on how this works.  It appears that setting the reset */
                        /* flag in this BCB causes this BCB to not be flagged as in error if not in  */
                        /* sequence with the last one received and also causes this BCB to be become */
                        /* the BCB that RSCS expects to find in the next buffer it receives.         */
                        /* Anyway, if I set the reset flag and make the BCB count the same as the    */
                        /* last one received plus one, RSCS seems to be relatively happy.  If the    */
                        /* reset bit is already set by the time it gets here, all bets are off.  If  */
                        /* the ignore bit is set, it's probably not a problem (yet).                 */
                        if (tpb->bcb & 0x30)
                        {
                            if (tpb->bcb & 0x20)
                            {
                                DBGMSG(16, "HHCTN086W %4.4X:TCPNJE WRITE - Received BCB with reset bit set from RSCS: %2.2X\n",
                                        dev->devnum, tpb->bcb);
                            }
                        }
                        else
                        {
                            tpb->bcb = (tpb->bcb & 0xf0) | 0x20 | ((tpb->bcb + 1) & 0x0f);

                            DBGMSG(2048, "HHCTN162D %4.4X:TCPNJE WRITE - Resetting outgoing BCB to %2.2X\n",
                                    dev->devnum, tpb->bcb);
                        }
                        tn->resetoutbcb = 0;
                    }
                    /* Also check for "permssion to open stream" response  */
                    /* going out and suppress it if FASTOPEN flag was used */
                    /* on the "open stream" request.  This falls short of  */
                    /* the full response called for but it is better than  */
                    /* doing nothing at all.                               */
                    if ((count >= 7) && (tpb->rcb == 0xa0) && (tn->fastopen == tpb->srcb))
                    {
                        DBGMSG(2048, "HHCTN087I %4.4X:TCPNJE WRITE - Permission to open stream %2.2X suppressed due to preceeding FASTOPEN\n",
                                dev->devnum, tn->fastopen);
                        tn->fastopen = 0;
                        tn->resetoutbcb = 1;
                        *residual = 0;
                        *unitstat = CSW_CE | CSW_DE;
                        break;
                    }
                    /* Are we sending an outgoing NCC I initial signon     */
                    /* record? Did RSCS request the PREPARE feature?       */
                    /* If so, reset it as we are not currently able to     */
                    /* provide this feature.                               */

                    if ((tpb->rcb == 0xf0) && (tpb->srcb == 0xc9) &&
                       (((tn->state == NJEACKRCD) && !tn->synnakreceived) ||
                        ((tn->state == NJEACKSNT) && tn->synnakreceived)))
                    {
                        char nodestring[9];

                        if (count >= (5 + 0x0b))
                        {
                            DBGMSG(64, "HHCTN088I %4.4X:TCPNJE - sending initial signon for local RSCS name %s.  Connection state %s\n",
                                dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring), tpb->nccinode),
                                tcpnje_state_text[tn->state]);
                        }

                        if (count >= (5 + 0x14))
                        {
                            U16 tpbufsize;

                            /* The following contortion is required because */
                            /* of incorrect alignment in I/J signon records */
                            tpbufsize = (tpb->nccibfsz[0] << 8) + tpb->nccibfsz[1];
                            DBGMSG(64, "HHCTN089I %4.4X:TCPNJE - sending TPbuffer size requested by local RSCS name %s: %d\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring), tpb->nccinode), tpbufsize);
                        }

                        if ((count >= (5 + 0x25)) && (tpb->nccifeat[0] & 0x80))
                        {
                            DBGMSG(64, "HHCTN090I %4.4X:TCPNJE - resetting unsupported PREPARE feature requested by local RSCS name %s\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring), tpb->nccinode));
                            tpb->nccifeat[0] &= 0x7f;
                        }
                    }
                    /* Are we sending an outgoing NCC J response signon    */
                    /* record?  If so, note the change in connection state */
                    if ((tpb->rcb == 0xf0) && (tpb->srcb == 0xd1) &&
                       (((tn->state == NJEACKSNT) && !tn->synnaksent) ||
                        ((tn->state == NJEACKRCD) && tn->synnaksent)))
                    {
                        char nodestring[9];

                        tn->state = NJECONSEC;

                        if (count >= (5 + 0x0b))
                        {
                            DBGMSG(64, "HHCTN091I %4.4X:TCPNJE - sending response signon for local RSCS name %s.  Connection state %s\n",
                                dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring), tpb->nccinode),
                                tcpnje_state_text[tn->state]);
                        }

                        if (count >= (5 + 0x14))
                        {
                            /* The following contortion is required because */
                            /* of incorrect alignment in I/J signon records */
                            tn->tpbufsize = (tpb->nccibfsz[0] << 8) + tpb->nccibfsz[1];
                            DBGMSG(64, "HHCTN092I %4.4X:TCPNJE - sending TPbuffer size negotiated by local RSCS name %s: %d\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring), tpb->nccinode),
                                    tn->tpbufsize);
                        }
                        if ((count >= (5 + 0x25)) && (tpb->nccifeat[0] & 0x80))
                        {
                            DBGMSG(16, "HHCTN093W %4.4X:TCPNJE - resetting forced use of unsuppored PREPARE protocol by local RSCS name %s\n",
                                    dev->devnum, guest_to_host_string(nodestring, sizeof(nodestring), tpb->nccinode));
                            tpb->nccifeat[0] &= 0x7f;
                        }
                    }
                }
                /* Check if we have an opened path */
                if (tn->state < TCPCONACT)
                {
                    /* Is this an attempt to get connected? */
                    if ((count != 2) || (memcmp(tpb->start, SOH ENQ, 2) && memcmp(tpb->start, SYN NAK, 2)))
                    {
                        DBGMSG(64, "HHCTN094W %4.4X:TCPNJE WRITE - Attempting to send other than SOH ENQ or SYN NAK when not connected. Connection state: %s\n",
                                dev->devnum, tcpnje_state_text[tn->state]);
                        /* RSCS must be confused.  Send it a signoff at next read.  */
                        tn->signoff = 1;
                        /* Drop write as connecting now will only cause more confusion */
                        *residual = 0;
                        *unitstat = CSW_CE | CSW_DE;
                        break;
                    }

                    /* Not connected and sending SOH ENQ or SYN NAK. Try to connect. */
                    tn->curpending = TCPNJE_PEND_CONNECT;

                    /* Wakeup worker thread and wait for it to complete CONNECT */
                    if (!tcpnje_wakeup_and_wait(tn, 0))
                    {
                        /* If thread is not running, indicate something is wrong */
                        *unitstat = CSW_CE | CSW_DE | CSW_UC;
                        dev->sense[0] = SENSE_IR;
                        dev->sense[1] = 0;
                        break;
                    }

                    /* Did we connect? */
                    if (tn->state < NJEACKSNT)
                    {
                        /* Connect failed. Pretend the write worked */
                        *residual = 0;
                        *unitstat = CSW_CE | CSW_DE;
                        break;
                    }
                }
                /* Is this SYN SYN SYN SYN? */
                if  ((count == 4) && !memcmp(iobuf, SYN SYN SYN SYN, 4))
                {
                    DBGMSG(4096, "HHCTN157D %4.4X:TCPNJE WRITE - dropping outgoing SYN SYN SYN SYN. Connection state: %s\n",
                            dev->devnum, tcpnje_state_text[tn->state]);

                    /* Yes. Drop it on the floor.  It would get eaten at the far end anyway. */
                    *residual = 0;
                    *unitstat = CSW_CE | CSW_DE;
                    break;
                }
                /* Drop DLE ETB as these are not used in TCPNJE protocol. */
                /* They will be recreated at the receiving end using the  */
                /* TTR length. We could have RSCS neither send nor expect */
                /* them but then we would lose compatibility with the     */
                /* real(ish) 2703 (if we still have it).                  */
                if ((count == 2) && !memcmp(tpb->start, DLE ETB, 2))
                {
                    DBGMSG(4096, "HHCTN163D %4.4X:TCPNJE WRITE - dropping outgoing DLE ETB - connection state %s\n",
                            dev->devnum, tcpnje_state_text[tn->state]);

                    /* If we are connected and signed on, count this as an        */
                    /* outgoing buffer which should have an ACK faked for it so   */
                    /* the read following this write is not delayed by waiting    */
                    /* for a timeout when not idle.                               */
                    if (tn->state > NJEACKRCD)
                    {
                        tn->ackcount++;
                    }
                    *residual = 0;
                    *unitstat = CSW_CE | CSW_DE;
                    break;
                }
                /* If we are connected and signed on, we can take some shortcuts  */
                if (tn->state > NJEACKRCD)
                {
                    /* If we are attempting to send DLE ACK0 then drop it as      */
                    /* the other end will fake it for us.                         */
                    if ((count == 2) && !memcmp(tpb->start, DLE ACK0, 2))
                    {
                        DBGMSG(4096, "HHCTN164D %4.4X:TCPNJE WRITE - Dropping outgoing DLE ACK0 - Connection state %s\n",
                                dev->devnum, tcpnje_state_text[tn->state]);

                        *residual = 0;
                        *unitstat = CSW_CE | CSW_DE;

                        /* RSCS has no real data to send out if it sent DLE ACK0. */
                        /* In order to avoid causing delays, we should send the   */
                        /* accumulated output buffer out now instead of waiting   */
                        /* around until RSCS has something else to send out.      */
                        tcpnje_flush(tn);
                        break;
                    }
                }
                /* Is this an attempt to become primary when we have already accepted the other end as primary?  Usually */
                /* the end which connects out and sends the TCPNJE OPEN and receives the TCPNJE ACK then sends SOH ENQ   */
                /* in an attempt to become primary.  If the other end is in this state, we are in the NJE ACK sent state */
                /* and we want to avoid trying to become primary as well.  However, the other end can instead send       */
                /* SYN NAK in an attempt to become secondary.  So, if we have seen an incoming SYN NAK, allow us to send */
                /* out SOH ENQ to become primary.                                                                        */
                if  ((count == 2) && !memcmp(tpb->start, SOH ENQ, 2) && (tn->state == NJEACKSNT) && !tn->synnakreceived)
                {
                    /* Other end is going to be primary.  Drop data and pretend write succeeded. */

                    DBGMSG(4096, "HHCTN165D %4.4X:TCPNJE WRITE - dropping outgoing SOH ENQ - Connection state: %s\n",
                           dev->devnum, tcpnje_state_text[tn->state]);

                    *residual = 0;
                    *unitstat = CSW_CE | CSW_DE;
                    break;
                }

                /* Check for SYN NAK indicating this end wants to be secondary (this has not been tested) */
                if ((count == 2) && !memcmp(tpb->start, SYN NAK, 2) && (tn->state == NJEACKRCD) && !tn->synnaksent)
                {
                    tn->synnaksent = 1;

                    DBGMSG(4096, "HHCTN171D %4.4X:TCPNJE WRITE - outgoing SYN NAK - Connection state: %s\n",
                           dev->devnum, tcpnje_state_text[tn->state]);
                }

                /* Check if the I/O was interrupted */
                if (tn->haltpending)
                {
                    tn->haltpending = 0;
                    *residual = count;
                    *unitstat = CSW_CE | CSW_DE | CSW_UX;
                    break;
                }

                /* At this point, we have used up every possible excuse not to send data to the  */
                /* network except for putting it in a buffer and leaving it there to send later. */

                /* Is there room in the output buffer for this record */
                if (tn->tcpoutbuf.inptr.address - tn->tcpoutbuf.base.address +
                    2 * SIZEOF_TTR + count > tn->tcpoutbuf.size)
                {
                    /* No room in the buffer.  Send out the current buffer first.    */
                    tcpnje_flush(tn);
                }

                /* Is there room in the buffer now? If not, there may be a problem.  */
                if (tn->tcpoutbuf.inptr.address - tn->tcpoutbuf.base.address +
                    2 * SIZEOF_TTR + count > tn->tcpoutbuf.size)
                {
                    /* All we can do now is have the write fail.  Believe it or not, */
                    /* it appears that RSCS will cope and retry the same write again */
                    /* and all will still be well, at least for some RSCS versions.  */
                    /* I'm not looking forward to trying this with a CTC though.     */
                    DBGMSG(16, "HHCTN095E %4.4X:TCPNJE WRITE - outgoing record size %d will not fit in output buffer\n",
                            dev->devnum, count);

                    *residual = count;
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0;
                    break;
                }

                /* If the last buffer is still in the process of being sent, this    */
                /* record can't be put into it yet.  Hopefully this situation should */
                /* not arise because RSCS should already have been told to stop      */
                /* sending out data.  However, if it does, the only option is to     */
                /* fail.  See comments just above.                                   */

                if (!tn->holdoutgoing)
                {
                    /* Put TTR header for this record into the output buffer */
                    tn->tcpoutbuf.inptr.block->record.ttr.flags = 0;
                    tn->tcpoutbuf.inptr.block->record.ttr.unused = 0;
                    tn->tcpoutbuf.inptr.block->record.ttr.length = htons(count);

                    /* Put the actual data to be written into the output buffer */
                    memcpy(&tn->tcpoutbuf.inptr.block->record.tpb, tpb, count);
                    tn->tcpoutbuf.inptr.address += SIZEOF_TTR + count;

                    /* Count the data to be transmitted.  Some overhead is included */
                    /* and some data is not counted due to compression.    */

                    if (count > 2)
                    {
                        tn->outbytecount += count;
                        tn->outbuffcount++;
                    }

                    /* If we are not yet signed on, we must send the buffer now.     */
                    /* Same if RSCS has send a control function, signoff for example.*/
                    /* If we are signed on, we can accumulate records and not send   */
                    /* until RSCS stops sending out data for the moment.  If it      */
                    /* appears that the next record to be written might not fit in   */
                    /* the buffer, we should send the buffer out now in order to     */
                    /* take the opportunity to tell RSCS not to send more data out   */
                    /* before the next write happens when it is too late.            */
                    if ((tn->state < NJECONPRI) || (tpb->rcb == 0xf0))
                    {
                        tcpnje_flush(tn);
                    }
                    else if (tn->tcpoutbuf.inptr.address - tn->tcpoutbuf.base.address +
                        2 * SIZEOF_TTR + tn->tpbufsize > tn->tcpoutbuf.size)
                    {
                        DBGMSG(2048, "HHCTN166D %4.4X:TCPNJE WRITE - Sending outgoing buffer as it is nearly full\n",
                               dev->devnum);
                        tcpnje_flush(tn);
                    }
                }
                else
                {
                    DBGMSG(16, "HHCTN096W %4.4X:TCPNJE WRITE - outgoing record cannot be buffered as buffer is busy\n",
                            dev->devnum);
                    *residual = count;
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0;
                    break;
                }
                /* All bytes dealt with, one way or another - residual = 0 */
                *residual = 0;
                *unitstat = CSW_CE | CSW_DE;
                break;

        /*---------------------------------------------------------------*/
        /* PREPARE                                                       */
        /* NOTE : DO NOT SET RESIDUAL to 0 : Otherwise, channel.c        */
        /*        will reflect a channel prot check - residual           */
        /*        should indicate NO data was transfered for this        */
        /*        pseudo-read operation                                  */
        /*---------------------------------------------------------------*/
        /* NOTE:  PREPARE is mostly as it was in commadpt with tweaks to */
        /*        adapt to the TCPNJE environment.  It is unlikely to    */
        /*        function correctly without further effort but I don't  */
        /*        know how it works and I have no way to test it.        */
        /*---------------------------------------------------------------*/
        case 0x06:
                DBGMSG(1024, "HHCTN167D %4.4X:TCPNJE CCW PREPARE\n",
                        dev->devnum);

                *residual = count;
                /* PREPARE not allowed unless line is enabled */
                if (!tn->enabled)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_CR;
                    dev->sense[1] = 0x06;
                    break;
                }

                DBGMSG(1, "HHCTN097W %4.4X:TCPNJE - PREPARE operation is not supported by TCPNJE\n",
                        dev->devnum);

                /* If data is present, prepare ends immediately */
                if (tn->tcpinbuf.valid)
                {
                    *unitstat = CSW_CE | CSW_DE;
                    break;
                }

                /* Indicate to the worker thread to notify us when data arrives */
                tn->curpending = TCPNJE_PEND_PREPARE;

                /* Wakeup worker thread and wait for it to complete PREPARE */
                if (!tcpnje_wakeup_and_wait(tn, 0))
                {
                    /* If thread is not running, indicate something is wrong */
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0;
                    break;
                }

                /* If I/O was halted (this one happens often) */
                if (tn->haltpending)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UX;
                    tn->haltpending = 0;
                    break;
                }

                /* Check if the line is still connected */
                if (tn->state <= NJEACKSNT)
                {
                    *unitstat = CSW_CE | CSW_DE | CSW_UC;
                    dev->sense[0] = SENSE_IR;
                    dev->sense[1] = 0;
                    break;
                }

                /* Normal Prepare exit condition - data is present in the input buffer */
                *unitstat = CSW_CE | CSW_DE;
                break;

        /*---------------------------------------------------------------*/
        /* POLL Command                                                  */
        /*---------------------------------------------------------------*/
        case 0x09:
                DBGMSG(1024, "HHCTN168D %4.4X:TCPNJE CCW POLL\n",
                        dev->devnum);

                /* Changing ring buffers to linear buffers for TCPNJE    */
                /* and other modifications to the original commadpt      */
                /* required many changes to the implementation of POLL.  */
                /* POLL is not used by the RSCS in VM/370 so I thought   */
                /* it better to take it out than expend a lot of effort  */
                /* making a half baked attempt to support something I    */
                /* I don't understand at all and can't test.             */
                DBGMSG(1, "HHCTN098E %4.4X:TCPNJE - POLL operation is not supported by TCPNJE\n",
                        dev->devnum);
                /* FALLTHRU */

        default:
        /*---------------------------------------------------------------*/
        /* INVALID OPERATION                                             */
        /*---------------------------------------------------------------*/

            DBGMSG(1024, "HHCTN098E %4.4X:TCPNJE - Invalid CCW opcode %2.2X\n",
                    dev->devnum, code);

            /* Set command reject sense byte, and unit check status */
            *unitstat = CSW_CE + CSW_DE + CSW_UC;
            dev->sense[0] = SENSE_CR;
            dev->sense[1] = 0;
            break;

    }
    release_lock(&tn->lock);
}

/*---------------------------------------------------------------*/
/* DEVICE FUNCTION POINTERS                                      */
/*---------------------------------------------------------------*/

#if defined(OPTION_DYNAMIC_LOAD)
static
#endif
DEVHND tcpnje_device_hndinfo = {
        &tcpnje_init_handler,          /* Device Initialisation      */
        &tcpnje_execute_ccw,           /* Device CCW execute         */
        &tcpnje_close_device,          /* Device Close               */
        &tcpnje_query_device,          /* Device Query               */
#if defined(HYPERION_DEVHND_FORMAT)
        NULL,                          /* Device Extended Query      */
#endif /* HYPERION_DEVHND_FORMAT */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
#if defined(HYPERION_DEVHND_FORMAT)
        &tcpnje_halt,                  /* Device Halt channel pgm    */
#endif /* HYPERION_DEVHND_FORMAT */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        tcpnje_immed_command,          /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
#if defined(HYPERION_DEVHND_FORMAT)
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
#endif /* HYPERION_DEVHND_FORMAT */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};


/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */
#if !defined(HDL_BUILD_SHARED) && defined(HDL_USE_LIBTOOL)
#define hdl_ddev hdtTCPNJE_LTX_hdl_ddev
#define hdl_depc hdtTCPNJE_LTX_hdl_depc
#define hdl_reso hdtTCPNJE_LTX_hdl_reso
#define hdl_init hdtTCPNJE_LTX_hdl_init
#define hdl_fini hdtTCPNJE_LTX_hdl_fini
#endif


#if defined(OPTION_DYNAMIC_LOAD)
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
    HDL_RESOLVE_PTRVAR( psysblk, sysblk);
  }
  END_RESOLVER_SECTION
#endif


HDL_DEVICE_SECTION;
{
    HDL_DEVICE(tcpnje, tcpnje_device_hndinfo);
}
END_DEVICE_SECTION
#endif
