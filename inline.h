/* INLINE.H     (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
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

// #define INLINE_STORE_FETCH_ADDR_CHECK

#if defined( FEATURE_ACCESS_REGISTERS )
U16 ARCH_DEP(translate_alet) (U32 alet, U16 eax,
        int acctype, REGS *regs, U32 *asteo, U32 aste[]);
void ARCH_DEP(purge_alb_all) ();
void ARCH_DEP(purge_alb) (REGS *regs);
#endif

int ARCH_DEP(translate_addr) (VADR vaddr, int arn,
        REGS *regs, int acctype);
void ARCH_DEP(purge_tlb_all) ();
void ARCH_DEP(purge_tlb) (REGS *regs);
void ARCH_DEP(purge_tlbe_all) (RADR pfra);
void ARCH_DEP(purge_tlbe) (REGS *regs, RADR pfra);
void ARCH_DEP(invalidate_tlb) (REGS *regs, BYTE mask);

#if ARCH_IDX == ARCH_390_IDX && defined( _900 )
void z900_invalidate_tlb (REGS *regs, BYTE mask);
#endif

void ARCH_DEP(invalidate_tlbe) (REGS *regs, BYTE *main);
void ARCH_DEP(invalidate_pte) (BYTE ibyte, RADR op1,
        U32 op2, REGS *regs);
_LOGICAL_C_STATIC BYTE *ARCH_DEP(logical_to_main) (VADR addr, int arn,
        REGS *regs, int acctype, BYTE akey);
_LOGICAL_C_STATIC BYTE *ARCH_DEP(logical_to_main_l) (VADR addr, int arn,
        REGS *regs, int acctype, BYTE akey, size_t len);

#if defined( _FEATURE_SIE ) && ARCH_IDX != ARCH_900_IDX
_LOGICAL_C_STATIC BYTE *s390_logical_to_main (U32 addr, int arn, REGS *regs,
        int acctype, BYTE akey);
int s390_translate_addr (U32 vaddr, int arn, REGS *regs,
        int acctype);
static inline U64 s390_apply_prefixing( U64 raddr, U64 px );
#endif

#if defined( _FEATURE_ZSIE )
_LOGICAL_C_STATIC BYTE *z900_logical_to_main (U64 addr, int arn, REGS *regs,
        int acctype, BYTE akey);
int z900_translate_addr (U64 vaddr, int arn, REGS *regs,
        int acctype);
static inline U64 z900_apply_prefixing( U64 raddr, U64 px );
#endif

_VSTORE_C_STATIC void ARCH_DEP(vstorec) (const void *src, BYTE len,
        VADR addr, int arn, REGS *regs);
_VSTORE_C_STATIC void ARCH_DEP(vstoreb) (BYTE value, VADR addr,
        int arn, REGS *regs);
_VSTORE_C_STATIC void ARCH_DEP(vstore2) (U16 value, VADR addr, int arn,
        REGS *regs);
_VSTORE_C_STATIC void ARCH_DEP(vstore4) (U32 value, VADR addr, int arn,
        REGS *regs);
_VSTORE_C_STATIC void ARCH_DEP(vstore8) (U64 value, VADR addr, int arn,
        REGS *regs);
_VSTORE_C_STATIC void ARCH_DEP(vfetchc) (void *dest, BYTE len,
        VADR addr, int arn, REGS *regs);
_VSTORE_C_STATIC BYTE ARCH_DEP(vfetchb) (VADR addr, int arn,
        REGS *regs);
_VSTORE_C_STATIC U16 ARCH_DEP(vfetch2) (VADR addr, int arn,
        REGS *regs);
_VSTORE_C_STATIC U32 ARCH_DEP(vfetch4) (VADR addr, int arn,
        REGS *regs);
_VSTORE_C_STATIC U64 ARCH_DEP(vfetch8) (VADR addr, int arn,
        REGS *regs);
_VSTORE_C_STATIC void ARCH_DEP(move_chars) (VADR addr1, int arn1,
      BYTE key1, VADR addr2, int arn2, BYTE key2, int len, REGS *regs);
_VSTORE_C_STATIC void ARCH_DEP(validate_operand) (VADR addr, int arn,
        int len, int acctype, REGS *regs);
_VFETCH_C_STATIC BYTE * ARCH_DEP(instfetch) (REGS *regs, int exec);

#if defined( FEATURE_027_MVCOS_FACILITY )
_VSTORE_C_STATIC void ARCH_DEP(move_charx) (VADR addr1, int space1,
       BYTE key1, VADR addr2, int space2, BYTE key2,
       int len, REGS *regs);
#endif

#if defined( _FEATURE_SIE ) && defined( _370 ) && !defined( _IEEE_C_ )
_VFETCH_C_STATIC BYTE * s370_instfetch (REGS *regs, int exec);
#endif

#if defined( _FEATURE_ZSIE ) && defined( _900 )
_VFETCH_C_STATIC BYTE * s390_instfetch (REGS *regs, int exec);
#endif


#ifndef _INLINE_H
#define _INLINE_H


/*-------------------------------------------------------------------*/
/* Add two unsigned fullwords giving an unsigned fullword result     */
/* and return the condition code for the AL or ALR instruction       */
/*-------------------------------------------------------------------*/
static inline int add_logical(U32 *result, U32 op1, U32 op2)
{
    *result = op1 + op2;
    return (*result == 0 ? 0 : 1) | (op1 > *result ? 2 : 0);
} /* end function add_logical */


