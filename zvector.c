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

#include "zvector.h"

/* ====================================================================== */
/* ZVECTOR_END macro for debugging Vector instructions                    */
/* Note: block comments are used to avoid gcc                             */
/*       warning: multi-line comment [-Wcomment]                          */

// To display the results of all Vector instructions, uncomment the
// following three lines:-

/*
    #undef ZVECTOR_END
    #define ZVECTOR_END(_regs)                                      \
                ARCH_DEP(display_inst) (_regs, inst);
*/

// To display the results of specific Vector instructions, uncomment
// and modify the following four lines:-

/*
    #undef ZVECTOR_END
    #define ZVECTOR_END(_regs)                                      \
            if (inst[5] == 0x3E || inst[5] == 0x36)                 \
                ARCH_DEP(display_inst) (_regs, inst);
*/
/* ====================================================================== */

#if defined( FEATURE_129_ZVECTOR_FACILITY )

/*===================================================================*/
/* Achitecture Independent Routines                                  */
/*===================================================================*/

#if !defined(_ZVECTOR_ARCH_INDEPENDENT_)
#define _ZVECTOR_ARCH_INDEPENDENT_

/*-------------------------------------------------------------------*/
/* Vector-processing exception.                                      */
/*-------------------------------------------------------------------*/
/* Create the VXC from the VIX and the VIC.                          */
/* Bits 0-3 of the VXC are the vector index (VIX).                   */
/*   The VIX is the index of the source element that caused          */
/*   the trapping exception. If trapping conditions exist for        */
/*   multiple elements, the exception of the lowest-indexed          */
/*   source element is recognized.                                   */
/* Bits 4-7 of the VXC are the vector interrupt code (VIC).          */
/*-------------------------------------------------------------------*/
static void vector_processing_trap( REGS *regs, int vix, U32 vic )
{
    U32 vxc;
    vxc = ( vix << VXC_VIX_SHIFT ) | vic;  /* Build VXC            */
    regs->dxc = vxc;                       /* Save VXC in PSA      */
    regs->fpc &= ~FPC_DXC;                 /* Clear DXC/VXC in FPC */
    regs->fpc |= (vxc << FPC_DXC_SHIFT);   /* Insert VXC into FPC  */
    regs->program_interrupt( regs, PGM_VECTOR_PROCESSING_EXCEPTION );
}

/*-------------------------------------------------------------------*/
/* Retun MAJOR(A,B,C) or MINOR(A,B,C)                                */
/*-------------------------------------------------------------------*/
static inline U128 U128_major_minor( U128 a, U128 b, U128 c, bool ind )
{
    U128    answer, mask;
    int     bits;
    int     i;

    answer.Q.D.H.D = 0x0000000000000000ull;
    answer.Q.D.L.D = 0x0000000000000000ull;
    mask.Q.D.H.D = 0x8000000000000000ull;
    mask.Q.D.L.D = 0x0000000000000000ull;

    for (i=0; i<128; i++)
    {
        bits = 0;

        if ((a.Q.D.H.D & mask.Q.D.H.D) || (a.Q.D.L.D & mask.Q.D.L.D))
            bits++;
        if ((b.Q.D.H.D & mask.Q.D.H.D) || (b.Q.D.L.D & mask.Q.D.L.D))
            bits++;
        if ((c.Q.D.H.D & mask.Q.D.H.D) || (c.Q.D.L.D & mask.Q.D.L.D))
            bits++;

        if (bits >= 2)
        {
            answer.Q.D.H.D |= mask.Q.D.H.D;
            answer.Q.D.L.D |= mask.Q.D.L.D;
        }

        mask = U128_shrl( mask, 1 );
    }

    if (ind == TRUE) return answer;
    answer = U128_not(answer);
    return answer;
}

#endif /*!defined(_ZVECTOR_ARCH_INDEPENDENT_)*/

