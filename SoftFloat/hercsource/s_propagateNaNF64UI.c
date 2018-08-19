
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

/*============================================================================
Modifications to comply with IBM IEEE Binary Floating Point, as defined
in the z/Architecture Principles of Operation, SA22-7832-10, by
Stephen R. Orso.  Said modifications identified by compilation conditioned
on preprocessor variable IBM_IEEE.
All such modifications placed in the public domain by Stephen R. Orso
Modifications:
 1) Replaced NaN propagation rules with those required to conform to
    SA22-7832-10.  Multiple figures describe these rules; see Figure 19-13
    on page 19-13 for the description for the Add instruction.
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

/*----------------------------------------------------------------------------
| Interpreting `uiA' and `uiB' as the bit patterns of two 64-bit floating-
| point values, at least one of which is a NaN, returns the bit pattern of
| the combined NaN result.  If either `uiA' or `uiB' has the pattern of a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

#ifdef IBM_IEEE
/*----------------------------------------------------------------------------
| Note: Above comment is incorrect whether compiled for the IBM_IEEE case
| or the default case.  Under no circumstances are the payloads combined.
| Only one of the two payloads is propagated.
*----------------------------------------------------------------------------*/
#endif

uint_fast64_t
 softfloat_propagateNaNF64UI( uint_fast64_t uiA, uint_fast64_t uiB )
{
#ifdef IBM_IEEE     /* IBM NaN propagation rules defined (consistently) in many tables      */
                    /* in SA22-7832-10; see for example Table 19-13 on page 19-16.          */
                    /* In short:                                                            */
                    /*    If A is SNaN, return QNaN(A)                                      */
                    /*    If B is SNaN, return QNaN(B)                                      */
                    /*    If A is QNaN, return A                                            */
                    /*    If B is QNaN, return B                                            */

    bool isSigNaNA;
    bool isSigNaNB;

    isSigNaNA = softfloat_isSigNaNF64UI(uiA);
    isSigNaNB = softfloat_isSigNaNF64UI(uiB);
    if (isSigNaNA || isSigNaNB)
        softfloat_raiseFlags(softfloat_flag_invalid);
    if (isSigNaNA) return uiA | UINT64_C(0x0008000000000000);
    if (isSigNaNB) return uiB | UINT64_C(0x0008000000000000);
    return (isNaNF64UI(uiA) ? uiA : uiB) | UINT64_C(0x0008000000000000);

#else
    bool isSigNaNA;

    isSigNaNA = softfloat_isSigNaNF64UI( uiA );
    if ( isSigNaNA || softfloat_isSigNaNF64UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        if ( isSigNaNA ) return uiA | UINT64_C( 0x0008000000000000 );
    }
    return (isNaNF64UI( uiA ) ? uiA : uiB) | UINT64_C( 0x0008000000000000 );
#endif

}

