basic fixes

relevant urls

for the general decimal arithmetic
http://speleotrove.com/decimal/

decNumber package documentation
http://speleotrove.com/decimal/decnumber.html

for the errata
http://speleotrove.com/decimal/decnumerr.html

quoting verbatim from the above
A decNumber user has reported that Visual Studio 2010 (32 bit) has difficulty with compiling the decNumber source files.
Here are his workarounds.
http://speleotrove.com/decimal/decnumVS.html

the workaround has been implemented using
quoting verbatim from the above

Method 2:
In file: "decBasic.c"
1. Change extension from .c to .h

2. Add the following at the beginning of the code:
#if !defined(DECBASIC)
#define DECBASIC

3. Add the following at the end of the code:
#endif

4. Replace the following line 2 lines
#error decBasic.c must be included after decCommon.c
#error Routines in decBasic.c are for decDouble and decQuad only
with
#error decBasic.h must be included after decCommon.h
#error Routines in decBasic.h are for decDouble and decQuad only
respectively.

In file "decCommon.c"
1. Change extension from .c to .h

2. Add the following at the beginning of the code:
#if !defined(DECCOMMON)
#define DECCOMMON
3. Add the following at the end of the code:
#endif

In the following files:
decQuad.c
decDouble.c
decSingle.c

replace lines:
#include "decCommon.c"
#include "decBasic.c"

with
#include "decCommon.h"
#include "decBasic.h"

as applicable.

References to decCommon.c and decBasic.c in commentary should also be modified to refer to the corresponding .h.

additional fixes

http://speleotrove.com/decimal/decnumerr.html
quoting verbatim from the above
In addition, an earlier fix (thought to have been applied in 3.67) is still missing in 3.68:
decDoubleIsSigned and decQuadIsSigned return incorrect result.
The documentation indicates that these functions return 1 if the number being tested is signed; however they return DECFLOAT_Sign in this case.

To fix, change the lines in decBasic.c:

uInt decFloatIsSigned(const decFloat *df) {
  return DFISSIGNED(df);
  }
to
uInt decFloatIsSigned(const decFloat *df) {
  return DFISSIGNED(df)!=0;
  }
Many thanks to Matthew Hagerty for finding this one, and to Rahul Ruikar for re-reporting it.
