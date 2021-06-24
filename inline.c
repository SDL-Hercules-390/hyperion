/* INLINE.C     (C) Copyright Jan Jaeger, 1999-2012                  */
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

#include "hstdinc.h"

#define _INLINE_C_
#define _HENGINE_DLL_

#include "hercules.h"
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
/* Test for fetch protected storage location.                        */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of storage location                  */
/*      skey    Storage key with fetch, reference, and change bits   */
/*              and one low-order zero appended                      */
/*      akey    Access key with 4 low-order zeroes appended          */
/*      regs    Pointer to the CPU register context                  */
/*      regs->dat.private  1=Location is in a private address space  */
/* Return value:                                                     */
/*      1=Fetch protected, 0=Not fetch protected                     */
/*-------------------------------------------------------------------*/
inline int ARCH_DEP(is_fetch_protected) (VADR addr, BYTE skey,
                    BYTE akey, REGS *regs)
{
    UNREFERENCED_370(addr);
    UNREFERENCED_370(regs);

    /* [3.4.1] Fetch is allowed if access key is zero, regardless
       of the storage key and fetch protection bit */
    /* [3.4.1] Fetch protection prohibits fetch if storage key fetch
       protect bit is on and access key does not match storage key */
    if (likely(akey == 0
    || akey == (skey & STORKEY_KEY)
    || !(skey & STORKEY_FETCH)))
    return 0;

#ifdef FEATURE_FETCH_PROTECTION_OVERRIDE
    /* [3.4.1.2] Fetch protection override allows fetch from first
       2K of non-private address spaces if CR0 bit 6 is set */
    if (addr < 2048
    && (regs->CR(0) & CR0_FETCH_OVRD)
    && regs->dat.pvtaddr == 0)
    return 0;
#endif /*FEATURE_FETCH_PROTECTION_OVERRIDE*/

#ifdef FEATURE_STORAGE_PROTECTION_OVERRIDE
    /* [3.4.1.1] Storage protection override allows access to
       locations with storage key 9, regardless of the access key,
       provided that CR0 bit 7 is set */
    if ((skey & STORKEY_KEY) == 0x90
    && (regs->CR(0) & CR0_STORE_OVRD))
    return 0;
#endif /*FEATURE_STORAGE_PROTECTION_OVERRIDE*/

    /* Return one if location is fetch protected */
    return 1;

} /* end function is_fetch_protected */

/*-------------------------------------------------------------------*/
/* Test for low-address protection.                                  */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of storage location                  */
/*      regs    Pointer to the CPU register context                  */
/*      regs->dat.private  1=Location is in a private address space  */
/* Return value:                                                     */
/*      1=Low-address protected, 0=Not low-address protected         */
/*-------------------------------------------------------------------*/
inline int ARCH_DEP(is_low_address_protected) (VADR addr,
                                              REGS *regs)
{
#if defined (FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* For ESAME, low-address protection applies to locations
       0-511 (0000-01FF) and 4096-4607 (1000-11FF) */
    if (addr & 0xFFFFFFFFFFFFEE00ULL)
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    /* For S/370 and ESA/390, low-address protection applies
       to locations 0-511 only */
    if (addr > 511)
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        return 0;

    /* Low-address protection applies only if the low-address
       protection control bit in control register 0 is set */
    if ((regs->CR(0) & CR0_LOW_PROT) == 0)
        return 0;

#if defined(_FEATURE_SIE)
    /* Host low-address protection is not applied to guest
       references to guest storage */
    if (regs->sie_active)
        return 0;
#endif /*defined(_FEATURE_SIE)*/

    /* Low-address protection does not apply to private address
       spaces */
    if (regs->dat.pvtaddr)
        return 0;

    /* Return one if location is low-address protected */
    return 1;

} /* end function is_low_address_protected */

