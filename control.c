/* CONTROL.C    (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) and others 2013-2023                             */
/*              ESA/390 CPU Emulator                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements all control instructions of the            */
/* S/370 and ESA/390 architectures, as described in the manuals      */
/* GA22-7000-03 System/370 Principles of Operation                   */
/* SA22-7201-06 ESA/390 Principles of Operation                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Bad frame support by Jan Jaeger                              */
/*      Branch tracing by Jan Jaeger                                 */
/*      CSP instructions by Jan Jaeger                               */
/*      Instruction decode by macros - Jan Jaeger                    */
/*      Prevent TOD from going backwards in time - Jan Jaeger        */
/*      Instruction decode rework - Jan Jaeger                       */
/*      PR may lose pending interrupts - Jan Jaeger                  */
/*      Modifications for Interpretive Execution (SIE) by Jan Jaeger */
/*      ESAME low-address protection - Roger Bowler                  */
/*      ESAME linkage stack operations - Roger Bowler                */
/*      ESAME BSA instruction - Roger Bowler                    v209c*/
/*      ASN-and-LX-reuse facility - Roger Bowler            June 2004*/
/*      SIGP orders 11,12.2,13,15 - Fish                     Oct 2005*/
/*      Configuration topology facility fixes by PaoloG      Oct 2013*/
/*      PER 1 GRA - Fish                                     Jan 2022*/
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _CONTROL_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "sie.h"


#if defined( FEATURE_SUBSPACE_GROUP )
/*-------------------------------------------------------------------*/
/* Perform subspace replacement                                      */
/*                                                                   */
/* Input:                                                            */
/*      std     Original segment table designation (STD) or ASCE     */
/*      asteo   ASTE origin obtained by ASN translation              */
/*      xcode   Pointer to field to receive exception code, or NULL  */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/* Output:                                                           */
/*      xcode   Exception code or zero (if xcode is not NULL)        */
/*                                                                   */
/* Return value:                                                     */
/*      On successful completion, the exception code field (if not   */
/*      NULL) is set to zero, and the function return value is the   */
/*      STD resulting from subspace replacement, or is the original  */
/*      STD if subspace replacement is not applicable.               */
/*                                                                   */
/* Operation:                                                        */
/*      If the ASF control is enabled, and the STD or ASCE is a      */
/*      member of a subspace-group (bit 22 is one), and the          */
/*      dispatchable unit is subspace active (DUCT word 1 bit 0 is   */
/*      one), and the ASTE obtained by ASN translation is the ASTE   */
/*      for the base space of the dispatchable unit, then the STD    */
/*      or ASCE is replaced (except for the event control bits) by   */
/*      the STD or ASCE from the ASTE for the subspace in which the  */
/*      dispatchable unit last had control; otherwise the STD or     */
/*      ASCE remains unchanged.                                      */
/*                                                                   */
/* Error conditions:                                                 */
/*      If an ASTE validity exception or ASTE sequence exception     */
/*      occurs, and the xcode parameter is a non-NULL pointer,       */
/*      then the exception code is returned in the xcode field       */
/*      and the function return value is zero.                       */
/*      For all other error conditions a program check is generated  */
/*      and the function does not return.                            */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline RADR ARCH_DEP( subspace_replace )( RADR std, U32 asteo,
                        U16* xcode, REGS* regs )
{
U32     ducto;                          /* DUCT origin               */
U32     duct0;                          /* DUCT word 0               */
U32     duct1;                          /* DUCT word 1               */
U32     duct3;                          /* DUCT word 3               */
U32     ssasteo;                        /* Subspace ASTE origin      */
U32     ssaste[16];                     /* Subspace ASTE             */
BYTE    *p;                             /* Mainstor pointer          */

    /* Clear the exception code field, if provided */
    if (xcode != NULL) *xcode = 0;

    /* Return the original STD unchanged if the address-space function
       control (CR0 bit 15) is zero, or if the subspace-group control
       (bit 22 of the STD) is zero */
    if (!ASF_ENABLED( regs )
        || (std & SSGROUP_BIT) == 0)
        return std;

    /* Load the DUCT origin address */
    ducto = regs->CR(2) & CR2_DUCTO;
    ducto = APPLY_PREFIXING( ducto, regs->PX );

    /* Program check if DUCT origin address is invalid */
    if (ducto > regs->mainlim)
        regs->program_interrupt( regs, PGM_ADDRESSING_EXCEPTION );

    /* Fetch DUCT words 0, 1, and 3 from absolute storage
       (note: the DUCT cannot cross a page boundary) */
    p = FETCH_MAIN_ABSOLUTE( ducto, regs, 16 );
    duct0 = fetch_fw( p +  0 );
    duct1 = fetch_fw( p +  4 );
    duct3 = fetch_fw( p + 12 );

    /* Return the original STD unchanged if the dispatchable unit is
       not subspace active or if the ASTE obtained by ASN translation
       is not the same as the base ASTE for the dispatchable unit */
    if ((duct1 & DUCT1_SA) == 0
        || asteo != (duct0 & DUCT0_BASTEO))
        return std;

    /* Load the subspace ASTE origin from the DUCT */
    ssasteo = duct1 & DUCT1_SSASTEO;
    ssasteo = APPLY_PREFIXING( ssasteo, regs->PX );

    /* Program check if ASTE origin address is invalid */
    if (ssasteo > regs->mainlim)
        regs->program_interrupt( regs, PGM_ADDRESSING_EXCEPTION );

    /* Fetch subspace ASTE words 0, 2, 3, and 5 from absolute
       storage (note: the ASTE cannot cross a page boundary) */
    p = FETCH_MAIN_ABSOLUTE( ssasteo, regs, 24 );
    ssaste[0] = fetch_fw( p +  0 );
    ssaste[2] = fetch_fw( p +  8 );
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    ssaste[3] = fetch_fw( p + 12 );
#endif
    ssaste[5] = fetch_fw( p + 20 );

    /* ASTE validity exception if subspace ASTE invalid bit is one */
    if (ssaste[0] & ASTE0_INVALID)
    {
        regs->excarid = 0;
        if (xcode == NULL)
            regs->program_interrupt( regs, PGM_ASTE_VALIDITY_EXCEPTION );
        else
            *xcode = PGM_ASTE_VALIDITY_EXCEPTION;
        return 0;
    }

    /* ASTE sequence exception if the subspace ASTE sequence
       number does not match the sequence number in the DUCT */
    if ((ssaste[5] & ASTE5_ASTESN) != (duct3 & DUCT3_SSASTESN))
    {
        regs->excarid = 0;
        if (xcode == NULL)
            regs->program_interrupt( regs, PGM_ASTE_SEQUENCE_EXCEPTION );
        else
            *xcode = PGM_ASTE_SEQUENCE_EXCEPTION;
        return 0;
    }

    /* Replace the STD or ASCE with the subspace ASTE STD or ASCE,
       except for the space switch event bit and the storage
       alteration event bit, which remain unchanged */
    std &= (SSEVENT_BIT | SAEVENT_BIT);
    std |= (ASTE_AS_DESIGNATOR( ssaste )
                & ~((RADR)(SSEVENT_BIT | SAEVENT_BIT)));

    /* Return the STD resulting from subspace replacement */
    return std;

} /* end function subspace_replace */
#endif /* FEATURE_SUBSPACE_GROUP */

/* When an operation code has unused operand(s) (IPK, e.g.), it will */
/* attract  a diagnostic for a set, but unused variable.  Fixing the */
/* macros to support e.g., RS_NOOPS is not productive, so:           */
DISABLE_GCC_UNUSED_SET_WARNING

/* Temporary debug */
extern  int ipending_cmd( int, void*, void* );

