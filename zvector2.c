/* ZVECTOR2.C   (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              z/Arch Vector Operations                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

#include "hstdinc.h"

#define _ZVECTOR2_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined(FEATURE_129_ZVECTOR_FACILITY)


#if defined( FEATURE_134_ZVECTOR_PACK_DEC_FACILITY )
    /*-------------------------------------------------------------------*/
    /* decNumber required for Z/Vector Packed Decimal                    */
    /*-------------------------------------------------------------------*/
    #define DECNUMDIGITS 66
    #include "decNumber.h"
    #include "decPacked.h"
#endif

/* Debug end of vector instruction execution                     */
#undef  ZVECTOR_END
#define ZVECTOR_END(_regs) \
        if (0 && inst[5] != (U8) 0x3E && inst[5] != (U8) 0x36) \
            ARCH_DEP(display_inst) (_regs, inst);

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define MAX_DECIMAL_LENGTH      16
#define MAX_DECIMAL_DIGITS      (((MAX_DECIMAL_LENGTH)*2)-1)
#define MAX_ZONED_LENGTH        32
#define VR_PACKED_SIGN          15


/* Decimal handling helpers */
#define IS_VALID_SIGN(s)    ( ((s) & 0x0F) > 9 )
#define IS_VALID_DECIMAL(s) ( ((s) & 0x0F) < 10 )
#define IS_PLUS_SIGN(s)     ( (((s) & 0x0F) == 0x0A) || (((s) & 0x0F) == 0x0C) || (((s) & 0x0F) >= 0x0E) )
#define IS_MINUS_SIGN(s)    ( (((s) & 0x0F) == 0x0B) || (((s) & 0x0F) == 0x0D) )
#define PREFERRED_PLUS      0x0C
#define PREFERRED_MINUS     0x0D
#define PREFERRED_ZONE      0x0F
#define PACKED_HIGH(p)      ( ((p) & 0xF0) >> 4)
#define PACKED_LOW(p)       ( ((p) & 0x0F)     )
#define PACKED_SIGN(p)      ( ((p) & 0x0F)     )
#define ZONED_DECIMAL(p)    ( ((p) & 0x0F)     )
#define ZONED_SIGN(p)       ( ((p) & 0xF0) >> 4)


/* Vector Register handling helpers */
#define SET_VR_ZERO(p)          SetZero_128( &(regs->VR_Q( (p) )) )
#define GET_VR_SIGN(p)          (PACKED_SIGN( regs->VR_B( (p), VR_PACKED_SIGN) ))
#define SET_VR_SIGN(p, sign)    ( regs->VR_B( (p), VR_PACKED_SIGN) =  (regs->VR_B( (p), VR_PACKED_SIGN) & 0xF0) | (sign & 0x0F) )
#define VR_HAS_PLUS_SIGN(p)     (IS_PLUS_SIGN( GET_VR_SIGN(p) ) )
#define VR_HAS_MINUS_SIGN(p)    (IS_MINUS_SIGN( GET_VR_SIGN(p) ) )
#define VR_HAS_VALID_SIGN(p)    (IS_VALID_SIGN( GET_VR_SIGN(p) ) )

/* local Vector handling helpers */
#define SET_LV_ZERO(p)          SetZero_128( &(lregs->VR_Q( (p) )) )
#define GET_LV_SIGN(p)          (PACKED_SIGN( lregs->VR_B( (p), VR_PACKED_SIGN) ))
#define SET_LV_SIGN(p, sign)    ( lregs->VR_B( (p), VR_PACKED_SIGN) =  (lregs->VR_B( (p), VR_PACKED_SIGN) & 0xF0) | (sign & 0x0F) )
#define LV_HAS_PLUS_SIGN(p)     (IS_PLUS_SIGN( GET_VR_SIGN(p) ) )
#define LV_HAS_MINUS_SIGN(p)    (IS_MINUS_SIGN( GET_VR_SIGN(p) ) )
#define LV_HAS_VALID_SIGN(p)    (IS_VALID_SIGN( GET_VR_SIGN(p) ) )

/*-------------------------------------------------------------------*/
/* Use Intrinsics                                                    */
/* - for MSVC                                                        */
/* - for Clang version less than 12 or GCC version less than 8 when  */
/*   when SSE 4.2 intrinsics are available                           */
/*-------------------------------------------------------------------*/
//Programmers note:
//  intrinsics are defined for X64.

//  future option for aarch64:
//  sse2neon.h adds aarch64 Neon impelementations of X64 intrisics
//  to allow a single intrinsic implementation to be used.
//  https://github.com/DLTcollab/sse2neon
//

#undef __V128_SSE__

/* MSVC on X64: intrinsics are available and should be used for optimization */
#if   ( defined (_MSC_VER ) || defined( _MSVC_ ) ) && defined( _M_X64 )

    #define __V128_SSE__ 1

/*
   GCC/Clang on X64: intrinsics are included if SSE2 is available
.  Use intrinsics for optimization, if SSE 4.2 is available (all SSE intrinsics) and
       Clang version less than 12 or
       GCC version less than 8
   otherwise use compiler -O3 optimization. -O3 optimization includes
   "Vectorization of loops" https://gcc.gnu.org/onlinedocs/gnat_ugn/Vectorization-of-loops.html.
   which uses native SIMD instructions, for example on ARM AArch64 processors, Intel X64 processors, ...
*/

#elif defined( __x86_64__ ) &&  defined( __SSE4_2__ ) &&           \
      ( (defined( __clang_major__ ) && __clang_major__ < 12  ) ||  \
        (defined( __GNUC__ ) && __GNUC__ < 8  )                    \
      )

    #define __V128_SSE__ 1

#endif

/* compile debug message: are we using intrinsics? */
#if 0
    #if defined(__V128_SSE__)
        #pragma message("__V128_SSE__ is defined.  Using intrinsics." )
    #else
        #pragma message("No intrinsics are included for optimization; only compiler optimization")
    #endif
#endif

/*-------------------------------------------------------------------*/
/* 128 bit types                                                     */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* are the compiler 128 bit types available?                         */
/*-------------------------------------------------------------------*/
#if defined( __SIZEOF_INT128__ )
    #define _USE_128_
#endif

/*-------------------------------------------------------------------*/
/* U128                                                              */
/*-------------------------------------------------------------------*/
typedef union {
        QW   Q;
#if defined( _USE_128_ )
    unsigned __int128 u_128;
#endif
        U64  u_64[2];
        U32  u_32[4];
        U16  u_16[8];
        U8   u_8[16];

#if defined( _USE_128_ )
    __int128 s_128;
#endif
        S64  s_64[2];
        S32  s_32[4];
        S16  s_16[8];
        S8   s_8[16];

#if defined(__V128_SSE__)
        __m128i V; 			// intrinsic type vector
#endif

}  U128  ;

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
/* Achitecture Independent Routines                                  */
/*===================================================================*/

#if !defined(_ZVECTOR2_ARCH_INDEPENDENT_)
#define _ZVECTOR2_ARCH_INDEPENDENT_

/*===================================================================*/
/* U128 Arithmetic (add, sub, mul)                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* U128 Add: return a + b                                            */
/*-------------------------------------------------------------------*/
static inline U128 U128_add( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 + b.u_128;
    return temp;

#else
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  a.Q.D.H.D + b.Q.D.H.D;
    temp.Q.D.L.D =  a.Q.D.L.D + b.Q.D.L.D;
    if (temp.Q.D.L.D < b.Q.D.L.D) temp.Q.D.H.D++;
    return temp;
#endif
}
/*-------------------------------------------------------------------*/
/* U128 Subtract: return a - b                                       */
/*-------------------------------------------------------------------*/
static inline U128 U128_sub( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 - b.u_128;
    return temp;

#else
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  a.Q.D.H.D - b.Q.D.H.D;
    if (a.Q.D.L.D < b.Q.D.L.D) temp.Q.D.H.D--;
    temp.Q.D.L.D =  a.Q.D.L.D - b.Q.D.L.D;

    return temp;
#endif
}

/*-------------------------------------------------------------------*/
/* U128 * U32 Multiply: return a * b (overflow ignored)              */
/*                                                                   */
/* Very simple, standard approach to arithmetic mulpiply             */
/*                                                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline U128 U128_U32_mul( U128 a, U32 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 * b;
    return temp;

#else
    U128 r;                           /* return value                */
    U64 t;                            /* temp                        */


    /* initialize result */
    r.Q.D.H.D = 0UL;
    r.Q.D.L.D = 0UL;

    if (b == 0) return r;

    /* 1st 32 bits : LL */
    if (a.Q.F.LL.F != 0) r.Q.D.L.D = (U64) a.Q.F.LL.F * (U64) b;

    /* 2nd 32 bits : LH */
    if( a.Q.F.LH.F != 0)
    {
        t = (U64) a.Q.F.LH.F  * (U64) b  +  (U64) r.Q.F.LH.F;
        r.Q.F.LH.F = t & 0xFFFFFFFFUL;
        r.Q.F.HL.F = t >> 32;
    }

    /* 3rd 32 bits : HL */
    if( a.Q.F.HL.F != 0)
    {
        t = (U64) a.Q.F.HL.F  * (U64) b  +  (U64) r.Q.F.HL.F;
        r.Q.F.HL.F = t & 0xFFFFFFFFUL;
        r.Q.F.HH.F = t >> 32;
    }

    /* 4th 32 bits : HH */
    if( a.Q.F.HH.F != 0)
    {
        t = (U64) a.Q.F.HH.F  * (U64) b  +  (U64) r.Q.F.HH.F;
        r.Q.F.HH.F = t & 0xFFFFFFFFUL;
    }
    return r;
#endif
}

/*-------------------------------------------------------------------*/
/* Debug helper for U128                                             */
/*                                                                   */
/* Input:                                                            */
/*      msg     pointer to logmsg context string                     */
/*      u       U128 number                                          */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline void u128_logmsg(const char * msg, U128 u)
{
    logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", msg, u.Q.D.H.D, u.Q.D.L.D);
}


/*===================================================================*/
/* Utility Helpers                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* Set 16 bytes (128 bits) to zero                                   */
/*                                                                   */
/* Input:                                                            */
/*      addr    address of 16-byte asrea to zero                     */
/*                                                                   */
/* version depends on whether intrinsics are being used              */
/*-------------------------------------------------------------------*/
#if defined(__V128_SSE__)

