/* INLINE.H     (C) Copyright Jan Jaeger, 1999-2012                  */
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
/* PER 1 GRA - Fish                                    Fish Jan 2022 */


#include "skey.h"       // Need storage key inline functions. Note
                        // that this #include MUST come BEFORE the
                        // the below _INLINE_H header guard to ensure
                        // it is compiled multiple times, AND that it
                        // get compiled BEFORE inline.h gets compiled,
                        // since some of the below inline.h functions
                        // may need to call a storage key function.
#ifndef _INLINE_H
#define _INLINE_H

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section of header: due to above _INLINE_H guard,    */
/*  the below code is compiled only once, BEFORE the  very first     */
/*  architecture is ever built.                                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                    Arithmetic functions                           */
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

#endif // defined( _INLINE_H )

/*-------------------------------------------------------------------*/
/*      ARCH_DEP section: the below section of this header           */
/*       is compiled multiple times, ONCE for each arch.             */
/*-------------------------------------------------------------------*/

//-------------------------------------------------------------------
//                       ARCH_DEP() code
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

#if (ARCH_370_IDX == ARCH_IDX && !defined( DID_370_INLINE_H )) \
 || (ARCH_390_IDX == ARCH_IDX && !defined( DID_390_INLINE_H )) \
 || (ARCH_900_IDX == ARCH_IDX && !defined( DID_900_INLINE_H ))

/*-------------------------------------------------------------------*/
/*                      logical_to_main_l                            */
/*-------------------------------------------------------------------*/
/*  All 3 build architecture variants of the below function must     */
/*  be declared at once (we can't wait for them to be declared later */
/*  on a subsequent pass when the next build architecture is built)  */
/*  since some of the below inline functions might need invoke the   */
/*  "SIE_TRANSLATE" macro, which invokes the "SIE_LOGICAL_TO_ABS"    */
/*  macro, which itself might need to call "logical_to_main_l" for   */
/*  a build architecture that is different from the one that is      */
/*  currently being built (or currently executing).                  */
/*-------------------------------------------------------------------*/
/*  NOTE TOO that they need to be declared HERE (and not in dat.h    */
/*  where they normally would go) since the below inline functions   */
/*  using "SIE_TRANSLATE" must be able to resolve their call to it.  */
/*  Note however that logical_to_main_l's actual implementation is   */
/*  still defined in dat.c where it logically belongs.               */
/*-------------------------------------------------------------------*/

DAT_DLL_IMPORT BYTE* s370_logical_to_main_l( U32 addr, int arn, REGS* regs, int acctype, BYTE akey, size_t len );
DAT_DLL_IMPORT BYTE* s390_logical_to_main_l( U32 addr, int arn, REGS* regs, int acctype, BYTE akey, size_t len );
DAT_DLL_IMPORT BYTE* z900_logical_to_main_l( U64 addr, int arn, REGS* regs, int acctype, BYTE akey, size_t len );

/*-------------------------------------------------------------------*/
/*                    Miscellaneous functions                        */
/*-------------------------------------------------------------------*/

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
/*      true = Fetch protected, false = Not fetch protected          */
/*-------------------------------------------------------------------*/
inline bool ARCH_DEP( is_fetch_protected )( VADR addr,
                                            BYTE skey, BYTE akey,
                                            REGS* regs )
{
    UNREFERENCED_370( addr );
    UNREFERENCED_370( regs );

    /* [3.4.1] Fetch is allowed if access key is zero, regardless
       of the storage key and fetch protection bit */
    /* [3.4.1] Fetch protection prohibits fetch if storage key fetch
       protect bit is on and access key does not match storage key */
    if (likely(0
               || akey == 0
               || akey == (skey & STORKEY_KEY)
               || !(skey & STORKEY_FETCH)
              )
    )
        return false;

#if defined( FEATURE_FETCH_PROTECTION_OVERRIDE )
    /* [3.4.1.2] Fetch protection override allows fetch from first
       2K of non-private address spaces if CR0 bit 6 is set */
    if (1
        && addr < 2048
        && (regs->CR(0) & CR0_FETCH_OVRD)
        && regs->dat.pvtaddr == 0
    )
        return false;
#endif

#if defined( FEATURE_STORAGE_PROTECTION_OVERRIDE )
    /* [3.4.1.1] Storage protection override allows access to
       locations with storage key 9, regardless of the access key,
       provided that CR0 bit 7 is set */
    if (1
        && (skey & STORKEY_KEY) == 0x90
        && (regs->CR(0) & CR0_STORE_OVRD)
    )
        return false;
#endif

    return true;    // (location *IS* fetch protected)

} /* end function is_fetch_protected */


