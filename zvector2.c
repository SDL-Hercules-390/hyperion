/* ZVECTOR2.C   (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              z/Arch Vector Operations                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/* ============================================= */
/* TEMPORARY while zvector2.c is being developed */
#if defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
/* ============================================= */

#include "hstdinc.h"

#define _ZVECTOR2_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined(FEATURE_129_ZVECTOR_FACILITY)

/* Debug end of vector instruction execution                     */
#undef  ZVECTOR_END
#define ZVECTOR_END(_regs) \
        if (0 && inst[5] != (U8) 0x3E && inst[5] != (U8) 0x36) \
            ARCH_DEP(display_inst) (_regs, inst);

/*-------------------------------------------------------------------*/
/* Use Intrinsics                                                    */
/* - for MSVS                                                        */
/* - for Clang version less than 12 or GCC version less than 8 when  */
/*   when SSE 4.2 intrinsics are available                           */
/*-------------------------------------------------------------------*/
//Programmers note:
//  intrinsics are defined to X64.

//  future option for aarch64:
//  sse2neon.h adds aarch64 Neon impelementations of X64 intrisics
//  to allow a single intrinsic implementation to be used.
//  https://github.com/DLTcollab/sse2neon
//

#undef __V128_SSE__

/* MSVC on X64: intrinsics are available and should be used for optimization */
#if   ( defined (_MSC_VER ) || defined( _MSVC_ ) ) && defined( _M_X64 )

    #define __V128_SSE__ 1

/*
   GCC/Clang on X64: intrinsics are included if SSE2 is available
.  Use intrinsics for optimization, if SSE 4.2 is available (all SSE intrinsics) and
       Clang version less than 12 or
       GCC version less than 8
   otherwise use compiler -O3 optimization. -O3 optimization includes
   "Vectorization of loops" https://gcc.gnu.org/onlinedocs/gnat_ugn/Vectorization-of-loops.html.
   which uses native SIMD instructions, for example on ARM AArch64 processors, Intel X64 processors, ...
*/

#elif defined( __x86_64__ ) &&  defined( __SSE4_2__ ) &&           \
      ( (defined( __clang_major__ ) && __clang_major__< 12  ) || \
        (defined( __GNUC__ ) && __GNUC__< 8  )                   \
      )
    #define __V128_SSE__ 1

#endif

/* compile debug message: are we using intrinsics? */
#if 1
    #if defined(__V128_SSE__)
        #pragma message("__V128_SSE__ is defined.  Using intrinsics." )
    #else
        #pragma message("No intrinsics are included for optimization; only compiler optimization")
    #endif
#endif

/*-------------------------------------------------------------------*/
/* 128 bit types                                                     */
/*-------------------------------------------------------------------*/
#if defined( HAVE___INT128_T )
    typedef unsigned __int128 U128;
    typedef          __int128 S128;
#else
    typedef QW U128;
    typedef QW S128;

#endif

//Note: V128 is a aligned 128 bit Vector type

typedef union {
        ALIGN_128
        QW   Q;
        U128 u_128;
        U64  u_64[2];
        U32  u_32[4];
        U16  u_16[8];
        U8   u_8[16];

        S128 s_128;
        S64  s_64[2];
        S32  s_32[4];
        S16  s_16[8];
        S8   s_8[16];

        float  fs[4];
        double fd[2];

#if defined(__V128_SSE__)
        __m128i V; 			// intrinsic type vector
#endif

}  V128  ;


/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define MAX_DECIMAL_LENGTH      16
#define MAX_DECIMAL_DIGITS      (((MAX_DECIMAL_LENGTH)*2)-1)
#define MAX_ZONED_LENGTH        32
#define VR_PACKED_SIGN          15


/* Decimal handling helpers */
#define IS_VALID_SIGN(s)    ( ((s) & 0x0F) > 9 )
#define IS_VALID_DECIMAL(s) ( ((s) & 0x0F) < 10 )
#define IS_PLUS_SIGN(s)     ( (((s) & 0x0F) == 0x0A) || (((s) & 0x0F) == 0x0C) || (((s) & 0x0F) == 0x0E) || (((s) & 0x0F) == 0x0F) )
#define IS_MINUS_SIGN(s)    ( (((s) & 0x0F) == 0x0B) || (((s) & 0x0F) == 0x0D) )
#define PREFERRED_PLUS      0x0C
#define PREFERRED_MINUS     0x0D
#define PREFERRED_ZONE      0x0F
#define PACKED_HIGH(p)      ( ((p) & 0xF0) >> 4)
#define PACKED_LOW(p)       ( ((p) & 0x0F)     )
#define PACKED_SIGN(p)      ( ((p) & 0x0F)     )
#define ZONED_DECIMAL(p)    ( ((p) & 0x0F)     )
#define ZONED_SIGN(p)       ( ((p) & 0xF0) >> 4)


/* Vector Register handling helpers */
#define SET_VR_ZERO(p)       SetZero_128( &(regs->VR_Q( (p) )) )
#define GET_VR_SIGN(p)       (PACKED_SIGN( regs->VR_B( (p), VR_PACKED_SIGN) ))

/*===================================================================*/
/* Achitecture Independent Routines                                  */
/*===================================================================*/

#if !defined(_ZVECTOR2_ARCH_INDEPENDENT_)
#define _ZVECTOR2_ARCH_INDEPENDENT_

/* SetZero_128 */
/*-------------------------------------------------------------------*/
/* Set 16 bytes (128 bits) to zero                                   */
/*                                                                   */
/* Input:                                                            */
/*      addr    address of 16-byte asrea to zero                     */
/*                                                                   */
/* version depends on whether intrinsics are being used              */
/*-------------------------------------------------------------------*/
#if defined(__V128_SSE__)

static inline void SetZero_128 (void* addr)
{
    __m128i v = _mm_setzero_si128();
    memcpy( addr, &v, sizeof( __m128i ));
}

#else

static inline void SetZero_128 (void* addr)
{
    memset( addr, 0, sizeof(U128) );
}

#endif

/*-------------------------------------------------------------------*/
/* A valid signed packed VR is converted to a decimal byte string    */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to convert                           */
/*                                                                   */
/* Output:                                                           */
/*      result  Points to a 31-byte area into which the decimal      */
/*              digits are loaded.  One decimal digit is loaded      */
/*              into the low-order 4 bits of each byte.              */
/*      count   Points to an integer to receive the number of        */
/*              digits in the result excluding leading zeroes.       */
/*              This field is set to zero if the result is all zero. */
/*      sign    Points to an integer which will be set to -1 if a    */
/*              negative sign was loaded from the operand, or +1 if  */
/*              a positive sign was loaded from the operand.         */
/*                                                                   */
/* Modifiied version of decimal.c 'load_decimal'                     */
/*-------------------------------------------------------------------*/
static inline void  vr_packed_2_decimal ( REGS* regs, int v1, BYTE *result, int *count, int * sign )
{
    int     h;                              /* Hexadecimal digit         */
    int     i, j;                           /* Array subscripts          */
    int     n;                              /* Significant digit counter */

    /* Unpack digits into result */
    for (i=0, j=0, n=0; i < MAX_DECIMAL_DIGITS; i++)
    {
        /* Load source digit */
        if (i & 1)
            h = regs->VR_B( v1, j++) & 0x0F;
        else
            h = regs->VR_B( v1, j) >> 4;

        /* Count significant digits */
        if (n > 0 || h != 0)
            n++;

        /* Store decimal digit in result */
        result[i] = h;

    } /* end for */

    /* Set number of significant digits */
    *count = n;

    /* Set sign of operand */
    *sign = (IS_PLUS_SIGN( regs->VR_B( v1, MAX_DECIMAL_LENGTH-1 ) ) ) ? 1 : -1;
}

/*-------------------------------------------------------------------*/
/* Load decimal byte string into signed packed decimal VR            */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to load                              */
/*      dec     A 31-byte area containing the decimal digits to be   */
/*              stored.  Each byte contains one decimal digit in     */
/*              the low-order 4 bits of the byte.                    */
/*      sign    -1 if a negative sign is to be stored, or +1 if a    */
/*              positive sign is to be stored.                       */
/*      forced  force the VR to be positive with b'1111' sign        */
/*      rdc     Result Digits Count: count of the right most digits  */
/*              to be loaded. Remaining leftmost diigits are set to  */
/*              zero                                                 */
/*                                                                   */
/* Output:                                                           */
/*              updated z/vector register v1                         */
/*                                                                   */
/* Modifiied version of decimal.c 'store_decimal'                    */
/*-------------------------------------------------------------------*/
static inline void  decimal_2_vr_packed ( REGS* regs, int v1, BYTE *dec, int  sign, bool forced, int rdc )
{
    int     i, j;                           /* Array subscripts          */
    BYTE    ps;                             /* packed sign               */
    int     leading0;                       /* leading zeros             */

    if (rdc == 0 )
        leading0 =  0;
    else
        leading0 =  MAX_DECIMAL_DIGITS - rdc;

    /* Pack digits into packed decimal work area */
    for (i=0, j=0; i < MAX_DECIMAL_DIGITS; i++)
    {
        if (i & 1)
            regs->VR_B( v1, j++) |= (leading0-- > 0) ? 0 : dec[i];
        else
            regs->VR_B( v1, j) = (leading0-- > 0) ? 0 : dec[i] << 4;
    } /* end for */

    /* Pack the sign into low-order digit */
    ps = ( forced ) ? PREFERRED_ZONE : ( ( sign < 0 ) ? PREFERRED_MINUS : PREFERRED_PLUS);
    regs->VR_B( v1, MAX_DECIMAL_LENGTH-1) |= ps;
}

