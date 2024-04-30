/* DFP.C        (C) Copyright Roger Bowler, 2007-2012                */
/*              (C) and others 2024                                  */
/*              Decimal Floating Point instructions                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module implements the Decimal Floating Point instructions    */
/* and the Floating Point Support Enhancement Facility instructions  */
/* described in the z/Architecture Principles of Operation manual.   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DFP_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined( FEATURE_042_DFP_FACILITY )
#include "decimal128.h"
#include "decimal64.h"
#include "decimal32.h"
#include "decPacked.h"
#endif

DISABLE_GCC_UNUSED_SET_WARNING;

#if defined(FEATURE_041_FPS_ENHANCEMENT_FACILITY)
/*===================================================================*/
/* FLOATING POINT SUPPORT INSTRUCTIONS                               */
/*===================================================================*/
/* Note: the Floating Point Support instructions use the HFPREG_CHECK
   and HFPREG2_CHECK macros to enforce an AFP-register data exception
   if an FPS instruction attempts to use one of the 12 additional FPR
   registers when the AFP-register-control bit in CR0 is zero. */

#if defined( FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
/*-------------------------------------------------------------------*/
/* B370 LPDFR - Load Positive FPR Long Register                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_positive_fpr_long_reg)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Copy register contents, clear the sign bit */
    regs->FPR_L(r1) = regs->FPR_L(r2) & 0x7FFFFFFFFFFFFFFFULL;

} /* end DEF_INST(load_positive_fpr_long_reg) */


/*-------------------------------------------------------------------*/
/* B371 LNDFR - Load Negative FPR Long Register                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_negative_fpr_long_reg)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Copy register contents, set the sign bit */
    regs->FPR_L(r1) = regs->FPR_L(r2) | 0x8000000000000000ULL;

} /* end DEF_INST(load_negative_fpr_long_reg) */


/*-------------------------------------------------------------------*/
/* B372 CPSDR - Copy Sign FPR Long Register                  [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST(copy_sign_fpr_long_reg)
{
int     r1, r2, r3;                     /* Values of R fields        */
U64     sign;                           /* Work area for sign bit    */

    RRF_M(inst, regs, r1, r2, r3);

    TXFC_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);
    HFPREG_CHECK(r3, regs);

    /* Copy the sign bit from r3 register */
    sign = regs->FPR_L(r3) & 0x8000000000000000ULL;

    /* Copy r2 register contents to r1 register */
    regs->FPR_L(r1) = regs->FPR_L(r2);

    /* Insert the sign bit into r1 register */
    regs->FPR_L(r1) &= 0x7FFFFFFFFFFFFFFFULL;
    regs->FPR_L(r1) |= sign;

} /* end DEF_INST(copy_sign_fpr_long_reg) */


/*-------------------------------------------------------------------*/
/* B373 LCDFR - Load Complement FPR Long Register              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_complement_fpr_long_reg)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Copy register contents, invert sign bit */
    regs->FPR_L(r1) = regs->FPR_L(r2) ^ 0x8000000000000000ULL;

} /* end DEF_INST(load_complement_fpr_long_reg) */
#endif /* defined( FEATURE_041_FPS_SIGN_HANDLING_FACILITY ) */


#if defined( FEATURE_041_FPR_GR_TRANSFER_FACILITY )
/*-------------------------------------------------------------------*/
/* B3C1 LDGR  - Load FPR from GR Long Register                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fpr_from_gr_long_reg)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Load FP register contents from general register */
    regs->FPR_L(r1) = regs->GR_G(r2);

} /* end DEF_INST(load_fpr_from_gr_long_reg) */


/*-------------------------------------------------------------------*/
/* B3CD LGDR  - Load GR from FPR Long Register                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_gr_from_fpr_long_reg)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r2, regs);

    /* Load general register contents from FP register */
    regs->GR_G(r1) = regs->FPR_L(r2);

} /* end DEF_INST(load_gr_from_fpr_long_reg) */
#endif /* defined( FEATURE_041_FPR_GR_TRANSFER_FACILITY ) */


#if defined( FEATURE_041_DFP_ROUNDING_FACILITY )
/*-------------------------------------------------------------------*/
/* B2B9 SRNMT - Set DFP Rounding Mode                            [S] */
/*-------------------------------------------------------------------*/
DEF_INST(set_dfp_rounding_mode)
{
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    TXFC_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Set DFP rounding mode in FPC register from address bits 61-63 */
    regs->fpc &= ~(FPC_DRM);
    regs->fpc |= ((effective_addr2 << FPC_DRM_SHIFT) & FPC_DRM);

} /* end DEF_INST(set_dfp_rounding_mode) */
#endif /* defined( FEATURE_041_DFP_ROUNDING_FACILITY ) */


#if defined(FEATURE_041_IEEE_EXCEPT_SIM_FACILITY)
/*===================================================================*/
/* IEEE-EXCEPTION-SIMULATION FACILITY INSTRUCTIONS                   */
/*===================================================================*/

#if !defined(_IXS_ARCH_INDEPENDENT_)
/*-------------------------------------------------------------------*/
/* Check if a simulated-IEEE-exception event is to be recognized     */
/*                                                                   */
/* This subroutine is called by the LFAS and SFASR instructions to   */
/* determine whether the instruction should raise a data exception   */
/* at the end of the instruction and, if so, the DXC code to be set. */
/*                                                                   */
/* Input:                                                            */
/*      cur_fpc         Current value of the FPC register            */
/*      src_fpc         Value of instruction source operand          */
/* Output:                                                           */
/*      The return value is the data exception code (DXC), or        */
/*      zero if no simulated-IEEE-exception event is recognized      */
/*-------------------------------------------------------------------*/
static BYTE
fpc_signal_check(U32 cur_fpc, U32 src_fpc)
{
U32     ff, sm, enabled_flags;      /* Mask and flag work areas  */
BYTE    dxc;                        /* Data exception code or 0      */

    /* AND the current FPC flags with the source FPC mask */
    ff = (cur_fpc & FPC_FLAGS) >> FPC_FLAG_SHIFT;
    sm = (src_fpc & FPC_MASKS) >> FPC_MASK_SHIFT;
    enabled_flags = (ff & sm)  << FPC_FLAG_SHIFT;

    /* A simulated-IEEE-exception event is recognized
       if any current flag corresponds to the source mask */
    if (enabled_flags & FPC_FLAG_SFI)
    {
        dxc = DXC_IEEE_INV_OP_IISE;
    }
    else if (enabled_flags & FPC_FLAG_SFZ)
    {
        dxc = DXC_IEEE_DIV_ZERO_IISE;
    }
    else if (enabled_flags & FPC_FLAG_SFO)
    {
        dxc = (cur_fpc & FPC_FLAG_SFX) ?
                DXC_IEEE_OF_INEX_IISE :
                DXC_IEEE_OF_EXACT_IISE;
    }
    else if (enabled_flags & FPC_FLAG_SFU)
    {
        dxc = (cur_fpc & FPC_FLAG_SFX) ?
                DXC_IEEE_UF_INEX_IISE :
                DXC_IEEE_UF_EXACT_IISE;
    }
    else if (enabled_flags & FPC_FLAG_SFX)
    {
        dxc = DXC_IEEE_INEXACT_IISE;
    }
    else
    {
        dxc = 0;
    }

    /* Return data exception code or zero */
    return dxc;
} /* end function fpc_signal_check */
#define _IXS_ARCH_INDEPENDENT_
#endif /*!defined(_IXS_ARCH_INDEPENDENT_)*/


/*-------------------------------------------------------------------*/
/* B2BD LFAS  - Load FPC and Signal                              [S] */
/*-------------------------------------------------------------------*/
DEF_INST( load_fpc_and_signal )
{
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
U32             src_fpc, new_fpc;       /* New value for FPC         */
BYTE            dxc;                    /* Data exception code       */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXFC_INSTR_CHECK( regs );
    DFPINST_CHECK( regs );

    /* Load new FPC register contents from operand location */
    src_fpc = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );

    /* Program check if reserved bits are non-zero */
    FPC_CHECK( src_fpc, regs );

    /* OR the flags from the current FPC register */
#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        new_fpc = src_fpc | (regs->fpc & FPC_FPX_FLAGS);
    else
#endif
        new_fpc = src_fpc | (regs->fpc & FPC_FLAGS);

    /* Determine whether an event is to be signaled */
    dxc = fpc_signal_check(regs->fpc, src_fpc);

    /* Update the FPC register */
    regs->fpc = new_fpc;

    /* Signal a simulated-IEEE-exception event if needed */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP( program_interrupt )( regs, PGM_DATA_EXCEPTION );
    }

} /* end DEF_INST(load_fpc_and_signal) */


/*-------------------------------------------------------------------*/
/* B385 SFASR - Set FPC and Signal                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( set_fpc_and_signal )
{
int             r1, r2;                 /* Values of R fields        */
U32             src_fpc, new_fpc;       /* New value for FPC         */
BYTE            dxc;                    /* Data exception code       */

    RRE( inst, regs, r1, r2 );

    TXFC_INSTR_CHECK( regs );
    DFPINST_CHECK( regs );

    /* Load new FPC register contents from R1 register bits 32-63 */
    src_fpc = regs->GR_L(r1);

    /* Program check if reserved bits are non-zero */
    FPC_CHECK( src_fpc, regs );

    /* OR the flags from the current FPC register */
#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        new_fpc = src_fpc | (regs->fpc & FPC_FPX_FLAGS);
    else
#endif
        new_fpc = src_fpc | (regs->fpc & FPC_FLAGS);

    /* Determine whether an event is to be signaled */
    dxc = fpc_signal_check(regs->fpc, src_fpc);

    /* Update the FPC register */
    regs->fpc = new_fpc;

    /* Signal a simulated-IEEE-exception event if needed */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP( program_interrupt )( regs, PGM_DATA_EXCEPTION );
    }

} /* end DEF_INST(set_fpc_and_signal) */
#endif /*defined(FEATURE_041_IEEE_EXCEPT_SIM_FACILITY)*/
#endif /*defined(FEATURE_041_FPS_ENHANCEMENT_FACILITY)*/


#if defined(FEATURE_042_DFP_FACILITY)
/*===================================================================*/
/* DECIMAL FLOATING POINT INSTRUCTIONS                               */
/*===================================================================*/
/* Note: the DFP instructions use the DFPINST_CHECK macro to check the
   setting of the AFP-register-control bit in CR0. If this bit is zero
   then the macro generates a DFP-instruction data exception. */

#if !defined(_DFP_ARCH_INDEPENDENT_)
/*-------------------------------------------------------------------*/
/* Extract the leftmost digit from a decimal32/64/128 structure      */
/*-------------------------------------------------------------------*/
static const int
dfp_lmdtable[32] = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7,
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 9, 8, 9, 0, 0};

static inline int
dfp32_extract_lmd(decimal32 *xp)
{
    unsigned int cf = (((FW*)xp)->F & 0x7C000000) >> 26;
    return dfp_lmdtable[cf];
} /* end function dfp32_extract_lmd */

static inline int
dfp64_extract_lmd(decimal64 *xp)
{
    unsigned int cf = (((DW*)xp)->F.H.F & 0x7C000000) >> 26;
    return dfp_lmdtable[cf];
} /* end function dfp64_extract_lmd */

static inline int
dfp128_extract_lmd(decimal128 *xp)
{
    unsigned int cf = (((QW*)xp)->F.HH.F & 0x7C000000) >> 26;
    return dfp_lmdtable[cf];
} /* end function dfp128_extract_lmd */

/*-------------------------------------------------------------------*/
/* Clear the CF and BXCF fields of a decimal32/64/128 structure      */
/*-------------------------------------------------------------------*/
static inline void
dfp32_clear_cf_and_bxcf(decimal32 *xp)
{
    ((FW*)xp)->F &= 0x800FFFFF;         /* Clear CF and BXCF fields  */
} /* end function dfp32_clear_cf_and_bxcf */

static inline void
dfp64_clear_cf_and_bxcf(decimal64 *xp)
{
    ((DW*)xp)->F.H.F &= 0x8003FFFF;     /* Clear CF and BXCF fields  */
} /* end function dfp64_clear_cf_and_bxcf */

static inline void
dfp128_clear_cf_and_bxcf(decimal128 *xp)
{
    ((QW*)xp)->F.HH.F &= 0x80003FFF;    /* Clear CF and BXCF fields  */
} /* end function dfp128_clear_cf_and_bxcf */

/*-------------------------------------------------------------------*/
/* Set the CF and BXCF fields of a decimal32/64/128 structure        */
/* Input:                                                            */
/*      xp      Pointer to a decimal32/64/128 structure              */
/*      cfs     A 32-bit value, of which bits 0-25 are ignored,      */
/*              bits 26-30 contain the new CF field value (5-bits),  */
/*              bit 31 is the new BXCF signaling indicator (1-bit).  */
/* Output:                                                           */
/*      The CF field and the high-order bit of the BXCF field in     */
/*      the decimal32/64/128 structure are set to the indicated      */
/*      values and the remaining bits of the BXCF field are cleared. */
/*-------------------------------------------------------------------*/
#define DFP_CFS_INF     ((30<<1)|0)     /* CF and BXCF-S for Inf     */
#define DFP_CFS_QNAN    ((31<<1)|0)     /* CF and BXCF-S for QNaN    */
#define DFP_CFS_SNAN    ((31<<1)|1)     /* CF and BXCF-S for SNaN    */

static inline void
dfp32_set_cf_and_bxcf(decimal32 *xp, U32 cfs)
{
    ((FW*)xp)->F &= 0x800FFFFF;         /* Clear CF and BXCF fields  */
    ((FW*)xp)->F |= (cfs & 0x3F) << 25;
                                        /* Set CF and BXCF S-bit     */
} /* end function dfp32_set_cf_and_bxcf */

static inline void
dfp64_set_cf_and_bxcf(decimal64 *xp, U32 cfs)
{
    ((DW*)xp)->F.H.F &= 0x8003FFFF;     /* Clear CF and BXCF fields  */
    ((DW*)xp)->F.H.F |= (cfs & 0x3F) << 25;
                                        /* Set CF and BXCF S-bit     */
} /* end function dfp64_set_cf_and_bxcf */

static inline void
dfp128_set_cf_and_bxcf(decimal128 *xp, U32 cfs)
{
    ((QW*)xp)->F.HH.F &= 0x80003FFF;    /* Clear CF and BXCF fields  */
    ((QW*)xp)->F.HH.F |= (cfs & 0x3F) << 25;
                                        /* Set CF and BXCF S-bit     */
} /* end function dfp128_set_cf_and_bxcf */

/*-------------------------------------------------------------------*/
/* Compare exponent and return condition code                        */
/*                                                                   */
/* This subroutine is called by the CEETR, CEDTR, and CEXTR          */
/* instructions. It compares the exponents of two decimal            */
/* numbers and returns a condition code.                             */
/*                                                                   */
/* Input:                                                            */
/*      d1,d2   Pointers to decimal number structures                */
/* Output:                                                           */
/*      The return value is the condition code                       */
/*-------------------------------------------------------------------*/
static inline int
dfp_compare_exponent(decNumber *d1, decNumber *d2)
{
int             cc;                     /* Condition code            */

    if (decNumberIsNaN(d1) && decNumberIsNaN(d2))
        cc = 0;
    else if (decNumberIsNaN(d1) || decNumberIsNaN(d2))
        cc = 3;
    else if (decNumberIsInfinite(d1) && decNumberIsInfinite(d2))
        cc = 0;
    else if (decNumberIsInfinite(d1) || decNumberIsInfinite(d2))
        cc = 3;
    else
        cc = (d1->exponent == d2->exponent) ? 0 :
                (d1->exponent < d2->exponent) ? 1 : 2 ;

    return cc;
} /* end function dfp_compare_exponent */

/*-------------------------------------------------------------------*/
/* Convert 64-bit signed binary integer to decimal number            */
/*                                                                   */
/* This subroutine is called by the CDGTR and CXGTR instructions.    */
/* It converts a 64-bit signed binary integer value into a           */
/* decimal number structure. The inexact condition will be set       */
/* in the decimal context structure if the number is rounded to      */
/* fit the maximum number of digits specified in the context.        */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*      n       64-bit signed binary integer value                   */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The decimal number structure is updated.                     */
/*-------------------------------------------------------------------*/
static void
dfp_number_from_fix64(decNumber *dn, S64 n, decContext *pset)
{
int             sign = 0;               /* Sign of binary integer    */
int             i;                      /* Counter                   */
char            zoned[32];              /* Zoned decimal work area   */
static char     maxnegzd[]="-9223372036854775808";
static U64      maxneg64 = 0x8000000000000000ULL;

    /* Handle maximum negative number as special case */
    if (n == (S64)maxneg64)
    {
        decNumberFromString(dn, maxnegzd, pset);
        return;
    }

    /* Convert binary value to zoned decimal */
    if (n < 0) { n = -n; sign = 1; }
    i = sizeof(zoned) - 1;
    zoned[i] = '\0';
    do {
        zoned[--i] = (n % 10) + '0';
        n /= 10;
    } while(i > 1 && n > 0);
    if (sign) zoned[--i] = '-';

    /* Convert zoned decimal value to decimal number structure */
    decNumberFromString(dn, zoned+i, pset);

} /* end function dfp_number_from_fix64 */