/*-------------------------------------------------------------------*/
/* Test for store protected storage location.                        */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of storage location                  */
/*      skey    Storage key with fetch, reference, and change bits   */
/*              and one low-order zero appended                      */
/*      akey    Access key with 4 low-order zeroes appended          */
/*      regs    Pointer to the CPU register context                  */
/*      regs->dat.private  1=Location is in a private address space  */
/*      regs->dat.protect  1=Access list protected or page protected */
/* Return value:                                                     */
/*      1=Store protected, 0=Not store protected                     */
/*-------------------------------------------------------------------*/
inline int ARCH_DEP(is_store_protected) (VADR addr, BYTE skey,
               BYTE akey, REGS *regs)
{
    /* [3.4.4] Low-address protection prohibits stores into certain
       locations in the prefixed storage area of non-private address
       address spaces, if the low-address control bit in CR0 is set,
       regardless of the access key and storage key */
    if (ARCH_DEP(is_low_address_protected) (addr, regs))
        return 1;

    /* Access-list controlled protection prohibits all stores into
       the address space, and page protection prohibits all stores
       into the page, regardless of the access key and storage key */
    if (regs->dat.protect)
        return 1;
#if defined(_FEATURE_SIE)
    if(SIE_MODE(regs) && HOSTREGS->dat.protect)
        return 1;
#endif

    /* [3.4.1] Store is allowed if access key is zero, regardless
       of the storage key */
    if (akey == 0)
        return 0;

#ifdef FEATURE_STORAGE_PROTECTION_OVERRIDE
    /* [3.4.1.1] Storage protection override allows access to
       locations with storage key 9, regardless of the access key,
       provided that CR0 bit 7 is set */
    if ((skey & STORKEY_KEY) == 0x90
        && (regs->CR(0) & CR0_STORE_OVRD))
        return 0;
#endif /*FEATURE_STORAGE_PROTECTION_OVERRIDE*/

    /* [3.4.1] Store protection prohibits stores if the access
       key does not match the storage key */
    if (akey != (skey & STORKEY_KEY))
        return 1;

    /* Return zero if location is not store protected */
    return 0;

} /* end function is_store_protected */


/*-------------------------------------------------------------------*/
/* Return mainstor address of absolute address.                      */
/* The caller is assumed to have already checked that the absolute   */
/* address is within the limit of main storage.                      */
/*-------------------------------------------------------------------*/
#if defined( INLINE_STORE_FETCH_ADDR_CHECK )   // (see inline.h)
inline BYTE* ARCH_DEP( fetch_main_absolute )( RADR addr, REGS* regs, int len )
#else
inline BYTE* ARCH_DEP( fetch_main_absolute )( RADR addr, REGS* regs )
#endif
{
#if defined( INLINE_STORE_FETCH_ADDR_CHECK )
    if (addr > (regs->mainlim - len))
        regs->program_interrupt( regs, PGM_ADDRESSING_EXCEPTION );
#endif

    SIE_TRANSLATE( &addr, ACCTYPE_READ, regs );

    /* Set the main storage reference bit */
    STORAGE_KEY(addr, regs) |= STORKEY_REF;

    /* Return absolute storage mainstor address */
    return (regs->mainstor + addr);

} /* end function fetch_main_absolute */


/*-------------------------------------------------------------------*/
/* Fetch a doubleword from absolute storage.                         */
/* The caller is assumed to have already checked that the absolute   */
/* address is within the limit of main storage.                      */
/* All bytes of the word are fetched concurrently as observed by     */
/* other CPUs.  The doubleword is first fetched as an integer, then  */
/* the bytes are reversed into host byte order if necessary.         */
/*-------------------------------------------------------------------*/
inline U64 ARCH_DEP(fetch_doubleword_absolute) (RADR addr,
                                REGS *regs)
{
 // The change below affects 32 bit hosts that use something like
 // cmpxchg8b to fetch the doubleword concurrently.
 // This routine is mainly called by DAT in 64 bit guest mode
 // to access DAT-related values.  In most `well-behaved' OS's,
 // other CPUs should not be interfering with these values
  return fetch_dw(FETCH_MAIN_ABSOLUTE(addr, regs, 8));
} /* end function fetch_doubleword_absolute */


