/* ZVECTOR3.C   (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              z/Arch Vector Operations                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*
 * z/Architecture Principles of Operation (SA22-7832-10 onwards)
 * Chapter 24. Vector Floating-Point Instructions
 */

/*-------------------------------------------------------------------*/
/* This module implements ESA/390 Binary Floating-Point (IEEE 754)   */
/* instructions as described in SA22-7201-05 ESA/390 Principles of   */
/* Operation and SA22-7832-10 z/Architecture Principles of Operation.*/
/*-------------------------------------------------------------------*/
/* This module also implements z/Architecture Vector Floating-Point  */
/* Instructions as described in SA22-7932-10 and later versions of   */
/* z/Architecture Principles of Operation.                           */
/*-------------------------------------------------------------------*/
/*
 * Hercules System/370, ESA/390, z/Architecture emulator zvector3.c
 * Binary (IEEE) Floating Point Instructions
 * Copyright (c) 2001-2009 Willem Konynenberg <wfk@xos.nl>
 * TCEB, TCDB and TCXB contributed by Per Jessen, 20 September 2001.
 * THDER,THDR by Roger Bowler, 19 July 2003.
 * Additional instructions by Roger Bowler, November 2004:
 *  LXDBR,LXDB,LXEBR,LXEB,LDXBR,LEXBR,CXFBR,CXGBR,CFXBR,CGXBR,
 *  MXDBR,MXDB,MDEBR,MDEB,MADBR,MADB,MAEBR,MAEB,MSDBR,MSDB,
 *  MSEBR,MSEB,DIEBR,DIDBR,TBEDR,TBDR.
 * Based very loosely on float.c by Peter Kuschnerus, (c) 2000-2006.
 * All instructions (except convert to/from HFP/BFP format THDR, THDER,
 *  TBDR and TBEDR) completely updated by "Fish" (David B. Trout)
 *  Aug 2011 to use SoftFloat Floating-Point package by John R. Hauser
 *  (http://www.jhauser.us/arithmetic/SoftFloat.html).
 * June 2016: All instructions (except convert to/from HFP/BFP and
 *  Load Positive/Negative/Complement) completely updated by Stephen
 *  R. Orso to use the updated Softfloat 3a library by John R. Hauser
 *  (link above).  Added interpretation of M3 and M4 operands for
 *  those instructions that support same, conditioned on
 *  FEATURE_037_FP_EXTENSION_FACILITY.  All changes are based
 *  on the -10 edition of the z/Architecture Principles of Operation,
 *  SA22-7832.
 * Spring 2024: z/Architecture Vector Floating-Point Instructions
 *  added based on the -13 edition of the z/Architecture
 *  Principles of Operation, SA22-7832.
 */
/*--------------------------------------------------------------------------*/
/* Modifications to the Softfloat interface enable use of a separately-     */
/* packaged Softfloat Library with minimal modifications.                   */
/*                                                                          */
/* Modifications required to Softfloat:  (so far)                           */
/* - Change NaN propagation in the following routines to conform to IBM     */
/*   NaN propagation rules:                                                 */
/*      softfloat_propagateNaNF32UI()                                       */
/*      softfloat_propagateNaNF64UI()                                       */
/*      softfloat_propagateNaNF128UI()                                      */
/* - Change the default NaNs defined in softfloat-specialize.h from         */
/*   negative NaNs to positive NaNs.                                        */
/*   Change init_detectTininess from softfloat_tininess_afterRounding       */
/*   to softfloat_tininess_beforeRounding in softfloat-specialize.h, as     */
/*   required by SA22-7832-10 page 9-22.                                    */
/* - Change the following Softfloat global state variables in               */
/*   softfloat-state.c to include the __thread attribute to enable          */
/*   state separation when multiple CPU threads are running.  Make the      */
/*   same change for these variables in softfloat.h                         */
/*      softfloat_roundingMode                                              */
/*      softfloat_detectTininess                                            */
/*      softfloat_exceptionFlags                                            */
/* - Expose the "unbounded exponent results" during round and pack          */
/*   operations within Softfloat as part of the global state variables.     */
/*   This enables correct scaling of results on trappable overflow and      */
/*   underflow operations.  Affected routines:                              */
/*       softfloat_roundPackToF32()                                         */
/*       softfloat_roundPackToF64()                                         */
/*       softfloat_roundPackToF128()                                        */
/* - Add flags softfloat_flag_incremented and softfloat_flag_tiny.  Raise   */
/*   incremented when rounding increases the value of the rounded result.   */
/*   Raise tiny whenever the result is tiny, whether exact or inexact.      */
/*   Affected routines:                                                     */
/*       softfloat_roundPackToF32()                                         */
/*       softfloat_roundPackToF64()                                         */
/*       softfloat_roundPackToF128()                                        */
/*   Enable rounding mode "softfloat_round_stickybit" which corresponds     */
/*   to the IBM rounding mode Round For Shorter Precision.  The following   */
/*   routines are affected:                                                 */
/*      softfloat_roundPackToF32()                                          */
/*      softfloat_roundPackToF64()                                          */
/*      softfloat_roundPackToF128()                                         */
/*      softfloat_roundPackToI32()                                          */
/*      softfloat_roundPackToI64()                                          */
/*      softfloat_roundPackToUI32()                                         */
/*      softfloat_roundPackToUI64()                                         */
/*      f32_roundToInt()                                                    */
/*      f64_roundToInt()                                                    */
/*      f128_roundToInt()                                                   */
/*      softfloat.h  (header)                                               */
/*   Return 0 instead of max uint-32 (or max uint-64) in fxxx_to_uixx()     */
/*   IBM requires zero returned per Table 19-19 in SA22-7832-10 p. 19-26.   */
/*   Affected routines:                                                     */
/*      softfloat_roundPackToUI32()                                         */
/*      softfloat_roundPackToUI64()                                         */
/*                                                                          */
/*   These modifications, and the unmodified Softfloat 3a source, are       */
/*   maintained in a separate public repository                             */
/*--------------------------------------------------------------------------*/

#include "hstdinc.h"

#define _ZVECTOR3_C_
#define _HENGINE_DLL_

#if !defined( _MSVC_ ) && !defined( _GNU_SOURCE )
#error '_GNU_SOURCE' required for zvector3.c!
#endif

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#include "zvector.h"

#if defined( FEATURE_BINARY_FLOATING_POINT )

#if defined( FEATURE_129_ZVECTOR_FACILITY )

#if !defined( _IEEE_NONARCHDEP_ )


    /* Architecture independent code goes within this ifdef */



/*****************************************************************************/
/*                                                                           */
/*                       ---  B E G I N  ---                                 */
/*                                                                           */
/*           'SoftFloat' IEEE Binary Floating Point package                  */
/*                                                                           */
/*****************************************************************************/
/* PROGRAMMING NOTE: the following three defines for SOFTFLOAT_FAST_INT64,   */
/* SOFTFLOAT_FAST_DIV64TO32 and LITTLEENDIAN *must* match the values that    */
/* were used to build the SoftFloat static libraries you are linking with.   */
/*****************************************************************************/
#define SOFTFLOAT_FAST_INT64        /* Hercules SoftFloat requirement        */
#define SOFTFLOAT_FAST_DIV64TO32    /* Hercules SoftFloat requirement        */
#undef  LITTLEENDIAN                /* (in case it's already #defined)       */
#if !defined( WORDS_BIGENDIAN )     /* NOT building for BIG endian platform? */
  #define LITTLEENDIAN              /* Then #define LITTLEENDIAN macro       */
#endif                              /* endif !defined( WORDS_BIGENDIAN )     */

/* ("arm_neon.h" type-conflict workaround...) */
#if defined( __aarch64__ )
#undef  float16_t
#undef  float32_t
#undef  float64_t
#undef  float128_t

#define float16_t   sfloat16_t
#define float32_t   sfloat32_t
#define float64_t   sfloat64_t
#define float128_t  sfloat128_t
#endif

#include "softfloat.h"              /* Master SoftFloat #include header      */

/*****************************************************************************/

/* Map of IBM M3 rounding mode values to those used by Softfloat */
static const BYTE map_m3_to_sf_rm[8] =
{
    0,                              /* M3 0: Use FPC BFP Rounding Mode  */
    softfloat_round_near_maxMag,    /* M3 1: RNTA                       */
    0,                              /* M3 2: invalid; detected in edits */
    softfloat_round_stickybit,      /* M3 3: RFS,                       */
    softfloat_round_near_even,      /* M3 4: RNTE                       */
    softfloat_round_minMag,         /* M3 5: RZ                         */
    softfloat_round_max,            /* M3 6: RP                         */
    softfloat_round_min,            /* M3 7: RM                         */
};


/* Map of IBM fpc BFP rounding mode values to those used by Softfloat   */
/* This table depends on FPS Support instructions to set the BFP        */
/* rounding mode to only valid values                                   */
static const BYTE map_fpc_brm_to_sf_rm[8] =
{
    softfloat_round_near_even,      /* FPC BRM 0: RNTE                  */
    softfloat_round_minMag,         /* FPC BRM 5: RZ                    */
    softfloat_round_max,            /* FPC BRM 6: RP                    */
    softfloat_round_min,            /* FPC BRM 6: RM                    */
    0,                              /* FPC BRM 4: invalid               */
    0,                              /* FPC BRM 5: invalid               */
    0,                              /* FPC BRM 6: invalid               */
    softfloat_round_stickybit,      /* FPC BRM 7: RFS                   */
};

#define SUPPRESS_INEXACT(_m4)  (_m4 & 0x04)

/* Scaling factors when used when trappable overflow or underflow       */
/* exceptions occur. Factors taken from Figure 19-8 (part 2) on page    */
/* 19-9 of SA22-7832-10. Scaling factors reduce the exponent to fit     */
/* in the target precision on overflow and increase them on underflow.  */

#define SCALE_FACTOR_ARITH_OFLOW_SHORT    -192
#define SCALE_FACTOR_ARITH_OFLOW_LONG    -1536
#define SCALE_FACTOR_ARITH_OFLOW_EXTD   -24576

#define SCALE_FACTOR_ARITH_UFLOW_SHORT     192
#define SCALE_FACTOR_ARITH_UFLOW_LONG     1536
#define SCALE_FACTOR_ARITH_UFLOW_EXTD    24576

#define SCALE_FACTOR_LOADR_OFLOW_LONG     -512
#define SCALE_FACTOR_LOADR_OFLOW_EXTD    -8192

#define SCALE_FACTOR_LOADR_UFLOW_LONG      512
#define SCALE_FACTOR_LOADR_UFLOW_EXTD     8192

/* In addition to extFloat80M (in softfloat_type.h), also the type      */
/* float128_t is dependent on the endian-ness of the host.  The indexes */
/* of op.v[ ] can therefore not be hardcoded [1] and [0], but must use  */
/* the following macros.  This was verified on a s390x SLES 12 SP5 sys- */
/* tem running on IBM system z15 hardware.                              */

#ifdef LITTLEENDIAN
  #define FLOAT128_HI   1
  #define FLOAT128_LO   0
#else
  #define FLOAT128_HI   0
  #define FLOAT128_LO   1
#endif

/* Identify NaNs  */

#define FLOAT128_ISNAN( _op )   (((_op.v[FLOAT128_HI] & 0x7FFF000000000000ULL) == 0x7FFF000000000000ULL) &&  \
                                  (_op.v[FLOAT128_HI] & 0x0000FFFFFFFFFFFFULL || _op.v[FLOAT128_LO] ))

#define FLOAT64_ISNAN( _op )    (((_op.v              & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) &&  \
                                  (_op.v              & 0x000FFFFFFFFFFFFFULL) )

#define FLOAT32_ISNAN( _op )    (((_op.v              & 0x7F800000           ) == 0x7F800000           ) &&  \
                                  (_op.v              & 0x007FFFFF))

/* Make SNaNs into QNaNs  */

#define FLOAT128_MAKE_QNAN( _op )  _op.v[FLOAT128_HI] |= 0x0000800000000000ULL
#define FLOAT64_MAKE_QNAN( _op )   _op.v              |= 0x0008000000000000ULL
#define FLOAT32_MAKE_QNAN( _op )   _op.v              |= 0x00400000

/*------------------------------------------------------------------------------*/
/* z/Architecture Floating-Point classes (for "Test Data Class" instruction)    */
/*                                                                              */
/* Values taken from SA22-7832-10, Table 19-21 on page 19-41                    */
/*                                                                              */
/* N.B.  These values *MUST* match Figure 19-21 because these values are        */
/*       returned as is as the results of the Test Data Class family of         */
/*       instructions                                                           */
/*------------------------------------------------------------------------------*/
enum
{
    float_class_pos_zero            = 0x00000800,
    float_class_neg_zero            = 0x00000400,
    float_class_pos_normal          = 0x00000200,
    float_class_neg_normal          = 0x00000100,
    float_class_pos_subnormal       = 0x00000080,
    float_class_neg_subnormal       = 0x00000040,
    float_class_pos_infinity        = 0x00000020,
    float_class_neg_infinity        = 0x00000010,
    float_class_pos_quiet_nan       = 0x00000008,
    float_class_neg_quiet_nan       = 0x00000004,
    float_class_pos_signaling_nan   = 0x00000002,
    float_class_neg_signaling_nan   = 0x00000001
};

static INLINE U32 float128_class( float128_t op )
{
    int neg = (op.v[FLOAT128_HI] & 0x8000000000000000ULL ) ? 1 : 0;

    if (f128_isSignalingNaN( op ))                                           {return float_class_pos_signaling_nan >> neg;}
    if (FLOAT128_ISNAN( op ))                                                {return float_class_pos_quiet_nan     >> neg;}
    if (!(op.v[FLOAT128_HI] & 0x7FFFFFFFFFFFFFFFULL ) && !op.v[FLOAT128_LO]) {return float_class_pos_zero          >> neg;}
    if ( (op.v[FLOAT128_HI] & 0x7FFFFFFFFFFFFFFFULL )
                           == 0x7FFF000000000000ULL   && !op.v[FLOAT128_LO]) {return float_class_pos_infinity      >> neg;}
    if (  op.v[FLOAT128_HI] & 0x7FFF000000000000ULL )                        {return float_class_pos_normal        >> neg;}
                                                                              return float_class_pos_subnormal     >> neg;
}

static INLINE U32 float64_class( float64_t op )
{
    int neg = (op.v & 0x8000000000000000ULL ) ? 1 : 0;

    if (f64_isSignalingNaN( op ))           {return float_class_pos_signaling_nan >> neg;}
    if (FLOAT64_ISNAN( op ))                {return float_class_pos_quiet_nan     >> neg;}
    if (!(op.v & 0x7FFFFFFFFFFFFFFFULL ))   {return float_class_pos_zero          >> neg;}
    if ( (op.v & 0x7FFFFFFFFFFFFFFFULL )
              == 0x7FF0000000000000ULL )    {return float_class_pos_infinity      >> neg;}
    if (  op.v & 0x7FF0000000000000ULL )    {return float_class_pos_normal        >> neg;}
                                             return float_class_pos_subnormal     >> neg;
}

static INLINE U32 float32_class( float32_t op )
{
    int neg = (op.v & 0x80000000) ? 1 : 0;

    if (f32_isSignalingNaN( op ))   {return float_class_pos_signaling_nan >> neg;}
    if (FLOAT32_ISNAN( op ))        {return float_class_pos_quiet_nan     >> neg;}
    if (!(op.v & 0x7FFFFFFF))       {return float_class_pos_zero          >> neg;}
    if ( (op.v & 0x7FFFFFFF)
              == 0x7F800000)        {return float_class_pos_infinity      >> neg;}
    if (  op.v & 0x7F800000)        {return float_class_pos_normal        >> neg;}
                                     return float_class_pos_subnormal     >> neg;
}

