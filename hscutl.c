/*  HSCUTL.C    (C) Copyright Ivan Warren & Others, 2003-2012        */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              (C) and others 2011-2023                             */
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
            if(!lstarted && isspace((unsigned char)c)) continue;
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

/* Hercules microsecond sleep */
DLL_EXPORT int herc_usleep( useconds_t usecs, const char* file, int line )
{
    int  rc, save_errno = 0, eintr_retrys = 0;

    struct timespec usecs_left;
    usecs_left.tv_nsec = usecs * 1000L;
    usecs_left.tv_sec  = usecs_left.tv_nsec / ONE_BILLION;
    usecs_left.tv_nsec %=                     ONE_BILLION;

    while (1
        && (rc = nanosleep( &usecs_left, &usecs_left ) != 0)
        && (save_errno = errno) == EINTR
        && (0
            || usecs_left.tv_sec != 0
            || usecs_left.tv_nsec > (USLEEP_MIN * 1000)
           )
    )
        ++eintr_retrys; // (keep retrying until done or min reached)

    if (0
        || rc != 0
        || eintr_retrys > NANOSLEEP_EINTR_RETRY_WARNING_TRESHOLD
    )
    {
        char fnc[128], msg[128];

        MSGBUF( fnc, "USLEEP() at %s(%d)",
            TRIMLOC( file ), line );

        if (save_errno != EINTR)
        {
            MSGBUF( msg, "rc=%d, errno=%d: %s",
                rc, save_errno, strerror( save_errno ));

            // "Error in function %s: %s"
            WRMSG( HHC00075, "E", fnc, msg );
            errno = save_errno;
        }

        if (eintr_retrys > NANOSLEEP_EINTR_RETRY_WARNING_TRESHOLD)
        {
            MSGBUF( msg, "rc=%d, EINTR retrys count=%d",
                rc, eintr_retrys );

            // "Warning in function %s: %s"
            WRMSG( HHC00092, "W", fnc, msg );
        }
    }
    return rc;
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
    tcpproto = getprotobyname("tcp");
    if (!tcpproto)
    {
        tcpproto = getprotobyname("TCP");
        if (!tcpproto)
        {
            WRMSG( HHC02219, "E", "getprotobyname(\"tcp\")", strerror( HSO_errno ));
            return -1;
        }
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
    tcpproto = getprotobyname("tcp");
    if (!tcpproto)
    {
        tcpproto = getprotobyname("TCP");
        if (!tcpproto)
        {
            WRMSG( HHC02219, "E", "getprotobyname(\"tcp\")", strerror( HSO_errno ));
            return -1;
        }
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
    sysblk.sysgroup = DEFAULT_SYSGROUP;

    initialize_detach_attr( DETACHED );
    initialize_join_attr( JOINABLE );
    initialize_lock( &sysblk.dasdcache_lock );
    initialize_lock( &sysblk.scrlock );
    initialize_condition( &sysblk.scrcond );

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
        while (*p && isspace((unsigned char)*p))
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

        while (*p && !isspace((unsigned char)*p) && *p != '\"' && *p != '\'')
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
/* Check if string is numeric or hexadecimal                         */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool is_numeric( const char* str )
{
    return is_numeric_l( str, strlen( str ));
}
DLL_EXPORT bool is_numeric_l( const char* str, size_t len )
{
    size_t i;
    for (i=0; i < len; i++)
        if (!(str[i] >= '0' && str[i] <= '9'))
            return false;
    return len ? true : false; // (empty string is not numeric)
}
DLL_EXPORT bool is_hex( const char* str )
{
    return is_hex_l( str, strlen( str ));
}
DLL_EXPORT bool is_hex_l( const char* str, size_t len )
{
    size_t i;
    for (i=0; i < len; i++)
        if (!(0
            || (str[i] >= '0' && str[i] <= '9')
            || (str[i] >= 'a' && str[i] <= 'f')
            || (str[i] >= 'A' && str[i] <= 'F')
        ))
            return false;
    return len ? true : false; // (empty string is not hex)
}

/*-------------------------------------------------------------------*/
/* Subroutines to convert strings to upper or lower case             */
/*-------------------------------------------------------------------*/
DLL_EXPORT void string_to_upper( char* source )
{
    int  i;
    for (i=0; source[i]; i++)
        source[i] = toupper( (unsigned char)source[i] );
}
DLL_EXPORT void string_to_lower( char* source )
{
    int  i;
    for (i=0; source[i]; i++)
        source[i] = tolower( (unsigned char)source[i] );
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

/*-------------------------------------------------------------------*/
/*                        idx_snprintf                               */
/*      Fix for "Potential snprintf buffer overflow" #457            */
/*-------------------------------------------------------------------*/
/* This function is a replacement for code that builds a message     */
/* piecemeal (a little bit at a time) via a series of snprint calls  */
/* indexing each time a little bit further into the message buffer   */
/* until the entire message is built:                                */
/*                                                                   */
/*      char buf[64];                                                */
/*      size_t bufl = sizeof( buf );                                 */
/*      int n = 0;                                                   */
/*                                                                   */
/*      n =  snprintf( buf,   bufl,   "%part1", part1 );             */
/*      n += snprintf( buf+n, bufl-n, "%part2", part2 );             */
/*      n += snprintf( buf+n, bufl-n, "%part3", part3 );             */
/*      ...                                                          */
/*      n += snprintf( buf+n, bufl-n, "%lastpart", lastpart );       */
/*                                                                   */
/*      WRMSG( HHCnnnn, "I", buf );                                  */
/*                                                                   */
/* The problem with the above code is there is no check to ensure    */
/* a buffer overflow does not occur due to trying to format more     */
/* data than the buffer can hold.                                    */
/*                                                                   */
/* The idx_snprintf function accomplishes the same thing but without */
/* overflowing the buffer. It checks to ensure the idx buffer offset */
/* value never exceeds the size of the buffer.                       */
/*                                                                   */
/* The return value is the same as what snprintf returns, BUT THE    */
/* BUFFER ADDRESS AND SIZE PASSED TO THE FUNCTION is the *original*  */
/* address and size of the buffer (i.e. the raw buf and bufl value   */
/* but NOT offset by the index):                                     */
/*                                                                   */
/*      char buf[64];                                                */
/*      size_t bufl = sizeof( buf );                                 */
/*      int n = 0;                                                   */
/*                                                                   */
/*      n =  idx_snprintf( n, buf, bufl, "%part1", part1 );          */
/*      n += idx_snprintf( n, buf, bufl, "%part2", part2 );          */
/*      n += idx_snprintf( n, buf, bufl, "%part3", part3 );          */
/*      ...                                                          */
/*      n += idx_snprintf( n, buf, bufl, "%lastpart", lastpart );    */
/*                                                                   */
/*      WRMSG( HHCnnnn, "I", buf );                                  */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int  idx_snprintf( int idx, char* buffer, size_t bufsiz, const char* fmt, ... )
{
    int rc;
    va_list vargs;

    if (idx < 0 || !bufsiz || (size_t)idx >= bufsiz)
    {
        // "Error in function %s: %s"
        WRMSG( HHC00075, "W", "idx_snprintf", "OVERFLOW (bufsiz too small)" );
        idx = (int)(bufsiz ? bufsiz-1 : 0); /* (prevent buffer overflow) */
    }

    va_start( vargs, fmt );
    rc = vsnprintf( buffer+idx, bufsiz-idx, fmt, vargs );
    return rc;
}

/*-------------------------------------------------------------------
 * Elbrus e2k
 *-------------------------------------------------------------------*/
#if defined(__e2k__) && defined(__LCC__)

volatile char __e2k_lockflags[__E2K_LOCKFLAGS_SIZE];

int __cmpxchg16_e2k(volatile char *lockflag, U64 *old1, U64 *old2, U64 new1,
                    U64 new2, volatile void *ptr)
{
    // returns 0 == success, 1 otherwise
    int result;

    while (__atomic_test_and_set(lockflag, __ATOMIC_ACQUIRE) == 1)
        ;

    if (*old1 == *(U64 *)ptr && *old2 == *((U64 *)ptr + 1)) {
        *(U64 *)ptr = new1;
        *((U64 *)ptr + 1) = new2;
        result = 0;
    } else {
        *old1 = *((U64 *)ptr);
        *old2 = *((U64 *)ptr + 1);
        result = 1;
    }

    __atomic_clear(lockflag, __ATOMIC_RELEASE);

    return result;
}
#endif /* e2k && LCC */

/*-------------------------------------------------------------------*/
/*     Processor types table and associated query functions          */
/*-------------------------------------------------------------------*/
struct PTYPTAB
{
    const BYTE  ptyp;           // 1-byte processor type (service.h)
    const char* shortname;      // 2-character short name (Hercules)
    const char* longname;       // 16-character long name (diag 224)
};
typedef struct PTYPTAB PTYPTAB;

static PTYPTAB ptypes[] =
{
    { SCCB_PTYP_CP,      "CP", "CP              " },  // 0
    { SCCB_PTYP_UNKNOWN, "??", "                " },  // 1 (unknown == blanks)
    { SCCB_PTYP_ZAAP,    "AP", "ZAAP            " },  // 2
    { SCCB_PTYP_IFL,     "IL", "IFL             " },  // 3
    { SCCB_PTYP_ICF,     "CF", "ICF             " },  // 4
    { SCCB_PTYP_ZIIP,    "IP", "ZIIP            " },  // 5
};

DLL_EXPORT const char* ptyp2long( BYTE ptyp )
{
    unsigned int i;
    for (i=0; i < _countof( ptypes ); i++)
        if (ptypes[i].ptyp == ptyp)
            return ptypes[i].longname;
    return "                ";              // 16 blanks
}

DLL_EXPORT const char* ptyp2short( BYTE ptyp )
{
    unsigned int i;
    for (i=0; i < _countof( ptypes ); i++)
        if (ptypes[i].ptyp == ptyp)
            return ptypes[i].shortname;
    return "??";
}

DLL_EXPORT BYTE short2ptyp( const char* shortname )
{
    unsigned int i;
    for (i=0; i < _countof( ptypes ); i++)
        if (strcasecmp( ptypes[i].shortname, shortname ) == 0)
            return ptypes[i].ptyp;
    return SCCB_PTYP_UNKNOWN;
}

DLL_EXPORT U64 do_make_psw64( PSW* psw, BYTE real_ilc, int arch /*370/390/900*/, bool bc )
{
    /* Return first/only 64-bit DWORD of the PSW -- IN HOST FORMAT!

       Caller is responsible for doing the STORE_DW on the returned
       value, which does a CSWAP to place it into guest storage in
       proper big endian format. The 900 mode caller (i.e. z/Arch)
       is also responsible for doing the store of the second DWORD
       of the 16-byte z/Arch PSW = the 64-bit instruction address.
    */

    BYTE  b0, b1, b2, b3, b4;
    U16   b23;
    U32   b567, b4567;
    U64   psw64 = 0;

    switch (arch)
    {
        case 370:

        if (bc) {
            //                      370 BC-mode
            //
            //     +---------------+---+-----+------+---------------+----
            //     | channel masks | E | key | 0MWP | interupt code | ..
            //     +---------------+---+-----+------+---------------+----
            //     0               7   8     12     16             31

            //  ---+-----+----+------+------------------------------+
            //   ..| ilc | cc | mask |     instruction address      |
            //  ---+-----+----+------+------------------------------+
            //     32    34   36     40                            63

            b0 = psw->sysmask;

            b1 = 0
                 | psw->pkey
                 | psw->states
                 ;

            b23 = psw->intcode;

            b4 = 0
                 | ((real_ilc >> 1) << 6)   // CAREFUL! Herc's ilc value
                                            // is the ACTUAL length (2,
                                            // 4 or 6), but the value in
                                            // the PSW is either 1,2,3!)
                 | (psw->cc << 4)
                 |  psw->progmask
                 ;

            b567 = psw->IA_L;

            if (!psw->zeroilc)
                b567 &= AMASK24;

            psw64 = 0
                    | ( (U64) b0   << (64-(1*8)) )
                    | ( (U64) b1   << (64-(2*8)) )
                    | ( (U64) b23  << (64-(4*8)) )
                    | ( (U64) b4   << (64-(5*8)) )
                    | ( (U64) b567 << (64-(8*8)) )
                    ;
            break;
        }

        /* Not 370 BC-mode = 370 EC-mode. Fall through to the 390 case,
           which handles both ESA/390 mode and S/370 EC-mode PSWs too.

           The below special "FALLTHRU" comment lets GCC know that we are
           purposely falling through to the next switch case and is needed
           in order to suppress the warning that GCC would otherwise issue.
        */
        /* FALLTHRU */

        case 390:
            //                      370 EC-mode
            //
            //     +------+------+-----+------+----+----+------+----------+---
            //     | 0R00 | 0TIE | key | 1MWP | S0 | cc | mask | 00000000 | ..
            //     +------+------+-----+------+----+----+------+----------+---
            //     0      4      8     12     16   18   20     24

            //  ---+----------+-------------------------------------------+
            //   ..| 00000000 |       instruction address                 |  (370)
            //  ---+----------+-------------------------------------------+
            //     32         40                                         63
            //
            //                        ESA/390
            //
            //     +------+------+-----+------+----+----+------+----------+---
            //     | 0R00 | 0TIE | key | 1MWP | AS | cc | mask | 00000000 | ..
            //     +------+------+-----+------+----+----+------+----------+---
            //     0      4      8     12     16   18   20     24
            //
            //  ---+---+--------------------------------------------------+
            //   ..| A |              instruction address                 |  (390)
            //  ---+---+--------------------------------------------------+
            //     32  33                                                63

            b0 = psw->sysmask;

            b1 = 0
                 | psw->pkey
                 | psw->states
                 ;

            b2 = 0
                 |  psw->asc           // (S0 or AS)
                 | (psw->cc << 4)
                 |  psw->progmask
                 ;

            b3 = psw->zerobyte;

            b4567 = psw->IA_L;

            if (!psw->zeroilc)
                b4567 &= psw->amode ? AMASK31 : AMASK24;

            if (psw->amode)
                b4567 |= 0x80000000;

            psw64 = 0
                    | ( (U64) b0    << (64-(1*8)) )
                    | ( (U64) b1    << (64-(2*8)) )
                    | ( (U64) b2    << (64-(3*8)) )
                    | ( (U64) b3    << (64-(4*8)) )
                    | ( (U64) b4567 << (64-(8*8)) )
                    ;
            break;

        case 900:
            //                      z/Architecture
            //
            //     +------+------+-----+------+----+----+------+-----------+---
            //     | 0R00 | 0TIE | key | 0MWP | AS | cc | mask | 0000 000E | ..
            //     +------+------+-----+------+----+----+------+-----------+---
            //     0      4      8     12     16   18   20     24         31
            //
            //  ---+---+---------------------------------------------------+
            //   ..| B | 0000000000000000000000000000000000000000000000000 |
            //  ---+---+---------------------------------------------------+
            //     32  33                                                 63

            b0 = psw->sysmask;

            b1 = 0
                 | psw->pkey
                 | psw->states
                 ;

            b2 = 0
                 |  psw->asc
                 | (psw->cc << 4)
                 |  psw->progmask
                 ;

            b3 = psw->zerobyte;

            if (psw->amode64)
                b3 |= 0x01;

            b4567 = psw->zeroword;

            if (psw->amode)
                b4567 |= 0x80000000;

            psw64 = 0
                    | ( (U64) b0    << (64-(1*8)) )
                    | ( (U64) b1    << (64-(2*8)) )
                    | ( (U64) b2    << (64-(3*8)) )
                    | ( (U64) b3    << (64-(4*8)) )
                    | ( (U64) b4567 << (64-(8*8)) )
                    ;
            break;

        default:        // LOGIC ERROR!
            CRASH();    // LOGIC ERROR!
    }
    return psw64;
}

/*-------------------------------------------------------------------*/
/*              Return Program Interrupt Name                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* PIC2Name( int pcode )
{
    static const char* pgmintname[] =
    {
        /* 01 */    "Operation exception",
        /* 02 */    "Privileged-operation exception",
        /* 03 */    "Execute exception",
        /* 04 */    "Protection exception",
        /* 05 */    "Addressing exception",
        /* 06 */    "Specification exception",
        /* 07 */    "Data exception",
        /* 08 */    "Fixed-point-overflow exception",
        /* 09 */    "Fixed-point-divide exception",
        /* 0A */    "Decimal-overflow exception",
        /* 0B */    "Decimal-divide exception",
        /* 0C */    "HFP-exponent-overflow exception",
        /* 0D */    "HFP-exponent-underflow exception",
        /* 0E */    "HFP-significance exception",
        /* 0F */    "HFP-floating-point-divide exception",
        /* 10 */    "Segment-translation exception",
        /* 11 */    "Page-translation exception",
        /* 12 */    "Translation-specification exception",
        /* 13 */    "Special-operation exception",
        /* 14 */    "Pseudo-page-fault exception",
        /* 15 */    "Operand exception",
        /* 16 */    "Trace-table exception",
        /* 17 */    "ASN-translation exception",
        /* 18 */    "Transaction constraint exception",
        /* 19 */    "Vector/Crypto operation exception",
        /* 1A */    "Page state exception",
        /* 1B */    "Vector processing exception",
        /* 1C */    "Space-switch event",
        /* 1D */    "Square-root exception",
        /* 1E */    "Unnormalized-operand exception",
        /* 1F */    "PC-translation specification exception",
        /* 20 */    "AFX-translation exception",
        /* 21 */    "ASX-translation exception",
        /* 22 */    "LX-translation exception",
        /* 23 */    "EX-translation exception",
        /* 24 */    "Primary-authority exception",
        /* 25 */    "Secondary-authority exception",
        /* 26 */ /* "Page-fault-assist exception",          */
        /* 26 */    "LFX-translation exception",
        /* 27 */ /* "Control-switch exception",             */
        /* 27 */    "LSX-translation exception",
        /* 28 */    "ALET-specification exception",
        /* 29 */    "ALEN-translation exception",
        /* 2A */    "ALE-sequence exception",
        /* 2B */    "ASTE-validity exception",
        /* 2C */    "ASTE-sequence exception",
        /* 2D */    "Extended-authority exception",
        /* 2E */    "LSTE-sequence exception",
        /* 2F */    "ASTE-instance exception",
        /* 30 */    "Stack-full exception",
        /* 31 */    "Stack-empty exception",
        /* 32 */    "Stack-specification exception",
        /* 33 */    "Stack-type exception",
        /* 34 */    "Stack-operation exception",
        /* 35 */    "Unassigned exception",
        /* 36 */    "Unassigned exception",
        /* 37 */    "Unassigned exception",
        /* 38 */    "ASCE-type exception",
        /* 39 */    "Region-first-translation exception",
        /* 3A */    "Region-second-translation exception",
        /* 3B */    "Region-third-translation exception",
        /* 3C */    "Unassigned exception",
        /* 3D */    "Unassigned exception",
        /* 3E */    "Unassigned exception",
        /* 3F */    "Unassigned exception",
        /* 40 */    "Monitor event"
    };

    int ndx, code = (pcode & 0xFF);

    if (code == 0x80)
        return "PER event";

    if (code < 1 || code > (int) _countof( pgmintname ))
        return "Unassigned exception";

    ndx = ((code - 1) & 0x3F);

    return (ndx >= 0 && ndx < (int) _countof( pgmintname )) ?
        pgmintname[ ndx ] : "Unassigned exception";
}

/*-------------------------------------------------------------------*/
/*                 Return SIGP Order Name                            */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* order2name( BYTE order )
{
static const char *ordername[] =
{
/* 0x00                          */  "Unassigned",
/* 0x01 SIGP_SENSE               */  "Sense",
/* 0x02 SIGP_EXTCALL             */  "External call",
/* 0x03 SIGP_EMERGENCY           */  "Emergency signal",
/* 0x04 SIGP_START               */  "Start",
/* 0x05 SIGP_STOP                */  "Stop",
/* 0x06 SIGP_RESTART             */  "Restart",
/* 0x07 SIGP_IPR                 */  "Initial program reset",
/* 0x08 SIGP_PR                  */  "Program reset",
/* 0x09 SIGP_STOPSTORE           */  "Stop and store status",
/* 0x0A SIGP_IMPL                */  "Initial microprogram load",
/* 0x0B SIGP_INITRESET           */  "Initial CPU reset",
/* 0x0C SIGP_RESET               */  "CPU reset",
/* 0x0D SIGP_SETPREFIX           */  "Set prefix",
/* 0x0E SIGP_STORE               */  "Store status",
/* 0x0F                          */  "Unassigned",
/* 0x10                          */  "Unassigned",
/* 0x11 SIGP_STOREX              */  "Store extended status at address",
/* 0x12 SIGP_SETARCH             */  "Set architecture mode",
/* 0x13 SIGP_COND_EMERGENCY      */  "Conditional emergency",
/* 0x14                          */  "Unassigned",
/* 0x15 SIGP_SENSE_RUNNING_STATE */  "Sense running state"
};
    return (order >= _countof( ordername )) ?
        "Unassigned" : ordername[ order ];
}

/*-------------------------------------------------------------------*/
/*                 Return PER Event name(s)                          */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* perc2name( BYTE perc, char* buf, size_t bufsiz )
{   /*
           Hex    Bit   PER Event
            80     0    Successful-branching
            40     1    Instruction-fetching
            20     2    Storage-alteration
            10     3    Storage-key-alteration
            08     4    Store-using-real-address
            04     5    Zero-address-detection
            02     6    Transaction-end
            01     7    Instruction-fetching nullification (PER-3) */

    const char* name;

    switch (perc)
    {
        /*-----------------------------------------------------------*/
        /*               Basic single-bit events...                  */
        /*-----------------------------------------------------------*/

        case 0x80:
        {
            name = "BRANCH";
            break;
        }

        case 0x40:
        {
            name = "IFETCH";
            break;
        }

        case 0x20:
        {
            name = "STOR";
            break;
        }

        case 0x10:
        {
            /* NOTE: docs say: "A zero is stored in bit position 3 of
               locations 150-151", so this SHOULDN'T(?) occur, but if
               for some reason it does, let's return an indication.
            */
            name = "SKEY";
            break;
        }

#if 0 // (probably illegal? i.e. should never occur? See 0x28 below!)

        case 0x08:
        {
            name = "STURA";
            break;
        }
#endif
        case 0x04:
        {
            name = "ZAD";
            break;
        }

        case 0x02:
        {
            name = "TEND";
            break;
        }

        case 0x01:
        {
            name = "IFNULL";
            break;
        }

        /*-----------------------------------------------------------*/
        /*    Exceptions to the rule requiring special handling      */
        /*-----------------------------------------------------------*/

        case 0x24:
        {
            /* "... when ... a storage-alteration ... event is re-
               cognized concurrently with a zero-address-detection
               event, only the storage alteration ... event is in-
               dicated." (Not sure whether that means ONLY the 0x20
               storage alteration bit is set, or whether the "event"
               should simply be TREATED AS a storage event, so to
               be safe, we'll code for it.)
            */
            name = "STOR";
            break;
        }

        case 0x28:
        {
            /* "... while ones in bit positions 2 and 4 indicate a
               store-using-real-address event."
            */
            name = "STURA";
            break;
        }

#if 0 // (probbaly illegal? i.e. should never occur?)

        case 0x2C:
        {
            /* "... when ... store-using-real-address event is re-
               cognized concurrently with a zero-address-detection
               event, only the ... store-using-real-address event
               is indicated."
            */
            name = "STURA";
            break;
        }
#endif
        case 0x41:
        {
            /* "When a program interruption occurs for a PER instruc-
               tion fetching nullification event, bits 1 and 7 are set
               to one in the PER code. No other PER events are concur-
               rently indicated.
            */
            name = "IFETCH+IFNULL";
            break;
        }

        case 0x42:
        {
            /* "If an instruction-fetching basic event coincides with
               the transaction-end event, bit 1 is also set to one
               in the PER code."
            */
            name = "IFETCH+TEND";
            break;
        }

        default:
        {
            name = "UNKNOWN!";
            break;
        }
    }

    strlcpy( buf, name, bufsiz );
    return buf;
}

/*-------------------------------------------------------------------*/
/*      Format Operation-Request Block (ORB) for display             */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* FormatORB( ORB* orb, char* buf, size_t bufsz )
{
    if (!buf)
        return NULL;

    if (bufsz)
        *buf = 0;

    if (bufsz <= 1 || !orb)
        return buf;

    snprintf( buf, bufsz,

        "IntP:%2.2X%2.2X%2.2X%2.2X Key:%d LPM:%2.2X "
        "Flags:%X%2.2X%2.2X %c%c%c%c%c%c%c%c%c%c%c%c %c%c.....%c "
        "%cCW:%2.2X%2.2X%2.2X%2.2X"

        , orb->intparm[0], orb->intparm[1], orb->intparm[2], orb->intparm[3]
        , (orb->flag4 & ORB4_KEY) >> 4
        , orb->lpm

        , (orb->flag4 & ~ORB4_KEY)
        , orb->flag5
        , orb->flag7

        , ( orb->flag4 & ORB4_S ) ? 'S' : '.'
        , ( orb->flag4 & ORB4_C ) ? 'C' : '.'
        , ( orb->flag4 & ORB4_M ) ? 'M' : '.'
        , ( orb->flag4 & ORB4_Y ) ? 'Y' : '.'

        , ( orb->flag5 & ORB5_F ) ? 'F' : '.'
        , ( orb->flag5 & ORB5_P ) ? 'P' : '.'
        , ( orb->flag5 & ORB5_I ) ? 'I' : '.'
        , ( orb->flag5 & ORB5_A ) ? 'A' : '.'

        , ( orb->flag5 & ORB5_U ) ? 'U' : '.'
        , ( orb->flag5 & ORB5_B ) ? 'B' : '.'
        , ( orb->flag5 & ORB5_H ) ? 'H' : '.'
        , ( orb->flag5 & ORB5_T ) ? 'T' : '.'

        , ( orb->flag7 & ORB7_L ) ? 'L' : '.'
        , ( orb->flag7 & ORB7_D ) ? 'D' : '.'
        , ( orb->flag7 & ORB7_X ) ? 'X' : '.'

        , ( orb->flag5 & ORB5_B ) ? 'T' : 'C'  // (TCW or CCW)

        , orb->ccwaddr[0], orb->ccwaddr[1], orb->ccwaddr[2], orb->ccwaddr[3]
    );

    return buf;
}

/*-------------------------------------------------------------------*/
/*      Determine if running on a big endian system or not           */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool are_big_endian()
{
    static union
    {
        uint32_t  ui32;
        char      b[4];
    }
    test = {0x01020304};
    return (0x01 == test.b[0]);
}

/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**                    TraceFile support                            **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

/*-------------------------------------------------------------------*/
/*    Determine if endian swaps are going to be needed or not        */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool tf_are_swaps_needed( TFSYS* sys )
{
    return
    (0
        || (1
            && are_big_endian()     // (we are big endian)
            && sys->bigend == 0     // (but file is little endian)
           )
        || (1
            && !are_big_endian()    // (we are little endian)
            && sys->bigend == 1     // (but file is big endian)
           )
    );
}

//---------------------------------------------------------------------
//       (helper function to call correct ARCH_DEP function)
//---------------------------------------------------------------------
static int tf_virt_to_real( U64* raptr, int* siptr, U64 vaddr, int arn, REGS* regs, int acctype )
{
    switch (regs->arch_mode)
    {
#if defined(       _370 )
        case   ARCH_370_IDX:
            return sysblk.s370_vtr( raptr, siptr, vaddr, arn, regs, acctype );
#endif
#if defined(       _390 )
        case   ARCH_390_IDX:
            return sysblk.s390_vtr( raptr, siptr, vaddr, arn, regs, acctype );
#endif
#if defined(       _900 )
        case   ARCH_900_IDX:
            return sysblk.z900_vtr( raptr, siptr, vaddr, arn, regs, acctype );
#endif
        default: CRASH();
            UNREACHABLE_CODE( return 0 );
    }
}

//---------------------------------------------------------------------
//       (helper function to call correct ARCH_DEP function)
//---------------------------------------------------------------------
static BYTE tf_get_storage_key( REGS* regs, U64 abs )
{
    switch (regs->arch_mode)
    {
#if defined(       _370 )
        case   ARCH_370_IDX:
            return sysblk.s370_gsk( abs );
#endif
#if defined(       _390 )
        case   ARCH_390_IDX:
            return sysblk.s390_gsk( abs );
#endif
#if defined(       _900 )
        case   ARCH_900_IDX:
            return sysblk.z900_gsk( abs );
#endif
        default: CRASH();
            UNREACHABLE_CODE( return 0 );
    }
}

//---------------------------------------------------------------------
//       (helper function to call correct ARCH_DEP function)
//---------------------------------------------------------------------
static void tf_store_int_timer( REGS* regs, U64 raddr )
{
    switch (regs->arch_mode)
    {
#if defined(     _370 )
        case ARCH_370_IDX:
            if (ITIMER_ACCESS( raddr, 16 ))
                 sysblk.s370_sit( regs );
            return;
#endif
#if defined(     _390 )
        case ARCH_390_IDX:
            //    390 doesn't have interval timers
            return;
#endif
#if defined(     _900 )
        case ARCH_900_IDX:
            //    900 doesn't have interval timers
            return;
#endif
        default: CRASH();
            UNREACHABLE_CODE( return );
    }
}

//---------------------------------------------------------------------
//       (helper function to call correct ARCH_DEP function)
//---------------------------------------------------------------------
static U64 tf_apply_prefixing( U64 radr, REGS* regs )
{
    switch (regs->arch_mode)
    {
#if defined(            _370 )
        case        ARCH_370_IDX:
            return APPLY_370_PREFIXING( radr, regs->PX_370 );
#endif
#if defined(            _390 )
        case        ARCH_390_IDX:
            return APPLY_390_PREFIXING( radr, regs->PX_390 );
#endif
#if defined(            _900 )
        case        ARCH_900_IDX:
            return APPLY_900_PREFIXING( radr, regs->PX_900 );
#endif
        default: CRASH();
            UNREACHABLE_CODE( return 0 );
    }
}

/*-------------------------------------------------------------------*/
/*                       TFSYS record                                */
/*-------------------------------------------------------------------*/
static const size_t tfsys_size = sizeof( TFSYS );
static TFSYS tfsys = {0};

/*-------------------------------------------------------------------*/
/*                Write starting TFSYS record                        */
/*-------------------------------------------------------------------*/
static bool tf_write_initial_TFSYS_locked( TIMEVAL* tod )
{
    size_t  written;
    bool    ret = true;

    /* Initialize TFSYS record */

    tfsys.ffmt[0] = '%';
    tfsys.ffmt[1] = 'T';
    tfsys.ffmt[2] = 'F';
    tfsys.ffmt[3] = TF_FMT;

    tfsys.bigend   = are_big_endian() ? 1 : 0;
    tfsys.engs     = MAX_CPU_ENGS;
    tfsys.archnum0 = _ARCH_NUM_0;
    tfsys.archnum1 = _ARCH_NUM_1;
    tfsys.archnum2 = _ARCH_NUM_2;

    memcpy( &tfsys.beg_tod, tod, sizeof( tfsys.beg_tod ));
    memset( &tfsys.end_tod,  0,  sizeof( tfsys.end_tod ));

    STRLCPY( tfsys.version, *sysblk.vers_info );
    memcpy( &tfsys.ptyp[0], &sysblk.ptyp[0], MAX_CPU_ENGS );

    if ((written = fwrite( &tfsys, 1, tfsys_size,
                sysblk.traceFILE )) < tfsys_size)
    {
        // "Error in function %s: %s"
        WRMSG( HHC00075, "E", "fwrite()", strerror( errno ));
        ret = false;
    }

    sysblk.curtracesize += written;

    // "Trace file tracing begun..."
    WRMSG( HHC02383, "I" );

    return ret;
}

/*-------------------------------------------------------------------*/
/*                     Close TraceFile                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT void tf_close_locked()
{
    /* Quick exit if already closed */
    if (!sysblk.traceFILE)
        return;

    if (fseek( sysblk.traceFILE, 0, SEEK_SET ) != 0)
    {
        // "Error in function %s: %s"
        WRMSG( HHC00075, "E", "fseek()", strerror( errno ));
    }
    else if (fwrite( &tfsys, 1, tfsys_size, sysblk.traceFILE ) < tfsys_size)
    {
        // "Error in function %s: %s"
        WRMSG( HHC00075, "E", "fwrite()", strerror( errno ));
    }

    /* Now close the file */
    VERIFY( 0 == fclose( sysblk.traceFILE ));
    sysblk.traceFILE = NULL;
    sysblk.curtracesize = 0;
}

DLL_EXPORT void tf_close( void* notused )
{
    UNREFERENCED( notused );

    OBTAIN_TRACEFILE_LOCK();
    {
        tf_close_locked();
    }
    RELEASE_TRACEFILE_LOCK();
}

/*-------------------------------------------------------------------*/
/*                 Write TraceFile record                            */
/*-------------------------------------------------------------------*/
static bool tf_write( REGS* regs, void* rec, U16 curr, U16 msgnum )
{
    static size_t  bufsize  = 0;
    static U16     prev     = 0;

    TFHDR*   hdr;
    size_t   written;
    TIMEVAL  tod;
    TID      tid;
    bool     nostop;
    bool     auto_closed = false; // (true when MAX= reached)
    bool     ret = true;

    OBTAIN_TRACEFILE_LOCK();
    {
        /* Quick exit if tracefile tracing isn't active */
        if (!sysblk.traceFILE)
        {
            RELEASE_TRACEFILE_LOCK();
            return true;
        }

        gettimeofday( (TIMEVAL*) &tod, NULL );

        /* Write TFSYS record if first time */
        if (!sysblk.curtracesize)
        {
            if (!tf_write_initial_TFSYS_locked( &tod ))
            {
                // (error message already issued)
                RELEASE_TRACEFILE_LOCK();
                return false;
            }

            bufsize = tf_MAX_RECSIZE();
            prev = 0;

            /* Register shutdown function so close updates TFSYS */
            hdl_addshut( "tf_close", tf_close, NULL );
        }

        /* Copy caller's entire record into our buffer */
        if ((size_t)curr > bufsize) CRASH();
        hdr = (TFHDR*) sysblk.tracefilebuff;
        memcpy( hdr, rec, curr );

        /* Finish building their header */
        hdr->prev      = prev;
        hdr->curr      = curr;
        hdr->cpuad     = regs ? regs->cpuad : 0xFFFF;
        hdr->msgnum    = msgnum;
        hdr->arch_mode = regs ? regs->arch_mode : 0xFF;
        hdr->tod       = tod;
        tid             = hthread_self();
        hdr->tidnum     = TID_CAST( tid );
        get_thread_name( tid, hdr->thrdname );

        /* Update the TFSYS record's end time too */
        memcpy( &tfsys.end_tod, &tod, sizeof( tfsys.end_tod ));

        /* Write caller's entire record to the trace file */
        if ((written = fwrite( hdr, 1, curr,
                sysblk.traceFILE )) <  curr)
        {
            // "Error in function %s: %s"
            WRMSG( HHC00075, "E", "fwrite()", strerror( errno ));
            ret = false;
        }

        /* Update variables for next time */
        sysblk.curtracesize += written;
        prev = curr;

        /* Count total trace records */
        if (hdr->cpuad == 0xFFFF)
            tfsys.tot_dev++;
        else
        {
            if (hdr->msgnum == 2324)
                tfsys.tot_ins++;
        }

        /* Stop tracing when MAX size exceeded */
        if (sysblk.curtracesize > sysblk.maxtracesize)
        {
            tf_close_locked();
            auto_closed = true; // (handled further below)
            nostop = sysblk.tfnostop ? true : false;
        }
    }
    RELEASE_TRACEFILE_LOCK();

    /* If file automatically closed due to MAX= being reached... */
    if (auto_closed)
    {
        /* Stop all instruction tracing too unless asked not to */
        if (nostop)
        {
            // "Trace file MAX= reached; file closed, tracing %s"
            WRMSG( HHC02379, "I", "continues" );
        }
        else
        {
            tf_autostop();

            // "Trace file MAX= reached; file closed, tracing %s"
            WRMSG( HHC02379, "I", "auto-stopped" );
        }
    }

    return ret;
}

//---------------------------------------------------------------------
//               Automatically stop all tracing
//---------------------------------------------------------------------
DLL_EXPORT bool tf_autostop()
{
    bool bWasActive;

    /* Stop all tracing */
    OBTAIN_INTLOCK( NULL );
    {
        DEVBLK* dev;
        int cpu;

        /* Stop overall instruction tracing */
        bWasActive = sysblk.insttrace ? true : false;
        sysblk.insttrace = false;

        /* Stop instruction tracing for all CPUs */
        for (cpu=0; cpu < sysblk.maxcpu; cpu++)
        {
            if (IS_CPU_ONLINE( cpu ))
            {
                bWasActive = bWasActive || sysblk.regs[ cpu ]->insttrace;
                sysblk.regs[ cpu ]->insttrace = false;
            }
        }

        /* Stop ORB/CCW/KEY tracing for all devices */
        for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        {
            bWasActive = bWasActive || dev->ccwtrace || dev->orbtrace || dev->ckdkeytrace;
            dev->ccwtrace    = false;
            dev->orbtrace    = false;
            dev->ckdkeytrace = false;
        }
    }
    RELEASE_INTLOCK( NULL );

    return bWasActive;
}

//---------------------------------------------------------------------
//                      Search key
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0423( DEVBLK* dev, size_t kl, BYTE* key )
{
    TF00423 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.kl = (BYTE) kl;
    memcpy( rec.key, key, kl );
    return tf_write( NULL, &rec, sizeof( TF00423 ), 423 );
}

//---------------------------------------------------------------------
//                      Cur trk
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0424( DEVBLK* dev, S32 trk )
{
    TF00424 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.trk    = trk;
    rec.bufcur = dev->bufcur;
    return tf_write( NULL, &rec, sizeof( TF00424 ), 424 );
}

//---------------------------------------------------------------------
//                      Updating track
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0425( DEVBLK* dev )
{
    TF00425 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.bufcur = dev->bufcur;
    return tf_write( NULL, &rec, sizeof( TF00425 ), 425 );
}

//---------------------------------------------------------------------
//                      Cache hit
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0426( DEVBLK* dev, S32 trk, S32 i )
{
    TF00426 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.trk = trk;
    rec.idx = i;
    return tf_write( NULL, &rec, sizeof( TF00426 ), 426 );
}

//---------------------------------------------------------------------
//                   Unavailable cache
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0427( DEVBLK* dev, S32 trk )
{
    TF00427 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.trk = trk;
    return tf_write( NULL, &rec, sizeof( TF00427 ), 427 );
}

//---------------------------------------------------------------------
//                      Cache miss
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0428( DEVBLK* dev, S32 trk, S32 o )
{
    TF00428 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.trk = trk;
    rec.idx = o;
    return tf_write( NULL, &rec, sizeof( TF00428 ), 428 );
}

//---------------------------------------------------------------------
//                      Offset
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0429( DEVBLK* dev, S32 trk, S32 fnum )
{
    TF00429 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.trk    = trk;
    rec.offset = dev->ckdtrkoff;
    rec.len    = dev->ckdtrksz;
    rec.fnum   = fnum;
    return tf_write( NULL, &rec, sizeof( TF00429 ), 429 );
}

//---------------------------------------------------------------------
//                      Trkhdr
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0430( DEVBLK* dev, S32 trk )
{
    TF00430 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.trk = trk;
    memcpy( rec.buf, dev->buf, sizeof( rec.buf ));
    return tf_write( NULL, &rec, sizeof( TF00430 ), 430 );
}

//---------------------------------------------------------------------
//                      Seeking
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0431( DEVBLK* dev, int cyl, int head )
{
    TF00431 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.cyl  = cyl;
    rec.head = head;
    return tf_write( NULL, &rec, sizeof( TF00431 ), 431 );
}

//---------------------------------------------------------------------
//                      MT advance error
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0432( DEVBLK* dev )
{
    TF00432 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.count = dev->ckdlcount;
    rec.mask  = dev->ckdfmask;
    return tf_write( NULL, &rec, sizeof( TF00432 ), 432 );
}

//---------------------------------------------------------------------
//                      MT advance
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0433( DEVBLK* dev, int cyl, int head )
{
    TF00433 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.cyl  = cyl;
    rec.head = head;
    return tf_write( NULL, &rec, sizeof( TF00433 ), 433 );
}

//---------------------------------------------------------------------
//                   Read count orientation
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0434( DEVBLK* dev )
{
    TF00434 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.orient = (BYTE) dev->ckdorient;
    return tf_write( NULL, &rec, sizeof( TF00434 ), 434 );
}

//---------------------------------------------------------------------
//                      Cyl head record kl dl
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0435( DEVBLK* dev )
{
    TF00435 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.dl     = dev->ckdcurdl;
    rec.cyl    = dev->ckdcurcyl;
    rec.head   = dev->ckdcurhead;
    rec.record = dev->ckdcurrec;
    rec.kl     = dev->ckdcurkl;
    rec.offset = dev->ckdtrkof;
    return tf_write( NULL, &rec, sizeof( TF00435 ), 435 );
}

//---------------------------------------------------------------------
//                      Read key
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0436( DEVBLK* dev )
{
    TF00436 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.kl = dev->ckdcurkl;
    return tf_write( NULL, &rec, sizeof( TF00436 ), 436 );
}

//---------------------------------------------------------------------
//                      Read data
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0437( DEVBLK* dev )
{
    TF00437 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.dl = dev->ckdcurdl;
    return tf_write( NULL, &rec, sizeof( TF00437 ), 437 );
}

//---------------------------------------------------------------------
//                 Write cyl head record kl dl
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0438( DEVBLK* dev, BYTE recnum, BYTE keylen, U16 datalen )
{
    TF00438 rec;
    rec.rhdr.devnum  = dev->devnum;
    rec.rhdr.lcss    = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.datalen = datalen;
    rec.recnum  = recnum;
    rec.keylen  = keylen;
    rec.cyl     = dev->ckdcurcyl;
    rec.head    = dev->ckdcurhead;
    return tf_write( NULL, &rec, sizeof( TF00438 ), 438 );
}

//---------------------------------------------------------------------
//                   Set track overflow flag
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0439( DEVBLK* dev, BYTE recnum )
{
    TF00439 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.recnum = recnum;
    rec.cyl    = dev->ckdcurcyl;
    rec.head   = dev->ckdcurhead;
    return tf_write( NULL, &rec, sizeof( TF00439 ), 439 );
}

//---------------------------------------------------------------------
//                 Update cyl head record kl dl
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0440( DEVBLK* dev )
{
    TF00440 rec;
    rec.rhdr.devnum  = dev->devnum;
    rec.rhdr.lcss    = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.cyl     = dev->ckdcurcyl;
    rec.head    = dev->ckdcurhead;
    rec.recnum  = dev->ckdcurrec;
    rec.keylen  = dev->ckdcurkl;
    rec.datalen = dev->ckdcurdl;

    return tf_write( NULL, &rec, sizeof( TF00440 ), 440 );
}

//---------------------------------------------------------------------
//                   Update cyl head record dl
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0441( DEVBLK* dev )
{
    TF00441 rec;
    rec.rhdr.devnum  = dev->devnum;
    rec.rhdr.lcss    = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.cyl     = dev->ckdcurcyl;
    rec.head    = dev->ckdcurhead;
    rec.recnum  = dev->ckdcurrec;
    rec.datalen = dev->ckdcurdl;
    return tf_write( NULL, &rec, sizeof( TF00441 ), 441 );
}

//---------------------------------------------------------------------
//                      Set file mask
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0442( DEVBLK* dev )
{
    TF00442 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.mask = dev->ckdfmask;
    return tf_write( NULL, &rec, sizeof( TF00442 ), 442 );
}

//---------------------------------------------------------------------
//                      Cache hit
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0516( DEVBLK* dev, S32 blkgrp, S32 i )
{
    TF00516 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.blkgrp = blkgrp;
    rec.idx    = i;
    return tf_write( NULL, &rec, sizeof( TF00516 ), 516 );
}

//---------------------------------------------------------------------
//                    Unavailable cache
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0517( DEVBLK* dev, S32 blkgrp )
{
    TF00517 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.blkgrp = blkgrp;
    return tf_write( NULL, &rec, sizeof( TF00517 ), 517 );
}

//---------------------------------------------------------------------
//                    Cache miss
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0518( DEVBLK* dev, S32 blkgrp, S32 o )
{
    TF00518 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.blkgrp = blkgrp;
    rec.idx    = o;
    return tf_write( NULL, &rec, sizeof( TF00518 ), 518 );
}

//---------------------------------------------------------------------
//                    Offset len
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0519( DEVBLK* dev, S32 blkgrp, U64 offset, S32 len )
{
    TF00519 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.blkgrp = blkgrp;
    rec.offset = offset;
    rec.len    = len;
    return tf_write( NULL, &rec, sizeof( TF00519 ), 519 );
}

//---------------------------------------------------------------------
//                    Positioning
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0520( DEVBLK* dev )
{
    TF00520 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    STRLCPY( rec.filename, TRIMLOC( dev->filename ));
    rec.rba    = dev->fbarba;
    return tf_write( NULL, &rec, sizeof( TF00520 ), 520 );
}

//---------------------------------------------------------------------
//                    Wait State PSW
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0800( REGS* regs )
{
    TF00800 rec;
    memcpy( &rec.psw, &regs->psw, sizeof( rec.psw ));
    rec.psw.ilc = REAL_ILC( regs );
    rec.amode64 = rec.psw.amode64;
    rec.amode   = rec.psw.amode;
    rec.zeroilc = rec.psw.zeroilc;
    return tf_write( regs, &rec, sizeof( TF00800 ), 800 );
}

//---------------------------------------------------------------------
//                  Program Interrupt
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0801( REGS* regs, U16 pcode, BYTE ilc )
{
    TF00801 rec;
    rec.sie =
#if defined( _FEATURE_SIE )
        SIE_MODE( regs );
#else
        false;
#endif
    rec.ilc = ilc;
    rec.pcode = pcode;
    rec.why = (pcode & PGM_TXF_EVENT) ? regs->txf_why : 0;
    rec.dxc = ((pcode & 0xFF) == PGM_DATA_EXCEPTION) ? regs->dxc : 0;
    return tf_write( regs, &rec, sizeof( TF00801 ), 801 );
}

//---------------------------------------------------------------------
//                       PER event
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0802( REGS* regs, U64 ia, U32 pcode, U16 perc )
{
    TF00802 rec;
    rec.ia    = ia;
    rec.pcode = pcode;
    rec.perc  = perc;
    return tf_write( regs, &rec, sizeof( TF00802 ), 802 );
}

//---------------------------------------------------------------------
//                  Program interrupt loop
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0803( REGS* regs, const char* str )
{
    TF00803 rec;
    STRLCPY( rec.str, str );
    return tf_write( regs, &rec, sizeof( TF00803 ), 803 );
}

//---------------------------------------------------------------------
//                  I/O interrupt (S/370)
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0804( REGS* regs, BYTE* csw, U16 ioid, BYTE lcss )
{
    TF00804 rec;
    rec.ioid = ioid;
    rec.rhdr.lcss = lcss;
    memcpy( rec.csw, csw, sizeof( rec.csw ));
    return tf_write( regs, &rec, sizeof( TF00804 ), 804 );
}

//---------------------------------------------------------------------
//                    I/O Interrupt
//            (handles both HHC00805 and HHC00806)
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0806( REGS* regs, U32 ioid, U32 ioparm, U32 iointid )
{
    TF00806 rec;
    rec.ioid    = ioid;
    rec.ioparm  = ioparm;
    rec.iointid = iointid;
    return tf_write( regs, &rec, sizeof( TF00806 ), 806 );
}

//---------------------------------------------------------------------
//                 Machine Check Interrupt
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0807( REGS* regs, U64 mcic, U64 fsta, U32 xdmg )
{
    TF00807 rec;
    rec.mcic = mcic;
    rec.xdmg = xdmg;
    rec.fsta = fsta;
    return tf_write( regs, &rec, sizeof( TF00807 ), 807 );
}

//---------------------------------------------------------------------
//                       Store Status
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0808( REGS* regs )
{
    TF00808 rec;
    return tf_write( regs, &rec, sizeof( TF00808 ), 808 );
}

//---------------------------------------------------------------------
//                    Disabled Wait State
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0809( REGS* regs, const char* str)
{
    TF00809 rec;
    STRLCPY( rec.str, str );
    return tf_write( regs, &rec, sizeof( TF00809 ), 809 );
}

//---------------------------------------------------------------------
//                      Architecture mode
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0811( REGS* regs, const char* archname )
{
    TF00811 rec;
    STRLCPY( rec.archname, archname );
    return tf_write( regs, &rec, sizeof( TF00811 ), 811 );
}

//---------------------------------------------------------------------
//               Vector facility online (370/390)
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0812( REGS* regs )
{
    TF00812 rec;
    return tf_write( regs, &rec, sizeof( TF00812 ), 812 );
}

//---------------------------------------------------------------------
//                    Signal Processor
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0814( REGS* regs, BYTE order, BYTE cc, U16 cpad, U32 status, U64 parm, BYTE got_status )
{
    TF00814 rec;
    rec.order      = order;
    rec.cc         = cc;
    rec.cpad       = cpad;
    rec.status     = status;
    rec.parm       = parm;
    rec.got_status = got_status;
    return tf_write( regs, &rec, sizeof( TF00814 ), 814 );
}

//---------------------------------------------------------------------
//                  External Interrupt
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0840( REGS* regs, U16 icode )
{
    TF00840 rec;
    rec.icode = icode;
    rec.cpu_timer = (EXT_CPU_TIMER_INTERRUPT == rec.icode)
                  ? sysblk.gct( regs ) : 0;
    return tf_write( regs, &rec, sizeof( TF00840 ), 840 );
}

//---------------------------------------------------------------------
//                  Block I/O Interrupt
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0844( REGS* regs )
{
    TF00844 rec;
    rec.rhdr.devnum   = sysblk.biodev->devnum;
    rec.rhdr.lcss     = SSID_TO_LCSS( sysblk.biodev->ssid );
    rec.servcode = sysblk.servcode;
    rec.bioparm  = sysblk.bioparm;
    rec.biostat  = sysblk.biostat;
    rec.biosubcd = sysblk.biosubcd;
    return tf_write( regs, &rec, sizeof( TF00844 ), 844 );
}

//---------------------------------------------------------------------
//               Block I/O External interrupt
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0845( REGS* regs )
{
    TF00845 rec;
    rec.bioparm  = sysblk.bioparm;
    rec.biosubcd = sysblk.biosubcd;
    return tf_write( regs, &rec, sizeof( TF00845 ), 845 );
}

//---------------------------------------------------------------------
//               Service Signal External Interrupt
//---------------------------------------------------------------------
DLL_EXPORT bool tf_0846( REGS* regs )
{
    TF00846 rec;
    rec.servparm = sysblk.servparm;
    return tf_write( regs, &rec, sizeof( TF00846 ), 846 );
}

//---------------------------------------------------------------------
//                      Halt subchannel
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1300( DEVBLK* dev, BYTE cc )
{
    TF01300 rec;
    rec.cc     = cc;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01300 ), 1300 );
}