/*-------------------------------------------------------------------*/
/* Check for decimal overflow given for a Result Digits Count        */
/*                                                                   */
/* Input:                                                            */
/*      dec     A 31-byte area containing the decimal digits to be   */
/*              checked.  Each byte contains one decimal digit in    */
/*              the low-order 4 bits of the byte.                    */
/*      rdc     Result Digits Count: count of the right most digits. */
/*              The leftmost digits (31-rdc) are checked for non-zero*/
/*              overflow digits.                                     */
/*                                                                   */
/* Returns:                                                          */
/*              true:  overflow: non-zero digits occur left of       */
/*                     Result Digits Count                           */
/*              false: no overflow; all left digits are zero         */
/*-------------------------------------------------------------------*/
static inline bool decimal_overflow ( BYTE *dec, int rdc )
{
    int     i;                              /* Array subscript           */
    bool    overflow = false;               /* overflow result           */
    int     leading0;                       /* leading zeros             */

    leading0 =  MAX_DECIMAL_DIGITS - rdc;

    /* check for non-zero for leading0 digits */
    for (i=0; i < leading0; i++)
    {
        if ( dec[i] != 0 )
        {
            overflow = true;
            break;
        }
    } /* end for */

    return overflow;
}

/*-------------------------------------------------------------------*/
/* Check a signed packed decimal VR for valid digits                 */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*                                                                   */
/* Returns:                                                          */
/*              true:  all 31 packed digits are valid                */
/*              false: at least one invalid digit in the VR          */
/*-------------------------------------------------------------------*/
static inline bool  vr_packed_valid_digits ( REGS* regs, int v1 )
{
    int     i, j;                           /* Array subscript           */
    bool    valid = true;                   /* valid result              */

    /* check for non-zero for leading0 digits */
    /* Pack digits into packed decimal work area */
    for (i=0, j=0; i < MAX_DECIMAL_DIGITS && valid; i++)
    {
        if (i & 1)
            valid = PACKED_LOW ( regs->VR_B( v1, j++ ) ) < 10;
        else
            valid = PACKED_HIGH ( regs->VR_B( v1, j ) ) < 10;
    } /* end for */

    return valid;
}

/*-------------------------------------------------------------------*/
/* Check a signed packed decimal VR for a valid signb                */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*                                                                   */
/* Returns:                                                          */
/*              true:  the VR sign is valid                          */
/*              false: the VR sign is invalid                        */
/*-------------------------------------------------------------------*/
static inline bool  vr_packed_valid_sign ( REGS* regs, int v1 )
{
    return PACKED_SIGN ( regs->VR_B( v1, VR_PACKED_SIGN ) ) > 9;
}

/*-------------------------------------------------------------------*/
/* Check a VR for a valid signed packed decimal                      */
/*                                                                   */
/* Input:                                                            */
/*      regs    CPU register context for VR access                   */
/*      v1      vector register to check                             */
/*                                                                   */
/* Returns:                                                          */
/*              true:  the VR is a valid signed packed decimal       */
/*              false: the VR is invalid (sign or at least on digit) */
/*-------------------------------------------------------------------*/
static inline bool  vr_packed_valid ( REGS* regs, int v1 )
{
    return vr_packed_valid_digits( regs, v1 ) &&
           vr_packed_valid_sign  ( regs, v1 );
}


#endif /*!defined(_ZVECTOR2_ARCH_INDEPENDENT_)*/

/*===================================================================*/
/* Achitecture Dependent Routines / instructions                     */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* E601 VLEBRH   - VECTOR LOAD BYTE REVERSED ELEMENT (16)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_element_16 )
{
    int     v1, m3, x2, b2;
    VADR    effective_addr2;

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK (regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 7)         /* M3 > 7 => Specficitcation excp */
        ARCH_DEP(program_interrupt) ( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_H( v1, m3 ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2, b2, regs ) );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E602 VLEBRG   - VECTOR LOAD BYTE REVERSED ELEMENT (64)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_element_64 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 1)                    /* M3 > 1 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_D( v1, m3 ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ) );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E603 VLEBRF   - VECTOR LOAD BYTE REVERSED ELEMENT (32)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_element_32 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 3)                    /* M3 > 3 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    regs->VR_F( v1, m3 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E604 VLLEBRZ - VECTOR LOAD BYTE REVERSED ELEMENT AND ZERO   [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_and_zero )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* M3= 0, 4, 5, 7-15 => Specficitcation excp */
    if (m3 == 0 || m3 == 4 || m3 == 5 || m3 >=7 )
        ARCH_DEP(program_interrupt) ( regs, PGM_SPECIFICATION_EXCEPTION );

    SET_VR_ZERO( v1 );

    switch (m3)
    {
    case 1: regs->VR_H( v1, 3 ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2, b2, regs ) ); break;
    case 2: regs->VR_F( v1, 1 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) ); break;
    case 3: regs->VR_D( v1, 0 ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ) ); break;
    case 6: regs->VR_F( v1, 0 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) ); break;
    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*----------------------------------------------------------------------*/
