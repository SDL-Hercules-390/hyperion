/* CMPSCPUT.H   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Put Next Index Functions            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CMPSCPUT_H_
#define _CMPSCPUT_H_    // Code to be compiled ONLY ONCE goes after here

#include "cmpscmem.h"   // (need MEMBLK)

///////////////////////////////////////////////////////////////////////////////
// COMPRESSION: Put Next output DST Index
//
// Returns:  TRUE if there is no more room remaining in the DST buffer for
//           any more index values (i.e. it's TRUE that we HAVE reached the
//           end of our buffer), otherwise FALSE (i.e. there is still room
//           remaining in the DST buffer for additional index values).

struct PIBLK                // PutIndex parameters block
{
    CMPSCBLK*  pCMPSCBLK;   // Ptr to CMPSC instruction control block.
                            // pOp1, nLen1, and cbn updated on success.
    MEMBLK*    pMEMBLK;     // Ptr to operand-1 memory control block
    void**     ppPutIndex;  // Ptr to PutIndex function pointer.
    U16        index;       // U16 index value to be written to DST.
};
typedef struct PIBLK PIBLK; // Returns TRUE/FALSE (no more room/room remains)

typedef U8 (CMPSC_FASTCALL PutIndex)( PIBLK* pPIBLK );

///////////////////////////////////////////////////////////////////////////////
// Function to derive CBN value based on PutIndex function pointer

typedef U8 (CMPSC_FASTCALL PutGetCBN)( PutIndex* pPutIndex );

///////////////////////////////////////////////////////////////////////////////
#endif // _CMPSCPUT_H_     // Place all 'ARCH_DEP' code after this statement

extern PutGetCBN* ARCH_DEP( PutGetCBNTab    )[5];  // (ptrs to PutGetCBN table by CDSS)
extern PutIndex** ARCH_DEP( PutIndexCDSSTab )[5];  // (ptrs to CBN jump tables by CDSS)
extern PutIndex*  ARCH_DEP( PutIndexCBNTab  )[8];  // (ptrs to PutIndex functions by CBN)

///////////////////////////////////////////////////////////////////////////////