static inline void SetZero_128 (void* addr)
{
    __m128i v = _mm_setzero_si128();
    memcpy( addr, &v, sizeof( __m128i ));
}

#else

static inline void SetZero_128 (void* addr)
{
    memset( addr, 0, sizeof(U128) );
}

#endif

/*===================================================================*/
/* decNumbers Helpers                                                */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* Debug helper for decNumbers                                       */
/*                                                                   */
/* Input:                                                            */
/*      msg     pointer to logmsg context string                     */
/*      dn      pointer to decNumber                                 */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline void dn_logmsg( const char * msg, decNumber* dn  )
{
    char string[DECNUMDIGITS+14]; // conversion buffer

    decNumberToString(dn, string);
    logmsg("%s: decNumber: digits=%d, bits=%hhx, exponent=%d, value=%s \n", msg, dn->digits, dn->bits, dn->exponent, string);
}

/*===================================================================*/
/* Local Vector Register Helpers                                     */
/*===================================================================*/
/* Programmer's note:                                                */
/*                                                                   */
/* There may be two helper routines (Local Vector and actual Vector) */
/* with similar names e.g.                                           */
/*                                                                   */
/*    lv_packed_valid_digits ( LOCAL_REGS* regs, int v1 )            */
/*    vr_packed_valid_digits ( REGS* regs, int v1 )                  */
/*                                                                   */
/* and function but with different register locations!               */
/*                                                                   */
/* The Local Vertor Register (copied VR) routines use 'lregs; as the */
/* context and LV1, LV2, or LV3 coresponding to the instruction      */
/* v1, v2, or v3.                                                    */
/*                                                                   */
/* The LOCALS() macro provides 3 local vector register saved areas   */
/* for an instruction.                                               */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* Check a signed packed decimal local vector for valid digits       */
/*                                                                   */
/* Input:                                                            */
/*      regs    context for Local vector register access             */
/*      v1      Local vector register to check                       */
/*                                                                   */
/* Returns:                                                          */
/*              true:  all 31 packed digits are valid                */
/*              false: at least one invalid digit in the VR          */
/*-------------------------------------------------------------------*/
static inline bool  lv_packed_valid_digits ( LOCAL_REGS* regs, int v1 )
{
    int     i, j;                           /* Array subscript       */
    bool    valid = true;                   /* valid result          */

    /* check for non-zero for leading0 digits */
    /* Pack digits into packed decimal work area */
    for (i=0, j=0; i < MAX_DECIMAL_DIGITS && valid; i++)
    {
        if (i & 1)
            valid = PACKED_LOW ( regs->VR_B( v1, j++ ) ) < 10;
        else
            valid = PACKED_HIGH ( regs->VR_B( v1, j ) ) < 10;
    } /* end for */

    return valid;
}

/*-------------------------------------------------------------------*/
/* Check a signed packed decimal local vector for a valid sign       */
/*                                                                   */
/* Input:                                                            */
/*      regs    context for Local vector register access             */
/*      v1      Local vector register to check                       */
/*                                                                   */
/* Returns:                                                          */
/*              true:  the VR sign is valid                          */
/*              false: the VR sign is invalid                        */
/*-------------------------------------------------------------------*/
static inline bool  lv_packed_valid_sign ( LOCAL_REGS* regs, int v1 )
{
    return PACKED_SIGN ( regs->VR_B( v1, VR_PACKED_SIGN ) ) > 9;
}

/*-------------------------------------------------------------------*/
/* Check a local Vector for a valid signed packed decimal            */
/*                                                                   */
/* Input:                                                            */
/*      regs    context for Local vector register access             */
/*      v1      Local vector register to check                       */
/*                                                                   */
/* Returns:                                                          */
/*              true:  the VR is a valid signed packed decimal       */
/*              false: the VR is invalid (sign or at least on digit) */
/*-------------------------------------------------------------------*/
static inline bool  lv_packed_valid ( LOCAL_REGS* regs, int v1 )
{
    return lv_packed_valid_digits( regs, v1 ) &&
           lv_packed_valid_sign  ( regs, v1 );
}

/*-------------------------------------------------------------------*/
/* Is a packed decimal Local vector register zero                    */
/*                                                                   */
/* Input:                                                            */
/*      regs    context for Local vector register access             */
/*      v1      Local vector register to check                       */
/* Returns:                                                          */
/*      true    all vr decimal packed digits are zero                */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline bool lv_is_zero( LOCAL_REGS* regs, int v1 )
{
    int     i;                 /* loop index                         */

    /* first 30 digits, two at a time */
    for ( i = 0; i < VR_PACKED_SIGN -1; i ++)
        if ( regs->VR_B( v1, i)  != 0 ) return false;

    /* 31st digit */
    if ( ( regs->VR_B( v1,  VR_PACKED_SIGN ) & 0xF0) != 0 ) return false;

    return true;
}

/*-------------------------------------------------------------------*/
/* leading zeros of a packed decimal Local vector register           */
/*                                                                   */
/* Input:                                                            */
/*      regs    context for Local vector register access             */
/*      v1      Local vector register to check                       */
/* Returns:                                                          */
/*              # of leading zeros                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline int lv_leading_zero( LOCAL_REGS* regs, int v1 )
{
    int     i;                 /* loop index                         */
    int     packedix;          /* packed index                       */
    int     count = 0;         /* leading zero count                 */

    /* copy 'count' digits */
    packedix = 0;
    for ( i = 0; i < MAX_DECIMAL_DIGITS; i++ )
    {
        if (i & 1)
        {
            if ( ( regs->VR_B( v1, packedix++) & 0xF0 ) != 0 ) return count;
        }
        else
        {
                if ( ( regs->VR_B( v1, packedix) & 0x0F ) != 0 ) return count;
        }

        count++;
    }

    return count;
}

/*-------------------------------------------------------------------*/
/* Copy count packed digits from a Local vector register             */
/*      to a vector register                                         */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register - copied to                          */
/*      lregs    context for Local vector register access             */
/*      v2      vector register - copied from                        */
/*      count   number of digits to copy                             */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline void lv_copy_to_vr(REGS* regs, int v1, LOCAL_REGS* lregs, int v2, int count)
{
    int     i;                 /* loop index                         */
    int     packedix;          /* packed index                       */

    SET_VR_ZERO( v1 );

    /* copy Sign */
    SET_VR_SIGN( v1, GET_LV_SIGN( v2 ) );

    /* copy 'count' digits */
    packedix = VR_PACKED_SIGN;
    for ( i = MAX_DECIMAL_DIGITS; count > 0 && i >= 0; i--, count-- )
    {
        if (i & 1)
        {
            regs->VR_B( v1, packedix) |= lregs->VR_B( v2, packedix) & 0xF0;
            packedix--;
        }
        else
            regs->VR_B( v1, packedix)   |= lregs->VR_B( v2, packedix) & 0x0F;
    }
}

/*===================================================================*/
/* Vector Register Helpers                                           */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* Check a signed packed decimal VR for valid digits                 */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*                                                                   */
/* Returns:                                                          */
/*              true:  all 31 packed digits are valid                */
/*              false: at least one invalid digit in the VR          */
/*-------------------------------------------------------------------*/
static inline bool  vr_packed_valid_digits ( REGS* regs, int v1 )
{
    int     i, j;                           /* Array subscript           */
    bool    valid = true;                   /* valid result              */

    /* check for non-zero for leading0 digits */
    /* Pack digits into packed decimal work area */
    for (i=0, j=0; i < MAX_DECIMAL_DIGITS && valid; i++)
    {
        if (i & 1)
            valid = PACKED_LOW ( regs->VR_B( v1, j++ ) ) < 10;
        else
            valid = PACKED_HIGH ( regs->VR_B( v1, j ) ) < 10;
    } /* end for */

    return valid;
}

/*-------------------------------------------------------------------*/
/* Check a signed packed decimal VR for a valid signb                */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*                                                                   */
/* Returns:                                                          */
/*              true:  the VR sign is valid                          */
/*              false: the VR sign is invalid                        */
/*-------------------------------------------------------------------*/
static inline bool  vr_packed_valid_sign ( REGS* regs, int v1 )
{
    return PACKED_SIGN ( regs->VR_B( v1, VR_PACKED_SIGN ) ) > 9;
}

/*-------------------------------------------------------------------*/
/* Check a VR for a valid signed packed decimal                      */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*                                                                   */
/* Returns:                                                          */
/*              true:  the VR is a valid signed packed decimal       */
/*              false: the VR is invalid (sign or at least on digit) */
/*-------------------------------------------------------------------*/
static inline bool  vr_packed_valid ( REGS* regs, int v1 )
{
    return vr_packed_valid_digits( regs, v1 ) &&
           vr_packed_valid_sign  ( regs, v1 );
}

/*-------------------------------------------------------------------*/
/* Load a valid packed decimal from a vector register to a U128      */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*      un      pointer to U128 field                                */
/*      forcePositive   boolean to inicate wheter the value should   */
/*              forced to a positive value                           */
/*                                                                   */
/*-------------------------------------------------------------------*/

static inline U128 vr_to_U128( REGS* regs, int v1, bool forcePositive )
{
    int     i;                   /* Loop variable                    */
    int     packedix;            /* packed byte index                */

    U8      digit;               /* digit of packed byte             */
    U128    result;              /* converted binary                 */
    U128    scale;               /* current digit scale              */
    U128    temp128;             /* temp U128                        */
    U128    zero128;             /* zero U128                        */

    packedix = VR_PACKED_SIGN;

    scale.Q.D.H.D  = 0;     scale.Q.D.L.D  = 1;
    result.Q.D.H.D = 0;     result.Q.D.L.D = 0;

    for ( i=MAX_DECIMAL_DIGITS-1; packedix >= 0 ; i--)
    {
        if (i & 1)
            digit = PACKED_LOW ( regs->VR_B( v1, packedix ) ) ;
        else
            digit = PACKED_HIGH ( regs->VR_B( v1, packedix-- ) ) ;

        /* increment curent digit and adjust scale */
        /*      result += scale * digit;           */
        /*      scale  *= 10;                      */

        if (digit != 0)
        {
            temp128 = U128_U32_mul (scale, digit);
            result  = U128_add ( result, temp128 );
        }
        scale = U128_U32_mul (scale, 10);

        // debug
        // logmsg("vr_to_u128: i=%d, digit=%d \n", i, digit);
        // u128_logmsg("...temp128", temp128);
        // u128_logmsg("...scale  ", scale);
        // u128_logmsg("...result ", result);
    }

    /* temp128 is positive */
    if (!forcePositive && IS_MINUS_SIGN( GET_VR_SIGN( v1 ) ) )
    {
        /* negate **/
        SetZero_128( &zero128 );
        result = U128_sub( zero128, result);
    }

    // u128_logmsg("vr_to_U128 return...", result);
    return result;
}

