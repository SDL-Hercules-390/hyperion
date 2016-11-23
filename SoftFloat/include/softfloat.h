
/*============================================================================

This C header file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3a, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014 The Regents of the University of California.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

/*============================================================================
Modifications to comply with IBM IEEE Binary Floating Point, as defined
in the z/Architecture Principles of Operation, SA22-7832-10, by
Stephen R. Orso.  Said modifications identified by compilation conditioned
on preprocessor variable IBM_IEEE.
All such modifications placed in the public domain by Stephen R. Orso
=============================================================================*/

/*============================================================================
| Note:  If SoftFloat is made available as a general library for programs to
| use, it is strongly recommended that a platform-specific version of this
| header, "softfloat.h", be created that folds in "softfloat_types.h" and that
| eliminates all dependencies on compile-time macros.
*============================================================================*/


#ifndef softfloat_h
#define softfloat_h 1

#if !defined(false) 
#include <stdbool.h> 
#endif
#if !defined(int32_t) 
#include <stdint.h>             /* C99 standard integers */ 
#endif

#include "softfloat_types.h"

/****************************************************************************************/
/*                                                                                      */
#define IBM_IEEE            /* Compile Softfloat to be compliant with with IBM IEEE     */
/*                                                                                      */
/****************************************************************************************/

#ifdef IBM_IEEE

 /* We never need anything fancy from softfloat_raiseFlag(), so there is no need    */
 /* to call a function.  A macro that does the logical or is quite sufficient.      */

# define softfloat_raiseFlags(flags) softfloat_exceptionFlags |= flags

# ifndef SF_THREAD_LOCAL
#  if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#   define SF_THREAD_LOCAL _Thread_local

#  elif defined _WIN32 && (          /* Windows platform and             */      \
        defined _MSC_VER ||          /* ..Visual Studio C or             */      \
        defined __ICL ||             /* ..Intel C or                     */      \
        defined __DMC__ ||           /* ..Digital Mars C or              */      \
        defined __BORLANDC__ )       /* ..Borland (Embarcadero) C        */
#   define SF_THREAD_LOCAL __declspec(thread) 

/* note that ICC (linux) and Clang are reportedly covered by __GNUC__   */
#  elif defined __GNUC__ ||          /* ..GNG C or variants or           */      \
        defined __SUNPRO_C ||        /* ..SunPro (Oracle) C or           */      \
        defined __xlC__              /* ..IBM C                          */
#   define SF_THREAD_LOCAL __thread

#  else
#   error "Cannot define SF_THREAD_LOCAL.  No Softfloat thread safety."
#   define SF_THREAD_LOCAL ""
#  endif /* compiler detection*/
# endif  /* SF_THREAD_LOCAL  */

#endif /* IBM_IEEE  */

/*----------------------------------------------------------------------------
| Software floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
extern uint_fast8_t softfloat_detectTininess;
enum {
    softfloat_tininess_beforeRounding = 0,
    softfloat_tininess_afterRounding  = 1
};

/*----------------------------------------------------------------------------
| Software floating-point rounding mode.
*----------------------------------------------------------------------------*/
#ifdef IBM_IEEE 
extern SF_THREAD_LOCAL  uint_fast8_t softfloat_roundingMode;
#else
extern uint_fast8_t softfloat_roundingMode;
#endif  /* IBM_IEEE */
enum {
    softfloat_round_near_even = 0,
    softfloat_round_minMag = 1,
    softfloat_round_min = 2,
    softfloat_round_max = 3,
    softfloat_round_near_maxMag = 4
#ifdef IBM_IEEE
    , softfloat_round_stickybit = 5
#endif /*  IBM_IEEE*/
};

/*----------------------------------------------------------------------------
| Software floating-point exception flags.
*----------------------------------------------------------------------------*/
#ifdef IBM_IEEE
extern SF_THREAD_LOCAL uint_fast8_t softfloat_exceptionFlags;
#else
extern  uint_fast8_t softfloat_exceptionFlags;
#endif /*  IBM_IEEE  */
enum {
    softfloat_flag_inexact   =  1,
    softfloat_flag_underflow =  2,
    softfloat_flag_overflow  =  4,
    softfloat_flag_infinite  =  8,
    softfloat_flag_invalid   = 16
#ifdef IBM_IEEE
    , softfloat_flag_incremented = 32
    , softfloat_flag_tiny = 64
#endif  /*  IBM_IEEE   */
};

