/* SOCKDEV.H    (C) Copyright Roger Bowler, 1999-2012                */
/*              SocketDevice support                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "htypes.h"         // need Herc's struct typedefs

#ifndef _SOCKDEV_H_
#define _SOCKDEV_H_

/*-------------------------------------------------------------------*/
/* The sockdev callback function is an optional callback function    */
/* that the sockdev connection handler calls after the connection    */
/* has been established but before it has logged the connection to   */
/* the console. The boolean return code from the callback indicates  */
/* true or false (!0 or 0) whether the connection should be accepted */
/* or not. If the return from the callback is 0 (false), the socket  */
/* is immediately closed and the "connection not accepted" message   */
/* is logged to the console. You should NOT perform any significant  */
/* processing in your callback. If you need to do any significant    */
/* processing you should instead create a worker to perform it in.   */
/*-------------------------------------------------------------------*/

typedef int (*ONCONNECT)( DEVBLK* );     // onconnect callback function (opt)

/*-------------------------------------------------------------------*/
/* Bind structure for "Socket Devices"                               */
/*-------------------------------------------------------------------*/

struct bind_struct          // Bind structure for "Socket Devices"
{
    LIST_ENTRY  bind_link;  // (just a link in the chain)

    DEVBLK  *dev;           // ptr to corresponding device block
    char    *spec;          // socket_spec for listening socket
    int      sd;            // listening socket to use in select

                            // NOTE: Following 2 fields malloc'ed.
    char    *clientname;    // connected client's hostname
    char    *clientip;      // conencted client's ip address

    ONCONNECT    fn;        // ptr to onconnect callback func (opt)
    void        *arg;       // argument for callback function (opt)
};

/* "Socket Device" functions */

extern        int bind_device_ex   (DEVBLK* dev, char* spec, ONCONNECT fn, void* arg );
extern        int unbind_device_ex (DEVBLK* dev, int forced);

static inline int bind_device      (DEVBLK* dev, char* spec) { return bind_device_ex   ( dev, spec, NULL, NULL ); }
static inline int unbind_device    (DEVBLK* dev)             { return unbind_device_ex ( dev, 0 );                }

#endif // _SOCKDEV_H_
