/* MEMRCHR.C    (C) Copyright Roger Bowler, 2006-2012                */
/*              Hercules Right to Left memory scan                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _MEMRCHR_C_
#define _HUTIL_DLL_

#include "hercules.h"

#if !defined( HAVE_MEMRCHR )

#include "memrchr.h"

DLL_EXPORT void *memrchr(const void *buf, int c, size_t num)
{
   unsigned char *pMem;
   if (num == 0)
   {
      return NULL;
   }
   for (pMem = (unsigned char *) buf + num - 1; pMem >= (unsigned char *) buf; pMem--)
   {
      if (*pMem == (unsigned char) c) break;
   }
   if (pMem >= (unsigned char *) buf)
   {
      return ((void *) pMem);
   }
   return NULL;
}

#endif // !defined(HAVE_MEMRCHR)
