/* INLINE.C     (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2022                             */
/*              Inline function definitions                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/* Storage protection override fix               Jan Jaeger 31/08/00 */
/* ESAME low-address protection          v208d Roger Bowler 20/01/01 */
/* ESAME subspace replacement            v208e Roger Bowler 27/01/01 */
/* Multiply/Divide Logical instructions         Vic Cross 13/02/2001 */
/* PER 1 GRA                                           Fish Jan 2022 */

#include "hstdinc.h"

#define _INLINE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/*                    Miscellaneous functions                        */
/*-------------------------------------------------------------------*/

extern inline bool ARCH_DEP( is_fetch_protected )       ( VADR addr, BYTE skey, BYTE akey, REGS* regs );
extern inline bool ARCH_DEP( is_store_protected )       ( VADR addr, BYTE skey, BYTE akey, REGS* regs );
extern inline bool ARCH_DEP( is_low_address_protected ) ( VADR addr, REGS *regs );

extern inline U64  ARCH_DEP( fetch_doubleword_absolute )( RADR addr, REGS* regs );
extern inline U32  ARCH_DEP( fetch_fullword_absolute )  ( RADR addr, REGS* regs );
extern inline U16  ARCH_DEP( fetch_halfword_absolute )  ( RADR addr, REGS* regs );

extern inline void ARCH_DEP( store_doubleword_absolute )( U64 value, RADR addr, REGS* regs );
extern inline void ARCH_DEP( store_fullword_absolute )  ( U32 value, RADR addr, REGS* regs );

#if defined( INLINE_STORE_FETCH_ADDR_CHECK )
  extern inline BYTE* ARCH_DEP( fetch_main_absolute )( RADR addr, REGS* regs, int len );
#else
  extern inline BYTE* ARCH_DEP( fetch_main_absolute )( RADR addr, REGS* regs );
#endif

extern inline void ARCH_DEP( per1_gra )( REGS* regs );
extern inline bool ARCH_DEP( is_per3_event_suppressed )( REGS* regs, U32 cr9_per_event );
extern inline void ARCH_DEP( per3_zero )( REGS* regs );
extern inline void ARCH_DEP( per3_zero_check )( REGS* regs, int r1 );
extern inline void ARCH_DEP( per3_zero_check2 )( REGS* regs, int r1, int r2 );
extern inline void ARCH_DEP( per3_zero_lcheck )( REGS* regs, int r1, int l1 );
extern inline void ARCH_DEP( per3_zero_lcheck2 )( REGS* regs, int r1, int l1, int r2, int l2 );
extern inline void ARCH_DEP( per3_zero_l24check )( REGS* regs, int r1, int l1 );
extern inline void ARCH_DEP( per3_zero_l24check2 )( REGS* regs, int r1, int l1, int r2, int l2 );
extern inline void ARCH_DEP( per3_zero_xcheck )( REGS* regs, int b1 );
extern inline void ARCH_DEP( per3_zero_xcheck2 )( REGS* regs, int x2, int b2 );

extern inline void ARCH_DEP( FPC_check )( REGS* regs, U32 fpc );

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "inline.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "inline.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                    Arithmetic functions                           */
/*-------------------------------------------------------------------*/

extern inline int   add_logical       ( U32* result, U32 op1, U32 op2 );
extern inline int   add_signed        ( U32* result, U32 op1, U32 op2 );

extern inline int   sub_logical       ( U32* result, U32 op1, U32 op2 );
extern inline int   sub_signed        ( U32* result, U32 op1, U32 op2 );

extern inline void  mul_signed        ( U32* resulthi, U32* resultlo, U32 op1, U32 op2 );
extern inline void  mul_unsigned_long ( U64* resulthi, U64* resultlo, U64 op1, U64 op2 );
extern inline void  mul_signed_long   ( S64* resulthi, S64* resultlo, S64 op1, S64 op2 );

extern inline int   add_logical_long  ( U64* result, U64 op1, U64 op2 );
extern inline int   add_signed_long   ( U64* result, U64 op1, U64 op2 );

extern inline int   sub_logical_long  ( U64* result, U64 op1, U64 op2 );
extern inline int   sub_signed_long   ( U64* result, U64 op1, U64 op2 );

extern inline int   mult_logical_long ( U64* high, U64* lo, U64 md, U64 mr );
extern inline int   div_signed        ( U32* rem, U32* quot, U32 high, U32 lo, U32 d );
extern inline int   div_logical_long  ( U64* rem, U64* quot, U64 high, U64 lo, U64 d );
extern inline QW    bswap_128         ( QW input );

#endif /*!defined( _GEN_ARCH )*/
