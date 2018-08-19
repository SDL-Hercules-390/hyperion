
/*============================================================================

This C source file softfloat_stdtypes.h is written by Stephen R. Orso.

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

/*----------------------------------------------------------------------------
| This header is at once a part of the public header set for SoftFloat-3a For
| Hercules (S3FH) and a required component to build the S3FH static library.
| When building S3FH, this header is included by platform.h.  It is also
| included by softfloat-types.h, one of the public headers for S3FH.
|
| The base S3FH package requires stdint.h (a C99-standard header), and the
| documentation suggests altering things if SoftFloat-3a is being comiled
| on a pre-C99 system, which includes Windows systems with versions of
| Visual Studio earlier than 2015.
|
| This header includes stdbool.h and stdint.h if the respective macros
| HAVE_STDINT_H and HAVE_STDBOOL_H are defined and expects those headers
| to define the types and macros for integer, fast_integer, least_integer,
| and boolean types.
|
| If HAVE_STDBOOL_H is undefined, this header defines the needed boolean
| types and macros.
|
| If HAVE_STDINT_H is undefined, this header defines the int_fast and
| int_least macros and tests HAVE_INTTYPES_H.  If that macro is defined,
| this header #includes it, otherwise it defines the integer types required
| to build S3FH.
*----------------------------------------------------------------------------*/


#if !defined(_SF_STDTYPES)
#define _SF_STDTYPES

#if defined(HAVE_STDBOOL_H)
#  include <stdbool.h>
#elif !defined(_SF_STDBOOL)
#  define _SF_STDBOOL
#  define false   0
#  define true    1
#  define bool unsigned char
#endif /* #if defined(HAVE_STDBOOL_H) */


/*----------------------------------------------------------------------------
| The 'fast' and 'least' types defined below match those in the stdint.h
| header shipped with Debian 9.  The "normal" integer types int.._t match
| those included in hstdint.h when compiling under MSVC.  Note that while
| MSVC 2015 does include stdint.h, the Windows build for Hercules does not
| include it and instead defines its own integer types in hstdint.h.  Prior
| versions of MSVC did not include stdint.h, and no version of MSVC includes
| inttypes.h.  This means that:
|  - The 'fast' and 'least' types must match those in Windows stdint.h because
|    if Windows has stdint.h (VS 2015 or better), SoftFloat will compile with
|    MSVC stdint.h and Hercules will compile the SoftFloat public headers with
|    the definitions below.
|  - The standard integer types must match those in Hercules 
*----------------------------------------------------------------------------*/

#if defined(HAVE_STDINT_H)
#  include <stdint.h>

#else
#  if !defined(_SF_FASTLEASTTYPES)
#    define _SF_FASTLEASTTYPES
     typedef          char          int_fast8_t;
     typedef          long int      int_fast16_t;
     typedef          long int      int_fast32_t;
     typedef          long long int int_fast64_t;
     typedef unsigned char          uint_fast8_t;
     typedef unsigned long int      uint_fast16_t;
     typedef unsigned long int      uint_fast32_t;
     typedef unsigned long long int uint_fast64_t;
     typedef unsigned char          uint_least8_t;
#    define INT32_C(c)              c ## L
#    define UINT32_C(c)             c ## UL
#    define INT64_C(c)              c ## LL
#    define UINT64_C(c)             c ## ULL
#  endif  /* !defined(_SF_FASTTYPES)  */

#  if defined(HAVE_INTTYPES_H)
#    include <inttypes.h>
#  else
#    if !defined(_SF_INTTYPES)
#      define _SF_INTTYPES
#      if !defined(_MSC_VER)
         typedef          short int     int16_t;
         typedef          int           int32_t;
         typedef          long long int int64_t;
         typedef unsigned short int     uint16_t;
         typedef unsigned int           uint32_t;
         typedef unsigned long long int uint64_t;
#      else
         typedef unsigned __int16       uint16_t;
         typedef unsigned __int32       uint32_t;
         typedef unsigned __int64       uint64_t;
         typedef signed   __int16       int16_t;
         typedef signed   __int32       int32_t;
         typedef signed   __int64       int64_t;
#      endif  /*  !defined(_MSC_VER)*/
#    endif  /*  !defined(_SF_INTTYPES)*/
#  endif /* #ifndef HAVE_INTTYPES_H */

#endif  /* defined(HAVE_STDINT_H  */

/* #define int32_t int32_t  */

#endif /* if !defined(_SF_STDTYPES)  */