
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3a, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014 The Regents of the University of California.
All Rights Reserved.

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
 1) Added fields to the global state to store a rounded result with unbounded 
    unbiased exponent to enable return of a scaled rounded result. 
=============================================================================*/

#ifdef HAVE_PLATFORM_H 
#include "platform.h" 
#endif
#if !defined(int32_t) 
#include <stdint.h>             /* C99 standard integers */ 
#endif
#include "internals.h"
#include "specialize.h"
#include "softfloat.h"

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

SF_THREAD_LOCAL softfloat_raw_t softfloat_raw;          /* Scaled results structure from softfloat.h            */

SF_THREAD_LOCAL uint_fast8_t softfloat_roundingMode = softfloat_round_near_even;
uint_fast8_t softfloat_detectTininess = init_detectTininess;
SF_THREAD_LOCAL uint_fast8_t softfloat_exceptionFlags;

#else
uint_fast8_t softfloat_roundingMode = softfloat_round_near_even;
uint_fast8_t softfloat_detectTininess = init_detectTininess;
uint_fast8_t softfloat_exceptionFlags = 0;
#endif /* IBM_IEEE  */

uint_fast8_t extF80_roundingPrecision = 80;