/***********************************************************************/
/*                 TAKE NOTE, TAKE NOTE!                               */
/*                                                                     */
/* Softfloat architecture dependant: softfloat_exceptionFlags must use */
/* the same bit pattern as FPC Flags                                   */
/*                                                                     */
/***********************************************************************/

/* Set FPCR IEEE flags from the softfloat_exceptionFlags               */
/* Flags are set only if the corresponding mask is set to non-trap.    */

#define SET_FPC_FLAGS_FROM_SF( regs )   regs->fpc |=                    \
                                                                        \
    /* Align softfloat flags with flags in FPCR */                      \
    (((U32)softfloat_exceptionFlags) << FPC_FLAG_SHIFT) &               \
                                                                        \
    /* ..and suppress those that could trap */                          \
    ~(regs->fpc >> 8) & FPC_FLAGS;

/* ****     End of Softfloat architecture-dependent code          **** */

/* Translate FPC rounding mode into matching Softfloat rounding mode */
#define SET_SF_RM_FROM_FPC                                              \
                                                                        \
    softfloat_roundingMode = map_fpc_brm_to_sf_rm[ (regs->fpc & FPC_BRM_3BIT) ]

/* Translate M3 or M4 rounding mode into matching Softfloat rounding
   mode. Use FPC rounding mode if M3 or M4 is zero.
*/
#define SET_SF_RM_FROM_MASK( _mask )                                 \
                                                                     \
    softfloat_roundingMode = _mask ?                                 \
        map_m3_to_sf_rm[_mask]                                       \
      : map_fpc_brm_to_sf_rm[ (regs->fpc & FPC_BRM_3BIT) ]

/* Save detected exceptions that are trap-enabled                      */

#define IEEE_EXCEPTION_TEST_TRAPS( _regs, _ieee_trap_conds, _exceptions )   \
                                                                            \
      _ieee_trap_conds = (_regs->fpc & FPC_MASKS) & (((U32)softfloat_exceptionFlags) << FPC_MASK_SHIFT) & (_exceptions)


/* Test FPC against Softfloat execptions; return field whose bits    */
/* identify those exceptions that a) were reported by Softfloat,     */
/* and b) are enabled for trapping by FPC byte zero. Only overflow,  */
/* underflow, and inexact are tested; invalid and divide by zero     */
/* are handled separately.                                           */

static INLINE U32 ieee_exception_test_oux( REGS* regs )
{
    U32 ieee_trap_conds = 0;

    if (regs->fpc & FPC_MASKS)
    {
        /* Some flags and some traps enabled. Figure it out. */

        /* If tiny and underflow trappable... */
        if (1
            && (softfloat_exceptionFlags & softfloat_flag_tiny)
            && regs->fpc & FPC_MASK_IMU
        )
            /* Raise underflow per SA22-7832-10 page 9-20 */
            softfloat_exceptionFlags |= softfloat_flag_underflow;

        IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        /* Transfer any returned flags from Softfloat to FPC */
        SET_FPC_FLAGS_FROM_SF( regs );

        /* Turn off inexact flag if overflow or underflow will trap */
        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            regs->fpc &= ~FPC_FLAG_SFX;
    }
    else
        /* Transfer any returned flags from Softfloat to FPC */
        SET_FPC_FLAGS_FROM_SF( regs );

    return ieee_trap_conds;
}

/*                          ---  E N D  ---                                  */
/*                                                                           */
/*           'SoftFloat' IEEE Binary Floating Point package                  */
/*****************************************************************************/

struct lbfp
{
    int     sign;
    int     exp;
    U64     fract;
    double  v;
};

struct sbfp
{
    int     sign;
    int     exp;
    int     fract;
    float   v;
};

#endif /* !defined( _IEEE_NONARCHDEP_ ) */

/*****************************************************************************/
/*                       ---  B E G I N  ---                                 */
/*                                                                           */
/*           'SoftFloat' IEEE Binary Floating Point package                  */

static INLINE void ARCH_DEP( get_float128 )( float128_t *op, U64 *fh, U64 *fl )
{
    op->v[FLOAT128_HI] = *fh;
    op->v[FLOAT128_LO] = *fl;
}

static INLINE void ARCH_DEP( put_float128 )( float128_t *op, U64 *fh, U64 *fl )
{
    *fh = op->v[FLOAT128_HI];
    *fl = op->v[FLOAT128_LO];
}

static INLINE void ARCH_DEP( get_float64 )( float64_t *op, U64 *f )
{
    op->v = *f;
}

static INLINE void ARCH_DEP( put_float64 )( float64_t *op, U64 *f )
{
    *f = op->v;
}

static INLINE void ARCH_DEP( get_float32 )( float32_t *op, U32 *f )
{
    op->v = *f;
}

static INLINE void ARCH_DEP( put_float32 )( float32_t *op, U32 *f )
{
    *f = op->v;
}

#undef GET_FLOAT128_OP
#undef GET_FLOAT64_OP
#undef GET_FLOAT32_OP

#define GET_FLOAT128_OP( op, r, regs )  ARCH_DEP( get_float128 )( &op, &regs->FPR_L( r ), &regs->FPR_L( r+2 ))
#define GET_FLOAT64_OP(  op, r, regs )  ARCH_DEP( get_float64  )( &op, &regs->FPR_L( r ))
#define GET_FLOAT32_OP(  op, r, regs )  ARCH_DEP( get_float32  )( &op, &regs->FPR_S( r ))

static INLINE BYTE ARCH_DEP( float128_cc_quiet )( float128_t op1, float128_t op2 )
{
    return FLOAT128_ISNAN( op1      ) ||
           FLOAT128_ISNAN(      op2 ) ? 3 :
           f128_eq(        op1, op2 ) ? 0 :
           f128_lt_quiet(  op1, op2 ) ? 1 : 2;
}

static INLINE BYTE ARCH_DEP( float128_compare )( float128_t op1, float128_t op2 )
{
    if (f128_isSignalingNaN( op1 ) ||
        f128_isSignalingNaN( op2 ))
        softfloat_raiseFlags( softfloat_flag_invalid );
    return ARCH_DEP( float128_cc_quiet )( op1, op2 );
}

static INLINE BYTE ARCH_DEP( float64_cc_quiet )( float64_t op1, float64_t op2 )
{
    return FLOAT64_ISNAN( op1      ) ||
           FLOAT64_ISNAN(      op2 ) ? 3 :
           f64_eq(        op1, op2 ) ? 0 :
           f64_lt_quiet(  op1, op2 ) ? 1 : 2;
}

static INLINE BYTE ARCH_DEP( float64_compare )( float64_t op1, float64_t op2 )
{
    if (f64_isSignalingNaN( op1 ) ||
        f64_isSignalingNaN( op2 ))
        softfloat_raiseFlags( softfloat_flag_invalid );
    return ARCH_DEP( float64_cc_quiet )( op1, op2 );
}

static INLINE BYTE ARCH_DEP( float32_cc_quiet )( float32_t op1, float32_t op2 )
{
    return FLOAT32_ISNAN( op1      ) ||
           FLOAT32_ISNAN(      op2 ) ? 3 :
           f32_eq(        op1, op2 ) ? 0 :
           f32_lt_quiet(  op1, op2 ) ? 1 : 2;
}

static INLINE BYTE ARCH_DEP( float32_compare )( float32_t op1, float32_t op2 )
{
    if (f32_isSignalingNaN( op1 ) ||
        f32_isSignalingNaN( op2 ))
        softfloat_raiseFlags( softfloat_flag_invalid );
    return ARCH_DEP( float32_cc_quiet )( op1, op2 );
}

#undef FLOAT128_COMPARE
#undef FLOAT64_COMPARE
#undef FLOAT32_COMPARE

#define FLOAT128_COMPARE( op1, op2 )  ARCH_DEP( float128_compare )( op1, op2 )
#define FLOAT64_COMPARE(  op1, op2 )  ARCH_DEP( float64_compare  )( op1, op2 )
#define FLOAT32_COMPARE(  op1, op2 )  ARCH_DEP( float32_compare  )( op1, op2 )

#undef PUT_FLOAT128_NOCC
#undef PUT_FLOAT64_NOCC
#undef PUT_FLOAT32_NOCC

#define PUT_FLOAT128_NOCC( op, r, regs )  ARCH_DEP( put_float128 )( &op, &regs->FPR_L( r ), &regs->FPR_L( r+2 ))
#define PUT_FLOAT64_NOCC(  op, r, regs )  ARCH_DEP( put_float64  )( &op, &regs->FPR_L( r ))
#define PUT_FLOAT32_NOCC(  op, r, regs )  ARCH_DEP( put_float32  )( &op, &regs->FPR_S( r ))


/*                          ---  E N D  ---                                  */
/*                                                                           */
/*           'SoftFloat' IEEE Binary Floating Point package                  */
/*****************************************************************************/

#if !defined( _IEEE_NONARCHDEP_ )

//  #if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*
 * Calculate result for a pair of data classes
 */
static int calculate_result_one( U32 op2_dataclass, U32 op3_dataclass )
{
    int result;

    if (op2_dataclass & ( float_class_pos_normal    |
                          float_class_pos_subnormal ))
        result = 40;
    else if (op2_dataclass & ( float_class_neg_normal    |
                               float_class_neg_subnormal ))
        result = 10;
    else if (op2_dataclass & ( float_class_pos_zero ))
        result = 30;
    else if (op2_dataclass & ( float_class_neg_zero ))
        result = 20;
    else if (op2_dataclass & ( float_class_pos_infinity ))
        result = 50;
    else if (op2_dataclass & ( float_class_neg_infinity ))
        result = 0;
    else if (op2_dataclass & ( float_class_pos_quiet_nan |
                               float_class_neg_quiet_nan ))
        result = 60;
    else /* if (op2_dataclass & ( float_class_pos_signaling_nan |  */
         /*                       float_class_neg_signaling_nan )) */
        result = 70;

    if (op3_dataclass & ( float_class_pos_normal    |
                          float_class_pos_subnormal ))
        result += 4;
    else if (op3_dataclass & ( float_class_neg_normal    |
                               float_class_neg_subnormal ))
        result += 1;
    else if (op3_dataclass & ( float_class_pos_zero ))
        result += 3;
    else if (op3_dataclass & ( float_class_neg_zero ))
        result += 2;
    else if (op3_dataclass & ( float_class_pos_infinity ))
        result += 5;
    else if (op3_dataclass & ( float_class_neg_infinity ))
        result += 0;
    else if (op3_dataclass & ( float_class_pos_quiet_nan |
                               float_class_neg_quiet_nan ))
        result += 6;
    else /* if (op3_dataclass & ( float_class_pos_signaling_nan |  */
         /*                       float_class_neg_signaling_nan )) */
        result += 7;

    return result;
}

/*
 * Convert result for IEEE MaxNum
 */
