/* CMPSCMEM.C   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Memory Access Functions             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"    // (MUST be first #include in EVERY source file)

#define _CMPSCMEM_C_
#define _HENGINE_DLL_

#if !defined( NOT_HERC )        // (building Hercules?)
#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#endif
#include "cmpsc.h"              // (Master header)

#ifdef FEATURE_CMPSC
/*-------------------------------------------------------------------*/
/* Fetch a one byte value from operand virtual storage               */
/*-------------------------------------------------------------------*/
U8 (CMPSC_FASTCALL ARCH_DEP( cmpsc_vfetchb ))( VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_READ,   // (fetch)
            pMEMBLK->pkey
        );
    }
    if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
    {
        return pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK];
    }
    else
    {
        if (unlikely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_READ,   // (fetch)
                pMEMBLK->pkey
            );
        }
        return pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK];
    }
}

/*-------------------------------------------------------------------*/
/* Store a one byte value into operand virtual storage               */
/*-------------------------------------------------------------------*/
void (CMPSC_FASTCALL ARCH_DEP( cmpsc_vstoreb ))( U8 byt, VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_WRITE,  // (store)
            pMEMBLK->pkey
        );
    }
    if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
    {
        pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] = byt;
    }
    else
    {
        if (unlikely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_WRITE,  // (store)
                pMEMBLK->pkey
            );
        }
        pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] = byt;
    }
}

/*-------------------------------------------------------------------*/
/* Fetch a two-byte halfword value from operand virtual storage      */
/*-------------------------------------------------------------------*/
U16 (CMPSC_FASTCALL ARCH_DEP( cmpsc_vfetch2 ))( VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_READ,   // (fetch)
            pMEMBLK->pkey
        );
    }
    if (!LASTBYTEOFPAGE( addr ))
    {
        if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
        {
            if (U16_ALIGNED( addr ))
                return CSWAP16( *(U16*) &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] );
            return fetch_hw(            &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] );
        }
        else
        {
            if (unlikely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_READ,   // (fetch)
                    pMEMBLK->pkey
                );
            }
            if (U16_ALIGNED( addr ))
                return CSWAP16( *(U16*) &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] );
            return fetch_hw(            &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] );
        }
    }
    else
    {
        if (likely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_READ,   // (fetch)
                pMEMBLK->pkey
            );
        }
        return (((U16) pMEMBLK->maddr[0][PAGEFRAME_BYTEMASK]) << 8)
           |    ((U16) pMEMBLK->maddr[1][0]                       );
    }
}

/*-------------------------------------------------------------------*/
/* Store a two-byte halfword value into operand virtual storage      */
/*-------------------------------------------------------------------*/
void (CMPSC_FASTCALL ARCH_DEP( cmpsc_vstore2 ))( U16 val, VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_WRITE,  // (store)
            pMEMBLK->pkey
        );
    }
    if (!LASTBYTEOFPAGE( addr ))
    {
        if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
        {
            if (U16_ALIGNED( addr ))
                *(U16*)   &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] = CSWAP16( val );
            else
                store_hw( &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK],           val );
        }
        else
        {
            if (unlikely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_WRITE,  // (store)
                    pMEMBLK->pkey
                );
            }
            if (U16_ALIGNED( addr ))
                *(U16*)   &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] = CSWAP16( val );
            else
                store_hw( &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK],           val );
        }
    }
    else
    {
        if (likely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_WRITE,  // (store)
                pMEMBLK->pkey
            );
        }
        pMEMBLK->maddr[0][PAGEFRAME_BYTEMASK] = (U8)(val >> 8);
        pMEMBLK->maddr[1][0]                  = (U8)(val     );
    }
}

/*-------------------------------------------------------------------*/
/* Fetch a four-byte fullword value from operand virtual storage     */
/*-------------------------------------------------------------------*/
U32 (CMPSC_FASTCALL ARCH_DEP( cmpsc_vfetch4 ))( VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_READ,   // (fetch)
            pMEMBLK->pkey
        );
    }
    if (NOCROSSPAGE( addr, 4 ))
    {
        if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
        {
            if (U32_ALIGNED( addr ))
                return CSWAP32( *(U32*) &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] );
            return fetch_fw(            &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] );
        }
        else
        {
            if (unlikely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_READ,   // (fetch)
                    pMEMBLK->pkey
                );
            }
            if (U32_ALIGNED( addr ))
                return CSWAP32( *(U32*) &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] );
            return fetch_fw(            &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] );
        }
    }
    else
    {
        U32 value;
        U16 len1 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
        if (likely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_READ,   // (fetch)
                pMEMBLK->pkey
            );
        }
        memcpy( (U8*)&value,        &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK],     len1 );
        memcpy( (U8*)&value + len1, &pMEMBLK->maddr[1][0],                         4 - len1 );
        return CSWAP32( value );
    }
}