/*-------------------------------------------------------------------*/
/* Test for low-address protection.                                  */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of storage location                  */
/*      regs    Pointer to the CPU register context                  */
/*      regs->dat.private  1=Location is in a private address space  */
/* Return value:                                                     */
/*      true = Is protected, false = Not protected                   */
/*-------------------------------------------------------------------*/
inline bool ARCH_DEP( is_low_address_protected )( VADR addr, REGS* regs )
{
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* For z/Arch, low-address protection applies to locations
       0-511 (0000-01FF) and also 4096-4607 (1000-11FF) */
    if (addr & 0xFFFFFFFFFFFFEE00ULL)
#else
    /* For S/370 and ESA/390, low-address protection applies
       only to locations 0-511 */
    if (addr > 511)
#endif
        return false;

    /* Low-address protection applies only if the low-address
       protection control bit in control register 0 is set */
    if (!(regs->CR(0) & CR0_LOW_PROT))
        return false;

#if defined( _FEATURE_SIE )
    /* Host low-address protection is not applied to guest
       references to guest storage */
    if (regs->sie_active)
        return false;
#endif

    /* Low-addr protection doesn't apply to private address spaces */
    if (regs->dat.pvtaddr)
        return false;

    return true;      // (location *IS* low-address protected)

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
/*      true = Store protected, false = Not store protected          */
/*-------------------------------------------------------------------*/
inline bool ARCH_DEP( is_store_protected )( VADR addr,
                                            BYTE skey, BYTE akey,
                                            REGS* regs )
{
    /* [3.4.4] Low-address protection prohibits stores into certain
       locations in the prefixed storage area of non-private address
       address spaces, if the low-address control bit in CR0 is set,
       regardless of the access key and storage key */
    if (ARCH_DEP( is_low_address_protected )( addr, regs ))
        return true;

    /* Access-list controlled protection prohibits all stores into
       the address space, and page protection prohibits all stores
       into the page, regardless of the access key and storage key */
    if (regs->dat.protect)
        return true;

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ) && HOSTREGS->dat.protect)
        return true;
#endif

    /* [3.4.1] Store is allowed if access key is zero, regardless
       of the storage key */
    if (akey == 0)
        return false;

#if defined( FEATURE_STORAGE_PROTECTION_OVERRIDE )
    /* [3.4.1.1] Storage protection override allows access to
       locations with storage key 9, regardless of the access key,
       provided that CR0 bit 7 is set */
    if (1
        && (skey & STORKEY_KEY) == 0x90
        && (regs->CR(0) & CR0_STORE_OVRD)
    )
        return false;
#endif

    /* [3.4.1] Store protection prohibits stores
       if the access key does not match the storage key */
    if (akey != (skey & STORKEY_KEY))
        return true;

    return false;      // (location is *NOT* store protected)

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

    /* Translate SIE host virt to SIE host abs. Note: macro
       is treated as a no-operation if SIE_MODE not active */
    SIE_TRANSLATE( &addr, ACCTYPE_READ, regs );

    /* Set the main storage reference bit */
    ARCH_DEP( or_storage_key )( addr, STORKEY_REF );

    /* Return absolute storage mainstor address */
    return (regs->mainstor + addr);

} /* end function fetch_main_absolute */