#ifdef IBM_IEEE
/*----------------------------------------------------------------------------
| Raw unbiased unbounded exponent and raw rounded significand, used for return
| of scaled results following a trappable overflow or underflow.  Because
| trappability is dependent on the caller's state, not Softfloat's, these
| values are generated for every rounding.
|
| The 128-bit rounded significand is stored with the binary point between 
| the second and third bits (from the left).  
|
|                  -----------------------------------------------
| bit              | 0 1 V 2 3 4 5 6 7 8 9 10 11 12 13 14... 127 |
| Place value      | 2 1 | fractional portion of significand     |
|                  -----------------------------------------------
|
| Note: place value is one higher than the power of two for that digit position. 
|
| Examples:
|   decimal 3 is represented as 11.000000000000000 ... 000
|   decimal 1 is represented as 01.000000000000000 ... 000
|
| The exponent bias is reduced by one to account for this; the leftmost digit
| appears only when rounding or arithmetic generate a carry into the 2's 
| position. 
|
| The booleans softfloat_rawInexact and softfloat_rawIncre preserve the
| status of the original result.  For non-trap results, the original result
| is replaced by a non-finite value and Inexact and Incre are not returned.
| When the scaled result is return, these booleans are used to update
| the softfloat_exceptionFlags.
|
| softfloat_rawTiny is used by the scaled result routines to ensure a
| renormalization of the scaled tiny result.  This avoids using the
| softfloat_exceptionFlags value that reports tiny because that flag is part
| of the external interface of Softfloat, not part of the internal state.
|
| The routines fxxx_returnScaledResult() uses these values to generate 
| scaled results.
*----------------------------------------------------------------------------*/

  /* The following struct should really be in softfloat_types.h, but doing it here      */
  /*   limits the modification footprint                                                */
typedef struct {                /* fields needed to return a scaled result  */
    uint_fast64_t Sig64;        /* Rounded significand bits 1-63    */
    uint_fast64_t Sig0;         /* Rounded significand bits 64-128  */
    int_fast16_t  Exp;          /* signed unbiased exponent         */
    bool          Sign;         /* sign of result                   */
    bool          Inexact;      /* Is raw result inexact            */
    bool          Incre;        /* Was rounded result incremented   */
    bool          Tiny;         /* Was rounded result a subnormal   */
        } softfloat_raw_t;

extern SF_THREAD_LOCAL softfloat_raw_t softfloat_raw;

float32_t  f32_scaledResult(int_fast16_t);      /* return scaled float32 result     */
float64_t  f64_scaledResult(int_fast16_t);      /* return scaled float64 result     */
float128_t f128_scaledResult(int_fast16_t);     /* return scaled float128 result    */

#endif /* IBM_IEEE  */



/*----------------------------------------------------------------------------
| Routine to raise any or all of the software floating-point exception flags.
*----------------------------------------------------------------------------*/

/* IBM_IEEE versions of Softfloat do not require function functionality; all
   traps are detected and managed by the caller.  A macro (above) defines 
   the logical or that is at the core of softfloat_raiseFlags()              */

#ifndef IBM_IEEE
void softfloat_raiseFlags( uint_fast8_t );
#endif

