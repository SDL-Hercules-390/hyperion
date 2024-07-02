/* IEEE.C       (C) Copyright Roger Bowler and others, 2003-2012     */
/*              (C) Copyright Willem Konynenberg, 2001-2003          */
/*              (C) Copyright "Fish" (David B. Trout), 2011          */
/*              (C) Copyright Stephen R. Orso, 2016                  */
/*              (C) and others 2024                                  */
/*              Hercules Binary (IEEE) Floating Point Instructions   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

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
 * Hercules System/370, ESA/390, z/Architecture emulator ieee.c
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

#define _IEEE_C_
#define _HENGINE_DLL_

#if !defined( _MSVC_ ) && !defined( _GNU_SOURCE )
#error '_GNU_SOURCE' required for ieee.c!
#endif

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined( FEATURE_BINARY_FLOATING_POINT )

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

#include "softfloat.h"              /* Master SoftFloat #include header      */

/*****************************************************************************/

/* Default QNaN per SA22-7832-10 page 9-3:
   plus sign, quiet, and payload of zero
*/
static const float64_t   float64_default_qnan = { 0x7FF8000000000000ULL  };
static const float32_t   float32_default_qnan = { 0x7FC00000 };

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


/* Table of valid M3 values and macro to generate program check if      */
/* invalid BFP rounding method. Map of valid IBM M3 rounding mode       */
/* values when the Floating Point Extension Facility *IS* installed     */
const BYTE map_valid_m3_values_FPX[8] =
{
    1,                              /* M3 0: Use FPC BFP Rounding Mode  */
    1,                              /* M3 1: RNTA                       */
    0,                              /* M3 2: invalid                    */
    1,                              /* M3 3: RFS, substitute ties away  */
    1,                              /* M3 4: RNTE                       */
    1,                              /* M3 5: RZ                         */
    1,                              /* M3 6: RP                         */
    1,                              /* M3 7: RM                         */
};

/* Map of valid IBM M3 rounding mode values when                        */
/* the Floating Point Extension Facility is *NOT* installed             */
const BYTE map_valid_m3_values_NOFPX[8] =
{
    1,                              /* M3 0: Use FPC BFP Rounding Mode  */
    1,                              /* M3 1: RNTA                       */
    0,                              /* M3 2: invalid                    */
    0,                              /* M3 3: RFS, invalid without FPEF  */
    1,                              /* M3 4: RNTE                       */
    1,                              /* M3 5: RZ                         */
    1,                              /* M3 6: RP                         */
    1,                              /* M3 7: RM                         */
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

/* Determine condition code based on value of result operand    */

/* Determine cc from float132 value */
#define FLOAT128_CC( _op1 )    FLOAT128_ISNAN( _op1 )  ? 3 :                    \
                               !((_op1.v[FLOAT128_HI]  & 0x7FFFFFFFFFFFFFFFULL) \
                                | _op1.v[FLOAT128_LO]) ? 0 :                    \
                                  _op1.v[FLOAT128_HI]  & 0x8000000000000000ULL  \
                                                       ? 1 : 2
/* Determine cc from float64 value */
#define FLOAT64_CC( _op1 )      FLOAT64_ISNAN( _op1 )  ? 3 :                    \
                                !(_op1.v               & 0x7FFFFFFFFFFFFFFFULL) \
                                                       ? 0 :                    \
                                  _op1.v               & 0x8000000000000000ULL  \
                                                       ? 1 : 2

/* Determine cc from float32 value */
#define FLOAT32_CC( _op1 )      FLOAT32_ISNAN( _op1 )  ? 3 :                    \
                                !(_op1.v               & 0x7FFFFFFF)            \
                                                       ? 0 :                    \
                                  _op1.v               & 0x80000000             \
                                                       ? 1 : 2

// Data with DXC 08, IEEE inexact and truncated
// Data with DXC 0B, Simulated IEEE inexact
// Data with DXC 0C, IEEE inexact and incremented
// Data with DXC 10, IEEE underflow, exact
// Data with DXC 13, Simulated IEEE underflow, exact
// Data with DXC 18, IEEE underflow, inexact and truncated
// Data with DXC 1B, Simulated IEEE underflow, inexact
// Data with DXC 1C, IEEE underflow, inexact and incremented
// Data with DXC 20, IEEE overflow, exact
// Data with DXC 23, Simulated IEEE overflow, exact
// Data with DXC 28, IEEE overflow, inexact and truncated
// Data with DXC 2B, Simulated IEEE overflow, inexact
// Data with DXC 2C, IEEE overflow, inexact and incremented
// Data with DXC 40, IEEE division by zero
// Data with DXC 43, Simulated IEEE division by zero
// Data with DXC 80, IEEE invalid operation
// Data with DXC 83, Simulated IEEE invalid operation
static void ieee_trap( REGS *regs, BYTE dxc)
{
    regs->dxc = dxc;                   /*  Save DXC in PSA         */
    regs->fpc &= ~FPC_DXC;             /*  Clear previous DXC/VXC  */
    regs->fpc |= ((U32)dxc << FPC_DXC_SHIFT);
    regs->program_interrupt( regs, PGM_DATA_EXCEPTION );
}

static void ieee_cond_trap( REGS *regs, U32 ieee_traps )
{
    /* ieee_cond_trap is called before instruction completion for Xi    */
    /* and Xz traps, resulting in instruction suppression.              */
    /* For other instructions, it is called after instruction results   */
    /* have been stored.                                                */

    /* PROGRAMMING NOTE: for the underflow/overflow and inexact         */
    /* data exceptions, SoftFloat does not distinguish between          */
    /* inexact and truncated or inexact and incremented, so neither     */
    /* can we. Thus for now we will always return "truncated."          */
    /* We can tell the difference between exact and inexact.            */

    /* We will add inexact and incremented when we get Softfloat up to  */
    /* speed on returning and incremented indication.                   */

    /* Note: switch/case does not work here because multiple            */
    /* exceptions may be passed by the caller.                          */

    if (ieee_traps & FPC_MASK_IMI)
        ieee_trap( regs, DXC_IEEE_INVALID_OP );

    else if (ieee_traps & FPC_MASK_IMZ)
        ieee_trap( regs, DXC_IEEE_DIV_ZERO );

    else if (ieee_traps & FPC_MASK_IMO)
        ieee_trap( regs, !(softfloat_exceptionFlags & softfloat_flag_inexact)    ? DXC_IEEE_OF_EXACT :
                           softfloat_exceptionFlags & softfloat_flag_incremented ? DXC_IEEE_OF_INEX_INCR
                                                                                 : DXC_IEEE_OF_INEX_TRUNC );
    else if (ieee_traps & FPC_MASK_IMU)
        ieee_trap( regs, !(softfloat_exceptionFlags & softfloat_flag_inexact)    ? DXC_IEEE_UF_EXACT :
                           softfloat_exceptionFlags & softfloat_flag_incremented ? DXC_IEEE_UF_INEX_INCR
                                                                                 : DXC_IEEE_UF_INEX_TRUNC );
    else if (ieee_traps & FPC_MASK_IMX)
        ieee_trap( regs,   softfloat_exceptionFlags & softfloat_flag_incremented ? DXC_IEEE_INEXACT_INCR
                                                                                 : DXC_IEEE_INEXACT_TRUNC );
}

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

/* Save detected exceptions that are trap-enabled                      */

#define IEEE_EXCEPTION_TEST_TRAPS( _regs, _ieee_trap_conds, _exceptions )   \
                                                                            \
      _ieee_trap_conds = (_regs->fpc & FPC_MASKS) & (((U32)softfloat_exceptionFlags) << FPC_MASK_SHIFT) & (_exceptions)

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


/* fastpath test for Xi trap; many instructions only return Xi */
#define IEEE_EXCEPTION_TRAP_XI( _regs )                              \
                                                                     \
    if (1                                                            \
        && (softfloat_exceptionFlags & softfloat_flag_invalid)       \
        && (_regs->fpc & FPC_MASK_IMI)                               \
    )                                                                \
        ieee_trap( _regs, DXC_IEEE_INVALID_OP )


/* fastpath test for Xz trap; only Divide returns Xz  */
#define IEEE_EXCEPTION_TRAP_XZ( _regs )                              \
                                                                     \
    if (1                                                            \
        && (softfloat_exceptionFlags & softfloat_flag_infinite)      \
        && (_regs->fpc & FPC_MASK_IMZ)                               \
    )                                                                \
        ieee_trap( _regs, DXC_IEEE_DIV_ZERO )


/* trap if any provided exception has been previously detected */
#define IEEE_EXCEPTION_TRAP( _regs, _ieee_trap_conds, _exceptions )  \
                                                                     \
    if (_ieee_trap_conds & (_exceptions))                            \
        ieee_cond_trap( _regs, _ieee_trap_conds)


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


/* Macro and funtion to validate rounding mode specified on M3 or M4 field   */
/* of selected instructions. Note that this table is architecture dependent. */

static INLINE void ARCH_DEP( BFP_RM_check )( REGS* regs, BYTE mask )
{
#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
    {
        if (mask > 7 || !map_valid_m3_values_FPX[ (mask & 0x7) ])
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }
    else
#endif /* defined FEATURE_037_FP_EXTENSION_FACILITY */
    {
        if (mask > 7 || !map_valid_m3_values_NOFPX[ (mask & 0x7) ])
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }
}

#undef  BFPRM_CHECK
#define BFPRM_CHECK( _mask, _regs )   ARCH_DEP( BFP_RM_check )((_regs),(_mask))

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

#undef VFETCH_FLOAT64_OP
#undef VFETCH_FLOAT32_OP

#define VFETCH_FLOAT64_OP( op, effective_addr, arn, regs )  op.v = ARCH_DEP( vfetch8 )( effective_addr, arn, regs )
#define VFETCH_FLOAT32_OP( op, effective_addr, arn, regs )  op.v = ARCH_DEP( vfetch4 )( effective_addr, arn, regs )

#undef GET_FLOAT128_OP
#undef GET_FLOAT64_OP
#undef GET_FLOAT32_OP

#define GET_FLOAT128_OP( op, r, regs )  ARCH_DEP( get_float128 )( &op, &regs->FPR_L( r ), &regs->FPR_L( r+2 ))
#define GET_FLOAT64_OP(  op, r, regs )  ARCH_DEP( get_float64  )( &op, &regs->FPR_L( r ))
#define GET_FLOAT32_OP(  op, r, regs )  ARCH_DEP( get_float32  )( &op, &regs->FPR_S( r ))

#undef GET_FLOAT128_OPS
#undef GET_FLOAT64_OPS
#undef GET_FLOAT32_OPS

#define GET_FLOAT128_OPS( op1, r1, op2, r2, regs )  \
    do {                                            \
        GET_FLOAT128_OP( op1, r1, regs );           \
        GET_FLOAT128_OP( op2, r2, regs );           \
    } while (0)

#define GET_FLOAT64_OPS( op1, r1, op2, r2, regs )   \
    do {                                            \
        GET_FLOAT64_OP( op1, r1, regs );            \
        GET_FLOAT64_OP( op2, r2, regs );            \
    } while (0)

#define GET_FLOAT32_OPS( op1, r1, op2, r2, regs )   \
    do {                                            \
        GET_FLOAT32_OP( op1, r1, regs );            \
        GET_FLOAT32_OP( op2, r2, regs );            \
    } while (0)

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

static INLINE BYTE ARCH_DEP( float128_signaling_compare )( float128_t op1, float128_t op2 )
{
    if (FLOAT128_ISNAN( op1 ) ||
        FLOAT128_ISNAN( op2 ))
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

static INLINE BYTE ARCH_DEP( float64_signaling_compare )( float64_t op1, float64_t op2 )
{
    if (FLOAT64_ISNAN( op1 ) ||
        FLOAT64_ISNAN( op2 ))
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

static INLINE BYTE ARCH_DEP( float32_signaling_compare )( float32_t op1, float32_t op2 )
{
    if (FLOAT32_ISNAN( op1 ) ||
        FLOAT32_ISNAN( op2 ))
        softfloat_raiseFlags( softfloat_flag_invalid );
    return ARCH_DEP( float32_cc_quiet )( op1, op2 );
}

#undef FLOAT128_COMPARE
#undef FLOAT64_COMPARE
#undef FLOAT32_COMPARE

#define FLOAT128_COMPARE( op1, op2 )  ARCH_DEP( float128_compare )( op1, op2 )
#define FLOAT64_COMPARE(  op1, op2 )  ARCH_DEP( float64_compare  )( op1, op2 )
#define FLOAT32_COMPARE(  op1, op2 )  ARCH_DEP( float32_compare  )( op1, op2 )

#undef FLOAT128_COMPARE_AND_SIGNAL
#undef FLOAT64_COMPARE_AND_SIGNAL
#undef FLOAT32_COMPARE_AND_SIGNAL

#define FLOAT128_COMPARE_AND_SIGNAL( op1, op2 )  ARCH_DEP( float128_signaling_compare )( op1, op2 )
#define FLOAT64_COMPARE_AND_SIGNAL(  op1, op2 )  ARCH_DEP( float64_signaling_compare  )( op1, op2 )
#define FLOAT32_COMPARE_AND_SIGNAL(  op1, op2 )  ARCH_DEP( float32_signaling_compare  )( op1, op2 )

#undef PUT_FLOAT128_NOCC
#undef PUT_FLOAT64_NOCC
#undef PUT_FLOAT32_NOCC

#define PUT_FLOAT128_NOCC( op, r, regs )  ARCH_DEP( put_float128 )( &op, &regs->FPR_L( r ), &regs->FPR_L( r+2 ))
#define PUT_FLOAT64_NOCC(  op, r, regs )  ARCH_DEP( put_float64  )( &op, &regs->FPR_L( r ))
#define PUT_FLOAT32_NOCC(  op, r, regs )  ARCH_DEP( put_float32  )( &op, &regs->FPR_S( r ))

#undef PUT_FLOAT128_CC
#undef PUT_FLOAT64_CC
#undef PUT_FLOAT32_CC

#define PUT_FLOAT128_CC( op, r, regs )                                           \
                                                                                 \
    do {                                                                         \
        ARCH_DEP( put_float128 )( &op, &regs->FPR_L( r ), &regs->FPR_L( r+2 ));  \
        regs->psw.cc = FLOAT128_CC( op );                                        \
    } while (0)


#define PUT_FLOAT64_CC( op, r, regs )                      \
                                                           \
    do {                                                   \
        ARCH_DEP( put_float64 )( &op, &regs->FPR_L( r ));  \
        regs->psw.cc = FLOAT64_CC( op );                   \
    } while (0)


#define PUT_FLOAT32_CC( op, r, regs )                      \
                                                           \
    do {                                                   \
        ARCH_DEP( put_float32 )( &op, &regs->FPR_S( r ));  \
        regs->psw.cc = FLOAT32_CC( op );                   \
    } while (0)


/*                          ---  E N D  ---                                  */
/*                                                                           */
/*           'SoftFloat' IEEE Binary Floating Point package                  */
/*****************************************************************************/

#if !defined( _IEEE_NONARCHDEP_ )

#if !defined( HAVE_MATH_H ) && (_MSC_VER < VS2015)
/* Avoid double definition for VS2015 (albeit with different values). */
/* All floating-point numbers can be put in one of these categories.  */
enum
{
    FP_NAN          =  0,
    FP_INFINITE     =  1,
    FP_ZERO         =  2,
    FP_SUBNORMAL    =  3,
    FP_NORMAL       =  4
};
#endif /*!defined( HAVE_MATH_H ) && (_MSC_VER < VS2015) */

/*
 * Classify emulated fp values
 */
static int lbfpclassify( struct lbfp* op )
{
    if (op->exp == 0)
    {
        if (op->fract == 0)
            return FP_ZERO;
        else
            return FP_SUBNORMAL;
    }
    else if (op->exp == 0x7FF)
    {
        if (op->fract == 0)
            return FP_INFINITE;
        else
            return FP_NAN;
    }
    else
        return FP_NORMAL;
}

static int sbfpclassify( struct sbfp* op )
{
    if (op->exp == 0)
    {
        if (op->fract == 0)
            return FP_ZERO;
        else
            return FP_SUBNORMAL;
    }
    else if (op->exp == 0xFF)
    {
        if (op->fract == 0)
            return FP_INFINITE;
        else
            return FP_NAN;
    }
    else
        return FP_NORMAL;
}

/*
 * Get/fetch binary float from registers/memory
 */
static void get_lbfp( struct lbfp* op, U64* fpr )
{
    op->sign  = (*fpr & 0x8000000000000000ULL) != 0;
    op->exp   = (*fpr & 0x7FF0000000000000ULL) >> 52;
    op->fract = *fpr & 0x000FFFFFFFFFFFFFULL;

    //logmsg("lget r=%8.8x%8.8x exp=%d fract=%"PRIx64"\n", fpr[0], fpr[1], op->exp, op->fract);
}

static void get_sbfp( struct sbfp* op, U32* fpr )
{
    op->sign  = (*fpr & 0x80000000) != 0;
    op->exp   = (*fpr & 0x7F800000) >> 23;
    op->fract = *fpr & 0x007FFFFF;

    //logmsg("sget r=%8.8x exp=%d fract=%x\n", *fpr, op->exp, op->fract);
}

/*
 * Put binary float in registers
 */
static void put_lbfp( struct lbfp* op, U64* fpr )
{
    *fpr = (op->sign ? (U64)1 << 63 : 0)
         | ((U64)op->exp << 52)
         | (op->fract);

    //logmsg("lput exp=%d fract=%"PRIx64" r=%8.8x%8.8x\n", op->exp, op->fract, fpr[0], fpr[1]);
}

static void put_sbfp( struct sbfp* op, U32* fpr )
{
    *fpr = ((U32)op->sign ? 1 << 31 : 0)
         | ((U32)op->exp << 23)
         | (op->fract);

    //logmsg("sput exp=%d fract=%x r=%8.8x\n", op->exp, op->fract, *fpr);
}

#endif /* !defined( _IEEE_NONARCHDEP_ ) */

/*
 * Chapter 9. Floating-Point Overview and Support Instructions
 */

#if defined( FEATURE_FPS_EXTENSIONS )
#if !defined( _CBH_FUNC )
/*
 * Convert binary floating point to hexadecimal long floating point
 * save result into long register and return condition code
 * Roger Bowler, 19 July 2003
 */
static int cnvt_bfp_to_hfp( struct lbfp* op, int fpclass, U64* fpr )
{
    int exp;
    U64 fract;
    U32 r0, r1;
    int cc;

    switch (fpclass) {
    default:
    case FP_NAN:
        r0 = 0x7FFFFFFF;
        r1 = 0xFFFFFFFF;
        cc = 3;
        break;
    case FP_INFINITE:
        r0 = op->sign ? 0xFFFFFFFF : 0x7FFFFFFF;
        r1 = 0xFFFFFFFF;
        cc = 3;
        break;
    case FP_ZERO:
        r0 = op->sign ? 0x80000000 : 0;
        r1 = 0;
        cc = 0;
        break;
    case FP_SUBNORMAL:
        r0 = op->sign ? 0x80000000 : 0;
        r1 = 0;
        cc = op->sign ? 1 : 2;
        break;
    case FP_NORMAL:
        //logmsg("ieee: exp=%d (X\'%3.3x\')\tfract=%16.16"PRIx64"\n",
        //        op->exp, op->exp, op->fract);
        /* Insert an implied 1. in front of the 52 bit binary
           fraction and lengthen the result to 56 bits */
        fract = (U64)(op->fract | 0x10000000000000ULL) << 3;

        /* The binary exponent is equal to the biased exponent - 1023
           adjusted by 1 to move the point before the 56 bit fraction */
        exp = op->exp - 1023 + 1;

        //logmsg("ieee: adjusted exp=%d\tfract=%16.16"PRIx64"\n", exp, fract);
        /* Shift the fraction right one bit at a time until
           the binary exponent becomes a multiple of 4 */
        while (exp & 3)
        {
            exp++;
            fract >>= 1;
        }
        //logmsg("ieee:  shifted exp=%d\tfract=%16.16"PRIx64"\n", exp, fract);

        /* Convert the binary exponent into a hexadecimal exponent
           by dropping the last two bits (which are now zero) */
        exp >>= 2;

        /* If the hexadecimal exponent is less than -64 then return
           a signed zero result with a non-zero condition code */
        if (exp < -64) {
            r0 = op->sign ? 0x80000000 : 0;
            r1 = 0;
            cc = op->sign ? 1 : 2;
            break;
        }

        /* If the hexadecimal exponent exceeds +63 then return
           a signed maximum result with condition code 3 */
        if (exp > 63) {
            r0 = op->sign ? 0xFFFFFFFF : 0x7FFFFFFF;
            r1 = 0xFFFFFFFF;
            cc = 3;
            break;
        }

        /* Convert the hexadecimal exponent to a characteristic
           by adding 64 */
        exp += 64;

        /* Pack the exponent and the fraction into the result */
        r0 = (op->sign ? 1 << 31 : 0) | (exp << 24) | (fract >> 32);
        r1 = fract & 0xFFFFFFFF;
        cc = op->sign ? 1 : 2;
        break;
    }
    /* Store high and low halves of result into fp register array
       and return condition code */
    *fpr = ((U64)r0 << 32) | (U64)r1;
    return cc;
} /* end function cnvt_bfp_to_hfp */

/*
 * Convert hexadecimal long floating point register to
 * binary floating point and return condition code
 * Roger Bowler, 28 Nov 2004
 */

/* Definitions of BFP rounding methods */
#define RM_DEFAULT_ROUNDING             0
#define RM_BIASED_ROUND_TO_NEAREST      1
#define RM_ROUND_TO_NEAREST             4
#define RM_ROUND_TOWARD_ZERO            5
#define RM_ROUND_TOWARD_POS_INF         6
#define RM_ROUND_TOWARD_NEG_INF         7

static int cnvt_hfp_to_bfp
(
    U64*  fpr,
    int   rounding,
    int   bfp_fractbits,
    int   bfp_emax,
    int   bfp_ebias,
    int*  result_sign,
    int*  result_exp,
    U64*  result_fract
)
{
    BYTE   sign;
    short  expo;
    U64    fract;
    int    roundup = 0;
    int    cc;
    U64    b;

    /* Break the source operand into sign, characteristic, fraction */
    sign  = *fpr >> 63;
    expo  = (*fpr >> 56) & 0x007F;
    fract = *fpr & 0x00FFFFFFFFFFFFFFULL;

    /* Determine whether to round up or down */
    switch (rounding)
    {
        case RM_BIASED_ROUND_TO_NEAREST:
        case RM_ROUND_TO_NEAREST:     roundup =         0;      break;

        case RM_DEFAULT_ROUNDING:
        case RM_ROUND_TOWARD_ZERO:    roundup =         0;      break;
        case RM_ROUND_TOWARD_POS_INF: roundup = (sign ? 0 : 1); break;
        case RM_ROUND_TOWARD_NEG_INF: roundup =  sign;          break;
    }

    /* Convert HFP zero to BFP zero and return cond code 0 */
    if (fract == 0) /* a = -0 or +0 */
    {
        *result_sign  = sign;
        *result_exp   = 0;
        *result_fract = 0;
        return 0;
    }

    /* Set the condition code */
    cc = sign ? 1 : 2;

    /* Convert the HFP characteristic to a true binary exponent */
    expo = (expo - 64) * 4;

    /* Convert true binary exponent to a biased exponent */
    expo += bfp_ebias;

    /* Shift the fraction left until leftmost 1 is in bit 8 */
    while ((fract & 0x0080000000000000ULL) == 0)
    {
        fract <<= 1;
        expo  -=  1;
    }

    /* Convert 56-bit fraction to 55-bit with implied 1 */
    expo--;
    fract &= 0x007FFFFFFFFFFFFFULL;

    if (expo < -(bfp_fractbits-1)) /* |a| < Dmin */
    {
        if (expo == -(bfp_fractbits-1) - 1)
        {
            if (0
                || rounding == RM_BIASED_ROUND_TO_NEAREST
                || rounding == RM_ROUND_TO_NEAREST
            )
                roundup = 1;
        }

        if (roundup) { expo = 0; fract = 1; }   /* Dmin */
        else         { expo = 0; fract = 0; }   /* Zero */
    }
    else if (expo < 1)                          /* Dmin <= |a| < Nmin */
    {
        /* Reinstate implied 1 in preparation for denormalization */
        fract |= 0x0080000000000000ULL;

        /* Denormalize to get exponent back in range */
        fract >>= (expo + (bfp_fractbits-1));
        expo    =  0;
    }
    else if (expo > (bfp_emax+bfp_ebias))       /* |a| > Nmax */
    {
        cc = 3;

        if (roundup)    /* Inf */
        {
            expo  = (bfp_emax+bfp_ebias) + 1;
            fract = 0;
        }
        else            /* Nmax */
        {
            expo  = (bfp_emax+bfp_ebias);
            fract = 0x007FFFFFFFFFFFFFULL - (((U64)1<<(1+(55-bfp_fractbits)))-1);
        }
    }

    /* Set the result sign and exponent */
    *result_sign = sign;
    *result_exp  = expo;

    /* Apply rounding before truncating to final fraction length */
    b = ( (U64)1 ) << ( 55 - bfp_fractbits);

    if (roundup && (fract & b))
        fract += b;

    /* Convert 55-bit fraction to result fraction length */
    *result_fract = fract >> (55-bfp_fractbits);

    return cc;
} /* end function cnvt_hfp_to_bfp */

#define _CBH_FUNC
#endif /*!defined(_CBH_FUNC)*/

/*-------------------------------------------------------------------*/
/* B359 THDR  - CONVERT BFP TO HFP (long)                      [RRE] */
/* Roger Bowler, 19 July 2003                                        */
/*-------------------------------------------------------------------*/
DEF_INST(convert_bfp_long_to_float_long_reg)
{
    int r1, r2;
    struct lbfp op2;

    RRE(inst, regs, r1, r2);
    TXFC_INSTR_CHECK( regs );
    //logmsg("THDR r1=%d r2=%d\n", r1, r2);
    HFPREG2_CHECK(r1, r2, regs);

    /* Load lbfp operand from R2 register */
    get_lbfp(&op2, &regs->FPR_L(r2));

    /* Convert to hfp register and set condition code */
    regs->psw.cc =
        cnvt_bfp_to_hfp (&op2,
                         lbfpclassify(&op2),
                         &regs->FPR_L(r1));

} /* end DEF_INST(convert_bfp_long_to_float_long_reg) */

/*-------------------------------------------------------------------*/
/* B358 THDER - CONVERT BFP TO HFP (short to long)             [RRE] */
/* Roger Bowler, 19 July 2003                                        */
/*-------------------------------------------------------------------*/
DEF_INST(convert_bfp_short_to_float_long_reg)
{
    int r1, r2;
    struct sbfp op2;
    struct lbfp lbfp_op2;

    RRE(inst, regs, r1, r2);
    TXFC_INSTR_CHECK( regs );
    //logmsg("THDER r1=%d r2=%d\n", r1, r2);
    HFPREG2_CHECK(r1, r2, regs);

    /* Load sbfp operand from R2 register */
    get_sbfp(&op2, &regs->FPR_S(r2));

    /* Lengthen sbfp operand to lbfp */
    lbfp_op2.sign = op2.sign;
    lbfp_op2.exp = op2.exp - 127 + 1023;
    lbfp_op2.fract = (U64)op2.fract << (52 - 23);

    /* Convert lbfp to hfp register and set condition code */
    regs->psw.cc =
        cnvt_bfp_to_hfp (&lbfp_op2,
                         sbfpclassify(&op2),
                         &regs->FPR_L(r1));

} /* end DEF_INST(convert_bfp_short_to_float_long_reg) */

/*-------------------------------------------------------------------*/
/* B351 TBDR  - CONVERT HFP TO BFP (long)                    [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_long_to_bfp_long_reg)
{
    int r1, r2, m3;
    struct lbfp op1;

    RRF_M(inst, regs, r1, r2, m3);
    TXFC_INSTR_CHECK( regs );
    //logmsg("TBDR r1=%d r2=%d\n", r1, r2);
    HFPREG2_CHECK(r1, r2, regs);
    BFPRM_CHECK(m3,regs);

    regs->psw.cc =
        cnvt_hfp_to_bfp (&regs->FPR_L(r2), m3,
            /*fractbits*/52, /*emax*/1023, /*ebias*/1023,
            &(op1.sign), &(op1.exp), &(op1.fract));

    put_lbfp(&op1, &regs->FPR_L(r1));

} /* end DEF_INST(convert_float_long_to_bfp_long_reg) */

/*-------------------------------------------------------------------*/
/* B350 TBEDR - CONVERT HFP TO BFP (long to short)           [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_float_long_to_bfp_short_reg)
{
    int r1, r2, m3;
    struct sbfp op1;
    U64 fract;

    RRF_M(inst, regs, r1, r2, m3);
    TXFC_INSTR_CHECK( regs );
    //logmsg("TBEDR r1=%d r2=%d\n", r1, r2);
    HFPREG2_CHECK(r1, r2, regs);
    BFPRM_CHECK(m3,regs);

    regs->psw.cc =
        cnvt_hfp_to_bfp (&regs->FPR_L(r2), m3,
            /*fractbits*/23, /*emax*/127, /*ebias*/127,
            &(op1.sign), &(op1.exp), &fract);
    op1.fract = (U32)fract;

    put_sbfp(&op1, &regs->FPR_S(r1));

} /* end DEF_INST(convert_float_long_to_bfp_short_reg) */
#endif /*defined(FEATURE_FPS_EXTENSIONS)*/

