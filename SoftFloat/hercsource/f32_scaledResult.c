
/*============================================================================

This C source file f32_scaledResult.c is written by Stephen R. Orso.

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

This function is not used to return a scaled result on trappable IEEE 
Overflow or Underflow exceptions when converting from longer to shorter 
floating point because that scaled result is always in the longer format;
there is (currently) no shorter format than float32_t.

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

float32_t
f32_scaledResult(int_fast16_t scale)
{
    int_fast32_t exp;
    union ui32_f32 uZ;
#ifdef IBM_IEEE
    struct exp16_sig32 z;
#endif  /* IBM_IEEE */

    exp = softfloat_raw.Exp + 126 + scale;

    /* Note that the high-order bit of the saved significand is the units position.  This is effectively    */
    /* added to the exponent in packToF32UI so that the units position disappears and the exponent is       */
    /* incremented.  So going in, the exponent limit must be one less than the float32 maximum of 0xFE.     */

    if (exp < 0 || exp > 0xFD)
        uZ.ui = (defaultNaNF32UI & ~UINT32_C(0x00400000)) | UINT32_C(0x0000DEAD);  /* Create SNaN 'DEAD'   */
    else
        if (softfloat_raw.Sig64 < 0x0400000000000000ULL)          /* result a subnormal?  */
        {
            z = softfloat_normSubnormalF32Sig((uint_fast32_t)(softfloat_raw.Sig64 >> 39));
            exp += z.exp - 1;
            uZ.ui = packToF32UI(softfloat_raw.Sign, exp, z.sig);
        }
        else
            uZ.ui = packToF32UI(softfloat_raw.Sign, exp, (uint_fast32_t)(softfloat_raw.Sig64 >> 39));

    softfloat_exceptionFlags &= ~(softfloat_flag_inexact | softfloat_flag_incremented);
    softfloat_exceptionFlags |= (softfloat_raw.Inexact ?  softfloat_flag_inexact     : 0) | 
                                (softfloat_raw.Incre   ?  softfloat_flag_incremented : 0);

    return uZ.f;

}
