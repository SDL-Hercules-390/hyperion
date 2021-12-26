/* HSCUTL.H     (C) Copyright Roger Bowler, 1999-2012                */
/*              Host-specific functions for Hercules                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* HSCUTL.H   --   Implementation of functions used in hercules that */
/* may be missing on some platform ports, or other convenient mis-   */
/* laneous global utility functions.                                 */
/*-------------------------------------------------------------------*/


#ifndef __HSCUTL_H__
#define __HSCUTL_H__

#include "hercules.h"

/*********************************************************************
  The following couple of Hercules 'utility' functions may be defined
  elsewhere depending on which host platform we're being built for...
  For Windows builds (e.g. MingW32), the functionality for the below
  functions is defined in 'w32util.c'. For other host build platforms
  (e.g. Linux, Apple, etc), the functionality for the below functions
  is defined right here in 'hscutil.c'...
 *********************************************************************/

#if defined(_MSVC_)

  /* The w32util.c module will provide the below functionality... */

#else

  /* THIS module (hscutil.c) is to provide the below functionality.. */

  /*
    Returns outpath as a host filesystem compatible filename path.
    This is a Cygwin-to-MSVC transitional period helper function.
    On non-Windows platforms it simply copies inpath to outpath.
    On Windows it converts inpath of the form "/cygdrive/x/foo.bar"
    to outpath in the form "x:/foo.bar" for Windows compatibility.
  */
  char *hostpath( char *outpath, const char *inpath, size_t buffsize );

  /* Poor man's  "fcntl( fd, F_GETFL )"... */
  /* (only returns access-mode flags and not any others) */
  int get_file_accmode_flags( int fd );

  /* Initialize/Deinitialize sockets package... */
  int socket_init( void );
  int socket_deinit( void );

  /* Set socket to blocking or non-blocking mode... */
  int socket_set_blocking_mode( int sfd, int blocking_mode );

  /* Determine whether a file descriptor is a socket or not... */
  /* (returns 1==true if it's a socket, 0==false otherwise)    */
  int socket_is_socket( int sfd );

  /* Set the SO_KEEPALIVE option and timeout values for a
     socket connection to detect when client disconnects.
     Returns 0==success, +1==warning, -1==failure
     (*) Warning failure means function only partially
         succeeded (not all requested values were set)
  */
  int set_socket_keepalive( int sfd, int idle_time, int probe_interval,
                            int probe_count );

  /* Function to retrieve keepalive values. 0==success, -1=failure */
  int get_socket_keepalive( int sfd, int* idle_time, int* probe_interval,
                            int* probe_count );

  /* Determine whether process is running "elevated" or not */
  /* (returns 1==true running elevated, 0==false otherwise) */
  bool are_elevated();

#endif // !defined(_MSVC_)

/*-------------------------------------------------------------------*/

#if !defined(_MSVC_) && !defined(HAVE_STRERROR_R)
  HUT_DLL_IMPORT void strerror_r_init(void);
  HUT_DLL_IMPORT int  strerror_r(int, char *, size_t);
#endif

  HUT_DLL_IMPORT const char *get_symbol(const char *);
  HUT_DLL_IMPORT char *resolve_symbol_string(const char *);
  HUT_DLL_IMPORT void set_symbol(const char *,const char *);
  HUT_DLL_IMPORT void del_symbol(const char *);
  HUT_DLL_IMPORT void list_all_symbols();

#ifdef _MSVC_
  #ifndef HAVE_ID_T
  #define HAVE_ID_T
    typedef unsigned long id_t;
  #endif
  #ifndef HAVE_SYS_RESOURCE_H
    #define  PRIO_PROCESS  0
    #define  PRIO_PGRP     1
    #define  PRIO_USER     2
  #endif
#endif // _MSVC_

#if !defined(HAVE_STRLCPY)
/* $OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $ */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
/*  ** NOTE **  'siz' is size of DESTINATION buffer, NOT src!  */
/*  ** NOTE **  returns 'size_t' and NOT 'char*' like strncpy! */
HUT_DLL_IMPORT size_t
strlcpy(char *dst, const char *src, size_t siz);
#endif

#if !defined(HAVE_STRLCAT)
/* $OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $ */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
/*  ** NOTE **  'siz' is size of DESTINATION buffer (disregarding
                any existing data!), NOT size of src argument! */
/*  ** NOTE **  returns 'size_t' and NOT 'char*' like strncat! */
HUT_DLL_IMPORT size_t
strlcat(char *dst, const char *src, size_t siz);
#endif

/* The following helper macros can be used in place of direct calls
 * to either the strlcpy or strlcat functions ONLY when the destination
 * buffer is an array. They must NEVER be used whenever the destination
 * buffer is a pointer!
 */
#define STRLCPY( dst, src )     strlcpy( (dst), (src), sizeof(dst) )
#define STRLCAT( dst, src )     strlcat( (dst), (src), sizeof(dst) )

/* Subtract/add gettimeofday struct timeval */
HUT_DLL_IMPORT int timeval_subtract (struct timeval *beg_timeval, struct timeval *end_timeval, struct timeval *dif_timeval);
HUT_DLL_IMPORT int timeval_add      (struct timeval *dif_timeval, struct timeval *accum_timeval);

/*
  Easier to use timed_wait_condition that waits for the specified
  relative amount of time without you having to build an absolute
  timeout time yourself. Use the "timed_wait_condition_relative_usecs"
  macro to call it.
*/
#define timed_wait_condition_relative_usecs(      plc, plk, usecs, ptv )  \
        timed_wait_condition_relative_usecs_impl( plc, plk, usecs, ptv, PTT_LOC )

