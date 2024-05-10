/* ZVECTOR.C    (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              z/Arch Vector Operations                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

#include "hstdinc.h"
#define _ZVECTOR_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined( FEATURE_129_ZVECTOR_FACILITY )
/*-------------------------------------------------------------------*/
/* E700 VLEB   - Vector Load Element (8)                       [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_8 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    regs->VR_B( v1, m3 ) = ARCH_DEP( vfetchb )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E701 VLEH   - Vector Load Element (16)                      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_16 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 7)                    /* M3 > 7 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_H( v1, m3 ) = ARCH_DEP( vfetch2 )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E702 VLEG   - Vector Load Element (64)                      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_64 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 1)                    /* M3 > 1 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_D( v1, m3 ) = ARCH_DEP( vfetch8 )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E703 VLEF   - Vector Load Element (32)                      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_32 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 3)                    /* M3 > 3 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_F( v1, m3 ) = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E704 VLLEZ  - Vector Load Logical Element and Zero          [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_logical_element_and_zero )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

#if defined(_M_X64) || defined( __SSE2__ )
    regs->VR_Q( v1 ).v = _mm_setzero_si128();
#else
    regs->VR_D(v1, 0) = 0x00;
    regs->VR_D(v1, 1) = 0x00;
#endif

    switch (m3)
    {
    case 0: regs->VR_B( v1, 7 ) = ARCH_DEP( vfetchb )( effective_addr2, b2, regs ); break;
    case 1: regs->VR_H( v1, 3 ) = ARCH_DEP( vfetch2 )( effective_addr2, b2, regs ); break;
    case 2: regs->VR_F( v1, 1 ) = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ); break;
    case 3: regs->VR_D( v1, 0 ) = ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ); break;
    case 6: regs->VR_F( v1, 0 ) = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ); break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E705 VLREP  - Vector Load and Replicate                     [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_and_replicate )
{
    int     v1, m3, x2, b2, i;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 0:
        regs->VR_B( v1, 0 ) = ARCH_DEP( vfetchb )( effective_addr2, b2, regs );
        for (i=1; i < 16; i++)
            regs->VR_B( v1, i ) = regs->VR_B( v1, 0 );
        break;
    case 1:
        regs->VR_H( v1, 0 ) = ARCH_DEP( vfetch2 )( effective_addr2, b2, regs );
        for (i=1; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_H( v1, 0 );
        break;
    case 2:
        regs->VR_F( v1, 0 ) = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );
        for (i=1; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v1, 0 );
        break;
    case 3:
        regs->VR_D( v1, 0 ) = ARCH_DEP( vfetch8 )( effective_addr2, b2, regs );
        regs->VR_D( v1, 1 ) = regs->VR_D( v1, 0 );
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E706 VL     - Vector Load                                   [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3);

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    regs->VR_Q( v1 ) = ARCH_DEP( vfetch16 )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E707 VLBB   - Vector Load to Block Boundary                 [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_to_block_boundary )
{
    int     v1, m3, x2, b2, length, i;
    VADR    effective_addr2, nextbound;
    U8      bytes[16];
    U64     boundary;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 6)                    /* M3 > 6 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    boundary = 64 << m3; /* 0: 64 Byte, 1: 128 Byte, 2: 256 Byte, 3: 512 Byte,
                                4: 1K - byte, 5: 2K - Byte, 6: 4K - Byte */

    nextbound = (effective_addr2 + boundary) & ~(boundary - 1);
    length = min( 16, nextbound - effective_addr2 );
    ARCH_DEP( vfetchc )( &bytes, length - 1, effective_addr2, b2, regs );

    for (i=0; i < length; i++)
        regs->VR_B( v1, i ) = bytes[i];

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E708 VSTEB  - Vector Store Element (8)                      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_element_8 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    ARCH_DEP( vstoreb )( regs->VR_B( v1, m3 ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E709 VSTEH  - Vector Store Element (16)                     [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_element_16 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 7)                    /* M3 > 7 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore2 )( regs->VR_H( v1, m3 ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E70A VSTEG  - Vector Store Element (64)                     [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_element_64 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 1)                    /* M3 > 1 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore8 )( regs->VR_D( v1, m3 ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E70B VSTEF  - Vector Store Element (32)                     [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_element_32 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 3)                    /* M3 > 3 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore4 )( regs->VR_F( v1, m3 ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E70E VST    - Vector Store                                  [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    ARCH_DEP( vstore16 )( regs->VR_Q( v1 ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E712 VGEG   - Vector Gather Element (64)                    [VRV] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_gather_element_64 )
{
    int      v1, v2, b2, d2, m3;
    VADR     effective_addr2;

    VRV( inst, regs, v1, v2, b2, d2, m3 );

    ZVECTOR_CHECK( regs );

    if (m3 > 1)                    /* M3 > 3 => Specification excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    effective_addr2 = d2;

    if (b2)
        effective_addr2 += regs->GR( b2 );

    effective_addr2 += regs->VR_D( v2, m3 );
    effective_addr2 &= ADDRESS_MAXWRAP( regs );

    PER_ZEROADDR_XCHECK( regs, b2 );

    regs->VR_D( v1, m3 ) = ARCH_DEP( vfetch8 )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E713 VGEF   - Vector Gather Element (32)                    [VRV] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_gather_element_32 )
{
    int      v1, v2, b2, d2, m3;
    VADR     effective_addr2;

    VRV( inst, regs, v1, v2, b2, d2, m3 );

    ZVECTOR_CHECK( regs );

    if (m3 > 3)                    /* M3 > 3 => Specification excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    effective_addr2 = d2;

    if (b2)
        effective_addr2 += regs->GR( b2 );

    effective_addr2 += regs->VR_F( v2, m3 );
    effective_addr2 &= ADDRESS_MAXWRAP( regs );

    PER_ZEROADDR_XCHECK( regs, b2 );

    regs->VR_F( v1, m3 ) = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E71A VSCEG  - Vector Scatter Element (64)                   [VRV] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_scatter_element_64 )
{
    int      v1, v2, b2, d2, m3;
    VADR     effective_addr2;

    VRV( inst, regs, v1, v2, b2, d2, m3 );

    ZVECTOR_CHECK( regs );

    if (m3 > 1)                    /* M3 > 3 => Specification excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    effective_addr2 = d2;

    if (b2)
        effective_addr2 += regs->GR( b2 );

    effective_addr2 += regs->VR_D( v2, m3 );
    effective_addr2 &= ADDRESS_MAXWRAP( regs );

    PER_ZEROADDR_XCHECK( regs, b2 );

    ARCH_DEP( vstore8 )( regs->VR_D( v1, m3 ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E71B VSCEF  - Vector Scatter Element (32)                   [VRV] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_scatter_element_32 )
{
    int      v1, v2, b2, d2, m3;
    VADR     effective_addr2;

    VRV( inst, regs, v1, v2, b2, d2, m3 );

    ZVECTOR_CHECK( regs );

    if (m3 > 3)                    /* M3 > 3 => Specification excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    effective_addr2 = d2;

    if (b2)
        effective_addr2 += regs->GR( b2 );

    effective_addr2 += regs->VR_F( v2, m3 );
    effective_addr2 &= ADDRESS_MAXWRAP( regs );

    PER_ZEROADDR_XCHECK( regs, b2 );

    ARCH_DEP( vstore4 )( regs->VR_F( v1, m3 ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E721 VLGV   - Vector Load GR from VR Element              [VRS-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_gr_from_vr_element )
{
    int     r1, v3, b2, d2, m4;

    VRS_C( inst, regs, r1, v3, b2, d2, m4 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0: regs->GR( r1 ) = regs->VR_B( v3, d2 ); break;
    case 1: regs->GR( r1 ) = regs->VR_H( v3, d2 ); break;
    case 2: regs->GR( r1 ) = regs->VR_F( v3, d2 ); break;
    case 3: regs->GR( r1 ) = regs->VR_D( v3, d2 ); break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E722 VLVG   - Vector Load VR Element from GR              [VRS-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_vr_element_from_gr )
{
    int     v1, r3, b2, d2, m4;

    VRS_B( inst, regs, v1, r3, b2, d2, m4 );

    ZVECTOR_CHECK( regs );

    if (m4 > 3 || d2 > (1 << m4)) /* m4 > elems or m4 > 3 => Specification excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0: regs->VR_B( v1, d2 ) = regs->GR_LHLCL( r3 ); break;
    case 1: regs->VR_H( v1, d2 ) = regs->GR_LHL  ( r3 ); break;
    case 2: regs->VR_F( v1, d2 ) = regs->GR_L    ( r3 ); break;
    case 3: regs->VR_D( v1, d2 ) = regs->GR_G    ( r3 ); break;
    default:
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E727 LCBB   - Load Count to Block Boundary                  [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_count_to_block_boundary )
{
    int     r1, x2, b2, m3, length;
    VADR    effective_addr2, nextbound;
    U64     boundary;

    RXE_M3( inst, regs, r1, x2, b2, effective_addr2, m3 );

    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 6)                    /* M3 > 6 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    boundary = 64 << m3; /* 0: 64 Byte, 1: 128 Byte, 2: 256 Byte, 3: 512 Byte,
                            4: 1K - byte, 5: 2K - Byte, 6: 4K - Byte */

    nextbound = (effective_addr2 + boundary) & ~(boundary - 1);
    length = min( 16, nextbound - effective_addr2 );

    regs->GR_L( r1 ) = length;
    regs->psw.cc = (length == 16) ? 0 : 3;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E730 VESL   - Vector Element Shift Left                   [VRS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_left )
{
    int     v1, v3, b2, m4, shift, i;
    VADR    effective_addr2;

    VRS_A( inst, regs, v1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:
        shift = b2 % 8;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = regs->VR_B( v3, i ) << shift;
        break;
    case 1:
        shift = b2 % 16;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_B( v3, i ) << shift;
        break;
    case 2:
        shift = b2 % 32;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v3, i ) << shift;
        break;
    case 3:
        shift = b2 % 64;
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = regs->VR_D( v3, i ) << shift;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E733 VERLL  - Vector Element Rotate Left Logical          [VRS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_rotate_left_logical )
{
    int     v1, v3, b2, m4, i, rotl, rotr;
    VADR    effective_addr2;

    VRS_A( inst, regs, v1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:
        rotl = b2 % 8;
        rotr = -rotl & 7;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = (regs->VR_B( v3, i ) << rotl) | (regs->VR_B( v3, i ) >> rotr);
        break;
    case 1:
        rotl = b2 % 16;
        rotr = -rotl & 15;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = (regs->VR_H( v3, i ) << rotl) | (regs->VR_H( v3, i ) >> rotr);
        break;
    case 2:
        rotl = b2 % 32;
        rotr = -rotl & 31;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = (regs->VR_F( v3, i ) << rotl) | (regs->VR_F( v3, i ) >> rotr);
        break;
    case 3:
        rotl = b2 % 64;
        rotr = -rotl & 63;
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = (regs->VR_D( v3, i ) << rotl) | (regs->VR_D( v3, i ) >> rotr);
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E736 VLM    - Vector Load Multiple                        [VRS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_multiple )
{
    int     v1, v3, b2, m4, i;
    VADR    effective_addr2;

    VRS_A( inst, regs, v1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    if (v3 < v1 || (v3 - v1 + 1) > 16)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    for (i=v1; i <= v3; i++)
    {
        regs->VR_Q( i ) = ARCH_DEP( vfetch16 )( effective_addr2, b2, regs );
        effective_addr2 += 16;
        effective_addr2 &= ADDRESS_MAXWRAP( regs );
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E737 VLL    - Vector Load With Length                     [VRS-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_with_length )
{
    int     v1, r3, b2, m4, i;
    VADR    effective_addr2;
    BYTE    temp[16];

    VRS_B( inst, regs, v1, r3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    memset(&temp, 0x00, sizeof(temp));

    ARCH_DEP( vfetchc )( &temp, min(regs->GR_L(r3), 15), effective_addr2, b2, regs );
    
    for(i=0; i < 16; i++)
        regs->VR_B(v1, i) = temp[i];

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E738 VESRL  - Vector Element Shift Right Logical          [VRS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_right_logical )
{
    int     v1, v3, b2, m4, i, shift;
    VADR    effective_addr2;

    VRS_A( inst, regs, v1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:
        shift = b2 % 8;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = regs->VR_B( v3, i ) >> shift;
        break;
    case 1:
        shift = b2 % 16;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_B( v3, i ) >> shift;
        break;
    case 2:
        shift = b2 % 32;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v3, i ) >> shift;
        break;
    case 3:
        shift = b2 % 64;
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = regs->VR_D( v3, i ) >> shift;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E73A VESRA  - Vector Element Shift Right Arithmetic       [VRS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_right_arithmetic )
{
    int     v1, v3, b2, m4, shift, i;
    VADR    effective_addr2;    

    VRS_A( inst, regs, v1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:
        shift = b2 % 8;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = (S8) regs->VR_B( v3, i ) >> shift;
        break;
    case 1:
        shift = b2 % 16;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = (S16) regs->VR_B( v3, i ) >> shift;
        break;
    case 2:
        shift = b2 % 32;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = (S32) regs->VR_F( v3, i ) >> shift;
        break;
    case 3:
        shift = b2 % 64;
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = (S64) regs->VR_D( v3, i ) >> shift;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E73E VSTM   - Vector Store Multiple                       [VRS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_multiple )
{
    int     v1, v3, b2, m4, i;
    VADR    effective_addr2;

    VRS_A( inst, regs, v1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    if (v3 < v1 || (v3 - v1 + 1) > 16)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    for (i = v1; i <= v3; i++)
    {
        ARCH_DEP( vstore16 )( regs->VR_Q( i ), effective_addr2, b2, regs );
        effective_addr2 += 16;
        effective_addr2 &= ADDRESS_MAXWRAP( regs );
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E73F VSTL   - Vector Store With Length                    [VRS-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_with_length )
{
    int     v1, r3, b2, m4, len, i;
    VADR    effective_addr2;
    BYTE    temp[16];

    VRS_B( inst, regs, v1, r3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );
    
    len = min(regs->GR_L(r3), 15);
    
    for (i = 0; i <= len; i++)
        temp[i] = regs->VR_B(v1, i);

    ARCH_DEP( vstorec )( &temp, len , effective_addr2, b2, regs );
    
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E740 VLEIB  - Vector Load Element Immediate (8)           [VRI-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_immediate_8 )
{
    int     v1, i2, m3;

    VRI_A( inst, regs, v1, i2, m3 );

    ZVECTOR_CHECK( regs );
    
    regs->VR_B(v1,m3) = i2 & 0xff;
    
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E741 VLEIH  - Vector Load Element Immediate (16)          [VRI-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_immediate_16 )
{
    int     v1, i2, m3;

    VRI_A( inst, regs, v1, i2, m3 );

    ZVECTOR_CHECK( regs );
    
    regs->VR_H(v1, m3) = (S16) i2;
    
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E742 VLEIG  - Vector Load Element Immediate (64)          [VRI-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_immediate_64 )
{
    int     v1, i2, m3;

    VRI_A( inst, regs, v1, i2, m3 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, m3) = (S64) i2;
    
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E743 VLEIF  - Vector Load Element Immediate (32)          [VRI-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_element_immediate_32 )
{
    int     v1, i2, m3;

    VRI_A( inst, regs, v1, i2, m3 );

    ZVECTOR_CHECK( regs );

    regs->VR_F(v1, m3) = (S32) i2;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E744 VGBM   - Vector Generate Byte Mask                   [VRI-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_generate_byte_mask )
{
    int     v1, i2, m3, i;

    VRI_A( inst, regs, v1, i2, m3 );

    ZVECTOR_CHECK( regs );

    for (i=0; i < 16; i++)
        regs->VR_B(v1, i) = (i2 & (0x1 << (15 - i))) ? 0xff : 0x00;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E745 VREPI  - Vector Replicate Immediate                  [VRI-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_replicate_immediate )
{
    int     v1, i2, m3, i;

    VRI_A( inst, regs, v1, i2, m3 );

    ZVECTOR_CHECK( regs );

    switch (m3) 
    {
        case 0:
            for (i=0; i < 16; i++)
                regs->VR_B(v1, i) = (S8) i2;
            break;
        case 1:
            for (i=0; i < 8; i++)
                regs->VR_H(v1, i) = (S16) i2;
            break;
        case 2:
            for (i=0; i < 4; i++)
                regs->VR_F(v1, i) = (S32) i2;
            break;
        case 3:
            for (i=0; i < 2; i++)
                regs->VR_D(v1, i) = (S64) i2;
            break;
        default:
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
            break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E746 VGM    - Vector Generate Mask                        [VRI-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_generate_mask )
{
    int     v1, i2, i3, m4, i;
    U64     bitmask;

    VRI_B( inst, regs, v1, i2, i3, m4 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:
        i2 &= 7;
        i3 &= 7;
        bitmask = (i2 <= i3) ? (1 << (8 - i2)) - (1 << (7 - i3)) : 0xffu - (1u << (7 - i3)) + (1u << (8 - i2));
        for (i=0; i < 16; i++)
            regs->VR_B(v1, i) = bitmask;
        break;
    case 1:
        i2 &= 15;
        i3 &= 15;
        bitmask = (i2 <= i3) ? (1 << (16 - i2)) - (1 << (15 - i3)) : 0xffffu - (1u << (15 - i3)) + (1u << (16 - i2));
        for (i=0; i < 8; i++)
            regs->VR_H(v1, i) = bitmask;
        break;
    case 2:
        i2 &= 31;
        i3 &= 31;
        bitmask = (i2 <= i3) ? (1 << (32 - i2)) - (1u << (31 - i3)) : 0xffffffffu - (1u << (31 - i3)) + (1u << (32 - i2));
        for (i=0; i < 4; i++)
            regs->VR_F(v1, i) = bitmask;
        break;
    case 3:
        i2 &= 63;
        i3 &= 63;
        bitmask = (i2 <= i3) ? (1ull << (64 - i2)) - (1ull << (63 - i3)) : 0xffffffffffffffffull - (1ull << (63 - i3)) + (1ull << (64 - i2));
        for (i=0; i < 2; i++)
            regs->VR_D(v1, i) = bitmask;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E74A VFTCI  - Vector FP Test Data Class Immediate         [VRI-e] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_test_data_class_immediate )
{
    int     v1, v2, i3, m4, m5;

    VRI_E( inst, regs, v1, v2, i3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E74D VREP   - Vector Replicate                            [VRI-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_replicate )
{
    int     v1, v3, i2, m4, i;

    VRI_C( inst, regs, v1, v3, i2, m4 );

    ZVECTOR_CHECK( regs );

    if (i2 >= (16 >> m4))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    
    switch (m4)
    {
    case 0:
        for (i=0; i < 16; i++)
            regs->VR_B(v1, i) = regs->VR_B(v3, i2);
        break;
    case 1:
        for (i=0; i < 8; i++)
            regs->VR_H(v1, i) = regs->VR_H(v3, i2);
        break;
    case 2:
        for (i=0; i < 4; i++)
            regs->VR_F(v1, i) = regs->VR_F(v3, i2);
        break;
    case 3:
        for (i=0; i < 2; i++)
            regs->VR_D(v1, i) = regs->VR_D(v3, i2);
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E750 VPOPCT - Vector Population Count                     [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_population_count )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E752 VCTZ   - Vector Count Trailing Zeros                 [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_count_trailing_zeros )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E753 VCLZ   - Vector Count Leading Zeros                  [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_count_leading_zeros )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E756 VLR    - Vector Load Vector                          [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_vector )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

    regs->VR_Q( v1 ) = regs->VR_Q( v2 );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E75C VISTR  - Vector Isolate String                       [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_isolate_string )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E75F VSEG   - Vector Sign Extend to Doubleword            [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_sign_extend_to_doubleword )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E760 VMRL   - Vector Merge Low                            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_merge_low )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E761 VMRH   - Vector Merge High                           [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_merge_high )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E762 VLVGP  - Vector Load VR from GRs Disjoint            [VRR-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_vr_from_grs_disjoint )
{
    int     v1, r2, r3;

    VRR_F( inst, regs, v1, r2, r3 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = regs->GR(r2);
    regs->VR_D(v1, 1) = regs->GR(r3);

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E764 VSUM   - Vector Sum Across Word                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_sum_across_word )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E765 VSUMG  - Vector Sum Across Doubleword                [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_sum_across_doubleword )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E766 VCKSM  - Vector Checksum                             [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_checksum )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E767 VSUMQ  - Vector Sum Across Quadword                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_sum_across_quadword )
{
    int     v1, v2, v3, m4, m5, m6;
    U64     high, low, add;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 2:
        regs->VR_D(v1, 0) = 0x00;
        regs->VR_D(v1, 1) = regs->VR_F(v2, 0) + regs->VR_F(v2, 1) + regs->VR_F(v2, 2) + regs->VR_F(v2, 3)
            + regs->VR_F(v3, 3);
        break;
    case 3:
        high = 0x00;
        low = regs->VR_D(v2, 0);
        add = low + regs->VR_D(v2, 1);
        if (add < low) high++;
        low = add;
        add = low + regs->VR_D(v3, 1);
        if (add < low) high++;
        regs->VR_D(v1, 0) = high;
        regs->VR_D(v1, 1) = add;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E768 VN     - Vector AND                                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_and )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = regs->VR_D(v2, 0) & regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = regs->VR_D(v2, 1) & regs->VR_D(v3, 1);
    
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E769 VNC    - Vector AND with Complement                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_and_with_complement )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E76A VO     - Vector OR                                   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_or )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E76B VNO    - Vector NOR                                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_nor )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E76D VX     - Vector Exclusive OR                         [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_exclusive_or )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = regs->VR_D(v2, 0) ^ regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = regs->VR_D(v2, 1) ^ regs->VR_D(v3, 1);

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E770 VESLV  - Vector Element Shift Left Vector            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_left_vector )
{
    int     v1, v2, v3, m4, m5, m6, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:
        for (i=0; i < 16; i++)
            regs->VR_B(v1, i) = regs->VR_B(v2, i) << (regs->VR_B(v3, i) % 8);
        break;
    case 1:
        for (i=0; i < 8; i++)
            regs->VR_H(v1, i) = regs->VR_H(v2, i) << (regs->VR_H(v3, i) % 16);
        break;
    case 2:
        for (i=0; i < 4; i++)
            regs->VR_F(v1, i) = regs->VR_F(v2, i) << (regs->VR_F(v3, i) % 32);
        break;
    case 3:
        for (i=0; i < 2; i++)
            regs->VR_D(v1, i) = regs->VR_D(v2, i) << (regs->VR_D(v3, i) % 64);
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E772 VERIM  - Vector Element Rotate and Insert Under Mask [VRI-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_rotate_and_insert_under_mask )
{
    int     v1, v2, v3, i4, m5;

    VRI_D( inst, regs, v1, v2, v3, i4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E773 VERLLV - Vector Element Rotate Left Logical Vector   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_rotate_left_logical_vector )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E774 VSL    - Vector Shift Left                           [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_left )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E775 VSLB   - Vector Shift Left By Byte                   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_left_by_byte )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E777 VSLDB  - Vector Shift Left Double By Byte            [VRI-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_left_double_by_byte )
{
    int     v1, v2, v3, i4, m5;

    VRI_D( inst, regs, v1, v2, v3, i4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E778 VESRLV - Vector Element Shift Right Logical Vector   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_right_logical_vector )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77A VESRAV - Vector Element Shift Right Arithmetic Vector [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_right_arithmetic_vector )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77C VSRL   - Vector Shift Right Logical                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_logical )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77D VSRLB  - Vector Shift Right Logical By Byte          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_logical_by_byte )
{
    int     v1, v2, v3, m4, m5, m6, shift, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    shift = (regs->VR_B(v3, 7) >> 3) & 0x0f;
    
    regs->VR_D(v1, 0) = 0x00;
    regs->VR_D(v1, 1) = 0x00;
    
    for (i=shift; i < 16; i++)
        regs->VR_B(v1, i) = regs->VR_B(v2, i - shift);

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77E VSRA   - Vector Shift Right Arithmetic               [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_arithmetic )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77F VSRAB  - Vector Shift Right Arithmetic By Byte       [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_arithmetic_by_byte )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E780 VFEE   - Vector Find Element Equal                   [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_find_element_equal )
{
    int     v1, v2, v3, m4, m5, ind1, ind2, max, i;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_RE ((m5 & 0xc) != 0) // Reserved
#define M5_ZS ((m5 & 0x2) != 0) // Zero Search
#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    if (m4 > 2 || M5_RE)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:
        ind1 = 16, ind2 = ind1, max = ind1;
        for (i=0; i < max && (ind1 == max || ind2 == max); i++)
        {
            if ((ind1 == max) && regs->VR_B(v2,i) == regs->VR_B(v3,i))
                ind1 = i;
            if ((ind2 == max) && M5_ZS && regs->VR_B(v2,i) == 0x00) // if M5-ZS (Zero Search)
                ind2 = i;
        }
        break;
    case 1:
        ind1 = 8, ind2 = ind1, max = ind1;
        for (i=0; i < max && (ind1 == max || ind2 == max); i++)
        {
            if ((ind1 == max) && regs->VR_H(v2,i) == regs->VR_H(v3,i))
                ind1 = i;
            if ((ind2 == max) && M5_ZS && regs->VR_H(v2, i) == 0x0000) // if M5-ZS (Zero Search)
                ind2 = i;
        }
        break;
    case 2:
        ind1 = 4, ind2 = ind1, max = ind1;
        for (i=0; i < max && (ind1 == max || ind2 == max); i++)
        {
            if ((ind1 == max) && regs->VR_F(v2,i) == regs->VR_F(v3,i))
                ind1 = i;
            if ((ind2 == max) && M5_ZS && regs->VR_F(v2,i) == 0x00000000) // if M5-ZS (Zero Search)
                ind2 = i;
        }
        break;
    default:
        break;
    }
    regs->VR_D(v1, 0) = 0x00;
    regs->VR_B(v1, 7) = min(ind1, ind2) * (1 << m4); 
    regs->VR_D(v1, 1) = 0x00;

    if (M5_CS)               // if M5_CS (Condition Code Set)
    {
        if (M5_ZS && (ind2 <= ind1))
            regs->psw.cc = 0;
        else if ((ind1 < max) && (ind2 == max))
            regs->psw.cc = 1;
        else if (M5_ZS && (ind1 < ind2) && (ind2 < max))
            regs->psw.cc = 2;
        else if ((ind1 == max) && (ind2 == max))
            regs->psw.cc = 3;
    }

#undef M5_RE
#undef M5_ZS
#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E781 VFENE  - Vector Find Element Not Equal               [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_find_element_not_equal )
{
    int     v1, v2, v3, m4, m5, i, ind1, ind2, max, match = 0;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_RE ((m5 & 0xc) != 0) // Reserved
#define M5_ZS ((m5 & 0x2) != 0) // Zero Search
#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    if (m4 > 2 || M5_RE)
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    
    switch (m4)
    {
    case 0:
        ind1 = 16, ind2 = ind1, max = ind1;
        for (i=0; i < max && (ind1 == max || ind2 == max); i++)
        {
            if ((ind1 == max) && regs->VR_B(v2,i) != regs->VR_B(v3,i))
            {
                match = (regs->VR_B(v2,i) < regs->VR_B(v3,i)) ? 1:2;
                ind1 = i;
            }
            if ((ind2 == max) && M5_ZS && regs->VR_B(v2, i) == 0x00) // if M5-ZS (Zero Search)
                ind2 = i;
        }
        break;
    case 1:
        ind1 = 8, ind2 = ind1, max = ind1;
        for (i=0; i < max && (ind1 == max || ind2 == max); i++)
        {
            if ((ind1 == max) && regs->VR_H(v2, i) != regs->VR_H(v3, i))
            {
                match = (regs->VR_H(v2, i) < regs->VR_H(v3, i)) ? 1 : 2;
                ind1 = i;
            }
            if ((ind2 == max) && M5_ZS && regs->VR_H(v2, i) == 0x0000) // if M5-ZS (Zero Search)
                ind2 = i;
        }
        break;
    case 2:
        ind1 = 4, ind2 = ind1, max = ind1;
        for (i=0; i < max && (ind1 == max || ind2 == max); i++)
        {
            if ((ind1 == max) && regs->VR_F(v2, i) != regs->VR_F(v3, i))
            {
                match = (regs->VR_F(v2, i) < regs->VR_F(v3, i)) ? 1 : 2;
                ind1 = i;
            }
            if ((ind2 == max) && M5_ZS && regs->VR_F(v2, i) == 0x00000000) // if M5-ZS (Zero Search)
                ind2 = i;
        }
        break;
    default:
        break;
    }
    regs->VR_D(v1, 0) = 0x00;
    regs->VR_B(v1, 7) = min(ind1, ind2) * (1 << m4);
    regs->VR_D(v1, 1) = 0x00;

    if (M5_CS)               // if M5_CS (Condition Code Set)
    {
        if (M5_ZS && (ind2 < ind1))
            regs->psw.cc = 0;
        else if (match)
            regs->psw.cc = match;
        else if ((ind1 == max) && (ind2 == max))
            regs->psw.cc = 3;
    }

#undef M5_RE
#undef M5_ZS
#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E782 VFAE   - Vector Find Any Element Equal               [VRR-b] */
/*-------------------------------------------------------------------*/
/* In PoP (SA22-7832-13), for VFAE & VFEE we can read:
"Programming Notes:
1. If the RT flag is zero, a byte index is always
stored into the first operand for any element size.
For example, if the specified element size is halfword
and the 2nd indexed halfword compared
equal, a byte index of 4 would be stored."

But I think that 4 would be a 2.
The 2nd Half = 1 (index byte) x 2 (length of H) = 2.

After 35 years, I must say this is the first typo I found in POP.
Sent to IBM, they will reformulate the sentence.

salva - 2023, feb,27.
*/

DEF_INST( vector_find_any_element_equal )
{
    int     v1, v2, v3, m4, m5, int1, ind1, ind2, max, i, j, s;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_IN ((m5 & 0x8) != 0) // Invert Result
#define M5_RT ((m5 & 0x4) != 0) // Result Type
#define M5_ZS ((m5 & 0x2) != 0) // Zero Search
#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    switch (m4)
    {
    case 0:
        max = 16, int1 = 0;  ind1 = max, ind2 = ind1;
        for (i=0; i < max; i++)
        {
            for (j=0; j < max; j++)
                if (regs->VR_B(v2, i) == regs->VR_B(v3, j))
                    int1 |= (1 << i);
            if (M5_ZS && (regs->VR_B(v2, i) == 0x00)) // if M5-ZS (Zero Search)
                ind2 = min(ind2, i);
        }
        if (M5_IN)
            int1 = ~int1;
        for (i=0; i < max; i++)
        {
            s = (int1 >> i) & 0x1;
            if (s)
                ind1 = min(ind1, i);
            if (M5_RT)
                regs->VR_B(v1, i) = s ? 0xff: 0x00;
        }
        break;
    case 1:
        max = 8, int1 = 0;  ind1 = max, ind2 = ind1;
        for (i=0; i < max; i++)
        {
            for (j=0; j < max; j++)
                if (regs->VR_H(v2, i) == regs->VR_H(v3, j))
                    int1 |= (1 << i);
            if (M5_ZS && (regs->VR_H(v2, i) == 0x0000)) // if M5-ZS (Zero Search)
                ind2 = min(ind2, i);
        }
        if (M5_IN)
            int1 = ~int1;
        for (i=0; i < max; i++)
        {
            s = (int1 >> i) & 0x1;
            if (s)
                ind1 = min(ind1, i);
            if (M5_RT)
                regs->VR_H(v1, i) = s ? 0xffff : 0x0000;
        }
        break;
    case 2:
        max = 4, int1 = 0;  ind1 = max, ind2 = ind1;
        for (i=0; i < max; i++)
        {
            for (j=0; j < max; j++)
                if (regs->VR_F(v2, i) == regs->VR_F(v3, j))
                    int1 |= (1 << i);
            if (M5_ZS && (regs->VR_F(v2, i) == 0x00)) // if M5-ZS (Zero Search)
                ind2 = min(ind2, i);
        }
        if (M5_IN)
            int1 = ~int1;
        for (i=0; i < max; i++)
        {
            s = (int1 >> i) & 0x1;
            if (s)
                ind1 = min(ind1, i);
            if (M5_RT)
                regs->VR_F(v1, i) = s ? 0xffffffff : 0x00000000;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }
    if (!M5_RT)               // if !M5_RT (No result Type)
    {
        regs->VR_D(v1, 0) = 0x00;
        regs->VR_B(v1, 7) = min(ind1, ind2) * (1 << m4);
        regs->VR_D(v1, 1) = 0x00;
    }

    if (M5_CS)               // if M5_CS (Condition Code Set)
    {
        if (M5_ZS && (ind1 >= ind2))
            regs->psw.cc = 0;
        else if ((ind1 < max) && !(M5_ZS && (ind2 == max)))
            regs->psw.cc = 1;
        else if (M5_ZS && (ind1 < max) && ind1 < ind2)
            regs->psw.cc = 2;
        else if ((ind1 == max) && (ind2 == max))
            regs->psw.cc = 3;
    }

#undef M5_IN
#undef M5_RT
#undef M5_ZS
#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E784 VPDI   - Vector Permute Doubleword Immediate         [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_permute_doubleword_immediate )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78A VSTRC  - Vector String Range Compare                 [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_string_range_compare )
{
    int     v1, v2, v3, v4, m5, m6, max, low1, low2, result1[16], result2[16], i, j, lr, rr;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M6_IN ((m6 & 0x8) != 0) // Invert Result
#define M6_RT ((m6 & 0x4) != 0) // Result Type
#define M6_ZS ((m6 & 0x2) != 0) // Zero Search
#define M6_CS ((m6 & 0x1) != 0) // Condition Code Set

    regs->VR_D(v1, 0) = 0x00;
    regs->VR_D(v1, 1) = 0x00;
    
    switch (m5)
    {
    case 0:
        max = 16, low1 = max, low2 = max;
        for (i=0; i < max; i++) {
            result1[i] = 0;
            result2[i] = M6_ZS & (regs->VR_B(v2, i) == 0x00);
            for (j=0; j < max; j+=2) {
                lr = 0, rr = 0;
                if ((regs->VR_B(v4, j)   & 0x80) && regs->VR_B(v2, i) == regs->VR_B(v3, j))   lr = 1;
                if ((regs->VR_B(v4, j)   & 0x40) && regs->VR_B(v2, i) <  regs->VR_B(v3, j))   lr = 1;
                if ((regs->VR_B(v4, j)   & 0x20) && regs->VR_B(v2, i) >  regs->VR_B(v3, j))   lr = 1;
                if ((regs->VR_B(v4, j+1) & 0x80) && regs->VR_B(v2, i) == regs->VR_B(v3, j+1)) rr = 1;
                if ((regs->VR_B(v4, j+1) & 0x40) && regs->VR_B(v2, i) <  regs->VR_B(v3, j+1)) rr = 1;
                if ((regs->VR_B(v4, j+1) & 0x20) && regs->VR_B(v2, i) >  regs->VR_B(v3, j+1)) rr = 1;
                result1[i] = (lr & rr) ^ M6_IN;
            }
            if (M6_RT) {
                regs->VR_B(v1, i) = result1[i] ? 0xff : 0x00;
            }
            if (result1[i]) low1 = min(low1, i);
            if (result2[i]) low2 = min(low2, i);
        }
        break;
    case 1:
        max = 8, low1 = max, low2 = max;
        for (i=0; i < max; i++) {
            result1[i] = 0;
            result2[i] = M6_ZS & (regs->VR_H(v2, i) == 0x00);
            for (j=0; j < max; j+=2) {
                lr = 0, rr = 0;
                if ((regs->VR_H(v4, j)   & 0x8000) && regs->VR_H(v2, i) == regs->VR_H(v3, j))   lr = 1;
                if ((regs->VR_H(v4, j)   & 0x4000) && regs->VR_H(v2, i) <  regs->VR_H(v3, j))   lr = 1;
                if ((regs->VR_H(v4, j)   & 0x2000) && regs->VR_H(v2, i) >  regs->VR_H(v3, j))   lr = 1;
                if ((regs->VR_H(v4, j+1) & 0x8000) && regs->VR_H(v2, i) == regs->VR_H(v3, j+1)) rr = 1;
                if ((regs->VR_H(v4, j+1) & 0x4000) && regs->VR_H(v2, i) <  regs->VR_H(v3, j+1)) rr = 1;
                if ((regs->VR_H(v4, j+1) & 0x2000) && regs->VR_H(v2, i) >  regs->VR_H(v3, j+1)) rr = 1;
                result1[i] = (lr & rr) ^ M6_IN;
            }
            if (M6_RT) {
                regs->VR_H(v1, i) = result1[i] ? 0xffff : 0x0000;
            }
            if (result1[i]) low1 = min(low1, i);
            if (result2[i]) low2 = min(low2, i);
        }
        break;
    case 2:
        max = 4, low1 = max, low2 = max;
        for (i=0; i < max; i++) {
            result1[i] = 0;
            result2[i] = M6_ZS & (regs->VR_F(v2, i) == 0x00);
            for (j=0; j < max; j+=2) {
                lr = 0, rr = 0;
                if ((regs->VR_F(v4, j)   & 0x8000) && regs->VR_F(v2, i) == regs->VR_F(v3, j))   lr = 1;
                if ((regs->VR_F(v4, j)   & 0x4000) && regs->VR_F(v2, i) <  regs->VR_F(v3, j))   lr = 1;
                if ((regs->VR_F(v4, j)   & 0x2000) && regs->VR_F(v2, i) >  regs->VR_F(v3, j))   lr = 1;
                if ((regs->VR_F(v4, j+1) & 0x8000) && regs->VR_F(v2, i) == regs->VR_F(v3, j+1)) rr = 1;
                if ((regs->VR_F(v4, j+1) & 0x4000) && regs->VR_F(v2, i) <  regs->VR_F(v3, j+1)) rr = 1;
                if ((regs->VR_F(v4, j+1) & 0x2000) && regs->VR_F(v2, i) >  regs->VR_F(v3, j+1)) rr = 1;
                result1[i] = (lr & rr) ^ M6_IN;
            }
            if (M6_RT) {
                regs->VR_F(v1, i) = result1[i] ? 0xffffffff : 0x00000000;
            }
            if (result1[i]) low1 = min(low1, i);
            if (result2[i]) low2 = min(low2, i);
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }
    if (!M6_RT) regs->VR_B(v1, 7) = min(low1, low2) * (1 << m5);;
    if (M6_CS) {               // if M6_CS (Condition Code Set)
        if (M6_ZS && (low1 >= low2))
            regs->psw.cc = 0;
        else if ((low1 < max) && (low2 == max))
            regs->psw.cc = 1;
        else if (M6_ZS && (low1 < max) && low1 < low2)
            regs->psw.cc = 2;
        else if ((low1 == max) && (low2 == max))
            regs->psw.cc = 3;
    }

#undef M6_IN
#undef M6_RT
#undef M6_ZS
#undef M6_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78C VPERM  - Vector Permute                              [VRR-e] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_permute )
{
    int     v1, v2, v3, v4, m5, m6, i;
    U8 temp[32];

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

    for (i=0; i < 16; i++) {
        temp[i] = regs->VR_B(v2, i);
        temp[i + 16] = regs->VR_B(v3, i);
    }
    for (i=0; i < 16; i++) {
        int x = regs->VR_B(v4, i) & 0x1f;
        regs->VR_B(v1, i) = temp[x];
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78D VSEL   - Vector Select                               [VRR-e] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_select )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 1) = (regs->VR_D(v4, 1) & regs->VR_D(v2, 1)) | (~regs->VR_D(v4, 1) & regs->VR_D(v3, 1));
    regs->VR_D(v1, 0) = (regs->VR_D(v4, 0) & regs->VR_D(v2, 0)) | (~regs->VR_D(v4, 0) & regs->VR_D(v3, 0));
    
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78E VFMS   - Vector FP Multiply and Subtract             [VRR-e] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_multiply_and_subtract )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78F VFMA   - Vector FP Multiply and Add                  [VRR-e] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_multiply_and_add )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E794 VPK    - Vector Pack                                 [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_pack )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E795 VPKLS  - Vector Pack Logical Saturate                [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_pack_logical_saturate)
{
    int     v1, v2, v3, m4, m5, sat = 0, i;
    U16     temp16[16];
    U32     temp32[8];
    U64     temp64[4];

    VRR_B(inst, regs, v1, v2, v3, m4, m5);

    ZVECTOR_CHECK(regs);

#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    switch (m4)
    {
    case 1:
        for (i=0; i < 8; i++) {
              temp16[i]   = regs->VR_H(v2, i);
              temp16[i+8] = regs->VR_H(v3, i);
        }
        for (i=0; i < 16; i++) {
            if (temp16[i] > 0xff) {
                regs->VR_B(v1, i) = 0xff;
                sat++;
            }
            else regs->VR_B(v1, i) = (U8) temp16[i];
        }
        break;
    case 2:
        for (i=0; i < 4; i++) {
              temp32[i]   = regs->VR_F(v2, i);
              temp32[i+4] = regs->VR_F(v3, i);
        }
        for (i=0; i < 8; i++) {
            if (temp32[i] > 0xffff) {
                regs->VR_H(v1, i) = 0xffff;
                sat++;
            }
            else regs->VR_H(v1, i) = (U16) temp32[i];
        }
        break;
    case 3:
        for (i=0; i < 2; i++) {
              temp64[i]   = regs->VR_D(v2, i);
              temp64[i+2] = regs->VR_D(v3, i);
        }
        for (i=0; i < 4; i++) {
            if (temp64[i] > 0xffffffff) {
                regs->VR_F(v1, i) = 0xffffffff;
                sat++;
            }
            else regs->VR_F(v1, i) = (U32) temp64[i];
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS) {               // if M5_CS (Condition Code Set)
        if (sat == 0) 
            regs->psw.cc = 0;
        else if (sat < 16)
            regs->psw.cc = 1;
        else if (sat == 16)
            regs->psw.cc = 3;
    }
#undef M5_CS

    ZVECTOR_END(regs);
}

/*-------------------------------------------------------------------*/
/* E797 VPKS   - Vector Pack Saturate                        [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_pack_saturate )
{
    int     v1, v2, v3, m4, m5;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A1 VMLH   - Vector Multiply Logical High                [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_logical_high )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A2 VML    - Vector Multiply Low                         [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_low )
{
    int     v1, v2, v3, m4, m5, m6, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:
        for (i=0; i < 16; i++)
            regs->VR_B(v1, i) = (regs->VR_B(v2, i) * regs->VR_B(v3, i)) & 0xff;
        break;
    case 1:
        for (i=0; i < 8; i++)
            regs->VR_H(v1, i) = (regs->VR_H(v2, i) * regs->VR_H(v3, i)) & 0xffff;
        break;
    case 2:
        for (i=0; i < 4; i++)
            regs->VR_F(v1, i) = (regs->VR_F(v2, i) * regs->VR_F(v3, i)) & 0xffffffff;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A3 VMH    - Vector Multiply High                        [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_high )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A4 VMLE   - Vector Multiply Logical Even                [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_logical_even )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A5 VMLO   - Vector Multiply Logical Odd                 [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_logical_odd )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A6 VME    - Vector Multiply Even                        [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_even )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A7 VMO    - Vector Multiply Odd                         [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_odd )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A9 VMALH  - Vector Multiply and Add Logical High        [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_logical_high )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AA VMAL   - Vector Multiply and Add Low                 [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_multiply_and_add_low)
{
    int     v1, v2, v3, v4, m5, m6, i;

    VRR_D(inst, regs, v1, v2, v3, v4, m5, m6);

    ZVECTOR_CHECK(regs);

    switch (m5)
    {
    case 0:
        for (i=0; i < 16; i++)
            regs->VR_B(v1, i) = (regs->VR_B(v2, i) * regs->VR_B(v3, i) + regs->VR_B(v4, i)) & 0xff;
        break;
    case 1:
        for (i=0; i < 8; i++)
            regs->VR_H(v1, i) = (regs->VR_H(v2, i) * regs->VR_H(v3, i) + regs->VR_H(v4, i)) & 0xffff;
        break;
    case 2:
        for (i=0; i < 4; i++)
            regs->VR_F(v1, i) = (regs->VR_F(v2, i) * regs->VR_F(v3, i) + regs->VR_F(v4, i)) & 0xffffffff;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END(regs);
}

/*-------------------------------------------------------------------*/
/* E7AB VMAH   - Vector Multiply and Add High                [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_high )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AC VMALE  - Vector Multiply and Add Logical Even        [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_logical_even )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AD VMALO  - Vector Multiply and Add Logical Odd         [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_logical_odd )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AE VMAE   - Vector Multiply and Add Even                [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_even )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AF VMAO   - Vector Multiply and Add Odd                 [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_odd )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7B4 VGFM   - Vector Galois Field Multiply Sum            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_galois_field_multiply_sum )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7B9 VACCC  - Vector Add With Carry Compute Carry         [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_with_carry_compute_carry )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7BB VAC    - Vector Add With Carry                       [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_with_carry )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7BC VGFMA  - Vector Galois Field Multiply Sum and Accumulate [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_galois_field_multiply_sum_and_accumulate )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7BD VSBCBI - Vector Subtract With Borrow Compute Borrow Indication [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_subtract_with_borrow_compute_borrow_indication )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7BF VSBI   - Vector Subtract With Borrow Indication      [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_subtract_with_borrow_indication )
{
    int     v1, v2, v3, v4, m5, m6;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C0 VCLGD  - Vector FP Convert to Logical 64-bit         [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_to_logical_64_bit )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C1 VCDLG  - Vector FP Convert from Logical 64-bit       [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_from_logical_64_bit )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C2 VCGD   - Vector FP Convert to Fixed 64-bit           [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_to_fixed_64_bit )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C3 VCDG   - Vector FP Convert from Fixed 64-bit         [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_from_fixed_64_bit )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C4 VFLL   - Vector FP Load Lengthened                   [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_load_lengthened )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C5 VFLR   - Vector FP Load Rounded                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_load_rounded )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7C7 VFI    - Vector Load FP Integer                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_fp_integer )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CA WFK    - Vector FP Compare and Signal Scalar         [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_and_signal_scalar )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CB WFC    - Vector FP Compare Scalar                    [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_scalar )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CC VFPSO  - Vector FP Perform Sign Operation            [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_perform_sign_operation )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7CE VFSQ   - Vector FP Square Root                       [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_square_root )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D4 VUPLL  - Vector Unpack Logical Low                   [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_logical_low )
{
    int     v1, v2, m3, m4, m5, i;
    U16      temp16[8];
    U32      temp32[4];
    U64      temp64[2];

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );

    switch (m3)
    {
    case 0:
        for (i = 0; i < 8; i++)
            temp16[i] = (U16) regs->VR_B(v2, i + 8);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp16[i];
        break;
    case 1:
        for (i = 0; i < 4; i++)
            temp32[i] = (U32) regs->VR_H(v2, i + 4);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp32[i];
        break;
    case 2:
        for (i = 0; i < 2; i++)
            temp64[i] = (U64) regs->VR_F(v2, i + 2);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp64[i];
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D5 VUPLH  - Vector Unpack Logical High                  [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_logical_high )
{
    int     v1, v2, m3, m4, m5, i;
    U16      temp16[8];
    U32      temp32[4];
    U64      temp64[2];

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    
    switch (m3)
    {
    case 0:
        for (i = 0; i < 8; i++)
            temp16[i] = (U16) regs->VR_B(v2, i);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp16[i];
        break;
    case 1:
        for (i = 0; i < 4; i++)
            temp32[i] = (U32) regs->VR_H(v2, i);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp32[i];
        break;
    case 2:
        for (i = 0; i < 2; i++)
            temp64[i] = (U64) regs->VR_F(v2, i);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp64[i];
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D6 VUPL   - Vector Unpack Low                           [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_low )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D7 VUPH   - Vector Unpack High                          [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_high )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D8 VTM    - Vector Test Under Mask                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_test_under_mask )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D9 VECL   - Vector Element Compare Logical              [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_compare_logical )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7DB VEC    - Vector Element Compare                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_compare )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7DE VLC    - Vector Load Complement                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_complement )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7DF VLP    - Vector Load Positive                        [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_positive )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E2 VFS    - Vector FP Subtract                          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_subtract )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E3 VFA    - Vector FP Add                               [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_add )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E5 VFD    - Vector FP Divide                            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_divide )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E7 VFM    - Vector FP Multiply                          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_multiply )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7E8 VFCE   - Vector FP Compare Equal                     [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_equal )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7EA VFCHE  - Vector FP Compare High or Equal             [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_high_or_equal )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7EB VFCH   - Vector FP Compare High                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_compare_high )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F0 VAVGL  - Vector Average Logical                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_average_logical )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F1 VACC   - Vector Add Compute Carry                    [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_compute_carry )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F2 VAVG   - Vector Average                              [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_average )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F3 VA     - Vector Add                                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_add)
{
    int     v1, v2, v3, m4, m5, m6, i;
    U64     high, low;
     
    VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);
    
    ZVECTOR_CHECK(regs);

    switch (m4)
    {
    case 0:
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) + (S8) regs->VR_B(v3, i);
        }
        break;
    case 1:
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) + (S16) regs->VR_H(v3, i);
        }
        break;
    case 2:
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) + (S32) regs->VR_F(v3, i);
        }
        break;
    case 3:
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64) regs->VR_D(v2, i) + (S64) regs->VR_D(v3, i);
        }
        break;
    case 4:
        high = regs->VR_D(v2, 0) + regs->VR_D(v3, 0);
        low  = regs->VR_D(v2, 1) + regs->VR_D(v3, 1);
        if (low < regs->VR_D(v2, 1))
            high++;
        regs->VR_D(v1, 0) = high;
        regs->VR_D(v1, 1) = low; 
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END(regs);
}

/*-------------------------------------------------------------------*/
/* E7F5 VSCBI  - Vector Subtract Compute Borrow Indication   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_subtract_compute_borrow_indication )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F7 VS     - Vector Subtract                             [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_subtract)
{
    int     v1, v2, v3, m4, m5, m6, i;
    U64     high, low;
    
     VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);
    
     ZVECTOR_CHECK(regs);

    switch (m4)
    {
    case 0:
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) - (S8) regs->VR_B(v3, i);
        }
        break;
    case 1:
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) - (S16) regs->VR_H(v3, i);
        }
        break;
    case 2:
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) - (S32) regs->VR_F(v3, i);
        }
        break;
    case 3:
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64)regs->VR_D(v2, i) - (S64)regs->VR_D(v3, i);
        }
        break;
    case 4:
        high = regs->VR_D(v2, 0) - regs->VR_D(v3, 0);
        low  = regs->VR_D(v2, 1) - regs->VR_D(v3, 2);
        if (low > regs->VR_D(v2, 1))
            high--;
        regs->VR_D(v1, 0) = high;
        regs->VR_D(v1, 1) = low;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }
    
    ZVECTOR_END(regs);
}

/*-------------------------------------------------------------------*/
/* E7F8 VCEQ   - Vector Compare Equal                        [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_compare_equal )
{
    int     v1, v2, v3, m4, m5, eq = 0, ne = 0, i;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set
    
    switch (m4)
    {
    case 0:
        for (i=0; i < 16; i++) {
            if (regs->VR_B(v2, i) == regs->VR_B(v3, i)) {
                regs->VR_B(v1, i) = 0xff;
                eq++;
            }
            else {
                regs->VR_B(v1, i) = 0x00;
                ne++;
            }
        }
        break;
    case 1:
        for (i=0; i < 8; i++) {
            if (regs->VR_H(v2, i) == regs->VR_H(v3, i)) {
                regs->VR_H(v1, i) = 0xffff;
                eq++;
            }
            else {
                regs->VR_H(v1, i) = 0x0000;
                ne++;
            }
        }
        break;
    case 2:
        for (i=0; i < 4; i++) {
            if (regs->VR_F(v2, i) == regs->VR_F(v3, i)) {
                regs->VR_F(v1, i) = 0xffffffff;
                eq++;
            }
            else {
                regs->VR_F(v1, i) = 0x00000000;
                ne++;
            }
        }
        break;
    case 3:
        for (i=0; i < 2; i++) {
            if (regs->VR_D(v2, i) == regs->VR_D(v3, i)) {
                regs->VR_D(v1, i) = 0xffffffffffffffff;
                eq++;
            }
            else {
                regs->VR_D(v1, i) = 0x0000000000000000;
                ne++;
            }
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS) {
        if (ne == 0)
            regs->psw.cc = 0;
        else if (eq > 0)
            regs->psw.cc = 1;
        else if (eq == 0)
            regs->psw.cc = 3;
    }

#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F9 VCHL   - Vector Compare High Logical                 [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_compare_high_logical )
{
    int     v1, v2, v3, m4, m5;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FB VCH    - Vector Compare High                         [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_compare_high )
{
    int     v1, v2, v3, m4, m5;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FC VMNL   - Vector Minimum Logical                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_minimum_logical )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FD VMXL   - Vector Maximum Logical                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_maximum_logical )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FE VMN    - Vector Minimum                              [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_minimum )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FF VMX    - Vector Maximum                              [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_maximum )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

#endif /* defined( FEATURE_129_ZVECTOR_FACILITY ) */

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "zvector.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "zvector.c"
  #endif

#endif /*!defined(_GEN_ARCH)*/
