/* CMPSCGET.H   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Get Next Index Functions            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CMPSCGET_H_
#define _CMPSCGET_H_    // Code to be compiled ONLY ONCE goes after here

#include "cmpscmem.h"   // (need MEMBLK)

///////////////////////////////////////////////////////////////////////////////
// EXPANSION: Get Next input SRC Index
//
// Returns:  Number of SRC bytes consumed, or '0' if the
//           end of SRC data i/p buffer has been reached.

struct GIBLK                // GetIndex parameters block
{
    CMPSCBLK*  pCMPSCBLK;   // Ptr to CMPSC instruction control block
    MEMBLK*    pMEMBLK;     // Ptr to operand-2 memory control block
    U16*       pIndex;      // Ptr to returned U16 index value(s).
    void**     ppGetIndex;  // Ptr to GetIndex function pointer.
};
typedef struct GIBLK GIBLK; // Returns: #of SRC bytes consumed, or 0
                            // if end of SRC data i/p buffer reached.

typedef U8 (CMPSC_FASTCALL GetIndex)( GIBLK* pGIBLK );

///////////////////////////////////////////////////////////////////////////////
#endif // _CMPSCGET_H_    // Place all 'ARCH_DEP' code after this statement

extern GetIndex** ARCH_DEP( GetIndexCDSSTab )[5];   // (ptrs to CBN jump tables by CDSS)
extern GetIndex*  ARCH_DEP( GetIndexCBNTab  )[8];   // (ptrs to GetIndex functions by CBN)

extern GetIndex** ARCH_DEP( Get8IndexCDSSTab )[5];  // (ptrs to CBN jump tables by CDSS)
extern GetIndex*  ARCH_DEP( Get8IndexCBNTab  )[8];  // (ptrs to Get8Index functions by CBN)

///////////////////////////////////////////////////////////////////////////////
