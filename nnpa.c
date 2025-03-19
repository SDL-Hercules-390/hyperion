/* NNPA.C       (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*                                                                   */
/*              NEURAL NETWORK PROCESSING ASSIST and associated      */
/*              z/Arch Vector Operations                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*------------------------------------------------------------------------------------------------
 James Wekel - August 2024

 nnpa.c  implements NEURAL NETWORK PROCESSING ASSIST and
         associated z/architecture E6xx instructions:

facility  code  test#   Instruction                                             op-code    format
--------  ----  -----   ----------------------------------------------------    -------    ------

    ** Experimental Implementation **

    nn      x     20    E655 VECTOR FP CONVERT TO NNP                           VCNF        VRR-a
    nn      x     20    E656 VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH       VCLFNH      VRR-a
    nn      x     20    E65D VECTOR FP CONVERT FROM NNP                         VCFN        VRR-a
    nn      x     20    E65E VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW        VCLFNL      VRR-a
    nn      x     21    E675 VECTOR FP CONVERT AND ROUND TO NNP                 VCRNF       VRR-c

  facility (bit):
    nn  -   165 - Neural-network-processing-assist facility

  test#:
    Instruction test are named 'zvector-e6-xx-hint' where 'xx' is the test# and 'hint' provides
    a hint of the instructions tested. A test is limited to a single instruction format.
    Multiple instructions may be tested within a single test. Multiple tests may be required
    to test all instructions for a given instruction format, e.g. load type instructions
    in one test and store type instructions in a second test.

  This is experimental implementation of the Neural-network-processing-assist facility
  vector instructions until PoP explicitly defines the NNP-Data-Type-1 Format. These
  instructions are located before the NNPA instruction. The NNPA instruction implements
  the following functions:

                  Code   Code
    code  test#  (Dec)  (Hex)     Function
    ----  -----  -----  -----     ------------------
     x               0      0     NNPA-QAF
                    16     10     NNPA-ADD
                    17     11     NNPA-SUB
                    18     12     NNPA-MUL
                    19     13     NNPA-DIV
                    20     14     NNPA-MIN
                    21     15     NNPA-MAX
                    32     20     NNPA-LOG
                    33     21     NNPA-EXP
                    49     31     NNPA-RELU
                    50     32     NNPA-TANH
                    51     33     NNPA-SIGMOID
                    52     34     NNPA-SOFTMAX
                    64     40     NNPA-BATCHNORM
                    80     50     NNPA-MAXPOOL2D
                    81     51     NNPA-AVGPOOL2D
                    96     60     NNPA-LSTMACT
                    97     61     NNPA-GRUACT
                   112     70     NNPA-CONVOLUTION
                   113     71     NNPA-MATMUL-OP
                   114     72     NNPA-MATMUL-OPBCAST23

  test#:
    Instruction test are named 'nnpa-xx-hint' where 'xx' is the test# and 'hint' provides
    a hint of the function tested.

--------------------------------------------------------------------------------------------*/

#include "hstdinc.h"

#define _NNPA_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined( FEATURE_165_NNET_ASSIST_FACILITY )

/*-------------------------------------------------------------------*/
/* SoftFloat definitions are used for zvector NNP instructions       */
/*-------------------------------------------------------------------*/

/* see ieee.c */
/*****************************************************************************/
/* PROGRAMMING NOTE: the following three defines for SOFTFLOAT_FAST_INT64,   */
/* SOFTFLOAT_FAST_DIV64TO32 and LITTLEENDIAN *must* match the values that    */
/* were used to build the SoftFloat static libraries you are linking with.   */
/*****************************************************************************/
#define SOFTFLOAT_FAST_INT64        /* Hercules SoftFloat requirement        */
#define SOFTFLOAT_FAST_DIV64TO32    /* Hercules SoftFloat requirement        */
#undef  LITTLEENDIAN                /* (in case it's already #defined)       */
#if !defined( WORDS_BIGENDIAN )     /* NOT building for BIG endian platform? */
#define LITTLEENDIAN                /* Then #define LITTLEENDIAN macro       */
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


/* ====================================================================== */
/* ZVECTOR_END macro for debugging Vector instructions                    */
/* Note: block comments are used to avoid gcc                             */
/*       warning: multi-line comment [-Wcomment]                          */
/*
    #undef  ZVECTOR_END
    #define ZVECTOR_END(_regs) \
            if (0 && inst[5] != (U8) 0x3E && inst[5] != (U8) 0x36) \
                ARCH_DEP(display_inst) (_regs, inst);
*/
/* ====================================================================== */