//---------------------------------------------------------------------
//                      IDAW/MIDAW
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1301( const DEVBLK* dev, const U64 addr, const U16 count,
              BYTE* data, BYTE amt, const BYTE flag, const BYTE type )
{
    TF01301 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    rec.amt   = amt;
    rec.addr  = addr;
    rec.count = count;
    rec.type  = type;
    rec.mflag = flag;
    rec.code  = dev->code;
    memcpy( rec.data, data, amt );
    return tf_write( NULL, &rec, sizeof( TF01301 ), 1301 );
}

//---------------------------------------------------------------------
//                  Attention signaled
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1304( DEVBLK* dev )
{
    TF01304 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01304 ), 1304 );
}

//---------------------------------------------------------------------
//                      Attention
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1305( DEVBLK* dev )
{
    TF01305 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01305 ), 1305 );
}

//---------------------------------------------------------------------
//                  Initial status interrupt
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1306( DEVBLK* dev )
{
    TF01306 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01306 ), 1306 );
}

//---------------------------------------------------------------------
//                  Attention completed
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1307( DEVBLK* dev )
{
    TF01307 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01307 ), 1307 );
}

//---------------------------------------------------------------------
//                  Clear completed
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1308( DEVBLK* dev )
{
    TF01308 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01308 ), 1308 );
}

