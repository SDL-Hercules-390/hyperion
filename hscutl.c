/*  HSCUTL.C    (C) Copyright Ivan Warren & Others, 2003-2012        */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              Hercules Platform Port & Misc Functions              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* HSCUTL.C   --   Implementation of functions used in hercules that */
/* may be missing on some platform ports, or other convenient mis-   */
/* laneous global utility functions.                                 */
/*-------------------------------------------------------------------*/
/* (c) 2003-2006 Ivan Warren & Others -- Released under the Q Public */
/* License -- This file is portion of the HERCULES S/370, S/390 and  */
/* z/Architecture emulator                                           */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HSCUTL_C_
#define _HUTIL_DLL_

#include "hercules.h"

#if defined(NEED_GETOPT_WRAPPER)
/*
                  GETOPT DYNAMIC LINKING KLUDGE...

  For some odd reason, on some platforms (namely windows & possibly
  Darwin) dynamically linking to the libc imports STATIC versions
  of 'getopt' and 'getopt_long' into the link-edited shared library.
  If any program then link edits against BOTH the shared library AND
  libc, then the linker complains about 'getopt' and/or 'getopt_long'
  being defined multiple times. In an effort to overcome this, I am
  defining a stub version of 'getopt' and 'getop_long' that can be
  called by loadable modules instead.

  -- Ivan


  ** ZZ FIXME: the above needs to be re-verified to see whether it is
  ** still true or not. I suspect it may no longer be true due to the
  ** subtle bug fixed in the "_HC_CHECK_NEED_GETOPT_WRAPPER" m4 macro
  ** in the "./autoconf/hercules.m4" member. -- Fish, Feb 2005

*/

int herc_opterr=0;
char *herc_optarg=NULL;
int herc_optopt=0;
int herc_optind=1;
#if defined(NEED_GETOPT_OPTRESET)
int herc_optreset=0;
#endif

DLL_EXPORT int herc_getopt(int ac,char * const av[],const char *opt)
{
    int rc;
#if defined(NEED_GETOPT_OPTRESET)
    optreset=herc_optreset;
#endif
    rc=getopt(ac,av,opt);
#if defined(NEED_GETOPT_OPTRESET)
    herc_optreset=optreset;
#endif
    herc_optarg=optarg;
    herc_optind=optind;
    herc_optopt=optind;
    herc_opterr=optind;
    return(rc);
}

#if defined(HAVE_GETOPT_LONG)
struct option; // (fwd ref)
int getopt_long (int, char *const *, const char *, const struct option *, int *);
struct option; // (fwd ref)
DLL_EXPORT int herc_getopt_long(int ac,
                     char * const av[],
                     const char *opt,
                     const struct option *lo,
                     int *li)
{
    int rc;
#if defined(NEED_GETOPT_OPTRESET)
    optreset=herc_optreset;
#endif
    optind=herc_optind;
    rc=getopt_long(ac,av,opt,lo,li);
#if defined(NEED_GETOPT_OPTRESET)
    herc_optreset=optreset;
#endif
    herc_optarg=optarg;
    herc_optind=optind;
    herc_optopt=optind;
    herc_opterr=optind;
    return(rc);
}
#endif /* HAVE_GETOPT_LONG */

#endif /* NEED_GETOPT_WRAPPER */

#if !defined(WIN32) && !defined(HAVE_STRERROR_R)
static LOCK strerror_lock;
DLL_EXPORT void strerror_r_init(void)
{
    initialize_lock(&strerror_lock);
}
DLL_EXPORT int strerror_r(int err,char *bfr,size_t sz)
{
    char *wbfr;
    obtain_lock(&strerror_lock);
    wbfr=strerror(err);
    if(wbfr==NULL || (int)wbfr==-1)
    {
        release_lock(&strerror_lock);
        return(-1);
    }
    if(sz<=strlen(wbfr))
    {
        errno=ERANGE;
        release_lock(&strerror_lock);
        return(-1);
    }
    strncpy(bfr,wbfr,sz);
    release_lock(&strerror_lock);
    return(0);
}
#endif // !defined(HAVE_STRERROR_R)

#if !defined(HAVE_STRLCPY)
/* $OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $ */

/*  ** NOTE **  returns 'size_t' and NOT 'char*' like strncpy! */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $";
#endif /* LIBC_SCCS and not lint */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */

/*  ** NOTE **  returns 'size_t' and NOT 'char*' like strncpy! */

DLL_EXPORT size_t
strlcpy(char *dst, const char *src, size_t siz)
{
 register char *d = dst;
 register const char *s = src;
 register size_t n = siz;

 /* Copy as many bytes as will fit */
 if (n != 0 && --n != 0) {
  do {
   if ((*d++ = *s++) == 0)
    break;
  } while (--n != 0);
 }

 /* Not enough room in dst, add NUL and traverse rest of src */
 if (n == 0) {
  if (siz != 0)
   *d = '\0';  /* NUL-terminate dst */
  while (*s++)
   ;
 }

 return(s - src - 1); /* count does not include NUL */
}
#endif // !defined(HAVE_STRLCPY)

#if !defined(HAVE_STRLCAT)
/* $OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $ */

/*  ** NOTE **  returns 'size_t' and NOT 'char*' like strncpy! */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $";
#endif /* LIBC_SCCS and not lint */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */

/*  ** NOTE **  returns 'size_t' and NOT 'char*' like strncat! */

DLL_EXPORT size_t
strlcat(char *dst, const char *src, size_t siz)
{
 register char *d = dst;
 register const char *s = src;
 register size_t n = siz;
 size_t dlen;

 /* Find the end of dst and adjust bytes left but don't go past end */
 while (n-- != 0 && *d != '\0')
  d++;
 dlen = d - dst;
 n = siz - dlen;

 if (n == 0)
  return(dlen + strlen(s));
 while (*s != '\0') {
  if (n != 1) {
   *d++ = *s;
   n--;
  }
  s++;
 }
 *d = '\0';

 return(dlen + (s - src)); /* count does not include NUL */
}
#endif // !defined(HAVE_STRLCAT)

/* The following structures are defined herein because they are private structures */
/* that MUST be opaque to the outside world                                        */
typedef struct _SYMBOL_TOKEN
{
    char *var;
    char *val;
} SYMBOL_TOKEN;

#define SYMBOL_TABLE_INCREMENT  256
#define SYMBOL_BUFFER_GROWTH    256
#define MAX_SYMBOL_SIZE         31
#define SYMBOL_QUAL_1   '$'
#define SYMBOL_QUAL_2   '('
#define SYMBOL_QUAL_3   ')'

static SYMBOL_TOKEN **symbols=NULL;
static int symbol_count=0;
static int symbol_max=0;

