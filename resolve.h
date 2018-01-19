/* RESOLVE.H    (C) Copyright Ian Shorter, 2011-2012                 */
/*              Resolve host name or IP address                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __RESOLVE_H_
#define __RESOLVE_H_

/* ----------------------------------------------------------------- */
/* Hercules Resolve Block                                            */
/* ----------------------------------------------------------------- */

struct _HRB;

typedef struct _HRB HRB, *PHRB;

struct _HRB
{
    char            host[256];          // Host name or IP address
    char            ipaddr[64];         // IP address
    unsigned int    salen;              // Sockaddr length
    union
       {
           struct sockaddr_in in;
           struct sockaddr_in6 in6;
       }            sa;                 // Sockaddr
    int             afam;               // Address family
    unsigned int    numeric;            // Numerical address
    int             wantafam;           // Wanted address family
    int             rv;                 // getaddrinfo() or gethostinfo() return value
    char            em[80];             // Error message
};

/* ----------------------------------------------------------------- */
/* Function Declarations                                             */
/* ----------------------------------------------------------------- */
extern  int  resolve_host( PHRB pHRB );
extern  int  resolve_ipaddr( PHRB pHRB );
extern  int  resolve_sa( PHRB pHRB );


#endif // __RESOLVE_H_
