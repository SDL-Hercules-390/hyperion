/* HMACROS.H    (C) Copyright Roger Bowler, 1999-2012                */
/*               Hercules macros...                                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

//      This header auto-#included by 'hercules.h'...
//
//      The <config.h> header and other required headers are
//      presumed to have already been #included ahead of it...


#ifndef _HMACROS_H
#define _HMACROS_H

#include "hercules.h"
#include "dbgtrace.h"

/*-------------------------------------------------------------------*/
/*      UNREACHABLE_CODE         code that should NEVER be reached   */
/*-------------------------------------------------------------------*/

#ifdef _MSVC_
  #define UNREACHABLE_CODE(ret)     __assume(0)
#else
  #define UNREACHABLE_CODE(ret)     BREAK_INTO_DEBUGGER(); ret
#endif

/*-------------------------------------------------------------------*/
/*              Define INLINE attributes by compiler                 */
/*-------------------------------------------------------------------*/
#if !defined(INLINE)
  #if defined(__GNUC__)
    #define INLINE          __inline
  #else
    #define INLINE          __inline
  #endif
#endif

/*-------------------------------------------------------------------*/
/*         Round a value 'x' up to the next 'b' boundary             */
/*-------------------------------------------------------------------*/
#define ROUND_UP(x,b)       ((x)?((((x)+((b)-1))/(b))*(b)):(b))

/*-------------------------------------------------------------------*/
/*         Ensure start-end range stays within limits                */
/*-------------------------------------------------------------------*/
#define LIMIT_RANGE( _start, _end, _limit )                           \
    do {                                                              \
        if ((_end) > (_limit) && ((_end) - (_limit) + 1) > (_start))  \
            (_end) = ((_start) + (_limit) - 1);                       \
    } while(0)

/*-------------------------------------------------------------------*/
/*      Define min/max macros                                        */
/*-------------------------------------------------------------------*/
#if !defined(min)
  #define min(_x, _y)       ((_x) < (_y) ? (_x) : (_y))
#endif
#if !defined(max)
  #define max(_x, _y)       ((_x) > (_y) ? (_x) : (_y))
#endif
#if !defined(MIN)
  #define MIN(_x,_y)        min((_x),(_y))
#endif
#if !defined(MAX)
  #define MAX(_x,_y)        max((_x),(_y))
#endif

/*-------------------------------------------------------------------*/
/*      MINMAX        ensures var x remains within range y to z      */
/*-------------------------------------------------------------------*/
#if !defined(MINMAX)
  #define  MINMAX(_x,_y,_z) ((_x) = MIN(MAX((_x),(_y)),(_z)))
#endif

/*-------------------------------------------------------------------*/
/*              some handy array/struct macros...                    */
/*-------------------------------------------------------------------*/
#ifndef   _countof
  #define _countof(x)       ( sizeof(x) / sizeof(x[0]) )
#endif
#ifndef   arraysize
  #define arraysize(x)      _countof(x)
#endif
#ifndef   sizeof_member
  #define sizeof_member(_struct,_member) sizeof(((_struct*)0)->_member)
#endif
#ifndef   offsetof
  #define offsetof(_struct,_member)   (size_t)&(((_struct*)0)->_member)
#endif

#define BIT_ARRAY_BYT( _b )         (        (_b / 8))
#define BIT_ARRAY_MSK( _b )         (0x80 >> (_b % 8))

#define BIT_ARRAY_SET( _a, _b )     (_a[BIT_ARRAY_BYT(_b)] |=  BIT_ARRAY_MSK(_b))
#define BIT_ARRAY_CLR( _a, _b )     (_a[BIT_ARRAY_BYT(_b)] &= ~BIT_ARRAY_MSK(_b))

#define IS_BIT_ARRAY_SET( _a, _b )  (_a[BIT_ARRAY_BYT(_b)] & BIT_ARRAY_MSK(_b))
#define IS_BIT_ARRAY_CLR( _a, _b )  (!IS_BIT_ARRAY_SET(_a,_b))

