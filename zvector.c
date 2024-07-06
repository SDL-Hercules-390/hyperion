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

/* ====================================================================== */
/* TEMPORARY while zVector instructions are being developed */

//  #if defined(__clang__)
//      #pragma clang diagnostic ignored "-Wunused-variable"
//      #pragma clang diagnostic ignored "-Wunused-but-set-variable"
//      #pragma clang diagnostic ignored "-Wcomment"
//      #pragma clang diagnostic ignored "-Wsometimes-uninitialized"
//      #pragma clang diagnostic ignored "-Wmacro-redefined"
//  #elif defined(__GNUC__)
//      #pragma GCC diagnostic ignored "-Wunused-variable"
//      #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
//      #pragma GCC diagnostic ignored "-Wcomment"
//      #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
//  #endif

//  #undef ZVECTOR_CHECK

//  #define ZVECTOR_CHECK(_regs)  /* (do nothing) */

//  #undef ZVECTOR_END

/* remove GCC: warning: multi-line comment [-Wcomment]

//  #define ZVECTOR_END(_regs)                                      \
//              ARCH_DEP(display_inst) (_regs, inst);

//  #define ZVECTOR_END(_regs)                                      \
//          if (0 && inst[5] != (U8) 0x3E && inst[5] != (U8) 0x36)  \
//              ARCH_DEP(display_inst) (_regs, inst);

*/ /*end remove GCC: warning: multi-line comment [-Wcomment] */

/* ====================================================================== */

#if defined( FEATURE_129_ZVECTOR_FACILITY )

/*------------------------------------------------------------------------*/
/* See ieee.c for the following Vector Floating-Point Instructions.       */
/*------------------------------------------------------------------------*/
/* E74A VFTCI  - Vector FP Test Data Class Immediate              [VRI-e] */
/* E78E VFMS   - Vector FP Multiply and Subtract                  [VRR-e] */
/* E78F VFMA   - Vector FP Multiply and Add                       [VRR-e] */
/* E79E VFNMS  - Vector FP Negative Multiply And Subtract         [VRR-e] */
/* E79F VFNMA  - Vector FP Negative Multiply And Add              [VRR-e] */
/* E7C0 VCLFP  - Vector FP Convert To Logical (short BFP to 32)   [VRR-a] */
/* E7C0 VCLGD  - Vector FP Convert To Logical (long BFP to 64)    [VRR-a] */
/* E7C1 VCFPL  - Vector FP Convert From Logical (32 to short BFP) [VRR-a] */
/* E7C1 VCDLG  - Vector FP Convert From Logical (64 to long BFP)  [VRR-a] */
/* E7C2 VCSFP  - Vector FP Convert To Fixed (short BFP to 32)     [VRR-a] */
/* E7C2 VCGD   - Vector FP Convert To Fixed (long BFP to 64)      [VRR-a] */
/* E7C3 VCFPS  - Vector FP Convert From Fixed (32 to short BFP)   [VRR-a] */
/* E7C3 VCDG   - Vector FP Convert From Fixed (64 to long BFP)    [VRR-a] */
/* E7C4 VFLL   - Vector FP Load Lengthened                        [VRR-a] */
/* E7C5 VFLR   - Vector FP Load Rounded                           [VRR-a] */
/* E7C7 VFI    - Vector Load FP Integer                           [VRR-a] */
/* E7CA WFK    - Vector FP Compare and Signal Scalar              [VRR-a] */
/* E7CB WFC    - Vector FP Compare Scalar                         [VRR-a] */
/* E7CC VFPSO  - Vector FP Perform Sign Operation                 [VRR-a] */
/* E7CE VFSQ   - Vector FP Square Root                            [VRR-a] */
/* E7E2 VFS    - Vector FP Subtract                               [VRR-c] */
/* E7E3 VFA    - Vector FP Add                                    [VRR-c] */
/* E7E5 VFD    - Vector FP Divide                                 [VRR-c] */
/* E7E7 VFM    - Vector FP Multiply                               [VRR-c] */
/* E7E8 VFCE   - Vector FP Compare Equal                          [VRR-c] */
/* E7EA VFCHE  - Vector FP Compare High or Equal                  [VRR-c] */
/* E7EB VFCH   - Vector FP Compare High                           [VRR-c] */
/* E7EE VFMIN  - Vector FP Minimum                                [VRR-c] */
/* E7EF VFMAX  - Vector FP Maximum                                [VRR-c] */
/*------------------------------------------------------------------------*/

#undef __V128_SSE__

/* INTEL X64 processor? */
#if defined( __x86_64__ ) || defined( _M_X64 )
    /* MSVC on X64: intrinsics are available and should be used for optimization */
    #if defined( _MSC_VER ) || defined( _MSVC_ )
        #define __V128_SSE__ 1

    /* gcc/clang on X64: intrinsics are available and should be used for optimization */
    /*                   Being conservative: require SSE 4.2 to be available to allow */
    /*                   any SSE intrinsic to be used for optimization.               */
    #elif defined( __GNUC__ ) && defined( __SSE4_2__ )
        #define __V128_SSE__ 1

    #endif

#endif

/* compile debug message: are we using intrinsics? */
#if 0
    #if defined(__V128_SSE__)
        #pragma message("__V128_SSE__ is defined.  Using intrinsics." )
    #else
        #pragma message("No intrinsics are included for optimization; only compiler optimization")
    #endif
#endif

/*===================================================================*/
/* Achitecture Independent Routines                                  */
/*===================================================================*/

#if !defined(_ZVECTOR_ARCH_INDEPENDENT_)
#define _ZVECTOR_ARCH_INDEPENDENT_

/*-------------------------------------------------------------------*/
/* Galois Field Multiply                                             */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Galois Field(2) 32-bit Multiply                                   */
/*                                                                   */
/* Input:                                                            */
/*      m1      32-bit multiply operand                              */
/*      m2      32-bit multiply operand                              */
/*                                                                   */
/* Returns:                                                          */
/*              64-bit GF(2) multiply result                         */
/*                                                                   */
/* version depends on whether intrinsics are being used              */
/*-------------------------------------------------------------------*/
static inline U64 gf_mul_32( U32 m1, U32 m2)
{
#if defined( __V128_SSE__ )
    /* intrinsic GF 64-bit multiply */
    QW  mm1;                      /* U128 m1                       */
    QW  mm2;                      /* U128 m2                       */
    QW  acc;                      /* U128 accumulator              */

    mm1.v = _mm_setzero_si128();
    mm1.D.L.D = m1;
        //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_32 mm1.v", mm1.D.H.D, mm1.D.L.D);

    mm2.v = _mm_setzero_si128();
    mm2.D.L.D = m2;
        //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_32 mm2.v", mm2.D.H.D, mm2.D.L.D);

    acc.v =  _mm_clmulepi64_si128 ( mm1.v, mm2.v, 0);
        //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_32 acc.v", acc.D.H.D, acc.D.L.D);

    return acc.D.L.D;

#else
    int     i;                    /* loop index                      */
    U32     myerU32;              /* multiplier                      */
    U64     mcandU64;             /* multiplicand                    */
    U64     accu64;               /* accumulator                     */

    accu64 = 0;

    /* select muliplier with fewest 'right most' bits */
    /* to exit loop soonest                           */
    if (m1 < m2)
    {
        mcandU64 = m2;
        myerU32  = m1;
    }
    else
    {
        mcandU64 = m1;
        myerU32  = m2;
    }

    /* galois multiply - no overflow */
    for (i=0; i < 32 && myerU32 !=0; i++)
    {
        if ( myerU32 & 0x01 )
            accu64 ^= mcandU64;
        myerU32  >>= 1;
        mcandU64 <<=1;
    }

    return accu64;

#endif

}