//---------------------------------------------------------------------
//                      Halt completed
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1309( DEVBLK* dev )
{
    TF01309 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01309 ), 1309 );
}

//---------------------------------------------------------------------
//                      Suspended
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1310( DEVBLK* dev )
{
    TF01310 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01310 ), 1310 );
}

//---------------------------------------------------------------------
//                        Resumed
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1311( DEVBLK* dev )
{
    TF01311 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01311 ), 1311 );
}

//---------------------------------------------------------------------
//                       I/O stat
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1312( DEVBLK* dev, BYTE us, BYTE cs, BYTE amt, U32 resid, BYTE* data )
{
    TF01312 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    rec.unitstat = us;
    rec.chanstat = cs;
    rec.amt      = amt;
    rec.residual = resid;
    memcpy( rec.data, data, amt );
    return tf_write( NULL, &rec, sizeof( TF01312 ), 1312 );
}

//---------------------------------------------------------------------
//                          Sense
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1313( DEVBLK* dev )
{
    TF01313 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    memcpy( rec.sense, dev->sense, sizeof( rec.sense ));
    if (dev->sns)
        dev->sns( dev, rec.sns, sizeof( rec.sns ));
    else
        rec.sns[0] = 0;
    return tf_write( NULL, &rec, sizeof( TF01313 ), 1313 );
}

