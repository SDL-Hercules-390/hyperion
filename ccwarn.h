/* CCWARN.H   (C) Copyright "Fish" (David B. Trout), 2018-2019       */
/*             Re-ENABLE (un-suppress) some compiler warnings        */
/*                                                                   */
/*             Released under "The Q Public License Version 1"       */
/*             (http://www.hercules-390.org/herclic.html)            */
/*             as modifications to Hercules.                         */

/*  This header re-enables again some of the compiler warnings       */
/*  that were suppressed by the 'ccnowarn.h' header or elsewhere.    */

#ifndef _CCWARN_H_
#define _CCWARN_H_

#include "ccnowarn.h"       // (need the 'ENABLE_MSVC_WARNING' macro)

ENABLE_MSVC_WARNING( 4146 ) // "unary minus operator applied to unsigned type, result still unsigned"
ENABLE_MSVC_WARNING( 4244 ) // "conversion from 'x' to 'y', possible loss of data"
ENABLE_MSVC_WARNING( 4254 ) // "conversion from 'x' to 'y', possible loss of data"
ENABLE_MSVC_WARNING( 4267 ) // "conversion from size_t to int possible loss of data"
ENABLE_MSVC_WARNING( 4287 ) // "unsigned/negative constant mismatch"
ENABLE_MSVC_WARNING( 4296 ) // "expression is always false"
ENABLE_MSVC_WARNING( 4555 ) // "expression has no effect; expected expression with side-effect"

#endif /* _CCWARN_H_ */
