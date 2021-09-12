/* MACHDEP.C    (C) Copyright Greg Smith, 2001-2012                  */
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

#include "hstdinc.h"

#define _MACHDEP_C_
#define _HENGINE_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*  PROGRAMMING NOTE: since the "machdep.h" header file has already  */
/*  been processed by the compiler/preprocessor, in order to ensure  */
/*  the below logic, when preprocessed by the compiler, takes the    */
/*  EXACT SAME PATH through the various preprocessor #if tests, we   */
/*  must ensure we start from the EXACT SAME STATE it started from.  */
/*-------------------------------------------------------------------*/

#undef GEN_MSC_ASSISTS

#undef MSC_X86_32BIT
#undef MSC_X86_AMD64
#undef MSC_X86_64BIT
#undef MSC_X86_IA64

#undef cmpxchg1
#undef cmpxchg4
#undef cmpxchg8
#undef cmpxchg16

#undef _ext_ia32
#undef _ext_amd64
#undef _ext_ppc

#undef ASSIST_CMPXCHG1
#undef ASSIST_CMPXCHG4
#undef ASSIST_CMPXCHG8
#undef ASSIST_CMPXCHG16
#undef ASSIST_FETCH_DW
#undef ASSIST_STORE_DW

#undef fetch_hw_noswap
#undef fetch_hw

#undef store_hw_noswap
#undef store_hw

#undef fetch_fw_noswap
#undef fetch_fw

#undef store_fw_noswap
#undef store_fw

#undef fetch_f3_noswap
#undef fetch_f3

#undef store_f3_noswap
#undef store_f3

#undef fetch_dw_noswap
#undef fetch_dw

#undef store_dw_noswap
#undef store_dw

/*-------------------------------------------------------------------*/
/*  PROGRAMMING NOTE: the below code should be exactly the same as   */
/*  the code in "machdep.h" except that where machdep.h defines the  */
/*  the inline function, we simply declare it as being extern. That  */
/*  should be the ONLY difference.                                   */
/*-------------------------------------------------------------------*/

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

    extern inline BYTE __fastcall cmpxchg8_x86 ( U64* old, U64 unew, volatile void* ptr );
    extern inline BYTE __fastcall cmpxchg4_x86 ( U32* old, U32 unew, volatile void* ptr );

    // (must follow cmpxchg4 since it uses it)
    extern inline BYTE __fastcall cmpxchg1_x86 ( BYTE* old, BYTE unew, volatile void* ptr );

    #if !defined( MSC_X86_32BIT ) // 64-bit only!

      #pragma intrinsic ( _InterlockedCompareExchange128 )
      #define cmpxchg16(  x, y, z, r, s  ) cmpxchg16_x86( x, y, z, r, s )
      extern inline int cmpxchg16_x86 ( U64* old1, U64* old2, U64 new1, U64 new2, volatile void* ptr );

    #endif // !defined( MSC_X86_32BIT ) // 64-bit only!

    #if defined( MSC_X86_32BIT )

      #define fetch_dw_noswap(_p) fetch_dw_x86_noswap((_p))
      // (must follow cmpxchg8 since it uses it)
      extern inline U64 __fastcall fetch_dw_x86_noswap ( volatile const void* ptr );

      #define store_dw_noswap(_p, _v) store_dw_x86_noswap( (_p), (_v))
      // (must follow cmpxchg8 since it uses it)
      extern inline void __fastcall store_dw_x86_noswap ( volatile void* ptr, U64 value );
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

    extern inline int __fastcall cmpxchg16_x86 ( U64* old1, U64* old2,
                                          U64  new1, U64  new2,
                                          volatile void* ptr );

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
extern inline BYTE cmpxchg1_i686(BYTE *old, BYTE new, void *ptr);

#define cmpxchg4(x,y,z) cmpxchg4_i686(x,y,z)
extern inline BYTE cmpxchg4_i686(U32 *old, U32 new, void *ptr);

#define cmpxchg8(x,y,z) cmpxchg8_i686(x,y,z)
extern inline BYTE cmpxchg8_i686(U64 *old, U64 new, void *ptr);

#define fetch_dw_noswap(x) fetch_dw_i686_noswap(x)
extern inline U64 fetch_dw_i686_noswap(const void *ptr);

#define store_dw_noswap(x,y) store_dw_i686_noswap(x,y)
extern inline void store_dw_i686_noswap(void *ptr, U64 value);

#endif /* defined(_ext_ia32) */

/*-------------------------------------------------------------------
 * AMD64
 *-------------------------------------------------------------------*/
#if defined(_ext_amd64)

#define cmpxchg1(x,y,z) cmpxchg1_amd64(x,y,z)
extern inline BYTE cmpxchg1_amd64(BYTE *old, BYTE new, void *ptr);

#define cmpxchg4(x,y,z) cmpxchg4_amd64(x,y,z)
extern inline BYTE cmpxchg4_amd64(U32 *old, U32 new, void *ptr);

