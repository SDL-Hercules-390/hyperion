
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3a, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015 The Regents of the University of
California.  All rights reserved.

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
Modifications:
 1) Saved rounded result with unbounded unbiased exponent to enable
    return of a scaled rounded result, required by many instructions.
 2) Added rounding mode softfloat_round_stickybit, which corresponds to
    IBM Round For Shorter precision (RFS).  (not coded yet)
 3) Added reporting of softfloat_flag_tiny when the result is tiny, even
    if the result is tiny and exact.
 4) Added reporting of softfloat_flag_incremented when the significand was
    increased in magnitude by rounding.

=============================================================================*/

#ifdef HAVE_PLATFORM_H 
#include "platform.h" 
#endif
#if !defined(false) 
#include <stdbool.h> 
#endif
#if !defined(int32_t) 
#include <stdint.h>             /* C99 standard integers */ 
#endif
#include "internals.h"
#include "softfloat.h"

float128_t
 softfloat_roundPackToF128(
     bool sign,
     int_fast32_t exp,
     uint_fast64_t sig64,
     uint_fast64_t sig0,
     uint_fast64_t sigExtra
 )
{
    uint_fast8_t roundingMode;
    bool roundNearEven, doIncrement, isTiny;
    struct uint128_extra sig128Extra;
    uint_fast64_t uiZ64, uiZ0;
    struct uint128 sig128;
    union ui128_f128 uZ;

#ifdef IBM_IEEE
    struct uint128_extra savesig;                   /* Savearea for rounded pre-underflow significand           */
    struct uint128 savesig2;                        /* Savearea for diagnostics                                 */
    bool saveincre;                                 /* Savearea for incremented status of rounded significand   */
#endif /* IBM_IEEE */

    roundingMode = softfloat_roundingMode;
    roundNearEven = (roundingMode == softfloat_round_near_even);
    doIncrement = (UINT64_C( 0x8000000000000000 ) <= sigExtra);
    if ( ! roundNearEven && (roundingMode != softfloat_round_near_maxMag) ) {
        doIncrement =
            (roundingMode
                 == (sign ? softfloat_round_min : softfloat_round_max))
                && sigExtra;
    }

#ifdef IBM_IEEE
    if (doIncrement) {
        sig128 = softfloat_add128(sig64, sig0, 0, 1);
        savesig.v.v64 = sig128.v64;
        savesig.v.v0  =
            sig128.v0
            & ~(uint64_t)
            (!(sigExtra & UINT64_C(0x7FFFFFFFFFFFFFFF))
                & roundNearEven);
        savesig.extra = 0;
    }
    else {
        savesig.v.v64 = sig64;
        savesig.v.v0 = sig0;
        savesig.extra = sigExtra;
    }
    /* Sticky bit rounding: if pre-rounding result is exact, no rounding.  Leave result unchanged.      */
    /* If the result is inexact (sigExtra non-zero), the low-order bit of the result must be odd.       */
    /* Or'ing in a one-in the low-order bit achieves this.  If it was not already a 1, it will be.      */
    savesig.v.v0 |= (uint_fast64_t)(sigExtra && (roundingMode == softfloat_round_stickybit));
    saveincre = softfloat_lt128(sig64, sig0, savesig.v.v64, savesig.v.v0);  /* save incremented status of rounded result   */

    softfloat_raw.Exp   = exp - 16382;               /* Save rounded result for later scaling                    */
    savesig2 = softfloat_shortShiftLeft128(savesig.v.v64, savesig.v.v0, 14);
    softfloat_raw.Sig64 = savesig2.v64;
    softfloat_raw.Sig0  = savesig2.v0 | sigExtra >> 50;    /* 64 - 14, include upper 14 bits of Extra in rawsig0 */
    softfloat_raw.Sign  = sign;                      /* Save sign                                                */
    softfloat_raw.Incre = saveincre;                 /* save rounding incremented status                         */
    softfloat_raw.Inexact = sigExtra;                /* Save inexact status of raw result                        */
    softfloat_raw.Tiny = false;                      /* assume not a tiny result                                 */
    isTiny = false;                                 /* Assume not a subnormal result for the moment.            */

    /* ************************************************************************************ */
    /* At this point, savesig has the rounded result, and sigSign, sigExp, sig64, sig0,     */
    /* and sigExtra are the caller's unmodified parameters.                                 */
    /* ************************************************************************************ */

#endif  /*   IBM_IEEE   */

    if ( 0x7FFD <= (uint32_t) exp ) {
        if ( exp < 0 ) {
            isTiny =
                   (softfloat_detectTininess
                        == softfloat_tininess_beforeRounding)
                || (exp < -1)
                || ! doIncrement
                || softfloat_lt128(
                       sig64,
                       sig0,
                       UINT64_C( 0x0001FFFFFFFFFFFF ),
                       UINT64_C( 0xFFFFFFFFFFFFFFFF )
                   );
            sig128Extra =
                softfloat_shiftRightJam128Extra( sig64, sig0, sigExtra, -exp );
            sig64 = sig128Extra.v.v64;
            sig0  = sig128Extra.v.v0;
            sigExtra = sig128Extra.extra;
            exp = 0;
#ifdef IBM_IEEE
            /* ************************************************************************************ */
            /* At this point, savesig has the rounded result, and sigSign, sigExp, sig64, sig0,     */
            /* and sigExtra contain the caller's parameters.  The caller's exponent is less than    */
            /* zero, and sig64, sig0, and sigExtra have been denormallized and the exponent set to  */
            /* zero                                                                                 */
            /* ************************************************************************************ */

            softfloat_raw.Tiny = isTiny;
            softfloat_exceptionFlags |= softfloat_flag_tiny;
#endif /* IBM_IEEE  */
            if ( isTiny && sigExtra ) {
                softfloat_raiseFlags( softfloat_flag_underflow );
            }
            doIncrement = (UINT64_C( 0x8000000000000000 ) <= sigExtra);
            if (
                   ! roundNearEven
                && (roundingMode != softfloat_round_near_maxMag)
            ) {
                doIncrement =
                    (roundingMode
                         == (sign ? softfloat_round_min : softfloat_round_max))
                        && sigExtra;
            }
        } else if (
               (0x7FFD < exp)
            || ((exp == 0x7FFD)
                    && softfloat_eq128( 
                           sig64,
                           sig0,
                           UINT64_C( 0x0001FFFFFFFFFFFF ),
                           UINT64_C( 0xFFFFFFFFFFFFFFFF )
                       )
                    && doIncrement)
        ) {
            softfloat_raiseFlags(
                softfloat_flag_overflow | softfloat_flag_inexact );
            if (
                   roundNearEven
                || (roundingMode == softfloat_round_near_maxMag)
                || (roundingMode
                        == (sign ? softfloat_round_min : softfloat_round_max))
            ) {
                uiZ64 = packToF128UI64( sign, 0x7FFF, 0 );
                uiZ0  = 0;
            } else {
                uiZ64 =
                    packToF128UI64(
                        sign, 0x7FFE, UINT64_C( 0x0000FFFFFFFFFFFF ) );
                uiZ0 = UINT64_C( 0xFFFFFFFFFFFFFFFF );
            }
            goto uiZ;
        }
    }
    if ( sigExtra ) softfloat_exceptionFlags |= softfloat_flag_inexact;

#ifdef IBM_IEEE
    if ( doIncrement && isTiny ) {
        sig128 = softfloat_add128( sig64, sig0, 0, 1 );   /* increment subnormal result                     */
        sig128.v0 &= ~(uint64_t)(! (sigExtra & UINT64_C( 0x7FFFFFFFFFFFFFFF )) & roundNearEven);
        /* Sticky bit rounding: if pre-rounding result is exact, no rounding.  Leave result unchanged.      */
        /* If the result is inexact (sigExtra non-zero), the low-order bit of the result must be odd.       */
        /* Or'ing in a one-in the low-order bit achieves this.  If it was not already a 1, it will be.      */
        sig128.v0 |= (uint_fast64_t)(sigExtra && (roundingMode == softfloat_round_stickybit));
        saveincre = softfloat_lt128(sig64, sig0, sig128.v64, sig128.v0);  /* save incremented status of rounded result   */
        sig64 = sig128.v64;
        sig0 = sig128.v0;
    } else if (!isTiny) {
        sig64 = savesig.v.v64;
        sig0 = savesig.v.v0;
    }
    if (!(sig64 | sig0)) exp = 0;

    softfloat_exceptionFlags |= saveincre ? softfloat_flag_incremented : 0;

#else   /* not defined IBM_IEEE  */
    if ( doIncrement ) {
        sig128 = softfloat_add128( sig64, sig0, 0, 1 );
        sig64 = sig128.v64;
        sig0 =
            sig128.v0
                & ~(uint64_t)
                       (! (sigExtra & UINT64_C( 0x7FFFFFFFFFFFFFFF ))
                            & roundNearEven);
    } else {
        if ( ! (sig64 | sig0) ) exp = 0;
    }
#endif  /* IBM_IEEE  */

    uiZ64 = packToF128UI64( sign, exp, sig64 );
    uiZ0  = sig0;

 uiZ:
    uZ.ui.v64 = uiZ64;
    uZ.ui.v0  = uiZ0;
    return uZ.f;

}

