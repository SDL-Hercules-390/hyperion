
/*============================================================================

This C source file f64_scaledResult.c is written by Stephen R. Orso.

Copyright 2016 by Stephen R Orso.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided  with the distribution.

3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

DISCLAMER: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

/*============================================================================
This function retrieves the precision-rounded result with unbounded exponent
from the Softfloat global state variables and:
1) Biases the exponent correctly for the target precision
2) Applies a caller-provided scale factor to the exponent
3) Returns in the correct precision the scaled precision-rounded result

This function is written to support the requirement, present in the 1985
version of the IEEE 754 floating point standard and in the IBM z/Architecture
Principles of Operation, SA22-7832-10, to return a scaled result when a
trappable IEEE Overflow or Underflow exception occurs during selected
arithemetic operations.

This function is also used to return a scaled result on trappable IEEE
Overflow or Underflow exceptions when converting from longer to shorter
floating point, as required by SA22-7832-10.

If the scaled result is to large to fit in the target precision, an SNaN
is generated with payload 0x0DEAD.
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
#include "specialize.h"
#include "softfloat.h"

float128_t  f128_scaledResult(int_fast16_t scale)
{
    int_fast32_t exp;
    union ui128_f128 uZ;
    struct exp32_sig128 z;
    struct uint128 uA;

    exp = softfloat_raw.Exp + 16382 + scale;

    /* Note that the high-order bit of the saved significand is the units position.  This is effectively    */
    /* added to the exponent in packToF128UI64 so that the units position disappears and the exponent is    */
    /* incremented.  So going in, the exponent limit must be one less than the float128 maximum of 0x7FFE.  */

    /* Note also that, unlike the C << operator, the function softfloat_shortShiftLeft128 behaves very      */
    /* badly when passed a shift count of zero.  When passed zero, softfloat_shortShiftLeft128 or's the     */
    /* the second half of the 128-bit significand into the first.  Not a bug, really, because this          */
    /* function is not a part of the Softfloat 3a API, but troubling nontheless.  Softfloat 3b did not      */
    /* change this behavior.                                                                                */

    if (exp < 0 || exp > 0x7FFD)
    {
        uZ.ui.v64 = (defaultNaNF128UI64 & ~UINT64_C(0x0000800000000000)) | UINT64_C(0x00000DEAD0000000);
        uZ.ui.v0 = defaultNaNF128UI0;
    }
    else
    {
        uA = softfloat_shortShiftRight128(softfloat_raw.Sig64, softfloat_raw.Sig0, 14);
        if (softfloat_raw.Tiny && (uA.v64 < 0x0001000000000000ULL))          /* result an unnormallized subnormal?  */
        {
            z = softfloat_normSubnormalF128Sig(uA.v64, uA.v0);
            exp += (z.exp - 1);
            uZ.ui.v64 = packToF128UI64(softfloat_raw.Sign, exp, z.sig.v64);  /* should not be needed  */
            uZ.ui.v0 = z.sig.v0;
        }
        else
        {
            uZ.ui.v64 = packToF128UI64(softfloat_raw.Sign, exp, uA.v64); 
            uZ.ui.v0 = uA.v0;
        }
    }

    softfloat_exceptionFlags &= ~(softfloat_flag_inexact | softfloat_flag_incremented | softfloat_flag_tiny);
    softfloat_exceptionFlags |= (softfloat_raw.Inexact ?  softfloat_flag_inexact     : 0) |
                                (softfloat_raw.Incre   ?  softfloat_flag_incremented : 0);

    return uZ.f;

}