/*===================================================================*/
/* LOCAL Registers (saved vector registers)                          */
/*     local vector registers ensure source input of a vector        */
/*     register which could also be an output vector register        */
/*     NOTE: the same vfp name to use VR_x macros                    */
/*===================================================================*/

/* local (saved) vector register */
typedef struct
    {
        QW vfp[3];
    }
    LOCAL_REGS;

#define LV1 0
#define LV2 1
#define LV3 2

#define VR_SAVE_LOCAL( _l, _r) memcpy( &( lregs->VR_Q(_l) ), &( regs->VR_Q(_r)  ), sizeof(QW) )

#define LOCALS()                               \
    LOCAL_REGS  locals;                        \
    LOCAL_REGS* lregs = &locals;

/*===================================================================*/
/* NNPA function prototypes (implemented)                            */
/*===================================================================*/
static inline void ARCH_DEP( nnpa_qaf ) ( VADR pb_addr, REGS* regs);

// static inline void ARCH_DEP( nnpa_qaf ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_add ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_sub ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_mul ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_div ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_min ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_max ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_log ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_exp ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_relu ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_tanh ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_sigmoid ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_softmax ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_batchnorm ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_maxpool2d ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_avgpool2d ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_lstmact ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_gruact ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_convolution ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_matmul-op ) ( VADR pb_addr, REGS* regs);
// static inline void ARCH_DEP( nnpa_matmul-opbcast23 ) ( VADR pb_addr, REGS* regs);

/*===================================================================*/
/* Achitecture Independent Routines                                  */
/*===================================================================*/

#if !defined(_NNPA_ARCH_INDEPENDENT_)
#define _NNPA_ARCH_INDEPENDENT_

/*================================================================================*/
/* Vector Neural Network Processing                                               */
/*================================================================================*/

/* N1FLOAT (NNP-data-type 1 aka DLFOAT) (16 bits): 1 bit sign, 6 bit exponent; 9 bit fraction   */
typedef struct { uint16_t v; } floatn1_t;
typedef U16 N1FLOAT;

#define N1FLOAT_BIAS                    -31

#define N1FLOAT_SIGN( f )               ( (bool) ( (U16) (f) >> 15) )
#define N1FLOAT_EXP( f )                ( (U8) ( (f) >> 9) & 0x3F)
#define N1FLOAT_FRAC( f )               ( (N1FLOAT) ( (f) & 0x01FF) )
#define N1FLOAT_PACK( sign, exp, frac ) ( ( (N1FLOAT) (sign) << 15 ) | ( (N1FLOAT) ( (exp) & 0x3F ) << 9 ) | (N1FLOAT) ( (frac) & 0x01FF) )

#define N1FLOAT_DEFAULT_NAN             0x7FFF
#define N1FLOAT_IS_ZERO(f)              ( ( (f) & 0x7FFF ) == 0x0000 )
#define N1FLOAT_IS_NAN_INFINITY(f)      ( ( (f) & 0x7FFF ) == 0x7FFF )

/* TINYB (16 bits): 1 bit sign, 5 bit exponent; 10 bit fraction   */
typedef U16 TINYB;
#define TINYB_BIAS                      -15

#define TINYB_SIGN( f )                 ( (bool) ( (U16) (f) >> 15) )
#define TINYB_EXP( f )                  ( (U8) ( (f) >> 10) & 0x1F)
#define TINYB_FRAC( f )                 ( (TINYB) ( (f) & 0x03FF) )
#define TINYB_PACK( sign, exp, frac )   ( ( (TINYB) (sign) << 15 ) | ( (TINYB) ( (exp) &  0x1F ) << 10 ) | (TINYB) ( (frac) & 0x03FF ) )

#define TINYB_DEFAULT_NAN               0x7E00
#define TINYB_IS_ZERO(f)                ( ( (TINYB) ( (f) & 0x7FFF ) == (TINYB) 0x0000 ) )
#define TINYB_IS_NAN( f )               ( ( ( (f) & 0x7C00 ) == 0x7C00 ) && ( (f) & 0x03FF) )
#define TINYB_IS_NAN_INFINITY(f)        ( (   (f) & 0x7C00 ) == 0x7C00 )
#define TINYB_IS_SUBNORMAL(f)           ( ( ( (f) & 0x7C00 ) == 0x0000 ) && ( (f) & 0x03FF ) != 0x0000 )
#define TINYB_IS_NORMAL(f)              ( ( ( (f) & 0x7C00 ) != 0x0000 ) && ( (f) & 0x7C00 ) != 0x7C00 )

/* SHORTB (32 bits): 1 bit sign, 8 bit exponent; 23 bit fraction   */
typedef U32 SHORTB;
#define SHORTB_BIAS                      -127