#if defined( FEATURE_BRANCH_AND_SET_AUTHORITY )
/*-------------------------------------------------------------------*/
/* B25A BSA   - Branch and Set Authority                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_and_set_authority )
{
int     r1, r2;                         /* Values of R fields        */
U32     ducto;                          /* DUCT origin               */
U32     duct_pkrp;                      /* DUCT PKM/Key/RA/P word    */
RADR    duct_reta;                      /* DUCT return address/amode */
BYTE    key;                            /* New PSW key               */
#if defined( FEATURE_TRACING )
CREG    newcr12 = 0;                    /* CR12 upon completion      */
#endif

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Special operation exception if ASF is not enabled */
    if (!ASF_ENABLED( regs ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, BSA ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Load real address of dispatchable unit control table */
    ducto = regs->CR(2) & CR2_DUCTO;

    /* Apply low-address protection to stores into the DUCT */
    if (ARCH_DEP( is_low_address_protected )( ducto, regs ))
    {
#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
        regs->TEA = (ducto & STORAGE_KEY_PAGEMASK);
        regs->excarid = 0;
#endif
        ARCH_DEP( program_interrupt )( regs, PGM_PROTECTION_EXCEPTION );
    }

    /* Convert DUCT real address to absolute address */
    ducto = APPLY_PREFIXING( ducto, regs->PX );

    /* Program check if DUCT origin address is invalid */
    if (ducto > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* For ESAME, load the PKM/Key/RA/P from DUCT word 5, and load
       the return address and amode from DUCT words 8 and 9
       (note: the DUCT cannot cross a page boundary) */
    duct_pkrp = ARCH_DEP( fetch_fullword_absolute   )( ducto+20, regs );
    duct_reta = ARCH_DEP( fetch_doubleword_absolute )( ducto+32, regs );
#else
    /* For ESA/390, load the PKM/Key/RA/P from DUCT word 9, and load
       the return address and amode from DUCT word 8
       (note: the DUCT cannot cross a page boundary) */
    duct_pkrp = ARCH_DEP( fetch_fullword_absolute )( ducto+36, regs );
    duct_reta = ARCH_DEP( fetch_fullword_absolute )( ducto+32, regs );
#endif

    /* Perform base authority or reduced authority operation */
    if ((duct_pkrp & DUCT_RA) == 0)
    {
        /* In base authority state R2 cannot specify register zero */
        if (r2 == 0)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

#if defined( FEATURE_TRACING )
        /* Perform tracing */
        if (regs->CR(12) & CR12_BRTRACE)
            newcr12 = ARCH_DEP( trace_br )( regs->GR_L(r2) & 0x80000000,
                                    regs->GR_L(r2), regs );
#endif

        /* Obtain the new PSW key from R1 register bits 24-27 */
        key = regs->GR_L(r1) & 0x000000F0;

        /* Privileged operation exception if in problem state and
           current PSW key mask does not permit new key value */
        if (PROBSTATE( &regs->psw )
            && ((regs->CR(3) << (key >> 4)) & 0x80000000) == 0 )
            ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

        /* Save current PSW amode and instruction address */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
        {
            duct_reta = PSW_IA_FROM_IP( regs, 0 );
        }
        else
#endif
        {
            duct_reta = PSW_IA_FROM_IP( regs, 0 ) & DUCT_IA31;
            if (regs->psw.amode) duct_reta |= DUCT_AM31;
        }

        /* Save current PSW key mask, PSW key, and problem state */
        duct_pkrp = (regs->CR(3) & CR3_KEYMASK) | regs->psw.pkey;
        if (PROBSTATE( &regs->psw )) duct_pkrp |= DUCT_PROB;

        /* Set the reduced authority bit */
        duct_pkrp |= DUCT_RA;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* For ESAME, store the PKM/Key/RA/P into DUCT word 5, and
           store the return address and amode into DUCT words 8 and 9
           (note: the DUCT cannot cross a page boundary) */
        ARCH_DEP( store_fullword_absolute   )( duct_pkrp, ducto+20, regs );
        ARCH_DEP( store_doubleword_absolute )( duct_reta, ducto+32, regs );
#else
        /* For ESA/390, store the PKM/Key/RA/P into DUCT word 9, and
           store the return address and amode into DUCT word 8
           (note: the DUCT cannot cross a page boundary) */
        ARCH_DEP( store_fullword_absolute )( duct_pkrp, ducto+36, regs );
        ARCH_DEP( store_fullword_absolute )( duct_reta, ducto+32, regs );
#endif

        /* Load new PSW key and PSW key mask from R1 register */
        regs->psw.pkey = key;
        regs->CR_LHH(3) &= regs->GR_LHH(r1);

        /* Set the problem state bit in the current PSW */
        regs->psw.states |= BIT( PSW_PROB_BIT );

        /* Set the breaking event address register */
        SET_BEAR_REG( regs, regs->ip - 4 );

        /* Set PSW instruction address and amode from R2 register */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
        {
            SET_PSW_IA_AND_MAYBE_IP( regs, regs->GR_G(r2) );
        }
        else
#endif
        if (regs->GR_L(r2) & 0x80000000)
        {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            regs->psw.amode64 = 0;
#endif
            regs->psw.amode = 1;
            regs->psw.AMASK = AMASK31;
            SET_PSW_IA_AND_MAYBE_IP( regs, regs->GR_L(r2) );
        }
        else
        {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            regs->psw.amode64 =
#endif
            regs->psw.amode = 0;
            regs->psw.AMASK = AMASK24;
            SET_PSW_IA_AND_MAYBE_IP( regs, regs->GR_L(r2) );
        }

    } /* end if (BSA-ba) */
    else
    { /* BSA-ra */

        /* In reduced authority state R2 must specify register zero */
        if (r2 != 0)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

#if defined( FEATURE_TRACING )
        /* Perform tracing */
        if (regs->CR(12) & CR12_BRTRACE)
                newcr12 = ARCH_DEP( trace_br )( duct_reta & DUCT_AM31,
                                        duct_reta &DUCT_IA31, regs );
#endif

        /* Set the breaking event address register */
        SET_BEAR_REG( regs, regs->ip - 4 );

        /* If R1 is non-zero, save the current PSW addressing mode
           and instruction address in the R1 register */
        if (r1 != 0)
        {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            if (regs->psw.amode64)
            {
                regs->GR_G(r1) = PSW_IA_FROM_IP( regs, 0 );
            }
            else
#endif
            {
                regs->GR_L(r1) = PSW_IA_FROM_IP( regs, 0 );
                if (regs->psw.amode) regs->GR_L(r1) |= 0x80000000;
            }
        }

        /* Restore PSW amode and instruction address from the DUCT */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
        {
            SET_PSW_IA_AND_MAYBE_IP( regs, duct_reta );
        }
        else
#endif
        {
            regs->psw.amode = (duct_reta & DUCT_AM31) ? 1 : 0;
            regs->psw.AMASK = regs->psw.amode ? AMASK31 : AMASK24;
            SET_PSW_IA_AND_MAYBE_IP( regs, duct_reta & DUCT_IA31 );
        }

        /* Restore the PSW key mask from the DUCT */
        regs->CR_L(3) &= 0x0000FFFF;
        regs->CR_L(3) |= duct_pkrp & DUCT_PKM;

        /* Restore the PSW key from the DUCT */
        regs->psw.pkey = duct_pkrp & DUCT_KEY;

        /* Restore the problem state bit from the DUCT */
        if (duct_pkrp & DUCT_PROB)
            regs->psw.states |=  BIT( PSW_PROB_BIT );
        else
            regs->psw.states &= ~BIT( PSW_PROB_BIT );

        /* Reset the reduced authority bit in the DUCT */
        duct_pkrp &= ~DUCT_RA;
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        ARCH_DEP( store_fullword_absolute )( duct_pkrp, ducto+20, regs );
#else
        ARCH_DEP( store_fullword_absolute )( duct_pkrp, ducto+36, regs );
#endif

        /* Specification exception if the PSW is now invalid...

           Since the SET_PSW_IA_AND_MAYBE_IP macro used above
           masks off invalid bits in the psw.IA, the only way to
           know if the PSW is now invalid is to test the duct_reta
           itself for any invalid bits.
        */
        if (0
            || (duct_reta & 1)
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            || (1
                && !regs->psw.amode64
                && !regs->psw.amode
                && (duct_reta & 0x7F000000)
               )
#else
            || (1
                && !regs->psw.amode
                && duct_reta > 0x00FFFFFF
               )
#endif
        )
        {
            /* The program_interrupt routine invokes INVALIDATE_AIA
               which applies the addressing mask to the psw.IA when
               the aie is still valid, which we don't want it to do.
               Thus we deliberately set the aie to an invalid value
               to prevent it from doing that.
            */
            regs->aie = INVALID_AIE;
            regs->psw.IA = duct_reta;
            regs->psw.zeroilc = 1;
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        }

    } /* end if (BSA-ra) */

#if defined( FEATURE_TRACING )
    /* Update trace table address if branch tracing is on */
    if (regs->CR(12) & CR12_BRTRACE)
        regs->CR(12) = newcr12;
#endif

    /* Check for Successful Branch PER event */
    PER_SB( regs, regs->psw.IA );

} /* end DEF_INST(branch_and_set_authority) */
#endif /* defined( FEATURE_BRANCH_AND_SET_AUTHORITY ) */


#if defined( FEATURE_SUBSPACE_GROUP )
/*-------------------------------------------------------------------*/
/* B258 BSG   - Branch in Subspace Group                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_in_subspace_group )
{
int     r1, r2;                         /* Values of R fields        */
U32     alet;                           /* Destination subspace ALET */
U32     dasteo=0;                       /* Destination ASTE origin   */
U32     daste[16];                      /* ASN second table entry    */
RADR    ducto;                          /* DUCT origin               */
U32     duct0;                          /* DUCT word 0               */
U32     duct1;                          /* DUCT word 1               */
U32     duct3;                          /* DUCT word 3               */
RADR    abs;                            /* Absolute address          */
BYTE   *mn;                             /* Mainstor address          */
VADR    newia;                          /* New instruction address   */
U16     xcode;                          /* Exception code            */
#if defined( FEATURE_TRACING )
CREG    newcr12 = 0;                    /* CR12 upon completion      */
#endif
CREG    inst_cr;                        /* Instruction CR            */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Special operation exception if DAT is off or ASF not enabled */
    if (REAL_MODE( &regs->psw )
        || !ASF_ENABLED( regs ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    inst_cr = regs->CR( regs->AEA_AR( USE_INST_SPACE ));

#if defined( FEATURE_TRACING )
    /* Perform tracing */
    if (regs->CR(12) & CR12_ASNTRACE)
        newcr12 = ARCH_DEP( trace_bsg )( (r2 == 0) ? 0 : regs->AR(r2),
                                regs->GR_L(r2), regs );
    else
        if (regs->CR(12) & CR12_BRTRACE)
            newcr12 = ARCH_DEP( trace_br )( regs->GR_L(r2) & 0x80000000,
                                regs->GR_L(r2), regs );
#endif

    /* Load real address of dispatchable unit control table */
    ducto = regs->CR(2) & CR2_DUCTO;

    /* Apply low-address protection to stores into the DUCT */
    if (ARCH_DEP( is_low_address_protected )( ducto, regs ))
    {
#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
        regs->TEA = (ducto & STORAGE_KEY_PAGEMASK);
        regs->excarid = 0;
#endif
        ARCH_DEP( program_interrupt )( regs, PGM_PROTECTION_EXCEPTION );
    }

    /* Convert DUCT real address to absolute address */
    ducto = APPLY_PREFIXING( ducto, regs->PX );

    /* Program check if DUCT origin address is invalid */
    if (ducto > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

    /* Fetch DUCT words 0, 1, and 3 from absolute storage
       (note: the DUCT cannot cross a page boundary) */
    mn = FETCH_MAIN_ABSOLUTE( ducto, regs, 16 );
    duct0 = fetch_fw( mn +  0 );
    duct1 = fetch_fw( mn +  4 );
    duct3 = fetch_fw( mn + 12 );

    /* Special operation exception if the current primary ASTE origin
       is not the same as the base ASTE for the dispatchable unit */
    if ((regs->CR_L(5) & CR5_PASTEO) != (duct0 & DUCT0_BASTEO))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Obtain the destination ALET from the R2 access register,
       except that register zero means destination ALET is zero */
    alet = (r2 == 0) ? 0 : regs->AR(r2);

    /* Perform special ALET translation to obtain destination ASTE */
    switch (alet) {

    case ALET_PRIMARY: /* Branch to base space */

        /* Load the base space ASTE origin from the DUCT */
        dasteo = duct0 & DUCT0_BASTEO;

        /* Convert the ASTE origin to an absolute address */
        abs = APPLY_PREFIXING( dasteo, regs->PX );

        /* Program check if ASTE origin address is invalid */
        if (abs > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Fetch destination ASTE words 2 and 3 from absolute storage
           (note: the ASTE cannot cross a page boundary) */
        mn = FETCH_MAIN_ABSOLUTE( abs, regs, 16 );
        daste[2] = fetch_fw( mn +  8 );
        daste[3] = fetch_fw( mn + 12 );

        break;

    case ALET_SECONDARY: /* Branch to last-used subspace */

        /* Load the subspace ASTE origin from the DUCT */
        dasteo = duct1 & DUCT1_SSASTEO;

        /* Special operation exception if SSASTEO is zero */
        if (dasteo == 0)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

        /* Convert the ASTE origin to an absolute address */
        abs = APPLY_PREFIXING( dasteo, regs->PX );

        /* Program check if ASTE origin address is invalid */
        if (abs > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Fetch subspace ASTE words 0, 2, 3, and 5 from absolute
           storage (note: the ASTE cannot cross a page boundary) */
        mn = FETCH_MAIN_ABSOLUTE( abs, regs, 24 );
        daste[0] = fetch_fw( mn +  0 );
        daste[2] = fetch_fw( mn +  8 );
        daste[3] = fetch_fw( mn + 12 );
        daste[5] = fetch_fw( mn + 20 );

        /* ASTE validity exception if ASTE invalid bit is one */
        if (daste[0] & ASTE0_INVALID)
        {
            regs->excarid = r2;
            ARCH_DEP( program_interrupt )( regs, PGM_ASTE_VALIDITY_EXCEPTION );
        }

        /* ASTE sequence exception if the subspace ASTE sequence
           number does not match the sequence number in the DUCT */
        if ((daste[5] & ASTE5_ASTESN) != (duct3 & DUCT3_SSASTESN))
        {
            regs->excarid = r2;
            ARCH_DEP( program_interrupt )( regs, PGM_ASTE_SEQUENCE_EXCEPTION );
        }

        break;

    default: /* ALET not 0 or 1 */

        /* Perform special ART to obtain destination ASTE */
        xcode = ARCH_DEP( translate_alet )( alet, 0, ACCTYPE_BSG, regs,
                                &dasteo, daste );

        /* Program check if ALET translation error */
        if (xcode != 0)
        {
            regs->excarid = r2;
            ARCH_DEP( program_interrupt )( regs, xcode );
        }

        /* Special operation exception if the destination ASTE
           is the base space of a different subspace group */
        if (dasteo != (duct0 & DUCT0_BASTEO)
                && ((ASTE_AS_DESIGNATOR( daste ) & SSGROUP_BIT) == 0
                    || (daste[0] & ASTE0_BASE) ))
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    } /* end switch (alet) */

    /* Update the primary STD (or ASCE) from the destination ASTE */
    if ((dasteo == (duct0 & DUCT0_BASTEO)) && (alet != ALET_SECONDARY))
    {
        /* When the destination ASTE is the base space, replace the
           primary STD (or ASCE) by the STD (or ASCE) in the ASTE */
        regs->CR(1) = ASTE_AS_DESIGNATOR( daste );
    }
    else
    {
        /* When the destination ASTE is a subspace, replace
           the primary STD or ASCE by the STD or ASTE in the
           ASTE, except for the space-switch event and storage
           alteration event bits, which remain unchanged */
        regs->CR(1) &= (SSEVENT_BIT | SAEVENT_BIT);
        regs->CR(1) |= (ASTE_AS_DESIGNATOR( daste )
                        & ~((RADR)(SSEVENT_BIT | SAEVENT_BIT)));
    }

    /* Compute the branch address from the R2 operand */
    newia = regs->GR(r2);

    /* If R1 is non-zero, save the current PSW addressing mode
       and instruction address in the R1 register */
    if (r1 != 0)
    {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            regs->GR_G(r1) = PSW_IA_FROM_IP( regs, 0 );
        else
#endif
            regs->GR_L(r1) = PSW_IA_FROM_IP( regs, 0 ) |
                                (regs->psw.amode ? 0x80000000 : 0);
    }

    /* Update the breaking event address register */
    SET_BEAR_REG( regs, regs->ip - 4 );

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (regs->psw.amode64 == 0 && (newia & 0x80000000))
#else
    if (newia & 0x80000000)
#endif
    {
        regs->psw.amode = 1;
        regs->psw.AMASK = AMASK31;
    }
    else
    {
        regs->psw.amode = 0;
        regs->psw.AMASK = AMASK24;
    }

    /* Set mode and branch to address specified by R2 operand */
    SET_PSW_IA_AND_MAYBE_IP( regs, newia );

    /* Set the SSTD (or SASCE) equal to PSTD (or PASCE) */
    regs->CR(7) = regs->CR(1);

    /* Set SASN equal to PASN */
    regs->CR_LHL(3) = regs->CR_LHL(4);

    /* When ASN-and-LX-reuse is installed and enabled by CR0,
       set the SASTEIN in CR3 equal to the PASTEIN in CR4 */
    if (ASN_AND_LX_REUSE_ENABLED( regs ))
    {
        regs->CR_H(3) = regs->CR_H(4);
    }

    /* Reset the subspace fields in the DUCT */
    if (alet == ALET_SECONDARY)
    {
        /* When the destination ASTE specifies a subspace by means
           of ALET 1, set the subspace active bit in the DUCT */
        duct1 |= DUCT1_SA;
        ARCH_DEP( store_fullword_absolute )( duct1, ducto+4, regs );
    }
    else if (dasteo == (duct0 & DUCT0_BASTEO))
    {
        /* When the destination ASTE is the base space,
           reset the subspace active bit in the DUCT */
        duct1 &= ~DUCT1_SA;
        ARCH_DEP( store_fullword_absolute )( duct1, ducto+4, regs );
    }
    else
    {
        /* When the destination ASTE specifies a subspace by means
           of an ALET other than ALET 1, set the subspace active
           bit and store the subspace ASTE origin in the DUCT */
        duct1 = DUCT1_SA | dasteo;
        ARCH_DEP( store_fullword_absolute )( duct1, ducto+4, regs );

        /* Set the subspace ASTE sequence number in the DUCT
           equal to the destination ASTE sequence number */
        duct3 = daste[5];
        ARCH_DEP( store_fullword_absolute )( duct3, ducto+12, regs );
    }

#if defined( FEATURE_TRACING )
    /* Update trace table address if ASN tracing or branch tracing */
    if (regs->CR(12) & (CR12_ASNTRACE | CR12_BRTRACE))
        regs->CR(12) = newcr12;
#endif

    SET_AEA_COMMON( regs );
    if (inst_cr != regs->CR( regs->AEA_AR( USE_INST_SPACE )))
        INVALIDATE_AIA( regs );

    /* Check for Successful Branch PER event */
    PER_SB( regs, regs->psw.IA );

} /* end DEF_INST(branch_in_subspace_group) */
#endif /* defined( FEATURE_SUBSPACE_GROUP ) */


#if defined( FEATURE_LINKAGE_STACK )
/*-------------------------------------------------------------------*/
/* B240 BAKR  - Branch and Stack Register                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_and_stack )
{
int     r1, r2;                         /* Values of R fields        */
VADR    n1, n2;                         /* Operand values            */
#if defined( FEATURE_TRACING )
VADR    n = 0;                          /* Work area                 */
#endif

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC3, BAKR ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* [5.12.3]/ Fig 10-2 Special operation exception if ASF is not enabled,
       or if DAT is off, or if not primary-space mode or AR-mode */
    if (!ASF_ENABLED( regs )
        || REAL_MODE( &regs->psw )
        || SPACE_BIT( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Obtain the return address and addressing mode from
       the R1 register, or use updated PSW if R1 is zero */
    if (r1 != 0)
    {
        n1 = regs->GR(r1);
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if ((n1 & 0x01) == 0)
            n1 &= (n1 & 0x80000000) ? 0xFFFFFFFF : 0x00FFFFFF;
#else
        if ((n1 & 0x80000000) == 0)
            n1 &= 0x00FFFFFF;
#endif
    }
    else
    {
        n1 = PSW_IA_FROM_IP( regs, 0 );
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            n1 |= 0x01;
        else
#endif
        if (regs->psw.amode)
            n1 |= 0x80000000;
    }

    /* Obtain the branch address from the R2 register, or use
       the updated PSW instruction address if R2 is zero */
    n2 = (r2 != 0) ? regs->GR(r2) : PSW_IA_FROM_IP( regs, 0 );
    n2 &= ADDRESS_MAXWRAP( regs );

    /* Set the addressing mode bit in the branch address */
  #if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (regs->psw.amode64)
        n2 |= 0x01;
    else
  #endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
    if (regs->psw.amode)
        n2 |= 0x80000000;

#if defined( FEATURE_TRACING )
    /* Form the branch trace entry */
    if ((regs->CR(12) & CR12_BRTRACE) && (r2 != 0))
        n = ARCH_DEP( trace_br )( regs->psw.amode, regs->GR_L(r2), regs );
#endif

    /* Form the linkage stack entry */
    ARCH_DEP( form_stack_entry )( LSED_UET_BAKR, n1, n2, 0, 0, regs );

#if defined( FEATURE_TRACING )
    /* Update CR12 to reflect the new branch trace entry */
    if ((regs->CR(12) & CR12_BRTRACE) && (r2 != 0))
        regs->CR(12) = n;
#endif

    /* Execute the branch as long as R2 is non-zero */
    if (r2)
    {
        SET_BEAR_REG( regs, regs->ip - 4 );
        SET_PSW_IA_AND_MAYBE_IP( regs, regs->GR(r2) );
        PER_SB( regs, regs->psw.IA );
    }

} /* end DEF_INST(branch_and_stack) */
#endif /* defined( FEATURE_LINKAGE_STACK ) */


#if defined( FEATURE_BROADCASTED_PURGING ) \
 || defined( FEATURE_003_DAT_ENHANCE_FACILITY_1 )
/*-------------------------------------------------------------------*/
/*      Common processing function for CSP/CSPG instructions         */
/*-------------------------------------------------------------------*/
void ARCH_DEP( compare_and_swap_and_purge_instruction )( BYTE inst[], REGS* regs, bool CSPG )
{
int     r1, r2;                         /* Values of R fields        */
U64     n2;                             /* Virtual address of op2    */
BYTE*   main2;                          /* Mainstor address of op2   */

U32     old32;                          /* Old value (CSP)           */
U32     new32;                          /* New value (CSP)           */

U64     old64;                          /* Old value (CSPG)          */
U64     new64;                          /* New value (CSPG)          */

    RRE( inst, regs, r1, r2 );

    TXF_MISC_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    ODD_CHECK( r1, regs );
    PER_ZEROADDR_CHECK( regs, r2 );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC0, IPTECSP ))
        SIE_INTERCEPT( regs );
#endif

    PERFORM_SERIALIZATION( regs );
    {
        OBTAIN_INTLOCK( regs );
        {
            SYNCHRONIZE_CPUS( regs );

#if defined( _FEATURE_SIE )
            if (SIE_MODE( regs ) && regs->sie_scao)
            {
                /* Try to obtain the SCA IPTE interlock. If successfully
                   obtained, then continue normally. Otherwise ask z/VM
                   to please intercept & execute this instruction itself.
                */
                if (!TRY_OBTAIN_SCALOCK( regs ))
                {
                    RELEASE_INTLOCK( regs );
                    SIE_INTERCEPT( regs );
                }
            }
#endif
            /* Obtain 2nd operand address from r2 */
            if (CSPG)
            {
                n2 = regs->GR(r2) & 0xFFFFFFFFFFFFFFF8ULL & ADDRESS_MAXWRAP( regs );
                main2 = MADDRL( n2, 8, r2, regs, ACCTYPE_WRITE, regs->psw.pkey );

                old64 = CSWAP64( regs->GR_G( r1   ));
                new64 = CSWAP64( regs->GR_G( r1+1 ));
            }
            else
            {
                n2 = regs->GR(r2) & 0xFFFFFFFFFFFFFFFCULL & ADDRESS_MAXWRAP( regs );
                main2 = MADDRL( n2, 4, r2, regs, ACCTYPE_WRITE, regs->psw.pkey );

                old32 = CSWAP32( regs->GR_L( r1   ));
                new32 = CSWAP32( regs->GR_L( r1+1 ));
            }

            /* MAINLOCK may be required if cmpxchg assists unavailable */
            OBTAIN_MAINLOCK( regs );
            {
                /* Attempt to exchange the values */
                if (CSPG)
                    regs->psw.cc = cmpxchg8( &old64, new64, main2 );
                else
                    regs->psw.cc = cmpxchg4( &old32, new32, main2 );
            }
            RELEASE_MAINLOCK( regs );

            if (regs->psw.cc == 0)
            {
                /* Perform requested function as per request-code in r2 */
                if (regs->GR_L(r2) & 3)
                {
                    /* Purge the TLB and/or ALB as requested */
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
                    if (FACILITY_ENABLED( 073_TRANSACT_EXEC, regs ))
                        txf_abort_all( regs->cpuad,
                            CSPG ? TXF_WHY_CSPG_INSTR
                                 : TXF_WHY_CSP_INSTR, PTT_LOC );
#endif
                    if (regs->GR_L(r2) & 1)
                        ARCH_DEP( purge_tlb_all )( regs, 0xFFFF );

                    if (regs->GR_L(r2) & 2)
                        ARCH_DEP( purge_alb_all )( regs );
                }
            }

#if defined( _FEATURE_SIE )
            /* Release the SCA lock */
            if (SIE_MODE( regs ) && regs->sie_scao)
                RELEASE_SCALOCK( regs );
#endif
        }
        RELEASE_INTLOCK( regs );
    }
    PERFORM_SERIALIZATION( regs );

    /* "Yield" if the swap failed, so as to give the
       guest's retry a better chance of succeeding.
    */
    if (regs->psw.cc != 0)
    {
        if (CSPG)
        {
            PTT_CSF( "*CSPG", regs->GR_G(r1), regs->GR_G(r2), regs->psw.IA_G );
            regs->GR_G(r1) = CSWAP64( old64 );
        }
        else
        {
            PTT_CSF( "*CSP", regs->GR_L(r1) ,regs->GR_L(r2), regs->psw.IA_L );
            regs->GR_L(r1) = CSWAP32( old32 );
        }

        if (sysblk.cpus > 1)
            sched_yield();
    }

} /* end compare_and_swap_and_purge_instruction */
#endif /* defined( FEATURE_BROADCASTED_PURGING )
       || defined( FEATURE_003_DAT_ENHANCE_FACILITY_1 ) */


#if defined( FEATURE_BROADCASTED_PURGING )
/*-------------------------------------------------------------------*/
/* B250 CSP   - Compare and Swap and Purge                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_and_swap_and_purge )
{
    ARCH_DEP( compare_and_swap_and_purge_instruction )( inst, regs, false );
}
#endif /* defined( FEATURE_BROADCASTED_PURGING ) */


/*-------------------------------------------------------------------*/
/* 83   DIAG  - Diagnose                                        [RS] */
/*-------------------------------------------------------------------*/
DEF_INST( diagnose )
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RS( inst, regs, r1, r3, b2, effective_addr2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    PTT_INF( "DIAG", regs->GR_G( r1 ), regs->GR_G( r3 ),
        (U32)(effective_addr2 & 0xffffff) );

    /* Let VM370 Assist take first crack at diagnose instructions */
#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dodiag( regs, r1, r3, b2, effective_addr2 ) == 0)
        return;
#endif
    /*  Diagnose F08/F09 (Get Hercules Instruction Counter) is NOT
        a privileged instruction and can be executed by ANY problem
        state program as long as the HERC_PROBSTATE_DIAGF08 facility
        is enabled.  All OTHER diagnose instructions (including the
        Diagnose F08 (Get Hercules Instruction Counter) instruction
        when the HERC_PROBSTATE_DIAGF08 facility is NOT enabled)
        ARE privileged and require supervisor state.
    */
#if defined( FEATURE_HERCULES_DIAGCALLS )
    if (1
#if defined( _FEATURE_SIE )
        && !SIE_MODE( regs )
#endif
        && !(((effective_addr2 == 0xF08) ||
              (effective_addr2 == 0xF09)) &&
              FACILITY_ENABLED( HERC_PROBSTATE_DIAGF08, regs ))
    )
#endif
    {
        /* Program Check Interrupt if not supervisor state */
        PRIV_CHECK( regs );
    }

    /* Allow SIE to intercept this instruction */
    SIE_INTERCEPT( regs );

    /* Otherwise process this diagnose instruction */
    regs->diagnose = TRUE;
    {
        ARCH_DEP( diagnose_call )( regs, r1, r3, b2, effective_addr2 );
    }
    regs->diagnose = FALSE;

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

#if defined( FEATURE_HERCULES_DIAGCALLS )
    RETURN_INTCHECK( regs );
#endif

} /* end DEF_INST(diagnose) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* B226 EPAR  - Extract Primary ASN                            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( extract_primary_asn )
{
int     r1, r2;                         /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Special operation exception if DAT is off */
    if ((regs->psw.sysmask & PSW_DATMODE) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if in problem state
       and the extraction-authority control bit is zero */
    if (PROBSTATE( &regs->psw )
         && (regs->CR(0) & CR0_EXT_AUTH) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Load R1 bits 48-63 with PASN from control register 4 bits 48-63
       and zeroize R1 bits 32-47 */
    regs->GR_L(r1) = regs->CR_LHL(4);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(extract_primary_asn) */
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
/*-------------------------------------------------------------------*/
/* B99A EPAIR - Extract Primary ASN and Instance               [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( extract_primary_asn_and_instance )
{
int r1, r2;                             /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Special operation exception if DAT is off */
    if ((regs->psw.sysmask & PSW_DATMODE) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if in problem state
       and the extraction-authority control bit is zero */
    if (PROBSTATE( &regs->psw )
         && (regs->CR(0) & CR0_EXT_AUTH) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Load R1 bits 48-63 with PASN from control register 4 bits 48-63
       and zeroize R1 bits 32-47 */
    regs->GR_L(r1) = regs->CR_LHL(4);

    /* Load R1 bits 0-31 with PASTEIN from control register 4 bits 0-31 */
    regs->GR_H(r1) = regs->CR_H(4);

} /* end DEF_INST(extract_primary_asn_and_instance) */
#endif /* defined( FEATURE_006_ASN_LX_REUSE_FACILITY ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* B227 ESAR  - Extract Secondary ASN                          [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( extract_secondary_asn )
{
int     r1, r2;                         /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Special operation exception if DAT is off */
    if ((regs->psw.sysmask & PSW_DATMODE) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if in problem state
       and the extraction-authority control bit is zero */
    if (PROBSTATE( &regs->psw )
         && (regs->CR(0) & CR0_EXT_AUTH) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Load R1 bits 48-63 with SASN from control register 3 bits 48-63
       and zeroize R1 bits 32-47 */
    regs->GR_L(r1) = regs->CR_LHL(3);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(extract_secondary_asn) */
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
/*-------------------------------------------------------------------*/
/* B99B ESAIR - Extract Secondary ASN and Instance             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( extract_secondary_asn_and_instance )
{
int r1, r2;                             /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Special operation exception if DAT is off */
    if ((regs->psw.sysmask & PSW_DATMODE) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if in problem state
       and the extraction-authority control bit is zero */
    if (PROBSTATE( &regs->psw )
         && (regs->CR(0) & CR0_EXT_AUTH) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Load R1 bits 48-63 with SASN from control register 3 bits 48-63
       and zeroize R1 bits 32-47 */
    regs->GR_L(r1) = regs->CR_LHL(3);

    /* Load R1 bits 0-31 with SASTEIN from control register 3 bits 0-31 */
    regs->GR_H(r1) = regs->CR_H(3);

} /* end DEF_INST(extract_secondary_asn_and_instance) */
#endif /* defined( FEATURE_006_ASN_LX_REUSE_FACILITY ) */


#if defined( FEATURE_LINKAGE_STACK )
/*-------------------------------------------------------------------*/
/* B249 EREG  - Extract Stacked Registers                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( extract_stacked_registers )
{
int     r1, r2;                         /* Values of R fields        */
LSED    lsed;                           /* Linkage stack entry desc. */
VADR    lsea;                           /* Linkage stack entry addr  */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Find the virtual address of the entry descriptor
       of the current state entry in the linkage stack */
    lsea = ARCH_DEP( locate_stack_entry )( 0, &lsed, regs );

    /* Load registers from the stack entry */
    ARCH_DEP( unstack_registers )( 0, lsea, r1, r2, regs );

#if defined( FEATURE_PER1 )
    if (EN_IC_PER_GRA( regs ))
    {
        /* Check for PER 1 GRA event */
        int i; U16 rmask = 0x0000;
        if (r1 > r2)
        {
            for (i = r1; i <= 15; ++i)
                rmask |= PER_GRA_MASK( i );
            for (i = 0; i <= r2; ++i)
                rmask |= PER_GRA_MASK( i );
        }
        else // (r1 <= r2)
            for (i = r1; i <= r2; ++i)
                rmask |= PER_GRA_MASK( i );
        PER_GRA_CHECK( regs, rmask );
    }
#endif
}
#endif /* defined( FEATURE_LINKAGE_STACK ) */


#if defined( FEATURE_LINKAGE_STACK )
/*-------------------------------------------------------------------*/
/* B24A ESTA  - Extract Stacked State                          [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( extract_stacked_state )
{
int     r1, r2;                         /* Values of R fields        */
BYTE    code;                           /* Extraction code           */
LSED    lsed;                           /* Linkage stack entry desc. */
VADR    lsea;                           /* Linkage stack entry addr  */
int     max_esta_code;

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    if (REAL_MODE( &regs->psw )
        || SECONDARY_SPACE_MODE( &regs->psw )
        || !ASF_ENABLED( regs ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Load the extraction code from low-order byte of R2 register */
    code = regs->GR_LHLCL(r2);

#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
    max_esta_code = FACILITY_ENABLED( 006_ASN_LX_REUSE, regs ) ? 5 : 4;
#elif defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    max_esta_code=4;
#else /*!defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )*/
    max_esta_code=3;
#endif /*!defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )*/

    /* Program check if r1 is odd, or if extraction code is invalid */
    if ((r1 & 1) || code > max_esta_code)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Find the virtual address of the entry descriptor
       of the current state entry in the linkage stack */
    lsea = ARCH_DEP( locate_stack_entry )( 0, &lsed, regs );

    /* Load general register pair from state entry */
    ARCH_DEP( stack_extract )( lsea, r1, code, regs );

    /* Set condition code depending on entry type */
    regs->psw.cc =  ((lsed.uet & LSED_UET_ET) == LSED_UET_PC) ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}
#endif /* defined( FEATURE_LINKAGE_STACK ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* B224 IAC   - Insert Address Space Control                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( insert_address_space_control )
{
int     r1, r2;                         /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Special operation exception if DAT is off */
    if (REAL_MODE( &regs->psw )
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
      /* Except in XC mode */
      && !SIE_STATE_BIT_ON( regs, MX, XC )
#endif
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if in problem state
       and the extraction-authority control bit is zero */
    if (PROBSTATE( &regs->psw )
         && !(regs->CR(0) & CR0_EXT_AUTH)
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
         /* Ignore extraction control in XC mode */
         && !SIE_STATE_BIT_ON( regs, MX, XC )
#endif
    )
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Extract the address-space control bits from the PSW */
    regs->psw.cc = (AR_BIT( &regs->psw ) << 1) | SPACE_BIT( &regs->psw );

    /* Insert address-space mode into register bits 22-23 */
    regs->GR_LHLCH(r1) = regs->psw.cc;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


/*-------------------------------------------------------------------*/
/* B20B IPK   - Insert PSW Key                                   [S] */
/*-------------------------------------------------------------------*/
DEF_INST( insert_psw_key )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Privileged operation exception if in problem state
       and the extraction-authority control bit is zero */
    if (PROBSTATE( &regs->psw )
         && (regs->CR(0) & CR0_EXT_AUTH) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Insert PSW key into bits 24-27 of general register 2
       and set bits 28-31 of general register 2 to zero */
    regs->GR_LHLCL(2) = regs->psw.pkey & 0xF0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( 2 ));
}


#if defined( FEATURE_BASIC_STORAGE_KEYS )
/*-------------------------------------------------------------------*/
/* 09   ISK   - Insert Storage Key                              [RR] */
/*-------------------------------------------------------------------*/
DEF_INST( insert_storage_key )
{
int     r1, r2;                         /* Operand register numbers  */
RADR    pageaddr;                       /* Working abs page address  */

    RR(inst, regs, r1, r2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* Special Operation Exception if Storkey exception control zero */
#if defined( FEATURE_4K_STORAGE_KEYS ) || defined( _FEATURE_SIE )
    if (
#if defined( _FEATURE_SIE ) && !defined( FEATURE_4K_STORAGE_KEYS )
        SIE_MODE( regs ) &&
#endif
        !(regs->CR(0) & CR0_STORKEY_4K) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
#endif

    /* Program check if r2 bits 28-31 are not zeroes */
    if (regs->GR_L(r2) & 0x0000000F)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Load 2K block address from r2 register */
    pageaddr = regs->GR_L(r2) & 0x00FFF800;

    /* Convert real address to absolute address */
    pageaddr = APPLY_PREFIXING( pageaddr, regs->PX );

    /* Addressing exception if block is outside of main storage */
    if (pageaddr > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
    {
        if (SIE_STATE_BIT_ON( regs, IC2, ISKE ))
            SIE_INTERCEPT( regs );

        if (!regs->sie_pref)
        {
#if defined( _FEATURE_STORAGE_KEY_ASSIST )
            if (1
                && SIE_STATE_BIT_ON( regs, RCPO0, ASIST )
                && SIE_STATE_BIT_ON( regs, RCPO2, RCPBY )
            )
            {
                /* When "bypass use of RCP table" is requested
                   the guest page is assumed to be accessible.
                */
                /* Translate guest absolute to host absolute */
                SIE_TRANSLATE( &pageaddr, ACCTYPE_SIE, regs );

                /* Insert the requested storage key */
                regs->GR_LHLCL(r1) = ARCH_DEP( get_2K_storage_key )( pageaddr );
            }
            else // use RCP...(and possibly PGSTE)
#endif
            {
                PGSTE* pgste;
                RCPTE* rcpte;
                int    sr;
                BYTE   oldkey;

                ARCH_DEP( GetPGSTE_and_RCPTE )( regs, pageaddr, &pgste, &rcpte );

                OBTAIN_KEYLOCK( pgste, rcpte, regs );
                {
                    /* Translate guest absolute address to host real.
                       Note that the RCP table MUST be locked BEFORE
                       we try to access the real page!
                    */
                    sr = SIE_TRANSLATE_ADDR( regs->sie_mso + pageaddr,
                                             USE_PRIMARY_SPACE,
                                             HOSTREGS, ACCTYPE_SIE );
                    if (sr == 0)
                    {
                        /* Translate host real to host absolute */
                        pageaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

                        /* Save the original key */
                        oldkey = ARCH_DEP( get_4K_storage_key )( pageaddr );

                        /* For ISK(E) include RCP table R/C bits too */
                        oldkey |= (rcpte->rcpbyte & RCPGUEST);
                    }
                    else // (sr != 0)
                    {
                        /* If the real page is inaccessible and SKA
                           is not active, then we cannot proceed since,
                           with the old non-SKA RCP table approach,
                           there isn't any other way to set or obtain
                           the page's access key and fetch-protect bits
                           since the old non-SKA RCP table contains
                           ONLY the R/C bits, but not anything else.
                        */
                        if (!pgste)
                        {
                            RELEASE_KEYLOCK( pgste, rcpte, regs );
                            SIE_INTERCEPT( regs );
                        }

                        /* Reconstruct the original storage key from
                           both the PGSTE and RCPTE entries.
                        */
                        oldkey = (pgste->pgsvkey & PGSVKACF)
                               | (rcpte->rcpbyte & RCPGUEST);
                    }

                    /* Insert the requested storage key */
                    regs->GR_LHLCL(r1) = oldkey;
                }
                RELEASE_KEYLOCK( pgste, rcpte, regs );
            }
        }
        else /* sie_pref */
        {
            /* Insert the requested storage key */
            regs->GR_LHLCL(r1) = ARCH_DEP( get_2K_storage_key )( pageaddr );
        }
    }
    else /* !SIE_MODE */
#endif /* defined( _FEATURE_SIE ) */
    {
        /* Insert the storage key into r1 register bits 24-31 */
        regs->GR_LHLCL(r1) = ARCH_DEP( get_2K_storage_key )( pageaddr );
    }

    /* In BC mode, clear bits 29-31 of r1 register */
    if (!ECMODE( &regs->psw ))
    {
        regs->GR_LHLCL(r1) &= 0xF8;
    }

//  /*debug*/LOGMSG( "ISK storage block %8.8X key %2.2X\n",
//                   regs->GR_L(r2), regs->GR_L(r1) & 0xFE );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST( insert_storage_key ) */
#endif /* defined( FEATURE_BASIC_STORAGE_KEYS ) */


#if defined( FEATURE_EXTENDED_STORAGE_KEYS )
/*-------------------------------------------------------------------*/
/* B229 ISKE  - Insert Storage Key Extended                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( insert_storage_key_extended )
{
int     r1, r2;                         /* Operand register numbers  */
RADR    pageaddr;                       /* Working abs page address  */

    RRE( inst, regs, r1, r2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* Load 4K block address from r2 register */
    pageaddr = regs->GR(r2) & ADDRESS_MAXWRAP_E(regs);

    /* Convert real address to absolute address */
    pageaddr = APPLY_PREFIXING( pageaddr, regs->PX );

    /* Addressing exception if block is outside of main storage */
    if (pageaddr > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
    {
        if (SIE_STATE_BIT_ON( regs, IC2, ISKE ))
            SIE_INTERCEPT( regs );

        if (!regs->sie_pref)
        {
#if defined( _FEATURE_STORAGE_KEY_ASSIST )
            if ((0
                || SIE_STATE_BIT_ON( regs, RCPO0, ASIST )
#if defined( _FEATURE_ZSIE )
                // SKA is always active for z/VM
                || ARCH_900_IDX == HOSTREGS->arch_mode
#endif
                )
                && SIE_STATE_BIT_ON( regs, RCPO2, RCPBY )
            )
            {
                /* When "bypass use of RCP table" is requested
                   the guest page is assumed to be accessible.
                */
                /* Translate guest absolute to host absolute */
                SIE_TRANSLATE( &pageaddr, ACCTYPE_SIE, regs );

                /* Insert the requested storage key */
                regs->GR_LHLCL(r1) = ARCH_DEP( get_4K_storage_key )( pageaddr );
            }
            else // use RCP...(and possibly PGSTE)
#endif /* defined( _FEATURE_STORAGE_KEY_ASSIST ) */
            {
                PGSTE* pgste;
                RCPTE* rcpte;
                int    sr;
                BYTE   oldkey;

                ARCH_DEP( GetPGSTE_and_RCPTE )( regs, pageaddr, &pgste, &rcpte );

                OBTAIN_KEYLOCK( pgste, rcpte, regs );
                {
                    /* Translate guest absolute address to host real.
                       Note that the RCP table MUST be locked BEFORE
                       we try to access the real page!
                    */
                    sr = SIE_TRANSLATE_ADDR( regs->sie_mso + pageaddr,
                                             USE_PRIMARY_SPACE,
                                             HOSTREGS, ACCTYPE_SIE );
                    if (sr == 0)
                    {
                        /* Translate host real to host absolute */
                        pageaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

                        /* Save the original key */
                        oldkey = ARCH_DEP( get_4K_storage_key )( pageaddr );

                        /* For ISK(E) include RCP table R/C bits too */
                        oldkey |= (rcpte->rcpbyte & RCPGUEST);
                    }
                    else // (sr != 0)
                    {
                        /* If the real page is inaccessible and SKA
                           is not active, then we cannot proceed since,
                           with the old non-SKA RCP table approach,
                           there isn't any other way to set or obtain
                           the page's access key and fetch-protect bits
                           since the old non-SKA RCP table contains
                           ONLY the R/C bits, but not anything else.
                        */
                        if (!pgste)
                        {
                            RELEASE_KEYLOCK( pgste, rcpte, regs );
                            SIE_INTERCEPT( regs );
                        }

                        /* Reconstruct the original storage key from
                           both the PGSTE and RCPTE entries.
                        */
                        oldkey = (pgste->pgsvkey & PGSVKACF)
                               | (rcpte->rcpbyte & RCPGUEST);
                    }

                    /* Insert the requested storage key */
                    regs->GR_LHLCL(r1) = oldkey;
                }
                RELEASE_KEYLOCK( pgste, rcpte, regs );
            }
        }
        else /* sie_pref */
        {
            /* Insert the requested storage key */
            regs->GR_LHLCL(r1) = ARCH_DEP( get_4K_storage_key )( pageaddr );
        }
    }
    else /* !SIE_MODE */
#endif /* defined( _FEATURE_SIE ) */
    {
        /* Insert the storage key into r1 register bits 24-31 */
        regs->GR_LHLCL(r1) = ARCH_DEP( get_4K_storage_key )( pageaddr );
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST( insert_storage_key_extended ) */
#endif /* defined( FEATURE_EXTENDED_STORAGE_KEYS ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* B223 IVSK  - Insert Virtual Storage Key                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( insert_virtual_storage_key )
{
int     r1, r2;                         /* Values of R fields        */
VADR    effective_addr;                 /* Operand-2 virtual page    */
RADR    pageaddr;                       /* Operand-2 page address    */
BYTE    vsk = 0;                        /* Virtual Storage Key       */
bool    need_realkey = true;            /* (get from real page)      */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Special operation exception if DAT is off */
    if ((regs->psw.sysmask & PSW_DATMODE) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if in problem state
       and the extraction-authority control bit is zero */
    if (PROBSTATE( &regs->psw )
         && (regs->CR(0) & CR0_EXT_AUTH) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Load virtual storage address from r2 register */
    effective_addr = regs->GR(r2) & ADDRESS_MAXWRAP( regs );

    /* Translate virtual address to real address */
    if (ARCH_DEP( translate_addr )( effective_addr, r2, regs, ACCTYPE_IVSK ))
        ARCH_DEP( program_interrupt )( regs, regs->dat.xcode );

    /* Convert real address to absolute address */
    pageaddr = APPLY_PREFIXING( regs->dat.raddr, regs->PX );

    /* Addressing exception if block is outside main storage */
    if (pageaddr > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( _FEATURE_STORAGE_KEY_ASSIST )
    if (1
        && SIE_MODE( regs )
        && !regs->sie_pref
        && (0
            || SIE_STATE_BIT_ON( regs, RCPO0, ASIST )
#if defined( _FEATURE_ZSIE )
            || (HOSTREGS->arch_mode == ARCH_900_IDX)
#endif
           )
        && !SIE_FEAT_BIT_ON( regs, RCPO2, RCPBY )
    )
    {
        /* SIE_MODE and not sie_pref and SKA and not RCP bypass... */
        int sr;

        /* Convert guest abs to host abs (sr=0) or PTE abs (sr=2) */
        sr = SIE_TRANSLATE_ADDR( regs->sie_mso + pageaddr,
                                 USE_PRIMARY_SPACE,
                                 HOSTREGS, ACCTYPE_SIE );

        /* Translate host real to host absolute */
        pageaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

        /* Program Check if any unexpected translation error */
        if (sr != 0 && sr != 2)
        {
            switch (HOSTREGS->arch_mode)
            {
            case ARCH_370_IDX: s370_program_interrupt( HOSTREGS, HOSTREGS->dat.xcode ); break;
            case ARCH_390_IDX: s390_program_interrupt( HOSTREGS, HOSTREGS->dat.xcode ); break;
            case ARCH_900_IDX: z900_program_interrupt( HOSTREGS, HOSTREGS->dat.xcode ); break;
            default: CRASH();
            }
        }

        if (sr != 0)
        {
            /* sr == 2: host abs page corresponding to guest abs page
               not available. pageaddr is thus host abs PTE instead.
               Obtain needed key information from the PGSTE instead.
            */
            PGSTE* pgste = ARCH_DEP( GetPGSTEFromPTE )( regs, pageaddr );
            vsk = (pgste->pgsvkey & PGSVKACF);
            need_realkey = false;
        }
    }
    else /* Not SIE, or else sie_pref, or else RCP Bypass...  */
#endif /* defined( _FEATURE_STORAGE_KEY_ASSIST ) */
    {
        /* When "bypass use of RCP table" is requested
           the guest page is assumed to be accessible.

           PROGRAMMING NOTE: if we're not in SIE mode,
           then the below SIE_TRANSLATE statement does
           absolutely nothing.
        */
        /* Translate guest absolute to host absolute */
        SIE_TRANSLATE( &pageaddr, ACCTYPE_SIE, regs );
    }

    /* Get needed key from the real page if it's available */
    if (need_realkey)
        vsk = ARCH_DEP( get_storage_key )( pageaddr )
                                       & (STORKEY_KEY | STORKEY_FETCH);

    /* Insert storage key bits 0-4 into r1 register bits
       56-60 and set bits 61-63 to zeroes (i.e. get just
       the access key and the fetch protect bit)
    */
    regs->GR_LHLCL(r1) = vsk;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(insert_virtual_storage_key) */
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


/*-------------------------------------------------------------------*/
/* B221 IPTE  - Invalidate Page Table Entry                  [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST( invalidate_page_table_entry )
{
int     r1, r2;                         /* Operand register numbers  */
#if defined( FEATURE_013_IPTE_RANGE_FACILITY ) || \
    defined( FEATURE_051_LOCAL_TLB_CLEARING_FACILITY )
int     r3;                             /* Operand-3 register number */
int     m4;                             /* Operand-4 mask field      */
#endif
RADR    pto;                            /* Page Table Origin         */
VADR    vaddr;                          /* Virtual Address of first or
                                           only page to invalidate   */
int     pageidx;                        /* Starting page index       */
int     pages = 1;                      /* Total Pages to invalidate */
int     i;                              /* work (for loop iterator)  */
bool    local = false;                  /* true == m4 bit 3 is on    */
#if defined( FEATURE_013_IPTE_RANGE_FACILITY )
bool    do_range = false;               /* helper flag               */
#endif

#if defined( FEATURE_013_IPTE_RANGE_FACILITY ) || \
    defined( FEATURE_051_LOCAL_TLB_CLEARING_FACILITY )

    RRF_RM( inst, regs, r1, r2, r3, m4 );

#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if ((regs->GR_G( r1 ) & 0xfffffffffffff000) == 0) // (bits 0-52)
        ARCH_DEP( per3_zero )( regs );
#endif

    if (1
        && (m4 & 0x01) /* LC == Local Clearing bit on? */
        && FACILITY_ENABLED( 051_LOCAL_TLB_CLEARING, regs )
    )
        local = true;
#else
    RRE( inst, regs, r1, r2 );
#endif

    TXF_MISC_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    pto = regs->GR(r1);
    vaddr = regs->GR(r2);
    pageidx = (vaddr >> SHIFT_4K) & 0xFF;

#if defined( FEATURE_013_IPTE_RANGE_FACILITY )
    if (r3 && FACILITY_ENABLED( 013_IPTE_RANGE, regs ))
    {
        int additional_pages = regs->GR_LHLCL( r3 );
        if ((pageidx + additional_pages) > 255)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        pages += additional_pages;
        do_range = true;
    }
#endif

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC0, IPTECSP ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Perform serialization before operation */
    PERFORM_SERIALIZATION( regs );

    if (!local) OBTAIN_INTLOCK( regs );
    {
        if (!local) SYNCHRONIZE_CPUS( regs );

#if defined( _FEATURE_SIE )

        /* Try to obtain the SCA IPTE interlock. If successfully
           obtained, then continue normally. Otherwise ask z/VM
           to please intercept & execute this instruction itself.
        */
        if (SIE_MODE( regs ) && regs->sie_scao)
        {
            if (!TRY_OBTAIN_SCALOCK( regs ))
            {
                if (!local) RELEASE_INTLOCK( regs );
                SIE_INTERCEPT( regs );
            }
        }
#endif

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
        /* Abort any/all active transactions beforehand */
        if (FACILITY_ENABLED( 073_TRANSACT_EXEC, regs ))
            txf_abort_all( regs->cpuad, TXF_WHY_IPTE_INSTR, PTT_LOC );
#endif
        /* Now invalidate all of the requested Page Table Entries */
        for (i=0; i < pages; ++i, vaddr += _4K)
            ARCH_DEP( invalidate_pte )( inst[1], pto, vaddr, regs, local );

#if defined( FEATURE_013_IPTE_RANGE_FACILITY )
        /* Update registers if range was specified */
        if (do_range)
        {
            regs->GR(r2) = vaddr;
            regs->GR_LHLCL(r3) -= pages;
        }
#endif

#if defined( _FEATURE_SIE )
        /* Release the SCA lock if we obtained it */
        if (SIE_MODE( regs ) && regs->sie_scao)
            RELEASE_SCALOCK (regs );
#endif
    }
    if (!local) RELEASE_INTLOCK( regs );

} /* DEF_INST(invalidate_page_table_entry) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* E500 LASP  - Load Address Space Parameters                  [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_address_space_parameters )
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
U64     dreg;
U32     sastein_d = 0;                  /* Designated SASTEIN        */
U32     pastein_d = 0;                  /* Designated PASTEIN        */
U32     sastein_new = 0;                /* New SASTEIN               */
U32     pastein_new = 0;                /* New PASTEIN               */
U16     pkm_d;                          /* Designated PKM            */
U16     sasn_d;                         /* Designated SASN           */
U16     ax_d;                           /* Designated AX             */
U16     pasn_d;                         /* Designated PASN           */
U32     aste[16];                       /* ASN second table entry    */
RADR    pstd;                           /* Primary STD               */
RADR    sstd;                           /* Secondary STD             */
U32     ltd;                            /* Linkage table designation */
U32     pasteo=0;                       /* Primary ASTE origin       */
U32     sasteo=0;                       /* Secondary ASTE origin     */
U16     ax;                             /* Authorisation index       */
#if defined( FEATURE_SUBSPACE_GROUP )
U16     xcode;                          /* Exception code            */
#endif
CREG    inst_cr;                        /* Instruction CR            */

    SSE( inst, regs, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b1 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );
    PRIV_CHECK( regs );

    /* Special operation exception if ASN translation control
       (bit 12 of control register 14) is zero */
    if ((regs->CR(14) & CR14_ASN_TRAN) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    DW_CHECK( effective_addr1, regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC2, LASP ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    inst_cr = regs->CR( regs->AEA_AR( USE_INST_SPACE ));

    /* Fetch LASP parameters from first operand location
       (note that the storage-operand references for LASP
       may be multiple-access references) */
    if (ASN_AND_LX_REUSE_ENABLED( regs ))
    {
        /* When ASN-and-LX-reuse is installed and enabled by CR0,
           the first operand consists of two doublewords */

        /* The first doubleword contains the SASTEIN-d (32 bits),
           PKM-d (16 bits), and SASN-d (16 bits) */
        dreg = ARCH_DEP( vfetch8 )( effective_addr1, b1, regs );
        sastein_d = (dreg >> 32);
        pkm_d = (dreg >> 16) & 0xFFFF;
        sasn_d = dreg & 0xFFFF;

        /* The second doubleword contains the PASTEIN-d (32 bits),
           AX-d (16 bits), and PASN-d (16 bits) */
        effective_addr1 += 8;
        effective_addr1 &= ADDRESS_MAXWRAP( regs );
        dreg = ARCH_DEP( vfetch8 )( effective_addr1, b1, regs );
        pastein_d = (dreg >> 32);
        ax_d = (dreg >> 16) & 0xFFFF;
        pasn_d = dreg & 0xFFFF;
    }
    else /* !ASN_AND_LX_REUSE_ENABLED */
    {
        /* When ASN-and-LX-reuse is not installed or not enabled,
           the first operand is one doubleword containing the
           PKM-d, SASN-d, AX-d, and PASN-d (16 bits each) */
        dreg = ARCH_DEP( vfetch8 )( effective_addr1, b1, regs );
        pkm_d  = (dreg >> 48) & 0xFFFF;
        sasn_d = (dreg >> 32) & 0xFFFF;
        ax_d   = (dreg >> 16) & 0xFFFF;
        pasn_d = (dreg      ) & 0xFFFF;
    } /* end else !ASN_AND_LX_REUSE_ENABLED */

    /* PASN translation */

    /* Perform PASN translation if PASN not equal to current
       PASN, or if LASP function bit 29 is set */
    if ((effective_addr2 & 0x00000004)
        || pasn_d != regs->CR_LHL(4) )
    {
        /* Translate PASN and return condition code 1 if
           AFX- or ASX-translation exception condition */
        if (ARCH_DEP( translate_asn )( pasn_d, regs, &pasteo, aste ))
        {
            regs->psw.cc = 1;
            return;
        }

        /* When ASN-and-LX-reuse is installed and enabled by CR0,
           condition code 1 is also set if the PASTEIN-d does not
           equal the ASTEIN in word 11 of the ASTE */
        if (ASN_AND_LX_REUSE_ENABLED( regs )
            && pastein_d != aste[11])
        {
            regs->psw.cc = 1;
            return;
        }

        /* Obtain new PSTD and LTD from ASTE */
        pstd = ASTE_AS_DESIGNATOR( aste );
        ltd  = ASTE_LT_DESIGNATOR( aste );
        ax = (aste[1] & ASTE1_AX) >> 16;

        /* When ASN-and-LX-reuse is installed and enabled by CR0,
           set the new PASTEIN equal to the PASTEIN-d */
        if (ASN_AND_LX_REUSE_ENABLED( regs ))
            pastein_new = pastein_d;

#if defined( FEATURE_SUBSPACE_GROUP )
        /* Perform subspace replacement on new PSTD */
        pstd = ARCH_DEP( subspace_replace )( pstd, pasteo, &xcode, regs );

        /* Return with condition code 1 if ASTE exception recognized */
        if (xcode != 0)
        {
            regs->psw.cc = 1;
            return;
        }
#endif
        /* Return with condition code 3 if either current STD
           or new STD indicates a space switch event */
        if ((regs->CR(1) & SSEVENT_BIT)
            || (ASTE_AS_DESIGNATOR( aste ) & SSEVENT_BIT))
        {
            regs->psw.cc = 3;
            return;
        }
    }
    else
    {
        /* Load current PSTD and LTD or PASTEO */
        pstd   = regs->CR(1);
        ltd    = regs->CR_L(5);   /* ZZZ1 NOT SURE ABOUT THE MEANING OF THIS CODE: REFER TO ZZZ2 */
        pasteo = regs->CR_L(5);   /* ZZZ1 NOT SURE ABOUT THE MEANING OF THIS CODE: REFER TO ZZZ2 */
        ax     = (regs->CR(4) & CR4_AX) >> 16;

        /* When ASN-and-LX-reuse is installed and enabled by CR0,
           load the current PASTEIN */
        if (ASN_AND_LX_REUSE_ENABLED( regs ))
            pastein_new = regs->CR_H(4);
    }

    /* If bit 30 of the LASP function bits is zero,
       use the current AX instead of the AX specified
       in the first operand */
    if ((effective_addr2 & 0x00000002))
        ax = ax_d;

    /* SASN translation */

    /* If new SASN = new PASN then set new SSTD = new PSTD,
       also set the new SASTEIN equal to new PASTEIN when
       ASN-and-LX-reuse is installed and enabled by CR0 */
    if (sasn_d == pasn_d)
    {
        sstd = pstd;
        if (ASN_AND_LX_REUSE_ENABLED( regs ))
            sastein_new = pastein_new;
    }
    else
    {
        /* If new SASN = current SASN, and bit 29 of the LASP
       function bits is 0, and bit 31 of the LASP function bits
       is 1, use current SSTD/SASCE in control register 7, also
       use current SASTEIN if ASN-and-LX-reuse is installed and
       enabled by CR0 */
        if (!(effective_addr2 & 0x00000004)
          && (effective_addr2 & 0x00000001)
          && (sasn_d == regs->CR_LHL(3)))
        {
            sstd = regs->CR(7);
            if (ASN_AND_LX_REUSE_ENABLED( regs ))
                sastein_new = regs->CR_H(3);
        }
        else
        {
            /* Translate SASN and return condition code 2 if
               AFX- or ASX-translation exception condition */
            if (ARCH_DEP( translate_asn )( sasn_d, regs, &sasteo, aste ))
            {
                regs->psw.cc = 2;
                return;
            }

            /* When ASN-and-LX-reuse is installed and enabled by CR0,
               condition code 2 is also set if the SASTEIN-d does not
               equal the ASTEIN in word 11 of the ASTE */
            if (ASN_AND_LX_REUSE_ENABLED( regs )
                && sastein_d != aste[11])
            {
                regs->psw.cc = 2;
                return;
            }

            /* Obtain new SSTD or SASCE from secondary ASTE */
            sstd = ASTE_AS_DESIGNATOR( aste );

            /* When ASN-and-LX-reuse is installed and enabled by CR0,
               set the new SASTEIN equal to the SASTEIN-d */
            if (ASN_AND_LX_REUSE_ENABLED( regs ))
                sastein_new = sastein_d;

#if defined( FEATURE_SUBSPACE_GROUP )
            /* Perform subspace replacement on new SSTD */
            sstd = ARCH_DEP(subspace_replace) (sstd, sasteo,
                                                &xcode, regs);

            /* Return condition code 2 if ASTE exception recognized */
            if (xcode != 0)
            {
                regs->psw.cc = 2;
                return;
            }
#endif /* defined( FEATURE_SUBSPACE_GROUP ) */

            /* Perform SASN authorization if bit 31 of the
               LASP function bits is 0 */
            if (!(effective_addr2 & 0x00000001))
            {
                /* Condition code 2 if SASN authorization fails */
                if (ARCH_DEP( authorize_asn )( ax, aste, ATE_SECONDARY,
                                                        regs ))
                {
                    regs->psw.cc = 2;
                    return;
                }

            } /* end if (SASN authorization) */

        } /* end if (SASN translation) */

    } /* end if (SASN = PASN) */

    /* Perform control-register loading */
    regs->CR(1)     = pstd;
    regs->CR_LHH(3) = pkm_d;
    regs->CR_LHL(3) = sasn_d;
    regs->CR_LHH(4) = ax;
    regs->CR_LHL(4) = pasn_d;
    regs->CR_L(5)   = ASF_ENABLED( regs ) ? pasteo : ltd;  /* ZZZ2 NOT SURE ABOUT THE MEANING OF THIS CODE: REFER TO ZZZ1 */
    regs->CR(7)     = sstd;

    if (ASN_AND_LX_REUSE_ENABLED( regs ))
    {
        regs->CR_H(3) = sastein_new;
        regs->CR_H(4) = pastein_new;
    }

    SET_AEA_COMMON( regs );

    if (inst_cr != regs->CR( regs->AEA_AR( USE_INST_SPACE )))
        INVALIDATE_AIA( regs );

    /* Return condition code zero */
    regs->psw.cc = 0;

} /* end DEF_INST(load_address_space_parameters) */
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


/*-------------------------------------------------------------------*/
/* B7   LCTL  - Load Control                                  [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( load_control )
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2 = NULL;                 /* Mainstor pointers         */
U16     updated = 0;                    /* Updated control regs      */

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dolctl( regs, r1, r3, b2, effective_addr2 ) == 0)
        return;
#endif

    PRIV_CHECK( regs );
    FW_CHECK( effective_addr2, regs );

    /* Calculate number of regs to load */
    n = ((r3 - r1) & 0xF) + 1;

    ITIMER_SYNC( effective_addr2, (n*4)-1, regs );

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
    {
        U16 cr_mask = fetch_hw( regs->siebk->lctl_ctl );
        for (i=0; i < n; i++)
            if (cr_mask & BIT( 15 - ( (r1 + i) & 0xF )))
                longjmp( regs->progjmp, SIE_INTERCEPT_INST );
    }
#endif

    /* Calculate number of words to next boundary */
    m = (PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK)) >> 2;

    /* Address of operand beginning */
    p1 = (U32*) MADDR( effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey );

    /* Get address of next page if boundary crossed */
    if (unlikely( m < n ))
        p2 = (U32*) MADDR( effective_addr2 + (m*4), b2, regs, ACCTYPE_READ, regs->psw.pkey );
    else
        m = n;

    /* Copy from operand beginning */
    for (i=0; i < m; i++, p1++)
    {
        regs->CR_L( (r1 + i) & 0xF ) = fetch_fw( p1 );
        updated |= BIT( (r1 + i) & 0xF );
    }

    /* Copy from next page */
    for (; i < n; i++, p2++)
    {
        regs->CR_L( (r1 + i) & 0xF ) = fetch_fw( p2 );
        updated |= BIT( (r1 + i) & 0xF );
    }

    /* Actions based on updated control regs */
    SET_IC_MASK( regs );

#if __GEN_ARCH == 370
    if (updated & BIT( 1 ))
    {
        SET_AEA_COMMON( regs );
        INVALIDATE_AIA( regs );
    }
#else
    if (updated & (BIT( 1 ) | BIT( 7 ) | BIT( 13 )))
        SET_AEA_COMMON( regs );
    if (updated & BIT( regs->AEA_AR( USE_INST_SPACE )))
        INVALIDATE_AIA( regs );
#endif
    if (updated & BIT( 9 ))
    {
        OBTAIN_INTLOCK( regs );
        {
            SET_IC_PER( regs );
        }
        RELEASE_INTLOCK( regs );

        if (EN_IC_PER_SA( regs ))
            ARCH_DEP( invalidate_tlb )( regs, ~(ACC_WRITE | ACC_CHECK) );
    }

    RETURN_INTCHECK( regs );

} /* end DEF_INST( load_control ) */


/*-------------------------------------------------------------------*/
/* 82   LPSW  - Load Program Status Word                        [SI] */
/*-------------------------------------------------------------------*/
DEF_INST( load_program_status_word )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
DBLWRD  dword;
int     rc;
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
int     amode64;
#endif

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dolpsw( regs, b2, effective_addr2 ) == 0)
    {
        return;
    }
#endif

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, LPSW ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );
    {
        /* Fetch new PSW from operand address */
        STORE_DW ( dword, ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ));

        /* Set the breaking event address register */
        SET_BEAR_REG( regs, regs->ip - 4 );

        /* Load updated PSW (ESA/390 Format in ESAME mode) */
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if ((rc = ARCH_DEP( load_psw )( regs, dword )))
            ARCH_DEP( program_interrupt )( regs, rc );
#else
        /* Make the PSW valid for ESA/390 mode
           after first saving our amode64 flag */
        amode64 = dword[3] & 0x01;
        dword[3] &= ~0x01;

        /* Now call 's390_load_psw' to load our ESA/390 PSW for us */
        rc = s390_load_psw( regs, dword );

        /* PROGRAMMING NOTE: z/Arch (ESAME) only supports the loading
           of ESA/390 mode (Extended Control mode (i.e. EC mode)) PSWs
           via the LPSW instruction. Thus the above 's390_load_psw' call
           has already checked to  make sure bit 12 -- the EC mode bit
           (otherwise also known as the 'NOTESAME' bit) -- was set when
           the PSW was loaded (otherwise it would not have even returned
           and instead would have thrown a PGM_SPECIFICATION_EXCEPTION).

           Since we're actually executing in z/Arch (ESAME) mode though
           we now need to turn that bit off (since it's not supposed to
           be on for z/Arch (ESAME) mode) as explained in the Principles
           of Operations manual for the LPSW instruction.
        */
        regs->psw.states &= ~BIT( PSW_NOTESAME_BIT );

        /* Restore the amode64 flag setting and set the actual correct
           AMASK value according to it (if we need to) since we had to
           force non-amode64 further above to get the 's390_load_psw'
           function to work right without erroneously program-checking.
        */
        if ((regs->psw.amode64 = amode64))
        {
            regs->psw.AMASK = AMASK64;

            /* amode31 bit must be set when amode64 is set */
            if (!regs->psw.amode)
            {
                regs->psw.zeroilc = 1;
                ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
            }
        }
        else
            /* Clear the high word of the address mask since
               the 's390_load_psw' function didn't do that for us */
            regs->psw.AMASK_H = 0;

        /* Check for Load PSW success/failure */
        if (rc)
            ARCH_DEP( program_interrupt )( regs, rc );

        /* Clear the high word of the instruction address since
           the 's390_load_psw' function didn't do that for us */
        regs->psw.IA_H = 0;

#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

    }
    PERFORM_CHKPT_SYNC( regs );
    PERFORM_SERIALIZATION( regs );

    RETURN_INTCHECK( regs );

} /* end DEF_INST(load_program_status_word) */


/*-------------------------------------------------------------------*/
/* B1   LRA   - Load Real Address                             [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST( load_real_address )
{
int     r1;                             /* Register number           */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX( inst, regs, r1, x2, b2, effective_addr2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dolra( regs, r1, b2, effective_addr2 ) == 0)
    {
        return;
    }
#endif

    ARCH_DEP( load_real_address_proc )( regs, r1, b2, effective_addr2 );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(load_real_address) */


/*-------------------------------------------------------------------*/
/* Common processing routine for the LRA and LRAY instructions       */
/*-------------------------------------------------------------------*/
void ARCH_DEP( load_real_address_proc )( REGS* regs,
                int r1, int b2, VADR effective_addr2 )
{
int     cc;                             /* Condition code            */

    SIE_XC_INTERCEPT( regs );
    PRIV_CHECK( regs );

    /* Translate the effective address to a real address */
    cc = ARCH_DEP( translate_addr )( effective_addr2, b2, regs, ACCTYPE_LRA );

    /* If ALET exception or ASCE-type or region translation
       exception, set exception code in R1 bits 48-63, set
       bit 32 of R1, and set condition code 3 */
    if (cc > 3) {
        regs->GR_L(r1) = 0x80000000 | regs->dat.xcode;
        cc = 3;
    }
    else
    {
        /* Set r1 and condition code as returned by translate_addr */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64 && cc != 3)
        {
            regs->GR_G(r1) = regs->dat.raddr;
        }
        else
        {
            if (regs->dat.raddr <= 0x7FFFFFFF)
            {
                regs->GR_L(r1) = regs->dat.raddr;
            }
            else
            {
                /* Special handling if in 24-bit or 31-bit mode
                   and the returned address exceeds 2GB, or if
                   cc=3 and the returned address exceeds 2GB */
                if (cc == 0)
                {
                    /* Real address exceeds 2GB */
                    ARCH_DEP( program_interrupt )( regs,
                                PGM_SPECIAL_OPERATION_EXCEPTION );
                }

                /* Condition code is 1, 2, or 3, and the returned
                   table entry address exceeds 2GB.  Convert to
                   condition code 3 and return the exception code
                   which will be X'0010' or X'0011' */
                regs->GR_L(r1) = 0x80000000 | regs->dat.xcode;
                cc = 3;
            } /* end else(regs->dat.raddr) */
        } /* end else(amode) */
#else
        regs->GR_L(r1) = regs->dat.raddr;
#endif /*!defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )*/
    } /* end else(cc) */

    regs->psw.cc = cc;

} /* end ARCH_DEP(load_real_address_proc) */


/*-------------------------------------------------------------------*/
/* B24B LURA  - Load Using Real Address                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_using_real_address )
{
int     r1, r2;                         /* Values of R fields        */
RADR    n;                              /* Unsigned work             */

    RRE( inst, regs, r1, r2 );
    PER_ZEROADDR_CHECK( regs, r2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* R2 register contains operand real storage address */
    n = regs->GR(r2) & ADDRESS_MAXWRAP( regs );

    /* Program check if operand not on fullword boundary */
    FW_CHECK( n, regs );

    /* Load R1 register from second operand */
    regs->GR_L(r1) = ARCH_DEP( vfetch4 )( n, USE_REAL_ADDR, regs );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#if defined( FEATURE_LOCK_PAGE )
/*-------------------------------------------------------------------*/
/* B262 LKPG  - Lock Page                                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( lock_page )
{
int     r1, r2;                         /* Values of R fields        */
VADR    n2;                             /* effective addr of r2      */
RADR    rpte;                           /* PTE real address          */
CREG    pte;                            /* Page Table Entry          */

    RRE( inst, regs, r1, r2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    if (REAL_MODE( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    if (regs->GR_L(0) & LKPG_GPR0_RESV)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    n2 = regs->GR(r2) & ADDRESS_MAXWRAP( regs );

    /* Access to PTE must be serialized */
    OBTAIN_MAINLOCK_UNCONDITIONAL( regs );

    /* Return condition code 3 if translation exception */
    if (ARCH_DEP( translate_addr )( n2, r2, regs, ACCTYPE_PTE ) == 0)
    {
        rpte = APPLY_PREFIXING( regs->dat.raddr, regs->PX );

        pte =
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
              ARCH_DEP( fetch_doubleword_absolute )( rpte, regs );
#else
              ARCH_DEP( fetch_fullword_absolute   )( rpte, regs );
#endif

        if (regs->GR_L(0) & LKPG_GPR0_LOCKBIT)
        {
            /* Lock request */
            if (!(pte & PAGETAB_PGLOCK))
            {
                /* Return condition code 3 if translation exception */
                if (ARCH_DEP( translate_addr )( n2, r2, regs, ACCTYPE_LRA ))
                {
                    regs->psw.cc = 3;
                    RELEASE_MAINLOCK_UNCONDITIONAL( regs );
                    return;
                }

                pte |= PAGETAB_PGLOCK;
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                ARCH_DEP( store_doubleword_absolute )( pte, rpte, regs );
#else
                ARCH_DEP( store_fullword_absolute   )( pte, rpte, regs );
#endif
                regs->GR(r1) = regs->dat.raddr;
                regs->psw.cc = 0;
            }
            else
                regs->psw.cc = 1;
        }
        else
        {
            /* Unlock reguest */
            if (pte & PAGETAB_PGLOCK)
            {
                pte &= ~((U64)PAGETAB_PGLOCK);
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                ARCH_DEP( store_doubleword_absolute )( pte, rpte, regs );
#else
                ARCH_DEP( store_fullword_absolute   )( pte, rpte, regs );
#endif
                regs->psw.cc = 0;
            }
            else
                regs->psw.cc = 1;
        }

    }
    else
        regs->psw.cc = 3;

    RELEASE_MAINLOCK_UNCONDITIONAL( regs );

} /* end DEF_INST(lock_page) */
#endif /* defined( FEATURE_LOCK_PAGE ) */


#if defined( FEATURE_LINKAGE_STACK )
/*-------------------------------------------------------------------*/
/* B247 MSTA  - Modify Stacked State                           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( modify_stacked_state )
{
int     r1, r2;                         /* Values of R fields        */
U32     m1, m2;                         /* Modify values             */
LSED    lsed;                           /* Linkage stack entry desc. */
VADR    lsea;                           /* Linkage stack entry addr  */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    if (REAL_MODE( &regs->psw )
        || SECONDARY_SPACE_MODE( &regs->psw )
        || !ASF_ENABLED( regs ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    ODD_CHECK( r1, regs );

    /* Find the virtual address of the entry descriptor
       of the current state entry in the linkage stack */
    lsea = ARCH_DEP( locate_stack_entry )( 0, &lsed, regs );

    /* Load values from rightmost 32 bits of R1 and R1+1 registers */
    m1 = regs->GR_L( r1 + 0 );
    m2 = regs->GR_L( r1 + 1 );

    /* Store two 32-bit values into modifiable area of state entry */
    ARCH_DEP( stack_modify )( lsea, m1, m2, regs );
}
#endif /* defined( FEATURE_LINKAGE_STACK ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* DA   MVCP  - Move to Primary                               [SS-d] */
/*-------------------------------------------------------------------*/
DEF_INST( move_to_primary )
{
int     r1, r3;                         /* Register numbers          */
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     cc;                             /* Condition code            */
int     k;                              /* Integer workarea          */
GREG    l;                              /* Unsigned workarea         */

    SS( inst, regs, r1, r3, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Program check if secondary space control (CR0 bit 5) is 0,
       or if DAT is off, or if in AR mode or home-space mode */
    if ((regs->CR(0) & CR0_SEC_SPACE) == 0
        || REAL_MODE( &regs->psw )
        || AR_BIT( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Load true length from R1 register */
    l = GR_A( r1,regs );

    /* If the true length does not exceed 256, set condition code
       zero, otherwise set cc=3 and use effective length of 256 */
    if (l <= 256)
        cc = 0;
    else {
        cc = 3;
        l = 256;
    }

    /* Load secondary space key from R3 register bits 24-27 */
    k = regs->GR_L(r3) & 0xF0;

    /* Program check if in problem state and key mask in
       CR3 bits 0-15 is not 1 for the specified key */
    if (PROBSTATE( &regs->psw)
        && ((regs->CR(3) << (k >> 4)) & 0x80000000) == 0 )
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Move characters from secondary address space to primary
       address space using secondary key for second operand */
    if (l > 0)
        ARCH_DEP( move_chars )( effective_addr1, USE_PRIMARY_SPACE,
                    regs->psw.pkey,
                    effective_addr2, USE_SECONDARY_SPACE,
                    k, l-1, regs );

    /* Set condition code */
    regs->psw.cc = cc;
}
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* DB   MVCS  - Move to Secondary                             [SS-d] */
/*-------------------------------------------------------------------*/
DEF_INST( move_to_secondary )
{
int     r1, r3;                         /* Register numbers          */
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     cc;                             /* Condition code            */
int     k;                              /* Integer workarea          */
GREG    l;                              /* Unsigned workarea         */

    SS( inst, regs, r1, r3, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Program check if secondary space control (CR0 bit 5) is 0,
       or if DAT is off, or if in AR mode or home-space mode */
    if ((regs->CR(0) & CR0_SEC_SPACE) == 0
        || REAL_MODE( &regs->psw )
        || AR_BIT( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Load true length from R1 register */
    l = GR_A( r1,regs );

    /* If the true length does not exceed 256, set condition code
       zero, otherwise set cc=3 and use effective length of 256 */
    if (l <= 256)
        cc = 0;
    else {
        cc = 3;
        l = 256;
    }

    /* Load secondary space key from R3 register bits 24-27 */
    k = regs->GR_L(r3) & 0xF0;

    /* Program check if in problem state and key mask in
       CR3 bits 0-15 is not 1 for the specified key */
    if (PROBSTATE( &regs->psw )
        && ((regs->CR(3) << (k >> 4)) & 0x80000000) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Move characters from primary address space to secondary
       address space using secondary key for first operand */
    if (l > 0)
        ARCH_DEP( move_chars )( effective_addr1, USE_SECONDARY_SPACE, k,
                    effective_addr2, USE_PRIMARY_SPACE,
                    regs->psw.pkey, l-1, regs );

    /* Set condition code */
    regs->psw.cc = cc;
}
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


/*-------------------------------------------------------------------*/
/* E50F MVCDK - Move with Destination Key                      [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST( move_with_destination_key )
{
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     k, l;                           /* Integer workarea          */

    SSE( inst, regs, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Load operand length-1 from register 0 bits 24-31 */
    l = regs->GR_L(0) & 0xFF;

    /* Load destination key from register 1 bits 24-27 */
    k = regs->GR_L(1) & 0xF0;

    /* Program check if in problem state and key mask in
       CR3 bits 0-15 is not 1 for the specified key */
    if (PROBSTATE( &regs->psw )
        && ((regs->CR(3) << (k >> 4)) & 0x80000000) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Move characters using destination key for operand 1 */
    ARCH_DEP( move_chars )( effective_addr1, b1, k,
                effective_addr2, b2, regs->psw.pkey,
                l, regs );
}


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* D9   MVCK  - Move with Key                                 [SS-d] */
/*-------------------------------------------------------------------*/
DEF_INST( move_with_key )
{
int     r1, r3;                         /* Register numbers          */
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     cc;                             /* Condition code            */
int     k;                              /* Integer workarea          */
GREG    l;                              /* Unsigned workarea         */

    SS( inst, regs, r1, r3, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Load true length from R1 register */
    l = GR_A( r1,regs );

    /* If the true length does not exceed 256, set condition code
       zero, otherwise set cc=3 and use effective length of 256 */
    if (l <= 256)
        cc = 0;
    else {
        cc = 3;
        l = 256;
    }

    /* Load source key from R3 register bits 24-27 */
    k = regs->GR_L(r3) & 0xF0;

    /* Program check if in problem state and key mask in
       CR3 bits 0-15 is not 1 for the specified key */
    if (PROBSTATE( &regs->psw )
        && ((regs->CR(3) << (k >> 4)) & 0x80000000) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Move characters using source key for second operand */
    if (l > 0)
        ARCH_DEP( move_chars )( effective_addr1, b1, regs->psw.pkey,
                    effective_addr2, b2, k, l-1, regs );

    /* Set condition code */
    regs->psw.cc = cc;
}
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_027_MVCOS_FACILITY )
/*-------------------------------------------------------------------*/
/* C8x0 MVCOS - Move with Optional Specifications              [SSF] */
/*-------------------------------------------------------------------*/
DEF_INST( move_with_optional_specifications )
{
int     r3;                             /* Register number           */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     kbit1, kbit2, abit1, abit2;     /* Key and AS validity bits  */
int     key1, key2;                     /* Access keys in bits 0-3   */
int     asc1, asc2;                     /* AS controls (same as PSW) */
int     cc;                             /* Condition code            */
GREG    len;                            /* Effective length          */
int     space1, space2;                 /* Address space modifiers   */

    SSF( inst, regs, b1, effective_addr1, b2, effective_addr2, r3 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

    /* Program check if DAT is off */
    if (REAL_MODE( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Extract the access key and address-space control for operand 1 */
    abit1 =  regs->GR_LHH(0) & 0x0001;
    kbit1 = (regs->GR_LHH(0) & 0x0002) >> 1;
    asc1  =  regs->GR_LHH(0) & 0x00C0;
    key1  = (regs->GR_LHH(0) & 0xF000) >> 8;

    /* Extract the access key and address-space control for operand 2 */
    abit2 =  regs->GR_LHL(0) & 0x0001;
    kbit2 = (regs->GR_LHL(0) & 0x0002) >> 1;
    asc2  =  regs->GR_LHL(0) & 0x00C0;
    key2  = (regs->GR_LHL(0) & 0xF000) >> 8;

    /* Use PSW address-space control for operand 1 if A bit is zero */
    if (abit1 == 0)
        asc1 = regs->psw.asc;

    /* Use PSW address-space control for operand 2 if A bit is zero */
    if (abit2 == 0)
        asc2 = regs->psw.asc;

    /* Use PSW key for operand 1 if K bit is zero */
    if (kbit1 == 0)
        key1 = regs->psw.pkey;

    /* Use PSW key for operand 2 if K bit is zero */
    if (kbit2 == 0)
        key2 = regs->psw.pkey;

    /* Program check if home-space mode is specified for operand 1
       and PSW indicates problem state */
    if (abit1 && asc1 == PSW_HOME_SPACE_MODE
        && PROBSTATE( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Program check if secondary space control (CR0 bit 5/37) is 0, and
       secondary space mode is specified or implied for either operand */
    if ((regs->CR(0) & CR0_SEC_SPACE) == 0
        && (asc1 == PSW_SECONDARY_SPACE_MODE
            || asc2 == PSW_SECONDARY_SPACE_MODE))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Program check if in problem state and the key mask in CR3 is zero
       for the specified or implied access key for either operand */
    if (PROBSTATE( &regs->psw )
        && ( ((regs->CR(3) << (key1 >> 4)) & 0x80000000) == 0
            || ((regs->CR(3) << (key2 >> 4)) & 0x80000000) == 0 ))
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Load true length from R3 register */
    len = GR_A( r3,regs );

    /* If the true length does not exceed 4096, set condition code
       zero, otherwise set cc=3 and use effective length of 4096 */
    if (len <= 4096)
        cc = 0;
    else {
        cc = 3;
        len = 4096;
    }

    /* Set the address space modifier for operand 1 */
    space1 = (asc1 == PSW_PRIMARY_SPACE_MODE)   ? USE_PRIMARY_SPACE :
             (asc1 == PSW_SECONDARY_SPACE_MODE) ? USE_SECONDARY_SPACE :
             (asc1 == PSW_ACCESS_REGISTER_MODE) ? USE_ARMODE | b1 :
             (asc1 == PSW_HOME_SPACE_MODE)      ? USE_HOME_SPACE : 0;

    /* Set the address space modifier for operand 2 */
    space2 = (asc2 == PSW_PRIMARY_SPACE_MODE)   ? USE_PRIMARY_SPACE :
             (asc2 == PSW_SECONDARY_SPACE_MODE) ? USE_SECONDARY_SPACE :
             (asc2 == PSW_ACCESS_REGISTER_MODE) ? USE_ARMODE | b2 :
             (asc2 == PSW_HOME_SPACE_MODE)      ? USE_HOME_SPACE : 0;

    /* Perform the move */
    ARCH_DEP( move_charx )( effective_addr1, space1, key1,
                effective_addr2, space2, key2, len, regs );

    /* Set the condition code */
    regs->psw.cc = cc;

} /* end DEF_INST(move_with_optional_specifications) */
#endif /* defined( FEATURE_027_MVCOS_FACILITY ) */


/*-------------------------------------------------------------------*/
/* E50E MVCSK - Move with Source Key                           [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST( move_with_source_key )
{
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     k, l;                           /* Integer workarea          */

    SSE( inst, regs, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Load operand length-1 from register 0 bits 24-31 */
    l = regs->GR_L(0) & 0xFF;

    /* Load source key from register 1 bits 24-27 */
    k = regs->GR_L(1) & 0xF0;

    /* Program check if in problem state and key mask in
       CR3 bits 0-15 is not 1 for the specified key */
    if (PROBSTATE( &regs->psw )
        && ((regs->CR(3) << (k >> 4)) & 0x80000000) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Move characters using source key for second operand */
    ARCH_DEP( move_chars )( effective_addr1, b1, regs->psw.pkey,
                effective_addr2, b2, k, l, regs );
}


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* B218 PC    - Program Call                                     [S] */
/*-------------------------------------------------------------------*/
DEF_INST( program_call )
{
int     b2;                             /* Base of effective addr    */
U32     pcnum;                          /* Program call number       */
U32     pctea;                          /* TEA in case of program chk*/
VADR    effective_addr2;                /* Effective address         */
RADR    abs;                            /* Absolute address          */
BYTE   *mn;                             /* Mainstor address          */
RADR    pstd;                           /* Primary STD or ASCE       */
U32     oldpstd;                        /* Old Primary STD or ASCE   */
U32     ltdesig;                        /* Linkage table designation
                                           (LTD or LFTD)             */
U32     pasteo=0;                       /* Primary ASTE origin       */
RADR    lto;                            /* Linkage table origin      */
U32     ltl;                            /* Linkage table length      */
U32     lte;                            /* Linkage table entry       */
RADR    lfto;                           /* Linkage first table origin*/
U32     lftl;                           /* Linkage first table length*/
U32     lfte;                           /* Linkage first table entry */
RADR    lsto;                           /* Linkage second table orig */
U32     lste[2];                        /* Linkage second table entry*/
RADR    eto;                            /* Entry table origin        */
U32     etl;                            /* Entry table length        */
U32     ete[8];                         /* Entry table entry         */
int     numwords;                       /* ETE size (4 or 8 words)   */
int     i;                              /* Array subscript           */
int     ssevent = 0;                    /* 1=space switch event      */
U32     aste[16];                       /* ASN second table entry    */
U32     akm;                            /* Bits 0-15=AKM, 16-31=zero */
U16     xcode;                          /* Exception code            */
U16     pasn;                           /* Primary ASN               */
U16     oldpasn;                        /* Old Primary ASN           */

#if defined( FEATURE_LINKAGE_STACK )
U32     csi;                            /* Called-space identifier   */
VADR    retn;                           /* Return address and amode  */
#endif

#if defined( FEATURE_TRACING )
CREG    newcr12 = 0;                    /* CR12 upon completion      */
#endif

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
CREG    savecr12 = 0;                   /* CR12 save                 */
#endif

    S( inst, regs, b2, effective_addr2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    SIE_XC_INTERCEPT( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC2, PC ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Load the PC number from the operand address */
    if (!ASN_AND_LX_REUSE_ENABLED( regs ))
    {
        /* When ASN-and-LX-reuse is not installed or not enabled, the
           PC number is the low-order 20 bits of the operand address
           and the translation exception identification is the 20-bit
           PC number with 12 high order zeroes appended to the left */
        pcnum = effective_addr2 & (PC_LX | PC_EX);
        pctea = pcnum;
    }
    else /* ASN_AND_LX_REUSE_ENABLED */
    {
        /* When ASN-and-LX-reuse is installed and enabled by CR0,
           the PC number is loaded from the low-order 20 bits of the
           operand address (bits 44-63) if bit 44 is zero, otherwise
           a 31-bit PC number is constructed using bits 32-43 (LFX1)
           of the operand address concatenated with bits 45-63 (LFX2,
           LSX,EX) of the operand address.  The translation exception
           identification is either the 20 bit PC number with 12 high
           order zeroes, or, if bit 44 is one, is the entire 32 bits
           of the effective address including the 1 in bit 44 */
        if ((effective_addr2 & PC_BIT44) == 0)
        {
            pcnum = effective_addr2 & (PC_LFX2 | PC_LSX | PC_EX);
            pctea = pcnum;
        }
        else
        {
            pcnum = ((effective_addr2 & PC_LFX1) >> 1)
                    | (effective_addr2 & (PC_LFX2 | PC_LSX | PC_EX));
            pctea = effective_addr2 & 0xFFFFFFFF;
        }
    } /* end ASN_AND_LX_REUSE_ENABLED */

    /* Special operation exception if DAT is off, or if
       in secondary space mode or home space mode */
    if (REAL_MODE( &regs->psw ) || SPACE_BIT( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Save CR4 and CR1 in case of space switch */
    oldpasn = regs->CR(4) & CR4_PASN;
    oldpstd = regs->CR(1);

    /* [5.5.3.1] Load the linkage table designation */
    if (!ASF_ENABLED( regs ))
    {
        /* Special operation exception if in AR mode */
        if (ACCESS_REGISTER_MODE(&(regs->psw)))
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

        /* Obtain the LTD from control register 5 */
        ltdesig = regs->CR_L(5);
    }
    else
    {
        /* Obtain the primary ASTE origin from control register 5 */
        pasteo = regs->CR_L(5) & CR5_PASTEO;

        /* Convert the PASTE origin to an absolute address */
        abs = APPLY_PREFIXING( pasteo, regs->PX );

        /* Program check if PASTE is outside main storage */
        if (abs > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Fetch primary ASTE words 3 or 6 from absolute storage
           (note: the ASTE cannot cross a page boundary) */
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        aste[3] = ARCH_DEP( fetch_fullword_absolute )( abs+12, regs );
#else
        aste[6] = ARCH_DEP( fetch_fullword_absolute )( abs+24, regs );
#endif

        /* Load LTD or LFTD from primary ASTE word 3 or 6 */
        ltdesig = ASTE_LT_DESIGNATOR( aste );
    }

    /* Note: When ASN-and-LX-reuse is installed and enabled
       by CR0, ltdesig is an LFTD, otherwise it is an LTD */

    /* Special operation exception if subsystem linkage
       control bit in linkage table designation is zero */
    if ((ltdesig & LTD_SSLINK) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

#if defined( FEATURE_TRACING )
    /* Form trace entry if ASN tracing is active */
    if (regs->CR(12) & CR12_ASNTRACE)
        newcr12 = ARCH_DEP(trace_pc) (pctea, regs);
#endif

    /* [5.5.3.2] Linkage table lookup */
    if (!ASN_AND_LX_REUSE_ENABLED( regs ))
    {
        /* Extract the linkage table origin and length from the LTD */
        lto = ltdesig & LTD_LTO;
        ltl = ltdesig & LTD_LTL;

        /* Program check if linkage index outside the linkage table */
        if (ltl < ((pcnum & PC_LX) >> 13))
        {
            regs->TEA = pctea;
            ARCH_DEP( program_interrupt )( regs, PGM_LX_TRANSLATION_EXCEPTION );
        }

        /* Calculate the address of the linkage table entry */
        lto += (pcnum & PC_LX) >> 6;
        lto &= 0x7FFFFFFF;

        /* Program check if linkage table entry outside real storage */
        if (lto > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Fetch linkage table entry from real storage.  All bytes
           must be fetched concurrently as observed by other CPUs */
        lto = APPLY_PREFIXING( lto, regs->PX );
        lte = ARCH_DEP( fetch_fullword_absolute )( lto, regs );

        /* Program check if the LX invalid bit is set */
        if (lte & LTE_INVALID)
        {
            regs->TEA = pctea;
            ARCH_DEP( program_interrupt )( regs, PGM_LX_TRANSLATION_EXCEPTION );
        }

        /* Extract the entry table origin and length from the LTE */
        eto = lte & LTE_ETO;
        etl = lte & LTE_ETL;

    }
    else /* ASN_AND_LX_REUSE_ENABLED */
    {
        /* Extract linkage first table origin and length from LFTD */
        lfto = ltdesig & LFTD_LFTO;
        lftl = ltdesig & LFTD_LFTL;

        /* If the linkage first index exceeds the length of the
           linkage first table, then generate a program check.
           The index exceeds the table length if the LFX1 (which
           is now in bits 1-12 of the 32-bit PC number) exceeds the
           LFTL. Since the LFTL is only 8 bits, this also implies
           that the first 4 bits of the LFX1 (originally bits 32-35
           of the operand address) must always be 0. The LFX1 was
           loaded from bits 32-43 of the operand address if bit 44
           of the operand address was 1, otherwise LFX1 is zero.
           However, when bit 44 of the effective address is zero,
           the LFTL (Linkage-First-Table Length) is ignored. */
        if ((effective_addr2 & PC_BIT44) && lftl < (pcnum >> 19))
        {
            regs->TEA = pctea;
            ARCH_DEP( program_interrupt )( regs, PGM_LFX_TRANSLATION_EXCEPTION );
        }

        /* Calculate the address of the linkage first table entry
           (it is always a 31-bit address even in ESAME) */
        lfto += (pcnum & ((PC_LFX1>>1)|PC_LFX2)) >> (13-2);
        lfto &= 0x7FFFFFFF;

        /* Program check if the LFTE address is outside real storage */
        if (lfto > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Fetch linkage first table entry from real storage.  All bytes
           must be fetched concurrently as observed by other CPUs */
        lfto = APPLY_PREFIXING( lfto, regs->PX );
        lfte = ARCH_DEP( fetch_fullword_absolute )( lfto, regs );

        /* Program check if the LFX invalid bit is set */
        if (lfte & LFTE_INVALID)
        {
            regs->TEA = pctea;
            ARCH_DEP( program_interrupt )( regs, PGM_LFX_TRANSLATION_EXCEPTION );
        }

        /* Extract the linkage second table origin from the LFTE */
        lsto = lfte & LFTE_LSTO;

        /* Calculate the address of the linkage second table entry
           (it is always a 31-bit address even in ESAME) */
        lsto += (pcnum & PC_LSX) >> (8-3);
        lsto &= 0x7FFFFFFF;

        /* Program check if the LSTE address is outside real storage */
        if (lsto > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Fetch the linkage second table entry from real storage.
           The LSTE is 2 fullwords and cannot cross a page boundary.
           All 8 bytes of the LSTE must be fetched concurrently as
           observed by other CPUs */
        abs = APPLY_PREFIXING( lsto, regs->PX );
        mn = FETCH_MAIN_ABSOLUTE( abs, regs, 2 * 4 );
        lste[0] = fetch_fw( mn + 0 );
        lste[1] = fetch_fw( mn + 4 );

        /* Program check if the LSX invalid bit is set */
        if (lste[0] & LSTE0_INVALID)
        {
            regs->TEA = pctea;
            ARCH_DEP( program_interrupt )( regs, PGM_LSX_TRANSLATION_EXCEPTION );
        }

        /* Program check if the LSTESN in word 1 of the LSTE is
           non-zero and not equal to bits 0-31 of register 15 */
        if (lste[1] != 0 && regs->GR_H(15) != lste[1])
        {
            regs->TEA = pctea;
            ARCH_DEP( program_interrupt )( regs, PGM_LSTE_SEQUENCE_EXCEPTION );
        }

        /* Extract the entry table origin and length from the LSTE */
        eto = lste[0] & LSTE0_ETO;
        etl = lste[0] & LSTE0_ETL;

    } /* end ASN_AND_LX_REUSE_ENABLED */

    /* [5.5.3.3] Entry table lookup */

    /* Program check if entry index is outside the entry table */
    if (etl < ((pcnum & PC_EX) >> (8-6)))
    {
        regs->TEA = pctea;
        ARCH_DEP( program_interrupt )( regs, PGM_EX_TRANSLATION_EXCEPTION );
    }

    /* Calculate the starting address of the entry table entry
       (it is always a 31-bit address even in ESAME) */
    eto += (pcnum & PC_EX) << (ASF_ENABLED( regs ) ? 5 : 4);
    eto &= 0x7FFFFFFF;

    /* Determine the size of the entry table entry */
    numwords = ASF_ENABLED( regs ) ? 8 : 4;

    /* Program check if entry table entry is outside main storage */
    abs = APPLY_PREFIXING( eto, regs->PX );
    if (abs > regs->mainlim - (numwords * 4))
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

    /* Fetch the 4- or 8-word entry table entry from real
       storage.  Each fullword of the ETE must be fetched
       concurrently as observed by other CPUs.  The entry
       table entry cannot cross a page boundary. */
    mn = FETCH_MAIN_ABSOLUTE( abs, regs, numwords * 4 );
    for (i=0; i < numwords; i++)
    {
        ete[i] = fetch_fw( mn );
        mn += 4;
    }

    /* Clear remaining words if fewer than 8 words were loaded */
    while (i < 8) ete[i++] = 0;

    /* Program check if basic program call in AR mode */
    if ((ete[4] & ETE4_T) == 0 && AR_BIT( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Program check if basic program call is attempting
       to switch into or out of 64-bit addressing mode */
    if ((ete[4] & ETE4_T) == 0
        && ((ete[4] & ETE4_G) ? 1 : 0) != regs->psw.amode64)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
#endif
    /* Program check if resulting addressing mode is 24 and the
       entry instruction address is not a 24-bit address */
    if ((ete[1] & ETE1_AMODE) == 0
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        && (ete[4] & ETE4_G) == 0
#endif
        && (ete[1] & ETE1_EIA) > 0x00FFFFFF)
        ARCH_DEP( program_interrupt )( regs, PGM_PC_TRANSLATION_SPECIFICATION_EXCEPTION );

    /* Obtain the authorization key mask from the entry table */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    akm = ete[2] & ETE2_AKM;
#else
    akm = ete[0] & ETE0_AKM;
#endif

    /* Program check if in problem state and the PKM in control
       register 3 produces zero when ANDed with the AKM in the ETE */
    if (PROBSTATE( &regs->psw )
        && ((regs->CR(3) & CR3_KEYMASK) & akm) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Obtain the new primary ASN from the entry table */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    pasn = ete[2] & ETE2_ASN;
#else
    pasn = ete[0] & ETE0_ASN;
#endif

    /* Obtain the ASTE if ASN is non-zero */
    if (pasn != 0)
    {
        /* Program check if ASN translation control is zero */
        if ((regs->CR(14) & CR14_ASN_TRAN) == 0)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

        /* For ESA/390 when ASF control is not enabled, the ASTE
           is obtained by ASN translation.  For ESAME, and for
           ESA/390 when ASF control is enabled, the ASTE is loaded
           using the ASTE real address from the entry table */
        if (!ASF_ENABLED( regs ))
        {
            /* Perform ASN translation to obtain ASTE */
            xcode = ARCH_DEP( translate_asn )( pasn, regs, &pasteo, aste );

            /* Program check if ASN translation exception */
            if (xcode != 0)
                ARCH_DEP( program_interrupt )( regs, xcode );
        }
        else
        {
            /* Load the ASTE origin from the entry table */
            pasteo = ete[5] & ETE5_ASTE;

            /* Convert the ASTE origin to an absolute address */
            abs = APPLY_PREFIXING( pasteo, regs->PX );

            /* Program check if ASTE origin address is invalid */
            if (abs > regs->mainlim)
                ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

            /* Fetch the 16-word ASTE from absolute storage
               (note: the ASTE cannot cross a page boundary) */
            mn = FETCH_MAIN_ABSOLUTE( abs, regs, 64 );
            for (i=0; i < 16; i++)
            {
                aste[i] = fetch_fw( mn );
                mn += 4;
            }

            /* ASX translation exception if ASTE invalid bit is one */
            if (aste[0] & ASTE0_INVALID)
            {
                regs->TEA = pasn;
                ARCH_DEP( program_interrupt )( regs, PGM_ASX_TRANSLATION_EXCEPTION );
            }
        }

        /* Obtain the new PSTD or PASCE from the ASTE */
        pstd = ASTE_AS_DESIGNATOR( aste );

#if defined( FEATURE_SUBSPACE_GROUP )
        /* Perform subspace replacement on new PSTD */
        pstd = ARCH_DEP( subspace_replace )( pstd, pasteo, NULL, regs );
#endif

    } /* end if (PC-ss) */
    else
    { /* PC-cp */

        /* For PC to current primary, load current primary STD */
        pstd = regs->CR(1);

    } /* end if (PC-cp) */

    /* Perform basic or stacking program call */
    if ((ete[4] & ETE4_T) == 0)
    {
        /* For basic PC, load linkage info into general register 14 */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            regs->GR_G(14) = PSW_IA_FROM_IP( regs, 0 ) | PROBSTATE( &regs->psw );
        else
#endif
            regs->GR_L(14) = (regs->psw.amode ? 0x80000000 : 0)
                            | PSW_IA_FROM_IP( regs, 0 ) | PROBSTATE( &regs->psw );

        /* Set the breaking event address register */
        SET_BEAR_REG( regs, regs->ip - 4 );

        /* Update the PSW from the entry table */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            SET_PSW_IA_AND_MAYBE_IP( regs, ((U64)(ete[0]) << 32)
                             | (U64)(ete[1] & 0xFFFFFFFE) );
        else
        {
            regs->psw.amode = (ete[1] & ETE1_AMODE) ? 1 : 0;
            regs->psw.AMASK = regs->psw.amode ? AMASK31 : AMASK24;
            SET_PSW_IA_AND_MAYBE_IP( regs, ete[1] & ETE1_EIA );
        }
#else
        regs->psw.amode = (ete[1] & ETE1_AMODE) ? 1 : 0;
        regs->psw.AMASK = regs->psw.amode ? AMASK31 : AMASK24;
        SET_PSW_IA_AND_MAYBE_IP( regs, ete[1] & ETE1_EIA );
#endif
        if (ete[1] & ETE1_PROB)
            regs->psw.states |=  BIT( PSW_PROB_BIT );
        else
            regs->psw.states &= ~BIT( PSW_PROB_BIT );

        /* Load the current PKM and PASN into general register 3 */
        regs->GR_L(3) = (regs->CR(3) & CR3_KEYMASK)
                        | (regs->CR(4) & CR4_PASN);

        /* OR the EKM into the current PKM */
        regs->CR(3) |= (ete[3] & ETE3_EKM);

        /* Load the entry parameter into general register 4 */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            regs->GR_H(4) = ete[6];
        regs->GR_L(4) = ete[7];
#else
        regs->GR_L(4) = ete[2];
#endif

    } /* end if (basic PC) */
    else
#if defined( FEATURE_LINKAGE_STACK )
    { /* stacking PC */

        /* ESA/390 POP Fig 10-17 8.B.11 */
        if (!ASF_ENABLED( regs ))
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

#if defined( FEATURE_TRACING )
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* Add a mode trace entry when switching in/out of 64 bit mode */
        if ((regs->CR(12) & CR12_MTRACE) && (regs->psw.amode64 != ((ete[4] & ETE4_G) ? 1 : 0)))
        {
            /* since ASN trace might be made already, need to save
               current CR12 and use newcr12 for this second entry */
            if (!newcr12)
                newcr12 = regs->CR(12);
            savecr12 = regs->CR(12);
            regs->CR(12) = newcr12;
            newcr12 = ARCH_DEP( trace_ms )( 0, 0, regs );
            regs->CR(12) = savecr12;
        }
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
#endif /* defined( FEATURE_TRACING ) */

        /* Set the called-space identification */
        if (pasn == 0)
            csi = 0;
        else if (ASN_AND_LX_REUSE_ENABLED( regs ))
            csi = pasn << 16 | (aste[11] & 0x0000FFFF);
        else
            csi = pasn << 16 | (aste[5] & 0x0000FFFF);

        /* Set the addressing mode bits in the return address */
        retn = PSW_IA_FROM_IP( regs, 0 );
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            retn |= 0x01;
        else
#endif
        if (regs->psw.amode)
            retn |= 0x80000000;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* Set the high-order bit of the PC number if
           the resulting addressing mode is 64-bit */
        if (ete[4] & ETE4_G)
            pcnum |= 0x80000000;
#endif
        /* Perform the stacking process */
        ARCH_DEP( form_stack_entry )( LSED_UET_PC, retn, 0, csi,
                                        pcnum, regs );

        /* Set the breaking event address register */
        SET_BEAR_REG( regs, regs->ip - 4 );

        /* Update the PSW from the entry table */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (ete[4] & ETE4_G)
        {
            regs->psw.amode64 = 1;
            regs->psw.amode = 1;
            regs->psw.AMASK = AMASK64;
            SET_PSW_IA_AND_MAYBE_IP( regs, ((U64)(ete[0]) << 32)
                                | (U64)(ete[1] & 0xFFFFFFFE) );
        }
        else
        {
            regs->psw.amode64 = 0;
            regs->psw.amode = (ete[1] & ETE1_AMODE) ? 1 : 0;
            regs->psw.AMASK = regs->psw.amode ? AMASK31 : AMASK24;
            SET_PSW_IA_AND_MAYBE_IP( regs, ete[1] & ETE1_EIA );
        }
#else
        regs->psw.amode = (ete[1] & ETE1_AMODE) ? 1 : 0;
        regs->psw.AMASK = regs->psw.amode ? AMASK31 : AMASK24;
        SET_PSW_IA_AND_MAYBE_IP( regs, ete[1] & ETE1_EIA );
#endif
        if (ete[1] & ETE1_PROB)
            regs->psw.states |=  BIT( PSW_PROB_BIT );
        else
            regs->psw.states &= ~BIT( PSW_PROB_BIT );

        /* Replace the PSW key by the entry key if the K bit is set */
        if (ete[4] & ETE4_K)
        {
            regs->psw.pkey = (ete[4] & ETE4_EK) >> 16;
        }

        /* Replace the PSW key mask by the EKM if the M bit is set,
           otherwise OR the EKM into the current PSW key mask */
        if (ete[4] & ETE4_M)
            regs->CR_LHH(3) = 0;
        regs->CR(3) |= (ete[3] & ETE3_EKM);

        /* Replace the EAX key by the EEAX if the E bit is set */
        if (ete[4] & ETE4_E)
        {
            regs->CR_LHH(8) = (ete[4] & ETE4_EEAX);
        }

        /* Set the access mode according to the C bit */
        if (ete[4] & ETE4_C)
            regs->psw.asc |=  BIT( PSW_AR_BIT );
        else
            regs->psw.asc &= ~BIT( PSW_AR_BIT );

        /* Load the entry parameter into general register 4 */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            regs->GR_H(4) = ete[6];
        regs->GR_L(4) = ete[7];
#else
        regs->GR_L(4) = ete[2];
#endif

    } /* end if (stacking PC) */
#else
    ARCH_DEP( program_interrupt )( regs, PGM_PC_TRANSLATION_SPECIFICATION_EXCEPTION );
#endif /*!defined( FEATURE_LINKAGE_STACK )*/

    /* If new ASN is zero, perform program call to current primary */
    if (pasn == 0)
    {
        /* Set SASN equal to PASN */
        regs->CR_LHL(3) = regs->CR_LHL(4);

        /* Set SSTD equal to PSTD */
        regs->CR(7) = regs->CR(1);

        /* When ASN-and-LX-reuse is installed and enabled,
           set the SASTEIN equal to the PASTEIN */
        if (ASN_AND_LX_REUSE_ENABLED( regs ))
            regs->CR_H(3) = regs->CR_H(4);

    } /* end if (PC-cp) */
    else
    { /* Program call with space switching */

        /* Set SASN and SSTD equal to current PASN and PSTD */
        regs->CR_LHL(3) = regs->CR_LHL(4);
        regs->CR(7) = regs->CR(1);

        /* When ASN-and-LX-reuse is installed and enabled,
           set the SASTEIN equal to the current PASTEIN */
        if (ASN_AND_LX_REUSE_ENABLED( regs ))
            regs->CR_H(3) = regs->CR_H(4);

        /* Set flag if either the current or new PSTD indicates
           a space switch event */
        if ((regs->CR(1) & SSEVENT_BIT)
            || (pstd & SSEVENT_BIT) )
        {
            /* Indicate space-switch event required */
            ssevent = 1;
        }

        /* Obtain new AX from the ASTE and new PASN from the ET */
        regs->CR_L(4) = (aste[1] & ASTE1_AX) | pasn;

        /* When ASN-and-LX-reuse is installed and enabled,
           obtain the new PASTEIN from the new primary ASTE */
        if (ASN_AND_LX_REUSE_ENABLED( regs ))
            regs->CR_H(4) = aste[11];

        /* Load the new primary STD or ASCE */
        regs->CR(1) = pstd;

        /* Update control register 5 with the new PASTEO or LTD */
        regs->CR_L(5) = ASF_ENABLED( regs ) ?
                                pasteo : ASTE_LT_DESIGNATOR( aste );

#if defined( FEATURE_LINKAGE_STACK )
        /* For stacking PC when the S-bit in the entry table is
           one, set SASN and SSTD equal to new PASN and PSTD */
        if ((ete[4] & ETE4_T) && (ete[4] & ETE4_S))
        {
            regs->CR_LHL(3) = regs->CR_LHL(4);
            regs->CR(7) = regs->CR(1);

            /* When ASN-and-LX-reuse is installed and enabled,
               also set the SASTEIN equal to the new PASTEIN */
            if (ASN_AND_LX_REUSE_ENABLED( regs ))
                regs->CR_H(3) = regs->CR_H(4);
        }
#endif /* defined( FEATURE_LINKAGE_STACK ) */

    } /* end if (PC-ss) */

#if defined( FEATURE_TRACING )
    /* Update trace table address if ASN or Mode switch made trace entry */
    if (newcr12)
        regs->CR(12) = newcr12;
#endif

    /* Update cpu states */
    SET_IC_MASK( regs );
    SET_AEA_MODE( regs );             // psw.asc may be updated
    SET_AEA_COMMON( regs );           // cr[1], cr[7] may be updated
    INVALIDATE_AIA( regs );

    /* Check for Successful Branch PER event */
    PER_SB( regs, regs->psw.IA );

    /* Generate space switch event if required */
    if (ssevent || (pasn != 0 && IS_IC_PER( regs )))
    {
        /* [6.5.2.34] Set the translation exception address equal
           to the old primary ASN, with the high-order bit set if
           the old primary space-switch-event control bit is one */
        regs->TEA = oldpasn;
        if (oldpstd & SSEVENT_BIT)
            regs->TEA |= TEA_SSEVENT;
        ARCH_DEP( program_interrupt )( regs, PGM_SPACE_SWITCH_EVENT );
    }

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, 0x1802 ); // GR 3, 4 and 14

} /* end DEF_INST(program_call) */
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_LINKAGE_STACK )
/*-------------------------------------------------------------------*/
/* 0101 PR    - Program Return                                   [E] */
/*-------------------------------------------------------------------*/
DEF_INST( program_return )
{
REGS    newregs;                        /* Copy of CPU registers     */
int     etype;                          /* Entry type unstacked      */
int     ssevent = 0;                    /* 1=space switch event      */
RADR    alsed;                          /* Absolute addr of LSED of
                                           previous stack entry      */
LSED   *lsedp;                          /* -> LSED in main storage   */
U32     aste[16];                       /* ASN second table entry    */
U32     pasteo=0;                       /* Primary ASTE origin       */
U32     sasteo=0;                       /* Secondary ASTE origin     */
U16     oldpasn;                        /* Original primary ASN      */
U32     oldpstd;                        /* Original primary STD      */
U16     pasn = 0;                       /* New primary ASN           */
U16     sasn;                           /* New secondary ASN         */
U16     ax;                             /* Authorization index       */
U16     xcode;                          /* Exception code            */
int     rc;                             /* return code from load_psw */

    E( inst, regs );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    UNREFERENCED( inst );
    SIE_XC_INTERCEPT( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC3, PR ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    INVALIDATE_AIA( regs );

    /* Create a working copy of the CPU registers... */
    memcpy( &newregs, regs, sysblk.regs_copy_len );

    /* Now INVALIDATE ALL TLB ENTRIES in our working copy.. */
    memset( &newregs.tlb.vaddr, 0, TLBN * sizeof(DW) );
    newregs.tlbID = 1;

    /* Set the breaking event address register in the copy */
    SET_BEAR_REG( &newregs, newregs.ip - (likely( !newregs.execflag ) ? 2 :
                                        newregs.exrl ? 6 : 4) );

    /* Save the primary ASN (CR4) and primary STD (CR1) */
    oldpasn = regs->CR_LHL(4);
    oldpstd = regs->CR(1);

    /* Perform the unstacking process */
    etype = ARCH_DEP( program_return_unstack )( &newregs, &alsed, &rc );

#if defined( FEATURE_TRACING )
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* If unstacked entry was a BAKR:                              */
        /* Add a mode trace entry when switching in/out of 64 bit mode */
    if ((etype == LSED_UET_BAKR)
        && (regs->CR(12) & CR12_MTRACE)
        && (regs->psw.amode64 != newregs.psw.amode64))
        newregs.CR(12) = ARCH_DEP(trace_ms) (0, 0, regs);
#endif
#endif
    /* Perform PR-cp or PR-ss if unstacked entry was a program call */
    if (etype == LSED_UET_PC)
    {
        /* Extract the new primary ASN from CR4 bits 16-31 */
        pasn = newregs.CR_LHL(4);

#if defined( FEATURE_TRACING )
        /* Perform tracing if ASN tracing is on */
        if (regs->CR(12) & CR12_ASNTRACE)
            newregs.CR(12) = ARCH_DEP(trace_pr) (&newregs, regs);

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        else
        /* Add a mode trace entry when switching in/out of 64 bit mode */
        if ((regs->CR(12) & CR12_MTRACE) && (regs->psw.amode64 != newregs.psw.amode64))
            newregs.CR(12) = ARCH_DEP(trace_ms) (0, 0, regs);
#endif
#endif /* defined( FEATURE_TRACING ) */

        /* Perform PASN translation if new PASN not equal old PASN */
        if (pasn != oldpasn)
        {
            /* Special operation exception if ASN translation
               control (control register 14 bit 12) is zero */
            if ((regs->CR(14) & CR14_ASN_TRAN) == 0)
                ARCH_DEP( program_interrupt )( &newregs, PGM_SPECIAL_OPERATION_EXCEPTION );

            /* Translate new primary ASN to obtain ASTE */
            xcode = ARCH_DEP( translate_asn )( pasn, &newregs, &pasteo, aste );

            /* Program check if ASN translation exception */
            if (xcode != 0)
                ARCH_DEP( program_interrupt )( &newregs, xcode );

            /* When ASN-and-LX-reuse is installed and enabled by CR0,
               the PASTEIN previously loaded from the state entry (by
               the program_return_unstack procedure) into the high word
               of CR4 must equal the ASTEIN in word 11 of the ASTE */
            if (ASN_AND_LX_REUSE_ENABLED( regs ))
            {
                if (newregs.CR_H(4) != aste[11])
                {
                    /* Set bit 2 of the exception access identification
                       to indicate that the program check occurred
                       during PASN translation in a PR instruction */
                    newregs.excarid = 0x20;
                    ARCH_DEP( program_interrupt )( &newregs, PGM_ASTE_INSTANCE_EXCEPTION );
                }
            } /* end if (ASN_AND_LX_REUSE_ENABLED) */

            /* Obtain new PSTD (or PASCE) and AX from the ASTE */
            newregs.CR(1) = ASTE_AS_DESIGNATOR( aste );
            newregs.CR_LHH(4) = 0;
            newregs.CR_L(4) |= aste[1] & ASTE1_AX;

            /* Load CR5 with the primary ASTE origin address */
            newregs.CR_L(5) = pasteo;

#if defined( FEATURE_SUBSPACE_GROUP )
            /* Perform subspace replacement on new PSTD */
            newregs.CR(1) = ARCH_DEP( subspace_replace )( newregs.CR(1),
                                            pasteo, NULL, &newregs );
#endif

            /* Space switch if either current PSTD or new PSTD
               space-switch-event control bit is set to 1 */
            if ((regs->CR(1) & SSEVENT_BIT)
                || (newregs.CR(1) & SSEVENT_BIT))
            {
                /* Indicate space-switch event required */
                ssevent = 1;
            }
            else
            {
                /* space-switch event maybe - if PER event */
                ssevent = 2;
            }

        } /* end if (pasn!=oldpasn) */

        /* Extract the new secondary ASN from CR3 bits 16-31 */
        sasn = newregs.CR_LHL(3);

        /* Set SSTD = PSTD if new SASN is equal to new PASN */
        if (sasn == pasn)
        {
            newregs.CR(7) = newregs.CR(1);
        }
        else /* sasn != pasn */
        {
            /* Perform SASN translation */

            /* Special operation exception if ASN translation
               control (control register 14 bit 12) is zero */
            if ((regs->CR(14) & CR14_ASN_TRAN) == 0)
                ARCH_DEP( program_interrupt )( &newregs, PGM_SPECIAL_OPERATION_EXCEPTION );

            /* Translate new secondary ASN to obtain ASTE */
            xcode = ARCH_DEP( translate_asn )( sasn, &newregs, &sasteo, aste );

            /* Program check if ASN translation exception */
            if (xcode != 0)
                ARCH_DEP( program_interrupt )( &newregs, xcode );

            /* When ASN-and-LX-reuse is installed and enabled by CR0,
               the SASTEIN previously loaded from the state entry (by
               the program_return_unstack procedure) into the high word
               of CR3 must equal the ASTEIN in word 11 of the ASTE */
            if (ASN_AND_LX_REUSE_ENABLED( regs ))
            {
                if (newregs.CR_H(3) != aste[11])
                {
                    /* Set bit 3 of the exception access identification
                       to indicate that the program check occurred
                       during SASN translation in a PR instruction */
                    newregs.excarid = 0x10;
                    ARCH_DEP( program_interrupt )( &newregs, PGM_ASTE_INSTANCE_EXCEPTION );
                }
            } /* end if (ASN_AND_LX_REUSE_ENABLED) */

            /* Obtain new SSTD or SASCE from secondary ASTE */
            newregs.CR(7) = ASTE_AS_DESIGNATOR( aste );

            /* Perform SASN authorization using new AX */
            ax = newregs.CR_LHH(4);
            if (ARCH_DEP( authorize_asn )( ax, aste, ATE_SECONDARY, &newregs ))
            {
                newregs.TEA = sasn;
                ARCH_DEP( program_interrupt )( &newregs, PGM_SECONDARY_AUTHORITY_EXCEPTION );
            }

#if defined( FEATURE_SUBSPACE_GROUP )
            /* Perform subspace replacement on new SSTD */
            newregs.CR(7) = ARCH_DEP( subspace_replace )( newregs.CR(7),
                                            sasteo, NULL, &newregs );
#endif

        } /* end else(sasn!=pasn) */

    } /* end if (LSED_UET_PC) */

    /* Update the updated CPU registers from the working copy */
    memcpy( &regs->psw,      &newregs.psw,       sizeof( newregs.psw       ));
    memcpy(  regs->gr,        newregs.gr,        sizeof( newregs.gr        ));
    memcpy(  regs->cr_struct, newregs.cr_struct, sizeof( newregs.cr_struct ));
    memcpy(  regs->ar,        newregs.ar,        sizeof( newregs.ar        ));
    regs->bear = newregs.bear;

    /* Set the main storage reference and change bits */
    ARCH_DEP( or_storage_key )( alsed, (STORKEY_REF | STORKEY_CHANGE) );

    /* [5.12.4.4] Clear the next entry size field of the linkage
       stack entry now pointed to by control register 15 */
    lsedp = (LSED*)(regs->mainstor + alsed);
    lsedp->nes[0] = 0;
    lsedp->nes[1] = 0;

#if defined( FEATURE_PER )

    /* Copy PER info from working copy to real copy of registers */
    if (1
        && IS_IC_PER_SA( &newregs )
        && !IS_PER_SUPRESS( &newregs, CR9_SA )
    )
    {
        ON_IC_PER_SA( regs );
        regs->perc = newregs.perc;
    }

    PER_SB( regs, regs->psw.IA );

#endif /* defined( FEATURE_PER ) */

    /* Update cpu states */
    SET_IC_MASK( regs );
    SET_AEA_MODE( regs );             // psw has been updated
    SET_AEA_COMMON( regs );           // control regs been updated

    /* Generate space switch event if required */
    if (ssevent == 1 || (ssevent == 2 && IS_IC_PER( regs )))
    {
        /* [6.5.2.34] Set translation exception address equal
           to old primary ASN, and set high-order bit if old
           primary space-switch-event control bit is one */
        regs->TEA = oldpasn;
        if (oldpstd & SSEVENT_BIT)
            regs->TEA |= TEA_SSEVENT;
         ARCH_DEP( program_interrupt )( regs, PGM_SPACE_SWITCH_EVENT );
    }

    if (rc) /* if new psw has bad format */
    {
        ARCH_DEP( program_interrupt )( regs, rc );
    }

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, 0x3FFE );  // GR 2-14

    RETURN_INTCHECK( regs );

} /* end DEF_INST(program_return) */
#endif /* defined( FEATURE_LINKAGE_STACK ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* Common processing routine for the PT and PTI instructions         */
/*-------------------------------------------------------------------*/
void ARCH_DEP( program_transfer_proc )( REGS* regs,
        int r1, int r2, int pti_instruction )
{
U16     pkm;                            /* New program key mask      */
U16     pasn;                           /* New primary ASN           */
U16     oldpasn;                        /* Old primary ASN           */
int     amode;                          /* New amode                 */
VADR    ia;                             /* New instruction address   */
int     prob;                           /* New problem state bit     */
RADR    abs;                            /* Absolute address          */
U32     ltd;                            /* Linkage table designation */
U32     pasteo=0;                       /* Primary ASTE origin       */
U32     aste[16];                       /* ASN second table entry    */
CREG    pstd;                           /* Primary STD               */
U32     oldpstd;                        /* Old Primary STD           */
U16     ax;                             /* Authorization index       */
U16     xcode;                          /* Exception code            */
int     ssevent = 0;                    /* 1=space switch event      */
#if defined( FEATURE_TRACING )
CREG    newcr12 = 0;                    /* CR12 upon completion      */
#endif

    SIE_XC_INTERCEPT( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC2, PT ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Special operation exception if DAT is off, or
       not in primary space mode */
    if (REAL_MODE( &regs->psw )
        || !PRIMARY_SPACE_MODE( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Special operation exception if subsystem linkage
       control bit in CR5 is zero (when ASF is off)*/
    if (!ASF_ENABLED( regs ) && !(regs->CR_L(5) & LTD_SSLINK))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Save the primary ASN (CR4) and primary STD (CR1) */
    oldpasn = regs->CR_LHL(4);
    oldpstd = regs->CR(1);

    /* Extract the PSW key mask from R1 register bits 0-15 */
    pkm = regs->GR_LHH(r1);

    /* Extract the ASN from R1 register bits 16-31 */
    pasn = regs->GR_LHL(r1);

#if defined( FEATURE_TRACING )
    /* Build trace entry if ASN tracing is on */
    if (regs->CR(12) & CR12_ASNTRACE)
        newcr12 = ARCH_DEP( trace_pt )( pti_instruction, pasn, regs->GR(r2), regs );
#endif

    /* Determine instruction address, amode, and problem state */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (regs->psw.amode64)
    {
        /* In 64-bit address mode, extract instruction address from
           R2 register bits 0-62, and leave address mode unchanged */
        ia = regs->GR_G(r2) & 0xFFFFFFFFFFFFFFFEULL;
        amode = regs->psw.amode;
    }
    else
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
    {
        /* In 31- or 24-bit mode, extract new amode from R2 bit 0 */
        amode = (regs->GR_L(r2) & 0x80000000) ? 1 : 0;

        /* Extract the instruction address from R2 bits 1-30 */
        ia = regs->GR_L(r2) & 0x7FFFFFFE;
    }

    /* Extract the problem state bit from R2 register bit 31 */
    prob = regs->GR_L(r2) & 0x00000001;

    /* [5.5.3.1] Load the linkage table designation */
    if (!ASF_ENABLED( regs ))
    {
        /* Obtain the LTD from control register 5 */
        ltd = regs->CR_L(5);
    }
    else
    {
        /* Obtain the primary ASTE origin from control register 5 */
        pasteo = regs->CR_L(5) & CR5_PASTEO;

        /* Convert the PASTE origin to an absolute address */
        abs = APPLY_PREFIXING( pasteo, regs->PX );

        /* Program check if PASTE is outside main storage */
        if (abs > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Fetch primary ASTE words 3 and 6 from absolute storage
           (note: the ASTE cannot cross a page boundary) */
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        aste[3] = ARCH_DEP( fetch_fullword_absolute )( abs+12, regs );
#else
        aste[6] = ARCH_DEP( fetch_fullword_absolute )( abs+24, regs );
#endif

        /* Load LTD from primary ASTE word 3 or 6 */
        ltd = ASTE_LT_DESIGNATOR( aste );
    }

    /* Special operation exception if subsystem linkage
       control bit in linkage table designation is zero */
    if ((ltd & LTD_SSLINK) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if in problem state and
       problem bit indicates a change to supervisor state */
    if (PROBSTATE( &regs->psw ) && prob == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Specification exception if new amode is 24-bit and
       new instruction address is not a 24-bit address */
    if (amode == 0 && ia > 0x00FFFFFF)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Space switch if ASN not equal to current PASN */
    if (pasn != regs->CR_LHL(4))
    {
        /* Special operation exception if ASN translation
           control (control register 14 bit 12) is zero */
        if ((regs->CR(14) & CR14_ASN_TRAN) == 0)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

        /* Translate ASN and generate program check if
           AFX- or ASX-translation exception condition */
        xcode = ARCH_DEP(translate_asn) (pasn, regs, &pasteo, aste);
        if (xcode != 0)
            ARCH_DEP( program_interrupt )( regs, xcode );

        /* For PT-ss only, generate a special operation exception
           if ASN-and-LX-reuse is enabled and the reusable-ASN bit
           in the ASTE is one */
        if (pti_instruction == 0 && ASN_AND_LX_REUSE_ENABLED( regs )
            && (aste[1] & ASTE1_RA) != 0)
        {
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        }

        /* For PTI-ss only, generate a special operation exception
           if the controlled-ASN bit in the ASTE is one and the CPU
           was in problem state at the beginning of the operation */
        if (pti_instruction && (aste[1] & ASTE1_CA) != 0
            && PROBSTATE( &regs->psw ))
        {
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        }

        /* For PTI-ss only, generate an ASTE instance exception
           if the ASTEIN in bits 0-31 of the R1 register does
           not equal the ASTEIN in the ASTE*/
        if (pti_instruction && aste[11] != regs->GR_H(r1))
        {
            /* Set bit 2 of the exception access identification
               to indicate that the program check occurred
               during PASN translation in a PTI instruction */
            regs->excarid = 0x20;
            ARCH_DEP( program_interrupt )( regs, PGM_ASTE_INSTANCE_EXCEPTION );
        }

        /* Perform primary address space authorization
           using current authorization index */
        ax = regs->CR_LHH(4);
        if (ARCH_DEP(authorize_asn) (ax, aste, ATE_PRIMARY, regs))
        {
            regs->TEA = pasn;
            ARCH_DEP( program_interrupt )( regs, PGM_PRIMARY_AUTHORITY_EXCEPTION );
        }

        /* Obtain new primary STD or ASCE from the ASTE */
        pstd = ASTE_AS_DESIGNATOR( aste );

#if defined( FEATURE_SUBSPACE_GROUP )
        /* Perform subspace replacement on new PSTD */
        pstd = ARCH_DEP( subspace_replace )( pstd, pasteo, NULL, regs );
#endif

        /* Space switch if either current PSTD or new PSTD
           space-switch-event control bit is set to 1 */
        if ((regs->CR(1) & SSEVENT_BIT) || (pstd & SSEVENT_BIT))
        {
            /* Indicate space-switch event required */
            ssevent = 1;
        }
        else
        {
            ssevent = 2; /* maybe, if PER is pending */
        }

        /* Load new primary STD or ASCE into control register 1 */
        regs->CR(1) = pstd;

        /* Load new AX and PASN into control register 4 */
        regs->CR_L(4) = (aste[1] & ASTE1_AX) | pasn;

        /* Load new PASTEO or LTD into control register 5 */
        regs->CR_L(5) = ASF_ENABLED( regs ) ?
                                pasteo : ASTE_LT_DESIGNATOR( aste );

        /* For PTI-ss, and for PT-ss when ASN-and-LX-reuse is enabled,
           load the new PASTEIN into CR4 from ASTE11_ASTEIN */
        if (pti_instruction || ASN_AND_LX_REUSE_ENABLED( regs ))
        {
            regs->CR_H(4) = aste[11];
        }
    } /* end if (PT-ss or PTI-ss) */
    else
    {
        /* For PT-cp or PTI-cp use current primary STD or ASCE */
        pstd = regs->CR(1);
    }

#if defined( FEATURE_TRACING )
    /* Update trace table address if ASN tracing is on */
    if (regs->CR(12) & CR12_ASNTRACE)
        regs->CR(12) = newcr12;
#endif

    /* Check for Successful Branch PER event */
    PER_SB( regs, ia );

    /* Set the breaking event address register */
    SET_BEAR_REG( regs, regs->ip - 4 );

    /* Replace PSW amode, instruction address, and problem state bit */
    regs->psw.amode = amode;
    SET_PSW_IA_AND_MAYBE_IP( regs, ia );
    if (prob)
        regs->psw.states |= BIT( PSW_PROB_BIT );
    else
        regs->psw.states &= ~BIT( PSW_PROB_BIT );

    regs->psw.AMASK =
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        regs->psw.amode64 ? AMASK64 :
#endif
        regs->psw.amode ? AMASK31 : AMASK24;

    /* AND control register 3 bits 0-15 with the supplied PKM value
       and replace the SASN in CR3 bits 16-31 with new PASN */
    regs->CR_LHH(3) &= pkm;
    regs->CR_LHL(3) = pasn;

    /* For PTI, and also for PT when ASN-and-LX-reuse is enabled,
       set the SASTEIN in CR3 equal to the new PASTEIN in CR4 */
    if (pti_instruction || ASN_AND_LX_REUSE_ENABLED( regs ))
    {
        regs->CR_H(3) = regs->CR_H(4);
    }

    /* Set secondary STD or ASCE equal to new primary STD or ASCE */
    regs->CR(7) = pstd;

    /* Update cpu states */
    SET_IC_MASK( regs );
    SET_AEA_COMMON( regs );
    INVALIDATE_AIA( regs );

    /* Generate space switch event if required */
    if (ssevent == 1 || (ssevent == 2 && IS_IC_PER( regs )))
    {
        /* [6.5.2.34] Set the translation exception address equal
           to the old primary ASN, with the high-order bit set if
           the old primary space-switch-event control bit is one */
        regs->TEA = oldpasn;
        if (oldpstd & SSEVENT_BIT)
            regs->TEA |= TEA_SSEVENT;
        ARCH_DEP( program_interrupt )( regs, PGM_SPACE_SWITCH_EVENT );
    }

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

} /* end ARCH_DEP(program_transfer_proc) */


/*-------------------------------------------------------------------*/
/* B228 PT    - Program Transfer                               [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( program_transfer )
{
int     r1, r2;                         /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    ARCH_DEP( program_transfer_proc )( regs, r1, r2, 0 );
}
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
/*-------------------------------------------------------------------*/
/* B99E PTI - Program Transfer with Instance                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( program_transfer_with_instance )
{
int     r1, r2;                         /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    ARCH_DEP( program_transfer_proc )( regs, r1, r2, 1 );
}
#endif /* defined( FEATURE_006_ASN_LX_REUSE_FACILITY ) */


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* B248 PALB  - Purge ALB                                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( purge_accesslist_lookaside_buffer )
{
int     r1, r2;                         /* Register values (unused)  */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    /* This instruction is executed as a no-operation in XC mode */
    if (SIE_STATE_BIT_ON( regs, MX, XC ))
        return;
#endif

    PRIV_CHECK( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, PXLB ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Purge the ART lookaside buffer for this CPU */
    ARCH_DEP(purge_alb) (regs);
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


/*-------------------------------------------------------------------*/
/* B20D PTLB  - Purge TLB                                        [S] */
/*-------------------------------------------------------------------*/
DEF_INST( purge_translation_lookaside_buffer )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    /* This instruction is executed as a no-operation in XC mode */
    if (SIE_STATE_BIT_ON( regs, MX, XC ))
        return;
#endif

    PRIV_CHECK( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, PXLB ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Purge the translation lookaside buffer for this CPU */
    ARCH_DEP( purge_tlb )( regs );
}


#if defined( FEATURE_BASIC_STORAGE_KEYS )
/*-------------------------------------------------------------------*/
/* B213 RRB   - Reset Reference Bit                              [S] */
/*-------------------------------------------------------------------*/
DEF_INST( reset_reference_bit )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
RADR    pageaddr;                       /* Operand-2 page address    */
BYTE    oldkey;                         /* Original Storage key      */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_4K_STORAGE_KEYS ) || defined( _FEATURE_SIE )
    if (
#if defined( _FEATURE_SIE ) && !defined( FEATURE_4K_STORAGE_KEYS )
        SIE_MODE( regs ) &&
#endif
        !(regs->CR(0) & CR0_STORKEY_4K) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
#endif

    PRIV_CHECK( regs );

    /* Load 2K block real address from operand address */
    pageaddr = effective_addr2 & 0x00FFF800;

    /* Convert real address to absolute address */
    pageaddr = APPLY_PREFIXING( pageaddr, regs->PX );

    /* Addressing exception if block is outside main storage */
    if (pageaddr > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
    {
        if (SIE_STATE_BIT_ON( regs, IC2, RRBE ))
            SIE_INTERCEPT( regs );

        if (!regs->sie_pref)
        {
#if defined( _FEATURE_STORAGE_KEY_ASSIST )
            if (1
                && SIE_STATE_BIT_ON( regs, RCPO0, ASIST )
                && SIE_STATE_BIT_ON( regs, RCPO2, RCPBY )
            )
            {
                /* When "bypass use of RCP table" is requested
                   the guest page is assumed to be accessible.
                */
                /* Translate guest absolute to host absolute */
                SIE_TRANSLATE( &pageaddr, ACCTYPE_SIE, regs );

                /* Save the original storage key */
                oldkey = ARCH_DEP( get_2K_storage_key )( pageaddr );

                /* Reset the reference bit in the real page */
                ARCH_DEP( and_2K_storage_key )( pageaddr, STORKEY_REF );
            }
            else // use RCP...(and possibly PGSTE)
#endif
            {
                PGSTE* pgste;
                RCPTE* rcpte;

                ARCH_DEP( GetPGSTE_and_RCPTE )( regs, pageaddr, &pgste, &rcpte );

                OBTAIN_KEYLOCK( pgste, rcpte, regs );
                {
                    int sr;
                    BYTE realkey;

                    /* Translate guest absolute address to host real */
                    sr = SIE_TRANSLATE_ADDR( regs->sie_mso + pageaddr,
                                             USE_PRIMARY_SPACE,
                                             HOSTREGS, ACCTYPE_SIE );
                    if (sr == 0)
                    {
                        /* Translate host real to host absolute */
                        pageaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

                        /* Save original key before modifying */
                        realkey = ARCH_DEP( get_2K_storage_key )( pageaddr );
                    }
                    else
                        realkey = 0;

                    /* Save the page's real R/C bits by OR'ing them
                       into the host's R/C set in the RCP byte */
                    rcpte->rcpbyte |= ((realkey << 4) & RCPHOST);

                    /* The CC is determined from the logical 'OR'
                       of the real page's R/C bits and the guest's
                       R/C bits from the RCP area byte */
                    oldkey = realkey | (rcpte->rcpbyte & RCPGUEST);

                    /* Update the guest RCP bits */
                    rcpte->rcpbyte &= ~(         RCPGUEST);
                    rcpte->rcpbyte |=  (oldkey & RCPGUEST);

                    /* Reset the reference bit in the guest RCP set */
                    rcpte->rcpbyte &= ~RCPGREF;

                    /* Reset the reference bit in the real page */
                    if (sr == 0)
                        ARCH_DEP( and_2K_storage_key )( pageaddr, STORKEY_REF );
                }
                RELEASE_KEYLOCK( pgste, rcpte, regs );
            }
        }
        else /* sie_perf */
        {
            /* Save the original storage key */
            oldkey = ARCH_DEP( get_2K_storage_key )( pageaddr );

            /* Reset the reference bit in the storage key */
            ARCH_DEP( and_2K_storage_key )( pageaddr, STORKEY_REF );
        }
    }
    else /* !SIE_MODE */
#endif /* defined( _FEATURE_SIE ) */
    {
        /* Save the original storage key */
        oldkey = ARCH_DEP( get_2K_storage_key )( pageaddr );

        /* Reset the reference bit in the storage key */
        ARCH_DEP( and_2K_storage_key )( pageaddr, STORKEY_REF );
    }

    /* Set the condition code according to the state of the
       reference and change bits in the ORIGINAL storage key:

            0  Reference bit zero; change bit zero
            1  Reference bit zero; change bit one
            2  Reference bit one;  change bit zero
            3  Reference bit one;  change bit one
    */
    regs->psw.cc =
         ((oldkey & STORKEY_REF)    ? 2 : 0)
       | ((oldkey & STORKEY_CHANGE) ? 1 : 0);

    /* If the storage key had the REF bit on then perform
     * accelerated lookup invalidations on all CPUs
     * so that the REF bit will be set when referenced next.
    */
    if (oldkey & STORKEY_REF)
        STORKEY_INVALIDATE( regs, pageaddr );
} /* end DEF_INST( reset_reference_bit ) */
#endif /* defined( FEATURE_BASIC_STORAGE_KEYS ) */


#if defined( FEATURE_EXTENDED_STORAGE_KEYS )
/*-------------------------------------------------------------------*/
/* B22A RRBE  - Reset Reference Bit Extended                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( reset_reference_bit_extended )
{
int     r1, r2;                         /* Register values           */
RADR    pageaddr;                       /* Operand-2 page address    */
BYTE    oldkey;                         /* Original Storage key      */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* Load 4K block real address from r2 register */
    pageaddr = regs->GR(r2) & ADDRESS_MAXWRAP_E(regs);

    /* Convert real address to absolute address */
    pageaddr = APPLY_PREFIXING( pageaddr, regs->PX );

    /* Addressing exception if block is outside main storage */
    if (pageaddr > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
    {
        if (SIE_STATE_BIT_ON( regs, IC2, RRBE ))
            SIE_INTERCEPT( regs );

        if (!regs->sie_pref)
        {
#if defined( _FEATURE_STORAGE_KEY_ASSIST )
            if (1
                && (0
                    || SIE_STATE_BIT_ON( regs, RCPO0, ASIST )
#if defined( _FEATURE_ZSIE )
                    // SKA is always active for z/VM
                    || ARCH_900_IDX == HOSTREGS->arch_mode
#endif
                   )
                && SIE_STATE_BIT_ON( regs, RCPO2, RCPBY )
            )
            {
                /* When "bypass use of RCP table" is requested
                   the guest page is assumed to be accessible.
                */
                /* Translate guest absolute to host absolute */
                SIE_TRANSLATE( &pageaddr, ACCTYPE_SIE, regs );

                /* Save the original storage key */
                oldkey = ARCH_DEP( get_4K_storage_key )( pageaddr );

                /* Reset the reference bit in the real page */
                ARCH_DEP( and_4K_storage_key )( pageaddr, STORKEY_REF );
            }
            else // use RCP...(and possibly PGSTE)
#endif
            {
                PGSTE* pgste;
                RCPTE* rcpte;

                ARCH_DEP( GetPGSTE_and_RCPTE )( regs, pageaddr, &pgste, &rcpte );

                OBTAIN_KEYLOCK( pgste, rcpte, regs );
                {
                    int sr;
                    BYTE realkey;

                    /* Translate guest absolute address to host real */
                    sr = SIE_TRANSLATE_ADDR( regs->sie_mso + pageaddr,
                                             USE_PRIMARY_SPACE,
                                             HOSTREGS, ACCTYPE_SIE );
                    if (sr == 0)
                    {
                        /* Translate host real to host absolute */
                        pageaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

                        /* Save original key before modifying */
                        realkey = ARCH_DEP( get_4K_storage_key )( pageaddr );
                    }
                    else
                        realkey = 0;

                    /* Save the page's real R/C bits by OR'ing them
                       into the host's R/C set in the RCP byte */
                    rcpte->rcpbyte |= ((realkey << 4) & RCPHOST);

                    /* The CC is determined from the logical 'OR'
                       of the real page's R/C bits and the guest's
                       R/C bits from the RCP area byte */
                    oldkey = realkey | (rcpte->rcpbyte & RCPGUEST);

                    /* Update the guest RCP bits */
                    rcpte->rcpbyte &= ~(         RCPGUEST);
                    rcpte->rcpbyte |=  (oldkey & RCPGUEST);

                    /* Reset the reference bit in the guest RCP set */
                    rcpte->rcpbyte &= ~RCPGREF;

                    /* Reset the reference bit in the real page */
                    if (sr == 0)
                        ARCH_DEP( and_4K_storage_key )( pageaddr, STORKEY_REF );
                }
                RELEASE_KEYLOCK( pgste, rcpte, regs );
            }
        }
        else /* sie_pref */
        {
            /* Save the original storage key */
            oldkey = ARCH_DEP( get_4K_storage_key )( pageaddr );

            /* Reset the reference bit in the storage key */
            ARCH_DEP( and_4K_storage_key )( pageaddr, STORKEY_REF );
        }
    }
    else /* !SIE_MODE */
#endif /* defined( _FEATURE_SIE ) */
    {
        /* Save the original storage key */
        oldkey = ARCH_DEP( get_4K_storage_key )( pageaddr );

        /* Reset the reference bit in the storage key */
        ARCH_DEP( and_4K_storage_key )( pageaddr, STORKEY_REF );
    }

    /* Set the condition code according to the state of the
       reference and change bits in the ORIGINAL storage key:

            0  Reference bit zero; change bit zero
            1  Reference bit zero; change bit one
            2  Reference bit one;  change bit zero
            3  Reference bit one;  change bit one
    */
    regs->psw.cc =
         ((oldkey & STORKEY_REF)    ? 2 : 0)
       | ((oldkey & STORKEY_CHANGE) ? 1 : 0);

    /* If the storage key had the REF bit on then perform
     * accelerated lookup invalidations on all CPUs
     * so that the REF bit will be set when referenced next.
     */
    if (oldkey & STORKEY_REF)
        STORKEY_INVALIDATE( regs, pageaddr );

} /* end DEF_INST( reset_reference_bit_extended ) */
#endif /* defined( FEATURE_EXTENDED_STORAGE_KEYS ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* B219 SAC   - Set Address Space Control                        [S] */
/* B279 SACF  - Set Address Space Control Fast                   [S] */
/*-------------------------------------------------------------------*/
DEF_INST( set_address_space_control )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE    mode;                           /* New addressing mode       */
BYTE    oldmode;                        /* Current addressing mode   */
int     ssevent = 0;                    /* 1=space switch event      */

    S( inst, regs, b2, effective_addr2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST )
    if (inst[1] == 0x19) // SAC only
#endif
    {
        /* Perform serialization and checkpoint-synchronization */
        PERFORM_SERIALIZATION( regs );
        PERFORM_CHKPT_SYNC( regs );
    }

    /* Isolate bits 20-23 of effective address */
    mode = (effective_addr2 & 0x00000F00) >> 8;

    /* Special operation exception if DAT is off or
       secondary-space control bit is zero */
    if ((REAL_MODE( &regs->psw )
         || (regs->CR(0) & CR0_SEC_SPACE) == 0)
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
         && !SIE_STATE_BIT_ON( regs, MX, XC )
#endif
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Privileged operation exception if setting home-space
       mode while in problem state */
    if (mode == 3 && PROBSTATE( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Special operation exception if setting AR mode
       and address-space function control bit is zero */
    if (mode == 2 && !ASF_ENABLED( regs ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Specification exception if mode is invalid */
    if (mode > 3
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    /* Secondary and Home space mode are not supported in XC mode */
      || ( SIE_STATE_BIT_ON( regs, MX, XC )
        && (mode == 1 || mode == 3) )
#endif
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Save the current address-space control bits */
    oldmode = (AR_BIT( &regs->psw ) << 1) | SPACE_BIT( &regs->psw );

    /* Reset the address-space control bits in the PSW */
    if (mode & 1)
        regs->psw.asc |=  BIT( PSW_SPACE_BIT );
    else
        regs->psw.asc &= ~BIT( PSW_SPACE_BIT );
    if (mode & 2)
        regs->psw.asc |=  BIT( PSW_AR_BIT );
    else
        regs->psw.asc &= ~BIT( PSW_AR_BIT );

    TEST_SET_AEA_MODE( regs );

    /* If switching into or out of home-space mode, and also:
       primary space-switch-event control bit is set; or
       home space-switch-event control bit is set; or
       PER event is to be indicated
       then indicate a space-switch-event */
    if (((oldmode != 3 && mode == 3) || (oldmode == 3 && mode != 3))
         && (regs->CR(1) & SSEVENT_BIT
              || regs->CR(13) & SSEVENT_BIT
              || OPEN_IC_PER( regs )))
      {
        /* Indicate space-switch event required */
        ssevent = 1;

        /* [6.5.2.34] Set the translation exception address */
        if (mode == 3)
        {
            /* When switching into home-space mode, set the
               translation exception address equal to the primary
               ASN, with the high-order bit set equal to the value
               of the primary space-switch-event control bit */
            regs->TEA = regs->CR_LHL(4);
            if (regs->CR(1) & SSEVENT_BIT)
                regs->TEA |= TEA_SSEVENT;
        }
        else
        {
            /* When switching out of home-space mode, set the
               translation exception address equal to zero, with
               the high-order bit set equal to the value of the
               home space-switch-event control bit */
            regs->TEA = 0;
            if (regs->CR(13) & SSEVENT_BIT)
                regs->TEA |= TEA_SSEVENT;
        }
    }

    /* Generate a space-switch-event if indicated */
    if (ssevent)
        ARCH_DEP( program_interrupt )( regs, PGM_SPACE_SWITCH_EVENT );

#if defined( FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST )
    if (inst[1] == 0x19) // SAC only
#endif
    {
        /* Perform serialization and checkpoint-synchronization */
        PERFORM_SERIALIZATION( regs );
        PERFORM_CHKPT_SYNC( regs );
    }
}
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


/*-------------------------------------------------------------------*/
/* B204 SCK   - Set Clock                                        [S] */
/*-------------------------------------------------------------------*/
DEF_INST( set_clock )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Clock value               */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    SIE_INTERCEPT( regs );

    PRIV_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

    /* Fetch new TOD clock value from operand address */
    dreg = ARCH_DEP( vfetch8 )( effective_addr2, b2, regs);

    /* Set the clock epoch register */
    set_tod_clock( dreg >> 8 );

    /* reset the clock comparator pending flag according to
       the setting of the tod clock */
    OBTAIN_INTLOCK( regs );
    {
        if (get_tod_clock( regs ) > regs->clkc)
            ON_IC_CLKC( regs );
        else
            OFF_IC_CLKC( regs );
    }
    RELEASE_INTLOCK( regs );

    /* Return condition code zero */
    regs->psw.cc = 0;

    RETURN_INTCHECK( regs );

//  /*debug*/LOGMSG( "Set TOD clock=%16.16"PRIX64"\n", dreg );
}


/*-------------------------------------------------------------------*/
/* B206 SCKC  - Set Clock Comparator                             [S] */
/*-------------------------------------------------------------------*/
DEF_INST( set_clock_comparator )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Clock value               */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC3, SCKC ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Fetch clock comparator value from operand location */
    dreg = ARCH_DEP( vfetch8 )( effective_addr2, b2, regs );

//  /*debug*/LOGMSG( "Set clock comparator=%16.16"PRIX64"\n", dreg );

    dreg >>= 8;

    OBTAIN_INTLOCK( regs );
    {
        regs->clkc = dreg;

        /* reset the clock comparator pending flag according to
           the setting of the tod clock */
        if (get_tod_clock( regs ) > dreg)
            ON_IC_CLKC( regs );
        else
            OFF_IC_CLKC( regs );
    }
    RELEASE_INTLOCK( regs );

    RETURN_INTCHECK( regs );
}


#if defined( FEATURE_EXTENDED_TOD_CLOCK )
/*-------------------------------------------------------------------*/
/* 0107 SCKPF - Set Clock Programmable Field                     [E] */
/*-------------------------------------------------------------------*/
DEF_INST( set_clock_programmable_field )
{
    E( inst, regs );

    UNREFERENCED( inst );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* Program check if register 0 bits 0-15 are not zeroes */
    if (regs->GR_LHH(0))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Set TOD programmable register from register 0 */
    regs->todpr = regs->GR_LHL(0);
}
#endif /* defined( FEATURE_EXTENDED_TOD_CLOCK ) */


/*-------------------------------------------------------------------*/
/* B208 SPT   - Set CPU Timer                                    [S] */
/*-------------------------------------------------------------------*/
DEF_INST( set_cpu_timer )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S64     dreg;                           /* Timer value               */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC3, SPT ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Fetch the CPU timer value from operand location */
    dreg = ARCH_DEP( vfetch8 )( effective_addr2, b2, regs );

    OBTAIN_INTLOCK( regs );
    {
        set_cpu_timer( regs, dreg );

        /* reset the cpu timer pending flag according to its value */
        if (unlikely( dreg < 0 ))
            ON_IC_PTIMER( regs );
        else
            OFF_IC_PTIMER( regs );
    }
    RELEASE_INTLOCK( regs );

//  /*debug*/LOGMSG( "Set CPU timer=%16.16"PRIX64"\n", dreg );

    RETURN_INTCHECK( regs );
}


/*-------------------------------------------------------------------*/
/* B210 SPX   - Set Prefix                                       [S] */
/*-------------------------------------------------------------------*/
DEF_INST( set_prefix )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
RADR    n;                              /* Prefix value              */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    SIE_INTERCEPT( regs );
    FW_CHECK( effective_addr2, regs );

    PERFORM_SERIALIZATION( regs );
    {
        /* Load new prefix value from operand address */
        n = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );

        /* Isolate bits 1-19 for ESA/390 or 1-18 for ESAME of new prefix value */
        n &= PX_MASK;

        /* Program check if prefix is invalid absolute address */
        if (n > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Load new value into prefix register */
        regs->PX = n;

        /* Set pointer to active PSA structure */
        regs->psa = (PSA_3XX*)(regs->mainstor + regs->PX);

        /* Invalidate the ALB and TLB */
        ARCH_DEP( purge_tlb )( regs );
#if defined( FEATURE_ACCESS_REGISTERS )
        ARCH_DEP( purge_alb )( regs );
#endif
    }
    PERFORM_SERIALIZATION( regs );
}


/*-------------------------------------------------------------------*/
/* B20A SPKA  - Set PSW Key from Address                         [S] */
/*-------------------------------------------------------------------*/
DEF_INST( set_psw_key_from_address )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     n;                              /* Storage key workarea      */
BYTE    pkey;                           /* Original key              */

    S( inst, regs, b2, effective_addr2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    pkey = regs->psw.pkey;

    /* Isolate the key from bits 24-27 of effective address */
    n = effective_addr2 & 0x000000F0;

    /* Privileged operation exception if in problem state
       and the corresponding PSW key mask bit is zero */
    if (PROBSTATE( &regs->psw )
        && ((regs->CR(3) << (n >> 4)) & 0x80000000) == 0)
        ARCH_DEP( program_interrupt )( regs, PGM_PRIVILEGED_OPERATION_EXCEPTION );

    /* Set PSW key */
    regs->psw.pkey = n;
    INVALIDATE_AIA( regs );
}


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* Common processing routine for the SSAR and SSAIR instructions     */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_secondary_asn_proc )( REGS* regs,
        int r1, int r2, int ssair_instruction )
{
U16     sasn;                           /* New Secondary ASN         */
RADR    sstd;                           /* Secondary STD             */
U32     sasteo=0;                       /* Secondary ASTE origin     */
U32     aste[16];                       /* ASN second table entry    */
U32     sastein;                        /* New Secondary ASTEIN      */
U16     xcode;                          /* Exception code            */
U16     ax;                             /* Authorization index       */
#if defined( FEATURE_TRACING )
CREG    newcr12 = 0;                    /* CR12 upon completion      */
#endif

    UNREFERENCED( r2 );
    SIE_XC_INTERCEPT( regs );

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Special operation exception if ASN translation control
       (bit 12 of control register 14) is zero or DAT is off */
    if ((regs->CR(14) & CR14_ASN_TRAN) == 0
        || REAL_MODE( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Load the new ASN from R1 register bits 16-31 */
    sasn = regs->GR_LHL(r1);

#if defined( FEATURE_TRACING )
    /* Form trace entry if ASN tracing is on */
    if (regs->CR(12) & CR12_ASNTRACE)
        newcr12 = ARCH_DEP(trace_ssar) (ssair_instruction, sasn, regs);
#endif

    /* Test for SSAR/SSAIR to current primary */
    if (sasn == regs->CR_LHL(4))
    {
        /* Set new secondary STD equal to primary STD */
        sstd = regs->CR(1);

        /* Set new secondary ASTEIN equal to primary ASTEIN */
        sastein = regs->CR_H(4);

    } /* end if (SSAR-cp or SSAIR-cp) */
    else
    { /* SSAR/SSAIR with space-switch */

        /* Perform ASN translation to obtain ASTE */
        xcode = ARCH_DEP( translate_asn )( sasn, regs, &sasteo, aste );

        /* Program check if ASN translation exception */
        if (xcode != 0)
            ARCH_DEP( program_interrupt )( regs, xcode );

        /* For SSAR-ss only, generate a special operation exception
           if ASN-and-LX-reuse is enabled and the reusable-ASN bit
           in the ASTE is one */
        if (ssair_instruction == 0 && ASN_AND_LX_REUSE_ENABLED( regs )
            && (aste[1] & ASTE1_RA) != 0)
        {
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        }

        /* For SSAIR-ss only, generate a special operation exception
           if the controlled-ASN bit in the ASTE is one and the CPU
           is in problem state */
        if (ssair_instruction && (aste[1] & ASTE1_CA) != 0
            && PROBSTATE( &regs->psw ))
        {
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
        }

        /* For SSAIR-ss only, generate an ASTE instance exception
           if the ASTEIN in bits 0-31 of the R1 register does
           not equal the ASTEIN in the ASTE */
        if (ssair_instruction && aste[11] != regs->GR_H(r1))
        {
            /* Set bit 3 of the exception access identification
               to indicate that the program check occurred
               during SASN translation in a SSAIR instruction */
            regs->excarid = 0x10;
            ARCH_DEP( program_interrupt )( regs, PGM_ASTE_INSTANCE_EXCEPTION );
        }

        /* Perform ASN authorization using current AX */
        ax = regs->CR_LHH(4);
        if (ARCH_DEP( authorize_asn )( ax, aste, ATE_SECONDARY, regs ))
        {
            regs->TEA = sasn;
            ARCH_DEP( program_interrupt )( regs, PGM_SECONDARY_AUTHORITY_EXCEPTION );
        }

        /* Load new secondary STD or ASCE from the ASTE */
        sstd = ASTE_AS_DESIGNATOR( aste );

        /* Load new secondary ASTEIN from the ASTE */
        sastein = aste[11];

#if defined( FEATURE_SUBSPACE_GROUP )
        /* Perform subspace replacement on new SSTD */
        sstd = ARCH_DEP( subspace_replace )( sstd, sasteo, NULL, regs );
#endif

    } /* end if (SSAR-ss or SSAIR-ss) */

#if defined( FEATURE_TRACING )
    /* Update trace table address if ASN tracing is on */
    if (regs->CR(12) & CR12_ASNTRACE)
        regs->CR(12) = newcr12;
#endif

    /* Load the new secondary ASN into control register 3 */
    regs->CR_LHL(3) = sasn;

    /* Load the new secondary STD into control register 7 */
    regs->CR(7) = sstd;

    /* For SSAIR, and for SSAR when ASN-and-LX-reuse is enabled,
       load the new secondary ASTEIN into control register 3 */
    if (ssair_instruction || ASN_AND_LX_REUSE_ENABLED( regs ))
    {
        regs->CR_H(3) = sastein;
    }

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

} /* end ARCH_DEP(set_secondary_asn_proc) */


/*-------------------------------------------------------------------*/
/* B225 SSAR  - Set Secondary ASN                              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( set_secondary_asn )
{
int     r1, r2;                         /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    ARCH_DEP( set_secondary_asn_proc )( regs, r1, r2, 0 );
}
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
/*-------------------------------------------------------------------*/
/* B99F SSAIR - Set Secondary ASN with Instance                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( set_secondary_asn_with_instance )
{
int     r1, r2;                         /* Values of R fields        */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    ARCH_DEP( set_secondary_asn_proc )( regs, r1, r2, 1 );
}
#endif /* defined( FEATURE_006_ASN_LX_REUSE_FACILITY ) */


#if defined( FEATURE_BASIC_STORAGE_KEYS )
/*-------------------------------------------------------------------*/
/* 08   SSK   - Set Storage Key                                 [RR] */
/*-------------------------------------------------------------------*/
DEF_INST( set_storage_key )
{
int     r1, r2;                         /* Operand register numbers  */
RADR    pageaddr;                       /* Working abs page address  */
BYTE    r1key;                          /* Key value to set from r1  */

    RR( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* Special Operation Exception if Storkey exception control zero */
#if defined( FEATURE_4K_STORAGE_KEYS ) || defined( _FEATURE_SIE )
    if (
#if defined( _FEATURE_SIE ) && !defined( FEATURE_4K_STORAGE_KEYS )
        SIE_MODE( regs ) &&
#endif
        !(regs->CR(0) & CR0_STORKEY_4K) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
#endif

    /* Key to be applied */
    r1key = regs->GR_LHLCL(r1);

    /* Program check if r2 bits 28-31 are not zeroes */
    if (regs->GR_L(r2) & 0x0000000F)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Load 2K block real address from r2 register */
    pageaddr = regs->GR_L(r2) & 0x00FFF800;

    /* Convert real address to absolute address */
    pageaddr = APPLY_PREFIXING( pageaddr, regs->PX );

    /* Addressing exception if block is outside of main storage */
    if (pageaddr > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
    {
        if (SIE_STATE_BIT_ON( regs, IC2, SSKE ))
            SIE_INTERCEPT( regs );

        if (!regs->sie_pref)
        {
#if defined( _FEATURE_STORAGE_KEY_ASSIST )
            if (1
                && SIE_STATE_BIT_ON( regs, RCPO0, ASIST )
                && SIE_STATE_BIT_ON( regs, RCPO2, RCPBY )
            )
            {
                /* When "bypass use of RCP table" is requested
                   the guest page is assumed to be accessible.
                */
                /* Translate guest absolute to host absolute */
                SIE_TRANSLATE( &pageaddr, ACCTYPE_SIE, regs );

                /* Set the storage key as requested */
                ARCH_DEP( put_4K_storage_key )( pageaddr, r1key );
            }
            else // use RCP...(and possibly PGSTE)
#endif
            {
                PGSTE* pgste;
                RCPTE* rcpte;
                int    sr;
                BYTE   oldkey;

                ARCH_DEP( GetPGSTE_and_RCPTE )( regs, pageaddr, &pgste, &rcpte );

                OBTAIN_KEYLOCK( pgste, rcpte, regs );
                {
                    /* Translate guest absolute address to host real.
                       Note that the RCP table MUST be locked BEFORE
                       we try to access the real page!
                    */
                    sr = SIE_TRANSLATE_ADDR( regs->sie_mso + pageaddr,
                                             USE_PRIMARY_SPACE,
                                             HOSTREGS, ACCTYPE_SIE );
                    if (sr == 0)
                    {
                        /* Translate host real to host absolute */
                        pageaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

                        /* Save the original key */
                        oldkey = ARCH_DEP( get_4K_storage_key )( pageaddr );

                        /* Before potentially changing the real page's
                           key, save it's real R/C bits by OR'ing them
                           into the host set's R/C bits in the RCP byte
                        */
                        rcpte->rcpbyte |= ((oldkey << 4) & RCPHOST);
                    }
                    else // (sr != 0)
                    {
                        /* If the real page is inaccessible and SKA
                           is not active, then we cannot proceed since,
                           with the old non-SKA RCP table approach,
                           there isn't any other way to set or obtain
                           the page's access key and fetch-protect bits
                           since the old non-SKA RCP table contains
                           ONLY the R/C bits, but not anything else.
                        */
                        if (!pgste)
                        {
                            RELEASE_KEYLOCK( pgste, rcpte, regs );
                            SIE_INTERCEPT( regs );
                        }

                        /* Reconstruct the original storage key from
                           both the PGSTE and RCPTE entries.
                        */
                        oldkey = (pgste->pgsvkey & PGSVKACF)
                               | (rcpte->rcpbyte & RCPGUEST);
                    }

                    /* Update the R/C bits in the RCP */
                    rcpte->rcpbyte &= ~(        RCPGUEST);
                    rcpte->rcpbyte |=  (r1key & RCPGUEST);

                    /* Update the other bits... */
                    if (pgste)
                    {
                        /* Save updated key in PGSTE */
                        pgste->pgsvkey = (r1key & PGSVKACF);

                        /* SIE *requires* the R/C bits be cleared to
                           zero in the real page if still accessible.
                           Otherwise if not still accessible, that's
                           okay since we've honored the guest's "SSK"
                           request by setting the updated  key in the
                           above PGSTE pgsvkey field as well as the
                           R/C bits too in the RCPTE byte.
                        */
                        if (sr == 0)
                        {
                            /* Clear real page's R/C bits to zero */
                            r1key &= ~(STORKEY_REF | STORKEY_CHANGE);
                        }
                    }

                    /* Update real page's key BEFORE releasing lock */
                    if (sr == 0)
                        ARCH_DEP( put_4K_storage_key )( pageaddr, r1key );
                }
                RELEASE_KEYLOCK( pgste, rcpte, regs );
            }
        }
        else /* sie_pref */
        {
            /* Update the storage key from r1 register bits 24-30 */
            ARCH_DEP( put_4K_storage_key )( pageaddr, r1key );
        }
    }
    else /* !SIE_MODE */
#endif /* defined( _FEATURE_SIE ) */
    {
        /* Update the storage key from r1 register bits 24-30 */
        ARCH_DEP( put_2K_storage_key )( pageaddr, r1key );
    }

    /* Invalidate AIA/AEA so that the REF and CHANGE bits
       will be set when referenced next */
    STORKEY_INVALIDATE( regs, pageaddr );

//  /*debug*/LOGMSG( "SSK storage block %8.8X key %2.2X\n",
//  /*debug*/        regs->GR_L(r2), regs->GR_LHLCL(r1) & 0xFE );

} /* end DEF_INST( set_storage_key ) */
#endif /* defined( FEATURE_BASIC_STORAGE_KEYS ) */


#if defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY )
/*-------------------------------------------------------------------*/
/*       SUBROUTINE TO PERFORM CONDITIONAL SSKE PROCESSING           */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*                                                                   */
/*      sske    true = SSKE, false = PFMF                            */
/*      regs    Register context                                     */
/*      r1      Operand-1 register number                            */
/*      m3      Operand-3 mask field from SSKE instruction.          */
/*      oldkey  Contents of storage key before modification          */
/*      r1key   Register r1 storage key comparison value             */
/*                                                                   */
/* Output (when conditional SSKE facility is *NOT* installed):       */
/*                                                                   */
/*      r1 register and condition code remain unchanged,             */
/*      and the return value is false (update key normally).         */
/*                                                                   */
/* Output (when conditional SSKE facility *IS* installed):           */
/*                                                                   */
/*   SSKE:                                                           */
/*                                                                   */
/*      r1 register bits 48-55 set to original storage key value;    */
/*                                                                   */
/*      - if storage key *SHOULD* be updated, the condition          */
/*        code is set to 1 and the function return code value        */
/*        is false: do normal key update processing.                 */
/*                                                                   */
/*      - if storage key update should be BYPASSED, the condition    */
/*        code is set to 0 and the function return code value        */
/*        is true: no change should be made to the storage key.      */
/*                                                                   */
/*   PFMF:                                                           */
/*                                                                   */
/*      r1 register remains unchanged;                               */
/*                                                                   */
/*      - if storage key *SHOULD* be updated, the condition          */
/*        code remains unchanged and the function return value       */
/*        is false: do normal key update processing.                 */
/*                                                                   */
/*      - if storage key update should be BYPASSED, the condition    */
/*        code remains unchanged and the function return value       */
/*        is true: no change should be made to the storage key.      */
/*                                                                   */
/*-------------------------------------------------------------------*/
bool ARCH_DEP( conditional_sske_procedure )( bool sske, REGS* regs,
                                             int r1, int m3,
                                             BYTE oldkey, BYTE r1key )
{
    /* Perform normal SSKE processing if Conditional-SSKE Facility
       is not installed or the both MR and MC bits are zero. Else
       perform Conditional-SSKE processing if facility is installed
       and either of the MR or MC bits are non-zero.
    */
    if (0
        || !FACILITY_ENABLED( 010_CONDITIONAL_SSKE, regs )
        || (m3 & (SSKE_MASK_MR | SSKE_MASK_MC)) == 0
    )
        return false;           /* Update key normally */

    /* Insert original storage key into R1 register bits 48-55 */
    if (sske)
        regs->GR_LHLCH(r1) = oldkey;

    /* Determine whether key update should be bypassed or not */
    if (bypass_skey_update( regs, m3, oldkey, r1key ))
    {
        if (sske)
            regs->psw.cc = 0;   /* cc0: storage key NOT set */
        return true;            /**** BYPASS key update! ****/
    }
    else
    {
        if (sske)
            regs->psw.cc = 1;   /* cc1: entire storage key set */
        return false;           /* Update storage key normally */
    }
} /* end function conditional_sske_procedure */
#endif /* defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY ) */


/*-------------------------------------------------------------------*/
/*                     sske_or_pfmf_procedure                        */
/*-------------------------------------------------------------------*/
/*  Common helper function used by both SSKE and PFMF instructions.  */
/*-------------------------------------------------------------------*/
void ARCH_DEP( sske_or_pfmf_procedure )
(
    bool   sske,            /* true = SSKE call, false = PFMF        */
    bool   intlocked,       /* true = INTLOCK held; false otherwise  */
    REGS*  regs,            /* Registers context                     */
    U64    abspage,         /* Absolute address of 4K page frame     */
    int    r1,              /* Operand-1 register number             */
    int    m3,              /* Mask field from SSKE instruction,
                               or PFMF register r1 bits 52-55.       */
    BYTE   r1key            /* Frame's POTENTIALLY new key value     */
)
{
    BYTE  oldkey;               /* Original key before setting       */
    bool  set_key;              /* SSKE or PFMF set key option       */
    bool  clear_frame;          /* PFMF clear frame option           */
    bool  replace_key = false;  /* Work flag to make things simpler  */
    bool  key_updated = false;  /* Work flag to make things simpler  */

#if !defined( FEATURE_010_CONDITIONAL_SSKE_FACILITY )
    UNREFERENCED( m3 );
#endif

    /* Addressing exception if block is outside of main storage */
    if (abspage > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

    set_key     = ( sske || (regs->GR(r1) & PFMF_FMFI_SK));
    clear_frame = (!sske && (regs->GR(r1) & PFMF_FMFI_CF));

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
    {
        if (SIE_STATE_BIT_ON( regs, IC2, SSKE ))
            SIE_INTERCEPT( regs );

        if (!regs->sie_pref)
        {
#if defined( _FEATURE_STORAGE_KEY_ASSIST )
            if (1
                && (0
                    || SIE_STATE_BIT_ON( regs, RCPO0, ASIST )
#if defined( _FEATURE_ZSIE )
                    // SKA is always active for z/VM
                    || ARCH_900_IDX == HOSTREGS->arch_mode
#endif
                   )
                && SIE_STATE_BIT_ON( regs, RCPO2, RCPBY )
            )
            {
                /* When "bypass use of RCP table" is requested
                   the guest page is assumed to be accessible.
                */
                /* Translate guest absolute to host absolute */
                SIE_TRANSLATE( &abspage, ACCTYPE_SIE, regs );
                replace_key = set_key;
            }
            else // use RCP...(and possibly PGSTE)
#endif /* defined( _FEATURE_STORAGE_KEY_ASSIST ) */
            {
                PGSTE* pgste;
                RCPTE* rcpte;
                int    sr;

                ARCH_DEP( GetPGSTE_and_RCPTE )( regs, abspage, &pgste, &rcpte );

                OBTAIN_KEYLOCK( pgste, rcpte, regs );
                {
                    /* Translate guest absolute address to host real.
                       Note that the RCP table MUST be locked BEFORE
                       we try to access the real page!
                    */
                    sr = SIE_TRANSLATE_ADDR( regs->sie_mso + abspage,
                                             USE_PRIMARY_SPACE,
                                             HOSTREGS, ACCTYPE_SIE );
                    if (sr == 0)
                    {
                        /* Translate host real to host absolute */
                        abspage = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

                        if (set_key)
                        {
                            /* Save the original key */
                            oldkey = ARCH_DEP( get_4K_storage_key )( abspage );
                        }
                    }
                    else // (sr != 0)
                    {
                        /* If the real page is inaccessible and SKA
                           is not active, then we cannot proceed since,
                           with the old non-SKA RCP table approach,
                           there isn't any other way to set or obtain
                           the page's access key and fetch-protect bits
                           since the old non-SKA RCP table contains
                           ONLY the R/C bits, but not anything else.

                           We also can't proceed if this is a PFMF call
                           and clear frame was specified since we need
                           access to the real page frame for that too.
                        */
                        if (!pgste || clear_frame)
                        {
                            RELEASE_KEYLOCK( pgste, rcpte, regs );
                            SIE_INTERCEPT( regs );
                        }

                        /* Reconstruct the original storage key from
                           both the PGSTE and RCPTE entries.
                        */
                        if (set_key)
                        {
                            oldkey = (pgste->pgsvkey & PGSVKACF)
                                   | (rcpte->rcpbyte & RCPGUEST);
                        }
                    }

                    if (set_key)
                    {
#if defined( FEATURE_010_CONDITIONAL_SSKE_FACILITY )
                        if (!ARCH_DEP( conditional_sske_procedure )
                            ( sske, regs, r1, m3, oldkey, r1key ))
#endif
                        {
                            /* Update the R/C bits in the RCP */
                            rcpte->rcpbyte &= ~(        RCPGUEST);
                            rcpte->rcpbyte |=  (r1key & RCPGUEST);

                            /* Update the other bits... */
                            if (pgste)
                            {
                                /* Save updated key in PGSTE */
                                pgste->pgsvkey = (r1key & PGSVKACF);

                                /* SIE *requires* the R/C bits be cleared to
                                   zero in the real page if still accessible.
                                   Otherwise if not still accessible, that's
                                   okay since we've honored the guest's "SSK"
                                   request by setting the updated  key in the
                                   above PGSTE pgsvkey field as well as the
                                   R/C bits too in the RCPTE byte.
                                */
                                if (sr == 0)
                                {
                                    /* Clear real page's R/C bits to zero */
                                    r1key &= ~(STORKEY_REF | STORKEY_CHANGE);
                                }
                            }

                            /* Update real page's key BEFORE releasing lock */
                            if (sr == 0)
                            {
                                /* Before updating the real page's key, update
                                   the host's R/C bits in the RCP table entry.
                                */
                                rcpte->rcpbyte |= ((oldkey << 4) & RCPHOST);

                                ARCH_DEP( put_4K_storage_key )( abspage, r1key );
                                key_updated = true;
                                replace_key = false; // (we just did it!)
                            }
                        }
                    }

                    /* Clear the page frame to zeros if requested.
                       Note that the RCP table MUST be locked while
                       accessing the real page!
                    */
                    if (clear_frame)
                    {
                        clear_page_4K( regs->mainstor + abspage );
                        clear_frame = false; // (don't do twice!)
                    }
                }
                RELEASE_KEYLOCK( pgste, rcpte, regs );
            }
        }
        else /* sie_pref */
            replace_key = set_key;
    }
    else /* !SIE_MODE */
#endif /* defined( _FEATURE_SIE ) */
        replace_key = set_key;

    if (replace_key)
    {
#if defined( FEATURE_010_CONDITIONAL_SSKE_FACILITY )
        BYTE oldkey = ARCH_DEP( get_4K_storage_key )( abspage );
        if (!ARCH_DEP( conditional_sske_procedure )
            ( sske, regs, r1, m3, oldkey, r1key ))
#endif
        {
            ARCH_DEP( put_4K_storage_key )( abspage, r1key );
            key_updated = true;
        }
    }

    /* Clear the page frame to zeros if requested */
    if (clear_frame)
        clear_page_4K( regs->mainstor + abspage );

    if (set_key)
    {
        /* Invalidate AIA/AEA so that the REF and CHANGE bits
           will be set when referenced next */
        if (intlocked)
            STORKEY_INVALIDATE_LOCKED( regs, abspage );
        else
            STORKEY_INVALIDATE( regs, abspage );
    }

#if defined( FEATURE_PER_STORAGE_KEY_ALTERATION_FACILITY )
    /* Indicate PER Storage-key Alteration event if key was updated */
    if (1
        && key_updated
        && EN_IC_PER_SKEY( regs )
        && PER_RANGE_CHECK( abspage, regs->CR(10), regs->CR(11) )
    )
        ON_IC_PER_SKEY( regs );
#endif
} /* end ARCH_DEP( sske_or_pfmf_procedure ) */


#if defined( FEATURE_EXTENDED_STORAGE_KEYS )
/*-------------------------------------------------------------------*/
/* B22B SSKE  - Set Storage Key extended                     [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST( set_storage_key_extended )
{
int     r1, r2;                         /* Operand register numbers  */
int     m3;                             /* Operand-3 mask field      */
RADR    pageaddr;                       /* Working abs page address  */
BYTE    r1key;                          /* Key value to set from r1  */

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
int     fc;                             /* Frame count (multi-block) */
bool    multi_block = false;            /* Work (simplifies things)  */
BYTE    original_cc = regs->psw.cc;     /* (in case of multi-block)  */
#endif
bool    quiesce = false;                /* Set Key should quiesce    */

    RRF_M( inst, regs, r1, r2, m3 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* Save key to MAYBE be set */
    r1key = regs->GR_LHLCL(r1);

    /* Load real or absolute page address from r2 register */
    pageaddr = regs->GR(r2);

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
    if (1
        && FACILITY_ENABLED( 008_EDAT_1, regs )
        && (m3 & SSKE_MASK_MB)
    )
    {
        U64  low_end_of_range = (pageaddr & STORAGE_KEY_4K_PAGEMASK);
        U64  hi_end_of_range  = ROUND_UP( low_end_of_range+1,
                                          ONE_MEGABYTE );
        fc = (hi_end_of_range - low_end_of_range) / _4K;
        multi_block = true;
    }
    else // (not multi-block)
    {
        fc = 1;
        /* Convert real address to absolute address */
        pageaddr = APPLY_PREFIXING( pageaddr, regs->PX );
    }
#endif

    /* Wrap address according to addressing mode */
    pageaddr &= ADDRESS_MAXWRAP_E( regs );

    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    /* TXF Key Quiescing support */
    quiesce = FACILITY_ENABLED( 073_TRANSACT_EXEC, regs ) &&
    (0
        || !FACILITY_ENABLED( 014_NONQ_KEY_SET, regs )
        || !(m3 & SSKE_MASK_NQ)
    );
    if (quiesce)
    {
        OBTAIN_INTLOCK( regs );
        SYNCHRONIZE_CPUS( regs );
    }
#endif

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )

    /* Process all pages within requested frame... */
    while (fc--)
    {
#endif
        /* Use helper function to actually set the key or not */
        ARCH_DEP( sske_or_pfmf_procedure )
        (
            true,       /* true = SSKE call, false = PFMF     */
            quiesce,    /* true = INTLOCK held; else false    */
            regs,       /* Registers context                  */
            pageaddr,   /* Absolute address of 4K page frame  */
            r1,         /* Operand-1 register number          */
            m3,         /* Mask field from SSKE instruction,
                           or PFMF register r1 bits 52-55.    */
            r1key       /* Frame's POTENTIALLY new key value  */
        );

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )

        /* Advance to next page if multi-block mode */
        if (multi_block)
        {
            if (regs->psw.amode64)
            {
                regs->GR_G(r2) += STORAGE_KEY_4K_PAGESIZE;
                regs->GR_G(r2) &= ADDRESS_MAXWRAP_E( regs );
            }
            else
            {
                regs->GR_L(r2) += STORAGE_KEY_4K_PAGESIZE;
                regs->GR_L(r2) &= ADDRESS_MAXWRAP_E( regs );
            }
            pageaddr += STORAGE_KEY_4K_PAGESIZE;
            pageaddr &= ADDRESS_MAXWRAP_E( regs );
        }

    } // end while...

    if (multi_block)
    {
        if (m3 & (SSKE_MASK_MR | SSKE_MASK_MC))
            regs->psw.cc = 3;
        else
            regs->psw.cc = original_cc;
    }

#endif /* defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 ) */

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    /* TXF Key Quiescing support */
    if (quiesce)
    {
        txf_abort_all( regs->cpuad, TXF_WHY_STORKEY, PTT_LOC );
        RELEASE_INTLOCK( regs );
    }
#endif

    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

} /* end DEF_INST( set_storage_key_extended ) */
#endif /* defined( FEATURE_EXTENDED_STORAGE_KEYS ) */


/*-------------------------------------------------------------------*/
/* 80   SSM   - Set System Mask                                 [SI] */
/*-------------------------------------------------------------------*/
DEF_INST( set_system_mask )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    /*
     * ECPS:VM - Before checking for prob/priv
     * Check CR6 to see if S-ASSIST is requested
     *
     * If we can process it, then do it
    */

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dossm( regs, b2, effective_addr2 ) == 0)
    {
        return;
    }
#endif

    PRIV_CHECK( regs );

    /* Special operation exception if SSM-suppression is active */
    if ((regs->CR(0) & CR0_SSM_SUPP)
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
      /* SSM-suppression is ignored in XC mode */
      && !SIE_STATE_BIT_ON( regs, MX, XC )
#endif
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, SSM ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Load new system mask value from operand address */
    regs->psw.sysmask = ARCH_DEP( vfetchb )( effective_addr2, b2, regs );

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    /* DAT must be off in XC mode */
    if (SIE_STATE_BIT_ON( regs, MX, XC )
      && (regs->psw.sysmask & PSW_DATMODE) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* For ECMODE, bits 0 and 2-4 of system mask must be zero */
    if ((regs->psw.sysmask & 0xB8) != 0
#if defined( FEATURE_BCMODE )
     && ECMODE( &regs->psw )
#endif
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    SET_IC_MASK( regs );
    TEST_SET_AEA_MODE( regs );

    RETURN_INTCHECK( regs );

} /* end DEF_INST(set_system_mask) */


/*-------------------------------------------------------------------*/
/* AE   SIGP  - Signal Processor                              [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( signal_processor )
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
REGS   *tregs;                          /* -> Target CPU registers   */
GREG    parm;                           /* Signal parameter          */
GREG    status = 0;                     /* Signal status             */
RADR    abs;                            /* Absolute address          */
U16     cpad;                           /* Target CPU address        */
BYTE    order;                          /* SIGP order code           */
#if defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( FEATURE_HERCULES_DIAGCALLS )
int     cpu;                            /* cpu number                */
int     set_arch = 0;                   /* Need to switch mode       */
#endif /*defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )*/
size_t  log_sigp = 0;                   /* Log SIGP instruction flag */
char    log_buf[128];                   /* Log buffer                */

    RS( inst, regs, r1, r3, b2, effective_addr2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    SIE_INTERCEPT( regs );

    /* Perform serialization before starting operation */
    PERFORM_SERIALIZATION( regs );

    /* Load the order code from operand address bits 24-31 */
    order = effective_addr2 & 0xFF;

    /* Load the target CPU address from R3 bits 16-31 */
    cpad = (order != SIGP_SETARCH) ? regs->GR_LHL(r3) : regs->cpuad;

    /* Load the parameter from R1 (if R1 odd), or R1+1 (if even) */
    parm = (r1 & 1) ? regs->GR_L(r1) : regs->GR_L(r1+1);

    PTT_SIG("SIGP",parm,cpad,order);

    /* Return condition code 3 if target CPU does not exist */
    if (cpad >= sysblk.maxcpu)
    {
        PTT_ERR("*SIGP",parm,cpad,order);
        regs->psw.cc = 3;
        return;
    }

    /* Issuing Sense to an offline CPU that is >= hicpu
       now not considered unusual especially since
       we have increased the default max CPU number to 8 */
    if (order == SIGP_SENSE && !IS_CPU_ONLINE( cpad )
     && cpad >= sysblk.hicpu)
    {
        PTT_ERR("*SIGP",parm,cpad,order);
        regs->psw.cc = 3;
        return;
    }

    /* Trace SIGP unless Sense, External Call, Emergency Signal,
       Sense Running State, or the target CPU is configured offline */
    if ((order > LOG_SIGPORDER && order != SIGP_SENSE_RUNNING_STATE)
        || !IS_CPU_ONLINE( cpad ))
    {
        log_sigp = MSGBUF( log_buf,
                "%s%02X: SIGP %-32s (%2.2X) %s%02X, PARM "F_GREG,
                PTYPSTR( regs->cpuad ), regs->cpuad,
                order2name( order ), order,
                PTYPSTR( cpad ), cpad,
                parm);
    }

    /* [4.9.2.1] Claim the use of the CPU signaling and response
       facility, and return condition code 2 if the facility is
       busy.  The sigplock is held while the facility is in
       use by any CPU. */
    if (try_obtain_lock( &sysblk.sigplock ))
    {
        regs->psw.cc = 2;
        if (log_sigp)
        {
            if (regs->insttrace && sysblk.traceFILE)
                tf_0814( regs, order, 2, cpad, status, parm, 0 );
            else
                // "Processor %s: CC %d%s"
                WRMSG( HHC00814, "I", log_buf, 2, "" );
        }
        return;
    }

    /* Obtain the interrupt lock */
    OBTAIN_INTLOCK( regs );

    /* If the cpu is not part of the configuration then return cc3
       Initial CPU reset may IML a processor that is currently not
       part of the configuration, ie configure the cpu implicitly
       online */
    if (order != SIGP_INITRESET
#if defined( FEATURE_S370_CHANNEL )
       && order != SIGP_IMPL
#endif
       && !IS_CPU_ONLINE( cpad ))
    {
        RELEASE_INTLOCK( regs );
        release_lock( &sysblk.sigplock );
        regs->psw.cc = 3;
        if (log_sigp)
        {
            if (regs->insttrace && sysblk.traceFILE)
                tf_0814( regs, order, 3, cpad, status, parm, 0 );
            else
                // "Processor %s: CC %d%s"
                WRMSG(HHC00814, "I", log_buf, 3, "");
        }
        return;
    }

    /* Point to the target CPU -- may be NULL if INITRESET or IMPL */
    tregs = sysblk.regs[ cpad ];

    /* Except for the reset orders, return condition code 2 if the
       target CPU is executing a previous start, stop, restart,
       stop and store status, set prefix, or store status order */
    if ((order != SIGP_RESET
#if defined( FEATURE_S370_CHANNEL )
       && order != SIGP_IMPL
       && order != SIGP_IPR
#endif /* defined( FEATURE_S370_CHANNEL ) */
       && order != SIGP_INITRESET)
       && (tregs) && (tregs->cpustate == CPUSTATE_STOPPING
        || IS_IC_RESTART( tregs )))
    {
        RELEASE_INTLOCK( regs );
        release_lock( &sysblk.sigplock );
        regs->psw.cc = 2;
        if (log_sigp)
        {
            if (regs->insttrace && sysblk.traceFILE)
                tf_0814( regs, order, 2, cpad, status, parm, 0 );
            else
                // "Processor %s: CC %d%s"
                WRMSG( HHC00814, "I", log_buf, 2, "" );
        }
        sched_yield();
        return;
    }

    /* The operator-intervening condition, when present, can be indicated
       in response to all orders, and is indicated in response to sense if
       it precludes the acceptance of any of the installed orders (which
       is always true in our case), but cannot arise as a result of a SIGP
       instruction executed by a CPU addressing itself.
    */
    if (1
        &&  order != SIGP_SENSE         // if this isn't a sense,
        &&  cpad != regs->cpuad         // and we're not addressing ourselves,
        &&  (tregs) && tregs->opinterv  // and operator intervening condition...
    )
    {
        // ...then we cannot proceed
        status |= SIGP_STATUS_OPERATOR_INTERVENING;
    }
    else /* (order == SIGP_SENSE || cpad == regs->cpuad || ((tregs) && !tregs->opinterv)) */
        /* Process signal according to order code */
        switch (order)
        {
        case SIGP_SENSE:
            /* Set status bit 24 if external call interrupt pending */
            if (IS_IC_EXTCALL(tregs))
                status |= SIGP_STATUS_EXTERNAL_CALL_PENDING;

            /* Set status bit 25 if target CPU is stopped */
            if (tregs->cpustate != CPUSTATE_STARTED)
                status |= SIGP_STATUS_STOPPED;

            /* Test for checkstop state */
            if (tregs->checkstop)
                status |= SIGP_STATUS_CHECK_STOP;

            /* Test for operator intervening state */
            if (cpad != regs->cpuad && tregs->opinterv)
                status |= SIGP_STATUS_OPERATOR_INTERVENING;

            break;

        case SIGP_EXTCALL:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Exit with status bit 24 set if a previous external
               call interrupt is still pending in the target CPU */
            if (IS_IC_EXTCALL(tregs))
            {
                status |= SIGP_STATUS_EXTERNAL_CALL_PENDING;
                break;
            }

            /* Raise an external call interrupt pending condition */
            ON_IC_EXTCALL( tregs );
            tregs->extccpu = regs->cpuad;

            break;

#if defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

        case SIGP_COND_EMERGENCY:

            /* Test for checkstop state */
            if (tregs->checkstop)
                status |= SIGP_STATUS_CHECK_STOP;
            else
            {
                U16 check_asn = (parm & 0xFFFF);

                MAYBE_SET_PSW_IA_FROM_IP( tregs );

                if (0

                    /* PSW disabled for external interruptions,
                       I/O interruptions, or both */
                    || (!(tregs->psw.sysmask & PSW_EXTMASK) ||
                        !(tregs->psw.sysmask & PSW_IOMASK))

                    /* CPU in wait state and the PSW instruction
                       address not zero */
                    || ( WAITSTATE( &tregs->psw ) && tregs->psw.IA )

                    /* CPU not in wait state and check_asn value
                       equals an ASN of the CPU (primary, secondary
                       or both) -- regardless of the setting of PSW
                       bits 16-17 */
                    || (1
                        && !WAITSTATE( &tregs->psw )
                        && (check_asn == tregs->CR_LHL(3) ||
                            check_asn == tregs->CR_LHL(4))
                    )
                )
                {
                    /* Raise an emergency signal interrupt pending condition */
                    tregs->emercpu[ regs->cpuad ] = 1;
                    ON_IC_EMERSIG( tregs );
                }
                else
                    status |= SIGP_STATUS_INCORRECT_STATE;
            }
            break;

#endif /* defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        case SIGP_EMERGENCY:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Raise an emergency signal interrupt pending condition */
            ON_IC_EMERSIG( tregs );
            tregs->emercpu[ regs->cpuad ] = 1;

            break;

        case SIGP_START:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Restart the target CPU if it is in the stopped state */
            tregs->cpustate = CPUSTATE_STARTED;

            break;

        case SIGP_STOP:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Put the the target CPU into the stopping state */
            tregs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT( tregs );

            break;

        case SIGP_RESTART:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Make restart interrupt pending in the target CPU */
            ON_IC_RESTART(tregs);
            /* Set cpustate to stopping. If the restart is successful,
               then the cpustate will be set to started in cpu.c */
            if (tregs->cpustate == CPUSTATE_STOPPED)
                tregs->cpustate = CPUSTATE_STOPPING;

            break;

        case SIGP_STOPSTORE:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Indicate store status is required when stopped */
            ON_IC_STORSTAT( tregs );

            /* Put the the target CPU into the stopping state */
            tregs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT( tregs );

            break;

#if defined( FEATURE_S370_CHANNEL )
        case SIGP_IMPL:
        case SIGP_IPR:
#endif
        case SIGP_INITRESET:
            if (!IS_CPU_ONLINE( cpad ))
            {
                configure_cpu(cpad);
                if (!IS_CPU_ONLINE( cpad ))
                {
                    status |= SIGP_STATUS_OPERATOR_INTERVENING;
                    break;
                }
                tregs = sysblk.regs[ cpad ];
            }

#if defined( FEATURE_S370_CHANNEL )
            if (order == SIGP_IMPL || order == SIGP_IPR)
                channelset_reset( tregs );
#endif

            /* Signal initial CPU reset function */
            tregs->sigp_ini_reset = 1;
            tregs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT( tregs );

            break;

#if defined( FEATURE_S370_CHANNEL )
        case SIGP_PR:
            channelset_reset( tregs );
            /* fallthrough*/
#endif
            /* FALLTHRU */
        case SIGP_RESET:
            /* Signal CPU reset function */
            tregs->sigp_reset = 1;
            tregs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT( tregs );

            break;

        case SIGP_SETPREFIX:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Exit with operator intervening if the status is
               stopping, such that a retry can be attempted */
            if (tregs->cpustate == CPUSTATE_STOPPING)
            {
                status |= SIGP_STATUS_OPERATOR_INTERVENING;
                break;
            }

            /* Exit with status bit 22 set if CPU is not stopped */
            if (tregs->cpustate != CPUSTATE_STOPPED)
            {
                status |= SIGP_STATUS_INCORRECT_STATE;
                break;
            }

            /* Obtain new prefix from parameter register bits 1-19
               or bits 1-18 in ESAME mode */
            abs = parm & PX_MASK;

            /* Exit with status bit 23 set if new prefix is invalid */
            if (abs > regs->mainlim)
            {
                status |= SIGP_STATUS_INVALID_PARAMETER;
                break;
            }

            /* Load new value into prefix register of target CPU */
            tregs->PX = abs;

            /* Set pointer to active PSA structure */
            tregs->psa = (PSA_3XX*)(tregs->mainstor + tregs->PX);

            /* Invalidate the ALB and TLB of the target CPU */
            ARCH_DEP( purge_tlb )( tregs );
#if defined( FEATURE_ACCESS_REGISTERS )
            ARCH_DEP( purge_alb )( tregs );
#endif
            /* Perform serialization and checkpoint-sync on target CPU */
//          perform_serialization (tregs);
//          perform_chkpt_sync (tregs);

            break;

        case SIGP_STORE:
            /* Test for checkstop state */
            if (tregs->checkstop)
            {
                status |= SIGP_STATUS_CHECK_STOP;
                break;
            }

            /* Exit with operator intervening if the status is
               stopping, such that a retry can be attempted */
            if (tregs->cpustate == CPUSTATE_STOPPING)
            {
                status |= SIGP_STATUS_OPERATOR_INTERVENING;
                break;
            }

            /* Exit with status bit 22 set if CPU is not stopped */
            if (tregs->cpustate != CPUSTATE_STOPPED)
            {
                status |= SIGP_STATUS_INCORRECT_STATE;
                break;
            }

            /* Obtain status address from parameter register bits 1-22 */
            abs = parm & 0x7FFFFE00;

            /* Exit with status bit 23 set if status address invalid */
            if (abs > regs->mainlim)
            {
                status |= SIGP_STATUS_INVALID_PARAMETER;
                break;
            }

            /* Store status at specified main storage address */
            ARCH_DEP( store_status )( tregs, abs );

            /* Perform serialization and checkpoint-sync on target CPU */
//          perform_serialization (tregs);
//          perform_chkpt_sync (tregs);

            break;

#if defined( _390 )
        case SIGP_STOREX:
        {
#if !defined( FEATURE_BASIC_FP_EXTENSIONS )
            status |= SIGP_STATUS_INVALID_ORDER;
#else
            RADR  absx;     /* abs addr of extended save area */

            /* Test for checkstop state */
            if (tregs->checkstop)
                status |= SIGP_STATUS_CHECK_STOP;

            /* Check if CPU is not stopped */
            if (tregs->cpustate != CPUSTATE_STOPPED)
                status |= SIGP_STATUS_INCORRECT_STATE;

            /* Only proceed if no conditions exist to preclude
               the acceptance of this signal-processor order */
            if (!status)
            {
                /* Obtain status address from parameter register bits 1-22 */
                abs = parm & 0x7FFFFE00;

                /* Exit with status bit 23 set if status address invalid */
                if (abs > regs->mainlim)
                    status |= SIGP_STATUS_INVALID_PARAMETER;
                else
                {
                    /* Fetch the address of the extended save area */
                    absx = ARCH_DEP( fetch_fullword_absolute )( abs+212, tregs );
                    absx &= 0x7FFFF000;

                    /* Invalid parameter if extended save area address invalid */
                    if (absx > regs->mainlim)
                        status |= SIGP_STATUS_INVALID_PARAMETER;
                    else
                    {   int i;  // (work)

                        /* Store status at specified main storage address */
                        ARCH_DEP(store_status) (tregs, abs );

                        /* Store extended status at specified main storage address */
                        for (i=0; i < 32; i++)
                            STORE_FW( tregs->mainstor + absx + (i*4), tregs->fpr[i] );
                        STORE_FW( tregs->mainstor + absx + 128, tregs->fpc );
                        STORE_FW( tregs->mainstor + absx + 132, 0 );
                        STORE_FW( tregs->mainstor + absx + 136, 0 );
                        STORE_FW( tregs->mainstor + absx + 140, 0 );

                        /* Perform serialization and checkpoint-sync on target CPU */
//                      perform_serialization (tregs);
//                      perform_chkpt_sync (tregs);
                    }
                }
            }
#endif /* defined( FEATURE_BASIC_FP_EXTENSIONS ) */
        }
        break;
#endif /* defined( _390 ) */

#if defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( FEATURE_HERCULES_DIAGCALLS )
        case SIGP_SETARCH:

            /* CPU must have ESAME support */
            if (!FACILITY_ENABLED( 001_ZARCH_INSTALLED, regs ))
                status = SIGP_STATUS_INVALID_ORDER;

            PERFORM_SERIALIZATION( regs );
            PERFORM_CHKPT_SYNC( regs );

            for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
                if (IS_CPU_ONLINE( cpu )
                 && sysblk.regs[ cpu ]->cpustate != CPUSTATE_STOPPED
                 && sysblk.regs[ cpu ]->cpuad != regs->cpuad)
                    status |= SIGP_STATUS_INCORRECT_STATE;

            if (!status) {
                switch (parm & 0xFF) {

#if defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

                    case 0:  // from: --  z/Arch   --to-->   390

                        if (sysblk.arch_mode == ARCH_390_IDX)
                            status = SIGP_STATUS_INVALID_PARAMETER;
                        else
                        {
                            sysblk.arch_mode = ARCH_390_IDX;
                            set_arch = 1;

                            INVALIDATE_AIA( regs );
                            regs->captured_zpsw = regs->psw;
                            regs->psw.states |= BIT( PSW_NOTESAME_BIT );
                            regs->PX &= PX_900_MASK;

                            for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
                            {
                                if (IS_CPU_ONLINE( cpu ) &&
                                    sysblk.regs[cpu]->cpuad != regs->cpuad)
                                {
                                    INVALIDATE_AIA( sysblk.regs[ cpu ]);
                                    sysblk.regs[ cpu ]->captured_zpsw = sysblk.regs[cpu]->psw;
                                    sysblk.regs[ cpu ]->psw.states |= BIT( PSW_NOTESAME_BIT );
                                    sysblk.regs[ cpu ]->PX &= PX_900_MASK;
                                }
                            }
                        }
                        break;

                    case 1:   // from: --  390   --to-->   z/Arch

                        if (sysblk.arch_mode == ARCH_900_IDX)
                            status = SIGP_STATUS_INVALID_PARAMETER;
                        else
                        {
                            sysblk.arch_mode = ARCH_900_IDX;
                            set_arch = 1;

                            INVALIDATE_AIA( regs );
                            regs->psw.states &= ~BIT( PSW_NOTESAME_BIT );
                            regs->psw.IA_H = 0;
                            regs->PX &= PX_900_MASK;

                            for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
                            {
                                if (IS_CPU_ONLINE( cpu ) &&
                                    sysblk.regs[ cpu ]->cpuad != regs->cpuad)
                                {
                                    INVALIDATE_AIA( sysblk.regs[ cpu ]);
                                    sysblk.regs[ cpu ]->psw.states &= ~BIT( PSW_NOTESAME_BIT );
                                    sysblk.regs[ cpu ]->psw.IA_H = 0;
                                    sysblk.regs[ cpu ]->PX &= PX_900_MASK;
                                }
                            }
                        }
                        break;

                    case 2:    //  restore CAPTURED z/Arch PSW...

                        if (sysblk.arch_mode == ARCH_900_IDX)
                            status = SIGP_STATUS_INVALID_PARAMETER;
                        else
                        {
                            sysblk.arch_mode = ARCH_900_IDX;
                            set_arch = 1;

                            INVALIDATE_AIA( regs );
                            regs->psw.states &= ~BIT( PSW_NOTESAME_BIT );
                            regs->psw.IA_H = 0;
                            regs->PX &= PX_900_MASK;

                            for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
                            {
                                if (IS_CPU_ONLINE( cpu ) &&
                                    sysblk.regs[cpu]->cpuad != regs->cpuad)
                                {
                                    INVALIDATE_AIA( sysblk.regs[ cpu ]);
                                    sysblk.regs[ cpu ]->psw = sysblk.regs[cpu]->captured_zpsw;
                                    sysblk.regs[ cpu ]->PX &= ARCH_900_IDX;
                                }
                            }
                        }
                        break;

#endif // defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

#if defined( FEATURE_HERCULES_DIAGCALLS )

                    case 37:
                        if (FACILITY_ENABLED( HERC_SIGP_SETARCH_S370, regs ))
                        {
                            if (sysblk.arch_mode == ARCH_370_IDX)
                                status = SIGP_STATUS_INVALID_PARAMETER;
                            else
                            {
                                sysblk.arch_mode = ARCH_370_IDX;
                                set_arch = 1;
                            }
                        }
                        else
                            status = SIGP_STATUS_INVALID_ORDER;
                        break;

#endif /* defined( FEATURE_HERCULES_DIAGCALLS ) */

                    default:
                        status |= SIGP_STATUS_INVALID_PARAMETER;
                } /* end switch (parm & 0xFF) */
            } /* end if (!status) */

            sysblk.dummyregs.arch_mode = sysblk.arch_mode;

            /* Invalidate the ALB and TLB */
            ARCH_DEP( purge_tlb )( regs );
#if defined( FEATURE_ACCESS_REGISTERS )
            ARCH_DEP( purge_alb )( tregs );
#endif

            PERFORM_SERIALIZATION( regs );
            PERFORM_CHKPT_SYNC( regs );

            break;
#endif /*defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( FEATURE_HERCULES_DIAGCALLS )*/

#if defined( FEATURE_009_SENSE_RUN_STATUS_FACILITY )

        case SIGP_SENSE_RUNNING_STATE:

            if (!FACILITY_ENABLED( 009_SENSE_RUN_STATUS, regs ))
                status = SIGP_STATUS_INVALID_ORDER;
            else if (tregs->cpustate != CPUSTATE_STARTED)
                status = SIGP_STATUS_NOT_RUNNING;

            break;

#endif /* defined( FEATURE_009_SENSE_RUN_STATUS_FACILITY ) */

        default:
            status = SIGP_STATUS_INVALID_ORDER;
        } /* end switch (order) */

    /* Release the use of the signalling and response facility */
    release_lock( &sysblk.sigplock );

    /* Wake up the target CPU */
    if (IS_CPU_ONLINE( cpad ))
        WAKEUP_CPU( sysblk.regs[ cpad ]);

    /* Release the interrupt lock */
    RELEASE_INTLOCK( regs );

    if (status)
        PTT_ERR("*SIGP",parm,cpad,order);

    /* If status is non-zero, load the status word into
       the R1 register and return condition code 1 */
    if (status != 0)
    {
        regs->GR_L(r1) = status;
        regs->psw.cc = 1;
    }
    else
        regs->psw.cc = 0;

    /* Log the results if we're interested in this particular SIGP */
    if (log_sigp)
    {
        if (regs->psw.cc == 0)
        {
            if (regs->insttrace && sysblk.traceFILE)
                tf_0814( regs, order, 0, cpad, status, parm, 0 );
            else
                // "Processor %s: CC %d%s"
                WRMSG( HHC00814, "I", log_buf, 0, "" );
        }
        else
        {
            if (regs->insttrace && sysblk.traceFILE)
                tf_0814( regs, order, regs->psw.cc, cpad, status, parm, 1 );
            else
            {
                char buf[40];
                MSGBUF( buf, " status %8.8X", (U32) status );
                // "Processor %s: CC %d%s"
                WRMSG( HHC00814, "I", log_buf, 1, buf );
            }
        }
    }

    /* Perform serialization after completing operation */
    PERFORM_SERIALIZATION( regs );

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    if (status != 0) // r1 modified?
        PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
#endif

#if defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( FEATURE_HERCULES_DIAGCALLS )
    if (set_arch)
    {
        OBTAIN_INTLOCK( regs );
        longjmp( regs->archjmp, 0 );
    }
#endif /*defined( _900 ) || defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )*/

    RETURN_INTCHECK( regs );
}


/*-------------------------------------------------------------------*/
/* B207 STCKC - Store Clock Comparator                           [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_clock_comparator )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Clock value               */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC3, SCKC ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    OBTAIN_INTLOCK( regs );
    {
        /* Save clock comparator value */
        dreg = regs->clkc;

        /* reset the clock comparator pending flag according to
           the setting of the tod clock */
        if (get_tod_clock( regs ) > dreg)
        {
            ON_IC_CLKC( regs );

            /* Roll back the instruction and take the
               timer interrupt if we have a pending CPU timer
               and we are enabled for such interrupts *JJ */
            if (OPEN_IC_CLKC( regs ))
            {
                RELEASE_INTLOCK( regs );
                SET_PSW_IA_AND_MAYBE_IP( regs, PSW_IA_FROM_IP( regs, -4 ));
                RETURN_INTCHECK( regs );
            }
        }
        else
            OFF_IC_CLKC( regs );
    }
    RELEASE_INTLOCK( regs );

    /* Store clock comparator value at operand location */
    ARCH_DEP( vstore8 )( (dreg << 8), effective_addr2, b2, regs );

//  /*debug*/LOGMSG( "Store clock comparator=%16.16"PRIX64"\n", dreg );

    RETURN_INTCHECK( regs );
}


/*-------------------------------------------------------------------*/
/* B6   STCTL - Store Control                                 [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( store_control )
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2 = NULL;                 /* Mainstor pointers         */

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dostctl( regs, r1, r3, b2, effective_addr2 ) == 0)
        return;
#endif

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    FW_CHECK( effective_addr2, regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, STCTL ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Calculate number of regs to store */
    n = ((r3 - r1) & 0xF) + 1;

    /* Calculate number of words to next boundary */
    m = (PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK)) >> 2;

    /* Address of operand beginning */
    p1 = (U32*) MADDR( effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* Get address of next page if boundary crossed */
    if (unlikely( m < n ))
        p2 = (U32*) MADDR( effective_addr2 + (m*4), b2, regs, ACCTYPE_WRITE, regs->psw.pkey );
    else
        m = n;

    /* Store at operand beginning */
    for (i=0; i < m; i++)
        store_fw( p1++, regs->CR_L( (r1 + i) & 0xF ));

    /* Store on next page */
    for (; i < n; i++)
        store_fw( p2++, regs->CR_L( (r1 + i) & 0xF ));

    ITIMER_UPDATE( effective_addr2, (n*4)-1, regs );

} /* end DEF_INST( store_control ) */


/*-------------------------------------------------------------------*/
/* B212 STAP  - Store CPU Address                                [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_cpu_address )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    SIE_INTERCEPT( regs );

    ODD_CHECK( effective_addr2, regs );

    /* Store CPU address at operand address */
    ARCH_DEP( vstore2 )( regs->cpuad, effective_addr2, b2, regs );
}


/*-------------------------------------------------------------------*/
/* B202 STIDP - Store CPU ID                                     [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_cpu_id )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    SIE_INTERCEPT( regs );

    DW_CHECK( effective_addr2, regs );

    /* Store CPU ID at operand address */
    ARCH_DEP( vstore8 )( regs->cpuid, effective_addr2, b2, regs );
}


/*-------------------------------------------------------------------*/
/* B209 STPT  - Store CPU Timer                                  [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_cpu_timer )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S64     dreg;                           /* Double word workarea      */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    DW_CHECK( effective_addr2, regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC3, SPT ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    OBTAIN_INTLOCK( regs );
    {
        /* Save the CPU timer value */
        dreg = get_cpu_timer(regs);

        /* reset the cpu timer pending flag according to its value */
        if (unlikely( dreg < 0 ))
        {
            ON_IC_PTIMER( regs );

            /* Roll back the instruction and take the
               timer interrupt if we have a pending CPU timer
               and we are enabled for such interrupts *JJ */
            if (OPEN_IC_PTIMER( regs ))
            {
                RELEASE_INTLOCK( regs );
                SET_PSW_IA_AND_MAYBE_IP( regs, PSW_IA_FROM_IP( regs, -4 ));
                RETURN_INTCHECK( regs );
            }
        }
        else
            OFF_IC_PTIMER( regs );
    }
    RELEASE_INTLOCK( regs );

    /* Store CPU timer value at operand location */
    ARCH_DEP( vstore8 )( dreg, effective_addr2, b2, regs );

//  /*debug*/LOGMSG( "Store CPU timer=%16.16"PRIX64"\n", dreg );

    RETURN_INTCHECK( regs );
}


/*-------------------------------------------------------------------*/
/* B211 STPX  - Store Prefix                                     [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_prefix )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    SIE_INTERCEPT( regs );

    FW_CHECK( effective_addr2, regs );

    /* Store prefix register at operand address */
    ARCH_DEP( vstore4 )( regs->PX, effective_addr2, b2, regs );
}


/*-------------------------------------------------------------------*/
/* Calculate real CPU capability indicator for the STSI instruction  */
/*                                                                   */
/* The CPU capability indicators are generated as two different,     */
/* dynamically generated 32-bit values, based on the total MIPS per  */
/* real CPU second:                                                  */
/*                                                                   */
/* 1) An MSU value (straight-line generated, rather than assigned    */
/*    by a marketing group); this value is used for an ascending     */
/*    performance indicator.                                         */
/*                                                                   */
/* 2) A performance value with a lower value indicating a faster     */
/*    CPU. The value may be either an unsigned binary integer or a   */
/*    floating point number. If bits 0-8 are zero, it is an integer  */
/*    in the range 0 to 2**23-1. If bits 0-8 are nonzero, it is a    */
/*    32-bit short BFP number.                                       */
/*                                                                   */
/* The generated values are stored in a static area, refreshed on    */
/* type 111 calls, to keep consistency between data areas.           */
/*                                                                   */
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/* 1) The real CPU capability indicators may fluctuate due to a      */
/*    variety of reasons during operation.                           */
/*                                                                   */
/* 2) A real CPU second is defined to be time consumed in the host   */
/*    user state for a given thread.                                 */
/*                                                                   */
/* 3) Host kernel time is considered to be part of management and    */
/*    overhead times.                                                */
/*                                                                   */
/*-------------------------------------------------------------------*/
#if !defined( _STSI_CAPABILITY )
#define _STSI_CAPABILITY


/*-------------------------------------------------------------------*/
/*  MIPSreal - Gather and generate MIPS per real CPU second          */
/*-------------------------------------------------------------------*/
long double MIPSreal()
{
    U64             instcount = 0;     /* Combined instruction count */
    U64             cputime = 0;       /* Combined CPU time in us    */
    int             cpu;               /* CPU loop variable          */
    long double     result;
    REGS*           regs;

    /* Gather real CPU time and instruction count for each processor.
     * Since the real CPU time is managed by the clock, only the
     * corresponding previous instruction counts are used.
     */
    for (cpu = 0; cpu < sysblk.maxcpu; ++cpu)
    {
        /* Loop through online CP CPUs */
        if (1
            && IS_CPU_ONLINE( cpu )
            && SCCB_PTYP_CP == sysblk.ptyp[ cpu ]
        )
        {
            regs = sysblk.regs[ cpu ];

            cputime   += regs->rcputime;
            instcount += regs->prevcount;
        }
    }

    /* Calculate MIPS per real CPU second
     *
     * Dividing the total instruction count by the time in microseconds
     * yields the average MIPS per real CPU second.
     */
    result = (long double)instcount /
             (long double)cputime;

    return (result);
}


/*-------------------------------------------------------------------*/
/*  stsi_capability - Generate capability matrix                     */
/*-------------------------------------------------------------------*/
enum
{
    stsicap_refresh,
    stsicap_read
}   stsicap_request;

static void stsi_capability( const int stsicap_request )
{
    /* The MSU and CPU factors used are estimates; actual MSUs assigned
     * to the hardware are determined by exhaustive testing and vendor
     * marketing requirements.
     */
    static const long double    CPU_Factor = 764748;
    static const long double    MSU_Factor_Multiplier =
                                    9.99209649643744627640427E-1L;

    static long double  MSU_Factor;
    static int          Real_CPUs;
    long double         MIPS_cpu;       /* MIPS per CPU */

    /* If valid data and a read request, just return */
    if (sysblk.cpmcap && stsicap_request == stsicap_read)
        return;

    /* Initialize current real CP CPU count; that is the lessor of the
     * number of online CP engines plus reserved (possible CP) CPU
     * count, or the number of host real logical processors.
     */
    Real_CPUs = get_RealCPCount();

    /* Create an MSU factor based on the number of possible CPs */
    MSU_Factor = 0.125 * pow( MSU_Factor_Multiplier, MIN( Real_CPUs, 1 ) - 1 );

    /* Request MIPS per real CPU second */
    MIPS_cpu = MIPSreal();

    /* Generate a rough MSU and CPU capabilities based on current MIPS
     * per real CPU second, with a range of 1 - 2**23-1.
     */
    sysblk.cpmcap = (U32)(CPU_Factor / MIPS_cpu) & 0x007FFFFFU;

    if (!sysblk.cpmcap)
        sysblk.cpmcap = 1;

    sysblk.cpmcr  = MIPS_cpu * Real_CPUs * MSU_Factor;
    sysblk.cpncap = sysblk.cpmcap;
    sysblk.cpccr  = sysblk.cpcai  = sysblk.cpncr = 0;
    sysblk.cpmpcr = sysblk.cpmtcr = sysblk.cpmcr;
    sysblk.cpnpcr = sysblk.cpntcr = sysblk.cpncr;
    sysblk.cpscap = sysblk.cpmcap;
    sysblk.cpacap = 0;

} /* end function stsi_capability */
#endif /*!defined( _STSI_CAPABILITY )*/


#if defined( FEATURE_STORE_SYSTEM_INFORMATION )
/*-------------------------------------------------------------------*/
/* B27D STSI  - Store System Information                         [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_system_information )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE   *m = 0;                          /* Mainstor address          */
int     i;
U16     offset;                         /* Offset into control block */
U32     curlvl;                         /* Current config level      */
SYSIB111  *sysib111;                    /* Basic machine conf        */
SYSIB121  *sysib121;                    /* Basic machine CPU         */
SYSIB122  *sysib122;                    /* Basic machine CPUs        */
SYSIB221  *sysib221;                    /* LPAR CPU                  */
SYSIB222  *sysib222;                    /* LPAR CPUs                 */
SYSIB322  *sysib322;                    /* VM CPUs                   */
SYSIBVMDB *sysibvmdb;                   /* VM description block      */

#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
SYSIB1512 *sysib1512;                   /* Configuration Topology    */
BYTE      *tle;                         /* Pointer to next TLE       */
TLECNTNR  *tlecntnr;                    /* Container TLE pointer     */
TLECPU    *tlecpu;                      /* CPU TLE Type              */
U64        cpumask;                     /* work                      */
int        cputype;                     /* work                      */
U16        cpuad;                       /* CPU address               */
BYTE       cntnrid;                     /* Container ID              */
#endif /* defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY ) */

                           /*  "0    1    2    3    4    5    6    7" */
static BYTE hexebcdic[16] = { 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
                           /*  "8    9    A    B    C    D    E    F" */
                              0xF8,0xF9,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6 };

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    SIE_INTERCEPT( regs );

    PTT_INF("STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));

#if defined( DEBUG_STSI )
    LOGMSG( "control.c: STSI %d.%d.%d ia="F_VADR" sysib="F_VADR"\n",
            (regs->GR_L(0) & STSI_GPR0_FC_MASK) >> 28,
            regs->GR_L(0) & STSI_GPR0_SEL1_MASK,
            regs->GR_L(1) & STSI_GPR1_SEL2_MASK,
            PSW_IA_FROM_IP( regs,-4 ),
            effective_addr2 );
#endif

     /* Determine current configuration level */
#if defined( _FEATURE_EMULATE_VM )
     if (FACILITY_ENABLED( HERC_VIRTUAL_MACHINE, regs ))
         curlvl = STSI_GPR0_FC_VM;
     else
#endif
#if defined( _FEATURE_HYPERVISOR )
         if (FACILITY_ENABLED( HERC_LOGICAL_PARTITION, regs ))
             curlvl = STSI_GPR0_FC_LPAR;
         else
#endif
             curlvl = STSI_GPR0_FC_BASIC;

    /* Check function code */
    if ((regs->GR_L(0) & STSI_GPR0_FC_MASK) > curlvl)
    {
#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
        if (0
            || !FACILITY_ENABLED( 011_CONFIG_TOPOLOGY, regs )
            || (regs->GR_L(0) & STSI_GPR0_FC_MASK) != STSI_GPR0_FC_CURRINFO
        )
#endif
        {
            PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
#if defined( DEBUG_STSI )
            LOGMSG( "control.c: STSI cc=3 function code invalid\n" );
#endif
            regs->psw.cc = 3;
            return;
        }
    }

    /* Program check if reserved bits not zero */
    if (regs->GR_L(0) & STSI_GPR0_RESERVED
       || regs->GR_L(1) & STSI_GPR1_RESERVED)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Return current level if function code is zero */
    if ((regs->GR_L(0) & STSI_GPR0_FC_MASK) == STSI_GPR0_FC_CURRNUM)
    {
        regs->GR_L(0) |= curlvl;
#if defined( DEBUG_STSI )
        LOGMSG( "control.c: STSI cc=0 R0=%8.8X\n", regs->GR_L(0) );
#endif
        regs->psw.cc = 0;
        return;
    }

    /* Program check if operand not on a page boundary */
    if (effective_addr2 & 0x00000FFF)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Return with cc3 if selector codes invalid */
    {
        U32 fcod = (regs->GR_L(0) & STSI_GPR0_FC_MASK);
        U32 sel1 = (regs->GR_L(0) & STSI_GPR0_SEL1_MASK);
        U32 sel2 = (regs->GR_L(1) & STSI_GPR1_SEL2_MASK);

        /*
          Func-
          tion  Selec- Selec-
          Code  tor 1  tor 2  Information Requested about
          ----  -----  -----  ----------------------------

            1     1      1    Basic-machine configuration
            1     2      1    Basic-machine CPU
            1     2      2    Basic-machine CPUs

            2     2      1    Logical-partition CPU
            2     2      2    Logical-partition CPUs

            3     2      2    Virtual-machine CPUs

            15    1      2    Topology information of current configuration
            15    1      3    (not currently supported) (TODO!)
            15    1      4    (not currently supported) (TODO!)
            15    1      5    (not currently supported) (TODO!)
            15    1      6    (not currently supported) (TODO!)
        */
        if (0
            || (STSI_GPR0_FC_BASIC == fcod
                && (0
                    || sel1 == 0
                    || sel1 >  2
                    || sel2 == 0
                    || sel2 >  2
                   )
               )
#if defined( _FEATURE_HYPERVISOR )
            || (STSI_GPR0_FC_LPAR == fcod
                && (0
                    || !FACILITY_ENABLED( HERC_LOGICAL_PARTITION, regs )
                    || !sysblk.lparmode
                    || sel1 != 2
                    || sel2 == 0
                    || sel2 >  2
                   )
               )
#endif
#if defined( _FEATURE_EMULATE_VM )
            || (STSI_GPR0_FC_VM == fcod
                && (0
                    || sel1 != 2
                    || sel2 != 2
                   )
               )
#endif
#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
            || (STSI_GPR0_FC_CURRINFO == fcod
                && (0
                    || !FACILITY_ENABLED( 011_CONFIG_TOPOLOGY, regs )
                    || sel1 != 1
#if 1 // (the only selector 2 value we currently support)
                    || sel2 != 2
#else // (TODO: provide support for selectors 3-6)
                    || sel2 < 2
                    || sel2 > 6
#endif
                   )
               )
#endif
        )
        {
            PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
#if defined( DEBUG_STSI )
            LOGMSG( "control.c: STSI cc=3 selector codes invalid\n" );
#endif
            regs->psw.cc = 3;
            return;
        }
    }

    switch (regs->GR_L(0) & STSI_GPR0_FC_MASK) {

    case STSI_GPR0_FC_BASIC:

        /* Obtain absolute address of main storage block,
           check protection, and set reference and change bits */
        m = MADDR( effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );

        switch (regs->GR_L(0) & STSI_GPR0_SEL1_MASK) {

        case 1:

            switch (regs->GR_L(1) & STSI_GPR1_SEL2_MASK) {

            case 1:
                /* Basic-machine configuration */
                sysib111 = (SYSIB111*)(m);
                memset( sysib111, 0, MAX( sizeof(SYSIB111), 64*4 ));
                stsi_capability( stsicap_refresh );
                sysib111->flag1 |= SYSIB111_PFLAG;
                get_manufacturer( sysib111->manufact );
                get_model( sysib111->model );
                for (i=0; i < 4; i++)
                    sysib111->type[i] =
                        hexebcdic[(sysblk.cpumodel >> (12 - (i*4))) & 0x0F];
                get_modelcapa( sysib111->modcapaid );
                if (sysib111->modcapaid[0] == '\0')
                    memcpy( sysib111->modcapaid, sysib111->model, sizeof( sysib111->model ));
                get_modelperm( sysib111->mpci );
                get_modeltemp( sysib111->mtci );
                bld_sysib_sequence( sysib111->seqc );
                get_plant( sysib111->plant );
                STORE_FW( sysib111->mcaprating,  sysblk.cpmcr  );
                STORE_FW( sysib111->mpcaprating, sysblk.cpmpcr );
                STORE_FW( sysib111->mtcaprating, sysblk.cpmtcr );
                for (i=0; i < 5; i++)
                {
                    sysib111->typepct[i] = 100;
                }
                if (sysblk.cpcai)
                {
                    sysib111->ccr = 1;
                    sysib111->cai = sysblk.cpcai;
                    STORE_FW( sysib111->ncaprating,  sysblk.cpncr  );
                    STORE_FW( sysib111->npcaprating, sysblk.cpnpcr );
                    STORE_FW( sysib111->ntcaprating, sysblk.cpntcr );
                }
                regs->psw.cc = 0;
                break;

            default:
                PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
                regs->psw.cc = 3;
            } /* selector 2 */
            break;

        case 2:

            switch (regs->GR_L(1) & STSI_GPR1_SEL2_MASK) {

            case 1:
                /* Basic-machine Current CPU */
                sysib121 = (SYSIB121*)(m);
                memset( sysib121, 0, MAX( sizeof( SYSIB121 ), 64*4 ));
                bld_sysib_sequence( sysib121->seqc );
                get_plant( sysib121->plant );
                STORE_HW( sysib121->cpuad, regs->cpuad );
                regs->psw.cc = 0;
                break;

            case 2:
                /* Basic-machine All CPUs */
                sysib122 = (SYSIB122*)(m);
                memset( sysib122, 0, MAX( sizeof( SYSIB122 ), 64*4 ));
                stsi_capability( stsicap_read );
                if (0)  /* Change -0- to the proper test when Hercules supports alternate CPUs */
                {
                    sysib122->format = 1;
                    offset = (U16)(sysib122->accap - (BYTE*)sysib122);
                    STORE_HW( sysib122->accoff, offset );
                }
                STORE_FW( sysib122->nccap,   sysblk.cpncap );
                STORE_FW( sysib122->sccap,   sysblk.cpscap );
                STORE_FW( sysib122->cap,     sysblk.cpmcap );
                STORE_HW( sysib122->totcpu,  MAX_CPU_ENGS  );
                STORE_HW( sysib122->confcpu, sysblk.cpus   );
                STORE_HW( sysib122->sbcpu,   sysblk.maxcpu - sysblk.cpus   );
                STORE_HW( sysib122->resvcpu, MAX_CPU_ENGS  - sysblk.maxcpu );
                get_mpfactors( (BYTE*)sysib122->mpfact );
                if (sysib122->format)
                {
                    STORE_FW( sysib122->accap, sysblk.cpacap );
                    get_mpfactors( (BYTE*)sysib122->ampfact );
                }
                regs->psw.cc = 0;
                break;

            default:
                PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
                regs->psw.cc = 3;
            } /* selector 2 */
            break;

        default:
            PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
            regs->psw.cc = 3;
        } /* selector 1 */
        break;

    case STSI_GPR0_FC_LPAR:

#if defined( _FEATURE_HYPERVISOR )
         if (!FACILITY_ENABLED( HERC_LOGICAL_PARTITION, regs ))
         {
             PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
             regs->psw.cc = 3;
             break;
         }
#endif

        /* Obtain absolute address of main storage block,
           check protection, and set reference and change bits */
        m = MADDR( effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );

        switch (regs->GR_L(0) & STSI_GPR0_SEL1_MASK) {

        case 2:

            switch (regs->GR_L(1) & STSI_GPR1_SEL2_MASK) {

            case 1:
                /* Logical-partition Current CPU */
                sysib221 = (SYSIB221 *)(m);
                memset( sysib221, 0, MAX( sizeof( SYSIB221 ), 64*4 ));
                bld_sysib_sequence( sysib221->seqc );
                get_plant( sysib221->plant );
                STORE_HW( sysib221->lcpuid, regs->cpuad );
                STORE_HW( sysib221->cpuad,  regs->cpuad );
                regs->psw.cc = 0;
                break;

            case 2:
                /* Logical-partition All CPUs */
                sysib222 = (SYSIB222 *)(m);
                memset( sysib222, 0, MAX( sizeof( SYSIB222 ), 64*4 ));
                STORE_HW( sysib222->lparnum, sysblk.lparnum );
                sysib222->lcpuc = SYSIB222_LCPUC_SHARED;
                STORE_HW( sysib222->totcpu,  MAX_CPU_ENGS );
                STORE_HW( sysib222->confcpu, sysblk.cpus  );
                STORE_HW( sysib222->sbcpu,   sysblk.maxcpu - sysblk.cpus   );
                STORE_HW( sysib222->resvcpu, MAX_CPU_ENGS  - sysblk.maxcpu );
                get_lparname(sysib222->lparname);
                /* FIXME: Should be a percentage of 1000, where 1000
                 *        represents 1.000 and capable of full CPU
                 *        utilization. This usually is NOT the case for
                 *        an LPAR, by definition.
                 */
                STORE_FW( sysib222->lparcaf, 1000 ); /* Full capability factor */
                STORE_FW( sysib222->mdep[0], 1000 ); /* ZZ nonzero value */
                STORE_FW( sysib222->mdep[1], 1000 ); /* ZZ nonzero value */
                STORE_HW( sysib222->shrcpu,  sysblk.cpus );
                regs->psw.cc = 0;
                break;

            default:
                PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
                regs->psw.cc = 3;
            } /* selector 2 */
            break;

        default:
            PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
            regs->psw.cc = 3;
        } /* selector 1 */
        break;

    case STSI_GPR0_FC_VM:

        /* Obtain absolute address of main storage block,
           check protection, and set reference and change bits */
        m = MADDR( effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );

        sysib322 = (SYSIB322 *)(m);
        memset( sysib322, 0, sizeof( SYSIB322 ));
        sysib322->dbct = 1;
        sysibvmdb = (SYSIBVMDB *)&sysib322->vmdb[0];
        STORE_HW( sysibvmdb->totcpu,  MAX_CPU_ENGS );
        STORE_HW( sysibvmdb->confcpu, sysblk.cpus  );
        STORE_HW( sysibvmdb->sbcpu,   sysblk.maxcpu - sysblk.cpus   );
        STORE_HW( sysibvmdb->resvcpu, MAX_CPU_ENGS  - sysblk.maxcpu );
        get_vmid( sysibvmdb->vmname );
        STORE_FW( sysibvmdb->vmcaf, 1000 ); /* Full capability factor */
        get_cpid( sysibvmdb->cpid );

        regs->psw.cc = 0;
        break;

#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
    case STSI_GPR0_FC_CURRINFO:

        if (!FACILITY_ENABLED( 011_CONFIG_TOPOLOGY, regs ))
        {
            PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
            regs->psw.cc = 3;
            break;
        }

        /* Obtain absolute address of main storage block,
           check protection, and set reference and change bits */
        m = MADDR( effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );

        switch (regs->GR_L(0) & STSI_GPR0_SEL1_MASK) {

        case 1:

            switch (regs->GR_L(1) & STSI_GPR1_SEL2_MASK) {

            case 2:
                /* Topology information of current configuration */
                sysib1512 = (SYSIB1512 *)(m);
                memset( sysib1512, 0, sizeof( SYSIB1512 ));

                sysib1512->mnest = 2;
                sysib1512->mag[4] = 1;
                sysib1512->mag[5] = sysblk.maxcpu;

                tle = sysib1512->tles;
                cntnrid = 1;
                cpuad = 0;

                /* Build a container TLE */
                tlecntnr = (TLECNTNR *)tle;
                memset( tlecntnr, 0x00, sizeof( TLECNTNR ));
                tlecntnr->nl = 1;
                tlecntnr->cntnrid = cntnrid;
                tle += sizeof( TLECNTNR );

                /* For each type of CPU */
                for (cputype = 0; cputype <= MAX_SCCB_PTYP; cputype++)
                {
                    tlecpu = (TLECPU *)tle;

                    /* For each CPU of this type */
                    cpumask = 0;
                    for (i=0; i < sysblk.hicpu; i++)
                    {
                        if (1
                            && sysblk.regs[i]
                            && sysblk.regs[i]->configured
                            && sysblk.ptyp[i] == cputype
                        )
                        {
                            /* Initialize new TLE for this type */
                            if (!cpumask)
                            {
                                memset( tlecpu, 0, sizeof( TLECPU ));
                                tlecpu->nl = 0;
                                if (sysblk.topology == TOPOLOGY_VERT) {
                                    tlecpu->flags = CPUTLE_FLAG_VERTMED;
                                } else {
                                    tlecpu->flags = CPUTLE_FLAG_HORIZ;
                                }
                                tlecpu->cpuadorg = cpuad;
                                tlecpu->cputype = cputype;
                            }
                            /* Update CPU mask field for this type */
                            cpumask |= 0x8000000000000000ULL >> sysblk.regs[i]->cpuad;
                        }
                    }
                    /* Bump to next TLE */
                    if (cpumask)
                    {
                        STORE_DW( &tlecpu->cpumask, cpumask );
                        tle += sizeof(TLECPU);
                    }
                }

                /* Save the length of this System Information Block */
                STORE_HW( sysib1512->len, (U16)( tle - (BYTE*)sysib1512 ));

                /* Successful completion */
                regs->psw.cc = 0;

                /* Clear topology-change-report-pending condition */
                OBTAIN_INTLOCK( regs );
                {
                    sysblk.topchnge = 0;
                }
                RELEASE_INTLOCK( regs );
                break;

            case 3: // TODO...
            case 4: // TODO...
            case 5: // TODO...
            case 6: // TODO...

            default:
                PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
                regs->psw.cc = 3;
            } /* selector 2 */
            break;

        default:
            PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
            regs->psw.cc = 3;
        } /* selector 1 */
        break;
#endif /* defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY ) */

    default:
        PTT_ERR("*STSI",regs->GR_L(0),regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff));
        regs->psw.cc = 3;
    } /* function code */

#if defined( DEBUG_STSI )
    /* Display results of STSI */
    LOGMSG( "control.c: STSI cc=%d\n", regs->psw.cc );
    for (i=0; i < 256; i += 16, m += 16) {
        BYTE c, s[17]; int j;
        for (j=0; j < 16; j++) {
            c = guest_to_host( m[j] );
            s[j] = isprint((unsigned char)c) ? c : '.';
        }
        s[j] = '\0';
        LOGMSG( "+%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X *%s*\n",
                i, m[0],m[1],m[ 2],m[ 3],m[ 4],m[ 5],m[ 6],m[ 7],
                   m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15], s );
    }
#endif /* defined( DEBUG_STSI ) */

} /* end DEF_INST(store_system_information) */
#endif /* defined( FEATURE_STORE_SYSTEM_INFORMATION ) */


/*-------------------------------------------------------------------*/
/* AC   STNSM - Store Then And System Mask                      [SI] */
/*-------------------------------------------------------------------*/
DEF_INST( store_then_and_system_mask )
{
BYTE    i2;                             /* Immediate byte of opcode  */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */

    SI( inst, regs, i2, b1, effective_addr1 );
    PER_ZEROADDR_XCHECK( regs, b1 );

#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dostnsm( regs, b1, effective_addr1, i2 ) == 0)
    {
        return;
    }
#endif

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, STNSM ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Store current system mask value into storage operand */
    ARCH_DEP( vstoreb )( regs->psw.sysmask, effective_addr1, b1, regs );

    /* AND system mask with immediate operand */
    regs->psw.sysmask &= i2;

    SET_IC_MASK( regs );
    TEST_SET_AEA_MODE( regs );

#if defined( FEATURE_002_ZARCH_ACTIVE_FACILITY )
    if ((effective_addr1 == 0x00000950) && (i2 == 0xff))
    {
        if (!ARCH_DEP( translate_addr )(           16, 0, regs, ACCTYPE_READ ))
        {
            VADR  va_1 = ARCH_DEP( vfetch4 )(      16, 0, regs ) + 140;
            if (!ARCH_DEP( translate_addr )(     va_1, 0, regs, ACCTYPE_READ ))
            {
                VADR va_2 = ARCH_DEP( vfetch4 )( va_1, 0, regs ) + 476;
                if (!ARCH_DEP( translate_addr )( va_2, 0, regs, ACCTYPE_READ ))
                {
                    if (ARCH_DEP( vfetch2 )(     va_2, 0, regs ) == 258)
                        ARCH_DEP( vstoreb )( 0x00, 1376 + ARCH_DEP( vfetch4 )( 2892, 0, regs ), 0, regs );
                }
            }
        }
    }
#endif

    RETURN_INTCHECK( regs );
} /* end DEF_INST(store_then_and_system_mask) */


/*-------------------------------------------------------------------*/
/* AD   STOSM - Store Then Or System Mask                       [SI] */
/*-------------------------------------------------------------------*/
DEF_INST( store_then_or_system_mask )
{
BYTE    i2;                             /* Immediate byte of opcode  */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */

    SI( inst, regs, i2, b1, effective_addr1 );
    PER_ZEROADDR_XCHECK( regs, b1 );

#if defined( FEATURE_ECPSVM )
    if (ecpsvm_dostosm( regs, b1, effective_addr1, i2 ) == 0)
    {
        return;
    }
#endif

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC1, STOSM ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Store current system mask value into storage operand */
    ARCH_DEP( vstoreb )( regs->psw.sysmask, effective_addr1, b1, regs );

    /* OR system mask with immediate operand */
    regs->psw.sysmask |= i2;

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    /* DAT must be off in XC mode */
    if (1
        && SIE_STATE_BIT_ON( regs, MX, XC )
        && (regs->psw.sysmask & PSW_DATMODE)
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* For ECMODE, bits 0 and 2-4 of system mask must be zero */
    if (
#if defined( FEATURE_BCMODE )
        ECMODE( &regs->psw ) &&
#endif /* defined( FEATURE_BCMODE ) */
                            (regs->psw.sysmask & 0xB8) != 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    SET_IC_MASK( regs );
    TEST_SET_AEA_MODE( regs );

    RETURN_INTCHECK( regs );
} /* end DEF_INST(store_then_or_system_mask) */


/*-------------------------------------------------------------------*/
/* B246 STURA - Store Using Real Address                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( store_using_real_address )
{
int     r1, r2;                         /* Values of R fields        */
RADR    n;                              /* Unsigned work             */

    RRE( inst, regs, r1, r2 );
    PER_ZEROADDR_CHECK( regs, r2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    /* R2 register contains operand real storage address */
    n = regs->GR(r2) & ADDRESS_MAXWRAP( regs );

    /* Program check if operand not on fullword boundary */
    FW_CHECK( n, regs );

    /* Store R1 register at second operand location */
    ARCH_DEP( vstore4 )( regs->GR_L(r1), n, USE_REAL_ADDR, regs );

#if defined( FEATURE_PER2 )
    /* Storage alteration must be enabled for STURA to be recognised */
    if (1
        && EN_IC_PER_SA(    regs ) && !IS_PER_SUPRESS( regs, CR9_SA    )
        && EN_IC_PER_STURA( regs ) && !IS_PER_SUPRESS( regs, CR9_STURA )
    )
    {
        ON_IC_PER_SA( regs );
        ON_IC_PER_STURA( regs );
        regs->perc &= 0xFFFC;    /* zero STD ID part of PER code */
    }
#endif
} /* end DEF_INST(store_using_real_address) */


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* B24C TAR   - Test Access                                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( test_access )
{
int     r1, r2;                         /* Values of R fields        */
U32     asteo;                          /* Real address of ASTE      */
U32     aste[16];                       /* ASN second table entry    */

    RRE( inst, regs, r1, r2 );

    /* All control instructions are restricted in transaction mode */
    TXF_INSTR_CHECK( regs );

    /* Program check if ASF control bit is zero */
    if (!ASF_ENABLED( regs ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );

    /* Set condition code 0 if ALET value is 0 */
    if (regs->AR(r1) == ALET_PRIMARY)
    {
        regs->psw.cc = 0;
        return;
    }

    /* Set condition code 3 if ALET value is 1 */
    if (regs->AR(r1) == ALET_SECONDARY)
    {
        regs->psw.cc = 3;
        return;
    }

    /* Perform ALET translation using EAX value from register
       R2 bits 0-15, and set condition code 3 if exception */
    if (ARCH_DEP( translate_alet )( regs->AR(r1), regs->GR_LHH(r2),
                        ACCTYPE_TAR,
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
                        SIE_STATE_BIT_ON( regs, MX, XC ) ? HOSTREGS :
#endif
                        regs,
                        &asteo, aste ))
    {
        regs->psw.cc = 3;
        return;
    }

    /* Set condition code 1 or 2 according to whether
       the ALET designates the DUCT or the PASTE */
    regs->psw.cc = (regs->AR(r1) & ALET_PRI_LIST) ? 2 : 1;
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


#if defined( FEATURE_TEST_BLOCK )
/*-------------------------------------------------------------------*/
/* B22C TB    - Test Block                                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( test_block )
{
int     r1, r2;                         /* Values of R fields        */
RADR    n;                              /* Real address              */

    RRE( inst, regs, r1, r2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

#if defined( FEATURE_REGION_RELOCATE )
    if (SIE_STATE_BIT_OFF( regs, MX, RRF ) && !regs->sie_pref)
#endif
        SIE_INTERCEPT( regs );

    /* Load 4K block address from R2 register */
    n = regs->GR(r2) & ADDRESS_MAXWRAP_E( regs );
    n &= XSTORE_PAGEMASK;  /* 4K boundary */

    PERFORM_SERIALIZATION( regs );
    {
        /* Addressing exception if block is outside main storage */
        if (n > regs->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

        /* Protection exception if low-address protection is set */
        if (ARCH_DEP( is_low_address_protected )( n, regs ))
        {
#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
            regs->TEA = (n & STORAGE_KEY_PAGEMASK);
            regs->excarid = 0;
#endif
            ARCH_DEP( program_interrupt )( regs, PGM_PROTECTION_EXCEPTION );
        }

        /* Convert real address to absolute address */
        n = APPLY_PREFIXING( n, regs->PX );

        /* Clear the 4K block to zeroes */
        clear_page_4K( regs->mainstor + n );

        /* Set condition code 0 if storage usable, 1 if unusable.
           Note: we use the internal "_get_storage_key" function
           so we're returned the internal STORKEY_BADFRM bit too.
        */
        if (ARCH_DEP( _get_storage_key )( n, SKEY_K ) & STORKEY_BADFRM)
            regs->psw.cc = 1;
        else
            regs->psw.cc = 0;
    }
    PERFORM_SERIALIZATION( regs );

    /* Clear general register 0 */
    SET_GR_A( 0, regs, 0 );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( 0 ));
}
#endif /* defined( FEATURE_TEST_BLOCK ) */


/*-------------------------------------------------------------------*/
/* E501 TPROT - Test Protection                                [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST( test_protection )
{
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
RADR    aaddr;                          /* Absolute address          */
BYTE    skey;                           /* Storage key               */
BYTE    akey;                           /* Access key                */

    SSE( inst, regs, b1, effective_addr1, b2, effective_addr2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_ON( regs, IC2, TPROT ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

    /* Convert logical address to real address */
    if (REAL_MODE( &regs->psw ))
    {
        regs->dat.protect = 0;
        regs->dat.raddr   = effective_addr1;
    }
    else
    {
        /* Return condition code 3 if translation exception */
        if (ARCH_DEP( translate_addr )( effective_addr1, b1, regs, ACCTYPE_TPROT ))
        {
            regs->psw.cc = 3;
            return;
        }
    }

    /* Convert real address to absolute address */
    aaddr = APPLY_PREFIXING( regs->dat.raddr, regs->PX );

    /* Program check if absolute address is outside main storage */
    if (aaddr > regs->mainlim)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs )  && !regs->sie_pref)
    {
        /* Under SIE TPROT also indicates if the host is using
           page protection */
        /* Translate to real address - eventually using an access
           register if the guest is in XC mode */
        if (SIE_TRANSLATE_ADDR( regs->sie_mso + aaddr,
                                b1>0 &&
                                  MULTIPLE_CONTROLLED_DATA_SPACE( regs ) ?
                                    b1 : USE_PRIMARY_SPACE,
                                HOSTREGS, ACCTYPE_SIE ))
            longjmp( regs->progjmp, SIE_INTERCEPT_INST );

        /* Convert host real address to host absolute address */
        aaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

        if (aaddr > HOSTREGS->mainlim)
            ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );
    }
#endif /* defined( _FEATURE_SIE ) */

    /* Load access key from operand 2 address bits 24-27 */
    akey = effective_addr2 & 0xF0;

    /* Load the storage key for the absolute address */
    skey = ARCH_DEP( get_storage_key )( aaddr );

    /* Return condition code 2 if location is fetch protected */
    if (ARCH_DEP( is_fetch_protected )( effective_addr1, skey, akey, regs ))
        regs->psw.cc = 2;
    else
        /* Return condition code 1 if location is store protected */
        if (ARCH_DEP( is_store_protected )( effective_addr1, skey, akey, regs ))
            regs->psw.cc = 1;
        else
            /* Return condition code 0 if location is not protected */
            regs->psw.cc = 0;
}


#if defined( FEATURE_TRACING )
/*-------------------------------------------------------------------*/
/* 99   TRACE - Trace                                         [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( trace )
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
#if defined( FEATURE_TRACING )
U32     op;                             /* Operand                   */
#endif /* defined( FEATURE_TRACING ) */

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );
    FW_CHECK( effective_addr2, regs );

#if defined( FEATURE_TRACING )
    /* Exit if explicit tracing (control reg 12 bit 31) is off */
    if ((regs->CR(12) & CR12_EXTRACE) == 0)
        return;

    /* Fetch the trace operand */
    op = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );

    /* Exit if bit zero of the trace operand is one */
    if ((op & 0x80000000))
        return;

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    regs->CR(12) = ARCH_DEP( trace_tr )( r1, r3, op, regs );

#endif /* defined( FEATURE_TRACING ) */

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );
}
#endif /* defined( FEATURE_TRACING ) */


#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "control.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "control.c"
  #endif

#endif /*!defined( _GEN_ARCH )*/
