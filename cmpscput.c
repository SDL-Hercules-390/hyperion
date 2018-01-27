/* CMPSCPUT.C   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Put Next Index Functions            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"    // (MUST be first #include in EVERY source file)

#define _CMPSCPUT_C_
#define _HENGINE_DLL_

#if !defined( NOT_HERC )        // (building Hercules?)
#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#endif
#include "cmpsc.h"              // (Master header)

#ifdef FEATURE_CMPSC

///////////////////////////////////////////////////////////////////////////////
// COMPRESSION: Put Next output DST Index
//
// Returns:  TRUE if there is no more room remaining in the DST buffer for
//           any more index values (i.e. it's TRUE that we HAVE reached the
//           end of our buffer), otherwise FALSE (i.e. there is still room
//           remaining in the DST buffer for additional index values).

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex009 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex109 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex209 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex309 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex409 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex509 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex609 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex709 )) ( PIBLK* pPIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex010 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex110 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex210 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex310 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex410 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex510 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex610 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex710 )) ( PIBLK* pPIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex011 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex111 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex211 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex311 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex411 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex511 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex611 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex711 )) ( PIBLK* pPIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex012 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex112 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex212 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex312 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex412 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex512 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex612 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex712 )) ( PIBLK* pPIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex013 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex113 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex213 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex313 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex413 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex513 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex613 )) ( PIBLK* pPIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex713 )) ( PIBLK* pPIBLK );

///////////////////////////////////////////////////////////////////////////////
// CDSS=1, 9 bits per index, CBN sequences:  0-1-2-3-4-5-6-7*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex009 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 7), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex109 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex109 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 6) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0x8000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex209 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex209 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 5) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xC000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex309 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex309 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 4) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xE000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex409 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex409 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 3) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex509 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex509 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF800), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex609 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex609 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFC00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex709 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex709 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index     ) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFE00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex009 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=2, 10 bits per index, CBN sequences:  0-2-4-6*,  1-3-5-*7*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex010 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 6), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex210 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex110 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 5) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0x8000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex310 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex210 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 4) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xC000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex410 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex310 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 3) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xE000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex510 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex410 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex610 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex510 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF800), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex710 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex610 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index     ) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFC00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex010 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex710 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFE00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 7), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex110 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=3, 11 bits per index, CBN sequences:  0-3-*6*-1-4-*7*-2-5*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex011 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 5), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex311 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex111 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 4) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0x8000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex411 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex211 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 3) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xC000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex511 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex311 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xE000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex611 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex411 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex711 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex511 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index     ) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF800), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex011 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex611 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFC00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 7), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex111 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex711 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFE00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 6), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex211 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=4, 12 bits per index, CBN sequences:  0-4*,  1-*5*,  2-*6*,  3-*7*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex012 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 4), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex412 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex112 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 3) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0x8000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex512 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex212 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xC000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex612 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex312 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xE000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex712 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex412 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index     ) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex012 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex512 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF800), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 7), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex112 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex612 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFC00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 6), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex212 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex712 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 3) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFE00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 5), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex312 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=5, 13 bits per index, CBN sequences:  0-*5*-2-*7*-*4*-1-*6*-3*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex013 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 3), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex513 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex113 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0x8000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex613 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex213 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index << 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xC000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 1;
    pPIBLK->pCMPSCBLK->nLen1  -= 1;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex713 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex313 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index     ) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xE000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex013 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex413 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 1) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF000), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 7), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex113 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex513 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 2) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xF800), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 6), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex213 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex613 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 3) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFC00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 5), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex313 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 2);
}
U8  (CMPSC_FASTCALL ARCH_DEP( PutIndex713 )) ( PIBLK* pPIBLK )
{
    store_op_hw( (pPIBLK->index >> 4) | (((U16)fetch_op_b( pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK ) << 8) & 0xFE00), pPIBLK->pCMPSCBLK->pOp1, pPIBLK->pMEMBLK );
    store_op_b( (pPIBLK->index << 4), pPIBLK->pCMPSCBLK->pOp1 + 2, pPIBLK->pMEMBLK );
    pPIBLK->pCMPSCBLK->pOp1   += 2;
    pPIBLK->pCMPSCBLK->nLen1  -= 2;
    *pPIBLK->ppPutIndex = ARCH_DEP( PutIndex413 );
    return (pPIBLK->pCMPSCBLK->nLen1 < 3);
}

///////////////////////////////////////////////////////////////////////////////
// Derive CBN value based on PutIndex function pointer: one for each CDSS

U8  (CMPSC_FASTCALL ARCH_DEP( PutGetCBN09 )) ( PutIndex* pPutIndex )
{
         if (&ARCH_DEP( PutIndex009 ) == pPutIndex) return 0;
    else if (&ARCH_DEP( PutIndex109 ) == pPutIndex) return 1;
    else if (&ARCH_DEP( PutIndex209 ) == pPutIndex) return 2;
    else if (&ARCH_DEP( PutIndex309 ) == pPutIndex) return 3;
    else if (&ARCH_DEP( PutIndex409 ) == pPutIndex) return 4;
    else if (&ARCH_DEP( PutIndex509 ) == pPutIndex) return 5;
    else if (&ARCH_DEP( PutIndex609 ) == pPutIndex) return 6;
    else                                return 7;
}

U8  (CMPSC_FASTCALL ARCH_DEP( PutGetCBN10 )) ( PutIndex* pPutIndex )
{
         if (&ARCH_DEP( PutIndex010 ) == pPutIndex) return 0;
    else if (&ARCH_DEP( PutIndex110 ) == pPutIndex) return 1;
    else if (&ARCH_DEP( PutIndex210 ) == pPutIndex) return 2;
    else if (&ARCH_DEP( PutIndex310 ) == pPutIndex) return 3;
    else if (&ARCH_DEP( PutIndex410 ) == pPutIndex) return 4;
    else if (&ARCH_DEP( PutIndex510 ) == pPutIndex) return 5;
    else if (&ARCH_DEP( PutIndex610 ) == pPutIndex) return 6;
    else                                return 7;
}

U8  (CMPSC_FASTCALL ARCH_DEP( PutGetCBN11 )) ( PutIndex* pPutIndex )
{
         if (&ARCH_DEP( PutIndex011 ) == pPutIndex) return 0;
    else if (&ARCH_DEP( PutIndex111 ) == pPutIndex) return 1;
    else if (&ARCH_DEP( PutIndex211 ) == pPutIndex) return 2;
    else if (&ARCH_DEP( PutIndex311 ) == pPutIndex) return 3;
    else if (&ARCH_DEP( PutIndex411 ) == pPutIndex) return 4;
    else if (&ARCH_DEP( PutIndex511 ) == pPutIndex) return 5;
    else if (&ARCH_DEP( PutIndex611 ) == pPutIndex) return 6;
    else                                return 7;
}

U8  (CMPSC_FASTCALL ARCH_DEP( PutGetCBN12 )) ( PutIndex* pPutIndex )
{
         if (&ARCH_DEP( PutIndex012 ) == pPutIndex) return 0;
    else if (&ARCH_DEP( PutIndex112 ) == pPutIndex) return 1;
    else if (&ARCH_DEP( PutIndex212 ) == pPutIndex) return 2;
    else if (&ARCH_DEP( PutIndex312 ) == pPutIndex) return 3;
    else if (&ARCH_DEP( PutIndex412 ) == pPutIndex) return 4;
    else if (&ARCH_DEP( PutIndex512 ) == pPutIndex) return 5;
    else if (&ARCH_DEP( PutIndex612 ) == pPutIndex) return 6;
    else                                return 7;
}

U8  (CMPSC_FASTCALL ARCH_DEP( PutGetCBN13 )) ( PutIndex* pPutIndex )
{
         if (&ARCH_DEP( PutIndex013 ) == pPutIndex) return 0;
    else if (&ARCH_DEP( PutIndex113 ) == pPutIndex) return 1;
    else if (&ARCH_DEP( PutIndex213 ) == pPutIndex) return 2;
    else if (&ARCH_DEP( PutIndex313 ) == pPutIndex) return 3;
    else if (&ARCH_DEP( PutIndex413 ) == pPutIndex) return 4;
    else if (&ARCH_DEP( PutIndex513 ) == pPutIndex) return 5;
    else if (&ARCH_DEP( PutIndex613 ) == pPutIndex) return 6;
    else                                return 7;
}

///////////////////////////////////////////////////////////////////////////////
// (by CBN)

PutIndex* ARCH_DEP( PutIndexTab09 )[8] = { ARCH_DEP( PutIndex009 ), ARCH_DEP( PutIndex109 ), ARCH_DEP( PutIndex209 ), ARCH_DEP( PutIndex309 ), ARCH_DEP( PutIndex409 ), ARCH_DEP( PutIndex509 ), ARCH_DEP( PutIndex609 ), ARCH_DEP( PutIndex709 ) };
PutIndex* ARCH_DEP( PutIndexTab10 )[8] = { ARCH_DEP( PutIndex010 ), ARCH_DEP( PutIndex110 ), ARCH_DEP( PutIndex210 ), ARCH_DEP( PutIndex310 ), ARCH_DEP( PutIndex410 ), ARCH_DEP( PutIndex510 ), ARCH_DEP( PutIndex610 ), ARCH_DEP( PutIndex710 ) };
PutIndex* ARCH_DEP( PutIndexTab11 )[8] = { ARCH_DEP( PutIndex011 ), ARCH_DEP( PutIndex111 ), ARCH_DEP( PutIndex211 ), ARCH_DEP( PutIndex311 ), ARCH_DEP( PutIndex411 ), ARCH_DEP( PutIndex511 ), ARCH_DEP( PutIndex611 ), ARCH_DEP( PutIndex711 ) };
PutIndex* ARCH_DEP( PutIndexTab12 )[8] = { ARCH_DEP( PutIndex012 ), ARCH_DEP( PutIndex112 ), ARCH_DEP( PutIndex212 ), ARCH_DEP( PutIndex312 ), ARCH_DEP( PutIndex412 ), ARCH_DEP( PutIndex512 ), ARCH_DEP( PutIndex612 ), ARCH_DEP( PutIndex712 ) };
PutIndex* ARCH_DEP( PutIndexTab13 )[8] = { ARCH_DEP( PutIndex013 ), ARCH_DEP( PutIndex113 ), ARCH_DEP( PutIndex213 ), ARCH_DEP( PutIndex313 ), ARCH_DEP( PutIndex413 ), ARCH_DEP( PutIndex513 ), ARCH_DEP( PutIndex613 ), ARCH_DEP( PutIndex713 ) };

///////////////////////////////////////////////////////////////////////////////
// (by CDSS)

PutIndex** ARCH_DEP( PutIndexCDSSTab )[5] = { ARCH_DEP( PutIndexTab09 ),
                                              ARCH_DEP( PutIndexTab10 ),
                                              ARCH_DEP( PutIndexTab11 ),
                                              ARCH_DEP( PutIndexTab12 ),
                                              ARCH_DEP( PutIndexTab13 ) };

PutGetCBN* ARCH_DEP( PutGetCBNTab )[5] = { ARCH_DEP( PutGetCBN09 ),
                                           ARCH_DEP( PutGetCBN10 ),
                                           ARCH_DEP( PutGetCBN11 ),
                                           ARCH_DEP( PutGetCBN12 ),
                                           ARCH_DEP( PutGetCBN13 ) };

///////////////////////////////////////////////////////////////////////////////
#endif /* FEATURE_CMPSC */

#ifndef _GEN_ARCH
  #ifdef _ARCH_NUM_1
    #define _GEN_ARCH _ARCH_NUM_1
    #include "cmpscput.c"
  #endif /* #ifdef _ARCH_NUM_1 */
  #ifdef _ARCH_NUM_2
    #undef _GEN_ARCH
    #define _GEN_ARCH _ARCH_NUM_2
    #include "cmpscput.c"
  #endif /* #ifdef _ARCH_NUM_2 */
#endif /* #ifndef _GEN_ARCH */
