
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
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

#if  1  /* includes define for IBM_IEEE  */
#include "softfloat.h"
#endif /* IBM_IEEE */

#include "specialize.h"

float128_t
 softfloat_addMagsF128(
     uint_fast64_t uiA64,
     uint_fast64_t uiA0,
     uint_fast64_t uiB64,
     uint_fast64_t uiB0,
     bool signZ
 )
{
    int_fast32_t expA;
    struct uint128 sigA;
    int_fast32_t expB;
    struct uint128 sigB;
    int_fast32_t expDiff;
    struct uint128 uiZ, sigZ;
    int_fast32_t expZ;
    uint_fast64_t sigZExtra;
    struct uint128_extra sig128Extra;
    union ui128_f128 uZ;

    expA = expF128UI64( uiA64 );
    sigA.v64 = fracF128UI64( uiA64 );
    sigA.v0  = uiA0;
    expB = expF128UI64( uiB64 );
    sigB.v64 = fracF128UI64( uiB64 );
    sigB.v0  = uiB0;
    expDiff = expA - expB;
    if ( ! expDiff ) {
        if ( expA == 0x7FFF ) {
            if ( sigA.v64 | sigA.v0 | sigB.v64 | sigB.v0 ) goto propagateNaN;
            uiZ.v64 = uiA64;
            uiZ.v0  = uiA0;
            goto uiZ;
        }
        sigZ = softfloat_add128( sigA.v64, sigA.v0, sigB.v64, sigB.v0 );
        if ( ! expA ) {
#ifdef IBM_IEEE
            softfloat_raw.Incre = 0;                                 /* Result was not incremented                       */
            softfloat_raw.Inexact = 0;                               /* Result is not inexact                            */
            softfloat_raw.Sign = signZ;                              /* Save result sign                                 */
            if (!(sigZ.v64 & 0xFFFF000000000000ULL) && (sigZ.v64 || sigZ.v0)) {
                softfloat_exceptionFlags |= softfloat_flag_tiny;    /* nonzero result is tiny      */
                uiZ = softfloat_shortShiftLeft128(sigZ.v64, sigZ.v0, 14);
                softfloat_raw.Exp = -16382;                          /* Save semi-unbiased exponent                      */
                softfloat_raw.Sig64 = uiZ.v64;                       /* Save significand part 1                          */
                softfloat_raw.Sig0 = uiZ.v0;                         /* Save significand part 2                          */
                softfloat_raw.Tiny = true;                           /* indicate this is a subnormal for scaling         */
            }
            else {
                softfloat_raw.Exp = 0;                               /* Set saved raw value to zero                      */
                softfloat_raw.Sig64 = 0;
                softfloat_raw.Sig0 = 0;
                softfloat_raw.Tiny = false;                           /* indicate this is a subnormal for scaling         */
            }
#endif /* IBM_IEEE  */
            uiZ.v64 = packToF128UI64( signZ, 0, sigZ.v64 );
            uiZ.v0  = sigZ.v0;
            goto uiZ;
        }
        expZ = expA;
        sigZ.v64 |= UINT64_C( 0x0002000000000000 );
        sigZExtra = 0;
        goto shiftRight1;
    }
    if ( expDiff < 0 ) {
        if ( expB == 0x7FFF ) {
            if ( sigB.v64 | sigB.v0 ) goto propagateNaN;
            uiZ.v64 = packToF128UI64( signZ, 0x7FFF, 0 );
            uiZ.v0  = 0;
            goto uiZ;
        }
        expZ = expB;
        if ( expA ) {
            sigA.v64 |= UINT64_C( 0x0001000000000000 );
        } else {
            ++expDiff;
            sigZExtra = 0;
            if ( ! expDiff ) goto newlyAligned;
        }
        sig128Extra =
            softfloat_shiftRightJam128Extra( sigA.v64, sigA.v0, 0, -expDiff );
        sigA = sig128Extra.v;
        sigZExtra = sig128Extra.extra;
    } else {
        if ( expA == 0x7FFF ) {
            if ( sigA.v64 | sigA.v0 ) goto propagateNaN;
            uiZ.v64 = uiA64;
            uiZ.v0  = uiA0;
            goto uiZ;
        }
        expZ = expA;
        if ( expB ) {
            sigB.v64 |= UINT64_C( 0x0001000000000000 );
        } else {
            --expDiff;
            sigZExtra = 0;
            if ( ! expDiff ) goto newlyAligned;
        }
        sig128Extra =
            softfloat_shiftRightJam128Extra( sigB.v64, sigB.v0, 0, expDiff );
        sigB = sig128Extra.v;
        sigZExtra = sig128Extra.extra;
    }
 newlyAligned:
    sigZ =
        softfloat_add128(
            sigA.v64 | UINT64_C( 0x0001000000000000 ),
            sigA.v0,
            sigB.v64,
            sigB.v0
        );
    --expZ;
    if ( sigZ.v64 < UINT64_C( 0x0002000000000000 ) ) goto roundAndPack;
    ++expZ;
 shiftRight1:
    sig128Extra =
        softfloat_shortShiftRightJam128Extra(
            sigZ.v64, sigZ.v0, sigZExtra, 1 );
    sigZ = sig128Extra.v;
    sigZExtra = sig128Extra.extra;
 roundAndPack:
    return
        softfloat_roundPackToF128( signZ, expZ, sigZ.v64, sigZ.v0, sigZExtra );
 propagateNaN:
    uiZ = softfloat_propagateNaNF128UI( uiA64, uiA0, uiB64, uiB0 );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;

}

