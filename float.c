/* FLOAT.C      (C) Copyright Peter Kuschnerus, 2000-2012            */
/*              ESA/390 Hex Floating-point Instructions              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module implements the ESA/390 Hex Floatingpoint Instructions */
/* described in the manual ESA/390 Principles of Operation.          */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Incorporated all floating point instructions from cpu.c in order  */
/* to implement revised instruction decoding.                        */
/*                                               Jan Jaeger 01/07/00 */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Added the square-root-facility (instructions SQDR, SQER).         */
/*                                         Peter Kuschnerus 01/09/00 */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Added the HFP-extension-facility (26 additional instructions).    */
/* Added the AFP-Registers-facility (additional float registers).    */
/*                                         Peter Kuschnerus 13/12/00 */
/* Long Displacement Facility: LDY,LEY,STDY,STEY   R.Bowler 29/06/03 */
/* FPS Extensions Facility: LXR,LZER,LZDR,LZXR     R.Bowler 06juil03 */
/* HFP Multiply and Add/Subtract Facility          R.Bowler 10juil03 */
/* Convert 64fixed to float family CEGR,CDGR,CXGR  BvdHelm  28/01/06 */
/* Completed the family CGER, CGDR and CGXR        BvdHelm  04/11/06 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _FLOAT_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/* When an operation code has unused operand(s) (IPK, e.g.), it will */
/* attract  a diagnostic for a set, but unused variable.  Fixing the */
/* macros to support e.g., RS_NOOPS is not productive, so:           */
DISABLE_GCC_UNUSED_SET_WARNING

#if defined(FEATURE_HEXADECIMAL_FLOATING_POINT)

/* Rename all inline functions for multi architectural support *JJ   */
 #undef get_sf
 #undef store_sf
 #undef get_lf
 #undef store_lf
 #undef get_ef
 #undef store_ef
 #undef vfetch_sf
 #undef vfetch_lf
 #undef normal_sf
 #undef normal_lf
 #undef normal_ef
 #undef overflow_sf
 #undef overflow_lf
 #undef overflow_ef
 #undef underflow_sf
 #undef underflow_lf
 #undef underflow_ef
 #undef over_under_flow_sf
 #undef over_under_flow_lf
 #undef over_under_flow_ef
 #undef significance_sf
 #undef significance_lf
 #undef significance_ef
 #undef add_ef
 #undef add_lf
 #undef add_sf
 #undef cmp_lf
 #undef cmp_sf
 #undef div_U128
 #undef div_U256
 #undef div_ef
 #undef div_lf
 #undef div_sf
 #undef mul_ef
 #undef mul_sf
 #undef mul_lf
 #undef mul_lf_to_ef
 #undef mul_sf_to_lf
 #undef square_root_fraction
 #undef sq_sf
 #undef sq_lf
#if __GEN_ARCH == 370
 #define get_ef s370_get_ef
 #define get_lf s370_get_lf
 #define get_sf s370_get_sf
 #define normal_ef      s370_normal_ef
 #define normal_lf      s370_normal_lf
 #define normal_sf      s370_normal_sf
 #define over_under_flow_ef     s370_over_under_flow_ef
 #define over_under_flow_lf     s370_over_under_flow_lf
 #define over_under_flow_sf     s370_over_under_flow_sf
 #define overflow_ef    s370_overflow_ef
 #define overflow_lf    s370_overflow_lf
 #define overflow_sf    s370_overflow_sf
 #define significance_ef        s370_significance_ef
 #define significance_lf        s370_significance_lf
 #define significance_sf        s370_significance_sf
 #define store_ef       s370_store_ef
 #define store_lf       s370_store_lf
 #define store_sf       s370_store_sf
 #define underflow_ef   s370_underflow_ef
 #define underflow_lf   s370_underflow_lf
 #define underflow_sf   s370_underflow_sf
 #define vfetch_lf      s370_vfetch_lf
 #define vfetch_sf      s370_vfetch_sf
 #define add_ef s370_add_ef
 #define add_lf s370_add_lf
 #define add_sf s370_add_sf
 #define cmp_lf s370_cmp_lf
 #define cmp_sf s370_cmp_sf
 #define div_U128       s370_div_U128
 #define div_U256       s370_div_U256
 #define div_ef s370_div_ef
 #define div_lf s370_div_lf
 #define div_sf s370_div_sf
 #define mul_ef s370_mul_ef
 #define mul_sf s370_mul_sf
 #define mul_lf s370_mul_lf
 #define mul_lf_to_ef   s370_mul_lf_to_ef
 #define mul_sf_to_lf   s370_mul_sf_to_lf
 #define square_root_fraction   s370_square_root_fraction
 #define sq_sf  s370_sq_sf
 #define sq_lf  s370_sq_lf
#elif __GEN_ARCH == 390
 #define get_ef s390_get_ef
 #define get_lf s390_get_lf
 #define get_sf s390_get_sf
 #define normal_ef      s390_normal_ef
 #define normal_lf      s390_normal_lf
 #define normal_sf      s390_normal_sf
 #define over_under_flow_ef     s390_over_under_flow_ef
 #define over_under_flow_lf     s390_over_under_flow_lf
 #define over_under_flow_sf     s390_over_under_flow_sf
 #define overflow_ef    s390_overflow_ef
 #define overflow_lf    s390_overflow_lf
 #define overflow_sf    s390_overflow_sf
 #define significance_ef        s390_significance_ef
 #define significance_lf        s390_significance_lf
 #define significance_sf        s390_significance_sf
 #define store_ef       s390_store_ef
 #define store_lf       s390_store_lf
 #define store_sf       s390_store_sf
 #define underflow_ef   s390_underflow_ef
 #define underflow_lf   s390_underflow_lf
 #define underflow_sf   s390_underflow_sf
 #define vfetch_lf      s390_vfetch_lf
 #define vfetch_sf      s390_vfetch_sf
 #define add_ef s390_add_ef
 #define add_lf s390_add_lf
 #define add_sf s390_add_sf
 #define cmp_lf s390_cmp_lf
 #define cmp_sf s390_cmp_sf
 #define div_U128       s390_div_U128
 #define div_U256       s390_div_U256
 #define div_ef s390_div_ef
 #define div_lf s390_div_lf
 #define div_sf s390_div_sf
 #define mul_ef s390_mul_ef
 #define mul_sf s390_mul_sf
 #define mul_lf s390_mul_lf
 #define mul_lf_to_ef   s390_mul_lf_to_ef
 #define mul_sf_to_lf   s390_mul_sf_to_lf
 #define square_root_fraction   s390_square_root_fraction
 #define sq_sf  s390_sq_sf
 #define sq_lf  s390_sq_lf
#elif __GEN_ARCH == 900
 #define get_ef z900_get_ef
 #define get_lf z900_get_lf
 #define get_sf z900_get_sf
 #define normal_ef      z900_normal_ef
 #define normal_lf      z900_normal_lf
 #define normal_sf      z900_normal_sf
 #define over_under_flow_ef     z900_over_under_flow_ef
 #define over_under_flow_lf     z900_over_under_flow_lf
 #define over_under_flow_sf     z900_over_under_flow_sf
 #define overflow_ef    z900_overflow_ef
 #define overflow_lf    z900_overflow_lf
 #define overflow_sf    z900_overflow_sf
 #define significance_ef        z900_significance_ef
 #define significance_lf        z900_significance_lf
 #define significance_sf        z900_significance_sf
 #define store_ef       z900_store_ef
 #define store_lf       z900_store_lf
 #define store_sf       z900_store_sf
 #define underflow_ef   z900_underflow_ef
 #define underflow_lf   z900_underflow_lf
 #define underflow_sf   z900_underflow_sf
 #define vfetch_lf      z900_vfetch_lf
 #define vfetch_sf      z900_vfetch_sf
 #define add_ef z900_add_ef
 #define add_lf z900_add_lf
 #define add_sf z900_add_sf
 #define cmp_lf z900_cmp_lf
 #define cmp_sf z900_cmp_sf
 #define div_U128       z900_div_U128
 #define div_U256       z900_div_U256
 #define div_ef z900_div_ef
 #define div_lf z900_div_lf
 #define div_sf z900_div_sf
 #define mul_ef z900_mul_ef
 #define mul_sf z900_mul_sf
 #define mul_lf z900_mul_lf
 #define mul_lf_to_ef   z900_mul_lf_to_ef
 #define mul_sf_to_lf   z900_mul_sf_to_lf
 #define square_root_fraction   z900_square_root_fraction
 #define sq_sf  z900_sq_sf
 #define sq_lf  z900_sq_lf
#else
 #error Unable to determine GEN_ARCH
#endif

#if !defined(_FLOAT_C)

#define _FLOAT_C


/*-------------------------------------------------------------------*/
/* Definitions                                                       */
/*-------------------------------------------------------------------*/
#define POS     0                       /* Positive value of sign    */
#define NEG     1                       /* Negative value of sign    */
#define UNNORMAL 0                      /* Without normalisation     */
#define NORMAL  1                       /* With normalisation        */
#define OVUNF   1                       /* Check for over/underflow  */
#define NOOVUNF 0                       /* Leave exponent as is (for
                                           multiply-add/subtrace)    */
#define SIGEX   1                       /* Significance exception if
                                           result fraction is zero   */
#define NOSIGEX 0                       /* Do not raise significance
                                           exception, use true zero  */

#define FLOAT_DEBUG 0        /* Change to 1 to enable debug messages */

/*-------------------------------------------------------------------*/
/* Add 128 bit unsigned integer                                      */
/* The result is placed in operand a                                 */
/*                                                                   */
/* msa  most significant 64 bit of operand a                         */
/* lsa  least significant 64 bit of operand a                        */
/* msb  most significant 64 bit of operand b                         */
/* lsb  least significant 64 bit of operand b                        */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define add_U128(msa, lsa, msb, lsb) \
    (lsa) += (lsb); \
    (msa) += (msb); \
    if ((lsa) < (lsb)) \
        (msa)++


/*-------------------------------------------------------------------*/
/* Subtract 128 bit unsigned integer                                 */
/* The operand b is subtracted from operand a                        */
/* The result is placed in operand a                                 */
/*                                                                   */
/* msa  most significant 64 bit of operand a                         */
/* lsa  least significant 64 bit of operand a                        */
/* msb  most significant 64 bit of operand b                         */
/* lsb  least significant 64 bit of operand b                        */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define sub_U128(msa, lsa, msb, lsb) \
    (msa) -= (msb); \
    if ((lsa) < (lsb)) \
        (msa)--; \
    (lsa) -= (lsb)


/*-------------------------------------------------------------------*/
/* Subtract 128 bit unsigned integer reverse                         */
/* The operand a is subtracted from operand b                        */
/* The result is placed in operand a                                 */
/*                                                                   */
/* msa  most significant 64 bit of operand a                         */
/* lsa  least significant 64 bit of operand a                        */
/* msb  most significant 64 bit of operand b                         */
/* lsb  least significant 64 bit of operand b                        */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define sub_reverse_U128(msa, lsa, msb, lsb) \
    (msa) = (msb) - (msa); \
    if ((lsb) < (lsa)) \
        (msa)--; \
    (lsa) = (lsb) - (lsa)


/*-------------------------------------------------------------------*/
/* Shift 128 bit one bit right                                       */
/*                                                                   */
/* ms   most significant 64 bit of operand                           */
/* ls   least significant 64 bit of operand                          */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define shift_right_U128(ms, ls) \
    (ls) = ((ls) >> 1) | ((ms) << 63); \
    (ms) >>= 1


/*-------------------------------------------------------------------*/
/* Shift 128 bit one bit left                                        */
/*                                                                   */
/* ms   most significant 64 bit of operand                           */
/* ls   least significant 64 bit of operand                          */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define shift_left_U128(ms, ls) \
    (ms) = ((ms) << 1) | ((ls) >> 63); \
    (ls) <<= 1

/*-------------------------------------------------------------------*/
/* Shift 128 bit 4 bits right with shifted digit                     */
/*                                                                   */
/* ms   most significant 64 bit of operand                           */
/* ls   least significant 64 bit of operand                          */
/* dig  least significant 4 bits removed by right shift              */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define shift_right4_U128(ms, ls, dig) \
    (dig) = (ls) & 0xF; \
    (ls) = ((ls) >> 4) | ((ms) << 60); \
    (ms) >>= 4


/*-------------------------------------------------------------------*/
/* Shift 128 bit 4 bits left with shifted digit                      */
/*                                                                   */
/* ms   most significant 64 bit of operand                           */
/* ls   least significant 64 bit of operand                          */
/* dig  most significant 4 bits removed by left shift                */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define shift_left4_U128(ms, ls, dig) \
    (dig) = (ms) >> 60; \
    (ms) = ((ms) << 4) | ((ls) >> 60); \
    (ls) <<= 4


/*-------------------------------------------------------------------*/
/* Shift 256 bit one bit left                                        */
/*                                                                   */
/* mms  most significant 64 bit of operand                           */
/* ms   more significant 64 bit of operand                           */
/* ls   less significant 64 bit of operand                           */
/* lls  least significant 64 bit of operand                          */
/*                                                                   */
/* all operands are expected to be defined as U64                    */
/*-------------------------------------------------------------------*/
#define shift_left_U256(mms, ms, ls, lls) \
    (mms) = ((mms) << 1) | ((ms) >> 63); \
    (ms) = ((ms) << 1) | ((ls) >> 63); \
    (ls) = ((ls) << 1) | ((lls) >> 63); \
    (lls) <<= 1


/*-------------------------------------------------------------------*/
/* Structure definition for internal short floatingpoint format      */
/*-------------------------------------------------------------------*/
typedef struct _SHORT_FLOAT {
        U32     short_fract;            /* Fraction                  */
        short   expo;                   /* Exponent + 64             */
        BYTE    sign;                   /* Sign                      */
} SHORT_FLOAT;


/*-------------------------------------------------------------------*/
/* Structure definition for internal long floatingpoint format       */
/*-------------------------------------------------------------------*/
typedef struct _LONG_FLOAT {
        U64     long_fract;             /* Fraction                  */
        short   expo;                   /* Exponent + 64             */
        BYTE    sign;                   /* Sign                      */
} LONG_FLOAT;


/*-------------------------------------------------------------------*/
/* Structure definition for internal extended floatingpoint format   */
/*-------------------------------------------------------------------*/
typedef struct _EXTENDED_FLOAT {
        U64     ms_fract, ls_fract;     /* Fraction                  */
        short   expo;                   /* Exponent + 64             */
        BYTE    sign;                   /* Sign                      */
} EXTENDED_FLOAT;


#endif /*!defined(_FLOAT_C)*/


/*-------------------------------------------------------------------*/
/* Static inline functions                                           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Get short float from register                                     */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted to             */
/*      fpr     Pointer to register to be converted from             */
/*-------------------------------------------------------------------*/
static inline void get_sf( SHORT_FLOAT *fl, U32 *fpr )
{
    fl->sign = *fpr >> 31;
    fl->expo = (*fpr >> 24) & 0x007F;
    fl->short_fract = *fpr & 0x00FFFFFF;

} /* end function get_sf */


/*-------------------------------------------------------------------*/
/* Store short float to register                                     */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static inline void store_sf( SHORT_FLOAT *fl, U32 *fpr )
{
    *fpr = ((U32)fl->sign << 31)
         | ((U32)fl->expo << 24)
         | (fl->short_fract);

} /* end function store_sf */


/*-------------------------------------------------------------------*/
/* Get long float from register                                      */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted to             */
/*      fpr     Pointer to register to be converted from             */
/*-------------------------------------------------------------------*/
static inline void get_lf( LONG_FLOAT *fl, U32 *fpr )
{
    fl->sign = *fpr >> 31;
    fl->expo = (*fpr >> 24) & 0x007F;
    fl->long_fract = ((U64)(fpr[0] & 0x00FFFFFF) << 32)
                   | fpr[1];

} /* end function get_lf */


/*-------------------------------------------------------------------*/
/* Store long float to register                                      */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static inline void store_lf( LONG_FLOAT *fl, U32 *fpr )
{
    fpr[0] = ((U32)fl->sign << 31)
           | ((U32)fl->expo << 24)
           | (fl->long_fract >> 32);
    fpr[1] = fl->long_fract & 0xffffffff;

} /* end function store_lf */


/*-------------------------------------------------------------------*/
/* Get extended float from register                                  */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted to             */
/*      fpr     Pointer to register to be converted from             */
/*-------------------------------------------------------------------*/
static inline void get_ef( EXTENDED_FLOAT *fl, U32 *fpr )
{
    fl->sign = *fpr >> 31;
    fl->expo = (*fpr >> 24) & 0x007F;
    fl->ms_fract = ((U64)(fpr[0] & 0x00FFFFFF) << 24)
                 | (fpr[1] >> 8);
    fl->ls_fract = (((U64)fpr[1]) << 56)
                 | (((U64)(fpr[FPREX] & 0x00FFFFFF)) << 32)
                 | fpr[FPREX+1];

} /* end function get_ef */


/*-------------------------------------------------------------------*/
/* Store extended float to register                                  */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted from             */
/*-------------------------------------------------------------------*/
static inline void store_ef( EXTENDED_FLOAT *fl, U32 *fpr )
{
    fpr[0] = ((U32)fl->sign << 31)
           | ((U32)fl->expo << 24)
           | (fl->ms_fract >> 24);
    fpr[1] = (fl->ms_fract << 8)
           | (fl->ls_fract >> 56);
    fpr[FPREX] = ((U32)fl->sign << 31)
               | ((fl->ls_fract >> 32) & 0x00FFFFFF);
    fpr[FPREX+1] = fl->ls_fract & 0xffffffff;

    if ( fpr[0]
    || fpr[1]
    || fpr[FPREX]
    || fpr[FPREX+1] ) {
        fpr[FPREX] |= ((((U32)fl->expo - 14) << 24) & 0x7f000000);
    }

} /* end function store_ef */

#if defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY)
/*-------------------------------------------------------------------*/
/* Store extended float high-order part to register unnormalized     */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static inline void ARCH_DEP(store_ef_unnorm_hi)( EXTENDED_FLOAT *fl, U32 *fpr )
{
    fpr[0] = ((U32)fl->sign << 31)
           | (((U32)fl->expo & 0x7f) << 24)
           | ((fl->ms_fract >> 24) & 0x00FFFFFF);
    fpr[1] = (fl->ms_fract << 8)
           | (fl->ls_fract >> 56);

} /* end ARCH_DEP(store_ef_unnorm_hi) */


/*-------------------------------------------------------------------*/
/* Store extended float low-order part to register unnormalized      */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static inline void ARCH_DEP(store_ef_unnorm_lo)( EXTENDED_FLOAT *fl, U32 *fpr )
{
    fpr[0] = ((U32)fl->sign << 31)
           | ((((U32)fl->expo - 14 ) & 0x7f) << 24)
           | ((fl->ls_fract >> 32) & 0x00FFFFFF);
    fpr[1] = fl->ls_fract;

} /* end ARCH_DEP(store_ef_unnorm_lo) */

/*-------------------------------------------------------------------*/
/* Store extended float to register pair unnormalized                */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static inline void ARCH_DEP(store_ef_unnorm)( EXTENDED_FLOAT *fl, U32 *fpr )
{
    ARCH_DEP(store_ef_unnorm_hi)(fl,fpr);
    ARCH_DEP(store_ef_unnorm_lo)(fl,fpr+FPREX);

} /* end ARCH_DEP(store_ef_unnorm) */


/*-------------------------------------------------------------------*/
/* Convert long to extended format unnormalized                      */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static inline void ARCH_DEP(lf_to_ef_unnorm)
                           ( EXTENDED_FLOAT *fx, LONG_FLOAT *fl )
{
    fx->sign = fl->sign;
    fx->expo = fl->expo;
    fx->ms_fract = fl->long_fract >> 8;
    fx->ls_fract = (fl->long_fract & 0xff) << 56;

} /* end ARCH_DEP(lf_to_ef_unnorm) */

#endif /* defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY) */


/*-------------------------------------------------------------------*/
/* Fetch a short floatingpoint operand from virtual storage          */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format                                */
/*      addr    Logical address of leftmost byte of operand          */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
static inline void vfetch_sf( SHORT_FLOAT *fl, VADR addr, int arn,
    REGS *regs )
{
U32     value;                          /* Operand value             */

    /* Fetch 4 bytes from operand address */
    value = ARCH_DEP(vfetch4) (addr, arn, regs);

    /* Extract sign and exponent from high-order byte */
    fl->sign = value >> 31;
    fl->expo = (value >> 24) & 0x007F;

    /* Extract fraction from low-order 3 bytes */
    fl->short_fract = value & 0x00FFFFFF;

} /* end function vfetch_sf */


/*-------------------------------------------------------------------*/
/* Fetch a long floatingpoint operand from virtual storage           */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format                                */
/*      addr    Logical address of leftmost byte of operand          */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
static inline void vfetch_lf( LONG_FLOAT *fl, VADR addr, int arn,
    REGS *regs )
{
U64     value;                          /* Operand value             */

    /* Fetch 8 bytes from operand address */
    value = ARCH_DEP(vfetch8) (addr, arn, regs);

    /* Extract sign and exponent from high-order byte */
    fl->sign = value >> 63;
    fl->expo = (value >> 56) & 0x007F;

    /* Extract fraction from low-order 7 bytes */
    fl->long_fract = value & 0x00FFFFFFFFFFFFFFULL;

} /* end function vfetch_lf */


/*-------------------------------------------------------------------*/
/* Normalize short float                                             */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*-------------------------------------------------------------------*/
static inline void normal_sf( SHORT_FLOAT *fl )
{
    if (fl->short_fract) {
        if ((fl->short_fract & 0x00FFFF00) == 0) {
            fl->short_fract <<= 16;
            fl->expo -= 4;
        }
        if ((fl->short_fract & 0x00FF0000) == 0) {
            fl->short_fract <<= 8;
            fl->expo -= 2;
        }
        if ((fl->short_fract & 0x00F00000) == 0) {
            fl->short_fract <<= 4;
            (fl->expo)--;
        }
    } else {
        fl->sign = POS;
        fl->expo = 0;
    }

} /* end function normal_sf */