/*----------------------------------------------------------------------------
| Integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/
float32_t ui32_to_f32( uint32_t );
float64_t ui32_to_f64( uint32_t );
#ifdef SOFTFLOAT_FAST_INT64
extFloat80_t ui32_to_extF80( uint32_t );
float128_t ui32_to_f128( uint32_t );
#endif
void ui32_to_extF80M( uint32_t, extFloat80_t * );
void ui32_to_f128M( uint32_t, float128_t * );
float32_t ui64_to_f32( uint64_t );
float64_t ui64_to_f64( uint64_t );
#ifdef SOFTFLOAT_FAST_INT64
extFloat80_t ui64_to_extF80( uint64_t );
float128_t ui64_to_f128( uint64_t );
#endif
void ui64_to_extF80M( uint64_t, extFloat80_t * );
void ui64_to_f128M( uint64_t, float128_t * );
float32_t i32_to_f32( int32_t );
float64_t i32_to_f64( int32_t );
#ifdef SOFTFLOAT_FAST_INT64
extFloat80_t i32_to_extF80( int32_t );
float128_t i32_to_f128( int32_t );
#endif
void i32_to_extF80M( int32_t, extFloat80_t * );
void i32_to_f128M( int32_t, float128_t * );
float32_t i64_to_f32( int64_t );
float64_t i64_to_f64( int64_t );
#ifdef SOFTFLOAT_FAST_INT64
extFloat80_t i64_to_extF80( int64_t );
float128_t i64_to_f128( int64_t );
#endif
void i64_to_extF80M( int64_t, extFloat80_t * );
void i64_to_f128M( int64_t, float128_t * );

/*----------------------------------------------------------------------------
| 32-bit (single-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f32_to_ui32( float32_t, uint_fast8_t, bool );
uint_fast64_t f32_to_ui64( float32_t, uint_fast8_t, bool );
int_fast32_t f32_to_i32( float32_t, uint_fast8_t, bool );
int_fast64_t f32_to_i64( float32_t, uint_fast8_t, bool );
uint_fast32_t f32_to_ui32_r_minMag( float32_t, bool );
uint_fast64_t f32_to_ui64_r_minMag( float32_t, bool );
int_fast32_t f32_to_i32_r_minMag( float32_t, bool );
int_fast64_t f32_to_i64_r_minMag( float32_t, bool );
float64_t f32_to_f64( float32_t );
#ifdef SOFTFLOAT_FAST_INT64
extFloat80_t f32_to_extF80( float32_t );
float128_t f32_to_f128( float32_t );
#endif
void f32_to_extF80M( float32_t, extFloat80_t * );
void f32_to_f128M( float32_t, float128_t * );
float32_t f32_roundToInt( float32_t, uint_fast8_t, bool );
float32_t f32_add( float32_t, float32_t );
float32_t f32_sub( float32_t, float32_t );
float32_t f32_mul( float32_t, float32_t );
float32_t f32_mulAdd( float32_t, float32_t, float32_t );
float32_t f32_div( float32_t, float32_t );
float32_t f32_rem( float32_t, float32_t );
float32_t f32_sqrt( float32_t );
bool f32_eq( float32_t, float32_t );
bool f32_le( float32_t, float32_t );
bool f32_lt( float32_t, float32_t );
bool f32_eq_signaling( float32_t, float32_t );
bool f32_le_quiet( float32_t, float32_t );
bool f32_lt_quiet( float32_t, float32_t );
bool f32_isSignalingNaN( float32_t );

/*----------------------------------------------------------------------------
| 64-bit (double-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f64_to_ui32( float64_t, uint_fast8_t, bool );
uint_fast64_t f64_to_ui64( float64_t, uint_fast8_t, bool );
int_fast32_t f64_to_i32( float64_t, uint_fast8_t, bool );
int_fast64_t f64_to_i64( float64_t, uint_fast8_t, bool );
uint_fast32_t f64_to_ui32_r_minMag( float64_t, bool );
uint_fast64_t f64_to_ui64_r_minMag( float64_t, bool );
int_fast32_t f64_to_i32_r_minMag( float64_t, bool );
int_fast64_t f64_to_i64_r_minMag( float64_t, bool );
float32_t f64_to_f32( float64_t );
#ifdef SOFTFLOAT_FAST_INT64
extFloat80_t f64_to_extF80( float64_t );
float128_t f64_to_f128( float64_t );
#endif
void f64_to_extF80M( float64_t, extFloat80_t * );
void f64_to_f128M( float64_t, float128_t * );
float64_t f64_roundToInt( float64_t, uint_fast8_t, bool );
float64_t f64_add( float64_t, float64_t );
float64_t f64_sub( float64_t, float64_t );
float64_t f64_mul( float64_t, float64_t );
float64_t f64_mulAdd( float64_t, float64_t, float64_t );
float64_t f64_div( float64_t, float64_t );
float64_t f64_rem( float64_t, float64_t );
float64_t f64_sqrt( float64_t );
bool f64_eq( float64_t, float64_t );
bool f64_le( float64_t, float64_t );
bool f64_lt( float64_t, float64_t );
bool f64_eq_signaling( float64_t, float64_t );
bool f64_le_quiet( float64_t, float64_t );
bool f64_lt_quiet( float64_t, float64_t );
bool f64_isSignalingNaN( float64_t );

/*----------------------------------------------------------------------------
| Rounding precision for 80-bit extended double-precision floating-point.
| Valid values are 32, 64, and 80.
*----------------------------------------------------------------------------*/
extern uint_fast8_t extF80_roundingPrecision;

