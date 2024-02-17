/* HSCUTL.H     (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
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

#define USLEEP( _u ) herc_usleep( _u, __FILE__, __LINE__ )
HUT_DLL_IMPORT int herc_usleep( useconds_t usecs, const char* file, int line );
#define USLEEP_MIN 1
#define NANOSLEEP_EINTR_RETRY_WARNING_TRESHOLD 256

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

#define LTRIMS( str, s )  ltrim ( (str), (s) )
#define RTRIMS( str, s )  rtrim ( (str), (s) )

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
/* Check if string is numeric or hexadecimal                         */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT bool is_numeric( const char* str );
HUT_DLL_IMPORT bool is_numeric_l( const char* str, size_t len );
HUT_DLL_IMPORT bool is_hex( const char* str );
HUT_DLL_IMPORT bool is_hex_l( const char* str, size_t len );

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

/*-------------------------------------------------------------------*/
/*                Processor types functions                          */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT const char* ptyp2long ( BYTE ptyp );
HUT_DLL_IMPORT const char* ptyp2short( BYTE ptyp );
HUT_DLL_IMPORT BYTE short2ptyp( const char* shortname );

/*-------------------------------------------------------------------*/
/*                Store PSW helper function                          */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT U64 do_make_psw64( PSW* psw, BYTE real_ilc, int arch /*370/390/900*/, bool bc );

/*-------------------------------------------------------------------*/
/*              Return Program Interrupt Name                        */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT const char* PIC2Name( int code );

/*-------------------------------------------------------------------*/
/*                 Return SIGP Order Name                            */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT const char* order2name( BYTE order );

/*-------------------------------------------------------------------*/
/*                 Return PER Event name(s)                          */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT const char* perc2name( BYTE perc, char* buf, size_t bufsiz );

/*-------------------------------------------------------------------*/
/*      Format Operation-Request Block (ORB) for display             */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT const char* FormatORB( ORB* orb, char* buf, size_t bufsz );

/*-------------------------------------------------------------------*/
/*      Determine if running on a big endian system or not           */
/*-------------------------------------------------------------------*/
HUT_DLL_IMPORT bool are_big_endian();



/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**                    TraceFile support                            **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

#define TF_FMT0   '0'       // Format 0 = original release
#define TF_FMT1   '1'       // Format 1 = 64 bytes of E7 CCW data
#define TF_FMT2   '2'       // Format 2 = TFHDR thread id/name

#define TF_FMT  TF_FMT2     // Current TraceFile file-format

#undef ATTRIBUTE_PACKED
#if defined(_MSVC_)
 #pragma pack(push)
 #pragma pack(1)
 #define ATTRIBUTE_PACKED
#else
 #define ATTRIBUTE_PACKED __attribute__((packed))
#endif

