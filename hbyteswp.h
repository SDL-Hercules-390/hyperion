/* HBYTESWP.H   (C) Copyright Roger Bowler, 2012                     */
/*              Hercules Little <> Big Endian conversion             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* These definitions are only nessesary when running on older        */
/* versions of linux that do not have /usr/include/byteswap.h        */
/* compile option -DNO_ASM_BYTESWAP will expand 'C' code             */
/* otherwise Intel (486+) assember will be generated  (Jan Jaeger)   */

#ifndef _BYTESWAP_H
#define _BYTESWAP_H

#if !defined(NO_ASM_BYTESWAP)

  #include "htypes.h"       // (need Hercules fixed-size data types)

  #if defined( _MSVC_ )

    static __inline  uint16_t  __fastcall  bswap_16 ( uint16_t  x )
    {
      return _byteswap_ushort((x));
    }

    static __inline  uint32_t  __fastcall  bswap_32 ( uint32_t  x )
    {
      return _byteswap_ulong((x));
    }

  #else // !defined( _MSVC_ )

    static __inline__ uint16_t (ATTR_REGPARM(1) bswap_16)(uint16_t x)
    {
#if defined(__x86_64__)
      __asm__("xchgb %b0,%h0" : "=Q" (x) :  "0" (x));
#else
      __asm__("xchgb %b0,%h0" : "=q" (x) :  "0" (x));
#endif
      return x;
    }

    static __inline__ uint32_t (ATTR_REGPARM(1) bswap_32)(uint32_t x)
    {
#if defined(__x86_64__)
      __asm__("bswapl %0" : "=r" (x) : "0" (x));
#else
      __asm__("bswap %0" : "=r" (x) : "0" (x));
#endif
      return x;
    }

  #endif // defined( _MSVC_ )

#else // defined(NO_ASM_BYTESWAP)

  #define bswap_16(_x) \
          ( (((_x) & 0xFF00) >> 8) \
          | (((_x) & 0x00FF) << 8) )

  #define bswap_32(_x) \
          ( (((_x) & 0xFF000000) >> 24) \
          | (((_x) & 0x00FF0000) >> 8)  \
          | (((_x) & 0x0000FF00) << 8)  \
          | (((_x) & 0x000000FF) << 24) )

#endif // !defined(NO_ASM_BYTESWAP)

#if defined( _MSVC_ )

  // Microsoft's Toolkit 2003 compiler (version 1300) has a known bug
  // that causes the _byteswap_uint64 intrinsic to screw up if global
  // otimizations are enabled. The new VStudio 8.0 compiler (version
  // 14.00) doesn't have this problem that I am aware of. NOTE that
  // the #pragma must be outside the function (at global scope) to
  // prevent compiler error C2156 "pragma must be outside function".

  #if ( _MSC_VER <= VS2003 )
    #pragma optimize("g",off)     // (disable global optimizations)
  #endif
  static __inline  uint64_t  __fastcall  bswap_64(uint64_t x)
  {
      return _byteswap_uint64((x));
  }
  #if ( _MSC_VER <= VS2003 )
    #pragma optimize ( "", on )
  #endif

#else // !defined( _MSVC_ )

  #if defined(NO_ASM_BYTESWAP)
    #define bswap_64(_x) \
          ( ((U64)((_x) & 0xFF00000000000000ULL) >> 56) \
          | ((U64)((_x) & 0x00FF000000000000ULL) >> 40) \
          | ((U64)((_x) & 0x0000FF0000000000ULL) >> 24) \
          | ((U64)((_x) & 0x000000FF00000000ULL) >> 8)  \
          | ((U64)((_x) & 0x00000000FF000000ULL) << 8)  \
          | ((U64)((_x) & 0x0000000000FF0000ULL) << 24) \
          | ((U64)((_x) & 0x000000000000FF00ULL) << 40) \
          | ((U64)((_x) & 0x00000000000000FFULL) << 56) )
  #else
    static __inline__ uint64_t (ATTR_REGPARM(1) bswap_64)(uint64_t x)
    {
    #if defined(__x86_64__)
      __asm__("bswapq %0" : "=r" (x) : "0" (x));
      return x;
    #else // swap the two words after byteswapping them
      union
      {
        struct
        {
          uint32_t high,low;
        } words;
        uint64_t quad;
      } value;

      value.quad=x;
      __asm__("bswap %0" : "=r" (value.words.high) : "0" (value.words.high));
      __asm__("bswap %0" : "=r" (value.words.low) : "0" (value.words.low));
      __asm__("xchgl %0,%1" : "=r" (value.words.high), "=r" (value.words.low) :
              "0" (value.words.high), "1" (value.words.low));
      return value.quad;
    #endif // defined(__x86_64__)
    }
  #endif // defined(NO_ASM_BYTESWAP)

#endif // defined( _MSVC_ )

#endif // _BYTESWAP_H