/*-------------------------------------------------------------------*/
/* Convert decimal number to 64-bit signed binary integer            */
/*                                                                   */
/* This subroutine is called by the CGDTR and CGXTR instructions.    */
/* It converts a decimal number structure to a 64-bit signed         */
/* binary integer value.  The inexact condition will be set in       */
/* the decimal context structure if the number is rounded to         */
/* an integer. The invalid operation condition will be set if        */
/* the decimal value is outside the range of a 64-bit integer.       */
/*                                                                   */
/* Input:                                                            */
/*      b       Pointer to decimal number structure                  */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The return value is the 64-bit signed binary integer result  */
/*-------------------------------------------------------------------*/
static S64
dfp_number_to_fix64(decNumber *b, decContext *pset)
{
S64             n;                      /* 64-bit signed result      */
int32_t         scale;                  /* Scaling factor            */
unsigned        i;                      /* Array subscript           */
BYTE            packed[17];             /* 33-digit packed work area */
decNumber       p, c;                   /* Working decimal numbers   */
static U64      mp64 = 0x7FFFFFFFFFFFFFFFULL;   /* Max pos fixed 64  */
static U64      mn64 = 0x8000000000000000ULL;   /* Max neg fixed 64  */
static char     mpzd[]="9223372036854775807";   /* Max pos zoned dec */
static char     mnzd[]="-9223372036854775808";  /* Max neg zoned dec */
static BYTE     mpflag = 0;             /* 1=mp,mn are initialized   */
static decNumber mp, mn;                /* Decimal maximum pos,neg   */
decContext      setmax;                 /* Working context for mp,mn */

    /* Prime the decimal number structures representing the maximum
       positive and negative numbers representable in 64 bits. Use
       a 128-bit DFP working context because these numbers are too
       big to be represented in the 32-bit and 64-bit DFP formats */
    if (mpflag == 0)
    {
        decContextDefault(&setmax, DEC_INIT_DECIMAL128);
        decNumberFromString(&mp, mpzd, &setmax);
        decNumberFromString(&mn, mnzd, &setmax);
        mpflag = 1;
    }

    /* If operand is a NaN then set invalid operation
       and return maximum negative result */
    if (decNumberIsNaN(b))
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (S64)mn64;
    }

    /* Remove fractional part of decimal number */
    decNumberToIntegralValue(&p, b, pset);

    /* Special case if operand is less than maximum negative
       number (including where operand is negative infinity) */
    decNumberCompare(&c, b, &mn, pset);
    if (decNumberIsNegative(&c))
    {
        /* If rounded value is less than maximum negative number
           then set invalid operation otherwise set inexact */
        decNumberCompare(&c, &p, &mn, pset);
        if (decNumberIsNegative(&c))
            pset->status |= DEC_IEEE_854_Invalid_operation;
        else
            pset->status |= DEC_IEEE_854_Inexact;

        /* Return maximum negative result */
        return (S64)mn64;
    }

    /* Special case if operand is greater than maximum positive
       number (including where operand is positive infinity) */
    decNumberCompare(&c, b, &mp, pset);
    if (decNumberIsNegative(&c) == 0 && decNumberIsZero(&c) == 0)
    {
        /* If rounded value is greater than maximum positive number
           then set invalid operation otherwise set inexact */
        decNumberCompare(&c, &p, &mp, pset);
        if (decNumberIsNegative(&c) == 0 && decNumberIsZero(&c) == 0)
            pset->status |= DEC_IEEE_854_Invalid_operation;
        else
            pset->status |= DEC_IEEE_854_Inexact;

        /* Return maximum positive result */
        return (S64)mp64;
    }

    /* Raise inexact condition if result was rounded */
    decNumberCompare(&c, &p, b, pset);
    if (decNumberIsZero(&c) == 0)
    {
        pset->status |= DEC_IEEE_854_Inexact;
        if (decNumberIsNegative(&c) == decNumberIsNegative(b))
            pset->status |= DEC_Rounded;
    }

    /* Convert decimal number structure to packed decimal */
    decPackedFromNumber(packed, sizeof(packed), &scale, &p);

    /* Convert packed decimal to binary value */
    for (i = 0, n = 0; i < sizeof(packed)-1; i++)
    {
        n = n * 10 + ((packed[i] & 0xF0) >> 4);
        n = n * 10 + (packed[i] & 0x0F);
    }
    n = n * 10 + ((packed[i] & 0xF0) >> 4);
    while (scale++) n *= 10;
    if ((packed[i] & 0x0F) == 0x0D) n = -n;

    /* Return 64-bit signed result */
    return n;

} /* end function dfp_number_to_fix64 */

#define MAXDECSTRLEN DECIMAL128_String  /* Maximum string length     */
/*-------------------------------------------------------------------*/
/* Shift decimal coefficient left or right                           */
/*                                                                   */
/* This subroutine is called by the SLDT, SLXT, SRDT and SRXT        */
/* instructions. It shifts the coefficient digits of a decimal       */
/* number left or right. For a left shift, zeroes are appended       */
/* to the coefficient. For a right shift, digits are dropped         */
/* from the end of the coefficient. No rounding is performed.        */
/* The sign and exponent of the number remain unchanged.             */
/*                                                                   */
/* Input:                                                            */
/*      pset    Pointer to decimal number context structure          */
/*      dn      Pointer to decimal number structure to be shifted    */
/*      count   Number of digits to shift (+ve=left, -ve=right)      */
/* Output:                                                           */
/*      The decimal number structure is updated.                     */
/*-------------------------------------------------------------------*/
static inline void
dfp_shift_coeff(decContext *pset, decNumber *dn, int count)
{
size_t          len;                    /* String length             */
size_t          maxlen;                 /* Maximum coefficient length*/
int32_t         exp;                    /* Original exponent         */
uint8_t         bits;                   /* Original flag bits        */
char            zd[MAXDECSTRLEN+64];    /* Zoned decimal work area   */

    /* Save original exponent and sign/Inf/NaN bits */
    exp = dn->exponent;
    bits = dn->bits;

    /* Clear exponent and sign/Inf/NaN bits */
    dn->exponent = 0;
    dn->bits &= ~(DECNEG | DECSPECIAL);

    /* Convert coefficient digits to zoned decimal */
    decNumberToString(dn, zd);
    len = strlen(zd);

    /* Shift zoned digits left or right */
    if (count > 0)
        memset(zd + len, '0', count);
    len += count;
    maxlen = (bits & DECSPECIAL) ? pset->digits - 1 : pset->digits;
    if (len > maxlen)
    {
        memmove(zd, zd + len - maxlen, maxlen);
        len = maxlen;
    }
    else if (len < 1)
    {
        zd[0] = '0';
        len = 1;
    }
    zd[len] = '\0';

    /* Convert shifted coefficient to decimal number structure */
    decNumberFromString(dn, zd, pset);

    /* Restore original exponent and sign/Inf/NaN bits */
    dn->exponent = exp;
    dn->bits |= bits & (DECNEG | DECSPECIAL);

} /* end function dfp_shift_coeff */

/* Bit numbers for Test Data Class instructions */
#define DFP_TDC_ZERO            52
#define DFP_TDC_SUBNORMAL       54
#define DFP_TDC_NORMAL          56
#define DFP_TDC_INFINITY        58
#define DFP_TDC_QUIET_NAN       60
#define DFP_TDC_SIGNALING_NAN   62

/*-------------------------------------------------------------------*/
/* Test data class and return condition code                         */
/*                                                                   */
/* This subroutine is called by the TDCET, TDCDT, and TDCXT          */
/* instructions. It tests the data class and sign of a decimal       */
/* number. Each combination of data class and sign corresponds       */
/* to one of 12 possible bits in a bitmask. The value (0 or 1)       */
/* of the corresponding bit is returned.                             */
/*                                                                   */
/* Input:                                                            */
/*      pset    Pointer to decimal number context structure          */
/*      dn      Pointer to decimal number structure to be tested     */
/*      bits    Bitmask in rightmost 12 bits                         */
/* Output:                                                           */
/*      The return value is 0 or 1.                                  */
/*-------------------------------------------------------------------*/
static inline int
dfp_test_data_class(decContext *pset, decNumber *dn, U32 bits)
{
int             bitn;                   /* Bit number                */
decNumber       dm;                     /* Normalized value of dn    */

    if (decNumberIsZero(dn))
        bitn = DFP_TDC_ZERO;
    else if (decNumberIsInfinite(dn))
        bitn = DFP_TDC_INFINITY;
    else if (decNumberIsQNaN(dn))
        bitn = DFP_TDC_QUIET_NAN;
    else if (decNumberIsSNaN(dn))
        bitn = DFP_TDC_SIGNALING_NAN;
    else {
        decNumberNormalize(&dm, dn, pset);
        bitn = (dm.exponent < pset->emin) ?
                DFP_TDC_SUBNORMAL :
                DFP_TDC_NORMAL ;
    }

    if (decNumberIsNegative(dn)) bitn++;

    return (bits >> (63 - bitn)) & 0x01;

} /* end function dfp_test_data_class */

/* Bit numbers for Test Data Group instructions */
#define DFP_TDG_SAFE_ZERO       52
#define DFP_TDG_EXTREME_ZERO    54
#define DFP_TDG_EXTREME_NONZERO 56
#define DFP_TDG_SAFE_NZ_LMD_Z   58
#define DFP_TDG_SAFE_NZ_LMD_NZ  60
#define DFP_TDG_SPECIAL         62

/*-------------------------------------------------------------------*/
/* Test data group and return condition code                         */
/*                                                                   */
/* This subroutine is called by the TDGET, TDGDT, and TDGXT          */
/* instructions. It tests the exponent and leftmost coefficient      */
/* digit of a decimal number to determine which of 12 possible       */
/* groups the number corresponds to. Each group corresponds to       */
/* one of 12 possible bits in a bitmask. The value (0 or 1) of       */
/* the corresponding bit is returned.                                */
/*                                                                   */
/* Input:                                                            */
/*      pset    Pointer to decimal number context structure          */
/*      dn      Pointer to decimal number structure to be tested     */
/*      lmd     Leftmost digit of decimal FP number                  */
/*      bits    Bitmask in rightmost 12 bits                         */
/* Output:                                                           */
/*      The return value is 0 or 1.                                  */
/*-------------------------------------------------------------------*/
static inline int
dfp_test_data_group(decContext *pset, decNumber *dn, int lmd, U32 bits)
{
int             bitn;                   /* Bit number                */
int             extreme;                /* 1=exponent is min or max  */
int             exp;                    /* Adjusted exponent         */

    exp = dn->exponent + pset->digits - 1;
    extreme = (exp == pset->emin) || (exp == pset->emax);

    if (decNumberIsZero(dn))
        bitn = extreme ?
                DFP_TDG_EXTREME_ZERO :
                DFP_TDG_SAFE_ZERO ;
    else if (decNumberIsInfinite(dn) || decNumberIsNaN(dn))
        bitn = DFP_TDG_SPECIAL;
    else if (extreme)
        bitn = DFP_TDG_EXTREME_NONZERO;
    else {
        bitn = (lmd == 0) ?
                DFP_TDG_SAFE_NZ_LMD_Z :
                DFP_TDG_SAFE_NZ_LMD_NZ ;
    }

    if (decNumberIsNegative(dn)) bitn++;

    return (bits >> (63 - bitn)) & 0x01;

} /* end function dfp_test_data_group */

#define _DFP_ARCH_INDEPENDENT_
#endif /*!defined(_DFP_ARCH_INDEPENDENT_)*/

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
#if !defined(_DFP_FPE_ARCH_INDEPENDENT_)
/*-------------------------------------------------------------------*/
/* Convert 32-bit signed binary integer to decimal number            */
/*                                                                   */
/* This subroutine is called by the CDFTR and CXFTR instructions.    */
/* It converts a 32-bit signed binary integer value into a           */
/* decimal number structure. The inexact condition will be set       */
/* in the decimal context structure if the number is rounded to      */
/* fit the maximum number of digits specified in the context.        */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*      n       32-bit signed binary integer value                   */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The decimal number structure is updated.                     */
/*-------------------------------------------------------------------*/
static void
dfp_number_from_fix32(decNumber *dn, S32 n, decContext *pset)
{
int             sign = 0;               /* Sign of binary integer    */
int             i;                      /* Counter                   */
char            zoned[32];              /* Zoned decimal work area   */
static char     maxnegzd[]="-2147483648";
static U32      maxneg32 = 0x80000000UL;

    /* Handle maximum negative number as special case */
    if (n == (S32)maxneg32)
    {
        decNumberFromString(dn, maxnegzd, pset);
        return;
    }

    /* Convert binary value to zoned decimal */
    if (n < 0) { n = -n; sign = 1; }
    i = sizeof(zoned) - 1;
    zoned[i] = '\0';
    do {
        zoned[--i] = (n % 10) + '0';
        n /= 10;
    } while(i > 1 && n > 0);
    if (sign) zoned[--i] = '-';

    /* Convert zoned decimal value to decimal number structure */
    decNumberFromString(dn, zoned+i, pset);

} /* end function dfp_number_from_fix32 */

/*-------------------------------------------------------------------*/
/* Convert 32-bit unsigned binary integer to decimal number          */
/*                                                                   */
/* This subroutine is called by the CDLFTR and CXLFTR instructions.  */
/* It converts a 32-bit unsigned binary integer value into a         */
/* decimal number structure. The inexact condition will be set       */
/* in the decimal context structure if the number is rounded to      */
/* fit the maximum number of digits specified in the context.        */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*      n       32-bit unsigned binary integer value                 */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The decimal number structure is updated.                     */
/*-------------------------------------------------------------------*/
static void
dfp_number_from_u32(decNumber *dn, U32 n, decContext *pset)
{
int             i;                      /* Counter                   */
char            zoned[32];              /* Zoned decimal work area   */

    /* Convert unsigned binary value to zoned decimal */
    i = sizeof(zoned) - 1;
    zoned[i] = '\0';
    do {
        zoned[--i] = (n % 10) + '0';
        n /= 10;
    } while(i > 1 && n > 0);

    /* Convert zoned decimal value to decimal number structure */
    decNumberFromString(dn, zoned+i, pset);

} /* end function dfp_number_from_u32 */

/*-------------------------------------------------------------------*/
/* Convert 64-bit unsigned binary integer to decimal number          */
/*                                                                   */
/* This subroutine is called by the CDLGTR and CXLGTR instructions.  */
/* It converts a 64-bit unsigned binary integer value into a         */
/* decimal number structure. The inexact condition will be set       */
/* in the decimal context structure if the number is rounded to      */
/* fit the maximum number of digits specified in the context.        */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*      n       64-bit unsigned binary integer value                 */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The decimal number structure is updated.                     */
/*-------------------------------------------------------------------*/
static void
dfp_number_from_u64(decNumber *dn, U64 n, decContext *pset)
{
int             i;                      /* Counter                   */
char            zoned[32];              /* Zoned decimal work area   */

    /* Convert unsigned binary value to zoned decimal */
    i = sizeof(zoned) - 1;
    zoned[i] = '\0';
    do {
        zoned[--i] = (n % 10) + '0';
        n /= 10;
    } while(i > 1 && n > 0);

    /* Convert zoned decimal value to decimal number structure */
    decNumberFromString(dn, zoned+i, pset);

} /* end function dfp_number_from_u64 */

