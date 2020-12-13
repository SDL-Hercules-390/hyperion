/* RESOLVE.C    (C) Copyright Ian Shorter, 2011-2012                 */
/*              Resolve host name or IP address                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#if !defined(__SOLARIS__)

#include "hercules.h"
#include "resolve.h"


/* ----------------------------------------------------------------- */
/* Resolve host name to IP address.                                  */
/* ----------------------------------------------------------------- */
/* Return value.                                                     */
/*  0  Resolve was successful.                                       */
/* -1  Resolve was not successful. HRB field em contains an error    */
/*     message.                                                      */
/* HRB fields.                                                       */
/* Input.                                                            */
/*   host                                                            */
/*     The character host name or numeric IP address to be resolved. */
/*   numeric                                                         */
/*     When set to TRUE (not 0) host must contain a numeric IP       */
/*     address, when set to FALSE (0) host can contain a host name   */
/*     or a numeric IP address.                                      */
/*   wantafam                                                        */
/*     When set to AF_UNSPEC (0) host can resolve to either an IPv4  */
/*     or an IPv6 address (whichever the resolver returns first),    */
/*     when set to AF_INET (2) host must resolve to an IPv4 address, */
/*     or when set to AF_INET6 (10) host must resolve to an IPv6     */
/*     address.                                                      */
/* Output.                                                           */
/*   ipaddr                                                          */
/*     The resolved character numeric IP address.                    */
/*   salen                                                           */
/*     The length of the sockaddr containing the resolved address    */
/*     family and IP address.                                        */
/*   sa                                                              */
/*     The sockaddr containing the resolved address family and IP    */
/*     address.                                                      */
/*   afam                                                            */
/*     The resolved address family.                                  */
/*   rv                                                              */
/*     The getaddrinfo() return value.                               */
/*   em                                                              */
/*     If the resolve was not successful, an error message.          */

int  resolve_host( PHRB pHRB )
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *rescon = NULL;
    int             rv = -1;


    // Clear the output.
    memset( &pHRB->ipaddr, 0, sizeof(pHRB->ipaddr) );
    pHRB->salen = 0;
    memset( &pHRB->sa, 0, sizeof(pHRB->sa) );
    pHRB->afam = 0;
    pHRB->rv = 0;
    memset( &pHRB->em, 0, sizeof(pHRB->em) );

    // Prepare the hints.
    memset( &hints, 0, sizeof(hints) );
    if( pHRB->numeric )
        hints.ai_flags |= AI_NUMERICHOST;
    hints.ai_family = pHRB->wantafam;

    // Resolve the host name or IP address.
    pHRB->rv = getaddrinfo( pHRB->host, NULL, &hints, &result );

    // If the getaddrinfo was not successful, let the caller know why.
    if( pHRB->rv != 0 )
    {
        MSGBUF( pHRB->em,
                  "getaddrinfo fail: %d, %s",
                  pHRB->rv, gai_strerror(pHRB->rv) );
        return rv;
    }

    // Process the getaddrinfo result.
    for (rescon = result; rescon != NULL; rescon = rescon->ai_next)
    {
        if( rescon->ai_family == AF_INET &&
            ( pHRB->wantafam == AF_INET || pHRB->wantafam == AF_UNSPEC ) )
        {
            pHRB->afam = AF_INET;
            pHRB->salen = rescon->ai_addrlen;
            memcpy( &pHRB->sa, rescon->ai_addr, rescon->ai_addrlen );
            hinet_ntop( AF_INET, &pHRB->sa.in.sin_addr, pHRB->ipaddr, sizeof(pHRB->ipaddr)-1 );
            rv = 0;
            break;
        }
        else if( rescon->ai_family == AF_INET6 &&
                 ( pHRB->wantafam == AF_INET6 || pHRB->wantafam == AF_UNSPEC ) )
        {
            pHRB->afam = AF_INET6;
            pHRB->salen = rescon->ai_addrlen;
            memcpy( &pHRB->sa, rescon->ai_addr, rescon->ai_addrlen );
            hinet_ntop( AF_INET6, &pHRB->sa.in6.sin6_addr, pHRB->ipaddr, sizeof(pHRB->ipaddr)-1 );
            rv = 0;
            break;
        }
    }

    // If the getaddrinfo result were not useful, let the caller know we
    // don't know why. This could happen if the function is called with
    // pHRB->wantafam == AF_UNSPEC and the host name does not resolve to
    // an AF_INET or AF_INET6 address, but does resolve to something
    // else. This scenario is extremely unlikely I know, but...
    if( rv != 0 )
    {
        MSGBUF( pHRB->em,
                  "resolve fail: host does not resolve to inet or inet6" );
    }

    // Free the getaddrinfo result.
    freeaddrinfo( result );

    return rv;
}   /* End function resolve_host() */


/* ----------------------------------------------------------------- */
/* Resolve character IP address to host name.                        */
/* ----------------------------------------------------------------- */
/* Return value.                                                     */
/*  0  Resolve was successful.                                       */
/* -1  Resolve was not successful. HRB field em contains an error    */
/*     message.                                                      */
/* HRB fields.                                                       */
/* Input.                                                            */
/*   ipaddr                                                          */
/*     The character IP address to be resolved.                      */
/* Output.                                                           */
/*   host                                                            */
/*     The resolved host name.                                       */
/*   salen                                                           */
/*     The length of the sockaddr containing the resolved address    */
/*     family and IP address.                                        */
/*   sa                                                              */
/*     The sockaddr containing the resolved address family and IP    */
/*     address.                                                      */
/*   afam                                                            */
/*     The resolved address family.                                  */
/*   rv                                                              */
/*     The getaddrinfo() or getnameinfo() return value.              */
/*   em                                                              */
/*     If the resolve was not successful, an error message.          */
/* Not used.                                                         */
/*   numeric                                                         */
/*   wantafam                                                        */

