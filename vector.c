/* VECTOR.C     (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              S/370 and ESA/390 Vector Operations                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements the Vector Facility instruction execution  */
/* function of the S/370 and ESA/390 architectures, as described in  */
/* SA22-7125-03 Vector Operations (S/370 & ESA/370)                  */
/* SA22-7207-00 Vector Operations (ESA/390)                          */
/*                                             28/05/2000 Jan Jaeger */
/*                                                                   */
/* Instruction decoding rework                 09/07/2000 Jan Jaeger */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _VECTOR_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

DISABLE_GCC_UNUSED_SET_WARNING;

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )

/* The vector save area must be aligned on a boundary 8 times the
   section size, however VM stores at 4 times the section size.
   I do not know if the book or VM is wrong.                     *JJ */
#define VSA_ALIGN       4

/*-------------------------------------------------------------------*/
/* A640 VTVM  - Test VMR                                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_test_vmr)
{
int     r1, r2;
U32     n, n1;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* cc0 when the vector count is zero */
    if( n == 0)
    {
        regs->psw.cc = 0;
        return;
    }

    /* Preset condition code according to first bit */
    regs->psw.cc = VMR_SET(0, regs) ? 3 : 0;

    /* Check VMR bit to be equal to the first,
       exit with cc1 if an unequal bit found */
    for(n1 = 1; n1 < n; n1++)
        if((regs->psw.cc == 0) != (VMR_SET(n1, regs) == 0))
        {
            regs->psw.cc = 1;
            return;
        }
}

/*-------------------------------------------------------------------*/
/* A641 VCVM  - Complement VMR                                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_complement_vmr)
{
int     r1, r2;
U32     n, n1, n2;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* Bytes - 1 */
    n1 = n >> 3;

    /* Complement VMR */
    for(n2 = 0; n2 <= n1; n2++)
        regs->vf->vmr[n2] ^= 0xFF;

    /* zeroize remainder */
    regs->vf->vmr[n1] &= 0x7F00 >> (n & 7);
    for(n1++; n1 < sizeof(regs->vf->vmr); n1++)
        regs->vf->vmr[n1] = 0;
}

/*-------------------------------------------------------------------*/
/* A642 VCZVM - Count Left Zeros in VMR                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_count_left_zeros_in_vmr)
{
int     r1, r2;
U32     n, n1;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* cc0 when the vector count is zero */
    if( n == 0)
    {
        regs->psw.cc = 0;
        return;
    }

    /* Preset condition code according to first bit */
    regs->psw.cc = VMR_SET(0, regs) ? 3 : 0;

    /* If the VCT is 1 and the first bit is one
       then exit wirh r1 set to zero */
    regs->GR_L(r1) = 0;
    if(n == 1 && VMR_SET(0, regs))
        return;

    /* Count left zeros, set cc1 and exit if a one is found */
    regs->GR_L(r1) = 1;
    for(n1 = 1; n1 < n; n1++)
    {
        if(!VMR_SET(n1, regs))
            regs->GR_L(r1)++;
        else
        {
            regs->psw.cc = 1;
            return;
        }
    }
}

/*-------------------------------------------------------------------*/
/* A643 VCOVM - Count Ones In VMR                              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_count_ones_in_vmr)
{
int     r1, r2;
U32     n, n1;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* cc0 when the vector count is zero */
    if( n == 0)
    {
        regs->psw.cc = 0;
        return;
    }

    /* Preset condition code according to first bit */
    regs->psw.cc = VMR_SET(0, regs) ? 3 : 0;

    /* Check VMR bit to be equal to the first,
       Count all ones, set cc1 if a bit is unequal */
    regs->GR_L(r1) = 0;
    for(n1 = 0; n1 < n; n1++)
    {
        if(VMR_SET(n1, regs))
        {
            regs->GR_L(r1)++;
            if(!VMR_SET(0, regs))
                regs->psw.cc = 1;
        }
        else
            if(VMR_SET(0, regs))
                regs->psw.cc = 1;
    }
}

