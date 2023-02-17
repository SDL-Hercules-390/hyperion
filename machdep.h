/* MACHDEP.H    (C) Copyright Greg Smith, 2001-2012                  */
/*              (C) and others 2013-2023                             */
/*              Hercules machine specific code                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*                                                                   */
/* This header file contains the following functions, defined as     */
/* either normal unoptimzed C code, or else as hand-tuned optimized  */
/* assembler-assisted functions for the given machine architecture:  */
/*                                                                   */
/*   Atomic COMPARE-AND-EXCHANGE functions:                          */
/*                                                                   */
/*         cmpxchg1, cmpxchg4, cmpxchg8, cmpxchg16                   */
/*                                                                   */
/*   Atomic half, full and doubleword fetch/store functions:         */
/*                                                                   */
/*     fetch_hw, fetch_hw_noswap, store_hw, store_hw_noswap          */
/*     fetch_fw, fetch_fw_noswap, store_fw, store_fw_noswap          */
/*     fetch_dw, fetch_dw_noswap, store_dw, store_dw_noswap          */
/*                                                                   */
/*   64-bit architectures would normally not need to specify any     */
/*   of the fetch_ or store_ variants.                               */
/*                                                                   */
/*   32-bit architectures should specify BOTH of the 'fetch_dw'      */
/*   and 'store_dw' variants.                                        */
/*                                                                   */
/*   Little-endian machines should specify the 'noswap' variants.    */
/*                                                                   */
/*   Big-endian machines can specify either, both being the same.    */
/*                                                                   */
/*-------------------------------------------------------------------*/

#ifndef _MACHDEP_H
#define _MACHDEP_H

#include "opcode.h"         // (need CSWAP32, et.al macros, etc)
#include "htypes.h"         // (need Hercules fixed-size data types)

/*-------------------------------------------------------------------
 * Microsoft Visual C/C++...
 *-------------------------------------------------------------------*/
