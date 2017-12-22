/* VSTORE.C     (c) Copyright Roger Bowler, 2000-2012                */
/*               ESA/390 Virtual Storage Functions                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (c) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (c) Copyright Jan Jaeger, 1999-2012      */

#include "hstdinc.h"

#ifndef _HENGINE_DLL_
#define _HENGINE_DLL_
#endif
#ifndef _VSTORE_C
#define _VSTORE_C
#endif

#include "hercules.h"

#if !defined(OPTION_INLINE_VSTORE) || !defined(OPTION_INLINE_IFETCH)

#include "opcode.h"
#include "inline.h"    /* automatically #includes dat.h and vstore.h */

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "vstore.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "vstore.c"
#endif

#endif /* !defined(_GEN_ARCH) */

#endif /* !defined(OPTION_INLINE_VSTORE) || !defined(OPTION_INLINE_IFETCH) */