/*-------------------------------------------------------------------*/
/* Convert decimal number to 32-bit signed binary integer            */
/*                                                                   */
/* This subroutine is called by the CFDTR and CFXTR instructions.    */
/* It converts a decimal number structure to a 32-bit signed         */
/* binary integer value.  The inexact condition will be set in       */
/* the decimal context structure if the number is rounded to         */
/* an integer. The invalid operation condition will be set if        */
/* the decimal value is outside the range of a 32-bit integer.       */
/*                                                                   */
/* Input:                                                            */
/*      b       Pointer to decimal number structure                  */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The return value is the 32-bit signed binary integer result  */
/*-------------------------------------------------------------------*/
static S32
dfp_number_to_fix32(decNumber *b, decContext *pset)
{
S32             n;                      /* 32-bit signed result      */
int32_t         scale;                  /* Scaling factor            */
unsigned        i;                      /* Array subscript           */
BYTE            packed[17];             /* 33-digit packed work area */
decNumber       p, c;                   /* Working decimal numbers   */
static U32      mp32 = 0x7FFFFFFFUL;    /* Max positive fixed 32     */
static U32      mn32 = 0x80000000UL;    /* Max negative fixed 32     */
static char     mp32zd[]="2147483647";  /* Max positive zoned dec    */
static char     mn32zd[]="-2147483648"; /* Max negative zoned dec    */
static BYTE     mp32flag = 0;           /* 1=mp32dn,mn32dn inited    */
static decNumber mp32dn, mn32dn;        /* Decimal maximum pos,neg   */
decContext      setmax;                 /* Working context           */

    /* Prime the decimal number structures representing the maximum
       positive and negative numbers representable in 32 bits. Use
       a 64-bit DFP working context because these numbers are too
       big to be represented in the 32-bit DFP format */
    if (mp32flag == 0)
    {
        decContextDefault(&setmax, DEC_INIT_DECIMAL64);
        decNumberFromString(&mp32dn, mp32zd, &setmax);
        decNumberFromString(&mn32dn, mn32zd, &setmax);
        mp32flag = 1;
    }

    /* If operand is a NaN then set invalid operation
       and return maximum negative result */
    if (decNumberIsNaN(b))
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (S32)mn32;
    }

    /* Remove fractional part of decimal number */
    decNumberToIntegralValue(&p, b, pset);

    /* Special case if operand is less than maximum negative
       number (including where operand is negative infinity) */
    decNumberCompare(&c, b, &mn32dn, pset);
    if (decNumberIsNegative(&c))
    {
        /* If rounded value is less than maximum negative number
           then set invalid operation otherwise set inexact */
        decNumberCompare(&c, &p, &mn32dn, pset);
        if (decNumberIsNegative(&c))
            pset->status |= DEC_IEEE_854_Invalid_operation;
        else
            pset->status |= DEC_IEEE_854_Inexact;

        /* Return maximum negative result */
        return (S32)mn32;
    }

    /* Special case if operand is greater than maximum positive
       number (including where operand is positive infinity) */
    decNumberCompare(&c, b, &mp32dn, pset);
    if (decNumberIsNegative(&c) == 0 && decNumberIsZero(&c) == 0)
    {
        /* If rounded value is greater than maximum positive number
           then set invalid operation otherwise set inexact */
        decNumberCompare(&c, &p, &mp32dn, pset);
        if (decNumberIsNegative(&c) == 0 && decNumberIsZero(&c) == 0)
            pset->status |= DEC_IEEE_854_Invalid_operation;
        else
            pset->status |= DEC_IEEE_854_Inexact;

        /* Return maximum positive result */
        return (S32)mp32;
    }

    /* Raise inexact condition if result was rounded */
    decNumberCompare(&c, &p, b, pset);
    if (decNumberIsZero(&c) == 0)
    {
        pset->status |= DEC_IEEE_854_Inexact;
        if (decNumberIsNegative(&c) == decNumberIsNegative(b))
            pset->status |= DEC_Rounded;
    }

    /* Convert decimal number structure to packed decimal */
    decPackedFromNumber(packed, sizeof(packed), &scale, &p);

    /* Convert packed decimal to binary value */
    for (i = 0, n = 0; i < sizeof(packed)-1; i++)
    {
        n = n * 10 + ((packed[i] & 0xF0) >> 4);
        n = n * 10 + (packed[i] & 0x0F);
    }
    n = n * 10 + ((packed[i] & 0xF0) >> 4);
    while (scale++) n *= 10;
    if ((packed[i] & 0x0F) == 0x0D) n = -n;

    /* Return 32-bit signed result */
    return n;

} /* end function dfp_number_to_fix32 */

/*-------------------------------------------------------------------*/
/* Convert decimal number to 32-bit unsigned binary integer          */
/*                                                                   */
/* This subroutine is called by the CLFDTR and CLFXTR instructions.  */
/* It converts a decimal number structure to a 32-bit unsigned       */
/* binary integer value.  The inexact condition will be set in       */
/* the decimal context structure if the number is rounded to         */
/* an integer. The invalid operation condition will be set if        */
/* the decimal value is outside the range of a 32-bit unsigned int.  */
/*                                                                   */
/* Input:                                                            */
/*      b       Pointer to decimal number structure                  */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The return value is the 32-bit unsigned integer result       */
/*-------------------------------------------------------------------*/
static U32
dfp_number_to_u32(decNumber *b, decContext *pset)
{
U32             n;                      /* 32-bit unsigned result    */
int32_t         scale;                  /* Scaling factor            */
unsigned        i;                      /* Array subscript           */
BYTE            packed[17];             /* 33-digit packed work area */
decNumber       p, c;                   /* Working decimal numbers   */
static U32      mu32 = 0xFFFFFFFFUL;    /* Max unsigned integer      */
static char     mu32zd[]="4294967295";  /* Max unsigned zoned dec    */
static BYTE     mu32flag = 0;           /* 1=mu32dn is initialized   */
static decNumber mu32dn;                /* Decimal maximum unsigned  */
decContext      setmax;                 /* Working context           */

    /* Prime the decimal number structure representing the maximum
       unsigned number representable in 32 bits. Use a 64-bit DFP
       working context because this number is too big to be
       represented in the 32-bit DFP format */
    if (mu32flag == 0)
    {
        decContextDefault(&setmax, DEC_INIT_DECIMAL64);
        decNumberFromString(&mu32dn, mu32zd, &setmax);
        mu32flag = 1;
    }

    /* If operand is a NaN then set invalid operation
       and return zero result */
    if (decNumberIsNaN(b))
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (U32)0;
    }

    /* Round decimal number to integer using current rounding mode */
    decNumberToIntegralValue(&p, b, pset);

    /* If rounded value is less than zero then set invalid operation
       and return zero result */
    if (decNumberIsNegative(&p))
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (U32)0;
    }

    /* If rounded value is greater than maximum unsigned number
       (including where operand is positive infinity) then set
       invalid operation and return maximum unsigned result */
    decNumberCompare(&c, &p, &mu32dn, pset);
    if (decNumberIsNegative(&c) == 0 && decNumberIsZero(&c) == 0)
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (U32)mu32;
    }

    /* Raise inexact condition if result was rounded */
    decNumberCompare(&c, &p, b, pset);
    if (decNumberIsZero(&c) == 0)
    {
        pset->status |= DEC_IEEE_854_Inexact;
        if (decNumberIsNegative(&c) == decNumberIsNegative(b))
            pset->status |= DEC_Rounded;
    }

    /* Convert decimal number structure to packed decimal */
    decPackedFromNumber(packed, sizeof(packed), &scale, &p);

    /* Convert packed decimal to binary value */
    for (i = 0, n = 0; i < sizeof(packed)-1; i++)
    {
        n = n * 10 + ((packed[i] & 0xF0) >> 4);
        n = n * 10 + (packed[i] & 0x0F);
    }
    n = n * 10 + ((packed[i] & 0xF0) >> 4);
    while (scale++) n *= 10;

    /* Return 32-bit unsigned result */
    return n;

} /* end function dfp_number_to_u32 */

/*-------------------------------------------------------------------*/
/* Convert decimal number to 64-bit unsigned binary integer          */
/*                                                                   */
/* This subroutine is called by the CLGDTR and CLGXTR instructions.  */
/* It converts a decimal number structure to a 64-bit unsigned       */
/* binary integer value.  The inexact condition will be set in       */
/* the decimal context structure if the number is rounded to         */
/* an integer. The invalid operation condition will be set if        */
/* the decimal value is outside the range of a 64-bit unsigned int.  */
/*                                                                   */
/* Input:                                                            */
/*      b       Pointer to decimal number structure                  */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The return value is the 64-bit unsigned integer result       */
/*-------------------------------------------------------------------*/
static U64
dfp_number_to_u64(decNumber *b, decContext *pset)
{
U64             n;                      /* 64-bit unsigned result    */
int32_t         scale;                  /* Scaling factor            */
unsigned        i;                      /* Array subscript           */
BYTE            packed[17];             /* 33-digit packed work area */
decNumber       p, c;                   /* Working decimal numbers   */
static U64      mu64 = 0xFFFFFFFFFFFFFFFFULL;    /* Max unsigned int */
static char     mu64zd[]="18446744073709551615"; /* Max zoned dec    */
static BYTE     mu64flag = 0;           /* 1=mu64dn is initialized   */
static decNumber mu64dn;                /* Decimal maximum unsigned  */
decContext      setmax;                 /* Working context           */

    /* Prime the decimal number structure representing the maximum
       unsigned number representable in 64 bits. Use a 128-bit DFP
       working context because this number is too big to be
       represented in the 32-bit or 64-bit DFP format */
    if (mu64flag == 0)
    {
        decContextDefault(&setmax, DEC_INIT_DECIMAL128);
        decNumberFromString(&mu64dn, mu64zd, &setmax);
        mu64flag = 1;
    }

    /* If operand is a NaN then set invalid operation
       and return zero result */
    if (decNumberIsNaN(b))
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (U64)0;
    }

    /* Round decimal number to integer using current rounding mode */
    decNumberToIntegralValue(&p, b, pset);

    /* If rounded value is less than zero then set invalid operation
       and return zero result */
    if (decNumberIsNegative(&p))
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (U64)0;
    }

    /* If rounded value is greater than maximum unsigned number
       (including where operand is positive infinity) then set
       invalid operation and return maximum unsigned result */
    decNumberCompare(&c, &p, &mu64dn, pset);
    if (decNumberIsNegative(&c) == 0 && decNumberIsZero(&c) == 0)
    {
        pset->status |= DEC_IEEE_854_Invalid_operation;
        return (U64)mu64;
    }

    /* Raise inexact condition if result was rounded */
    decNumberCompare(&c, &p, b, pset);
    if (decNumberIsZero(&c) == 0)
    {
        pset->status |= DEC_IEEE_854_Inexact;
        if (decNumberIsNegative(&c) == decNumberIsNegative(b))
            pset->status |= DEC_Rounded;
    }

    /* Convert decimal number structure to packed decimal */
    decPackedFromNumber(packed, sizeof(packed), &scale, &p);

    /* Convert packed decimal to binary value */
    for (i = 0, n = 0; i < sizeof(packed)-1; i++)
    {
        n = n * 10 + ((packed[i] & 0xF0) >> 4);
        n = n * 10 + (packed[i] & 0x0F);
    }
    n = n * 10 + ((packed[i] & 0xF0) >> 4);
    while (scale++) n *= 10;

    /* Return 64-bit unsigned result */
    return n;

} /* end function dfp_number_to_u64 */

#define _DFP_FPE_ARCH_INDEPENDENT_
#endif /*!defined(_DFP_FPE_ARCH_INDEPENDENT_)*/
#endif /* defined( FEATURE_037_FP_EXTENSION_FACILITY ) */

#if defined(FEATURE_048_DFP_ZONE_CONV_FACILITY)
#if !defined(_DFP_ZONED_ARCH_INDEPENDENT_)
#define CDZT_MAXLEN 16                  /* CDZT maximum operand len  */
#define CXZT_MAXLEN 34                  /* CXZT maximum operand len  */
/*-------------------------------------------------------------------*/
/* Convert zoned decimal to decimal number                           */
/*                                                                   */
/* This subroutine is called by the CDZT and CXZT instructions.      */
/* It converts a zoned decimal value into a decimal number           */
/* structure. The length of the zoned decimal value should not       */
/* exceed the maximum number of digits specified in the context.     */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*      zoned   zoned decimal input area                             */
/*      len     length-1 of zoned decimal input                      */
/*      mask    mask field: 0x8=signed                               */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The decimal number structure is updated.                     */
/*      Return value: 0=success, 1=data exception                    */
/*-------------------------------------------------------------------*/
static int
dfp_number_from_zoned(decNumber *dn, char *zoned, int len,
                int mask, decContext *pset)
{
int             i;                      /* Array subscript           */
int             signflag;               /* 1=signed decimal operand  */
char            zwork[1+CXZT_MAXLEN+1]; /* Sign + digits + null      */
char            *pzw = zwork;           /* Addr next byte in zwork   */
char            c;                      /* Character work area       */

    /* Set the signed flag if indicated by the mask */
    signflag = (mask & 0x8) ? 1 : 0;

    /* Set the sign according to the operand sign code */
    if (signflag) {
        switch ((zoned[len] & 0xF0) >> 4) {
        case 0xB: case 0xD:
            *pzw++ = '-'; break;
        case 0xA: case 0xC: case 0xE: case 0xF:
            break;
        default:
            /* Data exception if invalid sign code */
            return 1;
        }
    }

    /* Convert zoned number to a decimal string */
    for (i=0; i <= len; i++)
    {
        c = zoned[i] & 0x0F;
        /* Data exception if invalid digit */
        if (c > 0x09) return 1;
        *pzw++ = c + '0';
    }
    *pzw = '\0';

    /* Convert decimal string to decimal number structure */
    decNumberFromString(dn, zwork, pset);

    return 0;

} /* end function dfp_number_from_zoned */

#define CZDT_MAXLEN 16                  /* CZDT maximum operand len  */
#define CZXT_MAXLEN 34                  /* CZXT maximum operand len  */
/*-------------------------------------------------------------------*/
/* Convert decimal number to zoned decimal                           */
/*                                                                   */
/* This subroutine is called by the CZDT and CZXT instructions.      */
/* It converts a decimal number to a zoned decimal string.           */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*              representing original DFP value                      */
/*      dc      Pointer to decimal number structure                  */
/*              containing only coeffecient of DFP value             */
/*      zoned   zoned decimal output area                            */
/*      len     length-1 of zoned decimal output area                */
/*      mask    mask field: 0x8=signed, 0x4=ASCII,                   */
/*              0x2=plus sign is F, 0x1=zero is always positive      */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The zoned decimal area is updated.                           */
/*      Return value is the condition code:                          */
/*      0=zero; 1=negative; 2=positive; 3=Inf, NaN, or overflow      */
/*-------------------------------------------------------------------*/
static int
dfp_number_to_zoned(decNumber *dn, decNumber *dc, char *zoned, int len,
                int mask, decContext *pset)
{
int             i;                      /* Array subscript           */
int             pad;                    /* Number of padding bytes   */
int             cc;                     /* Condition code            */
int             sign;                   /* 1=negative number         */
char            zwork[MAXDECSTRLEN+64]; /* Decimal string work area  */
int             zwlen;                  /* Length of zwork string    */
int             zwind;                  /* Index into zwork string   */

    UNREFERENCED(pset);

    /* Determine if the number is negative or positive */
    sign = (decNumberIsNegative(dn)) ? 1 : 0;

    /* Force plus zero if mask bit 3 is set */
    if ((mask & 0x1) && decNumberIsZero(dn)) sign = 0;

    /* Convert decimal number to string */
    if (decNumberIsNaN(dn) || (decNumberIsInfinite(dn)))
    {
        /* For NaN or Inf set cc=3 and use coefficient only */
        cc = 3;
        dc->exponent = 0;
        dc->bits &= ~(DECNEG);
        decNumberToString(dc, zwork);
    } else {
        /* For finite numbers set cc=0, 1, or 2 and convert digits */
        cc = (decNumberIsZero(dn)) ? 0 :
            (decNumberIsNegative(dn)) ? 1 : 2;
        dn->exponent = 0;
        dn->bits &= ~(DECNEG);
        decNumberToString(dn, zwork);
    }

    /* Calculate the number of padding bytes needed, and set
       condition code 3 if significant digits will be lost */
    zwlen = (int)(strlen(zwork));
    if (zwlen <= len + 1)
    {
        zwind = 0;
        pad = len + 1 - zwlen;
    } else {
        zwind = zwlen - len - 1;
        pad = 0;
        cc = 3;
    }

    /* Copy digits to zoned decimal result area */
    for (i = 0; i <= len; i++)
    {
        /* Pad with zero or copy digit from work string */
        zoned[i] = (pad > 0) ? 0x00 : zwork[zwind++] - '0';
        if (pad > 0) pad--;
        /* Set ASCII or EBCDIC zone according to mask bit 1 */
        zoned[i] |= (mask & 0x4) ? 0x30 : 0xF0;
    }

    /* Replace final zone by EBCDIC sign if mask bit 0 is one */
    if (mask & 0x8)
    {
        /* -ve sign is D, +ve sign is F or C depending on mask bit 2 */
        zoned[i-1] &= 0x0F;
        zoned[i-1] |= (sign) ? 0xD0 : (mask & 0x2) ? 0xF0 : 0xC0;
    }

    /* Return the condition code */
    return cc;

} /* end function dfp_number_to_zoned */

#define _DFP_ZONED_ARCH_INDEPENDENT_
#endif /*!defined(_DFP_ZONED_ARCH_INDEPENDENT_)*/
#endif /*defined(FEATURE_048_DFP_ZONE_CONV_FACILITY)*/

#if defined(FEATURE_080_DFP_PACK_CONV_FACILITY)
#if !defined(_DFP_PACKED_ARCH_INDEPENDENT_)
#define CDPT_MAXLEN 9                   /* CDPT maximum operand len  */
#define CXPT_MAXLEN 18                  /* CXPT maximum operand len  */
/*-------------------------------------------------------------------*/
/* Convert packed decimal to decimal number                           */
/*                                                                   */
/* This subroutine is called by the CDPT and CXPT instructions.      */
/* It converts a packed decimal value into a decimal number          */
/* structure. The length of the packed decimal value should not      */
/* exceed the maximum number of digits specified in the context.     */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*      packed  packed decimal input area                            */
/*      len     length-1 of packed decimal input                     */
/*      mask    mask field: 0x8=     signed                          */
/*                          0x1=     Ignore sign                     */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The decimal number structure is updated.                     */
/*      Return value: 0=success, 1=data exception                    */
/*-------------------------------------------------------------------*/

/* mask (m3) bits */
#define m_S     (mask & 0x8)    /* m - Sign Control            - m3 bit 0 */
#define m_P     (mask & 0x2)    /* m - Plus-Sign-Code Control  - m3 bit 2 */
#define m_F     (mask & 0x1)    /* m - Force-Plus-Zero Control - m3 bit 3 */
#define m_I     (mask & 0x1)    /* m - Ignore sign             - m3 bit 3 */