#if defined( _MSVC_ )

  // PROGRAMMING NOTE: Optimizations normally only apply for release
  // builds, but we support optionally enabling them for debug too,
  // as well as purposely DISABLING them for troubleshooting...

     #define  OPTION_ENABLE_MSVC_OPTIMIZATIONS_FOR_DEBUG_BUILDS_TOO
  // #define  OPTION_DISABLE_MSVC_OPTIMIZATIONS

  #undef GEN_MSC_ASSISTS

  #if defined( DEBUG) || defined( _DEBUG )
    #if defined(OPTION_ENABLE_MSVC_OPTIMIZATIONS_FOR_DEBUG_BUILDS_TOO) && \
       !defined(OPTION_DISABLE_MSVC_OPTIMIZATIONS)
      #define GEN_MSC_ASSISTS
    #endif
  #else // (presumed RELEASE build)
    #if !defined(OPTION_DISABLE_MSVC_OPTIMIZATIONS)
      #define GEN_MSC_ASSISTS
    #endif
  #endif // (debug or release)

  #undef MSC_X86_32BIT        // any 32-bit X86  (Pentium Pro, Pentium II, Pentium III or better)
  #undef MSC_X86_64BIT        // any 64-bit X86  (AMD64 or Intel Itanium)
  #undef MSC_X86_AMD64        // AMD64 only
  #undef MSC_X86_IA64         // Intel Itanium only

  #if defined( _M_IX86 ) && ( _M_IX86 >= 600 )
    #define MSC_X86_32BIT
  #endif
  #if defined( _M_AMD64 )
    #define MSC_X86_AMD64
    #define MSC_X86_64BIT
  #endif
  #if defined( _M_IA64 )
    #define MSC_X86_IA64
    #define MSC_X86_64BIT
  #endif

  #if defined( GEN_MSC_ASSISTS ) && (defined( MSC_X86_32BIT ) || defined( MSC_X86_64BIT ))

    // Any X86 at all (both 32/64-bit)

    #pragma  intrinsic  ( _InterlockedCompareExchange )

    #define  cmpxchg1(  x, y, z )  cmpxchg1_x86( x, y, z )
    #define  cmpxchg4(  x, y, z )  cmpxchg4_x86( x, y, z )
    #define  cmpxchg8(  x, y, z )  cmpxchg8_x86( x, y, z )

    #pragma intrinsic ( _InterlockedCompareExchange64 )

    inline BYTE __fastcall cmpxchg8_x86 ( U64* old, U64 unew, volatile void* ptr )
    {
        // returns 0 == success, 1 otherwise
        U64 tmp = *old;
        *old = _InterlockedCompareExchange64( ptr, unew, *old );
        return ((tmp == *old) ? 0 : 1);
    }
    inline BYTE __fastcall cmpxchg4_x86 ( U32* old, U32 unew, volatile void* ptr )
    {
        // returns 0 == success, 1 otherwise
        U32 tmp = *old;
        *old = _InterlockedCompareExchange( ptr, unew, *old );
        return ((tmp == *old) ? 0 : 1);
    }

    // (must follow cmpxchg4 since it uses it)
    inline BYTE __fastcall cmpxchg1_x86 ( BYTE* old, BYTE unew, volatile void* ptr )
    {
        // returns 0 == success, 1 otherwise

        LONG_PTR  off, shift;
        BYTE  cc;
        U32  *ptr4, val4, old4, new4;

        off   = (LONG_PTR)ptr & 3;
        shift = (3 - off) * 8;
        ptr4  = (U32*)(((BYTE*)ptr) - off);
        val4  = CSWAP32(*ptr4);

        old4  = CSWAP32((val4 & ~(0xff << shift)) | (*old << shift));
        new4  = CSWAP32((val4 & ~(0xff << shift)) | ( unew << shift));

        cc    = cmpxchg4( &old4, new4, ptr4 );

        *old  = (CSWAP32(old4) >> shift) & 0xff;

        return cc;
    }

    #if !defined( MSC_X86_32BIT ) // 64-bit only!

      #pragma intrinsic ( _InterlockedCompareExchange128 )
      #define cmpxchg16(  x, y, z, r, s  ) cmpxchg16_x86( x, y, z, r, s )
      inline int cmpxchg16_x86 ( U64* old1, U64* old2, U64 new1, U64 new2, volatile void* ptr )
      {
        // Please note : old1 MUST be 16-byte aligned !
        // returns 0 == success, 1 otherwise
        UNREFERENCED( old2 );
        return ( _InterlockedCompareExchange128( ptr, new2, new1, old1 ) ? 0 : 1 );
      }

    #endif // !defined( MSC_X86_32BIT ) // 64-bit only!

    #if defined( MSC_X86_32BIT )

      #define fetch_dw_noswap(_p) fetch_dw_x86_noswap((_p))
      // (must follow cmpxchg8 since it uses it)
      inline U64 __fastcall fetch_dw_x86_noswap ( volatile const void* ptr )
      {
        U64 value = *(U64*)ptr;
        cmpxchg8( &value, value, (U64*)ptr );
        return value;
      }

      #define store_dw_noswap(_p, _v) store_dw_x86_noswap( (_p), (_v))
      // (must follow cmpxchg8 since it uses it)
      inline void __fastcall store_dw_x86_noswap ( volatile void* ptr, U64 value )
      {
        U64 orig = *(U64*)ptr;
        while ( cmpxchg8( &orig, value, (U64*)ptr ) );
      }
    #endif /* defined( MSC_X86_32BIT ) */

  #endif // defined( GEN_MSC_ASSISTS ) && (defined( MSC_X86_32BIT ) || defined( MSC_X86_64BIT ))

  // ------------------------------------------------------------------

  #if defined( GEN_MSC_ASSISTS ) && defined( MSC_X86_IA64 )

    // (64-bit Itanium assists only)

    // ZZ FIXME: we should probably use the 'cmpxchg16b' instruction here
    // instead if the processor supports it (CPUID instruction w/EAX function
    // code 1 == Feature Information --> ECX bit 13 = CMPXCHG16B available)

    #pragma  intrinsic  ( _AcquireSpinLock )
    #pragma  intrinsic  ( _ReleaseSpinLock )
    #pragma  intrinsic  ( _ReadWriteBarrier )

    #define  cmpxchg16(     x1, x2, y1, y2, z ) \
             cmpxchg16_x86( x1, x2, y1, y2, z )

    inline int __fastcall cmpxchg16_x86 ( U64* old1, U64* old2,
                                          U64  new1, U64  new2,
                                          volatile void* ptr )
    {
        // returns 0 == success, 1 otherwise

        static unsigned __int64 lock = 0;
        int code;

        _AcquireSpinLock( &lock );

        _ReadWriteBarrier();

        if (*old1 == *(U64*)ptr && *old2 == *((U64*)ptr + 1))
        {
            *(U64*)ptr = new1;
            *((U64*)ptr + 1) = new2;
            code = 0;
        }
        else
        {
            *old1 = *((U64*)ptr);
            *old2 = *((U64*)ptr + 1);
            code = 1;
        }

        _ReleaseSpinLock( &lock );

        return code;
    }

  #endif // defined( GEN_MSC_ASSISTS ) && defined( MSC_X86_IA64 )