/*-------------------------------------------------------------------*/
/* Fetch a fullword from absolute storage.                           */
/* The caller is assumed to have already checked that the absolute   */
/* address is within the limit of main storage.                      */
/* All bytes of the word are fetched concurrently as observed by     */
/* other CPUs.  The fullword is first fetched as an integer, then    */
/* the bytes are reversed into host byte order if necessary.         */
/*-------------------------------------------------------------------*/
inline U32 ARCH_DEP(fetch_fullword_absolute) (RADR addr,
                                REGS *regs)
{
    return fetch_fw(FETCH_MAIN_ABSOLUTE(addr, regs, 4));
} /* end function fetch_fullword_absolute */


/*-------------------------------------------------------------------*/
/* Fetch a halfword from absolute storage.                           */
/* The caller is assumed to have already checked that the absolute   */
/* address is within the limit of main storage.                      */
/* All bytes of the halfword are fetched concurrently as observed by */
/* other CPUs.  The halfword is first fetched as an integer, then    */
/* the bytes are reversed into host byte order if necessary.         */
/*-------------------------------------------------------------------*/
inline U16 ARCH_DEP(fetch_halfword_absolute) (RADR addr,
                                REGS *regs)
{
    return fetch_hw(FETCH_MAIN_ABSOLUTE(addr, regs, 2));
} /* end function fetch_halfword_absolute */


/*-------------------------------------------------------------------*/
/* Store doubleword into absolute storage.                           */
/* All bytes of the word are stored concurrently as observed by      */
/* other CPUs.  The bytes of the word are reversed if necessary      */
/* and the word is then stored as an integer in absolute storage.    */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP(store_doubleword_absolute) (U64 value,
                          RADR addr, REGS *regs)
{
#if defined(INLINE_STORE_FETCH_ADDR_CHECK)
    if(addr > regs->mainlim - 8)
        regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);
#endif /*defined(INLINE_STORE_FETCH_ADDR_CHECK)*/

    SIE_TRANSLATE(&addr, ACCTYPE_WRITE, regs);

    /* Set the main storage reference and change bits */
    STORAGE_KEY(addr, regs) |= (STORKEY_REF | STORKEY_CHANGE);

    /* Store the doubleword into absolute storage */
    store_dw(regs->mainstor + addr, value);

} /* end function store_doubleword_absolute */


/*-------------------------------------------------------------------*/
/* Store a fullword into absolute storage.                           */
/* All bytes of the word are stored concurrently as observed by      */
/* other CPUs.  The bytes of the word are reversed if necessary      */
/* and the word is then stored as an integer in absolute storage.    */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP(store_fullword_absolute) (U32 value,
                          RADR addr, REGS *regs)
{
#if defined(INLINE_STORE_FETCH_ADDR_CHECK)
    if(addr > regs->mainlim - 4)
        regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);
#endif /*defined(INLINE_STORE_FETCH_ADDR_CHECK)*/

    SIE_TRANSLATE(&addr, ACCTYPE_WRITE, regs);

    /* Set the main storage reference and change bits */
    STORAGE_KEY(addr, regs) |= (STORKEY_REF | STORKEY_CHANGE);

    /* Store the fullword into absolute storage */
    store_fw(regs->mainstor + addr, value);

} /* end function store_fullword_absolute */

inline RADR ARCH_DEP( apply_prefixing )( RADR addr, RADR px )
{
    return APPLY_PREFIXING( addr, px );
}

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
/* Add two unsigned fullwords giving an unsigned fullword result     */
/* and return the condition code for the AL or ALR instruction       */
/*-------------------------------------------------------------------*/
inline int add_logical( U32* result, U32 op1, U32 op2 )
{
    *result = op1 + op2;
    return (*result == 0 ? 0 : 1) | (op1 > *result ? 2 : 0);
}

