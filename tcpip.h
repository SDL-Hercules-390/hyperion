/*-------------------------------------------------------------------*/
/* TCPIP.H (C) Copyright Jason Paul Winter, 2003,2010                */
/*         Minor adaptions for SDL Hyperion, Juergen Winkelmann 2019 */
/*-------------------------------------------------------------------*/

#define Ccom 1024    /* maximum number of concurrent connections + 1 */

#if !defined( _X75_C_ )
#ifndef _MSVC_

/* Convert Windows declarations back to normal *nix types if needed: */

#define SOCKET int
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define LPSOCKADDR struct sockaddr *

#define closesocket close
#define ioctlsocket ioctl

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#endif

typedef struct talk_tag * talk_ptr;
typedef struct talk_tag {
    u_int  len_in;
    char * buffer_in;
    u_int  len_out;
    char * buffer_out;
    int    ret_cd;
    u_int  slot;
    u_int  buffer_in_slot;
    u_int  buffer_out_slot;
} talk;

typedef struct selects_tag * selects_ptr;
typedef struct selects_tag {
    u_int    len;
    u_int    invalid;
    u_int  * ri;
    u_int  * wi;
    u_int  * ei;
    u_int  * ro;
    u_int  * wo;
    u_int  * eo;
} selects;


#define hercNBBY 8
#define hercFD_SETSIZE  256
typedef u_int  hercfd_mask;
#define hercNFDBITS (sizeof(hercfd_mask) * hercNBBY)
#define herchowmany(x, y) (((x) + ((y) - 1)) / (y))

typedef struct hercfd_set {
    hercfd_mask hercfds_bits [herchowmany(hercFD_SETSIZE, hercNFDBITS)];
} hercfd_set;

#define hercFD_SET(n, p)    ((p)->hercfds_bits[(n)/hercNFDBITS] |= (1 << ((n) % hercNFDBITS)))
#define hercFD_CLR(n, p)    ((p)->hercfds_bits[(n)/hercNFDBITS] &= ~(1 << ((n) % hercNFDBITS)))
#define hercFD_ISSET(n, p)  ((p)->hercfds_bits[(n)/hercNFDBITS] & (1 << ((n) % hercNFDBITS)))

#ifdef _MSVC_

/* When using MSVC, just use the normal critical section functions: */

#undef initialize_lock
#undef LOCK
#undef obtain_lock
#undef release_lock

#define initialize_lock InitializeCriticalSection
#define LOCK CRITICAL_SECTION
#define obtain_lock EnterCriticalSection
#define release_lock LeaveCriticalSection

#endif

#define hEMFILE          24        /* Too many open files */
/* non-blocking and interrupt i/o */
#define    hEAGAIN          35        /* Resource temporarily unavailable */
#define    hEWOULDBLOCK     hEAGAIN /* Operation would block */
#define    hEINPROGRESS     36        /* Operation now in progress */
#define    hEALREADY        37        /* Operation already in progress */

/* ipc/network software -- argument errors */
#define    hENOTSOCK        38        /* Socket operation on non-socket */
#define    hEDESTADDRREQ    39        /* Destination address required */
#define    hEMSGSIZE        40        /* Message too int  */
#define    hEPROTOTYPE      41        /* Protocol wrong type for socket */
#define    hENOPROTOOPT     42        /* Protocol not available */
#define    hEPROTONOSUPPORT 43        /* Protocol not supported */
#define    hESOCKTNOSUPPORT 44        /* Socket type not supported */
#define    hEOPNOTSUPP      45        /* Operation not supported */
#define    hEPFNOSUPPORT    46        /* Protocol family not supported */
#define    hEAFNOSUPPORT    47        /* Address family not supported by protocol family */
#define    hEADDRINUSE      48        /* Address already in use */
#define    hEADDRNOTAVAIL   49        /* Can't assign requested address */

/* ipc/network software -- operational errors */
#define hENETDOWN        50        /* Network is down */
#define hENETUNREACH     51        /* Network is unreachable */
#define hENETRESET       52        /* Network dropped connection on reset */
#define hECONNABORTED    53        /* Software caused connection abort */
#define hECONNRESET      54        /* Connection reset by peer */
#define hENOBUFS         55        /* No buffer space available */
#define hEISCONN         56        /* Socket is already connected */
#define hENOTCONN        57        /* Socket is not connected */
#define hESHUTDOWN       58        /* Can't send after socket shutdown */
#define hETOOMANYREFS    59        /* Too many references: can't splice */
#define hETIMEDOUT       60        /* Operation timed out */
#define hECONNREFUSED    61        /* Connection refused */

