/* CMPSCDCT.C   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Get Dictionary Entry functions      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"    // (MUST be first #include in EVERY source file)

#define _CMPSCDCT_C_
#define _HENGINE_DLL_

#if !defined( NOT_HERC )        // (building Hercules?)
#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#endif
#include "cmpsc.h"              // (Master header)

#ifdef FEATURE_CMPSC
///////////////////////////////////////////////////////////////////////////////
// GetDCT: fetch 8-byte dictionary entry as a 64-bit unsigned integer

U64  (CMPSC_FASTCALL ARCH_DEP( GetDCT ))( U16 index, DCTBLK* pDCTBLK )
{
    register U16  pagenum  = INDEX_TO_PAGENUM( index );
    register U16  pageidx  = INDEX_TO_PAGEIDX( index );

    if (!pDCTBLK->maddr[ pagenum ])
    {
        pDCTBLK->maddr[ pagenum ] = MADDR
        (
            pDCTBLK->pDict + PAGENUM_TO_BYTES( pagenum ),
            pDCTBLK->arn,
            pDCTBLK->regs,
            ACCTYPE_READ,
            pDCTBLK->pkey
        );
    }
    return CSWAP64(*(U64*)(uintptr_t)(&pDCTBLK->maddr[ pagenum ][ pageidx ]));
}

///////////////////////////////////////////////////////////////////////////////
// GetECE: Extract EXPANSION Character Entry into portable ECE structure
//
// Returns: TRUE/FALSE (success/data exception)

U8 (CMPSC_FASTCALL ARCH_DEP( GetECE ))( U16 index, ECEBLK* pECEBLK )
{
    register U64 ece;
    register ECE* pECE = pECEBLK->pECE;

    if (pECEBLK->ece[ index ].cached)
    {
        *pECE = pECEBLK->ece[ index ];
        return TRUE;
    }

    ece = ARCH_DEP( GetDCT )( index, pECEBLK->pDCTBLK );

    if (!(pECE->psl = ECE_U8R( 0, 3 )))
    {
        if (!(pECE->csl = ECE_U8R( 5, 3 )))
            return FALSE;
        pECE->ec_dw = CSWAP64( ece << 8 );
    }
    else if (pECE->psl > 5)
        return FALSE;
    else
    {
        if ((pECE->pptr = ECE_U16R( 3, 13 )) > pECEBLK->max_index)
            return FALSE;
        pECE->ofst = (U8)(ece);
        pECE->ec_dw = CSWAP64( ece << 16 );
        pECE->csl = 0;
    }

    pECE->cached = TRUE;
    pECEBLK->ece[ index ] = *pECE;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// GetCCE: Extract COMPRESSION Character Entry into portable CCE structure
//
// Returns: TRUE/FALSE (success/data exception)

U8 (CMPSC_FASTCALL ARCH_DEP( GetCCE ))( U16 index, CCEBLK* pCCEBLK )
{
    register U64 cce;
    register CCE* pCCE = pCCEBLK->pCCE;

    if (pCCEBLK->cce[ index ].cached)
    {
        *pCCE = pCCEBLK->cce[ index ];
        return TRUE;
    }

    cce = ARCH_DEP( GetDCT )( index, pCCEBLK->pDCTBLK );
    pCCE->mc = FALSE;

    if (!(pCCE->cct = CCE_U8R( 0, 3 )))  // (no children)
    {
        if ((pCCE->act = CCE_U8R( 8, 3 )) > 5)
            return FALSE;

        if (pCCE->act)
            pCCE->ec_dw = CSWAP64( cce << 24 );

        pCCE->cached = TRUE;
        pCCEBLK->cce[ index ] = *pCCE;

        return TRUE;
    }

    if (pCCE->cct == 1)  // (only one child)
    {
        register U8 wrk;

        if ((pCCE->act = CCE_U8R( 8, 3 )) > 4)
            return FALSE;

        wrk = (pCCE->act << 3);
        pCCE->cc[0] = CCE_U8R( 24 + wrk, 8 );

        if (pCCE->act)
            pCCE->ec_dw = CSWAP64( cce << 24 );

        pCCE->cptr = CCE_U16R( 11, 13 );
        pCCE->ecb  = CCE_U16L(  3,  1 );
    }
    else // (many children)
    {
        register U8 wrk; // (max cct)

        if (!(pCCE->act = CCE_U8R( 10, 1 ))) // (act == 0)
            wrk = 5;
        else // (act == 1)
        {
            wrk = 4;
            pCCE->ec[0] = CCE_U8R( 24, 8 );
        }

        if (pCCE->cct > (wrk + 1))   // (data exception?)
            return FALSE;

        if (pCCE->cct == (wrk + 1))  // (more children?)
        {
            pCCE->mc = TRUE;         // (set flag)
            pCCE->cct = wrk;         // (fix cct)
        }

        wrk = (pCCE->act << 3);      // (cc start bit - 24)

        pCCE->cc_dw = CSWAP64( cce << (24 + wrk) );

        pCCE->cptr = CCE_U16R( 11, 13 );
        pCCE->ecb  = CCE_U16L(  3,  5 );
        pCCE->yy   = CCE_U16L(  8,  2 );
    }

    pCCE->cached = TRUE;
    pCCEBLK->cce[ index ] = *pCCE;

    return (pCCE->cptr > pCCEBLK->max_index) ? FALSE : TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// GetSD0: Extract Format-0 Sibling Descriptor into portable SDE structure
//
// Returns: TRUE/FALSE (success/data exception)

U8 (CMPSC_FASTCALL ARCH_DEP( GetSD0 ))( U16 index, SDEBLK* pSDEBLK )
{
    register U64 sd1;
    register SDE* pSDE = pSDEBLK->pSDE;

    if (pSDEBLK->sde[ index ].cached)
    {
        *pSDE = pSDEBLK->sde[ index ];
        return TRUE;
    }

    sd1 = ARCH_DEP( GetDCT )( index, pSDEBLK->pDCTBLK );
    pSDE->ms = FALSE;

    if (!(pSDE->sct = SD1_U8R( 0, 3 )) || pSDE->sct >= 7)
    {
        pSDE->sct = 7;
        pSDE->ms = TRUE;
    }

    // Examine child bits for children 1 to 5

    pSDE->ecb = SD1_U16L( 3, 5 );

    // If children exist append examine child bits for
    // children 6 and 7 which are in the parent CCE so
    // they're all in one place (makes things easier).
    //
    // Note: this only applies for the FIRST sibling
    // of the parent. Examine child bits for children
    // 6 and 7 do not exist for subsequent siblings
    // of parent and thus must ALWAYS be examined.

    if (pSDEBLK->pCCE)  // (first sibling of parent?)
    {
        if (pSDEBLK->pCCE->cct > 1)
            pSDE->ecb |= pSDEBLK->pCCE->yy >> 5;
    }
    else // (force child to ALWAYS be examined)
    {
        pSDE->ecb |= 0xFFFF >> 5;
    }

    pSDE->sc_dw = CSWAP64( sd1 << 8 );

    pSDE->cached = TRUE;
    pSDEBLK->sde[ index ] = *pSDE;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// GetSD1: Extract Format-1 Sibling Descriptor into portable SDE structure
//
// Returns: TRUE/FALSE (success/data exception)

U8 (CMPSC_FASTCALL ARCH_DEP( GetSD1 ))( U16 index, SDEBLK* pSDEBLK )
{
    register U64 sd1;
    register SDE* pSDE = pSDEBLK->pSDE;

    if (pSDEBLK->sde[ index ].cached)
    {
        *pSDE = pSDEBLK->sde[ index ];
        return TRUE;
    }

    sd1 = ARCH_DEP( GetDCT )( index, pSDEBLK->pDCTBLK );
    pSDE->ms = FALSE;

    if (!(pSDE->sct = SD1_U8R( 0, 4 )) || pSDE->sct >= 15)
    {
        pSDE->sct = 14;
        pSDE->ms = TRUE;
    }

    // Examine child bits for children 1 to 12

    pSDE->ecb = SD1_U16L( 4, 12 );

    // If children exist append examine child bits for
    // children 13 and 14 which are in the parent CCE
    // so they are all in one place (simpler testing).
    //
    // Note: this only applies for the FIRST sibling
    // of the parent. Examine child bits for children
    // 13 and 14 do not exist for subsequent siblings
    // of parent and thus must ALWAYS be examined.

    if (pSDEBLK->pCCE)  // (first sibling of parent?)
    {
        if (pSDEBLK->pCCE->cct > 1)
            pSDE->ecb |= pSDEBLK->pCCE->yy >> 12;
    }
    else // (force child to ALWAYS be examined)
    {
        pSDE->ecb |= 0xFFFF >> 12;
    }

    sd1 <<= 16;                               // (first 6 bytes)

    if (pSDE->sct <= 6)
        pSDE->sc_dw = CSWAP64( sd1 );         // (only 6 bytes)
    else
    {
        register U64 sd2 = ARCH_DEP( GetDCT )( index, pSDEBLK->pDCTBLK2 );

        sd1 |= sd2 >> (64-16);                // (append 2 more)
        pSDE->sc_dw= CSWAP64( sd1 );          // (store first 8)

        sd2 <<= 16;                           // (next 6 bytes)
        pSDE->sc_dw2 = CSWAP64( sd2 );        // (store next 6)
    }

    pSDE->cached = TRUE;
    pSDEBLK->sde[ index ] = *pSDE;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
#endif /* FEATURE_CMPSC */

#ifndef _GEN_ARCH
  #ifdef _ARCH_NUM_1
    #define _GEN_ARCH _ARCH_NUM_1
    #include "cmpscdct.c"
  #endif /* #ifdef _ARCH_NUM_1 */
  #ifdef _ARCH_NUM_2
    #undef _GEN_ARCH
    #define _GEN_ARCH _ARCH_NUM_2
    #include "cmpscdct.c"
  #endif /* #ifdef _ARCH_NUM_2 */
#endif /* #ifndef _GEN_ARCH */