/*----------------------------------------------------------------------------
| 80-bit extended double-precision floating-point operations.
*----------------------------------------------------------------------------*/
#ifdef SOFTFLOAT_FAST_INT64
uint_fast32_t extF80_to_ui32( extFloat80_t, uint_fast8_t, bool );
uint_fast64_t extF80_to_ui64( extFloat80_t, uint_fast8_t, bool );
int_fast32_t extF80_to_i32( extFloat80_t, uint_fast8_t, bool );
int_fast64_t extF80_to_i64( extFloat80_t, uint_fast8_t, bool );
uint_fast32_t extF80_to_ui32_r_minMag( extFloat80_t, bool );
uint_fast64_t extF80_to_ui64_r_minMag( extFloat80_t, bool );
int_fast32_t extF80_to_i32_r_minMag( extFloat80_t, bool );
int_fast64_t extF80_to_i64_r_minMag( extFloat80_t, bool );
float32_t extF80_to_f32( extFloat80_t );
float64_t extF80_to_f64( extFloat80_t );
float128_t extF80_to_f128( extFloat80_t );
extFloat80_t extF80_roundToInt( extFloat80_t, uint_fast8_t, bool );
extFloat80_t extF80_add( extFloat80_t, extFloat80_t );
extFloat80_t extF80_sub( extFloat80_t, extFloat80_t );
extFloat80_t extF80_mul( extFloat80_t, extFloat80_t );
extFloat80_t extF80_div( extFloat80_t, extFloat80_t );
extFloat80_t extF80_rem( extFloat80_t, extFloat80_t );
extFloat80_t extF80_sqrt( extFloat80_t );
bool extF80_eq( extFloat80_t, extFloat80_t );
bool extF80_le( extFloat80_t, extFloat80_t );
bool extF80_lt( extFloat80_t, extFloat80_t );
bool extF80_eq_signaling( extFloat80_t, extFloat80_t );
bool extF80_le_quiet( extFloat80_t, extFloat80_t );
bool extF80_lt_quiet( extFloat80_t, extFloat80_t );
bool extF80_isSignalingNaN( extFloat80_t );
#endif
uint_fast32_t extF80M_to_ui32( const extFloat80_t *, uint_fast8_t, bool );
uint_fast64_t extF80M_to_ui64( const extFloat80_t *, uint_fast8_t, bool );
int_fast32_t extF80M_to_i32( const extFloat80_t *, uint_fast8_t, bool );
int_fast64_t extF80M_to_i64( const extFloat80_t *, uint_fast8_t, bool );
uint_fast32_t extF80M_to_ui32_r_minMag( const extFloat80_t *, bool );
uint_fast64_t extF80M_to_ui64_r_minMag( const extFloat80_t *, bool );
int_fast32_t extF80M_to_i32_r_minMag( const extFloat80_t *, bool );
int_fast64_t extF80M_to_i64_r_minMag( const extFloat80_t *, bool );
float32_t extF80M_to_f32( const extFloat80_t * );
float64_t extF80M_to_f64( const extFloat80_t * );
void extF80M_to_f128M( const extFloat80_t *, float128_t * );
void
 extF80M_roundToInt(
     const extFloat80_t *, uint_fast8_t, bool, extFloat80_t * );
void extF80M_add( const extFloat80_t *, const extFloat80_t *, extFloat80_t * );
void extF80M_sub( const extFloat80_t *, const extFloat80_t *, extFloat80_t * );
void extF80M_mul( const extFloat80_t *, const extFloat80_t *, extFloat80_t * );
void extF80M_div( const extFloat80_t *, const extFloat80_t *, extFloat80_t * );
void extF80M_rem( const extFloat80_t *, const extFloat80_t *, extFloat80_t * );
void extF80M_sqrt( const extFloat80_t *, extFloat80_t * );
bool extF80M_eq( const extFloat80_t *, const extFloat80_t * );
bool extF80M_le( const extFloat80_t *, const extFloat80_t * );
bool extF80M_lt( const extFloat80_t *, const extFloat80_t * );
bool extF80M_eq_signaling( const extFloat80_t *, const extFloat80_t * );
bool extF80M_le_quiet( const extFloat80_t *, const extFloat80_t * );
bool extF80M_lt_quiet( const extFloat80_t *, const extFloat80_t * );
bool extF80M_isSignalingNaN( const extFloat80_t * );