/*-------------------------------------------------------------------*/
/* Subtract two unsigned fullwords giving unsigned fullword result   */
/* and return the condition code for the SL or SLR instruction       */
/*-------------------------------------------------------------------*/
inline int sub_logical( U32* result, U32 op1, U32 op2 )
{
    *result = op1 - op2;
    return (*result == 0 ? 0 : 1) | (op1 < *result ? 0 : 2);
}

/*-------------------------------------------------------------------*/
/* Add two signed fullwords giving a signed fullword result          */
/* and return the condition code for the A or AR instruction         */
/*-------------------------------------------------------------------*/
inline int add_signed( U32* result, U32 op1, U32 op2 )
{
    S32 sres, sop1, sop2;

    /* NOTE: cannot use casting here as signed fixed point overflow
       leads to undefined behavior! (whereas unsigned doesn't)
    */
    *result = op1 + op2;

    sres = (S32) *result;
    sop1 = (S32) op1;
    sop2 = (S32) op2;

    return
    (0
        || (sop2 > 0 && sop1 > (INT_MAX - sop2))
        || (sop2 < 0 && sop1 < (INT_MIN - sop2))
    )
    ? 3 : (sres < 0 ? 1 : (sres > 0 ? 2 : 0));
}

/*-------------------------------------------------------------------*/
/* Subtract two signed fullwords giving a signed fullword result     */
/* and return the condition code for the S or SR instruction         */
/*-------------------------------------------------------------------*/
inline int sub_signed( U32* result, U32 op1, U32 op2 )
{
    S32 sres, sop1, sop2;

    /* NOTE: cannot use casting here as signed fixed point overflow
       leads to undefined behavior! (whereas unsigned doesn't)
    */
    *result = op1 - op2;

    sres = (S32) *result;
    sop1 = (S32) op1;
    sop2 = (S32) op2;

    return
    (0
        || (sop2 < 0 && sop1 > (INT_MAX + sop2))
        || (sop2 > 0 && sop1 < (INT_MIN + sop2))
    )
    ? 3 : (sres < 0 ? 1 : (sres > 0 ? 2 : 0));
}

/*-------------------------------------------------------------------*/
/* Multiply two signed fullwords giving a signed doubleword result   */
/*-------------------------------------------------------------------*/
inline void mul_signed( U32* resulthi, U32* resultlo, U32 op1, U32  op2 )
{
    S64 r = (S64)(S32)op1 * (S32)op2;
    *resulthi = (U32)((U64)r >> 32);
    *resultlo = (U32)((U64)r & 0xFFFFFFFF);
}

/*-------------------------------------------------------------------*/
/* Multiply two unsigned doublewords giving unsigned 128-bit result  */
// https://stackoverflow.com/questions/31652875/fastest-way-to-multiply-two-64-bit-ints-to-128-bit-then-to-64-bit
/*-------------------------------------------------------------------*/
inline void mul_unsigned_long( U64* resulthi, U64* resultlo, U64 op1, U64 op2 )
{
    U64  a_lo  =  (U64)(U32)op1;
    U64  a_hi  =  op1 >> 32;
    U64  b_lo  =  (U64)(U32)op2;
    U64  b_hi  =  op2 >> 32;

    U64  p0    =  a_lo * b_lo;
    U64  p1    =  a_lo * b_hi;
    U64  p2    =  a_hi * b_lo;
    U64  p3    =  a_hi * b_hi;

    U32  cy    =  (U32)(((p0 >> 32) + (U32)p1 + (U32)p2) >> 32);

    *resultlo  =  p0 + (p1 << 32) + (p2 << 32);
    *resulthi  =  p3 + (p1 >> 32) + (p2 >> 32) + cy;
}

/*-------------------------------------------------------------------*/
/* Multiply two signed doublewords giving a signed 128-bit result    */
// https://stackoverflow.com/questions/31652875/fastest-way-to-multiply-two-64-bit-ints-to-128-bit-then-to-64-bit
/*-------------------------------------------------------------------*/
inline void mul_signed_long( S64* resulthi, S64* resultlo, S64 op1, S64 op2 )
{
    mul_unsigned_long( (U64*)resulthi, (U64*)resultlo,
                       (U64) op1,      (U64) op2 );

    if (op1 < 0LL)  *resulthi  -=  op2;
    if (op2 < 0LL)  *resulthi  -=  op1;
}