#else // !defined( _MSVC_ )
/*-------------------------------------------------------------------
 * GNU C or other compiler...   (i.e. NON-Microsoft C/C++)
 *-------------------------------------------------------------------*/
  #if defined( __i686__       ) \
   || defined( __pentiumpro__ ) \
   || defined( __pentium4__   ) \
   || defined( __athlon__     ) \
   || defined( __athlon       )

    #define _ext_ia32

  #elif defined( __amd64__ )

    #define _ext_amd64

  #elif defined( __powerpc__ ) \
     || defined( __ppc__     ) \
     || defined( __POWERPC__ ) \
     || defined( __PPC__     ) \
     || defined( _POWER      )

    #define _ext_ppc

  #endif

/*-------------------------------------------------------------------
 * Intel pentiumpro/i686
 *-------------------------------------------------------------------*/
#if defined( _ext_ia32 )
    /*
     * If PIC is defined then ebx is used as the 'thunk' reg
     * However cmpxchg8b requires ebx
     * In this case we load the value into esi and then
     * exchange esi and ebx before and after cmpxchg8b
     */
#undef BREG
#undef XCHG_BREG

#if defined( PIC ) && !defined( __CYGWIN__ )
  #define BREG          "S"
  #define XCHG_BREG     "xchgl   %%ebx,%%esi\n\t"
#else
  #define BREG          "b"
  #define XCHG_BREG     ""
#endif

#define cmpxchg1(x,y,z) cmpxchg1_i686(x,y,z)
inline BYTE cmpxchg1_i686(BYTE *old, BYTE new, void *ptr) {
 BYTE code;
 __asm__ __volatile__ (
         "lock; cmpxchgb %b3,%4\n\t"
         "setnz   %b0"
         : "=q"(code), "=a"(*old)
         : "1" (*old),
           "q" (new),
           "m" (*(BYTE *)ptr)
         : "cc" );
 return code;
}

#define cmpxchg4(x,y,z) cmpxchg4_i686(x,y,z)
inline BYTE cmpxchg4_i686(U32 *old, U32 new, void *ptr) {
 BYTE code;
 __asm__ __volatile__ (
         "lock; cmpxchgl %3,%4\n\t"
         "setnz   %b0"
         : "=q"(code), "=a"(*old)
         : "1" (*old),
           "q" (new),
           "m" (*(U32 *)ptr)
         : "cc" );
 return code;
}

#define cmpxchg8(x,y,z) cmpxchg8_i686(x,y,z)
inline BYTE cmpxchg8_i686(U64 *old, U64 new, void *ptr) {
 BYTE code;
__asm__ __volatile__ (
         XCHG_BREG
         "lock; cmpxchg8b %5\n\t"
         XCHG_BREG
         "setnz   %b0"
         : "=q"(code), "=A"(*old)
         : "1" (*old),
           BREG ((unsigned long)new),
           "c"  ((unsigned long)(new >> 32)),
           "m" (*(U64 *)ptr)
         : "cc");
 return code;
}

#define fetch_dw_noswap(x) fetch_dw_i686_noswap(x)
inline U64 fetch_dw_i686_noswap(const void *ptr)
{
 U64 value = *(U64 *)ptr;
__asm__ __volatile__ (
         XCHG_BREG
         "lock; cmpxchg8b (%4)\n\t"
         XCHG_BREG
         : "=A" (value)
         : "0" (value),
           BREG ((unsigned long)value),
           "c"  ((unsigned long)(value >> 32)),
           "D" (ptr));
 return value;
}

#define store_dw_noswap(x,y) store_dw_i686_noswap(x,y)
inline void store_dw_i686_noswap(void *ptr, U64 value) {
__asm__ __volatile__ (
         XCHG_BREG
         "1:\t"
         "lock; cmpxchg8b %3\n\t"
         "jne     1b\n\t"
         XCHG_BREG
         :
         : "A" (*(U64 *)ptr),
           BREG ((unsigned long)value),
           "c"  ((unsigned long)(value >> 32)),
           "m" (*(U64 *)ptr));
}

