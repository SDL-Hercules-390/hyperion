/* HREXX.H      (c)Copyright Enrico Sorichetti, 2012                          */
/*              Rexx Interpreter Support                                      */
/*                                                                            */
/*  Released under "The Q Public License Version 1"                           */
/*  (http://www.hercules-390.org/herclic.html) as modifications to            */
/*  Hercules.                                                                 */

/*  inspired by the previous Rexx implementation by Jan Jaeger                */

#ifndef _HREXX_H_
#define _HREXX_H_

#define HAVKEYW( _T_ )       (  strcasecmp( _T_, argv[iarg]) == 0 )
#define HAVABBR( _T_ )       ( strncasecmp( _T_, argv[iarg], argl) == 0 )

// #define  CMPARG( _VALUE_ )  strcasecmp( _VALUE_, argv[iarg])
// #define CMPARGL( _VALUE_ ) strncasecmp( _VALUE_, argv[iarg], argl)


#if defined(PATH_MAX)
# define MAX_PATHNAME_LENGTH PATH_MAX + 1
#elif defined(_POSIX_PATH_MAX)
# define MAX_PATHNAME_LENGTH _POSIX_PATH_MAX + 1
#else
# define MAX_PATHNAME_LENGTH 1024 + 1
#endif

#if defined(FILENAME_MAX)
# define MAX_FILENAME_LENGTH FILENAME_MAX + 1
#elif defined(_MAX_FNAME)
# define MAX_FILENAME_LENGTH _MAX_FNAME + 1
#elif defined(_POSIX_NAME_MAX)
# define MAX_FILENAME_LENGTH _POSIX_NAME_MAX + 1
#else
# define MAX_FILENAME_LENGTH 1024 + 1
#endif

#ifndef MAX_ARGS_TO_REXXSTART
#define MAX_ARGS_TO_REXXSTART   64
#endif

#ifndef MAX_ARGS_TO_SUBCOMHANDLER
#define MAX_ARGS_TO_SUBCOMHANDLER   64
#endif

#ifndef WRK_AREA_SIZE
#define WRK_AREA_SIZE 33
#endif

#ifdef  _HREXX_C_
#define _HREXX_EXTERN
#else
#define _HREXX_EXTERN extern
#endif

#define REGINA_PACKAGE "Regina"
#define OOREXX_PACKAGE "ooRexx"

#define hSubcom  "HERCULES"
#define hSIOExit "HERCSIOE"

#if     defined( _MSVC_ )
#define EXTNDELIM             ";"
#define EXTENSIONS            ".REXX;.rexx;.REX;.rex;.CMD;.cmd;.RX;.rx"
#define PATHDELIM             ";"
#define PATHFORMAT            "%s\\%s%s"
#define REGINA_LIBRARY        "regina.dll"
#define OOREXX_LIBRARY        "rexx.dll"
#define OOREXX_API_LIBRARY    "rexxapi.dll"

#elif   defined ( __APPLE__ )
#define EXTNDELIM             ":"
#define EXTENSIONS            ".rexx:.rex:.cmd:.rx" /* APPLE is caseless */
#define PATHDELIM             ":"
#define PATHFORMAT            "%s/%s%s"
#define REGINA_LIBRARY        "libregina.dylib"
#define OOREXX_LIBRARY        "librexx.dylib"
#define OOREXX_API_LIBRARY    "librexxapi.dylib"

#else
#define EXTNDELIM             ":"
#define EXTENSIONS            ".REXX:.rexx:.REX:.rex:.CMD:.cmd:.RX:.rx"
#define PATHDELIM             ":"
#define PATHFORMAT            "%s/%s%s"
#define REGINA_LIBRARY        "libregina.so"
#define OOREXX_LIBRARY        "librexx.so"
#define OOREXX_API_LIBRARY    "librexxapi.so"
#endif

#define REXX_START                  "RexxStart"
#define REXX_REGISTER_FUNCTION      "RexxRegisterFunctionExe"
#define REXX_DEREGISTER_FUNCTION    "RexxDeregisterFunction"
#define REXX_REGISTER_SUBCOM        "RexxRegisterSubcomExe"
#define REXX_DEREGISTER_SUBCOM      "RexxDeregisterSubcom"
#define REXX_REGISTER_EXIT          "RexxRegisterExitExe"
#define REXX_DEREGISTER_EXIT        "RexxDeregisterExit"
#define REXX_ALLOCATE_MEMORY        "RexxAllocateMemory"
#define REXX_FREE_MEMORY            "RexxFreeMemory"
#define REXX_VARIABLE_POOL          "RexxVariablePool"

#define HREXX_ERRORHANDLER_VARNAME            "HREXX.ERRORHANDLER"
#define HREXX_PERSISTENTRESPSTEMNAME_VARNAME  "HREXX.PERSISTENTRESPSTEMNAME"
#define HREXX_RESPSTEMNAME_VARNAME            "HREXX.RESPSTEMNAME"

#if defined ( _MSVC_ )

#define HDLOPEN( _LIBHND_, _LIBNAM_ , _LIBPAR_ ) do {\
    _LIBHND_  = LoadLibrary( ( _LIBNAM_ ) );\
    if (! ( _LIBHND_ )  ) { \
        WRMSG( HHC17531, "E", RexxPackage, ( _LIBNAM_ ) ) ; \
        return -1; \
    } \
} while (0)

#define HDLCLOSE( _LIBHND_) do {\
    if ( FreeLibrary( ( _LIBHND_ ) ) == 0 ) { \
        WRMSG( HHC17532, "E", RexxPackage, ( _LIBHND_ ) ) ; \
        return -1; \
    } \
    ( _LIBHND_ ) = NULL; \
} while (0)

#define HDLSYM(_SYMHND_, _LIBHND_, _SYMNAM_ ) do {\
    ( _SYMHND_ ) = (void *) GetProcAddress( ( _LIBHND_ ), ( _SYMNAM_ ) ) ; \
    if (! ( _SYMHND_ )  ) { \
        WRMSG( HHC17533, "E", RexxPackage, ( _SYMNAM_ ), "") ; \
        return -1; \
    } \
} while (0)

#else

#if !defined ( OPTION_DYNAMIC_LOAD )
#include <dlfcn.h>
#endif

#define HDLOPEN( _LIBHND_, _LIBNAM_, _LIBPAR_ ) do {\
    ( _LIBHND_ )  = dlopen( ( _LIBNAM_ ), ( _LIBPAR_ ) );\
    if (! ( _LIBHND_ )  ) { \
        WRMSG( HHC17530, "E", RexxPackage, dlerror() ) ; \
        return -1; \
    } \
} while (0)

#define HDLCLOSE( _LIBHND_) do {\
    if ( dlclose( ( _LIBHND_ ) ) != 0 ) { \
        WRMSG( HHC17530, "E", RexxPackage, dlerror() ) ; \
        return -1; \
    } \
    ( _LIBHND_ ) = NULL; \
} while (0)

#define HDLSYM(_SYMHND_, _LIBNAM_, _SYMNAM_ ) do {\
    ( _SYMHND_ )  = dlsym( ( _LIBNAM_ ), ( _SYMNAM_ ) );\
    if (! ( _SYMHND_ ) ) { \
        WRMSG( HHC17530, "E", RexxPackage, dlerror() ) ; \
        return -1; \
    } \
} while (0)

#endif

#endif /* #ifndef _HREXX_H_  */