/*-------------------------------------------------------------------*/
/* Divide a signed doubleword dividend by a signed fullword divisor  */
/* giving a signed fullword remainder and a signed fullword quotient.*/
/* Returns 0 if successful, 1 if divide overflow.                    */
/*-------------------------------------------------------------------*/
inline int div_signed( U32* rem, U32* quot, U32 high, U32 lo, U32 d )
{
U64 dividend;
S64 quotient, remainder;

    if (d == 0) return 1;
    dividend = (U64)high << 32 | lo;
    quotient = (S64)dividend / (S32)d;
    remainder = (S64)dividend % (S32)d;
    if (quotient < -2147483648LL || quotient > 2147483647LL) return 1;
    *quot = (U32)quotient;
    *rem = (U32)remainder;
    return 0;
}

/*-------------------------------------------------------------------*/
/* Divide an unsigned 128-bit dividend by an unsigned 64-bit divisor */
/* giving unsigned 64-bit remainder and unsigned 64-bit quotient.    */
/* Returns 0 if successful, 1 if divide overflow.                    */
/*-------------------------------------------------------------------*/
inline int div_logical_long( U64* rem, U64* quot, U64 high, U64 lo, U64 d )
{
    int i;

    *quot = 0;
    if (high >= d) return 1;
    for (i = 0; i < 64; i++)
    {
    int ovf;
        ovf = high >> 63;
        high = (high << 1) | (lo >> 63);
        lo <<= 1;
        *quot <<= 1;
        if (high >= d || ovf)
        {
            *quot += 1;
            high -= d;
        }
    }
    *rem = high;
    return 0;
}

/*-------------------------------------------------------------------*/
/* Add two unsigned doublewords giving an unsigned doubleword result */
/* and return the condition code for the ALG or ALGR instruction     */
/*-------------------------------------------------------------------*/
inline int add_logical_long( U64* result, U64 op1, U64 op2 )
{
    *result = op1 + op2;
    return (*result == 0 ? 0 : 1) | (op1 > *result ? 2 : 0);
}

/*-------------------------------------------------------------------*/
/* Subtract unsigned doublewords giving unsigned doubleword result   */
/* and return the condition code for the SLG or SLGR instruction     */
/*-------------------------------------------------------------------*/
inline int sub_logical_long( U64* result, U64 op1, U64 op2 )
{
    *result = op1 - op2;
    return (*result == 0 ? 0 : 1) | (op1 < *result ? 0 : 2);
}

/*-------------------------------------------------------------------*/
/* Add two signed doublewords giving a signed doubleword result      */
/* and return the condition code for the AG or AGR instruction       */
/*-------------------------------------------------------------------*/
inline int add_signed_long( U64* result, U64 op1, U64 op2 )
{
    S64 sres, sop1, sop2;

    /* NOTE: cannot use casting here as signed fixed point overflow
       leads to undefined behavior! (whereas unsigned doesn't)
    */
    *result = op1 + op2;

    sres = (S64) *result;
    sop1 = (S64) op1;
    sop2 = (S64) op2;

    return
    (0
        || (sop2 > 0 && sop1 > (LLONG_MAX - sop2))
        || (sop2 < 0 && sop1 < (LLONG_MIN - sop2))
    )
    ? 3 : (sres < 0 ? 1 : (sres > 0 ? 2 : 0));
}