#define SHORTB_SIGN( f )                ( (bool) ((SHORTB) (f) >> 31))
#define SHORTB_EXP( f )                 ( (U16) ((f) >> 23) & 0xFF)
#define SHORTB_FRAC( f )                ( (SHORTB) ( (f) & 0x007FFFFF) )
#define SHORTB_PACK( sign, exp, frac )  ( ((SHORTB) (sign) << 31) | ( (SHORTB) ( (exp) & 0xFF ) << 23) | (SHORTB) ( (frac) & 0x007FFFFF ) )

#define SHORTB_DEFAULT_NAN               0x7FC00000
#define SHORTB_IS_ZERO(f)                ( ( (f) & 0x7FFFFFFF ) == 0x00000000 )
#define SHORTB_IS_NAN_INFINITY(f)        ( ( (f) & 0x7F800000 ) == 0x7F800000 )
#define SHORTB_IS_SUBNORMAL(f)           ( ( (f) & 0x7F800000 ) == 0x00000000 && ( (f) & 0x007FFFFF ) != 0x00000000 )
#define SHORTB_IS_NORMAL(f)              ( ( (f) & 0x7F800000 ) != 0x00000000 && ( (f) & 0x7F800000 ) != 0x7F800000 )

/*--------------------------------------------------------------------------------*/
/* Vector processing with VXC for IEEE exception and element index                */
/* also see ieee.c                                                                */
/*                                                                                */
/* Note: An IEEE-inexact exception is not recognized during vector conversion     */
/*--------------------------------------------------------------------------------*/
static inline void vector_softfloat_conditional( REGS *regs, int vix )
{
    U32 ieee_check_mask;
    BYTE vxc;

    if ( softfloat_exceptionFlags == 0)
        return;                     /* no errors: exit */

    else if ( softfloat_exceptionFlags & softfloat_flag_invalid)
    {
        vxc =  ( (vix & 0x0F) << 4) | VXC_IEEE_INVALID_OP;
        ieee_check_mask = FPC_MASK_IMI;
    }

    else if ( softfloat_exceptionFlags & softfloat_flag_overflow)
    {
        vxc =  ( (vix & 0x0F) << 4) | VXC_IEEE_OVERFLOW;
        ieee_check_mask = FPC_MASK_IMO;
    }

    else if ( softfloat_exceptionFlags & softfloat_flag_underflow)
    {
        vxc =  ( (vix & 0x0F) << 4) | VXC_IEEE_UNDERFLOW;
        ieee_check_mask = FPC_MASK_IMU;
    }

    else if ( softfloat_exceptionFlags & softfloat_flag_inexact)
    {
        vxc =  ( (vix & 0x0F) << 4) | VXC_IEEE_INEXACT;
        ieee_check_mask = FPC_MASK_IMX;
    }

    else
        return;

    // logmsg("vector_softfloat_conditional: exceptions: %x, vxc: %x\n", softfloat_exceptionFlags, vxc);

    /* set FPC flags  from softfloat exception */
    regs->fpc |=
        /* Align softfloat flags with flags in FPCR */
        (( (U32) softfloat_exceptionFlags) << FPC_FLAG_SHIFT) &
        /* ..and suppress those that could trap */
        ~(regs->fpc >> 8) & FPC_FLAGS;

    regs->dxc = vxc;                   /*  Save VXC in PSA         */
    regs->fpc &= ~FPC_DXC;             /*  Clear previous DXC/VXC  */
    regs->fpc |= ((U32)vxc << FPC_DXC_SHIFT);

    if ( (regs->fpc & ieee_check_mask) == 0 )
        return;                 /* no trap enabled */

    /* trap enabled */
    regs->program_interrupt( regs, PGM_VECTOR_PROCESSING_EXCEPTION );
}


static inline void vector_ieee_inexact( REGS *regs, int vix )
{
     softfloat_exceptionFlags = softfloat_flag_inexact;
     vector_softfloat_conditional( regs, vix );
}

static inline void vector_ieee_invalid( REGS *regs, int vix )
{
     softfloat_exceptionFlags = softfloat_flag_invalid;
     vector_softfloat_conditional( regs, vix );
}

static inline void vector_ieee_overflow( REGS *regs, int vix )
{
     softfloat_exceptionFlags = softfloat_flag_overflow;
     vector_softfloat_conditional( regs, vix );
}

static inline void vector_ieee_underflow( REGS *regs, int vix )
{
     softfloat_exceptionFlags = softfloat_flag_underflow;
     vector_softfloat_conditional( regs, vix );
}