static int
dfp_number_from_packed(decNumber* dn, char* packed, int len,
    int mask, decContext* pset)
{
    int             i;                      /* Array subscript           */
    int             signflag;               /* 1=signed decimal operand  */
    int             ignoreflag;             /* 1=ignore sign             */
    char            pwork[1 + (CXPT_MAXLEN*2) + 1]; /* Sign + digits + null      */
    char*           ppw = pwork;            /* Addr next byte in pwork   */
    char            c;                      /* Character work area       */

    /* Set the signed & ignore flags as indicated by the mask */
    signflag   = m_S ? 1 : 0;
    ignoreflag = m_I ? 1 : 0;

    /* Set the sign according to the operand sign code */
    if (signflag) {
        if (!ignoreflag) {
            switch ((packed[len] & 0x0F)) {
            case 0xB: case 0xD:
                *ppw++ = '-'; break;
            case 0xA: case 0xC: case 0xE: case 0xF:
                break;
            default:
                /* Data exception if invalid sign code */
                return 1;
            }
        }
    }

    /* Convert packed number to a decimal string */
    for (i = 0; i <= len; i++)
    {
        /* 1st digit of byte */
        c = (packed[i] & 0xF0) >> 4;
        /* Data exception if invalid digit */
        if (c > 0x09) return 1;
        *ppw++ = c + '0';

        if (i == len && signflag) break;   /* is last nibble a sign, already done */

        /* 2nd digit of byte */
        c = packed[i] & 0x0F;
        /* Data exception if invalid digit */
        if (c > 0x09) return 1;
        *ppw++ = c + '0';
    }
    *ppw = '\0';

    /* Convert decimal string to decimal number structure */
    decNumberFromString(dn, pwork, pset);

    return 0;

} /* end function dfp_number_from_packed */

#define CPDT_MAXLEN 9                   /* CPDT maximum operand len  */
#define CPXT_MAXLEN 18                  /* CPXT maximum operand len  */
/*-------------------------------------------------------------------*/
/* Convert decimal number to packed decimal                          */
/*                                                                   */
/* This subroutine is called by the CPDT and CPXT instructions.      */
/* It converts a decimal number to a packed decimal string.          */
/*                                                                   */
/* Input:                                                            */
/*      dn      Pointer to decimal number structure                  */
/*              representing original DFP value                      */
/*      dc      Pointer to decimal number structure                  */
/*              containing only coeffecient of DFP value             */
/*      packed  packed decimal output area                           */
/*      len     length-1 of packed decimal output area               */
/*      mask    mask field: 0x8=signed, 0x4=reserved,                */
/*              0x2=plus sign is F, 0x1=zero is always positive      */
/*      pset    Pointer to decimal number context structure          */
/* Output:                                                           */
/*      The packed decimal area is updated.                          */
/*      Return value is the condition code:                          */
/*      0=zero; 1=negative; 2=positive; 3=Inf, NaN, or overflow      */
/*-------------------------------------------------------------------*/

static int
dfp_number_to_packed(decNumber* dn, decNumber* dc, char* packed, int len,
    int mask, decContext* pset)
{
    int             i;                      /* Array subscript           */
    int             pad;                    /* Number of padding bytes   */
    int             cc;                     /* Condition code            */
    char            pwork[MAXDECSTRLEN + 64]; /* Decimal string work area  */
    int             pwlen;                  /* Length of pwork string    */
    int             pwind;                  /* Index into pwork string   */
    int             pSigned;                /* packed field is signed    */
    unsigned char   pSign = 0x0C;           /* packed sign               */
    int             pDigits;                /* number of packed digits   */

    UNREFERENCED(pset);

    /* determine whether packed value is signed and the sign field */
    pSigned = (m_S) ? 1 : 0;
    if (pSigned) {
        if (decNumberIsNegative(dn)) {
            pSign = 0x0D;
            /* -0 : force positive sign based on plus-sign-control */
            if ( m_F && decNumberIsZero(dn))
                pSign =  (m_P) ? 0x0F : 0x0C;      /* 0b1111 : 0b1100 */
        }
        else {
            /* select plus sign */
            pSign = (m_P) ? 0x0F : 0x0C;            /* 0b1111 : 0b1100 */
        }
    }

    /* the number of packed digits */
    pDigits = 2 * (len + 1) - pSigned;

    /* Convert decimal number to string */
    /* only sigificant digits, ignore exponent and sign */
    if (decNumberIsNaN(dn) || (decNumberIsInfinite(dn)))
    {
        /* For NaN or Inf set cc=3 and use coefficient only */
        cc = 3;
        dc->exponent = 0;
        dc->bits &= ~(DECNEG);
        decNumberToString(dc, pwork);
    }
    else {
        /* For finite numbers set cc=0, 1, or 2 and convert digits */
        cc = (decNumberIsZero(dn)) ? 0 :
            (decNumberIsNegative(dn)) ? 1 : 2;
        dn->exponent = 0;
        dn->bits &= ~(DECNEG);
        decNumberToString(dn, pwork);
    }

    /* Calculate the number of padding digits needed, and set
       condition code 3 if significant digits will be lost */
    pwlen = (int)(strlen(pwork));
    if (pwlen <= pDigits)
    {
        pwind = 0;
        pad =  pDigits - pwlen;
    }
    else {
        pwind = pwlen - pDigits ;
        pad = 0;
        cc = 3;
    }

    /* Copy digits to packed decimal result area */
    for (i = 0; i <= len; i++)
    {
        /* Pad with zero or copy digit from work string */
        packed[i]  = (((pad-- > 0) ? 0x00 : pwork[pwind++] - '0') & 0x0F ) << 4 ;
        packed[i] |=  ((pad-- > 0) ? 0x00 : pwork[pwind++] - '0') & 0x0F;
    }

    /* if signed, insert sign in final nibble */
    if (pSigned)
    {
        packed[len] &= 0xF0;
        packed[len] |= pSign;
    }

    /* Return the condition code */
    return cc;

} /* end function dfp_number_to_packed */

#undef m_S     /* m - Sign Control            - m3 bit 0 */
#undef m_P     /* m - Plus-Sign-Code  Control - m3 bit 2 */
#undef m_F     /* m - Force-Plus-Zero Control - m3 bit 3 */
#undef m_I     /* m - Ignore sign             - m3 bit 3 */

#define _DFP_PACKED_ARCH_INDEPENDENT_
#endif /*!defined(_DFP_PACKED_ARCH_INDEPENDENT_)*/
#endif /*defined(FEATURE_080_DFP_PACK_CONV_FACILITY)*/

/*-------------------------------------------------------------------*/
/* Set rounding mode in decimal context structure                    */
/*                                                                   */
/* Input:                                                            */
/*      pset    Pointer to decimal number context structure          */
/*      mask    4-bit mask value                                     */
/*      regs    CPU register context                                 */
/* Output:                                                           */
/*      If mask bit X'08' is one then the rounding mode in the       */
/*      context structure is set according to the value (0 to 7)     */
/*      indicated by the low-order three bits of the mask.           */
/*      If mask bit X'08' is zero then the rounding mode in the      */
/*      context structure is set according to the value (0 to 7)     */
/*      of the DRM field in the FPC register.                        */
/*-------------------------------------------------------------------*/
static inline void
ARCH_DEP(dfp_rounding_mode) (decContext *pset, int mask, REGS *regs)
{
BYTE    drm;                            /* Decimal rounding mode     */

    /* Load DRM from mask or from FPC register */
    if (mask & 0x08)
        drm = mask & 0x07;
    else
        drm = (regs->fpc & FPC_DRM) >> FPC_DRM_SHIFT;

    /* Set rounding mode according to DRM value */
    switch (drm) {
    case DRM_RNE:  pset->round = DEC_ROUND_HALF_EVEN; break;
    case DRM_RTZ:  pset->round = DEC_ROUND_DOWN; break;
    case DRM_RTPI: pset->round = DEC_ROUND_CEILING; break;
    case DRM_RTMI: pset->round = DEC_ROUND_FLOOR; break;
    case DRM_RNAZ: pset->round = DEC_ROUND_HALF_UP; break;
    case DRM_RNTZ: pset->round = DEC_ROUND_HALF_DOWN; break;
    case DRM_RAFZ: pset->round = DEC_ROUND_UP; break;
    case DRM_RFSP:
    /* Rounding mode DRM_RFSP is not supported by
       the decNumber library, so we arbitrarily
       convert it to another mode instead... */
        pset->round = DEC_ROUND_DOWN; break;
    } /* end switch(drm) */

} /* end function dfp_rounding_mode */

/*-------------------------------------------------------------------*/
/* Copy a DFP short register into a decimal32 structure              */
/*                                                                   */
/* Input:                                                            */
/*      rn      FP register number                                   */
/*      xp      Pointer to decimal32 structure                       */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static inline void
ARCH_DEP(dfp_reg_to_decimal32) (int rn, decimal32 *xp, REGS *regs)
{
FW      *fwp;                           /* Fullword pointer          */

    fwp = (FW*)xp;                      /* Convert to FW pointer     */
    fwp->F = regs->FPR_S(rn);           /* Copy FPR bits 0-31        */

} /* end function dfp_reg_to_decimal32 */

/*-------------------------------------------------------------------*/
/* Load a DFP short register from a decimal32 structure              */
/*                                                                   */
/* Input:                                                            */
/*      rn      FP register number (left register of pair)           */
/*      xp      Pointer to decimal32 structure                       */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static inline void
ARCH_DEP(dfp_reg_from_decimal32) (int rn, decimal32 *xp, REGS *regs)
{
FW      *fwp;                           /* Fullword pointer          */

    fwp = (FW*)xp;                      /* Convert to FW pointer     */
    regs->FPR_S(rn) = fwp->F;           /* Load FPR bits 0-31        */

} /* end function dfp_reg_from_decimal32 */

/*-------------------------------------------------------------------*/
/* Copy a DFP long register into a decimal64 structure               */
/*                                                                   */
/* Input:                                                            */
/*      rn      FP register number                                   */
/*      xp      Pointer to decimal64 structure                       */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static inline void
ARCH_DEP(dfp_reg_to_decimal64) (int rn, decimal64 *xp, REGS *regs)
{
DW      *dwp;                           /* Doubleword pointer        */

    dwp = (DW*)xp;                      /* Convert to DW pointer     */
    dwp->D = regs->FPR_L(rn);           /* Copy FPR bits 0-63        */

} /* end function dfp_reg_to_decimal64 */

/*-------------------------------------------------------------------*/
/* Load a DFP long register from a decimal64 structure               */
/*                                                                   */
/* Input:                                                            */
/*      rn      FP register number (left register of pair)           */
/*      xp      Pointer to decimal64 structure                       */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static inline void
ARCH_DEP(dfp_reg_from_decimal64) (int rn, decimal64 *xp, REGS *regs)
{
DW      *dwp;                           /* Doubleword pointer        */

    dwp = (DW*)xp;                      /* Convert to DW pointer     */
    regs->FPR_L(rn) = dwp->D;           /* Load FPR bits 0-63        */

} /* end function dfp_reg_from_decimal64 */

/*-------------------------------------------------------------------*/
/* Copy a DFP extended register into a decimal128 structure          */
/*                                                                   */
/* Input:                                                            */
/*      rn      FP register number (left register of pair)           */
/*      xp      Pointer to decimal128 structure                      */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static inline void
ARCH_DEP(dfp_reg_to_decimal128) (int rn, decimal128 *xp, REGS *regs)
{
QW      *qwp;                           /* Quadword pointer          */

    qwp = (QW*)xp;                      /* Convert to QW pointer     */
    qwp->F.HH.F = regs->FPR_L(rn) >> 32;    /* Copy FPR bits 0-31    */
    qwp->F.HL.F = regs->FPR_L(rn);          /* Copy FPR bits 32-63   */
    qwp->F.LH.F = regs->FPR_L(rn+2) >> 32;  /* Copy FPR bits 64-95   */
    qwp->F.LL.F = regs->FPR_L(rn+2);        /* Copy FPR bits 96-127  */

} /* end function dfp_reg_to_decimal128 */

/*-------------------------------------------------------------------*/
/* Load a DFP extended register from a decimal128 structure          */
/*                                                                   */
/* Input:                                                            */
/*      rn      FP register number (left register of pair)           */
/*      xp      Pointer to decimal128 structure                      */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static inline void
ARCH_DEP(dfp_reg_from_decimal128) (int rn, decimal128 *xp, REGS *regs)
{
QW      *qwp;                           /* Quadword pointer          */

    qwp = (QW*)xp;                      /* Convert to QW pointer     */
    regs->FPR_L(rn) = (U64)qwp->F.HH.F << 32 | qwp->F.HL.F;
                                        /* Copy FPR bits 0-63        */
    regs->FPR_L(rn+2) = (U64)qwp->F.LH.F << 32 | qwp->F.LL.F;
                                        /* Copy FPR bits 64-127      */

} /* end function dfp_reg_from_decimal128 */

/*-------------------------------------------------------------------*/
/* Check for DFP exception conditions                                */
/*                                                                   */
/* This subroutine is called by the DFP instruction processing       */
/* routines after the calculation has been performed but before      */
/* the result is loaded into the result register (or before any      */
/* storage location is updated, as the case may be).                 */
/*                                                                   */
/* The purpose of this subroutine is to check whether any DFP        */
/* exception conditions are indicated by the decimal context         */
/* structure, and to initiate the appropriate action if so.          */
/*                                                                   */
/* Input:                                                            */
/*      set     Decimal number context                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/* Output:                                                           */
/*      Return value is DXC (data exception code) or zero.           */
/*                                                                   */
/* When no exception conditions are indicated, the return value      */
/* is zero indicating that the instruction may proceed normally.     */
/*                                                                   */
/* When an exception condition exists and the corresponding mask     */
/* bit in the FPC register is zero, then the corresponding flag      */
/* bit is set in the FPC register. The return value is zero to       */
/* indicate that the instruction may proceed normally.               */
/*                                                                   */
/* When an exception condition exists and the corresponding mask     */
/* bit in the FPC register is one, then the DXC is set according     */
/* to the type of exception, and one of two actions is taken:        */
/* - if the exception is of a type which causes the instruction      */
/*   to be suppressed, then this subroutine raises a program         */
/*   exception and does not return to the calling instruction        */
/* - if the exception is of a type which causes the instruction      */
/*   to be completed, then this subroutine returns with the          */
/*   DXC code as its return value. The calling instruction will      */
/*   then raise a program exception after storing its results.       */
/*-------------------------------------------------------------------*/
static BYTE
ARCH_DEP(dfp_status_check) (decContext *pset, REGS *regs)
{
BYTE    dxc = 0;                        /* Data exception code       */
int     suppress = 0;                   /* 1=suppress, 0=complete    */

    if (pset->status & DEC_IEEE_854_Invalid_operation)
    {
        /* An IEEE-invalid-operation condition was recognized */
        if ((regs->fpc & FPC_MASK_IMI) == 0)
        {
            regs->fpc |= FPC_FLAG_SFI;
        }
        else
        {
            dxc = DXC_IEEE_INVALID_OP;
            suppress = 1;
        }
    }
    else if (pset->status & DEC_IEEE_854_Division_by_zero)
    {
        /* An IEEE-division-by-zero condition was recognized */
        if ((regs->fpc & FPC_MASK_IMZ) == 0)
        {
            /* Division-by-zero mask is zero */
            regs->fpc |= FPC_FLAG_SFZ;
        }
        else
        {
            /* Division-by-zero mask is one */
            dxc = DXC_IEEE_DIV_ZERO;
            suppress = 1;
        }
    }
    else if (pset->status & DEC_IEEE_854_Overflow)
    {
        /* An IEEE-overflow condition was recognized */
        if ((regs->fpc & FPC_MASK_IMO) == 0)
        {
            /* Overflow mask is zero */
            regs->fpc |= FPC_FLAG_SFO;
        }
        else
        {
            /* Overflow mask is one */
            dxc = (pset->status & DEC_IEEE_854_Inexact) ?
                    ((pset->status & DEC_Rounded) ?
                        DXC_IEEE_OF_INEX_INCR :
                        DXC_IEEE_OF_INEX_TRUNC ) :
                        DXC_IEEE_OF_EXACT ;
        }
    }
    else if (pset->status & DEC_IEEE_854_Underflow)
    {
        /* An IEEE-underflow condition was recognized */
        if ((regs->fpc & FPC_MASK_IMU) == 0)
        {
            /* Underflow mask is zero */
            if (pset->status & DEC_IEEE_854_Inexact)
            {
                if ((regs->fpc & FPC_MASK_IMX) == 0)
                {
                    /* Inexact result with inexact mask zero */
                    regs->fpc |= (FPC_FLAG_SFU | FPC_FLAG_SFX);
                }
                else
                {
                    /* Inexact result with inexact mask one */
                    regs->fpc |= FPC_FLAG_SFU;
                    dxc = (pset->status & DEC_Rounded) ?
                            DXC_IEEE_INEXACT_INCR :
                            DXC_IEEE_INEXACT_TRUNC ;
                }
            }
        }
        else
        {
            /* Underflow mask is one */
            if (pset->status & DEC_IEEE_854_Inexact)
            {
                /* Underflow with inexact result */
                dxc = (pset->status & DEC_Rounded) ?
                        DXC_IEEE_UF_INEX_INCR :
                        DXC_IEEE_UF_INEX_TRUNC ;
            }
            else
            {
                /* Underflow with exact result */
                dxc = DXC_IEEE_UF_EXACT;
            }
        }
    }
    else if (pset->status & DEC_IEEE_854_Inexact)
    {
        /* An IEEE-inexact condition was recognized */
        if ((regs->fpc & FPC_MASK_IMX) == 0)
        {
            /* Inexact mask is zero */
            regs->fpc |= FPC_FLAG_SFX;
        }
        else
        {
            /* Inexact mask is one */
            dxc = (pset->status & DEC_Rounded) ?
                    DXC_IEEE_INEXACT_INCR :
                    DXC_IEEE_INEXACT_TRUNC ;
        }
    }

    /* If suppression is indicated, raise a data exception */
    if (suppress)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Otherwise return to complete the instruction */
    return dxc;

} /* end function dfp_status_check */