/* This function retrieves or allocates a new SYMBOL_TOKEN */
static SYMBOL_TOKEN *get_symbol_token(const char *sym, int alloc)
{
    SYMBOL_TOKEN        *tok;
    int i;

    for(i=0;i<symbol_count;i++)
    {
        tok=symbols[i];
        if(tok==NULL)
        {
            continue;
        }
#if defined( CASELESS_SYMBOLS )
        if(strcasecmp(symbols[i]->var,sym)==0)
#else
        if(strcmp(symbols[i]->var,sym)==0)
#endif
        {
            return(symbols[i]);
        }
    }
    if(!alloc)
    {
        return(NULL);
    }
    if(symbol_count>=symbol_max)
    {
        symbol_max+=SYMBOL_TABLE_INCREMENT;
        if(symbols==NULL)
        {
        symbols=malloc(sizeof(SYMBOL_TOKEN *)*symbol_max);
        if(symbols==NULL)
        {
            symbol_max=0;
            symbol_count=0;
            return(NULL);
        }
        }
        else
        {
        symbols=realloc(symbols,sizeof(SYMBOL_TOKEN *)*symbol_max);
        if(symbols==NULL)
        {
            symbol_max=0;
            symbol_count=0;
            return(NULL);
        }
        }
    }
    tok=malloc(sizeof(SYMBOL_TOKEN));
    if(tok==NULL)
    {
        return(NULL);
    }

    tok->var=malloc(MAX_SYMBOL_SIZE+1);
    if(tok->var==NULL)
    {
        free(tok);
        return(NULL);
    }
    strncpy(tok->var,sym,MAX_SYMBOL_SIZE);
    tok->var[MAX_SYMBOL_SIZE]=0;    /* Ensure null termination */

    tok->val=NULL;
    symbols[symbol_count]=tok;
    symbol_count++;
    return(tok);
}

DLL_EXPORT void del_symbol(const char *sym)
{
    SYMBOL_TOKEN        *tok;
    int i;

    for(i=0;i<symbol_count;i++)
    {
        tok=symbols[i];
        if(tok==NULL)
        {
            continue;
        }
#if defined( CASELESS_SYMBOLS )
        if(strcasecmp(symbols[i]->var,sym)==0)
#else
        if(strcmp(symbols[i]->var,sym)==0)
#endif
        {
            if ( tok->val != NULL ) free(tok->val);
            if ( tok->var != NULL ) free(tok->var);
            free(tok);
            symbols[i] = NULL;
            return;
        }
    }

    return;
}

DLL_EXPORT void set_symbol(const char *sym, const char *value)
{
    SYMBOL_TOKEN *tok;
    size_t size;

    if ( sym == NULL || value == NULL || strlen(sym) == 0 )
        return;

    tok=get_symbol_token(sym,1);
    if(tok==NULL)
    {
        return;
    }
    if(tok->val!=NULL)
    {
        free(tok->val);
    }
    size = strlen(value)+1;
    tok->val=malloc(size);
    if(tok->val==NULL)
    {
        return;
    }
    strlcpy(tok->val,value,size);
    return;
}

DLL_EXPORT const char* get_symbol( const char* sym )
{
    SYMBOL_TOKEN* tok;
    char buf[ MAX_ENVVAR_LEN ];

    if (CMD( sym, DATE, 4 ))
    {
        // Rebuild new value each time date/time symbol is retrieved
        time_t  raw_tt;
        time( &raw_tt );                // YYYYMMDD
        strftime( buf, sizeof( buf ) - 1, "%Y%m%d", localtime( &raw_tt ));
        set_symbol( sym = "DATE", buf );
    }
    else if (CMD( sym, TIME, 4 ))
    {
        // Rebuild new value each time date/time symbol is retrieved
        time_t  raw_tt;
        time( &raw_tt );                // HHMMSS
        strftime( buf, sizeof( buf ) - 1, "%H%M%S", localtime( &raw_tt ));
        set_symbol( sym = "TIME", buf );
    }

    if (!(tok = get_symbol_token( sym, 0 )))
    {
        // Add this environment variable to our DEFSYM pool
        const char* val = getenv( sym );
        MSGBUF( buf, "%s", val ? val : "" );
        set_symbol( sym, buf );
        // (now try again; should succeed this time)
        tok = get_symbol_token( sym, 0 );
    }
    return tok->val;
}

DLL_EXPORT char *resolve_symbol_string(const char *text)
{
    char    buf[MAX_PATH*4];                /* Statement buffer          */
    char    dflt[MAX_PATH*4];               /* temp location for default */
    int     c;                              /* Character work area       */
    int     i = 0;                          /* Position in the input     */
    int     stmtlen = 0;                    /* Statement length          */
    int     inc_dollar = -1;                /* >=0 Ndx of dollar         */
    int     inc_lbrace = -1;                /* >=0 Ndx of lbrace + 1     */
    int     inc_colon  = -1;                /* >=0 Ndx of colon          */
    int     inc_equals = -1;                /* >=0 Ndx of equals         */
    int     lstarted;                       /* Indicate if non-whitespace*/
    char   *inc_envvar;                     /* ->Environment variable    */

    char    symt = 0;                       /* Character work area       */

    if( strstr( text, "$(" ) == NULL && strstr( text, "${" ) == NULL )
    {
        /* Malloc anyway - the caller will free() */
        return( strdup( text ) );
    }

    memset(buf, 0, sizeof(buf));

    while(1)
    {
        for (i = 0, stmtlen = 0, lstarted = 0; ;i++)
        {
            c = text[i];

            /* Ignore nulls and carriage returns */
            if (c == '\0' ) break;

            /* Check if it is a white space and no other character yet */
            if(!lstarted && isspace(c)) continue;
            lstarted=1;

            /* Check that statement does not overflow buffer */
            if (stmtlen >= (int)(sizeof(buf) - 1))
            {
                WRMSG( HHC01418, "E" );
                return( strdup(text) );
            }

            /* inc_dollar already processed? */
            if (inc_dollar >= 0)
            {
                /* Left brace already processed? */
                if (inc_lbrace >= 0)
                {
                    /* End of variable spec? */
                    if ( c == symt )
                    {
                        /* Terminate it */
                        buf[stmtlen] = '\0';

                        /* Terminate var name if we have a inc_colon specifier */
                        if (inc_colon >= 0)
                        {
                            buf[inc_colon] = '\0';
                        }

                        /* Terminate var name if we have a default value */
                        if (inc_equals >= 0)
                        {
                            buf[inc_equals] = '\0';
                        }

                        /* Reset statement index to start of variable */
                        stmtlen = inc_dollar;

                        /* Get variable value */
                        inc_envvar = (char *)get_symbol (&buf[inc_lbrace]);

                        /* Variable unset? */
                        if (inc_envvar == NULL || strlen(inc_envvar) == 0 )
                        {
                            /* Substitute default if specified */
                            if (inc_equals >= 0)
                            {
                                memset(dflt, 0, sizeof(dflt));
                                STRLCPY( dflt, &buf[inc_equals+1] );
                                inc_envvar = dflt;
                            }
                        }

                        /* Have a value? (environment or default) */
                        if (inc_envvar != NULL)
                        {
                            /* Check that statement does not overflow buffer */
                            if (stmtlen+strlen(inc_envvar) >= sizeof(buf) - 1)
                            {
                                WRMSG( HHC01418, "E" );
                                return( strdup( text ) );
                            }

                            /* Copy to buffer and update index */
                            stmtlen += snprintf( &buf[stmtlen],
                                                 (sizeof(buf) - stmtlen) - 1,
                                                 "%s", inc_envvar );
                        }
                        memset(&buf[stmtlen], 0, (sizeof(buf) - stmtlen));

                        /* Reset indexes */
                        inc_equals = -1;
                        inc_colon = -1;
                        inc_lbrace = -1;
                        inc_dollar = -1;
                        continue;
                    }
                    else if (c == ':' && inc_colon < 0 && inc_equals < 0)
                    {
                        /* Remember possible start of default specifier */
                        inc_colon = stmtlen;
                    }
                    else if (c == '=' && inc_equals < 0)
                    {
                        /* Remember possible start of default specifier */
                        inc_equals = stmtlen;
                    }
                }
                else // (inc_lbrace < 0)
                {
                    /* Remember start of variable name */

                    if ( c == '(' )
                    {
                        symt = ')' ;
                        inc_lbrace = stmtlen + 1;
                    }
                    else if ( c == '{' )
                    {
                        symt = '}' ;
                        inc_lbrace = stmtlen + 1;
                    }
                    else
                    {
                        /* Reset inc_dollar specifier if immediately following
                        character is not a left brace */
                        inc_dollar = -1;
                    }
                }
            }
            else // (inc_dollar < 0)
            {
                /* Enter variable substitution state */
                if (c == '$')
                {
                    inc_dollar = stmtlen;
                }
            }

            /* Append character to buffer */
            buf[stmtlen++] = c;

        } /* end for(stmtlen) */

        break;
    }

    return (strdup(buf));
}