/*===================================================================*/
/* Achitecture Dependent Routines / Instructions                     */
/*===================================================================*/

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

    /* m3 - Alignment Hint: not used */
    UNREFERENCED( m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    regs->VR_Q( v1 ) = ARCH_DEP( vfetch16 )( effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E707 VLBB   - Vector Load To Block Boundary                 [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_to_block_boundary )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2, boundary_addr;
    U64     boundary;
    U64     length;
    QW      temp;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 6)                    /* M3 > 6 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    boundary = 64 << m3; /* 0: 64 Byte, 1: 128 Byte, 2: 256 Byte, 3: 512 Byte,
                            4: 1 K-byte, 5: 2 K-Byte, 6: 4 K-Byte */

    boundary_addr = (effective_addr2 + boundary) & ~(boundary - 1);

    length = boundary_addr - effective_addr2;
    if (length > 16) length = 16;
    length--;

    memset(&temp, 0x00, sizeof(temp));

    ARCH_DEP( vfetchc )( &temp, (U32)length, effective_addr2, b2, regs );

    regs->VR_Q( v1 ) = CSWAP128( temp );

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

    /* m3 - Alignment Hint: not used */
    UNREFERENCED( m3 );

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
/*                                                                   */
/* In PoP (SA22-7832-13), for VLGV we can read:                      */
/*   If the index specified by the second-operand address is         */
/*   greater than the highest numbered element in the third          */
/*   operand, of the specified element size, the result in the       */
/*   first operand is unpredictable.                                 */
/*                                                                   */
/* However, empirical evidence suggests that any index larger than   */
/* the highest numbered element is treated as the modulo of the      */
/* highest numbered element. This may be model dependant behaviour,  */
/* but this implementation will follow a models (z15) behaviour.     */
/*                                                                   */
DEF_INST( vector_load_gr_from_vr_element )
{
    int     r1, v3, b2, m4;
    VADR    effective_addr2;
    int     i;

    VRS_C( inst, regs, r1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );

    i = effective_addr2 & 0xFFF;  // Isolate the element index

    switch (m4)
    {
    case 0:  /* Byte */
        i %= 16;
        regs->GR( r1 ) = regs->VR_B( v3, i );
        break;
    case 1:  /* Halfword */
        i %= 8;
        regs->GR( r1 ) = regs->VR_H( v3, i );
        break;
    case 2:  /* Word */
        i %= 4;
        regs->GR( r1 ) = regs->VR_F( v3, i );
        break;
    case 3:  /* Doubleword */
        i %= 2;
        regs->GR( r1 ) = regs->VR_D( v3, i );
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E722 VLVG   - Vector Load VR Element from GR              [VRS-b] */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* In PoP (SA22-7832-13), for VLVG we can read:                      */
/*   If the index, specified by the second-operand address, is       */
/*   greater than the highest numbered element in the first          */
/*   operand, of the specified element size, it is unpredictable     */
/*   which element, if any, is replaced.                             */
/*                                                                   */
/* However, empirical evidence suggests that any index larger than   */
/* the highest numbered element is treated as the modulo of the      */
/* highest numbered element. This may be model dependant behaviour,  */
/* but this implementation will follow a models (z15) behaviour.     */
/*                                                                   */
DEF_INST( vector_load_vr_element_from_gr )
{
    int     v1, r3, b2, m4;
    VADR    effective_addr2;
    int     i;

    VRS_B( inst, regs, v1, r3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );

    i = effective_addr2 & 0xFFF;  // Isolate the element index

    switch (m4)
    {
    case 0:  /* Byte */
        i %= 16;
        regs->VR_B( v1, i ) = regs->GR_LHLCL( r3 );
        break;
    case 1:  /* Halfword */
        i %= 8;
        regs->VR_H( v1, i ) = regs->GR_LHL  ( r3 );
        break;
    case 2:  /* Word */
        i %= 4;
        regs->VR_F( v1, i ) = regs->GR_L    ( r3 );
        break;
    case 3:  /* Doubleword */
        i %= 2;
        regs->VR_D( v1, i ) = regs->GR_G    ( r3 );
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E727 LCBB   - Load Count To Block Boundary                  [RXE] */
/*-------------------------------------------------------------------*/
DEF_INST( load_count_to_block_boundary )
{
    int     r1, x2, b2, m3;
    VADR    effective_addr2, boundary_addr;
    U64     boundary;
    U64     length;

    RXE_M3( inst, regs, r1, x2, b2, effective_addr2, m3 );

    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 6)                    /* M3 > 6 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    boundary = 64 << m3; /* 0: 64 Byte, 1: 128 Byte, 2: 256 Byte, 3: 512 Byte,
                            4: 1 K-byte, 5: 2 K-Byte, 6: 4 K-Byte */

    boundary_addr = (effective_addr2 + boundary) & ~(boundary - 1);

    length = boundary_addr - effective_addr2;
    if (length > 16) length = 16;

    regs->GR_L( r1 ) = (U32)length;
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

    shift = effective_addr2 & 0xFFF;  // Isolate number of bit positions

    switch (m4)
    {
    case 0:
        shift %= 8;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = regs->VR_B( v3, i ) << shift;
        break;
    case 1:
        shift %= 16;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_H( v3, i ) << shift;
        break;
    case 2:
        shift %= 32;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v3, i ) << shift;
        break;
    case 3:
        shift %= 64;
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

    rotl = effective_addr2 & 0xFFF;  // Isolate number of bit positions

    switch (m4)
    {
    case 0:
        rotl %= 8;
        rotr = -rotl & 7;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = (regs->VR_B( v3, i ) << rotl) | (regs->VR_B( v3, i ) >> rotr);
        break;
    case 1:
        rotl %= 16;
        rotr = -rotl & 15;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = (regs->VR_H( v3, i ) << rotl) | (regs->VR_H( v3, i ) >> rotr);
        break;
    case 2:
        rotl %= 32;
        rotr = -rotl & 31;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = (regs->VR_F( v3, i ) << rotl) | (regs->VR_F( v3, i ) >> rotr);
        break;
    case 3:
        rotl %= 64;
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

    /* m4 - Alignment Hint: not used */
    UNREFERENCED( m4 );

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
    int     v1, r3, b2, m4;
    VADR    effective_addr2;
    U32     length;
    QW      temp;

    VRS_B( inst, regs, v1, r3, b2, effective_addr2, m4 );

    /* m4 - Alignment Hint: not used */
    UNREFERENCED( m4 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    length = regs->GR_L(r3);
    if (length > 15) length = 15;

    memset(&temp, 0x00, sizeof(temp));

    ARCH_DEP( vfetchc )( &temp, length, effective_addr2, b2, regs );

    regs->VR_Q( v1 ) = CSWAP128( temp );

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

    shift = effective_addr2 & 0xFFF;  // Isolate number of bit positions

    switch (m4)
    {
    case 0:
        shift %= 8;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = regs->VR_B( v3, i ) >> shift;
        break;
    case 1:
        shift %= 16;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_H( v3, i ) >> shift;
        break;
    case 2:
        shift %= 32;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v3, i ) >> shift;
        break;
    case 3:
        shift %= 64;
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

    shift = effective_addr2 & 0xFFF;  // Isolate number of bit positions

    switch (m4)
    {
    case 0:
        shift %= 8;
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = (S8) regs->VR_B( v3, i ) >> shift;
        break;
    case 1:
        shift %= 16;
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = (S16) regs->VR_H( v3, i ) >> shift;
        break;
    case 2:
        shift %= 32;
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = (S32) regs->VR_F( v3, i ) >> shift;
        break;
    case 3:
        shift %= 64;
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

    /* m4 - Alignment Hint: not used */
    UNREFERENCED( m4 );

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
    int     v1, r3, b2, m4;
    VADR    effective_addr2;
    U32     length;
    QW      temp;

    VRS_B( inst, regs, v1, r3, b2, effective_addr2, m4 );

    /* m4 is not part of this instruction */
    UNREFERENCED( m4 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    length = regs->GR_L(r3);
    if (length > 15) length = 15;

    temp = CSWAP128( regs->VR_Q( v1 ) );

    ARCH_DEP( vstorec )( &temp, length , effective_addr2, b2, regs );

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

    if (m3 > 15)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

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

    if (m3 > 7)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_H(v1, m3) = i2;

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

    if (m3 > 1)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    if (i2 & 0x8000)
        i2 |= 0xFFFF0000;

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

    if (m3 > 3)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    if (i2 & 0x8000)
        i2 |= 0xFFFF0000;

    regs->VR_F(v1, m3) = i2;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E744 VGBM   - Vector Generate Byte Mask                   [VRI-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_generate_byte_mask )
{
    int     v1, i2, m3, i;

    VRI_A( inst, regs, v1, i2, m3 );

    /* m3 is not part of this instruction */
    UNREFERENCED( m3 );

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

    if (i2 & 0x8000)
        i2 |= 0xFFFF0000;

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
        if (i2 <= i3) {
            if (i2 == 0)
                bitmask = 0u - (1u << (7 - i3));
            else
                bitmask = (1u << (8 - i2)) - (1u << (7 - i3));
        } else {
            if (i2 == 0)
                bitmask = 0xFFu - (1u << (7 - i3));
            else
                bitmask = 0xFFu - (1u << (7 - i3)) + (1u << (8 - i2));
        }
        for (i=0; i < 16; i++)
            regs->VR_B(v1, i) = bitmask;
        break;
    case 1:
        i2 &= 15;
        i3 &= 15;
        if (i2 <= i3) {
            if (i2 == 0)
                bitmask = 0u - (1u << (15 - i3));
            else
                bitmask = (1u << (16 - i2)) - (1u << (15 - i3));
        } else {
            if (i2 == 0)
                bitmask = 0xFFFFu - (1u << (15 - i3));
            else
                bitmask = 0xFFFFu - (1u << (15 - i3)) + (1u << (16 - i2));
        }
        for (i=0; i < 8; i++)
            regs->VR_H(v1, i) = bitmask;
        break;
    case 2:
        i2 &= 31;
        i3 &= 31;
        if (i2 <= i3) {
            if (i2 == 0)
                bitmask = 0u - (1u << (31 - i3));
            else
                bitmask = (1u << (32 - i2)) - (1u << (31 - i3));
        } else {
            if (i2 == 0)
                bitmask = 0xFFFFFFFFu - (1u << (31 - i3));
            else
                bitmask = 0xFFFFFFFFu - (1u << (31 - i3)) + (1u << (32 - i2));
        }
        for (i=0; i < 4; i++)
            regs->VR_F(v1, i) = bitmask;
        break;
    case 3:
        i2 &= 63;
        i3 &= 63;
        if (i2 <= i3) {
            if (i2 == 0)
                bitmask = 0ull - (1ull << (63 - i3));
            else
                bitmask = (1ull << (64 - i2)) - (1ull << (63 - i3));
        } else {
            if (i2 == 0)
                bitmask = 0xFFFFFFFFFFFFFFFFull - (1ull << (63 - i3));
            else
                bitmask = 0xFFFFFFFFFFFFFFFFull - (1ull << (63 - i3)) + (1ull << (64 - i2));
        }
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

    static const BYTE OneBitsInByte[256] =
    /*        -0  -1  -2  -3  -4  -5  -6  -7  -8  -9  -A  -B  -C  -D  -E  -F */
    /* 0- */ { 0,  1,  1,  2,  1,  2,  2,  3,  1,  2,  2,  3,  2,  3,  3,  4,
    /* 1- */   1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5,
    /* 2- */   1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5,
    /* 3- */   2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6,
    /* 4- */   1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5,
    /* 5- */   2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6,
    /* 6- */   2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6,
    /* 7- */   3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7,
    /* 8- */   1,  2,  2,  3,  2,  3,  3,  4,  2,  3,  3,  4,  3,  4,  4,  5,
    /* 9- */   2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6,
    /* A- */   2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6,
    /* B- */   3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7,
    /* C- */   2,  3,  3,  4,  3,  4,  4,  5,  3,  4,  4,  5,  4,  5,  5,  6,
    /* D- */   3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7,
    /* E- */   3,  4,  4,  5,  4,  5,  5,  6,  4,  5,  5,  6,  5,  6,  6,  7,
    /* F- */   4,  5,  5,  6,  5,  6,  6,  7,  5,  6,  6,  7,  6,  7,  7,  8 };

    int     v1, v2, m3, m4, m5;
    int     i, j, count;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( !FACILITY_ENABLED( 135_ZVECTOR_ENH_1, regs ) )
    {
        if ( m3 > 1 )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    switch (m3)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            count = OneBitsInByte[regs->VR_B(v2, i)];
            regs->VR_B(v1, i) = count;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            count = 0;
            for (j=i*2; j < (i*2)+2; j++)
            {
                count += OneBitsInByte[regs->VR_B(v2, j)];
            }
            regs->VR_H(v1, i) = count;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            count = 0;
            for (j=i*4; j < (i*4)+4; j++)
            {
                count += OneBitsInByte[regs->VR_B(v2, j)];
            }
            regs->VR_F(v1, i) = count;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            count = 0;
            for (j=i*8; j < (i*8)+8; j++)
            {
                count += OneBitsInByte[regs->VR_B(v2, j)];
            }
            regs->VR_D(v1, i) = count;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E752 VCTZ   - Vector Count Trailing Zeros                 [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_count_trailing_zeros )
{

    static const BYTE TrailingZerosInByte[256] =
    /*        -0  -1  -2  -3  -4  -5  -6  -7  -8  -9  -A  -B  -C  -D  -E  -F */
    /* 0- */ { 8,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 1- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 2- */   5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 3- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 4- */   6,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 5- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 6- */   5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 7- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 8- */   7,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* 9- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* A- */   5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* B- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* C- */   6,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* D- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* E- */   5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
    /* F- */   4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0 };

    int     v1, v2, m3, m4, m5;
    int     i, j, k, count;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            count = TrailingZerosInByte[regs->VR_B(v2, i)];
            regs->VR_B(v1, i) = count;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            count = 0;
            for (j=(i*2)+1; j >= i*2; j--)
            {
                k = TrailingZerosInByte[regs->VR_B(v2, j)];
                count += k;
                if (k != 8) break;
            }
            regs->VR_H(v1, i) = count;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            count = 0;
            for (j=(i*4)+3; j >= i*4; j--)
            {
                k = TrailingZerosInByte[regs->VR_B(v2, j)];
                count += k;
                if (k != 8) break;
            }
            regs->VR_F(v1, i) = count;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            count = 0;
            for (j=(i*8)+7; j >= i*8; j--)
            {
                k = TrailingZerosInByte[regs->VR_B(v2, j)];
                count += k;
                if (k != 8) break;
            }
            regs->VR_D(v1, i) = count;
        }
        break;
    case 4:  /* Quadword */
        count = 0;
        for (j=15; j >= 0; j--)
        {
            k = TrailingZerosInByte[regs->VR_B(v2, j)];
            count += k;
            if (k != 8) break;
        }
        regs->VR_D(v1, 0) = 0;
        regs->VR_D(v1, 1) = count;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E753 VCLZ   - Vector Count Leading Zeros                  [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_count_leading_zeros )
{

    static const BYTE LeadingZerosInByte[256] =
    /*        -0  -1  -2  -3  -4  -5  -6  -7  -8  -9  -A  -B  -C  -D  -E  -F */
    /* 0- */ { 8,  7,  6,  6,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  4,
    /* 1- */   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    /* 2- */   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    /* 3- */   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    /* 4- */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* 5- */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* 6- */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* 7- */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* 8- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* 9- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* A- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* B- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* C- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* D- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* E- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* F- */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };

    int     v1, v2, m3, m4, m5;
    int     i, j, k, count;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            count = LeadingZerosInByte[regs->VR_B(v2, i)];
            regs->VR_B(v1, i) = count;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            count = 0;
            for (j=i*2; j < (i*2)+2; j++)
            {
                k = LeadingZerosInByte[regs->VR_B(v2, j)];
                count += k;
                if (k != 8) break;
            }
            regs->VR_H(v1, i) = count;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            count = 0;
            for (j=i*4; j < (i*4)+4; j++)
            {
                k = LeadingZerosInByte[regs->VR_B(v2, j)];
                count += k;
                if (k != 8) break;
            }
            regs->VR_F(v1, i) = count;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            count = 0;
            for (j=i*8; j < (i*8)+8; j++)
            {
                k = LeadingZerosInByte[regs->VR_B(v2, j)];
                count += k;
                if (k != 8) break;
            }
            regs->VR_D(v1, i) = count;
        }
        break;
    case 4:  /* Quadword */
        count = 0;
        for (j=0; j < 16; j++)
        {
            k = LeadingZerosInByte[regs->VR_B(v2, j)];
            count += k;
            if (k != 8) break;
        }
        regs->VR_D(v1, 0) = 0;
        regs->VR_D(v1, 1) = count;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

#if defined( FEATURE_198_VECTOR_ENH_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E754 VGEM   - Vector Generate Element Masks               [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_generate_element_masks )
{
    int     v1, v2, m3, m4, m5;
    int     i;
    U32     mask;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    mask = regs->VR_F(v2, 0);

    switch (m3)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            if (mask & 0x80000000)
                regs->VR_B(v1, i) = 0xFF;
            else
                regs->VR_B(v1, i) = 0x00;
            mask <<= 1;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            if (mask & 0x80000000)
                regs->VR_H(v1, i) = 0xFFFF;
            else
                regs->VR_H(v1, i) = 0x0000;
            mask <<= 1;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            if (mask & 0x80000000)
                regs->VR_F(v1, i) = 0xFFFFFFFF;
            else
                regs->VR_F(v1, i) = 0x00000000;
            mask <<= 1;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            if (mask & 0x80000000)
                regs->VR_D(v1, i) = 0xFFFFFFFFFFFFFFFFull;
            else
                regs->VR_D(v1, i) = 0x0000000000000000ull;
            mask <<= 1;
        }
        break;
    case 4:  /* Quadword */
        if (mask & 0x80000000)
        {
            regs->VR_D(v1, 0) = 0xFFFFFFFFFFFFFFFFull;
            regs->VR_D(v1, 1) = 0xFFFFFFFFFFFFFFFFull;
        }
        else
        {
            regs->VR_D(v1, 0) = 0x0000000000000000ull;
            regs->VR_D(v1, 1) = 0x0000000000000000ull;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_198_VECTOR_ENH_FACILITY_3 ) */

/*-------------------------------------------------------------------*/
/* E756 VLR    - Vector Load Vector                          [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_vector )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m3, m4, m5 are not part of this instruction */
    UNREFERENCED( m3 );
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

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
    int     i;
    BYTE    newcc = 3;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4 is not part of this instruction */
    UNREFERENCED( m4 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0)  // Condition Code Set

    switch (m3)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            if (regs->VR_B(v2, i) != 0)
            {
                regs->VR_B(v1, i) = regs->VR_B(v2, i);
            }
            else
            {
                newcc = 0;
                for (; i < 16; i++)
                {
                    regs->VR_B(v1, i) = 0;
                }
                break;
            }
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            if (regs->VR_H(v2, i) != 0)
            {
                regs->VR_H(v1, i) = regs->VR_H(v2, i);
            }
            else
            {
                newcc = 0;
                for (; i < 8; i++)
                {
                    regs->VR_H(v1, i) = 0;
                }
                break;
            }
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            if (regs->VR_F(v2, i) != 0)
            {
                regs->VR_F(v1, i) = regs->VR_F(v2, i);
            }
            else
            {
                newcc = 0;
                for (; i < 4; i++)
                {
                    regs->VR_F(v1, i) = 0;
                }
                break;
            }
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS)               // if M5_CS (Condition Code Set)
        regs->psw.cc = newcc;

#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E75F VSEG   - Vector Sign Extend To Doubleword            [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_sign_extend_to_doubleword )
{
    int     v1, v2, m3, m4, m5;
    U64     element;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    switch (m3)
    {
    case 0:  /* Byte */
        element = regs->VR_B(v2, 7);
        if (element & 0x0000000000000080ull)
            element |= 0xFFFFFFFFFFFFFF00ull;
        regs->VR_D(v1, 0) = element;
        element = regs->VR_B(v2, 15);
        if (element & 0x0000000000000080ull)
            element |= 0xFFFFFFFFFFFFFF00ull;
        regs->VR_D(v1, 1) = element;
        break;
    case 1:  /* Halfword */
        element = regs->VR_H(v2, 3);
        if (element & 0x0000000000008000ull)
            element |= 0xFFFFFFFFFFFF0000ull;
        regs->VR_D(v1, 0) = element;
        element = regs->VR_H(v2, 7);
        if (element & 0x0000000000008000ull)
            element |= 0xFFFFFFFFFFFF0000ull;
        regs->VR_D(v1, 1) = element;
        break;
    case 2:  /* Word */
        element = regs->VR_F(v2, 1);
        if (element & 0x0000000080000000ull)
            element |= 0xFFFFFFFF00000000ull;
        regs->VR_D(v1, 0) = element;
        element = regs->VR_F(v2, 3);
        if (element & 0x0000000080000000ull)
            element |= 0xFFFFFFFF00000000ull;
        regs->VR_D(v1, 1) = element;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E760 VMRL   - Vector Merge Low                            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_merge_low )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j;
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for ( i=0, j=8; i<16; i+=2, j++ )
        {
            SV_B( temp, i   ) = regs->VR_B( v2, j );
            SV_B( temp, i+1 ) = regs->VR_B( v3, j );
        }
        break;
    case 1:  /* Halfword */
        for ( i=0, j=4; i<8; i+=2, j++ )
        {
            SV_H( temp, i   ) = regs->VR_H( v2, j );
            SV_H( temp, i+1 ) = regs->VR_H( v3, j );
        }
        break;
    case 2:  /* Word */
        for ( i=0, j=2; i<4; i+=2, j++ )
        {
            SV_F( temp, i   ) = regs->VR_F( v2, j );
            SV_F( temp, i+1 ) = regs->VR_F( v3, j );
        }
        break;
    case 3:  /* Doubleword */
        SV_D( temp, 0 ) = regs->VR_D( v2, 1 );
        SV_D( temp, 1 ) = regs->VR_D( v3, 1 );
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    regs->VR_D( v1, 0 ) = SV_D( temp, 0 );
    regs->VR_D( v1, 1 ) = SV_D( temp, 1 );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E761 VMRH   - Vector Merge High                           [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_merge_high )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j;
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for ( i=0, j=0; i<16; i+=2, j++ )
        {
            SV_B( temp, i   ) = regs->VR_B( v2, j );
            SV_B( temp, i+1 ) = regs->VR_B( v3, j );
        }
        break;
    case 1:  /* Halfword */
        for ( i=0, j=0; i<8; i+=2, j++ )
        {
            SV_H( temp, i   ) = regs->VR_H( v2, j );
            SV_H( temp, i+1 ) = regs->VR_H( v3, j );
        }
        break;
    case 2:  /* Word */
        for ( i=0, j=0; i<4; i+=2, j++ )
        {
            SV_F( temp, i   ) = regs->VR_F( v2, j );
            SV_F( temp, i+1 ) = regs->VR_F( v3, j );
        }
        break;
    case 3:  /* Doubleword */
        SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
        SV_D( temp, 1 ) = regs->VR_D( v3, 0 );
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    regs->VR_D( v1, 0 ) = SV_D( temp, 0 );
    regs->VR_D( v1, 1 ) = SV_D( temp, 1 );

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
    int     i, j;
    U32     sum[4];

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i = 0, j = 0; i < 4; i++, j+=4)
        {
            sum[i] = 0;
            sum[i] += regs->VR_B(v2, j+0);
            sum[i] += regs->VR_B(v2, j+1);
            sum[i] += regs->VR_B(v2, j+2);
            sum[i] += regs->VR_B(v2, j+3);
            sum[i] += regs->VR_B(v3, j+3);
        }
        break;
    case 1:  /* Halfword */
        for (i = 0, j = 0; i < 4; i++, j+=2)
        {
            sum[i] = 0;
            sum[i] += regs->VR_H(v2, j+0);
            sum[i] += regs->VR_H(v2, j+1);
            sum[i] += regs->VR_H(v3, j+1);
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    regs->VR_F(v1, 0) = sum[0];
    regs->VR_F(v1, 1) = sum[1];
    regs->VR_F(v1, 2) = sum[2];
    regs->VR_F(v1, 3) = sum[3];

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E765 VSUMG  - Vector Sum Across Doubleword                [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_sum_across_doubleword )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j;
    U64     sum[2];

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 1:  /* Halfword */
        for (i = 0, j = 0; i < 2; i++, j+=4)
        {
            sum[i] = 0;
            sum[i] += regs->VR_H(v2, j+0);
            sum[i] += regs->VR_H(v2, j+1);
            sum[i] += regs->VR_H(v2, j+2);
            sum[i] += regs->VR_H(v2, j+3);
            sum[i] += regs->VR_H(v3, j+3);
        }
        break;
    case 2:  /* Word */
        for (i = 0, j = 0; i < 2; i++, j+=2)
        {
            sum[i] = 0;
            sum[i] += regs->VR_F(v2, j+0);
            sum[i] += regs->VR_F(v2, j+1);
            sum[i] += regs->VR_F(v3, j+1);
        }
        break;
    default:
        /* remove initialization warning */
        sum[0] = 0;
        sum[1] = 0;

        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    regs->VR_D(v1, 0) = sum[0];
    regs->VR_D(v1, 1) = sum[1];

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E766 VCKSM  - Vector Checksum                             [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_checksum )
{
    int     v1, v2, v3, m4, m5, m6;
    U64     ksum;
    U32     carry[2];
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    ksum = 0;
    carry[0] = carry[1] = 0;

    for (i = 0; i < 4; i++)
    {
        ksum += regs->VR_F(v2, i);
        carry[1] = ksum >> 32;
        if (carry[0] != carry[1])
            ksum += 1;
        carry[0] = carry[1];
    }

    ksum += regs->VR_F(v3, 1);
    carry[1] = ksum >> 32;
    if (carry[0] != carry[1])
        ksum += 1;

    regs->VR_F(v1, 0) = 0;
    regs->VR_F(v1, 1) = ksum;
    regs->VR_D(v1, 1) = 0;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E767 VSUMQ  - Vector Sum Across Quadword                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_sum_across_quadword )
{
    int     v1, v2, v3, m4, m5, m6;
    U64     high, low, add;
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    high = low = add = 0;

    switch (m4)
    {
    case 2:  /* Word */
        for (i = 0; i < 4; i++)
        {
            add += regs->VR_F(v2, i);
        }
        add += regs->VR_F(v3, 3);
        break;
    case 3:  /* Doubleword */
        low = regs->VR_D(v2, 0);
        add = low + regs->VR_D(v2, 1);
        if (add < low) high++;
        low = add;
        add = low + regs->VR_D(v3, 1);
        if (add < low) high++;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    regs->VR_D(v1, 0) = high;
    regs->VR_D(v1, 1) = add;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E768 VN     - Vector AND                                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_and )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

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

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = regs->VR_D(v2, 0) & ~regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = regs->VR_D(v2, 1) & ~regs->VR_D(v3, 1);

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E76A VO     - Vector OR                                   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_or )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = regs->VR_D(v2, 0) | regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = regs->VR_D(v2, 1) | regs->VR_D(v3, 1);

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E76B VNO    - Vector NOR                                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_nor )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = ~(regs->VR_D(v2, 0) | regs->VR_D(v3, 0));
    regs->VR_D(v1, 1) = ~(regs->VR_D(v2, 1) | regs->VR_D(v3, 1));

    ZVECTOR_END( regs );
}

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*-------------------------------------------------------------------*/
/* E76C VNX    - Vector Not Exclusive OR                     [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_not_exclusive_or )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = ~(regs->VR_D(v2, 0) ^ regs->VR_D(v3, 0));
    regs->VR_D(v1, 1) = ~(regs->VR_D(v2, 1) ^ regs->VR_D(v3, 1));

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

/*-------------------------------------------------------------------*/
/* E76D VX     - Vector Exclusive OR                         [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_exclusive_or )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = regs->VR_D(v2, 0) ^ regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = regs->VR_D(v2, 1) ^ regs->VR_D(v3, 1);

    ZVECTOR_END( regs );
}

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*-------------------------------------------------------------------*/
/* E76E VNN    - Vector NAND                                 [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_nand )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = ~(regs->VR_D(v2, 0) & regs->VR_D(v3, 0));
    regs->VR_D(v1, 1) = ~(regs->VR_D(v2, 1) & regs->VR_D(v3, 1));

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*-------------------------------------------------------------------*/
/* E76F VOC    - Vector OR with Complement                   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_or_with_complement )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 0) = regs->VR_D(v2, 0) | ~regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = regs->VR_D(v2, 1) | ~regs->VR_D(v3, 1);

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

/*-------------------------------------------------------------------*/
/* E770 VESLV  - Vector Element Shift Left Vector            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_left_vector )
{
    int     v1, v2, v3, m4, m5, m6, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

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
    int     sl, sr;
    union   { U64 d[2]; U32 f[2]; U16 h[2]; BYTE b[2]; } temp;
    int     i;

    VRI_D( inst, regs, v1, v2, v3, i4, m5 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            sl = i4 % 8;
            sr = 8 - sl;
            temp.b[1] = ((regs->VR_B( v2, i ) << sl) | (regs->VR_B( v2, i ) >> sr)) & regs->VR_B( v3, i );
            temp.b[0] = regs->VR_B( v1, i ) & (~regs->VR_B( v3, i ));
            regs->VR_B( v1, i ) = temp.b[0] | temp.b[1];
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            sl = i4 % 16;
            sr = 16 - sl;
            temp.h[1] = ((regs->VR_H( v2, i ) << sl) | (regs->VR_H( v2, i ) >> sr)) & regs->VR_H( v3, i );
            temp.h[0] = regs->VR_H( v1, i ) & (~regs->VR_H( v3, i ));
            regs->VR_H( v1, i ) = temp.h[0] | temp.h[1];
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            sl = i4 % 32;
            sr = 32 - sl;
            temp.f[1] = ((regs->VR_F( v2, i ) << sl) | (regs->VR_F( v2, i ) >> sr)) & regs->VR_F( v3, i );
            temp.f[0] = regs->VR_F( v1, i ) & (~regs->VR_F( v3, i ));
            regs->VR_F( v1, i ) = temp.f[0] | temp.f[1];
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            sl = i4 % 64;
            sr = 64 - sl;
            temp.d[1] = ((regs->VR_D( v2, i ) << sl) | (regs->VR_D( v2, i ) >> sr)) & regs->VR_D( v3, i );
            temp.d[0] = regs->VR_D( v1, i ) & (~regs->VR_D( v3, i ));
            regs->VR_D( v1, i ) = temp.d[0] | temp.d[1];
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E773 VERLLV - Vector Element Rotate Left Logical Vector   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_rotate_left_logical_vector )
{
    int     v1, v2, v3, m4, m5, m6;
    int     sl, sr, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            sl = regs->VR_B( v3, i ) % 8;
            sr = 8 - sl;
            regs->VR_B( v1, i ) = (regs->VR_B( v2, i ) << sl) | (regs->VR_B( v2, i ) >> sr);
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            sl = regs->VR_H( v3, i ) % 16;
            sr = 16 - sl;
            regs->VR_H( v1, i ) = (regs->VR_H( v2, i ) << sl) | (regs->VR_H( v2, i ) >> sr);
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            sl = regs->VR_F( v3, i ) % 32;
            sr = 32 - sl;
            regs->VR_F( v1, i ) = (regs->VR_F( v2, i ) << sl) | (regs->VR_F( v2, i ) >> sr);
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            sl = regs->VR_D( v3, i ) % 64;
            sr = 64 - sl;
            regs->VR_D( v1, i ) = (regs->VR_D( v2, i ) << sl) | (regs->VR_D( v2, i ) >> sr);
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E774 VSL    - Vector Shift Left                           [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_left )
{
    int     v1, v2, v3, m4, m5, m6;
    int     sl, sr, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    for (i = 0 ; ; i++)
    {
        sl = regs->VR_B( v3, i ) & 0x07;
        if (i == 15)
        {
            regs->VR_B( v1, i ) = regs->VR_B( v2, i ) << sl;
            break;
        }
        sr = 8 - sl;
        regs->VR_B( v1, i ) = (regs->VR_B( v2, i ) << sl) | (regs->VR_B( v2, i+1 ) >> sr);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E775 VSLB   - Vector Shift Left By Byte                   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_left_by_byte )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j;
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = 0;
    SV_D( temp, 3 ) = 0;

    j = (regs->VR_B( v3, 7 ) & 0x78) >> 3;

    for (i = 0; i < 16; i++, j++)
        regs->VR_B(v1, i) = SV_B( temp, j );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E777 VSLDB  - Vector Shift Left Double By Byte            [VRI-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_left_double_by_byte )
{
    int     v1, v2, v3, i4, m5;
    int     i, j;
    SV      temp;

    VRI_D( inst, regs, v1, v2, v3, i4, m5 );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    for (i = 0, j = i4; i < 16; i++, j++)
        regs->VR_B(v1, i) = SV_B( temp, j );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E778 VESRLV - Vector Element Shift Right Logical Vector   [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_right_logical_vector )
{
    int     v1, v2, v3, m4, m5, m6;
    int     shift, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            shift = regs->VR_B( v3, i ) % 8;
            regs->VR_B( v1, i ) = regs->VR_B( v2, i ) >> shift;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            shift = regs->VR_H( v3, i ) % 16;
            regs->VR_H( v1, i ) = regs->VR_H( v2, i ) >> shift;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            shift = regs->VR_F( v3, i ) % 32;
            regs->VR_F( v1, i ) = regs->VR_F( v2, i ) >> shift;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            shift = regs->VR_D( v3, i ) % 64;
            regs->VR_D( v1, i ) = regs->VR_D( v2, i ) >> shift;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77A VESRAV - Vector Element Shift Right Arithmetic Vector[VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_shift_right_arithmetic_vector )
{
    int     v1, v2, v3, m4, m5, m6;
    int     shift, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            shift = regs->VR_B( v3, i ) % 8;
            regs->VR_B( v1, i ) = (S8) regs->VR_B( v2, i ) >> shift;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            shift = regs->VR_H( v3, i ) % 16;
            regs->VR_H( v1, i ) = (S16) regs->VR_H( v2, i ) >> shift;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            shift = regs->VR_F( v3, i ) % 32;
            regs->VR_F( v1, i ) = (S32) regs->VR_F( v2, i ) >> shift;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            shift = regs->VR_D( v3, i ) % 64;
            regs->VR_D( v1, i ) = (S64) regs->VR_D( v2, i ) >> shift;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77C VSRL   - Vector Shift Right Logical                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_logical )
{
    int     v1, v2, v3, m4, m5, m6;
    int     sr, sl, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    for (i = 15 ; ; i--)
    {
        sr = regs->VR_B( v3, i ) & 0x07;
        if (i == 0)
        {
            regs->VR_B( v1, i ) = regs->VR_B( v2, i ) >> sr;
            break;
        }
        sl = 8 - sr;
        regs->VR_B( v1, i ) = (regs->VR_B( v2, i ) >> sr) | (regs->VR_B( v2, i-1 ) << sl);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77D VSRLB  - Vector Shift Right Logical By Byte          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_logical_by_byte )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j;
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    SV_D( temp, 0 ) = 0;
    SV_D( temp, 1 ) = 0;
    SV_D( temp, 2 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v2, 1 );

    j = 16 - ((regs->VR_B( v3, 7 ) & 0x78) >> 3);

    for (i = 0; i < 16; i++, j++)
        regs->VR_B(v1, i) = SV_B( temp, j );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77E VSRA   - Vector Shift Right Arithmetic               [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_arithmetic )
{
    int     v1, v2, v3, m4, m5, m6;
    int     sr, sl, i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    for (i = 15 ; ; i--)
    {
        sr = regs->VR_B( v3, i ) & 0x07;
        if (i == 0)
        {
            regs->VR_B( v1, i ) = (S8)regs->VR_B( v2, i ) >> sr;
            break;
        }
        sl = 8 - sr;
        regs->VR_B( v1, i ) = (regs->VR_B( v2, i ) >> sr) | (regs->VR_B( v2, i-1 ) << sl);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E77F VSRAB  - Vector Shift Right Arithmetic By Byte       [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_arithmetic_by_byte )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j;
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if (regs->VR_B( v2, 0 ) & 0x80)
    {
        SV_D( temp, 0 ) = 0xFFFFFFFFFFFFFFFFull;
        SV_D( temp, 1 ) = 0xFFFFFFFFFFFFFFFFull;
    }
    else
    {
        SV_D( temp, 0 ) = 0;
        SV_D( temp, 1 ) = 0;
    }
    SV_D( temp, 2 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v2, 1 );

    j = 16 - ((regs->VR_B( v3, 7 ) & 0x78) >> 3);

    for (i = 0; i < 16; i++, j++)
        regs->VR_B(v1, i) = SV_B( temp, j );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E780 VFEE   - Vector Find Element Equal                   [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_find_element_equal )
{
    int     v1, v2, v3, m4, m5;
    int     ef, ei, zf, zi, i;
    BYTE    newcc;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_RE ((m5 & 0xc) != 0) // Reserved
#define M5_ZS ((m5 & 0x2) != 0) // Zero Search
#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    if (m4 > 2 || M5_RE)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    zf = ef = FALSE;
    zi = ei = 16;     // Number of bytes in vector

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i<16; i++)
        {
            if (regs->VR_B(v2,i) == regs->VR_B(v3,i))
            {
                ef = TRUE;
                ei = i;     // Element index in bytes
                break;
            }
        }
        if (M5_ZS)
        {
            for (i=0; i<16; i++)
            {
                if (regs->VR_B(v2,i) == 0)
                {
                    zf = TRUE;
                    zi = i;      // Zero element index in bytes
                    break;
                }
            }
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i<8; i++)
        {
            if (regs->VR_H(v2,i) == regs->VR_H(v3,i))
            {
                ef = TRUE;
                ei = i * 2;  // Element index in bytes
                break;
            }
        }
        if (M5_ZS)
        {
            for (i=0; i<8; i++)
            {
                if (regs->VR_H(v2,i) == 0)
                {
                    zf = TRUE;
                    zi = i * 2;  // Zero element index in bytes
                    break;
                }
            }
        }
        break;
    case 2:  /* Word */
        for (i=0; i<4; i++)
        {
            if (regs->VR_F(v2,i) == regs->VR_F(v3,i))
            {
                ef = TRUE;
                ei = i * 4;  // Element index in bytes
                break;
            }
        }
        if (M5_ZS)
        {
            for (i=0; i<4; i++)
            {
                if (regs->VR_F(v2,i) == 0)
                {
                    zf = TRUE;
                    zi = i * 4;  // Zero element index in bytes
                    break;
                }
            }
        }
        break;
    }

    if (ef == TRUE)
    {
        if (zf == TRUE)
        {
            if (zi <= ei)
            {
                newcc = 0;   // Equal element follows zero element, or equal element is zero element
                ei = zi;     // Element index in bytes
            }
            else
            {
                newcc = 2;   // Equal element before zero element
            }
        }
        else /* zf == FALSE */
        {
            newcc = 1;       // Equal element and, if M5_ZS, no zero element
        }
    }
    else  /* ef == FALSE */
    {
        if (zf == TRUE)
        {
            newcc = 0;       // No equal element and a zero element
            ei = zi;         // Element index in bytes
        }
        else /* zf == FALSE */
        {
            newcc = 3;       // No equal element and, if M5_ZS, no zero element
        }
    }

    regs->VR_D(v1, 0) = ei;
    regs->VR_D(v1, 1) = 0;

    if (M5_CS)
        regs->psw.cc = newcc;

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
    int     v1, v2, v3, m4, m5;
    int     nef, nei, zf, zi, i;
    BYTE    newcc;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_RE ((m5 & 0xc) != 0) // Reserved
#define M5_ZS ((m5 & 0x2) != 0) // Zero Search
#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    if (m4 > 2 || M5_RE)
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    zf = nef = FALSE;
    zi = nei = 16;  // Number of bytes in vector
    newcc = 3;  // All equal, no zero

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i<16; i++)
        {
            if (regs->VR_B(v2,i) != regs->VR_B(v3,i))
            {
                nef = TRUE;
                nei = i;     // Element index in bytes
                newcc = (regs->VR_B(v2,i) < regs->VR_B(v3,i)) ? 1 : 2;
                break;
            }
        }
        if (M5_ZS)
        {
            for (i=0; i<16; i++)
            {
                if (regs->VR_B(v2,i) == 0)
                {
                    zf = TRUE;
                    zi = i;      // Zero element index in bytes
                    break;
                }
            }
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i<8; i++)
        {
            if (regs->VR_H(v2,i) != regs->VR_H(v3,i))
            {
                nef = TRUE;
                nei = i * 2;  // Element index in bytes
                newcc = (regs->VR_H(v2,i) < regs->VR_H(v3,i)) ? 1 : 2;
                break;
            }
        }
        if (M5_ZS)
        {
            for (i=0; i<8; i++)
            {
                if (regs->VR_H(v2,i) == 0)
                {
                    zf = TRUE;
                    zi = i * 2;  // Zero element index in bytes
                    break;
                }
            }
        }
        break;
    case 2:  /* Word */
        for (i=0; i<4; i++)
        {
            if (regs->VR_F(v2,i) != regs->VR_F(v3,i))
            {
                nef = TRUE;
                nei = i * 4;  // Element index in bytes
                newcc = (regs->VR_F(v2,i) < regs->VR_F(v3,i)) ? 1 : 2;
                break;
            }
        }
        if (M5_ZS)
        {
            for (i=0; i<4; i++)
            {
                if (regs->VR_F(v2,i) == 0)
                {
                    zf = TRUE;
                    zi = i * 4;  // Zero element index in bytes
                    break;
                }
            }
        }
        break;
    }

    if (nef == TRUE)
    {
        if (zf == TRUE)
        {
            if (zi < nei)
            {
                newcc = 0;   // Not equal element follows zero element
                nei = zi;    // Element index in bytes
            }
            else
            {
             /* newcc = 1 or 2; */     // Not equal element before zero element, or not equal element is zero element
            }
        }
        else /* zf == FALSE */
        {
         /* newcc = 1 or 2; */         // Not equal element and, if M5_ZS, no zero element
        }
    }
    else  /* nef == FALSE */
    {
        if (zf == TRUE)
        {
            newcc = 0;       // No not equal (i.e. all equal) element and a zero element
            nei = zi;        // Element index in bytes
        }
        else /* zf == FALSE */
        {
         /* newcc = 3; */    // No not equal (i.e. all equal) element and, if M5_ZS, no zero element
        }
    }

    regs->VR_D(v1, 0) = nei;
    regs->VR_D(v1, 1) = 0;

    if (M5_CS)
        regs->psw.cc = newcc;

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

    int     v1, v2, v3, m4, m5;
    int     i, j;
    int     lxt1, lxt2;                // Lowest indexed true
    int     mxt;                       // Maximum indexed true
    BYTE    irt1[16], irt2[16];        // First and second intermediate results

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_IN ((m5 & 0x8) != 0) // Invert Result
#define M5_RT ((m5 & 0x4) != 0) // Result Type
#define M5_ZS ((m5 & 0x2) != 0) // Zero Search
#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    for (i=0; i<16; i++)
    {
        irt1[i] = irt2[i] = FALSE;
    }

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i<16; i++)
        {
            // Compare the element of the second with the elements of the third operands
            for (j=0; j<16; j++)
            {
                if (regs->VR_B(v2,i) == regs->VR_B(v3,j))
                {
                    irt1[i] = TRUE;
                    break;
                }
            }
            // Invert the result if required
            if (M5_IN) irt1[i] ^= TRUE;
            // Compare the element of the second operand with zero
            if (M5_ZS && regs->VR_B(v2,i) == 0) irt2[i] = TRUE;
        }
        if (M5_RT)                     // if M5_RT (Result Type)
        {
            for (i=0; i<16; i++)
            {
                regs->VR_B(v1, i) = (irt1[i] == TRUE) ? 0xFF : 0x00;
            }
        }
        else                           // else !M5_RT
        {
            lxt1 = lxt2 = 16;
            for (i=0; i<16; i++)
            {
                if (irt1[i] == TRUE && lxt1 == 16)
                    lxt1 = i;
                if (irt2[i] == TRUE && lxt2 == 16)
                    lxt2 = i;
            }
            regs->VR_D(v1, 0) = min(lxt1, lxt2);
            regs->VR_D(v1, 1) = 0;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i<8; i++)
        {
            // Compare the element of the second with the elements of the third operands
            for (j=0; j<8; j++)
            {
                if (regs->VR_H(v2,i) == regs->VR_H(v3,j))
                {
                    irt1[i] = TRUE;
                    break;
                }
            }
            // Invert the result if required
            if (M5_IN) irt1[i] ^= TRUE;
            // Compare the element of the second operand with zero
            if (M5_ZS && regs->VR_H(v2,i) == 0) irt2[i] = TRUE;
        }
        if (M5_RT)                     // if M5_RT (Result Type)
        {
            for (i=0; i<8; i++)
            {
                regs->VR_H(v1, i) = (irt1[i] == TRUE) ? 0xFFFF : 0x0000;
            }
        }
        else                           // else !M5_RT
        {
            lxt1 = lxt2 = 16;
            for (i=0; i<8; i++)
            {
                if (irt1[i] == TRUE && lxt1 == 16)
                    lxt1 = i * 2;
                if (irt2[i] == TRUE && lxt2 == 16)
                    lxt2 = i * 2;
            }
            regs->VR_D(v1, 0) = min(lxt1, lxt2);
            regs->VR_D(v1, 1) = 0;
        }
        break;
    case 2:  /* Word */
        for (i=0; i<4; i++)
        {
            // Compare the element of the second with the elements of the third operands
            for (j=0; j<4; j++)
            {
                if (regs->VR_F(v2,i) == regs->VR_F(v3,j))
                {
                    irt1[i] = TRUE;
                    break;
                }
            }
            // Invert the result if required
            if (M5_IN) irt1[i] ^= TRUE;
            // Compare the element of the second operand with zero
            if (M5_ZS && regs->VR_F(v2,i) == 0) irt2[i] = TRUE;
        }
        if (M5_RT)                     // if M5_RT (Result Type)
        {
            for (i=0; i<4; i++)
            {
                regs->VR_F(v1, i) = (irt1[i] == TRUE) ? 0xFFFFFFFF : 0x00000000;
            }
        }
        else                           // else !M5_RT
        {
            lxt1 = lxt2 = 16;
            for (i=0; i<4; i++)
            {
                if (irt1[i] == TRUE && lxt1 == 16)
                    lxt1 = i * 4;
                if (irt2[i] == TRUE && lxt2 == 16)
                    lxt2 = i * 4;
            }
            regs->VR_D(v1, 0) = min(lxt1, lxt2);
            regs->VR_D(v1, 1) = 0;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS)                         // if M5_CS (Condition Code Set)
    {
        switch (m4)
        {
        case 0:  /* Byte */
            lxt1 = lxt2 = mxt = 16;
            break;
        case 1:  /* Halfword */
            lxt1 = lxt2 = mxt = 8;
            break;
        case 2:  /* Word */
            lxt1 = lxt2 = mxt = 4;
            break;
        default:  // Prevent erroneous "may be used uninitialized" warnings
            lxt1 = lxt2 = mxt = 0;
            break;
        }

        for (i = 0; i < mxt; i++)
        {
            if (irt1[i] == TRUE && lxt1 == mxt)
                lxt1 = i;
            if (M5_ZS)                 // if M5_ZS (Zero Search)
            {
                if (irt2[i] == TRUE && lxt2 == mxt)
                    lxt2 = i;
            }
        }

        // cc 1 and 3 are possible when M5_ZS is 0 or 1.
        if (lxt1 == mxt && lxt2 == mxt)
            regs->psw.cc = 3;
        else if (lxt1 < mxt && lxt2 == mxt )
            regs->psw.cc = 1;
        // cc 0 and 2 are only possible when M5_ZS is 1.
        else if (lxt1 < lxt2)
            regs->psw.cc = 2;
        else
            regs->psw.cc = 0;
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
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M4_SO ((m4 & 0x4) != 0)  // Second operand index
#define M4_TO ((m4 & 0x1) != 0)  // Third operand index

    SV_D( temp, 0 ) = regs->VR_D( v2, M4_SO );
    SV_D( temp, 1 ) = regs->VR_D( v3, M4_TO );

    regs->VR_D( v1, 0 ) = SV_D( temp, 0 );
    regs->VR_D( v1, 1 ) = SV_D( temp, 1 );

#undef M4_SO
#undef M4_TO

    ZVECTOR_END( regs );
}

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*-------------------------------------------------------------------*/
/* E785 VBPERM - Vector Bit Permute                          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_bit_permute )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j, k;
    U16     wanted, result = 0;
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m4, m5, m6 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = 0;
    SV_D( temp, 3 ) = 0;

    // Each of the sixteen 1-byte elements in vector register v3
    // contains the bit number (0 to 255) of a bit in the source
    // vector.
    // 1. Calculate the number of the byte (0 to 31) in the source
    //    vector that contains the wanted bit number.
    // 2. Calculate the number of the bit (0 to 7) in the byte
    //    that is the wanted bit.
    // 3. Calculate the value (0 or 1) of the wanted bit, and
    // 4. If the wanted bit has a value of 1, place the bit in
    //    the result.
    // The bit value of the bit number in the first element of v3
    // becomes result bit 0, the bit value of the bit number in the
    // second element of v3 becomes result bit 1, and so on until
    // the bit value of the bit number in the sixteenth element of
    // v3 becomes result bit 15.
    for (i = 0; i < 16; i++)
    {
        j = regs->VR_B( v3, i ) / 8;
        k = regs->VR_B( v3, i ) % 8;
        wanted = SV_B( temp, j ) & ( 0x80 >> k );
        if (wanted)
        {
            result |= ( 0x0001 << ( 15 - i ) );
        }
    }

    regs->VR_D( v1, 0 ) = result;
    regs->VR_D( v1, 1 ) = 0;

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

/*-------------------------------------------------------------------*/
/* E786 VSLD   - Vector Shift Left Double By Bit             [VRI-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_left_double_by_bit )
{
    int     v1, v2, v3, i4, m5;
    int     i;
    U64     j, k;
    SV      temp;

    VRI_D( inst, regs, v1, v2, v3, i4, m5 );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if (i4 & 0xF8)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    if ( i4 > 0 )
        for (i = 0; i < 3; i++) {
            j = SV_D( temp, i ) << i4;
            k = SV_D( temp, i+1 ) >> ( 64 - i4 );
            SV_D( temp, i ) = j | k;
        }

    regs->VR_D( v1, 0 ) = SV_D( temp, 0 );
    regs->VR_D( v1, 1 ) = SV_D( temp, 1 );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E787 VSRD   - Vector Shift Right Double By Bit            [VRI-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_right_double_by_bit )
{
    int     v1, v2, v3, i4, m5;
    int     i;
    U64     j, k;
    SV      temp;

    VRI_D( inst, regs, v1, v2, v3, i4, m5 );

    /* m5 is not part of this instruction */
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if (i4 & 0xF8)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    if ( i4 > 0 )
        for (i = 3; i > 0; i--) {
            j = SV_D( temp, i-1 ) << ( 64 - i4 );
            k = SV_D( temp, i ) >> i4;
            SV_D( temp, i ) = j | k;
        }

    regs->VR_D( v1, 0 ) = SV_D( temp, 2 );
    regs->VR_D( v1, 1 ) = SV_D( temp, 3 );

    ZVECTOR_END( regs );
}

#if defined( FEATURE_198_VECTOR_ENH_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E788 VEVAL  - Vector Evaluate                             [VRI-k] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_evaluate )
{
    int     v1, v2, v3, v4, i5;
    U128    tempv1, tempv2, tempv3, tempv4;
    U128    tnotv2, tnotv3, tnotv4;
    U128    tempv1x, tempv1y, tempv2x, tempv2y, tempv3x, tempv3y, tempv4x, tempv4y;

    VRI_K( inst, regs, v1, v2, v3, v4, i5 );

    ZVECTOR_CHECK( regs );

    tempv1 = U128_zero();
    tempv2.Q = regs->VR_Q(v2);
    tempv3.Q = regs->VR_Q(v3);
    tempv4.Q = regs->VR_Q(v4);
    tnotv2 = U128_not( tempv2 );
    tnotv3 = U128_not( tempv3 );
    tnotv4 = U128_not( tempv4 );

    switch (i5)
    {
    /* Row 0 */
    case 0:     /* 00000 000  |||  */
        break;
    case 1:     /* 00000 001  AND(A,B,C)  */
        tempv1 = U128_and(tempv2, U128_and(tempv3, tempv4));
        break;
    case 2:     /* 00000 010  +++  */
        break;
    case 3:     /* 00000 011  |||  */
        break;
    case 4:     /* 00000 100  +++  */
        break;
    case 5:     /* 00000 101  |||  */
        break;
    case 6:     /* 00000 110  AND(A,XOR(B,C))  */
        tempv1 = U128_and(tempv2, U128_xor(tempv3, tempv4));
        break;
    case 7:     /* 00000 111  AND(A,OR(B,C))  */
        tempv1 = U128_and(tempv2, U128_or(tempv3, tempv4));
        break;
    /* Row 1 */
    case 8:     /* 00001 000  AND(A,NOR(B,C))  */
        tempv1 = U128_and(tempv2, U128_nor(tempv3, tempv4));
        break;
    case 9:     /* 00001 001  AND(A,NXOR(B,C))  */
        tempv1 = U128_and(tempv2, U128_nxor(tempv3, tempv4));
        break;
    case 10:    /* 00001 010  |||  */
        break;
    case 11:    /* 00001 011  +++  */
        break;
    case 12:    /* 00001 100  |||  */
        break;
    case 13:    /* 00001 101  +++  */
        break;
    case 14:    /* 00001 110  AND(A,NAND(B,C))  */
        tempv1 = U128_and(tempv2, U128_nand(tempv3, tempv4));
        break;
    case 15:    /* 00001 111  |||  */
        break;
    /* Row 2 */
    case 16:    /* 00010 000  NOR(A,NAND(B,C))  */
        tempv1 = U128_nor(tempv2, U128_nand(tempv3, tempv4));
        break;
    case 17:    /* 00010 001  |||  */
        break;
    case 18:    /* 00010 010  +++  */
        break;
    case 19:    /* 00010 011  +++  */
        break;
    case 20:    /* 00010 100  +++  */
        break;
    case 21:    /* 00010 101  +++  */
        break;
    case 22:    /* 00010 110  SEL(A,XOR(B,C),AND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_xor(tempv3x, tempv4x);
        tempv1y = U128_and(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 23:    /* 00010 111  MAJOR(A,B,C)  */
        tempv1 = U128_major_minor(tempv2, tempv3, tempv4, TRUE);
        break;
    /* Row 3 */
    case 24:    /* 00011 000  SEL(A,NOR(B,C),AND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nor(tempv3x, tempv4x);
        tempv1y = U128_and(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 25:    /* 00011 001  SEL(A,NXOR(B,C),AND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nxor(tempv3x, tempv4x);
        tempv1y = U128_and(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 26:    /* 00011 010  +++  */
        break;
    case 27:    /* 00011 011  |||  */
        break;
    case 28:    /* 00011 100  SEL(A,NOT(B),AND(B,C)) */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_not(tempv3x);
        tempv1y = U128_and(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 29:    /* 00011 101  |||  */
        break;
    case 30:    /* 00011 110  XOR(A,AND(B,C))  */
        tempv1 = U128_xor(tempv2, U128_and(tempv3, tempv4));
        break;
    case 31:    /* 00011 111  OR(A,AND(B,C))  */
        tempv1 = U128_or(tempv2, U128_and(tempv3, tempv4));
        break;
    /* Row 4 */
    case 32:    /* 00100 000  +++  */
        break;
    case 33:    /* 00100 001  +++  */
        break;
    case 34:    /* 00100 010  |||  */
        break;
    case 35:    /* 00100 011  +++  */
        break;
    case 36:    /* 00100 100  +++  */
        break;
    case 37:    /* 00100 101  +++  */
        break;
    case 38:    /* 00100 110  +++  */
        break;
    case 39:    /* 00100 111  |||  */
        break;
    /* Row 5 */
    case 40:    /* 00101 000  +++  */
        break;
    case 41:    /* 00101 001  +++  */
        break;
    case 42:    /* 00101 010  +++  */
        break;
    case 43:    /* 00101 011  +++  */
        break;
    case 44:    /* 00101 100  +++  */
        break;
    case 45:    /* 00101 101  +++  */
        break;
    case 46:    /* 00101 110  +++  */
        break;
    case 47:    /* 00101 111  +++  */
        break;
    /* Row 6 */
    case 48:    /* 00110 000  |||  */
        break;
    case 49:    /* 00110 001  +++  */
        break;
    case 50:    /* 00110 010  +++  */
        break;
    case 51:    /* 00110 011  |||  */
        break;
    case 52:    /* 00110 100  +++  */
        break;
    case 53:    /* 00110 101  |||  */
        break;
    case 54:    /* 00110 110  +++  */
        break;
    case 55:    /* 00110 111  +++  */
        break;
    /* Row 7 */
    case 56:    /* 00111 000  +++  */
        break;
    case 57:    /* 00111 001  +++  */
        break;
    case 58:    /* 00111 010  +++  */
        break;
    case 59:    /* 00111 011  +++  */
        break;
    case 60:    /* 00111 100  |||  */
        break;
    case 61:    /* 00111 101  +++  */
        break;
    case 62:    /* 00111 110  +++  */
        break;
    case 63:    /* 00111 111  |||  */
        break;
    /* Row 8 */
    case 64:    /* 01000 000  +++  */
        break;
    case 65:    /* 01000 001  +++  */
        break;
    case 66:    /* 01000 010  +++  */
        break;
    case 67:    /* 01000 011  +++  */
        break;
    case 68:    /* 01000 100  |||  */
        break;
    case 69:    /* 01000 101  +++  */
        break;
    case 70:    /* 01000 110  +++  */
        break;
    case 71:    /* 01000 111  |||  */
        break;
    /* Row 9  */
    case 72:    /* 01001 000  +++  */
        break;
    case 73:    /* 01001 001  +++  */
        break;
    case 74:    /* 01001 010  +++  */
        break;
    case 75:    /* 01001 011  +++  */
        break;
    case 76:    /* 01001 100  +++  */
        break;
    case 77:    /* 01001 101  +++  */
        break;
    case 78:    /* 01001 110  +++  */
        break;
    case 79:    /* 01001 111  +++  */
        break;
    /* Row 10 */
    case 80:    /* 01010 000  |||  */
        break;
    case 81:    /* 01010 001  SEL(A,AND(B,C),C)  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_and(tempv3x, tempv4x);
        tempv1y = tempv4y;
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 82:    /* 01010 010  +++  */
        break;
    case 83:    /* 01010 011  |||  */
        break;
    case 84:    /* 01010 100  +++  */
        break;
    case 85:    /* 01010 101  |||  */
        break;
    case 86:    /* 01010 110  +++  */
        break;
    case 87:    /* 01010 111  +++  */
        break;
    /* Row 11 */
    case 88:    /* 01011 000  SEL(A,NOR(B,C),C)  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nor(tempv3x, tempv4x);
        tempv1y = tempv4y;
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 89:    /* 01011 001  SEL(A,NXOR(B,C),C)  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nxor(tempv3x, tempv4x);
        tempv1y = tempv4y;
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 90:    /* 01011 010  |||  */
        break;
    case 91:    /* 01011 011   +++  */
        break;
    case 92:    /* 01011 100  SEL(A,NOT(B),C)  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_not(tempv3x);
        tempv1y = tempv4y;
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 93:    /* 01011 101  +++  */
        break;
    case 94:    /* 01011 110  SEL(A,NAND(B,C),C)  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nand(tempv3x, tempv4x);
        tempv1y = tempv4y;
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 95:    /* 01011 111  |||  */
        break;
    /* Row 12 */
    case 96:    /* 01100 000  NOR(A,NXOR(B,C))  */
        tempv1 = U128_nor(tempv2, U128_nxor(tempv3, tempv4));
        break;
    case 97:    /* 01100 001  SEL(A,AND(B,C),XOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_and(tempv3x, tempv4x);
        tempv1y = U128_xor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 98:    /* 01100 010   +++  */
        break;
    case 99:    /* 01100 011  SEL(A,B,XOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = tempv3x;
        tempv1y = U128_xor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 100:   /* 01100 100  +++  */
        break;
    case 101:   /* 01100 101  +++  */
        break;
    case 102:   /* 01100 110  |||  */
        break;
    case 103:   /* 01100 111  SEL(A,OR(B,C),XOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_or(tempv3x, tempv4x);
        tempv1y = U128_xor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    /* Row 13 */
    case 104:   /* 01101 000  SEL(A,NOR(B,C),XOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nor(tempv3x, tempv4x);
        tempv1y = U128_xor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 105:   /* 01101 001  XOR(A,B,C)  */
        tempv1 = U128_xor(tempv2, U128_xor(tempv3, tempv4));
        break;
    case 106:   /* 01101 010  +++  */
        break;
    case 107:   /* 01101 011  +++  */
        break;
    case 108:   /* 01101 100  +++  */
        break;
    case 109:   /* 01101 101  +++  */
        break;
    case 110:   /* 01101 110  +++  */
        break;
    case 111:   /* 01101 111  OR(A,XOR(B,C))  */
        tempv1 = U128_or(tempv2, U128_xor(tempv3, tempv4));
        break;
    /* Row 14 */
    case 112:   /* 01110 000  NOR(A,NOR(B,C))  */
        tempv1 = U128_nor(tempv2, U128_nor(tempv3, tempv4));
        break;
    case 113:   /* 01110 001  SEL(A,AND(B,C),OR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_and(tempv3x, tempv4x);
        tempv1y = U128_or(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 114:   /* 01110 010  +++  */
        break;
    case 115:   /* 01110 011  SEL(A,B,OR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = tempv3x;
        tempv1y = U128_or(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 116:   /* 01110 100  +++  */
        break;
    case 117:   /* 01110 101  +++  */
        break;
    case 118:   /* 01110 110  +++  */
        break;
    case 119:   /* 01110 111  |||  */
        break;
    /* Row 15 */
    case 120:   /* 01111 000  XOR(A,OR(B,C))  */
        tempv1 = U128_xor(tempv2, U128_or(tempv3, tempv4));
        break;
    case 121:   /* 01111 001  SEL(A,NXOR(B,C),OR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nxor(tempv3x, tempv4x);
        tempv1y = U128_or(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 122:   /* 01111 010  +++  */
        break;
    case 123:   /* 01111 011  +++  */
        break;
    case 124:   /* 01111 100  SEL(A,NOT(B),OR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_not(tempv3x);
        tempv1y = U128_or(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 125:   /* 01111 101  +++  */
        break;
    case 126:   /* 01111 110  SEL(A,NAND(B,C),OR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nand(tempv3x, tempv4x);
        tempv1y = U128_or(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 127:   /* 01111 111  OR(A,B,C)  */
        tempv1 = U128_or(tempv2, U128_or(tempv3, tempv4));
        break;
    /* Row 16 */
    case 128:   /* 10000 000  NOR(A,B,C)  */
        tempv1 = U128_nor(tempv2, U128_nor(tempv3, tempv4));
        break;
    case 129:   /* 10000 001  SEL(A,AND(B,C),NOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_and(tempv3x, tempv4x);
        tempv1y = U128_nor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 130:   /* 10000 010  +++  */
        break;
    case 131:   /* 10000 011  SEL(A,B,NOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = tempv3x;
        tempv1y = U128_nor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 132:   /* 10000 100  +++  */
        break;
    case 133:   /* 10000 101  +++  */
        break;
    case 134:   /* 10000 110  SEL(A,XOR(B,C),NOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_xor(tempv3x, tempv4x);
        tempv1y = U128_nor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 135:   /* 10000 111  NXOR(A,OR(B,C))  */
        tempv1 = U128_nxor(tempv2, U128_or(tempv3, tempv4));
        break;
    /* Row 17 */
    case 136:   /* 10001 000  |||  */
        break;
    case 137:   /* 10001 001  +++  */
        break;
    case 138:   /* 10001 010  +++  */
        break;
    case 139:   /* 10001 011  +++  */
        break;
    case 140:   /* 10001 100  SEL(A,NOT(B),NOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_not(tempv3x);
        tempv1y = U128_nor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 141:   /* 10001 101  +++  */
        break;
    case 142:   /* 10001 110  SEL(A,NAND(B,C),NOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nand(tempv3x, tempv4x);
        tempv1y = U128_nor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 143:   /* 10001 111  OR(A,NOR(B,C))  */
        tempv1 = U128_or(tempv2, U128_nor(tempv3, tempv4));
        break;
    /* Row 18 */
    case 144:   /* 10010 000  NOR(A,XOR(B,C))  */
        tempv1 = U128_nor(tempv2, U128_xor(tempv3, tempv4));
        break;
    case 145:   /* 10010 001  +++  */
        break;
    case 146:   /* 10010 010  +++  */
        break;
    case 147:   /* 10010 011  +++  */
        break;
    case 148:   /* 10010 100  +++  */
        break;
    case 149:   /* 10010 101  +++  */
        break;
    case 150:   /* 10010 110  NXOR(A,B,C)  */
        tempv1 = U128_nxor(tempv2, U128_nxor(tempv3, tempv4));
        break;
    case 151:   /* 10010 111  SEL(A,OR(B,C),NXOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_or(tempv3x, tempv4x);
        tempv1y = U128_nxor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    /* Row 19 */
    case 152:   /* 10011 000  SEL(A,NOR(B,C),NXOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nor(tempv3x, tempv4x);
        tempv1y = U128_nxor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 153:   /* 10011 001  |||  */
        break;
    case 154:   /* 10011 010  +++  */
        break;
    case 155:   /* 10011 011  +++  */
        break;
    case 156:   /* 10011 100  SEL(A,NOT(B),NXOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_not(tempv3x);
        tempv1y = U128_nxor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 157:   /* 10011 101  +++  */
        break;
    case 158:   /* 10011 110  SEL(A,NAND(B,C),NXOR(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nand(tempv3x, tempv4x);
        tempv1y = U128_nxor(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 159:   /* 10011 111  OR(A,NXOR(B,C))  */
        tempv1 = U128_or(tempv2, U128_nxor(tempv3, tempv4));
        break;
    /* Row 20 */
    case 160:   /* 10100 000 * |||  */
        break;
    case 161:   /* 10100 001  SEL(A,AND(B,C),NOT(C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_and(tempv3x, tempv4x);
        tempv1y = U128_not(tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 162:   /* 10100 010  +++  */
        break;
    case 163:   /* 10100 011  SEL(A,B,NOT(C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = tempv3x;
        tempv1y = U128_not(tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 164:   /* 10100 100  +++  */
        break;
    case 165:   /* 10100 101  |||  */
        break;
    case 166:   /* 10100 110  SEL(A,XOR(B,C),NOT(C)  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_xor(tempv3x, tempv4x);
        tempv1y = U128_not(tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 167:   /* 10100 111  SEL(A,OR(B,C),NOT(C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_or(tempv3x, tempv4x);
        tempv1y = U128_not(tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    /* Row 21 */
    case 168:   /* 10101 000  +++  */
        break;
    case 169:   /* 10101 001  +++  */
        break;
    case 170:   /* 10101 010  |||  */
        break;
    case 171:   /* 10101 011  +++  */
        break;
    case 172:   /* 10101 100  SEL(A,NOT(B),NOT(C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_not(tempv3x);
        tempv1y = U128_not(tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 173:   /* 10101 101  +++  */
        break;
    case 174:   /* 10101 110  SEL(A,NAND(B,C),NOT(C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nand(tempv3x, tempv4x);
        tempv1y = U128_not(tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 175:   /* 10101 111  |||  */
        break;
    /* Row 22 */
    case 176:   /* 10110 000  +++  */
        break;
    case 177:   /* 10110 001  +++  */
        break;
    case 178:   /* 10110 010  +++  */
        break;
    case 179:   /* 10110 011  +++  */
        break;
    case 180:   /* 10110 100  +++  */
        break;
    case 181:   /* 10110 101  +++  */
        break;
    case 182:   /* 10110 110  +++  */
        break;
    case 183:   /* 10110 111  +++  */
        break;
    /* Row 23 */
    case 184:   /* 10111 000  +++  */
        break;
    case 185:   /* 10111 001  +++  */
        break;
    case 186:   /* 10111 010  +++  */
        break;
    case 187:   /* 10111 011  |||  */
        break;
    case 188:   /* 10111 100  +++  */
        break;
    case 189:   /* 10111 101  +++  */
        break;
    case 190:   /* 10111 110  +++  */
        break;
    case 191:   /* 10111 111  +++  */
        break;
    /* Row 24 */
    case 192:   /* 11000 000  |||  */
        break;
    case 193:   /* 11000 001  +++  */
        break;
    case 194:   /* 11000 010  +++  */
        break;
    case 195:   /* 11000 011  |||  */
        break;
    case 196:   /* 11000 100  +++  */
        break;
    case 197:   /* 11000 101  +++  */
        break;
    case 198:   /* 11000 110  +++  */
        break;
    case 199:   /* 11000 111  +++  */
        break;
    /* Row 25 */
    case 200:   /* 11001 000  +++  */
        break;
    case 201:   /* 11001 001  +++  */
        break;
    case 202:   /* 11001 010  +++  */
        break;
    case 203:   /* 11001 011  +++  */
        break;
    case 204:   /* 11001 100  |||  */
        break;
    case 205:   /* 11001 101  +++  */
        break;
    case 206:   /* 11001 110  +++  */
        break;
    case 207:   /* 11001 111  ||| */
        break;
    /* Row 26 */
    case 208:   /* 11010 000  +++  */
        break;
    case 209:   /* 11010 001  +++  */
        break;
    case 210:   /* 11010 010  +++  */
        break;
    case 211:   /* 11010 011  +++  */
        break;
    case 212:   /* 11010 100  +++  */
        break;
    case 213:   /* 11010 101  +++  */
        break;
    case 214:   /* 11010 110  +++  */
        break;
    case 215:   /* 11010 111  +++  */
        break;
    /* Row 27 */
    case 216:   /* 11011 000  +++  */
        break;
    case 217:   /* 11011 001  +++  */
        break;
    case 218:   /* 11011 010  +++  */
        break;
    case 219:   /* 11011 011  +++  */
        break;
    case 220:   /* 11011 100  +++  */
        break;
    case 221:   /* 11011 101  |||  */
        break;
    case 222:   /* 11011 110  +++  */
        break;
    case 223:   /* 11011 111  +++  */
        break;
    /* Row 28 */
    case 224:   /* 11100 000  NOR(A,AND(B,C))  */
        tempv1 = U128_nor(tempv2, U128_and(tempv3, tempv4));
        break;
    case 225:   /* 11100 001  NXOR(A,AND(B,C))  */
        tempv1 = U128_nxor(tempv2, U128_and(tempv3, tempv4));
        break;
    case 226:   /* 11100 010  +++  */
        break;
    case 227:   /* 11100 011  SEL(A,B,NAND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = tempv3x;
        tempv1y = U128_nand(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 228:   /* 11100 100  +++  */
        break;
    case 229:   /* 11100 101  +++  */
        break;
    case 230:   /* 11100 110  SEL(A,XOR(B,C),NAND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_xor(tempv3x, tempv4x);
        tempv1y = U128_nand(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 231:   /* 11100 111  SEL(A,OR(B,C),NAND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_or(tempv3x, tempv4x);
        tempv1y = U128_nand(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    /* Row 29 */
    case 232:   /* 11101 000  MINOR(A,B,C)  */
        tempv1 = U128_major_minor(tempv2, tempv3, tempv4, FALSE);
        break;
    case 233:   /* 11101 001  SEL(A,NXOR(B,C),NAND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_nxor(tempv3x, tempv4x);
        tempv1y = U128_nand(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
        tempv1 = U128_or(tempv1x, tempv1y);
        break;
    case 234:   /* 11101 010  +++  */
        break;
    case 235:   /* 11101 011  +++  */
        break;
    case 236:   /* 11101 100  +++  */
        break;
    case 237:   /* 11101 101  +++  */
        break;
    case 238:   /* 11101 110  |||  */
        break;
    case 239:   /* 11101 111  OR(A,NAND(B,C))  */
        tempv1 = U128_or(tempv2, U128_nand(tempv3, tempv4));
        break;
    /* Row 30 */
    case 240:   /* 11110 000  |||  */
        break;
    case 241:   /* 11110 001  NAND(A,NAND(B,C))  */
        tempv1 = U128_nand(tempv2, U128_nand(tempv3, tempv4));
        break;
    case 242:   /* 11110 010  +++  */
        break;
    case 243:   /* 11110 011  |||  */
        break;
    case 244:   /* 11110 100  +++  */
        break;
    case 245:   /* 11110 101  |||  */
        break;
    case 246:   /* 11110 110  NAND(A,NXOR(B,C))  */
        tempv1 = U128_nand(tempv2, U128_nxor(tempv3, tempv4));
        break;
    case 247:   /* 11110 111  NAND(A,NOR(B,C))  */
        tempv1 = U128_nand(tempv2, U128_nor(tempv3, tempv4));
        break;
    /* Row 31 */
    case 248:   /* 11111 000  NAND(A,OR(B,C))  */
        tempv1 = U128_nand(tempv2, U128_or(tempv3, tempv4));
        break;
    case 249:   /* 11111 001  NAND(A,XOR(B,C))  */
        tempv1 = U128_nand(tempv2, U128_xor(tempv3, tempv4));
        break;
    case 250:   /* 11111 010  |||  */
        break;
    case 251:   /* 11111 011  +++  */
        break;
    case 252:   /* 11111 100  |||  */
        break;
    case 253:   /* 11111 101  +++  */
        break;
    case 254:   /* 11111 110  NAND(A,B,C)  */
        tempv1 = U128_nand(tempv2, U128_nand(tempv3, tempv4));
        break;
    case 255:   /* 11111 111  |||  */
        break;
    default:
//  The default code is here to 1) prevent compiler warnings,
//  and 2) as examples of possible future requirements. I've
//  assumed B is B,C,A, but it might equally be B,A,C!  C,A
//  or A,C doesn't matter in the example below, but there are
//  SEL's were the order would matter, e.g. SEL(B,C,NOT(A)).
//  case 22:    /* 00010 110  SEL(A,XOR(B,C),AND(B,C))  */
        tempv3x = U128_and(tempv3, tempv2);
        tempv3y = U128_and(tempv3, tnotv2);
        tempv4x = U128_and(tempv4, tempv2);
        tempv4y = U128_and(tempv4, tnotv2);
        tempv1x = U128_xor(tempv3x, tempv4x);
        tempv1y = U128_and(tempv3y, tempv4y);
        tempv1x = U128_and(tempv1x, tempv2);
        tempv1y = U128_and(tempv1y, tnotv2);
//      tempv1 = U128_or(tempv1x, tempv1y);
//  case ??:    /* ????? ???  SEL(B,XOR(C,A),AND(C,A))  */
        tempv4x = U128_and(tempv4, tempv3);
        tempv4y = U128_and(tempv4, tnotv3);
        tempv2x = U128_and(tempv2, tempv3);
        tempv2y = U128_and(tempv2, tnotv3);
        tempv1x = U128_xor(tempv4x, tempv2x);
        tempv1y = U128_and(tempv4y, tempv2y);
        tempv1x = U128_and(tempv1x, tempv3);
        tempv1y = U128_and(tempv1y, tnotv3);
//      tempv1 = U128_or(tempv1x, tempv1y);
//  case ??:    /* ????? ???  SEL(C,XOR(A,B),AND(A,B))  */
        tempv2x = U128_and(tempv2, tempv4);
        tempv2y = U128_and(tempv2, tnotv4);
        tempv3x = U128_and(tempv3, tempv4);
        tempv3y = U128_and(tempv3, tnotv4);
        tempv1x = U128_xor(tempv2x, tempv3x);
        tempv1y = U128_and(tempv2y, tempv3y);
        tempv1x = U128_and(tempv1x, tempv4);
        tempv1y = U128_and(tempv1y, tnotv4);
//      tempv1 = U128_or(tempv1x, tempv1y);
        break;
    }

    regs->VR_Q(v1) = tempv1.Q;

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_198_VECTOR_ENH_FACILITY_3 ) */

#if defined( FEATURE_198_VECTOR_ENH_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E789 VBLEND - Vector Blend                                [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_blend )
{
    int     v1, v2, v3, v4, m5, m6;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0; i<16; i++)
        {
            if (regs->VR_B(v4, i) & 0x80)
                regs->VR_B(v1, i) = regs->VR_B(v2, i);
            else
                regs->VR_B(v1, i) = regs->VR_B(v3, i);
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i<8; i++)
        {
            if (regs->VR_H(v4, i) & 0x8000)
                regs->VR_H(v1, i) = regs->VR_H(v2, i);
            else
                regs->VR_H(v1, i) = regs->VR_H(v3, i);
        }
        break;
    case 2:  /* Word */
        for (i=0; i<4; i++)
        {
            if (regs->VR_F(v4, i) & 0x80000000)
                regs->VR_F(v1, i) = regs->VR_F(v2, i);
            else
                regs->VR_F(v1, i) = regs->VR_F(v3, i);
        }
        break;
    case 3:  /* Doublword */
        for (i=0; i<2; i++)
        {
            if (regs->VR_D(v4, i) & 0x8000000000000000ull )
                regs->VR_D(v1, i) = regs->VR_D(v2, i);
            else
                regs->VR_D(v1, i) = regs->VR_D(v3, i);
        }
        break;
    case 4:  /* Quadword */
        if (regs->VR_D(v4, 0) & 0x8000000000000000ull )
        {
            regs->VR_D(v1, 0) = regs->VR_D(v2, 0);
            regs->VR_D(v1, 1) = regs->VR_D(v2, 1);
        }
        else
        {
            regs->VR_D(v1, 0) = regs->VR_D(v3, 0);
            regs->VR_D(v1, 1) = regs->VR_D(v3, 1);
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_198_VECTOR_ENH_FACILITY_3 ) */

/*-------------------------------------------------------------------*/
/* E78A VSTRC  - Vector String Range Compare                 [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_string_range_compare )
{
    int     v1, v2, v3, v4, m5, m6;
    int     i, j;
    int     lxt1, lxt2;                // Lowest indexed true
    int     mxt;                       // Maximum indexed true
    BYTE    irt1[16], irt2[16];        // First and second intermediate results
    BYTE    erc, orc;                  // Even and Odd range comparison results

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M6_IN ((m6 & 0x8) != 0) // Invert Result
#define M6_RT ((m6 & 0x4) != 0) // Result Type
#define M6_ZS ((m6 & 0x2) != 0) // Zero Search
#define M6_CS ((m6 & 0x1) != 0) // Condition Code Set

    for (i=0; i<16; i++)
    {
        irt1[i] = irt2[i] = FALSE;
    }

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0; i<16; i++)
        {
            // Compare the element of the second operand with the ranges
            for (j=0; j<16; j+=2)
            {
                erc = orc = FALSE;
                // Compare the element of the second operand with the even element of the third operand.
                if ((regs->VR_B(v4, j)   & 0x80) && regs->VR_B(v2, i) == regs->VR_B(v3, j))   erc = TRUE;
                if ((regs->VR_B(v4, j)   & 0x40) && regs->VR_B(v2, i) <  regs->VR_B(v3, j))   erc = TRUE;
                if ((regs->VR_B(v4, j)   & 0x20) && regs->VR_B(v2, i) >  regs->VR_B(v3, j))   erc = TRUE;
                // Compare the element of the second operand with the odd element of the third operand.
                if ((regs->VR_B(v4, j+1) & 0x80) && regs->VR_B(v2, i) == regs->VR_B(v3, j+1)) orc = TRUE;
                if ((regs->VR_B(v4, j+1) & 0x40) && regs->VR_B(v2, i) <  regs->VR_B(v3, j+1)) orc = TRUE;
                if ((regs->VR_B(v4, j+1) & 0x20) && regs->VR_B(v2, i) >  regs->VR_B(v3, j+1)) orc = TRUE;
                // Determine the result of the range comparison
                if (erc == TRUE && orc == TRUE) irt1[i] = TRUE;
            }
            // Invert the ranges result if required
            if (M6_IN) irt1[i] ^= TRUE;
            // Compare the element of the second operand with zero
            if (M6_ZS && regs->VR_B(v2,i) == 0) irt2[i] = TRUE;
        }
        if (M6_RT)                     // if M6_RT (Result Type)
        {
            for (i=0; i<16; i++)
            {
                regs->VR_B(v1, i) = (irt1[i] == TRUE) ? 0xFF : 0x00;
            }
        }
        else                           // else !M6_RT
        {
            lxt1 = lxt2 = 16;
            for (i=0; i<16; i++)
            {
                if (irt1[i] == TRUE && lxt1 == 16) lxt1 = i;
                if (irt2[i] == TRUE && lxt2 == 16) lxt2 = i;
            }
            regs->VR_D(v1, 0) = min(lxt1, lxt2);
            regs->VR_D(v1, 1) = 0;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i<8; i++)
        {
            // Compare the element of the second operand with the ranges
            for (j=0; j<8; j+=2)
            {
                erc = orc = FALSE;
                // Compare the element of the second operand with the even element of the third operand.
                if ((regs->VR_H(v4, j)   & 0x8000) && regs->VR_H(v2, i) == regs->VR_H(v3, j))   erc = TRUE;
                if ((regs->VR_H(v4, j)   & 0x4000) && regs->VR_H(v2, i) <  regs->VR_H(v3, j))   erc = TRUE;
                if ((regs->VR_H(v4, j)   & 0x2000) && regs->VR_H(v2, i) >  regs->VR_H(v3, j))   erc = TRUE;
                // Compare the element of the second operand with the odd element of the third operand.
                if ((regs->VR_H(v4, j+1) & 0x8000) && regs->VR_H(v2, i) == regs->VR_H(v3, j+1)) orc = TRUE;
                if ((regs->VR_H(v4, j+1) & 0x4000) && regs->VR_H(v2, i) <  regs->VR_H(v3, j+1)) orc = TRUE;
                if ((regs->VR_H(v4, j+1) & 0x2000) && regs->VR_H(v2, i) >  regs->VR_H(v3, j+1)) orc = TRUE;
                // Determine the result of the range comparison
                if (erc == TRUE && orc == TRUE) irt1[i] = TRUE;
            }
            // Invert the ranges result if required
            if (M6_IN) irt1[i] ^= TRUE;
            // Compare the element of the second operand with zero
            if (M6_ZS && regs->VR_H(v2,i) == 0) irt2[i] = TRUE;
        }
        if (M6_RT)                     // if M6_RT (Result Type)
        {
            for (i=0; i<8; i++)
            {
                regs->VR_H(v1, i) = (irt1[i] == TRUE) ? 0xFFFF : 0x0000;
            }
        }
        else                           // else !M6_RT
        {
            lxt1 = lxt2 = 16;
            for (i=0; i<8; i++)
            {
                if (irt1[i] == TRUE && lxt1 == 16) lxt1 = i * 2;
                if (irt2[i] == TRUE && lxt2 == 16) lxt2 = i * 2;
            }
            regs->VR_D(v1, 0) = min(lxt1, lxt2);
            regs->VR_D(v1, 1) = 0;
        }
        break;
    case 2:  /* Word */
        for (i=0; i<4; i++)
        {
            // Compare the element of the second operand with the ranges
            for (j=0; j<4; j+=2)
            {
                erc = orc = FALSE;
                // Compare the element of the second operand with the even element of the third operand.
                if ((regs->VR_F(v4, j)   & 0x80000000) && regs->VR_F(v2, i) == regs->VR_F(v3, j))   erc = TRUE;
                if ((regs->VR_F(v4, j)   & 0x40000000) && regs->VR_F(v2, i) <  regs->VR_F(v3, j))   erc = TRUE;
                if ((regs->VR_F(v4, j)   & 0x20000000) && regs->VR_F(v2, i) >  regs->VR_F(v3, j))   erc = TRUE;
                // Compare the element of the second operand with the odd element of the third operand.
                if ((regs->VR_F(v4, j+1) & 0x80000000) && regs->VR_F(v2, i) == regs->VR_F(v3, j+1)) orc = TRUE;
                if ((regs->VR_F(v4, j+1) & 0x40000000) && regs->VR_F(v2, i) <  regs->VR_F(v3, j+1)) orc = TRUE;
                if ((regs->VR_F(v4, j+1) & 0x20000000) && regs->VR_F(v2, i) >  regs->VR_F(v3, j+1)) orc = TRUE;
                // Determine the result of the range comparison
                if (erc == TRUE && orc == TRUE) irt1[i] = TRUE;
            }
            // Invert the ranges result if required
            if (M6_IN) irt1[i] ^= TRUE;
            // Compare the element of the second operand with zero
            if (M6_ZS && regs->VR_F(v2,i) == 0) irt2[i] = TRUE;
        }
        if (M6_RT)                     // if M6_RT (Result Type)
        {
            for (i=0; i<4; i++)
            {
                regs->VR_F(v1, i) = (irt1[i] == TRUE) ? 0xFFFFFFFF : 0x00000000;
            }
        }
        else                           // else !M6_RT
        {
            lxt1 = lxt2 = 16;
            for (i=0; i<4; i++)
            {
                if (irt1[i] == TRUE && lxt1 == 16) lxt1 = i * 4;
                if (irt2[i] == TRUE && lxt2 == 16) lxt2 = i * 4;
            }
            regs->VR_D(v1, 0) = min(lxt1, lxt2);
            regs->VR_D(v1, 1) = 0;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M6_CS)                         // if M6_CS (Condition Code Set)
    {
        switch (m5)
        {
        case 0:  /* Byte */
            lxt1 = lxt2 = mxt = 16;
            break;
        case 1:  /* Halfword */
            lxt1 = lxt2 = mxt = 8;
            break;
        case 2:  /* Word */
            lxt1 = lxt2 = mxt = 4;
            break;
        default:  // Prevent erroneous "may be used uninitialized" warnings
            lxt1 = lxt2 = mxt = 0;
            break;
        }

        for (i = 0; i < mxt; i++)
        {
            if (irt1[i] == TRUE && lxt1 == mxt)
                lxt1 = i;
            if (M6_ZS)                 // if M6_ZS (Zero Search)
            {
                if (irt2[i] == TRUE && lxt2 == mxt)
                    lxt2 = i;
            }
        }

        // cc 1 and 3 are possible when M6_ZS is 0 or 1.
        if (lxt1 == mxt && lxt2 == mxt)
            regs->psw.cc = 3;
        else if (lxt1 < mxt && lxt2 == mxt )
            regs->psw.cc = 1;
        // cc 0 and 2 are only possible when M6_ZS is 1.
        else if (lxt1 < lxt2)
            regs->psw.cc = 2;
        else
            regs->psw.cc = 0;
    }

#undef M6_IN
#undef M6_RT
#undef M6_ZS
#undef M6_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78B VSTRS  - Vector String Search                        [VRR-d] */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* In PoP (SA22-7832-13), for VSTRS we can read:                     */
/*   Byte element seven of the fourth operand specifies              */
/*   the length of the substring in bytes and must be in             */
/*   the range of 0-16. Other values will result in an               */
/*   unpredictable result.                                           */
/*                                                                   */
/* However, empirical evidence suggests that any value larger than   */
/* 16 is treated as 16. This may be model dependant behaviour, but   */
/* this implementation will follow a models (z15) behaviour.         */
/*                                                                   */
DEF_INST( vector_string_search )
{
    int     v1, v2, v3, v4, m5, m6;
    char    v2_temp[16], v3_temp[16], nulls[16];
    int     substr_len, char_size, str_len, eos, i, k;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M6_RE ((m6 & 0xD) != 0) // Reserved
#define M6_ZS ((m6 & 0x2) != 0) // Zero Search

    if (m5 > 2 || M6_RE)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Get the contents of v2 and v3 as a string of bytes arranged */
    /* as they would be if they were in the guests storage.        */
    for (i = 0; i < 16; i++)
    {
        v2_temp[i] = regs->VR_B( v2, i );
        v3_temp[i] = regs->VR_B( v3, i );
    }

    substr_len = regs->VR_B( v4, 7 );

    switch (m5)
    {
    case 0:
        char_size = 1;
        break;
    case 1:
        char_size = 2;
        break;
    case 2:
        char_size = 4;
        break;
    default:  // Prevent erroneous "may be used uninitialized" warnings
        char_size = 1;
        break;
    }

    str_len = eos = i = k = 0;

    if (M6_ZS)
    {
        memset( nulls, 0, sizeof(nulls) );

        for (i = 0; i < 16; i += char_size)
        {
            if ( memcmp(&v3_temp[i], &nulls, char_size) == 0 )
            {
                break;
            }
        }

        if ( i < substr_len )
        {
            substr_len = i;
        }

        if (substr_len == 0)
        {
            goto vector_string_search_full_match;
        }
        else
        {
            if (substr_len > 16)
            {
                substr_len = 16;
            }

            for ( ; k < 16 ; k += char_size )
            {
                if ( memcmp(&v2_temp[k], &nulls, char_size) == 0 )
                {
                    eos = 1;
                    break;
                }
            }

            str_len = k;
            k = 0;

            if ( (substr_len % char_size) != 0 )
            {
                goto vector_string_search_mdresult;
            }

            for ( ; ; k += char_size )
            {
                if ( k < str_len )
                {
                    if ( eos == 0 || ( k + substr_len ) <= str_len )
                    {
                        if ((k + substr_len) <= str_len)
                        {
                            if ( memcmp(&v2_temp[k], &v3_temp[0], substr_len) == 0 )
                            {
                                goto vector_string_search_full_match;
                            }
                        }
                        else
                        {
                            if ( memcmp(&v2_temp[k], &v3_temp[0], str_len - k ) == 0 )
                            {
                                goto vector_string_search_partial_match;
                            }
                        }
                    }
                    else
                    {
                        goto vector_string_search_no_match_zero;
                    }
                }
                else
                {
                    goto vector_string_search_no_match_zero;
                }
            }
        }
    }
    else
    {
        if ( substr_len == 0 )
        {
            goto vector_string_search_full_match;
        }
        else
        {
            if ( (substr_len % char_size) != 0 )
            {
                goto vector_string_search_mdresult;
            }
            if (substr_len > 16)
            {
                substr_len = 16;
            }
            for ( ; ; k += char_size )
            {
                if (k == 16)
                {
                    goto vector_string_search_no_match;
                }

                if ((k + substr_len) <= 16)
                {
                    if ( memcmp(&v2_temp[k], &v3_temp[0], substr_len) == 0 )
                    {
                        goto vector_string_search_full_match;
                    }
                }
                else
                {
                    if ( memcmp(&v2_temp[k], &v3_temp[0], 16 - k) == 0 )
                    {
                        goto vector_string_search_partial_match;
                    }
                }
            }
        }
    }

    UNREACHABLE_CODE( goto vector_string_search_mdresult );

vector_string_search_mdresult:
    regs->VR_D( v1, 0 ) = 16;                    /* Model dependant */
    regs->VR_D( v1, 1 ) = 0;                     /* results are     */
    regs->psw.cc = 0;  /* no match */            /* unpredictable   */
    goto vector_string_search_end;

vector_string_search_no_match:
    regs->VR_D( v1, 0 ) = 16;
    regs->VR_D( v1, 1 ) = 0;
    regs->psw.cc = 0;  /* no match */
    goto vector_string_search_end;

vector_string_search_no_match_zero:
    regs->VR_D( v1, 0 ) = 16;
    regs->VR_D( v1, 1 ) = 0;
    if ( eos == 1 )
        regs->psw.cc = 1;  /* no match, zero char */
    else
        regs->psw.cc = 0;  /* no match */
    goto vector_string_search_end;

vector_string_search_full_match:
    regs->VR_D( v1, 0 ) = k;
    regs->VR_D( v1, 1 ) = 0;
    regs->psw.cc = 2;  /* full match */
    goto vector_string_search_end;

vector_string_search_partial_match:
    regs->VR_D( v1, 0 ) = k;
    regs->VR_D( v1, 1 ) = 0;
    regs->psw.cc = 3;  /* partial match */
    goto vector_string_search_end;

vector_string_search_end:

#undef M6_RE
#undef M6_ZS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E78C VPERM  - Vector Permute                              [VRR-e] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_permute )
{
    int     v1, v2, v3, v4, m5, m6;
    int     i, j;
    SV      temp;

    VRR_E( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    for (i = 0; i < 16; i++) {
        j = regs->VR_B(v4, i) & 0x1f;
        regs->VR_B(v1, i) = SV_B( temp, j );
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

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    regs->VR_D(v1, 1) = (regs->VR_D(v4, 1) & regs->VR_D(v2, 1)) | (~regs->VR_D(v4, 1) & regs->VR_D(v3, 1));
    regs->VR_D(v1, 0) = (regs->VR_D(v4, 0) & regs->VR_D(v2, 0)) | (~regs->VR_D(v4, 0) & regs->VR_D(v3, 0));

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E794 VPK    - Vector Pack                                 [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_pack )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    SV      temp;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    switch (m4)
    {
    case 1:  /* Halfword: Low-order bytes from halfwords */
        for ( i = 0; i < 16; i++ )
        {
            regs->VR_B( v1, i ) = SV_B( temp, (i*2)+1 );
        }
        break;
    case 2:  /* Word: Low-order halfwords from words */
        for ( i = 0; i < 8; i++ )
        {
            regs->VR_H( v1, i ) = SV_H( temp, (i*2)+1 );
        }
        break;
    case 3:  /* Doubleword: Low-order words from doublewords */
        for ( i = 0; i < 4; i++ )
        {
            regs->VR_F( v1, i ) = SV_F( temp, (i*2)+1 );
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E795 VPKLS  - Vector Pack Logical Saturate                [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_pack_logical_saturate)
{
    int     v1, v2, v3, m4, m5;
    int     sat, allsat, i;
    BYTE    newcc;
    SV      temp;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0)  // Condition Code Set

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    sat = allsat = 0;

    switch (m4)
    {
    case 1:  /* Halfword: Low-order bytes from halfwords */
        for ( i = 0; i < 16; i++ )
        {
            if ( SV_H( temp, i ) <= 0x00FF )
            {
                regs->VR_B( v1, i ) = SV_B( temp, (i*2)+1 );
            }
            else
            {
                regs->VR_B( v1, i ) = 0xFF;
                sat++;
            }
        }
        allsat = 16;
        break;
    case 2:  /* Word: Low-order halfwords from words */
        for ( i = 0; i < 8; i++ )
        {
            if ( SV_F( temp, i ) <= 0x0000FFFF )
            {
                regs->VR_H( v1, i ) = SV_H( temp, (i*2)+1 );
            }
            else
            {
                regs->VR_H( v1, i ) = 0xFFFF;
                sat++;
            }
        }
        allsat = 8;
        break;
    case 3:  /* Doubleword: Low-order words from doublewords */
        for ( i = 0; i < 4; i++ )
        {
            if ( SV_D( temp, i ) <= 0x00000000FFFFFFFFull )
            {
                regs->VR_F( v1, i ) = SV_F( temp, (i*2)+1 );
            }
            else
            {
                regs->VR_F( v1, i ) = 0xFFFFFFFF;
                sat++;
            }
        }
        allsat = 4;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    if (M5_CS)               // if M5_CS (Condition Code Set)
    {
        if ( sat >= allsat )
        {
            newcc = 3;       // Saturation on all elements
        }
        else if ( sat != 0 )
        {
            newcc = 1;       // At least one but not all elements saturated
        }
        else
        {
            newcc = 0;       // No saturation
        }
        regs->psw.cc = newcc;
    }

#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E797 VPKS   - Vector Pack Saturate                        [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_pack_saturate )
{
    int     v1, v2, v3, m4, m5;
    int     sat, allsat, i;
    BYTE    newcc;
    SV      temp;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0)  // Condition Code Set

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    sat = allsat = 0;

    switch (m4)
    {
    case 1:  /* Halfword: Low-order bytes from halfwords */
        for ( i = 0; i < 16; i++ )
        {
            if ( !( SV_H( temp, i ) & 0x8000 ) )
            {
                if ( SV_H( temp, i ) <= 0x007F )
                {
                    regs->VR_B( v1, i ) = SV_B( temp, (i*2)+1 );
                }
                else
                {
                    regs->VR_B( v1, i ) = 0x7F;
                    sat++;
                }
            }
            else
            {
                if ( (S16)SV_H( temp, i ) >= (S16)0xFF80 )
                {
                    regs->VR_B( v1, i ) = SV_B( temp, (i*2)+1 );
                }
                else
                {
                    regs->VR_B( v1, i ) = 0x80;
                    sat++;
                }
            }
        }
        allsat = 16;
        break;
    case 2:  /* Word: Low-order halfwords from words */
        for ( i = 0; i < 8; i++ )
        {
            if ( !( SV_F( temp, i ) & 0x80000000 ) )
            {
                if ( SV_F( temp, i ) <= 0x00007FFF )
                {
                    regs->VR_H( v1, i ) = SV_H( temp, (i*2)+1 );
                }
                else
                {
                    regs->VR_H( v1, i ) = 0x7FFF;
                    sat++;
                }
            }
            else
            {
                if ( (S32)SV_F( temp, i ) >= (S32)0xFFFF8000 )
                {
                    regs->VR_H( v1, i ) = SV_H( temp, (i*2)+1 );
                }
                else
                {
                    regs->VR_H( v1, i ) = 0x8000;
                    sat++;
                }
            }
        }
        allsat = 8;
        break;
    case 3:  /* Doubleword: Low-order words from doublewords */
        for ( i = 0; i < 4; i++ )
        {
            if ( !( SV_D( temp, i ) & 0x8000000000000000ull ) )
            {
                if ( SV_D( temp, i ) <= 0x000000007FFFFFFFull )
                {
                    regs->VR_F( v1, i ) = SV_F( temp, (i*2)+1 );
                }
                else
                {
                    regs->VR_F( v1, i ) = 0x7FFFFFFF;
                    sat++;
                }
            }
            else
            {
                if ( (S64)SV_D( temp, i ) >= (S64)0xFFFFFFFF80000000ull )
                {
                    regs->VR_F( v1, i ) = SV_F( temp, (i*2)+1 );
                }
                else
                {
                    regs->VR_F( v1, i ) = 0x80000000;
                    sat++;
                }
            }
        }
        allsat = 4;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    if (M5_CS)               // if M5_CS (Condition Code Set)
    {
        if ( sat >= allsat )
        {
            newcc = 3;       // Saturation on all elements
        }
        else if ( sat != 0 )
        {
            newcc = 1;       // At least one but not all elements saturated
        }
        else
        {
            newcc = 0;       // No saturation
        }
        regs->psw.cc = newcc;
    }

#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A1 VMLH   - Vector Multiply Logical High                [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_logical_high )
{
    int     v1, v2, v3, m4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1;
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( (m4 == 3 || m4 == 4) && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_B(v1, i) = temp.h >> 8;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_H(v1, i) = temp.f >> 16;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_F(v1, i) = temp.d >> 32;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            u128temp1 = U128_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i) );
            regs->VR_D(v1, i) = u128temp1.Q.D.H.D;
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv2, tempv3;
            U128 temphi, templo;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            U128_mul(  tempv2, tempv3, &temphi, &templo );
            regs->VR_Q(v1) = temphi.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A2 VML    - Vector Multiply Low                         [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_low )
{
    int     v1, v2, v3, m4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1;
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( (m4 == 3 || m4 == 4) && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_B(v1, i) = temp.h & 0xFF;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_H(v1, i) = temp.f & 0xFFFF;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_F(v1, i) = temp.d & 0xFFFFFFFF;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            u128temp1 = S128_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i) );
            regs->VR_D(v1, i) = u128temp1.Q.D.L.D;
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv2, tempv3;
            U128 temphi, templo;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            S128_mul(  tempv2, tempv3, &temphi, &templo );
            regs->VR_Q(v1) = templo.Q;
        }
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
    union   { S64 sd; S32 sf; S16 sh; } temp;
    U128    u128temp1;
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( (m4 == 3 || m4 == 4) && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            regs->VR_B(v1, i) = temp.sh >> 8;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            regs->VR_H(v1, i) = temp.sf >> 16;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            regs->VR_F(v1, i) = temp.sd >> 32;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            u128temp1 = S128_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i) );
            regs->VR_D(v1, i) = u128temp1.Q.D.H.D;
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv2, tempv3;
            U128 temphi, templo;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            S128_mul(  tempv2, tempv3, &temphi, &templo );
            regs->VR_Q(v1) = temphi.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A4 VMLE   - Vector Multiply Logical Even                [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_logical_even )
{
    int     v1, v2, v3, m4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1;
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m4 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  /* Halfword */
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  /* Word */
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.d;
        }
        break;
    case 3:  /* Doubleword */
        u128temp1 = U128_mul_64( regs->VR_D(v2, 0), regs->VR_D(v3, 0) );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A5 VMLO   - Vector Multiply Logical Odd                 [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_logical_odd )
{
    int     v1, v2, v3, m4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1;
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( (m4 == 3 || m4 == 4) && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  /* Halfword */
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  /* Word */
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.d;
        }
        break;
    case 3:  /* Doubleword */
        u128temp1 = U128_mul_64( regs->VR_D(v2, 1), regs->VR_D(v3, 1) );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A6 VME    - Vector Multiply Even                        [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_even )
{
    int     v1, v2, v3, m4, m5, m6;
    union   { S64 sd; S32 sf; S16 sh; } temp;
    U128    u128temp1;
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m4 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  /* Halfword */
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  /* Word */
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.sd;
        }
        break;
    case 3:  /* Doubleword */
        u128temp1 = S128_mul_64( regs->VR_D(v2, 0), regs->VR_D(v3, 0) );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A7 VMO    - Vector Multiply Odd                         [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_odd )
{
    int     v1, v2, v3, m4, m5, m6;
    union   { S64 sd; S32 sf; S16 sh; } temp;
    U128    u128temp1;
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m4 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  /* Halfword */
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  /* Word */
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.sd;
        }
        break;
    case 3:  /* Doubleword */
        u128temp1 = S128_mul_64( regs->VR_D(v2, 1), regs->VR_D(v3, 1) );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7A9 VMALH  - Vector Multiply and Add Logical High        [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_logical_high )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( (m5 == 3 || m5 == 4) && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_B(v4, i);
            regs->VR_B(v1, i) = temp.h >> 8;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_H(v4, i);
            regs->VR_H(v1, i) = temp.f >> 16;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_F(v4, i);
            regs->VR_F(v1, i) = temp.d >> 32;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            u128temp1 = U128_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i) );
            u128temp1 = U128_add ( u128temp1, U128_U64( regs->VR_D(v4, i) ));
            regs->VR_D(v1, i) = u128temp1.Q.D.H.D;
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv2, tempv3, tempv4;
            U128 temphi, templo;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            tempv4.Q = regs->VR_Q(v4);

            U128_mul(  tempv2, tempv3, &temphi, &templo );
            U256_add_U128 ( &temphi, &templo, tempv4, &temphi, &templo );
            regs->VR_Q(v1) = temphi.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AA VMAL   - Vector Multiply and Add Low                 [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_multiply_and_add_low)
{
    int     v1, v2, v3, v4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1;
    int     i;

    VRR_D(inst, regs, v1, v2, v3, v4, m5, m6);

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK(regs);

    if ( (m5 == 3 || m5 == 4) && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_B(v4, i);
            regs->VR_B(v1, i) = temp.h & 0xFF;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_H(v4, i);
            regs->VR_H(v1, i) = temp.f & 0xFFFF;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_F(v4, i);
            regs->VR_F(v1, i) = temp.d & 0xFFFFFFFF;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            u128temp1 = S128_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i) );
            u128temp1 = S128_add ( u128temp1, U128_S64( regs->VR_D(v4, i) ));
            regs->VR_D(v1, i) = u128temp1.Q.D.L.D;
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv2, tempv3, tempv4;
            U128 temphi, templo;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            tempv4.Q = regs->VR_Q(v4);

            S128_mul(  tempv2, tempv3, &temphi, &templo );
            S256_add_S128 ( &temphi, &templo, tempv4, &temphi, &templo );
            regs->VR_Q(v1) = templo.Q;
        }
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
    union   { S64 sd; S32 sf; S16 sh; } temp;
    U128    u128temp1;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( (m5 == 3 || m5 == 4) && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            temp.sh += (S8)regs->VR_B(v4, i);
            regs->VR_B(v1, i) = temp.sh >> 8;
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            temp.sf += (S16)regs->VR_H(v4, i);
            regs->VR_H(v1, i) = temp.sf >> 16;
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            temp.sd += (S32)regs->VR_F(v4, i);
            regs->VR_F(v1, i) = temp.sd >> 32;
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            u128temp1 = S128_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i) );
            u128temp1 = S128_add ( u128temp1, U128_S64( regs->VR_D(v4, i) ));
            regs->VR_D(v1, i) = u128temp1.Q.D.H.D;
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv2, tempv3, tempv4;
            U128 temphi, templo;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            tempv4.Q = regs->VR_Q(v4);

            S128_mul(  tempv2, tempv3, &temphi, &templo );
            S256_add_S128 ( &temphi, &templo, tempv4, &temphi, &templo );
            regs->VR_Q(v1) = temphi.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AC VMALE  - Vector Multiply and Add Logical Even        [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_logical_even )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1, u128temp2;
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m5 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  /* Halfword */
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  /* Word */
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.d;
        }
        break;
    case 3:  /* Doubleword */
        u128temp2.Q = regs->VR_Q(v4);
        u128temp1 = U128_mul_64( regs->VR_D(v2, 0), regs->VR_D(v3, 0) );
        u128temp1 = U128_add ( u128temp1, u128temp2 );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AD VMALO  - Vector Multiply and Add Logical Odd         [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_logical_odd )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { U64 d; U32 f; U16 h; } temp;
    U128    u128temp1, u128temp2;
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m5 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  /* Halfword */
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  /* Word */
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.d;
        }
        break;
    case 3:  /* Doubleword */
        u128temp2.Q = regs->VR_Q(v4);
        u128temp1 = U128_mul_64( regs->VR_D(v2, 1), regs->VR_D(v3, 1) );
        u128temp1 = U128_add ( u128temp1, u128temp2 );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AE VMAE   - Vector Multiply and Add Even                [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_even )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { S64 sd; S32 sf; S16 sh; } temp;
    U128    u128temp1, u128temp2;
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m5 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            temp.sh += (S16)regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  /* Halfword */
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            temp.sf += (S32)regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  /* Word */
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            temp.sd += (S64)regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.sd;
        }
        break;
    case 3:  /* Doubleword */
        u128temp2.Q = regs->VR_Q(v4);
        u128temp1 = S128_mul_64( regs->VR_D(v2, 0), regs->VR_D(v3, 0) );
        u128temp1 = S128_add ( u128temp1, u128temp2 );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7AF VMAO   - Vector Multiply and Add Odd                 [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_add_odd )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { S64 sd; S32 sf; S16 sh; } temp;
    U128    u128temp1, u128temp2;
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m5 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m5)
    {
    case 0:  /* Byte */
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            temp.sh += (S16)regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  /* Halfword */
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            temp.sf += (S32)regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  /* Word */
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            temp.sd += (S64)regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.sd;
        }
        break;
    case 3:  /* Doubleword */
        u128temp2.Q = regs->VR_Q(v4);
        u128temp1 = S128_mul_64( regs->VR_D(v2, 1), regs->VR_D(v3, 1) );
        u128temp1 = S128_add ( u128temp1, u128temp2 );
        regs->VR_Q(v1) = u128temp1.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

#if defined( FEATURE_198_VECTOR_ENH_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E7B0 VDL    - Vector Divide Logical                       [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_divide_logical )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U128    dividend, divisor, quotient;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_IDC ((m5 & 0x8) != 0)  // Integer-Divide Control

    switch (m4)
    {
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            if (regs->VR_F( v3, i ) == 0)
            {
                if (M5_IDC)
                {
                    regs->VR_F( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_F( v1, i ) = regs->VR_F( v2, i ) / regs->VR_F( v3, i );
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            if (regs->VR_D( v3, i ) == 0)
            {
                if (M5_IDC)
                {
                    regs->VR_D( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_D( v1, i ) = regs->VR_D( v2, i ) / regs->VR_D( v3, i );
        }
        break;
    case 4:  /* Quadword */
        dividend.Q = regs->VR_Q( v2 );
        divisor.Q = regs->VR_Q( v3 );
        if (U128_isZero( divisor ))
        {
            if (M5_IDC)
            {
                regs->VR_D( v1, 0 ) = 0;
                regs->VR_D( v1, 1 ) = 0;
                break;
            }
            else
                vector_processing_trap( regs, 0, VXC_INTEGER_DIVIDE );
        }
        quotient = U128_div( dividend, divisor );
        regs->VR_Q( v1 ) = quotient.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

#undef M5_IDC

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_198_VECTOR_ENH_FACILITY_3 ) */

#if defined( FEATURE_198_VECTOR_ENH_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E7B1 VRL    - Vector Remainder Logical                    [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_remainder_logical )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U128    dividend, divisor, remainder;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_IDC ((m5 & 0x8) != 0)  // Integer-Divide Control

    switch (m4)
    {
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            if (regs->VR_F( v3, i ) == 0)
            {
                if (M5_IDC)
                {
                    regs->VR_F( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_F( v1, i ) = regs->VR_F( v2, i ) % regs->VR_F( v3, i );
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            if (regs->VR_D( v3, i ) == 0)
            {
                if (M5_IDC)
                {
                    regs->VR_D( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_D( v1, i ) = regs->VR_D( v2, i ) % regs->VR_D( v3, i );
        }
        break;
    case 4:  /* Quadword */
        dividend.Q = regs->VR_Q( v2 );
        divisor.Q = regs->VR_Q( v3 );
        if (U128_isZero( divisor ))
        {
            if (M5_IDC)
            {
                regs->VR_D( v1, 0 ) = 0;
                regs->VR_D( v1, 1 ) = 0;
                break;
            }
            else
                vector_processing_trap( regs, 0, VXC_INTEGER_DIVIDE );
        }
        remainder = U128_rem( dividend, divisor );
        regs->VR_Q( v1 ) = remainder.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

#undef M5_IDC

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_198_VECTOR_ENH_FACILITY_3 ) */

#if defined( FEATURE_198_VECTOR_ENH_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E7B2 VD     - Vector Divide                               [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_divide )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U128    dividend, divisor, quotient;
    U128    negone, negmax;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_IDC ((m5 & 0x8) != 0)  // Integer-Divide Control

    switch (m4)
    {
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            if (regs->VR_F( v3, i ) == 0 ||
               ((regs->VR_F( v3, i ) == 0xFFFFFFFF) && (regs->VR_F( v2, i ) == 0x80000000)) )
            {
                if (M5_IDC)
                {
                    regs->VR_F( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_F( v1, i ) = (S32)regs->VR_F( v2, i ) / (S32)regs->VR_F( v3, i );
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            if (regs->VR_D( v3, i ) == 0 ||
               ((regs->VR_D( v3, i ) == 0xFFFFFFFFFFFFFFFFull) && (regs->VR_D( v2, i ) == 0x8000000000000000ull)) )
            {
                if (M5_IDC)
                {
                    regs->VR_D( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_D( v1, i ) = (S64)regs->VR_D( v2, i ) / (S64)regs->VR_D( v3, i );
        }
        break;
    case 4:  /* Quadword */
        negone.Q.D.H.D = 0xFFFFFFFFFFFFFFFFull;
        negone.Q.D.L.D = 0xFFFFFFFFFFFFFFFFull;
        negmax.Q.D.H.D = 0x8000000000000000ull;
        negmax.Q.D.L.D = 0x0000000000000000ull;
        dividend.Q = regs->VR_Q( v2 );
        divisor.Q = regs->VR_Q( v3 );
        if (U128_isZero( divisor ) ||
           ((U128_cmp( divisor, negone) == 0) && (U128_cmp( dividend, negmax) == 0)) )
        {
            if (M5_IDC)
            {
                regs->VR_D( v1, 0 ) = 0;
                regs->VR_D( v1, 1 ) = 0;
                break;
            }
            else
                vector_processing_trap( regs, 0, VXC_INTEGER_DIVIDE );
        }
        quotient = S128_div(  dividend, divisor );
        regs->VR_Q( v1 ) = quotient.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

#undef M5_IDC

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_198_VECTOR_ENH_FACILITY_3 ) */

#if defined( FEATURE_198_VECTOR_ENH_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E7B3 VR     - Vector Remainder                            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_remainder )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i;
    U128    dividend, divisor, remainder;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M5_IDC ((m5 & 0x8) != 0)  // Integer-Divide Control

    switch (m4)
    {
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            if (regs->VR_F( v3, i ) == 0)
            {
                if (M5_IDC)
                {
                    regs->VR_F( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_F( v1, i ) = (S32)regs->VR_F( v2, i ) % (S32)regs->VR_F( v3, i );
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            if (regs->VR_D( v3, i ) == 0)
            {
                if (M5_IDC)
                {
                    regs->VR_D( v1, i ) = 0;
                    continue;
                }
                else
                    vector_processing_trap( regs, i, VXC_INTEGER_DIVIDE );
            }
            regs->VR_D( v1, i ) = (S64)regs->VR_D( v2, i ) % (S64)regs->VR_D( v3, i );
        }
        break;
    case 4:  /* Quadword */
        dividend.Q = regs->VR_Q( v2 );
        divisor.Q = regs->VR_Q( v3 );
        if (U128_isZero( divisor ))
        {
            if (M5_IDC)
            {
                regs->VR_D( v1, 0 ) = 0;
                regs->VR_D( v1, 1 ) = 0;
                break;
            }
            else
                vector_processing_trap( regs, 0, VXC_INTEGER_DIVIDE );
        }
        remainder = S128_rem( dividend, divisor );
        regs->VR_Q( v1 ) = remainder.Q;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

#undef M5_IDC

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_198_VECTOR_ENH_FACILITY_3 ) */

/*-------------------------------------------------------------------*/
/* E7B4 VGFM   - Vector Galois Field Multiply Sum            [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_galois_field_multiply_sum )
{
    int     v1, v2, v3, m4, m5, m6;

    int     i, k;                 /* loop index                      */
    U16     accu16[16];           /* byte accumulator                */
    U32     accu32[8];            /* halfword accumulator            */
    U64     accu64[4];            /* word accumulator                */
    U64     accu128h[2];          /* doublewword accumulator  - high */
    U64     accu128l[2];          /* doublewword accumulator  - low  */

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++)
            accu16[i] = (U16) gf_mul_32( (U32) regs->VR_B(v2, i), (U32) regs->VR_B(v3, i) );

        /* sum even-odd pair */
        for ( i=0, k=0; i < 8; i++, k += 2)
            regs->VR_H( v1, i ) = accu16[k] ^ accu16[k+1];

        break;

    case 1:         /* Halfword */
        for (i=0; i < 8; i++)
            accu32[i] = (U32) gf_mul_32( (U32) regs->VR_H(v2, i), (U32) regs->VR_H(v3, i) );

        /* sum even-odd pair */
        for ( i=0, k=0; i < 4; i++, k += 2)
            regs->VR_F( v1, i ) = accu32[k] ^ accu32[k+1];

        break;

    case 2:         /* Word */
        for (i=0; i < 4; i++)
            accu64[i] = (U64) gf_mul_32( (U32) regs->VR_F(v2, i), (U32) regs->VR_F(v3, i) );

        /* sum even-odd pair */
        for ( i=0, k=0; i < 2; i++, k += 2)
            regs->VR_D( v1, i ) = accu64[k] ^ accu64[k+1];

        break;

    case 3:         /* Doubleword */
        for (i=0; i < 2; i++)
            gf_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i), &accu128h[i], &accu128l[i]);

        /* sum even-odd pair */
        regs->VR_D( v1, 0 ) = accu128h[0] ^ accu128h[1];
        regs->VR_D( v1, 1 ) = accu128l[0] ^ accu128l[1];

        break;

    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
/*-------------------------------------------------------------------*/
/* E7B8 VMSL   - Vector Multiply Sum Logical                 [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_sum_logical )
{
    int     v1, v2, v3, v4, m5, m6;
    U128    intere, intero;
#if defined( _MSVC_ )
    U128    copyv4;
#endif

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M6_ES ((m6 & 0x8) != 0) // Even Shift Indication
#define M6_OS ((m6 & 0x4) != 0) // Odd Shift Indication

    switch (m5)
    {
    case 3:  /* Doubleword */
        intere = U128_mul_64( regs->VR_D(v2, 0), regs->VR_D(v3, 0) );
        intero = U128_mul_64( regs->VR_D(v2, 1), regs->VR_D(v3, 1) );
        if (M6_ES)
            intere = U128_mul_32( intere, 2 );  // Shift left
        if (M6_OS)
            intero = U128_mul_32( intero, 2 );  // Shift left
        intere = U128_add( intere, intero );
#if defined( _MSVC_ )
        copyv4.Q = regs->VR_Q(v4);
        intere = U128_add( intere, copyv4 );
#else
        intere = U128_add( intere, (U128)regs->VR_Q(v4) );
#endif
        regs->VR_Q(v1) = intere.Q;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

#undef M6_ES
#undef M6_OS

    ZVECTOR_END( regs );
}
#endif /* defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) */

/*-------------------------------------------------------------------*/
/* E7B9 VACCC  - Vector Add With Carry Compute Carry         [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_with_carry_compute_carry )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { U64 d[4]; } temp;
    U64     carry;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 4:  /* Quadword */
        carry = regs->VR_D( v4, 1 ) & 0x0000000000000001ull;
        for (i=3; i >= 0; i--)
        {
            temp.d[i] = carry;
            temp.d[i] += regs->VR_F( v3, i );
            temp.d[i] += regs->VR_F( v2, i );
            carry = temp.d[i] >> 32;
        }
        regs->VR_D( v1, 0 ) = 0;
        regs->VR_D( v1, 1 ) = carry;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7BB VAC    - Vector Add With Carry                       [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_with_carry )
{
    int     v1, v2, v3, v4, m5, m6;
    U128    inter, rmost;
#if defined( _MSVC_ )
    U128    copyv2, copyv3;
#endif

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 4:  /* Quadword */
#if defined( _MSVC_ )
        copyv2.Q = regs->VR_Q(v2);
        copyv3.Q = regs->VR_Q(v3);
        inter = U128_add( copyv2, copyv3 );
#else
        inter = U128_add( (U128)regs->VR_Q(v2), (U128)regs->VR_Q(v3) );
#endif
        if (regs->VR_D( v4, 1 ) & 0x0000000000000001ull)
        {
            rmost.Q.D.H.D = 0;
            rmost.Q.D.L.D = 1;
            inter = U128_add(inter, rmost);
        }
        regs->VR_Q(v1) = inter.Q;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-----------------------------------------------------------------------*/
/* E7BC VGFMA  - Vector Galois Field Multiply Sum and Accumulate [VRR-d] */
/*-----------------------------------------------------------------------*/
DEF_INST( vector_galois_field_multiply_sum_and_accumulate )
{
    int     v1, v2, v3, v4, m5, m6;

    int     i, k;                     /* loop index                      */
    U16     accu16[16];               /* byte accumulator                */
    U32     accu32[8];                /* halfword accumulator            */
    U64     accu64[4];                /* word accumulator                */
    U64     accu128h[2];              /* doublewword accumulator  - high */
    U64     accu128l[2];              /* doublewword accumulator  - low  */

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++)
            accu16[i] = (U16) gf_mul_32( (U32) regs->VR_B(v2, i), (U32) regs->VR_B(v3, i) );

        /* sum even-odd pair plus vector accumulate */
        for ( i=0, k=0; i < 8; i++, k += 2)
            regs->VR_H( v1, i ) = accu16[k] ^ accu16[k+1] ^ regs->VR_H( v4, i);

        break;

    case 1:         /* Halfword */
        for (i=0; i < 8; i++)
            accu32[i] = (U32) gf_mul_32( (U32) regs->VR_H(v2, i), (U32) regs->VR_H(v3, i) );

        /* sum even-odd pair plus vector accumulate */
        for ( i=0, k=0; i < 4; i++, k += 2)
            regs->VR_F( v1, i ) = accu32[k] ^ accu32[k+1] ^ regs->VR_F( v4, i);

        break;

    case 2:         /* Word */
        for (i=0; i < 4; i++)
            accu64[i] = (U64) gf_mul_32( (U32) regs->VR_F(v2, i), (U32) regs->VR_F(v3, i) );

        /* sum even-odd pair plus vector accumulate */
        for ( i=0, k=0; i < 2; i++, k += 2)
            regs->VR_D( v1, i ) = accu64[k] ^ accu64[k+1] ^ regs->VR_D( v4, i );

        break;

    case 3:         /* Doubleword */
        for (i=0; i < 2; i++)
            gf_mul_64( regs->VR_D(v2, i), regs->VR_D(v3, i), &accu128h[i], &accu128l[i]);

        /* sum even-odd pair plus vector accumulate */
        regs->VR_D( v1, 0 ) = accu128h[0] ^ accu128h[1] ^ regs->VR_D( v4, 0);
        regs->VR_D( v1, 1 ) = accu128l[0] ^ accu128l[1] ^ regs->VR_D( v4, 1);

        break;

    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-----------------------------------------------------------------------------*/
/* E7BD VSBCBI - Vector Subtract With Borrow Compute Borrow Indication [VRR-d] */
/*-----------------------------------------------------------------------------*/
DEF_INST( vector_subtract_with_borrow_compute_borrow_indication )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { U64 d; } temp;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 4:  /* Quadword */
        temp.d = regs->VR_D( v4, 1 ) & 0x0000000000000001ull;
        for (i=3; i >= 0; i--)
        {
            temp.d += ~regs->VR_F( v3, i );
            temp.d += regs->VR_F( v2, i );
            temp.d >>= 32;
        }
        regs->VR_D( v1, 0 ) = 0;
        regs->VR_D( v1, 1 ) = temp.d;
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7BF VSBI   - Vector Subtract With Borrow Indication      [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_subtract_with_borrow_indication )
{
    int     v1, v2, v3, v4, m5, m6;
    union   { U64 d[4]; } temp;
    U64     carry;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 4:  /* Quadword */
        carry = regs->VR_D( v4, 1 ) & 0x0000000000000001ull;
        for (i=3; i >= 0; i--)
        {
            temp.d[i] = carry;
            temp.d[i] += ~regs->VR_F( v3, i );
            temp.d[i] += regs->VR_F( v2, i );
            carry = temp.d[i] >> 32;
        }
        for (i=3; i >= 0; i--)
            regs->VR_F( v1, i ) = temp.d[i];
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D4 VUPLL  - Vector Unpack Logical Low                   [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_logical_low )
{
    int     v1, v2, m3, m4, m5, i;
    union   { U64 d[2]; U32 f[4]; U16 h[8]; } temp;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i = 0; i < 8; i++)
            temp.h[i] = (U16) regs->VR_B(v2, i + 8);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.h[i];
        break;
    case 1:  /* Halfword */
        for (i = 0; i < 4; i++)
            temp.f[i] = (U32) regs->VR_H(v2, i + 4);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.f[i];
        break;
    case 2:  /* Word */
        for (i = 0; i < 2; i++)
            temp.d[i] = (U64) regs->VR_F(v2, i + 2);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.d[i];
        break;
    case 3:  /* Doubleword */
        regs->VR_D(v1, 1) = regs->VR_D(v2, 1);
        regs->VR_D(v1, 0) = 0x0000000000000000ull;
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
    union   { U64 d[2]; U32 f[4]; U16 h[8]; } temp;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i = 0; i < 8; i++)
            temp.h[i] = (U16) regs->VR_B(v2, i);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.h[i];
        break;
    case 1:  /* Halfword */
        for (i = 0; i < 4; i++)
            temp.f[i] = (U32) regs->VR_H(v2, i);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.f[i];
        break;
    case 2:  /* Word */
        for (i = 0; i < 2; i++)
            temp.d[i] = (U64) regs->VR_F(v2, i);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.d[i];
        break;
    case 3:  /* Doubleword */
        regs->VR_D(v1, 1) = regs->VR_D(v2, 0);
        regs->VR_D(v1, 0) = 0x0000000000000000ull;
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
    union   { U64 sd[2]; U32 sf[4]; U16 sh[8]; } temp;
    int     i;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i = 0; i < 8; i++)
        {
            temp.sh[i] = regs->VR_B(v2, i + 8);
            if (temp.sh[i] & 0x0080) temp.sh[i] |= 0xFF00;
        }
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.sh[i];
        break;
    case 1:  /* Halfword */
        for (i = 0; i < 4; i++)
        {
            temp.sf[i] = regs->VR_H(v2, i + 4);
            if (temp.sf[i] & 0x00008000) temp.sf[i] |= 0xFFFF0000;
        }
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.sf[i];
        break;
    case 2:  /* Word */
        for (i = 0; i < 2; i++)
        {
            temp.sd[i] = regs->VR_F(v2, i + 2);
            if (temp.sd[i] & 0x0000000080000000ull) temp.sd[i] |= 0xFFFFFFFF00000000ull;
        }
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.sd[i];
        break;
    case 3:  /* Doubleword */
        if (regs->VR_D(v2, 1) & 0x8000000000000000ull)
        {
            regs->VR_D(v1, 1) = regs->VR_D(v2, 1);
            regs->VR_D(v1, 0) = 0xFFFFFFFFFFFFFFFFull;
        }
        else
        {
            regs->VR_D(v1, 1) = regs->VR_D(v2, 1);
            regs->VR_D(v1, 0) = 0x0000000000000000ull;
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D7 VUPH   - Vector Unpack High                          [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_high )
{
    int     v1, v2, m3, m4, m5;
    union   { U64 sd[2]; U32 sf[4]; U16 sh[8]; } temp;
    int     i;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 3 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i = 0; i < 8; i++)
        {
            temp.sh[i] = regs->VR_B(v2, i);
            if (temp.sh[i] & 0x0080) temp.sh[i] |= 0xFF00;
        }
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.sh[i];
        break;
    case 1:  /* Halfword */
        for (i = 0; i < 4; i++)
        {
            temp.sf[i] = regs->VR_H(v2, i);
            if (temp.sf[i] & 0x00008000) temp.sf[i] |= 0xFFFF0000;
        }
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.sf[i];
        break;
    case 2:  /* Word */
        for (i = 0; i < 2; i++)
        {
            temp.sd[i] = regs->VR_F(v2, i);
            if (temp.sd[i] & 0x0000000080000000ull) temp.sd[i] |= 0xFFFFFFFF00000000ull;
        }
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.sd[i];
        break;
    case 3:  /* Doubleword */
        if (regs->VR_D(v2, 0) & 0x8000000000000000ull)
        {
            regs->VR_D(v1, 1) = regs->VR_D(v2, 0);
            regs->VR_D(v1, 0) = 0xFFFFFFFFFFFFFFFFull;
        }
        else
        {
            regs->VR_D(v1, 1) = regs->VR_D(v2, 0);
            regs->VR_D(v1, 0) = 0x0000000000000000ull;
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D8 VTM    - Vector Test Under Mask                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_test_under_mask )
{
    int     v1, v2, m3, m4, m5;
    union   { U64 d[2]; } temp;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m3, m4, m5 are not part of this instruction */
    UNREFERENCED( m3 );
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    //note: V2 is mask
    temp.d[0] = regs->VR_D(v1,0) & regs->VR_D(v2,0);
    temp.d[1] = regs->VR_D(v1,1) & regs->VR_D(v2,1);

    // Selected bits all zeros; or all mask bits zero
    if ( temp.d[0] == 0 && temp.d[1] == 0 )
        regs->psw.cc = 0;

    // Selected bits all ones
    else if ( temp.d[0] == regs->VR_D(v2,0) && temp.d[1] == regs->VR_D(v2,1) )
        regs->psw.cc = 3;

    // Selected bits a mix of zeros and ones
    else
        regs->psw.cc = 1;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7D9 VECL   - Vector Element Compare Logical              [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_compare_logical )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        if (regs->VR_B( v1, 7 ) == regs->VR_B( v2, 7 ))
            regs->psw.cc = 0;
        else if (regs->VR_B( v1, 7 ) < regs->VR_B( v2, 7 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 1:  /* Halfword */
        if (regs->VR_H( v1, 3 ) == regs->VR_H( v2, 3 ))
            regs->psw.cc = 0;
        else if (regs->VR_H( v1, 3 ) < regs->VR_H( v2, 3 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 2:  /* Word */
        if (regs->VR_F( v1, 1 ) == regs->VR_F( v2, 1 ))
            regs->psw.cc = 0;
        else if (regs->VR_F( v1, 1 ) < regs->VR_F( v2, 1 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 3:  /* Doubleword */
        if (regs->VR_D( v1, 0 ) == regs->VR_D( v2, 0 ))
            regs->psw.cc = 0;
        else if (regs->VR_D( v1, 0 ) < regs->VR_D( v2, 0 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 4:  /* Quadword */
        {
            U128 tempv1, tempv2;
            int  rc;

            tempv1.Q = regs->VR_Q(v1);
            tempv2.Q = regs->VR_Q(v2);
            rc = U128_cmp( tempv1, tempv2 );
            if (rc == 0)
                regs->psw.cc = 0;
            else if (rc == -1)
                regs->psw.cc = 1;
            else
                regs->psw.cc = 2;
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7DB VEC    - Vector Element Compare                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_element_compare )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        if ((S8)regs->VR_B( v1, 7 ) == (S8)regs->VR_B( v2, 7 ))
            regs->psw.cc = 0;
        else if ((S8)regs->VR_B( v1, 7 ) < (S8)regs->VR_B( v2, 7 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 1:  /* Halfword */
        if ((S16)regs->VR_H( v1, 3 ) == (S16)regs->VR_H( v2, 3 ))
            regs->psw.cc = 0;
        else if ((S16)regs->VR_H( v1, 3 ) < (S16)regs->VR_H( v2, 3 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 2:  /* Word */
        if ((S32)regs->VR_F( v1, 1 ) == (S32)regs->VR_F( v2, 1 ))
            regs->psw.cc = 0;
        else if ((S32)regs->VR_F( v1, 1 ) < (S32)regs->VR_F( v2, 1 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 3:  /* Doubleword */
        if ((S64)regs->VR_D( v1, 0 ) == (S64)regs->VR_D( v2, 0 ))
            regs->psw.cc = 0;
        else if ((S64)regs->VR_D( v1, 0 ) < (S64)regs->VR_D( v2, 0 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 4:  /* Quadword */
        {
            U128 tempv1, tempv2;
            int  rc;

            tempv1.Q = regs->VR_Q(v1);
            tempv2.Q = regs->VR_Q(v2);
            rc = S128_cmp( tempv1, tempv2 );
            if (rc == 0)
                regs->psw.cc = 0;
            else if (rc == -1)
                regs->psw.cc = 1;
            else
                regs->psw.cc = 2;
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7DE VLC    - Vector Load Complement                      [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_complement )
{
    int     v1, v2, m3, m4, m5;
    int     i;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = ~(S8)regs->VR_B( v2, i ) + 1;
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = ~(S16)regs->VR_H( v2, i ) + 1;
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = ~(S32)regs->VR_F( v2, i ) + 1;
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = ~(S64)regs->VR_D( v2, i ) + 1;
        break;
    case 4:  /* Quadword */
        {
            U128 tempv1, tempv2;

            tempv2.Q = regs->VR_Q(v2);
            tempv1 = S128_neg( tempv2 );
            regs->VR_Q(v1) = tempv1.Q;
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7DF VLP    - Vector Load Positive                        [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_positive )
{
    int     v1, v2, m3, m4, m5;
    int     i;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    if ( m3 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = (S8)regs->VR_B( v2, i ) < 0 ?
                                        -((S8)regs->VR_B( v2, i )) :
                                        (S8)regs->VR_B( v2, i );
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = (S16)regs->VR_H( v2, i ) < 0 ?
                                         -((S16)regs->VR_H( v2, i )) :
                                         (S16)regs->VR_H( v2, i );
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = (S32)regs->VR_F( v2, i ) < 0 ?
                                         -((S32)regs->VR_F( v2, i )) :
                                         (S32)regs->VR_F( v2, i );
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = (S64)regs->VR_D( v2, i ) < 0 ?
                                         -((S64)regs->VR_D( v2, i )) :
                                         (S64)regs->VR_D( v2, i );
        break;
    case 4:  /* Quadword */
        {
            U128 tempv1, tempv2;

            tempv2.Q = regs->VR_Q(v2);
            if (S128_isNeg( tempv2 ))
                tempv1 = S128_neg( tempv2 );
            else
                tempv1 = tempv2;
            regs->VR_Q(v1) = tempv1.Q;
        }
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F0 VAVGL  - Vector Average Logical                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_average_logical )
{
    int     v1, v2, v3, m4, m5, m6;

    int i;                          /* loop index                    */
    U64 lsa, lsb;                   /* least significant 64-bits     */
    U64 msa;                        /* most significant 64-bits      */

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    /* m5 and m5 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (U8) ( ( (U16) regs->VR_B(v2, i) + (U16) regs->VR_B(v3, i) + 1) >> 1 );
        }
        break;
    case 1:         /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (U16) ( ( (U32) regs->VR_H(v2, i) + (U32) regs->VR_H(v3, i) + 1) >> 1 );
        }
        break;
    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (U32) ( ( (U64) regs->VR_F(v2, i) + (U64) regs->VR_F(v3, i) + 1) >> 1 );
        }
        break;
    case 3:         /* Doubleword */
        for (i=0; i < 2; i++) {
            /* U128 a + U64 b */
            msa = 0;
            lsa = regs->VR_D(v2, i);
            lsb = regs->VR_D(v3, i);

            /* inline U128:  a + b */
            lsa += lsb;
            if ( lsa < lsb ) msa++;

            /* inline U128: a+1 */
            lsa += 1;
            if ( lsa < 1 ) msa++;

            /* shift right: average  */
            regs->VR_D(v1, i) = (lsa >> 1) | ( msa << 63 );
        }
        break;
    case 4:         /* Quadword */
        {
            U128 tempv1, tempv2, tempv3;
            U128 temp1, temp2;
            int carry = 0;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);

            temp1 = U128_add( tempv2, tempv3);
            if ( U128_cmp( temp1, tempv2) == -1 )   carry++;

            temp2 = U128_add( temp1, U128_one() );
            if ( U128_cmp( temp2, temp1) == -1 )    carry++;

            tempv1 = U128_shrl( temp2, 1 );
            if ( carry ) tempv1.Q.D.H.D |= 0x8000000000000000ull;

            regs->VR_Q(v1) = tempv1.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F1 VACC   - Vector Add Compute Carry                    [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_compute_carry )
{
    int     v1, v2, v3, m4, m5, m6;
    union   { U64 d; } temp;
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
        {
            regs->VR_B( v1, i ) = (U8) ( ( (U16)regs->VR_B( v2, i ) + (U16)regs->VR_B( v3, i ) ) >> 8 );
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
        {
            regs->VR_H( v1, i ) = (U16) ( ( (U32)regs->VR_H( v2, i ) + (U32)regs->VR_H( v3, i ) ) >> 16 );
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
        {
            regs->VR_F( v1, i ) = (U32) ( ( (U64)regs->VR_F( v2, i ) + (U64)regs->VR_F( v3, i ) ) >> 32 );
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
        {
            temp.d = 0;
            temp.d += regs->VR_F( v2, (i*2)+1 );
            temp.d += regs->VR_F( v3, (i*2)+1 );
            temp.d >>= 32;
            temp.d += regs->VR_F( v2, i*2 );
            temp.d += regs->VR_F( v3, i*2 );
            regs->VR_D( v1, i ) = temp.d >> 32;
        }
        break;
    case 4:  /* Quadword */
        temp.d = 0;
        for (i=3; i >= 0; i--)
        {
            temp.d += regs->VR_F( v2, i );
            temp.d += regs->VR_F( v3, i );
            temp.d >>= 32;
        }
        regs->VR_D( v1, 0 ) = 0;
        regs->VR_D( v1, 1 ) = temp.d;
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F2 VAVG   - Vector Average                              [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_average )
{
    int     v1, v2, v3, m4, m5, m6;

    int     i;                      /* loop index                    */
    union   { S64 sd; } temp;       /* signed temp                   */

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    ZVECTOR_CHECK( regs );

    /* m5 and m5 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = ( (S16) ( (S8) regs->VR_B(v2, i) + (S8) regs->VR_B(v3, i) ) + 1) >> 1;
        }
        break;
    case 1:         /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = ( (S32) ( (S16) regs->VR_H(v2, i) + (S16) regs->VR_H(v3, i) ) + 1) >> 1;
        }
        break;
    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = ( (S64) ( (S32) regs->VR_F(v2, i) + (S32) regs->VR_F(v3, i) ) + 1) >> 1;
        }
        break;
    case 3:         /* Doubleword */
        for (i=0; i < 2; i++) {
            if  (
                    ( regs->VR_D(v2, i) & 0x8000000000000000ull )  ==
                    ( regs->VR_D(v3, i) & 0x8000000000000000ull )
                )
            {
                /* same signs: possible overflow */
                if  ( regs->VR_D(v2, i) & 0x8000000000000000ull )
                {
                    /* negative signs: allow overflow, round and force back to negative */
                    temp.sd = (S64) regs->VR_D(v2, i) + (S64) regs->VR_D(v3, i);
                    temp.sd++;
                    regs->VR_D(v1, i) = (U64) ( temp.sd >> 1 ) | 0x8000000000000000ull;
                }
                else
                {
                    /* positive signs: handle as U64 values */
                    regs->VR_D(v1, i) = (U64) ( ( (U64) regs->VR_D(v2, i) + (U64) regs->VR_D(v3, i) + 1) >> 1 );
                }
            }
            else
            {
                /* different signs */
                regs->VR_D(v1, i) = (U64) ( ( (S64) regs->VR_D(v2, i) + (S64) regs->VR_D(v3, i) + 1) >> 1 );
            }
        }
        break;
    case 4:         /* Quadword */
        {
            U128 temp128, tempv2, tempv3;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);

            if  (
                    ( regs->VR_D(v2, 0) & 0x8000000000000000ull )  ==
                    ( regs->VR_D(v3, 0) & 0x8000000000000000ull )
                )
            {
                /* same signs: possible overflow */
                if  ( regs->VR_D(v2, 0) & 0x8000000000000000ull )
                {
                    /* negative signs: allow overflow, round and force back to negative */
                    temp128 = S128_add( tempv2, tempv3 );
                    temp128 = S128_add( temp128, U128_one() );
                    temp128 = U128_shrl( temp128, 1 );
                    temp128.Q.D.H.D |= 0x8000000000000000ull;
                    regs->VR_Q(v1) = temp128.Q;
                }
                else
                {
                    /* positive signs: handle as U6128 values */
                    temp128 = S128_add( tempv2, tempv3 );
                    temp128 = S128_add( temp128, U128_one() );
                    temp128 = U128_shrl( temp128, 1 );
                    regs->VR_Q(v1) = temp128.Q;
                }
            }
            else
            {
                /* different signs */
                temp128 = S128_add( tempv2, tempv3 );
                temp128 = S128_add( temp128, U128_one() );
                temp128 = U128_shrl( temp128, 1 );
                regs->VR_Q(v1) = temp128.Q;
            }
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F3 VA     - Vector Add                                  [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_add)
{
    int     v1, v2, v3, m4, m5, m6, i;
    U128    temp;
#if defined( _MSVC_ )
    U128    copyv2, copyv3;
#endif

    VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK(regs);

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) + (S8) regs->VR_B(v3, i);
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) + (S16) regs->VR_H(v3, i);
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) + (S32) regs->VR_F(v3, i);
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64) regs->VR_D(v2, i) + (S64) regs->VR_D(v3, i);
        }
        break;
    case 4:  /* Quadword */
#if defined( _MSVC_ )
        copyv2.Q = regs->VR_Q(v2);
        copyv3.Q = regs->VR_Q(v3);
        temp = U128_add( copyv2, copyv3 );
#else
        temp = U128_add( (U128)regs->VR_Q(v2), (U128)regs->VR_Q(v3) );
#endif
        regs->VR_Q(v1) = temp.Q;
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
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = (regs->VR_B( v2, i ) < regs->VR_B( v3, i )) ? 0 : 1;
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = (regs->VR_H( v2, i ) < regs->VR_H( v3, i )) ? 0 : 1;
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = (regs->VR_F( v2, i ) < regs->VR_F( v3, i )) ? 0 : 1;
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = (regs->VR_D( v2, i ) < regs->VR_D( v3, i )) ? 0 : 1;
        break;
    case 4:  /* Quadword */
        if (regs->VR_D( v2, 0 ) == regs->VR_D( v3, 0 ))
        {
            regs->VR_D( v1, 1 ) = (regs->VR_D( v2, 1 ) < regs->VR_D( v3, 1 )) ? 0 : 1;
            regs->VR_D( v1, 0 ) = 0;
        }
        else
        {
            regs->VR_D( v1, 1 ) = (regs->VR_D( v2, 0 ) < regs->VR_D( v3, 0 )) ? 0 : 1;
            regs->VR_D( v1, 0 ) = 0;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7F7 VS     - Vector Subtract                             [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST(vector_subtract)
{
    int     v1, v2, v3, m4, m5, m6, i;
    U128    temp;
#if defined( _MSVC_ )
    U128    copyv2, copyv3;
#endif

    VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK(regs);

    switch (m4)
    {
    case 0:  /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) - (S8) regs->VR_B(v3, i);
        }
        break;
    case 1:  /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) - (S16) regs->VR_H(v3, i);
        }
        break;
    case 2:  /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) - (S32) regs->VR_F(v3, i);
        }
        break;
    case 3:  /* Doubleword */
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64)regs->VR_D(v2, i) - (S64)regs->VR_D(v3, i);
        }
        break;
    case 4:  /* Quadword */
#if defined( _MSVC_ )
        copyv2.Q = regs->VR_Q(v2);
        copyv3.Q = regs->VR_Q(v3);
        temp = U128_sub( copyv2, copyv3 );
#else
        temp = U128_sub( (U128)regs->VR_Q(v2), (U128)regs->VR_Q(v3) );
#endif
        regs->VR_Q(v1) = temp.Q;
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
    int     v1, v2, v3, m4, m5;
    int     i, el, eq = 0;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    if ( m4 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:  /* Byte */
        for (el=16, i=0; i < 16; i++) {
            if (regs->VR_B(v2, i) == regs->VR_B(v3, i)) {
                regs->VR_B(v1, i) = 0xff;
                eq++;
            }
            else {
                regs->VR_B(v1, i) = 0x00;
            }
        }
        break;
    case 1:  /* Halfword */
        for (el=8, i=0; i < 8; i++) {
            if (regs->VR_H(v2, i) == regs->VR_H(v3, i)) {
                regs->VR_H(v1, i) = 0xffff;
                eq++;
            }
            else {
                regs->VR_H(v1, i) = 0x0000;
            }
        }
        break;
    case 2:  /* Word */
        for (el=4, i=0; i < 4; i++) {
            if (regs->VR_F(v2, i) == regs->VR_F(v3, i)) {
                regs->VR_F(v1, i) = 0xFFFFFFFF;
                eq++;
            }
            else {
                regs->VR_F(v1, i) = 0x00000000;
            }
        }
        break;
    case 3:  /* Doubleword */
        for (el=2, i=0; i < 2; i++) {
            if (regs->VR_D(v2, i) == regs->VR_D(v3, i)) {
                regs->VR_D(v1, i) = 0xFFFFFFFFFFFFFFFFull;
                eq++;
            }
            else {
                regs->VR_D(v1, i) = 0x0000000000000000ull;
            }
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv1, tempv2, tempv3;
            int  rc;

            el = 1;
            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            rc = U128_cmp( tempv2, tempv3 );
            if (rc == 0) {
                tempv1 = U128_minus_one();
                eq++;
            }
            else {
                tempv1 = U128_zero();
            }
            regs->VR_Q(v1) = tempv1.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS) {
        if (eq == el)
            regs->psw.cc = 0;
        else if (eq != 0)
            regs->psw.cc = 1;
        else
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
    int     i, el, hi = 0;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    if ( m4 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:         /* Byte */
        for (el=16, i=0; i < 16; i++) {
            if (regs->VR_B(v2, i) > regs->VR_B(v3, i)) {
                regs->VR_B(v1, i) = 0xff;
                hi++;
            }
            else {
                regs->VR_B(v1, i) = 0x00;
            }
        }
        break;
    case 1:        /* Halfword */
        for (el=8, i=0; i < 8; i++) {
            if (regs->VR_H(v2, i) > regs->VR_H(v3, i)) {
                regs->VR_H(v1, i) = 0xffff;
                hi++;
            }
            else {
                regs->VR_H(v1, i) = 0x0000;
            }
        }
        break;
    case 2:         /* Word */
        for (el=4, i=0; i < 4; i++) {
            if (regs->VR_F(v2, i) > regs->VR_F(v3, i)) {
                regs->VR_F(v1, i) = 0xFFFFFFFF;
                hi++;
            }
            else {
                regs->VR_F(v1, i) = 0x00000000;
            }
        }
        break;
    case 3:        /* Doubleword */
        for (el=2, i=0; i < 2; i++) {
            if (regs->VR_D(v2, i) > regs->VR_D(v3, i)) {
                regs->VR_D(v1, i) = 0xFFFFFFFFFFFFFFFFull;
                hi++;
            }
            else {
                regs->VR_D(v1, i) = 0x0000000000000000ull;
            }
        }
        break;
    case 4:  /* Quadword */
        {
            U128 tempv1, tempv2, tempv3;
            int  rc;

            el = 1;
            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            rc = U128_cmp( tempv2, tempv3 );
            if (rc == 1) {
                tempv1 = U128_minus_one();
                hi++;
            }
            else {
                tempv1 = U128_zero();
            }
            regs->VR_Q(v1) = tempv1.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS) {
        if (hi == el)
            regs->psw.cc = 0;
        else if (hi != 0)
            regs->psw.cc = 1;
        else
            regs->psw.cc = 3;
    }

#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FB VCH    - Vector Compare High                         [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_compare_high )
{
    int     v1, v2, v3, m4, m5;
    int     i, el, hi = 0;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    if ( m4 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:         /* Byte */
        for (el=16, i=0; i < 16; i++) {
            if ( (S8) regs->VR_B(v2, i) > (S8) regs->VR_B(v3, i) ) {
                regs->VR_B(v1, i) = 0xff;
                hi++;
            }
            else {
                regs->VR_B(v1, i) = 0x00;
            }
        }
        break;
    case 1:        /* Halfword */
        for (el=8, i=0; i < 8; i++) {
            if ( (S16) regs->VR_H(v2, i) > (S16) regs->VR_H(v3, i) ) {
                regs->VR_H(v1, i) = 0xffff;
                hi++;
            }
            else {
                regs->VR_H(v1, i) = 0x0000;
            }
        }
        break;
    case 2:         /* Word */
        for (el=4, i=0; i < 4; i++) {
            if ( (S32) regs->VR_F(v2, i) > (S32) regs->VR_F(v3, i) ) {
                regs->VR_F(v1, i) = 0xFFFFFFFF;
                hi++;
            }
            else {
                regs->VR_F(v1, i) = 0x00000000;
            }
        }
        break;
    case 3:        /* Doubleword */
        for (el=2, i=0; i < 2; i++) {
            if ( (S64) regs->VR_D(v2, i) > (S64) regs->VR_D(v3, i) ) {
                regs->VR_D(v1, i) = 0xFFFFFFFFFFFFFFFFull;
                hi++;
            }
            else {
                regs->VR_D(v1, i) = 0x0000000000000000ull;
            }
        }
        break;
    case 4:        /* Quadword */
        {
            U128 tempv1, tempv2, tempv3;
            int  rc;

            el = 1;
            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            rc = S128_cmp( tempv2, tempv3 );
            if (rc == 1) {
                tempv1 = U128_minus_one();
                hi++;
            }
            else {
                tempv1 = U128_zero();
            }
            regs->VR_Q(v1) = tempv1.Q;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS) {
        if (hi == el)
            regs->psw.cc = 0;
        else if (hi != 0)
            regs->psw.cc = 1;
        else
            regs->psw.cc = 3;
    }

#undef M5_CS

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FC VMNL   - Vector Minimum Logical                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_minimum_logical )
{
    int     v1, v2, v3, m4, m5, m6;
    int i;                          /* loop index                    */

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m4 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) =  regs->VR_B(v2, i) <= regs->VR_B(v3, i) ? regs->VR_B(v2, i) : regs->VR_B(v3, i);
        }
        break;
    case 1:         /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = regs->VR_H(v2, i) <= regs->VR_H(v3, i) ? regs->VR_H(v2, i) : regs->VR_H(v3, i);
        }
        break;
    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = regs->VR_F(v2, i) <= regs->VR_F(v3, i) ? regs->VR_F(v2, i) : regs->VR_F(v3, i);
        }
        break;
    case 3:         /* Doubleword */
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = regs->VR_D(v2, i) <= regs->VR_D(v3, i) ? regs->VR_D(v2, i) : regs->VR_D(v3, i);
        }
        break;
    case 4:         /* Quadword */
        {
            U128 tempv2, tempv3;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            regs->VR_Q(v1) = ( U128_cmp( tempv2,  tempv3) == -1)  ? regs->VR_Q(v2) : regs->VR_Q(v3);
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FD VMXL   - Vector Maximum Logical                      [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_maximum_logical )
{
    int     v1, v2, v3, m4, m5, m6;
    int i;                          /* loop index                    */

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m4 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) =  regs->VR_B(v2, i) >= regs->VR_B(v3, i) ? regs->VR_B(v2, i) : regs->VR_B(v3, i);
        }
        break;
    case 1:         /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = regs->VR_H(v2, i) >= regs->VR_H(v3, i) ? regs->VR_H(v2, i) : regs->VR_H(v3, i);
        }
        break;
    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = regs->VR_F(v2, i) >= regs->VR_F(v3, i) ? regs->VR_F(v2, i) : regs->VR_F(v3, i);
        }
        break;
    case 3:         /* Doubleword */
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = regs->VR_D(v2, i) >= regs->VR_D(v3, i) ? regs->VR_D(v2, i) : regs->VR_D(v3, i);
        }
        break;
    case 4:         /* Quadword */
        {
            U128 tempv2, tempv3;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            regs->VR_Q(v1) = ( U128_cmp( tempv2,  tempv3) == 1)  ? regs->VR_Q(v2) : regs->VR_Q(v3);
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FE VMN    - Vector Minimum                              [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_minimum )
{
    int     v1, v2, v3, m4, m5, m6;
    int i;                          /* loop index                    */

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m4 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) <= (S8) regs->VR_B(v3, i) ? regs->VR_B(v2, i) : regs->VR_B(v3, i);
        }
        break;
    case 1:         /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) <= (S16) regs->VR_H(v3, i) ? regs->VR_H(v2, i) : regs->VR_H(v3, i);
        }
        break;
    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) <= (S32) regs->VR_F(v3, i) ? regs->VR_F(v2, i) : regs->VR_F(v3, i);
        }
        break;
    case 3:         /* Doubleword */
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64) regs->VR_D(v2, i) <= (S64) regs->VR_D(v3, i) ? regs->VR_D(v2, i) : regs->VR_D(v3, i);
        }
        break;
    case 4:         /* Quadword */
        {
            U128 tempv2, tempv3;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            regs->VR_Q(v1) = ( S128_cmp( tempv2,  tempv3) == -1)  ? regs->VR_Q(v2) : regs->VR_Q(v3);
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E7FF VMX    - Vector Maximum                              [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_maximum )
{
    int     v1, v2, v3, m4, m5, m6;
    int i;                          /* loop index                    */

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    if ( m4 == 4 && !FACILITY_ENABLED( 198_VECTOR_ENH_3, regs ) )
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) >= (S8) regs->VR_B(v3, i) ? regs->VR_B(v2, i) : regs->VR_B(v3, i);
        }
        break;
    case 1:         /* Halfword */
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) >= (S16) regs->VR_H(v3, i) ? regs->VR_H(v2, i) : regs->VR_H(v3, i);
        }
        break;
    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) >= (S32) regs->VR_F(v3, i) ? regs->VR_F(v2, i) : regs->VR_F(v3, i);
        }
        break;
    case 3:         /* Doubleword */
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64) regs->VR_D(v2, i) >= (S64) regs->VR_D(v3, i) ? regs->VR_D(v2, i) : regs->VR_D(v3, i);
        }
        break;
    case 4:         /* Quadword */
        {
            U128 tempv2, tempv3;

            tempv2.Q = regs->VR_Q(v2);
            tempv3.Q = regs->VR_Q(v3);
            regs->VR_Q(v1) = ( S128_cmp( tempv2,  tempv3) == 1)  ? regs->VR_Q(v2) : regs->VR_Q(v3);
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

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
