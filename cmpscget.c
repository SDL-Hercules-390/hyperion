/* CMPSCGET.C   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Get Next Index Functions            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"    // (MUST be first #include in EVERY source file)

#define _CMPSCGET_C_
#define _HENGINE_DLL_

#if !defined( NOT_HERC )        // (building Hercules?)
#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#endif
#include "cmpsc.h"              // (Master header)

#ifdef FEATURE_CMPSC

///////////////////////////////////////////////////////////////////////////////
// EXPANSION: Get Next input SRC Index.
//
//  Returns:  #of SRC bytes consumed, or 0 if end of SRC i/p buffer reached.

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex009 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex109 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex209 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex309 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex409 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex509 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex609 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex709 )) ( GIBLK* pGIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex010 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex110 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex210 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex310 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex410 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex510 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex610 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex710 )) ( GIBLK* pGIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex011 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex111 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex211 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex311 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex411 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex511 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex611 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex711 )) ( GIBLK* pGIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex012 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex112 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex212 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex312 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex412 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex512 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex612 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex712 )) ( GIBLK* pGIBLK );

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex013 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex113 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex213 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex313 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex413 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex513 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex613 )) ( GIBLK* pGIBLK );
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex713 )) ( GIBLK* pGIBLK );

///////////////////////////////////////////////////////////////////////////////
// CDSS=1,  9 bits per index, CBN sequences:  0-1-2-3-4-5-6-7*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex009 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK )         ) >> 7;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex109 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex109 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x7FFF) >> 6;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex209 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex209 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x3FFF) >> 5;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex309 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex309 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x1FFF) >> 4;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex409 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex409 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x0FFF) >> 3;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex509 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex509 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x07FF) >> 2;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex609 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex609 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x03FF) >> 1;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex709 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex709 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x01FF);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex009 );
    return 2;
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=2, 10 bits per index, CBN sequences:  0-2-4-6*,  1-3-5-*7*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex010 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK )         ) >> 6;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex210 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex110 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x7FFF) >> 5;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex310 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex210 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x3FFF) >> 4;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex410 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex310 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x1FFF) >> 3;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex510 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex410 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x0FFF) >> 2;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex610 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex510 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x07FF) >> 1;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex710 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex610 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x03FF);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex010 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex710 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x01FF) << 1)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 7);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex110 );
    return 2;
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=3, 11 bits per index, CBN sequences:  0-3-*6*-1-4-*7*-2-5*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex011 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK )         ) >> 5;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex311 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex111 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x7FFF) >> 4;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex411 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex211 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x3FFF) >> 3;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex511 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex311 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x1FFF) >> 2;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex611 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex411 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x0FFF) >> 1;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex711 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex511 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x07FF);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex011 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex611 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x03FF) << 1)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 7);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex111 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex711 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x01FF) << 2)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 6);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex211 );
    return 2;
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=4, 12 bits per index, CBN sequences:  0-4*,  1-*5*,  2-*6*,  3-*7*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex012 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK )         ) >> 4;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex412 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex112 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x7FFF) >> 3;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex512 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex212 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x3FFF) >> 2;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex612 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex312 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x1FFF) >> 1;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex712 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex412 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x0FFF);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex012 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex512 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x07FF) << 1)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 7);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex112 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex612 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x03FF) << 2)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 6);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex212 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex712 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x01FF) << 3)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 5);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex312 );
    return 2;
}

///////////////////////////////////////////////////////////////////////////////
// CDSS=5, 13 bits per index, CBN sequences:  0-*5*-2-*7*-*4*-1-*6*-3*.
// * before CBN# = 3 bytes needed.
// * after  CBN# = 2 bytes consumed.

U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex013 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK )         ) >> 3;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex513 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex113 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x7FFF) >> 2;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex613 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex213 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x3FFF) >> 1;
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex713 );
    return 1;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex313 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 2) return 0;
    *pGIBLK->pIndex = (fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x1FFF);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex013 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex413 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x0FFF) << 1)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 7);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex113 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex513 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x07FF) << 2)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 6);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex213 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex613 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x03FF) << 3)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 5);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex313 );
    return 2;
}
U8  (CMPSC_FASTCALL ARCH_DEP( GetIndex713 )) ( GIBLK* pGIBLK )
{
    if (pGIBLK->pCMPSCBLK->nLen2 < 3) return 0;
    *pGIBLK->pIndex = ((fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2,     pGIBLK->pMEMBLK ) & 0x01FF) << 4)
                 | ((U16)fetch_op_b( pGIBLK->pCMPSCBLK->pOp2 + 2, pGIBLK->pMEMBLK )           >> 4);
    *pGIBLK->ppGetIndex = ARCH_DEP( GetIndex413 );
    return 2;
}

///////////////////////////////////////////////////////////////////////////////
// (Same thing, but 8 indexes at a time starting at CBN=0)
///////////////////////////////////////////////////////////////////////////////
// 0...4...
// 77777777
//(8)
//
// 0...4...8..12..16..20..24..28..32..36..40..44..48..52..56..60..6
// 0000000001111111112222222223333333334444444445555555556666666667
//    (9)      (9)      (9)      (9)      (9)      (9)      (9)   (1)

U8  (CMPSC_FASTCALL ARCH_DEP( Get8Index009 )) ( GIBLK* pGIBLK )
{
    register U8  b;
    register U64 dw;

    if (pGIBLK->pCMPSCBLK->nLen2 <  9) return 0;

    b  = fetch_op_b ( pGIBLK->pCMPSCBLK->pOp2 + 8, pGIBLK->pMEMBLK );
    dw = fetch_op_dw( pGIBLK->pCMPSCBLK->pOp2 + 0, pGIBLK->pMEMBLK );

    *(pGIBLK->pIndex + 7) = ( ((U16) b) | (((U16) dw) << 8) ) & 0x01FF; dw >>= 1;
    *(pGIBLK->pIndex + 6) = (               (U16) dw        ) & 0x01FF; dw >>= 9;
    *(pGIBLK->pIndex + 5) = (               (U16) dw        ) & 0x01FF; dw >>= 9;
    *(pGIBLK->pIndex + 4) = (               (U16) dw        ) & 0x01FF; dw >>= 9;
    *(pGIBLK->pIndex + 3) = (               (U16) dw        ) & 0x01FF; dw >>= 9;
    *(pGIBLK->pIndex + 2) = (               (U16) dw        ) & 0x01FF; dw >>= 9;
    *(pGIBLK->pIndex + 1) = (               (U16) dw        ) & 0x01FF; dw >>= 9;
    *(pGIBLK->pIndex + 0) = (               (U16) dw        );

    return  9;
}

///////////////////////////////////////////////////////////////////////////////
// 0...4...8..12..1
// 6666667777777777
//(6)      (10)
//
// 0...4...8..12..16..20..24..28..32..36..40..44..48..52..56..60..6
// 0000000000111111111122222222223333333333444444444455555555556666
//   (10)      (10)      (10)      (10)      (10)      (10)       (4)

U8  (CMPSC_FASTCALL ARCH_DEP( Get8Index010 )) ( GIBLK* pGIBLK )
{
    register U16 hw;
    register U64 dw;

    if (pGIBLK->pCMPSCBLK->nLen2 < 10) return 0;

    hw = fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2 + 8, pGIBLK->pMEMBLK );
    dw = fetch_op_dw( pGIBLK->pCMPSCBLK->pOp2 + 0, pGIBLK->pMEMBLK );

    *(pGIBLK->pIndex + 7) = ( hw                     ) & 0x03FF; hw >>= 10;
    *(pGIBLK->pIndex + 6) = ( hw | (((U16) dw) << 6) ) & 0x03FF; dw >>=  4;
    *(pGIBLK->pIndex + 5) = (        (U16) dw        ) & 0x03FF; dw >>= 10;
    *(pGIBLK->pIndex + 4) = (        (U16) dw        ) & 0x03FF; dw >>= 10;
    *(pGIBLK->pIndex + 3) = (        (U16) dw        ) & 0x03FF; dw >>= 10;
    *(pGIBLK->pIndex + 2) = (        (U16) dw        ) & 0x03FF; dw >>= 10;
    *(pGIBLK->pIndex + 1) = (        (U16) dw        ) & 0x03FF; dw >>= 10;
    *(pGIBLK->pIndex + 0) = (        (U16) dw        );

    return  10;
}

///////////////////////////////////////////////////////////////////////////////
// 0...4...
// 77777777
//(8)
//
// 0...4...8..12..1
// 5566666666666777
//(2)   (11)      (3)
//
// 0...4...8..12..16..20..24..28..32..36..40..44..48..52..56..60..6
// 0000000000011111111111222222222223333333333344444444444555555555
//    (11)       (11)       (11)       (11)       (11)            (9)

U8  (CMPSC_FASTCALL ARCH_DEP( Get8Index011 )) ( GIBLK* pGIBLK )
{
    register U8  b;
    register U16 hw;
    register U64 dw;

    if (pGIBLK->pCMPSCBLK->nLen2 < 11) return 0;

    b  = fetch_op_b ( pGIBLK->pCMPSCBLK->pOp2 + 10, pGIBLK->pMEMBLK );
    hw = fetch_op_hw( pGIBLK->pCMPSCBLK->pOp2 +  8, pGIBLK->pMEMBLK );
    dw = fetch_op_dw( pGIBLK->pCMPSCBLK->pOp2 +  0, pGIBLK->pMEMBLK );

    *(pGIBLK->pIndex + 7) = ( ((U16) b) | (       hw  << 8) ) & 0x07FF; hw >>=  3;
    *(pGIBLK->pIndex + 6) = (    hw                         ) & 0x07FF; hw >>= 11;
    *(pGIBLK->pIndex + 5) = (    hw     | (((U16) dw) << 2) ) & 0x07FF; dw >>=  9;
    *(pGIBLK->pIndex + 4) = (               (U16) dw        ) & 0x07FF; dw >>= 11;
    *(pGIBLK->pIndex + 3) = (               (U16) dw        ) & 0x07FF; dw >>= 11;
    *(pGIBLK->pIndex + 2) = (               (U16) dw        ) & 0x07FF; dw >>= 11;
    *(pGIBLK->pIndex + 1) = (               (U16) dw        ) & 0x07FF; dw >>= 11;
    *(pGIBLK->pIndex + 0) = (               (U16) dw        );

    return  11;
}

///////////////////////////////////////////////////////////////////////////////
//
// 0...4...8..12..16..20..24..28..3
// 55555555666666666666777777777777
//(8)          (12)        (12)
//
// 0...4...8..12..16..20..24..28..32..36..40..44..48..52..56..60..6
// 0000000000001111111111112222222222223333333333334444444444445555
//     (12)        (12)        (12)        (12)        (12)       (4)

U8  (CMPSC_FASTCALL ARCH_DEP( Get8Index012 )) ( GIBLK* pGIBLK )
{
    register U32 fw;
    register U64 dw;

    if (pGIBLK->pCMPSCBLK->nLen2 < 12) return 0;

    fw = fetch_op_fw( pGIBLK->pCMPSCBLK->pOp2 + 8, pGIBLK->pMEMBLK );
    dw = fetch_op_dw( pGIBLK->pCMPSCBLK->pOp2 + 0, pGIBLK->pMEMBLK );

    *(pGIBLK->pIndex + 7) = (  (U16) fw                      ) & 0x0FFF; fw >>= 12;
    *(pGIBLK->pIndex + 6) = (  (U16) fw                      ) & 0x0FFF; fw >>= 12;
    *(pGIBLK->pIndex + 5) = ( ((U16) fw) | (((U16) dw) << 8) ) & 0x0FFF; dw >>=  4;
    *(pGIBLK->pIndex + 4) = (                (U16) dw        ) & 0x0FFF; dw >>= 12;
    *(pGIBLK->pIndex + 3) = (                (U16) dw        ) & 0x0FFF; dw >>= 12;
    *(pGIBLK->pIndex + 2) = (                (U16) dw        ) & 0x0FFF; dw >>= 12;
    *(pGIBLK->pIndex + 1) = (                (U16) dw        ) & 0x0FFF; dw >>= 12;
    *(pGIBLK->pIndex + 0) = (                (U16) dw        );

    return  12;
}

///////////////////////////////////////////////////////////////////////////////
//  0...4...
//  77777777
//(8)
//
// 0...4...8..12..16..20..24..28..3
// 45555555555555666666666666677777
//(1)    (13)         (13)        (5)
//
// 0...4...8..12..16..20..24..28..32..36..40..44..48..52..56..60..6
// 0000000000000111111111111122222222222223333333333333444444444444
//      (13)         (13)         (13)         (13)               (12)

U8  (CMPSC_FASTCALL ARCH_DEP( Get8Index013 )) ( GIBLK* pGIBLK )
{
    register U8  b;
    register U32 fw;
    register U64 dw;

    if (pGIBLK->pCMPSCBLK->nLen2 < 13) return 0;

    b  = fetch_op_b ( pGIBLK->pCMPSCBLK->pOp2 + 12, pGIBLK->pMEMBLK );
    fw = fetch_op_fw( pGIBLK->pCMPSCBLK->pOp2 +  8, pGIBLK->pMEMBLK );
    dw = fetch_op_dw( pGIBLK->pCMPSCBLK->pOp2 +  0, pGIBLK->pMEMBLK );

    *(pGIBLK->pIndex + 7) = ( ((U16) b)  | (((U16) fw) << 8) ) & 0x1FFF; fw >>=  5;
    *(pGIBLK->pIndex + 6) = (  (U16) fw                      ) & 0x1FFF; fw >>= 13;
    *(pGIBLK->pIndex + 5) = (  (U16) fw                      ) & 0x1FFF; fw >>= 13;
    *(pGIBLK->pIndex + 4) = ( ((U16) fw) | (((U16) dw) << 1) ) & 0x1FFF; dw >>= 12;
    *(pGIBLK->pIndex + 3) = (                (U16) dw        ) & 0x1FFF; dw >>= 13;
    *(pGIBLK->pIndex + 2) = (                (U16) dw        ) & 0x1FFF; dw >>= 13;
    *(pGIBLK->pIndex + 1) = (                (U16) dw        ) & 0x1FFF; dw >>= 13;
    *(pGIBLK->pIndex + 0) = (                (U16) dw        );

    return  13;
}

///////////////////////////////////////////////////////////////////////////////
// (by CBN)

GetIndex* ARCH_DEP( GetIndexTab09 )[8] = { ARCH_DEP( GetIndex009 ), ARCH_DEP( GetIndex109 ), ARCH_DEP( GetIndex209 ), ARCH_DEP( GetIndex309 ), ARCH_DEP( GetIndex409 ), ARCH_DEP( GetIndex509 ), ARCH_DEP( GetIndex609 ), ARCH_DEP( GetIndex709 ) };
GetIndex* ARCH_DEP( GetIndexTab10 )[8] = { ARCH_DEP( GetIndex010 ), ARCH_DEP( GetIndex110 ), ARCH_DEP( GetIndex210 ), ARCH_DEP( GetIndex310 ), ARCH_DEP( GetIndex410 ), ARCH_DEP( GetIndex510 ), ARCH_DEP( GetIndex610 ), ARCH_DEP( GetIndex710 ) };
GetIndex* ARCH_DEP( GetIndexTab11 )[8] = { ARCH_DEP( GetIndex011 ), ARCH_DEP( GetIndex111 ), ARCH_DEP( GetIndex211 ), ARCH_DEP( GetIndex311 ), ARCH_DEP( GetIndex411 ), ARCH_DEP( GetIndex511 ), ARCH_DEP( GetIndex611 ), ARCH_DEP( GetIndex711 ) };
GetIndex* ARCH_DEP( GetIndexTab12 )[8] = { ARCH_DEP( GetIndex012 ), ARCH_DEP( GetIndex112 ), ARCH_DEP( GetIndex212 ), ARCH_DEP( GetIndex312 ), ARCH_DEP( GetIndex412 ), ARCH_DEP( GetIndex512 ), ARCH_DEP( GetIndex612 ), ARCH_DEP( GetIndex712 ) };
GetIndex* ARCH_DEP( GetIndexTab13 )[8] = { ARCH_DEP( GetIndex013 ), ARCH_DEP( GetIndex113 ), ARCH_DEP( GetIndex213 ), ARCH_DEP( GetIndex313 ), ARCH_DEP( GetIndex413 ), ARCH_DEP( GetIndex513 ), ARCH_DEP( GetIndex613 ), ARCH_DEP( GetIndex713 ) };

GetIndex* ARCH_DEP( Get8IndexTab09 )[8] = { ARCH_DEP( Get8Index009 ), NULL, NULL, NULL, NULL, NULL, NULL, NULL };
GetIndex* ARCH_DEP( Get8IndexTab10 )[8] = { ARCH_DEP( Get8Index010 ), NULL, NULL, NULL, NULL, NULL, NULL, NULL };
GetIndex* ARCH_DEP( Get8IndexTab11 )[8] = { ARCH_DEP( Get8Index011 ), NULL, NULL, NULL, NULL, NULL, NULL, NULL };
GetIndex* ARCH_DEP( Get8IndexTab12 )[8] = { ARCH_DEP( Get8Index012 ), NULL, NULL, NULL, NULL, NULL, NULL, NULL };
GetIndex* ARCH_DEP( Get8IndexTab13 )[8] = { ARCH_DEP( Get8Index013 ), NULL, NULL, NULL, NULL, NULL, NULL, NULL };

///////////////////////////////////////////////////////////////////////////////
// (by CDSS)

GetIndex** ARCH_DEP( GetIndexCDSSTab )[5] = { ARCH_DEP( GetIndexTab09 ),
                                              ARCH_DEP( GetIndexTab10 ),
                                              ARCH_DEP( GetIndexTab11 ),
                                              ARCH_DEP( GetIndexTab12 ),
                                              ARCH_DEP( GetIndexTab13 ) };

GetIndex** ARCH_DEP( Get8IndexCDSSTab )[5] = { ARCH_DEP( Get8IndexTab09 ),
                                               ARCH_DEP( Get8IndexTab10 ),
                                               ARCH_DEP( Get8IndexTab11 ),
                                               ARCH_DEP( Get8IndexTab12 ),
                                               ARCH_DEP( Get8IndexTab13 ) };

///////////////////////////////////////////////////////////////////////////////
#endif /* FEATURE_CMPSC */

#ifndef _GEN_ARCH
  #ifdef _ARCH_NUM_1
    #define _GEN_ARCH _ARCH_NUM_1
    #include "cmpscget.c"
  #endif /* #ifdef _ARCH_NUM_1 */
  #ifdef _ARCH_NUM_2
    #undef _GEN_ARCH
    #define _GEN_ARCH _ARCH_NUM_2
    #include "cmpscget.c"
  #endif /* #ifdef _ARCH_NUM_2 */
#endif /* #ifndef _GEN_ARCH */