/*-------------------------------------------------------------------*/
/* Normalize long float                                              */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*-------------------------------------------------------------------*/
static inline void normal_lf( LONG_FLOAT *fl )
{
    if (fl->long_fract) {
        if ((fl->long_fract & 0x00FFFFFFFF000000ULL) == 0) {
            fl->long_fract <<= 32;
            fl->expo -= 8;
        }
        if ((fl->long_fract & 0x00FFFF0000000000ULL) == 0) {
            fl->long_fract <<= 16;
            fl->expo -= 4;
        }
        if ((fl->long_fract & 0x00FF000000000000ULL) == 0) {
            fl->long_fract <<= 8;
            fl->expo -= 2;
        }
        if ((fl->long_fract & 0x00F0000000000000ULL) == 0) {
            fl->long_fract <<= 4;
            (fl->expo)--;
        }
    } else {
        fl->sign = POS;
        fl->expo = 0;
    }

} /* end function normal_lf */


/*-------------------------------------------------------------------*/
/* Normalize extended float                                          */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*-------------------------------------------------------------------*/
static inline void normal_ef( EXTENDED_FLOAT *fl )
{
    if (fl->ms_fract
    || fl->ls_fract) {
        if (fl->ms_fract == 0) {
            fl->ms_fract = fl->ls_fract >> 16;
            fl->ls_fract <<= 48;
            fl->expo -= 12;
        }
        if ((fl->ms_fract & 0x0000FFFFFFFF0000ULL) == 0) {
            if (fl->ls_fract) {
                fl->ms_fract = (fl->ms_fract << 32)
                             | (fl->ls_fract >> 32);
                fl->ls_fract <<= 32;
            } else {
                fl->ms_fract <<= 32;
            }
            fl->expo -= 8;
        }
        if ((fl->ms_fract & 0x0000FFFF00000000ULL) == 0) {
            if (fl->ls_fract) {
                fl->ms_fract = (fl->ms_fract << 16)
                             | (fl->ls_fract >> 48);
                fl->ls_fract <<= 16;
            } else {
                fl->ms_fract <<= 16;
            }
            fl->expo -= 4;
        }
        if ((fl->ms_fract & 0x0000FF0000000000ULL) == 0) {
            if (fl->ls_fract) {
                fl->ms_fract = (fl->ms_fract << 8)
                             | (fl->ls_fract >> 56);
                fl->ls_fract <<= 8;
            } else {
                fl->ms_fract <<= 8;
            }
            fl->expo -= 2;
        }
        if ((fl->ms_fract & 0x0000F00000000000ULL) == 0) {
            if (fl->ls_fract) {
                fl->ms_fract = (fl->ms_fract << 4)
                             | (fl->ls_fract >> 60);
                fl->ls_fract <<= 4;
            } else {
                fl->ms_fract <<= 4;
            }
            (fl->expo)--;
        }
    } else {
        fl->sign = POS;
        fl->expo = 0;
    }

} /* end function normal_ef */


/*-------------------------------------------------------------------*/
/* Overflow of short float                                           */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int overflow_sf( SHORT_FLOAT *fl, REGS *regs )
{
    UNREFERENCED(regs);

    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    }
    return(0);

} /* end function overflow_sf */


/*-------------------------------------------------------------------*/
/* Overflow of long float                                            */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int overflow_lf( LONG_FLOAT *fl, REGS *regs )
{
    UNREFERENCED(regs);

    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    }
    return(0);

} /* end function overflow_lf */


/*-------------------------------------------------------------------*/
/* Overflow of extended float                                        */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int overflow_ef( EXTENDED_FLOAT *fl, REGS *regs )
{
    UNREFERENCED(regs);

    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    }
    return(0);

} /* end function overflow_ef */


/*-------------------------------------------------------------------*/
/* Underflow of short float                                          */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                                     */
/*-------------------------------------------------------------------*/
static inline int underflow_sf( SHORT_FLOAT *fl, REGS *regs )
{
    if (fl->expo < 0) {
        if (EUMASK(&regs->psw)) {
            fl->expo &= 0x007F;
            return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
        } else {
            /* set true 0 */

            fl->short_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    }
    return(0);

} /* end function underflow_sf */


/*-------------------------------------------------------------------*/
/* Underflow of long float                                           */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int underflow_lf( LONG_FLOAT *fl, REGS *regs )
{
    if (fl->expo < 0) {
        if (EUMASK(&regs->psw)) {
            fl->expo &= 0x007F;
            return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
        } else {
            /* set true 0 */

            fl->long_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    }
    return(0);

} /* end function underflow_lf */


/*-------------------------------------------------------------------*/
/* Underflow of extended float                                       */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      fpr     Pointer to register to be stored to                  */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int underflow_ef( EXTENDED_FLOAT *fl, U32 *fpr,
    REGS *regs )
{
    if (fl->expo < 0) {
        if (EUMASK(&regs->psw)) {
            fl->expo &= 0x007F;
            store_ef( fl, fpr );
            return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
        } else {
            /* set true 0 */

            fpr[0] = 0;
            fpr[1] = 0;
            fpr[FPREX] = 0;
            fpr[FPREX+1] = 0;
            fl->ms_fract = 0;
            fl->ls_fract = 0;
            return(0);
        }
    }

    store_ef( fl, fpr );
    return(0);

} /* end function underflow_ef */


/*-------------------------------------------------------------------*/
/* Overflow and underflow of short float                             */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int over_under_flow_sf( SHORT_FLOAT *fl, REGS *regs )
{
    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    } else {
        if (fl->expo < 0) {
            if (EUMASK(&regs->psw)) {
                fl->expo &= 0x007F;
                return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
            } else {
                /* set true 0 */

                fl->short_fract = 0;
                fl->expo = 0;
                fl->sign = POS;
            }
        }
    }
    return(0);

} /* end function over_under_flow_sf */


/*-------------------------------------------------------------------*/
/* Overflow and underflow of long float                              */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int over_under_flow_lf( LONG_FLOAT *fl, REGS *regs )
{
    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    } else {
        if (fl->expo < 0) {
            if (EUMASK(&regs->psw)) {
                fl->expo &= 0x007F;
                return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
            } else {
                /* set true 0 */

                fl->long_fract = 0;
                fl->expo = 0;
                fl->sign = POS;
            }
        }
    }
    return(0);

} /* end function over_under_flow_lf */


/*-------------------------------------------------------------------*/
/* Overflow and underflow of extended float                          */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int over_under_flow_ef( EXTENDED_FLOAT *fl, REGS *regs )
{
    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    } else {
        if (fl->expo < 0) {
            if (EUMASK(&regs->psw)) {
                fl->expo &= 0x007F;
                return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
            } else {
                /* set true 0 */

                fl->ms_fract = 0;
                fl->ls_fract = 0;
                fl->expo = 0;
                fl->sign = POS;
            }
        }
    }
    return(0);

} /* end function over_under_flow_ef */


/*-------------------------------------------------------------------*/
/* Significance of short float                                       */
/* The fraction is expected to be zero                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      sigex   Allow significance exception if true                 */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int significance_sf( SHORT_FLOAT *fl, BYTE sigex,
    REGS *regs )
{
    fl->sign = POS;
    if (sigex && SGMASK(&regs->psw)) {
        return(PGM_SIGNIFICANCE_EXCEPTION);
    }
    /* set true 0 */

    fl->expo = 0;
    return(0);

} /* end function significance_sf */


/*-------------------------------------------------------------------*/
/* Significance of long float                                        */
/* The fraction is expected to be zero                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      sigex   Allow significance exception if true                 */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int significance_lf( LONG_FLOAT *fl, BYTE sigex,
    REGS *regs )
{
    fl->sign = POS;
    if (sigex && SGMASK(&regs->psw)) {
        return(PGM_SIGNIFICANCE_EXCEPTION);
    }
    /* set true 0 */

    fl->expo = 0;
    return(0);

} /* end function significance_lf */


/*-------------------------------------------------------------------*/
/* Significance of extended float                                    */
/* The fraction is expected to be zero                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      fpr     Pointer to register to be stored to                  */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static inline int significance_ef( EXTENDED_FLOAT *fl, U32 *fpr,
    REGS *regs )
{
    fpr[1] = 0;
    fpr[FPREX+1] = 0;

    if (SGMASK(&regs->psw)) {
        fpr[0] = (U32)fl->expo << 24;
        fpr[FPREX] = (((U32)fl->expo - 14) << 24) & 0x7f000000;
        return(PGM_SIGNIFICANCE_EXCEPTION);
    }
    /* set true 0 */

    fpr[0] = 0;
    fpr[FPREX] = 0;
    return(0);

} /* end function significance_ef */