/*-------------------------------------------------------------------*/
/* Subtract two unsigned fullwords giving unsigned fullword result   */
/* and return the condition code for the SL or SLR instruction       */
/*-------------------------------------------------------------------*/
static inline int sub_logical(U32 *result, U32 op1, U32 op2)
{
    *result = op1 - op2;
    return (*result == 0 ? 0 : 1) | (op1 < *result ? 0 : 2);
} /* end function sub_logical */


/*-------------------------------------------------------------------*/
/* Add two signed fullwords giving a signed fullword result          */
/* and return the condition code for the A or AR instruction         */
/*-------------------------------------------------------------------*/
static inline int add_signed( U32* result, U32 op1, U32 op2 )
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
static inline int sub_signed( U32* result, U32 op1, U32 op2 )
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
static inline void mul_signed ( U32 *resulthi, U32 *resultlo,
                             U32 op1, U32 op2 )
{
S64 r;

    r = (S64)(S32)op1 * (S32)op2;
    *resulthi = (U32)((U64)r >> 32);
    *resultlo = (U32)((U64)r & 0xFFFFFFFF);
} /* end function mul_signed */


/*-------------------------------------------------------------------*/
/* Divide a signed doubleword dividend by a signed fullword divisor  */
/* giving a signed fullword remainder and a signed fullword quotient.*/
/* Returns 0 if successful, 1 if divide overflow.                    */
/*-------------------------------------------------------------------*/
static inline int div_signed ( U32 *remainder, U32 *quotient,
              U32 dividendhi, U32 dividendlo, U32 divisor )
{
U64 dividend;
S64 quot, rem;

    if (divisor == 0) return 1;
    dividend = (U64)dividendhi << 32 | dividendlo;
    quot = (S64)dividend / (S32)divisor;
    rem = (S64)dividend % (S32)divisor;
    if (quot < -2147483648LL || quot > 2147483647LL) return 1;
    *quotient = (U32)quot;
    *remainder = (U32)rem;
    return 0;
} /* end function div_signed */

/*
 * The following routines were moved from esame.c rev 1.139 21oct2005
 */

/*-------------------------------------------------------------------*/
/* Add two unsigned doublewords giving an unsigned doubleword result */
/* and return the condition code for the ALG or ALGR instruction     */
/*-------------------------------------------------------------------*/
static inline int add_logical_long(U64 *result, U64 op1, U64 op2)
{
    *result = op1 + op2;
    return (*result == 0 ? 0 : 1) | (op1 > *result ? 2 : 0);
} /* end function add_logical_long */


/*-------------------------------------------------------------------*/
/* Subtract unsigned doublewords giving unsigned doubleword result   */
/* and return the condition code for the SLG or SLGR instruction     */
/*-------------------------------------------------------------------*/
static inline int sub_logical_long(U64 *result, U64 op1, U64 op2)
{
    *result = op1 - op2;
    return (*result == 0 ? 0 : 1) | (op1 < *result ? 0 : 2);
} /* end function sub_logical_long */


/*-------------------------------------------------------------------*/
/* Add two signed doublewords giving a signed doubleword result      */
/* and return the condition code for the AG or AGR instruction       */
/*-------------------------------------------------------------------*/
static inline int add_signed_long( U64* result, U64 op1, U64 op2 )
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
static inline int sub_signed_long( U64* result, U64 op1, U64 op2 )
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
/* Divide an unsigned 128-bit dividend by an unsigned 64-bit divisor */
/* giving unsigned 64-bit remainder and unsigned 64-bit quotient.    */
/* Returns 0 if successful, 1 if divide overflow.                    */
/*-------------------------------------------------------------------*/
static inline int div_logical_long
                  (U64 *rem, U64 *quot, U64 high, U64 lo, U64 d)
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
} /* end function div_logical_long */


/*-------------------------------------------------------------------*/
/* Multiply two unsigned doublewords giving unsigned 128-bit result  */
/*-------------------------------------------------------------------*/
static inline int mult_logical_long
                  (U64 *high, U64 *lo, U64 md, U64 mr)
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
} /* end function mult_logical_long */


#endif /*!defined(_INLINE_H)*/


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
static inline int ARCH_DEP(is_fetch_protected) (VADR addr, BYTE skey,
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
static inline int ARCH_DEP(is_low_address_protected) (VADR addr,
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
/*      1=Store protected, 0=Not store protected                         */
/*-------------------------------------------------------------------*/
static inline int ARCH_DEP(is_store_protected) (VADR addr, BYTE skey,
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
#if defined(INLINE_STORE_FETCH_ADDR_CHECK)
static inline BYTE *ARCH_DEP(fetch_main_absolute) (RADR addr,
                                REGS *regs, int len)
#else
static inline BYTE *ARCH_DEP(fetch_main_absolute) (RADR addr,
                                REGS *regs)
#endif
{
#if defined(INLINE_STORE_FETCH_ADDR_CHECK)
    if(addr > regs->mainlim - len)
        regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);
#endif /*defined(INLINE_STORE_FETCH_ADDR_CHECK)*/

    SIE_TRANSLATE(&addr, ACCTYPE_READ, regs);

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
static inline U64 ARCH_DEP(fetch_doubleword_absolute) (RADR addr,
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
static inline U32 ARCH_DEP(fetch_fullword_absolute) (RADR addr,
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
static inline U16 ARCH_DEP(fetch_halfword_absolute) (RADR addr,
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
static inline void ARCH_DEP(store_doubleword_absolute) (U64 value,
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
static inline void ARCH_DEP(store_fullword_absolute) (U32 value,
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

static inline RADR ARCH_DEP( apply_prefixing )( RADR addr, RADR px )
{
    return APPLY_PREFIXING( addr, px );
}


#include "dat.h"
#include "vstore.h"

/* end of INLINE.H */