/*-------------------------------------------------------------------*/
/* A644 VXVC  - Exctract VCT                                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_extract_vct)
{
int     r1, r2;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    regs->GR_L(r1) = VECTOR_COUNT(regs);
}

/*-------------------------------------------------------------------*/
/* A646 VXVMM - Extract Vector Modes                           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_extract_vector_modes)
{
int     r1, r2;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    regs->GR_L(r1) = (regs->vf->vsr >> 48);
}

/*-------------------------------------------------------------------*/
/* A648 VRRS  - Restore VR                                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_restore_vr)
{
int     r1, r2;
U32     n, n1, n2;
U64     d;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    ODD_CHECK(r1, regs);

    /* n contains the current save area address */
    n = regs->GR_L(r1) & ADDRESS_MAXWRAP(regs);

    /* n1 contains the starting element number */
    if((n1 = regs->GR_L(r1 + 1) >> 16) >= VECTOR_SECTION_SIZE)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    /* Starting address must be eight times the section size aligned */
    if((n - (8 * n1)) & ((VECTOR_SECTION_SIZE * VSA_ALIGN) - 1) )
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    /* n2 contains VR pair, which must be an even reg */
    if((n2 = regs->GR_L(r1 + 1) & 0x0000FFFF) & 0x0000FFF1)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    if( VR_INUSE(n2, regs) )
    {
        /* Set the vector changed bit if in problem state */
        if( PROBSTATE(&regs->psw) )
            SET_VR_CHANGED(n2, regs);

        for(; n1 < VECTOR_SECTION_SIZE; n1++)
        {
            /* Fetch vr pair from central storage */
            d = ARCH_DEP(vfetch8)(n, r1, regs);
            regs->vf->vr[n2][n1] = d >> 32;
            regs->vf->vr[n2+1][n1] = d;

            /* Increment element number */
            n1++;
            regs->GR_L(r1 + 1) &= 0x0000FFFF;
            regs->GR_L(r1 + 1) |= n1 << 16;
            /* Update savearea address */
            regs->GR_L(r1) += 8;
#if 0
            /* This is where the instruction may be interrupted */
            SET_PSW_IA_AND_MAYBE_IP(regs, PSW_IA_FROM_IP(regs, -4));
            return;
#endif
        }

        /* Indicate vr pair restored */
        regs->psw.cc = 2;
    }
    else
    {
        regs->GR_L(r1) += 8 * (VECTOR_SECTION_SIZE - n1);
        /* indicate v2 pair not restored */
        regs->psw.cc = 0;
    }

    /* Set 2 if vr 14 is restored, 0 if not restored,
       3 and 1 for other VR's respectively */
    if(n2 != 14) regs->psw.cc++;

    /* Update the vector pair number, and zero element number */
    n2 += 2;
    regs->GR_L(r1 + 1) = n2;
}

