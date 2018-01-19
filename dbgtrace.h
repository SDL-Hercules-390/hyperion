/* DBGTRACE.H   (C) Copyright "Fish" (David B. Trout), 2011          */
/*              TRACE/ASSERT/VERIFY debugging macros                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This header implements the TRACE, ASSERT and VERIFY debugging     */
/* macros. Use TRACE statements during and after developing your     */
/* code to display helpful debugging messages. Use ASSERT to test    */
/* for a condition which should always be true. Use VERIFY whenever  */
/* the condition must always be evaluated even for non-debug builds  */
/* such as when the VERIFY conditional itself uses a function call   */
/* which must always be called.                                      */
/*                                                                   */
/* You should #define ENABLE_TRACING_STMTS to either 0 or 1 before   */
/* #including this header. If left undefined then the _DEBUG switch  */
/* controls if they're enabled or not (default is yes for _DEBUG).   */
/* This allows you to enable/disable TRACING separate from _DEBUG    */
/* on an individual module by module basis if needed or desired.     */
/*-------------------------------------------------------------------*/

/* PROGRAMMING NOTE: this header purposely does not prevent itself   */
/* from being #included multiple times. This is so default handling  */
/* can be overridden by simply #defining 'ENABLE_TRACING_STMTS' and  */
/* then #including this header again to activate the new settings.   */

#undef BREAK_INTO_DEBUGGER

#if defined( _MSVC_ )
  #define BREAK_INTO_DEBUGGER()     __debugbreak()
#else
  #define BREAK_INTO_DEBUGGER()     raise( SIGTRAP )
#endif

#undef _ENABLE_TRACING_STMTS_IMPL

#if defined( ENABLE_TRACING_STMTS )
  #if        ENABLE_TRACING_STMTS
    #define _ENABLE_TRACING_STMTS_IMPL     1
  #else
    #define _ENABLE_TRACING_STMTS_IMPL     0
  #endif
#else
  #if defined(DEBUG) || defined(_DEBUG)
    #define _ENABLE_TRACING_STMTS_IMPL     1
  #else
    #define _ENABLE_TRACING_STMTS_IMPL     0
  #endif
#endif

#undef  VERIFY
#undef  ASSERT
#undef  TRACE

/* VERIFY conditions are always evaluated */
/* ASSERT conditions may not be evaluated */

#if !_ENABLE_TRACING_STMTS_IMPL
  #define VERIFY(a)       ((void)(a))
  #define ASSERT(a)
  #define TRACE(...)
#else /* _ENABLE_TRACING_STMTS_IMPL */
  #if defined( _MSVC_ )
    #define TRACE(...) do { \
        /* Write to both places */  \
        logmsg(__VA_ARGS__); \
        if (IsDebuggerPresent())    \
          DebuggerTrace (__VA_ARGS__); \
      } while (0)
    #define ASSERT(a) do { \
        if (!(a)) { \
          /* Programming Note: message formatted specifically for Visual Studio F4-key "next message" compatibility */  \
          TRACE("%s(%d) : warning HHC90999W : *** Assertion Failed! *** function: %s\n",__FILE__,__LINE__,__FUNCTION__); \
          if (IsDebuggerPresent()) BREAK_INTO_DEBUGGER(); \
        } \
      } while(0)
  #else /* !defined( _MSVC_ ) */
    #define TRACE logmsg
    #define ASSERT(a) do { \
        if (!(a)) { \
          TRACE("HHC91999W *** Assertion Failed! *** %s(%d)\n",__FILE__,__LINE__); \
        } \
      } while(0)
  #endif // _MSVC_
  #define VERIFY  ASSERT
#endif /* !_ENABLE_TRACING_STMTS_IMPL */

#if defined( _MSVC_ ) && _ENABLE_TRACING_STMTS_IMPL
  #ifndef _ENABLE_TRACING_STMTS_DEBUGGERTRACE_DEFINED
  #define _ENABLE_TRACING_STMTS_DEBUGGERTRACE_DEFINED
  /* Windows: also send to debugger window */
  static inline void DebuggerTrace(char* fmt, ...) {
      const int chunksize = 512;
      int buffsize = 0;
      char* buffer = NULL;
      int rc = -1;
      va_list args;
      va_start( args, fmt );
      do {
          if (buffer) free( buffer );
          buffsize += chunksize;
          buffer = malloc( buffsize + 1 );
          if (!buffer) BREAK_INTO_DEBUGGER();
          rc = _vsnprintf_s( buffer, buffsize+1, buffsize, fmt, args);
      } while (rc < 0 || rc >= buffsize);
      OutputDebugStringA( buffer ); /* send to debugger pane */
      if (buffer) free( buffer );
      va_end( args );
  }
  #endif /* _ENABLE_TRACING_STMTS_DEBUGGERTRACE_DEFINED */
#endif /* defined( _MSVC_ ) && _ENABLE_TRACING_STMTS_IMPL */
