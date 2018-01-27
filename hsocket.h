/* HSOCKET.H    (C) Copyright Roger Bowler, 2005-2012                */
/*              Equates for socket functions                         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*  This header file contains equates for the socket functions       */
/*  and constants whose values differ between Unix and Winsock       */

#if !defined(_HSOCKET_H)
#define _HSOCKET_H

/*-------------------------------------------------------------------*/
/* Socket related constants related to 'shutdown' API call           */
/*-------------------------------------------------------------------*/

#ifdef _MSVC_

    /* Map SUS\*nix constants to Windows socket equivalents */

    #define  SHUT_RD     SD_RECEIVE
    #define  SHUT_WR     SD_SEND
    #define  SHUT_RDWR   SD_BOTH

#endif

#if defined(_WINSOCKAPI_)

/*-------------------------------------------------------------------*/
/* Equates for systems which use the Winsock API                     */
/*-------------------------------------------------------------------*/

#define get_HSO_errno()         ((int)WSAGetLastError())
#define set_HSO_errno(e)        (WSASetLastError(e))

#define HSO_errno               get_HSO_errno()

#define HSO_EINTR               WSAEINTR
#define HSO_EBADF               WSAEBADF
#define HSO_EACCES              WSAEACCES
#define HSO_EFAULT              WSAEFAULT
#define HSO_EINVAL              WSAEINVAL
#define HSO_EMFILE              WSAEMFILE
#define HSO_EWOULDBLOCK         WSAEWOULDBLOCK
#define HSO_EAGAIN              WSAEWOULDBLOCK
#define HSO_EINPROGRESS         WSAEINPROGRESS
#define HSO_EALREADY            WSAEALREADY
#define HSO_ENOTSOCK            WSAENOTSOCK
#define HSO_EDESTADDRREQ        WSAEDESTADDRREQ
#define HSO_EMSGSIZE            WSAEMSGSIZE
#define HSO_EPROTOTYPE          WSAEPROTOTYPE
#define HSO_ENOPROTOOPT         WSAENOPROTOOPT
#define HSO_EPROTONOSUPPORT     WSAEPROTONOSUPPORT
#define HSO_ESOCKTNOSUPPORT     WSAESOCKTNOSUPPORT
#define HSO_EOPNOTSUPP          WSAEOPNOTSUPP
#define HSO_EPFNOSUPPORT        WSAEPFNOSUPPORT
#define HSO_EAFNOSUPPORT        WSAEAFNOSUPPORT
#define HSO_EADDRINUSE          WSAEADDRINUSE
#define HSO_EADDRNOTAVAIL       WSAEADDRNOTAVAIL
#define HSO_ENETDOWN            WSAENETDOWN
#define HSO_ENETUNREACH         WSAENETUNREACH
#define HSO_ENETRESET           WSAENETRESET
#define HSO_ECONNABORTED        WSAECONNABORTED
#define HSO_ECONNRESET          WSAECONNRESET
#define HSO_ENOBUFS             WSAENOBUFS
#define HSO_EISCONN             WSAEISCONN
#define HSO_ENOTCONN            WSAENOTCONN
#define HSO_ESHUTDOWN           WSAESHUTDOWN
#define HSO_ETOOMANYREFS        WSAETOOMANYREFS
#define HSO_ETIMEDOUT           WSAETIMEDOUT
#define HSO_ECONNREFUSED        WSAECONNREFUSED
#define HSO_ELOOP               WSAELOOP
#define HSO_ENAMETOOLONG        WSAENAMETOOLONG
#define HSO_EHOSTDOWN           WSAEHOSTDOWN
#define HSO_EHOSTUNREACH        WSAEHOSTUNREACH
#define HSO_ENOTEMPTY           WSAENOTEMPTY
#define HSO_EPROCLIM            WSAEPROCLIM
#define HSO_EUSERS              WSAEUSERS
#define HSO_EDQUOT              WSAEDQUOT
#define HSO_ESTALE              WSAESTALE
#define HSO_EREMOTE             WSAEREMOTE

#else

/*-------------------------------------------------------------------*/
/* Equates for systems which use the Berkeley sockets API            */
/*-------------------------------------------------------------------*/

#define get_HSO_errno()         (errno)
#define set_HSO_errno(e)        (errno=(e))

#define HSO_errno               get_HSO_errno()

#define HSO_EINTR               EINTR
#define HSO_EBADF               EBADF
#define HSO_EACCES              EACCES
#define HSO_EFAULT              EFAULT
#define HSO_EINVAL              EINVAL
#define HSO_EMFILE              EMFILE
#define HSO_EWOULDBLOCK         EWOULDBLOCK
#define HSO_EAGAIN              EAGAIN
#define HSO_EINPROGRESS         EINPROGRESS
#define HSO_EALREADY            EALREADY
#define HSO_ENOTSOCK            ENOTSOCK
#define HSO_EDESTADDRREQ        EDESTADDRREQ
#define HSO_EMSGSIZE            EMSGSIZE
#define HSO_EPROTOTYPE          EPROTOTYPE
#define HSO_ENOPROTOOPT         ENOPROTOOPT
#define HSO_EPROTONOSUPPORT     EPROTONOSUPPORT
#define HSO_ESOCKTNOSUPPORT     ESOCKTNOSUPPORT
#define HSO_EOPNOTSUPP          EOPNOTSUPP
#define HSO_EPFNOSUPPORT        EPFNOSUPPORT
#define HSO_EAFNOSUPPORT        EAFNOSUPPORT
#define HSO_EADDRINUSE          EADDRINUSE
#define HSO_EADDRNOTAVAIL       EADDRNOTAVAIL
#define HSO_ENETDOWN            ENETDOWN
#define HSO_ENETUNREACH         ENETUNREACH
#define HSO_ENETRESET           ENETRESET
#define HSO_ECONNABORTED        ECONNABORTED
#define HSO_ECONNRESET          ECONNRESET
#define HSO_ENOBUFS             ENOBUFS
#define HSO_EISCONN             EISCONN
#define HSO_ENOTCONN            ENOTCONN
#define HSO_ESHUTDOWN           ESHUTDOWN
#define HSO_ETOOMANYREFS        ETOOMANYREFS
#define HSO_ETIMEDOUT           ETIMEDOUT
#define HSO_ECONNREFUSED        ECONNREFUSED
#define HSO_ELOOP               ELOOP
#define HSO_ENAMETOOLONG        ENAMETOOLONG
#define HSO_EHOSTDOWN           EHOSTDOWN
#define HSO_EHOSTUNREACH        EHOSTUNREACH
#define HSO_ENOTEMPTY           ENOTEMPTY
#define HSO_EPROCLIM            EPROCLIM
#define HSO_EUSERS              EUSERS
#define HSO_EDQUOT              EDQUOT
#define HSO_ESTALE              ESTALE
#define HSO_EREMOTE             EREMOTE

#endif

/*-------------------------------------------------------------------*/
/* Local function definitions                                        */
/*-------------------------------------------------------------------*/

HSOCK_DLL_IMPORT int read_socket(int fd, void *ptr, int nbytes);
HSOCK_DLL_IMPORT int write_socket(int fd, const void *ptr, int nbytes);
HSOCK_DLL_IMPORT int disable_nagle(int fd);

#endif /*!defined(_HSOCKET_H)*/