/*-------------------------------------------------------------------*/
/* B3DA AXTR  - Add DFP Extended Register                    [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_dfp_ext_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal128      x1, x2, x3;             /* Extended DFP values       */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR3_CHECK(r1, r2, r3, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Add FP register r3 to FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);
    decimal128ToNumber(&x2, &d2);
    decimal128ToNumber(&x3, &d3);
    decNumberAdd(&d1, &d2, &d3, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&d1) ? 3 :
                   decNumberIsZero(&d1) ? 0 :
                   decNumberIsNegative(&d1) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(add_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3D2 ADTR  - Add DFP Long Register                        [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_dfp_long_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal64       x1, x2, x3;             /* Long DFP values           */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Add FP register r3 to FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);
    decimal64ToNumber(&x2, &d2);
    decimal64ToNumber(&x3, &d3);
    decNumberAdd(&d1, &d2, &d3, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&d1) ? 3 :
                   decNumberIsZero(&d1) ? 0 :
                   decNumberIsNegative(&d1) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(add_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3EC CXTR  - Compare DFP Extended Register                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal128      x1, x2;                 /* Extended DFP values       */
decNumber       d1, d2, dr;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Compare FP register r1 with FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r1, &x1, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x1, &d1);
    decimal128ToNumber(&x2, &d2);
    decNumberCompare(&dr, &d1, &d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&dr) ? 3 :
                   decNumberIsZero(&dr) ? 0 :
                   decNumberIsNegative(&dr) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3E4 CDTR  - Compare DFP Long Register                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal64       x1, x2;                 /* Long DFP values           */
decNumber       d1, d2, dr;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Compare FP register r1 with FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r1, &x1, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x1, &d1);
    decimal64ToNumber(&x2, &d2);
    decNumberCompare(&dr, &d1, &d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&dr) ? 3 :
                   decNumberIsZero(&dr) ? 0 :
                   decNumberIsNegative(&dr) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3E8 KXTR  - Compare and Signal DFP Extended Register       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_signal_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal128      x1, x2;                 /* Extended DFP values       */
decNumber       d1, d2, dr;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Compare FP register r1 with FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r1, &x1, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x1, &d1);
    decimal128ToNumber(&x2, &d2);
    decNumberCompare(&dr, &d1, &d2, &set);

    /* Force signaling condition if result is a NaN */
    if (decNumberIsNaN(&dr))
        set.status |= DEC_IEEE_854_Invalid_operation;

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&dr) ? 3 :
                   decNumberIsZero(&dr) ? 0 :
                   decNumberIsNegative(&dr) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_and_signal_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3E0 KDTR  - Compare and Signal DFP Long Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_signal_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal64       x1, x2;                 /* Long DFP values           */
decNumber       d1, d2, dr;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Compare FP register r1 with FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r1, &x1, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x1, &d1);
    decimal64ToNumber(&x2, &d2);
    decNumberCompare(&dr, &d1, &d2, &set);

    /* Force signaling condition if result is a NaN */
    if (decNumberIsNaN(&dr))
        set.status |= DEC_IEEE_854_Invalid_operation;

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&dr) ? 3 :
                   decNumberIsZero(&dr) ? 0 :
                   decNumberIsNegative(&dr) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_and_signal_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3FC CEXTR - Compare Exponent DFP Extended Register         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_exponent_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal128      x1, x2;                 /* Extended DFP values       */
decNumber       d1, d2;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Convert FP register values to numbers */
    ARCH_DEP(dfp_reg_to_decimal128)(r1, &x1, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x1, &d1);
    decimal128ToNumber(&x2, &d2);

    /* Compare exponents and set condition code */
    regs->psw.cc = dfp_compare_exponent(&d1, &d2);

} /* end DEF_INST(compare_exponent_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3F4 CEDTR - Compare Exponent DFP Long Register             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_exponent_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal64       x1, x2;                 /* Long DFP values           */
decNumber       d1, d2;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Convert FP register values to numbers */
    ARCH_DEP(dfp_reg_to_decimal64)(r1, &x1, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x1, &d1);
    decimal64ToNumber(&x2, &d2);

    /* Compare exponents and set condition code */
    regs->psw.cc = dfp_compare_exponent(&d1, &d2);

} /* end DEF_INST(compare_exponent_dfp_long_reg) */


