/* INLINE.H     (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2021                             */
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

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled *BEFORE* the FIRST arch is built  */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

#ifndef _INLINE_H
#define _INLINE_H

extern inline int  add_logical       ( U32 *result, U32 op1, U32 op2 );
extern inline int  add_signed        ( U32* result, U32 op1, U32 op2 );

extern inline int  sub_logical       ( U32 *result, U32 op1, U32 op2 );
extern inline int  sub_signed        ( U32* result, U32 op1, U32 op2 );

extern inline void mul_signed        ( U32* resulthi, U32* resultlo, U32 op1, U32 op2 );
extern inline void mul_unsigned_long ( U64* resulthi, U64* resultlo, U64 op1, U64 op2 );
extern inline void mul_signed_long   ( S64* resulthi, S64* resultlo, S64 op1, S64 op2 );

extern inline int add_logical_long   ( U64 *result, U64 op1, U64 op2 );
extern inline int add_signed_long    ( U64* result, U64 op1, U64 op2 );

extern inline int sub_logical_long   ( U64 *result, U64 op1, U64 op2 );
extern inline int sub_signed_long    ( U64* result, U64 op1, U64 op2 );

extern inline int mult_logical_long  ( U64* high, U64* lo, U64 md, U64 mr );
extern inline int div_signed         ( U32* rem, U32* quot, U32 high, U32 lo, U32 d );
extern inline int div_logical_long   ( U64* rem, U64* quot, U64 high, U64 lo, U64 d );

#endif // defined( _INLINE_H )

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

extern inline int  ARCH_DEP( is_fetch_protected )       ( VADR addr, BYTE skey, BYTE akey, REGS *regs );
extern inline int  ARCH_DEP( is_low_address_protected ) ( VADR addr, REGS *regs );
extern inline int  ARCH_DEP( is_store_protected )       ( VADR addr, BYTE skey, BYTE akey, REGS *regs );
extern inline U64  ARCH_DEP( fetch_doubleword_absolute )( RADR addr, REGS *regs );
extern inline U32  ARCH_DEP( fetch_fullword_absolute )  ( RADR addr, REGS *regs );
extern inline U16  ARCH_DEP( fetch_halfword_absolute )  ( RADR addr, REGS *regs );
extern inline void ARCH_DEP( store_doubleword_absolute )( U64 value, RADR addr, REGS *regs );
extern inline void ARCH_DEP( store_fullword_absolute )  ( U32 value, RADR addr, REGS *regs );

#if defined( INLINE_STORE_FETCH_ADDR_CHECK )
extern inline BYTE* ARCH_DEP( fetch_main_absolute )( RADR addr, REGS *regs, int len );
#else
extern inline BYTE* ARCH_DEP( fetch_main_absolute )( RADR addr, REGS *regs );
#endif

/*-------------------------------------------------------------------*/
/* The following function must be declared separately for each       */
/* build architecture because of the dat.c "logical_to_main_l"       */
/* function's need (for SIE purposes) to apply prefixing for an      */
/* architecture other than the current build architecture.           */
/*-------------------------------------------------------------------*/

extern inline RADR s370_apply_prefixing( RADR addr, RADR px );
extern inline RADR s390_apply_prefixing( RADR addr, RADR px );
extern inline RADR z900_apply_prefixing( RADR addr, RADR px );

/*-------------------------------------------------------------------*/
/*       Automatically #include dat.h and vstore.h headers           */
/*-------------------------------------------------------------------*/

#include "dat.h"
#include "vstore.h"

/* end of INLINE.H */
