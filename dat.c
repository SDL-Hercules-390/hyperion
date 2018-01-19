/* DAT.C        (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules Supported DAT Functions                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

#include "hstdinc.h"

#ifndef _HENGINE_DLL_
#define _HENGINE_DLL_
#endif
#ifndef _DAT_C
#define _DAT_C
#endif

#include "hercules.h"

#if !defined(OPTION_INLINE_DAT) || !defined(OPTION_INLINE_LOGICAL)

#include "opcode.h"
#include "inline.h"    /* automatically #includes dat.h and vstore.h */

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "dat.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "dat.c"
#endif

#endif /*!defined(_GEN_ARCH)*/

#endif /* !defined(OPTION_INLINE_DAT) || !defined(OPTION_INLINE_LOGICAL) */