/*-------------------------------------------------------------------*/
/*      CASSERT macro       a compile time assertion check           */
/*-------------------------------------------------------------------*/
/**
 *  Validates at compile time whether condition is true without
 *  generating code. It can be used at any point in a source file
 *  where typedefs are legal.
 *
 *  On success, compilation proceeds normally.
 *
 *  Sample usage:
 *
 *      CASSERT( sizeof( struct foo ) == 76, demo_c );
 *
 *  On failure, attempts to typedef an array type of negative size
 *  resulting in a compiler error message that might look something
 *  like the following:
 *
 *      demo.c:32: error: size of array 'assertion_failed_demo_c_32' is negative
 *
 *  The offending line itself will look something like this:
 *
 *      typedef assertion_failed_demo_c_32[-1]
 *
 *  where demo_c is the content of the second parameter which should
 *  typically be related in some obvious way to the containing file
 *  name, 32 is the line number in the file on which the assertion
 *  appears, and -1 is the result of a calculation based on the cond
 *  failing.
 *
 *  \param cond         The predicate to test. It should evaluate
 *                      to something which can be coerced into a
 *                      normal C boolean.
 *
 *  \param file         A sequence of legal identifier characters
 *                      that should uniquely identify the source
 *                      file where the CASSERT statement appears.
 */