#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
/*-------------------------------------------------------------------*/
/* B959 CXFTR - Convert from fixed 32 to DFP Extended Register [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fix32_to_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
S32             n2;                     /* Value of R2 register      */
decimal128      x1;                     /* Extended DFP value        */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load 32-bit binary integer value from r2 register */
    n2 = (S32)(regs->GR_L(r2));

    /* Convert binary integer to extended DFP format */
    dfp_number_from_fix32(&d1, n2, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_fix32_to_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B951 CDFTR - Convert from fixed 32 to DFP Long Register     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fix32_to_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
S32             n2;                     /* Value of R2 register      */
decimal64       x1;                     /* Long DFP value            */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load 32-bit binary integer value from r2 register */
    n2 = (S32)(regs->GR_L(r2));

    /* Convert binary integer to long DFP format */
    dfp_number_from_fix32(&d1, n2, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(convert_fix32_to_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B95B CXLFTR - Convert from unsigned 32 to DFP Ext. Reg.   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_u32_to_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U32             n2;                     /* Value of R2 register      */
decimal128      x1;                     /* Extended DFP value        */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load 32-bit unsigned value from r2 register */
    n2 = regs->GR_L(r2);

    /* Convert unsigned binary integer to extended DFP format */
    dfp_number_from_u32(&d1, n2, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_u32_to_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B953 CDLFTR - Convert from unsigned 32 to DFP Long Reg.   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_u32_to_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U32             n2;                     /* Value of R2 register      */
decimal64       x1;                     /* Long DFP value            */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load 32-bit unsigned value from r2 register */
    n2 = regs->GR_L(r2);

    /* Convert unsigned binary integer to long DFP format */
    dfp_number_from_u32(&d1, n2, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(convert_u32_to_dfp_long_reg) */
#endif /* defined( FEATURE_037_FP_EXTENSION_FACILITY ) */


/*-------------------------------------------------------------------*/
/* B3F9 CXGTR - Convert from fixed 64 to DFP Extended Reg.   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fix64_to_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
S64             n2;                     /* Value of R2 register      */
decimal128      x1;                     /* Extended DFP value        */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Load 64-bit binary integer value from r2 register */
    n2 = (S64)(regs->GR_G(r2));

    /* Convert binary integer to extended DFP format */
    dfp_number_from_fix64(&d1, n2, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_fix64_to_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3F1 CDGTR - Convert from fixed 64 to DFP Long Register   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fix64_to_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
S64             n2;                     /* Value of R2 register      */
decimal64       x1;                     /* Long DFP value            */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Load 64-bit binary integer value from r2 register */
    n2 = (S64)(regs->GR_G(r2));

    /* Convert binary integer to long DFP format */
    dfp_number_from_fix64(&d1, n2, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_fix64_to_dfp_long_reg) */


#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
/*-------------------------------------------------------------------*/
/* B95A CXLGTR - Convert from unsigned 64 to DFP Ext Reg.    [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_u64_to_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U64             n2;                     /* Value of R2 register      */
decimal128      x1;                     /* Extended DFP value        */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load 64-bit unsigned value from r2 register */
    n2 = regs->GR_G(r2);

    /* Convert unsigned binary integer to extended DFP format */
    dfp_number_from_u64(&d1, n2, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_u64_to_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B952 CDLGTR - Convert from unsigned 64 to DFP Long Reg.   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_u64_to_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U64             n2;                     /* Value of R2 register      */
decimal64       x1;                     /* Long DFP value            */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load 64-bit unsigned value from r2 register */
    n2 = regs->GR_G(r2);

    /* Convert unsigned binary integer to long DFP format */
    dfp_number_from_u64(&d1, n2, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(convert_u64_to_dfp_long_reg) */
#endif /* defined( FEATURE_037_FP_EXTENSION_FACILITY ) */


/*-------------------------------------------------------------------*/
/* B3FB CXSTR - Convert from signed BCD (128-bit to DFP ext)   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_sbcd128_to_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal128      x1;                     /* Extended DFP values       */
decNumber       dwork, *dp;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            pwork[16];              /* 31-digit packed work area */
int32_t         scale = 0;              /* Scaling factor            */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);
    ODD_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Store general register pair in work area */
    STORE_DW(pwork, regs->GR_G(r2));
    STORE_DW(pwork+8, regs->GR_G(r2+1));

    /* Convert signed BCD to internal number format */
    dp = decPackedToNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Data exception if digits or sign was invalid */
    if (dp == NULL)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert internal number to DFP extended format */
    decimal128FromNumber(&x1, &dwork, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_sbcd128_to_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3F3 CDSTR - Convert from signed BCD (64-bit to DFP long)   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_sbcd64_to_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal64       x1;                     /* Long DFP values           */
decNumber       dwork, *dp;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            pwork[8];               /* 15-digit packed work area */
int32_t         scale = 0;              /* Scaling factor            */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Store general register in work area */
    STORE_DW(pwork, regs->GR_G(r2));

    /* Convert signed BCD to internal number format */
    dp = decPackedToNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Data exception if digits or sign was invalid */
    if (dp == NULL)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert internal number to DFP long format */
    decimal64FromNumber(&x1, &dwork, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(convert_sbcd64_to_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3FA CXUTR - Convert from unsigned BCD (128-bit to DFP ext) [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_ubcd128_to_dfp_ext_reg)
{
unsigned        i;                      /* Array subscript           */
int             r1, r2;                 /* Values of R fields        */
decimal128      x1;                     /* Extended DFP values       */
decNumber       dwork, *dp;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            pwork[17];              /* 33-digit packed work area */
int32_t         scale = 0;              /* Scaling factor            */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);
    ODD_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Store general register pair in work area */
    pwork[0] = 0;
    STORE_DW(pwork+1, regs->GR_G(r2));
    STORE_DW(pwork+9, regs->GR_G(r2+1));

    /* Convert unsigned BCD to signed BCD */
    for (i = 0; i < sizeof(pwork)-1; i++)
        pwork[i] = ((pwork[i] & 0x0F) << 4) | (pwork[i+1] >> 4);
    pwork[i] = ((pwork[i] & 0x0F) << 4) | 0x0F;

    /* Convert signed BCD to internal number format */
    dp = decPackedToNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Data exception if digits invalid */
    if (dp == NULL)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert internal number to DFP extended format */
    decimal128FromNumber(&x1, &dwork, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_ubcd128_to_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3F2 CDUTR - Convert from unsigned BCD (64-bit to DFP long) [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_ubcd64_to_dfp_long_reg)
{
unsigned        i;                      /* Array subscript           */
int             r1, r2;                 /* Values of R fields        */
decimal64       x1;                     /* Long DFP values           */
decNumber       dwork, *dp;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            pwork[9];               /* 17-digit packed work area */
int32_t         scale = 0;              /* Scaling factor            */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Store general register in work area */
    pwork[0] = 0;
    STORE_DW(pwork+1, regs->GR_G(r2));

    /* Convert unsigned BCD to signed BCD */
    for (i = 0; i < sizeof(pwork)-1; i++)
        pwork[i] = ((pwork[i] & 0x0F) << 4) | (pwork[i+1] >> 4);
    pwork[i] = ((pwork[i] & 0x0F) << 4) | 0x0F;

    /* Convert signed BCD to internal number format */
    dp = decPackedToNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Data exception if digits or sign was invalid */
    if (dp == NULL)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert internal number to DFP long format */
    decimal64FromNumber(&x1, &dwork, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(convert_ubcd64_to_dfp_long_reg) */


#if defined(FEATURE_048_DFP_ZONE_CONV_FACILITY)
/*-------------------------------------------------------------------*/
/* EDAB CXZT  - Convert from zoned to DFP Extended           [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_zoned_to_dfp_ext)
{
int             rc;                     /* Return code               */
int             r1, m3;                 /* Values of R and M fields  */
int             l2;                     /* Operand length minus 1    */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal128      x1;                     /* Extended DFP value        */
decNumber       d;                      /* Working decimal number    */
decContext      set;                    /* Working context           */
char            zoned[CXZT_MAXLEN];     /* Zoned decimal operand     */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Program check if operand length exceeds maximum */
    if (l2 > CXZT_MAXLEN-1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Fetch the zoned decimal operand into the work area */
    ARCH_DEP(vfetchc) (zoned, l2, effective_addr2, b2, regs);

    /* Convert zoned decimal to decimal number structure */
    rc = dfp_number_from_zoned(&d, zoned, l2, m3, &set);

    /* Program check if data exception is indicated */
    if (rc != 0)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert decimal number to extended DFP format */
    decimal128FromNumber(&x1, &d, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_zoned_to_dfp_ext) */


/*-------------------------------------------------------------------*/
/* EDAA CDZT  - Convert from zoned to DFP Long               [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_zoned_to_dfp_long)
{
int             rc;                     /* Return code               */
int             r1, m3;                 /* Values of R and M fields  */
int             l2;                     /* Operand length minus 1    */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal64       x1;                     /* Long DFP value            */
decNumber       d;                      /* Working decimal number    */
decContext      set;                    /* Working context           */
char            zoned[CDZT_MAXLEN];     /* Zoned decimal operand     */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Program check if operand length exceeds maximum */
    if (l2 > CDZT_MAXLEN-1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Fetch the zoned decimal operand into the work area */
    ARCH_DEP(vfetchc) (zoned, l2, effective_addr2, b2, regs);

    /* Convert zoned decimal to decimal number structure */
    rc = dfp_number_from_zoned(&d, zoned, l2, m3, &set);

    /* Program check if data exception is indicated */
    if (rc != 0)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert decimal number to long DFP format */
    decimal64FromNumber(&x1, &d, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(convert_zoned_to_dfp_long) */
#endif /*defined(FEATURE_048_DFP_ZONE_CONV_FACILITY)*/


#if defined(FEATURE_080_DFP_PACK_CONV_FACILITY)
/*-------------------------------------------------------------------*/
/* EDAF CXPT - Convert from packed to DFP Extended           [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_packed_to_dfp_ext)
{
    int             rc;                     /* Return code               */
    int             r1, m3;                 /* Values of R and M fields  */
    int             l2;                     /* Operand length minus 1    */
    int             b2;                     /* Base of effective addr    */
    VADR            effective_addr2;        /* Effective address         */
    decimal128      x1;                     /* Extended DFP value        */
    decNumber       d;                      /* Working decimal number    */
    decContext      set;                    /* Working context           */
    char            packed[CXPT_MAXLEN];    /* Packed decimal operand    */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK(regs, b2);

    TXF_FLOAT_INSTR_CHECK(regs);
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Program check if operand length exceeds maximum */
    if (l2 > CXPT_MAXLEN - 1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Fetch the packed decimal operand into the work area */
    ARCH_DEP(vfetchc) (packed, l2, effective_addr2, b2, regs);

    /* when l2 = 17; check that unused digits are 0 */
    rc = 0;
    if (l2 == 17) {
        if (m3 & 0x8) {
            /* signed - first nibble must be 0 */
            if (packed[0] & 0xF0)  rc = 1;
        }
        else {
            /* unsigned - first byte must be 0 */
            if (packed[0]) rc = 1;
        }
    }

    /* Convert packed decimal to decimal number structure */
    if (rc == 0)  rc = dfp_number_from_packed(&d, packed, l2, m3, &set);

    /* Program check if data exception is indicated */
    if (rc != 0)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert decimal number to extended DFP format */
    decimal128FromNumber(&x1, &d, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(convert_packed_to_dfp_ext) */


/*-------------------------------------------------------------------*/
/* EDAE CDPT  - Convert from packed to DFP Long              [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_packed_to_dfp_long)
{
    int             rc;                     /* Return code               */
    int             r1, m3;                 /* Values of R and M fields  */
    int             l2;                     /* Operand length minus 1    */
    int             b2;                     /* Base of effective addr    */
    VADR            effective_addr2;        /* Effective address         */
    decimal64       x1;                     /* Long DFP value            */
    decNumber       d;                      /* Working decimal number    */
    decContext      set;                    /* Working context           */
    char            packed[CDPT_MAXLEN];    /* Packed decimal operand    */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK(regs, b2);

    TXF_FLOAT_INSTR_CHECK(regs);
    DFPINST_CHECK(regs);

    /* Program check if operand length exceeds maximum */
    if (l2 > CDPT_MAXLEN - 1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Fetch the packed decimal operand into the work area */
    ARCH_DEP(vfetchc) (packed, l2, effective_addr2, b2, regs);

    /* when l2 = 8; check that unused digits are 0 */
    rc = 0;
    if (l2 == 8) {
        if (m3 & 0x8) {
            /* signed - first nibble must be 0 */
            if ( (packed[0] & 0xF0) != 0)  rc = 1;
        }
        else {
            /* unsigned - first byte must be 0 */
            if (packed[0] != 0)  rc = 1;
        }
    }

    /* Convert packed decimal to decimal number structure */
    if (rc == 0) rc = dfp_number_from_packed(&d, packed, l2, m3, &set);

    /* Program check if data exception is indicated */
    if (rc != 0)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Convert decimal number to long DFP format */
    decimal64FromNumber(&x1, &d, &set);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(convert_packed_to_dfp_long) */
#endif /*defined(FEATURE_080_DFP_PACK_CONV_FACILITY)*/


#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
/*-------------------------------------------------------------------*/
/* B949 CFXTR - Convert from DFP Extended Reg. to fixed 32   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_fix32_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
S32             n1;                     /* Result value              */
decimal128      x2;                     /* Extended DFP value        */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load extended DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &d2);

    /* Convert decimal number to 32-bit binary integer */
    n1 = dfp_number_to_fix32(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_L(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_ext_to_fix32_reg) */


/*-------------------------------------------------------------------*/
/* B941 CFDTR - Convert from DFP Long Register to fixed 32   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_fix32_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
S32             n1;                     /* Result value              */
decimal64       x2;                     /* Long DFP value            */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load long DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d2);

    /* Convert decimal number to 32-bit binary integer */
    n1 = dfp_number_to_fix32(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_L(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_long_to_fix32_reg) */


/*-------------------------------------------------------------------*/
/* B94B CLFXTR - Convert from DFP Ext Reg. to unsigned 32    [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_u32_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U32             n1;                     /* Result value              */
decimal128      x2;                     /* Extended DFP value        */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load extended DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &d2);

    /* Convert decimal number to 32-bit unsigned integer */
    n1 = dfp_number_to_u32(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_L(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_ext_to_u32_reg) */


/*-------------------------------------------------------------------*/
/* B943 CLFDTR - Convert from DFP Long Reg. to unsigned 32   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_u32_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U32             n1;                     /* Result value              */
decimal64       x2;                     /* Long DFP value            */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load long DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d2);

    /* Convert decimal number to 32-bit unsigned integer */
    n1 = dfp_number_to_u32(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_L(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_long_to_u32_reg) */
#endif /* defined( FEATURE_037_FP_EXTENSION_FACILITY ) */


/*-------------------------------------------------------------------*/
/* B3E9 CGXTR - Convert from DFP Extended Reg. to fixed 64   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_fix64_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3;                     /* Values of M fields        */
S64             n1;                     /* Result value              */
decimal128      x2;                     /* Extended DFP value        */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load extended DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &d2);

    /* Convert decimal number to 64-bit binary integer */
    n1 = dfp_number_to_fix64(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_G(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_ext_to_fix64_reg) */


/*-------------------------------------------------------------------*/
/* B3E1 CGDTR - Convert from DFP Long Register to fixed 64   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_fix64_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3;                     /* Values of M fields        */
S64             n1;                     /* Result value              */
decimal64       x2;                     /* Long DFP value            */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load long DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d2);

    /* Convert decimal number to 64-bit binary integer */
    n1 = dfp_number_to_fix64(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_G(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_long_to_fix64_reg) */


#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
/*-------------------------------------------------------------------*/
/* B94A CLGXTR - Convert from DFP Ext Reg. to unsigned 64    [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_u64_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U64             n1;                     /* Result value              */
decimal128      x2;                     /* Extended DFP value        */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load extended DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &d2);

    /* Convert decimal number to 64-bit unsigned integer */
    n1 = dfp_number_to_u64(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_G(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_ext_to_u64_reg) */


/*-------------------------------------------------------------------*/
/* B942 CLGDTR - Convert from DFP Long Reg. to unsigned 64   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_u64_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m3, m4;                 /* Values of M fields        */
U64             n1;                     /* Result value              */
decimal64       x2;                     /* Long DFP value            */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load long DFP value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d2);

    /* Convert decimal number to 64-bit unsigned integer */
    n1 = dfp_number_to_u64(&d2, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into general register r1 */
    regs->GR_G(r1) = n1;

    /* Set condition code */
    regs->psw.cc = (set.status & DEC_IEEE_854_Invalid_operation) ? 3 :
                   decNumberIsZero(&d2) ? 0 :
                   decNumberIsNegative(&d2) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(convert_dfp_long_to_u64_reg) */
#endif /* defined( FEATURE_037_FP_EXTENSION_FACILITY ) */


/*-------------------------------------------------------------------*/
/* B3EB CSXTR - Convert to signed BCD (DFP ext to 128-bit)   [RRF-d] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_sbcd128_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m4;                     /* Values of M fields        */
decimal128      x2;                     /* Extended DFP values       */
decNumber       dwork;                  /* Working decimal number    */
decContext      set;                    /* Working context           */
int32_t         scale;                  /* Scaling factor            */
BYTE            pwork[18];              /* 33-digit packed work area */

    RRF_M4(inst, regs, r1, r2, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);
    ODD_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &dwork);

    /* If NaN or Inf then use coefficient only */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp128_clear_cf_and_bxcf(&x2);
        decimal128ToNumber(&x2, &dwork);
    }

    /* Convert number to signed BCD in work area */
    decPackedFromNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Make the plus-sign X'F' if m4 bit 3 is one */
    if ((m4 & 0x01) && !decNumberIsNegative(&dwork))
        pwork[sizeof(pwork)-1] |= 0x0F;

    /* Load general register pair r1 and r1+1 from
       rightmost 31 packed decimal digits of work area */
    FETCH_DW(regs->GR_G(r1), pwork+sizeof(pwork)-16);
    FETCH_DW(regs->GR_G(r1+1), pwork+sizeof(pwork)-8);

} /* end DEF_INST(convert_dfp_ext_to_sbcd128_reg) */


/*-------------------------------------------------------------------*/
/* B3E3 CSDTR - Convert to signed BCD (DFP long to 64-bit)   [RRF-d] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_sbcd64_reg)
{
int             r1, r2;                 /* Values of R fields        */
int             m4;                     /* Values of M fields        */
decimal64       x2;                     /* Long DFP values           */
decNumber       dwork;                  /* Working decimal number    */
decContext      set;                    /* Working context           */
int32_t         scale;                  /* Scaling factor            */
BYTE            pwork[9];               /* 17-digit packed work area */

    RRF_M4(inst, regs, r1, r2, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &dwork);

    /* If NaN or Inf then use coefficient only */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp64_clear_cf_and_bxcf(&x2);
        decimal64ToNumber(&x2, &dwork);
    }

    /* Convert number to signed BCD in work area */
    decPackedFromNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Make the plus-sign X'F' if m4 bit 3 is one */
    if ((m4 & 0x01) && !decNumberIsNegative(&dwork))
        pwork[sizeof(pwork)-1] |= 0x0F;

    /* Load general register r1 from rightmost
       15 packed decimal digits of work area */
    FETCH_DW(regs->GR_G(r1), pwork+sizeof(pwork)-8);

} /* end DEF_INST(convert_dfp_long_to_sbcd64_reg) */


/*-------------------------------------------------------------------*/
/* B3EA CUXTR - Convert to unsigned BCD (DFP ext to 128-bit)   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_ubcd128_reg)
{
int             i;                      /* Array subscript           */
int             r1, r2;                 /* Values of R fields        */
decimal128      x2;                     /* Extended DFP values       */
decNumber       dwork;                  /* Working decimal number    */
decContext      set;                    /* Working context           */
int32_t         scale;                  /* Scaling factor            */
BYTE            pwork[17];              /* 33-digit packed work area */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);
    ODD_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &dwork);

    /* If NaN or Inf then use coefficient only */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp128_clear_cf_and_bxcf(&x2);
        decimal128ToNumber(&x2, &dwork);
    }

    /* Convert number to signed BCD in work area */
    decPackedFromNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Convert signed BCD to unsigned BCD */
    for (i = sizeof(pwork)-1; i > 0; i--)
        pwork[i] = (pwork[i] >> 4) | ((pwork[i-1] & 0x0F) << 4);

    /* Load general register pair r1 and r1+1 from
       rightmost 32 packed decimal digits of work area */
    FETCH_DW(regs->GR_G(r1), pwork+sizeof(pwork)-16);
    FETCH_DW(regs->GR_G(r1+1), pwork+sizeof(pwork)-8);

} /* end DEF_INST(convert_dfp_ext_to_ubcd128_reg) */


/*-------------------------------------------------------------------*/
/* B3E2 CUDTR - Convert to unsigned BCD (DFP long to 64-bit)   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_ubcd64_reg)
{
int             i;                      /* Array subscript           */
int             r1, r2;                 /* Values of R fields        */
decimal64       x2;                     /* Long DFP values           */
decNumber       dwork;                  /* Working decimal number    */
decContext      set;                    /* Working context           */
int32_t         scale;                  /* Scaling factor            */
BYTE            pwork[9];               /* 17-digit packed work area */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &dwork);

    /* If NaN or Inf then use coefficient only */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp64_clear_cf_and_bxcf(&x2);
        decimal64ToNumber(&x2, &dwork);
    }

    /* Convert number to signed BCD in work area */
    decPackedFromNumber(pwork, sizeof(pwork), &scale, &dwork);

    /* Convert signed BCD to unsigned BCD */
    for (i = sizeof(pwork)-1; i > 0; i--)
        pwork[i] = (pwork[i] >> 4) | ((pwork[i-1] & 0x0F) << 4);

    /* Load general register r1 from rightmost
       16 packed decimal digits of work area */
    FETCH_DW(regs->GR_G(r1), pwork+sizeof(pwork)-8);

} /* end DEF_INST(convert_dfp_long_to_ubcd64_reg) */


#if defined(FEATURE_048_DFP_ZONE_CONV_FACILITY)
/*-------------------------------------------------------------------*/
/* EDA9 CZXT  - Convert to zoned from DFP Extended           [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_zoned)
{
int             r1, m3;                 /* Values of R and M fields  */
int             l2;                     /* Operand length minus 1    */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal128      x1;                     /* Extended DFP value        */
decNumber       dwork, dcoeff;          /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int             cc;                     /* Condition code            */
char            zoned[CZXT_MAXLEN];     /* Zoned decimal result      */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Program check if operand length exceeds 34 */
    if (l2 > CZXT_MAXLEN-1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r1 */
    ARCH_DEP(dfp_reg_to_decimal128)(r1, &x1, regs);
    decimal128ToNumber(&x1, &dwork);

    /* Extract coefficient only for Inf and NaN */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp128_clear_cf_and_bxcf(&x1);
        decimal128ToNumber(&x1, &dcoeff);
    }

    /* Convert number to zoned decimal and set condition code */
    cc = dfp_number_to_zoned(&dwork, &dcoeff, zoned, l2, m3, &set);

    /* Store the zoned decimal result at the operand location */
    ARCH_DEP(vstorec) (zoned, l2, effective_addr2, b2, regs);

    /* Set the condition code in the PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(convert_dfp_ext_to_zoned) */


/*-------------------------------------------------------------------*/
/* EDA8 CZDT  - Convert to zoned from DFP Long               [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_zoned)
{
int             r1, m3;                 /* Values of R and M fields  */
int             l2;                     /* Operand length minus 1    */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal64       x1;                     /* Long DFP value            */
decNumber       dwork, dcoeff;          /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int             cc;                     /* Condition code            */
char            zoned[CZDT_MAXLEN];     /* Zoned decimal result      */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Program check if operand length exceeds 16 */
    if (l2 > CZDT_MAXLEN-1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r1 */
    ARCH_DEP(dfp_reg_to_decimal64)(r1, &x1, regs);
    decimal64ToNumber(&x1, &dwork);

    /* Extract coefficient only for Inf and NaN */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp64_clear_cf_and_bxcf(&x1);
        decimal64ToNumber(&x1, &dcoeff);
    }

    /* Convert number to zoned decimal and set condition code */
    cc = dfp_number_to_zoned(&dwork, &dcoeff, zoned, l2, m3, &set);

    /* Store the zoned decimal result at the operand location */
    ARCH_DEP(vstorec) (zoned, l2, effective_addr2, b2, regs);

    /* Set the condition code in the PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(convert_dfp_long_to_zoned) */
#endif /*defined(FEATURE_048_DFP_ZONE_CONV_FACILITY)*/


#if defined(FEATURE_080_DFP_PACK_CONV_FACILITY)
/*-------------------------------------------------------------------*/
/* EDAD CPXT  - Convert to packed from DFP Extended          [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_ext_to_packed)
{
    int             r1, m3;                 /* Values of R and M fields  */
    int             l2;                     /* Operand length minus 1    */
    int             b2;                     /* Base of effective addr    */
    VADR            effective_addr2;        /* Effective address         */
    decimal128      x1;                     /* Extended DFP value        */
    decNumber       dwork, dcoeff;          /* Working decimal numbers   */
    decContext      set;                    /* Working context           */
    int             cc;                     /* Condition code            */
    char            packed[CPXT_MAXLEN];    /* Packed decimal result     */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK(regs, b2);

    TXF_FLOAT_INSTR_CHECK(regs);
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Program check if operand length exceeds 18 */
    if (l2 > CPXT_MAXLEN - 1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r1 */
    ARCH_DEP(dfp_reg_to_decimal128)(r1, &x1, regs);
    decimal128ToNumber(&x1, &dwork);

    /* Extract coefficient only for Inf and NaN */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp128_clear_cf_and_bxcf(&x1);
        decimal128ToNumber(&x1, &dcoeff);
    }

    /* Convert number to packed decimal and set condition code */
    cc = dfp_number_to_packed(&dwork, &dcoeff, packed, l2, m3, &set);

    /* Store the zoned decimal result at the operand location */
    ARCH_DEP(vstorec) (packed, l2, effective_addr2, b2, regs);

    /* Set the condition code in the PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(convert_dfp_ext_to_zoned) */


/*-------------------------------------------------------------------*/
/* EDAC CPDT  - Convert to packed from DFP Long               [RSL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_dfp_long_to_packed)
{
    int             r1, m3;                 /* Values of R and M fields  */
    int             l2;                     /* Operand length minus 1    */
    int             b2;                     /* Base of effective addr    */
    VADR            effective_addr2;        /* Effective address         */
    decimal64       x1;                     /* Long DFP value            */
    decNumber       dwork, dcoeff;          /* Working decimal numbers   */
    decContext      set;                    /* Working context           */
    int             cc;                     /* Condition code            */
    char            packed[CPDT_MAXLEN];    /* Packed decimal result     */

    RSL_RM(inst, regs, r1, l2, b2, effective_addr2, m3);
    PER_ZEROADDR_XCHECK(regs, b2);

    TXF_FLOAT_INSTR_CHECK(regs);
    DFPINST_CHECK(regs);

    /* Program check if operand length exceeds 8 */
    if (l2 > CPDT_MAXLEN - 1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r1 */
    ARCH_DEP(dfp_reg_to_decimal64)(r1, &x1, regs);
    decimal64ToNumber(&x1, &dwork);

    /* Extract coefficient only for Inf and NaN */
    if (decNumberIsNaN(&dwork) || (decNumberIsInfinite(&dwork)))
    {
        dfp64_clear_cf_and_bxcf(&x1);
        decimal64ToNumber(&x1, &dcoeff);
    }

    /* Convert number to zoned decimal and set condition code */
    cc = dfp_number_to_packed(&dwork, &dcoeff, packed, l2, m3, &set);

    /* Store the zoned decimal result at the operand location */
    ARCH_DEP(vstorec) (packed, l2, effective_addr2, b2, regs);

    /* Set the condition code in the PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(convert_dfp_long_to_zoned) */
#endif /*defined(FEATURE_080_DFP_PACK_CONV_FACILITY)*/


/*-------------------------------------------------------------------*/
/* B3D9 DXTR  - Divide DFP Extended Register                 [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_dfp_ext_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal128      x1, x2, x3;             /* Extended DFP values       */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR3_CHECK(r1, r2, r3, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Divide FP register r2 by FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);
    decimal128ToNumber(&x2, &d2);
    decimal128ToNumber(&x3, &d3);
    decNumberDivide(&d1, &d2, &d3, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(divide_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3D1 DDTR  - Divide DFP Long Register                     [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_dfp_long_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal64       x1, x2, x3;             /* Long DFP values           */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Divide FP register r2 by FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);
    decimal64ToNumber(&x2, &d2);
    decimal64ToNumber(&x3, &d3);
    decNumberDivide(&d1, &d2, &d3, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(divide_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3ED EEXTR - Extract Biased Exponent DFP Extended Register  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_biased_exponent_dfp_ext_to_fix64_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal128      x2;                     /* Extended DFP value        */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
S64             exponent;               /* Biased exponent           */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);

    /* Convert to internal decimal number format */
    decimal128ToNumber(&x2, &d2);

    /* Calculate the biased exponent */
    if (decNumberIsInfinite(&d2))
        exponent = -1;
    else if (decNumberIsQNaN(&d2))
        exponent = -2;
    else if (decNumberIsSNaN(&d2))
        exponent = -3;
    else
        exponent = d2.exponent + DECIMAL128_Bias;

    /* Load result into general register r1 */
    regs->GR(r1) = exponent;

} /* end DEF_INST(extract_biased_exponent_dfp_ext_to_fix64_reg) */


/*-------------------------------------------------------------------*/
/* B3E5 EEDTR - Extract Biased Exponent DFP Long Register      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_biased_exponent_dfp_long_to_fix64_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal64       x2;                     /* Long DFP value            */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
S64             exponent;               /* Biased exponent           */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);

    /* Convert to internal decimal number format */
    decimal64ToNumber(&x2, &d2);

    /* Calculate the biased exponent */
    if (decNumberIsInfinite(&d2))
        exponent = -1;
    else if (decNumberIsQNaN(&d2))
        exponent = -2;
    else if (decNumberIsSNaN(&d2))
        exponent = -3;
    else
        exponent = d2.exponent + DECIMAL64_Bias;

    /* Load result into general register r1 */
    regs->GR(r1) = exponent;

} /* end DEF_INST(extract_biased_exponent_dfp_long_to_fix64_reg) */


/*-------------------------------------------------------------------*/
/* B3EF ESXTR - Extract Significance DFP Extended Register     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_significance_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal128      x2;                     /* Extended DFP value        */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
S64             digits;                 /* Number of decimal digits  */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);

    /* Convert to internal decimal number format */
    decimal128ToNumber(&x2, &d2);

    /* Calculate number of significant digits */
    if (decNumberIsZero(&d2))
        digits = 0;
    else if (decNumberIsInfinite(&d2))
        digits = -1;
    else if (decNumberIsQNaN(&d2))
        digits = -2;
    else if (decNumberIsSNaN(&d2))
        digits = -3;
    else
        digits = d2.digits;

    /* Load result into general register r1 */
    regs->GR(r1) = digits;

} /* end DEF_INST(extract_significance_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3E7 ESDTR - Extract Significance DFP Long Register         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_significance_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal64       x2;                     /* Long DFP value            */
decNumber       d2;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
S64             digits;                 /* Number of decimal digits  */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);

    /* Convert to internal decimal number format */
    decimal64ToNumber(&x2, &d2);

    /* Calculate number of significant digits */
    if (decNumberIsZero(&d2))
        digits = 0;
    else if (decNumberIsInfinite(&d2))
        digits = -1;
    else if (decNumberIsQNaN(&d2))
        digits = -2;
    else if (decNumberIsSNaN(&d2))
        digits = -3;
    else
        digits = d2.digits;

    /* Load result into general register r1 */
    regs->GR(r1) = digits;

} /* end DEF_INST(extract_significance_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3FE IEXTR - Insert Biased Exponent DFP Extended Register [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_biased_exponent_fix64_to_dfp_ext_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal128      x1, x3;                 /* Extended DFP values       */
decNumber       d;                      /* Working decimal number    */
decContext      set;                    /* Working context           */
S64             bexp;                   /* Biased exponent           */

    RRF_M(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r3, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load biased exponent from general register r2 */
    bexp = (S64)(regs->GR(r2));

    /* Load DFP extended number from FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);

    /* Insert biased exponent into number */
    if (bexp > DECIMAL128_Ehigh || bexp == -2 || bexp <= -4)
    {
        /* Result is a QNaN with re-encoded coefficient-continuation */
        dfp128_clear_cf_and_bxcf(&x3);
        decimal128ToNumber(&x3, &d);
        decimal128FromNumber(&x1, &d, &set);
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
    }
    else if (bexp == -3)
    {
        /* Result is a SNaN with re-encoded coefficient-continuation */
        dfp128_clear_cf_and_bxcf(&x3);
        decimal128ToNumber(&x3, &d);
        decimal128FromNumber(&x1, &d, &set);
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
    }
    else if (bexp == -1) /* Infinity */
    {
        /* Result is Infinity with re-encoded coefficient-continuation */
        dfp128_clear_cf_and_bxcf(&x3);
        decimal128ToNumber(&x3, &d);
        decimal128FromNumber(&x1, &d, &set);
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_INF);
    }
    else
    {
        decimal128ToNumber(&x3, &d);
        /* Clear CF and BXCF if source is Infinity or NaN */
        if (decNumberIsInfinite(&d) || decNumberIsNaN(&d))
        {
            dfp128_clear_cf_and_bxcf(&x3);
            decimal128ToNumber(&x3, &d);
        }
        /* Update exponent and re-encode coefficient-continuation */
        d.exponent = bexp - DECIMAL128_Bias;
        decimal128FromNumber(&x1, &d, &set);
    }

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(insert_biased_exponent_fix64_to_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3F6 IEDTR - Insert Biased Exponent DFP Long Register     [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_biased_exponent_fix64_to_dfp_long_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal64       x1, x3;                 /* Long DFP values           */
decNumber       d;                      /* Working decimal number    */
decContext      set;                    /* Working context           */
S64             bexp;                   /* Biased exponent           */

    RRF_M(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load biased exponent from general register r2 */
    bexp = (S64)(regs->GR(r2));

    /* Load DFP long number from FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);

    /* Insert biased exponent into number */
    if (bexp > DECIMAL64_Ehigh || bexp == -2 || bexp <= -4)
    {
        /* Result is a QNaN with re-encoded coefficient-continuation */
        dfp64_clear_cf_and_bxcf(&x3);
        decimal64ToNumber(&x3, &d);
        decimal64FromNumber(&x1, &d, &set);
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
    }
    else if (bexp == -3)
    {
        /* Result is a SNaN with re-encoded coefficient-continuation */
        dfp64_clear_cf_and_bxcf(&x3);
        decimal64ToNumber(&x3, &d);
        decimal64FromNumber(&x1, &d, &set);
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
    }
    else if (bexp == -1) /* Infinity */
    {
        /* Result is Infinity with re-encoded coefficient-continuation */
        dfp64_clear_cf_and_bxcf(&x3);
        decimal64ToNumber(&x3, &d);
        decimal64FromNumber(&x1, &d, &set);
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_INF);
    }
    else
    {
        decimal64ToNumber(&x3, &d);
        /* Clear CF and BXCF if source is Infinity or NaN */
        if (decNumberIsInfinite(&d) || decNumberIsNaN(&d))
        {
            dfp64_clear_cf_and_bxcf(&x3);
            decimal64ToNumber(&x3, &d);
        }
        /* Update exponent and re-encode coefficient-continuation */
        d.exponent = bexp - DECIMAL64_Bias;
        decimal64FromNumber(&x1, &d, &set);
    }

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(insert_biased_exponent_fix64_to_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3DE LTXTR - Load and Test DFP Extended Register            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_dfp_ext_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal128      x1, x2;                 /* Extended DFP values       */
decNumber       d;                      /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &d);

    /* For SNaN, force signaling condition and convert to QNaN */
    if (decNumberIsSNaN(&d))
    {
        set.status |= DEC_IEEE_854_Invalid_operation;
        d.bits &= ~DECSNAN;
        d.bits |= DECNAN;
    }

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Reencode value and load into FP register r1 */
    decimal128FromNumber(&x1, &d, &set);
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&d) ? 3 :
                   decNumberIsZero(&d) ? 0 :
                   decNumberIsNegative(&d) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_and_test_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3D6 LTDTR - Load and Test DFP Long Register                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_dfp_long_reg)
{
int             r1, r2;                 /* Values of R fields        */
decimal64       x1, x2;                 /* Long DFP values           */
decNumber       d;                      /* Working decimal number    */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load value from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d);

    /* For SNaN, force signaling condition and convert to QNaN */
    if (decNumberIsSNaN(&d))
    {
        set.status |= DEC_IEEE_854_Invalid_operation;
        d.bits &= ~DECSNAN;
        d.bits |= DECNAN;
    }

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Reencode value and load into FP register r1 */
    decimal64FromNumber(&x1, &d, &set);
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&d) ? 3 :
                   decNumberIsZero(&d) ? 0 :
                   decNumberIsNegative(&d) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_and_test_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3DF FIXTR - Load FP Integer DFP Extended Register        [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fp_int_dfp_ext_reg)
{
int             r1, r2, m3, m4;         /* Values of R and M fields  */
decimal128      x1, x2;                 /* Extended DFP values       */
decNumber       d1, d2, dc;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load decimal number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &d2);

    if (decNumberIsInfinite(&d2) == 0 && decNumberIsNaN(&d2) == 0)
    {
        /* Remove fractional part of decimal number */
        decNumberToIntegralValue(&d1, &d2, &set);

        /* Raise inexact condition if M4 bit 1 is zero and
           result differs in value from original value */
        if ((m4 & 0x04) == 0)
        {
            decNumberCompare(&dc, &d1, &d2, &set);
            if (decNumberIsZero(&dc) == 0)
            {
                set.status |= DEC_IEEE_854_Inexact;
                if (decNumberIsNegative(&dc) == decNumberIsNegative(&d2))
                    set.status |= DEC_Rounded;
            }
        }
    }
    else
    {
        /* Propagate NaN or default infinity */
        decNumberCopy(&d1, &d2);

        /* For SNaN, force signaling condition and convert to QNaN */
        if (decNumberIsSNaN(&d2))
        {
            set.status |= DEC_IEEE_854_Invalid_operation;
            d1.bits &= ~DECSNAN;
            d1.bits |= DECNAN;
        }
    }

    /* Convert result to extended DFP format */
    decimal128FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_fp_int_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3D7 FIDTR - Load FP Integer DFP Long Register            [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fp_int_dfp_long_reg)
{
int             r1, r2, m3, m4;         /* Values of R and M fields  */
decimal64       x1, x2;                 /* Long DFP values           */
decNumber       d1, d2, dc;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load decimal number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d2);

    if (decNumberIsInfinite(&d2) == 0 && decNumberIsNaN(&d2) == 0)
    {
        /* Remove fractional part of decimal number */
        decNumberToIntegralValue(&d1, &d2, &set);

        /* Raise inexact condition if M4 bit 1 is zero and
           result differs in value from original value */
        if ((m4 & 0x04) == 0)
        {
            decNumberCompare(&dc, &d1, &d2, &set);
            if (decNumberIsZero(&dc) == 0)
            {
                set.status |= DEC_IEEE_854_Inexact;
                if (decNumberIsNegative(&dc) == decNumberIsNegative(&d2))
                    set.status |= DEC_Rounded;
            }
        }
    }
    else
    {
        /* Propagate NaN or default infinity */
        decNumberCopy(&d1, &d2);

        /* For SNaN, force signaling condition and convert to QNaN */
        if (decNumberIsSNaN(&d2))
        {
            set.status |= DEC_IEEE_854_Invalid_operation;
            d1.bits &= ~DECSNAN;
            d1.bits |= DECNAN;
        }
    }

    /* Convert result to long DFP format */
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_fp_int_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3DC LXDTR - Load Lengthened DFP Long to Extended Reg.    [RRF-d] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_dfp_long_to_ext_reg)
{
int             r1, r2, m4;             /* Values of R and M fields  */
decimal128      x1;                     /* Extended DFP value        */
decimal64       x2;                     /* Long DFP value            */
decNumber       d1, d2;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_M4(inst, regs, r1, r2, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP long number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d2);

    /* Convert number to DFP extended format */
    if (decNumberIsInfinite(&d2) && (m4 & 0x08))
    {
        /* For Inf with mask bit 0 set, propagate the digits */
        dfp64_clear_cf_and_bxcf(&x2);
        decimal64ToNumber(&x2, &d1);
        decimal128FromNumber(&x1, &d1, &set);
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_INF);
    }
    else if (decNumberIsNaN(&d2))
    {
        decimal64ToNumber(&x2, &d1);
        /* For SNaN with mask bit 0 off, convert to a QNaN
           and raise signaling condition */
        if (decNumberIsSNaN(&d2) && (m4 & 0x08) == 0)
        {
            set.status |= DEC_IEEE_854_Invalid_operation;
            d1.bits &= ~DECSNAN;
            d1.bits |= DECNAN;
        }
        decimal128FromNumber(&x1, &d1, &set);
    }
    else
    {
        decNumberCopy(&d1, &d2);
        decimal128FromNumber(&x1, &d1, &set);
    }

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_lengthened_dfp_long_to_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3D4 LDETR - Load Lengthened DFP Short to Long Register   [RRF-d] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_dfp_short_to_long_reg)
{
int             r1, r2, m4;             /* Values of R and M fields  */
decimal64       x1;                     /* Long DFP value            */
decimal32       x2;                     /* Short DFP value           */
decNumber       d1, d2;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_M4(inst, regs, r1, r2, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP short number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal32)(r2, &x2, regs);
    decimal32ToNumber(&x2, &d2);

    /* Convert number to DFP long format */
    if (decNumberIsInfinite(&d2) && (m4 & 0x08))
    {
        /* For Inf with mask bit 0 set, propagate the digits */
        dfp32_clear_cf_and_bxcf(&x2);
        decimal32ToNumber(&x2, &d1);
        decimal64FromNumber(&x1, &d1, &set);
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_INF);
    }
    else if (decNumberIsNaN(&d2))
    {
        decimal32ToNumber(&x2, &d1);
        /* For SNaN with mask bit 0 off, convert to a QNaN
           and raise signaling condition */
        if (decNumberIsSNaN(&d2) && (m4 & 0x08) == 0)
        {
            set.status |= DEC_IEEE_854_Invalid_operation;
            d1.bits &= ~DECSNAN;
            d1.bits |= DECNAN;
        }
        decimal64FromNumber(&x1, &d1, &set);
    }
    else
    {
        decNumberCopy(&d1, &d2);
        decimal64FromNumber(&x1, &d1, &set);
    }

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_lengthened_dfp_short_to_long_reg) */