/*-------------------------------------------------------------------*/
/* A649 VRSVC - Save Changed VR                                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_save_changed_vr)
{
int     r1, r2;
U32     n, n1, n2;
U64     d;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    PRIV_CHECK(regs);

    ODD_CHECK(r1, regs);

    /* n contains the current save area address */
    n = regs->GR_L(r1) & ADDRESS_MAXWRAP(regs);

    /* n1 contains the starting element number */
    if((n1 = regs->GR_L(r1 + 1) >> 16) >= VECTOR_SECTION_SIZE)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    /* Starting address must be eight times the section size aligned */
    if((n - (8 * n1)) & ((VECTOR_SECTION_SIZE * VSA_ALIGN) - 1) )
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    /* n2 contains VR pair, which must be an even reg */
    if((n2 = regs->GR_L(r1 + 1) & 0x0000FFFF) & 0x0000FFF1)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    if( VR_CHANGED(n2, regs) )
    {
        for(; n1 < VECTOR_SECTION_SIZE; n1++)
        {
            /* Store vr pair in savearea */
            d = ((U64)regs->vf->vr[n2][n1] << 32)
              | regs->vf->vr[n2+1][n1];
            ARCH_DEP(vstore8)(d, n, r1, regs);

            /* Update element number */
            n1++;
            regs->GR_L(r1 + 1) &= 0x0000FFFF;
            regs->GR_L(r1 + 1) |= n1 << 16;
            regs->GR_L(r1) += 8;
#if 0
            /* This is where the instruction may be interrupted */
            SET_PSW_IA_AND_MAYBE_IP(regs, PSW_IA_FROM_IP(regs, -4));
            return;
#endif
        }

        /* Indicate vr pair saved */
        regs->psw.cc = 2;

        /* Reset the VR changed bit */
        RESET_VR_CHANGED(n2, regs);
    }
    else
    {
        regs->GR_L(r1) += 8 * (VECTOR_SECTION_SIZE - n1);
        /* vr pair not saved */
        regs->psw.cc = 0;
    }

    /* Set 2 if vr 14 is restored, 0 if not restored,
       3 and 1 for other VR's respectively */
    if(n2 != 14) regs->psw.cc++;

    /* Update the vector pair number, and zero element number */
    n2 += 2;
    regs->GR_L(r1 + 1) = n2;
}

/*-------------------------------------------------------------------*/
/* A64A VRSV  - Save VR                                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(v_save_vr)
{
int     r1, r2;
U32     n, n1, n2;
U64     d;

    RRE(inst, regs, r1, r2);

    VOP_CHECK(regs);

    ODD_CHECK(r1, regs);

    /* n contains the current save area address */
    n = regs->GR_L(r1) & ADDRESS_MAXWRAP(regs);

    /* n1 contains the starting element number */
    if((n1 = regs->GR_L(r1 + 1) >> 16) >= VECTOR_SECTION_SIZE)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    /* Starting address must be eight times the section size aligned */
    if((n - (8 * n1)) & ((VECTOR_SECTION_SIZE * VSA_ALIGN) - 1) )
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    /* n2 contains VR pair, which must be an even reg */
    if((n2 = regs->GR_L(r1 + 1) & 0x0000FFFF) & 0x0000FFF1)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    if( VR_INUSE(n2, regs) )
    {
        for(; n1 < VECTOR_SECTION_SIZE; n1++)
        {
            /* Store vr pair in savearea */
            d = ((U64)regs->vf->vr[n2][n1] << 32)
              | regs->vf->vr[n2+1][n1];
            ARCH_DEP(vstore8)(d, n, r1, regs);

            /* Update element number */
            n1++;
            regs->GR_L(r1 + 1) &= 0x0000FFFF;
            regs->GR_L(r1 + 1) |= n1 << 16;
            regs->GR_L(r1) += 8;
#if 0
            /* This is where the instruction may be interrupted */
            SET_PSW_IA_AND_MAYBE_IP(regs, PSW_IA_FROM_IP(regs, -4));
            return;
#endif
        }

        /* Indicate vr pair restored */
        regs->psw.cc = 2;
    }
    else
    {
        regs->GR_L(r1) += 8 * (VECTOR_SECTION_SIZE - n1);
        /* Indicate vr pair not restored */
        regs->psw.cc = 0;
    }

    /* Set 2 if vr 14 is restored, 0 if not restored,
       3 and 1 for other VR's respectively */
    if(n2 != 14) regs->psw.cc++;

    /* Update the vector pair number, and zero element number */
    n2 += 2;
    regs->GR_L(r1 + 1) = n2;
}