/*-------------------------------------------------------------------*/
/* Store a four-byte fullword value into operand virtual storage     */
/*-------------------------------------------------------------------*/
void (CMPSC_FASTCALL ARCH_DEP( cmpsc_vstore4 ))( U32 val, VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_WRITE,  // (store)
            pMEMBLK->pkey
        );
    }
    if (NOCROSSPAGE( addr, 4 ))
    {
        if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
        {
            if (U32_ALIGNED( addr ))
                *(U32*)   &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] = CSWAP32( val );
            else
                store_dw( &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK],           val );
        }
        else
        {
            if (unlikely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_WRITE,  // (store)
                    pMEMBLK->pkey
                );
            }
            if (U32_ALIGNED( addr ))
                *(U32*)   &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] = CSWAP32( val );
            else
                store_dw( &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK],           val );
        }
    }
    else
    {
        U32 value = CSWAP32( val );
        U16 len1 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
        if (likely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_WRITE,  // (store)
                pMEMBLK->pkey
            );
        }
        memcpy( &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK], (U8*)&value,            len1 );
        memcpy( &pMEMBLK->maddr[1][0],                         (U8*)&value + len1, 4 - len1 );
    }
}

/*-------------------------------------------------------------------*/
/* Fetch an eight-byte doubleword value from operand virtual storage */
/*-------------------------------------------------------------------*/
U64 (CMPSC_FASTCALL ARCH_DEP( cmpsc_vfetch8 ))( VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_READ,   // (fetch)
            pMEMBLK->pkey
        );
    }
    if (NOCROSSPAGE( addr, 8 ))
    {
        if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
        {
            if (U64_ALIGNED( addr ))
                return CSWAP64( *(U64*) &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] );
            return fetch_dw(            &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] );
        }
        else
        {
            if (unlikely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_READ,   // (fetch)
                    pMEMBLK->pkey
                );
            }
            if (U64_ALIGNED( addr ))
                return CSWAP64( *(U64*) &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] );
            return fetch_dw(            &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] );
        }
    }
    else
    {
        U64 value;
        U16 len1 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
        if (likely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_READ,   // (fetch)
                pMEMBLK->pkey
            );
        }
        memcpy( (U8*)&value,        &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK],     len1 );
        memcpy( (U8*)&value + len1, &pMEMBLK->maddr[1][0],                         8 - len1 );
        return CSWAP64( value );
    }
}

/*-------------------------------------------------------------------*/
/* Store an eight-byte doubleword value into operand virtual storage */
/*-------------------------------------------------------------------*/
void (CMPSC_FASTCALL ARCH_DEP( cmpsc_vstore8 ))( U64 val, VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_WRITE,  // (store)
            pMEMBLK->pkey
        );
    }
    if (NOCROSSPAGE( addr, 8 ))
    {
        if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
        {
            if (U64_ALIGNED( addr ))
                *(U64*)   &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK] = CSWAP64( val );
            else
                store_dw( &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK],           val );
        }
        else
        {
            if (unlikely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_WRITE,  // (store)
                    pMEMBLK->pkey
                );
            }
            if (U64_ALIGNED( addr ))
                *(U64*)   &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK] = CSWAP64( val );
            else
                store_dw( &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK],           val );
        }
    }
    else
    {
        U64 value = CSWAP64( val );
        U16 len1 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
        if (likely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_WRITE,  // (store)
                pMEMBLK->pkey
            );
        }
        memcpy( &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK], (U8*)&value,            len1 );
        memcpy( &pMEMBLK->maddr[1][0],                         (U8*)&value + len1, 8 - len1 );
    }
}