/* (called by defsym panel command) */
DLL_EXPORT void list_all_symbols()
{
    SYMBOL_TOKEN* tok; int i;
    for (i=0; i < symbol_count; i++)
        if ((tok = symbols[i]) != NULL)
            // "Symbol %-12s %s"
            WRMSG( HHC02199, "I", tok->var, tok->val ? tok->val : "" );
}

/* Subtract 'beg_timeval' from 'end_timeval' yielding 'dif_timeval' */
/* Return code: success == 0, error == -1 (difference was negative) */

DLL_EXPORT int timeval_subtract
(
    struct timeval *beg_timeval,
    struct timeval *end_timeval,
    struct timeval *dif_timeval
)
{
    struct timeval begtime;
    struct timeval endtime;
    ASSERT ( beg_timeval -> tv_sec >= 0  &&  beg_timeval -> tv_usec >= 0 );
    ASSERT ( end_timeval -> tv_sec >= 0  &&  end_timeval -> tv_usec >= 0 );

    memcpy(&begtime,beg_timeval,sizeof(struct timeval));
    memcpy(&endtime,end_timeval,sizeof(struct timeval));

    dif_timeval->tv_sec = endtime.tv_sec - begtime.tv_sec;

    if (endtime.tv_usec >= begtime.tv_usec)
    {
        dif_timeval->tv_usec = endtime.tv_usec - begtime.tv_usec;
    }
    else
    {
        dif_timeval->tv_sec--;
        dif_timeval->tv_usec = (endtime.tv_usec + 1000000) - begtime.tv_usec;
    }

    return ((dif_timeval->tv_sec < 0 || dif_timeval->tv_usec < 0) ? -1 : 0);
}

/* Add 'dif_timeval' to 'accum_timeval' (use to accumulate elapsed times) */
/* Return code: success == 0, error == -1 (accum_timeval result negative) */

DLL_EXPORT int timeval_add
(
    struct timeval *dif_timeval,
    struct timeval *accum_timeval
)
{
    ASSERT ( dif_timeval   -> tv_sec >= 0  &&  dif_timeval   -> tv_usec >= 0 );
    ASSERT ( accum_timeval -> tv_sec >= 0  &&  accum_timeval -> tv_usec >= 0 );

    accum_timeval->tv_sec  += dif_timeval->tv_sec;
    accum_timeval->tv_usec += dif_timeval->tv_usec;

    if (accum_timeval->tv_usec >= 1000000)
    {
        int usec = accum_timeval->tv_usec / 1000000;
        accum_timeval->tv_sec  += usec;
        accum_timeval->tv_usec -= usec * 1000000;
    }

    return ((accum_timeval->tv_sec < 0 || accum_timeval->tv_usec < 0) ? -1 : 0);
}

/*
  Easier to use timed_wait_condition that waits for the specified
  relative amount of time without you having to build an absolute
  timeout time yourself. Use the "timed_wait_condition_relative_usecs"
  macro to call it.
*/
DLL_EXPORT int timed_wait_condition_relative_usecs_impl
(
    COND*            pCOND,     // ptr to condition to wait on
    LOCK*            pLOCK,     // ptr to controlling lock (must be held!)
    U32              usecs,     // max #of microseconds to wait
    struct timeval*  pTV,       // [OPTIONAL] ptr to tod value (may be NULL)
    const char*      loc        // (location)
)
{
    struct timespec timeout_timespec;
    struct timeval  now;

    if (!pTV)
    {
        pTV = &now;
        gettimeofday( pTV, NULL );
    }

    timeout_timespec.tv_sec  = pTV->tv_sec  + ( usecs / 1000000 );
    timeout_timespec.tv_nsec = pTV->tv_usec + ( usecs % 1000000 );

    if ( timeout_timespec.tv_nsec >= 1000000 )
    {
        timeout_timespec.tv_sec  += timeout_timespec.tv_nsec / 1000000;
        timeout_timespec.tv_nsec %=                            1000000;
    }

    timeout_timespec.tv_nsec *= 1000;
//  return timed_wait_condition( pCOND, pLOCK, &timeout_timespec );
    return hthread_timed_wait_condition( pCOND, pLOCK, &timeout_timespec, loc );
}

/*********************************************************************
  The following couple of Hercules 'utility' functions may be defined
  elsewhere depending on which host platform we're being built for...
  For Windows builds (e.g. MingW32), the functionality for the below
  functions is defined in 'w32util.c'. For other host build platforms
  (e.g. Linux, Apple, etc), the functionality for the below functions
  is defined right here in 'hscutil.c'...
 *********************************************************************/

#if !defined(_MSVC_)

/* THIS module (hscutil.c) is to provide the below functionality.. */

/*
  Returns outpath as a host filesystem compatible filename path.
  This is a Cygwin-to-MSVC transitional period helper function.
  On non-Windows platforms it simply copies inpath to outpath.
  On Windows it converts inpath of the form "/cygdrive/x/foo.bar"
  to outpath in the form "x:/foo.bar" for Windows compatibility.
*/
DLL_EXPORT char *hostpath( char *outpath, const char *inpath, size_t buffsize )
{
    if (inpath && outpath && buffsize > 1)
        strlcpy( outpath, inpath, buffsize );
    else if (outpath && buffsize)
        *outpath = 0;
    return outpath;
}

/* Poor man's  "fcntl( fd, F_GETFL )"... */
/* (only returns access-mode flags and not any others) */
DLL_EXPORT int get_file_accmode_flags( int fd )
{
    int flags = fcntl( fd, F_GETFL );
    return ( flags & O_ACCMODE );
}

