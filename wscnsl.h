/* WSCNSL.H    (C) Copyright Hercules contributors, 2026             */
/*              WebSocket Console Bridge                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module bridges the existing telnet/3270 console transport    */
/* over WebSocket (RFC 6455) connections, websockify-style: the WS   */
/* payload is the same byte stream the raw socket would carry, so    */
/* the libtelnet/3270 stack above is unchanged.                      */
/*-------------------------------------------------------------------*/

#ifndef _WSCNSL_H_
#define _WSCNSL_H_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Perform the HTTP Upgrade handshake on a freshly accepted socket.  */
/* Returns 0 on success, -1 on failure (and writes an HTTP 4xx       */
/* response to the client when possible).                            */
/*-------------------------------------------------------------------*/
int  ws_handshake ( int csock );

/*-------------------------------------------------------------------*/
/* Drop-in replacement for recv() at the three console.c read sites. */
/* For non-WebSocket connections (tn->is_websocket == 0) it simply   */
/* calls recv() on tn->csock. For WebSocket connections it parses    */
/* RFC 6455 frames, transparently handles control frames (PING/PONG/ */
/* CLOSE), unmasks payload bytes and returns them to the caller.     */
/* Same return semantics as recv(): >0 = bytes, 0 = EOF, -1 = error. */
/*-------------------------------------------------------------------*/
int  ws_recv ( TELNET* tn, void* buf, int nbytes );

/*-------------------------------------------------------------------*/
/* Drop-in replacement for write_socket() in the TELNET_EV_SEND case.*/
/* For non-WebSocket connections it calls write_socket() directly.   */
/* For WebSocket connections it wraps the bytes in a binary frame    */
/* (opcode 0x2, FIN=1, no mask) and writes the framed bytes.         */
/* Returns total payload bytes accepted (>0) or <=0 on error.        */
/*-------------------------------------------------------------------*/
int  ws_send ( TELNET* tn, const void* buf, int nbytes );

/*-------------------------------------------------------------------*/
/* Tear down per-connection WebSocket state. Sends a Close frame to  */
/* the peer if applicable, then frees the WS-only buffers. Caller is */
/* still responsible for closing the underlying socket.              */
/*-------------------------------------------------------------------*/
void ws_free_state ( TELNET* tn );

#endif /* _WSCNSL_H_ */
