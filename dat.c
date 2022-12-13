/* DAT.C        (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2021                             */
/*              Dynamic Address Translation                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements the DAT, ALET, and ASN translation         */
/* functions of the ESA/390 architecture, described in the manual    */
/* SA22-7201-04 ESA/390 Principles of Operation.  The numbers in     */
/* square brackets in the comments refer to sections in the manual.  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      S/370 DAT support by Jay Maynard (as described in            */
/*      GA22-7000 System/370 Principles of Operation)                */
/*      Clear remainder of ASTE when ASF=0 - Jan Jaeger              */
/*      S/370 DAT support when running under SIE - Jan Jaeger        */
/*      ESAME DAT support by Roger Bowler (SA22-7832)                */
/*      ESAME ASN authorization and ALET translation - Roger Bowler  */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DAT_C
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

extern inline void ARCH_DEP( purge_tlb )( REGS* regs );
extern inline void ARCH_DEP( purge_tlb_all )( REGS* regs, U16 cpuad );
extern inline void ARCH_DEP( purge_tlbe_all )( REGS* regs, RADR pfra, U16 cpuad );

#if defined( FEATURE_ACCESS_REGISTERS )
extern inline void ARCH_DEP( purge_alb )( REGS* regs );
extern inline void ARCH_DEP( purge_alb_all )( REGS* regs );
#endif

#if defined( FEATURE_DUAL_ADDRESS_SPACE )
extern inline bool ARCH_DEP( authorize_asn )( U16 ax, U32 aste[], int atemask, REGS* regs );
#endif

extern inline BYTE* ARCH_DEP( maddr_l )( VADR addr, size_t len, const int arn, REGS* regs, const int acctype, const BYTE akey );

/*-------------------------------------------------------------------*/
/*                     update_psw_ia                                 */
/*-------------------------------------------------------------------*/
void ARCH_DEP( update_psw_ia )( REGS* regs, int n )
{
    regs->psw.IA += n;
    regs->psw.IA &= ADDRESS_MAXWRAP( regs );
    PTT_PGM( "PGM IA+-sie", regs->psw.IA, regs->instinvalid, n );
}

/*-------------------------------------------------------------------*/
/*                     update_guest_psw_ia                           */
/*-------------------------------------------------------------------*/
void ARCH_DEP( update_guest_psw_ia )( REGS* regs, int n )
{
    switch (GUESTREGS->arch_mode)
    {
    case ARCH_370_IDX: s370_update_psw_ia( GUESTREGS, n ); break;
    case ARCH_390_IDX: s390_update_psw_ia( GUESTREGS, n ); break;
    case ARCH_900_IDX: z900_update_psw_ia( GUESTREGS, n ); break;
    default: CRASH();
    }
}

/*-------------------------------------------------------------------*/
/*                   set_aea_common                                  */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_aea_common )( REGS* regs )
{
    SET_AEA_COMMON( regs );
}

/*-------------------------------------------------------------------*/
/*                   set_guest_aea_common                            */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_guest_aea_common )( REGS* regs )
{
    switch (GUESTREGS->arch_mode)
    {
    case ARCH_370_IDX: s370_set_aea_common( GUESTREGS ); break;
    case ARCH_390_IDX: s390_set_aea_common( GUESTREGS ); break;
    case ARCH_900_IDX: z900_set_aea_common( GUESTREGS ); break;
    default: CRASH();
    }
}

/*-------------------------------------------------------------------*/
/*                     invalidate_aia                                */
/*-------------------------------------------------------------------*/
void ARCH_DEP( invalidate_aia )( REGS* regs )
{
    INVALIDATE_AIA( regs );
}
/*-------------------------------------------------------------------*/
/*                     set_ic_mask                                   */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_ic_mask )( REGS* regs )
{
    SET_IC_MASK( regs );
}
/*-------------------------------------------------------------------*/
/*                     set_aea_mode                                  */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_aea_mode )( REGS* regs )
{
    SET_AEA_MODE( regs );
}

/*-------------------------------------------------------------------*/
/*                   invalidate_guest_aia                            */
/*-------------------------------------------------------------------*/
void ARCH_DEP( invalidate_guest_aia )( REGS* regs )
{
    switch (GUESTREGS->arch_mode)
    {
    case ARCH_370_IDX: s370_invalidate_aia( GUESTREGS ); break;
    case ARCH_390_IDX: s390_invalidate_aia( GUESTREGS ); break;
    case ARCH_900_IDX: z900_invalidate_aia( GUESTREGS ); break;
    default: CRASH();
    }
}

/*-------------------------------------------------------------------*/
/*                   set_guest_ic_mask                               */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_guest_ic_mask )( REGS* regs )
{
    switch (GUESTREGS->arch_mode)
    {
    case ARCH_370_IDX: s370_set_ic_mask( GUESTREGS ); break;
    case ARCH_390_IDX: s390_set_ic_mask( GUESTREGS ); break;
    case ARCH_900_IDX: z900_set_ic_mask( GUESTREGS ); break;
    default: CRASH();
    }
}

/*-------------------------------------------------------------------*/
/*                   set_guest_aea_mode                              */
/*-------------------------------------------------------------------*/
void ARCH_DEP( set_guest_aea_mode )( REGS* regs )
{
    switch (GUESTREGS->arch_mode)
    {
    case ARCH_370_IDX: s370_set_aea_mode( GUESTREGS ); break;
    case ARCH_390_IDX: s390_set_aea_mode( GUESTREGS ); break;
    case ARCH_900_IDX: z900_set_aea_mode( GUESTREGS ); break;
    default: CRASH();
    }
}

/*-------------------------------------------------------------------*/
/*                      do_purge_tlb                                 */
/*-------------------------------------------------------------------*/
void ARCH_DEP( do_purge_tlb )( REGS* regs )
{
    INVALIDATE_AIA( regs );

    if (((++regs->tlbID) & TLBID_BYTEMASK) == 0)
    {
        memset( &regs->tlb.vaddr, 0, TLBN * sizeof( DW ));
        regs->tlbID = 1;
    }
}

/*-------------------------------------------------------------------*/
/* Purge entire translation lookaside buffer for this CPU            */
/*-------------------------------------------------------------------*/
void ARCH_DEP( purge_tlb )( REGS* regs )
{
    /* Do it for the current architecture first */
    ARCH_DEP( do_purge_tlb )( regs );

#if defined( _FEATURE_SIE )

    /* Also clear the guest registers in the SIE copy */
    if (regs->host && GUESTREGS)
    {
        switch (GUESTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_do_purge_tlb( GUESTREGS ); break;
        case ARCH_390_IDX: s390_do_purge_tlb( GUESTREGS ); break;
        case ARCH_900_IDX: z900_do_purge_tlb( GUESTREGS ); break;
        default: CRASH();
        }
    }
#endif // defined( _FEATURE_SIE )
}


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/*                 purge_alb helper function                         */
/*-------------------------------------------------------------------*/
void ARCH_DEP( do_purge_alb )( REGS* regs )
{
    int  i;
    for (i=1; i < 16; i++)
        if (regs->AEA_AR(i) >= CR_ALB_OFFSET)
            regs->AEA_AR(i) = 0;
}