/*-------------------------------------------------------------------*/
/* Static functions                                                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Add short float                                                   */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      add_fl  Float to be added                                    */
/*      normal  Normalize if true                                    */
/*      sigex   Allow significance exception if true                 */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int add_sf( SHORT_FLOAT *fl, SHORT_FLOAT *add_fl,
    BYTE normal, BYTE sigex, REGS *regs )
{
int     pgm_check;
BYTE    shift;

    pgm_check = 0;
    if (add_fl->short_fract
    || add_fl->expo) {          /* add_fl not 0 */
        if (fl->short_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == add_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->short_fract <<= 4;
                add_fl->short_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < add_fl->expo) {
                    /* shift minus guard digit */
                    shift = add_fl->expo - fl->expo - 1;
                    fl->expo = add_fl->expo;

                    if (shift) {
                        if (shift >= 6
                        || ((fl->short_fract >>= (shift * 4)) == 0)) {
                            /* 0, copy summand */

                            fl->sign = add_fl->sign;
                            fl->short_fract = add_fl->short_fract;

                            if (fl->short_fract == 0) {
                                pgm_check = significance_sf(fl, sigex, regs);
                            } else {
                                if (normal == NORMAL) {
                                    normal_sf(fl);
                                    pgm_check = underflow_sf(fl, regs);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    add_fl->short_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - add_fl->expo - 1;

                    if (shift) {
                        if (shift >= 6
                        || ((add_fl->short_fract >>= (shift * 4)) == 0)) {
                            /* 0, nothing to add */

                            if (fl->short_fract == 0) {
                                pgm_check = significance_sf(fl, sigex, regs);
                            } else {
                                if (normal == NORMAL) {
                                    normal_sf(fl);
                                    pgm_check = underflow_sf(fl, regs);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    fl->short_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign == add_fl->sign) {
                fl->short_fract += add_fl->short_fract;
            } else {
                if (fl->short_fract == add_fl->short_fract) {
                    /* true 0 */

                    fl->short_fract = 0;
                    return( significance_sf(fl, sigex, regs) );

                } else if (fl->short_fract > add_fl->short_fract) {
                    fl->short_fract -= add_fl->short_fract;
                } else {
                    fl->short_fract = add_fl->short_fract - fl->short_fract;
                    fl->sign = add_fl->sign;
                }
            }

            /* handle overflow with guard digit */
            if (fl->short_fract & 0xF0000000) {
                fl->short_fract >>= 8;
                (fl->expo)++;
                pgm_check = overflow_sf(fl, regs);
            } else {

                if (normal == NORMAL) {
                    /* normalize with guard digit */
                    if (fl->short_fract) {
                        /* not 0 */

                        if (fl->short_fract & 0x0F000000) {
                            /* not normalize, just guard digit */
                            fl->short_fract >>= 4;
                        } else {
                            (fl->expo)--;
                            normal_sf(fl);
                            pgm_check = underflow_sf(fl, regs);
                        }
                    } else {
                        /* true 0 */

                        pgm_check = significance_sf(fl, sigex, regs);
                    }
                } else {
                    /* not normalize, just guard digit */
                    fl->short_fract >>= 4;
                    if (fl->short_fract == 0) {
                        pgm_check = significance_sf(fl, sigex, regs);
                    }
                }
            }
            return(pgm_check);
        } else { /* fl 0, add_fl not 0 */
            /* copy summand */

            fl->expo = add_fl->expo;
            fl->sign = add_fl->sign;
            fl->short_fract = add_fl->short_fract;
            if (fl->short_fract == 0) {
                return( significance_sf(fl, sigex, regs) );
            }
        }
    } else {                        /* add_fl 0 */
        if (fl->short_fract == 0) { /* fl 0 */
            /* both 0 */

            return( significance_sf(fl, sigex, regs) );
        }
    }
    if (normal == NORMAL) {
        normal_sf(fl);
        pgm_check = underflow_sf(fl, regs);
    }
    return(pgm_check);

} /* end function add_sf */


/*-------------------------------------------------------------------*/
/* Add long float                                                    */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      add_fl  Float to be added                                    */
/*      normal  Normalize if true                                    */
/*      sigex   Allow significance exception if true                 */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int add_lf( LONG_FLOAT *fl, LONG_FLOAT *add_fl,
    BYTE normal, BYTE sigex, REGS *regs )
{
int     pgm_check;
BYTE    shift;

    pgm_check = 0;
    if (add_fl->long_fract
    || add_fl->expo) {          /* add_fl not 0 */
        if (fl->long_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == add_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->long_fract <<= 4;
                add_fl->long_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < add_fl->expo) {
                    /* shift minus guard digit */
                    shift = add_fl->expo - fl->expo - 1;
                    fl->expo = add_fl->expo;

                    if (shift) {
                        if (shift >= 14
                        || ((fl->long_fract >>= (shift * 4)) == 0)) {
                            /* 0, copy summand */

                            fl->sign = add_fl->sign;
                            fl->long_fract = add_fl->long_fract;

                            if (fl->long_fract == 0) {
                                pgm_check = significance_lf(fl, sigex, regs);
                            } else {
                                if (normal == NORMAL) {
                                    normal_lf(fl);
                                    pgm_check = underflow_lf(fl, regs);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    add_fl->long_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - add_fl->expo - 1;

                    if (shift) {
                        if (shift >= 14
                        || ((add_fl->long_fract >>= (shift * 4)) == 0)) {
                            /* 0, nothing to add */

                            if (fl->long_fract == 0) {
                                pgm_check = significance_lf(fl, sigex, regs);
                            } else {
                                if (normal == NORMAL) {
                                    normal_lf(fl);
                                    pgm_check = underflow_lf(fl, regs);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    fl->long_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign == add_fl->sign) {
                fl->long_fract += add_fl->long_fract;
            } else {
                if (fl->long_fract == add_fl->long_fract) {
                    /* true 0 */

                    fl->long_fract = 0;
                    return( significance_lf(fl, sigex, regs) );

                } else if (fl->long_fract > add_fl->long_fract) {
                    fl->long_fract -= add_fl->long_fract;
                } else {
                    fl->long_fract = add_fl->long_fract - fl->long_fract;
                    fl->sign = add_fl->sign;
                }
            }

            /* handle overflow with guard digit */
            if (fl->long_fract & 0xF000000000000000ULL) {
                fl->long_fract >>= 8;
                (fl->expo)++;
                pgm_check = overflow_lf(fl, regs);
            } else {

                if (normal == NORMAL) {
                    /* normalize with guard digit */
                    if (fl->long_fract) {
                        /* not 0 */

                        if (fl->long_fract & 0x0F00000000000000ULL) {
                            /* not normalize, just guard digit */
                            fl->long_fract >>= 4;
                        } else {
                            (fl->expo)--;
                            normal_lf(fl);
                            pgm_check = underflow_lf(fl, regs);
                        }
                    } else {
                        /* true 0 */

                        pgm_check = significance_lf(fl, sigex, regs);
                    }
                } else {
                    /* not normalize, just guard digit */
                    fl->long_fract >>= 4;
                    if (fl->long_fract == 0) {
                        pgm_check = significance_lf(fl, sigex, regs);
                    }
                }
            }
            return(pgm_check);
        } else { /* fl 0, add_fl not 0 */
            /* copy summand */

            fl->expo = add_fl->expo;
            fl->sign = add_fl->sign;
            fl->long_fract = add_fl->long_fract;
            if (fl->long_fract == 0) {
                return( significance_lf(fl, sigex, regs) );
            }
        }
    } else {                       /* add_fl 0 */
        if (fl->long_fract == 0) { /* fl 0 */
            /* both 0 */

            return( significance_lf(fl, sigex, regs) );
        }
    }
    if (normal == NORMAL) {
        normal_lf(fl);
        pgm_check = underflow_lf(fl, regs);
    }
    return(pgm_check);

} /* end function add_lf */


/*-------------------------------------------------------------------*/
/* Add extended float normalized                                     */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      add_fl  Float to be added                                    */
/*      fpr     Pointer to register                                  */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int add_ef( EXTENDED_FLOAT *fl, EXTENDED_FLOAT *add_fl,
    U32 *fpr, REGS *regs )
{
int     pgm_check;
BYTE    shift;

    pgm_check = 0;
    if (add_fl->ms_fract
    || add_fl->ls_fract
    || add_fl->expo) {          /* add_fl not 0 */
        if (fl->ms_fract
        || fl->ls_fract
        || fl->expo)    {       /* fl not 0 */
            /* both not 0 */

            if (fl->expo == add_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->ms_fract = (fl->ms_fract << 4)
                             | (fl->ls_fract >> 60);
                fl->ls_fract <<= 4;
                add_fl->ms_fract = (add_fl->ms_fract << 4)
                                 | (add_fl->ls_fract >> 60);
                add_fl->ls_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < add_fl->expo) {
                    /* shift minus guard digit */
                    shift = add_fl->expo - fl->expo - 1;
                    fl->expo = add_fl->expo;

                    if (shift) {
                        if (shift >= 28) {
                            /* 0, copy summand */

                            fl->sign = add_fl->sign;
                            fl->ms_fract = add_fl->ms_fract;
                            fl->ls_fract = add_fl->ls_fract;

                            if ((fl->ms_fract == 0)
                            && (fl->ls_fract == 0)) {
                                pgm_check = significance_ef(fl, fpr, regs);
                            } else {
                                normal_ef(fl);
                                pgm_check =  underflow_ef(fl, fpr, regs);
                            }
                            return(pgm_check);
                        } else if (shift >= 16) {
                            fl->ls_fract = fl->ms_fract;
                            if (shift > 16) {
                                fl->ls_fract >>= (shift - 16) * 4;
                            }
                            fl->ms_fract = 0;
                        } else {
                            shift *= 4;
                            fl->ls_fract = fl->ms_fract << (64 - shift)
                                         | fl->ls_fract >> shift;
                            fl->ms_fract >>= shift;
                        }

                        if ((fl->ms_fract == 0)
                        && (fl->ls_fract == 0)) {
                            /* 0, copy summand */

                            fl->sign = add_fl->sign;
                            fl->ms_fract = add_fl->ms_fract;
                            fl->ls_fract = add_fl->ls_fract;

                            if ((fl->ms_fract == 0)
                            && (fl->ls_fract == 0)) {
                                pgm_check = significance_ef(fl, fpr, regs);
                            } else {
                                normal_ef(fl);
                                pgm_check = underflow_ef(fl, fpr, regs);
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    add_fl->ms_fract = (add_fl->ms_fract << 4)
                                     | (add_fl->ls_fract >> 60);
                    add_fl->ls_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - add_fl->expo - 1;

                    if (shift) {
                        if (shift >= 28) {
                            /* 0, nothing to add */

                            if ((fl->ms_fract == 0)
                            && (fl->ls_fract == 0)) {
                                pgm_check = significance_ef(fl, fpr, regs);
                            } else {
                                normal_ef(fl);
                                pgm_check = underflow_ef(fl, fpr, regs);
                            }
                            return(pgm_check);
                        } else if (shift >= 16) {
                            add_fl->ls_fract = add_fl->ms_fract;
                            if (shift > 16) {
                                add_fl->ls_fract >>= (shift - 16) * 4;
                            }
                            add_fl->ms_fract = 0;
                        } else {
                            shift *= 4;
                            add_fl->ls_fract = add_fl->ms_fract << (64 - shift)
                                             | add_fl->ls_fract >> shift;
                            add_fl->ms_fract >>= shift;
                        }

                        if ((add_fl->ms_fract == 0)
                        && (add_fl->ls_fract == 0)) {
                            /* 0, nothing to add */

                            if ((fl->ms_fract == 0)
                            && (fl->ls_fract == 0)) {
                                pgm_check = significance_ef(fl, fpr, regs);
                            } else {
                                normal_ef(fl);
                                pgm_check = underflow_ef(fl, fpr, regs);
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    fl->ms_fract = (fl->ms_fract << 4)
                                 | (fl->ls_fract >> 60);
                    fl->ls_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign == add_fl->sign) {
                add_U128(fl->ms_fract, fl->ls_fract, add_fl->ms_fract, add_fl->ls_fract);
            } else {
                if ((fl->ms_fract == add_fl->ms_fract)
                && (fl->ls_fract == add_fl->ls_fract)) {
                    /* true 0 */

                    fl->ms_fract = 0;
                    fl->ls_fract = 0;
                    return( significance_ef(fl, fpr, regs) );

                } else if ( (fl->ms_fract > add_fl->ms_fract)
                       || ( (fl->ms_fract == add_fl->ms_fract)
                           && (fl->ls_fract > add_fl->ls_fract))) {
                    sub_U128(fl->ms_fract, fl->ls_fract, add_fl->ms_fract, add_fl->ls_fract);
                } else {
                    sub_reverse_U128(fl->ms_fract, fl->ls_fract, add_fl->ms_fract, add_fl->ls_fract);
                    fl->sign = add_fl->sign;
                }
            }

            /* handle overflow with guard digit */
            if (fl->ms_fract & 0x00F0000000000000ULL) {
                fl->ls_fract = (fl->ms_fract << 56)
                             | (fl->ls_fract >> 8);
                fl->ms_fract >>= 8;
                (fl->expo)++;
                pgm_check = overflow_ef(fl, regs);
                store_ef( fl, fpr );
            } else {
                /* normalize with guard digit */
                if (fl->ms_fract
                || fl->ls_fract) {
                    /* not 0 */

                    if (fl->ms_fract & 0x000F000000000000ULL) {
                        /* not normalize, just guard digit */
                        fl->ls_fract = (fl->ms_fract << 60)
                                     | (fl->ls_fract >> 4);
                        fl->ms_fract >>= 4;
                        store_ef( fl, fpr );
                    } else {
                        (fl->expo)--;
                        normal_ef(fl);
                        pgm_check = underflow_ef(fl, fpr, regs);
                    }
                } else {
                    /* true 0 */

                    pgm_check = significance_ef(fl, fpr, regs);
                }
            }
            return(pgm_check);
        } else { /* fl 0, add_fl not 0 */
            /* copy summand */

            fl->expo = add_fl->expo;
            fl->sign = add_fl->sign;
            fl->ms_fract = add_fl->ms_fract;
            fl->ls_fract = add_fl->ls_fract;
            if ((fl->ms_fract == 0)
            && (fl->ls_fract == 0)) {
                return( significance_ef(fl, fpr, regs) );
            }
        }
    } else {                                              /* add_fl 0*/
        if ((fl->ms_fract == 0)
        && (fl->ls_fract == 0)) { /* fl 0 */
            /* both 0 */

            return( significance_ef(fl, fpr, regs) );
        }
    }
    normal_ef(fl);
    return( underflow_ef(fl, fpr, regs) );

} /* end function add_ef */


/*-------------------------------------------------------------------*/
/* Compare short float                                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      cmp_fl  Float to be compared                                 */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static void cmp_sf( SHORT_FLOAT *fl, SHORT_FLOAT *cmp_fl, REGS *regs )
{
BYTE    shift;

    if (cmp_fl->short_fract
    || cmp_fl->expo) {          /* cmp_fl not 0 */
        if (fl->short_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == cmp_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->short_fract <<= 4;
                cmp_fl->short_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < cmp_fl->expo) {
                    /* shift minus guard digit */
                    shift = cmp_fl->expo - fl->expo - 1;

                    if (shift) {
                        if (shift >= 6
                        || ((fl->short_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (cmp_fl->short_fract) {
                                regs->psw.cc = cmp_fl->sign ? 2 : 1;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    cmp_fl->short_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - cmp_fl->expo - 1;

                    if (shift) {
                        if (shift >= 6
                        || ((cmp_fl->short_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (fl->short_fract) {
                                regs->psw.cc = fl->sign ? 1 : 2;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    fl->short_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign != cmp_fl->sign) {
                fl->short_fract += cmp_fl->short_fract;
            } else if (fl->short_fract >= cmp_fl->short_fract) {
                fl->short_fract -= cmp_fl->short_fract;
            } else {
                fl->short_fract = cmp_fl->short_fract - fl->short_fract;
                fl->sign = ! (cmp_fl->sign);
            }

            /* handle overflow with guard digit */
            if (fl->short_fract & 0xF0000000) {
                fl->short_fract >>= 4;
            }

            /* Set condition code */
            if (fl->short_fract) {
                regs->psw.cc = fl->sign ? 1 : 2;
            } else {
                regs->psw.cc = 0;
            }
            return;
        } else { /* fl 0, cmp_fl not 0 */
            /* Set condition code */
            if (cmp_fl->short_fract) {
                regs->psw.cc = cmp_fl->sign ? 2 : 1;
            } else {
                regs->psw.cc = 0;
            }
            return;
        }
    } else {                        /* cmp_fl 0 */
        /* Set condition code */
        if (fl->short_fract) {
            regs->psw.cc = fl->sign ? 1 : 2;
        } else {
            regs->psw.cc = 0;
        }
        return;
    }

} /* end function cmp_sf */


/*-------------------------------------------------------------------*/
/* Compare long float                                                */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      cmp_fl  Float to be compared                                 */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static void cmp_lf( LONG_FLOAT *fl, LONG_FLOAT *cmp_fl, REGS *regs )
{
BYTE    shift;

    if (cmp_fl->long_fract
    || cmp_fl->expo) {          /* cmp_fl not 0 */
        if (fl->long_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == cmp_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->long_fract <<= 4;
                cmp_fl->long_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < cmp_fl->expo) {
                    /* shift minus guard digit */
                    shift = cmp_fl->expo - fl->expo - 1;

                    if (shift) {
                        if (shift >= 14
                        || ((fl->long_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (cmp_fl->long_fract) {
                                regs->psw.cc = cmp_fl->sign ? 2 : 1;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    cmp_fl->long_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - cmp_fl->expo - 1;

                    if (shift) {
                        if (shift >= 14
                        || ((cmp_fl->long_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (fl->long_fract) {
                                regs->psw.cc = fl->sign ? 1 : 2;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    fl->long_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign != cmp_fl->sign) {
                fl->long_fract += cmp_fl->long_fract;
            } else if (fl->long_fract >= cmp_fl->long_fract) {
                fl->long_fract -= cmp_fl->long_fract;
            } else {
                fl->long_fract = cmp_fl->long_fract - fl->long_fract;
                fl->sign = ! (cmp_fl->sign);
            }

            /* handle overflow with guard digit */
            if (fl->long_fract & 0xF0000000) {
                fl->long_fract >>= 4;
            }

            /* Set condition code */
            if (fl->long_fract) {
                regs->psw.cc = fl->sign ? 1 : 2;
            } else {
                regs->psw.cc = 0;
            }
            return;
        } else { /* fl 0, cmp_fl not 0 */
            /* Set condition code */
            if (cmp_fl->long_fract) {
                regs->psw.cc = cmp_fl->sign ? 2 : 1;
            } else {
                regs->psw.cc = 0;
            }
            return;
        }
    } else {                        /* cmp_fl 0 */
        /* Set condition code */
        if (fl->long_fract) {
            regs->psw.cc = fl->sign ? 1 : 2;
        } else {
            regs->psw.cc = 0;
        }
        return;
    }

} /* end function cmp_lf */


/*-------------------------------------------------------------------*/
/* Multiply short float to long float                                */
/*                                                                   */
/* Input:                                                            */
/*      fl      Multiplicand short float                             */
/*      mul_fl  Multiplicator short float                            */
/*      result_fl       Result long float                            */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int mul_sf_to_lf( SHORT_FLOAT *fl, SHORT_FLOAT *mul_fl,
    LONG_FLOAT *result_fl, REGS *regs )
{
    if (fl->short_fract
    && mul_fl->short_fract) {
        /* normalize operands */
        normal_sf( fl );
        normal_sf( mul_fl );

        /* multiply fracts */
        result_fl->long_fract = (U64) fl->short_fract * mul_fl->short_fract;

        /* normalize result and compute expo */
        if (result_fl->long_fract & 0x0000F00000000000ULL) {
            result_fl->long_fract <<= 8;
            result_fl->expo = fl->expo + mul_fl->expo - 64;
        } else {
            result_fl->long_fract <<= 12;
            result_fl->expo = fl->expo + mul_fl->expo - 65;
        }

        /* determine sign */
        result_fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

        /* handle overflow and underflow */
        return( over_under_flow_lf(result_fl, regs) );
    } else {
        /* set true 0 */

        result_fl->long_fract = 0;
        result_fl->expo = 0;
        result_fl->sign = POS;
        return(0);
    }

} /* end function mul_sf_to_lf */


/*-------------------------------------------------------------------*/
/* Multiply long float to extended float                             */
/*                                                                   */
/* Input:                                                            */
/*      fl      Multiplicand long float                              */
/*      mul_fl  Multiplicator long float                             */
/*      result_fl       Result extended float                        */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int mul_lf_to_ef( LONG_FLOAT *fl, LONG_FLOAT *mul_fl,
    EXTENDED_FLOAT *result_fl, REGS *regs )
{
U64     wk;

    if (fl->long_fract
    && mul_fl->long_fract) {
        /* normalize operands */
        normal_lf( fl );
        normal_lf( mul_fl );

        /* multiply fracts by sum of partial multiplications */
        wk = (fl->long_fract & 0x00000000FFFFFFFFULL) * (mul_fl->long_fract & 0x00000000FFFFFFFFULL);
        result_fl->ls_fract = wk & 0x00000000FFFFFFFFULL;

        wk >>= 32;
        wk += ((fl->long_fract & 0x00000000FFFFFFFFULL) * (mul_fl->long_fract >> 32));
        wk += ((fl->long_fract >> 32) * (mul_fl->long_fract & 0x00000000FFFFFFFFULL));
        result_fl->ls_fract |= wk << 32;

        result_fl->ms_fract = (wk >> 32) + ((fl->long_fract >> 32) * (mul_fl->long_fract >> 32));

        /* normalize result and compute expo */
        if (result_fl->ms_fract & 0x0000F00000000000ULL) {
            result_fl->expo = fl->expo + mul_fl->expo - 64;
        } else {
            result_fl->ms_fract = (result_fl->ms_fract << 4)
                                | (result_fl->ls_fract >> 60);
            result_fl->ls_fract <<= 4;
            result_fl->expo = fl->expo + mul_fl->expo - 65;
        }

        /* determine sign */
        result_fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

        /* handle overflow and underflow */
        return( over_under_flow_ef(result_fl, regs) );
    } else {
        /* set true 0 */

        result_fl->ms_fract = 0;
        result_fl->ls_fract = 0;
        result_fl->expo = 0;
        result_fl->sign = POS;
        return(0);
    }

} /* end function mul_lf_to_ef */


#if defined(FEATURE_HFP_EXTENSIONS) \
 || defined(FEATURE_020_HFP_MULT_ADD_SUB_FACILITY)
/*-------------------------------------------------------------------*/
/* Multiply short float                                              */
/*                                                                   */
/* Input:                                                            */
/*      fl      Multiplicand short float                             */
/*      mul_fl  Multiplicator short float                            */
/*      ovunf   Handle overflow/underflow if true                    */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int mul_sf( SHORT_FLOAT *fl, SHORT_FLOAT *mul_fl,
    BYTE ovunf, REGS *regs )
{
U64     wk;

    if (fl->short_fract
    && mul_fl->short_fract) {
        /* normalize operands */
        normal_sf( fl );
        normal_sf( mul_fl );

        /* multiply fracts */
        wk = (U64) fl->short_fract * mul_fl->short_fract;

        /* normalize result and compute expo */
        if (wk & 0x0000F00000000000ULL) {
            fl->short_fract = wk >> 24;
            fl->expo = fl->expo + mul_fl->expo - 64;
        } else {
            fl->short_fract = wk >> 20;
            fl->expo = fl->expo + mul_fl->expo - 65;
        }

        /* determine sign */
        fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

        /* handle overflow and underflow if required */
        if (ovunf == OVUNF)
            return( over_under_flow_sf(fl, regs) );

        /* otherwise leave exponent as is */
        return(0);
    } else {
        /* set true 0 */

        fl->short_fract = 0;
        fl->expo = 0;
        fl->sign = POS;
        return(0);
    }

} /* end function mul_sf */
#endif /*FEATURE_HFP_EXTENSIONS || FEATURE_020_HFP_MULT_ADD_SUB_FACILITY*/


/*-------------------------------------------------------------------*/
/* Multiply long float                                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Multiplicand long float                              */
/*      mul_fl  Multiplicator long float                             */
/*      ovunf   Handle overflow/underflow if true                    */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int mul_lf( LONG_FLOAT *fl, LONG_FLOAT *mul_fl,
    BYTE ovunf, REGS *regs )
{
U64     wk;
U32     v;

    if (fl->long_fract
    && mul_fl->long_fract) {
        /* normalize operands */
        normal_lf( fl );
        normal_lf( mul_fl );

        /* multiply fracts by sum of partial multiplications */
        wk = ((fl->long_fract & 0x00000000FFFFFFFFULL) * (mul_fl->long_fract & 0x00000000FFFFFFFFULL)) >> 32;

        wk += ((fl->long_fract & 0x00000000FFFFFFFFULL) * (mul_fl->long_fract >> 32));
        wk += ((fl->long_fract >> 32) * (mul_fl->long_fract & 0x00000000FFFFFFFFULL));
        v = wk & 0xffffffff;

        fl->long_fract = (wk >> 32) + ((fl->long_fract >> 32) * (mul_fl->long_fract >> 32));

        /* normalize result and compute expo */
        if (fl->long_fract & 0x0000F00000000000ULL) {
            fl->long_fract = (fl->long_fract << 8)
                           | (v >> 24);
            fl->expo = fl->expo + mul_fl->expo - 64;
        } else {
            fl->long_fract = (fl->long_fract << 12)
                           | (v >> 20);
            fl->expo = fl->expo + mul_fl->expo - 65;
        }

        /* determine sign */
        fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

        /* handle overflow and underflow if required */
        if (ovunf == OVUNF)
            return( over_under_flow_lf(fl, regs) );

        /* otherwise leave exponent as is */
        return(0);
    } else {
        /* set true 0 */

        fl->long_fract = 0;
        fl->expo = 0;
        fl->sign = POS;
        return(0);
    }

} /* end function mul_lf */


/*-------------------------------------------------------------------*/
/* Multiply extended float                                           */
/*                                                                   */
/* Input:                                                            */
/*      fl      Multiplicand extended float                          */
/*      mul_fl  Multiplicator extended float                         */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int mul_ef( EXTENDED_FLOAT *fl, EXTENDED_FLOAT *mul_fl,
    REGS *regs )
{
U64 wk1;
U64 wk2;
U64 wk3;
U64 wk4;
U64 wk;
U32 wk0;
U32 v;

    if ((fl->ms_fract
        || fl->ls_fract)
    && (mul_fl->ms_fract
        || mul_fl->ls_fract)) {
        /* normalize operands */
        normal_ef ( fl );
        normal_ef ( mul_fl );

        /* multiply fracts by sum of partial multiplications */
        wk0 = ((fl->ls_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ls_fract & 0x00000000FFFFFFFFULL)) >> 32;

        wk1 = (fl->ls_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ls_fract >> 32);
        wk2 = (fl->ls_fract >> 32) * (mul_fl->ls_fract & 0x00000000FFFFFFFFULL);
        wk = wk0 + (wk1 & 0x00000000FFFFFFFFULL) + (wk2 & 0x00000000FFFFFFFFULL);
        wk = (wk >> 32) + (wk1 >> 32) + (wk2 >> 32);

        wk1 = (fl->ls_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ms_fract & 0x00000000FFFFFFFFULL);
        wk2 = (fl->ls_fract >> 32) * (mul_fl->ls_fract >> 32);
        wk3 = (fl->ms_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ls_fract & 0x00000000FFFFFFFFULL);
        wk += ((wk1 & 0x00000000FFFFFFFFULL) + (wk2 & 0x00000000FFFFFFFFULL) + (wk3 & 0x00000000FFFFFFFFULL));
        wk = (wk >> 32) + (wk1 >> 32) + (wk2 >> 32) + (wk3 >> 32);

        wk1 = (fl->ls_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ms_fract >> 32);
        wk2 = (fl->ls_fract >> 32) * (mul_fl->ms_fract & 0x00000000FFFFFFFFULL);
        wk3 = (fl->ms_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ls_fract >> 32);
        wk4 = (fl->ms_fract >> 32) * (mul_fl->ls_fract & 0x00000000FFFFFFFFULL);
        wk += ((wk1 & 0x00000000FFFFFFFFULL) + (wk2 & 0x00000000FFFFFFFFULL) + (wk3 & 0x00000000FFFFFFFFULL) + (wk4 & 0x00000000FFFFFFFFULL));
        v = wk;
        wk = (wk >> 32) + (wk1 >> 32) + (wk2 >> 32) + (wk3 >> 32) + (wk4 >> 32);

        wk1 = (fl->ls_fract >> 32) * (mul_fl->ms_fract >> 32);
        wk2 = (fl->ms_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ms_fract & 0x00000000FFFFFFFFULL);
        wk3 = (fl->ms_fract >> 32) * (mul_fl->ls_fract >> 32);
        wk += ((wk1 & 0x00000000FFFFFFFFULL) + (wk2 & 0x00000000FFFFFFFFULL) + (wk3 & 0x00000000FFFFFFFFULL));
        fl->ls_fract = wk & 0x00000000FFFFFFFFULL;
        wk = (wk >> 32) + (wk1 >> 32) + (wk2 >> 32) + (wk3 >> 32);

        wk1 = (fl->ms_fract & 0x00000000FFFFFFFFULL) * (mul_fl->ms_fract >> 32);
        wk2 = (fl->ms_fract >> 32) * (mul_fl->ms_fract & 0x00000000FFFFFFFFULL);
        wk += ((wk1 & 0x00000000FFFFFFFFULL) + (wk2 & 0x00000000FFFFFFFFULL));
        fl->ls_fract |= wk << 32;
        wk0 = (wk >> 32) + (wk1 >> 32) + (wk2 >> 32);

        wk0 += ((fl->ms_fract >> 32) * (mul_fl->ms_fract >> 32));
        fl->ms_fract = wk0;

        /* normalize result and compute expo */
        if (wk0 & 0xF0000000UL) {
            fl->ms_fract = (fl->ms_fract << 16)
                         | (fl->ls_fract >> 48);
            fl->ls_fract = (fl->ls_fract << 16)
                         | (v >> 16);
            fl->expo = fl->expo + mul_fl->expo - 64;
        } else {
            fl->ms_fract = (fl->ms_fract << 20)
                         | (fl->ls_fract >> 44);
            fl->ls_fract = (fl->ls_fract << 20)
                         | (v >> 12);
            fl->expo = fl->expo + mul_fl->expo - 65;
        }

        /* determine sign */
        fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

        /* handle overflow and underflow */
        return ( over_under_flow_ef (fl, regs) );
    } else {
        /* set true 0 */

        fl->ms_fract = 0;
        fl->ls_fract = 0;
        fl->expo = 0;
        fl->sign = POS;
        return (0);
    }

} /* end function mul_ef */


/*-------------------------------------------------------------------*/
/* Divide short float                                                */
/*                                                                   */
/* Input:                                                            */
/*      fl      Dividend short float                                 */
/*      div_fl  Divisor short float                                  */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int div_sf( SHORT_FLOAT *fl, SHORT_FLOAT *div_fl, REGS *regs )
{
U64     wk;

    if (div_fl->short_fract) {
        if (fl->short_fract) {
            /* normalize operands */
            normal_sf( fl );
            normal_sf( div_fl );

            /* position fracts and compute expo */
            if (fl->short_fract < div_fl->short_fract) {
                wk = (U64) fl->short_fract << 24;
                fl->expo = fl->expo - div_fl->expo + 64;
            } else {
                wk = (U64) fl->short_fract << 20;
                fl->expo = fl->expo - div_fl->expo + 65;
            }
            /* divide fractions */
            fl->short_fract = wk / div_fl->short_fract;

            /* determine sign */
            fl->sign = (fl->sign == div_fl->sign) ? POS : NEG;

            /* handle overflow and underflow */
            return( over_under_flow_sf(fl, regs) );
        } else {
            /* fraction of dividend 0, set true 0 */

            fl->short_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    } else {
        /* divisor 0 */

        ARCH_DEP(program_interrupt) (regs, PGM_FLOATING_POINT_DIVIDE_EXCEPTION);
    }
    return(0);

} /* end function div_sf */


/*-------------------------------------------------------------------*/
/* Divide long float                                                 */
/*                                                                   */
/* Input:                                                            */
/*      fl      Dividend long float                                  */
/*      div_fl  Divisor long float                                   */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int div_lf( LONG_FLOAT *fl, LONG_FLOAT *div_fl, REGS *regs )
{
U64     wk;
U64     wk2;
int     i;

    if (div_fl->long_fract) {
        if (fl->long_fract) {
            /* normalize operands */
            normal_lf( fl );
            normal_lf( div_fl );

            /* position fracts and compute expo */
            if (fl->long_fract < div_fl->long_fract) {
                fl->expo = fl->expo - div_fl->expo + 64;
            } else {
                fl->expo = fl->expo - div_fl->expo + 65;
                div_fl->long_fract <<= 4;
            }

            /* partial divide first hex digit */
            wk2 = fl->long_fract / div_fl->long_fract;
            wk = (fl->long_fract % div_fl->long_fract) << 4;

            /* partial divide middle hex digits */
            i = 13;
            while (i--) {
                wk2 = (wk2 << 4)
                    | (wk / div_fl->long_fract);
                wk = (wk % div_fl->long_fract) << 4;
            }

            /* partial divide last hex digit */
            fl->long_fract = (wk2 << 4)
                           | (wk / div_fl->long_fract);

            /* determine sign */
            fl->sign = (fl->sign == div_fl->sign) ? POS : NEG;

            /* handle overflow and underflow */
            return( over_under_flow_lf(fl, regs) );
        } else {
            /* fraction of dividend 0, set true 0 */

            fl->long_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    } else {
        /* divisor 0 */

        ARCH_DEP(program_interrupt) (regs, PGM_FLOATING_POINT_DIVIDE_EXCEPTION);
    }
    return(0);

} /* end function div_lf */


/*-------------------------------------------------------------------*/
/* Divide extended float                                             */
/*                                                                   */
/* Input:                                                            */
/*      fl      Dividend extended float                              */
/*      div_fl  Divisor extended float                               */
/*      regs    CPU register context                                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int div_ef( EXTENDED_FLOAT *fl, EXTENDED_FLOAT *div_fl,
    REGS *regs )
{
U64     wkm;
U64     wkl;
int     i;

    if (div_fl->ms_fract
    || div_fl->ls_fract) {
        if (fl->ms_fract
        || fl->ls_fract) {
            /* normalize operands */
            normal_ef( fl );
            normal_ef( div_fl );

            /* position fracts and compute expo */
            if ((fl->ms_fract < div_fl->ms_fract)
            || ((fl->ms_fract == div_fl->ms_fract)
                && (fl->ls_fract < div_fl->ls_fract))) {
                fl->expo = fl->expo - div_fl->expo + 64;
            } else {
                fl->expo = fl->expo - div_fl->expo + 65;
                div_fl->ms_fract = (div_fl->ms_fract << 4)
                                 | (div_fl->ls_fract >> 60);
                div_fl->ls_fract <<= 4;
            }

            /* divide fractions */

            /* the first binary digit */
            wkm = fl->ms_fract;
            wkl = fl->ls_fract;
            sub_U128(wkm, wkl, div_fl->ms_fract, div_fl->ls_fract);
            wkm = (wkm << 1)
                | (wkl >> 63);
            wkl <<= 1;
            fl->ms_fract = 0;
            if (((S64)wkm) >= 0) {
                fl->ls_fract = 1;
                sub_U128(wkm, wkl, div_fl->ms_fract, div_fl->ls_fract);
            } else {
                fl->ls_fract = 0;
                add_U128(wkm, wkl, div_fl->ms_fract, div_fl->ls_fract);
            }

            /* the middle binary digits */
            i = 111;
            while (i--) {
                wkm = (wkm << 1)
                    | (wkl >> 63);
                wkl <<= 1;

                fl->ms_fract = (fl->ms_fract << 1)
                             | (fl->ls_fract >> 63);
                fl->ls_fract <<= 1;
                if (((S64)wkm) >= 0) {
                    fl->ls_fract |= 1;
                    sub_U128(wkm, wkl, div_fl->ms_fract, div_fl->ls_fract);
                } else {
                    add_U128(wkm, wkl, div_fl->ms_fract, div_fl->ls_fract);
                }
            }

            /* the last binary digit */
            fl->ms_fract = (fl->ms_fract << 1)
                         | (fl->ls_fract >> 63);
            fl->ls_fract <<= 1;
            if (((S64)wkm) >= 0) {
                fl->ls_fract |= 1;
            }

            /* determine sign */
            fl->sign = (fl->sign == div_fl->sign) ? POS : NEG;

            /* handle overflow and underflow */
            return( over_under_flow_ef(fl, regs) );
        } else {
            /* fraction of dividend 0, set true 0 */

            fl->ms_fract = 0;
            fl->ls_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    } else {
        /* divisor 0 */

        ARCH_DEP(program_interrupt) (regs, PGM_FLOATING_POINT_DIVIDE_EXCEPTION);
    }
    return(0);

} /* end function div_ef */


#if defined (FEATURE_SQUARE_ROOT)
/*-------------------------------------------------------------------*/
/* Square root of fraction                                           */
/* This routine uses the Newton-Iteration-Method                     */
/* The iteration is started with a table look up                     */
/*                                                                   */
/* Input:                                                            */
/*      a       short fraction expanded to U64                       */
/* Value:                                                            */
/*              square root as U32                                   */
/*-------------------------------------------------------------------*/
static U32 square_root_fraction( U64 a )
{
U32     xi;
U32     xj;
static const unsigned short sqtab[] = {
/* 0 */         0,
/* 1 */         304,
/* 2 */         401,
/* 3 */         476,
/* 4 */         541,
/* 5 */         599,
/* 6 */         652,
/* 7 */         700,
/* 8 */         746,
/* 9 */         788,
/* 10 */        829,
/* 11 */        868,
/* 12 */        905,
/* 13 */        940,
/* 14 */        975,
/* 15 */        1008,
/* 16 */        1040,
/* 17 */        1071,
/* 18 */        1101,
/* 19 */        1130,
/* 20 */        1159,
/* 21 */        1187,
/* 22 */        1214,
/* 23 */        1241,
/* 24 */        1267,
/* 25 */        1293,
/* 26 */        1318,
/* 27 */        1342,
/* 28 */        1367,
/* 29 */        1390,
/* 30 */        1414,
/* 31 */        1437,
/* 32 */        1459,
/* 33 */        1482,
/* 34 */        1504,
/* 35 */        1525,
/* 36 */        1547,
/* 37 */        1568,
/* 38 */        1588,
/* 39 */        1609,
/* 40 */        1629,
/* 41 */        1649,
/* 42 */        1669,
/* 43 */        1688,
/* 44 */        1708,
/* 45 */        1727,
/* 46 */        1746,
/* 47 */        1764,
/* 48 */        1783,
/* 49 */        1801,
/* 50 */        1819,
/* 51 */        1837,
/* 52 */        1855,
/* 53 */        1872,
/* 54 */        1890,
/* 55 */        1907,
/* 56 */        1924,
/* 57 */        1941,
/* 58 */        1958,
/* 59 */        1975,
/* 60 */        1991,
/* 61 */        2008,
/* 62 */        2024,
/* 63 */        2040,
/* 64 */        2056,
/* 65 */        2072,
/* 66 */        2088,
/* 67 */        2103,
/* 68 */        2119,
/* 69 */        2134,
/* 70 */        2149,
/* 71 */        2165,
/* 72 */        2180,
/* 73 */        2195,
/* 74 */        2210,
/* 75 */        2224,
/* 76 */        2239,
/* 77 */        2254,
/* 78 */        2268,
/* 79 */        2283,
/* 80 */        2297,
/* 81 */        2311,
/* 82 */        2325,
/* 83 */        2339,
/* 84 */        2353,
/* 85 */        2367,
/* 86 */        2381,
/* 87 */        2395,
/* 88 */        2408,
/* 89 */        2422,
/* 90 */        2435,
/* 91 */        2449,
/* 92 */        2462,
/* 93 */        2475,
/* 94 */        2489,
/* 95 */        2502,
/* 96 */        2515,
/* 97 */        2528,
/* 98 */        2541,
/* 99 */        2554,
/* 100 */       2566,
/* 101 */       2579,
/* 102 */       2592,
/* 103 */       2604,
/* 104 */       2617,
/* 105 */       2629,
/* 106 */       2642,
/* 107 */       2654,
/* 108 */       2667,
/* 109 */       2679,
/* 110 */       2691,
/* 111 */       2703,
/* 112 */       2715,
/* 113 */       2727,
/* 114 */       2739,
/* 115 */       2751,
/* 116 */       2763,
/* 117 */       2775,
/* 118 */       2787,
/* 119 */       2798,
/* 120 */       2810,
/* 121 */       2822,
/* 122 */       2833,
/* 123 */       2845,
/* 124 */       2856,
/* 125 */       2868,
/* 126 */       2879,
/* 127 */       2891,
/* 128 */       2902,
/* 129 */       2913,
/* 130 */       2924,
/* 131 */       2936,
/* 132 */       2947,
/* 133 */       2958,
/* 134 */       2969,
/* 135 */       2980,
/* 136 */       2991,
/* 137 */       3002,
/* 138 */       3013,
/* 139 */       3024,
/* 140 */       3034,
/* 141 */       3045,
/* 142 */       3056,
/* 143 */       3067,
/* 144 */       3077,
/* 145 */       3088,
/* 146 */       3099,
/* 147 */       3109,
/* 148 */       3120,
/* 149 */       3130,
/* 150 */       3141,
/* 151 */       3151,
/* 152 */       3161,
/* 153 */       3172,
/* 154 */       3182,
/* 155 */       3192,
/* 156 */       3203,
/* 157 */       3213,
/* 158 */       3223,
/* 159 */       3233,
/* 160 */       3243,
/* 161 */       3253,
/* 162 */       3263,
/* 163 */       3273,
/* 164 */       3283,
/* 165 */       3293,
/* 166 */       3303,
/* 167 */       3313,
/* 168 */       3323,
/* 169 */       3333,
/* 170 */       3343,
/* 171 */       3353,
/* 172 */       3362,
/* 173 */       3372,
/* 174 */       3382,
/* 175 */       3391,
/* 176 */       3401,
/* 177 */       3411,
/* 178 */       3420,
/* 179 */       3430,
/* 180 */       3439,
/* 181 */       3449,
/* 182 */       3458,
/* 183 */       3468,
/* 184 */       3477,
/* 185 */       3487,
/* 186 */       3496,
/* 187 */       3505,
/* 188 */       3515,
/* 189 */       3524,
/* 190 */       3533,
/* 191 */       3543,
/* 192 */       3552,
/* 193 */       3561,
/* 194 */       3570,
/* 195 */       3579,
/* 196 */       3589,
/* 197 */       3598,
/* 198 */       3607,
/* 199 */       3616,
/* 200 */       3625,
/* 201 */       3634,
/* 202 */       3643,
/* 203 */       3652,
/* 204 */       3661,
/* 205 */       3670,
/* 206 */       3679,
/* 207 */       3688,
/* 208 */       3697,
/* 209 */       3705,
/* 210 */       3714,
/* 211 */       3723,
/* 212 */       3732,
/* 213 */       3741,
/* 214 */       3749,
/* 215 */       3758,
/* 216 */       3767,
/* 217 */       3775,
/* 218 */       3784,
/* 219 */       3793,
/* 220 */       3801,
/* 221 */       3810,
/* 222 */       3819,
/* 223 */       3827,
/* 224 */       3836,
/* 225 */       3844,
/* 226 */       3853,
/* 227 */       3861,
/* 228 */       3870,
/* 229 */       3878,
/* 230 */       3887,
/* 231 */       3895,
/* 232 */       3903,
/* 233 */       3912,
/* 234 */       3920,
/* 235 */       3929,
/* 236 */       3937,
/* 237 */       3945,
/* 238 */       3954,
/* 239 */       3962,
/* 240 */       3970,
/* 241 */       3978,
/* 242 */       3987,
/* 243 */       3995,
/* 244 */       4003,
/* 245 */       4011,
/* 246 */       4019,
/* 247 */       4027,
/* 248 */       4036,
/* 249 */       4044,
/* 250 */       4052,
/* 251 */       4060,
/* 252 */       4068,
/* 253 */       4076,
/* 254 */       4084,
/* 255 */       4092 };

    /* initial table look up */
    xi = ((U32) sqtab[a >> 48]) << 16;

    if (xi == 0)
        return(xi);

    /* iterate */
    /* exit iteration when xi, xj equal or differ by 1 */
    for (;;) {
        xj = (((U32)(a / xi)) + xi) >> 1;
        if (0
            || xj == xi
            || xj == (xi+1)
            || xj == (xi-1)
        )
            break;
        xi = xj;
    }

    return(xj);

} /* end function square_root_fraction */


/*-------------------------------------------------------------------*/
/* Divide 128 bit integer by 64 bit integer                          */
/* The result is returned as 64 bit integer                          */
/*                                                                   */
/* Input:                                                            */
/*      msa     most significant 64 bit of dividend                  */
/*      lsa     least significant 64 bit of dividend                 */
/*      div     divisor                                              */
/* Value:                                                            */
/*              quotient                                             */
/*-------------------------------------------------------------------*/
static U64 div_U128( U64 msa, U64 lsa, U64 div )
{
U64     q;
int     i;

    /* the first binary digit */
    msa -= div;
    shift_left_U128(msa, lsa);

    if (((S64)msa) >= 0) {
        q = 1;
        msa -= div;
    } else {
        q = 0;
        msa += div;
    }

    /* the middle binary digits */
    i = 63;
    while (i--) {
        shift_left_U128(msa, lsa);

        q <<= 1;
        if (((S64)msa) >= 0) {
            q |= 1;
            msa -= div;
        } else {
            msa += div;
        }
    }

    /* the last binary digit */
    q <<= 1;
    if (((S64)msa) >= 0) {
        q |= 1;
    }

    return(q);

} /* end function div_U128 */


/*-------------------------------------------------------------------*/
/* Square root short float                                           */
/*                                                                   */
/* Input:                                                            */
/*      sq_fl   Result short float                                   */
/*      fl      Input short float                                    */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static void sq_sf( SHORT_FLOAT *sq_fl, SHORT_FLOAT *fl, REGS *regs )
{
U64     a;
U32     x;

    if (fl->short_fract) {
        if (fl->sign) {
            /* less than zero */
            sq_fl->short_fract = 0;
            sq_fl->expo = 0;

            ARCH_DEP(program_interrupt) (regs, PGM_SQUARE_ROOT_EXCEPTION);
        } else {
            /* normalize operand */
            normal_sf(fl);

            if (fl->expo & 1) {
                /* odd */

                /* compute characteristic */
                sq_fl->expo = (fl->expo + 65) >> 1;

                /* with guard digit */
                a = (U64) fl->short_fract << 28;
            } else {
                /* even */

                /* compute characteristic */
                sq_fl->expo = (fl->expo + 64) >> 1;

                /* without guard digit */
                a = (U64) fl->short_fract << 32;
            }

            /* square root of fraction */
            /* common subroutine of all square root */
            x = square_root_fraction(a);

            /* round with guard digit */
            sq_fl->short_fract = (x + 8) >> 4;
        }
    } else {
        /* true zero */
        sq_fl->short_fract = 0;
        sq_fl->expo = 0;
    }
    /* all results positive */
    sq_fl->sign = POS;

} /* end function sq_sf */


/*-------------------------------------------------------------------*/
/* Square root long float                                            */
/*                                                                   */
/* Input:                                                            */
/*      sq_fl   Result long float                                    */
/*      fl      Input long float                                     */
/*      regs    CPU register context                                 */
/*-------------------------------------------------------------------*/
static void sq_lf( LONG_FLOAT *sq_fl, LONG_FLOAT *fl, REGS *regs )
{
U64     msa, lsa;
U64     xi;
U64     xj;

    if (fl->long_fract) {
        if (fl->sign) {
            /* less than zero */

            ARCH_DEP(program_interrupt) (regs, PGM_SQUARE_ROOT_EXCEPTION);
        } else {
            /* normalize operand */
            normal_lf(fl);

            if (fl->expo & 1) {
                /* odd */

                /* compute characteristic */
                sq_fl->expo = (fl->expo + 65) >> 1;

                /* with guard digit */
                msa = fl->long_fract >> 4;
                lsa = fl->long_fract << 60;
            } else {
                /* even */

                /* compute characteristic */
                sq_fl->expo = (fl->expo + 64) >> 1;

                /* without guard digit */
                msa = fl->long_fract;
                lsa = 0;
            }

            /* square root of fraction low precision */
            /* common subroutine of all square root */
            xi = ((U64) (square_root_fraction(msa & 0xFFFFFFFFFFFFFFFEULL)) << 32)
               | 0x80000000UL;

            /* continue iteration for high precision */
            for (;;) {
                xj = (div_U128(msa, lsa, xi) + xi) >> 1;

                if (xj == xi) {
                    break;
                }
                xi = xj;
            }

            /* round with guard digit */
            sq_fl->long_fract = (xi + 8) >> 4;
        }
    } else {
        /* true zero */
        sq_fl->long_fract = 0;
        sq_fl->expo = 0;
    }
    /* all results positive */
    sq_fl->sign = POS;

} /* end function sq_lf */
#endif /* FEATURE_SQUARE_ROOT */


#if defined (FEATURE_HFP_EXTENSIONS)
/*-------------------------------------------------------------------*/
/* Divide 256 bit integer by 128 bit integer                         */
/* The result is returned as 128 bit integer                         */
/*                                                                   */
/* Input:                                                            */
/*      mmsa    most significant 64 bit of dividend                  */
/*      msa     more significant 64 bit of dividend                  */
/*      lsa     less significant 64 bit of dividend                  */
/*      llsa    least significant 64 bit of dividend                 */
/*      msd     most significant 64 bit of divisor                   */
/*      lsd     least significant 64 bit of divisor                  */
/*      msq     most significant 64 bit of quotient                  */
/*      lsq     least significant 64 bit of quotient                 */
/*-------------------------------------------------------------------*/
static void div_U256( U64 mmsa, U64 msa, U64 lsa, U64 llsa, U64 msd,
    U64 lsd, U64 *msq, U64 *lsq )
{
int     i;

    /* the first binary digit */
    sub_U128(mmsa, msa, msd, lsd);
    shift_left_U256(mmsa, msa, lsa, llsa);

    *msq = 0;
    if (((S64)mmsa) >= 0) {
        *lsq = 1;
        sub_U128(mmsa, msa, msd, lsd);
    } else {
        *lsq = 0;
        add_U128(mmsa, msa, msd, lsd);
    }

    /* the middle binary digits */
    i = 127;
    while (i--) {
        shift_left_U256(mmsa, msa, lsa, llsa);

        shift_left_U128(*msq, *lsq);
        if (((S64)mmsa) >= 0) {
            *lsq |= 1;
            sub_U128(mmsa, msa, msd, lsd);
        } else {
            add_U128(mmsa, msa, msd, lsd);
        }
    }

    /* the last binary digit */
    shift_left_U128(*msq, *lsq);
    if (((S64)mmsa) >= 0) {
        *lsq |= 1;
    }

} /* end function div_U256 */
#endif /* FEATURE_HFP_EXTENSIONS */

#if defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY)
/*-------------------------------------------------------------------*/
/* Multiply long float to extended float unnormalized                */
/*                                                                   */
/* Input:                                                            */
/*      fl         Multiplicand long float                           */
/*      mul_fl     Multiplicator long float                          */
/*      result_fl  Intermediate extended float result                */
/*      regs       CPU register context                              */
/* Value:                                                            */
/*      none                                                         */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(mul_lf_to_ef_unnorm)(
                     LONG_FLOAT *fl, LONG_FLOAT *mul_fl,
                     EXTENDED_FLOAT *result_fl )
{
U64     wk;

    /* multiply fracts by sum of partial multiplications */
    wk = (fl->long_fract & 0x00000000FFFFFFFFULL) * (mul_fl->long_fract & 0x00000000FFFFFFFFULL);
    result_fl->ls_fract = wk & 0x00000000FFFFFFFFULL;

    wk >>= 32;
    wk += ((fl->long_fract & 0x00000000FFFFFFFFULL) * (mul_fl->long_fract >> 32));
    wk += ((fl->long_fract >> 32) * (mul_fl->long_fract & 0x00000000FFFFFFFFULL));
    result_fl->ls_fract |= wk << 32;

    result_fl->ms_fract = (wk >> 32) + ((fl->long_fract >> 32) * (mul_fl->long_fract >> 32));

    /* compute expo */
    result_fl->expo = fl->expo + mul_fl->expo - 64;

    /* determine sign */
    result_fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

} /* end function mul_lf_to_ef_unnorm */


/*-------------------------------------------------------------------*/
/* Add extended float unnormalized                                   */
/* Note: This addition is specific to MULTIPLY AND ADD UNNORMALIZED  */
/*       set of instructions                                         */
/*                                                                   */
/* Input:                                                            */
/*      prod_fl    Extended Intermediate product                     */
/*      add_fl     Extended Addend                                   */
/*      result_fl  Extended Intermediate result                      */
/* Value:                                                            */
/*      none                                                         */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(add_ef_unnorm)(
                     EXTENDED_FLOAT *prod_fl, EXTENDED_FLOAT *add_fl,
                     EXTENDED_FLOAT *result_fl )
{
int  ldigits = 0;                  /* or'd left digits shifted      */
int  rdigits = 0;                  /* or'd right digits shifted     */
int  xdigit;                       /* digit lost by addend shifting */

    /* Note: In EXTENDED_FLOAT, ms_fract and ls_fract taken together*/
    /* constitute a U128 value.                                     */

    /* Convert separate high/low fractions to contiguous U128 */
#if FLOAT_DEBUG
    LOGMSG( "Prod Frac: %16.16llX %16.16llX\n",
               prod_fl->ms_fract, prod_fl->ls_fract );

    LOGMSG( "Adnd Frac: %16.16llX %16.16llX\n",
               add_fl->ms_fract, add_fl->ls_fract );
#endif

    result_fl->ms_fract = 0;
    result_fl->ls_fract = 0;

    /* Default result to product sign in case addend expo == prod expo */
    result_fl->sign = prod_fl->sign;

    /* Step one - shift addend to match product's characteristic */
    if (add_fl->expo < prod_fl->expo)
    {
        while(add_fl->expo != prod_fl->expo)
        {
            if ((!add_fl->ms_fract) && (!add_fl->ls_fract))
            {  /* If both the high and low parts of the fraction are zero */
               /* we don't need to shift any more digits.                 */
               /* Just force the exponents equal and quit                 */
               add_fl->expo = prod_fl->expo;
               break;
            }
            /* shift addend fraction right until characteristics are equal */
            shift_right4_U128(add_fl->ms_fract, add_fl->ls_fract, xdigit);
            rdigits |= xdigit;
            add_fl->expo += 1;
        }
    }
    else if(add_fl->expo > prod_fl->expo)
    {
        while(add_fl->expo != prod_fl->expo)
        {
            if ((!add_fl->ms_fract) && (!add_fl->ls_fract))
            {  /* If both the high and low parts of the fraction are zero */
               /* we don't need to shift any more digits.                 */
               /* Just force the exponents equal and quit                 */
               add_fl->expo = prod_fl->expo;
               break;
            }

            /* shift addend fraction right until characteristics are equal */
            shift_left4_U128(add_fl->ms_fract, add_fl->ls_fract, xdigit);
            ldigits |= xdigit;
            add_fl->expo -= 1;
        }
    }
#if FLOAT_DEBUG
    LOGMSG( "Shft Frac: %16.16llX %16.16llX\n",
               add_fl->ms_fract, add_fl->ls_fract );
#endif

    /* Step 2 - Do algebraic addition of aligned fractions */
    if (add_fl->sign == prod_fl->sign)
    {   /* signs equal, so just add fractions */

        result_fl->sign = prod_fl->sign;
        result_fl->ms_fract = prod_fl->ms_fract;
        result_fl->ls_fract = prod_fl->ls_fract;

        add_U128(result_fl->ms_fract, result_fl->ls_fract,
                 add_fl->ms_fract, add_fl->ls_fract);

        /* Recognize any overflow of left hand digits */
        ldigits |= result_fl->ms_fract >> 48;

        /* Remove them from the result */
        result_fl->ms_fract &= 0x0000FFFFFFFFFFFFULL;

        /* result sign already set to product sign */
    }
    else
    {   /* signs unequal, subtract the larger fraction from the smaller */
        /* result has sign of the larger fraction                       */

        if ( (prod_fl->ms_fract > add_fl->ms_fract)
          || ((prod_fl->ms_fract == add_fl->ms_fract) &&
              (prod_fl->ls_fract >= add_fl->ls_fract)) )
        /* product fraction larger than or equal to addend fraction */

        {  /* subtract addend fraction from product fraction */
           /* result has sign of product                     */

           result_fl->ms_fract = prod_fl->ms_fract;
           result_fl->ls_fract = prod_fl->ls_fract;

           if (rdigits)
           {   /* If any right shifted addend digits, then we need to    */
               /* borrow from the product fraction to reflect the shifted*/
               /* digits participation in the result                     */
               sub_U128(result_fl->ms_fract, result_fl->ls_fract,
                        (U64)0, (U64)1);
#if FLOAT_DEBUG
               LOGMSG( "Barw Frac: %16.16llX %16.16llX\n",
                      result_fl->ms_fract, result_fl->ls_fract );
#endif
               /* Due to participation of right shifted digits           */
               /* result fraction NOT zero, so true zero will not result */
               /* hence, force sign will not to change                   */
               ldigits = 1;
           }

           sub_U128(result_fl->ms_fract, result_fl->ls_fract,
                    add_fl->ms_fract, add_fl->ls_fract);
#if FLOAT_DEBUG
           LOGMSG( "P-A  Frac: %16.16llX %16.16llX\n",
                      result_fl->ms_fract, result_fl->ls_fract );
#endif
           /* result sign already set to product sign above as default */
        }
        else
        /* addend fraction larger than product fraction */

        {  /* subtract product fraction from addend fraction */
           /* result has sign of addend                      */
           result_fl->ms_fract = add_fl->ms_fract;
           result_fl->ls_fract = add_fl->ls_fract;

           sub_U128(result_fl->ms_fract, result_fl->ls_fract,
                    prod_fl->ms_fract, prod_fl->ls_fract);
#if FLOAT_DEBUG
           LOGMSG( "A-P  Frac: %16.16llX %16.16llX\n",
                      result_fl->ms_fract, result_fl->ls_fract );
#endif
           result_fl->sign = add_fl->sign;
        }
    }
#if FLOAT_DEBUG
    LOGMSG( "Resl Frac: %16.16llX %16.16llX\n",
               result_fl->ms_fract, result_fl->ls_fract );
#endif

    /* result exponent always the same as the product */
    result_fl->expo = prod_fl->expo;

    /* Step 3 - If fraction is TRULY zero, sign is set to positive */
    if ( (!result_fl->ms_fract) && (!result_fl->ls_fract) &&
          (!ldigits) && (!rdigits) )
    {
        result_fl->sign = POS;
    }

} /* end ARCH_DEP(add_ef_unnorm) */

#endif /* defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY) */


/*-------------------------------------------------------------------*/
/* Extern functions                                                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* 20   LPDR  - Load Positive Floating Point Long Register      [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_positive_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    /* Copy register contents, clear the sign bit */
    regs->fpr[i1] = regs->fpr[i2] & 0x7FFFFFFF;
    regs->fpr[i1+1] = regs->fpr[i2+1];

    /* Set condition code */
    regs->psw.cc = ((regs->fpr[i1] & 0x00FFFFFF)
                 || regs->fpr[i1+1]) ? 2 : 0;
}


/*-------------------------------------------------------------------*/
/* 21   LNDR  - Load Negative Floating Point Long Register      [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_negative_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    /* Copy register contents, set the sign bit */
    regs->fpr[i1] = regs->fpr[i2]
                  | 0x80000000;
    regs->fpr[i1+1] = regs->fpr[i2+1];

    /* Set condition code */
    regs->psw.cc = ((regs->fpr[i1] & 0x00FFFFFF)
                 || regs->fpr[i1+1]) ? 1 : 0;
}


/*-------------------------------------------------------------------*/
/* 22   LTDR  - Load and Test Floating Point Long Register      [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    /* Copy register contents */
    regs->fpr[i1] = regs->fpr[i2];
    regs->fpr[i1+1] = regs->fpr[i2+1];

    /* Set condition code */
    if ((regs->fpr[i1] & 0x00FFFFFF)
    || regs->fpr[i1+1]) {
        regs->psw.cc = (regs->fpr[i1] & 0x80000000) ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* 23   LCDR  - Load Complement Floating Point Long Register    [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_complement_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    /* Copy register contents, invert sign bit */
    regs->fpr[i1] = regs->fpr[i2] ^ 0x80000000;
    regs->fpr[i1+1] = regs->fpr[i2+1];

    /* Set condition code */
    if ((regs->fpr[i1] & 0x00FFFFFF)
    || regs->fpr[i1+1]) {
        regs->psw.cc = (regs->fpr[i1] & 0x80000000) ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* 24   HDR   - Halve Floating Point Long Register              [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(halve_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
LONG_FLOAT fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Get register content */
    get_lf(&fl, regs->fpr + FPR2I(r2));

    /* Halve the value */
    if (fl.long_fract & 0x00E0000000000000ULL) {
        fl.long_fract >>= 1;
        pgm_check = 0;
    } else {
        fl.long_fract <<= 3;
        (fl.expo)--;
        normal_lf(&fl);
        pgm_check = underflow_lf(&fl, regs);
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + FPR2I(r1));

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 25   LDXR  - Load Rounded Floating Point Long Register       [RR] */
/*              Older mnemonic of this instruction is LRDR           */
/*-------------------------------------------------------------------*/
DEF_INST(load_rounded_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
LONG_FLOAT fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);
    HFPODD_CHECK(r2, regs);

    /* Get register content */
    get_lf(&fl, regs->fpr + FPR2I(r2));

    /* Rounding */
    fl.long_fract += ((regs->fpr[FPR2I(r2 + 2)] >> 23) & 1);

    /* Handle overflow */
    if (fl.long_fract & 0x0F00000000000000ULL) {
        fl.long_fract >>= 4;
        (fl.expo)++;
        pgm_check = overflow_lf(&fl, regs);
    } else {
        pgm_check = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + FPR2I(r1));

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 26   MXR   - Multiply Floating Point Extended Register       [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
EXTENDED_FLOAT fl;
EXTENDED_FLOAT mul_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_ef(&fl, regs->fpr + i1);
    get_ef(&mul_fl, regs->fpr + FPR2I(r2));

    /* multiply extended */
    pgm_check = mul_ef(&fl, &mul_fl, regs);

    /* Back to register */
    store_ef(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 27   MXDR  - Multiply Floating Point Long to Extended Reg.   [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_long_to_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
LONG_FLOAT fl;
LONG_FLOAT mul_fl;
EXTENDED_FLOAT result_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);

    HFPREG_CHECK(r2, regs);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    get_lf(&mul_fl, regs->fpr + FPR2I(r2));

    /* multiply long to extended */
    pgm_check = mul_lf_to_ef(&fl, &mul_fl, &result_fl, regs);

    /* Back to register */
    store_ef(&result_fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 28   LDR   - Load Floating Point Long Register               [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RR(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    /* Copy register contents */
    regs->fpr[i1] = regs->fpr[i2];
    regs->fpr[i1+1] = regs->fpr[i2+1];
}


/*-------------------------------------------------------------------*/
/* 29   CDR   - Compare Floating Point Long Register            [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
LONG_FLOAT fl;
LONG_FLOAT cmp_fl;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Get the operands */
    get_lf(&fl, regs->fpr + FPR2I(r1));
    get_lf(&cmp_fl, regs->fpr + FPR2I(r2));

    /* Compare long */
    cmp_lf(&fl, &cmp_fl, regs);
}


/*-------------------------------------------------------------------*/
/* 2A   ADR   - Add Floating Point Long Register                [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
LONG_FLOAT fl;
LONG_FLOAT add_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    get_lf(&add_fl, regs->fpr + FPR2I(r2));

    /* Add long with normalization */
    pgm_check = add_lf(&fl, &add_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_float_long_reg) */


/*-------------------------------------------------------------------*/
/* 2B   SDR   - Subtract Floating Point Long Register           [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
LONG_FLOAT fl;
LONG_FLOAT sub_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    get_lf(&sub_fl, regs->fpr + FPR2I(r2));

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add long with normalization */
    pgm_check = add_lf(&fl, &sub_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_float_long_reg) */


/*-------------------------------------------------------------------*/
/* 2C   MDR   - Multiply Floating Point Long Register           [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
LONG_FLOAT fl;
LONG_FLOAT mul_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    get_lf(&mul_fl, regs->fpr + FPR2I(r2));

    /* multiply long */
    pgm_check = mul_lf(&fl, &mul_fl, OVUNF, regs);

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_float_long_reg) */


/*-------------------------------------------------------------------*/
/* 2D   DDR   - Divide Floating Point Long Register             [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
LONG_FLOAT fl;
LONG_FLOAT div_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    get_lf(&div_fl, regs->fpr + FPR2I(r2));

    /* divide long */
    pgm_check = div_lf(&fl, &div_fl, regs);

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 2E   AWR   - Add Unnormalized Floating Point Long Register   [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_unnormal_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
LONG_FLOAT fl;
LONG_FLOAT add_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    get_lf(&add_fl, regs->fpr + FPR2I(r2));

    /* Add long without normalization */
    pgm_check = add_lf(&fl, &add_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_unnormal_float_long_reg) */


/*-------------------------------------------------------------------*/
/* 2F   SWR   - Subtract Unnormalized Floating Point Long Reg.  [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_unnormal_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
LONG_FLOAT fl;
LONG_FLOAT sub_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    get_lf(&sub_fl, regs->fpr + FPR2I(r2));

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add long without normalization */
    pgm_check = add_lf(&fl, &sub_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_unnormal_float_long_reg) */


/*-------------------------------------------------------------------*/
/* 30   LPER  - Load Positive Floating Point Short Register     [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_positive_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Copy register contents, clear sign bit */
    regs->fpr[i1] = regs->fpr[FPR2I(r2)] & 0x7FFFFFFF;

    /* Set condition code */
    regs->psw.cc = (regs->fpr[i1] & 0x00FFFFFF) ? 2 : 0;
}


/*-------------------------------------------------------------------*/
/* 31   LNER  - Load Negative Floating Point Short Register     [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_negative_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Copy register contents, set sign bit */
    regs->fpr[i1] = regs->fpr[FPR2I(r2)]
                  | 0x80000000;

    /* Set condition code */
    regs->psw.cc = (regs->fpr[i1] & 0x00FFFFFF) ? 1 : 0;
}


/*-------------------------------------------------------------------*/
/* 32   LTER  - Load and Test Floating Point Short Register     [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Copy register contents */
    regs->fpr[i1] = regs->fpr[FPR2I(r2)];

    /* Set condition code */
    if (regs->fpr[i1] & 0x00FFFFFF) {
        regs->psw.cc = (regs->fpr[i1] & 0x80000000) ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* 33   LCER  - Load Complement Floating Point Short Register   [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_complement_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Copy register contents, invert sign bit */
    regs->fpr[i1] = regs->fpr[FPR2I(r2)] ^ 0x80000000;

    /* Set condition code */
    if (regs->fpr[i1] & 0x00FFFFFF) {
        regs->psw.cc = (regs->fpr[i1] & 0x80000000) ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* 34   HER   - Halve Floating Point Short Register             [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(halve_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
SHORT_FLOAT fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Get register content */
    get_sf(&fl, regs->fpr + FPR2I(r2));

    /* Halve the value */
    if (fl.short_fract & 0x00E00000) {
        fl.short_fract >>= 1;
        pgm_check = 0;
    } else {
        fl.short_fract <<= 3;
        (fl.expo)--;
        normal_sf(&fl);
        pgm_check = underflow_sf(&fl, regs);
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + FPR2I(r1));

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 35   LEDR  - Load Rounded Floating Point Short Register      [RR] */
/*              Older mnemonic of this instruction is LRER           */
/*-------------------------------------------------------------------*/
DEF_INST(load_rounded_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
LONG_FLOAT from_fl;
SHORT_FLOAT to_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Get register content */
    get_lf(&from_fl, regs->fpr + FPR2I(r2));

    /* Rounding */
    to_fl.short_fract = (from_fl.long_fract + 0x0000000080000000ULL) >> 32;
    to_fl.sign = from_fl.sign;
    to_fl.expo = from_fl.expo;

    /* Handle overflow */
    if (to_fl.short_fract & 0x0F000000) {
        to_fl.short_fract >>= 4;
        (to_fl.expo)++;
        pgm_check = overflow_sf(&to_fl, regs);
    } else {
        pgm_check = 0;
    }

    /* To register */
    store_sf(&to_fl, regs->fpr + FPR2I(r1));

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 36   AXR   - Add Floating Point Extended Register            [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
EXTENDED_FLOAT fl;
EXTENDED_FLOAT add_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_ef(&fl, regs->fpr + i1);
    get_ef(&add_fl, regs->fpr + FPR2I(r2));

    /* Add extended */
    pgm_check = add_ef(&fl, &add_fl, regs->fpr + i1, regs);

    /* Set condition code */
    if (fl.ms_fract
    || fl.ls_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 37   SXR   - Subtract Floating Point Extended Register       [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
EXTENDED_FLOAT fl;
EXTENDED_FLOAT sub_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_ef(&fl, regs->fpr + i1);
    get_ef(&sub_fl, regs->fpr + FPR2I(r2));

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add extended */
    pgm_check = add_ef(&fl, &sub_fl, regs->fpr + i1, regs);

    /* Set condition code */
    if (fl.ms_fract
    || fl.ls_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 38   LER   - Load Floating Point Short Register              [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Copy register content */
    regs->fpr[FPR2I(r1)] = regs->fpr[FPR2I(r2)];
}


/*-------------------------------------------------------------------*/
/* 39   CER   - Compare Floating Point Short Register           [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
SHORT_FLOAT fl;
SHORT_FLOAT cmp_fl;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Get the operands */
    get_sf(&fl, regs->fpr + FPR2I(r1));
    get_sf(&cmp_fl, regs->fpr + FPR2I(r2));

    /* Compare short */
    cmp_sf(&fl, &cmp_fl, regs);
}


/*-------------------------------------------------------------------*/
/* 3A   AER   - Add Floating Point Short Register               [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
SHORT_FLOAT fl;
SHORT_FLOAT add_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    get_sf(&add_fl, regs->fpr + FPR2I(r2));

    /* Add short with normalization */
    pgm_check = add_sf(&fl, &add_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_float_short_reg) */


/*-------------------------------------------------------------------*/
/* 3B   SER   - Subtract Floating Point Short Register          [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
SHORT_FLOAT fl;
SHORT_FLOAT sub_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    get_sf(&sub_fl, regs->fpr + FPR2I(r2));

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Subtract short with normalization */
    pgm_check = add_sf(&fl, &sub_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_float_short_reg) */


/*-------------------------------------------------------------------*/
/* 3C   MDER  - Multiply Short to Long Floating Point Register  [RR] */
/*              Older mnemonic of this instruction is MER            */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_short_to_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
SHORT_FLOAT fl;
SHORT_FLOAT mul_fl;
LONG_FLOAT result_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    get_sf(&mul_fl, regs->fpr + FPR2I(r2));

    /* multiply short to long */
    pgm_check = mul_sf_to_lf(&fl, &mul_fl, &result_fl, regs);

    /* Back to register */
    store_lf(&result_fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 3D   DER   - Divide Floating Point Short Register            [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
SHORT_FLOAT fl;
SHORT_FLOAT div_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    get_sf(&div_fl, regs->fpr + FPR2I(r2));

    /* divide short */
    pgm_check = div_sf(&fl, &div_fl, regs);

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 3E   AUR   - Add Unnormalized Floating Point Short Register  [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_unnormal_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
SHORT_FLOAT fl;
SHORT_FLOAT add_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    get_sf(&add_fl, regs->fpr + FPR2I(r2));

    /* Add short without normalization */
    pgm_check = add_sf(&fl, &add_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_unnormal_float_short_reg) */


/*-------------------------------------------------------------------*/
/* 3F   SUR   - Subtract Unnormalized Floating Point Short Reg. [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_unnormal_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
SHORT_FLOAT fl;
SHORT_FLOAT sub_fl;
int     pgm_check;

    RR(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    get_sf(&sub_fl, regs->fpr + FPR2I(r2));

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add short without normalization */
    pgm_check = add_sf(&fl, &sub_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_unnormal_float_short_reg) */


/*-------------------------------------------------------------------*/
/* 60   STD   - Store Floating Point Long                     [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(store_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Double word workarea      */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Store register contents at operand address */
    dreg = ((U64)regs->fpr[i1] << 32)
         | regs->fpr[i1+1];
    ARCH_DEP(vstore8) (dreg, effective_addr2, b2, regs);
}


/*-------------------------------------------------------------------*/
/* 67   MXD   - Multiply Floating Point Long to Extended      [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_long_to_ext)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT mul_fl;
EXTENDED_FLOAT result_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    vfetch_lf(&mul_fl, effective_addr2, b2, regs );

    /* multiply long to extended */
    pgm_check = mul_lf_to_ef(&fl, &mul_fl, &result_fl, regs);

    /* Back to register */
    store_ef(&result_fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 68   LD    - Load Floating Point Long                      [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Double word workarea      */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Fetch value from operand address */
    dreg = ARCH_DEP(vfetch8) (effective_addr2, b2, regs);

    /* Update register contents */
    regs->fpr[i1] = dreg >> 32;
    regs->fpr[i1+1] = dreg & 0xffffffff;
}


/*-------------------------------------------------------------------*/
/* 69   CD    - Compare Floating Point Long                   [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_float_long)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT cmp_fl;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Get the operands */
    get_lf(&fl, regs->fpr + FPR2I(r1));
    vfetch_lf(&cmp_fl, effective_addr2, b2, regs );

    /* Compare long */
    cmp_lf(&fl, &cmp_fl, regs);
}


/*-------------------------------------------------------------------*/
/* 6A   AD    - Add Floating Point Long                       [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT add_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    vfetch_lf(&add_fl, effective_addr2, b2, regs );

    /* Add long with normalization */
    pgm_check = add_lf(&fl, &add_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_float_long) */


/*-------------------------------------------------------------------*/
/* 6B   SD    - Subtract Floating Point Long                  [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT sub_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    vfetch_lf(&sub_fl, effective_addr2, b2, regs );

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add long with normalization */
    pgm_check = add_lf(&fl, &sub_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_float_long) */


/*-------------------------------------------------------------------*/
/* 6C   MD    - Multiply Floating Point Long                  [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT mul_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    vfetch_lf(&mul_fl, effective_addr2, b2, regs );

    /* multiply long */
    pgm_check = mul_lf(&fl, &mul_fl, OVUNF, regs);

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_float_long) */


/*-------------------------------------------------------------------*/
/* 6D   DD    - Divide Floating Point Long                    [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT div_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    vfetch_lf(&div_fl, effective_addr2, b2, regs );

    /* divide long */
    pgm_check = div_lf(&fl, &div_fl, regs);

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 6E   AW    - Add Unnormalized Floating Point Long          [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_unnormal_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT add_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    vfetch_lf(&add_fl, effective_addr2, b2, regs );

    /* Add long without normalization */
    pgm_check = add_lf(&fl, &add_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_unnormal_float_long) */


/*-------------------------------------------------------------------*/
/* 6F   SW    - Subtract Unnormalized Floating Point Long     [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_unnormal_float_long)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl;
LONG_FLOAT sub_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl, regs->fpr + i1);
    vfetch_lf(&sub_fl, effective_addr2, b2, regs );

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add long without normalization */
    pgm_check = add_lf(&fl, &sub_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.long_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_lf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_unnormal_float_long) */


/*-------------------------------------------------------------------*/
/* 70   STE   - Store Floating Point Short                    [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(store_float_short)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Store register contents at operand address */
    ARCH_DEP(vstore4) (regs->fpr[FPR2I(r1)], effective_addr2, b2, regs);
}


/*-------------------------------------------------------------------*/
/* 78   LE    - Load Floating Point Short                     [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_float_short)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Update first 32 bits of register from operand address */
    regs->fpr[FPR2I(r1)] = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);
}


/*-------------------------------------------------------------------*/
/* 79   CE    - Compare Floating Point Short                  [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_float_short)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT cmp_fl;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Get the operands */
    get_sf(&fl, regs->fpr + FPR2I(r1));
    vfetch_sf(&cmp_fl, effective_addr2, b2, regs );

    /* Compare long */
    cmp_sf(&fl, &cmp_fl, regs);
}


/*-------------------------------------------------------------------*/
/* 7A   AE    - Add Floating Point Short                      [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_float_short)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT add_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    vfetch_sf(&add_fl, effective_addr2, b2, regs );

    /* Add short with normalization */
    pgm_check = add_sf(&fl, &add_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_float_short) */


/*-------------------------------------------------------------------*/
/* 7B   SE    - Subtract Floating Point Short                 [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_float_short)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT sub_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    vfetch_sf(&sub_fl, effective_addr2, b2, regs );

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add short with normalization */
    pgm_check = add_sf(&fl, &sub_fl, NORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_float_short) */


/*-------------------------------------------------------------------*/
/* 7C   MDE   - Multiply Floating Point Short to Long         [RX-a] */
/*              Older mnemonic of this instruction is ME             */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_short_to_long)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT mul_fl;
LONG_FLOAT result_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    vfetch_sf(&mul_fl, effective_addr2, b2, regs );

    /* multiply short to long */
    pgm_check = mul_sf_to_lf(&fl, &mul_fl, &result_fl, regs);

    /* Back to register */
    store_lf(&result_fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 7D   DE    - Divide Floating Point Short                   [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_float_short)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT div_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    vfetch_sf(&div_fl, effective_addr2, b2, regs );

    /* divide short */
    pgm_check = div_sf(&fl, &div_fl, regs);

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* 7E   AU    - Add Unnormalized Floating Point Short         [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_unnormal_float_short)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT add_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    vfetch_sf(&add_fl, effective_addr2, b2, regs );

    /* Add short without normalization */
    pgm_check = add_sf(&fl, &add_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(add_unnormal_float_short) */


/*-------------------------------------------------------------------*/
/* 7F   SU    - Subtract Unnormalized Floating Point Short    [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_unnormal_float_short)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT sub_fl;
int     pgm_check;

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    vfetch_sf(&sub_fl, effective_addr2, b2, regs );

    /* Invert the sign of 2nd operand */
    sub_fl.sign = ! (sub_fl.sign);

    /* Add short without normalization */
    pgm_check = add_sf(&fl, &sub_fl, UNNORMAL, SIGEX, regs);

    /* Set condition code */
    if (fl.short_fract) {
        regs->psw.cc = fl.sign ? 1 : 2;
    } else {
        regs->psw.cc = 0;
    }

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(subtract_unnormal_float_short) */


/*-------------------------------------------------------------------*/
/* B22D DXR   - Divide Floating Point Extended Register        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
EXTENDED_FLOAT fl;
EXTENDED_FLOAT div_fl;
int     pgm_check;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_ef(&fl, regs->fpr + i1);
    get_ef(&div_fl, regs->fpr + FPR2I(r2));

    /* divide extended */
    pgm_check = div_ef(&fl, &div_fl, regs);

    /* Back to register */
    store_ef(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


#if defined (FEATURE_SQUARE_ROOT)
/*-------------------------------------------------------------------*/
/* B244 SQDR  - Square Root Floating Point Long Register       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(squareroot_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
LONG_FLOAT sq_fl = { 0, 0, 0 };
LONG_FLOAT fl;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Get the 2nd operand */
    get_lf(&fl, regs->fpr + FPR2I(r2));

    /* square root long */
    sq_lf(&sq_fl, &fl, regs);

    /* Back to register */
    store_lf(&sq_fl, regs->fpr + FPR2I(r1));
}


/*-------------------------------------------------------------------*/
/* B245 SQER  - Square Root Floating Point Short Register      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(squareroot_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
SHORT_FLOAT sq_fl;
SHORT_FLOAT fl;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    /* Get the 2nd operand */
    get_sf(&fl, regs->fpr + FPR2I(r2));

    /* square root short */
    sq_sf(&sq_fl, &fl, regs);

    /* Back to register */
    store_sf(&sq_fl, regs->fpr + FPR2I(r1));
}
#endif /* FEATURE_SQUARE_ROOT */


#if defined (FEATURE_HFP_EXTENSIONS)
/*-------------------------------------------------------------------*/
/* B324 LDER  - Load Length. Float. Short to Long Register     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_float_short_to_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Copy register content */
    regs->fpr[i1] = regs->fpr[FPR2I(r2)];

    /* Clear register */
    regs->fpr[i1+1] = 0;
}


/*-------------------------------------------------------------------*/
/* B325 LXDR  - Load Length. Float. Long to Extended Register  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_float_long_to_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);
    HFPREG_CHECK(r2, regs);
    i2 = FPR2I(r2);

    if ((regs->fpr[i2] & 0x00FFFFFF)
    || regs->fpr[i2+1]) {
        /* Copy register contents */
        regs->fpr[i1] = regs->fpr[i2];
        regs->fpr[i1+1] = regs->fpr[i2+1];

        /* Low order register */
        regs->fpr[i1+FPREX] = (regs->fpr[i2] & 0x80000000)
                            | ((regs->fpr[i2] - (14 << 24)) & 0x7F000000);
    } else {
        /* true zero with sign */
        regs->fpr[i1] = regs->fpr[i2] & 0x80000000;
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX] = regs->fpr[i1];
    }
    /* Clear register */
    regs->fpr[i1+FPREX+1] = 0;
}


/*-------------------------------------------------------------------*/
/* B326 LXER  - Load Length. Float. Short to Extended Register [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_float_short_to_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);
    HFPREG_CHECK(r2, regs);
    i2 = FPR2I(r2);

    if (regs->fpr[i2] & 0x00FFFFFF) {
        /* Copy register content */
        regs->fpr[i1] = regs->fpr[i2];

        /* Low order register */
        regs->fpr[i1+FPREX] = (regs->fpr[i2] & 0x80000000)
                            | ((regs->fpr[i2] - (14 << 24)) & 0x7F000000);
    } else {
        /* true zero with sign */
        regs->fpr[i1] = regs->fpr[i2] & 0x80000000;
        regs->fpr[i1+FPREX] = regs->fpr[i1];
    }
    /* Clear register */
    regs->fpr[i1+1] = 0;
    regs->fpr[i1+FPREX+1] = 0;
}


/*-------------------------------------------------------------------*/
/* B336 SQXR  - Square Root Floating Point Extended Register   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(squareroot_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
EXTENDED_FLOAT sq_fl;
EXTENDED_FLOAT fl;
U64     mmsa, msa, lsa, llsa;
U64     xi;
U64     xj;
U64     msi, lsi;
U64     msj, lsj;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    /* Get the 2nd operand */
    get_ef(&fl, regs->fpr + FPR2I(r2));

    if ((fl.ms_fract)
    || (fl.ls_fract)) {
        if (fl.sign) {
            /* less than zero */

            ARCH_DEP(program_interrupt) (regs, PGM_SQUARE_ROOT_EXCEPTION);
            UNREACHABLE_CODE( return );
        } else {
            /* normalize operand */
            normal_ef(&fl);

            if (fl.expo & 1) {
                /* odd */

                /* compute characteristic */
                sq_fl.expo = (fl.expo + 65) >> 1;

                /* with guard digit */
                mmsa = fl.ms_fract >> 4;
                msa = (fl.ms_fract << 60)
                     | (fl.ls_fract >> 4);
                lsa = fl.ls_fract << 60;
                llsa = 0;
            } else {
                /* even */

                /* compute characteristic */
                sq_fl.expo = (fl.expo + 64) >> 1;

                /* without guard digit */
                mmsa = fl.ms_fract;
                msa = fl.ls_fract;
                lsa = 0;
                llsa = 0;
            }

            /* square root of fraction low precision */
            /* common subroutine of all square root */
            xi = ((U64) (square_root_fraction(mmsa & 0xFFFFFFFFFFFFFFFEULL)) << 32)
               | 0x80000000UL;

            /* continue iteration for high precision */
            /* done iteration when xi, xj equal or differ by 1 */
            for (;;) {
                xj = (div_U128(mmsa, msa, xi) + xi) >> 1;
                if (0
                    || xj == xi
                    || xj == (xi+1)
                    || xj == (xi-1)
                )
                    break;
                xi = xj;
            }

            msi = xi;
            lsi = 0x8000000000000000ULL;

            /* continue iteration for extended precision */
            for (;;) {
                div_U256(mmsa, msa, lsa, llsa, msi, lsi, &msj, &lsj);
                add_U128(msj, lsj, msi, lsi);
                shift_right_U128(msj, lsj);

                if ((msj == msi)
                && (lsj == lsi)) {
                    break;
                }
                msi = msj;
                lsi = lsj;
            }
            /* round with guard digit */
            add_U128(msi, lsi, 0, 0x80);

            sq_fl.ls_fract = (lsi >> 8)
                           | (msi << 56);
            sq_fl.ms_fract = msi >> 8;
        }
    } else {
        /* true zero */
        sq_fl.ms_fract = 0;
        sq_fl.ls_fract = 0;
        sq_fl.expo = 0;
    }
    /* all results positive */
    sq_fl.sign = POS;

    /* Back to register */
    store_ef(&sq_fl, regs->fpr + FPR2I(r1));
}


/*-------------------------------------------------------------------*/
/* B337 MEER  - Multiply Floating Point Short Register         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
SHORT_FLOAT fl;
SHORT_FLOAT mul_fl;
int     pgm_check;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    get_sf(&mul_fl, regs->fpr + FPR2I(r2));

    /* multiply short to long */
    pgm_check = mul_sf(&fl, &mul_fl, OVUNF, regs);

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_float_short_reg) */


/*-------------------------------------------------------------------*/
/* B360 LPXR  - Load Positive Floating Point Extended Register [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_positive_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    if ((regs->fpr[i2] & 0x00FFFFFF)
    || regs->fpr[i2+1]
    || (regs->fpr[i2+FPREX] & 0x00FFFFFF)
    || regs->fpr[i2+FPREX+1]) {
        /* Copy register contents, clear the sign bit */
        regs->fpr[i1] = regs->fpr[i2] & 0x7FFFFFFF;
        regs->fpr[i1+1] = regs->fpr[i2+1];

        /* Low order register */
        regs->fpr[i1+FPREX] = (((regs->fpr[i2] - (14 << 24)) & 0x7F000000))
                            | (regs->fpr[i2+FPREX] & 0x00FFFFFF);
        regs->fpr[i1+FPREX+1] = regs->fpr[i2+FPREX+1];

        /* Set condition code */
        regs->psw.cc = 2;
    } else {
        /* true zero */
        regs->fpr[i1] = 0;
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX] = 0;
        regs->fpr[i1+FPREX+1] = 0;

        /* Set condition code */
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B361 LNXR  - Load Negative Floating Point Extended Register [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_negative_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    if ((regs->fpr[i2] & 0x00FFFFFF)
    || regs->fpr[i2+1]
    || (regs->fpr[i2+FPREX] & 0x00FFFFFF)
    || regs->fpr[i2+FPREX+1]) {
        /* Copy register contents, set the sign bit */
        regs->fpr[i1] = 0x80000000
                      | regs->fpr[i2];
        regs->fpr[i1+1] = regs->fpr[i2+1];

        /* Low order register */
        regs->fpr[i1+FPREX] = 0x80000000
                            | (((regs->fpr[i2] - (14 << 24)) & 0x7F000000))
                            | (regs->fpr[i2+FPREX] & 0x00FFFFFF);
        regs->fpr[i1+FPREX+1] = regs->fpr[i2+FPREX+1];

        /* Set condition code */
        regs->psw.cc = 1;
    } else {
        /* true zero with sign */
        regs->fpr[i1] = 0x80000000;
        regs->fpr[i1+FPREX] = 0x80000000;
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX+1] = 0;

        /* Set condition code */
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B362 LTXR  - Load and Test Floating Point Extended Register [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    if ((regs->fpr[i2] & 0x00FFFFFF)
    || regs->fpr[i2+1]
    || (regs->fpr[i2+FPREX] & 0x00FFFFFF)
    || regs->fpr[i2+FPREX+1]) {
        /* Copy register contents */
        regs->fpr[i1] = regs->fpr[i2];
        regs->fpr[i1+1] = regs->fpr[i2+1];

        /* Low order register */
        regs->fpr[i1+FPREX] = (regs->fpr[i2] & 0x80000000)
                            | ((regs->fpr[i2] - (14 << 24)) & 0x7F000000)
                            | (regs->fpr[i2+FPREX] & 0x00FFFFFF);
        regs->fpr[i1+FPREX+1] = regs->fpr[i2+FPREX+1];

        /* Set condition code */
        regs->psw.cc = (regs->fpr[i2] & 0x80000000) ? 1 : 2;
    } else {
        /* true zero with sign */
        regs->fpr[i1] = regs->fpr[i2] & 0x80000000;
        regs->fpr[i1+FPREX] = regs->fpr[i1];
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX+1] = 0;

        /* Set condition code */
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B363 LCXR  - Load Complement Float. Extended Register       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_complement_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    if ((regs->fpr[i2] & 0x00FFFFFF)
    || regs->fpr[i2+1]
    || (regs->fpr[i2+FPREX] & 0x00FFFFFF)
    || regs->fpr[i2+FPREX+1]) {
        /* Copy register contents, invert sign bit */
        regs->fpr[i1] = regs->fpr[i2] ^ 0x80000000;
        regs->fpr[i1+1] = regs->fpr[i2+1];

        /* Low order register */
        regs->fpr[i1+FPREX] = (regs->fpr[i1] & 0x80000000)
                           |(((regs->fpr[i1] & 0x7F000000) - 0x0E000000) & 0x7F000000)
                           |  (regs->fpr[i2+FPREX] & 0x00FFFFFF);
        regs->fpr[i1+FPREX+1] = regs->fpr[i2+FPREX+1];

        /* Set condition code */
        regs->psw.cc = (regs->fpr[i1] & 0x80000000) ? 1 : 2;
    } else {
        /* true zero with sign */
        regs->fpr[i1] = (regs->fpr[i2] ^ 0x80000000) & 0x80000000;
        regs->fpr[i1+FPREX] = regs->fpr[i1];
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX+1] = 0;

        /* Set condition code */
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B366 LEXR  - Load Rounded Float. Extended to Short Register [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_rounded_float_ext_to_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
EXTENDED_FLOAT from_fl;
SHORT_FLOAT to_fl;
int     pgm_check;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);
    HFPODD_CHECK(r2, regs);

    /* Get register content */
    get_ef(&from_fl, regs->fpr + FPR2I(r2));

    /* Rounding (herc ms fract is 12 digits) */
    to_fl.short_fract = (from_fl.ms_fract + 0x0000000000800000ULL) >> 24;
    to_fl.sign = from_fl.sign;
    to_fl.expo = from_fl.expo;

    /* Handle overflow */
    if (to_fl.short_fract & 0x0F000000) {
        to_fl.short_fract >>= 4;
        (to_fl.expo)++;
        pgm_check = overflow_sf(&to_fl, regs);
    } else {
        pgm_check = 0;
    }

    /* To register */
    store_sf(&to_fl, regs->fpr + FPR2I(r1));

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}


/*-------------------------------------------------------------------*/
/* B367 FIXR  - Load FP Integer Float. Extended Register       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fp_int_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
EXTENDED_FLOAT fl;
BYTE    shift;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get register content */
    get_ef(&fl, regs->fpr + FPR2I(r2));

    if (fl.expo > 64) {
        if (fl.expo < 92) {
            /* Denormalize */
            shift = (92 - fl.expo) * 4;
            if (shift > 64) {
                fl.ls_fract = fl.ms_fract >> (shift - 64);
                fl.ms_fract = 0;
            } else if (shift == 64) {
                fl.ls_fract = fl.ms_fract;
                fl.ms_fract = 0;
            } else {
                fl.ls_fract = (fl.ls_fract >> shift)
                            | (fl.ms_fract << (64 - shift));
                fl.ms_fract >>= shift;
            }
            fl.expo = 92;
        }

        /* Normalize result */
        normal_ef(&fl);

        /* To register */
        store_ef(&fl, regs->fpr + i1);
    } else {
        /* True zero */
        regs->fpr[i1] = 0;
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX] = 0;
        regs->fpr[i1+FPREX+1] = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B369 CXR   - Compare Floating Point Extended Register       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
EXTENDED_FLOAT fl;
EXTENDED_FLOAT cmp_fl;
BYTE    shift;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    /* Get the operands */
    get_ef(&fl, regs->fpr + FPR2I(r1));
    get_ef(&cmp_fl, regs->fpr + FPR2I(r2));;

    if (cmp_fl.ms_fract
    || cmp_fl.ls_fract
    || cmp_fl.expo) {           /* cmp_fl not 0 */
        if (fl.ms_fract
        || fl.ls_fract
        || fl.expo) {           /* fl not 0 */
            /* both not 0 */

            if (fl.expo == cmp_fl.expo) {
                /* expo equal */

                /* both guard digits */
                fl.ms_fract = (fl.ms_fract << 4)
                            | (fl.ls_fract >> 60);
                fl.ls_fract <<= 4;
                cmp_fl.ms_fract = (cmp_fl.ms_fract << 4)
                                | (cmp_fl.ls_fract >> 60);
                cmp_fl.ls_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl.expo < cmp_fl.expo) {
                    /* shift minus guard digit */
                    shift = cmp_fl.expo - fl.expo - 1;

                    if (shift) {
                        if (shift >= 28) {
                            /* Set condition code */
                            if (cmp_fl.ms_fract
                            || cmp_fl.ls_fract) {
                                regs->psw.cc = cmp_fl.sign ? 2 : 1;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        } else if (shift >= 16) {
                            fl.ls_fract = fl.ms_fract;
                            if (shift > 16) {
                                fl.ls_fract >>= (shift - 16) * 4;
                            }
                            fl.ms_fract = 0;
                        } else {
                            shift *= 4;
                            fl.ls_fract = fl.ms_fract << (64 - shift)
                                        | fl.ls_fract >> shift;
                            fl.ms_fract >>= shift;
                        }
                        if ((fl.ms_fract == 0)
                        && (fl.ls_fract == 0)) {
                            /* Set condition code */
                            if (cmp_fl.ms_fract
                            || cmp_fl.ls_fract) {
                                regs->psw.cc = cmp_fl.sign ? 2 : 1;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    cmp_fl.ms_fract = (cmp_fl.ms_fract << 4)
                                    | (cmp_fl.ls_fract >> 60);
                    cmp_fl.ls_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl.expo - cmp_fl.expo - 1;

                    if (shift) {
                        if (shift >= 28) {
                            /* Set condition code */
                            if (fl.ms_fract
                            || fl.ls_fract) {
                                regs->psw.cc = fl.sign ? 1 : 2;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        } else if (shift >= 16) {
                            cmp_fl.ls_fract = cmp_fl.ms_fract;
                            if (shift > 16) {
                                cmp_fl.ls_fract >>= (shift - 16) * 4;
                            }
                            cmp_fl.ms_fract = 0;
                        } else {
                            shift *= 4;
                            cmp_fl.ls_fract = cmp_fl.ms_fract << (64 - shift)
                                            | cmp_fl.ls_fract >> shift;
                            cmp_fl.ms_fract >>= shift;
                        }
                        if ((cmp_fl.ms_fract == 0)
                        && (cmp_fl.ls_fract == 0)) {
                            /* Set condition code */
                            if (fl.ms_fract
                            || fl.ls_fract) {
                                regs->psw.cc = fl.sign ? 1 : 2;
                            } else {
                                regs->psw.cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    fl.ms_fract = (fl.ms_fract << 4)
                                | (fl.ls_fract >> 60);
                    fl.ls_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl.sign != cmp_fl.sign) {
                add_U128(fl.ms_fract, fl.ls_fract, cmp_fl.ms_fract, cmp_fl.ls_fract);
            } else if ((fl.ms_fract > cmp_fl.ms_fract)
                   || ((fl.ms_fract == cmp_fl.ms_fract)
                       && (fl.ls_fract >= cmp_fl.ls_fract))) {
                sub_U128(fl.ms_fract, fl.ls_fract, cmp_fl.ms_fract, cmp_fl.ls_fract);
            } else {
                sub_reverse_U128(fl.ms_fract, fl.ls_fract, cmp_fl.ms_fract, cmp_fl.ls_fract);
                fl.sign = ! (cmp_fl.sign);
            }

            /* handle overflow with guard digit */
            if (fl.ms_fract & 0x00F0000000000000ULL) {
                fl.ls_fract = (fl.ms_fract << 60)
                            | (fl.ls_fract >> 4);
                fl.ms_fract >>= 4;
            }

            /* Set condition code */
            if (fl.ms_fract
            || fl.ls_fract) {
                regs->psw.cc = fl.sign ? 1 : 2;
            } else {
                regs->psw.cc = 0;
            }
            return;
        } else { /* fl 0, cmp_fl not 0 */
            /* Set condition code */
            if (cmp_fl.ms_fract
            || cmp_fl.ls_fract) {
                regs->psw.cc = cmp_fl.sign ? 2 : 1;
            } else {
                regs->psw.cc = 0;
            }
            return;
        }
    } else {                        /* cmp_fl 0 */
        /* Set condition code */
        if (fl.ms_fract
        || fl.ls_fract) {
            regs->psw.cc = fl.sign ? 1 : 2;
        } else {
            regs->psw.cc = 0;
        }
        return;
    }
}


/*-------------------------------------------------------------------*/
/* B377 FIER  - Load FP Integer Floating Point Short Register  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fp_int_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
SHORT_FLOAT fl;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get register content */
    get_sf(&fl, regs->fpr + FPR2I(r2));

    if (fl.expo > 64) {
        if (fl.expo < 70) {
            /* Denormalize */
            fl.short_fract >>= ((70 - fl.expo) * 4);
            fl.expo = 70;
        }

        /* Normalize result */
        normal_sf(&fl);

        /* To register */
        store_sf(&fl, regs->fpr + i1);
    } else {
        /* True zero */
        regs->fpr[i1] = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B37F FIDR  - Load FP Integer Floating Point Long Register   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fp_int_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
LONG_FLOAT fl;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);

    /* Get register content */
    get_lf(&fl, regs->fpr + FPR2I(r2));

    if (fl.expo > 64) {
        if (fl.expo < 78) {
            /* Denormalize */
            fl.long_fract >>= ((78 - fl.expo) * 4);
            fl.expo = 78;
        }

        /* Normalize result */
        normal_lf(&fl);

        /* To register */
        store_lf(&fl, regs->fpr + i1);
    } else {
        /* True zero */
        regs->fpr[i1] = 0;
        regs->fpr[i1+1] = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B3B4 CEFR  - Convert from Fixed to Float. Short Register    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fixed_to_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
LONG_FLOAT fl;
S64     fix;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* get fixed value */
    fix = regs->GR_L(r2);
    if (fix &  0x0000000080000000)
        fix |= 0xFFFFFFFF00000000ULL;

    if (fix) {
        if (fix < 0) {
            fl.sign = NEG;
            fl.long_fract = (-fix);
        } else {
            fl.sign = POS;
            fl.long_fract = fix;
        }
        fl.expo = 78;

        /* Normalize result */
        normal_lf(&fl);

        /* To register (converting to short float) */
        regs->fpr[i1] = ((U32)fl.sign << 31)
                      | ((U32)fl.expo << 24)
                      | (fl.long_fract >> 32);
    } else {
        /* true zero */
        regs->fpr[i1] = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B3B5 CDFR  - Convert from Fixed to Float. Long Register     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fixed_to_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
LONG_FLOAT fl;
S64     fix;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* get fixed value */
    fix = regs->GR_L(r2);
    if (fix &  0x0000000080000000)
        fix |= 0xFFFFFFFF00000000ULL;

    if (fix) {
        if (fix < 0) {
            fl.sign = NEG;
            fl.long_fract = (-fix);
        } else {
            fl.sign = POS;
            fl.long_fract = fix;
        }
        fl.expo = 78;

        /* Normalize result */
        normal_lf(&fl);

        /* To register */
        store_lf(&fl, regs->fpr + i1);
    } else {
        /* true zero */
        regs->fpr[i1] = 0;
        regs->fpr[i1+1] = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B3B6 CXFR  - Convert from Fixed to Float. Extended Register [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fixed_to_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
EXTENDED_FLOAT fl;
S64     fix;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* get fixed value */
    fix = regs->GR_L(r2);
    if (fix &  0x0000000080000000)
        fix |= 0xFFFFFFFF00000000ULL;

    if (fix) {
        if (fix < 0) {
            fl.sign = NEG;
            fl.ms_fract = (-fix);
        } else {
            fl.sign = POS;
            fl.ms_fract = fix;
        }
        fl.ls_fract = 0;
        fl.expo = 76;  /* 64 + 12 (Herc ms fract is 12 digits) */

        /* Normalize result */
        normal_ef(&fl);

        /* To register */
        store_ef(&fl, regs->fpr + i1);
    } else {
        /* true zero */
        regs->fpr[i1] = 0;
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX] = 0;
        regs->fpr[i1+FPREX+1] = 0;
    }
}


#if defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS )
/*-------------------------------------------------------------------*/
/* B3C4 CEGR - Convert from Fix64 to Float. Short Register     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fix64_to_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
SHORT_FLOAT fl;
U64     fix;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);
    fix = regs->GR_G(r2);

    /* Test for negative value */
    if (fix & 0x8000000000000000ULL) {
        fix = ~fix + 1;  /* fix = abs(fix); */
        fl.sign = NEG;
    } else
        fl.sign = POS;

    if (fix) {
        fl.expo = 70;

        /* Truncate fraction to 6 hexadecimal digits */
        while (fix & 0xFFFFFFFFFF000000ULL)
        {
            fix >>= 4;
            fl.expo += 1;
        }
        fl.short_fract = (U32)fix;

        /* Normalize result */
        normal_sf(&fl);

        /* To register */
        store_sf(&fl, regs->fpr + i1);
    } else {
        /* true zero */
        regs->fpr[i1] = 0;
    }
}
#endif /* defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */


#if defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS )
/*-------------------------------------------------------------------*/
/* B3C5 CDGR - Convert from Fix64 to Float. Long Register      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fix64_to_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
LONG_FLOAT fl;
U64     fix;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);
    fix = regs->GR_G(r2);

    /* Test for negative value */
    if (fix & 0x8000000000000000ULL) {
        fix = ~fix + 1;  /* fix = abs(fix); */
        fl.sign = NEG;
    } else
        fl.sign = POS;

    if (fix) {
        fl.long_fract = fix;
        fl.expo = 78;

        /* Truncate fraction to 14 hexadecimal digits */
        while (fl.long_fract & 0xFF00000000000000ULL)
        {
            fl.long_fract >>= 4;
            fl.expo += 1;
        }

        /* Normalize result */
        normal_lf(&fl);

        /* To register */
        store_lf(&fl, regs->fpr + i1);
    } else {
        /* true zero */
        regs->fpr[i1] = 0;
        regs->fpr[i1+1] = 0;
    }
}
#endif /* defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */


#if defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS )
/*-------------------------------------------------------------------*/
/* B3C6 CXGR - Convert from Fix64 to Float. Extended Register  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_fix64_to_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;
EXTENDED_FLOAT fl;
U64     fix;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);
    fix = regs->GR_G(r2);

    /* Test for negative value */
    if (fix & 0x8000000000000000ULL) {
        fix = ~fix + 1;  /* fix = abs(fix); */
        fl.sign = NEG;
    } else
        fl.sign = POS;

    if (fix) {
        fl.ms_fract = fix >> 16;        /* Fraction high (12 digits) */
        fl.ls_fract = fix << 48;        /* Fraction low (16 digits)  */
        fl.expo = 80;

        /* Normalize result */
        normal_ef(&fl);

        /* To register */
        store_ef(&fl, regs->fpr + i1);
    } else {
        /* true zero */
        regs->fpr[i1] = 0;
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX] = 0;
        regs->fpr[i1+FPREX+1] = 0;
    }
}
#endif /* defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */


/*-------------------------------------------------------------------*/
/* B3B8 CFER  - Convert from Float. Short to Fixed Register  [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_short_to_fixed_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     m3;
SHORT_FLOAT fl;
BYTE    shift;
U32     lsfract;

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPM_CHECK(m3, regs);
    HFPREG_CHECK(r2, regs);

    /* Get register content */
    get_sf(&fl, regs->fpr + FPR2I(r2));

    if (fl.short_fract) {
        /* not zero */
        normal_sf(&fl);

        if (fl.expo > 72) {
            /* exeeds range by exponent */
            regs->GR_L(r1) = fl.sign ? 0x80000000UL : 0x7FFFFFFFUL;
            regs->psw.cc = 3;
            return;
        }
        if (fl.expo > 70) {
            /* to be left shifted */
            fl.short_fract <<= ((fl.expo - 70) * 4);
            if (fl.sign) {
                /* negative */
                if (fl.short_fract > 0x80000000UL) {
                    /* exeeds range by value */
                    regs->GR_L(r1) = 0x80000000UL;
                    regs->psw.cc = 3;
                    return;
                }
            } else {
                /* positive */
                if (fl.short_fract > 0x7FFFFFFFUL) {
                    /* exeeds range by value */
                    regs->GR_L(r1) = 0x7FFFFFFFUL;
                    regs->psw.cc = 3;
                    return;
                }
            }
        } else if ((fl.expo > 64)
               && (fl.expo < 70)) {
            /* to be right shifted and to be rounded */
            shift = ((70 - fl.expo) * 4);
            lsfract = fl.short_fract << (32 - shift);
            fl.short_fract >>= shift;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x80000000UL) {
                    fl.short_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract > 0x80000000UL)
                || ((fl.short_fract & 0x00000001UL)
                    && (lsfract == 0x80000000UL))) {
                    fl.short_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.short_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.short_fract++;
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = fl.short_fract << 8;
            fl.short_fract = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x80000000UL) {
                    fl.short_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (lsfract > 0x80000000UL) {
                    fl.short_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.short_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.short_fract++;
                }
            }
        } else if (fl.expo < 64) {
            fl.short_fract = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                fl.short_fract++;
            }
        }
        if (fl.sign) {
            /* negative */
            regs->GR_L(r1) = -((S32) fl.short_fract);
            regs->psw.cc = 1;
        } else {
            /* positive */
            regs->GR_L(r1) = fl.short_fract;
            regs->psw.cc = 2;
        }
    } else {
        /* zero */
        regs->GR_L(r1) = 0;
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B3B9 CFDR  - Convert from Float. Long to Fixed Register   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_long_to_fixed_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     m3;
LONG_FLOAT fl;
BYTE    shift;
U64     lsfract;

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPM_CHECK(m3, regs);
    HFPREG_CHECK(r2, regs);

    /* Get register content */
    get_lf(&fl, regs->fpr + FPR2I(r2));

    if (fl.long_fract) {
        /* not zero */
        normal_lf(&fl);

        if (fl.expo > 72) {
            /* exeeds range by exponent */
            regs->GR_L(r1) = fl.sign ? 0x80000000UL : 0x7FFFFFFFUL;
            regs->psw.cc = 3;
            return;
        }
        if (fl.expo > 64) {
            /* to be right shifted and to be rounded */
            shift = ((78 - fl.expo) * 4);
            lsfract = fl.long_fract << (64 - shift);
            fl.long_fract >>= shift;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    fl.long_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract > 0x8000000000000000ULL)
                || ((fl.long_fract & 0x0000000000000001ULL)
                    && (lsfract == 0x8000000000000000ULL))) {
                    fl.long_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.long_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.long_fract++;
                }
            }
            if (fl.expo == 72) {
                if (fl.sign) {
                    /* negative */
                    if (fl.long_fract > 0x80000000UL) {
                        /* exeeds range by value */
                        regs->GR_L(r1) = 0x80000000UL;
                        regs->psw.cc = 3;
                        return;
                    }
                } else {
                    /* positive */
                    if (fl.long_fract > 0x7FFFFFFFUL) {
                        /* exeeds range by value */
                        regs->GR_L(r1) = 0x7FFFFFFFUL;
                        regs->psw.cc = 3;
                        return;
                    }
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = fl.long_fract << 8;
            fl.long_fract = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    fl.long_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (lsfract > 0x8000000000000000ULL) {
                    fl.long_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.long_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.long_fract++;
                }
            }
        } else {
            /* fl.expo < 64 */
            fl.long_fract = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                fl.long_fract++;
            }
        }
        if (fl.sign) {
            /* negative */
            regs->GR_L(r1) = -((S32) fl.long_fract);
            regs->psw.cc = 1;
        } else {
            /* positive */
            regs->GR_L(r1) = fl.long_fract;
            regs->psw.cc = 2;
        }
    } else {
        /* zero */
        regs->GR_L(r1) = 0;
        regs->psw.cc = 0;
    }
}


/*-------------------------------------------------------------------*/
/* B3BA CFXR  - Convert from Float. Extended to Fixed Reg.   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_ext_to_fixed_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     m3;
EXTENDED_FLOAT fl;
BYTE    shift;
U64     lsfract;

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPM_CHECK(m3, regs);
    HFPODD_CHECK(r2, regs);

    /* Get register content */
    get_ef(&fl, regs->fpr + FPR2I(r2));

    if (fl.ms_fract
    || fl.ls_fract) {
        /* not zero */
        normal_ef(&fl);

        if (fl.expo > 72) {
            /* exeeds range by exponent */
            regs->GR_L(r1) = fl.sign ? 0x80000000UL : 0x7FFFFFFFUL;
            regs->psw.cc = 3;
            return;
        }
        if (fl.expo > 64) {
            /* to be right shifted and to be rounded */
            shift = ((92 - fl.expo - 16) * 4);
            lsfract = fl.ms_fract << (64 - shift);
            fl.ms_fract >>= shift;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    fl.ms_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (((lsfract & 0x8000000000000000ULL)
                    && ((lsfract & 0x7FFFFFFFFFFFFFFFULL)
                        || fl.ls_fract))
                || ( (fl.ms_fract & 0x0000000000000001ULL)
                    && (lsfract == 0x8000000000000000ULL)
                    && (fl.ls_fract == 0))) {
                    fl.ms_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && (lsfract
                    || fl.ls_fract)) {
                    fl.ms_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && (lsfract
                    || fl.ls_fract)) {
                    fl.ms_fract++;
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = fl.ms_fract << 16;
            fl.ms_fract = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    fl.ms_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract & 0x8000000000000000ULL)
                && ((lsfract & 0x7FFFFFFFFFFFFFFFULL)
                    || fl.ls_fract)) {
                    fl.ms_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && (lsfract
                    || fl.ls_fract)) {
                    fl.ms_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && (lsfract
                    || fl.ls_fract)) {
                    fl.ms_fract++;
                }
            }
        } else {
            /* fl.expo < 64 */
            fl.ms_fract = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                fl.ms_fract++;
            }
        }
        if (fl.sign) {
            /* negative */
            if (fl.ms_fract > 0x80000000UL) {
                /* exeeds range by value */
                regs->GR_L(r1) = 0x80000000UL;
                regs->psw.cc = 3;
                return;
            }
            regs->GR_L(r1) = -((S32) fl.ms_fract);
            regs->psw.cc = 1;
        } else {
            /* positive */
            if (fl.ms_fract > 0x7FFFFFFFUL) {
                /* exeeds range by value */
                regs->GR_L(r1) = 0x7FFFFFFFUL;
                regs->psw.cc = 3;
                return;
            }
            regs->GR_L(r1) = fl.ms_fract;
            regs->psw.cc = 2;
        }
    } else {
        /* zero */
        regs->GR_L(r1) = 0;
        regs->psw.cc = 0;
    }
}


#if defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS )
/*-------------------------------------------------------------------*/
/* B3C8 CGER  - Convert from Float. Short to Fix64 Register  [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_short_to_fix64_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     m3;
SHORT_FLOAT fl;
BYTE    shift;
U64     intpart;
U32     lsfract;

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPM_CHECK(m3, regs);
    HFPREG_CHECK(r2, regs);

    /* Get register content */
    get_sf(&fl, regs->fpr + FPR2I(r2));

    if (fl.short_fract) {
        /* not zero */
        normal_sf(&fl);

        if (fl.expo > 80) {
            /* exeeds range by exponent */
            regs->GR_G(r1) = fl.sign ? 0x8000000000000000ULL : 0x7FFFFFFFFFFFFFFFULL;
            regs->psw.cc = 3;
            return;
        }
        if (fl.expo > 70) {
            /* to be left shifted */
            intpart = (U64)fl.short_fract << ((fl.expo - 70) * 4);
            if (fl.sign) {
                /* negative */
                if (intpart > 0x8000000000000000ULL) {
                    /* exeeds range by value */
                    regs->GR_G(r1) = 0x8000000000000000ULL;
                    regs->psw.cc = 3;
                    return;
                }
            } else {
                /* positive */
                if (intpart > 0x7FFFFFFFFFFFFFFFULL) {
                    /* exeeds range by value */
                    regs->GR_G(r1) = 0x7FFFFFFFFFFFFFFFULL;
                    regs->psw.cc = 3;
                    return;
                }
            }
        } else if ((fl.expo > 64)
               && (fl.expo < 70)) {
            /* to be right shifted and to be rounded */
            shift = ((70 - fl.expo) * 4);
            lsfract = fl.short_fract << (32 - shift);
            intpart = fl.short_fract >> shift;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x80000000UL) {
                    intpart++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract > 0x80000000UL)
                || ((intpart & 0x0000000000000001ULL)
                    && (lsfract == 0x80000000UL))) {
                    intpart++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    intpart++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    intpart++;
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = fl.short_fract << 8;
            intpart = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x80000000UL) {
                    intpart++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (lsfract > 0x80000000UL) {
                    intpart++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    intpart++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    intpart++;
                }
            }
        } else if (fl.expo < 64) {
            intpart = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                intpart++;
            }
        } else /* fl.expo == 70 */ {
            /* no shift or round required */
            intpart = (U64)fl.short_fract;
        }
        if (fl.sign) {
            /* negative */
            if (intpart == 0x8000000000000000ULL)
                regs->GR_G(r1) = 0x8000000000000000ULL;
            else
                regs->GR_G(r1) = -((S64)intpart);
            regs->psw.cc = 1;
        } else {
            /* positive */
            regs->GR_G(r1) = intpart;
            regs->psw.cc = 2;
        }
    } else {
        /* zero */
        regs->GR_G(r1) = 0;
        regs->psw.cc = 0;
    }
}
#endif /* defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */


#if defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS )
/*-------------------------------------------------------------------*/
/* B3C9 CGDR  - Convert from Float. Long to Fix64 Register   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_long_to_fix64_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     m3;
LONG_FLOAT fl;
BYTE    shift;
U64     lsfract;

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPM_CHECK(m3, regs);
    HFPREG_CHECK(r2, regs);

    /* Get register content */
    get_lf(&fl, regs->fpr + FPR2I(r2));

    if (fl.long_fract) {
        /* not zero */
        normal_lf(&fl);

        if (fl.expo > 80) {
            /* exeeds range by exponent */
            regs->GR_G(r1) = fl.sign ? 0x8000000000000000ULL : 0x7FFFFFFFFFFFFFFFULL;
            regs->psw.cc = 3;
            return;
        }
        if (fl.expo > 78) {
            /* to be left shifted */
            fl.long_fract <<= ((fl.expo - 78) * 4);
            if (fl.sign) {
                /* negative */
                if (fl.long_fract > 0x8000000000000000ULL) {
                    /* exeeds range by value */
                    regs->GR_G(r1) = 0x8000000000000000ULL;
                    regs->psw.cc = 3;
                    return;
                }
            } else {
                /* positive */
                if (fl.long_fract > 0x7FFFFFFFFFFFFFFFULL) {
                    /* exeeds range by value */
                    regs->GR_G(r1) = 0x7FFFFFFFFFFFFFFFULL;
                    regs->psw.cc = 3;
                    return;
                }
            }
        } else if ((fl.expo > 64)
               && (fl.expo < 78)) {
            /* to be right shifted and to be rounded */
            shift = ((78 - fl.expo) * 4);
            lsfract = fl.long_fract << (64 - shift);
            fl.long_fract >>= shift;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    fl.long_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract > 0x8000000000000000ULL)
                || ((fl.long_fract & 0x0000000000000001ULL)
                    && (lsfract == 0x8000000000000000ULL))) {
                    fl.long_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.long_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.long_fract++;
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = fl.long_fract << 8;
            fl.long_fract = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    fl.long_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (lsfract > 0x8000000000000000ULL) {
                    fl.long_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.long_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.long_fract++;
                }
            }
        } else if (fl.expo < 64) {
            fl.long_fract = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                fl.long_fract++;
            }
        }
        if (fl.sign) {
            /* negative */
            if (fl.long_fract == 0x8000000000000000ULL)
                regs->GR_G(r1) = 0x8000000000000000ULL;
            else
                regs->GR_G(r1) = -((S64)fl.long_fract);
            regs->psw.cc = 1;
        } else {
            /* positive */
            regs->GR_G(r1) = fl.long_fract;
            regs->psw.cc = 2;
        }
    } else {
        /* zero */
        regs->GR_G(r1) = 0;
        regs->psw.cc = 0;
    }
}
#endif /* defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */


#if defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS )
/*-------------------------------------------------------------------*/
/* B3CA CGXR  - Convert from Float. Extended to Fix64 Reg.   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_ext_to_fix64_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     m3;
EXTENDED_FLOAT fl;
BYTE    shift;
U64     intpart;
U64     lsfract;

    RRF_M(inst, regs, r1, r2, m3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPM_CHECK(m3, regs);
    HFPODD_CHECK(r2, regs);

    /* Get register content */
    get_ef(&fl, regs->fpr + FPR2I(r2));

    if (fl.ms_fract
    || fl.ls_fract) {
        /* not zero */
        normal_ef(&fl);

        if (fl.expo > 80) {
            /* exeeds range by exponent */
            regs->GR_G(r1) = fl.sign ? 0x8000000000000000ULL : 0x7FFFFFFFFFFFFFFFULL;
            regs->psw.cc = 3;
            return;
        }
        if (fl.expo > 64) {
            /* to be right shifted and to be rounded */
            shift = ((92 - fl.expo) * 4);
            if (shift > 64) {
                intpart = fl.ms_fract >> (shift - 64);
                lsfract = (fl.ls_fract >> (128 - shift))
                            | (fl.ms_fract << (128 - shift));
            } else if (shift < 64) {
                intpart = (fl.ls_fract >> shift)
                            | (fl.ms_fract << (64 - shift));
                lsfract = fl.ls_fract << (64 - shift);
            } else /* shift == 64 */ {
                intpart = fl.ms_fract;
                lsfract = fl.ls_fract;
            }

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    intpart++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract > 0x8000000000000000ULL)
                || ((intpart & 0x0000000000000001ULL)
                    && (lsfract == 0x8000000000000000ULL))) {
                    intpart++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    intpart++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    intpart++;
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = (fl.ms_fract << 16) | (fl.ls_fract >> 48);
            intpart = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x8000000000000000ULL) {
                    intpart++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (lsfract > 0x8000000000000000ULL) {
                    intpart++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    intpart++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    intpart++;
                }
            }
        } else {
            /* fl.expo < 64 */
            intpart = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                intpart++;
            }
        }
        if (fl.sign) {
            /* negative */
            if (intpart > 0x8000000000000000ULL) {
                /* exeeds range by value */
                regs->GR_G(r1) = 0x8000000000000000ULL;
                regs->psw.cc = 3;
                return;
            }
            if (intpart == 0x8000000000000000ULL)
                regs->GR_G(r1) = 0x8000000000000000ULL;
            else
                regs->GR_G(r1) = -((S64)intpart);
            regs->psw.cc = 1;
        } else {
            /* positive */
            if (intpart > 0x7FFFFFFFFFFFFFFFULL) {
                /* exeeds range by value */
                regs->GR_G(r1) = 0x7FFFFFFFFFFFFFFFULL;
                regs->psw.cc = 3;
                return;
            }
            regs->GR_G(r1) = intpart;
            regs->psw.cc = 2;
        }
    } else {
        /* zero */
        regs->GR_G(r1) = 0;
        regs->psw.cc = 0;
    }
}
#endif /* defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */


/*-------------------------------------------------------------------*/
/* ED24 LDE   - Load Lengthened Floating Point Short to Long   [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_float_short_to_long)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXE(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Update first 32 bits of register from operand address */
    regs->fpr[i1] = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);

    /* Zero register content */
    regs->fpr[i1+1] = 0;
}


/*-------------------------------------------------------------------*/
/* ED25 LXD  - Load Lengthened Floating Point Long to Extended [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_float_long_to_ext)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     wk;
U64     wkd;

    RXE(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the 2nd operand */
    wkd = ARCH_DEP(vfetch8) (effective_addr2, b2, regs);
    wk = wkd >> 32;

    if (wkd & 0x00FFFFFFFFFFFFFFULL) {
        /* Back to register */
        regs->fpr[i1] = wk;
        regs->fpr[i1+1] = wkd;

        /* Low order register */
        regs->fpr[i1+FPREX] = (wk & 0x80000000)
                            | ((wk - (14 << 24)) & 0x7F000000);
    } else {
        /* true zero with sign */
        regs->fpr[i1] = (wk & 0x80000000);
        regs->fpr[i1+FPREX] = regs->fpr[i1];
        regs->fpr[i1+1] = 0;
    }
    regs->fpr[i1+FPREX+1] = 0;
}


/*-------------------------------------------------------------------*/
/* ED26 LXE   - Load Lengthened Float. Short to Extended       [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_lengthened_float_short_to_ext)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     wk;

    RXE(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the 2nd operand */
    wk = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);

    if (wk & 0x00FFFFFF) {
        /* Back to register */
        regs->fpr[i1] = wk;

        /* Zero register content */
        regs->fpr[i1+1] = 0;

        /* Low order register */
        regs->fpr[i1+FPREX] = (wk & 0x80000000)
                       | ((wk - (14 << 24)) & 0x7F000000);
        regs->fpr[i1+FPREX+1] = 0;
    } else {
        /* true zero with sign */
        regs->fpr[i1] = (wk & 0x80000000);
        regs->fpr[i1+FPREX] = regs->fpr[i1];
        regs->fpr[i1+1] = 0;
        regs->fpr[i1+FPREX+1] = 0;
    }
}


/*-------------------------------------------------------------------*/
/* ED34 SQE   - Square Root Floating Point Short               [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(squareroot_float_short)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT sq_fl;
SHORT_FLOAT fl;

    RXE(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Get the 2nd operand */
    vfetch_sf(&fl, effective_addr2, b2, regs );

    /* short square root subroutine */
    sq_sf(&sq_fl, &fl, regs);

    /* Back to register */
    store_sf(&sq_fl, regs->fpr + FPR2I(r1));
}


/*-------------------------------------------------------------------*/
/* ED35 SQD   - Square Root Floating Point Long                [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(squareroot_float_long)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT sq_fl = { 0, 0, 0 };
LONG_FLOAT fl;

    RXE(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Get the 2nd operand */
    vfetch_lf(&fl, effective_addr2, b2, regs );

    /* long square root subroutine */
    sq_lf(&sq_fl, &fl, regs);

    /* Back to register */
    store_lf(&sq_fl, regs->fpr + FPR2I(r1));
}


/*-------------------------------------------------------------------*/
/* ED37 MEE   - Multiply Floating Point Short                  [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_float_short)
{
int     r1;                             /* Value of R field          */
int     i1;
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl;
SHORT_FLOAT mul_fl;
int     pgm_check;

    RXE(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl, regs->fpr + i1);
    vfetch_sf(&mul_fl, effective_addr2, b2, regs );

    /* multiply short to long */
    pgm_check = mul_sf(&fl, &mul_fl, OVUNF, regs);

    /* Back to register */
    store_sf(&fl, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }
}
#endif /* FEATURE_HFP_EXTENSIONS */


#if defined(FEATURE_FPS_EXTENSIONS)
/*-------------------------------------------------------------------*/
/* B365 LXR   - Load Floating Point Extended Register          [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1, i2;                         /* Index into fpr array      */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPODD2_CHECK(r1, r2, regs);

    i1 = FPR2I(r1);
    i2 = FPR2I(r2);

    /* Copy register R2 contents to register R1 */
    regs->fpr[i1] = regs->fpr[i2];
    regs->fpr[i1+1] = regs->fpr[i2+1];
    regs->fpr[i1+FPREX] = regs->fpr[i2+FPREX];
    regs->fpr[i1+FPREX+1] = regs->fpr[i2+FPREX+1];

} /* end DEF_INST(load_float_ext_reg) */


/*-------------------------------------------------------------------*/
/* B374 LZER  - Load Zero Floating Point Short Register        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_zero_float_short_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Set all bits of register R1 to zeros */
    regs->fpr[i1] = 0;

} /* end DEF_INST(load_zero_float_short_reg) */


/*-------------------------------------------------------------------*/
/* B375 LZDR  - Load Zero Floating Point Long Register         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_zero_float_long_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Set all bits of register R1 to zeros */
    regs->fpr[i1] = 0;
    regs->fpr[i1+1] = 0;

} /* end DEF_INST(load_zero_float_long_reg) */


/*-------------------------------------------------------------------*/
/* B376 LZXR  - Load Zero Floating Point Extended Register     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_zero_float_ext_reg)
{
int     r1, r2;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */

    RRE(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Set all bits of register R1 to zeros */
    regs->fpr[i1] = 0;
    regs->fpr[i1+1] = 0;
    regs->fpr[i1+FPREX] = 0;
    regs->fpr[i1+FPREX+1] = 0;

} /* end DEF_INST(load_zero_float_ext_reg) */
#endif /*defined(FEATURE_FPS_EXTENSIONS)*/


#if defined(FEATURE_020_HFP_MULT_ADD_SUB_FACILITY)
/*-------------------------------------------------------------------*/
/* B32E MAER  - Multiply and Add Floating Point Short Register [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_float_short_reg)
{
int     r1, r2, r3;                     /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
SHORT_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);
    HFPREG_CHECK(r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl1, regs->fpr + i1);
    get_sf(&fl2, regs->fpr + FPR2I(r2));
    get_sf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply third and second operands */
    mul_sf(&fl2, &fl3, NOOVUNF, regs);

    /* Add the first operand with normalization */
    pgm_check = add_sf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Store result back to first operand register */
    store_sf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_add_float_short_reg) */


/*-------------------------------------------------------------------*/
/* B32F MSER  - Multiply and Subtract Floating Point Short Reg [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_subtract_float_short_reg)
{
int     r1, r2, r3;                     /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
SHORT_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);
    HFPREG_CHECK(r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl1, regs->fpr + i1);
    get_sf(&fl2, regs->fpr + FPR2I(r2));
    get_sf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply third and second operands */
    mul_sf(&fl2, &fl3, NOOVUNF, regs);

    /* Invert the sign of the first operand */
    fl1.sign = ! (fl1.sign);

    /* Subtract the first operand with normalization */
    pgm_check = add_sf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Store result back to first operand register */
    store_sf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_subtract_float_short_reg) */


/*-------------------------------------------------------------------*/
/* B33E MADR  - Multiply and Add Floating Point Long Register  [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_float_long_reg)
{
int     r1, r2, r3;                     /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
LONG_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);
    HFPREG_CHECK(r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply long third and second operands */
    mul_lf(&fl2, &fl3, NOOVUNF, regs);

    /* Add the first operand with normalization */
    pgm_check = add_lf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Store result back to first operand register */
    store_lf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_add_float_long_reg) */


/*-------------------------------------------------------------------*/
/* B33F MSDR  - Multiply and Subtract Floating Point Long Reg  [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_subtract_float_long_reg)
{
int     r1, r2, r3;                     /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
LONG_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r2, regs);
    HFPREG_CHECK(r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply long third and second operands */
    mul_lf(&fl2, &fl3, NOOVUNF, regs);

    /* Invert the sign of the first operand */
    fl1.sign = ! (fl1.sign);

    /* Subtract the first operand with normalization */
    pgm_check = add_lf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Store result back to first operand register */
    store_lf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_subtract_float_long_reg) */


/*-------------------------------------------------------------------*/
/* ED2E MAE   - Multiply and Add Floating Point Short          [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_float_short)
{
int     r1, r3;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl1, regs->fpr + i1);
    vfetch_sf(&fl2, effective_addr2, b2, regs );
    get_sf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply third and second operands */
    mul_sf(&fl2, &fl3, NOOVUNF, regs);

    /* Add the first operand with normalization */
    pgm_check = add_sf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Back to register */
    store_sf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_add_float_short) */


/*-------------------------------------------------------------------*/
/* ED2F MSE   - Multiply and Subtract Floating Point Short     [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_subtract_float_short)
{
int     r1, r3;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
SHORT_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_sf(&fl1, regs->fpr + i1);
    vfetch_sf(&fl2, effective_addr2, b2, regs );
    get_sf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply third and second operands */
    mul_sf(&fl2, &fl3, NOOVUNF, regs);

    /* Invert the sign of the first operand */
    fl1.sign = ! (fl1.sign);

    /* Subtract the first operand with normalization */
    pgm_check = add_sf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Back to register */
    store_sf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_subtract_float_short) */


/*-------------------------------------------------------------------*/
/* ED3E MAD   - Multiply and Add Floating Point Long           [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_float_long)
{
int     r1, r3;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply long third and second operands */
    mul_lf(&fl2, &fl3, NOOVUNF, regs);

    /* Add long first operand with normalization */
    pgm_check = add_lf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Back to register */
    store_lf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_add_float_long) */


/*-------------------------------------------------------------------*/
/* ED3F MSD   - Multiply and Subtract Floating Point Long      [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_subtract_float_long)
{
int     r1, r3;                         /* Values of R fields        */
int     i1;                             /* Index of R1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
LONG_FLOAT fl1, fl2, fl3;
int     pgm_check;

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Multiply long third and second operands */
    mul_lf(&fl2, &fl3, NOOVUNF, regs);

    /* Invert the sign of the first operand */
    fl1.sign = ! (fl1.sign);

    /* Subtract long with normalization */
    pgm_check = add_lf(&fl1, &fl2, NORMAL, NOSIGEX, regs);

    /* Back to register */
    store_lf(&fl1, regs->fpr + i1);

    /* Program check ? */
    if (pgm_check) {
        ARCH_DEP(program_interrupt) (regs, pgm_check);
    }

} /* end DEF_INST(multiply_subtract_float_long) */

#endif /*defined(FEATURE_020_HFP_MULT_ADD_SUB_FACILITY)*/


#if defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY)
/*-------------------------------------------------------------------*/
/* B338 MAYLR - Multiply and Add Unnorm. Long to Ext. Low Reg. [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_low_reg)
{
int            r1, r2, r3;              /* Values of R fields        */
int            i1;                      /* Index of FP register      */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
LONG_FLOAT     fl1;                     /* Addend                    */
EXTENDED_FLOAT fxp1;                    /* Intermediate product      */
EXTENDED_FLOAT fxadd;                   /* Addend in extended format */
EXTENDED_FLOAT fxres;                   /* Extended result           */

    RRD(inst, regs, r1, r2, r3);

    HFPREG2_CHECK(r2, r3, regs);
    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate product */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fxp1);

    /* Convert Addend to extended format */
    ARCH_DEP(lf_to_ef_unnorm)(&fxadd, &fl1);

    /* Add the addend to the intermediate product */
    ARCH_DEP(add_ef_unnorm)(&fxp1, &fxadd, &fxres);

    /* Place low-order part of result in register */
    ARCH_DEP(store_ef_unnorm_lo)(&fxres, regs->fpr + FPR2I(r1));


} /* end DEF_INST(multiply_add_unnormal_float_long_to_ext_low_reg) */


/*-------------------------------------------------------------------*/
/* B339 MYLR  - Multiply Unnormalized Long to Ext. Low FP Reg. [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_unnormal_float_long_to_ext_low_reg)
{
int          r1, r2, r3;                /* Values of R fields        */
LONG_FLOAT   fl2, fl3;                  /* Multiplier/Multiplicand   */
EXTENDED_FLOAT fx1;                     /* Intermediate result       */

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r2, r3, regs);
    HFPREG_CHECK(r1, regs);

    /* Get the operands */
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate result */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fx1);

    /* Place low-order part of result in register */
    ARCH_DEP(store_ef_unnorm_lo)(&fx1, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_unnormal_float_long_to_ext_low_reg) */


/*-------------------------------------------------------------------*/
/* B33A MAYR  - Multiply and Add Unnorm. Long to Ext. Reg.     [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_reg)
{
int            r1, r2, r3;              /* Values of R fields        */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
LONG_FLOAT     fl1;                     /* Addend                    */
EXTENDED_FLOAT fxp1;                    /* Intermediate product      */
EXTENDED_FLOAT fxadd;                   /* Addend in extended format */
EXTENDED_FLOAT fxres;                   /* Extended result           */

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r2, r3, regs);
    HFPREG_CHECK(r1, regs);

    /* Either the low- or high-numbered register of a pair is valid */

    /* Get the operands */
    get_lf(&fl1, regs->fpr + FPR2I(r1));
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate product */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fxp1);

    /* Convert Addend to extended format */
    ARCH_DEP(lf_to_ef_unnorm)(&fxadd, &fl1);

    /* Add the addend to the intermediate product */
    ARCH_DEP(add_ef_unnorm)(&fxp1, &fxadd, &fxres);

    /* Place result in register */
    r1 &= 13;               /* Convert to the low numbered register */
    ARCH_DEP(store_ef_unnorm)(&fxres, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_add_unnormal_float_long_to_ext_reg) */