/* Other */
#define hEINVAL          22        /* Invalid argument */
#define hELOOP           62        /* Too many levels of symbolic links */
#define hENAMETOOLONG    63        /* File name too int  */
#define hEHOSTDOWN       64        /* Host is down */
#define hEHOSTUNREACH    65        /* No route to host */
#define hENOTEMPTY       66        /* Directory not empty */
#define hEPROCLIM        67        /* Too many processes */
#define hEUSERS          68        /* Too many users */
#define hEDQUOT          69        /* Disc quota exceeded */
#define hESTALE          70        /* Stale NFS file handle */
#define hEREMOTE         71        /* Too many levels of remote in path */

#ifndef _MSVC_

/* Reverse my MSVC changes for Non MSVC compilers: */

#define fEMFILE          EMFILE
#define fEINVAL          EINVAL
#define fENAMETOOLONG    ENAMETOOLONG
#define fENOTEMPTY       ENOTEMPTY

#else

/* New MSVC compilers have EMFILE, EINVAL, ENAMETOOLONG and ENOTEMPTY */
/* defined in multiple places - so some new code here adds a 'f'ix!  */
/* This is what we need to do with error #s: Windows->Unix->Hercules */

#undef fEMFILE
#undef  EWOULDBLOCK
#undef  EINPROGRESS
#undef  EALREADY

#undef  ENOTSOCK
#undef  EDESTADDRREQ
#undef  EMSGSIZE
#undef  EPROTOTYPE
#undef  ENOPROTOOPT
#undef  EPROTONOSUPPORT
#undef  ESOCKTNOSUPPORT
#undef  EOPNOTSUPP
#undef  EPFNOSUPPORT
#undef  EAFNOSUPPORT
#undef  EADDRINUSE
#undef  EADDRNOTAVAIL

#undef  ENETDOWN
#undef  ENETUNREACH
#undef  ENETRESET
#undef  ECONNABORTED
#undef  ECONNRESET
#undef  ENOBUFS
#undef  EISCONN
#undef  ENOTCONN
#undef  ESHUTDOWN
#undef  ETOOMANYREFS
#undef  ETIMEDOUT
#undef  ECONNREFUSED

#undef fEINVAL
#undef  ELOOP
#undef fENAMETOOLONG
#undef  EHOSTDOWN
#undef  EHOSTUNREACH
#undef fENOTEMPTY
#undef  EPROCLIM
#undef  EUSERS
#undef  EDQUOT
#undef  ESTALE
#undef  EREMOTE

#define fEMFILE          WSAEMFILE
#define  EWOULDBLOCK     WSAEWOULDBLOCK /* Is also EAGAIN */
#define  EINPROGRESS     WSAEINPROGRESS
#define  EALREADY        WSAEALREADY

#define  ENOTSOCK        WSAENOTSOCK
#define  EDESTADDRREQ    WSAEDESTADDRREQ
#define  EMSGSIZE        WSAEMSGSIZE
#define  EPROTOTYPE      WSAEPROTOTYPE
#define  ENOPROTOOPT     WSAENOPROTOOPT
#define  EPROTONOSUPPORT WSAEPROTONOSUPPORT
#define  ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#define  EOPNOTSUPP      WSAEOPNOTSUPP
#define  EPFNOSUPPORT    WSAEPFNOSUPPORT
#define  EAFNOSUPPORT    WSAEAFNOSUPPORT
#define  EADDRINUSE      WSAEADDRINUSE
#define  EADDRNOTAVAIL   WSAEADDRNOTAVAIL

#define  ENETDOWN        WSAENETDOWN
#define  ENETUNREACH     WSAENETUNREACH
#define  ENETRESET       WSAENETRESET
#define  ECONNABORTED    WSAECONNABORTED
#define  ECONNRESET      WSAECONNRESET
#define  ENOBUFS         WSAENOBUFS
#define  EISCONN         WSAEISCONN
#define  ENOTCONN        WSAENOTCONN
#define  ESHUTDOWN       WSAESHUTDOWN
#define  ETOOMANYREFS    WSAETOOMANYREFS
#define  ETIMEDOUT       WSAETIMEDOUT
#define  ECONNREFUSED    WSAECONNREFUSED

#define fEINVAL          WSAEINVAL
#define  ELOOP           WSAELOOP
#define fENAMETOOLONG    WSAENAMETOOLONG
#define  EHOSTDOWN       WSAEHOSTDOWN
#define  EHOSTUNREACH    WSAEHOSTUNREACH
#define fENOTEMPTY       WSAENOTEMPTY
#define  EPROCLIM        WSAEPROCLIM
#define  EUSERS          WSAEUSERS
#define  EDQUOT          WSAEDQUOT
#define  ESTALE          WSAESTALE
#define  EREMOTE         WSAEREMOTE

#endif