#endif /* defined(_ext_ia32) */

/*-------------------------------------------------------------------
 * AMD64
 *-------------------------------------------------------------------*/
#if defined(_ext_amd64)

#define cmpxchg1(x,y,z) cmpxchg1_amd64(x,y,z)
inline BYTE cmpxchg1_amd64(BYTE *old, BYTE new, void *ptr) {
 /* returns 0 on success otherwise returns 1 */
 BYTE code;
 BYTE *ptr_data=ptr;
 __asm__ __volatile__ (
         "lock;   cmpxchgb %b2,%4\n\t"
         "setnz   %b0\n\t"
         : "=q"(code), "=a"(*old)
         : "q"(new),
           "1"(*old),
           "m"(*ptr_data)
         : "cc");
 return code;
}

#define cmpxchg4(x,y,z) cmpxchg4_amd64(x,y,z)
inline BYTE cmpxchg4_amd64(U32 *old, U32 new, void *ptr) {
 /* values passed in guest big-endian format */
 /* returns 0 on success otherwise returns 1 */
 BYTE code;
 U32 *ptr_data=ptr;
 __asm__ __volatile__ (
         "lock;   cmpxchgl %2,%4\n\t"
         "setnz   %b0\n\t"
         : "=q"(code), "=a"(*old)
         : "q"(new),
           "1"(*old),
           "m"(*ptr_data)
         : "cc");
 return code;
}

#define cmpxchg8(x,y,z) cmpxchg8_amd64(x,y,z)
inline BYTE cmpxchg8_amd64(U64 *old, U64 new, void *ptr) {
 /* values passed in guest big-endian format */
 /* returns 0 on success otherwise returns 1 */
 BYTE code;
 U64 *ptr_data=ptr;
 __asm__ __volatile__ (
         "lock;   cmpxchgq %2,%4\n\t"
         "setnz   %b0\n\t"
         : "=q"(code), "=a"(*old)
         : "q"(new),
           "1"(*old),
           "m"(*ptr_data)
         : "cc");
 return code;
}

#define cmpxchg16(x,y,z,r,s) cmpxchg16_amd64(x,y,z,r,s)
inline int cmpxchg16_amd64(U64 *old1, U64 *old2, U64 new1, U64 new2, volatile void *ptr) {
/* returns 0 on success otherwise returns 1 */
    BYTE code;
    volatile __int128_t *ptr_data=ptr;
    __asm__ __volatile__ (
        "lock;   cmpxchg16b %1\n\t"
        "setnz   %b0\n\t"
        : "=q" ( code ), "+m" ( *ptr_data ), "+a" ( *old1 ), "+d" ( *old2 )
        : "c" ( new2 ), "b" ( new1 )
        : "cc");
    return (int)code;
}

#endif /* defined(_ext_amd64) */

/*-------------------------------------------------------------------
 * PowerPC
 *-------------------------------------------------------------------*/
#if defined(_ext_ppc)

/* From /usr/src/linux/include/asm-ppc/system.h */

/* NOTE: IBM's VisualAge compiler likes 1: style labels
         but GNU's gcc compiler running on AIX does not. */

#if !defined( __GNUC__ )        // (VisualAge presumed)
  #define LABEL1 "1:\n"
  #define LABEL2 "2:\n"
  #define BRNCH2 "2f"
  #define BRNCH1 "1b"
#else                           // (else gcc...)
  #define LABEL1 "loop%=:\n"
  #define LABEL2 "exit%=:\n"
  #define BRNCH2 "exit%="
  #define BRNCH1 "loop%="
#endif

/* NOTE: Both VisualAge *and* gcc define __64BIT__
         see: http://gmplib.org/list-archives/gmp-discuss/2008-July/003339.html */

#if defined( __64BIT__ )

inline U64
__cmpxchg_u64(volatile U64 *p, U64 old, U64 new)
{
    U64 prev;

    __asm__ __volatile__ ("\n"
LABEL1
"       ldarx   %0,0,%2\n\
        cmpd    0,%0,%3\n\
        bne     "BRNCH2"\n\
        stdcx.  %4,0,%2\n\
        bne-    "BRNCH1"\n"
"       sync\n"
LABEL2
    : "=&r" (prev), "=m" (*p)
    : "r" (p), "r" (old), "r" (new), "m" (*p)
    : "cc", "memory");

    return prev;
}