/* Set socket to blocking or non-blocking mode... */
DLL_EXPORT int socket_set_blocking_mode( int sfd, int blocking_mode )
{
    int flags = fcntl( sfd, F_GETFL );

    if ( blocking_mode )
        flags &= ~O_NONBLOCK;
    else
        flags |=  O_NONBLOCK;

    return fcntl( sfd, F_SETFL, flags );
}

/* Initialize/Deinitialize sockets package... */
int  socket_init   ( void ) { return 0; }
DLL_EXPORT int  socket_deinit ( void ) { return 0; }

/* Determine whether a file descriptor is a socket or not... */
/* (returns 1==true if it's a socket, 0==false otherwise)    */
DLL_EXPORT int socket_is_socket( int sfd )
{
    struct stat st;
    return ( fstat( sfd, &st ) == 0 && S_ISSOCK( st.st_mode ) );
}

/* Set the SO_KEEPALIVE option and timeout values for a
   socket connection to detect when client disconnects.
   Returns 0==success, +1==warning, -1==failure
   (*) Warning failure means function only partially
       succeeded (not all requested values were set)
*/
int set_socket_keepalive( int sfd,
                          int idle_time,
                          int probe_interval,
                          int probe_count )
{
#if !defined( HAVE_BASIC_KEEPALIVE )

    UNREFERENCED( sfd );
    UNREFERENCED( idle_time );
    UNREFERENCED( probe_interval );
    UNREFERENCED( probe_count );
    errno = EOPNOTSUPP;
    return -1;

#else // basic, partial or full: must attempt setting keepalive

    int idle, intv, cnt;
    int rc, l_tcp, optval, succeeded = 0, tried = 0;
    struct protoent * tcpproto;

    /* Retrieve TCP protocol value (mostly for FreeBSD portability) */
    tcpproto = getprotobyname("TCP");
    if (!tcpproto)
    {
        WRMSG( HHC02219, "E", "getprotobyname(\"TCP\")", strerror( HSO_errno ));
        return -1;
    }
    l_tcp = tcpproto->p_proto;

    optval = 1;
  #if defined( HAVE_DECL_SO_KEEPALIVE ) && HAVE_DECL_SO_KEEPALIVE
    tried++;
    rc = setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    if (rc)
    {
        WRMSG( HHC02219, "E", "setsockopt(SO_KEEPALIVE)", strerror( HSO_errno ));
        return -1;
    }
    else
        succeeded++;
  #endif

    optval = idle_time;
  #if defined( HAVE_DECL_TCP_KEEPALIVE ) && HAVE_DECL_TCP_KEEPALIVE
    tried++;
    rc = setsockopt(sfd, l_tcp, TCP_KEEPALIVE, &optval, sizeof(optval));
    if (rc)
        WRMSG( HHC02219, "E", "setsockopt(TCP_KEEPALIVE)", strerror( HSO_errno ));
    else
    {
        succeeded++;
        idle = idle_time;
    }
  #endif

    /* Only try setting individual keepalive values if full or partial
       keepalive is supported. Otherwise if we have only basic keepalive
       then don't bother even trying
    */
#if defined( HAVE_FULL_KEEPALIVE ) || defined( HAVE_PARTIAL_KEEPALIVE )

    optval = idle_time;
  #if defined( HAVE_DECL_TCP_KEEPIDLE ) && HAVE_DECL_TCP_KEEPIDLE
    tried++;
    rc = setsockopt(sfd, l_tcp, TCP_KEEPIDLE, &optval, sizeof(optval));
    if (rc)
        WRMSG( HHC02219, "E", "setsockopt(TCP_KEEPIDLE)", strerror( HSO_errno ));
    else
    {
        succeeded++;
        idle = idle_time;
    }
  #endif

    optval = probe_interval;
  #if defined( HAVE_DECL_TCP_KEEPINTVL ) && HAVE_DECL_TCP_KEEPINTVL
    tried++;
    rc = setsockopt(sfd, l_tcp, TCP_KEEPINTVL, &optval, sizeof(optval));
    if (rc)
        WRMSG( HHC02219, "E", "setsockopt(TCP_KEEPINTVL)", strerror( HSO_errno ));
    else
    {
        succeeded++;
        intv = probe_interval;
    }
  #endif

    optval = probe_count;
  #if defined( HAVE_DECL_TCP_KEEPCNT ) && HAVE_DECL_TCP_KEEPCNT
    tried++;
    rc = setsockopt(sfd, l_tcp, TCP_KEEPCNT, &optval, sizeof(optval));
    if (rc)
        WRMSG( HHC02219, "E", "setsockopt(TCP_KEEPCNT)", strerror( HSO_errno ));
    else
    {
        succeeded++;
        cnt  = probe_count;
    }
  #endif

#endif // defined( HAVE_FULL_KEEPALIVE ) || defined( HAVE_PARTIAL_KEEPALIVE )

    /* Retrieve values in use, ignoring any error */
    get_socket_keepalive( sfd, &idle, &intv, & cnt );

    /* Determine return code: did everything succeed and,
       more importantly, were all of our values accepted?
       Did the values we tried setting (if any) "stick"?
    */
    if (succeeded >= tried)
        rc = (idle != idle_time || intv != probe_interval || cnt != probe_count) ? +1 : 0;
    else
        rc = (succeeded <= 0 ? -1 : +1);

    return rc;

#endif // (KEEPALIVE)
}

/* Function to retrieve keepalive values. 0==success, -1=failure */
int get_socket_keepalive( int sfd, int* idle_time, int* probe_interval,
                          int* probe_count )
{
#if defined( HAVE_FULL_KEEPALIVE ) || defined( HAVE_PARTIAL_KEEPALIVE )
    struct protoent*  tcpproto;
    int        optval, l_tcp;
    socklen_t  optlen = sizeof( optval );

    /* Retrieve TCP protocol value (mostly for FreeBSD portability) */
    tcpproto = getprotobyname("TCP");
    if (!tcpproto)
    {
        WRMSG( HHC02219, "E", "getprotobyname(\"TCP\")", strerror( HSO_errno ));
        return -1;
    }
    l_tcp = tcpproto->p_proto;
#else
    UNREFERENCED( sfd );
#endif // HAVE_FULL_KEEPALIVE || HAVE_PARTIAL_KEEPALIVE

    /* Default values */
    *idle_time      = sysblk.kaidle;
    *probe_interval = sysblk.kaintv;
    *probe_count    = sysblk.kacnt;

#if defined( HAVE_FULL_KEEPALIVE ) || defined( HAVE_PARTIAL_KEEPALIVE )
    /* Retrieve the actual values from the system */
  #if defined( HAVE_DECL_TCP_KEEPIDLE ) && HAVE_DECL_TCP_KEEPIDLE
    if (getsockopt( sfd, l_tcp, TCP_KEEPIDLE,  &optval, &optlen ) >= 0) *idle_time      = optval;
  #endif
  #if defined( HAVE_DECL_TCP_KEEPINTVL ) && HAVE_DECL_TCP_KEEPINTVL
    if (getsockopt( sfd, l_tcp, TCP_KEEPINTVL, &optval, &optlen ) >= 0) *probe_interval = optval;
  #endif
  #if defined( HAVE_DECL_TCP_KEEPCNT ) && HAVE_DECL_TCP_KEEPCNT
    if (getsockopt( sfd, l_tcp, TCP_KEEPCNT,   &optval, &optlen ) >= 0) *probe_count    = optval;
  #endif
#endif // HAVE_FULL_KEEPALIVE || HAVE_PARTIAL_KEEPALIVE

    return 0;
}

