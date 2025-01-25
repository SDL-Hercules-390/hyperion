/* DBGTRACE.H   (C) Copyright "Fish" (David B. Trout), 2011          */
/*              TRACE/ASSERT/VERIFY debugging macros                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*                                                                   */
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
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*                      PROGRAMMING NOTE                             */
/*                                                                   */
/* This header PURPOSELY DOES NOT prevent itself from being included */
/* multiple times. This is so default handling can be overridden by  */
/* simply #defining 'ENABLE_TRACING_STMTS' and then #including this  */
/* header again to activate the new settings.                        */
/*                                                                   */
/*-------------------------------------------------------------------*/

#undef BREAK_INTO_DEBUGGER

#if defined( _MSVC_ )
  #define BREAK_INTO_DEBUGGER()     do { if (IsDebuggerPresent())         __debugbreak();  } while(0)
#else
  #define BREAK_INTO_DEBUGGER()     do { if (sysblk.is_debugger_present) raise( SIGTRAP ); } while(0)
#endif

/*-------------------------------------------------------------------*/
/*     Implement debugging macros and function -- IF REQUESTED!      */
/*-------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------*/
/*             Implement TRACE, ASSERT and VERIFY macros             */
/*-------------------------------------------------------------------*/

#undef  VERIFY
#undef  ASSERT
#undef  TRACE

// VERIFY conditions are ALWAYS evaluated
// ASSERT conditions MIGHT NOT be evaluated
// TRACE just issues a message if debugging

#if !_ENABLE_TRACING_STMTS_IMPL

// Debugging NOT requested: define "do nothing" macros

  #define VERIFY(a)     ((void)(a)) // (just call the function)
  #define ASSERT(a)                 // (do nothing)
  #define TRACE(...)                // (do nothing)

#else // implement TRACE, ASSERT and VERIFY macros

  #if defined( _MSVC_ )     // Windows...

    #define TRACE(...)                          \
        do                                      \
        {                                       \
            /* Write to Hercules log */         \
            logmsg(__VA_ARGS__);                \
                                                \
            /* And to debugger pane too */      \
            if (IsDebuggerPresent())            \
                DebuggerTrace(__VA_ARGS__);     \
        }                                       \
        while (0)

    #define ASSERT(a)                           \
        do                                      \
        {                                       \
            if (!(a))                           \
            {                                   \
              /* Programming Note: message formatted specifically for Visual Studio F4-key "next message" compatibility */    \
              TRACE("%s(%d) : warning HHC90999W : *** Assertion Failed! *** function: %s\n",__FILE__,__LINE__,__FUNCTION__);  \
              BREAK_INTO_DEBUGGER();            \
            }                                   \
        }                                       \
        while(0)

  #else // Linux...

    #define TRACE(...)                          \
        do                                      \
        {                                       \
            /* Write to Hercules log */         \
            logmsg(__VA_ARGS__);                \
        }                                       \
        while (0)

    #define ASSERT(a)                           \
        do                                      \
        {                                       \
            if (!(a))                           \
            {                                   \
                TRACE("HHC91999W *** Assertion Failed! *** %s(%d)\n",__FILE__,__LINE__); \
                BREAK_INTO_DEBUGGER();          \
            }                                   \
        }                                       \
        while(0)

  #endif // _MSVC_ or Linux

  // Both MSVC *and* Linux...

  #define VERIFY  ASSERT

#endif // !_ENABLE_TRACING_STMTS_IMPL */

/*-------------------------------------------------------------------*/
/*     DebuggerTrace:   format string and write to debugger pane     */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ ) && _ENABLE_TRACING_STMTS_IMPL

  // (static function: only need to implement this once)

  #ifndef _ENABLE_TRACING_STMTS_DEBUGGERTRACE_DEFINED
  #define _ENABLE_TRACING_STMTS_DEBUGGERTRACE_DEFINED

    static inline void DebuggerTrace( char* fmt, ...) 
    {
        const int       chunksize  = 512;
              int       buffsize   = 0;
              char*     buffer     = NULL;
              int       rc         = -1;
              va_list   args;

        va_start( args, fmt );

        do
        {
            if (buffer)
                free( buffer );

            buffsize += chunksize;
            buffer = malloc( buffsize + 1 );

            if (!buffer)
                BREAK_INTO_DEBUGGER();

            rc = _vsnprintf_s( buffer, buffsize+1, buffsize, fmt, args);
        }
        while (rc < 0 || rc >= buffsize);

        /* Write to debugger pane */
        OutputDebugStringA( buffer );

        if (buffer)
            free( buffer );

        va_end( args );
    }

  #endif // _ENABLE_TRACING_STMTS_DEBUGGERTRACE_DEFINED

#endif // defined( _MSVC_ ) && _ENABLE_TRACING_STMTS_IMPL

/*----------------------------------------------------------------------------*/