/*-------------------------------------------------------------------*/
/* B3DD LDXTR - Load Rounded DFP Extended to Long Register   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(load_rounded_dfp_ext_to_long_reg)
{
int             r1, r2, m3, m4;         /* Values of R and M fields  */
decimal64       x1;                     /* Long DFP value            */
decimal128      x2;                     /* Extended DFP value        */
decNumber       d1, d2;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int32_t         scale;                  /* Scaling factor            */
BYTE            pwork[17];              /* 33-digit packed work area */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r2, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load DFP extended number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    decimal128ToNumber(&x2, &d2);

    /* Convert number to DFP long format */
    if ((decNumberIsInfinite(&d2) && (m4 & 0x08))
         || decNumberIsNaN(&d2))
    {
        /* For Inf with mask bit 0 set, or for QNan or SNan,
           propagate the low 15 digits */
        dfp128_clear_cf_and_bxcf(&x2);
        decimal128ToNumber(&x2, &d1);
        decPackedFromNumber(pwork, sizeof(pwork), &scale, &d1);
        scale = 0;
        decPackedToNumber(pwork+sizeof(pwork)-8, 8, &scale, &d1);
        decimal64FromNumber(&x1, &d1, &set);
        if (decNumberIsInfinite(&d2))
        {
            dfp64_set_cf_and_bxcf(&x1, DFP_CFS_INF);
        }
        else if (decNumberIsQNaN(&d2))
        {
            dfp64_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
        }
        else /* it is an SNaN */
        {
            /* For SNaN with mask bit 0 off, convert to a QNaN
               and raise signaling condition */
            if (decNumberIsSNaN(&d2) && (m4 & 0x08) == 0)
            {
                dfp64_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
                set.status |= DEC_IEEE_854_Invalid_operation;
            }
            else
            {
                dfp64_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
            }
        }
    }
    else
    {
        /* For finite number, load value rounded to long DFP format,
           or for Inf with mask bit 0 not set, load default infinity */
        decNumberCopy(&d1, &d2);
        decimal64FromNumber(&x1, &d1, &set);
    }

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_rounded_dfp_ext_to_long_reg) */


/*-------------------------------------------------------------------*/
/* B3D5 LEDTR - Load Rounded DFP Long to Short Register      [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(load_rounded_dfp_long_to_short_reg)
{
int             r1, r2, m3, m4;         /* Values of R and M fields  */
decimal32       x1;                     /* Short DFP value           */
decimal64       x2;                     /* Long DFP value            */
decNumber       d1, d2;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int32_t         scale;                  /* Scaling factor            */
BYTE            pwork[9];               /* 17-digit packed work area */
BYTE            dxc;                    /* Data exception code       */

    RRF_MM(inst, regs, r1, r2, m3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m3, regs);

    /* Load DFP long number from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    decimal64ToNumber(&x2, &d2);

    /* Convert number to DFP short format */
    if ((decNumberIsInfinite(&d2) && (m4 & 0x08))
         || decNumberIsNaN(&d2))
    {
        /* For Inf with mask bit 0 set, or for QNan or SNan,
           propagate the low 6 digits */
        dfp64_clear_cf_and_bxcf(&x2);
        decimal64ToNumber(&x2, &d1);
        decPackedFromNumber(pwork, sizeof(pwork), &scale, &d1);
        scale = 0;
        decPackedToNumber(pwork+sizeof(pwork)-4, 4, &scale, &d1);
        decimal32FromNumber(&x1, &d1, &set);
        if (decNumberIsInfinite(&d2))
        {
            dfp32_set_cf_and_bxcf(&x1, DFP_CFS_INF);
        }
        else if (decNumberIsQNaN(&d2))
        {
            dfp32_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
        }
        else /* it is an SNaN */
        {
            /* For SNaN with mask bit 0 off, convert to a QNaN
               and raise signaling condition */
            if (decNumberIsSNaN(&d2) && (m4 & 0x08) == 0)
            {
                dfp32_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
                set.status |= DEC_IEEE_854_Invalid_operation;
            }
            else
            {
                dfp32_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
            }
        }
    }
    else
    {
        /* For finite number, load value rounded to short DFP format,
           or for Inf with mask bit 0 not set, load default infinity */
        decNumberCopy(&d1, &d2);
        decimal32FromNumber(&x1, &d1, &set);
    }

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal32)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_rounded_dfp_long_to_short_reg) */