//---------------------------------------------------------------------
//                          CCW
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1315( const DEVBLK* dev, const BYTE* ccw,
              const U32 addr, const U16 count, BYTE* data, BYTE amt )
{
    TF01315 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    rec.amt   = amt;
    rec.addr  = addr;
    rec.count = count;
    memcpy( rec.ccw,  ccw,  sizeof( rec.ccw ));
    memcpy( rec.data, data, amt );
    return tf_write( NULL, &rec, sizeof( TF01315 ), 1315 );
}

//---------------------------------------------------------------------
//                      CSW (370)
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1316( const DEVBLK* dev, const BYTE csw[] )
{
    TF01316 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    memcpy( rec.csw, csw, sizeof( rec.csw ));
    return tf_write( NULL, &rec, sizeof( TF01316 ), 1316 );
}

//---------------------------------------------------------------------
//                         SCSW
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1317( const DEVBLK* dev, const SCSW scsw )
{
    TF01317 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    memcpy( &rec.scsw, &scsw, sizeof( rec.scsw ));
    return tf_write( NULL, &rec, sizeof( TF01317 ), 1317 );
}

//---------------------------------------------------------------------
//                        TEST I/O
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1318( DEVBLK* dev, BYTE cc )
{
    TF01318 rec;
    rec.cc     = cc;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01318 ), 1318 );
}