// Hercules low-level file open...
DLL_EXPORT  int hopen( const char* path, int oflag, ... )
{
    int pmode = 0;
    if (oflag & O_CREAT)
    {
        va_list vargs;
        va_start( vargs, oflag );
        pmode = va_arg( vargs, int );
    }
    return open( path, oflag, pmode );
}

/* Determine whether process is running "elevated" or not */
/* (returns 1==true running elevated, 0==false otherwise) */
bool are_elevated()
{
    return (sysblk.euid == 0) ? true : false;
}

#endif // !defined(_MSVC_)

//////////////////////////////////////////////////////////////////////////////////////////

/*******************************************/
/* Read/Write to socket functions          */
/*******************************************/
DLL_EXPORT int hgetc(int s)
{
    char c;
    int rc;
    rc=recv(s,&c,1,0);
    if(rc<1)
    {
        return EOF;
    }
    return c;
}

DLL_EXPORT char * hgets(char *b,size_t c,int s)
{
    size_t ix=0;
    while(ix<c)
    {
        b[ix]=hgetc(s);
        if ((signed char)b[ix] == EOF)
        {
            return NULL;
        }
        b[ix+1]=0;
        if(b[ix]=='\n')
        {
            return(b);
        }
        ix++;
    }
    return NULL;
}

DLL_EXPORT int hwrite(int s,const char *bfr,size_t sz)
{
    return send(s,bfr,(int)sz,0); /* (int) cast is for _WIN64 */
}

DLL_EXPORT int hprintf(int s,char *fmt,...)
{
    char *bfr;
    size_t bsize=1024;
    int rc;
    va_list vl;

    bfr=malloc(bsize);
    while(1)
    {
        if(!bfr)
        {
            return -1;
        }
        va_start(vl,fmt);
        rc=vsnprintf(bfr,bsize,fmt,vl);
        va_end(vl);
        if(rc<(int)bsize)
        {
            break;
        }
        bsize+=1024;
        bfr=realloc(bfr,bsize);
    }
    rc=hwrite(s,bfr,strlen(bfr));
    free(bfr);
    return rc;
}

/* Hercules page-aligned calloc/free */

DLL_EXPORT void* hpcalloc( BYTE type, size_t size )
{
    /* PROGRAMMING NOTE: we presume the host page size
       will never be smaller than the guest page size */

    void*      ptr        = NULL;       /* (aligned)   */
    void*      p          = NULL;       /* (unaligned) */
    size_t     bytes      = 0;
    size_t     pagesize   = HPAGESIZE();
    uintptr_t  alignmask  = ~(pagesize-1);

    /* Need extra room for pointer and pagesize alignment */
    bytes = size + sizeof(void*) + pagesize - 1;

    /* Get memory already pre-initialized to binary zeroes */
    if ((p = calloc( bytes, 1 )) != NULL)
    {
        /* Round up to the next host pagesize boundary
           being careful there is room for our pointer */
        ptr = (void*)(((uintptr_t) p + sizeof(void*) + pagesize - 1) & alignmask);

        /* Save original ptr for hpcfree() just before
           page-aligned ptr we'll be returning to them */
        *((void**)((uintptr_t) ptr - sizeof(void*))) = p;

        /* Indicate the storage has already been cleared */
        if (HPC_MAINSTOR == type) sysblk.main_clear = 1;
        if (HPC_XPNDSTOR == type) sysblk.xpnd_clear = 1;
    }
    return ptr;   /* Return page-aligned allocation */
}

DLL_EXPORT void hpcfree( BYTE type, void* ptr )
{
    /* Retrieve the original ptr that hpcalloc() saved
       immediately preceding the original allocation */
    void* p = *(void**)((uintptr_t)ptr - sizeof(void*));

    /* Free the original calloc() allocated memory */
    free( p );

    /* Indicate guest storage is no longer cleared */
    if (HPC_MAINSTOR == type) sysblk.main_clear = 0;
    if (HPC_XPNDSTOR == type) sysblk.xpnd_clear = 0;
}

/* Posix 1003.1e capabilities support */

#if defined(HAVE_SYS_CAPABILITY_H) && defined(HAVE_SYS_PRCTL_H) && defined(OPTION_CAPABILITIES)
/*-------------------------------------------------------------------*/
/* DROP root privileges but retain a capability                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int drop_privileges(int capa)
{
    uid_t    uid;
    gid_t    gid;
    cap_t   c;
    int rc;
    int failed;
    cap_value_t v;
    int have_capt;

    /* If *real* userid is root, no need to do all this */
    uid=getuid();
    if(!uid) return 0;

    failed=1;
    have_capt=0;
    do
    {
        c=cap_init();
        if(!c) break;
        have_capt=1;
        v=capa;
        rc=cap_set_flag(c,CAP_EFFECTIVE,1,&v,CAP_SET);
        if(rc<0) break;
        rc=cap_set_flag(c,CAP_INHERITABLE,1,&v,CAP_SET);
        if(rc<0) break;
        rc=cap_set_flag(c,CAP_PERMITTED,1,&v,CAP_SET);
        if(rc<0) break;
        rc=cap_set_proc(c);
        if(rc<0) break;
        rc=prctl(PR_SET_KEEPCAPS,1);
        if(rc<0) break;
        failed=0;
    } while(0);
    gid=getgid();
    setregid(gid,gid);
    setreuid(uid,uid);
    if(!failed)
    {
        rc=cap_set_proc(c);
        if(rc<0) failed=1;
    }

    if(have_capt)
        cap_free(c);

    return failed;
}
/*-------------------------------------------------------------------*/
/* DROP all capabilities                                             */
/*-------------------------------------------------------------------*/
DLL_EXPORT int drop_all_caps(void)
{
    uid_t    uid;
    cap_t   c;
    int rc;
    int failed;
    int have_capt;

    /* If *real* userid is root, no need to do all this */
    uid=getuid();
    if(!uid) return 0;

    failed=1;
    have_capt=0;
    do
    {
        c=cap_from_text("all-eip");
        if(!c) break;
        have_capt=1;
        rc=cap_set_proc(c);
        if(rc<0) break;
        failed=0;
    } while(0);

    if(have_capt)
        cap_free(c);

    return failed;
}
#endif /* defined(HAVE_SYS_CAPABILITY_H) && defined(HAVE_SYS_PRCTL_H) && defined(OPTION_CAPABILITIES) */