/*-------------------------------------------------------------------*/
/* Fetch a 1 to PAGEFRAME_PAGESIZE character operand from virtual    */
/* storage; len = size of operand minus 1                            */
/*-------------------------------------------------------------------*/
void (CMPSC_FASTCALL ARCH_DEP( cmpsc_vfetchc ))( U8* dst, U16 len, VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_READ,   // (fetch)
            pMEMBLK->pkey
        );
    }
    if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
    {
        if (NOCROSSPAGE( addr, len ))
        {
            memcpy( dst, &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK], len+1 );
        }
        else
        {
            U16 len1 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
            if (likely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_READ,   // (fetch)
                    pMEMBLK->pkey
                );
            }
            memcpy(  dst,         &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK], len1 );
            memcpy( (dst + len1), &pMEMBLK->maddr[1][0],        len+1     -      len1 );
        }
    }
    else
    {
        if (unlikely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_READ,   // (fetch)
                pMEMBLK->pkey
            );
        }
        memcpy( dst, &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK], len+1 );
    }
}

/*-------------------------------------------------------------------*/
/* Store 1 to PAGEFRAME_PAGESIZE characters into operand virtual     */
/* storage; len = size of operand minus 1                            */
/*-------------------------------------------------------------------*/
void (CMPSC_FASTCALL ARCH_DEP( cmpsc_vstorec ))( U8* src, U16 len, VADR addr, MEMBLK* pMEMBLK )
{
    addr &= ADDRESS_MAXWRAP( pMEMBLK->regs );

    if (unlikely(!pMEMBLK->maddr[0] || addr < pMEMBLK->vpagebeg))
    {
        pMEMBLK->vpagebeg = (addr & PAGEFRAME_PAGEMASK);
        pMEMBLK->maddr[1] = 0;
        pMEMBLK->maddr[0] = MADDR
        (
            pMEMBLK->vpagebeg,
            pMEMBLK->arn,
            pMEMBLK->regs,
            ACCTYPE_WRITE,  // (store)
            pMEMBLK->pkey
        );
    }
    if (MEMBLK_FIRSTPAGE( pMEMBLK, addr ))
    {
        if (NOCROSSPAGE( addr, len ))
        {
            memcpy( &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK], src, len+1 );
        }
        else
        {
            U16 len1 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
            if (likely(!pMEMBLK->maddr[1]))
            {
                pMEMBLK->maddr[1] = MADDR
                (
                    pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                    pMEMBLK->arn,
                    pMEMBLK->regs,
                    ACCTYPE_WRITE,  // (store)
                    pMEMBLK->pkey
                );
            }
            memcpy( &pMEMBLK->maddr[0][addr & PAGEFRAME_BYTEMASK], src,                len1 );
            memcpy(  pMEMBLK->maddr[1],                            src + len1, len+1 - len1 );
        }
    }
    else
    {
        if (unlikely(!pMEMBLK->maddr[1]))
        {
            pMEMBLK->maddr[1] = MADDR
            (
                pMEMBLK->vpagebeg + PAGEFRAME_PAGESIZE,
                pMEMBLK->arn,
                pMEMBLK->regs,
                ACCTYPE_WRITE,  // (store)
                pMEMBLK->pkey
            );
        }
        memcpy( &pMEMBLK->maddr[1][addr & PAGEFRAME_BYTEMASK], src, len+1 );
    }
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions to either build CMPSCBLK from information in REGS
// or the complete opposite: build the REGS struct from the CMPSCBLK.

void (CMPSC_FASTCALL ARCH_DEP( cmpsc_SetREGS ))( CMPSCBLK* pCMPSCBLK, REGS* regs, int r1, int r2 )
{
    SET_GR_A( r1,     regs, (VADR) pCMPSCBLK->pOp1  );
    SET_GR_A( r2,     regs, (VADR) pCMPSCBLK->pOp2  );
    SET_GR_A( r1 + 1, regs, (GREG) pCMPSCBLK->nLen1 );
    SET_GR_A( r2 + 1, regs, (GREG) pCMPSCBLK->nLen2 );

    regs->psw.cc      = pCMPSCBLK->cc;
    regs->psw.intcode = pCMPSCBLK->pic;

    /* Register 0 is input-only and thus not modified.

    SET_GR_A( 0, regs,
    (0
        | ((GREG)   zeropad       << 17)
        | ((GREG) pCMPSCBLK->st   << 16)
        | ((GREG) pCMPSCBLK->cdss << 12)
        | ((GREG) pCMPSCBLK->f1   <<  9)
        | ((GREG)   expand        <<  8)
    ));
    */

    SET_GR_A( 1, regs, ((GREG) pCMPSCBLK->pDict     ) |
                       ((GREG) pCMPSCBLK->stt  <<  3) |
                       ((GREG) pCMPSCBLK->cbn       ) );
}