/*-------------------------------------------------------------------*/
/* B33B MYR   - Multiply Unnormalized Long to Extended Reg     [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_unnormal_float_long_to_ext_reg)
{
int            r1, r2, r3;              /* Values of R fields        */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
EXTENDED_FLOAT fx1;                     /* Intermediate result       */

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);
    HFPREG2_CHECK(r2, r3, regs);

    /* Get the operands */
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate result */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fx1);

    /* Place result in register */
    ARCH_DEP(store_ef_unnorm)(&fx1, regs->fpr + FPR2I(r1));

} /* DEF_INST(multiply_unnormal_float_long_to_ext_reg) */


/*-------------------------------------------------------------------*/
/* B33C MAYHR - Multiply and Add Unnorm. Long to Ext. High Reg [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_high_reg)
{
int            r1, r2, r3;              /* Values of R fields        */
int            i1;                      /* Index of FP register      */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
LONG_FLOAT     fl1;                     /* Addend                    */
EXTENDED_FLOAT fxp1;                    /* Intermediate product      */
EXTENDED_FLOAT fxadd;                   /* Addend in extended format */
EXTENDED_FLOAT fxres;                   /* Extended result           */

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r2, r3, regs);
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate product */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fxp1);

    /* Convert Addend to extended format */
    ARCH_DEP(lf_to_ef_unnorm)(&fxadd, &fl1);

    /* Add the addend to the intermediate product */
    ARCH_DEP(add_ef_unnorm)(&fxp1, &fxadd, &fxres);

    /* Place high-order part of result in register */
    ARCH_DEP(store_ef_unnorm_hi)(&fxres, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_add_unnormal_float_long_to_ext_high_reg) */