#define cmpxchg8(x,y,z) cmpxchg8_amd64(x,y,z)
extern inline BYTE cmpxchg8_amd64(U64 *old, U64 new, void *ptr);

#define cmpxchg16(x,y,z,r,s) cmpxchg16_amd64(x,y,z,r,s)
extern inline int cmpxchg16_amd64(U64 *old1, U64 *old2, U64 new1, U64 new2, volatile void *ptr);

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

extern inline U64
__cmpxchg_u64(volatile U64 *p, U64 old, U64 new);

#define cmpxchg8(x,y,z) cmpxchg8_ppc(x,y,z)
extern inline BYTE cmpxchg8_ppc(U64 *old, U64 new, void *ptr);

#endif // defined( __64BIT__ )

extern inline U32
__cmpxchg_u32(volatile U32 *p, U32 old, U32 new);

#define cmpxchg4(x,y,z) cmpxchg4_ppc(x,y,z)
extern inline BYTE cmpxchg4_ppc(U32 *old, U32 new, void *ptr);

#define cmpxchg1(x,y,z) cmpxchg1_ppc(x,y,z)
extern inline BYTE cmpxchg1_ppc(BYTE *old, BYTE new, void *ptr);

#endif /* defined(_ext_ppc) */

/*-------------------------------------------------------------------
 * ARM aarch64 (like the Raspberry Pi 4)
 *-------------------------------------------------------------------*/
#if defined(__aarch64__)

#ifndef cmpxchg16
  #define cmpxchg16(x,y,z,r,s) cmpxchg16_aarch64(x,y,z,r,s)
extern inline int cmpxchg16_aarch64(U64 *old1, U64 *old2, U64 new1, U64 new2, volatile void *ptr);
#endif /* cmpxchg16 */

#endif /* define(__aarch64__) */

/*-------------------------------------------------------------------
 * C11_ATOMICS_AVAILABLE
 *-------------------------------------------------------------------*/
#if defined( C11_ATOMICS_AVAILABLE )

#if defined( cmpxchg1 ) && !defined( C11_ATOMICS_ASSISTS_NOT_PREFERRED )
  #undef cmpxchg1
#endif
#ifndef cmpxchg1
#define cmpxchg1(x,y,z) cmpxchg1_C11(x,y,z)
extern inline BYTE cmpxchg1_C11(BYTE *old, BYTE new, volatile void *ptr);
#endif

#if defined( cmpxchg4 ) && !defined( C11_ATOMICS_ASSISTS_NOT_PREFERRED )
  #undef cmpxchg4
#endif
#ifndef cmpxchg4
#define cmpxchg4(x,y,z) cmpxchg4_C11(x,y,z)
extern inline BYTE cmpxchg4_C11(U32 *old, U32 new, volatile void *ptr);
#endif

#if defined( cmpxchg8 ) && !defined( C11_ATOMICS_ASSISTS_NOT_PREFERRED )
  #undef cmpxchg8
#endif
#ifndef cmpxchg8
#define cmpxchg8(x,y,z) cmpxchg8_C11(x,y,z)
extern inline BYTE cmpxchg8_C11(U64 *old, U64 new, volatile void *ptr);
#endif /* cmpxchg8 */

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
    extern inline U16 fetch_hw_noswap(const void *ptr);
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
    extern inline void store_hw_noswap(void *ptr, U16 value);
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
    extern inline U32 fetch_fw_noswap(const void *ptr);
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
    extern inline void store_fw_noswap(void *ptr, U32 value);
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
    extern inline U32 fetch_f3_noswap(const void *ptr);
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
    extern inline void store_f3_noswap(void *ptr, U32 value);
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
    extern inline U64 fetch_dw_noswap(const void *ptr);
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
    extern inline void store_dw_noswap(void *ptr, U64 value);
  #endif
#endif
#if !defined(store_dw)
  #define store_dw(_p, _v) store_dw_noswap((_p), CSWAP64((_v)))
#endif

/*-------------------------------------------------------------------
 * cmpxchg1
 *-------------------------------------------------------------------*/
#ifndef cmpxchg1
extern inline BYTE cmpxchg1(BYTE *old, BYTE new, volatile void *ptr);
#endif

/*-------------------------------------------------------------------
 * cmpxchg4
 *-------------------------------------------------------------------*/
#ifndef cmpxchg4
extern inline BYTE cmpxchg4(U32 *old, U32 new, volatile void *ptr);
#endif

/*-------------------------------------------------------------------
 * cmpxchg8
 *-------------------------------------------------------------------*/
#ifndef cmpxchg8
extern inline BYTE cmpxchg8(U64 *old, U64 new, volatile void *ptr);
#endif

/*-------------------------------------------------------------------
 * cmpxchg16
 *-------------------------------------------------------------------*/
#ifndef cmpxchg16
extern inline int cmpxchg16(U64 *old1, U64 *old2, U64 new1, U64 new2, volatile void *ptr);
#endif

/* end of _MACHDEP_C */
