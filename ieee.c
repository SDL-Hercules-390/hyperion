/* IEEE.C       (C) Copyright Roger Bowler and others, 2003-2012     */
/*              (C) Copyright Willem Konynenberg, 2001-2003          */
/*              (C) Copyright "Fish" (David B. Trout), 2011          */
/*              (C) Copyright Stephen R. Orso, 2016                  */
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

static void ieee_trap( REGS *regs, BYTE dxc)
{
    regs->dxc = dxc;                   /*  Save DXC in PSA         */
    regs->fpc &= ~FPC_DXC;             /*  Clear any previous DXC  */
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

/* Save detected exceptions that are trap-enabled                   */

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
#define SET_SF_RM_FROM_MASK( _mask )                                    \
                                                                        \
    softfloat_roundingMode = _mask ?                                    \
        map_m3_to_sf_rm[_mask]                                          \
      : map_fpc_brm_to_sf_rm[ (regs->fpc & FPC_BRM_3BIT) ]


/* fastpath test for Xi trap; many instructions only return Xi */
#define IEEE_EXCEPTION_TRAP_XI( _regs )                                 \
                                                                        \
    if (1                                                               \
        && (softfloat_exceptionFlags & softfloat_flag_invalid)          \
        && (_regs->fpc & FPC_MASK_IMI)                                  \
    )                                                                   \
        ieee_trap( _regs, DXC_IEEE_INVALID_OP )


/* fastpath test for Xz trap; only Divide returns Xz  */
#define IEEE_EXCEPTION_TRAP_XZ( _regs )                                 \
                                                                        \
    if (1                                                               \
        && (softfloat_exceptionFlags & softfloat_flag_infinite)         \
        && (_regs->fpc & FPC_MASK_IMZ)                                  \
    )                                                                   \
        ieee_trap( _regs, DXC_IEEE_DIV_ZERO )


/* trap if any provided exception has been previously detected */
#define IEEE_EXCEPTION_TRAP( _regs, _ieee_trap_conds, _exceptions )     \
                                                                        \
    if (_ieee_trap_conds & (_exceptions))                               \
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
#endif
    {
        if (mask > 7 || !map_valid_m3_values_NOFPX[ (mask & 0x7) ])
            regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }
}

#undef  BFPRM_CHECK
#define BFPRM_CHECK( _mask, _regs )   ARCH_DEP( BFP_RM_check )((_regs),(_mask))

static INLINE void ARCH_DEP( get_float128 )( float128_t *op, U32 *fpr )
{
                                                      /* high order bits in v[1], low order in v[0]  */
    op->v[FLOAT128_HI] = ((U64)fpr[0]     << 32) | fpr[1];               /* *****  Possible endian concern  ******* */
    op->v[FLOAT128_LO] = ((U64)fpr[FPREX] << 32) | fpr[FPREX+1];
}

static INLINE void ARCH_DEP( put_float128 )( float128_t *op, U32 *fpr )
{
    fpr[0]       = (U32) (op->v[FLOAT128_HI] >> 32);
    fpr[1]       = (U32) (op->v[FLOAT128_HI] & 0xFFFFFFFF);
    fpr[FPREX]   = (U32) (op->v[FLOAT128_LO] >> 32);
    fpr[FPREX+1] = (U32) (op->v[FLOAT128_LO] & 0xFFFFFFFF);
}

static INLINE void ARCH_DEP( get_float64 )( float64_t *op, U32 *fpr )
{
    op->v = ((U64)fpr[0] << 32) | fpr[1];
}

static INLINE void ARCH_DEP( put_float64 )( float64_t *op, U32 *fpr )
{
    fpr[0] = (U32) (op->v >> 32);
    fpr[1] = (U32) (op->v & 0xFFFFFFFF);
}

static INLINE void ARCH_DEP( get_float32 )( float32_t *op, U32 *fpr )
{
    op->v = *fpr;
}

static INLINE void ARCH_DEP( put_float32 )( float32_t *op, U32 *fpr )
{
    *fpr = op->v;
}

#undef VFETCH_FLOAT64_OP
#undef VFETCH_FLOAT32_OP

#define VFETCH_FLOAT64_OP( op, effective_addr, arn, regs )  op.v = ARCH_DEP( vfetch8 )( effective_addr, arn, regs )
#define VFETCH_FLOAT32_OP( op, effective_addr, arn, regs )  op.v = ARCH_DEP( vfetch4 )( effective_addr, arn, regs )

#undef GET_FLOAT128_OP
#undef GET_FLOAT64_OP
#undef GET_FLOAT32_OP