#define cmpxchg8(x,y,z) cmpxchg8_ppc(x,y,z)
inline BYTE cmpxchg8_ppc(U64 *old, U64 new, void *ptr) {
/* values passed in guest big-endian format */
/* returns 0 on success otherwise returns 1 */
U64 prev = *old;
return (prev != (*old = __cmpxchg_u64((U64*)ptr, prev, new)));
}

#endif // defined( __64BIT__ )

inline U32
__cmpxchg_u32(volatile U32 *p, U32 old, U32 new)
{
    U32 prev;

    __asm__ __volatile__ ("\n"
LABEL1
"       lwarx   %0,0,%2\n\
        cmpw    0,%0,%3\n\
        bne     "BRNCH2"\n\
        stwcx.  %4,0,%2\n\
        bne-    "BRNCH1"\n"
"       sync\n"
LABEL2
    : "=&r" (prev), "=m" (*p)
    : "r" (p), "r" (old), "r" (new), "m" (*p)
    : "cc", "memory");

    return prev;
}

#define cmpxchg4(x,y,z) cmpxchg4_ppc(x,y,z)
inline BYTE cmpxchg4_ppc(U32 *old, U32 new, void *ptr) {
/* values passed in guest big-endian format */
/* returns 0 on success otherwise returns 1 */
U32 prev = *old;
return (prev != (*old = __cmpxchg_u32((U32*)ptr, prev, new)));
}

#define cmpxchg1(x,y,z) cmpxchg1_ppc(x,y,z)
inline BYTE cmpxchg1_ppc(BYTE *old, BYTE new, void *ptr) {
/* returns 0 on success otherwise returns 1 */
long  off, shift;
BYTE  cc;
U32  *ptr4, val4, old4, new4;

    off = (long)ptr & 3;
    shift = (3 - off) * 8;
    ptr4 = ptr - off;
    val4 = *ptr4;
    old4 = (val4 & ~(0xff << shift)) | (*old << shift);
    new4 = (val4 & ~(0xff << shift)) | (new << shift);
    cc = cmpxchg4_ppc(&old4, new4, ptr4);
    *old = (old4 >> shift) & 0xff;
    return cc;
}

#endif /* defined(_ext_ppc) */

/*-------------------------------------------------------------------
 * ARM aarch64 (like the Raspberry Pi 4)
 *-------------------------------------------------------------------*/
#if defined(__aarch64__)

#ifndef cmpxchg16
  #define cmpxchg16(x,y,z,r,s) cmpxchg16_aarch64(x,y,z,r,s)
inline int cmpxchg16_aarch64(U64 *old1, U64 *old2, U64 new1, U64 new2, volatile void *ptr) {
/* returns 0 on success otherwise returns 1 */
    int result = 1;
    U64 expected1 = *old1;
    U64 expected2 = *old2;
    __asm __volatile(
        "ldaxp %[old1], %[old2], [%[ptr]]"
            : [old1] "+r" (*old1), [old2] "+r" (*old2)
            : [ptr] "r" (ptr));
    if ( expected1 == *old1 && expected2 == *old2 )
    {
        __asm __volatile(
            "stlxp %w[result], %[new1], %[new2], [%[ptr]]"
                : [result] "+r" (result)
                : [new1] "r" (new1), [new2] "r" (new2), [ptr] "r" (ptr)
                : "memory");
    }
    return result ;
}
#endif /* cmpxchg16 */

#endif /* define(__aarch64__) */



/*-------------------------------------------------------------------
 * Elbrus e2k
 *-------------------------------------------------------------------*/
#if defined(__e2k__) && defined(__LCC__)

#ifndef cmpxchg16
  #define  cmpxchg16(     x1, x2, y1, y2, z ) \
           cmpxchg16_e2k( x1, x2, y1, y2, z )

#define __E2K_LOCKFLAGS_SIZE    (512) /* 8 cache lines */
extern volatile char __e2k_lockflags[ /*512 bytes, 8 cache lines */ ];

extern int __cmpxchg16_e2k(volatile char *lockflag, U64 *old1, U64 *old2,
                           U64 new1, U64 new2, volatile void *ptr);

/* Elbrus e2k hash calculation code contributed by Leonid Yuriev */

