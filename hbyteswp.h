/* HBYTESWP.H   (C) Copyright Roger Bowler, 2012                     */
/*              (C) and others 2013-2022                             */
/*              Hercules Little <> Big Endian conversion             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* These definitions are only nessesary when running on older        */
/* versions of linux that do not have /usr/include/byteswap.h.       */
/* Compile option -DNO_ASM_BYTESWAP will expand 'C' code.            */
/* Otherwise custom assember will be generated.  (Jan Jaeger)        */
/*-------------------------------------------------------------------*/

#ifndef _BYTESWAP_H
#define _BYTESWAP_H

/*--------------------------------------------------------------------
  The following sequence of tests allows platforms to use a compiler's
  builtin intrinsic byte swapping function even if it fails to provide
  its own <byteswap.h> header.
  -------------------------------------------------------------------*/

#if defined( HAVE_BYTESWAP_H )

    /* Do nothing; i.e. use the functions defined in <byteswap.h> */

#elif defined( HAVE_SWAP_BUILTINS )

  #include "htypes.h"   // (Hercules fixed-size data types)

  #if defined( _MSVC_ )

    #define bswap_16(x)     _byteswap_ushort((x))
    #define bswap_32(x)     _byteswap_ulong((x))
    #define bswap_64(x)     _byteswap_uint64((x))

  #else // !defined( _MSVC_ )

    #define bswap_16(x)    __builtin_bswap16((x))
    #define bswap_32(x)    __builtin_bswap32((x))
    #define bswap_64(x)    __builtin_bswap64((x))

  #endif

#elif !defined( NO_ASM_BYTESWAP )

  #include "htypes.h"       // (need Hercules fixed-size data types)

  inline uint16_t (ATTR_REGPARM(1) bswap_16 )( uint16_t  x )
  {
  #if defined(__x86_64__)
      __asm__("xchgb %b0,%h0" : "=Q" (x) :  "0" (x));
  #else
      __asm__("xchgb %b0,%h0" : "=q" (x) :  "0" (x));
  #endif
    return x;
  }

  inline uint32_t (ATTR_REGPARM(1) bswap_32 )( uint32_t  x )
  {
  #if defined(__x86_64__)
      __asm__("bswapl %0" : "=r" (x) : "0" (x));
  #else
      __asm__("bswap  %0" : "=r" (x) : "0" (x));
  #endif
      return x;
  }

  inline uint64_t (ATTR_REGPARM(1) bswap_64 )( uint64_t  x )
  {
  #if defined(__x86_64__)
      __asm__("bswapq %0" : "=r" (x) : "0" (x));
      return x;
  #else // swap the two words after byteswapping them
      union
      {
          struct
          {
              uint32_t  high, low;
          } words;
          uint64_t      quad;
      }                 value;
      value.quad = x;
      __asm__("bswap %0"    : "=r" (value.words.high) : "0" (value.words.high));
      __asm__("bswap %0"    : "=r" (value.words.low)  : "0" (value.words.low));
      __asm__("xchgl %0,%1" : "=r" (value.words.high), "=r" (value.words.low) :
                               "0" (value.words.high),  "1" (value.words.low));
      return value.quad;
  #endif
  }

#else

  #define bswap_16(_x)                                      \
          ( (((_x) & 0xFF00) >> 8)                          \
          | (((_x) & 0x00FF) << 8) )

  #define bswap_32(_x)                                      \
          ( (((_x) & 0xFF000000) >> 24)                     \
          | (((_x) & 0x00FF0000) >>  8)                     \
          | (((_x) & 0x0000FF00) <<  8)                     \
          | (((_x) & 0x000000FF) << 24) )

  #define bswap_64(_x)                                      \
        ( ((U64)((_x) & 0xFF00000000000000ULL) >> 56)       \
        | ((U64)((_x) & 0x00FF000000000000ULL) >> 40)       \
        | ((U64)((_x) & 0x0000FF0000000000ULL) >> 24)       \
        | ((U64)((_x) & 0x000000FF00000000ULL) >>  8)       \
        | ((U64)((_x) & 0x00000000FF000000ULL) <<  8)       \
        | ((U64)((_x) & 0x0000000000FF0000ULL) << 24)       \
        | ((U64)((_x) & 0x000000000000FF00ULL) << 40)       \
        | ((U64)((_x) & 0x00000000000000FFULL) << 56) )

#endif

#endif // _BYTESWAP_H