/*-------------------------------------------------------------------*/
/* B34A AXBR  - ADD (extended BFP)                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( add_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op1, op2, ans;
    U32         ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f128_add( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );

        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_EXTD :
                SCALE_FACTOR_ARITH_UFLOW_EXTD );
    }

    PUT_FLOAT128_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B31A ADBR  - ADD (long BFP)                                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( add_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op1, op2, ans;
    int        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_add( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED1A ADB   - ADD (long BFP)                                 [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( add_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1, op2, ans;
    int        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op1, r1, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_add( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B30A AEBR  - ADD (short BFP)                                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( add_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op1, op2, ans;
    int        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_add( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED0A AEB   - ADD (short BFP)                                [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( add_bfp_short )
{
    int         r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2, ans;
    int        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_add( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B349 CXBR  - COMPARE (extended BFP)                         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op1, op2;
    BYTE        newcc;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT128_COMPARE( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );

    SET_FPC_FLAGS_FROM_SF( regs );

    /* Xi is only possible exception detected for Compare   */
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* B319 CDBR  - COMPARE (long BFP)                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op1, op2;
    BYTE       newcc;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT64_COMPARE( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* ED19 CDB   - COMPARE (long BFP)                             [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1, op2;
    BYTE       newcc;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    GET_FLOAT64_OP( op1, r1, regs );

    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT64_COMPARE( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* B309 CEBR  - COMPARE (short BFP)                            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op1, op2;
    BYTE       newcc;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT32_COMPARE( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* ED09 CEB   - COMPARE (short BFP)                            [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_bfp_short )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2;
    BYTE       newcc;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT32_COMPARE( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* B348 KXBR  - COMPARE AND SIGNAL (extended BFP)              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_and_signal_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op1, op2;
    BYTE        newcc;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT128_COMPARE_AND_SIGNAL( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* B318 KDBR  - COMPARE AND SIGNAL (long BFP)                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_and_signal_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op1, op2;
    BYTE       newcc;

    RRE(inst, regs, r1, r2);

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK(regs);
    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT64_COMPARE_AND_SIGNAL( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* ED18 KDB   - COMPARE AND SIGNAL (long BFP)                  [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_and_signal_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1, op2;
    BYTE       newcc;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op1, r1, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT64_COMPARE_AND_SIGNAL( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* B308 KEBR  - COMPARE AND SIGNAL (short BFP)                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_and_signal_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op1, op2;
    BYTE       newcc;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT32_COMPARE_AND_SIGNAL( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*-------------------------------------------------------------------*/
/* ED08 KEB   - COMPARE AND SIGNAL (short BFP)                 [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( compare_and_signal_bfp_short )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2;
    BYTE       newcc;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    newcc = FLOAT32_COMPARE_AND_SIGNAL( op1, op2 );

    /* Xi is only trap that suppresses result, no return */
    IEEE_EXCEPTION_TRAP_XI( regs );
    SET_FPC_FLAGS_FROM_SF( regs );
    regs->psw.cc = newcc;
}

/*--------------------------------------------------------------------------*/
/* CONVERT FROM FIXED                                                       */
/*                                                                          */
/* Input is a signed integer; Xo, Xu, and Xx are only exceptions possible   */
/*                                                                          */
/* If FEATURE_FLOATING_POINT_EXTENSION FACILITY installed (enabled)         */
/*   M3 field controls rounding, 0=Use FPC BRM                              */
/*   M4 field bit 0x04 XxC (inexact) suppresses inexact exception: no       */
/*   inexact trap or FPC status flag.                                       */
/*                                                                          */
/* If Floating Point Extension Facility not installed                       */
/*   M3 & M4 must be zero else program check specification exception        */
/*   Rounding is controlled by the BFP Rounding Mode in the FPC             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* B396 CXFBR  - CONVERT FROM FIXED (32 to extended BFP)        [RRF-e] */
/* B396 CXFBRA - CONVERT FROM FIXED (32 to extended BFP)        [RRF-e] */
/*                                                                      */
/* Fixed 32-bit always fits in Extended BFP; no exceptions possible     */
/*                                                                      */
/*----------------------------------------------------------------------*/
DEF_INST( convert_fix32_to_bfp_ext_reg )
{
    int         r1, r2;
    BYTE        m3, m4;
    S32         op2;
    float128_t  op1;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        BFPRM_CHECK( m3, regs );
    else
#endif
    {
        if (m3 | m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    op2 = regs->GR_L( r2 );
    SET_SF_RM_FROM_MASK( m3 );
    softfloat_exceptionFlags = 0;
    op1 = i32_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}

/*----------------------------------------------------------------------*/
/* B395 CDFBR  - CONVERT FROM FIXED (32 to long BFP)            [RRF-e] */
/* B395 CDFBRA - CONVERT FROM FIXED (32 to long BFP)            [RRF-e] */
/*                                                                      */
/* Fixed 32-bit always fits in long BFP; no exceptions possible         */
/*----------------------------------------------------------------------*/
DEF_INST( convert_fix32_to_bfp_long_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    S32        op2;
    float64_t  op1;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        BFPRM_CHECK( m3, regs );
    else
#endif
    {
        if (m3 | m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_L( r2 );
    softfloat_exceptionFlags = 0;
    op1 = i32_to_f64( op2 );

    PUT_FLOAT64_NOCC( op1, r1, regs );
}

/*--------------------------------------------------------------------------*/
/* B394 CEFBR  - CONVERT FROM FIXED (32 to short BFP)               [RRF-e] */
/* B394 CEFBRA - CONVERT FROM FIXED (32 to short BFP)               [RRF-e] */
/*                                                                          */
/* Fixed 32-bit may need to be rounded to fit in the 23+1 bits available    */
/* in a short BFP, IEEE Inexact may be raised.  If m4 Inexact suppression   */
/* (XxC) is on, then no inexact exception is recognized (no trap nor flag   */
/* set).                                                                    */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_fix32_to_bfp_short_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    S32        op2;
    float32_t  op1;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        BFPRM_CHECK( m3, regs );
    else
#endif
    {
        if (m3 | m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_L( r2 );
    softfloat_exceptionFlags = 0;
    op1 = i32_to_f32( op2 );
    PUT_FLOAT32_NOCC( op1, r1, regs );

    /* Inexact occurred and not masked by m4? */
    if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
    {
        /* Yes, set FPC flags and test for a trap */
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*----------------------------------------------------------------------*/
/* B3A6 CXGBR  - CONVERT FROM FIXED (64 to extended BFP)        [RRF-e] */
/* B3A6 CXGBRA - CONVERT FROM FIXED (64 to extended BFP)        [RRF-e] */
/*                                                                      */
/* Fixed 64-bit always fits in extended BFP; no exceptions possible     */
/*----------------------------------------------------------------------*/
DEF_INST( convert_fix64_to_bfp_ext_reg )
{
    int         r1, r2;
    BYTE        m3, m4;
    S64         op2;
    float128_t  op1;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        BFPRM_CHECK( m3, regs );
    else
#endif
    {
        if (m3 | m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_G( r2 );
    softfloat_exceptionFlags = 0;
    op1 = i64_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*--------------------------------------------------------------------------*/
/* B3A5 CDGBR  - CONVERT FROM FIXED (64 to long BFP)                [RRF-e] */
/* B3A5 CDGBRA - CONVERT FROM FIXED (64 to long BFP)                [RRF-e] */
/*                                                                          */
/* Fixed 64-bit may not fit in the 52+1 bits available in a long BFP, IEEE  */
/* Inexact exceptions are possible.  If m4 Inexact suppression control      */
/* (XxC) is on, then no Inexact exceptions recognized (no trap nor flag     */
/* set).                                                                    */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_fix64_to_bfp_long_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    S64        op2;
    float64_t  op1;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        BFPRM_CHECK( m3, regs );
    else
#endif
    {
        if (m3 | m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_G( r2 );
    softfloat_exceptionFlags = 0;
    op1 = i64_to_f64( op2 );

    PUT_FLOAT64_NOCC( op1, r1, regs );

    /* Inexact occurred and not masked by m4? */
    if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
    {
        /* Yes, set FPC flags and test for a trap */
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*--------------------------------------------------------------------------*/
/* B3A4 CEGBR  - CONVERT FROM FIXED (64 to short BFP)               [RRF-e] */
/* B3A4 CEGBRA - CONVERT FROM FIXED (64 to short BFP)               [RRF-e] */
/*                                                                          */
/* Fixed 64-bit may need to be rounded to fit in the 23+1 bits available    */
/* in a short BFP, IEEE Inexact may be raised.  If m4 Inexact suppression   */
/* (XxC) is on, then no inexact exception is recognized (no trap nor flag   */
/* set).                                                                    */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_fix64_to_bfp_short_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    S64        op2;
    float32_t  op1;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        BFPRM_CHECK( m3, regs );
    else
#endif
    {
        if (m3 | m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_G( r2 );
    softfloat_exceptionFlags = 0;
    op1 = i64_to_f32( op2 );

    PUT_FLOAT32_NOCC( op1, r1, regs );

    /* Inexact occurred and not masked by m4? */
    if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
    {
        /* Yes, set FPC flags and test for a trap */
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }

}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

/*--------------------------------------------------------------------------*/
/* CONVERT TO FIXED                                                         */
/*                                                                          */
/* Input is a floating point value; Xi and Xx are only exceptions possible  */
/* M3 field controls rounding, 0=Use FPC BRM                                */
/*                                                                          */
/* If the input value magnitude is too large to be represented in the       */
/* target format, an IEEE Invalid exception is raised.  If Invalid is not   */
/* trappable, the result is a maximum-magnitude integer of matching sign    */
/* and the IEEE Inexact exception is raised.                                */
/*                                                                          */
/* If FEATURE_FLOATING_POINT_EXTENSION FACILITY installed (enabled)         */
/*   M4 field bit 0x40 XxC (inexact) suppresses inexact exception: no       */
/*   IEEE Inexact trap or FPC Inexact status flag set.                      */
/*                                                                          */
/* If Floating Point Extension Facility not installed                       */
/*   M4 must be zero else program check specification exception             */
/*                                                                          */
/* Softfloat does not do two things required by SA-22-7832-10 table 19-18   */
/* on page 19.23:                                                           */
/* ** If the input is a NaN, return the largest negative number (Softfloat  */
/*    returns the largest positive number).  We code around this issue.     */
/* ** If Invalid is returned by Softfloat or due to a NaN and is not        */
/*    trappable, Inexact must be returned if not masked by M4               */
/*                                                                          */
/* The condition code is set based on the input, not the result.            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* B39A CFXBR  - CONVERT TO FIXED (extended BFP to 32)           [RRF-e] */
/* B39A CFXBRA - CONVERT TO FIXED (extended BFP to 32)           [RRF-e] */
/*-----------------------------------------------------------------------*/
DEF_INST( convert_bfp_ext_to_fix32_reg )
{
    int         r1, r2;
    BYTE        m3, m4, newcc;
    S32         op1;
    float128_t  op2;
    U32         ieee_trap_conds = 0;
    U32         op2_dataclass;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r2, regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT128_OP( op2, r2, regs );
    op2_dataclass = float128_class( op2 );
    softfloat_exceptionFlags = 0;

    if (op2_dataclass & (float_class_neg_signaling_nan |
                         float_class_pos_signaling_nan |
                         float_class_neg_quiet_nan     |
                         float_class_pos_quiet_nan ))
    {
        /* NaN input always returns maximum negative integer,
           cc3, and IEEE invalid exception */
        op1 = -0x7FFFFFFF - 1;
        newcc = 3;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
    {
        newcc = 0;
        op1 = 0;
    }
    else
    {
        newcc = (op2.v[FLOAT128_HI] & 0x8000000000000000ULL) ? 1 : 2;

        if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
            op1 = 0;
        else
        {
            SET_SF_RM_FROM_MASK( m3 );
            op1 = f128_to_i32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
        }
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    /* Non-trappable Invalid exception? */
    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        /* Inexact not suppressed? */
        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }

    regs->GR_L( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B399 CFDBR - CONVERT TO FIXED (long BFP to 32)            [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST( convert_bfp_long_to_fix32_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    S32        op1;
    float64_t  op2;
    U32        ieee_trap_conds = 0;
    U32        op2_dataclass;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT64_OP( op2, r2, regs );
    op2_dataclass = float64_class( op2 );
    softfloat_exceptionFlags = 0;

    if (op2_dataclass & (float_class_neg_signaling_nan |
                         float_class_pos_signaling_nan |
                         float_class_neg_quiet_nan     |
                         float_class_pos_quiet_nan ))
    {
        /* NaN input always returns maximum negative integer,
           cc3, and IEEE invalid exception */
        op1 = -0x7FFFFFFF - 1;
        newcc = 3;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
    {
        newcc = 0;
        op1 = 0;
    }
    else
    {
        newcc = (op2.v & 0x8000000000000000ULL) ? 1 : 2;

        if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
            op1 = 0;
        else
        {
            SET_SF_RM_FROM_MASK( m3 );
            op1 = f64_to_i32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
        }
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;
        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }

    regs->GR_L( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B398 CFEBR - CONVERT TO FIXED (short BFP to 32)           [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST( convert_bfp_short_to_fix32_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    S32        op1;
    float32_t  op2;
    U32        ieee_trap_conds = 0;
    U32        op2_dataclass;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT32_OP( op2, r2, regs );
    op2_dataclass = float32_class( op2 );
    softfloat_exceptionFlags = 0;

    if (op2_dataclass & (float_class_neg_signaling_nan |
                         float_class_pos_signaling_nan |
                         float_class_neg_quiet_nan     |
                         float_class_pos_quiet_nan ))
    {
        /* NaN input always returns maximum negative integer,
           cc3, and IEEE invalid exception */
        op1 = -0x7FFFFFFF - 1;
        newcc = 3;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
    {
        newcc = 0;
        op1 = 0;
    }
    else
    {
        newcc = (op2.v & 0x80000000) ? 1 : 2;
        if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
            op1 = 0;
        else
        {
            SET_SF_RM_FROM_MASK( m3 );
            op1 = f32_to_i32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
        }
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }

    regs->GR_L( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*-------------------------------------------------------------------*/
/* B3AA CGXBR - CONVERT TO FIXED (extended BFP to 64)        [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST( convert_bfp_ext_to_fix64_reg )
{
    int         r1, r2;
    BYTE        m3, m4, newcc;
    S64         op1;
    float128_t  op2;
    U32         ieee_trap_conds = 0;
    U32         op2_dataclass;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r2, regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT128_OP( op2, r2, regs );
    op2_dataclass = float128_class( op2 );
    softfloat_exceptionFlags = 0;

    if (op2_dataclass & (float_class_neg_signaling_nan |
                         float_class_pos_signaling_nan |
                         float_class_neg_quiet_nan     |
                         float_class_pos_quiet_nan ))
    {
        /* NaN input always returns maximum negative integer,
           cc3, and IEEE invalid exception */
        op1 = -(0x7FFFFFFFFFFFFFFFULL) - 1;
        newcc = 3;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
    {
        newcc = 0;
        op1 = 0;
    }
    else
    {
        newcc = (op2.v[FLOAT128_HI] & 0x8000000000000000ULL) ? 1 : 2;

        if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
            op1 = 0;
        else
        {
            SET_SF_RM_FROM_MASK( m3 );
            op1 = f128_to_i64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
        }
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }

    regs->GR_G( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*-------------------------------------------------------------------*/
/* B3A9 CGDBR - CONVERT TO FIXED (long BFP to 64)            [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST( convert_bfp_long_to_fix64_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    S64        op1;
    float64_t  op2;
    U32        ieee_trap_conds = 0;
    U32        op2_dataclass;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT64_OP( op2, r2, regs );
    op2_dataclass = float64_class( op2 );
    softfloat_exceptionFlags = 0;

    if (op2_dataclass & (float_class_neg_signaling_nan |
                         float_class_pos_signaling_nan |
                         float_class_neg_quiet_nan     |
                         float_class_pos_quiet_nan ))
    {
        /* NaN input always returns maximum negative integer,
           cc3, and IEEE invalid exception */
        op1 = -(0x7FFFFFFFFFFFFFFFULL) - 1;
        newcc = 3;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
    {
        newcc = 0;
        op1 = 0;
    }
    else
    {
        newcc = (op2.v & 0x8000000000000000ULL) ? 1 : 2;

        if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
            op1 = 0;
        else
        {
            SET_SF_RM_FROM_MASK( m3 );
            op1 = f64_to_i64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
        }
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }

    regs->GR_G( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*-------------------------------------------------------------------*/
/* B3A8 CGEBR - CONVERT TO FIXED (short BFP to 64)           [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST( convert_bfp_short_to_fix64_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    S64        op1;
    float32_t  op2;
    U32        ieee_trap_conds = 0;
    U32        op2_dataclass;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT32_OP( op2, r2, regs );
    op2_dataclass = float32_class( op2 );
    softfloat_exceptionFlags = 0;

    if (op2_dataclass & (float_class_neg_signaling_nan |
                         float_class_pos_signaling_nan |
                         float_class_neg_quiet_nan     |
                         float_class_pos_quiet_nan ))
    {
        /* NaN input always returns maximum negative integer,
           cc3, and IEEE invalid exception */
        op1 = -(0x7FFFFFFFFFFFFFFFULL) - 1;
        newcc = 3;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
    {
        newcc = 0;
        op1 = 0;
    }
    else
    {
        newcc = (op2.v & 0x80000000) ? 1 : 2;

        if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
            op1 = 0;
        else
        {
            SET_SF_RM_FROM_MASK( m3 );
            op1 = f32_to_i64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
        }
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }

    regs->GR_G( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined( FEATURE_037_FP_EXTENSION_FACILITY )

/*--------------------------------------------------------------------------*/
/* CONVERT FROM LOGICAL                                                     */
/*                                                                          */
/* Input is a signed integer; Xo, Xu, and Xx are only exceptions possible   */
/*                                                                          */
/* If FEATURE_FLOATING_POINT_EXTENSION FACILITY installed (enabled)         */
/*   M3 field controls rounding, 0=Use FPC BRM                              */
/*   M4 field bit 0x04 XxC (inexact) suppresses inexact exception: no       */
/*   inexact trap or FPC status flag.                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* B392 CXLFBR - CONVERT FROM LOGICAL (32 to extended BFP)          [RRF-e] */
/*                                                                          */
/* Fixed 32-bit always fits in Extended BFP; no exceptions possible         */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_u32_to_bfp_ext_reg )
{
    int         r1, r2;
    BYTE        m3;
    U32         op2;
    float128_t  op1;

    RRF_M( inst, regs, r1, r2, m3 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );
    BFPRM_CHECK( m3, regs );

    op2 = regs->GR_L( r2 );
    SET_SF_RM_FROM_MASK( m3 );
    softfloat_exceptionFlags = 0;

    op1 = ui32_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}

/*--------------------------------------------------------------------------*/
/* B391 CDLFBR - CONVERT FROM LOGICAL (32 to long BFP)              [RRF-e] */
/*                                                                          */
/* Fixed 32-bit always fits in long BFP; no exceptions possible             */
/*--------------------------------------------------------------------------*/

DEF_INST( convert_u32_to_bfp_long_reg )
{
    int        r1, r2;
    BYTE       m3;
    U32        op2;
    float64_t  op1;

    RRF_M( inst, regs, r1, r2, m3 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    op2 = regs->GR_L( r2 );
    SET_SF_RM_FROM_MASK( m3 );
    softfloat_exceptionFlags = 0;

    op1 = ui32_to_f64( op2 );

    PUT_FLOAT64_NOCC( op1, r1, regs );
}

/*--------------------------------------------------------------------------*/
/* B390 CELFBR - CONVERT FROM LOGICAL (32 to short BFP)             [RRF-e] */
/*                                                                          */
/* Fixed 32-bit may need to be rounded to fit in the 23+1 bits available    */
/* in a short BFP, IEEE Inexact may be raised.  If m4 Inexact suppression   */
/* (XxC) is on, then no inexact exception is recognized (no trap nor flag   */
/* set).                                                                    */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_u32_to_bfp_short_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    U32        op2;
    float32_t  op1;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_L( r2 );
    softfloat_exceptionFlags = 0;

    op1 = ui32_to_f32( op2 );

    PUT_FLOAT32_NOCC( op1, r1, regs );

    if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}

/*--------------------------------------------------------------------------*/
/* B3A2 CXLGBR - CONVERT FROM LOGICAL (64 to extended BFP)          [RRF-e] */
/*                                                                          */
/* Fixed 64-bit always fits in extended BFP; no exceptions possible         */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_u64_to_bfp_ext_reg )
{
    int         r1, r2;
    BYTE        m3;
    U64         op2;
    float128_t  op1;

    RRF_M( inst, regs, r1, r2, m3 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );
    BFPRM_CHECK( m3, regs );

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_G( r2 );
    softfloat_exceptionFlags = 0;

    op1 = ui64_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}


/*--------------------------------------------------------------------------*/
/* B3A1 CDLGBR - CONVERT FROM LOGICAL (64 to long BFP)              [RRF-e] */
/*                                                                          */
/* Fixed 64-bit may not fit in the 52+1 bits available in a long BFP, IEEE  */
/* Inexact exceptions are possible.  If m4 Inexact suppression control      */
/* (XxC) is on, then no Inexact exceptions recognized (no trap nor flag     */
/* set).                                                                    */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_u64_to_bfp_long_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    U64        op2;
    float64_t  op1;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_G( r2 );
    softfloat_exceptionFlags = 0;

    op1 = ui64_to_f64( op2 );

    PUT_FLOAT64_NOCC( op1, r1, regs );

    if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}


/*--------------------------------------------------------------------------*/
/* B3A0 CELGBR - CONVERT FROM LOGICAL (64 to short BFP)             [RRF-e] */
/*                                                                          */
/* Fixed 64-bit may need to be rounded to fit in the 23+1 bits available    */
/* in a short BFP, IEEE Inexact may be raised.  If m4 Inexact suppression   */
/* (XxC) is on, then no inexact exception is recognized (no trap nor flag   */
/* set).                                                                    */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_u64_to_bfp_short_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    U64        op2;
    float32_t  op1;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    SET_SF_RM_FROM_MASK( m3 );
    op2 = regs->GR_G( r2 );
    softfloat_exceptionFlags = 0;

    op1 = ui64_to_f32( op2 );

    PUT_FLOAT32_NOCC( op1, r1, regs );

    if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}

/*--------------------------------------------------------------------------*/
/* CONVERT TO LOGICAL                                                       */
/*                                                                          */
/* Input is a floating point value; Xi and Xx are only exceptions possible  */
/* M3 field controls rounding, 0=Use FPC BRM                                */
/*                                                                          */
/* If the input value magnitude is too large to be represented in the       */
/* target format, an IEEE Invalid exception is raised.  If Invalid is not   */
/* trappable, the result is a maximum-magnitude integer of matching sign    */
/* and the IEEE Inexact exception is raised.                                */
/*                                                                          */
/* The M4 field bit 0x40 XxC (inexact) suppresses inexact exception: no     */
/* IEEE Inexact trap or FPC Inexact status flag set.                        */
/*                                                                          */
/* Softfloat does not do two things required by SA-22-7832-10 table 19-18   */
/* on page 19.23:                                                           */
/* ** If the input is a NaN, return the largest negative number (Softfloat  */
/*    returns the largest positive number).  We code around this issue.     */
/* ** If Invalid is returned by Softfloat or due to a NaN and is not        */
/*    trappable, Inexact must be returned if not masked by M4               */
/*                                                                          */
/* We also need some test cases to probe Softfloat behavior when the        */
/* rounded result fits in an integer but the input is larger than that.     */
/* PoP requires inexact and maximum magnitude integer result.               */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* B39E CLFXBR - CONVERT TO LOGICAL (extended BFP to 32)            [RRF-e] */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_bfp_ext_to_u32_reg )
{
    int         r1, r2;
    BYTE        m3, m4, newcc;
    U32         op1;
    float128_t  op2;
    U32         ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r2, regs );
    BFPRM_CHECK( m3, regs );

    GET_FLOAT128_OP( op2, r2, regs );
    softfloat_exceptionFlags = 0;

    if (FLOAT128_ISNAN( op2 ))
    {
        op1 = 0;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else
    {
        SET_SF_RM_FROM_MASK( m3 );
        op1 = f128_to_ui32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }
    else
        newcc = FLOAT128_CC( op2 );

    regs->GR_L( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}

/*--------------------------------------------------------------------------*/
/* B39D CLFDBR - CONVERT TO LOGICAL (long BFP to 32)                [RRF-e] */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_bfp_long_to_u32_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    U32        op1;
    float64_t  op2;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    GET_FLOAT64_OP( op2, r2, regs );
    softfloat_exceptionFlags = 0;

    if (FLOAT64_ISNAN( op2 ))
    {
        op1 = 0x00000000;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else
    {
        SET_SF_RM_FROM_MASK( m3 );
        op1 = f64_to_ui32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }
    else
        newcc = FLOAT64_CC( op2 );

    regs->GR_L( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}

/*--------------------------------------------------------------------------*/
/* B39C CLFEBR - CONVERT TO LOGICAL (short BFP to 32)               [RRF-e] */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_bfp_short_to_u32_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    U32        op1;
    float32_t  op2;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    GET_FLOAT32_OP( op2, r2, regs );
    softfloat_exceptionFlags = 0;

    if (FLOAT32_ISNAN( op2 ))
    {
        op1 = 0;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else
    {
        SET_SF_RM_FROM_MASK( m3 );
        op1 = f32_to_ui32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }
    else
        newcc = FLOAT32_CC( op2 );

    regs->GR_L( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}

/*--------------------------------------------------------------------------*/
/* B3AE CLGXBR - CONVERT TO LOGICAL (extended BFP to 64)            [RRF-e] */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_bfp_ext_to_u64_reg )
{
    int         r1, r2;
    BYTE        m3, m4, newcc;
    U64         op1;
    float128_t  op2;
    U32         ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r2, regs );
    BFPRM_CHECK( m3, regs );

    GET_FLOAT128_OP( op2, r2, regs );
    softfloat_exceptionFlags = 0;

    if (FLOAT128_ISNAN( op2 ))
    {
        op1 = 0;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else
    {
        SET_SF_RM_FROM_MASK( m3 );
        op1 = f128_to_ui64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }
    else
        newcc = FLOAT128_CC( op2 );

    regs->GR_G( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}


/*--------------------------------------------------------------------------*/
/* B3AD CLGDBR - CONVERT TO LOGICAL (long BFP to 64)                [RRF-e] */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_bfp_long_to_u64_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    U64        op1;
    float64_t  op2;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    GET_FLOAT64_OP( op2, r2, regs );
    softfloat_exceptionFlags = 0;

    if (FLOAT64_ISNAN( op2 ))
    {
        op1 = 0;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else
    {
        SET_SF_RM_FROM_MASK( m3 );
        op1 = f64_to_ui64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }
    else
        newcc = FLOAT64_CC( op2 );

    regs->GR_G( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}


/*--------------------------------------------------------------------------*/
/* B3AC CLGEBR - CONVERT TO LOGICAL (short BFP to 64)               [RRF-e] */
/*--------------------------------------------------------------------------*/
DEF_INST( convert_bfp_short_to_u64_reg )
{
    int        r1, r2;
    BYTE       m3, m4, newcc;
    U64        op1;
    float32_t  op2;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

    GET_FLOAT32_OP( op2, r2, regs );
    softfloat_exceptionFlags = 0;

    if (FLOAT32_ISNAN( op2 ))
    {
        op1 = 0;
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    else
    {
        SET_SF_RM_FROM_MASK( m3 );
        op1 = f32_to_ui64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
    }

    IEEE_EXCEPTION_TRAP_XI( regs );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        newcc = 3;

        if (!SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags |= softfloat_flag_inexact;
    }
    else
        newcc = FLOAT32_CC( op2 );

    regs->GR_G( r1 ) = op1;
    regs->psw.cc = newcc;

    ieee_trap_conds = ieee_exception_test_oux( regs );
    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
}
#endif /* defined FEATURE_037_FP_EXTENSION_FACILITY */

/*-------------------------------------------------------------------*/
/* B34D DXBR  - DIVIDE (extended BFP)                          [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( divide_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op1, op2, ans;
    U32         ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f128_div( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TRAP_XZ( regs );

        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_EXTD :
                SCALE_FACTOR_ARITH_UFLOW_EXTD );
    }

    PUT_FLOAT128_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B31D DDBR  - DIVIDE (long BFP)                              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( divide_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_div( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TRAP_XZ( regs );

        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED1D DDB   - DIVIDE (long BFP)                              [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( divide_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op1, r1, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_div( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TRAP_XZ( regs );

        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}


/*-------------------------------------------------------------------*/
/* B30D DEBR  - DIVIDE (short BFP)                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( divide_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_div( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TRAP_XZ( regs );

        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED0D DEB   - DIVIDE (short BFP)                             [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( divide_bfp_short )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_div( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TRAP_XZ( regs );

        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}


/*-------------------------------------------------------------------*/
/* B342 LTXBR - LOAD AND TEST (extended BFP)                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_and_test_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OP( op, r2, regs );

    if (FLOAT128_ISNAN( op ))
    {
        if (f128_isSignalingNaN( op ))
        {
            if (regs->fpc & FPC_MASK_IMI)
                ieee_trap( regs, DXC_IEEE_INVALID_OP );
            else
            {
                regs->fpc |= FPC_FLAG_SFI;
                FLOAT128_MAKE_QNAN( op );
            }
        }
    }

    PUT_FLOAT128_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B312 LTDBR - LOAD AND TEST (long BFP)                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_and_test_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op, r2, regs );

    if (FLOAT64_ISNAN( op ))
    {
        if (f64_isSignalingNaN( op ))
        {
            if (regs->fpc & FPC_MASK_IMI)
                ieee_trap( regs, DXC_IEEE_INVALID_OP );
            else
            {
                regs->fpc |= FPC_FLAG_SFI;
                FLOAT64_MAKE_QNAN( op );
            }
        }
    }

    PUT_FLOAT64_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B302 LTEBR - LOAD AND TEST (short BFP)                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_and_test_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op;

    RRE( inst, regs, r1, r2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op, r2, regs );

    if (FLOAT32_ISNAN( op ))
    {
        if (f32_isSignalingNaN( op ))
        {
            if (regs->fpc & FPC_MASK_IMI)
                ieee_trap( regs, DXC_IEEE_INVALID_OP );
            else
            {
                regs->fpc |= FPC_FLAG_SFI;
                FLOAT32_MAKE_QNAN( op );
            }
        }
    }

    PUT_FLOAT32_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B357 FIEBR - LOAD FP INTEGER (short BFP)                  [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fp_int_bfp_short_reg)
{
    int        r1, r2;
    BYTE       m3, m4;
    float32_t  op;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT32_OP( op, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_MASK( m3 );

    op = f32_roundToInt( op, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

    IEEE_EXCEPTION_TRAP_XI( regs );

    PUT_FLOAT32_NOCC( op, r1, regs );

    if (softfloat_exceptionFlags)
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}

/*-------------------------------------------------------------------*/
/* B35F FIDBR - LOAD FP INTEGER (long BFP)                   [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST( load_fp_int_bfp_long_reg )
{
    int        r1, r2;
    BYTE m3,   m4;
    float64_t  op;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT64_OP( op, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_MASK( m3 );

    op = f64_roundToInt( op, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

    IEEE_EXCEPTION_TRAP_XI( regs );

    PUT_FLOAT64_NOCC( op, r1, regs );

    if (softfloat_exceptionFlags)
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}

/*-------------------------------------------------------------------*/
/* B347 FIXBR - LOAD FP INTEGER (extended BFP)               [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fp_int_bfp_ext_reg)
{
    int         r1, r2;
    BYTE        m3, m4;
    float128_t  op;
    U32         ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );
    BFPRM_CHECK( m3, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (!FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
#endif
        m4 = 0;

    GET_FLOAT128_OP( op, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_MASK( m3 );

    op = f128_roundToInt( op, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

    IEEE_EXCEPTION_TRAP_XI( regs );

    PUT_FLOAT128_NOCC( op, r1, regs );

    if (softfloat_exceptionFlags)
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );
        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    }
}


/*-------------------------------------------------------------------*/
/* Load Lengthened                                                   */
/*                                                                   */
/* IBM expects SNaNs to raise the IEEE Invalid exception, to         */
/* suppress the result if the exception is trapped, and to make the  */
/* SNaN a QNaN if the exception is not trapped.  (Table 19-17 on     */
/* page 19-21 of SA22-7832-10.)                                      */
/*                                                                   */
/* Softfloat 3a never raises invalid in the routines that increase   */
/* the width of floating point values, nor does it make QNaNs of     */
/* SNaNs.                                                            */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* B304 LDEBR - LOAD LENGTHENED (short to long BFP)            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_lengthened_bfp_short_to_long_reg )
{
    int        r1, r2;
    float32_t  op2;
    float64_t  op1;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op2, r2, regs );

    if (f32_isSignalingNaN( op2 ))
    {
        softfloat_exceptionFlags = softfloat_flag_invalid;
        IEEE_EXCEPTION_TRAP_XI( regs );

        FLOAT32_MAKE_QNAN( op2 );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    op1 = f32_to_f64( op2 );

    PUT_FLOAT64_NOCC( op1, r1, regs );
}

/*-------------------------------------------------------------------*/
/* ED04 LDEB  - LOAD LENGTHENED (short to long BFP)            [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_lengthened_bfp_short_to_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op2;
    float64_t  op1;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    if (f32_isSignalingNaN( op2 ))
    {
        softfloat_exceptionFlags = softfloat_flag_invalid;
        IEEE_EXCEPTION_TRAP_XI( regs );

        FLOAT32_MAKE_QNAN( op2 );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    op1 = f32_to_f64( op2 );

    PUT_FLOAT64_NOCC( op1, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B305 LXDBR - LOAD LENGTHENED (long to extended BFP)         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_lengthened_bfp_long_to_ext_reg )
{
    int         r1, r2;
    float64_t   op2;
    float128_t  op1;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

    GET_FLOAT64_OP( op2, r2, regs );

    if (f64_isSignalingNaN( op2 ))
    {
        softfloat_exceptionFlags = softfloat_flag_invalid;
        IEEE_EXCEPTION_TRAP_XI( regs );

        FLOAT64_MAKE_QNAN( op2 );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    op1 = f64_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}

/*-------------------------------------------------------------------*/
/* ED05 LXDB  - LOAD LENGTHENED (long to extended BFP)         [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_lengthened_bfp_long_to_ext )
{
    int         r1, x2, b2;
    VADR        effective_addr2;
    float64_t   op2;
    float128_t  op1;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    if (f64_isSignalingNaN( op2 ))
    {
        softfloat_exceptionFlags = softfloat_flag_invalid;
        IEEE_EXCEPTION_TRAP_XI( regs );

        FLOAT64_MAKE_QNAN( op2 );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    op1 = f64_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B306 LXEBR - LOAD LENGTHENED (short to extended BFP)        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_lengthened_bfp_short_to_ext_reg )
{
    int         r1, r2;
    float32_t   op2;
    float128_t  op1;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

    GET_FLOAT32_OP( op2, r2, regs );

    if (f32_isSignalingNaN( op2 ))
    {
        softfloat_exceptionFlags = softfloat_flag_invalid;
        IEEE_EXCEPTION_TRAP_XI( regs );

        FLOAT32_MAKE_QNAN( op2 );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    op1 = f32_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}

/*-------------------------------------------------------------------*/
/* ED06 LXEB  - LOAD LENGTHENED (short to extended BFP)        [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_lengthened_bfp_short_to_ext )
{
    int         r1, x2, b2;
    VADR        effective_addr2;
    float32_t   op2;
    float128_t  op1;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    if (f32_isSignalingNaN( op2 ))
    {
        softfloat_exceptionFlags = softfloat_flag_invalid;
        IEEE_EXCEPTION_TRAP_XI( regs );

        FLOAT32_MAKE_QNAN( op2 );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    op1 = f32_to_f128( op2 );

    PUT_FLOAT128_NOCC( op1, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B341 LNXBR - LOAD NEGATIVE (extended BFP)                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_negative_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OP( op, r2, regs );
    op.v[FLOAT128_HI] |= 0x8000000000000000ULL;

    PUT_FLOAT128_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B311 LNDBR - LOAD NEGATIVE (long BFP)                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_negative_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op, r2, regs );
    op.v |= 0x8000000000000000ULL;

    PUT_FLOAT64_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B301 LNEBR - LOAD NEGATIVE (short BFP)                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_negative_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op, r2, regs );
    op.v |= 0x80000000;

    PUT_FLOAT32_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B343 LCXBR - LOAD COMPLEMENT (extended BFP)                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_complement_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OP( op, r2, regs );
    op.v[FLOAT128_HI] ^= 0x8000000000000000ULL;

    PUT_FLOAT128_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B313 LCDBR - LOAD COMPLEMENT (long BFP)                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_complement_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op, r2, regs );
    op.v ^= 0x8000000000000000ULL;

    PUT_FLOAT64_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B303 LCEBR - LOAD COMPLEMENT (short BFP)                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_complement_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op, r2, regs );
    op.v ^= 0x80000000;

    PUT_FLOAT32_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B340 LPXBR - LOAD POSITIVE (extended BFP)                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_positive_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op;

    RRE( inst, regs, r1, r2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OP( op, r2, regs );
    op.v[FLOAT128_HI] &= ~0x8000000000000000ULL;

    PUT_FLOAT128_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B310 LPDBR - LOAD POSITIVE (long BFP)                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_positive_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op;

    RRE( inst, regs, r1, r2 );

    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op, r2, regs );
    op.v  &= ~0x8000000000000000ULL;

    PUT_FLOAT64_CC( op, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B300 LPEBR - LOAD POSITIVE (short BFP)                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_positive_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op, r2, regs );
    op.v &= ~0x80000000;

    PUT_FLOAT32_CC( op, r1, regs );
}

/*----------------------------------------------------------------------*/
/* Load Rounded                                                         */
/*                                                                      */
/* IBM expects SNaNs to raise the IEEE Invalid exception, to            */
/* suppress the result if the exception is trapped, and to make the     */
/* SNaN a QNaN if the exception is not trapped.  (Table 19-17 on        */
/* page 19-21 of SA22-7832-10.)                                         */
/*                                                                      */
/* Softfloat 3a only raises invalid in the routines that decrease       */
/* the width of floating point values when the input is a signalling    */
/* NaN.  The NaN is also converted to a quiet NaN.  This is consistent  */
/* with IBM's NaN processing during Load Rounded.                       */
/*                                                                      */
/* A bigger "gotcha" is the behavior IBM defines when overflow or       */
/* underflow exceptions occur and are trappable.  IBM expects the       */
/* input value, rounded to the target precision but maintained in       */
/* the input precision, to be placed in the result before taking        */
/* trap.  Softfloat does not support this; we must do it ourselves      */
/*                                                                      */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* B344 LEDBR  - LOAD ROUNDED (long to short BFP)               [RRF-e] */
/* B344 LEDBRA - LOAD ROUNDED (long to short BFP)               [RRF-e] */
/*----------------------------------------------------------------------*/
DEF_INST( load_rounded_bfp_long_to_short_reg )
{
    int        r1, r2;
    BYTE       m3, m4;
    float64_t  op2;
    float32_t  op1;
    U32        ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op2, r2, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        SET_SF_RM_FROM_MASK( m3 );
    else
#endif
    {
        if (m3 || m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
        SET_SF_RM_FROM_FPC;
    }

    softfloat_exceptionFlags = 0;

    op1 = f64_to_f32( op2 );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
    {
        if (SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags &= ~softfloat_flag_inexact;
    }
#endif

    IEEE_EXCEPTION_TRAP_XI( regs );

    PUT_FLOAT32_NOCC( op1, r1, regs );

    if (softfloat_exceptionFlags)
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
        {
            op2 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_LOADR_OFLOW_LONG :
                SCALE_FACTOR_LOADR_UFLOW_LONG );

            PUT_FLOAT64_NOCC( op2, r1, regs );
        }

        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
    }
}

/*----------------------------------------------------------------------*/
/* B345 LDXBR  - LOAD ROUNDED (extended to long BFP)            [RRF-e] */
/* B345 LDXBRA - LOAD ROUNDED (extended to long BFP)            [RRF-e] */
/*----------------------------------------------------------------------*/
DEF_INST( load_rounded_bfp_ext_to_long_reg )
{
    int         r1, r2;
    BYTE        m3, m4;
    float128_t  op2;
    float64_t   op1;
    U32         ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OP( op2, r2, regs );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        SET_SF_RM_FROM_MASK( m3 );
    else
#endif
    {
        if (m3 || m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
        SET_SF_RM_FROM_FPC;
    }

    softfloat_exceptionFlags = 0;

    op1 = f128_to_f64( op2 );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
    {
        if (SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags &= ~softfloat_flag_inexact;
    }
#endif

    IEEE_EXCEPTION_TRAP_XI( regs );

    PUT_FLOAT64_NOCC( op1, r1, regs );

    if (softfloat_exceptionFlags)
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
        {
            op2 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_LOADR_OFLOW_EXTD :
                SCALE_FACTOR_LOADR_UFLOW_EXTD );

            PUT_FLOAT128_NOCC( op2, r1, regs );
        }

        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
    }
}

/*-------------------------------------------------------------------*/
/* B346 LEXBR  - LOAD ROUNDED (extended to short BFP)        [RRF-e] */
/* B346 LEXBRA - LOAD ROUNDED (extended to short BFP)        [RRF-e] */
/*-------------------------------------------------------------------*/
DEF_INST( load_rounded_bfp_ext_to_short_reg )
{
    int         r1, r2;
    BYTE        m3, m4;
    float128_t  op2;
    float32_t   op1;
    U32         ieee_trap_conds = 0;

    RRF_MM( inst, regs, r1, r2, m3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OP( op2, r2, regs );
//  {
//  char xx[128];
//  snprintf(xx, sizeof(xx), "%16.16llX  %16.16llX  %16.16llX_%16.16llX  %d %d  %d %d",
//      regs->FPR_L(r2), regs->FPR_L(r2+2), op2.v[FLOAT128_HI], op2.v[FLOAT128_LO], r2, r2+2, r1, r2);
//  // HHC01390 "%s" // DUMP               (debugging)
//  WRMSG(HHC01390, "D", xx );
//  }

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
        SET_SF_RM_FROM_MASK( m3 );
    else
#endif
    {
        if (m3 || m4)
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
        SET_SF_RM_FROM_FPC;
    }

    softfloat_exceptionFlags = 0;

    op1 = f128_to_f32( op2 );

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
    if (FACILITY_ENABLED( 037_FP_EXTENSION, regs ))
    {
        if (SUPPRESS_INEXACT( m4 ))
            softfloat_exceptionFlags &= ~softfloat_flag_inexact;
    }
#endif

    IEEE_EXCEPTION_TRAP_XI( regs );

    PUT_FLOAT32_NOCC( op1, r1, regs );
//  {
//  char xx[128];
//  snprintf(xx, sizeof(xx), "LEXBR > %d  %8.8X  %16.16llX  %d", r1, regs->FPR_S(r1), regs->FPR_L(r1), r1);
//  // HHC01390 "%s" // DUMP               (debugging)
//  WRMSG(HHC01390, "D", xx );
//  }

    if (softfloat_exceptionFlags)
    {
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
        {
            op2 = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_LOADR_OFLOW_EXTD :
                SCALE_FACTOR_LOADR_UFLOW_EXTD );

            PUT_FLOAT128_NOCC( op2, r1, regs );
//          {
//          char xx[128];
//          snprintf(xx, sizeof(xx), "LEXBR > %d/%d  Scaled  %16.16llX %16.16llX",
//              r1, r1+2, regs->FPR_L(r1), regs->FPR_L(r1+2));
//          // HHC01390 "%s" // DUMP               (debugging)
//          WRMSG(HHC01390, "D", xx );
//          }
        }

        IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
    }
}

/*-------------------------------------------------------------------*/
/* B34C MXBR  - MULTIPLY (extended BFP)                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op1, op2, ans;
    U32         ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OPS(op1, r1, op2, r2, regs);

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f128_mul( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_EXTD :
                SCALE_FACTOR_ARITH_UFLOW_EXTD );
    }

    PUT_FLOAT128_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B307 MXDBR - MULTIPLY (long to extended BFP)                [RRE] */
/*-------------------------------------------------------------------*/
/* Because the operation result is in a longer format than the       */
/* operands, IEEE exceptions Overflow, Underflow, and Inexact        */
/* cannot occur.  An SNaN will still generate Invalid.               */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_long_to_ext_reg )
{
    int         r1, r2;
    float64_t   op1, op2;
    float128_t  iop1, iop2, ans;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    /* f64_to_f128 will, if presented with a SNaN, convert it to quiet
       and raise softfloat_flags_invalid. Unfortunately, if one of the
       operands is an SNaN and the other a QNaN, f128_mul() will be
       unable to do NaN propagation correctly because it will see only
       two QNaNs.  So if we encounter an SNaN while upconverting the
       input operands, that becomes the answer.  If both operands are
       QNaNs, then f128_mul() will be able to do NaN propagation correctly.
    */
    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_even;

    iop1 = f64_to_f128( op1 );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
        ans = iop1;
    else
    {
        iop2 = f64_to_f128( op2 );

        if (softfloat_exceptionFlags & softfloat_flag_invalid)
            ans = iop2;
        else
            ans = f128_mul( iop1, iop2 );
    }

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    PUT_FLOAT128_NOCC( ans, r1, regs );
}

/*-------------------------------------------------------------------*/
/* ED07 MXDB  - MULTIPLY (long to extended BFP)                [RXE] */
/*-------------------------------------------------------------------*/
/* Because the operation result is in a longer format than the       */
/* operands, IEEE exceptions Overflow, Underflow, and Inexact        */
/* cannot occur.  An SNaN will still generate Invalid.               */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_long_to_ext )
{
    int         r1, x2, b2;
    VADR        effective_addr2;
    float64_t   op1, op2;
    float128_t  iop1, iop2, ans;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

    GET_FLOAT64_OP( op1, r1, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    /* f64_to_f128 will, if presented with a SNaN, convert it to quiet
       and raise softfloat_flags_invalid.  Unfortunately, if one of
       the operands is an SNaN and the other a QNaN, f128_mul() will
       be unable to do NaN propagation correctly because it will see
       only two QNaNs.  So if we encounter an SNaN while upconverting
       the input operands, that becomes the answer.  If both operands
       are QNaNs, then f128_mul() will be able to do NaN propagation correctly.
    */
    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_even;

    iop1 = f64_to_f128( op1 );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
        ans = iop1;
    else
    {
        iop2 = f64_to_f128( op2 );

        if (softfloat_exceptionFlags & softfloat_flag_invalid)
            ans = iop2;
        else
            ans = f128_mul( iop1, iop2 );
    }

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        SET_FPC_FLAGS_FROM_SF( regs );
    }
    PUT_FLOAT128_NOCC( ans, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B31C MDBR  - MULTIPLY (long BFP)                            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op1, op2, ans;
    U32        ieee_trap_conds =0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_mul( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED1C MDB   - MULTIPLY (long BFP)                            [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1, op2, ans;
    U32        ieee_trap_conds =0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op1, r1, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_mul( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B30C MDEBR - MULTIPLY (short to long BFP)                   [RRE] */
/*-------------------------------------------------------------------*/
/* Because the operation result is in a longer format than the       */
/* operands, IEEE exceptions Overflow, Underflow, and Inexact        */
/* cannot occur.  An SNaN will still generate Invalid.               */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_short_to_long_reg )
{
    int        r1, r2;
    float32_t  op1, op2;
    float64_t  iop1, iop2, ans;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    /* f32_to_f64 will, if presented with a SNaN, convert it to quiet
       and raise softfloat_flags_invalid.  Unfortunately, if one of
       the operands is an SNaN and the other a QNaN, f64_mul() will be
       unable to do NaN propagation correctly because it will see only
       two QNaNs.  So if we encounter an SNaN while upconverting the
       input operands, that becomes the answer.  If both operands are
       QNaNs, then f64_mul() will be able to do NaN propagation correctly.
    */
    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_even;

    iop1 = f32_to_f64( op1 );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
        ans = iop1;
    else
    {
        iop2 = f32_to_f64( op2 );

        if (softfloat_exceptionFlags & softfloat_flag_invalid)
            ans = iop2;
        else
            ans = f64_mul( iop1, iop2 );
    }

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    PUT_FLOAT64_NOCC(ans, r1, regs);
}

/*-------------------------------------------------------------------*/
/* ED0C MDEB  - MULTIPLY (short to long BFP)                   [RXE] */
/*-------------------------------------------------------------------*/
/* Because the operation result is in a longer format than the       */
/* operands, IEEE exceptions Overflow, Underflow, and Inexact        */
/* cannot occur.  An SNaN will still generate Invalid.               */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_short_to_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2;
    float64_t  iop1, iop2, ans;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    /* f32_to_f64 will, if presented with a SNaN, convert it to quiet
       and raise softfloat_flags_invalid.  Unfortunately, if one of the
       operands is an SNaN and the other a QNaN, f64_mul() will be
       unable to do NaN propagation correctly because it will see only
       two QNaNs.  So if we encounter an SNaN while upconverting the
       input operands, that becomes the answer.  If both operands are
       QNaNs, then f64_mul() will be able to do NaN propagation correctly.
    */
    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_even;

    iop1 = f32_to_f64( op1 );

    if (softfloat_exceptionFlags & softfloat_flag_invalid)
        ans = iop1;
    else
    {
        iop2 = f32_to_f64( op2 );

        if (softfloat_exceptionFlags & softfloat_flag_invalid)
            ans = iop2;
        else
            ans = f64_mul( iop1, iop2 );
    }

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        SET_FPC_FLAGS_FROM_SF( regs );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );
}

/*-------------------------------------------------------------------*/
/* B317 MEEBR - MULTIPLY (short BFP)                           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op1, op2, ans;
    U32        ieee_trap_conds =0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_mul( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED17 MEEB  - MULTIPLY (short BFP)                           [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_bfp_short )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2, ans;
    U32        ieee_trap_conds =0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_mul( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B31E MADBR - MULTIPLY AND ADD (long BFP)                    [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_add_bfp_long_reg )
{
    int        r1, r2, r3;
    float64_t  op1, op2, op3, ans;
    U32        ieee_trap_conds =0;

    RRD( inst, regs, r1, r2, r3 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op3, r3, regs );
    GET_FLOAT64_OP( op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED1E MADB  - MULTIPLY AND ADD (long BFP)                    [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_add_bfp_long )
{
    int        x2, r1, r3, b2;
    VADR       effective_addr2;
    float64_t  op1, op2, op3, ans;
    U32        ieee_trap_conds =0;

    RXF( inst, regs, r1, r3, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op3, r3, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B30E MAEBR - MULTIPLY AND ADD (short BFP)                   [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_add_bfp_short_reg )
{
    int        r1, r2, r3;
    float32_t  op1, op2, op3, ans;
    U32        ieee_trap_conds =0;

    RRD( inst, regs, r1, r2, r3 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op3, r3, regs );
    GET_FLOAT32_OP( op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED0E MAEB  - MULTIPLY AND ADD (short BFP)                   [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_add_bfp_short )
{
    int        x2, r1, r3, b2;
    VADR       effective_addr2;
    float32_t  op1, op2, op3, ans;
    U32        ieee_trap_conds =0;

    RXF( inst, regs, r1, r3, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op3, r3, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B31F MSDBR - MULTIPLY AND SUBTRACT (long BFP)               [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_subtract_bfp_long_reg )
{
    int        r1, r2, r3;
    float64_t  op1, op2, op3, ans;
    U32        ieee_trap_conds =0;

    RRD( inst, regs, r1, r2, r3 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op3, r3, regs );
    GET_FLOAT64_OP( op2, r2, regs );

    /* if Operand 1 is not a NaN, the sign bit is inverted */
    if (0
        || !(op1.v & 0x000FFFFFFFFFFFFF)
        || ((op1.v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
    )
        op1.v ^= 0x8000000000000000ULL;

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED1F MSDB  - MULTIPLY AND SUBTRACT (long BFP)               [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_subtract_bfp_long )
{
    int        x2, r1, r3, b2;
    VADR       effective_addr2;
    float64_t  op1, op2, op3, ans;
    U32        ieee_trap_conds =0;

    RXF( inst, regs, r1, r3, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op3, r3, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    /* if Operand 1 is not a NaN, the sign bit is inverted */
    if (0
        || !(op1.v & 0x000FFFFFFFFFFFFF)
        || ((op1.v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
    )
        op1.v ^= 0x8000000000000000ULL;

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B30F MSEBR - MULTIPLY AND SUBTRACT (short BFP)              [RRD] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_subtract_bfp_short_reg )
{
    int        r1, r2, r3;
    float32_t  op1, op2, op3, ans;
    U32        ieee_trap_conds =0;

    RRD( inst, regs, r1, r2, r3 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op3, r3, regs );
    GET_FLOAT32_OP( op2, r2, regs );

    /* if Operand 1 is not a NaN, the sign bit is inverted */
    if (0
        || !(op1.v & 0x007FFFFF)
        || ((op1.v & 0x7F800000) ^ 0x7F800000)
    )
        op1.v ^= 0x80000000;

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED0F MSEB  - MULTIPLY AND SUBTRACT (short BFP)              [RXF] */
/*-------------------------------------------------------------------*/
DEF_INST( multiply_subtract_bfp_short )
{
    int        x2, r1, r3, b2;
    VADR       effective_addr2;
    float32_t  op1, op2, op3, ans;
    U32        ieee_trap_conds = 0;

    RXF( inst, regs, r1, r3, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op3, r3, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    /* if Operand 1 is not a NaN, the sign bit is inverted */
    if (0
        || !(op1.v & 0x007FFFFF)
        || ((op1.v & 0x7F800000) ^ 0x7F800000)
    )
        op1.v ^= 0x80000000;

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_mulAdd( op2, op3, op1 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_NOCC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B316 SQXBR - SQUARE ROOT (extended BFP)                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( squareroot_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op1, op2;
    U32         ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OP( op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    op1 = f128_sqrt( op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
    }

    PUT_FLOAT128_NOCC( op1, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    SET_FPC_FLAGS_FROM_SF( regs );
}

/*-------------------------------------------------------------------*/
/* B315 SQDBR - SQUARE ROOT (long BFP)                         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( squareroot_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op1, op2;
    U32        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    op1 = f64_sqrt( op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
    }

    PUT_FLOAT64_NOCC( op1, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    SET_FPC_FLAGS_FROM_SF( regs );
}

/*-------------------------------------------------------------------*/
/* ED15 SQDB  - SQUARE ROOT (long BFP)                         [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( squareroot_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1, op2;
    U32        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    op1 = f64_sqrt( op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
    }

    PUT_FLOAT64_NOCC( op1, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    SET_FPC_FLAGS_FROM_SF( regs );
}

/*-------------------------------------------------------------------*/
/* B314 SQEBR - SQUARE ROOT (short BFP)                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( squareroot_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op1, op2;
    U32        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    op1 = f32_sqrt( op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
    }

    PUT_FLOAT32_NOCC( op1, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    SET_FPC_FLAGS_FROM_SF( regs );
}

/*-------------------------------------------------------------------*/
/* ED14 SQEB  - SQUARE ROOT (short BFP)                        [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( squareroot_bfp_short )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2;
    U32        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    op1 = f32_sqrt( op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
    }

    PUT_FLOAT32_NOCC( op1, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds, FPC_MASK_IMX );
    SET_FPC_FLAGS_FROM_SF( regs );
}


/*-------------------------------------------------------------------*/
/* B34B SXBR  - SUBTRACT (extended BFP)                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( subtract_bfp_ext_reg )
{
    int         r1, r2;
    float128_t  op1, op2, ans;
    U32         ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR2_CHECK( r1, r2, regs );

    GET_FLOAT128_OPS( op1, r1, op2, r2, regs );

    SET_SF_RM_FROM_FPC;
    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f128_sub( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f128_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_EXTD :
                SCALE_FACTOR_ARITH_UFLOW_EXTD );
    }

    PUT_FLOAT128_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B31B SDBR  - SUBTRACT (long BFP)                            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( subtract_bfp_long_reg )
{
    int        r1, r2;
    float64_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_sub( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED1B SDB   - SUBTRACT (long BFP)                            [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( subtract_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op1, r1, regs );
    VFETCH_FLOAT64_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f64_sub( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_LONG :
                SCALE_FACTOR_ARITH_UFLOW_LONG );
    }

    PUT_FLOAT64_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* B30B SEBR  - SUBTRACT (short BFP)                           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( subtract_bfp_short_reg )
{
    int        r1, r2;
    float32_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RRE( inst, regs, r1, r2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_sub( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED0B SEB   - SUBTRACT (short BFP)                           [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( subtract_bfp_short )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1, op2, ans;
    U32        ieee_trap_conds = 0;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );
    VFETCH_FLOAT32_OP( op2, effective_addr2, b2, regs );

    softfloat_exceptionFlags = 0;
    SET_SF_RM_FROM_FPC;

    ans = f32_sub( op1, op2 );

    if (softfloat_exceptionFlags)
    {
        IEEE_EXCEPTION_TRAP_XI( regs );
        ieee_trap_conds = ieee_exception_test_oux( regs );

        if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
            ans = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                SCALE_FACTOR_ARITH_OFLOW_SHORT :
                SCALE_FACTOR_ARITH_UFLOW_SHORT );
    }

    PUT_FLOAT32_CC( ans, r1, regs );

    IEEE_EXCEPTION_TRAP( regs, ieee_trap_conds,
        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
}

/*-------------------------------------------------------------------*/
/* ED10 TCEB  - TEST DATA CLASS (short BFP)                    [RXE] */
/* Per Jessen, Willem Konynenberg, 20 September 2001                 */
/*-------------------------------------------------------------------*/
DEF_INST( test_data_class_bfp_short )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float32_t  op1;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT32_OP( op1, r1, regs );

    regs->psw.cc = !!(((U32)effective_addr2) & float32_class( op1 ));
}

/*-------------------------------------------------------------------*/
/* ED11 TCDB  - TEST DATA CLASS (long BFP)                     [RXE] */
/* Per Jessen, Willem Konynenberg, 20 September 2001                 */
/*-------------------------------------------------------------------*/
DEF_INST( test_data_class_bfp_long )
{
    int        r1, x2, b2;
    VADR       effective_addr2;
    float64_t  op1;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );

    GET_FLOAT64_OP( op1, r1, regs );

    regs->psw.cc = !!(((U32)effective_addr2) & float64_class( op1 ));
}

/*-------------------------------------------------------------------*/
/* ED12 TCXB  - TEST DATA CLASS (extended BFP)                 [RXE] */
/* Per Jessen, Willem Konynenberg, 20 September 2001                 */
/*-------------------------------------------------------------------*/
DEF_INST( test_data_class_bfp_ext )
{
    int         r1, x2, b2;
    VADR        effective_addr2;
    float128_t  op1;

    RXE( inst, regs, r1, x2, b2, effective_addr2 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPREGPAIR_CHECK( r1, regs );

    GET_FLOAT128_OP( op1, r1, regs );

    regs->psw.cc = !!(((U32)effective_addr2) & float128_class( op1 ));
}

/*----------------------------------------------------------------------*/
/* DIVIDE TO INTEGER (All formats)                                      */
/*                                                                      */
/* Softfloat 3a does not have a Divide to Integer equivalent.           */
/*                                                                      */
/* Of the 64 possible combinations of operand class (NaN, Inf, etc),    */
/* only four actually require calculation of a quotent and remainder.   */
/*                                                                      */
/* So we will focus on those four cases first, followed by tests of     */
/* of operand classes to sort out results for the remaining 60 cases.   */
/* Note: it does not take 60 "else if" constructs to sort this out.     */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* B35B DIDBR - DIVIDE TO INTEGER (long BFP)                    [RRF-b] */
/*                                                                      */
/* Softfloat 3a does not have a Divide to Integer equivalent.           */
/*----------------------------------------------------------------------*/
DEF_INST( divide_integer_bfp_long_reg )
{
    int        r1, r2, r3;
    BYTE       m4, newcc;
    float64_t  op1, op2;
    float64_t  quo, rem;
    U32        ieee_trap_conds = 0;
    U32        op1_data_class, op2_data_class;

    RRF_RM( inst, regs, r1, r2, r3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m4,regs );

    if (r1 == r2 || r2 == r3 || r1 == r3)
        regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );

    GET_FLOAT64_OPS( op1, r1, op2, r2, regs );

    op1_data_class = float64_class( op1 );
    op2_data_class = float64_class( op2 );

    softfloat_exceptionFlags = 0;

    /************************************************************************************/
    /* Following if / else if / else implements a decision tree based on SA-22-7832-10  */
    /* Table 19-21 parts 1 and 2 on pages 19-29 and 19-30 respectively.                 */
    /*                                                                                  */
    /* ORDER OF TESTS IS IMPORTANT                                                      */
    /*                                                                                  */
    /*  1. Tests for cases that include two non-finite non-zeroes.                      */
    /*  2. Tests for cases that include one or two NaNs as input values                 */
    /*  3. Tests for cases that always generate the default quiet NaN                   */
    /*  4. Tests for cases that generate non-NaN results.                               */
    /*                                                                                  */
    /* When viewed from the perspective of Table 19-21, this order:                     */
    /*                                                                                  */
    /*  1. Addresses what is likely to be the most frequent case first                  */
    /*  2. Removes the bottom two rows and the right-hand two columns                   */
    /*  3. Removes the center two colums and the top and new bottom rows                */
    /*  4. Leaves only those cases that involve returning a zero or operand as a result.*/
    /*                                                                                  */
    /************************************************************************************/

    /************************************************************************************/
    /* Group 1: Tests for cases with finite non-zeros for both operands.  This is seen  */
    /* as the most frequent case, and should therefore be tested first.                 */
    /************************************************************************************/

                                                                    /* Both operands finite numbers?*/
    if ((op1_data_class & (float_class_neg_normal
                         | float_class_pos_normal
                         | float_class_neg_subnormal
                         | float_class_pos_subnormal))
        &&
        (op2_data_class & (float_class_neg_normal
                         | float_class_pos_normal
                         | float_class_neg_subnormal
                         | float_class_pos_subnormal))
    )
    {                                                               /* ..yes, we can do division */
        newcc = 0;                                                  /* Initial cc set to zero    */
        SET_SF_RM_FROM_MASK( m4 );

        quo = f64_div( op1, op2 );                                  /* calculate precise quotient                       */
        quo = f64_roundToInt( quo, softfloat_roundingMode, TRUE );  /* Round to integer per m4 rounding mode            */

        softfloat_exceptionFlags &= softfloat_flag_overflow;        /* Quotient only cares about overflow               */
        SET_SF_RM_FROM_FPC;                                         /* Set rounding mode from FPC for final remainder   */
        quo.v ^= 0x8000000000000000ULL;                             /* Reverse sign of integer quotient                 */
        rem = f64_mulAdd( quo, op2, op1 );                          /* Calculate remainder                              */
        quo.v ^= 0x8000000000000000ULL;                             /* Return sign of integer quotient to original value*/

        if (!(rem.v & 0x7fffffffffffffffULL))                       /* Is remainder zero?                               */
            rem.v = (op1.v & 0x8000000000000000ULL) | (rem.v & 0x7fffffffffffffffULL);
                                                                    /* ..yes, ensure remainder sign matches dividend    */
        if (!softfloat_exceptionFlags)                              /* If no exceptions, check for partial results      */
        {
            if (((quo.v & 0x7fffffffffffffffULL) > 0x4340000000000000ULL) && (rem.v & 0x7fffffffffffffffULL))
            {                                                       /* Quotient > 2^24th & rem <>0?                     */
                newcc += 2;                                         /* ..yes, indicate partial results in cc            */
                softfloat_roundingMode = softfloat_round_minMag;    /* Repeat calculation of quotient/remainder         */
                                                                    /* ..with rounding toward zero                      */
                softfloat_exceptionFlags = 0;                       /* Clear all prior flags                            */

                quo = f64_div( op1, op2 );                          /* calculate precise quotient                       */
                quo = f64_roundToInt( quo, softfloat_roundingMode, TRUE );  /* Round to integer per m4 rounding mode    */

                quo.v ^= 0x8000000000000000ULL;                     /* Reverse sign of integer quotient                 */
                rem = f64_mulAdd( quo, op2, op1 );                  /* Calculate remainder                              */
                quo.v ^= 0x8000000000000000ULL;                     /* Return sign of integer quotient to original value*/
                softfloat_exceptionFlags = 0;                       /* No exceptions or flags on partial results        */
            }
        }
        else                                                        /* Exception flagged...we have work to do.          */
        {
            if (softfloat_exceptionFlags & softfloat_flag_overflow) /* on oveflow, scale result and set cc=1 or 3       */
                                                                    /* and recalculate the remainder using a scaled     */
                                                                    /* quotient in 64-bit precision                     */
            {
                float128_t  quo128, intquo128, rem128;
                float128_t  op1128, op2128;

                newcc += 1;                                         /* Set condition code odd for quotient overflow     */
                softfloat_roundingMode = softfloat_round_minMag;    /* Repeat calculation of quotient/remainder         */
                                                                    /* ..with rounding toward zero                      */
                softfloat_exceptionFlags = 0;                       /* Clear all prior flags                            */

                quo = f64_div( op1, op2 );                          /* calculate precise quotient                       */
                quo = f64_roundToInt( quo, softfloat_roundingMode, TRUE );  /* Round to integer per m4 rounding mode    */
                quo = f64_scaledResult( SCALE_FACTOR_ARITH_OFLOW_LONG );

                op1128 = f64_to_f128( op1 );
                op2128 = f64_to_f128( op2 );

                softfloat_roundingMode = softfloat_round_minMag;

                quo128 = f128_div( op1128, op2128 );
                quo128.v[FLOAT128_LO] &= 0xf000000000000000ULL;     /* truncate to long precision with extended exponent*/

                intquo128 = f128_roundToInt( quo128, softfloat_round_minMag, FALSE );
                intquo128.v[FLOAT128_HI] ^= 0x8000000000000000ULL;  /* flip sign of dividend for fused multiply-add     */

                rem128 = f128_mulAdd( intquo128, op2128, op1128 );  /* rem = (-intquo * divisor) + dividend             */

                intquo128.v[FLOAT128_HI] ^= 0x8000000000000000ULL;  /* Restore sign of dividend                         */
                softfloat_exceptionFlags = 0;                       /* clear any prior Softfloat flags                  */
                softfloat_roundingMode = softfloat_round_minMag;    /* round remainder toward zero (but remainder       */

                rem = f128_to_f64( rem128 );                        /* should be exact!?)                               */

                if (rem.v & 0x7fffffffffffffffULL)                  /* non-zero remainder?                              */
                    newcc += 2;                                     /* yes, indicate partial results                    */

            }
            else if (softfloat_exceptionFlags & (softfloat_flag_tiny | softfloat_flag_underflow))
            {
                /* Inexact and underflow'ed remainder issues and traps are handled by code at the end of Divide To Integer  */
                /* But because this is the only situation where the remainder might need scaling, we will do it here        */
                if (regs->fpc & FPC_MASK_IMU)
                    rem = f64_scaledResult( SCALE_FACTOR_ARITH_UFLOW_LONG );
            }
        }
    }

    /************************************************************************************/
    /* Group 2: tests for cases with NaNs for one or both operands                      */
    /* The sequence is required to ensure that the generated results match the IBM NaN  */
    /* propagation rules shown in Table 19-21                                           */
    /************************************************************************************/

    /* ******* NEXT FOUR TESTS, ALL GROUP 2 TESTS, MUST REMAIN IN SEQUENCE *******      */
    else if (op1_data_class & (float_class_neg_signaling_nan | float_class_pos_signaling_nan))   /* first case: op1 an SNaN?  */
    {
        quo = op1;
        FLOAT64_MAKE_QNAN( quo );
        rem = quo;
        softfloat_exceptionFlags |= softfloat_flag_invalid;
        newcc = 1;                                                                               /* Any NaN returns cc=1      */
    }
    else if (op2_data_class & (float_class_neg_signaling_nan | float_class_pos_signaling_nan))   /* second case: op2 an SNaN? */
    {
        quo = op2;
        FLOAT64_MAKE_QNAN( quo );
        rem = quo;
        softfloat_exceptionFlags |= softfloat_flag_invalid;
        newcc = 1;                                                                              /* Any NaN returns cc=1       */
    }
    else if (op1_data_class & (float_class_neg_quiet_nan | float_class_pos_quiet_nan))          /* third case: op1 a QNaN?    */
    {
        rem = quo = op1;
        newcc = 1;                                                                              /* Any NaN returns cc=1       */
    }
    else if (op2_data_class & (float_class_neg_quiet_nan | float_class_pos_quiet_nan))          /* fourth case: op2 a QNaN?   */
    {
        rem = quo = op2;
        newcc = 1;                                                                              /* Any NaN returns cc=1       */
    }
    /* END OF FOUR TESTS THAT MUST REMAIN IN SEQUENCE                                   */

    /************************************************************************************/
    /* NEXT TEST MUST FOLLOW ALL FOUR NAN TESTS                                         */
    /* Group 3: Test cases that generate the default NaN and IEEE exception Invalid     */
    /* If operand 1 is an infinity OR operand two is a zero, and none of the above      */
    /* conditions are met, i.e., neither operand is a NaN, return a default NaN         */
    /************************************************************************************/
    else if ((op1_data_class & (float_class_neg_infinity | float_class_pos_infinity))           /* Operand 1 an infinity?             */
         ||  (op2_data_class & (float_class_neg_zero     | float_class_pos_zero))               /* ..or operand 2 a zero?             */
    )
    {                                                                                           /* ..yes, return DNaN, raise invalid  */
        quo = rem = float64_default_qnan;
        softfloat_exceptionFlags |= softfloat_flag_invalid;
        newcc = 1;
    }
    /* ABOVE TEST MUST IMMEDIATELY FOLLOW ALL FOUR NAN TESTS                            */

    /************************************************************************************/
    /* Group 4: Tests for cases that generate zero or an operand value as a result.     */
    /*                                                                                  */
    /* At this point, only the remaining operand combinations remain:                   */
    /* - Operand 1 is zero and operand 2 is non-zero (either finite or infinity)        */
    /* - Operand 1 is finite and operand 2 is infinity                                  */
    /*                                                                                  */
    /* The result is the same for each of the above: Operand 1 becomes the remainder,   */
    /* and the quotient is zero with a signed determined by the signs of the operands.  */
    /* Exclusive Or sets the sign correctly.                                            */
    /************************************************************************************/
    else
    {
        rem = op1;                                          /* remainder is operand 1                               */
        quo.v = (op1.v ^ op2.v) & 0x8000000000000000ULL;    /* quotient zero, sign is exclusive or of operand signs */
        newcc = 0;
    }

    IEEE_EXCEPTION_TRAP_XI( regs );                         /* IEEE Invalid Exception raised and trappable?         */
    ieee_trap_conds = ieee_exception_test_oux( regs );

    /* *********** Underflow flag means remainder underflowed.  It has already been scaled if necessary             */

    regs->psw.cc = newcc;

    PUT_FLOAT64_NOCC( rem, r1, regs );
    PUT_FLOAT64_NOCC( quo, r3, regs );

    ieee_cond_trap( regs, ieee_trap_conds );
}

/*-------------------------------------------------------------------*/
/* B353 DIEBR - DIVIDE TO INTEGER (short BFP)                [RRF-b] */
/*-------------------------------------------------------------------*/
DEF_INST( divide_integer_bfp_short_reg )
{
    int        r1, r2, r3;
    BYTE       m4, newcc;
    float32_t  op1, op2;
    float32_t  rem, quo;
    U32        ieee_trap_conds = 0;
    U32        op1_data_class, op2_data_class;

    RRF_RM( inst, regs, r1, r2, r3, m4 );
    TXF_FLOAT_INSTR_CHECK( regs );
    BFPINST_CHECK( regs );
    BFPRM_CHECK( m4, regs );

    if (r1 == r2 || r2 == r3 || r1 == r3)
        regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );

    GET_FLOAT32_OPS( op1, r1, op2, r2, regs );

    op1_data_class = float32_class( op1 );
    op2_data_class = float32_class( op2 );

    softfloat_exceptionFlags = 0;

    /************************************************************************************/
    /* Following if / else if / else implements a decision tree based on SA-22-7832-10  */
    /* Table 19-21 parts 1 and 2 on pages 19-29 and 19-30 respectively.                 */
    /*                                                                                  */
    /* ORDER OF TESTS IS IMPORTANT                                                      */
    /*                                                                                  */
    /*  1. Tests for cases that include two non-finite non-zeroes.                      */
    /*  2. Tests for cases that include one or two NaNs as input values                 */
    /*  3. Tests for cases that always generate the default quiet NaN                   */
    /*  4. Tests for cases that generate non-NaN results.                               */
    /*                                                                                  */
    /* When viewed from the perspective of Table 19-21, this order:                     */
    /*                                                                                  */
    /*  1. Addresses what is likely to be the most frequent case first                  */
    /*  2. Removes the bottom two rows and the right-hand two columns                   */
    /*  3. Removes the center two colums and the top and new bottom rows                */
    /*  4. Leaves only those cases that involve returning a zero or operand as a result.*/
    /*                                                                                  */
    /************************************************************************************/

    /************************************************************************************/
    /* Group 1: Tests for cases with finite non-zeros for both operands.  This is seen  */
    /* as the most frequent case, and should therefore be tested first.                 */
    /************************************************************************************/

                                                                    /* Both operands finite numbers?*/
    if ((op1_data_class & (float_class_neg_normal
                         | float_class_pos_normal
                         | float_class_neg_subnormal
                         | float_class_pos_subnormal))
        &&
        (op2_data_class & (float_class_neg_normal
                         | float_class_pos_normal
                         | float_class_neg_subnormal
                         | float_class_pos_subnormal))
    )
    {                                                               /* ..yes, we can do division */

        newcc = 0;                                                  /* Initial cc set to zero    */
        SET_SF_RM_FROM_MASK( m4 );

        quo = f32_div( op1, op2 );                                  /* calculate precise quotient                       */
        quo = f32_roundToInt( quo, softfloat_roundingMode, TRUE );  /* Round to integer per m4 rounding mode            */

        softfloat_exceptionFlags &= softfloat_flag_overflow;        /* Quotient only cares about overflow               */
        SET_SF_RM_FROM_FPC;                                         /* Set rounding mode from FPC for final remainder   */
        quo.v ^= 0x80000000;                                        /* Reverse sign of integer quotient                 */
        rem = f32_mulAdd(quo, op2, op1);                            /* Calculate remainder                              */
        quo.v ^= 0x80000000;                                        /* Return sign of integer quotient to original value*/

        if (!(rem.v & 0x7fffffff))                                  /* Is remainder zero?                               */
            rem.v = (op1.v & 0x80000000) | (rem.v & 0x7fffffff);    /* ..yes, ensure remainder sign matches dividend    */

        if (!softfloat_exceptionFlags)                              /* If no exceptions, check for partial results      */
        {
            if (((quo.v & 0x7fffffff) > 0x4B800000) && (rem.v & 0x7fffffff))  /* Quotient > 2^24th & rem <>0?           */
            {
                newcc += 2;                                         /* ..yes, indicate partial results in cc            */
                softfloat_roundingMode = softfloat_round_minMag;    /* Repeat calculation of quotient/remainder         */
                                                                    /* ..with rounding toward zero                      */
                softfloat_exceptionFlags = 0;                       /* Clear all prior flags                            */

                quo = f32_div( op1, op2 );                          /* calculate precise quotient                       */
                quo = f32_roundToInt( quo, softfloat_roundingMode, TRUE );  /* Round to integer per m4 rounding mode    */

                quo.v ^= 0x80000000;                                /* Reverse sign of integer quotient                 */

                rem = f32_mulAdd( quo, op2, op1 );                  /* Calculate remainder                              */

                quo.v ^= 0x80000000;                                /* Return sign of integer quotient to original value*/
                softfloat_exceptionFlags = 0;                       /* No exceptions or flags on partial results        */
            }
        }
        else                                                        /* Exception flagged...we have work to do.          */
        {
            if (softfloat_exceptionFlags & softfloat_flag_overflow) /* on oveflow, scale result and set cc=1 or 3       */
                                                                    /* and recalculate the remainder using a scaled     */
                                                                    /* quotient in 64-bit precision                     */
                                                                    /* Note that there is no fractional part to the     */
                                                                    /* quotient when the quotient overflows             */
            {
                float64_t  quo64, intquo64, rem64;
                float64_t  op164, op264;

                newcc += 1;                                         /* Set condition code odd for quotient overflow     */
                softfloat_roundingMode = softfloat_round_minMag;    /* Repeat calculation of quotient/remainder         */
                                                                    /* ..with rounding toward zero                      */
                softfloat_exceptionFlags = 0;                       /* Clear all prior flags                            */

                quo = f32_div( op1, op2 );                          /* calculate precise quotient                       */
                quo = f32_roundToInt( quo, softfloat_roundingMode, TRUE ); /* Partial result, round to zero             */
                quo = f32_scaledResult( SCALE_FACTOR_ARITH_OFLOW_SHORT );  /* Scale quotient                            */

                op164 = f32_to_f64( op1 );
                op264 = f32_to_f64( op2 );

                softfloat_roundingMode = softfloat_round_minMag;

                quo64 = f64_div( op164, op264 );

                intquo64.v = quo64.v & 0xFFFFFFFFE0000000ULL;       /* Truncate significand to BFP Short bits           */
                intquo64.v ^= 0x8000000000000000ULL;                /* flip sign of dividend for fused multiply-add     */

                rem64 = f64_mulAdd( intquo64, op264, op164 );       /* -rem = intquo * divisor + (-dividend)            */

                intquo64.v ^= 0x8000000000000000ULL;                /* Restore sign of dividend                         */
                softfloat_exceptionFlags = 0;                       /* clear any prior Softfloat flags                  */

                rem = f64_to_f32( rem64 );                          /* should be exact!?)                               */

                if (rem.v & 0x7fffffff)                             /* non-zero remainder?                              */
                    newcc += 2;                                     /* yes, indicate partial results                    */
            }
            else if (softfloat_exceptionFlags & (softfloat_flag_tiny | softfloat_flag_underflow))
            {
                /* Inexact and underflow'ed remainder issues and traps are handled by code at the end of Divide To Integer  */
                /* But because this is the only situation where the remainder might need scaling, we will do it here        */
                if (regs->fpc & FPC_MASK_IMU)
                    rem = f32_scaledResult(SCALE_FACTOR_ARITH_UFLOW_SHORT);
            }
        }
    }

    /************************************************************************************/
    /* Group 2: tests for cases with NaNs for one or both operands                      */
    /* The sequence is required to ensure that the generated results match the IBM NaN  */
    /* propagation rules shown in Table 19-21                                           */
    /************************************************************************************/

    /* ******* NEXT FOUR TESTS, ALL GROUP 2 TESTS, MUST REMAIN IN SEQUENCE *******      */
    else if (op1_data_class & (float_class_neg_signaling_nan | float_class_pos_signaling_nan))   /* first case: op1 an SNaN?  */
    {
        quo = op1;
        FLOAT32_MAKE_QNAN( quo );
        rem = quo;
        softfloat_exceptionFlags |= softfloat_flag_invalid;
        newcc = 1;                                                                               /* Any NaN returns cc=1      */
    }
    else if (op2_data_class & (float_class_neg_signaling_nan | float_class_pos_signaling_nan))   /* second case: op2 an SNaN? */
    {
        quo = op2;
        FLOAT32_MAKE_QNAN( quo );
        rem = quo;
        softfloat_exceptionFlags |= softfloat_flag_invalid;
        newcc = 1;                                                                              /* Any NaN returns cc=1       */
    }
    else if (op1_data_class & (float_class_neg_quiet_nan | float_class_pos_quiet_nan))          /* third case: op1 a QNaN?    */
    {
        rem = quo = op1;
        newcc = 1;                                                                              /* Any NaN returns cc=1       */
    }
    else if (op2_data_class & (float_class_neg_quiet_nan | float_class_pos_quiet_nan))          /* fourth case: op2 a QNaN?   */
    {
        rem = quo = op2;
        newcc = 1;                                                                              /* Any NaN returns cc=1       */
    }
    /* END OF FOUR TESTS THAT MUST REMAIN IN SEQUENCE                                   */

    /************************************************************************************/
    /* NEXT TEST MUST FOLLOW ALL FOUR NAN TESTS                                         */
    /* Group 3: Test cases that generate the default NaN and IEEE exception Invalid     */
    /* If operand 1 is an infinity OR operand two is a zero, and none of the above      */
    /* conditions are met, i.e., neither operand is a NaN, return a default NaN         */
    /************************************************************************************/

    else if ((op1_data_class & (float_class_neg_infinity | float_class_pos_infinity))           /* Operand 1 an infinity?     */
         ||  (op2_data_class & (float_class_neg_zero     | float_class_pos_zero))               /* ..or operand 2 a zero?     */
    )
    {                                                                                           /* ..yes, return DNaN, raise invalid  */
        quo = rem = float32_default_qnan;
        softfloat_exceptionFlags |= softfloat_flag_invalid;
        newcc = 1;
    }
    /* ABOVE TEST MUST IMMEDIATELY FOLLOW ALL FOUR NAN TESTS                            */

    /************************************************************************************/
    /* Group 4: Tests for cases that generate zero or an operand value as a result.     */
    /*                                                                                  */
    /* At this point, only the remaining operand combinations remain:                   */
    /* - Operand 1 is zero and operand 2 is non-zero (either finite or infinity)        */
    /* - Operand 1 is finite and operand 2 is infinity                                  */
    /*                                                                                  */
    /* The result is the same for each of the above: Operand 1 becomes the remainder,   */
    /* and the quotient is zero with a signed determined by the signs of the operands.  */
    /* Exclusive Or sets the sign correctly.                                            */
    /************************************************************************************/
    else
    {
        rem = op1;                                          /* remainder is operand 1                       */
        quo.v = (op1.v ^ op2.v) & 0x80000000;               /* quotient zero, sign is exclusive or of operand signs   */
        newcc = 0;
    }

    IEEE_EXCEPTION_TRAP_XI( regs );                         /* IEEE Invalid Exception raised and trappable?         */
    ieee_trap_conds = ieee_exception_test_oux( regs );

    /* *********** Underflow flag means remainder underflowed.  It has already been scaled if necessary             */

    regs->psw.cc = newcc;

    PUT_FLOAT32_NOCC( rem, r1, regs );
    PUT_FLOAT32_NOCC( quo, r3, regs );

    ieee_cond_trap( regs, ieee_trap_conds );
}

/*===========================================================================*/
/*  Start of z/Arch Vector Floating Point instructions                       */
/*===========================================================================*/

#if defined( FEATURE_129_ZVECTOR_FACILITY )

/*
 * z/Architecture Principles of Operation (SA22-7832-10 onwards)
 * Chapter 24. Vector Floating-Point Instructions
 */

#undef IEEE_EXCEPTION_TRAP_XI
#undef IEEE_EXCEPTION_TRAP_XZ
#undef IEEE_EXCEPTION_TRAP

// Vector processing with VXC for IEEE exception and element index
static void vector_ieee_trap( REGS *regs, BYTE vxc)
{
    regs->dxc = vxc;                   /*  Save VXC in PSA         */
    regs->fpc &= ~FPC_DXC;             /*  Clear previous DXC/VXC  */
    regs->fpc |= ((U32)vxc << FPC_DXC_SHIFT);
    regs->program_interrupt( regs, PGM_VECTOR_PROCESSING_EXCEPTION );
}

static void vector_ieee_cond_trap( int vix, REGS *regs, U32 ieee_traps )
{
    if (ieee_traps & FPC_MASK_IMI)
        vector_ieee_trap( regs, ((vix << VXC_VIX_SHIFT) | VXC_IEEE_INVALID_OP) );

    else if (ieee_traps & FPC_MASK_IMZ)
        vector_ieee_trap( regs, ((vix << VXC_VIX_SHIFT) | VXC_IEEE_DIV_ZERO) );

    else if (ieee_traps & FPC_MASK_IMO)
        vector_ieee_trap( regs, ((vix << VXC_VIX_SHIFT) | VXC_IEEE_OVERFLOW) );

    else if (ieee_traps & FPC_MASK_IMU)
        vector_ieee_trap( regs, ((vix << VXC_VIX_SHIFT) | VXC_IEEE_UNDERFLOW) );

    else if (ieee_traps & FPC_MASK_IMX)
        vector_ieee_trap( regs, ((vix << VXC_VIX_SHIFT) | VXC_IEEE_INEXACT) );
}

/* fastpath test for Xi trap; many instructions only return Xi */
#define VECTOR_IEEE_EXCEPTION_TRAP_XI( _vix, _regs )                 \
                                                                     \
    if (1                                                            \
        && (softfloat_exceptionFlags & softfloat_flag_invalid)       \
        && (_regs->fpc & FPC_MASK_IMI)                               \
    )                                                                \
        vector_ieee_trap( _regs, VXC_IEEE_INVALID_OP )

/* fastpath test for Xz trap; only Divide returns Xz  */
#define VECTOR_IEEE_EXCEPTION_TRAP_XZ( _vix, _regs )                 \
                                                                     \
    if (1                                                            \
        && (softfloat_exceptionFlags & softfloat_flag_infinite)      \
        && (_regs->fpc & FPC_MASK_IMZ)                               \
    )                                                                \
        vector_ieee_trap( _regs, VXC_IEEE_DIV_ZERO )

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

#define VECTOR_PUT_FLOAT128_NOCC( op, v, regs )  ARCH_DEP( put_float128 )( &op, &regs->VR_Q( v ).d[FLOAT128_HI], &regs->VR_Q( v ).d[FLOAT128_LO])
#define VECTOR_PUT_FLOAT64_NOCC(  op, v, i, regs )  ARCH_DEP( put_float64  )( &op, &regs->VR_D( v, i ))
#define VECTOR_PUT_FLOAT32_NOCC(  op, v, i, regs )  ARCH_DEP( put_float32  )( &op, &regs->VR_F( v, i ))

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
            if (i == 0 || !M5_SE)
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
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
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
            }
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
        float64_t  op1, op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
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

                op1 = f64_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1, op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
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

                op1 = f32_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
           }
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
                    SCALE_FACTOR_ARITH_OFLOW_LONG :
                    SCALE_FACTOR_ARITH_UFLOW_LONG );
        }

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );


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
        float64_t  op1, op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT64_OP( op4, v4, i, regs );
                VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                ieee_trap_conds = 0;
                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f64_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1, op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT32_OP( op4, v4, i, regs );
                VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                ieee_trap_conds = 0;
                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f32_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
           }
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
                    SCALE_FACTOR_ARITH_OFLOW_LONG :
                    SCALE_FACTOR_ARITH_UFLOW_LONG );
        }

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
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
        float64_t  op1, op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
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

                op1 = f64_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                /* if Operand 1 is not a NaN, the sign bit is inverted */
                if (0
                    || !(op1.v & 0x000FFFFFFFFFFFFF)
                    || ((op1.v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
                )
                    op1.v ^= 0x8000000000000000ULL;

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1, op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
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

                op1 = f32_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                /* if Operand 1 is not a NaN, the sign bit is inverted */
                if (0
                    || !(op1.v & 0x007FFFFF)
                    || ((op1.v & 0x7F800000) ^ 0x7F800000)
                )
                    op1.v ^= 0x80000000;

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
           }
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
                    SCALE_FACTOR_ARITH_OFLOW_LONG :
                    SCALE_FACTOR_ARITH_UFLOW_LONG );
        }

        /* if Operand 1 is not a NaN, the sign bit is inverted */
        if (0
            || !(op1.v[FLOAT128_HI] & 0x000FFFFFFFFFFFFF)
            || ((op1.v[FLOAT128_HI] & 0x7FF0000000000000) ^ 0x7FF0000000000000)
        )
            op1.v[FLOAT128_HI] ^= 0x8000000000000000ULL;

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );


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
        float64_t  op1, op2, op3, op4;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT64_OP( op4, v4, i, regs );
                VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                ieee_trap_conds = 0;
                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f64_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                /* if Operand 1 is not a NaN, the sign bit is inverted */
                if (0
                    || !(op1.v & 0x000FFFFFFFFFFFFF)
                    || ((op1.v & 0x7FF0000000000000) ^ 0x7FF0000000000000)
                )
                    op1.v ^= 0x8000000000000000ULL;

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m6 == 2 )  // Short format
    {
        float32_t  op1, op2, op3, op4;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT32_OP( op4, v4, i, regs );
                VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                ieee_trap_conds = 0;
                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f32_mulAdd( op2, op3, op4 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
               }

                /* if Operand 1 is not a NaN, the sign bit is inverted */
                if (0
                    || !(op1.v & 0x007FFFFF)
                    || ((op1.v & 0x7F800000) ^ 0x7F800000)
                )
                    op1.v ^= 0x80000000;

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
           }
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
                    SCALE_FACTOR_ARITH_OFLOW_LONG :
                    SCALE_FACTOR_ARITH_UFLOW_LONG );
        }

        /* if Operand 1 is not a NaN, the sign bit is inverted */
        if (0
            || !(op1.v[FLOAT128_HI] & 0x000FFFFFFFFFFFFF)
            || ((op1.v[FLOAT128_HI] & 0x7FF0000000000000) ^ 0x7FF0000000000000)
        )
            op1.v[FLOAT128_HI] ^= 0x8000000000000000ULL;

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
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
        U64         op1;
        float64_t   op2;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M4_SE)
            {
                SET_SF_RM_FROM_MASK( m5 );

                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
                softfloat_exceptionFlags = 0;

                if (FLOAT64_ISNAN( op2 ))
                {
                    op1 = 0;
                    softfloat_raiseFlags( softfloat_flag_invalid );
                }
                else
                {
                    SET_SF_RM_FROM_MASK( m3 );
                    op1 = f64_to_ui64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
                }

                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                if (softfloat_exceptionFlags & softfloat_flag_invalid)
                {
                    if (!SUPPRESS_INEXACT( m4 ))
                        softfloat_exceptionFlags |= softfloat_flag_inexact;
                }

                regs->VR_D( v1, i ) = op1;

                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }
        }
    }
    else if ( m3 == 2 )  /* BFP short format to word */
    {
        U32         op1;
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M4_SE)
            {
                SET_SF_RM_FROM_MASK( m5 );

                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
                softfloat_exceptionFlags = 0;

                if (FLOAT32_ISNAN( op2 ))
                {
                    op1 = 0;
                    softfloat_raiseFlags( softfloat_flag_invalid );
                }
                else
                {
                    SET_SF_RM_FROM_MASK( m3 );
                    op1 = f32_to_ui32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
                }

                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                if (softfloat_exceptionFlags & softfloat_flag_invalid)
                {
                    if (!SUPPRESS_INEXACT( m4 ))
                        softfloat_exceptionFlags |= softfloat_flag_inexact;
                }

                regs->VR_F( v1, i ) = op1;

                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }
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
        float64_t  op1;
        U64        op2;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M4_SE)
            {
                SET_SF_RM_FROM_MASK( m5 );

                op2 = regs->VR_D(  v2, i );
                softfloat_exceptionFlags = 0;

                op1 = ui64_to_f64( op2 );

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
                {
                    ieee_trap_conds = ieee_exception_test_oux( regs );
                    VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                }
            }
        }
    }
    else if ( m3 == 2 )  // Word to BFP short format
    {
        float32_t  op1;
        U32        op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M4_SE)
            {
                SET_SF_RM_FROM_MASK( m5 );

                op2 = regs->VR_F(  v2, i );
                softfloat_exceptionFlags = 0;

                op1 = ui32_to_f32( op2 );

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
                {
                    ieee_trap_conds = ieee_exception_test_oux( regs );
                    VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                }
            }
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
        S64         op1;
        float64_t   op2;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M4_SE)
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
                    op1 = -(0x7FFFFFFFFFFFFFFFULL) - 1;
                    softfloat_raiseFlags( softfloat_flag_invalid );
                }
                else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
                {
                    op1 = 0;
                }
                else
                {
                    if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
                        op1 = 0;
                    else
                    {
                        SET_SF_RM_FROM_MASK( m5 );
                        op1 = f64_to_i64( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
                    }
                }

                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                if (softfloat_exceptionFlags & softfloat_flag_invalid)
                {
                    if (!SUPPRESS_INEXACT( m4 ))
                        softfloat_exceptionFlags |= softfloat_flag_inexact;
                }

                regs->VR_D( v1, i ) = op1;

                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }
        }
    }
    else if ( m3 == 2 )  /* BFP short format to word */
    {
        S32         op1;
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M4_SE)
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
                    op1 = -0x7FFFFFFF - 1;
                    softfloat_raiseFlags( softfloat_flag_invalid );
                }
                else if (op2_dataclass & (float_class_neg_zero | float_class_pos_zero))
                {
                    op1 = 0;
                }
                else
                {
                    if (op2_dataclass & (float_class_neg_subnormal | float_class_pos_subnormal))
                        op1 = 0;
                    else
                    {
                        SET_SF_RM_FROM_MASK( m3 );
                        op1 = f32_to_i32( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));
                    }
                }

                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                if (softfloat_exceptionFlags & softfloat_flag_invalid)
                {
                    if (!SUPPRESS_INEXACT( m4 ))
                        softfloat_exceptionFlags |= softfloat_flag_inexact;
                }

                regs->VR_F( v1, i ) = op1;

                ieee_trap_conds = ieee_exception_test_oux( regs );
                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
            }
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
        float64_t   op1;
        S64         op2;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M4_SE)
            {
                SET_SF_RM_FROM_MASK( m5 );

                op2 = regs->VR_D( v2, i );
                softfloat_exceptionFlags = 0;
                op1 = i64_to_f64( op2);
                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                /* Inexact occurred and not masked by m4? */
                if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
                {
                    /* Yes, set FPC flags and test for a trap */
                    ieee_trap_conds = ieee_exception_test_oux( regs );
                    VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                }
            }
        }
    }
    else if ( m3 == 2 )  // Word to BFP short format
    {
        /* Fixed 32-bit may need to be rounded to fit in the 23+1    */
        /* bits available in a short BFP, IEEE Inexact may be        */
        /* raised. If m4 Inexact suppression (XxC)  See SUPPRESS_INEXACT is on, then no    */
        /* inexact exception is recognized (no trap nor flag set).   */
        float32_t   op1;
        S32         op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M4_SE)
            {
                SET_SF_RM_FROM_MASK( m5 );

                op2 = regs->VR_F( v2, i );
                softfloat_exceptionFlags = 0;
                op1 = i32_to_f32( op2 );
                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                /* Inexact occurred and not masked by m4? */
                if (softfloat_exceptionFlags && !SUPPRESS_INEXACT( m4 ))
                {
                    /* Yes, set FPC flags and test for a trap */
                    ieee_trap_conds = ieee_exception_test_oux( regs );
                    VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                }
            }
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
        float64_t   op1;
        float32_t   op2;
        int         j;

        for (i=0, j=0; i < 4; i+=2, j++)
        {
            if (i == 0 || !M4_SE)
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
                op1 = f32_to_f64( op2 );
                VECTOR_PUT_FLOAT64_NOCC( op1, v1, j, regs );
            }
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
        float32_t   op1;
        float64_t   op2;

        for (i=0, j=0; i < 2; i++, j+=2)
        {
            if (i == 0 || !M4_SE)
            {
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                SET_SF_RM_FROM_MASK( m5 );

                softfloat_exceptionFlags = 0;

                op1 = f64_to_f32( op2 );

                if (SUPPRESS_INEXACT( m4 ))
                    softfloat_exceptionFlags &= ~softfloat_flag_inexact;

                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, j, regs );

                if (softfloat_exceptionFlags)
                {
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                        FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
                }
            }
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

        VECTOR_PUT_FLOAT64_NOCC( op1, v1, 0, regs );
        VECTOR_PUT_FLOAT64_NOCC( op1, v1, 1, regs );

        if (softfloat_exceptionFlags)
        {
            ieee_trap_conds = ieee_exception_test_oux( regs );

            VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
                FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
        }
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
        float64_t   op1, op2;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M4_SE)
            {
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_MASK( m5 );

                op1 = f64_roundToInt( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                if (softfloat_exceptionFlags)
                {
                    ieee_trap_conds = ieee_exception_test_oux( regs );
                    VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                }
            }
        }
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op1, op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M4_SE)
            {
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_MASK( m5 );

                op1 = f32_roundToInt( op2, softfloat_roundingMode, !SUPPRESS_INEXACT( m4 ));

                VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                if (softfloat_exceptionFlags)
                {
                    ieee_trap_conds = ieee_exception_test_oux( regs );
                    VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                }
            }
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

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        if (softfloat_exceptionFlags)
        {
            ieee_trap_conds = ieee_exception_test_oux( regs );
            VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds, FPC_MASK_IMX );
        }
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
        VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
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
        VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
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
        VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
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
        VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
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
        VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
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
        VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
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
            if (i == 0 || !M4_SE)
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
            }
        }
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M4_SE)
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
            }
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
        float64_t   op1, op2;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M4_SE)
            {
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f64_sqrt( op2 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
                }

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                SET_FPC_FLAGS_FROM_SF( regs );
            }
        }
    }
    else if ( m3 == 2 )  // Short format
    {
        float32_t   op1, op2;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M4_SE)
            {
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f32_sqrt( op2 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    IEEE_EXCEPTION_TEST_TRAPS( regs, ieee_trap_conds, FPC_MASK_IMX );
                }

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds, FPC_MASK_IMX );
                SET_FPC_FLAGS_FROM_SF( regs );
            }
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

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds, FPC_MASK_IMX );
        SET_FPC_FLAGS_FROM_SF( regs );
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
        float64_t   op1, op2, op3;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f64_sub( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
                }

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1, op2, op3;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f32_sub( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_SHORT :
                            SCALE_FACTOR_ARITH_UFLOW_SHORT );
                }

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
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

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
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
        float64_t   op1, op2, op3;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f64_add( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
                }

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1, op2, op3;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f32_add( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_SHORT :
                            SCALE_FACTOR_ARITH_UFLOW_SHORT );
                }

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
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

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
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
        float64_t   op1, op2, op3;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f64_div( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    VECTOR_IEEE_EXCEPTION_TRAP_XZ( i, regs );

                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
                }

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1, op2, op3;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f32_div( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    VECTOR_IEEE_EXCEPTION_TRAP_XZ( i, regs );

                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_SHORT :
                            SCALE_FACTOR_ARITH_UFLOW_SHORT );
                }

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
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

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
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
        float64_t   op1, op2, op3;

        for (i=0; i < 2; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f64_mul( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f64_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_LONG :
                            SCALE_FACTOR_ARITH_UFLOW_LONG );
                }

                VECTOR_PUT_FLOAT64_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op1, op2, op3;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
            {
                VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
                VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );

                softfloat_exceptionFlags = 0;
                SET_SF_RM_FROM_FPC;

                op1 = f32_mul( op2, op3 );

                if (softfloat_exceptionFlags)
                {
                    VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
                    ieee_trap_conds = ieee_exception_test_oux( regs );

                    if (ieee_trap_conds & (FPC_MASK_IMO | FPC_MASK_IMU))
                        op1 = f32_scaledResult( ieee_trap_conds & FPC_MASK_IMO ?
                            SCALE_FACTOR_ARITH_OFLOW_SHORT :
                            SCALE_FACTOR_ARITH_UFLOW_SHORT );
                }

                VECTOR_PUT_FLOAT32_NOCC( op1, v1, i, regs );

                VECTOR_IEEE_EXCEPTION_TRAP( i, regs, ieee_trap_conds,
                    FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
            }
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

        VECTOR_PUT_FLOAT128_NOCC( op1, v1, regs );

        VECTOR_IEEE_EXCEPTION_TRAP( 0, regs, ieee_trap_conds,
            FPC_MASK_IMO | FPC_MASK_IMU | FPC_MASK_IMX );
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
            if (i == 0 || !M5_SE)
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
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2, op3;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
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
            }
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
            if (i == 0 || !M5_SE)
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
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2, op3;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
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
            }
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
            if (i == 0 || !M5_SE)
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
            }
        }
    }
    else if ( m4 == 2 )  // Short format
    {
        float32_t   op2, op3;

        for (i=0; i < 4; i++)
        {
            if (i == 0 || !M5_SE)
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
                    regs->VR_D( v1, i ) = 0xFFFFFFFF;
                }
                else
                {
                    not_greater_than_or_unordered_found = TRUE;
                    regs->VR_D( v1, i ) = 0x00000000;
                }
            }
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
//      int     i;
//      BYTE    newcc = 3;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    /* Todo: FixMe! Investigate how to implement this instruction! */
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );


    /* Temporary! */
    UNREFERENCED( v1 );
    UNREFERENCED( v2 );
    UNREFERENCED( v3 );
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );


