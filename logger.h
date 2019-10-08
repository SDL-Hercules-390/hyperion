/* LOGGER.H     (C) Copyright Jan Jaeger, 2003-2012                  */
/*              System logger functions                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __LOGGER_H__
#define __LOGGER_H__

/*-------------------------------------------------------------------*/
/* Define a default minimum/maximum logger pipe buffer size          */
/*-------------------------------------------------------------------*/
#define MIN_LOG_DEFSIZE     (      64 * 1024)       // 64K
#define MAX_LOG_DEFSIZE     (1 * 1024 * 1024)       // 1MB

#if !defined( LOG_DEFSIZE )
  #if defined( SSIZE_MAX) && (SSIZE_MAX < MAX_LOG_DEFSIZE)
    #define  LOG_DEFSIZE      SSIZE_MAX
  #else
    #define  LOG_DEFSIZE      MAX_LOG_DEFSIZE
  #endif
#endif

#if defined( LOG_DEFSIZE )
  #if        LOG_DEFSIZE   <  MIN_LOG_DEFSIZE
    #undef   LOG_DEFSIZE
    #define  LOG_DEFSIZE      MIN_LOG_DEFSIZE
  #endif
  #if        LOG_DEFSIZE   >  MAX_LOG_DEFSIZE
    #undef   LOG_DEFSIZE
    #define  LOG_DEFSIZE      MAX_LOG_DEFSIZE
  #endif
#endif

/*-------------------------------------------------------------------*/
/* log message logging facility                                      */
/*-------------------------------------------------------------------*/

#define LOG_NOBLOCK     0           // log_read() block option
#define LOG_BLOCK       1           // log_read() block option

LOGR_DLL_IMPORT void   logger_init     ();
LOGR_DLL_IMPORT int    log_read        ( char** msg, int* msgidx, int block );
LOGR_DLL_IMPORT int    log_line        ( int linenumber );
LOGR_DLL_IMPORT void   log_sethrdcpy   ( const char* filename );
LOGR_DLL_IMPORT void   log_wakeup      ( void* arg );
LOGR_DLL_IMPORT char*  log_dsphrdcpy   ();
LOGR_DLL_IMPORT int    logger_isactive ();

#define TIMESTAMPLOG   (!sysblk.logoptnotime)
#define DATESTAMPLOG   (!sysblk.logoptnodate)
#define STAMPLOG       (TIMESTAMPLOG || DATESTAMPLOG)

LOGR_DLL_IMPORT void   logger_timestamped_logfile_write( const void* pBuff, size_t nBytes );

#if !defined( _MSVC_ )
LOGR_DLL_IMPORT void   logger_unredirect();
#endif

/*-------------------------------------------------------------------*/

extern int logger_syslogfd[2];      // logger pipe file descriptors

#define LOG_READ           0        // read  end of logger pipe
#define LOG_WRITE          1        // write end of logger pipe

/*-------------------------------------------------------------------*/

#endif /* __LOGGER_H__ */