/*-------------------------------------------------------------------*/
/* Trim path information from __FILE__ macro value                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* trimloc( const char* loc )
{
    /*
    ** The __FILE__ macro expands to a path by which the preprocessor
    ** opened the file, not the short name specified in "#include" or
    ** as the input file name argument.  For example, __FILE__ might
    ** expand to "/usr/local/include/myheader.h", not "myheader.h".
    ** The following compensates for this condition by returning just
    ** the base name.
    **
    ** PROGRAMMING NOTE: we cannot use the basename() function here
    ** because:
    **
    **
    **    "The basename() function may modify the string
    **     pointed to by path, and may return a pointer to
    **     internal storage. The returned pointer might be
    **     invalidated or the storage might be overwritten
    **     by a subsequent call to basename()."
    **
    **    "The basename() function need not be thread-safe."
    **
    **
    ** The below implementation avoids both issues by just returning
    ** a pointer indexed into the current string constant.
    */
    char* p = strrchr( loc, '\\' );         /* Windows */
    if (!p) p = strrchr( loc, '/' );        /* non-Windows */
    if (p)
        loc = p+1;
    return loc;
}

/*-------------------------------------------------------------------*/
/* Format TIMEVAL to printable value: "YYYY-MM-DD HH:MM:SS.uuuuuu",  */
/* being exactly 26 characters long (27 bytes with null terminator). */
/* pTV points to the TIMEVAL to be formatted. If NULL is passed then */
/* the curent time of day as returned by a call to 'gettimeofday' is */
/* used instead. buf must point to a char work buffer where the time */
/* is formatted into and must not be NULL. bufsz is the size of buf  */
/* and must be >= 2. If successful then the value of buf is returned */
/* and is always zero terminated. If an error occurs or an invalid   */
/* parameter is passed then NULL is returned instead.                */
/*-------------------------------------------------------------------*/
DLL_EXPORT char* FormatTIMEVAL( const TIMEVAL* pTV, char* buf, int bufsz )
{
    struct timeval  tv;
    struct tm*      pTM;
    time_t          todsecs;
    if (!buf || bufsz < 2)
        return NULL;
    if (!pTV)
    {
        gettimeofday( &tv, NULL );
        pTV = &tv;
    }
    todsecs = pTV->tv_sec;
    pTM = localtime( &todsecs );
    strftime( buf, bufsz, "%Y-%m-%d %H:%M:%S", pTM );
    if (bufsz > 20)
        snprintf( &buf[19], bufsz-19, ".%06d", (int)pTV->tv_usec );
    buf[ bufsz-1 ] = 0;
    return buf;
}

