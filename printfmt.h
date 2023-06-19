/* PRINTFMT.H   (C) Copyright "Fish" (David B. Trout), 2013          */
/*              printf/sscanf format specifier strings               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _PRINTFMT_H_
#define _PRINTFMT_H_

/*-------------------------------------------------------------------*/
/*  Length modifiers only                                            */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
#define  I16_FMT                    "h"         // length modifier only
#define  I32_FMT                    "I32"       // length modifier only
#define  I64_FMT                    "I64"       // length modifier only
#elif defined( __PRI_64_LENGTH_MODIFIER__ )     // (MAC)
#define  I16_FMT                    "h"         // length modifier only
#define  I32_FMT                    ""          // length modifier only
#define  I64_FMT                    __PRI_64_LENGTH_MODIFIER__
#elif defined( SIZEOF_LONG ) && SIZEOF_LONG >= 8
#define  I16_FMT                    "h"         // length modifier only
#define  I32_FMT                    ""          // length modifier only
#define  I64_FMT                    "l"         // length modifier only
#else
#define  I16_FMT                    "h"         // length modifier only
#define  I32_FMT                    ""          // length modifier only
#define  I64_FMT                    "ll"        // length modifier only
#endif

/*-------------------------------------------------------------------*/
/* C99 ISO Standard Names             (most of them but not all)     */
/*-------------------------------------------------------------------*/

#if !defined( PRId64 )

#define PRId16                     I16_FMT "d"  // without leading "%"
#define PRId32                     I32_FMT "d"  // without leading "%"
#define PRId64                     I64_FMT "d"  // without leading "%"

#define PRIi16                     I16_FMT "i"  // without leading "%"
#define PRIi32                     I32_FMT "i"  // without leading "%"
#define PRIi64                     I64_FMT "i"  // without leading "%"

#define PRIu16                     I16_FMT "u"  // without leading "%"
#define PRIu32                     I32_FMT "u"  // without leading "%"
#define PRIu64                     I64_FMT "u"  // without leading "%"

#define PRIx16                     I16_FMT "x"  // without leading "%"
#define PRIx32                     I32_FMT "x"  // without leading "%"
#define PRIx64                     I64_FMT "x"  // without leading "%"

#define PRIX16                     I16_FMT "X"  // without leading "%"
#define PRIX32                     I32_FMT "X"  // without leading "%"
#define PRIX64                     I64_FMT "X"  // without leading "%"

#if defined( SIZEOF_INT_P ) && SIZEOF_INT_P >= 8
#define PRIdPTR                    I64_FMT "d"  // without leading "%"
#define PRIiPTR                    I64_FMT "i"  // without leading "%"
#define PRIuPTR                    I64_FMT "u"  // without leading "%"
#define PRIxPTR                    I64_FMT "x"  // without leading "%"
#define PRIXPTR                    I64_FMT "X"  // without leading "%"
#else
#define PRIdPTR                    I32_FMT "d"  // without leading "%"
#define PRIiPTR                    I32_FMT "i"  // without leading "%"
#define PRIuPTR                    I32_FMT "u"  // without leading "%"
#define PRIxPTR                    I32_FMT "x"  // without leading "%"
#define PRIXPTR                    I32_FMT "X"  // without leading "%"
#endif

#define SCNd16                     PRId16       // without leading "%"
#define SCNd32                     PRId32       // without leading "%"
#define SCNd64                     PRId64       // without leading "%"

#define SCNi16                     PRIi16       // without leading "%"
#define SCNi32                     PRIi32       // without leading "%"
#define SCNi64                     PRIi64       // without leading "%"

#define SCNu16                     PRIu16       // without leading "%"
#define SCNu32                     PRIu32       // without leading "%"
#define SCNu64                     PRIu64       // without leading "%"

#define SCNx16                     PRIx16       // without leading "%"
#define SCNx32                     PRIx32       // without leading "%"
#define SCNx64                     PRIx64       // without leading "%"

#define SCNdPTR                    PRIdPTR      // without leading "%"
#define SCNiPTR                    PRIiPTR      // without leading "%"
#define SCNuPTR                    PRIuPTR      // without leading "%"
#define SCNxPTR                    PRIxPTR      // without leading "%"

#endif // !defined( PRId64 )

/*-------------------------------------------------------------------*/
/* Hercules pointer/address formats                                  */
/*-------------------------------------------------------------------*/

#if defined( SIZEOF_INT_P ) && SIZEOF_INT_P >= 8
 #define PTR_FMTx          "%16.16"PRIx64       // complete format spec
 #define PTR_FMTX          "%16.16"PRIX64       // complete format spec
 #define PTR_CAST( p )     ((U64)(p))           // cast to printable
#else
 #define PTR_FMTx           "%8.8" PRIx32       // complete format spec
 #define PTR_FMTX           "%8.8" PRIX32       // complete format spec
 #define PTR_CAST( p )     ((U32)(p))           // cast to printable
#endif

/*-------------------------------------------------------------------*/
/* Hercules thread-id format                                         */
/*-------------------------------------------------------------------*/

#if defined( _MSVC_ )
  #define TIDPAT             "%8.8"PRIx32       // complete format spec
  #define SCN_TIDPAT            "%"PRIx32       // complete format spec
  #define TID_INT                     U32       // integer most like a TID
#elif defined( SIZEOF_PTHREAD_T ) && SIZEOF_PTHREAD_T >= 8
  #define TIDPAT           "%16.16"PRIx64       // complete format spec
  #define SCN_TIDPAT            "%"PRIx64       // complete format spec
  #define TID_INT                     U64       // integer most like a TID
#else
  #define TIDPAT             "%8.8"PRIx32       // complete format spec
  #define SCN_TIDPAT            "%"PRIx32       // complete format spec
  #define TID_INT                     U32       // integer most like a TID
#endif
#define TID_CAST( _tid )   ((TID_INT)(_tid))    // cast to printable

#define LOG_TID_BEGIN( _tid, _name ) \
    WRMSG( HHC00100, "I", \
        TID_CAST( _tid ), get_thread_priority_id( _tid ), _name );

#define LOG_TID_END( _tid, _name ) \
    WRMSG( HHC00101, "I", \
        TID_CAST( _tid ), get_thread_priority_id( _tid ), _name );

#define LOG_THREAD_BEGIN( _name )   LOG_TID_BEGIN( thread_id(), _name )
#define LOG_THREAD_END(   _name )   LOG_TID_END  ( thread_id(), _name )

#endif // _PRINTFMT_H_