/*-------------------------------------------------------------------*/
/* B33D MYHR   - Multiply Unnormalized Long to Ext. High FP Reg[RRD] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_unnormal_float_long_to_ext_high_reg)
{
int          r1, r2, r3;                /* Values of R fields        */
LONG_FLOAT   fl2, fl3;                  /* Multiplier/Multiplicand   */
EXTENDED_FLOAT fx1;                     /* Intermediate result       */

    RRD(inst, regs, r1, r2, r3);

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);
    HFPREG2_CHECK(r2, r3, regs);

    /* Get the operands */
    get_lf(&fl2, regs->fpr + FPR2I(r2));
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate result */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fx1);

    /* Place high-order part of result in register */
    ARCH_DEP(store_ef_unnorm_hi)(&fx1, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_unnormal_float_long_to_ext_high_reg) */


/*-------------------------------------------------------------------*/
/* ED38 MAYL  - Multiply and Add Unnorm. Long to Ext. Low FP   [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_low)
{
int            r1, r3;                  /* Values of R fields        */
int            x2;                      /* Index register            */
int            b2;                      /* Base of effective addr    */
VADR           effective_addr2;         /* Effective address         */
int            i1;                      /* Index of FP register      */
LONG_FLOAT     fl2,fl3;                 /* Multiplier/Multiplicand   */
LONG_FLOAT     fl1;                     /* Addend                    */
EXTENDED_FLOAT fxp1;                    /* Intermediate product      */
EXTENDED_FLOAT fxadd;                   /* Addend in extended format */
EXTENDED_FLOAT fxres;                   /* Extended result           */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate product */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fxp1);

    /* Convert Addend to extended format */
    ARCH_DEP(lf_to_ef_unnorm)(&fxadd, &fl1);

    /* Add the addend to the intermediate product */
    ARCH_DEP(add_ef_unnorm)(&fxp1, &fxadd, &fxres);

    /* Place low-order part of result in register */
    ARCH_DEP(store_ef_unnorm_lo)(&fxres, regs->fpr + FPR2I(r1));


} /* end DEF_INST(multiply_add_unnormal_float_long_to_ext_low) */