//---------------------------------------------------------------------
//               S/370 start I/O conversion started
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1320( DEVBLK* dev )
{
    TF01320 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01320 ), 1320 );
}

//---------------------------------------------------------------------
//               S/370 start I/O conversion success
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1321( DEVBLK* dev )
{
    TF01321 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01321 ), 1321 );
}

//---------------------------------------------------------------------
//                      Halt I/O
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1329( DEVBLK* dev )
{
    TF01329 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01329 ), 1329 );
}

//---------------------------------------------------------------------
//                      HIO modification
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1330( DEVBLK* dev )
{
    TF01330 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01330 ), 1330 );
}

//---------------------------------------------------------------------
//                      Clear subchannel
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1331( DEVBLK* dev )
{
    TF01331 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01331 ), 1331 );
}

//---------------------------------------------------------------------
//                      Halt subchannel
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1332( DEVBLK* dev )
{
    TF01332 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01332 ), 1332 );
}

//---------------------------------------------------------------------
//                    Resume subchannel
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1333( DEVBLK* dev, BYTE cc )
{
    TF01333 rec;
    rec.cc     = cc;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    return tf_write( NULL, &rec, sizeof( TF01333 ), 1333 );
}

//---------------------------------------------------------------------
//                          ORB
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1334( DEVBLK* dev, ORB* orb )
{
    TF01334 rec;
    rec.rhdr.devnum = dev->devnum;
    rec.rhdr.lcss   = SSID_TO_LCSS( dev->ssid );
    memcpy( &rec.orb, orb, sizeof( rec.orb ));
    return tf_write( NULL, &rec, sizeof( TF01334 ), 1334 );
}