int  resolve_ipaddr( PHRB pHRB )
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;


    // Clear the output.
    memset( &pHRB->host, 0, sizeof(pHRB->host) );
    pHRB->salen = 0;
    memset( &pHRB->sa, 0, sizeof(pHRB->sa) );
    pHRB->afam = 0;
    pHRB->rv = 0;
    memset( &pHRB->em, 0, sizeof(pHRB->em) );

    // Prepare the hints.
    memset( &hints, 0, sizeof(hints) );
    hints.ai_flags = AI_NUMERICHOST;

    // Resolve the character IP address.
    pHRB->rv = getaddrinfo( pHRB->ipaddr, NULL, &hints, &result );

    // If the getaddrinfo was not successful, let the caller know why.
    if( pHRB->rv != 0 )
    {
        MSGBUF( pHRB->em,
                  "getaddrinfo fail: %d, %s",
                  pHRB->rv, gai_strerror(pHRB->rv) );
        return -1;
    }

    // Process the getaddrinfo result.
    if( result->ai_family == AF_INET )
    {
        pHRB->salen = result->ai_addrlen;
        pHRB->sa.in.sin_family = AF_INET;
        memcpy( &pHRB->sa, result->ai_addr, result->ai_addrlen );
        pHRB->afam = AF_INET;
    }
    else if( result->ai_family == AF_INET6 )
    {
        pHRB->salen = result->ai_addrlen;
        pHRB->sa.in6.sin6_family = AF_INET6;
        memcpy( &pHRB->sa, result->ai_addr, result->ai_addrlen );
        pHRB->afam = AF_INET6;
    }
    else
    {
        MSGBUF( pHRB->em,
                  "resolve fail: IP address not inet or inet6" );
        freeaddrinfo( result );
        return -1;
    }

    // Free the getaddrinfo result.
    freeaddrinfo( result );

    // Resolve the sockaddr IP address to host name.
    pHRB->rv = getnameinfo( (struct sockaddr*)&pHRB->sa, pHRB->salen,
                            pHRB->host, sizeof(pHRB->host)-1,
                            NULL, 0, 0);

    // If the getnameinfo was not successful, let the caller know why.
    if( pHRB->rv != 0 )
    {
        MSGBUF( pHRB->em,
                  "getnameinfo fail: %d, %s",
                  pHRB->rv, gai_strerror(pHRB->rv) );
        return -1;
    }

    return 0;
}   /* End function resolve_ipaddr() */


/* ----------------------------------------------------------------- */
/* Resolve sockaddr IP address to host name.                         */
/* ----------------------------------------------------------------- */
/* Return value.                                                     */
/*  0  Resolve was successful.                                       */
/* -1  Resolve was not successful. HRB field em contains an error    */
/*     message.                                                      */
/* HRB fields.                                                       */
/* Input.                                                            */
/*   sa                                                              */
/*     The sockaddr containing the address family and IP address to  */
/*     be resolved.                                                  */
/* Output.                                                           */
/*   host                                                            */
/*     The resolved host name.                                       */
/*   salen                                                           */
/*     The length of the sockaddr containing the resolved address    */
/*     family and IP address.                                        */
/*   afam                                                            */
/*     The resolved address family.                                  */
/*   rv                                                              */
/*     The getnameinfo() return value.                               */
/*   em                                                              */
/*     If the resolve was not successful, an error message.          */
/* Not used.                                                         */
/*   numeric                                                         */
/*   wantafam                                                        */
/*   ipaddr                                                          */

int  resolve_sa( PHRB pHRB )
{

    // Clear the output.
    memset( &pHRB->host, 0, sizeof(pHRB->host) );
    pHRB->salen = 0;
    pHRB->afam = 0;
    pHRB->rv = 0;
    memset( &pHRB->em, 0, sizeof(pHRB->em) );

    // Check the sockaddr address family.
    if( pHRB->sa.in.sin_family == AF_INET )
    {
        pHRB->salen = sizeof(pHRB->sa.in);
        pHRB->afam = AF_INET;
    }
    else if( pHRB->sa.in6.sin6_family == AF_INET6 )
    {
        pHRB->salen = sizeof(pHRB->sa.in6);
        pHRB->afam = AF_INET6;
    }
    else
    {
        MSGBUF( pHRB->em,
                  "resolve fail: address family not inet or inet6" );
        return -1;
    }

    // Resolve the sockaddr IP address to host name.
    pHRB->rv = getnameinfo( (struct sockaddr*)&pHRB->sa, pHRB->salen,
                            pHRB->host, sizeof(pHRB->host)-1,
                            NULL, 0, 0);

    // If the getnameinfo was not successful, let the caller know why.
    if( pHRB->rv != 0 )
    {
        MSGBUF( pHRB->em,
                  "getnameinfo fail: %d, %s",
                  pHRB->rv, gai_strerror(pHRB->rv) );
        return -1;
    }

    return 0;
}   /* End function resolve_sa() */


#endif /* !defined(__SOLARIS__) */