__attribute__((__const__)) static inline volatile char *
__cmpxchg16_e2k_lockflag(U64 addr)
{
    U64 h;
    h  = addr * UINT64_C(0x00ADC4E8E91A095F /* 48,911,675,593,197,919 prime */);
    h += addr ^ UINT64_C(0x0060F9B69F8F4D17 /* 27,296,160,520,555,799 prime */);
    h ^= h >> 32;
    h &= __E2K_LOCKFLAGS_SIZE - 1 - 63 /* trim to the array size and align to cache line */;
    return __e2k_lockflags + h;
}

static inline int cmpxchg16_e2k(U64 *old1, U64 *old2, U64 new1, U64 new2,
                                volatile void *ptr)
{
    return __cmpxchg16_e2k(__cmpxchg16_e2k_lockflag((uintptr_t)ptr),
                           old1, old2, new1, new2, ptr);
}

#endif /* cmpxchg16 */

#endif /* defined(__e2k__) && defined(__LCC__) */

/*-------------------------------------------------------------------
 * C11_ATOMICS_AVAILABLE
 *-------------------------------------------------------------------*/
#if defined( C11_ATOMICS_AVAILABLE )

#if defined( cmpxchg1 ) && !defined( C11_ATOMICS_ASSISTS_NOT_PREFERRED )
  #undef cmpxchg1