/*-------------------------------------------------------------------*/
/* Convert N1 Float to F16 Float                                     */
/*                                                                   */
/* Input:                                                            */
/*      floatn1_t   n1f:  N1 Float to be converted                   */
/*                                                                   */
/* Output:                                                           */
/*      float16_t   converted f16 float                              */
/*                                                                   */
/* Softfloat Exceptions                                              */
/*          0, no error                                              */
/*             softfloat_flag_invalid                                */
/*             softfloat_flag_underflow                              */
/*             softfloat_flag_overflow                               */
/*-------------------------------------------------------------------*/
static inline float16_t fn1_to_f16( const floatn1_t n1f)
{
    bool        sign;
    U32         frac;
    S16         exp;
    float32_t   sbf;
    float16_t   t16;

    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_maxMag;  /* match DLFOAT rounding */

    sign = N1FLOAT_SIGN( n1f.v );
    exp  = N1FLOAT_EXP( n1f.v );
    frac = N1FLOAT_FRAC( n1f.v );

    /* case: 0 */
    if ( N1FLOAT_IS_ZERO( n1f.v ) )
    {
        t16.v = TINYB_PACK( sign, 0, 0 );
        return t16;
    }

    /* cases: nan or infinity */
    if ( N1FLOAT_IS_NAN_INFINITY( n1f.v ) )
    {
        softfloat_exceptionFlags = softfloat_flag_invalid;  /* is this nan or infinity? */
        t16.v = TINYB_DEFAULT_NAN | (sign << 15);
        return t16;
    }

    /* case: normal (DL float only has normals) */
    exp = (exp + N1FLOAT_BIAS) - SHORTB_BIAS;    /* change bias */
    frac <<= (23-9);                             /* change frac bits */
    sbf.v = SHORTB_PACK ( sign, exp, frac );

    t16 = f32_to_f16 ( sbf );
    return t16;
}

/*-------------------------------------------------------------------*/
/* Convert F16 Float to N1 Float                                     */
/*                                                                   */
/* Input:                                                            */
/*      float16_t   t16f:  f16 Float to be converted                 */
/*                                                                   */
/* Output:                                                           */
/*      floatn1_t   converted n1 float                               */
/*                                                                   */
/* Softfloat Exceptions                                              */
/*          0, no error                                              */
/*             softfloat_flag_invalid                                */
/*             softfloat_flag_underflow                              */
/*             softfloat_flag_overflow                               */
/*-------------------------------------------------------------------*/
static inline floatn1_t f16_to_fn1( float16_t tbf)
{
    bool         sign;
    U32          frac;
    S16          exp;
    float32_t    sbf;
    floatn1_t    n1f;

    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_maxMag;  /* match DLFOAT rounding */

    sign = TINYB_SIGN( tbf.v );
    exp  = TINYB_EXP( tbf.v );
    frac = TINYB_FRAC( tbf.v );

    /* case: 0 */
    if ( TINYB_IS_ZERO( tbf.v ) )
    {
        n1f.v = N1FLOAT_PACK( sign, 0, 0 );
        return n1f;
    }

    /* cases: nan or infinity -> NaN                              */
    /* N1FLOAT NAN-INFINITY doesn't care about sign; but maintain */
    if ( TINYB_IS_NAN_INFINITY( tbf.v ) )
    {
        n1f.v  = N1FLOAT_DEFAULT_NAN | (sign << 15);
        return n1f;
    }

    /* cases: normal, subnormal */
    /* convert tiny float to short float (always normal) */
    sbf = f16_to_f32 ( tbf );

    sign = SHORTB_SIGN( sbf.v );
    exp  = SHORTB_EXP(  sbf.v );
    frac = SHORTB_FRAC( sbf.v );

    exp = (exp + SHORTB_BIAS ) - N1FLOAT_BIAS;      /* change bias */
    frac >>= (23-10);                               /* change frac bits (10) */

    /* round  fraction (Dl float round up) and shorten to 9 bits: check for round overflow */
    if ((frac & 0x03FF)  == 0x03FF)
    {
        frac = 0;
        exp += 1;
    }
    else
    {
        frac += 1;
        frac >>= 1;
    }

    /* check for under/over flow */
    if (exp < 0 )
    {
        n1f.v  = N1FLOAT_PACK( sign, 0, 0 );
        softfloat_exceptionFlags = softfloat_flag_underflow;
        return n1f;
    }
    if (exp > 63 )
    {
        n1f.v = N1FLOAT_PACK( sign, 0x3f, (0x01FF - 1) );
        softfloat_exceptionFlags = softfloat_flag_overflow;
        return  n1f;
    }

    /* convert to dl float */
    n1f.v =  N1FLOAT_PACK ( sign, exp, frac );
    return n1f;
}

