/* VSTORE.C     (C) Copyright Roger Bowler, 2000-2012                */
/*              (C) and others 2013-2023                             */
/*                  Virtual Storage Functions                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*                  Virtual Storage Functions                        */
/*                                                                   */
/*  This module contains various functions which store, fetch, and   */
/*  copy values to, from, or between virtual storage locations.      */
/*                                                                   */
/*  Functions provided in this module are:                           */
/*                                                                   */
/*  vstoreb      Store a single byte into virtual storage            */
/*  vstore2      Store a two-byte integer into virtual storage       */
/*  vstore4      Store a four-byte integer into virtual storage      */
/*  vstore8      Store an eight-byte integer into virtual storage    */
/*  vstorec      Store 1 to 256 characters into virtual storage      */
/*                                                                   */
/*  wstoreX      Address-wrapping version of the above               */
/*                                                                   */
/*  vfetchb      Fetch a single byte from virtual storage            */
/*  vfetch2      Fetch a two-byte integer from virtual storage       */
/*  vfetch4      Fetch a four-byte integer from virtual storage      */
/*  vfetch8      Fetch an eight-byte integer from virtual storage    */
/*  vfetchc      Fetch 1 to 256 characters from virtual storage      */
/*                                                                   */
/*  wfetchX      Address-wrapping version of the above               */
/*                                                                   */
/*  instfetch    Fetch instruction from virtual storage              */
/*                                                                   */
/*  validate_operand   Validate addressing, protection, translation  */
/*  wvalidate_opend    Address wrapping version of the above         */
/*                                                                   */
/*  move_chars   Move characters using specified keys and addrspaces */
/*  wmove_chars  Address-wrapping version of the above               */
/*                                                                   */
/*  move_charx   Move characters with optional specifications        */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* 2019-12-05  Bob Wood                                              */
/* Code modified to pass the correct length when needed for MADDR.   */
/* This is needed to support transaction execution mode.  Note that  */
/* for most of these instructions, no change is required.  Since     */
/* transaction mode operates at the cache line level (per the POP)   */
/* it is only an issue if the actual length spans a cache line       */
/* boundary but not a page boundary.                                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _VSTORE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

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

extern inline void ARCH_DEP( vstore2_full )( U16 value, VADR addr, int arn, REGS* regs );
extern inline void ARCH_DEP( vstore4_full )( U32 value, VADR addr, int arn, REGS* regs );
extern inline void ARCH_DEP( vstore8_full )( U64 value, VADR addr, int arn, REGS* regs );

extern inline U16  ARCH_DEP( vfetch2_full )( VADR addr, int arn, REGS* regs );
extern inline U32  ARCH_DEP( vfetch4_full )( VADR addr, int arn, REGS* regs );
extern inline U64  ARCH_DEP( vfetch8_full )( VADR addr, int arn, REGS* regs );

extern inline void ARCH_DEP( vstoreb )( BYTE value, VADR addr, int arn, REGS* regs );
extern inline void ARCH_DEP( vstore2 )( U16  value, VADR addr, int arn, REGS* regs );
extern inline void ARCH_DEP( vstore4 )( U32  value, VADR addr, int arn, REGS* regs );
extern inline void ARCH_DEP( vstore8 )( U64  value, VADR addr, int arn, REGS* regs );

extern inline BYTE ARCH_DEP( vfetchb )( VADR addr, int arn, REGS* regs );
extern inline U16  ARCH_DEP( vfetch2 )( VADR addr, int arn, REGS* regs );
extern inline U32  ARCH_DEP( vfetch4 )( VADR addr, int arn, REGS* regs );
extern inline U64  ARCH_DEP( vfetch8 )( VADR addr, int arn, REGS* regs );

extern inline BYTE* ARCH_DEP( instfetch )( REGS* regs, int exec );

// (needed by dyncrypt.c)
INL_DLL_EXPORT inline void ARCH_DEP( vstorec )( const void* src, BYTE len, VADR addr, int arn, REGS* regs );
INL_DLL_EXPORT inline void ARCH_DEP( vfetchc )( void* dest, BYTE len, VADR addr, int arn, REGS* regs );
INL_DLL_EXPORT inline void ARCH_DEP( validate_operand )( VADR addr, int arn, int len, int acctype, REGS* regs );

extern inline void ARCH_DEP( move_chars )( VADR addr1, int arn1, BYTE key1,
                                           VADR addr2, int arn2, BYTE key2,
                                           int len, REGS* regs );

#if defined( FEATURE_027_MVCOS_FACILITY )
extern inline void ARCH_DEP( move_charx )( VADR addr1, int space1, BYTE key1,
                                           VADR addr2, int space2, BYTE key2,
                                           int len, REGS* regs );
#endif

#if defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
extern inline void ARCH_DEP( move_chars_rl )( VADR addr1, int arn1, BYTE key1,
                                              VADR addr2, int arn2, BYTE key2,
                                              int len, REGS* regs );
#endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "vstore.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "vstore.c"
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

extern inline void concpy    ( REGS* regs, void* d, void* s, int n );
#if defined( _FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
extern inline void concpy_rl ( REGS* regs, void* d, void* s, int n );
#endif

#endif // !defined(_GEN_ARCH)