static unsigned char DCCascii_to_ebcdic[] = {
    "\x00\x01\x02\x03\x37\x2D\x2E\x2F\x16\x05\x15\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\x3C\x3D\x32\x26\x18\x19\x3F\x27\x1C\x1D\x1E\x1F"
    "\x40\x5A\x7F\x7B\x5B\x6C\x50\x7D\x4D\x5D\x5C\x4E\x6B\x60\x4B\x61"
    "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\x7A\x5E\x4C\x7E\x6E\x6F"
    "\x7C\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xD1\xD2\xD3\xD4\xD5\xD6"
    "\xD7\xD8\xD9\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xAD\xE0\xBD\x5F\x6D"
    "\x79\x81\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96"
    "\x97\x98\x99\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xC0\x4F\xD0\xA1\x07"
    "\x20\x21\x22\x23\x24\x25\x06\x17\x28\x29\x2A\x2B\x2C\x09\x0A\x1B"
    "\x30\x31\x1A\x33\x34\x35\x36\x08\x38\x39\x3A\x3B\x04\x14\x3E\xFF"
    "\x41\xAA\x4A\xB1\x9F\xB2\x6A\xB5\xBB\xB4\x9A\x8A\xB0\xCA\xAF\xBC"
    "\x90\x8F\xEA\xFA\xBE\xA0\xB6\xB3\x9D\xDA\x9B\x8B\xB7\xB8\xB9\xAB"
    "\x64\x65\x62\x66\x63\x67\x9E\x68\x74\x71\x72\x73\x78\x75\x76\x77"
    "\xAC\x69\xED\xEE\xEB\xEF\xEC\xBF\x80\xFD\xFE\xFB\xFC\xBA\xAE\x59"
    "\x44\x45\x42\x46\x43\x47\x9C\x48\x54\x51\x52\x53\x58\x55\x56\x57"
    "\x8C\x49\xCD\xCE\xCB\xCF\xCC\xE1\x70\xDD\xDE\xDB\xDC\x8D\x8E\xDF"
};

static unsigned char DCCebcdic_to_ascii[] = {
    "\x00\x01\x02\x03\x9C\x09\x86\x7F\x97\x8D\x8E\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\x9D\x0A\x08\x87\x18\x19\x92\x8F\x1C\x1D\x1E\x1F"
    "\x80\x81\x82\x83\x84\x85\x17\x1B\x88\x89\x8A\x8B\x8C\x05\x06\x07"
    "\x90\x91\x16\x93\x94\x95\x96\x04\x98\x99\x9A\x9B\x14\x15\x9E\x1A"
    "\x20\xA0\xE2\xE4\xE0\xE1\xE3\xE5\xE7\xF1\xA2\x2E\x3C\x28\x2B\x7C"
    "\x26\xE9\xEA\xEB\xE8\xED\xEE\xEF\xEC\xDF\x21\x24\x2A\x29\x3B\x5E"
    "\x2D\x2F\xC2\xC4\xC0\xC1\xC3\xC5\xC7\xD1\xA6\x2C\x25\x5F\x3E\x3F"
    "\xF8\xC9\xCA\xCB\xC8\xCD\xCE\xCF\xCC\x60\x3A\x23\x40\x27\x3D\x22"
    "\xD8\x61\x62\x63\x64\x65\x66\x67\x68\x69\xAB\xBB\xF0\xFD\xFE\xB1"
    "\xB0\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\xAA\xBA\xE6\xB8\xC6\xA4"
    "\xB5\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\xA1\xBF\xD0\x5B\xDE\xAE"
    "\xAC\xA3\xA5\xB7\xA9\xA7\xB6\xBC\xBD\xBE\xDD\xA8\xAF\x5D\xB4\xD7"
    "\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49\xAD\xF4\xF6\xF2\xF3\xF5"
    "\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xB9\xFB\xFC\xF9\xFA\xFF"
    "\x5C\xF7\x53\x54\x55\x56\x57\x58\x59\x5A\xB2\xD4\xD6\xD2\xD3\xD5"
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xB3\xDB\xDC\xD9\xDA\x9F"
};

static char    Ccom_opn [Ccom];
static char    Ccom_blk [Ccom]; /* Is a blocking socket? */
static SOCKET  Ccom_han [Ccom];

static u_int     CerrGen;         /* Last error, thread-dodgy but it's only used for "non socket" errors */
static u_int     Cerr     [Ccom]; /* Last error for specific socket, this one is thread safe. */

static selects_ptr Cselect [Ccom];      /* Array for the 3 arrays of select input/output data */

static LOCK tcpip_lock;

static u_int  tcpip_init_req = 1;

U_LONG_PTR map32[Ccom];
#endif /*!defined( _X75_C_ )*/