/*-------------------------------------------------------------------*/
/* Copy count packed digits from a vector register                   */
/*      to a vector register                                         */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register - copied to                          */
/*      v2      vector register - copied from                        */
/*      count   number of digits to copy                             */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline void vr_copy_to_vr(REGS* regs, int v1, int v2, int count)
{
    int     i;                 /* loop index                         */
    int     packedix;          /* packed index                       */

    SET_VR_ZERO( v1 );

    /* copy Sign */
    SET_VR_SIGN( v1, GET_VR_SIGN( v2) );

    /* copy 'count' digits */
    packedix = VR_PACKED_SIGN;
    for ( i = MAX_DECIMAL_DIGITS; count > 0 && i >= 0; i--, count-- )
    {
        if (i & 1)
        {
            regs->VR_B( v1, packedix) |= regs->VR_B( v2, packedix) & 0xF0;
            packedix--;
        }
        else
            regs->VR_B( v1, packedix)   |= regs->VR_B( v2, packedix) & 0x0F;
    }
}

/*-------------------------------------------------------------------*/
/* Is a packed decimal vector register zero                          */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register - copied to                          */
/* Returns:                                                          */
/*      true    all vr decimal packed digits are zero                */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline bool vr_is_zero(REGS* regs, int v1)
{
    int     i;                 /* loop index                         */

    /* first 30 digits, two at a time */
    for ( i = 0; i < VR_PACKED_SIGN -1; i ++)
        if ( regs->VR_B( v1, i)  != 0 ) return false;

    /* 31st digit */
    if ( ( regs->VR_B( v1,  VR_PACKED_SIGN ) & 0xF0) != 0 ) return false;

    return true;
}

/*-------------------------------------------------------------------*/
/* leading zeros of a packed decimal vector register                 */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register - copied to                          */
/* Returns:                                                          */
/*              # of leading zeros                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline int vr_leading_zero(REGS* regs, int v1)
{
    int     i;                 /* loop index                         */
    int     packedix;          /* packed index                       */
    int     count = 0;         /* leading zero count                 */

    /* copy 'count' digits */
    packedix = 0;
    for ( i = 0; i < MAX_DECIMAL_DIGITS; i++ )
    {
        if (i & 1)
        {
            if ( ( regs->VR_B( v1, packedix++) & 0xF0 ) != 0 ) return count;
        }
        else
        {
                if ( ( regs->VR_B( v1, packedix) & 0x0F ) != 0 ) return count;
        }

        count++;
    }

    return count;
}

/*-------------------------------------------------------------------*/
/* Load a valid packed decimal from a vector register to a decNumber */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*      dn      pointer to decNumber to save vector packed value     */
/*      forcePositive   boolean to inicate wheter the value should   */
/*              forced to a positive value                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline void vr_to_decNumber( REGS* regs, int v1, decNumber* pdn, bool forcePositive )
{
    QW      vr_bigEndian;       /* vr as big endian                  */
    int     scale = 0;          /* always 0 for zn's                 */

    /* decPacked assumes big-endian packed decimal */
    vr_bigEndian = CSWAP128( regs->VR_Q( v1 ) );
    decPackedToNumber(  (uint8_t *) &vr_bigEndian, sizeof( QW ), &scale, pdn );

    if (forcePositive && decNumberIsNegative( pdn ) )
       pdn->bits &= ~( DECNEG );                   /* efficieny hack */
}

/*-------------------------------------------------------------------*/
/* Load a vector register froma decNumber                            */
/*                                                                   */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*      dn      pointer to decNumber to save vector packed value     */
/*      forcePositive   boolean to inicate wheter the value should   */
/*              forced to a positive value                           */
/*      rdc     result digit count: the number of rightmost digits   */
/*              to load                                              */
/*                                                                   */
/* Returns:                                                          */
/*              true:  if an overflow was recognized                 */
/*                     - decNumber overflowed                        */
/*                     - number of digits is greater than result     */
/*                       digit count                                 */
/*              false: no overflow                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline bool vr_from_decNumber( REGS* regs, int v1, decNumber* pdn, bool forcePositive, int rdc )
{
    int     overflow = false;          /* overflow recognized        */
    U8      bcd_zn[DECNUMDIGITS];      /* decimal digits             */
    int     toCopy;                    /* number of digits to pack   */
    int     i, j;                      /* indexes                    */
    BYTE    ps;                        /* packed sign                */
    int     startZn;                   /* Zn index                   */
    int     startVr;                   /* vector index               */

    /* rdc safety check */
    if ( rdc <= 0 ) rdc = MAX_DECIMAL_DIGITS;

    // dn_logmsg("vr_from_decNumber:", pdn);

    /* get binary code decimal of number */
    decNumberGetBCD(pdn, bcd_zn);

    /* set vector to zero */
    SET_VR_ZERO( v1 );

    /* determine decimals to copy and where to pack them */
    toCopy = (pdn->digits <= rdc) ? pdn->digits : rdc;
    startZn = pdn->digits - toCopy;
    startVr = (MAX_DECIMAL_DIGITS - toCopy) / 2;

    // logmsg("packing: startVr=%d, startZn=%d, toCopy=%d, rdc=%d, digits=%d\n",startVr, startZn, toCopy, rdc, pdn->digits);

    /* Pack digits into vector register */
    for (i=startZn, j=startVr; i < startZn + toCopy; i++)
    {
        if ( (MAX_DECIMAL_DIGITS - toCopy + i) & 1)
            regs->VR_B( v1, j++) |= bcd_zn[i];
        else
            regs->VR_B( v1, j) |= bcd_zn[i] << 4;
    }

    /* Pack the sign into low-order digit */
    ps = ( forcePositive ) ? PREFERRED_ZONE : ( ( decNumberIsNegative( pdn ) ) ? PREFERRED_MINUS : PREFERRED_PLUS);
    regs->VR_B( v1, VR_PACKED_SIGN) |= ps;

    /* overflowed? */
    if (    pdn->digits > rdc ||
            pdn->digits > MAX_DECIMAL_DIGITS  ||
            pdn->exponent != 0 )
        overflow = true;

    return overflow;
}

/*-------------------------------------------------------------------*/
/* Set a decContext to the default for Z/vector numbers (fixed)      */
/*       - no traps                                                  */
/*       - 66 digits (note: could be 62 (31 digits * 31 digits)      */
/*                          but want to ensure that exponent is      */
/*                          always zero                              */
/*                                                                   */
/* Input:                                                            */
/*      set     pointer to decContext to set defaults                */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline void zn_ContextDefault( decContext* set )
{
    decContextDefault( set, DEC_INIT_BASE);  // initialize
    set->traps = 0;                          // no traps, thank you
    set->digits= DECNUMDIGITS;
}


#endif /*!defined(_ZVECTOR2_ARCH_INDEPENDENT_)*/


/*===================================================================*/
/* Achitecture Dependent Routines / Instructions                     */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* E601 VLEBRH   - VECTOR LOAD BYTE REVERSED ELEMENT (16)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_element_16 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK (regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 7)         /* M3 > 7 => Specficitcation excp */
        ARCH_DEP(program_interrupt) ( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_H( v1, m3 ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2, b2, regs ) );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E602 VLEBRG   - VECTOR LOAD BYTE REVERSED ELEMENT (64)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_element_64 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 1)                    /* M3 > 1 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_D( v1, m3 ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ) );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E603 VLEBRF   - VECTOR LOAD BYTE REVERSED ELEMENT (32)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_element_32 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 3)                    /* M3 > 3 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_F( v1, m3 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E604 VLLEBRZ - VECTOR LOAD BYTE REVERSED ELEMENT AND ZERO   [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_and_zero )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* M3= 0, 4, 5, 7-15 => Specficitcation excp */
    if (m3 == 0 || m3 == 4 || m3 == 5 || m3 >=7 )
        ARCH_DEP(program_interrupt) ( regs, PGM_SPECIFICATION_EXCEPTION );

    SET_VR_ZERO( v1 );

    switch (m3)
    {
    case 1: regs->VR_H( v1, 3 ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2, b2, regs ) ); break;
    case 2: regs->VR_F( v1, 1 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) ); break;
    case 3: regs->VR_D( v1, 0 ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ) ); break;
    case 6: regs->VR_F( v1, 0 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) ); break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*----------------------------------------------------------------------*/