/*-------------------------------------------------------------------*/
/* Convert N1 Float to F32 Float                                     */
/*                                                                   */
/* Input:                                                            */
/*      floatn1_t   n1f:  n1 Float to be converted                   */
/*                                                                   */
/* Output:                                                           */
/*      float32_t   converted f32 float                              */
/*                                                                   */
/* Softfloat Exceptions                                              */
/*          0, no error                                              */
/*             softfloat_flag_invalid                                */
/*-------------------------------------------------------------------*/
static inline float32_t fn1_to_f32( const floatn1_t n1f)
{
    bool        sign;
    U32         frac;
    S16         exp;
    float32_t   sbf;

    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_maxMag;  /* match DLFOAT rounding */

    sign = N1FLOAT_SIGN( n1f.v );
    exp  = N1FLOAT_EXP( n1f.v );
    frac = N1FLOAT_FRAC( n1f.v );

    /* case: 0 */
    if ( N1FLOAT_IS_ZERO( n1f.v ) )
    {
        sbf.v = SHORTB_PACK( sign, 0, 0 );
        return sbf;
    }

    /* cases: nan or infinity */
    if ( N1FLOAT_IS_NAN_INFINITY( n1f.v ) )
    {
        sbf.v = SHORTB_DEFAULT_NAN  | (sign << 31);
        softfloat_exceptionFlags = softfloat_flag_invalid;      /* is this nan or infinity? */
        return sbf;
    }

    /* case: normal (DL float only has normals) */
    exp = (exp + N1FLOAT_BIAS) - SHORTB_BIAS;        /* change bias */
    frac <<= (23-9);                                 /* change frac bits */
    sbf.v = SHORTB_PACK ( sign, exp, frac );

    return sbf;
}

/*-------------------------------------------------------------------*/
/* Convert F32 Float to N1 Float                                     */
/*                                                                   */
/* Input:                                                            */
/*      float32_t   sbf:  f32 Float to be converted                  */
/*                                                                   */
/* Output:                                                           */
/*      floatn1_t   converted N1 float                               */
/*                                                                   */
/* Softfloat Exceptions                                              */
/*          0, no error                                              */
/*             softfloat_flag_invalid   (????)                       */
/*             softfloat_flag_underflow                              */
/*             softfloat_flag_overflow                               */
/*-------------------------------------------------------------------*/
static inline floatn1_t f32_to_fn1( const float32_t sbf)
{
    bool    sign;
    U32     frac;
    S16     exp;
    floatn1_t   n1f;

    softfloat_exceptionFlags = 0;
    softfloat_roundingMode = softfloat_round_near_maxMag;  /* match DLFOAT rounding */

    sign = SHORTB_SIGN( sbf.v );
    exp  = SHORTB_EXP( sbf.v );
    frac = SHORTB_FRAC( sbf.v );

    /* case: 0 */
    if ( SHORTB_IS_ZERO( sbf.v ) )
    {
        n1f.v = N1FLOAT_PACK( sign, 0, 0 );
        return n1f;
    }

    /* cases: nan or infinity -> NaN                              */
    /* N1FLOAT NAN-INFINITY doesn't care about sign; but maintain */
    if ( SHORTB_IS_NAN_INFINITY( sbf.v ) )
    {
        n1f.v = N1FLOAT_DEFAULT_NAN | (sign << 15);
        return n1f;
    }

    /* cases: normal */
    if ( SHORTB_IS_NORMAL( sbf.v ) )
    {
        exp = (exp + SHORTB_BIAS) - N1FLOAT_BIAS;   /* change bias */
        frac >>= (23-10);                           /* change frac bits (10) */

        /* round  fraction (Dl float round up) and shorten to 9 bits: check for round overflow */
        if ((frac & 0x03FF)  == 0x03FF)
        {
            frac = 0;
            exp += 1;
        }
        else
        {
            frac += 1;
            frac >>= 1;
        }

        /* check for under/over flow */
        if (exp < 0 )
        {
            n1f.v = N1FLOAT_PACK( sign, 0, 0 );
            softfloat_exceptionFlags = softfloat_flag_underflow;
            return n1f;
        }
        if (exp > 63 )
        {
            n1f.v = N1FLOAT_PACK( sign, 0x3f, (0x01FF - 1) );
            softfloat_exceptionFlags = softfloat_flag_overflow;
            return  n1f;
        }

        n1f.v =  N1FLOAT_PACK ( sign, exp, frac );
        return n1f;
    }

    /* cases: subnormal - always underflow */
    n1f.v = N1FLOAT_PACK( sign, 0, 0 );
    softfloat_exceptionFlags = softfloat_flag_underflow;
    return n1f;
}