/*-------------------------------------------------------------------*/
/* A680 VLVM  - Load VMR                                        [VS] */
/*-------------------------------------------------------------------*/
DEF_INST(v_load_vmr)
{
int     r2;
U32     n, n1;

    VS(inst, regs, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);
    n1 = n >> 3;

    ARCH_DEP(vfetchc)(regs->vf->vmr, n1,
        regs->GR_L(r2) & ADDRESS_MAXWRAP(regs), r2, regs);

    /* Set the inactive bits to zero */
    regs->vf->vmr[n1] &= 0x7F00 >> (n & 7);
    for(n1++; n1 < sizeof(regs->vf->vmr); n1++)
        regs->vf->vmr[n1] = 0;
}

/*-------------------------------------------------------------------*/
/* A681 VLCVM - Load VMR Complement                             [VS] */
/*-------------------------------------------------------------------*/
DEF_INST(v_load_vmr_complement)
{
int     r2;
U32     n, n1, n2;

    VS(inst, regs, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* Number of bytes - 1 */
    n1 = n >> 3;

    ARCH_DEP(vfetchc)(regs->vf->vmr, n1,
        regs->GR_L(r2) & ADDRESS_MAXWRAP(regs), r2, regs);

    /* Complement all bits loaded */
    for(n2 = 0; n2 <= n1; n2++)
        regs->vf->vmr[n2] ^= 0xFF;

    /* Set the inactive bits to zero */
    regs->vf->vmr[n1] &= 0x7F00 >> (n & 7);
    for(n1++; n1 < sizeof(regs->vf->vmr); n1++)
        regs->vf->vmr[n1] = 0;
}

/*-------------------------------------------------------------------*/
/* A682 VSTVM - Store VMR                                       [VS] */
/*-------------------------------------------------------------------*/
DEF_INST(v_store_vmr)
{
int     r2;
U32     n;

    VS(inst, regs, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    ARCH_DEP(vstorec)(regs->vf->vmr, n >> 3,
            regs->GR_L(r2) & ADDRESS_MAXWRAP(regs), r2, regs);
}

/*-------------------------------------------------------------------*/
/* A684 VNVM  - AND To VMR                                      [VS] */
/*-------------------------------------------------------------------*/
DEF_INST(v_and_to_vmr)
{
int     r2;
U32     n, n1, n2;
BYTE    workvmr[VECTOR_SECTION_SIZE/8];

    VS(inst, regs, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* Number of bytes - 1 */
    n1 = n >> 3;

    ARCH_DEP(vfetchc)(workvmr, n1,
        regs->GR_L(r2) & ADDRESS_MAXWRAP(regs), r2, regs);

    /* And VMR with workvmr */
    for(n2 = 0; n2 <= n1; n2++)
        regs->vf->vmr[n2] &= workvmr[n2];

    /* zeroize remainder */
    regs->vf->vmr[n1] &= 0x7F00 >> (n & 7);
    for(n1++; n1 < sizeof(regs->vf->vmr); n1++)
        regs->vf->vmr[n1] = 0;
}

/*-------------------------------------------------------------------*/
/* A685 VOVM  - OR To VMR                                       [VS] */
/*-------------------------------------------------------------------*/
DEF_INST(v_or_to_vmr)
{
int     r2;
U32     n, n1, n2;
BYTE    workvmr[VECTOR_SECTION_SIZE/8];

    VS(inst, regs, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* Number of bytes - 1 */
    n1 = n >> 3;

    ARCH_DEP(vfetchc)(workvmr, n1,
        regs->GR_L(r2) & ADDRESS_MAXWRAP(regs), r2, regs);

    /* OR VMR with workvmr */
    for(n2 = 0; n2 <= n1; n2++)
        regs->vf->vmr[n2] |= workvmr[n2];

    /* zeroize remainder */
    regs->vf->vmr[n1] &= 0x7F00 >> (n & 7);
    for(n1++; n1 < sizeof(regs->vf->vmr); n1++)
        regs->vf->vmr[n1] = 0;
}

/*-------------------------------------------------------------------*/
/* A686 VXVM  - Exclusive OR To VMR                             [VS] */
/*-------------------------------------------------------------------*/
DEF_INST(v_exclusive_or_to_vmr)
{
int     r2;
U32     n, n1, n2;
BYTE    workvmr[VECTOR_SECTION_SIZE/8];

    VS(inst, regs, r2);

    VOP_CHECK(regs);

    /* Extract vector count (number of active bits in vmr) */
    n = VECTOR_COUNT(regs);

    /* Number of bytes - 1 */
    n1 = n >> 3;

    ARCH_DEP(vfetchc)(workvmr, n1,
        regs->GR_L(r2) & ADDRESS_MAXWRAP(regs), r2, regs);

    /* OR VMR with workvmr */
    for(n2 = 0; n2 <= n1; n2++)
        regs->vf->vmr[n2] ^= workvmr[n2];

    /* zeroize remainder */
    regs->vf->vmr[n1] &= 0x7F00 >> (n & 7);
    for(n1++; n1 < sizeof(regs->vf->vmr); n1++)
        regs->vf->vmr[n1] = 0;
}

/*-------------------------------------------------------------------*/
/* A6C0 VSRSV - Save VSR                                         [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_save_vsr)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    VOP_CHECK(regs);

    DW_CHECK(effective_addr2, regs);

    ARCH_DEP(vstore8)(regs->vf->vsr, effective_addr2, b2, regs);
}

/*-------------------------------------------------------------------*/
/* A6C1 VMRSV - Save VMR                                         [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_save_vmr)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    VOP_CHECK(regs);

    ARCH_DEP(vstorec)(regs->vf->vmr, sizeof(regs->vf->vmr) - 1,
        effective_addr2, b2, regs);
}

/*-------------------------------------------------------------------*/
/* A6C2 VSRRS - Restore VSR                                      [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_restore_vsr)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n1, n2;
U64     d;

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    VOP_CHECK(regs);

    DW_CHECK(effective_addr2, regs);

    /* Fetch operand */
    d = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    /* Check for reserved bits nonzero,
       vector count not greater then section size and
       vector interruption index not greater then section size */
    if((d & VSR_RESV)
        || ((d & VSR_VCT) >> 32) > VECTOR_SECTION_SIZE
        || ((d & VSR_VIX) >> 16) >= VECTOR_SECTION_SIZE)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    /* In problem state the change bit are set corresponding
       the inuse bits */
    if(PROBSTATE(&regs->psw))
    {
        d &= ~VSR_VCH;
        d |= (d & VSR_VIU) >> 8;
    }

    /* Clear any VRs whose inuse bits are being set to zero */
    for(n1 = 0; n1 < 16; n1 += 2)
    {
        if( VR_INUSE(n1, regs)
            && !((d & VSR_VIU) & (VSR_VCH0 >> (n1 >> 1))) )
            for(n2 = 0; n2 < VECTOR_SECTION_SIZE; n2++)
            {
                regs->vf->vr[n1][n2] = 0;
                regs->vf->vr[n1+1][n2] = 0;
            }
    }

    /* Update the vector status register */
    regs->vf->vsr = d;
}

/*-------------------------------------------------------------------*/
/* A6C3 VMRRS - Restore VMR                                      [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_restore_vmr)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    VOP_CHECK(regs);

    ARCH_DEP(vfetchc)(regs->vf->vmr, sizeof(regs->vf->vmr) - 1,
        effective_addr2, b2, regs);
}

/*-------------------------------------------------------------------*/
/* A6C4 VLVCA - Load VCT from Address                            [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_load_vct_from_address)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;

    S_NW(inst, regs, b2, effective_addr2);

    VOP_CHECK(regs);

    regs->psw.cc = ((S32)effective_addr2 == 0) ? 0 :
                   ((S32)effective_addr2 < 0) ? 1 :
                   ((S32)effective_addr2 > VECTOR_SECTION_SIZE) ? 2 : 3;

    n = (S32)effective_addr2 < 0 ? 0 :
        (S32)effective_addr2 > VECTOR_SECTION_SIZE ?
                 VECTOR_SECTION_SIZE : (S32)effective_addr2;

    regs->vf->vsr &= ~VSR_VCT;
    regs->vf->vsr |= (U64)n << 32;
}

/*-------------------------------------------------------------------*/
/* A6C5 VRCL  - Clear VR                                         [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_clear_vr)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n, n1, n2;

    S(inst, regs, b2, effective_addr2);

    VOP_CHECK(regs);

    /* Set vector interruption index to zero */
    regs->vf->vsr &= ~VSR_VIX;

    /* Clear vr's identified in the bit mask
       n1 contains the vr number
       n2 contains the bitmask identifying the vr number
       n contains the element number */
    for(n1 = 0, n2 = 0x80; n1 <= 14; n1 += 2, n2 >>= 1)
        if(effective_addr2 & n2)
        {
            for(n = 0; n < VECTOR_SECTION_SIZE; n++)
            {
                regs->vf->vr[n1][n] = 0;
                regs->vf->vr[n1+1][n] = 0;
            }
            RESET_VR_INUSE(n1, regs);
        }
}

/*-------------------------------------------------------------------*/
/* A6C6 VSVMM - Set Vector Mask Mode                             [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_set_vector_mask_mode)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    VOP_CHECK(regs);

    if(effective_addr2 & 1)
        regs->vf->vsr |= VSR_M;
    else
        regs->vf->vsr &= ~VSR_M;
}

/*-------------------------------------------------------------------*/
/* A6C7 VLVXA - Load VIX from Address                            [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_load_vix_from_address)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;

    S_NW(inst, regs, b2, effective_addr2);

    VOP_CHECK(regs);

    regs->psw.cc = ((S32)effective_addr2 == 0) ? 0 :
                   ((S32)effective_addr2 <  0) ? 1 :
                   ((S32)effective_addr2 <  (S32)VECTOR_COUNT(regs))
                                               ? 2
                                               : 3;

    n = (S32)effective_addr2 < 0 ? 0 :
        (S32)effective_addr2 > VECTOR_SECTION_SIZE ?
                 VECTOR_SECTION_SIZE : (S32)effective_addr2;

    regs->vf->vsr &= ~VSR_VIX;
    regs->vf->vsr |= (U64)n << 16;
}

/*-------------------------------------------------------------------*/
/* A6C8 VSTVP - Store Vector Parameters                          [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_store_vector_parameters)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    VOP_CHECK(regs);

    FW_CHECK(effective_addr2, regs);

    /* Store the section size and partial sum number */
    ARCH_DEP(vstore4)(VECTOR_SECTION_SIZE << 16 | VECTOR_PARTIAL_SUM_NUMBER,
                                  effective_addr2, b2, regs);
}

/*-------------------------------------------------------------------*/
/* A6CA VACSV - Save VAC                                         [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_save_vac)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    VOP_CHECK(regs);

    PRIV_CHECK(regs);

    DW_CHECK(effective_addr2, regs);

#if defined(_FEATURE_SIE)
    if(SIE_STATE_BIT_ON(regs, IC3, VACSV))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    ARCH_DEP(vstore8)(regs->vf->vac, effective_addr2, b2, regs);
}

/*-------------------------------------------------------------------*/
/* A6CB VACRS - Restore VAC                                      [S] */
/*-------------------------------------------------------------------*/
DEF_INST(v_restore_vac)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    VOP_CHECK(regs);

    PRIV_CHECK(regs);

    DW_CHECK(effective_addr2, regs);

#if defined(_FEATURE_SIE)
    if(SIE_STATE_BIT_ON(regs, IC3, VACRS))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    regs->vf->vac = ARCH_DEP(vfetch8)(effective_addr2, b2, regs) & VAC_MASK;
}

#endif /* defined( FEATURE_S370_S390_VECTOR_FACILITY ) */

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "vector.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "vector.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