#define GET_FLOAT128_OP( op, r, regs )  ARCH_DEP( get_float128 )( &op, regs->fpr + FPR2I( r ))
#define GET_FLOAT64_OP(  op, r, regs )  ARCH_DEP( get_float64  )( &op, regs->fpr + FPR2I( r ))
#define GET_FLOAT32_OP(  op, r, regs )  ARCH_DEP( get_float32  )( &op, regs->fpr + FPR2I( r ))

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

#define PUT_FLOAT128_NOCC( op1, r1, regs )  ARCH_DEP( put_float128 )( &op1, regs->fpr + FPR2I( r1 ))
#define PUT_FLOAT64_NOCC(  op1, r1, regs )  ARCH_DEP( put_float64  )( &op1, regs->fpr + FPR2I( r1 ))
#define PUT_FLOAT32_NOCC(  op1, r1, regs )  ARCH_DEP( put_float32  )( &op1, regs->fpr + FPR2I( r1 ))

#undef PUT_FLOAT128_CC
#undef PUT_FLOAT64_CC
#undef PUT_FLOAT32_CC

#define PUT_FLOAT128_CC( op1, r1, regs )                            \
                                                                    \
    do {                                                            \
        ARCH_DEP(  put_float128 )( &op1, regs->fpr + FPR2I( r1 ));  \
        regs->psw.cc = FLOAT128_CC( op1 );                          \
    } while (0)


#define PUT_FLOAT64_CC( op1, r1, regs )                             \
                                                                    \
    do {                                                            \
        ARCH_DEP(  put_float64 )( &op1, regs->fpr + FPR2I (r1 ));   \
        regs->psw.cc = FLOAT64_CC( op1 );                           \
    } while (0)


#define PUT_FLOAT32_CC( op1, r1, regs )                             \
                                                                    \
    do {                                                            \
        ARCH_DEP(  put_float32 )( &op1, regs->fpr + FPR2I( r1 ));   \
        regs->psw.cc = FLOAT32_CC( op1 );                           \
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
#endif /*!defined( HAVE_MATH_H ) */

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
static void get_lbfp( struct lbfp* op, U32* fpr )
{
    op->sign  = (fpr[0] & 0x80000000) != 0;
    op->exp   = (fpr[0] & 0x7FF00000) >> 20;
    op->fract = (((U64)fpr[0] & 0x000FFFFF) << 32) | fpr[1];

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
static void put_lbfp( struct lbfp* op, U32* fpr )
{
    fpr[0] = (op->sign ? 1 << 31 : 0) | (op->exp << 20) | (op->fract >> 32);
    fpr[1] = op->fract & 0xFFFFFFFF;

    //logmsg("lput exp=%d fract=%"PRIx64" r=%8.8x%8.8x\n", op->exp, op->fract, fpr[0], fpr[1]);
}

static void put_sbfp( struct sbfp* op, U32* fpr )
{
    fpr[0] = (op->sign ? 1 << 31 : 0) | (op->exp << 23) | op->fract;

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
static int cnvt_bfp_to_hfp( struct lbfp* op, int fpclass, U32* fpr )
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
    fpr[0] = r0;
    fpr[1] = r1;
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
    U32*  fpr,
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
    sign  =        fpr[0] >> 31;
    expo  =       (fpr[0] >> 24) & 0x007F;
    fract = ((U64)(fpr[0] & 0x00FFFFFF) << 32) | fpr[1];

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
    get_lbfp(&op2, regs->fpr + FPR2I(r2));

    /* Convert to hfp register and set condition code */
    regs->psw.cc =
        cnvt_bfp_to_hfp (&op2,
                         lbfpclassify(&op2),
                         regs->fpr + FPR2I(r1));

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
    get_sbfp(&op2, regs->fpr + FPR2I(r2));

    /* Lengthen sbfp operand to lbfp */
    lbfp_op2.sign = op2.sign;
    lbfp_op2.exp = op2.exp - 127 + 1023;
    lbfp_op2.fract = (U64)op2.fract << (52 - 23);

    /* Convert lbfp to hfp register and set condition code */
    regs->psw.cc =
        cnvt_bfp_to_hfp (&lbfp_op2,
                         sbfpclassify(&op2),
                         regs->fpr + FPR2I(r1));

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
        cnvt_hfp_to_bfp (regs->fpr + FPR2I(r2), m3,
            /*fractbits*/52, /*emax*/1023, /*ebias*/1023,
            &(op1.sign), &(op1.exp), &(op1.fract));

    put_lbfp(&op1, regs->fpr + FPR2I(r1));

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
        cnvt_hfp_to_bfp (regs->fpr + FPR2I(r2), m3,
            /*fractbits*/23, /*emax*/127, /*ebias*/127,
            &(op1.sign), &(op1.exp), &fract);
    op1.fract = (U32)fract;

    put_sbfp(&op1, regs->fpr + FPR2I(r1));

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
