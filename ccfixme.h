/* ccfixme.h (C) Copyright "Fish" (David B. Trout), 2011             */
/*           (C) and others 2013-2023                                */
/*                                                                   */
/*             Released under "The Q Public License Version 1"       */
/*             (http://www.hercules-390.org/herclic.html)            */
/*             as modifications to Hercules.                         */

/*-------------------------------------------------------------------*/
/*  This header file #defines a generic "FIXME" macro used to mark   */
/*  suspicious code needing fixed or at least further investigated.  */
/*  Sample usage: TODO( "Figure out a better way to calculate foo" ) */
/*  It is designed to work identically for GCC as well as MSVC. The  */
/*  only argument is the "fixme" message you wish to be issued. To   */
/*  suppress a compiler warning, use the "DISABLE_xxx_WARNING" and   */
/*  "ENABLE_xxx_WARNING" macros instead, defined in "ccnowarn.h".    */
/*-------------------------------------------------------------------*/

#ifndef _CCFIXME_H_
#define _CCFIXME_H_

/*-------------------------------------------------------------------*/
/* Construct a GCC "version" constant for easier gcc version testing */
/*-------------------------------------------------------------------*/
#if defined( __GNUC__ )
  #define  GCC_VERSION \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

/*-------------------------------------------------------------------*/
/* Same thing for CLANG                                              */
/*-------------------------------------------------------------------*/
#if defined( __clang__ )
  #define  CLANG_VERSION \
    (__clang_major__ * 10000 + __clang_minor__ * 100 + 0)
#endif

/*-------------------------------------------------------------------*/
/* Macros for issuing compiler messages                              */
/*-------------------------------------------------------------------*/

#define Q( _s )                 #_s
#define QSTR( _s )              Q( _s )
#define QLINE                   __FILE__ "(" QSTR( __LINE__ ) ") : "

#define WARN_LINE               QLINE "warning : "
#define FIXME_LINE              QLINE "FIXME : "
#define TODO_LINE               QLINE "TODO : "
#define NOTE_LINE               QLINE "NOTE : "

/*-------------------------------------------------------------------*/
/* Macro to suppress compiler "unreferenced variable" warnings       */
/*-------------------------------------------------------------------*/

#define UNREFERENCED(x)         do{}while(0 && x)
#define UNREFERENCED_370(x)     do{}while(0 && x)
#define UNREFERENCED_390(x)     do{}while(0 && x)
#define UNREFERENCED_900(x)     do{}while(0 && x)

/*-------------------------------------------------------------------*/
/* Determine GCC diagnostic pragma support level                     */
/*-------------------------------------------------------------------*/

#if defined(HAVE_GCC_DIAG_PRAGMA)
  #define QPRAGMA( x )          _Pragma( #x )
#endif

/*-------------------------------------------------------------------*/
/* The "FIXME" macro itself to mark code needing further research    */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
  #define FIXME( _str )         __pragma( message( FIXME_LINE  _str ))
#elif defined( __GNUC__ ) && defined( HAVE_GCC_DIAG_PRAGMA )
  #define FIXME( _str )         QPRAGMA( message(  FIXME_LINE  _str ))
#endif

#ifndef   FIXME
  #define FIXME( _str )         /* (do nothing) */
#endif

/*-------------------------------------------------------------------*/
/* Same idea, but for unfinished code items still under development  */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
  #define TODO( _str )          __pragma( message( TODO_LINE  _str ))
#elif defined( __GNUC__ ) && defined( HAVE_GCC_DIAG_PRAGMA )
  #define TODO( _str )          QPRAGMA( message(  TODO_LINE  _str ))
#endif

#ifndef   TODO
  #define TODO( _str )          /* (do nothing) */
#endif

/*-------------------------------------------------------------------*/
/* Same idea, but for issuing a build "#warning ..." message         */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
  #define WARNING( _str )       __pragma( message( WARN_LINE  _str ))
#elif defined( __GNUC__ ) && defined( HAVE_GCC_DIAG_PRAGMA )
  #define WARNING( _str )       QPRAGMA( message(  WARN_LINE  _str ))
#endif

#ifndef   WARNING
  #define WARNING( _str )       /* (do nothing) */
#endif

/*-------------------------------------------------------------------*/
/* Same idea, but for issuing an informative note during compile     */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
  #define NOTE( _str )          __pragma( message( NOTE_LINE  _str ))
#elif defined( __GNUC__ ) && defined( HAVE_GCC_DIAG_PRAGMA )
  #define NOTE( _str )          QPRAGMA( message(  NOTE_LINE  _str ))
#endif

#ifndef   NOTE
  #define NOTE( _str )          /* (do nothing) */
#endif

#endif /* _CCFIXME_H_ */