/*-------------------------------------------------------------------*/
/* ED39 MYL   - Multiply Unnormalized Long to Extended Low FP [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_unnormal_float_long_to_ext_low)
{
int            r1, r3;                  /* Values of R fields        */
int            x2;                      /* Index register            */
int            b2;                      /* Base of effective addr    */
VADR           effective_addr2;         /* Effective address         */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
EXTENDED_FLOAT fx1;                     /* Intermediate result       */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    /* Get the operands */
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate result */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fx1);

    /* Place low-order part of result in register */
    ARCH_DEP(store_ef_unnorm_lo)(&fx1, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_unnormal_float_long_to_ext_low) */


/*-------------------------------------------------------------------*/
/* ED3A MAY   - Multiply and Add Unnorm. Long to Extended FP   [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_unnormal_float_long_to_ext)
{
int            r1, r3;                  /* Values of R fields        */
int            x2;                      /* Index register            */
int            b2;                      /* Base of effective addr    */
VADR           effective_addr2;         /* Effective address         */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
LONG_FLOAT     fl1;                     /* Addend                    */
EXTENDED_FLOAT fxp1;                    /* Intermediate product      */
EXTENDED_FLOAT fxadd;                   /* Addend in extended format */
EXTENDED_FLOAT fxres;                   /* Extended result           */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    /* Either the low- or high-numbered register of a pair is valid */

    /* Get the operands */
    get_lf(&fl1, regs->fpr + FPR2I(r1));
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate product */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fxp1);

    /* Convert Addend to extended format */
    ARCH_DEP(lf_to_ef_unnorm)(&fxadd, &fl1);

    /* Add the addend to the intermediate product */
    ARCH_DEP(add_ef_unnorm)(&fxp1, &fxadd, &fxres);

    /* Place result in register */
    r1 &= 13;               /* Convert to the low numbered register */
    ARCH_DEP(store_ef_unnorm)(&fxres, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_add_unnormal_float_long_to_ext) */