/*-------------------------------------------------------------------*/
/* Purge the ART lookaside buffer for this CPU                       */
/*-------------------------------------------------------------------*/
void ARCH_DEP( purge_alb )( REGS* regs )
{
    /* Do it for the current architecture first */
    ARCH_DEP( do_purge_alb )( regs );

#if defined( _FEATURE_SIE )
    /* Also clear the guest registers in the SIE copy */
    if (regs->host && GUESTREGS)
    {
        switch (GUESTREGS->arch_mode)
        {
        case ARCH_370_IDX: /* No access regs for 370! */   break;
        case ARCH_390_IDX: s390_do_purge_alb( GUESTREGS ); break;
        case ARCH_900_IDX: z900_do_purge_alb( GUESTREGS ); break;
        default: CRASH();
        }
    }
#endif // defined( _FEATURE_SIE )
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


#if defined( FEATURE_DUAL_ADDRESS_SPACE )
/*-------------------------------------------------------------------*/
/* Translate ASN to produce address-space control parameters         */
/*                                                                   */
/* Input:                                                            */
/*      asn     Address space number to be translated                */
/*      regs    Pointer to the CPU register context                  */
/*      asteo   Pointer to a word to receive real address of ASTE    */
/*      aste    Pointer to 16-word area to receive a copy of the     */
/*              ASN second table entry associated with the ASN       */
/*                                                                   */
/* Output:                                                           */
/*      If successful, the ASTE corresponding to the ASN value will  */
/*      be stored into the 16-word area pointed to by aste, and the  */
/*      return value is zero.  Either 4 or 16 words will be stored   */
/*      depending on the value of the ASF control bit (CR0 bit 15).  */
/*      The real address of the ASTE will be stored into the word    */
/*      pointed to by asteo.                                         */
/*                                                                   */
/*      If unsuccessful, the return value is a non-zero exception    */
/*      code indicating AFX-translation or ASX-translation error     */
/*      (this is to allow the LASP instruction to handle these       */
/*      exceptions by setting the condition code).                   */
/*                                                                   */
/*      A program check may be generated for addressing and ASN      */
/*      translation specification exceptions, in which case the      */
/*      function does not return.                                    */
/*-------------------------------------------------------------------*/
U16 ARCH_DEP( translate_asn )( U16 asn, REGS* regs, U32* asteo, U32 aste[] )
{
U32     afte_addr;                      /* Address of AFTE           */
U32     afte;                           /* ASN first table entry     */
U32     aste_addr;                      /* Address of ASTE           */
BYTE   *aste_main;                      /* ASTE mainstor address     */
int     code;                           /* Exception code            */
int     numwords;                       /* ASTE size (4 or 16 words) */
int     i;                              /* Array subscript           */

    /* [3.9.3.1] Use the AFX to obtain the real address of the AFTE */
    afte_addr = (regs->CR(14) & CR14_AFTO) << 12;
    afte_addr += (asn & ASN_AFX) >> 4;

    /* Addressing exception if AFTE is outside main storage */
    if (afte_addr > regs->mainlim)
        goto asn_addr_excp;

    /* Load the AFTE from main storage. All four bytes must be
       fetched concurrently as observed by other CPUs */
    afte_addr = APPLY_PREFIXING (afte_addr, regs->PX);
    afte = ARCH_DEP(fetch_fullword_absolute) (afte_addr, regs);

    /* AFX translation exception if AFTE invalid bit is set */
    if (afte & AFTE_INVALID)
        goto asn_afx_tran_excp;

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* ASN translation specification exception if reserved bits set */
    if (!ASF_ENABLED(regs)) {
        if (afte & AFTE_RESV_0)
              goto asn_asn_tran_spec_excp;
    } else {
        if (afte & AFTE_RESV_1)
              goto asn_asn_tran_spec_excp;
    }
#endif

    /* [3.9.3.2] Use AFTE and ASX to obtain real address of ASTE */
    if (!ASF_ENABLED(regs)) {
        aste_addr = afte & AFTE_ASTO_0;
        aste_addr += (asn & ASN_ASX) << 4;
        numwords = 4;
    } else {
        aste_addr = afte & AFTE_ASTO_1;
        aste_addr += (asn & ASN_ASX) << 6;
        numwords = 16;
    }

    /* Ignore carry into bit position 0 of ASTO */
    aste_addr &= 0x7FFFFFFF;

    /* Addressing exception if ASTE is outside main storage */
    if (aste_addr > regs->mainlim)
        goto asn_addr_excp;

    /* Return the real address of the ASTE */
    *asteo = aste_addr;

    /* Fetch the 16- or 64-byte ASN second table entry from real
       storage.  Each fullword of the ASTE must be fetched
       concurrently as observed by other CPUs */
    aste_addr = APPLY_PREFIXING (aste_addr, regs->PX);
    aste_main = FETCH_MAIN_ABSOLUTE(aste_addr, regs, numwords * 4);
    for (i = 0; i < numwords; i++)
    {
        aste[i] = fetch_fw(aste_main);
        aste_main += 4;
    }
    /* Clear remaining words if fewer than 16 words were loaded */
    while (i < 16) aste[i++] = 0;


    /* Check the ASX invalid bit in the ASTE */
    if (aste[0] & ASTE0_INVALID)
        goto asn_asx_tran_excp;

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Check the reserved bits in first two words of ASTE */
    if ((aste[0] & ASTE0_RESV) || (aste[1] & ASTE1_RESV)
        || ((aste[0] & ASTE0_BASE)
#ifdef FEATURE_SUBSPACE_GROUP
            && !ASF_ENABLED(regs)
#endif
            ))
        goto asn_asn_tran_spec_excp;
#endif

    return 0;

/* Conditions which always cause program check */
asn_addr_excp:
    code = PGM_ADDRESSING_EXCEPTION;
    goto asn_prog_check;

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
asn_asn_tran_spec_excp:
    code = PGM_ASN_TRANSLATION_SPECIFICATION_EXCEPTION;
    goto asn_prog_check;
#endif

asn_prog_check:
    regs->program_interrupt (regs, code);

/* Conditions which the caller may or may not program check */
asn_afx_tran_excp:
    regs->TEA = asn;
    code = PGM_AFX_TRANSLATION_EXCEPTION;
    return code;

asn_asx_tran_excp:
    regs->TEA = asn;
    code = PGM_ASX_TRANSLATION_EXCEPTION;
    return code;

} /* end function translate_asn */
#endif /* defined( FEATURE_DUAL_ADDRESS_SPACE ) */


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* Translate an ALET to produce the corresponding ASTE               */
/*                                                                   */
/* This routine performs both ordinary ART (as used by DAT when      */
/* operating in access register mode, and by the TAR instruction),   */
/* and special ART (as used by the BSG instruction).  The caller     */
/* is assumed to have already eliminated the special cases of ALET   */
/* values 0 and 1 (which have different meanings depending on        */
/* whether the caller is DAT, TAR, or BSG).                          */
/*                                                                   */
/* Input:                                                            */
/*      alet    ALET value                                           */
/*      eax     The authorization index (normally obtained from      */
/*              CR8; obtained from R2 for TAR; not used for BSG)     */
/*      acctype Type of access requested: READ, WRITE, instfetch,    */
/*              TAR, LRA, TPROT, or BSG                              */
/*      regs    Pointer to the CPU register context                  */
/*      asteo   Pointer to word to receive ASTE origin address       */
/*      aste    Pointer to 16-word area to receive a copy of the     */
/*              ASN second table entry associated with the ALET      */
/*                                                                   */
/* Output:                                                           */
/*      If successful, the ASTE is copied into the 16-word area,     */
/*      the real address of the ASTE is stored into the word pointed */
/*      word pointed to by asteop, and the return value is zero;     */
/*      regs->dat.protect is set to 2 if the fetch-only bit          */
/*      in the ALE is set, otherwise it is set to zero.              */
/*                                                                   */
/*      If unsuccessful, the return value is a non-zero exception    */
/*      code in the range X'0028' through X'002D' (this is to allow  */
/*      the TAR, LRA, and TPROT instructions to handle these         */
/*      exceptions by setting the condition code).                   */
/*      regs->dat.xcode is also set to the exception code.           */
/*                                                                   */
/*      A program check may be generated for addressing and ASN      */
/*      translation specification exceptions, in which case the      */
/*      function does not return.                                    */
/*-------------------------------------------------------------------*/
U16 ARCH_DEP( translate_alet )( U32 alet, U16 eax, int acctype,
                                REGS* regs, U32* asteo, U32 aste[] )
{
U32     cb;                             /* DUCT or PASTE address     */
U32     ald;                            /* Access-list designation   */
U32     alo;                            /* Access-list origin        */
U32     all;                            /* Access-list length        */
U32     ale[4];                         /* Access-list entry         */
U32     aste_addr;                      /* Real address of ASTE      */
U32     abs;                            /* Absolute address          */
BYTE   *mn;                             /* Mainstor address          */
int     i;                              /* Array subscript           */

    regs->dat.protect = 0;

    /* [5.8.4.3] Check the reserved bits in the ALET */
    if ( alet & ALET_RESV )
        goto alet_spec_excp;

    /* [5.8.4.4] Obtain the effective access-list designation */

    /* Obtain the real address of the control block containing
       the effective access-list designation.  This is either
       the Primary ASTE or the DUCT */
    cb = (alet & ALET_PRI_LIST) ?
            regs->CR(5) & CR5_PASTEO :
            regs->CR(2) & CR2_DUCTO;

    /* Addressing exception if outside main storage */
    if (cb > regs->mainlim)
        goto alet_addr_excp;

    /* Load the effective access-list designation (ALD) from
       offset 16 in the control block.  All four bytes must be
       fetched concurrently as observed by other CPUs.  Note
       that the DUCT and the PASTE cannot cross a page boundary */
    cb = APPLY_PREFIXING (cb, regs->PX);
    ald = ARCH_DEP(fetch_fullword_absolute) (cb+16, regs);

    /* [5.8.4.5] Access-list lookup */

    /* Isolate the access-list origin and access-list length */
    alo = ald & ALD_ALO;
    all = ald & ALD_ALL;

    /* Check that the ALEN does not exceed the ALL */
    if (((alet & ALET_ALEN) >> ALD_ALL_SHIFT) > all)
        goto alen_tran_excp;

    /* Add the ALEN x 16 to the access list origin */
    alo += (alet & ALET_ALEN) << 4;

    /* Addressing exception if outside main storage */
    if (alo > regs->mainlim)
        goto alet_addr_excp;

    /* Fetch the 16-byte access list entry from absolute storage.
       Each fullword of the ALE must be fetched concurrently as
       observed by other CPUs */
    alo = APPLY_PREFIXING (alo, regs->PX);
    mn = FETCH_MAIN_ABSOLUTE(alo, regs, 16);
    for (i = 0; i < 4; i++)
    {
        ale[i] = fetch_fw (mn);
        mn += 4;
    }

    /* Check the ALEN invalid bit in the ALE */
    if (ale[0] & ALE0_INVALID)
        goto alen_tran_excp;

    /* For ordinary ART (but not for special ART),
       compare the ALE sequence number with the ALET */
    if (!(acctype & ACC_SPECIAL_ART)
        && (ale[0] & ALE0_ALESN) != (alet & ALET_ALESN))
        goto ale_seq_excp;

    /* [5.8.4.6] Locate the ASN-second-table entry */
    aste_addr = ale[2] & ALE2_ASTE;

    /* Addressing exception if ASTE is outside main storage */
    abs = APPLY_PREFIXING (aste_addr, regs->PX);
    if (abs > regs->mainlim)
        goto alet_addr_excp;
    mn = FETCH_MAIN_ABSOLUTE(abs, regs, 64);

    /* Fetch the 64-byte ASN second table entry from real storage.
       Each fullword of the ASTE must be fetched concurrently as
       observed by other CPUs.  ASTE cannot cross a page boundary */
    for (i = 0; i < 16; i++)
    {
        aste[i] = fetch_fw(mn);
        mn += 4;
    }

    /* Check the ASX invalid bit in the ASTE */
    if (aste[0] & ASTE0_INVALID)
        goto aste_vald_excp;

    /* Compare the ASTE sequence number with the ALE */
    if ((aste[5] & ASTE5_ASTESN) != (ale[3] & ALE3_ASTESN))
        goto aste_seq_excp;

    /* [5.8.4.7] For ordinary ART (but not for special ART),
       authorize the use of the access-list entry */
    if (!(acctype & ACC_SPECIAL_ART))
    {
        /* If ALE private bit is zero, or the ALE AX equals the
           EAX, then authorization succeeds.  Otherwise perform
           the extended authorization process. */
        if ((ale[0] & ALE0_PRIVATE)
                && (ale[0] & ALE0_ALEAX) != eax)
        {
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            /* Check the reserved bits in first two words of ASTE */
            if ((aste[0] & ASTE0_RESV) || (aste[1] & ASTE1_RESV)
                || ((aste[0] & ASTE0_BASE)
#ifdef FEATURE_SUBSPACE_GROUP
                        && !ASF_ENABLED(regs)
#endif
                   ))
                goto alet_asn_tran_spec_excp;
#endif

            /* Perform extended authorization */
            if (ARCH_DEP(authorize_asn)(eax, aste, ATE_SECONDARY, regs) != 0)
                goto ext_auth_excp;
        }

    } /* end if(!ACC_SPECIAL_ART) */

    /* [5.8.4.8] Check for access-list controlled protection */
    if (ale[0] & ALE0_FETCHONLY)
    {
        if (acctype & (ACC_WRITE|ACC_CHECK))
            regs->dat.protect = 2;
    }

    /* Return the ASTE origin address */
    *asteo = aste_addr;
    return 0;

/* Conditions which always cause program check, except
   when performing translation for the control panel */
alet_addr_excp:
    regs->dat.xcode = PGM_ADDRESSING_EXCEPTION;
    goto alet_prog_check;

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
alet_asn_tran_spec_excp:
    regs->dat.xcode = PGM_ASN_TRANSLATION_SPECIFICATION_EXCEPTION;
    goto alet_prog_check;
#endif

alet_prog_check:
    regs->program_interrupt (regs, regs->dat.xcode);

/* Conditions which the caller may or may not program check */
alet_spec_excp:
    regs->dat.xcode = PGM_ALET_SPECIFICATION_EXCEPTION;
    return regs->dat.xcode;

alen_tran_excp:
    regs->dat.xcode = PGM_ALEN_TRANSLATION_EXCEPTION;
    return regs->dat.xcode;

ale_seq_excp:
    regs->dat.xcode = PGM_ALE_SEQUENCE_EXCEPTION;
    return regs->dat.xcode;

aste_vald_excp:
    regs->dat.xcode = PGM_ASTE_VALIDITY_EXCEPTION;
    return regs->dat.xcode;

aste_seq_excp:
    regs->dat.xcode = PGM_ASTE_SEQUENCE_EXCEPTION;
    return regs->dat.xcode;

ext_auth_excp:
    regs->dat.xcode = PGM_EXTENDED_AUTHORITY_EXCEPTION;
    return regs->dat.xcode;

} /* end function translate_alet */
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


/*-------------------------------------------------------------------*/
/* Determine effective ASCE or STD                                   */
/*                                                                   */
/* This routine returns either an address-space control element      */
/* (for ESAME) or a segment table descriptor (for S/370 and ESA/390) */
/* loaded from control register 1, 7, or 13, or computed from the    */
/* contents of an address register, together with an indication of   */
/* the addressing mode (home, primary, secondary, or AR mode)        */
/* which was used to determine the source of the ASCE or STD.        */
/*                                                                   */
/* Input:                                                            */
/*      arn     Access register number (0-15) to be used if the      */
/*              address-space control (PSW bits 16-17) indicates     */
/*              that AR-mode is the current translation mode.        */
/*              An access register number ORed with the special      */
/*              value USE_ARMODE forces this routine to use AR-mode  */
/*              address translation regardless of the PSW address-   */
/*              space control setting.                               */
/*              Access register 0 is treated as if it contained 0    */
/*              and its actual contents are not examined.            */
/*              Alternatively the arn parameter may contain one      */
/*              of these special values (defined in hconsts.h):      */
/*              USE_PRIMARY_SPACE, USE_SECONDARY_SPACE,              */
/*              USE_HOME_SPACE, USE_REAL_ADDR to force the use of    */
/*              a specific translation mode instead of the mode      */
/*              indicated by the address-space control in the PSW.   */
/*      regs    Pointer to the CPU register context                  */
/*      acctype Type of access requested: READ, WRITE, INSTFETCH,    */
/*              LRA, IVSK, TPROT, STACK, PTE, LPTEA                  */
/*                                                                   */
/* Output:                                                           */
/*      regs->dat.asd = the selected ASCE or STD                     */
/*      regs->dat.stid = TEA_ST_PRIMARY, TEA_ST_SECNDRY,             */
/*              TEA_ST_HOME, or TEA_ST_ARMODE indicates which        */
/*              address space was used to select the ASCE or STD.    */
/*      regs->dat.protect = 2 if in AR mode and access-list          */
/*              controlled protection is indicated by the ALE        */
/*              fetch-only bit; otherwise it remains unchanged.      */
/*                                                                   */
/*      If an ALET translation error occurs, the return value        */
/*      is the exception code; otherwise the return value is zero,   */
/*      regs->dat.asd field contains the ASCE or STD, and            */
/*      regs->dat.stid is set to TEA_ST_PRIMARY, TEA_ST_SECNDRY,     */
/*      TEA_ST_HOME, or TEA_ST_ARMODE.                               */
/*-------------------------------------------------------------------*/
U16 ARCH_DEP( load_address_space_designator )( int arn, REGS* regs, int acctype )
{
#if defined( FEATURE_ACCESS_REGISTERS )
U32     alet;                           /* Access list entry token   */
U32     asteo;                          /* Real address of ASTE      */
U32     aste[16];                       /* ASN second table entry    */
U16     eax;                            /* Authorization index       */
#else
    UNREFERENCED( acctype );
#endif

    switch(arn) {

    case USE_INST_SPACE:
        switch(regs->AEA_AR(USE_INST_SPACE)) {

        case 1:
            regs->dat.stid = TEA_ST_PRIMARY;
            break;
#if defined( FEATURE_LINKAGE_STACK )
        case 13:
            regs->dat.stid = TEA_ST_HOME;
            break;
#endif
        default:
            regs->dat.stid = 0;
        } /* end switch(regs->AEA_AR(USE_INST_SPACE)) */

        regs->dat.asd = regs->CR(regs->AEA_AR(USE_INST_SPACE));
        break;

    case USE_PRIMARY_SPACE:
        regs->dat.stid = TEA_ST_PRIMARY;
        regs->dat.asd = regs->CR(1);
        break;

    case USE_SECONDARY_SPACE:
        regs->dat.stid = TEA_ST_SECNDRY;
        regs->dat.asd = regs->CR(7);
        break;

    case USE_HOME_SPACE:
        regs->dat.stid = TEA_ST_HOME;
        regs->dat.asd = regs->CR(13);
        break;

    case USE_REAL_ADDR:
        regs->dat.stid = 0;
        regs->dat.asd = TLB_REAL_ASD;
        break;

    default:

#if defined( FEATURE_ACCESS_REGISTERS )
        if (ACCESS_REGISTER_MODE(&regs->psw)
         || (SIE_ACTIVE(regs) && MULTIPLE_CONTROLLED_DATA_SPACE(GUESTREGS))
         || (arn >= USE_ARMODE)
           )
        {
            /* Remove flags giving access register number 0-15 */
            arn &= 0xF;

            /* [5.8.4.1] Select the access-list-entry token */
            alet = (arn == 0) ? 0 :
                   /* Guest ALET if XC guest in AR mode */
                   (SIE_ACTIVE(regs) && MULTIPLE_CONTROLLED_DATA_SPACE(GUESTREGS))
                   ? GUESTREGS->AR(arn) :
                   /* If SIE host but not XC guest in AR mode then alet is 0 */
                   SIE_ACTIVE(regs) ? 0 :
                   /* Otherwise alet is in the access register */
                   regs->AR(arn);

            /* Use the ALET to determine the segment table origin */
            switch (alet) {

            case ALET_PRIMARY:
                /* [5.8.4.2] Obtain primary segment table designation */
                regs->dat.stid = TEA_ST_PRIMARY;
                regs->dat.asd = regs->CR(1);
                break;

            case ALET_SECONDARY:
                /* [5.8.4.2] Obtain secondary segment table designation */
                regs->dat.stid = TEA_ST_SECNDRY;
                regs->dat.asd = regs->CR(7);
                break;

            default:
                /* ALB Lookup */
                if(regs->AEA_AR(arn) >= CR_ALB_OFFSET)
                {
                    regs->dat.asd = regs->CR(regs->AEA_AR(arn));
                    regs->dat.protect = regs->aea_aleprot[arn];
                    regs->dat.stid = TEA_ST_ARMODE;
                }
                else
                {
                    /* Extract the extended AX from CR8 bits 0-15 (32-47) */
                    eax = regs->CR_LHH(8);

                    /* [5.8.4.3] Perform ALET translation to obtain ASTE */
                    if (ARCH_DEP(translate_alet) (alet, eax, acctype,
                                                  regs, &asteo, aste))
                        /* Exit if ALET translation error */
                        return regs->dat.xcode;

                    /* [5.8.4.9] Obtain the STD or ASCE from the ASTE */
                    regs->dat.asd = ASTE_AS_DESIGNATOR(aste);
                    regs->dat.stid = TEA_ST_ARMODE;

                    if (regs->dat.protect & 2)
                    {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                       regs->dat.asd ^= ASCE_RESV;
                       regs->dat.asd |= ASCE_P;
#else
                       regs->dat.asd ^= STD_RESV;
                       regs->dat.asd |= STD_PRIVATE;
#endif
                    }

                    /* Update ALB */
                    regs->CR(CR_ALB_OFFSET + arn) = regs->dat.asd;
                    regs->AEA_AR(arn) = CR_ALB_OFFSET + arn;
                    regs->AEA_COMMON(CR_ALB_OFFSET + arn) = (regs->dat.asd & ASD_PRIVATE) == 0;
                    regs->aea_aleprot[arn] = regs->dat.protect & 2;
                }

            } /* end switch(alet) */

            break;

        } /* end if(ACCESS_REGISTER_MODE) */
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */

#if defined( FEATURE_DUAL_ADDRESS_SPACE )
        if (SECONDARY_SPACE_MODE(&regs->psw))
        {
            regs->dat.stid = TEA_ST_SECNDRY;
            regs->dat.asd = regs->CR(7);
            break;
        }
#endif

#if defined( FEATURE_LINKAGE_STACK )
        if (HOME_SPACE_MODE(&regs->psw))
        {
            regs->dat.stid = TEA_ST_HOME;
            regs->dat.asd = regs->CR(13);
            break;
        }
#endif

        /* Primary space mode */
        regs->dat.stid = TEA_ST_PRIMARY;
        regs->dat.asd = regs->CR(1);
        break;

    } /* switch(arn) */

    return 0;

} /* end function load_address_space_designator */


/*-------------------------------------------------------------------*/
/*                        translate_addr                             */
/*           PRIMARY DYNAMIC ADDRESS TRANSLATION LOGIC               */
/*-------------------------------------------------------------------*/
/* Translate a virtual address to a real address                     */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/*      vaddr   virtual address to be translated                     */
/*      arn     Access register number or special value (see         */
/*              load_address_space_designator function for a         */
/*              complete description of this parameter)              */
/*      regs    Pointer to the CPU register context                  */
/*      acctype Type of access requested: READ, WRITE, INSTFETCH,    */
/*              LRA, IVSK, TPROT, STACK, PTE, LPTEA or ACCTYPE_HW.   */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/*      The return value is set to facilitate the setting of the     */
/*      condition code by the LRA instruction:                       */
/*                                                                   */
/*      0 = Translation successful; real address field contains      */
/*          the real address corresponding to the virtual address    */
/*          supplied by the caller; exception code set to zero.      */
/*      1 = Segment table entry invalid; real address field          */
/*          contains real address of segment table entry;            */
/*          exception code is set to X'0010'.                        */
/*      2 = Page table entry invalid; real address field contains    */
/*          real address of page table entry; exception code         */
/*          is set to X'0011'.                                       */
/*      3 = Segment or page table length exceeded; real address      */
/*          field contains the real address of the entry that        */
/*          would have been fetched if length violation had not      */
/*          occurred; exception code is set to X'0010' or X'0011'.   */
/*      4 = ALET translation error: real address field is not        */
/*          set; exception code is set to X'0028' through X'002D'.   */
/*          ASCE-type or region-translation error: real address      */
/*          is not set; exception code is X'0038' through X'003B'.   */
/*          The LRA instruction converts this to condition code 3.   */
/*      5 = For ACCTYPE_EMC (Enhanced MC access only):               */
/*          A translation specification exception occured            */
/*                                                                   */
/*                                                                   */
/*      For ACCTYPE_LPTEA, the return value is set to facilitate     */
/*      setting the condition code by the LPTEA instruction:         */
/*                                                                   */
/*      0 = Page table entry found, and page protection bit in the   */
/*          segment table entry is zero; the real address field      */
/*          contains the real address of the page table entry;       */
/*          exception code is set to zero.                           */
/*      1 = Page table entry found, and page protection bit in the   */
/*          segment table entry is one; the real address field       */
/*          contains the real address of the page table entry;       */
/*          exception code is set to zero.                           */
/*      2 = Region table or segment table entry invalid bit is set;  */
/*          the real address field contains the real address of the  */
/*          region table entry or segment table entry, with the      */
/*          entry type in the low-order two bits of the address.     */
/*      3 = Region table or segment table length exceeded; real      */
/*          address field is not set; exception code is set to       */
/*          X'0010' or X'0039' through X'003B'.                      */
/*          ALET translation error: real address field is not        */
/*          set; exception code is set to X'0028' through X'002D'.   */
/*          ASCE-type error: real address is not set; exception      */
/*          exception code is X'0038'.                               */
/*                                                                   */
/*                                                                   */
/*      regs->dat.raddr is set to the real address if translation    */
/*      was successful; otherwise it may contain the address of      */
/*      a page or segment table entry as described above.            */
/*      For ACCTYPE_PTE or ACCTYPE_LPTEA it contains the address of  */
/*      the page table entry if translation was successful.          */
/*                                                                   */
/*      regs->dat.xcode is set to the exception code if translation  */
/*      was unsuccessful; otherwise it is set to zero.               */
/*                                                                   */
/*      regs->dat.private is set to 1 if translation was             */
/*      successful and the STD indicates a private address space;    */
/*      otherwise it is set to zero.                                 */
/*                                                                   */
/*      regs->dat.protect is set to 1 if translation was             */
/*      successful and page protection, segment protection, or       */
/*      segment controlled page protection is in effect; it is       */
/*      set to 2 if translation was successful and ALE controlled    */
/*      protection (but not page protection) is in effect;           */
/*      otherwise it is set to zero.                                 */
/*                                                                   */
/*      regs->dat.stid is set to one of the following values:        */
/*      TEA_ST_PRIMARY, TEA_ST_SECNDRY, TEA_ST_HOME, TEA_ST_ARMODE   */
/*      if the translation was successful.  This indication is used  */
/*      to set bits 30-31 of the translation exception address in    */
/*      the event of a protection exception when the suppression on  */
/*      protection facility is used.                                 */
/*                                                                   */
/*      A program check may be generated for addressing and          */
/*      translation specification exceptions, in which case the      */
/*      function does not return. Otherwise the return is one of     */
/*      the return values mentioned above.                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( translate_addr )( VADR vaddr, int arn, REGS* regs, int acctype )
{
RADR    sto = 0;                        /* Segment table origin      */
RADR    pto = 0;                        /* Page table origin         */
int     cc;                             /* Condition code            */
int     tlbix = TLBIX(vaddr);           /* TLB entry index           */

#if !defined( FEATURE_S390_DAT ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
/*-----------------------------------*/
/* S/370 Dynamic Address Translation */
/*-----------------------------------*/
U32     stl;                            /* Segment table length      */
RADR    ste;                            /* Segment table entry       */
U16     pte;                            /* Page table entry          */
U32     ptl;                            /* Page table length         */

    regs->dat.pvtaddr = regs->dat.protect = 0;

    /* Load the effective segment table descriptor */
    if (ARCH_DEP(load_address_space_designator) (arn, regs, acctype))
        goto tran_alet_excp;

    /* Check the translation format bits in CR0 */
    if ((((regs->CR(0) & CR0_PAGE_SIZE) != CR0_PAGE_SZ_2K) &&
       ((regs->CR(0) & CR0_PAGE_SIZE) != CR0_PAGE_SZ_4K)) ||
       (((regs->CR(0) & CR0_SEG_SIZE) != CR0_SEG_SZ_64K) &&
       ((regs->CR(0) & CR0_SEG_SIZE) != CR0_SEG_SZ_1M)))
       goto tran_spec_excp;

    /* Look up the address in the TLB */
    if (   ((vaddr & TLBID_PAGEMASK) | regs->tlbID) == regs->tlb.TLB_VADDR(tlbix)
        && (regs->tlb.common[tlbix] || regs->dat.asd == regs->tlb.TLB_ASD(tlbix))
        && !(regs->tlb.common[tlbix] && regs->dat.pvtaddr)
        && !(acctype & ACC_NOTLB) )
    {
        pte = regs->tlb.TLB_PTE(tlbix);

#if defined( FEATURE_SEGMENT_PROTECTION )
        /* Set the protection indicator if segment is protected */
        if (regs->tlb.protect[tlbix])
            regs->dat.protect = regs->tlb.protect[tlbix];
#endif
    }
    else
    {
        /* S/370 segment table lookup */

        /* Calculate the real address of the segment table entry */
        sto = regs->dat.asd & STD_370_STO;
        stl = regs->dat.asd & STD_370_STL;
        sto += ((regs->CR(0) & CR0_SEG_SIZE) == CR0_SEG_SZ_1M) ?
            ((vaddr & 0x00F00000) >> 18) :
            ((vaddr & 0x00FF0000) >> 14);

        /* Check that virtual address is within the segment table */
        if (((regs->CR(0) & CR0_SEG_SIZE) == CR0_SEG_SZ_64K) &&
            ((vaddr << 4) & STD_370_STL) > stl)
            goto seg_tran_length;

        /* Generate addressing exception if outside real storage */
        if (sto > regs->mainlim)
            goto address_excp;

        /* Fetch segment table entry from real storage.  All bytes
           must be fetched concurrently as observed by other CPUs */
        sto = APPLY_PREFIXING (sto, regs->PX);
        ste = ARCH_DEP(fetch_fullword_absolute) (sto, regs);

        /* Generate segment translation exception if segment invalid */
        if (ste & SEGTAB_370_INVL)
            goto seg_tran_invalid;

        /* Check that all the reserved bits in the STE are zero */
        if (ste & SEGTAB_370_RSV)
            goto tran_spec_excp;

        /* Isolate page table origin and length */
        pto = ste & SEGTAB_370_PTO;
        ptl = ste & SEGTAB_370_PTL;

        /* S/370 page table lookup */

        /* Calculate the real address of the page table entry */
        pto += ((regs->CR(0) & CR0_SEG_SIZE) == CR0_SEG_SZ_1M) ?
            (((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K) ?
            ((vaddr & 0x000FF000) >> 11) :
            ((vaddr & 0x000FF800) >> 10)) :
            (((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K) ?
            ((vaddr & 0x0000F000) >> 11) :
            ((vaddr & 0x0000F800) >> 10));

        /* Generate addressing exception if outside real storage */
        if (pto > regs->mainlim)
            goto address_excp;

        /* Check that the virtual address is within the page table */
        if ((((regs->CR(0) & CR0_SEG_SIZE) == CR0_SEG_SZ_1M) &&
            (((vaddr & 0x000F0000) >> 16) > ptl)) ||
            (((regs->CR(0) & CR0_SEG_SIZE) == CR0_SEG_SZ_64K) &&
            (((vaddr & 0x0000F000) >> 12) > ptl)))
            goto page_tran_length;

        /* Fetch the page table entry from real storage.  All bytes
           must be fetched concurrently as observed by other CPUs */
        pto = APPLY_PREFIXING (pto, regs->PX);
        pte = ARCH_DEP(fetch_halfword_absolute) (pto, regs);

        /* Generate page translation exception if page invalid */
        if ((((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K) &&
            (pte & PAGETAB_INV_4K)) ||
            (((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_2K) &&
            (pte & PAGETAB_INV_2K)))
            goto page_tran_invalid;

        /* Check that all the reserved bits in the PTE are zero */
        if (((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_2K) &&
            (pte & PAGETAB_RSV_2K))
            goto tran_spec_excp;

#if defined( FEATURE_SEGMENT_PROTECTION )
        /* Set the protection indicator if segment is protected */
        if (ste & SEGTAB_370_PROT)
            regs->dat.protect |= 1;
#endif

        /* Place the translated address in the TLB */
        if (!(acctype & ACC_NOTLB))
        {
            regs->tlb.TLB_ASD(tlbix)   = regs->dat.asd;
            regs->tlb.TLB_VADDR(tlbix) = (vaddr & TLBID_PAGEMASK) | regs->tlbID;
            regs->tlb.TLB_PTE(tlbix)   = pte;
            regs->tlb.common[tlbix]    = (ste & SEGTAB_370_CMN) ? 1 : 0;
            regs->tlb.protect[tlbix]   = regs->dat.protect;
            regs->tlb.acc[tlbix]       = 0;
            regs->tlb.main[tlbix]      = NULL;

            /* Set adjacent TLB entry if 4K page sizes */
            if ((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K)
            {
                regs->tlb.TLB_ASD(tlbix^1)   = regs->tlb.TLB_ASD(tlbix);
                regs->tlb.TLB_VADDR(tlbix^1) = (vaddr & TLBID_PAGEMASK) | regs->tlbID;
                regs->tlb.TLB_PTE(tlbix^1)   = regs->tlb.TLB_PTE(tlbix);
                regs->tlb.common[tlbix^1]    = regs->tlb.common[tlbix];
                regs->tlb.protect[tlbix^1]   = regs->tlb.protect[tlbix];
                regs->tlb.acc[tlbix^1]       = 0;
                regs->tlb.main[tlbix^1]      = NULL;
            }
        }
    } /* end if(!TLB) */

    /* Combine the page frame real address with the byte
       index of the virtual address to form the real address */
    regs->dat.raddr = ((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K) ?
#if defined( FEATURE_S370E_EXTENDED_ADDRESSING )
        (((U32)pte & PAGETAB_EA_4K) << 23) |
#endif
        (((U32)pte & PAGETAB_PFRA_4K) << 8) | (vaddr & 0xFFF) :
        (((U32)pte & PAGETAB_PFRA_2K) << 8) | (vaddr & 0x7FF);

    regs->dat.rpfra = regs->dat.raddr & PAGEFRAME_PAGEMASK;
#endif /* !defined( FEATURE_S390_DAT ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

#if defined( FEATURE_S390_DAT )
/*-----------------------------------*/
/* S/390 Dynamic Address Translation */
/*-----------------------------------*/
U32     stl;                            /* Segment table length      */
RADR    ste;                            /* Segment table entry       */
RADR    pte;                            /* Page table entry          */
U32     ptl;                            /* Page table length         */

    regs->dat.pvtaddr = regs->dat.protect = 0;

    /* [3.11.3.1] Load the effective segment table descriptor */
    if (ARCH_DEP(load_address_space_designator) (arn, regs, acctype))
        goto tran_alet_excp;

    /* [3.11.3.2] Check the translation format bits in CR0 */
    if ((regs->CR(0) & CR0_TRAN_FMT) != CR0_TRAN_ESA390)
        goto tran_spec_excp;

    /* Extract the private space bit from segment table descriptor */
    regs->dat.pvtaddr = ((regs->dat.asd & STD_PRIVATE) != 0);

    /* [3.11.4] Look up the address in the TLB */
    if (   ((vaddr & TLBID_PAGEMASK) | regs->tlbID) == regs->tlb.TLB_VADDR(tlbix)
        && (regs->tlb.common[tlbix] || regs->dat.asd == regs->tlb.TLB_ASD(tlbix))
        && !(regs->tlb.common[tlbix] && regs->dat.pvtaddr)
        && !(acctype & ACC_NOTLB) )
    {
        pte = regs->tlb.TLB_PTE(tlbix);
        if (regs->tlb.protect[tlbix])
            regs->dat.protect = regs->tlb.protect[tlbix];
    }
    else
    {
        /* [3.11.3.3] Segment table lookup */

        /* Calculate the real address of the segment table entry */
        sto = regs->dat.asd & STD_STO;
        stl = regs->dat.asd & STD_STL;
        sto += (vaddr & 0x7FF00000) >> 18;

        /* Check that virtual address is within the segment table */
        if ((vaddr >> 24) > stl)
            goto seg_tran_length;

        /* Generate addressing exception if outside real storage */
        if (sto > regs->mainlim)
            goto address_excp;

        /* Fetch segment table entry from real storage.  All bytes
           must be fetched concurrently as observed by other CPUs */
        sto = APPLY_PREFIXING (sto, regs->PX);
        ste = ARCH_DEP(fetch_fullword_absolute) (sto, regs);

        /* Generate segment translation exception if segment invalid */
        if (ste & SEGTAB_INVALID)
            goto seg_tran_invalid;

        /* Check that all the reserved bits in the STE are zero */
        if (ste & SEGTAB_RESV)
            goto tran_spec_excp;

        /* If the segment table origin register indicates a private
           address space then STE must not indicate a common segment */
        if (regs->dat.pvtaddr && (ste & (SEGTAB_COMMON)))
            goto tran_spec_excp;

        /* Isolate page table origin and length */
        pto = ste & SEGTAB_PTO;
        ptl = ste & SEGTAB_PTL;

        /* [3.11.3.4] Page table lookup */

        /* Calculate the real address of the page table entry */
        pto += (vaddr & 0x000FF000) >> 10;

        /* Check that the virtual address is within the page table */
        if (((vaddr & 0x000FF000) >> 16) > ptl)
            goto page_tran_length;

        /* Generate addressing exception if outside real storage */
        if (pto > regs->mainlim)
            goto address_excp;

        /* Fetch the page table entry from real storage.  All bytes
           must be fetched concurrently as observed by other CPUs */
        pto = APPLY_PREFIXING (pto, regs->PX);
        pte = ARCH_DEP(fetch_fullword_absolute) (pto, regs);

        /* Generate page translation exception if page invalid */
        if (pte & PAGETAB_INVALID)
            goto page_tran_invalid;

        /* Check that all the reserved bits in the PTE are zero */
        if (pte & PAGETAB_RESV)
            goto tran_spec_excp;

        /* Set the protection indicator if page protection is active */
        if (pte & PAGETAB_PROT)
            regs->dat.protect |= 1;

        /* [3.11.4.2] Place the translated address in the TLB */
        if (!(acctype & ACC_NOTLB))
        {
            regs->tlb.TLB_ASD(tlbix)   = regs->dat.asd;
            regs->tlb.TLB_VADDR(tlbix) = (vaddr & TLBID_PAGEMASK) | regs->tlbID;
            regs->tlb.TLB_PTE(tlbix)   = pte;
            regs->tlb.common[tlbix]    = (ste & SEGTAB_COMMON) ? 1 : 0;
            regs->tlb.acc[tlbix]       = 0;
            regs->tlb.protect[tlbix]   = regs->dat.protect;
            regs->tlb.main[tlbix]      = NULL;
        }
    } /* end if(!TLB) */

    if(!(acctype & ACC_PTE))
    {
    /* [3.11.3.5] Combine the page frame real address with the byte
       index of the virtual address to form the real address */
        regs->dat.raddr = (pte & PAGETAB_PFRA) | (vaddr & 0xFFF);
        regs->dat.rpfra = (pte & PAGETAB_PFRA);
    }
    else
    /* In the case of lock page, return the address of the
       pagetable entry */
        regs->dat.raddr = pto;

#endif /* defined( FEATURE_S390_DAT ) */

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
/*-----------------------------------*/
/* ESAME Dynamic Address Translation */
/*-----------------------------------*/
RADR    rte;                            /* Region table entry        */
#define rto     sto                     /* Region/seg table origin   */
RADR    ste = 0;                        /* Segment table entry       */
RADR    pte = 0;                        /* Page table entry          */
BYTE    tt;                             /* Table type                */
BYTE    tl;                             /* Table length              */
BYTE    tf;                             /* Table offset              */
U16     rfx, rsx, rtx;                  /* Region first/second/third
                                           index + 3 low-order zeros */
U16     sx, px;                         /* Segment and page index,
                                           + 3 low-order zero bits   */

    regs->dat.pvtaddr = regs->dat.protect = 0;

    /* Load the address space control element */
    if (ARCH_DEP(load_address_space_designator) (arn, regs, acctype))
        goto tran_alet_excp;

    /* Extract the private space bit from the ASCE */
    regs->dat.pvtaddr = ((regs->dat.asd & (ASCE_P|ASCE_R)) != 0);

//  LOGMSG("asce=%16.16"PRIX64"\n",regs->dat.asd);

    /* [3.11.4] Look up the address in the TLB */
    if (   ((vaddr & TLBID_PAGEMASK) | regs->tlbID) == regs->tlb.TLB_VADDR(tlbix)
        && (regs->tlb.common[tlbix] || regs->dat.asd == regs->tlb.TLB_ASD(tlbix))
        && !(regs->tlb.common[tlbix] && regs->dat.pvtaddr)
        && !(acctype & ACC_NOTLB) )
    {
        pte = regs->tlb.TLB_PTE(tlbix);
        if (regs->tlb.protect[tlbix])
            regs->dat.protect = regs->tlb.protect[tlbix];
    }
    else
    {
        /* If ASCE indicates a real-space then real addr = virtual addr */
        if (regs->dat.asd & ASCE_R)
        {
//      LOGMSG("asce type = real\n");

            /* Translation specification exception if LKPG for a real-space */
            if(acctype & ACC_PTE)
                goto tran_spec_excp;

            /* Special operation exception if LPTEA for a real-space */
            if(acctype & ACC_LPTEA)
                goto spec_oper_excp;

            /* Construct a fake page table entry for real = virtual */
            pte = vaddr & 0xFFFFFFFFFFFFF000ULL;
        }
        else
        {
            /* Extract the table origin, type, and length from the ASCE,
               and set the table offset to zero */
            rto = regs->dat.asd & ASCE_TO;
            tf = 0;
            tt = regs->dat.asd & ASCE_DT;
            tl = regs->dat.asd & ASCE_TL;

            /* Extract the 11-bit region first index, region second index,
               and region third index from the virtual address, and shift
               each index into bits 2-12 of a 16-bit integer, ready for
               addition to the appropriate region table origin */
            rfx = (vaddr >> 50) & 0x3FF8;
            rsx = (vaddr >> 39) & 0x3FF8;
            rtx = (vaddr >> 28) & 0x3FF8;

            /* Extract the 11-bit segment index from the virtual address,
               and shift it into bits 2-12 of a 16-bit integer, ready
               for addition to the segment table origin */
            sx = (vaddr >> 17) & 0x3FF8;

            /* Extract the 8-bit page index from the virtual address,
               and shift it into bits 2-12 of a 16-bit integer, ready
               for addition to the page table origin */
            px = (vaddr >> 9) & 0x07F8;

            /* ASCE-type exception if the virtual address is too large
               for the table type designated by the ASCE */
            if ((rfx != 0 && tt < TT_R1TABL)
                || (rsx != 0 && tt < TT_R2TABL)
                || (rtx != 0 && tt < TT_R3TABL))
                goto asce_type_excp;

            /* Perform region translation */
            switch (tt) {

            /* Perform region-first translation */
            case TT_R1TABL:

                /* Region-first translation exception if table length is
                   less than high-order 2 bits of region-first index */
                if (tl < (rfx >> 12))
                    goto reg_first_excp;

                /* Add the region-first index (with three low-order zeroes)
                   to the region-first table origin, giving the address of
                   the region-first table entry */
                rto += rfx;

                /* Addressing exception if outside main storage */
                if (rto > regs->mainlim)
                    goto address_excp;

                /* Fetch region-first table entry from absolute storage.
                   All bytes must be fetched concurrently as observed by
                   other CPUs */
                rte = ARCH_DEP(fetch_doubleword_absolute) (rto, regs);
//              LOGMSG("r1te:%16.16"PRIX64"=>%16.16"PRIX64"\n",rto,rte);

                /* Region-first translation exception if the bit 58 of
                   the region-first table entry is set (region invalid) */
                if (rte & REGTAB_I)
                    goto reg_first_invalid;

                /* Translation specification exception if bits 60-61 of
                   the region-first table entry do not indicate the
                   correct type of region table */
                if ((rte & REGTAB_TT) != TT_R1TABL)
                    goto tran_spec_excp;

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
                if (FACILITY_ENABLED( 008_EDAT_1, regs )
                 && (regs->CR_L(0) & CR0_ED)
                 && (rte & REGTAB_P))
                    regs->dat.protect |= 1;
#endif
                /* Extract the region-second table origin, offset, and
                   length from the region-first table entry */
                rto = rte & REGTAB_TO;
                tf = (rte & REGTAB_TF) >> 6;
                tl = rte & REGTAB_TL;

                /* Fall through to perform region-second translation */
                /* FALLTHRU */

            /* Perform region-second translation */
            case TT_R2TABL:

                /* Region-second translation exception if table offset is
                   greater than high-order 2 bits of region-second index */
                if (tf > (rsx >> 12))
                    goto reg_second_excp;

                /* Region-second translation exception if table length is
                   less than high-order 2 bits of region-second index */
                if (tl < (rsx >> 12))
                    goto reg_second_excp;

                /* Add the region-second index (with three low-order zeroes)
                   to the region-second table origin, giving the address of
                   the region-second table entry */
                rto += rsx;

                /* Addressing exception if outside main storage */
                if (rto > regs->mainlim)
                    goto address_excp;

                /* Fetch region-second table entry from absolute storage.
                   All bytes must be fetched concurrently as observed by
                   other CPUs */
                rte = ARCH_DEP(fetch_doubleword_absolute) (rto, regs);
//              LOGMSG("r2te:%16.16"PRIX64"=>%16.16"PRIX64"\n",rto,rte);

                /* Region-second translation exception if the bit 58 of
                   the region-second table entry is set (region invalid) */
                if (rte & REGTAB_I)
                    goto reg_second_invalid;

                /* Translation specification exception if bits 60-61 of
                   the region-second table entry do not indicate the
                   correct type of region table */
                if ((rte & REGTAB_TT) != TT_R2TABL)
                    goto tran_spec_excp;

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
                if (FACILITY_ENABLED( 008_EDAT_1, regs )
                 && (regs->CR_L(0) & CR0_ED)
                 && (rte & REGTAB_P))
                    regs->dat.protect |= 1;
#endif
                /* Extract the region-third table origin, offset, and
                   length from the region-second table entry */
                rto = rte & REGTAB_TO;
                tf = (rte & REGTAB_TF) >> 6;
                tl = rte & REGTAB_TL;

                /* Fall through to perform region-third translation */
                /* FALLTHRU */

            /* Perform region-third translation */
            case TT_R3TABL:

                /* Region-third translation exception if table offset is
                   greater than high-order 2 bits of region-third index */
                if (tf > (rtx >> 12))
                    goto reg_third_excp;

                /* Region-third translation exception if table length is
                   less than high-order 2 bits of region-third index */
                if (tl < (rtx >> 12))
                    goto reg_third_excp;

                /* Add the region-third index (with three low-order zeroes)
                   to the region-third table origin, giving the address of
                   the region-third table entry */
                rto += rtx;

                /* Addressing exception if outside main storage */
                if (rto > regs->mainlim)
                    goto address_excp;

                /* Fetch region-third table entry from absolute storage.
                   All bytes must be fetched concurrently as observed by
                   other CPUs */
                rte = ARCH_DEP(fetch_doubleword_absolute) (rto, regs);
//              LOGMSG("r3te:%16.16"PRIX64"=>%16.16"PRIX64"\n",rto,rte);

                /* Region-third translation exception if the bit 58 of
                   the region-third table entry is set (region invalid) */
                if (rte & REGTAB_I)
                    goto reg_third_invalid;

                /* Translation specification exception if bits 60-61 of
                   the region-third table entry do not indicate the
                   correct type of region table */
                if ((rte & REGTAB_TT) != TT_R3TABL)
                    goto tran_spec_excp;

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
                if (FACILITY_ENABLED( 008_EDAT_1, regs )
                 && (regs->CR_L(0) & CR0_ED)
                 && (rte & REGTAB_P))
                    regs->dat.protect |= 1;
#endif
                /* Extract the segment table origin, offset, and
                   length from the region-third table entry */
                sto = rte & REGTAB_TO;
                tf = (rte & REGTAB_TF) >> 6;
                tl = rte & REGTAB_TL;

                /* Fall through to perform segment translation */
            } /* end switch(tt) */

            /* Perform ESAME segment translation */

            /* Add the segment index (with three low-order zeroes)
               to the segment table origin, giving the address of
               the segment table entry */
            sto += sx;

            /* Segment translation exception if table offset is
               greater than high-order 2 bits of segment index */
            if (tf > (sx >> 12))
                goto seg_tran_length;

            /* Segment translation exception if table length is
               less than high-order 2 bits of segment index */
            if (tl < (sx >> 12))
                goto seg_tran_length;

            /* Addressing exception if outside real storage */
            if (sto > regs->mainlim)
                goto address_excp;

            /* Fetch segment table entry from absolute storage.  All bytes
               must be fetched concurrently as observed by other CPUs */
            ste = ARCH_DEP(fetch_doubleword_absolute) (sto, regs);
//          LOGMSG("ste:%16.16"PRIX64"=>%16.16"PRIX64"\n",sto,ste);

            /* Segment translation exception if segment invalid */
            if (ste & ZSEGTAB_I)
                goto seg_tran_invalid;

            /* Translation specification exception if bits 60-61 of
               the segment table entry do not indicate segment table */
            if ((ste & ZSEGTAB_TT) != TT_SEGTAB)
                goto tran_spec_excp;

            /* Translation specification exception if the ASCE
               indicates a private space, and the segment table
               entry indicates a common segment */
            if (regs->dat.pvtaddr && (ste & ZSEGTAB_C))
                goto tran_spec_excp;

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
            if (FACILITY_ENABLED( 008_EDAT_1, regs )
              && (regs->CR_L(0) & CR0_ED)
              && (ste & ZSEGTAB_FC))
            {
                /* Set protection indicator if page protection is indicated */
                if (ste & ZSEGTAB_P)
                    regs->dat.protect |= 1;

                /* For LPTEA instruction, return the address of the STE */
                if (unlikely(acctype & ACC_LPTEA))
                {
                    regs->dat.raddr = sto | (regs->dat.protect ? 0x04 : 0);
//                  LOGMSG("raddr:%16.16"PRIX64" cc=2\n",regs->dat.raddr);
                    regs->dat.xcode = 0;
                    cc = 2;
                    return cc;
                } /* end if(ACCTYPE_LPTEA) */

                /* Combine the page frame real address with the byte index
                   of the virtual address to form the real address */
                regs->dat.raddr = (ste & ZSEGTAB_SFAA) | (vaddr & ~ZSEGTAB_SFAA);
                /* Fake 4K PFRA for TLB purposes */
                regs->dat.rpfra = ((ste & ZSEGTAB_SFAA) | (vaddr & ~ZSEGTAB_SFAA)) & PAGEFRAME_PAGEMASK;

//              LOGMSG("raddr:%16.16"PRIX64" cc=0\n",regs->dat.raddr);

                /* [3.11.4.2] Place the translated address in the TLB */
                if (!(acctype & ACC_NOTLB))
                {
                    regs->tlb.TLB_ASD(tlbix)   = regs->dat.asd;
                    regs->tlb.TLB_VADDR(tlbix) = (vaddr & TLBID_PAGEMASK) | regs->tlbID;
                    /* Fake 4K PTE for TLB purposes */
                    regs->tlb.TLB_PTE(tlbix)   = ((ste & ZSEGTAB_SFAA) | (vaddr & ~ZSEGTAB_SFAA)) & PAGEFRAME_PAGEMASK;
                    regs->tlb.common[tlbix]    = (ste & SEGTAB_COMMON) ? 1 : 0;
                    regs->tlb.protect[tlbix]   = regs->dat.protect;
                    regs->tlb.acc[tlbix]       = 0;
                    regs->tlb.main[tlbix]      = NULL;
                }

                /* Clear exception code and return with zero return code */
                regs->dat.xcode = 0;
                return 0;

            }
#endif /* defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 ) */

            /* Extract the page table origin from segment table entry */
            pto = ste & ZSEGTAB_PTO;

            /* Perform ESAME page translation */

            /* Add the page index (with three low-order zeroes) to the
               page table origin, giving address of page table entry */
            pto += px;

            /* For LPTEA instruction, return the address of the PTE */
            if (acctype & ACC_LPTEA)
            {
                regs->dat.raddr = pto;
                regs->dat.xcode = 0;
                cc = (ste & ZSEGTAB_P) ? 1 : 0;
#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
                if (FACILITY_ENABLED( 008_EDAT_1, regs )
                  && (regs->CR_L(0) & CR0_ED)
                  && regs->dat.protect)
                    cc = 1;
#endif
                return cc;
            } /* end if(ACCTYPE_LPTEA) */

            /* Addressing exception if outside real storage */
            if (pto > regs->mainlim)
                goto address_excp;

            /* Fetch the page table entry from absolute storage.  All bytes
               must be fetched concurrently as observed by other CPUs */
            pte = ARCH_DEP(fetch_doubleword_absolute) (pto, regs);
//          LOGMSG("pte:%16.16"PRIX64"=>%16.16"PRIX64"\n",pto,pte);

            /* Page translation exception if page invalid */
            if (pte & ZPGETAB_I)
                goto page_tran_invalid;

            /* Check that all the reserved bits in the PTE are zero */
            if (pte & ZPGETAB_RESV)
                goto tran_spec_excp;

        } /* end else(ASCE_R) */

        /* Set protection indicator if page protection is indicated
           in either the segment table or the page table */
        if ((ste & ZSEGTAB_P) || (pte & ZPGETAB_P))
            regs->dat.protect |= 1;

        /* [3.11.4.2] Place the translated address in the TLB */
        if (!(acctype & ACC_NOTLB))
        {
            regs->tlb.TLB_ASD(tlbix)   = regs->dat.asd;
            regs->tlb.TLB_VADDR(tlbix) = (vaddr & TLBID_PAGEMASK) | regs->tlbID;
            regs->tlb.TLB_PTE(tlbix)   = pte;
            regs->tlb.common[tlbix]    = (ste & SEGTAB_COMMON) ? 1 : 0;
            regs->tlb.protect[tlbix]   = regs->dat.protect;
            regs->tlb.acc[tlbix]       = 0;
            regs->tlb.main[tlbix]      = NULL;
        }
    }

    if(!(acctype & ACC_PTE))
    {
        /* Combine the page frame real address with the byte index
           of the virtual address to form the real address */
        regs->dat.raddr = (pte & ZPGETAB_PFRA) | (vaddr & 0xFFF);
        regs->dat.rpfra = (pte & ZPGETAB_PFRA);
    }
    else
        regs->dat.raddr = pto;
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

    /*-----------------------------------------------------------*/
    /* The following code is common to S/370, ESA/390, and ESAME */
    /*-----------------------------------------------------------*/

    /* Clear exception code and return with zero return code */
    regs->dat.xcode = 0;
    return 0;

/* Conditions which always cause program check, except
   when performing translation for the control panel */
address_excp:
//    LOGMSG("dat.c: addressing exception: %8.8X %8.8X %4.4X %8.8X\n",
//        regs->CR(0),regs->dat.asd,pte,vaddr);
    regs->dat.xcode = PGM_ADDRESSING_EXCEPTION;
    goto tran_prog_check;

tran_spec_excp:
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
//    LOGMSG("dat.c: translation specification exception...\n");
//    LOGMSG("       pte = %16.16"PRIX64", ste = %16.16"PRIX64", rte=%16.16"PRIX64"\n",
//        pte, ste, rte);
#else
//    LOGMSG("dat.c: translation specification exception...\n");
//    LOGMSG("       cr0=%8.8X ste=%8.8X pte=%4.4X vaddr=%8.8X\n",
//        regs->CR(0),ste,pte,vaddr);
#endif
    regs->dat.xcode = PGM_TRANSLATION_SPECIFICATION_EXCEPTION;
    goto tran_prog_check;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
spec_oper_excp:
    regs->dat.xcode = PGM_SPECIAL_OPERATION_EXCEPTION;
    goto tran_prog_check;
#endif

tran_prog_check:
#if defined( FEATURE_036_ENH_MONITOR_FACILITY )
    if (FACILITY_ENABLED( 036_ENH_MONITOR, regs ))
    {
        /* No program interrupt for enhanced MC */
        if (acctype & ACC_ENH_MC)
        {
            cc = 5;
            return cc;
        }
    }
#endif
    regs->program_interrupt( regs, regs->dat.xcode );

/* Conditions which the caller may or may not program check */
seg_tran_invalid:
    /* For LPTEA, return segment table entry address with cc 2 */
    if (acctype & ACC_LPTEA)
    {
        regs->dat.raddr = sto;
        cc = 2;
        return cc;
    } /* end if(ACCTYPE_LPTEA) */

    /* Otherwise set translation exception code */
    regs->dat.xcode = PGM_SEGMENT_TRANSLATION_EXCEPTION;
    regs->dat.raddr = sto;
    cc = 1;
    goto tran_excp_addr;

page_tran_invalid:
    regs->dat.xcode = PGM_PAGE_TRANSLATION_EXCEPTION;
    regs->dat.raddr = pto;
    if(acctype & ACC_PTE) return 0;
    cc = 2;
    goto tran_excp_addr;

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
page_tran_length:
    regs->dat.xcode = PGM_PAGE_TRANSLATION_EXCEPTION;
    regs->dat.raddr = pto;
    cc = 3;
    goto tran_excp_addr;
#endif

seg_tran_length:
//  LOGMSG("dat.c: segment translation exception due to segment length\n");
//  LOGMSG("       cr0=" F_RADR " sto=" F_RADR "\n",regs->CR(0),sto);
    regs->dat.xcode = PGM_SEGMENT_TRANSLATION_EXCEPTION;
    regs->dat.raddr = sto;
    cc = 3;
    goto tran_excp_addr;

tran_alet_excp:
    regs->excarid = arn;
    cc = (acctype & ACC_LPTEA) ? 3 : 4;
    return cc;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
reg_first_invalid:
    /* For LPTEA, return region table entry address with cc 2 */
    if (acctype & ACC_LPTEA)
    {
        regs->dat.raddr = rto | (TT_R1TABL >> 2);
        cc = 2;
        return cc;
    } /* end if(ACCTYPE_LPTEA) */

    /* Otherwise set translation exception code */
    goto reg_first_excp;

reg_second_invalid:
    /* For LPTEA, return region table entry address with cc 2 */
    if (acctype & ACC_LPTEA)
    {
        regs->dat.raddr = rto | (TT_R2TABL >> 2);
        cc = 2;
        return cc;
    } /* end if(ACCTYPE_LPTEA) */

    /* Otherwise set translation exception code */
    goto reg_second_excp;

reg_third_invalid:
    /* For LPTEA, return region table entry address with cc 2 */
    if (acctype & ACC_LPTEA)
    {
        regs->dat.raddr = rto | (TT_R3TABL >> 2);
        cc = 2;
        return cc;
    } /* end if(ACCTYPE_LPTEA) */

    /* Otherwise set translation exception code */
    goto reg_third_excp;

asce_type_excp:
//  LOGMSG("rfx = %4.4X, rsx %4.4X, rtx = %4.4X, tt = %1.1X\n",
//      rfx, rsx, rtx, tt);
    regs->dat.xcode = PGM_ASCE_TYPE_EXCEPTION;
    cc = 4;
    goto tran_excp_addr;

reg_first_excp:
    regs->dat.xcode = PGM_REGION_FIRST_TRANSLATION_EXCEPTION;
    cc = 4;
    goto tran_excp_addr;

reg_second_excp:
    regs->dat.xcode = PGM_REGION_SECOND_TRANSLATION_EXCEPTION;
    cc = 4;
    goto tran_excp_addr;

reg_third_excp:
    regs->dat.xcode = PGM_REGION_THIRD_TRANSLATION_EXCEPTION;
    cc = 4;
    goto tran_excp_addr;
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

tran_excp_addr:
    /* For LPTEA instruction, return xcode with cc = 3 */
    if (acctype & ACC_LPTEA)
        return 3;

    /* Set the translation exception address */
    regs->TEA = vaddr & PAGEFRAME_PAGEMASK;

    /* Set the address space indication in the exception address */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if(regs->dat.stid == TEA_ST_ARMODE)
    {
        if ((regs->dat.asd & ASCE_TO) == (regs->CR(1) & ASCE_TO))
            regs->TEA |= TEA_ST_PRIMARY;
        else if ((regs->dat.asd & ASCE_TO) == (regs->CR(7) & ASCE_TO))
            regs->TEA |= TEA_ST_SECNDRY;
        else if ((regs->dat.asd & ASCE_TO) == (regs->CR(13) & ASCE_TO))
            regs->TEA |= TEA_ST_HOME;
        else
            regs->TEA |= TEA_ST_ARMODE;
    }
    else
        regs->TEA |= regs->dat.stid;
#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
    if(regs->dat.stid == TEA_ST_ARMODE)
    {
        if ((regs->dat.asd & STD_STO) == (regs->CR(1) & STD_STO))
            regs->TEA |= TEA_ST_PRIMARY;
        else if ((regs->dat.asd & STD_STO) == (regs->CR(7) & STD_STO))
            regs->TEA |= TEA_ST_SECNDRY;
        else if ((regs->dat.asd & STD_STO) == (regs->CR(13) & STD_STO))
            regs->TEA |= TEA_ST_HOME;
        else
            regs->TEA |= TEA_ST_ARMODE;
    }
    else
        if((regs->dat.stid == TEA_ST_SECNDRY)
          && (PRIMARY_SPACE_MODE(&regs->psw)
            || SECONDARY_SPACE_MODE(&regs->psw)))
            regs->TEA |= TEA_ST_SECNDRY | TEA_SECADDR;
        else
            regs->TEA |= regs->dat.stid;
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

#if defined( FEATURE_075_ACC_EX_FS_INDIC_FACILITY )
    if (FACILITY_ENABLED( 075_ACC_EX_FS_INDIC, regs ))
    {
        /* Set the fetch/store indication bits 52-53 in the TEA */
        if (acctype & ACC_READ)
        {
            regs->TEA |= TEA_FETCH;
        }
        else if (acctype & (ACC_WRITE | ACC_CHECK))
        {
            regs->TEA |= TEA_STORE;
        }
    }
#endif
    /* Set the exception access identification */
    if (ACCESS_REGISTER_MODE(&regs->psw)
     || (SIE_ACTIVE(regs) && MULTIPLE_CONTROLLED_DATA_SPACE(GUESTREGS))
       )
       regs->excarid = arn < 0 ? 0 : arn;

    /* Return condition code */
    return cc;

} /* end function translate_addr */


/*-------------------------------------------------------------------*/
/*                      is_tlbe_match                                */
/*-------------------------------------------------------------------*/
bool ARCH_DEP( is_tlbe_match )( REGS* regs, REGS* host_regs, U64 pfra, int i )
{
RADR pte;
RADR ptemask;
bool match = false;

#if !defined( FEATURE_S390_DAT ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    ptemask = ((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K) ?
              PAGETAB_PFRA_4K : PAGETAB_PFRA_2K;
    pte = ((pfra & 0xFFFFFF) >> 8) & ptemask;
#endif
#if defined( FEATURE_S390_DAT )
    ptemask = PAGETAB_PFRA;
    pte = pfra & ptemask;
#endif
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    ptemask = (RADR)ZPGETAB_PFRA;
    pte = pfra & ptemask;
#endif

    if ((regs->tlb.TLB_PTE(i) & ptemask) == pte)
        match = true;
    else if (!host_regs)
        match = false;
    else switch (host_regs->arch_mode)
    {
    case ARCH_370_IDX: match = s370_is_tlbe_match( host_regs, NULL, pfra, i ); break;
    case ARCH_390_IDX: match = s390_is_tlbe_match( host_regs, NULL, pfra, i ); break;
    case ARCH_900_IDX: match = z900_is_tlbe_match( host_regs, NULL, pfra, i ); break;
    default: CRASH();
    }

    return match;
}

/*-------------------------------------------------------------------*/
/*                      do_purge_tlbe                                */
/*-------------------------------------------------------------------*/
void ARCH_DEP( do_purge_tlbe )( REGS* regs, REGS* host_regs, U64 pfra )
{
int  i;

    INVALIDATE_AIA( regs );

    for (i=0; i < TLBN; i++)
        if (ARCH_DEP( is_tlbe_match )( regs, host_regs, pfra, i ))
            regs->tlb.TLB_VADDR(i) &= TLBID_PAGEMASK;
}

/*-------------------------------------------------------------------*/
/* Purge a specific translation lookaside buffer entry               */
/*-------------------------------------------------------------------*/
void ARCH_DEP( purge_tlbe )( REGS* regs, U64 pfra )
{
    /* Do it for the current architecture first */
    ARCH_DEP( do_purge_tlbe )( regs, NULL, pfra );

#if defined( _FEATURE_SIE )

    /* Also clear the GUEST registers in the SIE copy */
    if (regs->host && GUESTREGS)
    {
        /*************************************************************/
        /*                                                           */
        /*                   PROGRAMMING NOTE                        */
        /*                                                           */
        /* The SIE guest's TLB PTE entries for DAT-OFF guests like   */
        /* CMS do NOT actually contain the PTE, but rather contain   */
        /* the host primary virtual address. Both are masked with    */
        /* TBLID_PAGEMASK however. Therefore in order to properly    */
        /* check if such a guest TLB PTE entry needs to be cleared,  */
        /* one needs to also check the host's TLB PTE for a match    */
        /* as well. In other words, instead of just doing:           */
        /*                                                           */
        /*    if ((GUESTREGS->tlb.TLB_PTE(i) & ptemask) == pte)      */
        /*         GUESTREGS->tlb.TLB_VADDR(i) &= TLBID_PAGEMASK;    */
        /*                                                           */
        /* we need to essentially do the following instead:          */
        /*                                                           */
        /*    if ((GUESTREGS->tlb.TLB_PTE(i) & ptemask) == pte ||    */
        /*         (HOSTREGS->tlb.TLB_PTE(i) & ptemask) == pte)      */
        /*         GUESTREGS->tlb.TLB_VADDR(i) &= TLBID_PAGEMASK;    */
        /*                                                           */
        /*                         (Peter J. Jansen, 29-Jul-2016)    */
        /*                                                           */
        /* This is accomplished by also passing the host's registers */
        /* to the "do_purge_tlbe" function so it can know to also    */
        /* check the host's TLB PTE entry for a match as well.       */
        /*                                                           */
        /*                    "Fish" (David B. Trout), 07-Oct-2021   */
        /*                                                           */
        /*************************************************************/

        switch (GUESTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_do_purge_tlbe( GUESTREGS, regs, pfra ); break;
        case ARCH_390_IDX: s390_do_purge_tlbe( GUESTREGS, regs, pfra ); break;
        case ARCH_900_IDX: z900_do_purge_tlbe( GUESTREGS, regs, pfra ); break;
        default: CRASH();
        }
    }
    else if (regs->guest)  /* For guests, also clear HOST entries */
    {
        switch (HOSTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_do_purge_tlbe( HOSTREGS, NULL, pfra ); break;
        case ARCH_390_IDX: s390_do_purge_tlbe( HOSTREGS, NULL, pfra ); break;
        case ARCH_900_IDX: z900_do_purge_tlbe( HOSTREGS, NULL, pfra ); break;
        default: CRASH();
        }
    }
#endif /* defined( _FEATURE_SIE ) */

} /* end function purge_tlbe */


/*-------------------------------------------------------------------*/
/*                 invalidate_tlb helper                             */
/*-------------------------------------------------------------------*/
void ARCH_DEP( do_invalidate_tlb )( REGS* regs, BYTE mask )
{
int  i;

    INVALIDATE_AIA( regs );
    if (mask == 0)
        memset( &regs->tlb.acc, 0, TLBN );
    else
        for (i=0; i < TLBN; i++)
            if ((regs->tlb.TLB_VADDR(i) & TLBID_BYTEMASK) == regs->tlbID)
                regs->tlb.acc[i] &= mask;
}

/*-------------------------------------------------------------------*/
/* Invalidate one or more translation lookaside buffer entries       */
/*-------------------------------------------------------------------*/
void ARCH_DEP( invalidate_tlb )( REGS* regs, BYTE mask )
{
    /* Do it for the current architecture first */
    ARCH_DEP( do_invalidate_tlb )( regs, mask );

#if defined( _FEATURE_SIE )
    /* Also invalidate the GUEST registers in the SIE copy */
    if (regs->host && GUESTREGS)
    {
        switch (GUESTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_do_invalidate_tlb( GUESTREGS, mask ); break;
        case ARCH_390_IDX: s390_do_invalidate_tlb( GUESTREGS, mask ); break;
        case ARCH_900_IDX: z900_do_invalidate_tlb( GUESTREGS, mask ); break;
        default: CRASH();
        }
    }
    else if (regs->guest)  /* For guests, also clear HOST entries */
    {
        switch (HOSTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_do_invalidate_tlb( HOSTREGS, mask ); break;
        case ARCH_390_IDX: s390_do_invalidate_tlb( HOSTREGS, mask ); break;
        case ARCH_900_IDX: z900_do_invalidate_tlb( HOSTREGS, mask ); break;
        default: CRASH();
        }
    }
#endif /* defined( _FEATURE_SIE ) */
} /* end function invalidate_tlb */


/*-------------------------------------------------------------------*/
/*                 invalidate_tlbe helper                            */
/*-------------------------------------------------------------------*/
void ARCH_DEP( do_invalidate_tlbe )( REGS* regs, BYTE* main )
{
    int     i;                          /* index into TLB            */
    int     shift;                      /* Number of bits to shift   */
    BYTE*   mainwid;                    /* mainstore with tlbid      */

    if (!main)
    {
        ARCH_DEP( invalidate_tlb )( regs, 0 );
        return;
    }

    mainwid = main + regs->tlbID;

    INVALIDATE_AIA_MAIN( regs, main );

    shift = (regs->arch_mode == ARCH_370_IDX) ? 11 : 12;

    for (i=0; i < TLBN; i++)
    {
        if (MAINADDR( regs->tlb.main[i], (regs->tlb.TLB_VADDR(i) | (i << shift)) ) == mainwid)
        {
            regs->tlb.acc[i] = 0;

            // 370?
#if !defined( FEATURE_S390_DAT ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

            if ((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K)
                regs->tlb.acc[i^1] = 0;
#endif
        }
    }
}

/*-------------------------------------------------------------------*/
/* Invalidate matching translation lookaside buffer entries          */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/*      main    mainstor address to match on. This is mainstor       */
/*              base plus absolute address (regs->mainstor+aaddr)    */
/*                                                                   */
/*    This function is called by the SSK(E) instructions to purge    */
/*    TLB entries that match the mainstor address. The "main"        */
/*    field in the TLB contains the mainstor address plus an         */
/*    XOR hash with effective address (regs->mainstor+aaddr^addr).   */
/*    Before the compare can happen, the effective address from      */
/*    the tlb (TLB_VADDR) must be XORed with the "main" field from   */
/*    the tlb (removing hash).  This is done using MAINADDR() macro. */
/*                                                                   */
/* NOTES:                                                            */
/*                                                                   */
/*   TLB_VADDR does not contain all the effective address bits and   */
/*   must be created on-the-fly using the tlb index (i << shift).    */
/*   TLB_VADDR also contains the tlbid, so the regs->tlbid is merged */
/*   with the main input variable before the search is begun.        */
/*                                                                   */
/*-------------------------------------------------------------------*/
void ARCH_DEP( invalidate_tlbe )( REGS* regs, BYTE* main )
{
    /* Do it for the current architecture first */
    ARCH_DEP( do_invalidate_tlbe )( regs, main );

#if defined( _FEATURE_SIE )
    /* Also clear the GUEST registers in the SIE copy */
    if (regs->host && GUESTREGS)
    {
        switch (GUESTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_do_invalidate_tlbe( GUESTREGS, main ); break;
        case ARCH_390_IDX: s390_do_invalidate_tlbe( GUESTREGS, main ); break;
        case ARCH_900_IDX: z900_do_invalidate_tlbe( GUESTREGS, main ); break;
        default: CRASH();
        }
    }
    else if (regs->guest)  /* For guests, also clear HOST entries */
    {
        switch (HOSTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_do_invalidate_tlbe( HOSTREGS, main ); break;
        case ARCH_390_IDX: s390_do_invalidate_tlbe( HOSTREGS, main ); break;
        case ARCH_900_IDX: z900_do_invalidate_tlbe( HOSTREGS, main ); break;
        default: CRASH();
        }
    }
#endif /* defined( _FEATURE_SIE ) */

} /* end function invalidate_tlbe */


/*-------------------------------------------------------------------*/
/*                Invalidate Page Table Entry                        */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  This function is called by the IPTE and IESBE instructions. It   */
/*  either sets the PAGETAB_INVALID bit (for IPTE instruction) or    */
/*  resets the PAGETAB_ESVALID bit (for IESBE instruction) for the   */
/*  Page Table Entry addressed by the passed Page Table Origin value */
/*  plus the Virtual Address's page index value, and clears the TLB  */
/*  of all entries with a Page Frame Real Address matching the one   */
/*  from the invalidated Page Table Entry.                           */
/*                                                                   */
/*  Input:                                                           */
/*                                                                   */
/*      ibyte   0x21 = IPTE instruction, 0x59 = IESBE instruction    */
/*      pto     Real address of Page Table Origin identifying the    */
/*              Page Table containing the Entry to be invalidated    */
/*      vaddr   Virtual Address of page within specified Page Table  */
/*              whose entry is to be invalidated                     */
/*      regs    CPU register context                                 */
/*      local   true = clear only local TLB entry, else all CPUs     */
/*                                                                   */
/*                     *** IMPORTANT! ***                            */
/*                                                                   */
/*           This function expects INTLOCK to be held                */
/*         and SYNCHRONIZE_CPUS to be called beforehand!             */
/*                                                                   */
/*-------------------------------------------------------------------*/
void ARCH_DEP( invalidate_pte )( BYTE ibyte, RADR pto, VADR vaddr, REGS* regs, bool local )
{
RADR    raddr;                          /* Addr of Page Table Entry  */
RADR    pte;                            /* Page Table Entry itself   */
RADR    pfra;                           /* Page Frame Real Address   */

    UNREFERENCED_370( ibyte );

#if !defined( FEATURE_S390_DAT ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    {
        // SYSTEM/370...

        /* Program check if translation format is invalid */
        if (0
            || (((regs->CR(0) & CR0_PAGE_SIZE) != CR0_PAGE_SZ_2K) && ((regs->CR(0) & CR0_PAGE_SIZE) != CR0_PAGE_SZ_4K))
            || (((regs->CR(0) & CR0_SEG_SIZE)  != CR0_SEG_SZ_64K) && ((regs->CR(0) & CR0_SEG_SIZE)  != CR0_SEG_SZ_1M))
        )
            regs->program_interrupt( regs, PGM_TRANSLATION_SPECIFICATION_EXCEPTION );

        /* Add the vaddr's page table entry index to the Page Table
           Origin, ignoring any carry, to form the 24-bit real address
           of the Page Table Entry to be invalidated, taking into account
           that each Page Table Entry is 2 bytes wide (shift 1 less bit)
        */
        raddr = (pto & SEGTAB_370_PTO) +
        (
            ((regs->CR(0)  & CR0_SEG_SIZE)  == CR0_SEG_SZ_1M)
            ?
            (((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K) ?
            ((vaddr & 0x000FF000) >> (SHIFT_4K-1)) : ((vaddr & 0x000FF800) >> (SHIFT_2K-1)))
            :
            (((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K) ?
            ((vaddr & 0x0000F000) >> (SHIFT_4K-1)) : ((vaddr & 0x0000F800) >> (SHIFT_2K-1)))
        );
        raddr &= MAXADDRESS;

        /* Fetch the Page Table Entry from real storage,
           subject to normal storage protection mechanisms
        */
        pte = ARCH_DEP( vfetch2 )( raddr, USE_REAL_ADDR, regs );

#if 0 // debug 370 IPTE
        LOGMSG
        (
            "dat.c: IPTE issued for entry %4.4X at %8.8X...\n"
            "       pto %8.8X, vaddr %8.8X, cr0 %8.8X\n"

            , pte, raddr
            , pto, vaddr, regs->CR(0)
        );
#endif

        /* Set the invalid bit in the Page Table Entry just fetched */
        pte |= ((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_2K)
               ? PAGETAB_INV_2K : PAGETAB_INV_4K;

        /* Store the now invalidated Page Table Entry back into
           real storage where we originally got it from, subject
           to the same storage protection mechanisms
        */
        ARCH_DEP( vstore2 )( pte, raddr, USE_REAL_ADDR, regs );

        /* Extract the Page Frame Address from the Page Table Entry */
        pfra = ((regs->CR(0) & CR0_PAGE_SIZE) == CR0_PAGE_SZ_4K)
            ?
#if defined( FEATURE_S370E_EXTENDED_ADDRESSING )
            (((U32)pte & PAGETAB_EA_4K) << 23) |
#endif
            (((U32)pte & PAGETAB_PFRA_4K) << 8)
            :
            (((U32)pte & PAGETAB_PFRA_2K) << 8);
    }
#elif defined( FEATURE_S390_DAT )
    {
        // SYSTEM/390...

        /* Program check if translation format is invalid */
        if ((regs->CR(0) & CR0_TRAN_FMT) != CR0_TRAN_ESA390)
            regs->program_interrupt( regs, PGM_TRANSLATION_SPECIFICATION_EXCEPTION );

        /* Add the vaddr's page table entry index to the Page Table
           Origin, ignoring any carry, to form the 31-bit real address
           of the Page Table Entry to be invalidated, taking into account
           that each Page Table Entry is 4 bytes wide (shift 2 fewer bits)
        */
        raddr = (pto & SEGTAB_PTO) + ((vaddr & 0x000FF000) >> (PAGEFRAME_PAGESHIFT-2));
        raddr &= MAXADDRESS;

        /* Fetch the Page Table Entry from real storage,
           subject to normal storage protection mechanisms */
        pte = ARCH_DEP( vfetch4 )( raddr, USE_REAL_ADDR, regs );

        /* Set the invalid bit in the Page Table Entry just fetched */
#if defined( FEATURE_MOVE_PAGE_FACILITY_2 ) && defined( FEATURE_EXPANDED_STORAGE )
        if (ibyte == 0x59) // (IESBE instruction?)
            pte &= ~PAGETAB_ESVALID;
        else
#endif
            pte |= PAGETAB_INVALID; // (no, IPTE instruction)

        /* Store the now invalidated Page Table Entry back into
           real storage where we originally got it from, subject
           to the same storage protection mechanisms
        */
        ARCH_DEP( vstore4 )( pte, raddr, USE_REAL_ADDR, regs );

        /* Extract the Page Frame Address from the Page Table Entry */
        pfra = pte & PAGETAB_PFRA;
    }
#else /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
    {
        // ESAME = z/Architecture...

        /* Add the vaddr's page table entry index to the Page Table
           Origin, ignoring any carry, to form the 64-bit real address
           of the Page Table Entry to be invalidated, taking into account
           that each Page Table Entry is 8 bytes wide (shift 3 fewer bits)
        */
        raddr = (pto & ZSEGTAB_PTO) + ((vaddr & 0x000FF000) >> (PAGEFRAME_PAGESHIFT-3));
        raddr &= MAXADDRESS;

        /* Fetch the Page Table Entry from real storage,
           subject to normal storage protection mechanisms
        */
        pte = ARCH_DEP( vfetch8 )( raddr, USE_REAL_ADDR, regs );

        /* Set the invalid bit in the Page Table Entry just fetched */
#if defined( FEATURE_MOVE_PAGE_FACILITY_2 ) && defined( FEATURE_EXPANDED_STORAGE )
        if (ibyte == 0x59) // (IESBE instruction?)
            pte &= ~ZPGETAB_ESVALID;
        else
#endif
            pte |= ZPGETAB_I; // (no, IPTE instruction)

        /* Store the now invalidated Page Table Entry back into
           real storage where we originally got it from, subject
           to the same storage protection mechanisms
        */
        ARCH_DEP( vstore8 )( pte, raddr, USE_REAL_ADDR, regs );

        /* Extract the Page Frame Address from the Page Table Entry */
        pfra = pte & ZPGETAB_PFRA;
    }
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

    /* Invalidate all TLB entries for this Page Frame Real Address */
    ARCH_DEP( purge_tlbe_all )( regs, pfra, local ? regs->cpuad : 0xFFFF );

} /* end function invalidate_pte */


#if defined( FEATURE_PER2 )
/*-------------------------------------------------------------------*/
/* Check for a storage alteration PER2 event. Returns true or false. */
/*-------------------------------------------------------------------*/
static inline bool ARCH_DEP( check_sa_per2 )( int arn, int acctype, REGS* regs )
{
    UNREFERENCED( acctype );

    if (0
        || (regs->dat.asd & SAEVENT_BIT)
        || !(regs->CR(9) & CR9_SAC)
    )
    {
        regs->peraid = arn > 0 ? arn : 0;
        regs->perc |= regs->dat.stid;
        return true;
    }
    return false;
}
#endif /* defined( FEATURE_PER2 ) */


/*-------------------------------------------------------------------*/
/* Convert logical address to absolute address and check protection  */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/*      addr    Logical address to be translated                     */
/*      arn     Access register number (or USE_REAL_ADDR,            */
/*                      USE_PRIMARY_SPACE, USE_SECONDARY_SPACE)      */
/*      regs    CPU register context                                 */
/*      acctype Type of access requested: READ, WRITE, CHECK or HW.  */
/*      akey    Bits 0-3=access key, 4-7=zeroes                      */
/*      len     Length of data access for PER SA purpose             */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/*      Absolute storage address.                                    */
/*                                                                   */
/*      If the PSW indicates DAT-off, or if the access register      */
/*      number parameter is the special value USE_REAL_ADDR,         */
/*      then the addr parameter is treated as a real address.        */
/*      Otherwise addr is a virtual address, so dynamic address      */
/*      translation is called to convert it to a real address.       */
/*                                                                   */
/*      Prefixing is then applied to convert the real address to     */
/*      an absolute address, and then, if this is not a hardware     */
/*      access, low-address protection, access-list controlled       */
/*      protection, page protection, and key controlled protection   */
/*      checks are applied to the address. When storage is being     */
/*      accessed by the hardware (acctype = ACCTYPE_HW = 0) then     */
/*      none of the previously mentioned checks are performed and    */
/*      the absolute address is immediately returned. Otherwise,     */
/*      if successful, the reference and change bits of the storage  */
/*      key are updated, and the absolute address is returned.       */
/*                                                                   */
/*      If the logical address causes an addressing, protection,     */
/*      or translation exception then a program check is generated   */
/*      and the function does not return.                            */
/*                                                                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT
BYTE *ARCH_DEP( logical_to_main_l )( VADR addr, int arn,
                                     REGS *regs, int acctype,
                                     BYTE akey, size_t len )
{
RADR    aaddr;                          /* Absolute address          */
RADR    apfra;                          /* Abs page frame address    */
int     ix = TLBIX(addr);               /* TLB index                 */

    /* Convert logical address to real address */
    if ( (REAL_MODE(&regs->psw) || arn == USE_REAL_ADDR)
#if defined( FEATURE_SIE )
      /* Under SIE guest real is always host primary, regardless
         of the DAT mode */
      && !(regs->sie_active
#if !defined( _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
                            && arn == USE_PRIMARY_SPACE
#else
//                          && ( (arn == USE_PRIMARY_SPACE)
//                               || SIE_STATE_BIT_ON(GUESTREGS, MX, XC) )
#endif /* defined( _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE ) */
          )
#endif /* defined( FEATURE_SIE ) */
       )
    {
        regs->dat.pvtaddr = regs->dat.protect = 0;
        regs->dat.raddr = addr;
        regs->dat.rpfra = addr & PAGEFRAME_PAGEMASK;

        /* Setup `real' TLB entry (for MADDR) */
        regs->tlb.TLB_ASD(ix)   = TLB_REAL_ASD;
        regs->tlb.TLB_VADDR(ix) = (addr & TLBID_PAGEMASK) | regs->tlbID;
        regs->tlb.TLB_PTE(ix)   = addr & TLBID_PAGEMASK;
        regs->tlb.acc[ix]       =
        regs->tlb.common[ix]    =
        regs->tlb.protect[ix]   = 0;
    }
    else {
        if (ARCH_DEP(translate_addr) (addr, arn, regs, acctype))
            goto vabs_prog_check;
    }

    if (regs->dat.protect
     && (acctype & (ACC_WRITE|ACC_CHECK)))
        goto vabs_prot_excp;

    /* Convert real address to absolute address */
    regs->dat.aaddr = aaddr = APPLY_PREFIXING (regs->dat.raddr, regs->PX);
    apfra=APPLY_PREFIXING(regs->dat.rpfra,regs->PX);

    /* Program check if absolute address is outside main storage */
    if (regs->dat.aaddr > regs->mainlim)
        goto vabs_addr_excp;

#if defined( _FEATURE_SIE )
    if(SIE_MODE(regs)) HOSTREGS->dat.protect = 0;
    if(SIE_MODE(regs)  && !regs->sie_pref)
    {
        if (SIE_TRANSLATE_ADDR (regs->sie_mso + regs->dat.aaddr,
                    (arn > 0 && MULTIPLE_CONTROLLED_DATA_SPACE(regs)) ? arn : USE_PRIMARY_SPACE,
                    HOSTREGS, ACCTYPE_SIE))
            (HOSTREGS->program_interrupt) (HOSTREGS, HOSTREGS->dat.xcode);

        regs->dat.protect     |= HOSTREGS->dat.protect;
        regs->tlb.protect[ix] |= HOSTREGS->dat.protect;

        if ( REAL_MODE(&regs->psw) || (arn == USE_REAL_ADDR) )
            regs->tlb.TLB_PTE(ix)   = addr & TLBID_PAGEMASK;

        /* Indicate a host real space entry for a XC dataspace */
        if (arn > 0 && MULTIPLE_CONTROLLED_DATA_SPACE(regs))
        {
            regs->tlb.TLB_ASD(ix) = regs->dat.asd;
            /* Ensure that the private bit is percolated to the guest such that LAP is applied correctly */
            regs->dat.pvtaddr = HOSTREGS->dat.pvtaddr;

            /* Build tlb entry of XC dataspace */
            regs->dat.asd = HOSTREGS->dat.asd ^ TLB_HOST_ASD;
            regs->CR(CR_ALB_OFFSET + arn) = regs->dat.asd;
            regs->AEA_AR(arn) = CR_ALB_OFFSET + arn;
            regs->AEA_COMMON(CR_ALB_OFFSET + arn) = (regs->dat.asd & ASD_PRIVATE) == 0;
            regs->aea_aleprot[arn] = HOSTREGS->dat.protect & 2;
        }

        /* Convert host real address to host absolute address.
           Use the Prefixing logic of the SIE host, not the guest!
                                               -- ISW 20181005
        */
        HOSTREGS->dat.aaddr =
        aaddr = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );
        apfra = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.rpfra );

        if (HOSTREGS->dat.aaddr > HOSTREGS->mainlim)
            goto vabs_addr_excp;

        /* Take into account SIE guests with a 2K page scheme
           because the SIE host may be operating with a 4K page
           system */
#if defined( FEATURE_2K_STORAGE_KEYS )
        if ((addr & PAGEFRAME_PAGEMASK) & 0x800)
            apfra |= 0x800;
#endif
    }
#endif /* defined( _FEATURE_SIE ) */

    /* Save ptr to storage key for this translated logical address */
    regs->dat.storkey = ARCH_DEP( get_ptr_to_storekey )( aaddr );

#if defined( _FEATURE_SIE )
    /* Do not apply host key access when SIE fetches/stores data */
    if (unlikely(SIE_ACTIVE(regs)))
        return regs->mainstor + aaddr;
#endif

    /* Skip further processing if hardware access */
    if (unlikely( acctype == ACCTYPE_HW ))
    {
        /* Return mainstor address */
        return regs->mainstor + aaddr;
    }

    /* Check protection and set reference and change bits */
    if (likely( acctype & ACC_READ ))
    {
        /* Program check if fetch protected location */
        if (unlikely(ARCH_DEP(is_fetch_protected) (addr, *regs->dat.storkey, akey, regs)))
        {
            if (SIE_MODE(regs)) HOSTREGS->dat.protect = 0;
            goto vabs_prot_excp;
        }

        /* Set the reference bit in the storage key */
        ARCH_DEP( or_storage_key_by_ptr )( regs->dat.storkey, STORKEY_REF );

        /* Update accelerated lookup TLB fields */
        regs->tlb.storkey[ix]    = regs->dat.storkey;
        regs->tlb.skey[ix]       = ARCH_DEP( get_storekey_by_ptr )( regs->dat.storkey ) & STORKEY_KEY;
        regs->tlb.acc[ix]        = ACC_READ;
        regs->tlb.main[ix]       = NEW_MAINADDR (regs, addr, apfra);

    }
    else /* (acctype & (ACC_WRITE | ACC_CHECK)) */
    {
        /* Program check if store protected location */
        if (unlikely(ARCH_DEP(is_store_protected) (addr, *regs->dat.storkey, akey, regs)))
        {
            if (SIE_MODE(regs)) HOSTREGS->dat.protect = 0;
            goto vabs_prot_excp;
        }
        if (SIE_MODE(regs) && HOSTREGS->dat.protect)
            goto vabs_prot_excp;

        /* Set the reference and change bits in the storage key */
        if (acctype & ACC_WRITE)
            ARCH_DEP( or_storage_key_by_ptr )( regs->dat.storkey, (STORKEY_REF | STORKEY_CHANGE) );

        /* Update accelerated lookup TLB fields */
        regs->tlb.storkey[ix] = regs->dat.storkey;
        regs->tlb.skey[ix]    = ARCH_DEP( get_storekey_by_ptr )( regs->dat.storkey ) & STORKEY_KEY;
        regs->tlb.acc[ix]     = (addr >= PSA_SIZE || regs->dat.pvtaddr)
                              ? (ACC_READ | ACC_CHECK | acctype)
                              :  ACC_READ;
        regs->tlb.main[ix]    = NEW_MAINADDR (regs, addr, apfra);

#if defined( FEATURE_PER )
        if (EN_IC_PER_SA( regs ))
        {
            regs->tlb.acc[ix] = ACC_READ;
            if (1
                && arn != USE_REAL_ADDR
#if defined( FEATURE_PER2 )
                && (0
                    || REAL_MODE( &regs->psw )
                    || ARCH_DEP( check_sa_per2 )( arn, acctype, regs )
                   )
#endif /* defined( FEATURE_PER2 ) */
                /* Check that the range that was altered is within the PER SA range */
                && PER_RANGE_CHECK2( addr, addr+(len-1), regs->CR(10), regs->CR(11))
                && !IS_PER_SUPRESS( regs, CR9_SA )
            )
                ON_IC_PER_SA( regs );
        }
#endif /* defined( FEATURE_PER ) */
    } /* (acctype & (ACC_WRITE | ACC_CHECK)) */

    /* Return mainstor address */
    return regs->mainstor + aaddr;

vabs_addr_excp:

    regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);

vabs_prot_excp:

#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
    regs->TEA = addr & STORAGE_KEY_PAGEMASK;
    if (regs->dat.protect && (acctype & (ACC_WRITE|ACC_CHECK)) )
    {
        regs->TEA |= TEA_PROT_AP;
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->dat.protect & 2)
            regs->TEA |= TEA_PROT_A;
#endif
    }
    regs->TEA |= regs->dat.stid;
    regs->excarid = (arn > 0 ? arn : 0);
#endif /* defined( FEATURE_SUPPRESSION_ON_PROTECTION ) */

#if defined( _FEATURE_PROTECTION_INTERCEPTION_CONTROL )
    if (SIE_MODE( regs ) && HOSTREGS->dat.protect)
    {
#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )

        switch (HOSTREGS->arch_mode)
        {
        case ARCH_370_IDX: HOSTREGS->TEA_370 = regs->TEA; break;
        case ARCH_390_IDX: HOSTREGS->TEA_390 = regs->TEA; break;
        case ARCH_900_IDX: HOSTREGS->TEA_900 = regs->TEA; break;
        default: CRASH();
        }

        HOSTREGS->excarid = regs->excarid;

#endif /* defined( FEATURE_SUPPRESSION_ON_PROTECTION ) */

        HOSTREGS->program_interrupt( HOSTREGS, PGM_PROTECTION_EXCEPTION );
    }
    else
#endif /* defined( _FEATURE_PROTECTION_INTERCEPTION_CONTROL ) */
        regs->program_interrupt (regs, PGM_PROTECTION_EXCEPTION);

vabs_prog_check:

    regs->program_interrupt (regs, regs->dat.xcode);

    return NULL; /* prevent warning from compiler */
} /* end function ARCH_DEP(logical_to_main_l) */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "dat.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "dat.c"
  #endif

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

RADR apply_host_prefixing( REGS* regs, RADR raddr )
{
    RADR aaddr = 0;
    switch (HOSTREGS->arch_mode)
    {
    case ARCH_370_IDX: aaddr = APPLY_370_PREFIXING( raddr, HOSTREGS->PX_370 ); break;
    case ARCH_390_IDX: aaddr = APPLY_390_PREFIXING( raddr, HOSTREGS->PX_390 ); break;
    case ARCH_900_IDX: aaddr = APPLY_900_PREFIXING( raddr, HOSTREGS->PX_900 ); break;
    default: CRASH();
    }
    return aaddr;
}

#endif /* !defined( _GEN_ARCH ) */
