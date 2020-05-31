/* HREXX.H      (C) Copyright Enrico Sorichetti, 2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2017          */
/*              Rexx Interpreter Support                             */
/*                                                                   */
/*  Released under "The Q Public License Version 1"                  */
/*  (http://www.hercules-390.org/herclic.html) as modifications to   */
/*  Hercules.                                                        */

/*  inspired by the previous Rexx implementation by Jan Jaeger       */

#ifndef _HREXX_H_
#define _HREXX_H_

/*-------------------------------------------------------------------*/
/* #define some descriptive macros to make #if statements simpler    */
/*-------------------------------------------------------------------*/

#if defined( HAVE_OBJECT_REXX )
   #define        OBJECT_REXX       // Support for Object Rexx
#endif

#if defined( HAVE_REGINA_REXX )
   #define        REGINA_REXX       // Support for Regina Rexx
#endif

#if defined( OBJECT_REXX ) || defined( REGINA_REXX )
#define HAVE_REXX                   // Support for EITHER Rexx
#endif

#if defined( OBJECT_REXX ) && defined( REGINA_REXX )
#define HAVE_BOTH_REXX              // Support for BOTH Rexxes
#endif

#if defined( HAVE_REXX ) && !defined( HAVE_BOTH_REXX )
#define HAVE_ONLY_ONE_REXX          // Support for only ONE Rexx
#endif

#ifdef HAVE_REXX                    // if we have EITHER rexx...
/*-------------------------------------------------------------------*/
/* The Rexx packages this implementation supports                    */
/*-------------------------------------------------------------------*/
#define OOREXX_PKGNUM       0       // PkgNames[] entry number
#define REGINA_PKGNUM       1       // PkgNames[] entry number
#define PKGS                2       // Number of defined packages

#define OOREXX_PKG          OORexx
#define REGINA_PKG          Regina

#define OOREXX_PKGNAME      QSTR( OOREXX_PKG )
#define REGINA_PKGNAME      QSTR( REGINA_PKG )

/*-------------------------------------------------------------------*/
/* typedefs for Hercules Rexx implementation functions               */
/*-------------------------------------------------------------------*/
typedef int (*PFNEXECCMDFUNC)( char* scriptname, char* cmdline );
typedef int (*PFNEXECSUBFUNC)( char* scriptname, int argc, char* argv[] );
typedef int (*PFNHALTEXECFUNC)( pid_t pid, TID tid );

/*-------------------------------------------------------------------*/
/* Hercules Rexx implementation publicly visible global variables    */
/*-------------------------------------------------------------------*/
extern char*                PackageName;
extern char*                PackageVersion;
extern char*                PackageSource;
extern char                 PackageMajorVers;
extern BYTE                 MsgLevel;
extern BYTE                 MsgPrefix;
extern BYTE                 ErrPrefix;
extern PFNEXECCMDFUNC       ExecCmd;
extern PFNEXECSUBFUNC       ExecSub;
extern PFNHALTEXECFUNC      HaltExec;

/*-------------------------------------------------------------------*/
/* Hercules Rexx implementation publicly visible functions           */
/*-------------------------------------------------------------------*/
extern void  InitializeRexx ();
extern int   rexx_cmd( int argc, char* argv[], char* cmdline );
extern int   exec_cmd( int argc, char* argv[], char* cmdline );

/*-------------------------------------------------------------------*/
/* Various platform dependent constants                              */
/*-------------------------------------------------------------------*/
#if defined( _MSVC_ )
  #define EXTDELIM          ";"
  #define EXTENSIONS        ".REXX;.rexx;.REX;.rex;.CMD;.cmd;.RX;.rx"
  #define PATHDELIM         ";"
  #define PATHFORMAT        "%s\\%s%s"
#elif defined( __APPLE__ )
  #define EXTDELIM          ":"
  #define EXTENSIONS        ".rexx:.rex:.cmd:.rx" // APPLE is caseless
  #define PATHDELIM         ":"
  #define PATHFORMAT        "%s/%s%s"
#else
  #define EXTDELIM          ":"
  #define EXTENSIONS        ".REXX:.rexx:.REX:.rex:.CMD:.cmd:.RX:.rx"
  #define PATHDELIM         ":"
  #define PATHFORMAT        "%s/%s%s"
#endif

/*-------------------------------------------------------------------*/
/* Various implementation constants and helper macros                */
/*-------------------------------------------------------------------*/

#define HAVKEYW( kw )   (strcasecmp(  (kw),   argv[ iarg ]       ) == 0)
#define HAVABBR( abbr ) (strncasecmp( (abbr), argv[ iarg ], argl ) == 0)
#define INT2VOIDP(i)    (void*)(ssize_t)(i)

#if defined( FILENAME_MAX )
  #define  MAX_FULLPATH_LEN         (FILENAME_MAX    + 1)
#elif defined(_MAX_FNAME)
  #define  MAX_FULLPATH_LEN         (_MAX_FNAME      + 1)
#elif defined(_POSIX_NAME_MAX)
  #define  MAX_FULLPATH_LEN         (_POSIX_NAME_MAX + 1)
#else
  #define  MAX_FULLPATH_LEN         (4095            + 1)
#endif

/* Some helpful Hercules Rexx implementation constants */
#define DEF_RXSTRING_BUFSZ          256   // Default RXSTRING buf size
#define MAX_REXXSTART_ARGS          64    // Max RexxStart arguments
#define NUM_DIGITS_32               10    // Digits in printed 32-bit value

/* The following are generic Hercules Rexx error codes */
typedef U16              HR_ERR_T;        // Hercules Rexx error code
#define HRERR_OK                    0     // Success
#define HRERR_ERROR                 0x01  // Generic error
#define HRERR_FAILURE               0x02  // Generic failure
#define HRERR_BADARGS               1001  // Invalid arguments
#define HRERR_NOMEM                 1002  // Out of memory

/* The following defines are missing from both OORexx and Regina */
#define RXFUNC_ERROR                0x01  // Error
#define RXFUNC_FAILURE              0x02  // Failure
#define RXFUNC_BADENTRY             40    // Invalid Entry Conditions

/* A simple Rexx "in storage" script to retrieve Rexx version/source */
#define VER_SRC_INSTOR_SCRIPT       /* return results as 2 lines */   \
                                    "parse version ver\n"             \
                                    "parse source  src .\n"           \
                                    "return ver || '0a'x || src\n"

/* Hercules Rexx implementation variable names */
#define HR_ERRHNDLR_VNAME           "HREXX.ERRORHANDLER"
#define HR_RESPSTEM_VNAME           "HREXX.RESPSTEMNAME"
#define HR_PERSISTRESPSTEM_VNAME    "HREXX.PERSISTENTRESPSTEMNAME"

#endif // HAVE_REXX
#endif // _HREXX_H_