/*-------------------------------------------------------------------*/
/* ED3B MY    - Multiply Unnormalized Long to Extended FP      [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_unnormal_float_long_to_ext)
{
int            r1, r3;                  /* Values of R fields        */
int            x2;                      /* Index register            */
int            b2;                      /* Base of effective addr    */
VADR           effective_addr2;         /* Effective address         */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
EXTENDED_FLOAT fx1;                     /* Intermediate result       */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPODD_CHECK(r1, regs);
    HFPREG_CHECK(r3, regs);

    /* Get the operands */
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate result */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fx1);

    /* Place result in register */
    ARCH_DEP(store_ef_unnorm)(&fx1, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_unnormal_float_long_to_ext) */


/*-------------------------------------------------------------------*/
/* ED3C MAYH  - Multiply and Add Unnorm. Long to Extended High [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_high)
{
int            r1, r3;                  /* Values of R fields        */
int            x2;                      /* Index register            */
int            b2;                      /* Base of effective addr    */
VADR           effective_addr2;         /* Effective address         */
int            i1;                      /* Index of FP register      */
LONG_FLOAT     fl2, fl3;                /* Multiplier/Multiplicand   */
LONG_FLOAT     fl1;                     /* Addend                    */
EXTENDED_FLOAT fxp1;                    /* Intermediate product      */
EXTENDED_FLOAT fxadd;                   /* Addend in extended format */
EXTENDED_FLOAT fxres;                   /* Extended result           */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    i1 = FPR2I(r1);

    /* Get the operands */
    get_lf(&fl1, regs->fpr + i1);
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate product */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fxp1);

    /* Convert Addend to extended format */
    ARCH_DEP(lf_to_ef_unnorm)(&fxadd, &fl1);

    /* Add the addend to the intermediate product */
    ARCH_DEP(add_ef_unnorm)(&fxp1, &fxadd, &fxres);

    /* Place high-order part of result in register */
    ARCH_DEP(store_ef_unnorm_hi)(&fxres, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_add_unnormal_float_long_to_ext_high) */