/*-------------------------------------------------------------------*/
/* fmt_memsize helper                                                */
/*-------------------------------------------------------------------*/
static char* _fmt_memsize( const U64 memsize, const u_int n,
                           char* buf, const size_t bufsz )
{
    /* Format storage in 2**(10*n) values at the highest integral
     * integer boundary.
     *
     * Mainframe memory and DASD amounts are reported in 2**(10*n)
     * values, (x_iB international format, and shown as x_ or x_B,
     * when x >= 1024; x when x < 1024).  Open Systems and Windows
     * report memory in the same format, but report DASD storage in
     * 10**(3*n) values.  (Thank you, various marketing groups and
     * international standards committees...)
     *
     * For Hercules, mainframe oriented reporting characteristics
     * will be formatted and shown as x_, when x >= 1024, and as x
     * when x < 1024.  Reporting of Open Systems and Windows specifics
     * should follow the international format, shown as x_iB,
     * when x >= 1024, and x or xB when x < 1024.
     *
     * Reporting is done at the highest integral boundary.
     */
    //------------------------------------------------------------------
    // The 'n' value passed to us determines which suffix we start with
    //------------------------------------------------------------------
    const  char  suffix[9] = {0x00, 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
    U64          mem  = memsize;
    u_int        i    = 0;

    if (mem)
        for (i = n;
             i < _countof( suffix ) && !(mem & 0x03FF);
             mem >>= 10, ++i);

    snprintf( buf, bufsz, "%"PRIu64"%c", mem, suffix[ i ]);
    return buf;
}

DLL_EXPORT char* fmt_memsize   ( const U64 memsize,   char* buf, const size_t bufsz ) { return _fmt_memsize( memsize,   0, buf, bufsz ); }
DLL_EXPORT char* fmt_memsize_KB( const U64 memsizeKB, char* buf, const size_t bufsz ) { return _fmt_memsize( memsizeKB, 1, buf, bufsz ); }
DLL_EXPORT char* fmt_memsize_MB( const U64 memsizeMB, char* buf, const size_t bufsz ) { return _fmt_memsize( memsizeMB, 2, buf, bufsz ); }

/*-------------------------------------------------------------------*/
/* Pretty format S64 value with thousand separators. Returns length. */
/*-------------------------------------------------------------------*/
DLL_EXPORT size_t fmt_S64( char dst[32], S64 num )
{
    char  src[32];
    char* p_src     = src;
    char* p_dst     = dst;

    const char separator = ',';     // (FIXME)
    int num_len, commas;

    num_len = snprintf( src, sizeof( src ), "%"PRId64, num );

    if (*p_src == '-')
    {
        *p_dst++ = *p_src++;
        num_len--;
    }

    for (commas = 2 - num_len % 3;
         *p_src;
         commas = (commas + 1) % 3)
    {
        *p_dst++ = *p_src++;
        if (commas == 1) {
            *p_dst++ = separator;
        }
    }

    *--p_dst = 0;

    return (size_t) (p_dst - dst);
}

/*-------------------------------------------------------------------*/
/*                Standard Utility Initialization                    */
/*-------------------------------------------------------------------*/
/* Performs standard utility initialization as follows: determines   */
/* the program name (i.e. just the name without the .ext) from the   */
/* passed argv[0] or default name value, displays the program name,  */
/* description, version, copyright and build date values, tells the  */
/* debugger what name to assign to the main thread (Windows only),   */
/* initializes the global 'extgui' flag, initializes SYSBLK "detach" */
/* and "join" create_thread attributes (the remainder of SYSBLK is   */
/* set to low values), initializes the translation codepage to the   */
/* system default, and lastly, intitializes the HOSTINFO structure.  */
/* (but it doesn't necessarily do all that in that order of course)  */
/*                                                                   */
/* The program name (without .ext) is optionally returned in *pgm    */
/* which must be freed by the caller when it is no longer needed.    */
/* Pass NULL for 'pgm' if you don't care to know your own name.      */
/*                                                                   */
/* The return value is the new argc value which may not be the same  */
/* as the value that was passed due to external gui initialization.  */
/*-------------------------------------------------------------------*/

int initialize_utility( int argc, char* argv[],
                        char*  defpgm,
                        char*  desc,
                        char** pgm )
{
    char*  exename;                     /* Executable name with .ext */
    char*  nameonly;                    /* Exename without any .ext  */
    char*  strtok_str;                  /* Work (strtok_r context)   */
    char   namedesc[256];               /* Message build work area   */

    setvbuf( stderr, NULL, _IONBF, 0 );
    setvbuf( stdout, NULL, _IONBF, 0 );

    /* If the last argument is "EXTERNALGUI" it means we're running
       under the control of an external GUI. Utilities need to know
       this so they can react differently than in command-line mode. */
    if (argc >= 1 && strncmp( argv[argc-1], "EXTERNALGUI", 11 ) == 0)
    {
        extgui = 1;   /* Tell utility to send progress msgs to GUI */

        /* Remove the "EXTERNALGUI" argument from the command-line */
        argv[argc-1] = NULL;
        argc--;
    }

    if (argc < 1)
        exename = strdup( defpgm );
    else
    {
        if (strlen( argv[0] ) == 0)
            exename = strdup( defpgm );
        else
        {
            char path[ MAX_PATH ];
#if defined( _MSVC_ )
            GetModuleFileName( NULL, path, MAX_PATH );
#else
            STRLCPY( path, argv[0] );
#endif
            exename = strdup( basename( path ));
        }
    }

    /*
    **  Initialize a bunch of other stuff...
    **
    **  Note that initialization MUST occur in the following order:
    **  SYSBLK first, then hthreads (since it updates SYSBLK), then
    **  locks, etc, since they require hthreads.
    */
    memset( &sysblk, 0, sizeof( SYSBLK ));      // (must be first)
    hthreads_internal_init();                   // (must be second)
    SET_THREAD_NAME( exename );                 // (then other stuff)
    sysblk.msglvl = DEFAULT_MLVL;

    initialize_detach_attr( DETACHED );
    initialize_join_attr( JOINABLE );
    initialize_lock( &sysblk.dasdcache_lock );
    set_codepage( NULL );
    init_hostinfo( &hostinfo );

    /* Seed the pseudo-random number generator */
    init_random();

    strtok_str = NULL;
    if (!(nameonly = strtok_r( exename, ".", &strtok_str )))
        nameonly = exename;

    /* PROGRAMMING NOTE: we cannot free "exename" until we're
       done using "nameonly" since nameonly now points to it! */

    if (pgm)
        *pgm = strdup( nameonly );

    /* Format the program identification message */
    MSGBUF( namedesc, "%s - %s", nameonly, desc );

    /* Now it's safe to discard exename */
    free( exename );

    /* Display version, copyright, and build date */
    init_sysblk_version_str_arrays( namedesc );
    if (defpgm && strcasecmp(defpgm, "hercifc") == 0)
        display_version( stderr, 0, NULL );
    else
        display_version( stdout, 0, NULL );

    return argc;
}

/*-------------------------------------------------------------------*/
/*                Reverse the bits in a BYTE                         */
/*-------------------------------------------------------------------*/

static unsigned char rev_nib_bits_tab[16] =
{
//  entry #1=0001==>1000,  entry #7=0111==>1110,  etc...

//  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,

//  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F
};
/* PROGRAMMING NOTE: while using a direct index into a 256 entry table
   might be more efficient it also takes much more scrutiny (careful
   visual inspection) of all 256 individual table entries to ensure
   they're all correct and is harder to see what's going on. Using a
   simple 16 entry table on the other hand (that only swaps 4 bits)
   not only makes it easier to see what's actually going on but also
   is much easier to visually verify all table entries are correct
   (and isn't significantly less inefficient when the number of bytes
   you are reversing is relatively small, which is usually the case)
*/
DLL_EXPORT BYTE reverse_bits( BYTE b )
{
    /* Reverse the top/bottom nibbles separately,
       then swap the two nibbles with each other.
    */
    return
    (
        rev_nib_bits_tab[ b & 0x0F ] << 4
        |
        rev_nib_bits_tab[ b >> 4 ]
    );
}

/*-------------------------------------------------------------------*/
/* Format printer cctape information                                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT void FormatCCTAPE( char* buf, size_t buflen,
                              int lpi, int lpp,
                              U16* cctape )
{
    size_t len, curlen = 0, newlen;
    int line, chnum, chans;
    U16 mask;
    char onechan[8], allchans[48], chanlist[64];

    /* Get started */
    snprintf( buf, buflen, "lpi=%d lpp=%d cctape=(", lpi, lpp );
    curlen = strlen( buf );

    /* Format: "cctape=(lll=cc,lll=(cc,cc),...)" */
    for (line=0; line < lpp; line++)
    {
        /* Skip lines with no channels defined */
        if (!cctape[line])
            continue;

        /* Format channel stop(s) for this line */
        for (chans=0, allchans[0]=0, chnum=1, mask=0x8000;
            chnum <= 12; mask >>= 1, chnum++)
        {
            if (cctape[ line ] & mask)
            {
                MSGBUF( onechan, "%d", chnum );
                if (chans++)
                    strlcat( allchans, ",", _countof( allchans ));
                strlcat( allchans, onechan, _countof( allchans ));
            }
        }

        /* Surround this line's channels with parens if needed */
        MSGBUF( chanlist, "%d=%s%s%s,",
            line+1,
            chans > 1 ? "(" : "",
            allchans,
            chans > 1 ? ")" : "" );

        len = strlen( chanlist );

        /* Truncate and return if out of buffer space */
        if ((newlen = curlen + len) >= (buflen - 5))
        {
            strlcat( buf, ", ...", buflen );
            return;
        }

        /* Append formatted channel stop(s) to output */
        strlcat( buf, chanlist, buflen );
        curlen = newlen;
    }

    /* No channel stops at all? */
    if (buf[ curlen-1 ] == '(')
        buf[ curlen-1 ] = '0';
    else
        /* Change ending comma to closing paren instead */
        buf[ curlen-1 ] = ')';
}

/*-------------------------------------------------------------------*/
/* Format printer FCB information                                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT void FormatFCB( char* buf, size_t buflen,
                           int index, int lpi, int lpp,
                           int* fcb )
{
    int line;
    size_t curlen, newlen;
    char sep, chan[16];

    /* Get started */
    snprintf( buf, buflen, "index=%d lpi=%d lpp=%d fcb",
        index, lpi, lpp );
    curlen = strlen( buf );
    sep = '=';

    /* Format the "fcb=" values... */
    for (line=1; line <= lpp; line++)
    {
        /* Skip lines with no channel stop defined */
        if (!fcb[line])
            continue;

        /* Format channel stop */
        MSGBUF( chan, "%c%d:%d", sep, fcb[line], line );
        sep = ',';

        /* Truncate and return if out of buffer space */
        if ((newlen = curlen + strlen( chan )) >= (buflen - 5))
        {
            strlcat( buf, ", ...", buflen );
            return;
        }

        /* Append formatted channel stop to output */
        strlcat( buf, chan, buflen );
        curlen = newlen;
    }

    /* No channel stops at all? */
    if (sep == '=')
        strlcat( buf, "=0", buflen );
}

/*-------------------------------------------------------------------*/
/* Count number of tokens in a string                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT int tkcount( const char* str, const char* delims )
{
    char* w;        // (work copy of passed str)
    char* p;        // (work ptr)
    int   k;        // (number of tokens counted)

    /* Make private copy of string, keep tokenizing it until no more
       tokens, counting as we go, then free temporary work string. */
    w = strdup( str );
    for (k=0, p=strtok( w, delims ); p; k++, p=strtok( NULL, delims ));
    free( w );
    return k;
}