/*----------------------------------------------------------------------------
| 128-bit (quadruple-precision) floating-point operations.
*----------------------------------------------------------------------------*/
#ifdef SOFTFLOAT_FAST_INT64
uint_fast32_t f128_to_ui32( float128_t, uint_fast8_t, bool );
uint_fast64_t f128_to_ui64( float128_t, uint_fast8_t, bool );
int_fast32_t f128_to_i32( float128_t, uint_fast8_t, bool );
int_fast64_t f128_to_i64( float128_t, uint_fast8_t, bool );
uint_fast32_t f128_to_ui32_r_minMag( float128_t, bool );
uint_fast64_t f128_to_ui64_r_minMag( float128_t, bool );
int_fast32_t f128_to_i32_r_minMag( float128_t, bool );
int_fast64_t f128_to_i64_r_minMag( float128_t, bool );
float32_t f128_to_f32( float128_t );
float64_t f128_to_f64( float128_t );
extFloat80_t f128_to_extF80( float128_t );
float128_t f128_roundToInt( float128_t, uint_fast8_t, bool );
float128_t f128_add( float128_t, float128_t );
float128_t f128_sub( float128_t, float128_t );
float128_t f128_mul( float128_t, float128_t );
float128_t f128_mulAdd( float128_t, float128_t, float128_t );
float128_t f128_div( float128_t, float128_t );
float128_t f128_rem( float128_t, float128_t );
float128_t f128_sqrt( float128_t );
bool f128_eq( float128_t, float128_t );
bool f128_le( float128_t, float128_t );
bool f128_lt( float128_t, float128_t );
bool f128_eq_signaling( float128_t, float128_t );
bool f128_le_quiet( float128_t, float128_t );
bool f128_lt_quiet( float128_t, float128_t );
bool f128_isSignalingNaN( float128_t );
#endif
uint_fast32_t f128M_to_ui32( const float128_t *, uint_fast8_t, bool );
uint_fast64_t f128M_to_ui64( const float128_t *, uint_fast8_t, bool );
int_fast32_t f128M_to_i32( const float128_t *, uint_fast8_t, bool );
int_fast64_t f128M_to_i64( const float128_t *, uint_fast8_t, bool );
uint_fast32_t f128M_to_ui32_r_minMag( const float128_t *, bool );
uint_fast64_t f128M_to_ui64_r_minMag( const float128_t *, bool );
int_fast32_t f128M_to_i32_r_minMag( const float128_t *, bool );
int_fast64_t f128M_to_i64_r_minMag( const float128_t *, bool );
float32_t f128M_to_f32( const float128_t * );
float64_t f128M_to_f64( const float128_t * );
void f128M_to_extF80M( const float128_t *, extFloat80_t * );
void f128M_roundToInt( const float128_t *, uint_fast8_t, bool, float128_t * );
void f128M_add( const float128_t *, const float128_t *, float128_t * );
void f128M_sub( const float128_t *, const float128_t *, float128_t * );
void f128M_mul( const float128_t *, const float128_t *, float128_t * );
void
 f128M_mulAdd(
     const float128_t *, const float128_t *, const float128_t *, float128_t *
 );
void f128M_div( const float128_t *, const float128_t *, float128_t * );
void f128M_rem( const float128_t *, const float128_t *, float128_t * );
void f128M_sqrt( const float128_t *, float128_t * );
bool f128M_eq( const float128_t *, const float128_t * );
bool f128M_le( const float128_t *, const float128_t * );
bool f128M_lt( const float128_t *, const float128_t * );
bool f128M_eq_signaling( const float128_t *, const float128_t * );
bool f128M_le_quiet( const float128_t *, const float128_t * );
bool f128M_lt_quiet( const float128_t *, const float128_t * );
bool f128M_isSignalingNaN( const float128_t * );

#endif