/*-------------------------------------------------------------------*/
/*  Generate a PER 1 General Register Alteration event interrupt     */
/*-------------------------------------------------------------------*/
static inline void ARCH_DEP( per1_gra )( REGS* regs )
{
#if !defined( FEATURE_PER1 )
    UNREFERENCED( regs );
#else
    OBTAIN_INTLOCK( regs );
    {
        regs->peradr = regs->periaddr;
        ON_IC_PER_GRA( regs );
    }
    RELEASE_INTLOCK( regs );

    if (OPEN_IC_PER_GRA( regs ))
        RETURN_INTCHECK( regs );
#endif
}

/*-------------------------------------------------------------------*/
/* Determine whether specified PER event should be suppressed or not */
/*-------------------------------------------------------------------*/
inline bool ARCH_DEP( is_per3_event_suppressed )( REGS* regs, U32 cr9_per_event )
{
#if !defined( FEATURE_PER3 )
    UNREFERENCED( regs );
    UNREFERENCED( cr9_per_event );
#else
    /* DON'T suppress this event if Event Suppression isn't enabled */
    if (!(regs->CR_L(9) & CR9_SUPPRESS))
        return false;

    /* Is this PER event one that is ALLOWED to be suppressed? */
    if (cr9_per_event & CR9_SUPPRESSABLE)
    {
        /* Is there an active transaction? */
        if (regs->txf_tnd)
            return true;        /* Yes, then suppress it! */

        /* Suppress instruction-fetch events for TBEGIN/TBEGINC */
        if (1
            && cr9_per_event & CR9_IF
            && *(regs->ip+0) == 0xE5
            && (0
                || *(regs->ip+1) == 0x60
                || *(regs->ip+1) == 0x61
               )
        )
            return true;        /* TBEGIN/TBEGINC: suppress it! */
    }
#endif
    /* Otherwise DON'T suppress this PER event */
    return false;
}

/*-------------------------------------------------------------------*/
/*     Generate a PER 3 Zero-Address Detection event interrupt       */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( per3_zero )( REGS* regs )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (1
        && EN_IC_PER_ZEROADDR( regs )
        && !IS_PER_SUPRESS( regs, CR9_ZEROADDR )
    )
    {
        regs->peradr = regs->periaddr;
        ON_IC_PER_ZEROADDR( regs );
        if (OPEN_IC_PER_ZEROADDR( regs ))
            RETURN_INTCHECK( regs );
    }
#else
    UNREFERENCED( regs );
#endif
}

/*-------------------------------------------------------------------*/
/* The CHECK macros are designed for instructions where the operand  */
/* address is specified in a register but the operand length is not  */
/* (or is otherwise implied, perhaps by a function code in another   */
/* operand register for example), and/or for instructions where it   */
/* is otherwise unpredictable whether operand data will actually be  */
/* accessed or not. Thus the only thing we can check is whether the  */
/* register holding the address of the operand is zero or not.       */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( per3_zero_check )( REGS* regs, int r1 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (GR_A( r1, regs ) == 0)
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( r1 );
#endif
}

inline void ARCH_DEP( per3_zero_check2 )( REGS* regs, int r1, int r2 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || GR_A( r1, regs ) == 0
        || GR_A( r2, regs ) == 0
    )
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( r1 );
    UNREFERENCED( r2 );
#endif
}

/*-------------------------------------------------------------------*/
/*  The LCHECK macros are designed for RR and RRE and similar format */
/*  type instructions where a PER Zero-Address event does NOT occur  */
/*  when the operand length (specified in another register) is zero, */
/*  thus causing that operand's storage to never be accessed. Note   */
/*  that all 32 (or 64) bits of the length register are checked.     */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( per3_zero_lcheck )( REGS* regs, int r1, int l1 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (1
        && GR_A( l1, regs ) != 0
        && GR_A( r1, regs ) == 0
    )
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( r1 );
    UNREFERENCED( l1 );
#endif
}

inline void ARCH_DEP( per3_zero_lcheck2 )( REGS* regs, int r1, int l1, int r2, int l2 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || (1
            && GR_A( l1, regs ) != 0
            && GR_A( r1, regs ) == 0
           )
        || (1
            && GR_A( l2, regs ) != 0
            && GR_A( r2, regs ) == 0
           )
    )
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( r1 );
    UNREFERENCED( l1 );
    UNREFERENCED( r2 );
    UNREFERENCED( l2 );