#endif /*!defined(_NNPA_ARCH_INDEPENDENT_)*/



/*===================================================================*/
/* Achitecture Dependent Routines / Instructions                     */
/*===================================================================*/

/*================================================================================*/
/* Vector Neural Network Processing                                               */
/*================================================================================*/
/* EXPERIMENTAL                                                                   */
/*================================================================================*/
/* z/Architecture Principles of Operation, SA22-7832-13, page 26-1                */
/* introduces NNP-Data-Type-1 Format:                                             */
/*                                                                                */
/*      NNP-data-type-1 format represents a 16-bit signed                         */
/*      floating-point number in a proprietary format with a                      */
/*      range and precision tailored toward neural-network                        */
/*      processing. Other models may use other data formats.                      */
/*                                                                                */
/* but the format is not defined.                                                 */
/*                                                                                */
/* This experimental implementation uses DLFLOAT as identified in                 */
/* https://pdfs.semanticscholar.org/5359/1b203af986668ca6586f80d30257d3ee52d7.pdf */
/*                                                                                */
/* and in                                                                         */
/*                                                                                */
/* Github project: https://github.com/IBM/zDNN                                    */
/*   IBM Z Deep Neural Network Library (zDNN) provides an interface for           */
/*   applications making use of Neural Network Processing Assist Facility (NNPA). */
/*   A function decription for zdnn_is_nnpa_installed indicates DLFOAT16 as an    */
/*   NNP-internal data type.                                                      */
/*        Description                                                             */
/*              Interrogates the hardware to determine if the NNPA and            */
/*              NNP-internal data type (N1FLOAT16) conversion instructions        */
/*              are installed.                                                    */
/*                                                                                */
/* Notes:                                                                         */
/*  1. Softfloat: This implementation uses softfloat and should probably be part  */
/*          of ieee.c. As these are EXPERIMENTAL zvector instructions, the        */
/*          implementation remains with Neural-network-processing-assist          */
/*          instructions.                                                         */
/*  2. Rounding: N1FLOAT rounding mode is 'Round nearest up'. This rounding mode  */
/*          is used for conversion to N1FLOAT.                                    */
/*  3. Normals: N1FLOAT does not have any subnormal numbers. Softfloat short      */
/*          (f32) is used as an intermediary when converting to/from tiny (f16)   */
/*          to handle normalization.                                              */
/*  4. NAN/Infinity: N1FLOAT has a fused NAN - Infinity with 0x7fff representing  */
/*          both NAN and Infinity. The sign bit is ignored. The IEEE Invalid      */
/*          Operation is raised when converting from a dlfloat NAN-Infinity       */
/*          to a tiny or short float as the conversion is unable to determine     */
/*          whether Nan or infinity is appropriate. The conversion returns QNAN   */
/*          with the sign of the N1FLOAT.                                         */
/*================================================================================*/



/*-------------------------------------------------------------------*/
/* E655 VCNF   - VECTOR FP CONVERT TO NNP                    [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_to_nnp )
{
    int     v1, v2, m3, m4, m5;           /* instruction parts       */

    int         i;                        /* loop index              */
    floatn1_t   n1f;                      /* n1 type float           */
    float16_t   t16;                      /* f16 type float          */

    LOCALS();

    VRR_A(inst, regs, v1, v2, m3, m4, m5);

    ZVECTOR_CHECK( regs );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    /* M3= 1-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m3 > 0  )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* M4= 0, 2-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m4 == 0 || m4 >= 2  )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* save v2 */
    VR_SAVE_LOCAL( LV2, v2 );

    /* the only option is tiny-bfp -> N1FLOAT */
    for ( i=0; i< 8; i++ )
    {
        t16.v = lregs->VR_H( LV2, i);
        n1f = f16_to_fn1( t16 );

        /* PoP: An IEEE-inexact exception is not recognized during   */
        /*      the conversion itself.                               */
        softfloat_exceptionFlags &= ~(softfloat_flag_inexact);
        if (softfloat_exceptionFlags)
            vector_softfloat_conditional( regs, i );

        regs->VR_H( v1, i)  = n1f.v;
    }

    ZVECTOR_END( regs );
}