/* E605 VLBRREP - VECTOR LOAD BYTE REVERSED ELEMENT AND REPLICATE [VRX] */
/*----------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_and_replicate )
{
    int     v1, m3, x2, b2;                /* Instruction parts         */
    VADR    effective_addr2;               /* Effective address         */
    int i;                                 /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:
        regs->VR_H( v1, 0 ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2, b2, regs ) );
        for (i=1; i < 8; i++)
            regs->VR_H( v1, i ) = regs->VR_H( v1, 0 );
        break;

    case 2:
        regs->VR_F( v1, 0 ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2, b2, regs ) );
        for (i=1; i < 4; i++)
            regs->VR_F( v1, i ) = regs->VR_F( v1, 0 );
        break;

    case 3:
        regs->VR_D( v1, 0 ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2, b2, regs ) );
        regs->VR_D( v1, 1 ) = regs->VR_D( v1, 0 );
        break;

    default:
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E606 VLBR  - VECTOR LOAD BYTE REVERSED ELEMENTS             [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_byte_reversed_elements )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            regs->VR_H( v1, i ) = bswap_16( ARCH_DEP( vfetch2 )( effective_addr2 + i*2, b2, regs ) );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            regs->VR_F( v1, i ) = bswap_32( ARCH_DEP( vfetch4 )( effective_addr2 + i *4, b2, regs ) );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            regs->VR_F( v1, i ) = bswap_64( ARCH_DEP( vfetch8 )( effective_addr2 + i*8, b2, regs ) );
        break;

    case 4:     /* quadword */
        regs->VR_Q( v1 ) = bswap_128( ARCH_DEP( vfetch16 )( effective_addr2, b2, regs ) );
        break;

    default:    /* M3= 0, 5-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E607 VLER   - VECTOR LOAD ELEMENTS REVERSED                 [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_elements_reversed )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* M3= 0, 5-15 => Specficitcation excp */
    if (m3 == 0 || m3 >=5 )
        ARCH_DEP(program_interrupt) ( regs, PGM_SPECIFICATION_EXCEPTION );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            regs->VR_H( v1, (7 - i) ) = ARCH_DEP( vfetch2 )( effective_addr2 + i*2, b2, regs );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            regs->VR_F( v1, (3 - i) ) = ARCH_DEP( vfetch4 )( effective_addr2 + i *4, b2, regs );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            regs->VR_F( v1, (1 - i) ) = ARCH_DEP( vfetch8 )( effective_addr2 + i*8, b2, regs );
        break;

    default:    /* M3= 0, 4-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E609 VSTEBRH - VECTOR STORE BYTE REVERSED ELEMENT (16)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_element_16 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX(inst, regs, v1, x2, b2, effective_addr2, m3);

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 7)                    /* M3 > 7 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore2 )( bswap_16( regs->VR_H( v1, m3 ) ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60A VSTEBRG - VECTOR STORE BYTE REVERSED ELEMENT (64)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_element_64 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 1)                    /* M3 > 1 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore8 )( bswap_64( regs->VR_D( v1, m3 ) ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60B VSTEBRF - VECTOR STORE BYTE REVERSED ELEMENT (32)      [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_element_32 )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    if (m3 > 3)                    /* M3 > 3 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    ARCH_DEP( vstore4 )( bswap_32( regs->VR_F( v1, m3 ) ), effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60E VSTBR - VECTOR STORE BYTE REVERSED ELEMENTS            [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_byte_reversed_elements )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            ARCH_DEP( vstore2 )( bswap_16( regs->VR_H( v1, i ) ), effective_addr2 + i*2, b2, regs );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            ARCH_DEP( vstore4 )( bswap_32( regs->VR_F( v1, i ) ), effective_addr2 + i*4, b2, regs );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            ARCH_DEP( vstore8 )( bswap_64( regs->VR_D( v1, i ) ), effective_addr2 + i*8, b2, regs );
        break;

    case 4:     /* quadword */
        ARCH_DEP( vstore16 )( bswap_128( regs->VR_Q( v1 ) ), effective_addr2, b2, regs );
        break;

    default:    /* M3= 0, 5-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E60F VSTER  - VECTOR STORE ELEMENTS REVERSED                [VRX] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_reversed_elements )
{
    int     v1, m3, x2, b2;             /* Instruction parts         */
    VADR    effective_addr2;            /* Effective address         */
    int i;                              /* Loop variable             */

    VRX( inst, regs, v1, x2, b2, effective_addr2, m3 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    switch (m3)
    {
    case 1:     /* halfword */
        for (i=0; i < 8; i++)
            ARCH_DEP( vstore2 )( regs->VR_H( v1, i ) , effective_addr2 + (14 - i*2), b2, regs );
        break;

    case 2:     /* fullword */
        for (i=0; i < 4; i++)
            ARCH_DEP( vstore4 )( regs->VR_F( v1, i ) , effective_addr2 + (12 - i*4), b2, regs );
        break;

    case 3:     /* doubleword */
        for (i=0; i < 2; i++)
            ARCH_DEP( vstore8 )( regs->VR_D( v1, i ), effective_addr2 + (8 - i*8), b2, regs );
        break;

    default:    /* M3= 0, 4-15 => Specficitcation excp */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
        break;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E634 VPKZ  - VECTOR PACK ZONED                              [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_packed_zoned )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      zoned[MAX_ZONED_LENGTH];   /* local zoned decimal        */
    U8      digit_high;                /* high digit of packed byte  */
    U8      digit_low;                 /* low digit of packed byte   */
    bool    doing_low;                 /* are we processing low digit*/
    int     packedix = 0;              /* current packed byte index  */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    l2 = i3 & 0xE0;           /* i3 reserved bits 0-2 must be zero    */
    if ( l2 != 0 )             /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x1F;          /* Operand 2 Length Code (L2): Bits 3-7 */
    if ( l2 > 30 )             /* L2 > 30 => Specficitcation excp      */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* get local copy; note: l2 is zoned length -1 */
    ARCH_DEP( vfetchc )( zoned, l2,  effective_addr2, b2, regs );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    /* handle last zoned field */
    digit_low   = ZONED_SIGN(  zoned[ l2 ] );   /* sign */
    digit_high  = ZONED_DECIMAL( zoned[ l2 ] );   /* lowest digit */
    regs->VR_B( v1, 15 ) = ( digit_high << 4 ) | digit_low;
    //LOGMSG("VECTOR PACK ZONED: V1.B15=%hhX \n", regs->VR_B( v1, 15));

    packedix = 14;
    doing_low = true;
    for ( i=l2 -1; i >=0;  i -= 2,  packedix--)
    {
        digit_low  = ZONED_DECIMAL( zoned[ i ] );
        digit_high = (i > 0 ) ? ZONED_DECIMAL( zoned[ i-1 ] ) << 4 : 0;  /* have two digits? */
        regs->VR_B( v1, packedix) = digit_high | digit_low;
        //LOGMSG("VECTOR PACK ZONED:  %d low:'%hhX' high: '%hhX' result: V%d.B%d=%hhX \n", i, zoned [i], zoned[i-1], v1, packedix, regs->VR_B( v1, packedix));
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E635 VLRL   - VECTOR LOAD RIGHTMOST WITH LENGTH             [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_rightmost_with_length )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    l2 = i3 & 0xF0;           /* i3 reserved bits 0-3 must be zero    */
    if ( l2 != 0 )            /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x0F;          /* Operand 2 Length Code (L2): Bits 4-7 */

    /* get local copy; note: l2 is length -1 */
    ARCH_DEP( vfetchc )( stor, l2,  effective_addr2, b2, regs );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    for ( i=0; i <= l2; i++ )
    {
        regs->VR_B( v1, (15 - l2) + i) = stor[i];
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E637 VLRLR  - VECTOR LOAD RIGHTMOST WITH LENGTH (reg)     [VRS-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_rightmost_with_length_reg )
{
    int     v1, b2, r3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */
    U32     reg32;                     /* r3: bits 32-63             */

    VRS_D( inst, regs, r3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    reg32 =  regs->GR_L( r3 );
    l2 = ( reg32 > 15 ) ? 15 : (int) reg32 ;
    //LOGMSG("VECTOR LOAD RIGHTMOST WITH LENGTH (reg) : r3=%d, reg32=%d, l2=%d \n", r3, reg32, l2);

    /* get local copy; note: l2 is length -1 */
    ARCH_DEP( vfetchc )( stor, l2, effective_addr2, b2, regs );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    for ( i=0; i <= l2; i++ )
    {
         regs->VR_B( v1, (15 - l2) + i) = stor[i];
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E63C VUPKZ  - VECTOR UNPACK ZONED                           [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_zoned )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      zoned[MAX_ZONED_LENGTH];   /* local zoned decimal        */
    U8      sign;                      /* sign of zoned decimal      */
    U8      digit_low;                 /* low digit of packed byte   */
    int     packedix = 0;              /* current packed byte index  */
    U8      zone = 0xF0;               /* zone field '1111'          */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    l2 = i3 & 0xE0;           /* i3 reserved bits 0-2 must be zero    */
    if ( l2 != 0 )             /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x1F;          /* Operand 2 Length Code (L2): Bits 3-7 */
    if ( l2 > 30 )             /* L2 > 30 => Specficitcation excp      */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* handle last zoned field - sign  & digit */
    sign       = PACKED_SIGN( regs->VR_B( v1, VR_PACKED_SIGN) );    /* sign */
    digit_low  = PACKED_HIGH( regs->VR_B( v1, VR_PACKED_SIGN) );    /* lowest digit */
    zoned[l2]  = ( sign << 4 ) | digit_low;
    //LOGMSG("VECTOR UNPACK ZONED: V1.B15=%hhX zoned:=%hhX \n", regs->VR_B( v1, 15), zoned[l2]);

    packedix = 14;
    for ( i=l2 -1; i >=0 ; i-=2, packedix-- )
    {
        zoned[i] = PACKED_LOW( regs->VR_B( v1, packedix) ) | zone;
        //LOGMSG("VECTOR UNPACK ZONED low:  %d '%hhX' source: V%d.B%d=%hhX \n", i, zoned [i], v1, packedix, regs->VR_B( v1, packedix));
        if( i > 0 )
        {
            zoned[i-1] = PACKED_HIGH( regs->VR_B( v1, packedix) ) | zone;
            //LOGMSG("VECTOR UNPACK ZONED high: %d '%hhX' source: V%d.B%d=%hhX \n", i, zoned [i-1], v1, packedix, regs->VR_B( v1, packedix));
        }
    }

    /* save local copy; note: l2 is zoned length -1 */
    ARCH_DEP( vstorec )( zoned, l2, effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E63D VSTRL  - VECTOR STORE RIGHTMOST WITH LENGTH            [VSI] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_rightmost_with_length )
{
    int     v1, b2, i3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */

    VSI( inst, regs, i3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    l2 = i3 & 0xF0;           /* i3 reserved bits 0-3 must be zero    */
    if ( l2 != 0 )            /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    l2 = i3 & 0x0F;          /* Operand 2 Length Code (L2): Bits 4-7 */

    for ( i=0; i <= l2; i++ )
    {
        stor[i] = regs->VR_B( v1, (15 - l2) + i);
    }

    /* store local copy; note: l2 is length -1 */
    ARCH_DEP( vstorec )( stor, l2,  effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E63F VSTRLR - VECTOR STORE RIGHTMOST WITH LENGTH (reg)    [VRS-d] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_store_rightmost_with_length_reg )
{
    int     v1, b2, r3;                /* Instruction parts          */
    VADR    effective_addr2;           /* Effective address          */
    int     l2;                        /* length code (L2) control   */
    int     i;                         /* Loop variable              */
    U8      stor[16];                  /* local stor                 */
    U32     reg32;                     /* r3: bits 32-63             */

    VRS_D( inst, regs, r3, b2, effective_addr2, v1 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    reg32 =  regs->GR_L( r3 );
    l2 = ( reg32 > 15 ) ? 15 : (int) reg32 ;
    //LOGMSG("VECTOR STORE RIGHTMOST WITH LENGTH (reg) : r3=%d, reg32=%d, l2=%d \n", r3, reg32, l2);

    for ( i=0; i <= l2; i++ )
    {
        stor[i] = regs->VR_B( v1, (15 - l2) + i);
    }

    /* store local copy; note: l2 is length -1 */
    ARCH_DEP( vstorec )( stor, l2,  effective_addr2, b2, regs );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E649 VLIP   - VECTOR LOAD IMMEDIATE DECIMAL               [VRI-h] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_load_immediate_decimal )
{
    int     v1, i3;              /* Instruction parts                */
    U16     i2;                  /* Instruction - Decimal Immediate  */
    int     sc;                  /* Sign Control (SC): bit 0         */
    int     shift;               /* Shift Amount (SHAMT): Bits 1-3   */
    int     i;                   /* Loop variable                    */
    int     idx;                 /* vector byte index to place digits*/
    U16     temp;                /* temporary                        */
    union imm32
    {
        U32 digits;             /* shifted digits for byte alignment */
        U8  db[4];
    } imm;                      /* immediate */

    VRI_H( inst, regs, v1, i2, i3 );

    ZVECTOR_CHECK( regs );

    /* check i2 for valid decimal digits */
    temp = i2;
    for ( i = 0; i < 3; i++)
    {
        if ( (temp & 0x0F) > 9)
        {
            regs->dxc = DXC_DECIMAL;
            ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
        }
        temp >>= 4;
    }

    /* get sign control and shift */
    sc    = ( i3 & 0x08 ) >> 3;
    shift = ( i3 & 0x07 );

    /* set v1 to zero */
    SET_VR_ZERO( v1 );

    /* set sign */
    if (sc == 0)
        regs->VR_B( v1, VR_PACKED_SIGN) = PREFERRED_PLUS;   /* positive with a sign code of 1100 */
    else
        regs->VR_B( v1, VR_PACKED_SIGN) = PREFERRED_MINUS;   /* negative with a sign code of 1101 */

    //LOGMSG("VECTOR LOAD IMMEDIATE DECIMAL: i2 %X\n",i2 );

    imm.digits = i2;
    idx = VR_PACKED_SIGN - ( (shift / 2) + 2);

    if ( shift & 0x01 )         /* is shift is odd */
    {
        /* odd shift; ignore sign */
        imm.digits = (U32) CSWAP32( imm.digits );
        regs->VR_B( v1, idx + 0 ) |=  imm.db[2];
        regs->VR_B( v1, idx + 1 ) |=  imm.db[3];
        //LOGMSG("VECTOR LOAD IMMEDIATE DECIMAL: Odd shift: %d  digits %X, idx=%d, db[0]= %x, db[1]= %x, db[2]= %x, db[3]= %x\n",shift, imm.digits, idx, imm.db[0], imm.db[1], imm.db[2], imm.db[3] );
    }
    else
    {
        /* even shift; allow for sign */
        imm.digits <<= 4;
        imm.digits = (U32) CSWAP32( imm.digits );
        regs->VR_B( v1, idx + 0 ) |=  imm.db[1];
        regs->VR_B( v1, idx + 1 ) |=  imm.db[2];
        regs->VR_B( v1, idx + 2 ) |=  imm.db[3];
        //LOGMSG("VECTOR LOAD IMMEDIATE DECIMAL: Even shift: %d  digits %X, idx=%d, db[0]= %x, db[1]= %x, db[2]= %x, db[3]= %x\n",shift, imm.digits, idx, imm.db[0], imm.db[1], imm.db[2], imm.db[3] );
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E650 VCVB   - VECTOR CONVERT TO BINARY (32)               [VRR-i] */
/*-------------------------------------------------------------------*/
/* FIX ME: z/Architecture Principles of Operation, SA22-7832-13      */
/*    page 25-8:                                                     */
/*    For any decimal number outside these ranges (overflow case),   */
/*    the 32 or 64 rightmost bits of the binary result are placed    */
/*    in the register.                                               */
/*                                                                   */
/* ==>Then all 31 digits need to be converted to binary in order     */
/*    to have valid rightmost bits on an overflow which requires     */
/*    128-bit arithmetic.  NOT IMPLEMENTED YET                       */
/*                                                                   */
/*-------------------------------------------------------------------*/

DEF_INST( vector_convert_to_binary_32 )
{
    int     r1, v2, m3, m4;      /* Instruction parts                */
    bool    p2;                  /* Force Operand 2 Positive (P2)    */
    bool    lb;                  /* Logical Binary (LB)              */
    bool    cs;                  /* Condition Code Set (CS)          */
    bool    iom;                 /* Instruction-Overflow Mask (IOM)  */
    bool    possign;             /* result has positive sign         */
    U8      sign;                /* decimal sign                     */
    U64     result;              /* converted binary                 */
    U32     reg_result;          /* converted binary register value  */
    int     i;                   /* Loop variable                    */
    U8      digit;               /* digit of packed byte             */
    U64     scale;               /* current digit scale              */
    int     packedix;            /* packed byte index                */
    bool    overflow;            /* did an overfor occur             */

    VRR_I( inst, regs, r1, v2, m3, m4 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    p2 = (m3 & 0x08) ? true : false;
    lb = (m3 & 0x02) ? true : false;
    cs = (m3 & 0x01) ? true : false;

    /* m4 parts */
    iom = (m4 & 0x08) ? true : false;

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    /* vector register has valid decimals */
    if ( !vr_packed_valid_digits( regs, v2 ) )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* determine result sign */
    possign = p2;
    if ( !p2 )
    {
        /* not forced positive (p2=0) */
        if ( lb )
        {
            /* unsigned (lb=1) */
            possign = true;
        }
        else
        {
            /* signed (lb=0) */
            sign = GET_VR_SIGN ( v2 );
            if ( !IS_VALID_SIGN(sign) )
            {
                regs->dxc = DXC_DECIMAL;
                ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
            }
            possign = IS_PLUS_SIGN( sign );
        }
    }

    /* convert and check for valid digits */
    packedix = VR_PACKED_SIGN;
    scale = 1;
    result = 0;

    /* programmers note: max unsigned 32-bit value is 4,294,967,295 ie. 10 digits */
    /* so convert vr bytes 9-15*/
    for ( i=MAX_DECIMAL_DIGITS-1; packedix >= 9 ; i--)
    {
        if (i & 1)
            digit = PACKED_LOW ( regs->VR_B( v2, packedix ) ) ;
        else
            digit = PACKED_HIGH ( regs->VR_B( v2, packedix-- ) ) ;

        result += scale * digit;
        scale  *= 10;
        //LOGMSG("VECTOR CONVERT TO BINARY (32): i=%d, digit=%d, result=%ld, scale=%ld \n", i, digit, result, scale);
    }


    /* did overflow happen? */
    overflow = false;

    /* check vr bytes 0-8 for "overflow" digits (two at a time) are not zero */
    for (i=0; i <= 8; i++ )
    {   if (regs->VR_B( v2, i ) != 0 )
        {
            overflow = true;
            break;
        }
    }

    /* check result for overflow */
    if (!overflow)
    {
        if (lb)
            overflow = ( result > (U64) UINT_MAX ) ? true : false;
        else
        {
            if (possign)
                overflow = ( result > (U64) INT_MAX ) ? true : false;
            else
                overflow = ( -result < (U64) INT_MIN ) ? true : false;
        }
    }

    /* CC and 32 bit results depend on overflow */
    if (overflow)
    {
        reg_result = (U32) (result & 0xFFFFFFFF);
        if (cs) regs->psw.cc = 3;
    }
    else
    {
        reg_result = (U32) ( ( (possign) ? result : -result) & 0xFFFFFFFF);
        if (cs) regs->psw.cc = 0;
    }
    regs->GR_L(r1) = reg_result;

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && FOMASK(&regs->psw))
    {
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E651 VCLZDP - VECTOR COUNT LEADING ZERO DIGITS            [VRR-k] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_count_leading_zero_digits )
{
    int     v1, v2, m3;        /* Instruction parts                  */
    bool    nv;                /* No Validation (NV): m3 bit 1       */
    bool    nz;                /* Negative Zero (NZ): m3 bit 2       */
    bool    cs;                /* Condition Code Set (CS) : m3 bit 3 */
    bool    found_nz = false;  /* found a non zero digit             */
    U8      leading_zeros = 0; /* leading zero count                 */
    bool    possign;           /* has positive sign                  */
    bool    valid = false;     /* valid packed decimal               */
    int     i;                 /* Loop variable                      */
    U8      digit;             /* digit of packed byte               */
    U8      sign;              /* sign  of packed decimal            */
    bool    doing_low;         /* are we processing low digit        */
    int     packedix;          /* packed byte index                  */
    BYTE    cc;                /* condition code                     */

    VRR_K( inst, regs, v1, v2, m3 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    nv = (m3 & 0x04) ? true : false;
    nz = (m3 & 0x02) ? true : false;
    cs = (m3 & 0x01) ? true : false;

    /* check all decimal digits and sign; count leading zeros */
    /* sign:  positive: either b'1100' preferred or  b'1111'; negative: b'1101'*/
    valid = true;
    possign = true;

    sign = regs->VR_B( v2, VR_PACKED_SIGN) & 0x0F;
    if      ( IS_PLUS_SIGN( sign )  )  possign = true;
    else if ( IS_MINUS_SIGN( sign ) )  possign = false;
    else    valid = false;

    packedix = 0;
    doing_low = false;

    for (i=0; i < MAX_DECIMAL_DIGITS; i++)
    {
        digit = regs->VR_B( v2, packedix);
        digit = (doing_low) ? (digit & 0x0F) : (digit >> 4);
        if (doing_low) packedix++;
        doing_low = !doing_low;

        if (digit > 9)
        {
            valid = false;
            found_nz = true;
        }
        else if (digit == 0)
        {
            if (!found_nz) leading_zeros++;
        }
        else
            found_nz = true;
    }

    /* invalid exception? */
    if (!nv && !valid)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* update V1 */
    SET_VR_ZERO( v1 );
    regs->VR_B( v1, 7) = leading_zeros;

    /* determine condition code */
    if (cs)
    {
        if ( valid )
        {
            if ( found_nz )
                cc = (possign) ? 2 : 1;
            else
                cc = (nz && !possign) ? 1 : 0;
        }
        else
            cc= 3;

        regs->psw.cc = cc;
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E652 VCVBG  - VECTOR CONVERT TO BINARY (64)               [VRR-i] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_to_binary_64 )
{
    int     r1, v2, m3, m4;

    VRR_I( inst, regs, r1, v2, m3, m4 );

    ZVECTOR_CHECK( regs );

    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E654 VUPKZH - VECTOR UNPACK ZONED HIGH                    [VRR-k] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_zoned_high )
{
    int     v1, v2, m3;        /* Instruction parts                  */
    bool    nsv;               /* No Sign Validation (NSV): bit 0    */
    bool    nv;                /* No Validation (NV): bit 1          */
    int     i;                 /* loop variable                      */
    int     indx;              /* index variable                     */
    U8      temp;              /* temp variable                      */

    VRR_K( inst, regs, v1, v2, m3 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    nsv = (m3 & 0x08) ? true : false;
    nv  = (m3 & 0x04) ? true : false;

    /* validate sign */
    if ( !nsv  &&  !vr_packed_valid_sign( regs, v2) )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* validate decimals */
    if (!nv && !vr_packed_valid_digits( regs, v2 ))
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* set siggnificant zone digit to zero */
    regs->VR_B( v1, 0 )  = 0xF0;

    /* 14 decimals */
    for (i = 1, indx = 0;  i < 14; i += 2, indx++ )
    {
        temp = regs->VR_B( v2, indx );
        regs->VR_B( v1, i )  = PACKED_HIGH (temp ) | 0xF0;
        regs->VR_B( v1, i+1 )  = PACKED_LOW  (temp ) | 0xF0;
    }

    /* 15th decimal */
    temp = regs->VR_B( v2, 7 );
    regs->VR_B( v1, 15 )  = PACKED_HIGH (temp ) | 0xF0;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E655 VCNF   - VECTOR FP CONVERT TO NNP                    [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_nnp )
{
    int     v1, r3, b2, m4;
    VADR    effective_addr2;

    VRR_A(inst, regs, v1, r3, b2, effective_addr2, m4);

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );

    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*--------------------------------------------------------------------*/
/* E656 VCLFNH - VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH [VRR_a] */
/*--------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_lengthen_from_nnp_high )
{
    int     v1, v3, b2, m4;
    VADR    effective_addr2;

    VRR_A( inst, regs, v1, v3, b2, effective_addr2, m4 );

    ZVECTOR_CHECK( regs );
    PER_ZEROADDR_XCHECK( regs, b2 );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E658 VCVD   - VECTOR CONVERT TO DECIMAL (32)              [VRI-i] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_to_decimal_32 )
{
    int     v1, r2, m4, i3;      /* Instruction parts                */
    bool    iom;                 /* Instruction-Overflow Mask (IOM)  */
    int     rdc;                 /* Result Digits Count(RDC) Bit 3-7 */
    bool    p1;                  /* Force Operand 1 Positive (P1)    */
    bool    lb;                  /* Logical Binary (LB)              */
    bool    cs;                  /* Condition Code Set (CS)          */

    bool    possign;             /* result has positive sign         */
    U64     convert;             /* value to convert                 */
    U32     reg32;               /* register to convert              */
    int     i;                   /* Loop variable                    */
    U8      digit;               /* digit of packed byte             */
    int     temp;                /* temp                             */
    bool    overflow;            /* did an overfor occur             */


    VRI_I( inst, regs, v1, r2, m4, i3 );

    ZVECTOR_CHECK( regs );

                              /* i3 reserved bits 1-2 must be zero    */
    if ( i3 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i3 parts */
    iom = (i3 & 0x80) ? true : false;
    rdc = (i3 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );


    /* m4 parts */
    lb = (m4 & 0x08) ? true : false;
    p1 = (m4 & 0x02) ? true : false;
    cs = (m4 & 0x01) ? true : false;

    /* start with zero vector */
    SET_VR_ZERO( v1 );

    /* get sign and value to convert */
    reg32 = regs->GR_L( r2 );  /* 32-bits to convert */
    if (lb)
    {
        convert = (U64) reg32;    /* unsigned */
        possign = true;
    }
    else
    {
        convert = (S32) reg32;  /* signed */
        if (convert >0 )
            possign = true;
        else
        {
            possign = false;
            convert = -convert;
        }
    }

    //logmsg("VECTOR CONVERT TO DECIMAL (32): lb=%d, reg32=%X, convert=%ld, convert=%lx \n", lb, reg32, convert, convert);

    /* do convertion to decimal digits */
    for (i = 30, temp = rdc; temp >0 && i >= 0 && convert > 0; i--, temp--)
    {
        digit = convert % 10;
        convert = convert / 10;

        regs->VR_B( v1, i / 2) |=  ( i & 1) ? digit : digit << 4;
    }

    overflow = convert > 0;     /* did not convert all (rdc limited rersult) */

    /* set sign */
    if (p1)
        regs->VR_B( v1, VR_PACKED_SIGN ) |= 0x0F;   /* forces b'1111' positive sign */
    else
        regs->VR_B( v1, VR_PACKED_SIGN ) |= (possign) ? PREFERRED_PLUS :  PREFERRED_MINUS ;

    /* set condition code */
    if (cs)
        regs->psw.cc = (overflow) ? 3 : 0;

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E659 VSRP   - VECTOR SHIFT AND ROUND DECIMAL              [VRi-g] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_and_round_decimal )
{
    int     v1, v2, i4, m5, i3;

    VRI_G( inst, regs, v1, v2, i4, m5, i3 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65A VCVDG  - VECTOR CONVERT TO DECIMAL (64)              [VRI-i] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_to_decimal_64 )
{
    int     v1, r2, m4, i3;      /* Instruction parts                */
    bool    iom;                 /* Instruction-Overflow Mask (IOM)  */
    int     rdc;                 /* Result Digits Count(RDC) Bit 3-7 */
    bool    p1;                  /* Force Operand 1 Positive (P1)    */
    bool    lb;                  /* Logical Binary (LB)              */
    bool    cs;                  /* Condition Code Set (CS)          */

    bool    possign;             /* result has positive sign         */
    S64     tempS64;             /* temp S64                         */
    U64     convert;             /* value to convert                 */
    U64     reg64;               /* register to convert              */
    int     i;                   /* Loop variable                    */
    U8      digit;               /* digit of packed byte             */
    int     temp;                /* temp                             */
    bool    overflow;            /* did an overfor occur             */

    VRI_I( inst, regs, v1, r2, m4, i3 );

    ZVECTOR_CHECK( regs );

                              /* i3 reserved bits 1-2 must be zero    */
    if ( i3 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i3 parts */
    iom = (i3 & 0x80) ? true : false;
    rdc = (i3 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m4 parts */
    lb = (m4 & 0x08) ? true : false;
    p1 = (m4 & 0x02) ? true : false;
    cs = (m4 & 0x01) ? true : false;

    /* start with zero vector */
    SET_VR_ZERO( v1 );

    /* get sign and value to convert */
    reg64 = regs->GR( r2 );  /* 64-bits to convert */
    if (lb)
    {
        convert = reg64;    /* unsigned */
        possign = true;
    }
    else
    {                       /* signed */
        tempS64 = (S64) reg64;
        if (tempS64 >0 )
        {
            possign = true;
            convert = (U64) tempS64;
        }
        else
        {
            possign = false;
            convert = (U64) -tempS64;
        }
    }

    //logmsg("VECTOR CONVERT TO DECIMAL (64): lb=%d, reg64=%lX, convert=%ld, convert=%lx \n", lb, reg64, convert, convert);

    /* do convertion to decimal digits */
    for (i = 30, temp = rdc; temp >0 && i >= 0 && convert > 0; i--, temp--)
    {
        digit = convert % 10;
        convert = convert / 10;

        regs->VR_B( v1, i / 2) |=  ( i & 1) ? digit : digit << 4;
    }

    overflow = convert > 0;     /* did not convert all (rdc limited rersult) */

    /* set sign */
    if (p1)
        regs->VR_B( v1, VR_PACKED_SIGN) |= 0x0F;   /* forces b'1111' positive sign */
    else
        regs->VR_B( v1, VR_PACKED_SIGN) |= (possign) ? PREFERRED_PLUS :  PREFERRED_MINUS ;

    /* set condition code */
    if (cs)
        regs->psw.cc = (overflow) ? 3 : 0;

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65B VPSOP  - VECTOR PERFORM SIGN OPERATION DECIMAL       [VRI-g] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_perform_sign_operation_decimal )
{
    int     v1, v2, i4, m5, i3;

    VRI_G( inst, regs, v1, v2, i4, m5, i3 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65C VUPKZL - VECTOR UNPACK ZONED LOW                     [VRR-k] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_unpack_zoned_low )
{
    int     v1, v2, m3;        /* Instruction parts                  */
    bool    nsv;               /* No Sign Validation (NSV): bit 0    */
    bool    nv;                /* No Validation (NV): bit 1          */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    int     i;                 /* loop variable                      */
    int     indx;              /* index variable                     */
    U8      temp;              /* temp variable                      */
    U8      zoned_sign;        /* sign for zoned digit               */

    VRR_K( inst, regs, v1, v2, m3 );

    ZVECTOR_CHECK( regs );

    /* m3 parts */
    nsv = (m3 & 0x08) ? true : false;
    nv  = (m3 & 0x04) ? true : false;
    p1  = (m3 & 0x02) ? true : false;

    /* validate sign: nsv=0 and nv=0 */
    if ( !nsv && !nv && !vr_packed_valid_sign( regs, v2 ) )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* validate decimals */
    if (!nv && !vr_packed_valid_digits( regs, v2 ))
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* result sign */
    zoned_sign = (p1) ? PREFERRED_ZONE : PACKED_SIGN ( regs->VR_B( v2, VR_PACKED_SIGN ) );
    zoned_sign <<=  4;

    /* 1st decimal */
    temp = regs->VR_B( v2, 7 );
    regs->VR_B( v1, 0 )  = PACKED_LOW (temp ) | 0xF0;

    /* 2-15 decimals */
    for (i = 1, indx = 8;  i < 15; i += 2, indx++ )
    {
        temp = regs->VR_B( v2, indx );
        regs->VR_B( v1, i )  = PACKED_HIGH (temp ) | 0xF0;
        regs->VR_B( v1, i+1 )  = PACKED_LOW  (temp ) | 0xF0;
    }

    /* 16th decimal */
    temp = regs->VR_B( v2, 15 );
    regs->VR_B( v1, 15 )  = PACKED_HIGH (temp ) | zoned_sign;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65D VCFN   - VECTOR FP CONVERT FROM NNP                  [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_from_nnp )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65E VCLFNL - VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW [VRR-a] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_lengthen_from_nnp_low )
{
    int     v1, v2, m3, m4, m5;

    VRR_A( inst, regs, v1, v2, m3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E65F VTP    - VECTOR TEST DECIMAL                         [VRR-g] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_test_decimal )
{
    int     v1;                /* Instruction parts                  */
    bool    valid_decimal;     /* decimal validation failed?         */
    bool    valid_sign;        /* sign validation failed?            */
    U8      cc;                /* condition code                     */

    VRR_G( inst, regs, v1 );

    ZVECTOR_CHECK( regs );

    /* validate decimals */
    valid_decimal = vr_packed_valid_digits( regs, v1 );

    /* validate sign */
    valid_sign = vr_packed_valid_sign( regs, v1 );

    /* set condition code */
    cc = (valid_decimal) ?  ( (valid_sign) ? 0 : 1) :
                            ( (valid_sign) ? 2 : 3) ;
    regs->psw.cc = cc;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E670 VPKZR  - VECTOR PACK ZONED REGISTER                  [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_pack_zoned_register )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
    bool    nsv;               /* No Sign Validation (NSV):    bit 0 */
    bool    nv;                /* No Validation (NV):          bit 1 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    overflowed;        /* overflow occurred                  */
    bool    isZero;            /* is zoned decimal zero              */
    bool    isPositive;        /* is zoned decimal positive          */
    int     i;                 /* loop variable                      */
    int     indx;              /* index variable                     */
    U8      temp;              /* temp variable                      */
    int     temp_rdc;          /* temp of rdc                        */
    U8      zoned[32];         /* intermediate zoned decimal         */
    U8      packed_sign;       /* sign for packed vector             */
    QW      tempVR;            /* temp vector regiter sized field    */
    bool    valid_sign;        /* is sign valid?                     */
    bool    valid_decimals;    /* are decimals valid?                */
    BYTE    cc;                /* condition code                     */

#if !defined( FEATURE_192_VECT_PACKDEC_ENH_2_FACILITY )
    ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
#endif

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    nsv = (m5 & 0x08) ? true : false;
    nv  = (m5 & 0x04) ? true : false;
    p1  = (m5 & 0x02) ? true : false;
    cs  = (m5 & 0x01) ? true : false;

    /* create intermediate source */
    tempVR =  CSWAP128( regs->VR_Q ( v2 ) );    /* VR part 1 */
    memcpy( &zoned[0], &tempVR, sizeof(QW)  );

    tempVR =  CSWAP128( regs->VR_Q ( v3 ) );    /* VR part 2 */
    memcpy( &zoned[16], &tempVR, sizeof(QW)  );

    /* zoned validation */
    valid_sign = ZONED_SIGN( zoned[31] ) > 9;
    valid_decimals = true;
    for (i =0; i < 32 && valid_decimals; i++)
        valid_decimals = IS_VALID_DECIMAL( zoned[i] );

    /* general-operand data exception? */
    /* validate sign: nsv=0 and nv=0 */
    if ( !nsv && !nv && !valid_sign )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* validate decimals */
    if ( !nv & !valid_decimals )
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* build packed vector */
    SET_VR_ZERO( v1 );
    packed_sign = (p1) ? PREFERRED_ZONE : ZONED_SIGN( zoned[31] );
    regs->VR_B( v1, VR_PACKED_SIGN) = (ZONED_DECIMAL( zoned[31] ) << 4) | packed_sign;

    indx = 14;
    for (i = 30, temp_rdc = rdc-1; i >=0 && temp_rdc > 0 && indx >=0 ; i--, temp_rdc--)
    {
        temp = ZONED_DECIMAL( zoned[ i ] );
        regs->VR_B( v1, indx) |= (i & 1) ? temp << 4 : temp;
        if ( i & 1) indx--;
    }

    /* check for overflow */
    overflowed = false;
    for (i = 0; i < (32 - rdc) && !overflowed; i++)
        overflowed = ( ZONED_DECIMAL( zoned[ i ]) == 0 ) ? false : true;

    /* set condition code */
    if (cs)
    {
        /* is the zone decimal 0 */
        isZero = true;
        for (i = 0; i < 32 && isZero; i++)
            isZero = ( ZONED_DECIMAL( zoned[ i ]) == 0 ) ? true : false;

        /* is the zone decimal positive */
        isPositive = IS_PLUS_SIGN(  packed_sign  );

        if       ( overflowed || !valid_decimals || !valid_sign) cc = 3;
        else if  (!overflowed &&  valid_decimals &&  valid_sign && !isZero &&  isPositive) cc = 2;
        else if  (!overflowed &&  valid_decimals &&  valid_sign && !isZero && !isPositive) cc = 1;
        else if  (!overflowed &&  valid_decimals &&  valid_sign &&  isZero ) cc = 0;
        else cc = 0; /* should not get here */

        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflowed  && FOMASK(&regs->psw))
    {
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
    }

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E671 VAP    - VECTOR ADD DECIMAL                          [VRI-f] */
/*-------------------------------------------------------------------*/
/* based on decimal.c (add_decimal)                                  */
/*-------------------------------------------------------------------*/
DEF_INST( vector_add_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 3 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    BYTE    dec2[MAX_DECIMAL_DIGITS];   /* Work area for operand 2   */
    BYTE    dec3[MAX_DECIMAL_DIGITS];   /* Work area for operand 3   */
    BYTE    dec1[MAX_DECIMAL_DIGITS];   /* Work area for result      */
    int     count1, count2, count3;     /* Significant digit counters*/
    int     sign1, sign2, sign3;        /* Sign of operands & result */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decimals */
    vr_packed_2_decimal( regs, v2, (BYTE*) &dec2, &count2, &sign2);
    vr_packed_2_decimal( regs, v3, (BYTE*) &dec3, &count3, &sign3);

    /* forced operand positive */
    if (p2) sign2 = 1;
    if (p3) sign3 = 1;

    /* Add or subtract operand values */
    if (count2 == 0)
    {
        /* If second operand is zero then result is third operand */
        memcpy (dec1, dec3, MAX_DECIMAL_DIGITS);
        count1 = count3;
        sign1 = sign3;
    }
    else if (count3 == 0)
    {
        /* If third operand is zero then result is second operand */
        memcpy (dec1, dec2, MAX_DECIMAL_DIGITS);
        count1 = count2;
        sign1 = sign2;
    }
    else if (sign2 == sign3)
    {
        /* If signs are equal then add operands */
        add_decimal (dec2, dec3, dec1, &count1);
        sign1 = sign3;
        overflow = count1 > MAX_DECIMAL_DIGITS;
    }
    else
    {
        /* If signs are opposite then subtract operands */
        subtract_decimal (dec2, dec3, dec1, &count1, &sign1);
        if (sign2 < 0) sign1 = -sign1;
    }

    /* overflowed */
    if ( !overflow && rdc < MAX_DECIMAL_DIGITS)
        overflow = decimal_overflow( dec1, rdc );

    decimal_2_vr_packed( regs, v1, (BYTE*) &dec1, sign1, p1, rdc);

    /* set condition code */
    if (cs)
    {
        if (p1) sign1 = 1;
        cc = (count1 == 0) ? 0 : (sign1 < 1) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E672 VSRPR  - VECTOR SHIFT AND ROUND DECIMAL REGISTER     [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_and_round_decimal_register )
{
    int     v1, v2, v3, m5, i4;

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E673 VSP    - VECTOR SUBTRACT DECIMAL                     [VRI-f] */
/*-------------------------------------------------------------------*/
/* based on decimal.c (subtract_decimal)                             */
/*-------------------------------------------------------------------*/
DEF_INST( vector_subtract_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 3 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    BYTE    dec2[MAX_DECIMAL_DIGITS];   /* Work area for operand 2   */
    BYTE    dec3[MAX_DECIMAL_DIGITS];   /* Work area for operand 3   */
    BYTE    dec1[MAX_DECIMAL_DIGITS];   /* Work area for result      */
    int     count1, count2, count3;     /* Significant digit counters*/
    int     sign1, sign2, sign3;        /* Sign of operands & result */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decimals */
    vr_packed_2_decimal( regs, v2, (BYTE*) &dec2, &count2, &sign2);
    vr_packed_2_decimal( regs, v3, (BYTE*) &dec3, &count3, &sign3);

    /* forced operand positive */
    if (p2) sign2 = 1;
    if (p3) sign3 = 1;

    /* Add or subtract operand values: doing (dec2 - dec3) */
    if (count2 == 0)
    {
        /* If second operand is zero then result is third operand */
        memcpy (dec1, dec3, MAX_DECIMAL_DIGITS);
        count1 = count3;
        sign1 = -sign3;
    }
    else if (count3 == 0)
    {
        /* If third operand is zero then result is second operand */
        memcpy (dec1, dec2, MAX_DECIMAL_DIGITS);
        count1 = count2;
        sign1 = sign2;
    }
    else if (sign2 != sign3)
    {
        /* If signs are opposite then add operands */
        add_decimal (dec2, dec3, dec1, &count1);
        sign1 = sign2;
    }
    else
    {
        /* If signs are equal then subtract operands */
        subtract_decimal (dec2, dec3, dec1, &count1, &sign1);
        if (sign2 < 0) sign1 = -sign1;
    }

    /* overflowed */
    overflow = decimal_overflow( dec1, rdc );

    decimal_2_vr_packed( regs, v1, (BYTE*) &dec1, sign1, p1, rdc);

    /* set condition code */
    if (cs)
    {
        if (p1) sign1 = 1;
        cc = (count1 == 0) ? 0 : (sign1 < 1) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E674 VSCHP  - DECIMAL SCALE AND CONVERT TO HFP            [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST( decimal_scale_and_convert_to_hfp )
{
    int     v1, v2, m3, m4, m5;

    VRR_B(inst, regs, v1, v2, m3, m4, m5);
    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E675 VCRNF  - VECTOR FP CONVERT AND ROUND TO NNP          [VRR-c] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_fp_convert_and_round_to_nnp )
{
    int     v1, v2, v3, m4, m5, m6;

    VRR_C(inst, regs, v1, v2, v3, m4, m5, m6);
    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E677 VCP    - VECTOR COMPARE DECIMAL                      [VRR-h] */
/*-------------------------------------------------------------------*/
/* based on decimal.c (compare_decimal)                              */
/*-------------------------------------------------------------------*/
DEF_INST( vector_compare_decimal )
{
    int     v1, v2, m3;        /* Instruction parts                 */

                               /* m3 bits                            */
    bool    p1;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p2;                /* Force Operand 3 Positive(P1) bit 3 */

    bool    valid_sign1;       /* v1: is sign valid?                 */
    bool    valid_decimals1;   /* v1: are decimals valid?            */
    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    BYTE    cc;                /* condition code                     */

    BYTE    dec1[MAX_DECIMAL_DIGITS];   /* Work area for operand 1   */
    BYTE    dec2[MAX_DECIMAL_DIGITS];   /* Work area for operand 2   */
    int     count1, count2;             /* Significant digit counters*/
    int     sign1, sign2;               /* Sign of operands          */
    int     rc;                         /* Return code               */

    VRR_H(inst, regs, v1, v2, m3);

    ZVECTOR_CHECK( regs );

    /* m5 parts */
    p1 = (m3 & 0x08) ? true : false;
    p2 = (m3 & 0x04) ? true : false;

    /* valid checks */
    valid_decimals1 = vr_packed_valid_digits( regs, v1 );
    valid_sign1 = (p1) ? true : vr_packed_valid_sign( regs, v1 );

    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    if (!valid_decimals1 || !valid_sign1 || !valid_decimals2 || !valid_sign2)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decimals */
    vr_packed_2_decimal( regs, v1, (BYTE*) &dec1, &count1, &sign1);
    vr_packed_2_decimal( regs, v2, (BYTE*) &dec2, &count2, &sign2);

    /* forced operand positive */
    if (p1) sign1 = 1;
    if (p2) sign2 = 1;

    /* Result is equal if both operands are zero */
    if (count1 == 0 && count2 == 0)
    {
        cc = 0;
    }

    /* Result is low if operand 1 is -ve and operand 2 is +ve */
    if (sign1 < 0 && sign2 > 0)
    {
        cc = 1;
    }

    /* Result is high if operand 1 is +ve and operand 2 is -ve */
    if (sign1 > 0 && sign2 < 0)
    {
        cc = 2;
    }
    else
    {
        /* signs are equal then compare the digits */
        rc = memcmp (dec1, dec2, MAX_DECIMAL_DIGITS);

        /* Return low or high (depending on sign) if digits are unequal */
        if (rc < 0)
            cc = (sign1 > 0) ? 1 : 2;
        else
            if (rc > 0)
                cc = (sign1 > 0) ? 2 : 1;
            else
                cc = 0;
    }

    regs->psw.cc = cc;

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E678 VMP    - VECTOR MULTIPLY DECIMAL                     [VRI-f] */
/*-------------------------------------------------------------------*/
/* based on decimal.c (multiply_decimal)                             */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 3 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    BYTE    dec2[MAX_DECIMAL_DIGITS];   /* Work area for operand 2   */
    BYTE    dec3[MAX_DECIMAL_DIGITS];   /* Work area for operand 3   */
    BYTE    dec1[MAX_DECIMAL_DIGITS];   /* Work area for result      */
    int     count1, count2, count3;     /* Significant digit counters*/
    int     sign1, sign2, sign3;        /* Sign of operands & result */
    int     d;                          /* Decimal digit             */
    int     i1, i2, i3;                 /* Array subscripts          */
    int     k;                          /* loop index                */
    int     carry = 0;                  /* Carry indicator           */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decimals */
    vr_packed_2_decimal( regs, v2, (BYTE*) &dec2, &count2, &sign2);
    vr_packed_2_decimal( regs, v3, (BYTE*) &dec3, &count3, &sign3);

    /* forced operand positive */
    if (p2) sign2 = 1;
    if (p3) sign3 = 1;

    /* Clear the result field */
    memset( dec1, 0, MAX_DECIMAL_DIGITS );

    /* Is either v2 or v3 is zero */
    if (count2 == 0 || count3 == 0)
        count1 = 0;         /* V1 is zero */

    /* Is v2 equal to one */
    else if (count2 == 1 && dec2[MAX_DECIMAL_DIGITS-1] == 1)
    {
       /* result is v3 */
        memcpy( dec1, dec3, MAX_DECIMAL_DIGITS );
        count1 = count3;
    }

    /* Is v3 equal to one */
    else if (count3 == 1 && dec3[MAX_DECIMAL_DIGITS-1] == 1)
    {
       /* result is v2 */
       memcpy( dec1, dec2, MAX_DECIMAL_DIGITS );
       count1 = count2;
    }

    /* Perform decimal multiplication */
    else
    {
        for (i2 = MAX_DECIMAL_DIGITS-1; i2 >= 0; i2--)
        {
            if (dec2[i2] != 0)
            {
                for (i1 = MAX_DECIMAL_DIGITS - 1, i3 = i2, carry = 0;
                            i3 >= 0; i1--, i3--)
                {
                    d = carry + dec3[i1]*dec2[i2] + dec1[i3];
                    dec1[i3] = d % 10;
                    carry = d / 10;

                }
            }
        } /* end for(i2) */

        /* result significant digits */
        for (k = 0, count1 = MAX_DECIMAL_DIGITS; k <  MAX_DECIMAL_DIGITS; k++, count1--)
        {
            if (dec1[k] != 0) break;
        }
    }

    /* Result is positive if operand signs are equal, and negative
       if operand signs are opposite, even if result is zero */
    sign1 = (sign2 == sign3) ? 1 : -1;

    /* overflowed */
    overflow = carry > 0 || decimal_overflow( dec1, rdc );

    decimal_2_vr_packed( regs, v1, (BYTE*) &dec1, sign1, p1, rdc);

    /* set condition code */
    if (cs)
    {
        if (p1) sign1 = 1;
        cc = (count1 == 0) ? 0 : (sign1 < 1) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E679 VMSP   - VECTOR MULTIPLY AND SHIFT DECIMAL           [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_multiply_and_shift_decimal )
{
    int     v1, v2, v3, m5, i4;

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67A VDP    - VECTOR DIVIDE DECIMAL                       [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_divide_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 3 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    BYTE    dec2[MAX_DECIMAL_DIGITS];   /* Work area for operand 2   */
    BYTE    dec3[MAX_DECIMAL_DIGITS];   /* Work area for operand 3   */
    BYTE    dec1[MAX_DECIMAL_DIGITS];   /* Work area for result      */
//  BYTE    quot[MAX_DECIMAL_DIGITS];   /* Quotient                  */
    BYTE    rem[MAX_DECIMAL_DIGITS];    /* Remainder                 */
    int     count1, count2, count3;     /* Significant digit counters*/
    int     sign1, sign2, sign3;        /* Sign of operands & result */
    int     k;                          /* loop index                */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decimals */
    vr_packed_2_decimal( regs, v2, (BYTE*) &dec2, &count2, &sign2);
    vr_packed_2_decimal( regs, v3, (BYTE*) &dec3, &count3, &sign3);

    /* Program check if second operand value is zero */
    if (count2 == 0)
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_DIVIDE_EXCEPTION);

    /* forced operand positive */
    if (p2) sign2 = 1;
    if (p3) sign3 = 1;

    //========================================================
    //FIX ME: this is NOT VALID as it does not handle overflow
    //========================================================
    /* Perform trial comparison to determine potential overflow.
    The leftmost digit of the divisor is aligned one digit to
    the right of the leftmost dividend digit.  When the divisor,
    so aligned, is less than or equal to the dividend, ignoring
    signs, a divide exception is indicated.  As a result of this
    comparison, it is also certain that the leftmost digit of the
    dividend must be zero, and that the divisor cannot be zero */
    if ( (MAX_DECIMAL_DIGITS - count2) > 0 &&
         (count2 - count3)== 1)
        overflow = true;

    /* Perform decimal division */
    if (!overflow)
        divide_decimal (dec2, count2, dec3, count3, dec1, rem);

    /* overflowed */
    overflow = overflow || decimal_overflow( dec1, rdc );

    /* Quotient is positive if operand signs are equal, and negative
       if operand signs are opposite, even if quotient is zero */
    sign1 = (sign2 == sign3) ? 1 : -1;

    decimal_2_vr_packed( regs, v1, (BYTE*) &dec1, sign1, p1, rdc);

    /* result significant digits */
    for (k = 0, count1 = MAX_DECIMAL_DIGITS; k <  MAX_DECIMAL_DIGITS; k++, count1--)
    {
        if (dec1[k] != 0) break;
    }

    /* set condition code */
    if (cs)
    {
        if (p1) sign1 = 1;
        cc = (count1 == 0) ? 0 : (sign1 < 1) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67B VRP    - VECTOR REMAINDER DECIMAL                    [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_remainder_decimal )
{
    int     v1, v2, v3, m5, i4; /* Instruction parts                 */
    bool    iom;               /* Instruction-Overflow Mask (IOM)    */
    int     rdc;               /* Result Digits Count(RDC) Bit 3-7   */
                               /* m5 bits                            */
    bool    p2;                /* Force Operand 2 Positive(P2) bit 0 */
    bool    p3;                /* Force Operand 3 Positive(P1) bit 3 */
    bool    p1;                /* Force Operand 1 Positive(P1) bit 2 */
    bool    cs;                /* Condition Code Set (CS):     bit 3 */

    bool    valid_sign2;       /* v2: is sign valid?                 */
    bool    valid_decimals2;   /* v2: are decimals valid?            */
    bool    valid_sign3;       /* v3: is sign valid?                 */
    bool    valid_decimals3;   /* v3: are decimals valid?            */
    BYTE    cc;                /* condition code                     */
    bool    overflow = false;  /* overflowed?                        */

    BYTE    dec2[MAX_DECIMAL_DIGITS];   /* Work area for operand 2   */
    BYTE    dec3[MAX_DECIMAL_DIGITS];   /* Work area for operand 3   */
    BYTE    dec1[MAX_DECIMAL_DIGITS];   /* Work area for result      */
//  BYTE    quot[MAX_DECIMAL_DIGITS];   /* Quotient                  */
    BYTE    rem[MAX_DECIMAL_DIGITS];    /* Remainder                 */
    int     count1, count2, count3;     /* Significant digit counters*/
    int     sign1, sign2, sign3;        /* Sign of operands & result */
    int     k;                          /* loop index                */

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );

                              /* i4 reserved bits 1-2 must be zero    */
    if ( i4 & 0x60 )          /*  not zero => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* i4 parts */
    iom = (i4 & 0x80) ? true : false;
    rdc = (i4 & 0x1F);

#if !defined( FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
    if (iom)
        ARCH_DEP(program_interrupt)( regs, PGM_SPECIFICATION_EXCEPTION );
#endif

    if ( rdc == 0 )          /* zero rdc => Specficitcation excp    */
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* m5 parts */
    p2 = (m5 & 0x08) ? true : false;
    p3 = (m5 & 0x04) ? true : false;
    p1 = (m5 & 0x02) ? true : false;
    cs = (m5 & 0x01) ? true : false;

    /* valid checks */
    valid_decimals2 = vr_packed_valid_digits( regs, v2 );
    valid_sign2 = (p2) ? true : vr_packed_valid_sign( regs, v2 );

    valid_decimals3 = vr_packed_valid_digits( regs, v3 );
    valid_sign3 = (p3) ? true : vr_packed_valid_sign( regs, v3 );

    if (!valid_decimals2 || !valid_sign2 || !valid_decimals3 || !valid_sign3)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) ( regs, PGM_DATA_EXCEPTION );
    }

    /* operands as decimals */
    vr_packed_2_decimal( regs, v2, (BYTE*) &dec2, &count2, &sign2);
    vr_packed_2_decimal( regs, v3, (BYTE*) &dec3, &count3, &sign3);

    /* Program check if second operand value is zero */
    if (count2 == 0)
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_DIVIDE_EXCEPTION);

    /* forced operand positive */
    if (p2) sign2 = 1;
    if (p3) sign3 = 1;

    //========================================================
    //FIX ME: this is NOT VALID as it does not handle overflow
    //========================================================
    /* Perform trial comparison to determine potential overflow.
    The leftmost digit of the divisor is aligned one digit to
    the right of the leftmost dividend digit.  When the divisor,
    so aligned, is less than or equal to the dividend, ignoring
    signs, a divide exception is indicated.  As a result of this
    comparison, it is also certain that the leftmost digit of the
    dividend must be zero, and that the divisor cannot be zero */
    if ( (MAX_DECIMAL_DIGITS - count2) > 0 &&
         (count2 - count3)== 1)
        overflow = true;

    /* Perform decimal division */
    if (!overflow)
        divide_decimal (dec2, count2, dec3, count3, dec1, rem);

    /* overflowed */
    overflow = overflow || decimal_overflow( rem, rdc );

    /* Quotient is positive if operand signs are equal, and negative
       if operand signs are opposite, even if quotient is zero */
    sign1 = (sign2 == sign3) ? 1 : -1;

    decimal_2_vr_packed( regs, v1, (BYTE*) &rem, sign1, p1, rdc);

    /* result significant digits */
    for (k = 0, count1 = MAX_DECIMAL_DIGITS; k <  MAX_DECIMAL_DIGITS; k++, count1--)
    {
        if (rem[k] != 0) break;
    }

    /* set condition code */
    if (cs)
    {
        if (p1) sign1 = 1;
        cc = (count1 == 0) ? 0 : (sign1 < 1) ? 1 : 2;
        if ( overflow ) cc = 3;
        regs->psw.cc = cc;
    }

    /* note: operation is completed before any fixed-point overflow exception */
    /* masked overflow? */
    if ( !iom && overflow  && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) ( regs, PGM_DECIMAL_OVERFLOW_EXCEPTION );

    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67C VSCSHP - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP  [VRR-b] */
/*-------------------------------------------------------------------*/
DEF_INST(decimal_scale_and_convert_and_split_to_hfp )
{
    int     v1, v2, v3, m4, m5;

    VRR_B( inst, regs, v1, v2, v3, m4, m5 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67D VCSPH  - VECTOR CONVERT HFP TO SCALED DECIMAL        [VRR-j] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_convert_hfp_to_scaled_decimal )
{
    int     v1, v2, v3, m4;

    VRR_J( inst, regs, v1, v2, v3, m4 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

/*-------------------------------------------------------------------*/
/* E67E VSDP   - VECTOR SHIFT AND DIVIDE DECIMAL             [VRI-f] */
/*-------------------------------------------------------------------*/
DEF_INST( vector_shift_and_divide_decimal )
{
    int     v1, v2, v3, m5, i4;

    VRI_F( inst, regs, v1, v2, v3, m5, i4 );

    ZVECTOR_CHECK( regs );
    //
    // TODO: insert code here
    //
    if (1) ARCH_DEP(program_interrupt)( regs, PGM_OPERATION_EXCEPTION );
    //
    ZVECTOR_END( regs );
}

#endif /* defined(FEATURE_129_ZVECTOR_FACILITY) */

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "zvector2.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "zvector2.c"
  #endif

#endif /*!defined(_GEN_ARCH)*/
