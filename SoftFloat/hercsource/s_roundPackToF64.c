
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
    IBM Round For Shorter precision (RFS).
 3) Added reporting of softfloat_flag_tiny when the result is tiny, even
    if the result is tiny and exact.
 4) Added reporting of softfloat_flag_incremented when the significand was
    increased in magnitude by rounding.
=============================================================================*/

#include "platform.h" 
#include "internals.h"
#include "softfloat.h"

float64_t
 softfloat_roundPackToF64( bool sign, int_fast16_t exp, uint_fast64_t sig )
{
    uint_fast8_t roundingMode;
    bool roundNearEven;
    uint_fast16_t roundIncrement, roundBits;
    bool isTiny;
    uint_fast64_t uiZ;
    union ui64_f64 uZ;

#ifdef IBM_IEEE
    uint_fast64_t savesig;              /* Savearea for rounded pre-underflow significand   */
    bool saveincre;                     /* Savearea for incremented status                  */
#endif /* IBM_IEEE */

    roundingMode = softfloat_roundingMode;
    roundNearEven = (roundingMode == softfloat_round_near_even);
    roundIncrement = 0x200;
    if ( ! roundNearEven && (roundingMode != softfloat_round_near_maxMag) ) {
        roundIncrement =
            (roundingMode
                 == (sign ? softfloat_round_min : softfloat_round_max))
                ? 0x3FF
                : 0;
    }
    roundBits = sig & 0x3FF;

#ifdef IBM_IEEE
    savesig = (sig + roundIncrement) >> 10;
    /* Sticky bit rounding: if pre-rounding result is exact, no rounding.  Leave result unchanged.      */
    /* If the result is inexact (sigExtra non-zero), the low-order bit of the result must be odd.       */
    /* Or'ing in a one-in the low-order bit achieves this.  If it was not already a 1, it will be.      */
    savesig |=  (uint_fast64_t)(roundBits && (roundingMode == softfloat_round_stickybit));
    savesig &= ~(uint_fast64_t)(!(roundBits ^ 0x200) & roundNearEven);

    saveincre = (savesig << 10) > sig;         /* Save incremented status of raw rounded result            */

    softfloat_raw.Incre = saveincre;                     /* Save incremented status of rounding      */
    softfloat_raw.Sig64 = savesig << 10;                 /* Save rounded significand for scaling     */
    softfloat_raw.Sig0  = 0;                             /* Zero bits 64-128 of rounded result       */
    softfloat_raw.Exp   = exp - 1022;                    /* Save unbiased exponent                   */
    softfloat_raw.Sign  = sign;                          /* Save result sign                         */
    softfloat_raw.Inexact = roundBits != 0;              /* Save inexact status of raw result        */
    isTiny = false;                                      /* Assume not a subnormal result for the moment.            */
#endif /* IBM_IEEE */

    if ( 0x7FD <= (uint16_t) exp ) {
        if ( exp < 0 ) {
            isTiny =
                   (softfloat_detectTininess
                        == softfloat_tininess_beforeRounding)
                || (exp < -1)
                || (sig + roundIncrement < UINT64_C( 0x8000000000000000 ));
            sig = softfloat_shiftRightJam64( sig, -exp );
            exp = 0;
            roundBits = sig & 0x3FF;
#ifdef IBM_IEEE
            softfloat_exceptionFlags |= softfloat_flag_tiny;
#endif /* IBM_IEEE  */
            if ( isTiny && roundBits ) {
                softfloat_raiseFlags( softfloat_flag_underflow );
            }
        } else if (
            (0x7FD < exp)
                || (UINT64_C( 0x8000000000000000 ) <= sig + roundIncrement)
        ) {
            softfloat_raiseFlags(
                softfloat_flag_overflow | softfloat_flag_inexact );
            uiZ = packToF64UI( sign, 0x7FF, 0 ) - ! roundIncrement;
            goto uiZ;
        }
    }
    if ( roundBits ) softfloat_exceptionFlags |= softfloat_flag_inexact;

#ifdef IBM_IEEE
    /*  NB:  isTiny is always true on underflow because IBM IEEE requires detect tininess before rounding       */
    softfloat_raw.Tiny = isTiny;                         /* preserve Tiny flag for return of scaled results      */
    if (isTiny) {                                       /* if tiny, we must round the shifted subnormal         */
        savesig  =  (sig + roundIncrement) >> 10;          /* apply rounding factor                                */
        savesig |=  (uint_fast64_t)(roundBits && (roundingMode == softfloat_round_stickybit)); /* ensure odd if needed */
        savesig &= ~(uint_fast64_t)(!(roundBits ^ 0x200) & roundNearEven); /* ensure even if RNTE and a tie      */
        saveincre = (savesig << 10) > sig;         /* Save incremented status of raw rounded result            */
    }
    uiZ = packToF64UI(sign, savesig ? exp : 0, savesig); 
    softfloat_exceptionFlags |= saveincre ? softfloat_flag_incremented : 0;
#else
    sig = (sig + roundIncrement)>>10;
    sig &= ~(uint_fast64_t) (! (roundBits ^ 0x200) & roundNearEven);
    uiZ = packToF64UI(sign, sig ? exp : 0, sig); 
#endif  /* IBM_IEEE  */



 uiZ:
    uZ.ui = uiZ;
    return uZ.f;

}