//---------------------------------------------------------------------
//                      Startio cc=2
//---------------------------------------------------------------------
DLL_EXPORT bool tf_1336( DEVBLK* dev )
{
    TF01336 rec;
    rec.rhdr.devnum  = dev->devnum;
    rec.rhdr.lcss    = SSID_TO_LCSS( dev->ssid );
    rec.busy         = dev->busy;
    rec.startpending = dev->startpending;
    return tf_write( NULL, &rec, sizeof( TF01336 ), 1336 );
}

//---------------------------------------------------------------------
//                  General Registers
//---------------------------------------------------------------------
DLL_EXPORT bool tf_2269( REGS* regs, BYTE* inst )
{
    TF02269 rec;
    memcpy( rec.gr, regs->gr, sizeof( rec.gr ));
    rec.ifetch_error = inst ? false : true;
    rec.sie =
#if defined( _FEATURE_SIE )
        SIE_MODE( regs );
#else
        false;
#endif
    return tf_write( regs, &rec, sizeof( TF02269 ), 2269 );
}

//---------------------------------------------------------------------
//               Floating Point Registers
//---------------------------------------------------------------------
DLL_EXPORT bool tf_2270( REGS* regs )
{
    TF02270 rec;
    memcpy( rec.fpr, regs->fpr, sizeof( rec.fpr ));
    rec.afp = (regs->CR(0) & CR0_AFP) ? true : false;
    return tf_write( regs, &rec, sizeof( TF02270 ), 2270 );
}

//---------------------------------------------------------------------
//                  Control Registers
//---------------------------------------------------------------------
DLL_EXPORT bool tf_2271( REGS* regs )
{
    TF02271 rec;
    memcpy( rec.cr, &regs->cr_struct[1], sizeof( rec.cr ));
    return tf_write( regs, &rec, sizeof( TF02271 ), 2271 );
}

//---------------------------------------------------------------------
//                  Access Registers
//---------------------------------------------------------------------
DLL_EXPORT bool tf_2272( REGS* regs )
{
    TF02272 rec;
    memcpy( rec.ar, regs->ar, sizeof( rec.ar ));
    return tf_write( regs, &rec, sizeof( TF02272 ), 2272 );
}

//---------------------------------------------------------------------
//           Floating Point Control Register
//---------------------------------------------------------------------
DLL_EXPORT bool tf_2276( REGS* regs )
{
    TF02276 rec;
    rec.fpc = regs->fpc;
    rec.afp = (regs->CR(0) & CR0_AFP) ? true : false;
    return tf_write( regs, &rec, sizeof( TF02276 ), 2276 );
}

//---------------------------------------------------------------------
//              Primary Instruction Trace
//---------------------------------------------------------------------
DLL_EXPORT bool tf_2324( REGS* regs, BYTE* inst )
{
    TF02324 rec;
    rec.sie =
#if defined( _FEATURE_SIE )
        SIE_MODE( regs );
#else
        false;
#endif
    rec.ilc = ILC( inst[0] ); // (value in regs not always right)
    memcpy( &rec.psw,  &regs->psw, sizeof( rec.psw ));
    memcpy( rec.inst, inst, rec.ilc );
    rec.amode64 = rec.psw.amode64;
    rec.amode   = rec.psw.amode;
    rec.zeroilc = rec.psw.zeroilc;
    return tf_write( regs, &rec, sizeof( TF02324 ), 2324 );
}

//---------------------------------------------------------------------
//           Instruction Storage Operand helper
//---------------------------------------------------------------------

PUSH_GCC_WARNINGS()
DISABLE_GCC_WARNING( "-Waddress-of-packed-member" )

static void tf_2326_op( REGS* regs, TFOP* op, BYTE opcode1, BYTE opcode2, int b, bool isop2 )
{
    if (b >= 0)
    {
        int ar, stid;

        /* Determine if operand should be treated as a real address */
        if (0
            || REAL_MODE( &regs->psw )
            || (isop2 && IS_REAL_ADDR_OP( opcode1, opcode2 ))
        )
            ar = USE_REAL_ADDR;
        else
            ar = b;

        /* Get the real address for this virtual address */
        if ((op->xcode = tf_virt_to_real( &op->raddr, &stid,
             op->vaddr, ar, regs, ACCTYPE_HW )) == 0)
        {
            U64 pagesize, bytemask, abs;
            size_t amt = sizeof( op->stor );

            /* Convert real address to absolute address */
            abs = tf_apply_prefixing( op->raddr, regs );

            /* Determine page size and byte mask */
            if (regs->arch_mode == ARCH_370_IDX)
            {
                pagesize = STORAGE_KEY_2K_PAGESIZE;
                bytemask = STORAGE_KEY_2K_BYTEMASK;
            }
            else
            {
                pagesize = STORAGE_KEY_4K_PAGESIZE;
                bytemask = STORAGE_KEY_4K_BYTEMASK;
            }

            /* If virtual storage, only save the data that's
               on this one page (i.e. to the end of the page) */
            if (ar != USE_REAL_ADDR)
            {
                amt = pagesize - (abs & bytemask);

                if (amt > sizeof( op->stor ))
                    amt = sizeof( op->stor );
            }

            /* Update Interval Timer beforehand, if needed */
#if defined( _FEATURE_INTERVAL_TIMER )
            tf_store_int_timer( regs, op->raddr );
#endif
            /* Save as much operand data as possible */
            memcpy( op->stor, regs->mainstor + abs, amt );

            /* Save the amount we saved and its storage key */
            op->amt = (BYTE) amt; // (how much we saved)
            op->skey = tf_get_storage_key( regs, abs );
        }
    }
}

POP_GCC_WARNINGS()

//---------------------------------------------------------------------
//                 Instruction Storage
//---------------------------------------------------------------------
DLL_EXPORT bool tf_2326( REGS* regs, TF02326* tf2326, BYTE opcode1, BYTE opcode2,
                                           int    b1,    int    b2 )
{
    if (tf2326->valid)
    {
        tf2326->real_mode = REAL_MODE( &regs->psw );
        tf2326->b1 = b1;
        tf2326->b2 = b2;
        tf_2326_op( regs, &tf2326->op1, opcode1, opcode2, b1, false );
        tf_2326_op( regs, &tf2326->op2, opcode1, opcode2, b2, true  );
    }
    return tf_write( regs, tf2326, sizeof( TF02326 ), 2326 );
}