#endif
}

/*-------------------------------------------------------------------*/
/*  The L24CHECK macros are identical to the LCHECK macros except    */
/*  for the operand length check: instead of checking all 32 or 64   */
/*  bits of the register containing the operand length, we instead   */
/*  check only the low-order 24 bits of the length register via the  */
/*  GR_LA24 macro. They are designed for MVCL and CLCL and other     */
/*  similar type instructions.                                       */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( per3_zero_l24check )( REGS* regs, int r1, int l1 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (1
        && regs->GR_LA24( l1 ) != 0
        && GR_A( r1, regs )    == 0
    )
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( r1 );
    UNREFERENCED( l1 );
#endif
}

inline void ARCH_DEP( per3_zero_l24check2 )( REGS* regs, int r1, int l1, int r2, int l2 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || (1
            && regs->GR_LA24( l1 ) != 0
            && GR_A( r1, regs )    == 0
           )
        || (1
            && regs->GR_LA24( l2 ) != 0
            && GR_A( r2, regs )    == 0
           )
    )
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( r1 );
    UNREFERENCED( l1 );
    UNREFERENCED( r2 );
    UNREFERENCED( l2 );
#endif
}

/*-------------------------------------------------------------------*/
/*  The XCHECK macros are designed for RS/RX/S and similar format    */
/*  type instructions where a PER Zero-Address event does NOT occur  */
/*  unless the specified base or index register number is non-zero.  */
/*  When the base or index register number is specified as zero, it  */
/*  is not used in effective address calculations and thus PER Zero  */
/*  Address Detection does not apply for that register.              */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( per3_zero_xcheck )( REGS* regs, int b1 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (b1 && GR_A( b1, regs ) == 0)
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( b1 );
#endif
}

inline void ARCH_DEP( per3_zero_xcheck2 )( REGS* regs, int x2, int b2 )
{
#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || (!b2 && x2 && GR_A( x2, regs ) == 0)
        || (!x2 && b2 && GR_A( b2, regs ) == 0)
        || ( b2 && x2 && (0
                          || GR_A( b2, regs ) == 0
                          || GR_A( b2, regs ) + GR_A( x2, regs ) == 0
                         )
           )
    )
        ARCH_DEP( per3_zero )( regs );
#else
    UNREFERENCED( regs );
    UNREFERENCED( x2 );
    UNREFERENCED( b2 );
#endif
}

/*-------------------------------------------------------------------*/
/* Program check if fpc is not valid contents for FPC register       */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( FPC_check )( REGS* regs, U32 fpc )
{
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
    {
        if (0
            || (fpc & FPC_RESV_FPX)
            || (fpc & FPC_BRM_3BIT) == BRM_RESV4
            || (fpc & FPC_BRM_3BIT) == BRM_RESV5
            || (fpc & FPC_BRM_3BIT) == BRM_RESV6
        )
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if (fpc & FPC_RESERVED)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }
}

/*-------------------------------------------------------------------*/
/* Fetch a doubleword from absolute storage.                         */
/* The caller is assumed to have already checked that the absolute   */
/* address is within the limit of main storage.                      */
/* All bytes of the word are fetched concurrently as observed by     */
/* other CPUs.  The doubleword is first fetched as an integer, then  */
/* the bytes are reversed into host byte order if necessary.         */
/*-------------------------------------------------------------------*/
inline U64 ARCH_DEP( fetch_doubleword_absolute )( RADR addr, REGS* regs )
{
    // The change below affects 32 bit hosts that use something like
    // cmpxchg8b to fetch the doubleword concurrently.
    // This routine is mainly called by DAT in 64 bit guest mode
    // to access DAT-related values.  In most `well-behaved' OS's,
    // other CPUs should not be interfering with these values

    return fetch_dw( FETCH_MAIN_ABSOLUTE( addr, regs, 8 ));
}