/*--------------------------------------------------------------------*/
/* E656 VCLFNH - VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH [VRR_a] */
/*--------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_lengthen_from_nnp_high )
{
    int     v1, v2, m3, m4, m5;

    int         i;                   /* loop index                    */
    float32_t   sbf;                 /* Short (f32) Boolean float     */
    floatn1_t   n1f;                 /* N1 Boolean float              */

    LOCALS();

    VRR_A(inst, regs, v1, v2, m3, m4, m5);

    ZVECTOR_CHECK( regs );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    /* M3= 0-1, 3-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m3 != 2 )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* M4= 1-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m4 >= 1  )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* save v2 */
    VR_SAVE_LOCAL( LV2, v2 );

    /* the only option is N1FLOAT -> short-bfp */
    for ( i=0; i< 4; i++ )
    {
        n1f.v = lregs->VR_H( LV2, i);
        sbf = fn1_to_f32( n1f );

        /* PoP: An IEEE-inexact exception is not recognized during   */
        /*      the conversion itself.                               */
        softfloat_exceptionFlags &= ~(softfloat_flag_inexact);
        if (softfloat_exceptionFlags)
            vector_softfloat_conditional( regs, i );

        regs->VR_F( v1, i)  = sbf.v;
    }

    ZVECTOR_END( regs );
}


/*-------------------------------------------------------------------*/
/* E65D VCFN   - VECTOR FP CONVERT FROM NNP                  [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_from_nnp )
{
    int     v1, v2, m3, m4, m5;

    int         i;                        /* loop index              */
    float16_t   t16;                      /* Tiny Boolean float      */
    floatn1_t   n16;                      /* N1 boolean float        */

    LOCALS();

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    /* M3= 0, 2-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m3 != 1 )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* M4= 1-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m4 >= 1  )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* save v2 */
    VR_SAVE_LOCAL( LV2, v2 );

    /* the only option is N1FLOAT -> tiny-bfp */
    for ( i=0; i< 8; i++ )
    {
        n16.v = lregs->VR_H( LV2, i);
        t16 =  fn1_to_f16( n16 );

        /* PoP: An IEEE-inexact exception is not recognized during   */
        /*      the conversion itself.                               */
        softfloat_exceptionFlags &= ~(softfloat_flag_inexact);
        if (softfloat_exceptionFlags)
            vector_softfloat_conditional( regs, i );

        regs->VR_H( v1, i)  = t16.v;
    }

    ZVECTOR_END( regs );
}


/*-------------------------------------------------------------------*/
/* E65E VCLFNL - VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_lengthen_from_nnp_low )
{
    int     v1, v2, m3, m4, m5;

    int         i;                    /* loop index                  */
    int         k;                    /* loop index                  */
    float32_t   sbf;                  /* Short  (f32) Boolean float  */
    floatn1_t   n1f;                  /* N1 Boolean float            */

    LOCALS();

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    /* M3= 0-1, 3-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m3 != 2 )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* M4= 1-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m4 >= 1  )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* save v2 */
    VR_SAVE_LOCAL( LV2, v2 );

    /* the only option is N1FLOAT -> short-bfp */
    for ( i=4, k=0; i< 8; i++, k++ )
    {
        n1f.v = lregs->VR_H( LV2, i);
        sbf =  fn1_to_f32( n1f );

        /* PoP: An IEEE-inexact exception is not recognized during   */
        /*      the conversion itself.                               */
        softfloat_exceptionFlags &= ~(softfloat_flag_inexact);
        if (softfloat_exceptionFlags)
            vector_softfloat_conditional( regs, i );

        regs->VR_F( v1, k)  = sbf.v;
    }

    ZVECTOR_END( regs );
}