/*-------------------------------------------------------------------*/
/*            Return maximum TraceFile record size                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT size_t  tf_MAX_RECSIZE()
{
    size_t
        max_recsize = sizeof( TFSYS );

    // Device tracing

    if (max_recsize < sizeof( TF00423 ))
        max_recsize = sizeof( TF00423 );

    if (max_recsize < sizeof( TF00424 ))
        max_recsize = sizeof( TF00424 );

    if (max_recsize < sizeof( TF00425 ))
        max_recsize = sizeof( TF00425 );

    if (max_recsize < sizeof( TF00426 ))
        max_recsize = sizeof( TF00426 );

    if (max_recsize < sizeof( TF00427 ))
        max_recsize = sizeof( TF00427 );

    if (max_recsize < sizeof( TF00428 ))
        max_recsize = sizeof( TF00428 );

    if (max_recsize < sizeof( TF00429 ))
        max_recsize = sizeof( TF00429 );

    if (max_recsize < sizeof( TF00430 ))
        max_recsize = sizeof( TF00430 );

    if (max_recsize < sizeof( TF00431 ))
        max_recsize = sizeof( TF00431 );

    if (max_recsize < sizeof( TF00432 ))
        max_recsize = sizeof( TF00432 );

    if (max_recsize < sizeof( TF00433 ))
        max_recsize = sizeof( TF00433 );

    if (max_recsize < sizeof( TF00434 ))
        max_recsize = sizeof( TF00434 );

    if (max_recsize < sizeof( TF00435 ))
        max_recsize = sizeof( TF00435 );

    if (max_recsize < sizeof( TF00436 ))
        max_recsize = sizeof( TF00436 );

    if (max_recsize < sizeof( TF00437 ))
        max_recsize = sizeof( TF00437 );

    if (max_recsize < sizeof( TF00438 ))
        max_recsize = sizeof( TF00438 );

    if (max_recsize < sizeof( TF00439 ))
        max_recsize = sizeof( TF00439 );

    if (max_recsize < sizeof( TF00440 ))
        max_recsize = sizeof( TF00440 );

    if (max_recsize < sizeof( TF00441 ))
        max_recsize = sizeof( TF00441 );

    if (max_recsize < sizeof( TF00442 ))
        max_recsize = sizeof( TF00442 );

    if (max_recsize < sizeof( TF00516 ))
        max_recsize = sizeof( TF00516 );

    if (max_recsize < sizeof( TF00517 ))
        max_recsize = sizeof( TF00517 );

    if (max_recsize < sizeof( TF00518 ))
        max_recsize = sizeof( TF00518 );

    if (max_recsize < sizeof( TF00519 ))
        max_recsize = sizeof( TF00519 );

    if (max_recsize < sizeof( TF00520 ))
        max_recsize = sizeof( TF00520 );

    // Instruction tracing

    if (max_recsize < sizeof( TF00800 ))
        max_recsize = sizeof( TF00800 );

    if (max_recsize < sizeof( TF00801 ))
        max_recsize = sizeof( TF00801 );

    if (max_recsize < sizeof( TF00802 ))
        max_recsize = sizeof( TF00802 );

    if (max_recsize < sizeof( TF00803 ))
        max_recsize = sizeof( TF00803 );

    if (max_recsize < sizeof( TF00804 ))
        max_recsize = sizeof( TF00804 );

    if (max_recsize < sizeof( TF00806 )) // (handles both HHC00805 and HHC00806)
        max_recsize = sizeof( TF00806 );

    if (max_recsize < sizeof( TF00807 ))
        max_recsize = sizeof( TF00807 );

    if (max_recsize < sizeof( TF00808 ))
        max_recsize = sizeof( TF00808 );

    if (max_recsize < sizeof( TF00809 ))
        max_recsize = sizeof( TF00809 );

    if (max_recsize < sizeof( TF00811 ))
        max_recsize = sizeof( TF00811 );

    if (max_recsize < sizeof( TF00812 ))
        max_recsize = sizeof( TF00812 );

    if (max_recsize < sizeof( TF00814 ))
        max_recsize = sizeof( TF00814 );

    if (max_recsize < sizeof( TF00840 ))
        max_recsize = sizeof( TF00840 );

    if (max_recsize < sizeof( TF00844 ))
        max_recsize = sizeof( TF00844 );

    if (max_recsize < sizeof( TF00845 ))
        max_recsize = sizeof( TF00845 );

    if (max_recsize < sizeof( TF00846 ))
        max_recsize = sizeof( TF00846 );

    // Device tracing

    if (max_recsize < sizeof( TF01300 ))
        max_recsize = sizeof( TF01300 );

    if (max_recsize < sizeof( TF01301 ))
        max_recsize = sizeof( TF01301 );

    if (max_recsize < sizeof( TF01304 ))
        max_recsize = sizeof( TF01304 );

    if (max_recsize < sizeof( TF01305 ))
        max_recsize = sizeof( TF01305 );

    if (max_recsize < sizeof( TF01306 ))
        max_recsize = sizeof( TF01306 );

    if (max_recsize < sizeof( TF01307 ))
        max_recsize = sizeof( TF01307 );

    if (max_recsize < sizeof( TF01308 ))
        max_recsize = sizeof( TF01308 );

    if (max_recsize < sizeof( TF01309 ))
        max_recsize = sizeof( TF01309 );

    if (max_recsize < sizeof( TF01310 ))
        max_recsize = sizeof( TF01310 );

    if (max_recsize < sizeof( TF01311 ))
        max_recsize = sizeof( TF01311 );

    if (max_recsize < sizeof( TF01312 ))
        max_recsize = sizeof( TF01312 );

    if (max_recsize < sizeof( TF01313 ))
        max_recsize = sizeof( TF01313 );

    if (max_recsize < sizeof( TF01315 ))
        max_recsize = sizeof( TF01315 );

    if (max_recsize < sizeof( TF01316 ))
        max_recsize = sizeof( TF01316 );

    if (max_recsize < sizeof( TF01317 ))
        max_recsize = sizeof( TF01317 );

    if (max_recsize < sizeof( TF01318 ))
        max_recsize = sizeof( TF01318 );

    if (max_recsize < sizeof( TF01320 ))
        max_recsize = sizeof( TF01320 );

    if (max_recsize < sizeof( TF01321 ))
        max_recsize = sizeof( TF01321 );

    if (max_recsize < sizeof( TF01329 ))
        max_recsize = sizeof( TF01329 );

    if (max_recsize < sizeof( TF01330 ))
        max_recsize = sizeof( TF01330 );

    if (max_recsize < sizeof( TF01331 ))
        max_recsize = sizeof( TF01331 );

    if (max_recsize < sizeof( TF01332 ))
        max_recsize = sizeof( TF01332 );

    if (max_recsize < sizeof( TF01333 ))
        max_recsize = sizeof( TF01333 );

    if (max_recsize < sizeof( TF01334 ))
        max_recsize = sizeof( TF01334 );

    if (max_recsize < sizeof( TF01336 ))
        max_recsize = sizeof( TF01336 );

    // Instruction tracing

    if (max_recsize < sizeof( TF02269 ))
        max_recsize = sizeof( TF02269 );

    if (max_recsize < sizeof( TF02270 ))
        max_recsize = sizeof( TF02270 );

    if (max_recsize < sizeof( TF02271 ))
        max_recsize = sizeof( TF02271 );

    if (max_recsize < sizeof( TF02272 ))
        max_recsize = sizeof( TF02272 );

    if (max_recsize < sizeof( TF02276 ))
        max_recsize = sizeof( TF02276 );

    if (max_recsize < sizeof( TF02324))
        max_recsize = sizeof( TF02324 );

    if (max_recsize < sizeof( TF02326))
        max_recsize = sizeof( TF02326 );

    return
        max_recsize;
}

/*-------------------------------------------------------------------*/
/*          Do unconditional Endian swap of TFSYS record             */
/*-------------------------------------------------------------------*/
DLL_EXPORT void tf_swap_sys( TFSYS* sys )
{
    sys->bigend          = !sys->bigend;
    sys->beg_tod.tv_sec  = SWAP32( sys->beg_tod.tv_sec  );
    sys->beg_tod.tv_usec = SWAP32( sys->beg_tod.tv_usec );
    sys->end_tod.tv_sec  = SWAP32( sys->end_tod.tv_sec  );
    sys->end_tod.tv_usec = SWAP32( sys->end_tod.tv_usec );
    sys->tot_ins         = SWAP64( sys->tot_ins         );
    sys->tot_dev         = SWAP64( sys->tot_dev         );
}

/*-------------------------------------------------------------------*/
/*        Do unconditional Endian swap of TFHDR record header        */
/*-------------------------------------------------------------------*/
DLL_EXPORT void tf_swap_hdr( BYTE sys_ffmt, TFHDR* hdr )
{
    hdr->prev        = SWAP16( hdr->prev        );
    hdr->curr        = SWAP16( hdr->curr        );
    hdr->cpuad       = SWAP16( hdr->cpuad       );
    hdr->msgnum      = SWAP16( hdr->msgnum      );
    hdr->devnum      = SWAP16( hdr->devnum      );
    hdr->tod.tv_sec  = SWAP32( hdr->tod.tv_sec  );
    hdr->tod.tv_usec = SWAP32( hdr->tod.tv_usec );

    // Format-2...

    if (sys_ffmt >= TF_FMT2)
        hdr->tidnum  = SWAP64( hdr->tidnum      );
}