/*-------------------------------------------------------------------*/
/* Fetch a fullword from absolute storage.                           */
/* The caller is assumed to have already checked that the absolute   */
/* address is within the limit of main storage.                      */
/* All bytes of the word are fetched concurrently as observed by     */
/* other CPUs.  The fullword is first fetched as an integer, then    */
/* the bytes are reversed into host byte order if necessary.         */
/*-------------------------------------------------------------------*/
inline U32 ARCH_DEP( fetch_fullword_absolute )( RADR addr, REGS* regs )
{
    return fetch_fw( FETCH_MAIN_ABSOLUTE( addr, regs, 4 ));
}


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
    return fetch_hw( FETCH_MAIN_ABSOLUTE( addr, regs, 2 ));
}


/*-------------------------------------------------------------------*/
/* Store doubleword into absolute storage.                           */
/* All bytes of the word are stored concurrently as observed by      */
/* other CPUs.  The bytes of the word are reversed if necessary      */
/* and the word is then stored as an integer in absolute storage.    */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( store_doubleword_absolute )( U64 value,
                                                   RADR addr, REGS* regs )
{
#if defined( INLINE_STORE_FETCH_ADDR_CHECK )
    if (addr > regs->mainlim - 8)
        regs->program_interrupt( regs, PGM_ADDRESSING_EXCEPTION );
#endif

    /* Translate SIE host virt to SIE host abs. Note: macro
       is treated as a no-operation if SIE_MODE not active */
    SIE_TRANSLATE( &addr, ACCTYPE_WRITE, regs );

    /* Set the main storage reference and change bits */
    ARCH_DEP( or_storage_key )( addr, (STORKEY_REF | STORKEY_CHANGE) );

    /* Store the doubleword into absolute storage */
    store_dw( regs->mainstor + addr, value );

} /* end function store_doubleword_absolute */


/*-------------------------------------------------------------------*/
/* Store a fullword into absolute storage.                           */
/* All bytes of the word are stored concurrently as observed by      */
/* other CPUs.  The bytes of the word are reversed if necessary      */
/* and the word is then stored as an integer in absolute storage.    */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( store_fullword_absolute )( U32   value,
                                                 RADR  addr,
                                                 REGS* regs )
{
#if defined( INLINE_STORE_FETCH_ADDR_CHECK )
    if (addr > regs->mainlim - 4)
        regs->program_interrupt( regs, PGM_ADDRESSING_EXCEPTION );
#endif

    /* Translate SIE host virt to SIE host abs. Note: macro
       is treated as a no-operation if SIE_MODE not active */
    SIE_TRANSLATE( &addr, ACCTYPE_WRITE, regs );

    /* Set the main storage reference and change bits */
    ARCH_DEP( or_storage_key )( addr, (STORKEY_REF | STORKEY_CHANGE) );

    /* Store the fullword into absolute storage */
    store_fw( regs->mainstor + addr, value );

} /* end function store_fullword_absolute */


/*-------------------------------------------------------------------*/
/*          Automatically #include other needed headers              */
/*-------------------------------------------------------------------*/

#include "dat.h"
#include "vstore.h"


/*-------------------------------------------------------------------*/
/*      We only need to compile the above section of this header     */
/*                only ONCE for each architecture                    */
/*-------------------------------------------------------------------*/

#if      ARCH_370_IDX == ARCH_IDX
  #define DID_370_INLINE_H
#endif
#if      ARCH_390_IDX == ARCH_IDX
  #define DID_390_INLINE_H
#endif
#if      ARCH_900_IDX == ARCH_IDX
  #define DID_900_INLINE_H
#endif

#endif // #if (ARCH_xxx_IDX == ARCH_IDX && !defined( DID_xxx_INLINE_H )) ...

/*-------------------------------------------------------------------*/
/*      This section of the header is compiled ONLY ONCE             */
/*      after *ALL* build architecture FEATURES are defined          */
/*-------------------------------------------------------------------*/

// (there is currently no code in this section of the header)

/* end of INLINE.H */
