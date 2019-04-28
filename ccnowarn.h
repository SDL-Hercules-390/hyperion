/* CCNOWARN.H (C) Copyright "Fish" (David B. Trout), 2011            */
/*             Suppress compiler warnings                            */
/*                                                                   */
/*             Released under "The Q Public License Version 1"       */
/*             (http://www.hercules-390.org/herclic.html)            */
/*             as modifications to Hercules.                         */

/*  This header forces suppression of certain compiler warnings      */
/*  that frequently occur all throughout most of Hercules.           */

/*-------------------------------------------------------------------*/
/* The "DISABLE_xxx_WARNING" and "ENABLE_xxx_WARNING" macros allow   */
/* you to temporarily suppress certain harmless compiler warnings.   */
/*                                                                   */
/* Sample usage: DISABLE_GCC_WARNING( "-Wpointer-to-int-cast" )      */
/* DISABLE_MSVC_WARNING( 4142 ) // "benign redefinition of type"     */
/* Use the "_DISABLE" macro before the source statement which is     */
/* causing the problem and the "ENABLE" macro shortly afterwards.    */
/*                                                                   */
/* PLEASE DO NOT GO OVERBOARD (overdo or overuse) THE SUPPRESSION    */
/* OF WARNINGS! Most warnings are actually bugs waiting to happen.   */
/*                                                                   */
/* The "DISABLE_xxx_WARNING" and "ENABLE_xxx_WARNING" macros are     */
/* only meant as a temporary measure until the warning itself can    */
/* be properly investigated and resolved.                            */
/*-------------------------------------------------------------------*/

#ifndef _CCNOWARN_H_
#define _CCNOWARN_H_

#include "ccfixme.h"      /* need HAVE_GCC_DIAG_PRAGMA, QPRAGMA, etc */

  /*-----------------------------------------------------------------*/
  /*                            MSVC                                 */
  /*-----------------------------------------------------------------*/

  #if defined( _MSVC_ )

    #define DISABLE_MSVC_WARNING( _num )    __pragma( warning( disable : _num ) )
    #define ENABLE_MSVC_WARNING( _num )     __pragma( warning( default : _num ) )

    #define PUSH_MSVC_WARNINGS()            __pragma( warning( push ))
    #define POP_MSVC_WARNINGS()             __pragma( warning( pop  ))

    /* Globally disable some uninteresting MSVC compiler warnings    */

    DISABLE_MSVC_WARNING( 4127 ) // "conditional expression is constant"
    DISABLE_MSVC_WARNING( 4142 ) // "benign redefinition of type"
    DISABLE_MSVC_WARNING( 4146 ) // "unary minus operator applied to unsigned type, result still unsigned"
    DISABLE_MSVC_WARNING( 4200 ) // "nonstandard extension used : zero-sized array in struct/union"
    DISABLE_MSVC_WARNING( 4244 ) // "conversion from 'x' to 'y', possible loss of data"
    DISABLE_MSVC_WARNING( 4267 ) // "conversion from size_t to int possible loss of data"
    DISABLE_MSVC_WARNING( 4748 ) // "/GS can not protect parameters and local variables from local buffer overrun because optimizations are disabled in function"

  #endif /* defined( _MSVC_ ) */

  #ifndef   PUSH_MSVC_WARNINGS
    #define PUSH_MSVC_WARNINGS()                     /* (do nothing) */
    #define POP_MSVC_WARNINGS()                      /* (do nothing) */
    #define DISABLE_MSVC_WARNING( _str )             /* (do nothing) */
    #define ENABLE_MSVC_WARNING(  _str )             /* (do nothing) */
  #endif

  /*-----------------------------------------------------------------*/
  /*                       GCC or CLANG                              */
  /*-----------------------------------------------------------------*/

  #if defined(HAVE_GCC_DIAG_PRAGMA)

    #define DISABLE_GCC_WARNING( _str )   QPRAGMA( GCC diagnostic ignored _str )
    #define ENABLE_GCC_WARNING(  _str )   QPRAGMA( GCC diagnostic warning _str )

    #if defined(HAVE_GCC_SET_UNUSED_WARNING)
      #define DISABLE_GCC_UNUSED_SET_WARNING            \
              DISABLE_GCC_WARNING("-Wunused-but-set-variable")
    #endif

    #if defined(HAVE_GCC_UNUSED_FUNC_WARNING)
      #define DISABLE_GCC_UNUSED_FUNCTION_WARNING       \
              DISABLE_GCC_WARNING("-Wunused-function")
    #endif

    #if defined(HAVE_GCC_DIAG_PUSHPOP)
      #define PUSH_GCC_WARNINGS()         QPRAGMA( GCC diagnostic push )
      #define POP_GCC_WARNINGS()          QPRAGMA( GCC diagnostic pop  )
    #endif

  #endif /* defined( HAVE_GCC_DIAG_PRAGMA ) */

  #ifndef   DISABLE_GCC_WARNING
    #define DISABLE_GCC_WARNING( _str )              /* (do nothing) */
    #define ENABLE_GCC_WARNING(  _str )              /* (do nothing) */
  #endif

  #if !defined(DISABLE_GCC_UNUSED_SET_WARNING)
    #define DISABLE_GCC_UNUSED_SET_WARNING           /* (do nothing) */
  #endif

  #if !defined(DISABLE_GCC_UNUSED_FUNCTION_WARNING)
    #define DISABLE_GCC_UNUSED_FUNCTION_WARNING      /* (do nothing) */
  #endif

  #ifndef   PUSH_GCC_WARNINGS
    #define PUSH_GCC_WARNINGS()                      /* (do nothing) */
    #define POP_GCC_WARNINGS()                       /* (do nothing) */
  #endif

  /* I am tired of BOGUS warnings about = {0} struct initialization! */
  DISABLE_GCC_WARNING( "-Wmissing-field-initializers" )
  DISABLE_GCC_WARNING( "-Wmissing-braces" )

  #if defined( GCC_VERSION ) && GCC_VERSION >= 40800 /* gcc >= 4.8.0 */
  /* Silence warnings about CASSERT macro usage within functions too */
  DISABLE_GCC_WARNING( "-Wunused-local-typedefs" )
  #endif

  // "converts between pointers to integer types with different sign"
  DISABLE_GCC_WARNING( "-Wpointer-sign" )

  /*-----------------------------------------------------------------*/
  /*            define support for other compilers here              */
  /*-----------------------------------------------------------------*/

  /* Don't forget to define all of the "FIXME" et al. macros too!    */

#endif /* _CCNOWARN_H_ */
