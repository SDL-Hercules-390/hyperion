/* VSTORE.H     (C) Copyright Roger Bowler, 2000-2012                */
/*              (C) and others 2013-2021                             */
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

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*   that use this header file.                                      */
/*-------------------------------------------------------------------*/

#define s370_wstorec(_src, _len, _addr, _arn, _regs) \
        s370_vstorec((_src), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstoreb(_value, _addr, _arn, _regs) \
        s370_vstoreb((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstore2(_value, _addr, _arn, _regs) \
        s370_vstore2((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstore4(_value, _addr, _arn, _regs) \
        s370_vstore4((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstore8(_value, _addr, _arn, _regs) \
        s370_vstore8((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetchc(_dest, _len, _addr, _arn, _regs) \
        s370_vfetchc((_dest), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetchb(_addr, _arn, _regs) \
        s370_vfetchb(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetch2(_addr, _arn, _regs) \
        s370_vfetch2(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetch4(_addr, _arn, _regs) \
        s370_vfetch4(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetch8(_addr, _arn, _regs) \
        s370_vfetch8(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wmove_chars(_addr1, _arn1, _key1, _addr2, _arn2, _key2, _len, _regs) \
        s370_move_chars(((_addr1) & ADDRESS_MAXWRAP((_regs))), (_arn1), (_key1), \
                        ((_addr2) & ADDRESS_MAXWRAP((_regs))), (_arn2), (_key2), (_len), (_regs))
#define s370_wvalidate_operand(_addr, _arn, _len, _acctype, _regs) \
        s370_validate_operand(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_len), (_acctype), (_regs))

/*-------------------------------------------------------------------*/

#define s390_wstorec(_src, _len, _addr, _arn, _regs) \
        s390_vstorec((_src), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstoreb(_value, _addr, _arn, _regs) \
        s390_vstoreb((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstore2(_value, _addr, _arn, _regs) \
        s390_vstore2((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstore4(_value, _addr, _arn, _regs) \
        s390_vstore4((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstore8(_value, _addr, _arn, _regs) \
        s390_vstore8((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetchc(_dest, _len, _addr, _arn, _regs) \
        s390_vfetchc((_dest), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetchb(_addr, _arn, _regs) \
        s390_vfetchb(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetch2(_addr, _arn, _regs) \
        s390_vfetch2(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetch4(_addr, _arn, _regs) \
        s390_vfetch4(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetch8(_addr, _arn, _regs) \
        s390_vfetch8(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wmove_chars(_addr1, _arn1, _key1, _addr2, _arn2, _key2, _len, _regs) \
        s390_move_chars(((_addr1) & ADDRESS_MAXWRAP((_regs))), (_arn1), (_key1), \
                        ((_addr2) & ADDRESS_MAXWRAP((_regs))), (_arn2), (_key2), (_len), (_regs))
#define s390_wvalidate_operand(_addr, _arn, _len, _acctype, _regs) \
        s390_validate_operand(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_len), (_acctype), (_regs))

/*-------------------------------------------------------------------*/

#define z900_wstorec(_src, _len, _addr, _arn, _regs) \
        z900_vstorec((_src), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstoreb(_value, _addr, _arn, _regs) \
        z900_vstoreb((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstore2(_value, _addr, _arn, _regs) \
        z900_vstore2((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstore4(_value, _addr, _arn, _regs) \
        z900_vstore4((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstore8(_value, _addr, _arn, _regs) \
        z900_vstore8((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetchc(_dest, _len, _addr, _arn, _regs) \
        z900_vfetchc((_dest), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetchb(_addr, _arn, _regs) \
        z900_vfetchb(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetch2(_addr, _arn, _regs) \
        z900_vfetch2(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetch4(_addr, _arn, _regs) \
        z900_vfetch4(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetch8(_addr, _arn, _regs) \
        z900_vfetch8(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wmove_chars(_addr1, _arn1, _key1, _addr2, _arn2, _key2, _len, _regs) \
        z900_move_chars(((_addr1) & ADDRESS_MAXWRAP((_regs))), (_arn1), (_key1), \
                        ((_addr2) & ADDRESS_MAXWRAP((_regs))), (_arn2), (_key2), (_len), (_regs))
#define z900_wvalidate_operand(_addr, _arn, _len, _acctype, _regs) \
        z900_validate_operand(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_len), (_acctype), (_regs))

/*-------------------------------------------------------------------*/

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
VSTORE_DLL_IMPORT inline void ARCH_DEP( vstorec )( const void* src, BYTE len, VADR addr, int arn, REGS* regs );
VSTORE_DLL_IMPORT inline void ARCH_DEP( vfetchc )( void* dest, BYTE len, VADR addr, int arn, REGS* regs );
VSTORE_DLL_IMPORT inline void ARCH_DEP( validate_operand )( VADR addr, int arn, int len, int acctype, REGS* regs );

/*-------------------------------------------------------------------*/

extern inline void ARCH_DEP( move_chars )( VADR addr1, int arn1, BYTE key1,
                                           VADR addr2, int arn2, BYTE key2,
                                           int len, REGS* regs );

/*-------------------------------------------------------------------*/

#if defined( FEATURE_027_MVCOS_FACILITY )
extern inline void ARCH_DEP( move_charx )( VADR addr1, int space1, BYTE key1,
                                           VADR addr2, int space2, BYTE key2,
                                           int len, REGS* regs );
#endif /* defined( FEATURE_027_MVCOS_FACILITY ) */

/*-------------------------------------------------------------------*/

#if defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
extern inline void ARCH_DEP( move_chars_rl )( VADR addr1, int arn1, BYTE key1,
                                              VADR addr2, int arn2, BYTE key2,
                                              int len, REGS* regs );
#endif /* defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 ) */

/*-------------------------------------------------------------------*/

#ifndef _VSTORE_CONCPY
#define _VSTORE_CONCPY

  extern inline void concpy(REGS *regs, void *d, void *s, int n);
  #if defined( _FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
  extern inline void concpy_rl( REGS* regs, void* d, void* s, int n );
  #endif

#endif

/*-------------------------------------------------------------------*/