/*-------------------------------------------------------------------*/
/*     Do unconditional Endian swap of Trace File record fields      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* PROGRAMMING NOTE: the TFHDR* passed to us by TFPRINT, from our    */
/* point of view, appears to point to a "new" (current) format trace */
/* record with both the header fields as well as the rec fields all  */
/* properly aligned.                                                 */
/*                                                                   */
/* When TFSWAP calls us however, it passes an adjusted TFHDR pointer */
/* wherein only the actual rec fields are aligned properly. This is  */
/* safe to do since we don't ever access any of the header fields    */
/* anyway. We only mess with the actual rec fields.                  */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT void tf_swap_rec( TFHDR* hdr, U16 msgnum )
{
    switch (msgnum)
    {
        case  423:
        {
            // (nothing to swap!)
        }
        break;

        case  424:
        {
            TF00424* rec = (TF00424*) hdr;
            rec->trk     = SWAP32( rec->trk    );
            rec->bufcur  = SWAP32( rec->bufcur );
        }
        break;

        case  425:
        {
            TF00425* rec = (TF00425*) hdr;
            rec->bufcur  = SWAP32( rec->bufcur );
        }
        break;

        case  426:
        {
            TF00426* rec = (TF00426*) hdr;
            rec->trk     = SWAP32( rec->trk );
            rec->idx     = SWAP32( rec->idx );
        }
        break;

        case  427:
        {
            TF00427* rec = (TF00427*) hdr;
            rec->trk     = SWAP32( rec->trk );
        }
        break;

        case  428:
        {
            TF00428* rec = (TF00428*) hdr;
            rec->trk     = SWAP32( rec->trk );
            rec->idx     = SWAP32( rec->idx );
        }
        break;

        case  429:
        {
            TF00429* rec = (TF00429*) hdr;
            rec->trk     = SWAP32( rec->trk    );
            rec->fnum    = SWAP32( rec->fnum   );
            rec->len     = SWAP32( rec->len    );
            rec->offset  = SWAP64( rec->offset );
        }
        break;

        case  430:
        {
            TF00430* rec = (TF00430*) hdr;
            rec->trk     = SWAP32( rec->trk );
        }
        break;

        case  431:
        {
            TF00431* rec = (TF00431*) hdr;
            rec->cyl     = SWAP32( rec->cyl  );
            rec->head    = SWAP32( rec->head );
        }
        break;

        case  432:
        {
            // (nothing to swap!)
        }
        break;

        case  433:
        {
            TF00433* rec = (TF00433*) hdr;
            rec->cyl     = SWAP32( rec->cyl  );
            rec->head    = SWAP32( rec->head );
        }
        break;

        case  434:
        {
            // (nothing to swap!)
        }
        break;

        case  435:
        {
            TF00435* rec = (TF00435*) hdr;
            rec->dl      = SWAP16( rec->dl     );
            rec->cyl     = SWAP32( rec->cyl    );
            rec->head    = SWAP32( rec->head   );
            rec->record  = SWAP32( rec->record );
            rec->kl      = SWAP32( rec->kl     );
            rec->offset  = SWAP32( rec->offset );
        }
        break;

        case  436:
        {
            TF00436* rec = (TF00436*) hdr;
            rec->kl      = SWAP32( rec->kl );
        }
        break;

        case  437:
        {
            TF00437* rec = (TF00437*) hdr;
            rec->dl      = SWAP16( rec->dl );
        }
        break;

        case  438:
        {
            TF00438* rec = (TF00438*) hdr;
            rec->datalen = SWAP16( rec->datalen );
            rec->cyl     = SWAP32( rec->cyl     );
            rec->head    = SWAP32( rec->head    );
        }
        break;

        case  439:
        {
            TF00439* rec = (TF00439*) hdr;
            rec->cyl     = SWAP32( rec->cyl  );
            rec->head    = SWAP32( rec->head );
        }
        break;

        case  440:
        {
            TF00440* rec = (TF00440*) hdr;
            rec->datalen = SWAP16( rec->datalen );
            rec->cyl     = SWAP32( rec->cyl     );
            rec->head    = SWAP32( rec->head    );
            rec->recnum  = SWAP32( rec->recnum  );
            rec->keylen  = SWAP32( rec->keylen  );
        }
        break;

        case  441:
        {
            TF00441* rec = (TF00441*) hdr;
            rec->datalen = SWAP16( rec->datalen );
            rec->cyl     = SWAP32( rec->cyl     );
            rec->head    = SWAP32( rec->head    );
            rec->recnum  = SWAP32( rec->recnum  );
        }
        break;

        case  442:
        {
            // (nothing to swap!)
        }
        break;

        case  516:
        {
            TF00516* rec = (TF00516*) hdr;
            rec->blkgrp  = SWAP32( rec->blkgrp );
            rec->idx     = SWAP32( rec->idx    );
        }
        break;

        case  517:
        {
            TF00517* rec = (TF00517*) hdr;
            rec->blkgrp  = SWAP32( rec->blkgrp );
        }
        break;

        case  518:
        {
            TF00518* rec = (TF00518*) hdr;
            rec->blkgrp  = SWAP32( rec->blkgrp );
            rec->idx     = SWAP32( rec->idx    );
        }
        break;

        case  519:
        {
            TF00519* rec = (TF00519*) hdr;
            rec->blkgrp  = SWAP32( rec->blkgrp );
            rec->len     = SWAP32( rec->len    );
            rec->offset  = SWAP64( rec->offset );
        }
        break;

        case  520:
        {
            TF00520* rec = (TF00520*) hdr;
            rec->rba     = SWAP64( rec->rba );
        }
        break;

        case  800:
        {
            TF00800* rec = (TF00800*) hdr;

            rec->psw.intcode  = SWAP16( rec->psw.intcode  );
            rec->psw.zeroword = SWAP32( rec->psw.zeroword );
            rec->psw.ia.D     = SWAP64( rec->psw.ia.D     );
            rec->psw.amask.D  = SWAP64( rec->psw.amask.D  );

            rec->psw.amode64  = rec->amode64 ? 1 : 0;
            rec->psw.amode    = rec->amode   ? 1 : 0;
            rec->psw.zeroilc  = rec->zeroilc ? 1 : 0;
        }
        break;

        case  801:
        {
            TF00801* rec = (TF00801*) hdr;
            rec->pcode  = SWAP16( rec->pcode );
            rec->dxc    = SWAP32( rec->dxc   );
            rec->why    = SWAP32( rec->why   );
        }
        break;

        case  802:
        {
            TF00802* rec = (TF00802*) hdr;
            rec->perc   = SWAP16( rec->perc  );
            rec->pcode  = SWAP32( rec->pcode );
            rec->ia     = SWAP64( rec->ia    );
        }
        break;

        case  803:
        {
            // (nothing to swap!)
        }
        break;

        case  804:
        {
            TF00804* rec = (TF00804*) hdr;
            rec->ioid    = SWAP16( rec->ioid );
        }
        break;

        case  806: // (handles both HHC00805 and HHC00806)
        {
            TF00806* rec = (TF00806*) hdr;
            rec->ioid    = SWAP32( rec->ioid    );
            rec->ioparm  = SWAP32( rec->ioparm  );
            rec->iointid = SWAP32( rec->iointid );
        }
        break;

        case  807:
        {
            TF00807* rec = (TF00807*) hdr;
            rec->xdmg    = SWAP32( rec->xdmg );
            rec->mcic    = SWAP64( rec->mcic );
            rec->fsta    = SWAP64( rec->fsta );
        }
        break;

        case  808:
        {
            // (nothing to swap!)
        }
        break;

        case  809:
        {
            // (nothing to swap!)
        }
        break;

        case  811:
        {
            // (nothing to swap!)
        }
        break;

        case  812:
        {
            // (nothing to swap!)
        }
        break;

        case  814:
        {
            TF00814* rec = (TF00814*) hdr;
            rec->cpad    = SWAP16( rec->cpad   );
            rec->status  = SWAP32( rec->status );
            rec->parm    = SWAP64( rec->parm   );
        }
        break;

        case  840:
        {
            TF00840* rec   = (TF00840*) hdr;
            rec->icode     = SWAP16( rec->icode     );
            rec->cpu_timer = SWAP64( rec->cpu_timer );
        }
        break;

        case  844:
        {
            TF00844* rec  = (TF00844*) hdr;
            rec->servcode = SWAP16( rec->servcode );
            rec->bioparm  = SWAP64( rec->bioparm  );
        }
        break;

        case  845:
        {
            TF00845* rec = (TF00845*) hdr;
            rec->bioparm = SWAP64( rec->bioparm );
        }
        break;

        case  846:
        {
            TF00846* rec  = (TF00846*) hdr;
            rec->servparm = SWAP32( rec->servparm );
        }
        break;

        case 1300:
        {
            // (nothing to swap!)
        }
        break;

        case 1301:
        {
            TF01301* rec = (TF01301*) hdr;
            rec->count   = SWAP16( rec->count );
            rec->addr    = SWAP64( rec->addr  );
        }
        break;

        case 1304:
        {
            // (nothing to swap!)
        }
        break;

        case 1305:
        {
            // (nothing to swap!)
        }
        break;

        case 1306:
        {
            // (nothing to swap!)
        }
        break;

        case 1307:
        {
            // (nothing to swap!)
        }
        break;

        case 1308:
        {
            // (nothing to swap!)
        }
        break;

        case 1309:
        {
            // (nothing to swap!)
        }
        break;

        case 1310:
        {
            // (nothing to swap!)
        }
        break;

        case 1311:
        {
            // (nothing to swap!)
        }
        break;

        case 1312:
        {
            TF01312* rec  = (TF01312*) hdr;
            rec->residual = SWAP32( rec->residual );
        }
        break;

        case 1313:
        {
            // (nothing to swap!)
        }
        break;

        case 1315:
        {
            TF01315* rec = (TF01315*) hdr;
            rec->count   = SWAP16( rec->count );
            rec->addr    = SWAP32( rec->addr  );
        }
        break;

        case 1316:
        {
            // (nothing to swap!)
        }
        break;

        case 1317:
        {
            // (nothing to swap!)
        }
        break;

        case 1318:
        {
            // (nothing to swap!)
        }
        break;

        case 1320:
        {
            // (nothing to swap!)
        }
        break;

        case 1321:
        {
            // (nothing to swap!)
        }
        break;

        case 1329:
        {
            // (nothing to swap!)
        }
        break;

        case 1330:
        {
            // (nothing to swap!)
        }
        break;

        case 1331:
        {
            // (nothing to swap!)
        }
        break;

        case 1332:
        {
            // (nothing to swap!)
        }
        break;

        case 1333:
        {
            // (nothing to swap!)
        }
        break;

        case 1334:
        {
            // (nothing to swap!)
        }
        break;

        case 1336:
        {
            // (nothing to swap!)
        }
        break;

        case 2269:
        {
            TF02269* rec = (TF02269*) hdr;
            int  i;
            for (i=0; i < 16; ++i)
                rec->gr[i].D   = SWAP64( rec->gr[i].D );
        }
        break;

        case 2270:
        {
            TF02270* rec = (TF02270*) hdr;
            int  i;
            for (i=0; i < 32; ++i)
                rec->fpr[i]   = SWAP32( rec->fpr[i] );
        }
        break;

        case 2271:
        {
            TF02271* rec = (TF02271*) hdr;
            int  i;
            for (i=0; i < 16; ++i)
                rec->cr[i].D   = SWAP64( rec->cr[i].D );
        }
        break;

        case 2272:
        {
            TF02272* rec = (TF02272*) hdr;
            int  i;
            for (i=0; i < 16; ++i)
                rec->ar[i]   = SWAP32( rec->ar[i] );
        }
        break;

        case 2276:
        {
            TF02276* rec = (TF02276*) hdr;
            rec->fpc     = SWAP32( rec->fpc );
        }
        break;

        case 2324:
        {
            TF02324* rec = (TF02324*) hdr;

            rec->psw.intcode  = SWAP16( rec->psw.intcode  );
            rec->psw.zeroword = SWAP32( rec->psw.zeroword );
            rec->psw.ia.D     = SWAP64( rec->psw.ia.D     );
            rec->psw.amask.D  = SWAP64( rec->psw.amask.D  );

            rec->psw.amode64  = rec->amode64 ? 1 : 0;
            rec->psw.amode    = rec->amode   ? 1 : 0;
            rec->psw.zeroilc  = rec->zeroilc ? 1 : 0;
        }
        break;

        case 2326:
        {
            TF02326* rec = (TF02326*) hdr;
            TFOP* op;

            rec->b1   = SWAP16( rec->b1 );
            rec->b2   = SWAP16( rec->b2 );

            op = &rec->op1;

            op->xcode = SWAP16( op->xcode );
            op->vaddr = SWAP64( op->vaddr );
            op->raddr = SWAP64( op->raddr );

            op = &rec->op2;

            op->xcode = SWAP16( op->xcode );
            op->vaddr = SWAP64( op->vaddr );
            op->raddr = SWAP64( op->raddr );
        }
        break;

        default: CRASH(); UNREACHABLE_CODE( return );
    }
}

/*********************************************************************/
/*                   END OF TraceFile support                        */
/*********************************************************************/