//  #define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
//  #define M5_RE ((m5 & 0x7) != 0) // Reserved
//
//      if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
//      {
//          if ( (m6 >= 5 && m6 <= 7) || m6 > 12 || M5_RE || m4 < 2 || m4 > 4 )
//              ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
//      }
//      else
//          ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
//
//      if ( m4 == 3 )  // Long format
//      {
//          float64_t   op2, op3;
//
//          for (i=0; i < 2; i++)
//          {
//              if (i == 0 || !M5_SE)
//              {
//                  VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
//                  VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
//
//                  /* Todo: FixMe! Write some code that implements this instruction! */
//
//
//                  switch (m6)
//                  {
//                  case 0:   // IEEE MinNum
//                      break;
//                  case 1:   // Java Math.Min()
//                      break;
//                  case 2:   // C-Style Min Macro
//                      break;
//                  case 3:   // C++ algorithm.min()
//                      break;
//                  case 4:   // fmin()
//                      break;
//                  case 8:   // IEEE MinNum of absolute values
//                      break;
//                  case 9:   // Java Math.Min() of absolute values
//                      break;
//                  case 10:  // C-Style Min Macro of absolute values
//                      break;
//                  case 11:  // C++ algorithm.min() of absolute values
//                      break;
//                  case 12:  // fmin() of absolute values
//                      break;
//                  }
//
//
//                  newcc = FLOAT64_COMPARE( op2, op3 );
//                  if (newcc == 3)
//                  {
//                      softfloat_exceptionFlags = softfloat_flag_invalid;
//                  }
//                  /* Xi is only trap that suppresses result, no return */
//                  VECTOR_IEEE_EXCEPTION_TRAP_XI( i, regs );
//
//                  if (newcc == 3)
//                  {
//                      continue;
//                  }
//                  if (newcc == 2)  // op3 is Minimum
//                  {
//                      VECTOR_PUT_FLOAT64_NOCC( op3, v1, i, regs );
//                  }
//                  else             // op2 is Minimum, or op2 and op3 are equal
//                  {
//                      VECTOR_PUT_FLOAT64_NOCC( op2, v1, i, regs );
//                  }
//
//
//              }
//          }
//      }
//      else if ( m4 == 2 )  // Short format
//      {
//          float32_t   op2, op3;
//
//          for (i=0; i < 4; i++)
//          {
//              if (i == 0 || !M5_SE)
//              {
//                  VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
//                  VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
//
//                  /* Todo: FixMe! Write some code that implements this instruction! */
//
//              }
//          }
//      }
//      else if ( m4 == 4 )  // Extended format
//      {
//          float128_t  op2, op3;
//
//          VECTOR_GET_FLOAT128_OP( op3, v3, regs );
//          VECTOR_GET_FLOAT128_OP( op2, v2, regs );
//
//          /* Todo: FixMe! Write some code that implements this instruction! */
//
//      }
//
//  #undef M5_SE
//  #undef M5_RE

    ZVECTOR_END( regs );
}

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
//      int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    /* Todo: FixMe! Investigate how to implement this instruction! */
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );


    /* Temporary! */
    UNREFERENCED( v1 );
    UNREFERENCED( v2 );
    UNREFERENCED( v3 );
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );


//  #define M5_SE ((m5 & 0x8) != 0) // Single-Element-Control (S)
//  #define M5_RE ((m5 & 0x7) != 0) // Reserved
//
//      if ( FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
//      {
//          if ( (m6 >= 5 && m6 <= 7) || m6 > 12 || M5_RE || m4 < 2 || m4 > 4 )
//              ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
//      }
//      else
//          ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
//
//      if ( m4 == 3 )  // Long format
//      {
//          float64_t   op2, op3;
//
//          for (i=0; i < 2; i++)
//          {
//              if (i == 0 || !M5_SE)
//              {
//                  VECTOR_GET_FLOAT64_OP( op3, v3, i, regs );
//                  VECTOR_GET_FLOAT64_OP( op2, v2, i, regs );
//
//                  /* Todo: FixMe! Write some code that implements this instruction! */
//
//
//                  switch (m6)
//                  {
//                  case 0:   // IEEE MaxNum
//                      break;
//                  case 1:   // Java Math.Max()
//                      break;
//                  case 2:   // C-Style Max Macro
//                      break;
//                  case 3:   // C++ Algorithm.max()
//                      break;
//                  case 4:   // fmax()
//                      break;
//                  case 8:   // IEEE MaxNumMag
//                      break;
//                  case 9:   // Java Math.Max() of absolute values
//                      break;
//                  case 10:  // C-Style Max Macro of absolute values
//                      break;
//                  case 11:  // C++ Algorithm.max() of absolute values
//                      break;
//                  case 12:  // fmax() of absolute values
//                      break;
//                  }
//
//
//              }
//          }
//      }
//      else if ( m4 == 2 )  // Short format
//      {
//          float32_t   op2, op3;
//
//          for (i=0; i < 4; i++)
//          {
//              if (i == 0 || !M5_SE)
//              {
//                  VECTOR_GET_FLOAT32_OP( op3, v3, i, regs );
//                  VECTOR_GET_FLOAT32_OP( op2, v2, i, regs );
//
//                  /* Todo: FixMe! Write some code that implements this instruction! */
//
//              }
//          }
//      }
//      else if ( m4 == 4 )  // Extended format
//      {
//          float128_t  op2, op3;
//
//          VECTOR_GET_FLOAT128_OP( op3, v3, regs );
//          VECTOR_GET_FLOAT128_OP( op2, v2, regs );
//
//          /* Todo: FixMe! Write some code that implements this instruction! */
//
//      }
//
//  #undef M5_SE
//  #undef M5_RE

    ZVECTOR_END( regs );
}

#endif /* defined( FEATURE_129_ZVECTOR_FACILITY ) */

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
    #include "ieee.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "ieee.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

// Place all non-arch-dep functions here...


//   We either have none (unlikely), or else they are all defined
//   further above controlled by the "_IEEE_NONARCHDEP_" #define.


#endif  /*!defined(_GEN_ARCH) */

/* end of ieee.c */