/*-------------------------------------------------------------------*/
/* Remove leading chars from a string and return str                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT char* ltrim( char* str, const char* dlm )   // (Left trim)
{
    char* p1;       // (points to first non-dlm)
    char* p2;       // (work for shifting to remove leading dlm's)
    const char* d;  // (iterates through dlm's)

    /* Skip leading dlm's to locate first non-dlm character */
    for (p1=str, d=dlm; *p1 && *d; )
    {
        if (*p1 == *d)      // (is this one of the delimiters?)
            p1++, d=dlm;    // (yes, get past it and reset scan)
        else
            d++;            // (no, check next delimiter if any)
    }

    /* Shift left remaining chars left via p2 */
    for (p2=str; *p1; *p2++ = *p1++);

    /* Terminate string */
    *p2 = 0;

    return str;
}

/*-------------------------------------------------------------------*/
/* Remove trailing chars from a string and return str                */
/*-------------------------------------------------------------------*/
DLL_EXPORT char* rtrim( char* str, const char* dlm )   // (right trim)
{
    char* p1;       // (work ptr for iterating backward thru string)
    const char* d;  // (work ptr for iterating through delimiters)

    /* Point to last char of str */
    p1 = str + strlen( str ) - 1;

    /* Replace all trailing dlm's with nulls */
    for (d=dlm; p1 >= str && *d; )
    {
        if (*p1 == *d)          // (is this one of the delimiters?)
            *p1-- = 0, d=dlm;   // (yes, remove it and reset scan)
        else
            d++;                // (no, check next delimiter if any)
    }

    return str;
}

/*-------------------------------------------------------------------*/
/* Remove leading and trailing chars from a string and return str    */
/*-------------------------------------------------------------------*/
DLL_EXPORT char* trim( char* str, const char* dlm )   // (trim both)
{
    return rtrim( ltrim( str, dlm ), dlm );
}

#if defined( HAVE_PTHREAD_SETNAME_NP ) // !defined( _MSVC_ ) implied
/*-------------------------------------------------------------------*/
/* Set thead name           (nonstandard GNU extension)              */
/*                          (note: retcode is error code, NOT errno) */
/*-------------------------------------------------------------------*/
DLL_EXPORT int nix_set_thread_name( pthread_t tid, const char* name )
{
    int rc = 0;
    char threadname[16];

    if (!name) return 0;            /* (ignore premature calls) */
    STRLCPY( threadname, name );

    /* Some platforms (e.g. Mac) can only set name of current thread */

#if defined( PTHREAD_SET_NAME_ONLY )
    if (!pthread_equal( tid, pthread_self()))
        return EINVAL;
    rc = pthread_setname_np( threadname );
#elif defined( PTHREAD_SET_NAME_3ARGS )
    rc = pthread_setname_np( tid, "%s", threadname );
#else
    rc = pthread_setname_np( tid, threadname );
#endif

    /* Ignore any error; it either worked or it didn't */
    return rc;
}

#endif // !defined( _MSVC_ )

/*-------------------------------------------------------------------*/
/* Subroutine to parse an argument string. The string that is passed */
/* is modified in-place by inserting null characters at the end of   */
/* each argument found. The returned array of argument pointers      */
/* then points to each argument found in the original string. Any    */
/* argument (except the first one) that begins with '#' character    */
/* is considered a line comment and causes early termination of      */
/* parsing and is not included in the argument count. If the first   */
/* argument begins with a '#' character, it is treated as a command  */
/* and parsing continues as normal, thus allowing comments to be     */
/* processed as commands. Any argument beginning with a double quote */
/* or single apostrophe causes all characters up to the next quote   */
/* or apostrophe to be included as part of that argument. The quotes */
/* and/or apostrophes themselves are not considered to be a part of  */
/* the argument itself and are replaced with nulls.                  */
/* p            Points to string to be parsed.                       */
/* maxargc      Maximum allowable number of arguments. (Prevents     */
/*              overflowing the pargv array)                         */
/* pargv        Pointer to buffer for argument pointer array.        */
/* pargc        Pointer to number of arguments integer result.       */
/* Returns number of arguments found. (same value as at *pargc)      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int parse_args( char* p, int maxargc, char** pargv, int* pargc )
{
    *pargc = 0;
    *pargv = NULL;

    while (*p && *pargc < maxargc)
    {
        while (*p && isspace(*p))
        {
            p++;
        }
        if (!*p)
        {
            break; /* find start of arg */
        }

        if (*p == '#' && *pargc)
        {
            break; /* stop when line comment reached */
        }

        /* Count new arg */
        *pargv = p;
        ++*pargc;

        while (*p && !isspace(*p) && *p != '\"' && *p != '\'')
        {
            p++;
        }
        if (!*p)
        {
            break; /* find end of arg */
        }

        if (*p == '\"' || *p == '\'')
        {
            char delim = *p;
            if (p == *pargv)
            {
                *pargv = p+1;
            }
            do {} while (*++p && *p != delim);
            if (!*p)
            {
                break; /* find end of quoted string */
            }
        }

        *p++ = 0; /* mark end of arg */
        pargv++; /* next arg ptr */
    }

    return *pargc;
}

/*-------------------------------------------------------------------*/
/* Ensure all calls to "rand()" return a hopefully unique value      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void init_random()
{
    struct timeval tv = {0};
    gettimeofday( &tv, NULL );
    srand( (U32) tv.tv_usec );
}

/*-------------------------------------------------------------------*/
/* Check if string is numeric                                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool is_numeric( const char* str )
{
    return is_numeric_l( str, strlen( str ));
}

DLL_EXPORT bool is_numeric_l( const char* str, int len )
{
    int  i;
    for (i=0; i < len; i++)
        if (str[i] < '0' || str[i] > '9')
            return false;
    return true;
}

/*-------------------------------------------------------------------*/
/* Subroutines to convert strings to upper or lower case             */
/*-------------------------------------------------------------------*/
DLL_EXPORT void string_to_upper( char* source )
{
    int  i;
    for (i=0; source[i]; i++)
        source[i] = toupper( source[i] );
}
DLL_EXPORT void string_to_lower( char* source )
{
    int  i;
    for (i=0; source[i]; i++)
        source[i] = tolower( source[i] );
}

/*-------------------------------------------------------------------*/
/* Subroutine to convert a string to EBCDIC and pad with blanks      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void convert_to_ebcdic( BYTE* dest, int len, const char* source )
{
    int  i;
    for (i=0; i < len && source[i]; i++)
        dest[i] = host_to_guest( source[i] );
    while (i < len)
        dest[i++] = 0x40;
}

/*-------------------------------------------------------------------*/
/* Subroutine to convert an EBCDIC string to an ASCIIZ string.       */
/* Removes trailing blanks and adds a terminating null.              */
/* Returns the length of the ASCII string excluding terminating null */
/*-------------------------------------------------------------------*/
DLL_EXPORT int make_asciiz( char* dest, int destlen, BYTE* src, int srclen )
{
    int  len;
    for (len=0; len < srclen && len < destlen-1; len++)
        dest[len] = guest_to_host( src[len] );
    while (len > 0 && dest[len-1] == SPACE)
        len--;
    dest[len] = 0;
    return len;
}