//---------------------------------------------------------------------
//                 TraceFile SYSTEM Record
//---------------------------------------------------------------------
struct TFSYS
{
    char    ffmt[ 4 ];      // "%TFn" where 'n' = file format
    BYTE    bigend;         // 1 = Big Endian, 0 = Little Endian
    BYTE    engs;           // MAX_CPU_ENGS
    U16     archnum0;       // _ARCH_NUM_0 (usually 370)
    U16     archnum1;       // _ARCH_NUM_1 (usually 390)
    U16     archnum2;       // _ARCH_NUM_2 (usually 900)
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
    TIMEVAL beg_tod;        // Time of day trace started
    TIMEVAL end_tod;        // Time of day trace ended
    U64     tot_ins;        // Total instructions traced
    U64     tot_dev;        // Total device trace records
    char    version[ 128 ]; // Hercules version string (ASCII)
    BYTE    ptyp[ ROUND_UP( MAX_CPU_ENGS, 8 ) ]; // Processor type for each CPU
}
ATTRIBUTE_PACKED; typedef struct TFSYS TFSYS;
CASSERT( sizeof( TFSYS ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//                 TraceFile Record Header
//---------------------------------------------------------------------
struct TFHDR
{
    U16     prev;           // Previous record length
    U16     curr;           // Current record length (including TFHDR)
    U16     cpuad;          // CPU Address
    U16     msgnum;         // Message Number (2269 - 2272, etc)
    TIMEVAL tod;            // Time of day of record/event
    BYTE    arch_mode;      // Architecture mode for this CPU
    BYTE    lcss;           // LCSS (0-7)
    U16     devnum;         // Device number
    BYTE    pad [ 4 ];      // (padding/alignment/unused)

    // Format-2...

    U64     tidnum;         // Thread-Id number (not 'TID'!)
    char    thrdname[16];   // Thread name
}
ATTRIBUTE_PACKED; typedef struct TFHDR TFHDR;
CASSERT( sizeof( TFHDR ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00423 Search Key Record
//---------------------------------------------------------------------
struct TF00423
{
    TFHDR   rhdr;           // Record Header
    BYTE    kl;             // Key length
    BYTE    key[45];        // Searched Key
    BYTE    pad [ 2 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00423 TF00423;
CASSERT( sizeof( TF00423 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00424 Cur trk Record
//---------------------------------------------------------------------
struct TF00424
{
    TFHDR   rhdr;           // Record Header
    S32     trk;            // Track
    S32     bufcur;         // Buffer data identifier
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00424 TF00424;
CASSERT( sizeof( TF00424 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00425 Updating track Record
//---------------------------------------------------------------------
struct TF00425
{
    TFHDR   rhdr;           // Record Header
    S32     bufcur;         // Buffer data identifier
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00425 TF00425;
CASSERT( sizeof( TF00425 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00426 Cache hit Record
//---------------------------------------------------------------------
struct TF00426
{
    TFHDR   rhdr;           // Record Header
    S32     trk;            // Track
    S32     idx;            // Cache index
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00426 TF00426;
CASSERT( sizeof( TF00426 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00427 Unavailable cache Record
//---------------------------------------------------------------------
struct TF00427
{
    TFHDR   rhdr;           // Record Header
    S32     trk;            // Track
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00427 TF00427;
CASSERT( sizeof( TF00427 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00428 Cache miss Record
//---------------------------------------------------------------------
struct TF00428
{
    TFHDR   rhdr;           // Record Header
    S32     trk;            // Track
    S32     idx;            // Cache index
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00428 TF00428;
CASSERT( sizeof( TF00428 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00429 Offset Record
//---------------------------------------------------------------------
struct TF00429
{
    TFHDR   rhdr;           // Record Header
    S32     trk;            // Track
    S32     fnum;           // File num?
    S32     len;            // Length
    BYTE    pad2[ 4 ];      // (padding/alignment/unused)
    U64     offset;         // Offset
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00429 TF00429;
CASSERT( sizeof( TF00429 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00430 Trkhdr Record
//---------------------------------------------------------------------
struct TF00430
{
    TFHDR   rhdr;           // Record Header
    S32     trk;            // Track
    BYTE    buf[5];         // Track Header
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00430 TF00430;
CASSERT( sizeof( TF00430 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00431 Seeking Record
//---------------------------------------------------------------------
struct TF00431
{
    TFHDR   rhdr;           // Record Header
    S32     cyl;            // Cylinder
    S32     head;           // Head
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00431 TF00431;
CASSERT( sizeof( TF00431 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00432 MT advance error Record
//---------------------------------------------------------------------
struct TF00432
{
    TFHDR   rhdr;           // Record Header
    BYTE    count;          // Count
    BYTE    mask;           // Mask
    BYTE    pad [ 6 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00432 TF00432;
CASSERT( sizeof( TF00432 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00433 MT advance Record
//---------------------------------------------------------------------
struct TF00433
{
    TFHDR   rhdr;           // Record Header
    S32     cyl;            // Cylinder
    S32     head;           // Head
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00433 TF00433;
CASSERT( sizeof( TF00433 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00434 Read count orientation Record
//---------------------------------------------------------------------
struct TF00434
{
    TFHDR   rhdr;           // Record Header
    BYTE    orient;         // Orientation
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00434 TF00434;
CASSERT( sizeof( TF00434 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00435 Cyl head record kl dl Record
//---------------------------------------------------------------------
struct TF00435
{
    TFHDR   rhdr;           // Record Header
    U16     dl;             // Data length
    BYTE    pad [ 2 ];      // (padding/alignment/unused)
    S32     cyl;            // Cylinder
    S32     head;           // Head
    S32     record;         // Record
    S32     kl;             // Key length
    U32     offset;         // Track offset
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00435 TF00435;
CASSERT( sizeof( TF00435 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00436 Read key Record
//---------------------------------------------------------------------
struct TF00436
{
    TFHDR   rhdr;           // Record Header
    S32     kl;             // Key length
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00436 TF00436;
CASSERT( sizeof( TF00436 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00437 Read data Record
//---------------------------------------------------------------------
struct TF00437
{
    TFHDR   rhdr;           // Record Header
    U16     dl;             // Data length
    BYTE    pad [ 6 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00437 TF00437;
CASSERT( sizeof( TF00437 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00438 Write cyl head record kl dl Record
//---------------------------------------------------------------------
struct TF00438
{
    TFHDR   rhdr;           // Record Header
    S32     cyl;            // Cylinder
    S32     head;           // Head
    U16     datalen;        // Data length
    BYTE    recnum;         // Record number
    BYTE    keylen;         // Key length
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00438 TF00438;
CASSERT( sizeof( TF00438 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00439 Set track overflow flag Record
//---------------------------------------------------------------------
struct TF00439
{
    TFHDR   rhdr;           // Record Header
    S32     cyl;            // Cylinder
    S32     head;           // Head
    BYTE    recnum;         // Record number
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00439 TF00439;
CASSERT( sizeof( TF00439 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00440 Update cyl head record kl dl Record
//---------------------------------------------------------------------
struct TF00440
{
    TFHDR   rhdr;           // Record Header
    S32     cyl;            // Cylinder
    S32     head;           // Head
    S32     recnum;         // Record number
    S32     keylen;         // Key length
    U16     datalen;        // Data length
    BYTE    pad [ 6 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00440 TF00440;
CASSERT( sizeof( TF00440 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00441 Update cyl head record dl Record
//---------------------------------------------------------------------
struct TF00441
{
    TFHDR   rhdr;           // Record Header
    S32     cyl;            // Cylinder
    S32     head;           // Head
    S32     recnum;         // Record number
    U16     datalen;        // Data length
    BYTE    pad [ 2 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00441 TF00441;
CASSERT( sizeof( TF00441 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00442 Set file mask Record
//---------------------------------------------------------------------
struct TF00442
{
    TFHDR   rhdr;           // Record Header
    BYTE    mask;           // Mask
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00442 TF00442;
CASSERT( sizeof( TF00442 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00516 Cache hit Record
//---------------------------------------------------------------------
struct TF00516
{
    TFHDR   rhdr;           // Record Header
    S32     blkgrp;         // Block group
    S32     idx;            // Cache index
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00516 TF00516;
CASSERT( sizeof( TF00516 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00517 Unavailable cache Record
//---------------------------------------------------------------------
struct TF00517
{
    TFHDR   rhdr;           // Record Header
    S32     blkgrp;         // Block group
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00517 TF00517;
CASSERT( sizeof( TF00517 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00518 Cache miss Record
//---------------------------------------------------------------------
struct TF00518
{
    TFHDR   rhdr;           // Record Header
    S32     blkgrp;         // Block group
    S32     idx;            // Cache index
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00518 TF00518;
CASSERT( sizeof( TF00518 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00519 Offset len Record
//---------------------------------------------------------------------
struct TF00519
{
    TFHDR   rhdr;           // Record Header
    S32     blkgrp;         // Block group
    S32     len;            // Length
    U64     offset;         // Offset
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00519 TF00519;
CASSERT( sizeof( TF00519 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00520 Positioning Record
//---------------------------------------------------------------------
struct TF00520
{
    TFHDR   rhdr;           // Record Header
    U64     rba;            // Relative Byte Offset
    char    filename[ 256 ];
}
ATTRIBUTE_PACKED; typedef struct TF00520 TF00520;
CASSERT( sizeof( TF00520 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00800 Wait State PSW Record
//---------------------------------------------------------------------
struct TF00800
{
    TFHDR   rhdr;           // Record Header
    PSW     psw;            // PSW
    BYTE    amode64;        // 64-bit addressing
    BYTE    amode;          // 31-bit addressing
    BYTE    zeroilc;        // 1=Zero ILC
    BYTE    pad [ 5 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00800 TF00800;
CASSERT( sizeof( TF00800 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00801 Program Interrupt Record
//---------------------------------------------------------------------
struct TF00801
{
    TFHDR   rhdr;           // Record Header
    U16     pcode;          // Interruption code
    BYTE    sie;            // SIE mode
    BYTE    ilc;            // Instruction Length Code
    U32     dxc;            // Data-Exception code
    U32     why;            // TXF why transaction aborted
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00801 TF00801;
CASSERT( sizeof( TF00801 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00802 PER Event Record
//---------------------------------------------------------------------
struct TF00802
{
    TFHDR   rhdr;           // Record Header
    U64     ia;             // PSW Address
    U32     pcode;          // Program Interrupt code
    U16     perc;           // PER code
    BYTE    pad [ 2 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00802 TF00802;
CASSERT( sizeof( TF00802 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00803 Program Interrupt Loop Record
//---------------------------------------------------------------------
struct TF00803
{
    TFHDR   rhdr;           // Record Header
    char    str[40];        // Formatted PSW string
}
ATTRIBUTE_PACKED; typedef struct TF00803 TF00803;
CASSERT( sizeof( TF00803 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00804 I/O Interrupt (S/370) Record
//---------------------------------------------------------------------
struct TF00804
{
    TFHDR   rhdr;           // Record Header
    BYTE    csw[8];         // Channel Status Word
    U16     ioid;           // Device number
    BYTE    lcss;           // LCSS (0-7)
    BYTE    pad [ 5 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00804 TF00804;
CASSERT( sizeof( TF00804 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00806 I/O Interrupt Record
//        (handles both HHC00805 and HHC00806)
//---------------------------------------------------------------------
struct TF00806
{
    TFHDR   rhdr;           // Record Header
    U32     ioid;           // I/O interruption address
    U32     ioparm;         // I/O interruption parameter
    U32     iointid;        // I/O interruption ident
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00806 TF00806;
CASSERT( sizeof( TF00806 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00807 Machine Check Interrupt Record
//---------------------------------------------------------------------
struct TF00807
{
    TFHDR   rhdr;           // Record Header
    U64     mcic;           // Interruption code
    U64     fsta;           // Failing storage address
    U32     xdmg;           // External-Damage code
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00807 TF00807;
CASSERT( sizeof( TF00807 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00808 Store Status Record
//---------------------------------------------------------------------
struct TF00808
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF00808 TF00808;
CASSERT( sizeof( TF00808 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00809 Disabled Wait State Record
//---------------------------------------------------------------------
struct TF00809
{
    TFHDR   rhdr;           // Record Header
    char    str[40];        // Formatted PSW string
}
ATTRIBUTE_PACKED; typedef struct TF00809 TF00809;
CASSERT( sizeof( TF00809 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00811 Architecture mode Record
//---------------------------------------------------------------------
struct TF00811
{
    TFHDR   rhdr;           // Record Header
    char    archname[8];    // Architecture name
}
ATTRIBUTE_PACKED; typedef struct TF00811 TF00811;
CASSERT( sizeof( TF00811 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00812 Vector facility online (370/390) Record
//---------------------------------------------------------------------
struct TF00812
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF00812 TF00812;
CASSERT( sizeof( TF00812 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00814 Signal Processor Record
//---------------------------------------------------------------------
struct TF00814
{
    TFHDR   rhdr;           // Record Header
    U16     cpad;           // Target CPU
    BYTE    order;          // Order
    BYTE    cc;             // Condition code
    U32     status;         // Status
    U64     parm;           // Parameter
    BYTE    got_status;     // boolean
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00814 TF00814;
CASSERT( sizeof( TF00814 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00840 External Interrupt Record
//---------------------------------------------------------------------
struct TF00840              // Messages HHC00840 .. HHC00843
{
    TFHDR   rhdr;           // Record Header
    S64     cpu_timer;      // CPU_TIMER
    U16     icode;          // Interruption code
    BYTE    pad [ 6 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00840 TF00840;
CASSERT( sizeof( TF00840 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00844 Block I/O Interrupt Record
//---------------------------------------------------------------------
struct TF00844
{
    TFHDR   rhdr;           // Record Header
    U64     bioparm;        // Block I/O interrupt parm
    U16     servcode;       // External interrupt code
    BYTE    biostat;        // Block I/O status
    BYTE    biosubcd;       // Block I/O sub int. code
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00844 TF00844;
CASSERT( sizeof( TF00844 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00845 Block I/O External interrupt Record
//---------------------------------------------------------------------
struct TF00845
{
    TFHDR   rhdr;           // Record Header
    U64     bioparm;        // Block I/O interrupt parm
    BYTE    biosubcd;       // Block I/O sub int. code
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00845 TF00845;
CASSERT( sizeof( TF00845 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF00846 Service Signal External Interrupt Record
//---------------------------------------------------------------------
struct TF00846
{
    TFHDR   rhdr;           // Record Header
    U32     servparm;       // Service signal parameter
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF00846 TF00846;
CASSERT( sizeof( TF00846 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01300 Halt subchannel Record
//---------------------------------------------------------------------
struct TF01300
{
    TFHDR   rhdr;           // Record Header
    BYTE    cc;             // Condition Code
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF01300 TF01300;
CASSERT( sizeof( TF01300 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01301 IDAW/MIDAW Record
//---------------------------------------------------------------------
struct TF01301
{
    TFHDR   rhdr;           // Record Header
    U64     addr;           // IDAW/MIDAW address
    U16     count;          // CCW byte count
    BYTE    amt;            // Data amount
    BYTE    type;           // IDA type (IDAW1/IDAW2/MIDAW)
    BYTE    mflag;          // MIDAW flag

    // Format-0...

//  BYTE    pad [ 3 ];      // (padding/alignment/unused)
//  BYTE    data[16];       // IDAW/MIDAW data (amt <= 16)

    // Format-1...

    BYTE    code;           // CCW opcode (if E7, amt = 64)
    BYTE    pad [ 2 ];      // (padding/alignment/unused)
    BYTE    data[64];       // IDAW/MIDAW data (amt <= 64)
}
ATTRIBUTE_PACKED; typedef struct TF01301 TF01301;
CASSERT( sizeof( TF01301 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01304 Attention signaled Record
//---------------------------------------------------------------------
struct TF01304
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01304 TF01304;
CASSERT( sizeof( TF01304 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01305 Attention Record
//---------------------------------------------------------------------
struct TF01305
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01305 TF01305;
CASSERT( sizeof( TF01305 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01306 Initial status interrupt Record
//---------------------------------------------------------------------
struct TF01306
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01306 TF01306;
CASSERT( sizeof( TF01306 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01307 Attention completed Record
//---------------------------------------------------------------------
struct TF01307
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01307 TF01307;
CASSERT( sizeof( TF01307 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01308 Clear completed Record
//---------------------------------------------------------------------
struct TF01308
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01308 TF01308;
CASSERT( sizeof( TF01308 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01309 Halt completed Record
//---------------------------------------------------------------------
struct TF01309
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01309 TF01309;
CASSERT( sizeof( TF01309 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01310 Suspended Record
//---------------------------------------------------------------------
struct TF01310
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01310 TF01310;
CASSERT( sizeof( TF01310 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01311 Resumed Record
//---------------------------------------------------------------------
struct TF01311
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01311 TF01311;
CASSERT( sizeof( TF01311 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01312 I/O stat Record
//---------------------------------------------------------------------
struct TF01312
{
    TFHDR   rhdr;           // Record Header
    U32     residual;       // residual
    BYTE    unitstat;       // unitstat
    BYTE    chanstat;       // chanstat
    BYTE    amt;            // data amount
    BYTE    pad [ 1 ];      // (padding/alignment/unused)
    BYTE    data[16];       // iobuf data
}
ATTRIBUTE_PACKED; typedef struct TF01312 TF01312;
CASSERT( sizeof( TF01312 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01313 Sense Record
//---------------------------------------------------------------------
struct TF01313
{
    TFHDR   rhdr;           // Record Header
    BYTE    sense[32];      // Sense
    char    sns[128];       // Device interpretation of sense bytes 0-1
}
ATTRIBUTE_PACKED; typedef struct TF01313 TF01313;
CASSERT( sizeof( TF01313 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01315 CCW Record
//---------------------------------------------------------------------
struct TF01315
{
    TFHDR   rhdr;           // Record Header
    U32     addr;           // CCW data address
    U16     count;          // CCW byte count
    BYTE    amt;            // Data amount
    BYTE    pad [ 1 ];      // (padding/alignment/unused)
    BYTE    ccw[8];         // CCW

    // Format-0...

//  BYTE    data[16];       // CCW data (amt <= 16)

    // Format-1...

    BYTE    data[64];       // CCW data (amt <= 64)
}
ATTRIBUTE_PACKED; typedef struct TF01315 TF01315;
CASSERT( sizeof( TF01315 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01316 CSW (370) Record
//---------------------------------------------------------------------
struct TF01316
{
    TFHDR   rhdr;           // Record Header
    BYTE    csw[8];         // CSW
}
ATTRIBUTE_PACKED; typedef struct TF01316 TF01316;
CASSERT( sizeof( TF01316 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01317 SCSW Record
//---------------------------------------------------------------------
struct TF01317
{
    TFHDR   rhdr;           // Record Header
    SCSW    scsw;           // SCSW
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF01317 TF01317;
CASSERT( sizeof( TF01317 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01318 TEST I/O Record
//---------------------------------------------------------------------
struct TF01318
{
    TFHDR   rhdr;           // Record Header
    BYTE    cc;             // Condition Code
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF01318 TF01318;
CASSERT( sizeof( TF01318 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//      TraceFile TF01320 S/370 SIO conversion started Record
//---------------------------------------------------------------------
struct TF01320
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01320 TF01320;
CASSERT( sizeof( TF01320 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01321 S/370 SIO conversion success Record
//---------------------------------------------------------------------
struct TF01321
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01321 TF01321;
CASSERT( sizeof( TF01321 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01329 Halt I/O Record
//---------------------------------------------------------------------
struct TF01329
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01329 TF01329;
CASSERT( sizeof( TF01329 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01330 HIO modification Record
//---------------------------------------------------------------------
struct TF01330
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01330 TF01330;
CASSERT( sizeof( TF01330 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01331 Clear subchannel Record
//---------------------------------------------------------------------
struct TF01331
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01331 TF01331;
CASSERT( sizeof( TF01331 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01332 Halt subchannel Record
//---------------------------------------------------------------------
struct TF01332
{
    TFHDR   rhdr;           // Record Header
}
ATTRIBUTE_PACKED; typedef struct TF01332 TF01332;
CASSERT( sizeof( TF01332 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01333 Resume subchannel Record
//---------------------------------------------------------------------
struct TF01333
{
    TFHDR   rhdr;           // Record Header
    BYTE    cc;             // Condition Code
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF01333 TF01333;
CASSERT( sizeof( TF01333 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01334 ORB Record
//---------------------------------------------------------------------
struct TF01334
{
    TFHDR   rhdr;           // Record Header
    ORB     orb;            // ORB
}
ATTRIBUTE_PACKED; typedef struct TF01334 TF01334;
CASSERT( sizeof( TF01334 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF01336 Startio cc=2 Record
//---------------------------------------------------------------------
struct TF01336
{
    TFHDR   rhdr;           // Record Header
    BYTE    busy;           // busy
    BYTE    startpending;   // startpending
    BYTE    pad [ 6 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF01336 TF01336;
CASSERT( sizeof( TF01336 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF02269 General Purpose Registers Record
//---------------------------------------------------------------------
struct TF02269
{
    TFHDR   rhdr;           // Record Header
    BYTE    ifetch_error;   // true == instruction fetch error
    BYTE    sie;            // true == SIE mode
    BYTE    pad [ 6 ];      // (padding/alignment/unused)
    DW      gr[16];         // General registers
}
ATTRIBUTE_PACKED; typedef struct TF02269 TF02269;
CASSERT( sizeof( TF02269 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//      TraceFile TF02270 Floating Point Registers Record
//---------------------------------------------------------------------
struct TF02270
{
    TFHDR   rhdr;           // Record Header
    U32     fpr[32];        // FP registers
    BYTE    afp;            // CR0 AFP enabled
    BYTE    pad [ 7 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF02270 TF02270;
CASSERT( sizeof( TF02270 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//          TraceFile TF02271 Control Registers Record
//---------------------------------------------------------------------
struct TF02271
{
    TFHDR   rhdr;           // Record Header
    DW      cr[16];         // Control registers
}
ATTRIBUTE_PACKED; typedef struct TF02271 TF02271;
CASSERT( sizeof( TF02271 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//          TraceFile TF02272 Access Registers Record
//---------------------------------------------------------------------
struct TF02272
{
    TFHDR   rhdr;           // Record Header
    U32     ar[16];         // Access registers
}
ATTRIBUTE_PACKED; typedef struct TF02272 TF02272;
CASSERT( sizeof( TF02272 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//   TraceFile TF02276 Floating Point Control Register Record
//---------------------------------------------------------------------
struct TF02276
{
    TFHDR   rhdr;           // Record Header
    U32     fpc;            // FP Control register
    BYTE    afp;            // CR0 AFP enabled
    BYTE    pad [ 3 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF02276 TF02276;
CASSERT( sizeof( TF02276 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//     TraceFile TF02324 Primary Instruction Trace Record
//---------------------------------------------------------------------
struct TF02324
{
    TFHDR   rhdr;           // Record Header
    PSW     psw;            // PSW
    BYTE    amode64;        // 64-bit addressing
    BYTE    amode;          // 31-bit addressing
    BYTE    zeroilc;        // 1=Zero ILC
    BYTE    sie;            // SIE mode
    BYTE    ilc;            // Instruction Length Code
    BYTE    pad [ 3 ];      // (padding/alignment/unused)
    BYTE    inst[6];        // Instruction
    BYTE    pad2[ 2 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TF02324 TF02324;
CASSERT( sizeof( TF02324 ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//         TraceFile Storage Operands Structure
//---------------------------------------------------------------------
struct TFOP
{
    U64     vaddr;          // Effective/Virtual Address
    U64     raddr;          // Real storage address
    BYTE    stor[16];       // Actual storage contents
    U16     xcode;          // Translation Exception code
    BYTE    skey;           // Storage key
    BYTE    amt;            // How much of storage we have
    BYTE    pad [ 4 ];      // (padding/alignment/unused)
}
ATTRIBUTE_PACKED; typedef struct TFOP TFOP;
CASSERT( sizeof( TFOP ) % 8 == 0, hscutl_h );

//---------------------------------------------------------------------
//       TraceFile TF02326 Instruction Operands Record
//---------------------------------------------------------------------
struct TF02326
{
    TFHDR   rhdr;           // Record Header
    BYTE    valid;          // Valid instruction
    BYTE    sie;            // SIE mode
    BYTE    real_mode;      // true == DAT not enabled
    BYTE    pad [ 1 ];      // (padding/alignment/unused)
    S16     b1;             // Operand-1 register number (if >= 0)
    S16     b2;             // Operand-2 register number (if >= 0)
    TFOP    op1;            // OPERAND-1
    TFOP    op2;            // OPERAND-2
}
ATTRIBUTE_PACKED; typedef struct TF02326 TF02326;
CASSERT( sizeof( TF02326 ) % 8 == 0, hscutl_h );


#if defined(_MSVC_)
 #pragma pack(pop)
#endif


// Device tracing -----------------------------------------------------------------------------

HUT_DLL_IMPORT bool tf_0423( DEVBLK* dev,              // Search key
                             size_t  kl,
                             BYTE*   key );

HUT_DLL_IMPORT bool tf_0424( DEVBLK* dev,              // Cur trk
                             S32     trk );

HUT_DLL_IMPORT bool tf_0425( DEVBLK* dev );            // Updating track

HUT_DLL_IMPORT bool tf_0426( DEVBLK* dev,              // Cache hit
                             S32     trk,
                             S32     i );

HUT_DLL_IMPORT bool tf_0427( DEVBLK* dev,              // Unavailable cache
                             S32     trk );

HUT_DLL_IMPORT bool tf_0428( DEVBLK* dev,              // Cache miss
                             S32     trk,
                             S32     o );

HUT_DLL_IMPORT bool tf_0429( DEVBLK* dev,              // Offset
                             S32     trk,
                             S32     fnum );

HUT_DLL_IMPORT bool tf_0430( DEVBLK* dev,              // Trkhdr
                             S32     trk );

HUT_DLL_IMPORT bool tf_0431( DEVBLK* dev,              // Seeking
                             int     cyl,
                             int     head );

HUT_DLL_IMPORT bool tf_0432( DEVBLK* dev );            // MT advance error

HUT_DLL_IMPORT bool tf_0433( DEVBLK* dev,              // MT advance
                             int     cyl,
                             int     head );

HUT_DLL_IMPORT bool tf_0434( DEVBLK* dev );            // Read count orientation

HUT_DLL_IMPORT bool tf_0435( DEVBLK* dev );            // Cyl head record kl dl

HUT_DLL_IMPORT bool tf_0436( DEVBLK* dev );            // Read key

HUT_DLL_IMPORT bool tf_0437( DEVBLK* dev );            // Read data

HUT_DLL_IMPORT bool tf_0438( DEVBLK* dev,              // Write cyl head record kl dl
                             BYTE    recnum,
                             BYTE    keylen,
                             U16     datalen );

HUT_DLL_IMPORT bool tf_0439( DEVBLK* dev,              // Set track overflow flag
                             BYTE    recnum );

HUT_DLL_IMPORT bool tf_0440( DEVBLK* dev );            // Update cyl head record kl dl

HUT_DLL_IMPORT bool tf_0441( DEVBLK* dev );            // Update cyl head record dl

HUT_DLL_IMPORT bool tf_0442( DEVBLK* dev );            // Set file mask

HUT_DLL_IMPORT bool tf_0516( DEVBLK* dev,              // Cache hit
                             S32     blkgrp,
                             S32     i );

HUT_DLL_IMPORT bool tf_0517( DEVBLK* dev,              // Unavailable cache
                             S32     blkgrp );

HUT_DLL_IMPORT bool tf_0518( DEVBLK* dev,              // Cache miss
                             S32     blkgrp,
                             S32     o );

HUT_DLL_IMPORT bool tf_0519( DEVBLK* dev,              // Offset len
                             S32     blkgrp,
                             U64     offset,
                             S32     len );

HUT_DLL_IMPORT bool tf_0520( DEVBLK* dev );            // Positioning

// Instruction tracing ------------------------------------------------------------------------

HUT_DLL_IMPORT bool tf_0800( REGS* regs );             // Wait State PSW

HUT_DLL_IMPORT bool tf_0801( REGS* regs,               // Program Interrupt
                             U16   pcode,
                             BYTE  ilc );

HUT_DLL_IMPORT bool tf_0802( REGS* regs,               // PER Event
                             U64   ia,
                             U32   pcode,
                             U16   perc );

HUT_DLL_IMPORT bool tf_0803( REGS* regs,               // Program Interrupt Loop
                             const char* str );

HUT_DLL_IMPORT bool tf_0804( REGS* regs,               // I/O Interrupt (S/370)
                             BYTE* csw,
                             U16   ioid,
                             BYTE  lcss );

HUT_DLL_IMPORT bool tf_0806( REGS* regs,               // I/O Interrupt (handles both HHC00805 and HHC00806)
                             U32   ioid,
                             U32   ioparm,
                             U32   iointid );

HUT_DLL_IMPORT bool tf_0807( REGS* regs,               // Machine Check Interrupt
                             U64   mcic,
                             U64   fsta,
                             U32   xdmg );

HUT_DLL_IMPORT bool tf_0808( REGS* regs );             // Store Status

HUT_DLL_IMPORT bool tf_0809( REGS* regs,               // Disabled Wait State
                             const char* str );

HUT_DLL_IMPORT bool tf_0811( REGS* regs,               // Architecture mode
                             const char* archname );

HUT_DLL_IMPORT bool tf_0812( REGS* regs );             // Vector facility online (370/390)

HUT_DLL_IMPORT bool tf_0814( REGS* regs,               // Signal Processor
                             BYTE  order,
                             BYTE  cc,
                             U16   cpad,
                             U32   status,
                             U64   parm,
                             BYTE  got_status );

HUT_DLL_IMPORT bool tf_0840( REGS* regs,               // External Interrupt
                             U16   icode );

HUT_DLL_IMPORT bool tf_0844( REGS* regs );             // Block I/O Interrupt

HUT_DLL_IMPORT bool tf_0845( REGS* regs );             // Block I/O External interrupt

HUT_DLL_IMPORT bool tf_0846( REGS* regs );             // Service Signal External Interrupt

// Device tracing -----------------------------------------------------------------------------

HUT_DLL_IMPORT bool tf_1300( DEVBLK* dev,              // Halt subchannel
                             BYTE    cc );

HUT_DLL_IMPORT bool tf_1301( const DEVBLK* dev,        // IDAW/MIDAW
                       const U64     addr,
                       const U16     count,
                             BYTE*   data,
                             BYTE    amt,
                       const BYTE    flag,
                       const BYTE    type );

HUT_DLL_IMPORT bool tf_1304( DEVBLK* dev );            // Attention signaled

HUT_DLL_IMPORT bool tf_1305( DEVBLK* dev );            // Attention

HUT_DLL_IMPORT bool tf_1306( DEVBLK* dev );            // Initial status interrupt

HUT_DLL_IMPORT bool tf_1307( DEVBLK* dev );            // Attention completed

HUT_DLL_IMPORT bool tf_1308( DEVBLK* dev );            // Clear completed

HUT_DLL_IMPORT bool tf_1309( DEVBLK* dev );            // Halt completed

HUT_DLL_IMPORT bool tf_1310( DEVBLK* dev );            // Suspended

HUT_DLL_IMPORT bool tf_1311( DEVBLK* dev );            // Resumed

HUT_DLL_IMPORT bool tf_1312( DEVBLK* dev,              // I/O stat
                             BYTE    unitstat,
                             BYTE    chanstat,
                             BYTE    amt,
                             U32     residual,
                             BYTE*   iobuf_data );

HUT_DLL_IMPORT bool tf_1313( DEVBLK* dev );            // Sense

HUT_DLL_IMPORT bool tf_1315( const DEVBLK* dev,        // CCW
                       const BYTE*   ccw,
                       const U32     addr,
                       const U16     count,
                             BYTE*   data,
                             BYTE    amt );

HUT_DLL_IMPORT bool tf_1316( const DEVBLK* dev,        // CSW (370)
                             const BYTE csw[] );

HUT_DLL_IMPORT bool tf_1317( const DEVBLK* dev,        // SCSW
                             const SCSW scsw );

HUT_DLL_IMPORT bool tf_1318( DEVBLK* dev,              // TEST I/O
                             BYTE    cc );

HUT_DLL_IMPORT bool tf_1320( DEVBLK* dev );            // S/370 start I/O conversion started

HUT_DLL_IMPORT bool tf_1321( DEVBLK* dev );            // S/370 start I/O conversion success

HUT_DLL_IMPORT bool tf_1329( DEVBLK* dev );            // Halt I/O

HUT_DLL_IMPORT bool tf_1330( DEVBLK* dev );            // HIO modification

HUT_DLL_IMPORT bool tf_1331( DEVBLK* dev );            // Clear subchannel

HUT_DLL_IMPORT bool tf_1332( DEVBLK* dev );            // Halt subchannel

HUT_DLL_IMPORT bool tf_1333( DEVBLK* dev,              // Resume subchannel
                             BYTE    cc );

HUT_DLL_IMPORT bool tf_1334( DEVBLK* dev,              // ORB
                             ORB*    orb );

HUT_DLL_IMPORT bool tf_1336( DEVBLK* dev );            // Startio cc=2

// Instruction tracing ------------------------------------------------------------------------

HUT_DLL_IMPORT bool tf_2269( REGS* regs,               // General Registers
                             BYTE* inst );

HUT_DLL_IMPORT bool tf_2270( REGS* regs );             // Floating Point Registers

HUT_DLL_IMPORT bool tf_2271( REGS* regs );             // Control Registers

HUT_DLL_IMPORT bool tf_2272( REGS* regs );             // Access Registers

HUT_DLL_IMPORT bool tf_2276( REGS* regs );             // Floating Point Control Register

HUT_DLL_IMPORT bool tf_2324( REGS* regs,               // Primary Instruction Trace
                             BYTE* inst );

HUT_DLL_IMPORT bool tf_2326( REGS*    regs,            // Instruction Storage
                             TF02326* tf2326,
                             BYTE     opcode1,
                             BYTE     opcode2,
                             int      b1,
                             int      b2 );

HUT_DLL_IMPORT size_t tf_MAX_RECSIZE();                // Return maximum TraceFile record size

HUT_DLL_IMPORT void tf_close( void* notused );         // Close TraceFile
HUT_DLL_IMPORT void tf_close_locked();                 // Close TraceFile
HUT_DLL_IMPORT bool tf_autostop();                     // Automatically stop all tracing

HUT_DLL_IMPORT bool tf_are_swaps_needed( TFSYS* sys );
HUT_DLL_IMPORT void tf_swap_sys( TFSYS* sys );
HUT_DLL_IMPORT void tf_swap_hdr( BYTE sys_ffmt, TFHDR* hdr );
HUT_DLL_IMPORT void tf_swap_rec( TFHDR* hdr, U16 msgnum );

#endif /* __HSCUTL_H__ */