/* E605 VLBRREP - VECTOR LOAD BYTE REVERSED ELEMENT AND REPLICATE [VRX] */
/*----------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_and_replicate )
{
    int     v1, m3, x2, b2;                /* Instruction parts         */
    VADR    effective_addr2;               /* Effective address         */
    int i;                                 /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:
        regs->VR_H( v1, 0 ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2, b2, regs ) );
        for (i=1; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_H( v1, 0 );
        break;

    case 2:
        regs->VR_F( v1, 0 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) );
        for (i=1; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v1, 0 );
        break;

    case 3:
        regs->VR_D( v1, 0 ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ) );
        regs->VR_D( v1, 1 ) = regs->VR_D( v1, 0 );
        break;

    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E606 VLBR  - VECTOR LOAD BYTE REVERSED ELEMENTS             [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_elements )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2 + i*2, b2, regs ) );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2 + i *4, b2, regs ) );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2 + i*8, b2, regs ) );
        break;

    case 4:     /* quadword */
        regs->VR_Q( v1 ) = bswap_128( ARCH_DEP( vfetch16 )( effective_addr2, b2, regs ) );
        break;

    default:    /* M3= 0, 5-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E607 VLER   - VECTOR LOAD ELEMENTS REVERSED                 [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_elements_reversed )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* M3= 0, 5-15 => Specficitcation excp */
    if (m3 == 0 || m3 >=5 )
        ARCH_DEP(program_interrupt) ( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            regs->VR_H( v1, (7 - i) ) = ARCH_DEP( vfetch2 )( effective_addr2 + i*2, b2, regs );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            regs->VR_F( v1, (3 - i) ) = ARCH_DEP( vfetch4 )( effective_addr2 + i *4, b2, regs );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            regs->VR_D( v1, (1 - i) ) = ARCH_DEP( vfetch8 )( effective_addr2 + i*8, b2, regs );
        break;

    default:    /* M3= 0, 4-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E609 VSTEBRH - VECTOR STORE BYTE REVERSED ELEMENT (16)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_element_16 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX(inst, regs, v1, x2, b2, effective_addr2, m3);

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 7)                    /* M3 > 7 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore2 )( bswap_16( regs->VR_H( v1, m3 ) ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60A VSTEBRG - VECTOR STORE BYTE REVERSED ELEMENT (64)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_element_64 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 1)                    /* M3 > 1 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore8 )( bswap_64( regs->VR_D( v1, m3 ) ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60B VSTEBRF - VECTOR STORE BYTE REVERSED ELEMENT (32)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_element_32 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 3)                    /* M3 > 3 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore4 )( bswap_32( regs->VR_F( v1, m3 ) ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60E VSTBR - VECTOR STORE BYTE REVERSED ELEMENTS            [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_elements )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            ARCH_DEP( vstore2 )( bswap_16( regs->VR_H( v1, i ) ), effective_addr2 + i*2, b2, regs );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            ARCH_DEP( vstore4 )( bswap_32( regs->VR_F( v1, i ) ), effective_addr2 + i*4, b2, regs );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            ARCH_DEP( vstore8 )( bswap_64( regs->VR_D( v1, i ) ), effective_addr2 + i*8, b2, regs );
        break;

    case 4:     /* quadword */
        ARCH_DEP( vstore16 )( bswap_128( regs->VR_Q( v1 ) ), effective_addr2, b2, regs );
        break;

    default:    /* M3= 0, 5-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60F VSTER  - VECTOR STORE ELEMENTS REVERSED                [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_reversed_elements )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            ARCH_DEP( vstore2 )( regs->VR_H( v1, i ) , effective_addr2 + (14 - i*2), b2, regs );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            ARCH_DEP( vstore4 )( regs->VR_F( v1, i ) , effective_addr2 + (12 - i*4), b2, regs );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            ARCH_DEP( vstore8 )( regs->VR_D( v1, i ), effective_addr2 + (8 - i*8), b2, regs );
        break;

    default:    /* M3= 0, 4-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E634 VPKZ  - VECTOR PACK ZONED                              [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_packed_zoned )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      zoned[MAX_ZONED_LENGTH];   /* local zoned decimal        */
    U8      digit_high;                /* high digit of packed byte  */
    U8      digit_low;                 /* low digit of packed byte   */
    int     packedix = 0;              /* current packed byte index  */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

                                      /* i3 reserved bits 0-2 must be zero    */
    if ( ( i3 & 0xE0 ) != 0 )         /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x1F;            /* Operand 2 Length Code (L2): Bits 3-7 */
    if ( l2 > 30 )             /* L2 > 30 => Specficitcation excp      */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* get local copy; note: l2 is zoned length -1 */
    ARCH_DEP( vfetchc )( zoned, l2,  effective_addr2, b2, regs );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    /* handle last zoned field */
    digit_low   = ZONED_SIGN(  zoned[ l2 ] );     /* sign */
    digit_high  = ZONED_DECIMAL( zoned[ l2 ] );   /* lowest digit */
    regs->VR_B( v1, 15 ) = ( digit_high << 4 ) | digit_low;
    //LOGMSG("VECTOR PACK ZONED: V1.B15=%hhX \n", regs->VR_B( v1, 15));

    packedix = 14;
    for ( i=l2 -1; i >=0;  i -= 2,  packedix--)
    {
        digit_low  = ZONED_DECIMAL( zoned[ i ] );
        digit_high = (i > 0 ) ? ZONED_DECIMAL( zoned[ i-1 ] ) << 4 : 0;  /* have two digits? */
        regs->VR_B( v1, packedix) = digit_high | digit_low;
        //LOGMSG("VECTOR PACK ZONED:  %d low:'%hhX' high: '%hhX' result: V%d.B%d=%hhX \n", i, zoned [i], zoned[i-1], v1, packedix, regs->VR_B( v1, packedix));
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E635 VLRL   - VECTOR LOAD RIGHTMOST WITH LENGTH             [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_rightmost_with_length )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    l2 = i3 & 0xF0;           /* i3 reserved bits 0-3 must be zero    */
    if ( l2 != 0 )            /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x0F;          /* Operand 2 Length Code (L2): Bits 4-7 */

    /* get local copy; note: l2 is length -1 */
    ARCH_DEP( vfetchc )( stor, l2,  effective_addr2, b2, regs );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    for ( i=0; i <= l2; i++ )
    {
        regs->VR_B( v1, (15 - l2) + i) = stor[i];
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E637 VLRLR  - VECTOR LOAD RIGHTMOST WITH LENGTH (reg)     [VRS-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_rightmost_with_length_reg )
{
    int     v1, b2, r3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */
    U32     reg32;                     /* r3: bits 32-63             */

    VRS_D( inst, regs, r3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    reg32 =  regs->GR_L( r3 );
    l2 = ( reg32 > 15 ) ? 15 : (int) reg32 ;
    //LOGMSG("VECTOR LOAD RIGHTMOST WITH LENGTH (reg) : r3=%d, reg32=%d, l2=%d \n", r3, reg32, l2);

    /* get local copy; note: l2 is length -1 */
    ARCH_DEP( vfetchc )( stor, l2, effective_addr2, b2, regs );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    for ( i=0; i <= l2; i++ )
    {
         regs->VR_B( v1, (15 - l2) + i) = stor[i];
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E63C VUPKZ  - VECTOR UNPACK ZONED                           [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_zoned )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      zoned[MAX_ZONED_LENGTH];   /* local zoned decimal        */
    U8      sign;                      /* sign of zoned decimal      */
    U8      digit_low;                 /* low digit of packed byte   */
    int     packedix = 0;              /* current packed byte index  */
    U8      zone = 0xF0;               /* zone field '1111'          */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    l2 = i3 & 0xE0;           /* i3 reserved bits 0-2 must be zero    */
    if ( l2 != 0 )             /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x1F;          /* Operand 2 Length Code (L2): Bits 3-7 */
    if ( l2 > 30 )             /* L2 > 30 => Specficitcation excp      */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* handle last zoned field - sign  & digit */
    sign       = PACKED_SIGN( regs->VR_B( v1, VR_PACKED_SIGN) );    /* sign */
    digit_low  = PACKED_HIGH( regs->VR_B( v1, VR_PACKED_SIGN) );    /* lowest digit */
    zoned[l2]  = ( sign << 4 ) | digit_low;
    //LOGMSG("VECTOR UNPACK ZONED: V1.B15=%hhX zoned:=%hhX \n", regs->VR_B( v1, 15), zoned[l2]);

    packedix = 14;
    for ( i=l2 -1; i >=0 ; i-=2, packedix-- )
    {
        zoned[i] = PACKED_LOW( regs->VR_B( v1, packedix) ) | zone;
        //LOGMSG("VECTOR UNPACK ZONED low:  %d '%hhX' source: V%d.B%d=%hhX \n", i, zoned [i], v1, packedix, regs->VR_B( v1, packedix));
        if( i > 0 )
        {
            zoned[i-1] = PACKED_HIGH( regs->VR_B( v1, packedix) ) | zone;
            //LOGMSG("VECTOR UNPACK ZONED high: %d '%hhX' source: V%d.B%d=%hhX \n", i, zoned [i-1], v1, packedix, regs->VR_B( v1, packedix));
        }
    }

    /* save local copy; note: l2 is zoned length -1 */
    ARCH_DEP( vstorec )( zoned, l2, effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E63D VSTRL  - VECTOR STORE RIGHTMOST WITH LENGTH            [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_rightmost_with_length )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    l2 = i3 & 0xF0;           /* i3 reserved bits 0-3 must be zero    */
    if ( l2 != 0 )            /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x0F;          /* Operand 2 Length Code (L2): Bits 4-7 */

    for ( i=0; i <= l2; i++ )
    {
        stor[i] = regs->VR_B( v1, (15 - l2) + i);
    }

    /* store local copy; note: l2 is length -1 */
    ARCH_DEP( vstorec )( stor, l2,  effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E63F VSTRLR - VECTOR STORE RIGHTMOST WITH LENGTH (reg)    [VRS-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_rightmost_with_length_reg )
{
    int     v1, b2, r3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */
    U32     reg32;                     /* r3: bits 32-63             */

    VRS_D( inst, regs, r3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    reg32 =  regs->GR_L( r3 );
    l2 = ( reg32 > 15 ) ? 15 : (int) reg32 ;
    //LOGMSG("VECTOR STORE RIGHTMOST WITH LENGTH (reg) : r3=%d, reg32=%d, l2=%d \n", r3, reg32, l2);

    for ( i=0; i <= l2; i++ )
    {
        stor[i] = regs->VR_B( v1, (15 - l2) + i);
    }

    /* store local copy; note: l2 is length -1 */
    ARCH_DEP( vstorec )( stor, l2,  effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E649 VLIP   - VECTOR LOAD IMMEDIATE DECIMAL               [VRI-h] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_immediate_decimal )
{
    int     v1, i3;              /* Instruction parts                */
    U16     i2;                  /* Instruction - Decimal Immediate  */
    int     sc;                  /* Sign Control (SC): bit 0         */
    int     shift;               /* Shift Amount (SHAMT): Bits 1-3   */
    int     i;                   /* Loop variable                    */
    int     idx;                 /* vector byte index to place digits*/
    U16     temp;                /* temporary                        */
    union imm32
    {
        U32 digits;             /* shifted digits for byte alignment */
        U8  db[4];
    } imm;                      /* immediate */

    VRI_H( inst, regs, v1, i2, i3 );

    ZVECTOR_CHECK( regs );

    /* check i2 for valid decimal digits */
    temp = i2;
    for ( i = 0; i < 3; i++)
    {
        if ( (temp & 0x0F) > 9)
        {
            regs->dxc = DXC_DECIMAL;
            ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
        }
        temp >>= 4;
    }

    /* get sign control and shift */
    sc    = ( i3 & 0x08 ) >> 3;
    shift = ( i3 & 0x07 );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    /* set sign */
    if (sc == 0)
        regs->VR_B( v1, VR_PACKED_SIGN) = PREFERRED_PLUS;   /* positive with a sign code of 1100 */
    else
        regs->VR_B( v1, VR_PACKED_SIGN) = PREFERRED_MINUS;   /* negative with a sign code of 1101 */

    //LOGMSG("VECTOR LOAD IMMEDIATE DECIMAL: i2 %X\n",i2 );

    imm.digits = i2;
    idx = VR_PACKED_SIGN - ( (shift / 2) + 2);

    if ( shift & 0x01 )         /* is shift is odd */
    {
        /* odd shift; ignore sign */
        imm.digits = (U32) CSWAP32( imm.digits );
        regs->VR_B( v1, idx + 0 ) |=  imm.db[2];
        regs->VR_B( v1, idx + 1 ) |=  imm.db[3];
        //LOGMSG("VECTOR LOAD IMMEDIATE DECIMAL: Odd shift: %d  digits %X, idx=%d, db[0]= %x, db[1]= %x, db[2]= %x, db[3]= %x\n",shift, imm.digits, idx, imm.db[0], imm.db[1], imm.db[2], imm.db[3] );
    }
    else
    {
        /* even shift; allow for sign */
        imm.digits <<= 4;
        imm.digits = (U32) CSWAP32( imm.digits );
        regs->VR_B( v1, idx + 0 ) |=  imm.db[1];
        regs->VR_B( v1, idx + 1 ) |=  imm.db[2];
        regs->VR_B( v1, idx + 2 ) |=  imm.db[3];
        //LOGMSG("VECTOR LOAD IMMEDIATE DECIMAL: Even shift: %d  digits %X, idx=%d, db[0]= %x, db[1]= %x, db[2]= %x, db[3]= %x\n",shift, imm.digits, idx, imm.db[0], imm.db[1], imm.db[2], imm.db[3] );
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E650 VCVB   - VECTOR CONVERT TO BINARY (32)               [VRR-i] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_to_binary_32 )
{
    int     r1, v2, m3, m4;      /* Instruction parts                */
    bool    p2;                  /* Force Operand 2 Positive (P2)    */
    bool    lb;                  /* Logical Binary (LB)              */
    bool    cs;                  /* Condition Code Set (CS)          */
    bool    iom;                 /* Instruction-Overflow Mask (IOM)  */
    U128    result;              /* converted binary                 */
    bool    overflow;            /* did an overfor occur             */

    bool    valid_sign2;         /* v2: is sign valid?               */
    bool    valid_decimals2;     /* v2: are decimals valid?          */

    VRR_I( inst, regs, r1, v2, m3, m4 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    p2 = (m3 & 0x08) ? true : false;
    lb = (m3 & 0x02) ? true : false;
    cs = (m3 & 0x01) ? true : false;

    /* m4 parts */
    iom = (m4 & 0x08) ? true : false;

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    if ( !valid_decimals2 || !valid_sign2 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    result = vr_to_U128( regs, v2, ( (lb) ? true : p2 ) );

    /* did overflow happen? */
    if (lb)
        overflow = ( result.Q.D.L.D > (U64) UINT_MAX ) ? true : false;
    else
    {
        if ( (p2) ? true : VR_HAS_PLUS_SIGN( v2 ) )
            overflow = ( result.Q.D.L.D > (U64) INT_MAX ) ? true : false;
        else
            overflow = ( result.Q.D.L.D < (U64) INT_MIN ) ? true : false;
    }

    /* CC and 32 bit results */
    //logmsg("... result=%16.16lX.%16.16lX \n", result.Q.D.H.D, result.Q.D.L.D);

    regs->GR_L(r1) = (U32) (result.Q.D.L.D & 0xFFFFFFFF);
    if (cs) regs->psw.cc = ( overflow ) ? 3 : 0;

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && FOMASK(&regs->psw))
    {
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E651 VCLZDP - VECTOR COUNT LEADING ZERO DIGITS            [VRR-k] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_count_leading_zero_digits )
{
    int     v1, v2, m3;        /* Instruction parts                  */
    bool    nv;                /* No Validation (NV): m3 bit 1       */
    bool    nz;                /* Negative Zero (NZ): m3 bit 2       */
    bool    cs;                /* Condition Code Set (CS) : m3 bit 3 */

    bool    isZero = false;    /* is V2 zero                         */
    U8      leading_zeros = 0; /* leading zero count                 */
    bool    isNeg;             /* has negative sign                  */
    bool    valid = false;     /* valid packed decimal               */
    BYTE    cc;                /* condition code                     */

    bool    valid_sign2;         /* v2: is sign valid?               */
    bool    valid_decimals2;     /* v2: are decimals valid?          */

    VRR_K( inst, regs, v1, v2, m3 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    nv = (m3 & 0x04) ? true : false;
    nz = (m3 & 0x02) ? true : false;
    cs = (m3 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = vr_packed_valid_sign( regs, v2 );

    if (!nv || cs)
    {
        if ( !valid_decimals2 || !valid_sign2 )
        {
            regs->dxc = DXC_DECIMAL;
            ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
        }
    }

    /* count leading zeros */
    leading_zeros = vr_leading_zero( regs, v2);

    /* determine condition code */
    if (cs)
    {
        isNeg = VR_HAS_MINUS_SIGN( v2 );
        isZero = vr_is_zero( regs, v2 );
        valid = valid_decimals2 && valid_sign2;

        if      ( valid && isZero && !nz && !isNeg)                 cc = 0;
        else if ( valid && (isNeg || (isNeg && nz && isZero) ) )    cc = 1;
        else if ( valid && !isZero && !isNeg )                      cc = 2;
        else                                                        cc = 3;
    }

    /* update V1 */
    SET_VR_ZERO( v1 );
    regs->VR_B( v1, 7) = leading_zeros;

    /* set condition code */
    if (cs)
        regs->psw.cc = cc;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E652 VCVBG  - VECTOR CONVERT TO BINARY (64)               [VRR-i] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_to_binary_64 )
{
    int     r1, v2, m3, m4;      /* Instruction parts                */
    bool    p2;                  /* Force Operand 2 Positive (P2)    */
    bool    lb;                  /* Logical Binary (LB)              */
    bool    cs;                  /* Condition Code Set (CS)          */
    bool    iom;                 /* Instruction-Overflow Mask (IOM)  */
    U128    result;              /* converted binary                 */
    bool    overflow;            /* did an overfor occur             */

    bool    valid_sign2;         /* v2: is sign valid?               */
    bool    valid_decimals2;     /* v2: are decimals valid?          */

    VRR_I( inst, regs, r1, v2, m3, m4 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    p2 = (m3 & 0x08) ? true : false;
    lb = (m3 & 0x02) ? true : false;
    cs = (m3 & 0x01) ? true : false;

    /* m4 parts */
    iom = (m4 & 0x08) ? true : false;

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    if ( !valid_decimals2 || !valid_sign2 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    result = vr_to_U128( regs, v2, ( (lb) ? true : p2 ) );

    /* did overflow happen? */
    if (lb)
        overflow = ( result.Q.D.H.D != 0 );
    else
    {
        if ( (p2) ? true : VR_HAS_PLUS_SIGN( v2 ) )
            overflow = ( result.Q.D.H.D != 0) ;
        else
            overflow = ( result.Q.D.H.D != (U64) -1);
    }

    /* CC and 32 bit results */
    //logmsg("... result=%16.16lX.%16.16lX \n", result.Q.D.H.D, result.Q.D.L.D);

    regs->GR_G(r1) = result.Q.D.L.D;
    if (cs) regs->psw.cc = ( overflow ) ? 3 : 0;

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && FOMASK(&regs->psw))
    {
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E654 VUPKZH - VECTOR UNPACK ZONED HIGH                    [VRR-k] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_zoned_high )
{
    int     v1, v2, m3;        /* Instruction parts                  */
    bool    nsv;               /* No Sign Validation (NSV): bit 0    */
    bool    nv;                /* No Validation (NV): bit 1          */
    int     i;                 /* loop variable                      */
    int     indx;              /* index variable                     */
    U8      temp;              /* temp variable                      */
                               /* local vvector registers            */
    LOCALS()

    VRR_K( inst, regs, v1, v2, m3 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    nsv = (m3 & 0x08) ? true : false;
    nv  = (m3 & 0x04) ? true : false;

    /* validate sign */
    if ( !nsv  &&  !vr_packed_valid_sign( regs, v2) )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* validate decimals */
    if (!nv && !vr_packed_valid_digits( regs, v2 ))
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* local v2 */
    VR_SAVE_LOCAL( LV2, v2 );

    /* set siggnificant zone digit to zero */
    regs->VR_B( v1, 0 )  = 0xF0;

    /* 14 decimals */
    for (i = 1, indx = 0;  i < 14; i += 2, indx++ )
    {
        temp = lregs->VR_B( LV2, indx );
        regs->VR_B( v1, i )  = PACKED_HIGH (temp ) | 0xF0;
        regs->VR_B( v1, i+1 )  = PACKED_LOW  (temp ) | 0xF0;
    }

    /* 15th decimal */
    temp = lregs->VR_B( LV2, 7 );
    regs->VR_B( v1, 15 )  = PACKED_HIGH (temp ) | 0xF0;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E658 VCVD   - VECTOR CONVERT TO DECIMAL (32)              [VRI-i] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_to_decimal_32 )
{
    int     v1, r2, m4, i3;      /* Instruction parts                */
    bool    iom;                 /* Instruction-Overflow Mask (IOM)  */
    int     rdc;                 /* Result Digits Count(RDC) Bit 3-7 */
    bool    p1;                  /* Force Operand 1 Positive (P1)    */
    bool    lb;                  /* Logical Binary (LB)              */
    bool    cs;                  /* Condition Code Set (CS)          */

    bool    possign;             /* result has positive sign         */
    U64     convert;             /* value to convert                 */
    U32     reg32;               /* register to convert              */
    int     i;                   /* Loop variable                    */
    U8      digit;               /* digit of packed byte             */
    int     temp;                /* temp                             */
    bool    overflow;            /* did an overfor occur             */


    VRI_I( inst, regs, v1, r2, m4, i3 );

    ZVECTOR_CHECK( regs );

                              /* i3 reserved bits 1-2 must be zero    */
    if ( i3 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i3 parts */
    iom = (i3 & 0x80) ? true : false;
    rdc = (i3 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );


    /* m4 parts */
    lb = (m4 & 0x08) ? true : false;
    p1 = (m4 & 0x02) ? true : false;
    cs = (m4 & 0x01) ? true : false;

    /* start with zero vector */
    SET_VR_ZERO( v1 );

    /* get sign and value to convert */
    reg32 = regs->GR_L( r2 );  /* 32-bits to convert */
    if (lb)
    {
        convert = (U64) reg32;    /* unsigned */
        possign = true;
    }
    else
    {
        convert = (S32) reg32;  /* signed */
        if (convert >0 )
            possign = true;
        else
        {
            possign = false;
            convert = -convert;
        }
    }

    //logmsg("VECTOR CONVERT TO DECIMAL (32): lb=%d, reg32=%X, convert=%ld, convert=%lx \n", lb, reg32, convert, convert);

    /* do convertion to decimal digits */
    for (i = 30, temp = rdc; temp >0 && i >= 0 && convert > 0; i--, temp--)
    {
        digit = convert % 10;
        convert = convert / 10;

        regs->VR_B( v1, i / 2) |=  ( i & 1) ? digit : digit << 4;
    }

    overflow = convert > 0;     /* did not convert all (rdc limited rersult) */

    /* set sign */
    if (p1)
        regs->VR_B( v1, VR_PACKED_SIGN ) |= 0x0F;   /* forces b'1111' positive sign */
    else
        regs->VR_B( v1, VR_PACKED_SIGN ) |= (possign) ? PREFERRED_PLUS :  PREFERRED_MINUS ;

    /* set condition code */
    if (cs)
        regs->psw.cc = (overflow) ? 3 : 0;

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E659 VSRP   - VECTOR SHIFT AND ROUND DECIMAL              [VRi-g] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_and_round_decimal )
{
    int     v1, v2, i4, m5, i3; /* Instruction parts                 */
                               /* i3 bits                            */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count (RDC): Bits 3-7*/
                               /* i4 bits                            */
    bool    drd;               /* Decimal Rounding Digit (DRD) bit 0 */
    S8      shamt;             /* Shift Amount (SHAMT): Bits 1-7     */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dntemp;          /* temp decNumber                     */
    decNumber dnshift;         /* -shamt as decNumber (note:negative)*/
    decContext set;            /* zn default contect                 */

    VRI_G( inst, regs, v1, v2, i4, m5, i3 );

    ZVECTOR_CHECK( regs );

                                  /* i3 reserved bits 1-2 must be zero    */
    if ( i3 & 0x60 )              /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i3 parts */
    iom = (i3 & 0x80) ? true : false;
    rdc = (i3 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* i4 parts */
    drd   = (i4 & 0x80) ? true : false;
    /* note: shamt is signed 7 bit field... */
    shamt = (i4 & 0x7F);
    shamt = (shamt  > 0x4F ) ? (shamt | 0x80) : shamt;

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    if ( !valid_decimals2 || !valid_sign2 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operand as decNumbers  and set context */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    zn_ContextDefault( &set );

    /* rounding by 5 and right shift */
    if (  shamt < 0  && drd )
    {
        /* note: shift is negative so shift +1 to allow rounding */
        decNumberFromInt32( &dnshift, shamt +1 );
        decNumberShift( &dntemp, &dnv2, &dnshift, &set );

    // dn_logmsg("dntemp: ", &dntemp);

        /* rounding is on a positive value */
        if (decNumberIsNegative( &dnv2) )
            decNumberMinus( &dntemp, &dntemp, &set );

    // dn_logmsg("dntemp: ", &dntemp);

        decNumberFromInt32( &dnshift, 5 ); /*use shift as rounding digit */
        decNumberAdd( &dntemp, &dntemp, &dnshift, &set);

    // dn_logmsg("dntemp: ", &dntemp);

        /* do last 1 position shift right */
        decNumberFromInt32( &dnshift, -1 );
        decNumberShift( &dnv1, &dntemp, &dnshift, &set );

        /* rounding was on a positive value, switch back to negative  */
        if (decNumberIsNegative( &dnv2) )
            decNumberMinus( &dnv1, &dnv1, &set );
    }
    else
    {
        /* get shift as decNumber and shift v2 */
        decNumberFromInt32(&dnshift, shamt);
        decNumberShift(&dnv1, &dnv2, &dnshift, &set);
    }

    // logmsg("... shamt=%d, rdc= %d, drd=%d, p1=%d, p2=%d \n",shamt, rdc, drd, p1, p2);
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnshift: ", &dnshift);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store shifted result in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, rdc);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65A VCVDG  - VECTOR CONVERT TO DECIMAL (64)              [VRI-i] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_to_decimal_64 )
{
    int     v1, r2, m4, i3;      /* Instruction parts                */
    bool    iom;                 /* Instruction-Overflow Mask (IOM)  */
    int     rdc;                 /* Result Digits Count(RDC) Bit 3-7 */
    bool    p1;                  /* Force Operand 1 Positive (P1)    */
    bool    lb;                  /* Logical Binary (LB)              */
    bool    cs;                  /* Condition Code Set (CS)          */

    bool    possign;             /* result has positive sign         */
    S64     tempS64;             /* temp S64                         */
    U64     convert;             /* value to convert                 */
    U64     reg64;               /* register to convert              */
    int     i;                   /* Loop variable                    */
    U8      digit;               /* digit of packed byte             */
    int     temp;                /* temp                             */
    bool    overflow;            /* did an overfor occur             */

    VRI_I( inst, regs, v1, r2, m4, i3 );

    ZVECTOR_CHECK( regs );

                              /* i3 reserved bits 1-2 must be zero    */
    if ( i3 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i3 parts */
    iom = (i3 & 0x80) ? true : false;
    rdc = (i3 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m4 parts */
    lb = (m4 & 0x08) ? true : false;
    p1 = (m4 & 0x02) ? true : false;
    cs = (m4 & 0x01) ? true : false;

    /* start with zero vector */
    SET_VR_ZERO( v1 );

    /* get sign and value to convert */
    reg64 = regs->GR( r2 );  /* 64-bits to convert */
    if (lb)
    {
        convert = reg64;    /* unsigned */
        possign = true;
    }
    else
    {                       /* signed */
        tempS64 = (S64) reg64;
        if (tempS64 >0 )
        {
            possign = true;
            convert = (U64) tempS64;
        }
        else
        {
            possign = false;
            convert = (U64) -tempS64;
        }
    }

    //logmsg("VECTOR CONVERT TO DECIMAL (64): lb=%d, reg64=%lX, convert=%ld, convert=%lx \n", lb, reg64, convert, convert);

    /* do convertion to decimal digits */
    for (i = 30, temp = rdc; temp >0 && i >= 0 && convert > 0; i--, temp--)
    {
        digit = convert % 10;
        convert = convert / 10;

        regs->VR_B( v1, i / 2) |=  ( i & 1) ? digit : digit << 4;
    }

    overflow = convert > 0;     /* did not convert all (rdc limited rersult) */

    /* set sign */
    if (p1)
        regs->VR_B( v1, VR_PACKED_SIGN) |= 0x0F;   /* forces b'1111' positive sign */
    else
        regs->VR_B( v1, VR_PACKED_SIGN) |= (possign) ? PREFERRED_PLUS :  PREFERRED_MINUS ;

    /* set condition code */
    if (cs)
        regs->psw.cc = (overflow) ? 3 : 0;

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65B VPSOP  - VECTOR PERFORM SIGN OPERATION DECIMAL       [VRI-g] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_perform_sign_operation_decimal )
{
    int     v1, v2, i4, m5, i3; /* Instruction parts                 */
                               /* i3 bits                            */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count (RDC): Bits 3-7*/
                               /* i4 bits                            */
    bool    nv;                /* No Validation (NV): bit 0          */
    bool    nz;                /* Negative Zero (NZ): bit 1          */
    U8      so;                /* Sign Operation (SO): Bits 4-5      */
    bool    pc;                /* Positive Sign Code (PC): bit 6     */
    bool    sv;                /* Op 2 Sign Validation (SV): bit 7   */
                               /* m5 bits                            */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    BYTE    cc = 0;            /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */
    bool    isZero = false;    /* is result zero                     */
    bool    suppressingDX = false; /* suppressed data exception      */
                               /* local vector registers             */
    LOCALS()

    VRI_G( inst, regs, v1, v2, i4, m5, i3 );

    ZVECTOR_CHECK( regs );

                              /* i3 reserved bits 1-2 must be zero    */
    if ( i3 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i3 parts */
    iom = (i3 & 0x80) ? true : false;
    rdc = (i3 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    nv = (i4 & 0x80) ? true : false;
    nz = (i4 & 0x40) ? true : false;
    so = (i4 & 0x0C) >> 2;
    pc = (i4 & 0x02) ? true : false;
    sv = (i4 & 0x01) ? true : false;

    /* m5 parts */
    cs = (m5 & 0x01) ? true : false;

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    nv = false;        /* validate digits  */
    nz = false;        /* no negative zero */
#endif

    /* valid checks */
    valid_decimals2 = ( nv ) ? true : vr_packed_valid_digits( regs, v2 );
    if (
            (so == 0x00 && !nv ) ||
            (so == 0x01 )        ||
            (so == 0x02 &&  sv ) ||
            (so == 0x03 &&  sv )
       )
        valid_sign2 = vr_packed_valid_sign( regs, v2 );
    else
        valid_sign2 = true; /* ignored */

    if ( !valid_decimals2 || !valid_sign2 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* local v2 */
    VR_SAVE_LOCAL( LV2, v2 );

    /* initialize V1 */
    lv_copy_to_vr( regs, v1, lregs, LV2, rdc );
    overflow = lv_leading_zero(lregs , LV2) < (MAX_DECIMAL_DIGITS - rdc);
    isZero = vr_is_zero(regs, v1);

    /* programmer note: letting compiler optimize the following! */
    switch (so)
    {
        case 0x00:                     /* 00 (maintain)       */
        {
            if ( !LV_HAS_VALID_SIGN( LV2 ) )   { cc = 2; break;  }

            if (isZero)
            {
               if ( LV_HAS_PLUS_SIGN( LV2 ) && pc )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }

                if ( LV_HAS_PLUS_SIGN( LV2 ) && !pc )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && !pc && !nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && pc && !nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_MINUS);  break;  }
            }
            else
            {
                if ( LV_HAS_PLUS_SIGN( LV2 ) && pc )
                    { cc = 2; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }

                if ( LV_HAS_PLUS_SIGN( LV2 ) && !pc )
                    { cc = 2; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && nz )
                    { cc = 1; SET_VR_SIGN( v1, PREFERRED_MINUS);  break;  }
            }
        }
        break;

        case 0x01:                     /* 01 (complement)     */
        {
            if ( !LV_HAS_VALID_SIGN( LV2 ) )   { suppressingDX = true; break;  }

            if (isZero)
            {
                if ( LV_HAS_PLUS_SIGN( LV2 ) && !pc && !nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }

                if ( LV_HAS_PLUS_SIGN( LV2 ) &&  pc && !nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }

                if ( LV_HAS_PLUS_SIGN( LV2 ) &&  nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_MINUS);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && pc )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && !pc )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }
            }
            else
            {
                if ( LV_HAS_PLUS_SIGN( LV2 ) )
                    { cc = 1; SET_VR_SIGN( v1, PREFERRED_MINUS);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && pc )
                    { cc = 2; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }

                if ( LV_HAS_MINUS_SIGN( LV2 ) && !pc )
                    { cc = 2; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }
            }
        }
        break;

        case 0x02:                     /* 10 (force positive) */
        {
            if (isZero)
            {
                if ( pc )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }
                else
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }
            }
            else
            {
                if ( pc )
                    { cc = 2; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }
                else
                    { cc = 2; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }
            }
        }
        break;

        case 0x03:                     /* 11 (force negative) */
        {
            if (isZero)
            {
                   if ( !pc && !nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_PLUS);  break;  }

                   if ( pc && !nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_ZONE);  break;  }

                   if ( nz )
                    { cc = 0; SET_VR_SIGN( v1, PREFERRED_MINUS);  break;  }

            }
            else
            {
                    { cc = 1; SET_VR_SIGN( v1, PREFERRED_MINUS);  break;  }
            }
        }
        break;

    }

    /* invalid sign */
    if ( suppressingDX )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* set condition code */
    if (cs)
    {
        if ( isZero ) cc= 0;        /* regardless on sign */
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65C VUPKZL - VECTOR UNPACK ZONED LOW                     [VRR-k] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_zoned_low )
{
    int     v1, v2, m3;        /* Instruction parts                  */
    bool    nsv;               /* No Sign Validation (NSV): bit 0    */
    bool    nv;                /* No Validation (NV): bit 1          */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    int     i;                 /* loop variable                      */
    int     indx;              /* index variable                     */
    U8      temp;              /* temp variable                      */
    U8      zoned_sign;        /* sign for zoned digit               */
                               /* local vector registers             */
    LOCALS()

    VRR_K( inst, regs, v1, v2, m3 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    nsv = (m3 & 0x08) ? true : false;
    nv  = (m3 & 0x04) ? true : false;
    p1  = (m3 & 0x02) ? true : false;

    /* validate sign: nsv=0 and nv=0 */
    if ( !nsv && !nv && !vr_packed_valid_sign( regs, v2 ) )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* validate decimals */
    if (!nv && !vr_packed_valid_digits( regs, v2 ))
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* save v2 */
    VR_SAVE_LOCAL( LV2, v2);

    /* result sign */
    zoned_sign = (p1) ? PREFERRED_ZONE : PACKED_SIGN ( lregs->VR_B( LV2, VR_PACKED_SIGN ) );
    zoned_sign <<=  4;

    /* 1st decimal */
    temp = lregs->VR_B( LV2, 7 );
    regs->VR_B( v1, 0 )  = PACKED_LOW (temp ) | 0xF0;

    /* 2-15 decimals */
    for (i = 1, indx = 8;  i < 15; i += 2, indx++ )
    {
        temp = lregs->VR_B( LV2, indx );
        regs->VR_B( v1, i )  = PACKED_HIGH (temp ) | 0xF0;
        regs->VR_B( v1, i+1 )  = PACKED_LOW  (temp ) | 0xF0;
    }

    /* 16th decimal */
    temp = lregs->VR_B( LV2, 15 );
    regs->VR_B( v1, 15 )  = PACKED_HIGH (temp ) | zoned_sign;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65F VTP    - VECTOR TEST DECIMAL                         [VRR-g] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_test_decimal )
{
    int     v1;                /* Instruction parts                  */
    bool    valid_decimal;     /* decimal validation failed?         */
    bool    valid_sign;        /* sign validation failed?            */
    U8      cc;                /* condition code                     */

    VRR_G( inst, regs, v1 );

    ZVECTOR_CHECK( regs );

    /* validate decimals */
    valid_decimal = vr_packed_valid_digits( regs, v1 );

    /* validate sign */
    valid_sign = vr_packed_valid_sign( regs, v1 );

    /* set condition code */
    cc = (valid_decimal) ?  ( (valid_sign) ? 0 : 1) :
                            ( (valid_sign) ? 2 : 3) ;
    regs->psw.cc = cc;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E670 VPKZR  - VECTOR PACK ZONED REGISTER                  [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_pack_zoned_register )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
    bool    nsv;               /* No Sign Validation (NSV):    bit 0 */
    bool    nv;                /* No Validation (NV):          bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    overflowed;        /* overflow occurred                  */
    bool    isZero;            /* is zoned decimal zero              */
    bool    isPositive;        /* is zoned decimal positive          */
    int     i;                 /* loop variable                      */
    int     indx;              /* index variable                     */
    U8      temp;              /* temp variable                      */
    int     temp_rdc;          /* temp of rdc                        */
    U8      zoned[32];         /* intermediate zoned decimal         */
    U8      packed_sign;       /* sign for packed vector             */
    QW      tempVR;            /* temp vector regiter sized field    */
    bool    valid_sign;        /* is sign valid?                     */
    bool    valid_decimals;    /* are decimals valid?                */
    BYTE    cc;                /* condition code                     */


#if !defined( FEATURE_192_VECT_PACKDEC_ENH_2_FACILITY )
    ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
#endif

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    nsv = (m5 & 0x08) ? true : false;
    nv  = (m5 & 0x04) ? true : false;
    p1  = (m5 & 0x02) ? true : false;
    cs  = (m5 & 0x01) ? true : false;

    /* create intermediate source */
    tempVR =  CSWAP128( regs->VR_Q ( v2 ) );    /* VR part 1 */
    memcpy( &zoned[0], &tempVR, sizeof(QW)  );

    tempVR =  CSWAP128( regs->VR_Q ( v3 ) );    /* VR part 2 */
    memcpy( &zoned[16], &tempVR, sizeof(QW)  );

    /* zoned validation */
    valid_sign = ZONED_SIGN( zoned[31] ) > 9;
    valid_decimals = true;
    for (i =0; i < 32 && valid_decimals; i++)
        valid_decimals = IS_VALID_DECIMAL( zoned[i] );

    /* general-operand data exception? */
    /* validate sign: nsv=0 and nv=0 */
    if ( !nsv && !nv && !valid_sign )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* validate decimals */
    if ( !nv & !valid_decimals )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* build packed vector */
    SET_VR_ZERO( v1 );
    packed_sign = (p1) ? PREFERRED_ZONE : ZONED_SIGN( zoned[31] );
    regs->VR_B( v1, VR_PACKED_SIGN) = (ZONED_DECIMAL( zoned[31] ) << 4) | packed_sign;

    indx = 14;
    for (i = 30, temp_rdc = rdc-1; i >=0 && temp_rdc > 0 && indx >=0 ; i--, temp_rdc--)
    {
        temp = ZONED_DECIMAL( zoned[ i ] );
        regs->VR_B( v1, indx) |= (i & 1) ? temp << 4 : temp;
        if ( i & 1) indx--;
    }

    /* check for overflow */
    overflowed = false;
    for (i = 0; i < (32 - rdc) && !overflowed; i++)
        overflowed = ( ZONED_DECIMAL( zoned[ i ]) == 0 ) ? false : true;

    /* set condition code */
    if (cs)
    {
        /* is the zone decimal 0 */
        isZero = true;
        for (i = 0; i < 32 && isZero; i++)
            isZero = ( ZONED_DECIMAL( zoned[ i ]) == 0 ) ? true : false;

        /* is the zone decimal positive */
        isPositive = IS_PLUS_SIGN(  packed_sign  );

        if       ( overflowed || !valid_decimals || !valid_sign) cc = 3;
        else if  (!overflowed &&  valid_decimals &&  valid_sign && !isZero &&  isPositive) cc = 2;
        else if  (!overflowed &&  valid_decimals &&  valid_sign && !isZero && !isPositive) cc = 1;
        else if  (!overflowed &&  valid_decimals &&  valid_sign &&  isZero ) cc = 0;
        else cc = 0; /* should not get here */

        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflowed  && FOMASK(&regs->psw))
    {
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E671 VAP    - VECTOR ADD DECIMAL                          [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dnv3;            /* v3 as decNumber                    */
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                               /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    vr_to_decNumber( regs, v3, &dnv3, p3);

    /* get product */
    zn_ContextDefault( &set );
    decNumberAdd( &dnv1,  &dnv2, &dnv3, &set );

    // DEBUG
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnv3: ", &dnv3);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store product in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, MAX_DECIMAL_DIGITS);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E672 VSRPR  - VECTOR SHIFT AND ROUND DECIMAL REGISTER     [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_and_round_decimal_register )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
                               /* i4 bits                            */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    bool    drd;               /* Decimal Rounding Digit (DRD) bit 1 */
    int     rdc;               /* Result Digits Count (RDC): Bits 3-7*/

                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    S8      shamt;             /* Shift Amount (SHAMT): V3 byte 7    */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dntemp;          /* temp decNumber                     */
    decNumber dnshift;         /* -shamt as decNumber (note:negative)*/
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                                  /* i3 reserved bit 2 must be zero      */
    if ( i4 & 0x20 )              /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    drd = (i4 & 0x40) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* get shamt from v3, bytes 7. note: shamt is signed */
    shamt = (S8) regs->VR_B( v3, 7);
    if (shamt < -32 ) shamt = -32;
    if (shamt > +31 ) shamt = +31;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    if ( !valid_decimals2 || !valid_sign2 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operand as decNumbers  and set context */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    zn_ContextDefault( &set );

    /* rounding by 5 and right shift */
    if (  shamt < 0  && drd )
    {
        /* note: shift is negative so shift +1 to allow rounding */
        decNumberFromInt32( &dnshift, shamt +1 );
        decNumberShift( &dntemp, &dnv2, &dnshift, &set );

    //  dn_logmsg("dntemp: ", &dntemp);

        /* rounding is on a positive value */
        if (decNumberIsNegative( &dnv2) )
            decNumberMinus( &dntemp, &dntemp, &set );

    // dn_logmsg("dntemp: ", &dntemp);

        decNumberFromInt32( &dnshift, 5 ); /*use shift as rounding digit */
        decNumberAdd( &dntemp, &dntemp, &dnshift, &set);

    // dn_logmsg("dntemp: ", &dntemp);

        /* do last 1 position shift right */
        decNumberFromInt32( &dnshift, -1 );
        decNumberShift( &dnv1, &dntemp, &dnshift, &set );

        /* rounding was on a positive value, switch back to negative  */
        if (decNumberIsNegative( &dnv2) )
            decNumberMinus( &dnv1, &dnv1, &set );
    }
    else
    {
        /* get shift as decNumber and shift v2 */
        decNumberFromInt32(&dnshift, shamt);
        decNumberShift(&dnv1, &dnv2, &dnshift, &set);
    }

    // logmsg("... shamt=%d, rdc= %d, drd=%d, p1=%d, p2=%d \n",shamt, rdc, drd, p1, p2);
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnshift: ", &dnshift);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store shifted result in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, rdc);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E673 VSP    - VECTOR SUBTRACT DECIMAL                     [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_subtract_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dnv3;            /* v3 as decNumber                    */
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    vr_to_decNumber( regs, v3, &dnv3, p3);

    /* get product */
    zn_ContextDefault( &set );
    decNumberSubtract( &dnv1,  &dnv2, &dnv3, &set );

    // DEBUG
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnv3: ", &dnv3);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store product in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, MAX_DECIMAL_DIGITS);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E677 VCP    - VECTOR COMPARE DECIMAL                      [VRR-h] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_compare_decimal )
{
    int     v1, v2, m3;        /* Instruction parts                 */

                               /* m3 bits                            */
    bool    p1;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p2;                /* Force Operand 3 Positive(P1) bit 3 */

    bool    valid_sign1;       /* v1: is sign valid?                 */
    bool    valid_decimals1;   /* v1: are decimals valid?            */
    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    BYTE    cc;                /* condition code                     */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dncompared;      /* compared as decNumber              */
    decContext set;            /* zn default contect                 */

    VRR_H(inst, regs, v1, v2, m3);

    ZVECTOR_CHECK( regs );

    /* m5 parts */
    p1 = (m3 & 0x08) ? true : false;
    p2 = (m3 & 0x04) ? true : false;

    /* valid checks */
    valid_decimals1 = vr_packed_valid_digits( regs, v1 );
    valid_sign1 = (p1) ? true : vr_packed_valid_sign( regs, v1 );

    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    if (!valid_decimals1 || !valid_sign1 || !valid_decimals2 || !valid_sign2)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v1, &dnv1, p1);
    vr_to_decNumber( regs, v2, &dnv2, p2);

    /* get compare  result*/
    zn_ContextDefault( &set );
    decNumberCompare( &dncompared,  &dnv1, &dnv2, &set );

    cc =  ( decNumberIsZero( &dncompared ) ) ? 0 : ( decNumberIsNegative( &dncompared ) ) ? 1 : 2;
    regs->psw.cc = cc;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E678 VMP    - VECTOR MULTIPLY DECIMAL                     [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dnv3;            /* v3 as decNumber                    */
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    vr_to_decNumber( regs, v3, &dnv3, p3);

    /* get product */
    zn_ContextDefault( &set );
    decNumberMultiply( &dnv1,  &dnv2, &dnv3, &set );

    // DEBUG
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnv3: ", &dnv3);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store product in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, MAX_DECIMAL_DIGITS);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E679 VMSP   - VECTOR MULTIPLY AND SHIFT DECIMAL           [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_shift_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
                               /* i4 bits                            */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     shamt;             /* Shift Amount (SHAMT): Bits 3-7     */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dnv3;            /* v3 as decNumber                    */
    decNumber dnproduct;       /* (v2 * v3) as decNumber             */
    decNumber dnshift;         /* -shamt as decNumber (note:negative)*/
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom   = (i4 & 0x80) ? true : false;
    shamt = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    vr_to_decNumber( regs, v3, &dnv3, p3);

    /* get product */
    zn_ContextDefault( &set );
    decNumberMultiply( &dnproduct,  &dnv2, &dnv3, &set );

    /* get shift as decNumber: right shift is negative */
    decNumberFromInt32(&dnshift, -shamt);
    decNumberShift(&dnv1, &dnproduct, &dnshift, &set);

    // DEBUG
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnv3: ", &dnv3);
    // dn_logmsg("dnvproduct: ", &dnproduct);
    // dn_logmsg("dnshift: ", &dnshift);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store shifted result in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, MAX_DECIMAL_DIGITS);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );


    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67A VDP    - VECTOR DIVIDE DECIMAL                       [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_divide_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dnv3;            /* v3 as decNumber                    */
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if ( !valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    vr_to_decNumber( regs, v3, &dnv3, p3);

    /* Program check if divisor (v3) is zero */
    if ( decNumberIsZero( &dnv3 ) )
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_DIVIDE_EXCEPTION);

    /* get integer quotient */
    zn_ContextDefault( &set );
    decNumberDivideInteger( &dnv1,  &dnv2, &dnv3, &set );

    // DEBUG
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnv3: ", &dnv3);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store product in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, MAX_DECIMAL_DIGITS);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67B VRP    - VECTOR REMAINDER DECIMAL                    [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_remainder_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dnv3;            /* v3 as decNumber                    */
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if ( !valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    vr_to_decNumber( regs, v3, &dnv3, p3);

    /* Program check if divisor (v3) is zero */
    if ( decNumberIsZero( &dnv3 ) )
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_DIVIDE_EXCEPTION);

    /* get remander */
    zn_ContextDefault( &set );
    decNumberRemainder( &dnv1,  &dnv2, &dnv3, &set );

    // DEBUG
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnv3: ", &dnv3);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store product in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, MAX_DECIMAL_DIGITS);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67E VSDP   - VECTOR SHIFT AND DIVIDE DECIMAL             [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_and_divide_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     shamt;             /* Shift Amount (SHAMT): Bits 3-7     */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    decNumber dnv1;            /* v1 as decNumber                    */
    decNumber dnv2;            /* v2 as decNumber                    */
    decNumber dnv3;            /* v3 as decNumber                    */
    decNumber dnshift;         /* shamt as decNumber                 */
    decNumber dntemp;          /* temp decNumber                     */
    decContext set;            /* zn default contect                 */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom   = (i4 & 0x80) ? true : false;
    shamt = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if ( !valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3 )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decNumbers */
    vr_to_decNumber( regs, v2, &dnv2, p2);
    vr_to_decNumber( regs, v3, &dnv3, p3);

    /* Program check if divisor (v3) is zero */
    if ( decNumberIsZero( &dnv3 ) )
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_DIVIDE_EXCEPTION);

    /* shifted left the dividend (v2); get integer quotient */
    zn_ContextDefault( &set );
    decNumberFromInt32( &dnshift, shamt);
    decNumberShift( &dntemp, &dnv2, &dnshift, &set);
    decNumberDivideInteger( &dnv1,  &dntemp, &dnv3, &set );

    // DEBUG
    // dn_logmsg("dnv2: ", &dnv2);
    // dn_logmsg("dnv3: ", &dnv3);
    // dn_logmsg("dnshift: ", &dnshift);
    // dn_logmsg("dntemp: ", &dntemp);
    // dn_logmsg("dnv1: ", &dnv1);

    /* store product in vector register */
    overflow = vr_from_decNumber( regs, v1, &dnv1, p1, MAX_DECIMAL_DIGITS);

    /* set condition code */
    if (cs)
    {
        cc = ( decNumberIsZero( &dnv1 ) ) ? 0 : ( decNumberIsNegative( &dnv1 ) ) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}


/*===================================================================*/
/* Vector Floating Point                                             */
/*===================================================================*/

/* ============================================= */
/* TEMPORARY while zvector2.c is being developed */
#if defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    #pragma GCC diagnostic ignored "-Wcomment"
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
/* ============================================= */

/*-------------------------------------------------------------------*/
/* E655 VCNF   - VECTOR FP CONVERT TO NNP                    [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_nnp )
{
    int     v1, v2, m3, m4, m5;           /* instruction parts       */

    VRR_A(inst, regs, v1, v2, m3, m4, m5);

    ZVECTOR_CHECK( regs );

    /* note: m5 is not used in this instruction                     */

    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*--------------------------------------------------------------------*/
/* E656 VCLFNH - VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH [VRR_a] */
/*--------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_lengthen_from_nnp_high )
{
    int     v1, v2, m3, m4, m5;

    VRR_A(inst, regs, v1, v2, m3, m4, m5);

    ZVECTOR_CHECK( regs );

    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}


/*-------------------------------------------------------------------*/
/* E65D VCFN   - VECTOR FP CONVERT FROM NNP                  [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_from_nnp )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65E VCLFNL - VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_lengthen_from_nnp_low )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E674 VSCHP  - DECIMAL SCALE AND CONVERT TO HFP            [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( decimal_scale_and_convert_to_hfp )
{
    int     v1, v2, m3, m4, m5;

    VRR_B(inst, regs, v1, v2, m3, m4, m5);
    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E675 VCRNF  - VECTOR FP CONVERT AND ROUND TO NNP          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_round_to_nnp )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);
    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67C VSCSHP - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP  [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST(decimal_scale_and_convert_and_split_to_hfp )
{
    int     v1, v2, v3, m4, m5;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67D VCSPH  - VECTOR CONVERT HFP TO SCALED DECIMAL        [VRR-j] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_hfp_to_scaled_decimal )
{
    int     v1, v2, v3, m4;

    VRR_J( inst, regs, v1, v2, v3, m4 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

#endif /* defined(FEATURE_129_ZVECTOR_FACILITY) */

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "zvector2.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "zvector2.c"
  #endif

#endif /*!defined(_GEN_ARCH)*/