///////////////////////////////////////////////////////////////////////////////
// (same thing but including GR0 too. Used only by utility test tool)

#if defined( NOT_HERC )
void (CMPSC_FASTCALL ARCH_DEP( util_cmpsc_SetREGS ))( CMPSCBLK* pCMPSCBLK, REGS* regs, int r1, int r2, U8 expand, U8 zeropad )
{
    SET_GR_A( r1,     regs, (VADR) pCMPSCBLK->pOp1  );
    SET_GR_A( r2,     regs, (VADR) pCMPSCBLK->pOp2  );
    SET_GR_A( r1 + 1, regs, (GREG) pCMPSCBLK->nLen1 );
    SET_GR_A( r2 + 1, regs, (GREG) pCMPSCBLK->nLen2 );

    regs->psw.cc      = pCMPSCBLK->cc;
    regs->psw.intcode = pCMPSCBLK->pic;

    /* (for the convenience of the utility testing tool) */

    SET_GR_A( 0, regs,
    (0
        | ((GREG)   zeropad       << 17)
        | ((GREG) pCMPSCBLK->st   << 16)
        | ((GREG) pCMPSCBLK->cdss << 12)
        | ((GREG) pCMPSCBLK->f1   <<  9)
        | ((GREG)   expand        <<  8)
    ));

    SET_GR_A( 1, regs, ((GREG) pCMPSCBLK->pDict     ) |
                       ((GREG) pCMPSCBLK->stt  <<  3) |
                       ((GREG) pCMPSCBLK->cbn       ) );
}
#endif // defined( NOT_HERC )

///////////////////////////////////////////////////////////////////////////////
// (same idea but going in the opposite direction)

void (CMPSC_FASTCALL ARCH_DEP( cmpsc_SetCMPSC ))( CMPSCBLK* pCMPSCBLK, REGS* regs, int r1, int r2 )
{
    register GREG GR0, GR1;

    pCMPSCBLK->r1       =  r1;
    pCMPSCBLK->r2       =  r2;

    pCMPSCBLK->pOp1     = GR_A( r1,   regs );
    pCMPSCBLK->pOp2     = GR_A( r2,   regs );
    pCMPSCBLK->nLen1    = GR_A( r1+1, regs );
    pCMPSCBLK->nLen2    = GR_A( r2+1, regs );

    GR0 = regs->GR_L( 0 );
    GR1 = GR_A( 1, regs );

//  pCMPSCBLK->e        = (GR0 >>  8) & 0x01;   // (no such field)
    pCMPSCBLK->f1       = (GR0 >>  9) & 0x01;
    pCMPSCBLK->cdss     = (GR0 >> 12) & 0x0F;
    pCMPSCBLK->st       = (GR0 >> 16) & 0x01;
#if defined(_FEATURE_047_CMPSC_ENH_FACILITY)
    if (FACILITY_ENABLED( 047_CMPSC_ENH, regs ))
    pCMPSCBLK->zp       = (GR0 >> 17) & 0x01; else
#endif
    pCMPSCBLK->zp       = FALSE;

    pCMPSCBLK->cbn      = (GR1 &  0x007);
    pCMPSCBLK->stt      = (GR1 &  0xFFF) >>  3;
    pCMPSCBLK->pDict    = (GR1 & ~0xFFF);

    pCMPSCBLK->regs     =  regs;
    pCMPSCBLK->cc       =  regs->psw.cc;
    pCMPSCBLK->pic      =  regs->psw.intcode;
    pCMPSCBLK->nCPUAmt  =  DEF_CMPSC_CPU_AMT;
//  pCMPSCBLK->dbg      =  0;  // (future)
}

///////////////////////////////////////////////////////////////////////////////
#endif /* FEATURE_CMPSC */

#ifndef _GEN_ARCH
  #ifdef _ARCH_NUM_1
    #define _GEN_ARCH _ARCH_NUM_1
    #include "cmpscmem.c"
  #endif /* #ifdef _ARCH_NUM_1 */
  #ifdef _ARCH_NUM_2
    #undef _GEN_ARCH
    #define _GEN_ARCH _ARCH_NUM_2
    #include "cmpscmem.c"
  #endif /* #ifdef _ARCH_NUM_2 */
#endif /* #ifndef _GEN_ARCH */