static int convert_result_ieee_maxnum( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 6:   // op2 = -inf  op3 = QNaN
    case 16:  // op2 = -Fn   op3 = QNaN
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 22:  // op2 = -0    op3 = -0
    case 26:  // op2 = -0    op3 = QNaN
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 36:  // op2 = +0    op3 = QNaN
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 46:  // op2 = +Fn   op3 = QNaN
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 55:  // op2 = +inf  op3 = +inf
    case 56:  // op2 = +inf  op3 = QNaN
    case 60:  // op2 = QNaN  op3 = -inf
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 10:  // op2 = -Fn   op3 = -inf
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for IEEE MinNum
 */
static int convert_result_ieee_minnum( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 6:   // op2 = -inf  op3 = QNaN
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 16:  // op2 = -Fn   op3 = QNaN
    case 22:  // op2 = -0    op3 = -0
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 26:  // op2 = -0    op3 = QNaN
    case 33:  // op2 = +0    op3 = +0
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 36:  // op2 = +0    op3 = QNaN
    case 45:  // op2 = +Fn   op3 = +inf
    case 46:  // op2 = +Fn   op3 = QNaN
    case 55:  // op2 = +inf  op3 = +inf
    case 56:  // op2 = +inf  op3 = QNaN
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 10:  // op2 = -Fn   op3 = -inf
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 77:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for JAVA Math.Max()
 */
static int convert_result_java_max( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 10:  // op2 = -Fn   op3 = -inf
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 22:  // op2 = -0    op3 = -0
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 55:  // op2 = +inf  op3 = +inf
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 6:   // op2 = -inf  op3 = QNaN
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 16:  // op2 = -Fn   op3 = QNaN
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 26:  // op2 = -0    op3 = QNaN
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 36:  // op2 = +0    op3 = QNaN
    case 45:  // op2 = +Fn   op3 = +inf
    case 46:  // op2 = +Fn   op3 = QNaN
    case 56:  // op2 = +inf  op3 = QNaN
        result_out = 3;  // T(op3)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for JAVA Math.Min()
 */
static int convert_result_java_min( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 6:   // op2 = -inf  op3 = QNaN
    case 10:  // op2 = -Fn   op3 = -inf
    case 16:  // op2 = -Fn   op3 = QNaN
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 22:  // op2 = -0    op3 = -0
    case 26:  // op2 = -0    op3 = QNaN
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 36:  // op2 = +0    op3 = QNaN
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 46:  // op2 = +Fn   op3 = QNaN
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 55:  // op2 = +inf  op3 = +inf
    case 56:  // op2 = +inf  op3 = QNaN
        result_out = 3;  // T(op3)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 77:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for C-style Max Macro
 */
static int convert_result_cee_max( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 10:  // op2 = -Fn   op3 = -inf
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
        result_out = 2;  // T(op2)
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 22:  // op2 = -0    op3 = -0
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 55:  // op2 = +inf  op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 6:   // op2 = -inf  op3 = QNaN
    case 7:   // op2 = -inf  op3 = SNaN
    case 16:  // op2 = -Fn   op3 = QNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 26:  // op2 = -0    op3 = QNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 36:  // op2 = +0    op3 = QNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 46:  // op2 = +Fn   op3 = QNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 56:  // op2 = +inf  op3 = QNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
    case 66:  // op2 = QNaN  op3 = QNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for C-style Min Macro
 */
static int convert_result_cee_min( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 55:  // op2 = +inf  op3 = +inf
        result_out = 2;  // T(op2)
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 10:  // op2 = -Fn   op3 = -inf
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 22:  // op2 = -0    op3 = -0
    case 23:  // op2 = -0    op3 = +0
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
        result_out = 3;  // T(op3)
        break;
    case 6:   // op2 = -inf  op3 = QNaN
    case 7:   // op2 = -inf  op3 = SNaN
    case 16:  // op2 = -Fn   op3 = QNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 26:  // op2 = -0    op3 = QNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 36:  // op2 = +0    op3 = QNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 46:  // op2 = +Fn   op3 = QNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 56:  // op2 = +inf  op3 = QNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
    case 66:  // op2 = QNaN  op3 = QNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for C++ algorithm.max()
 */
static int convert_result_cpp_max( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 10:  // op2 = -Fn   op3 = -inf
    case 12:  // op2 = -Fn   op3 = +0
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 22:  // op2 = -0    op3 = -0
    case 23:  // op2 = -0    op3 = +0
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 34:  // op2 = +0    op3 = +Fn
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 55:  // op2 = +inf  op3 = +inf
        result_out = 2;  // T(op2)
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 35:  // op2 = +0    op3 = +inf
    case 45:  // op2 = +Fn   op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 6:   // op2 = -inf  op3 = QNaN
    case 7:   // op2 = -inf  op3 = SNaN
    case 16:  // op2 = -Fn   op3 = QNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 26:  // op2 = -0    op3 = QNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 36:  // op2 = +0    op3 = QNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 46:  // op2 = +Fn   op3 = QNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 56:  // op2 = +inf  op3 = QNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
    case 66:  // op2 = QNaN  op3 = QNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 4;  // XI: T(op2)
        break;
    }

    return result_out;
}

/*
 * Convert result for C++ algorithm.min()
 */
static int convert_result_cpp_min( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 22:  // op2 = -0    op3 = -0
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 55:  // op2 = +inf  op3 = +inf
        result_out = 2;  // T(op2)
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 10:  // op2 = -Fn   op3 = -inf
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
        result_out = 3;  // T(op3)
        break;
    case 6:   // op2 = -inf  op3 = QNaN
    case 7:   // op2 = -inf  op3 = SNaN
    case 16:  // op2 = -Fn   op3 = QNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 26:  // op2 = -0    op3 = QNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 36:  // op2 = +0    op3 = QNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 46:  // op2 = +Fn   op3 = QNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 56:  // op2 = +inf  op3 = QNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
    case 66:  // op2 = QNaN  op3 = QNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 4;  // XI: T(op2)
        break;
    }

    return result_out;
}

/*
 * Convert result for fmax()
 */
static int convert_result_fmax( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 6:   // op2 = -inf  op3 = QNaN
    case 10:  // op2 = -Fn   op3 = -inf
    case 16:  // op2 = -Fn   op3 = QNaN
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 22:  // op2 = -0    op3 = -0
    case 26:  // op2 = -0    op3 = QNaN
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 36:  // op2 = +0    op3 = QNaN
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 46:  // op2 = +Fn   op3 = QNaN
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 55:  // op2 = +inf  op3 = +inf
    case 56:  // op2 = +inf  op3 = QNaN
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for fmin()
 */
static int convert_result_fmin( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 5:   // op2 = -inf  op3 = +inf
    case 6:   // op2 = -inf  op3 = QNaN
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 14:  // op2 = -Fn   op3 = +Fn
    case 15:  // op2 = -Fn   op3 = +inf
    case 16:  // op2 = -Fn   op3 = QNaN
    case 22:  // op2 = -0    op3 = -0
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 26:  // op2 = -0    op3 = QNaN
    case 33:  // op2 = +0    op3 = +0
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 36:  // op2 = +0    op3 = QNaN
    case 45:  // op2 = +Fn   op3 = +inf
    case 46:  // op2 = +Fn   op3 = QNaN
    case 55:  // op2 = +inf  op3 = +inf
    case 56:  // op2 = +inf  op3 = QNaN
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 10:  // op2 = -Fn   op3 = -inf
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 32:  // op2 = +0    op3 = -0
    case 40:  // op2 = +Fn   op3 = -inf
    case 41:  // op2 = +Fn   op3 = -Fn
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for IEEE MaxNumMag
 */
static int convert_result_ieee_maxnummag( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 14:  // op2 = -Fn   op3 = +Fn
    case 41:  // op2 = +Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 6:   // op2 = -inf  op3 = QNaN
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 16:  // op2 = -Fn   op3 = QNaN
    case 22:  // op2 = -0    op3 = -0
    case 26:  // op2 = -0    op3 = QNaN
    case 32:  // op2 = +0    op3 = -0
    case 33:  // op2 = +0    op3 = +0
    case 36:  // op2 = +0    op3 = QNaN
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 46:  // op2 = +Fn   op3 = QNaN
    case 55:  // op2 = +inf  op3 = +inf
    case 56:  // op2 = +inf  op3 = QNaN
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 5:   // op2 = -inf  op3 = +inf
    case 10:  // op2 = -Fn   op3 = -inf
    case 15:  // op2 = -Fn   op3 = +inf
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 40:  // op2 = +Fn   op3 = -inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for IEEE MinNumMag
 */
static int convert_result_ieee_minnummag( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = -Fn   op3 = -Fn
    case 14:  // op2 = -Fn   op3 = +Fn
    case 41:  // op2 = +Fn   op3 = -Fn
    case 44:  // op2 = +Fn   op3 = +Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 0:   // op2 = -inf  op3 = -inf
    case 5:   // op2 = -inf  op3 = +inf
    case 6:   // op2 = -inf  op3 = QNaN
    case 10:  // op2 = -Fn   op3 = -inf
    case 15:  // op2 = -Fn   op3 = +inf
    case 16:  // op2 = -Fn   op3 = QNaN
    case 20:  // op2 = -0    op3 = -inf
    case 21:  // op2 = -0    op3 = -Fn
    case 22:  // op2 = -0    op3 = -0
    case 23:  // op2 = -0    op3 = +0
    case 24:  // op2 = -0    op3 = +Fn
    case 25:  // op2 = -0    op3 = +inf
    case 26:  // op2 = -0    op3 = QNaN
    case 30:  // op2 = +0    op3 = -inf
    case 31:  // op2 = +0    op3 = -Fn
    case 33:  // op2 = +0    op3 = +0
    case 34:  // op2 = +0    op3 = +Fn
    case 35:  // op2 = +0    op3 = +inf
    case 36:  // op2 = +0    op3 = QNaN
    case 40:  // op2 = +Fn   op3 = -inf
    case 45:  // op2 = +Fn   op3 = +inf
    case 46:  // op2 = +Fn   op3 = QNaN
    case 55:  // op2 = +inf  op3 = +inf
    case 56:  // op2 = +inf  op3 = QNaN
    case 66:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 1:   // op2 = -inf  op3 = -Fn
    case 2:   // op2 = -inf  op3 = -0
    case 3:   // op2 = -inf  op3 = +0
    case 4:   // op2 = -inf  op3 = +Fn
    case 12:  // op2 = -Fn   op3 = +0
    case 13:  // op2 = -Fn   op3 = +0
    case 32:  // op2 = +0    op3 = -0
    case 42:  // op2 = +Fn   op3 = -0
    case 43:  // op2 = +Fn   op3 = +0
    case 50:  // op2 = +inf  op3 = -inf
    case 51:  // op2 = +inf  op3 = -Fn
    case 52:  // op2 = +inf  op3 = -0
    case 53:  // op2 = +inf  op3 = +0
    case 54:  // op2 = +inf  op3 = +Fn
    case 60:  // op2 = QNaN  op3 = -inf
    case 61:  // op2 = QNaN  op3 = -Fn
    case 62:  // op2 = QNaN  op3 = -0
    case 63:  // op2 = QNaN  op3 = +0
    case 64:  // op2 = QNaN  op3 = +Fn
    case 65:  // op2 = QNaN  op3 = +inf
        result_out = 3;  // T(op3)
        break;
    case 70:  // op2 = SNaN  op3 = -inf
    case 71:  // op2 = SNaN  op3 = -Fn
    case 72:  // op2 = SNaN  op3 = -0
    case 73:  // op2 = SNaN  op3 = +0
    case 74:  // op2 = SNaN  op3 = +Fn
    case 75:  // op2 = SNaN  op3 = +inf
    case 76:  // op2 = SNaN  op3 = QNaN
    case 77:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 7:   // op2 = -inf  op3 = SNaN
    case 17:  // op2 = -Fn   op3 = SNaN
    case 27:  // op2 = -0    op3 = SNaN
    case 37:  // op2 = +0    op3 = SNaN
    case 47:  // op2 = +Fn   op3 = SNaN
    case 57:  // op2 = +inf  op3 = SNaN
    case 67:  // op2 = QNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Calculate result for a pair of data classes
 */
static int calculate_result_two( U32 op2_dataclass, U32 op3_dataclass )
{
    int result;

    if (op2_dataclass & ( float_class_pos_normal    |
                          float_class_neg_normal    |
                          float_class_pos_subnormal |
                          float_class_neg_subnormal ))
        result = 10;
    else if (op2_dataclass & ( float_class_pos_zero |
                               float_class_neg_zero ))
        result = 0;
    else if (op2_dataclass & ( float_class_pos_infinity |
                               float_class_neg_infinity ))
        result = 20;
    else if (op2_dataclass & ( float_class_neg_quiet_nan |
                               float_class_pos_quiet_nan ))
        result = 30;
    else /* if (op2_dataclass & ( float_class_pos_signaling_nan |  */
         /*                       float_class_neg_signaling_nan )) */
        result = 40;

    if (op3_dataclass & ( float_class_pos_normal    |
                          float_class_neg_normal    |
                          float_class_pos_subnormal |
                          float_class_neg_subnormal ))
        result += 1;
    else if (op3_dataclass & ( float_class_pos_zero |
                               float_class_neg_zero ))
        result += 0;
    else if (op3_dataclass & ( float_class_pos_infinity |
                               float_class_neg_infinity ))
        result += 2;
    else if (op3_dataclass & ( float_class_neg_quiet_nan |
                               float_class_pos_quiet_nan ))
        result += 3;
    else /* if (op3_dataclass & ( float_class_pos_signaling_nan |  */
         /*                       float_class_neg_signaling_nan )) */
        result += 4;

    return result;
}

/*
 * Convert result for JAVA Math.Max() of absolute value
 */
static int convert_result_java_max_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 00:  // op2 = 0     op3 = 0
    case 10:  // op2 = Fn    op3 = 0
    case 20:  // op2 = inf   op3 = 0
    case 21:  // op2 = inf   op3 = Fn
    case 22:  // op2 = inf   op3 = inf
    case 30:  // op2 = QNaN  op3 = 0
    case 31:  // op2 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
    case 33:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 03:  // op2 = 0     op3 = QNaN
    case 12:  // op2 = Fn    op3 = inf
    case 13:  // op2 = Fn    op3 = QNaN
    case 23:  // op2 = inf   op3 = QNaN
        result_out = 3;  // T(op3)
        break;
    case 40:  // op2 = SNaN  op3 = 0
    case 41:  // op2 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    case 43:  // op2 = SNaN  op3 = QNaN
    case 44:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 04:  // op2 = 0     op3 = SNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 24:  // op2 = inf   op3 = SNaN
    case 34:  // op2 = QNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for JAVA Math.Min() of absolute value
 */
static int convert_result_java_min_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 00:  // op2 = 0     op3 = 0
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 12:  // op2 = Fn    op3 = inf
    case 22:  // op2 = inf   op3 = inf
    case 30:  // op2 = QNaN  op3 = 0
    case 31:  // op2 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
    case 33:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 03:  // op2 = 0     op3 = QNaN
    case 10:  // op2 = Fn    op3 = 0
    case 13:  // op2 = Fn    op3 = QNaN
    case 20:  // op2 = inf   op3 = 0
    case 21:  // op2 = inf   op3 = Fn
    case 23:  // op2 = inf   op3 = QNaN
        result_out = 3;  // T(op3)
        break;
    case 40:  // op2 = SNaN  op3 = 0
    case 41:  // op3 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    case 43:  // op2 = SNaN  op3 = QNaN
    case 44:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 04:  // op2 = 0     op3 = SNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 24:  // op2 = inf   op3 = SNaN
    case 34:  // op2 = QNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for C-style Max Macro of absolute value
 */
static int convert_result_cee_max_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 10:  // op2 = Fn    op3 = 0
    case 20:  // op3 = inf   op3 = 0
    case 21:  // op3 = inf   op3 = Fn
        result_out = 2;  // T(op2)
        break;
    case 00:  // op2 = 0     op3 = 0
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 12:  // op2 = Fn    op3 = inf
    case 22:  // op2 = inf   op3 = inf
        result_out = 3;  // T(op3)
        break;
    case 03:  // op2 = 0     op3 = QNaN
    case 04:  // op2 = 0     op3 = SNaN
    case 13:  // op2 = Fn    op3 = QNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 23:  // op3 = inf   op3 = QNaN
    case 24:  // op3 = inf   op3 = SNaN
    case 30:  // op3 = QNaN  op3 = 0
    case 31:  // op3 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
    case 33:  // op3 = QNaN  op3 = QNaN
    case 34:  // op3 = QNaN  op3 = SNaN
    case 40:  // op3 = SNaN  op3 = 0
    case 41:  // op3 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    case 43:  // op3 = SNaN  op3 = QNaN
    case 44:  // op3 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for C-style Min Macro of absolute value
 */
static int convert_result_cee_min_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 12:  // op2 = Fn    op3 = inf
        result_out = 2;  // T(op2)
        break;
    case 00:  // op2 = 0     op3 = 0
    case 10:  // op2 = Fn    op3 = 0
    case 20:  // op2 = inf   op3 = 0
    case 21:  // op2 = inf   op3 = Fn
    case 22:  // op2 = inf   op3 = inf
        result_out = 3;  // T(op3)
        break;
    case 03:  // op2 = 0     op3 = QNaN
    case 04:  // op2 = 0     op3 = SNaN
    case 13:  // op2 = Fn    op3 = QNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 23:  // op2 = inf   op3 = QNaN
    case 24:  // op2 = inf   op3 = SNaN
    case 30:  // op2 = QNaN  op3 = 0
    case 31:  // op2 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
    case 33:  // op2 = QNaN  op3 = QNaN
    case 34:  // op2 = QNaN  op3 = SNaN
    case 40:  // op2 = SNaN  op3 = 0
    case 41:  // op2 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    case 43:  // op2 = SNaN  op3 = QNaN
    case 44:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for C++ algorithm.max() of absolute value
 */
static int convert_result_cpp_max_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 00:  // op2 = 0     op3 = 0
    case 10:  // op2 = Fn    op3 = 0
    case 20:  // op3 = inf   op3 = 0
    case 21:  // op3 = inf   op3 = Fn
    case 22:  // op2 = inf   op3 = inf
        result_out = 2;  // T(op2)
        break;
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 12:  // op2 = Fn    op3 = inf
        result_out = 3;  // T(op3)
        break;
    case 03:  // op2 = 0     op3 = QNaN
    case 04:  // op2 = 0     op3 = SNaN
    case 13:  // op2 = Fn    op3 = QNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 23:  // op3 = inf   op3 = QNaN
    case 24:  // op3 = inf   op3 = SNaN
    case 30:  // op3 = QNaN  op3 = 0
    case 31:  // op3 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
    case 33:  // op3 = QNaN  op3 = QNaN
    case 34:  // op3 = QNaN  op3 = SNaN
    case 40:  // op3 = SNaN  op3 = 0
    case 41:  // op3 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    case 43:  // op3 = SNaN  op3 = QNaN
    case 44:  // op3 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 4;  // XI: T(op2)
        break;
    }

    return result_out;
}

/*
 * Convert result for C++ algorithm.min() of absolute value
 */
static int convert_result_cpp_min_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 00:  // op2 = 0     op3 = 0
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 12:  // op2 = Fn    op3 = inf
    case 22:  // op2 = inf   op3 = inf
        result_out = 2;  // T(op2)
        break;
    case 10:  // op2 = Fn    op3 = 0
    case 20:  // op2 = inf   op3 = 0
    case 21:  // op2 = inf   op3 = Fn
        result_out = 3;  // T(op3)
        break;
    case 03:  // op2 = 0     op3 = QNaN
    case 04:  // op2 = 0     op3 = SNaN
    case 13:  // op2 = Fn    op3 = QNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 23:  // op2 = inf   op3 = QNaN
    case 24:  // op2 = inf   op3 = SNaN
    case 30:  // op2 = QNaN  op3 = 0
    case 31:  // op2 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
    case 33:  // op2 = QNaN  op3 = QNaN
    case 34:  // op2 = QNaN  op3 = SNaN
    case 40:  // op2 = SNaN  op3 = 0
    case 41:  // op2 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    case 43:  // op2 = SNaN  op3 = QNaN
    case 44:  // op2 = SNaN  op3 = SNaN
    default:  // Should not occur!
        result_out = 4;  // XI: T(op2)
        break;
    }

    return result_out;
}

/*
 * Convert result for fmax() of absolute value
 */
static int convert_result_fmax_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 00:  // op2 = 0     op3 = 0
    case 10:  // op2 = Fn    op3 = 0
    case 13:  // op2 = Fn    op3 = QNaN
    case 20:  // op3 = inf   op3 = 0
    case 21:  // op3 = inf   op3 = Fn
    case 22:  // op2 = inf   op3 = inf
    case 23:  // op3 = inf   op3 = QNaN
    case 33:  // op3 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 03:  // op2 = 0     op3 = QNaN
    case 12:  // op2 = Fn    op3 = inf
    case 30:  // op3 = QNaN  op3 = 0
    case 31:  // op3 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
        result_out = 3;  // T(op3)
        break;
    case 04:  // op2 = 0     op3 = SNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 24:  // op3 = inf   op3 = SNaN
    case 34:  // op3 = QNaN  op3 = SNaN
    case 43:  // op3 = SNaN  op3 = QNaN
    case 44:  // op3 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 40:  // op3 = SNaN  op3 = 0
    case 41:  // op3 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

/*
 * Convert result for fmin() of absolute value
 */
static int convert_result_fmin_abs( int result_in )
{
    int result_out;

    switch (result_in)
    {
    case 11:  // op2 = Fn    op3 = Fn
        result_out = 1;  // T((M(op2,op3))
        break;
    case 00:  // op2 = 0     op3 = 0
    case 01:  // op2 = 0     op3 = Fn
    case 02:  // op2 = 0     op3 = inf
    case 03:  // op2 = 0     op3 = QNaN
    case 12:  // op2 = Fn    op3 = inf
    case 13:  // op2 = Fn    op3 = QNaN
    case 22:  // op2 = inf   op3 = inf
    case 23:  // op2 = inf   op3 = QNaN
    case 33:  // op2 = QNaN  op3 = QNaN
        result_out = 2;  // T(op2)
        break;
    case 10:  // op2 = Fn    op3 = 0
    case 20:  // op3 = inf   op3 = 0
    case 21:  // op2 = inf   op3 = Fn
    case 30:  // op2 = QNaN  op3 = 0
    case 31:  // op2 = QNaN  op3 = Fn
    case 32:  // op2 = QNaN  op3 = inf
        result_out = 3;  // T(op3)
        break;
    case 04:  // op2 = 0     op3 = SNaN
    case 14:  // op2 = Fn    op3 = SNaN
    case 24:  // op2 = inf   op3 = SNaN
    case 34:  // op2 = QNaN  op3 = SNaN
    case 43:  // op2 = SNaN  op3 = QNaN
    case 44:  // op2 = SNaN  op3 = SNaN
        result_out = 4;  // XI: T(op2)
        break;
    case 40:  // op2 = SNaN  op3 = 0
    case 41:  // op2 = SNaN  op3 = Fn
    case 42:  // op2 = SNaN  op3 = inf
    default:  // Should not occur!
        result_out = 5;  // XI: T(op3)
        break;
    }

    return result_out;
}

//  #endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

#endif /* !defined( _IEEE_NONARCHDEP_ ) */

// Vector processing with VXC for IEEE exception and element index
// Bits 0-3 of the VXC are the vector index (VIX).
// Bits 4-7 of the VXC are the vector interrupt code (VIC).
static void vector_ieee_trap( REGS *regs, int vix, U32 vic )
{
    U32 vxc;
    vxc = ( vix << VXC_VIX_SHIFT ) | vic;  /* Build VXC            */
    regs->dxc = vxc;                       /* Save VXC in PSA      */
    regs->fpc &= ~FPC_DXC;                 /* Clear DXC/VXC in FPC */
    regs->fpc |= (vxc << FPC_DXC_SHIFT);   /* Insert VXC into FPC  */
    regs->program_interrupt( regs, PGM_VECTOR_PROCESSING_EXCEPTION );
}

static void vector_ieee_cond_trap( int vix, REGS *regs, U32 ieee_traps )
{
    if (ieee_traps & FPC_MASK_IMI)
        vector_ieee_trap( regs, vix, VXC_IEEE_INVALID_OP );

    else if (ieee_traps & FPC_MASK_IMZ)
        vector_ieee_trap( regs, vix, VXC_IEEE_DIV_ZERO );

    else if (ieee_traps & FPC_MASK_IMO)
        vector_ieee_trap( regs, vix, VXC_IEEE_OVERFLOW );

    else if (ieee_traps & FPC_MASK_IMU)
        vector_ieee_trap( regs, vix, VXC_IEEE_UNDERFLOW );

    else if (ieee_traps & FPC_MASK_IMX)
        vector_ieee_trap( regs, vix, VXC_IEEE_INEXACT );
}

/* fastpath test for Xi trap; many instructions only return Xi */
#define VECTOR_IEEE_EXCEPTION_TRAP_XI( _vix, _regs )                 \
                                                                     \
    if (1                                                            \
        && (softfloat_exceptionFlags & softfloat_flag_invalid)       \
        && (_regs->fpc & FPC_MASK_IMI)                               \
    )                                                                \
        vector_ieee_trap( _regs, _vix, VXC_IEEE_INVALID_OP )

/* fastpath test for Xz trap; only Divide returns Xz  */
#define VECTOR_IEEE_EXCEPTION_TRAP_XZ( _vix, _regs )                 \
                                                                     \
    if (1                                                            \
        && (softfloat_exceptionFlags & softfloat_flag_infinite)      \
        && (_regs->fpc & FPC_MASK_IMZ)                               \
    )                                                                \
        vector_ieee_trap( _regs, _vix, VXC_IEEE_DIV_ZERO )

/* trap if any provided exception has been previously detected */
#define VECTOR_IEEE_EXCEPTION_TRAP( _vix, _regs, _ieee_trap_conds, _exceptions )  \
                                                                                  \
    if (_ieee_trap_conds & (_exceptions))                                         \
        vector_ieee_cond_trap( _vix, _regs, _ieee_trap_conds)

#undef VECTOR_GET_FLOAT128_OP
#undef VECTOR_GET_FLOAT64_OP
#undef VECTOR_GET_FLOAT32_OP

#define VECTOR_GET_FLOAT128_OP( op, v, regs )     ARCH_DEP( get_float128 )( &op, &regs->VR_Q( v ).d[FLOAT128_HI], &regs->VR_Q( v ).d[FLOAT128_LO])
#define VECTOR_GET_FLOAT64_OP(  op, v, i, regs )  ARCH_DEP( get_float64  )( &op, &regs->VR_D( v, i ))
#define VECTOR_GET_FLOAT32_OP(  op, v, i, regs )  ARCH_DEP( get_float32  )( &op, &regs->VR_F( v, i ))

#undef VECTOR_PUT_FLOAT128_NOCC
#undef VECTOR_PUT_FLOAT64_NOCC
#undef VECTOR_PUT_FLOAT32_NOCC

#define VECTOR_PUT_FLOAT128_NOCC( op, v, regs )     ARCH_DEP( put_float128 )( &op, &regs->VR_Q( v ).d[FLOAT128_HI], &regs->VR_Q( v ).d[FLOAT128_LO])
#define VECTOR_PUT_FLOAT64_NOCC(  op, v, i, regs )  ARCH_DEP( put_float64  )( &op, &regs->VR_D( v, i ))
#define VECTOR_PUT_FLOAT32_NOCC(  op, v, i, regs )  ARCH_DEP( put_float32  )( &op, &regs->VR_F( v, i ))

/*===========================================================================*/
/*  Start of z/Arch Vector Floating Point instructions                       */
/*===========================================================================*/

/*-------------------------------------------------------------------*/
/* E74A VFTCI  - Vector FP Test Data Class Immediate         [VRI-e] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFTCISB V1,V2,I3     VFTCI V1,V2,I3,2,0                         */
/*   VFTCIDB V1,V2,I3     VFTCI V1,V2,I3,3,0                         */
/*   WFTCISB V1,V2,I3     VFTCI V1,V2,I3,2,8                         */
/*   WFTCIDB V1,V2,I3     VFTCI V1,V2,I3,3,8                         */
/*   WFTCIXB V1,V2,I3     VFTCI V1,V2,I3,4,8                         */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_test_data_class_immediate )
{
    int     v1, v2, i3, m4, m5;
    int     i;
    int     one_bit_found, zero_bit_found;
    U32     op2_dataclass;

    VRI_E( inst, regs, v1, v2, i3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    one_bit_found = zero_bit_found = FALSE;

    if ( m4 == 3 )  // Long format
    {
        float64_t   op2;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
            op2_dataclass = float64_class( op2 );
            softfloat_exceptionFlags = 0;
            if (op2_dataclass & i3)
            {
                one_bit_found = TRUE;
                regs->VR_D( v1, i ) = 0xFFFFFFFFFFFFFFFFULL;
            }
            else
            {
                zero_bit_found = TRUE;
                regs->VR_D( v1, i ) = 0x0000000000000000ULL;
            }
            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
            op2_dataclass = float32_class( op2 );
            softfloat_exceptionFlags = 0;
            if (op2_dataclass & i3)
            {
                one_bit_found = TRUE;
                regs->VR_F( v1, i ) = 0xFFFFFFFF;
            }
            else
            {
                zero_bit_found = TRUE;
                regs->VR_F( v1, i ) = 0x00000000;
            }
            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op2;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );
        op2_dataclass = float128_class( op2 );
        softfloat_exceptionFlags = 0;
        if (op2_dataclass & i3)
        {
            one_bit_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0xFFFFFFFFFFFFFFFFULL;
            regs->VR_Q( v1 ).d[1] = 0xFFFFFFFFFFFFFFFFULL;
        }
        else
        {
            zero_bit_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0x0000000000000000ULL;
            regs->VR_Q( v1 ).d[1] = 0x0000000000000000ULL;
        }
    }

    if (one_bit_found == TRUE)
    {
        if (zero_bit_found == TRUE)
            // Selected bit is 1 for at least one but not all elements (when S-bit is zero)
            // Note: When the selected bit is 1 and the S-bit is 1 the previous if will not be true
            regs->psw.cc = 1;
        else
            // Selected bit is 1 for all elements (match)
            regs->psw.cc = 0;
    }
    else
        // Selected bit is 0 for all elements (no match)
        regs->psw.cc = 3;

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78E VFMS   - Vector FP Multiply and Subtract             [VRR-e] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFMSSB V1,V2,V3,V4   VFMS V1,V2,V3,V4,0,2                       */
/*   VFMSDB V1,V2,V3,V4   VFMS V1,V2,V3,V4,0,3                       */
/*   WFMSSB V1,V2,V3,V4   VFMS V1,V2,V3,V4,8,2                       */
/*   WFMSDB V1,V2,V3,V4   VFMS V1,V2,V3,V4,8,3                       */
/*   WFMSXB V1,V2,V3,V4   VFMS V1,V2,V3,V4,8,4                       */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_multiply_and_subtract )
{
    int     v1, v2, v3, v4, m5, m6;
    int     i;
    U32     ieee_trap_conds;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m6 < 2 || m6 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m6 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m6 == 3 )  // Long format
    {
        float64_t  op1[2], op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            /* if Operand 4 is not a NaN, the sign bit is inverted */
            if (0
                || !(op4.v & 0x000FFFFFFFFFFFFF)
                || ((op4.v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
            )
                op4.v ^= 0x8000000000000000ULL;

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
           }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1[4], op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            /* if Operand 4 is not a NaN, the sign bit is inverted */
            if (0
                || !(op4.v & 0x007FFFFF)
                || ((op4.v & 0x7F800000) ^ 0x7F800000)
            )
                op4.v ^= 0x80000000;

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3, op4;

        VECTOR_GET_FLOAT128_OP( op4, v4, regs );
        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        /* if Operand 4 is not a NaN, the sign bit is inverted */
        if (0
            || !(op4.v[FLOAT128_HI] & 0x000FFFFFFFFFFFFF)
            || ((op4.v[FLOAT128_HI] & 0x7FF0000000000000) ^ 0x7FF0000000000000)
        )
            op4.v[FLOAT128_HI] ^= 0x8000000000000000ULL;

        ieee_trap_conds = 0;
        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_mulAdd( op2, op3, op4 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78F VFMA   - Vector FP Multiply and Add                  [VRR-e] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFMASB V1,V2,V3,V4   VFMA V1,V2,V3,V4,0,2                       */
/*   VFMADB V1,V2,V3,V4   VFMA V1,V2,V3,V4,0,3                       */
/*   WFMASB V1,V2,V3,V4   VFMA V1,V2,V3,V4,8,2                       */
/*   WFMADB V1,V2,V3,V4   VFMA V1,V2,V3,V4,8,3                       */
/*   WFMAXB V1,V2,V3,V4   VFMA V1,V2,V3,V4,8,4                       */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_multiply_and_add )
{
    int     v1, v2, v3, v4, m5, m6;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m6 < 2 || m6 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m6 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m6 == 3 )  // Long format
    {
        float64_t  op1[2], op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
           }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1[4], op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3, op4;

        VECTOR_GET_FLOAT128_OP( op4, v4, regs );
        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        ieee_trap_conds = 0;
        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_mulAdd( op2, op3, op4 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E79E VFNMS  - Vector FP Negative Multiply And Subtract    [VRR-e] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFNMSSB V1,V2,V3,V4  VFNMS V1,V2,V3,V4,0,2                      */
/*   VFNMSDB V1,V2,V3,V4  VFNMS V1,V2,V3,V4,0,3                      */
/*   WFNMSSB V1,V2,V3,V4  VFNMS V1,V2,V3,V4,8,2                      */
/*   WFNMSDB V1,V2,V3,V4  VFNMS V1,V2,V3,V4,8,3                      */
/*   WFNMSXB V1,V2,V3,V4  VFNMS V1,V2,V3,V4,8,4                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_negative_multiply_and_subtract )
{
    int     v1, v2, v3, v4, m5, m6;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m6 < 2 || m6 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m6 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m6 == 3 )  // Long format
    {
        float64_t  op1[2], op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            /* if Operand 4 is not a NaN, the sign bit is inverted */
            if (0
                || !(op4.v & 0x000FFFFFFFFFFFFF)
                || ((op4.v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
            )
                op4.v ^= 0x8000000000000000ULL;

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
           }

            /* if Operand 1 is not a NaN, the sign bit is inverted */
            if (0
                || !(op1[i].v & 0x000FFFFFFFFFFFFF)
                || ((op1[i].v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
            )
                op1[i].v ^= 0x8000000000000000ULL;

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1[4], op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            /* if Operand 4 is not a NaN, the sign bit is inverted */
            if (0
                || !(op4.v & 0x007FFFFF)
                || ((op4.v & 0x7F800000) ^ 0x7F800000)
            )
                op4.v ^= 0x80000000;

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
           }

            /* if Operand 1 is not a NaN, the sign bit is inverted */
            if (0
                || !(op1[i].v & 0x007FFFFF)
                || ((op1[i].v & 0x7F800000) ^ 0x7F800000)
            )
                op1[i].v ^= 0x80000000;

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3, op4;

        VECTOR_GET_FLOAT128_OP( op4, v4, regs );
        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        /* if Operand 4 is not a NaN, the sign bit is inverted */
        if (0
            || !(op4.v[FLOAT128_HI] & 0x000FFFFFFFFFFFFF)
            || ((op4.v[FLOAT128_HI] & 0x7FF0000000000000) ^ 0x7FF0000000000000)
        )
            op4.v[FLOAT128_HI] ^= 0x8000000000000000ULL;

        ieee_trap_conds = 0;
        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_mulAdd( op2, op3, op4 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        /* if Operand 1 is not a NaN, the sign bit is inverted */
        if (0
            || !(op1.v[FLOAT128_HI] & 0x000FFFFFFFFFFFFF)
            || ((op1.v[FLOAT128_HI] & 0x7FF0000000000000) ^ 0x7FF0000000000000)
        )
            op1.v[FLOAT128_HI] ^= 0x8000000000000000ULL;

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E79F VFNMA  - Vector FP Negative Multiply And Add         [VRR-e] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFNMASB V1,V2,V3,V4  VFNMA V1,V2,V3,V4,0,2                      */
/*   VFNMADB V1,V2,V3,V4  VFNMA V1,V2,V3,V4,0,3                      */
/*   WFNMASB V1,V2,V3,V4  VFNMA V1,V2,V3,V4,8,2                      */
/*   WFNMADB V1,V2,V3,V4  VFNMA V1,V2,V3,V4,8,3                      */
/*   WFNMAXB V1,V2,V3,V4  VFNMA V1,V2,V3,V4,8,4                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_negative_multiply_and_add )
{
    int     v1, v2, v3, v4, m5, m6;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m6 < 2 || m6 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m6 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m6 == 3 )  // Long format
    {
        float64_t  op1[2], op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
           }

            /* if Operand 1 is not a NaN, the sign bit is inverted */
            if (0
                || !(op1[i].v & 0x000FFFFFFFFFFFFF)
                || ((op1[i].v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
            )
                op1[i].v ^= 0x8000000000000000ULL;

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1[4], op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op4, v4, i, regs );
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            ieee_trap_conds = 0;
            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_mulAdd( op2, op3, op4 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
           }

            /* if Operand 1 is not a NaN, the sign bit is inverted */
            if (0
                || !(op1[i].v & 0x007FFFFF)
                || ((op1[i].v & 0x7F800000) ^ 0x7F800000)
            )
                op1[i].v ^= 0x80000000;

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m6 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3, op4;

        VECTOR_GET_FLOAT128_OP( op4, v4, regs );
        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        ieee_trap_conds = 0;
        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_mulAdd( op2, op3, op4 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        /* if Operand 1 is not a NaN, the sign bit is inverted */
        if (0
            || !(op1.v[FLOAT128_HI] & 0x000FFFFFFFFFFFFF)
            || ((op1.v[FLOAT128_HI] & 0x7FF0000000000000) ^ 0x7FF0000000000000)
        )
            op1.v[FLOAT128_HI] ^= 0x8000000000000000ULL;

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*----------------------------------------------------------------------*/
/* E7C0 VCLFP  - Vector FP Convert To Logical (short BFP to 32) [VRR-a] */
/* E7C0 VCLGD  - Vector FP Convert To Logical (long BFP to 64)  [VRR-a] */
/*                                                                      */
/*   Extended Mnemonic      Base Mnemonic                               */
/*   VCLGD  V1,V2,M3,M4,M5  VCLFP V1,V2,M3,M4,M5                        */
/*   VCLFEB V1,V2,M4,M5     VCLFP V1,V2,2,M4,M5                         */
/*   VCLGDB V1,V2,M4,M5     VCLFP V1,V2,3,M4,M5                         */
/*   WCLFEB V1,V2,M4,M5     VCLFP V1,V2,2,(8 | M4),M5                   */
/*   WCLGDB V1,V2,M4,M5     VCLFP V1,V2,3,(8 | M4),M5                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_to_logical )
{
    int     v1, v2, m3, m4, m5;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_XC ((m4 & 0x4) != 0) // IEEE-inexact-exception control (XxC)  See SUPPRESS_INEXACT
#define M4_RE ((m4 & 0x3) != 0) // Reserved

    if ( FACILITY_ENABLED( 148_VECTOR_ENH_2, regs ) )
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 < 2 || m3 > 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  /* BFP long format to doubleword */
    {
        U64         op1[2];
        float64_t   op2;

        for (i=0; i < 2; i++)
        {
            SET_SF_RM_FROM_MASK( m5 );

            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
            softfloat_exceptionFlags = 0;

            if (FLOAT64_ISNAN( op2 ))
            {
                op1[i] = 0;
                softfloat_raiseFlags( softfloat_flag_invalid );
            }
            else
            {
                SET_SF_RM_FROM_MASK( m3 );
                op1[i] = f64_to_ui64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
            }

            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                if (!SUPPRESS_INEXACT( m4 ))
                    softfloat_exceptionFlags |= softfloat_flag_inexact;
            }

            ieee_trap_conds = ieee_exception_test_oux( regs );
            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );

            if (M4_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            regs->VR_D( v1, i ) = op1[i];
            if (M4_SE) break;
        }
    }
    else if ( m3 == 2 )  /* BFP short format to word */
    {
        U32         op1[4];
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            SET_SF_RM_FROM_MASK( m5 );

            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
            softfloat_exceptionFlags = 0;

            if (FLOAT32_ISNAN( op2 ))
            {
                op1[i] = 0;
                softfloat_raiseFlags( softfloat_flag_invalid );
            }
            else
            {
                SET_SF_RM_FROM_MASK( m3 );
                op1[i] = f32_to_ui32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
            }

            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                if (!SUPPRESS_INEXACT( m4 ))
                    softfloat_exceptionFlags |= softfloat_flag_inexact;
            }

            ieee_trap_conds = ieee_exception_test_oux( regs );
            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );

            if (M4_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            regs->VR_F( v1, i ) = op1[i];
            if (M4_SE) break;
        }
    }

#undef M4_SE
#undef M4_XC
#undef M4_RE

    ZVECTOR_END( regs );
}

/*------------------------------------------------------------------------*/
/* E7C1 VCFPL  - Vector FP Convert From Logical (32 to short BFP) [VRR-a] */
/* E7C1 VCDLG  - Vector FP Convert From Logical (64 to long BFP)  [VRR-a] */
/*                                                                        */
/*   Extended Mnemonic      Base Mnemonic                                 */
/*   VCDLG  V1,V2,M3,M4,M5  VCFPL V1,V2,M3,M4,M5                          */
/*   VCELFB V1,V2,M4,M5     VCFPL V1,V2,2,M4,M5                           */
/*   VCDLGB V1,V2,M4,M5     VCFPL V1,V2,3,M4,M5                           */
/*   WCELFB V1,V2,M4,M5     VCFPL V1,V2,2,(8 | M4),M5                     */
/*   WCDLGB V1,V2,M4,M5     VCFPL V1,V2,3,(8 | M4),M5                     */
/*                                                                        */
/*------------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_from_logical )
{
    int     v1, v2, m3, m4, m5;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_XC ((m4 & 0x4) != 0) // IEEE-inexact-exception control (XxC)  See SUPPRESS_INEXACT
#define M4_RE ((m4 & 0x3) != 0) // Reserved

    if ( FACILITY_ENABLED( 148_VECTOR_ENH_2, regs ) )
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 < 2 || m3 > 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Doubleword to BFP long format
    {
        float64_t  op1[2];
        U64        op2;

        for (i=0; i < 2; i++)
        {
            SET_SF_RM_FROM_MASK( m5 );

            op2 = regs->VR_D(  v2, i );
            softfloat_exceptionFlags = 0;

            op1[i] = ui64_to_f64( op2 );

            if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
            {
                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            if (M4_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }
    else if ( m3 == 2 )  // Word to BFP short format
    {
        float32_t  op1[4];
        U32        op2;

        for (i=0; i < 4; i++)
        {
            SET_SF_RM_FROM_MASK( m5 );

            op2 = regs->VR_F(  v2, i );
            softfloat_exceptionFlags = 0;

            op1[i] = ui32_to_f32( op2 );

            if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
            {
                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            if (M4_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }

#undef M4_SE
#undef M4_XC
#undef M4_RE

    ZVECTOR_END( regs );
}

/*--------------------------------------------------------------------*/
/* E7C2 VCSFP  - Vector FP Convert To Fixed (short BFP to 32) [VRR-a] */
/* E7C2 VCGD   - Vector FP Convert To Fixed (long BFP to 64)  [VRR-a] */
/*                                                                    */
/*   Extended Mnemonic     Base Mnemonic                              */
/*   VCGD  V1,V2,M3,M4,M5  VCSFP V1,V2,M3,M4,M5                       */
/*   VCFEB V1,V2,M4,M5     VCSFP V1,V2,2,M4,M5                        */
/*   VCGDB V1,V2,M4,M5     VCSFP V1,V2,3,M4,M5                        */
/*   WCFEB V1,V2,M4,M5     VCSFP V1,V2,2,(8 | M4),M5                  */
/*   WCGDB V1,V2,M4,M5     VCSFP V1,V2,3,(8 | M4),M5                  */
/*                                                                    */
/*--------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_to_fixed )
{
    int     v1, v2, m3, m4, m5;
    int     i;
    U32     ieee_trap_conds = 0;
    U32     op2_dataclass;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_XC ((m4 & 0x4) != 0) // IEEE-inexact-exception control (XxC)  See SUPPRESS_INEXACT
#define M4_RE ((m4 & 0x3) != 0) // Reserved

    if ( FACILITY_ENABLED( 148_VECTOR_ENH_2, regs ) )
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 < 2 || m3 > 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  /* BFP long format to doubleword */
    {
        S64         op1[2];
        float64_t   op2;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
            op2_dataclass = float64_class( op2 );
            softfloat_exceptionFlags = 0;

            if (op2_dataclass & (float_class_neg_signaling_nan |
                                 float_class_pos_signaling_nan |
                                 float_class_neg_quiet_nan     |
                                 float_class_pos_quiet_nan ))
            {
                /* NaN input always returns maximum negative integer,
                   cc3, and IEEE invalid exception */
                op1[i] = -(0x7FFFFFFFFFFFFFFFULL) - 1;
                softfloat_raiseFlags( softfloat_flag_invalid );
            }
            else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
            {
                op1[i] = 0;
            }
            else
            {
                if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
                    op1[i] = 0;
                else
                {
                    SET_SF_RM_FROM_MASK( m5 );
                    op1[i] = f64_to_i64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
                }
            }

            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                if (!SUPPRESS_INEXACT( m4 ))
                    softfloat_exceptionFlags |= softfloat_flag_inexact;
            }

            ieee_trap_conds = ieee_exception_test_oux( regs );
            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );

            if (M4_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            regs->VR_D( v1, i ) = op1[i];
            if (M4_SE) break;
        }
    }
    else if ( m3 == 2 )  /* BFP short format to word */
    {
        S32         op1[4];
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
            op2_dataclass = float32_class( op2 );
            softfloat_exceptionFlags = 0;

            if (op2_dataclass & (float_class_neg_signaling_nan |
                                 float_class_pos_signaling_nan |
                                 float_class_neg_quiet_nan     |
                                 float_class_pos_quiet_nan ))
            {
                /* NaN input always returns maximum negative integer,
                   cc3, and IEEE invalid exception */
                op1[i] = -0x7FFFFFFF - 1;
                softfloat_raiseFlags( softfloat_flag_invalid );
            }
            else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
            {
                op1[i] = 0;
            }
            else
            {
                if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
                    op1[i] = 0;
                else
                {
                    SET_SF_RM_FROM_MASK( m3 );
                    op1[i] = f32_to_i32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
                }
            }

            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                if (!SUPPRESS_INEXACT( m4 ))
                    softfloat_exceptionFlags |= softfloat_flag_inexact;
            }

            ieee_trap_conds = ieee_exception_test_oux( regs );
            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );

            if (M4_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            regs->VR_F( v1, i ) = op1[i];
            if (M4_SE) break;
        }
    }

#undef M4_SE
#undef M4_XC
#undef M4_RE

    ZVECTOR_END( regs );
}

/*----------------------------------------------------------------------*/
/* E7C3 VCFPS  - Vector FP Convert From Fixed (32 to short BFP) [VRR-a] */
/* E7C3 VCDG   - Vector FP Convert From Fixed (64 to long BFP)  [VRR-a] */
/*                                                                      */
/*   Extended Mnemonic     Base Mnemonic                                */
/*   VCDG  V1,V2,M3,M4,M5  VCFPS V1,V2,M3,M4,M5                         */
/*   VCEFB V1,V2,M4,M5     VCFPS V1,V2,2,M4,M5                          */
/*   VCDGB V1,V2,M4,M5     VCFPS V1,V2,3,M4,M5                          */
/*   WCEFB V1,V2,M4,M5     VCFPS V1,V2,2,(8 | M4),M5                    */
/*   WCDGB V1,V2,M4,M5     VCFPS V1,V2,3,(8 | M4),M5                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_from_fixed )
{
    int     v1, v2, m3, m4, m5;
    int     i;
    int     ieee_trap_conds = 0;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_XC ((m4 & 0x4) != 0) // IEEE-inexact-exception control (XxC)  See SUPPRESS_INEXACT
#define M4_RE ((m4 & 0x3) != 0) // Reserved

    if ( FACILITY_ENABLED( 148_VECTOR_ENH_2, regs ) )
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 < 2 || m3 > 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Doubleword to BFP long format
    {
        /* Fixed 64-bit may not fit in the 52+1 bits available in a  */
        /* long BFP, IEEE Inexact exceptions are possible. If m4     */
        /* Inexact suppression control (XxC)  See SUPPRESS_INEXACT is on, then no Inexact  */
        /* exceptions recognized (no trap nor flag set).             */
        float64_t   op1[2];
        S64         op2;

        for (i=0; i < 2; i++)
        {
            SET_SF_RM_FROM_MASK( m5 );

            op2 = regs->VR_D( v2, i );
            softfloat_exceptionFlags = 0;
            op1[i] = i64_to_f64( op2);

            /* Inexact occurred and not masked by m4? */
            if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
            {
                /* Yes, set FPC flags and test for a trap */
                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            if (M4_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }
    else if ( m3 == 2 )  // Word to BFP short format
    {
        /* Fixed 32-bit may need to be rounded to fit in the 23+1    */
        /* bits available in a short BFP, IEEE Inexact may be        */
        /* raised. If m4 Inexact suppression (XxC)  See SUPPRESS_INEXACT is on, then no    */
        /* inexact exception is recognized (no trap nor flag set).   */
        float32_t   op1[4];
        S32         op2;

        for (i=0; i < 4; i++)
        {
            SET_SF_RM_FROM_MASK( m5 );

            op2 = regs->VR_F( v2, i );
            softfloat_exceptionFlags = 0;
            op1[i] = i32_to_f32( op2 );

            /* Inexact occurred and not masked by m4? */
            if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
            {
                /* Yes, set FPC flags and test for a trap */
                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            if (M4_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }

#undef M4_SE
#undef M4_XC
#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C4 VFLL   - Vector FP Load Lengthened                   [VRR-a] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   WLDEB V1,V2          VFLL V1,V2,2,8                             */
/*   VFLLS V1,V2          VFLL V1,V2,2,0                             */
/*   WFLLS V1,V2          VFLL V1,V2,2,8                             */
/*   WFLLD V1,V2          VFLL V1,V2,3,8                             */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_load_lengthened )
{
    int     v1, v2, m3, m4, m5;
    int     i;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_RE ((m4 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M4_RE || m3 < 2 || m3 > 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  /* Long format to Extended format, i.e. 64-bit to 128-bit */
    {
        float128_t  op1;
        float64_t   op2;

        VECTOR_GET_FLOAT64_OP( op2, v2, 0, regs );
        softfloat_exceptionFlags = 0;
        if (f64_isSignalingNaN( op2 ))
        {
            softfloat_exceptionFlags = softfloat_flag_invalid;
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );

            FLOAT64_MAKE_QNAN( op2 );
            SET_FPC_FLAGS_FROM_SF( regs );
        }
        op1 = f64_to_f128( op2 );
        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }
    else if ( m3 == 2 )  /* Short format to Long format, i.e. 32-bit to 64-bit */
    {
        float64_t   op1[2];
        float32_t   op2;
        int         j;

        for (i=0, j=0; i < 4; i+=2, j++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
            softfloat_exceptionFlags = 0;
            if (f32_isSignalingNaN( op2 ))
            {
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                FLOAT32_MAKE_QNAN( op2 );
                SET_FPC_FLAGS_FROM_SF( regs );
            }
            op1[j] = f32_to_f64( op2 );
            if (M4_SE) break;
        }
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[j], v1, j, regs );
            if (M4_SE) break;
        }
    }

#undef M4_SE
#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C5 VFLR   - Vector FP Load Rounded                      [VRR-a] */
/*                                                                   */
/*   Extended Mnemonic     Base Mnemonic                             */
/*   VLED  V1,V2,M3,M4,M5  VFLR V1,V2,M3,M4,M5                       */
/*   VLEDB V1,V2,M4,M5     VFLR V1,V2,3,M4,M5                        */
/*   WLEDB V1,V2,M4,M5     VFLR V1,V2,3,(8 | M4),M5                  */
/*   VFLRD V1,V2,M4,M5     VFLR V1,V2,3,M4,M5                        */
/*   WFLRD V1,V2,M4,M5     VFLR V1,V2,3,(8 | M4),M5                  */
/*   WFLRX V1,V2,M4,M5     VFLR V1,V2,4,(8 | M4),M5                  */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_load_rounded )
{
    int     v1, v2, m3, m4, m5;
    int     i, j;
    U32     ieee_trap_conds = 0;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_XC ((m4 & 0x4) != 0) // IEEE-inexact-exception control (XxC)  See SUPPRESS_INEXACT
#define M4_RE ((m4 & 0x3) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 < 3 || m3 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Long format
    {
        float32_t   op1[4];
        float64_t   op2;

        for (i=0, j=0; i < 2; i++, j+=2)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            SET_SF_RM_FROM_MASK( m5 );

            softfloat_exceptionFlags = 0;

            op1[j] = f64_to_f32( op2 );

            if (SUPPRESS_INEXACT( m4 ))
                softfloat_exceptionFlags &= ~softfloat_flag_inexact;

            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags)
            {
                ieee_trap_conds = ieee_exception_test_oux( regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }

            if (M4_SE) break;
        }
        for (i=0, j=0; i < 2; i++, j+=2)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[j], v1, j, regs );
            if (M4_SE) break;
        }
    }
    else if ( m3 == 4 )  // Extended format
    {
        float64_t   op1;
        float128_t  op2;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        SET_SF_RM_FROM_MASK( m5 );

        softfloat_exceptionFlags = 0;

        op1 = f128_to_f64( op2 );

        if (SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags &= ~softfloat_flag_inexact;

        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );

        if (softfloat_exceptionFlags)
        {
            ieee_trap_conds = ieee_exception_test_oux( regs );

            VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
        }

        VECTOR_PUT_FLOAT64_NOCC( op1, v1, 0, regs );
    }

#undef M4_SE
#undef M4_XC
#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C7 VFI    - Vector Load FP Integer                      [VRR-a] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFISB V1,V2,M4,M5    VFI V1,V2,2,M4,M5                          */
/*   WFISB V1,V2,M4,M5    VFI V1,V2,2,(8 | M4),M5                    */
/*   VFIDB V1,V2,M4,M5    VFI V1,V2,3,M4,M5                          */
/*   WFIDB V1,V2,M4,M5    VFI V1,V2,3,(8 | M4),M5                    */
/*   WFIXB V1,V2,M4,M5    VFI V1,V2,4,(8 | M4),M5                    */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_fp_integer )
{
    int     v1, v2, m3, m4, m5;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_XC ((m4 & 0x4) != 0) // IEEE-inexact-exception control (XxC)  See SUPPRESS_INEXACT
#define M4_RE ((m4 & 0x3) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 < 2 || m3 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( m5 == 2 || m5 > 7 || M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Long format
    {
        float64_t   op1[2], op2;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_MASK( m5 );

            op1[i] = f64_roundToInt( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags)
            {
                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            if (M4_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op1[4], op2;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_MASK( m5 );

            op1[i] = f32_roundToInt( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags)
            {
                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            if (M4_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }
    else if ( m3 == 4 )  // Extended format
    {
        float128_t  op1, op2;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_MASK( m5 );

        op1 = f128_roundToInt( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );

        if (softfloat_exceptionFlags)
        {
            ieee_trap_conds = ieee_exception_test_oux( regs );
            VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds, FPC_MASK_IMX );
        }

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M4_SE
#undef M4_XC
#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CA WFK    - Vector FP Compare and Signal Scalar         [VRR-a] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   WFKSB  V1,V2         WFK  V1,V2,2,0                             */
/*   WFKDB  V1,V2         WFK  V1,V2,3,0                             */
/*   WFKXB  V1,V2         WFK  V1,V2,4,0                             */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_and_signal_scalar )
{
    int     v1, v2, m3, m4, m5;
    BYTE    newcc = 3;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

#define M4_RE ((m4 & 0xF) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M4_RE || m3 < 2 || m3 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Long format
    {
        float64_t   op1, op2;

        VECTOR_GET_FLOAT64_OP( op2, v2, 0, regs );
        VECTOR_GET_FLOAT64_OP( op1, v1, 0, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT64_COMPARE( op1, op2 );
        if (newcc == 3)
        {
            softfloat_exceptionFlags = softfloat_flag_invalid;
        }
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op1, op2;

        VECTOR_GET_FLOAT32_OP( op2, v2, 0, regs );
        VECTOR_GET_FLOAT32_OP( op1, v1, 0, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT32_COMPARE( op1, op2 );
        if (newcc == 3)
        {
            softfloat_exceptionFlags = softfloat_flag_invalid;
        }
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
    }
    else if ( m3 == 4 )  // Extended format
    {
        float128_t  op1, op2;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );
        VECTOR_GET_FLOAT128_OP( op1, v1, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT128_COMPARE( op1, op2 );
        if (newcc == 3)
        {
            softfloat_exceptionFlags = softfloat_flag_invalid;
        }
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
    }

    //  Resulting Condition Code:
    //  0 Operand elements equal
    //  1 First operand element low
    //  2 First operand element high
    //  3 Operand elements unordered
    regs->psw.cc = newcc;

#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CB WFC    - Vector FP Compare Scalar                    [VRR-a] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   WFCSB  V1,V2         WFC  V1,V2,2,0                             */
/*   WFCDB  V1,V2         WFC  V1,V2,3,0                             */
/*   WFCXB  V1,V2         WFC  V1,V2,4,0                             */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_scalar )
{
    int     v1, v2, m3, m4, m5;
    BYTE    newcc = 3;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

#define M4_RE ((m4 & 0xF) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M4_RE || m3 < 2 || m3 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Long format
    {
        float64_t   op1, op2;

        VECTOR_GET_FLOAT64_OP( op2, v2, 0, regs );
        VECTOR_GET_FLOAT64_OP( op1, v1, 0, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT64_COMPARE( op1, op2 );
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
        if (softfloat_exceptionFlags & softfloat_flag_invalid)
        {
            SET_FPC_FLAGS_FROM_SF( regs );
        }
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op1, op2;

        VECTOR_GET_FLOAT32_OP( op2, v2, 0, regs );
        VECTOR_GET_FLOAT32_OP( op1, v1, 0, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT32_COMPARE( op1, op2 );
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
        if (softfloat_exceptionFlags & softfloat_flag_invalid)
        {
            SET_FPC_FLAGS_FROM_SF( regs );
        }
    }
    else if ( m3 == 4 )  // Extended format
    {
        float128_t  op1, op2;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );
        VECTOR_GET_FLOAT128_OP( op1, v1, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT128_COMPARE( op1, op2 );
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
        if (softfloat_exceptionFlags & softfloat_flag_invalid)
        {
            SET_FPC_FLAGS_FROM_SF( regs );
        }
    }

    //  Resulting Condition Code:
    //  0 Operand elements equal
    //  1 First operand element low
    //  2 First operand element high
    //  3 Operand elements unordered
    regs->psw.cc = newcc;

#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CC VFPSO  - Vector FP Perform Sign Operation            [VRR-a] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFPSOSB V1,V2,M5     VFPSO V1,V2,2,0,M5                         */
/*   WFPSOSB V1,V2,M5     VFPSO V1,V2,2,8,M5                         */
/*   VFLCSB V1,V2         VFPSO V1,V2,2,0,0                          */
/*   WFLCSB V1,V2         VFPSO V1,V2,2,8,0                          */
/*   VFLNSB V1,V2         VFPSO V1,V2,2,0,1                          */
/*   WFLNSB V1,V2         VFPSO V1,V2,2,8,1                          */
/*   VFLPSB V1,V2         VFPSO V1,V2,2,0,2                          */
/*   WFLPSB V1,V2         VFPSO V1,V2,2,8,2                          */
/*   VFPSODB V1,V2,M5     VFPSO V1,V2,3,0,M5                         */
/*   WFPSODB V1,V2,M5     VFPSO V1,V2,3,8,M5                         */
/*   VFLCDB V1,V2         VFPSO V1,V2,3,0,0                          */
/*   WFLCDB V1,V2         VFPSO V1,V2,3,8,0                          */
/*   VFLNDB V1,V2         VFPSO V1,V2,3,0,1                          */
/*   WFLNDB V1,V2         VFPSO V1,V2,3,8,1                          */
/*   VFLPDB V1,V2         VFPSO V1,V2,3,0,2                          */
/*   WFLPDB V1,V2         VFPSO V1,V2,3,8,2                          */
/*   WFPSOXB V1,V2,M5     VFPSO V1,V2,4,8,M5                         */
/*   WFLCXB V1,V2         VFPSO V1,V2,4,8,0                          */
/*   WFLNXB V1,V2         VFPSO V1,V2,4,8,1                          */
/*   WFLPXB V1,V2         VFPSO V1,V2,4,8,2                          */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_perform_sign_operation )
{
    int     v1, v2, m3, m4, m5;
    int     i;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_RE ((m4 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( m5 > 2 || M4_RE || m3 < 2 || m3 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( m5 > 2 || M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Long format
    {
        float64_t   op2;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            switch (m5)
            {
            case 0:  // Complement
                op2.v ^= 0x8000000000000000ULL;
                break;
            case 1:  // Negative
                op2.v |= 0x8000000000000000ULL;
                break;
            case 2:  // Positive
                op2.v &= ~0x8000000000000000ULL;
                break;
            }

            VECTOR_PUT_FLOAT64_NOCC( op2, v1, i, regs );

            if (M4_SE) break;
        }
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            switch (m5)
            {
            case 0:  // Complement
                op2.v ^= 0x80000000;
                break;
            case 1:  // Negative
                op2.v |= 0x80000000;
                break;
            case 2:  // Positive
                op2.v &= ~0x80000000;
                break;
            }

            VECTOR_PUT_FLOAT32_NOCC( op2, v1, i, regs );

            if (M4_SE) break;
        }
    }
    else if ( m3 == 4 )  // Extended format
    {
        float128_t  op2;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        switch (m5)
        {
        case 0:  // Complement
            op2.v[FLOAT128_HI] ^= 0x8000000000000000ULL;
            break;
        case 1:  // Negative
            op2.v[FLOAT128_HI] |= 0x8000000000000000ULL;
            break;
        case 2:  // Positive
            op2.v[FLOAT128_HI] &= ~0x8000000000000000ULL;
            break;
        }

        VECTOR_PUT_FLOAT128_NOCC( op2, v1, regs );
    }

#undef M4_SE
#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CE VFSQ   - Vector FP Square Root                       [VRR-a] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFSQSB V1,V2         VFSQ V1,V2,2,0                             */
/*   VFSQDB V1,V2         VFSQ V1,V2,3,0                             */
/*   WFSQSB V1,V2         VFSQ V1,V2,2,8                             */
/*   WFSQDB V1,V2         VFSQ V1,V2,3,8                             */
/*   WFSQXB V1,V2         VFSQ V1,V2,4,8                             */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_square_root )
{
    int     v1, v2, m3, m4, m5;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

#define M4_SE ((m4 & 0x8) != 0) // Single-Element-Control (S)
#define M4_RE ((m4 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M4_RE || m3 < 2 || m3 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M4_RE || m3 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m3 == 3 )  // Long format
    {
        float64_t   op1[2], op2;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_sqrt( op2 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            SET_FPC_FLAGS_FROM_SF( regs );

            if (M4_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op1[4], op2;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_sqrt( op2 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            SET_FPC_FLAGS_FROM_SF( regs );

            if (M4_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M4_SE) break;
        }
    }
    else if ( m3 == 4 )  // Extended format
    {
        float128_t  op1, op2;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_sqrt( op2 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
        }

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds, FPC_MASK_IMX );
        SET_FPC_FLAGS_FROM_SF( regs );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M4_SE
#undef M4_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E2 VFS    - Vector FP Subtract                          [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFSSB V1,V2,V3       VFS V1,V2,V3,2,0                           */
/*   VFSDB V1,V2,V3       VFS V1,V2,V3,3,0                           */
/*   WFSSB V1,V2,V3       VFS V1,V2,V3,2,8                           */
/*   WFSDB V1,V2,V3       VFS V1,V2,V3,3,8                           */
/*   WFSXB V1,V2,V3       VFS V1,V2,V3,4,8                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_subtract )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m4 == 3 )  // Long format
    {
        float64_t   op1[2], op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_sub( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1[4], op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_sub( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3;

        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_sub( op2, op3 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E3 VFA    - Vector FP Add                               [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFASB  V1,V2,V3      VFA  V1,V2,V3,2,0                          */
/*   VFADB  V1,V2,V3      VFA  V1,V2,V3,3,0                          */
/*   WFASB  V1,V2,V3      VFA  V1,V2,V3,2,8                          */
/*   WFADB  V1,V2,V3      VFA  V1,V2,V3,3,8                          */
/*   WFAXB  V1,V2,V3      VFA  V1,V2,V3,4,8                          */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_add )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m4 == 3 )  // Long format
    {
        float64_t   op1[2], op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_add( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1[4], op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_add( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3;

        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_add( op2, op3 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );

            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E5 VFD    - Vector FP Divide                            [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFDSB V1,V2,V3       VFD V1,V2,V3,2,0                           */
/*   WFDSB V1,V2,V3       VFD V1,V2,V3,2,8                           */
/*   VFDDB V1,V2,V3       VFD V1,V2,V3,3,0                           */
/*   WFDDB V1,V2,V3       VFD V1,V2,V3,3,8                           */
/*   WFDXB V1,V2,V3       VFD V1,V2,V3,4,8                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_divide )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m4 == 3 )  // Long format
    {
        float64_t   op1[2], op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_div( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                VECTOR_IEEE_EXCEPTION_TRAP_XZ( i, regs );

                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1[4], op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_div( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                VECTOR_IEEE_EXCEPTION_TRAP_XZ( i, regs );

                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3;

        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_div( op2, op3 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            VECTOR_IEEE_EXCEPTION_TRAP_XZ( 0, regs );

            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E7 VFM    - Vector FP Multiply                          [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFMSB V1,V2,V3       VFM V1,V2,V3,2,0                           */
/*   VFMDB V1,V2,V3       VFM V1,V2,V3,3,0                           */
/*   WFMSB V1,V2,V3       VFM V1,V2,V3,2,8                           */
/*   WFMDB V1,V2,V3       VFM V1,V2,V3,3,8                           */
/*   WFMXB V1,V2,V3       VFM V1,V2,V3,4,8                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_multiply )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U32     ieee_trap_conds = 0;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    if ( m4 == 3 )  // Long format
    {
        float64_t   op1[2], op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f64_mul( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_LONG :
                        SCALE_FACTOR_ARITH_UFLOW_LONG );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1[4], op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            SET_SF_RM_FROM_FPC;

            op1[i] = f32_mul( op2, op3 );

            if (softfloat_exceptionFlags)
            {
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                ieee_trap_conds = ieee_exception_test_oux( regs );

                if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                    op1[i] = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                        SCALE_FACTOR_ARITH_OFLOW_SHORT :
                        SCALE_FACTOR_ARITH_UFLOW_SHORT );
            }

            VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3;

        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        SET_SF_RM_FROM_FPC;

        op1 = f128_mul( op2, op3 );

        if (softfloat_exceptionFlags)
        {
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            ieee_trap_conds = ieee_exception_test_oux( regs );

            if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                op1 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                    SCALE_FACTOR_ARITH_OFLOW_EXTD :
                    SCALE_FACTOR_ARITH_UFLOW_EXTD );
        }

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E8 VFCE   - Vector FP Compare Equal                     [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFCESB  V1,V2,V3     VFCE V1,V2,V3,2,0,0                        */
/*   VFCESBS V1,V2,V3     VFCE V1,V2,V3,2,0,1                        */
/*   VFCEDB  V1,V2,V3     VFCE V1,V2,V3,3,0,0                        */
/*   VFCEDBS V1,V2,V3     VFCE V1,V2,V3,3,0,1                        */
/*   WFCESB  V1,V2,V3     VFCE V1,V2,V3,2,8,0                        */
/*   WFCESBS V1,V2,V3     VFCE V1,V2,V3,2,8,1                        */
/*   WFCEDB  V1,V2,V3     VFCE V1,V2,V3,3,8,0                        */
/*   WFCEDBS V1,V2,V3     VFCE V1,V2,V3,3,8,1                        */
/*   WFCEXB  V1,V2,V3     VFCE V1,V2,V3,4,8,0                        */
/*   WFCEXBS V1,V2,V3     VFCE V1,V2,V3,4,8,1                        */
/*   VFKESB  V1,V2,V3     VFCE V1,V2,V3,2,4,0                        */
/*   VFKESBS V1,V2,V3     VFCE V1,V2,V3,2,4,1                        */
/*   VFKEDB  V1,V2,V3     VFCE V1,V2,V3,3,4,0                        */
/*   VFKEDBS V1,V2,V3     VFCE V1,V2,V3,3,4,1                        */
/*   WFKESB  V1,V2,V3     VFCE V1,V2,V3,2,12,0                       */
/*   WFKESBS V1,V2,V3     VFCE V1,V2,V3,2,12,1                       */
/*   WFKEDB  V1,V2,V3     VFCE V1,V2,V3,3,12,0                       */
/*   WFKEDBS V1,V2,V3     VFCE V1,V2,V3,3,12,1                       */
/*   WFKEXB  V1,V2,V3     VFCE V1,V2,V3,4,12,0                       */
DEF_INST( vector_fp_compare_equal )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    int     equal_found, not_equal_or_unordered_found;
    BYTE    newcc = 3;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_SQ ((m5 & 0x4) != 0) // Signal-on-QNaN (SQ)
#define M5_RE ((m5 & 0x3) != 0) // Reserved
#define M6_RE ((m6 & 0xE) != 0) // Reserved
#define M6_CS ((m6 & 0x1) != 0) // Condition Code Set (CS)

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M6_RE || M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M6_RE || M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    equal_found = not_equal_or_unordered_found = FALSE;

    if ( m4 == 3 )  // Long format
    {
        float64_t   op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            newcc = FLOAT64_COMPARE( op2, op3 );
            if (newcc == 3 && M5_SQ)
            {
                softfloat_exceptionFlags = softfloat_flag_invalid;
            }
            /* Xi is only trap that suppresses result, no return */
            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                SET_FPC_FLAGS_FROM_SF( regs );
            }

            if (newcc == 0)
            {
                equal_found = TRUE;
                regs->VR_D( v1, i ) = 0xFFFFFFFFFFFFFFFFULL;
            }
            else
            {
                not_equal_or_unordered_found = TRUE;
                regs->VR_D( v1, i ) = 0x0000000000000000ULL;
            }

            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            newcc = FLOAT32_COMPARE( op2, op3 );
            if (newcc == 3 && M5_SQ)
            {
                softfloat_exceptionFlags = softfloat_flag_invalid;
            }
            /* Xi is only trap that suppresses result, no return */
            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                SET_FPC_FLAGS_FROM_SF( regs );
            }

            if (newcc == 0)
            {
                equal_found = TRUE;
                regs->VR_F( v1, i ) = 0xFFFFFFFF;
            }
            else
            {
                not_equal_or_unordered_found = TRUE;
                regs->VR_F( v1, i ) = 0x00000000;
            }

            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op2, op3;

        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT128_COMPARE( op2, op3 );
        if (newcc == 3 && M5_SQ)
        {
            softfloat_exceptionFlags = softfloat_flag_invalid;
        }
        /* Xi is only trap that suppresses result, no return */
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );

        if (softfloat_exceptionFlags & softfloat_flag_invalid)
        {
            SET_FPC_FLAGS_FROM_SF( regs );
        }

        if (newcc == 0)
        {
            equal_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0xFFFFFFFFFFFFFFFFULL;
            regs->VR_Q( v1 ).d[1] = 0xFFFFFFFFFFFFFFFFULL;
        }
        else
        {
            not_equal_or_unordered_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0x0000000000000000ULL;
            regs->VR_Q( v1 ).d[1] = 0x0000000000000000ULL;
        }
    }

    //  Resulting Condition Code: When Bit 3 of the M6
    //  field is one, the code is set as follows:
    //  0 All elements equal (All T results in Figure 24-5)
    //  1 Mix of equal, and unequal or unordered elements
    //    (A mix of T and F results in Figure 24-5)
    //  2 --
    //  3 All elements not equal or unordered (All F results
    //    in Figure 24-5)
    //  Otherwise, the code remains unchanged.
    if M6_CS {
        if (equal_found == TRUE && not_equal_or_unordered_found == TRUE)
            regs->psw.cc = 1;
        else if (equal_found == FALSE && not_equal_or_unordered_found == TRUE)
            regs->psw.cc = 3;
        else
            regs->psw.cc = 0;
    }

#undef M5_SE
#undef M5_SQ
#undef M5_RE
#undef M6_RE
#undef M6_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7EA VFCHE  - Vector FP Compare High or Equal             [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFCHESB  V1,V2,V3    VFCHE V1,V2,V3,2,0,0                       */
/*   VFCHESBS V1,V2,V3    VFCHE V1,V2,V3,2,0,1                       */
/*   VFCHEDB  V1,V2,V3    VFCHE V1,V2,V3,3,0,0                       */
/*   VFCHEDBS V1,V2,V3    VFCHE V1,V2,V3,3,0,1                       */
/*   WFCHESB  V1,V2,V3    VFCHE V1,V2,V3,2,8,0                       */
/*   WFCHESBS V1,V2,V3    VFCHE V1,V2,V3,2,8,1                       */
/*   WFCHEDB  V1,V2,V3    VFCHE V1,V2,V3,3,8,0                       */
/*   WFCHEDBS V1,V2,V3    VFCHE V1,V2,V3,3,8,1                       */
/*   WFCHEXB  V1,V2,V3    VFCHE V1,V2,V3,4,8,0                       */
/*   WFCHEXBS V1,V2,V3    VFCHE V1,V2,V3,4,8,1                       */
/*   VFKHESB  V1,V2,V3    VFCHE V1,V2,V3,2,4,0                       */
/*   VFKHESBS V1,V2,V3    VFCHE V1,V2,V3,2,4,1                       */
/*   VFKHEDB  V1,V2,V3    VFCHE V1,V2,V3,3,4,0                       */
/*   VFKHEDBS V1,V2,V3    VFCHE V1,V2,V3,3,4,1                       */
/*   WFKHESB  V1,V2,V3    VFCHE V1,V2,V3,2,12,0                      */
/*   WFKHESBS V1,V2,V3    VFCHE V1,V2,V3,2,12,1                      */
/*   WFKHEDB  V1,V2,V3    VFCHE V1,V2,V3,3,12,0                      */
/*   WFKHEDBS V1,V2,V3    VFCHE V1,V2,V3,3,12,1                      */
/*   WFKHEXB  V1,V2,V3    VFCHE V1,V2,V3,4,12,0                      */
/*   WFKHEXBS V1,V2,V3    VFCHE V1,V2,V3,4,12,1                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_high_or_equal )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    int     greater_than_or_equal_found, less_than_or_unordered_found;
    BYTE    newcc = 3;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_SQ ((m5 & 0x4) != 0) // Signal-on-QNaN (SQ)
#define M5_RE ((m5 & 0x3) != 0) // Reserved
#define M6_RE ((m6 & 0xE) != 0) // Reserved
#define M6_CS ((m6 & 0x1) != 0) // Condition Code Set (CS)

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M6_RE || M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M6_RE || M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    greater_than_or_equal_found = less_than_or_unordered_found = FALSE;

    if ( m4 == 3 )  // Long format
    {
        float64_t   op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            newcc = FLOAT64_COMPARE( op2, op3 );
            if (newcc == 3 && M5_SQ)
            {
                softfloat_exceptionFlags = softfloat_flag_invalid;
            }
            /* Xi is only trap that suppresses result, no return */
            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                SET_FPC_FLAGS_FROM_SF( regs );
            }

            if (newcc == 0 || newcc == 2)
            {
                greater_than_or_equal_found = TRUE;
                regs->VR_D( v1, i ) = 0xFFFFFFFFFFFFFFFFULL;
            }
            else
            {
                less_than_or_unordered_found = TRUE;
                regs->VR_D( v1, i ) = 0x0000000000000000ULL;
            }

            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            newcc = FLOAT32_COMPARE( op2, op3 );
            if (newcc == 3 && M5_SQ)
            {
                softfloat_exceptionFlags = softfloat_flag_invalid;
            }
            /* Xi is only trap that suppresses result, no return */
            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                SET_FPC_FLAGS_FROM_SF( regs );
            }

            if (newcc == 0 || newcc == 2)
            {
                greater_than_or_equal_found = TRUE;
                regs->VR_F( v1, i ) = 0xFFFFFFFF;
            }
            else
            {
                less_than_or_unordered_found = TRUE;
                regs->VR_F( v1, i ) = 0x00000000;
            }

            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op2, op3;

        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT128_COMPARE( op2, op3 );
        if (newcc == 3 && M5_SQ)
        {
            softfloat_exceptionFlags = softfloat_flag_invalid;
        }
        /* Xi is only trap that suppresses result, no return */
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );

        if (softfloat_exceptionFlags & softfloat_flag_invalid)
        {
            SET_FPC_FLAGS_FROM_SF( regs );
        }

        if (newcc == 0 || newcc == 2)
        {
            greater_than_or_equal_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0xFFFFFFFFFFFFFFFFULL;
            regs->VR_Q( v1 ).d[1] = 0xFFFFFFFFFFFFFFFFULL;
        }
        else
        {
            less_than_or_unordered_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0x0000000000000000ULL;
            regs->VR_Q( v1 ).d[1] = 0x0000000000000000ULL;
        }
    }

    //  Resulting Condition Code: When Bit 3 of the M6
    //  field is one, the code is set as follows:
    //  0 All elements greater than or equal (All T results
    //    in Figure 24-7)
    //  1 Mix of greater than or equal and less than or
    //    unordered elements (A mix of T and F results in
    //    Figure 24-7)
    //  2 --
    //  3 All elements less than or unordered (All F results
    //    in Figure 24-7)
    //  Otherwise, the code remains unchanged.
    if M6_CS {
        if (greater_than_or_equal_found == TRUE && less_than_or_unordered_found == TRUE)
            regs->psw.cc = 1;
        else if (greater_than_or_equal_found == FALSE && less_than_or_unordered_found == TRUE)
            regs->psw.cc = 3;
        else
            regs->psw.cc = 0;
    }

#undef M5_SE
#undef M5_SQ
#undef M5_RE
#undef M6_RE
#undef M6_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7EB VFCH   - Vector FP Compare High                      [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFCHSB  V1,V2,V3     VFCH V1,V2,V3,2,0,0                        */
/*   VFCHSBS V1,V2,V3     VFCH V1,V2,V3,2,0,1                        */
/*   VFCHDB  V1,V2,V3     VFCH V1,V2,V3,3,0,0                        */
/*   VFCHDBS V1,V2,V3     VFCH V1,V2,V3,3,0,1                        */
/*   WFCHSB  V1,V2,V3     VFCH V1,V2,V3,2,8,0                        */
/*   WFCHSBS V1,V2,V3     VFCH V1,V2,V3,2,8,1                        */
/*   WFCHDB  V1,V2,V3     VFCH V1,V2,V3,3,8,0                        */
/*   WFCHDBS V1,V2,V3     VFCH V1,V2,V3,3,8,1                        */
/*   WFCHXB  V1,V2,V3     VFCH V1,V2,V3,4,8,0                        */
/*   WFCHXBS V1,V2,V3     VFCH V1,V2,V3,4,8,1                        */
/*   VFKHSB  V1,V2,V3     VFCH V1,V2,V3,2,4,0                        */
/*   VFKHSBS V1,V2,V3     VFCH V1,V2,V3,2,4,1                        */
/*   VFKHDB  V1,V2,V3     VFCH V1,V2,V3,3,4,0                        */
/*   VFKHDBS V1,V2,V3     VFCH V1,V2,V3,3,4,1                        */
/*   WFKHSB  V1,V2,V3     VFCH V1,V2,V3,2,12,0                       */
/*   WFKHSBS V1,V2,V3     VFCH V1,V2,V3,2,12,1                       */
/*   WFKHDB  V1,V2,V3     VFCH V1,V2,V3,3,12,0                       */
/*   WFKHDBS V1,V2,V3     VFCH V1,V2,V3,3,12,1                       */
/*   WFKHXB  V1,V2,V3     VFCH V1,V2,V3,4,12,0                       */
/*   WFKHXBS V1,V2,V3     VFCH V1,V2,V3,4,12,1                       */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_high )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    int     greater_than_found, not_greater_than_or_unordered_found;
    BYTE    newcc = 3;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_SQ ((m5 & 0x4) != 0) // Signal-on-QNaN (SQ)
#define M5_RE ((m5 & 0x3) != 0) // Reserved
#define M6_RE ((m6 & 0xE) != 0) // Reserved
#define M6_CS ((m6 & 0x1) != 0) // Condition Code Set (CS)

    if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( M6_RE || M5_RE || m4 < 2 || m4 > 4 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
    {
        if ( M6_RE || M5_RE || m4 != 3 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    greater_than_found = not_greater_than_or_unordered_found = FALSE;

    if ( m4 == 3 )  // Long format
    {
        float64_t   op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            newcc = FLOAT64_COMPARE( op2, op3 );
            if (newcc == 3 && M5_SQ)
            {
                softfloat_exceptionFlags = softfloat_flag_invalid;
            }
            /* Xi is only trap that suppresses result, no return */
            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                SET_FPC_FLAGS_FROM_SF( regs );
            }

            if (newcc == 2)
            {
                greater_than_found = TRUE;
                regs->VR_D( v1, i ) = 0xFFFFFFFFFFFFFFFFULL;
            }
            else
            {
                not_greater_than_or_unordered_found = TRUE;
                regs->VR_D( v1, i ) = 0x0000000000000000ULL;
            }

            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

            softfloat_exceptionFlags = 0;
            newcc = FLOAT32_COMPARE( op2, op3 );
            if (newcc == 3 && M5_SQ)
            {
                softfloat_exceptionFlags = softfloat_flag_invalid;
            }
            /* Xi is only trap that suppresses result, no return */
            VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

            if (softfloat_exceptionFlags & softfloat_flag_invalid)
            {
                SET_FPC_FLAGS_FROM_SF( regs );
            }

            if (newcc == 2)
            {
                greater_than_found = TRUE;
                regs->VR_F( v1, i ) = 0xFFFFFFFF;
            }
            else
            {
                not_greater_than_or_unordered_found = TRUE;
                regs->VR_F( v1, i ) = 0x00000000;
            }

            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op2, op3;

        VECTOR_GET_FLOAT128_OP( op3, v3, regs );
        VECTOR_GET_FLOAT128_OP( op2, v2, regs );

        softfloat_exceptionFlags = 0;
        newcc = FLOAT128_COMPARE( op2, op3 );
        if (newcc == 3 && M5_SQ)
        {
            softfloat_exceptionFlags = softfloat_flag_invalid;
        }
        /* Xi is only trap that suppresses result, no return */
        VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );

        if (softfloat_exceptionFlags & softfloat_flag_invalid)
        {
            SET_FPC_FLAGS_FROM_SF( regs );
        }

        if (newcc == 2)
        {
            greater_than_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0xFFFFFFFFFFFFFFFFULL;
            regs->VR_Q( v1 ).d[1] = 0xFFFFFFFFFFFFFFFFULL;
        }
        else
        {
            not_greater_than_or_unordered_found = TRUE;
            regs->VR_Q( v1 ).d[0] = 0x0000000000000000ULL;
            regs->VR_Q( v1 ).d[1] = 0x0000000000000000ULL;
        }
    }

    //  Resulting Condition Code: When Bit 3 of the M6
    //  field is one, the code is set as follows:
    //  0 All elements greater than (All T results in
    //    Figure 24-6)
    //  1 Mix of greater than, and non-greater than or
    //    unordered elements (A mix of T and F results in
    //    Figure 24-6)
    //  2 --
    //  3 All elements not greater than, or unordered (All F
    //    results in Figure 24-6)
    //  Otherwise, the code remains unchanged.
    if M6_CS {
        if (greater_than_found == TRUE && not_greater_than_or_unordered_found == TRUE)
            regs->psw.cc = 1;
        else if (greater_than_found == FALSE && not_greater_than_or_unordered_found == TRUE)
            regs->psw.cc = 3;
        else
            regs->psw.cc = 0;
    }

#undef M5_SE
#undef M5_SQ
#undef M5_RE
#undef M6_RE
#undef M6_CS

    ZVECTOR_END( regs );
}

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*-------------------------------------------------------------------*/
/* E7EE VFMIN  - Vector FP Minimum                           [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFMINSB V1,V2,V3,M6  VFMIN V1,V2,V3,2,0,M6                      */
/*   VFMINDB V1,V2,V3,M6  VFMIN V1,V2,V3,3,0,M6                      */
/*   WFMINSB V1,V2,V3,M6  VFMIN V1,V2,V3,2,8,M6                      */
/*   WFMINDB V1,V2,V3,M6  VFMIN V1,V2,V3,3,8,M6                      */
/*   WFMINXB V1,V2,V3,M6  VFMIN V1,V2,V3,4,8,M6                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_minimum )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, toqnan, result = 0;
    U32     op2_dataclass, op3_dataclass;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( (m6 >= 5 && m6 <= 7) || m6 > 12 || M5_RE || m4 < 2 || m4 > 4 )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    if ( m6 == 0 || m6 == 1 || m6 == 8 || m6 == 9 )
        toqnan = TRUE;
    else
        toqnan = FALSE;

    if ( m4 == 3 )  // Long format
    {
        float64_t   op1[2], op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );

            op2_dataclass = float64_class( op2 );
            op3_dataclass = float64_class( op3 );

            switch (m6)
            {
            case 0:   // IEEE MinNum
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_minnum( result );
                break;
            case 1:   // Java Math.Min()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_java_min( result );
                break;
            case 2:   // C-Style Min Macro
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cee_min( result );
                break;
            case 3:   // C++ algorithm.min()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_min( result );
                break;
            case 4:   // fmin()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_fmin( result );
                break;
            case 8:   // IEEE MinNumMag
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_minnummag( result );
                break;
            case 9:   // Java Math.Min() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_java_min_abs( result );
                break;
            case 10:  // C-Style Min Macro of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cee_min_abs( result );
                break;
            case 11:  // C++ algorithm.min() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_min_abs( result );
                break;
            case 12:  // fmin() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_fmin_abs( result );
                break;
            }

            softfloat_exceptionFlags = 0;

            switch (result)
            {
            case 1:
                if (f64_le_quiet( op2, op3 ))
                    op1[i] = op2;
                else
                    op1[i] = op3;
                break;
            case 2:
                op1[i] = op2;
                break;
            case 3:
                op1[i] = op3;
                break;
            case 4:
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT64_MAKE_QNAN( op2 );
                op1[i] = op2;
                break;
            case 5:
            default:  // Should not occur!
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT64_MAKE_QNAN( op3 );
                op1[i] = op3;
                break;
            }

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1[4], op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );

            op2_dataclass = float32_class( op2 );
            op3_dataclass = float32_class( op3 );

            switch (m6)
            {
            case 0:   // IEEE MinNum
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_minnum( result );
                break;
            case 1:   // Java Math.Min()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_java_min( result );
                break;
            case 2:   // C-Style Min Macro
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cee_min( result );
                break;
            case 3:   // C++ algorithm.min()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_min( result );
                break;
            case 4:   // fmin()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_fmin( result );
                break;
            case 8:   // IEEE MinNumMag
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_minnummag( result );
                break;
            case 9:   // Java Math.Min() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_java_min_abs( result );
                break;
            case 10:  // C-Style Min Macro of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cee_min_abs( result );
                break;
            case 11:  // C++ algorithm.min() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_min_abs( result );
                break;
            case 12:  // fmin() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_fmin_abs( result );
                break;
            }

            softfloat_exceptionFlags = 0;

            switch (result)
            {
            case 1:
                if (f32_le_quiet( op2, op3 ))
                    op1[i] = op2;
                else
                    op1[i] = op3;
                break;
            case 2:
                op1[i] = op2;
                break;
            case 3:
                op1[i] = op3;
                break;
            case 4:
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT32_MAKE_QNAN( op2 );
                op1[i] = op2;
                break;
            case 5:
            default:  // Should not occur!
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT32_MAKE_QNAN( op3 );
                op1[i] = op3;
                break;
            }

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );
        VECTOR_GET_FLOAT128_OP( op3, v3, regs );

        op2_dataclass = float128_class( op2 );
        op3_dataclass = float128_class( op3 );

        switch (m6)
        {
        case 0:   // IEEE MinNum
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_ieee_minnum( result );
            break;
        case 1:   // Java Math.Min()
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_java_min( result );
            break;
        case 2:   // C-Style Min Macro
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_cee_min( result );
            break;
        case 3:   // C++ algorithm.min()
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_cpp_min( result );
            break;
        case 4:   // fmin()
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_fmin( result );
            break;
        case 8:   // IEEE MinNumMag
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_ieee_minnummag( result );
            break;
        case 9:   // Java Math.Min() of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_java_min_abs( result );
            break;
        case 10:  // C-Style Min Macro of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_cee_min_abs( result );
            break;
        case 11:  // C++ algorithm.min() of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_cpp_min_abs( result );
            break;
        case 12:  // fmin() of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_fmin_abs( result );
            break;
        }

        softfloat_exceptionFlags = 0;

        switch (result)
        {
        case 1:
            if (f128_le_quiet( op2, op3 ))
                op1 = op2;
            else
                op1 = op3;
            break;
        case 2:
            op1 = op2;
            break;
        case 3:
            op1 = op3;
            break;
        case 4:
            softfloat_exceptionFlags = softfloat_flag_invalid;
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            SET_FPC_FLAGS_FROM_SF( regs );
            if (toqnan)
                FLOAT128_MAKE_QNAN( op2 );
            op1 = op2;
            break;
        case 5:
        default:  // Should not occur!
            softfloat_exceptionFlags = softfloat_flag_invalid;
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            SET_FPC_FLAGS_FROM_SF( regs );
            if (toqnan)
                FLOAT128_MAKE_QNAN( op3 );
            op1 = op3;
            break;
        }

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*-------------------------------------------------------------------*/
/* E7EF VFMAX  - Vector FP Maximum                           [VRR-c] */
/*                                                                   */
/*   Extended Mnemonic    Base Mnemonic                              */
/*   VFMAXSB V1,V2,V3,M6  VFMAX V1,V2,V3,2,0,M6                      */
/*   VFMAXDB V1,V2,V3,M6  VFMAX V1,V2,V3,3,0,M6                      */
/*   WFMAXSB V1,V2,V3,M6  VFMAX V1,V2,V3,2,8,M6                      */
/*   WFMAXDB V1,V2,V3,M6  VFMAX V1,V2,V3,3,8,M6                      */
/*   WFMAXXB V1,V2,V3,M6  VFMAX V1,V2,V3,4,8,M6                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_maximum )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, toqnan, result = 0;
    U32     op2_dataclass, op3_dataclass;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
#define M5_RE ((m5 & 0x7) != 0) // Reserved

    if ( (m6 >= 5 && m6 <= 7) || m6 > 12 || M5_RE || m4 < 2 || m4 > 4 )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    if ( m6 == 0 || m6 == 1 || m6 == 8 || m6 == 9 )
        toqnan = TRUE;
    else
        toqnan = FALSE;

    if ( m4 == 3 )  // Long format
    {
        float64_t   op1[2], op2, op3;

        for (i=0; i < 2; i++)
        {
            VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
            VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );

            op2_dataclass = float64_class( op2 );
            op3_dataclass = float64_class( op3 );

            switch (m6)
            {
            case 0:   // IEEE MaxNum
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_maxnum( result );
                break;
            case 1:   // Java Math.Max()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_java_max( result );
                break;
            case 2:   // C-Style Max Macro
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cee_max( result );
                break;
            case 3:   // C++ algorithm.max()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_max( result );
                break;
            case 4:   // fmax()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_fmax( result );
                break;
            case 8:   // IEEE MaxNumMag
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_maxnummag( result );
                break;
            case 9:   // Java Math.Max() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_java_max_abs( result );
                break;
            case 10:  // C-Style Max Macro of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cee_max_abs( result );
                break;
            case 11:  // C++ algorithm.max() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_max_abs( result );
                break;
            case 12:  // fmax() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_fmax_abs( result );
                break;
            }

            softfloat_exceptionFlags = 0;

            switch (result)
            {
            case 1:
                if (f64_le_quiet( op3, op2 ))
                    op1[i] = op2;
                else
                    op1[i] = op3;
                break;
            case 2:
                op1[i] = op2;
                break;
            case 3:
                op1[i] = op3;
                break;
            case 4:
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT64_MAKE_QNAN( op2 );
                op1[i] = op2;
                break;
            case 5:
            default:  // Should not occur!
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT64_MAKE_QNAN( op3 );
                op1[i] = op3;
                break;
            }

            if (M5_SE) break;
        }
        for (i=0; i < 2; i++)
        {
            VECTOR_PUT_FLOAT64_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1[4], op2, op3;

        for (i=0; i < 4; i++)
        {
            VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
            VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );

            op2_dataclass = float32_class( op2 );
            op3_dataclass = float32_class( op3 );

            switch (m6)
            {
            case 0:   // IEEE MaxNum
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_maxnum( result );
                break;
            case 1:   // Java Math.Max()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_java_max( result );
                break;
            case 2:   // C-Style Max Macro
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cee_max( result );
                break;
            case 3:   // C++ algorithm.max()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_max( result );
                break;
            case 4:   // fmax()
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_fmax( result );
                break;
            case 8:   // IEEE MaxNumMag
                result = calculate_result_one( op2_dataclass, op3_dataclass );
                result = convert_result_ieee_maxnummag( result );
                break;
            case 9:   // Java Math.Max() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_java_max_abs( result );
                break;
            case 10:  // C-Style Max Macro of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cee_max_abs( result );
                break;
            case 11:  // C++ algorithm.max() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_cpp_max_abs( result );
                break;
            case 12:  // fmax() of absolute values
                result = calculate_result_two( op2_dataclass, op3_dataclass );
                result = convert_result_fmax_abs( result );
                break;
            }

            softfloat_exceptionFlags = 0;

            switch (result)
            {
            case 1:
                if (f32_le_quiet( op3, op2 ))
                    op1[i] = op2;
                else
                    op1[i] = op3;
                break;
            case 2:
                op1[i] = op2;
                break;
            case 3:
                op1[i] = op3;
                break;
            case 4:
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT32_MAKE_QNAN( op2 );
                op1[i] = op2;
                break;
            case 5:
            default:  // Should not occur!
                softfloat_exceptionFlags = softfloat_flag_invalid;
                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                SET_FPC_FLAGS_FROM_SF( regs );
                if (toqnan)
                    FLOAT32_MAKE_QNAN( op3 );
                op1[i] = op3;
                break;
            }

            if (M5_SE) break;
        }
        for (i=0; i < 4; i++)
        {
            VECTOR_PUT_FLOAT32_NOCC( op1[i], v1, i, regs );
            if (M5_SE) break;
        }
    }
    else if ( m4 == 4 )  // Extended format
    {
        float128_t  op1, op2, op3;

        VECTOR_GET_FLOAT128_OP( op2, v2, regs );
        VECTOR_GET_FLOAT128_OP( op3, v3, regs );

        op2_dataclass = float128_class( op2 );
        op3_dataclass = float128_class( op3 );

        switch (m6)
        {
        case 0:   // IEEE MaxNum
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_ieee_maxnum( result );
            break;
        case 1:   // Java Math.Max()
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_java_max( result );
            break;
        case 2:   // C-Style Max Macro
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_cee_max( result );
            break;
        case 3:   // C++ algorithm.max()
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_cpp_max( result );
            break;
        case 4:   // fmax()
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_fmax( result );
            break;
        case 8:   // IEEE MaxNumMag
            result = calculate_result_one( op2_dataclass, op3_dataclass );
            result = convert_result_ieee_maxnummag( result );
            break;
        case 9:   // Java Math.Max() of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_java_max_abs( result );
            break;
        case 10:  // C-Style Max Macro of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_cee_max_abs( result );
            break;
        case 11:  // C++ algorithm.max() of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_cpp_max_abs( result );
            break;
        case 12:  // fmax() of absolute values
            result = calculate_result_two( op2_dataclass, op3_dataclass );
            result = convert_result_fmax_abs( result );
            break;
        }

        softfloat_exceptionFlags = 0;

        switch (result)
        {
        case 1:
            if (f128_le_quiet( op3, op2 ))
                op1 = op2;
            else
                op1 = op3;
            break;
        case 2:
            op1 = op2;
            break;
        case 3:
            op1 = op3;
            break;
        case 4:
            softfloat_exceptionFlags = softfloat_flag_invalid;
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            SET_FPC_FLAGS_FROM_SF( regs );
            if (toqnan)
                FLOAT128_MAKE_QNAN( op2 );
            op1 = op2;
            break;
        case 5:
        default:  // Should not occur!
            softfloat_exceptionFlags = softfloat_flag_invalid;
            VECTOR_IEEE_EXCEPTION_TRAP_XI( 0, regs );
            SET_FPC_FLAGS_FROM_SF( regs );
            if (toqnan)
                FLOAT128_MAKE_QNAN( op3 );
            op1 = op3;
            break;
        }

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );
    }

#undef M5_SE
#undef M5_RE

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

/*===========================================================================*/
/*  End of z/Arch Vector Floating Point instructions                         */
/*===========================================================================*/

/*********************************************************************/
/*  Some functions are 'generic' functions which are NOT dependent   */
/*  upon any specific build architecture and thus only need to be    */
/*  built once since they work identically for all architectures.    */
/*  Thus the below #define to prevent them from being built again.   */
/*  Also note that this #define must come BEFORE the #endif check    */
/*  for 'FEATURE_BINARY_FLOATING_POINT' or build errors occur.       */
/*********************************************************************/

#define _IEEE_NONARCHDEP_  /* (prevent rebuilding some code) */

#endif /* defined( FEATURE_129_ZVECTOR_FACILITY ) */

#endif /* defined( FEATURE_BINARY_FLOATING_POINT ) */


/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  // All of the architecture dependent (i.e. "ARCH_DEP") functions
  // for the default "_ARCH_NUM_0" build architecture have now just
  // been built (usually 370).

  // Now we need to build the same architecture dependent "ARCH_DEP"
  // functions for all of the OTHER build architectures that remain
  // (usually S/390 and z/Arch), so we #include ourselves again but
  // with the next build archiecture #defined instead...

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "zvector3.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "zvector3.c"
  #endif

#endif  /*!defined(_GEN_ARCH) */

/* end of zvector3.c */
