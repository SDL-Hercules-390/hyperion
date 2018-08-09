/* HSOCKET.C    (C) Copyright Roger Bowler, 2003-2012                */
/*             Hercules Socket read/write routines                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HSOCKET_C_
#define _HUTIL_DLL_

#include "hercules.h"

/************************************************************************

NAME:      read_socket - read a passed number of bytes from a socket.

PURPOSE:   This routine is used in place of read to read a passed number
           of bytes from a socket.  A read on a socket will return less
           than the number of bytes requested if there are less currenly
           available.  Thus we read in a loop till we get all we want.

PARAMETERS:
   1.   fd        -  int (INPUT)
                     This is the file descriptor for the socket to be read.

   2.   ptr        - pointer to void (OUTPUT)
                     This is a pointer to where the data is to be put.

   3.   nbytes     - int (INPUT)
                     This is the number of bytes to read.

FUNCTIONS :

   1.   Read in a loop till an error occurs or the data is read.

OUTPUTS:
   data into the buffer

*************************************************************************/

DLL_EXPORT
int read_socket( int fd, void *_ptr, int nbytes )
{
char  *ptr;
int   nleft, nread;

    ptr   = _ptr;               /* point to input buffer             */
    nleft = nbytes;             /* number of bytes to be read        */

    while (nleft > 0)           /* while room in i/p buffer remains  */
    {
#ifdef _MSVC_
        nread = recv( fd, ptr, nleft, 0 );
#else
        nread = read( fd, ptr, nleft );
#endif
        if (nread < 0)
            return nread;       /* error, return < 0 */
        else
        if (nread == 0)         /* EOF; we read all we could */
            break;

        ptr   += nread;         /* bump to next i/p buffer location  */
        nleft -= nread;         /* adjust remaining bytes to be read */

    } /* end of do while */

    return (nbytes - nleft);    /* return number of bytes read */

} /* end of read_socket */


/************************************************************************

NAME:      write_socket - write a passed number of bytes into a socket.

PURPOSE:   This routine is used in place of write to write a passed number
           of bytes into a socket.  A write on a socket may take less
           than the number of bytes requested if there is currently insufficient
           buffer space available.  Thus we write in a loop till we get all we want.

PARAMETERS:
   1.   fd        -  int (OUTPUT)
                     This is the file descriptor for the socket to be written.

   2.   ptr        - pointer to void (INPUT)
                     This is a pointer to where the data is to be gotten from.

   3.   nbytes     - int (INPUT)
                     This is the number of bytes to write.

FUNCTIONS :
   1.   Write in a loop till an error occurs or the data is written.

OUTPUTS:
   Data to the socket.

*************************************************************************/

/*
 * Write "n" bytes to a descriptor.
 * Use in place of write() when fd is a stream socket.
 */

DLL_EXPORT
int write_socket( int fd, const void *_ptr, int nbytes )
{
const char *ptr;
int  nleft, nwritten;

    ptr   = _ptr;               /* point to data to be written       */
    nleft = nbytes;             /* number of bytes to be written     */

    while (nleft > 0)           /* while bytes remain to be written  */
    {
#ifdef _MSVC_
        nwritten = send( fd, ptr, nleft, 0 );
#else
        nwritten = write( fd, ptr, nleft );
#endif
        if (nwritten <= 0)
            return nwritten;    /* error, return <= 0 */

        ptr   += nwritten;      /* bump to next o/p buffer location  */
        nleft -= nwritten;      /* fix remaining bytes to be written */

    } /* end of do while */

    return (nbytes - nleft);    /* return number of bytes written */

} /* end of write_socket */


/************************************************************************

  NAME:     disable_nagle - disable Nagle's algorithm for the socket.

  PURPOSE:  This routine is used to disable Nagle's algorithm for those
            sockets which need to send their data with miminum delay,
            such as notification pipes or telnet console devices, etc.

  RETURNS:  Returns 0 if successful or -1 if unsuccessful.

*************************************************************************/

DLL_EXPORT int disable_nagle( int fd )
{
static const int nodelay = 1;

    int rc = setsockopt
    (
        fd,                 /* socket affected          */
        IPPROTO_TCP,        /* set option at TCP level  */
        TCP_NODELAY,        /* the name of the option   */
        (char*)&nodelay,    /* pointer to option value  */
        sizeof(nodelay)     /* length of option value   */
    );
    return rc;

} /* end of disable_nagle */