/*-------------------------------------------------------------------*/
/* Galois Field(2) 64-bit Multiply                                   */
/*                                                                   */
/* Input:                                                            */
/*      m1          64-bit multiply operand                          */
/*      m2          64-bit multiply operand                          */
/*      accu128h    pointer to high 64 bits of result                */
/*      accu128l    pointer to low 64 bits of result                 */
/*                                                                   */
/* version depends on whether intrinsics are being used              */
/*-------------------------------------------------------------------*/
static inline void gf_mul_64( U64 m1, U64 m2, U64* accu128h, U64* accu128l)
{
#if defined( __V128_SSE__ )
    /* intrinsic GF 64-bit multiply */
    QW  mm1;                      /* U128 m1                       */
    QW  mm2;                      /* U128 m2                       */
    QW  acc;                      /* U128 accumulator              */

    mm1.v = _mm_setzero_si128();
    mm1.D.L.D = m1;
        //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_64 mm1.v", mm1.D.H.D, mm1.D.L.D);

    mm2.v = _mm_setzero_si128();
    mm2.D.L.D = m2;
        //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_64 mm2.v", mm2.D.H.D, mm2.D.L.D);

    acc.v =  _mm_clmulepi64_si128 ( mm1.v, mm2.v, 0);
        //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_64 acc.v", acc.D.H.D, acc.D.L.D);

    *accu128h = acc.D.H.D;
    *accu128l = acc.D.L.D;

#else
    /* portable C: GF 64-bit multiply */
    int     i;                    /* loop index                      */
    U64     myerU64;              /* doublewword multiplier          */
    U64     mcandU128h;           /* doublewword multiplicand - high */
    U64     mcandU128l;           /* doublewword multiplicand - low  */

    *accu128h = 0;
    *accu128l = 0;

    /* select muliplier with fewest 'right most' bits */
    /* to exit loop soonest                           */
    if (m1 < m2)
    {
        mcandU128h  = 0;
        mcandU128l = m2;
        myerU64  = m1;
    }
    else
    {
        mcandU128h  = 0;
        mcandU128l = m1;
        myerU64  = m2;
    }

    /* galois multiply - no overflow */
    for (i=0; i < 64 && myerU64 !=0; i++)
    {
        if ( myerU64 & 0x01 )
        {
            *accu128h ^= mcandU128h;
            *accu128l ^= mcandU128l;
        }
        myerU64  >>= 1;

        /* U128: shift left 1 bit*/
        mcandU128h = (mcandU128h << 1) | (mcandU128l >> 63);
        mcandU128l <<= 1;
    }

#endif

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

    /* m3 is not part of this instruction */
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

    /* m3 is not part of this instruction */
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
    case 0:
        if ( i > 15 ) break;
        regs->GR( r1 ) = regs->VR_B( v3, i );
        break;
    case 1:
        if ( i > 7 ) break;
        regs->GR( r1 ) = regs->VR_H( v3, i );
        break;
    case 2:
        if ( i > 3 ) break;
        regs->GR( r1 ) = regs->VR_F( v3, i );
        break;
    case 3:
        if ( i > 1 ) break;
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
    case 0:
        if ( i > 15 ) break;
        regs->VR_B( v1, i ) = regs->GR_LHLCL( r3 );
        break;
    case 1:
        if ( i > 7 ) break;
        regs->VR_H( v1, i ) = regs->GR_LHL  ( r3 );
        break;
    case 2:
        if ( i > 3 ) break;
        regs->VR_F( v1, i ) = regs->GR_L    ( r3 );
        break;
    case 3:
        if ( i > 1 ) break;
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
    int     r1, x2, b2, m3, length;
    VADR    effective_addr2, nextbound;
    U64     boundary;

    RXE_M3( inst, regs, r1, x2, b2, effective_addr2, m3 );

    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 6)                    /* M3 > 6 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    boundary = 64 << m3; /* 0: 64 Byte, 1: 128 Byte, 2: 256 Byte, 3: 512 Byte,
                            4: 1 K-byte, 5: 2 K-Byte, 6: 4 K-Byte */

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

    /* m4 is not part of this instruction */
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
    int     v1, r3, b2, m4, i;
    VADR    effective_addr2;
    BYTE    temp[16];

    VRS_B( inst, regs, v1, r3, b2, effective_addr2, m4 );

    /* m4 is not part of this instruction */
    UNREFERENCED( m4 );

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

    /* m4 is not part of this instruction */
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
    int     v1, r3, b2, m4, len, i;
    VADR    effective_addr2;
    BYTE    temp[16];

    VRS_B( inst, regs, v1, r3, b2, effective_addr2, m4 );

    /* m4 is not part of this instruction */
    UNREFERENCED( m4 );

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
        bitmask = (i2 <= i3) ? (1u << (8 - i2)) - (1u << (7 - i3)) : 0xffu - (1u << (7 - i3)) + (1u << (8 - i2));
        for (i=0; i < 16; i++)
            regs->VR_B(v1, i) = bitmask;
        break;
    case 1:
        i2 &= 15;
        i3 &= 15;
        bitmask = (i2 <= i3) ? (1u << (16 - i2)) - (1u << (15 - i3)) : 0xffffu - (1u << (15 - i3)) + (1u << (16 - i2));
        for (i=0; i < 8; i++)
            regs->VR_H(v1, i) = bitmask;
        break;
    case 2:
        i2 &= 31;
        i3 &= 31;
        bitmask = (i2 <= i3) ? (1u << (32 - i2)) - (1u << (31 - i3)) : 0xffffffffu - (1u << (31 - i3)) + (1u << (32 - i2));
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
    int     i, j, count;
    U64     delement;
    U32     felement;
    U16     helement;
    BYTE    belement;

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
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            count = 0;
            belement = regs->VR_B(v2, i);
            for (j=0; j < 8; j++)
            {
                if (belement & 0x80) count++;
                belement <<= 1;
            }
            regs->VR_B(v1, i) = count;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            count = 0;
            helement = regs->VR_H(v2, i);
            for (j=0; j < 16; j++)
            {
                if (helement & 0x8000) count++;
                helement <<= 1;
            }
            regs->VR_H(v1, i) = count;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            count = 0;
            felement = regs->VR_F(v2, i);
            for (j=0; j < 32; j++)
            {
                if (felement & 0x80000000) count++;
                felement <<= 1;
            }
            regs->VR_F(v1, i) = count;
        }
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++)
        {
            count = 0;
            delement = regs->VR_D(v2, i);
            for (j=0; j < 64; j++)
            {
                if (delement & 0x8000000000000000ull) count++;
                delement <<= 1;
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
    int     v1, v2, m3, m4, m5;
    int     i, j, count;
    U64     delement;
    U32     felement;
    U16     helement;
    BYTE    belement;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    switch (m3)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            belement = regs->VR_B(v2, i);
            for (j=0, count=0; j < 8; j++, count++)
            {
                if (belement & 0x01) break;
                belement >>= 1;
            }
            regs->VR_B(v1, i) = count;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            helement = regs->VR_H(v2, i);
            for (j=0, count=0; j < 16; j++, count++)
            {
                if (helement & 0x0001) break;
                helement >>= 1;
            }
            regs->VR_H(v1, i) = count;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            felement = regs->VR_F(v2, i);
            for (j=0, count=0; j < 32; j++, count++)
            {
                if (felement & 0x00000001) break;
                felement >>= 1;
            }
            regs->VR_F(v1, i) = count;
        }
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++)
        {
            delement = regs->VR_D(v2, i);
            for (j=0, count=0; j < 64; j++, count++)
            {
                if (delement & 0x0000000000000001ull) break;
                delement >>= 1;
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
/* E753 VCLZ   - Vector Count Leading Zeros                  [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_count_leading_zeros )
{
    int     v1, v2, m3, m4, m5;
    int     i, j, count;
    U64     delement;
    U32     felement;
    U16     helement;
    BYTE    belement;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m4, m5 are not part of this instruction */
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    switch (m3)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            belement = regs->VR_B(v2, i);
            for (j=0, count=0; j < 8; j++, count++)
            {
                if (belement & 0x80) break;
                belement <<= 1;
            }
            regs->VR_B(v1, i) = count;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            helement = regs->VR_H(v2, i);
            for (j=0, count=0; j < 16; j++, count++)
            {
                if (helement & 0x8000) break;
                helement <<= 1;
            }
            regs->VR_H(v1, i) = count;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            felement = regs->VR_F(v2, i);
            for (j=0, count=0; j < 32; j++, count++)
            {
                if (felement & 0x80000000) break;
                felement <<= 1;
            }
            regs->VR_F(v1, i) = count;
        }
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++)
        {
            delement = regs->VR_D(v2, i);
            for (j=0, count=0; j < 64; j++, count++)
            {
                if (delement & 0x8000000000000000ull) break;
                delement <<= 1;
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
    case 0:  // Byte
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
    case 1:  // Halfword
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
    case 2:  // Word
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
    case 0:  // Byte
        element = regs->VR_B(v2, 7);
        if (element & 0x0000000000000080ull)
            element |= 0xFFFFFFFFFFFFFF00ull;
        regs->VR_D(v1, 0) = element;
        element = regs->VR_B(v2, 15);
        if (element & 0x0000000000000080ull)
            element |= 0xFFFFFFFFFFFFFF00ull;
        regs->VR_D(v1, 1) = element;
        break;
    case 1:  // Halfword
        element = regs->VR_H(v2, 3);
        if (element & 0x0000000000008000ull)
            element |= 0xFFFFFFFFFFFF0000ull;
        regs->VR_D(v1, 0) = element;
        element = regs->VR_H(v2, 7);
        if (element & 0x0000000000008000ull)
            element |= 0xFFFFFFFFFFFF0000ull;
        regs->VR_D(v1, 1) = element;
        break;
    case 2:  // Word
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

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for ( i=0, j=8; i<16; i+=2, j++ )
        {
            regs->VR_B( v1, i   ) = regs->VR_B( v2, j );
            regs->VR_B( v1, i+1 ) = regs->VR_B( v3, j );
        }
        break;
    case 1:  // Halfword
        for ( i=0, j=4; i<8; i+=2, j++ )
        {
            regs->VR_H( v1, i   ) = regs->VR_H( v2, j );
            regs->VR_H( v1, i+1 ) = regs->VR_H( v3, j );
        }
        break;
    case 2:  // Word
        for ( i=0, j=2; i<4; i+=2, j++ )
        {
            regs->VR_F( v1, i   ) = regs->VR_F( v2, j );
            regs->VR_F( v1, i+1 ) = regs->VR_F( v3, j );
        }
        break;
    case 3:  // Doubleword
        regs->VR_D( v1, 0 ) = regs->VR_D( v2, 1 );
        regs->VR_D( v1, 1 ) = regs->VR_D( v3, 1 );
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E761 VMRH   - Vector Merge High                           [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_merge_high )
{
    int     v1, v2, v3, m4, m5, m6;
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for ( i=0, j=0; i<16; i+=2, j++ )
        {
            regs->VR_B( v1, i   ) = regs->VR_B( v2, j );
            regs->VR_B( v1, i+1 ) = regs->VR_B( v3, j );
        }
        break;
    case 1:  // Halfword
        for ( i=0, j=0; i<8; i+=2, j++ )
        {
            regs->VR_H( v1, i   ) = regs->VR_H( v2, j );
            regs->VR_H( v1, i+1 ) = regs->VR_H( v3, j );
        }
        break;
    case 2:  // Word
        for ( i=0, j=0; i<4; i+=2, j++ )
        {
            regs->VR_F( v1, i   ) = regs->VR_F( v2, j );
            regs->VR_F( v1, i+1 ) = regs->VR_F( v3, j );
        }
        break;
    case 3:  // Doubleword
        regs->VR_D( v1, 0 ) = regs->VR_D( v2, 0 );
        regs->VR_D( v1, 1 ) = regs->VR_D( v3, 0 );
        break;
    default:
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        break;
    }

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
    case 0:  // Byte
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
    case 1:  // Halfword
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
    case 1:  // Halfword
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
    case 2:  // Word
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
    case 2:  // Word
        for (i = 0; i < 4; i++)
        {
            add += regs->VR_F(v2, i);
        }
        add += regs->VR_F(v3, 3);
        break;
    case 3:  // Doubleword
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

    regs->VR_D(v1, 0) = ~regs->VR_D(v2, 0) & ~regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = ~regs->VR_D(v2, 1) & ~regs->VR_D(v3, 1);

    ZVECTOR_END( regs );
}

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

    regs->VR_D(v1, 0) = ~regs->VR_D(v2, 0) | ~regs->VR_D(v3, 0);
    regs->VR_D(v1, 1) = ~regs->VR_D(v2, 1) | ~regs->VR_D(v3, 1);

    ZVECTOR_END( regs );
}

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
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            sl = i4 % 8;
            sr = 8 - sl;
            temp.b[1] = ((regs->VR_B( v2, i ) << sl) | (regs->VR_B( v2, i ) >> sr)) & regs->VR_B( v3, i );
            temp.b[0] = regs->VR_B( v1, i ) & (~regs->VR_B( v3, i ));
            regs->VR_B( v1, i ) = temp.b[0] | temp.b[1];
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            sl = i4 % 16;
            sr = 16 - sl;
            temp.h[1] = ((regs->VR_H( v2, i ) << sl) | (regs->VR_H( v2, i ) >> sr)) & regs->VR_H( v3, i );
            temp.h[0] = regs->VR_H( v1, i ) & (~regs->VR_H( v3, i ));
            regs->VR_H( v1, i ) = temp.h[0] | temp.h[1];
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            sl = i4 % 32;
            sr = 32 - sl;
            temp.f[1] = ((regs->VR_F( v2, i ) << sl) | (regs->VR_F( v2, i ) >> sr)) & regs->VR_F( v3, i );
            temp.f[0] = regs->VR_F( v1, i ) & (~regs->VR_F( v3, i ));
            regs->VR_F( v1, i ) = temp.f[0] | temp.f[1];
        }
        break;
    case 3:  // Doubleword
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
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            sl = regs->VR_B( v3, i ) % 8;
            sr = 8 - sl;
            regs->VR_B( v1, i ) = (regs->VR_B( v2, i ) << sl) | (regs->VR_B( v2, i ) >> sr);
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            sl = regs->VR_H( v3, i ) % 16;
            sr = 16 - sl;
            regs->VR_H( v1, i ) = (regs->VR_H( v2, i ) << sl) | (regs->VR_H( v2, i ) >> sr);
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            sl = regs->VR_F( v3, i ) % 32;
            sr = 32 - sl;
            regs->VR_F( v1, i ) = (regs->VR_F( v2, i ) << sl) | (regs->VR_F( v2, i ) >> sr);
        }
        break;
    case 3:  // Doubleword
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
/* git7 VSLDB  - Vector Shift Left Double By Byte            [VRI-d] */
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
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            shift = regs->VR_B( v3, i ) % 8;
            regs->VR_B( v1, i ) = regs->VR_B( v2, i ) >> shift;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            shift = regs->VR_H( v3, i ) % 16;
            regs->VR_H( v1, i ) = regs->VR_H( v2, i ) >> shift;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            shift = regs->VR_F( v3, i ) % 32;
            regs->VR_F( v1, i ) = regs->VR_F( v2, i ) >> shift;
        }
        break;
    case 3:  // Doubleword
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
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            shift = regs->VR_B( v3, i ) % 8;
            regs->VR_B( v1, i ) = (S8) regs->VR_B( v2, i ) >> shift;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            shift = regs->VR_H( v3, i ) % 16;
            regs->VR_H( v1, i ) = (S16) regs->VR_H( v2, i ) >> shift;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            shift = regs->VR_F( v3, i ) % 32;
            regs->VR_F( v1, i ) = (S32) regs->VR_F( v2, i ) >> shift;
        }
        break;
    case 3:  // Doubleword
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
    ei = 16;    // Number of bytes in vector
    newcc = 3;  // No equal, no zero

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i<16; i++)
        {
            if (regs->VR_B(v2,i) == regs->VR_B(v3,i))
            {
                ef = TRUE;
                ei = i;     // Element index in bytes
                newcc = 1;  // Equal, no zero
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
    case 1:  // Halfword
        for (i=0; i<8; i++)
        {
            if (regs->VR_H(v2,i) == regs->VR_H(v3,i))
            {
                ef = TRUE;
                ei = i * 2;  // Element index in bytes
                newcc = 1;   // Equal, no zero
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
    case 2:  // Word
        for (i=0; i<4; i++)
        {
            if (regs->VR_F(v2,i) == regs->VR_F(v3,i))
            {
                ef = TRUE;
                ei = i * 4;  // Element index in bytes
                newcc = 1;   // Equal, no zero
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

    if (zf)
    {
        if (ef)
        {
            if (zi < ei)
            {
                ei = zi;    // Element index in bytes
                newcc = 0;  // Zero before equal
            }
            else
            {
                newcc = 2;  // Zero not before equal
            }
        }
        else
        {
            ei = zi;        // Element index in bytes
            newcc = 0;      // Zero before equal
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
    nei = 16;   // Number of bytes in vector
    newcc = 3;  // All equal, no zero

    switch (m4)
    {
    case 0:  // Byte
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
    case 1:  // Halfword
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
    case 2:  // Word
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

    if (zf)
    {
        if (nef)
        {
            if (zi < nei)
            {
                nei = zi;   // Element index in bytes
                newcc = 0;  // Zero before not equal
            }
        }
        else
        {
            nei = zi;       // Element index in bytes
            newcc = 0;      // Zero before equal
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
// Overkill to prevent erroneous "may be used uninitialized" warnings
#if defined(__clang__)
    #pragma clang diagnostic ignored "-Wsometimes-uninitialized"
#elif defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

    int     v1, v2, v3, m4, m5;
    int     i, j;
    BYTE    irt1[16], irt2[16];        // First and second intermediate results
    int     lxt1, lxt2;                // Lowest indexed true
    int     mxt;                       // Maximum indexed true

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_IN ((m5 & 0x8) != 0) // Invert Result
#define M5_RT ((m5 & 0x4) != 0) // Result Type
#define M5_ZS ((m5 & 0x2) != 0) // Zero Search
#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i<16; i++)
        {
            irt1[i] = irt2[i] = FALSE;
            for (j=0; j<16; j++)
            {
                if (regs->VR_B(v2,i) == regs->VR_B(v3,j))
                {
                    irt1[i] = TRUE;
                    break;
                }
            }
            if (M5_ZS)                 // if M5_ZS (Zero Search)
            {
                if (regs->VR_B(v2,i) == 0)
                {
                    irt2[i] = TRUE;
                }
            }
            if (M5_IN)                 // if M5_IN (Invert Result)
            {
                irt1[i] ^= TRUE;
            }
        }
        if (M5_RT)                     // if M5_RT (Result Type)
        {
            for (i=0; i<16; i++)
            {
                if (irt1[i] == TRUE)
                    regs->VR_B(v1,i) = 0xFF;
                else
                    regs->VR_B(v1,i) = 0x00;
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
    case 1:  // Halfword
        for (i=0; i<8; i++)
        {
            irt1[i] = irt2[i] = FALSE;
            for (j=0; j<8; j++)
            {
                if (regs->VR_H(v2,i) == regs->VR_H(v3,j))
                {
                    irt1[i] = TRUE;
                    break;
                }
            }
            if (M5_ZS)                 // if M5_ZS (Zero Search)
            {
                if (regs->VR_H(v2,i) == 0)
                {
                    irt2[i] = TRUE;
                }
            }
            if (M5_IN)                 // if M5_IN (Invert Result)
            {
                irt1[i] ^= TRUE;
            }
        }
        if (M5_RT)                     // if M5_RT (Result Type)
        {
            for (j=0; j<8; j++)
            {
                if (irt1[i] == TRUE)
                    regs->VR_H(v1,i) = 0xFFFF;
                else
                    regs->VR_H(v1,i) = 0x0000;
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
    case 2:  // Word
        for (i=0; i<4; i++)
        {
            irt1[i] = irt2[i] = FALSE;
            for (j=0; j<4; j++)
            {
                if (regs->VR_F(v2,i) == regs->VR_F(v3,j))
                {
                    irt1[i] = TRUE;
                    break;
                }
            }
            if (M5_ZS)                 // if M5_ZS (Zero Search)
            {
                if (regs->VR_F(v2,i) == 0)
                {
                    irt2[i] = TRUE;
                }
            }
            if (M5_IN)                 // if M5_IN (Invert Result)
            {
                irt1[i] ^= TRUE;
            }
        }
        if (M5_RT)                     // if M5_RT (Result Type)
        {
            for (j=0; j<4; j++)
            {
                if (irt1[i] == TRUE)
                    regs->VR_F(v1,i) = 0xFFFFFFFF;
                else
                    regs->VR_F(v1,i) = 0x00000000;
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
        case 0:  // Byte
            lxt1 = lxt2 = mxt = 16;
            break;
        case 1:  // Halfword
            lxt1 = lxt2 = mxt = 8;
            break;
        case 2:  // Word
            lxt1 = lxt2 = mxt = 4;
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

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

#define M4_SO ((m4 & 0x4) != 0)  // Second operand index
#define M4_TO ((m4 & 0x1) != 0)  // Third operand index

    regs->VR_D( v1, 0 ) = regs->VR_D( v2, M4_SO );
    regs->VR_D( v1, 1 ) = regs->VR_D( v3, M4_TO );

#undef M4_SO
#undef M4_TO

    ZVECTOR_END( regs );
}

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

    for (i = 3; i > 0; i--) {
        j = SV_D( temp, i-1 ) << ( 64 - i4 );
        k = SV_D( temp, i ) >> i4;
        SV_D( temp, i ) = j | k;
    }

    regs->VR_D( v1, 0 ) = SV_D( temp, 2 );
    regs->VR_D( v1, 1 ) = SV_D( temp, 3 );

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
        /* remove initialization warnings */
        max = 0, low1 = max, low2 = max;

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
/* E78B VSTRS  - Vector String Search                        [VRR-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_string_search )
{
    int     v1, v2, v3, v4, m5, m6;
    char    v2_temp[16], v3_temp[16], nulls[16];
    int     substr_len, str_len, i, k, eos;
    int     char_size = 1; /* initialize to remove warning */

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    ZVECTOR_CHECK( regs );

#define M6_RE ((m6 & 0xD) != 0) // Reserved
#define M6_ZS ((m6 & 0x2) != 0) // Zero Search

    if (m5 > 2 || M6_RE)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Get the contents of v2 and v3 as a string of bytes arranged */
    /* as they would be if they were in the guests storage.        */
    for (i = 0; i < 16; i++)
        v2_temp[i] = regs->VR_B(v2, i);
    for (i = 0; i < 16; i++)
        v3_temp[i] = regs->VR_B( v3, i );

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
    }

    substr_len = regs->VR_B( v4, 7 );
    if (substr_len < 0 || substr_len > 16)
        goto vector_string_search_mdresult;

    str_len = i = k = eos = 0;

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
                            if ( memcmp(&v2_temp[k], &v3_temp[0], k + substr_len - 16) == 0 )
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
                    if ( memcmp(&v2_temp[k], &v3_temp[0], k + substr_len - 16) == 0 )
                    {
                        goto vector_string_search_partial_match;
                    }
                }
            }
        }
    }

    UNREACHABLE_CODE( goto vector_string_search_mdresult );

vector_string_search_mdresult:
    regs->VR_D( v1, 0 ) = 0;                     /* Model dependant */
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
    case 1:  // Low-order bytes from 16 halfwords
        for ( i = 0; i < 16; i++ )
        {
            regs->VR_B( v1, i ) = SV_B( temp, (i*2)+1 );
        }
        break;
    case 2:  // Low-order halfwords from 8 fullwords
        for ( i = 0; i < 8; i++ )
        {
            regs->VR_H( v1, i ) = SV_H( temp, (i*2)+1 );
        }
        break;
    case 3:  // Low-order fullwords from 4 doublewords
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
    int     sat = 0;
    int     allsat = 0;
    int     i;
    BYTE    newcc;
    SV      temp;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0)  // Condition Code Set

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    switch (m4)
    {
    case 1:  // Low-order bytes from 16 halfwords
        for ( i = 0; i < 16; i++ )
        {
            if ( SV_H( temp, i ) > 0x00FF )
            {
                regs->VR_B( v1, i ) = 0xFF;
                sat++;
            }
            else
            {
                regs->VR_B( v1, i ) = SV_B( temp, (i*2)+1 );
            }
        }
        allsat = 16;
        break;
    case 2:  // Low-order halfwords from 8 fullwords
        for ( i = 0; i < 8; i++ )
        {
            if ( SV_F( temp, i ) > 0x0000FFFF )
            {
                regs->VR_H( v1, i ) = 0xFFFF;
                sat++;
            }
            else
            {
                regs->VR_H( v1, i ) = SV_H( temp, (i*2)+1 );
            }
        }
        allsat = 8;
        break;
    case 3:  // Low-order fullwords from 4 doublewords
        for ( i = 0; i < 4; i++ )
        {
            if ( SV_D( temp, i ) > 0x00000000FFFFFFFFull )
            {
                regs->VR_F( v1, i ) = 0xFFFFFFFF;
                sat++;
            }
            else
            {
                regs->VR_F( v1, i ) = SV_F( temp, (i*2)+1 );
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
    int     sat = 0;
    int     allsat = 0;
    int     i;
    BYTE    newcc;
    SV      temp;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0)  // Condition Code Set

    SV_D( temp, 0 ) = regs->VR_D( v2, 0 );
    SV_D( temp, 1 ) = regs->VR_D( v2, 1 );
    SV_D( temp, 2 ) = regs->VR_D( v3, 0 );
    SV_D( temp, 3 ) = regs->VR_D( v3, 1 );

    switch (m4)
    {
    case 1:  // Low-order bytes from 16 halfwords
        for ( i = 0; i < 16; i++ )
        {
            if ( SV_H( temp, i ) > 0x007F )
            {
                regs->VR_B( v1, i ) = 0x7F;
                sat++;
            }
            else
            {
                regs->VR_B( v1, i ) = SV_B( temp, (i*2)+1 );
            }
        }
        allsat = 16;
        break;
    case 2:  // Low-order halfwords from 8 fullwords
        for ( i = 0; i < 8; i++ )
        {
            if ( SV_F( temp, i ) > 0x00007FFF )
            {
                regs->VR_H( v1, i ) = 0x7FFF;
                sat++;
            }
            else
            {
                regs->VR_H( v1, i ) = SV_H( temp, (i*2)+1 );
            }
        }
        allsat = 8;
        break;
    case 3:  // Low-order fullwords from 4 doublewords
        for ( i = 0; i < 4; i++ )
        {
            if ( SV_D( temp, i ) > 0x000000007FFFFFFFull )
            {
                regs->VR_F( v1, i ) = 0x7FFFFFFF;
                sat++;
            }
            else
            {
                regs->VR_F( v1, i ) = SV_F( temp, (i*2)+1 );
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
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_B(v1, i) = temp.h >> 8;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_H(v1, i) = temp.f >> 16;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_F(v1, i) = temp.d >> 32;
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
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_B(v1, i) = temp.h & 0xFF;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_H(v1, i) = temp.f & 0xFFFF;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_F(v1, i) = temp.d & 0xFFFFFFFF;
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
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            regs->VR_B(v1, i) = temp.sh >> 8;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            regs->VR_H(v1, i) = temp.sf >> 16;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            regs->VR_F(v1, i) = temp.sd >> 32;
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
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  // Halfword
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  // Word
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.d;
        }
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
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  // Halfword
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  // Word
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.d;
        }
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
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  // Halfword
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  // Word
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.sd;
        }
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
    int     i, j;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  // Halfword
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  // Word
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            regs->VR_D(v1, j) = temp.sd;
        }
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
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_B(v4, i);
            regs->VR_B(v1, i) = temp.h >> 8;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_H(v4, i);
            regs->VR_H(v1, i) = temp.f >> 16;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_F(v4, i);
            regs->VR_F(v1, i) = temp.d >> 32;
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
    int     i;

    VRR_D(inst, regs, v1, v2, v3, v4, m5, m6);

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK(regs);

    switch (m5)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_B(v4, i);
            regs->VR_B(v1, i) = temp.h & 0xFF;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_H(v4, i);
            regs->VR_H(v1, i) = temp.f & 0xFFFF;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_F(v4, i);
            regs->VR_F(v1, i) = temp.d & 0xFFFFFFFF;
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
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            temp.sh += (S8)regs->VR_B(v4, i);
            regs->VR_B(v1, i) = temp.sh >> 8;
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            temp.sf += (S16)regs->VR_H(v4, i);
            regs->VR_H(v1, i) = temp.sf >> 16;
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            temp.sd += (S32)regs->VR_F(v4, i);
            regs->VR_F(v1, i) = temp.sd >> 32;
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
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  // Byte
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  // Halfword
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  // Word
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.d;
        }
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
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  // Byte
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.h = regs->VR_B(v2, i);
            temp.h *= regs->VR_B(v3, i);
            temp.h += regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.h;
        }
        break;
    case 1:  // Halfword
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.f = regs->VR_H(v2, i);
            temp.f *= regs->VR_H(v3, i);
            temp.f += regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.f;
        }
        break;
    case 2:  // Word
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.d = regs->VR_F(v2, i);
            temp.d *= regs->VR_F(v3, i);
            temp.d += regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.d;
        }
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
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  // Byte
        for (i=0, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            temp.sh += (S16)regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  // Halfword
        for (i=0, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            temp.sf += (S32)regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  // Word
        for (i=0, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            temp.sd += (S64)regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.sd;
        }
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
    int     i, j;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 0:  // Byte
        for (i=1, j=0; i < 16; i+=2, j++)
        {
            temp.sh = (S8)regs->VR_B(v2, i);
            temp.sh *= (S8)regs->VR_B(v3, i);
            temp.sh += (S16)regs->VR_H(v4, j);
            regs->VR_H(v1, j) = temp.sh;
        }
        break;
    case 1:  // Halfword
        for (i=1, j=0; i < 8; i+=2, j++)
        {
            temp.sf = (S16)regs->VR_H(v2, i);
            temp.sf *= (S16)regs->VR_H(v3, i);
            temp.sf += (S32)regs->VR_F(v4, j);
            regs->VR_F(v1, j) = temp.sf;
        }
        break;
    case 2:  // Word
        for (i=1, j=0; i < 4; i+=2, j++)
        {
            temp.sd = (S32)regs->VR_F(v2, i);
            temp.sd *= (S32)regs->VR_F(v3, i);
            temp.sd += (S64)regs->VR_D(v4, j);
            regs->VR_D(v1, j) = temp.sd;
        }
        break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

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
    case 4:  // Quadword
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
    union   { U64 d[4]; } temp;
    U64     carry;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 4:  // Quadword
        carry = regs->VR_D( v4, 1 ) & 0x0000000000000001ull;
        for (i=3; i >= 0; i--)
        {
            temp.d[i] = carry;
            temp.d[i] += regs->VR_F( v3, i );
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
    U64     tempd;
    int     i;

    VRR_D( inst, regs, v1, v2, v3, v4, m5, m6 );

    /* m6 is not part of this instruction */
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m5)
    {
    case 4:  // Quadword
        tempd = regs->VR_D( v4, 1 ) & 0x0000000000000001ull;
        for (i=3; i >= 0; i--)
        {
            tempd += ~regs->VR_F( v3, i );
            tempd += regs->VR_F( v2, i );
            tempd >>= 32;
        }
        regs->VR_D( v1, 0 ) = 0;
        regs->VR_D( v1, 1 ) = tempd;
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
    case 4:  // Quadword
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

    switch (m3)
    {
    case 0:  // Byte
        for (i = 0; i < 8; i++)
            temp.h[i] = (U16) regs->VR_B(v2, i + 8);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.h[i];
        break;
    case 1:  // Halfword
        for (i = 0; i < 4; i++)
            temp.f[i] = (U32) regs->VR_H(v2, i + 4);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.f[i];
        break;
    case 2:  // Word
        for (i = 0; i < 2; i++)
            temp.d[i] = (U64) regs->VR_F(v2, i + 2);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.d[i];
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

    switch (m3)
    {
    case 0:  // Byte
        for (i = 0; i < 8; i++)
            temp.h[i] = (U16) regs->VR_B(v2, i);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.h[i];
        break;
    case 1:  // Halfword
        for (i = 0; i < 4; i++)
            temp.f[i] = (U32) regs->VR_H(v2, i);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.f[i];
        break;
    case 2:  // Word
        for (i = 0; i < 2; i++)
            temp.d[i] = (U64) regs->VR_F(v2, i);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.d[i];
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

    switch (m3)
    {
    case 0:  // Byte
        for (i = 0; i < 8; i++)
            temp.sh[i] = (S16) regs->VR_B(v2, i + 8);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.sh[i];
        break;
    case 1:  // Halfword
        for (i = 0; i < 4; i++)
            temp.sf[i] = (S32) regs->VR_H(v2, i + 4);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.sf[i];
        break;
    case 2:  // Word
        for (i = 0; i < 2; i++)
            temp.sd[i] = (S64) regs->VR_F(v2, i + 2);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.sd[i];
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

    switch (m3)
    {
    case 0:  // Byte
        for (i = 0; i < 8; i++)
            temp.sh[i] = (S16) regs->VR_B(v2, i);
        for (i = 0; i < 8; i++)
            regs->VR_H(v1, i) = temp.sh[i];
        break;
    case 1:  // Halfword
        for (i = 0; i < 4; i++)
            temp.sf[i] = (S32) regs->VR_H(v2, i);
        for (i = 0; i < 4; i++)
            regs->VR_F(v1, i) = temp.sf[i];
        break;
    case 2:  // Word
        for (i = 0; i < 2; i++)
            temp.sd[i] = (S64) regs->VR_F(v2, i);
        for (i = 0; i < 2; i++)
            regs->VR_D(v1, i) = temp.sd[i];
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
    union   { U32 f[2]; } temp;
    int     i, j, masko, selz, selo;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    /* m3, m4, m5 are not part of this instruction */
    UNREFERENCED( m3 );
    UNREFERENCED( m4 );
    UNREFERENCED( m5 );

    ZVECTOR_CHECK( regs );

    masko = selz = selo = 0;
    for (i=0; i < 4; i++)
    {
        temp.f[0] = regs->VR_F( v1, i );
        temp.f[1] = regs->VR_F( v2, i );
        for (j=0; j < 32; j++)
        {
            if (temp.f[1] & 0x80000000)
            {
                masko++;
                if (temp.f[0] & 0x80000000)
                    selo++;
                else
                    selz++;
            }
            temp.f[0] <<= 1;
            temp.f[1] <<= 1;
        }
    }

    if (masko == 0)
        regs->psw.cc = 0;    // All mask bits zero
    else if (masko == selz)
        regs->psw.cc = 0;    // Selected bits all zeros
    else if (masko == selo)
        regs->psw.cc = 3;    // Selected bits all ones
    else
        regs->psw.cc = 1;    // Selected bits a mix of zeros and ones

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

    switch (m3)
    {
    case 0:  // Byte
        if (regs->VR_B( v1, 7 ) == regs->VR_B( v2, 7 ))
            regs->psw.cc = 0;
        else if (regs->VR_B( v1, 7 ) < regs->VR_B( v2, 7 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 1:  // Halfword
        if (regs->VR_H( v1, 3 ) == regs->VR_H( v2, 3 ))
            regs->psw.cc = 0;
        else if (regs->VR_H( v1, 3 ) < regs->VR_H( v2, 3 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 2:  // Word
        if (regs->VR_F( v1, 1 ) == regs->VR_F( v2, 1 ))
            regs->psw.cc = 0;
        else if (regs->VR_F( v1, 1 ) < regs->VR_F( v2, 1 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 3:  // Doubleword
        if (regs->VR_D( v1, 0 ) == regs->VR_D( v2, 0 ))
            regs->psw.cc = 0;
        else if (regs->VR_D( v1, 0 ) < regs->VR_D( v2, 0 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
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

    switch (m3)
    {
    case 0:  // Byte
        if ((S8)regs->VR_B( v1, 7 ) == (S8)regs->VR_B( v2, 7 ))
            regs->psw.cc = 0;
        else if ((S8)regs->VR_B( v1, 7 ) < (S8)regs->VR_B( v2, 7 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 1:  // Halfword
        if ((S16)regs->VR_H( v1, 3 ) == (S16)regs->VR_H( v2, 3 ))
            regs->psw.cc = 0;
        else if ((S16)regs->VR_H( v1, 3 ) < (S16)regs->VR_H( v2, 3 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 2:  // Word
        if ((S32)regs->VR_F( v1, 1 ) == (S32)regs->VR_F( v2, 1 ))
            regs->psw.cc = 0;
        else if ((S32)regs->VR_F( v1, 1 ) < (S32)regs->VR_F( v2, 1 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
        break;
    case 3:  // Doubleword
        if ((S64)regs->VR_D( v1, 0 ) == (S64)regs->VR_D( v2, 0 ))
            regs->psw.cc = 0;
        else if ((S64)regs->VR_D( v1, 0 ) < (S64)regs->VR_D( v2, 0 ))
            regs->psw.cc = 1;
        else
            regs->psw.cc = 2;
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

    switch (m3)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = ~(S8)regs->VR_B( v2, i ) + 1;
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = ~(S16)regs->VR_H( v2, i ) + 1;
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = ~(S32)regs->VR_F( v2, i ) + 1;
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = ~(S64)regs->VR_D( v2, i ) + 1;
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

    switch (m3)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = regs->VR_B( v2, i ) & 0x7F;
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_H( v2, i ) & 0x7FFF;
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v2, i ) & 0x7FFFFFFF;
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = regs->VR_D( v2, i ) & 0x7FFFFFFFFFFFFFFFull;
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
    U64     tempd;
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
        {
            regs->VR_B( v1, i ) = (U8) ( ( (U16)regs->VR_B( v2, i ) + (U16)regs->VR_B( v3, i ) ) >> 8 );
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
        {
            regs->VR_H( v1, i ) = (U16) ( ( (U32)regs->VR_H( v2, i ) + (U32)regs->VR_H( v3, i ) ) >> 16 );
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
        {
            regs->VR_F( v1, i ) = (U32) ( ( (U64)regs->VR_F( v2, i ) + (U64)regs->VR_F( v3, i ) ) >> 32 );
        }
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++)
        {
            tempd = 0;
            tempd += regs->VR_F( v2, (i*2)+1 );
            tempd += regs->VR_F( v3, (i*2)+1 );
            tempd >>= 32;
            tempd += regs->VR_F( v2, i*2 );
            tempd += regs->VR_F( v3, i*2 );
            regs->VR_D( v1, i ) = tempd >> 32;
        }
        break;
    case 4:  // Quadword
        tempd = 0;
        for (i=3; i >= 0; i--)
        {
            tempd += regs->VR_F( v2, i );
            tempd += regs->VR_F( v3, i );
            tempd >>= 32;
        }
        regs->VR_D( v1, 0 ) = 0;
        regs->VR_D( v1, 1 ) = tempd;
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

    int i;                          /* loop index                    */
    S64 temps64;                    /* signed temp                   */

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
                    ( regs->VR_D(v2, i) & 0x8000000000000000ULL )  ==
                    ( regs->VR_D(v3, i) & 0x8000000000000000ULL )
                )
            {
                /* same sign: possible overflow */
                if  ( regs->VR_D(v2, i) & 0x8000000000000000ULL )
                {
                    /* negative signs: allow overflow, round and force back to negative */
                    temps64 = (S64) regs->VR_D(v2, i) + (S64) regs->VR_D(v3, i);
                    temps64++;
                    regs->VR_D(v1, i) = (U64) ( temps64 >> 1 ) | 0x8000000000000000ULL;
                }
                else
                {
                    /* positive signs: handle as U64 values */
                    regs->VR_D(v1, i) = (U64) ( ( (U64) regs->VR_D(v2, i) + (U64) regs->VR_D(v3, i) + 1) >> 1 );
                }
            }
            else
                regs->VR_D(v1, i) = (U64) ( ( (S64) regs->VR_D(v2, i) + (S64) regs->VR_D(v3, i) + 1) >> 1 );
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
    U64     high, low;

    VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK(regs);

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) + (S8) regs->VR_B(v3, i);
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) + (S16) regs->VR_H(v3, i);
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) + (S32) regs->VR_F(v3, i);
        }
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64) regs->VR_D(v2, i) + (S64) regs->VR_D(v3, i);
        }
        break;
    case 4:  // Quadword
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
    int     i;

    VRR_C( inst, regs, v1, v2, v3, m4, m5, m6 );

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

    ZVECTOR_CHECK( regs );

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++)
            regs->VR_B( v1, i ) = (regs->VR_B( v2, i ) < regs->VR_B( v3, i )) ? 0 : 1;
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = (regs->VR_H( v2, i ) < regs->VR_H( v3, i )) ? 0 : 1;
        break;
    case 2:  // Word
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = (regs->VR_F( v2, i ) < regs->VR_F( v3, i )) ? 0 : 1;
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++)
            regs->VR_D( v1, i ) = (regs->VR_D( v2, i ) < regs->VR_D( v3, i )) ? 0 : 1;
        break;
    case 4:  // Quadword
        regs->VR_D( v1, 1 ) = (regs->VR_D( v2, 0 ) < regs->VR_D( v3, 0 )) ? 0 : 1;
        regs->VR_D( v1, 0 ) = 0;
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
    U64     high, low;

     VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);

    /* m5, m6 are not part of this instruction */
    UNREFERENCED( m5 );
    UNREFERENCED( m6 );

     ZVECTOR_CHECK(regs);

    switch (m4)
    {
    case 0:  // Byte
        for (i=0; i < 16; i++) {
            regs->VR_B(v1, i) = (S8) regs->VR_B(v2, i) - (S8) regs->VR_B(v3, i);
        }
        break;
    case 1:  // Halfword
        for (i=0; i < 8; i++) {
            regs->VR_H(v1, i) = (S16) regs->VR_H(v2, i) - (S16) regs->VR_H(v3, i);
        }
        break;
    case 2:  // Word
        for (i=0; i < 4; i++) {
            regs->VR_F(v1, i) = (S32) regs->VR_F(v2, i) - (S32) regs->VR_F(v3, i);
        }
        break;
    case 3:  // Doubleword
        for (i=0; i < 2; i++) {
            regs->VR_D(v1, i) = (S64)regs->VR_D(v2, i) - (S64)regs->VR_D(v3, i);
        }
        break;
    case 4:  // Quadword
        high = regs->VR_D(v2, 0) - regs->VR_D(v3, 0);
        low  = regs->VR_D(v2, 1) - regs->VR_D(v3, 0);
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
    case 0:  // Byte
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
    case 1:  // Halfword
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
    case 2:  // Word
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
    case 3:  // Doubleword
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
    int     hi = 0;
    int     nothi = 0;
    int     i;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            if (regs->VR_B(v2, i) > regs->VR_B(v3, i)) {
                regs->VR_B(v1, i) = 0xff;
                hi++;
            }
            else {
                regs->VR_B(v1, i) = 0x00;
                nothi++;
            }
        }
        break;

    case 1:        /* Halfword */
        for (i=0; i < 8; i++) {
            if (regs->VR_H(v2, i) > regs->VR_H(v3, i)) {
                regs->VR_H(v1, i) = 0xffff;
                hi++;
            }
            else {
                regs->VR_H(v1, i) = 0x0000;
                nothi++;
            }
        }
        break;

    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            if (regs->VR_F(v2, i) > regs->VR_F(v3, i)) {
                regs->VR_F(v1, i) = 0xffffffff;
                hi++;
            }
            else {
                regs->VR_F(v1, i) = 0x00000000;
                nothi++;
            }
        }
        break;

    case 3:        /* Doubleword */
        for (i=0; i < 2; i++) {
            if (regs->VR_D(v2, i) > regs->VR_D(v3, i)) {
                regs->VR_D(v1, i) = 0xffffffffffffffff;
                hi++;
            }
            else {
                regs->VR_D(v1, i) = 0x0000000000000000;
                nothi++;
            }
        }
        break;

    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS) {
        if (nothi == 0)
            regs->psw.cc = 0;
        else if (hi > 0)
            regs->psw.cc = 1;
        else if (hi == 0)
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
    int     hi = 0;
    int     nothi = 0;
    int     i;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );

#define M5_CS ((m5 & 0x1) != 0) // Condition Code Set

    switch (m4)
    {
    case 0:         /* Byte */
        for (i=0; i < 16; i++) {
            if ( (S8) regs->VR_B(v2, i) > (S8) regs->VR_B(v3, i) ) {
                regs->VR_B(v1, i) = 0xff;
                hi++;
            }
            else {
                regs->VR_B(v1, i) = 0x00;
                nothi++;
            }
        }
        break;

    case 1:        /* Halfword */
        for (i=0; i < 8; i++) {
            if ( (S16) regs->VR_H(v2, i) > (S16) regs->VR_H(v3, i) ) {
                regs->VR_H(v1, i) = 0xffff;
                hi++;
            }
            else {
                regs->VR_H(v1, i) = 0x0000;
                nothi++;
            }
        }
        break;

    case 2:         /* Word */
        for (i=0; i < 4; i++) {
            if ( (S32) regs->VR_F(v2, i) > (S32) regs->VR_F(v3, i) ) {
                regs->VR_F(v1, i) = 0xffffffff;
                hi++;
            }
            else {
                regs->VR_F(v1, i) = 0x00000000;
                nothi++;
            }
        }
        break;

    case 3:        /* Doubleword */
        for (i=0; i < 2; i++) {
            if ( (S64) regs->VR_D(v2, i) > (S64) regs->VR_D(v3, i) ) {
                regs->VR_D(v1, i) = 0xffffffffffffffff;
                hi++;
            }
            else {
                regs->VR_D(v1, i) = 0x0000000000000000;
                nothi++;
            }
        }
        break;

    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    if (M5_CS) {
        if (nothi == 0)
            regs->psw.cc = 0;
        else if (hi > 0)
            regs->psw.cc = 1;
        else if (hi == 0)
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