/*-------------------------------------------------------------------*/
/* B3D8 MXTR  - Multiply DFP Extended Register               [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_dfp_ext_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal128      x1, x2, x3;             /* Extended DFP values       */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR3_CHECK(r1, r2, r3, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Multiply FP register r2 by FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);
    decimal128ToNumber(&x2, &d2);
    decimal128ToNumber(&x3, &d3);
    decNumberMultiply(&d1, &d2, &d3, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(multiply_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3D0 MDTR  - Multiply DFP Long Register                   [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_dfp_long_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal64       x1, x2, x3;             /* Long DFP values           */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Multiply FP register r2 by FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);
    decimal64ToNumber(&x2, &d2);
    decimal64ToNumber(&x3, &d3);
    decNumberMultiply(&d1, &d2, &d3, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(multiply_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3FD QAXTR - Quantize DFP Extended Register               [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST(quantize_dfp_ext_reg)
{
int             r1, r2, r3, m4;         /* Values of R and M fields  */
decimal128      x1, x2, x3;             /* Extended DFP values       */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_RM(inst, regs, r1, r2, r3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR3_CHECK(r1, r2, r3, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m4, regs);

    /* Quantize FP register r3 using FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);
    decimal128ToNumber(&x2, &d2);
    decimal128ToNumber(&x3, &d3);
    decNumberQuantize(&d1, &d2, &d3, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(quantize_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3F5 QADTR - Quantize DFP Long Register                   [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST(quantize_dfp_long_reg)
{
int             r1, r2, r3, m4;         /* Values of R and M fields  */
decimal64       x1, x2, x3;             /* Long DFP values           */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_RM(inst, regs, r1, r2, r3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m4, regs);

    /* Quantize FP register r3 using FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);
    decimal64ToNumber(&x2, &d2);
    decimal64ToNumber(&x3, &d3);
    decNumberQuantize(&d1, &d2, &d3, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(quantize_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* B3FF RRXTR - Reround DFP Extended Register                [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST(reround_dfp_ext_reg)
{
int             signif;                 /* Requested significance    */
int             r1, r2, r3, m4;         /* Values of R and M fields  */
decimal128      x1, x3;                 /* Extended DFP values       */
decNumber       d1, d3;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_RM(inst, regs, r1, r2, r3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r3, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, m4, regs);

    /* Load significance from bits 58-63 of general register r2 */
    signif = regs->GR_L(r2) & 0x3F;

    /* Reround FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);
    decimal128ToNumber(&x3, &d3);
    if (decNumberIsInfinite(&d3) || decNumberIsNaN(&d3)
        || decNumberIsZero(&d3)
        || signif == 0 || d3.digits <= signif)
    {
        decNumberCopy(&d1, &d3);
    }
    else
    {
        set.digits = signif;
        decNumberPlus(&d1, &d3, &set);
    }
    decimal128FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(reround_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3F7 RRDTR - Reround DFP Long Register                    [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST(reround_dfp_long_reg)
{
int             signif;                 /* Requested significance    */
int             r1, r2, r3, m4;         /* Values of R and M fields  */
decimal64       x1, x3;                 /* Long DFP values           */
decNumber       d1, d3;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRF_RM(inst, regs, r1, r2, r3, m4);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, m4, regs);

    /* Load significance from bits 58-63 of general register r2 */
    signif = regs->GR_L(r2) & 0x3F;

    /* Reround FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);
    decimal64ToNumber(&x3, &d3);
    if (decNumberIsInfinite(&d3) || decNumberIsNaN(&d3)
        || decNumberIsZero(&d3)
        || signif == 0 || d3.digits <= signif)
    {
        decNumberCopy(&d1, &d3);
    }
    else
    {
        set.digits = signif;
        decNumberPlus(&d1, &d3, &set);
    }
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(reround_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* ED48 SLXT  - Shift Coefficient Left DFP Extended            [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_coefficient_left_dfp_ext)
{
int             r1, r3;                 /* Values of R fields        */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal128      x1, x3;                 /* Extended DFP values       */
decNumber       d1, d3;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int             n;                      /* Number of bits to shift   */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r3, regs);

    /* Isolate rightmost 6 bits of second operand address */
    n = effective_addr2 & 0x3F;

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);

    /* Convert to internal decimal number format */
    decimal128ToNumber(&x3, &d3);

    /* For NaN and Inf use coefficient continuation digits only */
    if (decNumberIsNaN(&d3) || decNumberIsInfinite(&d3))
    {
        dfp128_clear_cf_and_bxcf(&x3);
        decimal128ToNumber(&x3, &d1);
    }
    else
    {
        decNumberCopy(&d1, &d3);
    }

    /* Shift coefficient left n digit positions */
    dfp_shift_coeff(&set, &d1, n);

    /* Convert result to DFP extended format */
    decimal128FromNumber(&x1, &d1, &set);

    /* Restore Nan or Inf indicators in the result */
    if (decNumberIsQNaN(&d3))
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
    else if (decNumberIsSNaN(&d3))
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
    else if (decNumberIsInfinite(&d3))
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_INF);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(shift_coefficient_left_dfp_ext) */


/*-------------------------------------------------------------------*/
/* ED40 SLDT  - Shift Coefficient Left DFP Long                [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_coefficient_left_dfp_long)
{
int             r1, r3;                 /* Values of R fields        */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal64       x1, x3;                 /* Long DFP values           */
decNumber       d1, d3;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int             n;                      /* Number of bits to shift   */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Isolate rightmost 6 bits of second operand address */
    n = effective_addr2 & 0x3F;

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);

    /* Convert to internal decimal number format */
    decimal64ToNumber(&x3, &d3);

    /* For NaN and Inf use coefficient continuation digits only */
    if (decNumberIsNaN(&d3) || decNumberIsInfinite(&d3))
    {
        dfp64_clear_cf_and_bxcf(&x3);
        decimal64ToNumber(&x3, &d1);
    }
    else
    {
        decNumberCopy(&d1, &d3);
    }

    /* Shift coefficient left n digit positions */
    dfp_shift_coeff(&set, &d1, n);

    /* Convert result to DFP long format */
    decimal64FromNumber(&x1, &d1, &set);

    /* Restore Nan or Inf indicators in the result */
    if (decNumberIsQNaN(&d3))
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
    else if (decNumberIsSNaN(&d3))
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
    else if (decNumberIsInfinite(&d3))
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_INF);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(shift_coefficient_left_dfp_long) */


/*-------------------------------------------------------------------*/
/* ED49 SRXT  - Shift Coefficient Right DFP Extended           [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_coefficient_right_dfp_ext)
{
int             r1, r3;                 /* Values of R fields        */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal128      x1, x3;                 /* Extended DFP values       */
decNumber       d1, d3;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int             n;                      /* Number of bits to shift   */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR2_CHECK(r1, r3, regs);

    /* Isolate rightmost 6 bits of second operand address */
    n = effective_addr2 & 0x3F;

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);

    /* Convert to internal decimal number format */
    decimal128ToNumber(&x3, &d3);

    /* For NaN and Inf use coefficient continuation digits only */
    if (decNumberIsNaN(&d3) || decNumberIsInfinite(&d3))
    {
        dfp128_clear_cf_and_bxcf(&x3);
        decimal128ToNumber(&x3, &d1);
    }
    else
    {
        decNumberCopy(&d1, &d3);
    }

    /* Shift coefficient right n digit positions */
    dfp_shift_coeff(&set, &d1, -n);

    /* Convert result to DFP extended format */
    decimal128FromNumber(&x1, &d1, &set);

    /* Restore Nan or Inf indicators in the result */
    if (decNumberIsQNaN(&d3))
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
    else if (decNumberIsSNaN(&d3))
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
    else if (decNumberIsInfinite(&d3))
        dfp128_set_cf_and_bxcf(&x1, DFP_CFS_INF);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

} /* end DEF_INST(shift_coefficient_right_dfp_ext) */


/*-------------------------------------------------------------------*/
/* ED41 SRDT  - Shift Coefficient Right DFP Long               [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_coefficient_right_dfp_long)
{
int             r1, r3;                 /* Values of R fields        */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal64       x1, x3;                 /* Long DFP values           */
decNumber       d1, d3;                 /* Working decimal numbers   */
decContext      set;                    /* Working context           */
int             n;                      /* Number of bits to shift   */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Isolate rightmost 6 bits of second operand address */
    n = effective_addr2 & 0x3F;

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r3 */
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);

    /* Convert to internal decimal number format */
    decimal64ToNumber(&x3, &d3);

    /* For NaN and Inf use coefficient continuation digits only */
    if (decNumberIsNaN(&d3) || decNumberIsInfinite(&d3))
    {
        dfp64_clear_cf_and_bxcf(&x3);
        decimal64ToNumber(&x3, &d1);
    }
    else
    {
        decNumberCopy(&d1, &d3);
    }

    /* Shift coefficient right n digit positions */
    dfp_shift_coeff(&set, &d1, -n);

    /* Convert result to DFP long format */
    decimal64FromNumber(&x1, &d1, &set);

    /* Restore Nan or Inf indicators in the result */
    if (decNumberIsQNaN(&d3))
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_QNAN);
    else if (decNumberIsSNaN(&d3))
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_SNAN);
    else if (decNumberIsInfinite(&d3))
        dfp64_set_cf_and_bxcf(&x1, DFP_CFS_INF);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

} /* end DEF_INST(shift_coefficient_right_dfp_long) */


/*-------------------------------------------------------------------*/
/* B3DB SXTR  - Subtract DFP Extended Register               [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_dfp_ext_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal128      x1, x2, x3;             /* Extended DFP values       */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR3_CHECK(r1, r2, r3, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Subtract FP register r3 from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal128)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal128)(r3, &x3, regs);
    decimal128ToNumber(&x2, &d2);
    decimal128ToNumber(&x3, &d3);
    decNumberSubtract(&d1, &d2, &d3, &set);
    decimal128FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal128)(r1, &x1, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&d1) ? 3 :
                   decNumberIsZero(&d1) ? 0 :
                   decNumberIsNegative(&d1) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(subtract_dfp_ext_reg) */


/*-------------------------------------------------------------------*/
/* B3D3 SDTR  - Subtract DFP Long Register                   [RRF-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_dfp_long_reg)
{
int             r1, r2, r3;             /* Values of R fields        */
decimal64       x1, x2, x3;             /* Long DFP values           */
decNumber       d1, d2, d3;             /* Working decimal numbers   */
decContext      set;                    /* Working context           */
BYTE            dxc;                    /* Data exception code       */

    RRR(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);
    ARCH_DEP(dfp_rounding_mode)(&set, 0, regs);

    /* Subtract FP register r3 from FP register r2 */
    ARCH_DEP(dfp_reg_to_decimal64)(r2, &x2, regs);
    ARCH_DEP(dfp_reg_to_decimal64)(r3, &x3, regs);
    decimal64ToNumber(&x2, &d2);
    decimal64ToNumber(&x3, &d3);
    decNumberSubtract(&d1, &d2, &d3, &set);
    decimal64FromNumber(&x1, &d1, &set);

    /* Check for exception condition */
    dxc = ARCH_DEP(dfp_status_check)(&set, regs);

    /* Load result into FP register r1 */
    ARCH_DEP(dfp_reg_from_decimal64)(r1, &x1, regs);

    /* Set condition code */
    regs->psw.cc = decNumberIsNaN(&d1) ? 3 :
                   decNumberIsZero(&d1) ? 0 :
                   decNumberIsNegative(&d1) ? 1 : 2;

    /* Raise data exception if error occurred */
    if (dxc != 0)
    {
        regs->dxc = dxc;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(subtract_dfp_long_reg) */


/*-------------------------------------------------------------------*/
/* ED58 TDCXT - Test Data Class DFP Extended                   [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(test_data_class_dfp_ext)
{
int             r1;                     /* Value of R field          */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal128      x1;                     /* Extended DFP value        */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
U32             bits;                   /* Low 12 bits of address    */

    RXE(inst, regs, r1, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Convert FP register r1 to decimal number format */
    ARCH_DEP(dfp_reg_to_decimal128)(r1, &x1, regs);
    decimal128ToNumber(&x1, &d1);

    /* Isolate rightmost 12 bits of second operand address */
    bits = effective_addr2 & 0xFFF;

    /* Test data class and set condition code */
    regs->psw.cc = dfp_test_data_class(&set, &d1, bits);

} /* end DEF_INST(test_data_class_dfp_ext) */


/*-------------------------------------------------------------------*/
/* ED54 TDCDT - Test Data Class DFP Long                       [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(test_data_class_dfp_long)
{
int             r1;                     /* Value of R field          */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal64       x1;                     /* Long DFP value            */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
U32             bits;                   /* Low 12 bits of address    */

    RXE(inst, regs, r1, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Convert FP register r1 to decimal number format */
    ARCH_DEP(dfp_reg_to_decimal64)(r1, &x1, regs);
    decimal64ToNumber(&x1, &d1);

    /* Isolate rightmost 12 bits of second operand address */
    bits = effective_addr2 & 0xFFF;

    /* Test data class and set condition code */
    regs->psw.cc = dfp_test_data_class(&set, &d1, bits);

} /* end DEF_INST(test_data_class_dfp_long) */


/*-------------------------------------------------------------------*/
/* ED50 TDCET - Test Data Class DFP Short                      [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(test_data_class_dfp_short)
{
int             r1;                     /* Value of R field          */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal32       x1;                     /* Short DFP value           */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
U32             bits;                   /* Low 12 bits of address    */

    RXE(inst, regs, r1, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for short DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL32);

    /* Convert FP register r1 to decimal number format */
    ARCH_DEP(dfp_reg_to_decimal32)(r1, &x1, regs);
    decimal32ToNumber(&x1, &d1);

    /* Isolate rightmost 12 bits of second operand address */
    bits = effective_addr2 & 0xFFF;

    /* Test data class and set condition code */
    regs->psw.cc = dfp_test_data_class(&set, &d1, bits);

} /* end DEF_INST(test_data_class_dfp_short) */


/*-------------------------------------------------------------------*/
/* ED59 TDGXT - Test Data Group DFP Extended                   [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(test_data_group_dfp_ext)
{
int             r1;                     /* Value of R field          */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal128      x1;                     /* Extended DFP value        */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
U32             bits;                   /* Low 12 bits of address    */
int             lmd;                    /* Leftmost digit            */

    RXE(inst, regs, r1, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);
    DFPREGPAIR_CHECK(r1, regs);

    /* Initialise the context for extended DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL128);

    /* Load DFP extended number from FP register r1 */
    ARCH_DEP(dfp_reg_to_decimal128)(r1, &x1, regs);

    /* Extract the leftmost digit from FP register r1 */
    lmd = dfp128_extract_lmd(&x1);

    /* Convert to internal decimal number format */
    decimal128ToNumber(&x1, &d1);

    /* Isolate rightmost 12 bits of second operand address */
    bits = effective_addr2 & 0xFFF;

    /* Test data group and set condition code */
    regs->psw.cc = dfp_test_data_group(&set, &d1, lmd, bits);

} /* end DEF_INST(test_data_group_dfp_ext) */


/*-------------------------------------------------------------------*/
/* ED55 TDGDT - Test Data Group DFP Long                       [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(test_data_group_dfp_long)
{
int             r1;                     /* Value of R field          */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal64       x1;                     /* Long DFP value            */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
U32             bits;                   /* Low 12 bits of address    */
int             lmd;                    /* Leftmost digit            */

    RXE(inst, regs, r1, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for long DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL64);

    /* Load DFP long number from FP register r1 */
    ARCH_DEP(dfp_reg_to_decimal64)(r1, &x1, regs);

    /* Extract the leftmost digit from FP register r1 */
    lmd = dfp64_extract_lmd(&x1);

    /* Convert to internal decimal number format */
    decimal64ToNumber(&x1, &d1);

    /* Isolate rightmost 12 bits of second operand address */
    bits = effective_addr2 & 0xFFF;

    /* Test data group and set condition code */
    regs->psw.cc = dfp_test_data_group(&set, &d1, lmd, bits);

} /* end DEF_INST(test_data_group_dfp_long) */


/*-------------------------------------------------------------------*/
/* ED51 TDGET - Test Data Group DFP Short                      [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(test_data_group_dfp_short)
{
int             r1;                     /* Value of R field          */
int             x2;                     /* Index register            */
int             b2;                     /* Base of effective addr    */
VADR            effective_addr2;        /* Effective address         */
decimal32       x1;                     /* Short DFP value           */
decNumber       d1;                     /* Working decimal number    */
decContext      set;                    /* Working context           */
U32             bits;                   /* Low 12 bits of address    */
int             lmd;                    /* Leftmost digit            */

    RXE(inst, regs, r1, x2, b2, effective_addr2);

    TXF_FLOAT_INSTR_CHECK( regs );
    DFPINST_CHECK(regs);

    /* Initialise the context for short DFP */
    decContextDefault(&set, DEC_INIT_DECIMAL32);

    /* Load DFP short number from FP register r1 */
    ARCH_DEP(dfp_reg_to_decimal32)(r1, &x1, regs);

    /* Extract the leftmost digit from FP register r1 */
    lmd = dfp32_extract_lmd(&x1);

    /* Convert to internal decimal number format */
    decimal32ToNumber(&x1, &d1);

    /* Isolate rightmost 12 bits of second operand address */
    bits = effective_addr2 & 0xFFF;

    /* Test data group and set condition code */
    regs->psw.cc = dfp_test_data_group(&set, &d1, lmd, bits);

} /* end DEF_INST(test_data_group_dfp_short) */

#endif /*defined(FEATURE_042_DFP_FACILITY)*/

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "dfp.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "dfp.c"
#endif

#endif /*!defined(_GEN_ARCH)*/


/* end of dfp.c */