#endif
#ifndef cmpxchg1
#define cmpxchg1(x,y,z) cmpxchg1_C11(x,y,z)
inline BYTE cmpxchg1_C11(BYTE *old, BYTE new, volatile void *ptr) {
/* returns 0 on success otherwise returns 1 */
            return __atomic_compare_exchange_n ((volatile BYTE *)ptr, old, new, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? 0 : 1;
        }
#endif

#if defined( cmpxchg4 ) && !defined( C11_ATOMICS_ASSISTS_NOT_PREFERRED )
  #undef cmpxchg4
#endif
#ifndef cmpxchg4
#define cmpxchg4(x,y,z) cmpxchg4_C11(x,y,z)
inline BYTE cmpxchg4_C11(U32 *old, U32 new, volatile void *ptr) {
/* returns zero on success otherwise returns 1 */
            return __atomic_compare_exchange_n ((volatile U32 *)ptr, old, new, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? 0 : 1;
        }
#endif

#if defined( cmpxchg8 ) && !defined( C11_ATOMICS_ASSISTS_NOT_PREFERRED )
  #undef cmpxchg8
#endif
#ifndef cmpxchg8
#define cmpxchg8(x,y,z) cmpxchg8_C11(x,y,z)

inline BYTE cmpxchg8_C11(U64 *old, U64 new, volatile void *ptr) {
/* returns 0 on success otherwise returns 1 */
    return __atomic_compare_exchange_n ((volatile U64 *)ptr, old, new, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? 0 : 1;
}
#endif /* cmpxchg8 */

/*-------------------------------------------------------------------
 * Elbrus e2k
 *-------------------------------------------------------------------*/
#if defined(__e2k__) && defined(__LCC__)

#define fetch_dw_noswap(_p) fetch_dw_e2k_noswap((_p))

inline U64 fetch_dw_e2k_noswap ( volatile const void* ptr )
{
    U64 value = __atomic_load_n((U64*)ptr, __ATOMIC_SEQ_CST);
    return value;
}

#define store_dw_noswap(_p, _v) store_dw_e2k_noswap( (_p), (_v))

inline void store_dw_e2k_noswap ( volatile void* ptr, U64 value )
{
    __atomic_store_n((U64*)ptr, value,__ATOMIC_SEQ_CST);
}

#endif /* defined(__e2k__) && defined(__LCC__) */

#endif /* defined( C11_ATOMICS_AVAILABLE ) */

#endif // !defined( _MSVC_ )

/*-------------------------------------------------------------------
 * Define the ASSIST_ macros
 *-------------------------------------------------------------------*/
#if defined(cmpxchg1)
 #define ASSIST_CMPXCHG1
#endif

#if defined(cmpxchg4)
 #define ASSIST_CMPXCHG4
#endif

#if defined(cmpxchg8)
 #define ASSIST_CMPXCHG8
#endif

#if defined(cmpxchg16)
 #define ASSIST_CMPXCHG16
#endif

#if defined(fetch_dw) || defined(fetch_dw_noswap)
 #define ASSIST_FETCH_DW
#endif

#if defined(store_dw) || defined(store_dw_noswap)
 #define ASSIST_STORE_DW
#endif

/*-------------------------------------------------------------------
 * OBTAIN/RELEASE_MAINLOCK is by default identical to
 * OBTAIN/RELEASE_MAINLOCK_UNCONDTIONAL but can be nullified in case
 * the required assists are present unless MAINLOCK_ALWAYS overriden.
 *-------------------------------------------------------------------*/

#if (! defined( MAINLOCK_ALWAYS )) \
    && defined( H_ATOMIC_OP )      \
    && defined( cmpxchg1 )         \
    && defined( cmpxchg4 )         \
    && defined( cmpxchg8 )         \
    && defined( cmpxchg16 )
  #undef  OBTAIN_MAINLOCK
  #define OBTAIN_MAINLOCK(_regs)
  #undef  RELEASE_MAINLOCK
  #define RELEASE_MAINLOCK(_regs)
#endif

/*-------------------------------------------------------------------
 * fetch_hw_noswap and fetch_hw
 *-------------------------------------------------------------------*/
#if !defined(fetch_hw_noswap)
  #if defined(fetch_hw)
    #define fetch_hw_noswap(_p) CSWAP16(fetch_hw((_p)))
  #else
    inline U16 fetch_hw_noswap(const void *ptr) {
      U16 value;
      memcpy(&value, (BYTE *)ptr, 2);
      return value;
    }
  #endif
#endif
#if !defined(fetch_hw)
  #define fetch_hw(_p) CSWAP16(fetch_hw_noswap((_p)))
#endif

/*-------------------------------------------------------------------
 * store_hw_noswap and store_hw
 *-------------------------------------------------------------------*/
#if !defined(store_hw_noswap)
  #if defined(store_hw)
    #define store_hw_noswap(_p, _v) store_hw((_p), CSWAP16(_v))
  #else
    inline void store_hw_noswap(void *ptr, U16 value) {
      memcpy((BYTE *)ptr, (BYTE *)&value, 2);
    }
  #endif
#endif
#if !defined(store_hw)
  #define store_hw(_p, _v) store_hw_noswap((_p), CSWAP16((_v)))
#endif

/*-------------------------------------------------------------------
 * fetch_fw_noswap and fetch_fw
 *-------------------------------------------------------------------*/
#if !defined(fetch_fw_noswap)
  #if defined(fetch_fw)
    #define fetch_fw_noswap(_p) CSWAP32(fetch_fw((_p)))
  #else
    inline U32 fetch_fw_noswap(const void *ptr) {
      U32 value;
      memcpy(&value, (BYTE *)ptr, 4);
      return value;
    }
  #endif
#endif
#if !defined(fetch_fw)
  #define fetch_fw(_p) CSWAP32(fetch_fw_noswap((_p)))
#endif

/*-------------------------------------------------------------------
 * store_fw_noswap and store_fw
 *-------------------------------------------------------------------*/
#if !defined(store_fw_noswap)
  #if defined(store_fw)
    #define store_fw_noswap(_p, _v) store_fw((_p), CSWAP32(_v))
  #else
    inline void store_fw_noswap(void *ptr, U32 value) {
      memcpy((BYTE *)ptr, (BYTE *)&value, 4);
    }
  #endif
#endif
#if !defined(store_fw)
  #define store_fw(_p, _v) store_fw_noswap((_p), CSWAP32((_v)))
#endif

/*-------------------------------------------------------------------
 * fetch_f3_noswap and fetch_f3
 *-------------------------------------------------------------------*/
#if !defined(fetch_f3_noswap)
  #if defined(fetch_f3)
    #define fetch_f3_noswap(_p) CSWAP32(fetch_f3((_p)))
  #else
    inline U32 fetch_f3_noswap(const void *ptr) {
      U32 value;
      memcpy(((BYTE *)&value), (BYTE *)ptr, 3);
      value <<= 8;
      return value;
    }
  #endif
#endif
#if !defined(fetch_f3)
  #define fetch_f3(_p) CSWAP32(fetch_f3_noswap((_p)))
#endif

/*-------------------------------------------------------------------
 * store_f3_noswap and store_f3
 *-------------------------------------------------------------------*/
#if !defined(store_f3_noswap)
  #if defined(store_f3)
    #define store_f3_noswap(_p, _v) store_f3((_p), CSWAP32(_v))
  #else
    inline void store_f3_noswap(void *ptr, U32 value) {
      value >>= 8;
      memcpy((BYTE *)ptr, ((BYTE *)&value), 3);
    }
  #endif
#endif
#if !defined(store_f3)
  #define store_f3(_p, _v) store_f3_noswap((_p), CSWAP32((_v)))
#endif

/*-------------------------------------------------------------------
 * fetch_dw_noswap and fetch_dw
 *-------------------------------------------------------------------*/
#if !defined(fetch_dw_noswap)
  #if defined(fetch_dw)
    #define fetch_dw_noswap(_p) CSWAP64(fetch_dw((_p)))
  #else
    inline U64 fetch_dw_noswap(const void *ptr) {
      U64 value;
      memcpy(&value, (BYTE *)ptr, 8);
      return value;
    }
  #endif
#endif
#if !defined(fetch_dw)
  #define fetch_dw(_p) CSWAP64(fetch_dw_noswap((_p)))
#endif

/*-------------------------------------------------------------------
 * store_dw_noswap and store_dw
 *-------------------------------------------------------------------*/
#if !defined(store_dw_noswap)
  #if defined(store_dw)
    #define store_dw_noswap(_p, _v) store_dw((_p), CSWAP64(_v))
  #else
    inline void store_dw_noswap(void *ptr, U64 value) {
      memcpy((BYTE *)ptr, (BYTE *)&value, 8);
    }
  #endif
#endif
#if !defined(store_dw)
  #define store_dw(_p, _v) store_dw_noswap((_p), CSWAP64((_v)))
#endif

/*-------------------------------------------------------------------
 * cmpxchg1
 *-------------------------------------------------------------------*/
#ifndef cmpxchg1
inline BYTE cmpxchg1(BYTE *old, BYTE new, volatile void *ptr) {
 /* returns 0 on success otherwise returns 1 */
 BYTE code;
 if (*old == *(BYTE *)ptr)
 {
     *(BYTE *)ptr = new;
     code = 0;
 }
 else
 {
     *old = *(BYTE *)ptr;
     code = 1;
 }
 return code;
}
#endif

/*-------------------------------------------------------------------
 * cmpxchg4
 *-------------------------------------------------------------------*/
#ifndef cmpxchg4
inline BYTE cmpxchg4(U32 *old, U32 new, volatile void *ptr) {
 /* values passed in guest big-endian format */
 /* returns 0 on success otherwise returns 1 */
 BYTE code;
 if (*old == *(U32 *)ptr)
 {
     *(U32 *)ptr = new;
     code = 0;
 }
 else
 {
     *old = *(U32 *)ptr;
     code = 1;
 }
 return code;
}
#endif

/*-------------------------------------------------------------------
 * cmpxchg8
 *-------------------------------------------------------------------*/
#ifndef cmpxchg8
inline BYTE cmpxchg8(U64 *old, U64 new, volatile void *ptr) {
 /* values passed in guest big-endian format */
 /* returns 0 on success otherwise returns 1 */
 BYTE code;
 if (*old == *(U64 *)ptr)
 {
     *(U64 *)ptr = new;
     code = 0;
 }
 else
 {
     *old = *(U64 *)ptr;
     code = 1;
 }
 return code;
}
#endif

/*-------------------------------------------------------------------
 * cmpxchg16
 *-------------------------------------------------------------------*/
#ifndef cmpxchg16
inline int cmpxchg16(U64 *old1, U64 *old2, U64 new1, U64 new2, volatile void *ptr) {
 /* values passed in guest big-endian format */
 /* returns 0 on success otherwise returns 1 */
 int code;
 if (*old1 == *(U64 *)ptr && *old2 == *((U64 *)ptr + 1))
 {
     *(U64 *)ptr = new1;
     *((U64 *)ptr + 1) = new2;
     code = 0;
 }
 else
 {
     *old1 = *((U64 *)ptr);
     *old2 = *((U64 *)ptr + 1);
     code = 1;
 }
 return code;
}
#endif

/*-------------------------------------------------------------------*/
/*                      Hardware Sync                                */
/*-------------------------------------------------------------------*/
#if defined( OPTION_HARDWARE_SYNC_ALL ) || defined( OPTION_HARDWARE_SYNC_BCR_ONLY )
  #if defined( _MSVC_ )
    #pragma intrinsic              ( _mm_mfence )
    #define HARDWARE_SYNC()          _mm_mfence()
  #else // gcc presumed
    #define HARDWARE_SYNC()         __sync_synchronize()
  #endif // (which compiler)
#endif

#endif /* _MACHDEP_H */