HUT_DLL_IMPORT int timed_wait_condition_relative_usecs_impl
(
    COND*            pCOND,     // ptr to condition to wait on
    LOCK*            pLOCK,     // ptr to controlling lock (must be held!)
    U32              usecs,     // max #of microseconds to wait
    struct timeval*  pTV,       // [OPTIONAL] ptr to tod value (may be NULL)
    const char*      loc        // (location)
);

/* Read/write to socket functions */
HUT_DLL_IMPORT int hprintf(int s,char *fmt,...) ATTR_PRINTF(2,3);
HUT_DLL_IMPORT int hwrite(int s,const char *,size_t);
HUT_DLL_IMPORT int hgetc(int s);
HUT_DLL_IMPORT char *hgets(char *b,size_t c,int s);


/* Posix 1003.e capabilities */
#if defined(OPTION_CAPABILITIES)
HUT_DLL_IMPORT int drop_privileges(int c);
HUT_DLL_IMPORT int drop_all_caps(void);
#endif

/* Hercules page-aligned calloc/free */
HUT_DLL_IMPORT  void*  hpcalloc ( BYTE type, size_t size );
HUT_DLL_IMPORT  void   hpcfree  ( BYTE type, void*  ptr  );

/* Hercules low-level file open */
HUT_DLL_IMPORT  int hopen( const char* path, int oflag, ... );

/* Trim path information from __FILE__ macro */
HUT_DLL_IMPORT const char* trimloc( const char* loc );
#define  TRIMLOC(_loc)     trimloc( _loc )

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
HUT_DLL_IMPORT char* FormatTIMEVAL( const TIMEVAL* pTV, char* buf, int bufsz );

/*-------------------------------------------------------------------*/
/* format memory size functions:   128K,  64M,  8G,  etc...          */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT char* fmt_memsize    ( const U64 memsize,   char* buf, const size_t bufsz );
HUT_DLL_IMPORT char* fmt_memsize_KB ( const U64 memsizeKB, char* buf, const size_t bufsz );
HUT_DLL_IMPORT char* fmt_memsize_MB ( const U64 memsizeMB, char* buf, const size_t bufsz );

/*-------------------------------------------------------------------*/
/* Pretty format S64 value with thousand separators. Returns length. */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT size_t fmt_S64( char dst[32], S64 num );

/*-------------------------------------------------------------------*/
/* Standard Utility Initialization                                   */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT int initialize_utility( int argc, char* argv[],
                                       char*  defpgm,
                                       char*  desc,
                                       char** pgm );

/*-------------------------------------------------------------------*/
/*                Reverse the bits in a BYTE                         */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT BYTE reverse_bits( BYTE b );

/*-------------------------------------------------------------------*/
/* Format printer FCB/CCTAPE information                             */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT void FormatFCB    ( char* buf, size_t buflen,
                                   int index, int lpi, int lpp,
                                   int* fcb );

HUT_DLL_IMPORT void FormatCCTAPE ( char* buf, size_t buflen,
                                   int lpi, int lpp,
                                   U16* cctape);

/*-------------------------------------------------------------------*/
/* Count number of tokens in a string                                */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT int tkcount( const char* str, const char* delims );

/*-------------------------------------------------------------------*/
/* Remove leading and/or trailing chars from a string and return str */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT char*  ltrim ( char* str, const char* dlm ); // (left trim)
HUT_DLL_IMPORT char*  rtrim ( char* str, const char* dlm ); // (right trim)
HUT_DLL_IMPORT char*   trim ( char* str, const char* dlm ); // (trim both)

/* Helper macros that trims whitespace as opposed to something else */

#define WHITESPACE      " \t\n\v\f\r"

#define LTRIM( str )  ltrim ( (str), WHITESPACE )
#define RTRIM( str )  rtrim ( (str), WHITESPACE )
#define  TRIM( str )   trim ( (str), WHITESPACE )

#if defined( HAVE_PTHREAD_SETNAME_NP ) // !defined( _MSVC_ ) implied
/*-------------------------------------------------------------------*/
/* Set thead name           (nonstandard GNU extension)              */
/*                          (note: retcode is error code, NOT errno) */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT int nix_set_thread_name( pthread_t tid, const char* name );
#endif

/*-------------------------------------------------------------------*/
/* Hercules command line parsing function                            */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT int parse_args( char* p, int maxargc, char** pargv, int* pargc );

/*-------------------------------------------------------------------*/
/* Ensure all calls to "rand()" return a hopefully unique value      */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT void init_random();

/*-------------------------------------------------------------------*/
/* Check if string is numeric                                        */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT bool is_numeric( const char* str );
HUT_DLL_IMPORT bool is_numeric_l( const char* str, int len );

/*-------------------------------------------------------------------*/
/* Subroutines to convert strings to upper or lower case             */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT void string_to_upper (char *source);
HUT_DLL_IMPORT void string_to_lower (char *source);

/*-------------------------------------------------------------------*/
/* Subroutine to convert a string to EBCDIC and pad with blanks      */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT void convert_to_ebcdic( BYTE* dest, int len, const char* source );

/*-------------------------------------------------------------------*/
/* Subroutine to convert an EBCDIC string to an ASCIIZ string.       */
/* Removes trailing blanks and adds a terminating null.              */
/* Returns the length of the ASCII string excluding terminating null */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT int  make_asciiz (char *dest, int destlen, BYTE *src, int srclen);

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
HUT_DLL_IMPORT int  idx_snprintf( int idx, char* buffer, size_t bufsiz, const char* fmt, ... ) ATTR_PRINTF(4,5);

#endif /* __HSCUTL_H__ */