/*-------------------------------------------------------------------*/
/* E675 VCRNF  - VECTOR FP CONVERT AND ROUND TO NNP          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_round_to_nnp )
{
    int     v1, v2, v3, m4, m5, m6;

    int         i;                    /* loop index                  */
    int         k;                    /* loop index                  */
    float32_t   sbf;                  /* Short  (f32) Boolean float  */
    floatn1_t   n1f;                  /* N1 Boolean float            */

    LOCALS();

    VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);

    ZVECTOR_CHECK( regs );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    /* M4= 1-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m4 > 0  )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* M5= 0-1, 3-15 (reserved) => an IEEE-inexact exception is recognized. */
    if ( m5 <= 1 || m5 >= 3  )
    {
          vector_ieee_inexact( regs, 0 );
          return;
    }

    /* save v2 */
    VR_SAVE_LOCAL( LV2, v2 );
    VR_SAVE_LOCAL( LV3, v3 );

    /* the only option is short-bfp -> N1FLOAT */
    /* vr 2 */
    for ( i=0, k=0; i< 4; i++, k++ )
    {
        sbf.v = lregs->VR_F( LV2, i);
        n1f =  f32_to_fn1( sbf );

        /* PoP: An IEEE-inexact exception is not recognized during   */
        /*      the conversion itself.                               */
        softfloat_exceptionFlags &= ~(softfloat_flag_inexact);
        if (softfloat_exceptionFlags)
            vector_softfloat_conditional( regs, k );

        regs->VR_H( v1, k)  = n1f.v;
    }

    /* vr 3 */
    for ( i=0, k=4; i< 4; i++, k++ )
    {
        sbf.v = lregs->VR_F( LV3, i);
        n1f =  f32_to_fn1( sbf );

        /* PoP: An IEEE-inexact exception is not recognized during   */
        /*      the conversion itself.                               */
        softfloat_exceptionFlags &= ~(softfloat_flag_inexact);
        if (softfloat_exceptionFlags)
            vector_softfloat_conditional( regs, k );

        regs->VR_H( v1, k)  = n1f.v;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* B93B NNPA   - NEURAL NETWORK PROCESSING ASSIST              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( neural_network_processing_assist )
{
    int     r1, r2;

    int     fc;                /* nnpa function code                 */
    VADR    pb_effective_addr; /* parameter block effective address  */

    RRE(inst, regs, r1, r2);
    TXF_INSTR_CHECK( regs );

    /* r1, r2 are not part of this instruction */
    UNREFERENCED( r1 );
    UNREFERENCED( r2 );

    fc = regs->GR (0 ) & 0xff;

    /* parameter block address */
    pb_effective_addr = regs->GR( 1 );
    pb_effective_addr &=  ADDRESS_MAXWRAP( regs );

    if (pb_effective_addr & 0x7 )  /* not double word => Specification excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    PER_ZEROADDR_XCHECK( regs, 1 );

    switch (fc)
    {
        case   0:                  //     NNPA-QAF
            ARCH_DEP( nnpa_qaf )( pb_effective_addr, regs );
            break;

        case  16:                  //     NNPA-ADD
        case  17:                  //     NNPA-SUB
        case  18:                  //     NNPA-MUL
        case  19:                  //     NNPA-DIV
        case  20:                  //     NNPA-MIN
        case  21:                  //     NNPA-MAX
        case  32:                  //     NNPA-LOG
        case  33:                  //     NNPA-EXP
        case  49:                  //     NNPA-RELU
        case  50:                  //     NNPA-TANH
        case  51:                  //     NNPA-SIGMOID
        case  52:                  //     NNPA-SOFTMAX
        case  64:                  //     NNPA-BATCHNORM
        case  80:                  //     NNPA-MAXPOOL2D
        case  81:                  //     NNPA-AVGPOOL2D
        case  96:                  //     NNPA-LSTMACT
        case  97:                  //     NNPA-GRUACT
        case 112:                  //     NNPA-CONVOLUTION
        case 113:                  //     NNPA-MATMUL-OP
        case 114:                  //     NNPA-MATMUL-OPBCAST23

        default:                   // not defined
            regs->GR_HHH( 0 ) = 0x0002;
            regs->psw.cc = 1;
    }
}

/*-------------------------------------------------------------------*/
/* Function Code 0: NNPA-QAF (Query Available Functions)             */
/*-------------------------------------------------------------------*/
static inline void ARCH_DEP( nnpa_qaf ) ( VADR pb_addr, REGS* regs)
{
    BYTE l_pb[256];                     /* local parameter block   */

    memset(&l_pb, 0, sizeof(l_pb) );    /* zero */

    /* functions: Bytes 0-31 */
    l_pb[0] = 0x80;                           /*  bit 0   NNPA-QAF */

    /* Installed-parameter-block formats (IPBF): Bytes 32-47       */

    /* Installed-data types: Bytes 48-49                           */
    l_pb[48] = 0x80;            /* bit 0  NNP-Data-Type 1 (16-bit) */

    /* Installed-data-layout formats: Bytes 52-55                  */

    /* Maximum-dimension-index size (MDIS): Bytes 60-63            */

    /* Maximum-tensor size (MTS): Bytes 64-71                      */

    /* Installed-NNP-Data-Type-1-conversions vector: Bytes 72-73   */
    l_pb[72] = 0x60;                /* bit 1  BFP tiny format      */
                                    /* bit 2  BFP short format     */

    ARCH_DEP(vstorec) (l_pb, sizeof(l_pb)-1,  pb_addr, 1, regs);
    regs->psw.cc = 0;
}

#endif /* defined( FEATURE_165_NNET_ASSIST_FACILITY ) */

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "nnpa.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "nnpa.c"
  #endif

#endif /*!defined(_GEN_ARCH)*/