#if !defined( CASSERT )
#define       CASSERT( cond, file )     _CASSERT_LINE( cond, __LINE__, file )
#define      _CASSERT_PASTE( a, b )     a ## b
#define      _CASSERT_LINE( cond, line, file ) \
typedef char _CASSERT_PASTE( assertion_failed_ ## file, line )[ 2 * !!(cond) - 1 ]
#endif

/*-------------------------------------------------------------------*/
/*      compiler optimization hints         (for performance)        */
/*-------------------------------------------------------------------*/

#undef likely
#undef unlikely

#ifdef _MSVC_

  #define likely(_c)      ( (_c) ? ( __assume((_c)), 1 ) :                    0   )
  #define unlikely(_c)    ( (_c) ?                   1   : ( __assume(!(_c)), 0 ) )

#else // !_MSVC_

  #if __GNUC__ >= 3
    #define likely(_c)    __builtin_expect((_c),1)
    #define unlikely(_c)  __builtin_expect((_c),0)
  #else
    #define likely(_c)    (_c)
    #define unlikely(_c)  (_c)
  #endif

#endif // _MSVC_

/*-------------------------------------------------------------------*/
/*      _MSVC_  portability macros                                   */
/*-------------------------------------------------------------------*/

/* PROGRAMMING NOTE: the following 'tape' portability macros are
   only for physical (SCSI) tape devices, not emulated aws files */

#ifdef _MSVC_
  #define  open_tape            w32_open_tape
  #define  read_tape            w32_read_tape
  #define  write_tape           w32_write_tape
  #define  ioctl_tape           w32_ioctl_tape
  #define  close_tape           w32_close_tape
#else
  #define  open_tape            open
  #define  read_tape            read
  #define  write_tape           write
  #define  ioctl_tape           ioctl
  #define  close_tape           close
#endif

#ifdef _MSVC_
  #define  create_pipe(a)       socketpair(AF_INET,SOCK_STREAM,IPPROTO_IP,a)
  #define  read_pipe(f,b,n)     recv(f,b,n,0)
  #define  write_pipe(f,b,n)    send(f,b,(int)n,0)
  #define  close_pipe(f)        closesocket(f)
#else
  #define  create_pipe(f)       pipe(f)
  #define  read_pipe(f,b,n)     read(f,b,n)
  #define  write_pipe(f,b,n)    write(f,b,n)
  #define  close_pipe(f)        close(f)
#endif

#ifdef _MSVC_
  #define  socket               w32_socket
  #define  accept               w32_accept
/* Now defined in hsocket.h
  int read_socket(int fd, char *ptr, int nbytes);
  int write_socket(int fd, const char *ptr, int nbytes);
*/
  #define  close_socket         w32_close_socket
  #define  hif_nametoindex      w32_if_nametoindex
  #define  hinet_ntop           w32_inet_ntop
  #define  hinet_pton           w32_inet_pton
#else
/* Now defined in hsocket.h
  int read_socket(int fd, char *ptr, int nbytes);
  int write_socket(int fd, const char *ptr, int nbytes);
*/
  #define  close_socket(f)      close(f)
  #define  hif_nametoindex      if_nametoindex
  #define  hinet_ntop           inet_ntop
  #define  hinet_pton           inet_pton
#endif

#ifdef _MSVC_
  #undef   FD_SET
  #undef   FD_ISSET
  #define  FD_SET               w32_FD_SET
  #define  FD_ISSET             w32_FD_ISSET
  #define  select(n,r,w,e,t)    w32_select((n),(r),(w),(e),(t),__FILE__,__LINE__)
  #define  pselect(n,r,w,e,t,m) w32_pselect((n),(r),(w),(e),(t),(m),__FILE__,__LINE__)
  #define  fdopen               w32_fdopen
  #define  fwrite               w32_fwrite
  #define  fprintf              w32_fprintf
  #define  fclose               w32_fclose
  #define  basename             w32_basename
  #define  dirname              w32_dirname
#ifndef strcasestr
  #define  strcasestr           w32_strcasestr
#endif
#endif

#ifdef _MSVC_
  #define  fdatasync            _commit
  #define  atoll                _atoi64
#else /* !_MSVC_ */
  #if !defined(HAVE_FDATASYNC_SUPPORTED)
    #ifdef HAVE_FSYNC
      #define  fdatasync        fsync
    #else
      #error Required 'fdatasync' function is missing and alternate 'fsync' function also missing
    #endif
  #endif
  #define  atoll(s)             strtoll(s,NULL,0)
#endif

/*-------------------------------------------------------------------*/
/* Portable macro for copying 'va_list' variable arguments variable  */
/*-------------------------------------------------------------------*/

// ZZ FIXME: this should probably be handled in configure.ac...

#if !defined( va_copy )
  #if defined( __va_copy )
    #define  va_copy            __va_copy
  #elif defined( _MSVC_ )
    #define  va_copy(to,from)   (to) = (from)
  #else
    #define  va_copy(to,from)   memcpy((to),(from),sizeof(va_list))
  #endif
#endif

/*-------------------------------------------------------------------*/
/* Handle newer RUSAGE_THREAD definition for older Linux levels      */
/* before 2.6.4 along with *nix systems not supporting...            */
/*-------------------------------------------------------------------*/

// ZZ FIXME: this should probably be handled in configure.ac...

#if !defined(RUSAGE_THREAD) && !defined(_MSVC_)
  #define   RUSAGE_THREAD       thread_id()
#endif

/*-------------------------------------------------------------------*/
/*      Some handy memory/string comparison macros                   */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
#define strcasecmp                  _stricmp
#define strncasecmp                 _strnicmp
#endif

#define mem_eq(_a,_b,_n)            (!memcmp(_a,_b,_n))
#define mem_ne(_a,_b,_n)            ( memcmp(_a,_b,_n))

// PROGRAMMING NOTE: see also header "extstring.h"

#define str_eq(_a,_b)               (!strcmp(_a,_b))
#define str_ne(_a,_b)               ( strcmp(_a,_b))

#define str_eq_n(_a,_b,_n)          (!strncmp(_a,_b,_n))
#define str_ne_n(_a,_b,_n)          ( strncmp(_a,_b,_n))

#define str_caseless_eq(_a,_b)      (!strcasecmp(_a,_b))
#define str_caseless_ne(_a,_b)      ( strcasecmp(_a,_b))

#define str_caseless_eq_n(_a,_b,_n) (!strncasecmp(_a,_b,_n))
#define str_caseless_ne_n(_a,_b,_n) ( strncasecmp(_a,_b,_n))

#define UPPER_ARGV_0( _argv ) do {if (_argv && _argv[0]) strupper( _argv[0], _argv[0] );} while (0)

/*-------------------------------------------------------------------*/
/*      Large File Support portability                               */
/*-------------------------------------------------------------------*/

#ifdef _MSVC_
  /* "Native" 64-bit Large File Support */
  #define  off_t                __int64
  #define  ftruncate            _chsize_s
  #define  ftell                _ftelli64
  #define  fseek                _fseeki64
  #define  lseek                _lseeki64
  #define  fstat                _fstati64
  #define  stat                 _stati64
#elif defined(_LFS_LARGEFILE) || ( defined(SIZEOF_OFF_T) && SIZEOF_OFF_T > 4 )
  /* Native 64-bit Large File Support */
  #if defined(HAVE_FSEEKO)
    #define  ftell              ftello
    #define  fseek              fseeko
  #else
    #if defined(SIZEOF_LONG) && SIZEOF_LONG <= 4
      WARNING( "fseek/ftell use offset arguments of insufficient size" )
    #endif
  #endif
#elif defined(_LFS64_LARGEFILE)
  /* Transitional 64-bit Large File Support */
  #define    off_t              off64_t
  #define    ftruncate          ftruncate64
  #define    ftell              ftello64
  #define    fseek              fseeko64
  #define    lseek              lseek64
  #define    fstat              fstat64
  #define    stat               stat64
#else // !defined(_LFS_LARGEFILE) && !defined(_LFS64_LARGEFILE) && (!defined(SIZEOF_OFF_T) || SIZEOF_OFF_T <= 4)
  /* No 64-bit Large File Support at all */
  WARNING( "Large File Support missing" )
#endif
// Hercules low-level file open...
// PROGRAMMING NOTE: the "##" preceding "__VA_ARGS__" is required for compat-
//                   ibility with gcc/MSVC compilers and must not be removed
#ifdef _MSVC_
  #define   HOPEN(_p,_o,...)    w32_hopen ((_p),(_o), ## __VA_ARGS__)
#else
  #define   HOPEN(_p,_o,...)    hopen     ((_p),(_o), ## __VA_ARGS__)
#endif

#define MAX_OFFSET_T            ((off_t)(ULLONG_MAX))

/*-------------------------------------------------------------------*/
/*      Macro for command parsing with variable length               */
/*-------------------------------------------------------------------*/
#define  CMD(str,cmd,min) ( strcaseabbrev(#cmd,str,min) )

#define  NCMP(_lvar,_rvar,_svar) ( !memcmp( _lvar, _rvar, _svar ) )
#define  SNCMP(_lvar,_rvar,_svar) ( !strncasecmp( _lvar, _rvar, _svar ) )

/*-------------------------------------------------------------------*/
/*      Debugging / Tracing macros.                                  */
/*-------------------------------------------------------------------*/
#define MLVL( _lvl) \
    (sysblk.msglvl & (MLVL_ ## _lvl))

/* Opcode routing table function pointer */
typedef void (ATTR_REGPARM(2)*FUNC)();

/* Program Interrupt function pointer */
typedef void (ATTR_REGPARM(2) *pi_func) (REGS *regs, int pcode);

/* trace_br function */
typedef U32  (*s390_trace_br_func) (int amode,  U32 ia, REGS *regs);
typedef U64  (*z900_trace_br_func) (int amode,  U64 ia, REGS *regs);

/* qsort comparison function typedef */
typedef int CMPFUNC(const void*, const void*);

/*-------------------------------------------------------------------*/
/*      CPU state related macros and constants                       */
/*-------------------------------------------------------------------*/

/* Definitions for CPU state */
#define CPUSTATE_STARTED        1       /* CPU is started            */
#define CPUSTATE_STOPPING       2       /* CPU is stopping           */
#define CPUSTATE_STOPPED        3       /* CPU is stopped            */

#define IS_CPU_ONLINE(_cpu) \
  (sysblk.regs[(_cpu)] != NULL)

#define HOST(  _regs )  (_regs)->hostregs
#define GUEST( _regs )  (_regs)->guestregs

#define HOSTREGS        HOST(  regs )   // 'regs' presumed
#define GUESTREGS       GUEST( regs )   // 'regs' presumed

/* Instruction count for a CPU */
#define INSTCOUNT(_regs) \
 (HOST(_regs)->prevcount + HOST(_regs)->instcount)

/*-------------------------------------------------------------------*/
/*                  Obtain/Release mainlock                          */
/*-------------------------------------------------------------------*/
/*  mainlock is only obtained by a CPU thread. PROGRAMMING NOTE:     */
/*  The below #defines for OBTAIN_MAINLOCK and RELEASE_MAINLOCK      */
/*  MIGHT be overridden/nullified by machdep.h if atomic assists     */
/*  are available, since normally, that is the only reason for       */
/*  needing to obtain mainlock in the first place: because you       */
/*  need to do something atomically (e.g. need to do cmpxchg).       */
/*-------------------------------------------------------------------*/

#define OBTAIN_MAINLOCK_UNCONDITIONAL(_regs) \
 do { \
  if (HOST(_regs)->cpubit != (_regs)->sysblk->started_mask) { \
   obtain_lock(&(_regs)->sysblk->mainlock); \
   (_regs)->sysblk->mainowner = HOST(_regs)->cpuad; \
  } \
 } while (0)

#define RELEASE_MAINLOCK_UNCONDITIONAL(_regs) \
 do { \
   if ((_regs)->sysblk->mainowner == HOST(_regs)->cpuad) { \
     (_regs)->sysblk->mainowner = LOCK_OWNER_NONE; \
     release_lock(&(_regs)->sysblk->mainlock); \
   } \
 } while (0)

#define  OBTAIN_MAINLOCK(_regs)  OBTAIN_MAINLOCK_UNCONDITIONAL((_regs))
#define RELEASE_MAINLOCK(_regs) RELEASE_MAINLOCK_UNCONDITIONAL((_regs))

/*-------------------------------------------------------------------*/
/*      Obtain/Release crwlock                                       */
/*      crwlock can be obtained by any thread                        */
/*-------------------------------------------------------------------*/

#define OBTAIN_CRWLOCK()    obtain_lock( &sysblk.crwlock )
#define RELEASE_CRWLOCK()   release_lock( &sysblk.crwlock )

/*-------------------------------------------------------------------*/
/*      Obtain/Release lock helper macros                            */
/*      used mostly by channel.c                                     */
/*-------------------------------------------------------------------*/

#define OBTAIN_IOQLOCK()          obtain_lock(  &sysblk.ioqlock )
#define RELEASE_IOQLOCK()         release_lock( &sysblk.ioqlock )

#define OBTAIN_IOINTQLK()         obtain_lock(  &sysblk.iointqlk )
#define RELEASE_IOINTQLK()        release_lock( &sysblk.iointqlk )

#define OBTAIN_DEVLOCK( dev )     obtain_lock(  &dev->lock )
#define RELEASE_DEVLOCK( dev )    release_lock( &dev->lock )

/*-------------------------------------------------------------------*/
/* Return whether specified CPU is waiting to acquire intlock or not */
/*-------------------------------------------------------------------*/
#define AT_SYNCPOINT(_regs) (HOST(_regs)->intwait)

/*-------------------------------------------------------------------*/
/*      Macro to check if DEVBLK is for an existing device           */
/*-------------------------------------------------------------------*/

#if defined(_FEATURE_INTEGRATED_3270_CONSOLE)
  #define IS_DEV(_dev) \
    ((_dev)->allocated && (((_dev)->pmcw.flag5 & PMCW5_V) || (_dev) == sysblk.sysgdev))
#else // !defined(_FEATURE_INTEGRATED_3270_CONSOLE)
  #define IS_DEV(_dev) \
    ((_dev)->allocated && ((_dev)->pmcw.flag5 & PMCW5_V))
#endif // defined(_FEATURE_INTEGRATED_3270_CONSOLE)

#define INTEGRATED_CONS_FD              INT_MAX

#define IS_INTEGRATED_CONS( _dev )          \
    (1                                      \
     && !(_dev)->console                    \
     && (_dev)->connected                   \
     && (_dev)->fd == INTEGRATED_CONS_FD    \
     && strlen( (_dev)->filename ) > 0      \
    )

/*-------------------------------------------------------------------*/
/*      HDL macro to call optional function override                 */
/*-------------------------------------------------------------------*/

#define HDC1(_func, _arg1) \
  ((_func) ? (_func) ((_arg1)) : (NULL))
#define HDC2(_func, _arg1,_arg2) \
  ((_func) ? (_func) ((_arg1),(_arg2)) : (NULL))
#define HDC3(_func, _arg1,_arg2,_arg3) \
  ((_func) ? (_func) ((_arg1),(_arg2),(_arg3)) : (NULL))
#define HDC4(_func, _arg1,_arg2,_arg3,_arg4) \
  ((_func) ? (_func) ((_arg1),(_arg2),(_arg3),(_arg4)) : (NULL))
#define HDC5(_func, _arg1,_arg2,_arg3,_arg4,_arg5) \
  ((_func) ? (_func) ((_arg1),(_arg2),(_arg3),(_arg4),(_arg5)) : (NULL))
#define HDC6(_func, _arg1,_arg2,_arg3,_arg4,_arg5,_arg6) \
  ((_func) ? (_func) ((_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6)) : (NULL))

/*-------------------------------------------------------------------*/
/*      Sleep for as long as we like   (whole number of seconds)     */
/*-------------------------------------------------------------------*/

#define SLEEP(_n) \
 do { \
   unsigned int rc = (_n); \
   while (rc) \
     if ((rc = sleep (rc))) \
       sched_yield(); \
 } while (0)

/*-------------------------------------------------------------------*/
/*      CRASH                       (with hopefully a dump)          */
/*-------------------------------------------------------------------*/
#define CRASH()         do { BYTE* p=NULL; *p=0; } while (0)

/*-------------------------------------------------------------------*/
/*      Perform standard utility initialization                      */
/*-------------------------------------------------------------------*/

#define INITIALIZE_UTILITY( def, desc, pgm )  \
    argc = initialize_utility( argc, argv, def, desc, pgm )

/*-------------------------------------------------------------------*/
/*      Assign name to thread           (debugging aid)              */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
  #define  SET_THREAD_NAME_ID(t,n)          \
    do                                      \
    {                                       \
      w32_set_thread_name((t),(n));         \
      set_thread_name((t),(n));             \
    }                                       \
    while (0)
  #define  SET_THREAD_NAME(n)       SET_THREAD_NAME_ID(GetCurrentThreadId(),(n))
#elif defined( HAVE_PTHREAD_SETNAME_NP )
  #define  SET_THREAD_NAME_ID(t,n)          \
    do                                      \
    {                                       \
      nix_set_thread_name((t),(n));         \
      set_thread_name((t),(n));             \
    }                                       \
    while (0)
  #define  SET_THREAD_NAME(n)       SET_THREAD_NAME_ID(pthread_self(),(n))
#else
  #define  SET_THREAD_NAME_ID(t,n)          \
    do                                      \
    {                                       \
      set_thread_name((t),(n));             \
    }                                       \
    while (0)
  #define  SET_THREAD_NAME(n)       SET_THREAD_NAME_ID(pthread_self(),(n))
#endif

/*-------------------------------------------------------------------*/
/*            ROOT privilege and capabilities macros                 */
/*-------------------------------------------------------------------*/
#if defined( NO_SETUID )

  #define DROP_PRIVILEGES( _capa )      // (do nothing)
  #define DROP_ALL_CAPS()               // (do nothing)
  #define SETMODE( _func )              // (do nothing)

#else // !defined( NO_SETUID )

  #if defined( HAVE_SYS_CAPABILITY_H ) && defined( HAVE_SYS_PRCTL_H ) && defined( OPTION_CAPABILITIES )

    #define DROP_PRIVILEGES(_capa)      drop_privileges(_capa)
    #define DROP_ALL_CAPS()             drop_all_caps()
    #define SETMODE(_x)                 // (do nothing)

  #else // !defined( HAVE_SYS_CAPABILITY_H ) || !defined( HAVE_SYS_PRCTL_H ) || !defined( OPTION_CAPABILITIES )

    /* SETMODE( INIT )
     *
     *   Sets the saved uid to the effective uid, and sets the
     *   effective uid to the real uid, such that the program
     *   is running with normal user attributes, other than it
     *   may switch to the saved uid via SETMODE( ROOT ). This
     *   call is usually made upon entry to the setuid program.
     *
     * SETMODE( ROOT )
     *
     *   Sets the saved uid to the real uid, and sets the real and
     *   effective uid to the saved uid. A setuid root program will
     *   enter 'root mode' and will have all the appropriate access.
     *
     * SETMODE( USER )
     *
     *   Sets the real and effective uid to the uid of the caller.
     *   The saved uid will be the effective uid that was active
     *   upon entry to the program (before SETMODE( INIT )).
     *
     * SETMODE( TERM )
     *
     *   Sets real, effective and saved uid to the real uid upon
     *   entry to the program.  This call will revoke any setuid
     *   access that the thread/process has.  It is important to
     *   issue this call BEFORE an exec to a shell or other program
     *   that could introduce integrity exposures when running with
     *   root access.
     */

    #define DROP_PRIVILEGES(_capa)      // (do nothing)
    #define DROP_ALL_CAPS()             // (do nothing)
    #define SETMODE(_func)              _SETMODE_ ## _func

    #if defined( HAVE_SETRESUID )

      #define _SETMODE_INIT                                                         \
                                                                                    \
          do                                                                        \
          {                                                                         \
              VERIFY( getresuid( &sysblk.ruid, &sysblk.euid, &sysblk.suid ) == 0 ); \
              VERIFY( getresgid( &sysblk.rgid, &sysblk.egid, &sysblk.sgid ) == 0 ); \
              VERIFY( setresgid(  sysblk.rgid,  sysblk.rgid,  sysblk.egid ) == 0 ); \
              VERIFY( setresuid(  sysblk.ruid,  sysblk.ruid,  sysblk.euid ) == 0 ); \
          }                                                                         \
          while(0)


      #define _SETMODE_ROOT                                                         \
                                                                                    \
          do                                                                        \
          {                                                                         \
              VERIFY(!setresuid(sysblk.suid,sysblk.suid,sysblk.ruid));              \
          }                                                                         \
          while(0)


      #define _SETMODE_USER                                                         \
                                                                                    \
          do                                                                        \
          {                                                                         \
              VERIFY( setresuid( sysblk.ruid, sysblk.ruid, sysblk.suid ) == 0 );    \
          }                                                                         \
          while(0)


      #define _SETMODE_TERM                                                         \
                                                                                    \
          do                                                                        \
          {                                                                         \
              VERIFY( setresgid( sysblk.rgid, sysblk.rgid, sysblk.rgid ) == 0 );    \
              VERIFY( setresuid( sysblk.ruid, sysblk.ruid, sysblk.ruid ) == 0 );    \
          }                                                                         \
          while(0)

    #elif defined( HAVE_SETREUID )

      #define _SETMODE_INIT                                         \
                                                                    \
          do                                                        \
          {                                                         \
              sysblk.ruid = getuid();                               \
              sysblk.euid = geteuid();                              \
              sysblk.rgid = getgid();                               \
              sysblk.egid = getegid();                              \
                                                                    \
              VERIFY( setregid( sysblk.egid, sysblk.rgid ) == 0 );  \
              VERIFY( setreuid( sysblk.euid, sysblk.ruid ) == 0 );  \
          }                                                         \
          while(0)


      #define _SETMODE_ROOT                                         \
                                                                    \
          do                                                        \
          {                                                         \
              VERIFY( setregid( sysblk.rgid, sysblk.egid ) == 0 );  \
              VERIFY( setreuid( sysblk.ruid, sysblk.euid ) == 0 );  \
          }                                                         \
          while(0)


      #define _SETMODE_USER                                         \
                                                                    \
          do                                                        \
          {                                                         \
              VERIFY( setregid( sysblk.egid, sysblk.rgid ) == 0 );  \
              VERIFY( setreuid( sysblk.euid, sysblk.ruid ) == 0 );  \
          }                                                         \
          while(0)


      #define _SETMODE_TERM                                         \
                                                                    \
          do                                                        \
          {                                                         \
              VERIFY( setgid( sysblk.rgid ) == 0 );                 \
              VERIFY( setuid( sysblk.ruid ) == 0 );                 \
          }                                                         \
          while(0)

    #else // !defined( HAVE_SETRESUID ) && !defined( HAVE_SETEREUID )

      #error Cannot figure out how to swap effective UID/GID! Maybe you should #define NO_SETUID?

    #endif // defined( HAVE_SETREUID ) || defined( HAVE_SETRESUID )

  #endif // defined( HAVE_SYS_CAPABILITY_H ) && defined( HAVE_SYS_PRCTL_H ) && defined( OPTION_CAPABILITIES )

#endif // defined( NO_SETUID )

/*-------------------------------------------------------------------*/
/*      Pipe signaling            (thread signaling via pipe)        */
/*-------------------------------------------------------------------*/

#define RECV_PIPE_SIGNAL( rfd, lock, flag )                         \
                                                                    \
    do                                                              \
    {                                                               \
        int f, saved_errno; BYTE c=0;                               \
                                                                    \
        saved_errno = get_HSO_errno();                              \
        {                                                           \
            obtain_lock( &(lock) );                                 \
            {                                                       \
                if ((f = (flag)) >= 1)                              \
                         (flag)   = 0;                              \
            }                                                       \
            release_lock( &(lock) );                                \
                                                                    \
            if (f >= 1)                                             \
                VERIFY( read_pipe( (rfd), &c, 1 ) == 1);            \
        }                                                           \
        set_HSO_errno( saved_errno );                               \
    }                                                               \
    while(0)


#define SEND_PIPE_SIGNAL( wfd, lock, flag )                         \
                                                                    \
    do                                                              \
    {                                                               \
        int f, saved_errno; BYTE c=0;                               \
                                                                    \
        saved_errno = get_HSO_errno();                              \
        {                                                           \
            obtain_lock( &(lock) );                                 \
            {                                                       \
                if ((f = (flag)) <= 0)                              \
                         (flag)   = 1;                              \
            }                                                       \
            release_lock( &(lock) );                                \
                                                                    \
            if (f <= 0)                                             \
                VERIFY( write_pipe( (wfd), &c, 1 ) == 1);           \
        }                                                           \
        set_HSO_errno( saved_errno );                               \
    }                                                               \
    while(0)


#define SUPPORT_WAKEUP_SELECT_VIA_PIPE( pipe_rfd, maxfd, prset )    \
                                                                    \
    do                                                              \
    {                                                               \
        FD_SET( (pipe_rfd), (prset) );                              \
                                                                    \
        (maxfd) = (maxfd) > (pipe_rfd) ? (maxfd)                    \
                                       : (pipe_rfd);                \
    }                                                               \
    while(0)


#define SUPPORT_WAKEUP_CONSOLE_SELECT_VIA_PIPE( maxfd, prset )  SUPPORT_WAKEUP_SELECT_VIA_PIPE( sysblk.cnslrpipe, (maxfd), (prset) )
#define SUPPORT_WAKEUP_SOCKDEV_SELECT_VIA_PIPE( maxfd, prset )  SUPPORT_WAKEUP_SELECT_VIA_PIPE( sysblk.sockrpipe, (maxfd), (prset) )

#define RECV_CONSOLE_THREAD_PIPE_SIGNAL()  RECV_PIPE_SIGNAL( sysblk.cnslrpipe, sysblk.cnslpipe_lock, sysblk.cnslpipe_flag )
#define RECV_SOCKDEV_THREAD_PIPE_SIGNAL()  RECV_PIPE_SIGNAL( sysblk.sockrpipe, sysblk.sockpipe_lock, sysblk.sockpipe_flag )
#define SIGNAL_CONSOLE_THREAD()            SEND_PIPE_SIGNAL( sysblk.cnslwpipe, sysblk.cnslpipe_lock, sysblk.cnslpipe_flag )
#define SIGNAL_SOCKDEV_THREAD()            SEND_PIPE_SIGNAL( sysblk.sockwpipe, sysblk.sockpipe_lock, sysblk.sockpipe_flag )

/*********************************************************************/
/*               Define compiler error bypasses                      */
/*********************************************************************/

#ifdef _MSVC_

    /*
     *  MS VC Bug ID 363375 Bypass
     *  Note: This error, or similar, also occurs w/VS2010 and VS2012.
     *  TODO: Verify if fixed in VS2013 or later.
     */

    #define ENABLE_VS_BUG_ID_363375_BYPASS          \
                                                    \
        __pragma( optimize( "", off ))              \
        __pragma( optimize( "t", on ))


    #define DISABLE_VS_BUG_ID_363375_BYPASS         \
                                                    \
        __pragma( optimize( "", on ))

#else // Linux

    #define ENABLE_VS_BUG_ID_363375_BYPASS      /* (nothing) */
    #define DISABLE_VS_BUG_ID_363375_BYPASS     /* (nothing) */

#endif // _MSVC_

#endif // _HMACROS_H