/*-------------------------------------------------------------------*/
/* Subtract two signed doublewords giving signed doubleword result   */
/* and return the condition code for the SG or SGR instruction       */
/*-------------------------------------------------------------------*/
inline int sub_signed_long( U64* result, U64 op1, U64 op2 )
{
    S64 sres, sop1, sop2;

    /* NOTE: cannot use casting here as signed fixed point overflow
       leads to undefined behavior! (whereas unsigned doesn't)
    */
    *result = op1 - op2;

    sres = (S64) *result;
    sop1 = (S64) op1;
    sop2 = (S64) op2;

    return
    (0
        || (sop2 < 0 && sop1 > (LLONG_MAX + sop2))
        || (sop2 > 0 && sop1 < (LLONG_MIN + sop2))
    )
    ? 3 : (sres < 0 ? 1 : (sres > 0 ? 2 : 0));
}

/*-------------------------------------------------------------------*/
/* Multiply two unsigned doublewords giving unsigned 128-bit result  */
/*-------------------------------------------------------------------*/
inline int mult_logical_long( U64* high, U64* lo, U64 md, U64 mr )
{
    int i;

    *high = 0; *lo = 0;
    for (i = 0; i < 64; i++)
    {
    U64 ovf;
        ovf = *high;
        if (md & 1)
            *high += mr;
        md >>= 1;
        *lo = (*lo >> 1) | (*high << 63);
        if(ovf > *high)
            *high = (*high >> 1) | 0x8000000000000000ULL;
        else
            *high >>= 1;
    }
    return 0;
}

#if defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY )
/*-------------------------------------------------------------------*/
/*     "Should Storage Key Update be BYPASSED?" helper function      */
/*-------------------------------------------------------------------*/
/*  Common Conditional-SSKE Facility decision-making logic used by   */
/*  both the SSKE instruction as well as the PFMF instruction.       */
/*                                                                   */
/*      regs    Register context                                     */
/*      m3      Operand-3 mask field from SSKE instruction,          */
/*              or PFMF instruction r1 register bits 52-55.          */
/*      oldkey  Contents of storage key before modification          */
/*      r1key   Register r1 storage key comparison value             */
/*                                                                   */
/*  Returns:                                                         */
/*                                                                   */
/*      true    Updating of storage key *SHOULD* be bypassed         */
/*              (i.e. do *NOT* update the storage key)               */
/*                                                                   */
/*      false   Updating of storage key should *NOT* be bypassed     */
/*              (i.e. perform normal storage key updating)           */
/*                                                                   */
/*-------------------------------------------------------------------*/
inline bool bypass_skey_update( REGS* regs, BYTE m3, BYTE oldkey, BYTE r1key )
{
    /* If the Conditional-SSKE Facility is not installed, or if both
       the MR and MC bits are both zero, OR if storage key and fetch
       bits are not the same as the values in R1 register bits 56-60,
       then update the key normally (i.e. do NOT bypass updating key).
    */
    if (0
        || !FACILITY_ENABLED( 010_CONDITIONAL_SSKE, regs )
        || (m3 & (SSKE_MASK_MR | SSKE_MASK_MC)) == 0
        || (oldkey & (STORKEY_KEY | STORKEY_FETCH)) !=
           (r1key  & (STORKEY_KEY | STORKEY_FETCH))
    )
        return false; /* (DON'T bypass updating this page's key) */

    /* If both the MR and MC mask bits are both one, or the MR bit is
       zero and the reference bit is equal to bit 61 of register r1,
       or the MC bit is zero and the change bit is equal to bit 62 of
       register r1, then BYPASS updating (do NOT update) storage key.
    */
    if (0
        || ((m3 & (SSKE_MASK_MR | SSKE_MASK_MC))
              ==  (SSKE_MASK_MR | SSKE_MASK_MC))
        || ((m3 & SSKE_MASK_MR) == 0 && (oldkey & STORKEY_REF   ) == (r1key & STORKEY_REF   ))
        || ((m3 & SSKE_MASK_MC) == 0 && (oldkey & STORKEY_CHANGE) == (r1key & STORKEY_CHANGE))
    )
        return true; /***>>  BYPASS updating this page's key <<***/

    return false;     /* (DON'T bypass updating this page's key) */
}
#endif // defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY )


#endif /*!defined( _GEN_ARCH )*/
