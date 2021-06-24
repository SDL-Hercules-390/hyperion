/* DAT.H        (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2021                             */
/*              ESA/390 Dynamic Address Translation                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*   that use this header file.                                      */
/*-------------------------------------------------------------------*/

extern inline BYTE* ARCH_DEP( maddr_l )( VADR addr, size_t len, const int arn, REGS* regs, const int acctype, const BYTE akey );

#if defined( FEATURE_DUAL_ADDRESS_SPACE )
extern inline int ARCH_DEP( authorize_asn )( U16 ax, U32 aste[], int atemask, REGS* regs );
#endif

extern inline void ARCH_DEP( purge_tlb )( REGS* regs );
extern inline void ARCH_DEP( purge_tlb_all )();
extern inline void ARCH_DEP( purge_tlbe_all )( RADR pfra );

#if defined( FEATURE_ACCESS_REGISTERS )
extern inline void ARCH_DEP( purge_alb )( REGS* regs );
extern inline void ARCH_DEP( purge_alb_all )();
#endif

/*-------------------------------------------------------------------*/

#if defined( FEATURE_ACCESS_REGISTERS )
U16  ARCH_DEP( translate_alet )( U32 alet, U16 eax, int acctype, REGS* regs, U32* asteo, U32 aste[] );
#endif

#if defined( FEATURE_DUAL_ADDRESS_SPACE )
U16 ARCH_DEP( translate_asn )( U16 asn, REGS* regs, U32* asteo, U32 aste[] );
#endif

int  ARCH_DEP( translate_addr  )( VADR vaddr, int arn, REGS* regs, int acctype );
void ARCH_DEP( purge_tlbe      )( REGS* regs, RADR pfra );
void ARCH_DEP( invalidate_tlb  )( REGS* regs, BYTE  mask );
void ARCH_DEP( invalidate_tlbe )( REGS* regs, BYTE* main );
void ARCH_DEP( invalidate_pte  )( BYTE ibyte, RADR op1, U32 op2, REGS* regs );

/*-------------------------------------------------------------------*/
/* The following function must be declared separately for each       */
/* build architecture because of opcode.h's "SIE_TRANSLATE_ADDR"     */
/* macro's need (for SIE purposes) to call "translate_addr" for      */
/* an architecture other than the current build architecture.        */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_SIE ) && ARCH_900_IDX != ARCH_IDX
  int s390_translate_addr ( U32 vaddr, int arn, REGS* regs, int acctype);
#endif
#if defined( _FEATURE_ZSIE )
  int z900_translate_addr ( U64 vaddr, int arn, REGS* regs, int acctype );
#endif

/*-------------------------------------------------------------------*/
/* The following function must be declared separately for each       */
/* build architecture because of opcode.h's "SIE_LOGICAL_TO_ABS"     */
/* macro's need (for SIE purposes) to call "logical_to_main" for     */
/* an architecture other than the current build architecture.        */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_SIE ) && ARCH_900_IDX != ARCH_IDX
  DAT_DLL_IMPORT BYTE* s390_logical_to_main( U32 addr,  int arn, REGS* regs, int acctype, BYTE akey );
#endif
#if defined( _FEATURE_ZSIE )
  DAT_DLL_IMPORT BYTE* z900_logical_to_main( U64 addr,  int arn, REGS* regs, int acctype, BYTE akey );
#endif

DAT_DLL_IMPORT BYTE* ARCH_DEP( logical_to_main   )( VADR addr, int arn, REGS* regs, int acctype, BYTE akey );
DAT_DLL_IMPORT BYTE* ARCH_DEP( logical_to_main_l )( VADR addr, int arn, REGS* regs, int acctype, BYTE akey, size_t len );

/* end of DAT.H */