/*-------------------------------------------------------------------*/
/* ED3D MYH   - Multiply Unnormalized Long to Extended High FP [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_unnormal_float_long_to_ext_high)
{
int          r1, r3;                    /* Values of R fields        */
int          x2;                        /* Index register            */
int          b2;                        /* Base of effective addr    */
VADR         effective_addr2;           /* Effective address         */
LONG_FLOAT   fl2, fl3;                  /* Multiplier/Multiplicand   */
EXTENDED_FLOAT fx1;                     /* Intermediate result       */

    RXF(inst, regs, r1, r3, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    HFPREG2_CHECK(r1, r3, regs);

    /* Get the operands */
    vfetch_lf(&fl2, effective_addr2, b2, regs );
    get_lf(&fl3, regs->fpr + FPR2I(r3));

    /* Calculate intermediate result */
    ARCH_DEP(mul_lf_to_ef_unnorm)(&fl2, &fl3, &fx1);

    /* Place high-order part of result in register */
    ARCH_DEP(store_ef_unnorm_hi)(&fx1, regs->fpr + FPR2I(r1));

} /* end DEF_INST(multiply_unnormal_float_long_to_ext_high) */

#endif /*defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY)*/


#if defined(FEATURE_018_LONG_DISPL_INST_FACILITY)
/*-------------------------------------------------------------------*/
/* ED64 LEY   - Load Floating Point Short (Long Disp.)       [RXY-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_float_short_y)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Update first 32 bits of register from operand address */
    regs->fpr[FPR2I(r1)] = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);
}


/*-------------------------------------------------------------------*/
/* ED65 LDY   - Load Floating Point Long (Long Displacement) [RXY-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_float_long_y)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of r1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Double word workarea      */

    RXY(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Fetch value from operand address */
    dreg = ARCH_DEP(vfetch8) (effective_addr2, b2, regs);

    /* Update register contents */
    regs->fpr[i1] = dreg >> 32;
    regs->fpr[i1+1] = dreg;
}


/*-------------------------------------------------------------------*/
/* ED66 STEY  - Store Floating Point Short (Long Disp.)      [RXY-a] */
/*-------------------------------------------------------------------*/
DEF_INST(store_float_short_y)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    /* Store register contents at operand address */
    ARCH_DEP(vstore4) (regs->fpr[FPR2I(r1)], effective_addr2, b2, regs);
}


/*-------------------------------------------------------------------*/
/* ED67 STDY  - Store Floating Point Long (Long Disp.)       [RXY-a] */
/*-------------------------------------------------------------------*/
DEF_INST(store_float_long_y)
{
int     r1;                             /* Value of R field          */
int     i1;                             /* Index of r1 in fpr array  */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Double word workarea      */

    RXY(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    HFPREG_CHECK(r1, regs);

    i1 = FPR2I(r1);

    /* Store register contents at operand address */
    dreg = ((U64)regs->fpr[i1] << 32)
         | regs->fpr[i1+1];
    ARCH_DEP(vstore8) (dreg, effective_addr2, b2, regs);
}
#endif /*defined(FEATURE_018_LONG_DISPL_INST_FACILITY)*/


#endif /* FEATURE_HEXADECIMAL_FLOATING_POINT */

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "float.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "float.c"
#endif

#endif /*!defined(_GEN_ARCH)*/


/* end of float.c */
